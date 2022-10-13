#include "graphics.h"

#include "half.h"
#include "srgb.h"
#include <obs-ffmpeg-compat.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#ifdef _WIN32
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#endif

struct ffmpeg_image {
	const char *file;
	AVFormatContext *fmt_ctx;
	AVCodecContext *decoder_ctx;

	int cx, cy;
	enum AVPixelFormat format;
};

static bool ffmpeg_image_open_decoder_context(struct ffmpeg_image *info)
{
	AVFormatContext *const fmt_ctx = info->fmt_ctx;
	int ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, 1, NULL,
				      0);
	if (ret < 0) {
		blog(LOG_WARNING, "Couldn't find video stream in file '%s': %s",
		     info->file, av_err2str(ret));
		return false;
	}

	AVStream *const stream = fmt_ctx->streams[ret];
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
	AVCodecParameters *const codecpar = stream->codecpar;
	const AVCodec *const decoder = avcodec_find_decoder(
		codecpar->codec_id); // fix discarded-qualifiers
#else
	AVCodecContext *const decoder_ctx = stream->codec;
	AVCodec *const decoder = avcodec_find_decoder(decoder_ctx->codec_id);
#endif
	if (!decoder) {
		blog(LOG_WARNING, "Failed to find decoder for file '%s'",
		     info->file);
		return false;
	}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
	AVCodecContext *const decoder_ctx = avcodec_alloc_context3(decoder);
	avcodec_parameters_to_context(decoder_ctx, codecpar);
#endif

	info->decoder_ctx = decoder_ctx;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
	info->cx = codecpar->width;
	info->cy = codecpar->height;
	info->format = codecpar->format;
#else
	info->cx = decoder_ctx->width;
	info->cy = decoder_ctx->height;
	info->format = decoder_ctx->pix_fmt;
#endif

	ret = avcodec_open2(decoder_ctx, decoder, NULL);
	if (ret < 0) {
		blog(LOG_WARNING,
		     "Failed to open video codec for file '%s': "
		     "%s",
		     info->file, av_err2str(ret));
		return false;
	}

	return true;
}

static void ffmpeg_image_free(struct ffmpeg_image *info)
{
	avcodec_free_context(&info->decoder_ctx);
	avformat_close_input(&info->fmt_ctx);
}

static bool ffmpeg_image_init(struct ffmpeg_image *info, const char *file)
{
	int ret;

	if (!file || !*file)
		return false;

	memset(info, 0, sizeof(struct ffmpeg_image));
	info->file = file;

	ret = avformat_open_input(&info->fmt_ctx, file, NULL, NULL);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to open file '%s': %s", info->file,
		     av_err2str(ret));
		return false;
	}

	ret = avformat_find_stream_info(info->fmt_ctx, NULL);
	if (ret < 0) {
		blog(LOG_WARNING,
		     "Could not find stream info for file '%s':"
		     " %s",
		     info->file, av_err2str(ret));
		goto fail;
	}

	if (!ffmpeg_image_open_decoder_context(info))
		goto fail;

	return true;

fail:
	ffmpeg_image_free(info);
	return false;
}

#ifdef _MSC_VER
#define obs_bswap16(v) _byteswap_ushort(v)
#else
#define obs_bswap16(v) __builtin_bswap16(v)
#endif

static void *ffmpeg_image_copy_data_straight(struct ffmpeg_image *info,
					     AVFrame *frame)
{
	const size_t linesize = (size_t)info->cx * 4;
	const size_t totalsize = info->cy * linesize;
	void *data = bmalloc(totalsize);

	const size_t src_linesize = frame->linesize[0];
	if (linesize != src_linesize) {
		const size_t min_line = linesize < src_linesize ? linesize
								: src_linesize;

		uint8_t *dst = data;
		const uint8_t *src = frame->data[0];
		for (int y = 0; y < info->cy; y++) {
			memcpy(dst, src, min_line);
			dst += linesize;
			src += src_linesize;
		}
	} else {
		memcpy(data, frame->data[0], totalsize);
	}

	return data;
}

