/******************************************************************************
    Copyright (C) 2023 by Richard Stanway

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

#include <Windows.h>
#include <psapi.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "detours.h"
#include "obs.h"

// Undocumented NT structs / function definitions !
typedef enum _SECTION_INHERIT { ViewShare = 1, ViewUnmap = 2 } SECTION_INHERIT;

typedef enum _SECTION_INFORMATION_CLASS {
	SectionBasicInformation = 0,
	SectionImageInformation
} SECTION_INFORMATION_CLASS;

typedef struct _SECTION_BASIC_INFORMATION {
	PVOID BaseAddress;
	ULONG Attributes;
	LARGE_INTEGER Size;
} SECTION_BASIC_INFORMATION;

typedef NTSTATUS(STDMETHODCALLTYPE *fn_NtMapViewOfSection)(HANDLE, HANDLE, PVOID, ULONG_PTR, SIZE_T, PLARGE_INTEGER,
							   PSIZE_T, SECTION_INHERIT, ULONG, ULONG);

typedef NTSTATUS(STDMETHODCALLTYPE *fn_NtUnmapViewOfSection)(HANDLE, PVOID);

typedef NTSTATUS(STDMETHODCALLTYPE *fn_NtQuerySection)(HANDLE, SECTION_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);

static fn_NtMapViewOfSection ntMap;
static fn_NtUnmapViewOfSection ntUnmap;
static fn_NtQuerySection ntQuery;

#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)

// Method of matching timestamp of DLL in PE header
typedef enum {
	TS_IGNORE = 0,      // Ignore timestamp; block all DLLs with this name
	TS_EQUAL,           // Block only DLL with this exact timestamp
	TS_LESS_THAN,       // Block all DLLs with an earlier timestamp
	TS_GREATER_THAN,    // Block all DLLs with a later timestamp
	TS_ALLOW_ONLY_THIS, // Invert behavior: only allow this specific timestamp
} ts_compare_t;

typedef struct {
	// DLL name, lower case
	const wchar_t *name;

	// Length of name, calculated at startup - leave as zero
	size_t name_len;

	// PE timestamp
	const uint32_t timestamp;

	// How to treat the timestamp field
	const ts_compare_t method;

	// Number of times we've blocked this DLL, for logging purposes
	uint64_t blocked_count;
} blocked_module_t;

/*
 * Note: The name matches at the end of the string based on its length, this allows
 * for matching DLLs that may have generic names but a problematic version only
 * exists in a certain directory. A name should always include a path component
 * so that e.g. fraps.dll doesn't match notfraps.dll.
 */

