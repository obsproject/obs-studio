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

#include <obs.h>
#include <stdio.h>
#include <util/dstr.h>
#include <util/array-serializer.h>
#include "flv-mux.h"
#include "obs-output-ver.h"
#include "rtmp-helpers.h"

/* TODO: FIXME: this is currently hard-coded to h264 and aac!  ..not that we'll
 * use anything else for a long time. */

//#define DEBUG_TIMESTAMPS
//#define WRITE_FLV_HEADER

#define AUDIODATA_AAC 10.0

#define AUDIO_FRAMETYPE_OFFSET 4
#define VIDEO_FRAMETYPE_OFFSET 4

enum video_frametype_t {
	FT_KEY = 1 << VIDEO_FRAMETYPE_OFFSET,
	FT_INTER = 2 << VIDEO_FRAMETYPE_OFFSET,
};

// Y2023 spec
const uint8_t AUDIO_HEADER_EX = 9 << AUDIO_FRAMETYPE_OFFSET;
enum audio_packet_type_t {
	AUDIO_PACKETTYPE_SEQ_START = 0,
	AUDIO_PACKETTYPE_FRAMES = 1,
	AUDIO_PACKETTYPE_MULTICHANNEL_CONFIG = 4,
	AUDIO_PACKETTYPE_MULTITRACK = 5,
};

const uint8_t FRAME_HEADER_EX = 8 << VIDEO_FRAMETYPE_OFFSET;
enum packet_type_t {
	PACKETTYPE_SEQ_START = 0,
	PACKETTYPE_FRAMES = 1,
	PACKETTYPE_SEQ_END = 2,
	PACKETTYPE_FRAMESX = 3,
	PACKETTYPE_METADATA = 4,
	PACKETTYPE_MPEG2TS_SEQ_START = 5,
	PACKETTYPE_MULTITRACK = 6
};

enum multitrack_type_t {
	MULTITRACKTYPE_ONE_TRACK = 0x00,
	MULTITRACKTYPE_MANY_TRACKS = 0x10,
	MULTITRACKTYPE_MANY_TRACKS_MANY_CODECS = 0x20,
};

enum datatype_t {
	DATA_TYPE_NUMBER = 0,
	DATA_TYPE_STRING = 2,
	DATA_TYPE_OBJECT = 3,
	DATA_TYPE_OBJECT_END = 9,
};

static void s_wa4cc(struct serializer *s, enum audio_id_t id)
{
	switch (id) {
	case AUDIO_CODEC_NONE:
		assert(0 && "Tried to serialize AUDIO_CODEC_NONE");
		break;

	case AUDIO_CODEC_AAC:
		s_w8(s, 'm');
		s_w8(s, 'p');
		s_w8(s, '4');
		s_w8(s, 'a');
		break;

	case AUDIO_CODEC_OPUS:
		s_w8(s, 'O');
		s_w8(s, 'p');
		s_w8(s, 'u');
		s_w8(s, 's');
		break;

	case AUDIO_CODEC_FLAC:
		s_w8(s, 'f');
		s_w8(s, 'L');
		s_w8(s, 'a');
		s_w8(s, 'C');
		break;
	}
}

static void s_w4cc(struct serializer *s, enum video_id_t id)
{
	switch (id) {
	case CODEC_NONE:
		assert(0 && "Tried to serialize CODEC_NONE");
		break;

	case CODEC_AV1:
		s_w8(s, 'a');
		s_w8(s, 'v');
		s_w8(s, '0');
		s_w8(s, '1');
		break;
	case CODEC_HEVC:
#ifdef ENABLE_HEVC
		s_w8(s, 'h');
		s_w8(s, 'v');
		s_w8(s, 'c');
		s_w8(s, '1');
		break;
#else
		assert(0);
#endif
	case CODEC_H264:
		s_w8(s, 'a');
		s_w8(s, 'v');
		s_w8(s, 'c');
		s_w8(s, '1');
		break;
	}
}

