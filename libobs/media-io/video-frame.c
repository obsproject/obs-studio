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
#include <assert.h>
#include "video-frame.h"

#define HALF(size) ((size + 1) / 2)
#define ALIGN(size, alignment) \
	*size = (*size + alignment - 1) & (~(alignment - 1));

static inline void align_size(size_t *size, size_t alignment)
{
	ALIGN(size, alignment);
}

static inline void align_uint32(uint32_t *size, size_t alignment)
{
	ALIGN(size, (uint32_t)alignment);
}

/* assumes already-zeroed array */
void video_frame_get_linesizes(uint32_t linesize[MAX_AV_PLANES],
			       enum video_format format, uint32_t width)
{
	switch (format) {
	default:
	case VIDEO_FORMAT_NONE:
		break;
	case VIDEO_FORMAT_BGR3: /* one plane: triple width */
		linesize[0] = width * 3;
		break;
	case VIDEO_FORMAT_RGBA: /* one plane: quadruple width */
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
	case VIDEO_FORMAT_AYUV:
	case VIDEO_FORMAT_R10L:
		linesize[0] = width * 4;
		break;
	case VIDEO_FORMAT_P416: /* two planes: double width, quadruple width */
		linesize[0] = width * 2;
		linesize[1] = width * 4;
		break;
	case VIDEO_FORMAT_I420: /* three planes: full width, half width, half width */
	case VIDEO_FORMAT_I422:
		linesize[0] = width;
		linesize[1] = HALF(width);
		linesize[2] = HALF(width);
		break;
	case VIDEO_FORMAT_I210: /* three planes: double width, full width, full width */
	case VIDEO_FORMAT_I010:
		linesize[0] = width * 2;
		linesize[1] = width;
		linesize[2] = width;
		break;
	case VIDEO_FORMAT_I40A: /* four planes: full width, half width, half width, full width */
	case VIDEO_FORMAT_I42A:
		linesize[0] = width;
		linesize[1] = HALF(width);
		linesize[2] = HALF(width);
		linesize[3] = width;
		break;

	case VIDEO_FORMAT_YVYU: /* one plane: double width */
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
		linesize[0] = width * 2;
		break;
	case VIDEO_FORMAT_P010: /* two planes: all double width */
	case VIDEO_FORMAT_P216:
		linesize[0] = width * 2;
		linesize[1] = width * 2;
		break;
	case VIDEO_FORMAT_I412: /* three planes: all double width */
		linesize[0] = width * 2;
		linesize[1] = width * 2;
		linesize[2] = width * 2;
		break;
	case VIDEO_FORMAT_YA2L: /* four planes: all double width */
		linesize[0] = width * 2;
		linesize[1] = width * 2;
		linesize[2] = width * 2;
		linesize[3] = width * 2;
		break;

	case VIDEO_FORMAT_Y800: /* one plane: full width */
		linesize[0] = width;
		break;
	case VIDEO_FORMAT_NV12: /* two planes: all full width */
		linesize[0] = width;
		linesize[1] = width;
		break;
	case VIDEO_FORMAT_I444: /* three planes: all full width */
		linesize[0] = width;
		linesize[1] = width;
		linesize[2] = width;
		break;
	case VIDEO_FORMAT_YUVA: /* four planes: all full width */
		linesize[0] = width;
		linesize[1] = width;
		linesize[2] = width;
		linesize[3] = width;
		break;

	case VIDEO_FORMAT_V210: { /* one plane: bruh (Little Endian Compressed) */
		align_uint32(&width, 48);
		linesize[0] = ((width + 5) / 6) * 16;
		break;
	}
	}
}

