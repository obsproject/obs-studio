#include "caffeine-foreground-process.h"

#include <obs.h>

#ifndef _WIN32
char *get_foreground_process_name()
{
	return NULL;
}
#else

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Psapi.h>
#include <Shlwapi.h>

char *get_foreground_process_name()
{
	HWND window = GetForegroundWindow();
	if (!window)
		return NULL;
	DWORD pid;
	if (!GetWindowThreadProcessId(window, &pid))
		return NULL;
	HANDLE process =
		OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!process)
		return NULL;

	DWORD const buffer_size = 4096;
	char *filename = NULL;
	char *full_path = bzalloc(buffer_size);
	if (GetProcessImageFileNameA(process, full_path, buffer_size)) {
		PathRemoveExtensionA(full_path);
		filename = bstrdup(PathFindFileNameA(full_path));
	}

	bfree(full_path);
	CloseHandle(process);
	return filename;
}

#endif // _WIN32
