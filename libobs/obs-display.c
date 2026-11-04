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

#include "graphics/vec4.h"
#include "obs.h"
#include "obs-internal.h"

#include <inttypes.h>

static void reset_display_stats(struct obs_display *display)
{
	display->presentation_stats = (struct gs_swapchain_present_stats){.generation = os_gettime_ns()};
	display->draw_attempts = 0;
	display->not_ready = 0;
	display->last_present_time = 0;
}

bool obs_display_get_stats(obs_display_t *display, struct obs_display_stats *stats)
{
	if (!stats)
		return false;
	*stats = (struct obs_display_stats){0};
	if (!display)
		return false;
	obs_enter_graphics();
	stats->enabled = display->enabled;
	stats->draw_attempts = display->draw_attempts;
	stats->not_ready = display->not_ready;
	stats->presentation = display->presentation_stats;
	stats->presentation.sample_time = os_gettime_ns();
#ifdef _WIN32
	stats->buffered = display->present_thread != NULL;
	if (stats->buffered && !gs_swapchain_get_present_stats(display->swap, &stats->presentation, false)) {
		obs_leave_graphics();
		return false;
	}
#endif
	obs_leave_graphics();
	return true;
}

void obs_display_reset_stats(obs_display_t *display)
{
	if (!display)
		return;
	obs_enter_graphics();
	reset_display_stats(display);
#ifdef _WIN32
	if (display->present_thread) {
		struct gs_swapchain_present_stats stats;
		gs_swapchain_get_present_stats(display->swap, &stats, true);
	}
#endif
	obs_leave_graphics();
}

#ifdef _WIN32
#include <windows.h>

struct obs_display_present_thread {
	pthread_t thread;
	HANDLE stop;
	struct obs_display *display;
	bool has_credit;
};

static void *display_present_thread(void *param)
{
	struct obs_display_present_thread *state = param;
	struct obs_display *display = state->display;
	os_set_thread_name("obs-preview-present");
	while (WaitForSingleObject(state->stop, 0) == WAIT_TIMEOUT) {
		obs_enter_graphics();
		const bool active = display->enabled && display->cx && display->cy;
		HANDLE wait_handle = active && !state->has_credit ? gs_swapchain_get_present_wait_handle(display->swap)
								  : NULL;
		obs_leave_graphics();
		if (!active || (!state->has_credit && !wait_handle)) {
			WaitForSingleObject(state->stop, 50);
			continue;
		}
		if (!state->has_credit) {
			const HANDLE handles[] = {state->stop, wait_handle};
			const DWORD result = WaitForMultipleObjects(2, handles, FALSE, 100);
			CloseHandle(wait_handle);
			if (result == WAIT_OBJECT_0)
				break;
			if (result != WAIT_OBJECT_0 + 1)
				continue;
			state->has_credit = true;
		}
		const uint64_t lock_start = os_gettime_ns();
		obs_enter_graphics();
		const uint64_t lock_wait = os_gettime_ns() - lock_start;
		int result = 0;
		if (display->enabled)
			result = gs_swapchain_present_buffered_frame(display->swap, lock_wait);
		obs_leave_graphics();
		if (result != 0)
			state->has_credit = false;
		if (result == 0)
			WaitForSingleObject(state->stop, 1);
		else if (result != 1)
			WaitForSingleObject(state->stop, 50);
	}
	return NULL;
}
#endif

