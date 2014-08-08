#include <obs-module.h>

#define warn(format, ...) \
	blog(LOG_WARNING, "[image_source: '%s'] " format, \
			obs_source_get_name(context->source), ##__VA_ARGS__)

struct image_source {
	obs_source_t source;

	gs_texture_t tex;
	uint32_t     cx;
	uint32_t     cy;
};

static const char *image_source_get_name(void)
{
	return obs_module_text("ImageInput");
}

static void image_source_update(void *data, obs_data_t settings)
{
	struct image_source *context = data;
	const char *file = obs_data_get_string(settings, "file");

	obs_enter_graphics();

	if (context->tex) {
		gs_texture_destroy(context->tex);
		context->tex = NULL;
	}

	if (file && *file) {
		context->tex = gs_texture_create_from_file(file);
		if (context->tex) {
			context->cx = gs_texture_get_width(context->tex);
			context->cy = gs_texture_get_height(context->tex);
		} else {
			warn("failed to load texture '%s'", file);
			context->cx = 0;
			context->cy = 0;
		}
	}

	obs_leave_graphics();
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

	obs_enter_graphics();
	gs_texture_destroy(context->tex);
	obs_leave_graphics();

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

static void image_source_render(void *data, gs_effect_t effect)
{
	struct image_source *context = data;
	if (!context->tex)
		return;

	gs_reset_blend_state();
	gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"),
			context->tex);
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

	obs_properties_add_path(props,
			"file", obs_module_text("File"),
			OBS_PATH_FILE, image_filter, NULL);

	return props;
}

static struct obs_source_info image_source_info = {
	.id             = "image_source",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_VIDEO,
	.get_name       = image_source_get_name,
	.create         = image_source_create,
	.destroy        = image_source_destroy,
	.update         = image_source_update,
	.get_width      = image_source_getwidth,
	.get_height     = image_source_getheight,
	.video_render   = image_source_render,
	.get_properties = image_source_properties
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("image-source", "en-US")

bool obs_module_load(void)
{
	obs_register_source(&image_source_info);
	return true;
}
