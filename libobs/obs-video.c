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
#include "graphics/vec4.h"

static void tick_sources(uint64_t cur_time, uint64_t *last_time)
{
	size_t i;
	uint64_t delta_time;
	float seconds;

	if (!last_time)
		*last_time = cur_time - video_getframetime(obs->video);
	delta_time = cur_time - *last_time;
	seconds = (float)((double)delta_time / 1000000000.0);

	for (i = 0; i < obs->sources.num; i++)
		obs_source_video_tick(obs->sources.array[i], seconds);

	*last_time = cur_time;
}

static inline void render_display(struct obs_display *display)
{
	struct vec4 clear_color;
	uint32_t width, height;

	gs_load_swapchain(display ? display->swap : NULL);

	gs_getsize(&width, &height);

	gs_beginscene();
	vec4_set(&clear_color, 0.3f, 0.0f, 0.0f, 1.0f);
	gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH | GS_CLEAR_STENCIL,
			&clear_color, 1.0f, 0);

	gs_enable_depthtest(false);
	gs_enable_blending(false);
	gs_setcullmode(GS_NEITHER);

	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_setviewport(0, 0, width, height);

	if (obs->primary_source)
		obs_source_video_render(obs->primary_source);

	gs_endscene();
	gs_present();
}

static inline void render_displays(void)
{
	size_t i;

	pthread_mutex_lock(&obs->display_list_mutex);

	for (i = 0; i < obs->displays.num; i++) {
		struct obs_display *display = obs->displays.array[i];

		render_display(display);
	}

	pthread_mutex_unlock(&obs->display_list_mutex);

	render_display(NULL);
}

static bool swap_frame(uint64_t timestamp)
{
	stagesurf_t last_surface = obs->copy_surfaces[obs->cur_texture];
	stagesurf_t surface;
	struct video_frame frame;

	if (obs->copy_mapped) {
		stagesurface_unmap(last_surface);
		obs->copy_mapped = false;
	}

	obs->textures_copied[obs->cur_texture] = true;
	//gs_stage_texture(last_surface, NULL);

	if (++obs->cur_texture == NUM_TEXTURES)
		obs->cur_texture = 0;

	if (obs->textures_copied[obs->cur_texture]) {
		surface = obs->copy_surfaces[obs->cur_texture];
		obs->copy_mapped = stagesurface_map(surface, &frame.data,
				&frame.row_size);

		if (obs->copy_mapped) {
			frame.timestamp = timestamp;
			video_output_frame(obs->video, &frame);
		}
	}

	return obs->copy_mapped;
}

void *obs_video_thread(void *param)
{
	uint64_t last_time = 0;

	while (video_output_wait(obs->video)) {
		uint64_t cur_time = video_gettime(obs->video);

		gs_entercontext(obs_graphics());

		tick_sources(cur_time, &last_time);
		render_displays();
		swap_frame(cur_time);

		gs_leavecontext();
	}

	return NULL;
}
