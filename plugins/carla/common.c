/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#if !(defined(__APPLE__) || defined(_WIN32))
// needed for libdl stuff and strcasestr
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#endif

#if defined(__linux__) || defined(__FreeBSD__)
#include <unistd.h>
#endif

#include <CarlaUtils.h>

#include <obs-module.h>
#include <util/platform.h>

#include "common.h"

// ----------------------------------------------------------------------------

static char *carla_bin_path = NULL;

const char *get_carla_bin_path(void)
{
	if (carla_bin_path != NULL)
		return carla_bin_path;

	const char *const utilspath = carla_get_library_folder();
	const size_t utilslen = strlen(utilspath);
	char *binpath;

	binpath = bmalloc(utilslen + 28);
	memcpy(binpath, utilspath, utilslen);
#ifdef _WIN32
	memcpy(binpath + utilslen, "\\carla-discovery-native.exe", 28);
#else
	memcpy(binpath + utilslen, "/carla-discovery-native", 24);
#endif

	if (os_file_exists(binpath)) {
		binpath[utilslen] = '\0';
		carla_bin_path = binpath;
		return carla_bin_path;
	}

	free(binpath);

#if !(defined(__APPLE__) || defined(_WIN32))
	// check path of this OBS plugin as fallback
	Dl_info info;
	dladdr(get_carla_bin_path, &info);
	binpath = realpath(info.dli_fname, NULL);

	if (binpath == NULL)
		return NULL;

	// truncate to last separator
	char *lastsep = strrchr(binpath, '/');
	if (lastsep == NULL)
		goto free;
	*lastsep = '\0';

	if (os_file_exists(binpath)) {
		carla_bin_path = bstrdup(binpath);
		free(binpath);
		return carla_bin_path;
	}

free:
	free(binpath);
#endif // !(__APPLE__ || _WIN32)

	return carla_bin_path;
}

void param_index_to_name(uint32_t index, char name[PARAM_NAME_SIZE])
{
	name[1] = '0' + ((index / 100) % 10);
	name[2] = '0' + ((index / 10) % 10);
	name[3] = '0' + ((index / 1) % 10);
}

void remove_all_props(obs_properties_t *props, obs_data_t *settings)
{
	obs_data_erase(settings, PROP_RELOAD_PLUGIN);
	obs_properties_remove_by_name(props, PROP_RELOAD_PLUGIN);

	obs_data_erase(settings, PROP_SHOW_GUI);
	obs_properties_remove_by_name(props, PROP_SHOW_GUI);

	obs_data_erase(settings, PROP_CHUNK);
	obs_properties_remove_by_name(props, PROP_CHUNK);

	obs_data_erase(settings, PROP_CUSTOM_DATA);
	obs_properties_remove_by_name(props, PROP_CUSTOM_DATA);

	char pname[PARAM_NAME_SIZE] = PARAM_NAME_INIT;

	for (uint32_t i = 0; i < MAX_PARAMS; ++i) {
		param_index_to_name(i, pname);
		obs_data_unset_default_value(settings, pname);
		obs_data_erase(settings, pname);
		obs_properties_remove_by_name(props, pname);
	}
}

void postpone_update_request(uint64_t *update_req)
{
	*update_req = os_gettime_ns();
}

void handle_update_request(obs_source_t *source, uint64_t *update_req)
{
	const uint64_t old_update_req = *update_req;

	if (old_update_req == 0)
		return;

	const uint64_t now = os_gettime_ns();

	// request in the future?
	if (now < old_update_req) {
		*update_req = now;
		return;
	}

	if (now - old_update_req >= 100000000ULL) // 100ms
	{
		*update_req = 0;
		signal_handler_signal(obs_source_get_signal_handler(source),
				      "update_properties", NULL);
	}
}

void obs_module_unload(void)
{
	bfree(carla_bin_path);
	carla_bin_path = NULL;
}

// ----------------------------------------------------------------------------
