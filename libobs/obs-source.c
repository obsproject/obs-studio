/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include <inttypes.h>

#include "media-io/format-conversion.h"
#include "media-io/video-frame.h"
#include "util/threading.h"
#include "util/platform.h"
#include "callback/calldata.h"
#include "graphics/matrix3.h"
#include "graphics/vec3.h"

#include "obs.h"
#include "obs-internal.h"

static inline const struct obs_source_info *find_source(struct darray *list,
		const char *id)
{
	size_t i;
	struct obs_source_info *array = list->array;

	for (i = 0; i < list->num; i++) {
		struct obs_source_info *info = array+i;
		if (strcmp(info->id, id) == 0)
			return info;
	}

	return NULL;
}

static const struct obs_source_info *get_source_info(enum obs_source_type type,
		const char *id)
{
	struct darray *list = NULL;

	switch (type) {
	case OBS_SOURCE_TYPE_INPUT:
		list = &obs->input_types.da;
		break;

	case OBS_SOURCE_TYPE_FILTER:
		list = &obs->filter_types.da;
		break;

	case OBS_SOURCE_TYPE_TRANSITION:
		list = &obs->transition_types.da;
		break;
	}

	return find_source(list, id);
}

static const char *source_signals[] = {
	"void destroy(ptr source)",
	"void add(ptr source)",
	"void remove(ptr source)",
	"void activate(ptr source)",
	"void deactivate(ptr source)",
	"void show(ptr source)",
	"void hide(ptr source)",
	"void volume(ptr source, in out float volume)",
	NULL
};

bool obs_source_init_context(struct obs_source *source,
		obs_data_t settings, const char *name)
{
	if (!obs_context_data_init(&source->context, settings, name))
		return false;

	return signal_handler_add_array(source->context.signals,
			source_signals);
}

const char *obs_source_getdisplayname(enum obs_source_type type,
		const char *id, const char *locale)
{
	const struct obs_source_info *info = get_source_info(type, id);
	return (info != NULL) ? info->getname(locale) : NULL;
}

/* internal initialization */
bool obs_source_init(struct obs_source *source,
		const struct obs_source_info *info)
{
	source->refs = 1;
	source->user_volume = 1.0f;
	source->present_volume = 1.0f;
	source->sync_offset = 0;
	pthread_mutex_init_value(&source->filter_mutex);
	pthread_mutex_init_value(&source->video_mutex);
	pthread_mutex_init_value(&source->audio_mutex);

	memcpy(&source->info, info, sizeof(struct obs_source_info));

	if (pthread_mutex_init(&source->filter_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->audio_mutex, NULL) != 0)
		return false;
	if (pthread_mutex_init(&source->video_mutex, NULL) != 0)
		return false;

	if (info->output_flags & OBS_SOURCE_AUDIO) {
		source->audio_line = audio_output_createline(obs->audio.audio,
				source->context.name);
		if (!source->audio_line) {
			blog(LOG_ERROR, "Failed to create audio line for "
			                "source '%s'", source->context.name);
			return false;
		}
	}

	obs_context_data_insert(&source->context,
			&obs->data.sources_mutex,
			&obs->data.first_source);
	return true;
}

static inline void obs_source_dosignal(struct obs_source *source,
		const char *signal_obs, const char *signal_source)
{
	struct calldata data;

	calldata_init(&data);
	calldata_setptr(&data, "source", source);
	if (signal_obs)
		signal_handler_signal(obs->signals, signal_obs, &data);
	if (signal_source)
		signal_handler_signal(source->context.signals, signal_source,
				&data);
	calldata_free(&data);
}

obs_source_t obs_source_create(enum obs_source_type type, const char *id,
		const char *name, obs_data_t settings)
{
	struct obs_source *source;

	const struct obs_source_info *info = get_source_info(type, id);
	if (!info) {
		blog(LOG_ERROR, "Source '%s' not found", id);
		return NULL;
	}

	source = bzalloc(sizeof(struct obs_source));

	if (!obs_source_init_context(source, settings, name))
		goto fail;

	if (info->defaults)
		info->defaults(source->context.settings);

	source->context.data = info->create(source->context.settings, source);
	if (!source->context.data)
		goto fail;

	if (!obs_source_init(source, info))
		goto fail;

	obs_source_dosignal(source, "source_create", NULL);
	return source;

fail:
	blog(LOG_ERROR, "obs_source_create failed");
	obs_source_destroy(source);
	return NULL;
}

void source_frame_init(struct source_frame *frame, enum video_format format,
		uint32_t width, uint32_t height)
{
	struct video_frame vid_frame;

	if (!frame)
		return;

	video_frame_init(&vid_frame, format, width, height);
	frame->format = format;
	frame->width  = width;
	frame->height = height;

	for (size_t i = 0; i < MAX_AV_PLANES; i++) {
		frame->data[i]     = vid_frame.data[i];
		frame->linesize[i] = vid_frame.linesize[i];
	}
}

