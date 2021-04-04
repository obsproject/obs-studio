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

#include <util/dstr.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

#include <fcntl.h>
#include <glad/glad.h>
#include <linux/dma-buf.h>
#include <spa/param/video/format-utils.h>
#include <spa/debug/format.h>
#include <spa/debug/types.h>
#include <spa/param/video/type-info.h>

#define REQUEST_PATH "/org/freedesktop/portal/desktop/request/%s/obs%u"
#define SESSION_PATH "/org/freedesktop/portal/desktop/session/%s/obs%u"

#define CURSOR_META_SIZE(width, height)                                    \
	(sizeof(struct spa_meta_cursor) + sizeof(struct spa_meta_bitmap) + \
	 width * height * 4)

#define fourcc_code(a, b, c, d)                                \
	((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | \
	 ((__u32)(d) << 24))

#define DRM_FORMAT_XRGB8888        \
	fourcc_code('X', 'R', '2', \
		    '4') /* [31:0] x:R:G:B 8:8:8:8 little endian */
#define DRM_FORMAT_XBGR8888        \
	fourcc_code('X', 'B', '2', \
		    '4') /* [31:0] x:B:G:R 8:8:8:8 little endian */
#define DRM_FORMAT_ARGB8888        \
	fourcc_code('A', 'R', '2', \
		    '4') /* [31:0] A:R:G:B 8:8:8:8 little endian */
#define DRM_FORMAT_ABGR8888        \
	fourcc_code('A', 'B', '2', \
		    '4') /* [31:0] A:B:G:R 8:8:8:8 little endian */

struct _obs_pipewire_data {
	GDBusConnection *connection;
	GDBusProxy *proxy;
	GCancellable *cancellable;

	char *sender_name;
	char *session_handle;

	uint32_t pipewire_node;
	int pipewire_fd;

	uint32_t available_cursor_modes;

	obs_source_t *source;
	obs_data_t *settings;

	gs_texture_t *texture;

	struct pw_thread_loop *thread_loop;
	struct pw_context *context;

	struct pw_core *core;
	struct spa_hook core_listener;

	struct pw_stream *stream;
	struct spa_hook stream_listener;
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

	enum obs_pw_capture_type capture_type;
	struct obs_video_info video_info;
	bool negotiated;
};

struct dbus_call_data {
	obs_pipewire_data *obs_pw;
	char *request_path;
	guint signal_id;
	gulong cancelled_id;
};

/* auxiliary methods */

static const char *capture_type_to_string(enum obs_pw_capture_type capture_type)
{
	switch (capture_type) {
	case DESKTOP_CAPTURE:
		return "desktop";
	case WINDOW_CAPTURE:
		return "window";
	}
	return "unknown";
}

static void new_request_path(obs_pipewire_data *data, char **out_path,
			     char **out_token)
{
	static uint32_t request_token_count = 0;

	request_token_count++;

	if (out_token) {
		struct dstr str;
		dstr_init(&str);
		dstr_printf(&str, "obs%u", request_token_count);
		*out_token = str.array;
	}

	if (out_path) {
		struct dstr str;
		dstr_init(&str);
		dstr_printf(&str, REQUEST_PATH, data->sender_name,
			    request_token_count);
		*out_path = str.array;
	}
}

static void new_session_path(obs_pipewire_data *data, char **out_path,
			     char **out_token)
{
	static uint32_t session_token_count = 0;

	session_token_count++;

	if (out_token) {
		struct dstr str;
		dstr_init(&str);
		dstr_printf(&str, "obs%u", session_token_count);
		*out_token = str.array;
	}

	if (out_path) {
		struct dstr str;
		dstr_init(&str);
		dstr_printf(&str, SESSION_PATH, data->sender_name,
			    session_token_count);
		*out_path = str.array;
	}
}

static void on_cancelled_cb(GCancellable *cancellable, void *data)
{
	UNUSED_PARAMETER(cancellable);

	struct dbus_call_data *call = data;

	blog(LOG_INFO, "[pipewire] screencast session cancelled");

	g_dbus_connection_call(
		call->obs_pw->connection, "org.freedesktop.portal.Desktop",
		call->request_path, "org.freedesktop.portal.Request", "Close",
		NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static struct dbus_call_data *subscribe_to_signal(obs_pipewire_data *obs_pw,
						  const char *path,
						  GDBusSignalCallback callback)
{
	struct dbus_call_data *call;

	call = bzalloc(sizeof(struct dbus_call_data));
	call->obs_pw = obs_pw;
	call->request_path = bstrdup(path);
	call->cancelled_id = g_signal_connect(obs_pw->cancellable, "cancelled",
					      G_CALLBACK(on_cancelled_cb),
					      call);
	call->signal_id = g_dbus_connection_signal_subscribe(
		obs_pw->connection, "org.freedesktop.portal.Desktop",
		"org.freedesktop.portal.Request", "Response",
		call->request_path, NULL, G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
		callback, call, NULL);

	return call;
}

static void dbus_call_data_free(struct dbus_call_data *call)
{
	if (!call)
		return;

	if (call->signal_id)
		g_dbus_connection_signal_unsubscribe(call->obs_pw->connection,
						     call->signal_id);

	if (call->cancelled_id > 0)
		g_signal_handler_disconnect(call->obs_pw->cancellable,
					    call->cancelled_id);

	g_clear_pointer(&call->request_path, bfree);
	bfree(call);
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
	if (obs_pw->session_handle) {
		g_dbus_connection_call(
			obs_pw->connection, "org.freedesktop.portal.Desktop",
			obs_pw->session_handle,
			"org.freedesktop.portal.Session", "Close", NULL, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

		g_clear_pointer(&obs_pw->session_handle, g_free);
	}

	g_clear_pointer(&obs_pw->sender_name, bfree);
	g_clear_pointer(&obs_pw->cursor.texture, gs_texture_destroy);
	g_clear_pointer(&obs_pw->texture, gs_texture_destroy);
	g_cancellable_cancel(obs_pw->cancellable);
	g_clear_object(&obs_pw->cancellable);
	g_clear_object(&obs_pw->connection);
	g_clear_object(&obs_pw->proxy);
}

static inline bool has_effective_crop(obs_pipewire_data *obs_pw)
{
	return obs_pw->crop.valid &&
	       (obs_pw->crop.x != 0 || obs_pw->crop.y != 0 ||
		obs_pw->crop.width < obs_pw->format.info.raw.size.width ||
		obs_pw->crop.height < obs_pw->format.info.raw.size.height);
}

static bool spa_pixel_format_to_drm_format(uint32_t spa_format,
					   uint32_t *out_format)
{
	switch (spa_format) {
	case SPA_VIDEO_FORMAT_RGBA:
		*out_format = DRM_FORMAT_ABGR8888;
		break;

	case SPA_VIDEO_FORMAT_RGBx:
		*out_format = DRM_FORMAT_XBGR8888;
		break;

	case SPA_VIDEO_FORMAT_BGRA:
		*out_format = DRM_FORMAT_ARGB8888;
		break;

	case SPA_VIDEO_FORMAT_BGRx:
		*out_format = DRM_FORMAT_XRGB8888;
		break;

	default:
		return false;
	}

	return true;
}

static bool spa_pixel_format_to_obs_format(uint32_t spa_format,
					   enum gs_color_format *out_format,
					   bool *swap_red_blue)
{
	switch (spa_format) {
	case SPA_VIDEO_FORMAT_RGBA:
		*out_format = GS_RGBA;
		*swap_red_blue = false;
		break;

	case SPA_VIDEO_FORMAT_RGBx:
		*out_format = GS_BGRX;
		*swap_red_blue = true;
		break;

	case SPA_VIDEO_FORMAT_BGRA:
		*out_format = GS_BGRA;
		*swap_red_blue = false;
		break;

	case SPA_VIDEO_FORMAT_BGRx:
		*out_format = GS_BGRX;
		*swap_red_blue = false;
		break;

	default:
		return false;
	}

	return true;
}

static void swap_texture_red_blue(gs_texture_t *texture)
{
	GLuint gl_texure = *(GLuint *)gs_texture_get_obj(texture);

	glBindTexture(GL_TEXTURE_2D, gl_texure);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
	glBindTexture(GL_TEXTURE_2D, 0);
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
		uint32_t offsets[1];
		uint32_t strides[1];
		uint64_t modifiers[1];
		int fds[1];

		blog(LOG_DEBUG,
		     "[pipewire] DMA-BUF info: fd:%ld, stride:%d, offset:%u, size:%dx%d",
		     buffer->datas[0].fd, buffer->datas[0].chunk->stride,
		     buffer->datas[0].chunk->offset,
		     obs_pw->format.info.raw.size.width,
		     obs_pw->format.info.raw.size.height);

		if (!spa_pixel_format_to_drm_format(
			    obs_pw->format.info.raw.format, &drm_format)) {
			blog(LOG_ERROR,
			     "[pipewire] unsupported DMA buffer format: %d",
			     obs_pw->format.info.raw.format);
			goto read_metadata;
		}

		fds[0] = buffer->datas[0].fd;
		offsets[0] = buffer->datas[0].chunk->offset;
		strides[0] = buffer->datas[0].chunk->stride;
		modifiers[0] = obs_pw->format.info.raw.modifier;

		g_clear_pointer(&obs_pw->texture, gs_texture_destroy);
		obs_pw->texture = gs_texture_create_from_dmabuf(
			obs_pw->format.info.raw.size.width,
			obs_pw->format.info.raw.size.height, drm_format,
			GS_BGRX, 1, fds, strides, offsets, modifiers);
	} else {
		blog(LOG_DEBUG, "[pipewire] Buffer has memory texture");
		enum gs_color_format obs_format;

		if (!spa_pixel_format_to_obs_format(
			    obs_pw->format.info.raw.format, &obs_format,
			    &swap_red_blue)) {
			blog(LOG_ERROR,
			     "[pipewire] unsupported DMA buffer format: %d",
			     obs_pw->format.info.raw.format);
			goto read_metadata;
		}

		g_clear_pointer(&obs_pw->texture, gs_texture_destroy);
		obs_pw->texture = gs_texture_create(
			obs_pw->format.info.raw.size.width,
			obs_pw->format.info.raw.size.height, obs_format, 1,
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
		enum gs_color_format format;

		if (cursor->bitmap_offset)
			bitmap = SPA_MEMBER(cursor, cursor->bitmap_offset,
					    struct spa_meta_bitmap);

		if (bitmap && bitmap->size.width > 0 &&
		    bitmap->size.height > 0 &&
		    spa_pixel_format_to_obs_format(bitmap->format, &format,
						   &swap_red_blue)) {
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
				format, 1, &bitmap_data, GS_DYNAMIC);

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

	blog(LOG_DEBUG, "[pipewire] Negotiated format:");

	blog(LOG_DEBUG, "[pipewire]     Format: %d (%s)",
	     obs_pw->format.info.raw.format,
	     spa_debug_type_find_name(spa_type_video_format,
				      obs_pw->format.info.raw.format));

	blog(LOG_DEBUG, "[pipewire]     Size: %dx%d",
	     obs_pw->format.info.raw.size.width,
	     obs_pw->format.info.raw.size.height);

	blog(LOG_DEBUG, "[pipewire]     Framerate: %d/%d",
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
		SPA_PARAM_BUFFERS_dataType,
		SPA_POD_Int((1 << SPA_DATA_MemPtr) | (1 << SPA_DATA_DmaBuf)));

	pw_stream_update_params(obs_pw->stream, params, 3);

	obs_pw->negotiated = true;
}

static void on_state_changed_cb(void *user_data, enum pw_stream_state old,
				enum pw_stream_state state, const char *error)
{
	UNUSED_PARAMETER(old);
	UNUSED_PARAMETER(error);

	obs_pipewire_data *obs_pw = user_data;

	blog(LOG_DEBUG, "[pipewire] stream %p state: \"%s\" (error: %s)",
	     obs_pw->stream, pw_stream_state_as_string(state),
	     error ? error : "none");
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.state_changed = on_state_changed_cb,
	.param_changed = on_param_changed_cb,
	.process = on_process_cb,
};

static void on_core_error_cb(void *user_data, uint32_t id, int seq, int res,
			     const char *message)
{
	UNUSED_PARAMETER(seq);

	obs_pipewire_data *obs_pw = user_data;

	blog(LOG_ERROR, "[pipewire] Error id:%u seq:%d res:%d (%s): %s", id,
	     seq, res, g_strerror(res), message);

	pw_thread_loop_signal(obs_pw->thread_loop, FALSE);
}

static void on_core_done_cb(void *user_data, uint32_t id, int seq)
{
	UNUSED_PARAMETER(seq);

	obs_pipewire_data *obs_pw = user_data;

	if (id == PW_ID_CORE)
		pw_thread_loop_signal(obs_pw->thread_loop, FALSE);
}

static const struct pw_core_events core_events = {
	PW_VERSION_CORE_EVENTS,
	.done = on_core_done_cb,
	.error = on_core_error_cb,
};

static void play_pipewire_stream(obs_pipewire_data *obs_pw)
{
	struct spa_pod_builder pod_builder;
	const struct spa_pod *params[1];
	uint8_t params_buffer[1024];
	struct obs_video_info ovi;

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

	/* Stream */
	obs_pw->stream = pw_stream_new(
		obs_pw->core, "OBS Studio",
		pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
				  PW_KEY_MEDIA_CATEGORY, "Capture",
				  PW_KEY_MEDIA_ROLE, "Screen", NULL));
	pw_stream_add_listener(obs_pw->stream, &obs_pw->stream_listener,
			       &stream_events, obs_pw);
	blog(LOG_INFO, "[pipewire] created stream %p", obs_pw->stream);

	/* Stream parameters */
	pod_builder =
		SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));

	obs_get_video_info(&ovi);
	params[0] = spa_pod_builder_add_object(
		&pod_builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
		SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
		SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
		SPA_FORMAT_VIDEO_format,
		SPA_POD_CHOICE_ENUM_Id(
			4, SPA_VIDEO_FORMAT_BGRA, SPA_VIDEO_FORMAT_RGBA,
			SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_RGBx),
		SPA_FORMAT_VIDEO_size,
		SPA_POD_CHOICE_RANGE_Rectangle(
			&SPA_RECTANGLE(320, 240), // Arbitrary
			&SPA_RECTANGLE(1, 1), &SPA_RECTANGLE(8192, 4320)),
		SPA_FORMAT_VIDEO_framerate,
		SPA_POD_CHOICE_RANGE_Fraction(
			&SPA_FRACTION(ovi.fps_num, ovi.fps_den),
			&SPA_FRACTION(0, 1), &SPA_FRACTION(360, 1)));
	obs_pw->video_info = ovi;

	pw_stream_connect(
		obs_pw->stream, PW_DIRECTION_INPUT, obs_pw->pipewire_node,
		PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS, params,
		1);

	blog(LOG_INFO, "[pipewire] playing stream…");

	pw_thread_loop_unlock(obs_pw->thread_loop);
}

/* ------------------------------------------------- */

static void on_pipewire_remote_opened_cb(GObject *source, GAsyncResult *res,
					 void *user_data)
{
	g_autoptr(GUnixFDList) fd_list = NULL;
	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) error = NULL;
	obs_pipewire_data *obs_pw = user_data;
	int fd_index;

	result = g_dbus_proxy_call_with_unix_fd_list_finish(
		G_DBUS_PROXY(source), &fd_list, res, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR,
			     "[pipewire] Error retrieving pipewire fd: %s",
			     error->message);
		return;
	}

	g_variant_get(result, "(h)", &fd_index, &error);

	obs_pw->pipewire_fd = g_unix_fd_list_get(fd_list, fd_index, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR,
			     "[pipewire] Error retrieving pipewire fd: %s",
			     error->message);
		return;
	}

	play_pipewire_stream(obs_pw);
}

