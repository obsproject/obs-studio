//
//  OBSAVCapture.h
//  mac-avcapture
//
//  Created by Patrick Heyer on 2023-03-07.
//

@import Foundation;
@import AVFoundation;
@import CoreMediaIO;

#import "OBSAVCapturePresetInfo.h"

// Import everything as libobs source data types are not available in a specific header.
#import <obs.h>
#import <pthread.h>
#import <media-io/video-io.h>

#pragma mark - Type aliases and type definitions

typedef enum video_colorspace OBSAVCaptureColorSpace;
typedef enum video_range_type OBSAVCaptureVideoRange;
typedef enum video_format OBSAVCaptureVideoFormat;
typedef struct obs_source_frame OBSAVCaptureVideoFrame;
typedef struct obs_source_audio OBSAVCaptureAudioFrame;
typedef struct gs_texture OBSAVCaptureTexture;
typedef struct gs_effect OBSAVCaptureEffect;

/// C struct for errors encountered in capture callback
typedef enum : NSUInteger {
    OBSAVCaptureError_NoError,
    OBSAVCaptureError_SampleBufferFormat,
    OBSAVCaptureError_ColorSpace,
    OBSAVCaptureError_AudioBuffer,
} OBSAVCaptureError;

/// C struct for interaction with obs-module functions
typedef struct av_capture {
    IOSurfaceRef previousSurface;
    IOSurfaceRef currentSurface;
    OBSAVCaptureTexture *texture;
    OBSAVCaptureEffect *effect;
    OBSAVCaptureVideoFrame *videoFrame;
    OBSAVCaptureAudioFrame *audioFrame;
    NSRect frameSize;

    pthread_mutex_t mutex;

    OBSAVCaptureColorSpace configuredColorSpace;
    OBSAVCaptureVideoRange configuredFourCC;

    void *settings;
    void *source;
    bool isFastPath;

    OBSAVCaptureError lastError;
    CMFormatDescriptionRef sampleBufferDescription;
    OBSAVCaptureError lastAudioError;
} OBSAVCaptureInfo;

/// C struct for sample buffer validity checks in capture callback
typedef struct av_capture_info {
    OBSAVCaptureColorSpace colorSpace;
    FourCharCode fourCC;
    bool isValid;
} OBSAVCaptureVideoInfo;

#pragma mark - OBSAVCapture Class

/// Video Capture implementation for [CoreMediaIO](https://developer.apple.com/documentation/coremediaio?language=objc)-based devices
///
/// Provides access to camera devices recognized by macOS via its [CoreMediaIO](https://developer.apple.com/documentation/coremediaio?language=objc) framework. Devices can be either entirely video-based or a "muxed" device that provides audio and video at the same time.
///
/// Devices can be configured either via [presets](https://developer.apple.com/documentation/avfoundation/avcapturesessionpreset?language=objc) (usually 3 quality-based presets in addition to resolution based presets). The resolution defined by the preset does not necessarily switch the actual device to the same resolution, instead the device is automatically switched to the best possible resolution and the [CMSampleBuffer](https://developer.apple.com/documentation/coremedia/cmsamplebuffer?language=objc) provided via [AVCaptureVideoDataOutput](https://developer.apple.com/documentation/avfoundation/avcapturevideodataoutput?language=objc) will be resized accordingly. If necessary the actual frame will be pillar-boxed to fit into a widescreen sample buffer in an attempt to fit the content into it.
///
/// Alternatively, devices can be configured manually by specifying a particular [AVCaptureDeviceFormat](https://developer.apple.com/documentation/avfoundation/avcapturedeviceformat?language=objc) representing a specific combination of resolution, frame-rate, color format and color space supported by the device. If a device was **not** configured via a preset originally, the size of the [CMSampleBuffer](https://developer.apple.com/documentation/coremedia/cmsamplebuffer?language=objc) will be adjusted to the selected resolution.
///
/// > Important: If a preset was configured before, the resolution of the last valid preset-based buffer will be retained and the frame will be fit into it with the selected resolution.
///
/// If a device is switched back from manual configuration to a preset-based output, the preset's original settings will be restored, as device configuration is not mutually exclusive with the settings of a preset on macOS.
@interface OBSAVCapture
    : NSObject <AVCaptureAudioDataOutputSampleBufferDelegate, AVCaptureVideoDataOutputSampleBufferDelegate>

