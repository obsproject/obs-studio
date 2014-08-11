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

#include <sys/mman.h>

#include <util/bmem.h>

#include "v4l2-helpers.h"

#define blog(level, msg, ...) blog(level, "v4l2-helpers: " msg, ##__VA_ARGS__)

int_fast32_t v4l2_start_capture(int_fast32_t dev, struct v4l2_buffer_data *buf)
{
	enum v4l2_buf_type type;
	struct v4l2_buffer enq;

	memset(&enq, 0, sizeof(enq));
	enq.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	enq.memory = V4L2_MEMORY_MMAP;

	for (enq.index = 0; enq.index < buf->count; ++enq.index) {
		if (v4l2_ioctl(dev, VIDIOC_QBUF, &enq) < 0) {
			blog(LOG_ERROR, "unable to queue buffer");
			return -1;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (v4l2_ioctl(dev, VIDIOC_STREAMON, &type) < 0) {
		blog(LOG_ERROR, "unable to start stream");
		return -1;
	}

	return 0;
}

int_fast32_t v4l2_stop_capture(int_fast32_t dev)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (v4l2_ioctl(dev, VIDIOC_STREAMOFF, &type) < 0) {
		blog(LOG_ERROR, "unable to stop stream");
		return -1;
	}

	return 0;
}

int_fast32_t v4l2_create_mmap(int_fast32_t dev, struct v4l2_buffer_data *buf)
{
	struct v4l2_requestbuffers req;
	struct v4l2_buffer map;

	memset(&req, 0, sizeof(req));
	req.count  = 4;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (v4l2_ioctl(dev, VIDIOC_REQBUFS, &req) < 0) {
		blog(LOG_ERROR, "Request for buffers failed !");
		return -1;
	}

	if (req.count < 2) {
		blog(LOG_ERROR, "Device returned less than 2 buffers");
		return -1;
	}

	buf->count = req.count;
	buf->info  = bzalloc(req.count * sizeof(struct v4l2_mmap_info));

	memset(&map, 0, sizeof(map));
	map.type   = req.type;
	map.memory = req.memory;

	for (map.index = 0; map.index < req.count; ++map.index) {
		if (v4l2_ioctl(dev, VIDIOC_QUERYBUF, &map) < 0) {
			blog(LOG_ERROR, "Failed to query buffer details");
			return -1;
		}

		buf->info[map.index].length = map.length;
		buf->info[map.index].start  = v4l2_mmap(NULL, map.length,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			dev, map.m.offset);

		if (buf->info[map.index].start == MAP_FAILED) {
			blog(LOG_ERROR, "mmap for buffer failed");
			return -1;
		}
	}

	return 0;
}

int_fast32_t v4l2_destroy_mmap(struct v4l2_buffer_data *buf)
{
	for(uint_fast32_t i = 0; i < buf->count; ++i) {
		if (buf->info[i].start != MAP_FAILED && buf->info[i].start != 0)
			v4l2_munmap(buf->info[i].start, buf->info[i].length);
	}

	if (buf->count) {
		bfree(buf->info);
		buf->count = 0;
	}

	return 0;
}

