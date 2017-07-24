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

#include "filter-blur.h"
#include "strings.h"

extern "C" {
#pragma warning (push)
#pragma warning (disable: 4201)
#include "libobs/util/platform.h"
#include "libobs/graphics/graphics.h"
#include "libobs/graphics/matrix4.h"
#pragma warning (pop)
}

#include <math.h>
#include <map>

enum ColorFormat : uint64_t {
	RGB,
	YUV, // 701
};

struct g_blurEffect {
	gs_effect_t* effect;
	std::vector<gs_texture_t*> kernels;
};
static g_blurEffect g_gaussianBlur, g_bilateralBlur;
static gs_effect_t* g_boxBlurEffect, *g_colorConversionEffect;

static size_t g_maxKernelSize = 25;

double_t gaussian1D(double_t x, double_t o) {
	return (1.0 / (o * sqrt(2 * M_PI))) * exp(-(x*x) / (2 * (o*o)));
}
double_t bilateral(double_t x, double_t o) {
	return 0.39894 * exp(-0.5 * (x * x) / (o * o)) / o;
}

Filter::Blur::Blur() {
	memset(&sourceInfo, 0, sizeof(obs_source_info));
	sourceInfo.id = "obs-stream-effects-filter-blur";
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

	obs_enter_graphics();

	// Blur Effects
	/// Box Blur
	{
		char* loadError = nullptr;
		char* file = obs_module_file("effects/box-blur.effect");
		g_boxBlurEffect = gs_effect_create_from_file(file, &loadError);
		bfree(file);
		if (loadError != nullptr) {
			PLOG_ERROR("<filter-blur> Loading box-blur effect failed with error(s): %s", loadError);
			bfree(loadError);
		} else if (!g_boxBlurEffect) {
			PLOG_ERROR("<filter-blur> Loading box-blur effect failed with unspecified error.");
		}
	}
	/// Gaussian Blur
	{
		gs_effect_t* effect;
		char* loadError = nullptr;
		char* file = obs_module_file("effects/gaussian-blur.effect");
		effect = gs_effect_create_from_file(file, &loadError);
		bfree(file);
		if (loadError != nullptr) {
			PLOG_ERROR("<filter-blur> Loading gaussian blur effect failed with error(s): %s", loadError);
			bfree(loadError);
		} else if (!effect) {
			PLOG_ERROR("<filter-blur> Loading gaussian blur effect failed with unspecified error.");
		} else {
			g_gaussianBlur.effect = effect;
			g_gaussianBlur.kernels.resize(g_maxKernelSize);
			std::vector<float_t> databuf;
			for (size_t n = 1; n <= g_maxKernelSize; n++) {
				databuf.resize(n);
				// Calculate
				float_t sum = 0.0;
				for (size_t p = 0; p < n; p ++) {
					databuf[p] = float_t(gaussian1D(double_t(p), double_t(n)));
					sum += databuf[p];
					if (p != 0)
						sum += databuf[p];
				}
				// Normalize
				for (size_t p = 0; p < n; p++) {
					databuf[p] /= sum;
				}

				uint8_t* data = reinterpret_cast<uint8_t*>(databuf.data());
				const uint8_t** pdata = const_cast<const uint8_t**>(&data);
				gs_texture_t* tex = gs_texture_create(uint32_t(n), 1, gs_color_format::GS_R32F, 1, pdata, 0);
				if (!tex) {
					PLOG_ERROR("<filter-blur> Failed to create gaussian kernel for %d width.", n);
				} else {
					g_gaussianBlur.kernels[n - 1] = tex;
				}
			}
		}
	}
	/// Bilateral Blur
	{
		gs_effect_t* effect;
		char* loadError = nullptr;
		char* file = obs_module_file("effects/bilateral-blur.effect");
		effect = gs_effect_create_from_file(file, &loadError);
		bfree(file);
		if (loadError != nullptr) {
			PLOG_ERROR("<filter-blur> Loading bilateral blur effect failed with error(s): %s", loadError);
			bfree(loadError);
		} else if (!effect) {
			PLOG_ERROR("<filter-blur> Loading bilateral blur effect failed with unspecified error.");
		} else {
			g_bilateralBlur.effect = effect;
			g_bilateralBlur.kernels.resize(g_maxKernelSize);
			std::vector<float_t> databuf;
			for (size_t n = 1; n <= g_maxKernelSize; n++) {
				databuf.resize(n);
				// Calculate
				float_t sum = 0.0;
				for (size_t p = 0; p < n; p++) {
					databuf[p] = float_t(gaussian1D(double_t(p), M_PI));
					sum += databuf[p];
					if (p != 0)
						sum += databuf[p];
				}
				// Normalize
				for (size_t p = 0; p < n; p++) {
					databuf[p] /= sum;
				}

				uint8_t* data = reinterpret_cast<uint8_t*>(databuf.data());
				const uint8_t** pdata = const_cast<const uint8_t**>(&data);
				gs_texture_t* tex = gs_texture_create(uint32_t(n), 1, gs_color_format::GS_R32F, 1, pdata, 0);
				if (!tex) {
					PLOG_ERROR("<filter-blur> Failed to create bilateral kernel for %d width.", n);
				} else {
					g_bilateralBlur.kernels[n - 1] = tex;
				}
			}
		}
	}

	// Color Conversion
	{
		char* loadError = nullptr;
		char* file = obs_module_file("effects/color-conversion.effect");
		g_colorConversionEffect = gs_effect_create_from_file(file, &loadError);
		bfree(file);
		if (loadError != nullptr) {
			PLOG_ERROR("<filter-blur> Loading color conversion effect failed with error(s): %s", loadError);
			bfree(loadError);
		} else if (!g_colorConversionEffect) {
			PLOG_ERROR("<filter-blur> Loading color conversion effect failed with unspecified error.");
		}
	}

	obs_leave_graphics();

	if (g_boxBlurEffect && g_gaussianBlur.effect && g_bilateralBlur.effect && g_colorConversionEffect)
		obs_register_source(&sourceInfo);
}

