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
#include <obs.h>

#include <libavformat/avformat.h>
#include "obs-ffmpeg-formats.h"

struct aac_encoder {
	obs_encoder_t    encoder;

	AVCodec          *aac;
	AVCodecContext   *context;

	struct circlebuf buffers[MAX_AV_PLANES];
	uint8_t          *samples[MAX_AV_PLANES];
	AVFrame          *aframe;
	int              total_samples;

	size_t           audio_planes;
	size_t           audio_size;

	int              frame_size; /* pretty much always 1024 for AAC */
	int              frame_size_bytes;
};

static const char *aac_getname(const char *locale)
{
	UNUSED_PARAMETER(locale);
	return "FFmpeg Default AAC Encoder";
}

static void aac_warn(const char *func, const char *format, ...)
{
	va_list args;
	char msg[1024];

	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	blog(LOG_WARNING, "[%s}: %s", func, msg);
	va_end(args);
}

static void aac_destroy(void *data)
{
	struct aac_encoder *enc = data;

	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		circlebuf_free(&enc->buffers[i]);

	if (enc->samples[0])
		av_freep(&enc->samples[0]);
	if (enc->context)
		avcodec_close(enc->context);
	if (enc->aframe)
		av_frame_free(&enc->aframe);
	bfree(enc);
}

static bool initialize_codec(struct aac_encoder *enc)
{
	int ret;

	enc->aframe  = av_frame_alloc();
	if (!enc->aframe) {
		aac_warn("initialize_codec", "Failed to allocate audio frame");
		return false;
	}

	ret = avcodec_open2(enc->context, enc->aac, NULL);
	if (ret < 0) {
		aac_warn("initialize_codec", "Failed to open AAC codec: %s",
				av_err2str(ret));
		return false;
	}

	enc->frame_size = enc->context->frame_size;
	if (!enc->frame_size)
		enc->frame_size = 1024;

	enc->frame_size_bytes = enc->frame_size * (int)enc->audio_size;

	ret = av_samples_alloc(enc->samples, NULL, enc->context->channels,
			enc->frame_size, enc->context->sample_fmt, 0);
	if (ret < 0) {
		aac_warn("initialize_codec", "Failed to create audio buffer: "
		                             "%s", av_err2str(ret));
		return false;
	}

	return true;
}

static void *aac_create(obs_data_t settings, obs_encoder_t encoder)
{
	struct aac_encoder *enc;
	int                bitrate = (int)obs_data_getint(settings, "bitrate");
	audio_t            audio   = obs_encoder_audio(encoder);

	if (!bitrate) {
		aac_warn("aac_create", "Invalid bitrate specified");
		return NULL;
	}

	enc          = bzalloc(sizeof(struct aac_encoder));
	enc->encoder = encoder;
	enc->aac     = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!enc->aac) {
		aac_warn("aac_create", "Couldn't find encoder");
		goto fail;
	}

	avcodec_register(enc->aac);

	enc->context = avcodec_alloc_context3(enc->aac);
	if (!enc->context) {
		aac_warn("aac_create", "Failed to create codec context");
		goto fail;
	}

	enc->context->bit_rate    = bitrate;
	enc->context->channels    = (int)audio_output_channels(audio);
	enc->context->sample_rate = audio_output_samplerate(audio);
	enc->context->sample_fmt  = enc->aac->sample_fmts ?
		enc->aac->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

	enc->audio_planes = audio_output_planes(audio);
	enc->audio_size   = audio_output_blocksize(audio);

	/* enable experimental FFmpeg encoder if the only one available */
	enc->context->strict_std_compliance = -2;

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
		aac_warn("do_aac_encode", "avcodec_fill_audio_frame failed: %s",
				av_err2str(ret));
		return false;
	}

	enc->total_samples += enc->frame_size;

	ret = avcodec_encode_audio2(enc->context, &avpacket, enc->aframe,
			&got_packet);
	if (ret < 0) {
		aac_warn("do_aac_encode", "avcodec_encode_audio2 failed: %s",
				av_err2str(ret));
		return false;
	}

	*received_packet = !!got_packet;
	if (!got_packet)
		return true;

	packet->pts  = rescale_ts(avpacket.pts, enc->context, time_base);
	packet->dts  = rescale_ts(avpacket.dts, enc->context, time_base);
	packet->data = bmemdup(avpacket.data, avpacket.size);
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
	size_t size = frame->frames * enc->audio_size;

	for (size_t i = 0; i < enc->audio_planes; i++)
		circlebuf_push_back(&enc->buffers[i], frame->data[i], size);

	if (enc->buffers[0].size < (size_t)enc->frame_size_bytes) {
		*received_packet = false;
		return true;
	}

	for (size_t i = 0; i < enc->audio_planes; i++)
		circlebuf_pop_front(&enc->buffers[i], enc->samples[i],
				enc->frame_size_bytes);

	return do_aac_encode(enc, packet, received_packet);
}

static void aac_defaults(obs_data_t settings)
{
	obs_data_setint(settings, "bitrate", 128);
}

static obs_properties_t aac_properties(const char *locale)
{
	obs_properties_t props = obs_properties_create(locale);

	/* TODO: locale */
	obs_properties_add_int(props, "bitrate", "Bitrate",
			32, 320, 32);
	return props;
}

static bool aac_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct aac_encoder *enc = data;

	*extra_data = enc->context->extradata;
	*size       = enc->context->extradata_size;
	return true;
}

static bool aac_audio_info(void *data, struct audio_convert_info *info)
{
	struct aac_encoder *enc = data;

	memset(info, 0, sizeof(struct audio_convert_info));
	info->format = convert_ffmpeg_sample_format(enc->context->sample_fmt);
	return true;
}

struct obs_encoder_info aac_encoder_info = {
	.id         = "ffmpeg_aac",
	.type       = OBS_ENCODER_AUDIO,
	.codec      = "AAC",
	.getname    = aac_getname,
	.create     = aac_create,
	.destroy    = aac_destroy,
	.encode     = aac_encode,
	.defaults   = aac_defaults,
	.properties = aac_properties,
	.extra_data = aac_extra_data,
	.audio_info = aac_audio_info
};
