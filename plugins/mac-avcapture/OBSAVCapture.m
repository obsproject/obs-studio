//
//  OBSAVCapture.m
//  mac-avcapture
//
//  Created by Patrick Heyer on 2023-03-07.
//

#import "OBSAVCapture.h"
#import "AVCaptureDeviceFormat+OBSListable.h"

@implementation OBSAVCapture

- (instancetype)init
{
    return [self initWithCaptureInfo:nil];
}

- (instancetype)initWithCaptureInfo:(OBSAVCaptureInfo *)capture_info
{
    self = [super init];

    if (self) {
        CMIOObjectPropertyAddress propertyAddress = {kCMIOHardwarePropertyAllowScreenCaptureDevices,
                                                     kCMIOObjectPropertyScopeGlobal, kCMIOObjectPropertyElementMaster};

        UInt32 allow = 1;
        CMIOObjectSetPropertyData(kCMIOObjectSystemObject, &propertyAddress, 0, NULL, sizeof(allow), &allow);

        _errorDomain = @"com.obsproject.obs-studio.av-capture";

        _presetList = @{
            AVCaptureSessionPresetLow: @"Low",
            AVCaptureSessionPresetMedium: @"Medium",
            AVCaptureSessionPresetHigh: @"High",
            AVCaptureSessionPreset320x240: @"320x240",
            AVCaptureSessionPreset352x288: @"352x288",
            AVCaptureSessionPreset640x480: @"640x480",
            AVCaptureSessionPreset960x540: @"960x540",
            AVCaptureSessionPreset1280x720: @"1280x720",
            AVCaptureSessionPreset1920x1080: @"1920x1080",
            AVCaptureSessionPreset3840x2160: @"3840x2160",
        };

        _sessionQueue = dispatch_queue_create("session queue", DISPATCH_QUEUE_SERIAL);

        OBSAVCaptureVideoInfo newInfo = {0};
        _videoInfo = newInfo;

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(deviceDisconnected:)
                                                     name:AVCaptureDeviceWasDisconnectedNotification
                                                   object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(deviceConnected:)
                                                     name:AVCaptureDeviceWasConnectedNotification
                                                   object:nil];

        if (capture_info) {
            _captureInfo = capture_info;

            NSString *UUID = [OBSAVCapture stringFromSettings:_captureInfo->settings withSetting:@"device"];
            NSString *presetName = [OBSAVCapture stringFromSettings:_captureInfo->settings withSetting:@"preset"];

            BOOL isPresetEnabled = obs_data_get_bool(_captureInfo->settings, "use_preset");

            if (capture_info->isFastPath) {
                _isFastPath = YES;
                _isPresetBased = NO;
            } else {
                BOOL isBufferingEnabled = obs_data_get_bool(_captureInfo->settings, "buffering");

                obs_source_set_async_unbuffered(_captureInfo->source, !isBufferingEnabled);
            }

            __weak OBSAVCapture *weakSelf = self;

            dispatch_async(_sessionQueue, ^{
                NSError *error = nil;

                OBSAVCapture *instance = weakSelf;

                if ([instance createSession:&error]) {
                    if ([instance switchCaptureDevice:UUID withError:nil]) {
                        BOOL isSessionConfigured = NO;

                        if (isPresetEnabled) {
                            isSessionConfigured = [instance configureSessionWithPreset:presetName withError:nil];
                        } else {
                            isSessionConfigured = [instance configureSession:nil];
                        }

                        if (isSessionConfigured) {
                            [instance startCaptureSession];
                        }
                    }
                } else {
                    [instance AVCaptureLog:LOG_ERROR withFormat:error.localizedDescription];
                }
            });
        }
    }

    return self;
}

#pragma mark - Capture Session Handling

- (BOOL)createSession:(NSError *__autoreleasing *)error
{
    AVCaptureSession *session = [[AVCaptureSession alloc] init];

    [session beginConfiguration];

    if (!session) {
        if (error) {
            NSDictionary *userInfo = @{NSLocalizedDescriptionKey: @"Failed to create AVCaptureSession"};

            *error = [NSError errorWithDomain:self.errorDomain code:-101 userInfo:userInfo];
        }

        return NO;
    }

    AVCaptureVideoDataOutput *videoOutput = [[AVCaptureVideoDataOutput alloc] init];

    if (!videoOutput) {
        if (error) {
            NSDictionary *userInfo = @{NSLocalizedDescriptionKey: @"Failed to create AVCaptureVideoDataOutput"};
            *error = [NSError errorWithDomain:self.errorDomain code:-102 userInfo:userInfo];
        }

        return NO;
    }

    AVCaptureAudioDataOutput *audioOutput = [[AVCaptureAudioDataOutput alloc] init];

    if (!audioOutput) {
        if (error) {
            NSDictionary *userInfo = @{NSLocalizedDescriptionKey: @"Failed to create AVCaptureAudioDataOutput"};
            *error = [NSError errorWithDomain:self.errorDomain code:-103 userInfo:userInfo];
        }

        return NO;
    }

    dispatch_queue_t videoQueue = dispatch_queue_create(nil, nil);

    if (!videoQueue) {
        if (error) {
            NSDictionary *userInfo = @{NSLocalizedDescriptionKey: @"Failed to create video dispatch queue"};

            *error = [NSError errorWithDomain:self.errorDomain code:-104 userInfo:userInfo];
        }

        return NO;
    }

    dispatch_queue_t audioQueue = dispatch_queue_create(nil, nil);

    if (!audioQueue) {
        if (error) {
            NSDictionary *userInfo = @{NSLocalizedDescriptionKey: @"Failed to create audio dispatch queue"};

            *error = [NSError errorWithDomain:self.errorDomain code:-105 userInfo:userInfo];
        }

        return NO;
    }

    if ([session canAddOutput:videoOutput]) {
        [session addOutput:videoOutput];
        [videoOutput setSampleBufferDelegate:self queue:videoQueue];
    }

    if ([session canAddOutput:audioOutput]) {
        [session addOutput:audioOutput];
        [audioOutput setSampleBufferDelegate:self queue:audioQueue];
    }

    [session commitConfiguration];

    self.session = session;
    self.videoOutput = videoOutput;
    self.videoQueue = videoQueue;
    self.audioOutput = audioOutput;
    self.audioQueue = audioQueue;

    return YES;
}

