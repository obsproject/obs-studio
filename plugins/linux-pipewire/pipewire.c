/* pipewire.c
 *
 * Copyright 2020 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pipewire.h"

#include <util/darray.h>
#include <media-io/video-frame.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

#include <fcntl.h>
#include <glad/glad.h>
#include <linux/dma-buf.h>
#include <libdrm/drm_fourcc.h>
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/debug/format.h>
#include <spa/debug/types.h>
#include <spa/param/video/type-info.h>
#include <spa/utils/result.h>

#define CURSOR_META_SIZE(width, height)                                    \
	(sizeof(struct spa_meta_cursor) + sizeof(struct spa_meta_bitmap) + \
	 width * height * 4)

struct obs_pw_version {
	int major;
	int minor;
	int micro;
};

struct format_info {
	uint32_t spa_format;
	uint32_t drm_format;
	DARRAY(uint64_t) modifiers;
};

struct _obs_pipewire {
	int pipewire_fd;

	struct pw_thread_loop *thread_loop;
	struct pw_context *context;

	struct pw_core *core;
	struct spa_hook core_listener;
	int sync_id;

	struct obs_pw_version server_version;

	struct pw_registry *registry;
	struct spa_hook registry_listener;

	GPtrArray *streams;
};

struct _obs_pipewire_stream {
	obs_pipewire *obs_pw;
	obs_pipewire_stream_data obs_data;

	gs_texture_t *texture;

	struct pw_stream *stream;
	struct spa_hook stream_listener;
	struct spa_source *reneg;
	uint32_t seq;

	struct spa_video_info format;

	struct {
		bool valid;
		int x, y;
		uint32_t width, height;
	} crop;

	struct {
		bool visible;
		bool valid;
		int x, y;
		int hotspot_x, hotspot_y;
		int width, height;
		gs_texture_t *texture;
	} cursor;

	struct obs_video_info video_info;
	bool negotiated;

	DARRAY(struct format_info) format_info;
};

/* auxiliary methods */

static bool parse_pw_version(struct obs_pw_version *dst, const char *version)
{
	int n_matches = sscanf(version, "%d.%d.%d", &dst->major, &dst->minor,
			       &dst->micro);
	return n_matches == 3;
}

static bool check_pw_version(const struct obs_pw_version *pw_version, int major,
			     int minor, int micro)
{
	if (pw_version->major != major)
		return pw_version->major > major;
	if (pw_version->minor != minor)
		return pw_version->minor > minor;
	return pw_version->micro >= micro;
}

static void update_pw_versions(obs_pipewire *obs_pw, const char *version)
{
	blog(LOG_INFO, "[pipewire] Server version: %s", version);
	blog(LOG_INFO, "[pipewire] Library version: %s",
	     pw_get_library_version());
	blog(LOG_INFO, "[pipewire] Header version: %s",
	     pw_get_headers_version());

	if (!parse_pw_version(&obs_pw->server_version, version))
		blog(LOG_WARNING, "[pipewire] failed to parse server version");
}

static void teardown_pipewire(obs_pipewire *obs_pw)
{
	if (obs_pw->thread_loop) {
		pw_thread_loop_wait(obs_pw->thread_loop);
		pw_thread_loop_stop(obs_pw->thread_loop);
	}

	g_clear_pointer(&obs_pw->context, pw_context_destroy);
	g_clear_pointer(&obs_pw->thread_loop, pw_thread_loop_destroy);

	if (obs_pw->pipewire_fd > 0) {
		close(obs_pw->pipewire_fd);
		obs_pw->pipewire_fd = 0;
	}
}

static inline bool has_effective_crop(obs_pipewire_stream *obs_pw_stream)
{
	return obs_pw_stream->crop.valid &&
	       (obs_pw_stream->crop.x != 0 || obs_pw_stream->crop.y != 0 ||
		obs_pw_stream->crop.width <
			obs_pw_stream->format.info.raw.size.width ||
		obs_pw_stream->crop.height <
			obs_pw_stream->format.info.raw.size.height);
}

struct format_data {
	uint32_t spa_format;
	uint32_t drm_format;
	enum gs_color_format gs_format;
	enum video_format video_format;
	bool swap_red_blue;
	uint32_t bpp;
	const char *pretty_name;
};

static const struct format_data supported_formats[] = {
	{
		SPA_VIDEO_FORMAT_BGRA,
		DRM_FORMAT_ARGB8888,
		GS_BGRA,
		VIDEO_FORMAT_BGRA,
		false,
		4,
		"ARGB8888",
	},
	{
		SPA_VIDEO_FORMAT_RGBA,
		DRM_FORMAT_ABGR8888,
		GS_RGBA,
		VIDEO_FORMAT_RGBA,
		false,
		4,
		"ABGR8888",
	},
	{
		SPA_VIDEO_FORMAT_BGRx,
		DRM_FORMAT_XRGB8888,
		GS_BGRX,
		VIDEO_FORMAT_BGRX,
		false,
		4,
		"XRGB8888",
	},
	{
		SPA_VIDEO_FORMAT_RGBx,
		DRM_FORMAT_XBGR8888,
		GS_BGRX,
		VIDEO_FORMAT_NONE,
		true,
		4,
		"XBGR8888",
	},
	{
		SPA_VIDEO_FORMAT_YUY2,
		DRM_FORMAT_YUYV,
		GS_UNKNOWN,
		VIDEO_FORMAT_YUY2,
		false,
		2,
		"YUYV422",
	},
};

#define N_SUPPORTED_FORMATS \
	(sizeof(supported_formats) / sizeof(supported_formats[0]))

static const uint32_t supported_formats_async[] = {
	SPA_VIDEO_FORMAT_RGBA,
	SPA_VIDEO_FORMAT_YUY2,
};

#define N_SUPPORTED_FORMATS_ASYNC \
	(sizeof(supported_formats_async) / sizeof(supported_formats_async[0]))

static const uint32_t supported_formats_sync[] = {
	SPA_VIDEO_FORMAT_BGRA,
	SPA_VIDEO_FORMAT_RGBA,
	SPA_VIDEO_FORMAT_BGRx,
	SPA_VIDEO_FORMAT_RGBx,
};

#define N_SUPPORTED_FORMATS_SYNC \
	(sizeof(supported_formats_sync) / sizeof(supported_formats_sync[0]))

static const uint32_t supported_formats_output[] = {
	SPA_VIDEO_FORMAT_BGRA, SPA_VIDEO_FORMAT_RGBA, SPA_VIDEO_FORMAT_BGRx,
	SPA_VIDEO_FORMAT_RGBx, SPA_VIDEO_FORMAT_YUY2,
};

