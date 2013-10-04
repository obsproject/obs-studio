/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "gl-subsystem.h"

static inline bool upload_texture_data(struct gs_texture_2d *tex, void **data)
{
	if (!gl_bind_texture(GL_TEXTURE_2D, tex->base.texture))
		return false;

	//if (is_compressed_format(tex->base.format))

	if (!gl_bind_texture(GL_TEXTURE_2D, 0))
		return false;

	return true;
}

texture_t device_create_texture(device_t device, uint32_t width,
		uint32_t height, enum gs_color_format color_format,
		uint32_t levels, void **data, uint32_t flags)
{
	struct gs_texture_2d *tex = bmalloc(sizeof(struct gs_texture_2d));
	memset(tex, 0, sizeof(struct gs_texture_2d));

	tex->base.device             = device;
	tex->base.type               = GS_TEXTURE_2D;
	tex->base.format             = color_format;
	tex->base.gl_format          = convert_gs_format(color_format);
	tex->base.gl_internal_format = convert_gs_internal_format(color_format);
	tex->width                   = width;
	tex->height                  = height;

	if (!gl_gen_textures(1, &tex->base.texture))
		goto fail;
	if (data && !upload_texture_data(tex, data))
		goto fail;

	return (texture_t)tex;

fail:
	texture_destroy((texture_t)tex);
	blog(LOG_ERROR, "device_create_texture (GL) failed");
	return NULL;
}

void texture_destroy(texture_t tex)
{
}
