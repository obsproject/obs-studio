/******************************************************************************
    Copyright (C) 2013-2015 by Hugh Bailey <obs.jim@gmail.com>
                               Philippe Groarke <philippe.groarke@gmail.com>

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

#include "util/threading.h"
#include "graphics/math-defs.h"
#include "obs-scene.h"

static const char *obs_scene_signals[] = {
	"void item_add(ptr scene, ptr item)",
	"void item_remove(ptr scene, ptr item)",
	"void reorder(ptr scene)",
	"void item_visible(ptr scene, ptr item, bool visible)",
	"void item_select(ptr scene, ptr item)",
	"void item_deselect(ptr scene, ptr item)",
	"void item_transform(ptr scene, ptr item)",
	NULL
};

static inline void signal_item_remove(struct obs_scene_item *item)
{
	struct calldata params = {0};
	calldata_set_ptr(&params, "scene", item->parent);
	calldata_set_ptr(&params, "item", item);

	signal_handler_signal(item->parent->source->context.signals,
			"item_remove", &params);
	calldata_free(&params);
}

static const char *scene_getname(void)
{
	/* TODO: locale */
	return "Scene";
}

static void *scene_create(obs_data_t *settings, struct obs_source *source)
{
	pthread_mutexattr_t attr;
	struct obs_scene *scene = bmalloc(sizeof(struct obs_scene));
	scene->source     = source;
	scene->first_item = NULL;

	signal_handler_add_array(obs_source_get_signal_handler(source),
			obs_scene_signals);

	if (pthread_mutexattr_init(&attr) != 0)
		goto fail;
	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		goto fail;
	if (pthread_mutex_init(&scene->mutex, &attr) != 0) {
		blog(LOG_ERROR, "scene_create: Couldn't initialize mutex");
		goto fail;
	}

	UNUSED_PARAMETER(settings);
	return scene;

fail:
	pthread_mutexattr_destroy(&attr);
	bfree(scene);
	return NULL;
}

static void remove_all_items(struct obs_scene *scene)
{
	struct obs_scene_item *item;

	pthread_mutex_lock(&scene->mutex);

	item = scene->first_item;

	while (item) {
		struct obs_scene_item *del_item = item;
		item = item->next;

		obs_sceneitem_remove(del_item);
	}

	pthread_mutex_unlock(&scene->mutex);
}

static void scene_destroy(void *data)
{
	struct obs_scene *scene = data;

	remove_all_items(scene);
	pthread_mutex_destroy(&scene->mutex);
	bfree(scene);
}

static void scene_enum_sources(void *data,
		obs_source_enum_proc_t enum_callback,
		void *param)
{
	struct obs_scene *scene = data;
	struct obs_scene_item *item;

	pthread_mutex_lock(&scene->mutex);

	item = scene->first_item;
	while (item) {
		struct obs_scene_item *next = item->next;

		obs_sceneitem_addref(item);
		enum_callback(scene->source, item->source, param);
		obs_sceneitem_release(item);

		item = next;
	}

	pthread_mutex_unlock(&scene->mutex);
}

static inline void detach_sceneitem(struct obs_scene_item *item)
{
	if (item->prev)
		item->prev->next = item->next;
	else
		item->parent->first_item = item->next;

	if (item->next)
		item->next->prev = item->prev;

	item->parent = NULL;
}

static inline void attach_sceneitem(struct obs_scene *parent,
		struct obs_scene_item *item, struct obs_scene_item *prev)
{
	item->prev   = prev;
	item->parent = parent;

	if (prev) {
		item->next = prev->next;
		if (prev->next)
			prev->next->prev = item;
		prev->next = item;
	} else {
		item->next = parent->first_item;
		if (parent->first_item)
			parent->first_item->prev = item;
		parent->first_item = item;
	}
}

static void add_alignment(struct vec2 *v, uint32_t align, int cx, int cy)
{
	if (align & OBS_ALIGN_RIGHT)
		v->x += (float)cx;
	else if ((align & OBS_ALIGN_LEFT) == 0)
		v->x += (float)(cx / 2);

	if (align & OBS_ALIGN_BOTTOM)
		v->y += (float)cy;
	else if ((align & OBS_ALIGN_TOP) == 0)
		v->y += (float)(cy / 2);
}

