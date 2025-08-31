#include "mac-sck-common.h"
#include "window-utils.h"

API_AVAILABLE(macos(12.5)) static void destroy_screen_stream(struct screen_capture *sc)
{
    if (sc->disp && !sc->capture_failed) {
        [sc->disp stopCaptureWithCompletionHandler:^(NSError *_Nullable error) {
            if (error && error.code != SCStreamErrorAttemptToStopStreamState) {
                MACCAP_ERR("destroy_screen_stream: Failed to stop stream with error %s\n",
                           [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
            }
        }];
    }

    if (sc->stream_properties) {
        [sc->stream_properties release];
        sc->stream_properties = NULL;
    }

    if (sc->tex) {
        gs_texture_destroy(sc->tex);
        sc->tex = NULL;
    }

    if (sc->current) {
        IOSurfaceDecrementUseCount(sc->current);
        CFRelease(sc->current);
        sc->current = NULL;
    }

    if (sc->prev) {
        IOSurfaceDecrementUseCount(sc->prev);
        CFRelease(sc->prev);
        sc->prev = NULL;
    }

    if (sc->disp) {
        [sc->disp release];
        sc->disp = NULL;
    }

    os_event_destroy(sc->stream_start_completed);
}

API_AVAILABLE(macos(12.5)) static void sck_video_capture_destroy(void *data)
{
    struct screen_capture *sc = data;

    if (!sc)
        return;

    obs_enter_graphics();

    destroy_screen_stream(sc);

    obs_leave_graphics();

    if (sc->shareable_content) {
        os_sem_wait(sc->shareable_content_available);
        [sc->shareable_content release];
        os_sem_destroy(sc->shareable_content_available);
        sc->shareable_content_available = NULL;
    }

    if (sc->capture_delegate) {
        [sc->capture_delegate release];
    }
    [sc->application_id release];

    pthread_mutex_destroy(&sc->mutex);
    bfree(sc);
}

API_AVAILABLE(macos(12.5)) static bool init_screen_stream(struct screen_capture *sc)
{
    SCContentFilter *content_filter;
    if (sc->capture_failed) {
        sc->capture_failed = false;
        obs_source_update_properties(sc->source);
    }

    sc->frame = CGRectZero;
    sc->stream_properties = [[SCStreamConfiguration alloc] init];
    os_sem_wait(sc->shareable_content_available);

    SCDisplayRef (^get_target_display)(void) = ^SCDisplayRef {
        for (SCDisplay *display in sc->shareable_content.displays) {
            if (display.displayID == sc->display) {
                return display;
            }
        }
        return nil;
    };

    void (^set_display_mode)(struct screen_capture *, SCDisplay *) =
        ^void(struct screen_capture *capture_data, SCDisplay *target_display) {
            CGDisplayModeRef display_mode = CGDisplayCopyDisplayMode(target_display.displayID);
            [capture_data->stream_properties setWidth:CGDisplayModeGetPixelWidth(display_mode)];
            [capture_data->stream_properties setHeight:CGDisplayModeGetPixelHeight(display_mode)];
            CGDisplayModeRelease(display_mode);
        };

    switch (sc->capture_type) {
        case ScreenCaptureDisplayStream: {
            SCDisplay *target_display = get_target_display();
            if (target_display == nil) {
                MACCAP_ERR("init_screen_stream: Invalid target display ID:  %u\n", sc->display);
                os_sem_post(sc->shareable_content_available);
                sc->disp = NULL;
                os_event_init(&sc->stream_start_completed, OS_EVENT_TYPE_MANUAL);
                return true;
            }
            if (sc->hide_obs) {
                SCRunningApplication *obsApp = nil;
                NSString *mainBundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
                for (SCRunningApplication *app in sc->shareable_content.applications) {
                    if ([app.bundleIdentifier isEqualToString:mainBundleIdentifier]) {
                        obsApp = app;
                        break;
                    }
                }
                NSArray *exclusions = [[NSArray alloc] initWithObjects:obsApp, nil];
                NSArray *empty = [[NSArray alloc] init];
                content_filter = [[SCContentFilter alloc] initWithDisplay:target_display
                                                    excludingApplications:exclusions
                                                         exceptingWindows:empty];
                [empty release];
                [exclusions release];
            } else {
                NSArray *empty = [[NSArray alloc] init];
                content_filter = [[SCContentFilter alloc] initWithDisplay:target_display excludingWindows:empty];
                [empty release];
            }

            set_display_mode(sc, target_display);
        } break;
        case ScreenCaptureWindowStream: {
            SCWindow *target_window = nil;
            if (sc->window != kCGNullWindowID) {
                for (SCWindow *window in sc->shareable_content.windows) {
                    if (window.windowID == sc->window) {
                        target_window = window;
                        break;
                    }
                }
            }
            if (target_window == nil) {
                MACCAP_ERR("init_screen_stream: Invalid target window ID:  %u\n", sc->window);
                os_sem_post(sc->shareable_content_available);
                sc->disp = NULL;
                os_event_init(&sc->stream_start_completed, OS_EVENT_TYPE_MANUAL);
                return true;
            } else {
                content_filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:target_window];

                [sc->stream_properties setWidth:(size_t) target_window.frame.size.width];
                [sc->stream_properties setHeight:(size_t) target_window.frame.size.height];

                if (@available(macOS 14.2, *)) {
                    [sc->stream_properties setIncludeChildWindows:YES];
                }
            }
        } break;
        case ScreenCaptureApplicationStream: {
            SCDisplay *target_display = get_target_display();
            if (target_display == nil) {
                MACCAP_ERR("init_screen_stream: Invalid target display ID:  %u\n", sc->display);
                os_sem_post(sc->shareable_content_available);
                sc->disp = NULL;
                os_event_init(&sc->stream_start_completed, OS_EVENT_TYPE_MANUAL);
                return true;
            }
            SCRunningApplication *target_application = nil;
            for (SCRunningApplication *application in sc->shareable_content.applications) {
                if ([application.bundleIdentifier isEqualToString:sc->application_id]) {
                    target_application = application;
                    break;
                }
            }
            NSArray *target_application_array = [[NSArray alloc] initWithObjects:target_application, nil];
            NSArray *empty_array = [[NSArray alloc] init];
            content_filter = [[SCContentFilter alloc] initWithDisplay:target_display
                                                includingApplications:target_application_array
                                                     exceptingWindows:empty_array];
            if (@available(macOS 14.2, *)) {
                content_filter.includeMenuBar = YES;
            }

            [target_application_array release];
            [empty_array release];

            set_display_mode(sc, target_display);
        } break;
    }
    os_sem_post(sc->shareable_content_available);

    struct obs_video_info video_info;
    bool hasVideoInfo = obs_get_video_info(&video_info);

    CGColorRef background = CGColorGetConstantColor(kCGColorClear);
    [sc->stream_properties setQueueDepth:8];
    [sc->stream_properties setShowsCursor:!sc->hide_cursor];
    [sc->stream_properties setColorSpaceName:kCGColorSpaceDisplayP3];
    [sc->stream_properties setBackgroundColor:background];
    if (hasVideoInfo) {
        CMTime frameTimeInterval = CMTimeMake((int64_t) video_info.fps_den, video_info.fps_num);
        CMTime minimumUpdateTime = CMTimeMultiplyByFloat64(frameTimeInterval, 0.9);
        [sc->stream_properties setMinimumFrameInterval:minimumUpdateTime];
    } else {
        blog(LOG_WARNING, "Unable to retrieve OBS output FPS when initializing macOS Screen Capture");
    }
    FourCharCode l10r_type = 0;
    l10r_type = ('l' << 24) | ('1' << 16) | ('0' << 8) | 'r';
    [sc->stream_properties setPixelFormat:l10r_type];

    if (@available(macOS 13.0, *)) {
        [sc->stream_properties setCapturesAudio:YES];
        [sc->stream_properties setExcludesCurrentProcessAudio:YES];
        [sc->stream_properties setChannelCount:2];
    } else {
        if (sc->capture_type != ScreenCaptureWindowStream) {
            sc->disp = NULL;
            [content_filter release];
            os_event_init(&sc->stream_start_completed, OS_EVENT_TYPE_MANUAL);
            return true;
        }
    }

    sc->disp = [[SCStream alloc] initWithFilter:content_filter configuration:sc->stream_properties
                                       delegate:sc->capture_delegate];

    [content_filter release];

    NSError *addStreamOutputError = nil;
    BOOL did_add_output = [sc->disp addStreamOutput:sc->capture_delegate type:SCStreamOutputTypeScreen
                                 sampleHandlerQueue:nil
                                              error:&addStreamOutputError];
    if (!did_add_output) {
        MACCAP_ERR("init_screen_stream: Failed to add stream output with error %s\n",
                   [[addStreamOutputError localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
        [addStreamOutputError release];
        return !did_add_output;
    }

    if (@available(macOS 13.0, *)) {
        did_add_output = [sc->disp addStreamOutput:sc->capture_delegate type:SCStreamOutputTypeAudio
                                sampleHandlerQueue:nil
                                             error:&addStreamOutputError];
        if (!did_add_output) {
            MACCAP_ERR("init_screen_stream: Failed to add audio stream output with error %s\n",
                       [[addStreamOutputError localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
            [addStreamOutputError release];
            return !did_add_output;
        }
    }
    os_event_init(&sc->stream_start_completed, OS_EVENT_TYPE_MANUAL);

    __block BOOL did_stream_start = NO;
    [sc->disp startCaptureWithCompletionHandler:^(NSError *_Nullable error) {
        did_stream_start = (BOOL) (error == nil);
        if (!did_stream_start) {
            MACCAP_ERR("init_screen_stream: Failed to start capture with error %s\n",
                       [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
            // Clean up disp so it isn't stopped
            [sc->disp release];
            sc->disp = NULL;
        }
        os_event_signal(sc->stream_start_completed);
    }];
    os_event_wait(sc->stream_start_completed);

    return did_stream_start;
}

API_AVAILABLE(macos(12.5)) static void *sck_video_capture_create(obs_data_t *settings, obs_source_t *source)
{
    struct screen_capture *sc = bzalloc(sizeof(struct screen_capture));

    sc->source = source;
    sc->hide_cursor = !obs_data_get_bool(settings, "show_cursor");
    sc->hide_obs = obs_data_get_bool(settings, "hide_obs");
    sc->show_empty_names = obs_data_get_bool(settings, "show_empty_names");
    sc->show_hidden_windows = obs_data_get_bool(settings, "show_hidden_windows");
    sc->window = (CGWindowID) obs_data_get_int(settings, "window");
    sc->capture_type = (unsigned int) obs_data_get_int(settings, "type");
    sc->audio_only = false;

    os_sem_init(&sc->shareable_content_available, 1);
    screen_capture_build_content_list(sc, sc->capture_type == ScreenCaptureDisplayStream);

    sc->capture_delegate = [[ScreenCaptureDelegate alloc] init];
    sc->capture_delegate.sc = sc;

    obs_enter_graphics();
    if (gs_get_device_type() == GS_DEVICE_OPENGL) {
        sc->effect = obs_get_base_effect(OBS_EFFECT_DEFAULT_RECT);
    } else {
        sc->effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    }
    obs_leave_graphics();

    if (!sc->effect)
        goto fail;

    sc->display = get_display_migrate_settings(settings);

    sc->application_id = [[NSString alloc] initWithUTF8String:obs_data_get_string(settings, "application")];
    pthread_mutex_init(&sc->mutex, NULL);

    if (!init_screen_stream(sc))
        goto fail;

    return sc;

fail:
    sck_video_capture_destroy(sc);
    return NULL;
}

API_AVAILABLE(macos(12.5)) static void sck_video_capture_tick(void *data, float seconds __unused)
{
    struct screen_capture *sc = data;

    if (!sc->current)
        return;
    if (!obs_source_showing(sc->source))
        return;

    IOSurfaceRef prev_prev = sc->prev;
    if (pthread_mutex_lock(&sc->mutex))
        return;
    sc->prev = sc->current;
    sc->current = NULL;
    pthread_mutex_unlock(&sc->mutex);

    if (prev_prev == sc->prev)
        return;

    obs_enter_graphics();
    if (sc->tex)
        gs_texture_rebind_iosurface(sc->tex, sc->prev);
    else
        sc->tex = gs_texture_create_from_iosurface(sc->prev);
    obs_leave_graphics();

    if (prev_prev) {
        IOSurfaceDecrementUseCount(prev_prev);
        CFRelease(prev_prev);
    }
}

API_AVAILABLE(macos(12.5)) static void sck_video_capture_render(void *data, gs_effect_t *effect __unused)
{
    struct screen_capture *sc = data;

    if (!sc->tex)
        return;

    const bool previous = gs_framebuffer_srgb_enabled();
    gs_enable_framebuffer_srgb(true);

    gs_eparam_t *param = gs_effect_get_param_by_name(sc->effect, "image");
    gs_effect_set_texture(param, sc->tex);

    while (gs_effect_loop(sc->effect, "DrawD65P3"))
        gs_draw_sprite(sc->tex, 0, 0, 0);

    gs_enable_framebuffer_srgb(previous);
}

static const char *sck_video_capture_getname(void *unused __unused)
{
    if (@available(macOS 13.0, *))
        return obs_module_text("SCK.Name");
    else
        return obs_module_text("SCK.Name.Beta");
}

API_AVAILABLE(macos(12.5)) static uint32_t sck_video_capture_getwidth(void *data)
{
    struct screen_capture *sc = data;

    return (uint32_t) sc->frame.size.width;
}

API_AVAILABLE(macos(12.5)) static uint32_t sck_video_capture_getheight(void *data)
{
    struct screen_capture *sc = data;

    return (uint32_t) sc->frame.size.height;
}

static void sck_video_capture_defaults(obs_data_t *settings)
{
    CGDirectDisplayID initial_display = 0;
    {
        NSScreen *mainScreen = [NSScreen mainScreen];
        if (mainScreen) {
            NSNumber *screen_num = mainScreen.deviceDescription[@"NSScreenNumber"];
            if (screen_num) {
                initial_display = (CGDirectDisplayID) (uintptr_t) screen_num.pointerValue;
            }
        }
    }

    CFUUIDRef display_uuid = CGDisplayCreateUUIDFromDisplayID(initial_display);
    CFStringRef uuid_string = CFUUIDCreateString(kCFAllocatorDefault, display_uuid);
    obs_data_set_default_string(settings, "display_uuid", CFStringGetCStringPtr(uuid_string, kCFStringEncodingUTF8));
    CFRelease(uuid_string);
    CFRelease(display_uuid);

    obs_data_set_default_string(settings, "application", NULL);
    obs_data_set_default_int(settings, "type", ScreenCaptureDisplayStream);
    obs_data_set_default_int(settings, "window", kCGNullWindowID);
    obs_data_set_default_bool(settings, "show_cursor", true);
    obs_data_set_default_bool(settings, "hide_obs", false);
    obs_data_set_default_bool(settings, "show_empty_names", false);
    obs_data_set_default_bool(settings, "show_hidden_windows", false);
}

API_AVAILABLE(macos(12.5)) static void sck_video_capture_update(void *data, obs_data_t *settings)
{
    struct screen_capture *sc = data;

    CGWindowID old_window_id = sc->window;
    CGWindowID new_window_id = (CGWindowID) obs_data_get_int(settings, "window");

    if (new_window_id > 0 && new_window_id != old_window_id)
        sc->window = new_window_id;

    ScreenCaptureStreamType capture_type = (ScreenCaptureStreamType) obs_data_get_int(settings, "type");

    CGDirectDisplayID display = get_display_migrate_settings(settings);

    NSString *application_id = [[NSString alloc] initWithUTF8String:obs_data_get_string(settings, "application")];
    bool show_cursor = obs_data_get_bool(settings, "show_cursor");
    bool hide_obs = obs_data_get_bool(settings, "hide_obs");
    bool show_empty_names = obs_data_get_bool(settings, "show_empty_names");
    bool show_hidden_windows = obs_data_get_bool(settings, "show_hidden_windows");

    if (capture_type == sc->capture_type) {
        switch (sc->capture_type) {
            case ScreenCaptureDisplayStream: {
                if (sc->display == display && sc->hide_cursor != show_cursor && sc->hide_obs == hide_obs) {
                    [application_id release];
                    return;
                }
            } break;
            case ScreenCaptureWindowStream: {
                if (old_window_id == sc->window && sc->hide_cursor != show_cursor) {
                    [application_id release];
                    return;
                }
            } break;
            case ScreenCaptureApplicationStream: {
                if (sc->display == display && [application_id isEqualToString:sc->application_id] &&
                    sc->hide_cursor != show_cursor) {
                    [application_id release];
                    return;
                }
            } break;
        }
    }

    obs_enter_graphics();

    destroy_screen_stream(sc);
    sc->capture_type = capture_type;
    sc->display = display;
    [sc->application_id release];
    sc->application_id = application_id;
    sc->hide_cursor = !show_cursor;
    sc->hide_obs = hide_obs;
    sc->show_empty_names = show_empty_names;
    sc->show_hidden_windows = show_hidden_windows;
    init_screen_stream(sc);

    obs_leave_graphics();
}

#pragma mark - obs_properties

API_AVAILABLE(macos(12.5))
static bool content_settings_changed(void *data, obs_properties_t *props, obs_property_t *list __unused,
                                     obs_data_t *settings)
{
    struct screen_capture *sc = data;

    unsigned int capture_type_id = (unsigned int) obs_data_get_int(settings, "type");
    obs_property_t *display_list = obs_properties_get(props, "display_uuid");
    obs_property_t *window_list = obs_properties_get(props, "window");
    obs_property_t *app_list = obs_properties_get(props, "application");
    obs_property_t *empty = obs_properties_get(props, "show_empty_names");
    obs_property_t *hidden = obs_properties_get(props, "show_hidden_windows");
    obs_property_t *hide_obs = obs_properties_get(props, "hide_obs");

    obs_property_t *capture_type_error = obs_properties_get(props, "capture_type_info");

    if (sc->capture_type != capture_type_id) {
        switch (capture_type_id) {
            case 0: {
                obs_property_set_visible(display_list, true);
                obs_property_set_visible(window_list, false);
                obs_property_set_visible(app_list, false);
                obs_property_set_visible(empty, false);
                obs_property_set_visible(hidden, false);
                obs_property_set_visible(hide_obs, true);

                if (capture_type_error) {
                    obs_property_set_visible(capture_type_error, true);
                }
                break;
            }
            case 1: {
                obs_property_set_visible(display_list, false);
                obs_property_set_visible(window_list, true);
                obs_property_set_visible(app_list, false);
                obs_property_set_visible(empty, true);
                obs_property_set_visible(hidden, true);
                obs_property_set_visible(hide_obs, false);

                if (capture_type_error) {
                    obs_property_set_visible(capture_type_error, false);
                }
                break;
            }
            case 2: {
                obs_property_set_visible(display_list, true);
                obs_property_set_visible(app_list, true);
                obs_property_set_visible(window_list, false);
                obs_property_set_visible(empty, false);
                obs_property_set_visible(hidden, true);
                obs_property_set_visible(hide_obs, false);

                if (capture_type_error) {
                    obs_property_set_visible(capture_type_error, true);
                }
                break;
            }
        }
    }

    sc->show_empty_names = obs_data_get_bool(settings, "show_empty_names");
    sc->show_hidden_windows = obs_data_get_bool(settings, "show_hidden_windows");
    sc->hide_obs = obs_data_get_bool(settings, "hide_obs");

    screen_capture_build_content_list(sc, capture_type_id == ScreenCaptureDisplayStream);
    build_display_list(sc, props);
    build_window_list(sc, props);
    build_application_list(sc, props);

    return true;
}

API_AVAILABLE(macos(12.5))
static bool reactivate_capture(obs_properties_t *props __unused, obs_property_t *property, void *data)
{
    struct screen_capture *sc = data;
    if (!sc->capture_failed) {
        MACCAP_LOG(LOG_WARNING, "Tried to reactivate capture that hadn't failed.");
        return false;
    }

    obs_enter_graphics();
    destroy_screen_stream(sc);
    sc->capture_failed = false;
    init_screen_stream(sc);
    obs_leave_graphics();
    obs_property_set_enabled(property, false);
    return true;
}

API_AVAILABLE(macos(12.5)) static obs_properties_t *sck_video_capture_properties(void *data)
{
    struct screen_capture *sc = data;

    obs_properties_t *props = obs_properties_create();

    obs_property_t *capture_type = obs_properties_add_list(props, "type", obs_module_text("SCK.Method"),
                                                           OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

    obs_property_list_add_int(capture_type, obs_module_text("DisplayCapture"), 0);
    obs_property_list_add_int(capture_type, obs_module_text("WindowCapture"), 1);
    obs_property_list_add_int(capture_type, obs_module_text("ApplicationCapture"), 2);

    obs_property_t *display_list = obs_properties_add_list(
        props, "display_uuid", obs_module_text("DisplayCapture.Display"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

    obs_property_t *app_list = obs_properties_add_list(props, "application", obs_module_text("Application"),
                                                       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

    obs_property_t *window_list = obs_properties_add_list(props, "window", obs_module_text("WindowUtils.Window"),
                                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

    obs_property_t *empty =
        obs_properties_add_bool(props, "show_empty_names", obs_module_text("WindowUtils.ShowEmptyNames"));

    obs_property_t *hidden =
        obs_properties_add_bool(props, "show_hidden_windows", obs_module_text("WindowUtils.ShowHidden"));

    obs_properties_add_bool(props, "show_cursor", obs_module_text("DisplayCapture.ShowCursor"));

    obs_property_t *hide_obs = obs_properties_add_bool(props, "hide_obs", obs_module_text("DisplayCapture.HideOBS"));
    obs_property_t *reactivate =
        obs_properties_add_button2(props, "reactivate_capture", obs_module_text("SCK.Restart"), reactivate_capture, sc);
    obs_property_set_enabled(reactivate, sc->capture_failed);

    if (sc) {
        obs_property_set_modified_callback2(capture_type, content_settings_changed, sc);

        obs_property_set_modified_callback2(hidden, content_settings_changed, sc);

        switch (sc->capture_type) {
            case 0: {
                obs_property_set_visible(display_list, true);
                obs_property_set_visible(window_list, false);
                obs_property_set_visible(app_list, false);
                obs_property_set_visible(empty, false);
                obs_property_set_visible(hidden, false);
                obs_property_set_visible(hide_obs, true);
                break;
            }
            case 1: {
                obs_property_set_visible(display_list, false);
                obs_property_set_visible(window_list, true);
                obs_property_set_visible(app_list, false);
                obs_property_set_visible(empty, true);
                obs_property_set_visible(hidden, true);
                obs_property_set_visible(hide_obs, false);
                break;
            }
            case 2: {
                obs_property_set_visible(display_list, true);
                obs_property_set_visible(app_list, true);
                obs_property_set_visible(window_list, false);
                obs_property_set_visible(empty, false);
                obs_property_set_visible(hidden, true);
                obs_property_set_visible(hide_obs, false);
                break;
            }
        }

        obs_property_set_modified_callback2(empty, content_settings_changed, sc);
    }

    if (@available(macOS 13.0, *))
        ;
    else {
        obs_property_t *audio_warning =
            obs_properties_add_text(props, "audio_info", obs_module_text("SCK.AudioUnavailable"), OBS_TEXT_INFO);
        obs_property_text_set_info_type(audio_warning, OBS_TEXT_INFO_WARNING);

        obs_property_t *capture_type_error = obs_properties_add_text(
            props, "capture_type_info", obs_module_text("SCK.CaptureTypeUnavailable"), OBS_TEXT_INFO);

        obs_property_text_set_info_type(capture_type_error, OBS_TEXT_INFO_ERROR);

        if (sc) {
            switch (sc->capture_type) {
                case ScreenCaptureDisplayStream: {
                    obs_property_set_visible(capture_type_error, true);
                    break;
                }
                case ScreenCaptureWindowStream: {
                    obs_property_set_visible(capture_type_error, false);
                    break;
                }
                case ScreenCaptureApplicationStream: {
                    obs_property_set_visible(capture_type_error, true);
                    break;
                }
            }
        } else {
            obs_property_set_visible(capture_type_error, false);
        }
    }

    return props;
}

enum gs_color_space sck_video_capture_get_color_space(void *data, size_t count,
                                                      const enum gs_color_space *preferred_spaces)
{
    UNUSED_PARAMETER(data);

    for (size_t i = 0; i < count; ++i) {
        if (preferred_spaces[i] == GS_CS_SRGB_16F)
            return GS_CS_SRGB_16F;
    }

    for (size_t i = 0; i < count; ++i) {
        if (preferred_spaces[i] == GS_CS_709_EXTENDED)
            return GS_CS_709_EXTENDED;
    }

    for (size_t i = 0; i < count; ++i) {
        if (preferred_spaces[i] == GS_CS_SRGB)
            return GS_CS_SRGB;
    }

    return GS_CS_SRGB_16F;
}

#pragma mark - obs_source_info

API_AVAILABLE(macos(12.5))
struct obs_source_info sck_video_capture_info = {
    .id = "screen_capture",
    .type = OBS_SOURCE_TYPE_INPUT,
    .get_name = sck_video_capture_getname,

    .create = sck_video_capture_create,
    .destroy = sck_video_capture_destroy,

    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_SRGB |
                    OBS_SOURCE_AUDIO,
    .video_tick = sck_video_capture_tick,
    .video_render = sck_video_capture_render,

    .get_width = sck_video_capture_getwidth,
    .get_height = sck_video_capture_getheight,

    .get_defaults = sck_video_capture_defaults,
    .get_properties = sck_video_capture_properties,
    .update = sck_video_capture_update,
    .icon_type = OBS_ICON_TYPE_DESKTOP_CAPTURE,
    .video_get_color_space = sck_video_capture_get_color_space,
};
