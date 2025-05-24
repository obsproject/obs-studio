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

#include <obs-module.h>
#include <util/deque.h>
#include <util/threading.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>

#include "obs-ffmpeg-output.h"
#include "obs-ffmpeg-formats.h"
#include "obs-ffmpeg-compat.h"
#include <libavutil/channel_layout.h>
#include <libavutil/mastering_display_metadata.h>

/* ------------------------------------------------------------------------- */

static void ffmpeg_output_set_last_error(struct ffmpeg_data *data, const char *error)
{
	if (data->last_error)
		bfree(data->last_error);

	data->last_error = bstrdup(error);
}

void ffmpeg_log_error(int log_level, struct ffmpeg_data *data, const char *format, ...)
{
	va_list args;
	char out[4096];

	va_start(args, format);
	vsnprintf(out, sizeof(out), format, args);
	va_end(args);

	ffmpeg_output_set_last_error(data, out);

	blog(log_level, "%s", out);
}

static bool new_stream(struct ffmpeg_data *data, AVStream **stream, const AVCodec **codec, enum AVCodecID id,
		       const char *name)
{
	*codec = (!!name && *name) ? avcodec_find_encoder_by_name(name) : avcodec_find_encoder(id);

	if (!*codec) {
		ffmpeg_log_error(LOG_WARNING, data, "Couldn't find encoder '%s'", avcodec_get_name(id));
		return false;
	}

	*stream = avformat_new_stream(data->output, *codec);
	if (!*stream) {
		ffmpeg_log_error(LOG_WARNING, data, "Couldn't create stream for encoder '%s'", avcodec_get_name(id));
		return false;
	}

	(*stream)->id = data->output->nb_streams - 1;
	return true;
}

static bool parse_params(AVCodecContext *context, char **opts)
{
	bool ret = true;

	if (!context || !context->priv_data)
		return true;

	while (*opts) {
		char *opt = *opts;
		char *assign = strchr(opt, '=');

		if (assign) {
			char *name = opt;
			char *value;

			*assign = 0;
			value = assign + 1;

			if (av_opt_set(context, name, value, AV_OPT_SEARCH_CHILDREN)) {
				blog(LOG_WARNING, "Failed to set %s=%s", name, value);
				ret = false;
			}
		}

		opts++;
	}

	return ret;
}

static bool open_video_codec(struct ffmpeg_data *data)
{
	AVCodecContext *const context = data->video_ctx;
	char **opts = strlist_split(data->config.video_settings, ' ', false);
	int ret;

	if (strcmp(data->vcodec->name, "libx264") == 0)
		av_opt_set(context->priv_data, "preset", "veryfast", 0);

	if (opts) {
		// libav requires x264 parameters in a special format which may be non-obvious
		if (!parse_params(context, opts) && strcmp(data->vcodec->name, "libx264") == 0)
			blog(LOG_WARNING,
			     "If you're trying to set x264 parameters, use x264-params=name=value:name=value");
		strlist_free(opts);
	}

	ret = avcodec_open2(context, data->vcodec, NULL);
	if (ret < 0) {
		ffmpeg_log_error(LOG_WARNING, data, "Failed to open video codec: %s", av_err2str(ret));
		return false;
	}

	data->vframe = av_frame_alloc();
	if (!data->vframe) {
		ffmpeg_log_error(LOG_WARNING, data, "Failed to allocate video frame");
		return false;
	}

	data->vframe->format = context->pix_fmt;
	data->vframe->width = context->width;
	data->vframe->height = context->height;
	data->vframe->color_range = data->config.color_range;
	data->vframe->color_primaries = data->config.color_primaries;
	data->vframe->color_trc = data->config.color_trc;
	data->vframe->colorspace = data->config.colorspace;
	data->vframe->chroma_location = determine_chroma_location(context->pix_fmt, data->config.colorspace);

	ret = av_frame_get_buffer(data->vframe, base_get_alignment());
	if (ret < 0) {
		ffmpeg_log_error(LOG_WARNING, data, "Failed to allocate vframe: %s", av_err2str(ret));
		return false;
	}

	avcodec_parameters_from_context(data->video->codecpar, context);

	return true;
}

static bool init_swscale(struct ffmpeg_data *data, AVCodecContext *context)
{
	data->swscale = sws_getContext(data->config.width, data->config.height, data->config.format,
				       data->config.scale_width, data->config.scale_height, context->pix_fmt,
				       SWS_BICUBIC, NULL, NULL, NULL);

	if (!data->swscale) {
		ffmpeg_log_error(LOG_WARNING, data, "Could not initialize swscale");
		return false;
	}

	return true;
}

