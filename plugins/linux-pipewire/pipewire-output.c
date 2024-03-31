/* pipewire-output.c
 *
 * Copyright 2024 columbarius <co1umbarius@protonmail.com>
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

#include "pipewire-output.h"
#include "pipewire-intern-copy.h"
#include "formats.h"
#include "pipewire.h"

#include <gio/gio.h>

#include <string.h>
#include <util/darray.h>
#include <obs-output.h>
#include <media-io/video-frame.h>

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

#ifndef SPA_POD_PROP_FLAG_DONT_FIXATE
#define SPA_POD_PROP_FLAG_DONT_FIXATE (1 << 4)
#endif

struct _obs_pipewire_output_stream {
	obs_pipewire *obs_pw;

	obs_output_t *obs_output;

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

	struct format_info_base format_info;
};

/* auxiliary methods */

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
				    &SPA_RECTANGLE(640, 480), // Arbitrary
				    &SPA_RECTANGLE(1, 1),
				    &SPA_RECTANGLE(8192, 4320)),
			    SPA_FORMAT_VIDEO_framerate,
			    SPA_POD_CHOICE_RANGE_Fraction(
				    &SPA_FRACTION(ovi->fps_num, ovi->fps_den),
				    &SPA_FRACTION(0, 1), &SPA_FRACTION(360, 1)),
			    0);
	return spa_pod_builder_pop(b, &format_frame);
}

static bool build_format_params(obs_pipewire_output_stream *obs_pw_stream,
				struct spa_pod_builder *pod_builder,
				const struct spa_pod ***param_list,
				uint32_t *n_params)
{
	obs_pipewire *obs_pw = obs_pw_stream->obs_pw;
	uint32_t params_count = 0;

	const struct spa_pod **params;
	params = bzalloc(2 * obs_pw_stream->format_info.formats.num *
			 sizeof(struct spa_pod *));

	if (!params) {
		blog(LOG_ERROR,
		     "[pipewire] Failed to allocate memory for param pointers");
		return false;
	}

	if (!check_pw_version(&obs_pw->server_version, 0, 3, 33))
		goto build_shm;

	for (size_t i = 0; i < obs_pw_stream->format_info.formats.num; i++) {
		if (obs_pw_stream->format_info.formats.array[i].modifiers.num == 0) {
			continue;
		}
		params[params_count++] = build_format(
			pod_builder, &obs_pw_stream->video_info,
			obs_pw_stream->format_info.formats.array[i].spa_format,
			obs_pw_stream->format_info.formats.array[i].modifiers.array,
			obs_pw_stream->format_info.formats.array[i].modifiers.num);
	}

build_shm:
	for (size_t i = 0; i < obs_pw_stream->format_info.formats.num; i++) {
		params[params_count++] = build_format(
			pod_builder, &obs_pw_stream->video_info,
			obs_pw_stream->format_info.formats.array[i].spa_format, NULL,
			0);
	}
	*param_list = params;
	*n_params = params_count;
	return true;
}

static const uint32_t supported_formats_output[] = {
	SPA_VIDEO_FORMAT_RGBA,
};

#define N_SUPPORTED_FORMATS_OUTPUT \
	(sizeof(supported_formats_output) / sizeof(supported_formats_output[0]))

static void init_format_info(obs_pipewire_output_stream *obs_pw_stream)
{
	init_format_info_with_formats(&obs_pw_stream->format_info,
			supported_formats_output, N_SUPPORTED_FORMATS_OUTPUT, false);
}

/* ------------------------------------------------- */