static void calculate_bounds_data(struct obs_scene_item *item,
		struct vec2 *origin, struct vec2 *scale,
		uint32_t *cx, uint32_t *cy)
{
	float    width         = (float)(*cx) * fabsf(scale->x);
	float    height        = (float)(*cy) * fabsf(scale->y);
	float    item_aspect   = width / height;
	float    bounds_aspect = item->bounds.x / item->bounds.y;
	uint32_t bounds_type   = item->bounds_type;
	float    width_diff, height_diff;

	if (item->bounds_type == OBS_BOUNDS_MAX_ONLY)
		if (width > item->bounds.x || height > item->bounds.y)
			bounds_type = OBS_BOUNDS_SCALE_INNER;

	if (bounds_type == OBS_BOUNDS_SCALE_INNER ||
	    bounds_type == OBS_BOUNDS_SCALE_OUTER) {
		bool  use_width = (bounds_aspect < item_aspect);
		float mul;

		if (item->bounds_type == OBS_BOUNDS_SCALE_OUTER)
			use_width = !use_width;

		mul = use_width ?
			item->bounds.x / width :
			item->bounds.y / height;

		vec2_mulf(scale, scale, mul);

	} else if (bounds_type == OBS_BOUNDS_SCALE_TO_WIDTH) {
		vec2_mulf(scale, scale, item->bounds.x / width);

	} else if (bounds_type == OBS_BOUNDS_SCALE_TO_HEIGHT) {
		vec2_mulf(scale, scale, item->bounds.y / height);

	} else if (bounds_type == OBS_BOUNDS_STRETCH) {
		scale->x = item->bounds.x / (float)(*cx);
		scale->y = item->bounds.y / (float)(*cy);
	}

	width       = (float)(*cx) * scale->x;
	height      = (float)(*cy) * scale->y;
	width_diff  = item->bounds.x - width;
	height_diff = item->bounds.y - height;
	*cx         = (uint32_t)item->bounds.x;
	*cy         = (uint32_t)item->bounds.y;

	add_alignment(origin, item->bounds_align,
			(int)-width_diff, (int)-height_diff);
}

static void update_item_transform(struct obs_scene_item *item)
{
	uint32_t        width         = obs_source_get_width(item->source);
	uint32_t        height        = obs_source_get_height(item->source);
	uint32_t        cx            = width;
	uint32_t        cy            = height;
	struct vec2     base_origin;
	struct vec2     origin;
	struct vec2     scale         = item->scale;
	struct calldata params        = {0};

	vec2_zero(&base_origin);
	vec2_zero(&origin);

	/* ----------------------- */

	if (item->bounds_type != OBS_BOUNDS_NONE) {
		calculate_bounds_data(item, &origin, &scale, &cx, &cy);
	} else {
		cx = (uint32_t)((float)cx * scale.x);
		cy = (uint32_t)((float)cy * scale.y);
	}

	add_alignment(&origin, item->align, (int)cx, (int)cy);

	matrix4_identity(&item->draw_transform);
	matrix4_scale3f(&item->draw_transform, &item->draw_transform,
			scale.x, scale.y, 1.0f);
	matrix4_translate3f(&item->draw_transform, &item->draw_transform,
			-origin.x, -origin.y, 0.0f);
	matrix4_rotate_aa4f(&item->draw_transform, &item->draw_transform,
			0.0f, 0.0f, 1.0f, RAD(item->rot));
	matrix4_translate3f(&item->draw_transform, &item->draw_transform,
			item->pos.x, item->pos.y, 0.0f);

	/* ----------------------- */

	if (item->bounds_type != OBS_BOUNDS_NONE) {
		vec2_copy(&scale, &item->bounds);
	} else {
		scale.x = (float)width  * item->scale.x;
		scale.y = (float)height * item->scale.y;
	}

	add_alignment(&base_origin, item->align, (int)scale.x, (int)scale.y);

	matrix4_identity(&item->box_transform);
	matrix4_scale3f(&item->box_transform, &item->box_transform,
			scale.x, scale.y, 1.0f);
	matrix4_translate3f(&item->box_transform, &item->box_transform,
			-base_origin.x, -base_origin.y, 0.0f);
	matrix4_rotate_aa4f(&item->box_transform, &item->box_transform,
			0.0f, 0.0f, 1.0f, RAD(item->rot));
	matrix4_translate3f(&item->box_transform, &item->box_transform,
			item->pos.x, item->pos.y, 0.0f);

	/* ----------------------- */

	item->last_width  = width;
	item->last_height = height;

	calldata_set_ptr(&params, "scene", item->parent);
	calldata_set_ptr(&params, "item", item);
	signal_handler_signal(item->parent->source->context.signals,
			"item_transform", &params);
	calldata_free(&params);
}