static bool create_video_stream(struct ffmpeg_data *data)
{
	enum AVPixelFormat closest_format;
	AVCodecContext *context;
	struct obs_video_info ovi;
	const enum AVPixelFormat *pix_fmts = NULL;

	if (!obs_get_video_info(&ovi)) {
		ffmpeg_log_error(LOG_WARNING, data, "No active video");
		return false;
	}

	if (!new_stream(data, &data->video, &data->vcodec, data->output->oformat->video_codec,
			data->config.video_encoder))
		return false;

	context = avcodec_alloc_context3(data->vcodec);
	context->bit_rate = (int64_t)data->config.video_bitrate * 1000;
	context->width = data->config.scale_width;
	context->height = data->config.scale_height;
	context->time_base = (AVRational){ovi.fps_den, ovi.fps_num};
	context->framerate = (AVRational){ovi.fps_num, ovi.fps_den};
	context->gop_size = data->config.gop_size;
	context->color_range = data->config.color_range;
	context->color_primaries = data->config.color_primaries;
	context->color_trc = data->config.color_trc;
	context->colorspace = data->config.colorspace;
	context->thread_count = 0;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 13, 100)
	pix_fmts = data->vcodec->pix_fmts;
#else
	avcodec_get_supported_config(context, data->vcodec, AV_CODEC_CONFIG_PIX_FORMAT, 0, (const void **)&pix_fmts,
				     NULL);
#endif

	closest_format = data->config.format;
	if (pix_fmts) {
		const int has_alpha = closest_format == AV_PIX_FMT_BGRA;
		closest_format = avcodec_find_best_pix_fmt_of_list(pix_fmts, closest_format, has_alpha, NULL);
	}

	context->pix_fmt = closest_format;
	context->chroma_sample_location = determine_chroma_location(closest_format, data->config.colorspace);

	data->video->time_base = context->time_base;
	data->video->avg_frame_rate = (AVRational){ovi.fps_num, ovi.fps_den};

	if (data->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	data->video_ctx = context;

	if (!open_video_codec(data))
		return false;

	const enum AVColorTransferCharacteristic trc = data->config.color_trc;
	const bool pq = trc == AVCOL_TRC_SMPTE2084;
	const bool hlg = trc == AVCOL_TRC_ARIB_STD_B67;
	if (pq || hlg) {
		const int hdr_nominal_peak_level = pq ? (int)obs_get_video_hdr_nominal_peak_level() : (hlg ? 1000 : 0);

		size_t content_size;
		AVContentLightMetadata *const content = av_content_light_metadata_alloc(&content_size);
		content->MaxCLL = hdr_nominal_peak_level;
		content->MaxFALL = hdr_nominal_peak_level;
		av_packet_side_data_add(&data->video->codecpar->coded_side_data,
					&data->video->codecpar->nb_coded_side_data, AV_PKT_DATA_CONTENT_LIGHT_LEVEL,
					(uint8_t *)content, content_size, 0);

		AVMasteringDisplayMetadata *const mastering = av_mastering_display_metadata_alloc();
		mastering->display_primaries[0][0] = av_make_q(17, 25);
		mastering->display_primaries[0][1] = av_make_q(8, 25);
		mastering->display_primaries[1][0] = av_make_q(53, 200);
		mastering->display_primaries[1][1] = av_make_q(69, 100);
		mastering->display_primaries[2][0] = av_make_q(3, 20);
		mastering->display_primaries[2][1] = av_make_q(3, 50);
		mastering->white_point[0] = av_make_q(3127, 10000);
		mastering->white_point[1] = av_make_q(329, 1000);
		mastering->min_luminance = av_make_q(0, 1);
		mastering->max_luminance = av_make_q(hdr_nominal_peak_level, 1);
		mastering->has_primaries = 1;
		mastering->has_luminance = 1;
		av_packet_side_data_add(&data->video->codecpar->coded_side_data,
					&data->video->codecpar->nb_coded_side_data,
					AV_PKT_DATA_MASTERING_DISPLAY_METADATA, (uint8_t *)mastering,
					sizeof(*mastering), 0);
	}

	if (context->pix_fmt != data->config.format || data->config.width != data->config.scale_width ||
	    data->config.height != data->config.scale_height) {

		if (!init_swscale(data, context))
			return false;
	}

	return true;
}

