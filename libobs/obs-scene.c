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

#include "graphics/math-defs.h"
#include "obs-scene.h"

static void *obs_scene_create(const char *settings, struct obs_source *source)
{
	struct obs_scene *scene = bmalloc(sizeof(struct obs_scene));
	scene->source = source;
	da_init(scene->items);

	return scene;
}

static void obs_scene_destroy(void *data)
{
	struct obs_scene *scene = data;
	size_t i;

	for (i = 0; i < scene->items.num; i++)
		bfree(scene->items.array[i]);

	da_free(scene->items);
	bfree(scene);
}

static uint32_t obs_scene_get_output_flags(void *data)
{
	return SOURCE_VIDEO | SOURCE_AUDIO;
}

static void obs_scene_video_render(void *data)
{
	struct obs_scene *scene = data;
	size_t i;

	for (i = scene->items.num; i > 0; i--) {
		struct obs_scene_item *item = scene->items.array[i-1];

		gs_matrix_push();
		gs_matrix_translate3f(item->origin.x, item->origin.y, 0.0f);
		gs_matrix_scale3f(item->scale.x, item->scale.y, 1.0f);
		gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, RAD(-item->rot));
		gs_matrix_translate3f(-item->pos.x, -item->pos.y, 0.0f);

		source_video_render(item->source);

		gs_matrix_pop();
	}
}

static int obs_scene_getsize(void *data)
{
	return -1;
}

static bool obs_scene_enum_children(void *data, size_t idx, source_t *child)
{
	struct obs_scene *scene = data;
	if (idx >= scene->items.num)
		return false;

	*child = scene->items.array[idx]->source;
	return true;
}

/* thanks for being completely worthless, microsoft. */
#if 1
static const struct source_info scene_info =
{
	"scene",
	obs_scene_create,
	obs_scene_destroy,
	obs_scene_get_output_flags, NULL, NULL, NULL, NULL,
	obs_scene_video_render,
	obs_scene_getsize,
	obs_scene_getsize, NULL, NULL,
	obs_scene_enum_children, NULL, NULL
};
#else
static const struct source_info scene_info =
{
	.name             = "scene",
	.create           = obs_scene_create,
	.destroy          = obs_scene_destroy,
	.get_output_flags = obs_scene_get_output_flags,
	.video_render     = obs_scene_video_render,
	.getwidth         = obs_scene_getsize,
	.getheight        = obs_scene_getsize,
	.enum_children    = obs_scene_enum_children
};
#endif

scene_t scene_create(obs_t obs)
{
	struct obs_source *source = bmalloc(sizeof(struct obs_source));
	struct obs_scene  *scene  = obs_scene_create(NULL, source);

	source->data = scene;
	if (!source->data) {
		bfree(source);
		return NULL;
	}

	scene->source = source;
	source_init(obs, source);
	memcpy(&source->callbacks, &scene_info, sizeof(struct source_info));
	return scene;
}

void scene_destroy(scene_t scene)
{
	if (scene)
		source_destroy(scene->source);
}

source_t scene_source(scene_t scene)
{
	return scene->source;
}

sceneitem_t scene_add(scene_t scene, source_t source)
{
	struct obs_scene_item *item = bmalloc(sizeof(struct obs_scene_item));
	memset(item, 0, sizeof(struct obs_scene_item));
	item->source  = source;
	item->visible = true;
	item->parent  = scene;
	vec2_set(&item->scale, 1.0f, 1.0f);

	da_push_back(scene->items, &item);
	return item;
}

void sceneitem_remove(sceneitem_t item)
{
	if (item) {
		da_erase_item(item->parent->items, item);
		bfree(item);
	}
}

void sceneitem_setpos(sceneitem_t item, const struct vec2 *pos)
{
	vec2_copy(&item->pos, pos);
}

void sceneitem_setrot(sceneitem_t item, float rot)
{
	item->rot = rot;
}

void sceneitem_setorigin(sceneitem_t item, const struct vec2 *origin)
{
	vec2_copy(&item->origin, origin);
}

void sceneitem_setscale(sceneitem_t item, const struct vec2 *scale)
{
	vec2_copy(&item->scale, scale);
}

void sceneitem_setorder(sceneitem_t item, enum order_movement movement)
{
	struct obs_scene *scene = item->parent;

	if (movement == ORDER_MOVE_UP) {
		size_t idx = da_find(scene->items, &item, 0);
		if (idx > 0)
			da_move_item(scene->items, idx, idx-1);

	} else if (movement == ORDER_MOVE_DOWN) {
		size_t idx = da_find(scene->items, &item, 0);
		if (idx < (scene->items.num-1))
			da_move_item(scene->items, idx, idx+1);

	} else if (movement == ORDER_MOVE_TOP) {
		size_t idx = da_find(scene->items, &item, 0);
		if (idx > 0)
			da_move_item(scene->items, idx, 0);

	} else if (movement == ORDER_MOVE_TOP) {
		size_t idx = da_find(scene->items, &item, 0);
		if (idx < (scene->items.num-1))
			da_move_item(scene->items, idx, scene->items.num-1);
	}
}

void sceneitem_getpos(sceneitem_t item, struct vec2 *pos)
{
	vec2_copy(pos, &item->pos);
}

float sceneitem_getrot(sceneitem_t item)
{
	return item->rot;
}

void sceneitem_getorigin(sceneitem_t item, struct vec2 *origin)
{
	vec2_copy(origin, &item->origin);
}

void sceneitem_getscale(sceneitem_t item, struct vec2 *scale)
{
	vec2_copy(scale, &item->scale);
}