static inline bool source_size_changed(struct obs_scene_item *item)
{
	uint32_t width  = obs_source_get_width(item->source);
	uint32_t height = obs_source_get_height(item->source);

	return item->last_width != width || item->last_height != height;
}

static void scene_video_render(void *data, gs_effect_t *effect)
{
	struct obs_scene *scene = data;
	struct obs_scene_item *item;

	pthread_mutex_lock(&scene->mutex);

	item = scene->first_item;

	gs_blend_state_push();
	gs_reset_blend_state();

	while (item) {
		if (obs_source_removed(item->source)) {
			struct obs_scene_item *del_item = item;
			item = item->next;

			obs_sceneitem_remove(del_item);
			continue;
		}

		if (source_size_changed(item))
			update_item_transform(item);

		if (item->visible) {
			gs_matrix_push();
			gs_matrix_mul(&item->draw_transform);
			obs_source_video_render(item->source);
			gs_matrix_pop();
		}

		item = item->next;
	}

	gs_blend_state_pop();

	pthread_mutex_unlock(&scene->mutex);

	UNUSED_PARAMETER(effect);
}

static void scene_load_item(struct obs_scene *scene, obs_data_t *item_data)
{
	const char            *name = obs_data_get_string(item_data, "name");
	obs_source_t          *source = obs_get_source_by_name(name);
	struct obs_scene_item *item;

	if (!source) {
		blog(LOG_WARNING, "[scene_load_item] Source %s not found!",
				name);
		return;
	}

	item = obs_scene_add(scene, source);
	if (!item) {
		blog(LOG_WARNING, "[scene_load_item] Could not add source '%s' "
		                  "to scene '%s'!",
		                  name, obs_source_get_name(scene->source));
		
		obs_source_release(source);
		return;
	}

	obs_data_set_default_int(item_data, "align",
			OBS_ALIGN_TOP | OBS_ALIGN_LEFT);

	item->rot     = (float)obs_data_get_double(item_data, "rot");
	item->align   = (uint32_t)obs_data_get_int(item_data, "align");
	item->visible = obs_data_get_bool(item_data, "visible");
	obs_data_get_vec2(item_data, "pos",    &item->pos);
	obs_data_get_vec2(item_data, "scale",  &item->scale);

	item->bounds_type =
		(enum obs_bounds_type)obs_data_get_int(item_data,
				"bounds_type");
	item->bounds_align =
		(uint32_t)obs_data_get_int(item_data, "bounds_align");
	obs_data_get_vec2(item_data, "bounds", &item->bounds);

	obs_source_release(source);

	update_item_transform(item);
}

static void scene_load(void *scene, obs_data_t *settings)
{
	obs_data_array_t *items = obs_data_get_array(settings, "items");
	size_t           count, i;

	remove_all_items(scene);

	if (!items) return;

	count = obs_data_array_count(items);

	for (i = 0; i < count; i++) {
		obs_data_t *item_data = obs_data_array_item(items, i);
		scene_load_item(scene, item_data);
		obs_data_release(item_data);
	}

	obs_data_array_release(items);
}

static void scene_save_item(obs_data_array_t *array,
		struct obs_scene_item *item)
{
	obs_data_t *item_data = obs_data_create();
	const char *name     = obs_source_get_name(item->source);

	obs_data_set_string(item_data, "name",         name);
	obs_data_set_bool  (item_data, "visible",      item->visible);
	obs_data_set_double(item_data, "rot",          item->rot);
	obs_data_set_vec2 (item_data, "pos",          &item->pos);
	obs_data_set_vec2 (item_data, "scale",        &item->scale);
	obs_data_set_int   (item_data, "align",        (int)item->align);
	obs_data_set_int   (item_data, "bounds_type",  (int)item->bounds_type);
	obs_data_set_int   (item_data, "bounds_align", (int)item->bounds_align);
	obs_data_set_vec2 (item_data, "bounds",       &item->bounds);

	obs_data_array_push_back(array, item_data);
	obs_data_release(item_data);
}