- (BOOL)switchCaptureDevice:(NSString *)uuid withError:(NSError *__autoreleasing *)error
{
    AVCaptureDevice *device = [AVCaptureDevice deviceWithUniqueID:uuid];

    if (self.deviceInput.device || !device) {
        [self stopCaptureSession];
        [self.session removeInput:self.deviceInput];

        [self.deviceInput.device unlockForConfiguration];
        self.deviceInput = nil;
        self.isDeviceLocked = NO;
        self.presetFormat = nil;
    }

    if (!device) {
        if (uuid.length < 1) {
            [self AVCaptureLog:LOG_INFO withFormat:@"No device selected"];
            self.deviceUUID = uuid;
            return NO;
        } else {
            [self AVCaptureLog:LOG_WARNING withFormat:@"Unable to initialize device with unique ID '%@'", uuid];
            return NO;
        }
    }

    const char *deviceName = device.localizedName.UTF8String;
    obs_data_set_string(self.captureInfo->settings, "device_name", deviceName);
    obs_data_set_string(self.captureInfo->settings, "device", device.uniqueID.UTF8String);

    [self AVCaptureLog:LOG_INFO withFormat:@"Selected device '%@'", device.localizedName];

    self.deviceUUID = device.uniqueID;

    BOOL isAudioSupported = [device hasMediaType:AVMediaTypeAudio] || [device hasMediaType:AVMediaTypeMuxed];

    obs_source_set_audio_active(self.captureInfo->source, isAudioSupported);

    AVCaptureDeviceInput *deviceInput = [AVCaptureDeviceInput deviceInputWithDevice:device error:error];

    if (!deviceInput) {
        return NO;
    }

    [self.session beginConfiguration];

    if ([self.session canAddInput:deviceInput]) {
        [self.session addInput:deviceInput];
        self.deviceInput = deviceInput;
    } else {
        if (error) {
            NSDictionary *userInfo = @{
                NSLocalizedDescriptionKey: [NSString
                    stringWithFormat:@"Unable to add device '%@' as deviceInput to capture session", self.deviceUUID]
            };

            *error = [NSError errorWithDomain:self.errorDomain code:-107 userInfo:userInfo];
        }

        [self.session commitConfiguration];
        return NO;
    }

    AVCaptureDeviceFormat *deviceFormat = device.activeFormat;

    CMMediaType mediaType = CMFormatDescriptionGetMediaType(deviceFormat.formatDescription);

    if (mediaType != kCMMediaType_Video && mediaType != kCMMediaType_Muxed) {
        if (error) {
            NSDictionary *userInfo = @{
                NSLocalizedDescriptionKey: [NSString stringWithFormat:@"CMMediaType '%@' is not supported",
                                                                      [OBSAVCapture stringFromFourCharCode:mediaType]]
            };
            *error = [NSError errorWithDomain:self.errorDomain code:-108 userInfo:userInfo];
        }

        [self.session removeInput:deviceInput];
        [self.session commitConfiguration];
        return NO;
    }

    if (self.isFastPath) {
        self.videoOutput.videoSettings = nil;

        NSMutableDictionary *videoSettings =
            [NSMutableDictionary dictionaryWithDictionary:self.videoOutput.videoSettings];

        FourCharCode targetPixelFormatType = kCVPixelFormatType_32BGRA;

        [videoSettings setObject:@(targetPixelFormatType)
                          forKey:(__bridge NSString *) kCVPixelBufferPixelFormatTypeKey];

        self.videoOutput.videoSettings = videoSettings;
    } else {
        self.videoOutput.videoSettings = nil;

        FourCharCode subType = [[self.videoOutput.videoSettings
            objectForKey:(__bridge NSString *) kCVPixelBufferPixelFormatTypeKey] unsignedIntValue];

        if ([OBSAVCapture formatFromSubtype:subType] != VIDEO_FORMAT_NONE) {
            [self AVCaptureLog:LOG_DEBUG
                    withFormat:@"Using native fourcc '%@'", [OBSAVCapture stringFromFourCharCode:subType]];
        } else {
            [self AVCaptureLog:LOG_DEBUG withFormat:@"Using fallback fourcc '%@' ('%@', 0x%08x unsupported)",
                                                    [OBSAVCapture stringFromFourCharCode:kCVPixelFormatType_32BGRA],
                                                    [OBSAVCapture stringFromFourCharCode:subType], subType];

            NSMutableDictionary *videoSettings =
                [NSMutableDictionary dictionaryWithDictionary:self.videoOutput.videoSettings];

            [videoSettings setObject:@(kCVPixelFormatType_32BGRA)
                              forKey:(__bridge NSString *) kCVPixelBufferPixelFormatTypeKey];

            self.videoOutput.videoSettings = videoSettings;
        }
    }

    [self.session commitConfiguration];

    return YES;
}

- (void)startCaptureSession
{
    if (!self.session.running) {
        [self.session startRunning];
    }
}

- (void)stopCaptureSession
{
    if (self.session.running) {
        [self.session stopRunning];
    }

    if (self.captureInfo->isFastPath) {
        if (self.captureInfo->texture) {
            obs_enter_graphics();
            gs_texture_destroy(self.captureInfo->texture);
            obs_leave_graphics();

            self.captureInfo->texture = NULL;
        }

        if (self.captureInfo->currentSurface) {
            IOSurfaceDecrementUseCount(self.captureInfo->currentSurface);
            CFRelease(self.captureInfo->currentSurface);
            self.captureInfo->currentSurface = NULL;
        }

        if (self.captureInfo->previousSurface) {
            IOSurfaceDecrementUseCount(self.captureInfo->previousSurface);
            CFRelease(self.captureInfo->previousSurface);
            self.captureInfo->previousSurface = NULL;
        }
    } else {
        if (self.captureInfo->source) {
            obs_source_output_video(self.captureInfo->source, NULL);
        }
    }
}

- (BOOL)configureSessionWithPreset:(AVCaptureSessionPreset)preset withError:(NSError *__autoreleasing *)error
{
    if (!self.deviceInput.device) {
        if (error) {
            NSDictionary *userInfo =
                @{NSLocalizedDescriptionKey: @"Unable to set session preset without capture device"};

            *error = [NSError errorWithDomain:self.errorDomain code:-108 userInfo:userInfo];
        }
        return NO;
    }

    if (![self.deviceInput.device supportsAVCaptureSessionPreset:preset]) {
        if (error) {
            NSDictionary *userInfo = @{
                NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Preset %@ not supported by device %@",
                                                                      [OBSAVCapture stringFromCapturePreset:preset],
                                                                      self.deviceInput.device.localizedName]
            };

            *error = [NSError errorWithDomain:self.errorDomain code:-201 userInfo:userInfo];
        }

        return NO;
    }

    if ([self.session canSetSessionPreset:preset]) {
        if (self.isDeviceLocked) {
            if ([preset isEqualToString:self.session.sessionPreset]) {
                if (self.deviceInput.device.activeFormat) {
                    self.deviceInput.device.activeFormat = self.presetFormat.activeFormat;
                    self.deviceInput.device.activeVideoMinFrameDuration = self.presetFormat.minFrameRate;
                    self.deviceInput.device.activeVideoMaxFrameDuration = self.presetFormat.maxFrameRate;
                }
                self.presetFormat = nil;
            }

            [self.deviceInput.device unlockForConfiguration];
            self.isDeviceLocked = NO;
        }

        if ([self.session canSetSessionPreset:preset]) {
            self.session.sessionPreset = preset;
        }
    } else {
        if (error) {
            NSDictionary *userInfo = @{
                NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Preset %@ not supported by capture session",
                                                                      [OBSAVCapture stringFromCapturePreset:preset]]
            };

            *error = [NSError errorWithDomain:self.errorDomain code:-202 userInfo:userInfo];
        }

        return NO;
    }

    self.isPresetBased = YES;
    return YES;
}