#define N_SUPPORTED_FORMATS_OUTPUT \
	(sizeof(supported_formats_output) / sizeof(supported_formats_output[0]))

static bool
lookup_format_info_from_spa_format(uint32_t spa_format,
				   struct format_data *out_format_data)
{
	for (size_t i = 0; i < N_SUPPORTED_FORMATS; i++) {
		if (supported_formats[i].spa_format != spa_format)
			continue;

		if (out_format_data)
			*out_format_data = supported_formats[i];

		return true;
	}
	return false;
}

static void swap_texture_red_blue(gs_texture_t *texture)
{
	GLuint gl_texure = *(GLuint *)gs_texture_get_obj(texture);

	glBindTexture(GL_TEXTURE_2D, gl_texure);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static inline struct spa_pod *build_format(struct spa_pod_builder *b,
					   struct obs_video_info *ovi,
					   uint32_t format, uint64_t *modifiers,
					   size_t modifier_count)
{
	struct spa_pod_frame format_frame;

	/* Make an object of type SPA_TYPE_OBJECT_Format and id SPA_PARAM_EnumFormat.
	 * The object type is important because it defines the properties that are
	 * acceptable. The id gives more context about what the object is meant to
	 * contain. In this case we enumerate supported formats. */
	spa_pod_builder_push_object(b, &format_frame, SPA_TYPE_OBJECT_Format,
				    SPA_PARAM_EnumFormat);
	/* add media type and media subtype properties */
	spa_pod_builder_add(b, SPA_FORMAT_mediaType,
			    SPA_POD_Id(SPA_MEDIA_TYPE_video), 0);
	spa_pod_builder_add(b, SPA_FORMAT_mediaSubtype,
			    SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), 0);

	/* formats */
	spa_pod_builder_add(b, SPA_FORMAT_VIDEO_format, SPA_POD_Id(format), 0);

	/* modifier */
	if (modifier_count > 0) {
		struct spa_pod_frame modifier_frame;

		/* build an enumeration of modifiers */
		spa_pod_builder_prop(b, SPA_FORMAT_VIDEO_modifier,
				     SPA_POD_PROP_FLAG_MANDATORY |
					     SPA_POD_PROP_FLAG_DONT_FIXATE);

		spa_pod_builder_push_choice(b, &modifier_frame, SPA_CHOICE_Enum,
					    0);

		/* The first element of choice pods is the preferred value. Here
		 * we arbitrarily pick the first modifier as the preferred one.
		 */
		spa_pod_builder_long(b, modifiers[0]);

		/* modifiers from  an array */
		for (uint32_t i = 0; i < modifier_count; i++)
			spa_pod_builder_long(b, modifiers[i]);

		spa_pod_builder_pop(b, &modifier_frame);
	}
	/* add size and framerate ranges */
	spa_pod_builder_add(b, SPA_FORMAT_VIDEO_size,
			    SPA_POD_CHOICE_RANGE_Rectangle(
				    &SPA_RECTANGLE(320, 240), // Arbitrary
				    &SPA_RECTANGLE(1, 1),
				    &SPA_RECTANGLE(8192, 4320)),
			    SPA_FORMAT_VIDEO_framerate,
			    SPA_POD_CHOICE_RANGE_Fraction(
				    &SPA_FRACTION(ovi->fps_num, ovi->fps_den),
				    &SPA_FRACTION(0, 1), &SPA_FRACTION(360, 1)),
			    0);
	return spa_pod_builder_pop(b, &format_frame);
}

static bool build_format_params(obs_pipewire_stream *obs_pw_stream,
				struct spa_pod_builder *pod_builder,
				const struct spa_pod ***param_list,
				uint32_t *n_params)
{
	obs_pipewire *obs_pw = obs_pw_stream->obs_pw;
	uint32_t params_count = 0;

	const struct spa_pod **params;
	params = bzalloc(2 * obs_pw_stream->format_info.num *
			 sizeof(struct spa_pod *));

	if (!params) {
		blog(LOG_ERROR,
		     "[pipewire] Failed to allocate memory for param pointers");
		return false;
	}

	if (!check_pw_version(&obs_pw->server_version, 0, 3, 33))
		goto build_shm;

	for (size_t i = 0; i < obs_pw_stream->format_info.num; i++) {
		if (obs_pw_stream->format_info.array[i].modifiers.num == 0) {
			continue;
		}
		params[params_count++] = build_format(
			pod_builder, &obs_pw_stream->video_info,
			obs_pw_stream->format_info.array[i].spa_format,
			obs_pw_stream->format_info.array[i].modifiers.array,
			obs_pw_stream->format_info.array[i].modifiers.num);
	}

build_shm:
	for (size_t i = 0; i < obs_pw_stream->format_info.num; i++) {
		params[params_count++] = build_format(
			pod_builder, &obs_pw_stream->video_info,
			obs_pw_stream->format_info.array[i].spa_format, NULL,
			0);
	}
	*param_list = params;
	*n_params = params_count;
	return true;
}

static bool drm_format_available(uint32_t drm_format, uint32_t *drm_formats,
				 size_t n_drm_formats)
{
	for (size_t j = 0; j < n_drm_formats; j++) {
		if (drm_format == drm_formats[j]) {
			return true;
		}
	}
	return false;
}

static void init_format_info_async(obs_pipewire_stream *obs_pw_stream)
{
	da_init(obs_pw_stream->format_info);

	for (size_t i = 0; i < N_SUPPORTED_FORMATS_ASYNC; i++) {
		struct format_info *info, tmp;
		if (!lookup_format_info_from_spa_format(
			    supported_formats_async[i], &tmp))
			continue;

		info = da_push_back_new(obs_pw_stream->format_info);
		da_init(info->modifiers);
		info->spa_format = tmp.spa_format;
		info->drm_format = tmp.drm_format;
	}
}

