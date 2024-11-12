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

#include <util/base.h>
#include <util/deque.h>
#include <util/darray.h>
#include <util/dstr.h>
#include <obs-module.h>

#include <libavutil/channel_layout.h>
#include <libavformat/avformat.h>

#include "obs-ffmpeg-formats.h"
#include "obs-ffmpeg-compat.h"

#define do_log(level, format, ...) \
	blog(level, "[FFmpeg %s encoder: '%s'] " format, enc->type, obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

struct enc_encoder {
	obs_encoder_t *encoder;

	const char *type;

	const AVCodec *codec;
	AVCodecContext *context;

	uint8_t *samples[MAX_AV_PLANES];
	AVFrame *aframe;
	int64_t total_samples;

	DARRAY(uint8_t) packet_buffer;

	size_t audio_planes;
	size_t audio_size;

	int frame_size; /* pretty much always 1024 for AAC */
	int frame_size_bytes;
};

static const char *aac_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegAAC");
}

static const char *opus_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegOpus");
}

static const char *pcm_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegPCM16Bit");
}

static const char *pcm24_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegPCM24Bit");
}

static const char *pcm32_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegPCM32BitFloat");
}

static const char *alac_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegALAC");
}

static const char *flac_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegFLAC");
}

static void enc_destroy(void *data)
{
	struct enc_encoder *enc = data;

	if (enc->samples[0])
		av_freep(&enc->samples[0]);

	if (enc->context)
		avcodec_free_context(&enc->context);

	if (enc->aframe)
		av_frame_free(&enc->aframe);

	da_free(enc->packet_buffer);
	bfree(enc);
}

static bool initialize_codec(struct enc_encoder *enc)
{
	int ret;
	int channels;

	enc->aframe = av_frame_alloc();
	if (!enc->aframe) {
		warn("Failed to allocate audio frame");
		return false;
	}

	ret = avcodec_open2(enc->context, enc->codec, NULL);
	if (ret < 0) {
		struct dstr error_message = {0};
		dstr_printf(&error_message, "Failed to open AAC codec: %s", av_err2str(ret));
		obs_encoder_set_last_error(enc->encoder, error_message.array);
		dstr_free(&error_message);
		warn("Failed to open AAC codec: %s", av_err2str(ret));
		return false;
	}
	enc->aframe->format = enc->context->sample_fmt;
	channels = enc->context->ch_layout.nb_channels;
	enc->aframe->ch_layout = enc->context->ch_layout;
	enc->aframe->sample_rate = enc->context->sample_rate;

	enc->frame_size = enc->context->frame_size;
	if (!enc->frame_size)
		enc->frame_size = 1024;

	enc->frame_size_bytes = enc->frame_size * (int)enc->audio_size;

	ret = av_samples_alloc(enc->samples, NULL, channels, enc->frame_size, enc->context->sample_fmt, 0);
	if (ret < 0) {
		warn("Failed to create audio buffer: %s", av_err2str(ret));
		return false;
	}

	return true;
}

static void init_sizes(struct enc_encoder *enc, audio_t *audio)
{
	const struct audio_output_info *aoi;
	enum audio_format format;

	aoi = audio_output_get_info(audio);
	format = convert_ffmpeg_sample_format(enc->context->sample_fmt);

	enc->audio_planes = get_audio_planes(format, aoi->speakers);
	enc->audio_size = get_audio_size(format, aoi->speakers, 1);
}

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

static void *enc_create(obs_data_t *settings, obs_encoder_t *encoder, const char *type, const char *alt,
			enum AVSampleFormat sample_format)
{
	struct enc_encoder *enc;
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	audio_t *audio = obs_encoder_audio(encoder);

	enc = bzalloc(sizeof(struct enc_encoder));
	enc->encoder = encoder;
	enc->codec = avcodec_find_encoder_by_name(type);
	enc->type = type;

	if (!enc->codec && alt) {
		enc->codec = avcodec_find_encoder_by_name(alt);
		enc->type = alt;
	}

