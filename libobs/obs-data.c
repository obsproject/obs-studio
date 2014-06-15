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

#include "util/bmem.h"
#include "util/threading.h"
#include "util/darray.h"
#include "graphics/vec2.h"
#include "graphics/vec3.h"
#include "graphics/vec4.h"
#include "graphics/quat.h"
#include "obs-data.h"

#include <jansson.h>

struct obs_data_item {
	volatile long        ref;
	struct obs_data      *parent;
	struct obs_data_item *next;
	enum obs_data_type   type;
	size_t               name_len;
	size_t               data_len;
	size_t               capacity;
};

struct obs_data {
	volatile long        ref;
	char                 *json;
	struct obs_data_item *first_item;
};

struct obs_data_array {
	volatile long        ref;
	DARRAY(obs_data_t)   objects;
};

struct obs_data_number {
	enum obs_data_number_type type;
	union {
		long long int_val;
		double    double_val;
	};
};

/* ------------------------------------------------------------------------- */
/* Item structure, designed to be one allocation only */

/* ensures data after the name has alignment (in case of SSE) */
static inline size_t get_name_align_size(const char *name)
{
	size_t name_size = strlen(name) + 1;
	size_t alignment = base_get_alignment();
	size_t total_size;

	total_size = sizeof(struct obs_data_item) + (name_size + alignment-1);
	total_size &= ~(alignment-1);

	return total_size - sizeof(struct obs_data_item);
}

static inline char *get_item_name(struct obs_data_item *item)
{
	return (char*)item + sizeof(struct obs_data_item);
}

static inline void *get_item_data(struct obs_data_item *item)
{
	return (uint8_t*)get_item_name(item) + item->name_len;
}

static inline size_t obs_data_item_total_size(struct obs_data_item *item)
{
	return sizeof(struct obs_data_item) + item->data_len + item->name_len;
}

static inline obs_data_t get_item_obj(struct obs_data_item *item)
{
	if (!item)
		return NULL;

	return *(obs_data_t*)get_item_data(item);
}

static inline obs_data_array_t get_item_array(struct obs_data_item *item)
{
	if (!item)
		return NULL;

	return *(obs_data_array_t*)get_item_data(item);
}

static inline void item_data_release(struct obs_data_item *item)
{
	if (item->type == OBS_DATA_OBJECT) {
		obs_data_t obj = get_item_obj(item);
		obs_data_release(obj);

	} else if (item->type == OBS_DATA_ARRAY) {
		obs_data_array_t array = get_item_array(item);
		obs_data_array_release(array);
	}
}

static inline void item_data_addref(struct obs_data_item *item)
{
	if (item->type == OBS_DATA_OBJECT) {
		obs_data_t obj = get_item_obj(item);
		obs_data_addref(obj);

	} else if (item->type == OBS_DATA_ARRAY) {
		obs_data_array_t array = get_item_array(item);
		obs_data_array_addref(array);
	}
}

static struct obs_data_item *obs_data_item_create(const char *name,
		const void *data, size_t size, enum obs_data_type type)
{
	struct obs_data_item *item;
	size_t name_size, total_size;

	if (!name || !data)
		return NULL;

	name_size = get_name_align_size(name);
	total_size = name_size + sizeof(struct obs_data_item) + size;

	item = bzalloc(total_size);

	item->capacity = total_size;
	item->type     = type;
	item->name_len = name_size;
	item->data_len = size;
	item->ref      = 1;

	strcpy(get_item_name(item), name);
	memcpy(get_item_data(item), data, size);

	item_data_addref(item);
	return item;
}

static struct obs_data_item **get_item_prev_next(struct obs_data *data,
		struct obs_data_item *current)
{
	if (!current || !data)
		return NULL;

	struct obs_data_item **prev_next = &data->first_item;
	struct obs_data_item *item       = data->first_item;

	while (item) {
		if (item == current)
			return prev_next;

		prev_next = &item->next;
		item      = item->next;
	}

	return NULL;
}

static inline void obs_data_item_detach(struct obs_data_item *item)
{
	struct obs_data_item **prev_next = get_item_prev_next(item->parent,
			item);

	if (prev_next) {
		*prev_next = item->next;
		item->next = NULL;
	}
}

