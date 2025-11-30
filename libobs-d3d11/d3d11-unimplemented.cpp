/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include <graphics/graphics.h>
#include <graphics/device-exports.h>

void device_enter_context(gs_device_t *device)
{
	/* does nothing */
	UNUSED_PARAMETER(device);
}

void device_leave_context(gs_device_t *device)
{
	/* does nothing */
	UNUSED_PARAMETER(device);
}

void device_load_default_samplerstate(gs_device_t *device, bool b_3d, int unit)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(b_3d);
	UNUSED_PARAMETER(unit);
}

void device_end_scene(gs_device_t *device)
{
	/* does nothing in D3D11 */
	UNUSED_PARAMETER(device);
}

uint32_t gs_voltexture_get_width(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t gs_voltexture_get_height(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t gs_voltexture_get_depth(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

enum gs_color_format gs_voltexture_get_color_format(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return GS_UNKNOWN;
}

extern "C" EXPORT bool device_gdi_texture_available(void)
{
	return true;
}

extern "C" EXPORT bool device_shared_texture_available(void)
{
	return true;
}