static inline size_t get_dst_position(const size_t w, const size_t h,
				      const size_t x, const size_t y,
				      int orient)
{
	size_t res_x = 0;
	size_t res_y = 0;

	if (orient == 2) {
		/*
		 * Orientation 2: Flip X
		 *
		 * 888888        888888
		 *     88   ->   88
		 *   8888   ->   8888
		 *     88   ->   88
		 *     88        88
		 *
		 * (0, 0) -> (w, 0)
		 * (0, h) -> (w, h)
		 * (w, 0) -> (0, 0)
		 * (w, h) -> (0, h)
		 *
		 * (w - x, y)
		 */

		res_x = w - 1 - x;
		res_y = y;

	} else if (orient == 3) {
		/*
		 * Orientation 3: 180 degree
		 *
		 *     88        888888
		 *     88   ->   88
		 *   8888   ->   8888
		 *     88   ->   88
		 * 888888        88
		 *
		 * (0, 0) -> (w, h)
		 * (0, h) -> (w, 0)
		 * (w, 0) -> (0, h)
		 * (w, h) -> (0, 0)
		 *
		 * (w - x, h - y)
		 */

		res_x = w - 1 - x;
		res_y = h - 1 - y;

	} else if (orient == 4) {
		/*
		 * Orientation 4: Flip Y
		 *
		 * 88            888888
		 * 88       ->   88
		 * 8888     ->   8888
		 * 88       ->   88
		 * 888888        88
		 *
		 * (0, 0) -> (0, h)
		 * (0, h) -> (0, 0)
		 * (w, 0) -> (w, h)
		 * (w, h) -> (w, 0)
		 *
		 * (x, h - y)
		 */

		res_x = x;
		res_y = h - 1 - y;

	} else if (orient == 5) {
		/*
		 * Orientation 5: Flip Y + 90 degree CW
		 *
		 * 8888888888        888888
		 * 88  88       ->   88
		 * 88           ->   8888
		 *              ->   88
		 *                   88
		 *
		 * (0, 0) -> (0, 0)
		 * (0, h) -> (w, 0)
		 * (w, 0) -> (0, h)
		 * (w, h) -> (w, h)
		 *
		 * (y, x)
		 */

		res_x = y;
		res_y = x;

	} else if (orient == 6) {
		/*
		 * Orientation 6: 90 degree CW
		 *
		 * 88                888888
		 * 88  88       ->   88
		 * 8888888888   ->   8888
		 *              ->   88
		 *                   88
		 *
		 * (0, 0) -> (w, 0)
		 * (0, h) -> (0, 0)
		 * (w, 0) -> (w, h)
		 * (w, h) -> (0, h)
		 *
		 * (w - y, x)
		 */

		res_x = w - 1 - y;
		res_y = x;

	} else if (orient == 7) {
		/*
		 * Orientation 7: Flip Y + 90 degree CCW
		 *
		 *         88        888888
		 *     88  88   ->   88
		 * 8888888888   ->   8888
		 *              ->   88
		 *                   88
		 *
		 * (0, 0) -> (w, h)
		 * (0, h) -> (0, h)
		 * (w, 0) -> (w, 0)
		 * (w, h) -> (0, 0)
		 *
		 * (w - y, h - x)
		 */

		res_x = w - 1 - y;
		res_y = h - 1 - x;

	} else if (orient == 8) {
		/*
		 * Orientation 8: 90 degree CCW
		 *
		 * 8888888888        888888
		 *     88  88   ->   88
		 *         88   ->   8888
		 *              ->   88
		 *                   88
		 *
		 * (0, 0) -> (0, h)
		 * (0, h) -> (w, h)
		 * (w, 0) -> (0, 0)
		 * (w, h) -> (w, 0)
		 *
		 * (y, h - x)
		 */

		res_x = y;
		res_y = h - 1 - x;
	}

	return (res_x + res_y * w) * 4;
}