Filter::Blur::~Blur() {
	obs_enter_graphics();
	gs_effect_destroy(g_colorConversionEffect);
	gs_effect_destroy(g_bilateralBlur.effect);
	gs_effect_destroy(g_gaussianBlur.effect);
	for (size_t n = 1; n <= g_maxKernelSize; n++) {
		gs_texture_destroy(g_gaussianBlur.kernels[n - 1]);
	}
	gs_effect_destroy(g_boxBlurEffect);
	obs_leave_graphics();
}

const char * Filter::Blur::get_name(void *) {
	return P_TRANSLATE(P_FILTER_BLUR);
}

void Filter::Blur::get_defaults(obs_data_t *data) {
	obs_data_set_default_int(data, P_FILTER_BLUR_TYPE, Filter::Blur::Type::Box);
	obs_data_set_default_int(data, P_FILTER_BLUR_SIZE, 5);

	// Bilateral Only
	obs_data_set_default_double(data, P_FILTER_BLUR_BILATERAL_SMOOTHING, 50.0);
	obs_data_set_default_double(data, P_FILTER_BLUR_BILATERAL_SHARPNESS, 90.0);

	// Advanced
	obs_data_set_default_bool(data, P_FILTER_BLUR_ADVANCED, false);
	obs_data_set_default_int(data, P_FILTER_BLUR_ADVANCED_COLORFORMAT, ColorFormat::RGB);
}