static void init_format_info_sync(obs_pipewire_stream *obs_pw_stream)
{
	da_init(obs_pw_stream->format_info);

	obs_enter_graphics();

	enum gs_dmabuf_flags dmabuf_flags;
	uint32_t *drm_formats = NULL;
	size_t n_drm_formats;

	bool capabilities_queried = gs_query_dmabuf_capabilities(
		&dmabuf_flags, &drm_formats, &n_drm_formats);

	for (size_t i = 0; i < N_SUPPORTED_FORMATS_SYNC; i++) {
		struct format_info *info, tmp;

		if (!lookup_format_info_from_spa_format(
			    supported_formats_sync[i], &tmp))
			continue;

		if (!drm_format_available(tmp.drm_format, drm_formats,
					  n_drm_formats))
			continue;

		info = da_push_back_new(obs_pw_stream->format_info);
		da_init(info->modifiers);
		info->spa_format = tmp.spa_format;
		info->drm_format = tmp.drm_format;

		if (!capabilities_queried)
			continue;

		size_t n_modifiers;
		uint64_t *modifiers = NULL;
		if (gs_query_dmabuf_modifiers_for_format(
			    tmp.drm_format, &modifiers, &n_modifiers)) {
			da_push_back_array(info->modifiers, modifiers,
					   n_modifiers);
		}
		bfree(modifiers);

		if (dmabuf_flags &
		    GS_DMABUF_FLAG_IMPLICIT_MODIFIERS_SUPPORTED) {
			uint64_t modifier_implicit = DRM_FORMAT_MOD_INVALID;
			da_push_back(info->modifiers, &modifier_implicit);
		}
	}
	obs_leave_graphics();

	bfree(drm_formats);
}

static void init_format_info_output(obs_pipewire_stream *obs_pw_stream)
{
	da_init(obs_pw_stream->format_info);

	for (size_t i = 0; i < N_SUPPORTED_FORMATS_OUTPUT; i++) {
		struct format_info *info, tmp;
		if (!lookup_format_info_from_spa_format(
			    supported_formats_output[i], &tmp))
			continue;

		info = da_push_back_new(obs_pw_stream->format_info);
		da_init(info->modifiers);
		info->spa_format = tmp.spa_format;
		info->drm_format = tmp.drm_format;
	}
}

static void init_format_info(obs_pipewire_stream *obs_pw_stream)
{
	if (obs_pw_stream->obs_data.type == OBS_PIPEWIRE_STREAM_TYPE_OUTPUT) {
		init_format_info_output(obs_pw_stream);
		return;
	}

	uint32_t output_flags;

	output_flags = obs_source_get_output_flags(obs_pw_stream->source);

	if (output_flags & OBS_SOURCE_VIDEO) {
		if (output_flags & OBS_SOURCE_ASYNC)
			init_format_info_async(obs_pw_stream);
		else
			init_format_info_sync(obs_pw_stream);
	}
}

static void clear_format_info(obs_pipewire_stream *obs_pw_stream)
{
	for (size_t i = 0; i < obs_pw_stream->format_info.num; i++) {
		da_free(obs_pw_stream->format_info.array[i].modifiers);
	}
	da_free(obs_pw_stream->format_info);
}

static void remove_modifier_from_format(obs_pipewire_stream *obs_pw_stream,
					uint32_t spa_format, uint64_t modifier)
{
	obs_pipewire *obs_pw = obs_pw_stream->obs_pw;

	for (size_t i = 0; i < obs_pw_stream->format_info.num; i++) {
		if (obs_pw_stream->format_info.array[i].spa_format !=
		    spa_format)
			continue;

		if (!check_pw_version(&obs_pw->server_version, 0, 3, 40)) {
			da_erase_range(
				obs_pw_stream->format_info.array[i].modifiers,
				0,
				obs_pw_stream->format_info.array[i]
						.modifiers.num -
					1);
			continue;
		}

		int idx = da_find(obs_pw_stream->format_info.array[i].modifiers,
				  &modifier, 0);
		while (idx != -1) {
			da_erase(obs_pw_stream->format_info.array[i].modifiers,
				 idx);
			idx = da_find(
				obs_pw_stream->format_info.array[i].modifiers,
				&modifier, 0);
		}
	}
}

static void renegotiate_format(void *data, uint64_t expirations)
{
	UNUSED_PARAMETER(expirations);
	obs_pipewire_stream *obs_pw_stream = (obs_pipewire_stream *)data;
	obs_pipewire *obs_pw = obs_pw_stream->obs_pw;
	const struct spa_pod **params = NULL;

	blog(LOG_INFO, "[pipewire] Renegotiating stream");

	pw_thread_loop_lock(obs_pw->thread_loop);

	uint8_t params_buffer[2048];
	struct spa_pod_builder pod_builder =
		SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));
	uint32_t n_params;
	if (!build_format_params(obs_pw_stream, &pod_builder, &params,
				 &n_params)) {
		teardown_pipewire(obs_pw);
		pw_thread_loop_unlock(obs_pw->thread_loop);
		return;
	}

	pw_stream_update_params(obs_pw_stream->stream, params, n_params);
	pw_thread_loop_unlock(obs_pw->thread_loop);
	bfree(params);
}

/* ------------------------------------------------- */

static inline struct pw_buffer *find_latest_buffer(struct pw_stream *stream)
{
	struct pw_buffer *b;

	/* Find the most recent buffer */
	b = NULL;
	while (true) {
		struct pw_buffer *aux = pw_stream_dequeue_buffer(stream);
		if (!aux)
			break;
		if (b)
			pw_stream_queue_buffer(stream, b);
		b = aux;
	}

	return b;
}

static enum video_colorspace
video_colorspace_from_spa_color_matrix(enum spa_video_color_matrix matrix)
{
	switch (matrix) {
	case SPA_VIDEO_COLOR_MATRIX_RGB:
		return VIDEO_CS_DEFAULT;
	case SPA_VIDEO_COLOR_MATRIX_BT601:
		return VIDEO_CS_601;
	case SPA_VIDEO_COLOR_MATRIX_BT709:
		return VIDEO_CS_709;
	default:
		return VIDEO_CS_DEFAULT;
	}
}

static enum video_range_type
video_color_range_from_spa_color_range(enum spa_video_color_range colorrange)
{
	switch (colorrange) {
	case SPA_VIDEO_COLOR_RANGE_0_255:
		return VIDEO_RANGE_FULL;
	case SPA_VIDEO_COLOR_RANGE_16_235:
		return VIDEO_RANGE_PARTIAL;
	default:
		return VIDEO_RANGE_DEFAULT;
	}
}

static bool prepare_obs_frame(obs_pipewire_stream *obs_pw_stream,
			      struct obs_source_frame *frame)
{
	struct format_data format_data;

	frame->width = obs_pw_stream->format.info.raw.size.width;
	frame->height = obs_pw_stream->format.info.raw.size.height;

