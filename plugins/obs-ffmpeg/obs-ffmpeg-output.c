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
#include <util/circlebuf.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

struct ffmpeg_data {
	AVStream           *video;
	AVStream           *audio;
	AVCodec            *acodec;
	AVCodec            *vcodec;
	AVFormatContext    *output;
	struct SwsContext  *swscale;

	AVPicture          dst_picture;
	AVFrame            *vframe;
	int                frame_size;
	int                total_frames;

	struct circlebuf   excess_frames[MAX_AV_PLANES];
	uint8_t            *samples[MAX_AV_PLANES];
	AVFrame            *aframe;
	int                total_samples;

	const char         *filename_test;

	bool               initialized;
};

struct ffmpeg_output {
	obs_output_t       output;
	volatile bool      active;
	struct ffmpeg_data ff_data;
};

/* ------------------------------------------------------------------------- */

/* TODO: remove these later */
#define SPS_TODO      44100

/* NOTE: much of this stuff is test stuff that was more or less copied from
 * the muxing.c ffmpeg example */

static inline enum AVPixelFormat obs_to_ffmpeg_video_format(
		enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_NONE: return AV_PIX_FMT_NONE;
	case VIDEO_FORMAT_I420: return AV_PIX_FMT_YUV420P;
	case VIDEO_FORMAT_NV12: return AV_PIX_FMT_NV12;
	case VIDEO_FORMAT_YVYU: return AV_PIX_FMT_NONE;
	case VIDEO_FORMAT_YUY2: return AV_PIX_FMT_YUYV422;
	case VIDEO_FORMAT_UYVY: return AV_PIX_FMT_UYVY422;
	case VIDEO_FORMAT_RGBA: return AV_PIX_FMT_RGBA;
	case VIDEO_FORMAT_BGRA: return AV_PIX_FMT_BGRA;
	case VIDEO_FORMAT_BGRX: return AV_PIX_FMT_BGRA;
	}

	return AV_PIX_FMT_NONE;
}

static bool new_stream(struct ffmpeg_data *data, AVStream **stream,
		AVCodec **codec, enum AVCodecID id)
{
	*codec = avcodec_find_encoder(id);
	if (!*codec) {
		blog(LOG_ERROR, "Couldn't find encoder '%s'",
				avcodec_get_name(id));
		return false;
	}

	*stream = avformat_new_stream(data->output, *codec);
	if (!*stream) {
		blog(LOG_ERROR, "Couldn't create stream for encoder '%s'",
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

	ret = avcodec_open2(context, data->vcodec, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "Failed to open video codec: %s",
				av_err2str(ret));
		return false;
	}

	data->vframe = av_frame_alloc();
	if (!data->vframe) {
		blog(LOG_ERROR, "Failed to allocate video frame");
		return false;
	}

	data->vframe->format = context->pix_fmt;
	data->vframe->width  = context->width;
	data->vframe->height = context->height;

	ret = avpicture_alloc(&data->dst_picture, context->pix_fmt,
			context->width, context->height);
	if (ret < 0) {
		blog(LOG_ERROR, "Failed to allocate dst_picture: %s",
				av_err2str(ret));
		return false;
	}

	*((AVPicture*)data->vframe) = data->dst_picture;
	return true;
}

static bool init_swscale(struct ffmpeg_data *data, AVCodecContext *context)
{
	data->swscale = sws_getContext(
			context->width, context->height, AV_PIX_FMT_YUV420P,
			context->width, context->height, context->pix_fmt,
			SWS_BICUBIC, NULL, NULL, NULL);

	if (!data->swscale) {
		blog(LOG_ERROR, "Could not initialize swscale");
		return false;
	}

	return true;
}

static bool create_video_stream(struct ffmpeg_data *data)
{
	AVCodecContext *context;
	struct obs_video_info ovi;

	if (!obs_get_video_info(&ovi)) {
		blog(LOG_ERROR, "No active video");
		return false;
	}

	if (!new_stream(data, &data->video, &data->vcodec,
				data->output->oformat->video_codec))
		return false;

	context                = data->video->codec;
	context->codec_id      = data->output->oformat->video_codec;
	context->bit_rate      = 6000000;
	context->width         = ovi.output_width;
	context->height        = ovi.output_height;
	context->time_base.num = ovi.fps_den;
	context->time_base.den = ovi.fps_num;
	context->gop_size      = 12;
	context->pix_fmt       = AV_PIX_FMT_YUV420P;

	if (data->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_HEADER;

	if (!open_video_codec(data))
		return false;

	if (context->pix_fmt != AV_PIX_FMT_YUV420P)
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
		blog(LOG_ERROR, "Failed to allocate audio frame");
		return false;
	}

	context->strict_std_compliance = -2;

	ret = avcodec_open2(context, data->acodec, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "Failed to open audio codec: %s",
				av_err2str(ret));
		return false;
	}

	data->frame_size = context->frame_size ? context->frame_size : 1024;

	ret = av_samples_alloc(data->samples, NULL, context->channels,
			data->frame_size, context->sample_fmt, 0);
	if (ret < 0) {
		blog(LOG_ERROR, "Failed to create audio buffer: %s",
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
		blog(LOG_ERROR, "No active audio");
		return false;
	}

	if (!new_stream(data, &data->audio, &data->acodec,
				data->output->oformat->audio_codec))
		return false;

	context              = data->audio->codec;
	context->bit_rate    = 128000;
	context->channels    = get_audio_channels(aoi.speakers);
	context->sample_rate = aoi.samples_per_sec;
	context->sample_fmt  = data->acodec->sample_fmts ?
		data->acodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

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
			blog(LOG_ERROR, "Couldn't open file '%s', %s",
					data->filename_test, av_err2str(ret));
			return false;
		}
	}

	ret = avformat_write_header(data->output, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "Error opening file '%s': %s",
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
	if ((data->output->oformat->flags & AVFMT_NOFILE) == 0)
		avio_close(data->output->pb);

	avformat_free_context(data->output);

	memset(data, 0, sizeof(struct ffmpeg_data));
}

