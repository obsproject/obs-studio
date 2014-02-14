#include <stdlib.h>
#include <obs.h>

struct random_tex {
	texture_t texture;
};

static const char *random_getname(const char *locale)
{
	UNUSED_PARAMETER(locale);
	return "20x20 Random Pixel Texture Source (Test)";
}

static void random_destroy(void *data)
{
	struct random_tex *rt = data;

	if (rt) {
		gs_entercontext(obs_graphics());

		texture_destroy(rt->texture);
		bfree(rt);

		gs_leavecontext();
	}
}

static void *random_create(obs_data_t settings, obs_source_t source)
{
	struct random_tex *rt = bzalloc(sizeof(struct random_tex));
	uint32_t *pixels = bmalloc(20*20*4);
	size_t x, y;

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

	UNUSED_PARAMETER(settings);
	UNUSED_PARAMETER(source);
	return rt;
}

static void random_video_render(void *data, effect_t effect)
{
	struct random_tex *rt = data;
	eparam_t image = effect_getparambyname(effect, "image");
	effect_settexture(effect, image, rt->texture);
	gs_draw_sprite(rt->texture, 0, 0, 0);
}

static uint32_t random_getwidth(void *data)
{
	struct random_tex *rt = data;
	return texture_getwidth(rt->texture);
}

static uint32_t random_getheight(void *data)
{
	struct random_tex *rt = data;
	return texture_getheight(rt->texture);
}

struct obs_source_info test_random = {
	.id           = "random",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.getname      = random_getname,
	.create       = random_create,
	.destroy      = random_destroy,
	.video_render = random_video_render,
	.getwidth     = random_getwidth,
	.getheight    = random_getheight
};