obs_properties_t * Filter::Blur::get_properties(void *) {
	obs_properties_t *pr = obs_properties_create();
	obs_property_t* p = NULL;

	p = obs_properties_add_list(pr, P_FILTER_BLUR_TYPE, P_TRANSLATE(P_FILTER_BLUR_TYPE),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_set_long_description(p, P_TRANSLATE(P_DESC(P_FILTER_BLUR_TYPE)));
	obs_property_set_modified_callback(p, modified_properties);
	obs_property_list_add_int(p, P_TRANSLATE(P_FILTER_BLUR_TYPE_BOX),
		Filter::Blur::Type::Box);
	obs_property_list_add_int(p, P_TRANSLATE(P_FILTER_BLUR_TYPE_GAUSSIAN),
		Filter::Blur::Type::Gaussian);
	obs_property_list_add_int(p, P_TRANSLATE(P_FILTER_BLUR_TYPE_BILATERAL),
		Filter::Blur::Type::Bilateral);

	p = obs_properties_add_int_slider(pr, P_FILTER_BLUR_SIZE,
		P_TRANSLATE(P_FILTER_BLUR_SIZE), 1, 25, 1);
	obs_property_set_long_description(p, P_TRANSLATE(P_DESC(P_FILTER_BLUR_SIZE)));
	obs_property_set_modified_callback(p, modified_properties);

	// Bilateral Only
	p = obs_properties_add_float_slider(pr, P_FILTER_BLUR_BILATERAL_SMOOTHING,
		P_TRANSLATE(P_FILTER_BLUR_BILATERAL_SMOOTHING), 0.01, 100.0, 0.01);
	obs_property_set_long_description(p, P_TRANSLATE(P_DESC(P_FILTER_BLUR_BILATERAL_SMOOTHING)));
	p = obs_properties_add_float_slider(pr, P_FILTER_BLUR_BILATERAL_SHARPNESS,
		P_TRANSLATE(P_FILTER_BLUR_BILATERAL_SHARPNESS), 0, 99.99, 0.01);
	obs_property_set_long_description(p, P_TRANSLATE(P_DESC(P_FILTER_BLUR_BILATERAL_SHARPNESS)));

	// Advanced
	p = obs_properties_add_bool(pr, P_FILTER_BLUR_ADVANCED, P_TRANSLATE(P_FILTER_BLUR_ADVANCED));
	obs_property_set_long_description(p, P_TRANSLATE(P_DESC(P_FILTER_BLUR_ADVANCED)));
	obs_property_set_modified_callback(p, modified_properties);

	p = obs_properties_add_list(pr, P_FILTER_BLUR_ADVANCED_COLORFORMAT,
		P_TRANSLATE(P_FILTER_BLUR_ADVANCED_COLORFORMAT),
		obs_combo_type::OBS_COMBO_TYPE_LIST, obs_combo_format::OBS_COMBO_FORMAT_INT);
	obs_property_set_long_description(p,
		P_TRANSLATE(P_DESC(P_FILTER_BLUR_ADVANCED_COLORFORMAT)));
	obs_property_list_add_int(p, "RGB",
		ColorFormat::RGB);
	obs_property_list_add_int(p, "YUV",
		ColorFormat::YUV);

	return pr;
}

bool Filter::Blur::modified_properties(obs_properties_t *pr, obs_property_t *, obs_data_t *d) {
	bool showBilateral = false;

	switch (obs_data_get_int(d, P_FILTER_BLUR_TYPE)) {
		case Filter::Blur::Type::Box:
			break;
		case Filter::Blur::Type::Gaussian:
			break;
		case Filter::Blur::Type::Bilateral:
			showBilateral = true;
			break;
	}

	// Bilateral Blur
	obs_property_set_visible(obs_properties_get(pr, P_FILTER_BLUR_BILATERAL_SMOOTHING),
		showBilateral);
	obs_property_set_visible(obs_properties_get(pr, P_FILTER_BLUR_BILATERAL_SHARPNESS),
		showBilateral);

	// Advanced
	bool showAdvanced = false;
	if (obs_data_get_bool(d, P_FILTER_BLUR_ADVANCED))
		showAdvanced = true;

	obs_property_set_visible(obs_properties_get(pr, P_FILTER_BLUR_ADVANCED_COLORFORMAT),
		showAdvanced);

	return true;
}

void * Filter::Blur::create(obs_data_t *data, obs_source_t *source) {
	return new Instance(data, source);
}

void Filter::Blur::destroy(void *ptr) {
	delete reinterpret_cast<Instance*>(ptr);
}

uint32_t Filter::Blur::get_width(void *ptr) {
	return reinterpret_cast<Instance*>(ptr)->get_width();
}

uint32_t Filter::Blur::get_height(void *ptr) {
	return reinterpret_cast<Instance*>(ptr)->get_height();
}

void Filter::Blur::update(void *ptr, obs_data_t *data) {
	reinterpret_cast<Instance*>(ptr)->update(data);
}

void Filter::Blur::activate(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->activate();
}

void Filter::Blur::deactivate(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->deactivate();
}

void Filter::Blur::show(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->show();
}

void Filter::Blur::hide(void *ptr) {
	reinterpret_cast<Instance*>(ptr)->hide();
}

void Filter::Blur::video_tick(void *ptr, float time) {
	reinterpret_cast<Instance*>(ptr)->video_tick(time);
}

void Filter::Blur::video_render(void *ptr, gs_effect_t *effect) {
	reinterpret_cast<Instance*>(ptr)->video_render(effect);
}

Filter::Blur::Instance::Instance(obs_data_t *data, obs_source_t *context) : m_source(context) {
	obs_enter_graphics();
	m_effect = g_boxBlurEffect;
	m_primaryRT = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	m_secondaryRT = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	m_rtHorizontal = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	m_rtVertical = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	obs_leave_graphics();

	update(data);
}

Filter::Blur::Instance::~Instance() {
	obs_enter_graphics();
	gs_texrender_destroy(m_primaryRT);
	gs_texrender_destroy(m_secondaryRT);
	gs_texrender_destroy(m_rtHorizontal);
	gs_texrender_destroy(m_rtVertical);
	obs_leave_graphics();
}

void Filter::Blur::Instance::update(obs_data_t *data) {
	m_type = (Type)obs_data_get_int(data, P_FILTER_BLUR_TYPE);
	switch (m_type) {
		case Filter::Blur::Type::Box:
			m_effect = g_boxBlurEffect;
			break;
		case Filter::Blur::Type::Gaussian:
			m_effect = g_gaussianBlur.effect;
			break;
		case Filter::Blur::Type::Bilateral:
			m_effect = g_bilateralBlur.effect;
			break;
	}
	m_size = (uint64_t)obs_data_get_int(data, P_FILTER_BLUR_SIZE);

	// Bilateral Blur
	m_bilateralSmoothing = obs_data_get_double(data, P_FILTER_BLUR_BILATERAL_SMOOTHING) / 100.0;
	m_bilateralSharpness = obs_data_get_double(data, P_FILTER_BLUR_BILATERAL_SHARPNESS) / 100.0;

	// Advanced
	m_colorFormat = obs_data_get_int(data, P_FILTER_BLUR_ADVANCED_COLORFORMAT);
}

uint32_t Filter::Blur::Instance::get_width() {
	return 0;
}

uint32_t Filter::Blur::Instance::get_height() {
	return 0;
}

void Filter::Blur::Instance::activate() {}

void Filter::Blur::Instance::deactivate() {}

void Filter::Blur::Instance::show() {}

void Filter::Blur::Instance::hide() {}

void Filter::Blur::Instance::video_tick(float) {}

void Filter::Blur::Instance::video_render(gs_effect_t *effect) {
	bool failed = false;
	obs_source_t
		*parent = obs_filter_get_parent(m_source),
		*target = obs_filter_get_target(m_source);
	uint32_t
		baseW = obs_source_get_base_width(target),
		baseH = obs_source_get_base_height(target);

	// Skip rendering if our target, parent or context is not valid.
	if (!target || !parent || !m_source || !m_effect || !m_primaryRT || !m_technique
		|| !baseW || !baseH) {
		obs_source_skip_video_filter(m_source);
		return;
	}

	gs_effect_t* defaultEffect = obs_get_base_effect(obs_base_effect::OBS_EFFECT_DEFAULT);
	gs_texture_t *sourceTexture = nullptr;

#pragma region Source To Texture
	gs_texrender_reset(m_primaryRT);
	if (!gs_texrender_begin(m_primaryRT, baseW, baseH)) {
		PLOG_ERROR("<filter-blur> Failed to set up base texture.");
		obs_source_skip_video_filter(m_source);
		return;
	} else {
		gs_ortho(0, (float)baseW, 0, (float)baseH, -1, 1);

		// Clear to Black
		vec4 black;
		vec4_zero(&black);
		gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &black, 0, 0);

		// Set up camera stuff
		gs_set_cull_mode(GS_NEITHER);
		gs_reset_blend_state();
		gs_blend_function_separate(
			gs_blend_type::GS_BLEND_ONE,
			gs_blend_type::GS_BLEND_ZERO,
			gs_blend_type::GS_BLEND_ONE,
			gs_blend_type::GS_BLEND_ZERO);
		gs_enable_depth_test(false);
		gs_enable_stencil_test(false);
		gs_enable_stencil_write(false);
		gs_enable_color(true, true, true, true);

		// Render
		if (obs_source_process_filter_begin(m_source, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
			obs_source_process_filter_end(m_source, effect ? effect : defaultEffect, baseW, baseH);
		} else {
			PLOG_ERROR("<filter-blur> Unable to render source.");
			failed = true;
		}
		gs_texrender_end(m_primaryRT);
	}

	if (failed) {
		obs_source_skip_video_filter(m_source);
		return;
	}

	sourceTexture = gs_texrender_get_texture(m_primaryRT);
	if (!sourceTexture) {
		PLOG_ERROR("<filter-blur> Failed to get source texture.");
		obs_source_skip_video_filter(m_source);
		return;
	}
#pragma endregion Source To Texture

	// Conversion
	if (m_colorFormat == ColorFormat::YUV) {
		gs_texrender_reset(m_secondaryRT);
		if (!gs_texrender_begin(m_secondaryRT, baseW, baseH)) {
			PLOG_ERROR("<filter-blur> Failed to set up base texture.");
			obs_source_skip_video_filter(m_source);
			return;
		} else {
			gs_ortho(0, (float)baseW, 0, (float)baseH, -1, 1);

			// Clear to Black
			vec4 black;
			vec4_zero(&black);
			gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &black, 0, 0);

			// Set up camera stuff
			gs_set_cull_mode(GS_NEITHER);
			gs_reset_blend_state();
			gs_blend_function_separate(
				gs_blend_type::GS_BLEND_ONE,
				gs_blend_type::GS_BLEND_ZERO,
				gs_blend_type::GS_BLEND_ONE,
				gs_blend_type::GS_BLEND_ZERO);
			gs_enable_depth_test(false);
			gs_enable_stencil_test(false);
			gs_enable_stencil_write(false);
			gs_enable_color(true, true, true, true);

			gs_eparam_t* param = gs_effect_get_param_by_name(g_colorConversionEffect, "image");
			if (!param) {
				PLOG_ERROR("<filter-blur:Final> Failed to set image param.");
				failed = true;
			} else {
				gs_effect_set_texture(param, sourceTexture);
			}
			while (gs_effect_loop(g_colorConversionEffect, "RGBToYUV")) {
				gs_draw_sprite(sourceTexture, 0, baseW, baseH);
			}
			gs_texrender_end(m_secondaryRT);
		}

		if (failed) {
			obs_source_skip_video_filter(m_source);
			return;
		}

		sourceTexture = gs_texrender_get_texture(m_secondaryRT);
		if (!sourceTexture) {
			PLOG_ERROR("<filter-blur> Failed to get source texture.");
			obs_source_skip_video_filter(m_source);
			return;
		}
	}

	gs_texture_t *blurred = blur_render(sourceTexture, baseW, baseH);
	if (blurred == nullptr) {
		obs_source_skip_video_filter(m_source);
		return;
	}

	// Draw final effect
	{
		gs_effect_t* finalEffect = defaultEffect;
		const char* technique = "Draw";

		if (m_colorFormat == ColorFormat::YUV) {
			finalEffect = g_colorConversionEffect;
			technique = "YUVToRGB";
		}

		gs_eparam_t* param = gs_effect_get_param_by_name(finalEffect, "image");
		if (!param) {
			PLOG_ERROR("<filter-blur:Final> Failed to set image param.");
			failed = true;
		} else {
			gs_effect_set_texture(param, blurred);
		}
		while (gs_effect_loop(finalEffect, technique)) {
			gs_draw_sprite(blurred, 0, baseW, baseH);
		}
	}

	if (failed) {
		obs_source_skip_video_filter(m_source);
		return;
	}
}

