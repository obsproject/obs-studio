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
	enum obs_number_type type;
};

struct int_data {
	int min, max, step;
	enum obs_number_type type;
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

struct editable_list_data {
	enum obs_editable_list_type type;
	char                        *filter;
	char                        *default_path;
};

struct button_data {
	obs_property_clicked_t callback;
};

struct frame_rate_option {
	char *name;
	char *description;
};

struct frame_rate_range {
	struct media_frames_per_second min_time;
	struct media_frames_per_second max_time;
};

struct frame_rate_data {
	DARRAY(struct frame_rate_option) extra_options;
	DARRAY(struct frame_rate_range)  ranges;
};

static inline void path_data_free(struct path_data *data)
{
	bfree(data->default_path);
	if (data->type == OBS_PATH_FILE)
		bfree(data->filter);
}

static inline void editable_list_data_free(struct editable_list_data *data)
{
	bfree(data->default_path);
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

static inline void frame_rate_data_options_free(struct frame_rate_data *data)
{
	for (size_t i = 0; i < data->extra_options.num; i++) {
		struct frame_rate_option *opt = &data->extra_options.array[i];
		bfree(opt->name);
		bfree(opt->description);
	}

	da_resize(data->extra_options, 0);
}

static inline void frame_rate_data_ranges_free(struct frame_rate_data *data)
{
	da_resize(data->ranges, 0);
}

static inline void frame_rate_data_free(struct frame_rate_data *data)
{
	frame_rate_data_options_free(data);
	frame_rate_data_ranges_free(data);

	da_free(data->extra_options);
	da_free(data->ranges);
}

struct obs_properties;

struct obs_property {
	char                    *name;
	char                    *desc;
	char                    *long_desc;
	void                    *priv;
	enum obs_property_type  type;
	bool                    visible;
	bool                    enabled;

	struct obs_properties   *parent;

	obs_property_modified_t modified;
	obs_property_modified2_t modified2;

	struct obs_property     *next;
};

struct obs_properties {
	void                    *param;
	void                    (*destroy)(void *param);
	uint32_t                flags;

	struct obs_property     *first_property;
	struct obs_property     **last;
};

obs_properties_t *obs_properties_create(void)
{
	struct obs_properties *props;
	props = bzalloc(sizeof(struct obs_properties));
	props->last = &props->first_property;
	return props;
}

void obs_properties_set_param(obs_properties_t *props,
		void *param, void (*destroy)(void *param))
{
	if (!props)
		return;

	if (props->param && props->destroy)
		props->destroy(props->param);

	props->param   = param;
	props->destroy = destroy;
}

void obs_properties_set_flags(obs_properties_t *props, uint32_t flags)
{
	if (props)
		props->flags = flags;
}

uint32_t obs_properties_get_flags(obs_properties_t *props)
{
	return props ? props->flags : 0;
}

void *obs_properties_get_param(obs_properties_t *props)
{
	return props ? props->param : NULL;
}

obs_properties_t *obs_properties_create_param(void *param,
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
	else if (property->type == OBS_PROPERTY_EDITABLE_LIST)
		editable_list_data_free(get_property_data(property));
	else if (property->type == OBS_PROPERTY_FRAME_RATE)
		frame_rate_data_free(get_property_data(property));

	bfree(property->name);
	bfree(property->desc);
	bfree(property->long_desc);
	bfree(property);
}

void obs_properties_destroy(obs_properties_t *props)
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

obs_property_t *obs_properties_first(obs_properties_t *props)
{
	return (props != NULL) ? props->first_property : NULL;
}

obs_property_t *obs_properties_get(obs_properties_t *props, const char *name)
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

void obs_properties_apply_settings(obs_properties_t *props, obs_data_t *settings)
{
	struct obs_property *p;

	if (!props)
		return;
	
	p = props->first_property;
	while (p) {
		if (p->modified)
			p->modified(props, p, settings);
		else if (p->modified2)
			p->modified2(p->priv, props, p, settings);
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
	case OBS_PROPERTY_EDITABLE_LIST:
		return sizeof(struct editable_list_data);
	case OBS_PROPERTY_FRAME_RATE:return sizeof(struct frame_rate_data);
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
	p->name    = bstrdup(name);
	p->desc    = bstrdup(desc);
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

obs_property_t *obs_properties_add_bool(obs_properties_t *props,
		const char *name, const char *desc)
{
	if (!props || has_prop(props, name)) return NULL;
	return new_prop(props, name, desc, OBS_PROPERTY_BOOL);
}

static obs_property_t *add_int(obs_properties_t *props,
		const char *name, const char *desc, int min, int max, int step,
		enum obs_number_type type)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, desc, OBS_PROPERTY_INT);
	struct int_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
	data->type = type;
	return p;
}

static obs_property_t *add_flt(obs_properties_t *props,
		const char *name, const char *desc,
		double min, double max, double step,
		enum obs_number_type type)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, desc,
			OBS_PROPERTY_FLOAT);
	struct float_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
	data->type = type;
	return p;
}

