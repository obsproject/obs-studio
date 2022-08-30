// SPDX-FileCopyrightText: 2022 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "vaapi-utils.h"

#include <util/bmem.h>
#include <util/dstr.h>

#include <va/va_drm.h>
#include <va/va_str.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

static bool version_logged = false;

inline static VADisplay vaapi_open_display_drm(int *fd, const char *device_path)
{
	VADisplay va_dpy;

	if (!device_path)
		return NULL;

	*fd = open(device_path, O_RDWR);
	if (*fd < 0) {
		blog(LOG_ERROR, "VAAPI: Failed to open device '%s'",
		     device_path);
		return NULL;
	}

	va_dpy = vaGetDisplayDRM(*fd);

	if (!va_dpy) {
		blog(LOG_ERROR, "VAAPI: Failed to initialize DRM display");
		return NULL;
	}

	return va_dpy;
}

inline static void vaapi_close_display_drm(int *fd)
{
	if (*fd < 0)
		return;

	close(*fd);
	*fd = -1;
}

static void vaapi_log_info_cb(void *user_context, const char *message)
{
	UNUSED_PARAMETER(user_context);

	// Libva message always ends with a newline
	struct dstr m;
	dstr_init_copy(&m, message);
	dstr_depad(&m);

	blog(LOG_DEBUG, "Libva: %s", m.array);

	dstr_free(&m);
}

static void vaapi_log_error_cb(void *user_context, const char *message)
{
	UNUSED_PARAMETER(user_context);

	// Libva message always ends with a newline
	struct dstr m;
	dstr_init_copy(&m, message);
	dstr_depad(&m);

	blog(LOG_DEBUG, "Libva error: %s", m.array);

	dstr_free(&m);
}

VADisplay vaapi_open_device(int *fd, const char *device_path,
			    const char *func_name)
{
	VADisplay va_dpy;
	VAStatus va_status;
	int major, minor;
	const char *driver;

	va_dpy = vaapi_open_display_drm(fd, device_path);
	if (!va_dpy)
		return NULL;

	blog(LOG_DEBUG, "VAAPI: Initializing display in %s", func_name);

	vaSetInfoCallback(va_dpy, vaapi_log_info_cb, NULL);
	vaSetErrorCallback(va_dpy, vaapi_log_error_cb, NULL);

	va_status = vaInitialize(va_dpy, &major, &minor);

	if (va_status != VA_STATUS_SUCCESS) {
		blog(LOG_ERROR, "VAAPI: Failed to initialize display in %s",
		     func_name);
		return NULL;
	}

	blog(LOG_DEBUG, "VAAPI: Display initialized");

	if (!version_logged) {
		blog(LOG_INFO, "VAAPI: API version %d.%d", major, minor);
		version_logged = true;
	}

	driver = vaQueryVendorString(va_dpy);

	blog(LOG_DEBUG, "VAAPI: '%s' in use for device '%s'", driver,
	     device_path);

	return va_dpy;
}

void vaapi_close_device(int *fd, VADisplay dpy)
{
	vaTerminate(dpy);
	vaapi_close_display_drm(fd);
}

static uint32_t vaapi_display_ep_combo_rate_controls(VAProfile profile,
						     VAEntrypoint entrypoint,
						     VADisplay dpy,
						     const char *device_path)
{
	bool ret = false;
	VAStatus va_status;
	VAConfigAttrib attrib[1];
	attrib->type = VAConfigAttribRateControl;

	va_status = vaGetConfigAttributes(dpy, profile, entrypoint, attrib, 1);

	switch (va_status) {
	case VA_STATUS_SUCCESS:
		return attrib->value;
	case VA_STATUS_ERROR_UNSUPPORTED_PROFILE:
		blog(LOG_DEBUG, "VAAPI: %s is not supported by the device '%s'",
		     vaProfileStr(profile), device_path);
		return 0;
	case VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT:
		blog(LOG_DEBUG,
		     "VAAPI: %s %s is not supported by the device '%s'",
		     vaProfileStr(profile), vaEntrypointStr(entrypoint),
		     device_path);
		return 0;
	default:
		blog(LOG_ERROR,
		     "VAAPI: Fail to get RC attribute from the %s %s of the device '%s'",
		     vaProfileStr(profile), vaEntrypointStr(entrypoint),
		     device_path);
		return 0;
	}
}

