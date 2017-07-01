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

#include <obs-module.h>
#include <util/circlebuf.h>
#include <util/threading.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>

#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "obs-ffmpeg-formats.h"
#include "closest-pixel-format.h"
#include "obs-ffmpeg-compat.h"

struct ffmpeg_cfg {
	const char         *url;
	const char         *format_name;
	const char         *format_mime_type;
	const char         *muxer_settings;
	int                gop_size;
	int                video_bitrate;
	int                audio_bitrate;
	const char         *video_encoder;
	int                video_encoder_id;
	const char         *audio_encoder;
	int                audio_encoder_id;
	const char         *video_settings;
	const char         *audio_settings;
	enum AVPixelFormat format;
	enum AVColorRange  color_range;
	enum AVColorSpace  color_space;
	int                scale_width;
	int                scale_height;
	int                width;
	int                height;
};

struct ffmpeg_data {
	AVStream           *video;
	AVStream           *audio;
	AVCodec            *acodec;
	AVCodec            *vcodec;
	AVFormatContext    *output;
	struct SwsContext  *swscale;

	int64_t            total_frames;
	AVPicture          dst_picture;
	AVFrame            *vframe;
	int                frame_size;

	uint64_t           start_timestamp;

	int64_t            total_samples;
	uint32_t           audio_samplerate;
	enum audio_format  audio_format;
	size_t             audio_planes;
	size_t             audio_size;
	struct circlebuf   excess_frames[MAX_AV_PLANES];
	uint8_t            *samples[MAX_AV_PLANES];
	AVFrame            *aframe;

	struct ffmpeg_cfg  config;

	bool               initialized;
};

struct ffmpeg_output {
	obs_output_t       *output;
	volatile bool      active;
	struct ffmpeg_data ff_data;

	bool               connecting;
	pthread_t          start_thread;

	uint64_t           total_bytes;

	uint64_t           audio_start_ts;
	uint64_t           video_start_ts;
	uint64_t           stop_ts;
	volatile bool      stopping;

	bool               write_thread_active;
	pthread_mutex_t    write_mutex;
	pthread_t          write_thread;
	os_sem_t           *write_sem;
	os_event_t         *stop_event;

	DARRAY(AVPacket)   packets;
};

/* ------------------------------------------------------------------------- */

static bool new_stream(struct ffmpeg_data *data, AVStream **stream,
		AVCodec **codec, enum AVCodecID id, const char *name)
{
	*codec = (!!name && *name) ?
		avcodec_find_encoder_by_name(name) :
		avcodec_find_encoder(id);

	if (!*codec) {
		blog(LOG_WARNING, "Couldn't find encoder '%s'",
				avcodec_get_name(id));
		return false;
	}

	*stream = avformat_new_stream(data->output, *codec);
	if (!*stream) {
		blog(LOG_WARNING, "Couldn't create stream for encoder '%s'",
				avcodec_get_name(id));
		return false;
	}

	(*stream)->id = data->output->nb_streams-1;
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
			value = assign+1;

			if (av_opt_set(context->priv_data, name, value, 0)) {
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
	AVCodecContext *context = data->video->codec;
	char **opts = strlist_split(data->config.video_settings, ' ', false);
	int ret;

	if (strcmp(data->vcodec->name, "libx264") == 0)
		av_opt_set(context->priv_data, "preset", "veryfast", 0);

	if (opts) {
		// libav requires x264 parameters in a special format which may be non-obvious
		if (!parse_params(context, opts) && strcmp(data->vcodec->name, "libx264") == 0)
			blog(LOG_WARNING, "If you're trying to set x264 parameters, use x264-params=name=value:name=value");
		strlist_free(opts);
	}

	ret = avcodec_open2(context, data->vcodec, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to open video codec: %s",
				av_err2str(ret));
		return false;
	}

	data->vframe = av_frame_alloc();
	if (!data->vframe) {
		blog(LOG_WARNING, "Failed to allocate video frame");
		return false;
	}

	data->vframe->format = context->pix_fmt;
	data->vframe->width  = context->width;
	data->vframe->height = context->height;
	data->vframe->colorspace = data->config.color_space;
	data->vframe->color_range = data->config.color_range;

	ret = avpicture_alloc(&data->dst_picture, context->pix_fmt,
			context->width, context->height);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to allocate dst_picture: %s",
				av_err2str(ret));
		return false;
	}

	*((AVPicture*)data->vframe) = data->dst_picture;
	return true;
}

