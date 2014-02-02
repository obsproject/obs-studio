/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

enum obs_property_type {
	OBS_PROPERTY_INVALID,
	OBS_PROPERTY_INT,
	OBS_PROPERTY_FLOAT,
	OBS_PROPERTY_TEXT,
	OBS_PROPERTY_PATH,
	OBS_PROPERTY_ENUM,
	OBS_PROPERTY_TEXT_LIST,
	OBS_PROPERTY_COLOR,
};

enum obs_dropdown_type {
	OBS_DROPDOWN_INVALID,
	OBS_DROPDOWN_EDIT,
	OBS_DROPDOWN_LIST,
};

struct obs_properties;
struct obs_category;
struct obs_property;
typedef struct obs_properties *obs_properties_t;
typedef struct obs_category   *obs_category_t;
typedef struct obs_property   *obs_property_t;

/* ------------------------------------------------------------------------- */

EXPORT obs_properties_t obs_properties_create();
EXPORT void obs_properties_destroy(obs_properties_t props);

EXPORT obs_category_t obs_properties_add_category(obs_properties_t props,
		const char *name);

EXPORT obs_category_t obs_properties_first_category(obs_properties_t props);

/* ------------------------------------------------------------------------- */

EXPORT void obs_category_add_int(obs_category_t cat, const char *name,
		const char *description, int min, int max, int step);
EXPORT void obs_category_add_float(obs_category_t cat, const char *name,
		const char *description, double min, double max, double step);
EXPORT void obs_category_add_text(obs_category_t cat, const char *name,
		const char *description);
EXPORT void obs_category_add_path(obs_category_t cat, const char *name,
		const char *description);
EXPORT void obs_category_add_enum_list(obs_category_t cat,
		const char *name, const char *description,
		const char **strings);
EXPORT void obs_category_add_text_list(obs_category_t cat,
		const char *name, const char *description,
		const char **strings, enum obs_dropdown_type type);
EXPORT void obs_category_add_color(obs_category_t cat, const char *name,
		const char *description);

EXPORT bool           obs_category_next(obs_category_t *cat);
EXPORT obs_property_t obs_category_first_property(obs_category_t cat);

/* ------------------------------------------------------------------------- */

EXPORT const char *           obs_property_name(obs_property_t p);
EXPORT const char *           obs_property_description(obs_property_t p);
EXPORT enum obs_property_type obs_property_type(obs_property_t p);

EXPORT bool                   obs_property_next(obs_property_t *p);

EXPORT int                    obs_property_int_min(obs_property_t p);
EXPORT int                    obs_property_int_max(obs_property_t p);
EXPORT int                    obs_property_int_step(obs_property_t p);
EXPORT double                 obs_property_float_min(obs_property_t p);
EXPORT double                 obs_property_float_max(obs_property_t p);
EXPORT double                 obs_property_float_step(obs_property_t p);
EXPORT const char **          obs_property_dropdown_strings(obs_property_t p);
EXPORT enum obs_dropdown_type obs_property_dropdown_type(obs_property_t p);

#ifdef __cplusplus
}
#endif
