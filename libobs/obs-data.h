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

struct vec2;
struct vec3;
struct vec4;
struct quat;

/*
 * OBS data settings storage
 *
 *   This is used for retrieving or setting the data settings for things such
 * as sources, encoders, etc.  This is designed for JSON serialization.
 */

struct obs_data;
struct obs_data_item;
struct obs_data_array;
typedef struct obs_data       *obs_data_t;
typedef struct obs_data_item  *obs_data_item_t;
typedef struct obs_data_array *obs_data_array_t;

enum obs_data_type {
	OBS_DATA_NULL,
	OBS_DATA_STRING,
	OBS_DATA_NUMBER,
	OBS_DATA_BOOLEAN,
	OBS_DATA_OBJECT,
	OBS_DATA_ARRAY
};

enum obs_data_number_type {
	OBS_DATA_NUM_INVALID,
	OBS_DATA_NUM_INT,
	OBS_DATA_NUM_DOUBLE
};

/* ------------------------------------------------------------------------- */
/* Main usage functions */

EXPORT obs_data_t obs_data_create();
EXPORT obs_data_t obs_data_create_from_json(const char *json_string);
EXPORT void obs_data_addref(obs_data_t data);
EXPORT void obs_data_release(obs_data_t data);

EXPORT const char *obs_data_getjson(obs_data_t data);

EXPORT void obs_data_apply(obs_data_t target, obs_data_t apply_data);

EXPORT void obs_data_erase(obs_data_t data, const char *name);

/* Set functions */
EXPORT void obs_data_setstring(obs_data_t data, const char *name,
		const char *val);
EXPORT void obs_data_setint(obs_data_t data, const char *name,
		long long val);
EXPORT void obs_data_setdouble(obs_data_t data, const char *name, double val);
EXPORT void obs_data_setbool(obs_data_t data, const char *name, bool val);
EXPORT void obs_data_setobj(obs_data_t data, const char *name, obs_data_t obj);
EXPORT void obs_data_setarray(obs_data_t data, const char *name,
		obs_data_array_t array);

/*
 * Default value functions.
 *
 * These functions check to ensure the value exists, and is of a specific type.
 * If not, it sets the default value instead.
 */
EXPORT void obs_data_set_default_string(obs_data_t data, const char *name,
		const char *val);
EXPORT void obs_data_set_default_int(obs_data_t data, const char *name,
		long long val);
EXPORT void obs_data_set_default_double(obs_data_t data, const char *name,
		double val);
EXPORT void obs_data_set_default_bool(obs_data_t data, const char *name,
		bool val);
EXPORT void obs_data_set_default_obj(obs_data_t data, const char *name,
		obs_data_t obj);

/*
 * Get functions
 * NOTE: use a macro if you use 'defaults' in more than one place
 */
EXPORT const char *obs_data_getstring(obs_data_t data, const char *name);
EXPORT long long obs_data_getint(obs_data_t data, const char *name);
EXPORT double obs_data_getdouble(obs_data_t data, const char *name);
EXPORT bool obs_data_getbool(obs_data_t data, const char *name);
EXPORT obs_data_t obs_data_getobj(obs_data_t data, const char *name);
EXPORT obs_data_array_t obs_data_getarray(obs_data_t data, const char *name);

/* Array functions */
EXPORT obs_data_array_t obs_data_array_create();
EXPORT void obs_data_array_addref(obs_data_array_t array);
EXPORT void obs_data_array_release(obs_data_array_t array);

EXPORT size_t obs_data_array_count(obs_data_array_t array);
EXPORT obs_data_t obs_data_array_item(obs_data_array_t array, size_t idx);
EXPORT size_t obs_data_array_push_back(obs_data_array_t array, obs_data_t obj);
EXPORT void obs_data_array_insert(obs_data_array_t array, size_t idx,
		obs_data_t obj);
EXPORT void obs_data_array_erase(obs_data_array_t array, size_t idx);

/* ------------------------------------------------------------------------- */
/* Item iteration */

EXPORT obs_data_item_t obs_data_first(obs_data_t data);
EXPORT obs_data_item_t obs_data_item_byname(obs_data_t data, const char *name);
EXPORT bool obs_data_item_next(obs_data_item_t *item);
EXPORT void obs_data_item_release(obs_data_item_t *item);
EXPORT void obs_data_item_remove(obs_data_item_t *item);

/* Gets Item type */
EXPORT enum obs_data_type obs_data_item_gettype(obs_data_item_t item);
EXPORT enum obs_data_number_type obs_data_item_numtype(obs_data_item_t item);

/* Item set functions */
EXPORT void obs_data_item_setstring(obs_data_item_t *item, const char *val);
EXPORT void obs_data_item_setint(obs_data_item_t *item, long long val);
EXPORT void obs_data_item_setdouble(obs_data_item_t *item, double val);
EXPORT void obs_data_item_setbool(obs_data_item_t *item, bool val);
EXPORT void obs_data_item_setobj(obs_data_item_t *item, obs_data_t val);
EXPORT void obs_data_item_setarray(obs_data_item_t *item, obs_data_array_t val);

/* Item get functions */
EXPORT const char *obs_data_item_getstring(obs_data_item_t item);
EXPORT long long obs_data_item_getint(obs_data_item_t item);
EXPORT double obs_data_item_getdouble(obs_data_item_t item);
EXPORT bool obs_data_item_getbool(obs_data_item_t item);
EXPORT obs_data_t obs_data_item_getobj(obs_data_item_t item);
EXPORT obs_data_array_t obs_data_item_getarray(obs_data_item_t item);

/* ------------------------------------------------------------------------- */
/* Helper functions for certain structures */
EXPORT void obs_data_set_vec2(obs_data_t data, const char *name,
		const struct vec2 *val);
EXPORT void obs_data_set_vec3(obs_data_t data, const char *name,
		const struct vec3 *val);
EXPORT void obs_data_set_vec4(obs_data_t data, const char *name,
		const struct vec4 *val);
EXPORT void obs_data_set_quat(obs_data_t data, const char *name,
		const struct quat *val);

EXPORT void obs_data_set_default_vec2(obs_data_t data, const char *name,
		const struct vec2 *val);
EXPORT void obs_data_set_default_vec3(obs_data_t data, const char *name,
		const struct vec3 *val);
EXPORT void obs_data_set_default_vec4(obs_data_t data, const char *name,
		const struct vec4 *val);
EXPORT void obs_data_set_default_quat(obs_data_t data, const char *name,
		const struct quat *val);

EXPORT void obs_data_get_vec2(obs_data_t data, const char *name,
		struct vec2 *val);
EXPORT void obs_data_get_vec3(obs_data_t data, const char *name,
		struct vec3 *val);
EXPORT void obs_data_get_vec4(obs_data_t data, const char *name,
		struct vec4 *val);
EXPORT void obs_data_get_quat(obs_data_t data, const char *name,
		struct quat *val);

/* ------------------------------------------------------------------------- */
/* OBS-specific functions */

static inline obs_data_t obs_data_newref(obs_data_t data)
{
	if (data)
		obs_data_addref(data);
	else
		data = obs_data_create();

	return data;
}

#ifdef __cplusplus
}
#endif