static void open_pipewire_remote(obs_pipewire_data *obs_pw)
{
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

	g_dbus_proxy_call_with_unix_fd_list(
		obs_pw->proxy, "OpenPipeWireRemote",
		g_variant_new("(oa{sv})", obs_pw->session_handle, &builder),
		G_DBUS_CALL_FLAGS_NONE, -1, NULL, obs_pw->cancellable,
		on_pipewire_remote_opened_cb, obs_pw);
}

/* ------------------------------------------------- */

static void on_start_response_received_cb(GDBusConnection *connection,
					  const char *sender_name,
					  const char *object_path,
					  const char *interface_name,
					  const char *signal_name,
					  GVariant *parameters, void *user_data)
{
	UNUSED_PARAMETER(connection);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);

	g_autoptr(GVariant) stream_properties = NULL;
	g_autoptr(GVariant) streams = NULL;
	g_autoptr(GVariant) result = NULL;
	struct dbus_call_data *call = user_data;
	obs_pipewire_data *obs_pw = call->obs_pw;
	GVariantIter iter;
	uint32_t response;
	size_t n_streams;

	g_clear_pointer(&call, dbus_call_data_free);

	g_variant_get(parameters, "(u@a{sv})", &response, &result);

	if (response != 0) {
		blog(LOG_WARNING,
		     "[pipewire] Failed to start screencast, denied or cancelled by user");
		return;
	}

	streams =
		g_variant_lookup_value(result, "streams", G_VARIANT_TYPE_ARRAY);

	g_variant_iter_init(&iter, streams);

	n_streams = g_variant_iter_n_children(&iter);
	if (n_streams != 1) {
		blog(LOG_WARNING,
		     "[pipewire] Received more than one stream when only one was expected. "
		     "This is probably a bug in the desktop portal implementation you are "
		     "using.");

		// The KDE Desktop portal implementation sometimes sends an invalid
		// response where more than one stream is attached, and only the
		// last one is the one we're looking for. This is the only known
		// buggy implementation, so let's at least try to make it work here.
		for (size_t i = 0; i < n_streams - 1; i++) {
			g_autoptr(GVariant) throwaway_properties = NULL;
			uint32_t throwaway_pipewire_node;

			g_variant_iter_loop(&iter, "(u@a{sv})",
					    &throwaway_pipewire_node,
					    &throwaway_properties);
		}
	}

	g_variant_iter_loop(&iter, "(u@a{sv})", &obs_pw->pipewire_node,
			    &stream_properties);

	blog(LOG_INFO, "[pipewire] %s selected, setting up screencast",
	     capture_type_to_string(obs_pw->capture_type));

	open_pipewire_remote(obs_pw);
}

