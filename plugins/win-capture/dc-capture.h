#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <obs-module.h>

struct dc_capture {
	gs_texture_t *texture;
	bool         texture_written;
	int          x, y;
	uint32_t     width;
	uint32_t     height;

	bool         compatibility;
	HDC          hdc;
	HBITMAP      bmp, old_bmp;
	BYTE         *bits;

	bool         capture_cursor;
	bool         cursor_captured;
	bool         cursor_hidden;
	CURSORINFO   ci;

	bool         valid;
	
	bool         anicursor;
};

extern void dc_capture_init(struct dc_capture *capture, int x, int y,
		uint32_t width, uint32_t height, bool cursor,
		bool compatibility, bool anicursor);
extern void dc_capture_free(struct dc_capture *capture);

extern void dc_capture_capture(struct dc_capture *capture, HWND window);
extern void dc_capture_render(struct dc_capture *capture, gs_effect_t *effect);

typedef HCURSOR (WINAPI *GETANICUR)(HCURSOR hCursor, DWORD reserved,
		DWORD istep, PINT rate_jiffies, DWORD *num_steps);

//
// Get undocumented WINAPI function GetCursorFrameInfo()
//
#define GET_WINAPI_CURSOR_FUNC(func_name, library_name) \
static HCURSOR func_name(HCURSOR hCursor, DWORD rsrv, DWORD istep, \
PINT dur, DWORD *num_steps) \
{ \
	HMODULE phModule = NULL; \
	static GETANICUR FUNC = NULL; \
	static HCURSOR hRet = NULL; \
	static bool initialized = false; \
\
	if (!initialized) { \
		if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_PIN, \
				TEXT(#library_name), &phModule)) \
			FUNC = (GETANICUR)GetProcAddress(phModule, \
					"GetCursorFrameInfo"); \
\
		initialized = true; \
	} \
\
	if (FUNC) \
		hRet = FUNC(hCursor, rsrv, istep, dur, num_steps); \
\
	return hRet; \
}

//
// call to GetCursorFrameInfo() function
//
GET_WINAPI_CURSOR_FUNC(GetCFI, user32)