	video_format_get_parameters(
		video_colorspace_from_spa_color_matrix(
			obs_pw_stream->format.info.raw.color_matrix),
		video_color_range_from_spa_color_range(
			obs_pw_stream->format.info.raw.color_range),
		frame->color_matrix, frame->color_range_min,
		frame->color_range_max);

	if (!lookup_format_info_from_spa_format(
		    obs_pw_stream->format.info.raw.format, &format_data) ||
	    format_data.video_format == VIDEO_FORMAT_NONE)
		return false;

	frame->format = format_data.video_format;
	frame->linesize[0] = SPA_ROUND_UP_N(frame->width * format_data.bpp, 4);
	return true;
}

static void process_video_async(obs_pipewire_stream *obs_pw_stream)
{
	struct spa_buffer *buffer;
	struct pw_buffer *b;
	bool has_buffer;

	b = find_latest_buffer(obs_pw_stream->stream);
	if (!b) {
		blog(LOG_DEBUG, "[pipewire] Out of buffers!");
		return;
	}

	buffer = b->buffer;
	has_buffer = buffer->datas[0].chunk->size != 0;

	if (!has_buffer)
		goto done;

	blog(LOG_DEBUG, "[pipewire] Buffer has memory texture");

	struct obs_source_frame out = {0};
	if (!prepare_obs_frame(obs_pw_stream, &out)) {
		blog(LOG_ERROR, "[pipewire] Couldn't prepare frame");
		goto done;
	}

	for (uint32_t i = 0; i < buffer->n_datas && i < MAX_AV_PLANES; i++) {
		out.data[i] = buffer->datas[i].data;
		if (out.data[i] == NULL) {
			blog(LOG_ERROR, "[pipewire] Failed to access data");
			goto done;
		}
	}

	blog(LOG_DEBUG, "[pipewire] Camera frame info: Format: %s, Planes: %u",
	     get_video_format_name(out.format), buffer->n_datas);
	for (uint32_t i = 0; i < buffer->n_datas && i < MAX_AV_PLANES; i++) {
		blog(LOG_DEBUG, "[pipewire] Plane %u: Dataptr:%p, Linesize:%d",
		     i, out.data[i], out.linesize[i]);
	}

	obs_source_output_video(obs_pw_stream->obs_data.source, &out);

done:
	pw_stream_queue_buffer(obs_pw_stream->stream, b);
}

static void process_video_sync(obs_pipewire_stream *obs_pw_stream)
{
	obs_pipewire *obs_pw = obs_pw_stream->obs_pw;
	struct spa_meta_cursor *cursor;
	struct spa_meta_region *region;
	struct format_data format_data;
	struct spa_buffer *buffer;
	struct pw_buffer *b;
	bool swap_red_blue = false;
	bool has_buffer;

	b = find_latest_buffer(obs_pw_stream->stream);
	if (!b) {
		blog(LOG_DEBUG, "[pipewire] Out of buffers!");
		return;
	}

	buffer = b->buffer;
	has_buffer = buffer->datas[0].chunk->size != 0;

	obs_enter_graphics();

	if (!has_buffer)
		goto read_metadata;

	if (buffer->datas[0].type == SPA_DATA_DmaBuf) {
		uint32_t planes = buffer->n_datas;
		uint32_t offsets[planes];
		uint32_t strides[planes];
		uint64_t modifiers[planes];
		int fds[planes];
		bool use_modifiers;

		blog(LOG_DEBUG,
		     "[pipewire] DMA-BUF info: fd:%ld, stride:%d, offset:%u, size:%dx%d",
		     buffer->datas[0].fd, buffer->datas[0].chunk->stride,
		     buffer->datas[0].chunk->offset,
		     obs_pw_stream->format.info.raw.size.width,
		     obs_pw_stream->format.info.raw.size.height);

		if (!lookup_format_info_from_spa_format(
			    obs_pw_stream->format.info.raw.format,
			    &format_data) ||
		    format_data.gs_format == GS_UNKNOWN) {
			blog(LOG_ERROR,
			     "[pipewire] unsupported DMA buffer format: %d",
			     obs_pw_stream->format.info.raw.format);
			goto read_metadata;
		}

		for (uint32_t plane = 0; plane < planes; plane++) {
			fds[plane] = buffer->datas[plane].fd;
			offsets[plane] = buffer->datas[plane].chunk->offset;
			strides[plane] = buffer->datas[plane].chunk->stride;
			modifiers[plane] =
				obs_pw_stream->format.info.raw.modifier;
		}

		g_clear_pointer(&obs_pw_stream->texture, gs_texture_destroy);

		use_modifiers = obs_pw_stream->format.info.raw.modifier !=
				DRM_FORMAT_MOD_INVALID;
		obs_pw_stream->texture = gs_texture_create_from_dmabuf(
			obs_pw_stream->format.info.raw.size.width,
			obs_pw_stream->format.info.raw.size.height,
			format_data.drm_format, GS_BGRX, planes, fds, strides,
			offsets, use_modifiers ? modifiers : NULL);

		if (obs_pw_stream->texture == NULL) {
			remove_modifier_from_format(
				obs_pw_stream,
				obs_pw_stream->format.info.raw.format,
				obs_pw_stream->format.info.raw.modifier);
			pw_loop_signal_event(
				pw_thread_loop_get_loop(obs_pw->thread_loop),
				obs_pw_stream->reneg);
		}
	} else {
		blog(LOG_DEBUG, "[pipewire] Buffer has memory texture");

		if (!lookup_format_info_from_spa_format(
			    obs_pw_stream->format.info.raw.format,
			    &format_data) ||
		    format_data.gs_format == GS_UNKNOWN) {
			blog(LOG_ERROR,
			     "[pipewire] unsupported DMA buffer format: %d",
			     obs_pw_stream->format.info.raw.format);
			goto read_metadata;
		}

		g_clear_pointer(&obs_pw_stream->texture, gs_texture_destroy);
		obs_pw_stream->texture = gs_texture_create(
			obs_pw_stream->format.info.raw.size.width,
			obs_pw_stream->format.info.raw.size.height,
			format_data.gs_format, 1,
			(const uint8_t **)&buffer->datas[0].data, GS_DYNAMIC);
	}

	if (swap_red_blue)
		swap_texture_red_blue(obs_pw_stream->texture);

	/* Video Crop */
	region = spa_buffer_find_meta_data(buffer, SPA_META_VideoCrop,
					   sizeof(*region));
	if (region && spa_meta_region_is_valid(region)) {
		blog(LOG_DEBUG,
		     "[pipewire] Crop Region available (%dx%d+%d+%d)",
		     region->region.position.x, region->region.position.y,
		     region->region.size.width, region->region.size.height);

		obs_pw_stream->crop.x = region->region.position.x;
		obs_pw_stream->crop.y = region->region.position.y;
		obs_pw_stream->crop.width = region->region.size.width;
		obs_pw_stream->crop.height = region->region.size.height;
		obs_pw_stream->crop.valid = true;
	} else {
		obs_pw_stream->crop.valid = false;
	}

read_metadata:

	/* Cursor */
	cursor = spa_buffer_find_meta_data(buffer, SPA_META_Cursor,
					   sizeof(*cursor));
	obs_pw_stream->cursor.valid = cursor &&
				      spa_meta_cursor_is_valid(cursor);
	if (obs_pw_stream->cursor.visible && obs_pw_stream->cursor.valid) {
		struct spa_meta_bitmap *bitmap = NULL;

		if (cursor->bitmap_offset)
			bitmap = SPA_MEMBER(cursor, cursor->bitmap_offset,
					    struct spa_meta_bitmap);

		if (bitmap && bitmap->size.width > 0 &&
		    bitmap->size.height > 0 &&
		    lookup_format_info_from_spa_format(bitmap->format,
						       &format_data) &&
		    format_data.gs_format != GS_UNKNOWN) {
			const uint8_t *bitmap_data;

			bitmap_data =
				SPA_MEMBER(bitmap, bitmap->offset, uint8_t);
			obs_pw_stream->cursor.hotspot_x = cursor->hotspot.x;
			obs_pw_stream->cursor.hotspot_y = cursor->hotspot.y;
			obs_pw_stream->cursor.width = bitmap->size.width;
			obs_pw_stream->cursor.height = bitmap->size.height;

			g_clear_pointer(&obs_pw_stream->cursor.texture,
					gs_texture_destroy);
			obs_pw_stream->cursor.texture =
				gs_texture_create(obs_pw_stream->cursor.width,
						  obs_pw_stream->cursor.height,
						  format_data.gs_format, 1,
						  &bitmap_data, GS_DYNAMIC);

			if (swap_red_blue)
				swap_texture_red_blue(
					obs_pw_stream->cursor.texture);
		}

		obs_pw_stream->cursor.x = cursor->position.x;
		obs_pw_stream->cursor.y = cursor->position.y;
	}

	pw_stream_queue_buffer(obs_pw_stream->stream, b);

	obs_leave_graphics();
}

