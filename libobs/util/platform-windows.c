/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "base.h"
#include "platform.h"
#include "dstr.h"

#include "../../deps/w32-pthreads/pthread.h"

static bool have_clockfreq = false;
static LARGE_INTEGER clock_freq;
static uint32_t winver = 0;

static inline uint64_t get_clockfreq(void)
{
	if (!have_clockfreq)
		QueryPerformanceFrequency(&clock_freq);
	return clock_freq.QuadPart;
}

static inline uint32_t get_winver(void)
{
	if (!winver) {
		OSVERSIONINFO osvi;
		memset(&osvi, 0, sizeof(osvi));
		winver = (osvi.dwMajorVersion << 16) | (osvi.dwMinorVersion);
	}

	return winver;	
}

void *os_dlopen(const char *path)
{
	struct dstr dll_name;
	wchar_t *wpath;
	HMODULE h_library = NULL;

	dstr_init_copy(&dll_name, path);
	if (!dstr_find(&dll_name, ".dll"))
		dstr_cat(&dll_name, ".dll");

	os_utf8_to_wcs(dll_name.array, 0, &wpath);
	h_library = LoadLibraryW(wpath);
	bfree(wpath);
	dstr_free(&dll_name);

	if (!h_library)
		blog(LOG_INFO, "LoadLibrary failed for '%s', error: %u",
				path, GetLastError());

	return h_library;
}

void *os_dlsym(void *module, const char *func)
{
	void *handle;

	handle = (void*)GetProcAddress(module, func);

	return handle;
}

void os_dlclose(void *module)
{
	FreeLibrary(module);
}

bool os_sleepto_ns(uint64_t time_target)
{
	uint64_t t = os_gettime_ns();
	uint32_t milliseconds;

	if (t >= time_target)
		return false;

	milliseconds = (uint32_t)((time_target - t)/1000000);
	if (milliseconds > 1)
		os_sleep_ms(milliseconds);

	for (;;) {
		t = os_gettime_ns();
		if (t >= time_target)
			return true;

#if 1
		Sleep(1);
#else
		Sleep(0);
#endif
	}
}

void os_sleep_ms(uint32_t duration)
{
	/* windows 8+ appears to have decreased sleep precision */
	if (get_winver() >= 0x0602 && duration > 0)
		duration--;

	Sleep(duration);
}

uint64_t os_gettime_ns(void)
{
	LARGE_INTEGER current_time;
	double time_val;

	QueryPerformanceCounter(&current_time);
	time_val = (double)current_time.QuadPart;
	time_val *= 1000000000.0;
	time_val /= (double)get_clockfreq();

	return (uint64_t)time_val;
}

/* returns %appdata%\[name] on windows */
char *os_get_config_path(const char *name)
{
	char *ptr;
	wchar_t path_utf16[MAX_PATH];
	struct dstr path;

	SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
			path_utf16);

	os_wcs_to_utf8(path_utf16, 0, &ptr);
	dstr_init_move_array(&path, ptr);
	dstr_cat(&path, "\\");
	dstr_cat(&path, name);
	return path.array;
}

bool os_file_exists(const char *path)
{
	WIN32_FIND_DATAW wfd;
	HANDLE hFind;
	wchar_t *path_utf16;

	if (!os_utf8_to_wcs(path, 0, &path_utf16))
		return false;

	hFind = FindFirstFileW(path_utf16, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	bfree(path_utf16);
	return hFind != INVALID_HANDLE_VALUE;
}

int os_mkdir(const char *path)
{
	wchar_t *path_utf16;
	BOOL success;

	if (!os_utf8_to_wcs(path, 0, &path_utf16))
		return MKDIR_ERROR;

	success = CreateDirectory(path_utf16, NULL);
	bfree(path_utf16);

	if (!success)
		return (GetLastError() == ERROR_ALREADY_EXISTS) ?
			MKDIR_EXISTS : MKDIR_ERROR;

	return MKDIR_SUCCESS;
}

#ifdef PTW32_STATIC_LIB

BOOL WINAPI DllMain(HINSTANCE hinst_dll, DWORD reason, LPVOID reserved)
{
	switch (reason) {

	case DLL_PROCESS_ATTACH:
		pthread_win32_process_attach_np();
		break;

	case DLL_PROCESS_DETACH:
		pthread_win32_process_detach_np();
		break;

	case DLL_THREAD_ATTACH:
		pthread_win32_thread_attach_np();
		break;

	case DLL_THREAD_DETACH:
		pthread_win32_thread_detach_np();
		break;
	}

	return true;
}

#endif