static blocked_module_t blocked_modules[] = {
	// Dell / Alienware Backup & Recovery, crashes during "Browse" dialogs
	{L"\\dbroverlayiconbackuped.dll", 0, 0, TS_IGNORE},

	// RTSS, no good reason for this to be in OBS
	{L"\\rtsshooks.dll", 0, 0, TS_IGNORE},

	// Dolby Axon overlay
	{L"\\axonoverlay.dll", 0, 0, TS_IGNORE},

	// Action! Recorder Software
	{L"\\action_x64.dll", 0, 0, TS_IGNORE},

	// ASUS GamerOSD, breaks DX11 things
	{L"\\atkdx11disp.dll", 0, 0, TS_IGNORE},

	// Malware
	{L"\\sendori.dll", 0, 0, TS_IGNORE},

	// Astril VPN Proxy, hooks stuff and crashes
	{L"\\asproxy64.dll", 0, 0, TS_IGNORE},

	// Nahimic Audio
	{L"\\nahimicmsidevprops.dll", 0, 0, TS_IGNORE},
	{L"\\nahimicmsiosd.dll", 0, 0, TS_IGNORE},

	// FRAPS hook
	{L"\\fraps64.dll", 0, 0, TS_IGNORE},

	// ASUS GPU TWEAK II OSD
	{L"\\gtii-osd64.dll", 0, 0, TS_IGNORE},
	{L"\\gtii-osd64-vk.dll", 0, 0, TS_IGNORE},

	// EVGA Precision, D3D crashes
	{L"\\pxshw10_x64.dll", 0, 0, TS_IGNORE},

	// Wacom / Other tablet driver, locks up UI
	{L"\\wintab32.dll", 0, 0, TS_IGNORE},

	// MainConcept Image Scaler, crashes in its own thread. Block versions
	// older than the one Elgato uses (2016-02-15).
	{L"\\mc_trans_video_imagescaler.dll", 0, 1455495131, TS_LESS_THAN},

	// Weird Polish banking "security" software, breaks UI
	{L"\\wslbscr64.dll", 0, 0, TS_IGNORE},

	// Various things hooking with EasyHook that probably shouldn't touch OBS
	{L"\\easyhook64.dll", 0, 0, TS_IGNORE},

	// Ultramon
	{L"\\rtsultramonhook.dll", 0, 0, TS_IGNORE},

	// HiAlgo Boost, locks up UI
	{L"\\hookdll.dll", 0, 0, TS_IGNORE},

	// Adobe Core Sync? Crashes NDI.
	{L"\\coresync_x64.dll", 0, 0, TS_IGNORE},

	// Fasso DRM, crashes D3D
	{L"\\f_sps.dll", 0, 0, TS_IGNORE},

	// Korean banking "security" software, crashes randomly
	{L"\\t_prevent64.dll", 0, 0, TS_IGNORE},

	// Bandicam, doesn't unhook cleanly and freezes preview
	// Reference: https://github.com/obsproject/obs-studio/issues/8552
	{L"\\bdcam64.dll", 0, 0, TS_IGNORE},

	// "Citrix ICAService" that crashes during DShow enumeration
	// Reference: https://obsproject.com/forum/threads/165863/
	{L"\\ctxdsendpoints64.dll", 0, 0, TS_IGNORE},

	// Generic named unity capture filter. Unfortunately only a forked version
	// has a critical fix to prevent deadlocks during enumeration. We block
	// all versions since if someone didn't change the DLL name they likely
	// didn't implement the deadlock fix.
	// Reference: https://github.com/schellingb/UnityCapture/commit/2eabf0f
	{L"\\unitycapturefilter64bit.dll", 0, 0, TS_IGNORE},

	// VSeeFace capture filter < v1.13.38b3 without above fix implemented
	{L"\\vseefacecamera64bit.dll", 0, 1666993098, TS_LESS_THAN},

	// VTuber Maker capture filter < 2023-03-13 without above fix implemented
	{L"\\live3dvirtualcam\\lib64_new2.dll", 0, 1678695956, TS_LESS_THAN},

	// Obsolete unfixed versions of VTuber Maker capture filter
	{L"\\live3dvirtualcam\\lib64_new.dll", 0, 0, TS_IGNORE},
	{L"\\live3dvirtualcam\\lib64.dll", 0, 0, TS_IGNORE},

	// VirtualMotionCapture capture filter < 2022-12-18 without above fix
	// Reference: https://github.com/obsproject/obs-studio/issues/8552
	{L"\\vmc_camerafilter64bit.dll", 0, 1671349891, TS_LESS_THAN},

	// HolisticMotionCapture capture filter, not yet patched. Blocking
	// all previous versions in case an update is released.
	// Reference: https://github.com/obsproject/obs-studio/issues/8552
	{L"\\holisticmotioncapturefilter64bit.dll", 0, 1680044549, TS_LESS_THAN},

	// Elgato Stream Deck plugin < 2024-02-01
	// Blocking all previous versions because they have undefined behavior
	// that results in crashes.
	// Reference: https://github.com/obsproject/obs-studio/issues/10245
	{L"\\streamdeckplugin.dll", 0, 1706745600, TS_LESS_THAN},

	// TikTok Live Studio Virtual Camera, causes freezing and other issues during enumeration
	// Different versions seem to be installed in different places, so we have to match on DLL only.
	// Reference: https://www.hanselman.com/blog/webcam-randomly-pausing-in-obs-discord-and-websites-lsvcam-and-tiktok-studio
	{L"\\lsvcam.dll", 0, 0, TS_IGNORE},
};

