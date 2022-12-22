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

#include <stdio.h>
#include <stdlib.h>
#include "ffmpeg-mux.h"

#include <util/threading.h>
#include <util/platform.h>
#include <util/circlebuf.h>
#include <util/dstr.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mastering_display_metadata.h>

#define ANSI_COLOR_RED "\x1b[0;91m"
#define ANSI_COLOR_MAGENTA "\x1b[0;95m"
#define ANSI_COLOR_RESET "\x1b[0m"

#if LIBAVCODEC_VERSION_MAJOR >= 58
#define CODEC_FLAG_GLOBAL_H AV_CODEC_FLAG_GLOBAL_HEADER
#else
#define CODEC_FLAG_GLOBAL_H CODEC_FLAG_GLOBAL_HEADER
#endif

#define AVIO_BUFFER_SIZE 65536

/* ------------------------------------------------------------------------- */

static char *global_stream_key = "";

struct resize_buf {
	uint8_t *buf;
	size_t size;
	size_t capacity;
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
	/* printable_file is file with any stream key information removed */
	struct dstr printable_file;
	int has_video;
	int tracks;
	char *vcodec;
	int vbitrate;
	int gop;
	int width;
	int height;
	int fps_num;
	int fps_den;
	int color_primaries;
	int color_trc;
	int colorspace;
	int color_range;
	int chroma_sample_location;
	int max_luminance;
	char *acodec;
	char *muxer_settings;
	int codec_tag;
};

struct audio_params {
	char *name;
	int abitrate;
	int sample_rate;
	int frame_size;
	int channels;
};

struct header {
	uint8_t *data;
	int size;
};

struct audio_info {
	AVStream *stream;
	AVCodecContext *ctx;
};

struct io_header {
	uint64_t seek_offset;
	size_t data_length;
};

struct io_buffer {
	bool active;
	bool shutdown_requested;
	bool output_error;
	os_event_t *buffer_space_available_event;
	os_event_t *new_data_available_event;
	pthread_t io_thread;
	pthread_mutex_t data_mutex;
	FILE *output_file;
	struct circlebuf data;
	uint64_t next_pos;
};

struct ffmpeg_mux {
	AVFormatContext *output;
	AVStream *video_stream;
	AVCodecContext *video_ctx;
	AVPacket *packet;
	struct audio_info *audio_infos;
	struct main_params params;
	struct audio_params *audio;
	struct header video_header;
	struct header *audio_header;
	int num_audio_streams;
	bool initialized;
	struct io_buffer io;
};

static void header_free(struct header *header)
{
	free(header->data);
}

static void free_avformat(struct ffmpeg_mux *ffm)
{
	if (ffm->output) {
		avcodec_free_context(&ffm->video_ctx);

		if ((ffm->output->oformat->flags & AVFMT_NOFILE) == 0)
			avio_close(ffm->output->pb);

		avformat_free_context(ffm->output);
		ffm->output = NULL;
	}

	if (ffm->audio_infos) {
		for (int i = 0; i < ffm->num_audio_streams; ++i)
			avcodec_free_context(&ffm->audio_infos[i].ctx);

		free(ffm->audio_infos);
	}

	ffm->video_stream = NULL;
	ffm->audio_infos = NULL;
	ffm->num_audio_streams = 0;
}

