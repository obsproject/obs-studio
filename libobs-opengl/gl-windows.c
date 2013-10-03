#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "util/darray.h"
#include "gl-subsystem.h"
#include "glew/include/GL/wglew.h"

struct gl_windowinfo {
	HWND hwnd;
	HDC  hdc;
};

struct gl_platform {
	HGLRC hrc;
	struct gl_windowinfo *wi;
};

static inline int get_color_format_bits(enum gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
		return 32;
	default:
		return 0;
	}
}

static inline int get_depth_format_bits(enum gs_zstencil_format zsformat)
{
	switch (zsformat) {
	case GS_Z16:
		return 16;
	case GS_Z24_S8:
		return 24;
	default:
		return 0;
	}
}

static inline int get_stencil_format_bits(enum gs_zstencil_format zsformat)
{
	switch (zsformat) {
	case GS_Z24_S8:
		return 8;
	default:
		return 0;
	}
}

/* would use designated initializers but microsoft sort of sucks */
static inline void init_dummy_pixel_format(PIXELFORMATDESCRIPTOR *pfd)
{
	memset(&pfd, 0, sizeof(pfd));
	pfd->nSize        = sizeof(pfd);
	pfd->nVersion     = 1;
	pfd->iPixelType   = PFD_TYPE_RGBA;
	pfd->cColorBits   = 32;
	pfd->cDepthBits   = 24;
	pfd->cStencilBits = 8;
	pfd->iLayerType   = PFD_MAIN_PLANE;
	pfd->dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                            PFD_DOUBLEBUFFER;
}

static const char *dummy_window_class = "GLDummyWindow";
static bool registered_dummy_window_class = false;

struct dummy_context {
	HWND  hwnd;
	HGLRC hrc;
	HDC   hdc;
};

static bool gl_register_dummy_window_class(void)
{
	WNDCLASSA wc;
	if (registered_dummy_window_class)
		return true;

	memset(&wc, 0, sizeof(wc));
	wc.style = CS_OWNDC;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = dummy_window_class;

	if (!RegisterClassA(&wc)) {
		blog(LOG_ERROR, "Could not create dummy window class");
		return false;
	}

	registered_dummy_window_class = true;
	return true;
}

static inline HWND gl_create_dummy_window(void)
{
	HWND hwnd = CreateWindowExA(0, dummy_window_class, "Dummy GL Window",
			WS_POPUP,
			0, 0, 2, 2,
			NULL, NULL, GetModuleHandle(NULL), NULL);
	if (!hwnd)
		blog(LOG_ERROR, "Could not create dummy context window");

	return hwnd;
}

static HGLRC gl_init_context(HDC hdc)
{
	HGLRC hglrc = wglCreateContext(hdc);
	if (!hglrc) {
		blog(LOG_ERROR, "wglCreateContext failed, %u", GetLastError());
		return NULL;
	}

	if (!wglMakeCurrent(hdc, hglrc)) {
		blog(LOG_ERROR, "wglMakeCurrent failed, %u", GetLastError());
		wglDeleteContext(hglrc);
		return NULL;
	}

	return hglrc;
}

static bool gl_dummy_context_init(struct dummy_context *dummy)
{
	PIXELFORMATDESCRIPTOR pfd;
	int format_index;

	if (!gl_register_dummy_window_class())
		return false;

	dummy->hwnd = gl_create_dummy_window();
	if (!dummy->hwnd)
		return false;

	dummy->hdc = GetDC(dummy->hwnd);

	init_dummy_pixel_format(&pfd);
	format_index = ChoosePixelFormat(dummy->hdc, &pfd);
	if (!format_index) {
		blog(LOG_ERROR, "Dummy ChoosePixelFormat failed, %u",
				GetLastError());
		return false;
	}

	if (!SetPixelFormat(dummy->hdc, format_index, &pfd)) {
		blog(LOG_ERROR, "Dummy SetPixelFormat failed, %u",
				GetLastError());
		return false;
	}

	dummy->hrc = gl_init_context(dummy->hdc);
	if (!dummy->hrc) {
		blog(LOG_ERROR, "Failed to initialize dummy context");
		return false;
	}

	return true;
}

static void gl_dummy_context_free(struct dummy_context *dummy)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(dummy->hrc);
	DestroyWindow(dummy->hwnd);
	memset(dummy, 0, sizeof(struct dummy_context));
}

static inline void required_extension_error(const char *extension)
{
	blog(LOG_ERROR, "OpenGL extension %s is required", extension);
}

static bool gl_init_extensions(void)
{
	GLenum errorcode = glewInit();
	if (errorcode != GLEW_OK) {
		blog(LOG_ERROR, "glewInit failed, %u", errorcode);
		return false;
	}

	if (!GLEW_VERSION_2_1) {
		blog(LOG_ERROR, "OpenGL 2.1 minimum required by the graphics "
		                "adapter");
		return false;
	}

	if (!GLEW_ARB_framebuffer_object) {
		required_extension_error("GL_ARB_framebuffer_object");
		return false;
	}

	if (!WGLEW_ARB_pixel_format) {
		required_extension_error("WGL_ARB_pixel_format");
		return false;
	}

	return true;
}

