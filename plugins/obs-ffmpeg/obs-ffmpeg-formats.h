#pragma once

#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>

static inline int64_t rescale_ts(int64_t val, AVCodecContext *context,
				 AVRational new_base)
{
	return av_rescale_q_rnd(val, context->time_base, new_base,
				AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
}

static inline enum AVPixelFormat
obs_to_ffmpeg_video_format(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_I444:
		return AV_PIX_FMT_YUV444P;
	case VIDEO_FORMAT_I412:
		return AV_PIX_FMT_YUV444P12LE;
	case VIDEO_FORMAT_I420:
		return AV_PIX_FMT_YUV420P;
	case VIDEO_FORMAT_NV12:
		return AV_PIX_FMT_NV12;
	case VIDEO_FORMAT_YUY2:
		return AV_PIX_FMT_YUYV422;
	case VIDEO_FORMAT_UYVY:
		return AV_PIX_FMT_UYVY422;
	case VIDEO_FORMAT_YVYU:
		return AV_PIX_FMT_YVYU422;
	case VIDEO_FORMAT_RGBA:
		return AV_PIX_FMT_RGBA;
	case VIDEO_FORMAT_BGRA:
		return AV_PIX_FMT_BGRA;
	case VIDEO_FORMAT_BGRX:
		return AV_PIX_FMT_BGRA;
	case VIDEO_FORMAT_Y800:
		return AV_PIX_FMT_GRAY8;
	case VIDEO_FORMAT_BGR3:
		return AV_PIX_FMT_BGR24;
	case VIDEO_FORMAT_I422:
		return AV_PIX_FMT_YUV422P;
	case VIDEO_FORMAT_I210:
		return AV_PIX_FMT_YUV422P10LE;
	case VIDEO_FORMAT_I40A:
		return AV_PIX_FMT_YUVA420P;
	case VIDEO_FORMAT_I42A:
		return AV_PIX_FMT_YUVA422P;
	case VIDEO_FORMAT_YUVA:
		return AV_PIX_FMT_YUVA444P;
#if LIBAVUTIL_BUILD >= AV_VERSION_INT(56, 31, 100)
	case VIDEO_FORMAT_YA2L:
		return AV_PIX_FMT_YUVA444P12LE;
#endif
	case VIDEO_FORMAT_I010:
		return AV_PIX_FMT_YUV420P10LE;
	case VIDEO_FORMAT_P010:
		return AV_PIX_FMT_P010LE;
	case VIDEO_FORMAT_NONE:
	case VIDEO_FORMAT_AYUV:
	default:
		return AV_PIX_FMT_NONE;
	}
}

static enum AVChromaLocation
determine_chroma_location(enum AVPixelFormat pix_fmt,
			  enum AVColorSpace colorspace)
{
	const AVPixFmtDescriptor *const desc = av_pix_fmt_desc_get(pix_fmt);
	if (desc) {
		const unsigned log_chroma_w = desc->log2_chroma_w;
		const unsigned log_chroma_h = desc->log2_chroma_h;
		switch (log_chroma_h) {
		case 0:
			switch (log_chroma_w) {
			case 0:
				/* 4:4:4 */
				return AVCHROMA_LOC_CENTER;
			case 1:
				/* 4:2:2 */
				return AVCHROMA_LOC_LEFT;
			}
			break;
		case 1:
			if (log_chroma_w == 1) {
				/* 4:2:0 */
				return (colorspace == AVCOL_SPC_BT2020_NCL)
					       ? AVCHROMA_LOC_TOPLEFT
					       : AVCHROMA_LOC_LEFT;
			}
		}
	}

	return AVCHROMA_LOC_UNSPECIFIED;
}

static inline enum audio_format
convert_ffmpeg_sample_format(enum AVSampleFormat format)
{
	switch (format) {
	case AV_SAMPLE_FMT_U8:
		return AUDIO_FORMAT_U8BIT;
	case AV_SAMPLE_FMT_S16:
		return AUDIO_FORMAT_16BIT;
	case AV_SAMPLE_FMT_S32:
		return AUDIO_FORMAT_32BIT;
	case AV_SAMPLE_FMT_FLT:
		return AUDIO_FORMAT_FLOAT;
	case AV_SAMPLE_FMT_U8P:
		return AUDIO_FORMAT_U8BIT_PLANAR;
	case AV_SAMPLE_FMT_S16P:
		return AUDIO_FORMAT_16BIT_PLANAR;
	case AV_SAMPLE_FMT_S32P:
		return AUDIO_FORMAT_32BIT_PLANAR;
	case AV_SAMPLE_FMT_FLTP:
		return AUDIO_FORMAT_FLOAT_PLANAR;
	}

	/* shouldn't get here */
	return AUDIO_FORMAT_16BIT;
}
