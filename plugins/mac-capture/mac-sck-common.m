#include "mac-sck-common.h"

bool is_screen_capture_available(void)
{
    if (@available(macOS 12.5, *)) {
        return true;
    } else {
        return false;
    }
}

#pragma mark - ScreenCaptureDelegate

@implementation ScreenCaptureDelegate

- (void)stream:(SCStream *)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type
{
    if (self.sc != NULL) {
        if (type == SCStreamOutputTypeScreen && !self.sc->audio_only) {
            screen_stream_video_update(self.sc, sampleBuffer);
        } else if (@available(macOS 13.0, *)) {
            if (type == SCStreamOutputTypeAudio) {
                screen_stream_audio_update(self.sc, sampleBuffer);
            }
        }
    }
}

- (void)stream:(SCStream *)stream didStopWithError:(NSError *)error
{
    NSString *errorMessage;
    switch (error.code) {
        case SCStreamErrorUserStopped:
            errorMessage = @"User stopped stream.";
            break;
        case SCStreamErrorNoCaptureSource:
            errorMessage = @"Stream stopped as no capture source was not found.";
            break;
        default:
            errorMessage = [NSString stringWithFormat:@"Stream stopped with error %ld (\"%s\")", error.code,
                                                      error.localizedDescription.UTF8String];
            break;
    }

    MACCAP_LOG(LOG_WARNING, "%s", errorMessage.UTF8String);

    [self.sc->picker_observer disable];
    self.sc->capture_failed = true;
    obs_source_update_properties(self.sc->source);
}

@end

@implementation OBSSharingPickerObserver

- (void)enable
{
    if (!self.picker) {
        self.picker = SCContentSharingPicker.sharedPicker;
        self.picker.active = YES;

        if (!self.observerAdded) {
            [self.picker addObserver:self];
            self.observerAdded = YES;
        }
    } else {
        self.observerAdded = NO;
    }
}

- (void)disable
{
    if (self.picker && self.observerAdded) {
        [self.picker removeObserver:self];
    }
    self.picker = NULL;
}

- (void)showPicker
{
    [self.picker present];
}

- (void)showPicker:(SCStream *)stream
{
    SCContentSharingPickerConfiguration *config = [[SCContentSharingPickerConfiguration alloc] init];

    if (self.sc->hide_obs) {
        config.excludedBundleIDs = @[[NSBundle mainBundle].bundleIdentifier];
    }

    [self.picker setConfiguration:config forStream:stream];

    [self.picker presentPickerForStream:stream];
}

