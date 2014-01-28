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

/* ------------------------------------------------------------------------- */
/* Main usage functions */

EXPORT obs_data_t obs_data_create();
EXPORT obs_data_t obs_data_create_from_json(const char *json_string);
EXPORT int obs_data_addref(obs_data_t data);
EXPORT int obs_data_release(obs_data_t data);

EXPORT const char *obs_data_getjson(obs_data_t data);

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
 * Get functions
 * NOTE: use a macro if you use 'defaults' in more than one place
 */
EXPORT const char *obs_data_getstring(obs_data_t data, const char *name,
		const char *def);
EXPORT long long obs_data_getint(obs_data_t data, const char *name,
		long long def);
EXPORT double obs_data_getdouble(obs_data_t data, const char *name, double def);
EXPORT bool obs_data_getbool(obs_data_t data, const char *name, bool def);
EXPORT obs_data_t obs_data_getobj(obs_data_t data, const char *name);
EXPORT obs_data_array_t obs_data_getarray(obs_data_t data, const char *name);

/* Array functions */
EXPORT obs_data_array_t obs_data_array_create();
EXPORT int obs_data_array_addref(obs_data_array_t array);
EXPORT int obs_data_array_release(obs_data_array_t array);

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

/* Item set functions */
EXPORT void obs_data_item_setstring(obs_data_item_t *item, const char *val);
EXPORT void obs_data_item_setint(obs_data_item_t *item, long long val);
EXPORT void obs_data_item_setdouble(obs_data_item_t *item, double val);
EXPORT void obs_data_item_setbool(obs_data_item_t *item, bool val);
EXPORT void obs_data_item_setobj(obs_data_item_t *item, obs_data_t val);
EXPORT void obs_data_item_setarray(obs_data_item_t *item, obs_data_array_t val);

/* Item get functions */
EXPORT const char *obs_data_item_getstring(obs_data_item_t item,
		const char *def);
EXPORT long long obs_data_item_getint(obs_data_item_t item, long long def);
EXPORT double obs_data_item_getdouble(obs_data_item_t item, double def);
EXPORT bool obs_data_item_getbool(obs_data_item_t item, bool def);
EXPORT obs_data_t obs_data_item_getobj(obs_data_item_t item);
EXPORT obs_data_array_t obs_data_item_getarray(obs_data_item_t item);

#ifdef __cplusplus
}
#endif
