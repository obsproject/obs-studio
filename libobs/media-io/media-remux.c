/******************************************************************************
    Copyright (C) 2023 by Ruwen Hahn <palana@stunned.de>

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

#include "media-remux.h"

#include "../util/base.h"
#include "../util/bmem.h"
#include "../util/platform.h"

#include <libavformat/avformat.h>
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 20, 100)
#include <libavcodec/version.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#ifndef FF_API_BUFFER_SIZE_T
#define FF_API_BUFFER_SIZE_T (LIBAVUTIL_VERSION_MAJOR < 57)
#endif

struct media_remux_job {
	int64_t in_size;
	AVFormatContext *ifmt_ctx, *ofmt_ctx;
};

static inline void init_size(media_remux_job_t job, const char *in_filename)
{
#ifdef _MSC_VER
	struct _stat64 st = {0};
	_stat64(in_filename, &st);
#else
	struct stat st = {0};
	stat(in_filename, &st);
#endif
	job->in_size = st.st_size;
}

static inline bool init_input(media_remux_job_t job, const char *in_filename)
{
	int ret = avformat_open_input(&job->ifmt_ctx, in_filename, NULL, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "media_remux: Could not open input file '%s'",
		     in_filename);
		return false;
	}

	ret = avformat_find_stream_info(job->ifmt_ctx, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "media_remux: Failed to retrieve input stream"
				" information");
		return false;
	}

#ifndef NDEBUG
	av_dump_format(job->ifmt_ctx, 0, in_filename, false);
#endif
	return true;
}

static inline bool init_output(media_remux_job_t job, const char *out_filename)
{
	int ret;

	avformat_alloc_output_context2(&job->ofmt_ctx, NULL, NULL,
				       out_filename);
	if (!job->ofmt_ctx) {
		blog(LOG_ERROR, "media_remux: Could not create output context");
		return false;
	}

	for (unsigned i = 0; i < job->ifmt_ctx->nb_streams; i++) {
		AVStream *in_stream = job->ifmt_ctx->streams[i];
		AVStream *out_stream = avformat_new_stream(job->ofmt_ctx, NULL);
		if (!out_stream) {
			blog(LOG_ERROR, "media_remux: Failed to allocate output"
					" stream");
			return false;
		}

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(60, 31, 102)
#if FF_API_BUFFER_SIZE_T
		int content_size;
#else
		size_t content_size;
#endif
		const uint8_t *const content_src = av_stream_get_side_data(
			in_stream, AV_PKT_DATA_CONTENT_LIGHT_LEVEL,
			&content_size);
		if (content_src) {
			uint8_t *const content_dst = av_stream_new_side_data(
				out_stream, AV_PKT_DATA_CONTENT_LIGHT_LEVEL,
				content_size);
			if (content_dst)
				memcpy(content_dst, content_src, content_size);
		}

#if FF_API_BUFFER_SIZE_T
		int mastering_size;
#else
		size_t mastering_size;
#endif
		const uint8_t *const mastering_src = av_stream_get_side_data(
			in_stream, AV_PKT_DATA_MASTERING_DISPLAY_METADATA,
			&mastering_size);
		if (mastering_src) {
			uint8_t *const mastering_dst = av_stream_new_side_data(
				out_stream,
				AV_PKT_DATA_MASTERING_DISPLAY_METADATA,
				mastering_size);
			if (mastering_dst) {
				memcpy(mastering_dst, mastering_src,
				       mastering_size);
			}
		}
#endif

		ret = avcodec_parameters_copy(out_stream->codecpar,
					      in_stream->codecpar);

		if (ret < 0) {
			blog(LOG_ERROR,
			     "media_remux: Failed to copy parameters");
			return false;
		}

		av_dict_copy(&out_stream->metadata, in_stream->metadata, 0);

		if (in_stream->codecpar->codec_id == AV_CODEC_ID_HEVC &&
		    job->ofmt_ctx->oformat->codec_tag &&
		    av_codec_get_id(job->ofmt_ctx->oformat->codec_tag,
				    MKTAG('h', 'v', 'c', '1')) ==
			    out_stream->codecpar->codec_id) {
			// Tag HEVC files with industry standard HVC1 tag for wider device compatibility
			// when HVC1 tag is supported by out stream codec
			out_stream->codecpar->codec_tag =
				MKTAG('h', 'v', 'c', '1');
		} else {
			// Otherwise tag 0 to let FFmpeg automatically select the appropriate tag
			out_stream->codecpar->codec_tag = 0;
		}

		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 24, 100)
			out_stream->codecpar->channel_layout =
				av_get_default_channel_layout(
					in_stream->codecpar->channels);
			/* The avutil default channel layout for 5 channels is
			 * 5.0, which OBS does not support. Manually set 5
			 * channels to 4.1. */
			if (in_stream->codecpar->channels == 5)
				out_stream->codecpar->channel_layout =
					av_get_channel_layout("4.1");
