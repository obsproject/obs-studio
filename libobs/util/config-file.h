/******************************************************************************
  Copyright (c) 2013 by Hugh Bailey <obs.jim@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

     1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

     2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

     3. This notice may not be removed or altered from any source
     distribution.
******************************************************************************/

#pragma once

#include "c99defs.h"

/*
 * Generic ini-style config file functions
 */

#ifdef __cplusplus
extern "C" {
#endif

struct config_data;
typedef struct config_data *config_t;

#define CONFIG_SUCCESS      0
#define CONFIG_FILENOTFOUND -1
#define CONFIG_ERROR        -2

enum config_open_type {
	CONFIG_OPEN_EXISTING,
	CONFIG_OPEN_ALWAYS,
};

EXPORT config_t config_create(const char *file);
EXPORT int config_open(config_t *config, const char *file,
		enum config_open_type open_type);
EXPORT int config_open_defaults(config_t config, const char *file);
EXPORT int config_save(config_t config);
EXPORT void config_close(config_t config);

EXPORT size_t config_num_sections(config_t config);
EXPORT const char *config_get_section(config_t config, size_t idx);

EXPORT void config_set_string(config_t config, const char *section,
		const char *name, const char *value);
EXPORT void config_set_int(config_t config, const char *section,
		const char *name, int64_t value);
EXPORT void config_set_uint(config_t config, const char *section,
		const char *name, uint64_t value);
EXPORT void config_set_bool(config_t config, const char *section,
		const char *name, bool value);
EXPORT void config_set_double(config_t config, const char *section,
		const char *name, double value);

EXPORT void config_set_default_string(config_t config, const char *section,
		const char *name, const char *value);
EXPORT void config_set_default_int(config_t config, const char *section,
		const char *name, int64_t value);
EXPORT void config_set_default_uint(config_t config, const char *section,
		const char *name, uint64_t value);
EXPORT void config_set_default_bool(config_t config, const char *section,
		const char *name, bool value);
EXPORT void config_set_default_double(config_t config, const char *section,
		const char *name, double value);

EXPORT const char *config_get_string(config_t config, const char *section,
		const char *name);
EXPORT int64_t config_get_int(config_t config, const char *section,
		const char *name);
EXPORT uint64_t config_get_uint(config_t config, const char *section,
		const char *name);
EXPORT bool config_get_bool(config_t config, const char *section,
		const char *name);
EXPORT double config_get_double(config_t config, const char *section,
		const char *name);

#ifdef __cplusplus
}
#endif
