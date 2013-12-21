/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "media-io/format-conversion.h"
#include "util/platform.h"
#include "graphics/matrix3.h"
#include "graphics/vec3.h"

#include "obs.h"
#include "obs-data.h"

static void obs_source_destroy(obs_source_t source);

bool get_source_info(void *module, const char *module_name,
		const char *source_id, struct source_info *info)
{
	info->getname = load_module_subfunc(module, module_name,
			source_id, "getname", true);
	info->create = load_module_subfunc(module, module_name,
			source_id,"create", true);
	info->destroy = load_module_subfunc(module, module_name,
			source_id, "destroy", true);
	info->get_output_flags = load_module_subfunc(module, module_name,
			source_id, "get_output_flags", true);

	if (!info->getname || !info->create || !info->destroy ||
	    !info->get_output_flags)
		return false;

	info->config = load_module_subfunc(module, module_name,
			source_id, "config", false);
	info->activate = load_module_subfunc(module, module_name,
			source_id, "activate", false);
	info->deactivate = load_module_subfunc(module, module_name,
			source_id, "deactivate", false);
	info->video_tick = load_module_subfunc(module, module_name,
			source_id, "video_tick", false);
	info->video_render = load_module_subfunc(module, module_name,
			source_id, "video_render", false);
	info->getwidth = load_module_subfunc(module, module_name,
			source_id, "getwidth", false);
	info->getheight = load_module_subfunc(module, module_name,
			source_id, "getheight", false);

	info->getparam = load_module_subfunc(module, module_name,
			source_id, "getparam", false);
	info->setparam = load_module_subfunc(module, module_name,
			source_id, "setparam", false);

	info->filter_video = load_module_subfunc(module, module_name,
			source_id, "filter_video", false);
	info->filter_audio = load_module_subfunc(module, module_name,
			source_id, "filter_audio", false);

	info->id = source_id;
	return true;
}

static inline const struct source_info *find_source(struct darray *list,
		const char *id)
{
	size_t i;
	struct source_info *array = list->array;

	for (i = 0; i < list->num; i++) {
		struct source_info *info = array+i;
		if (strcmp(info->id, id) == 0)
			return info;
	}

	return NULL;
}

/* internal initialization */
bool obs_source_init(struct obs_source *source, const char *settings,
		const struct source_info *info)
{
	uint32_t flags = info->get_output_flags(source->data);

	source->refs = 1;
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
		source->audio_line = audio_output_createline(obs->audio.audio);
		if (!source->audio_line) {
			blog(LOG_ERROR, "Failed to create audio line for "
			                "source");
			return false;
		}
	}

	return true;
}

obs_source_t obs_source_create(enum obs_source_type type, const char *id,
		const char *name, const char *settings)
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
		blog(LOG_WARNING, "Tried to create invalid source type");
		return NULL;
	}

	info = find_source(list, id);
	if (!info) {
		blog(LOG_WARNING, "Source '%s' not found", id);
		return NULL;
	}

	source = bmalloc(sizeof(struct obs_source));
	memset(source, 0, sizeof(struct obs_source));

	source->name = bstrdup(name);
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

static void obs_source_destroy(obs_source_t source)
{
	size_t i;

	if (source->filter_parent)
		obs_source_filter_remove(source->filter_parent, source);

	for (i = 0; i < source->filters.num; i++)
		obs_source_release(source->filters.array[i]);

	for (i = 0; i < source->audio_wait_buffer.num; i++)
		audiobuf_free(source->audio_wait_buffer.array+i);
	for (i = 0; i < source->video_frames.num; i++)
		source_frame_destroy(source->video_frames.array[i]);

	gs_entercontext(obs->video.graphics);
	texture_destroy(source->output_texture);
	gs_leavecontext();

	if (source->data)
		source->callbacks.destroy(source->data);

	bfree(source->audio_data.data);
	audio_line_destroy(source->audio_line);
	audio_resampler_destroy(source->resampler);

	da_free(source->video_frames);
	da_free(source->audio_wait_buffer);
	da_free(source->filters);
	pthread_mutex_destroy(&source->filter_mutex);
	pthread_mutex_destroy(&source->audio_mutex);
	pthread_mutex_destroy(&source->video_mutex);
	dstr_free(&source->settings);
	bfree(source->name);
	bfree(source);
}