- (BOOL)configureSession:(NSError *__autoreleasing *)error
{
    struct media_frames_per_second fps;
    if (!obs_data_get_frames_per_second(self.captureInfo->settings, "frame_rate", &fps, NULL)) {
        [self AVCaptureLog:LOG_DEBUG withFormat:@"No valid framerate found in settings"];
        return NO;
    }

    CMTime time = {.value = fps.denominator, .timescale = fps.numerator, .flags = 1};

    const char *selectedFormat = obs_data_get_string(self.captureInfo->settings, "supported_format");
    NSString *selectedFormatNSString = selectedFormat != NULL ? @(selectedFormat) : @"";

    AVCaptureDeviceFormat *format = nil;
    FourCharCode subtype;
    OBSAVCaptureColorSpace colorSpace;
    bool fpsSupported = false;

    if (![selectedFormatNSString isEqualToString:@""]) {
        for (AVCaptureDeviceFormat *formatCandidate in [self.deviceInput.device.formats reverseObjectEnumerator]) {
            if ([selectedFormatNSString isEqualToString:formatCandidate.obsPropertyListInternalRepresentation]) {
                CMFormatDescriptionRef formatDescription = formatCandidate.formatDescription;
                FourCharCode formatFourCC = CMFormatDescriptionGetMediaSubType(formatDescription);
                format = formatCandidate;
                subtype = formatFourCC;
                colorSpace = [OBSAVCapture colorspaceFromDescription:formatDescription];
                break;
            }
        }
    } else {
        //try to migrate from the legacy suite of properties
        int legacyVideoRange = (int) obs_data_get_int(self.captureInfo->settings, "video_range");
        int legacyInputFormat = (int) obs_data_get_int(self.captureInfo->settings, "input_format");
        int legacyColorSpace = (int) obs_data_get_int(self.captureInfo->settings, "color_space");
        CMVideoDimensions legacyDimensions = [OBSAVCapture legacyDimensionsFromSettings:self.captureInfo->settings];
        for (AVCaptureDeviceFormat *formatCandidate in [self.deviceInput.device.formats reverseObjectEnumerator]) {
            CMFormatDescriptionRef formatDescription = formatCandidate.formatDescription;
            CMVideoDimensions formatDimensions = CMVideoFormatDescriptionGetDimensions(formatDescription);
            int formatColorSpace = [OBSAVCapture colorspaceFromDescription:formatDescription];
            int formatInputFormat =
                [OBSAVCapture formatFromSubtype:CMFormatDescriptionGetMediaSubType(formatDescription)];
            int formatVideoRange = [OBSAVCapture isFullRangeFormat:formatInputFormat] ? VIDEO_RANGE_FULL
                                                                                      : VIDEO_RANGE_PARTIAL;
            bool foundFormat = legacyVideoRange == formatVideoRange && legacyInputFormat == formatInputFormat &&
                               legacyColorSpace == formatColorSpace &&
                               legacyDimensions.width == formatDimensions.width &&
                               legacyDimensions.height == formatDimensions.height;
            if (foundFormat) {
                format = formatCandidate;
                subtype = formatInputFormat;
                colorSpace = formatColorSpace;
                break;
            }
        }
    }

    if (!format) {
        [self AVCaptureLog:LOG_WARNING withFormat:@"Configured format not found on device"];
        return NO;
    }

    for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges) {
        if (CMTimeCompare(range.maxFrameDuration, time) >= 0 && CMTimeCompare(range.minFrameDuration, time) <= 0) {
            fpsSupported = true;
        }
    }

    if (!fpsSupported) {
        [self AVCaptureLog:LOG_WARNING withFormat:@"Frame rate is not supported: %g FPS (%u/%u)",
                                                  media_frames_per_second_to_fps(fps), fps.numerator, fps.denominator];
        return NO;
    }

    [self.session beginConfiguration];

    self.isDeviceLocked = [self.deviceInput.device lockForConfiguration:error];

    if (!self.isDeviceLocked) {
        [self AVCaptureLog:LOG_WARNING withFormat:@"Could not lock device for configuration"];
        return NO;
    }

    [self AVCaptureLog:LOG_INFO withFormat:@"Capturing '%@' (%@):\n"
                                            " Using Format          : %@"
                                            " FPS                   : %g (%u/%u)\n"
                                            " Frame Interval        : %g\u00a0s\n",
                                           self.deviceInput.device.localizedName, self.deviceInput.device.uniqueID,
                                           selectedFormatNSString, media_frames_per_second_to_fps(fps), fps.numerator,
                                           fps.denominator, media_frames_per_second_to_frame_interval(fps)];

    OBSAVCaptureVideoInfo newInfo = {.colorSpace = _videoInfo.colorSpace,
                                     .fourCC = _videoInfo.fourCC,
                                     .isValid = false};

    self.videoInfo = newInfo;
    self.captureInfo->configuredColorSpace = colorSpace;
    self.captureInfo->configuredFourCC = subtype;

    self.isPresetBased = NO;

    if (!self.presetFormat) {
        OBSAVCapturePresetInfo *presetInfo = [[OBSAVCapturePresetInfo alloc] init];
        presetInfo.activeFormat = self.deviceInput.device.activeFormat;
        presetInfo.minFrameRate = self.deviceInput.device.activeVideoMinFrameDuration;
        presetInfo.maxFrameRate = self.deviceInput.device.activeVideoMaxFrameDuration;
        self.presetFormat = presetInfo;
    }

    self.deviceInput.device.activeFormat = format;
    self.deviceInput.device.activeVideoMinFrameDuration = time;
    self.deviceInput.device.activeVideoMaxFrameDuration = time;

    [self.session commitConfiguration];

    return YES;
}