#define TILE_SIZE 16
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static void *ffmpeg_image_orient(struct ffmpeg_image *info, void *in_data,
				 int orient)
{
	const size_t sx = (size_t)info->cx;
	const size_t sy = (size_t)info->cy;
	uint8_t *data = NULL;

	if (orient == 0 || orient == 1)
		return in_data;

	data = bmalloc(sx * 4 * sy);

	if (orient >= 5 && orient < 9) {
		info->cx = (int)sy;
		info->cy = (int)sx;
	}

	uint8_t *src = in_data;

	size_t off_dst;
	size_t off_src = 0;
	for (size_t y0 = 0; y0 < sy; y0 += TILE_SIZE) {
		for (size_t x0 = 0; x0 < sx; x0 += TILE_SIZE) {
			size_t lim_x = MIN((size_t)sx, x0 + TILE_SIZE);
			size_t lim_y = MIN((size_t)sy, y0 + TILE_SIZE);

			for (size_t y = y0; y < lim_y; y++) {
				for (size_t x = x0; x < lim_x; x++) {
					off_src = (x + y * sx) * 4;

					off_dst = get_dst_position(info->cx,
								   info->cy, x,
								   y, orient);

					memcpy(data + off_dst, src + off_src,
					       4);
				}
			}
		}
	}

	bfree(in_data);
	return data;
}

static void *ffmpeg_image_reformat_frame(struct ffmpeg_image *info,
					 AVFrame *frame,
					 enum gs_image_alpha_mode alpha_mode)
{
	struct SwsContext *sws_ctx = NULL;
	void *data = NULL;
	int ret = 0;

	AVDictionary *dict = frame->metadata;
	AVDictionaryEntry *entry = NULL;

	int orient = 0;

	if (dict) {
		entry = av_dict_get(dict, "Orientation", NULL,
				    AV_DICT_MATCH_CASE);
		if (entry && entry->value) {
			orient = atoi(entry->value);
		}
	}

