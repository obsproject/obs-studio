/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

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
*/

#pragma once

#include <linux/videodev2.h>
#include <libv4l2.h>

#include <obs-module.h>
#include <media-io/video-io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Data structure for mapped buffers
 */
struct v4l2_mmap_info {
	/** length of the mapped buffer */
	size_t length;
	/** start address of the mapped buffer */
	void *start;
};

/**
 * Data structure for buffer info
 */
struct v4l2_buffer_data {
	/** number of mapped buffers */
	uint_fast32_t count;
	/** memory info for mapped buffers */
	struct v4l2_mmap_info *info;
};

/**
 * Convert v4l2 pixel format to obs video format
 *
 * @param format v4l2 format id
 *
 * @return obs video_format id
 */
static inline enum video_format v4l2_to_obs_video_format(uint_fast32_t format)
{
	switch (format) {
	case V4L2_PIX_FMT_YVYU:   return VIDEO_FORMAT_YVYU;
	case V4L2_PIX_FMT_YUYV:   return VIDEO_FORMAT_YUY2;
	case V4L2_PIX_FMT_UYVY:   return VIDEO_FORMAT_UYVY;
	case V4L2_PIX_FMT_NV12:   return VIDEO_FORMAT_NV12;
	case V4L2_PIX_FMT_YUV420: return VIDEO_FORMAT_I420;
	case V4L2_PIX_FMT_YVU420: return VIDEO_FORMAT_I420;
	default:                  return VIDEO_FORMAT_NONE;
	}
}

/**
 * Fixed framesizes for devices that don't support enumerating discrete values.
 *
 * The framesizes in this array are packed, the width encoded in the high word
 * and the height in the low word.
 * The array is terminated with a zero.
 */
static const int v4l2_framesizes[] =
{
	/* 4:3 */
	160<<16		| 120,
	320<<16		| 240,
	480<<16		| 320,
	640<<16		| 480,
	800<<16		| 600,
	1024<<16	| 768,
	1280<<16	| 960,
	1440<<16	| 1050,
	1440<<16	| 1080,
	1600<<16	| 1200,

	/* 16:9 */
	640<<16		| 360,
	960<<16		| 540,
	1280<<16	| 720,
	1600<<16	| 900,
	1920<<16	| 1080,
	1920<<16	| 1200,

	/* tv */
	432<<16		| 520,
	480<<16		| 320,
	480<<16		| 530,
	486<<16		| 440,
	576<<16		| 310,
	576<<16		| 520,
	576<<16		| 570,
	720<<16		| 576,
	1024<<16	| 576,

	0
};

/**
 * Fixed framerates for devices that don't support enumerating discrete values.
 *
 * The framerates in this array are packed, the numerator encoded in the high
 * word and the denominator in the low word.
 * The array is terminated with a zero.
 */
static const int v4l2_framerates[] =
{
	1<<16		| 60,
	1<<16		| 50,
	1<<16		| 30,
	1<<16		| 25,
	1<<16		| 20,
	1<<16		| 15,
	1<<16		| 10,
	1<<16		| 5,

	0
};

/**
 * Start the video capture on the device.
 *
 * This enqueues the memory mapped buffers and instructs the device to start
 * the video stream.
 *
 * @param dev handle for the v4l2 device
 * @param buf buffer data
 *
 * @return negative on failure
 */
int_fast32_t v4l2_start_capture(int_fast32_t dev, struct v4l2_buffer_data *buf);

/**
 * Stop the video capture on the device.
 *
 * @param dev handle for the v4l2 device
 *
 * @return negative on failure
 */
int_fast32_t v4l2_stop_capture(int_fast32_t dev);

/**
 * Create memory mapping for buffers
 *
 * This tries to map at least 2, preferably 4, buffers to application memory.
 *
 * @param dev handle for the v4l2 device
 * @param buf buffer data
 *
 * @return negative on failure
 */
int_fast32_t v4l2_create_mmap(int_fast32_t dev, struct v4l2_buffer_data *buf);

/**
 * Destroy the memory mapping for buffers
 *
 * @param dev handle for the v4l2 device
 * @param buf buffer data
 *
 * @return negative on failure
 */
int_fast32_t v4l2_destroy_mmap(struct v4l2_buffer_data *buf);

#ifdef __cplusplus
}
#endif
