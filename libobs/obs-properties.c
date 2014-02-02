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

#include "obs-properties.h"

struct float_data {
	double min, max, step;
};

struct int_data {
	int min, max, step;
};

struct list_data {
	const char             **strings;
	enum obs_dropdown_type type;
};

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

struct obs_property_list {
	struct obs_category    *first_category;
};

obs_property_list_t obs_property_list_create()
{
	struct obs_property_list *list;
	list = bmalloc(sizeof(struct obs_property_list));
	memset(list, 0, sizeof(struct obs_property_list));
	return list;
}

static void obs_property_destroy(struct obs_property *property)
{
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

void obs_property_list_destroy(obs_property_list_t props)
{
	struct obs_category *c = props->first_category;
	while (c) {
		struct obs_category *next = c->next;
		obs_category_destroy(c);
		c = next;
	}

	bfree(props);
}

obs_category_t obs_property_list_add_category(obs_property_list_t props,
		const char *name)
{
	struct obs_category *c = bmalloc(sizeof(struct obs_category));
	memset(c, 0, sizeof(struct obs_category));

	c->name = name;
	c->next = props->first_category;
	props->first_category = c;

	return c;
}

obs_category_t obs_property_list_categories(obs_property_list_t props)
{
	return props->first_category;
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
	case OBS_PROPERTY_ENUM:      return sizeof(struct list_data);
	case OBS_PROPERTY_TEXT_LIST: return sizeof(struct list_data);
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

	p = bmalloc(sizeof(struct obs_property) + data_size);
	memset(p, 0, sizeof(struct obs_property) + data_size);

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
	struct obs_property *p = new_prop(cat, name, desc, OBS_PROPERTY_INT);
	struct int_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
}

void obs_category_add_float(obs_category_t cat, const char *name,
		const char *desc, double min, double max, double step)
{
	struct obs_property *p = new_prop(cat, name, desc, OBS_PROPERTY_FLOAT);
	struct float_data *data = get_property_data(p);
	data->min  = min;
	data->max  = max;
	data->step = step;
}

void obs_category_add_text(obs_category_t cat, const char *name,
		const char *desc)
{
	new_prop(cat, name, desc, OBS_PROPERTY_TEXT);
}

void obs_category_add_path(obs_category_t cat, const char *name,
		const char *desc)
{
	new_prop(cat, name, desc, OBS_PROPERTY_PATH);
}

void obs_category_add_enum_list(obs_category_t cat, const char *name,
		const char *desc, const char **strings)
{
	struct obs_property *p = new_prop(cat, name, desc, OBS_PROPERTY_ENUM);
	struct list_data *data = get_property_data(p);
	data->strings = strings;
	data->type    = OBS_DROPDOWN_LIST;
}

void obs_category_add_text_list(obs_category_t cat, const char *name,
		const char *desc, const char **strings,
		enum obs_dropdown_type type)
{
	struct obs_property *p = new_prop(cat, name, desc,
			OBS_PROPERTY_TEXT_LIST);
	struct list_data *data = get_property_data(p);
	data->strings = strings;
	data->type    = type;
}

void obs_category_add_color(obs_category_t cat, const char *name,
		const char *desc)
{
	new_prop(cat, name, desc, OBS_PROPERTY_COLOR);
}

bool obs_category_next(obs_category_t *cat)
{
	if (!cat || !*cat)
		return false;

	*cat = (*cat)->next;
	return *cat != NULL;
}

obs_property_t obs_category_properties(obs_category_t cat)
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
	return p->name;
}

const char *obs_property_description(obs_property_t p)
{
	return p->desc;
}

enum obs_property_type obs_property_type(obs_property_t p)
{
	return p->type;
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

static inline bool is_dropdown(struct obs_property *p)
{
	return p->type == OBS_PROPERTY_ENUM ||
	       p->type == OBS_PROPERTY_TEXT_LIST;
}

const char **obs_property_dropdown_strings(obs_property_t p)
{
	if (!p || !is_dropdown(p))
		return NULL;

	struct list_data *data = get_property_data(p);
	return data->strings;
}

enum obs_dropdown_type obs_property_dropdown_type(obs_property_t p)
{
	if (!p || !is_dropdown(p))
		return OBS_DROPDOWN_INVALID;

	struct list_data *data = get_property_data(p);
	return data->type;
}
