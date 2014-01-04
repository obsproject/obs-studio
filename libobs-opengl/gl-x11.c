#include <X11/Xlib.h>

#include <stdio.h>

#include "gl-subsystem.h"

#include <GL/glx.h>
#include <GL/glxext.h>
	
static PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
	
static const GLenum fb_attribs[] = {
	/* Hardcoded for now... */
	GLX_STENCIL_SIZE, 8,
	GLX_DEPTH_SIZE, 24, 
	GLX_BUFFER_SIZE, 32, /* Color buffer depth */
	GLX_DOUBLEBUFFER, True,
	None
};

static const GLenum ctx_attribs[] = {
#ifdef _DEBUG
	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
	GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
	None, 
};

static const char* __GLX_error_table[] = {
	"Success",
	"Bad Screen",
	"Bad Attribute",
	"No Extension",
	"Bad Visual",
	"Bad Content",
	"Bad Value",
	"Bad Enumeration"
};

#define GET_GLX_ERROR(x) \
	__GLX_error_table[x]

struct gl_windowinfo {
	uint32_t id;
	Display *display;
};
	
struct gl_platform {
	GLXContext context;
	struct gs_swap_chain swap;
};

static int GLXErrorHandler(Display *disp, XErrorEvent *error)
{
	blog(LOG_ERROR, "GLX error: %s\n", GET_GLX_ERROR(error->error_code));
	return 0;
}

extern struct gs_swap_chain *gl_platform_getswap(struct gl_platform *platform) 
{
	return &platform->swap;
}

extern struct gl_windowinfo *gl_windowinfo_create(struct gs_init_data *info) 
{
	struct gl_windowinfo *wi = bmalloc(sizeof(struct gl_windowinfo));
	memset(wi, 0, sizeof(struct gl_windowinfo));
	
	wi->id = info->window.id;
	
	return wi;
}

extern void gl_windowinfo_destroy(struct gl_windowinfo *wi) 
{
	bfree(wi);
}

extern void gl_getclientsize(struct gs_swap_chain *swap,
			     uint32_t *width, uint32_t *height) 
{
	XWindowAttributes info = { 0 };
	
	XGetWindowAttributes(swap->wi->display, swap->wi->id, &info);
	
	*height = info.height;
	*width = info.width;
}

static void print_info_stuff(struct gs_init_data *info)
{
	blog(	LOG_INFO,
		"X and Y: %i %i\n"
		"Backbuffers: %i\n"
		"Color Format: %i\n"
		"ZStencil Format: %i\n"
		"Adapter: %i\n",
		info->cx, info->cy, 
		info->num_backbuffers,
		info->format, info->zsformat, 
		info->adapter
	);
}

struct gl_platform *gl_platform_create(device_t device,
		struct gs_init_data *info)
{	
	/* X11 */
	int num_configs = 0;
	int error_base = 0, event_base = 0;
	Display *display = XOpenDisplay(NULL); /* Open default screen */
	
	/* OBS */
	struct gl_platform *plat = bmalloc(sizeof(struct gl_platform));
	
	/* GLX */
	GLXFBConfig* configs;
	
	print_info_stuff(info);
	
	if (!plat) { 
		blog(LOG_ERROR, "Out of memory");
		return NULL;
	}
	
	memset(plat, 0, sizeof(struct gl_platform));
	
	if (!display) {
		blog(LOG_ERROR, "Unable to find display. DISPLAY variable may not be set correctly.");
		goto fail0;
	}
	
	
	if (!glXQueryExtension(display, &error_base, &event_base)) {
		blog(LOG_ERROR, "GLX not supported.");
		goto fail0;
	}
	
	XSetErrorHandler(GLXErrorHandler);
	
	glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress("glXCreateContextAttribsARB");
	if (!glXCreateContextAttribsARB) {
		blog(LOG_ERROR, "ARB_GLX_create_context not supported!");
		goto fail0;
	}
	
	configs = glXChooseFBConfig(display, DefaultScreen(display), fb_attribs, &num_configs);

	if(!configs) {
		blog(LOG_ERROR, "Attribute list or screen is invalid.");
		goto fail0;
	}
	
	if(num_configs == 0) {
		blog(LOG_ERROR, "No framebuffer configurations found.");
		goto fail1;
	}
	
	/* We just use the first configuration found... as it does everything we want at the very least. */
	plat->context = glXCreateContextAttribsARB(display, configs[0], NULL, True, ctx_attribs);
	if(!plat->context) { 
		blog(LOG_ERROR, "Failed to create OpenGL context.");
		goto fail1;
	}
	
	if(!glXMakeCurrent(display, info->window.id, plat->context)) {
		blog(LOG_ERROR, "Failed to make context current.");
		goto fail2;
	}

	/* Initialize GLEW */
	{
		GLenum err = glewInit();
		glewExperimental = true;
		
		if (GLEW_OK != err) {
			blog(LOG_ERROR, glewGetErrorString(err));
			goto fail2;
		}
	}
	
	blog(LOG_INFO, "OpenGL version: %s\n", glGetString(GL_VERSION));
	
	plat->swap.device = device;
	plat->swap.info	  = *info;
	plat->swap.wi     = gl_windowinfo_create(info);
	plat->swap.wi->display = display;
	
	XFree(configs);
	XSync(display, False);
	
	return plat;
	
fail2:
	glXDestroyContext(display, plat->context);
fail1: 
	XFree(configs);
fail0:
	bfree(plat);
	return NULL;
}

extern void gl_platform_destroy(struct gl_platform *platform) 
{
	if (!platform)
		return; 
	
	glXMakeCurrent(platform->swap.wi->display, None, NULL);
	glXDestroyContext(platform->swap.wi->display, platform->context);
	XCloseDisplay(platform->swap.wi->display);
	gl_windowinfo_destroy(platform->swap.wi);
	bfree(platform);
}

void device_entercontext(device_t device) 
{
	GLXContext context = device->plat->context;
	XID window = device->plat->swap.wi->id;
	Display *display = device->plat->swap.wi->display;
	
	if (device->cur_swap)
		 device->plat->swap.wi->id = device->cur_swap->wi->id;
	
	if (!glXMakeCurrent(display, window, context)) {
		blog(LOG_ERROR, "Failed to make context current.");
	}
}
void device_leavecontext(device_t device) 
{
	Display *display = device->plat->swap.wi->display;
	
	if(!glXMakeCurrent(display, None, NULL)) { 
		blog(LOG_ERROR, "Failed to reset current context.");
	}
}

void gl_update(device_t device)
{
	/* I don't know of anything we can do here. */
}

void device_load_swapchain(device_t device, swapchain_t swap) 
{
	if(!swap)
		swap = &device->plat->swap;
	
	device->cur_swap = swap;
}

void device_present(device_t device) 
{
	Display *display = device->plat->swap.wi->display;
	XID window = device->plat->swap.wi->id;
	
	glXSwapBuffers(display, window);
}
