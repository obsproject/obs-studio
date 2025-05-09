/******************************************************************************
    Copyright (C) 2025 by Dennis Sädtler <saedtler@twitch.tv>

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

#include "obs.h"
#include "obs-internal.h"
#include "obs-scene.h"

/* The primary canvas has static name/uuid. */
static const char *MAIN_CANVAS_NAME = "Main";
static const char *MAIN_CANVAS_UUID = "6c69626f-6273-4c00-9d88-c5136d61696e";
/* Internal flag to mark a canvas as removed */
static const uint32_t REMOVED = 1u << 31;

/*** Signals ***/

static const char *canvas_signals[] = {
	"void destroy(ptr canvas)",
	"void remove(ptr canvas)",
	"void video_reset(ptr canvas)",

	"void source_add(ptr canvas, ptr source)",
	"void source_remove(ptr canvas, ptr source)",
	"void source_rename(ptr source, string new_name, string prev_name)",

	"void rename(ptr source, string new_name, string prev_name)",

	"void channel_change(ptr canvas, int channel, in out ptr source, ptr prev_source)",

	NULL,
};

static inline void canvas_dosignal(obs_canvas_t *canvas, const char *signal_obs, const char *signal_source)
{
	struct calldata data;
	uint8_t stack[128];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "canvas", canvas);
	if (signal_obs)
		signal_handler_signal(obs->signals, signal_obs, &data);
	if (signal_source)
		signal_handler_signal(canvas->context.signals, signal_source, &data);
}

static inline void canvas_dosignal_source(const char *signal, obs_canvas_t *canvas, obs_source_t *source)
{
	struct calldata data;
	uint8_t stack[128];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "canvas", canvas);
	calldata_set_ptr(&data, "source", source);

	signal_handler_signal(canvas->context.signals, signal, &data);
}

/*** Reference Counting ***/

void obs_canvas_release(obs_canvas_t *canvas)
{
	if (!obs && canvas) {
		blog(LOG_WARNING, "Tried to release a canvas when the OBS core is shut down!");
		return;
	}

	if (!canvas)
		return;

	obs_weak_canvas_t *control = (obs_weak_canvas_t *)canvas->context.control;
	if (obs_ref_release(&control->ref)) {
		obs_canvas_destroy(canvas);
		obs_weak_canvas_release(control);
	}
}

void obs_weak_canvas_addref(obs_weak_canvas_t *weak)
{
	if (!weak)
		return;

	obs_weak_ref_addref(&weak->ref);
}

void obs_weak_canvas_release(obs_weak_canvas_t *weak)
{
	if (!weak)
		return;

	if (obs_weak_ref_release(&weak->ref))
		bfree(weak);
}

obs_canvas_t *obs_canvas_get_ref(obs_canvas_t *canvas)
{
	if (!canvas)
		return NULL;

	return obs_weak_canvas_get_canvas((obs_weak_canvas_t *)canvas->context.control);
}

obs_weak_canvas_t *obs_canvas_get_weak_canvas(obs_canvas_t *canvas)
{
	if (!canvas)
		return NULL;

	obs_weak_canvas_t *weak = (obs_weak_canvas_t *)canvas->context.control;
	obs_weak_canvas_addref(weak);
	return weak;
}

obs_canvas_t *obs_weak_canvas_get_canvas(obs_weak_canvas_t *weak)
{
	if (!weak)
		return NULL;

	if (obs_weak_ref_get_ref(&weak->ref))
		return weak->canvas;

	return NULL;
}

/*** Creation / Destruction ***/

static obs_canvas_t *obs_canvas_create_internal(const char *name, const char *uuid, struct obs_video_info *ovi,
						uint32_t flags, bool private)
{
	struct obs_canvas *canvas = bzalloc(sizeof(struct obs_canvas));
	canvas->flags = flags;

	if (!obs_context_data_init(&canvas->context, OBS_OBJ_TYPE_CANVAS, NULL, name, uuid, NULL, private))
		return NULL;

	if (!signal_handler_add_array(canvas->context.signals, canvas_signals)) {
		obs_context_data_free(&canvas->context);
		bfree(canvas);
		return NULL;
	}

	if (pthread_mutex_init_recursive(&canvas->sources_mutex) != 0) {
		obs_context_data_free(&canvas->context);
		bfree(canvas);
		return NULL;
	}

	obs_view_init(&canvas->view, flags & ACTIVATE ? MAIN_VIEW : AUX_VIEW);
	obs_context_init_control(&canvas->context, canvas, (obs_destroy_cb)obs_canvas_destroy);

	/* A canvas can be created without a mix. */
	if (ovi) {
		canvas->ovi = *ovi;
		canvas->mix = obs_create_video_mix(ovi);
		if (canvas->mix) {
			canvas->mix->view = &canvas->view;
			canvas->mix->mix_audio = (flags & MIX_AUDIO) != 0;

			pthread_mutex_lock(&obs->video.mixes_mutex);
			da_push_back(obs->video.mixes, &canvas->mix);
			pthread_mutex_unlock(&obs->video.mixes_mutex);
		}
	}

	obs_context_data_insert_uuid(&canvas->context, &obs->data.canvases_mutex, &obs->data.canvases);
	if (!private) {
		obs_context_data_insert_name(&canvas->context, &obs->data.canvases_mutex, &obs->data.named_canvases);
		canvas_dosignal(canvas, "canvas_create", NULL);
	}

	blog(LOG_DEBUG, "%scanvas '%s' (%s) created", private ? "private " : "", canvas->context.name,
	     canvas->context.uuid);

	return canvas;
}

