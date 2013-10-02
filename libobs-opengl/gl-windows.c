#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "util/darray.h"
#include "gl-subsystem.h"
#include "glew/include/GL/wglew.h"

struct gl_platform {
	HDC   hdc;
	HGLRC hrc;
};

struct gl_window {
	HDC hdc;
};

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

static bool gl_create_false_context(struct gl_platform *plat,
		struct gs_init_data *info)
{
	PIXELFORMATDESCRIPTOR pfd;
	int format_index;

	plat->hdc = GetDC(info->hwnd);

	init_dummy_pixel_format(&pfd);

	format_index = ChoosePixelFormat(plat->hdc, &pfd);
	if (!format_index) {
		blog(LOG_ERROR, "ChoosePixelFormat failed, %u", GetLastError());
		return false;
	}

	if (!SetPixelFormat(plat->hdc, format_index, &pfd)) {
		blog(LOG_ERROR, "SetPixelFormat failed, %u", GetLastError());
		return false;
	}

	plat->hrc = wglCreateContext(plat->hdc);
	if (!plat->hrc) {
		blog(LOG_ERROR, "wglCreateContext failed, %u", GetLastError());
		return false;
	}

	if (!wglMakeCurrent(plat->hdc, plat->hrc)) {
		blog(LOG_ERROR, "wglMakeCurrent failed, %u", GetLastError());
		return false;
	}

	return true;
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

	if (!WGLEW_EXT_swap_control) {
		required_extension_error("WGL_EXT_swap_control");
		return false;
	}

	if (!WGLEW_ARB_pixel_format) {
		required_extension_error("WGL_ARB_pixel_format");
		return false;
	}

	return true;
}

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

static inline void add_attrib(struct darray *list, int attrib, int val)
{
	darray_push_back(sizeof(int), list, &attrib);
	darray_push_back(sizeof(int), list, &val);
}

static int gl_get_pixel_format(struct gl_platform *plat,
		struct gs_init_data *info)
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

	if (!wglChoosePixelFormatARB(plat->hdc, attribs.array, NULL, 1,
			&format, &num_formats)) {
		blog(LOG_ERROR, "wglChoosePixelFormatARB failed, %u",
				GetLastError());
		format = 0;
	}

	darray_free(&attribs);

	return format;
}

static inline bool gl_init_pixel_format(struct gl_platform *plat,
		struct gs_init_data *info)
{
	PIXELFORMATDESCRIPTOR pfd;
	int format = gl_get_pixel_format(plat, info);

	if (!format)
		return false;

	if (!DescribePixelFormat(plat->hdc, format, sizeof(pfd), &pfd)) {
		blog(LOG_ERROR, "DescribePixelFormat failed, %u",
				GetLastError());
		return false;
	}

	if (!SetPixelFormat(plat->hdc, format, &pfd)) {
		blog(LOG_ERROR, "SetPixelFormat failed, %u", GetLastError());
		return false;
	}

	return true;
}

bool gl_platform_init(struct gs_device *device, struct gs_init_data *info)
{
	device->plat = bmalloc(sizeof(struct gl_platform));
	memset(device->plat, 0, sizeof(struct gl_platform));

	if (!gl_create_false_context(device->plat, info))
		goto fail;

	if (!gl_init_extensions())
		goto fail;

	if (!gl_init_pixel_format(device->plat, info))
		goto fail;

	return true;

fail:
	return false;
}