static void on_started_cb(GObject *source, GAsyncResult *res, void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) error = NULL;

	result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR,
			     "[pipewire] Error selecting screencast source: %s",
			     error->message);
		return;
	}
}

static void start(obs_pipewire_data *obs_pw)
{
	GVariantBuilder builder;
	struct dbus_call_data *call;
	char *request_token;
	char *request_path;

	new_request_path(obs_pw, &request_path, &request_token);

	blog(LOG_INFO, "[pipewire] asking for %s…",
	     capture_type_to_string(obs_pw->capture_type));

	call = subscribe_to_signal(obs_pw, request_path,
				   on_start_response_received_cb);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&builder, "{sv}", "handle_token",
			      g_variant_new_string(request_token));

	g_dbus_proxy_call(obs_pw->proxy, "Start",
			  g_variant_new("(osa{sv})", obs_pw->session_handle, "",
					&builder),
			  G_DBUS_CALL_FLAGS_NONE, -1, obs_pw->cancellable,
			  on_started_cb, call);

	bfree(request_token);
	bfree(request_path);
}

/* ------------------------------------------------- */

static void on_select_source_response_received_cb(
	GDBusConnection *connection, const char *sender_name,
	const char *object_path, const char *interface_name,
	const char *signal_name, GVariant *parameters, void *user_data)
{
	UNUSED_PARAMETER(connection);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);

	g_autoptr(GVariant) ret = NULL;
	struct dbus_call_data *call = user_data;
	obs_pipewire_data *obs_pw = call->obs_pw;
	uint32_t response;

	blog(LOG_DEBUG, "[pipewire] Response to select source received");

	g_clear_pointer(&call, dbus_call_data_free);

	g_variant_get(parameters, "(u@a{sv})", &response, &ret);

	if (response != 0) {
		blog(LOG_WARNING,
		     "[pipewire] Failed to select source, denied or cancelled by user");
		return;
	}

	start(obs_pw);
}

