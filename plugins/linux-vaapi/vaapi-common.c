#include <obs-module.h>
#include <util/darray.h>

#include <va/va.h>
#include <va/va_x11.h>
#include <va/va_drm.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>

#include "vaapi-common.h"

#include "vaapi-caps.h"
#include "vaapi-display.h"
#include "vaapi-internal.h"

DARRAY(vaapi_display_t *) vaapi_displays = {0};

size_t vaapi_get_display_cnt()
{
	return vaapi_displays.num;
}

vaapi_display_t *vaapi_get_display(size_t index)
{
	return vaapi_displays.array[index];
}

vaapi_display_t *vaapi_find_display(vaapi_display_type_t type, const char *path)
{
	for(size_t i = 0; i < vaapi_displays.num; i++) {
		vaapi_display_t *d = vaapi_displays.array[i];
		if (d->type == type) {
			if (path != NULL && !!strlen(path) &&
			    d->path != NULL && !!strlen(d->path)) {
				if (strcmp(path, d->path) == 0)
					return d;
			} else if ((path == NULL || !strlen(path)) &&
			           (d->path == NULL || !strlen(path))) {
				return d;
			}
		}
	}

	return NULL;
}

bool vaapi_initialize()
{

	char path[128] = "/dev/dri/card";
	for (int i = 0;; i++) {
		sprintf(path, "/dev/dri/card%d", i);
		if (access(path, F_OK) == 0) {
			vaapi_display_t *d = vaapi_display_create(
					VAAPI_DISPLAY_DRM, path);
			if (d != NULL) {
				vaapi_display_close(d);
				da_push_back(vaapi_displays, &d);
			}
		} else {
			break;
		}
	}

	vaapi_display_t *d = vaapi_display_create(VAAPI_DISPLAY_X, NULL);
	if (d != NULL)
		da_push_back(vaapi_displays, &d);

	return vaapi_displays.num > 0;
}

void vaapi_shutdown()
{
	for(size_t i = 0; i < vaapi_displays.num; i++) {
		vaapi_display_t *d = vaapi_displays.array[i];
		assert(d != NULL);
		vaapi_display_destroy(d);
	}
	da_free(vaapi_displays);
}