obs_canvas_t *obs_create_main_canvas(void)
{
	const uint32_t main_flags = MAIN | PROGRAM;
	return obs_canvas_create_internal(MAIN_CANVAS_NAME, MAIN_CANVAS_UUID, NULL, main_flags, false);
}

obs_canvas_t *obs_canvas_create(const char *name, struct obs_video_info *ovi, uint32_t flags)
{
	flags &= ~MAIN; /* Prevent user from creating a MAIN canvas. */
	return obs_canvas_create_internal(name, NULL, ovi, flags, false);
}

obs_canvas_t *obs_canvas_create_private(const char *name, struct obs_video_info *ovi, uint32_t flags)
{
	flags &= ~MAIN; /* Prevent user from creating a MAIN canvas. */
	return obs_canvas_create_internal(name, NULL, ovi, flags, true);
}

void obs_canvas_destroy(obs_canvas_t *canvas)
{
	canvas_dosignal(canvas, "canvas_destroy", "destroy");

	obs_canvas_clear_mix(canvas);

	obs_source_t *source = canvas->sources;
	while (source) {
		/* Canvases can hold strong refs to scene sources, release them here. */
		if (canvas->flags & SCENE_REF && obs_source_is_scene(source))
			obs_source_release(source);

		source = source->context.hh.next;
	}

	obs_context_data_remove_uuid(&canvas->context, &obs->data.canvases_mutex, &obs->data.canvases);
	if (!canvas->context.private) {
		obs_context_data_remove_name(&canvas->context, &obs->data.canvases_mutex, &obs->data.named_canvases);
	}

	blog(LOG_DEBUG, "%scanvas '%s' (%s) destroyed", canvas->context.private ? "private " : "", canvas->context.name,
	     canvas->context.uuid);

	pthread_mutex_destroy(&canvas->sources_mutex);
	obs_context_data_free(&canvas->context);
	obs_view_free(&canvas->view);
	bfree(canvas);
}

/*** Saving / Loading ***/

obs_data_t *obs_save_canvas(obs_canvas_t *canvas)
{
	if (canvas->flags & (EPHEMERAL | REMOVED))
		return NULL;

	obs_data_t *canvas_data = obs_data_create();

	obs_data_set_string(canvas_data, "name", canvas->context.name);
	obs_data_set_string(canvas_data, "uuid", canvas->context.uuid);
	obs_data_set_bool(canvas_data, "private", canvas->context.private);
	obs_data_set_int(canvas_data, "flags", canvas->flags);

	return canvas_data;
}

obs_canvas_t *obs_load_canvas(obs_data_t *data)
{
	const char *name = obs_data_get_string(data, "name");
	const char *uuid = obs_data_get_string(data, "uuid");
	const bool private = obs_data_get_bool(data, "private");
	uint32_t flags = (uint32_t)obs_data_get_int(data, "flags");

	flags &= ~MAIN; /* Prevent user from creating a MAIN canvas. */
	return obs_canvas_create_internal(name, uuid, NULL, flags, private);
}

/*** Internal API ***/

/* Free canvas mix (if any) */
void obs_canvas_clear_mix(obs_canvas_t *canvas)
{
	if (!canvas->mix)
		return;

	pthread_mutex_lock(&obs->video.mixes_mutex);
	for (size_t i = 0; i < obs->video.mixes.num; i++) {
		struct obs_core_video_mix *mix = obs->video.mixes.array[i];
		if (mix == canvas->mix) {
			da_erase(obs->video.mixes, i);
			obs_free_video_mix(mix);
			break;
		}
	}
	pthread_mutex_unlock(&obs->video.mixes_mutex);

	canvas->mix = NULL;
}