static void on_process_cb(void *user_data)
{
	obs_pipewire_stream *obs_pw_stream = user_data;
	uint32_t output_flags;

	output_flags =
		obs_source_get_output_flags(obs_pw_stream->obs_data.source);

	if (output_flags & OBS_SOURCE_VIDEO) {
		if (output_flags & OBS_SOURCE_ASYNC)
			process_video_async(obs_pw_stream);
		else
			process_video_sync(obs_pw_stream);
	}
}

static void param_changed_source(obs_pipewire_stream *obs_pw_stream,
				 uint32_t id, const struct spa_pod *param)
{
	obs_pipewire *obs_pw = obs_pw_stream->obs_pw;
	struct spa_pod_builder pod_builder;
	const struct spa_pod *params[3];
	uint32_t output_flags;
	uint32_t buffer_types;
	uint8_t params_buffer[1024];
	int result;

	if (!param || id != SPA_PARAM_Format)
		return;

	result = spa_format_parse(param, &obs_pw_stream->format.media_type,
				  &obs_pw_stream->format.media_subtype);
	if (result < 0)
		return;

	if (obs_pw_stream->format.media_type != SPA_MEDIA_TYPE_video ||
	    obs_pw_stream->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
		return;

	spa_format_video_raw_parse(param, &obs_pw_stream->format.info.raw);

	output_flags =
		obs_source_get_output_flags(obs_pw_stream->obs_data.source);

	buffer_types = 1 << SPA_DATA_MemPtr;
	bool has_modifier =
		spa_pod_find_prop(param, NULL, SPA_FORMAT_VIDEO_modifier) !=
		NULL;
	if ((has_modifier ||
	     check_pw_version(&obs_pw->server_version, 0, 3, 24)) &&
	    (output_flags & OBS_SOURCE_ASYNC_VIDEO) != OBS_SOURCE_ASYNC_VIDEO)
		buffer_types |= 1 << SPA_DATA_DmaBuf;

	blog(LOG_INFO, "[pipewire] Negotiated format:");

	blog(LOG_INFO, "[pipewire]     Format: %d (%s)",
	     obs_pw_stream->format.info.raw.format,
	     spa_debug_type_find_name(spa_type_video_format,
				      obs_pw_stream->format.info.raw.format));

	if (has_modifier) {
		blog(LOG_INFO, "[pipewire]     Modifier: %" PRIu64,
		     obs_pw_stream->format.info.raw.modifier);
	}

	blog(LOG_INFO, "[pipewire]     Size: %dx%d",
	     obs_pw_stream->format.info.raw.size.width,
	     obs_pw_stream->format.info.raw.size.height);

	blog(LOG_INFO, "[pipewire]     Framerate: %d/%d",
	     obs_pw_stream->format.info.raw.framerate.num,
	     obs_pw_stream->format.info.raw.framerate.denom);

	/* Video crop */
	pod_builder =
		SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));
	params[0] = spa_pod_builder_add_object(
		&pod_builder, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
		SPA_PARAM_META_type, SPA_POD_Id(SPA_META_VideoCrop),
		SPA_PARAM_META_size,
		SPA_POD_Int(sizeof(struct spa_meta_region)));

	/* Cursor */
	params[1] = spa_pod_builder_add_object(
		&pod_builder, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
		SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Cursor),
		SPA_PARAM_META_size,
		SPA_POD_CHOICE_RANGE_Int(CURSOR_META_SIZE(64, 64),
					 CURSOR_META_SIZE(1, 1),
					 CURSOR_META_SIZE(1024, 1024)));

	/* Buffer options */
	params[2] = spa_pod_builder_add_object(
		&pod_builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		SPA_PARAM_BUFFERS_dataType, SPA_POD_Int(buffer_types));

	pw_stream_update_params(obs_pw_stream->stream, params, 3);

	obs_pw_stream->negotiated = true;
}

