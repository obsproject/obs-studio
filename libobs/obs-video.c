/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include "obs.h"
#include "obs-internal.h"
#include "graphics/vec4.h"
#include "media-io/format-conversion.h"
#include "media-io/video-frame.h"

static uint64_t tick_sources(uint64_t cur_time, uint64_t last_time)
{
	struct obs_core_data *data = &obs->data;
	struct obs_source    *source;
	uint64_t             delta_time;
	float                seconds;

	if (!last_time)
		last_time = cur_time -
			video_output_get_frame_time(obs->video.video);

	delta_time = cur_time - last_time;
	seconds = (float)((double)delta_time / 1000000000.0);

	pthread_mutex_lock(&data->sources_mutex);

	/* call the tick function of each source */
	source = data->first_source;
	while (source) {
		obs_source_video_tick(source, seconds);
		source = (struct obs_source*)source->context.next;
	}

	pthread_mutex_unlock(&data->sources_mutex);

	return cur_time;
}

/* in obs-display.c */
extern void render_display(struct obs_display *display);

static inline void render_displays(void)
{
	struct obs_display *display;

	if (!obs->data.valid)
		return;

	gs_enter_context(obs->video.graphics);

	/* render extra displays/swaps */
	pthread_mutex_lock(&obs->data.displays_mutex);

	display = obs->data.first_display;
	while (display) {
		render_display(display);
		display = display->next;
	}

	pthread_mutex_unlock(&obs->data.displays_mutex);

	gs_leave_context();
}

static inline void set_render_size(uint32_t width, uint32_t height)
{
	gs_enable_depth_test(false);
	gs_set_cull_mode(GS_NEITHER);

	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_set_viewport(0, 0, width, height);
}

static inline void unmap_last_surface(struct obs_core_video *video)
{
	if (video->mapped_surface) {
		gs_stagesurface_unmap(video->mapped_surface);
		video->mapped_surface = NULL;
	}
}

static const char *render_main_texture_name = "render_main_texture";
static inline void render_main_texture(struct obs_core_video *video,
		int cur_texture)
{
	profile_start(render_main_texture_name);

	struct vec4 clear_color;
	vec4_set(&clear_color, 0.0f, 0.0f, 0.0f, 1.0f);

	gs_set_render_target(video->render_textures[cur_texture], NULL);
	gs_clear(GS_CLEAR_COLOR, &clear_color, 1.0f, 0);

	set_render_size(video->base_width, video->base_height);

	pthread_mutex_lock(&obs->data.draw_callbacks_mutex);

	for (size_t i = 0; i < obs->data.draw_callbacks.num; i++) {
		struct draw_callback *callback;
		callback = obs->data.draw_callbacks.array+i;

		callback->draw(callback->param,
				video->base_width, video->base_height);
	}

	pthread_mutex_unlock(&obs->data.draw_callbacks_mutex);

	obs_view_render(&obs->data.main_view);

	video->textures_rendered[cur_texture] = true;

	profile_end(render_main_texture_name);
}

static inline gs_effect_t *get_scale_effect_internal(
		struct obs_core_video *video)
{
	/* if the dimension is under half the size of the original image,
	 * bicubic/lanczos can't sample enough pixels to create an accurate
	 * image, so use the bilinear low resolution effect instead */
	if (video->output_width  < (video->base_width  / 2) &&
	    video->output_height < (video->base_height / 2)) {
		return video->bilinear_lowres_effect;
	}

	switch (video->scale_type) {
	case OBS_SCALE_BILINEAR: return video->default_effect;
	case OBS_SCALE_LANCZOS:  return video->lanczos_effect;
	case OBS_SCALE_BICUBIC:
	default:;
	}

	return video->bicubic_effect;
}

static inline bool resolution_close(struct obs_core_video *video,
		uint32_t width, uint32_t height)
{
	long width_cmp  = (long)video->base_width  - (long)width;
	long height_cmp = (long)video->base_height - (long)height;

	return labs(width_cmp) <= 16 && labs(height_cmp) <= 16;
}