static bool init_swscale(struct ffmpeg_data *data, AVCodecContext *context)
{
	data->swscale = sws_getContext(
			data->config.width, data->config.height,
			data->config.format,
			data->config.scale_width, data->config.scale_height,
			context->pix_fmt,
			SWS_BICUBIC, NULL, NULL, NULL);

	if (!data->swscale) {
		blog(LOG_WARNING, "Could not initialize swscale");
		return false;
	}

	return true;
}

static bool create_video_stream(struct ffmpeg_data *data)
{
	enum AVPixelFormat closest_format;
	AVCodecContext *context;
	struct obs_video_info ovi;

	if (!obs_get_video_info(&ovi)) {
		blog(LOG_WARNING, "No active video");
		return false;
	}

	if (!new_stream(data, &data->video, &data->vcodec,
				data->output->oformat->video_codec,
				data->config.video_encoder))
		return false;

	closest_format = get_closest_format(data->config.format,
			data->vcodec->pix_fmts);

	context                 = data->video->codec;
	context->bit_rate       = data->config.video_bitrate * 1000;
	context->width          = data->config.scale_width;
	context->height         = data->config.scale_height;
	context->time_base      = (AVRational){ ovi.fps_den, ovi.fps_num };
	context->gop_size       = data->config.gop_size;
	context->pix_fmt        = closest_format;
	context->colorspace     = data->config.color_space;
	context->color_range    = data->config.color_range;
	context->thread_count   = 0;

	data->video->time_base = context->time_base;

	if (data->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_HEADER;

	if (!open_video_codec(data))
		return false;

	if (context->pix_fmt    != data->config.format ||
	    data->config.width  != data->config.scale_width ||
	    data->config.height != data->config.scale_height) {

		if (!init_swscale(data, context))
			return false;
	}

	return true;
}

static bool open_audio_codec(struct ffmpeg_data *data)
{
	AVCodecContext *context = data->audio->codec;
	char **opts = strlist_split(data->config.audio_settings, ' ', false);
	int ret;

	if (opts) {
		parse_params(context, opts);
		strlist_free(opts);
	}

	data->aframe = av_frame_alloc();
	if (!data->aframe) {
		blog(LOG_WARNING, "Failed to allocate audio frame");
		return false;
	}

	context->strict_std_compliance = -2;

	ret = avcodec_open2(context, data->acodec, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to open audio codec: %s",
				av_err2str(ret));
		return false;
	}

	data->frame_size = context->frame_size ? context->frame_size : 1024;

	ret = av_samples_alloc(data->samples, NULL, context->channels,
			data->frame_size, context->sample_fmt, 0);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to create audio buffer: %s",
		                av_err2str(ret));
		return false;
	}

	return true;
}

