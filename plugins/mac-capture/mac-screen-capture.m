#include <AvailabilityMacros.h>
#include <Cocoa/Cocoa.h>

bool is_screen_capture_available(void)
{
	if (@available(macOS 12.5, *)) {
		return true;
	} else {
		return false;
	}
}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 120300 // __MAC_12_3
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

#define MACCAP_LOG(level, msg, ...) \
	blog(level, "[ mac-screencapture ]: " msg, ##__VA_ARGS__)
#define MACCAP_ERR(msg, ...) MACCAP_LOG(LOG_ERROR, msg, ##__VA_ARGS__)

typedef enum {
	ScreenCaptureDisplayStream = 0,
	ScreenCaptureWindowStream = 1,
	ScreenCaptureApplicationStream = 2,
} ScreenCaptureStreamType;

@interface ScreenCaptureDelegate : NSObject <SCStreamOutput>

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
	CGDirectDisplayID display;
	CGWindowID window;
	NSString *application_id;
};

#pragma mark -

static void destroy_screen_stream(struct screen_capture *sc)
{
	if (sc->disp) {
		[sc->disp stopCaptureWithCompletionHandler:^(
				  NSError *_Nullable error) {
			if (error && error.code != 3808) {
				MACCAP_ERR(
					"destroy_screen_stream: Failed to stop stream with error %s\n",
					[[error localizedFailureReason]
						cStringUsingEncoding:
							NSUTF8StringEncoding]);
			}
			os_event_signal(sc->disp_finished);
		}];
		os_event_wait(sc->disp_finished);
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

	os_event_destroy(sc->disp_finished);
	os_event_destroy(sc->stream_start_completed);
}

static void screen_capture_destroy(void *data)
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

static inline void screen_stream_video_update(struct screen_capture *sc,
					      CMSampleBufferRef sample_buffer)
{
	bool frame_detail_errored = false;
	float scale_factor = 1.0f;
	CGRect window_rect = {};

	CFArrayRef attachments_array =
		CMSampleBufferGetSampleAttachmentsArray(sample_buffer, false);
	if (sc->capture_type == ScreenCaptureWindowStream &&
	    attachments_array != NULL &&
	    CFArrayGetCount(attachments_array) > 0) {
		CFDictionaryRef attachments_dict =
			CFArrayGetValueAtIndex(attachments_array, 0);
		if (attachments_dict != NULL) {

			CFTypeRef frame_scale_factor = CFDictionaryGetValue(
				attachments_dict, SCStreamFrameInfoScaleFactor);
			if (frame_scale_factor != NULL) {
				Boolean result = CFNumberGetValue(
					(CFNumberRef)frame_scale_factor,
					kCFNumberFloatType, &scale_factor);
				if (result == false) {
					scale_factor = 1.0f;
					frame_detail_errored = true;
				}
			}

			CFTypeRef content_rect_dict = CFDictionaryGetValue(
				attachments_dict, SCStreamFrameInfoContentRect);
			CFTypeRef content_scale_factor = CFDictionaryGetValue(
				attachments_dict,
				SCStreamFrameInfoContentScale);
			if ((content_rect_dict != NULL) &&
			    (content_scale_factor != NULL)) {
				CGRect content_rect = {};
				float points_to_pixels = 0.0f;

				Boolean result =
					CGRectMakeWithDictionaryRepresentation(
						(__bridge CFDictionaryRef)
							content_rect_dict,
						&content_rect);
				if (result == false) {
					content_rect = CGRectZero;
					frame_detail_errored = true;
				}
				result = CFNumberGetValue(
					(CFNumberRef)content_scale_factor,
					kCFNumberFloatType, &points_to_pixels);
				if (result == false) {
					points_to_pixels = 1.0f;
					frame_detail_errored = true;
				}

				window_rect.origin = content_rect.origin;
				window_rect.size.width =
					content_rect.size.width /
					points_to_pixels * scale_factor;
				window_rect.size.height =
					content_rect.size.height /
					points_to_pixels * scale_factor;
			}
		}
	}

	CVImageBufferRef image_buffer =
		CMSampleBufferGetImageBuffer(sample_buffer);

	CVPixelBufferLockBaseAddress(image_buffer, 0);
	IOSurfaceRef frame_surface = CVPixelBufferGetIOSurface(image_buffer);
	CVPixelBufferUnlockBaseAddress(image_buffer, 0);

	IOSurfaceRef prev_current = NULL;

	if (frame_surface && !pthread_mutex_lock(&sc->mutex)) {

		bool needs_to_update_properties = false;

		if (!frame_detail_errored) {
			if (sc->capture_type == ScreenCaptureWindowStream) {
				if ((sc->frame.size.width !=
				     window_rect.size.width) ||
				    (sc->frame.size.height !=
				     window_rect.size.height)) {
					sc->frame.size.width =
						window_rect.size.width;
					sc->frame.size.height =
						window_rect.size.height;
					needs_to_update_properties = true;
				}
			} else {
				size_t width =
					CVPixelBufferGetWidth(image_buffer);
				size_t height =
					CVPixelBufferGetHeight(image_buffer);

				if ((sc->frame.size.width != width) ||
				    (sc->frame.size.height != height)) {
					sc->frame.size.width = width;
					sc->frame.size.height = height;
					needs_to_update_properties = true;
				}
			}
		}

		if (needs_to_update_properties) {
			[sc->stream_properties
				setWidth:(size_t)sc->frame.size.width];
			[sc->stream_properties
				setHeight:(size_t)sc->frame.size.height];

			[sc->disp
				updateConfiguration:sc->stream_properties
				  completionHandler:^(
					  NSError *_Nullable error) {
					  if (error) {
						  MACCAP_ERR(
							  "screen_stream_video_update: Failed to update stream properties with error %s\n",
							  [[error localizedFailureReason]
								  cStringUsingEncoding:
									  NSUTF8StringEncoding]);
					  }
				  }];
		}

		prev_current = sc->current;
		sc->current = frame_surface;
		CFRetain(sc->current);
		IOSurfaceIncrementUseCount(sc->current);

		pthread_mutex_unlock(&sc->mutex);
	}

	if (prev_current) {
		IOSurfaceDecrementUseCount(prev_current);
		CFRelease(prev_current);
	}
}

static inline void screen_stream_audio_update(struct screen_capture *sc,
					      CMSampleBufferRef sample_buffer)
{
	CMFormatDescriptionRef format_description =
		CMSampleBufferGetFormatDescription(sample_buffer);
	const AudioStreamBasicDescription *audio_description =
		CMAudioFormatDescriptionGetStreamBasicDescription(
			format_description);

	if (audio_description->mChannelsPerFrame < 1) {
		MACCAP_ERR(
			"screen_stream_audio_update: Received sample buffer has less than 1 channel per frame (mChannelsPerFrame set to '%d')\n",
			audio_description->mChannelsPerFrame);
		return;
	}

	char *_Nullable bytes = NULL;
	CMBlockBufferRef data_buffer =
		CMSampleBufferGetDataBuffer(sample_buffer);
	size_t data_buffer_length = CMBlockBufferGetDataLength(data_buffer);
	CMBlockBufferGetDataPointer(data_buffer, 0, &data_buffer_length, NULL,
				    &bytes);

	CMTime presentation_time =
		CMSampleBufferGetOutputPresentationTimeStamp(sample_buffer);

	struct obs_source_audio audio_data = {};

	for (uint32_t channel_idx = 0;
	     channel_idx < audio_description->mChannelsPerFrame;
	     ++channel_idx) {
		uint32_t offset =
			(uint32_t)(data_buffer_length /
				   audio_description->mChannelsPerFrame) *
			channel_idx;
		audio_data.data[channel_idx] = (uint8_t *)bytes + offset;
	}

	audio_data.frames = (uint32_t)(data_buffer_length /
				       audio_description->mBytesPerFrame /
				       audio_description->mChannelsPerFrame);
	audio_data.speakers = audio_description->mChannelsPerFrame;
	audio_data.samples_per_sec = (uint32_t)audio_description->mSampleRate;
	audio_data.timestamp =
		(uint64_t)(CMTimeGetSeconds(presentation_time) * NSEC_PER_SEC);
	audio_data.format = AUDIO_FORMAT_FLOAT_PLANAR;
	obs_source_output_audio(sc->source, &audio_data);
}

static bool init_screen_stream(struct screen_capture *sc)
{
	SCContentFilter *content_filter;

	sc->frame = CGRectZero;
	sc->stream_properties = [[SCStreamConfiguration alloc] init];
	os_sem_wait(sc->shareable_content_available);

	SCDisplay * (^get_target_display)() = ^SCDisplay *()
	{
		__block SCDisplay *target_display = nil;
		[sc->shareable_content.displays
			indexOfObjectPassingTest:^BOOL(
				SCDisplay *_Nonnull display, NSUInteger idx,
				BOOL *_Nonnull stop) {
				if (display.displayID == sc->display) {
					target_display = sc->shareable_content
								 .displays[idx];
					*stop = TRUE;
				}
				return *stop;
			}];
		return target_display;
	};

	void (^set_display_mode)(struct screen_capture *, SCDisplay *) = ^void(
		struct screen_capture *capture_data,
		SCDisplay *target_display) {
		CGDisplayModeRef display_mode =
			CGDisplayCopyDisplayMode(target_display.displayID);
		[capture_data->stream_properties
			setWidth:CGDisplayModeGetPixelWidth(display_mode)];
		[capture_data->stream_properties
			setHeight:CGDisplayModeGetPixelHeight(display_mode)];
		CGDisplayModeRelease(display_mode);
	};

	switch (sc->capture_type) {
	case ScreenCaptureDisplayStream: {
		SCDisplay *target_display = get_target_display();

		if (sc->hide_obs) {
			__block SCRunningApplication *obsApp = nil;
			[sc->shareable_content
					.applications indexOfObjectPassingTest:^BOOL(
							      SCRunningApplication
								      *_Nonnull app,
							      NSUInteger idx
								      __unused,
							      BOOL *_Nonnull stop) {
				if ([app.bundleIdentifier
					    isEqualToString:
						    [[NSBundle mainBundle]
							    bundleIdentifier]]) {
					obsApp = app;
					*stop = TRUE;
				}
				return *stop;
			}];

			NSArray *exclusions =
				[[NSArray alloc] initWithObjects:obsApp, nil];

			NSArray *empty = [[NSArray alloc] init];
			content_filter = [[SCContentFilter alloc]
				      initWithDisplay:target_display
				excludingApplications:exclusions
				     exceptingWindows:empty];
			[empty release];
			[exclusions release];
		} else {
			NSArray *empty = [[NSArray alloc] init];
			content_filter = [[SCContentFilter alloc]
				 initWithDisplay:target_display
				excludingWindows:empty];
			[empty release];
		}

		set_display_mode(sc, target_display);
	} break;
	case ScreenCaptureWindowStream: {
		__block SCWindow *target_window = nil;
		if (sc->window != 0) {
			[sc->shareable_content.windows
				indexOfObjectPassingTest:^BOOL(
					SCWindow *_Nonnull window,
					NSUInteger idx, BOOL *_Nonnull stop) {
					if (window.windowID == sc->window) {
						target_window =
							sc->shareable_content
								.windows[idx];
						*stop = TRUE;
					}
					return *stop;
				}];
		} else {
			target_window =
				[sc->shareable_content.windows objectAtIndex:0];
			sc->window = target_window.windowID;
		}
		content_filter = [[SCContentFilter alloc]
			initWithDesktopIndependentWindow:target_window];

		if (target_window) {
			[sc->stream_properties
				setWidth:(size_t)target_window.frame.size.width];
			[sc->stream_properties
				setHeight:(size_t)target_window.frame.size
						  .height];
		}

	} break;
	case ScreenCaptureApplicationStream: {
		SCDisplay *target_display = get_target_display();
		__block SCRunningApplication *target_application = nil;
		{
			[sc->shareable_content.applications
				indexOfObjectPassingTest:^BOOL(
					SCRunningApplication
						*_Nonnull application,
					NSUInteger idx, BOOL *_Nonnull stop) {
					if ([application.bundleIdentifier
						    isEqualToString:
							    sc->
							    application_id]) {
						target_application =
							sc->shareable_content
								.applications
									[idx];
						*stop = TRUE;
					}
					return *stop;
				}];
		}
		NSArray *target_application_array = [[NSArray alloc]
			initWithObjects:target_application, nil];

		NSArray *empty_array = [[NSArray alloc] init];
		content_filter = [[SCContentFilter alloc]
			      initWithDisplay:target_display
			includingApplications:target_application_array
			     exceptingWindows:empty_array];
		[target_application_array release];
		[empty_array release];

		set_display_mode(sc, target_display);
	} break;
	}
	os_sem_post(sc->shareable_content_available);
	[sc->stream_properties setQueueDepth:8];
	[sc->stream_properties setShowsCursor:!sc->hide_cursor];
	[sc->stream_properties setColorSpaceName:kCGColorSpaceDisplayP3];
	FourCharCode l10r_type = 0;
	l10r_type = ('l' << 24) | ('1' << 16) | ('0' << 8) | 'r';
	[sc->stream_properties setPixelFormat:l10r_type];

	if (@available(macOS 13.0, *)) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 130000
		[sc->stream_properties setCapturesAudio:TRUE];
		[sc->stream_properties setExcludesCurrentProcessAudio:TRUE];
		[sc->stream_properties setChannelCount:2];
#endif
	} else {
		if (sc->capture_type != ScreenCaptureWindowStream) {
			sc->disp = NULL;
			[content_filter release];
			os_event_init(&sc->disp_finished, OS_EVENT_TYPE_MANUAL);
			os_event_init(&sc->stream_start_completed,
				      OS_EVENT_TYPE_MANUAL);
			return true;
		}
	}

	sc->disp = [[SCStream alloc] initWithFilter:content_filter
				      configuration:sc->stream_properties
					   delegate:nil];

	[content_filter release];

	NSError *addStreamOutputError = nil;
	BOOL did_add_output = [sc->disp addStreamOutput:sc->capture_delegate
						   type:SCStreamOutputTypeScreen
				     sampleHandlerQueue:nil
						  error:&addStreamOutputError];
	if (!did_add_output) {
		MACCAP_ERR(
			"init_screen_stream: Failed to add stream output with error %s\n",
			[[addStreamOutputError localizedFailureReason]
				cStringUsingEncoding:NSUTF8StringEncoding]);
		[addStreamOutputError release];
		return !did_add_output;
	}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 130000
	if (@available(macOS 13.0, *)) {
		did_add_output = [sc->disp
			   addStreamOutput:sc->capture_delegate
				      type:SCStreamOutputTypeAudio
			sampleHandlerQueue:nil
				     error:&addStreamOutputError];
		if (!did_add_output) {
			MACCAP_ERR(
				"init_screen_stream: Failed to add audio stream output with error %s\n",
				[[addStreamOutputError localizedFailureReason]
					cStringUsingEncoding:
						NSUTF8StringEncoding]);
			[addStreamOutputError release];
			return !did_add_output;
		}
	}
#endif
	os_event_init(&sc->disp_finished, OS_EVENT_TYPE_MANUAL);
	os_event_init(&sc->stream_start_completed, OS_EVENT_TYPE_MANUAL);

	__block BOOL did_stream_start = false;
	[sc->disp startCaptureWithCompletionHandler:^(
			  NSError *_Nullable error) {
		did_stream_start = (BOOL)(error == nil);
		if (!did_stream_start) {
			MACCAP_ERR(
				"init_screen_stream: Failed to start capture with error %s\n",
				[[error localizedFailureReason]
					cStringUsingEncoding:
						NSUTF8StringEncoding]);
			// Clean up disp so it isn't stopped
			[sc->disp release];
			sc->disp = NULL;
		}
		os_event_signal(sc->stream_start_completed);
	}];
	os_event_wait(sc->stream_start_completed);

	return did_stream_start;
}

