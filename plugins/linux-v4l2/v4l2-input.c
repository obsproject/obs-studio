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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <linux/videodev2.h>

#include <util/threading.h>
#include <util/bmem.h>
#include <obs.h>

#define V4L2_DATA(voidptr) struct v4l2_data *data = voidptr;

#define timeval2ns(tv) \
	(((uint64_t) tv.tv_sec * 1000000000) + ((uint64_t) tv.tv_usec * 1000))

static const char video[] = "/dev/video0";

struct v4l2_buffer_data {
	size_t length;
	void *start;
};

struct v4l2_data {
	pthread_t thread;
	os_event_t event;
	obs_source_t source;
	uint_fast32_t linesize;

	uint64_t frames;

	int_fast32_t dev;
	uint_fast32_t buf_count;
	struct v4l2_buffer_data *buf;
};

/*
 * start capture
 */
static int_fast32_t v4l2_start_capture(struct v4l2_data *data)
{
	enum v4l2_buf_type type;

	for (uint_fast32_t i = 0; i < data->buf_count; ++i) {
		struct v4l2_buffer buf;

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (ioctl(data->dev, VIDIOC_QBUF, &buf) < 0) {
			blog(LOG_ERROR, "v4l2-input: unable to queue buffer");
			return -1;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(data->dev, VIDIOC_STREAMON, &type) < 0) {
		blog(LOG_ERROR, "v4l2-input: unable to start stream");
		return -1;
	}

	return 0;
}

/*
 * stop capture
 */
static int_fast32_t v4l2_stop_capture(struct v4l2_data *data)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(data->dev, VIDIOC_STREAMOFF, &type) < 0) {
		blog(LOG_ERROR, "v4l2-input: unable to stop stream");
	}

	return 0;
}

/*
 * create memory mapping for buffers
 */
static int_fast32_t v4l2_create_mmap(struct v4l2_data *data)
{
	struct v4l2_requestbuffers req;

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (ioctl(data->dev, VIDIOC_REQBUFS, &req) < 0) {
		blog(LOG_DEBUG, "v4l2-input: request for buffers failed !");
		return -1;
	}

	if (req.count < 2) {
		blog(LOG_DEBUG, "v4l2-input: not enough memory !");
		return -1;
	}

	data->buf_count = req.count;
	data->buf = bzalloc(req.count * sizeof(struct v4l2_buffer_data));

	for (uint_fast32_t i = 0; i < req.count; ++i) {
		struct v4l2_buffer buf;

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (ioctl(data->dev, VIDIOC_QUERYBUF, &buf) < 0) {
			blog(LOG_ERROR, "v4l2-input: failed to query buffer");
			return -1;
		}

		data->buf[i].length = buf.length;
		data->buf[i].start = mmap(NULL, buf.length,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			data->dev, buf.m.offset);

		if (data->buf[i].start == MAP_FAILED) {
			blog(LOG_ERROR, "v4l2-input: mmap for buffer failed");
			return -1;
		}
	}

	return 0;
}

/*
 * destroy memory mapping for buffers
 */
static void v4l2_destroy_mmap(struct v4l2_data *data)
{
	for(uint_fast32_t i = 0; i < data->buf_count; ++i) {
		if (data->buf[i].start != MAP_FAILED)
			munmap(data->buf[i].start, data->buf[i].length);
	}

	data->buf_count = 0;
	bfree(data->buf);
}

/*
 * Worker thread to get video data
 */