static inline void obs_data_item_reattach(struct obs_data_item *old_ptr,
		struct obs_data_item *new_ptr)
{
	struct obs_data_item **prev_next = get_item_prev_next(new_ptr->parent,
			old_ptr);

	if (prev_next)
		*prev_next = new_ptr;
}

static struct obs_data_item *obs_data_item_ensure_capacity(
		struct obs_data_item *item)
{
	size_t new_size = obs_data_item_total_size(item);
	struct obs_data_item *new_item;

	if (item->capacity >= new_size)
		return item;

	new_item = brealloc(item, new_size);
	new_item->capacity = new_size;

	obs_data_item_reattach(item, new_item);
	return new_item;
}

static inline void obs_data_item_destroy(struct obs_data_item *item)
{
	item_data_release(item);
	obs_data_item_detach(item);
	bfree(item);
}

static inline void obs_data_item_setdata(
		struct obs_data_item **p_item, const void *data, size_t size,
		enum obs_data_type type)
{
	if (!p_item || !*p_item)
		return;

	struct obs_data_item *item = *p_item;
	item_data_release(item);

	item->data_len = size;
	item->type     = type;
	item = obs_data_item_ensure_capacity(item);

	if (size) {
		memcpy(get_item_data(item), data, size);
		item_data_addref(item);
	}

	*p_item = item;
}

/* ------------------------------------------------------------------------- */

static void obs_data_add_json_item(obs_data_t data, const char *key,
		json_t *json);

static inline void obs_data_add_json_object_data(obs_data_t data, json_t *jobj)
{
	const char *item_key;
	json_t *jitem;

	json_object_foreach (jobj, item_key, jitem) {
		obs_data_add_json_item(data, item_key, jitem);
	}
}

static inline void obs_data_add_json_object(obs_data_t data, const char *key,
		json_t *jobj)
{
	obs_data_t sub_obj = obs_data_create();

	obs_data_add_json_object_data(sub_obj, jobj);
	obs_data_setobj(data, key, sub_obj);
	obs_data_release(sub_obj);
}

static void obs_data_add_json_array(obs_data_t data, const char *key,
		json_t *jarray)
{
	obs_data_array_t array = obs_data_array_create();
	size_t idx;
	json_t *jitem;

	json_array_foreach (jarray, idx, jitem) {
		obs_data_t item;

		if (!json_is_object(jitem))
			continue;

		item = obs_data_create();
		obs_data_add_json_object_data(item, jitem);
		obs_data_array_push_back(array, item);
		obs_data_release(item);
	}

	obs_data_setarray(data, key, array);
	obs_data_array_release(array);
}

static void obs_data_add_json_item(obs_data_t data, const char *key,
		json_t *json)
{
	if (json_is_object(json))
		obs_data_add_json_object(data, key, json);
	else if (json_is_array(json))
		obs_data_add_json_array(data, key, json);
	else if (json_is_string(json))
		obs_data_setstring(data, key, json_string_value(json));
	else if (json_is_integer(json))
		obs_data_setint(data, key, json_integer_value(json));
	else if (json_is_real(json))
		obs_data_setdouble(data, key, json_real_value(json));
	else if (json_is_true(json))
		obs_data_setbool(data, key, true);
	else if (json_is_false(json))
		obs_data_setbool(data, key, false);
}

/* ------------------------------------------------------------------------- */

static inline void set_json_string(json_t *json, const char *name,
		obs_data_item_t item)
{
	const char *val = obs_data_item_getstring(item);
	json_object_set_new(json, name, json_string(val));
}

static inline void set_json_number(json_t *json, const char *name,
		obs_data_item_t item)
{
	enum obs_data_number_type type = obs_data_item_numtype(item);

	if (type == OBS_DATA_NUM_INT) {
		long long val = obs_data_item_getint(item);
		json_object_set_new(json, name, json_integer(val));
	} else {
		double val = obs_data_item_getdouble(item);
		json_object_set_new(json, name, json_real(val));
	}
}

static inline void set_json_bool(json_t *json, const char *name,
		obs_data_item_t item)
{
	bool val = obs_data_item_getbool(item);
	json_object_set_new(json, name, val ? json_true() : json_false());
}

static json_t *obs_data_to_json(obs_data_t data);

