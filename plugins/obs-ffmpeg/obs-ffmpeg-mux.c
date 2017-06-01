/******************************************************************************
    Copyright (C) 2015 by Hugh Bailey <obs.jim@gmail.com>

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

#include <obs-module.h>
#include <obs-hotkey.h>
#include <obs-avc.h>
#include <util/dstr.h>
#include <util/pipe.h>
#include <util/darray.h>
#include <util/platform.h>
#include <util/circlebuf.h>
#include <util/threading.h>
#include "ffmpeg-mux/ffmpeg-mux.h"

#include <libavformat/avformat.h>

#define do_log(level, format, ...) \
	blog(level, "[ffmpeg muxer: '%s'] " format, \
			obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)

struct ffmpeg_muxer {
	obs_output_t      *output;
	os_process_pipe_t *pipe;
	int64_t           stop_ts;
	uint64_t          total_bytes;
	struct dstr       path;
	bool              sent_headers;
	volatile bool     active;
	volatile bool     stopping;
	volatile bool     capturing;

	/* replay buffer */
	struct circlebuf  packets;
	int64_t           cur_size;
	int64_t           cur_time;
	int64_t           max_size;
	int64_t           max_time;
	int64_t           save_ts;
	int               keyframes;
	obs_hotkey_id     hotkey;

	DARRAY(struct encoder_packet) mux_packets;
	pthread_t                     mux_thread;
	bool                          mux_thread_joinable;
	volatile bool                 muxing;
};

static const char *ffmpeg_mux_getname(void *type)
{
	UNUSED_PARAMETER(type);
	return obs_module_text("FFmpegMuxer");
}

static inline void replay_buffer_clear(struct ffmpeg_muxer *stream)
{
	while (stream->packets.size > 0) {
		struct encoder_packet pkt;
		circlebuf_pop_front(&stream->packets, &pkt, sizeof(pkt));
		obs_encoder_packet_release(&pkt);
	}

	circlebuf_free(&stream->packets);
	stream->cur_size = 0;
	stream->cur_time = 0;
	stream->max_size = 0;
	stream->max_time = 0;
	stream->save_ts = 0;
	stream->keyframes = 0;
}

static void ffmpeg_mux_destroy(void *data)
{
	struct ffmpeg_muxer *stream = data;

	replay_buffer_clear(stream);
	if (stream->mux_thread_joinable)
		pthread_join(stream->mux_thread, NULL);
	da_free(stream->mux_packets);

	os_process_pipe_destroy(stream->pipe);
	dstr_free(&stream->path);
	bfree(stream);
}

static void *ffmpeg_mux_create(obs_data_t *settings, obs_output_t *output)
{
	struct ffmpeg_muxer *stream = bzalloc(sizeof(*stream));
	stream->output = output;

	UNUSED_PARAMETER(settings);
	return stream;
}

#ifdef _WIN32
#ifdef _WIN64
#define FFMPEG_MUX "ffmpeg-mux64.exe"
#else
#define FFMPEG_MUX "ffmpeg-mux32.exe"
#endif
#else
#define FFMPEG_MUX "ffmpeg-mux"
#endif

static inline bool capturing(struct ffmpeg_muxer *stream)
{
	return os_atomic_load_bool(&stream->capturing);
}

static inline bool stopping(struct ffmpeg_muxer *stream)
{
	return os_atomic_load_bool(&stream->stopping);
}

static inline bool active(struct ffmpeg_muxer *stream)
{
	return os_atomic_load_bool(&stream->active);
}

/* TODO: allow codecs other than h264 whenever we start using them */

static void add_video_encoder_params(struct ffmpeg_muxer *stream,
		struct dstr *cmd, obs_encoder_t *vencoder)
{
	obs_data_t *settings = obs_encoder_get_settings(vencoder);
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	video_t *video = obs_get_video();
	const struct video_output_info *info = video_output_get_info(video);

	obs_data_release(settings);