	if (info->format == AV_PIX_FMT_BGR0) {
		data = ffmpeg_image_copy_data_straight(info, frame);
	} else if (info->format == AV_PIX_FMT_RGBA ||
		   info->format == AV_PIX_FMT_BGRA) {
		if (alpha_mode == GS_IMAGE_ALPHA_STRAIGHT) {
			data = ffmpeg_image_copy_data_straight(info, frame);
		} else {
			const size_t linesize = (size_t)info->cx * 4;
			const size_t totalsize = info->cy * linesize;
			data = bmalloc(totalsize);
			const size_t src_linesize = frame->linesize[0];
			const size_t min_line = linesize < src_linesize
							? linesize
							: src_linesize;
			uint8_t *dst = data;
			const uint8_t *src = frame->data[0];
			const size_t row_elements = min_line >> 2;
			if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY_SRGB) {
				for (int y = 0; y < info->cy; y++) {
					gs_premultiply_xyza_srgb_loop_restrict(
						dst, src, row_elements);
					dst += linesize;
					src += src_linesize;
				}
			} else if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY) {
				for (int y = 0; y < info->cy; y++) {
					gs_premultiply_xyza_loop_restrict(
						dst, src, row_elements);
					dst += linesize;
					src += src_linesize;
				}
			}
		}
	} else if (info->format == AV_PIX_FMT_RGBA64BE) {
		const size_t dst_linesize = (size_t)info->cx * 4;
		data = bmalloc(info->cy * dst_linesize);
		const size_t src_linesize = frame->linesize[0];
		const size_t src_min_line = (dst_linesize * 2) < src_linesize
						    ? (dst_linesize * 2)
						    : src_linesize;
		const size_t row_elements = src_min_line >> 3;
		uint8_t *dst = data;
		const uint8_t *src = frame->data[0];
		uint16_t value[4];
		float f[4];
		if (alpha_mode == GS_IMAGE_ALPHA_STRAIGHT) {
			for (int y = 0; y < info->cy; y++) {
				for (size_t x = 0; x < row_elements; ++x) {
					memcpy(value, src, sizeof(value));
					f[0] = (float)obs_bswap16(value[0]) /
					       65535.0f;
					f[1] = (float)obs_bswap16(value[1]) /
					       65535.0f;
					f[2] = (float)obs_bswap16(value[2]) /
					       65535.0f;
					f[3] = (float)obs_bswap16(value[3]) /
					       65535.0f;
					gs_float4_to_u8x4(dst, f);
					dst += sizeof(*dst) * 4;
					src += sizeof(value);
				}

				src += src_linesize - src_min_line;
			}
		} else if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY_SRGB) {
			for (int y = 0; y < info->cy; y++) {
				for (size_t x = 0; x < row_elements; ++x) {
					memcpy(value, src, sizeof(value));
					f[0] = (float)obs_bswap16(value[0]) /
					       65535.0f;
					f[1] = (float)obs_bswap16(value[1]) /
					       65535.0f;
					f[2] = (float)obs_bswap16(value[2]) /
					       65535.0f;
					f[3] = (float)obs_bswap16(value[3]) /
					       65535.0f;
					gs_float3_srgb_nonlinear_to_linear(f);
					gs_premultiply_float4(f);
					gs_float3_srgb_linear_to_nonlinear(f);
					gs_float4_to_u8x4(dst, f);
					dst += sizeof(*dst) * 4;
					src += sizeof(value);
				}

				src += src_linesize - src_min_line;
			}
		} else if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY) {
			for (int y = 0; y < info->cy; y++) {
				for (size_t x = 0; x < row_elements; ++x) {
					memcpy(value, src, sizeof(value));
					f[0] = (float)obs_bswap16(value[0]) /
					       65535.0f;
					f[1] = (float)obs_bswap16(value[1]) /
					       65535.0f;
					f[2] = (float)obs_bswap16(value[2]) /
					       65535.0f;
					f[3] = (float)obs_bswap16(value[3]) /
					       65535.0f;
					gs_premultiply_float4(f);
					gs_float4_to_u8x4(dst, f);
					dst += sizeof(*dst) * 4;
					src += sizeof(value);
				}

				src += src_linesize - src_min_line;
			}
		}

		info->format = AV_PIX_FMT_RGBA;
	} else {
		static const enum AVPixelFormat format = AV_PIX_FMT_BGRA;

		sws_ctx = sws_getContext(info->cx, info->cy, info->format,
					 info->cx, info->cy, format, SWS_POINT,
					 NULL, NULL, NULL);
		if (!sws_ctx) {
			blog(LOG_WARNING,
			     "Failed to create scale context "
			     "for '%s'",
			     info->file);
			goto fail;
		}

		uint8_t *pointers[4];
		int linesizes[4];
		ret = av_image_alloc(pointers, linesizes, info->cx, info->cy,
				     format, 32);
		if (ret < 0) {
			blog(LOG_WARNING, "av_image_alloc failed for '%s': %s",
			     info->file, av_err2str(ret));
			sws_freeContext(sws_ctx);
			goto fail;
		}

		ret = sws_scale(sws_ctx, (const uint8_t *const *)frame->data,
				frame->linesize, 0, info->cy, pointers,
				linesizes);
		sws_freeContext(sws_ctx);

		if (ret < 0) {
			blog(LOG_WARNING, "sws_scale failed for '%s': %s",
			     info->file, av_err2str(ret));
			av_freep(pointers);
			goto fail;
		}

		const size_t linesize = (size_t)info->cx * 4;
		data = bmalloc(info->cy * linesize);
		const uint8_t *src = pointers[0];
		uint8_t *dst = data;
		for (size_t y = 0; y < (size_t)info->cy; y++) {
			memcpy(dst, src, linesize);
			dst += linesize;
			src += linesizes[0];
		}

		av_freep(pointers);

		if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY_SRGB) {
			gs_premultiply_xyza_srgb_loop(data, (size_t)info->cx *
								    info->cy);
		} else if (alpha_mode == GS_IMAGE_ALPHA_PREMULTIPLY) {
			gs_premultiply_xyza_loop(data,
						 (size_t)info->cx * info->cy);
		}

		info->format = format;
	}

	data = ffmpeg_image_orient(info, data, orient);