static void s_wstring(struct serializer *s, const char *str)
{
	size_t len = strlen(str);
	s_wb16(s, (uint16_t)len);
	s_write(s, str, len);
}

static inline void s_wtimestamp(struct serializer *s, int32_t i32)
{
	s_wb24(s, (uint32_t)(i32 & 0xFFFFFF));
	s_w8(s, (uint32_t)(i32 >> 24) & 0x7F);
}

static inline double encoder_bitrate(obs_encoder_t *encoder)
{
	obs_data_t *settings = obs_encoder_get_settings(encoder);
	double bitrate = obs_data_get_double(settings, "bitrate");

	obs_data_release(settings);
	return bitrate;
}

static const double VIDEODATA_AVCVIDEOPACKET = 7.0;
// Additional FLV onMetaData values for Enhanced RTMP/FLV
static const double VIDEODATA_AV1VIDEOPACKET = 1635135537.0; // FourCC "av01"
#ifdef ENABLE_HEVC
static const double VIDEODATA_HEVCVIDEOPACKET = 1752589105.0; // FourCC "hvc1"
#endif

static inline double encoder_video_codec(obs_encoder_t *encoder)
{
	const char *codec = obs_encoder_get_codec(encoder);

	if (strcmp(codec, "h264") == 0)
		return VIDEODATA_AVCVIDEOPACKET;
	if (strcmp(codec, "av1") == 0)
		return VIDEODATA_AV1VIDEOPACKET;
#ifdef ENABLE_HEVC
	if (strcmp(codec, "hevc") == 0)
		return VIDEODATA_HEVCVIDEOPACKET;
#endif

	return 0.0;
}

/*
 * This is based on the position of `duration` and `fileSize` in
 * `build_flv_meta_data` relative to the beginning of the file
 * to allow `write_file_info` to overwrite these two fields once
 * the file is finalized.
 */
#define FLV_INFO_SIZE_OFFSET 58

void write_file_info(FILE *file, int64_t duration_ms, int64_t size)
{
	char buf[64];
	char *enc = buf;
	char *end = enc + sizeof(buf);

	fseek(file, FLV_INFO_SIZE_OFFSET, SEEK_SET);

	enc_num_val(&enc, end, "duration", (double)duration_ms / 1000.0);
	enc_num_val(&enc, end, "fileSize", (double)size);

	fwrite(buf, 1, enc - buf, file);
}