static void on_param_changed_cb(void *user_data, uint32_t id,
				const struct spa_pod *param)
{
	obs_pipewire_output_stream *obs_pw_stream = user_data;
	obs_pipewire *obs_pw = obs_pw_stream->obs_pw;
	struct spa_pod_builder pod_builder;
	const struct spa_pod *params[2];
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

	struct obs_pw_video_format format_data;
	if (!obs_pw_video_format_from_spa_format(
		    obs_pw_stream->format.info.raw.format, &format_data)) {
		blog(LOG_ERROR, "[pipewire] Unsupported format: %u",
		     obs_pw_stream->format.info.raw.format);
		return;
	}

	struct video_scale_info vsi = {0};
	vsi.format = format_data.video_format;
	vsi.width = obs_pw_stream->format.info.raw.size.width;
	vsi.height = obs_pw_stream->format.info.raw.size.height;
	obs_output_set_video_conversion(obs_pw_stream->obs_output, &vsi);

	stride = SPA_ROUND_UP_N(
		format_data.bpp * obs_pw_stream->format.info.raw.size.width, 4);
	size = SPA_ROUND_UP_N(
		stride * obs_pw_stream->format.info.raw.size.height, 4);

	blog(LOG_INFO, "[pipewire] Negotiated format:");

	blog(LOG_INFO, "[pipewire]     Format: %d (%s), %d",
	     obs_pw_stream->format.info.raw.format,
	     spa_debug_type_find_name(spa_type_video_format,
				      obs_pw_stream->format.info.raw.format),
	     format_data.video_format);

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

	/* Buffer options */
	params[1] = spa_pod_builder_add_object(
		&pod_builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(6, 1, 32),
		SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
		SPA_PARAM_BUFFERS_size, SPA_POD_Int(size),
		SPA_PARAM_BUFFERS_stride, SPA_POD_Int(stride),
		SPA_PARAM_BUFFERS_align, SPA_POD_Int(16),
		SPA_PARAM_BUFFERS_dataType, SPA_POD_Int(1 << SPA_DATA_MemPtr));

	pw_stream_update_params(obs_pw_stream->stream, params, 2);

	obs_pw_stream->negotiated = true;
}

static void on_state_changed_cb(void *user_data, enum pw_stream_state old,
				enum pw_stream_state state, const char *error)
{
	UNUSED_PARAMETER(old);
	UNUSED_PARAMETER(error);

	obs_pipewire_output_stream *obs_pw_stream = user_data;

	blog(LOG_INFO, "[pipewire] Stream %p state: \"%s\" (error: %s)",
	     obs_pw_stream->stream, pw_stream_state_as_string(state),
	     error ? error : "none");
	switch (state) {
	case PW_STREAM_STATE_STREAMING:
		obs_output_begin_data_capture(obs_pw_stream->obs_output, 0);
		break;
	default:
		if (old = PW_STREAM_STATE_STREAMING) {
			obs_output_end_data_capture(obs_pw_stream->obs_output);
		}
	}
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.state_changed = on_state_changed_cb,
	.param_changed = on_param_changed_cb,
};

obs_pipewire_output_stream *obs_pipewire_connect_output_stream(
	obs_pipewire *obs_pw, obs_output_t *output,
	const char *stream_name, struct pw_properties *stream_properties)
{
	struct spa_pod_builder pod_builder;
	const struct spa_pod **params = NULL;
	obs_pipewire_output_stream *obs_pw_stream;
	uint32_t n_params;
	uint8_t params_buffer[2048];
	uint32_t pw_stream_flags;

	obs_pw_stream = bzalloc(sizeof(obs_pipewire_output_stream));
	obs_pw_stream->obs_pw = obs_pw;
	obs_pw_stream->obs_output = output;

	init_format_info(obs_pw_stream);

	pw_thread_loop_lock(obs_pw->thread_loop);

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
			  PW_STREAM_FLAG_MAP_BUFFERS |
			  PW_STREAM_FLAG_DRIVER;

	pw_stream_connect(obs_pw_stream->stream, PW_DIRECTION_OUTPUT,
			  PW_ID_ANY, pw_stream_flags, params, n_params);

	blog(LOG_INFO, "[pipewire] Playing stream %p", obs_pw_stream->stream);

	pw_thread_loop_unlock(obs_pw->thread_loop);
	bfree(params);

	return obs_pw_stream;
}

void obs_pipewire_output_stream_show(obs_pipewire_output_stream *obs_pw_stream)
{
	if (obs_pw_stream->stream)
		pw_stream_set_active(obs_pw_stream->stream, true);
}

void obs_pipewire_output_stream_hide(obs_pipewire_output_stream *obs_pw_stream)
{
	if (obs_pw_stream->stream)
		pw_stream_set_active(obs_pw_stream->stream, false);
}

uint32_t obs_pipewire_output_stream_get_width(obs_pipewire_output_stream *obs_pw_stream)
{
	if (!obs_pw_stream->negotiated)
		return 0;

	if (obs_pw_stream->crop.valid)
		return obs_pw_stream->crop.width;
	else
		return obs_pw_stream->format.info.raw.size.width;
}