/// Bare initialiser for ``OBSAVCapture`` class.
///
/// > Tip: Use ``OBSAVCapture/initWithCaptureInfo:capture_info:`` instead.
///
/// - Returns: A new instance of ``OBSAVCapture`` without settings or a source attached and without an active [AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc).
- (instancetype)init;

/// Creates new ``OBSAVCapture`` instance with provided ``mac_avcapture/OBSAVCaptureInfo`` struct. If settings and source pointers in the provided struct are valid, a new capture session will be created immediately and capture of a valid device will begin.
///
/// If the device specified in the settings is not valid (e.g., because the device has been disconnected in the meantime) the source will retain the settings and can be reconfigured. If no valid device is specified, the source will stay empty.
/// - Parameters:
///   - capture_info: ``OBSAVCaptureInfo`` struct containing source and settings pointers provided by ``libobs``
/// - Returns: A new instance of an ``OBSAVCapture``
- (instancetype)initWithCaptureInfo:(OBSAVCaptureInfo *)capture_info NS_DESIGNATED_INITIALIZER;

#pragma mark - Capture Session Handling

/// Creates a new [AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc) for the device configured in the ``OBSAVCapture`` instance.
/// - Parameters:
///   - error: Optional pointer to a valid [NSError](https://developer.apple.com/documentation/foundation/nserror?language=objc) instance to retain possible errors that occurred
/// - Returns: `YES` if session was created successfully, `NO` otherwise
- (BOOL)createSession:(NSError **)error;

/// Switches the active capture device used by ``OBSAVCapture`` instance.
/// - Parameters:
///   - uuid: UUID of new device to switch to (as provided by [CoreMediaIO](https://developer.apple.com/documentation/coremediaio?language=objc)
///   - error: Optional pointer to a valid [NSError](https://developer.apple.com/documentation/foundation/nserror?language=objc) instance to retain possible errors that occurred
/// - Returns: `YES` if the device was successfully switched, `NO` otherwise
- (BOOL)switchCaptureDevice:(NSString *)uuid withError:(NSError **)error;

/// Starts a capture session with the active capture device used by ``OBSAVCapture`` instance.
- (void)startCaptureSession;

/// Stops a capture session with the active capture device used by ``OBSAVCapture`` instance. Also sends an empty frame to clear any frames provided by the source.
- (void)stopCaptureSession;

/// Configures the current [AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc) to use the [AVCaptureSessionPreset](https://developer.apple.com/documentation/avfoundation/avcapturesessionpreset?language=objc) value selected in the source's property window.
/// - Parameter error: Optional pointer to a valid [NSError](https://developer.apple.com/documentation/foundation/nserror?language=objc) instance to retain possible errors that occurred
/// - Returns: `YES` if configuration finished successfully, `NO` otherwise
- (BOOL)configureSessionWithPreset:(AVCaptureSessionPreset)preset withError:(NSError **)error;

/// Configures the device used in the current [AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc) directly by attempting to apply the values selected in the source's property window.
///
/// The available values are commonly filled by all settings available for the current [AVCaptureDevice](https://developer.apple.com/documentation/avfoundation/avcapturedevice?language=objc). This includes:
/// * Resolution
/// * Frame rate
/// * Color Format
/// * Color Space
///
/// If the combination of property values read from the settings does not match any format supported by the [AVCaptureDevice](https://developer.apple.com/documentation/avfoundation/avcapturedevice?language=objc), the session will not be configured and if a valid [AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc) exist, it will be kept active.
/// - Parameter error: Optional pointer to a valid [NSError](https://developer.apple.com/documentation/foundation/nserror?language=objc) instance to retain possible errors that occurred
/// - Returns: `YES` if configuration finished successfully, `NO` otherwise
- (BOOL)configureSession:(NSError **)error;

