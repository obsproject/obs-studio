#import <AppKit/AppKit.h>
#import <IOSurface/IOSurface.h>
#import <ScriptingBridge/ScriptingBridge.h>
#import <Syphon/Syphon.h>

#import <obs-module.h>
#import <AvailabilityMacros.h>

#import "SyphonOBSClient.h"

#define LOG(level, message, ...) blog(level, "%s: " message, obs_source_get_name(s->source), ##__VA_ARGS__)

struct syphon {
    SyphonOBSClient *client;
    IOSurfaceRef ref;

    gs_samplerstate_t *sampler;
    gs_effect_t *effect;
    gs_vertbuffer_t *vertbuffer;
    gs_texture_t *tex;
    uint32_t width, height;
    bool crop;
    CGRect crop_rect;
    bool allow_transparency;

    obs_source_t *source;

    bool active;
    bool uuid_changed;
    id new_server_listener;
    id retire_listener;

    NSString *app_name;
    NSString *name;
    NSString *uuid;
};
typedef struct syphon *syphon_t;

static inline void update_properties(syphon_t s)
{
    obs_source_update_properties(s->source);
}

static const char *syphon_get_name(void *unused __attribute((unused)))
{
    return obs_module_text("Syphon");
}

static void stop_client(syphon_t s)
{
    obs_enter_graphics();

    if (s->client) {
        [s->client stop];
    }

    if (s->tex) {
        gs_texture_destroy(s->tex);
        s->tex = NULL;
    }

    if (s->ref) {
        IOSurfaceDecrementUseCount(s->ref);
        CFRelease(s->ref);
        s->ref = NULL;
    }

    s->width = 0;
    s->height = 0;

    obs_leave_graphics();
}

static inline NSDictionary *find_by_uuid(NSArray *arr, NSString *uuid)
{
    for (NSDictionary *dict in arr) {
        if ([dict[SyphonServerDescriptionUUIDKey] isEqual:uuid])
            return dict;
    }

    return nil;
}

/* If you see this and think "surely these must be defined in some type of
 * public header!": They don't. Or at least I couldnt't find anything. The
 * definitions inside of Syphon are only in SyphonPrivate.h/m, and nowhere
 * else. When we had Syphon as a submodule we abused this by using extern, but
 * now that we use a prebuilt framework and as such no longer can acceess the
 * private headers and sources directly, that's no longer possible. Other
 * projects sometimes copy SyphonPrivate.h entirely (with the full definitions
 * of everything), but for our purpose these strings are enough. */
const NSString *SyphonServerDescriptionDictionaryVersionKey = @"SyphonServerDescriptionDictionaryVersionKey";
const NSString *SyphonSurfaceType = @"SyphonSurfaceType";
const NSString *SyphonSurfaceTypeIOSurface = @"SyphonSurfaceTypeIOSurface";
const NSString *SyphonServerDescriptionSurfacesKey = @"SyphonServerDescriptionSurfacesKey";

static inline void check_version(syphon_t s, NSDictionary *desc)
{
    NSNumber *version = desc[SyphonServerDescriptionDictionaryVersionKey];
    if (!version)
        return LOG(LOG_WARNING, "Server description does not contain "
                                "VersionKey");

    if (version.unsignedIntValue > 0)
        LOG(LOG_WARNING,
            "Got server description version %d, "
            "expected 0",
            version.unsignedIntValue);
}

static inline void check_description(syphon_t s, NSDictionary *desc)
{
    NSArray *surfaces = desc[SyphonServerDescriptionSurfacesKey];
    if (!surfaces)
        return LOG(LOG_WARNING, "Server description does not contain "
                                "SyphonServerDescriptionSurfacesKey");

    if (!surfaces.count)
        return LOG(LOG_WARNING, "Server description contains empty "
                                "SyphonServerDescriptionSurfacesKey");

    for (NSDictionary *surface in surfaces) {
        NSString *type = surface[SyphonSurfaceType];
        if (type && [type isEqual:SyphonSurfaceTypeIOSurface])
            return;
    }

    NSString *surfaces_string = [NSString stringWithFormat:@"%@", surfaces];
    LOG(LOG_WARNING,
        "SyphonSurfaces does not contain"
        "'SyphonSurfaceTypeIOSurface': %s",
        surfaces_string.UTF8String);
}