static void scene_save(void *data, obs_data_t *settings)
{
	struct obs_scene      *scene = data;
	obs_data_array_t      *array  = obs_data_array_create();
	struct obs_scene_item *item;

	pthread_mutex_lock(&scene->mutex);

	item = scene->first_item;
	while (item) {
		scene_save_item(array, item);
		item = item->next;
	}

	pthread_mutex_unlock(&scene->mutex);

	obs_data_set_array(settings, "items", array);
	obs_data_array_release(array);
}

static uint32_t scene_getwidth(void *data)
{
	UNUSED_PARAMETER(data);
	return obs->video.base_width;
}

static uint32_t scene_getheight(void *data)
{
	UNUSED_PARAMETER(data);
	return obs->video.base_height;
}

const struct obs_source_info scene_info =
{
	.id            = "scene",
	.type          = OBS_SOURCE_TYPE_INPUT,
	.output_flags  = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.get_name      = scene_getname,
	.create        = scene_create,
	.destroy       = scene_destroy,
	.video_render  = scene_video_render,
	.get_width     = scene_getwidth,
	.get_height    = scene_getheight,
	.load          = scene_load,
	.save          = scene_save,
	.enum_sources  = scene_enum_sources
};

obs_scene_t *obs_scene_create(const char *name)
{
	struct obs_source *source =
		obs_source_create(OBS_SOURCE_TYPE_INPUT, "scene", name, NULL,
				NULL);
	return source->context.data;
}

void obs_scene_addref(obs_scene_t *scene)
{
	if (scene)
		obs_source_addref(scene->source);
}

void obs_scene_release(obs_scene_t *scene)
{
	if (scene)
		obs_source_release(scene->source);
}

obs_source_t *obs_scene_get_source(const obs_scene_t *scene)
{
	return scene ? scene->source : NULL;
}

obs_scene_t *obs_scene_from_source(const obs_source_t *source)
{
	if (!source || source->info.id != scene_info.id)
		return NULL;

	return source->context.data;
}

obs_sceneitem_t *obs_scene_find_source(obs_scene_t *scene, const char *name)
{
	struct obs_scene_item *item;

	if (!scene)
		return NULL;

	pthread_mutex_lock(&scene->mutex);

	item = scene->first_item;
	while (item) {
		if (strcmp(item->source->context.name, name) == 0)
			break;

		item = item->next;
	}

	pthread_mutex_unlock(&scene->mutex);

	return item;
}

void obs_scene_enum_items(obs_scene_t *scene,
		bool (*callback)(obs_scene_t*, obs_sceneitem_t*, void*),
		void *param)
{
	struct obs_scene_item *item;

	if (!scene || !callback)
		return;

	pthread_mutex_lock(&scene->mutex);

	item = scene->first_item;
	while (item) {
		struct obs_scene_item *next = item->next;

		obs_sceneitem_addref(item);

		if (!callback(scene, item, param)) {
			obs_sceneitem_release(item);
			break;
		}

		obs_sceneitem_release(item);

		item = next;
	}

	pthread_mutex_unlock(&scene->mutex);
}

obs_sceneitem_t *obs_scene_add(obs_scene_t *scene, obs_source_t *source)
{
	struct obs_scene_item *last;
	struct obs_scene_item *item;
	struct calldata params = {0};

	if (!scene)
		return NULL;

	if (!source) {
		blog(LOG_ERROR, "Tried to add a NULL source to a scene");
		return NULL;
	}

	if (!obs_source_add_child(scene->source, source)) {
		blog(LOG_WARNING, "Failed to add source to scene due to "
		                  "infinite source recursion");
		return NULL;
	}

	item = bzalloc(sizeof(struct obs_scene_item));
	item->source  = source;
	item->visible = true;
	item->parent  = scene;
	item->ref     = 1;
	item->align   = OBS_ALIGN_TOP | OBS_ALIGN_LEFT;
	vec2_set(&item->scale, 1.0f, 1.0f);
	matrix4_identity(&item->draw_transform);
	matrix4_identity(&item->box_transform);

	obs_source_addref(source);

	pthread_mutex_lock(&scene->mutex);

	last = scene->first_item;
	if (!last) {
		scene->first_item = item;
	} else {
		while (last->next)
			last = last->next;

		last->next = item;
		item->prev = last;
	}

	pthread_mutex_unlock(&scene->mutex);

	calldata_set_ptr(&params, "scene", scene);
	calldata_set_ptr(&params, "item", item);
	signal_handler_signal(scene->source->context.signals, "item_add",
			&params);
	calldata_free(&params);

	return item;
}

