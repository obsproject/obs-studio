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
#include <mmsystem.h>
#include <shellapi.h>
#include <shlobj.h>

#include "base.h"
#include "platform.h"
#include "darray.h"
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

	if (!path)
		return NULL;

	dstr_init_copy(&dll_name, path);
	if (!dstr_find(&dll_name, ".dll"))
		dstr_cat(&dll_name, ".dll");

	os_utf8_to_wcs_ptr(dll_name.array, 0, &wpath);
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

union time_data {
	FILETIME           ft;
	unsigned long long val;
};

struct os_cpu_usage_info {
	union time_data last_time, last_sys_time, last_user_time;
	DWORD core_count;
};

os_cpu_usage_info_t os_cpu_usage_info_start(void)
{
	struct os_cpu_usage_info *info = bzalloc(sizeof(*info));
	SYSTEM_INFO           si;
	FILETIME              dummy;

	GetSystemInfo(&si);
	GetSystemTimeAsFileTime(&info->last_time.ft);
	GetProcessTimes(GetCurrentProcess(), &dummy, &dummy,
			&info->last_sys_time.ft, &info->last_user_time.ft);
	info->core_count = si.dwNumberOfProcessors;

	return info;
}

double os_cpu_usage_info_query(os_cpu_usage_info_t info)
{
	union time_data cur_time, cur_sys_time, cur_user_time;
	FILETIME        dummy;
	double          percent;

	if (!info)
		return 0.0;

	GetSystemTimeAsFileTime(&cur_time.ft);
	GetProcessTimes(GetCurrentProcess(), &dummy, &dummy,
			&cur_sys_time.ft, &cur_user_time.ft);

	percent = (double)(cur_sys_time.val - info->last_sys_time.val +
		(cur_user_time.val - info->last_user_time.val));
	percent /= (double)(cur_time.val - info->last_time.val);
	percent /= (double)info->core_count;

	info->last_time.val      = cur_time.val;
	info->last_sys_time.val  = cur_sys_time.val;
	info->last_user_time.val = cur_user_time.val;

	return percent * 100.0;
}

void os_cpu_usage_info_destroy(os_cpu_usage_info_t info)
{
	if (info)
		bfree(info);
}

