#pragma once

#if !defined(__cplusplus) && !defined(inline)
#define inline __inline
#endif

#define GC_EVENT_FLAGS (EVENT_MODIFY_STATE | SYNCHRONIZE)
#define GC_MUTEX_FLAGS (SYNCHRONIZE)

static inline HANDLE create_event(const char *name)
{
	return CreateEventA(NULL, false, false, name);
}

static inline HANDLE open_event(const char *name)
{
	return OpenEventA(GC_EVENT_FLAGS, false, name);
}

static inline HANDLE create_mutex(const char *name)
{
	return CreateMutexA(NULL, false, name);
}

static inline HANDLE open_mutex(const char *name)
{
	return OpenMutexA(GC_MUTEX_FLAGS, false, name);
}

static inline HANDLE create_event_plus_id(const char *name, DWORD id)
{
	char new_name[64];
	sprintf(new_name, "%s%lu", name, id);
	return create_event(new_name);
}

static inline HANDLE open_event_plus_id(const char *name, DWORD id)
{
	char new_name[64];
	sprintf(new_name, "%s%lu", name, id);
	return open_event(new_name);
}

static inline HANDLE create_mutex_plus_id(const char *name, DWORD id)
{
	char new_name[64];
	sprintf(new_name, "%s%lu", name, id);
	return create_mutex(new_name);
}

static inline HANDLE open_mutex_plus_id(const char *name, DWORD id)
{
	char new_name[64];
	sprintf(new_name, "%s%lu", name, id);
	return open_mutex(new_name);
}

static inline bool object_signalled(HANDLE event)
{
	if (!event)
		return false;

	return WaitForSingleObject(event, 0) == WAIT_OBJECT_0;
}