	dstr_catf(cmd, "%s %d %d %d %d %d ",
			obs_encoder_get_codec(vencoder),
			bitrate,
			obs_output_get_width(stream->output),
			obs_output_get_height(stream->output),
			(int)info->fps_num,
			(int)info->fps_den);
}

static void add_audio_encoder_params(struct dstr *cmd, obs_encoder_t *aencoder)
{
	obs_data_t *settings = obs_encoder_get_settings(aencoder);
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	audio_t *audio = obs_get_audio();
	struct dstr name = {0};

	obs_data_release(settings);

	dstr_copy(&name, obs_encoder_get_name(aencoder));
	dstr_replace(&name, "\"", "\"\"");

	dstr_catf(cmd, "\"%s\" %d %d %d ",
			name.array,
			bitrate,
			(int)obs_encoder_get_sample_rate(aencoder),
			(int)audio_output_get_channels(audio));

	dstr_free(&name);
}

static void log_muxer_params(struct ffmpeg_muxer *stream, const char *settings)
{
	int ret;

	AVDictionary *dict = NULL;
	if ((ret = av_dict_parse_string(&dict, settings, "=", " ", 0))) {
		warn("Failed to parse muxer settings: %s\n%s",
				av_err2str(ret), settings);

		av_dict_free(&dict);
		return;
	}

	if (av_dict_count(dict) > 0) {
		struct dstr str = {0};

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry,
						AV_DICT_IGNORE_SUFFIX)))
			dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

		info("Using muxer settings:%s", str.array);
		dstr_free(&str);
	}

	av_dict_free(&dict);
}

static void add_muxer_params(struct dstr *cmd, struct ffmpeg_muxer *stream)
{
	obs_data_t *settings = obs_output_get_settings(stream->output);
	struct dstr mux = {0};

	dstr_copy(&mux, obs_data_get_string(settings, "muxer_settings"));

	log_muxer_params(stream, mux.array);

	dstr_replace(&mux, "\"", "\\\"");
	obs_data_release(settings);

	dstr_catf(cmd, "\"%s\" ", mux.array ? mux.array : "");

	dstr_free(&mux);
}

static void build_command_line(struct ffmpeg_muxer *stream, struct dstr *cmd,
		const char *path)
{
	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);
	obs_encoder_t *aencoders[MAX_AUDIO_MIXES];
	int num_tracks = 0;

	for (;;) {
		obs_encoder_t *aencoder = obs_output_get_audio_encoder(
				stream->output, num_tracks);
		if (!aencoder)
			break;

		aencoders[num_tracks] = aencoder;
		num_tracks++;
	}

	dstr_init_move_array(cmd, obs_module_file(FFMPEG_MUX));
	dstr_insert_ch(cmd, 0, '\"');
	dstr_cat(cmd, "\" \"");

	dstr_copy(&stream->path, path);
	dstr_replace(&stream->path, "\"", "\"\"");
	dstr_cat_dstr(cmd, &stream->path);

	dstr_catf(cmd, "\" %d %d ", vencoder ? 1 : 0, num_tracks);

	if (vencoder)
		add_video_encoder_params(stream, cmd, vencoder);

	if (num_tracks) {
		dstr_cat(cmd, "aac ");

		for (int i = 0; i < num_tracks; i++) {
			add_audio_encoder_params(cmd, aencoders[i]);
		}
	}

	add_muxer_params(cmd, stream);
}

static inline void start_pipe(struct ffmpeg_muxer *stream, const char *path)
{
	struct dstr cmd;
	build_command_line(stream, &cmd, path);
	stream->pipe = os_process_pipe_create(cmd.array, "w");
	dstr_free(&cmd);
}

static bool ffmpeg_mux_start(void *data)
{
	struct ffmpeg_muxer *stream = data;
	obs_data_t *settings;
	const char *path;

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;
	if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	settings = obs_output_get_settings(stream->output);
	path = obs_data_get_string(settings, "path");
	start_pipe(stream, path);
	obs_data_release(settings);

	if (!stream->pipe) {
		warn("Failed to create process pipe");
		return false;
	}

	/* write headers and start capture */
	os_atomic_set_bool(&stream->active, true);
	os_atomic_set_bool(&stream->capturing, true);
	stream->total_bytes = 0;
	obs_output_begin_data_capture(stream->output, 0);

	info("Writing file '%s'...", stream->path.array);
	return true;
}