static bool open_audio_codec(struct ffmpeg_data *data, int idx)
{
	AVCodecContext *const context = data->audio_infos[idx].ctx;
	char **opts = strlist_split(data->config.audio_settings, ' ', false);
	int ret;
	int channels;

	if (opts) {
		parse_params(context, opts);
		strlist_free(opts);
	}

	data->aframe[idx] = av_frame_alloc();
	if (!data->aframe[idx]) {
		ffmpeg_log_error(LOG_WARNING, data, "Failed to allocate audio frame");
		return false;
	}

	data->aframe[idx]->format = context->sample_fmt;
	data->aframe[idx]->ch_layout = context->ch_layout;
	channels = context->ch_layout.nb_channels;
	data->aframe[idx]->sample_rate = context->sample_rate;
	context->strict_std_compliance = -2;

	ret = avcodec_open2(context, data->acodec, NULL);
	if (ret < 0) {
		ffmpeg_log_error(LOG_WARNING, data, "Failed to open audio codec: %s", av_err2str(ret));
		return false;
	}

	data->frame_size = context->frame_size ? context->frame_size : 1024;

	ret = av_samples_alloc(data->samples[idx], NULL, channels, data->frame_size, context->sample_fmt, 0);
	if (ret < 0) {
		ffmpeg_log_error(LOG_WARNING, data, "Failed to create audio buffer: %s", av_err2str(ret));
		return false;
	}

	avcodec_parameters_from_context(data->audio_infos[idx].stream->codecpar, context);

	return true;
}

static bool create_audio_stream(struct ffmpeg_data *data, int idx)
{
	AVCodecContext *context;
	AVStream *stream;
	struct obs_audio_info aoi;
	int channels;
	const enum AVSampleFormat *sample_fmts = NULL;

	if (!obs_get_audio_info(&aoi)) {
		ffmpeg_log_error(LOG_WARNING, data, "No active audio");
		return false;
	}

	if (!new_stream(data, &stream, &data->acodec, data->output->oformat->audio_codec, data->config.audio_encoder))
		return false;

	context = avcodec_alloc_context3(data->acodec);
	context->bit_rate = (int64_t)data->config.audio_bitrate * 1000;
	context->time_base = (AVRational){1, aoi.samples_per_sec};
	channels = get_audio_channels(aoi.speakers);

	context->sample_rate = aoi.samples_per_sec;
	av_channel_layout_default(&context->ch_layout, channels);
	if (aoi.speakers == SPEAKERS_4POINT1)
		context->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_4POINT1;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 13, 100)
	sample_fmts = data->acodec->sample_fmts;
#else
	avcodec_get_supported_config(context, data->acodec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0,
				     (const void **)&sample_fmts, NULL);
#endif

	context->sample_fmt = sample_fmts ? sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

	stream->time_base = context->time_base;

	data->audio_samplerate = aoi.samples_per_sec;
	data->audio_format = convert_ffmpeg_sample_format(context->sample_fmt);
	data->audio_planes = get_audio_planes(data->audio_format, aoi.speakers);
	data->audio_size = get_audio_size(data->audio_format, aoi.speakers, 1);

	if (data->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	data->audio_infos[idx].stream = stream;
	data->audio_infos[idx].ctx = context;

	if (data->config.audio_stream_names[idx] && *data->config.audio_stream_names[idx] != '\0')
		av_dict_set(&stream->metadata, "title", data->config.audio_stream_names[idx], 0);

	return open_audio_codec(data, idx);
}

static inline bool init_streams(struct ffmpeg_data *data)
{
	const AVOutputFormat *format = data->output->oformat;

	if (format->video_codec != AV_CODEC_ID_NONE)
		if (!create_video_stream(data))
			return false;

	if (format->audio_codec != AV_CODEC_ID_NONE && data->num_audio_streams) {
		data->audio_infos = calloc(data->num_audio_streams, sizeof(*data->audio_infos));
		for (int i = 0; i < data->num_audio_streams; i++) {
			if (!create_audio_stream(data, i))
				return false;
		}
	}

	return true;
}

static inline bool open_output_file(struct ffmpeg_data *data)
{
	const AVOutputFormat *format = data->output->oformat;
	int ret;

	AVDictionary *dict = NULL;
	if ((ret = av_dict_parse_string(&dict, data->config.muxer_settings, "=", " ", 0))) {
		ffmpeg_log_error(LOG_WARNING, data, "Failed to parse muxer settings: %s\n%s", av_err2str(ret),
				 data->config.muxer_settings);

		av_dict_free(&dict);
		return false;
	}

	if (av_dict_count(dict) > 0) {
		struct dstr str = {0};

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)))
			dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

		blog(LOG_INFO, "Using muxer settings: %s", str.array);
		dstr_free(&str);
	}

	if ((format->flags & AVFMT_NOFILE) == 0) {
		ret = avio_open2(&data->output->pb, data->config.url, AVIO_FLAG_WRITE, NULL, &dict);
		if (ret < 0) {
			ffmpeg_log_error(LOG_WARNING, data, "Couldn't open '%s', %s", data->config.url,
					 av_err2str(ret));
			av_dict_free(&dict);
			return false;
		}
	}

	ret = avformat_write_header(data->output, &dict);
	if (ret < 0) {
		ffmpeg_log_error(LOG_WARNING, data, "Error opening '%s': %s", data->config.url, av_err2str(ret));
		return false;
	}

	if (av_dict_count(dict) > 0) {
		struct dstr str = {0};

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)))
			dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

		blog(LOG_INFO, "Invalid muxer settings: %s", str.array);
		dstr_free(&str);
	}

	av_dict_free(&dict);

	return true;
}