static void build_flv_meta_data(obs_output_t *context, uint8_t **output, size_t *size)
{
	obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
	obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
	video_t *video = obs_encoder_video(vencoder);
	audio_t *audio = obs_encoder_audio(aencoder);
	char buf[4096];
	char *enc = buf;
	char *end = enc + sizeof(buf);
	struct dstr encoder_name = {0};

	enc_str(&enc, end, "@setDataFrame");
	enc_str(&enc, end, "onMetaData");

	*enc++ = AMF_ECMA_ARRAY;
	enc = AMF_EncodeInt32(enc, end, 20);

	enc_num_val(&enc, end, "duration", 0.0);
	enc_num_val(&enc, end, "fileSize", 0.0);

	enc_num_val(&enc, end, "width", (double)obs_encoder_get_width(vencoder));
	enc_num_val(&enc, end, "height", (double)obs_encoder_get_height(vencoder));

	enc_num_val(&enc, end, "videocodecid", encoder_video_codec(vencoder));
	enc_num_val(&enc, end, "videodatarate", encoder_bitrate(vencoder));
	enc_num_val(&enc, end, "framerate", video_output_get_frame_rate(video));

	enc_num_val(&enc, end, "audiocodecid", AUDIODATA_AAC);
	enc_num_val(&enc, end, "audiodatarate", encoder_bitrate(aencoder));
	enc_num_val(&enc, end, "audiosamplerate", (double)obs_encoder_get_sample_rate(aencoder));
	enc_num_val(&enc, end, "audiosamplesize", 16.0);
	enc_num_val(&enc, end, "audiochannels", (double)audio_output_get_channels(audio));

	enc_bool_val(&enc, end, "stereo", audio_output_get_channels(audio) == 2);
	enc_bool_val(&enc, end, "2.1", audio_output_get_channels(audio) == 3);
	enc_bool_val(&enc, end, "3.1", audio_output_get_channels(audio) == 4);
	enc_bool_val(&enc, end, "4.0", audio_output_get_channels(audio) == 4);
	enc_bool_val(&enc, end, "4.1", audio_output_get_channels(audio) == 5);
	enc_bool_val(&enc, end, "5.1", audio_output_get_channels(audio) == 6);
	enc_bool_val(&enc, end, "7.1", audio_output_get_channels(audio) == 8);

	dstr_printf(&encoder_name, "%s (libobs version ", MODULE_NAME);

#ifdef HAVE_OBSCONFIG_H
	dstr_cat(&encoder_name, obs_get_version_string());
#else
	dstr_catf(&encoder_name, "%d.%d.%d", LIBOBS_API_MAJOR_VER, LIBOBS_API_MINOR_VER, LIBOBS_API_PATCH_VER);
#endif

	dstr_cat(&encoder_name, ")");

	enc_str_val(&enc, end, "encoder", encoder_name.array);
	dstr_free(&encoder_name);

	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	*size = enc - buf;
	*output = bmemdup(buf, *size);
}

static inline void write_previous_tag_size_without_header(struct serializer *s, uint32_t header_size)
{
	assert(serializer_get_pos(s) >= header_size);
	assert(serializer_get_pos(s) >= 11);

	/*
	 * From FLV file format specification version 10:
	 * Size of previous [current] tag, including its header.
	 * For FLV version 1 this value is 11 plus the DataSize of
	 * the previous [current] tag.
	 */
	s_wb32(s, (uint32_t)serializer_get_pos(s) - header_size);
}

static inline void write_previous_tag_size(struct serializer *s)
{
	write_previous_tag_size_without_header(s, 0);
}

void flv_meta_data(obs_output_t *context, uint8_t **output, size_t *size, bool write_header)
{
	struct array_output_data data;
	struct serializer s;
	uint8_t *meta_data = NULL;
	size_t meta_data_size;
	uint32_t start_pos;

	array_output_serializer_init(&s, &data);
	build_flv_meta_data(context, &meta_data, &meta_data_size);

	if (write_header) {
		s_write(&s, "FLV", 3);
		s_w8(&s, 1);
		s_w8(&s, 5);
		s_wb32(&s, 9);
		s_wb32(&s, 0);
	}

	start_pos = serializer_get_pos(&s);

	s_w8(&s, RTMP_PACKET_TYPE_INFO);

	s_wb24(&s, (uint32_t)meta_data_size);
	s_wb32(&s, 0);
	s_wb24(&s, 0);

	s_write(&s, meta_data, meta_data_size);

	write_previous_tag_size_without_header(&s, start_pos);

	*output = data.bytes.array;
	*size = data.bytes.num;

	bfree(meta_data);
}

#ifdef DEBUG_TIMESTAMPS
static int32_t last_time = 0;
#endif

static void flv_video(struct serializer *s, int32_t dts_offset, struct encoder_packet *packet, bool is_header)
{
	int64_t offset = packet->pts - packet->dts;
	int32_t time_ms = get_ms_time(packet, packet->dts) - dts_offset;

	if (!packet->data || !packet->size)
		return;

	s_w8(s, RTMP_PACKET_TYPE_VIDEO);

#ifdef DEBUG_TIMESTAMPS
	blog(LOG_DEBUG, "Video: %lu", time_ms);

	if (last_time > time_ms)
		blog(LOG_DEBUG, "Non-monotonic");

	last_time = time_ms;
#endif

	s_wb24(s, (uint32_t)packet->size + 5);
	s_wb24(s, (uint32_t)time_ms);
	s_w8(s, (time_ms >> 24) & 0x7F);
	s_wb24(s, 0);

	/* these are the 5 extra bytes mentioned above */
	s_w8(s, packet->keyframe ? 0x17 : 0x27);
	s_w8(s, is_header ? 0 : 1);
	s_wb24(s, get_ms_time(packet, offset));
	s_write(s, packet->data, packet->size);

	write_previous_tag_size(s);
}

