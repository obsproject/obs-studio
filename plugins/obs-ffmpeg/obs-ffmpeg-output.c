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
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "obs-ffmpeg-formats.h"
#include "obs-ffmpeg-compat.h"

//#define OBS_FFMPEG_VIDEO_FORMAT VIDEO_FORMAT_I420
#define OBS_FFMPEG_VIDEO_FORMAT VIDEO_FORMAT_NV12

/* NOTE: much of this stuff is test stuff that was more or less copied from
 * the muxing.c ffmpeg example */

struct ffmpeg_data {
	AVStream           *video;
	AVStream           *audio;
	AVCodec            *acodec;
	AVCodec            *vcodec;
	AVFormatContext    *output;
	struct SwsContext  *swscale;

	int                video_bitrate;
	AVPicture          dst_picture;
	AVFrame            *vframe;
	int                frame_size;
	int                total_frames;

	int                width;
	int                height;

	uint64_t           start_timestamp;

	int                audio_bitrate;
	uint32_t           audio_samplerate;
	enum audio_format  audio_format;
	size_t             audio_planes;
	size_t             audio_size;
	struct circlebuf   excess_frames[MAX_AV_PLANES];
	uint8_t            *samples[MAX_AV_PLANES];
	AVFrame            *aframe;
	int                total_samples;

	const char         *filename_test;

	bool               initialized;
};

struct ffmpeg_output {
	obs_output_t       *output;
	volatile bool      active;
	struct ffmpeg_data ff_data;

	bool               connecting;
	pthread_t          start_thread;

	bool               write_thread_active;
	pthread_mutex_t    write_mutex;
	pthread_t          write_thread;
	os_sem_t           *write_sem;
	os_event_t         *stop_event;

	DARRAY(AVPacket)   packets;
};

/* ------------------------------------------------------------------------- */

static bool new_stream(struct ffmpeg_data *data, AVStream **stream,
		AVCodec **codec, enum AVCodecID id)
{
	*codec = avcodec_find_encoder(id);
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

static bool open_video_codec(struct ffmpeg_data *data)
{
	AVCodecContext *context = data->video->codec;
	int ret;

	if (data->vcodec->id == AV_CODEC_ID_H264)
		av_opt_set(context->priv_data, "preset", "veryfast", 0);

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
	enum AVPixelFormat format;
	format = obs_to_ffmpeg_video_format(OBS_FFMPEG_VIDEO_FORMAT);

	data->swscale = sws_getContext(
			context->width, context->height, format,
			context->width, context->height, context->pix_fmt,
			SWS_BICUBIC, NULL, NULL, NULL);

	if (!data->swscale) {
		blog(LOG_WARNING, "Could not initialize swscale");
		return false;
	}

	return true;
}

static bool create_video_stream(struct ffmpeg_data *data)
{
	AVCodecContext *context;
	struct obs_video_info ovi;
	enum AVPixelFormat vformat;

	vformat = obs_to_ffmpeg_video_format(OBS_FFMPEG_VIDEO_FORMAT);

	if (!obs_get_video_info(&ovi)) {
		blog(LOG_WARNING, "No active video");
		return false;
	}

	if (!new_stream(data, &data->video, &data->vcodec,
				data->output->oformat->video_codec))
		return false;

	context                 = data->video->codec;
	context->codec_id       = data->output->oformat->video_codec;
	context->bit_rate       = data->video_bitrate * 1000;
	context->rc_buffer_size = data->video_bitrate * 1000;
	context->rc_max_rate    = data->video_bitrate * 1000;
	context->width          = data->width;
	context->height         = data->height;
	context->time_base.num  = ovi.fps_den;
	context->time_base.den  = ovi.fps_num;
	context->gop_size       = 120;
	context->pix_fmt        = vformat;

	if (data->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_HEADER;

	if (!open_video_codec(data))
		return false;

	if (context->pix_fmt != vformat)
		if (!init_swscale(data, context))
			return false;

	return true;
}

static bool open_audio_codec(struct ffmpeg_data *data)
{
	AVCodecContext *context = data->audio->codec;
	int ret;

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
	struct audio_output_info aoi;

	if (!obs_get_audio_info(&aoi)) {
		blog(LOG_WARNING, "No active audio");
		return false;
	}

	if (!new_stream(data, &data->audio, &data->acodec,
				data->output->oformat->audio_codec))
		return false;

	context              = data->audio->codec;
	context->bit_rate    = data->audio_bitrate * 1000;
	context->channels    = get_audio_channels(aoi.speakers);
	context->sample_rate = aoi.samples_per_sec;
	context->sample_fmt  = data->acodec->sample_fmts ?
		data->acodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

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
		ret = avio_open(&data->output->pb, data->filename_test,
				AVIO_FLAG_WRITE);
		if (ret < 0) {
			blog(LOG_WARNING, "Couldn't open file '%s', %s",
					data->filename_test, av_err2str(ret));
			return false;
		}
	}

	ret = avformat_write_header(data->output, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Error opening file '%s': %s",
				data->filename_test, av_err2str(ret));
		return false;
	}

	return true;
}

