#define _GNU_SOURCE

#include <obs-module.h>
#include <util/dstr.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include "linux-virtualcam.h"

#define OBS_V4L2_CARD_LABEL "OBS Virtual Camera"

struct v4l2_output_data {
	obs_output_t *output;
	int device;
	uint32_t frame_size;
};

static bool loopback_module_loaded()
{
	return module_loaded("v4l2loopback");
}

bool video_possible()
{
	if (loopback_module_loaded()) {
		return true;
	}

	return access("/sys/module/v4l2loopback", F_OK) == 0;
}

static int loopback_module_load()
{
	return run_command(
		"pkexec modprobe v4l2loopback exclusive_caps=1 card_label='" OBS_V4L2_CARD_LABEL
		"' && sleep 0.5");
}

static int loopback_module_add_card()
{
	return run_command(
		"pkexec v4l2loopback-ctl add -n '" OBS_V4L2_CARD_LABEL
		"' && sleep 0.5");
}

static bool try_connect(void *data, const char *device, const char *name)
{
	struct v4l2_output_data *vcam = (struct v4l2_output_data *)data;
	struct v4l2_format format = {0};
	struct v4l2_capability capability = {0};
	struct v4l2_streamparm parm = {0};

	vcam->device = open(device, O_RDWR);

	if (vcam->device < 0)
		return false;

	if (ioctl(vcam->device, VIDIOC_QUERYCAP, &capability) < 0) {
		blog(LOG_WARNING,
		     "v4l2-output: VIDIOC_QUERYCAP failed: device:%s (%s)",
		     device, strerror(errno));
		goto fail_close_device;
	}

	blog(LOG_DEBUG, "v4l2-output: found device: '%s'", capability.card);
	if (name &&
	    strncmp((const char *)capability.card, name, strlen(name)) != 0) {
		goto fail_close_device;
	}

	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (ioctl(vcam->device, VIDIOC_G_FMT, &format) < 0)
		goto fail_close_device;

	struct obs_video_info ovi;
	obs_get_video_info(&ovi);

	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	parm.parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	parm.parm.output.timeperframe.numerator = ovi.fps_den;
	parm.parm.output.timeperframe.denominator = ovi.fps_num;

	if (ioctl(vcam->device, VIDIOC_S_PARM, &parm) < 0)
		goto fail_close_device;

	uint32_t width = obs_output_get_width(vcam->output);
	uint32_t height = obs_output_get_height(vcam->output);
	vcam->frame_size = width * height * 2;

	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.fmt.pix.field = V4L2_FIELD_NONE;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	format.fmt.pix.sizeimage = vcam->frame_size;

	if (ioctl(vcam->device, VIDIOC_S_FMT, &format) < 0)
		goto fail_close_device;

	struct video_scale_info vsi = {0};
	vsi.format = VIDEO_FORMAT_YUY2;
	vsi.width = width;
	vsi.height = height;
	obs_output_set_video_conversion(vcam->output, &vsi);

	memset(&parm, 0, sizeof(parm));
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (ioctl(vcam->device, VIDIOC_STREAMON, &parm) < 0) {
		blog(LOG_ERROR, "Failed to start streaming on '%s' (%s)",
		     device, strerror(errno));
		goto fail_close_device;
	}

	blog(LOG_INFO, "v4l2-output: Using device '%s' at '%s'",
	     capability.card, device);

	return true;

fail_close_device:
	close(vcam->device);
	vcam->device = 0;
	return false;
}

static int scanfilter(const struct dirent *entry)
{
	return !astrcmp_n(entry->d_name, "video", 5);
}

static bool loopback_card_open(void *data, const char *name)
{
	struct dirent **list;
	bool success = false;
	int n;

	n = scandir("/dev", &list, scanfilter,
#if defined(__linux__)
		    versionsort
#else
		    alphasort
#endif
	);

	if (n == -1)
		return false;

	for (int i = 0; i < n; i++) {
		char device[32] = {0};

		// Use the return value of snprintf to prevent truncation warning.
		int written = snprintf(device, 32, "/dev/%s", list[i]->d_name);
		if (written >= 32)
			blog(LOG_DEBUG,
			     "v4l2-output: A format truncation may have occurred."
			     " This can be ignored since it is quite improbable.");

		if (try_connect(data, device, name)) {
			success = true;
			break;
		}
	}

	while (n--)
		free(list[n]);
	free(list);

	return success;
}

void *video_start(obs_output_t *output)
{
	if (!loopback_module_loaded()) {
		if (loopback_module_load() != 0) {
			return NULL;
		}
	}

	struct v4l2_output_data data = {
		.output = output,
		.device = -1,
		.frame_size = 0,
	};

	bool success = loopback_card_open(&data, OBS_V4L2_CARD_LABEL);

	if (!success) {
		// TODO: Parse the output of add command and connect directly
		loopback_module_add_card();
		success = loopback_card_open(&data, OBS_V4L2_CARD_LABEL);
	}

	if (!success) {
		success = loopback_card_open(&data, NULL);
	}

	if (success) {
		struct v4l2_output_data *vcam =
			(struct v4l2_output_data *)bzalloc(sizeof(*vcam));

		memcpy(vcam, &data, sizeof(*vcam));
		return vcam;
	}

	blog(LOG_WARNING, "Failed to start v4l2 output");
	return NULL;
}

void video_stop(void *data)
{
	if (!data)
		return;

	struct v4l2_output_data *vcam = (struct v4l2_output_data *)data;

	close(vcam->device);
	bfree(data);
}

void virtual_video(void *data, struct video_data *frame)
{
	struct v4l2_output_data *vcam = (struct v4l2_output_data *)data;
	uint32_t frame_size = vcam->frame_size;
	while (frame_size > 0) {
		ssize_t written =
			write(vcam->device, frame->data[0], vcam->frame_size);
		if (written == -1)
			break;
		frame_size -= written;
	}
}