static void on_source_selected_cb(GObject *source, GAsyncResult *res,
				  void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) error = NULL;

	result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR,
			     "[pipewire] Error selecting screencast source: %s",
			     error->message);
		return;
	}
}

static void select_source(obs_pipewire_data *obs_pw)
{
	struct dbus_call_data *call;
	GVariantBuilder builder;
	char *request_token;
	char *request_path;

	new_request_path(obs_pw, &request_path, &request_token);

	call = subscribe_to_signal(obs_pw, request_path,
				   on_select_source_response_received_cb);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&builder, "{sv}", "types",
			      g_variant_new_uint32(obs_pw->capture_type));
	g_variant_builder_add(&builder, "{sv}", "multiple",
			      g_variant_new_boolean(FALSE));
	g_variant_builder_add(&builder, "{sv}", "handle_token",
			      g_variant_new_string(request_token));

	if (obs_pw->available_cursor_modes & 4)
		g_variant_builder_add(&builder, "{sv}", "cursor_mode",
				      g_variant_new_uint32(4));
	else if ((obs_pw->available_cursor_modes & 2) && obs_pw->cursor.visible)
		g_variant_builder_add(&builder, "{sv}", "cursor_mode",
				      g_variant_new_uint32(2));
	else
		g_variant_builder_add(&builder, "{sv}", "cursor_mode",
				      g_variant_new_uint32(1));

	g_dbus_proxy_call(obs_pw->proxy, "SelectSources",
			  g_variant_new("(oa{sv})", obs_pw->session_handle,
					&builder),
			  G_DBUS_CALL_FLAGS_NONE, -1, obs_pw->cancellable,
			  on_source_selected_cb, call);

	bfree(request_token);
	bfree(request_path);
}