static void screen_capture_build_content_list(struct screen_capture *sc,
					      bool display_capture)
{
	typedef void (^shareable_content_callback)(SCShareableContent *,
						   NSError *);
	shareable_content_callback new_content_received = ^void(
		SCShareableContent *shareable_content, NSError *error) {
		if (error == nil && sc->shareable_content_available != NULL) {
			sc->shareable_content = [shareable_content retain];
		} else {
#ifdef DEBUG
			MACCAP_ERR(
				"screen_capture_properties: Failed to get shareable content with error %s\n",
				[[error localizedFailureReason]
					cStringUsingEncoding:
						NSUTF8StringEncoding]);
#endif
			MACCAP_LOG(
				LOG_WARNING,
				"Unable to get list of available applications or windows. "
				"Please check if OBS has necessary screen capture permissions.");
		}
		os_sem_post(sc->shareable_content_available);
	};

	os_sem_wait(sc->shareable_content_available);
	[sc->shareable_content release];
	[SCShareableContent
		getShareableContentExcludingDesktopWindows:TRUE
				       onScreenWindowsOnly:
					       (display_capture
							? FALSE
							: !sc->show_hidden_windows)
				       completionHandler:new_content_received];
}

static void *screen_capture_create(obs_data_t *settings, obs_source_t *source)
{
	struct screen_capture *sc = bzalloc(sizeof(struct screen_capture));

	sc->source = source;
	sc->hide_cursor = !obs_data_get_bool(settings, "show_cursor");
	sc->hide_obs = obs_data_get_bool(settings, "hide_obs");
	sc->show_empty_names = obs_data_get_bool(settings, "show_empty_names");
	sc->show_hidden_windows =
		obs_data_get_bool(settings, "show_hidden_windows");
	sc->window = (CGWindowID)obs_data_get_int(settings, "window");
	sc->capture_type = (unsigned int)obs_data_get_int(settings, "type");

	os_sem_init(&sc->shareable_content_available, 1);
	screen_capture_build_content_list(
		sc, sc->capture_type == ScreenCaptureDisplayStream);

	sc->capture_delegate = [[ScreenCaptureDelegate alloc] init];
	sc->capture_delegate.sc = sc;

	sc->effect = obs_get_base_effect(OBS_EFFECT_DEFAULT_RECT);
	if (!sc->effect)
		goto fail;

	bool legacy_display_id = obs_data_has_user_value(settings, "display");
	if (legacy_display_id) {
		CGDirectDisplayID display_id =
			(CGDirectDisplayID)obs_data_get_int(settings,
							    "display");
		CFUUIDRef display_uuid =
			CGDisplayCreateUUIDFromDisplayID(display_id);
		CFStringRef uuid_string =
			CFUUIDCreateString(kCFAllocatorDefault, display_uuid);
		obs_data_set_string(
			settings, "display_uuid",
			CFStringGetCStringPtr(uuid_string,
					      kCFStringEncodingUTF8));
		obs_data_erase(settings, "display");
		CFRelease(uuid_string);
		CFRelease(display_uuid);
	}

	const char *display_uuid =
		obs_data_get_string(settings, "display_uuid");
	if (display_uuid) {
		CFStringRef uuid_string = CFStringCreateWithCString(
			kCFAllocatorDefault, display_uuid,
			kCFStringEncodingUTF8);
		CFUUIDRef uuid_ref = CFUUIDCreateFromString(kCFAllocatorDefault,
							    uuid_string);
		sc->display = CGDisplayGetDisplayIDFromUUID(uuid_ref);
		CFRelease(uuid_string);
		CFRelease(uuid_ref);
	} else {
		sc->display = CGMainDisplayID();
	}

	sc->application_id = [[NSString alloc]
		initWithUTF8String:obs_data_get_string(settings,
						       "application")];
	pthread_mutex_init(&sc->mutex, NULL);

	if (!init_screen_stream(sc))
		goto fail;

	return sc;

fail:
	obs_leave_graphics();
	screen_capture_destroy(sc);
	return NULL;
}