static inline void handle_new_frame(syphon_t s, SyphonOBSClient *client)
{
    IOSurfaceRef ref = [client newFrameImage];

    if (!ref)
        return;

    if (ref == s->ref) {
        CFRelease(ref);
        return;
    }

    IOSurfaceIncrementUseCount(ref);

    obs_enter_graphics();
    if (s->ref) {
        gs_texture_destroy(s->tex);
        IOSurfaceDecrementUseCount(s->ref);
        CFRelease(s->ref);
    }

    s->ref = ref;
    s->tex = gs_texture_create_from_iosurface(s->ref);
    s->width = gs_texture_get_width(s->tex);
    s->height = gs_texture_get_height(s->tex);
    obs_leave_graphics();
}

static void create_client(syphon_t s)
{
    stop_client(s);

    if (!s->app_name.length && !s->name.length && !s->uuid.length)
        return;

    SyphonServerDirectory *ssd = [SyphonServerDirectory sharedDirectory];
    NSArray *servers = [ssd serversMatchingName:s->name appName:s->app_name];
    if (!servers.count)
        return;

    NSDictionary *desc = find_by_uuid(servers, s->uuid);
    if (!desc) {
        desc = servers[0];
        if (![s->uuid isEqualToString:desc[SyphonServerDescriptionUUIDKey]]) {
            s->uuid_changed = true;
        }
    }

    check_version(s, desc);
    check_description(s, desc);

    s->client = [[SyphonOBSClient alloc] initWithServerDescription:desc options:nil
                                                   newFrameHandler:^(SyphonOBSClient *client) {
                                                       handle_new_frame(s, client);
                                                   }];

    s->active = true;
}

static inline bool load_syphon_settings(syphon_t s, obs_data_t *settings)
{
    NSString *app_name = @(obs_data_get_string(settings, "app_name"));
    NSString *name = @(obs_data_get_string(settings, "name"));
    bool equal_names = [app_name isEqual:s->app_name] && [name isEqual:s->name];
    if (s->uuid_changed && equal_names)
        return false;

    NSString *uuid = @(obs_data_get_string(settings, "uuid"));
    if ([uuid isEqual:s->uuid] && equal_names)
        return false;

    s->app_name = app_name;
    s->name = name;
    s->uuid = uuid;
    s->uuid_changed = false;
    return true;
}

static inline void update_from_announce(syphon_t s, NSDictionary *info)
{
    if (s->active)
        return;

    if (!info)
        return;

    NSString *app_name = info[SyphonServerDescriptionAppNameKey];
    NSString *name = info[SyphonServerDescriptionNameKey];
    NSString *uuid = info[SyphonServerDescriptionUUIDKey];

    if (![uuid isEqual:s->uuid] && !([app_name isEqual:s->app_name] && [name isEqual:s->name]))
        return;

    s->app_name = app_name;
    s->name = name;
    if (![s->uuid isEqualToString:uuid]) {
        s->uuid = uuid;
        s->uuid_changed = true;
    }

    create_client(s);
}

static inline void handle_announce(syphon_t s, NSNotification *note)
{
    if (!note)
        return;

    update_from_announce(s, note.userInfo);
    update_properties(s);
}

static inline void update_from_retire(syphon_t s, NSDictionary *info)
{
    if (!info)
        return;

    NSString *uuid = info[SyphonServerDescriptionUUIDKey];
    if (!uuid)
        return;

    if (![uuid isEqual:s->uuid])
        return;

    s->active = false;
}

static inline void handle_retire(syphon_t s, NSNotification *note)
{
    if (!note)
        return;

    update_from_retire(s, note.userInfo);
    update_properties(s);
}

static inline gs_vertbuffer_t *create_vertbuffer()
{
    struct gs_vb_data *vb_data = gs_vbdata_create();
    vb_data->num = 4;
    vb_data->points = bzalloc(sizeof(struct vec3) * 4);
    if (!vb_data->points)
        return NULL;

    vb_data->num_tex = 1;
    vb_data->tvarray = bzalloc(sizeof(struct gs_tvertarray));
    if (!vb_data->tvarray)
        goto fail_tvarray;

    vb_data->tvarray[0].width = 2;
    vb_data->tvarray[0].array = bzalloc(sizeof(struct vec2) * 4);
    if (!vb_data->tvarray[0].array)
        goto fail_array;

    gs_vertbuffer_t *vbuff = gs_vertexbuffer_create(vb_data, GS_DYNAMIC);
    if (vbuff)
        return vbuff;

    bfree(vb_data->tvarray[0].array);
fail_array:
    bfree(vb_data->tvarray);
fail_tvarray:
    bfree(vb_data->points);

    return NULL;
}

