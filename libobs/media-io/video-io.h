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
	uint8_t           *data[MAX_AV_PLANES];
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
	VIDEO_SCALE_DEFAULT,
	VIDEO_SCALE_POINT,
	VIDEO_SCALE_FAST_BILINEAR,
	VIDEO_SCALE_BILINEAR,
	VIDEO_SCALE_BICUBIC,
};

enum video_colorspace {
	VIDEO_CS_DEFAULT,
	VIDEO_CS_601,
	VIDEO_CS_709,
};

enum video_range_type {
	VIDEO_RANGE_DEFAULT,
	VIDEO_RANGE_PARTIAL,
	VIDEO_RANGE_FULL
};

struct video_scale_info {
	enum video_format     format;
	uint32_t              width;
	uint32_t              height;
	enum video_range_type range;
	enum video_colorspace colorspace;
};

EXPORT enum video_format video_format_from_fourcc(uint32_t fourcc);

EXPORT bool video_format_get_parameters(enum video_colorspace color_space,
		enum video_range_type range, float matrix[16],
		float min_range[3], float max_range[3]);

#define VIDEO_OUTPUT_SUCCESS       0
#define VIDEO_OUTPUT_INVALIDPARAM -1
#define VIDEO_OUTPUT_FAIL         -2

EXPORT int video_output_open(video_t *video, struct video_output_info *info);
EXPORT void video_output_close(video_t video);

EXPORT bool video_output_connect(video_t video,
		const struct video_scale_info *conversion,
		void (*callback)(void *param, struct video_data *frame),
		void *param);
EXPORT void video_output_disconnect(video_t video,
		void (*callback)(void *param, struct video_data *frame),
		void *param);

EXPORT bool video_output_active(video_t video);

EXPORT const struct video_output_info *video_output_get_info(video_t video);
EXPORT void video_output_swap_frame(video_t video, struct video_data *frame);
EXPORT bool video_output_wait(video_t video);
EXPORT uint64_t video_output_get_frame_time(video_t video);
EXPORT uint64_t video_output_get_time(video_t video);
EXPORT void video_output_stop(video_t video);

EXPORT enum video_format video_output_get_format(video_t video);
EXPORT uint32_t video_output_get_width(video_t video);
EXPORT uint32_t video_output_get_height(video_t video);
EXPORT double video_output_get_frame_rate(video_t video);

EXPORT uint32_t video_output_get_skipped_frames(video_t video);
EXPORT uint32_t video_output_get_total_frames(video_t video);


#ifdef __cplusplus
}
#endif
