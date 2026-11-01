//
//  AVCaptureDeviceFormat+OBSListable.h
//  obs-studio
//
//  Created by jcm on 7/9/24.
//

/// [AVCaptureDeviceFormat](https://developer.apple.com/documentation/avfoundation/avcapturedeviceformat) class customization adding comparison functionality as well as stored properties for sort order and descriptions. Used to present multiple `AVCaptureDeviceFormat`s in a sorted OBS property list.
@interface AVCaptureDeviceFormat (OBSListable)

/// Lazily computed property representing a scalar for the potential 'pixel throughput' of the format. Computed by multiplying together the resolution area and maximum frames per second.
@property (nonatomic, strong) NSNumber *pixelBandwidthComparisonValue;

/// Lazily computed property representing the aspect ratio value used for comparing to other format aspect ratios. Given by the width divided by height, to only two decimal places. Low precision is used to account for resolutions with a precise aspect ratio that is not equal to their semantic aspect ratio. For example, 854x480 and 1366x768 are semantically both 16:9, but not precisely 16:9.
@property (nonatomic, strong) NSNumber *aspectRatioComparisonValue;

/// Lazily computed property containing the bits per pixel for the format. For planar formats, accounts for the number of bytes of chrominance and luminance per pixel block.
@property (nonatomic, strong) NSNumber *bitsPerPixel;

/// Lazily computed property containing a general localized description of the format, suitable for use in picking a format in the OBS source properties window.
@property (nonatomic, strong) NSString *obsPropertyListDescription;

/// Lazily computed property containing the string value used by OBS to represent and uniquely identify the format by its dimensions, supported frame rate ranges, color space and [four character code](https://developer.apple.com/documentation/coremedia/1489255-cmformatdescriptiongetmediasubty?language=objc).
@property (nonatomic, strong) NSString *obsPropertyListInternalRepresentation;

@end