- (BOOL)updateSessionwithError:(NSError *__autoreleasing *)error
{
    switch (self.captureInfo->lastError) {
        case OBSAVCaptureError_SampleBufferFormat:
            if (self.captureInfo->sampleBufferDescription) {
                FourCharCode mediaSubType =
                    CMFormatDescriptionGetMediaSubType(self.captureInfo->sampleBufferDescription);

                [self AVCaptureLog:LOG_ERROR
                        withFormat:@"Incompatible sample buffer format received for sync AVCapture source: %@ (0x%x)",
                                   [OBSAVCapture stringFromFourCharCode:mediaSubType], mediaSubType];
            }
            break;
        case OBSAVCaptureError_ColorSpace: {
            if (self.captureInfo->sampleBufferDescription) {
                FourCharCode mediaSubType =
                    CMFormatDescriptionGetMediaSubType(self.captureInfo->sampleBufferDescription);
                BOOL isSampleBufferFullRange = [OBSAVCapture isFullRangeFormat:mediaSubType];
                OBSAVCaptureColorSpace sampleBufferColorSpace =
                    [OBSAVCapture colorspaceFromDescription:self.captureInfo->sampleBufferDescription];
                OBSAVCaptureVideoRange sampleBufferRangeType = isSampleBufferFullRange ? VIDEO_RANGE_FULL
                                                                                       : VIDEO_RANGE_PARTIAL;

                [self AVCaptureLog:LOG_ERROR
                        withFormat:@"Failed to get colorspace parameters for colorspace %u and range %u",
                                   sampleBufferColorSpace, sampleBufferRangeType];
            }
            break;
            default:
                self.captureInfo->lastError = OBSAVCaptureError_NoError;
                self.captureInfo->sampleBufferDescription = NULL;
                break;
        }
    }

    switch (self.captureInfo->lastAudioError) {
        case OBSAVCaptureError_AudioBuffer: {
            [OBSAVCapture AVCaptureLog:LOG_ERROR
                            withFormat:@"Unable to retrieve required AudioBufferList size from sample buffer."];
            break;
        }
        default:
            self.captureInfo->lastAudioError = OBSAVCaptureError_NoError;
            break;
    }

    NSString *newDeviceUUID = [OBSAVCapture stringFromSettings:self.captureInfo->settings withSetting:@"device"];
    NSString *presetName = [OBSAVCapture stringFromSettings:self.captureInfo->settings withSetting:@"preset"];
    BOOL isPresetEnabled = obs_data_get_bool(self.captureInfo->settings, "use_preset");

    BOOL updateSession = YES;

    if (![self.deviceUUID isEqualToString:newDeviceUUID]) {
        if (![self switchCaptureDevice:newDeviceUUID withError:error]) {
            obs_source_update_properties(self.captureInfo->source);
            return NO;
        }
    } else if (self.isPresetBased && isPresetEnabled && [presetName isEqualToString:self.session.sessionPreset]) {
        updateSession = NO;
    }

    if (updateSession) {
        if (isPresetEnabled) {
            [self configureSessionWithPreset:presetName withError:error];
        } else {
            if (![self configureSession:error]) {
                obs_source_update_properties(self.captureInfo->source);
                return NO;
            }
        }

        __weak OBSAVCapture *weakSelf = self;
        dispatch_async(self.sessionQueue, ^{
            [weakSelf startCaptureSession];
        });
    }

    BOOL isAudioAvailable = [self.deviceInput.device hasMediaType:AVMediaTypeAudio] ||
                            [self.deviceInput.device hasMediaType:AVMediaTypeMuxed];

    obs_source_set_audio_active(self.captureInfo->source, isAudioAvailable);

    if (!self.isFastPath) {
        BOOL isBufferingEnabled = obs_data_get_bool(self.captureInfo->settings, "buffering");
        obs_source_set_async_unbuffered(self.captureInfo->source, !isBufferingEnabled);
    }

    return YES;
}

#pragma mark - OBS Settings Helpers

+ (CMVideoDimensions)legacyDimensionsFromSettings:(void *)settings
{
    CMVideoDimensions zero = {0};

    NSString *jsonString = [OBSAVCapture stringFromSettings:settings withSetting:@"resolution"];
    NSDictionary *data = [NSJSONSerialization JSONObjectWithData:[jsonString dataUsingEncoding:NSUTF8StringEncoding]
                                                         options:0
                                                           error:nil];
    if (data.count == 0) {
        return zero;
    }

    NSInteger width = [[data objectForKey:@"width"] intValue];
    NSInteger height = [[data objectForKey:@"height"] intValue];

    if (!width || !height) {
        return zero;
    }

    CMVideoDimensions dimensions = {.width = (int32_t) clamp_Uint(width, 0, UINT32_MAX),
                                    .height = (int32_t) clamp_Uint(height, 0, UINT32_MAX)};
    return dimensions;
}

+ (NSString *)aspectRatioStringFromDimensions:(CMVideoDimensions)dimensions
{
    if (dimensions.width <= 0 || dimensions.height <= 0) {
        return @"";
    }
    double divisor = (double) gcd(dimensions.width, dimensions.height);
    if (divisor <= 50) {
        if (dimensions.width > dimensions.height) {
            double x = (double) dimensions.width / (double) dimensions.height;
            return [NSString stringWithFormat:@"%.2f:1", x];
        } else {
            double y = (double) dimensions.height / (double) dimensions.width;
            return [NSString stringWithFormat:@"1:%.2f", y];
        }
    } else {
        SInt32 x = dimensions.width / (SInt32) divisor;
        SInt32 y = dimensions.height / (SInt32) divisor;
        if (x == 8 && y == 5) {
            x = 16;
            y = 10;
        }
        return [NSString stringWithFormat:@"%i:%i", x, y];
    }
}

+ (NSString *)stringFromSettings:(void *)settings withSetting:(NSString *)setting
{
    return [OBSAVCapture stringFromSettings:settings withSetting:setting withDefault:@""];
}

+ (NSString *)stringFromSettings:(void *)settings withSetting:(NSString *)setting withDefault:(NSString *)defaultValue
{
    NSString *result;

    if (settings) {
        const char *setting_value = obs_data_get_string(settings, setting.UTF8String);

        if (!setting_value) {
            result = [NSString stringWithString:defaultValue];
        } else {
            result = @(setting_value);
        }
    } else {
        result = [NSString stringWithString:defaultValue];
    }

    return result;
}

+ (NSString *)effectsWarningForDevice:(AVCaptureDevice *)device
{
    int effectsCount = 0;
    NSString *effectWarning = nil;
    if (@available(macOS 12.0, *)) {
        if (device.portraitEffectActive) {
            effectWarning = @"Warning.Effect.Portrait";
            effectsCount++;
        }
    }
    if (@available(macOS 12.3, *)) {
        if (device.centerStageActive) {
            effectWarning = @"Warning.Effect.CenterStage";
            effectsCount++;
        }
    }
    if (@available(macOS 13.0, *)) {
        if (device.studioLightActive) {
            effectWarning = @"Warning.Effect.StudioLight";
            effectsCount++;
        }
    }
    if (@available(macOS 14.0, *)) {
        /// Reaction effects do not follow the same paradigm as other effects in terms of checking whether they are active. According to Apple, this is because a device instance property `reactionEffectsActive` would have been ambiguous (conflicting with whether a reaction is currently rendering).
        ///
        /// Instead, Apple exposes the `AVCaptureDevice.reactionEffectGesturesEnabled` class property (an equivalent exists for all other effects, but is hidden/private) to tell us whether the effect is enabled application-wide, as well as the `device.canPerformReactionEffects` instance property to tell us whether the device's active format currently supports the effect.
        ///
        /// The logical conjunction of these two properties tells us whether the effect is 'active'; i.e. whether putting our thumbs inside the video frame will make fireworks appear. The device instance properties for other effects are a convenience 'shorthand' for this private class/instance property combination.
        if (device.canPerformReactionEffects && AVCaptureDevice.reactionEffectGesturesEnabled) {
            effectWarning = @"Warning.Effect.Reactions";
            effectsCount++;
        }
    }
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
    if (@available(macOS 15.0, *)) {
        if (device.backgroundReplacementActive) {
            effectWarning = @"Warning.Effect.BackgroundReplacement";
            effectsCount++;
        }
    }
#endif
    if (effectsCount > 1) {
        effectWarning = @"Warning.Effect.Multiple";
    }
    return effectWarning;
}