static bool create_audio_stream(struct ffmpeg_data *data)
{
	AVCodecContext *context;
	struct obs_audio_info aoi;

	if (!obs_get_audio_info(&aoi)) {
		blog(LOG_WARNING, "No active audio");
		return false;
	}

	if (!new_stream(data, &data->audio, &data->acodec,
				data->output->oformat->audio_codec,
				data->config.audio_encoder))
		return false;

	context              = data->audio->codec;
	context->bit_rate    = data->config.audio_bitrate * 1000;
	context->time_base   = (AVRational){ 1, aoi.samples_per_sec };
	context->channels    = get_audio_channels(aoi.speakers);
	context->sample_rate = aoi.samples_per_sec;
	context->channel_layout =
			av_get_default_channel_layout(context->channels);
	context->sample_fmt  = data->acodec->sample_fmts ?
		data->acodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

	data->audio->time_base = context->time_base;

	data->audio_samplerate = aoi.samples_per_sec;
	data->audio_format = convert_ffmpeg_sample_format(context->sample_fmt);
	data->audio_planes = get_audio_planes(data->audio_format, aoi.speakers);
	data->audio_size = get_audio_size(data->audio_format, aoi.speakers, 1);

	if (data->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return open_audio_codec(data);
}

static inline bool init_streams(struct ffmpeg_data *data)
{
	AVOutputFormat *format = data->output->oformat;

	if (format->video_codec != AV_CODEC_ID_NONE)
		if (!create_video_stream(data))
			return false;

	if (format->audio_codec != AV_CODEC_ID_NONE)
		if (!create_audio_stream(data))
			return false;

	return true;
}

static inline bool open_output_file(struct ffmpeg_data *data)
{
	AVOutputFormat *format = data->output->oformat;
	int ret;

	if ((format->flags & AVFMT_NOFILE) == 0) {
		ret = avio_open(&data->output->pb, data->config.url,
				AVIO_FLAG_WRITE);
		if (ret < 0) {
			blog(LOG_WARNING, "Couldn't open '%s', %s",
					data->config.url, av_err2str(ret));
			return false;
		}
	}

	strncpy(data->output->filename, data->config.url,
			sizeof(data->output->filename));
	data->output->filename[sizeof(data->output->filename) - 1] = 0;

	AVDictionary *dict = NULL;
	if ((ret = av_dict_parse_string(&dict, data->config.muxer_settings,
				"=", " ", 0))) {
		blog(LOG_WARNING, "Failed to parse muxer settings: %s\n%s",
				av_err2str(ret), data->config.muxer_settings);

		av_dict_free(&dict);
		return false;
	}

	if (av_dict_count(dict) > 0) {
		struct dstr str = {0};

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry,
						AV_DICT_IGNORE_SUFFIX)))
			dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

		blog(LOG_INFO, "Using muxer settings:%s", str.array);
		dstr_free(&str);
	}

	ret = avformat_write_header(data->output, &dict);
	if (ret < 0) {
		blog(LOG_WARNING, "Error opening '%s': %s",
				data->config.url, av_err2str(ret));
		return false;
	}

	av_dict_free(&dict);

	return true;
}

static void close_video(struct ffmpeg_data *data)
{
	avcodec_close(data->video->codec);
	avpicture_free(&data->dst_picture);

	// This format for some reason derefs video frame
	// too many times
	if (data->vcodec->id == AV_CODEC_ID_A64_MULTI ||
	    data->vcodec->id == AV_CODEC_ID_A64_MULTI5)
		return;

	av_frame_free(&data->vframe);
}

static void close_audio(struct ffmpeg_data *data)
{
	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		circlebuf_free(&data->excess_frames[i]);

	av_freep(&data->samples[0]);
	avcodec_close(data->audio->codec);
	av_frame_free(&data->aframe);
}

static void ffmpeg_data_free(struct ffmpeg_data *data)
{
	if (data->initialized)
		av_write_trailer(data->output);

	if (data->video)
		close_video(data);
	if (data->audio)
		close_audio(data);

	if (data->output) {
		if ((data->output->oformat->flags & AVFMT_NOFILE) == 0)
			avio_close(data->output->pb);

		avformat_free_context(data->output);
	}

	memset(data, 0, sizeof(struct ffmpeg_data));
}

static inline const char *safe_str(const char *s)
{
	if (s == NULL)
		return "(NULL)";
	else
		return s;
}

static enum AVCodecID get_codec_id(const char *name, int id)
{
	AVCodec *codec;

	if (id != 0)
		return (enum AVCodecID)id;

	if (!name || !*name)
		return AV_CODEC_ID_NONE;

	codec = avcodec_find_encoder_by_name(name);
	if (!codec)
		return AV_CODEC_ID_NONE;

	return codec->id;
}

static void set_encoder_ids(struct ffmpeg_data *data)
{
	data->output->oformat->video_codec = get_codec_id(
			data->config.video_encoder,
			data->config.video_encoder_id);

	data->output->oformat->audio_codec = get_codec_id(
			data->config.audio_encoder,
			data->config.audio_encoder_id);
}

static bool ffmpeg_data_init(struct ffmpeg_data *data,
		struct ffmpeg_cfg *config)
{
	bool is_rtmp = false;

	memset(data, 0, sizeof(struct ffmpeg_data));
	data->config = *config;

	if (!config->url || !*config->url)
		return false;

	av_register_all();
	avformat_network_init();

	is_rtmp = (astrcmpi_n(config->url, "rtmp://", 7) == 0);

	AVOutputFormat *output_format = av_guess_format(
			is_rtmp ? "flv" : data->config.format_name,
			data->config.url,
			is_rtmp ? NULL : data->config.format_mime_type);