obs_property_t *obs_properties_add_int(obs_properties_t *props,
		const char *name, const char *desc, int min, int max, int step)
{
	return add_int(props, name, desc, min, max, step, OBS_NUMBER_SCROLLER);
}

obs_property_t *obs_properties_add_float(obs_properties_t *props,
		const char *name, const char *desc,
		double min, double max, double step)
{
	return add_flt(props, name, desc, min, max, step, OBS_NUMBER_SCROLLER);
}

obs_property_t *obs_properties_add_int_slider(obs_properties_t *props,
		const char *name, const char *desc, int min, int max, int step)
{
	return add_int(props, name, desc, min, max, step, OBS_NUMBER_SLIDER);
}

obs_property_t *obs_properties_add_float_slider(obs_properties_t *props,
		const char *name, const char *desc,
		double min, double max, double step)
{
	return add_flt(props, name, desc, min, max, step, OBS_NUMBER_SLIDER);
}

obs_property_t *obs_properties_add_text(obs_properties_t *props,
		const char *name, const char *desc, enum obs_text_type type)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, desc, OBS_PROPERTY_TEXT);
	struct text_data *data = get_property_data(p);
	data->type = type;
	return p;
}

obs_property_t *obs_properties_add_path(obs_properties_t *props,
		const char *name, const char *desc, enum obs_path_type type,
		const char *filter, const char *default_path)
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

obs_property_t *obs_properties_add_list(obs_properties_t *props,
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

obs_property_t *obs_properties_add_color(obs_properties_t *props,
		const char *name, const char *desc)
{
	if (!props || has_prop(props, name)) return NULL;
	return new_prop(props, name, desc, OBS_PROPERTY_COLOR);
}

obs_property_t *obs_properties_add_button(obs_properties_t *props,
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

obs_property_t *obs_properties_add_button2(obs_properties_t *props,
		const char *name, const char *text,
		obs_property_clicked_t callback, void *priv)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, text,
			OBS_PROPERTY_BUTTON);
	struct button_data *data = get_property_data(p);
	data->callback = callback;
	p->priv = priv;
	return p;
}

obs_property_t *obs_properties_add_font(obs_properties_t *props,
		const char *name, const char *desc)
{
	if (!props || has_prop(props, name)) return NULL;
	return new_prop(props, name, desc, OBS_PROPERTY_FONT);
}

obs_property_t *obs_properties_add_editable_list(obs_properties_t *props,
		const char *name, const char *desc,
		enum obs_editable_list_type type, const char *filter,
		const char *default_path)
{
	if (!props || has_prop(props, name)) return NULL;
	struct obs_property *p = new_prop(props, name, desc,
			OBS_PROPERTY_EDITABLE_LIST);

	struct editable_list_data *data = get_property_data(p);
	data->type = type;
	data->filter = bstrdup(filter);
	data->default_path = bstrdup(default_path);
	return p;
}

obs_property_t *obs_properties_add_frame_rate(obs_properties_t *props,
		const char *name, const char *desc)
{
	if (!props || has_prop(props, name)) return NULL;

	struct obs_property *p = new_prop(props, name, desc,
			OBS_PROPERTY_FRAME_RATE);

	struct frame_rate_data *data = get_property_data(p);
	da_init(data->extra_options);
	da_init(data->ranges);
	return p;
}