static void obs_sceneitem_destroy(obs_sceneitem_t *item)
{
	if (item) {
		if (item->source)
			obs_source_release(item->source);
		bfree(item);
	}
}

void obs_sceneitem_addref(obs_sceneitem_t *item)
{
	if (item)
		os_atomic_inc_long(&item->ref);
}

void obs_sceneitem_release(obs_sceneitem_t *item)
{
	if (!item)
		return;

	if (os_atomic_dec_long(&item->ref) == 0)
		obs_sceneitem_destroy(item);
}

void obs_sceneitem_remove(obs_sceneitem_t *item)
{
	obs_scene_t *scene;

	if (!item)
		return;

	scene = item->parent;

	if (scene)
		pthread_mutex_lock(&scene->mutex);

	if (item->removed) {
		if (scene)
			pthread_mutex_unlock(&scene->mutex);
		return;
	}

	item->removed = true;

	assert(scene != NULL);
	assert(scene->source != NULL);
	obs_source_remove_child(scene->source, item->source);

	signal_item_remove(item);
	detach_sceneitem(item);

	pthread_mutex_unlock(&scene->mutex);

	obs_sceneitem_release(item);
}

obs_scene_t *obs_sceneitem_get_scene(const obs_sceneitem_t *item)
{
	return item ? item->parent : NULL;
}

obs_source_t *obs_sceneitem_get_source(const obs_sceneitem_t *item)
{
	return item ? item->source : NULL;
}

void obs_sceneitem_select(obs_sceneitem_t *item, bool select)
{
	struct calldata params = {0};
	const char *command = select ? "item_select" : "item_deselect";

	if (!item || item->selected == select)
		return;

	item->selected = select;

	calldata_set_ptr(&params, "scene", item->parent);
	calldata_set_ptr(&params, "item",  item);
	signal_handler_signal(item->parent->source->context.signals,
			command, &params);

	calldata_free(&params);
}

bool obs_sceneitem_selected(const obs_sceneitem_t *item)
{
	return item ? item->selected : false;
}

void obs_sceneitem_set_pos(obs_sceneitem_t *item, const struct vec2 *pos)
{
	if (item) {
		vec2_copy(&item->pos, pos);
		update_item_transform(item);
	}
}

void obs_sceneitem_set_rot(obs_sceneitem_t *item, float rot)
{
	if (item) {
		item->rot = rot;
		update_item_transform(item);
	}
}

void obs_sceneitem_set_scale(obs_sceneitem_t *item, const struct vec2 *scale)
{
	if (item) {
		vec2_copy(&item->scale, scale);
		update_item_transform(item);
	}
}

void obs_sceneitem_set_alignment(obs_sceneitem_t *item, uint32_t alignment)
{
	if (item) {
		item->align = alignment;
		update_item_transform(item);
	}
}

static inline void signal_reorder(struct obs_scene_item *item)
{
	const char *command = NULL;
	struct calldata params = {0};

	command = "reorder";

	calldata_set_ptr(&params, "scene", item->parent);

	signal_handler_signal(item->parent->source->context.signals,
			command, &params);

	calldata_free(&params);
}

void obs_sceneitem_set_order(obs_sceneitem_t *item,
		enum obs_order_movement movement)
{
	if (!item) return;

	struct obs_scene_item *next, *prev;
	struct obs_scene *scene = item->parent;

	obs_scene_addref(scene);
	pthread_mutex_lock(&scene->mutex);

	next = item->next;
	prev = item->prev;

	detach_sceneitem(item);

	if (movement == OBS_ORDER_MOVE_DOWN) {
		attach_sceneitem(scene, item, prev ? prev->prev : NULL);

	} else if (movement == OBS_ORDER_MOVE_UP) {
		attach_sceneitem(scene, item, next ? next : prev);

	} else if (movement == OBS_ORDER_MOVE_TOP) {
		struct obs_scene_item *last = next;
		if (!last) {
			last = prev;
		} else {
			while (last->next)
				last = last->next;
		}

		attach_sceneitem(scene, item, last);

	} else if (movement == OBS_ORDER_MOVE_BOTTOM) {
		attach_sceneitem(scene, item, NULL);
	}

	signal_reorder(item);

