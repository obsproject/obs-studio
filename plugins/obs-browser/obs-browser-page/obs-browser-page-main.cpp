/******************************************************************************
 Copyright (C) 2014 by John R. Bradley <jrb@turrettech.com>
 Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "cef-headers.hpp"
#include "browser-app.hpp"

#ifdef _WIN32
#include <windows.h>
#include <string>
#include <thread>

// GPU hint exports for AMD/NVIDIA laptops
#ifdef _MSC_VER
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

static HANDLE shutdown_event = nullptr;
static bool thread_initialized = false;

DECLARE_HANDLE(OBS_DPI_AWARENESS_CONTEXT);
#define OBS_DPI_AWARENESS_CONTEXT_UNAWARE ((OBS_DPI_AWARENESS_CONTEXT)-1)
#define OBS_DPI_AWARENESS_CONTEXT_SYSTEM_AWARE ((OBS_DPI_AWARENESS_CONTEXT)-2)
#define OBS_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ((OBS_DPI_AWARENESS_CONTEXT)-3)
#define OBS_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((OBS_DPI_AWARENESS_CONTEXT)-4)

static bool SetHighDPIv2Scaling()
{
	static BOOL(WINAPI * func)(OBS_DPI_AWARENESS_CONTEXT) = nullptr;
	func = reinterpret_cast<decltype(func)>(
		GetProcAddress(GetModuleHandleW(L"USER32"), "SetProcessDpiAwarenessContext"));
	if (!func) {
		return false;
	}

	return !!func(OBS_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

static void shutdown_check_thread(DWORD parent_pid, DWORD main_thread_id)
{
	HANDLE parent = OpenProcess(SYNCHRONIZE, false, parent_pid);
	if (!parent) {
		return;
	}

	HANDLE handles[2] = {parent, shutdown_event};

	DWORD ret = WaitForMultipleObjects(2, handles, false, INFINITE);
	if (ret == WAIT_OBJECT_0) {
		PostThreadMessage(main_thread_id, WM_QUIT, 0, 0);
		ret = WaitForSingleObject(shutdown_event, 5000);
		if (ret != WAIT_OBJECT_0) {
			TerminateProcess(GetCurrentProcess(), (UINT)-1);
		}
	}

	CloseHandle(parent);
}

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	PROCESS_POWER_THROTTLING_STATE PowerThrottling;
	PowerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
	PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
	PowerThrottling.StateMask = 0;

	SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &PowerThrottling, sizeof(PowerThrottling));

	std::thread shutdown_check;

	CefMainArgs mainArgs(nullptr);
#if CHROME_VERSION_BUILD < 5615
	if (!SetHighDPIv2Scaling())
		CefEnableHighDPISupport();
#endif

	CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
	command_line->InitFromString(::GetCommandLineW());

	std::string parent_pid_str = command_line->GetSwitchValue("parent_pid");
	std::string current_pid = std::to_string(GetCurrentProcessId());
	if (!parent_pid_str.empty()) {
		shutdown_event = CreateEvent(nullptr, true, false, nullptr);
		DWORD parent_pid = (DWORD)std::stoi(parent_pid_str);
		shutdown_check = std::thread(shutdown_check_thread, parent_pid, GetCurrentThreadId());
		thread_initialized = true;
	}

#else
#if defined(NO_STACK_PROTECTOR)
NO_STACK_PROTECTOR
#endif
int main(int argc, char *argv[])
{
#if defined(__APPLE__) && !defined(ENABLE_BROWSER_LEGACY)
	CefScopedLibraryLoader library_loader;
	if (!library_loader.LoadInHelper())
		return 1;
#endif
	CefMainArgs mainArgs(argc, argv);
#endif
	CefRefPtr<BrowserApp> mainApp(new BrowserApp());

	int ret = CefExecuteProcess(mainArgs, mainApp.get(), NULL);

#ifdef _WIN32
	/* chromium browser subprocesses actually have TerminateProcess called
	 * on them for whatever reason, so it's unlikely this code will ever
	 * get called, but better to be safe than sorry */
	if (thread_initialized) {
		SetEvent(shutdown_event);
		shutdown_check.join();
	}
	if (shutdown_event) {
		CloseHandle(shutdown_event);
	}
#endif
	return ret;
}
