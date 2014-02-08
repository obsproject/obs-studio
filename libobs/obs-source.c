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
#include "callback/calldata.h"
#include "graphics/matrix3.h"
#include "graphics/vec3.h"

#include "obs.h"
#include "obs-internal.h"

static void obs_source_destroy(obs_source_t source);

bool load_source_info(void *module, const char *module_name,
		const char *id, struct source_info *info)
{
	LOAD_MODULE_SUBFUNC(getname, true);
	LOAD_MODULE_SUBFUNC(create, true);
	LOAD_MODULE_SUBFUNC(destroy, true);
	LOAD_MODULE_SUBFUNC(get_output_flags, true);

	LOAD_MODULE_SUBFUNC(properties, false);
	LOAD_MODULE_SUBFUNC(update, false);
	LOAD_MODULE_SUBFUNC(activate, false);
	LOAD_MODULE_SUBFUNC(deactivate, false);
	LOAD_MODULE_SUBFUNC(video_tick, false);
	LOAD_MODULE_SUBFUNC(video_render, false);
	LOAD_MODULE_SUBFUNC(getwidth, false);
	LOAD_MODULE_SUBFUNC(getheight, false);
	LOAD_MODULE_SUBFUNC(filter_video, false);
	LOAD_MODULE_SUBFUNC(filter_audio, false);

	info->id = id;
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

static const struct source_info *get_source_info(enum obs_source_type type,
		const char *id)
{
	struct darray *list = NULL;

	switch (type) {
	case SOURCE_INPUT:      list = &obs->input_types.da; break;
	case SOURCE_FILTER:     list = &obs->filter_types.da; break;
	case SOURCE_TRANSITION: list = &obs->transition_types.da; break;
	case SOURCE_SCENE:
	default:
		blog(LOG_WARNING, "get_source_info: invalid source type");
		return NULL;
	}

	return find_source(list, id);
}

bool obs_source_init_handlers(struct obs_source *source)
{
	source->signals = signal_handler_create();
	if (!source->signals)
		return false;

	source->procs = proc_handler_create();
	return (source->procs != NULL);
}

const char *obs_source_getdisplayname(enum obs_source_type type,
		const char *id, const char *locale)
{
	const struct source_info *info = get_source_info(type, id);
	return (info != NULL) ? info->getname(locale) : NULL;
}

/* internal initialization */
bool obs_source_init(struct obs_source *source, const struct source_info *info)
{
	uint32_t flags = info->get_output_flags(source->data);

	source->refs = 1;
	source->volume = 1.0f;
	pthread_mutex_init_value(&source->filter_mutex);
	pthread_mutex_init_value(&source->video_mutex);
	pthread_mutex_init_value(&source->audio_mutex);

	memcpy(&source->callbacks, info, sizeof(struct source_info));

	if (pthread_mutex_init(&source->filter_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->audio_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->video_mutex, NULL) != 0)
		return false;

	if (flags & SOURCE_AUDIO) {
		source->audio_line = audio_output_createline(obs->audio.audio,
				source->name);
		if (!source->audio_line) {
			blog(LOG_ERROR, "Failed to create audio line for "
			                "source '%s'", source->name);
			return false;
		}
	}

	return true;
}

static inline void obs_source_dosignal(struct obs_source *source,
		const char *signal)
{
	struct calldata data;

	calldata_init(&data);
	calldata_setptr(&data, "source", source);
	signal_handler_signal(obs->signals, signal, &data);
	calldata_free(&data);
}

obs_source_t obs_source_create(enum obs_source_type type, const char *id,
		const char *name, obs_data_t settings)
{
	struct obs_source *source;

	const struct source_info *info = get_source_info(type, id);
	if (!info) {
		blog(LOG_WARNING, "Source '%s' not found", id);
		return NULL;
	}

	source = bmalloc(sizeof(struct obs_source));
	memset(source, 0, sizeof(struct obs_source));

	if (!obs_source_init_handlers(source))
		goto fail;

	source->name     = bstrdup(name);
	source->type     = type;
	source->settings = obs_data_newref(settings);
	source->data     = info->create(source->settings, source);

	if (!source->data)
		goto fail;

	if (!obs_source_init(source, info))
		goto fail;

	obs_source_dosignal(source, "source-create");
	return source;

fail:
	blog(LOG_ERROR, "obs_source_create failed");
	obs_source_destroy(source);
	return NULL;
}

#define ALIGN_SIZE(size, align) \
	size = (((size)+(align-1)) & (~(align-1)))

static void alloc_frame_data(struct source_frame *frame,
		enum video_format format, uint32_t width, uint32_t height)
{
	size_t size;
	size_t offsets[MAX_VIDEO_PLANES];
	memset(offsets, 0, sizeof(offsets));

	switch (format) {
	case VIDEO_FORMAT_NONE:
		return;

	case VIDEO_FORMAT_I420:
		size = width * height;
		ALIGN_SIZE(size, 32);
		offsets[0] = size;
		size += (width/2) * (height/2);
		ALIGN_SIZE(size, 32);
		offsets[1] = size;
		size += (width/2) * (height/2);
		ALIGN_SIZE(size, 32);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t*)frame->data[0] + offsets[0];
		frame->data[2] = (uint8_t*)frame->data[0] + offsets[1];
		frame->row_bytes[0] = width;
		frame->row_bytes[1] = width/2;
		frame->row_bytes[2] = width/2;
		break;

	case VIDEO_FORMAT_NV12:
		size = width * height;
		ALIGN_SIZE(size, 32);
		offsets[0] = size;
		size += (width/2) * (height/2) * 2;
		ALIGN_SIZE(size, 32);
		frame->data[0] = bmalloc(size);
		frame->data[1] = (uint8_t*)frame->data[0] + offsets[0];
		frame->row_bytes[0] = width;
		frame->row_bytes[1] = width;
		break;

	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
		size = width * height * 2;
		ALIGN_SIZE(size, 32);
		frame->data[0] = bmalloc(size);
		frame->row_bytes[0] = width*2;
		break;

	case VIDEO_FORMAT_YUVX:
	case VIDEO_FORMAT_UYVX:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		size = width * height * 4;
		ALIGN_SIZE(size, 32);
		frame->data[0] = bmalloc(size);
		frame->row_bytes[0] = width*4;
		break;
	}
}