static void flv_audio(struct serializer *s, int32_t dts_offset, struct encoder_packet *packet, bool is_header)
{
	int32_t time_ms = get_ms_time(packet, packet->dts) - dts_offset;

	if (!packet->data || !packet->size)
		return;

	s_w8(s, RTMP_PACKET_TYPE_AUDIO);

#ifdef DEBUG_TIMESTAMPS
	blog(LOG_DEBUG, "Audio: %lu", time_ms);

	if (last_time > time_ms)
		blog(LOG_DEBUG, "Non-monotonic");

	last_time = time_ms;
#endif

	s_wb24(s, (uint32_t)packet->size + 2);
	s_wb24(s, (uint32_t)time_ms);
	s_w8(s, (time_ms >> 24) & 0x7F);
	s_wb24(s, 0);

	/* these are the two extra bytes mentioned above */
	s_w8(s, 0xaf);
	s_w8(s, is_header ? 0 : 1);
	s_write(s, packet->data, packet->size);

	write_previous_tag_size(s);
}

void flv_packet_mux(struct encoder_packet *packet, int32_t dts_offset, uint8_t **output, size_t *size, bool is_header)
{
	struct array_output_data data;
	struct serializer s;

	array_output_serializer_init(&s, &data);

	if (packet->type == OBS_ENCODER_VIDEO)
		flv_video(&s, dts_offset, packet, is_header);
	else
		flv_audio(&s, dts_offset, packet, is_header);

	*output = data.bytes.array;
	*size = data.bytes.num;
}

void flv_packet_audio_ex(struct encoder_packet *packet, enum audio_id_t codec_id, int32_t dts_offset, uint8_t **output,
			 size_t *size, int type, size_t idx)
{
	struct array_output_data data;
	struct serializer s;

	array_output_serializer_init(&s, &data);

	assert(packet->type == OBS_ENCODER_AUDIO);

	int32_t time_ms = get_ms_time(packet, packet->dts) - dts_offset;

	bool is_multitrack = idx > 0;

	if (!packet->data || !packet->size)
		return;

	int header_metadata_size = 5; // w8+wa4cc
	if (is_multitrack)
		header_metadata_size += 2; // w8 + w8

	s_w8(&s, RTMP_PACKET_TYPE_AUDIO);

#ifdef DEBUG_TIMESTAMPS
	blog(LOG_DEBUG, "Audio: %lu", time_ms);

	if (last_time > time_ms)
		blog(LOG_DEBUG, "Non-monotonic");

	last_time = time_ms;
#endif

	s_wb24(&s, (uint32_t)packet->size + header_metadata_size);
	s_wb24(&s, (uint32_t)time_ms);
	s_w8(&s, (time_ms >> 24) & 0x7F);
	s_wb24(&s, 0);

	s_w8(&s, AUDIO_HEADER_EX | (is_multitrack ? AUDIO_PACKETTYPE_MULTITRACK : type));
	if (is_multitrack) {
		s_w8(&s, MULTITRACKTYPE_ONE_TRACK | type);
		s_wa4cc(&s, codec_id);
		s_w8(&s, (uint8_t)idx);
	} else {
		s_wa4cc(&s, codec_id);
	}

	s_write(&s, packet->data, packet->size);

	write_previous_tag_size(&s);

	*output = data.bytes.array;
	*size = data.bytes.num;
}

