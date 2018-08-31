#include "dc-capture.h"

#define WIN32_MEAN_AND_LEAN
#include <windows.h>

static inline void init_textures(struct dc_capture *capture)
{
	if (capture->compatibility)
		capture->texture = gs_texture_create(
				capture->width, capture->height,
				GS_BGRA, 1, NULL, GS_DYNAMIC);
	else
		capture->texture = gs_texture_create_gdi(
				capture->width, capture->height);

	if (!capture->texture) {
		blog(LOG_WARNING, "[dc_capture_init] Failed to "
				  "create textures");
		return;
	}

	capture->valid = true;
}

void dc_capture_init(struct dc_capture *capture, int x, int y,
		uint32_t width, uint32_t height, bool cursor,
		bool compatibility, bool anicursor)
{
	memset(capture, 0, sizeof(struct dc_capture));

	capture->x              = x;
	capture->y              = y;
	capture->width          = width;
	capture->height         = height;
	capture->capture_cursor = cursor;
	capture->anicursor      = anicursor;

	obs_enter_graphics();

	if (!gs_gdi_texture_available())
		compatibility = true;

	capture->compatibility = compatibility;

	init_textures(capture);

	obs_leave_graphics();

	if (!capture->valid)
		return;

	if (compatibility) {
		BITMAPINFO bi = {0};
		BITMAPINFOHEADER *bih = &bi.bmiHeader;
		bih->biSize     = sizeof(BITMAPINFOHEADER);
		bih->biBitCount = 32;
		bih->biWidth    = width;
		bih->biHeight   = height;
		bih->biPlanes   = 1;

		capture->hdc = CreateCompatibleDC(NULL);
		capture->bmp = CreateDIBSection(capture->hdc, &bi,
				DIB_RGB_COLORS, (void**)&capture->bits,
				NULL, 0);
		capture->old_bmp = SelectObject(capture->hdc, capture->bmp);
	}
}

void dc_capture_free(struct dc_capture *capture)
{
	if (capture->hdc) {
		SelectObject(capture->hdc, capture->old_bmp);
		DeleteDC(capture->hdc);
		DeleteObject(capture->bmp);
	}

	obs_enter_graphics();
	gs_texture_destroy(capture->texture);
	obs_leave_graphics();

	memset(capture, 0, sizeof(struct dc_capture));
}

static int32_t getAniCurNextStep(HICON *icon, int32_t istep) {
	static HICON hLastCur = NULL;
	static float rFrameDur = 0;  //rendered frame interval in seconds
	float aniFrameDur;           //animation frame interval in seconds
	int32_t dur = 1;             //duration of the frame in jiffies,
	                             // 1 jif = 1/60 s
	uint32_t num_steps = 1;      //number of blits to render whole .ani
	static uint64_t renderDurMs = 0; //render call interval in ms
	static uint64_t last_ticks = 0;  //previous call time in ms

	//try to detect cursor change and reset animation
	if (*icon != hLastCur) {
		blog(LOG_DEBUG, "Cursor was changed");
		istep = 0;
		renderDurMs = 0; //reset rendered interval
		last_ticks = 0;
	}

	hLastCur = *icon;

	//get duration of the animated frame
	if (GetCFI((HCURSOR)*icon, 0L, istep, &dur, &num_steps)) {
		if (num_steps < 2) { //no animation
			num_steps = 1;
			rFrameDur = 0; //prepare to the next cursor
		}

		if (dur < 1) dur = 1; //keep frame duration in bounds

	} else
		return 0; //animation not supported

	if (last_ticks == 0)
		last_ticks = GetTickCount64() - 10; //initial conditions

	//not precise timer used because min .ani frame duraton is 1/60 s
	renderDurMs = GetTickCount64() - last_ticks;
	last_ticks += renderDurMs;

	rFrameDur += (float)renderDurMs / 1000;
	aniFrameDur = (float)dur / 60;

	//make sure that next animation frame is needs to be rendered
	if (aniFrameDur <= rFrameDur) {
		istep += (int32_t)(rFrameDur * 60 / dur);
		rFrameDur = rFrameDur - aniFrameDur; //remainder

		//rendered fps < .ani fps, do not accumulate remainder
		if (rFrameDur > aniFrameDur)
			rFrameDur = 0;
	}

	istep = istep % num_steps; //recalculate in case of the loop overflow

	return istep;
}

