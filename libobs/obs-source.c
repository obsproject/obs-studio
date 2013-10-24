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

#include "util/platform.h"

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

static inline const struct source_info *find_source(struct darray *list,
		const char *name)
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

bool obs_source_init(struct obs_source *source, const char *settings,
		const struct source_info *info)
{
	uint32_t flags = info->get_output_flags(source->data);

	pthread_mutex_init_value(&source->filter_mutex);
	pthread_mutex_init_value(&source->video_mutex);
	pthread_mutex_init_value(&source->audio_mutex);
	dstr_copy(&source->settings, settings);
	memcpy(&source->callbacks, info, sizeof(struct source_info));

	if (pthread_mutex_init(&source->filter_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->audio_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->video_mutex, NULL) != 0)
		return false;

	if (flags & SOURCE_AUDIO) {
		source->audio_line = audio_output_createline(obs->audio);
		if (!source->audio_line) {
			blog(LOG_ERROR, "Failed to create audio line for "
			                "source");
			return false;
		}
	}

	source->valid = true;
	pthread_mutex_lock(&obs->source_list_mutex);
	da_push_back(obs->sources, &source);
	pthread_mutex_unlock(&obs->source_list_mutex);

	return true;
}

obs_source_t obs_source_create(enum obs_source_type type, const char *name,
		const char *settings)
{
	const struct source_info *info = NULL;
	struct darray *list = NULL;
	struct obs_source *source;

	switch (type) {
	case SOURCE_INPUT:      list = &obs->input_types.da; break;
	case SOURCE_FILTER:     list = &obs->filter_types.da; break;
	case SOURCE_TRANSITION: list = &obs->transition_types.da; break;
	case SOURCE_SCENE:
	default:
		return NULL;
	}

	info = find_source(list, name);
	if (!info) {
		blog(LOG_WARNING, "Source '%s' not found", type);
		return NULL;
	}

	source = bmalloc(sizeof(struct obs_source));
	memset(source, 0, sizeof(struct obs_source));

	source->data = info->create(settings, source);
	if (!source->data)
		goto fail;

	if (!obs_source_init(source, settings, info))
		goto fail;

	return source;

fail:
	blog(LOG_ERROR, "obs_source_create failed");
	obs_source_destroy(source);
	return NULL;
}

void obs_source_destroy(obs_source_t source)
{
	if (source) {
		size_t i;
		if (source->filter_parent)
			obs_source_filter_remove(source->filter_parent, source);

		for (i = 0; i < source->filters.num; i++)
			obs_source_destroy(source->filters.array[i]);

		if (source->valid) {
			pthread_mutex_lock(&obs->source_list_mutex);
			da_erase_item(obs->sources, &source);
			pthread_mutex_unlock(&obs->source_list_mutex);
		}

		for (i = 0; i < source->audio_buffer.num; i++)
			audiobuf_free(source->audio_buffer.array+i);
		for (i = 0; i < source->video_frames.num; i++)
			source_frame_destroy(source->video_frames.array[i]);

		gs_entercontext(obs->graphics);
		texture_destroy(source->output_texture);
		gs_leavecontext();

		da_free(source->filters);
		if (source->data)
			source->callbacks.destroy(source->data);

		audio_line_destroy(source->audio_line);

		pthread_mutex_destroy(&source->filter_mutex);
		pthread_mutex_destroy(&source->audio_mutex);
		pthread_mutex_destroy(&source->video_mutex);
		dstr_free(&source->settings);
		bfree(source);
	}
}

uint32_t obs_source_get_output_flags(obs_source_t source)
{
	return source->callbacks.get_output_flags(source->data);
}

bool obs_source_hasconfig(obs_source_t source)
{
	return source->callbacks.config != NULL;
}

void obs_source_config(obs_source_t source, void *parent)
{
	if (source->callbacks.config)
		source->callbacks.config(source->data, parent);
}

void obs_source_activate(obs_source_t source)
{
	if (source->callbacks.activate)
		source->callbacks.activate(source->data);
}

void obs_source_deactivate(obs_source_t source)
{
	if (source->callbacks.deactivate)
		source->callbacks.deactivate(source->data);
}