// Y2023 spec
void flv_packet_ex(struct encoder_packet *packet, enum video_id_t codec_id, int32_t dts_offset, uint8_t **output,
		   size_t *size, int type, size_t idx)
{
	struct array_output_data data;
	struct serializer s;
	array_output_serializer_init(&s, &data);

	assert(packet->type == OBS_ENCODER_VIDEO);

	int32_t time_ms = get_ms_time(packet, packet->dts) - dts_offset;

	bool is_multitrack = idx > 0;

	// packet head
	int header_metadata_size = 5; // w8+w4cc
	// 3 extra bytes for composition time offset
	if ((codec_id == CODEC_H264 || codec_id == CODEC_HEVC) && type == PACKETTYPE_FRAMES) {
		header_metadata_size += 3; // w24
	}
	if (is_multitrack)
		header_metadata_size += 2; // w8+w8

	s_w8(&s, RTMP_PACKET_TYPE_VIDEO);
	s_wb24(&s, (uint32_t)packet->size + header_metadata_size);
	s_wtimestamp(&s, time_ms);
	s_wb24(&s, 0); // always 0

	uint8_t frame_type = packet->keyframe ? FT_KEY : FT_INTER;

	/*
	 * We only explicitly emit trackIds iff idx > 0.
	 * The default trackId is 0.
	 */
	if (is_multitrack) {
		s_w8(&s, FRAME_HEADER_EX | PACKETTYPE_MULTITRACK | frame_type);
		s_w8(&s, MULTITRACKTYPE_ONE_TRACK | type);
		s_w4cc(&s, codec_id);
		// trackId
		s_w8(&s, (uint8_t)idx);
	} else {
		s_w8(&s, FRAME_HEADER_EX | type | frame_type);
		s_w4cc(&s, codec_id);
	}

	// H.264/HEVC composition time offset
	if ((codec_id == CODEC_H264 || codec_id == CODEC_HEVC) && type == PACKETTYPE_FRAMES) {
		s_wb24(&s, get_ms_time(packet, packet->pts - packet->dts));
	}

	// packet data
	s_write(&s, packet->data, packet->size);

	// packet tail
	write_previous_tag_size(&s);

	*output = data.bytes.array;
	*size = data.bytes.num;
}

void flv_packet_start(struct encoder_packet *packet, enum video_id_t codec, uint8_t **output, size_t *size, size_t idx)
{
	flv_packet_ex(packet, codec, 0, output, size, PACKETTYPE_SEQ_START, idx);
}

void flv_packet_frames(struct encoder_packet *packet, enum video_id_t codec, int32_t dts_offset, uint8_t **output,
		       size_t *size, size_t idx)
{
	int packet_type = PACKETTYPE_FRAMES;
	// PACKETTYPE_FRAMESX is an optimization to avoid sending composition
	// time offsets of 0. See Enhanced RTMP spec.
	if ((codec == CODEC_H264 || codec == CODEC_HEVC) && packet->dts == packet->pts)
		packet_type = PACKETTYPE_FRAMESX;
	flv_packet_ex(packet, codec, dts_offset, output, size, packet_type, idx);
}

void flv_packet_end(struct encoder_packet *packet, enum video_id_t codec, uint8_t **output, size_t *size, size_t idx)
{
	flv_packet_ex(packet, codec, 0, output, size, PACKETTYPE_SEQ_END, idx);
}

void flv_packet_audio_start(struct encoder_packet *packet, enum audio_id_t codec, uint8_t **output, size_t *size,
			    size_t idx)
{
	flv_packet_audio_ex(packet, codec, 0, output, size, AUDIO_PACKETTYPE_SEQ_START, idx);
}

void flv_packet_audio_frames(struct encoder_packet *packet, enum audio_id_t codec, int32_t dts_offset, uint8_t **output,
			     size_t *size, size_t idx)
{
	flv_packet_audio_ex(packet, codec, dts_offset, output, size, AUDIO_PACKETTYPE_FRAMES, idx);
}