	blog(LOG_INFO, "---------------------------------");

	if (!enc->codec) {
		warn("Couldn't find encoder");
		goto fail;
	}

	const AVCodecDescriptor *codec_desc = avcodec_descriptor_get(enc->codec->id);

	if (!codec_desc) {
		warn("Failed to get codec descriptor");
		goto fail;
	}

	if (!bitrate && !(codec_desc->props & AV_CODEC_PROP_LOSSLESS)) {
		warn("Invalid bitrate specified");
		goto fail;
	}

	enc->context = avcodec_alloc_context3(enc->codec);
	if (!enc->context) {
		warn("Failed to create codec context");
		goto fail;
	}

	if (codec_desc->props & AV_CODEC_PROP_LOSSLESS)
		// Set by encoder on init, not known at this time
		enc->context->bit_rate = 0;
	else
		enc->context->bit_rate = bitrate * 1000;

	const struct audio_output_info *aoi;
	aoi = audio_output_get_info(audio);

	av_channel_layout_default(&enc->context->ch_layout, (int)audio_output_get_channels(audio));
	/* The avutil default channel layout for 5 channels is 5.0, which OBS
	 * does not support. Manually set 5 channels to 4.1. */
	if (aoi->speakers == SPEAKERS_4POINT1)
		enc->context->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_4POINT1;
	/* AAC, ALAC, & FLAC default to 3.0 for 3 channels instead of 2.1.
	 * Tell the encoder to deal with 2.1 as if it were 3.0. */
	if (aoi->speakers == SPEAKERS_2POINT1)
		enc->context->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_SURROUND;
	// ALAC supports 7.1 wide instead of regular 7.1.
	if (aoi->speakers == SPEAKERS_7POINT1 && astrcmpi(enc->type, "alac") == 0)
		enc->context->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_7POINT1_WIDE_BACK;

	enc->context->sample_rate = audio_output_get_sample_rate(audio);

	if (enc->codec->sample_fmts) {
		/* Check if the requested format is actually available for the specified
		 * encoder. This may not always be the case due to FFmpeg changes or a
		 * fallback being used (for example, when libopus is unavailable). */
		const enum AVSampleFormat *fmt = enc->codec->sample_fmts;
		while (*fmt != AV_SAMPLE_FMT_NONE) {
			if (*fmt == sample_format) {
				enc->context->sample_fmt = *fmt;
				break;
			}
			fmt++;
		}

		/* Fall back to default if requested format was not found. */
		if (enc->context->sample_fmt == AV_SAMPLE_FMT_NONE)
			enc->context->sample_fmt = enc->codec->sample_fmts[0];
	} else {
		/* Fall back to planar float if codec does not specify formats. */
		enc->context->sample_fmt = AV_SAMPLE_FMT_FLTP;
	}

	/* check to make sure sample rate is supported */
	if (enc->codec->supported_samplerates) {
		const int *rate = enc->codec->supported_samplerates;
		int cur_rate = enc->context->sample_rate;
		int closest = 0;

		while (*rate) {
			int dist = abs(cur_rate - *rate);
			int closest_dist = abs(cur_rate - closest);

			if (dist < closest_dist)
				closest = *rate;
			rate++;
		}

		if (closest)
			enc->context->sample_rate = closest;
	}

	char buf[256];
	av_channel_layout_describe(&enc->context->ch_layout, buf, 256);
	info("bitrate: %" PRId64 ", channels: %d, channel_layout: %s, track: %d\n",
	     (int64_t)enc->context->bit_rate / 1000, (int)enc->context->ch_layout.nb_channels, buf,
	     (int)obs_encoder_get_mixer_index(enc->encoder) + 1);
	init_sizes(enc, audio);

	/* enable experimental FFmpeg encoder if the only one available */
	enc->context->strict_std_compliance = -2;

	enc->context->flags = AV_CODEC_FLAG_GLOBAL_HEADER;

	if (initialize_codec(enc))
		return enc;

fail:
	enc_destroy(enc);
	return NULL;
}