gs_texture_t* Filter::Blur::Instance::blur_render(gs_texture_t* input, uint32_t baseW, uint32_t baseH) {
	bool failed = false;
	gs_texture_t *intermediate;

#pragma region Horizontal Pass
	gs_texrender_reset(m_rtHorizontal);
	if (!gs_texrender_begin(m_rtHorizontal, baseW, baseH)) {
		PLOG_ERROR("<filter-blur:Horizontal> Failed to initialize.");
		return nullptr;
	} else {
		gs_ortho(0, (float)baseW, 0, (float)baseH, -1, 1);

		// Clear to Black
		vec4 black;
		vec4_zero(&black);
		gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &black, 0, 0);

		// Set up camera stuff
		gs_set_cull_mode(GS_NEITHER);
		gs_reset_blend_state();
		gs_blend_function_separate(
			gs_blend_type::GS_BLEND_ONE,
			gs_blend_type::GS_BLEND_ZERO,
			gs_blend_type::GS_BLEND_ONE,
			gs_blend_type::GS_BLEND_ZERO);
		gs_enable_depth_test(false);
		gs_enable_stencil_test(false);
		gs_enable_stencil_write(false);
		gs_enable_color(true, true, true, true);

		// Prepare Effect
		if (!apply_effect_param(input, (float)(1.0 / baseW), 0)) {
			PLOG_ERROR("<filter-blur:Horizontal> Failed to set effect parameters.");
			failed = true;
		} else {
			while (gs_effect_loop(m_effect, "Draw")) {
				gs_draw_sprite(input, 0, baseW, baseH);
			}
		}
		gs_texrender_end(m_rtHorizontal);
	}

	if (failed) {
		return nullptr;
	}

	intermediate = gs_texrender_get_texture(m_rtHorizontal);
	if (!intermediate) {
		PLOG_ERROR("<filter-blur:Horizontal> Failed to get intermediate texture.");
		return nullptr;
	}
