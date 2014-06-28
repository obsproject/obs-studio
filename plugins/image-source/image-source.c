#include <obs-module.h>

#define warn(format, ...) \
	blog(LOG_WARNING, "[image_source: '%s'] " format, \
			obs_source_getname(context->source), ##__VA_ARGS__)

struct image_source {
	obs_source_t source;

	texture_t    tex;
	uint32_t     cx;
	uint32_t     cy;
};

static const char *image_source_get_name(void)
{
	/* TODO: locale */
	return "Image";
}

static void image_source_update(void *data, obs_data_t settings)
{
	struct image_source *context = data;
	const char *file = obs_data_getstring(settings, "file");

	gs_entercontext(obs_graphics());

	if (context->tex) {
		texture_destroy(context->tex);
		context->tex = NULL;
	}

	if (file) {
		context->tex = gs_create_texture_from_file(file);
		if (context->tex) {
			context->cx = texture_getwidth(context->tex);
			context->cy = texture_getheight(context->tex);
		} else {
			warn("failed to load texture '%s'", file);
			context->cx = 0;
			context->cy = 0;
		}
	}

	gs_leavecontext();
}

static void *image_source_create(obs_data_t settings, obs_source_t source)
{
	struct image_source *context = bzalloc(sizeof(struct image_source));
	context->source = source;

	image_source_update(context, settings);
	return context;
}

static void image_source_destroy(void *data)
{
	struct image_source *context = data;

	gs_entercontext(obs_graphics());
	texture_destroy(context->tex);
	gs_leavecontext();

	bfree(context);
}

static uint32_t image_source_getwidth(void *data)
{
	struct image_source *context = data;
	return context->cx;
}

static uint32_t image_source_getheight(void *data)
{
	struct image_source *context = data;
	return context->cy;
}

static void image_source_render(void *data, effect_t effect)
{
	struct image_source *context = data;
	if (!context->tex)
		return;

	effect_settexture(effect_getparambyname(effect, "image"), context->tex);
	gs_draw_sprite(context->tex, 0, context->cx, context->cy);
}

static const char *image_filter =
	"All formats (*.bmp *.tga *.png *.jpeg *.jpg *.gif);;"
	"BMP Files (*.bmp);;"
	"Targa Files (*.tga);;"
	"PNG Files (*.png);;"
	"JPEG Files (*.jpeg *.jpg);;"
	"GIF Files (*.gif)";

static obs_properties_t image_source_properties(void)
{
	obs_properties_t props = obs_properties_create();

	/* TODO: locale */
	obs_properties_add_path(props, "file", "Image file", OBS_PATH_FILE,
			image_filter, NULL);

	return props;
}

static struct obs_source_info image_source_info = {
	.id           = "image_source",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.getname      = image_source_get_name,
	.create       = image_source_create,
	.destroy      = image_source_destroy,
	.update       = image_source_update,
	.getwidth     = image_source_getwidth,
	.getheight    = image_source_getheight,
	.video_render = image_source_render,
	.properties   = image_source_properties
};

OBS_DECLARE_MODULE()

bool obs_module_load(uint32_t libobs_version)
{
	obs_register_source(&image_source_info);

	UNUSED_PARAMETER(libobs_version);
	return true;
}