/// Triggers an update of the current ``OBSAVCapture`` source using the property values currently set up on the source.
///
/// The function will automatically call ``switchCaptureDevice:uuid:error`` if the device property was changed, and also call ``configureSession:error`` or ``configureSessionWithPreset:preset:error`` based on the value of the associated source property.
///
/// A new [AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc) will be created and started to reflect any changes.
///
/// - Parameter error: Optional pointer to a valid [NSError](https://developer.apple.com/documentation/foundation/nserror?language=objc) instance to retain possible errors that occurred
/// - Returns: `YES` if session was updated successfully, `NO` otherwise
- (BOOL)updateSessionwithError:(NSError **)error;

#pragma mark - OBS Settings Helpers

/// Reads source dimensions from the legacy user settings and converts them into a [CMVideoDimensions](https://developer.apple.com/documentation/coremedia/cmvideodimensions?language=objc) struct for convenience when interacting with the [CoreMediaIO](https://developer.apple.com/documentation/coremediaio?language=objc) framework.
/// - Parameter settings: Pointer to settings struct used by ``libobs``
/// - Returns: [CMVideoDimensions](https://developer.apple.com/documentation/coremedia/cmvideodimensions?language=objc) struct with resolution from user settings
+ (CMVideoDimensions)legacyDimensionsFromSettings:(void *)settings;

/// Generates a new [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance containing a human-readable aspect ratio for a given pixel width and height.
/// - Parameter dimensions: [CMVideoDimensions](https://developer.apple.com/documentation/coremedia/cmvideodimensions?language=objc) struct containing the width and height in pixels.
/// - Returns: New [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance containing the aspect ratio description.
/// For resolutions with too low of a common divisor (i.e. 2.35:1, resolutions slightly off of a common aspect ratio), this function provides the ratio between 1 and the larger float value.
+ (NSString *)aspectRatioStringFromDimensions:(CMVideoDimensions)dimensions;

/// Reads a C-character pointer from user settings and converts it into an [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance.
/// - Parameters:
///   - settings: Pointer to user settings struct used by ``libobs``
///   - setting: String identifier for setting
/// - Returns: New [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance created from user setting if setting represented a valid C character pointer.
+ (NSString *)stringFromSettings:(void *)settings withSetting:(NSString *)setting;

/// Reads a C-character pointer from user settings and converts it into an [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance.
/// - Parameters:
///   - settings: Pointer to user settings struct used by ``libobs``
///   - setting: String identifer for setting
///   - widthDefault: Optional fallback value to use if C-character pointer read from settings is invalid
/// - Returns: New [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance created from user setting if setting represented a valid C character pointer.
+ (NSString *)stringFromSettings:(void *)settings withSetting:(NSString *)setting withDefault:(NSString *)defaultValue;

/// Generates an NSString representing the name of the warning to display in the properties window for macOS system effects that are active on a particular `AVCaptureDevice`.
/// - Parameter device: The [AVCaptureDevice](https://developer.apple.com/documentation/avfoundation/avcapturedevice?language=objc) to generate an effects warning string for.
/// - Returns: `nil` if there are no effects active on the device. If effects are found, returns a new [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance containing the `libobs` key used to retrieve the appropriate localized warning string.
+ (NSString *)effectsWarningForDevice:(AVCaptureDevice *)device;

#pragma mark - Format Conversion Helpers

/// Converts a FourCC-based color format identifier into a human-readable string represented as an [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance.
/// - Parameter subtype: FourCC-based color format identifier
/// - Returns: Human-readable representation of the color format
+ (NSString *)stringFromSubType:(FourCharCode)subtype;