static int deactivate(struct ffmpeg_muxer *stream)
{
	int ret = -1;

	if (active(stream)) {
		ret = os_process_pipe_destroy(stream->pipe);
		stream->pipe = NULL;

		os_atomic_set_bool(&stream->active, false);
		os_atomic_set_bool(&stream->sent_headers, false);

		info("Output of file '%s' stopped", stream->path.array);
	}

	if (stopping(stream))
		obs_output_end_data_capture(stream->output);

	os_atomic_set_bool(&stream->stopping, false);
	return ret;
}

static void ffmpeg_mux_stop(void *data, uint64_t ts)
{
	struct ffmpeg_muxer *stream = data;

	if (capturing(stream) || ts == 0) {
		stream->stop_ts = (int64_t)ts / 1000LL;
		os_atomic_set_bool(&stream->stopping, true);
		os_atomic_set_bool(&stream->capturing, false);
	}
}

static void signal_failure(struct ffmpeg_muxer *stream)
{
	int ret = deactivate(stream);
	int code;

	switch (ret) {
	case FFM_UNSUPPORTED:          code = OBS_OUTPUT_UNSUPPORTED; break;
	default:                       code = OBS_OUTPUT_ERROR;
	}

	obs_output_signal_stop(stream->output, code);
	os_atomic_set_bool(&stream->capturing, false);
}

static bool write_packet(struct ffmpeg_muxer *stream,
		struct encoder_packet *packet)
{
	bool is_video = packet->type == OBS_ENCODER_VIDEO;
	size_t ret;

	struct ffm_packet_info info = {
		.pts = packet->pts,
		.dts = packet->dts,
		.size = (uint32_t)packet->size,
		.index = (int)packet->track_idx,
		.type = is_video ? FFM_PACKET_VIDEO : FFM_PACKET_AUDIO,
		.keyframe = packet->keyframe
	};

	ret = os_process_pipe_write(stream->pipe, (const uint8_t*)&info,
			sizeof(info));
	if (ret != sizeof(info)) {
		warn("os_process_pipe_write for info structure failed");
		signal_failure(stream);
		return false;
	}

	ret = os_process_pipe_write(stream->pipe, packet->data, packet->size);
	if (ret != packet->size) {
		warn("os_process_pipe_write for packet data failed");
		signal_failure(stream);
		return false;
	}

	stream->total_bytes += packet->size;
	return true;
}

static bool send_audio_headers(struct ffmpeg_muxer *stream,
		obs_encoder_t *aencoder, size_t idx)
{
	struct encoder_packet packet = {
		.type         = OBS_ENCODER_AUDIO,
		.timebase_den = 1,
		.track_idx    = idx
	};

	obs_encoder_get_extra_data(aencoder, &packet.data, &packet.size);
	return write_packet(stream, &packet);
}

static bool send_video_headers(struct ffmpeg_muxer *stream)
{
	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);

	struct encoder_packet packet = {
		.type         = OBS_ENCODER_VIDEO,
		.timebase_den = 1
	};

	obs_encoder_get_extra_data(vencoder, &packet.data, &packet.size);
	return write_packet(stream, &packet);
}

static bool send_headers(struct ffmpeg_muxer *stream)
{
	obs_encoder_t *aencoder;
	size_t idx = 0;

	if (!send_video_headers(stream))
		return false;

	do {
		aencoder = obs_output_get_audio_encoder(stream->output, idx);
		if (aencoder) {
			if (!send_audio_headers(stream, aencoder, idx)) {
				return false;
			}
			idx++;
		}
	} while (aencoder);

	return true;
}