void video_frame_get_plane_heights(uint32_t heights[MAX_AV_PLANES],
				   enum video_format format, uint32_t height)
{
	switch (format) {
	default:
	case VIDEO_FORMAT_NONE:
		return;

	case VIDEO_FORMAT_I420: /* three planes: full height, half height, half height */
	case VIDEO_FORMAT_I010:
		heights[0] = height;
		heights[1] = height / 2;
		heights[2] = height / 2;
		break;

	case VIDEO_FORMAT_NV12: /* two planes: full height, half height */
	case VIDEO_FORMAT_P010:
		heights[0] = height;
		heights[1] = height / 2;
		break;

	case VIDEO_FORMAT_Y800: /* one plane: full height */
	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
	case VIDEO_FORMAT_BGR3:
	case VIDEO_FORMAT_AYUV:
	case VIDEO_FORMAT_V210:
	case VIDEO_FORMAT_R10L:
		heights[0] = height;
		break;

	case VIDEO_FORMAT_I444: /* three planes: all full height */
	case VIDEO_FORMAT_I422:
	case VIDEO_FORMAT_I210:
	case VIDEO_FORMAT_I412:
		heights[0] = height;
		heights[1] = height;
		heights[2] = height;
		break;

	case VIDEO_FORMAT_I40A: /* four planes: full height, half height, half height, full height */
		heights[0] = height;
		heights[1] = height / 2;
		heights[2] = height / 2;
		heights[0] = height;
		break;

	case VIDEO_FORMAT_I42A: /* four planes: all full height */
	case VIDEO_FORMAT_YUVA:
	case VIDEO_FORMAT_YA2L:
		heights[0] = height;
		heights[1] = height;
		heights[2] = height;
		heights[3] = height;
		break;

	case VIDEO_FORMAT_P216: /* two planes: all full height */
	case VIDEO_FORMAT_P416:
		heights[0] = height;
		heights[1] = height;
		break;
	}
}

void video_frame_init(struct video_frame *frame, enum video_format format,
		      uint32_t width, uint32_t height)
{
	size_t size = 0;
	uint32_t linesizes[MAX_AV_PLANES];
	uint32_t heights[MAX_AV_PLANES];
	size_t offsets[MAX_AV_PLANES - 1];
	int alignment = base_get_alignment();

	if (!frame)
		return;

	memset(frame, 0, sizeof(struct video_frame));
	memset(linesizes, 0, sizeof(linesizes));
	memset(heights, 0, sizeof(heights));
	memset(offsets, 0, sizeof(offsets));

	/* determine linesizes for each plane */
	video_frame_get_linesizes(linesizes, format, width);

	/* determine line count for each plane */
	video_frame_get_plane_heights(heights, format, height);

	/* calculate total buffer required */
	for (uint32_t i = 0; i < MAX_AV_PLANES; i++) {
		if (!linesizes[i] || !heights[i])
			continue;
		size_t plane_size = (size_t)linesizes[i] * (size_t)heights[i];
		align_size(&plane_size, alignment);
		size += plane_size;
		offsets[i] = size;
	}

	/* allocate memory */
	frame->data[0] = bmalloc(size);
	frame->linesize[0] = linesizes[0];

	/* apply plane data pointers according to offsets */
	for (uint32_t i = 1; i < MAX_AV_PLANES; i++) {
		if (!linesizes[i] || !heights[i])
			continue;
		frame->data[i] = frame->data[0] + offsets[i - 1];
		frame->linesize[i] = linesizes[i];
	}
}

void video_frame_copy(struct video_frame *dst, const struct video_frame *src,
		      enum video_format format, uint32_t cy)
{
	uint32_t heights[MAX_AV_PLANES];

	memset(heights, 0, sizeof(heights));

	/* determine line count for each plane */
	video_frame_get_plane_heights(heights, format, cy);

	/* copy each plane */
	for (uint32_t i = 0; i < MAX_AV_PLANES; i++) {
		if (!heights[i])
			continue;

		if (src->linesize[i] == dst->linesize[i]) {
			memcpy(dst->data[i], src->data[i],
			       src->linesize[i] * heights[i]);
		} else { /* linesizes which do not match must be copied line-by-line */
			size_t src_linesize = src->linesize[i];
			size_t dst_linesize = dst->linesize[i];
			/* determine how much we can write (frames with different line sizes require more )*/
			size_t linesize = src_linesize < dst_linesize
						  ? src_linesize
						  : dst_linesize;
			for (uint32_t y = 0; y < heights[i]; y++) {
				uint8_t *src_pos =
					src->data[i] + (src_linesize * y);
				uint8_t *dst_pos =
					dst->data[i] + (dst_linesize * y);
				memcpy(dst_pos, src_pos, linesize);
			}
		}
	}
}