fail:
	return data;
}

static void *ffmpeg_image_decode(struct ffmpeg_image *info,
				 enum gs_image_alpha_mode alpha_mode)
{
	AVPacket packet = {0};
	void *data = NULL;
	AVFrame *frame = av_frame_alloc();
	int got_frame = 0;
	int ret;

	if (!frame) {
		blog(LOG_WARNING, "Failed to create frame data for '%s'",
		     info->file);
		return NULL;
	}

	ret = av_read_frame(info->fmt_ctx, &packet);
	if (ret < 0) {
		blog(LOG_WARNING, "Failed to read image frame from '%s': %s",
		     info->file, av_err2str(ret));
		goto fail;
	}

	while (!got_frame) {
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		ret = avcodec_send_packet(info->decoder_ctx, &packet);
		if (ret == 0)
			ret = avcodec_receive_frame(info->decoder_ctx, frame);

		got_frame = (ret == 0);

		if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
			ret = 0;
#else
		ret = avcodec_decode_video2(info->decoder_ctx, frame,
					    &got_frame, &packet);
#endif
		if (ret < 0) {
			blog(LOG_WARNING, "Failed to decode frame for '%s': %s",
			     info->file, av_err2str(ret));
			goto fail;
		}
	}

	data = ffmpeg_image_reformat_frame(info, frame, alpha_mode);

fail:
	av_packet_unref(&packet);
	av_frame_free(&frame);
	return data;
}

void gs_init_image_deps(void)
{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
#endif
}

void gs_free_image_deps(void) {}

static inline enum gs_color_format convert_format(enum AVPixelFormat format)
{
	switch ((int)format) {
	case AV_PIX_FMT_RGBA:
		return GS_RGBA;
	case AV_PIX_FMT_BGRA:
		return GS_BGRA;
	case AV_PIX_FMT_BGR0:
		return GS_BGRX;
	case AV_PIX_FMT_RGBA64BE:
		return GS_RGBA16;
	}

	return GS_BGRX;
}

uint8_t *gs_create_texture_file_data(const char *file,
				     enum gs_color_format *format,
				     uint32_t *cx_out, uint32_t *cy_out)
{
	struct ffmpeg_image image;
	uint8_t *data = NULL;

	if (ffmpeg_image_init(&image, file)) {
		data = ffmpeg_image_decode(&image, GS_IMAGE_ALPHA_STRAIGHT);
		if (data) {
			*format = convert_format(image.format);
			*cx_out = (uint32_t)image.cx;
			*cy_out = (uint32_t)image.cy;
		}

		ffmpeg_image_free(&image);
	}

	return data;
}

#ifdef _WIN32
static float pq_to_linear(float u)
{
	const float common = powf(u, 1.f / 78.84375f);
	return powf(fabsf(max(common - 0.8359375f, 0.f) /
			  (18.8515625f - 18.6875f * common)),
		    1.f / 0.1593017578f);
}