struct source_frame *source_frame_alloc(enum video_format format,
		uint32_t width, uint32_t height)
{
	struct source_frame *frame = bmalloc(sizeof(struct source_frame));
	memset(frame, 0, sizeof(struct source_frame));

	alloc_frame_data(frame, format, width, height);
	frame->format = format;
	frame->width  = width;
	frame->height = height;
	return frame;
}

static void obs_source_destroy(obs_source_t source)
{
	size_t i;

	obs_source_dosignal(source, "source-destroy");

	if (source->filter_parent)
		obs_source_filter_remove(source->filter_parent, source);

	for (i = 0; i < source->filters.num; i++)
		obs_source_release(source->filters.array[i]);

	for (i = 0; i < source->video_frames.num; i++)
		source_frame_destroy(source->video_frames.array[i]);

	gs_entercontext(obs->video.graphics);
	texture_destroy(source->output_texture);
	gs_leavecontext();

	if (source->data)
		source->callbacks.destroy(source->data);

	for (i = 0; i < MAX_AUDIO_PLANES; i++)
		bfree(source->audio_data.data[i]);

	audio_line_destroy(source->audio_line);
	audio_resampler_destroy(source->resampler);

	proc_handler_destroy(source->procs);
	signal_handler_destroy(source->signals);

	da_free(source->video_frames);
	da_free(source->filters);
	pthread_mutex_destroy(&source->filter_mutex);
	pthread_mutex_destroy(&source->audio_mutex);
	pthread_mutex_destroy(&source->video_mutex);
	obs_data_release(source->settings);
	bfree(source->name);
	bfree(source);
}

void obs_source_addref(obs_source_t source)
{
	if (source)
		++source->refs;
}

void obs_source_release(obs_source_t source)
{
	if (!source)
		return;

	if (--source->refs == 0)
		obs_source_destroy(source);
}

