#include <stdio.h>
#include <windows.h>
#include <shellapi.h>
#include <stdbool.h>
#include "../obfuscate.h"

static void load_debug_privilege(void)
{
	const DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
	TOKEN_PRIVILEGES tp;
	HANDLE token;
	LUID val;

	if (!OpenProcessToken(GetCurrentProcess(), flags, &token)) {
		return;
	}

	if (!!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, false, &tp,
				sizeof(tp), NULL, NULL);
	}

	CloseHandle(token);
}

typedef HHOOK (WINAPI *set_windows_hook_ex_t)(int, HOOKPROC, HINSTANCE, DWORD);

#define RETRY_INTERVAL_MS      500
#define TOTAL_RETRY_TIME_MS    4000
#define RETRY_COUNT            (TOTAL_RETRY_TIME_MS / RETRY_INTERVAL_MS)

static int inject_library_safe(DWORD thread_id, const char *dll)
{
	HMODULE user32 = GetModuleHandleW(L"USER32");
	set_windows_hook_ex_t set_windows_hook_ex;
	HMODULE lib = LoadLibraryA(dll);
	LPVOID proc;
	HHOOK hook;
	size_t i;

	if (!lib || !user32) {
		return -3;
	}

#ifdef _WIN64
	proc = GetProcAddress(lib, "dummy_debug_proc");
#else
	proc = GetProcAddress(lib, "_dummy_debug_proc@12");
#endif

	if (!proc) {
		return -4;
	}

	set_windows_hook_ex = get_obfuscated_func(user32, "[bs^fbkmwuKfmfOvI",
			0xEAD293602FCF9778ULL);

	hook = set_windows_hook_ex(WH_GETMESSAGE, proc, lib, thread_id);
	if (!hook) {
		return -5;
	}

	/* SetWindowsHookEx does not inject the library in to the target
	 * process unless the event associated with it has occurred, so
	 * repeatedly send the hook message to start the hook at small
	 * intervals to signal to SetWindowsHookEx to process the message and
	 * therefore inject the library in to the target process.  Repeating
	 * this is mostly just a precaution. */

	for (i = 0; i < RETRY_COUNT; i++) {
		Sleep(RETRY_INTERVAL_MS);
		PostThreadMessage(thread_id, WM_USER + 432, 0, (LPARAM)hook);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	DWORD thread_id;

	load_debug_privilege();

	if (argc < 3) {
		return -1;
	}

	thread_id = strtol(argv[2], NULL, 10);
	if (thread_id == 0) {
		return -2;
	}

	return inject_library_safe(thread_id, argv[1]);
}
