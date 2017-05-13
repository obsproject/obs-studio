/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <algorithm>
#include <sstream>
#include "obs-config.h"
#include "obs-app.hpp"
#include "platform.hpp"
using namespace std;

#include <util/windows/win-version.h>
#include <util/platform.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <Dwmapi.h>
#include <psapi.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>

static inline bool check_path(const char* data, const char *path,
		string &output)
{
	ostringstream str;
	str << path << data;
	output = str.str();

	printf("Attempted path: %s\n", output.c_str());

	return os_file_exists(output.c_str());
}

bool GetDataFilePath(const char *data, string &output)
{
	if (check_path(data, "data/obs-studio/", output))
		return true;

	return check_path(data, OBS_DATA_PATH "/obs-studio/", output);
}

bool InitApplicationBundle()
{
	return true;
}

string GetDefaultVideoSavePath()
{
	wchar_t path_utf16[MAX_PATH];
	char    path_utf8[MAX_PATH]  = {};

	SHGetFolderPathW(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT,
			path_utf16);

	os_wcs_to_utf8(path_utf16, wcslen(path_utf16), path_utf8, MAX_PATH);
	return string(path_utf8);
}

static vector<string> GetUserPreferredLocales()
{
	vector<string> result;

	ULONG num, length = 0;
	if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &num,
				nullptr, &length))
		return result;

	vector<wchar_t> buffer(length);
	if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &num,
				&buffer.front(), &length))
		return result;

	result.reserve(num);
	auto start = begin(buffer);
	auto end_  = end(buffer);
	decltype(start) separator;
	while ((separator = find(start, end_, 0)) != end_) {
		if (result.size() == num)
			break;

		char conv[MAX_PATH] = {};
		os_wcs_to_utf8(&*start, separator - start, conv, MAX_PATH);

		result.emplace_back(conv);

		start = separator + 1;
	}

	return result;
}

vector<string> GetPreferredLocales()
{
	vector<string> windows_locales = GetUserPreferredLocales();
	auto obs_locales = GetLocaleNames();
	auto windows_to_obs = [&obs_locales](string windows) {
		string lang_match;

		for (auto &locale_pair : obs_locales) {
			auto &locale = locale_pair.first;
			if (locale == windows.substr(0, locale.size()))
				return locale;

			if (lang_match.size())
				continue;

			if (locale.substr(0, 2) == windows.substr(0, 2))
				lang_match = locale;
		}

		return lang_match;
	};

	vector<string> result;
	result.reserve(obs_locales.size());

	for (const string &locale : windows_locales) {
		string match = windows_to_obs(locale);
		if (!match.size())
			continue;

		if (find(begin(result), end(result), match) != end(result))
			continue;

		result.emplace_back(match);
	}

	return result;
}

uint32_t GetWindowsVersion()
{
	static uint32_t ver = 0;

	if (ver == 0) {
		struct win_version_info ver_info;

		get_win_ver(&ver_info);
		ver = (ver_info.major << 8) | ver_info.minor;
	}

	return ver;
}

void SetAeroEnabled(bool enable)
{
	static HRESULT (WINAPI *func)(UINT) = nullptr;
	static bool failed = false;

	if (!func) {
		if (failed) {
			return;
		}

		HMODULE dwm = LoadLibraryW(L"dwmapi");
		if (!dwm) {
			failed = true;
			return;
		}

		func = reinterpret_cast<decltype(func)>(GetProcAddress(dwm,
						"DwmEnableComposition"));
		if (!func) {
			failed = true;
			return;
		}
	}

	func(enable ? DWM_EC_ENABLECOMPOSITION : DWM_EC_DISABLECOMPOSITION);
}

bool IsAlwaysOnTop(QWidget *window)
{
	DWORD exStyle = GetWindowLong((HWND)window->winId(), GWL_EXSTYLE);
	return (exStyle & WS_EX_TOPMOST) != 0;
}

void SetAlwaysOnTop(QWidget *window, bool enable)
{
	HWND hwnd = (HWND)window->winId();
	SetWindowPos(hwnd, enable ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void SetProcessPriority(const char *priority)
{
	if (!priority)
		return;

	if (strcmp(priority, "High") == 0)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else if (strcmp(priority, "AboveNormal") == 0)
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
	else if (strcmp(priority, "Normal") == 0)
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	else if (strcmp(priority, "Idle") == 0)
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
}

void SetWin32DropStyle(QWidget *window)
{
	HWND hwnd = (HWND)window->winId();
	LONG_PTR ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	ex_style |= WS_EX_ACCEPTFILES;
	SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex_style);
}

bool DisableAudioDucking(bool disable)
{
	ComPtr<IMMDeviceEnumerator>   devEmum;
	ComPtr<IMMDevice>             device;
	ComPtr<IAudioSessionManager2> sessionManager2;
	ComPtr<IAudioSessionControl>  sessionControl;
	ComPtr<IAudioSessionControl2> sessionControl2;

	HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator),
			nullptr, CLSCTX_INPROC_SERVER,
			__uuidof(IMMDeviceEnumerator),
			(void **)&devEmum);
	if (FAILED(result))
		return false;

	result = devEmum->GetDefaultAudioEndpoint(eRender, eConsole, &device);
	if (FAILED(result))
		return false;

	result = device->Activate(__uuidof(IAudioSessionManager2),
			CLSCTX_INPROC_SERVER, nullptr,
			(void **)&sessionManager2);
	if (FAILED(result))
		return false;

	result = sessionManager2->GetAudioSessionControl(nullptr, 0,
			&sessionControl);
	if (FAILED(result))
		return false;

	result = sessionControl->QueryInterface(&sessionControl2);
	if (FAILED(result))
		return false;

	result = sessionControl2->SetDuckingPreference(disable);
	return SUCCEEDED(result);
}

uint64_t CurrentMemoryUsage()
{
	PROCESS_MEMORY_COUNTERS pmc = {};
	pmc.cb = sizeof(pmc);

	if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		return 0;

	return (uint64_t)pmc.WorkingSetSize;
}
