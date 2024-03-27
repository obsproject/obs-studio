/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "../util/bmem.h"
#include "video-io.h"

#ifdef __cplusplus
extern "C" {
#endif

struct video_frame {
	uint8_t *data[MAX_AV_PLANES];
	uint32_t linesize[MAX_AV_PLANES];
};

EXPORT void video_frame_init2(struct video_frame *frame,
			     enum video_format format, uint32_t width,
			     uint32_t height, int32_t align);

EXPORT void video_frame_init(struct video_frame *frame,
			     enum video_format format, uint32_t width,
			     uint32_t height);

static inline void video_frame_free(struct video_frame *frame)
{
	if (frame) {
		bfree(frame->data[0]);
		memset(frame, 0, sizeof(struct video_frame));
	}
}

static inline struct video_frame *
video_frame_create2(enum video_format format, uint32_t width, uint32_t height, int32_t align)
{
	struct video_frame *frame;

	frame = (struct video_frame *)bzalloc(sizeof(struct video_frame));
	video_frame_init2(frame, format, width, height, align);
	return frame;
}

static inline struct video_frame *
video_frame_create(enum video_format format, uint32_t width, uint32_t height)
{
	return video_frame_create2(format, width, height, 0);
}

static inline void video_frame_destroy(struct video_frame *frame)
{
	if (frame) {
		bfree(frame->data[0]);
		bfree(frame);
	}
}

EXPORT void video_frame_copy(struct video_frame *dst,
			     const struct video_frame *src,
			     enum video_format format, uint32_t height);

#ifdef __cplusplus
}
#endif