/// Converts a ``libobs``-based color space value into a human-readable string represented as an [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance.
/// - Parameter subtype: ``libobs``-based colorspace value
/// - Returns: Human-readable representation of the color space
+ (NSString *)stringFromColorspace:(enum video_colorspace)colorspace;

/// Converts a ``libobs``-based video range value into a human-readable string represented as an [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance.
/// - Parameter subtype: ``libobs``-based video range value
/// - Returns: Human-readable representation of the video range
+ (NSString *)stringFromVideoRange:(enum video_range_type)videoRange;

/// Converts a FourCC value into a human-readable string represented as an [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance.
/// - Parameter fourCharCode: Arbitrary FourCC code in big-endian format
/// - Returns: Human-readable representation of the FourCC code
+ (NSString *)stringFromFourCharCode:(OSType)fourCharCode;

/// Converts a [AVCaptureSessionPreset](https://developer.apple.com/documentation/avfoundation/avcapturesessionpreset?language=objc) into a human-readable string represented as an [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance.
///
/// Supported presets include:
/// * High preset
/// * Medium preset
/// * Low preset
/// * Common resolutions ranging from 320x240 up to and including 3840x2160 (4k)
/// - Parameter preset: Supported [AVCaptureSessionPreset](https://developer.apple.com/documentation/avfoundation/avcapturesessionpreset?language=objc)
/// - Returns: Human-readable representation of the [AVCaptureSessionPreset](https://developer.apple.com/documentation/avfoundation/avcapturesessionpreset?language=objc)
+ (NSString *)stringFromCapturePreset:(AVCaptureSessionPreset)preset;

/// Converts an [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) into a big-endian FourCC value.
/// - Parameter codeString: [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) representation of a big-endian FourCC value
/// - Returns: Big-endian FourCC value of the provided [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc)
+ (FourCharCode)fourCharCodeFromString:(NSString *)codeString;

/// Checks whether the provided colorspace value is actually supported by ``libobs``.
/// - Parameter colorSpace: ``libobs``-based color-space value
/// - Returns: `YES` if provided color space value is supported, `NO` otherwise
+ (BOOL)isValidColorspace:(enum video_colorspace)colorSpace;

/// Checks whether the provided video range value is actually supported by ``libobs``.
/// - Parameter videoRange: ``libobs``-based video range value
/// - Returns: `YES` if provided video range value is supported, `NO` otherwise
+ (BOOL)isValidVideoRange:(enum video_range_type)videoRange;

/// Checks whether the provided FourCC-based pixel format is a full video range variant.
/// - Parameter pixelFormat: FourCC code of the pixel format in big-endian format
/// - Returns: `YES` if provided pixel format has full video range, `NO` otherwise
+ (BOOL)isFullRangeFormat:(FourCharCode)pixelFormat;

/// Converts a FourCC-based media subtype in big-endian format to a video format understood by ``libobs``.
/// - Parameter subtype: FourCC code of the media subtype in big-endian format
/// - Returns: Video format identifier understood by ``libobs``
+ (OBSAVCaptureVideoFormat)formatFromSubtype:(FourCharCode)subtype;

/// Converts a ``libobs``based video format its FourCC-based media subtype in big-endian format
/// - Parameter format: ``libobs``-based video format
/// - Returns: FourCC-based media subtype in big-endian format
+ (FourCharCode)fourCharCodeFromFormat:(OBSAVCaptureVideoFormat)format;

/// Converts a ``libobs``based video format with a provided video range into its FourCC-based media subtype in big-endian format
/// - Parameters:
///   -  format: ``libobs``-based video format
///   - videoRange: ``libobs``-based video range
/// - Returns: FourCC-based media subtype in big-endian format

+ (FourCharCode)fourCharCodeFromFormat:(OBSAVCaptureVideoFormat)format withRange:(enum video_range_type)videoRange;