#pragma endregion Horizontal Pass

#pragma region Vertical Pass
	gs_texrender_reset(m_rtVertical);
	if (!gs_texrender_begin(m_rtVertical, baseW, baseH)) {
		PLOG_ERROR("<filter-blur:Vertical> Failed to initialize.");
		return nullptr;
	} else {
		gs_ortho(0, (float)baseW, 0, (float)baseH, -1, 1);

		// Clear to Black
		vec4 black;
		vec4_zero(&black);
		gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &black, 0, 0);

		// Set up camera stuff
		gs_set_cull_mode(GS_NEITHER);
		gs_reset_blend_state();
		gs_blend_function_separate(
			gs_blend_type::GS_BLEND_ONE,
			gs_blend_type::GS_BLEND_ZERO,
			gs_blend_type::GS_BLEND_ONE,
			gs_blend_type::GS_BLEND_ZERO);
		gs_enable_depth_test(false);
		gs_enable_stencil_test(false);
		gs_enable_stencil_write(false);
		gs_enable_color(true, true, true, true);

		// Prepare Effect
		if (!apply_effect_param(intermediate, 0, (float)(1.0 / baseH))) {
			PLOG_ERROR("<filter-blur:Vertical> Failed to set effect parameters.");
			failed = true;
		} else {
			while (gs_effect_loop(m_effect, "Draw")) {
				gs_draw_sprite(intermediate, 0, baseW, baseH);
			}
		}
		gs_texrender_end(m_rtVertical);
	}

	if (failed) {
		return nullptr;
	}

	intermediate = gs_texrender_get_texture(m_rtVertical);
	if (!intermediate) {
		PLOG_ERROR("<filter-blur:Vertical> Failed to get intermediate texture.");
		return nullptr;
	}
