/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "common.h"

#include <util/platform.h>

// ----------------------------------------------------------------------------

static const struct {
	const char *bin;
	const char *res;
} carla_system_paths[] = {
#if defined(_WIN32)
// TODO
#else
#ifdef __APPLE__
	{"~/Applications/Carla.app/Contents/MacOS",
	 "~/Applications/Carla.app/Contents/MacOS/resources"},
	{"/Applications/Carla.app/Contents/MacOS",
	 "/Applications/Carla.app/Contents/MacOS/resources"},
#endif
	{"/usr/local/lib/carla", "/usr/local/share/carla/resources"},
	{"/usr/lib/carla", "/usr/share/carla/resources"},
};
#endif

	static const char *module_path = NULL;
static const char *resource_path = NULL;

const char *get_carla_bin_path(void)
{
	if (module_path != NULL)
		return module_path;

	for (size_t i = 0;
	     i < sizeof(carla_system_paths) / sizeof(carla_system_paths[0]);
	     ++i) {
		if (os_file_exists(carla_system_paths[i].bin)) {
			module_path = carla_system_paths[i].bin;
			resource_path = carla_system_paths[i].res;
			break;
		}
	}

	return module_path;
}

const char *get_carla_resource_path(void)
{
	if (resource_path != NULL)
		return resource_path;

	for (size_t i = 0;
	     i < sizeof(carla_system_paths) / sizeof(carla_system_paths[0]);
	     ++i) {
		if (os_file_exists(carla_system_paths[i].res)) {
			resource_path = carla_system_paths[i].res;
			break;
		}
	}

	return resource_path;
}

void param_index_to_name(uint32_t index, char name[PARAM_NAME_SIZE])
{
	name[1] = '0' + ((index / 100) % 10);
	name[2] = '0' + ((index / 10) % 10);
	name[3] = '0' + ((index / 1) % 10);
}

void remove_all_props(obs_properties_t *props, obs_data_t *settings)
{
	obs_data_erase(settings, PROP_SHOW_GUI);
	obs_properties_remove_by_name(props, PROP_SHOW_GUI);

	char pname[PARAM_NAME_SIZE] = PARAM_NAME_INIT;

	for (uint32_t i = 0; i < MAX_PARAMS; ++i) {
		param_index_to_name(i, pname);
		obs_data_erase(settings, pname);
		obs_data_unset_default_value(settings, pname);
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

		signal_handler_t *sighandler =
			obs_source_get_signal_handler(source);
		signal_handler_signal(sighandler, "update_properties", NULL);
	}
}

// ----------------------------------------------------------------------------
