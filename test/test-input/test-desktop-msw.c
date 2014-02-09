#include <stdlib.h>
#include <Windows.h>
#include "test-desktop-msw.h"

const char *msw_desktop_test_getname(const char *locale)
{
	return "MSW Monitor Capture Source (Test)";
}

struct msw_desktop_tex *msw_desktop_test_create(const char *settings, obs_source_t source)
{
	struct msw_desktop_tex *rt = bmalloc(sizeof(struct msw_desktop_tex));

	memset(rt, 0, sizeof(struct msw_desktop_tex));

	rt->width = GetSystemMetrics(SM_CXSCREEN);
	rt->height = GetSystemMetrics(SM_CYSCREEN);

	gs_entercontext(obs_graphics());

	
	rt->hdcCompatible = CreateCompatibleDC(NULL);

	BITMAPINFO bi;
	memset(&bi, 0, sizeof(bi));

	BITMAPINFOHEADER *bih = &bi.bmiHeader;
	bih->biSize = sizeof(bi.bmiHeader);
	bih->biBitCount = 32;
	bih->biWidth = rt->width;
	bih->biHeight = rt->height;
	bih->biPlanes = 1;

	
	rt->hbmpCompatible = CreateDIBSection(rt->hdcCompatible, &bi, DIB_RGB_COLORS, (void**)&rt->captureBits, NULL, 0);
	rt->hbmpOld = (HBITMAP)SelectObject(rt->hdcCompatible,rt->hbmpCompatible);

	HDC sdc = GetDC(NULL);
	BitBlt(rt->hdcCompatible, 0, 0, rt->width, rt->height, sdc, 0, 0, SRCCOPY);
	ReleaseDC(NULL, sdc);

	rt->texture = gs_create_texture(rt->width, rt->height, GS_BGRA, 1,
		(const void **)&rt->captureBits, GS_DYNAMIC);


	if (!rt->texture) {
		msw_desktop_test_destroy(rt);
		return NULL;
	}

	gs_leavecontext();

	return rt;
}

void msw_desktop_test_destroy(struct msw_desktop_tex *rt)
{
	if (rt) {
		gs_entercontext(obs_graphics());
		
		SelectObject(rt->hdcCompatible, rt->hbmpOld);
		DeleteObject(rt->hdcCompatible);
		DeleteObject(rt->hbmpCompatible);
		
		texture_destroy(rt->texture);
		bfree(rt);

		gs_leavecontext();
	}

}

uint32_t msw_desktop_test_get_output_flags(struct msw_desktop_tex *rt)
{
	return SOURCE_VIDEO | SOURCE_DEFAULT_EFFECT;
}

void msw_desktop_test_video_tick(struct msw_desktop_tex *rt, float seconds)
{
	gs_entercontext(obs_graphics());

	HDC sdc = GetDC(0);
	BitBlt(rt->hdcCompatible, 0, 0, rt->width, rt->height, sdc, 0, 0, SRCCOPY);
	ReleaseDC(NULL,sdc);

	texture_setimage(rt->texture, rt->captureBits, rt->width * 4, false);
	
	gs_leavecontext();
}

void msw_desktop_test_video_render(struct msw_desktop_tex *rt, obs_source_t filter_target)
{
	effect_t effect  = gs_geteffect();
	eparam_t diffuse = effect_getparambyname(effect, "diffuse");

	effect_settexture(effect, diffuse, rt->texture);
	gs_draw_sprite(rt->texture, GS_FLIP_V, 0, 0);
}

uint32_t msw_desktop_test_getwidth(struct msw_desktop_tex *rt)
{
	return texture_getwidth(rt->texture);
}

uint32_t msw_desktop_test_getheight(struct msw_desktop_tex *rt)
{
	return texture_getheight(rt->texture);
}
