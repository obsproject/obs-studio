//
//  plugin-properties.m
//  mac-avcapture
//
//  Created by Patrick Heyer on 2023-03-07.
//

#import "OBSAVCapture.h"
#import "plugin-properties.h"
#import "AVCaptureDeviceFormat+OBSListable.h"

extern const char *av_capture_get_text(const char *text_id);

void configure_property(obs_property_t *property, bool enable, bool visible, void *callback, OBSAVCapture *capture)
{
    if (property) {
        obs_property_set_enabled(property, enable);
        obs_property_set_visible(property, visible);

        if (callback) {
            obs_property_set_modified_callback2(property, callback, (__bridge void *) (capture));
        }
    }
}

bool properties_changed(OBSAVCapture *capture, obs_properties_t *properties, obs_property_t *property __unused,
                        obs_data_t *settings)
{
    OBSAVCaptureInfo *captureInfo = capture.captureInfo;

    obs_property_t *prop_use_preset = obs_properties_get(properties, "use_preset");
    obs_property_t *prop_device = obs_properties_get(properties, "device");
    obs_property_t *prop_presets = obs_properties_get(properties, "preset");

    obs_property_set_enabled(prop_use_preset, !captureInfo->isFastPath);

    if (captureInfo && settings) {
        properties_update_device(capture, prop_device, settings);

        bool use_preset = (settings ? obs_data_get_bool(settings, "use_preset") : true);

        if (use_preset) {
            properties_update_preset(capture, prop_presets, settings);
        } else {
            properties_update_config(capture, properties, settings);
        }
    }

    return true;
}

bool properties_changed_preset(OBSAVCapture *capture, obs_properties_t *properties __unused, obs_property_t *property,
                               obs_data_t *settings)
{
    bool use_preset = obs_data_get_bool(settings, "use_preset");

    if (capture && settings && use_preset) {
        NSArray *presetKeys =
            [capture.presetList keysSortedByValueUsingComparator:^NSComparisonResult(NSString *obj1, NSString *obj2) {
                NSNumber *obj1Resolution;
                NSNumber *obj2Resolution;
                if ([obj1 isEqualToString:@"High"]) {
                    obj1Resolution = @3;
                } else if ([obj1 isEqualToString:@"Medium"]) {
                    obj1Resolution = @2;
                } else if ([obj1 isEqualToString:@"Low"]) {
                    obj1Resolution = @1;
                } else {
                    NSArray<NSString *> *obj1Dimensions = [obj1 componentsSeparatedByString:@"x"];
                    obj1Resolution = [NSNumber numberWithInt:([[obj1Dimensions objectAtIndex:0] intValue] *
                                                              [[obj1Dimensions objectAtIndex:1] intValue])];
                }

                if ([obj2 isEqualToString:@"High"]) {
                    obj2Resolution = @3;
                } else if ([obj2 isEqualToString:@"Medium"]) {
                    obj2Resolution = @2;
                } else if ([obj2 isEqualToString:@"Low"]) {
                    obj2Resolution = @1;
                } else {
                    NSArray<NSString *> *obj2Dimensions = [obj2 componentsSeparatedByString:@"x"];
                    obj2Resolution = [NSNumber numberWithInt:([[obj2Dimensions objectAtIndex:0] intValue] *
                                                              [[obj2Dimensions objectAtIndex:1] intValue])];
                }

                NSComparisonResult result = [obj1Resolution compare:obj2Resolution];

                if (result == NSOrderedAscending) {
                    return (NSComparisonResult) NSOrderedDescending;
                } else if (result == NSOrderedDescending) {
                    return (NSComparisonResult) NSOrderedAscending;
                } else {
                    return (NSComparisonResult) NSOrderedSame;
                }
            }];

        NSString *UUID = [OBSAVCapture stringFromSettings:settings withSetting:@"device"];
        AVCaptureDevice *device = [AVCaptureDevice deviceWithUniqueID:UUID];
        NSString *currentPreset = [OBSAVCapture stringFromSettings:settings withSetting:@"preset"];

        obs_property_list_clear(property);

        if (device) {
            for (NSString *presetName in presetKeys) {
                NSString *presetDescription = capture.presetList[presetName];

                if ([device supportsAVCaptureSessionPreset:presetName]) {
                    obs_property_list_add_string(property, presetDescription.UTF8String, presetName.UTF8String);
                } else if ([currentPreset isEqualToString:presetName]) {
                    size_t index =
                        obs_property_list_add_string(property, presetDescription.UTF8String, presetName.UTF8String);
                    obs_property_list_item_disable(property, index, true);
                }
            };
        } else if (UUID.length) {
            size_t index = obs_property_list_add_string(property, capture.presetList[currentPreset].UTF8String,
                                                        currentPreset.UTF8String);
            obs_property_list_item_disable(property, index, true);
        }

        return YES;
    } else {
        return NO;
    }
}