static void close_video(struct ffmpeg_data *data)
{
	avcodec_close(data->video->codec);
	avpicture_free(&data->dst_picture);
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

static bool ffmpeg_data_init(struct ffmpeg_data *data, const char *filename,
		int vbitrate, int abitrate, int width, int height)
{
	bool is_rtmp = false;

	memset(data, 0, sizeof(struct ffmpeg_data));
	data->filename_test = filename;
	data->video_bitrate = vbitrate;
	data->audio_bitrate = abitrate;
	data->width         = width;
	data->height        = height;

	if (!filename || !*filename)
		return false;

	av_register_all();
	avformat_network_init();

	is_rtmp = (astrcmp_n(filename, "rtmp://", 7) == 0);

	/* TODO: settings */
	avformat_alloc_output_context2(&data->output, NULL,
			is_rtmp ? "flv" : NULL, data->filename_test);
	if (is_rtmp) {
		data->output->oformat->video_codec = AV_CODEC_ID_H264;
		data->output->oformat->audio_codec = AV_CODEC_ID_AAC;
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

static const char *ffmpeg_output_getname(void)
{
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

static void ffmpeg_output_stop(void *data);

static void ffmpeg_output_destroy(void *data)
{
	struct ffmpeg_output *output = data;

	if (output) {
		if (output->connecting)
			pthread_join(output->start_thread, NULL);

		ffmpeg_output_stop(output);

		pthread_mutex_destroy(&output->write_mutex);
		os_sem_destroy(output->write_sem);
		os_event_destroy(output->stop_event);
		bfree(data);
	}
}

static inline void copy_data(AVPicture *pic, const struct video_data *frame,
		int height)
{
	for (int plane = 0; plane < MAX_AV_PLANES; plane++) {
		if (!frame->data[plane])
			continue;

		int frame_rowsize = (int)frame->linesize[plane];
		int pic_rowsize   = pic->linesize[plane];
		int bytes = frame_rowsize < pic_rowsize ?
			frame_rowsize : pic_rowsize;
		int plane_height = plane == 0 ? height : height/2;

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
	AVCodecContext *context = data->video->codec;
	AVPacket packet = {0};
	int ret = 0, got_packet;
	enum AVPixelFormat format;

	av_init_packet(&packet);

	if (!data->start_timestamp)
		data->start_timestamp = frame->timestamp;

	format = obs_to_ffmpeg_video_format(OBS_FFMPEG_VIDEO_FORMAT);
	if (context->pix_fmt != format)
		sws_scale(data->swscale, (const uint8_t *const *)frame->data,
				(const int*)frame->linesize,
				0, context->height, data->dst_picture.data,
				data->dst_picture.linesize);
	else
		copy_data(&data->dst_picture, frame, context->height);

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

	AVCodecContext *context = data->audio->codec;

	if (!data->start_timestamp)
		return;
	if (!prepare_audio(data, frame, &in))
		return;

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

static bool process_packet(struct ffmpeg_output *output)
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
		return true;

	/*blog(LOG_DEBUG, "size = %d, flags = %lX, stream = %d, "
			"packets queued: %lu",
			packet.size, packet.flags,
			packet.stream_index, output->packets.num);*/

	ret = av_interleaved_write_frame(output->ff_data.output, &packet);
	if (ret < 0) {
		av_free_packet(&packet);
		blog(LOG_WARNING, "receive_audio: Error writing packet: %s",
				av_err2str(ret));
		return false;
	}

	return true;
}

static void *write_thread(void *data)
{
	struct ffmpeg_output *output = data;

	while (os_sem_wait(output->write_sem) == 0) {
		/* check to see if shutting down */
		if (os_event_try(output->stop_event) == 0)
			break;

		if (!process_packet(output)) {
			pthread_detach(output->write_thread);
			output->write_thread_active = false;

			ffmpeg_output_stop(output);
			break;
		}
	}

	output->active = false;
	return NULL;
}

static bool try_connect(struct ffmpeg_output *output)
{
	const char *filename_test;
	obs_data_t *settings;
	int audio_bitrate, video_bitrate;
	int width, height;
	int ret;

	settings = obs_output_get_settings(output->output);
	filename_test = obs_data_get_string(settings, "filename");
	video_bitrate = (int)obs_data_get_int(settings, "video_bitrate");
	audio_bitrate = (int)obs_data_get_int(settings, "audio_bitrate");
	obs_data_release(settings);

	if (!filename_test || !*filename_test)
		return false;

	width  = (int)obs_output_get_width(output->output);
	height = (int)obs_output_get_height(output->output);

	if (!ffmpeg_data_init(&output->ff_data, filename_test,
				video_bitrate, audio_bitrate,
				width, height))
		return false;

	struct audio_convert_info aci = {
		.format = output->ff_data.audio_format
	};

	struct video_scale_info vsi = {
		.format = OBS_FFMPEG_VIDEO_FORMAT
	};

	output->active = true;

	if (!obs_output_can_begin_data_capture(output->output, 0))
		return false;

	ret = pthread_create(&output->write_thread, NULL, write_thread, output);
	if (ret != 0) {
		blog(LOG_WARNING, "ffmpeg_output_start: failed to create write "
		                  "thread.");
		ffmpeg_output_stop(output);
		return false;
	}

	obs_output_set_video_conversion(output->output, &vsi);
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

	ret = pthread_create(&output->start_thread, NULL, start_thread, output);
	return (output->connecting = (ret == 0));
}

static void ffmpeg_output_stop(void *data)
{
	struct ffmpeg_output *output = data;

	if (output->active) {
		obs_output_end_data_capture(output->output);

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
};
