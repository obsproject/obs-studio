/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#pragma once

#include "media-io-defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Base video output component.  Use this to create a video output track. */

struct video_output;
typedef struct video_output *video_t;

enum video_format {
	VIDEO_FORMAT_NONE,

	/* planar 420 format */
	VIDEO_FORMAT_I420, /* three-plane */
	VIDEO_FORMAT_NV12, /* two-plane, luma and packed chroma */

	/* packed 422 formats */
	VIDEO_FORMAT_YVYU,
	VIDEO_FORMAT_YUY2, /* YUYV */
	VIDEO_FORMAT_UYVY,

	/* packed uncompressed formats */
	VIDEO_FORMAT_RGBA,
	VIDEO_FORMAT_BGRA,
	VIDEO_FORMAT_BGRX,
};

struct video_data {
	const uint8_t     *data[MAX_AV_PLANES];
	uint32_t          linesize[MAX_AV_PLANES];
	uint64_t          timestamp;
};

struct video_output_info {
	const char        *name;

	enum video_format format;
	uint32_t          fps_num;
	uint32_t          fps_den;
	uint32_t          width;
	uint32_t          height;
};

static inline bool format_is_yuv(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_I420:
	case VIDEO_FORMAT_NV12:
	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
		return true;
	case VIDEO_FORMAT_NONE:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		return false;
	}

	return false;
}

enum video_scale_type {
	VIDEO_SCALE_POINT         = 0,
	VIDEO_SCALE_FAST_BILINEAR = 1,
	VIDEO_SCALE_DEFAULT       = VIDEO_SCALE_FAST_BILINEAR,
	VIDEO_SCALE_BILINEAR      = 2,
	VIDEO_SCALE_BICUBIC       = 3,
};

enum video_colorspace {
	VIDEO_CS_601              = 0,
	VIDEO_CS_DEFAULT          = VIDEO_CS_601,
	VIDEO_CS_709              = 1,
};

struct video_scale_info {
	enum video_format     format;
	uint32_t              width;
	uint32_t              height;
	bool                  full_range;
	enum video_colorspace colorspace;
};

#define VIDEO_OUTPUT_SUCCESS       0
#define VIDEO_OUTPUT_INVALIDPARAM -1
#define VIDEO_OUTPUT_FAIL         -2

EXPORT int video_output_open(video_t *video, struct video_output_info *info);
EXPORT void video_output_close(video_t video);

EXPORT bool video_output_connect(video_t video,
		struct video_scale_info *conversion,
		void (*callback)(void *param, const struct video_data *frame),
		void *param);
EXPORT void video_output_disconnect(video_t video,
		void (*callback)(void *param, const struct video_data *frame),
		void *param);

EXPORT bool video_output_active(video_t video);

EXPORT const struct video_output_info *video_output_getinfo(video_t video);
EXPORT void video_output_swap_frame(video_t video, struct video_data *frame);
EXPORT bool video_output_wait(video_t video);
EXPORT uint64_t video_getframetime(video_t video);
EXPORT uint64_t video_gettime(video_t video);
EXPORT void video_output_stop(video_t video);

#ifdef __cplusplus
}
#endif