#else
			av_channel_layout_default(
				&out_stream->codecpar->ch_layout,
				in_stream->codecpar->ch_layout.nb_channels);
			/* The avutil default channel layout for 5 channels is
			 * 5.0, which OBS does not support. Manually set 5
			 * channels to 4.1. */
			if (in_stream->codecpar->ch_layout.nb_channels == 5)
				out_stream->codecpar->ch_layout =
					(AVChannelLayout)
						AV_CHANNEL_LAYOUT_4POINT1;
#endif
		}
	}

#ifndef NDEBUG
	av_dump_format(job->ofmt_ctx, 0, out_filename, true);
#endif

	if (!(job->ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&job->ofmt_ctx->pb, out_filename,
				AVIO_FLAG_WRITE);
		if (ret < 0) {
			blog(LOG_ERROR,
			     "media_remux: Failed to open output"
			     " file '%s'",
			     out_filename);
			return false;
		}
	}

	return true;
}

bool media_remux_job_create(media_remux_job_t *job, const char *in_filename,
			    const char *out_filename)
{
	if (!job)
		return false;

	*job = NULL;
	if (!os_file_exists(in_filename))
		return false;

	if (strcmp(in_filename, out_filename) == 0)
		return false;

	*job = (media_remux_job_t)bzalloc(sizeof(struct media_remux_job));
	if (!*job)
		return false;

	init_size(*job, in_filename);

	if (!init_input(*job, in_filename))
		goto fail;

	if (!init_output(*job, out_filename))
		goto fail;

	return true;

fail:
	media_remux_job_destroy(*job);
	return false;
}

static inline void process_packet(AVPacket *pkt, AVStream *in_stream,
				  AVStream *out_stream)
{
	pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base,
				    out_stream->time_base,
				    AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
	pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base,
				    out_stream->time_base,
				    AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
	pkt->duration = (int)av_rescale_q(pkt->duration, in_stream->time_base,
					  out_stream->time_base);
	pkt->pos = -1;
}

static inline int process_packets(media_remux_job_t job,
				  media_remux_progress_callback callback,
				  void *data)
{
	AVPacket pkt;

	int ret, throttle = 0;
	for (;;) {
		ret = av_read_frame(job->ifmt_ctx, &pkt);
		if (ret < 0) {
			if (ret != AVERROR_EOF)
				blog(LOG_ERROR,
				     "media_remux: Error reading"
				     " packet: %s",
				     av_err2str(ret));
			break;
		}

		if (callback != NULL && throttle++ > 10) {
			float progress = pkt.pos / (float)job->in_size * 100.f;
			if (!callback(data, progress))
				break;
			throttle = 0;
		}

		process_packet(&pkt, job->ifmt_ctx->streams[pkt.stream_index],
			       job->ofmt_ctx->streams[pkt.stream_index]);

		ret = av_interleaved_write_frame(job->ofmt_ctx, &pkt);
		av_packet_unref(&pkt);

		if (ret < 0) {
			blog(LOG_ERROR, "media_remux: Error muxing packet: %s",
			     av_err2str(ret));

			/* Treat "Invalid data found when processing input" and
			 * "Invalid argument" as non-fatal */
			if (ret == AVERROR_INVALIDDATA || ret == -EINVAL)
				continue;

			break;
		}
	}

	return ret;
}

bool media_remux_job_process(media_remux_job_t job,
			     media_remux_progress_callback callback, void *data)
{
	int ret;
	bool success = false;

	if (!job)
		return success;

	ret = avformat_write_header(job->ofmt_ctx, NULL);
	if (ret < 0) {
		blog(LOG_ERROR, "media_remux: Error opening output file: %s",
		     av_err2str(ret));
		return success;
	}

	if (callback != NULL)
		callback(data, 0.f);

	ret = process_packets(job, callback, data);
	success = ret >= 0 || ret == AVERROR_EOF;

	ret = av_write_trailer(job->ofmt_ctx);
	if (ret < 0) {
		blog(LOG_ERROR, "media_remux: av_write_trailer: %s",
		     av_err2str(ret));
		success = false;
	}

	if (callback != NULL)
		callback(data, 100.f);

	return success;
}

void media_remux_job_destroy(media_remux_job_t job)
{
	if (!job)
		return;

	avformat_close_input(&job->ifmt_ctx);

	if (job->ofmt_ctx && !(job->ofmt_ctx->oformat->flags & AVFMT_NOFILE))
		avio_close(job->ofmt_ctx->pb);

	avformat_free_context(job->ofmt_ctx);

	bfree(job);
}
