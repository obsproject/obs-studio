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

vertbuffer_t device_create_vertexbuffer(device_t device,
		struct vb_data *data, uint32_t flags)
{
}

void vertexbuffer_destroy(vertbuffer_t vertbuffer)
{
}

void vertexbuffer_flush(vertbuffer_t vertbuffer, bool rebuild)
{
}

struct vb_data *vertexbuffer_getdata(vertbuffer_t vertbuffer)
{
}