bool os_sleepto_ns(uint64_t time_target)
{
	uint64_t t = os_gettime_ns();
	uint32_t milliseconds;

	if (t >= time_target)
		return false;

	milliseconds = (uint32_t)((time_target - t)/1000000);
	if (milliseconds > 1)
		Sleep(milliseconds-1);

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

	os_wcs_to_utf8_ptr(path_utf16, 0, &ptr);
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

	if (!os_utf8_to_wcs_ptr(path, 0, &path_utf16))
		return false;

	hFind = FindFirstFileW(path_utf16, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	bfree(path_utf16);
	return hFind != INVALID_HANDLE_VALUE;
}

struct os_dir {
	HANDLE           handle;
	WIN32_FIND_DATA  wfd;
	bool             first;
	struct os_dirent out;
};

os_dir_t os_opendir(const char *path)
{
	struct dstr     path_str = {0};
	struct os_dir   *dir     = NULL;
	WIN32_FIND_DATA wfd;
	HANDLE          handle;
	wchar_t         *w_path;

	dstr_copy(&path_str, path);
	dstr_cat(&path_str, "/*.*");

	if (os_utf8_to_wcs_ptr(path_str.array, path_str.len, &w_path) > 0) {
		handle = FindFirstFileW(w_path, &wfd);
		if (handle != INVALID_HANDLE_VALUE) {
			dir         = bzalloc(sizeof(struct os_dir));
			dir->handle = handle;
			dir->first  = true;
			dir->wfd    = wfd;
		}

		bfree(w_path);
	}

	dstr_free(&path_str);

	return dir;
}

static inline bool is_dir(WIN32_FIND_DATA *wfd)
{
	return !!(wfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

struct os_dirent *os_readdir(os_dir_t dir)
{
	if (!dir)
		return NULL;

	if (dir->first) {
		dir->first = false;
	} else {
		if (!FindNextFileW(dir->handle, &dir->wfd))
			return NULL;
	}

	os_wcs_to_utf8(dir->wfd.cFileName, 0, dir->out.d_name,
			sizeof(dir->out.d_name));

	dir->out.directory = is_dir(&dir->wfd);

	return &dir->out;
}

void os_closedir(os_dir_t dir)
{
	if (dir) {
		FindClose(dir->handle);
		bfree(dir);
	}
}

static void make_globent(struct os_globent *ent, WIN32_FIND_DATA *wfd,
		const char *pattern)
{
	struct dstr name = {0};
	struct dstr path = {0};
	char *slash;

	dstr_from_wcs(&name, wfd->cFileName);
	dstr_copy(&path, pattern);
	slash = strrchr(path.array, '/');
	if (slash)
		dstr_resize(&path, slash + 1 - path.array);
	else
		dstr_free(&path);

	dstr_cat_dstr(&path, &name);
	ent->path = path.array;
	ent->directory = is_dir(wfd);

	dstr_free(&name);
}

int os_glob(const char *pattern, int flags, os_glob_t *pglob)
{
	DARRAY(struct os_globent) files;
	HANDLE                    handle;
	WIN32_FIND_DATA           wfd;
	int                       ret = -1;
	os_glob_t                 out = NULL;
	wchar_t                   *w_path;

	da_init(files);

	if (os_utf8_to_wcs_ptr(pattern, 0, &w_path) > 0) {
		handle = FindFirstFileW(w_path, &wfd);
		if (handle != INVALID_HANDLE_VALUE) {
			do {
				struct os_globent ent = {0};
				make_globent(&ent, &wfd, pattern);
				if (ent.path)
					da_push_back(files, &ent);
			} while (FindNextFile(handle, &wfd));
			FindClose(handle);

			*pglob = bmalloc(sizeof(**pglob));
			(*pglob)->gl_pathc = files.num;
			(*pglob)->gl_pathv = files.array;

			ret = 0;
		}

		bfree(w_path);
	}

	if (ret != 0)
		*pglob = NULL;

	UNUSED_PARAMETER(flags);
	return ret;
}

void os_globfree(os_glob_t pglob)
{
	if (pglob) {
		for (size_t i = 0; i < pglob->gl_pathc; i++)
			bfree(pglob->gl_pathv[i].path);
		bfree(pglob->gl_pathv);
		bfree(pglob);
	}
}

int os_unlink(const char *path)
{
	wchar_t *w_path;
	bool success;

	os_utf8_to_wcs_ptr(path, 0, &w_path);
	if (!w_path)
		return -1;

	success = !!DeleteFileW(w_path);
	bfree(w_path);

	return success ? 0 : -1;
}

int os_mkdir(const char *path)
{
	wchar_t *path_utf16;
	BOOL success;

	if (!os_utf8_to_wcs_ptr(path, 0, &path_utf16))
		return MKDIR_ERROR;

	success = CreateDirectory(path_utf16, NULL);
	bfree(path_utf16);

	if (!success)
		return (GetLastError() == ERROR_ALREADY_EXISTS) ?
			MKDIR_EXISTS : MKDIR_ERROR;

	return MKDIR_SUCCESS;
}


BOOL WINAPI DllMain(HINSTANCE hinst_dll, DWORD reason, LPVOID reserved)
{
	switch (reason) {

	case DLL_PROCESS_ATTACH:
		timeBeginPeriod(1);
#ifdef PTW32_STATIC_LIB
		pthread_win32_process_attach_np();
#endif
		break;

	case DLL_PROCESS_DETACH:
		timeEndPeriod(1);
#ifdef PTW32_STATIC_LIB
		pthread_win32_process_detach_np();
#endif
		break;

	case DLL_THREAD_ATTACH:
#ifdef PTW32_STATIC_LIB
		pthread_win32_thread_attach_np();
#endif
		break;

	case DLL_THREAD_DETACH:
#ifdef PTW32_STATIC_LIB
		pthread_win32_thread_detach_np();
#endif
		break;
	}

	UNUSED_PARAMETER(hinst_dll);
	UNUSED_PARAMETER(reserved);
	return true;
}
