/*
 * Copyright (c) 2015 John R. Bradley <jrb@turrettech.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <obs-module.h>
#include <util/platform.h>

#include "obs-ffmpeg-compat.h"
#include "obs-ffmpeg-formats.h"

#include <libff/ff-demuxer.h>

#include <libswscale/swscale.h>

static bool video_frame(struct ff_frame *frame, void *opaque);
static bool video_format(AVCodecContext *codec_context, void *opaque);

static void ffmpeg_log_callback(void* context, int level, const char* format,
	va_list args)
{
	UNUSED_PARAMETER(context);
	int obs_level;

	switch (level) {
	case AV_LOG_PANIC:
	case AV_LOG_FATAL:
		obs_level = LOG_ERROR;
		break;
	case AV_LOG_ERROR:
	case AV_LOG_WARNING:
		obs_level = LOG_WARNING;
		break;
	case AV_LOG_INFO:
	case AV_LOG_VERBOSE:
		obs_level = LOG_INFO;
		break;
	case AV_LOG_DEBUG:
	default:
		obs_level = LOG_DEBUG;
	}

	blogva(obs_level, format, args);
}

void initialize_ffmpeg_source()
{
	av_log_set_callback(ffmpeg_log_callback);
}

struct ffmpeg_source {
	struct ff_demuxer *demuxer;
	struct SwsContext *sws_ctx;
	uint8_t *sws_data;
	int sws_linesize;
	obs_source_t *source;
	enum AVPixelFormat format;
	bool is_forcing_scale;
	bool is_hw_decoding;
};

static bool set_obs_frame_colorprops(struct ff_frame *frame,
		struct obs_source_frame *obs_frame)
{
	enum AVColorSpace frame_cs = av_frame_get_colorspace(frame->frame);
	enum video_colorspace obs_cs;

	switch(frame_cs) {
	case AVCOL_SPC_BT709:       obs_cs = VIDEO_CS_709; break;
	case AVCOL_SPC_SMPTE170M:
	case AVCOL_SPC_BT470BG:     obs_cs = VIDEO_CS_601; break;
	case AVCOL_SPC_UNSPECIFIED: obs_cs = VIDEO_CS_DEFAULT; break;
	default:
		blog(LOG_WARNING, "frame using an unsupported colorspace %d",
				frame_cs);
		return false;
	}

	enum video_range_type range;
	obs_frame->format = ffmpeg_to_obs_video_format(frame->frame->format);
	obs_frame->full_range =
		frame->frame->color_range == AVCOL_RANGE_JPEG;

	range = obs_frame->full_range ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;

	if (!video_format_get_parameters(obs_cs,
			range, obs_frame->color_matrix,
			obs_frame->color_range_min,
			obs_frame->color_range_max)) {
		blog(LOG_ERROR, "Failed to get video format "
                                "parameters for video format %u",
                                obs_cs);
		return false;
	}

	return true;
}

bool update_sws_context(struct ffmpeg_source *source, AVFrame *frame)
{
	struct SwsContext *sws_ctx = sws_getCachedContext(
			source->sws_ctx,
			frame->width,
			frame->height,
			frame->format,
			frame->width,
			frame->height,
			AV_PIX_FMT_BGRA,
			SWS_BILINEAR,
			NULL, NULL, NULL);

	if (sws_ctx != NULL && sws_ctx != source->sws_ctx) {
		if (source->sws_data != NULL)
			bfree(source->sws_data);
		source->sws_data = bzalloc(frame->width * frame->height * 4);
	}

	source->sws_ctx = sws_ctx;
	source->sws_linesize = frame->width * 4;

	return source->sws_ctx != NULL;
}

static bool video_frame_scale(struct ff_frame *frame,
		struct ffmpeg_source *s, struct obs_source_frame *obs_frame)
{
	if (!update_sws_context(s, frame->frame))
		return false;

	sws_scale(
		s->sws_ctx,
		(uint8_t const *const *)frame->frame->data,
		frame->frame->linesize,
		0,
		frame->frame->height,
		&s->sws_data,
		&s->sws_linesize
	);

	obs_frame->data[0]     = s->sws_data;
	obs_frame->linesize[0] = s->sws_linesize;
	obs_frame->format      = VIDEO_FORMAT_BGRA;

	obs_source_output_video(s->source, obs_frame);

	return true;
}

static bool video_frame_hwaccel(struct ff_frame *frame,
		obs_source_t *source, struct obs_source_frame *obs_frame)
{
	// 4th plane is pixelbuf reference for mac
	for (int i = 0; i < 3; i++) {
		obs_frame->data[i] = frame->frame->data[i];
		obs_frame->linesize[i] = frame->frame->linesize[i];
	}

	if (!set_obs_frame_colorprops(frame, obs_frame))
		return false;

	obs_source_output_video(source, obs_frame);
	return true;
}

static bool video_frame_direct(struct ff_frame *frame,
		struct ffmpeg_source *s, struct obs_source_frame *obs_frame)
{
	int i;

	for (i = 0; i < MAX_AV_PLANES; i++) {
		obs_frame->data[i] = frame->frame->data[i];
		obs_frame->linesize[i] = frame->frame->linesize[i];
	}

	if (!set_obs_frame_colorprops(frame, obs_frame))
		return false;

	obs_source_output_video(s->source, obs_frame);
	return true;
}

static bool video_frame(struct ff_frame *frame, void *opaque)
{
	struct ffmpeg_source *s = opaque;
	struct obs_source_frame obs_frame = {0};

	double d_pts = ff_get_sync_clock(&s->demuxer->clock) - frame->pts;
	uint64_t pts = os_gettime_ns() - (uint64_t)(d_pts * 1000000000.0L);

	obs_frame.timestamp = pts;
	obs_frame.width = frame->frame->width;
	obs_frame.height = frame->frame->height;

	enum video_format format =
			ffmpeg_to_obs_video_format(frame->frame->format);

	if (s->is_forcing_scale || format == VIDEO_FORMAT_NONE)
		return video_frame_scale(frame, s, &obs_frame);
	else if (s->is_hw_decoding)
		return video_frame_hwaccel(frame, s->source, &obs_frame);
	else
		return video_frame_direct(frame, s, &obs_frame);
}

static bool audio_frame(struct ff_frame *frame, void *opaque)
{
	struct ffmpeg_source *s = opaque;

	struct obs_source_audio audio_data = {0};

	double d_pts = ff_get_sync_clock(&s->demuxer->clock) - frame->pts;
	uint64_t pts = os_gettime_ns() - (uint64_t)(d_pts * 1000000000.0L);

	int channels = av_get_channel_layout_nb_channels(
				av_frame_get_channel_layout(frame->frame));

	for(int i = 0; i < channels; i++)
		audio_data.data[i] = frame->frame->data[i];

	audio_data.samples_per_sec = frame->frame->sample_rate;
	audio_data.frames = frame->frame->nb_samples;
	audio_data.timestamp = pts;
	audio_data.format =
		convert_ffmpeg_sample_format(frame->frame->format);
	audio_data.speakers = channels;

	obs_source_output_audio(s->source, &audio_data);

	return true;
}

static bool is_local_file_modified(obs_properties_t *props,
		obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);

	bool enabled = obs_data_get_bool(settings, "is_local_file");
	obs_property_t *input = obs_properties_get(props, "input");
	obs_property_t *input_format =obs_properties_get(props,
			"input_format");
	obs_property_t *local_file = obs_properties_get(props, "local_file");
	obs_property_t *looping = obs_properties_get(props, "looping");
	obs_property_set_visible(input, !enabled);
	obs_property_set_visible(input_format, !enabled);
	obs_property_set_visible(local_file, enabled);
	obs_property_set_visible(looping, enabled);

	return true;
}

static bool is_advanced_modified(obs_properties_t *props,
		obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);

	bool enabled = obs_data_get_bool(settings, "advanced");
	obs_property_t *abuf = obs_properties_get(props, "audio_buffer_size");
	obs_property_t *vbuf = obs_properties_get(props, "video_buffer_size");
	obs_property_t *frame_drop = obs_properties_get(props, "frame_drop");
	obs_property_set_visible(abuf, enabled);
	obs_property_set_visible(vbuf, enabled);
	obs_property_set_visible(frame_drop, enabled);

	return true;
}

static obs_properties_t *ffmpeg_source_getproperties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *prop;
	// use this when obs allows non-readonly paths
	prop = obs_properties_add_bool(props, "is_local_file",
			obs_module_text("LocalFile"));

	obs_property_set_modified_callback(prop, is_local_file_modified);

	obs_properties_add_path(props, "local_file",
			obs_module_text("LocalFile"), OBS_PATH_FILE, "*.*",
			NULL);

	obs_properties_add_bool(props, "looping", obs_module_text("Looping"));

	obs_properties_add_text(props, "input",
			obs_module_text("Input"), OBS_TEXT_DEFAULT);

	obs_properties_add_text(props, "input_format",
			obs_module_text("InputFormat"), OBS_TEXT_DEFAULT);

	obs_properties_add_bool(props, "force_scale",
			obs_module_text("ForceFormat"));

	obs_properties_add_bool(props, "hw_decode",
			obs_module_text("HardwareDecode"));

	prop = obs_properties_add_bool(props, "advanced",
			obs_module_text("Advanced"));

	obs_property_set_modified_callback(prop, is_advanced_modified);

	prop = obs_properties_add_int(props, "audio_buffer_size",
			obs_module_text("AudioBufferSize"), 1, 9999, 1);

	obs_property_set_visible(prop, false);

	prop = obs_properties_add_int(props, "video_buffer_size",
			obs_module_text("VideoBufferSize"), 1, 9999, 1);

	obs_property_set_visible(prop, false);

	prop = obs_properties_add_list(props, "frame_drop",
			obs_module_text("FrameDropping"), OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(prop, obs_module_text("DiscardNone"),
			AVDISCARD_NONE);
	obs_property_list_add_int(prop, obs_module_text("DiscardDefault"),
			AVDISCARD_DEFAULT);
	obs_property_list_add_int(prop, obs_module_text("DiscardNonRef"),
			AVDISCARD_NONREF);
	obs_property_list_add_int(prop, obs_module_text("DiscardBiDir"),
			AVDISCARD_BIDIR);
	obs_property_list_add_int(prop, obs_module_text("DiscardNonIntra"),
			AVDISCARD_NONINTRA);
	obs_property_list_add_int(prop, obs_module_text("DiscardNonKey"),
			AVDISCARD_NONKEY);
	obs_property_list_add_int(prop, obs_module_text("DiscardAll"),
			AVDISCARD_ALL);

	obs_property_set_visible(prop, false);

	return props;
}

static void ffmpeg_source_update(void *data, obs_data_t *settings)
{
	struct ffmpeg_source *s = data;

	bool is_local_file = obs_data_get_bool(settings, "is_local_file");
	bool is_advanced = obs_data_get_bool(settings, "advanced");

	bool is_looping;
	char *input;
	char *input_format;

	if (is_local_file) {
		input = (char *)obs_data_get_string(settings, "local_file");
		input_format = NULL;
		is_looping = obs_data_get_bool(settings, "looping");
	} else {
		input = (char *)obs_data_get_string(settings, "input");
		input_format = (char *)obs_data_get_string(settings,
				"input_format");
		is_looping = false;
	}

	s->is_forcing_scale = obs_data_get_bool(settings, "force_scale");
	s->is_hw_decoding = obs_data_get_bool(settings, "hw_decode");

	if (s->demuxer != NULL)
		ff_demuxer_free(s->demuxer);

	s->demuxer = ff_demuxer_init();
	s->demuxer->options.is_hw_decoding = s->is_hw_decoding;
	s->demuxer->options.is_looping = is_looping;

	if (is_advanced) {
		int audio_buffer_size = (int)obs_data_get_int(settings,
				"audio_buffer_size");
		int video_buffer_size = (int)obs_data_get_int(settings,
				"video_buffer_size");
		enum AVDiscard frame_drop =
				(enum AVDiscard)obs_data_get_int(settings,
					"frame_drop");

		if (audio_buffer_size < 1) {
			audio_buffer_size = 1;
			blog(LOG_WARNING, "invalid audio_buffer_size %d",
					audio_buffer_size);
		}
		if (video_buffer_size < 1) {
			video_buffer_size = 1;
			blog(LOG_WARNING, "invalid audio_buffer_size %d",
					audio_buffer_size);
		}
		s->demuxer->options.audio_frame_queue_size = audio_buffer_size;
		s->demuxer->options.video_frame_queue_size = video_buffer_size;

		if (frame_drop < AVDISCARD_NONE || frame_drop > AVDISCARD_ALL) {
			frame_drop = AVDISCARD_NONE;
			blog(LOG_WARNING, "invalid frame_drop %d", frame_drop);
		}
		s->demuxer->options.frame_drop = frame_drop;
	}

	ff_demuxer_set_callbacks(&s->demuxer->video_callbacks,
			video_frame, NULL,
			NULL, NULL, NULL, s);

	ff_demuxer_set_callbacks(&s->demuxer->audio_callbacks,
			audio_frame, NULL,
			NULL, NULL, NULL, s);

	ff_demuxer_open(s->demuxer, input, input_format);
}


static const char *ffmpeg_source_getname(void)
{
	return obs_module_text("FFMpegSource");
}

static void *ffmpeg_source_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);

	struct ffmpeg_source *s = bzalloc(sizeof(struct ffmpeg_source));
	s->source = source;

	ffmpeg_source_update(s, settings);
	return s;
}

static void ffmpeg_source_destroy(void *data)
{
	struct ffmpeg_source *s = data;

	ff_demuxer_free(s->demuxer);

	if (s->sws_ctx != NULL)
		sws_freeContext(s->sws_ctx);
	if (s->sws_data != NULL)
		bfree(s->sws_data);

	bfree(s);
}

struct obs_source_info ffmpeg_source = {
	.id             = "ffmpeg_source",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO,
	.get_name       = ffmpeg_source_getname,
	.create         = ffmpeg_source_create,
	.destroy        = ffmpeg_source_destroy,
	.get_properties = ffmpeg_source_getproperties,
	.update         = ffmpeg_source_update
};