static inline bool init_obs_graphics_objects(syphon_t s)
{
    struct gs_sampler_info info = {
        .filter = GS_FILTER_LINEAR,
        .address_u = GS_ADDRESS_CLAMP,
        .address_v = GS_ADDRESS_CLAMP,
        .address_w = GS_ADDRESS_CLAMP,
        .max_anisotropy = 1,
    };

    obs_enter_graphics();
    s->sampler = gs_samplerstate_create(&info);
    s->vertbuffer = create_vertbuffer();

    if (gs_get_device_type() == GS_DEVICE_OPENGL) {
        s->effect = obs_get_base_effect(OBS_EFFECT_DEFAULT_RECT);
    } else {
        s->effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    }
    obs_leave_graphics();

    return s->sampler != NULL && s->vertbuffer != NULL && s->effect != NULL;
}

static inline bool create_syphon_listeners(syphon_t s)
{
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    s->new_server_listener = [nc addObserverForName:SyphonServerAnnounceNotification object:nil
                                              queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
                                                  handle_announce(s, note);
                                              }];

    s->retire_listener = [nc addObserverForName:SyphonServerRetireNotification object:nil
                                          queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
                                              handle_retire(s, note);
                                          }];

    return s->new_server_listener != nil && s->retire_listener != nil;
}

static inline void load_crop(syphon_t s, obs_data_t *settings)
{
    s->crop = obs_data_get_bool(settings, "crop");

#define LOAD_CROP(x) s->crop_rect.x = obs_data_get_double(settings, "crop." #x)
    LOAD_CROP(origin.x);
    LOAD_CROP(origin.y);
    LOAD_CROP(size.width);
    LOAD_CROP(size.height);
#undef LOAD_CROP
}

static inline void syphon_destroy_internal(syphon_t s);

static void *syphon_create_internal(obs_data_t *settings, obs_source_t *source)
{
    syphon_t s = bzalloc(sizeof(struct syphon));
    if (!s)
        return s;

    s->source = source;

    if (!init_obs_graphics_objects(s)) {
        syphon_destroy_internal(s);
        return NULL;
    }

    if (!load_syphon_settings(s, settings)) {
        syphon_destroy_internal(s);
        return NULL;
    }

    if (!create_syphon_listeners(s)) {
        syphon_destroy_internal(s);
        return NULL;
    }

    create_client(s);

    load_crop(s, settings);

    s->allow_transparency = obs_data_get_bool(settings, "allow_transparency");

    return s;
}

static void *syphon_create(obs_data_t *settings, obs_source_t *source)
{
    @autoreleasepool {
        return syphon_create_internal(settings, source);
    }
}

static inline void stop_listener(id listener)
{
    if (!listener)
        return;

    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    [nc removeObserver:listener];
}

static inline void syphon_destroy_internal(syphon_t s)
{
    stop_listener(s->new_server_listener);
    stop_listener(s->retire_listener);

    obs_enter_graphics();
    stop_client(s);

    if (s->sampler)
        gs_samplerstate_destroy(s->sampler);
    if (s->vertbuffer)
        gs_vertexbuffer_destroy(s->vertbuffer);
    obs_leave_graphics();

    bfree(s);
}

static void syphon_destroy(void *data)
{
    @autoreleasepool {
        syphon_destroy_internal(data);
    }
}

static inline NSString *get_string(obs_data_t *settings, const char *name)
{
    if (!settings)
        return nil;

    return @(obs_data_get_string(settings, name));
}

static inline void update_strings_from_context(syphon_t s, obs_data_t *settings, NSString **app, NSString **name,
                                               NSString **uuid)
{
    if (!s || !s->uuid_changed)
        return;

    s->uuid_changed = false;
    *app = s->app_name;
    *name = s->name;
    *uuid = s->uuid;

    obs_data_set_string(settings, "app_name", s->app_name.UTF8String);
    obs_data_set_string(settings, "name", s->name.UTF8String);
    obs_data_set_string(settings, "uuid", s->uuid.UTF8String);
}

