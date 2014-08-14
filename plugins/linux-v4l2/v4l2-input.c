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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <linux/videodev2.h>
#include <libv4l2.h>

#include <util/threading.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <obs-module.h>

#include "v4l2-helpers.h"

#define V4L2_DATA(voidptr) struct v4l2_data *data = voidptr;

#define timeval2ns(tv) \
	(((uint64_t) tv.tv_sec * 1000000000) + ((uint64_t) tv.tv_usec * 1000))

#define blog(level, msg, ...) blog(level, "v4l2-input: " msg, ##__VA_ARGS__)

/**
 * Data structure for the v4l2 source
 *
 * The data is divided into two sections, data being used inside and outside
 * the capture thread. Data used by the capture thread must not be modified
 * from the outside while the thread is running.
 *
 * Data members prefixed with "set_" are settings from the source properties
 * and may be used from outside the capture thread.
 */
struct v4l2_data {
	/* data used outside of the capture thread */
	obs_source_t source;

	pthread_t thread;
	os_event_t event;

	char *set_device;
	int_fast32_t set_input;
	int_fast32_t set_pixfmt;
	int_fast32_t set_res;
	int_fast32_t set_fps;

	/* data used within the capture thread */
	int_fast32_t dev;

	uint64_t frames;
	int_fast32_t width;
	int_fast32_t height;
	int_fast32_t pixfmt;
	uint_fast32_t linesize;

	struct v4l2_buffer_data buffers;
};

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

/**
 * Prepare the output frame structure for obs and compute plane offsets
 *
 * Basically all data apart from memory pointers and the timestamp is known
 * before the capture starts. This function prepares the obs_source_frame
 * struct with all the data that is already known.
 *
 * v4l2 uses a continuous memory segment for all planes so we simply compute
 * offsets to add to the start address in order to give obs the correct data
 * pointers for the individual planes.
 */
static void v4l2_prep_obs_frame(struct v4l2_data *data,
	struct obs_source_frame *frame, size_t *plane_offsets)
{
	memset(frame, 0, sizeof(struct obs_source_frame));
	memset(plane_offsets, 0, sizeof(size_t) * MAX_AV_PLANES);

	frame->width = data->width;
	frame->height = data->height;
	frame->format = v4l2_to_obs_video_format(data->pixfmt);
	video_format_get_parameters(VIDEO_CS_DEFAULT, VIDEO_RANGE_PARTIAL,
		frame->color_matrix, frame->color_range_min,
		frame->color_range_max);

	switch(data->pixfmt) {
	case V4L2_PIX_FMT_NV12:
		frame->linesize[0] = data->linesize;
		frame->linesize[1] = data->linesize / 2;
		plane_offsets[1] = data->linesize * data->height;
		break;
	case V4L2_PIX_FMT_YVU420:
		frame->linesize[0] = data->linesize;
		frame->linesize[1] = data->linesize / 2;
		frame->linesize[2] = data->linesize / 2;
		plane_offsets[1] = data->linesize * data->height * 5 / 4;
		plane_offsets[2] = data->linesize * data->height;
		break;
	case V4L2_PIX_FMT_YUV420:
		frame->linesize[0] = data->linesize;
		frame->linesize[1] = data->linesize / 2;
		frame->linesize[2] = data->linesize / 2;
		plane_offsets[1] = data->linesize * data->height;
		plane_offsets[2] = data->linesize * data->height * 5 / 4;
		break;
	default:
		frame->linesize[0] = data->linesize;
		break;
	}
}

/*
 * Worker thread to get video data
 */
static void *v4l2_thread(void *vptr)
{
	V4L2_DATA(vptr);
	int r;
	fd_set fds;
	uint8_t *start;
	struct timeval tv;
	struct v4l2_buffer buf;
	struct obs_source_frame out;
	size_t plane_offsets[MAX_AV_PLANES];

	if (v4l2_start_capture(data->dev, &data->buffers) < 0)
		goto exit;

	data->frames = 0;

	FD_ZERO(&fds);
	FD_SET(data->dev, &fds);

	v4l2_prep_obs_frame(data, &out, plane_offsets);

	while (os_event_try(data->event) == EAGAIN) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		r = select(data->dev + 1, &fds, NULL, NULL, &tv);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			blog(LOG_DEBUG, "select failed");
			break;
		} else if (r == 0) {
			blog(LOG_DEBUG, "select timeout");
			continue;
		}

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (v4l2_ioctl(data->dev, VIDIOC_DQBUF, &buf) < 0) {
			if (errno == EAGAIN)
				continue;
			blog(LOG_DEBUG, "failed to dequeue buffer");
			break;
		}

		out.timestamp = timeval2ns(buf.timestamp);
		start = (uint8_t *) data->buffers.info[buf.index].start;
		for (uint_fast32_t i = 0; i < MAX_AV_PLANES; ++i)
			out.data[i] = start + plane_offsets[i];
		obs_source_output_video(data->source, &out);

		if (v4l2_ioctl(data->dev, VIDIOC_QBUF, &buf) < 0) {
			blog(LOG_DEBUG, "failed to enqueue buffer");
			break;
		}

		data->frames++;
	}

	blog(LOG_INFO, "Stopped capture after %"PRIu64" frames", data->frames);