int obs_source_addref(obs_source_t source)
{
	assert(source != NULL);
	if (!source)
		return 0;

	return ++source->refs;
}

int obs_source_release(obs_source_t source)
{
	int refs;

	assert(source != NULL);
	if (!source)
		return 0;

	refs = --source->refs;
	if (refs == 0)
		obs_source_destroy(source);

	return refs;
}

void obs_source_remove(obs_source_t source)
{
	struct obs_data *data = &obs->data;
	size_t id;

	source->removed = true;

	pthread_mutex_lock(&data->sources_mutex);

	id = da_find(data->sources, &source, 0);
	if (id != DARRAY_INVALID) {
		da_erase_item(data->sources, &source);
		obs_source_release(source);
	}

	pthread_mutex_unlock(&data->sources_mutex);
}

bool obs_source_removed(obs_source_t source)
{
	return source->removed;
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

/* maximum "direct" timestamp variance in nanoseconds */
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
		 * a certain threshold */
		if ((source->timing_adjust+MAX_VARIANCE) < MAX_VARIANCE*2)
			source->timing_adjust = 0;
	}

	in.timestamp += source->timing_adjust;
	audio_line_output(source->audio_line, &in);
}

static void obs_source_flush_audio_wait_buffer(obs_source_t source)
{
	size_t i;

	pthread_mutex_lock(&source->audio_mutex);
	source->timing_set = true;

	for (i = 0; i < source->audio_wait_buffer.num; i++) {
		struct audiobuf *buf = source->audio_wait_buffer.array+i;
		struct audio_data data;

		data.data      = buf->data;
		data.frames    = buf->frames;
		data.timestamp = buf->timestamp + source->timing_adjust;
		audio_line_output(source->audio_line, &data);
		audiobuf_free(buf);
	}

	da_free(source->audio_wait_buffer);
	pthread_mutex_unlock(&source->audio_mutex);
}

static bool set_texture_size(obs_source_t source, struct source_frame *frame)
{
	if (source->output_texture) {
		uint32_t width  = texture_getwidth(source->output_texture);
		uint32_t height = texture_getheight(source->output_texture);

		if (width == frame->width && height == frame->height)
			return true;
	}

	texture_destroy(source->output_texture);
	source->output_texture = gs_create_texture(frame->width, frame->height,
			GS_RGBA, 1, NULL, GS_DYNAMIC);

	return source->output_texture != NULL;
}

enum convert_type {
	CONVERT_NONE,
	CONVERT_NV12,
	CONVERT_420,
	CONVERT_422_U,
	CONVERT_422_Y,
};

static inline enum convert_type get_convert_type(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_I420:
		return CONVERT_420;
	case VIDEO_FORMAT_NV12:
		return CONVERT_NV12;

	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
		return CONVERT_422_Y;
	case VIDEO_FORMAT_UYVY:
		return CONVERT_422_U;

	case VIDEO_FORMAT_UNKNOWN:
	case VIDEO_FORMAT_YUVX:
	case VIDEO_FORMAT_UYVX:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		return CONVERT_NONE;
	}

	return CONVERT_NONE;
}

static inline bool is_yuv(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_I420:
	case VIDEO_FORMAT_NV12:
	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
	case VIDEO_FORMAT_YUVX:
	case VIDEO_FORMAT_UYVX:
		return true;
	case VIDEO_FORMAT_UNKNOWN:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		return false;
	}

	return false;
}

static bool upload_frame(texture_t tex, const struct source_frame *frame)
{
	void *ptr;
	uint32_t row_bytes;
	enum convert_type type = get_convert_type(frame->format);

	if (type == CONVERT_NONE) {
		texture_setimage(tex, frame->data, frame->row_bytes, false);
		return true;
	}

	if (!texture_map(tex, &ptr, &row_bytes))
		return false;

	if (type == CONVERT_420)
		decompress_420(frame->data, frame->width, frame->height,
				frame->row_bytes, 0, frame->height, ptr);

	else if (type == CONVERT_NV12)
		decompress_nv12(frame->data, frame->width, frame->height,
				frame->row_bytes, 0, frame->height, ptr);

	else if (type == CONVERT_422_Y)
		decompress_422(frame->data, frame->width, frame->height,
				frame->row_bytes, 0, frame->height, ptr, true);

	else if (type == CONVERT_422_U)
		decompress_422(frame->data, frame->width, frame->height,
				frame->row_bytes, 0, frame->height, ptr, false);

	texture_unmap(tex);
	return true;
}