static inline void set_json_obj(json_t *json, const char *name,
		obs_data_item_t item)
{
	obs_data_t obj = obs_data_item_getobj(item);
	json_object_set_new(json, name, obs_data_to_json(obj));
	obs_data_release(obj);
}

static inline void set_json_array(json_t *json, const char *name,
		obs_data_item_t item)
{
	json_t           *jarray = json_array();
	obs_data_array_t array   = obs_data_item_getarray(item);
	size_t           count   = obs_data_array_count(array);

	for (size_t idx = 0; idx < count; idx++) {
		obs_data_t sub_item = obs_data_array_item(array, idx);
		json_t     *jitem   = obs_data_to_json(sub_item);
		json_array_append_new(jarray, jitem);
		obs_data_release(sub_item);
	}

	json_object_set_new(json, name, jarray);
	obs_data_array_release(array);
}

static json_t *obs_data_to_json(obs_data_t data)
{
	json_t *json = json_object();
	obs_data_item_t item = obs_data_first(data);

	while (item) {
		enum obs_data_type type = obs_data_item_gettype(item);
		const char *name        = get_item_name(item);

		if (type == OBS_DATA_STRING)
			set_json_string(json, name, item);
		else if (type == OBS_DATA_NUMBER)
			set_json_number(json, name, item);
		else if (type == OBS_DATA_BOOLEAN)
			set_json_bool(json, name, item);
		else if (type == OBS_DATA_OBJECT)
			set_json_obj(json, name, item);
		else if (type == OBS_DATA_ARRAY)
			set_json_array(json, name, item);

		obs_data_item_next(&item);
	}

	return json;
}

/* ------------------------------------------------------------------------- */

obs_data_t obs_data_create()
{
	struct obs_data *data = bzalloc(sizeof(struct obs_data));
	data->ref = 1;

	return data;
}

obs_data_t obs_data_create_from_json(const char *json_string)
{
	obs_data_t data = obs_data_create();

	json_error_t error;
	json_t *root = json_loads(json_string, JSON_REJECT_DUPLICATES, &error);

	if (root) {
		obs_data_add_json_object_data(data, root);
		json_decref(root);
	} else {
		blog(LOG_ERROR, "obs-data.c: [obs_data_create_from_json] "
		                "Failed reading json string (%d): %s",
		                error.line, error.text);
	}

	return data;
}

void obs_data_addref(obs_data_t data)
{
	if (data)
		os_atomic_inc_long(&data->ref);
}

static inline void obs_data_destroy(struct obs_data *data)
{
	struct obs_data_item *item = data->first_item;

	while (item) {
		struct obs_data_item *next = item->next;
		obs_data_item_release(&item);
		item = next;
	}

	/* NOTE: don't use bfree for json text, allocated by json */
	free(data->json);
	bfree(data);
}

void obs_data_release(obs_data_t data)
{
	if (!data) return;

	if (os_atomic_dec_long(&data->ref) == 0)
		obs_data_destroy(data);
}

const char *obs_data_getjson(obs_data_t data)
{
	if (!data) return NULL;

	/* NOTE: don't use libobs bfree for json text */
	free(data->json);
	data->json = NULL;

	json_t *root = obs_data_to_json(data);
	data->json = json_dumps(root, JSON_PRESERVE_ORDER | JSON_INDENT(4));
	json_decref(root);

	return data->json;
}

static struct obs_data_item *get_item(struct obs_data *data, const char *name)
{
	if (!data) return NULL;

	struct obs_data_item *item = data->first_item;

	while (item) {
		if (strcmp(get_item_name(item), name) == 0)
			return item;

		item = item->next;
	}

	return NULL;
}

static inline struct obs_data_item *get_item_of(struct obs_data *data,
		const char *name, enum obs_data_type type)
{
	if (!data)
		return NULL;

	struct obs_data_item *item = get_item(data, name);
	return (item && item->type == type) ? item : NULL;
}

static void set_item_data(struct obs_data *data, struct obs_data_item *item,
		const char *name, const void *ptr, size_t size,
		enum obs_data_type type)
{
	if (!item) {
		item = obs_data_item_create(name, ptr, size, type);
		item->next = data->first_item;
		item->parent = data;

		data->first_item = item;

	} else {
		obs_data_item_setdata(&item, ptr, size, type);
	}
}