void obs_source_remove(obs_source_t source)
{
	struct obs_core_data *data = &obs->data;
	size_t id;

	pthread_mutex_lock(&data->sources_mutex);

	if (!source || source->removed)
		return;

	source->removed = true;

	obs_source_addref(source);

	id = da_find(data->sources, &source, 0);
	if (id != DARRAY_INVALID) {
		da_erase_item(data->sources, &source);
		obs_source_release(source);
	}

	pthread_mutex_unlock(&data->sources_mutex);

	obs_source_dosignal(source, "source-remove");
	obs_source_release(source);
}

bool obs_source_removed(obs_source_t source)
{
	return source->removed;
}

obs_properties_t obs_source_properties(enum obs_source_type type,
		const char *id, const char *locale)
{
	const struct source_info *info = get_source_info(type, id);
	if (info && info->properties)
	       return info->properties(locale);
	return NULL;
}

uint32_t obs_source_get_output_flags(obs_source_t source)
{
	return source->callbacks.get_output_flags(source->data);
}

void obs_source_update(obs_source_t source, obs_data_t settings)
{
	obs_data_replace(&source->settings, settings);

	if (source->callbacks.update)
		source->callbacks.update(source->data, source->settings);
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

static inline uint64_t conv_frames_to_time(obs_source_t source, size_t frames)
{
	const struct audio_output_info *info;
	double sps_to_ns;

	info = audio_output_getinfo(obs->audio.audio);
	sps_to_ns = 1000000000.0 / (double)info->samples_per_sec;
	return (uint64_t)((double)frames * sps_to_ns);
}

/* maximum "direct" timestamp variance in nanoseconds */
#define MAX_TS_VAR         5000000000ULL
/* maximum time that timestamp can jump in nanoseconds */
#define MAX_TIMESTAMP_JUMP 2000000000ULL

static inline void reset_audio_timing(obs_source_t source, uint64_t timetamp)
{
	source->timing_set    = true;
	source->timing_adjust = os_gettime_ns() - timetamp;
}

static inline void handle_ts_jump(obs_source_t source, uint64_t ts,
		uint64_t diff)
{
	uint32_t flags = source->callbacks.get_output_flags(source->data);

	blog(LOG_DEBUG, "Timestamp for source '%s' jumped by '%lld', "
	                "resetting audio timing", source->name, diff);

	/* if has video, ignore audio data until reset */
	if (flags & SOURCE_ASYNC_VIDEO)
		source->audio_reset_ref--;
	else 
		reset_audio_timing(source, ts);
}

static void source_output_audio_line(obs_source_t source,
		const struct audio_data *data)
{
	struct audio_data in = *data;
	uint64_t diff;

	if (!source->timing_set) {
		reset_audio_timing(source, in.timestamp);

		/* detects 'directly' set timestamps as long as they're within
		 * a certain threshold */
		if ((source->timing_adjust + MAX_TS_VAR) < MAX_TS_VAR * 2)
			source->timing_adjust = 0;

	} else {
		diff = in.timestamp - source->next_audio_ts_min;

		/* don't need signed because negative will trigger it
		 * regardless, which is what we want */
		if (diff > MAX_TIMESTAMP_JUMP)
			handle_ts_jump(source, in.timestamp, diff);
	}

	source->next_audio_ts_min = in.timestamp +
		conv_frames_to_time(source, in.frames);

	if (source->audio_reset_ref != 0)
		return;

	in.timestamp += source->timing_adjust;
	in.volume = source->volume;
	audio_line_output(source->audio_line, &in);
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

	case VIDEO_FORMAT_NONE:
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
	case VIDEO_FORMAT_NONE:
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
		texture_setimage(tex, frame->data[0], frame->row_bytes[0],
				false);
		return true;
	}

	if (!texture_map(tex, &ptr, &row_bytes))
		return false;

	if (type == CONVERT_420)
		decompress_420(frame->data, frame->row_bytes,
				frame->width, frame->height, 0, frame->height,
				ptr, row_bytes);

	else if (type == CONVERT_NV12)
		decompress_nv12(frame->data, frame->row_bytes,
				frame->width, frame->height, 0, frame->height,
				ptr, row_bytes);

	else if (type == CONVERT_422_Y)
		decompress_422(frame->data[0], frame->row_bytes[0],
				frame->width, frame->height, 0, frame->height,
				ptr, row_bytes, true);

	else if (type == CONVERT_422_U)
		decompress_422(frame->data[0], frame->row_bytes[0],
				frame->width, frame->height, 0, frame->height,
				ptr, row_bytes, false);

	texture_unmap(tex);
	return true;
}

