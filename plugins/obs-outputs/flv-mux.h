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

#pragma once

#include <obs.h>

#define MILLISECOND_DEN 1000

enum audio_id_t {
	AUDIO_CODEC_NONE = 0,
	AUDIO_CODEC_AAC = 1,
	AUDIO_CODEC_FLAC = 2,
	AUDIO_CODEC_OPUS = 3,
};

enum video_id_t {
	CODEC_NONE = 0, // not valid in rtmp
	CODEC_H264 = 1, // legacy & Y2023 spec
	CODEC_AV1,      // Y2023 spec
	CODEC_HEVC,
};

static enum audio_id_t to_audio_type(const char *codec)
{
	if (strcmp(codec, "aac") == 0)
		return AUDIO_CODEC_AAC;
	if (strcmp(codec, "flac") == 0)
		return AUDIO_CODEC_FLAC;
	if (strcmp(codec, "opus") == 0)
		return AUDIO_CODEC_OPUS;
	return AUDIO_CODEC_NONE;
}

static enum video_id_t to_video_type(const char *codec)
{
	if (strcmp(codec, "h264") == 0)
		return CODEC_H264;
	if (strcmp(codec, "av1") == 0)
		return CODEC_AV1;
#ifdef ENABLE_HEVC
	if (strcmp(codec, "hevc") == 0)
		return CODEC_HEVC;
#endif
	return 0;
}

static int32_t get_ms_time(struct encoder_packet *packet, int64_t val)
{
	return (int32_t)(val * MILLISECOND_DEN / packet->timebase_den);
}

extern void write_file_info(FILE *file, int64_t duration_ms, int64_t size);

extern void flv_meta_data(obs_output_t *context, uint8_t **output, size_t *size, bool write_header);
extern void flv_packet_mux(struct encoder_packet *packet, int32_t dts_offset, uint8_t **output, size_t *size,
			   bool is_header);
// Y2023 spec
extern void flv_packet_start(struct encoder_packet *packet, enum video_id_t codec, uint8_t **output, size_t *size,
			     size_t idx);
extern void flv_packet_frames(struct encoder_packet *packet, enum video_id_t codec, int32_t dts_offset,
			      uint8_t **output, size_t *size, size_t idx);
extern void flv_packet_end(struct encoder_packet *packet, enum video_id_t codec, uint8_t **output, size_t *size,
			   size_t idx);
extern void flv_packet_metadata(enum video_id_t codec, uint8_t **output, size_t *size, int bits_per_raw_sample,
				uint8_t color_primaries, int color_trc, int color_space, int min_luminance,
				int max_luminance, size_t idx);
extern void flv_packet_audio_start(struct encoder_packet *packet, enum audio_id_t codec, uint8_t **output, size_t *size,
				   size_t idx);
extern void flv_packet_audio_frames(struct encoder_packet *packet, enum audio_id_t codec, int32_t dts_offset,
				    uint8_t **output, size_t *size, size_t idx);
