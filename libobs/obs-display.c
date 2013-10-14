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

#include "obs.h"
#include "obs-data.h"

obs_display_t obs_display_create(struct gs_init_data *graphics_data)
{
	struct obs_display *display = bmalloc(sizeof(struct obs_display));
	memset(display, 0, sizeof(struct obs_display));

	if (graphics_data) {
		display->swap = gs_create_swapchain(graphics_data);
		if (!display->swap) {
			obs_display_destroy(display);
			return NULL;
		}
	}

	return display;
}

void obs_display_destroy(obs_display_t display)
{
	if (display) {
		swapchain_destroy(display->swap);
		bfree(display);
	}
}

obs_source_t obs_display_getsource(obs_display_t display)
{
	return display->source;
}

void obs_display_setsource(obs_display_t display, obs_source_t source)
{
	display->source = source;
}