static void convert_pq_to_cccs(const BYTE *intermediate,
			       const UINT intermediate_size, BYTE *bytes)
{
	const BYTE *src_cursor = intermediate;
	const BYTE *src_cursor_end = src_cursor + intermediate_size;
	BYTE *dst_cursor = bytes;
	uint32_t rgb10;
	struct half rgba16[4];
	rgba16[3].u = 0x3c00;
	while (src_cursor < src_cursor_end) {
		memcpy(&rgb10, src_cursor, sizeof(rgb10));
		const float blue = (float)(rgb10 & 0x3ff) / 1023.f;
		const float green = (float)((rgb10 >> 10) & 0x3ff) / 1023.f;
		const float red = (float)((rgb10 >> 20) & 0x3ff) / 1023.f;
		const float red2020 = pq_to_linear(red);
		const float green2020 = pq_to_linear(green);
		const float blue2020 = pq_to_linear(blue);
		const float red709 = 1.6604910021084345f * red2020 -
				     0.58764113878854951f * green2020 -
				     0.072849863319884883f * blue2020;
		const float green709 = -0.12455047452159074f * red2020 +
				       1.1328998971259603f * green2020 -
				       0.0083494226043694768f * blue2020;
		const float blue709 = -0.018150763354905303f * red2020 -
				      0.10057889800800739f * green2020 +
				      1.1187296613629127f * blue2020;
		rgba16[0] = half_from_float(red709 * 125.f);
		rgba16[1] = half_from_float(green709 * 125.f);
		rgba16[2] = half_from_float(blue709 * 125.f);
		memcpy(dst_cursor, &rgba16, sizeof(rgba16));
		src_cursor += 4;
		dst_cursor += 8;
	}
}

static void *wic_image_init_internal(const char *file,
				     IWICBitmapFrameDecode *pFrame,
				     enum gs_color_format *format,
				     uint32_t *cx_out, uint32_t *cy_out,
				     enum gs_color_space *space)
{
	BYTE *bytes = NULL;

	WICPixelFormatGUID pixelFormat;
	HRESULT hr = pFrame->lpVtbl->GetPixelFormat(pFrame, &pixelFormat);
	if (SUCCEEDED(hr)) {
		const bool scrgb = memcmp(&pixelFormat,
					  &GUID_WICPixelFormat64bppRGBAHalf,
					  sizeof(pixelFormat)) == 0;
		const bool pq10 = memcmp(&pixelFormat,
					 &GUID_WICPixelFormat32bppBGR101010,
					 sizeof(pixelFormat)) == 0;
		if (scrgb || pq10) {
			UINT width, height;
			hr = pFrame->lpVtbl->GetSize(pFrame, &width, &height);
			if (SUCCEEDED(hr)) {
				const UINT pitch = 8 * width;
				const UINT size = pitch * height;
				bytes = bmalloc(size);
				if (bytes) {
					bool success = false;
					if (pq10) {
						const UINT intermediate_pitch =
							4 * width;
						const UINT intermediate_size =
							intermediate_pitch *
							height;
						BYTE *intermediate = bmalloc(
							intermediate_size);
						if (intermediate) {
							hr = pFrame->lpVtbl->CopyPixels(
								pFrame, NULL,
								intermediate_pitch,
								intermediate_size,
								intermediate);
							success = SUCCEEDED(hr);
							if (success) {
								convert_pq_to_cccs(
									intermediate,
									intermediate_size,
									bytes);
							} else {
								blog(LOG_WARNING,
								     "WIC: Failed to CopyPixels intermediate for file: %s",
								     file);
							}

							bfree(intermediate);
						} else {
							blog(LOG_WARNING,
							     "WIC: Failed to allocate intermediate for file: %s",
							     file);
						}
					} else {
						hr = pFrame->lpVtbl->CopyPixels(
							pFrame, NULL, pitch,
							size, bytes);
						success = SUCCEEDED(hr);
						if (!success) {
							blog(LOG_WARNING,
							     "WIC: Failed to CopyPixels for file: %s",
							     file);
						}
					}

					if (success) {
						*format = GS_RGBA16F;
						*cx_out = width;
						*cy_out = height;
						*space = GS_CS_709_SCRGB;
					} else {
						bfree(bytes);
						bytes = NULL;
					}
				} else {
					blog(LOG_WARNING,
					     "WIC: Failed to allocate for file: %s",
					     file);
				}
			} else {
				blog(LOG_WARNING,
				     "WIC: Failed to GetSize of frame for file: %s",
				     file);
			}
		} else {
			blog(LOG_WARNING,
			     "WIC: Only handle GUID_WICPixelFormat32bppBGR101010 and GUID_WICPixelFormat64bppRGBAHalf for now");
		}
	} else {
		blog(LOG_WARNING, "WIC: Failed to GetPixelFormat for file: %s",
		     file);
	}

