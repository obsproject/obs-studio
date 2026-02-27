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
#include "asio-device-list.h"

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include <obs-module.h>

// blacklist taken from JUCE to which I added the crappy Realtek ASIO, which gave me only headaches
static const char *blacklisted_drivers[] = {"ASIO DirectX Full Duplex", "ASIO Multimedia Driver", "Realtek ASIO", NULL};

static bool is_blacklisted_driver(const char *name)
{
	for (int i = 0; blacklisted_drivers[i]; ++i) {
		if (strcmp(name, blacklisted_drivers[i]) == 0)
			return true;
	}
	return false;
}

static bool read_asio_driver_info(HKEY hKey, const char *subkey, struct asio_driver_entry *entry)
{
	HKEY driverKey;
	LONG err;
	DWORD valueSize;
	wchar_t clsid_str[256];
	wchar_t desc_str[256];

	if (is_blacklisted_driver(subkey)) {
		info2("Skipping blacklisted driver: %s", subkey);
		return false;
	}

	err = RegOpenKeyExA(hKey, subkey, 0, KEY_READ, &driverKey);
	if (err != ERROR_SUCCESS) {
		info2("Failed to open subkey: %s (err=%ld)", subkey, err);
		return false;
	}

	valueSize = sizeof(clsid_str);
	err = RegGetValueW(driverKey, NULL, L"CLSID", RRF_RT_REG_SZ, NULL, clsid_str, &valueSize);
	if (err != ERROR_SUCCESS) {
		info2("Could not read CLSID for %s (err=%ld)", subkey, err);
		RegCloseKey(driverKey);
		return false;
	}

	if (CLSIDFromString(clsid_str, &entry->clsid) != S_OK) {
		info2("CLSIDFromString failed for %s → %ls", subkey, clsid_str);
		RegCloseKey(driverKey);
		return false;
	}

	valueSize = sizeof(desc_str);
	err = RegGetValueW(driverKey, NULL, L"Description", RRF_RT_REG_SZ, NULL, desc_str, &valueSize);
	if (err != ERROR_SUCCESS) {
		info2("Missing Description for %s, using subkey as name", subkey);
		StringCchCopyA(entry->name, sizeof(entry->name), subkey);
	} else {
		WideCharToMultiByte(CP_UTF8, 0, desc_str, -1, entry->name, sizeof(entry->name), NULL, NULL);
	}

	entry->loadable = true;

	RegCloseKey(driverKey);
	return true;
}

struct asio_device_list *asio_device_list_create(void)
{
	HKEY hKey;
	LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\ASIO", 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS) {
		blog(LOG_ERROR, "[ASIO] Failed to open registry key SOFTWARE\\ASIO (error code: %ld)", result);
		return NULL;
	}

	blog(LOG_INFO, "[ASIO] Successfully opened registry key SOFTWARE\\ASIO");

	struct asio_device_list *list = calloc(1, sizeof(struct asio_device_list));
	if (!list) {
		blog(LOG_ERROR, "[ASIO] Failed to allocate memory for ASIO device list");
		RegCloseKey(hKey);
		return NULL;
	}

	DWORD index = 0;
	char subkey[256];
	DWORD subkey_len;

	while (true) {
		subkey_len = sizeof(subkey);
		LONG enum_result = RegEnumKeyExA(hKey, index++, subkey, &subkey_len, NULL, NULL, NULL, NULL);
		if (enum_result != ERROR_SUCCESS)
			break;

		if (list->count >= MAX_NUM_ASIO_DEVICES) {
			error2("Max number of drivers (%d) reached, skipping others.", MAX_NUM_ASIO_DEVICES);
			break;
		}

		struct asio_driver_entry *entry = &list->drivers[list->count];
		if (read_asio_driver_info(hKey, subkey, entry)) {
			blog(LOG_INFO, "[ASIO] Found ASIO driver: %s", entry->name);
			list->count++;
		} else {
			blog(LOG_WARNING, "[ASIO] Failed to read driver info for subkey: %s", subkey);
		}
	}

	blog(LOG_INFO, "[ASIO] Total drivers found: %zu", list->count);
	list->has_scanned = true;

	RegCloseKey(hKey);
	return list;
}

void asio_device_list_destroy(struct asio_device_list *list)
{
	if (list) {
		free(list);
	}
}

size_t asio_device_list_get_count(const struct asio_device_list *list)
{
	return list ? list->count : 0;
}

const char *asio_device_list_get_name(const struct asio_device_list *list, size_t index)
{
	if (!list || index >= list->count)
		return NULL;
	return list->drivers[index].name;
}

int find_free_driver_slot()
{
	for (int i = 0; i < MAX_NUM_ASIO_DEVICES; ++i) {
		if (current_asio_devices[i] == NULL)
			return i;
	}
	error2("You have more than 16 asio devices, that's too many !\nShip me some...");
	return -1;
}

int asio_device_list_get_index_from_driver_name(const struct asio_device_list *list, const char *name)
{
	if (!list || !name)
		return -1;
	if (!list->has_scanned)
		return -1;
	for (size_t i = 0; i < list->count; ++i) {
		if (strcmp(list->drivers[i].name, name) == 0)
			return (int)i;
	}
	return -1;
}

struct asio_device *asio_device_list_attach_device(struct asio_device_list *list, const char *name)
{
	if (!list || !name)
		return NULL;
	if (!list->has_scanned)
		return NULL;
	int index = asio_device_list_get_index_from_driver_name(list, name);
	if (index >= 0) {
		for (int j = 0; j < MAX_NUM_ASIO_DEVICES; j++) {
			if (current_asio_devices[j]) {
				if (strcmp(name, current_asio_devices[j]->device_name) == 0)
					return current_asio_devices[j];
			}
		}
		int free_slot = find_free_driver_slot();
		if (free_slot >= 0) {
			struct asio_device *dev = asio_device_create(name, list->drivers[index].clsid, free_slot);
			if (!dev)
				list->drivers[index].loadable = false;

			return dev;
		}
	}
	return NULL;
}
