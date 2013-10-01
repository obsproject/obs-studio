/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "obs.h"
#include "obs-data.h"

bool get_source_info(void *module, const char *module_name,
		const char *source_name, struct source_info *info)
{
	info->create = load_module_subfunc(module, module_name,
			source_name,"create", true);
	info->destroy = load_module_subfunc(module, module_name,
			source_name, "destroy", true);
	info->get_output_flags = load_module_subfunc(module, module_name,
			source_name, "get_output_flags", true);

	if (!info->create || !info->destroy || !info->get_output_flags)
		return false;

	info->config = load_module_subfunc(module, module_name,
			source_name, "config", false);
	info->activate = load_module_subfunc(module, module_name,
			source_name, "activate", false);
	info->deactivate = load_module_subfunc(module, module_name,
			source_name, "deactivate", false);
	info->video_tick = load_module_subfunc(module, module_name,
			source_name, "video_tick", false);
	info->video_render = load_module_subfunc(module, module_name,
			source_name, "video_render", false);
	info->getwidth = load_module_subfunc(module, module_name,
			source_name, "getwidth", false);
	info->getheight = load_module_subfunc(module, module_name,
			source_name, "getheight", false);

	info->getparam = load_module_subfunc(module, module_name,
			source_name, "getparam", false);
	info->setparam = load_module_subfunc(module, module_name,
			source_name, "setparam", false);
	info->enum_children = load_module_subfunc(module, module_name,
			source_name, "enum_children", false);

	info->filter_video = load_module_subfunc(module, module_name,
			source_name, "filter_video", false);
	info->filter_audio = load_module_subfunc(module, module_name,
			source_name, "filter_audio", false);

	info->name = source_name;
	return true;
}

static inline const struct source_info *find_source(obs_t obs,
		struct darray *list, const char *name)
{
	size_t i;
	struct source_info *array = list->array;

	for (i = 0; i < list->num; i++) {
		struct source_info *info = array+i;
		if (strcmp(info->name, name) == 0)
			return info;
	}

	return NULL;
}

void source_init(obs_t obs, struct obs_source *source)
{
	source->obs              = obs;
	source->filter_target    = NULL;
	source->rendering_filter = false;

	dstr_init(&source->settings);
	da_init(source->filters);
	da_push_back(obs->sources, &source);
}

source_t source_create(obs_t obs, enum source_type type, const char *name,
		const char *settings)
{
	const struct source_info *info = NULL;
	struct darray *list = NULL;
	struct obs_source *source;

	switch (type) {
	case SOURCE_INPUT:      list = &obs->input_types.da; break;
	case SOURCE_FILTER:     list = &obs->filter_types.da; break;
	case SOURCE_TRANSITION: list = &obs->transition_types.da; break;
	default:
		return NULL;
	}

	info = find_source(obs, list, name);
	if (!info) {
		blog(LOG_WARNING, "Source '%s' not found", type);
		return NULL;
	}

	source = bmalloc(sizeof(struct obs_source));
	source->data = info->create(settings, source);
	if (!source->data) {
		bfree(source);
		return NULL;
	}

	source_init(obs, source);
	dstr_copy(&source->settings, settings);
	memcpy(&source->callbacks, info, sizeof(struct source_info));
	return source;
}

void source_destroy(source_t source)
{
	if (source) {
		da_free(source->filters);
		da_erase_item(source->obs->sources, &source);

		source->callbacks.destroy(source->data);
		dstr_free(&source->settings);
		bfree(source);
	}
}

uint32_t source_get_output_flags(source_t source)
{
	return source->callbacks.get_output_flags(source->data);
}

bool source_hasconfig(source_t source)
{
	return source->callbacks.config != NULL;
}

void source_config(source_t source, void *parent)
{
	if (source->callbacks.config)
		source->callbacks.config(source->data, parent);
}

