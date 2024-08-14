#include "mac-sck-common.h"

const char *sck_audio_capture_getname(void *unused __unused)
{
    return obs_module_text("SCK.Audio.Name");
}

API_AVAILABLE(macos(13.0)) static void destroy_audio_screen_stream(struct screen_capture *sc)
{
    if (sc->disp && !sc->capture_failed) {
        [sc->disp stopCaptureWithCompletionHandler:^(NSError *_Nullable error) {
            if (error && error.code != SCStreamErrorAttemptToStopStreamState) {
                MACCAP_ERR("destroy_audio_screen_stream: Failed to stop stream with error %s\n",
                           [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
            }
        }];
    }

    if (sc->stream_properties) {
        [sc->stream_properties release];
        sc->stream_properties = NULL;
    }

    if (sc->disp) {
        [sc->disp release];
        sc->disp = NULL;
    }

    os_event_destroy(sc->stream_start_completed);
}

API_AVAILABLE(macos(13.0)) static void sck_audio_capture_destroy(void *data)
{
    struct screen_capture *sc = data;

    if (!sc)
        return;

    destroy_audio_screen_stream(sc);

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

API_AVAILABLE(macos(13.0)) static bool init_audio_screen_stream(struct screen_capture *sc)
{
    SCContentFilter *content_filter;
    if (sc->capture_failed) {
        sc->capture_failed = false;
        obs_source_update_properties(sc->source);
    }

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

    switch (sc->audio_capture_type) {
        case ScreenCaptureAudioDesktopStream: {
            SCDisplay *target_display = get_target_display();

            NSArray *empty = [[NSArray alloc] init];
            content_filter = [[SCContentFilter alloc] initWithDisplay:target_display excludingWindows:empty];
            [empty release];
        } break;
        case ScreenCaptureAudioApplicationStream: {
            SCDisplay *target_display = get_target_display();
            SCRunningApplication *target_application = nil;
            for (SCRunningApplication *application in sc->shareable_content.applications) {
                if ([application.bundleIdentifier isEqualToString:sc->application_id]) {
                    target_application = application;
                    break;
                }
            }
            NSArray *target_application_array = [[NSArray alloc] initWithObjects:target_application, nil];

            NSArray *empty = [[NSArray alloc] init];
            content_filter = [[SCContentFilter alloc] initWithDisplay:target_display
                                                includingApplications:target_application_array
                                                     exceptingWindows:empty];
            [target_application_array release];
            [empty release];
        } break;
    }
    os_sem_post(sc->shareable_content_available);
    [sc->stream_properties setQueueDepth:8];

    [sc->stream_properties setCapturesAudio:TRUE];
    [sc->stream_properties setExcludesCurrentProcessAudio:TRUE];

    struct obs_audio_info audio_info;
    BOOL did_get_audio_info = obs_get_audio_info(&audio_info);
    if (!did_get_audio_info) {
        MACCAP_ERR("init_audio_screen_stream: No audio configured, returning %d\n", did_get_audio_info);
        [content_filter release];
        return did_get_audio_info;
    }
    int channel_count = get_audio_channels(audio_info.speakers);
    if (channel_count > 1) {
        [sc->stream_properties setChannelCount:2];
    } else {
        [sc->stream_properties setChannelCount:channel_count];
    }

    sc->disp = [[SCStream alloc] initWithFilter:content_filter configuration:sc->stream_properties
                                       delegate:sc->capture_delegate];
    [content_filter release];

    //add a dummy video stream output to silence errors from SCK. frames are dropped by the delegate
    NSError *error = nil;
    BOOL did_add_output = [sc->disp addStreamOutput:sc->capture_delegate type:SCStreamOutputTypeScreen
                                 sampleHandlerQueue:nil
                                              error:&error];
    if (!did_add_output) {
        MACCAP_ERR("init_audio_screen_stream: Failed to add video stream output with error %s\n",
                   [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
        [error release];
        [sc->disp release];
        sc->disp = NULL;
        return !did_add_output;
    }

    did_add_output = [sc->disp addStreamOutput:sc->capture_delegate type:SCStreamOutputTypeAudio sampleHandlerQueue:nil
                                         error:&error];
    if (!did_add_output) {
        MACCAP_ERR("init_audio_screen_stream: Failed to add audio stream output with error %s\n",
                   [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
        [error release];
        [sc->disp release];
        sc->disp = NULL;
        return !did_add_output;
    }
    os_event_init(&sc->stream_start_completed, OS_EVENT_TYPE_MANUAL);

    __block BOOL did_stream_start = false;
    [sc->disp startCaptureWithCompletionHandler:^(NSError *_Nullable error2) {
        did_stream_start = (BOOL) (error2 == nil);
        if (!did_stream_start) {
            MACCAP_ERR("init_audio_screen_stream: Failed to start capture with error %s\n",
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

static void sck_audio_capture_defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, "application", NULL);
    obs_data_set_default_int(settings, "type", ScreenCaptureAudioDesktopStream);
}

API_AVAILABLE(macos(13.0)) static void *sck_audio_capture_create(obs_data_t *settings, obs_source_t *source)
{
    struct screen_capture *sc = bzalloc(sizeof(struct screen_capture));

    sc->source = source;
    sc->audio_only = true;
    sc->audio_capture_type = (unsigned int) obs_data_get_int(settings, "type");

    os_sem_init(&sc->shareable_content_available, 1);
    screen_capture_build_content_list(sc, sc->capture_type);

    sc->capture_delegate = [[ScreenCaptureDelegate alloc] init];
    sc->capture_delegate.sc = sc;

    sc->display = CGMainDisplayID();
    sc->picker_observer = NULL;

    sc->application_id = [[NSString alloc] initWithUTF8String:obs_data_get_string(settings, "application")];
    pthread_mutex_init(&sc->mutex, NULL);

    if (!init_audio_screen_stream(sc))
        goto fail;

    return sc;

fail:
    sck_audio_capture_destroy(sc);
    return NULL;
}

#pragma mark - obs_properties

API_AVAILABLE(macos(13.0))
static bool audio_capture_method_changed(void *data, obs_properties_t *props, obs_property_t *list __unused,
                                         obs_data_t *settings)
{
    struct screen_capture *sc = data;

    unsigned int capture_type_id = (unsigned int) obs_data_get_int(settings, "type");
    obs_property_t *app_list = obs_properties_get(props, "application");

    switch (capture_type_id) {
        case ScreenCaptureAudioDesktopStream: {
            obs_property_set_visible(app_list, false);
            break;
        }
        case ScreenCaptureAudioApplicationStream: {
            obs_property_set_visible(app_list, true);
            screen_capture_build_content_list(sc, capture_type_id);
            build_application_list(sc, props);
            break;
        }
    }

    return true;
}

API_AVAILABLE(macos(13.0))
static bool reactivate_capture(obs_properties_t *props __unused, obs_property_t *property, void *data)
{
    struct screen_capture *sc = data;
    if (!sc->capture_failed) {
        MACCAP_LOG(LOG_WARNING, "Tried to reactivate capture that hadn't failed.");
        return false;
    }

    destroy_audio_screen_stream(sc);
    sc->capture_failed = false;
    init_audio_screen_stream(sc);
    obs_property_set_enabled(property, false);
    return true;
}

API_AVAILABLE(macos(13.0)) static obs_properties_t *sck_audio_capture_properties(void *data)
{
    struct screen_capture *sc = data;

    obs_properties_t *props = obs_properties_create();

    obs_property_t *capture_type = obs_properties_add_list(props, "type", obs_module_text("SCK.Method"),
                                                           OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

    obs_property_list_add_int(capture_type, obs_module_text("DesktopAudioCapture"), ScreenCaptureAudioDesktopStream);
    obs_property_list_add_int(capture_type, obs_module_text("ApplicationAudioCapture"),
                              ScreenCaptureAudioApplicationStream);

    obs_property_set_modified_callback2(capture_type, audio_capture_method_changed, data);

    obs_property_t *app_list = obs_properties_add_list(props, "application", obs_module_text("Application"),
                                                       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_t *reactivate =
        obs_properties_add_button2(props, "reactivate_capture", obs_module_text("SCK.Restart"), reactivate_capture, sc);
    obs_property_set_enabled(reactivate, sc->capture_failed);

    if (sc) {
        switch (sc->audio_capture_type) {
            case 0: {
                obs_property_set_visible(app_list, false);
                break;
            }
            case 1: {
                obs_property_set_visible(app_list, true);
                break;
            }
        }
    }

    return props;
}

API_AVAILABLE(macos(13.0)) static void sck_audio_capture_update(void *data, obs_data_t *settings)
{
    struct screen_capture *sc = data;

    ScreenCaptureAudioStreamType capture_type = (ScreenCaptureAudioStreamType) obs_data_get_int(settings, "type");
    NSString *application_id = [[NSString alloc] initWithUTF8String:obs_data_get_string(settings, "application")];

    destroy_audio_screen_stream(sc);
    sc->audio_capture_type = capture_type;
    [sc->application_id release];
    sc->application_id = application_id;
    init_audio_screen_stream(sc);
}

#pragma mark - obs_source_info

API_AVAILABLE(macos(13.0))
struct obs_source_info sck_audio_capture_info = {
    .id = "sck_audio_capture",
    .type = OBS_SOURCE_TYPE_INPUT,
    .get_name = sck_audio_capture_getname,

    .create = sck_audio_capture_create,
    .destroy = sck_audio_capture_destroy,

    .output_flags = OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_AUDIO,

    .get_defaults = sck_audio_capture_defaults,
    .get_properties = sck_audio_capture_properties,
    .update = sck_audio_capture_update,
    .icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT,
};
