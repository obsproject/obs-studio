/******************************************************************************
  Copyright (c) 2013 by Hugh Bailey <obs.jim@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

     1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

     2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

     3. This notice may not be removed or altered from any source
     distribution.
******************************************************************************/

#include <windows.h>
#include "base.h"
#include "platform.h"
#include "bmem.h"

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
	wchar_t *wpath;
	HMODULE h_library = NULL;

	os_utf8_to_wcs(path, 0, &wpath);
	h_library = LoadLibraryW(wpath);
	bfree(wpath);

	if (!h_library)
		blog(LOG_INFO, "LoadLibrary failed for '%s', error: %u",
				path, GetLastError());

	return h_library;
}

void *os_dlsym(void *module, const char *func)
{
	return (void*)GetProcAddress(module, func);
}

void os_dlclose(void *module)
{
	FreeLibrary(module);
}

void os_sleepto_ns(uint64_t time_target)
{
	uint64_t t = os_gettime_ns();
	uint32_t milliseconds;

	if (t >= time_target)
		return;

	milliseconds = (uint32_t)((time_target - t)/1000000);
	if (milliseconds > 1)
		os_sleep_ms(milliseconds);

	for (;;) {
		t = os_gettime_ns();
		if (t >= time_target)
			return;

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

uint64_t os_gettime_ms(void)
{
	LARGE_INTEGER current_time;
	uint64_t time_val;

	QueryPerformanceCounter(&current_time);
	time_val = current_time.QuadPart;
	time_val *= 1000;
	time_val /= get_clockfreq();

	return time_val;
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