void obs_source_destroy(struct obs_source *source)
{
	size_t i;

	if (!source)
		return;

	obs_context_data_remove(&source->context);

	obs_source_dosignal(source, "source_destroy", "destroy");

	if (source->filter_parent)
		obs_source_filter_remove(source->filter_parent, source);

	for (i = 0; i < source->filters.num; i++)
		obs_source_release(source->filters.array[i]);

	for (i = 0; i < source->video_frames.num; i++)
		source_frame_destroy(source->video_frames.array[i]);

	gs_entercontext(obs->video.graphics);
	texrender_destroy(source->async_convert_texrender);
	texture_destroy(source->async_texture);
	gs_leavecontext();

	if (source->context.data)
		source->info.destroy(source->context.data);

	for (i = 0; i < MAX_AV_PLANES; i++)
		bfree(source->audio_data.data[i]);

	audio_line_destroy(source->audio_line);
	audio_resampler_destroy(source->resampler);

	texrender_destroy(source->filter_texrender);
	da_free(source->video_frames);
	da_free(source->filters);
	pthread_mutex_destroy(&source->filter_mutex);
	pthread_mutex_destroy(&source->audio_mutex);
	pthread_mutex_destroy(&source->video_mutex);
	obs_context_data_free(&source->context);
	bfree(source);
}

void obs_source_addref(obs_source_t source)
{
	if (source)
		os_atomic_inc_long(&source->refs);
}

void obs_source_release(obs_source_t source)
{
	if (!source)
		return;

	if (os_atomic_dec_long(&source->refs) == 0)
		obs_source_destroy(source);
}

void obs_source_remove(obs_source_t source)
{
	struct obs_core_data *data = &obs->data;
	size_t id;
	bool   exists;

	pthread_mutex_lock(&data->sources_mutex);

	if (!source || source->removed) {
		pthread_mutex_unlock(&data->sources_mutex);
		return;
	}

	source->removed = true;

	obs_source_addref(source);

	id = da_find(data->user_sources, &source, 0);
	exists = (id != DARRAY_INVALID);
	if (exists) {
		da_erase(data->user_sources, id);
		obs_source_release(source);
	}

	pthread_mutex_unlock(&data->sources_mutex);

	if (exists)
		obs_source_dosignal(source, "source_remove", "remove");

	obs_source_release(source);
}

bool obs_source_removed(obs_source_t source)
{
	return source ? source->removed : true;
}

static inline obs_data_t get_defaults(const struct obs_source_info *info)
{
	obs_data_t settings = obs_data_create();
	if (info->defaults)
		info->defaults(settings);
	return settings;
}

obs_data_t obs_source_settings(enum obs_source_type type, const char *id)
{
	const struct obs_source_info *info = get_source_info(type, id);
	return (info) ? get_defaults(info) : NULL;
}

obs_properties_t obs_get_source_properties(enum obs_source_type type,
		const char *id, const char *locale)
{
	const struct obs_source_info *info = get_source_info(type, id);
	if (info && info->properties) {
		obs_data_t       defaults = get_defaults(info);
		obs_properties_t properties;

		properties = info->properties(locale);
		obs_properties_apply_settings(properties, defaults);
		obs_data_release(defaults);
		return properties;
	}
	return NULL;
}

obs_properties_t obs_source_properties(obs_source_t source, const char *locale)
{
	if (source && source->info.properties) {
		obs_properties_t props;
		props = source->info.properties(locale);
		obs_properties_apply_settings(props, source->context.settings);
		return props;
	}

	return NULL;
}

uint32_t obs_source_get_output_flags(obs_source_t source)
{
	return source ? source->info.output_flags : 0;
}

static void obs_source_deferred_update(obs_source_t source)
{
	source->info.update(source->context.data, source->context.settings);
	source->defer_update = false;
}

void obs_source_update(obs_source_t source, obs_data_t settings)
{
	if (!source) return;

	obs_data_apply(source->context.settings, settings);

	if (source->info.update) {
		if (source->info.output_flags & OBS_SOURCE_VIDEO)
			source->defer_update = true;
		else
			source->info.update(source->context.data,
					source->context.settings);
	}
}

static void activate_source(obs_source_t source)
{
	if (source->info.activate)
		source->info.activate(source->context.data);
	obs_source_dosignal(source, "source_activate", "activate");
}

static void deactivate_source(obs_source_t source)
{
	if (source->info.deactivate)
		source->info.deactivate(source->context.data);
	obs_source_dosignal(source, "source_deactivate", "deactivate");
}

static void show_source(obs_source_t source)
{
	if (source->info.show)
		source->info.show(source->context.data);
	obs_source_dosignal(source, "source_show", "show");
}