static void ffmpeg_mux_data(void *data, struct encoder_packet *packet)
{
	struct ffmpeg_muxer *stream = data;

	if (!active(stream))
		return;

	if (!stream->sent_headers) {
		if (!send_headers(stream))
			return;

		stream->sent_headers = true;
	}

	if (stopping(stream)) {
		if (packet->sys_dts_usec >= stream->stop_ts) {
			deactivate(stream);
			return;
		}
	}

	write_packet(stream, packet);
}

static obs_properties_t *ffmpeg_mux_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "path",
			obs_module_text("FilePath"),
			OBS_TEXT_DEFAULT);
	return props;
}

static uint64_t ffmpeg_mux_total_bytes(void *data)
{
	struct ffmpeg_muxer *stream = data;
	return stream->total_bytes;
}

struct obs_output_info ffmpeg_muxer = {
	.id             = "ffmpeg_muxer",
	.flags          = OBS_OUTPUT_AV |
	                  OBS_OUTPUT_ENCODED |
	                  OBS_OUTPUT_MULTI_TRACK,
	.get_name       = ffmpeg_mux_getname,
	.create         = ffmpeg_mux_create,
	.destroy        = ffmpeg_mux_destroy,
	.start          = ffmpeg_mux_start,
	.stop           = ffmpeg_mux_stop,
	.encoded_packet = ffmpeg_mux_data,
	.get_total_bytes= ffmpeg_mux_total_bytes,
	.get_properties = ffmpeg_mux_properties
};

/* ------------------------------------------------------------------------ */

static const char *replay_buffer_getname(void *type)
{
	UNUSED_PARAMETER(type);
	return obs_module_text("ReplayBuffer");
}

static void replay_buffer_hotkey(void *data, obs_hotkey_id id,
		obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	UNUSED_PARAMETER(pressed);

	struct ffmpeg_muxer *stream = data;
	if (os_atomic_load_bool(&stream->active))
		stream->save_ts = os_gettime_ns() / 1000LL;
}

static void save_replay_proc(void *data, calldata_t *cd)
{
	replay_buffer_hotkey(data, 0, NULL, true);
	UNUSED_PARAMETER(cd);
}

static void *replay_buffer_create(obs_data_t *settings, obs_output_t *output)
{
	UNUSED_PARAMETER(settings);
	struct ffmpeg_muxer *stream = bzalloc(sizeof(*stream));
	stream->output = output;

	stream->hotkey = obs_hotkey_register_output(output,
			"ReplayBuffer.Save",
			obs_module_text("ReplayBuffer.Save"),
			replay_buffer_hotkey, stream);

	proc_handler_t *ph = obs_output_get_proc_handler(output);
	proc_handler_add(ph, "void save()", save_replay_proc, stream);

	return stream;
}

static void replay_buffer_destroy(void *data)
{
	struct ffmpeg_muxer *stream = data;
	if (stream->hotkey)
		obs_hotkey_unregister(stream->hotkey);
	ffmpeg_mux_destroy(data);
}

static bool replay_buffer_start(void *data)
{
	struct ffmpeg_muxer *stream = data;

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;
	if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	obs_data_t *s = obs_output_get_settings(stream->output);
	stream->max_time = obs_data_get_int(s, "max_time_sec") * 1000000LL;
	stream->max_size = obs_data_get_int(s, "max_size_mb") * (1024 * 1024);
	obs_data_release(s);

	os_atomic_set_bool(&stream->active, true);
	os_atomic_set_bool(&stream->capturing, true);
	stream->total_bytes = 0;
	obs_output_begin_data_capture(stream->output, 0);

	return true;
}

static bool purge_front(struct ffmpeg_muxer *stream)
{
	struct encoder_packet pkt;
	bool keyframe;

	circlebuf_pop_front(&stream->packets, &pkt, sizeof(pkt));

	keyframe = pkt.type == OBS_ENCODER_VIDEO && pkt.keyframe;

	if (keyframe)
		stream->keyframes--;

	if (!stream->packets.size) {
		stream->cur_size = 0;
		stream->cur_time = 0;
	} else {
		struct encoder_packet first;
		circlebuf_peek_front(&stream->packets, &first, sizeof(first));
		stream->cur_time = first.dts_usec;
		stream->cur_size -= (int64_t)pkt.size;
	}

	obs_encoder_packet_release(&pkt);
	return keyframe;
}

