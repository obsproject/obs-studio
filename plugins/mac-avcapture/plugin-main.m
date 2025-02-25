//
//  plugin-main.m
//  mac-avcapture
//
//  Created by Patrick Heyer on 2023-03-07.
//

#import "plugin-main.h"

#pragma mark av-capture API

const char *av_capture_get_text(const char *text_id)
{
    return obs_module_text(text_id);
}

static void *av_capture_create(obs_data_t *settings, obs_source_t *source)
{
    OBSAVCaptureInfo *capture_data = bzalloc(sizeof(OBSAVCaptureInfo));
    capture_data->isFastPath = false;
    capture_data->settings = settings;
    capture_data->source = source;
    capture_data->videoFrame = bmalloc(sizeof(OBSAVCaptureVideoFrame));
    capture_data->audioFrame = bmalloc(sizeof(OBSAVCaptureAudioFrame));

    OBSAVCapture *capture = [[OBSAVCapture alloc] initWithCaptureInfo:capture_data];

    return (void *) CFBridgingRetain(capture);
}

static void *av_fast_capture_create(obs_data_t *settings, obs_source_t *source)
{
    OBSAVCaptureInfo *capture_info = bzalloc(sizeof(OBSAVCaptureInfo));
    capture_info->isFastPath = true;
    capture_info->settings = settings;
    capture_info->source = source;
    capture_info->effect = obs_get_base_effect(OBS_EFFECT_DEFAULT_RECT);
    capture_info->frameSize = CGRectZero;

    if (!capture_info->effect) {
        return NULL;
    }

    pthread_mutex_init(&capture_info->mutex, NULL);

    OBSAVCapture *capture = [[OBSAVCapture alloc] initWithCaptureInfo:capture_info];

    return (void *) CFBridgingRetain(capture);
}

static const char *av_capture_get_name(void *av_capture __unused)
{
    return obs_module_text("AVCapture");
}

static const char *av_fast_capture_get_name(void *av_capture __unused)
{
    return obs_module_text("AVCapture_Fast");
}

static void av_capture_set_defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, "device", "");
    obs_data_set_default_bool(settings, "use_preset", true);

    obs_data_set_default_string(settings, "preset", AVCaptureSessionPresetHigh.UTF8String);

    obs_data_set_default_bool(settings, "enable_audio", true);
}

static void av_fast_capture_set_defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, "device", "");
    obs_data_set_default_bool(settings, "use_preset", false);
    obs_data_set_default_bool(settings, "enable_audio", true);
}