	pthread_mutex_unlock(&scene->mutex);
	obs_scene_release(scene);
}

void obs_sceneitem_set_order_position(obs_sceneitem_t *item,
		int position)
{
	if (!item) return;

	struct obs_scene *scene = item->parent;
	struct obs_scene_item *next;

	obs_scene_addref(scene);
	pthread_mutex_lock(&scene->mutex);

	detach_sceneitem(item);
	next = scene->first_item;

	if (position == 0) {
		attach_sceneitem(scene, item, NULL);
	} else {
		for (int i = position; i > 1; --i) {
			if (next->next == NULL)
				break;
			next = next->next;
		}

		attach_sceneitem(scene, item, next);
	}

	signal_reorder(item);

	pthread_mutex_unlock(&scene->mutex);
	obs_scene_release(scene);
}

void obs_sceneitem_set_bounds_type(obs_sceneitem_t *item,
		enum obs_bounds_type type)
{
	if (item) {
		item->bounds_type = type;
		update_item_transform(item);
	}
}

void obs_sceneitem_set_bounds_alignment(obs_sceneitem_t *item,
		uint32_t alignment)
{
	if (item) {
		item->bounds_align = alignment;
		update_item_transform(item);
	}
}

void obs_sceneitem_set_bounds(obs_sceneitem_t *item, const struct vec2 *bounds)
{
	if (item) {
		item->bounds = *bounds;
		update_item_transform(item);
	}
}

void obs_sceneitem_get_pos(const obs_sceneitem_t *item, struct vec2 *pos)
{
	if (item)
		vec2_copy(pos, &item->pos);
}

float obs_sceneitem_get_rot(const obs_sceneitem_t *item)
{
	return item ? item->rot : 0.0f;
}

void obs_sceneitem_get_scale(const obs_sceneitem_t *item, struct vec2 *scale)
{
	if (item)
		vec2_copy(scale, &item->scale);
}

uint32_t obs_sceneitem_get_alignment(const obs_sceneitem_t *item)
{
	return item ? item->align : 0;
}

enum obs_bounds_type obs_sceneitem_get_bounds_type(const obs_sceneitem_t *item)
{
	return item ? item->bounds_type : OBS_BOUNDS_NONE;
}

uint32_t obs_sceneitem_get_bounds_alignment(const obs_sceneitem_t *item)
{
	return item ? item->bounds_align : 0;
}

void obs_sceneitem_get_bounds(const obs_sceneitem_t *item, struct vec2 *bounds)
{
	if (item)
		*bounds = item->bounds;
}

void obs_sceneitem_get_info(const obs_sceneitem_t *item,
		struct obs_transform_info *info)
{
	if (item && info) {
		info->pos              = item->pos;
		info->rot              = item->rot;
		info->scale            = item->scale;
		info->alignment        = item->align;
		info->bounds_type      = item->bounds_type;
		info->bounds_alignment = item->bounds_align;
		info->bounds           = item->bounds;
	}
}

void obs_sceneitem_set_info(obs_sceneitem_t *item,
		const struct obs_transform_info *info)
{
	if (item && info) {
		item->pos          = info->pos;
		item->rot          = info->rot;
		item->scale        = info->scale;
		item->align        = info->alignment;
		item->bounds_type  = info->bounds_type;
		item->bounds_align = info->bounds_alignment;
		item->bounds       = info->bounds;
		update_item_transform(item);
	}
}

void obs_sceneitem_get_draw_transform(const obs_sceneitem_t *item,
		struct matrix4 *transform)
{
	if (item)
		matrix4_copy(transform, &item->draw_transform);
}

void obs_sceneitem_get_box_transform(const obs_sceneitem_t *item,
		struct matrix4 *transform)
{
	if (item)
		matrix4_copy(transform, &item->box_transform);
}

bool obs_sceneitem_visible(const obs_sceneitem_t *item)
{
	return item ? item->visible : false;
}

void obs_sceneitem_set_visible(obs_sceneitem_t *item, bool visible)
{
	struct calldata cd = {0};

	if (!item)
		return;

	item->visible = visible;

	if (!item->parent)
		return;

	calldata_set_ptr(&cd, "scene", item->parent);
	calldata_set_ptr(&cd, "item", item);
	calldata_set_bool(&cd, "visible", visible);

	signal_handler_signal(item->parent->source->context.signals,
			"item_visible", &cd);

	calldata_free(&cd);
}