	return bytes;
}

static void *wic_image_init(const struct ffmpeg_image *info, const char *file,
			    enum gs_color_format *format, uint32_t *cx_out,
			    uint32_t *cy_out, enum gs_color_space *space)
{
	const size_t len = strlen(file);
	if (len <= 4 && astrcmpi(file + len - 4, ".jxr") != 0) {
		blog(LOG_WARNING,
		     "WIC: Only handle JXR for WIC images for now");
		return NULL;
	}

	BYTE *bytes = NULL;

	wchar_t *file_w = NULL;
	os_utf8_to_wcs_ptr(file, 0, &file_w);
	if (file_w) {
		IWICImagingFactory *pFactory = NULL;
		HRESULT hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL,
					      CLSCTX_INPROC_SERVER,
					      &IID_IWICImagingFactory,
					      &pFactory);
		if (SUCCEEDED(hr)) {
			IWICBitmapDecoder *pDecoder = NULL;
			hr = pFactory->lpVtbl->CreateDecoderFromFilename(
				pFactory, file_w, NULL, GENERIC_READ,
				WICDecodeMetadataCacheOnDemand, &pDecoder);
			if (SUCCEEDED(hr)) {
				IWICBitmapFrameDecode *pFrame = NULL;
				hr = pDecoder->lpVtbl->GetFrame(pDecoder, 0,
								&pFrame);
				if (SUCCEEDED(hr)) {
					bytes = wic_image_init_internal(
						file, pFrame, format, cx_out,
						cy_out, space);

					pFrame->lpVtbl->Release(pFrame);
				} else {
					blog(LOG_WARNING,
					     "WIC: Failed to create IWICBitmapFrameDecode from file: %s",
					     file);
				}

				pDecoder->lpVtbl->Release(pDecoder);
			} else {
				blog(LOG_WARNING,
				     "WIC: Failed to create IWICBitmapDecoder from file: %s",
				     file);
			}

			pFactory->lpVtbl->Release(pFactory);
		} else {
			blog(LOG_WARNING,
			     "WIC: Failed to create IWICImagingFactory");
		}

		bfree(file_w);
	} else {
		blog(LOG_WARNING, "WIC: Failed to widen file name: %s", file);
	}

	return bytes;
}
#endif

uint8_t *gs_create_texture_file_data2(const char *file,
				      enum gs_image_alpha_mode alpha_mode,
				      enum gs_color_format *format,
				      uint32_t *cx_out, uint32_t *cy_out)
{
	enum gs_color_space unused;
	return gs_create_texture_file_data3(file, alpha_mode, format, cx_out,
					    cy_out, &unused);
}

uint8_t *gs_create_texture_file_data3(const char *file,
				      enum gs_image_alpha_mode alpha_mode,
				      enum gs_color_format *format,
				      uint32_t *cx_out, uint32_t *cy_out,
				      enum gs_color_space *space)
{
	struct ffmpeg_image image;
	uint8_t *data = NULL;

	if (ffmpeg_image_init(&image, file)) {
		data = ffmpeg_image_decode(&image, alpha_mode);
		if (data) {
			*format = convert_format(image.format);
			*cx_out = (uint32_t)image.cx;
			*cy_out = (uint32_t)image.cy;
			*space = GS_CS_SRGB;
		}

		ffmpeg_image_free(&image);
	}

#ifdef _WIN32
	if (data == NULL) {
		data = wic_image_init(&image, file, format, cx_out, cy_out,
				      space);
	}
#endif

	return data;
}
