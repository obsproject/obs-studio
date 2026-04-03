#include <obs-module.h>
#include <graphics/image-file.h>
#include <graphics/vec2.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/* Settings keys                                                        */
/* ------------------------------------------------------------------ */
#define SETTING_UVMAP_PATH "uvmap_path"
#define SETTING_ZOOM       "zoom"
#define SETTING_POS_X      "pos_x"
#define SETTING_POS_Y      "pos_y"

/* ------------------------------------------------------------------ */
/* Localisation keys                                                    */
/* ------------------------------------------------------------------ */
#define TEXT_FILTER_NAME obs_module_text("UVMapFilter")
#define TEXT_UVMAP_PATH  obs_module_text("UVMapFilter.Path")
#define TEXT_ZOOM        obs_module_text("UVMapFilter.Zoom")
#define TEXT_POS_X       obs_module_text("UVMapFilter.PosX")
#define TEXT_POS_Y       obs_module_text("UVMapFilter.PosY")
#define TEXT_PATH_IMAGES obs_module_text("BrowsePath.Images")
#define TEXT_PATH_ALL    obs_module_text("BrowsePath.AllFiles")

/* ------------------------------------------------------------------ */
/* Filter state                                                         */
/* ------------------------------------------------------------------ */
struct uvmap_filter_data {
	obs_source_t *context;
	gs_effect_t  *effect;

	/* UV map texture */
	char           *uvmap_path;
	time_t          uvmap_timestamp;
	gs_image_file_t uvmap_image;
	float           update_elapsed;

	/* Transform params */
	float zoom;
	float pos_x;
	float pos_y;
};

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
static time_t get_file_timestamp(const char *path)
{
	struct stat st;
	return (os_stat(path, &st) == 0) ? st.st_mtime : -1;
}

static void uvmap_image_load(struct uvmap_filter_data *filter)
{
	obs_enter_graphics();
	gs_image_file_free(&filter->uvmap_image);
	obs_leave_graphics();

	if (filter->uvmap_path && *filter->uvmap_path) {
		filter->uvmap_timestamp = get_file_timestamp(filter->uvmap_path);
		gs_image_file_init(&filter->uvmap_image, filter->uvmap_path);
		filter->update_elapsed = 0.f;

		obs_enter_graphics();
		gs_image_file_init_texture(&filter->uvmap_image);
		obs_leave_graphics();
	}
}

/* ------------------------------------------------------------------ */
/* OBS source callbacks                                                 */
/* ------------------------------------------------------------------ */
static const char *uvmap_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return TEXT_FILTER_NAME;
}

static void uvmap_filter_update(void *data, obs_data_t *settings)
{
	struct uvmap_filter_data *filter = data;

	const char *path = obs_data_get_string(settings, SETTING_UVMAP_PATH);

	bfree(filter->uvmap_path);
	filter->uvmap_path = bstrdup(path);

	filter->zoom  = (float)obs_data_get_double(settings, SETTING_ZOOM);
	filter->pos_x = (float)obs_data_get_double(settings, SETTING_POS_X);
	filter->pos_y = (float)obs_data_get_double(settings, SETTING_POS_Y);

	if (filter->zoom < 0.01f)
		filter->zoom = 0.01f;

	uvmap_image_load(filter);

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	char *effect_path = obs_module_file("uvmap_filter.effect");
	filter->effect    = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);
	obs_leave_graphics();
}

static void *uvmap_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct uvmap_filter_data *filter =
		bzalloc(sizeof(struct uvmap_filter_data));
	filter->context = context;

	obs_source_update(context, settings);
	return filter;
}

static void uvmap_filter_destroy(void *data)
{
	struct uvmap_filter_data *filter = data;

	bfree(filter->uvmap_path);

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	gs_image_file_free(&filter->uvmap_image);
	obs_leave_graphics();

	bfree(filter);
}

static void uvmap_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, SETTING_ZOOM, 1.0);
	obs_data_set_default_double(settings, SETTING_POS_X, 0.0);
	obs_data_set_default_double(settings, SETTING_POS_Y, 0.0);
}

#define IMAGE_FILTER_EXTENSIONS " (*.png *.bmp *.jpg *.jpeg *.tga)"

static obs_properties_t *uvmap_filter_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	struct dstr       filter_str = {0};

	dstr_copy(&filter_str, TEXT_PATH_IMAGES);
	dstr_cat(&filter_str, IMAGE_FILTER_EXTENSIONS ";;");
	dstr_cat(&filter_str, TEXT_PATH_ALL);
	dstr_cat(&filter_str, " (*.*)");

	obs_properties_add_path(props, SETTING_UVMAP_PATH, TEXT_UVMAP_PATH,
				OBS_PATH_FILE, filter_str.array, NULL);

	obs_properties_add_float_slider(props, SETTING_ZOOM, TEXT_ZOOM,
					0.25, 16.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_POS_X, TEXT_POS_X,
					-0.5, 0.5, 0.001);
	obs_properties_add_float_slider(props, SETTING_POS_Y, TEXT_POS_Y,
					-0.5, 0.5, 0.001);

	dstr_free(&filter_str);
	return props;
}

static void uvmap_filter_tick(void *data, float seconds)
{
	struct uvmap_filter_data *filter = data;
	filter->update_elapsed += seconds;

	/* Reload UV map if the file changed on disk */
	if (filter->update_elapsed >= 1.f && filter->uvmap_path &&
	    *filter->uvmap_path) {
		filter->update_elapsed = 0.f;
		time_t t = get_file_timestamp(filter->uvmap_path);
		if (t != filter->uvmap_timestamp)
			uvmap_image_load(filter);
	}
}

static void uvmap_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct uvmap_filter_data *filter = data;

	/* Skip if no UV map texture is loaded */
	if (!filter->effect || !filter->uvmap_image.texture) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING))
		return;

	gs_eparam_t *p;

	p = gs_effect_get_param_by_name(filter->effect, "uvmap");
	gs_effect_set_texture(p, filter->uvmap_image.texture);

	struct vec2 offset = {filter->pos_x, filter->pos_y};
	p = gs_effect_get_param_by_name(filter->effect, "uv_offset");
	gs_effect_set_vec2(p, &offset);

	p = gs_effect_get_param_by_name(filter->effect, "uv_zoom");
	gs_effect_set_float(p, filter->zoom);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);
}

/* ------------------------------------------------------------------ */
/* Registration struct                                                  */
/* ------------------------------------------------------------------ */
struct obs_source_info uvmap_filter = {
	.id           = "uvmap_filter",
	.type         = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name     = uvmap_filter_get_name,
	.create       = uvmap_filter_create,
	.destroy      = uvmap_filter_destroy,
	.update       = uvmap_filter_update,
	.get_defaults = uvmap_filter_defaults,
	.get_properties = uvmap_filter_properties,
	.video_tick   = uvmap_filter_tick,
	.video_render = uvmap_filter_render,
};