/* Clear mixes attached to canvases */
void obs_free_canvas_mixes(void)
{
	pthread_mutex_lock(&obs->data.canvases_mutex);
	struct obs_context_data *ctx, *tmp;
	HASH_ITER (hh, (struct obs_context_data *)obs->data.canvases, ctx, tmp) {
		obs_canvas_t *canvas = (obs_canvas_t *)ctx;
		obs_canvas_clear_mix(canvas);
	}
	pthread_mutex_unlock(&obs->data.canvases_mutex);
}

bool obs_canvas_reset_video_internal(obs_canvas_t *canvas, struct obs_video_info *ovi)
{
	obs_canvas_clear_mix(canvas);

	if (ovi)
		canvas->ovi = *ovi;

	canvas->mix = obs_create_video_mix(&canvas->ovi);
	if (canvas->mix) {
		canvas->mix->view = &canvas->view;
		canvas->mix->mix_audio = (canvas->flags & MIX_AUDIO) != 0;

		pthread_mutex_lock(&obs->video.mixes_mutex);
		da_push_back(obs->video.mixes, &canvas->mix);
		pthread_mutex_unlock(&obs->video.mixes_mutex);
	}

	canvas_dosignal(canvas, "canvas_video_reset", "video_reset");

	return !!canvas->mix;
}

void obs_canvas_insert_source(obs_canvas_t *canvas, obs_source_t *source)
{
	if (canvas->flags & SCENE_REF && obs_source_is_scene(source))
		obs_source_get_ref(source);
	if (source->canvas)
		obs_canvas_remove_source(source);

	source->canvas = obs_canvas_get_weak_canvas(canvas);
	obs_context_data_insert_name(&source->context, &canvas->sources_mutex, &canvas->sources);
	canvas_dosignal_source("source_add", canvas, source);
}

static bool remove_groups_items_cb(obs_scene_t *scene, obs_sceneitem_t *item, void *param)
{
	UNUSED_PARAMETER(scene);

	obs_source_t *source = param;
	if (item->source == source)
		obs_sceneitem_remove(item);

	return true;
}

static bool remove_groups_enum_cb(void *param, obs_source_t *scene_source)
{
	obs_source_t *source = param;
	obs_scene_t *scene = obs_scene_from_source(scene_source);

	obs_scene_enum_items(scene, remove_groups_items_cb, source);
	return true;
}

void obs_canvas_remove_source(obs_source_t *source)
{
	obs_canvas_t *canvas = obs_weak_canvas_get_canvas(source->canvas);
	if (canvas) {
		obs_weak_canvas_release(source->canvas);
		obs_context_data_remove_name(&source->context, &canvas->sources_mutex, &canvas->sources);

		canvas_dosignal_source("source_remove", canvas, source);
		if (canvas->flags & SCENE_REF && obs_source_is_scene(source))
			obs_source_release(source);

		/* If source is a group, also remove it from all other scenes in the old canvas */
		if (obs_source_is_group(source))
			obs_canvas_enum_scenes(canvas, remove_groups_enum_cb, source);

		obs_canvas_release(canvas);
	}
	source->canvas = NULL;
}

void obs_canvas_rename_source(obs_source_t *source, const char *name)
{
	obs_canvas_t *canvas = obs_weak_canvas_get_canvas(source->canvas);
	if (canvas) {
		struct calldata data;
		char *prev_name = bstrdup(source->context.name);

		obs_context_data_setname_ht(&source->context, name, &canvas->sources);

		calldata_init(&data);
		calldata_set_ptr(&data, "source", source);
		calldata_set_string(&data, "new_name", source->context.name);
		calldata_set_string(&data, "prev_name", prev_name);

		signal_handler_signal(source->context.signals, "rename", &data);
		signal_handler_signal(canvas->context.signals, "source_rename", &data);
		if (canvas->flags & MAIN)
			signal_handler_signal(obs->signals, "source_rename", &data);

		calldata_free(&data);
		bfree(prev_name);

		obs_canvas_release(canvas);
	}
}

/*** Public Canvas Object API ***/

bool obs_canvas_reset_video(obs_canvas_t *canvas, struct obs_video_info *ovi)
{
	if (canvas->flags & MAIN || obs_video_active())
		return false;

	return obs_canvas_reset_video_internal(canvas, ovi);
}

video_t *obs_canvas_get_video(const obs_canvas_t *canvas)
{
	return canvas->mix ? canvas->mix->video : NULL;
}

bool obs_canvas_get_video_info(const obs_canvas_t *canvas, struct obs_video_info *ovi)
{
	if (!obs->video.graphics || !canvas->mix)
		return false;

	*ovi = canvas->ovi;
	return true;
}

signal_handler_t *obs_canvas_get_signal_handler(obs_canvas_t *canvas)
{
	return canvas->context.signals;
}

