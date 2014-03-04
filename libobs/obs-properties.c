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
#include "util/darray.h"
#include "obs-properties.h"

static inline void *get_property_data(struct obs_property *prop);

/* ------------------------------------------------------------------------- */

struct float_data {
	double min, max, step;
};

struct int_data {
	int min, max, step;
};

struct list_item {
	char *name;
	char *value;
};

struct list_data {
	DARRAY(struct list_item) items;
	enum obs_combo_type      type;
	enum obs_combo_format    format;
};

static inline void list_data_free(struct list_data *data)
{
	for (size_t i = 0; i < data->items.num; i++) {
		bfree(data->items.array[i].name);
		bfree(data->items.array[i].value);
	}

	da_free(data->items);
}

struct obs_property {
	const char             *name;
	const char             *desc;
	enum obs_property_type type;

	struct obs_property    *next;
};

struct obs_properties {
	struct obs_property    *first_property;
	struct obs_property    **last;
};

obs_properties_t obs_properties_create()
{
	struct obs_properties *props;
	props = bzalloc(sizeof(struct obs_properties));
	props->last = &props->first_property;
	return props;
}

static void obs_property_destroy(struct obs_property *property)
{
	if (property->type == OBS_PROPERTY_LIST) {
		struct list_data *data = get_property_data(property);
		list_data_free(data);
	}

	bfree(property);
}

void obs_properties_destroy(obs_properties_t props)
{
	if (props) {
		struct obs_property *p = props->first_property;
		while (p) {
			struct obs_property *next = p->next;
			obs_property_destroy(p);
			p = next;
		}

		bfree(props);
	}
}

obs_property_t obs_properties_first(obs_properties_t props)
{
	return (props != NULL) ? props->first_property : NULL;
}

