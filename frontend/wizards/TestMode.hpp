#pragma once

#include <graphics/math-extra.h>
#include <obs.hpp>

class TestMode {
	obs_video_info ovi;
	OBSSource source[6];

	static void render_rand(void *, uint32_t cx, uint32_t cy)
	{
		gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
		gs_eparam_t *randomvals[3] = {gs_effect_get_param_by_name(solid, "randomvals1"),
					      gs_effect_get_param_by_name(solid, "randomvals2"),
					      gs_effect_get_param_by_name(solid, "randomvals3")};

		struct vec4 r;

		for (int i = 0; i < 3; i++) {
			vec4_set(&r, rand_float(true) * 100.0f, rand_float(true) * 100.0f,
				 rand_float(true) * 50000.0f + 10000.0f, 0.0f);
			gs_effect_set_vec4(randomvals[i], &r);
		}

		while (gs_effect_loop(solid, "Random"))
			gs_draw_sprite(nullptr, 0, cx, cy);
	}

public:
	inline TestMode()
	{
		obs_get_video_info(&ovi);
		obs_add_main_render_callback(render_rand, this);

		for (uint32_t i = 0; i < 6; i++) {
			source[i] = obs_get_output_source(i);
			obs_source_release(source[i]);
			obs_set_output_source(i, nullptr);
		}
	}

	inline ~TestMode()
	{
		for (uint32_t i = 0; i < 6; i++)
			obs_set_output_source(i, source[i]);

		obs_remove_main_render_callback(render_rand, this);
		obs_reset_video(&ovi);
	}

	inline void SetVideo(int cx, int cy, int fps_num, int fps_den)
	{
		obs_video_info newOVI = ovi;

		newOVI.output_width = (uint32_t)cx;
		newOVI.output_height = (uint32_t)cy;
		newOVI.fps_num = (uint32_t)fps_num;
		newOVI.fps_den = (uint32_t)fps_den;

		obs_reset_video(&newOVI);
	}
};