#pragma mark - Format Conversion Helpers

+ (NSString *)stringFromSubType:(FourCharCode)subtype
{
    switch (subtype) {
        case kCVPixelFormatType_422YpCbCr8:
            return @"UYVY (2vuy)";
        case kCVPixelFormatType_422YpCbCr8_yuvs:
            return @"YUY2 (yuvs)";
        case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
            return @"NV12 (420v)";
        case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
            return @"NV12 (420f)";
        case kCVPixelFormatType_420YpCbCr10BiPlanarFullRange:
            return @"P010 (xf20)";
        case kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange:
            return @"P010 (x420)";
        case kCVPixelFormatType_32ARGB:
            return @"ARGB - 32ARGB";
        case kCVPixelFormatType_32BGRA:
            return @"BGRA - 32BGRA";

        case kCMVideoCodecType_Animation:
            return @"Apple Animation";
        case kCMVideoCodecType_Cinepak:
            return @"Cinepak";
        case kCMVideoCodecType_JPEG:
            return @"JPEG";
        case kCMVideoCodecType_JPEG_OpenDML:
            return @"MJPEG - JPEG OpenDML";
        case kCMVideoCodecType_SorensonVideo:
            return @"Sorenson Video";
        case kCMVideoCodecType_SorensonVideo3:
            return @"Sorenson Video 3";
        case kCMVideoCodecType_H263:
            return @"H.263";
        case kCMVideoCodecType_H264:
            return @"H.264";
        case kCMVideoCodecType_MPEG4Video:
            return @"MPEG-4";
        case kCMVideoCodecType_MPEG2Video:
            return @"MPEG-2";
        case kCMVideoCodecType_MPEG1Video:
            return @"MPEG-1";

        case kCMVideoCodecType_DVCNTSC:
            return @"DV NTSC";
        case kCMVideoCodecType_DVCPAL:
            return @"DV PAL";
        case kCMVideoCodecType_DVCProPAL:
            return @"Panasonic DVCPro Pal";
        case kCMVideoCodecType_DVCPro50NTSC:
            return @"Panasonic DVCPro-50 NTSC";
        case kCMVideoCodecType_DVCPro50PAL:
            return @"Panasonic DVCPro-50 PAL";
        case kCMVideoCodecType_DVCPROHD720p60:
            return @"Panasonic DVCPro-HD 720p60";
        case kCMVideoCodecType_DVCPROHD720p50:
            return @"Panasonic DVCPro-HD 720p50";
        case kCMVideoCodecType_DVCPROHD1080i60:
            return @"Panasonic DVCPro-HD 1080i60";
        case kCMVideoCodecType_DVCPROHD1080i50:
            return @"Panasonic DVCPro-HD 1080i50";
        case kCMVideoCodecType_DVCPROHD1080p30:
            return @"Panasonic DVCPro-HD 1080p30";
        case kCMVideoCodecType_DVCPROHD1080p25:
            return @"Panasonic DVCPro-HD 1080p25";

        case kCMVideoCodecType_AppleProRes4444:
            return @"Apple ProRes 4444";
        case kCMVideoCodecType_AppleProRes422HQ:
            return @"Apple ProRes 422 HQ";
        case kCMVideoCodecType_AppleProRes422:
            return @"Apple ProRes 422";
        case kCMVideoCodecType_AppleProRes422LT:
            return @"Apple ProRes 422 LT";
        case kCMVideoCodecType_AppleProRes422Proxy:
            return @"Apple ProRes 422 Proxy";

        default:
            return @"Unknown";
    }
}

+ (NSString *)stringFromColorspace:(enum video_colorspace)colorspace
{
    switch (colorspace) {
        case VIDEO_CS_DEFAULT:
            return @"Default";
        case VIDEO_CS_601:
            return @"CS 601";
        case VIDEO_CS_709:
            return @"CS 709";
        case VIDEO_CS_SRGB:
            return @"sRGB";
        case VIDEO_CS_2100_PQ:
            return @"CS 2100 (PQ)";
        case VIDEO_CS_2100_HLG:
            return @"CS 2100 (HLG)";
        default:
            return @"Unknown";
    }
}

+ (NSString *)stringFromVideoRange:(enum video_range_type)videoRange
{
    switch (videoRange) {
        case VIDEO_RANGE_FULL:
            return @"Full";
        case VIDEO_RANGE_PARTIAL:
            return @"Partial";
        case VIDEO_RANGE_DEFAULT:
            return @"Default";
    }
}

+ (NSString *)stringFromCapturePreset:(AVCaptureSessionPreset)preset
{
    NSDictionary *presetDescriptions = @{
        AVCaptureSessionPresetLow: @"Low",
        AVCaptureSessionPresetMedium: @"Medium",
        AVCaptureSessionPresetHigh: @"High",
        AVCaptureSessionPreset320x240: @"320x240",
        AVCaptureSessionPreset352x288: @"352x288",
        AVCaptureSessionPreset640x480: @"640x480",
        AVCaptureSessionPreset960x540: @"960x460",
        AVCaptureSessionPreset1280x720: @"1280x720",
        AVCaptureSessionPreset1920x1080: @"1920x1080",
        AVCaptureSessionPreset3840x2160: @"3840x2160",
    };

    NSString *presetDescription = [presetDescriptions objectForKey:preset];

    if (!presetDescription) {
        return [NSString stringWithFormat:@"Unknown (%@)", preset];
    } else {
        return presetDescription;
    }
}

+ (NSString *)stringFromFourCharCode:(OSType)fourCharCode
{
    char cString[5] = {(fourCharCode >> 24) & 0xFF, (fourCharCode >> 16) & 0xFF, (fourCharCode >> 8) & 0xFF,
                       fourCharCode & 0xFF, 0};

    NSString *codeString = @(cString);

    return codeString;
}

+ (FourCharCode)fourCharCodeFromString:(NSString *)codeString
{
    FourCharCode fourCharCode;
    const char *cString = codeString.UTF8String;

    fourCharCode = (cString[0] << 24) | (cString[1] << 16) | (cString[2] << 8) | cString[3];

    return fourCharCode;
}

