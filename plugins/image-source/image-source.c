#include <obs-module.h>
#include <util/platform.h>

#define warn(format, ...) \
	blog(LOG_WARNING, "[image_source: '%s'] " format, \
			obs_source_get_name(context->source), ##__VA_ARGS__)

struct image_source {
	obs_source_t *source;

    char* file_name;

    gs_texture_t *tex, **tex_gif;
	uint32_t     cx;
    uint32_t     cy;

    uint32_t     cur_frame_gif, size_tex_gif; float fps_gif, timestamp;
    bool         animate_gif;
};

static const char *image_source_get_name(void)
{
	return obs_module_text("ImageInput");
}

static void image_source_update_gif(void *data)
{
    struct image_source *context = data;
    if( ( (os_gettime_ns()/1000) - context->timestamp ) > (1.0f/context->fps_gif)*1000000 )
    {
        context->timestamp=os_gettime_ns()/1000;
        obs_enter_graphics();

        context->tex=context->tex_gif[context->cur_frame_gif];

        context->cur_frame_gif++;
        if(context->cur_frame_gif==context->size_tex_gif)
             context->cur_frame_gif=0;

        obs_leave_graphics();
    }
}

static void image_source_update(void *data, obs_data_t *settings)
{
    struct image_source *context = data;
    context->file_name = obs_data_get_string(settings, "file");
    context->animate_gif = obs_data_get_bool(settings, "animate_gif");
    context->fps_gif=obs_data_get_double(settings,"custom_fps_gif");

    context->size_tex_gif=0;
    context->cur_frame_gif=0;
    context->timestamp=os_gettime_ns()/1000; //cut to microseconds


    obs_enter_graphics();


    if (context->tex) {
        gs_texture_destroy(context->tex);
        context->tex = NULL;
    }

    if (context->file_name && *(context->file_name)) {

        if(!context->animate_gif)
            context->tex = gs_texture_create_from_file(context->file_name);
        else {
            if(!obs_data_get_bool(settings, "auto_fps_gif")){
                context->tex_gif = gs_texture_create_from_file_gif(context->file_name, &context->size_tex_gif, NULL);
                context->tex=context->tex_gif[0];
            }
            else {
                context->tex_gif = gs_texture_create_from_file_gif(context->file_name, &context->size_tex_gif, &context->fps_gif);
                context->tex=context->tex_gif[0];
            }
        }

        if (context->tex) {
            context->cx = gs_texture_get_width(context->tex);
            context->cy = gs_texture_get_height(context->tex);
        } else {
            warn("failed to load texture '%s'", context->file_name);
            context->cx = 0;
            context->cy = 0;
        }
    }

    obs_leave_graphics();

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

static void image_source_render(void *data, gs_effect_t *effect)
{

    struct image_source *context = data;
    if(context->animate_gif)
    {
        image_source_update_gif(context);
    }
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

static obs_properties_t *image_source_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_path(props,
			"file", obs_module_text("File"),
			OBS_PATH_FILE, image_filter, NULL);
    obs_properties_add_bool(props,"animate_gif", obs_module_text("Animate Gifs"));
    obs_properties_add_bool(props,"auto_fps_gif", obs_module_text("Automatic gif frame rate"));
    obs_properties_add_float(props,"custom_fps_gif",  obs_module_text("Custom gif frame rate"),1,1000,0.01);
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