static void hide_source(obs_source_t source)
{
	if (source->info.hide)
		source->info.hide(source->context.data);
	obs_source_dosignal(source, "source_hide", "hide");
}

static void activate_tree(obs_source_t parent, obs_source_t child, void *param)
{
	if (os_atomic_inc_long(&child->activate_refs) == 1)
		activate_source(child);

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

static void deactivate_tree(obs_source_t parent, obs_source_t child,
		void *param)
{
	if (os_atomic_dec_long(&child->activate_refs) == 0)
		deactivate_source(child);

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

static void show_tree(obs_source_t parent, obs_source_t child, void *param)
{
	if (os_atomic_inc_long(&child->show_refs) == 1)
		show_source(child);

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

static void hide_tree(obs_source_t parent, obs_source_t child, void *param)
{
	if (os_atomic_dec_long(&child->show_refs) == 0)
		hide_source(child);

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

void obs_source_activate(obs_source_t source, enum view_type type)
{
	if (!source) return;

	if (os_atomic_inc_long(&source->show_refs) == 1) {
		show_source(source);
		obs_source_enum_tree(source, show_tree, NULL);
	}

	if (type == MAIN_VIEW) {
		if (os_atomic_inc_long(&source->activate_refs) == 1) {
			activate_source(source);
			obs_source_enum_tree(source, activate_tree, NULL);
			obs_source_set_present_volume(source, 1.0f);
		}
	}
}

void obs_source_deactivate(obs_source_t source, enum view_type type)
{
	if (!source) return;

	if (os_atomic_dec_long(&source->show_refs) == 0) {
		hide_source(source);
		obs_source_enum_tree(source, hide_tree, NULL);
	}

	if (type == MAIN_VIEW) {
		if (os_atomic_dec_long(&source->activate_refs) == 0) {
			deactivate_source(source);
			obs_source_enum_tree(source, deactivate_tree, NULL);
			obs_source_set_present_volume(source, 0.0f);
		}
	}
}

void obs_source_video_tick(obs_source_t source, float seconds)
{
	if (!source) return;

	if (source->defer_update)
		obs_source_deferred_update(source);

	/* reset the filter render texture information once every frame */
	if (source->filter_texrender)
		texrender_reset(source->filter_texrender);

	if (source->info.video_tick)
		source->info.video_tick(source->context.data, seconds);
}

/* unless the value is 3+ hours worth of frames, this won't overflow */
static inline uint64_t conv_frames_to_time(size_t frames)
{
	const struct audio_output_info *info;
	info = audio_output_getinfo(obs->audio.audio);

	return (uint64_t)frames * 1000000000ULL /
		(uint64_t)info->samples_per_sec;
}

/* maximum "direct" timestamp variance in nanoseconds */
#define MAX_TS_VAR          5000000000ULL
/* maximum time that timestamp can jump in nanoseconds */
#define MAX_TIMESTAMP_JUMP  2000000000ULL
/* time threshold in nanoseconds to ensure audio timing is as seamless as
 * possible */
#define TS_SMOOTHING_THRESHOLD 70000000ULL

static inline void reset_audio_timing(obs_source_t source, uint64_t timetamp)
{
	source->timing_set    = true;
	source->timing_adjust = os_gettime_ns() - timetamp;
}

static inline void handle_ts_jump(obs_source_t source, uint64_t expected,
		uint64_t ts, uint64_t diff)
{
	blog(LOG_DEBUG, "Timestamp for source '%s' jumped by '%"PRIu64"', "
	                "expected value %"PRIu64", input value %"PRIu64,
	                source->context.name, diff, expected, ts);

	/* if has video, ignore audio data until reset */
	if (source->info.output_flags & OBS_SOURCE_ASYNC)
		os_atomic_dec_long(&source->audio_reset_ref);
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
		bool ts_under = (in.timestamp < source->next_audio_ts_min);

		diff = ts_under ?
			(source->next_audio_ts_min - in.timestamp) :
			(in.timestamp - source->next_audio_ts_min);

		/* smooth audio if lower or within threshold */
		if (diff > MAX_TIMESTAMP_JUMP)
			handle_ts_jump(source, source->next_audio_ts_min,
					in.timestamp, diff);
		else if (ts_under || diff < TS_SMOOTHING_THRESHOLD)
			in.timestamp = source->next_audio_ts_min;
	}

	source->next_audio_ts_min = in.timestamp +
		conv_frames_to_time(in.frames);

	if (source->audio_reset_ref != 0)
		return;

	in.timestamp += source->timing_adjust + source->sync_offset;
	in.volume = source->user_volume * source->present_volume *
		obs->audio.user_volume * obs->audio.present_volume;

	audio_line_output(source->audio_line, &in);
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
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		return CONVERT_NONE;
	}

	return CONVERT_NONE;
}

static inline bool set_packed422_sizes(struct obs_source *source,
		struct source_frame *frame)
{
	source->async_convert_height = frame->height;
	source->async_convert_width = frame->width / 2;
	return true;
}

static inline bool init_gpu_conversion(struct obs_source *source,
		struct source_frame *frame)
{
	switch (get_convert_type(frame->format)) {
		case CONVERT_422_Y:
		case CONVERT_422_U:
			return set_packed422_sizes(source, frame);

		case CONVERT_NV12:
		case CONVERT_420:
			/* TODO: implement conversion */
			break;

		case CONVERT_NONE:
			assert(false && "No conversion requested");
			break;

	}
	return false;
}

static inline bool set_async_texture_size(struct obs_source *source,
		struct source_frame *frame)
{
	enum convert_type prev, cur;
	prev = get_convert_type(source->async_format);
	cur  = get_convert_type(frame->format);
	if (source->async_texture) {
		if (source->async_width  == frame->width &&
		    source->async_height == frame->height &&
		    prev == cur)
			return true;
	}

	texture_destroy(source->async_texture);
	texrender_destroy(source->async_convert_texrender);
	source->async_convert_texrender = NULL;

	if (cur != CONVERT_NONE && init_gpu_conversion(source, frame)) {
		source->async_gpu_conversion = true;

		source->async_convert_texrender =
			texrender_create(GS_RGBA, GS_ZS_NONE);

		source->async_texture = gs_create_texture(
				source->async_convert_width,
				source->async_convert_height,
				GS_RGBA, 1, NULL, GS_DYNAMIC);

	} else {
		source->async_gpu_conversion = false;

		source->async_texture = gs_create_texture(
				frame->width, frame->height,
				GS_RGBA, 1, NULL, GS_DYNAMIC);
	}

	if (!source->async_texture)
		return false;

	source->async_width  = frame->width;
	source->async_height = frame->height;
	return true;
}

static void upload_raw_frame(texture_t tex, const struct source_frame *frame)
{
	switch (get_convert_type(frame->format)) {
		case CONVERT_422_U:
		case CONVERT_422_Y:
			texture_setimage(tex, frame->data[0],
					frame->linesize[0], false);
			break;

		case CONVERT_NV12:
		case CONVERT_420:
			assert(false && "Conversion not yet implemented");
			break;

		case CONVERT_NONE:
			assert(false && "No conversion requested");
			break;
	}
}

static const char *select_conversion_technique(enum video_format format)
{
	switch (format) {
		case VIDEO_FORMAT_UYVY:
			return "UYUV_Reverse";

		case VIDEO_FORMAT_YUY2:
			return "YUY2_Reverse";

		case VIDEO_FORMAT_YVYU:
			return "YVYU_Reverse";

		case VIDEO_FORMAT_NV12:
		case VIDEO_FORMAT_I420:
			assert(false && "Conversion not yet implemented");
			break;

		case VIDEO_FORMAT_BGRA:
		case VIDEO_FORMAT_BGRX:
		case VIDEO_FORMAT_RGBA:
		case VIDEO_FORMAT_NONE:
			assert(false && "No conversion requested");
			break;
	}
	return NULL;
}

static inline void set_eparam(effect_t effect, const char *name, float val)
{
	eparam_t param = effect_getparambyname(effect, name);
	effect_setfloat(effect, param, val);
}

static bool update_async_texrender(struct obs_source *source,
		const struct source_frame *frame)
{
	texture_t   tex       = source->async_texture;
	texrender_t texrender = source->async_convert_texrender;

	texrender_reset(texrender);

	upload_raw_frame(tex, frame);

	uint32_t cx = source->async_width;
	uint32_t cy = source->async_height;

	effect_t conv = obs->video.conversion_effect;
	technique_t tech = effect_gettechnique(conv,
			select_conversion_technique(frame->format));

	if (!texrender_begin(texrender, cx, cy))
		return false;

	technique_begin(tech);
	technique_beginpass(tech, 0);

	effect_settexture(conv, effect_getparambyname(conv, "image"),
			tex);
	set_eparam(conv, "width",  (float)cx);
	set_eparam(conv, "height", (float)cy);
	set_eparam(conv, "width_i",  1.0f / cx);
	set_eparam(conv, "height_i", 1.0f / cy);
	set_eparam(conv, "width_d2",  cx  * 0.5f);
	set_eparam(conv, "height_d2", cy * 0.5f);
	set_eparam(conv, "width_d2_i",  1.0f / (cx  * 0.5f));
	set_eparam(conv, "height_d2_i", 1.0f / (cy * 0.5f));
	set_eparam(conv, "input_height", (float)cy);

	gs_ortho(0.f, (float)cx, 0.f, (float)cy, -100.f, 100.f);

	gs_draw_sprite(tex, 0, cx, cy);

	technique_endpass(tech);
	technique_end(tech);

	texrender_end(texrender);

	return true;
}

static bool update_async_texture(struct obs_source *source,
		const struct source_frame *frame)
{
	texture_t         tex       = source->async_texture;
	texrender_t       texrender = source->async_convert_texrender;
	enum convert_type type      = get_convert_type(frame->format);
	void              *ptr;
	uint32_t          linesize;

	source->async_format     = frame->format;
	source->async_flip       = frame->flip;
	source->async_full_range = frame->full_range;
	memcpy(source->async_color_matrix, frame->color_matrix,
			sizeof(frame->color_matrix));
	memcpy(source->async_color_range_min, frame->color_range_min,
			sizeof frame->color_range_min);
	memcpy(source->async_color_range_max, frame->color_range_max,
			sizeof frame->color_range_max);

	if (source->async_gpu_conversion && texrender)
		return update_async_texrender(source, frame);

	if (type == CONVERT_NONE) {
		texture_setimage(tex, frame->data[0], frame->linesize[0],
				false);
		return true;
	}

	if (!texture_map(tex, &ptr, &linesize))
		return false;

	if (type == CONVERT_420)
		decompress_420((const uint8_t* const*)frame->data,
				frame->linesize,
				0, frame->height, ptr, linesize);

	else if (type == CONVERT_NV12)
		decompress_nv12((const uint8_t* const*)frame->data,
				frame->linesize,
				0, frame->height, ptr, linesize);

	else if (type == CONVERT_422_Y)
		decompress_422(frame->data[0], frame->linesize[0],
				0, frame->height, ptr, linesize, true);

	else if (type == CONVERT_422_U)
		decompress_422(frame->data[0], frame->linesize[0],
				0, frame->height, ptr, linesize, false);

	texture_unmap(tex);
	return true;
}

static inline void obs_source_draw_texture(struct obs_source *source,
		effect_t effect, float *color_matrix,
		float const *color_range_min, float const *color_range_max)
{
	texture_t tex = source->async_texture;
	eparam_t  param;

	if (source->async_convert_texrender)
		tex = texrender_gettexture(source->async_convert_texrender);

	if (color_range_min) {
		size_t const size = sizeof(float) * 3;
		param = effect_getparambyname(effect, "color_range_min");
		effect_setval(effect, param, color_range_min, size);
	}
	
	if (color_range_max) {
		size_t const size = sizeof(float) * 3;
		param = effect_getparambyname(effect, "color_range_max");
		effect_setval(effect, param, color_range_max, size);
	}
	
	if (color_matrix) {
		param = effect_getparambyname(effect, "color_matrix");
		effect_setval(effect, param, color_matrix, sizeof(float) * 16);
	}

	param = effect_getparambyname(effect, "image");
	effect_settexture(effect, param, tex);

	gs_draw_sprite(tex, source->async_flip ? GS_FLIP_V : 0, 0, 0);
}

static void obs_source_draw_async_texture(struct obs_source *source)
{
	effect_t    effect        = gs_geteffect();
	bool        yuv           = format_is_yuv(source->async_format);
	bool        limited_range = yuv && !source->async_full_range;
	const char  *type         = yuv ? "DrawMatrix" : "Draw";
	bool        def_draw      = (!effect);
	technique_t tech;

	if (def_draw) {
		effect = obs_get_default_effect();
		tech = effect_gettechnique(effect, type);
		technique_begin(tech);
		technique_beginpass(tech, 0);
	}

	obs_source_draw_texture(source, effect,
			yuv ? source->async_color_matrix : NULL,
			limited_range ? source->async_color_range_min : NULL,
			limited_range ? source->async_color_range_max : NULL);

	if (def_draw) {
		technique_endpass(tech);
		technique_end(tech);
	}
}

static void obs_source_render_async_video(obs_source_t source)
{
	struct source_frame *frame = obs_source_getframe(source);
	if (frame) {
		if (!set_async_texture_size(source, frame))
			return;
		if (!update_async_texture(source, frame))
			return;
	}

	if (source->async_texture)
		obs_source_draw_async_texture(source);

	obs_source_releaseframe(source, frame);
}

static inline void obs_source_render_filters(obs_source_t source)
{
	source->rendering_filter = true;
	obs_source_video_render(source->filters.array[0]);
	source->rendering_filter = false;
}

static inline void obs_source_default_render(obs_source_t source,
		bool color_matrix)
{
	effect_t    effect     = obs->video.default_effect;
	const char  *tech_name = color_matrix ? "DrawMatrix" : "Draw";
	technique_t tech       = effect_gettechnique(effect, tech_name);
	size_t      passes, i;

	passes = technique_begin(tech);
	for (i = 0; i < passes; i++) {
		technique_beginpass(tech, i);
		source->info.video_render(source->context.data, effect);
		technique_endpass(tech);
	}
	technique_end(tech);
}

static inline void obs_source_main_render(obs_source_t source)
{
	uint32_t flags      = source->info.output_flags;
	bool color_matrix   = (flags & OBS_SOURCE_COLOR_MATRIX) != 0;
	bool custom_draw    = (flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
	bool default_effect = !source->filter_parent &&
	                      source->filters.num == 0 &&
	                      !custom_draw;

	if (default_effect)
		obs_source_default_render(source, color_matrix);
	else
		source->info.video_render(source->context.data,
				custom_draw ? NULL : gs_geteffect());
}

void obs_source_video_render(obs_source_t source)
{
	if (!source) return;

	if (source->filters.num && !source->rendering_filter)
		obs_source_render_filters(source);

	else if (source->info.video_render)
		obs_source_main_render(source);

	else if (source->filter_target)
		obs_source_video_render(source->filter_target);

	else
		obs_source_render_async_video(source);
}

uint32_t obs_source_getwidth(obs_source_t source)
{
	if (!source) return 0;

	if (source->info.getwidth)
		return source->info.getwidth(source->context.data);
	return source->async_width;
}

uint32_t obs_source_getheight(obs_source_t source)
{
	if (!source) return 0;

	if (source->info.getheight)
		return source->info.getheight(source->context.data);
	return source->async_height;
}

obs_source_t obs_filter_getparent(obs_source_t filter)
{
	return filter ? filter->filter_parent : NULL;
}

obs_source_t obs_filter_gettarget(obs_source_t filter)
{
	return filter ? filter->filter_target : NULL;
}

void obs_source_filter_add(obs_source_t source, obs_source_t filter)
{
	if (!source || !filter)
		return;

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

	if (!source || !filter)
		return;

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
	size_t idx, i;

	if (!source || !filter)
		return;

	idx = da_find(source->filters, &filter, 0);
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
	if (!source) return NULL;

	obs_data_addref(source->context.settings);
	return source->context.settings;
}

static inline struct source_frame *filter_async_video(obs_source_t source,
		struct source_frame *in)
{
	size_t i;
	for (i = source->filters.num; i > 0; i--) {
		struct obs_source *filter = source->filters.array[i-1];
		if (filter->info.filter_video) {
			in = filter->info.filter_video(filter->context.data,
					in);
			if (!in)
				return NULL;
		}
	}

	return in;
}

static inline void copy_frame_data_line(struct source_frame *dst,
		const struct source_frame *src, uint32_t plane, uint32_t y)
{
	uint32_t pos_src = y * src->linesize[plane];
	uint32_t pos_dst = y * dst->linesize[plane];
	uint32_t bytes = dst->linesize[plane] < src->linesize[plane] ?
		dst->linesize[plane] : src->linesize[plane];

	memcpy(dst->data[plane] + pos_dst, src->data[plane] + pos_src, bytes);
}

static inline void copy_frame_data_plane(struct source_frame *dst,
		const struct source_frame *src, uint32_t plane, uint32_t lines)
{
	if (dst->linesize[plane] != src->linesize[plane])
		for (uint32_t y = 0; y < lines; y++)
			copy_frame_data_line(dst, src, plane, y);
	else
		memcpy(dst->data[plane], src->data[plane],
				dst->linesize[plane] * lines);
}

static void copy_frame_data(struct source_frame *dst,
		const struct source_frame *src)
{
	dst->flip         = src->flip;
	dst->full_range   = src->full_range;
	dst->timestamp    = src->timestamp;
	memcpy(dst->color_matrix, src->color_matrix, sizeof(float) * 16);
	if (!dst->full_range) {
		size_t const size = sizeof(float) * 3;
		memcpy(dst->color_range_min, src->color_range_min, size);
		memcpy(dst->color_range_max, src->color_range_max, size);
	}

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
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		copy_frame_data_plane(dst, src, 0, dst->height);
	}
}

static inline struct source_frame *cache_video(const struct source_frame *frame)
{
	/* TODO: use an actual cache */
	struct source_frame *new_frame = source_frame_create(frame->format,
			frame->width, frame->height);

	copy_frame_data(new_frame, frame);
	return new_frame;
}

void obs_source_output_video(obs_source_t source,
		const struct source_frame *frame)
{
	if (!source || !frame)
		return;

	struct source_frame *output = cache_video(frame);

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
		if (filter->info.filter_audio) {
			in = filter->info.filter_audio(filter->context.data,
					in);
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
		const uint8_t *const data[], uint32_t frames, uint64_t ts)
{
	size_t planes    = audio_output_planes(obs->audio.audio);
	size_t blocksize = audio_output_blocksize(obs->audio.audio);
	size_t size      = (size_t)frames * blocksize;
	bool   resize    = source->audio_storage_size < size;

	source->audio_data.frames    = frames;
	source->audio_data.timestamp = ts;

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
		uint8_t  *output[MAX_AV_PLANES];
		uint32_t frames;
		uint64_t offset;

		memset(output, 0, sizeof(output));

		audio_resampler_resample(source->resampler,
				output, &frames, &offset,
				audio->data, audio->frames);

		copy_audio_data(source, (const uint8_t *const *)output, frames,
				audio->timestamp - offset);
	} else {
		copy_audio_data(source, audio->data, audio->frames,
				audio->timestamp);
	}
}

void obs_source_output_audio(obs_source_t source,
		const struct source_audio *audio)
{
	uint32_t flags;
	struct filtered_audio *output;

	if (!source || !audio)
		return;

	flags = source->info.output_flags;
	process_audio(source, audio);

	pthread_mutex_lock(&source->filter_mutex);
	output = filter_async_audio(source, &source->audio_data);

	if (output) {
		bool async = (flags & OBS_SOURCE_ASYNC) != 0;

		pthread_mutex_lock(&source->audio_mutex);

		/* wait for video to start before outputting any audio so we
		 * have a base for sync */
		if (source->timing_set || !async) {
			struct audio_data data;

			for (int i = 0; i < MAX_AV_PLANES; i++)
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
	int      audio_time_refs = 0;
	uint64_t sys_time;

	if (!source)
		return NULL;

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
	if (source && frame) {
		source_frame_destroy(frame);
		obs_source_release(source);
	}
}

const char *obs_source_getname(obs_source_t source)
{
	return source ? source->context.name : NULL;
}

void obs_source_setname(obs_source_t source, const char *name)
{
	if (!source) return;
	obs_context_data_setname(&source->context, name);
}

void obs_source_gettype(obs_source_t source, enum obs_source_type *type,
		const char **id)
{
	if (!source) return;

	if (type) *type = source->info.type;
	if (id)   *id   = source->info.id;
}

static inline void render_filter_bypass(obs_source_t target, effect_t effect,
		bool use_matrix)
{
	const char  *tech_name = use_matrix ? "DrawMatrix" : "Draw";
	technique_t tech       = effect_gettechnique(effect, tech_name);
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
		uint32_t width, uint32_t height, bool use_matrix)
{
	const char  *tech_name = use_matrix ? "DrawMatrix" : "Draw";
	technique_t tech       = effect_gettechnique(effect, tech_name);
	eparam_t    image      = effect_getparambyname(effect, "image");
	size_t      passes, i;

	effect_settexture(effect, image, tex);

	passes = technique_begin(tech);
	for (i = 0; i < passes; i++) {
		technique_beginpass(tech, i);
		gs_draw_sprite(tex, width, height, 0);
		technique_endpass(tech);
	}
	technique_end(tech);
}

void obs_source_process_filter(obs_source_t filter, effect_t effect,
		uint32_t width, uint32_t height, enum gs_color_format format,
		enum allow_direct_render allow_direct)
{
	obs_source_t target, parent;
	uint32_t     target_flags, parent_flags;
	int          cx, cy;
	bool         use_matrix, expects_def, can_directly;

	if (!filter) return;

	target       = obs_filter_gettarget(filter);
	parent       = obs_filter_getparent(filter);
	target_flags = target->info.output_flags;
	parent_flags = parent->info.output_flags;
	cx           = obs_source_getwidth(target);
	cy           = obs_source_getheight(target);
	use_matrix   = !!(target_flags & OBS_SOURCE_COLOR_MATRIX);
	expects_def  = !(parent_flags & OBS_SOURCE_CUSTOM_DRAW);
	can_directly = allow_direct == ALLOW_DIRECT_RENDERING;

	/* if the parent does not use any custom effects, and this is the last
	 * filter in the chain for the parent, then render the parent directly
	 * using the filter effect instead of rendering to texture to reduce
	 * the total number of passes */
	if (can_directly && expects_def && target == parent) {
		render_filter_bypass(target, effect, use_matrix);
		return;
	}

	if (!filter->filter_texrender)
		filter->filter_texrender = texrender_create(format,
				GS_ZS_NONE);

	if (texrender_begin(filter->filter_texrender, cx, cy)) {
		gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);
		if (expects_def && parent == target)
			obs_source_default_render(parent, use_matrix);
		else
			obs_source_video_render(target);
		texrender_end(filter->filter_texrender);
	}

	/* --------------------------- */

	render_filter_tex(texrender_gettexture(filter->filter_texrender),
			effect, width, height, use_matrix);
}

signal_handler_t obs_source_signalhandler(obs_source_t source)
{
	return source ? source->context.signals : NULL;
}

proc_handler_t obs_source_prochandler(obs_source_t source)
{
	return source ? source->context.procs : NULL;
}

void obs_source_setvolume(obs_source_t source, float volume)
{
	if (source) {
		struct calldata data = {0};
		calldata_setptr(&data, "source", source);
		calldata_setfloat(&data, "volume", volume);

		signal_handler_signal(source->context.signals, "volume", &data);
		signal_handler_signal(obs->signals, "source_volume", &data);

		volume = (float)calldata_float(&data, "volume");
		calldata_free(&data);

		source->user_volume = volume;
	}
}

static void set_tree_preset_vol(obs_source_t parent, obs_source_t child,
		void *param)
{
	float *vol = param;
	child->present_volume = *vol;

	UNUSED_PARAMETER(parent);
}

void obs_source_set_present_volume(obs_source_t source, float volume)
{
	if (source) {
		source->present_volume = volume;

		/* don't set the presentation volume of the tree if a
		 * transition source, let the transition handle presentation
		 * volume for the child sources itself. */
		if (source->info.type != OBS_SOURCE_TYPE_TRANSITION)
			obs_source_enum_tree(source, set_tree_preset_vol,
					&volume);
	}
}

float obs_source_getvolume(obs_source_t source)
{
	return source ? source->user_volume : 0.0f;
}

float obs_source_get_present_volume(obs_source_t source)
{
	return source ? source->present_volume : 0.0f;
}

void obs_source_set_sync_offset(obs_source_t source, int64_t offset)
{
	if (source)
		source->sync_offset = offset;
}

int64_t obs_source_get_sync_offset(obs_source_t source)
{
	return source ? source->sync_offset : 0;
}

struct source_enum_data {
	obs_source_enum_proc_t enum_callback;
	void *param;
};

static void enum_source_tree_callback(obs_source_t parent, obs_source_t child,
		void *param)
{
	struct source_enum_data *data = param;

	if (child->info.enum_sources && !child->enum_refs) {
		os_atomic_inc_long(&child->enum_refs);

		child->info.enum_sources(child->context.data,
				enum_source_tree_callback, data);

		os_atomic_dec_long(&child->enum_refs);
	}

	data->enum_callback(parent, child, data->param);
}

void obs_source_enum_sources(obs_source_t source,
		obs_source_enum_proc_t enum_callback,
		void *param)
{
	if (!source || !source->info.enum_sources || source->enum_refs)
		return;

	obs_source_addref(source);

	os_atomic_inc_long(&source->enum_refs);
	source->info.enum_sources(source->context.data, enum_callback, param);
	os_atomic_dec_long(&source->enum_refs);

	obs_source_release(source);
}

void obs_source_enum_tree(obs_source_t source,
		obs_source_enum_proc_t enum_callback,
		void *param)
{
	struct source_enum_data data = {enum_callback, param};

	if (!source || !source->info.enum_sources || source->enum_refs)
		return;

	obs_source_addref(source);

	os_atomic_inc_long(&source->enum_refs);
	source->info.enum_sources(source->context.data,
			enum_source_tree_callback,
			&data);
	os_atomic_dec_long(&source->enum_refs);

	obs_source_release(source);
}

void obs_source_add_child(obs_source_t parent, obs_source_t child)
{
	if (!parent || !child) return;

	for (int i = 0; i < parent->show_refs; i++) {
		enum view_type type;
		type = (i < parent->activate_refs) ? MAIN_VIEW : AUX_VIEW;
		obs_source_activate(child, type);
	}
}

void obs_source_remove_child(obs_source_t parent, obs_source_t child)
{
	if (!parent || !child) return;

	for (int i = 0; i < parent->show_refs; i++) {
		enum view_type type;
		type = (i < parent->activate_refs) ? MAIN_VIEW : AUX_VIEW;
		obs_source_deactivate(child, type);
	}
}

static void reset_transition_vol(obs_source_t parent, obs_source_t child,
		void *param)
{
	child->transition_volume = 0.0f;

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

static void add_transition_vol(obs_source_t parent, obs_source_t child,
		void *param)
{
	float *vol = param;
	child->transition_volume += *vol;

	UNUSED_PARAMETER(parent);
}

static void apply_transition_vol(obs_source_t parent, obs_source_t child,
		void *param)
{
	child->present_volume = child->transition_volume;

	UNUSED_PARAMETER(parent);
	UNUSED_PARAMETER(param);
}

void obs_transition_begin_frame(obs_source_t transition)
{
	if (!transition) return;
	obs_source_enum_tree(transition, reset_transition_vol, NULL);
}

void obs_source_set_transition_vol(obs_source_t source, float vol)
{
	if (!source) return;

	add_transition_vol(NULL, source, &vol);
	obs_source_enum_tree(source, add_transition_vol, &vol);
}

void obs_transition_end_frame(obs_source_t transition)
{
	if (!transition) return;
	obs_source_enum_tree(transition, apply_transition_vol, NULL);
}

void obs_source_save(obs_source_t source)
{
	if (!source || !source->info.save) return;
	source->info.save(source->context.data, source->context.settings);
}

void obs_source_load(obs_source_t source)
{
	if (!source || !source->info.load) return;
	source->info.load(source->context.data, source->context.settings);
}