static void *v4l2_thread(void *vptr)
{
	V4L2_DATA(vptr);

	if (v4l2_start_capture(data) < 0)
		goto exit;

	while (os_event_try(data->event) == EAGAIN) {
		int r;
		fd_set fds;
		struct timeval tv;
		struct v4l2_buffer buf;
		struct source_frame out;

		FD_ZERO(&fds);
		FD_SET(data->dev, &fds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		r = select(data->dev + 1, &fds, NULL, NULL, &tv);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			blog(LOG_DEBUG, "v4l2-input: select failed");
			break;
		} else if (r == 0) {
			blog(LOG_DEBUG, "v4l2-input: select timeout");
			continue;
		}

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (ioctl(data->dev, VIDIOC_DQBUF, &buf) < 0) {
			if (errno == EAGAIN)
				continue;
			blog(LOG_DEBUG, "v4l2-input: failed to dequeue buffer");
			break;
		}

		video_format_get_parameters(VIDEO_CS_709, VIDEO_RANGE_PARTIAL,
				out.color_matrix, out.color_range_min,
				out.color_range_max);
		out.data[0] = (uint8_t *) data->buf[buf.index].start;
		out.linesize[0] = data->linesize;
		out.width = 640;
		out.height = 480;
		out.timestamp = timeval2ns(buf.timestamp);
		out.format = VIDEO_FORMAT_YUY2;

		obs_source_output_video(data->source, &out);

		if (ioctl(data->dev, VIDIOC_QBUF, &buf) < 0) {
			blog(LOG_DEBUG, "v4l2-input: failed to enqueue buffer");
			break;
		}

		data->frames++;
	}

exit:
	v4l2_stop_capture(data);
	return NULL;
}

static const char* v4l2_getname(const char* locale)
{
	UNUSED_PARAMETER(locale);
	return "V4L2 Capture Input";
}

static void v4l2_destroy(void *vptr)
{
	V4L2_DATA(vptr);

	if (!data)
		return;

	if (data->thread) {
		os_event_signal(data->event);
		pthread_join(data->thread, NULL);
	}

	if (data->buf_count)
		v4l2_destroy_mmap(data);

	if (data->dev != -1)
		close(data->dev);

	bfree(data);
}

static void *v4l2_create(obs_data_t settings, obs_source_t source)
{
	UNUSED_PARAMETER(settings);

	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_streamparm par;

	struct v4l2_data *data = bzalloc(sizeof(struct v4l2_data));
	data->source = source;

	blog(LOG_DEBUG, "v4l2-input: New input created");

	data->dev = open(video, O_RDWR | O_NONBLOCK);
	if (data->dev == -1) {
		blog(LOG_ERROR, "v4l2-input: Unable to open device: %s", video);
		goto fail;
	}

	if (ioctl(data->dev, VIDIOC_QUERYCAP, &cap) < 0) {
		blog(LOG_ERROR, "v4l2-input: Unable to get capabilities !");
		goto fail;
	}

	blog(LOG_DEBUG, "v4l2-input: Got capabilities for '%s'", cap.card);

	/* TODO: check if device supports needed capabilities */

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(data->dev, VIDIOC_S_FMT, &fmt) < 0) {
		blog(LOG_DEBUG, "v4l2-input: unable to set format");
		goto fail;
	}

	par.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	par.parm.capture.timeperframe.numerator = 1;
	par.parm.capture.timeperframe.denominator = 30;

	if (ioctl(data->dev, VIDIOC_S_PARM, &par) < 0) {
		blog(LOG_DEBUG, "v4l2-input: unable to set params");
		goto fail;
	}

	data->linesize = fmt.fmt.pix.bytesperline;
	blog(LOG_DEBUG, "v4l2-input: Linesize: %"PRIuFAST32, data->linesize);

	if (v4l2_create_mmap(data) < 0) {
		blog(LOG_ERROR, "v4l2-input: failed to map buffers");
		goto fail;
	}

	if (os_event_init(&data->event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (pthread_create(&data->thread, NULL, v4l2_thread, data) != 0)
		goto fail;

	return data;
fail:
	v4l2_destroy(data);
	return NULL;
}

static uint32_t v4l2_getwidth(void *vptr)
{
	V4L2_DATA(vptr);

	return 640;
}

static uint32_t v4l2_getheight(void *vptr)
{
	V4L2_DATA(vptr);

	return 480;
}

struct obs_source_info v4l2_input = {
	.id           = "v4l2_input",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.getname      = v4l2_getname,
	.create       = v4l2_create,
	.destroy      = v4l2_destroy,
	.getwidth     = v4l2_getwidth,
	.getheight    = v4l2_getheight
};
