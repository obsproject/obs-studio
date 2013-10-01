#include "test-filter.h"

struct test_filter *test_create(const char *settings, source_t source)
{
	struct test_filter *tf = bmalloc(sizeof(struct test_filter));
	memset(tf, 0, sizeof(struct test_filter));

	tf->source = source;
	tf->whatever = gs_create_effect_from_file("test.effect", NULL);
	if (!tf->whatever) {
		test_destroy(tf);
		return NULL;
	}

	tf->texrender = texrender_create(GS_RGBA, GS_ZS_NONE);

	return tf;
}

void test_destroy(struct test_filter *tf)
{
	if (tf) {
		effect_destroy(tf->whatever);
		texrender_destroy(tf->texrender);
		bfree(tf);
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
	source_t filter_target = filter_gettarget(tf->source);
	int cx = source_getwidth(filter_target);
	int cy = source_getheight(filter_target);
	float fcx = (float)cx;
	float fcy = (float)cy;
	technique_t tech;
	texture_t tex;

	if (texrender_begin(tf->texrender, cx, cy)) {
		gs_ortho(0.0f, fcx, 0.0f, fcy, -100.0f, 100.0f);
		source_video_render(filter_target);
		texrender_end(tf->texrender);
	}

	/* --------------------------- */

	tex = texrender_gettexture(tf->texrender);
	tech = effect_gettechnique(tf->whatever, "Default");
	effect_settexture(tf->whatever, effect_getparambyidx(tf->whatever, 1),
			tex);
	technique_begin(tech);
	technique_beginpass(tech, 0);

	gs_draw_sprite(tex);

	technique_endpass(tech);
	technique_end(tech);
}
