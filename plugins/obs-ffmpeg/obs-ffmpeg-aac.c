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

#include <util/base.h>
#include <util/circlebuf.h>
#include <util/darray.h>
#include <obs-module.h>

#include <libavformat/avformat.h>

#include "obs-ffmpeg-formats.h"
#include "obs-ffmpeg-compat.h"

#define do_log(level, format, ...) \
	blog(level, "[FFmpeg aac encoder: '%s'] " format, \
			obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

struct aac_encoder {
	obs_encoder_t    *encoder;

	AVCodec          *aac;
	AVCodecContext   *context;

	uint8_t          *samples[MAX_AV_PLANES];
	AVFrame          *aframe;
	int64_t          total_samples;

	DARRAY(uint8_t)  packet_buffer;

	size_t           audio_planes;
	size_t           audio_size;

	int              frame_size; /* pretty much always 1024 for AAC */
	int              frame_size_bytes;
};

static const char *aac_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegAAC");
}

static void aac_destroy(void *data)
{
	struct aac_encoder *enc = data;

	if (enc->samples[0])
		av_freep(&enc->samples[0]);
	if (enc->context)
		avcodec_close(enc->context);
	if (enc->aframe)
		av_frame_free(&enc->aframe);

	da_free(enc->packet_buffer);
	bfree(enc);
}

static bool initialize_codec(struct aac_encoder *enc)
{
	int ret;

	enc->aframe  = av_frame_alloc();
	if (!enc->aframe) {
		warn("Failed to allocate audio frame");
		return false;
	}

	ret = avcodec_open2(enc->context, enc->aac, NULL);
	if (ret < 0) {
		warn("Failed to open AAC codec: %s", av_err2str(ret));
		return false;
	}

	enc->frame_size = enc->context->frame_size;
	if (!enc->frame_size)
		enc->frame_size = 1024;

	enc->frame_size_bytes = enc->frame_size * (int)enc->audio_size;

	ret = av_samples_alloc(enc->samples, NULL, enc->context->channels,
			enc->frame_size, enc->context->sample_fmt, 0);
	if (ret < 0) {
		warn("Failed to create audio buffer: %s", av_err2str(ret));
		return false;
	}

	return true;
}

static void init_sizes(struct aac_encoder *enc, audio_t *audio)
{
	const struct audio_output_info *aoi;
	enum audio_format format;

	aoi    = audio_output_get_info(audio);
	format = convert_ffmpeg_sample_format(enc->context->sample_fmt);

	enc->audio_planes = get_audio_planes(format, aoi->speakers);
	enc->audio_size   = get_audio_size(format, aoi->speakers, 1);
}

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

static void *aac_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	struct aac_encoder *enc;
	int                bitrate = (int)obs_data_get_int(settings, "bitrate");
	audio_t            *audio   = obs_encoder_audio(encoder);

	avcodec_register_all();

	enc          = bzalloc(sizeof(struct aac_encoder));
	enc->encoder = encoder;
	enc->aac     = avcodec_find_encoder(AV_CODEC_ID_AAC);

	blog(LOG_INFO, "---------------------------------");

	if (!enc->aac) {
		warn("Couldn't find encoder");
		goto fail;
	}

	if (!bitrate) {
		warn("Invalid bitrate specified");
		return NULL;
	}

	enc->context = avcodec_alloc_context3(enc->aac);
	if (!enc->context) {
		warn("Failed to create codec context");
		goto fail;
	}

	enc->context->bit_rate    = bitrate * 1000;
	enc->context->channels    = (int)audio_output_get_channels(audio);
	enc->context->sample_rate = audio_output_get_sample_rate(audio);
	enc->context->sample_fmt  = enc->aac->sample_fmts ?
		enc->aac->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

	/* if using FFmpeg's AAC encoder, at least set a cutoff value
	 * (recommended by konverter) */
	if (strcmp(enc->aac->name, "aac") == 0) {
		int cutoff1 = 4000 + (int)enc->context->bit_rate / 8;
		int cutoff2 = 12000 + (int)enc->context->bit_rate / 8;
		int cutoff3 = enc->context->sample_rate / 2;
		int cutoff;

		cutoff = MIN(cutoff1, cutoff2);
		cutoff = MIN(cutoff, cutoff3);
		enc->context->cutoff = cutoff;
	}

	info("bitrate: %d, channels: %d",
			enc->context->bit_rate / 1000, enc->context->channels);

	init_sizes(enc, audio);

	/* enable experimental FFmpeg encoder if the only one available */
	enc->context->strict_std_compliance = -2;

	enc->context->flags = CODEC_FLAG_GLOBAL_HEADER;

	if (initialize_codec(enc))
		return enc;

