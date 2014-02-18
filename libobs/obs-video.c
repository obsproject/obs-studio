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

static void tick_sources(uint64_t cur_time, uint64_t *last_time)
{
	size_t i;
	uint64_t delta_time;
	float seconds;

	if (!last_time)
		*last_time = cur_time - video_getframetime(obs->video.video);
	delta_time = cur_time - *last_time;
	seconds = (float)((double)delta_time / 1000000000.0);

	for (i = 0; i < obs->data.sources.num; i++)
		obs_source_video_tick(obs->data.sources.array[i], seconds);

	*last_time = cur_time;
}

/* in obs-display.c */
extern void render_display(struct obs_display *display);

static inline void render_displays(void)
{
	if (!obs->data.valid)
		return;

	gs_entercontext(obs_graphics());

	/* render extra displays/swaps */
	pthread_mutex_lock(&obs->data.displays_mutex);

	for (size_t i = 0; i < obs->data.displays.num; i++)
		render_display(obs->data.displays.array[i]);

	pthread_mutex_unlock(&obs->data.displays_mutex);

	/* render main display */
	render_display(&obs->video.main_display);

	gs_leavecontext();
}

static inline void set_render_size(uint32_t width, uint32_t height)
{
	gs_enable_depthtest(false);
	gs_enable_blending(false);
	gs_setcullmode(GS_NEITHER);

	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
	gs_setviewport(0, 0, width, height);
}

static inline void unmap_last_surface(struct obs_core_video *video)
{
	if (video->mapped_surface) {
		stagesurface_unmap(video->mapped_surface);
		video->mapped_surface = NULL;
	}
}

static inline void render_main_texture(struct obs_core_video *video,
		int cur_texture)
{
	struct vec4 clear_color;
	vec4_set(&clear_color, 0.3f, 0.0f, 0.0f, 1.0f);

	gs_setrendertarget(video->render_textures[cur_texture], NULL);
	gs_clear(GS_CLEAR_COLOR, &clear_color, 1.0f, 0);

	set_render_size(video->base_width, video->base_height);
	obs_view_render(&obs->data.main_view);

	video->textures_rendered[cur_texture] = true;
}

static inline void render_output_texture(struct obs_core_video *video,
		int cur_texture, int prev_texture)
{
	texture_t   texture = video->render_textures[prev_texture];
	texture_t   target  = video->output_textures[cur_texture];
	uint32_t    width   = texture_getwidth(target);
	uint32_t    height  = texture_getheight(target);

	/* TODO: replace with actual downscalers or unpackers */
	effect_t    effect  = video->default_effect;
	technique_t tech    = effect_gettechnique(effect, "DrawMatrix");
	eparam_t    image   = effect_getparambyname(effect, "image");
	eparam_t    matrix  = effect_getparambyname(effect, "color_matrix");
	size_t      passes, i;

	if (!video->textures_rendered[prev_texture])
		return;

	gs_setrendertarget(target, NULL);
	set_render_size(width, height);

	/* TODO: replace with programmable code */
	const float mat_val[16] =
	{
		-0.100644f, -0.338572f,  0.439216f,  0.501961f,
		 0.182586f,  0.614231f,  0.062007f,  0.062745f,
		 0.439216f, -0.398942f, -0.040274f,  0.501961f,
		 0.000000f,  0.000000f,  0.000000f,  1.000000f
	};

	effect_setval(effect, matrix, mat_val, sizeof(mat_val));
	effect_settexture(effect, image, texture);

	passes = technique_begin(tech);
	for (i = 0; i < passes; i++) {
		technique_beginpass(tech, i);
		gs_draw_sprite(texture, 0, width, height);
		technique_endpass(tech);
	}
	technique_end(tech);

	video->textures_output[cur_texture] = true;
}

static inline void set_eparam(effect_t effect, const char *name, float val)
{
	eparam_t param = effect_getparambyname(effect, name);
	effect_setfloat(effect, param, val);
}

static void render_convert_texture(struct obs_core_video *video,
		int cur_texture, int prev_texture)
{
	texture_t   texture = video->output_textures[prev_texture];
	texture_t   target  = video->convert_textures[cur_texture];
	float       fwidth  = (float)video->output_width;
	float       fheight = (float)video->output_height;
	size_t      passes, i;

	effect_t    effect  = video->conversion_effect;
	eparam_t    image   = effect_getparambyname(effect, "image");
	technique_t tech    = effect_gettechnique(effect,
			video->conversion_tech);

	if (!video->textures_output[prev_texture])
		return;

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

	effect_settexture(effect, image, texture);

	gs_setrendertarget(target, NULL);
	set_render_size(video->output_width, video->conversion_height);

	passes = technique_begin(tech);
	for (i = 0; i < passes; i++) {
		technique_beginpass(tech, i);
		gs_draw_sprite(texture, 0, video->output_width,
				video->conversion_height);
		technique_endpass(tech);
	}
	technique_end(tech);

	video->textures_converted[cur_texture] = true;
}

