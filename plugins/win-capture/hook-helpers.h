#pragma once

#if !defined(__cplusplus) && !defined(inline)
#define inline __inline
#endif

static inline HANDLE get_event(const char *name)
{
	HANDLE event = CreateEventA(NULL, false, false, name);
	if (!event)
		event = OpenEventA(EVENT_ALL_ACCESS, false, name);

	return event;
}

static inline HANDLE get_mutex(const char *name)
{
	HANDLE event = CreateMutexA(NULL, false, name);
	if (!event)
		event = OpenMutexA(MUTEX_ALL_ACCESS, false, name);

	return event;
}

static inline HANDLE get_event_plus_id(const char *name, DWORD id)
{
	char new_name[64];
	sprintf(new_name, "%s%d", name, id);
	return get_event(new_name);
}

static inline HANDLE get_mutex_plus_id(const char *name, DWORD id)
{
	char new_name[64];
	sprintf(new_name, "%s%d", name, id);
	return get_mutex(new_name);
}

static inline bool object_signalled(HANDLE event)
{
	if (!event)
		return false;

	return WaitForSingleObject(event, 0) == WAIT_OBJECT_0;
}

