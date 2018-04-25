#include <obs-module.h>
#include <util/darray.h>

#include "vaapi-display.h"
#include "vaapi-caps.h"
#include "vaapi-internal.h"

#include <va/va_x11.h>
#include <va/va_drm.h>

#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>

static bool init_va_display(VADisplay display)
{
	VAStatus status;
	int major_version;
	int minor_version;

	if (!vaDisplayIsValid(display))
		return false;

	CHECK_STATUS_FALSE(vaInitialize(display, &major_version,
			&minor_version));

	return true;
}

void vaapi_display_close(vaapi_display_t *d)
{
	if (d->display != NULL) {
		vaTerminate(d->display);
		d->display = NULL;
	}

	if (d->type == VAAPI_DISPLAY_DRM) {
		if (d->drm_fd >= 0) {
			close(d->drm_fd);
			d->drm_fd = 0;
		}
	}
	else if (d->type == VAAPI_DISPLAY_X) {
		if (d->x_display != NULL) {
			XCloseDisplay(d->x_display);
			d->x_display = NULL;
		}
	}
}

static bool init_drm_display(vaapi_display_t *d)
{
	d->drm_fd = open(d->path, O_RDWR);
	if (d->drm_fd < 0)
		return false;

	d->display = vaGetDisplayDRM(d->drm_fd);
	if (d->display == NULL)
		return false;

	return true;
}

static bool init_x_display(vaapi_display_t *d)
{

	d->x_display = XOpenDisplay(NULL);
	if (d->x_display == NULL)
		return false;

	d->display = vaGetDisplay(d->x_display);
	if (d->display == NULL)
		return false;

	return true;
}

vaapi_display_type_t vaapi_display_type(vaapi_display_t *d)
{
	return d->type;
}

const char *vaapi_display_name(vaapi_display_t *d)
{
	return d->name;
}

const char *vaapi_display_path(vaapi_display_t *d)
{
	return d->path;
}

void vaapi_display_lock(vaapi_display_t *d)
{
	pthread_mutex_lock(&d->mutex);
}

void vaapi_display_unlock(vaapi_display_t *d)
{
	pthread_mutex_unlock(&d->mutex);
}

bool vaapi_display_open(vaapi_display_t *d)
{
	// already open
	if (d->display != NULL)
		return true;

	if (d->type == VAAPI_DISPLAY_X) {
		if (!init_x_display(d))
			goto fail;

	} else if (d->type == VAAPI_DISPLAY_DRM) {
		if (!init_drm_display(d))
			goto fail;
	} else {
		VA_LOG(LOG_ERROR, "invalid vaapi display type");
		goto fail;
	}

	if (!init_va_display(d->display))
		goto fail;

	return true;

fail:
	vaapi_display_close(d);
	return false;
}

void vaapi_display_destroy(vaapi_display_t *d)
{
	vaapi_display_close(d);

	da_free(d->caps);
	bfree((char *)d->path);
	bfree((char *)d->name);
	pthread_mutex_destroy(&d->mutex);
	bfree(d);
}

vaapi_display_t *vaapi_display_create(vaapi_display_type_t type,
		const char *path)
{
	vaapi_display_t *d;
	size_t path_len;
	const char *name;
	size_t name_len;

	d = bzalloc(sizeof(vaapi_display_t));

	d->type = type;

	pthread_mutex_init(&d->mutex, NULL);

	if (path != NULL) {
		path_len = strlen(path);
		d->path = bzalloc(path_len + 1);
		strcpy((char *)d->path, path);
	}

	if (type == VAAPI_DISPLAY_X) {
		if (!init_x_display(d))
			goto fail;

	} else if (type == VAAPI_DISPLAY_DRM) {
		if (!init_drm_display(d))
			goto fail;
	} else {
		VA_LOG(LOG_ERROR, "invalid vaapi display type");
		goto fail;
	}

	if (!init_va_display(d->display))
		goto fail;

	name = vaQueryVendorString(d->display);
	if (name != NULL) {
		name_len = strlen(name);
		d->name = bzalloc(name_len + 1);
		strcpy((char *)d->name, name);
	}

	if (!vaapi_profile_caps_init(d))
		goto fail;

	return d;

fail:
	vaapi_display_destroy(d);

	return NULL;
}