/* ------------------------------------------------- */

static void on_create_session_response_received_cb(
	GDBusConnection *connection, const char *sender_name,
	const char *object_path, const char *interface_name,
	const char *signal_name, GVariant *parameters, void *user_data)
{
	UNUSED_PARAMETER(connection);
	UNUSED_PARAMETER(sender_name);
	UNUSED_PARAMETER(object_path);
	UNUSED_PARAMETER(interface_name);
	UNUSED_PARAMETER(signal_name);

	g_autoptr(GVariant) result = NULL;
	struct dbus_call_data *call = user_data;
	obs_pipewire_data *obs_pw = call->obs_pw;
	uint32_t response;

	g_clear_pointer(&call, dbus_call_data_free);

	g_variant_get(parameters, "(u@a{sv})", &response, &result);

	if (response != 0) {
		blog(LOG_WARNING,
		     "[pipewire] Failed to create session, denied or cancelled by user");
		return;
	}

	blog(LOG_INFO, "[pipewire] screencast session created");

	g_variant_lookup(result, "session_handle", "s",
			 &obs_pw->session_handle);

	select_source(obs_pw);
}

static void on_session_created_cb(GObject *source, GAsyncResult *res,
				  void *user_data)
{
	UNUSED_PARAMETER(user_data);

	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) error = NULL;

	result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR,
			     "[pipewire] Error creating screencast session: %s",
			     error->message);
		return;
	}
}

