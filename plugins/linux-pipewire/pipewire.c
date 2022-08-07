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

struct _obs_pipewire_data {
	uint32_t pipewire_node;
	int pipewire_fd;

	gs_texture_t *texture;

	struct pw_thread_loop *thread_loop;
	struct pw_context *context;

	struct pw_core *core;
	struct spa_hook core_listener;
	int server_version_sync;

	struct obs_pw_version server_version;

	struct pw_stream *stream;
	struct spa_hook stream_listener;
	struct spa_source *reneg;

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

static void update_pw_versions(obs_pipewire_data *obs_pw, const char *version)
{
	blog(LOG_INFO, "[pipewire] Server version: %s", version);
	blog(LOG_INFO, "[pipewire] Library version: %s",
	     pw_get_library_version());
	blog(LOG_INFO, "[pipewire] Header version: %s",
	     pw_get_headers_version());

	if (!parse_pw_version(&obs_pw->server_version, version))
		blog(LOG_WARNING, "[pipewire] failed to parse server version");
}

static void teardown_pipewire(obs_pipewire_data *obs_pw)
{
	if (obs_pw->thread_loop) {
		pw_thread_loop_wait(obs_pw->thread_loop);
		pw_thread_loop_stop(obs_pw->thread_loop);
	}

	if (obs_pw->stream)
		pw_stream_disconnect(obs_pw->stream);
	g_clear_pointer(&obs_pw->stream, pw_stream_destroy);
	g_clear_pointer(&obs_pw->context, pw_context_destroy);
	g_clear_pointer(&obs_pw->thread_loop, pw_thread_loop_destroy);

	if (obs_pw->pipewire_fd > 0) {
		close(obs_pw->pipewire_fd);
		obs_pw->pipewire_fd = 0;
	}

	obs_pw->negotiated = false;
}

static void destroy_session(obs_pipewire_data *obs_pw)
{
	obs_enter_graphics();
	g_clear_pointer(&obs_pw->cursor.texture, gs_texture_destroy);
	g_clear_pointer(&obs_pw->texture, gs_texture_destroy);
	obs_leave_graphics();
}

static inline bool has_effective_crop(obs_pipewire_data *obs_pw)
{
	return obs_pw->crop.valid &&
	       (obs_pw->crop.x != 0 || obs_pw->crop.y != 0 ||
		obs_pw->crop.width < obs_pw->format.info.raw.size.width ||
		obs_pw->crop.height < obs_pw->format.info.raw.size.height);
}

static const struct {
	uint32_t spa_format;
	uint32_t drm_format;
	enum gs_color_format gs_format;
	bool swap_red_blue;
	const char *pretty_name;
} supported_formats[] = {
	{
		SPA_VIDEO_FORMAT_BGRA,
		DRM_FORMAT_ARGB8888,
		GS_BGRA,
		false,
		"ARGB8888",
	},
	{
		SPA_VIDEO_FORMAT_RGBA,
		DRM_FORMAT_ABGR8888,
		GS_RGBA,
		false,
		"ABGR8888",
	},
	{
		SPA_VIDEO_FORMAT_BGRx,
		DRM_FORMAT_XRGB8888,
		GS_BGRX,
		false,
		"XRGB8888",
	},
	{
		SPA_VIDEO_FORMAT_RGBx,
		DRM_FORMAT_XBGR8888,
		GS_BGRX,
		true,
		"XBGR8888",
	},
};

#define N_SUPPORTED_FORMATS \
	(sizeof(supported_formats) / sizeof(supported_formats[0]))

static bool lookup_format_info_from_spa_format(
	uint32_t spa_format, uint32_t *out_drm_format,
	enum gs_color_format *out_gs_format, bool *out_swap_red_blue)
{
	for (size_t i = 0; i < N_SUPPORTED_FORMATS; i++) {
		if (supported_formats[i].spa_format != spa_format)
			continue;

		if (out_drm_format)
			*out_drm_format = supported_formats[i].drm_format;
		if (out_gs_format)
			*out_gs_format = supported_formats[i].gs_format;
		if (out_swap_red_blue)
			*out_swap_red_blue = supported_formats[i].swap_red_blue;
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

static bool build_format_params(obs_pipewire_data *obs_pw,
				struct spa_pod_builder *pod_builder,
				const struct spa_pod ***param_list,
				uint32_t *n_params)
{
	uint32_t params_count = 0;

	const struct spa_pod **params;
	params =
		bzalloc(2 * obs_pw->format_info.num * sizeof(struct spa_pod *));

	if (!params) {
		blog(LOG_ERROR,
		     "[pipewire] Failed to allocate memory for param pointers");
		return false;
	}

	if (!check_pw_version(&obs_pw->server_version, 0, 3, 33))
		goto build_shm;

	for (size_t i = 0; i < obs_pw->format_info.num; i++) {
		if (obs_pw->format_info.array[i].modifiers.num == 0) {
			continue;
		}
		params[params_count++] = build_format(
			pod_builder, &obs_pw->video_info,
			obs_pw->format_info.array[i].spa_format,
			obs_pw->format_info.array[i].modifiers.array,
			obs_pw->format_info.array[i].modifiers.num);
	}

build_shm:
	for (size_t i = 0; i < obs_pw->format_info.num; i++) {
		params[params_count++] = build_format(
			pod_builder, &obs_pw->video_info,
			obs_pw->format_info.array[i].spa_format, NULL, 0);
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

static void init_format_info(obs_pipewire_data *obs_pw)
{
	da_init(obs_pw->format_info);

	obs_enter_graphics();

	enum gs_dmabuf_flags dmabuf_flags;
	uint32_t *drm_formats = NULL;
	size_t n_drm_formats;

	bool capabilities_queried = gs_query_dmabuf_capabilities(
		&dmabuf_flags, &drm_formats, &n_drm_formats);

	for (size_t i = 0; i < N_SUPPORTED_FORMATS; i++) {
		struct format_info *info;

		if (!drm_format_available(supported_formats[i].drm_format,
					  drm_formats, n_drm_formats))
			continue;

		info = da_push_back_new(obs_pw->format_info);
		da_init(info->modifiers);
		info->spa_format = supported_formats[i].spa_format;
		info->drm_format = supported_formats[i].drm_format;

		if (!capabilities_queried)
			continue;

		size_t n_modifiers;
		uint64_t *modifiers = NULL;
		if (gs_query_dmabuf_modifiers_for_format(
			    supported_formats[i].drm_format, &modifiers,
			    &n_modifiers)) {
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

static void clear_format_info(obs_pipewire_data *obs_pw)
{
	for (size_t i = 0; i < obs_pw->format_info.num; i++) {
		da_free(obs_pw->format_info.array[i].modifiers);
	}
	da_free(obs_pw->format_info);
}

static void remove_modifier_from_format(obs_pipewire_data *obs_pw,
					uint32_t spa_format, uint64_t modifier)
{
	for (size_t i = 0; i < obs_pw->format_info.num; i++) {
		if (obs_pw->format_info.array[i].spa_format != spa_format)
			continue;

		if (!check_pw_version(&obs_pw->server_version, 0, 3, 40)) {
			da_erase_range(
				obs_pw->format_info.array[i].modifiers, 0,
				obs_pw->format_info.array[i].modifiers.num - 1);
			continue;
		}

		int idx = da_find(obs_pw->format_info.array[i].modifiers,
				  &modifier, 0);
		while (idx != -1) {
			da_erase(obs_pw->format_info.array[i].modifiers, idx);
			idx = da_find(obs_pw->format_info.array[i].modifiers,
				      &modifier, 0);
		}
	}
}

static void renegotiate_format(void *data, uint64_t expirations)
{
	UNUSED_PARAMETER(expirations);
	obs_pipewire_data *obs_pw = (obs_pipewire_data *)data;
	const struct spa_pod **params = NULL;

	blog(LOG_INFO, "[pipewire] Renegotiating stream");

	pw_thread_loop_lock(obs_pw->thread_loop);

	uint8_t params_buffer[2048];
	struct spa_pod_builder pod_builder =
		SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));
	uint32_t n_params;
	if (!build_format_params(obs_pw, &pod_builder, &params, &n_params)) {
		teardown_pipewire(obs_pw);
		pw_thread_loop_unlock(obs_pw->thread_loop);
		return;
	}

	pw_stream_update_params(obs_pw->stream, params, n_params);
	pw_thread_loop_unlock(obs_pw->thread_loop);
	bfree(params);
}

/* ------------------------------------------------- */

static void on_process_cb(void *user_data)
{
	obs_pipewire_data *obs_pw = user_data;
	struct spa_meta_cursor *cursor;
	uint32_t drm_format;
	struct spa_meta_region *region;
	struct spa_buffer *buffer;
	struct pw_buffer *b;
	bool swap_red_blue = false;
	bool has_buffer;

	/* Find the most recent buffer */
	b = NULL;
	while (true) {
		struct pw_buffer *aux =
			pw_stream_dequeue_buffer(obs_pw->stream);
		if (!aux)
			break;
		if (b)
			pw_stream_queue_buffer(obs_pw->stream, b);
		b = aux;
	}

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
		     obs_pw->format.info.raw.size.width,
		     obs_pw->format.info.raw.size.height);

		if (!lookup_format_info_from_spa_format(
			    obs_pw->format.info.raw.format, &drm_format, NULL,
			    NULL)) {
			blog(LOG_ERROR,
			     "[pipewire] unsupported DMA buffer format: %d",
			     obs_pw->format.info.raw.format);
			goto read_metadata;
		}

		for (uint32_t plane = 0; plane < planes; plane++) {
			fds[plane] = buffer->datas[plane].fd;
			offsets[plane] = buffer->datas[plane].chunk->offset;
			strides[plane] = buffer->datas[plane].chunk->stride;
			modifiers[plane] = obs_pw->format.info.raw.modifier;
		}

		g_clear_pointer(&obs_pw->texture, gs_texture_destroy);

		use_modifiers = obs_pw->format.info.raw.modifier !=
				DRM_FORMAT_MOD_INVALID;
		obs_pw->texture = gs_texture_create_from_dmabuf(
			obs_pw->format.info.raw.size.width,
			obs_pw->format.info.raw.size.height, drm_format,
			GS_BGRX, planes, fds, strides, offsets,
			use_modifiers ? modifiers : NULL);

		if (obs_pw->texture == NULL) {
			remove_modifier_from_format(
				obs_pw, obs_pw->format.info.raw.format,
				obs_pw->format.info.raw.modifier);
			pw_loop_signal_event(
				pw_thread_loop_get_loop(obs_pw->thread_loop),
				obs_pw->reneg);
		}
	} else {
		blog(LOG_DEBUG, "[pipewire] Buffer has memory texture");
		enum gs_color_format gs_format;

		if (!lookup_format_info_from_spa_format(
			    obs_pw->format.info.raw.format, NULL, &gs_format,
			    &swap_red_blue)) {
			blog(LOG_ERROR,
			     "[pipewire] unsupported DMA buffer format: %d",
			     obs_pw->format.info.raw.format);
			goto read_metadata;
		}

		g_clear_pointer(&obs_pw->texture, gs_texture_destroy);
		obs_pw->texture = gs_texture_create(
			obs_pw->format.info.raw.size.width,
			obs_pw->format.info.raw.size.height, gs_format, 1,
			(const uint8_t **)&buffer->datas[0].data, GS_DYNAMIC);
	}

	if (swap_red_blue)
		swap_texture_red_blue(obs_pw->texture);

	/* Video Crop */
	region = spa_buffer_find_meta_data(buffer, SPA_META_VideoCrop,
					   sizeof(*region));
	if (region && spa_meta_region_is_valid(region)) {
		blog(LOG_DEBUG,
		     "[pipewire] Crop Region available (%dx%d+%d+%d)",
		     region->region.position.x, region->region.position.y,
		     region->region.size.width, region->region.size.height);

		obs_pw->crop.x = region->region.position.x;
		obs_pw->crop.y = region->region.position.y;
		obs_pw->crop.width = region->region.size.width;
		obs_pw->crop.height = region->region.size.height;
		obs_pw->crop.valid = true;
	} else {
		obs_pw->crop.valid = false;
	}

read_metadata:

	/* Cursor */
	cursor = spa_buffer_find_meta_data(buffer, SPA_META_Cursor,
					   sizeof(*cursor));
	obs_pw->cursor.valid = cursor && spa_meta_cursor_is_valid(cursor);
	if (obs_pw->cursor.visible && obs_pw->cursor.valid) {
		struct spa_meta_bitmap *bitmap = NULL;
		enum gs_color_format gs_format;

		if (cursor->bitmap_offset)
			bitmap = SPA_MEMBER(cursor, cursor->bitmap_offset,
					    struct spa_meta_bitmap);

		if (bitmap && bitmap->size.width > 0 &&
		    bitmap->size.height > 0 &&
		    lookup_format_info_from_spa_format(
			    bitmap->format, NULL, &gs_format, &swap_red_blue)) {
			const uint8_t *bitmap_data;

			bitmap_data =
				SPA_MEMBER(bitmap, bitmap->offset, uint8_t);
			obs_pw->cursor.hotspot_x = cursor->hotspot.x;
			obs_pw->cursor.hotspot_y = cursor->hotspot.y;
			obs_pw->cursor.width = bitmap->size.width;
			obs_pw->cursor.height = bitmap->size.height;

			g_clear_pointer(&obs_pw->cursor.texture,
					gs_texture_destroy);
			obs_pw->cursor.texture = gs_texture_create(
				obs_pw->cursor.width, obs_pw->cursor.height,
				gs_format, 1, &bitmap_data, GS_DYNAMIC);

			if (swap_red_blue)
				swap_texture_red_blue(obs_pw->cursor.texture);
		}

		obs_pw->cursor.x = cursor->position.x;
		obs_pw->cursor.y = cursor->position.y;
	}

	pw_stream_queue_buffer(obs_pw->stream, b);

	obs_leave_graphics();
}

static void on_param_changed_cb(void *user_data, uint32_t id,
				const struct spa_pod *param)
{
	obs_pipewire_data *obs_pw = user_data;
	struct spa_pod_builder pod_builder;
	const struct spa_pod *params[3];
	uint32_t buffer_types;
	uint8_t params_buffer[1024];
	int result;

	if (!param || id != SPA_PARAM_Format)
		return;

	result = spa_format_parse(param, &obs_pw->format.media_type,
				  &obs_pw->format.media_subtype);
	if (result < 0)
		return;

	if (obs_pw->format.media_type != SPA_MEDIA_TYPE_video ||
	    obs_pw->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
		return;

	spa_format_video_raw_parse(param, &obs_pw->format.info.raw);

	buffer_types = 1 << SPA_DATA_MemPtr;
	bool has_modifier =
		spa_pod_find_prop(param, NULL, SPA_FORMAT_VIDEO_modifier) !=
		NULL;
	if (has_modifier || check_pw_version(&obs_pw->server_version, 0, 3, 24))
		buffer_types |= 1 << SPA_DATA_DmaBuf;

	blog(LOG_INFO, "[pipewire] Negotiated format:");

	blog(LOG_INFO, "[pipewire]     Format: %d (%s)",
	     obs_pw->format.info.raw.format,
	     spa_debug_type_find_name(spa_type_video_format,
				      obs_pw->format.info.raw.format));

	if (has_modifier) {
		blog(LOG_INFO, "[pipewire]     Modifier: %" PRIu64,
		     obs_pw->format.info.raw.modifier);
	}

	blog(LOG_INFO, "[pipewire]     Size: %dx%d",
	     obs_pw->format.info.raw.size.width,
	     obs_pw->format.info.raw.size.height);

	blog(LOG_INFO, "[pipewire]     Framerate: %d/%d",
	     obs_pw->format.info.raw.framerate.num,
	     obs_pw->format.info.raw.framerate.denom);

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

	pw_stream_update_params(obs_pw->stream, params, 3);

	obs_pw->negotiated = true;
}

static void on_state_changed_cb(void *user_data, enum pw_stream_state old,
				enum pw_stream_state state, const char *error)
{
	UNUSED_PARAMETER(old);

	obs_pipewire_data *obs_pw = user_data;

	blog(LOG_INFO, "[pipewire] Stream %p state: \"%s\" (error: %s)",
	     obs_pw->stream, pw_stream_state_as_string(state),
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
	obs_pipewire_data *obs_pw = user_data;

	update_pw_versions(obs_pw, info->version);
}

static void on_core_error_cb(void *user_data, uint32_t id, int seq, int res,
			     const char *message)
{
	obs_pipewire_data *obs_pw = user_data;

	blog(LOG_ERROR, "[pipewire] Error id:%u seq:%d res:%d (%s): %s", id,
	     seq, res, g_strerror(res), message);

	pw_thread_loop_signal(obs_pw->thread_loop, FALSE);
}

static void on_core_done_cb(void *user_data, uint32_t id, int seq)
{
	obs_pipewire_data *obs_pw = user_data;

	if (id == PW_ID_CORE && obs_pw->server_version_sync == seq)
		pw_thread_loop_signal(obs_pw->thread_loop, FALSE);
}

static const struct pw_core_events core_events = {
	PW_VERSION_CORE_EVENTS,
	.info = on_core_info_cb,
	.done = on_core_done_cb,
	.error = on_core_error_cb,
};

static void play_pipewire_stream(obs_pipewire_data *obs_pw)
{
	struct spa_pod_builder pod_builder;
	const struct spa_pod **params = NULL;
	uint32_t n_params;
	uint8_t params_buffer[2048];

	obs_pw->thread_loop = pw_thread_loop_new("PipeWire thread loop", NULL);
	obs_pw->context = pw_context_new(
		pw_thread_loop_get_loop(obs_pw->thread_loop), NULL, 0);

	if (pw_thread_loop_start(obs_pw->thread_loop) < 0) {
		blog(LOG_WARNING, "Error starting threaded mainloop");
		return;
	}

	pw_thread_loop_lock(obs_pw->thread_loop);

	/* Core */
	obs_pw->core = pw_context_connect_fd(
		obs_pw->context, fcntl(obs_pw->pipewire_fd, F_DUPFD_CLOEXEC, 5),
		NULL, 0);
	if (!obs_pw->core) {
		blog(LOG_WARNING, "Error creating PipeWire core: %m");
		pw_thread_loop_unlock(obs_pw->thread_loop);
		return;
	}

	pw_core_add_listener(obs_pw->core, &obs_pw->core_listener, &core_events,
			     obs_pw);

	/* Signal to renegotiate */
	obs_pw->reneg =
		pw_loop_add_event(pw_thread_loop_get_loop(obs_pw->thread_loop),
				  renegotiate_format, obs_pw);
	blog(LOG_DEBUG, "[pipewire] registered event %p", obs_pw->reneg);

	// Dispatch to receive the info core event
	obs_pw->server_version_sync = pw_core_sync(obs_pw->core, PW_ID_CORE,
						   obs_pw->server_version_sync);
	pw_thread_loop_wait(obs_pw->thread_loop);

	/* Stream */
	obs_pw->stream = pw_stream_new(
		obs_pw->core, "OBS Studio",
		pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
				  PW_KEY_MEDIA_CATEGORY, "Capture",
				  PW_KEY_MEDIA_ROLE, "Screen", NULL));
	pw_stream_add_listener(obs_pw->stream, &obs_pw->stream_listener,
			       &stream_events, obs_pw);
	blog(LOG_INFO, "[pipewire] Created stream %p", obs_pw->stream);

	/* Stream parameters */
	pod_builder =
		SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));

	obs_get_video_info(&obs_pw->video_info);

	if (!build_format_params(obs_pw, &pod_builder, &params, &n_params)) {
		pw_thread_loop_unlock(obs_pw->thread_loop);
		teardown_pipewire(obs_pw);
		return;
	}

	pw_stream_connect(
		obs_pw->stream, PW_DIRECTION_INPUT, obs_pw->pipewire_node,
		PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS, params,
		n_params);

	blog(LOG_INFO, "[pipewire] Playing stream %p", obs_pw->stream);

	pw_thread_loop_unlock(obs_pw->thread_loop);
	bfree(params);
}

/* obs_source_info methods */

void *obs_pipewire_create(int pipewire_fd, int pipewire_node)
{
	obs_pipewire_data *obs_pw = bzalloc(sizeof(obs_pipewire_data));

	obs_pw->pipewire_fd = pipewire_fd;
	obs_pw->pipewire_node = pipewire_node;

	init_format_info(obs_pw);
	play_pipewire_stream(obs_pw);

	return obs_pw;
}

void obs_pipewire_destroy(obs_pipewire_data *obs_pw)
{
	if (!obs_pw)
		return;

	teardown_pipewire(obs_pw);
	destroy_session(obs_pw);

	clear_format_info(obs_pw);

	bfree(obs_pw);
}

void obs_pipewire_show(obs_pipewire_data *obs_pw)
{
	if (obs_pw->stream)
		pw_stream_set_active(obs_pw->stream, true);
}

void obs_pipewire_hide(obs_pipewire_data *obs_pw)
{
	if (obs_pw->stream)
		pw_stream_set_active(obs_pw->stream, false);
}

uint32_t obs_pipewire_get_width(obs_pipewire_data *obs_pw)
{
	if (!obs_pw->negotiated)
		return 0;

	if (obs_pw->crop.valid)
		return obs_pw->crop.width;
	else
		return obs_pw->format.info.raw.size.width;
}

uint32_t obs_pipewire_get_height(obs_pipewire_data *obs_pw)
{
	if (!obs_pw->negotiated)
		return 0;

	if (obs_pw->crop.valid)
		return obs_pw->crop.height;
	else
		return obs_pw->format.info.raw.size.height;
}

void obs_pipewire_video_render(obs_pipewire_data *obs_pw, gs_effect_t *effect)
{
	gs_eparam_t *image;

	if (!obs_pw->texture)
		return;

	image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, obs_pw->texture);

	if (has_effective_crop(obs_pw)) {
		gs_draw_sprite_subregion(obs_pw->texture, 0, obs_pw->crop.x,
					 obs_pw->crop.y, obs_pw->crop.width,
					 obs_pw->crop.height);
	} else {
		gs_draw_sprite(obs_pw->texture, 0, 0, 0);
	}

	if (obs_pw->cursor.visible && obs_pw->cursor.valid &&
	    obs_pw->cursor.texture) {
		float cursor_x = obs_pw->cursor.x - obs_pw->cursor.hotspot_x;
		float cursor_y = obs_pw->cursor.y - obs_pw->cursor.hotspot_y;

		gs_matrix_push();
		gs_matrix_translate3f(cursor_x, cursor_y, 0.0f);

		gs_effect_set_texture(image, obs_pw->cursor.texture);
		gs_draw_sprite(obs_pw->texture, 0, obs_pw->cursor.width,
			       obs_pw->cursor.height);

		gs_matrix_pop();
	}
}

void obs_pipewire_set_cursor_visible(obs_pipewire_data *obs_pw,
				     bool cursor_visible)
{
	obs_pw->cursor.visible = cursor_visible;
}
