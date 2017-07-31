/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "filter-displacement.h"
#include "strings.h"

Filter::Displacement::Displacement() {
	memset(&sourceInfo, 0, sizeof(obs_source_info));
	sourceInfo.id = "obs-stream-effects-filter-displacement";
	sourceInfo.type = OBS_SOURCE_TYPE_FILTER;
	sourceInfo.output_flags = OBS_SOURCE_VIDEO;
	sourceInfo.get_name = get_name;
	sourceInfo.get_defaults = get_defaults;
	sourceInfo.get_properties = get_properties;

	sourceInfo.create = create;
	sourceInfo.destroy = destroy;
	sourceInfo.update = update;
	sourceInfo.activate = activate;
	sourceInfo.deactivate = deactivate;
	sourceInfo.show = show;
	sourceInfo.hide = hide;
	sourceInfo.video_tick = video_tick;
	sourceInfo.video_render = video_render;

	obs_register_source(&sourceInfo);
}

Filter::Displacement::~Displacement() {

}

const char * Filter::Displacement::get_name(void *) {
	return P_TRANSLATE(P_FILTER_DISPLACEMENT);
}

void * Filter::Displacement::create(obs_data_t *data, obs_source_t *source) {
	return new Instance(data, source);
}

void Filter::Displacement::destroy(void *ptr) {
	delete reinterpret_cast<Instance*>(ptr);
}

uint32_t Filter::Displacement::get_width(void *ptr) {
	return reinterpret_cast<Instance*>(ptr)->get_width();
}

uint32_t Filter::Displacement::get_height(void *ptr) {
	return reinterpret_cast<Instance*>(ptr)->get_height();
}

void Filter::Displacement::get_defaults(obs_data_t *data) {
	char* disp = obs_module_file("filter-displacement/neutral.png");
	obs_data_set_default_string(data, P_FILTER_DISPLACEMENT_FILE, disp);
	obs_data_set_default_double(data, P_FILTER_DISPLACEMENT_RATIO, 0);
	obs_data_set_default_double(data, P_FILTER_DISPLACEMENT_SCALE, 0);
	bfree(disp);
}

obs_properties_t * Filter::Displacement::get_properties(void *ptr) {
	obs_properties_t *pr = obs_properties_create();

	std::string path = "";
	if (ptr)
		path = reinterpret_cast<Instance*>(ptr)->get_file();

	obs_properties_add_path(pr, P_FILTER_DISPLACEMENT_FILE, P_TRANSLATE(P_FILTER_DISPLACEMENT_FILE), obs_path_type::OBS_PATH_FILE, P_TRANSLATE(P_FILTER_DISPLACEMENT_FILE_TYPES), path.c_str());
	obs_properties_add_float_slider(pr, P_FILTER_DISPLACEMENT_RATIO, P_TRANSLATE(P_FILTER_DISPLACEMENT_RATIO), 0, 1, 0.01);
	obs_properties_add_float_slider(pr, P_FILTER_DISPLACEMENT_SCALE, P_TRANSLATE(P_FILTER_DISPLACEMENT_SCALE), -1000, 1000, 0.01);
	return pr;
}

void Filter::Displacement::update(void *ptr, obs_data_t *data) {
	reinterpret_cast<Instance*>(ptr)->update(data);
}

void Filter::Displacement::activate(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->activate();
}

void Filter::Displacement::deactivate(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->deactivate();
}

void Filter::Displacement::show(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->show();
}

void Filter::Displacement::hide(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->hide();
}

void Filter::Displacement::video_tick(void *ptr, float time) {
	reinterpret_cast<Instance*>(ptr)->video_tick(time);
}

void Filter::Displacement::video_render(void *ptr, gs_effect_t *effect) {
	reinterpret_cast<Instance*>(ptr)->video_render(effect);
}

Filter::Displacement::Instance::Instance(obs_data_t *data, obs_source_t *context) {
	this->dispmap.texture = nullptr;
	this->dispmap.createTime = 0;
	this->dispmap.modifiedTime = 0;
	this->dispmap.size = 0;
	this->timer = 0;
	this->context = context;

	obs_enter_graphics();
	char* effectFile = obs_module_file("effects/displacement.effect");
	char* errorMessage = nullptr;
	this->customEffect = gs_effect_create_from_file(effectFile, &errorMessage);
	bfree(effectFile);
	if (errorMessage != nullptr) {
		PLOG_ERROR("%s", errorMessage);
		bfree(errorMessage);
	}
	obs_leave_graphics();

	update(data);
}

