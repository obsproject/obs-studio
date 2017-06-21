/*
 * Copyright (c) 2015 Hugh Bailey <obs.jim@gmail.com>
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

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#define inline __inline

#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "ffmpeg-mux.h"

#include <libavformat/avformat.h>

/* ------------------------------------------------------------------------- */

struct resize_buf {
	uint8_t *buf;
	size_t  size;
	size_t  capacity;
};

static inline void resize_buf_resize(struct resize_buf *rb, size_t size)
{
	if (!rb->buf) {
		rb->buf = malloc(size);
		rb->size = size;
		rb->capacity = size;
	} else {
		if (rb->capacity < size) {
			size_t capx2 = rb->capacity * 2;
			size_t new_cap = capx2 > size ? capx2 : size;
			rb->buf = realloc(rb->buf, new_cap);
			rb->capacity = new_cap;
		}

		rb->size = size;
	}
}

static inline void resize_buf_free(struct resize_buf *rb)
{
	free(rb->buf);
}

/* ------------------------------------------------------------------------- */

struct main_params {
	char *file;
	int has_video;
	int tracks;
	char *vcodec;
	int vbitrate;
	int gop;
	int width;
	int height;
	int fps_num;
	int fps_den;
	char *acodec;
	char *muxer_settings;
};

struct audio_params {
	char *name;
	int abitrate;
	int sample_rate;
	int channels;
};

struct header {
	uint8_t *data;
	int size;
};

struct ffmpeg_mux {
	AVFormatContext        *output;
	AVStream               *video_stream;
	AVStream               **audio_streams;
	struct main_params     params;
	struct audio_params    *audio;
	struct header          video_header;
	struct header          *audio_header;
	int                    num_audio_streams;
	bool                   initialized;
	char error[4096];
};

static void header_free(struct header *header)
{
	free(header->data);
}

static void free_avformat(struct ffmpeg_mux *ffm)
{
	if (ffm->output) {
		if ((ffm->output->oformat->flags & AVFMT_NOFILE) == 0)
			avio_close(ffm->output->pb);

		avformat_free_context(ffm->output);
		ffm->output = NULL;
	}

	if (ffm->audio_streams) {
		free(ffm->audio_streams);
	}

	ffm->video_stream = NULL;
	ffm->audio_streams = NULL;
	ffm->num_audio_streams = 0;
}

static void ffmpeg_mux_free(struct ffmpeg_mux *ffm)
{
	if (ffm->initialized) {
		av_write_trailer(ffm->output);
	}

	free_avformat(ffm);

	header_free(&ffm->video_header);

	if (ffm->audio_header) {
		for (int i = 0; i < ffm->params.tracks; i++) {
			header_free(&ffm->audio_header[i]);
		}

		free(ffm->audio_header);
	}

	if (ffm->audio) {
		free(ffm->audio);
	}

	memset(ffm, 0, sizeof(*ffm));
}

static bool get_opt_str(int *p_argc, char ***p_argv, char **str,
		const char *opt)
{
	int argc = *p_argc;
	char **argv = *p_argv;

	if (!argc) {
		printf("Missing expected option: '%s'\n", opt);
		return false;
	}

	(*p_argc)--;
	(*p_argv)++;
	*str = argv[0];
	return true;
}

static bool get_opt_int(int *p_argc, char ***p_argv, int *i, const char *opt)
{
	char *str;

	if (!get_opt_str(p_argc, p_argv, &str, opt)) {
		return false;
	}

	*i = atoi(str);
	return true;
}

static bool get_audio_params(struct audio_params *audio, int *argc,
		char ***argv)
{
	if (!get_opt_str(argc, argv, &audio->name, "audio track name"))
		return false;
	if (!get_opt_int(argc, argv, &audio->abitrate, "audio bitrate"))
		return false;
	if (!get_opt_int(argc, argv, &audio->sample_rate, "audio sample rate"))
		return false;
	if (!get_opt_int(argc, argv, &audio->channels, "audio channels"))
		return false;
	return true;
}

static bool init_params(int *argc, char ***argv, struct main_params *params,
		struct audio_params **p_audio)
{
	struct audio_params *audio = NULL;

	if (!get_opt_str(argc, argv, &params->file, "file name"))
		return false;
	if (!get_opt_int(argc, argv, &params->has_video, "video track count"))
		return false;
	if (!get_opt_int(argc, argv, &params->tracks, "audio track count"))
		return false;