static inline void set_item(struct obs_data *data, const char *name,
		const void *ptr, size_t size, enum obs_data_type type)
{
	if (!data)
		return;

	struct obs_data_item *item = get_item(data, name);
	set_item_data(data, item, name, ptr, size, type);
}

static inline void set_item_def(struct obs_data *data, const char *name,
		const void *ptr, size_t size, enum obs_data_type type)
{
	if (!data)
		return;

	struct obs_data_item *item = get_item(data, name);
	if (item && item->type == type)
		return;

	set_item_data(data, item, name, ptr, size, type);
}

static inline void copy_item(struct obs_data *data, struct obs_data_item *item)
{
	const char *name = get_item_name(item);
	void *ptr = get_item_data(item);

	set_item(data, name, ptr, item->data_len, item->type);
}

void obs_data_apply(obs_data_t target, obs_data_t apply_data)
{
	struct obs_data_item *item;

	if (!target || !apply_data || target == apply_data)
		return;

	item = apply_data->first_item;

	while (item) {
		copy_item(target, item);
		item = item->next;
	}
}

void obs_data_erase(obs_data_t data, const char *name)
{
	struct obs_data_item *item = get_item(data, name);

	if (item) {
		obs_data_item_detach(item);
		obs_data_item_release(&item);
	}
}

void obs_data_setstring(obs_data_t data, const char *name, const char *val)
{
	if (!val) val = "";
	set_item(data, name, val, strlen(val)+1, OBS_DATA_STRING);
}

void obs_data_setint(obs_data_t data, const char *name, long long val)
{
	struct obs_data_number num;
	num.type    = OBS_DATA_NUM_INT;
	num.int_val = val;
	set_item(data, name, &num, sizeof(struct obs_data_number),
			OBS_DATA_NUMBER);
}

void obs_data_setdouble(obs_data_t data, const char *name, double val)
{
	struct obs_data_number num;
	num.type       = OBS_DATA_NUM_DOUBLE;
	num.double_val = val;
	set_item(data, name, &num, sizeof(struct obs_data_number),
			OBS_DATA_NUMBER);
}

void obs_data_setbool(obs_data_t data, const char *name, bool val)
{
	set_item(data, name, &val, sizeof(bool), OBS_DATA_BOOLEAN);
}

void obs_data_setobj(obs_data_t data, const char *name, obs_data_t obj)
{
	set_item(data, name, &obj, sizeof(obs_data_t), OBS_DATA_OBJECT);
}

void obs_data_setarray(obs_data_t data, const char *name,
		obs_data_array_t array)
{
	set_item(data, name, &array, sizeof(obs_data_t), OBS_DATA_ARRAY);
}

void obs_data_set_default_string(obs_data_t data, const char *name,
		const char *val)
{
	if (!val) val = "";
	set_item_def(data, name, val, strlen(val)+1, OBS_DATA_STRING);
}

void obs_data_set_default_int(obs_data_t data, const char *name, long long val)
{
	struct obs_data_number num;
	num.type    = OBS_DATA_NUM_INT;
	num.int_val = val;
	set_item_def(data, name, &num, sizeof(struct obs_data_number),
			OBS_DATA_NUMBER);
}

void obs_data_set_default_double(obs_data_t data, const char *name, double val)
{
	struct obs_data_number num;
	num.type       = OBS_DATA_NUM_DOUBLE;
	num.double_val = val;
	set_item_def(data, name, &num, sizeof(struct obs_data_number),
			OBS_DATA_NUMBER);
}

void obs_data_set_default_bool(obs_data_t data, const char *name, bool val)
{
	set_item_def(data, name, &val, sizeof(bool), OBS_DATA_BOOLEAN);
}

void obs_data_set_default_obj(obs_data_t data, const char *name, obs_data_t obj)
{
	set_item_def(data, name, &obj, sizeof(obs_data_t), OBS_DATA_OBJECT);
}

const char *obs_data_getstring(obs_data_t data, const char *name)
{
	struct obs_data_item *item = get_item_of(data, name, OBS_DATA_STRING);
	return item ? get_item_data(item) : "";
}

static inline long long item_int(struct obs_data_item *item)
{
	if (item) {
		struct obs_data_number *num = get_item_data(item);
		return (num->type == OBS_DATA_NUM_INT) ?
			num->int_val : (long long)num->double_val;
	}

	return 0;
}