static inline void add_servers(syphon_t s, obs_property_t *list, obs_data_t *settings)
{
    bool found_current = settings == NULL;

    NSString *set_app = get_string(settings, "app_name");
    NSString *set_name = get_string(settings, "name");
    NSString *set_uuid = get_string(settings, "uuid");

    update_strings_from_context(s, settings, &set_app, &set_name, &set_uuid);

    obs_property_list_add_string(list, "", "");
    NSArray *arr = [[SyphonServerDirectory sharedDirectory] servers];
    for (NSDictionary *server in arr) {
        NSString *app = server[SyphonServerDescriptionAppNameKey];
        NSString *name = server[SyphonServerDescriptionNameKey];
        NSString *uuid = server[SyphonServerDescriptionUUIDKey];
        NSString *serv = [NSString stringWithFormat:@"[%@] %@", app, name];

        obs_property_list_add_string(list, serv.UTF8String, uuid.UTF8String);

        if (!found_current)
            found_current = [uuid isEqual:set_uuid];
    }

    if (found_current || !set_uuid.length || !set_app.length)
        return;

    NSString *serv = [NSString stringWithFormat:@"[%@] %@", set_app, set_name];
    size_t idx = obs_property_list_add_string(list, serv.UTF8String, set_uuid.UTF8String);
    obs_property_list_item_disable(list, idx, true);
}

static bool servers_changed(obs_properties_t *props, obs_property_t *list, obs_data_t *settings)
{
    @autoreleasepool {
        obs_property_list_clear(list);
        add_servers(obs_properties_get_param(props), list, settings);
        return true;
    }
}