+ (BOOL)isValidColorspace:(enum video_colorspace)colorspace
{
    switch (colorspace) {
        case VIDEO_CS_DEFAULT:
        case VIDEO_CS_601:
        case VIDEO_CS_709:
            return YES;
        default:
            return NO;
    }
}

+ (BOOL)isValidVideoRange:(enum video_range_type)videoRange
{
    switch (videoRange) {
        case VIDEO_RANGE_DEFAULT:
        case VIDEO_RANGE_PARTIAL:
        case VIDEO_RANGE_FULL:
            return YES;
        default:
            return NO;
    }
}

+ (BOOL)isFullRangeFormat:(FourCharCode)pixelFormat
{
    switch (pixelFormat) {
        case kCVPixelFormatType_420YpCbCr8PlanarFullRange:
        case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
        case kCVPixelFormatType_420YpCbCr10BiPlanarFullRange:
        case kCVPixelFormatType_422YpCbCr8FullRange:
            return YES;
        default:
            return NO;
    }
}

+ (OBSAVCaptureVideoFormat)formatFromSubtype:(FourCharCode)subtype
{
    switch (subtype) {
        case kCVPixelFormatType_422YpCbCr8:
            return VIDEO_FORMAT_UYVY;
        case kCVPixelFormatType_422YpCbCr8_yuvs:
            return VIDEO_FORMAT_YUY2;
        case kCVPixelFormatType_32BGRA:
            return VIDEO_FORMAT_BGRA;
        case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
        case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
            return VIDEO_FORMAT_NV12;
        case kCVPixelFormatType_420YpCbCr10BiPlanarFullRange:
        case kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange:
            return VIDEO_FORMAT_P010;
        default:
            return VIDEO_FORMAT_NONE;
    }
}

+ (FourCharCode)fourCharCodeFromFormat:(OBSAVCaptureVideoFormat)format withRange:(enum video_range_type)videoRange
{
    switch (format) {
        case VIDEO_FORMAT_UYVY:
            return kCVPixelFormatType_422YpCbCr8;
        case VIDEO_FORMAT_YUY2:
            return kCVPixelFormatType_422YpCbCr8_yuvs;
        case VIDEO_FORMAT_NV12:
            if (videoRange == VIDEO_RANGE_FULL) {
                return kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
            } else {
                return kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
            }
        case VIDEO_FORMAT_P010:
            if (videoRange == VIDEO_RANGE_FULL) {
                return kCVPixelFormatType_420YpCbCr10BiPlanarFullRange;
            } else {
                return kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange;
            }
        case VIDEO_FORMAT_BGRA:
            return kCVPixelFormatType_32BGRA;
        default:
            return 0;
    }
}

+ (FourCharCode)fourCharCodeFromFormat:(OBSAVCaptureVideoFormat)format
{
    return [OBSAVCapture fourCharCodeFromFormat:format withRange:VIDEO_RANGE_PARTIAL];
}

+ (NSString *)frameRateDescription:(NSArray<AVFrameRateRange *> *)ranges
{
    NSString *frameRateDescription = @"";
    uint32_t index = 0;
    for (AVFrameRateRange *range in ranges) {
        double minFrameRate = round(range.minFrameRate * 100) / 100;
        double maxFrameRate = round(range.maxFrameRate * 100) / 100;
        if (minFrameRate == maxFrameRate) {
            if (fmod(minFrameRate, 1.0) == 0 && fmod(maxFrameRate, 1.0) == 0) {
                frameRateDescription = [frameRateDescription stringByAppendingFormat:@"%.0f", maxFrameRate];
            } else {
                frameRateDescription = [frameRateDescription stringByAppendingFormat:@"%.2f", maxFrameRate];
            }
        } else {
            if (fmod(minFrameRate, 1.0) == 0 && fmod(maxFrameRate, 1.0) == 0) {
                frameRateDescription =
                    [frameRateDescription stringByAppendingFormat:@"%.0f-%.0f", minFrameRate, maxFrameRate];
            } else {
                frameRateDescription =
                    [frameRateDescription stringByAppendingFormat:@"%.2f-%.2f", minFrameRate, maxFrameRate];
            }
        }
        index++;
        if (index < (ranges.count)) {
            frameRateDescription = [frameRateDescription stringByAppendingString:@", "];
        }
    }
    if (ranges.count > 0) {
        frameRateDescription = [frameRateDescription stringByAppendingString:@" FPS"];
    }
    return frameRateDescription;
}

+ (OBSAVCaptureColorSpace)colorspaceFromDescription:(CMFormatDescriptionRef)description
{
    CFPropertyListRef matrix = CMFormatDescriptionGetExtension(description, kCMFormatDescriptionExtension_YCbCrMatrix);

    if (!matrix) {
        return VIDEO_CS_DEFAULT;
    }

    CFComparisonResult is601 = CFStringCompare(matrix, kCVImageBufferYCbCrMatrix_ITU_R_601_4, 0);
    CFComparisonResult is709 = CFStringCompare(matrix, kCVImageBufferYCbCrMatrix_ITU_R_709_2, 0);
    CFComparisonResult is2020 = CFStringCompare(matrix, kCVImageBufferYCbCrMatrix_ITU_R_2020, 0);

    if (is601 == kCFCompareEqualTo) {
        return VIDEO_CS_601;
    } else if (is709 == kCFCompareEqualTo) {
        return VIDEO_CS_709;
    } else if (is2020 == kCFCompareEqualTo) {
        CFPropertyListRef transferFunction =
            CMFormatDescriptionGetExtension(description, kCMFormatDescriptionExtension_TransferFunction);

        if (!matrix) {
            return VIDEO_CS_DEFAULT;
        }

        CFComparisonResult isPQ = CFStringCompare(transferFunction, kCVImageBufferTransferFunction_SMPTE_ST_2084_PQ, 0);
        CFComparisonResult isHLG = CFStringCompare(transferFunction, kCVImageBufferTransferFunction_ITU_R_2100_HLG, 0);

        if (isPQ == kCFCompareEqualTo) {
            return VIDEO_CS_2100_PQ;
        } else if (isHLG == kCFCompareEqualTo) {
            return VIDEO_CS_2100_HLG;
        }
    }

    return VIDEO_CS_DEFAULT;
}

#pragma mark - Notification Handlers