static void draw_cursor(struct dc_capture *capture, HDC hdc, HWND window)
{
	HICON      icon;
	ICONINFO   ii;
	CURSORINFO *ci = &capture->ci;
	POINT      win_pos = {capture->x, capture->y};

	if (!(capture->ci.flags & CURSOR_SHOWING))
		return;

	icon = (HICON)capture->ci.hCursor;
	if (!icon)
		return;

	if (GetIconInfo(icon, &ii)) {
		POINT pos;

		if (window)
			ClientToScreen(window, &win_pos);

		pos.x = ci->ptScreenPos.x - (int)ii.xHotspot - win_pos.x;
		pos.y = ci->ptScreenPos.y - (int)ii.yHotspot - win_pos.y;

		static int32_t istep = 0; //animated frame(step) number

		if (capture->anicursor)
			istep = getAniCurNextStep(&icon, istep);

		DrawIconEx(hdc, pos.x, pos.y, icon, 0, 0, istep, NULL,
				DI_NORMAL);

		DeleteObject(ii.hbmColor);
		DeleteObject(ii.hbmMask);
	}
}

static inline HDC dc_capture_get_dc(struct dc_capture *capture)
{
	if (!capture->valid)
		return NULL;

	if (capture->compatibility)
		return capture->hdc;
	else
		return gs_texture_get_dc(capture->texture);
}

static inline void dc_capture_release_dc(struct dc_capture *capture)
{
	if (capture->compatibility) {
		gs_texture_set_image(capture->texture,
				capture->bits, capture->width*4, false);
	} else {
		gs_texture_release_dc(capture->texture);
	}
}

void dc_capture_capture(struct dc_capture *capture, HWND window)
{
	HDC hdc_target;
	HDC hdc;

	if (capture->capture_cursor) {
		memset(&capture->ci, 0, sizeof(CURSORINFO));
		capture->ci.cbSize = sizeof(CURSORINFO);
		capture->cursor_captured = GetCursorInfo(&capture->ci);
	}

	hdc = dc_capture_get_dc(capture);
	if (!hdc) {
		blog(LOG_WARNING, "[capture_screen] Failed to get "
		                  "texture DC");
		return;
	}

	hdc_target = GetDC(window);

	BitBlt(hdc, 0, 0, capture->width, capture->height,
			hdc_target, capture->x, capture->y, SRCCOPY);

	ReleaseDC(NULL, hdc_target);

	if (capture->cursor_captured && !capture->cursor_hidden)
		draw_cursor(capture, hdc, window);

	dc_capture_release_dc(capture);

	capture->texture_written = true;
}

static void draw_texture(struct dc_capture *capture, gs_effect_t *effect)
{
	gs_texture_t   *texture = capture->texture;
	gs_technique_t *tech    = gs_effect_get_technique(effect, "Draw");
	gs_eparam_t    *image   = gs_effect_get_param_by_name(effect, "image");
	size_t      passes;

	gs_effect_set_texture(image, texture);

	passes = gs_technique_begin(tech);
	for (size_t i = 0; i < passes; i++) {
		if (gs_technique_begin_pass(tech, i)) {
			if (capture->compatibility)
				gs_draw_sprite(texture, GS_FLIP_V, 0, 0);
			else
				gs_draw_sprite(texture, 0, 0, 0);

			gs_technique_end_pass(tech);
		}
	}
	gs_technique_end(tech);
}

void dc_capture_render(struct dc_capture *capture, gs_effect_t *effect)
{
	if (capture->valid && capture->texture_written)
		draw_texture(capture, effect);
}