static bool update_crop(obs_properties_t *props, obs_property_t *prop, obs_data_t *settings)
{
    bool enabled = obs_data_get_bool(settings, "crop");

#define LOAD_CROP(x)                              \
    prop = obs_properties_get(props, "crop." #x); \
    obs_property_set_enabled(prop, enabled);
    LOAD_CROP(origin.x);
    LOAD_CROP(origin.y);
    LOAD_CROP(size.width);
    LOAD_CROP(size.height);
#undef LOAD_CROP

    return true;
}

static void show_syphon_license_internal(void)
{
    char *path = obs_module_file("syphon_license.txt");
    if (!path)
        return;

    NSWorkspace *ws = [NSWorkspace sharedWorkspace];

    NSURL *url =
        [NSURL URLWithString:[@"file://" stringByAppendingString:[NSString stringWithCString:path
                                                                                    encoding:NSUTF8StringEncoding]]];
    [ws openURL:url];
    bfree(path);
}

static bool show_syphon_license(obs_properties_t *props, obs_property_t *prop, void *data)
{
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(prop);
    UNUSED_PARAMETER(data);

    @autoreleasepool {
        show_syphon_license_internal();
        return false;
    }
}

static void syphon_release(void *param)
{
    if (!param)
        return;

    obs_source_release(((syphon_t) param)->source);
}

static inline obs_properties_t *syphon_properties_internal(syphon_t s)
{
    if (s && obs_source_get_ref(s->source) == NULL) {
        s = NULL;
    }

    obs_properties_t *props = obs_properties_create_param(s, syphon_release);

    obs_property_t *list =
        obs_properties_add_list(props, "uuid", obs_module_text("Source"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_set_modified_callback(list, servers_changed);

    obs_properties_add_bool(props, "allow_transparency", obs_module_text("AllowTransparency"));

    obs_property_t *crop = obs_properties_add_bool(props, "crop", obs_module_text("Crop"));
    obs_property_set_modified_callback(crop, update_crop);

#define LOAD_CROP(x) obs_properties_add_float(props, "crop." #x, obs_module_text("Crop." #x), 0., 4096.f, .5f);
    LOAD_CROP(origin.x);
    LOAD_CROP(origin.y);
    LOAD_CROP(size.width);
    LOAD_CROP(size.height);
#undef LOAD_CROP

    obs_properties_add_button(props, "syphon license", obs_module_text("SyphonLicense"), show_syphon_license);

    return props;
}

static obs_properties_t *syphon_properties(void *data)
{
    @autoreleasepool {
        return syphon_properties_internal(data);
    }
}

static inline void syphon_save_internal(syphon_t s, obs_data_t *settings)
{
    if (!s->uuid_changed)
        return;

    obs_data_set_string(settings, "app_name", s->app_name.UTF8String);
    obs_data_set_string(settings, "name", s->name.UTF8String);
    obs_data_set_string(settings, "uuid", s->uuid.UTF8String);
}

static void syphon_save(void *data, obs_data_t *settings)
{
    @autoreleasepool {
        syphon_save_internal(data, settings);
    }
}

static inline void build_sprite(struct gs_vb_data *data, float fcx, float fcy, float start_u, float end_u,
                                float start_v, float end_v)
{
    struct vec2 *tvarray = data->tvarray[0].array;

    vec3_set(data->points + 1, fcx, 0.0f, 0.0f);
    vec3_set(data->points + 2, 0.0f, fcy, 0.0f);
    vec3_set(data->points + 3, fcx, fcy, 0.0f);
    vec2_set(tvarray, start_u, start_v);
    vec2_set(tvarray + 1, end_u, start_v);
    vec2_set(tvarray + 2, start_u, end_v);
    vec2_set(tvarray + 3, end_u, end_v);
}

static inline void build_sprite_rect(struct gs_vb_data *data, float origin_x, float origin_y, float end_x, float end_y)
{
    build_sprite(data, fabsf(end_x - origin_x), fabsf(end_y - origin_y), origin_x, end_x, origin_y, end_y);
}

static void syphon_video_tick(void *data, float seconds)
{
    UNUSED_PARAMETER(seconds);

    syphon_t s = data;

    if (!s->tex)
        return;

    static const CGRect null_crop = {{0.f}};
    const CGRect *crop = &null_crop;
    if (s->crop)
        crop = &s->crop_rect;

    obs_enter_graphics();
    build_sprite_rect(gs_vertexbuffer_get_data(s->vertbuffer), (float) crop->origin.x,
                      s->height - (float) crop->origin.y, s->width - (float) crop->size.width,
                      (float) crop->size.height);
    obs_leave_graphics();
}

static void syphon_video_render(void *data, gs_effect_t *effect)
{
    UNUSED_PARAMETER(effect);

    syphon_t s = data;

    if (!s->tex)
        return;

    gs_vertexbuffer_flush(s->vertbuffer);
    gs_load_vertexbuffer(s->vertbuffer);
    gs_load_indexbuffer(NULL);
    gs_load_samplerstate(s->sampler, 0);
    const char *tech_name = s->allow_transparency ? "Draw" : "DrawOpaque";
    gs_technique_t *tech = gs_effect_get_technique(s->effect, tech_name);
    gs_effect_set_texture(gs_effect_get_param_by_name(s->effect, "image"), s->tex);
    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    gs_draw(GS_TRISTRIP, 0, 4);

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}

static uint32_t syphon_get_width(void *data)
{
    syphon_t s = (syphon_t) data;
    if (!s->crop)
        return s->width;
    int32_t width = s->width - (int) s->crop_rect.origin.x - (int) s->crop_rect.size.width;
    return MAX(0, width);
}

static uint32_t syphon_get_height(void *data)
{
    syphon_t s = (syphon_t) data;
    if (!s->crop)
        return s->height;
    int32_t height = s->height - (int) s->crop_rect.origin.y - (int) s->crop_rect.size.height;
    return MAX(0, height);
}

static inline bool update_syphon(syphon_t s, obs_data_t *settings)
{
    NSArray *arr = [[SyphonServerDirectory sharedDirectory] servers];

    if (!load_syphon_settings(s, settings))
        return false;

    NSDictionary *dict = find_by_uuid(arr, s->uuid);
    if (dict) {
        NSString *app = dict[SyphonServerDescriptionAppNameKey];
        NSString *name = dict[SyphonServerDescriptionNameKey];
        obs_data_set_string(settings, "app_name", app.UTF8String);
        obs_data_set_string(settings, "name", name.UTF8String);
        load_syphon_settings(s, settings);

    } else if (!dict && !s->uuid.length) {
        obs_data_set_string(settings, "app_name", "");
        obs_data_set_string(settings, "name", "");
        load_syphon_settings(s, settings);
    }

    return true;
}

static void syphon_update_internal(syphon_t s, obs_data_t *settings)
{
    s->allow_transparency = obs_data_get_bool(settings, "allow_transparency");

    load_crop(s, settings);

    if (update_syphon(s, settings))
        create_client(s);
}

static void syphon_update(void *data, obs_data_t *settings)
{
    @autoreleasepool {
        syphon_update_internal(data, settings);
    }
}

struct obs_source_info syphon_info = {
    .id = "syphon-input",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_DO_NOT_DUPLICATE,
    .get_name = syphon_get_name,
    .create = syphon_create,
    .destroy = syphon_destroy,
    .video_render = syphon_video_render,
    .video_tick = syphon_video_tick,
    .get_properties = syphon_properties,
    .get_width = syphon_get_width,
    .get_height = syphon_get_height,
    .update = syphon_update,
    .save = syphon_save,
    .icon_type = OBS_ICON_TYPE_GAME_CAPTURE,
};