bool obs_display_set_buffered_present(obs_display_t *display, bool enabled)
{
#ifdef _WIN32
	if (!display || !display->swap)
		return false;
	if (!enabled) {
		obs_enter_graphics();
		struct obs_display_present_thread *state = display->present_thread;
		if (state)
			SetEvent(state->stop);
		obs_leave_graphics();
		if (!state)
			return true;
		pthread_join(state->thread, NULL);
		obs_enter_graphics();
		display->present_thread = NULL;
		display->present_credit = state->has_credit;
		gs_load_swapchain(display->swap);
		gs_swapchain_set_buffered_present(display->swap, false);
		display->render_stats = (struct obs_display_render_stats){0};
		reset_display_stats(display);
		obs_leave_graphics();
		CloseHandle(state->stop);
		bfree(state);
		blog(LOG_DEBUG, "[obs-preview-sync] display=%p, buffered_present=disabled", (void *)display);
		return true;
	}

	struct obs_display_present_thread *state = bzalloc(sizeof(*state));
	state->display = display;
	state->stop = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (!state->stop) {
		bfree(state);
		return false;
	}
	obs_enter_graphics();
	if (display->present_thread) {
		obs_leave_graphics();
		CloseHandle(state->stop);
		bfree(state);
		return true;
	}
	bool success = gs_swapchain_set_buffered_present(display->swap, true);
	if (success) {
		state->has_credit = display->present_credit;
		display->present_thread = state;
		success = pthread_create(&state->thread, NULL, display_present_thread, state) == 0;
		if (!success) {
			display->present_thread = NULL;
			gs_load_swapchain(display->swap);
			gs_swapchain_set_buffered_present(display->swap, false);
		}
		if (success) {
			display->present_credit = false;
			reset_display_stats(display);
		}
	}
	obs_leave_graphics();
	if (!success) {
		CloseHandle(state->stop);
		bfree(state);
	}
	blog(LOG_DEBUG, "[obs-preview-sync] display=%p, buffered_present=%s", (void *)display,
	     success ? "enabled" : "unavailable; using original path");
	return success;
#else
	UNUSED_PARAMETER(display);
	UNUSED_PARAMETER(enabled);
	return false;
#endif
}

bool obs_display_init(struct obs_display *display, const struct gs_init_data *graphics_data)
{
	pthread_mutex_init_value(&display->draw_callbacks_mutex);
	pthread_mutex_init_value(&display->draw_info_mutex);

#if defined(_WIN32)
	/* Conservative test for NVIDIA flickering in multi-GPU setups */
	display->use_clear_workaround = gs_get_adapter_count() > 1 && !gs_can_adapter_fast_clear();
#else
	display->use_clear_workaround = false;
#endif

	if (graphics_data) {
		display->swap = gs_swapchain_create(graphics_data);
		if (!display->swap) {
			blog(LOG_ERROR, "obs_display_init: Failed to "
					"create swap chain");
			return false;
		}

		const uint32_t cx = graphics_data->cx;
		const uint32_t cy = graphics_data->cy;
		display->cx = cx;
		display->cy = cy;
		display->next_cx = cx;
		display->next_cy = cy;
	}

	if (pthread_mutex_init(&display->draw_callbacks_mutex, NULL) != 0) {
		blog(LOG_ERROR, "obs_display_init: Failed to create mutex");
		return false;
	}
	if (pthread_mutex_init(&display->draw_info_mutex, NULL) != 0) {
		blog(LOG_ERROR, "obs_display_init: Failed to create mutex");
		return false;
	}

	display->enabled = true;
	reset_display_stats(display);
	return true;
}

obs_display_t *obs_display_create(const struct gs_init_data *graphics_data, uint32_t background_color)
{
	struct obs_display *display = bzalloc(sizeof(struct obs_display));

	gs_enter_context(obs->video.graphics);

	display->background_color = background_color;

	if (!obs_display_init(display, graphics_data)) {
		obs_display_destroy(display);
		display = NULL;
	} else {
		pthread_mutex_lock(&obs->data.displays_mutex);
		display->prev_next = &obs->data.first_display;
		display->next = obs->data.first_display;
		obs->data.first_display = display;
		if (display->next)
			display->next->prev_next = &display->next;
		pthread_mutex_unlock(&obs->data.displays_mutex);
	}

	gs_leave_context();

	return display;
}

void obs_display_free(obs_display_t *display)
{
	pthread_mutex_destroy(&display->draw_callbacks_mutex);
	pthread_mutex_destroy(&display->draw_info_mutex);
	da_free(display->draw_callbacks);

	if (display->swap) {
		gs_swapchain_destroy(display->swap);
		display->swap = NULL;
	}
}

void obs_display_destroy(obs_display_t *display)
{
	if (display) {
		pthread_mutex_lock(&obs->data.displays_mutex);
		if (display->prev_next)
			*display->prev_next = display->next;
		if (display->next)
			display->next->prev_next = display->prev_next;
		pthread_mutex_unlock(&obs->data.displays_mutex);

		obs_display_set_buffered_present(display, false);
		obs_enter_graphics();
		obs_display_free(display);
		obs_leave_graphics();

		bfree(display);
	}
}

void obs_display_resize(obs_display_t *display, uint32_t cx, uint32_t cy)
{
	if (!display)
		return;

	pthread_mutex_lock(&display->draw_info_mutex);

	display->next_cx = cx;
	display->next_cy = cy;

	pthread_mutex_unlock(&display->draw_info_mutex);
}