static void ffmpeg_mux_free(struct ffmpeg_mux *ffm)
{
	if (ffm->initialized) {
		av_write_trailer(ffm->output);
	}

	// If we're writing to a file with the circlebuf, shut it
	// down gracefully
	if (ffm->io.active) {
		os_atomic_set_bool(&ffm->io.shutdown_requested, true);

		// Wakes up the I/O thread and waits for it to finish
		pthread_mutex_lock(&ffm->io.data_mutex);
		os_event_signal(ffm->io.new_data_available_event);
		pthread_mutex_unlock(&ffm->io.data_mutex);
		pthread_join(ffm->io.io_thread, NULL);

		// Cleanup everything else
		av_free(ffm->output->pb->buffer);
		avio_context_free(&ffm->output->pb);

		os_event_destroy(ffm->io.new_data_available_event);
		os_event_destroy(ffm->io.buffer_space_available_event);

		pthread_mutex_destroy(&ffm->io.data_mutex);

		circlebuf_free(&ffm->io.data);
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

	dstr_free(&ffm->params.printable_file);

	av_packet_free(&ffm->packet);

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
	if (!get_opt_int(argc, argv, &audio->frame_size, "audio frame size"))
		return false;
	if (!get_opt_int(argc, argv, &audio->channels, "audio channels"))
		return false;
	return true;
}

static void ffmpeg_log_callback(void *param, int level, const char *format,
				va_list args)
{
#ifdef ENABLE_FFMPEG_MUX_DEBUG
	char out_buffer[4096];
	struct dstr out = {0};

	vsnprintf(out_buffer, sizeof(out_buffer), format, args);
	dstr_copy(&out, out_buffer);
	if (global_stream_key && *global_stream_key) {
		dstr_replace(&out, global_stream_key, "{stream_key}");
	}

	switch (level) {
	case AV_LOG_INFO:
		fprintf(stdout, "info: [ffmpeg_muxer] %s", out.array);
		fflush(stdout);
		break;

	case AV_LOG_WARNING:
		fprintf(stdout, "%swarning: [ffmpeg_muxer] %s%s",
			ANSI_COLOR_MAGENTA, out.array, ANSI_COLOR_RESET);
		fflush(stdout);
		break;

	case AV_LOG_ERROR:
		fprintf(stderr, "%serror: [ffmpeg_muxer] %s%s", ANSI_COLOR_RED,
			out.array, ANSI_COLOR_RESET);
		fflush(stderr);
	}

	dstr_free(&out);
#else
	UNUSED_PARAMETER(level);
	UNUSED_PARAMETER(format);
	UNUSED_PARAMETER(args);
#endif
	UNUSED_PARAMETER(param);
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
		if (!get_opt_int(argc, argv, &params->vbitrate,
				 "video bitrate"))
			return false;
		if (!get_opt_int(argc, argv, &params->width, "video width"))
			return false;
		if (!get_opt_int(argc, argv, &params->height, "video height"))
			return false;
		if (!get_opt_int(argc, argv, &params->color_primaries,
				 "video color primaries"))
			return false;
		if (!get_opt_int(argc, argv, &params->color_trc,
				 "video color trc"))
			return false;
		if (!get_opt_int(argc, argv, &params->colorspace,
				 "video colorspace"))
			return false;
		if (!get_opt_int(argc, argv, &params->color_range,
				 "video color range"))
			return false;
		if (!get_opt_int(argc, argv, &params->chroma_sample_location,
				 "video chroma sample location"))
			return false;
		if (!get_opt_int(argc, argv, &params->max_luminance,
				 "video max luminance"))
			return false;
		if (!get_opt_int(argc, argv, &params->fps_num, "video fps num"))
			return false;
		if (!get_opt_int(argc, argv, &params->fps_den, "video fps den"))
			return false;
		if (!get_opt_int(argc, argv, &params->codec_tag,
				 "video codec tag"))
			params->codec_tag = 0;
	}

	if (params->tracks) {
		if (!get_opt_str(argc, argv, &params->acodec, "audio codec"))
			return false;

		audio = calloc(params->tracks, sizeof(*audio));

		for (int i = 0; i < params->tracks; i++) {
			if (!get_audio_params(&audio[i], argc, argv)) {
				free(audio);
				return false;
			}
		}
	}

	*p_audio = audio;

	dstr_copy(&params->printable_file, params->file);

	get_opt_str(argc, argv, &global_stream_key, "stream key");
	if (strcmp(global_stream_key, "") != 0) {
		dstr_replace(&params->printable_file, global_stream_key,
			     "{stream_key}");
	}

	av_log_set_callback(ffmpeg_log_callback);

	get_opt_str(argc, argv, &params->muxer_settings, "muxer settings");

	return true;
}

static bool new_stream(struct ffmpeg_mux *ffm, AVStream **stream,
		       const char *name)
{
	*stream = avformat_new_stream(ffm->output, NULL);
	if (!*stream) {
		fprintf(stderr, "Couldn't create stream for encoder '%s'\n",
			name);
		return false;
	}

	(*stream)->id = ffm->output->nb_streams - 1;
	return true;
}