obs_property_t obs_properties_get(obs_properties_t props, const char *name)
{
	struct obs_property *property;

	if (!props)
		return NULL;

	property = props->first_property;
	while (property) {
		if (strcmp(property->name, name) == 0)
			return property;

		property = property->next;
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

static inline void propertes_add(struct obs_properties *props,
		struct obs_property *p)
{
	*props->last = p;
	props->last = &p->next;
}

static inline size_t get_property_size(enum obs_property_type type)
{
	switch (type) {
	case OBS_PROPERTY_INVALID:   return 0;
	case OBS_PROPERTY_INT:       return sizeof(struct int_data);
	case OBS_PROPERTY_FLOAT:     return sizeof(struct float_data);
	case OBS_PROPERTY_TEXT:      return 0;
	case OBS_PROPERTY_PATH:      return 0;
	case OBS_PROPERTY_LIST:      return sizeof(struct list_data);
	case OBS_PROPERTY_COLOR:     return 0;
	}

	return 0;
}

static inline struct obs_property *new_prop(struct obs_properties *props,
		const char *name, const char *desc,
		enum obs_property_type type)
{
	size_t data_size = get_property_size(type);
	struct obs_property *p;

	p = bzalloc(sizeof(struct obs_property) + data_size);
	p->type = type;
	p->name = name;
	p->desc = desc;
	propertes_add(props, p);

	return p;
}

static inline bool has_prop(struct obs_properties *props, const char *name)
{
	struct obs_property *p = props->first_property;

	while (p) {
		if (strcmp(p->name, name) == 0) {
			blog(LOG_WARNING, "Property '%s' exists", name);
			return true;
		}

		p = p->next;
	}

	return false;
}

static inline void *get_property_data(struct obs_property *prop)
{
	return (uint8_t*)prop + sizeof(struct obs_property);
}

static inline void *get_type_data(struct obs_property *prop,
		enum obs_property_type type)
{
	if (!prop || prop->type != type)
		return NULL;

	return get_property_data(prop);
}

void obs_properties_add_bool(obs_properties_t props, const char *name,
		const char *desc)
{
	if (!props || has_prop(props, name)) return;
	new_prop(props, name, desc, OBS_PROPERTY_BOOL);
}

void obs_properties_add_int(obs_properties_t props, const char *name,
		const char *desc, int min, int max, int step)
{
	if (!props || has_prop(props, name)) return;

	struct obs_property *p = new_prop(props, name, desc, OBS_PROPERTY_INT);
	struct int_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
}

void obs_properties_add_float(obs_properties_t props, const char *name,
		const char *desc, double min, double max, double step)
{
	if (!props || has_prop(props, name)) return;

	struct obs_property *p = new_prop(props, name, desc,
			OBS_PROPERTY_FLOAT);
	struct float_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
}

void obs_properties_add_text(obs_properties_t props, const char *name,
		const char *desc)
{
	if (!props || has_prop(props, name)) return;
	new_prop(props, name, desc, OBS_PROPERTY_TEXT);
}

void obs_properties_add_path(obs_properties_t props, const char *name,
		const char *desc)
{
	if (!props || has_prop(props, name)) return;
	new_prop(props, name, desc, OBS_PROPERTY_PATH);
}

obs_property_t obs_properties_add_list(obs_properties_t props,
		const char *name, const char *desc,
		enum obs_combo_type type,
		enum obs_combo_format format)
{
	if (!props || has_prop(props, name)) return NULL;

	if (type   == OBS_COMBO_TYPE_EDITABLE &&
	    format != OBS_COMBO_FORMAT_STRING) {
		blog(LOG_WARNING, "List '%s', error: Editable combo boxes "
		                  "must be of the 'string' type", name);
		return NULL;
	}

	struct obs_property *p = new_prop(props, name, desc, OBS_PROPERTY_LIST);
	struct list_data *data = get_property_data(p);
	data->format = format;
	data->type   = type;

	return p;
}

void obs_properties_add_color(obs_properties_t props, const char *name,
		const char *desc)
{
	if (!props || has_prop(props, name)) return;
	new_prop(props, name, desc, OBS_PROPERTY_COLOR);
}


static inline bool is_combo(struct obs_property *p)
{
	return p->type == OBS_PROPERTY_LIST;
}

static inline struct list_data *get_list_data(struct obs_property *p)
{
	if (!p || !is_combo(p))
		return NULL;

	return get_property_data(p);
}

void obs_property_list_add_item(obs_property_t p,
		const char *name, const char *value)
{
	struct list_data *data = get_list_data(p);
	if (data) {
		struct list_item item = {
			.name  = bstrdup(name),
			.value = bstrdup(value)
		};

		da_insert(data->items, data->items.num-1, &item);
	}
}

/* ------------------------------------------------------------------------- */

bool obs_property_next(obs_property_t *p)
{
	if (!p || !*p)
		return false;

	*p = (*p)->next;
	return *p != NULL;
}

const char *obs_property_name(obs_property_t p)
{
	return p ? p->name : NULL;
}

const char *obs_property_description(obs_property_t p)
{
	return p ? p->desc : NULL;
}

enum obs_property_type obs_property_type(obs_property_t p)
{
	return p ? p->type : OBS_PROPERTY_INVALID;
}

int obs_property_int_min(obs_property_t p)
{
	struct int_data *data = get_type_data(p, OBS_PROPERTY_INT);
	return data ? data->min : 0;
}

int obs_property_int_max(obs_property_t p)
{
	struct int_data *data = get_type_data(p, OBS_PROPERTY_INT);
	return data ? data->max : 0;
}

int obs_property_int_step(obs_property_t p)
{
	struct int_data *data = get_type_data(p, OBS_PROPERTY_INT);
	return data ? data->step : 0;
}

double obs_property_float_min(obs_property_t p)
{
	struct float_data *data = get_type_data(p, OBS_PROPERTY_FLOAT);
	return data ? data->min : 0;
}

double obs_property_float_max(obs_property_t p)
{
	struct float_data *data = get_type_data(p, OBS_PROPERTY_FLOAT);
	return data ? data->max : 0;
}

double obs_property_float_step(obs_property_t p)
{
	struct float_data *data = get_type_data(p, OBS_PROPERTY_FLOAT);
	return data ? data->step : 0;
}

enum obs_combo_type obs_property_list_type(obs_property_t p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->type : OBS_COMBO_TYPE_INVALID;
}

enum obs_combo_format obs_property_list_format(obs_property_t p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->format : OBS_COMBO_FORMAT_INVALID;
}

size_t obs_property_list_item_count(obs_property_t p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->items.num : 0;
}

const char *obs_property_list_item_name(obs_property_t p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	return (data && idx < data->items.num) ?
		data->items.array[idx].name : NULL;
}

const char *obs_property_list_item_value(obs_property_t p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	return (data && idx < data->items.num) ?
		data->items.array[idx].value : NULL;
}