exit:
	v4l2_stop_capture(data->dev);
	return NULL;
}

static const char* v4l2_getname(void)
{
	return obs_module_text("V4L2Input");
}

static void v4l2_defaults(obs_data_t settings)
{
	obs_data_set_default_int(settings, "input", 0);
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

	dirp = opendir("/sys/class/video4linux");
	if (!dirp)
		return;

	obs_property_list_clear(prop);

	dstr_init_copy(&device, "/dev/");

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_type == DT_DIR)
			continue;

		dstr_resize(&device, 5);
		dstr_cat(&device, dp->d_name);

		if ((fd = v4l2_open(device.array, O_RDWR | O_NONBLOCK)) == -1) {
			blog(LOG_INFO, "Unable to open %s", device.array);
			continue;
		}

		if (v4l2_ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1) {
			blog(LOG_INFO, "Failed to query capabilities for %s",
			     device.array);
		} else if (video_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
			obs_property_list_add_string(prop,
					(char *) video_cap.card,
					device.array);
			if (first) {
				obs_data_set_string(settings,
					"device_id", device.array);
				first = false;
			}
			blog(LOG_INFO, "Found device '%s' at %s",
			     video_cap.card, device.array);
		}
		else {
			blog(LOG_INFO, "%s seems to not support video capture",
			     device.array);
		}

		close(fd);
	}

	closedir(dirp);
	dstr_free(&device);
}

/*
 * List inputs for device
 */
static void v4l2_input_list(int_fast32_t dev, obs_property_t prop)
{
	struct v4l2_input in;
	memset(&in, 0, sizeof(in));

	obs_property_list_clear(prop);

	while (v4l2_ioctl(dev, VIDIOC_ENUMINPUT, &in) == 0) {
		if (in.type & V4L2_INPUT_TYPE_CAMERA) {
			obs_property_list_add_int(prop, (char *) in.name,
					in.index);
			blog(LOG_INFO, "Found input '%s'", in.name);
		}
		in.index++;
	}
}

/*
 * List formats for device
 */