bool properties_changed_use_preset(OBSAVCapture *capture, obs_properties_t *properties,
                                   obs_property_t *property __unused, obs_data_t *settings)
{
    bool use_preset = obs_data_get_bool(settings, "use_preset");
    obs_property_t *preset_list = obs_properties_get(properties, "preset");

    obs_property_set_visible(preset_list, use_preset);

    if (use_preset) {
        properties_changed_preset(capture, properties, preset_list, settings);
    }

    const char *update_properties[2] = {"frame_rate", "supported_format"};

    size_t number_of_properties = sizeof(update_properties) / sizeof(update_properties[0]);

    for (size_t i = 0; i < number_of_properties; i++) {
        obs_property_t *update_property = obs_properties_get(properties, update_properties[i]);

        if (update_property) {
            obs_property_set_visible(update_property, !use_preset);
            obs_property_set_enabled(update_property, !use_preset);
        }
    }

    return true;
}

bool properties_update_preset(OBSAVCapture *capture, obs_property_t *property, obs_data_t *settings)
{
    NSArray *presetKeys =
        [capture.presetList keysSortedByValueUsingComparator:^NSComparisonResult(NSString *obj1, NSString *obj2) {
            NSNumber *obj1Resolution;
            NSNumber *obj2Resolution;
            if ([obj1 isEqualToString:@"High"]) {
                obj1Resolution = @3;
            } else if ([obj1 isEqualToString:@"Medium"]) {
                obj1Resolution = @2;
            } else if ([obj1 isEqualToString:@"Low"]) {
                obj1Resolution = @1;
            } else {
                NSArray<NSString *> *obj1Dimensions = [obj1 componentsSeparatedByString:@"x"];
                obj1Resolution = [NSNumber numberWithInt:([[obj1Dimensions objectAtIndex:0] intValue] *
                                                          [[obj1Dimensions objectAtIndex:1] intValue])];
            }

            if ([obj2 isEqualToString:@"High"]) {
                obj2Resolution = @3;
            } else if ([obj2 isEqualToString:@"Medium"]) {
                obj2Resolution = @2;
            } else if ([obj2 isEqualToString:@"Low"]) {
                obj2Resolution = @1;
            } else {
                NSArray<NSString *> *obj2Dimensions = [obj2 componentsSeparatedByString:@"x"];
                obj2Resolution = [NSNumber numberWithInt:([[obj2Dimensions objectAtIndex:0] intValue] *
                                                          [[obj2Dimensions objectAtIndex:1] intValue])];
            }

            NSComparisonResult result = [obj1Resolution compare:obj2Resolution];

            if (result == NSOrderedAscending) {
                return (NSComparisonResult) NSOrderedDescending;
            } else if (result == NSOrderedDescending) {
                return (NSComparisonResult) NSOrderedAscending;
            } else {
                return (NSComparisonResult) NSOrderedSame;
            }
        }];

    NSString *deviceUUID = [OBSAVCapture stringFromSettings:settings withSetting:@"device"];
    AVCaptureDevice *device = [AVCaptureDevice deviceWithUniqueID:deviceUUID];
    NSString *currentPreset = [OBSAVCapture stringFromSettings:settings withSetting:@"preset"];

    obs_property_list_clear(property);

    if (device) {
        for (NSString *presetName in presetKeys) {
            NSString *presetDescription = capture.presetList[presetName];

            if ([device supportsAVCaptureSessionPreset:presetName]) {
                obs_property_list_add_string(property, presetDescription.UTF8String, presetName.UTF8String);
            } else if ([currentPreset isEqualToString:presetName]) {
                size_t index =
                    obs_property_list_add_string(property, presetDescription.UTF8String, presetName.UTF8String);
                obs_property_list_item_disable(property, index, true);
            }
        };
    } else if (deviceUUID.length) {
        size_t index = obs_property_list_add_string(property, capture.presetList[currentPreset].UTF8String,
                                                    currentPreset.UTF8String);
        obs_property_list_item_disable(property, index, true);
    }

    return true;
}

