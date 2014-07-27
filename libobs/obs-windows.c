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

#include "util/platform.h"
#include "util/dstr.h"
#include "obs.h"
#include "obs-internal.h"

#include <windows.h>

const char *get_module_extension(void)
{
	return ".dll";
}

#ifdef _WIN64
#define BIT_STRING "64bit"
#else
#define BIT_STRING "32bit"
#endif

static const char *module_bin[] = {
	"obs-plugins/" BIT_STRING,
	"../../obs-plugins/" BIT_STRING,
};

static const char *module_data[] = {
	"data/%module%",
	"../../data/obs-plugins/%module%"
};

static const int module_patterns_size =
	sizeof(module_bin)/sizeof(module_bin[0]);

void add_default_module_paths(void)
{
	for (int i = 0; i < module_patterns_size; i++)
		obs_add_module_path(module_bin[i], module_data[i]);
}

/* on windows, points to [base directory]/data/libobs */
char *find_libobs_data_file(const char *file)
{
	struct dstr path;
	dstr_init(&path);

	if (check_path(file, "data/libobs/", &path))
		return path.array;

	if (check_path(file, "../../data/libobs/", &path))
		return path.array;

	dstr_free(&path);
	return NULL;
}

static void log_processor_info(void)
{
	HKEY    key;
	wchar_t data[1024];
	char    *str = NULL;
	DWORD   size, speed;
	LSTATUS status;

	memset(data, 0, 1024);

	status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
			L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
			&key);
	if (status != ERROR_SUCCESS)
		return;

	size = 1024;
	status = RegQueryValueExW(key, L"ProcessorNameString", NULL, NULL,
			(LPBYTE)data, &size);
	if (status == ERROR_SUCCESS) {
		os_wcs_to_utf8_ptr(data, 0, &str);
		blog(LOG_INFO, "CPU Name: %s", str);
		bfree(str);
	}

	size = sizeof(speed);
	status = RegQueryValueExW(key, L"~MHz", NULL, NULL, (LPBYTE)&speed,
			&size);
	if (status == ERROR_SUCCESS)
		blog(LOG_INFO, "CPU Speed: %dMHz", speed);

	RegCloseKey(key);
}

static DWORD num_logical_cores(ULONG_PTR mask)
{
	DWORD     left_shift    = sizeof(ULONG_PTR) * 8 - 1;
	DWORD     bit_set_count = 0;
	ULONG_PTR bit_test      = (ULONG_PTR)1 << left_shift;

	for (DWORD i = 0; i <= left_shift; ++i) {
		bit_set_count += ((mask & bit_test) ? 1 : 0);
		bit_test      /= 2;
	}

	return bit_set_count;
}

static void log_processor_cores(void)
{
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION info = NULL, temp = NULL;
	DWORD len = 0;

	GetLogicalProcessorInformation(info, &len);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return;

	info = malloc(len);

	if (GetLogicalProcessorInformation(info, &len)) {
		DWORD num            = len / sizeof(*info);
		int   physical_cores = 0;
		int   logical_cores  = 0;

		temp = info;

		for (DWORD i = 0; i < num; i++) {
			if (temp->Relationship == RelationProcessorCore) {
				ULONG_PTR mask = temp->ProcessorMask;

				physical_cores++;
				logical_cores += num_logical_cores(mask);
			}

			temp++;
		}

		blog(LOG_INFO, "Physical Cores: %d, Logical Cores: %d",
				physical_cores, logical_cores);
	}

	free(info);
}

static void log_available_memory(void)
{
	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);

#ifdef _WIN64
	const char *note = "";
#else
	const char *note = " (NOTE: 4 gigs max is normal for 32bit programs)";
#endif

	blog(LOG_INFO, "Physical Memory: %ldMB Total, %ldMB Free%s",
			ms.dwTotalPhys / 1048576,
			ms.dwAvailPhys / 1048576,
			note);
}

static void log_windows_version(void)
{
	OSVERSIONINFOW osvi;
	char           *build = NULL;

	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionExW(&osvi);

	os_wcs_to_utf8_ptr(osvi.szCSDVersion, 0, &build);
	blog(LOG_INFO, "Windows Version: %u.%u Build %u %s",
			osvi.dwMajorVersion,
			osvi.dwMinorVersion,
			osvi.dwBuildNumber,
			build);

	bfree(build);
}

void log_system_info(void)
{
	log_processor_info();
	log_processor_cores();
	log_available_memory();
	log_windows_version();
}