static void obs_source_draw_texture(texture_t tex, struct source_frame *frame)
{
	effect_t    effect = obs->video.default_effect;
	bool        yuv   = is_yuv(frame->format);
	const char  *type = yuv ? "DrawYUVToRGB" : "DrawRGB";
	technique_t tech;
	eparam_t    param;

	if (!upload_frame(tex, frame))
		return;

	tech = effect_gettechnique(effect, type);
	technique_begin(tech);
	technique_beginpass(tech, 0);

	if (yuv) {
		param = effect_getparambyname(effect, "yuv_matrix");
		effect_setval(effect, param, frame->yuv_matrix,
				sizeof(float) * 16);
	}

	param = effect_getparambyname(effect, "diffuse");
	effect_settexture(effect, param, tex);

	gs_draw_sprite(tex, frame->flip ? GS_FLIP_V : 0, 0, 0);

	technique_endpass(tech);
	technique_end(tech);
}

static void obs_source_render_async_video(obs_source_t source)
{
	struct source_frame *frame = obs_source_getframe(source);
	if (!frame)
		return;

	source->timing_adjust = frame->timestamp - os_gettime_ns();
	if (!source->timing_set && source->audio_wait_buffer.num)
		obs_source_flush_audio_wait_buffer(source);

	if (set_texture_size(source, frame))
		obs_source_draw_texture(source->output_texture, frame);

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

	/* reorder filter targets, not the nicest way of dealing with things */
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

static inline struct source_frame *filter_async_video(obs_source_t source,
		struct source_frame *in)
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

static inline struct source_frame *cache_video(obs_source_t source,
		const struct source_frame *frame)
{
	/* TODO: use an actual cache */
	struct source_frame *new_frame = bmalloc(sizeof(struct source_frame));
	memcpy(new_frame, frame, sizeof(struct source_frame));
	new_frame->data = bmalloc(frame->row_bytes * frame->height);

	return new_frame;
}

void obs_source_output_video(obs_source_t source,
		const struct source_frame *frame)
{
	struct source_frame *output = cache_video(source, frame);

	pthread_mutex_lock(&source->filter_mutex);
	output = filter_async_video(source, output);
	pthread_mutex_unlock(&source->filter_mutex);

	if (output) {
		pthread_mutex_lock(&source->video_mutex);
		da_push_back(source->video_frames, &output);
		pthread_mutex_unlock(&source->video_mutex);
	}
}

static inline struct filtered_audio *filter_async_audio(obs_source_t source,
		struct filtered_audio *in)
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

static inline void reset_resampler(obs_source_t source,
		const struct source_audio *audio)
{
	const struct audio_info *obs_info;
	struct resample_info output_info;

	obs_info = audio_output_getinfo(obs->audio.audio);

	output_info.format           = obs_info->format;
	output_info.samples_per_sec  = obs_info->samples_per_sec;
	output_info.speakers         = obs_info->speakers;

	source->sample_info.format          = audio->format;
	source->sample_info.samples_per_sec = audio->samples_per_sec;
	source->sample_info.speakers        = audio->speakers;

	if (source->sample_info.samples_per_sec == obs_info->samples_per_sec &&
	    source->sample_info.format          == obs_info->format          &&
	    source->sample_info.speakers        == obs_info->speakers) {
		source->audio_failed = false;
		return;
	}

	audio_resampler_destroy(source->resampler);
	source->resampler = audio_resampler_create(&output_info,
			&source->sample_info);

	source->audio_failed = source->resampler == NULL;
	if (source->resampler == NULL)
		blog(LOG_ERROR, "creation of resampler failed");
}

static inline void copy_audio_data(obs_source_t source,
		const void *data, uint32_t frames, uint64_t timestamp)
{
	size_t blocksize = audio_output_blocksize(obs->audio.audio);
	size_t size = (size_t)frames * blocksize;

	/* ensure audio storage capacity */
	if (source->audio_storage_size < size) {
		bfree(source->audio_data.data);
		source->audio_data.data = bmalloc(size);
		source->audio_storage_size = size;
	}

	source->audio_data.frames = frames;
	source->audio_data.timestamp = timestamp;
	memcpy(source->audio_data.data, data, size);
}

/* resamples/remixes new audio to the designated main audio output format */
static void process_audio(obs_source_t source, const struct source_audio *audio)
{
	if (source->sample_info.samples_per_sec != audio->samples_per_sec ||
	    source->sample_info.format          != audio->format          ||
	    source->sample_info.speakers        != audio->speakers)
		reset_resampler(source, audio);

	if (source->audio_failed)
		return;

	if (source->resampler) {
		void *output;
		uint32_t frames;
		uint64_t offset;

		audio_resampler_resample(source->resampler, &output, &frames,
				audio->data, audio->frames, &offset);

		copy_audio_data(source, output, frames,
				audio->timestamp - offset);
	} else {
		copy_audio_data(source, audio->data, audio->frames,
				audio->timestamp);
	}
}

void obs_source_output_audio(obs_source_t source,
		const struct source_audio *audio)
{
	uint32_t flags = obs_source_get_output_flags(source);
	size_t blocksize = audio_output_blocksize(obs->audio.audio);
	struct filtered_audio *output;

	process_audio(source, audio);

	pthread_mutex_lock(&source->filter_mutex);
	output = filter_async_audio(source, &source->audio_data);

	if (output) {
		pthread_mutex_lock(&source->audio_mutex);

		/* wait for video to start before outputting any audio so we
		 * have a base for sync */
		if (!source->timing_set && flags & SOURCE_ASYNC_VIDEO) {
			struct audiobuf newbuf;
			size_t audio_size = blocksize * output->frames;

			newbuf.data      = bmalloc(audio_size);
			newbuf.frames    = output->frames;
			newbuf.timestamp = output->timestamp;
			memcpy(newbuf.data, output->data, audio_size);

			da_push_back(source->audio_wait_buffer, &newbuf);

		} else {
			struct audio_data data;
			data.data      = output->data;
			data.frames    = output->frames;
			data.timestamp = output->timestamp;
			source_output_audio_line(source, &data);
		}

		pthread_mutex_unlock(&source->audio_mutex);
	}

	pthread_mutex_unlock(&source->filter_mutex);
}

/*
 * Ensures that cached frames are displayed on time.  If multiple frames
 * were cached between renders, then releases the unnecessary frames and uses
 * the frame with the closest timing to ensure sync.
 */
struct source_frame *obs_source_getframe(obs_source_t source)
{
	uint64_t last_frame_time = source->last_frame_timestamp;
	struct   source_frame *frame = NULL;
	struct   source_frame *next_frame;
	uint64_t sys_time, frame_time;

	pthread_mutex_lock(&source->video_mutex);

	if (!source->video_frames.num)
		goto unlock;

	next_frame = source->video_frames.array[0];
	sys_time   = os_gettime_ns();
	frame_time = next_frame->timestamp;

	if (!source->last_frame_timestamp) {
		frame = next_frame;
		da_erase(source->video_frames, 0);

		source->last_frame_timestamp = frame_time;
	} else {
		uint64_t sys_offset, frame_offset;
		sys_offset   = sys_time   - source->last_sys_timestamp;
		frame_offset = frame_time - last_frame_time;

		source->last_frame_timestamp += sys_offset;

		while (frame_offset <= sys_offset) {
			if (frame)
				source_frame_destroy(frame);

			frame = next_frame;
			da_erase(source->video_frames, 0);

			if (!source->video_frames.num)
				break;

			next_frame   = source->video_frames.array[0];
			frame_time   = next_frame->timestamp;
			frame_offset = frame_time - last_frame_time;
		}
	}

	source->last_sys_timestamp = sys_time;

unlock:
	pthread_mutex_unlock(&source->video_mutex);

	if (frame != NULL)
		obs_source_addref(source);

	return frame;
}

void obs_source_releaseframe(obs_source_t source, struct source_frame *frame)
{
	if (frame) {
		source_frame_destroy(frame);
		obs_source_release(source);
	}
}