static void create_session(obs_pipewire_data *obs_pw)
{
	struct dbus_call_data *call;
	GVariantBuilder builder;
	char *session_token;
	char *request_token;
	char *request_path;

	new_request_path(obs_pw, &request_path, &request_token);
	new_session_path(obs_pw, NULL, &session_token);

	call = subscribe_to_signal(obs_pw, request_path,
				   on_create_session_response_received_cb);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(&builder, "{sv}", "handle_token",
			      g_variant_new_string(request_token));
	g_variant_builder_add(&builder, "{sv}", "session_handle_token",
			      g_variant_new_string(session_token));

	g_dbus_proxy_call(obs_pw->proxy, "CreateSession",
			  g_variant_new("(a{sv})", &builder),
			  G_DBUS_CALL_FLAGS_NONE, -1, obs_pw->cancellable,
			  on_session_created_cb, call);

	bfree(session_token);
	bfree(request_token);
	bfree(request_path);
}

/* ------------------------------------------------- */

static void update_available_cursor_modes(obs_pipewire_data *obs_pw)
{
	g_autoptr(GVariant) cached_cursor_modes = NULL;
	uint32_t available_cursor_modes;

	cached_cursor_modes = g_dbus_proxy_get_cached_property(
		obs_pw->proxy, "AvailableCursorModes");
	available_cursor_modes =
		cached_cursor_modes ? g_variant_get_uint32(cached_cursor_modes)
				    : 0;

	obs_pw->available_cursor_modes = available_cursor_modes;

	blog(LOG_INFO, "[pipewire] available cursor modes:");
	if (available_cursor_modes & 4)
		blog(LOG_INFO, "[pipewire]     - Metadata");
	if (available_cursor_modes & 2)
		blog(LOG_INFO, "[pipewire]     - Always visible");
	if (available_cursor_modes & 1)
		blog(LOG_INFO, "[pipewire]     - Hidden");
}