static inline void add_attrib(struct darray *list, int attrib, int val)
{
	darray_push_back(sizeof(int), list, &attrib);
	darray_push_back(sizeof(int), list, &val);
}

static int gl_choose_pixel_format(HDC hdc, struct gs_init_data *info)
{
	struct darray attribs;
	int color_bits   = get_color_format_bits(info->format);
	int depth_bits   = get_depth_format_bits(info->zsformat);
	int stencil_bits = get_stencil_format_bits(info->zsformat);
	UINT num_formats;
	int format;

	if (!color_bits) {
		blog(LOG_ERROR, "gl_init_pixel_format: color format not "
		                "supported");
		return false;
	}

	darray_init(&attribs);
	add_attrib(&attribs, WGL_DRAW_TO_WINDOW_ARB, GL_TRUE);
	add_attrib(&attribs, WGL_SUPPORT_OPENGL_ARB, GL_TRUE);
	add_attrib(&attribs, WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB);
	add_attrib(&attribs, WGL_DOUBLE_BUFFER_ARB,  GL_TRUE);
	add_attrib(&attribs, WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB);
	add_attrib(&attribs, WGL_COLOR_BITS_ARB,     color_bits);
	add_attrib(&attribs, WGL_DEPTH_BITS_ARB,     depth_bits);
	add_attrib(&attribs, WGL_STENCIL_BITS_ARB,   stencil_bits);
	add_attrib(&attribs, 0, 0);

	if (!wglChoosePixelFormatARB(hdc, attribs.array, NULL, 1, &format,
				&num_formats)) {
		blog(LOG_ERROR, "wglChoosePixelFormatARB failed, %u",
				GetLastError());
		format = 0;
	}

	darray_free(&attribs);

	return format;
}

static inline bool gl_get_pixel_format(HDC hdc, struct gs_init_data *info,
		int *format, PIXELFORMATDESCRIPTOR *pfd)
{
	*format = gl_choose_pixel_format(hdc, info);

	if (!format)
		return false;

	if (!DescribePixelFormat(hdc, *format, sizeof(*pfd), pfd)) {
		blog(LOG_ERROR, "DescribePixelFormat failed, %u",
				GetLastError());
		return false;
	}

	return true;
}

static inline bool gl_set_pixel_format(HDC hdc, int format,
		PIXELFORMATDESCRIPTOR *pfd)
{
	if (!SetPixelFormat(hdc, format, pfd)) {
		blog(LOG_ERROR, "SetPixelFormat failed, %u", GetLastError());
		return false;
	}

	return true;
}

struct gl_platform *gl_platform_create(struct gs_init_data *info)
{
	struct gl_platform *plat = bmalloc(sizeof(struct gl_platform));
	struct dummy_context dummy;
	int pixel_format;
	PIXELFORMATDESCRIPTOR pfd;

	memset(plat, 0, sizeof(struct gl_platform));
	memset(&dummy, 0, sizeof(struct dummy_context));

	if (!gl_dummy_context_init(&dummy))
		goto fail;

	if (!gl_init_extensions())
		goto fail;

	/* you have to have a dummy context open before you can actually
	 * use wglChoosePixelFormatARB */
	if (!gl_get_pixel_format(dummy.hdc, info, &pixel_format, &pfd))
		goto fail;

	gl_dummy_context_free(&dummy);

	plat->wi = gl_windowinfo_create(info);
	if (!plat->wi)
		goto fail;

	if (!gl_set_pixel_format(plat->wi->hdc, pixel_format, &pfd))
		goto fail;

	plat->hrc = gl_init_context(plat->wi->hdc);
	if (!plat->hrc)
		goto fail;

	return plat;

fail:
	gl_platform_destroy(plat);
	gl_dummy_context_free(&dummy);
	return NULL;
}

void gl_platform_destroy(struct gl_platform *plat)
{
	if (plat) {
		if (plat->hrc) {
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(plat->hrc);
		}

		gl_windowinfo_destroy(plat->wi);
		bfree(plat);
	}
}

struct gl_windowinfo *gl_windowinfo_create(struct gs_init_data *info)
{
	struct gl_windowinfo *wi = bmalloc(sizeof(struct gl_windowinfo));
	memset(wi, 0, sizeof(struct gl_windowinfo));

	wi->hwnd = info->hwnd;
	wi->hdc  = GetDC(wi->hwnd);
	if (!wi->hdc) {
		blog(LOG_ERROR, "Unable to get device context from window");
		bfree(wi);
		return NULL;
	}

	return wi;
}

void gl_windowinfo_destroy(struct gl_windowinfo *wi)
{
	if (wi) {
		if (wi->hdc)
			ReleaseDC(wi->hwnd, wi->hdc);
		bfree(wi);
	}
}