/* ------------------------------------------------------------------------- */

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
	return (data && data->format == format) ? data : NULL;
}

/* ------------------------------------------------------------------------- */

bool obs_property_next(obs_property_t **p)
{
	if (!p || !*p)
		return false;

	*p = (*p)->next;
	return *p != NULL;
}

void obs_property_set_modified_callback(obs_property_t *p,
		obs_property_modified_t modified)
{
	if (p) p->modified = modified;
}

void obs_property_set_modified_callback2(obs_property_t *p,
		obs_property_modified2_t modified2, void *priv)
{
	if (p) {
		p->modified2 = modified2;
		p->priv = priv;
	}
}

bool obs_property_modified(obs_property_t *p, obs_data_t *settings)
{
	if (p) {
		if (p->modified) {
			return p->modified(p->parent, p, settings);
		} else if (p->modified2) {
			return p->modified2(p->priv, p->parent, p, settings);
		}
	}
	return false;
}

bool obs_property_button_clicked(obs_property_t *p, void *obj)
{
	struct obs_context_data *context = obj;
	if (p) {
		struct button_data *data = get_type_data(p,
				OBS_PROPERTY_BUTTON);
		if (data && data->callback) {
			if (p->priv)
				return data->callback(p->parent, p, p->priv);
			return data->callback(p->parent, p,
					(context ? context->data : NULL));
		}
	}

	return false;
}

void obs_property_set_visible(obs_property_t *p, bool visible)
{
	if (p) p->visible = visible;
}

void obs_property_set_enabled(obs_property_t *p, bool enabled)
{
	if (p) p->enabled = enabled;
}

void obs_property_set_description(obs_property_t *p, const char *description)
{
	if (p) {
		bfree(p->desc);
		p->desc = description && *description
			? bstrdup(description)
			: NULL;
	}
}

void obs_property_set_long_description(obs_property_t *p, const char *long_desc)
{
	if (p) {
		bfree(p->long_desc);
		p->long_desc = long_desc && *long_desc
			? bstrdup(long_desc)
			: NULL;
	}
}

const char *obs_property_name(obs_property_t *p)
{
	return p ? p->name : NULL;
}

const char *obs_property_description(obs_property_t *p)
{
	return p ? p->desc : NULL;
}

const char *obs_property_long_description(obs_property_t *p)
{
	return p ? p->long_desc : NULL;
}

enum obs_property_type obs_property_get_type(obs_property_t *p)
{
	return p ? p->type : OBS_PROPERTY_INVALID;
}

bool obs_property_enabled(obs_property_t *p)
{
	return p ? p->enabled : false;
}

bool obs_property_visible(obs_property_t *p)
{
	return p ? p->visible : false;
}

int obs_property_int_min(obs_property_t *p)
{
	struct int_data *data = get_type_data(p, OBS_PROPERTY_INT);
	return data ? data->min : 0;
}

int obs_property_int_max(obs_property_t *p)
{
	struct int_data *data = get_type_data(p, OBS_PROPERTY_INT);
	return data ? data->max : 0;
}

int obs_property_int_step(obs_property_t *p)
{
	struct int_data *data = get_type_data(p, OBS_PROPERTY_INT);
	return data ? data->step : 0;
}

enum obs_number_type obs_property_int_type(obs_property_t *p)
{
	struct int_data *data = get_type_data(p, OBS_PROPERTY_INT);
	return data ? data->type : OBS_NUMBER_SCROLLER;
}

double obs_property_float_min(obs_property_t *p)
{
	struct float_data *data = get_type_data(p, OBS_PROPERTY_FLOAT);
	return data ? data->min : 0;
}

double obs_property_float_max(obs_property_t *p)
{
	struct float_data *data = get_type_data(p, OBS_PROPERTY_FLOAT);
	return data ? data->max : 0;
}

double obs_property_float_step(obs_property_t *p)
{
	struct float_data *data = get_type_data(p, OBS_PROPERTY_FLOAT);
	return data ? data->step : 0;
}

enum obs_number_type obs_property_float_type(obs_property_t *p)
{
	struct float_data *data = get_type_data(p, OBS_PROPERTY_FLOAT);
	return data ? data->type : OBS_NUMBER_SCROLLER;
}

