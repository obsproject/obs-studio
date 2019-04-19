#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <obs-module.h>

#include "v4l2-decompress.h"

AVInputFormat *input_format;

struct buffer_data {
    uint8_t *ptr;
    size_t pos;
    size_t size;
};

struct ffmpeg_image {
	AVFormatContext    *fmt_ctx;
	AVCodecParameters *codecpar;
	AVCodecContext     *decoder_ctx;
	AVCodec            *decoder;
	AVStream           *stream;
	struct buffer_data bd;
	uint8_t *avio_ctx_buffer;
	int                cx, cy;
	enum AVPixelFormat format;
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;

    if (bd->size - bd->pos == 0) {
        return AVERROR_EOF;
	}

	int to_read = FFMIN(buf_size, FFMIN(4096, bd->size - bd->pos));

    memcpy(buf, bd->ptr + bd->pos, to_read);
    bd->pos += to_read;

    return to_read;
}

static bool ffmpeg_image_reformat_frame(struct ffmpeg_image *image,
		AVFrame *frame, uint8_t *out, int linesize)
{
	struct SwsContext *sws_ctx = NULL;
	int               ret      = 0;

	if (image->format == AV_PIX_FMT_RGBA ||
	    image->format == AV_PIX_FMT_BGRA ||
	    image->format == AV_PIX_FMT_BGR0) {

		if (linesize != frame->linesize[0]) {
			int min_line = linesize < frame->linesize[0] ?
				linesize : frame->linesize[0];

			for (int y = 0; y < image->cy; y++)
				memcpy(out + y * linesize,
				       frame->data[0] + y * frame->linesize[0],
				       min_line);
		} else {
			memcpy(out, frame->data[0], linesize * image->cy);
		}

	} else {
		sws_ctx = sws_getContext(image->cx, image->cy, image->format,
				image->cx, image->cy, AV_PIX_FMT_BGRA,
				SWS_POINT, NULL, NULL, NULL);
		if (!sws_ctx) {
			blog(LOG_WARNING, "Failed to create scale context");
			return false;
		}

		ret = sws_scale(sws_ctx, (const uint8_t *const*)frame->data,
				frame->linesize, 0, image->cy, &out, &linesize);
		sws_freeContext(sws_ctx);

		if (ret < 0) {
			blog(LOG_WARNING, "sws_scale failed: %s", av_err2str(ret));
			return false;
		}

		image->format = AV_PIX_FMT_BGRA;
	}

	return true;
}

static bool ffmpeg_image_decode(struct ffmpeg_image *image, uint8_t *out,
		int linesize)
{
	AVPacket          packet    = {0};
	bool              success   = false;
	AVFrame           *frame    = av_frame_alloc();
	int               got_frame = 0;
	int               ret;

	if (!frame) {
		blog(LOG_WARNING, "Failed to create frame data");
		return false;
	}

	ret = av_read_frame(image->fmt_ctx, &packet);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to read image frame: %s",
				av_err2str(ret));
		goto fail;
	}

	while (!got_frame) {
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)

		ret = avcodec_send_packet(image->decoder_ctx, &packet);
		if (ret == 0)
			ret = avcodec_receive_frame(image->decoder_ctx, frame);

		got_frame = (ret == 0);

		if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
			ret = 0;
#else
		ret = avcodec_decode_video2(image->decoder_ctx, frame,
				&got_frame, &packet);
#endif
		if (ret < 0) {
			blog(LOG_WARNING, "Failed to decode frame: %s",
					av_err2str(ret));
			goto fail;
		}
	}

	success = ffmpeg_image_reformat_frame(image, frame, out, linesize);

fail:
	av_packet_unref(&packet);
	av_frame_free(&frame);
	return success;
}

static void ffmpeg_image_free(struct ffmpeg_image *image)
{
	avcodec_close(image->decoder_ctx);
	avformat_close_input(&image->fmt_ctx);
}

static bool ffmpeg_image_open_decoder_context(struct ffmpeg_image *info)
{
	int ret = av_find_best_stream(info->fmt_ctx, AVMEDIA_TYPE_VIDEO,
			-1, 1, NULL, 0);
	if (ret < 0) {
		blog(LOG_WARNING, "Couldn't find video stream: %s",
				av_err2str(ret));
		return false;
	}

	info->stream      = info->fmt_ctx->streams[ret];
    info->codecpar    = info->stream->codecpar;
	info->decoder_ctx = info->stream->codec;
	info->decoder     = avcodec_find_decoder(info->codecpar->codec_id);

	if (!info->decoder) {
		blog(LOG_WARNING, "Failed to find decoder");
		return false;
	}

	ret = avcodec_open2(info->decoder_ctx, info->decoder, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to open video codec: %s", av_err2str(ret));
		return false;
	}

	return true;
}

int ffmepg_open_buffer(struct ffmpeg_image *image, uint8_t *buffer, int buf_len) {
	image->avio_ctx_buffer = av_malloc(4096);
    if (!image->avio_ctx_buffer) {
        return -1;
    }

    image->bd.ptr = buffer;
	image->bd.pos = 0;
    image->bd.size = buf_len;

	image->fmt_ctx->pb = avio_alloc_context(image->avio_ctx_buffer, 4096, 0, 
			&image->bd, &read_packet, NULL, NULL);

	if(!image->fmt_ctx->pb) {
		return -1;
	}

	return avformat_open_input(&image->fmt_ctx, NULL, input_format, NULL);
}

static bool ffmpeg_image_init(struct ffmpeg_image *image, uint8_t *buffer, int buf_size)
{
	int ret;

	image->fmt_ctx = avformat_alloc_context();
	if (!image->fmt_ctx) {
		return -1;
	}
	image->fmt_ctx->format_probesize = 32;
	image->fmt_ctx->max_ts_probe = 1;

	ret = ffmepg_open_buffer(image, buffer, buf_size);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to open buffer: %s", av_err2str(ret));
		return false;
	}

	ret = avformat_find_stream_info(image->fmt_ctx, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Could not find stream info: %s", av_err2str(ret));
		goto fail;
	}

	if (!ffmpeg_image_open_decoder_context(image))
		goto fail;

	image->cx     = image->decoder_ctx->width;
	image->cy     = image->decoder_ctx->height;
	image->format = image->decoder_ctx->pix_fmt;
	return true;

fail:
	ffmpeg_image_free(image);

	return false;
}

/**
 * Performs MJPEG decompression
 *
 * @param v4l2 buffer
 * @param output
 * @param width
 * @param height
 *
 * @return bool if success
 */
bool v4l2_decompress(uint8_t *buffer, int buf_size, uint8_t *output, int width, int height) {
    bool ret_val = false;

	if (!input_format) {
		av_register_all();
		input_format = av_find_input_format("mjpeg"); 
	}

	struct ffmpeg_image image;
	memset(&image, 0, sizeof(struct ffmpeg_image));

	image.cx = width;
	image.cy = height;

	if (ffmpeg_image_init(&image, buffer, buf_size)) {
		if (ffmpeg_image_decode(&image, output, width * 4)) {
			ret_val = true;
		}
		ffmpeg_image_free(&image);
	}

    return ret_val;
}