static void close_video(struct ffmpeg_data *data)
{
	avcodec_free_context(&data->video_ctx);
	av_frame_unref(data->vframe);

	// This format for some reason derefs video frame
	// too many times
	if (data->vcodec->id == AV_CODEC_ID_A64_MULTI || data->vcodec->id == AV_CODEC_ID_A64_MULTI5)
		return;

	av_frame_free(&data->vframe);
}

static void close_audio(struct ffmpeg_data *data)
{
	for (int idx = 0; idx < data->num_audio_streams; idx++) {
		for (size_t i = 0; i < MAX_AV_PLANES; i++)
			deque_free(&data->excess_frames[idx][i]);

		if (data->samples[idx][0])
			av_freep(&data->samples[idx][0]);
		if (data->audio_infos[idx].ctx)
			avcodec_free_context(&data->audio_infos[idx].ctx);
		if (data->aframe[idx])
			av_frame_free(&data->aframe[idx]);
	}
}

void ffmpeg_data_free(struct ffmpeg_data *data)
{
	if (data->initialized)
		av_write_trailer(data->output);

	if (data->video)
		close_video(data);
	if (data->audio_infos) {
		close_audio(data);
		free(data->audio_infos);
		data->audio_infos = NULL;
	}

	if (data->output) {
		if ((data->output->oformat->flags & AVFMT_NOFILE) == 0)
			avio_close(data->output->pb);

		avformat_free_context(data->output);
	}

	if (data->last_error)
		bfree(data->last_error);

	memset(data, 0, sizeof(struct ffmpeg_data));
}

static inline const char *safe_str(const char *s)
{
	if (s == NULL)
		return "(NULL)";
	else
		return s;
}

bool ffmpeg_data_init(struct ffmpeg_data *data, struct ffmpeg_cfg *config)
{
	bool is_rtmp = false;

	memset(data, 0, sizeof(struct ffmpeg_data));
	data->config = *config;
	data->num_audio_streams = config->audio_mix_count;
	data->audio_tracks = config->audio_tracks;
	if (!config->url || !*config->url)
		return false;

	avformat_network_init();

	is_rtmp = (astrcmpi_n(config->url, "rtmp://", 7) == 0);

	const AVOutputFormat *output_format = av_guess_format(is_rtmp ? "flv" : data->config.format_name,
							      data->config.url,
							      is_rtmp ? NULL : data->config.format_mime_type);

	if (output_format == NULL) {
		ffmpeg_log_error(LOG_WARNING, data,
				 "Couldn't find matching output format with "
				 "parameters: name=%s, url=%s, mime=%s",
				 safe_str(is_rtmp ? "flv" : data->config.format_name), safe_str(data->config.url),
				 safe_str(is_rtmp ? NULL : data->config.format_mime_type));

		goto fail;
	}

	avformat_alloc_output_context2(&data->output, output_format, NULL, data->config.url);

	if (!data->output) {
		ffmpeg_log_error(LOG_WARNING, data, "Couldn't create avformat context");
		goto fail;
	}

	if (is_rtmp) {
		data->config.audio_encoder_id = AV_CODEC_ID_AAC;
		data->config.video_encoder_id = AV_CODEC_ID_H264;
	}

	if (!init_streams(data))
		goto fail;
	if (!open_output_file(data))
		goto fail;

	av_dump_format(data->output, 0, NULL, 1);

	data->initialized = true;
	return true;

fail:
	blog(LOG_WARNING, "ffmpeg_data_init failed");
	return false;
}

/* ------------------------------------------------------------------------- */

static inline bool stopping(struct ffmpeg_output *output)
{
	return os_atomic_load_bool(&output->stopping);
}

static const char *ffmpeg_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegOutput");
}

static void ffmpeg_log_callback(void *param, int level, const char *format, va_list args)
{
	if (level <= AV_LOG_INFO)
		blogva(LOG_DEBUG, format, args);

	UNUSED_PARAMETER(param);
}

