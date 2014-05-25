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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <linux/videodev2.h>

#include <util/threading.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <obs.h>

#define V4L2_DATA(voidptr) struct v4l2_data *data = voidptr;

#define timeval2ns(tv) \
	(((uint64_t) tv.tv_sec * 1000000000) + ((uint64_t) tv.tv_usec * 1000))

struct v4l2_buffer_data {
	size_t length;
	void *start;
};

struct v4l2_data {
	char *device;

	pthread_t thread;
	os_event_t event;
	obs_source_t source;
	uint_fast32_t linesize;

	uint64_t frames;

	int_fast32_t dev;
	int_fast32_t pixelformat;
	int_fast32_t width;
	int_fast32_t height;
	int_fast32_t fps_numerator;
	int_fast32_t fps_denominator;
	uint_fast32_t buf_count;
	struct v4l2_buffer_data *buf;
};

static enum video_format v4l2_to_obs_video_format(uint_fast32_t format)
{
	switch (format) {
	case V4L2_PIX_FMT_YVYU:   return VIDEO_FORMAT_YVYU;
	case V4L2_PIX_FMT_YUYV:   return VIDEO_FORMAT_YUY2;
	case V4L2_PIX_FMT_UYVY:   return VIDEO_FORMAT_UYVY;
	case V4L2_PIX_FMT_NV12:   return VIDEO_FORMAT_NV12;
	case V4L2_PIX_FMT_YUV420: return VIDEO_FORMAT_I420;
	default:                  return VIDEO_FORMAT_NONE;
	}
}

/*
 * used to store framerate and resolution values
 */
static int pack_tuple(int a, int b)
{
	return (a << 16) | (b & 0xffff);
}

static void unpack_tuple(int *a, int *b, int packed)
{
	*a = packed >> 16;
	*b = packed & 0xffff;
}

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
		out.width = data->width;
		out.height = data->height;
		out.timestamp = timeval2ns(buf.timestamp);
		out.format = v4l2_to_obs_video_format(data->pixelformat);

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

static void v4l2_defaults(obs_data_t settings)
{
	obs_data_set_default_int(settings, "pixelformat", V4L2_PIX_FMT_YUYV);
	obs_data_set_default_int(settings, "resolution",
			pack_tuple(640, 480));
	obs_data_set_default_int(settings, "framerate", pack_tuple(1, 30));
}

/*
 * List available devices
 */
static void v4l2_device_list(obs_property_t prop, obs_data_t settings)
{
	DIR *dirp;
	struct dirent *dp;
	int fd;
	struct v4l2_capability video_cap;
	struct dstr device;
	bool first = true;

	obs_property_list_clear(prop);
	dstr_init_copy(&device, "/dev/");

	dirp = opendir("/sys/class/video4linux");
	if (dirp) {
		while ((dp = readdir(dirp)) != NULL) {
			dstr_resize(&device, 5);
			dstr_cat(&device, dp->d_name);
			if ((fd = open(device.array,
						O_RDWR | O_NONBLOCK)) == -1) {
				continue;
			}
			if (ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1) {
				continue;
			} else if (video_cap.capabilities &
					V4L2_CAP_VIDEO_CAPTURE) {
				obs_property_list_add_string(prop,
						(char *) video_cap.card,
						device.array);
				if (first) {
					obs_data_setstring(settings,
						"device_id", device.array);
					first = false;
				}
			}
			close(fd);
		}
		closedir(dirp);
	}
	dstr_free(&device);
}

/*
 * List formats for device
 */
static void v4l2_format_list(int dev, obs_property_t prop)
{
	struct v4l2_fmtdesc fmt;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.index = 0;

	obs_property_list_clear(prop);

	while (ioctl(dev, VIDIOC_ENUM_FMT, &fmt) == 0) {
		if (v4l2_to_obs_video_format(fmt.pixelformat)
				!= VIDEO_FORMAT_NONE) {
			obs_property_list_add_int(prop,
					(char *) fmt.description,
					fmt.pixelformat);
		}
		fmt.index++;
	}
}

/*
 * List resolutions for device and format
 */