void obs_canvas_set_channel(obs_canvas_t *canvas, uint32_t channel, obs_source_t *source)
{
	assert(channel < MAX_CHANNELS);

	if (channel >= MAX_CHANNELS)
		return;

	struct obs_view *view = &canvas->view;

	pthread_mutex_lock(&view->channels_mutex);

	source = obs_source_get_ref(source);

	obs_source_t *prev_source = view->channels[channel];

	if (source == prev_source) {
		obs_source_release(source);
		pthread_mutex_unlock(&view->channels_mutex);
		return;
	}

	struct calldata params = {0};
	calldata_set_ptr(&params, "canvas", canvas);
	calldata_set_int(&params, "channel", channel);
	calldata_set_ptr(&params, "prev_source", prev_source);
	calldata_set_ptr(&params, "source", source);

	signal_handler_signal(canvas->context.signals, "channel_change", &params);
	if (canvas->flags & MAIN)
		signal_handler_signal(obs->signals, "channel_change", &params);

	/* For some reason the original implementation allows overriding the source from the callback,
	 * so just in case support that here as well. This isn't used anywhere in OBS itself. */
	calldata_get_ptr(&params, "source", &source);
	view->channels[channel] = source;

	calldata_free(&params);
	pthread_mutex_unlock(&view->channels_mutex);

	if (source)
		obs_source_activate(source, view->type);

	if (prev_source) {
		obs_source_deactivate(prev_source, view->type);
		obs_source_release(prev_source);
	}
}

obs_source_t *obs_canvas_get_channel(obs_canvas_t *canvas, uint32_t channel)
{
	return obs_view_get_source(&canvas->view, channel);
}

obs_scene_t *obs_canvas_scene_create(obs_canvas_t *canvas, const char *name)
{
	struct obs_source *source = obs_source_create_canvas(canvas, "scene", name, NULL, NULL);
	return source->context.data;
}

void obs_canvas_scene_remove(obs_scene_t *scene)
{
	obs_canvas_remove_source(scene->source);
}

void obs_canvas_set_name(obs_canvas_t *canvas, const char *name)
{
	if (!name || !*name)
		return;
	if (canvas->flags & MAIN) /* Do not allow renaming main canvases. */
		return;
	if (strcmp(name, canvas->context.name) == 0)
		return;

	char *prev_name = bstrdup(canvas->context.name);

	if (canvas->context.private)
		obs_context_data_setname(&canvas->context, name);
	else
		obs_context_data_setname_ht(&canvas->context, name, &obs->data.named_canvases);

	struct calldata data;
	calldata_init(&data);
	calldata_set_ptr(&data, "canvas", canvas);
	calldata_set_string(&data, "new_name", canvas->context.name);
	calldata_set_string(&data, "prev_name", prev_name);
	signal_handler_signal(canvas->context.signals, "rename", &data);

	if (!canvas->context.private)
		signal_handler_signal(obs->signals, "canvas_rename", &data);

	calldata_free(&data);
	bfree(prev_name);
}

const char *obs_canvas_get_name(const obs_canvas_t *canvas)
{
	return canvas->context.name;
}

const char *obs_canvas_get_uuid(const obs_canvas_t *canvas)
{
	return canvas->context.uuid;
}

uint32_t obs_canvas_get_flags(const obs_canvas_t *canvas)
{
	return canvas->flags;
}

static bool enum_move_cb(obs_scene_t *scene, obs_sceneitem_t *item, void *param)
{
	UNUSED_PARAMETER(scene);

	obs_canvas_t *dst = param;
	obs_source_t *source = item->source;

	if (obs_source_is_group(source)) {
		obs_canvas_remove_source(source);
		obs_canvas_insert_source(dst, source);
	}

	return true;
}

void obs_canvas_move_scene(obs_scene_t *scene, obs_canvas_t *dst)
{
	obs_source_t *source = scene->source;
	obs_canvas_remove_source(source);
	obs_canvas_insert_source(dst, source);

	/* Also move all groups within this scene */
	obs_scene_enum_items(scene, enum_move_cb, dst);
}

void obs_canvas_remove(obs_canvas_t *canvas)
{
	/* Do not allow removing the main canvas, or canvases already marked as removed. */
	if (canvas->flags & (REMOVED | MAIN))
		return;

	obs_canvas_t *c = obs_canvas_get_ref(canvas);
	if (c) {
		c->flags |= REMOVED;
		canvas_dosignal(c, "canvas_remove", "remove");
		obs_canvas_release(c);
	}
}

bool obs_canvas_removed(obs_canvas_t *canvas)
{
	return (canvas->flags & REMOVED) != 0;
}

bool obs_canvas_has_video(obs_canvas_t *canvas)
{
	return canvas->mix != NULL;
}

void obs_canvas_render(obs_canvas_t *canvas)
{
	obs_view_render(&canvas->view);
}
