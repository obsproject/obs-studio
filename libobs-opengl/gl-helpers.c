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

bool upload_face(GLenum type, uint32_t num_levels,
		GLenum format, GLint internal_format, bool compressed,
		uint32_t width, uint32_t height, uint32_t size, void ***p_data)
{
	bool success = true;
	void **data = *p_data;
	uint32_t i;

	for (i = 0; i < num_levels; i++) {
		if (compressed) {
			glCompressedTexImage2D(type, i, internal_format,
					width, height, 0, size, *data);
			if (!gl_success("glCompressedTexImage2D"))
				success = false;

		} else {
			glTexImage2D(type, i, internal_format, width, height, 0,
					format, GL_UNSIGNED_BYTE, *data);
			if (!gl_success("glTexImage2D"))
				success = false;
		}

		data++;
		size   /= 4;
		width  /= 2;
		height /= 2;
	}

	*p_data = data;
	return success;
}