static bool vaapi_display_ep_combo_supported(VAProfile profile,
					     VAEntrypoint entrypoint,
					     VADisplay dpy,
					     const char *device_path)
{
	uint32_t ret = vaapi_display_ep_combo_rate_controls(profile, entrypoint,
							    dpy, device_path);
	if (ret & VA_RC_CBR || ret & VA_RC_CQP || ret & VA_RC_VBR)
		return true;

	return false;
}

bool vaapi_device_rc_supported(VAProfile profile, VADisplay dpy, uint32_t rc,
			       const char *device_path)
{
	uint32_t ret = vaapi_display_ep_combo_rate_controls(
		profile, VAEntrypointEncSlice, dpy, device_path);
	if (ret & rc)
		return true;
	ret = vaapi_display_ep_combo_rate_controls(
		profile, VAEntrypointEncSliceLP, dpy, device_path);
	if (ret & rc)
		return true;

	return false;
}

#define CHECK_PROFILE(ret, profile, va_dpy, device_path)                      \
	if (vaapi_display_ep_combo_supported(profile, VAEntrypointEncSlice,   \
					     va_dpy, device_path)) {          \
		blog(LOG_DEBUG, "'%s' support encoding with %s", device_path, \
		     vaProfileStr(profile));                                  \
		ret |= true;                                                  \
	}

#define CHECK_PROFILE_LP(ret, profile, va_dpy, device_path)                   \
	if (vaapi_display_ep_combo_supported(profile, VAEntrypointEncSliceLP, \
					     va_dpy, device_path)) {          \
		blog(LOG_DEBUG, "'%s' support low power encoding with %s",    \
		     device_path, vaProfileStr(profile));                     \
		ret |= true;                                                  \
	}

bool vaapi_display_h264_supported(VADisplay dpy, const char *device_path)
{
	bool ret = false;

	CHECK_PROFILE(ret, VAProfileH264ConstrainedBaseline, dpy, device_path);
	CHECK_PROFILE(ret, VAProfileH264Main, dpy, device_path);
	CHECK_PROFILE(ret, VAProfileH264High, dpy, device_path);

	if (!ret) {
		CHECK_PROFILE_LP(ret, VAProfileH264ConstrainedBaseline, dpy,
				 device_path);
		CHECK_PROFILE_LP(ret, VAProfileH264Main, dpy, device_path);
		CHECK_PROFILE_LP(ret, VAProfileH264High, dpy, device_path);
	}

	return ret;
}

bool vaapi_device_h264_supported(const char *device_path)
{
	bool ret = false;
	VADisplay va_dpy;

	int drm_fd = -1;

	va_dpy = vaapi_open_device(&drm_fd, device_path,
				   "vaapi_device_h264_supported");
	if (!va_dpy)
		return false;

	ret = vaapi_display_h264_supported(va_dpy, device_path);

	vaapi_close_device(&drm_fd, va_dpy);

	return ret;
}

const char *vaapi_get_h264_default_device()
{
	static const char *default_h264_device = NULL;

	if (!default_h264_device) {
		bool ret = false;
		char path[32] = "/dev/dri/renderD1";
		for (int i = 28;; i++) {
			sprintf(path, "/dev/dri/renderD1%d", i);
			if (access(path, F_OK) != 0)
				break;

			ret = vaapi_device_h264_supported(path);
			if (ret) {
				default_h264_device = strdup(path);
				break;
			}
		}
	}

	return default_h264_device;
}
