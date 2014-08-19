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
#include "obs-internal.h"
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
	bool disabled;

	union {
		char      *str;
		long long ll;
		double    d;
	};
};

struct path_data {
	char               *filter;
	char               *default_path;
	enum obs_path_type type;
};

struct text_data {
	enum obs_text_type type;
};

struct list_data {
	DARRAY(struct list_item) items;
	enum obs_combo_type      type;
	enum obs_combo_format    format;
};

struct button_data {
	obs_property_clicked_t callback;
};

static inline void path_data_free(struct path_data *data)
{
	bfree(data->default_path);
	if (data->type == OBS_PATH_FILE)
		bfree(data->filter);
}

static inline void list_item_free(struct list_data *data,
		struct list_item *item)
{
	bfree(item->name);
	if (data->format == OBS_COMBO_FORMAT_STRING)
		bfree(item->str);
}

static inline void list_data_free(struct list_data *data)
{
	for (size_t i = 0; i < data->items.num; i++)
		list_item_free(data, data->items.array+i);

	da_free(data->items);
}

struct obs_properties;

struct obs_property {
	const char              *name;
	const char              *desc;
	enum obs_property_type  type;
	bool                    visible;
	bool                    enabled;

	struct obs_properties   *parent;

	obs_property_modified_t modified;

	struct obs_property     *next;
};

struct obs_properties {
	void                    *param;
	void                    (*destroy)(void *param);

	struct obs_property     *first_property;
	struct obs_property     **last;
};

obs_properties_t obs_properties_create(void)
{
	struct obs_properties *props;
	props = bzalloc(sizeof(struct obs_properties));
	props->last = &props->first_property;
	return props;
}

void obs_properties_set_param(obs_properties_t props,
		void *param, void (*destroy)(void *param))
{
	if (!props)
		return;

	if (props->param && props->destroy)
		props->destroy(props->param);

	props->param   = param;
	props->destroy = destroy;
}

void *obs_properties_get_param(obs_properties_t props)
{
	return props ? props->param : NULL;
}

obs_properties_t obs_properties_create_param(void *param,
		void (*destroy)(void *param))
{
	struct obs_properties *props = obs_properties_create();
	obs_properties_set_param(props, param, destroy);
	return props;
}

static void obs_property_destroy(struct obs_property *property)
{
	if (property->type == OBS_PROPERTY_LIST)
		list_data_free(get_property_data(property));
	else if (property->type == OBS_PROPERTY_PATH)
		path_data_free(get_property_data(property));

	bfree(property);
}

void obs_properties_destroy(obs_properties_t props)
{
	if (props) {
		struct obs_property *p = props->first_property;

		if (props->destroy && props->param)
			props->destroy(props->param);

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

void obs_properties_apply_settings(obs_properties_t props, obs_data_t settings)
{
	struct obs_property *p = props->first_property;

	while (p) {
		if (p->modified)
			p->modified(props, p, settings);
		p = p->next;
	}
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
	case OBS_PROPERTY_BOOL:      return 0;
	case OBS_PROPERTY_INT:       return sizeof(struct int_data);
	case OBS_PROPERTY_FLOAT:     return sizeof(struct float_data);
	case OBS_PROPERTY_TEXT:      return sizeof(struct text_data);
	case OBS_PROPERTY_PATH:      return sizeof(struct path_data);
	case OBS_PROPERTY_LIST:      return sizeof(struct list_data);
	case OBS_PROPERTY_COLOR:     return 0;
	case OBS_PROPERTY_BUTTON:    return sizeof(struct button_data);
	case OBS_PROPERTY_FONT:      return 0;
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
	p->parent  = props;
	p->enabled = true;
	p->visible = true;
	p->type    = type;
	p->name    = name;
	p->desc    = desc;
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

obs_property_t obs_properties_add_bool(obs_properties_t props, const char *name,
		const char *desc)
{
	if (!props || has_prop(props, name)) return NULL;
	return new_prop(props, name, desc, OBS_PROPERTY_BOOL);
}

obs_property_t obs_properties_add_int(obs_properties_t props, const char *name,
		const char *desc, int min, int max, int step)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, desc, OBS_PROPERTY_INT);
	struct int_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
	return p;
}

obs_property_t obs_properties_add_float(obs_properties_t props,
		const char *name, const char *desc,
		double min, double max, double step)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, desc,
			OBS_PROPERTY_FLOAT);
	struct float_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
	return p;
}

obs_property_t obs_properties_add_text(obs_properties_t props, const char *name,
		const char *desc, enum obs_text_type type)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, desc, OBS_PROPERTY_TEXT);
	struct text_data *data = get_property_data(p);
	data->type = type;
	return p;
}

obs_property_t obs_properties_add_path(obs_properties_t props, const char *name,
		const char *desc, enum obs_path_type type, const char *filter,
		const char *default_path)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, desc, OBS_PROPERTY_PATH);
	struct path_data *data = get_property_data(p);
	data->type         = type;
	data->default_path = bstrdup(default_path);

	if (data->type == OBS_PATH_FILE)
		data->filter = bstrdup(filter);

	return p;
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

obs_property_t obs_properties_add_color(obs_properties_t props,
		const char *name, const char *desc)
{
	if (!props || has_prop(props, name)) return NULL;
	return new_prop(props, name, desc, OBS_PROPERTY_COLOR);
}