	if (output_format == NULL) {
		blog(LOG_WARNING, "Couldn't find matching output format with "
				" parameters: name=%s, url=%s, mime=%s",
				safe_str(is_rtmp ?
					"flv" :	data->config.format_name),
				safe_str(data->config.url),
				safe_str(is_rtmp ?
					NULL : data->config.format_mime_type));
		goto fail;
	}

	avformat_alloc_output_context2(&data->output, output_format,
			NULL, NULL);

	if (is_rtmp) {
		data->output->oformat->video_codec = AV_CODEC_ID_H264;
		data->output->oformat->audio_codec = AV_CODEC_ID_AAC;
	} else {
		if (data->config.format_name)
			set_encoder_ids(data);
	}

	if (!data->output) {
		blog(LOG_WARNING, "Couldn't create avformat context");
		goto fail;
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
	ffmpeg_data_free(data);
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

static void ffmpeg_log_callback(void *param, int level, const char *format,
		va_list args)
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

static inline void copy_data(AVPicture *pic, const struct video_data *frame,
		int height, enum AVPixelFormat format)
{
	int h_chroma_shift, v_chroma_shift;
	av_pix_fmt_get_chroma_sub_sample(format, &h_chroma_shift, &v_chroma_shift);
	for (int plane = 0; plane < MAX_AV_PLANES; plane++) {
		if (!frame->data[plane])
			continue;

		int frame_rowsize = (int)frame->linesize[plane];
		int pic_rowsize   = pic->linesize[plane];
		int bytes = frame_rowsize < pic_rowsize ?
			frame_rowsize : pic_rowsize;
		int plane_height = height >> (plane ? v_chroma_shift : 0);

		for (int y = 0; y < plane_height; y++) {
			int pos_frame = y * frame_rowsize;
			int pos_pic   = y * pic_rowsize;

			memcpy(pic->data[plane] + pos_pic,
			       frame->data[plane] + pos_frame,
			       bytes);
		}
	}
}

static void receive_video(void *param, struct video_data *frame)
{
	struct ffmpeg_output *output = param;
	struct ffmpeg_data   *data   = &output->ff_data;

	// codec doesn't support video or none configured
	if (!data->video)
		return;

	AVCodecContext *context = data->video->codec;
	AVPacket packet = {0};
	int ret = 0, got_packet;

	av_init_packet(&packet);

	if (!output->video_start_ts)
		output->video_start_ts = frame->timestamp;
	if (!data->start_timestamp)
		data->start_timestamp = frame->timestamp;

	if (!!data->swscale)
		sws_scale(data->swscale, (const uint8_t *const *)frame->data,
				(const int*)frame->linesize,
				0, data->config.height, data->dst_picture.data,
				data->dst_picture.linesize);
	else
		copy_data(&data->dst_picture, frame, context->height, context->pix_fmt);

	if (data->output->flags & AVFMT_RAWPICTURE) {
		packet.flags        |= AV_PKT_FLAG_KEY;
		packet.stream_index  = data->video->index;
		packet.data          = data->dst_picture.data[0];
		packet.size          = sizeof(AVPicture);

		pthread_mutex_lock(&output->write_mutex);
		da_push_back(output->packets, &packet);
		pthread_mutex_unlock(&output->write_mutex);
		os_sem_post(output->write_sem);

	} else {
		data->vframe->pts = data->total_frames;
		ret = avcodec_encode_video2(context, &packet, data->vframe,
				&got_packet);
		if (ret < 0) {
			blog(LOG_WARNING, "receive_video: Error encoding "
			                  "video: %s", av_err2str(ret));
			return;
		}

		if (!ret && got_packet && packet.size) {
			packet.pts = rescale_ts(packet.pts, context,
					data->video->time_base);
			packet.dts = rescale_ts(packet.dts, context,
					data->video->time_base);
			packet.duration = (int)av_rescale_q(packet.duration,
					context->time_base,
					data->video->time_base);

			pthread_mutex_lock(&output->write_mutex);
			da_push_back(output->packets, &packet);
			pthread_mutex_unlock(&output->write_mutex);
			os_sem_post(output->write_sem);
		} else {
			ret = 0;
		}
	}

	if (ret != 0) {
		blog(LOG_WARNING, "receive_video: Error writing video: %s",
				av_err2str(ret));
	}

	data->total_frames++;
}

static void encode_audio(struct ffmpeg_output *output,
		struct AVCodecContext *context, size_t block_size)
{
	struct ffmpeg_data *data = &output->ff_data;

	AVPacket packet = {0};
	int ret, got_packet;
	size_t total_size = data->frame_size * block_size * context->channels;

	data->aframe->nb_samples = data->frame_size;
	data->aframe->pts = av_rescale_q(data->total_samples,
			(AVRational){1, context->sample_rate},
			context->time_base);

	ret = avcodec_fill_audio_frame(data->aframe, context->channels,
			context->sample_fmt, data->samples[0],
			(int)total_size, 1);
	if (ret < 0) {
		blog(LOG_WARNING, "encode_audio: avcodec_fill_audio_frame "
		                  "failed: %s", av_err2str(ret));
		return;
	}

	data->total_samples += data->frame_size;

	ret = avcodec_encode_audio2(context, &packet, data->aframe,
			&got_packet);
	if (ret < 0) {
		blog(LOG_WARNING, "encode_audio: Error encoding audio: %s",
				av_err2str(ret));
		return;
	}

	if (!got_packet)
		return;

	packet.pts = rescale_ts(packet.pts, context, data->audio->time_base);
	packet.dts = rescale_ts(packet.dts, context, data->audio->time_base);
	packet.duration = (int)av_rescale_q(packet.duration, context->time_base,
			data->audio->time_base);
	packet.stream_index = data->audio->index;

	pthread_mutex_lock(&output->write_mutex);
	da_push_back(output->packets, &packet);
	pthread_mutex_unlock(&output->write_mutex);
	os_sem_post(output->write_sem);
}

static bool prepare_audio(struct ffmpeg_data *data,
		const struct audio_data *frame, struct audio_data *output)
{
	*output = *frame;

	if (frame->timestamp < data->start_timestamp) {
		uint64_t duration = (uint64_t)frame->frames * 1000000000 /
			(uint64_t)data->audio_samplerate;
		uint64_t end_ts = (frame->timestamp + duration);
		uint64_t cutoff;

		if (end_ts <= data->start_timestamp)
			return false;

		cutoff = data->start_timestamp - frame->timestamp;
		output->timestamp += cutoff;

		cutoff = cutoff * (uint64_t)data->audio_samplerate /
			1000000000;

		for (size_t i = 0; i < data->audio_planes; i++)
			output->data[i] += data->audio_size * (uint32_t)cutoff;
		output->frames -= (uint32_t)cutoff;
	}

	return true;
}

static void receive_audio(void *param, struct audio_data *frame)
{
	struct ffmpeg_output *output = param;
	struct ffmpeg_data   *data   = &output->ff_data;
	size_t frame_size_bytes;
	struct audio_data in;

	// codec doesn't support audio or none configured
	if (!data->audio)
		return;

	AVCodecContext *context = data->audio->codec;

	if (!data->start_timestamp)
		return;
	if (!prepare_audio(data, frame, &in))
		return;

	if (!output->audio_start_ts)
		output->audio_start_ts = in.timestamp;

	frame_size_bytes = (size_t)data->frame_size * data->audio_size;

	for (size_t i = 0; i < data->audio_planes; i++)
		circlebuf_push_back(&data->excess_frames[i], in.data[i],
				in.frames * data->audio_size);

	while (data->excess_frames[0].size >= frame_size_bytes) {
		for (size_t i = 0; i < data->audio_planes; i++)
			circlebuf_pop_front(&data->excess_frames[i],
					data->samples[i], frame_size_bytes);

		encode_audio(output, context, data->audio_size);
	}
}

static uint64_t get_packet_sys_dts(struct ffmpeg_output *output,
		AVPacket *packet)
{
	struct ffmpeg_data *data = &output->ff_data;
	uint64_t start_ts;

	AVRational time_base;

	if (data->video && data->video->index == packet->stream_index) {
		time_base = data->video->time_base;
		start_ts = output->video_start_ts;
	} else {
		time_base = data->audio->time_base;
		start_ts = output->audio_start_ts;
	}

	return start_ts + (uint64_t)av_rescale_q(packet->dts,
			time_base, (AVRational){1, 1000000000});
}

static int process_packet(struct ffmpeg_output *output)
{
	AVPacket packet;
	bool new_packet = false;
	int ret;

	pthread_mutex_lock(&output->write_mutex);
	if (output->packets.num) {
		packet = output->packets.array[0];
		da_erase(output->packets, 0);
		new_packet = true;
	}
	pthread_mutex_unlock(&output->write_mutex);

	if (!new_packet)
		return 0;

	/*blog(LOG_DEBUG, "size = %d, flags = %lX, stream = %d, "
			"packets queued: %lu",
			packet.size, packet.flags,
			packet.stream_index, output->packets.num);*/

	if (stopping(output)) {
		uint64_t sys_ts = get_packet_sys_dts(output, &packet);
		if (sys_ts >= output->stop_ts) {
			ffmpeg_output_full_stop(output);
			return 0;
		}
	}

	output->total_bytes += packet.size;

	ret = av_interleaved_write_frame(output->ff_data.output, &packet);
	if (ret < 0) {
		av_free_packet(&packet);
		blog(LOG_WARNING, "receive_audio: Error writing packet: %s",
				av_err2str(ret));
		return ret;
	}

	return 0;
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

static inline const char *get_string_or_null(obs_data_t *settings,
		const char *name)
{
	const char *value = obs_data_get_string(settings, name);
	if (!value || !strlen(value))
		return NULL;
	return value;
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
	config.format_mime_type = get_string_or_null(settings,
			"format_mime_type");
	config.muxer_settings = obs_data_get_string(settings, "muxer_settings");
	config.video_bitrate = (int)obs_data_get_int(settings, "video_bitrate");
	config.audio_bitrate = (int)obs_data_get_int(settings, "audio_bitrate");
	config.gop_size = (int)obs_data_get_int(settings, "gop_size");
	config.video_encoder = get_string_or_null(settings, "video_encoder");
	config.video_encoder_id = (int)obs_data_get_int(settings,
			"video_encoder_id");
	config.audio_encoder = get_string_or_null(settings, "audio_encoder");
	config.audio_encoder_id = (int)obs_data_get_int(settings,
			"audio_encoder_id");
	config.video_settings = obs_data_get_string(settings, "video_settings");
	config.audio_settings = obs_data_get_string(settings, "audio_settings");
	config.scale_width = (int)obs_data_get_int(settings, "scale_width");
	config.scale_height = (int)obs_data_get_int(settings, "scale_height");
	config.width  = (int)obs_output_get_width(output->output);
	config.height = (int)obs_output_get_height(output->output);
	config.format = obs_to_ffmpeg_video_format(
			video_output_get_format(video));

	if (format_is_yuv(voi->format)) {
		config.color_range = voi->range == VIDEO_RANGE_FULL ?
			AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
		config.color_space = voi->colorspace == VIDEO_CS_709 ?
			AVCOL_SPC_BT709 : AVCOL_SPC_BT470BG;
	} else {
		config.color_range = AVCOL_RANGE_UNSPECIFIED;
		config.color_space = AVCOL_SPC_RGB;
	}

	if (config.format == AV_PIX_FMT_NONE) {
		blog(LOG_DEBUG, "invalid pixel format used for FFmpeg output");
		return false;
	}

	if (!config.scale_width)
		config.scale_width = config.width;
	if (!config.scale_height)
		config.scale_height = config.height;

	success = ffmpeg_data_init(&output->ff_data, &config);
	obs_data_release(settings);

	if (!success)
		return false;

	struct audio_convert_info aci = {
		.format = output->ff_data.audio_format
	};

	output->active = true;

	if (!obs_output_can_begin_data_capture(output->output, 0))
		return false;

	ret = pthread_create(&output->write_thread, NULL, write_thread, output);
	if (ret != 0) {
		blog(LOG_WARNING, "ffmpeg_output_start: failed to create write "
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
		obs_output_signal_stop(output->output,
				OBS_OUTPUT_CONNECT_FAILED);

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
		if (ts == 0) {
			ffmpeg_output_full_stop(output);
		} else {
			os_atomic_set_bool(&output->stopping, true);
			output->stop_ts = ts;
		}
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
		av_free_packet(output->packets.array+i);
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
	.id        = "ffmpeg_output",
	.flags     = OBS_OUTPUT_AUDIO | OBS_OUTPUT_VIDEO,
	.get_name  = ffmpeg_output_getname,
	.create    = ffmpeg_output_create,
	.destroy   = ffmpeg_output_destroy,
	.start     = ffmpeg_output_start,
	.stop      = ffmpeg_output_stop,
	.raw_video = receive_video,
	.raw_audio = receive_audio,
	.get_total_bytes = ffmpeg_output_total_bytes,
};