	if (params->has_video > 1 || params->has_video < 0) {
		puts("Invalid number of video tracks\n");
		return false;
	}
	if (params->tracks < 0) {
		puts("Invalid number of audio tracks\n");
		return false;
	}
	if (params->has_video == 0 && params->tracks == 0) {
		puts("Must have at least 1 audio track or 1 video track\n");
		return false;
	}

	if (params->has_video) {
		if (!get_opt_str(argc, argv, &params->vcodec, "video codec"))
			return false;
		if (!get_opt_int(argc, argv, &params->vbitrate,"video bitrate"))
			return false;
		if (!get_opt_int(argc, argv, &params->width, "video width"))
			return false;
		if (!get_opt_int(argc, argv, &params->height, "video height"))
			return false;
		if (!get_opt_int(argc, argv, &params->fps_num, "video fps num"))
			return false;
		if (!get_opt_int(argc, argv, &params->fps_den, "video fps den"))
			return false;
	}

	if (params->tracks) {
		if (!get_opt_str(argc, argv, &params->acodec, "audio codec"))
			return false;

		audio = calloc(1, sizeof(*audio) * params->tracks);

		for (int i = 0; i < params->tracks; i++) {
			if (!get_audio_params(&audio[i], argc, argv)) {
				free(audio);
				return false;
			}
		}
	}

	*p_audio = audio;

	get_opt_str(argc, argv, &params->muxer_settings, "muxer settings");

	return true;
}

static bool new_stream(struct ffmpeg_mux *ffm, AVStream **stream,
		const char *name, enum AVCodecID *id)
{
	const AVCodecDescriptor *desc = avcodec_descriptor_get_by_name(name);
	AVCodec *codec;

	if (!desc) {
		printf("Couldn't find encoder '%s'\n", name);
		return false;
	}

	*id = desc->id;

	codec = avcodec_find_encoder(desc->id);
	if (!codec) {
		printf("Couldn't create encoder");
		return false;
	}

	*stream = avformat_new_stream(ffm->output, codec);
	if (!*stream) {
		printf("Couldn't create stream for encoder '%s'\n", name);
		return false;
	}

	(*stream)->id = ffm->output->nb_streams-1;
	return true;
}

