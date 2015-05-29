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
#include <obs-avc.h>
#include <util/dstr.h>
#include <util/pipe.h>
#include "ffmpeg-mux/ffmpeg-mux.h"

#define do_log(level, format, ...) \
	blog(level, "[ffmpeg muxer: '%s'] " format, \
			obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)

struct ffmpeg_muxer {
	obs_output_t      *output;
	os_process_pipe_t *pipe;
	struct dstr       path;
	bool              sent_headers;
	bool              active;
	bool              capturing;
};

static const char *ffmpeg_mux_getname(void)
{
	return obs_module_text("FFmpegMuxer");
}

static void ffmpeg_mux_destroy(void *data)
{
	struct ffmpeg_muxer *stream = data;
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
			"h264",
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
			(int)audio_output_get_sample_rate(audio),
			(int)audio_output_get_channels(audio));

	dstr_free(&name);
}

static void build_command_line(struct ffmpeg_muxer *stream, struct dstr *cmd)
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
}

static bool ffmpeg_mux_start(void *data)
{
	struct ffmpeg_muxer *stream = data;
	obs_data_t *settings;
	struct dstr cmd;
	const char *path;

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;
	if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	settings = obs_output_get_settings(stream->output);
	path = obs_data_get_string(settings, "path");
	dstr_copy(&stream->path, path);
	dstr_replace(&stream->path, "\"", "\"\"");
	obs_data_release(settings);

	build_command_line(stream, &cmd);
	stream->pipe = os_process_pipe_create(cmd.array, "w");
	dstr_free(&cmd);

	if (!stream->pipe) {
		warn("Failed to create process pipe");
		return false;
	}

	/* write headers and start capture */
	stream->active = true;
	stream->capturing = true;
	obs_output_begin_data_capture(stream->output, 0);

	info("Writing file '%s'...", stream->path.array);
	return true;
}

static int deactivate(struct ffmpeg_muxer *stream)
{
	int ret = -1;

	if (stream->active) {
		ret = os_process_pipe_destroy(stream->pipe);
		stream->pipe = NULL;

		stream->active = false;
		stream->sent_headers = false;

		info("Output of file '%s' stopped", stream->path.array);
	}

	return ret;
}

static void ffmpeg_mux_stop(void *data)
{
	struct ffmpeg_muxer *stream = data;

	if (stream->capturing) {
		obs_output_end_data_capture(stream->output);
		stream->capturing = false;
	}

	deactivate(stream);
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
	stream->capturing = false;
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

	if (!stream->active)
		return;

	if (!stream->sent_headers) {
		if (!send_headers(stream))
			return;

		stream->sent_headers = true;
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
	.get_properties = ffmpeg_mux_properties
};
