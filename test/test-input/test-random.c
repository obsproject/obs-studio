#include <stdlib.h>
#include "test-random.h"

struct random_tex *random_create(const char *settings, source_t source)
{
	struct random_tex *rt = bmalloc(sizeof(struct random_tex));
	uint32_t *pixels = bmalloc(20*20*4);
	size_t x, y;

	memset(rt, 0, sizeof(struct random_tex));

	for (y = 0; y < 20; y++) {
		for (x = 0; x < 20; x++) {
			uint32_t pixel = 0xFF000000;
			pixel |= (rand()%256);
			pixel |= (rand()%256) << 8;
			pixel |= (rand()%256) << 16;
			pixels[y*20 + x] = pixel;
		}
	}

	rt->texture = gs_create_texture(20, 20, GS_RGBA, pixels, 0);
	bfree(pixels);

	if (!rt->texture) {
		random_destroy(rt);
		return NULL;
	}

	rt->whatever = gs_create_effect_from_file("draw.effect", NULL);
	if (!rt->whatever) {
		random_destroy(rt);
		return NULL;
	}

	return rt;
}

void random_destroy(struct random_tex *rt)
{
	if (rt) {
		effect_destroy(rt->whatever);
		texture_destroy(rt->texture);
		bfree(rt);
	}
}

uint32_t random_get_output_flags(struct random_tex *rt)
{
	return SOURCE_VIDEO;
}

void random_video_render(struct random_tex *rt, source_t filter_target)
{
	technique_t tech = effect_gettechnique(rt->whatever, "Default");
	effect_settexture(rt->whatever, effect_getparambyidx(rt->whatever, 1), rt->texture);
	technique_begin(tech);
	technique_beginpass(tech, 0);

	gs_draw_sprite(rt->texture);

	technique_endpass(tech);
	technique_end(tech);
}

int random_getwidth(struct random_tex *rt)
{
	return texture_getwidth(rt->texture);
}

int random_getheight(struct random_tex *rt)
{
	return texture_getheight(rt->texture);
}