static void *aac_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return enc_create(settings, encoder, "aac", NULL, AV_SAMPLE_FMT_NONE);
}

static void *opus_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return enc_create(settings, encoder, "libopus", "opus", AV_SAMPLE_FMT_FLT);
}

static void *pcm_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return enc_create(settings, encoder, "pcm_s16le", NULL, AV_SAMPLE_FMT_NONE);
}

static void *pcm24_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return enc_create(settings, encoder, "pcm_s24le", NULL, AV_SAMPLE_FMT_NONE);
}

static void *pcm32_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return enc_create(settings, encoder, "pcm_f32le", NULL, AV_SAMPLE_FMT_NONE);
}

static void *alac_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return enc_create(settings, encoder, "alac", NULL, AV_SAMPLE_FMT_S32P);
}

static void *flac_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return enc_create(settings, encoder, "flac", NULL, AV_SAMPLE_FMT_S16);
}

static bool do_encode(struct enc_encoder *enc, struct encoder_packet *packet, bool *received_packet)
{
	AVRational time_base = {1, enc->context->sample_rate};
	AVPacket avpacket = {0};
	int got_packet;
	int ret;
	int channels;

	enc->aframe->nb_samples = enc->frame_size;
	enc->aframe->pts =
		av_rescale_q(enc->total_samples, (AVRational){1, enc->context->sample_rate}, enc->context->time_base);
	enc->aframe->ch_layout = enc->context->ch_layout;
	channels = enc->context->ch_layout.nb_channels;
	ret = avcodec_fill_audio_frame(enc->aframe, channels, enc->context->sample_fmt, enc->samples[0],
				       enc->frame_size_bytes * channels, 1);
	if (ret < 0) {
		warn("avcodec_fill_audio_frame failed: %s", av_err2str(ret));
		return false;
	}

	enc->total_samples += enc->frame_size;

	ret = avcodec_send_frame(enc->context, enc->aframe);
	if (ret == 0)
		ret = avcodec_receive_packet(enc->context, &avpacket);

	got_packet = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;
	if (ret < 0) {
		warn("avcodec_encode_audio2 failed: %s", av_err2str(ret));
		return false;
	}

	*received_packet = !!got_packet;
	if (!got_packet)
		return true;

	da_resize(enc->packet_buffer, 0);
	da_push_back_array(enc->packet_buffer, avpacket.data, avpacket.size);

	packet->pts = rescale_ts(avpacket.pts, enc->context, time_base);
	packet->dts = rescale_ts(avpacket.dts, enc->context, time_base);
	packet->data = enc->packet_buffer.array;
	packet->size = avpacket.size;
	packet->type = OBS_ENCODER_AUDIO;
	packet->keyframe = true;
	packet->timebase_num = 1;
	packet->timebase_den = (int32_t)enc->context->sample_rate;
	av_packet_unref(&avpacket);
	return true;
}

static bool enc_encode(void *data, struct encoder_frame *frame, struct encoder_packet *packet, bool *received_packet)
{
	struct enc_encoder *enc = data;

	for (size_t i = 0; i < enc->audio_planes; i++)
		memcpy(enc->samples[i], frame->data[i], enc->frame_size_bytes);

	return do_encode(enc, packet, received_packet);
}

static void enc_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 128);
}

static obs_properties_t *enc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_properties_add_int(props, "bitrate", obs_module_text("Bitrate"), 64, 1024, 32);
	return props;
}

static bool enc_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct enc_encoder *enc = data;

	*extra_data = enc->context->extradata;
	*size = enc->context->extradata_size;
	return true;
}

static void enc_audio_info(void *data, struct audio_convert_info *info)
{
	struct enc_encoder *enc = data;
	int channels;
	channels = enc->context->ch_layout.nb_channels;
	info->format = convert_ffmpeg_sample_format(enc->context->sample_fmt);
	info->samples_per_sec = (uint32_t)enc->context->sample_rate;
	if (channels != 7 && channels <= 8)
		info->speakers = (enum speaker_layout)(channels);
	else
		info->speakers = SPEAKERS_UNKNOWN;
}

