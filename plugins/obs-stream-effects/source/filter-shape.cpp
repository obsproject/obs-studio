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

#include "filter-shape.h"
#include "strings.h"

extern "C" {
#pragma warning (push)
#pragma warning (disable: 4201)
#include "libobs/util/platform.h"
#include "libobs/graphics/graphics.h"
#include "libobs/graphics/matrix4.h"
#pragma warning (pop)
}

#include <string>
#include <vector>
#include <map>
#include <memory>

typedef std::pair<uint32_t, std::string> cacheKey;
typedef std::pair<std::string, std::string> cacheValue;
static std::map<cacheKey, cacheValue> cache;
static const uint32_t minimumPoints = 3;
static const uint32_t maximumPoints = 16;

static void initialize() {
	if (cache.size() != 0)
		return;
	for (uint32_t point = 0; point < maximumPoints; point++) {
		std::vector<char> handle(1024), name(1024);
		const char* vals[] = {
			P_SHAPE_POINT_X,
			P_SHAPE_POINT_Y,
			P_SHAPE_POINT_U,
			P_SHAPE_POINT_V
		};
		for (const char* v : vals) {
			snprintf(handle.data(), handle.size(), "%s.%" PRIu32, v, point);
			snprintf(name.data(), name.size(), P_TRANSLATE(v), point);
			cacheValue x = std::make_pair(std::string(handle.data()), std::string(name.data()));
			cache.insert(std::make_pair(std::make_pair(point, v), x));
		}
	}
}

Filter::Shape::Shape() {
	memset(&sourceInfo, 0, sizeof(obs_source_info));
	sourceInfo.id = "obs-stream-effects-filter-shape";
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

	// Disabled for the time being. 3D Transform is better for this.
	//obs_register_source(&sourceInfo);

	initialize();
}

Filter::Shape::~Shape() {

}

const char * Filter::Shape::get_name(void *) {
	return "Shape";
}

void Filter::Shape::get_defaults(obs_data_t *data) {
	obs_data_set_default_bool(data, P_SHAPE_LOOP, true);
	obs_data_set_default_int(data, P_SHAPE_POINTS, minimumPoints);

	for (uint32_t point = 0; point < maximumPoints; point++) {
		const char* vals[] = {
			P_SHAPE_POINT_X,
			P_SHAPE_POINT_Y,
			P_SHAPE_POINT_U,
			P_SHAPE_POINT_V
		};
		for (const char* v : vals) {
			auto strings = cache.find(std::make_pair(point, v));
			if (strings != cache.end()) {
				obs_data_set_default_double(data, strings->second.first.c_str(), 0);
			}
		}
	}
}

