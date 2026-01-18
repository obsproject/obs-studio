/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#pragma once

#include "obs.h"
#include "graphics/matrix4.h"

/* how obs scene! */

struct item_action {
	bool visible;
	uint64_t timestamp;
};

struct obs_scene_item {
	volatile long ref;
	volatile bool removed;

	bool is_group;
	bool is_scene;
	bool update_transform;
	bool update_group_resize;

	int64_t id;

	struct obs_scene *parent;
	struct obs_source *source;
	volatile long active_refs;
	volatile long defer_update;
	volatile long defer_group_resize;
	bool user_visible;
	bool visible;
	bool selected;
	bool locked;

	gs_texrender_t *item_render;
	struct obs_sceneitem_crop crop;

	bool absolute_coordinates;
	struct vec2 pos;
	struct vec2 scale;
	struct vec2 scale_ref;
	float rot;
	uint32_t align;

	/* last width/height of the source, this is used to check whether
	 * the transform needs updating */
	uint32_t last_width;
	uint32_t last_height;

	struct vec2 output_scale;
	enum obs_scale_type scale_filter;

	enum obs_blending_method blend_method;
	enum obs_blending_type blend_type;

	struct matrix4 box_transform;
	struct vec2 box_scale;
	struct matrix4 draw_transform;

	enum obs_bounds_type bounds_type;
	uint32_t bounds_align;
	struct vec2 bounds;
	bool crop_to_bounds;
	struct obs_sceneitem_crop bounds_crop;

	obs_hotkey_pair_id toggle_visibility;

	obs_data_t *private_settings;

	pthread_mutex_t actions_mutex;
	DARRAY(struct item_action) audio_actions;

	struct obs_source *show_transition;
	struct obs_source *hide_transition;
	uint32_t show_transition_duration;
	uint32_t hide_transition_duration;

	/* would do **prev_next, but not really great for reordering */
	struct obs_scene_item *prev;
	struct obs_scene_item *next;
};

struct obs_scene {
	struct obs_source *source;

	bool is_group;
	bool custom_size;
	uint32_t cx;
	uint32_t cy;

	bool absolute_coordinates;
	uint32_t last_width;
	uint32_t last_height;

	int64_t id_counter;

	pthread_mutex_t video_mutex;
	pthread_mutex_t audio_mutex;
	struct obs_scene_item *first_item;
};