/// Generates a string describing an array of frame rate ranges.
/// - Parameter ranges: [NSArray](https://developer.apple.com/documentation/foundation/nsarray?language=objc) of [AVFrameRateRange](https://developer.apple.com/documentation/avfoundation/avframeraterange?language=objc), such as might be provided by an `AVCaptureDeviceFormat` instance's [videoSupportedFrameRateRanges](https://developer.apple.com/documentation/avfoundation/avcapturedeviceformat/1387592-videosupportedframerateranges) property.
/// - Returns: A new [NSString](https://developer.apple.com/documentation/foundation/nsstring?language=objc) instance that describes the frame rate ranges.
+ (NSString *)frameRateDescription:(NSArray<AVFrameRateRange *> *)ranges;

/// Converts a [CMFormatDescription](https://developer.apple.com/documentation/coremedia/cmformatdescription?language=objc) into a ``libobs``-based color space value
/// - Parameter description: A [CMFormatDescription](https://developer.apple.com/documentation/coremedia/cmformatdescription?language=objc) media format descriptor
/// - Returns: A ``libobs``-based color space value
+ (OBSAVCaptureColorSpace)colorspaceFromDescription:(CMFormatDescriptionRef)description;

#pragma mark - Notification Handlers

/// Notification center callback function for [AVCaptureDeviceWasDisconnected](https://developer.apple.com/documentation/avfoundation/avcapturedevicewasdisconnectednotification?language=objc) notification.
/// - Parameter notification: [NSNotification](https://developer.apple.com/documentation/foundation/nsnotification?language=objc) container for notification
- (void)deviceDisconnected:(NSNotification *)notification;

/// Notification center callback function for [AVCaptureDeviceWasConnected](https://developer.apple.com/documentation/avfoundation/avcapturedevicewasconnectednotification?language=objc) notification.
/// - Parameter notification: [NSNotification](https://developer.apple.com/documentation/foundation/nsnotification?language=objc) container for notification
- (void)deviceConnected:(NSNotification *)notification;

#pragma mark - Log Helpers

/// ObjC-based wrapper for ``libobs`` logging. This instance function automatically adds the localized ``OBSAVCapture`` source name to all provided log strings.
///
/// The signature for string composition is similar to [NSString:stringWithFormat](https://developer.apple.com/documentation/foundation/nsstring/1497275-stringwithformat?language=objc), accepting a `printf`-like format string with added support for ObjC types:
///
/// ```objc
/// [self AVCaptureLog:LOG_WARNING withFormat:@"%@ - %i", @"Some String", 12];
/// ```
///
/// - Parameters:
///   - logLevel: ``libobs``-based log severity level
///   - format: [NSString:stringWithFormat:](https://developer.apple.com/documentation/foundation/nsstring/1497275-stringwithformat?language=objc)-compatible format string]
- (void)AVCaptureLog:(int)logLevel withFormat:(NSString *)format, ...;

/// ObjC-based wrapper for ``libobs`` logging. This class function is available for ObjC code without access to an existing ``OBSAVCapture`` instance.
///
/// The signature for string composition is similar to [NSString:stringWithFormat](https://developer.apple.com/documentation/foundation/nsstring/1497275-stringwithformat?language=objc), accepting a `printf`-like format string with added support for ObjC types:
///
/// ```objc
/// [self AVCaptureLog:LOG_WARNING withFormat:@"%@ - %i", @"Some String", 12];
/// ```
///
/// - Parameters:
///   - logLevel: ``libobs``-based log severity level
///   - format: [NSString:stringWithFormat:](https://developer.apple.com/documentation/foundation/nsstring/1497275-stringwithformat?language=objc)-compatible format string]
+ (void)AVCaptureLog:(int)logLevel withFormat:(NSString *)format, ...;

#pragma mark - Instance Properties
/// Internal reference to ``OBSAVCaptureInfo`` struct created by ``libobs`` module code.
@property (nonatomic) OBSAVCaptureInfo *captureInfo;

/// ``OBSVideoCaptureVideoInfo`` struct used to hold state information of the video configuration
@property OBSAVCaptureVideoInfo videoInfo;

