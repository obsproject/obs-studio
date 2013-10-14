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

static void *scene_create(const char *settings, struct obs_source *source)
{
	struct obs_scene *scene = bmalloc(sizeof(struct obs_scene));
	scene->source = source;
	da_init(scene->items);

	return scene;
}

static void scene_destroy(void *data)
{
	struct obs_scene *scene = data;
	size_t i;

	for (i = 0; i < scene->items.num; i++)
		bfree(scene->items.array[i]);

	da_free(scene->items);
	bfree(scene);
}

static uint32_t scene_get_output_flags(void *data)
{
	return SOURCE_VIDEO | SOURCE_AUDIO;
}

static void scene_video_render(void *data)
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

		obs_source_video_render(item->source);

		gs_matrix_pop();
	}
}

static int scene_getsize(void *data)
{
	return -1;
}

static bool scene_enum_children(void *data, size_t idx, obs_source_t *child)
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
	scene_create,
	scene_destroy,
	scene_get_output_flags, NULL, NULL, NULL, NULL,
	scene_video_render,
	scene_getsize,
	scene_getsize, NULL, NULL,
	scene_enum_children, NULL, NULL
};
#else
static const struct source_info scene_info =
{
	.name             = "scene",
	.create           = scene_create,
	.destroy          = scene_destroy,
	.get_output_flags = scene_get_output_flags,
	.video_render     = scene_video_render,
	.getwidth         = scene_getsize,
	.getheight        = scene_getsize,
	.enum_children    = scene_enum_children
};
#endif

obs_scene_t obs_scene_create(void)
{
	struct obs_source *source = bmalloc(sizeof(struct obs_source));
	struct obs_scene  *scene  = scene_create(NULL, source);

	source->data = scene;
	if (!source->data) {
		bfree(source);
		return NULL;
	}

	scene->source = source;
	obs_source_init(source);
	memcpy(&source->callbacks, &scene_info, sizeof(struct source_info));
	return scene;
}

void obs_scene_destroy(obs_scene_t scene)
{
	if (scene)
		obs_source_destroy(scene->source);
}

obs_source_t obs_scene_getsource(obs_scene_t scene)
{
	return scene->source;
}

obs_sceneitem_t obs_scene_add(obs_scene_t scene, obs_source_t source)
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

void obs_sceneitem_remove(obs_sceneitem_t item)
{
	if (item) {
		da_erase_item(item->parent->items, item);
		bfree(item);
	}
}

void obs_sceneitem_setpos(obs_sceneitem_t item, const struct vec2 *pos)
{
	vec2_copy(&item->pos, pos);
}

void obs_sceneitem_setrot(obs_sceneitem_t item, float rot)
{
	item->rot = rot;
}

void obs_sceneitem_setorigin(obs_sceneitem_t item, const struct vec2 *origin)
{
	vec2_copy(&item->origin, origin);
}

void obs_sceneitem_setscale(obs_sceneitem_t item, const struct vec2 *scale)
{
	vec2_copy(&item->scale, scale);
}

void obs_sceneitem_setorder(obs_sceneitem_t item, enum order_movement movement)
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

void obs_sceneitem_getpos(obs_sceneitem_t item, struct vec2 *pos)
{
	vec2_copy(pos, &item->pos);
}

float obs_sceneitem_getrot(obs_sceneitem_t item)
{
	return item->rot;
}

void obs_sceneitem_getorigin(obs_sceneitem_t item, struct vec2 *origin)
{
	vec2_copy(origin, &item->origin);
}

void obs_sceneitem_getscale(obs_sceneitem_t item, struct vec2 *scale)
{
	vec2_copy(scale, &item->scale);
}