static void v4l2_resolution_list(int dev, uint_fast32_t pixelformat,
		obs_property_t prop)
{
	struct v4l2_frmsizeenum frmsize;
	frmsize.pixel_format = pixelformat;
	frmsize.index = 0;
	struct dstr buffer;
	dstr_init(&buffer);

	obs_property_list_clear(prop);

	while (ioctl(dev, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
		dstr_printf(&buffer, "%dx%d", frmsize.discrete.width,
				frmsize.discrete.height);
		obs_property_list_add_int(prop, buffer.array,
				pack_tuple(frmsize.discrete.width,
				frmsize.discrete.height));
		frmsize.index++;
	}
	dstr_free(&buffer);
}

/*
 * List framerates for device and resolution
 */
static void v4l2_framerate_list(int dev, uint_fast32_t pixelformat,
		uint_fast32_t width, uint_fast32_t height, obs_property_t prop)
{
	struct v4l2_frmivalenum frmival;
	frmival.pixel_format = pixelformat;
	frmival.width = width;
	frmival.height = height;
	frmival.index = 0;
	struct dstr buffer;
	dstr_init(&buffer);

	obs_property_list_clear(prop);

	while (ioctl(dev, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
		float fps = (float) frmival.discrete.denominator /
			frmival.discrete.numerator;
		int pack = pack_tuple(frmival.discrete.numerator,
				frmival.discrete.denominator);
		dstr_printf(&buffer, "%.2f", fps);
		obs_property_list_add_int(prop, buffer.array, pack);
		frmival.index++;
	}
	dstr_free(&buffer);
}

/*
 * Device selected callback
 */
static bool device_selected(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	UNUSED_PARAMETER(p);
	int dev = open(obs_data_getstring(settings, "device_id"),
			O_RDWR | O_NONBLOCK);
	obs_property_t prop = obs_properties_get(props, "pixelformat");
	v4l2_format_list(dev, prop);
	obs_property_modified(prop, settings);
	close(dev);
	return true;
}

/*
 * Format selected callback
 */
static bool format_selected(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	UNUSED_PARAMETER(p);
	int dev = open(obs_data_getstring(settings, "device_id"),
			O_RDWR | O_NONBLOCK);
	obs_property_t prop = obs_properties_get(props, "resolution");
	v4l2_resolution_list(dev, obs_data_getint(settings, "pixelformat"),
			prop);
	obs_property_modified(prop, settings);
	close(dev);
	return true;
}

/*
 * Resolution selected callback
 */
static bool resolution_selected(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	UNUSED_PARAMETER(p);
	int width, height;
	int dev = open(obs_data_getstring(settings, "device_id"),
			O_RDWR | O_NONBLOCK);
	obs_property_t prop = obs_properties_get(props, "framerate");
	unpack_tuple(&width, &height, obs_data_getint(settings,
				"resolution"));
	v4l2_framerate_list(dev, obs_data_getint(settings, "pixelformat"),
			width, height, prop);
	obs_property_modified(prop, settings);
	close(dev);
	return true;
}

static obs_properties_t v4l2_properties(const char *locale)
{
	obs_properties_t props = obs_properties_create(locale);
	obs_property_t device_list = obs_properties_add_list(props, "device_id",
			"Device", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_t format_list = obs_properties_add_list(props,
			"pixelformat", "Image Format", OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_INT);
	obs_property_t resolution_list = obs_properties_add_list(props,
			"resolution", "Resolution", OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_INT);
	obs_properties_add_list(props, "framerate", "Frame Rate",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	v4l2_device_list(device_list, NULL);
	obs_property_set_modified_callback(device_list, device_selected);
	obs_property_set_modified_callback(format_list, format_selected);
	obs_property_set_modified_callback(resolution_list,
			resolution_selected);
	return props;
}

static uint32_t v4l2_getwidth(void *vptr)
{
	V4L2_DATA(vptr);

	return data->width;
}

static uint32_t v4l2_getheight(void *vptr)
{
	V4L2_DATA(vptr);

	return data->height;
}

static void v4l2_terminate(struct v4l2_data *data)
{
	if (data->thread) {
		os_event_signal(data->event);
		pthread_join(data->thread, NULL);
		os_event_destroy(data->event);
	}

	if (data->buf_count)
		v4l2_destroy_mmap(data);

	if (data->dev != -1) {
		close(data->dev);
		data->dev = -1;
	}
}

static void v4l2_destroy(void *vptr)
{
	V4L2_DATA(vptr);

	if (!data)
		return;

	v4l2_terminate(data);

	if (data->device)
		bfree(data->device);
	bfree(data);
}

static void v4l2_init(struct v4l2_data *data)
{
	struct v4l2_format fmt;
	struct v4l2_streamparm par;

	data->dev = open(data->device, O_RDWR | O_NONBLOCK);
	if (data->dev == -1) {
		blog(LOG_ERROR, "v4l2-input: Unable to open device: %s",
				data->device);
		goto fail;
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = data->width;
	fmt.fmt.pix.height = data->height;
	fmt.fmt.pix.pixelformat = data->pixelformat;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(data->dev, VIDIOC_S_FMT, &fmt) < 0) {
		blog(LOG_DEBUG, "v4l2-input: unable to set format");
		goto fail;
	}
	data->pixelformat = fmt.fmt.pix.pixelformat;
	data->width = fmt.fmt.pix.width;
	data->height = fmt.fmt.pix.height;

	par.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	par.parm.capture.timeperframe.numerator = data->fps_numerator;
	par.parm.capture.timeperframe.denominator = data->fps_denominator;

	if (ioctl(data->dev, VIDIOC_S_PARM, &par) < 0) {
		blog(LOG_DEBUG, "v4l2-input: unable to set params");
		goto fail;
	}
	data->fps_numerator = par.parm.capture.timeperframe.numerator;
	data->fps_denominator = par.parm.capture.timeperframe.denominator;

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
	return;
fail:
	blog(LOG_DEBUG, "v4l2-input: initialization failed");
	v4l2_terminate(data);
}

static void v4l2_update(void *vptr, obs_data_t settings)
{
	V4L2_DATA(vptr);
	bool restart = false;
	const char *new_device;
	int width, height, fps_num, fps_denom;

	new_device = obs_data_getstring(settings, "device_id");
	if (strlen(new_device) == 0) {
		v4l2_device_list(NULL, settings);
		new_device = obs_data_getstring(settings, "device_id");
	}

	if (!data->device || strcmp(data->device, new_device) != 0) {
		if (data->device)
			bfree(data->device);
		data->device = bstrdup(new_device);
		restart = true;
	}

	if (data->pixelformat != obs_data_getint(settings, "pixelformat")) {
		data->pixelformat = obs_data_getint(settings, "pixelformat");
		restart = true;
	}

	unpack_tuple(&width, &height, obs_data_getint(settings,
				"resolution"));
	if (width != data->width || height != data->height) {
		restart = true;
	}

	unpack_tuple(&fps_num, &fps_denom, obs_data_getint(settings,
				"framerate"));

	if (fps_num != data->fps_numerator
			|| fps_denom != data->fps_denominator) {
		data->fps_numerator = fps_num;
		data->fps_denominator = fps_denom;
		restart = true;
	}

	if (restart) {
		v4l2_terminate(data);

		/* Wait for v4l2_thread to finish before
		 * updating width and height */
		data->width = width;
		data->height = height;

		v4l2_init(data);
	}
}

static void *v4l2_create(obs_data_t settings, obs_source_t source)
{
	UNUSED_PARAMETER(settings);

	struct v4l2_data *data = bzalloc(sizeof(struct v4l2_data));
	data->dev = -1;
	data->source = source;

	v4l2_update(data, settings);
	blog(LOG_DEBUG, "v4l2-input: New input created");

	return data;
}

struct obs_source_info v4l2_input = {
	.id           = "v4l2_input",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.getname      = v4l2_getname,
	.create       = v4l2_create,
	.destroy      = v4l2_destroy,
	.update       = v4l2_update,
	.defaults     = v4l2_defaults,
	.properties   = v4l2_properties,
	.getwidth     = v4l2_getwidth,
	.getheight    = v4l2_getheight
};