static obs_properties_t *av_capture_properties(void *av_capture)
{
    OBSAVCapture *capture = (__bridge OBSAVCapture *) (av_capture);
    OBSAVCaptureInfo *capture_info = capture.captureInfo;
    AVCaptureDevice *device = capture.deviceInput.device;
    NSString *effectsWarningKey = [OBSAVCapture effectsWarningForDevice:device];

    obs_properties_t *properties = obs_properties_create();

    // Create Properties
    obs_property_t *device_list = obs_properties_add_list(properties, "device", obs_module_text("Device"),
                                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

    obs_property_t *effects_warning;
    if (effectsWarningKey) {
        effects_warning = obs_properties_add_text(properties, "effects_warning",
                                                  obs_module_text(effectsWarningKey.UTF8String), OBS_TEXT_INFO);
        obs_property_text_set_info_type(effects_warning, OBS_TEXT_INFO_WARNING);
    }

    obs_property_t *use_preset = obs_properties_add_bool(properties, "use_preset", obs_module_text("UsePreset"));
    obs_property_t *preset_list = obs_properties_add_list(properties, "preset", obs_module_text("Preset"),
                                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_t *supported_formats = obs_properties_add_list(
        properties, "supported_format", obs_module_text("InputFormat"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_t *use_buffering = obs_properties_add_bool(properties, "buffering", obs_module_text("Buffering"));
    obs_property_t *frame_rates = obs_properties_add_frame_rate(properties, "frame_rate", obs_module_text("FrameRate"));

    if (capture_info) {
        bool isFastPath = capture_info->isFastPath;

        // Add Property Visibility and Callbacks
        configure_property(device_list, true, true, properties_changed, capture);
        if (effectsWarningKey) {
            configure_property(effects_warning, true, true, NULL, NULL);
        }

        configure_property(use_preset, !isFastPath, !isFastPath, (!isFastPath) ? properties_changed_use_preset : NULL,
                           capture);
        configure_property(preset_list, !isFastPath, !isFastPath, (!isFastPath) ? properties_changed_preset : NULL,
                           capture);

        configure_property(supported_formats, true, true, properties_changed, capture);

        configure_property(use_buffering, !isFastPath, !isFastPath, NULL, NULL);
        configure_property(frame_rates, isFastPath, isFastPath, NULL, NULL);
    }

    return properties;
}

static void av_capture_update(void *av_capture, obs_data_t *settings)
{
    OBSAVCapture *capture = (__bridge OBSAVCapture *) (av_capture);
    capture.captureInfo->settings = settings;

    [capture updateSessionwithError:NULL];
}

static void av_fast_capture_tick(void *av_capture, float seconds __unused)
{
    OBSAVCapture *capture = (__bridge OBSAVCapture *) (av_capture);
    OBSAVCaptureInfo *capture_info = capture.captureInfo;

    if (!capture_info->currentSurface) {
        return;
    }

    if (!obs_source_showing(capture_info->source)) {
        return;
    }

    IOSurfaceRef previousSurface = capture_info->previousSurface;

    if (pthread_mutex_lock(&capture_info->mutex)) {
        return;
    }

    capture_info->previousSurface = capture_info->currentSurface;
    capture_info->currentSurface = NULL;
    pthread_mutex_unlock(&capture_info->mutex);

    if (previousSurface == capture_info->previousSurface) {
        return;
    }

    if (capture_info->previousSurface) {
        obs_enter_graphics();
        if (capture_info->texture) {
            gs_texture_rebind_iosurface(capture_info->texture, capture_info->previousSurface);
        } else {
            capture_info->texture = gs_texture_create_from_iosurface(capture_info->previousSurface);
        }
        obs_leave_graphics();
    }

    if (previousSurface) {
        IOSurfaceDecrementUseCount(previousSurface);
        CFRelease(previousSurface);
    }
}

static void av_fast_capture_render(void *av_capture, gs_effect_t *effect __unused)
{
    OBSAVCapture *capture = (__bridge OBSAVCapture *) (av_capture);
    OBSAVCaptureInfo *capture_info = capture.captureInfo;

    if (!capture_info->texture) {
        return;
    }

    const bool linear_srgb = gs_get_linear_srgb();

    const bool previous = gs_framebuffer_srgb_enabled();
    gs_enable_framebuffer_srgb(linear_srgb);

    gs_eparam_t *param = gs_effect_get_param_by_name(capture_info->effect, "image");
    gs_effect_set_texture_srgb(param, capture_info->texture);

    if (linear_srgb) {
        gs_effect_set_texture_srgb(param, capture_info->texture);
    } else {
        gs_effect_set_texture(param, capture_info->texture);
    }

    while (gs_effect_loop(capture_info->effect, "Draw")) {
        gs_draw_sprite(capture_info->texture, 0, 0, 0);
    }

    gs_enable_framebuffer_srgb(previous);
}

static UInt32 av_fast_capture_get_width(void *av_capture)
{
    OBSAVCapture *capture = (__bridge OBSAVCapture *) (av_capture);
    OBSAVCaptureInfo *capture_info = capture.captureInfo;

    CGSize frameSize = capture_info->frameSize.size;

    return (UInt32) frameSize.width;
}

static UInt32 av_fast_capture_get_height(void *av_capture)
{
    OBSAVCapture *capture = (__bridge OBSAVCapture *) (av_capture);
    OBSAVCaptureInfo *capture_info = capture.captureInfo;

    CGSize frameSize = capture_info->frameSize.size;

    return (UInt32) frameSize.height;
}

static void av_capture_destroy(void *av_capture)
{
    OBSAVCapture *capture = (__bridge OBSAVCapture *) (av_capture);

    if (!capture) {
        return;
    }

    OBSAVCaptureInfo *capture_info = capture.captureInfo;

    [capture stopCaptureSession];
    [capture.deviceInput.device unlockForConfiguration];

    if (capture_info->isFastPath) {
        pthread_mutex_destroy(&capture_info->mutex);
    }

    if (capture_info->videoFrame) {
        bfree(capture_info->videoFrame);
        capture_info->videoFrame = NULL;
    }

    if (capture_info->audioFrame) {
        bfree(capture_info->audioFrame);
        capture_info->audioFrame = NULL;
    }

    if (capture_info->sampleBufferDescription) {
        capture_info->sampleBufferDescription = NULL;
    }

    bfree(capture_info);

    CFBridgingRelease((__bridge CFTypeRef _Nullable)(capture));
}

#pragma mark - OBS Module API

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("macOS-avcapture", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
    return "macOS AVFoundation Capture Source";
}

bool obs_module_load(void)
{
    struct obs_source_info av_capture_info = {
        .id = "macos-avcapture",
        .type = OBS_SOURCE_TYPE_INPUT,
        .output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
        .create = av_capture_create,
        .get_name = av_capture_get_name,
        .get_defaults = av_capture_set_defaults,
        .get_properties = av_capture_properties,
        .update = av_capture_update,
        .destroy = av_capture_destroy,
        .icon_type = OBS_ICON_TYPE_CAMERA,
    };

    obs_register_source(&av_capture_info);

    struct obs_source_info av_capture_sync_info = {
        .id = "macos-avcapture-fast",
        .type = OBS_SOURCE_TYPE_INPUT,
        .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_AUDIO | OBS_SOURCE_SRGB |
                        OBS_SOURCE_DO_NOT_DUPLICATE,
        .create = av_fast_capture_create,
        .get_name = av_fast_capture_get_name,
        .get_defaults = av_fast_capture_set_defaults,
        .get_properties = av_capture_properties,
        .update = av_capture_update,
        .destroy = av_capture_destroy,
        .video_tick = av_fast_capture_tick,
        .video_render = av_fast_capture_render,
        .get_width = av_fast_capture_get_width,
        .get_height = av_fast_capture_get_height,
        .icon_type = OBS_ICON_TYPE_CAMERA,
    };

    obs_register_source(&av_capture_sync_info);

    return true;
}