static void create_video_stream(struct ffmpeg_mux *ffm)
{
	AVCodecContext *context;
	void *extradata = NULL;

	if (!new_stream(ffm, &ffm->video_stream, ffm->params.vcodec,
				&ffm->output->oformat->video_codec))
		return;

	if (ffm->video_header.size) {
		extradata = av_memdup(ffm->video_header.data,
				ffm->video_header.size);
	}

	context                 = ffm->video_stream->codec;
	context->bit_rate       = ffm->params.vbitrate * 1000;
	context->width          = ffm->params.width;
	context->height         = ffm->params.height;
	context->coded_width    = ffm->params.width;
	context->coded_height   = ffm->params.height;
	context->extradata      = extradata;
	context->extradata_size = ffm->video_header.size;
	context->time_base =
		(AVRational){ffm->params.fps_den, ffm->params.fps_num};

	ffm->video_stream->time_base = context->time_base;

	if (ffm->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_HEADER;
}

static void create_audio_stream(struct ffmpeg_mux *ffm, int idx)
{
	AVCodecContext *context;
	AVStream *stream;
	void *extradata = NULL;

	if (!new_stream(ffm, &stream, ffm->params.acodec,
				&ffm->output->oformat->audio_codec))
		return;

	ffm->audio_streams[idx] = stream;

	av_dict_set(&stream->metadata, "title", ffm->audio[idx].name, 0);

	stream->time_base = (AVRational){1, ffm->audio[idx].sample_rate};

	if (ffm->audio_header[idx].size) {
		extradata = av_memdup(ffm->audio_header[idx].data,
				ffm->audio_header[idx].size);
	}

	context                 = stream->codec;
	context->bit_rate       = ffm->audio[idx].abitrate * 1000;
	context->channels       = ffm->audio[idx].channels;
	context->sample_rate    = ffm->audio[idx].sample_rate;
	context->sample_fmt     = AV_SAMPLE_FMT_S16;
	context->time_base      = stream->time_base;
	context->extradata      = extradata;
	context->extradata_size = ffm->audio_header[idx].size;
	context->channel_layout =
			av_get_default_channel_layout(context->channels);

	if (ffm->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_HEADER;

	ffm->num_audio_streams++;
}

static bool init_streams(struct ffmpeg_mux *ffm)
{
	if (ffm->params.has_video)
		create_video_stream(ffm);

	if (ffm->params.tracks) {
		ffm->audio_streams =
			calloc(1, ffm->params.tracks * sizeof(void*));

		for (int i = 0; i < ffm->params.tracks; i++)
			create_audio_stream(ffm, i);
	}

	if (!ffm->video_stream && !ffm->num_audio_streams)
		return false;

	return true;
}

static void set_header(struct header *header, uint8_t *data, size_t size)
{
	header->size = (int)size;
	header->data = malloc(size);
	memcpy(header->data, data, size);
}

static void ffmpeg_mux_header(struct ffmpeg_mux *ffm, uint8_t *data,
		struct ffm_packet_info *info)
{
	if (info->type == FFM_PACKET_VIDEO) {
		set_header(&ffm->video_header, data, (size_t)info->size);
	} else {
		set_header(&ffm->audio_header[info->index], data,
				(size_t)info->size);
	}
}

static size_t safe_read(void *vdata, size_t size)
{
	uint8_t *data = vdata;
	size_t  total = size;

	while (size > 0) {
		size_t in_size = fread(data, 1, size, stdin);
		if (in_size == 0)
			return 0;

		size -= in_size;
		data += in_size;
	}

	return total;
}

static bool ffmpeg_mux_get_header(struct ffmpeg_mux *ffm)
{
	struct ffm_packet_info info = {0};

	bool success = safe_read(&info, sizeof(info)) == sizeof(info);
	if (success) {
		uint8_t *data = malloc(info.size);

		if (safe_read(data, info.size) == info.size) {
			ffmpeg_mux_header(ffm, data, &info);
		} else {
			success = false;
		}

		free(data);
	}

	return success;
}

static inline bool ffmpeg_mux_get_extra_data(struct ffmpeg_mux *ffm)
{
	if (ffm->params.has_video) {
		if (!ffmpeg_mux_get_header(ffm)) {
			return false;
		}
	}

	for (int i = 0; i < ffm->params.tracks; i++) {
		if (!ffmpeg_mux_get_header(ffm)) {
			return false;
		}
	}

	return true;
}

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

static inline int open_output_file(struct ffmpeg_mux *ffm)
{
	AVOutputFormat *format = ffm->output->oformat;
	int ret;

	if ((format->flags & AVFMT_NOFILE) == 0) {
		ret = avio_open(&ffm->output->pb, ffm->params.file,
				AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Couldn't open '%s', %s",
					ffm->params.file, av_err2str(ret));
			return FFM_ERROR;
		}
	}

	strncpy(ffm->output->filename, ffm->params.file,
			sizeof(ffm->output->filename));
	ffm->output->filename[sizeof(ffm->output->filename) - 1] = 0;

	AVDictionary *dict = NULL;
	if ((ret = av_dict_parse_string(&dict, ffm->params.muxer_settings,
				"=", " ", 0))) {
		printf("Failed to parse muxer settings: %s\n%s",
				av_err2str(ret), ffm->params.muxer_settings);

		av_dict_free(&dict);
	}

	if (av_dict_count(dict) > 0) {
		printf("Using muxer settings:");

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry,
						AV_DICT_IGNORE_SUFFIX)))
			printf("\n\t%s=%s", entry->key, entry->value);

		printf("\n");
	}

	ret = avformat_write_header(ffm->output, &dict);
	if (ret < 0) {
		printf("Error opening '%s': %s",
				ffm->params.file, av_err2str(ret));

		av_dict_free(&dict);

		return ret == -22 ? FFM_UNSUPPORTED : FFM_ERROR;
	}

	av_dict_free(&dict);

	return FFM_SUCCESS;
}

static int ffmpeg_mux_init_context(struct ffmpeg_mux *ffm)
{
	AVOutputFormat *output_format;
	int ret;

	output_format = av_guess_format(NULL, ffm->params.file, NULL);
	if (output_format == NULL) {
		printf("Couldn't find an appropriate muxer for '%s'\n",
				ffm->params.file);
		return FFM_ERROR;
	}

	ret = avformat_alloc_output_context2(&ffm->output, output_format,
			NULL, NULL);
	if (ret < 0) {
		printf("Couldn't initialize output context: %s\n",
				av_err2str(ret));
		return FFM_ERROR;
	}

	ffm->output->oformat->video_codec = AV_CODEC_ID_NONE;
	ffm->output->oformat->audio_codec = AV_CODEC_ID_NONE;

	if (!init_streams(ffm)) {
		free_avformat(ffm);
		return FFM_ERROR;
	}

	ret = open_output_file(ffm);
	if (ret != FFM_SUCCESS) {
		free_avformat(ffm);
		return ret;
	}

	return FFM_SUCCESS;
}

