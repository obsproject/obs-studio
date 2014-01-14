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

#include "../util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Base video output component.  Use this to create an video output track. */

struct video_output;
typedef struct video_output *video_t;

enum video_format {
	VIDEO_FORMAT_UNKNOWN,

	/* planar 420 format */
	VIDEO_FORMAT_I420, /* three-plane */
	VIDEO_FORMAT_NV12, /* two-plane, luma and packed chroma */

	/* packed 422 formats */
	VIDEO_FORMAT_YVYU,
	VIDEO_FORMAT_YUY2, /* YUYV */
	VIDEO_FORMAT_UYVY,

	/* packed uncompressed formats */
	VIDEO_FORMAT_YUVX,
	VIDEO_FORMAT_UYVX,
	VIDEO_FORMAT_RGBA,
	VIDEO_FORMAT_BGRA,
	VIDEO_FORMAT_BGRX,
};

struct video_frame {
	const void        *data;
	uint32_t          row_size; /* for RGB/BGR formats and UYVX */
	uint64_t          timestamp;
};

struct video_output_info {
	const char        *name;

	enum video_format type;
	uint32_t          fps_num;
	uint32_t          fps_den;
	uint32_t          width;
	uint32_t          height;
};

struct video_info {
	enum video_format type;
	uint32_t          width;
	uint32_t          height;
	uint32_t          row_size; /* if any */
};

#define VIDEO_OUTPUT_SUCCESS       0
#define VIDEO_OUTPUT_INVALIDPARAM -1
#define VIDEO_OUTPUT_FAIL         -2

EXPORT int video_output_open(video_t *video, struct video_output_info *info);
EXPORT void video_output_close(video_t video);

EXPORT void video_output_connect(video_t video, struct video_info *format,
		void (*callback)(void *param, struct video_frame *frame),
		void *param);
EXPORT void video_output_disconnect(video_t video,
		void (*callback)(void *param, struct video_frame *frame),
		void *param);

EXPORT const struct video_output_info *video_output_getinfo(video_t video);
EXPORT void     video_output_frame(video_t video, struct video_frame *frame);
EXPORT bool     video_output_wait(video_t video);
EXPORT uint64_t video_getframetime(video_t video);
EXPORT uint64_t video_gettime(video_t video);
EXPORT void     video_output_stop(video_t video);

#ifdef __cplusplus
}
#endif