static void obs_source_draw_texture(texture_t tex, struct source_frame *frame)
{
	effect_t    effect = obs->video.default_effect;
	bool        yuv    = is_yuv(frame->format);
	const char  *type  = yuv ? "DrawMatrix" : "Draw";
	technique_t tech;
	eparam_t    param;

	if (!upload_frame(tex, frame))
		return;

	tech = effect_gettechnique(effect, type);
	technique_begin(tech);
	technique_beginpass(tech, 0);

	if (yuv) {
		param = effect_getparambyname(effect, "color_matrix");
		effect_setval(effect, param, frame->color_matrix,
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

	if (set_texture_size(source, frame))
		obs_source_draw_texture(source->output_texture, frame);

	obs_source_releaseframe(source, frame);
}

static inline void obs_source_render_filters(obs_source_t source)
{
	source->rendering_filter = true;
	obs_source_video_render(source->filters.array[0]);
	source->rendering_filter = false;
}

static inline void obs_source_default_render(obs_source_t source, bool yuv)
{
	effect_t    effect     = obs->video.default_effect;
	const char  *tech_name = yuv ? "DrawMatrix" : "Draw";
	technique_t tech       = effect_gettechnique(effect, tech_name);
	size_t      passes, i;

	passes = technique_begin(tech);
	for (i = 0; i < passes; i++) {
		technique_beginpass(tech, i);
		source->callbacks.video_render(source->data);
		technique_endpass(tech);
	}
	technique_end(tech);
}

static inline void obs_source_main_render(obs_source_t source)
{
	uint32_t flags = source->callbacks.get_output_flags(source->data);
	bool default_effect = !source->filter_parent &&
	                      source->filters.num == 0 &&
	                      (flags & SOURCE_DEFAULT_EFFECT) != 0;

	if (default_effect)
		obs_source_default_render(source, (flags & SOURCE_YUV) != 0);
	else
		source->callbacks.video_render(source->data);
}

void obs_source_video_render(obs_source_t source)
{
	if (source->callbacks.video_render) {
		if (source->filters.num && !source->rendering_filter)
			obs_source_render_filters(source);
		else
			obs_source_main_render(source);

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

obs_source_t obs_filter_getparent(obs_source_t filter)
{
	return filter->filter_parent;
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

obs_data_t obs_source_getsettings(obs_source_t source)
{
	obs_data_addref(source->settings);
	return source->settings;
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

static inline void copy_frame_data_line(struct source_frame *dst,
		const struct source_frame *src, uint32_t plane, uint32_t y)
{
	uint32_t pos_src = y * src->row_bytes[plane];
	uint32_t pos_dst = y * dst->row_bytes[plane];
	uint32_t bytes = dst->row_bytes[plane] < src->row_bytes[plane] ?
		dst->row_bytes[plane] : src->row_bytes[plane];

	memcpy(dst->data[plane] + pos_dst, src->data[plane] + pos_src, bytes);
}

static inline void copy_frame_data_plane(struct source_frame *dst,
		const struct source_frame *src, uint32_t plane, uint32_t lines)
{
	if (dst->row_bytes != src->row_bytes)
		for (uint32_t y = 0; y < lines; y++)
			copy_frame_data_line(dst, src, plane, y);
	else
		memcpy(dst->data[plane], src->data[plane],
				dst->row_bytes[plane] * lines);
}

static void copy_frame_data(struct source_frame *dst,
		const struct source_frame *src)
{
	dst->flip         = src->flip;
	dst->timestamp    = src->timestamp;
	memcpy(dst->color_matrix, src->color_matrix, sizeof(float) * 16);

	switch (dst->format) {
	case VIDEO_FORMAT_I420:
		copy_frame_data_plane(dst, src, 0, dst->height);
		copy_frame_data_plane(dst, src, 1, dst->height/2);
		copy_frame_data_plane(dst, src, 2, dst->height/2);
		break;

	case VIDEO_FORMAT_NV12:
		copy_frame_data_plane(dst, src, 0, dst->height);
		copy_frame_data_plane(dst, src, 1, dst->height/2);
		break;

	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
	case VIDEO_FORMAT_NONE:
	case VIDEO_FORMAT_YUVX:
	case VIDEO_FORMAT_UYVX:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		copy_frame_data_plane(dst, src, 0, dst->height);
	}
}

static inline struct source_frame *cache_video(obs_source_t source,
		const struct source_frame *frame)
{
	/* TODO: use an actual cache */
	struct source_frame *new_frame = source_frame_alloc(frame->format,
			frame->width, frame->height);

	copy_frame_data(new_frame, frame);
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
	const struct audio_output_info *obs_info;
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
		const void *const data[], uint32_t frames, uint64_t timestamp)
{
	size_t planes    = audio_output_planes(obs->audio.audio);
	size_t blocksize = audio_output_blocksize(obs->audio.audio);
	size_t size      = (size_t)frames * blocksize;
	bool   resize    = source->audio_storage_size < size;

	source->audio_data.frames = frames;
	source->audio_data.timestamp = timestamp;

	for (size_t i = 0; i < planes; i++) {
		/* ensure audio storage capacity */
		if (resize) {
			bfree(source->audio_data.data[i]);
			source->audio_data.data[i] = bmalloc(size);
		}

		memcpy(source->audio_data.data[i], data[i], size);
	}

	if (resize)
		source->audio_storage_size = size;
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
		uint8_t  *output[MAX_AUDIO_PLANES];
		uint32_t frames;
		uint64_t offset;

		memset(output, 0, sizeof(output));

		audio_resampler_resample(source->resampler,
				output, &frames, &offset,
				audio->data, audio->frames);

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
		if (source->timing_set || (flags & SOURCE_ASYNC_VIDEO) == 0) {
			struct audio_data data;

			for (int i = 0; i < MAX_AUDIO_PLANES; i++)
				data.data[i] = output->data[i];

			data.frames    = output->frames;
			data.timestamp = output->timestamp;
			source_output_audio_line(source, &data);
		}

		pthread_mutex_unlock(&source->audio_mutex);
	}

	pthread_mutex_unlock(&source->filter_mutex);
}

static inline bool frame_out_of_bounds(obs_source_t source, uint64_t ts)
{
	return ((ts - source->last_frame_ts) > MAX_TIMESTAMP_JUMP);
}

static inline struct source_frame *get_closest_frame(obs_source_t source,
		uint64_t sys_time, int *audio_time_refs)
{
	struct source_frame *next_frame = source->video_frames.array[0];
	struct source_frame *frame      = NULL;
	uint64_t sys_offset = sys_time - source->last_sys_timestamp;
	uint64_t frame_time = next_frame->timestamp;
	uint64_t frame_offset = 0;

	/* account for timestamp invalidation */
	if (frame_out_of_bounds(source, frame_time)) {
		source->last_frame_ts = next_frame->timestamp;
		(*audio_time_refs)++;
	} else {
		frame_offset = frame_time - source->last_frame_ts;
		source->last_frame_ts += sys_offset;
	}

	while (frame_offset <= sys_offset) {
		source_frame_destroy(frame);

		frame = next_frame;
		da_erase(source->video_frames, 0);

		if (!source->video_frames.num)
			break;

		next_frame = source->video_frames.array[0];

		/* more timestamp checking and compensating */
		if ((next_frame->timestamp - frame_time) > MAX_TIMESTAMP_JUMP) {
			source->last_frame_ts =
				next_frame->timestamp - frame_offset;
			(*audio_time_refs)++;
		}

		frame_time   = next_frame->timestamp;
		frame_offset = frame_time - source->last_frame_ts;
	}

	return frame;
}

/*
 * Ensures that cached frames are displayed on time.  If multiple frames
 * were cached between renders, then releases the unnecessary frames and uses
 * the frame with the closest timing to ensure sync.  Also ensures that timing
 * with audio is synchronized.
 */
struct source_frame *obs_source_getframe(obs_source_t source)
{
	struct source_frame *frame = NULL;
	uint64_t last_frame_time = source->last_frame_ts;
	int      audio_time_refs = 0;
	uint64_t sys_time;

	pthread_mutex_lock(&source->video_mutex);

	if (!source->video_frames.num)
		goto unlock;

	sys_time = os_gettime_ns();

	if (!source->last_frame_ts) {
		frame = source->video_frames.array[0];
		da_erase(source->video_frames, 0);

		source->last_frame_ts = frame->timestamp;
	} else {
		frame = get_closest_frame(source, sys_time, &audio_time_refs);
	}

	/* reset timing to current system time */
	if (frame) {
		source->audio_reset_ref += audio_time_refs;
		source->timing_adjust = sys_time - frame->timestamp;
		source->timing_set = true;
	}

	source->last_sys_timestamp = sys_time;

unlock:
	pthread_mutex_unlock(&source->video_mutex);

	if (frame)
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

const char *obs_source_getname(obs_source_t source)
{
	return source->name;
}

void obs_source_setname(obs_source_t source, const char *name)
{
	bfree(source->name);
	source->name = bstrdup(name);
}

void obs_source_gettype(obs_source_t source, enum obs_source_type *type,
		const char **id)
{
	if (type) *type = source->type;
	if (id)   *id   = source->callbacks.id;
}

static inline void render_filter_bypass(obs_source_t target, effect_t effect,
		uint32_t width, uint32_t height, bool yuv)
{
	const char  *tech_name = yuv ? "DrawMatrix" : "Draw";
	technique_t tech       = effect_gettechnique(effect, tech_name);
	eparam_t    diffuse    = effect_getparambyname(effect, "diffuse");
	size_t      passes, i;

	passes = technique_begin(tech);
	for (i = 0; i < passes; i++) {
		technique_beginpass(tech, i);
		obs_source_video_render(target);
		technique_endpass(tech);
	}
	technique_end(tech);
}

static inline void render_filter_tex(texture_t tex, effect_t effect,
		uint32_t width, uint32_t height, bool yuv)
{
	const char  *tech_name = yuv ? "DrawMatrix" : "Draw";
	technique_t tech       = effect_gettechnique(effect, tech_name);
	eparam_t    diffuse    = effect_getparambyname(effect, "diffuse");
	size_t      passes, i;

	effect_settexture(effect, diffuse, tex);

	passes = technique_begin(tech);
	for (i = 0; i < passes; i++) {
		technique_beginpass(tech, i);
		gs_draw_sprite(tex, width, height, 0);
		technique_endpass(tech);
	}
	technique_end(tech);
}

void obs_source_process_filter(obs_source_t filter, texrender_t texrender,
		effect_t effect, uint32_t width, uint32_t height,
		enum allow_direct_render allow_direct)
{
	obs_source_t target       = obs_filter_gettarget(filter);
	obs_source_t parent       = obs_filter_getparent(filter);
	uint32_t     target_flags = obs_source_get_output_flags(target);
	uint32_t     parent_flags = obs_source_get_output_flags(parent);
	int          cx           = obs_source_getwidth(target);
	int          cy           = obs_source_getheight(target);
	bool         yuv          = (target_flags & SOURCE_YUV) != 0;
	bool         expects_def  = (parent_flags & SOURCE_DEFAULT_EFFECT) != 0;
	bool         can_directly = allow_direct == ALLOW_DIRECT_RENDERING;

	/* if the parent does not use any custom effects, and this is the last
	 * filter in the chain for the parent, then render the parent directly
	 * using the filter effect instead of rendering to texture to reduce
	 * the total number of passes */
	if (can_directly && expects_def && target == parent) {
		render_filter_bypass(target, effect, width, height, yuv);
		return;
	}

	if (texrender_begin(texrender, cx, cy)) {
		gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);
		if (expects_def && parent == target)
			obs_source_default_render(parent, yuv);
		else
			obs_source_video_render(target);
		texrender_end(texrender);
	}

	/* --------------------------- */

	render_filter_tex(texrender_gettexture(texrender), effect,
			width, height, yuv);
}

signal_handler_t obs_source_signalhandler(obs_source_t source)
{
	return source->signals;
}

proc_handler_t obs_source_prochandler(obs_source_t source)
{
	return source->procs;
}

void obs_source_setvolume(obs_source_t source, float volume)
{
	source->volume = volume;
}

float obs_source_getvolume(obs_source_t source)
{
	return source->volume;
}