enum obs_text_type obs_proprety_text_type(obs_property_t *p)
{
	struct text_data *data = get_type_data(p, OBS_PROPERTY_TEXT);
	return data ? data->type : OBS_TEXT_DEFAULT;
}

enum obs_path_type obs_property_path_type(obs_property_t *p)
{
	struct path_data *data = get_type_data(p, OBS_PROPERTY_PATH);
	return data ? data->type : OBS_PATH_DIRECTORY;
}

const char *obs_property_path_filter(obs_property_t *p)
{
	struct path_data *data = get_type_data(p, OBS_PROPERTY_PATH);
	return data ? data->filter : NULL;
}

const char *obs_property_path_default_path(obs_property_t *p)
{
	struct path_data *data = get_type_data(p, OBS_PROPERTY_PATH);
	return data ? data->default_path : NULL;
}

enum obs_combo_type obs_property_list_type(obs_property_t *p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->type : OBS_COMBO_TYPE_INVALID;
}

enum obs_combo_format obs_property_list_format(obs_property_t *p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->format : OBS_COMBO_FORMAT_INVALID;
}

void obs_property_int_set_limits(obs_property_t *p,
		int min, int max, int step)
{
	struct int_data *data = get_type_data(p, OBS_PROPERTY_INT);
	if (!data)
		return;

	data->min = min;
	data->max = max;
	data->step = step;
}

void obs_property_float_set_limits(obs_property_t *p,
		double min, double max, double step)
{
	struct float_data *data = get_type_data(p, OBS_PROPERTY_INT);
	if (!data)
		return;

	data->min = min;
	data->max = max;
	data->step = step;
}

void obs_property_list_clear(obs_property_t *p)
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

static void insert_item(struct list_data *data, size_t idx, const char *name,
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

	da_insert(data->items, idx, &item);
}

size_t obs_property_list_add_string(obs_property_t *p,
		const char *name, const char *val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_STRING)
		return add_item(data, name, val);
	return 0;
}

size_t obs_property_list_add_int(obs_property_t *p,
		const char *name, long long val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_INT)
		return add_item(data, name, &val);
	return 0;
}

size_t obs_property_list_add_float(obs_property_t *p,
		const char *name, double val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_FLOAT)
		return add_item(data, name, &val);
	return 0;
}

void obs_property_list_insert_string(obs_property_t *p, size_t idx,
		const char *name, const char *val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_STRING)
		insert_item(data, idx, name, val);
}

void obs_property_list_insert_int(obs_property_t *p, size_t idx,
		const char *name, long long val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_INT)
		insert_item(data, idx, name, &val);
}

void obs_property_list_insert_float(obs_property_t *p, size_t idx,
		const char *name, double val)
{
	struct list_data *data = get_list_data(p);
	if (data && data->format == OBS_COMBO_FORMAT_FLOAT)
		insert_item(data, idx, name, &val);
}

void obs_property_list_item_remove(obs_property_t *p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	if (data && idx < data->items.num) {
		list_item_free(data, data->items.array+idx);
		da_erase(data->items, idx);
	}
}

size_t obs_property_list_item_count(obs_property_t *p)
{
	struct list_data *data = get_list_data(p);
	return data ? data->items.num : 0;
}

bool obs_property_list_item_disabled(obs_property_t *p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	return (data && idx < data->items.num) ?
		data->items.array[idx].disabled : false;
}

void obs_property_list_item_disable(obs_property_t *p, size_t idx,
		bool disabled)
{
	struct list_data *data = get_list_data(p);
	if (!data || idx >= data->items.num)
		return;
	data->items.array[idx].disabled = disabled;
}

const char *obs_property_list_item_name(obs_property_t *p, size_t idx)
{
	struct list_data *data = get_list_data(p);
	return (data && idx < data->items.num) ?
		data->items.array[idx].name : NULL;
}

const char *obs_property_list_item_string(obs_property_t *p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OBS_COMBO_FORMAT_STRING);
	return (data && idx < data->items.num) ?
		data->items.array[idx].str : NULL;
}

long long obs_property_list_item_int(obs_property_t *p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OBS_COMBO_FORMAT_INT);
	return (data && idx < data->items.num) ?
		data->items.array[idx].ll : 0;
}