#pragma endregion Vertical Pass

	return intermediate;
}

bool Filter::Blur::Instance::apply_effect_param(gs_texture_t* texture, float uvTexelX, float uvTexelY) {
	gs_eparam_t *param;

	// UV Stepping
	param = gs_effect_get_param_by_name(m_effect, "texel");
	if (!param) {
		PLOG_ERROR("<filter-blur> Failed to set texel param.");
		return false;
	} else {
		PLOG_DEBUG("<filter-blur> Applying texel parameter.");
		vec2 texel;
		vec2_set(&texel, uvTexelX, uvTexelY);
		gs_effect_set_vec2(param, &texel);
	}

	// Filter Width
	param = gs_effect_get_param_by_name(m_effect, "widthHalf");
	if (!param) {
		PLOG_ERROR("<filter-blur> Failed to set widthHalf param.");
		return false;
	} else {
		PLOG_DEBUG("<filter-blur> Applying widthHalf parameter.");
		gs_effect_set_int(param, (int)m_size);
	}
	param = gs_effect_get_param_by_name(m_effect, "width");
	if (!param) {
		PLOG_ERROR("<filter-blur> Failed to set width param.");
		return false;
	} else {
		PLOG_DEBUG("<filter-blur> Applying width parameter.");
		gs_effect_set_int(param, (int)(1 + m_size * 2));
	}

	// Texture
	param = gs_effect_get_param_by_name(m_effect, "image");
	if (!param) {
		PLOG_ERROR("<filter-blur> Failed to set image param.");
		return false;
	} else {
		PLOG_DEBUG("<filter-blur> Applying image parameter.");
		gs_effect_set_texture(param, texture);
	}

	// Bilateral Blur
	if (m_type == Type::Bilateral) {
		param = gs_effect_get_param_by_name(m_effect, "bilateralSmoothing");
		if (!param) {
			PLOG_ERROR("<filter-blur> Failed to set bilateralSmoothing param.");
			return false;
		} else {
			PLOG_DEBUG("<filter-blur> Applying bilateralSmoothing parameter.");
			gs_effect_set_float(param,
				(float)(m_bilateralSmoothing * (1 + m_size * 2)));
		}

		param = gs_effect_get_param_by_name(m_effect, "bilateralSharpness");
		if (!param) {
			PLOG_ERROR("<filter-blur> Failed to set bilateralSmoothing param.");
			return false;
		} else {
			PLOG_DEBUG("<filter-blur> Applying bilateralSharpness parameter.");
			gs_effect_set_float(param, (float)(1.0 - m_bilateralSharpness));
		}
	}

	return true;
}