void obs_display_update_color_space(obs_display_t *display)
{
	if (!display)
		return;

	pthread_mutex_lock(&display->draw_info_mutex);

	display->update_color_space = true;

	pthread_mutex_unlock(&display->draw_info_mutex);
}

void obs_display_add_draw_callback(obs_display_t *display, void (*draw)(void *param, uint32_t cx, uint32_t cy),
				   void *param)
{
	if (!display)
		return;

	struct draw_callback data = {draw, param};

	pthread_mutex_lock(&display->draw_callbacks_mutex);
	da_push_back(display->draw_callbacks, &data);
	pthread_mutex_unlock(&display->draw_callbacks_mutex);
}

void obs_display_remove_draw_callback(obs_display_t *display, void (*draw)(void *param, uint32_t cx, uint32_t cy),
				      void *param)
{
	if (!display)
		return;

	struct draw_callback data = {draw, param};

	pthread_mutex_lock(&display->draw_callbacks_mutex);
	da_erase_item(display->draw_callbacks, &data);
	pthread_mutex_unlock(&display->draw_callbacks_mutex);
}

static inline bool render_display_begin(struct obs_display *display, uint32_t cx, uint32_t cy, bool update_color_space,
					bool buffered)
{
	struct vec4 clear_color;

	gs_load_swapchain(display->swap);

	if ((display->cx != cx) || (display->cy != cy)) {
		gs_resize(cx, cy);
		display->cx = cx;
		display->cy = cy;
	} else if (update_color_space) {
		gs_update_color_space();
	}

	bool success;
#ifdef _WIN32
	if (buffered) {
		success = gs_swapchain_begin_buffered_frame(display->swap);
	} else {
		success = display->present_credit || gs_is_present_ready();
		display->present_credit = false;
	}
#else
	UNUSED_PARAMETER(buffered);
	success = gs_is_present_ready();
#endif
	if (success) {
		gs_begin_scene();

		/*
		 * In contrast to OpenGL or Direct3D 11, Metal and Direct3D 12 require the clear color to use linear gamma
		 * as either the load command to clear the render target (Metal) or the explicit clear command seem to operate
		 * on the render target in linear space.
		 *
		 * As OpenGL is implemented via Metal on Apple Silicon Macs and "glClear" has to be emulated via an explicit
		 * render pass that returns the clear color for every fragment, the color becomes subject to automatic sRGB
		 * gamma encoding if the render target uses an sRGB color format.
		 */
#if defined(__APPLE__) && defined(__aarch64__)
		vec4_from_rgba_srgb(&clear_color, display->background_color);
#else
		if (gs_get_color_space() == GS_CS_SRGB)
			vec4_from_rgba(&clear_color, display->background_color);
		else
			vec4_from_rgba_srgb(&clear_color, display->background_color);
#endif
		clear_color.w = 1.0f;

		const bool use_clear_workaround = display->use_clear_workaround;

		uint32_t clear_flags = GS_CLEAR_DEPTH | GS_CLEAR_STENCIL;
		if (!use_clear_workaround)
			clear_flags |= GS_CLEAR_COLOR;
		gs_clear(clear_flags, &clear_color, 1.0f, 0);

		gs_enable_depth_test(false);
		/* gs_enable_blending(false); */
		gs_set_cull_mode(GS_NEITHER);

		gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);
		gs_set_viewport(0, 0, cx, cy);

		if (use_clear_workaround) {
			gs_effect_t *const solid_effect = obs->video.solid_effect;
			gs_effect_set_vec4(gs_effect_get_param_by_name(solid_effect, "color"), &clear_color);
			while (gs_effect_loop(solid_effect, "Solid"))
				gs_draw_sprite(NULL, 0, cx, cy);
		}
	}

	return success;
}

static inline void render_display_end()
{
	gs_end_scene();
}