static inline void purge(struct ffmpeg_muxer *stream)
{
	if (purge_front(stream)) {
		struct encoder_packet pkt;

		for (;;) {
			circlebuf_peek_front(&stream->packets, &pkt,
					sizeof(pkt));
			if (pkt.type == OBS_ENCODER_VIDEO && pkt.keyframe)
				return;

			purge_front(stream);
		}
	}
}

static inline void replay_buffer_purge(struct ffmpeg_muxer *stream,
		struct encoder_packet *pkt)
{
	if (stream->max_size) {
		if (!stream->packets.size || stream->keyframes <= 2)
			return;

		while ((stream->cur_size + (int64_t)pkt->size) >
				stream->max_size)
			purge(stream);
	}

	if (!stream->packets.size || stream->keyframes <= 2)
		return;

	while ((pkt->dts_usec - stream->cur_time) > stream->max_time)
		purge(stream);
}

static void insert_packet(struct darray *array, struct encoder_packet *packet,
		int64_t video_offset, int64_t *audio_offsets,
		int64_t video_dts_offset, int64_t *audio_dts_offsets)
{
	struct encoder_packet pkt;
	DARRAY(struct encoder_packet) packets;
	packets.da = *array;
	size_t idx;

	obs_encoder_packet_ref(&pkt, packet);

	if (pkt.type == OBS_ENCODER_VIDEO) {
		pkt.dts_usec -= video_offset;
		pkt.dts -= video_dts_offset;
		pkt.pts -= video_dts_offset;
	} else {
		pkt.dts_usec -= audio_offsets[pkt.track_idx];
		pkt.dts -= audio_dts_offsets[pkt.track_idx];
		pkt.pts -= audio_dts_offsets[pkt.track_idx];
	}

	for (idx = packets.num; idx > 0; idx--) {
		struct encoder_packet *p = packets.array + (idx - 1);
		if (p->dts_usec < pkt.dts_usec)
			break;
	}

	da_insert(packets, idx, &pkt);
	*array = packets.da;
}

static void *replay_buffer_mux_thread(void *data)
{
	struct ffmpeg_muxer *stream = data;

	start_pipe(stream, stream->path.array);

	if (!stream->pipe) {
		warn("Failed to create process pipe");
		goto error;
	}

	if (!send_headers(stream)) {
		warn("Could not write headers for file '%s'",
				stream->path.array);
		goto error;
	}

	for (size_t i = 0; i < stream->mux_packets.num; i++) {
		struct encoder_packet *pkt = &stream->mux_packets.array[i];
		write_packet(stream, pkt);
		obs_encoder_packet_release(pkt);
	}

	info("Wrote replay buffer to '%s'", stream->path.array);

error:
	os_process_pipe_destroy(stream->pipe);
	stream->pipe = NULL;
	da_free(stream->mux_packets);
	os_atomic_set_bool(&stream->muxing, false);
	return NULL;
}