static void *ffmpeg_output_create(obs_data_t *settings, obs_output_t *output)
{
	struct ffmpeg_output *data = bzalloc(sizeof(struct ffmpeg_output));
	pthread_mutex_init_value(&data->write_mutex);
	data->output = output;

	if (pthread_mutex_init(&data->write_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&data->stop_event, OS_EVENT_TYPE_AUTO) != 0)
		goto fail;
	if (os_sem_init(&data->write_sem, 0) != 0)
		goto fail;

	av_log_set_callback(ffmpeg_log_callback);

	UNUSED_PARAMETER(settings);
	return data;

fail:
	pthread_mutex_destroy(&data->write_mutex);
	os_event_destroy(data->stop_event);
	bfree(data);
	return NULL;
}

static void ffmpeg_output_full_stop(void *data);
static void ffmpeg_deactivate(struct ffmpeg_output *output);

static void ffmpeg_output_destroy(void *data)
{
	struct ffmpeg_output *output = data;

	if (output) {
		if (output->connecting)
			pthread_join(output->start_thread, NULL);

		ffmpeg_output_full_stop(output);

		pthread_mutex_destroy(&output->write_mutex);
		os_sem_destroy(output->write_sem);
		os_event_destroy(output->stop_event);
		bfree(data);
	}
}

static inline void copy_data(AVFrame *pic, const struct video_data *frame, int height, enum AVPixelFormat format)
{
	int h_chroma_shift, v_chroma_shift;
	av_pix_fmt_get_chroma_sub_sample(format, &h_chroma_shift, &v_chroma_shift);
	for (int plane = 0; plane < MAX_AV_PLANES; plane++) {
		if (!frame->data[plane])
			continue;

		int frame_rowsize = (int)frame->linesize[plane];
		int pic_rowsize = pic->linesize[plane];
		int bytes = frame_rowsize < pic_rowsize ? frame_rowsize : pic_rowsize;
		int plane_height = height >> (plane ? v_chroma_shift : 0);

		for (int y = 0; y < plane_height; y++) {
			int pos_frame = y * frame_rowsize;
			int pos_pic = y * pic_rowsize;

			memcpy(pic->data[plane] + pos_pic, frame->data[plane] + pos_frame, bytes);
		}
	}
}

static void receive_video(void *param, struct video_data *frame)
{
	struct ffmpeg_output *output = param;
	struct ffmpeg_data *data = &output->ff_data;

	// codec doesn't support video or none configured
	if (!data->video)
		return;

	AVCodecContext *context = data->video_ctx;
	AVPacket *packet = NULL;
	int ret = 0, got_packet;

	if (!output->video_start_ts)
		output->video_start_ts = frame->timestamp;
	if (!data->start_timestamp)
		data->start_timestamp = frame->timestamp;

	ret = av_frame_make_writable(data->vframe);
	if (ret < 0) {
		blog(LOG_WARNING,
		     "receive_video: Error obtaining writable "
		     "AVFrame: %s",
		     av_err2str(ret));
		//FIXME: stop the encode with an error
		return;
	}
	if (!!data->swscale)
		sws_scale(data->swscale, (const uint8_t *const *)frame->data, (const int *)frame->linesize, 0,
			  data->config.height, data->vframe->data, data->vframe->linesize);
	else
		copy_data(data->vframe, frame, context->height, context->pix_fmt);

	packet = av_packet_alloc();

	data->vframe->pts = data->total_frames;
	ret = avcodec_send_frame(context, data->vframe);
	if (ret == 0)
		ret = avcodec_receive_packet(context, packet);

	got_packet = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;

	if (ret < 0) {
		blog(LOG_WARNING,
		     "receive_video: Error encoding "
		     "video: %s",
		     av_err2str(ret));
		//FIXME: stop the encode with an error
		goto fail;
	}

	if (!ret && got_packet && packet->size) {
		packet->pts = rescale_ts(packet->pts, context, data->video->time_base);
		packet->dts = rescale_ts(packet->dts, context, data->video->time_base);
		packet->duration = (int)av_rescale_q(packet->duration, context->time_base, data->video->time_base);

		pthread_mutex_lock(&output->write_mutex);
		da_push_back(output->packets, &packet);
		packet = NULL;
		pthread_mutex_unlock(&output->write_mutex);
		os_sem_post(output->write_sem);
	} else {
		ret = 0;
	}

	if (ret != 0) {
		blog(LOG_WARNING, "receive_video: Error writing video: %s", av_err2str(ret));
		//FIXME: stop the encode with an error
	}

	data->total_frames++;

fail:
	av_packet_free(&packet);
}

