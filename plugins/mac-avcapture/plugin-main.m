//
//  plugin-main.m
//  mac-avcapture
//
//  Created by Patrick Heyer on 2023-03-07.
//

#import "plugin-main.h"
#import <inttypes.h>
#import <stdlib.h>

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
    capture_data->videoFrame = bzalloc(sizeof(OBSAVCaptureVideoFrame));
    capture_data->audioFrame = bzalloc(sizeof(OBSAVCaptureAudioFrame));

    OBSAVCapture *capture = [[OBSAVCapture alloc] initWithCaptureInfo:capture_data];

    return (void *) CFBridgingRetain(capture);
}

static void *av_fast_capture_create(obs_data_t *settings, obs_source_t *source)
{
    OBSAVCaptureInfo *capture_info = bzalloc(sizeof(OBSAVCaptureInfo));
    capture_info->isFastPath = true;
    const char *diagnostics = getenv("OBS_AVCAPTURE_DIAGNOSTICS");
    capture_info->diagnosticsEnabled = diagnostics && strcmp(diagnostics, "1") == 0;
    capture_info->settings = settings;
    capture_info->source = source;

    obs_enter_graphics();
    if (gs_get_device_type() == GS_DEVICE_OPENGL) {
        capture_info->effect = obs_get_base_effect(OBS_EFFECT_DEFAULT_RECT);
    } else {
        capture_info->effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    }
    obs_leave_graphics();

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

    if (!obs_source_showing(capture_info->source)) {
        return;
    }

    if (pthread_mutex_lock(&capture_info->mutex)) {
        return;
    }

    // Test readiness under the same lock used by the capture callback.
    capture_info->diagnosticTicks++;
    IOSurfaceRef incomingSurface = capture_info->currentSurface;
    IOSurfaceRef previousSurface = capture_info->previousSurface;
    if (incomingSurface) {
        capture_info->previousSurface = incomingSurface;
        capture_info->currentSurface = NULL;
        capture_info->diagnosticConsumed++;
        capture_info->diagnosticConsumedPTS = capture_info->diagnosticLatestPTS;
    } else {
        capture_info->diagnosticRepeats++;
    }

    const bool report = capture_info->diagnosticsEnabled && capture_info->diagnosticTicks % 300 == 0;
    const uint64_t samples = capture_info->diagnosticSamples;
    const uint64_t drops = capture_info->diagnosticDrops;
    const uint64_t overwritten = capture_info->diagnosticOverwrites;
    const uint64_t ticks = capture_info->diagnosticTicks;
    const uint64_t repeats = capture_info->diagnosticRepeats;
    const uint64_t consumed = capture_info->diagnosticConsumed;
    const int64_t pts = capture_info->diagnosticConsumedPTS;
    const uint64_t late = capture_info->diagnosticLateDrops;
    const uint64_t buffers = capture_info->diagnosticBufferDrops;
    const uint64_t discontinuities = capture_info->diagnosticDiscontinuities;
    const uint64_t arrivalGap = capture_info->diagnosticMaxArrivalGapNS;
    const int64_t ptsGap = capture_info->diagnosticMaxPTSGapNS;
    if (report) {
        capture_info->diagnosticMaxArrivalGapNS = 0;
        capture_info->diagnosticMaxPTSGapNS = 0;
    }
    pthread_mutex_unlock(&capture_info->mutex);

    // Aggregate logging only, outside the handoff lock. Counters are cumulative.
    if (report) {
        blog(LOG_INFO,
             "[capture-diag] source=%s samples=%" PRIu64 " drops=%" PRIu64 " overwritten=%" PRIu64 " ticks=%" PRIu64
             " no_new_surface=%" PRIu64 " consumed=%" PRIu64 " consumed_pts_ns=%" PRId64,
             obs_source_get_name(capture_info->source), samples, drops, overwritten, ticks, repeats, consumed, pts);
        blog(LOG_INFO,
             "[capture-diag] source=%s late=%" PRIu64 " out_of_buffers=%" PRIu64 " discontinuities=%" PRIu64
             " max_arrival_gap_ns=%" PRIu64 " max_pts_gap_ns=%" PRId64,
             obs_source_get_name(capture_info->source), late, buffers, discontinuities, arrivalGap, ptsGap);
    }
    if (!incomingSurface) {
        return;
    }

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
    /// It is possible that the source's serial queue is still creating this source, so perform destruction
    /// synchronously on that queue to ensure the source is fully initialized before being destroyed.
    dispatch_sync(capture.sessionQueue, ^{
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
    });
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