static void log_display_render_stats(struct obs_display *display, uint64_t now)
{
	struct obs_display_render_stats *stats = &display->render_stats;
	const uint64_t elapsed_ns = now - stats->interval_start_ns;
	if (elapsed_ns < 1000000000ULL)
		return;

	const double elapsed_seconds = (double)elapsed_ns / 1000000000.0;
	const double not_ready_percent = stats->attempts ? 100.0 * (double)stats->not_ready / (double)stats->attempts
							 : 0.0;
	const double present_avg_ms =
		stats->present_calls ? (double)stats->present_total_ns / (double)stats->present_calls / 1000000.0 : 0.0;

	blog(LOG_DEBUG,
	     "[obs-display-stats] display=%p, size=%ux%u, interval=%.3fs, obs_fps=%.2f, "
	     "attempts=%" PRIu64 " (%.2f/s), not_ready=%" PRIu64 " (%.1f%%), "
	     "present_calls=%" PRIu64 " (%.2f/s), present_ms(avg/max)=%.3f/%.3f",
	     (void *)display, display->cx, display->cy, elapsed_seconds, obs_get_active_fps(), stats->attempts,
	     (double)stats->attempts / elapsed_seconds, stats->not_ready, not_ready_percent, stats->present_calls,
	     (double)stats->present_calls / elapsed_seconds, present_avg_ms, (double)stats->present_max_ns / 1000000.0);

	*stats = (struct obs_display_render_stats){.interval_start_ns = now};
}

void render_display(struct obs_display *display)
{
	uint32_t cx, cy;
	bool update_color_space;

	if (!display)
		return;
	if (!display->enabled) {
		display->render_stats = (struct obs_display_render_stats){0};
		return;
	}
	bool buffered = false;
#ifdef _WIN32
	buffered = display->present_thread != NULL;
#endif

	struct obs_display_render_stats *stats = &display->render_stats;
	display->draw_attempts++;
	if (!buffered) {
		if (!stats->interval_start_ns)
			stats->interval_start_ns = os_gettime_ns();
		stats->attempts++;
	}

	/* -------------------------------------------- */

	pthread_mutex_lock(&display->draw_info_mutex);

	cx = display->next_cx;
	cy = display->next_cy;
	update_color_space = display->update_color_space;

	display->update_color_space = false;

	pthread_mutex_unlock(&display->draw_info_mutex);

	/* -------------------------------------------- */

	if (render_display_begin(display, cx, cy, update_color_space, buffered)) {
		GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DISPLAY, "obs_display");

		pthread_mutex_lock(&display->draw_callbacks_mutex);

		for (size_t i = 0; i < display->draw_callbacks.num; i++) {
			struct draw_callback *callback;
			callback = display->draw_callbacks.array + i;

			callback->draw(callback->param, cx, cy);
		}

		pthread_mutex_unlock(&display->draw_callbacks_mutex);

		render_display_end();

		GS_DEBUG_MARKER_END();

#ifdef _WIN32
		if (buffered) {
			gs_swapchain_end_buffered_frame(display->swap);
			return;
		}
#endif
		const uint64_t present_start_ns = os_gettime_ns();
		gs_present();
		const uint64_t present_duration_ns = os_gettime_ns() - present_start_ns;
		struct preview_present_counters *totals = &display->presentation_stats.counters;
		totals->produced++;
		totals->submitted++;
		totals->present_total += present_duration_ns;
		if (present_duration_ns > totals->present_max)
			totals->present_max = present_duration_ns;
		const uint64_t present_end_ns = present_start_ns + present_duration_ns;
		if (display->last_present_time) {
			const uint64_t gap = present_end_ns - display->last_present_time;
			if (gap > totals->gap_max)
				totals->gap_max = gap;
		}
		display->last_present_time = present_end_ns;
		stats->present_calls++;
		stats->present_total_ns += present_duration_ns;
		if (present_duration_ns > stats->present_max_ns)
			stats->present_max_ns = present_duration_ns;
	} else if (!buffered) {
		stats->not_ready++;
		display->not_ready++;
	}

	if (!buffered)
		log_display_render_stats(display, os_gettime_ns());
}

void obs_display_set_enabled(obs_display_t *display, bool enable)
{
	if (display) {
		obs_enter_graphics();
		display->enabled = enable;
		obs_leave_graphics();
	}
}

bool obs_display_enabled(obs_display_t *display)
{
	return display ? display->enabled : false;
}

void obs_display_set_background_color(obs_display_t *display, uint32_t color)
{
	if (display)
		display->background_color = color;
}

void obs_display_size(obs_display_t *display, uint32_t *width, uint32_t *height)
{
	*width = 0;
	*height = 0;

	if (display) {
		pthread_mutex_lock(&display->draw_info_mutex);

		*width = display->cx;
		*height = display->cy;

		pthread_mutex_unlock(&display->draw_info_mutex);
	}
}
