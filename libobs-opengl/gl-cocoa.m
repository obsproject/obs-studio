/******************************************************************************
    Copyright (C) 2013 by Ruwen Hahn <palana@stunned.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <GL/glew.h>
#include "gl-subsystem.h"
#include <OpenGL/OpenGL.h>

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>


//#include "util/darray.h"


struct gl_windowinfo {
	NSView *view;
};

struct gl_platform {
	NSOpenGLContext *context;
	struct gs_swap_chain swap;
};

static NSOpenGLContext *gl_context_create(struct gs_init_data *info)
{
	unsigned attrib_count = 0;

#define ADD_ATTR(x) \
	{ attributes[attrib_count++] = (NSOpenGLPixelFormatAttribute)x; }
#define ADD_ATTR2(x, y) { ADD_ATTR(x); ADD_ATTR(y); }

	NSOpenGLPixelFormatAttribute attributes[40];

	switch(info->num_backbuffers) {
		case 0:
			break;
		case 1:
			ADD_ATTR(NSOpenGLPFADoubleBuffer);
			break;
		case 2:
			ADD_ATTR(NSOpenGLPFATripleBuffer);
			break;
		default:
			blog(LOG_ERROR, "Requested backbuffers (%d) not "
			                "supported", info->num_backbuffers);
	}

	ADD_ATTR(NSOpenGLPFAClosestPolicy);
	ADD_ATTR2(NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core);

	int color_bits = 0;//get_color_format_bits(info->format);
	if(color_bits == 0) color_bits = 24;
	else if(color_bits < 15) color_bits = 15;

	ADD_ATTR2(NSOpenGLPFAColorSize, color_bits);

	ADD_ATTR2(NSOpenGLPFAAlphaSize, 8);

	ADD_ATTR2(NSOpenGLPFADepthSize, 16);

	ADD_ATTR(0);

#undef ADD_ATTR2
#undef ADD_ATTR

	NSOpenGLPixelFormat *pf;
	pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	if(!pf) {
		blog(LOG_ERROR, "Failed to create pixel format");
		return NULL;
	}

	NSOpenGLContext *context;
	context = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];
	[pf release];
	if(!context) {
		blog(LOG_ERROR, "Failed to create context");
		return NULL;
	}

	[context setView:info->view];

	return context;
}

static inline void required_extension_error(const char *extension)
{
	blog(LOG_ERROR, "OpenGL extension %s is required", extension);
}

static bool gl_init_extensions(device_t device)
{
	glewExperimental=true;
	GLenum error = glewInit();
	if(error != GLEW_OK) {
	       blog(LOG_ERROR, "glewInit failed, %u\n%s\n", error,
			       glewGetErrorString(error));
	       return false;
	}

	if(!GLEW_VERSION_2_1) {
	       blog(LOG_ERROR, "OpenGL 2.1 minimum required by the graphics "
	                       "adapter");
	       return false;
	}

	if(!GLEW_ARB_framebuffer_object) {
		required_extension_error("GL_ARB_framebuffer_object");
		return false;
	}

	if(!GLEW_ARB_separate_shader_objects) {
		required_extension_error("GL_ARB_separate_shader_objects");
		return false;
	}

	//something inside glew produces error code 1280 (invalid enum)
	glGetError();

	device->copy_type = COPY_TYPE_FBO_BLIT;

	return true;
}

static bool gl_init_default_swap(struct gl_platform *plat, device_t dev,
		struct gs_init_data *info)
{
	if(!(plat->context = gl_context_create(info)))
		return false;

	plat->swap.device = dev;
	plat->swap.info	  = *info;
	plat->swap.wi     = gl_windowinfo_create(info);

	return plat->swap.wi != NULL;
}

struct gl_platform *gl_platform_create(device_t device,
		struct gs_init_data *info)
{
	struct gl_platform *plat = bmalloc(sizeof(struct gl_platform));
	memset(plat, 0, sizeof(struct gl_platform));

	if(!gl_init_default_swap(plat, device, info))
		goto fail;

	[plat->context makeCurrentContext];

	if(!gl_init_extensions(device))
		goto fail;

	if (GLEW_ARB_seamless_cube_map) {
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		gl_success("GL_TEXTURE_CUBE_MAP_SEAMLESS");
	}

	return plat;

fail:
	blog(LOG_ERROR, "gl_platform_create failed");
	gl_platform_destroy(plat);
	return NULL;
}

struct gs_swap_chain *gl_platform_getswap(struct gl_platform *platform)
{
	if(platform)
		return &platform->swap;
	return NULL;
}

void gl_platform_destroy(struct gl_platform *platform)
{
	if(!platform)
		return;

	[platform->context release];
	platform->context = nil;
	gl_windowinfo_destroy(platform->swap.wi);

	bfree(platform);
}

struct gl_windowinfo *gl_windowinfo_create(struct gs_init_data *info)
{
	if(!info)
		return NULL;

	if(!info->view)
		return NULL;

	struct gl_windowinfo *wi = bmalloc(sizeof(struct gl_windowinfo));
	memset(wi, 0, sizeof(struct gl_windowinfo));

	wi->view = info->view;

	return wi;
}

void gl_windowinfo_destroy(struct gl_windowinfo *wi)
{
	if(!wi)
		return;

	wi->view = nil;
	bfree(wi);
}

void device_entercontext(device_t device)
{
	[device->plat->context makeCurrentContext];
}

void device_leavecontext(device_t device)
{
	[NSOpenGLContext clearCurrentContext];
}

void device_load_swapchain(device_t device, swapchain_t swap)
{
	if(!swap)
		swap = &device->plat->swap;

	if(device->cur_swap == swap)
		return;

	device->cur_swap = swap;
}

void device_present(device_t device)
{
	[device->plat->context flushBuffer];
}

void gl_getclientsize(struct gs_swap_chain *swap, uint32_t *width,
		uint32_t *height)
{
	if(width) *width = swap->info.cx;
	if(height) *height = swap->info.cy;
}