void flv_packet_metadata(enum video_id_t codec_id, uint8_t **output, size_t *size, int bits_per_raw_sample,
			 uint8_t color_primaries, int color_trc, int color_space, int min_luminance, int max_luminance,
			 size_t idx)
{
	// metadata array
	struct array_output_data data;
	struct array_output_data metadata;
	struct serializer s;
	array_output_serializer_init(&s, &data);

	// metadata data array
	{
		struct serializer s;
		array_output_serializer_init(&s, &metadata);

		s_w8(&s, DATA_TYPE_STRING);
		s_wstring(&s, "colorInfo");
		s_w8(&s, DATA_TYPE_OBJECT);
		{
			// colorConfig:
			s_wstring(&s, "colorConfig");
			s_w8(&s, DATA_TYPE_OBJECT);
			{
				s_wstring(&s, "bitDepth");
				s_w8(&s, DATA_TYPE_NUMBER);
				s_wbd(&s, bits_per_raw_sample);

				s_wstring(&s, "colorPrimaries");
				s_w8(&s, DATA_TYPE_NUMBER);
				s_wbd(&s, color_primaries);

				s_wstring(&s, "transferCharacteristics");
				s_w8(&s, DATA_TYPE_NUMBER);
				s_wbd(&s, color_trc);

				s_wstring(&s, "matrixCoefficients");
				s_w8(&s, DATA_TYPE_NUMBER);
				s_wbd(&s, color_space);
			}
			s_w8(&s, 0);
			s_w8(&s, 0);
			s_w8(&s, DATA_TYPE_OBJECT_END);

			if (max_luminance != 0) {
				// hdrMdcv
				s_wstring(&s, "hdrMdcv");
				s_w8(&s, DATA_TYPE_OBJECT);
				{
					s_wstring(&s, "maxLuminance");
					s_w8(&s, DATA_TYPE_NUMBER);
					s_wbd(&s, max_luminance);

					s_wstring(&s, "minLuminance");
					s_w8(&s, DATA_TYPE_NUMBER);
					s_wbd(&s, min_luminance);
				}
				s_w8(&s, 0);
				s_w8(&s, 0);
				s_w8(&s, DATA_TYPE_OBJECT_END);
			}
		}
		s_w8(&s, 0);
		s_w8(&s, 0);
		s_w8(&s, DATA_TYPE_OBJECT_END);
	}

	bool is_multitrack = idx > 0;

	// packet head
	// w8+w4cc
	int header_metadata_size = 5;
	if (is_multitrack) {
		// w8+w8
		header_metadata_size += 2;
	}

	s_w8(&s, RTMP_PACKET_TYPE_VIDEO);
	s_wb24(&s, (uint32_t)metadata.bytes.num + header_metadata_size);
	s_wtimestamp(&s, 0);
	s_wb24(&s, 0); // always 0

	// packet ext header
	// these are the 5 extra bytes mentioned above
	s_w8(&s, FRAME_HEADER_EX | (is_multitrack ? PACKETTYPE_MULTITRACK : PACKETTYPE_METADATA));

	/*
	 * We only add explicitly emit trackIds iff idx > 0.
	 * The default trackId is 0.
	 */
	if (is_multitrack) {
		s_w8(&s, (uint8_t)MULTITRACKTYPE_ONE_TRACK | (uint8_t)PACKETTYPE_METADATA);
		s_w4cc(&s, codec_id);
		// trackId
		s_w8(&s, (uint8_t)idx);
	} else {
		s_w4cc(&s, codec_id);
	}

	// packet data
	s_write(&s, metadata.bytes.array, metadata.bytes.num);
	array_output_serializer_free(&metadata); // must be freed

	// packet tail
	write_previous_tag_size(&s);

	*output = data.bytes.array;
	*size = data.bytes.num;
}
