#pragma once

bool thread_is_suspended(DWORD process_id, DWORD thread_id);
HANDLE nt_create_mutex(const wchar_t *name);
HANDLE nt_open_mutex(const wchar_t *name);
HANDLE nt_open_event(const wchar_t *name);
HANDLE nt_open_map(const wchar_t *name);