- (void)contentSharingPicker:(SCContentSharingPicker *)picker
         didUpdateWithFilter:(SCContentFilter *)filter
                   forStream:(SCStream *)stream
{
    if (stream) {
        if (stream == self.sc->disp) {
            [stream updateContentFilter:filter completionHandler:nil];
            self.sc->picked_filter = filter;
            [self.sc->picked_filter retain];

            if (self.sc->capture_type != ScreenCaptureAutomaticStream) {
                obs_data_t *settings = obs_source_get_settings(self.sc->source);
                obs_data_set_int(settings, "type", ScreenCaptureAutomaticStream);
                obs_source_update(self.sc->source, settings);
            }
        }
    } else {
        NSString *pickerType = NULL;

        switch (self.sc->capture_type) {
            case ScreenCaptureDisplayStream: {
                pickerType = @"Display Capture";
                break;
            }
            case ScreenCaptureWindowStream: {
                pickerType = @"Window Capture";
                break;
            }
            case ScreenCaptureApplicationStream: {
                pickerType = @"Application Capture";
                break;
            }
            default: {
                self.sc->frame = CGRectZero;
                self.sc->stream_properties = [[SCStreamConfiguration alloc] init];

                CGColorRef background = CGColorGetConstantColor(kCGColorClear);
                [self.sc->stream_properties setQueueDepth:8];
                [self.sc->stream_properties setShowsCursor:!self.sc->hide_cursor];
                [self.sc->stream_properties setColorSpaceName:kCGColorSpaceDisplayP3];
                [self.sc->stream_properties setBackgroundColor:background];
                FourCharCode l10r_type = 0;
                l10r_type = ('l' << 24) | ('1' << 16) | ('0' << 8) | 'r';
                [self.sc->stream_properties setPixelFormat:l10r_type];

                [self.sc->stream_properties setCapturesAudio:YES];
                [self.sc->stream_properties setExcludesCurrentProcessAudio:YES];
                [self.sc->stream_properties setChannelCount:2];

                self.sc->disp = [[SCStream alloc] initWithFilter:filter configuration:self.sc->stream_properties
                                                        delegate:self.sc->capture_delegate];

                NSError *addStreamOutputError = nil;

                BOOL success = [self.sc->disp addStreamOutput:self.sc->capture_delegate type:SCStreamOutputTypeScreen
                                           sampleHandlerQueue:nil
                                                        error:&addStreamOutputError];
                if (!success) {
                    MACCAP_ERR("didUpdateWithFilter: Failed to add stream output with error %s\n",
                               [[addStreamOutputError localizedFailureReason]
                                   cStringUsingEncoding:NSUTF8StringEncoding]);
                    [addStreamOutputError release];
                    return;
                }

                success = [self.sc->disp addStreamOutput:self.sc->capture_delegate type:SCStreamOutputTypeAudio
                                      sampleHandlerQueue:nil
                                                   error:&addStreamOutputError];

                if (!success) {
                    MACCAP_ERR("didUpdateWithFilter: Failed to add audio stream output with error %s\n",
                               [[addStreamOutputError localizedFailureReason]
                                   cStringUsingEncoding:NSUTF8StringEncoding]);
                    [addStreamOutputError release];
                    return;
                }

                [self.sc->disp startCaptureWithCompletionHandler:^(NSError *_Nullable error) {
                    if (error) {
                        MACCAP_ERR("didUpdateWithFilter: Failed to start capture with error %s\n",
                                   [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
                        [self.sc->disp release];
                        self.sc->disp = NULL;
                    }
                }];

                self.sc->picked_filter = filter;
                [self.sc->picked_filter retain];

                return;
            }
        }

        blog(LOG_WARNING, "Attempted to use macOS screen sharing picker to change capture of type %s",
             pickerType.UTF8String);
    }
}

- (void)contentSharingPicker:(nonnull SCContentSharingPicker *)picker didCancelForStream:(nullable SCStream *)stream
{
    return;
}

- (void)contentSharingPickerStartDidFailWithError:(nonnull NSError *)error
{
    if (error) {
        MACCAP_ERR("didFailWithError: Sharing picker failed with error %s\n",
                   [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
    }
}

@end

#pragma mark - obs_properties

void screen_capture_build_content_list(struct screen_capture *sc, ScreenCaptureStreamType captureType)
{
    typedef void (^shareable_content_callback)(SCShareableContent *, NSError *);
    shareable_content_callback new_content_received = ^void(SCShareableContent *shareable_content, NSError *error) {
        if (error == nil && sc->shareable_content_available != NULL) {
            sc->shareable_content = [shareable_content retain];
        } else {
#ifdef DEBUG
            MACCAP_ERR("screen_capture_properties: Failed to get shareable content with error %s\n",
                       [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
#endif
            MACCAP_LOG(LOG_WARNING, "Unable to get list of available applications or windows. "
                                    "Please check if OBS has necessary screen capture permissions.");
        }
        os_sem_post(sc->shareable_content_available);
    };

    os_sem_wait(sc->shareable_content_available);
    [sc->shareable_content release];

    if (captureType != ScreenCaptureAutomaticStream) {
        BOOL onScreenWindowsOnly = (captureType == ScreenCaptureDisplayStream) ? NO : !sc->show_hidden_windows;
        [SCShareableContent getShareableContentExcludingDesktopWindows:YES onScreenWindowsOnly:onScreenWindowsOnly
                                                     completionHandler:new_content_received];
        return;
    }

    os_sem_post(sc->shareable_content_available);
}

bool build_display_list(struct screen_capture *sc, obs_properties_t *props)
{
    os_sem_wait(sc->shareable_content_available);

    obs_property_t *display_list = obs_properties_get(props, "display_uuid");
    obs_property_list_clear(display_list);

    for (SCDisplay *display in sc->shareable_content.displays) {
        NSScreen *display_screen = nil;
        for (NSScreen *screen in NSScreen.screens) {
            NSNumber *screen_num = screen.deviceDescription[@"NSScreenNumber"];
            CGDirectDisplayID screen_display_id = (CGDirectDisplayID) screen_num.intValue;
            if (screen_display_id == display.displayID) {
                display_screen = screen;
                break;
            }
        }
        if (!display_screen) {
            continue;
        }

        char dimension_buffer[4][12] = {};
        char name_buffer[256] = {};
        snprintf(dimension_buffer[0], sizeof(dimension_buffer[0]), "%u", (uint32_t) display_screen.frame.size.width);
        snprintf(dimension_buffer[1], sizeof(dimension_buffer[0]), "%u", (uint32_t) display_screen.frame.size.height);
        snprintf(dimension_buffer[2], sizeof(dimension_buffer[0]), "%d", (int32_t) display_screen.frame.origin.x);
        snprintf(dimension_buffer[3], sizeof(dimension_buffer[0]), "%d", (int32_t) display_screen.frame.origin.y);

        snprintf(name_buffer, sizeof(name_buffer), "%.200s: %.12sx%.12s @ %.12s,%.12s",
                 display_screen.localizedName.UTF8String, dimension_buffer[0], dimension_buffer[1], dimension_buffer[2],
                 dimension_buffer[3]);

        CFUUIDRef display_uuid = CGDisplayCreateUUIDFromDisplayID(display.displayID);
        CFStringRef uuid_string = CFUUIDCreateString(kCFAllocatorDefault, display_uuid);
        obs_property_list_add_string(display_list, name_buffer,
                                     CFStringGetCStringPtr(uuid_string, kCFStringEncodingUTF8));
        CFRelease(uuid_string);
        CFRelease(display_uuid);
    }

    os_sem_post(sc->shareable_content_available);
    return true;
}

bool build_window_list(struct screen_capture *sc, obs_properties_t *props)
{
    os_sem_wait(sc->shareable_content_available);

    obs_property_t *window_list = obs_properties_get(props, "window");
    obs_property_list_clear(window_list);

    NSPredicate *filteredWindowPredicate =
        [NSPredicate predicateWithBlock:^BOOL(SCWindow *window, NSDictionary *bindings __unused) {
            NSString *app_name = window.owningApplication.applicationName;
            NSString *title = window.title;
            if (!sc->show_empty_names) {
                return (app_name.length > 0) && (title.length > 0);
            } else {
                return YES;
            }
        }];
    NSArray<SCWindow *> *filteredWindows;
    filteredWindows = [sc->shareable_content.windows filteredArrayUsingPredicate:filteredWindowPredicate];

    NSArray<SCWindow *> *sortedWindows;
    sortedWindows = [filteredWindows sortedArrayUsingComparator:^NSComparisonResult(SCWindow *window, SCWindow *other) {
        NSComparisonResult appNameCmp = [window.owningApplication.applicationName
            compare:other.owningApplication.applicationName
            options:NSCaseInsensitiveSearch];
        if (appNameCmp == NSOrderedAscending) {
            return NSOrderedAscending;
        } else if (appNameCmp == NSOrderedSame) {
            return [window.title compare:other.title options:NSCaseInsensitiveSearch];
        } else {
            return NSOrderedDescending;
        }
    }];

    for (SCWindow *window in sortedWindows) {
        NSString *app_name = window.owningApplication.applicationName;
        NSString *title = window.title;

        const char *list_text = [[NSString stringWithFormat:@"[%@] %@", app_name, title] UTF8String];
        obs_property_list_add_int(window_list, list_text, window.windowID);
    }

    os_sem_post(sc->shareable_content_available);
    return true;
}

bool build_application_list(struct screen_capture *sc, obs_properties_t *props)
{
    os_sem_wait(sc->shareable_content_available);

    obs_property_t *application_list = obs_properties_get(props, "application");
    obs_property_list_clear(application_list);

    NSArray<SCRunningApplication *> *filteredApplications;
    filteredApplications = [sc->shareable_content.applications
        filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(SCRunningApplication *app,
                                                                          NSDictionary *bindings __unused) {
            return app.applicationName.length > 0;
        }]];

    NSArray<SCRunningApplication *> *sortedApplications;
    sortedApplications = [filteredApplications
        sortedArrayUsingComparator:^NSComparisonResult(SCRunningApplication *app, SCRunningApplication *other) {
            return [app.applicationName compare:other.applicationName options:NSCaseInsensitiveSearch];
        }];

    for (SCRunningApplication *application in sortedApplications) {
        const char *name = [application.applicationName UTF8String];
        const char *bundle_id = [application.bundleIdentifier UTF8String];
        obs_property_list_add_string(application_list, name, bundle_id);
    }

    os_sem_post(sc->shareable_content_available);
    return true;
}

#pragma mark - audio/video

API_AVAILABLE(macos(12.5)) void screen_stream_video_update(struct screen_capture *sc, CMSampleBufferRef sample_buffer)
{
    bool frame_detail_errored = false;
    float scale_factor = 1.0f;
    CGRect window_rect = {};

    CFArrayRef attachments_array = CMSampleBufferGetSampleAttachmentsArray(sample_buffer, false);
    if ((sc->capture_type == ScreenCaptureWindowStream || sc->capture_type == ScreenCaptureAutomaticStream) &&
        attachments_array != NULL && CFArrayGetCount(attachments_array) > 0) {
        CFDictionaryRef attachments_dict = CFArrayGetValueAtIndex(attachments_array, 0);
        if (attachments_dict != NULL) {
            CFTypeRef frame_scale_factor = CFDictionaryGetValue(attachments_dict, SCStreamFrameInfoScaleFactor);
            if (frame_scale_factor != NULL) {
                Boolean result = CFNumberGetValue((CFNumberRef) frame_scale_factor, kCFNumberFloatType, &scale_factor);
                if (result == false) {
                    scale_factor = 1.0f;
                    frame_detail_errored = true;
                }
            }

            CFTypeRef content_rect_dict = CFDictionaryGetValue(attachments_dict, SCStreamFrameInfoContentRect);
            CFTypeRef content_scale_factor = CFDictionaryGetValue(attachments_dict, SCStreamFrameInfoContentScale);
            if ((content_rect_dict != NULL) && (content_scale_factor != NULL)) {
                CGRect content_rect = {};
                float points_to_pixels = 0.0f;

                Boolean result =
                    CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef) content_rect_dict, &content_rect);
                if (result == false) {
                    content_rect = CGRectZero;
                    frame_detail_errored = true;
                }
                result = CFNumberGetValue((CFNumberRef) content_scale_factor, kCFNumberFloatType, &points_to_pixels);
                if (result == false) {
                    points_to_pixels = 1.0f;
                    frame_detail_errored = true;
                }

                window_rect.origin = content_rect.origin;
                window_rect.size.width = content_rect.size.width / points_to_pixels * scale_factor;
                window_rect.size.height = content_rect.size.height / points_to_pixels * scale_factor;
            }
        }
    }

    CVImageBufferRef image_buffer = CMSampleBufferGetImageBuffer(sample_buffer);

    CVPixelBufferLockBaseAddress(image_buffer, 0);
    IOSurfaceRef frame_surface = CVPixelBufferGetIOSurface(image_buffer);
    CVPixelBufferUnlockBaseAddress(image_buffer, 0);

    IOSurfaceRef prev_current = NULL;

    if (frame_surface && !pthread_mutex_lock(&sc->mutex)) {
        bool needs_to_update_properties = false;

        if (!frame_detail_errored) {
            if ((sc->capture_type == ScreenCaptureWindowStream || sc->capture_type == ScreenCaptureAutomaticStream)) {
                if ((sc->frame.size.width != window_rect.size.width) ||
                    (sc->frame.size.height != window_rect.size.height)) {
                    sc->frame.size.width = window_rect.size.width;
                    sc->frame.size.height = window_rect.size.height;
                    needs_to_update_properties = true;
                }
            } else {
                size_t width = CVPixelBufferGetWidth(image_buffer);
                size_t height = CVPixelBufferGetHeight(image_buffer);

                if ((sc->frame.size.width != width) || (sc->frame.size.height != height)) {
                    sc->frame.size.width = width;
                    sc->frame.size.height = height;
                    needs_to_update_properties = true;
                }
            }
        }

        if (needs_to_update_properties) {
            [sc->stream_properties setWidth:(size_t) sc->frame.size.width];
            [sc->stream_properties setHeight:(size_t) sc->frame.size.height];

            [sc->disp updateConfiguration:sc->stream_properties completionHandler:^(NSError *_Nullable error) {
                if (error) {
                    MACCAP_ERR("screen_stream_video_update: Failed to update stream properties with error %s\n",
                               [[error localizedFailureReason] cStringUsingEncoding:NSUTF8StringEncoding]);
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

void screen_stream_audio_update(struct screen_capture *sc, CMSampleBufferRef sample_buffer)
{
    CMFormatDescriptionRef format_description = CMSampleBufferGetFormatDescription(sample_buffer);
    const AudioStreamBasicDescription *audio_description =
        CMAudioFormatDescriptionGetStreamBasicDescription(format_description);

    if (audio_description->mChannelsPerFrame < 1) {
        MACCAP_ERR(
            "screen_stream_audio_update: Received sample buffer has less than 1 channel per frame (mChannelsPerFrame set to '%d')\n",
            audio_description->mChannelsPerFrame);
        return;
    }

    char *_Nullable bytes = NULL;
    CMBlockBufferRef data_buffer = CMSampleBufferGetDataBuffer(sample_buffer);
    size_t data_buffer_length = CMBlockBufferGetDataLength(data_buffer);
    CMBlockBufferGetDataPointer(data_buffer, 0, &data_buffer_length, NULL, &bytes);

    CMTime presentation_time = CMSampleBufferGetOutputPresentationTimeStamp(sample_buffer);

    struct obs_source_audio audio_data = {};

    for (uint32_t channel_idx = 0; channel_idx < audio_description->mChannelsPerFrame; ++channel_idx) {
        uint32_t offset = (uint32_t) (data_buffer_length / audio_description->mChannelsPerFrame) * channel_idx;
        audio_data.data[channel_idx] = (uint8_t *) bytes + offset;
    }

    audio_data.frames =
        (uint32_t) (data_buffer_length / audio_description->mBytesPerFrame / audio_description->mChannelsPerFrame);
    audio_data.speakers = audio_description->mChannelsPerFrame;
    audio_data.samples_per_sec = (uint32_t) audio_description->mSampleRate;
    audio_data.timestamp = (uint64_t) (CMTimeGetSeconds(presentation_time) * NSEC_PER_SEC);
    audio_data.format = AUDIO_FORMAT_FLOAT_PLANAR;
    obs_source_output_audio(sc->source, &audio_data);
}
