/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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
#include <util/array-serializer.h>
#include "flv-mux.h"
#include "obs-output-ver.h"
#include "rtmp-helpers.h"

#define VIDEO_HEADER_SIZE 5
#define MILLISECOND_DEN   1000

static inline double encoder_bitrate(obs_encoder_t encoder)
{
	obs_data_t settings = obs_encoder_get_settings(encoder);
	double bitrate = obs_data_getdouble(settings, "bitrate");

	obs_data_release(settings);
	return bitrate;
}

static void build_flv_meta_data(obs_output_t context,
		uint8_t **output, size_t *size)
{
	obs_encoder_t vencoder = obs_output_get_video_encoder(context);
	obs_encoder_t aencoder = obs_output_get_audio_encoder(context);
	video_t       video    = obs_encoder_video(vencoder);
	audio_t       audio    = obs_encoder_audio(aencoder);
	char buf[4096];
	char *enc = buf;
	char *end = enc+sizeof(buf);

	*enc++ = AMF_ECMA_ARRAY;
	enc    = AMF_EncodeInt32(enc, end, 14);

	enc_num(&enc, end, "duration", 0.0);
	enc_num(&enc, end, "fileSize", 0.0);

	enc_num(&enc, end, "width",  (double)video_output_width(video));
	enc_num(&enc, end, "height", (double)video_output_height(video));
	enc_str(&enc, end, "videocodecid", "avc1");
	enc_num(&enc, end, "videodatarate", encoder_bitrate(vencoder));
	enc_num(&enc, end, "framerate", video_output_framerate(video));

	enc_str(&enc, end, "audiocodecid", "mp4a");
	enc_num(&enc, end, "audiodatarate", encoder_bitrate(aencoder));
	enc_num(&enc, end, "audiosamplerate",
			(double)audio_output_samplerate(audio));
	enc_num(&enc, end, "audiosamplesize", 16.0);
	enc_num(&enc, end, "audiochannels",
			(double)audio_output_channels(audio));

	enc_bool(&enc, end, "stereo", audio_output_channels(audio) == 2);
	enc_str(&enc, end, "encoder", MODULE_NAME);

	*enc++  = 0;
	*enc++  = 0;
	*enc++  = AMF_OBJECT_END;

	*size   = enc-buf;
	*output = bmemdup(buf, *size);
}

void flv_meta_data(obs_output_t context, uint8_t **output, size_t *size)
{
	struct array_output_data data;
	struct serializer s;
	uint8_t *meta_data;
	size_t  meta_data_size;

	build_flv_meta_data(context, &meta_data, &meta_data_size);

	array_output_serializer_init(&s, &data);
	s_w8(&s, RTMP_PACKET_TYPE_VIDEO);

	s_wb24(&s, (uint32_t)meta_data_size);
	s_wb32(&s, 0);
	s_wb24(&s, 0);
	s_write(&s, meta_data, meta_data_size);

	s_wb32(&s, (uint32_t)serializer_get_pos(&s) + 4 - 1);

	*output = data.bytes.array;
	*size   = data.bytes.num;

	bfree(meta_data);
}

static uint32_t get_ms_time(struct encoder_packet *packet, int64_t val)
{
	return (uint32_t)(val * MILLISECOND_DEN / packet->timebase_den);
}

static void flv_video(struct serializer *s, struct encoder_packet *packet,
		bool is_header)
{
	int64_t offset = packet->pts - packet->dts;

	if (!packet->data || !packet->size)
		return;

	s_w8(s, RTMP_PACKET_TYPE_VIDEO);

	s_wb24(s, (uint32_t)packet->size + 5);
	s_wb32(s, get_ms_time(packet, packet->pts));
	s_wb24(s, 0);

	/* these are the 5 extra bytes mentioned above */
	s_w8(s, packet->keyframe ? 0x17 : 0x27);
	s_w8(s, is_header ? 0 : 1);
	s_wb24(s, get_ms_time(packet, offset));
	s_write(s, packet->data, packet->size);

	/* write tag size (starting byte doesnt count) */
	s_wb32(s, (uint32_t)serializer_get_pos(s) + 4 - 1);
}

static void flv_audio(struct serializer *s, struct encoder_packet *packet,
		bool is_header)
{
	if (!packet->data || !packet->size)
		return;

	s_w8(s, RTMP_PACKET_TYPE_AUDIO);

	s_wb24(s, (uint32_t)packet->size + 2);
	s_wb32(s, get_ms_time(packet, packet->pts));
	s_wb24(s, 0);

	/* these are the two extra bytes mentioned above */
	s_w8(s, 0xaf);
	s_w8(s, is_header ? 0 : 1);
	s_write(s, packet->data, packet->size);

	/* write tag size (starting byte doesnt count) */
	s_wb32(s, (uint32_t)serializer_get_pos(s) + 4 - 1);
}

void flv_packet_mux(struct encoder_packet *packet,
		uint8_t **output, size_t *size, bool is_header)
{
	struct array_output_data data;
	struct serializer s;

	array_output_serializer_init(&s, &data);

	if (packet->type == OBS_ENCODER_VIDEO)
		flv_video(&s, packet, is_header);
	else
		flv_audio(&s, packet, is_header);

	*output = data.bytes.array;
	*size   = data.bytes.num;
}
