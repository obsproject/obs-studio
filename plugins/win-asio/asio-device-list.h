/* Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 * This file is part of win-asio.
 *
 * This file uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#pragma once

#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include "iasiodrv.h"
#include "asio-device.h"

struct asio_driver_entry {
	CLSID clsid;
	char name[256];
	bool loadable;
};

struct asio_device_list {
	struct asio_driver_entry drivers[MAX_NUM_ASIO_DEVICES];
	bool has_scanned;
	size_t count;
};

struct asio_device_list *asio_device_list_create(void);
void asio_device_list_destroy(struct asio_device_list *list);
size_t asio_device_list_get_count(const struct asio_device_list *list);
const char *asio_device_list_get_name(const struct asio_device_list *list, size_t index);
struct asio_device *asio_device_list_attach_device(struct asio_device_list *list, const char *name);
int asio_device_list_get_index_from_driver_name(const struct asio_device_list *list, const char *name);

static WCHAR *utf8_to_wide(const char *str)
{
	if (!str)
		return NULL;

	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	if (size_needed <= 0)
		return NULL;

	WCHAR *wstr = (WCHAR *)malloc(size_needed * sizeof(WCHAR));
	if (!wstr)
		return NULL;

	if (MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, size_needed) == 0) {
		free(wstr);
		return NULL;
	}

	return wstr;
}