Filter::Displacement::Instance::~Instance() {
	obs_enter_graphics();
	gs_effect_destroy(customEffect);
	gs_texture_destroy(dispmap.texture);
	obs_leave_graphics();
}

void Filter::Displacement::Instance::update(obs_data_t *data) {
	updateDisplacementMap(obs_data_get_string(data, P_FILTER_DISPLACEMENT_FILE));

	distance = float_t(obs_data_get_double(data, P_FILTER_DISPLACEMENT_RATIO));
	vec2_set(&displacementScale,
		float_t(obs_data_get_double(data, P_FILTER_DISPLACEMENT_SCALE)),
		float_t(obs_data_get_double(data, P_FILTER_DISPLACEMENT_SCALE)));
}

uint32_t Filter::Displacement::Instance::get_width() {
	return 0;
}

uint32_t Filter::Displacement::Instance::get_height() {
	return 0;
}

void Filter::Displacement::Instance::activate() {}

void Filter::Displacement::Instance::deactivate() {}

void Filter::Displacement::Instance::show() {}

void Filter::Displacement::Instance::hide() {}

void Filter::Displacement::Instance::video_tick(float time) {
	timer += time;
	if (timer >= 1.0) {
		timer -= 1.0;
		updateDisplacementMap(dispmap.file);
	}
}

float interp(float a, float b, float v) {
	return (a * (1.0f - v)) + (b * v);
}

void Filter::Displacement::Instance::video_render(gs_effect_t *) {
	obs_source_t *parent = obs_filter_get_parent(context);
	obs_source_t *target = obs_filter_get_target(context);
	uint32_t
		baseW = obs_source_get_base_width(target),
		baseH = obs_source_get_base_height(target);

	// Skip rendering if our target, parent or context is not valid.
	if (!target || !parent || !context || !dispmap.texture
		|| !baseW || !baseH) {
		obs_source_skip_video_filter(context);
		return;
	}

	if (!obs_source_process_filter_begin(context, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING))
		return;

	gs_eparam_t *param;

	vec2 texelScale;
	vec2_set(&texelScale,
		interp((1.0f / baseW), 1.0f, distance),
		interp((1.0f / baseH), 1.0f, distance));
	param = gs_effect_get_param_by_name(customEffect, "texelScale");
	if (param)
		gs_effect_set_vec2(param, &texelScale);
	else
		PLOG_ERROR("Failed to set texel scale param.");

	param = gs_effect_get_param_by_name(customEffect, "displacementScale");
	if (param)
		gs_effect_set_vec2(param, &displacementScale);
	else
		PLOG_ERROR("Failed to set displacement scale param.");

	param = gs_effect_get_param_by_name(customEffect, "displacementMap");
	if (param)
		gs_effect_set_texture(param, dispmap.texture);
	else
		PLOG_ERROR("Failed to set texture param.");

	obs_source_process_filter_end(context, customEffect, baseW, baseH);
}

std::string Filter::Displacement::Instance::get_file() {
	return dispmap.file;
}

void Filter::Displacement::Instance::updateDisplacementMap(std::string file) {
	bool shouldUpdateTexture = false;

	// Different File
	if (file != dispmap.file) {
		dispmap.file = file;
		shouldUpdateTexture = true;
	} else { // Different Timestamps
		struct stat stats;
		if (os_stat(dispmap.file.c_str(), &stats) != 0) {
			shouldUpdateTexture = shouldUpdateTexture ||
			(dispmap.createTime != stats.st_ctime) ||
			(dispmap.modifiedTime != stats.st_mtime);
			dispmap.createTime = stats.st_ctime;
			dispmap.modifiedTime = stats.st_mtime;
		}
	}

	if (shouldUpdateTexture) {
		obs_enter_graphics();
		if (dispmap.texture) {
			gs_texture_destroy(dispmap.texture);
			dispmap.texture = nullptr;
		}
		if (os_file_exists(file.c_str()))
			dispmap.texture = gs_texture_create_from_file(dispmap.file.c_str());
		obs_leave_graphics();
	}
}