static inline double item_double(struct obs_data_item *item)
{
	if (item) {
		struct obs_data_number *num = get_item_data(item);
		return (num->type == OBS_DATA_NUM_INT) ?
			(double)num->int_val : num->double_val;
	}

	return 0.0;
}

long long obs_data_getint(obs_data_t data, const char *name)
{
	return item_int(get_item_of(data, name, OBS_DATA_NUMBER));
}

double obs_data_getdouble(obs_data_t data, const char *name)
{
	return item_double(get_item_of(data, name, OBS_DATA_NUMBER));
}

bool obs_data_getbool(obs_data_t data, const char *name)
{
	struct obs_data_item *item = get_item_of(data, name, OBS_DATA_BOOLEAN);
	return item ? *(bool*)get_item_data(item) : false;
}

obs_data_t obs_data_getobj(obs_data_t data, const char *name)
{
	struct obs_data_item *item = get_item_of(data, name, OBS_DATA_OBJECT);
	obs_data_t obj = get_item_obj(item);

	if (obj)
		os_atomic_inc_long(&obj->ref);
	return obj;
}

obs_data_array_t obs_data_getarray(obs_data_t data, const char *name)
{
	struct obs_data_item *item = get_item_of(data, name, OBS_DATA_ARRAY);
	obs_data_array_t array = get_item_array(item);

	if (array)
		os_atomic_inc_long(&array->ref);
	return array;
}

obs_data_array_t obs_data_array_create()
{
	struct obs_data_array *array = bzalloc(sizeof(struct obs_data_array));
	array->ref = 1;

	return array;
}

void obs_data_array_addref(obs_data_array_t array)
{
	if (array)
		os_atomic_inc_long(&array->ref);
}

static inline void obs_data_array_destroy(obs_data_array_t array)
{
	if (array) {
		for (size_t i = 0; i < array->objects.num; i++)
			obs_data_release(array->objects.array[i]);
		da_free(array->objects);
		bfree(array);
	}
}

void obs_data_array_release(obs_data_array_t array)
{
	if (!array)
		return;

	if (os_atomic_dec_long(&array->ref) == 0)
		obs_data_array_destroy(array);
}

size_t obs_data_array_count(obs_data_array_t array)
{
	return array ? array->objects.num : 0;
}

obs_data_t obs_data_array_item(obs_data_array_t array, size_t idx)
{
	obs_data_t data;

	if (!array)
		return NULL;

	data = (idx < array->objects.num) ? array->objects.array[idx] : NULL;

	if (data)
		os_atomic_inc_long(&data->ref);
	return data;
}

size_t obs_data_array_push_back(obs_data_array_t array, obs_data_t obj)
{
	if (!array || !obj)
		return 0;

	os_atomic_inc_long(&obj->ref);
	return da_push_back(array->objects, &obj);
}

void obs_data_array_insert(obs_data_array_t array, size_t idx, obs_data_t obj)
{
	if (!array || !obj)
		return;

	os_atomic_inc_long(&obj->ref);
	da_insert(array->objects, idx, &obj);
}

void obs_data_array_erase(obs_data_array_t array, size_t idx)
{
	if (array) {
		obs_data_release(array->objects.array[idx]);
		da_erase(array->objects, idx);
	}
}

/* ------------------------------------------------------------------------- */
/* Item iteration */

obs_data_item_t obs_data_first(obs_data_t data)
{
	if (!data)
		return NULL;

	if (data->first_item)
		os_atomic_inc_long(&data->first_item->ref);
	return data->first_item;
}

obs_data_item_t obs_data_item_byname(obs_data_t data, const char *name)
{
	if (!data)
		return NULL;

	struct obs_data_item *item = get_item(data, name);
	if (item)
		os_atomic_inc_long(&item->ref);
	return item;
}

bool obs_data_item_next(obs_data_item_t *item)
{
	if (item && *item) {
		obs_data_item_t next = (*item)->next;
		obs_data_item_release(item);

		*item = next;

		if (next) {
			os_atomic_inc_long(&next->ref);
			return true;
		}
	}

	return false;
}

void obs_data_item_release(obs_data_item_t *item)
{
	if (item && *item) {
		long ref = os_atomic_dec_long(&(*item)->ref);
		if (!ref) {
			obs_data_item_destroy(*item);
			*item = NULL;
		}
	}
}

