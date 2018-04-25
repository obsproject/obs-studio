#pragma once

#include <va/va.h>

struct vaapi_display;
typedef struct vaapi_display vaapi_display_t;

enum vaapi_display_type {
	VAAPI_DISPLAY_X,
	VAAPI_DISPLAY_DRM
};
typedef enum vaapi_display_type vaapi_display_type_t;

vaapi_display_t *vaapi_display_create(vaapi_display_type_t type,
		const char *path);
void vaapi_display_destroy(vaapi_display_t *display);

vaapi_display_type_t vaapi_display_type(vaapi_display_t *d);
const char *vaapi_display_name(vaapi_display_t *d);
const char *vaapi_display_path(vaapi_display_t *d);

void vaapi_display_lock(vaapi_display_t *d);
void vaapi_display_unlock(vaapi_display_t *d);