double obs_property_list_item_float(obs_property_t *p, size_t idx)
{
	struct list_data *data = get_list_fmt_data(p, OBS_COMBO_FORMAT_FLOAT);
	return (data && idx < data->items.num) ?
		data->items.array[idx].d : 0.0;
}

enum obs_editable_list_type obs_property_editable_list_type(obs_property_t *p)
{
	struct editable_list_data *data = get_type_data(p,
			OBS_PROPERTY_EDITABLE_LIST);
	return data ? data->type : OBS_EDITABLE_LIST_TYPE_STRINGS;
}

const char *obs_property_editable_list_filter(obs_property_t *p)
{
	struct editable_list_data *data = get_type_data(p,
			OBS_PROPERTY_EDITABLE_LIST);
	return data ? data->filter : NULL;
}

const char *obs_property_editable_list_default_path(obs_property_t *p)
{
	struct editable_list_data *data = get_type_data(p,
			OBS_PROPERTY_EDITABLE_LIST);
	return data ? data->default_path : NULL;
}

/* ------------------------------------------------------------------------- */
/* OBS_PROPERTY_FRAME_RATE */

void obs_property_frame_rate_clear(obs_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	if (!data) return;

	frame_rate_data_options_free(data);
	frame_rate_data_ranges_free(data);
}

void obs_property_frame_rate_options_clear(obs_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	if (!data) return;

	frame_rate_data_options_free(data);
}

void obs_property_frame_rate_fps_ranges_clear(obs_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	if (!data) return;

	frame_rate_data_ranges_free(data);
}

size_t obs_property_frame_rate_option_add(obs_property_t *p,
		const char *name, const char *description)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	if (!data) return DARRAY_INVALID;

	struct frame_rate_option *opt = da_push_back_new(data->extra_options);

	opt->name        = bstrdup(name);
	opt->description = bstrdup(description);

	return data->extra_options.num - 1;
}

size_t obs_property_frame_rate_fps_range_add(obs_property_t *p,
		struct media_frames_per_second min,
		struct media_frames_per_second max)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	if (!data) return DARRAY_INVALID;

	struct frame_rate_range *rng = da_push_back_new(data->ranges);

	rng->min_time = min;
	rng->max_time = max;

	return data->ranges.num - 1;
}

void obs_property_frame_rate_option_insert(obs_property_t *p, size_t idx,
		const char *name, const char *description)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	if (!data) return;

	struct frame_rate_option *opt = da_insert_new(data->extra_options, idx);

	opt->name        = bstrdup(name);
	opt->description = bstrdup(description);
}

void obs_property_frame_rate_fps_range_insert(obs_property_t *p, size_t idx,
		struct media_frames_per_second min,
		struct media_frames_per_second max)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	if (!data) return;

	struct frame_rate_range *rng = da_insert_new(data->ranges, idx);

	rng->min_time = min;
	rng->max_time = max;
}

size_t obs_property_frame_rate_options_count(obs_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	return data ? data->extra_options.num : 0;
}

const char *obs_property_frame_rate_option_name(obs_property_t *p,
		size_t idx)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	return data && data->extra_options.num > idx ?
		data->extra_options.array[idx].name : NULL;
}

const char *obs_property_frame_rate_option_description(
		obs_property_t *p, size_t idx)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	return data && data->extra_options.num > idx ?
		data->extra_options.array[idx].description : NULL;
}

size_t obs_property_frame_rate_fps_ranges_count(obs_property_t *p)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	return data ? data->ranges.num : 0;
}

struct media_frames_per_second obs_property_frame_rate_fps_range_min(
		obs_property_t *p, size_t idx)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	return data && data->ranges.num > idx ?
		data->ranges.array[idx].min_time :
		(struct media_frames_per_second){0};
}
struct media_frames_per_second obs_property_frame_rate_fps_range_max(
		obs_property_t *p, size_t idx)
{
	struct frame_rate_data *data =
		get_type_data(p, OBS_PROPERTY_FRAME_RATE);
	return data && data->ranges.num > idx ?
		data->ranges.array[idx].max_time :
		(struct media_frames_per_second){0};
}