- (void)deviceConnected:(NSNotification *)notification
{
    AVCaptureDevice *device = notification.object;

    if (!device) {
        return;
    }

    if (![[device uniqueID] isEqualTo:self.deviceUUID]) {
        obs_source_update_properties(self.captureInfo->source);
        return;
    }

    if (self.deviceInput.device) {
        [self AVCaptureLog:LOG_INFO withFormat:@"Received connect event with active device '%@' (UUID %@)",
                                               self.deviceInput.device.localizedName, self.deviceInput.device.uniqueID];

        obs_source_update_properties(self.captureInfo->source);
        return;
    }

    [self AVCaptureLog:LOG_INFO
            withFormat:@"Received connect event for device '%@' (UUID %@)", device.localizedName, device.uniqueID];

    NSError *error;
    NSString *presetName = [OBSAVCapture stringFromSettings:self.captureInfo->settings withSetting:@"preset"];
    BOOL isPresetEnabled = obs_data_get_bool(self.captureInfo->settings, "use_preset");
    BOOL isFastPath = self.captureInfo->isFastPath;

    if ([self switchCaptureDevice:device.uniqueID withError:&error]) {
        BOOL success;
        if (isPresetEnabled && !isFastPath) {
            success = [self configureSessionWithPreset:presetName withError:&error];
        } else {
            success = [self configureSession:&error];
        }

        if (success) {
            dispatch_async(self.sessionQueue, ^{
                [self startCaptureSession];
            });
        } else {
            [self AVCaptureLog:LOG_ERROR withFormat:error.localizedDescription];
        }
    } else {
        [self AVCaptureLog:LOG_ERROR withFormat:error.localizedDescription];
    }

    obs_source_update_properties(self.captureInfo->source);
}

- (void)deviceDisconnected:(NSNotification *)notification
{
    AVCaptureDevice *device = notification.object;

    if (!device) {
        return;
    }

    if (![[device uniqueID] isEqualTo:self.deviceUUID]) {
        obs_source_update_properties(self.captureInfo->source);
        return;
    }

    if (!self.deviceInput.device) {
        [self AVCaptureLog:LOG_ERROR withFormat:@"Received disconnect event for inactive device '%@' (UUID %@)",
                                                device.localizedName, device.uniqueID];
        obs_source_update_properties(self.captureInfo->source);
        return;
    }

    [self AVCaptureLog:LOG_INFO
            withFormat:@"Received disconnect event for device '%@' (UUID %@)", device.localizedName, device.uniqueID];

    __weak OBSAVCapture *weakSelf = self;
    dispatch_async(self.sessionQueue, ^{
        OBSAVCapture *instance = weakSelf;

        [instance stopCaptureSession];
        [instance.session removeInput:instance.deviceInput];

        instance.deviceInput = nil;
        instance = nil;
    });

    obs_source_update_properties(self.captureInfo->source);
}

#pragma mark - AVCapture Delegate Methods