static void param_changed_output(obs_pipewire_stream *obs_pw_stream,
				 uint32_t id, const struct spa_pod *param)
{
	obs_pipewire *obs_pw = obs_pw_stream->obs_pw;
	struct spa_pod_builder pod_builder;
	const struct spa_pod *params[3];
	uint32_t output_flags;
	uint32_t buffer_types;
	uint32_t size;
	uint32_t stride;
	uint8_t params_buffer[1024];
	int result;

	if (!param || id != SPA_PARAM_Format)
		return;

	result = spa_format_parse(param, &obs_pw_stream->format.media_type,
				  &obs_pw_stream->format.media_subtype);
	if (result < 0)
		return;

	if (obs_pw_stream->format.media_type != SPA_MEDIA_TYPE_video ||
	    obs_pw_stream->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
		return;

	spa_format_video_raw_parse(param, &obs_pw_stream->format.info.raw);

	buffer_types = 1 << SPA_DATA_MemPtr;
	bool has_modifier =
		spa_pod_find_prop(param, NULL, SPA_FORMAT_VIDEO_modifier) !=
		NULL;

	struct format_data format_data;
	if (!lookup_format_info_from_spa_format(
		    obs_pw_stream->format.info.raw.format, &format_data)) {
		blog(LOG_ERROR, "[pipewire] Unsupported format: %u",
		     obs_pw_stream->format.info.raw.format);
		return;
	}

	struct video_scale_info vsi = {0};
	vsi.format = format_data.video_format;
	vsi.width = obs_pw_stream->format.info.raw.size.width;
	vsi.height = obs_pw_stream->format.info.raw.size.height;
	obs_output_set_video_conversion(obs_pw_stream->obs_data.output, &vsi);

	stride = SPA_ROUND_UP_N(
		format_data.bpp * obs_pw_stream->format.info.raw.size.width, 4);
	size = SPA_ROUND_UP_N(
		stride * obs_pw_stream->format.info.raw.size.width, 4);

	blog(LOG_INFO, "[pipewire] Negotiated format:");

	blog(LOG_INFO, "[pipewire]     Format: %d (%s)",
	     obs_pw_stream->format.info.raw.format,
	     spa_debug_type_find_name(spa_type_video_format,
				      obs_pw_stream->format.info.raw.format));

	blog(LOG_INFO, "[pipewire]     Size: %dx%d",
	     obs_pw_stream->format.info.raw.size.width,
	     obs_pw_stream->format.info.raw.size.height);

	blog(LOG_INFO, "[pipewire]     Framerate: %d/%d",
	     obs_pw_stream->format.info.raw.framerate.num,
	     obs_pw_stream->format.info.raw.framerate.denom);

	pod_builder =
		SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));

	/* Video crop */
	params[0] = spa_pod_builder_add_object(
		&pod_builder, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
		SPA_PARAM_META_type, SPA_POD_Id(SPA_META_VideoCrop),
		SPA_PARAM_META_size,
		SPA_POD_Int(sizeof(struct spa_meta_region)));

	/* Cursor */
	params[1] = spa_pod_builder_add_object(
		&pod_builder, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
		SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Cursor),
		SPA_PARAM_META_size,
		SPA_POD_CHOICE_RANGE_Int(CURSOR_META_SIZE(64, 64),
					 CURSOR_META_SIZE(1, 1),
					 CURSOR_META_SIZE(1024, 1024)));

	/* Buffer options */
	params[2] = spa_pod_builder_add_object(
		&pod_builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(4, 1, 32),
		SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
		SPA_PARAM_BUFFERS_size, SPA_POD_Int(size),
		SPA_PARAM_BUFFERS_stride, SPA_POD_Int(stride),
		SPA_PARAM_BUFFERS_align, SPA_POD_Int(16),
		SPA_PARAM_BUFFERS_dataType, SPA_POD_Int(1 << SPA_DATA_MemPtr));

	pw_stream_update_params(obs_pw_stream->stream, params, 3);

	obs_pw_stream->negotiated = true;
}

static void on_param_changed_cb(void *user_data, uint32_t id,
				const struct spa_pod *param)
{
	obs_pipewire_stream *obs_pw_stream = user_data;
	switch (obs_pw_stream->obs_data.type) {
	case OBS_PIPEWIRE_STREAM_TYPE_SOURCE:
		param_changed_source(obs_pw_stream, id, param);
		break;
	case OBS_PIPEWIRE_STREAM_TYPE_OUTPUT:
		param_changed_output(obs_pw_stream, id, param);
		break;
	}
}

static void on_state_changed_cb(void *user_data, enum pw_stream_state old,
				enum pw_stream_state state, const char *error)
{
	UNUSED_PARAMETER(old);

	obs_pipewire_stream *obs_pw_stream = user_data;

	blog(LOG_INFO, "[pipewire] Stream %p state: \"%s\" (error: %s)",
	     obs_pw_stream->stream, pw_stream_state_as_string(state),
	     error ? error : "none");
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.state_changed = on_state_changed_cb,
	.param_changed = on_param_changed_cb,
	.process = on_process_cb,
};

static void on_core_info_cb(void *user_data, const struct pw_core_info *info)
{
	obs_pipewire *obs_pw = user_data;

	update_pw_versions(obs_pw, info->version);
}

static void on_core_error_cb(void *user_data, uint32_t id, int seq, int res,
			     const char *message)
{
	obs_pipewire_data *obs_pw = user_data;
	UNUSED_PARAMETER(seq);

	obs_pipewire *obs_pw = user_data;

	blog(LOG_ERROR, "[pipewire] Error id:%u seq:%d res:%d (%s): %s", id,
	     seq, res, g_strerror(res), message);

	pw_thread_loop_signal(obs_pw->thread_loop, FALSE);
}

static void on_core_done_cb(void *user_data, uint32_t id, int seq)
{
	obs_pipewire *obs_pw = user_data;

	if (id == PW_ID_CORE && obs_pw->sync_id == seq)
		pw_thread_loop_signal(obs_pw->thread_loop, FALSE);
}

static const struct pw_core_events core_events = {
	PW_VERSION_CORE_EVENTS,
	.info = on_core_info_cb,
	.done = on_core_done_cb,
	.error = on_core_error_cb,
};

/* obs_source_info methods */

