/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <obs-module.h>

#define MAX_PARAMS 100

#define PARAM_NAME_SIZE 5
#define PARAM_NAME_INIT                  \
	{                                \
		'p', '0', '0', '0', '\0' \
	}

// property names
#define PROP_LOAD_FILE "load-file"
#define PROP_SELECT_PLUGIN "select-plugin"
#define PROP_RELOAD_PLUGIN "reload"
#define PROP_BUFFER_SIZE "buffer-size"
#define PROP_SHOW_GUI "show-gui"

#define PROP_CHUNK "chunk"
#define PROP_CUSTOM_DATA "customdata"

// ----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

const char *get_carla_bin_path(void);

void param_index_to_name(uint32_t index, char name[PARAM_NAME_SIZE]);
void remove_all_props(obs_properties_t *props, obs_data_t *settings);

void postpone_update_request(uint64_t *update_req);
void handle_update_request(obs_source_t *source, uint64_t *update_req);

#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------