- (void)captureOutput:(AVCaptureOutput *)output
    didDropSampleBuffer:(CMSampleBufferRef)sampleBuffer
         fromConnection:(AVCaptureConnection *)connection
{
    return;
}

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection
{
    CMItemCount sampleCount = CMSampleBufferGetNumSamples(sampleBuffer);

    if (!_captureInfo || sampleCount < 1) {
        return;
    }

    CMTime presentationTimeStamp = CMSampleBufferGetOutputPresentationTimeStamp(sampleBuffer);
    CMTime presentationNanoTimeStamp = CMTimeConvertScale(presentationTimeStamp, 1E9, kCMTimeRoundingMethod_Default);

    CMFormatDescriptionRef description = CMSampleBufferGetFormatDescription(sampleBuffer);
    CMMediaType mediaType = CMFormatDescriptionGetMediaType(description);

    switch (mediaType) {
        case kCMMediaType_Video: {
            CMVideoDimensions sampleBufferDimensions = CMVideoFormatDescriptionGetDimensions(description);
            CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
            FourCharCode mediaSubType = CMFormatDescriptionGetMediaSubType(description);

            OBSAVCaptureVideoInfo newInfo = {.fourCC = _videoInfo.fourCC,
                                             .colorSpace = _videoInfo.colorSpace,
                                             .isValid = false};

            BOOL usePreset = obs_data_get_bool(_captureInfo->settings, "use_preset");

            if (_isFastPath) {
                if (mediaSubType != kCVPixelFormatType_32BGRA &&
                    mediaSubType != kCVPixelFormatType_ARGB2101010LEPacked) {
                    _captureInfo->lastError = OBSAVCaptureError_SampleBufferFormat;
                    CMFormatDescriptionCreate(kCFAllocatorDefault, mediaType, mediaSubType, NULL,
                                              &_captureInfo->sampleBufferDescription);
                    obs_source_update_properties(_captureInfo->source);
                    break;
                } else {
                    _captureInfo->lastError = OBSAVCaptureError_NoError;
                    _captureInfo->sampleBufferDescription = NULL;
                }

                CVPixelBufferLockBaseAddress(imageBuffer, 0);
                IOSurfaceRef frameSurface = CVPixelBufferGetIOSurface(imageBuffer);
                CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

                IOSurfaceRef previousSurface = NULL;

                if (frameSurface && !pthread_mutex_lock(&_captureInfo->mutex)) {
                    NSRect frameSize = _captureInfo->frameSize;

                    if (frameSize.size.width != sampleBufferDimensions.width ||
                        frameSize.size.height != sampleBufferDimensions.height) {
                        frameSize = CGRectMake(0, 0, sampleBufferDimensions.width, sampleBufferDimensions.height);
                    }

                    previousSurface = _captureInfo->currentSurface;
                    _captureInfo->currentSurface = frameSurface;

                    CFRetain(_captureInfo->currentSurface);
                    IOSurfaceIncrementUseCount(_captureInfo->currentSurface);
                    pthread_mutex_unlock(&_captureInfo->mutex);

                    newInfo.isValid = true;

                    if (_videoInfo.isValid != newInfo.isValid) {
                        obs_source_update_properties(_captureInfo->source);
                    }

                    _captureInfo->frameSize = frameSize;
                    _videoInfo = newInfo;
                }

                if (previousSurface) {
                    IOSurfaceDecrementUseCount(previousSurface);
                    CFRelease(previousSurface);
                }

                break;
            } else {
                OBSAVCaptureVideoFrame *frame = _captureInfo->videoFrame;

                frame->timestamp = presentationNanoTimeStamp.value;

                enum video_format videoFormat = [OBSAVCapture formatFromSubtype:mediaSubType];

                if (videoFormat == VIDEO_FORMAT_NONE) {
                    _captureInfo->lastError = OBSAVCaptureError_SampleBufferFormat;
                    CMFormatDescriptionCreate(kCFAllocatorDefault, mediaType, mediaSubType, NULL,
                                              &_captureInfo->sampleBufferDescription);
                } else {
                    _captureInfo->lastError = OBSAVCaptureError_NoError;
                    _captureInfo->sampleBufferDescription = NULL;
#ifdef DEBUG
                    if (frame->format != VIDEO_FORMAT_NONE && frame->format != videoFormat) {
                        [self AVCaptureLog:LOG_DEBUG
                                withFormat:@"Switching fourcc: '%@' (0x%x) -> '%@' (0x%x)",
                                           [OBSAVCapture stringFromFourCharCode:frame->format], frame -> format,
                                           [OBSAVCapture stringFromFourCharCode:mediaSubType], mediaSubType];
                    }
#endif
                    bool isFrameYuv = format_is_yuv(frame->format);
                    bool isSampleBufferYuv = format_is_yuv(videoFormat);

                    frame->format = videoFormat;
                    frame->width = sampleBufferDimensions.width;
                    frame->height = sampleBufferDimensions.height;

                    BOOL isSampleBufferFullRange = [OBSAVCapture isFullRangeFormat:mediaSubType];

                    if (isSampleBufferYuv) {
                        OBSAVCaptureColorSpace sampleBufferColorSpace =
                            [OBSAVCapture colorspaceFromDescription:description];
                        OBSAVCaptureVideoRange sampleBufferRangeType = isSampleBufferFullRange ? VIDEO_RANGE_FULL
                                                                                               : VIDEO_RANGE_PARTIAL;
                        BOOL isColorSpaceMatching = NO;

                        SInt64 configuredColorSpace = _captureInfo->configuredColorSpace;

                        if (usePreset) {
                            isColorSpaceMatching = sampleBufferColorSpace == _videoInfo.colorSpace;
                        } else {
                            isColorSpaceMatching = configuredColorSpace == _videoInfo.colorSpace;
                        }

                        BOOL isFourCCMatching = NO;
                        SInt64 configuredFourCC = _captureInfo->configuredFourCC;

                        if (usePreset) {
                            isFourCCMatching = mediaSubType == _videoInfo.fourCC;
                        } else {
                            isFourCCMatching = configuredFourCC == _videoInfo.fourCC;
                        }

                        if (isColorSpaceMatching && isFourCCMatching) {
                            newInfo.isValid = true;
                        } else {
                            frame->full_range = isSampleBufferFullRange;

                            bool success = video_format_get_parameters_for_format(
                                sampleBufferColorSpace, sampleBufferRangeType, frame->format, frame->color_matrix,
                                frame->color_range_min, frame->color_range_max);

                            if (!success) {
                                _captureInfo->lastError = OBSAVCaptureError_ColorSpace;
                                CMFormatDescriptionCreate(kCFAllocatorDefault, mediaType, mediaSubType, NULL,
                                                          &_captureInfo->sampleBufferDescription);
                                newInfo.isValid = false;
                            } else {
                                newInfo.colorSpace = sampleBufferColorSpace;
                                newInfo.fourCC = mediaSubType;
                                newInfo.isValid = true;
                            }
                        }
                    } else if (!isFrameYuv && !isSampleBufferYuv) {
                        newInfo.isValid = true;
                    }
                }

                if (newInfo.isValid != _videoInfo.isValid) {
                    obs_source_update_properties(_captureInfo->source);
                }

                _videoInfo = newInfo;

                if (newInfo.isValid) {
                    CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

                    if (!CVPixelBufferIsPlanar(imageBuffer)) {
                        frame->linesize[0] = (UInt32) CVPixelBufferGetBytesPerRow(imageBuffer);
                        frame->data[0] = CVPixelBufferGetBaseAddress(imageBuffer);
                    } else {
                        size_t planeCount = CVPixelBufferGetPlaneCount(imageBuffer);

                        for (size_t i = 0; i < planeCount; i++) {
                            frame->linesize[i] = (UInt32) CVPixelBufferGetBytesPerRowOfPlane(imageBuffer, i);
                            frame->data[i] = CVPixelBufferGetBaseAddressOfPlane(imageBuffer, i);
                        }
                    }

                    obs_source_output_video(_captureInfo->source, frame);
                    CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
                } else {
                    obs_source_output_video(_captureInfo->source, NULL);
                }

                break;
            }
        }
        case kCMMediaType_Audio: {
            size_t requiredBufferListSize;
            OSStatus status = noErr;

            status = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
                sampleBuffer, &requiredBufferListSize, NULL, 0, NULL, NULL,
                kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment, NULL);

            if (status != noErr) {
                _captureInfo->lastAudioError = OBSAVCaptureError_AudioBuffer;
                obs_source_update_properties(_captureInfo->source);
                break;
            }

            AudioBufferList *bufferList = (AudioBufferList *) malloc(requiredBufferListSize);
            CMBlockBufferRef blockBuffer = NULL;

            OSStatus error = noErr;
            error = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
                sampleBuffer, NULL, bufferList, requiredBufferListSize, kCFAllocatorSystemDefault,
                kCFAllocatorSystemDefault, kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment, &blockBuffer);

            if (error == noErr) {
                _captureInfo->lastAudioError = OBSAVCaptureError_NoError;

                OBSAVCaptureAudioFrame *audio = _captureInfo->audioFrame;

                for (size_t i = 0; i < bufferList->mNumberBuffers; i++) {
                    audio->data[i] = bufferList->mBuffers[i].mData;
                }

                audio->timestamp = presentationNanoTimeStamp.value;
                audio->frames = (uint32_t) CMSampleBufferGetNumSamples(sampleBuffer);

                const AudioStreamBasicDescription *basicDescription =
                    CMAudioFormatDescriptionGetStreamBasicDescription(description);

                audio->samples_per_sec = (uint32_t) basicDescription->mSampleRate;
                audio->speakers = (enum speaker_layout) basicDescription->mChannelsPerFrame;

                switch (basicDescription->mBitsPerChannel) {
                    case 8:
                        audio->format = AUDIO_FORMAT_U8BIT;
                        break;
                    case 16:
                        audio->format = AUDIO_FORMAT_16BIT;
                        break;
                    case 32:
                        audio->format = AUDIO_FORMAT_32BIT;
                        break;
                    default:
                        audio->format = AUDIO_FORMAT_UNKNOWN;
                        break;
                }

                obs_source_output_audio(_captureInfo->source, audio);
            } else {
                _captureInfo->lastAudioError = OBSAVCaptureError_AudioBuffer;
                obs_source_output_audio(_captureInfo->source, NULL);
            }

            if (blockBuffer != NULL) {
                CFRelease(blockBuffer);
            }

            if (bufferList != NULL) {
                free(bufferList);
                bufferList = NULL;
            }

            break;
        }
        default:
            break;
    }
}

#pragma mark - Log Helpers

- (void)AVCaptureLog:(int)logLevel withFormat:(NSString *)format, ...
{
    va_list args;
    va_start(args, format);

    NSString *logMessage = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);

    const char *name_value = obs_source_get_name(self.captureInfo->source);
    NSString *sourceName = @((name_value) ? name_value : "");

    blog(logLevel, "%s: %s", sourceName.UTF8String, logMessage.UTF8String);
}

+ (void)AVCaptureLog:(int)logLevel withFormat:(NSString *)format, ...
{
    va_list args;
    va_start(args, format);

    NSString *logMessage = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);

    blog(logLevel, "%s", logMessage.UTF8String);
}

@end
