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
	if (!close(vcam->device)) {
		blog(LOG_WARNING,
		     "Failed to close virtual camera file descriptor %d (%d)",
		     vcam->device, errno);
	}
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
	if (result == -1) {
		blog(LOG_ERROR, "Failed to run command '%s' (%d)", str.array,
		     errno);
	}
	dstr_free(&str);
	return result;
}

static bool loopback_module_loaded()
{
	bool loaded = false;

	char temp[512];

	FILE *fp = fopen("/proc/modules", "r");

	if (!fp) {
		blog(LOG_WARNING, "Failed to open /proc/modules (%d)", errno);
		return false;
	}

	while (fgets(temp, sizeof(temp), fp)) {
		if (strstr(temp, "v4l2loopback")) {
			loaded = true;
			break;
		}
	}

	if (!fclose(fp)) {
		blog(LOG_WARNING,
		     "Failed to close loopback module check file descriptor (%d)",
		     errno);
	}
	return loaded;
}

bool loopback_module_available()
{
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
		"pkexec modprobe v4l2loopback exclusive_caps=1 card_label='OBS Virtual Camera' && sleep 0.5");
}

static void *virtualcam_create(obs_data_t *settings, obs_output_t *output)
{
	struct virtualcam_data *vcam =
		(struct virtualcam_data *)bzalloc(sizeof(*vcam));
	vcam->output = output;

	UNUSED_PARAMETER(settings);
	return vcam;
}

static bool try_connect(void *data, const char *device)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	struct v4l2_format format;
	struct v4l2_capability capability;
	struct v4l2_streamparm parm;

	uint32_t width = obs_output_get_width(vcam->output);
	uint32_t height = obs_output_get_height(vcam->output);

	vcam->frame_size = width * height * 2;

	vcam->device = open(device, O_RDWR);

	if (vcam->device == -1) {
		blog(LOG_ERROR, "Failed to open device '%s' (%d)", device,
		     errno);
		return false;
	}

	if (ioctl(vcam->device, VIDIOC_QUERYCAP, &capability) == -1) {
		blog(LOG_WARNING,
		     "Failed to query device capabilities on '%s' (%d)", device,
		     errno);
		goto fail_close_device;
	}

	blog(LOG_DEBUG,
	     "Virtual video device: driver='%s', card='%s', capabilities=0x%x, device_caps=0x%x",
	     capability.driver, capability.card, capability.capabilities,
	     capability.device_caps);

	/* The v42loopback driver always supports reporting device node capabilities.
		Here we just make sure to properly follow the API contract which requires
		that before accessing the content of capability.device_caps, we first
		check if the driver does in fact support reporting device capabilities */
	if (!(capability.capabilities & V4L2_CAP_DEVICE_CAPS)) {
		blog(LOG_ERROR,
		     "Driver on device '%s' is unable to report on device node capabilities",
		     device);
		goto fail_close_device;
	}

	if (!(capability.device_caps & V4L2_CAP_VIDEO_OUTPUT)) {
		blog(LOG_ERROR, "Device '%s' does not support video output",
		     device);
		goto fail_close_device;
	}

	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (ioctl(vcam->device, VIDIOC_G_FMT, &format) == -1) {
		blog(LOG_ERROR,
		     "Failed to get video output format from device '%s' (%d)",
		     device, errno);
		goto fail_close_device;
	}

	struct obs_video_info ovi;
	if (!obs_get_video_info(&ovi)) {
		blog(LOG_WARNING, "Failed to get video info for device '%s'",
		     device);
		// .. but do continue anyway
	}

	memset(&parm, 0, sizeof(parm));
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	parm.parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	parm.parm.output.timeperframe.numerator = ovi.fps_den;
	parm.parm.output.timeperframe.denominator = ovi.fps_num;

	if (ioctl(vcam->device, VIDIOC_S_PARM, &parm) == -1) {
		blog(LOG_WARNING,
		     "Failed to set video output format parameter for time per frame on device '%s' (%d)",
		     device, errno);
		goto fail_close_device;
	}

	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	format.fmt.pix.sizeimage = vcam->frame_size;

	if (ioctl(vcam->device, VIDIOC_S_FMT, &format) == -1) {
		blog(LOG_ERROR,
		     "Failed to set video output format to YUYV on device '%s' (%d)",
		     device, errno);
		goto fail_close_device;
	}

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

	blog(LOG_INFO, "Virtual camera started");
	if (!obs_output_begin_data_capture(vcam->output, 0)) {
		blog(LOG_WARNING,
		     "Unable to begin OBS data capture on device '%s'", device);
	}

	blog(LOG_DEBUG, "Using file descriptor %d for video device '%s'",
	     vcam->device, device);

	return true;

fail_close_device:
	close(vcam->device);
	return false;
}

static int scanfilter(const struct dirent *entry)
{
	return !astrcmp_n(entry->d_name, "video", 5);
}

static bool virtualcam_start(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	struct dirent **list;
	bool success = false;
	int n;

	if (loopback_module_loaded()) {
		blog(LOG_DEBUG, "loopback module is already loaded");
	} else {
		if (loopback_module_load() != 0) {
			blog(LOG_WARNING,
			     "Failed to explicitly load loopback module");
			return false;
		}
	}

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

		blog(LOG_DEBUG, "Trying to connect to video device '%s'",
		     device);
		if (try_connect(vcam, device)) {
			blog(LOG_DEBUG,
			     "Successfully connected to video device '%s'",
			     device);
			success = true;
			break;
		}
	}

	while (n--)
		free(list[n]);
	free(list);

	if (!success)
		blog(LOG_WARNING, "Failed to start virtual camera");

	return success;
}

static void virtualcam_stop(void *data, uint64_t ts)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	obs_output_end_data_capture(vcam->output);

	struct v4l2_streamparm parm = {0};
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (ioctl(vcam->device, VIDIOC_STREAMOFF, &parm) < 0) {
		blog(LOG_WARNING,
		     "Failed to stop streaming on video device %d (%s)",
		     vcam->device, strerror(errno));
	}

	if (close(vcam->device) == -1) {
		blog(LOG_WARNING, "Failed to close file descriptor %d (%d)",
		     vcam->device, errno);
	}
	blog(LOG_INFO, "Virtual camera stopped");

	UNUSED_PARAMETER(ts);
}

static void virtual_video(void *param, struct video_data *frame)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)param;
	uint32_t frame_size = vcam->frame_size;
	while (frame_size > 0) {
		ssize_t written =
			write(vcam->device, frame->data[0], vcam->frame_size);
		if (written == -1) {
			blog(LOG_WARNING,
			     "Failed to write video data on file descriptor %d; skipping (remainder of) frame (%d)",
			     vcam->device, errno);
			break;
		}
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