static void screen_capture_video_tick(void *data, float seconds __unused)
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

static void screen_capture_video_render(void *data,
					gs_effect_t *effect __unused)
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

static const char *screen_capture_getname(void *unused __unused)
{
	if (@available(macOS 13.0, *))
		return obs_module_text("SCK.Name");
	else
		return obs_module_text("SCK.Name.Beta");
}

static uint32_t screen_capture_getwidth(void *data)
{
	struct screen_capture *sc = data;

	return (uint32_t)sc->frame.size.width;
}

static uint32_t screen_capture_getheight(void *data)
{
	struct screen_capture *sc = data;

	return (uint32_t)sc->frame.size.height;
}

static void screen_capture_defaults(obs_data_t *settings)
{
	CGDirectDisplayID initial_display = 0;
	{
		NSScreen *mainScreen = [NSScreen mainScreen];
		if (mainScreen) {
			NSNumber *screen_num =
				mainScreen.deviceDescription[@"NSScreenNumber"];
			if (screen_num) {
				initial_display =
					(CGDirectDisplayID)(uintptr_t)
						screen_num.pointerValue;
			}
		}
	}

	CFUUIDRef display_uuid =
		CGDisplayCreateUUIDFromDisplayID(initial_display);
	CFStringRef uuid_string =
		CFUUIDCreateString(kCFAllocatorDefault, display_uuid);
	obs_data_set_default_string(
		settings, "display_uuid",
		CFStringGetCStringPtr(uuid_string, kCFStringEncodingUTF8));
	CFRelease(uuid_string);
	CFRelease(display_uuid);

	obs_data_set_default_obj(settings, "application", NULL);
	obs_data_set_default_int(settings, "type", ScreenCaptureDisplayStream);
	obs_data_set_default_int(settings, "window", kCGNullWindowID);
	obs_data_set_default_bool(settings, "show_cursor", true);
	obs_data_set_default_bool(settings, "hide_obs", false);
	obs_data_set_default_bool(settings, "show_empty_names", false);
	obs_data_set_default_bool(settings, "show_hidden_windows", false);
}