void source_activate(source_t source)
{
	if (source->callbacks.activate)
		source->callbacks.activate(source->data);
}

void source_deactivate(source_t source)
{
	if (source->callbacks.deactivate)
		source->callbacks.deactivate(source->data);
}

void source_video_tick(source_t source, float seconds)
{
	if (source->callbacks.video_tick)
		source->callbacks.video_tick(source->data, seconds);
}

void source_video_render(source_t source)
{
	if (source->callbacks.video_render) {
		if (source->filters.num && !source->rendering_filter) {
			source->rendering_filter = true;
			source_video_render(source->filters.array[0]);
			source->rendering_filter = false;
		} else {
			source->callbacks.video_render(source->data);
		}
	}
}

int source_getwidth(source_t source)
{
	if (source->callbacks.getwidth)
		return source->callbacks.getwidth(source->data);
	return 0;
}

int source_getheight(source_t source)
{
	if (source->callbacks.getheight)
		return source->callbacks.getheight(source->data);
	return 0;
}

size_t source_getparam(source_t source, const char *param, void *buf,
		size_t buf_size)
{
	if (source->callbacks.getparam)
		return source->callbacks.getparam(source->data, param, buf,
				buf_size);
	return 0;
}

void source_setparam(source_t source, const char *param, const void *data,
		size_t size)
{
	if (source->callbacks.setparam)
		source->callbacks.setparam(source->data, param, data, size);
}

bool source_enum_children(source_t source, size_t idx, source_t *child)
{
	if (source->callbacks.enum_children)
		return source->callbacks.enum_children(source, idx, child);
	return false;
}

source_t filter_gettarget(source_t filter)
{
	return filter->filter_target;
}

void source_filter_add(source_t source, source_t filter)
{
	if (da_find(source->filters, &filter, 0) != -1) {
		blog(LOG_WARNING, "Tried to add a filter that was already "
		                  "present on the source");
		return;
	}

	if (source->filters.num) {
		source_t *back = da_end(source->filters);
		(*back)->filter_target = filter;
	}

	da_push_back(source->filters, &filter);
	filter->filter_target = source;
}

void source_filter_remove(source_t source, source_t filter)
{
	size_t idx = da_find(source->filters, &filter, 0);
	if (idx == -1)
		return;

	if (idx > 0) {
		source_t prev = source->filters.array[idx-1];
		prev->filter_target = filter->filter_target;
	}

	da_erase(source->filters, idx);
	filter->filter_target = NULL;
}

void source_filter_setorder(source_t source, source_t filter,
		enum order_movement movement)
{
	size_t idx = da_find(source->filters, &filter, 0);
	size_t i;
	if (idx == -1)
		return;

	if (movement == ORDER_MOVE_UP) {
		if (idx == source->filters.num-1)
			return;
		da_move_item(source->filters, idx, idx+1);

	} else if (movement == ORDER_MOVE_DOWN) {
		if (idx == 0)
			return;
		da_move_item(source->filters, idx, idx-1);

	} else if (movement == ORDER_MOVE_TOP) {
		if (idx == source->filters.num-1)
			return;
		da_move_item(source->filters, idx, source->filters.num-1);

	} else if (movement == ORDER_MOVE_BOTTOM) {
		if (idx == 0)
			return;
		da_move_item(source->filters, idx, 0);
	}

	/* reorder filter targets */
	for (i = 0; i < source->filters.num; i++) {
		source_t next_filter = (i == source->filters.num-1) ?
			source : source->filters.array[idx+1];
		source->filters.array[i]->filter_target = next_filter;
	}
}

const char *source_get_settings(source_t source)
{
	return source->settings.array;
}

void source_save_settings(source_t source, const char *settings)
{
	dstr_copy(&source->settings, settings);
}

void source_output_video(source_t source, struct video_frame *frame)
{
	/* TODO */
}

void source_output_audio(source_t source, struct audio_data *audio)
{
	/* TODO */
}