void obs_source_video_tick(obs_source_t source, float seconds)
{
	if (source->callbacks.video_tick)
		source->callbacks.video_tick(source->data, seconds);
}

#define MAX_VARIANCE 2000000000ULL

static void source_output_audio_line(obs_source_t source,
		const struct audio_data *data)
{
	struct audio_data in = *data;

	if (!in.timestamp) {
		in.timestamp = os_gettime_ns();
		if (!source->timing_set) {
			source->timing_set    = true;
			source->timing_adjust = 0;
		}
	}

	if (!source->timing_set) {
		source->timing_set    = true;
		source->timing_adjust = in.timestamp - os_gettime_ns();

		/* detects 'directly' set timestamps as long as they're within
		 * a certain threashold */
		if ((source->timing_adjust+MAX_VARIANCE) < MAX_VARIANCE*2)
			source->timing_adjust = 0;
	}

	in.timestamp += source->timing_adjust;
	audio_line_output(source->audio_line, &in);
}

static void obs_source_flush_audio_buffer(obs_source_t source)
{
	size_t i;

	pthread_mutex_lock(&source->audio_mutex);
	source->timing_set = true;

	for (i = 0; i < source->audio_buffer.num; i++) {
		struct audiobuf *buf = source->audio_buffer.array+i;
		struct audio_data data;

		data.data      = buf->data;
		data.frames    = buf->frames;
		data.timestamp = buf->timestamp + source->timing_adjust;
		audio_line_output(source->audio_line, &data);
		audiobuf_free(buf);
	}

	da_free(source->audio_buffer);
	pthread_mutex_unlock(&source->audio_mutex);
}

static void obs_source_render_async_video(obs_source_t source)
{
	struct source_frame *frame = obs_source_getframe(source);
	if (!frame)
		return;

	source->timing_adjust = frame->timestamp - os_gettime_ns();
	if (!source->timing_set && source->audio_buffer.num)
		obs_source_flush_audio_buffer(source);

	if (!source->output_texture) {
		/* TODO */
	}

	obs_source_releaseframe(source, frame);
}

void obs_source_video_render(obs_source_t source)
{
	if (source->callbacks.video_render) {
		if (source->filters.num && !source->rendering_filter) {
			source->rendering_filter = true;
			obs_source_video_render(source->filters.array[0]);
			source->rendering_filter = false;
		} else {
			source->callbacks.video_render(source->data);
		}

	} else if (source->filter_target) {
		obs_source_video_render(source->filter_target);

	} else {
		obs_source_render_async_video(source);
	}
}

uint32_t obs_source_getwidth(obs_source_t source)
{
	if (source->callbacks.getwidth)
		return source->callbacks.getwidth(source->data);
	return 0;
}

uint32_t obs_source_getheight(obs_source_t source)
{
	if (source->callbacks.getheight)
		return source->callbacks.getheight(source->data);
	return 0;
}

size_t obs_source_getparam(obs_source_t source, const char *param, void *buf,
		size_t buf_size)
{
	if (source->callbacks.getparam)
		return source->callbacks.getparam(source->data, param, buf,
				buf_size);
	return 0;
}

void obs_source_setparam(obs_source_t source, const char *param,
		const void *data, size_t size)
{
	if (source->callbacks.setparam)
		source->callbacks.setparam(source->data, param, data, size);
}

bool obs_source_enum_children(obs_source_t source, size_t idx,
		obs_source_t *child)
{
	if (source->callbacks.enum_children)
		return source->callbacks.enum_children(source, idx, child);
	return false;
}

obs_source_t obs_filter_gettarget(obs_source_t filter)
{
	return filter->filter_target;
}

void obs_source_filter_add(obs_source_t source, obs_source_t filter)
{
	pthread_mutex_lock(&source->filter_mutex);

	if (da_find(source->filters, &filter, 0) != DARRAY_INVALID) {
		blog(LOG_WARNING, "Tried to add a filter that was already "
		                  "present on the source");
		return;
	}

	if (source->filters.num) {
		obs_source_t *back = da_end(source->filters);
		(*back)->filter_target = filter;
	}

	da_push_back(source->filters, &filter);

	pthread_mutex_unlock(&source->filter_mutex);

	filter->filter_parent = source;
	filter->filter_target = source;
}