static void encode_audio(struct ffmpeg_output *output, int idx, struct AVCodecContext *context, size_t block_size)
{
	struct ffmpeg_data *data = &output->ff_data;

	AVPacket *packet = NULL;
	int ret, got_packet;
	int channels = context->ch_layout.nb_channels;
	size_t total_size = data->frame_size * block_size * channels;

	data->aframe[idx]->nb_samples = data->frame_size;
	data->aframe[idx]->pts =
		av_rescale_q(data->total_samples[idx], (AVRational){1, context->sample_rate}, context->time_base);

	ret = avcodec_fill_audio_frame(data->aframe[idx], channels, context->sample_fmt, data->samples[idx][0],
				       (int)total_size, 1);
	if (ret < 0) {
		blog(LOG_WARNING,
		     "encode_audio: avcodec_fill_audio_frame "
		     "failed: %s",
		     av_err2str(ret));
		//FIXME: stop the encode with an error
		return;
	}

	data->total_samples[idx] += data->frame_size;

	packet = av_packet_alloc();

	ret = avcodec_send_frame(context, data->aframe[idx]);
	if (ret == 0)
		ret = avcodec_receive_packet(context, packet);

	got_packet = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;

	if (ret < 0) {
		blog(LOG_WARNING, "encode_audio: Error encoding audio: %s", av_err2str(ret));
		//FIXME: stop the encode with an error
		goto fail;
	}

	if (!got_packet)
		goto fail;

	packet->pts = rescale_ts(packet->pts, context, data->audio_infos[idx].stream->time_base);
	packet->dts = rescale_ts(packet->dts, context, data->audio_infos[idx].stream->time_base);
	packet->duration =
		(int)av_rescale_q(packet->duration, context->time_base, data->audio_infos[idx].stream->time_base);
	packet->stream_index = data->audio_infos[idx].stream->index;

	pthread_mutex_lock(&output->write_mutex);
	da_push_back(output->packets, &packet);
	pthread_mutex_unlock(&output->write_mutex);
	os_sem_post(output->write_sem);

	return;
fail:
	av_packet_free(&packet);
}

/* Given a bitmask for the selected tracks and the mix index,
 * this returns the stream index which will be passed to the muxer. */
static int get_track_order(int track_config, size_t mix_index)
{
	int position = 0;
	for (size_t i = 0; i < mix_index; i++) {
		if (track_config & 1 << i)
			position++;
	}
	return position;
}

static void receive_audio(void *param, size_t mix_idx, struct audio_data *frame)
{
	struct ffmpeg_output *output = param;
	struct ffmpeg_data *data = &output->ff_data;
	size_t frame_size_bytes;
	struct audio_data in = *frame;
	int track_order;

	// codec doesn't support audio or none configured
	if (!data->audio_infos)
		return;

	/* check that the track was selected */
	if ((data->audio_tracks & (1 << mix_idx)) == 0)
		return;

	/* get track order (first selected, etc ...) */
	track_order = get_track_order(data->audio_tracks, mix_idx);

	AVCodecContext *context = data->audio_infos[track_order].ctx;

	if (!data->start_timestamp && data->video)
		return;

	if (!output->audio_start_ts)
		output->audio_start_ts = in.timestamp;

	frame_size_bytes = (size_t)data->frame_size * data->audio_size;

	for (size_t i = 0; i < data->audio_planes; i++)
		deque_push_back(&data->excess_frames[track_order][i], in.data[i], in.frames * data->audio_size);

	while (data->excess_frames[track_order][0].size >= frame_size_bytes) {
		for (size_t i = 0; i < data->audio_planes; i++)
			deque_pop_front(&data->excess_frames[track_order][i], data->samples[track_order][i],
					frame_size_bytes);

		encode_audio(output, track_order, context, data->audio_size);
	}
}

static uint64_t get_packet_sys_dts(struct ffmpeg_output *output, AVPacket *packet)
{
	struct ffmpeg_data *data = &output->ff_data;
	uint64_t pause_offset = obs_output_get_pause_offset(output->output);
	uint64_t start_ts;

	AVRational time_base;

	if (data->video && data->video->index == packet->stream_index) {
		time_base = data->video->time_base;
		start_ts = output->video_start_ts;
	} else {
		time_base = data->audio_infos[0].stream->time_base;
		start_ts = output->audio_start_ts;
	}

	return start_ts + pause_offset + (uint64_t)av_rescale_q(packet->dts, time_base, (AVRational){1, 1000000000});
}

