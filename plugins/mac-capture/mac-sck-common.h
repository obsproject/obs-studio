#include <AvailabilityMacros.h>
#include <Cocoa/Cocoa.h>

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 120300  // __MAC_12_3
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

#include <stdlib.h>
#include <obs-module.h>
#include <util/threading.h>
#include <pthread.h>

#include <IOSurface/IOSurface.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include <CoreMedia/CMSampleBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>

#define MACCAP_LOG(level, msg, ...) blog(level, "[ mac-screencapture ]: " msg, ##__VA_ARGS__)
#define MACCAP_ERR(msg, ...)        MACCAP_LOG(LOG_ERROR, msg, ##__VA_ARGS__)

typedef enum {
    ScreenCaptureDisplayStream = 0,
    ScreenCaptureWindowStream = 1,
    ScreenCaptureApplicationStream = 2,
} ScreenCaptureStreamType;

typedef enum {
    ScreenCaptureAudioDesktopStream = 0,
    ScreenCaptureAudioApplicationStream = 1,
} ScreenCaptureAudioStreamType;

@interface ScreenCaptureDelegate : NSObject <SCStreamOutput, SCStreamDelegate>

@property struct screen_capture *sc;

@end

struct screen_capture {
    obs_source_t *source;

    gs_effect_t *effect;
    gs_texture_t *tex;

    NSRect frame;
    bool hide_cursor;
    bool hide_obs;
    bool show_hidden_windows;
    bool show_empty_names;
    bool audio_only;

    SCStream *disp;
    SCStreamConfiguration *stream_properties;
    SCShareableContent *shareable_content;
    ScreenCaptureDelegate *capture_delegate;

    os_event_t *disp_finished;
    os_event_t *stream_start_completed;
    os_sem_t *shareable_content_available;
    IOSurfaceRef current, prev;

    pthread_mutex_t mutex;

    ScreenCaptureStreamType capture_type;
    ScreenCaptureAudioStreamType audio_capture_type;
    CGDirectDisplayID display;
    CGWindowID window;
    NSString *application_id;
};

bool is_screen_capture_available(void);

void screen_capture_build_content_list(struct screen_capture *sc, bool display_capture);

bool build_display_list(struct screen_capture *sc, obs_properties_t *props);

bool build_window_list(struct screen_capture *sc, obs_properties_t *props);

bool build_application_list(struct screen_capture *sc, obs_properties_t *props);

static const char *screen_capture_getname(void *unused __unused);

void screen_stream_video_update(struct screen_capture *sc, CMSampleBufferRef sample_buffer);

void screen_stream_audio_update(struct screen_capture *sc, CMSampleBufferRef sample_buffer);

#pragma clang diagnostic pop
#endif