static bool is_module_blocked(wchar_t *dll, uint32_t timestamp)
{
	blocked_module_t *first_allowed = NULL;
	size_t len;

	len = wcslen(dll);

	wcslwr(dll);

	// Default behavior is to not block
	bool should_block = false;

	for (int i = 0; i < _countof(blocked_modules); i++) {
		blocked_module_t *b = &blocked_modules[i];
		wchar_t *dll_ptr;

		if (len >= b->name_len)
			dll_ptr = dll + len - b->name_len;
		else
			dll_ptr = dll;

		if (!wcscmp(dll_ptr, b->name)) {
			if (b->method == TS_IGNORE) {
				b->blocked_count++;
				return true;
			} else if (b->method == TS_EQUAL && timestamp == b->timestamp) {
				b->blocked_count++;
				return true;
			} else if (b->method == TS_LESS_THAN && timestamp < b->timestamp) {
				b->blocked_count++;
				return true;
			} else if (b->method == TS_GREATER_THAN && timestamp > b->timestamp) {
				b->blocked_count++;
				return true;
			} else if (b->method == TS_ALLOW_ONLY_THIS) {
				// Invert default behavior to block if
				// we don't find any matching timestamps
				// for this DLL.
				should_block = true;

				if (timestamp == b->timestamp)
					return false;

				// Bit of a hack to support counting of
				// TS_ALLOW_ONLY_THIS blocks as there may
				// be multiple entries for the same DLL.
				if (!first_allowed)
					first_allowed = b;
			}
		}
	}

	if (first_allowed)
		first_allowed->blocked_count++;

	return should_block;
}

static NTSTATUS NtMapViewOfSection_hook(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress,
					ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset,
					PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType,
					ULONG Win32Protect)
{
	SECTION_BASIC_INFORMATION section_information;
	wchar_t fileName[MAX_PATH];
	SIZE_T wrote = 0;
	NTSTATUS ret;
	uint32_t timestamp = 0;

	ret = ntMap(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize,
		    InheritDisposition, AllocationType, Win32Protect);

	// Verify map and process
	if (ret < 0 || ProcessHandle != GetCurrentProcess())
		return ret;

	// Fetch section information
	if (ntQuery(SectionHandle, SectionBasicInformation, &section_information, sizeof(section_information), &wrote) <
	    0)
		return ret;

	// Verify fetch was successful
	if (wrote != sizeof(section_information))
		return ret;

	// We're not interested in non-image maps
	if (!(section_information.Attributes & SEC_IMAGE))
		return ret;

	// Examine the PE header. Perhaps the map is small
	// so wrap it in an exception handler in case we
	// read past the end of the buffer.
	__try {
		BYTE *p = (BYTE *)*BaseAddress;
		IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)p;
		if (dos->e_magic != IMAGE_DOS_SIGNATURE)
			return ret;

		IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(p + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE)
			return ret;

		timestamp = nt->FileHeader.TimeDateStamp;

	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return ret;
	}

	// Get the actual filename if possible
	if (K32GetMappedFileNameW(ProcessHandle, *BaseAddress, fileName, _countof(fileName)) == 0)
		return ret;

	if (is_module_blocked(fileName, timestamp)) {
		ntUnmap(ProcessHandle, BaseAddress);
		ret = STATUS_UNSUCCESSFUL;
	}

	return ret;
}

void install_dll_blocklist_hook(void)
{
	HMODULE nt = GetModuleHandle(L"NTDLL");
	if (!nt)
		return;

	ntMap = (fn_NtMapViewOfSection)GetProcAddress(nt, "NtMapViewOfSection");
	if (!ntMap)
		return;

	ntUnmap = (fn_NtUnmapViewOfSection)GetProcAddress(nt, "NtUnmapViewOfSection");
	if (!ntUnmap)
		return;

	ntQuery = (fn_NtQuerySection)GetProcAddress(nt, "NtQuerySection");
	if (!ntQuery)
		return;

	// Pre-compute length of all DLL names for exact matching
	for (int i = 0; i < _countof(blocked_modules); i++) {
		blocked_module_t *b = &blocked_modules[i];
		b->name_len = wcslen(b->name);
	}

	DetourTransactionBegin();

	if (DetourAttach((PVOID *)&ntMap, NtMapViewOfSection_hook) != NO_ERROR)
		DetourTransactionAbort();
	else
		DetourTransactionCommit();
}

void log_blocked_dlls(void)
{
	for (int i = 0; i < _countof(blocked_modules); i++) {
		blocked_module_t *b = &blocked_modules[i];
		if (b->blocked_count) {
			blog(LOG_WARNING, "Blocked loading of '%S' %" PRIu64 " time%S.", b->name, b->blocked_count,
			     b->blocked_count == 1 ? L"" : L"s");
		}
	}
}