static bool ffmpeg_data_init(struct ffmpeg_data *data, const char *filename)
{
	memset(data, 0, sizeof(struct ffmpeg_data));
	data->filename_test = filename;

	if (!filename || !*filename)
		return false;

	av_register_all();

	/* TODO: settings */
	avformat_alloc_output_context2(&data->output, NULL, NULL,
			data->filename_test);
	if (!data->output) {
		blog(LOG_ERROR, "Couldn't create avformat context");
		goto fail;
	}

	if (!init_streams(data))
		goto fail;
	if (!open_output_file(data))
		goto fail;

	data->initialized = true;
	return true;

fail:
	blog(LOG_ERROR, "ffmpeg_data_init failed");
	ffmpeg_data_free(data);
	return false;
}

/* ------------------------------------------------------------------------- */

static const char *ffmpeg_output_getname(const char *locale)
{
	UNUSED_PARAMETER(locale);
	return "FFmpeg file output";
}

static void ffmpeg_log_callback(void *param, int bla, const char *format,
		va_list args)
{
	blogva(LOG_DEBUG, format, args);

	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(bla);
}

static void *ffmpeg_output_create(obs_data_t settings,
		obs_output_t output)
{
	struct ffmpeg_output *data = bzalloc(sizeof(struct ffmpeg_output));
	data->output = output;

	av_log_set_callback(ffmpeg_log_callback);

	UNUSED_PARAMETER(settings);
	return data;
}

static void ffmpeg_output_destroy(void *data)
{
	struct ffmpeg_output *output = data;

	if (output) {
		if (output->active)
			ffmpeg_data_free(&output->ff_data);
		bfree(data);
	}
}