static inline void stage_output_texture(struct obs_core_video *video,
		int cur_texture, int prev_texture)
{
	texture_t   texture;
	bool        texture_ready;
	stagesurf_t copy = video->copy_surfaces[cur_texture];

	if (video->gpu_conversion) {
		texture = video->convert_textures[prev_texture];
		texture_ready = video->textures_converted[prev_texture];
	} else {
		texture = video->output_textures[prev_texture];
		texture_ready = video->output_textures[prev_texture];
	}

	unmap_last_surface(video);

	if (!texture_ready)
		return;

	gs_stage_texture(copy, texture);

	video->textures_copied[cur_texture] = true;
}

static inline void render_video(struct obs_core_video *video, int cur_texture,
		int prev_texture)
{
	gs_beginscene();

	gs_enable_depthtest(false);
	gs_setcullmode(GS_NEITHER);

	render_main_texture(video, cur_texture);
	render_output_texture(video, cur_texture, prev_texture);
	if (video->gpu_conversion)
		render_convert_texture(video, cur_texture, prev_texture);

	stage_output_texture(video, cur_texture, prev_texture);

	gs_setrendertarget(NULL, NULL);
	gs_enable_blending(true);

	gs_endscene();
}

/* TODO: replace with more optimal conversion */
static inline bool download_frame(struct obs_core_video *video,
		int prev_texture, struct video_data *frame)
{
	stagesurf_t surface = video->copy_surfaces[prev_texture];

	if (!video->textures_copied[prev_texture])
		return false;

	if (!stagesurface_map(surface, &frame->data[0], &frame->linesize[0]))
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
		struct video_data *frame, int cur_texture)
{
	struct source_frame *new_frame = &video->convert_frames[cur_texture];
	uint32_t src_linesize = frame->linesize[0];
	uint32_t dst_linesize = video->output_width * 4;
	uint32_t src_pos      = 0;

	for (size_t i = 0; i < 3; i++) {
		if (video->plane_linewidth[i] == 0)
			break;

		src_pos = make_aligned_linesize_offset(video->plane_offsets[i],
				dst_linesize, src_linesize);

		copy_dealign(new_frame->data[i], 0, dst_linesize,
				frame->data[0], src_pos, src_linesize,
				video->plane_sizes[i]);
	}

	/* replace with cached frames */
	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		frame->data[i]     = new_frame->data[i];
		frame->linesize[i] = new_frame->linesize[i];
	}
}

static bool set_gpu_converted_data(struct obs_core_video *video,
		struct video_data *frame, int cur_texture)
{
	if (frame->linesize[0] == video->output_width*4) {
		for (size_t i = 0; i < 3; i++) {
			if (video->plane_linewidth[i] == 0)
				break;

			frame->linesize[i] = video->plane_linewidth[i];
			frame->data[i] =
				frame->data[0] + video->plane_offsets[i];
		}

	} else {
		fix_gpu_converted_alignment(video, frame, cur_texture);
	}

	return true;
}

static bool convert_frame(struct obs_core_video *video,
		struct video_data *frame,
		const struct video_output_info *info, int cur_texture)
{
	struct source_frame *new_frame = &video->convert_frames[cur_texture];

	if (info->format == VIDEO_FORMAT_I420) {
		compress_uyvx_to_i420(
				frame->data[0], frame->linesize[0],
				0, info->height,
				new_frame->data, new_frame->linesize);

	} else if (info->format == VIDEO_FORMAT_NV12) {
		compress_uyvx_to_nv12(
				frame->data[0], frame->linesize[0],
				0, info->height,
				new_frame->data, new_frame->linesize);

	} else {
		blog(LOG_WARNING, "convert_frame: unsupported texture format");
		return false;
	}

	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		frame->data[i]     = new_frame->data[i];
		frame->linesize[i] = new_frame->linesize[i];
	}

	return true;
}

static inline void output_video_data(struct obs_core_video *video,
		struct video_data *frame, int cur_texture)
{
	const struct video_output_info *info;
	info = video_output_getinfo(video->video);

	if (video->gpu_conversion) {
		if (!set_gpu_converted_data(video, frame, cur_texture))
			return;

	} else if (format_is_yuv(info->format)) {
		if (!convert_frame(video, frame, info, cur_texture))
			return;
	}

	video_output_swap_frame(video->video, frame);
}

static inline void output_frame(uint64_t timestamp)
{
	struct obs_core_video *video = &obs->video;
	int cur_texture  = video->cur_texture;
	int prev_texture = cur_texture == 0 ? NUM_TEXTURES-1 : cur_texture-1;
	struct video_data frame;
	bool frame_ready;

	memset(&frame, 0, sizeof(struct video_data));
	frame.timestamp = timestamp;

	gs_entercontext(obs_graphics());

	render_video(video, cur_texture, prev_texture);
	frame_ready = download_frame(video, prev_texture, &frame);

	gs_leavecontext();

	if (frame_ready)
		output_video_data(video, &frame, cur_texture);

	if (++video->cur_texture == NUM_TEXTURES)
		video->cur_texture = 0;
}

void *obs_video_thread(void *param)
{
	uint64_t last_time = 0;

	while (video_output_wait(obs->video.video)) {
		uint64_t cur_time = video_gettime(obs->video.video);

		tick_sources(cur_time, &last_time);

		render_displays();

		output_frame(cur_time);

	}

	UNUSED_PARAMETER(param);
	return NULL;
}