obs_property_t obs_properties_add_button(obs_properties_t props,
		const char *name, const char *text,
		obs_property_clicked_t callback)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, text,
			OBS_PROPERTY_BUTTON);
	struct button_data *data = get_property_data(p);
	data->callback = callback;
	return p;
}

obs_property_t obs_properties_add_font(obs_properties_t props,
		const char *name, const char *desc)
{
	if (!props || has_prop(props, name)) return NULL;
	return new_prop(props, name, desc, OBS_PROPERTY_FONT);
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

static inline struct list_data *get_list_fmt_data(struct obs_property *p,
		enum obs_combo_format format)
{
	struct list_data *data = get_list_data(p);
	return (data->format == format) ? data : NULL;
}

/* ------------------------------------------------------------------------- */

bool obs_property_next(obs_property_t *p)
{
	if (!p || !*p)
		return false;

	*p = (*p)->next;
	return *p != NULL;
}

void obs_property_set_modified_callback(obs_property_t p,
		obs_property_modified_t modified)
{
	if (p) p->modified = modified;
}

bool obs_property_modified(obs_property_t p, obs_data_t settings)
{
	if (p && p->modified)
		return p->modified(p->parent, p, settings);
	return false;
}

bool obs_property_button_clicked(obs_property_t p, void *obj)
{
	struct obs_context_data *context = obj;
	if (p) {
		struct button_data *data = get_type_data(p,
				OBS_PROPERTY_BUTTON);
		if (data && data->callback)
			return data->callback(p->parent, p, context->data);
	}

	return false;
}

void obs_property_set_visible(obs_property_t p, bool visible)
{
	if (p) p->visible = visible;
}

void obs_property_set_enabled(obs_property_t p, bool enabled)
{
	if (p) p->enabled = enabled;
}

const char *obs_property_name(obs_property_t p)
{
	return p ? p->name : NULL;
}

const char *obs_property_description(obs_property_t p)
{
	return p ? p->desc : NULL;
}

enum obs_property_type obs_property_get_type(obs_property_t p)
{
	return p ? p->type : OBS_PROPERTY_INVALID;
}

bool obs_property_enabled(obs_property_t p)
{
	return p ? p->enabled : false;
}

bool obs_property_visible(obs_property_t p)
{
	return p ? p->visible : false;
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

enum obs_text_type obs_proprety_text_type(obs_property_t p)
{
	struct text_data *data = get_type_data(p, OBS_PROPERTY_TEXT);
	return data ? data->type : OBS_TEXT_DEFAULT;
}

enum obs_path_type obs_property_path_type(obs_property_t p)
{
	struct path_data *data = get_type_data(p, OBS_PROPERTY_PATH);
	return data ? data->type : OBS_PATH_DIRECTORY;
}

const char *obs_property_path_filter(obs_property_t p)
{
	struct path_data *data = get_type_data(p, OBS_PROPERTY_PATH);
	return data ? data->filter : NULL;
}

const char *obs_property_path_default_path(obs_property_t p)
{
	struct path_data *data = get_type_data(p, OBS_PROPERTY_PATH);
	return data ? data->default_path : NULL;
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

void obs_property_list_clear(obs_property_t p)
{
	struct list_data *data = get_list_data(p);
	if (data)
		list_data_free(data);
}

static size_t add_item(struct list_data *data, const char *name,
		const void *val)
{
	struct list_item item = { NULL };
	item.name  = bstrdup(name);

	if (data->format == OBS_COMBO_FORMAT_INT)
		item.ll  = *(const long long*)val;
	else if (data->format == OBS_COMBO_FORMAT_FLOAT)
		item.d   = *(const double*)val;
	else
		item.str = bstrdup(val);

	return da_push_back(data->items, &item);
}

size_t obs_property_list_add_string(obs_property_t p,
		const char *name, const char *val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_STRING)
		return add_item(data, name, val);
	return 0;
}

size_t obs_property_list_add_int(obs_property_t p,
		const char *name, long long val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_INT)
		return add_item(data, name, &val);
	return 0;
}

size_t obs_property_list_add_float(obs_property_t p,
		const char *name, double val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_FLOAT)
		return add_item(data, name, &val);
	return 0;
}

void obs_property_list_item_remove(obs_property_t p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	if (data && idx < data->items.num) {
		list_item_free(data, data->items.array+idx);
		da_erase(data->items, idx);
	}
}

size_t obs_property_list_item_count(obs_property_t p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->items.num : 0;
}

bool obs_property_list_item_disabled(obs_property_t p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	return (data && idx < data->items.num) ?
		data->items.array[idx].disabled : false;
}

void obs_property_list_item_disable(obs_property_t p, size_t idx, bool disabled)
{
	struct list_data *data = get_list_data(p);
	if (!data || idx >= data->items.num)
		return;
	data->items.array[idx].disabled = disabled;
}

const char *obs_property_list_item_name(obs_property_t p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	return (data && idx < data->items.num) ?
		data->items.array[idx].name : NULL;
}

const char *obs_property_list_item_string(obs_property_t p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OBS_COMBO_FORMAT_STRING);
	return (data && idx < data->items.num) ?
		data->items.array[idx].str : "";
}

long long obs_property_list_item_int(obs_property_t p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OBS_COMBO_FORMAT_INT);
	return (data && idx < data->items.num) ?
		data->items.array[idx].ll : 0;
}

double obs_property_list_item_float(obs_property_t p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OBS_COMBO_FORMAT_FLOAT);
	return (data && idx < data->items.num) ?
		data->items.array[idx].d : 0.0;
}