static void create_video_stream(struct ffmpeg_mux *ffm)
{
	AVCodecContext *context;
	void *extradata = NULL;
	const char *name = ffm->params.vcodec;

	const AVCodecDescriptor *codec = avcodec_descriptor_get_by_name(name);
	if (!codec) {
		fprintf(stderr, "Couldn't find codec '%s'\n", name);
		return;
	}

	if (!new_stream(ffm, &ffm->video_stream, name))
		return;

	if (ffm->video_header.size) {
		extradata = av_memdup(ffm->video_header.data,
				      ffm->video_header.size);
	}

	context = avcodec_alloc_context3(NULL);
	context->codec_type = codec->type;
	context->codec_id = codec->id;
	context->codec_tag = ffm->params.codec_tag;
	context->bit_rate = (int64_t)ffm->params.vbitrate * 1000;
	context->width = ffm->params.width;
	context->height = ffm->params.height;
	context->coded_width = ffm->params.width;
	context->coded_height = ffm->params.height;
	context->color_primaries = ffm->params.color_primaries;
	context->color_trc = ffm->params.color_trc;
	context->colorspace = ffm->params.colorspace;
	context->color_range = ffm->params.color_range;
	context->chroma_sample_location = ffm->params.chroma_sample_location;
	context->extradata = extradata;
	context->extradata_size = ffm->video_header.size;
	context->time_base =
		(AVRational){ffm->params.fps_den, ffm->params.fps_num};

	ffm->video_stream->time_base = context->time_base;
#if LIBAVFORMAT_VERSION_MAJOR < 59
	// codec->time_base may still be used if LIBAVFORMAT_VERSION_MAJOR < 59
	ffm->video_stream->codec->time_base = context->time_base;
#endif
	ffm->video_stream->avg_frame_rate = av_inv_q(context->time_base);

	const int max_luminance = ffm->params.max_luminance;
	if (max_luminance > 0) {
		size_t content_size;
		AVContentLightMetadata *const content =
			av_content_light_metadata_alloc(&content_size);
		content->MaxCLL = max_luminance;
		content->MaxFALL = max_luminance;
		av_stream_add_side_data(ffm->video_stream,
					AV_PKT_DATA_CONTENT_LIGHT_LEVEL,
					(uint8_t *)content, content_size);

		AVMasteringDisplayMetadata *const mastering =
			av_mastering_display_metadata_alloc();
		mastering->display_primaries[0][0] = av_make_q(17, 25);
		mastering->display_primaries[0][1] = av_make_q(8, 25);
		mastering->display_primaries[1][0] = av_make_q(53, 200);
		mastering->display_primaries[1][1] = av_make_q(69, 100);
		mastering->display_primaries[2][0] = av_make_q(3, 20);
		mastering->display_primaries[2][1] = av_make_q(3, 50);
		mastering->white_point[0] = av_make_q(3127, 10000);
		mastering->white_point[1] = av_make_q(329, 1000);
		mastering->min_luminance = av_make_q(0, 1);
		mastering->max_luminance = av_make_q(max_luminance, 1);
		mastering->has_primaries = 1;
		mastering->has_luminance = 1;
		av_stream_add_side_data(ffm->video_stream,
					AV_PKT_DATA_MASTERING_DISPLAY_METADATA,
					(uint8_t *)mastering,
					sizeof(*mastering));
	}

	if (ffm->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_H;

	avcodec_parameters_from_context(ffm->video_stream->codecpar, context);

	ffm->video_ctx = context;
}

static void create_audio_stream(struct ffmpeg_mux *ffm, int idx)
{
	AVCodecContext *context;
	AVStream *stream;
	void *extradata = NULL;
	const char *name = ffm->params.acodec;
	int channels;

	const AVCodecDescriptor *codec = avcodec_descriptor_get_by_name(name);
	if (!codec) {
		fprintf(stderr, "Couldn't find codec '%s'\n", name);
		return;
	}

	if (!new_stream(ffm, &stream, name))
		return;

	av_dict_set(&stream->metadata, "title", ffm->audio[idx].name, 0);

	stream->time_base = (AVRational){1, ffm->audio[idx].sample_rate};

	if (ffm->audio_header[idx].size) {
		extradata = av_memdup(ffm->audio_header[idx].data,
				      ffm->audio_header[idx].size);
	}

	context = avcodec_alloc_context3(NULL);
	context->codec_type = codec->type;
	context->codec_id = codec->id;
	context->bit_rate = (int64_t)ffm->audio[idx].abitrate * 1000;
	channels = ffm->audio[idx].channels;
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(57, 24, 100)
	context->channels = channels;
#endif
	context->sample_rate = ffm->audio[idx].sample_rate;
	context->frame_size = ffm->audio[idx].frame_size;
	context->sample_fmt = AV_SAMPLE_FMT_S16;
	context->time_base = stream->time_base;
	context->extradata = extradata;
	context->extradata_size = ffm->audio_header[idx].size;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 24, 100)
	context->channel_layout = av_get_default_channel_layout(channels);
	//avutil default channel layout for 5 channels is 5.0 ; fix for 4.1
	if (channels == 5)
		context->channel_layout = av_get_channel_layout("4.1");
#else
	av_channel_layout_default(&context->ch_layout, channels);
	//avutil default channel layout for 5 channels is 5.0 ; fix for 4.1
	if (channels == 5)
		context->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_4POINT1;
#endif
	if (ffm->output->oformat->flags & AVFMT_GLOBALHEADER)
		context->flags |= CODEC_FLAG_GLOBAL_H;

	avcodec_parameters_from_context(stream->codecpar, context);

	ffm->audio_infos[ffm->num_audio_streams].stream = stream;
	ffm->audio_infos[ffm->num_audio_streams].ctx = context;
	ffm->num_audio_streams++;
}