static inline gs_effect_t *get_scale_effect(struct obs_core_video *video,
		uint32_t width, uint32_t height)
{
	if (resolution_close(video, width, height)) {
		return video->default_effect;
	} else {
		/* if the scale method couldn't be loaded, use either bicubic
		 * or bilinear by default */
		gs_effect_t *effect = get_scale_effect_internal(video);
		if (!effect)
			effect = !!video->bicubic_effect ?
				video->bicubic_effect :
				video->default_effect;
		return effect;
	}
}

static const char *render_output_texture_name = "render_output_texture";
static inline void render_output_texture(struct obs_core_video *video,
		int cur_texture, int prev_texture)
{
	profile_start(render_output_texture_name);

	gs_texture_t *texture = video->render_textures[prev_texture];
	gs_texture_t *target  = video->output_textures[cur_texture];
	uint32_t     width   = gs_texture_get_width(target);
	uint32_t     height  = gs_texture_get_height(target);
	struct vec2  base_i;

	vec2_set(&base_i,
		1.0f / (float)video->base_width,
		1.0f / (float)video->base_height);

	gs_effect_t    *effect  = get_scale_effect(video, width, height);
	gs_technique_t *tech    = gs_effect_get_technique(effect, "DrawMatrix");
	gs_eparam_t    *image   = gs_effect_get_param_by_name(effect, "image");
	gs_eparam_t    *matrix  = gs_effect_get_param_by_name(effect,
			"color_matrix");
	gs_eparam_t    *bres_i  = gs_effect_get_param_by_name(effect,
			"base_dimension_i");
	size_t      passes, i;

	if (!video->textures_rendered[prev_texture])
		goto end;

	gs_set_render_target(target, NULL);
	set_render_size(width, height);

	if (bres_i)
		gs_effect_set_vec2(bres_i, &base_i);

	gs_effect_set_val(matrix, video->color_matrix, sizeof(float) * 16);
	gs_effect_set_texture(image, texture);

	gs_enable_blending(false);
	passes = gs_technique_begin(tech);
	for (i = 0; i < passes; i++) {
		gs_technique_begin_pass(tech, i);
		gs_draw_sprite(texture, 0, width, height);
		gs_technique_end_pass(tech);
	}
	gs_technique_end(tech);
	gs_enable_blending(true);

	video->textures_output[cur_texture] = true;

end:
	profile_end(render_output_texture_name);
}

static inline void set_eparam(gs_effect_t *effect, const char *name, float val)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);
	gs_effect_set_float(param, val);
}

static const char *render_convert_texture_name = "render_convert_texture";
static void render_convert_texture(struct obs_core_video *video,
		int cur_texture, int prev_texture)
{
	profile_start(render_convert_texture_name);

	gs_texture_t *texture = video->output_textures[prev_texture];
	gs_texture_t *target  = video->convert_textures[cur_texture];
	float        fwidth  = (float)video->output_width;
	float        fheight = (float)video->output_height;
	size_t       passes, i;

	gs_effect_t    *effect  = video->conversion_effect;
	gs_eparam_t    *image   = gs_effect_get_param_by_name(effect, "image");
	gs_technique_t *tech    = gs_effect_get_technique(effect,
			video->conversion_tech);

	if (!video->textures_output[prev_texture])
		goto end;

	set_eparam(effect, "u_plane_offset", (float)video->plane_offsets[1]);
	set_eparam(effect, "v_plane_offset", (float)video->plane_offsets[2]);
	set_eparam(effect, "width",  fwidth);
	set_eparam(effect, "height", fheight);
	set_eparam(effect, "width_i",  1.0f / fwidth);
	set_eparam(effect, "height_i", 1.0f / fheight);
	set_eparam(effect, "width_d2",  fwidth  * 0.5f);
	set_eparam(effect, "height_d2", fheight * 0.5f);
	set_eparam(effect, "width_d2_i",  1.0f / (fwidth  * 0.5f));
	set_eparam(effect, "height_d2_i", 1.0f / (fheight * 0.5f));
	set_eparam(effect, "input_height", (float)video->conversion_height);

	gs_effect_set_texture(image, texture);

	gs_set_render_target(target, NULL);
	set_render_size(video->output_width, video->conversion_height);

	gs_enable_blending(false);
	passes = gs_technique_begin(tech);
	for (i = 0; i < passes; i++) {
		gs_technique_begin_pass(tech, i);
		gs_draw_sprite(texture, 0, video->output_width,
				video->conversion_height);
		gs_technique_end_pass(tech);
	}
	gs_technique_end(tech);
	gs_enable_blending(true);

	video->textures_converted[cur_texture] = true;

end:
	profile_end(render_convert_texture_name);
}