obs_properties_t * Filter::Shape::get_properties(void *) {
	obs_properties_t *pr = obs_properties_create();
	obs_property_t* p = NULL;

	p = obs_properties_add_bool(pr, P_SHAPE_LOOP, P_TRANSLATE(P_SHAPE_LOOP));
	obs_property_set_long_description(p, P_DESC(P_SHAPE_LOOP));

	p = obs_properties_add_list(pr, P_SHAPE_MODE, P_TRANSLATE(P_SHAPE_MODE),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_set_long_description(p, P_TRANSLATE(P_DESC(P_SHAPE_MODE)));
	obs_property_list_add_int(p, P_TRANSLATE(P_SHAPE_MODE_TRIS), GS_TRIS);
	obs_property_list_add_int(p, P_TRANSLATE(P_SHAPE_MODE_TRISTRIP), GS_TRISTRIP);

	p = obs_properties_add_int_slider(pr, P_SHAPE_POINTS, P_TRANSLATE(P_SHAPE_POINTS), minimumPoints, maximumPoints, 1);
	obs_property_set_long_description(p, P_DESC(P_SHAPE_POINTS));
	obs_property_set_modified_callback(p, modified_properties);

	for (uint32_t point = 0; point < maximumPoints; point++) {
		std::pair<const char*, const char*> vals[] = {
			{ P_SHAPE_POINT_X, P_TRANSLATE(P_DESC(P_SHAPE_POINT_X)) },
			{ P_SHAPE_POINT_Y, P_TRANSLATE(P_DESC(P_SHAPE_POINT_Y)) },
			{ P_SHAPE_POINT_U, P_TRANSLATE(P_DESC(P_SHAPE_POINT_U)) },
			{ P_SHAPE_POINT_V, P_TRANSLATE(P_DESC(P_SHAPE_POINT_V)) }
		};
		for (std::pair<const char*, const char*> v : vals) {
			auto strings = cache.find(std::make_pair(point, v.first));
			if (strings != cache.end()) {
				p = obs_properties_add_float_slider(pr, strings->second.first.c_str(), strings->second.second.c_str(), 0, 100.0, 0.01);
				obs_property_set_long_description(p, v.second);
			}
		}
	}

	return pr;
}

bool Filter::Shape::modified_properties(obs_properties_t *pr, obs_property_t *, obs_data_t *data) {
	uint32_t points = (uint32_t)obs_data_get_int(data, P_SHAPE_POINTS);
	for (uint32_t point = 0; point < maximumPoints; point++) {
		bool visible = point < points ? true : false;
		const char* vals[] = {
			P_SHAPE_POINT_X,
			P_SHAPE_POINT_Y,
			P_SHAPE_POINT_U,
			P_SHAPE_POINT_V
		};
		for (const char* v : vals) {
			auto strings = cache.find(std::make_pair(point, v));
			if (strings != cache.end()) {
				obs_property_set_visible(obs_properties_get(pr, strings->second.first.c_str()), visible);
			}
		}
	}
	return true;
}

void * Filter::Shape::create(obs_data_t *data, obs_source_t *source) {
	return new Instance(data, source);
}

void Filter::Shape::destroy(void *ptr) {
	delete reinterpret_cast<Instance*>(ptr);
}

uint32_t Filter::Shape::get_width(void *ptr) {
	return reinterpret_cast<Instance*>(ptr)->get_width();
}

uint32_t Filter::Shape::get_height(void *ptr) {
	return reinterpret_cast<Instance*>(ptr)->get_height();
}

void Filter::Shape::update(void *ptr, obs_data_t *data) {
	reinterpret_cast<Instance*>(ptr)->update(data);
}

void Filter::Shape::activate(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->activate();
}

void Filter::Shape::deactivate(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->deactivate();
}

void Filter::Shape::show(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->show();
}

void Filter::Shape::hide(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->hide();
}

void Filter::Shape::video_tick(void *ptr, float time) {
	reinterpret_cast<Instance*>(ptr)->video_tick(time);
}

void Filter::Shape::video_render(void *ptr, gs_effect_t *effect) {
	reinterpret_cast<Instance*>(ptr)->video_render(effect);
}

Filter::Shape::Instance::Instance(obs_data_t *data, obs_source_t *context)
	: context(context) {
	obs_enter_graphics();
	m_vertexHelper = new Helper::VertexBuffer(maximumPoints);
	m_vertexHelper->set_uv_layers(1);
	m_texRender = gs_texrender_create(GS_RGBA, GS_Z32F);
	obs_leave_graphics();

	update(data);
}

Filter::Shape::Instance::~Instance() {
	obs_enter_graphics();
	delete m_vertexHelper;
	obs_leave_graphics();
}

void Filter::Shape::Instance::update(obs_data_t *data) {
	uint32_t points = (uint32_t)obs_data_get_int(data, P_SHAPE_POINTS);
	m_vertexHelper->resize(points);
	for (uint32_t point = 0; point < points; point++) {
		Helper::Vertex& v = m_vertexHelper->at(point);
		{
			auto strings = cache.find(std::make_pair(point, P_SHAPE_POINT_X));
			if (strings != cache.end()) {
				v.position.x = (float)(obs_data_get_double(data, strings->second.first.c_str()) / 100.0);
			}
		}
		{
			auto strings = cache.find(std::make_pair(point, P_SHAPE_POINT_Y));
			if (strings != cache.end()) {
				v.position.y = (float)(obs_data_get_double(data, strings->second.first.c_str()) / 100.0);
			}
		}
		{
			auto strings = cache.find(std::make_pair(point, P_SHAPE_POINT_U));
			if (strings != cache.end()) {
				v.uv[0].x = (float)(obs_data_get_double(data, strings->second.first.c_str()) / 100.0);
			}
		}
		{
			auto strings = cache.find(std::make_pair(point, P_SHAPE_POINT_V));
			if (strings != cache.end()) {
				v.uv[0].y = (float)(obs_data_get_double(data, strings->second.first.c_str()) / 100.0);
			}
		}
		v.color = 0xFFFFFFFF;
		v.position.z = 0.0f;
	}
	drawmode = (gs_draw_mode)obs_data_get_int(data, P_SHAPE_MODE);
	obs_enter_graphics();
	m_vertexBuffer = m_vertexHelper->update();
	obs_leave_graphics();
}

uint32_t Filter::Shape::Instance::get_width() {
	return 0;
}

uint32_t Filter::Shape::Instance::get_height() {
	return 0;
}

void Filter::Shape::Instance::activate() {

}

void Filter::Shape::Instance::deactivate() {

}

void Filter::Shape::Instance::show() {

}

void Filter::Shape::Instance::hide() {

}

void Filter::Shape::Instance::video_tick(float) {

}

void Filter::Shape::Instance::video_render(gs_effect_t *effect) {
	obs_source_t *parent = obs_filter_get_parent(context);
	obs_source_t *target = obs_filter_get_target(context);
	uint32_t
		baseW = obs_source_get_base_width(target),
		baseH = obs_source_get_base_height(target);

	// Skip rendering if our target, parent or context is not valid.
	if (!target || !parent || !context || !m_vertexBuffer
		|| !m_texRender || !baseW || !baseH) {
		obs_source_skip_video_filter(context);
		return;
	}

	gs_texrender_reset(m_texRender);
	if (gs_texrender_begin(m_texRender, baseW, baseH)) {
		if (obs_source_process_filter_begin(context, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
			obs_source_process_filter_end(context, effect ? effect : obs_get_base_effect(OBS_EFFECT_OPAQUE), baseW, baseH);
		} else {
			obs_source_skip_video_filter(context);
		}
		gs_texrender_end(m_texRender);
	} else {
		obs_source_skip_video_filter(context);
		return;
	}
	gs_texture* tex = gs_texrender_get_texture(m_texRender);

	//gs_projection_push();
	//gs_viewport_push();

	matrix4 alignedMatrix;
	gs_matrix_get(&alignedMatrix);
	gs_matrix_push();
	gs_matrix_set(&alignedMatrix);
	gs_matrix_scale3f((float)baseW, (float)baseH, 1.0);

	gs_set_cull_mode(GS_NEITHER);
	gs_enable_blending(false);
	gs_enable_depth_test(false);
	gs_enable_stencil_test(false);
	gs_enable_stencil_write(false);
	gs_enable_color(true, true, true, true);
	gs_enable_depth_test(false);

	gs_effect_t* eff = obs_get_base_effect(OBS_EFFECT_OPAQUE);
	while (gs_effect_loop(eff, "Draw")) {
		gs_effect_set_texture(gs_effect_get_param_by_name(eff, "image"), tex);
		gs_load_vertexbuffer(m_vertexBuffer);
		gs_load_indexbuffer(nullptr);
		gs_draw(drawmode, 0, (uint32_t)m_vertexHelper->size());
	}

	gs_matrix_pop();
	//gs_viewport_pop();
	//gs_projection_pop();
}