static int process_packet(struct ffmpeg_output *output)
{
	AVPacket *packet = NULL;
	int ret = 0;

	pthread_mutex_lock(&output->write_mutex);
	if (output->packets.num) {
		packet = output->packets.array[0];
		da_erase(output->packets, 0);
	}
	pthread_mutex_unlock(&output->write_mutex);

	if (!packet)
		return 0;

	/*blog(LOG_DEBUG, "size = %d, flags = %lX, stream = %d, "
			"packets queued: %lu",
			packet.size, packet.flags,
			packet.stream_index, output->packets.num);*/

	if (stopping(output)) {
		uint64_t sys_ts = get_packet_sys_dts(output, packet);
		if (sys_ts >= output->stop_ts) {
			ret = 0;
			goto end;
		}
	}

	output->total_bytes += packet->size;

	ret = av_interleaved_write_frame(output->ff_data.output, packet);
	if (ret < 0) {
		ffmpeg_log_error(LOG_WARNING, &output->ff_data, "process_packet: Error writing packet: %s",
				 av_err2str(ret));
	}

end:
	av_packet_free(&packet);
	return ret;
}

static void *write_thread(void *data)
{
	struct ffmpeg_output *output = data;

	while (os_sem_wait(output->write_sem) == 0) {
		/* check to see if shutting down */
		if (os_event_try(output->stop_event) == 0)
			break;

		int ret = process_packet(output);
		if (ret != 0) {
			int code = OBS_OUTPUT_ERROR;

			pthread_detach(output->write_thread);
			output->write_thread_active = false;

			if (ret == -ENOSPC)
				code = OBS_OUTPUT_NO_SPACE;

			obs_output_signal_stop(output->output, code);
			ffmpeg_deactivate(output);
			break;
		}
	}

	output->active = false;
	return NULL;
}

static inline const char *get_string_or_null(obs_data_t *settings, const char *name)
{
	const char *value = obs_data_get_string(settings, name);
	if (!value || !strlen(value))
		return NULL;
	return value;
}

static int get_audio_mix_count(int audio_mix_mask)
{
	int mix_count = 0;
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		if ((audio_mix_mask & (1 << i)) != 0) {
			mix_count++;
		}
	}

	return mix_count;
}

static bool try_connect(struct ffmpeg_output *output)
{
	video_t *video = obs_output_video(output->output);
	const struct video_output_info *voi = video_output_get_info(video);
	struct ffmpeg_cfg config;
	obs_data_t *settings;
	bool success;
	int ret;

	settings = obs_output_get_settings(output->output);

	obs_data_set_default_int(settings, "gop_size", 120);

	config.url = obs_data_get_string(settings, "url");
	config.format_name = get_string_or_null(settings, "format_name");
	config.format_mime_type = get_string_or_null(settings, "format_mime_type");
	config.muxer_settings = obs_data_get_string(settings, "muxer_settings");
	config.video_bitrate = (int)obs_data_get_int(settings, "video_bitrate");
	config.audio_bitrate = (int)obs_data_get_int(settings, "audio_bitrate");
	config.gop_size = (int)obs_data_get_int(settings, "gop_size");
	config.video_encoder = get_string_or_null(settings, "video_encoder");
	config.video_encoder_id = (int)obs_data_get_int(settings, "video_encoder_id");
	config.audio_encoder = get_string_or_null(settings, "audio_encoder");
	config.audio_encoder_id = (int)obs_data_get_int(settings, "audio_encoder_id");
	config.video_settings = obs_data_get_string(settings, "video_settings");
	config.audio_settings = obs_data_get_string(settings, "audio_settings");
	config.scale_width = (int)obs_data_get_int(settings, "scale_width");
	config.scale_height = (int)obs_data_get_int(settings, "scale_height");
	config.width = (int)obs_output_get_width(output->output);
	config.height = (int)obs_output_get_height(output->output);
	config.format = obs_to_ffmpeg_video_format(video_output_get_format(video));
	config.audio_tracks = (int)obs_output_get_mixers(output->output);
	config.audio_mix_count = get_audio_mix_count(config.audio_tracks);

	config.color_range = voi->range == VIDEO_RANGE_FULL ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
	config.colorspace = format_is_yuv(voi->format) ? AVCOL_SPC_BT709 : AVCOL_SPC_RGB;
	switch (voi->colorspace) {
	case VIDEO_CS_601:
		config.color_primaries = AVCOL_PRI_SMPTE170M;
		config.color_trc = AVCOL_TRC_SMPTE170M;
		config.colorspace = AVCOL_SPC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		config.color_primaries = AVCOL_PRI_BT709;
		config.color_trc = AVCOL_TRC_BT709;
		config.colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_SRGB:
		config.color_primaries = AVCOL_PRI_BT709;
		config.color_trc = AVCOL_TRC_IEC61966_2_1;
		config.colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_2100_PQ:
		config.color_primaries = AVCOL_PRI_BT2020;
		config.color_trc = AVCOL_TRC_SMPTE2084;
		config.colorspace = AVCOL_SPC_BT2020_NCL;
		break;
	case VIDEO_CS_2100_HLG:
		config.color_primaries = AVCOL_PRI_BT2020;
		config.color_trc = AVCOL_TRC_ARIB_STD_B67;
		config.colorspace = AVCOL_SPC_BT2020_NCL;
		break;
	}

	if (config.format == AV_PIX_FMT_NONE) {
		blog(LOG_DEBUG, "invalid pixel format used for FFmpeg output");
		return false;
	}

	if (!config.scale_width)
		config.scale_width = config.width;
	if (!config.scale_height)
		config.scale_height = config.height;

	obs_data_array_t *audioNames = obs_data_get_array(settings, "audio_names");
	if (audioNames) {
		for (size_t i = 0, idx = 0; i < MAX_AUDIO_MIXES; i++) {
			if ((config.audio_tracks & (1 << i)) == 0)
				continue;

			obs_data_t *item_data = obs_data_array_item(audioNames, i);
			config.audio_stream_names[idx] = obs_data_get_string(item_data, "name");
			obs_data_release(item_data);

			idx++;
		}
		obs_data_array_release(audioNames);
	} else {
		for (int idx = 0; idx < config.audio_mix_count; idx++)
			config.audio_stream_names[idx] = NULL;
	}

	success = ffmpeg_data_init(&output->ff_data, &config);
	obs_data_release(settings);

	if (!success) {
		if (output->ff_data.last_error) {
			obs_output_set_last_error(output->output, output->ff_data.last_error);
		}
		ffmpeg_data_free(&output->ff_data);
		return false;
	}

	struct audio_convert_info aci = {.format = output->ff_data.audio_format};

	output->active = true;

	if (!obs_output_can_begin_data_capture(output->output, 0))
		return false;

	ret = pthread_create(&output->write_thread, NULL, write_thread, output);
	if (ret != 0) {
		ffmpeg_log_error(LOG_WARNING, &output->ff_data,
				 "ffmpeg_output_start: failed to create write "
				 "thread.");
		ffmpeg_output_full_stop(output);
		return false;
	}

	obs_output_set_video_conversion(output->output, NULL);
	obs_output_set_audio_conversion(output->output, &aci);
	obs_output_begin_data_capture(output->output, 0);
	output->write_thread_active = true;
	return true;
}