static const char *stage_output_texture_name = "stage_output_texture";
static inline void stage_output_texture(struct obs_core_video *video,
		int cur_texture, int prev_texture)
{
	profile_start(stage_output_texture_name);

	gs_texture_t   *texture;
	bool        texture_ready;
	gs_stagesurf_t *copy = video->copy_surfaces[cur_texture];

	if (video->gpu_conversion) {
		texture = video->convert_textures[prev_texture];
		texture_ready = video->textures_converted[prev_texture];
	} else {
		texture = video->output_textures[prev_texture];
		texture_ready = video->output_textures[prev_texture];
	}

	unmap_last_surface(video);

	if (!texture_ready)
		goto end;

	gs_stage_texture(copy, texture);

	video->textures_copied[cur_texture] = true;

end:
	profile_end(stage_output_texture_name);
}

static inline void render_video(struct obs_core_video *video, int cur_texture,
		int prev_texture)
{
	gs_begin_scene();

	gs_enable_depth_test(false);
	gs_set_cull_mode(GS_NEITHER);

	render_main_texture(video, cur_texture);
	render_output_texture(video, cur_texture, prev_texture);
	if (video->gpu_conversion)
		render_convert_texture(video, cur_texture, prev_texture);

	stage_output_texture(video, cur_texture, prev_texture);

	gs_set_render_target(NULL, NULL);
	gs_enable_blending(true);

	gs_end_scene();
}

static inline bool download_frame(struct obs_core_video *video,
		int prev_texture, struct video_data *frame)
{
	gs_stagesurf_t *surface = video->copy_surfaces[prev_texture];

	if (!video->textures_copied[prev_texture])
		return false;

	if (!gs_stagesurface_map(surface, &frame->data[0], &frame->linesize[0]))
		return false;

	video->mapped_surface = surface;
	return true;
}

static inline uint32_t calc_linesize(uint32_t pos, uint32_t linesize)
{
	uint32_t size = pos % linesize;
	return size ? size : linesize;
}

static void copy_dealign(
		uint8_t *dst, uint32_t dst_pos, uint32_t dst_linesize,
		const uint8_t *src, uint32_t src_pos, uint32_t src_linesize,
		uint32_t remaining)
{
	while (remaining) {
		uint32_t src_remainder = src_pos % src_linesize;
		uint32_t dst_offset = dst_linesize - src_remainder;
		uint32_t src_offset = src_linesize - src_remainder;

		if (remaining < dst_offset) {
			memcpy(dst + dst_pos, src + src_pos, remaining);
			src_pos += remaining;
			dst_pos += remaining;
			remaining = 0;
		} else {
			memcpy(dst + dst_pos, src + src_pos, dst_offset);
			src_pos += src_offset;
			dst_pos += dst_offset;
			remaining -= dst_offset;
		}
	}
}

static inline uint32_t make_aligned_linesize_offset(uint32_t offset,
		uint32_t dst_linesize, uint32_t src_linesize)
{
	uint32_t remainder = offset % dst_linesize;
	return (offset / dst_linesize) * src_linesize + remainder;
}

