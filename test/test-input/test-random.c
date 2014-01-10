#include <stdlib.h>
#include "test-random.h"

const char *random_getname(const char *locale)
{
	return "20x20 Random Pixel Texture Source (Test)";
}

struct random_tex *random_create(const char *settings, obs_source_t source)
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
			//pixel |= 0xFFFFFFFF;
			pixels[y*20 + x] = pixel;
		}
	}

	gs_entercontext(obs_graphics());

	rt->texture = gs_create_texture(20, 20, GS_RGBA, 1,
			(const void**)&pixels, 0);
	bfree(pixels);

	if (!rt->texture) {
		random_destroy(rt);
		return NULL;
	}

	gs_leavecontext();

	return rt;
}

void random_destroy(struct random_tex *rt)
{
	if (rt) {
		gs_entercontext(obs_graphics());

		texture_destroy(rt->texture);
		bfree(rt);

		gs_leavecontext();
	}
}

uint32_t random_get_output_flags(struct random_tex *rt)
{
	return SOURCE_VIDEO | SOURCE_DEFAULT_EFFECT;
}

void random_video_render(struct random_tex *rt, obs_source_t filter_target)
{
	effect_t effect  = gs_geteffect();
	eparam_t diffuse = effect_getparambyname(effect, "diffuse");

	effect_settexture(effect, diffuse, rt->texture);
	gs_draw_sprite(rt->texture, 0, 0, 0);
}

uint32_t random_getwidth(struct random_tex *rt)
{
	return texture_getwidth(rt->texture);
}

uint32_t random_getheight(struct random_tex *rt)
{
	return texture_getheight(rt->texture);
}