void obs_source_filter_remove(obs_source_t source, obs_source_t filter)
{
	size_t idx;

	pthread_mutex_lock(&source->filter_mutex);

	idx = da_find(source->filters, &filter, 0);
	if (idx == DARRAY_INVALID)
		return;

	if (idx > 0) {
		obs_source_t prev = source->filters.array[idx-1];
		prev->filter_target = filter->filter_target;
	}

	da_erase(source->filters, idx);

	pthread_mutex_unlock(&source->filter_mutex);

	filter->filter_parent = NULL;
	filter->filter_target = NULL;
}

void obs_source_filter_setorder(obs_source_t source, obs_source_t filter,
		enum order_movement movement)
{
	size_t idx = da_find(source->filters, &filter, 0);
	size_t i;
	if (idx == DARRAY_INVALID)
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
		obs_source_t next_filter = (i == source->filters.num-1) ?
			source : source->filters.array[idx+1];
		source->filters.array[i]->filter_target = next_filter;
	}
}

const char *obs_source_get_settings(obs_source_t source)
{
	return source->settings.array;
}

void obs_source_save_settings(obs_source_t source, const char *settings)
{
	dstr_copy(&source->settings, settings);
}

static inline struct filter_frame *filter_async_video(obs_source_t source,
		struct filter_frame *in)
{
	size_t i;
	for (i = source->filters.num; i > 0; i--) {
		struct obs_source *filter = source->filters.array[i-1];
		if (filter->callbacks.filter_video) {
			in = filter->callbacks.filter_video(filter->data, in);
			if (!in)
				return NULL;
		}
	}

	return in;
}

static struct filter_frame *process_video(obs_source_t source,
		const struct source_video *frame)
{
	/* TODO: convert to YUV444 or RGB */
	return NULL;
}

void obs_source_output_video(obs_source_t source,
		const struct source_video *frame)
{
	struct filter_frame *output;

	output = process_video(source, frame);

	pthread_mutex_lock(&source->filter_mutex);
	output = filter_async_video(source, output);
	pthread_mutex_unlock(&source->filter_mutex);

	pthread_mutex_lock(&source->video_mutex);
	da_push_back(source->video_frames, &output);
	pthread_mutex_unlock(&source->video_mutex);
}

static inline const struct audio_data *filter_async_audio(obs_source_t source,
		const struct audio_data *in)
{
	size_t i;
	for (i = source->filters.num; i > 0; i--) {
		struct obs_source *filter = source->filters.array[i-1];
		if (filter->callbacks.filter_audio) {
			in = filter->callbacks.filter_audio(filter->data, in);
			if (!in)
				return NULL;
		}
	}

	return in;
}

static struct audio_data *process_audio(obs_source_t source,
		const struct source_audio *audio)
{
	/* TODO: upmix/downmix/resample */
	return NULL;
}

void obs_source_output_audio(obs_source_t source,
		const struct source_audio *audio)
{
	uint32_t flags = obs_source_get_output_flags(source);
	struct audio_data *data;
	const struct audio_data *output;

	data = process_audio(source, audio);

	pthread_mutex_lock(&source->filter_mutex);
	output = filter_async_audio(source, data);

	if (output) {
		pthread_mutex_lock(&source->audio_mutex);

		if (!source->timing_set && flags & SOURCE_ASYNC_VIDEO) {
			struct audiobuf newbuf;
			size_t audio_size = audio_output_blocksize(obs->audio) *
				output->frames;

			newbuf.data      = bmalloc(audio_size);
			newbuf.frames    = output->frames;
			newbuf.timestamp = output->timestamp;
			memcpy(newbuf.data, output->data, audio_size);

			da_push_back(source->audio_buffer, &newbuf);

		} else {
			source_output_audio_line(source, output);
		}

		pthread_mutex_unlock(&source->audio_mutex);
	}

	pthread_mutex_unlock(&source->filter_mutex);
}

struct source_frame *obs_source_getframe(obs_source_t source)
{
	/* TODO */
	return NULL;
}

void obs_source_releaseframe(obs_source_t source, struct source_frame *frame)
{
	/* TODO */
}