static void fix_gpu_converted_alignment(struct obs_core_video *video,
		struct video_frame *output, const struct video_data *input)
{
	uint32_t src_linesize = input->linesize[0];
	uint32_t dst_linesize = output->linesize[0] * 4;
	uint32_t src_pos      = 0;

	for (size_t i = 0; i < 3; i++) {
		if (video->plane_linewidth[i] == 0)
			break;

		src_pos = make_aligned_linesize_offset(video->plane_offsets[i],
				dst_linesize, src_linesize);

		copy_dealign(output->data[i], 0, dst_linesize,
				input->data[0], src_pos, src_linesize,
				video->plane_sizes[i]);
	}
}

static void set_gpu_converted_data(struct obs_core_video *video,
		struct video_frame *output, const struct video_data *input,
		const struct video_output_info *info)
{
	if (input->linesize[0] == video->output_width*4) {
		struct video_frame frame;

		for (size_t i = 0; i < 3; i++) {
			if (video->plane_linewidth[i] == 0)
				break;

			frame.linesize[i] = video->plane_linewidth[i];
			frame.data[i] =
				input->data[0] + video->plane_offsets[i];
		}

		video_frame_copy(output, &frame, info->format, info->height);

	} else {
		fix_gpu_converted_alignment(video, output, input);
	}
}

static void convert_frame(
		struct video_frame *output, const struct video_data *input,
		const struct video_output_info *info)
{
	if (info->format == VIDEO_FORMAT_I420) {
		compress_uyvx_to_i420(
				input->data[0], input->linesize[0],
				0, info->height,
				output->data, output->linesize);

	} else if (info->format == VIDEO_FORMAT_NV12) {
		compress_uyvx_to_nv12(
				input->data[0], input->linesize[0],
				0, info->height,
				output->data, output->linesize);

	} else if (info->format == VIDEO_FORMAT_I444) {
		convert_uyvx_to_i444(
				input->data[0], input->linesize[0],
				0, info->height,
				output->data, output->linesize);

	} else {
		blog(LOG_ERROR, "convert_frame: unsupported texture format");
	}
}

static inline void copy_rgbx_frame(
		struct video_frame *output, const struct video_data *input,
		const struct video_output_info *info)
{
	uint8_t *in_ptr = input->data[0];
	uint8_t *out_ptr = output->data[0];

	/* if the line sizes match, do a single copy */
	if (input->linesize[0] == output->linesize[0]) {
		memcpy(out_ptr, in_ptr, input->linesize[0] * info->height);
	} else {
		for (size_t y = 0; y < info->height; y++) {
			memcpy(out_ptr, in_ptr, info->width * 4);
			in_ptr += input->linesize[0];
			out_ptr += output->linesize[0];
		}
	}
}

static inline void output_video_data(struct obs_core_video *video,
		struct video_data *input_frame, int count)
{
	const struct video_output_info *info;
	struct video_frame output_frame;
	bool locked;

	info = video_output_get_info(video->video);

	locked = video_output_lock_frame(video->video, &output_frame, count,
			input_frame->timestamp);
	if (locked) {
		if (video->gpu_conversion) {
			set_gpu_converted_data(video, &output_frame,
					input_frame, info);

		} else if (format_is_yuv(info->format)) {
			convert_frame(&output_frame, input_frame, info);
		} else {
			copy_rgbx_frame(&output_frame, input_frame, info);
		}

		video_output_unlock_frame(video->video);
	}
}

static inline void video_sleep(struct obs_core_video *video,
		uint64_t *p_time, uint64_t interval_ns)
{
	struct obs_vframe_info vframe_info;
	uint64_t cur_time = *p_time;
	uint64_t t = cur_time + interval_ns;
	int count;

	if (os_sleepto_ns(t)) {
		*p_time = t;
		count = 1;
	} else {
		count = (int)((os_gettime_ns() - cur_time) / interval_ns);
		*p_time = cur_time + interval_ns * count;
	}

	video->total_frames += count;
	video->lagged_frames += count - 1;

	vframe_info.timestamp = cur_time;
	vframe_info.count = count;
	circlebuf_push_back(&video->vframe_info_buffer, &vframe_info,
			sizeof(vframe_info));
}