obs_pipewire *
obs_pipewire_create(int pipewire_fd,
		    const struct pw_registry_events *registry_events,
		    void *user_data)
{
	obs_pipewire *obs_pw;

	obs_pw = bzalloc(sizeof(obs_pipewire));
	obs_pw->pipewire_fd = pipewire_fd;
	obs_pw->thread_loop = pw_thread_loop_new("PipeWire thread loop", NULL);
	obs_pw->context = pw_context_new(
		pw_thread_loop_get_loop(obs_pw->thread_loop), NULL, 0);

	if (pw_thread_loop_start(obs_pw->thread_loop) < 0) {
		blog(LOG_WARNING, "Error starting threaded mainloop");
		bfree(obs_pw);
		return NULL;
	}

	pw_thread_loop_lock(obs_pw->thread_loop);

	/* Core */
	if (obs_pw->pipewire_fd == -1) {
		obs_pw->core = pw_context_connect(obs_pw->context, NULL, 0);
	} else {
		obs_pw->core = pw_context_connect_fd(obs_pw->context,
						     fcntl(obs_pw->pipewire_fd,
							   F_DUPFD_CLOEXEC, 5),
						     NULL, 0);
	}
	if (!obs_pw->core) {
		blog(LOG_WARNING, "Error creating PipeWire core: %m");
		pw_thread_loop_unlock(obs_pw->thread_loop);
		bfree(obs_pw);
		return NULL;
	}

	pw_core_add_listener(obs_pw->core, &obs_pw->core_listener, &core_events,
			     obs_pw);

	// Dispatch to receive the info core event
	obs_pw->sync_id =
		pw_core_sync(obs_pw->core, PW_ID_CORE, obs_pw->sync_id);
	pw_thread_loop_wait(obs_pw->thread_loop);

	/* Registry */
	if (registry_events) {
		obs_pw->registry = pw_core_get_registry(obs_pw->core,
							PW_VERSION_REGISTRY, 0);
		pw_registry_add_listener(obs_pw->registry,
					 &obs_pw->registry_listener,
					 registry_events, user_data);
		blog(LOG_INFO, "[pipewire] Created registry %p",
		     obs_pw->registry);
	}

	pw_thread_loop_unlock(obs_pw->thread_loop);

	obs_pw->streams = g_ptr_array_new();

	return obs_pw;
}

struct pw_registry *obs_pipewire_get_registry(obs_pipewire *obs_pw)
{
	return obs_pw->registry;
}

void obs_pipewire_roundtrip(obs_pipewire *obs_pw)
{
	pw_thread_loop_lock(obs_pw->thread_loop);

	obs_pw->sync_id =
		pw_core_sync(obs_pw->core, PW_ID_CORE, obs_pw->sync_id);
	pw_thread_loop_wait(obs_pw->thread_loop);

	pw_thread_loop_unlock(obs_pw->thread_loop);
}

void obs_pipewire_destroy(obs_pipewire *obs_pw)
{
	if (!obs_pw)
		return;

	while (obs_pw->streams->len > 0) {
		obs_pipewire_stream *obs_pw_stream =
			g_ptr_array_index(obs_pw->streams, 0);
		obs_pipewire_stream_destroy(obs_pw_stream);
	}
	g_clear_pointer(&obs_pw->streams, g_ptr_array_unref);
	teardown_pipewire(obs_pw);
	bfree(obs_pw);
}

obs_pipewire_stream *obs_pipewire_connect_stream(
	obs_pipewire *obs_pw, obs_pipewire_stream_data *data, int pipewire_node,
	const char *stream_name, struct pw_properties *stream_properties)
{
	struct spa_pod_builder pod_builder;
	const struct spa_pod **params = NULL;
	obs_pipewire_stream *obs_pw_stream;
	uint32_t n_params;
	uint8_t params_buffer[2048];
	uint32_t pw_stream_flags;

	obs_pw_stream = bzalloc(sizeof(obs_pipewire_stream));
	obs_pw_stream->obs_pw = obs_pw;
	switch (data->type) {
	case OBS_PIPEWIRE_STREAM_TYPE_SOURCE:
		obs_pw_stream->obs_data.type = OBS_PIPEWIRE_STREAM_TYPE_SOURCE;
		obs_pw_stream->obs_data.source = data->source;
		break;
	case OBS_PIPEWIRE_STREAM_TYPE_OUTPUT:
		obs_pw_stream->obs_data.type = OBS_PIPEWIRE_STREAM_TYPE_OUTPUT;
		obs_pw_stream->obs_data.output = data->output;
		break;
	}

	init_format_info(obs_pw_stream);

	pw_thread_loop_lock(obs_pw->thread_loop);

	/* Signal to renegotiate */
	obs_pw_stream->reneg =
		pw_loop_add_event(pw_thread_loop_get_loop(obs_pw->thread_loop),
				  renegotiate_format, obs_pw);
	blog(LOG_DEBUG, "[pipewire] registered event %p", obs_pw_stream->reneg);

	/* Stream */
	obs_pw_stream->stream =
		pw_stream_new(obs_pw->core, stream_name, stream_properties);
	pw_stream_add_listener(obs_pw_stream->stream,
			       &obs_pw_stream->stream_listener, &stream_events,
			       obs_pw_stream);
	blog(LOG_INFO, "[pipewire] Created stream %p", obs_pw_stream->stream);

	/* Stream parameters */
	pod_builder =
		SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));

	obs_get_video_info(&obs_pw_stream->video_info);

	if (!build_format_params(obs_pw_stream, &pod_builder, &params,
				 &n_params)) {
		pw_thread_loop_unlock(obs_pw->thread_loop);
		bfree(obs_pw_stream);
		return NULL;
	}

	pw_stream_flags = PW_STREAM_FLAG_AUTOCONNECT |
			  PW_STREAM_FLAG_MAP_BUFFERS;
	if (obs_pw_stream->obs_data.type == OBS_PIPEWIRE_STREAM_TYPE_OUTPUT) {
		pw_stream_flags |= PW_STREAM_FLAG_DRIVER;
	}

	pw_stream_connect(obs_pw_stream->stream, PW_DIRECTION_INPUT,
			  pipewire_node, pw_stream_flags, params, n_params);

	blog(LOG_INFO, "[pipewire] Playing stream %p", obs_pw_stream->stream);

	pw_thread_loop_unlock(obs_pw->thread_loop);
	bfree(params);

	g_ptr_array_add(obs_pw->streams, obs_pw_stream);

	return obs_pw_stream;
}

void obs_pipewire_stream_show(obs_pipewire_stream *obs_pw_stream)
{
	if (obs_pw_stream->stream)
		pw_stream_set_active(obs_pw_stream->stream, true);
}

void obs_pipewire_stream_hide(obs_pipewire_stream *obs_pw_stream)
{
	if (obs_pw_stream->stream)
		pw_stream_set_active(obs_pw_stream->stream, false);
}