static bool init_streams(struct ffmpeg_mux *ffm)
{
	if (ffm->params.has_video)
		create_video_stream(ffm);

	if (ffm->params.tracks) {
		ffm->audio_infos =
			calloc(ffm->params.tracks, sizeof(*ffm->audio_infos));

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
	size_t total = size;

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

#define CHUNK_SIZE 1048576

static void *ffmpeg_mux_io_thread(void *data)
{
	struct ffmpeg_mux *ffm = data;

	// Chunk collects the writes into a larger batch
	size_t chunk_used = 0;

	unsigned char *chunk = malloc(CHUNK_SIZE);
	if (!chunk) {
		os_atomic_set_bool(&ffm->io.output_error, true);
		fprintf(stderr, "Error allocating memory for output\n");
		goto error;
	}

	bool shutting_down;
	bool want_seek = false;
	bool force_flush_chunk = false;

	// current_seek_position is a virtual position updated as we read from
	// the buffer, if it becomes discontinuous due to a seek request from
	// ffmpeg, then we flush the chunk. next_seek_position is the actual
	// offset we should seek to when we write the chunk.
	uint64_t current_seek_position = 0;
	uint64_t next_seek_position;

	for (;;) {
		// Wait for ffmpeg to write data to the buffer
		os_event_wait(ffm->io.new_data_available_event);

		// Loop to write in chunk_size chunks
		for (;;) {
			shutting_down = os_atomic_load_bool(
				&ffm->io.shutdown_requested);

			pthread_mutex_lock(&ffm->io.data_mutex);

			// Fetch as many writes as possible from the circlebuf
			// and fill up our local chunk. This may involve seeking
			// if ffmpeg needs to, so take care of that as well.
			for (;;) {
				size_t available = ffm->io.data.size;

				// Buffer is empty (now) or was already empty (we got
				// woken up to exit)
				if (!available)
					break;

				// Get seek offset and data size
				struct io_header header;
				circlebuf_peek_front(&ffm->io.data, &header,
						     sizeof(header));

				// Do we need to seek?
				if (header.seek_offset !=
				    current_seek_position) {

					// If there's already part of a chunk pending,
					// flush it at the current offset. Similarly,
					// if we already plan to seek, then seek.
					if (chunk_used || want_seek) {
						force_flush_chunk = true;
						break;
					}

					// Mark that we need to seek and where to
					want_seek = true;
					next_seek_position = header.seek_offset;

					// Update our virtual position
					current_seek_position =
						header.seek_offset;
				}

				// Make sure there's enough room for the data, if
				// not then force a flush
				if (header.data_length + chunk_used >
				    CHUNK_SIZE) {
					force_flush_chunk = true;
					break;
				}

				// Remove header that we already read
				circlebuf_pop_front(&ffm->io.data, NULL,
						    sizeof(header));

				// Copy from the buffer to our local chunk
				circlebuf_pop_front(&ffm->io.data,
						    chunk + chunk_used,
						    header.data_length);

				// Update offsets
				chunk_used += header.data_length;
				current_seek_position += header.data_length;
			}

			// Signal that there is more room in the buffer
			os_event_signal(ffm->io.buffer_space_available_event);

			// Try to avoid lots of small writes unless this was the final
			// data left in the buffer. The buffer might be entirely empty
			// if we were woken up to exit.
			if (!force_flush_chunk &&
			    (!chunk_used ||
			     (chunk_used < 65536 && !shutting_down))) {
				os_event_reset(
					ffm->io.new_data_available_event);
				pthread_mutex_unlock(&ffm->io.data_mutex);
				break;
			}

			pthread_mutex_unlock(&ffm->io.data_mutex);

			// Seek if we need to
			if (want_seek) {
				os_fseeki64(ffm->io.output_file,
					    next_seek_position, SEEK_SET);

				// Update the next virtual position, making sure to take
				// into account the size of the chunk we're about to write.
				current_seek_position =
					next_seek_position + chunk_used;

				want_seek = false;
			}

			// Write the current chunk to the output file
			if (fwrite(chunk, chunk_used, 1, ffm->io.output_file) !=
			    1) {
				os_atomic_set_bool(&ffm->io.output_error, true);
				fprintf(stderr, "Error writing to '%s', %s\n",
					ffm->params.printable_file.array,
					strerror(errno));
				goto error;
			}

			chunk_used = 0;
			force_flush_chunk = false;
		}

		// If this was the last chunk, time to exit
		if (shutting_down)
			break;
	}

error:
	if (chunk)
		free(chunk);

	fclose(ffm->io.output_file);
	return NULL;
}

static int64_t ffmpeg_mux_seek_av_buffer(void *opaque, int64_t offset,
					 int whence)
{
	struct ffmpeg_mux *ffm = opaque;

	// If the output thread failed, signal that back up the stack
	if (os_atomic_load_bool(&ffm->io.output_error))
		return -1;

	// Update where the next write should go
	pthread_mutex_lock(&ffm->io.data_mutex);
	if (whence == SEEK_SET)
		ffm->io.next_pos = offset;
	else if (whence == SEEK_CUR)
		ffm->io.next_pos += offset;
	pthread_mutex_unlock(&ffm->io.data_mutex);

	return 0;
}

static int ffmpeg_mux_write_av_buffer(void *opaque, uint8_t *buf, int buf_size)
{
	struct ffmpeg_mux *ffm = opaque;

	// If the output thread failed, signal that back up the stack
	if (os_atomic_load_bool(&ffm->io.output_error))
		return -1;

	for (;;) {
		pthread_mutex_lock(&ffm->io.data_mutex);

		// Avoid unbounded growth of the circlebuf, cap to 256 MB
		if (ffm->io.data.capacity >= 256 * 1048576 &&
		    ffm->io.data.capacity - ffm->io.data.size <
			    buf_size + sizeof(struct io_header)) {
			// No space, wait for the I/O thread to make space
			os_event_reset(ffm->io.buffer_space_available_event);
			pthread_mutex_unlock(&ffm->io.data_mutex);
			os_event_wait(ffm->io.buffer_space_available_event);
		} else {
			break;
		}
	}

	struct io_header header;

	header.data_length = buf_size;
	header.seek_offset = ffm->io.next_pos;

	// Copy the data into the buffer
	circlebuf_push_back(&ffm->io.data, &header, sizeof(header));
	circlebuf_push_back(&ffm->io.data, buf, buf_size);

	// Advance the next write position
	ffm->io.next_pos += buf_size;

	// Tell the I/O thread that there's new data to be written
	os_event_signal(ffm->io.new_data_available_event);

	pthread_mutex_unlock(&ffm->io.data_mutex);

	return buf_size;
}

#define SRT_PROTO "srt"
#define UDP_PROTO "udp"
#define TCP_PROTO "tcp"
#define HTTP_PROTO "http"
#define RIST_PROTO "rist"

static bool ffmpeg_mux_is_network(struct ffmpeg_mux *ffm)
{
	return !strncmp(ffm->params.file, SRT_PROTO, sizeof(SRT_PROTO) - 1) ||
	       !strncmp(ffm->params.file, UDP_PROTO, sizeof(UDP_PROTO) - 1) ||
	       !strncmp(ffm->params.file, TCP_PROTO, sizeof(TCP_PROTO) - 1) ||
	       !strncmp(ffm->params.file, HTTP_PROTO, sizeof(HTTP_PROTO) - 1) ||
	       !strncmp(ffm->params.file, RIST_PROTO, sizeof(RIST_PROTO) - 1);
}

static inline int open_output_file(struct ffmpeg_mux *ffm)
{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(59, 0, 100)
	AVOutputFormat *format = ffm->output->oformat;
#else
	const AVOutputFormat *format = ffm->output->oformat;
#endif
	int ret;

	if ((format->flags & AVFMT_NOFILE) == 0) {
		if (!ffmpeg_mux_is_network(ffm)) {
			// If not outputting to a network, write to a circlebuf
			// instead of relying on ffmpeg disk output. This hopefully
			// works around too small buffers somewhere causing output
			// stalls when recording.

			// We're in charge of managing the actual file now
			ffm->io.output_file = os_fopen(ffm->params.file, "wb");
			if (!ffm->io.output_file) {
				fprintf(stderr, "Couldn't open '%s', %s\n",
					ffm->params.printable_file.array,
					strerror(errno));
				return FFM_ERROR;
			}

			// Start at 1MB, this can grow up to 256 MB depending
			// how fast data is going in and out (limited in
			// ffmpeg_mux_write_av_buffer)
			circlebuf_reserve(&ffm->io.data, 1048576);

			pthread_mutex_init(&ffm->io.data_mutex, NULL);

			os_event_init(&ffm->io.buffer_space_available_event,
				      OS_EVENT_TYPE_AUTO);
			os_event_init(&ffm->io.new_data_available_event,
				      OS_EVENT_TYPE_AUTO);

			pthread_create(&ffm->io.io_thread, NULL,
				       ffmpeg_mux_io_thread, ffm);

			unsigned char *avio_ctx_buffer =
				av_malloc(AVIO_BUFFER_SIZE);

			ffm->output->pb = avio_alloc_context(
				avio_ctx_buffer, AVIO_BUFFER_SIZE, 1, ffm, NULL,
				ffmpeg_mux_write_av_buffer,
				ffmpeg_mux_seek_av_buffer);

			ffm->io.active = true;
		} else {
			ret = avio_open(&ffm->output->pb, ffm->params.file,
					AVIO_FLAG_WRITE);
			if (ret < 0) {
				fprintf(stderr, "Couldn't open '%s', %s\n",
					ffm->params.printable_file.array,
					av_err2str(ret));
				return FFM_ERROR;
			}
		}
	}

	AVDictionary *dict = NULL;
	if ((ret = av_dict_parse_string(&dict, ffm->params.muxer_settings, "=",
					" ", 0))) {
		fprintf(stderr, "Failed to parse muxer settings: %s\n%s\n",
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
		fprintf(stderr, "Error opening '%s': %s",
			ffm->params.printable_file.array, av_err2str(ret));

		av_dict_free(&dict);

		return ret == -22 ? FFM_UNSUPPORTED : FFM_ERROR;
	}

	av_dict_free(&dict);

	return FFM_SUCCESS;
}

static int ffmpeg_mux_init_context(struct ffmpeg_mux *ffm)
{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(59, 0, 100)
	AVOutputFormat *output_format;
#else
	const AVOutputFormat *output_format;
#endif
	int ret;
	bool is_http = false;
	is_http = (strncmp(ffm->params.file, HTTP_PROTO,
			   sizeof(HTTP_PROTO) - 1) == 0);

	bool is_network = ffmpeg_mux_is_network(ffm);

	if (is_network) {
		avformat_network_init();
	}

	if (is_network && !is_http)
		output_format = av_guess_format("mpegts", NULL, "video/M2PT");
	else
		output_format = av_guess_format(NULL, ffm->params.file, NULL);

	if (output_format == NULL) {
		fprintf(stderr, "Couldn't find an appropriate muxer for '%s'\n",
			ffm->params.printable_file.array);
		return FFM_ERROR;
	}

#ifdef ENABLE_FFMPEG_MUX_DEBUG
	printf("info: Output format name and long_name: %s, %s\n",
	       output_format->name ? output_format->name : "unknown",
	       output_format->long_name ? output_format->long_name : "unknown");
#endif

	ret = avformat_alloc_output_context2(&ffm->output, output_format, NULL,
					     ffm->params.file);
	if (ret < 0) {
		fprintf(stderr, "Couldn't initialize output context: %s\n",
			av_err2str(ret));
		return FFM_ERROR;
	}

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(59, 0, 100)
	ffm->output->oformat->video_codec = AV_CODEC_ID_NONE;
	ffm->output->oformat->audio_codec = AV_CODEC_ID_NONE;
#endif

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
			calloc(ffm->params.tracks, sizeof(*ffm->audio_header));
	}

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
#endif

	if (!ffmpeg_mux_get_extra_data(ffm))
		return FFM_ERROR;

	ffm->packet = av_packet_alloc();

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
			return ffm->audio_infos[info->index].stream->id;
		}
	}

	return -1;
}