static void enc_audio_info_float(void *data, struct audio_convert_info *info)
{
	enc_audio_info(data, info);
	info->allow_clipping = true;
}

static size_t enc_frame_size(void *data)
{
	struct enc_encoder *enc = data;
	return enc->frame_size;
}

struct obs_encoder_info aac_encoder_info = {
	.id = "ffmpeg_aac",
	.type = OBS_ENCODER_AUDIO,
	.codec = "aac",
	.get_name = aac_getname,
	.create = aac_create,
	.destroy = enc_destroy,
	.encode = enc_encode,
	.get_frame_size = enc_frame_size,
	.get_defaults = enc_defaults,
	.get_properties = enc_properties,
	.get_extra_data = enc_extra_data,
	.get_audio_info = enc_audio_info,
};

struct obs_encoder_info opus_encoder_info = {
	.id = "ffmpeg_opus",
	.type = OBS_ENCODER_AUDIO,
	.codec = "opus",
	.get_name = opus_getname,
	.create = opus_create,
	.destroy = enc_destroy,
	.encode = enc_encode,
	.get_frame_size = enc_frame_size,
	.get_defaults = enc_defaults,
	.get_properties = enc_properties,
	.get_extra_data = enc_extra_data,
	.get_audio_info = enc_audio_info,
};

struct obs_encoder_info pcm_encoder_info = {
	.id = "ffmpeg_pcm_s16le",
	.type = OBS_ENCODER_AUDIO,
	.codec = "pcm_s16le",
	.get_name = pcm_getname,
	.create = pcm_create,
	.destroy = enc_destroy,
	.encode = enc_encode,
	.get_frame_size = enc_frame_size,
	.get_defaults = enc_defaults,
	.get_properties = enc_properties,
	.get_extra_data = enc_extra_data,
	.get_audio_info = enc_audio_info,
};

struct obs_encoder_info pcm24_encoder_info = {
	.id = "ffmpeg_pcm_s24le",
	.type = OBS_ENCODER_AUDIO,
	.codec = "pcm_s24le",
	.get_name = pcm24_getname,
	.create = pcm24_create,
	.destroy = enc_destroy,
	.encode = enc_encode,
	.get_frame_size = enc_frame_size,
	.get_defaults = enc_defaults,
	.get_properties = enc_properties,
	.get_extra_data = enc_extra_data,
	.get_audio_info = enc_audio_info,
};

struct obs_encoder_info pcm32_encoder_info = {
	.id = "ffmpeg_pcm_f32le",
	.type = OBS_ENCODER_AUDIO,
	.codec = "pcm_f32le",
	.get_name = pcm32_getname,
	.create = pcm32_create,
	.destroy = enc_destroy,
	.encode = enc_encode,
	.get_frame_size = enc_frame_size,
	.get_defaults = enc_defaults,
	.get_properties = enc_properties,
	.get_extra_data = enc_extra_data,
	.get_audio_info = enc_audio_info_float,
};

struct obs_encoder_info alac_encoder_info = {
	.id = "ffmpeg_alac",
	.type = OBS_ENCODER_AUDIO,
	.codec = "alac",
	.get_name = alac_getname,
	.create = alac_create,
	.destroy = enc_destroy,
	.encode = enc_encode,
	.get_frame_size = enc_frame_size,
	.get_defaults = enc_defaults,
	.get_properties = enc_properties,
	.get_extra_data = enc_extra_data,
	.get_audio_info = enc_audio_info,
};

struct obs_encoder_info flac_encoder_info = {
	.id = "ffmpeg_flac",
	.type = OBS_ENCODER_AUDIO,
	.codec = "flac",
	.get_name = flac_getname,
	.create = flac_create,
	.destroy = enc_destroy,
	.encode = enc_encode,
	.get_frame_size = enc_frame_size,
	.get_defaults = enc_defaults,
	.get_properties = enc_properties,
	.get_extra_data = enc_extra_data,
	.get_audio_info = enc_audio_info,
};
