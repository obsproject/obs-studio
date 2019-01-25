#include <obs-module.h>
#include <obs-source.h>
#include <obs.h>
#include <util/platform.h>
#include <graphics/image-file.h>
#include <inttypes.h>

#define SETTING_IMAGE_PATH "image_path"

#define blog(level, format, ...) \
	blog(level, "%s: " format, __FUNCTION__, __VA_ARGS__)

struct nine_patch_data {
    obs_source_t    *context;

    gs_image_file_t image;
    gs_texture_t    *target;
    gs_effect_t     *effect;
    struct vec2     ninePatchSize;
    struct vec4     marginBounds;
    struct vec4     paddingBounds;
};

static const char *nine_patch_getname(void *unused)
{
    UNUSED_PARAMETER(unused);
    return obs_module_text("9PatchFilter");
}

// Grab the color from a point in the image data and return
// a uint32 representation
static uint32_t getPixel(gs_image_file_t image, uint32_t x, uint32_t y) {
    uint32_t offset = (x*4) + (y*image.cx*4);

    uint32_t val;
    val  = image.texture_data[offset];
    val |= ((uint32_t)image.texture_data[offset+1])  << 8;
    val |= ((uint32_t)image.texture_data[offset+2])  << 16;
    val |= ((uint32_t)image.texture_data[offset+3])  << 24;
    return val;
}

// Iterate over the edges of an image looking for black pixels
// Store the margins and content sizes from these pixels
static void update_bounds(void *data) {
    struct nine_patch_data *filter = data;

    //rbga
    uint32_t blackColor = 4278190080;

    bool marginXFound = false;
    bool paddingXFound = false;
    bool marginYFound = false;
    bool paddingYFound = false;

    struct vec4 marginBounds;
    struct vec4 paddingBounds;

    uint32_t x;
    for (x=0; x<filter->image.cx; x++){
        // scan the top row
        uint32_t pixel = getPixel(filter->image, x, 0);
        if (!marginXFound && pixel == blackColor) {
            marginXFound = true;
            marginBounds.x = x - 1;
        } else if (marginXFound && pixel == blackColor) {
            // store the last found black pixel
            marginBounds.z = filter->image.cx - x - 2;
        }
        // scan the bottom row
        pixel = getPixel(filter->image, x, filter->image.cy-1);
        if (!paddingXFound && pixel == blackColor) {
            paddingXFound = true;
            paddingBounds.x = x - 1;
        } else if (paddingXFound && pixel == blackColor) {
            paddingBounds.z = filter->image.cx - x - 2;
        }
    }

    uint32_t y;
    for (y=0; y<filter->image.cy; y++){
        // scan the left row
        uint32_t pixel = getPixel(filter->image, 0, y);
        if (!marginYFound && pixel == blackColor) {
            marginYFound = true;
            marginBounds.y = y - 1;
        } else if (marginXFound && pixel == blackColor) {
            marginBounds.w = filter->image.cy - y - 2;
        }
        // scan the right row
        pixel = getPixel(filter->image, filter->image.cx-1, y);
        if (!paddingYFound && pixel == blackColor) {
            paddingYFound = true;
            paddingBounds.y = y - 1;
        } else if (paddingYFound && pixel == blackColor) {
            paddingBounds.w = filter->image.cy - y - 2;
        }
    }

    filter->marginBounds = marginBounds;
    filter->paddingBounds = paddingBounds;
}

static void nine_patch_update(void *data, obs_data_t *settings)
{
    struct nine_patch_data *filter = data;

    const char *path = obs_data_get_string(settings, SETTING_IMAGE_PATH);

    obs_enter_graphics();
    gs_image_file_free(&filter->image);
    obs_leave_graphics();

    gs_image_file_init(&filter->image, path);

    // Run this before init texture since that frees the
    // underlying image data
    update_bounds(filter);

    obs_enter_graphics();

    gs_image_file_init_texture(&filter->image);

    filter->target = filter->image.texture;

    char *effect_path = obs_module_file("9patch_filter.effect");
    gs_effect_destroy(filter->effect);
    filter->effect = gs_effect_create_from_file(effect_path, NULL);
    bfree(effect_path);

    obs_leave_graphics();

    vec2_set(&filter->ninePatchSize, filter->image.cx, filter->image.cy);
}

static void nine_patch_destroy(void *data)
{
    struct nine_patch_data *filter = data;

    obs_enter_graphics();
    gs_image_file_free(&filter->image);
    obs_leave_graphics();

    bfree(data);
}

static void *nine_patch_create(obs_data_t *settings, obs_source_t *context)
{
    struct nine_patch_data *filter =
            bzalloc(sizeof(struct nine_patch_data));

    filter->context = context;

    obs_source_update(context, settings);

    return filter;
}

static void nine_patch_render(void *data, gs_effect_t *effect)
{
    struct nine_patch_data *filter = data;
    obs_source_t *target = obs_filter_get_target(filter->context);
    gs_eparam_t *param;

    if (!target || !filter->target || !filter->effect) {
        obs_source_skip_video_filter(filter->context);
        return;
    }

    if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
                                         OBS_ALLOW_DIRECT_RENDERING))
        return;

    param = gs_effect_get_param_by_name(filter->effect, "nine_patch");
    gs_effect_set_texture(param, filter->target);

    param = gs_effect_get_param_by_name(filter->effect, "nine_patch_size");
    gs_effect_set_vec2(param, &filter->ninePatchSize);

    param = gs_effect_get_param_by_name(filter->effect, "margin_bounds");
    gs_effect_set_vec4(param, &filter->marginBounds);

    param = gs_effect_get_param_by_name(filter->effect, "padding_bounds");
    gs_effect_set_vec4(param, &filter->paddingBounds);

    param = gs_effect_get_param_by_name(filter->effect, "image_width");
    gs_effect_set_float(param, obs_source_get_width(filter->context));

    param = gs_effect_get_param_by_name(filter->effect, "image_height");
    gs_effect_set_float(param, obs_source_get_height(filter->context));

    obs_source_process_filter_end(filter->context, filter->effect, 0, 0);
}

static obs_properties_t *nine_patch_properties(void *data)
{
    obs_properties_t *props = obs_properties_create();

    obs_properties_add_path(props, SETTING_IMAGE_PATH, "Image Path",
                            OBS_PATH_FILE, "*.9.png", NULL);

    UNUSED_PARAMETER(data);
    return props;
}

static void nine_patch_defaults(obs_data_t *settings)
{

}

struct obs_source_info nine_patch_filter = {
        .id = "nine_patch_filter",
        .type = OBS_SOURCE_TYPE_FILTER,
        .output_flags = OBS_SOURCE_VIDEO,
        .get_name = nine_patch_getname,
        .create = nine_patch_create,
        .destroy = nine_patch_destroy,
        .update = nine_patch_update,
        .video_render = nine_patch_render,
        .get_properties = nine_patch_properties,
        .get_defaults = nine_patch_defaults
};