static const char *output_frame_gs_context_name = "gs_context(video->graphics)";
static const char *output_frame_render_video_name = "render_video";
static const char *output_frame_download_frame_name = "download_frame";
static const char *output_frame_gs_flush_name = "gs_flush";
static const char *output_frame_output_video_data_name = "output_video_data";
static inline void output_frame(void)
{
	struct obs_core_video *video = &obs->video;
	int cur_texture  = video->cur_texture;
	int prev_texture = cur_texture == 0 ? NUM_TEXTURES-1 : cur_texture-1;
	struct video_data frame;
	bool frame_ready;

	memset(&frame, 0, sizeof(struct video_data));

	profile_start(output_frame_gs_context_name);
	gs_enter_context(video->graphics);

	profile_start(output_frame_render_video_name);
	render_video(video, cur_texture, prev_texture);
	profile_end(output_frame_render_video_name);

	profile_start(output_frame_download_frame_name);
	frame_ready = download_frame(video, prev_texture, &frame);
	profile_end(output_frame_download_frame_name);

	profile_start(output_frame_gs_flush_name);
	gs_flush();
	profile_end(output_frame_gs_flush_name);

	gs_leave_context();
	profile_end(output_frame_gs_context_name);

	if (frame_ready) {
		struct obs_vframe_info vframe_info;
		circlebuf_pop_front(&video->vframe_info_buffer, &vframe_info,
				sizeof(vframe_info));

		frame.timestamp = vframe_info.timestamp;
		profile_start(output_frame_output_video_data_name);
		output_video_data(video, &frame, vframe_info.count);
		profile_end(output_frame_output_video_data_name);
	}

	if (++video->cur_texture == NUM_TEXTURES)
		video->cur_texture = 0;
}

#define NBSP "\xC2\xA0"

static const char *tick_sources_name = "tick_sources";
static const char *render_displays_name = "render_displays";
static const char *output_frame_name = "output_frame";
void *obs_video_thread(void *param)
{
	uint64_t last_time = 0;
	uint64_t interval = video_output_get_frame_time(obs->video.video);
	uint64_t frame_time_total_ns = 0;
	uint64_t fps_total_ns = 0;
	uint32_t fps_total_frames = 0;

	obs->video.video_time = os_gettime_ns();

	os_set_thread_name("libobs: graphics thread");

	const char *video_thread_name =
		profile_store_name(obs_get_profiler_name_store(),
			"obs_video_thread(%g"NBSP"ms)", interval / 1000000.);
	profile_register_root(video_thread_name, interval);

	while (!video_output_stopped(obs->video.video)) {
		uint64_t frame_start = os_gettime_ns();
		uint64_t frame_time_ns;

		profile_start(video_thread_name);

		profile_start(tick_sources_name);
		last_time = tick_sources(obs->video.video_time, last_time);
		profile_end(tick_sources_name);

		profile_start(render_displays_name);
		render_displays();
		profile_end(render_displays_name);

		profile_start(output_frame_name);
		output_frame();
		profile_end(output_frame_name);

		frame_time_ns = os_gettime_ns() - frame_start;

		profile_end(video_thread_name);

		profile_reenable_thread();

		video_sleep(&obs->video, &obs->video.video_time, interval);

		frame_time_total_ns += frame_time_ns;
		fps_total_ns += (obs->video.video_time - last_time);
		fps_total_frames++;

		if (fps_total_ns >= 1000000000ULL) {
			obs->video.video_fps = (double)fps_total_frames /
				((double)fps_total_ns / 1000000000.0);
			obs->video.video_avg_frame_time_ns =
				frame_time_total_ns / (uint64_t)fps_total_frames;

			frame_time_total_ns = 0;
			fps_total_ns = 0;
			fps_total_frames = 0;
		}
	}

	UNUSED_PARAMETER(param);
	return NULL;
}