fail:
	aac_destroy(enc);
	return NULL;
}

static bool do_aac_encode(struct aac_encoder *enc,
		struct encoder_packet *packet, bool *received_packet)
{
	AVRational time_base = {1, enc->context->sample_rate};
	AVPacket   avpacket  = {0};
	int        got_packet;
	int        ret;

	enc->aframe->nb_samples = enc->frame_size;
	enc->aframe->pts = av_rescale_q(enc->total_samples,
			(AVRational){1, enc->context->sample_rate},
			enc->context->time_base);

	ret = avcodec_fill_audio_frame(enc->aframe, enc->context->channels,
			enc->context->sample_fmt, enc->samples[0],
			enc->frame_size_bytes * enc->context->channels, 1);
	if (ret < 0) {
		warn("avcodec_fill_audio_frame failed: %s", av_err2str(ret));
		return false;
	}

	enc->total_samples += enc->frame_size;

	ret = avcodec_encode_audio2(enc->context, &avpacket, enc->aframe,
			&got_packet);
	if (ret < 0) {
		warn("avcodec_encode_audio2 failed: %s", av_err2str(ret));
		return false;
	}

	*received_packet = !!got_packet;
	if (!got_packet)
		return true;

	da_resize(enc->packet_buffer, 0);
	da_push_back_array(enc->packet_buffer, avpacket.data, avpacket.size);

	packet->pts  = rescale_ts(avpacket.pts, enc->context, time_base);
	packet->dts  = rescale_ts(avpacket.dts, enc->context, time_base);
	packet->data = enc->packet_buffer.array;
	packet->size = avpacket.size;
	packet->type = OBS_ENCODER_AUDIO;
	packet->timebase_num = 1;
	packet->timebase_den = (int32_t)enc->context->sample_rate;
	av_free_packet(&avpacket);
	return true;
}

static bool aac_encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
	struct aac_encoder *enc = data;

	for (size_t i = 0; i < enc->audio_planes; i++)
		memcpy(enc->samples[i], frame->data[i], enc->frame_size_bytes);

	return do_aac_encode(enc, packet, received_packet);
}

static void aac_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 128);
}

static obs_properties_t *aac_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(props, "bitrate",
			obs_module_text("Bitrate"), 32, 320, 32);
	return props;
}

static bool aac_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct aac_encoder *enc = data;

	*extra_data = enc->context->extradata;
	*size       = enc->context->extradata_size;
	return true;
}

static void aac_audio_info(void *data, struct audio_convert_info *info)
{
	struct aac_encoder *enc = data;
	info->format = convert_ffmpeg_sample_format(enc->context->sample_fmt);
}

static size_t aac_frame_size(void *data)
{
	struct aac_encoder *enc =data;
	return enc->frame_size;
}

struct obs_encoder_info aac_encoder_info = {
	.id             = "ffmpeg_aac",
	.type           = OBS_ENCODER_AUDIO,
	.codec          = "AAC",
	.get_name       = aac_getname,
	.create         = aac_create,
	.destroy        = aac_destroy,
	.encode         = aac_encode,
	.get_frame_size = aac_frame_size,
	.get_defaults   = aac_defaults,
	.get_properties = aac_properties,
	.get_extra_data = aac_extra_data,
	.get_audio_info = aac_audio_info
};
