#define _GNU_SOURCE

#include <obs-module.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#define OBS_V4L2_CARD_LABEL "OBS Virtual Camera"

static int v4l2loopback_ctl_available = 0;

struct virtualcam_data {
	obs_output_t *output;
	int device;
	uint32_t frame_size;
};

static const char *virtualcam_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Virtual Camera Output";
}

static void virtualcam_destroy(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	close(vcam->device);
	bfree(data);
}

static bool is_flatpak_sandbox(void)
{
	static bool flatpak_info_exists = false;
	static bool initialized = false;

	if (!initialized) {
		flatpak_info_exists = access("/.flatpak-info", F_OK) == 0;
		initialized = true;
	}

	return flatpak_info_exists;
}

static int run_command(const char *command)
{
	struct dstr str;
	int result;

	dstr_init_copy(&str, "PATH=\"$PATH:/sbin\" ");

	if (is_flatpak_sandbox())
		dstr_cat(&str, "flatpak-spawn --host ");

	dstr_cat(&str, command);
	result = system(str.array);
	dstr_free(&str);
	return result;
}

static bool loopback_module_loaded()
{
	bool loaded = false;

	char temp[512];

	FILE *fp = fopen("/proc/modules", "r");

	if (!fp)
		return false;

	while (fgets(temp, sizeof(temp), fp)) {
		if (strstr(temp, "v4l2loopback")) {
			loaded = true;
			break;
		}
	}

	fclose(fp);

	return loaded;
}

bool loopback_module_available()
{
	if (run_command("v4l2loopback-ctl -v >/dev/null 2>&1") == 0) {
		v4l2loopback_ctl_available = 1;
	}

	if (loopback_module_loaded()) {
		return true;
	}

	if (run_command("modinfo v4l2loopback >/dev/null 2>&1") == 0) {
		return true;
	}

	return false;
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
		"pkexec v4l2loopback-ctl add -x 1 -n '" OBS_V4L2_CARD_LABEL
		"' && sleep 0.5");
}

static void *virtualcam_create(obs_data_t *settings, obs_output_t *output)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)bzalloc(sizeof(*vcam));
	vcam->output = output;

	UNUSED_PARAMETER(settings);
	return vcam;
}

static bool try_connect(void *data, const char *device, const char *name)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
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
		blog(LOG_ERROR, "Failed to start streaming on '%s' (%s)", device, strerror(errno));
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
			blog(LOG_DEBUG, "v4l2-output: A format truncation may have occurred."
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

static bool virtualcam_start(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;

	if (!loopback_module_loaded()) {
		if (loopback_module_load() != 0) {
			return false;
		}
	}

	bool success = loopback_card_open(data, OBS_V4L2_CARD_LABEL);

	if (!success && v4l2loopback_ctl_available) {
		/* Try to dynamically add a new device using the `v4l2loopback-ctl`
		 * utility, if available and if possible. Only try this once. */

		// TODO: Parse the output of add command and connect directly
		if (loopback_module_add_card() == 0) {
			success = loopback_card_open(data, OBS_V4L2_CARD_LABEL);
		} else {
			v4l2loopback_ctl_available = 0;
		}
	}

	if (!success) {
		success = loopback_card_open(data, NULL);
	}

	if (success) {
		blog(LOG_INFO, "Virtual camera started");
		obs_output_begin_data_capture(vcam->output, 0);
	} else {
		blog(LOG_WARNING, "Failed to start virtual camera");
	}

	return success;
}

static void virtualcam_stop(void *data, uint64_t ts)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	obs_output_end_data_capture(vcam->output);

	struct v4l2_streamparm parm = {0};
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (ioctl(vcam->device, VIDIOC_STREAMOFF, &parm) < 0) {
		blog(LOG_WARNING, "Failed to stop streaming on video device %d (%s)", vcam->device, strerror(errno));
	}

	close(vcam->device);
	blog(LOG_INFO, "Virtual camera stopped");

	UNUSED_PARAMETER(ts);
}

static void virtual_video(void *param, struct video_data *frame)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)param;
	uint32_t frame_size = vcam->frame_size;
	while (frame_size > 0) {
		ssize_t written = write(vcam->device, frame->data[0], vcam->frame_size);
		if (written == -1)
			break;
		frame_size -= written;
	}
}

struct obs_output_info virtualcam_info = {
	.id = "virtualcam_output",
	.flags = OBS_OUTPUT_VIDEO,
	.get_name = virtualcam_name,
	.create = virtualcam_create,
	.destroy = virtualcam_destroy,
	.start = virtualcam_start,
	.stop = virtualcam_stop,
	.raw_video = virtual_video,
};