/// ``libobs``-based frame struct represented by a [NSMutableData](https://developer.apple.com/documentation/foundation/nsmutabledata?language=objc) instance
@property NSMutableData *obsFrame;

/// ``libobs``-based audio frame struct represented by a [NSMutableData](https://developer.apple.com/documentation/foundation/nsmutabledata?language=objc) instance
@property NSMutableData *obsAudioFrame;

/// Dictionary of human-readable descriptions of [AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc) values
@property NSDictionary<NSString *, NSString *> *presetList;

/// UUID of [AVCaptureDevice](https://developer.apple.com/documentation/avfoundation/avcapturedevice?language=objc) currently used by the ``OBSAVCapture`` instance
@property NSString *deviceUUID;

/// Instance of ``OBSAVCapturePresetInfo`` to store format and frame rate of a [AVCaptureSessionPreset](https://developer.apple.com/documentation/avfoundation/avcapturesessionpreset?language=objc).
@property OBSAVCapturePresetInfo *presetFormat;

/// Instance of [AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc) used by ``OBSAVCapture``
@property AVCaptureSession *session;

/// Instance of [AVCaptureDeviceInput](https://developer.apple.com/documentation/avfoundation/avcapturesession?language=objc) used by ``OBSAVCapture``
@property AVCaptureDeviceInput *deviceInput;

/// Instance of [AVCaptureVideoDataOutput](https://developer.apple.com/documentation/avfoundation/avcapturevideodataoutput?language=objc) used by ``OBSAVCapture``
@property AVCaptureVideoDataOutput *videoOutput;

/// Instance of [AVCaptureAudioDataOutput](https://developer.apple.com/documentation/avfoundation/avcaptureaudiodataoutput?language=objc) used by ``OBSAVCapture``
@property AVCaptureAudioDataOutput *audioOutput;

/// [Dispatch Queue](https://developer.apple.com/documentation/dispatch/dispatch_queue?language=objc) used by the [AVCaptureVideoDataOutput](https://developer.apple.com/documentation/avfoundation/avcapturevideodataoutput?language=objc) instance
@property dispatch_queue_t videoQueue;
/// [Dispatch Queue](https://developer.apple.com/documentation/dispatch/dispatch_queue?language=objc) used by the [AVCaptureAudioDataOutput](https://developer.apple.com/documentation/avfoundation/avcaptureaudiodataoutput?language=objc) instance
@property dispatch_queue_t audioQueue;

/// [Dispatch Queue](https://developer.apple.com/documentation/dispatch/dispatch_queue?language=objc) used by asynchronous blocks for functions that are required to not block the main thread.
@property (nonatomic) dispatch_queue_t sessionQueue;

/// `YES` if the device's active format is set and is locked against configuration changes, `NO` otherwise
@property BOOL isDeviceLocked;

/// `YES` if the device's configuration is based on a preset, `NO` otherwise
@property BOOL isPresetBased;

/// `YES` if the ``OBSAVCapture`` instance handles frame rendering by itself (used by the Capture Card variant), `NO` otherwise
@property (readonly) BOOL isFastPath;

/// Error domain identifier used for [NSError](https://developer.apple.com/documentation/foundation/nserror?language=objc) instances
@property (readonly) NSString *errorDomain;

@end

#pragma mark - Static helper functions

/// Clamp an unsigned 64-bit integer value to the specified minimum and maximum values
static inline UInt64 clamp_Uint(UInt64 value, UInt64 min, UInt64 max)
{
    const UInt64 clamped = value < min ? min : value;

    return clamped > max ? max : value;
}

/// Clamp a signed 64-bit integer value to the specified minimum and maximum values
static inline SInt64 clamp_Sint(SInt64 value, SInt64 min, SInt64 max)
{
    const SInt64 clamped = value < min ? min : value;

    return clamped > max ? max : value;
}

/// Compute the greatest common divisor of two signed 32-bit integers.
static inline SInt32 gcd(SInt32 a, SInt32 b)
{
    return (b == 0) ? a : gcd(b, a % b);
}
