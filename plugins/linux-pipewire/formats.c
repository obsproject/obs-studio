/*
 * formats.c
 *
 * Copyright 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "formats.h"

#include <libdrm/drm_fourcc.h>
#include <pipewire/pipewire.h>

static const struct obs_pw_video_format supported_formats[] = {
	{
		SPA_VIDEO_FORMAT_BGRA,
		DRM_FORMAT_ARGB8888,
		GS_BGRA,
		VIDEO_FORMAT_BGRA,
		false,
		4,
		"ARGB8888",
	},
	{
		SPA_VIDEO_FORMAT_RGBA,
		DRM_FORMAT_ABGR8888,
		GS_RGBA,
		VIDEO_FORMAT_RGBA,
		false,
		4,
		"ABGR8888",
	},
	{
		SPA_VIDEO_FORMAT_BGRx,
		DRM_FORMAT_XRGB8888,
		GS_BGRX,
		VIDEO_FORMAT_BGRX,
		false,
		4,
		"XRGB8888",
	},
	{
		SPA_VIDEO_FORMAT_RGBx,
		DRM_FORMAT_XBGR8888,
		GS_BGRX,
		VIDEO_FORMAT_NONE,
		true,
		4,
		"XBGR8888",
	},
	{
		SPA_VIDEO_FORMAT_YUY2,
		DRM_FORMAT_YUYV,
		GS_UNKNOWN,
		VIDEO_FORMAT_YUY2,
		false,
		2,
		"YUYV422",
	},
	{
		SPA_VIDEO_FORMAT_NV12,
		DRM_FORMAT_NV12,
		GS_UNKNOWN,
		VIDEO_FORMAT_NV12,
		false,
		2,
		"NV12",
	},
#if PW_CHECK_VERSION(0, 3, 41)
	{
		SPA_VIDEO_FORMAT_ABGR_210LE,
		DRM_FORMAT_ABGR2101010,
		GS_R10G10B10A2,
		VIDEO_FORMAT_NONE,
		false,
		10,
		"ABGR2101010",

	},
	{
		SPA_VIDEO_FORMAT_xBGR_210LE,
		DRM_FORMAT_XBGR2101010,
		GS_R10G10B10A2,
		VIDEO_FORMAT_NONE,
		false,
		10,
		"XBGR2101010",

	},
#endif
	{
		SPA_VIDEO_FORMAT_ENCODED,
		DRM_FORMAT_INVALID,
		GS_UNKNOWN,
		VIDEO_FORMAT_NONE,
		false,
		0,
		"Encoded",
	},

};

#define N_SUPPORTED_FORMATS (sizeof(supported_formats) / sizeof(supported_formats[0]))

bool obs_pw_video_format_from_spa_format(uint32_t spa_format, struct obs_pw_video_format *out_video_format)
{
	for (size_t i = 0; i < N_SUPPORTED_FORMATS; i++) {
		if (supported_formats[i].spa_format != spa_format)
			continue;

		if (out_video_format)
			*out_video_format = supported_formats[i];

		return true;
	}
	return false;
}