static AVCodecContext *get_codec_context(struct ffmpeg_mux *ffm,
					 struct ffm_packet_info *info)
{
	if (info->type == FFM_PACKET_VIDEO) {
		if (ffm->video_stream) {
			return ffm->video_ctx;
		}
	} else {
		if ((int)info->index < ffm->num_audio_streams) {
			return ffm->audio_infos[info->index].ctx;
		}
	}

	return NULL;
}

static inline AVStream *get_stream(struct ffmpeg_mux *ffm, int idx)
{
	return ffm->output->streams[idx];
}

static inline int64_t rescale_ts(struct ffmpeg_mux *ffm,
				 AVRational codec_time_base, int64_t val,
				 int idx)
{
	AVStream *stream = get_stream(ffm, idx);

	return av_rescale_q_rnd(val / codec_time_base.num, codec_time_base,
				stream->time_base,
				AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
}

static inline bool ffmpeg_mux_packet(struct ffmpeg_mux *ffm, uint8_t *buf,
				     struct ffm_packet_info *info)
{
	int idx = get_index(ffm, info);

	/* The muxer might not support video/audio, or multiple audio tracks */
	if (idx == -1) {
		return true;
	}

	const AVRational codec_time_base =
		get_codec_context(ffm, info)->time_base;

	ffm->packet->data = buf;
	ffm->packet->size = (int)info->size;
	ffm->packet->stream_index = idx;
	ffm->packet->pts = rescale_ts(ffm, codec_time_base, info->pts, idx);
	ffm->packet->dts = rescale_ts(ffm, codec_time_base, info->dts, idx);

	if (info->keyframe)
		ffm->packet->flags = AV_PKT_FLAG_KEY;

	int ret = av_interleaved_write_frame(ffm->output, ffm->packet);

	/* Treat "Invalid data found when processing input" and "Invalid argument" as non-fatal */
	if (ret == AVERROR_INVALIDDATA || ret == -EINVAL) {
		return true;
	}

	if (ret < 0) {
		fprintf(stderr, "av_interleaved_write_frame failed: %d: %s\n",
			ret, av_err2str(ret));
	}

	return ret >= 0;
}

static inline bool read_change_file(struct ffmpeg_mux *ffm, uint32_t size,
				    struct resize_buf *filename, int argc,
				    char **argv)
{
	resize_buf_resize(filename, size + 1);
	if (safe_read(filename->buf, size) != size) {
		return false;
	}
	filename->buf[size] = 0;

#ifdef ENABLE_FFMPEG_MUX_DEBUG
	fprintf(stderr, "info: New output file name: %s\n", filename->buf);
#endif

	int ret;
	char *argv1_backup = argv[1];
	argv[1] = (char *)filename->buf;

	ffmpeg_mux_free(ffm);

	ret = ffmpeg_mux_init(ffm, argc, argv);
	if (ret != FFM_SUCCESS) {
		fprintf(stderr, "Couldn't initialize muxer\n");
		return false;
	}

	argv[1] = argv1_backup;

	return true;
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
	struct resize_buf rb_filename = {0};
	bool fail = false;
	int ret;

#ifdef _WIN32
	char **argv;

	SetErrorMode(SEM_FAILCRITICALERRORS);

	argv = malloc(argc * sizeof(char *));
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
		fprintf(stderr, "Couldn't initialize muxer\n");
		return ret;
	}

	while (!fail && safe_read(&info, sizeof(info)) == sizeof(info)) {
		if (info.type == FFM_PACKET_CHANGE_FILE) {
			fail = !read_change_file(&ffm, info.size, &rb_filename,
						 argc, argv);
			continue;
		}

		resize_buf_resize(&rb, info.size);

		if (safe_read(rb.buf, info.size) == info.size) {
			fail = !ffmpeg_mux_packet(&ffm, rb.buf, &info);
		} else {
			fail = true;
		}
	}

	ffmpeg_mux_free(&ffm);
	resize_buf_free(&rb);
	resize_buf_free(&rb_filename);

#ifdef _WIN32
	for (int i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
#endif
	return 0;
}
