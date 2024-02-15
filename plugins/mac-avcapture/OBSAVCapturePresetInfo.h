//
//  OBSAVCapturePresetInfo.h
//  mac-avcapture
//
//  Created by Patrick Heyer on 2023-03-07.
//

@import Foundation;
@import AVFoundation;

/// Stores format and framerate of a [AVCaptureSessionPreset](https://developer.apple.com/documentation/avfoundation/avcapturesessionpreset?language=objc).
///
/// Changing the [activeFormat](https://developer.apple.com/documentation/avfoundation/avcapturedevice/1389221-activeformat?language=objc) of a device takes precedence over the configuration contained in a [AVCaptureSessionPreset](https://developer.apple.com/documentation/avfoundation/avcapturesessionpreset?language=objc). To restore a preset's configuration after changing to a different format, the values of a configured preset are stored in this object and restored when the source is switched back to a preset-based configuration.
@interface OBSAVCapturePresetInfo : NSObject

/// [activeFormat](https://developer.apple.com/documentation/avfoundation/avcapturedevice/1389221-activeformat?language=objc) used by the preset
@property (nonatomic) AVCaptureDeviceFormat *activeFormat;

/// Minimum framerate supported by the preset
@property (nonatomic) CMTime minFrameRate;

/// Maximum framerate supported by the preset
@property (nonatomic) CMTime maxFrameRate;
@end