static void replay_buffer_save(struct ffmpeg_muxer *stream)
{
	const size_t size = sizeof(struct encoder_packet);
	size_t num_packets = stream->packets.size / size;

	da_reserve(stream->mux_packets, num_packets);

	/* ---------------------------- */
	/* reorder packets */

	bool found_video = false;
	bool found_audio[MAX_AUDIO_MIXES] = {0};
	int64_t video_offset = 0;
	int64_t video_dts_offset = 0;
	int64_t audio_offsets[MAX_AUDIO_MIXES] = {0};
	int64_t audio_dts_offsets[MAX_AUDIO_MIXES] = {0};

	for (size_t i = 0; i < num_packets; i++) {
		struct encoder_packet *pkt;
		pkt = circlebuf_data(&stream->packets, i * size);

		if (pkt->type == OBS_ENCODER_VIDEO) {
			if (!found_video) {
				video_offset = pkt->dts_usec;
				video_dts_offset = pkt->dts;
				found_video = true;
			}
		} else {
			if (!found_audio[pkt->track_idx]) {
				found_audio[pkt->track_idx] = true;
				audio_offsets[pkt->track_idx] = pkt->dts_usec;
				audio_dts_offsets[pkt->track_idx] = pkt->dts;
			}
		}

		insert_packet(&stream->mux_packets.da, pkt,
				video_offset, audio_offsets,
				video_dts_offset, audio_dts_offsets);
	}

	/* ---------------------------- */
	/* generate filename */

	obs_data_t *settings = obs_output_get_settings(stream->output);
	const char *dir = obs_data_get_string(settings, "directory");
	const char *fmt = obs_data_get_string(settings, "format");
	const char *ext = obs_data_get_string(settings, "extension");
	bool space = obs_data_get_bool(settings, "allow_spaces");

	char *filename = os_generate_formatted_filename(ext, space, fmt);

	dstr_copy(&stream->path, dir);
	dstr_replace(&stream->path, "\\", "/");
	if (dstr_end(&stream->path) != '/')
		dstr_cat_ch(&stream->path, '/');
	dstr_cat(&stream->path, filename);

	bfree(filename);
	obs_data_release(settings);

	/* ---------------------------- */

	os_atomic_set_bool(&stream->muxing, true);
	stream->mux_thread_joinable = pthread_create(&stream->mux_thread, NULL,
			replay_buffer_mux_thread, stream) == 0;
}

static void deactivate_replay_buffer(struct ffmpeg_muxer *stream)
{
	if (stopping(stream))
		obs_output_end_data_capture(stream->output);

	os_atomic_set_bool(&stream->active, false);
	os_atomic_set_bool(&stream->sent_headers, false);
	os_atomic_set_bool(&stream->stopping, false);
	replay_buffer_clear(stream);
}

static void replay_buffer_data(void *data, struct encoder_packet *packet)
{
	struct ffmpeg_muxer *stream = data;
	struct encoder_packet pkt;

	if (!active(stream))
		return;

	if (stopping(stream)) {
		if (packet->sys_dts_usec >= stream->stop_ts) {
			deactivate_replay_buffer(stream);
			return;
		}
	}

	obs_encoder_packet_ref(&pkt, packet);
	replay_buffer_purge(stream, &pkt);

	if (!stream->packets.size)
		stream->cur_time = pkt.dts_usec;
	stream->cur_size += pkt.size;

	circlebuf_push_back(&stream->packets, packet, sizeof(*packet));

	if (packet->type == OBS_ENCODER_VIDEO && packet->keyframe)
		stream->keyframes++;

	if (stream->save_ts && packet->sys_dts_usec >= stream->save_ts) {
		if (os_atomic_load_bool(&stream->muxing))
			return;

		if (stream->mux_thread_joinable) {
			pthread_join(stream->mux_thread, NULL);
			stream->mux_thread_joinable = false;
		}

		stream->save_ts = 0;
		replay_buffer_save(stream);
	}
}

static void replay_buffer_defaults(obs_data_t *s)
{
	obs_data_set_default_int(s, "max_time_sec", 15);
	obs_data_set_default_int(s, "max_size_mb", 500);
	obs_data_set_default_string(s, "format", "%CCYY-%MM-%DD %hh-%mm-%ss");
	obs_data_set_default_string(s, "extension", "mp4");
	obs_data_set_default_bool(s, "allow_spaces", true);
}

struct obs_output_info replay_buffer = {
	.id             = "replay_buffer",
	.flags          = OBS_OUTPUT_AV |
	                  OBS_OUTPUT_ENCODED |
	                  OBS_OUTPUT_MULTI_TRACK,
	.get_name       = replay_buffer_getname,
	.create         = replay_buffer_create,
	.destroy        = replay_buffer_destroy,
	.start          = replay_buffer_start,
	.stop           = ffmpeg_mux_stop,
	.encoded_packet = replay_buffer_data,
	.get_total_bytes= ffmpeg_mux_total_bytes,
	.get_defaults   = replay_buffer_defaults
};