static inline int64_t rescale_ts(int64_t val, AVCodecContext *context,
		AVStream *stream)
{
	return av_rescale_q_rnd(val, context->time_base,
			stream->time_base,
			AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
}

#define YUV420_PLANES 3

static inline void copy_data(AVPicture *pic, const struct video_frame *frame,
		int height)
{
	for (int plane = 0; plane < YUV420_PLANES; plane++) {
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

static void receive_video(void *param, const struct video_frame *frame)
{
	struct ffmpeg_output *output = param;
	struct ffmpeg_data   *data   = &output->ff_data;
	AVCodecContext *context = data->video->codec;
	AVPacket packet = {0};
	int ret, got_packet;

	av_init_packet(&packet);

	if (context->pix_fmt != AV_PIX_FMT_YUV420P)
		sws_scale(data->swscale, frame->data,
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

		ret = av_interleaved_write_frame(data->output, &packet);

	} else {
		data->vframe->pts = data->total_frames;
		ret = avcodec_encode_video2(context, &packet, data->vframe,
				&got_packet);
		if (ret < 0) {
			blog(LOG_ERROR, "receive_video: Error encoding "
			                "video: %s", av_err2str(ret));
			return;
		}

		if (!ret && got_packet && packet.size) {
			packet.pts = rescale_ts(packet.pts, context,
					data->video);
			packet.dts = rescale_ts(packet.dts, context,
					data->video);
			packet.duration = (int)av_rescale_q(packet.duration,
					context->time_base,
					data->video->time_base);

			ret = av_interleaved_write_frame(data->output,
					&packet);
		} else {
			ret = 0;
		}
	}

	if (ret != 0) {
		blog(LOG_ERROR, "receive_video: Error writing video: %s",
				av_err2str(ret));
	}

	data->total_frames++;
}

static inline void encode_audio(struct ffmpeg_data *output,
		struct AVCodecContext *context, size_t block_size)
{
	AVPacket packet = {0};
	int ret, got_packet;
	size_t total_size = output->frame_size * block_size * context->channels;

	output->aframe->nb_samples = output->frame_size;
	output->aframe->pts = av_rescale_q(output->total_samples,
			(AVRational){1, context->sample_rate},
			context->time_base);

	ret = avcodec_fill_audio_frame(output->aframe, context->channels,
			context->sample_fmt, output->samples[0],
			(int)total_size, 1);
	if (ret < 0) {
		blog(LOG_ERROR, "receive_audio: avcodec_fill_audio_frame "
		                "failed: %s", av_err2str(ret));
		return;
	}

	output->total_samples += output->frame_size;

	ret = avcodec_encode_audio2(context, &packet, output->aframe,
			&got_packet);
	if (ret < 0) {
		blog(LOG_ERROR, "receive_audio: Error encoding audio: %s",
				av_err2str(ret));
		return;
	}

	if (!got_packet)
		return;

	packet.pts = rescale_ts(packet.pts, context, output->audio);
	packet.dts = rescale_ts(packet.dts, context, output->audio);
	packet.duration = (int)av_rescale_q(packet.duration, context->time_base,
			output->audio->time_base);
	packet.stream_index = output->audio->index;

	ret = av_interleaved_write_frame(output->output, &packet);
	if (ret != 0)
		blog(LOG_ERROR, "receive_audio: Error writing audio: %s",
				av_err2str(ret));
}

static void receive_audio(void *param, const struct audio_data *frame)
{
	struct ffmpeg_output *output = param;
	struct ffmpeg_data   *data   = &output->ff_data;

	AVCodecContext *context = data->audio->codec;
	size_t planes = audio_output_planes(obs_audio());
	size_t block_size = audio_output_blocksize(obs_audio());

	size_t frame_size_bytes = (size_t)data->frame_size * block_size;

	for (size_t i = 0; i < planes; i++)
		circlebuf_push_back(&data->excess_frames[i], frame->data[0],
				frame->frames * block_size);

	while (data->excess_frames[0].size >= frame_size_bytes) {
		for (size_t i = 0; i < planes; i++)
			circlebuf_pop_front(&data->excess_frames[i],
					data->samples[i], frame_size_bytes);

		encode_audio(data, context, block_size);
	}
}

static bool ffmpeg_output_start(void *data)
{
	struct ffmpeg_output *output = data;

	video_t video = obs_video();
	audio_t audio = obs_audio();

	if (!video || !audio) {
		blog(LOG_ERROR, "ffmpeg_output_start: audio and video must "
		                "both be active (at least as of this writing)");
		return false;
	}

	const char *filename_test;
	obs_data_t settings = obs_output_get_settings(output->output);
	filename_test = obs_data_getstring(settings, "filename");
	obs_data_release(settings);

	if (!filename_test || !*filename_test)
		return false;

	if (!ffmpeg_data_init(&output->ff_data, filename_test))
		return false;

	struct audio_convert_info aci;
	aci.samples_per_sec = SPS_TODO;
	aci.format          = AUDIO_FORMAT_FLOAT_PLANAR;
	aci.speakers        = SPEAKERS_STEREO;

	struct video_convert_info vci;
	vci.format          = VIDEO_FORMAT_I420;
	vci.width           = 0;
	vci.height          = 0;

	video_output_connect(video, &vci, receive_video, output);
	audio_output_connect(audio, &aci, receive_audio, output);
	output->active = true;

	return true;
}

static void ffmpeg_output_stop(void *data)
{
	struct ffmpeg_output *output = data;

	if (output->active) {
		output->active = false;
		video_output_disconnect(obs_video(), receive_video, data);
		audio_output_disconnect(obs_audio(), receive_audio, data);
		ffmpeg_data_free(&output->ff_data);
	}
}

static bool ffmpeg_output_active(void *data)
{
	struct ffmpeg_output *output = data;
	return output->active;
}

struct obs_output_info ffmpeg_output = {
	.id      = "ffmpeg_output",
	.getname = ffmpeg_output_getname,
	.create  = ffmpeg_output_create,
	.destroy = ffmpeg_output_destroy,
	.start   = ffmpeg_output_start,
	.stop    = ffmpeg_output_stop,
	.active  = ffmpeg_output_active
};