void obs_data_item_remove(obs_data_item_t *item)
{
	if (item && *item) {
		obs_data_item_detach(*item);
		obs_data_item_release(item);
	}
}

enum obs_data_type obs_data_item_gettype(obs_data_item_t item)
{
	return item ? item->type : OBS_DATA_NULL;
}

enum obs_data_number_type obs_data_item_numtype(obs_data_item_t item)
{
	struct obs_data_number *num;

	if (!item || item->type != OBS_DATA_NUMBER)
		return OBS_DATA_NUM_INVALID;

	num = get_item_data(item);
	return num->type;
}

void obs_data_item_setstring(obs_data_item_t *item, const char *val)
{
	if (!val) val = "";
	obs_data_item_setdata(item, val, strlen(val)+1, OBS_DATA_STRING);
}

void obs_data_item_setint(obs_data_item_t *item, long long val)
{
	struct obs_data_number num;
	num.type    = OBS_DATA_NUM_INT;
	num.int_val = val;
	obs_data_item_setdata(item, &num, sizeof(struct obs_data_number),
			OBS_DATA_NUMBER);
}

void obs_data_item_setdouble(obs_data_item_t *item, double val)
{
	struct obs_data_number num;
	num.type       = OBS_DATA_NUM_DOUBLE;
	num.double_val = val;
	obs_data_item_setdata(item, &num, sizeof(struct obs_data_number),
			OBS_DATA_NUMBER);
}

void obs_data_item_setbool(obs_data_item_t *item, bool val)
{
	obs_data_item_setdata(item, &val, sizeof(bool), OBS_DATA_BOOLEAN);
}

void obs_data_item_setobj(obs_data_item_t *item, obs_data_t val)
{
	obs_data_item_setdata(item, &val, sizeof(obs_data_t), OBS_DATA_OBJECT);
}

void obs_data_item_setarray(obs_data_item_t *item, obs_data_array_t val)
{
	obs_data_item_setdata(item, &val, sizeof(obs_data_array_t),
			OBS_DATA_ARRAY);
}

static inline bool item_valid(struct obs_data_item *item,
		enum obs_data_type type)
{
	return item && item->type == type;
}

const char *obs_data_item_getstring(obs_data_item_t item)
{
	return item_valid(item, OBS_DATA_STRING) ? get_item_data(item) : "";
}

long long obs_data_item_getint(obs_data_item_t item)
{
	return item_valid(item, OBS_DATA_NUMBER) ?
		item_int(item) : 0;
}

double obs_data_item_getdouble(obs_data_item_t item)
{
	return item_valid(item, OBS_DATA_NUMBER) ?
		item_double(item) : 0.0;
}

bool obs_data_item_getbool(obs_data_item_t item)
{
	return item_valid(item, OBS_DATA_BOOLEAN) ?
		*(bool*)get_item_data(item) : false;
}

obs_data_t obs_data_item_getobj(obs_data_item_t item)
{
	obs_data_t obj = item_valid(item, OBS_DATA_OBJECT) ?
		get_item_obj(item) : NULL;

	if (obj)
		os_atomic_inc_long(&obj->ref);
	return obj;
}

obs_data_array_t obs_data_item_getarray(obs_data_item_t item)
{
	obs_data_array_t array = item_valid(item, OBS_DATA_ARRAY) ?
		get_item_array(item) : NULL;

	if (array)
		os_atomic_inc_long(&array->ref);
	return array;
}

/* ------------------------------------------------------------------------- */
/* Helper functions for certain structures */
void obs_data_set_vec2(obs_data_t data, const char *name,
		const struct vec2 *val)
{
	obs_data_t obj = obs_data_create();
	obs_data_setdouble(obj, "x", val->x);
	obs_data_setdouble(obj, "y", val->y);
	obs_data_setobj(data, name, obj);
	obs_data_release(obj);
}

void obs_data_set_vec3(obs_data_t data, const char *name,
		const struct vec3 *val)
{
	obs_data_t obj = obs_data_create();
	obs_data_setdouble(obj, "x", val->x);
	obs_data_setdouble(obj, "y", val->y);
	obs_data_setdouble(obj, "z", val->z);
	obs_data_setobj(data, name, obj);
	obs_data_release(obj);
}