static void *start_thread(void *data)
{
	struct ffmpeg_output *output = data;

	if (!try_connect(output))
		obs_output_signal_stop(output->output, OBS_OUTPUT_CONNECT_FAILED);

	output->connecting = false;
	return NULL;
}

static bool ffmpeg_output_start(void *data)
{
	struct ffmpeg_output *output = data;
	int ret;

	if (output->connecting)
		return false;

	os_atomic_set_bool(&output->stopping, false);
	output->audio_start_ts = 0;
	output->video_start_ts = 0;
	output->total_bytes = 0;

	ret = pthread_create(&output->start_thread, NULL, start_thread, output);
	return (output->connecting = (ret == 0));
}

static void ffmpeg_output_full_stop(void *data)
{
	struct ffmpeg_output *output = data;

	if (output->active) {
		obs_output_end_data_capture(output->output);
		ffmpeg_deactivate(output);
	}
}

static void ffmpeg_output_stop(void *data, uint64_t ts)
{
	struct ffmpeg_output *output = data;

	if (output->active) {
		if (ts > 0) {
			output->stop_ts = ts;
			os_atomic_set_bool(&output->stopping, true);
		}

		ffmpeg_output_full_stop(output);
	}
}

static void ffmpeg_deactivate(struct ffmpeg_output *output)
{
	if (output->write_thread_active) {
		os_event_signal(output->stop_event);
		os_sem_post(output->write_sem);
		pthread_join(output->write_thread, NULL);
		output->write_thread_active = false;
	}

	pthread_mutex_lock(&output->write_mutex);

	for (size_t i = 0; i < output->packets.num; i++)
		av_packet_free(output->packets.array + i);
	da_free(output->packets);

	pthread_mutex_unlock(&output->write_mutex);

	ffmpeg_data_free(&output->ff_data);
}

static uint64_t ffmpeg_output_total_bytes(void *data)
{
	struct ffmpeg_output *output = data;
	return output->total_bytes;
}

struct obs_output_info ffmpeg_output = {
	.id = "ffmpeg_output",
	.flags = OBS_OUTPUT_AUDIO | OBS_OUTPUT_VIDEO | OBS_OUTPUT_MULTI_TRACK | OBS_OUTPUT_CAN_PAUSE,
	.get_name = ffmpeg_output_getname,
	.create = ffmpeg_output_create,
	.destroy = ffmpeg_output_destroy,
	.start = ffmpeg_output_start,
	.stop = ffmpeg_output_stop,
	.raw_video = receive_video,
	.raw_audio2 = receive_audio,
	.get_total_bytes = ffmpeg_output_total_bytes,
};
