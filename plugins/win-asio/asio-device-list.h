/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

    This file is part of win-asio.
    It uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#pragma once
#include "asio-device.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

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