static void screen_capture_update(void *data, obs_data_t *settings)
{
	struct screen_capture *sc = data;

	CGWindowID old_window_id = sc->window;
	CGWindowID new_window_id =
		(CGWindowID)obs_data_get_int(settings, "window");

	if (new_window_id > 0 && new_window_id != old_window_id)
		sc->window = new_window_id;

	ScreenCaptureStreamType capture_type =
		(ScreenCaptureStreamType)obs_data_get_int(settings, "type");

	CFStringRef uuid_string = CFStringCreateWithCString(
		kCFAllocatorDefault,
		obs_data_get_string(settings, "display_uuid"),
		kCFStringEncodingUTF8);
	CFUUIDRef display_uuid =
		CFUUIDCreateFromString(kCFAllocatorDefault, uuid_string);
	CGDirectDisplayID display = CGDisplayGetDisplayIDFromUUID(display_uuid);
	CFRelease(uuid_string);
	CFRelease(display_uuid);

	NSString *application_id = [[NSString alloc]
		initWithUTF8String:obs_data_get_string(settings,
						       "application")];
	bool show_cursor = obs_data_get_bool(settings, "show_cursor");
	bool hide_obs = obs_data_get_bool(settings, "hide_obs");
	bool show_empty_names = obs_data_get_bool(settings, "show_empty_names");
	bool show_hidden_windows =
		obs_data_get_bool(settings, "show_hidden_windows");

	if (capture_type == sc->capture_type) {
		switch (sc->capture_type) {
		case ScreenCaptureDisplayStream: {
			if (sc->display == display &&
			    sc->hide_cursor != show_cursor &&
			    sc->hide_obs == hide_obs) {
				[application_id release];
				return;
			}
		} break;
		case ScreenCaptureWindowStream: {
			if (old_window_id == sc->window &&
			    sc->hide_cursor != show_cursor) {
				[application_id release];
				return;
			}
		} break;
		case ScreenCaptureApplicationStream: {
			if (sc->display == display &&
			    [application_id
				    isEqualToString:sc->application_id] &&
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

static bool build_display_list(struct screen_capture *sc,
			       obs_properties_t *props)
{
	os_sem_wait(sc->shareable_content_available);

	obs_property_t *display_list =
		obs_properties_get(props, "display_uuid");
	obs_property_list_clear(display_list);

	[sc->shareable_content.displays enumerateObjectsUsingBlock:^(
						SCDisplay *_Nonnull display,
						NSUInteger idx __unused,
						BOOL *_Nonnull _stop __unused) {
		NSUInteger screen_index =
			[NSScreen.screens indexOfObjectPassingTest:^BOOL(
						  NSScreen *_Nonnull screen,
						  NSUInteger index __unused,
						  BOOL *_Nonnull stop) {
				NSNumber *screen_num =
					screen.deviceDescription
						[@"NSScreenNumber"];
				CGDirectDisplayID screen_display_id =
					(CGDirectDisplayID)screen_num.intValue;
				*stop = (screen_display_id ==
					 display.displayID);

				return *stop;
			}];
		NSScreen *screen =
			[NSScreen.screens objectAtIndex:screen_index];

		char dimension_buffer[4][12] = {};
		char name_buffer[256] = {};
		snprintf(dimension_buffer[0], sizeof(dimension_buffer[0]), "%u",
			 (uint32_t)screen.frame.size.width);
		snprintf(dimension_buffer[1], sizeof(dimension_buffer[0]), "%u",
			 (uint32_t)screen.frame.size.height);
		snprintf(dimension_buffer[2], sizeof(dimension_buffer[0]), "%d",
			 (int32_t)screen.frame.origin.x);
		snprintf(dimension_buffer[3], sizeof(dimension_buffer[0]), "%d",
			 (int32_t)screen.frame.origin.y);

		snprintf(name_buffer, sizeof(name_buffer),
			 "%.200s: %.12sx%.12s @ %.12s,%.12s",
			 screen.localizedName.UTF8String, dimension_buffer[0],
			 dimension_buffer[1], dimension_buffer[2],
			 dimension_buffer[3]);

		CFUUIDRef display_uuid =
			CGDisplayCreateUUIDFromDisplayID(display.displayID);
		CFStringRef uuid_string =
			CFUUIDCreateString(kCFAllocatorDefault, display_uuid);
		obs_property_list_add_string(
			display_list, name_buffer,
			CFStringGetCStringPtr(uuid_string,
					      kCFStringEncodingUTF8));
		CFRelease(uuid_string);
		CFRelease(display_uuid);
	}];

	os_sem_post(sc->shareable_content_available);
	return true;
}

static bool build_window_list(struct screen_capture *sc,
			      obs_properties_t *props)
{
	os_sem_wait(sc->shareable_content_available);

	obs_property_t *window_list = obs_properties_get(props, "window");
	obs_property_list_clear(window_list);

	NSArray<SCWindow *> *filteredWindows;
	filteredWindows = [sc->shareable_content.windows
		filteredArrayUsingPredicate:
			[NSPredicate predicateWithBlock:^BOOL(
					     SCWindow *window,
					     NSDictionary *bindings __unused) {
				NSString *app_name =
					window.owningApplication.applicationName;
				NSString *title = window.title;
				if (!sc->show_empty_names) {
					if (app_name == NULL || title == NULL) {
						return false;
					} else if ([app_name
							   isEqualToString:@""] ||
						   [title isEqualToString:@""]) {
						return false;
					}
				}
				return true;
			}]];

	NSArray<SCWindow *> *sortedWindows;
	sortedWindows =
		[filteredWindows sortedArrayUsingComparator:^NSComparisonResult(
					 SCWindow *window, SCWindow *other) {
			NSComparisonResult appNameCmp =
				[window.owningApplication.applicationName
					compare:other.owningApplication
							.applicationName
					options:NSCaseInsensitiveSearch];
			if (appNameCmp == NSOrderedAscending) {
				return NSOrderedAscending;
			} else if (appNameCmp == NSOrderedSame) {
				return [window.title
					compare:other.title
					options:NSCaseInsensitiveSearch];
			} else {
				return NSOrderedDescending;
			}
		}];

	[sortedWindows enumerateObjectsUsingBlock:^(
			       SCWindow *_Nonnull window,
			       NSUInteger idx __unused,
			       BOOL *_Nonnull stop __unused) {
		NSString *app_name = window.owningApplication.applicationName;
		NSString *title = window.title;

		const char *list_text =
			[[NSString stringWithFormat:@"[%@] %@", app_name, title]
				UTF8String];
		obs_property_list_add_int(window_list, list_text,
					  window.windowID);
	}];

	os_sem_post(sc->shareable_content_available);
	return true;
}

static bool build_application_list(struct screen_capture *sc,
				   obs_properties_t *props)
{
	os_sem_wait(sc->shareable_content_available);

	obs_property_t *application_list =
		obs_properties_get(props, "application");
	obs_property_list_clear(application_list);

	NSArray<SCRunningApplication *> *filteredApplications;
	filteredApplications = [sc->shareable_content.applications
		filteredArrayUsingPredicate:
			[NSPredicate predicateWithBlock:^BOOL(
					     SCRunningApplication *app,
					     NSDictionary *bindings __unused) {
				const char *name =
					[app.applicationName UTF8String];
				if (strcmp(name, "") == 0) {
					return false;
				}
				return true;
			}]];

	NSArray<SCRunningApplication *> *sortedApplications;
	sortedApplications = [filteredApplications
		sortedArrayUsingComparator:^NSComparisonResult(
			SCRunningApplication *app,
			SCRunningApplication *other) {
			return [app.applicationName
				compare:other.applicationName
				options:NSCaseInsensitiveSearch];
		}];

	[sortedApplications enumerateObjectsUsingBlock:^(
				    SCRunningApplication *_Nonnull application,
				    NSUInteger idx __unused,
				    BOOL *_Nonnull stop __unused) {
		const char *name = [application.applicationName UTF8String];
		const char *bundle_id =
			[application.bundleIdentifier UTF8String];
		obs_property_list_add_string(application_list, name, bundle_id);
	}];

	os_sem_post(sc->shareable_content_available);
	return true;
}

static bool content_settings_changed(void *data, obs_properties_t *props,
				     obs_property_t *list __unused,
				     obs_data_t *settings)
{
	struct screen_capture *sc = data;

	unsigned int capture_type_id =
		(unsigned int)obs_data_get_int(settings, "type");
	obs_property_t *display_list =
		obs_properties_get(props, "display_uuid");
	obs_property_t *window_list = obs_properties_get(props, "window");
	obs_property_t *app_list = obs_properties_get(props, "application");
	obs_property_t *empty = obs_properties_get(props, "show_empty_names");
	obs_property_t *hidden =
		obs_properties_get(props, "show_hidden_windows");
	obs_property_t *hide_obs = obs_properties_get(props, "hide_obs");

	obs_property_t *capture_type_error =
		obs_properties_get(props, "capture_type_info");

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
				obs_property_set_visible(capture_type_error,
							 true);
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
				obs_property_set_visible(capture_type_error,
							 false);
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
				obs_property_set_visible(capture_type_error,
							 true);
			}
			break;
		}
		}
	}

	screen_capture_build_content_list(
		sc, capture_type_id == ScreenCaptureDisplayStream);
	build_display_list(sc, props);
	build_window_list(sc, props);
	build_application_list(sc, props);

	sc->show_empty_names = obs_data_get_bool(settings, "show_empty_names");
	sc->show_hidden_windows =
		obs_data_get_bool(settings, "show_hidden_windows");

	sc->hide_obs = obs_data_get_bool(settings, "hide_obs");

	return true;
}

static obs_properties_t *screen_capture_properties(void *data)
{
	struct screen_capture *sc = data;

	obs_properties_t *props = obs_properties_create();

	obs_property_t *capture_type = obs_properties_add_list(
		props, "type", obs_module_text("SCK.Method"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(capture_type,
				  obs_module_text("DisplayCapture"), 0);
	obs_property_list_add_int(capture_type,
				  obs_module_text("WindowCapture"), 1);
	obs_property_list_add_int(capture_type,
				  obs_module_text("ApplicationCapture"), 2);

	obs_property_t *display_list = obs_properties_add_list(
		props, "display_uuid",
		obs_module_text("DisplayCapture.Display"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);

	obs_property_t *app_list = obs_properties_add_list(
		props, "application", obs_module_text("Application"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_t *window_list = obs_properties_add_list(
		props, "window", obs_module_text("WindowUtils.Window"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_t *empty = obs_properties_add_bool(
		props, "show_empty_names",
		obs_module_text("WindowUtils.ShowEmptyNames"));

	obs_property_t *hidden = obs_properties_add_bool(
		props, "show_hidden_windows",
		obs_module_text("WindowUtils.ShowHidden"));

	obs_properties_add_bool(props, "show_cursor",
				obs_module_text("DisplayCapture.ShowCursor"));

	obs_property_t *hide_obs = obs_properties_add_bool(
		props, "hide_obs", obs_module_text("DisplayCapture.HideOBS"));

	if (sc) {
		obs_property_set_modified_callback2(
			capture_type, content_settings_changed, sc);

		obs_property_set_modified_callback2(
			hidden, content_settings_changed, sc);

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

		obs_property_set_modified_callback2(
			empty, content_settings_changed, sc);
	}

	if (@available(macOS 13.0, *))
		;
	else {
		obs_property_t *audio_warning = obs_properties_add_text(
			props, "audio_info",
			obs_module_text("SCK.AudioUnavailable"), OBS_TEXT_INFO);
		obs_property_text_set_info_type(audio_warning,
						OBS_TEXT_INFO_WARNING);

		obs_property_t *capture_type_error = obs_properties_add_text(
			props, "capture_type_info",
			obs_module_text("SCK.CaptureTypeUnavailable"),
			OBS_TEXT_INFO);

		obs_property_text_set_info_type(capture_type_error,
						OBS_TEXT_INFO_ERROR);

		if (sc) {
			switch (sc->capture_type) {
			case ScreenCaptureDisplayStream: {
				obs_property_set_visible(capture_type_error,
							 true);
				break;
			}
			case ScreenCaptureWindowStream: {
				obs_property_set_visible(capture_type_error,
							 false);
				break;
			}
			case ScreenCaptureApplicationStream: {
				obs_property_set_visible(capture_type_error,
							 true);
				break;
			}
			}
		} else {
			obs_property_set_visible(capture_type_error, false);
		}
	}

	return props;
}

enum gs_color_space screen_capture_video_get_color_space(
	void *data, size_t count, const enum gs_color_space *preferred_spaces)
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

struct obs_source_info screen_capture_info = {
	.id = "screen_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.get_name = screen_capture_getname,

	.create = screen_capture_create,
	.destroy = screen_capture_destroy,

	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
			OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_SRGB |
			OBS_SOURCE_AUDIO,
	.video_tick = screen_capture_video_tick,
	.video_render = screen_capture_video_render,

	.get_width = screen_capture_getwidth,
	.get_height = screen_capture_getheight,

	.get_defaults = screen_capture_defaults,
	.get_properties = screen_capture_properties,
	.update = screen_capture_update,
	.icon_type = OBS_ICON_TYPE_DESKTOP_CAPTURE,
	.video_get_color_space = screen_capture_video_get_color_space,
};

#pragma mark - ScreenCaptureDelegate

@implementation ScreenCaptureDelegate

- (void)stream:(SCStream *)stream
	didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
		       ofType:(SCStreamOutputType)type
{
	if (self.sc != NULL) {
		if (type == SCStreamOutputTypeScreen) {
			screen_stream_video_update(self.sc, sampleBuffer);
		}
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 130000
		else if (@available(macOS 13.0, *)) {
			if (type == SCStreamOutputTypeAudio) {
				screen_stream_audio_update(self.sc,
							   sampleBuffer);
			}
		}
#endif
	}
}

@end

// "-Wunguarded-availability-new"
#pragma clang diagnostic pop
#endif