static void on_proxy_created_cb(GObject *source, GAsyncResult *res,
				void *user_data)
{
	UNUSED_PARAMETER(source);

	g_autoptr(GError) error = NULL;
	obs_pipewire_data *obs_pw = user_data;

	obs_pw->proxy = g_dbus_proxy_new_finish(res, &error);

	if (error) {
		if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			blog(LOG_ERROR, "[pipewire] Error creating proxy: %s",
			     error->message);
		return;
	}

	update_available_cursor_modes(obs_pw);
	create_session(obs_pw);
}

static void create_proxy(obs_pipewire_data *obs_pw)
{
	g_dbus_proxy_new(obs_pw->connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
			 "org.freedesktop.portal.Desktop",
			 "/org/freedesktop/portal/desktop",
			 "org.freedesktop.portal.ScreenCast",
			 obs_pw->cancellable, on_proxy_created_cb, obs_pw);
}

/* ------------------------------------------------- */

static gboolean init_obs_pipewire(obs_pipewire_data *obs_pw)
{
	g_autoptr(GError) error = NULL;
	char *aux;

	obs_pw->cancellable = g_cancellable_new();
	obs_pw->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (error) {
		g_error("Error getting session bus: %s", error->message);
		return FALSE;
	}

	obs_pw->sender_name = bstrdup(
		g_dbus_connection_get_unique_name(obs_pw->connection) + 1);

	/* Replace dots by underscores */
	while ((aux = strstr(obs_pw->sender_name, ".")) != NULL)
		*aux = '_';

	blog(LOG_INFO, "PipeWire initialized (sender name: %s)",
	     obs_pw->sender_name);

	create_proxy(obs_pw);

	return TRUE;
}

static bool reload_session_cb(obs_properties_t *properties,
			      obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(properties);
	UNUSED_PARAMETER(property);

	obs_pipewire_data *obs_pw = data;

	teardown_pipewire(obs_pw);
	destroy_session(obs_pw);

	init_obs_pipewire(obs_pw);

	return false;
}

/* obs_source_info methods */

void *obs_pipewire_create(enum obs_pw_capture_type capture_type,
			  obs_data_t *settings, obs_source_t *source)
{
	obs_pipewire_data *obs_pw = bzalloc(sizeof(obs_pipewire_data));

	obs_pw->source = source;
	obs_pw->settings = settings;
	obs_pw->capture_type = capture_type;
	obs_pw->cursor.visible = obs_data_get_bool(settings, "ShowCursor");

	if (!init_obs_pipewire(obs_pw))
		g_clear_pointer(&obs_pw, bfree);

	return obs_pw;
}

void obs_pipewire_destroy(obs_pipewire_data *obs_pw)
{
	if (!obs_pw)
		return;

	teardown_pipewire(obs_pw);
	destroy_session(obs_pw);

	bfree(obs_pw);
}

void obs_pipewire_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "ShowCursor", true);
}

obs_properties_t *obs_pipewire_get_properties(obs_pipewire_data *obs_pw,
					      const char *reload_string_id)
{
	obs_properties_t *properties;

	properties = obs_properties_create();
	obs_properties_add_button2(properties, "Reload",
				   obs_module_text(reload_string_id),
				   reload_session_cb, obs_pw);
	obs_properties_add_bool(properties, "ShowCursor",
				obs_module_text("ShowCursor"));

	return properties;
}

void obs_pipewire_update(obs_pipewire_data *obs_pw, obs_data_t *settings)
{
	obs_pw->cursor.visible = obs_data_get_bool(settings, "ShowCursor");
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
					 obs_pw->crop.y,
					 obs_pw->crop.x + obs_pw->crop.width,
					 obs_pw->crop.y + obs_pw->crop.height);
	} else {
		gs_draw_sprite(obs_pw->texture, 0, 0, 0);
	}

	if (obs_pw->cursor.visible && obs_pw->cursor.valid &&
	    obs_pw->cursor.texture) {
		gs_matrix_push();
		gs_matrix_translate3f((float)obs_pw->cursor.x,
				      (float)obs_pw->cursor.y, 0.0f);

		gs_effect_set_texture(image, obs_pw->cursor.texture);
		gs_draw_sprite(obs_pw->texture, 0, obs_pw->cursor.width,
			       obs_pw->cursor.height);

		gs_matrix_pop();
	}
}

enum obs_pw_capture_type
obs_pipewire_get_capture_type(obs_pipewire_data *obs_pw)
{
	return obs_pw->capture_type;
}
