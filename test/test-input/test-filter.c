#include "test-filter.h"

const char *test_getname(const char *locale)
{
	return "Test";
}

struct test_filter *test_create(const char *settings, obs_source_t source)
{
	struct test_filter *tf = bmalloc(sizeof(struct test_filter));
	char *effect_file;
	memset(tf, 0, sizeof(struct test_filter));

	gs_entercontext(obs_graphics());

	effect_file = obs_find_plugin_file("test-input/test.effect");

	tf->source = source;
	tf->whatever = gs_create_effect_from_file(effect_file, NULL);
	bfree(effect_file);
	if (!tf->whatever) {
		test_destroy(tf);
		return NULL;
	}

	tf->texrender = texrender_create(GS_RGBA, GS_ZS_NONE);

	gs_leavecontext();

	return tf;
}

void test_destroy(struct test_filter *tf)
{
	if (tf) {
		gs_entercontext(obs_graphics());

		effect_destroy(tf->whatever);
		texrender_destroy(tf->texrender);
		bfree(tf);

		gs_leavecontext();
	}
}

uint32_t test_get_output_flags(struct test_filter *tf)
{
	return SOURCE_VIDEO;
}

void test_video_tick(struct test_filter *tf, float seconds)
{
	texrender_reset(tf->texrender);
}

void test_video_render(struct test_filter *tf)
{
	obs_source_t filter_target = obs_filter_gettarget(tf->source);
	int cx = obs_source_getwidth(filter_target);
	int cy = obs_source_getheight(filter_target);
	float fcx = (float)cx;
	float fcy = (float)cy;
	technique_t tech;
	texture_t tex;

	if (texrender_begin(tf->texrender, cx, cy)) {
		gs_ortho(0.0f, fcx, 0.0f, fcy, -100.0f, 100.0f);
		obs_source_video_render(filter_target);
		texrender_end(tf->texrender);
	}

	/* --------------------------- */

	tex = texrender_gettexture(tf->texrender);
	tech = effect_gettechnique(tf->whatever, "Default");
	effect_settexture(tf->whatever, effect_getparambyidx(tf->whatever, 1),
			tex);
	technique_begin(tech);
	technique_beginpass(tech, 0);

	gs_draw_sprite(tex, 0, 0, 0);

	technique_endpass(tech);
	technique_end(tech);
}
