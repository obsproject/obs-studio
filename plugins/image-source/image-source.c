#include <obs-module.h>
#include <util/platform.h>
#include <sys/stat.h>

#define blog(log_level, format, ...) \
	blog(log_level, "[image_source: '%s'] " format, \
			obs_source_get_name(context->source), ##__VA_ARGS__)

#define debug(format, ...) \
	blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) \
	blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) \
	blog(LOG_WARNING, format, ##__VA_ARGS__)

struct image_source {
	obs_source_t *source;

	char         *file;
	bool         persistent;
	time_t       file_timestamp;
	float        update_time_elapsed;

	gs_texture_t *tex;
	uint32_t     cx;
	uint32_t     cy;
};


static time_t get_modified_timestamp(const char *filename)
{
	struct stat stats;
	stat(filename, &stats);
	return stats.st_mtime;
}

static const char *image_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ImageInput");
}

static void image_source_load(struct image_source *context)
{
	char *file = context->file;

	obs_enter_graphics();

	if (context->tex)
		gs_texture_destroy(context->tex);
	context->tex = NULL;

	if (file && *file) {
		debug("loading texture '%s'", file);
		context->file_timestamp = get_modified_timestamp(file);
		context->tex = gs_texture_create_from_file(file);
		context->update_time_elapsed = 0;

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

static void image_source_unload(struct image_source *context)
{
	obs_enter_graphics();

	if (context->tex)
		gs_texture_destroy(context->tex);
	context->tex = NULL;

	obs_leave_graphics();
}

static void image_source_update(void *data, obs_data_t *settings)
{
	struct image_source *context = data;
	const char *file = obs_data_get_string(settings, "file");
	const bool unload = obs_data_get_bool(settings, "unload");

	if (context->file)
		bfree(context->file);
	context->file = bstrdup(file);
	context->persistent = !unload;

	/* Load the image if the source is persistent or showing */
	if (context->persistent || obs_source_showing(context->source))
		image_source_load(data);
	else
		image_source_unload(data);
}

static void image_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "unload", true);
}

static void image_source_show(void *data)
{
	struct image_source *context = data;

	if (!context->persistent)
		image_source_load(context);
}

static void image_source_hide(void *data)
{
	struct image_source *context = data;

	if (!context->persistent)
		image_source_unload(context);
}

static void *image_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct image_source *context = bzalloc(sizeof(struct image_source));
	context->source = source;

	image_source_update(context, settings);
	return context;
}

static void image_source_destroy(void *data)
{
	struct image_source *context = data;

	image_source_unload(context);

	if (context->file)
		bfree(context->file);
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

static void image_source_render(void *data, gs_effect_t *effect)
{
	struct image_source *context = data;

	if (!context->tex)
		return;

	gs_reset_blend_state();
	gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"),
			context->tex);
	gs_draw_sprite(context->tex, 0, context->cx, context->cy);
}

static void image_source_tick(void *data, float seconds)
{
	struct image_source *context = data;

	if (!obs_source_showing(context->source)) return;

	context->update_time_elapsed += seconds;

	if (context->update_time_elapsed >= 1.0f) {
		time_t t = get_modified_timestamp(context->file);
		context->update_time_elapsed = 0.0f;

		if (context->file_timestamp < t) {
			image_source_load(context);
		}
	}
}


static const char *image_filter =
	"All formats (*.bmp *.tga *.png *.jpeg *.jpg *.gif);;"
	"BMP Files (*.bmp);;"
	"Targa Files (*.tga);;"
	"PNG Files (*.png);;"
	"JPEG Files (*.jpeg *.jpg);;"
	"GIF Files (*.gif)";

static obs_properties_t *image_source_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_path(props,
			"file", obs_module_text("File"),
			OBS_PATH_FILE, image_filter, NULL);
	obs_properties_add_bool(props,
			"unload", obs_module_text("UnloadWhenNotShowing"));

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
	.get_defaults   = image_source_defaults,
	.show           = image_source_show,
	.hide           = image_source_hide,
	.get_width      = image_source_getwidth,
	.get_height     = image_source_getheight,
	.video_render   = image_source_render,
	.video_tick     = image_source_tick,
	.get_properties = image_source_properties
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("image-source", "en-US")

bool obs_module_load(void)
{
	obs_register_source(&image_source_info);
	return true;
}