uint32_t obs_pipewire_output_stream_get_height(obs_pipewire_output_stream *obs_pw_stream)
{
	if (!obs_pw_stream->negotiated)
		return 0;

	if (obs_pw_stream->crop.valid)
		return obs_pw_stream->crop.height;
	else
		return obs_pw_stream->format.info.raw.size.height;
}

void obs_pipewire_output_stream_export_frame(obs_pipewire_output_stream *obs_pw_stream,
				      struct video_data *frame)
{
	struct pw_buffer *buffer;
	struct obs_pw_video_format format_data;
	// check if we have a running pipewire stream
	if (pw_stream_get_state(obs_pw_stream->stream, NULL) !=
	    PW_STREAM_STATE_STREAMING) {
		blog(LOG_DEBUG, "No node connected");
		return;
	}

	blog(LOG_DEBUG, "exporting frame to pipewire");

	if ((buffer = pw_stream_dequeue_buffer(obs_pw_stream->stream)) ==
	    NULL) {
		blog(LOG_WARNING, "pipewire: out of buffers");
		return;
	}

	if (!obs_pw_video_format_from_spa_format(
		    obs_pw_stream->format.info.raw.format, &format_data)) {
		blog(LOG_WARNING, "pipewire: unsupported format");
		return;
	}

	struct spa_buffer *spa_buf = buffer->buffer;
	struct spa_data *d = spa_buf->datas;

	blog(LOG_DEBUG, "********************");
	blog(LOG_DEBUG, "pipewire: width %d",
	     obs_pw_stream->format.info.raw.size.width);
	blog(LOG_DEBUG, "pipewire: height %d",
	     obs_pw_stream->format.info.raw.size.height);
	for (unsigned int i = 0; i < spa_buf->n_datas; i++) {
		if (d[i].data == NULL) {
			blog(LOG_WARNING, "pipewire: buffer not mapped");
			continue;
		}
		uint32_t stride = SPA_ROUND_UP_N(
			obs_pw_stream->format.info.raw.size.width *
				format_data.bpp,
			4);
		if (stride != frame->linesize[i]) {
			blog(LOG_WARNING, "pipewire: buffer stride missmatch %d != %d",
					stride, frame->linesize[i]);
		}
		memcpy(d[i].data, frame->data[i], frame->linesize[i]*obs_pw_stream->format.info.raw.size.height);
		d[i].chunk->offset = 0;
		d[i].chunk->stride = stride;
		d[i].chunk->size =
			obs_pw_stream->format.info.raw.size.height * stride;
		
		blog(LOG_DEBUG, "pipewire: id: %d, fd %lu", i, d[i].fd);
		blog(LOG_DEBUG, "pipewire: id: %d, dataptr %p", i, d[i].data);
		blog(LOG_DEBUG, "pipewire: id: %d, size %d", i, d[i].maxsize);
		blog(LOG_DEBUG, "pipewire: id: %d, chunk stride %d", i, d[i].chunk->stride);
		blog(LOG_DEBUG, "pipewire: id: %d, chunk size %d", i, d[i].chunk->size);
	}

	struct spa_meta_header *h;
	if ((h = spa_buffer_find_meta_data(spa_buf, SPA_META_Header,
					   sizeof(*h)))) {
		h->pts = frame->timestamp;
		h->flags = 0;
		h->seq = obs_pw_stream->seq++;
		h->dts_offset = 0;
	}

	blog(LOG_DEBUG, "********************");

	pw_stream_queue_buffer(obs_pw_stream->stream, buffer);
}

void obs_pipewire_output_stream_destroy(obs_pipewire_output_stream *obs_pw_stream)
{
	uint32_t output_flags;

	if (!obs_pw_stream)
		return;

	obs_enter_graphics();
	g_clear_pointer(&obs_pw_stream->cursor.texture, gs_texture_destroy);
	g_clear_pointer(&obs_pw_stream->texture, gs_texture_destroy);
	obs_leave_graphics();

	if (obs_pw_stream->stream)
		pw_stream_disconnect(obs_pw_stream->stream);
	g_clear_pointer(&obs_pw_stream->stream, pw_stream_destroy);

	clear_format_info(&obs_pw_stream->format_info);
	bfree(obs_pw_stream);
}