bool properties_update_device(OBSAVCapture *capture __unused, obs_property_t *property, obs_data_t *settings)
{
    obs_property_list_clear(property);

    NSString *currentDeviceUUID = [OBSAVCapture stringFromSettings:settings withSetting:@"device"];
    NSString *currentDeviceName = [OBSAVCapture stringFromSettings:settings withSetting:@"device_name"];
    BOOL isDeviceFound = NO;

    obs_property_list_add_string(property, "", "");

    NSArray *deviceTypes;
    if (@available(macOS 13, *)) {
        deviceTypes = @[
            AVCaptureDeviceTypeBuiltInWideAngleCamera, AVCaptureDeviceTypeExternalUnknown,
            AVCaptureDeviceTypeDeskViewCamera
        ];
    } else {
        deviceTypes = @[AVCaptureDeviceTypeBuiltInWideAngleCamera, AVCaptureDeviceTypeExternalUnknown];
    }

    AVCaptureDeviceDiscoverySession *videoDiscoverySession =
        [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:deviceTypes mediaType:AVMediaTypeVideo
                                                                position:AVCaptureDevicePositionUnspecified];
    AVCaptureDeviceDiscoverySession *muxedDiscoverySession =
        [AVCaptureDeviceDiscoverySession discoverySessionWithDeviceTypes:deviceTypes mediaType:AVMediaTypeMuxed
                                                                position:AVCaptureDevicePositionUnspecified];

    for (AVCaptureDevice *device in [videoDiscoverySession devices]) {
        obs_property_list_add_string(property, device.localizedName.UTF8String, device.uniqueID.UTF8String);

        if (!isDeviceFound && [currentDeviceUUID isEqualToString:device.uniqueID]) {
            isDeviceFound = YES;
        }
    }

    for (AVCaptureDevice *device in [muxedDiscoverySession devices]) {
        obs_property_list_add_string(property, device.localizedName.UTF8String, device.uniqueID.UTF8String);

        if (!isDeviceFound && [currentDeviceUUID isEqualToString:device.uniqueID]) {
            isDeviceFound = YES;
        }
    }

    if (!isDeviceFound && currentDeviceUUID.length > 0) {
        size_t index =
            obs_property_list_add_string(property, currentDeviceName.UTF8String, currentDeviceUUID.UTF8String);
        obs_property_list_item_disable(property, index, true);
    }

    return true;
}

bool properties_update_config(OBSAVCapture *capture, obs_properties_t *properties, obs_data_t *settings)
{
    AVCaptureDevice *device = [AVCaptureDevice deviceWithUniqueID:[OBSAVCapture stringFromSettings:settings
                                                                                       withSetting:@"device"]];

    obs_property_t *prop_framerate = obs_properties_get(properties, "frame_rate");
    obs_property_t *prop_format = obs_properties_get(properties, "supported_format");
    obs_property_t *prop_effects_warning = obs_properties_get(properties, "effects_warning");

    obs_property_frame_rate_clear(prop_framerate);
    obs_property_list_clear(prop_format);
    obs_property_list_clear(prop_effects_warning);

    struct media_frames_per_second fps;
    if (!obs_data_get_frames_per_second(settings, "frame_rate", &fps, NULL)) {
        [capture AVCaptureLog:LOG_DEBUG withFormat:@"No valid framerate found in settings"];
    }

    const char *selectedFormatData = obs_data_get_string(settings, "supported_format");
    NSString *selectedFormatString = [NSString stringWithCString:selectedFormatData encoding:NSUTF8StringEncoding];

    NSMutableArray *frameRates = [[NSMutableArray alloc] init];

    if (device) {
        NSSortDescriptor *aspectSort = [[NSSortDescriptor alloc] initWithKey:@"aspectRatioComparisonValue"
                                                                   ascending:false];
        NSSortDescriptor *pixelBandwidthSort = [[NSSortDescriptor alloc] initWithKey:@"pixelBandwidthComparisonValue"
                                                                           ascending:false];
        NSSortDescriptor *bppSort = [[NSSortDescriptor alloc] initWithKey:@"bitsPerPixel" ascending:true];
        NSArray<NSSortDescriptor *> *sortArray =
            [NSArray arrayWithObjects:aspectSort, pixelBandwidthSort, bppSort, nil];
        for (AVCaptureDeviceFormat *format in [device.formats sortedArrayUsingDescriptors:sortArray]) {
            NSString *enumeratedFormatString = format.obsPropertyListDescription;
            NSString *internalRepresentation = format.obsPropertyListInternalRepresentation;
            obs_property_list_add_string(prop_format, enumeratedFormatString.UTF8String,
                                         internalRepresentation.UTF8String);
            if ([selectedFormatString isEqualToString:internalRepresentation]) {
                for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges) {
                    struct media_frames_per_second min_fps = {
                        .numerator = (uint32_t) clamp_Uint(range.maxFrameDuration.timescale, 0, UINT32_MAX),
                        .denominator = (uint32_t) clamp_Uint(range.maxFrameDuration.value, 0, UINT32_MAX)};
                    struct media_frames_per_second max_fps = {
                        .numerator = (uint32_t) clamp_Uint(range.minFrameDuration.timescale, 0, UINT32_MAX),
                        .denominator = (uint32_t) clamp_Uint(range.minFrameDuration.value, 0, UINT32_MAX)};

                    if (![frameRates containsObject:range]) {
                        obs_property_frame_rate_fps_range_add(prop_framerate, min_fps, max_fps);
                        [frameRates addObject:range];
                    }
                }
            }
        }
    }

    return true;
}