void obs_data_set_vec4(obs_data_t data, const char *name,
		const struct vec4 *val)
{
	obs_data_t obj = obs_data_create();
	obs_data_setdouble(obj, "x", val->x);
	obs_data_setdouble(obj, "y", val->y);
	obs_data_setdouble(obj, "z", val->z);
	obs_data_setdouble(obj, "w", val->w);
	obs_data_setobj(data, name, obj);
	obs_data_release(obj);
}

void obs_data_set_quat(obs_data_t data, const char *name,
		const struct quat *val)
{
	obs_data_t obj = obs_data_create();
	obs_data_setdouble(obj, "x", val->x);
	obs_data_setdouble(obj, "y", val->y);
	obs_data_setdouble(obj, "z", val->z);
	obs_data_setdouble(obj, "w", val->w);
	obs_data_setobj(data, name, obj);
	obs_data_release(obj);
}

void obs_data_set_default_vec2(obs_data_t data, const char *name,
		const struct vec2 *val)
{
	obs_data_t obj = obs_data_create();
	obs_data_setdouble(obj, "x", val->x);
	obs_data_setdouble(obj, "y", val->y);
	obs_data_set_default_obj(data, name, obj);
	obs_data_release(obj);
}

void obs_data_set_default_vec3(obs_data_t data, const char *name,
		const struct vec3 *val)
{
	obs_data_t obj = obs_data_create();
	obs_data_setdouble(obj, "x", val->x);
	obs_data_setdouble(obj, "y", val->y);
	obs_data_setdouble(obj, "z", val->z);
	obs_data_set_default_obj(data, name, obj);
	obs_data_release(obj);
}

void obs_data_set_default_vec4(obs_data_t data, const char *name,
		const struct vec4 *val)
{
	obs_data_t obj = obs_data_create();
	obs_data_setdouble(obj, "x", val->x);
	obs_data_setdouble(obj, "y", val->y);
	obs_data_setdouble(obj, "z", val->z);
	obs_data_setdouble(obj, "w", val->w);
	obs_data_set_default_obj(data, name, obj);
	obs_data_release(obj);
}

void obs_data_set_default_quat(obs_data_t data, const char *name,
		const struct quat *val)
{
	obs_data_t obj = obs_data_create();
	obs_data_setdouble(obj, "x", val->x);
	obs_data_setdouble(obj, "y", val->y);
	obs_data_setdouble(obj, "z", val->z);
	obs_data_setdouble(obj, "w", val->w);
	obs_data_set_default_obj(data, name, obj);
	obs_data_release(obj);
}

void obs_data_get_vec2(obs_data_t data, const char *name, struct vec2 *val)
{
	obs_data_t obj = obs_data_getobj(data, name);
	if (!obj) return;

	val->x = (float)obs_data_getdouble(obj, "x");
	val->y = (float)obs_data_getdouble(obj, "y");
	obs_data_release(obj);
}

void obs_data_get_vec3(obs_data_t data, const char *name, struct vec3 *val)
{
	obs_data_t obj = obs_data_getobj(data, name);
	if (!obj) return;

	val->x = (float)obs_data_getdouble(obj, "x");
	val->y = (float)obs_data_getdouble(obj, "y");
	val->z = (float)obs_data_getdouble(obj, "z");
	obs_data_release(obj);
}

void obs_data_get_vec4(obs_data_t data, const char *name, struct vec4 *val)
{
	obs_data_t obj = obs_data_getobj(data, name);
	if (!obj) return;

	val->x = (float)obs_data_getdouble(obj, "x");
	val->y = (float)obs_data_getdouble(obj, "y");
	val->z = (float)obs_data_getdouble(obj, "z");
	val->w = (float)obs_data_getdouble(obj, "w");
	obs_data_release(obj);
}

void obs_data_get_quat(obs_data_t data, const char *name, struct quat *val)
{
	obs_data_t obj = obs_data_getobj(data, name);
	if (!obj) return;

	val->x = (float)obs_data_getdouble(obj, "x");
	val->y = (float)obs_data_getdouble(obj, "y");
	val->z = (float)obs_data_getdouble(obj, "z");
	val->w = (float)obs_data_getdouble(obj, "w");
	obs_data_release(obj);
}
