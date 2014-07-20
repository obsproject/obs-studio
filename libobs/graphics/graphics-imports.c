/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "../util/base.h"
#include "../util/dstr.h"
#include "../util/platform.h"
#include "graphics-internal.h"

#define GRAPHICS_IMPORT(func) \
	do { \
		exports->func = os_dlsym(module, #func); \
		if (!exports->func) { \
			success = false; \
			blog(LOG_ERROR, "Could not load function '%s' from " \
			                "module '%s'", #func, module_name); \
		} \
	} while (false)

#define GRAPHICS_IMPORT_OPTIONAL(func) \
	do { \
		exports->func = os_dlsym(module, #func); \
	} while (false)

bool load_graphics_imports(struct gs_exports *exports, void *module,
		const char *module_name)
{
	bool success = true;

	GRAPHICS_IMPORT(device_name);
	GRAPHICS_IMPORT(device_preprocessor_name);
	GRAPHICS_IMPORT(device_create);
	GRAPHICS_IMPORT(device_destroy);
	GRAPHICS_IMPORT(device_entercontext);
	GRAPHICS_IMPORT(device_leavecontext);
	GRAPHICS_IMPORT(device_create_swapchain);
	GRAPHICS_IMPORT(device_resize);
	GRAPHICS_IMPORT(device_getsize);
	GRAPHICS_IMPORT(device_getwidth);
	GRAPHICS_IMPORT(device_getheight);
	GRAPHICS_IMPORT(device_create_texture);
	GRAPHICS_IMPORT(device_create_cubetexture);
	GRAPHICS_IMPORT(device_create_volumetexture);
	GRAPHICS_IMPORT(device_create_zstencil);
	GRAPHICS_IMPORT(device_create_stagesurface);
	GRAPHICS_IMPORT(device_create_samplerstate);
	GRAPHICS_IMPORT(device_create_vertexshader);
	GRAPHICS_IMPORT(device_create_pixelshader);
	GRAPHICS_IMPORT(device_create_vertexbuffer);
	GRAPHICS_IMPORT(device_create_indexbuffer);
	GRAPHICS_IMPORT(device_gettexturetype);
	GRAPHICS_IMPORT(device_load_vertexbuffer);
	GRAPHICS_IMPORT(device_load_indexbuffer);
	GRAPHICS_IMPORT(device_load_texture);
	GRAPHICS_IMPORT(device_load_samplerstate);
	GRAPHICS_IMPORT(device_load_vertexshader);
	GRAPHICS_IMPORT(device_load_pixelshader);
	GRAPHICS_IMPORT(device_load_defaultsamplerstate);
	GRAPHICS_IMPORT(device_getvertexshader);
	GRAPHICS_IMPORT(device_getpixelshader);
	GRAPHICS_IMPORT(device_getrendertarget);
	GRAPHICS_IMPORT(device_getzstenciltarget);
	GRAPHICS_IMPORT(device_setrendertarget);
	GRAPHICS_IMPORT(device_setcuberendertarget);
	GRAPHICS_IMPORT(device_copy_texture_region);
	GRAPHICS_IMPORT(device_copy_texture);
	GRAPHICS_IMPORT(device_stage_texture);
	GRAPHICS_IMPORT(device_beginscene);
	GRAPHICS_IMPORT(device_draw);
	GRAPHICS_IMPORT(device_load_swapchain);
	GRAPHICS_IMPORT(device_endscene);
	GRAPHICS_IMPORT(device_clear);
	GRAPHICS_IMPORT(device_present);
	GRAPHICS_IMPORT(device_flush);
	GRAPHICS_IMPORT(device_setcullmode);
	GRAPHICS_IMPORT(device_getcullmode);
	GRAPHICS_IMPORT(device_enable_blending);
	GRAPHICS_IMPORT(device_enable_depthtest);
	GRAPHICS_IMPORT(device_enable_stenciltest);
	GRAPHICS_IMPORT(device_enable_stencilwrite);
	GRAPHICS_IMPORT(device_enable_color);
	GRAPHICS_IMPORT(device_blendfunction);
	GRAPHICS_IMPORT(device_depthfunction);
	GRAPHICS_IMPORT(device_stencilfunction);
	GRAPHICS_IMPORT(device_stencilop);
	GRAPHICS_IMPORT(device_setviewport);
	GRAPHICS_IMPORT(device_getviewport);
	GRAPHICS_IMPORT(device_setscissorrect);
	GRAPHICS_IMPORT(device_ortho);
	GRAPHICS_IMPORT(device_frustum);
	GRAPHICS_IMPORT(device_projection_push);
	GRAPHICS_IMPORT(device_projection_pop);

	GRAPHICS_IMPORT(swapchain_destroy);

	GRAPHICS_IMPORT(texture_destroy);
	GRAPHICS_IMPORT(texture_getwidth);
	GRAPHICS_IMPORT(texture_getheight);
	GRAPHICS_IMPORT(texture_getcolorformat);
	GRAPHICS_IMPORT(texture_map);
	GRAPHICS_IMPORT(texture_unmap);
	GRAPHICS_IMPORT_OPTIONAL(texture_isrect);
	GRAPHICS_IMPORT(texture_getobj);

	GRAPHICS_IMPORT(cubetexture_destroy);
	GRAPHICS_IMPORT(cubetexture_getsize);
	GRAPHICS_IMPORT(cubetexture_getcolorformat);

	GRAPHICS_IMPORT(volumetexture_destroy);
	GRAPHICS_IMPORT(volumetexture_getwidth);
	GRAPHICS_IMPORT(volumetexture_getheight);
	GRAPHICS_IMPORT(volumetexture_getdepth);
	GRAPHICS_IMPORT(volumetexture_getcolorformat);

	GRAPHICS_IMPORT(stagesurface_destroy);
	GRAPHICS_IMPORT(stagesurface_getwidth);
	GRAPHICS_IMPORT(stagesurface_getheight);
	GRAPHICS_IMPORT(stagesurface_getcolorformat);
	GRAPHICS_IMPORT(stagesurface_map);
	GRAPHICS_IMPORT(stagesurface_unmap);

	GRAPHICS_IMPORT(zstencil_destroy);

	GRAPHICS_IMPORT(samplerstate_destroy);

	GRAPHICS_IMPORT(vertexbuffer_destroy);
	GRAPHICS_IMPORT(vertexbuffer_flush);
	GRAPHICS_IMPORT(vertexbuffer_getdata);

	GRAPHICS_IMPORT(indexbuffer_destroy);
	GRAPHICS_IMPORT(indexbuffer_flush);
	GRAPHICS_IMPORT(indexbuffer_getdata);
	GRAPHICS_IMPORT(indexbuffer_numindices);
	GRAPHICS_IMPORT(indexbuffer_gettype);

	GRAPHICS_IMPORT(shader_destroy);
	GRAPHICS_IMPORT(shader_numparams);
	GRAPHICS_IMPORT(shader_getparambyidx);
	GRAPHICS_IMPORT(shader_getparambyname);
	GRAPHICS_IMPORT(shader_getviewprojmatrix);
	GRAPHICS_IMPORT(shader_getworldmatrix);
	GRAPHICS_IMPORT(shader_getparaminfo);
	GRAPHICS_IMPORT(shader_setbool);
	GRAPHICS_IMPORT(shader_setfloat);
	GRAPHICS_IMPORT(shader_setint);
	GRAPHICS_IMPORT(shader_setmatrix3);
	GRAPHICS_IMPORT(shader_setmatrix4);
	GRAPHICS_IMPORT(shader_setvec2);
	GRAPHICS_IMPORT(shader_setvec3);
	GRAPHICS_IMPORT(shader_setvec4);
	GRAPHICS_IMPORT(shader_settexture);
	GRAPHICS_IMPORT(shader_setval);
	GRAPHICS_IMPORT(shader_setdefault);

	/* OSX/Cocoa specific functions */
#ifdef __APPLE__
	GRAPHICS_IMPORT_OPTIONAL(texture_create_from_iosurface);
	GRAPHICS_IMPORT_OPTIONAL(texture_rebind_iosurface);

	/* win32 specific functions */
#elif _WIN32
	GRAPHICS_IMPORT(gdi_texture_available);
	GRAPHICS_IMPORT_OPTIONAL(device_create_gdi_texture);
	GRAPHICS_IMPORT_OPTIONAL(texture_get_dc);
	GRAPHICS_IMPORT_OPTIONAL(texture_release_dc);
#endif

	return success;
}
