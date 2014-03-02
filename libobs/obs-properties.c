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
#include "obs-properties.h"

static inline void *get_property_data(struct obs_property *prop);

static inline void free_str_list(char **str_list)
{
	char **temp_list = str_list;
	while (*temp_list)
		bfree(*(temp_list++));
	bfree(str_list);
}

/* ------------------------------------------------------------------------- */

struct float_data {
	double min, max, step;
};

struct int_data {
	int min, max, step;
};

struct list_data {
	char                   **names;
	char                   **values;
	enum obs_combo_type    type;
	enum obs_combo_format  format;
};

static inline void list_data_free(struct list_data *data)
{
	free_str_list(data->names);
	free_str_list(data->values);
}

struct obs_property {
	const char             *name;
	const char             *desc;
	enum obs_property_type type;

	struct obs_property    *next;
};

struct obs_category {
	const char             *name;
	struct obs_property    *first_property;

	struct obs_category    *next;
};

struct obs_properties {
	struct obs_category    *first_category;
};

obs_properties_t obs_properties_create()
{
	struct obs_properties *list;
	list = bzalloc(sizeof(struct obs_properties));
	return list;
}

static void obs_property_destroy(struct obs_property *property)
{
	if (property->type == OBS_PROPERTY_LIST) {
		struct list_data *data = get_property_data(property);
		list_data_free(data);
	}

	bfree(property);
}

static void obs_category_destroy(struct obs_category *category)
{
	struct obs_property *p = category->first_property;
	while (p) {
		struct obs_property *next = p->next;
		obs_property_destroy(p);
		p = next;
	}

	bfree(category);
}

void obs_properties_destroy(obs_properties_t props)
{
	if (props) {
		struct obs_category *c = props->first_category;
		while (c) {
			struct obs_category *next = c->next;
			obs_category_destroy(c);
			c = next;
		}

		bfree(props);
	}
}

obs_category_t obs_properties_add_category(obs_properties_t props,
		const char *name)
{
	if (!props) return NULL;

	struct obs_category *c = bzalloc(sizeof(struct obs_category));
	c->name = name;
	c->next = props->first_category;
	props->first_category = c;

	return c;
}

obs_category_t obs_properties_first_category(obs_properties_t props)
{
	return (props != NULL) ? props->first_category : NULL;
}

/* ------------------------------------------------------------------------- */

static inline void category_add(struct obs_category *cat,
		struct obs_property *p)
{
	p->next = cat->first_property;
	cat->first_property = p;
}

static inline size_t get_property_size(enum obs_property_type type)
{
	switch (type) {
	case OBS_PROPERTY_INVALID:   return 0;
	case OBS_PROPERTY_INT:       return sizeof(struct int_data);
	case OBS_PROPERTY_FLOAT:     return sizeof(struct float_data);
	case OBS_PROPERTY_TEXT:      return 0;
	case OBS_PROPERTY_PATH:      return 0;
	case OBS_PROPERTY_LIST:     return sizeof(struct list_data);
	case OBS_PROPERTY_COLOR:     return 0;
	}

	return 0;
}

static inline struct obs_property *new_prop(struct obs_category *cat,
		const char *name, const char *desc,
		enum obs_property_type type)
{
	size_t data_size = get_property_size(type);
	struct obs_property *p;

	p = bzalloc(sizeof(struct obs_property) + data_size);
	p->type = type;
	p->name = name;
	p->desc = desc;
	category_add(cat, p);

	return p;
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

void obs_category_add_int(obs_category_t cat, const char *name,
		const char *desc, int min, int max, int step)
{
	if (!cat) return;

	struct obs_property *p = new_prop(cat, name, desc, OBS_PROPERTY_INT);
	struct int_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
}

void obs_category_add_float(obs_category_t cat, const char *name,
		const char *desc, double min, double max, double step)
{
	if (!cat) return;

	struct obs_property *p = new_prop(cat, name, desc, OBS_PROPERTY_FLOAT);
	struct float_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
}

void obs_category_add_text(obs_category_t cat, const char *name,
		const char *desc)
{
	if (!cat) return;
	new_prop(cat, name, desc, OBS_PROPERTY_TEXT);
}

void obs_category_add_path(obs_category_t cat, const char *name,
		const char *desc)
{
	if (!cat) return;
	new_prop(cat, name, desc, OBS_PROPERTY_PATH);
}

static char **dup_str_list(const char **str_list)
{
	int count = 0;
	char **new_list;
	const char **temp_list = str_list;

	if (!str_list)
		return NULL;

	while (*(temp_list++) != NULL)
		count++;

	new_list  = bmalloc((count+1) * sizeof(char*));
	new_list[count] = NULL;
	for (int i = 0; i < count; i++)
		new_list[i] = bstrdup(str_list[i]);

	return new_list;
}

void obs_category_add_list(obs_category_t cat,
		const char *name, const char *desc,
		const char **value_names, const char **values,
		enum obs_dropdown_type type,
		enum obs_dropdown_format format)
{
	if (!cat) return;

	if (type   == OBS_COMBO_TYPE_EDITABLE &&
	    format != OBS_COMBO_FORMAT_STRING) {
		blog(LOG_WARNING, "Catagory '%s', list '%s', error: "
		                  "Editable combo boxes must be strings",
		                  cat->name, name);
		return;
	}

	struct obs_property *p = new_prop(cat, name, desc, OBS_PROPERTY_LIST);
	struct list_data *data = get_property_data(p);
	data->names  = dup_str_list(value_names);
	data->values = dup_str_list(values);
	data->format = format;
	data->type   = type;
}

void obs_category_add_color(obs_category_t cat, const char *name,
		const char *desc)
{
	if (!cat) return;
	new_prop(cat, name, desc, OBS_PROPERTY_COLOR);
}

bool obs_category_next(obs_category_t *cat)
{
	if (!cat || !*cat)
		return false;

	*cat = (*cat)->next;
	return *cat != NULL;
}

obs_property_t obs_category_first_property(obs_category_t cat)
{
	if (!cat)
		return NULL;

	return cat->first_property;
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

static inline bool is_combo(struct obs_property *p)
{
	return p->type == OBS_PROPERTY_LIST;
}

const char **obs_property_list_names(obs_property_t p)
{
	if (!p || !is_combo(p))
		return NULL;

	struct list_data *data = get_property_data(p);
	return data->names;
}

const char **obs_property_list_values(obs_property_t p)
{
	if (!p || !is_combo(p))
		return NULL;

	struct list_data *data = get_property_data(p);
	return data->values;
}

enum obs_combo_type obs_property_list_type(obs_property_t p)
{
	if (!p || !is_combo(p))
		return OBS_COMBO_TYPE_INVALID;

	struct list_data *data = get_property_data(p);
	return data->type;
}

enum obs_combo_type obs_property_list_format(obs_property_t p)
{
	if (!p || !is_combo(p))
		return OBS_COMBO_FORMAT_INVALID;

	struct list_data *data = get_property_data(p);
	return data->format;
}
