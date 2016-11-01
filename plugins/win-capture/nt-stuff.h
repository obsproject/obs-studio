#pragma once

#include <winternl.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(status) ((NTSTATUS)(status) >= 0)
#endif

#define init_named_attribs(o, name) \
	do { \
		(o)->Length = sizeof(*(o)); \
		(o)->ObjectName = name; \
		(o)->RootDirectory = NULL; \
		(o)->Attributes = 0; \
		(o)->SecurityDescriptor = NULL; \
		(o)->SecurityQualityOfService = NULL; \
	} while (false)

typedef void (WINAPI *RTLINITUNICODESTRINGFUNC)(PCUNICODE_STRING pstr, const wchar_t *lpstrName);
typedef NTSTATUS (WINAPI *NTOPENFUNC)(PHANDLE phandle, ACCESS_MASK access, POBJECT_ATTRIBUTES objattr);

static FARPROC get_nt_func(const char *name)
{
	static bool initialized = false;
	static HANDLE ntdll = NULL;
	if (!initialized) {
		ntdll = GetModuleHandleW(L"ntdll");
		initialized = true;
	}

	return GetProcAddress(ntdll, name);
}

static void nt_set_last_error(NTSTATUS status)
{
	static bool initialized = false;
	static RTLNTSTATUSTODOSERRORFUNC func = NULL;

	if (!initialized) {
		func = (RTLNTSTATUSTODOSERRORFUNC)get_nt_func(
				"RtlNtStatusToDosError");
		initialized = true;
	}

	if (func)
		SetLastError(func(status));
}

static void rtl_init_str(UNICODE_STRING *unistr, const wchar_t *str)
{
	static bool initialized = false;
	static RTLINITUNICODESTRINGFUNC func = NULL;

	if (!initialized) {
		func = (RTLINITUNICODESTRINGFUNC)get_nt_func(
				"RtlInitUnicodeString");
		initialized = true;
	}

	if (func)
		func(unistr, str);
}

#define MAKE_NT_OPEN_FUNC(func_name, nt_name, access) \
static HANDLE func_name(const wchar_t *name) \
{ \
	static bool initialized = false; \
	static NTOPENFUNC open = NULL; \
	HANDLE handle; \
	NTSTATUS status; \
	UNICODE_STRING unistr; \
	OBJECT_ATTRIBUTES attr; \
\
	if (!initialized) { \
		open = (NTOPENFUNC)get_nt_func(#nt_name); \
		initialized = true; \
	} \
\
	if (!open) \
		return NULL; \
\
	rtl_init_str(&unistr, name); \
	init_named_attribs(&attr, &unistr); \
\
	status = open(&handle, access, &attr); \
	if (NT_SUCCESS(status)) \
		return handle; \
	nt_set_last_error(status); \
	return NULL; \
}

MAKE_NT_OPEN_FUNC(nt_open_mutex, NtOpenMutant, SYNCHRONIZE)
MAKE_NT_OPEN_FUNC(nt_open_event, NtOpenEvent, EVENT_MODIFY_STATE | SYNCHRONIZE)
MAKE_NT_OPEN_FUNC(nt_open_map, NtOpenSection, FILE_MAP_READ | FILE_MAP_WRITE)
