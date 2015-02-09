#pragma once

static const enum AVPixelFormat i420_formats[] = {
	AV_PIX_FMT_YUV420P,
	AV_PIX_FMT_NV12,
	AV_PIX_FMT_NV21,
	AV_PIX_FMT_YUYV422,
	AV_PIX_FMT_UYVY422,
	AV_PIX_FMT_YUV422P,
	AV_PIX_FMT_NONE
};

static const enum AVPixelFormat nv12_formats[] = {
	AV_PIX_FMT_NV12,
	AV_PIX_FMT_NV21,
	AV_PIX_FMT_YUV420P,
	AV_PIX_FMT_YUYV422,
	AV_PIX_FMT_UYVY422,
	AV_PIX_FMT_NONE
};

static const enum AVPixelFormat yuy2_formats[] = {
	AV_PIX_FMT_YUYV422,
	AV_PIX_FMT_UYVY422,
	AV_PIX_FMT_NV12,
	AV_PIX_FMT_NV21,
	AV_PIX_FMT_YUV420P,
	AV_PIX_FMT_NONE
};

static const enum AVPixelFormat uyvy_formats[] = {
	AV_PIX_FMT_UYVY422,
	AV_PIX_FMT_YUYV422,
	AV_PIX_FMT_NV12,
	AV_PIX_FMT_NV21,
	AV_PIX_FMT_YUV420P,
	AV_PIX_FMT_NONE
};

static const enum AVPixelFormat rgba_formats[] = {
	AV_PIX_FMT_RGBA,
	AV_PIX_FMT_BGRA,
	AV_PIX_FMT_YUYV422,
	AV_PIX_FMT_UYVY422,
	AV_PIX_FMT_NV12,
	AV_PIX_FMT_NV21,
	AV_PIX_FMT_NONE
};

static const enum AVPixelFormat bgra_formats[] = {
	AV_PIX_FMT_BGRA,
	AV_PIX_FMT_RGBA,
	AV_PIX_FMT_YUYV422,
	AV_PIX_FMT_UYVY422,
	AV_PIX_FMT_NV12,
	AV_PIX_FMT_NV21,
	AV_PIX_FMT_NONE
};

static enum AVPixelFormat get_best_format(
		const enum AVPixelFormat *best,
		const enum AVPixelFormat *formats)
{
	while (*best != AV_PIX_FMT_NONE) {
		enum AVPixelFormat best_format = *best;
		const enum AVPixelFormat *cur_formats = formats;

		while (*cur_formats != AV_PIX_FMT_NONE) {
			enum AVPixelFormat avail_format = *cur_formats;

			if (best_format == avail_format)
				return best_format;

			cur_formats++;
		}

		best++;
	}

	return AV_PIX_FMT_NONE;
}

static inline enum AVPixelFormat get_closest_format(
		enum AVPixelFormat format,
		const enum AVPixelFormat *formats)
{
	enum AVPixelFormat best_format = AV_PIX_FMT_NONE;

	if (!formats || formats[0] == AV_PIX_FMT_NONE)
		return format;

	switch ((int)format) {

	case AV_PIX_FMT_YUV420P:
		best_format = get_best_format(i420_formats, formats);
		break;
	case AV_PIX_FMT_NV12:
		best_format = get_best_format(nv12_formats, formats);
		break;
	case AV_PIX_FMT_YUYV422:
		best_format = get_best_format(yuy2_formats, formats);
		break;
	case AV_PIX_FMT_UYVY422:
		best_format = get_best_format(uyvy_formats, formats);
		break;
	case AV_PIX_FMT_RGBA:
		best_format = get_best_format(rgba_formats, formats);
		break;
	case AV_PIX_FMT_BGRA:
		best_format = get_best_format(bgra_formats, formats);
		break;
	}

	return (best_format == AV_PIX_FMT_NONE) ? formats[0] : best_format;
}