static void v4l2_format_list(int dev, obs_property_t prop)
{
	struct v4l2_fmtdesc fmt;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.index = 0;
	struct dstr buffer;
	dstr_init(&buffer);

	obs_property_list_clear(prop);

	while (v4l2_ioctl(dev, VIDIOC_ENUM_FMT, &fmt) == 0) {
		dstr_copy(&buffer, (char *) fmt.description);
		if (fmt.flags & V4L2_FMT_FLAG_EMULATED)
			dstr_cat(&buffer, " (Emulated)");

		if (v4l2_to_obs_video_format(fmt.pixelformat)
				!= VIDEO_FORMAT_NONE) {
			obs_property_list_add_int(prop, buffer.array,
					fmt.pixelformat);
			blog(LOG_INFO, "Pixelformat: %s (available)",
			     buffer.array);
		} else {
			blog(LOG_INFO, "Pixelformat: %s (unavailable)",
			     buffer.array);
		}
		fmt.index++;
	}

	dstr_free(&buffer);
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

	v4l2_ioctl(dev, VIDIOC_ENUM_FRAMESIZES, &frmsize);

	switch(frmsize.type) {
	case V4L2_FRMSIZE_TYPE_DISCRETE:
		while (v4l2_ioctl(dev, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
			dstr_printf(&buffer, "%dx%d", frmsize.discrete.width,
					frmsize.discrete.height);
			obs_property_list_add_int(prop, buffer.array,
					pack_tuple(frmsize.discrete.width,
					frmsize.discrete.height));
			frmsize.index++;
		}
		break;
	default:
		blog(LOG_INFO, "Stepwise and Continuous framesizes "
			"are currently hardcoded");

		for (const int *packed = v4l2_framesizes; *packed; ++packed) {
			int width;
			int height;
			unpack_tuple(&width, &height, *packed);
			dstr_printf(&buffer, "%dx%d", width, height);
			obs_property_list_add_int(prop, buffer.array, *packed);
		}
		break;
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

	v4l2_ioctl(dev, VIDIOC_ENUM_FRAMEINTERVALS, &frmival);

	switch(frmival.type) {
	case V4L2_FRMIVAL_TYPE_DISCRETE:
		while (v4l2_ioctl(dev, VIDIOC_ENUM_FRAMEINTERVALS,
				&frmival) == 0) {
			float fps = (float) frmival.discrete.denominator /
				frmival.discrete.numerator;
			int pack = pack_tuple(frmival.discrete.numerator,
					frmival.discrete.denominator);
			dstr_printf(&buffer, "%.2f", fps);
			obs_property_list_add_int(prop, buffer.array, pack);
			frmival.index++;
		}
		break;
	default:
		blog(LOG_INFO, "Stepwise and Continuous framerates "
			"are currently hardcoded");

		for (const int *packed = v4l2_framerates; *packed; ++packed) {
			int num;
			int denom;
			unpack_tuple(&num, &denom, *packed);
			float fps = (float) denom / num;
			dstr_printf(&buffer, "%.2f", fps);
			obs_property_list_add_int(prop, buffer.array, *packed);
		}
		break;
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
	int dev = v4l2_open(obs_data_get_string(settings, "device_id"),
			O_RDWR | O_NONBLOCK);
	if (dev == -1)
		return false;

	obs_property_t prop = obs_properties_get(props, "input");
	v4l2_input_list(dev, prop);
	obs_property_modified(prop, settings);
	v4l2_close(dev);
	return true;
}

/*
 * Input selected callback
 */
static bool input_selected(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	UNUSED_PARAMETER(p);
	int dev = v4l2_open(obs_data_get_string(settings, "device_id"),
			O_RDWR | O_NONBLOCK);
	if (dev == -1)
		return false;

	obs_property_t prop = obs_properties_get(props, "pixelformat");
	v4l2_format_list(dev, prop);
	obs_property_modified(prop, settings);
	v4l2_close(dev);
	return true;
}

/*
 * Format selected callback
 */
static bool format_selected(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	UNUSED_PARAMETER(p);
	int dev = v4l2_open(obs_data_get_string(settings, "device_id"),
			O_RDWR | O_NONBLOCK);
	if (dev == -1)
		return false;

	obs_property_t prop = obs_properties_get(props, "resolution");
	v4l2_resolution_list(dev, obs_data_get_int(settings, "pixelformat"),
			prop);
	obs_property_modified(prop, settings);
	v4l2_close(dev);
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
	int dev = v4l2_open(obs_data_get_string(settings, "device_id"),
			O_RDWR | O_NONBLOCK);
	if (dev == -1)
		return false;

	obs_property_t prop = obs_properties_get(props, "framerate");
	unpack_tuple(&width, &height, obs_data_get_int(settings,
				"resolution"));
	v4l2_framerate_list(dev, obs_data_get_int(settings, "pixelformat"),
			width, height, prop);
	obs_property_modified(prop, settings);
	v4l2_close(dev);
	return true;
}

static obs_properties_t v4l2_properties(void)
{
	obs_properties_t props = obs_properties_create();

	obs_property_t device_list = obs_properties_add_list(props,
			"device_id", obs_module_text("Device"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_t input_list = obs_properties_add_list(props,
			"input", obs_module_text("Input"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_t format_list = obs_properties_add_list(props,
			"pixelformat", obs_module_text("VideoFormat"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_t resolution_list = obs_properties_add_list(props,
			"resolution", obs_module_text("Resolution"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_properties_add_list(props,
			"framerate", obs_module_text("FrameRate"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	v4l2_device_list(device_list, NULL);
	obs_property_set_modified_callback(device_list, device_selected);
	obs_property_set_modified_callback(input_list, input_selected);
	obs_property_set_modified_callback(format_list, format_selected);
	obs_property_set_modified_callback(resolution_list,
			resolution_selected);
	return props;
}

static void v4l2_terminate(struct v4l2_data *data)
{
	if (data->thread) {
		os_event_signal(data->event);
		pthread_join(data->thread, NULL);
		os_event_destroy(data->event);
		data->thread = 0;
	}

	v4l2_destroy_mmap(&data->buffers);

	if (data->dev != -1) {
		v4l2_close(data->dev);
		data->dev = -1;
	}
}

static void v4l2_destroy(void *vptr)
{
	V4L2_DATA(vptr);

	if (!data)
		return;

	v4l2_terminate(data);

	if (data->set_device)
		bfree(data->set_device);
	bfree(data);
}

/**
 * Initialize the v4l2 device
 *
 * This function:
 * - tries to open the device
 * - sets pixelformat and requested resolution
 * - sets the requested framerate
 * - maps the buffers
 * - starts the capture thread
 */
static void v4l2_init(struct v4l2_data *data)
{
	struct v4l2_format fmt;
	struct v4l2_streamparm par;
	struct dstr fps;
	int width, height;
	int fps_num, fps_denom;

	blog(LOG_INFO, "Start capture from %s", data->set_device);
	data->dev = v4l2_open(data->set_device, O_RDWR | O_NONBLOCK);
	if (data->dev == -1) {
		blog(LOG_ERROR, "Unable to open device");
		goto fail;
	}

	/* set input */
	if (v4l2_ioctl(data->dev, VIDIOC_S_INPUT, &data->set_input) < 0) {
		blog(LOG_ERROR, "Unable to set input");
		goto fail;
	}

	/* set pixel format and resolution */
	unpack_tuple(&width, &height, data->set_res);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = data->set_pixfmt;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	if (v4l2_ioctl(data->dev, VIDIOC_S_FMT, &fmt) < 0) {
		blog(LOG_ERROR, "Unable to set format");
		goto fail;
	}
	data->width = fmt.fmt.pix.width;
	data->height = fmt.fmt.pix.height;
	data->pixfmt = fmt.fmt.pix.pixelformat;
	data->linesize = fmt.fmt.pix.bytesperline;
	blog(LOG_INFO, "Resolution: %"PRIuFAST32"x%"PRIuFAST32,
	     data->width, data->height);
	blog(LOG_INFO, "Linesize: %"PRIuFAST32" Bytes", data->linesize);

	/* set framerate */
	unpack_tuple(&fps_num, &fps_denom, data->set_fps);
	par.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	par.parm.capture.timeperframe.numerator = fps_num;
	par.parm.capture.timeperframe.denominator = fps_denom;
	if (v4l2_ioctl(data->dev, VIDIOC_S_PARM, &par) < 0) {
		blog(LOG_ERROR, "Unable to set framerate");
		goto fail;
	}
	dstr_init(&fps);
	dstr_printf(&fps, "%.2f",
		(float) par.parm.capture.timeperframe.denominator
		/ par.parm.capture.timeperframe.numerator);
	blog(LOG_INFO, "Framerate: %s fps", fps.array);
	dstr_free(&fps);

	/* map buffers */
	if (v4l2_create_mmap(data->dev, &data->buffers) < 0) {
		blog(LOG_ERROR, "Failed to map buffers");
		goto fail;
	}

	/* start the capture thread */
	if (os_event_init(&data->event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (pthread_create(&data->thread, NULL, v4l2_thread, data) != 0)
		goto fail;
	return;
fail:
	blog(LOG_ERROR, "Initialization failed");
	v4l2_terminate(data);
}

static void v4l2_update(void *vptr, obs_data_t settings)
{
	V4L2_DATA(vptr);
	bool restart = false;
	const char *new_device;

	new_device = obs_data_get_string(settings, "device_id");
	if (strlen(new_device) == 0) {
		v4l2_device_list(NULL, settings);
		new_device = obs_data_get_string(settings, "device_id");
	}

	if (!data->set_device || strcmp(data->set_device, new_device) != 0) {
		if (data->set_device)
			bfree(data->set_device);
		data->set_device = bstrdup(new_device);
		restart = true;
	}

	if (data->set_input != obs_data_get_int(settings, "input")) {
		data->set_input = obs_data_get_int(settings, "input");
		restart = true;
	}

	if (data->set_pixfmt != obs_data_get_int(settings, "pixelformat")) {
		data->set_pixfmt = obs_data_get_int(settings, "pixelformat");
		restart = true;
	}

	if (data->set_res != obs_data_get_int(settings, "resolution")) {
		data->set_res = obs_data_get_int(settings, "resolution");
		restart = true;
	}

	if (data->set_fps != obs_data_get_int(settings, "framerate")) {
		data->set_fps = obs_data_get_int(settings, "framerate");
		restart = true;
	}

	if (restart) {
		v4l2_terminate(data);
		v4l2_init(data);
	}
}

static void *v4l2_create(obs_data_t settings, obs_source_t source)
{
	struct v4l2_data *data = bzalloc(sizeof(struct v4l2_data));
	data->dev = -1;
	data->source = source;

	v4l2_update(data, settings);

	return data;
}

struct obs_source_info v4l2_input = {
	.id             = "v4l2_input",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_ASYNC_VIDEO,
	.get_name       = v4l2_getname,
	.create         = v4l2_create,
	.destroy        = v4l2_destroy,
	.update         = v4l2_update,
	.get_defaults   = v4l2_defaults,
	.get_properties = v4l2_properties
};