uint32_t obs_pipewire_stream_get_width(obs_pipewire_stream *obs_pw_stream)
{
	if (!obs_pw_stream->negotiated)
		return 0;

	if (obs_pw_stream->crop.valid)
		return obs_pw_stream->crop.width;
	else
		return obs_pw_stream->format.info.raw.size.width;
}

uint32_t obs_pipewire_stream_get_height(obs_pipewire_stream *obs_pw_stream)
{
	if (!obs_pw_stream->negotiated)
		return 0;

	if (obs_pw_stream->crop.valid)
		return obs_pw_stream->crop.height;
	else
		return obs_pw_stream->format.info.raw.size.height;
}

void obs_pipewire_stream_video_render(obs_pipewire_stream *obs_pw_stream,
				      gs_effect_t *effect)
{
	gs_eparam_t *image;

	if (!obs_pw_stream->texture)
		return;

	image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, obs_pw_stream->texture);

	if (has_effective_crop(obs_pw_stream)) {
		gs_draw_sprite_subregion(obs_pw_stream->texture, 0,
					 obs_pw_stream->crop.x,
					 obs_pw_stream->crop.y,
					 obs_pw_stream->crop.width,
					 obs_pw_stream->crop.height);
	} else {
		gs_draw_sprite(obs_pw_stream->texture, 0, 0, 0);
	}

	if (obs_pw_stream->cursor.visible && obs_pw_stream->cursor.valid &&
	    obs_pw_stream->cursor.texture) {
		float cursor_x = obs_pw_stream->cursor.x -
				 obs_pw_stream->cursor.hotspot_x;
		float cursor_y = obs_pw_stream->cursor.y -
				 obs_pw_stream->cursor.hotspot_y;

		gs_matrix_push();
		gs_matrix_translate3f(cursor_x, cursor_y, 0.0f);

		gs_effect_set_texture(image, obs_pw_stream->cursor.texture);
		gs_draw_sprite(obs_pw_stream->texture, 0,
			       obs_pw_stream->cursor.width,
			       obs_pw_stream->cursor.height);

		gs_matrix_pop();
	}
}

void obs_pipewire_stream_export_frame(obs_pipewire_stream *obs_pw_stream,
				      struct video_data *frame)
{
	struct pw_buffer *buffer;
	struct video_frame frame_out = {0};
	struct format_data format_data;
	// check if we have a running pipewire stream
	if (pw_stream_get_state(obs_pw_stream->stream, NULL) !=
	    PW_STREAM_STATE_STREAMING) {
		blog(LOG_INFO, "No node connected");
		return;
	}

	blog(LOG_DEBUG, "exporting frame to pipewire");

	if ((buffer = pw_stream_dequeue_buffer(obs_pw_stream->stream)) ==
	    NULL) {
		blog(LOG_WARNING, "pipewire: out of buffers");
		return;
	}

	if (!lookup_format_info_from_spa_format(
		    obs_pw_stream->format.info.raw.format, &format_data)) {
		blog(LOG_WARNING, "pipewire: unsupported format");
		return;
	}

	struct spa_buffer *spa_buf = buffer->buffer;
	struct spa_data *d = spa_buf->datas;

	for (unsigned int i = 0; i < spa_buf->n_datas; i++) {
		if (d[i].data == NULL) {
			blog(LOG_WARNING, "pipewire: buffer not mapped");
			continue;
		}
		uint32_t stride = SPA_ROUND_UP_N(
			obs_pw_stream->format.info.raw.size.width *
				format_data.bpp,
			4);
		frame_out.data[i] = d[i].data;
		d[i].mapoffset = 0;
		d[i].maxsize =
			obs_pw_stream->format.info.raw.size.height * stride;
		d[i].flags = SPA_DATA_FLAG_READABLE;
		d[i].type = SPA_DATA_MemPtr;
		d[i].chunk->offset = 0;
		d[i].chunk->stride = stride;
		d[i].chunk->size =
			obs_pw_stream->format.info.raw.size.height * stride;
	}
	video_frame_copy(&frame_out, (struct video_frame *)frame,
			 format_data.video_format, 1);

	struct spa_meta_header *h;
	if ((h = spa_buffer_find_meta_data(spa_buf, SPA_META_Header,
					   sizeof(*h)))) {
		h->pts = frame->timestamp;
		h->flags = 0;
		h->seq = obs_pw_stream->seq++;
		h->dts_offset = 0;
	}

	blog(LOG_DEBUG, "********************");
	blog(LOG_DEBUG, "pipewire: fd %lu", d[0].fd);
	blog(LOG_DEBUG, "pipewire: dataptr %p", d[0].data);
	blog(LOG_DEBUG, "pipewire: size %d", d[0].maxsize);
	blog(LOG_DEBUG, "pipewire: stride %d", d[0].chunk->stride);
	blog(LOG_DEBUG, "pipewire: width %d",
	     obs_pw_stream->format.info.raw.size.width);
	blog(LOG_DEBUG, "pipewire: height %d",
	     obs_pw_stream->format.info.raw.size.height);
	blog(LOG_DEBUG, "********************");

	pw_stream_queue_buffer(obs_pw_stream->stream, buffer);
}

void obs_pipewire_stream_set_cursor_visible(obs_pipewire_stream *obs_pw_stream,
					    bool cursor_visible)
{
	obs_pw_stream->cursor.visible = cursor_visible;
}

void obs_pipewire_stream_destroy(obs_pipewire_stream *obs_pw_stream)
{
	uint32_t output_flags;

	if (!obs_pw_stream)
		return;

	if (obs_pw_stream->obs_data.type == OBS_PIPEWIRE_STREAM_TYPE_SOURCE) {
		output_flags = obs_source_get_output_flags(
			obs_pw_stream->obs_data.source);
		if (output_flags & OBS_SOURCE_ASYNC_VIDEO)
			obs_source_output_video(obs_pw_stream->obs_data.source,
						NULL);
	}

	g_ptr_array_remove(obs_pw_stream->obs_pw->streams, obs_pw_stream);

	obs_enter_graphics();
	g_clear_pointer(&obs_pw_stream->cursor.texture, gs_texture_destroy);
	g_clear_pointer(&obs_pw_stream->texture, gs_texture_destroy);
	obs_leave_graphics();

	if (obs_pw_stream->stream)
		pw_stream_disconnect(obs_pw_stream->stream);
	g_clear_pointer(&obs_pw_stream->stream, pw_stream_destroy);

	clear_format_info(obs_pw_stream);
	bfree(obs_pw_stream);
}