static int ffmpeg_mux_init_internal(struct ffmpeg_mux *ffm, int argc,
		char *argv[])
{
	argc--;
	argv++;
	if (!init_params(&argc, &argv, &ffm->params, &ffm->audio))
		return FFM_ERROR;

	if (ffm->params.tracks) {
		ffm->audio_header =
			calloc(1, sizeof(struct header) * ffm->params.tracks);
	}

	av_register_all();

	if (!ffmpeg_mux_get_extra_data(ffm))
		return FFM_ERROR;

	/* ffmpeg does not have a way of telling what's supported
	 * for a given output format, so we try each possibility */
	return ffmpeg_mux_init_context(ffm);
}

static int ffmpeg_mux_init(struct ffmpeg_mux *ffm, int argc, char *argv[])
{
	int ret = ffmpeg_mux_init_internal(ffm, argc, argv);
	if (ret != FFM_SUCCESS) {
		ffmpeg_mux_free(ffm);
		return ret;
	}

	ffm->initialized = true;
	return ret;
}

static inline int get_index(struct ffmpeg_mux *ffm,
		struct ffm_packet_info *info)
{
	if (info->type == FFM_PACKET_VIDEO) {
		if (ffm->video_stream) {
			return ffm->video_stream->id;
		}
	} else {
		if ((int)info->index < ffm->num_audio_streams) {
			return ffm->audio_streams[info->index]->id;
		}
	}

	return -1;
}

static inline AVStream *get_stream(struct ffmpeg_mux *ffm, int idx)
{
	return ffm->output->streams[idx];
}

static inline int64_t rescale_ts(struct ffmpeg_mux *ffm, int64_t val, int idx)
{
	AVStream *stream = get_stream(ffm, idx);

	return av_rescale_q_rnd(val / stream->codec->time_base.num,
			stream->codec->time_base, stream->time_base,
			AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
}

static inline bool ffmpeg_mux_packet(struct ffmpeg_mux *ffm, uint8_t *buf,
		struct ffm_packet_info *info)
{
	int idx = get_index(ffm, info);
	AVPacket packet = {0};

	/* The muxer might not support video/audio, or multiple audio tracks */
	if (idx == -1) {
		return true;
	}

        av_init_packet(&packet);

	packet.data = buf;
	packet.size = (int)info->size;
	packet.stream_index = idx;
	packet.pts = rescale_ts(ffm, info->pts, idx);
	packet.dts = rescale_ts(ffm, info->dts, idx);

	if (info->keyframe)
		packet.flags = AV_PKT_FLAG_KEY;

	return av_interleaved_write_frame(ffm->output, &packet) >= 0;
}

/* ------------------------------------------------------------------------- */

#ifdef _WIN32
int wmain(int argc, wchar_t *argv_w[])
#else
int main(int argc, char *argv[])
#endif
{
	struct ffm_packet_info info = {0};
	struct ffmpeg_mux ffm = {0};
	struct resize_buf rb = {0};
	bool fail = false;
	int ret;

#ifdef _WIN32
	char **argv;

	SetErrorMode(SEM_FAILCRITICALERRORS);

	argv = malloc(argc * sizeof(char*));
	for (int i = 0; i < argc; i++) {
		size_t len = wcslen(argv_w[i]);
		int size;

		size = WideCharToMultiByte(CP_UTF8, 0, argv_w[i], (int)len,
				NULL, 0, NULL, NULL);
		argv[i] = malloc(size + 1);
		WideCharToMultiByte(CP_UTF8, 0, argv_w[i], (int)len, argv[i],
				size + 1, NULL, NULL);
		argv[i][size] = 0;
	}

	_setmode(_fileno(stdin), O_BINARY);
#endif
	setvbuf(stderr, NULL, _IONBF, 0);

	ret = ffmpeg_mux_init(&ffm, argc, argv);
	if (ret != FFM_SUCCESS) {
		puts("Couldn't initialize muxer");
		return ret;
	}

	while (!fail && safe_read(&info, sizeof(info)) == sizeof(info)) {
		resize_buf_resize(&rb, info.size);

		if (safe_read(rb.buf, info.size) == info.size) {
			ffmpeg_mux_packet(&ffm, rb.buf, &info);
		} else {
			fail = true;
		}
	}

	ffmpeg_mux_free(&ffm);
	resize_buf_free(&rb);

#ifdef _WIN32
	for (int i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
#endif
	return 0;
}
