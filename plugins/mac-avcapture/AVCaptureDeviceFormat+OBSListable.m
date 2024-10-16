//
//  AVCaptureDeviceFormat+OBSListable.m
//  obs-studio
//
//  Created by jcm on 7/9/24.
//

#import "OBSAVCapture.h"

@implementation AVCaptureDeviceFormat (OBSListable)

- (NSString *)obsPropertyListDescription
{
    if (!objc_getAssociatedObject(self, @selector(obsPropertyListDescription))) {
        CMVideoDimensions formatDimensions = CMVideoFormatDescriptionGetDimensions(self.formatDescription);
        FourCharCode formatSubType = CMFormatDescriptionGetMediaSubType(self.formatDescription);
        NSString *pixelFormatDescription = [OBSAVCapture stringFromSubType:formatSubType];
        OBSAVCaptureColorSpace deviceColorSpace = [OBSAVCapture colorspaceFromDescription:self.formatDescription];
        NSString *colorspaceDescription = [OBSAVCapture stringFromColorspace:deviceColorSpace];
        NSString *fpsRangesDescription = [OBSAVCapture frameRateDescription:self.videoSupportedFrameRateRanges];
        NSString *aspectRatioString = [OBSAVCapture aspectRatioStringFromDimensions:formatDimensions];

        NSString *resolutionDescriptionComponent;
        if (formatDimensions.width > 0 && formatDimensions.height > 0) {
            resolutionDescriptionComponent = [NSString
                stringWithFormat:@"%dx%d (%@) - ", formatDimensions.width, formatDimensions.height, aspectRatioString];
        } else {
            resolutionDescriptionComponent = @"";
        }
        if (![fpsRangesDescription isEqualToString:@""]) {
            fpsRangesDescription = [fpsRangesDescription stringByAppendingString:@" - "];
        }
        if (deviceColorSpace != VIDEO_CS_DEFAULT) {
            colorspaceDescription = [colorspaceDescription stringByAppendingString:@" - "];
        } else {
            colorspaceDescription = @"";
        }
        if ([pixelFormatDescription isEqualToString:@"Unknown"]) {
            pixelFormatDescription = @"";
        }

        NSString *propertyListDescription = [NSString stringWithFormat:@"%@%@%@%@", resolutionDescriptionComponent,
                                                                       fpsRangesDescription, colorspaceDescription,
                                                                       pixelFormatDescription];
        if ([propertyListDescription isEqualToString:@""]) {
            propertyListDescription = @"Default";
        }
        objc_setAssociatedObject(self, @selector(obsPropertyListDescription), propertyListDescription,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return objc_getAssociatedObject(self, @selector(obsPropertyListDescription));
}

- (NSString *)obsPropertyListInternalRepresentation
{
    if (!objc_getAssociatedObject(self, @selector(obsPropertyListInternalRepresentation))) {
        CMVideoDimensions formatDimensions = CMVideoFormatDescriptionGetDimensions(self.formatDescription);
        FourCharCode formatSubType = CMFormatDescriptionGetMediaSubType(self.formatDescription);
        OBSAVCaptureColorSpace deviceColorSpace = [OBSAVCapture colorspaceFromDescription:self.formatDescription];
        NSString *frameRatesInternalDescription = @"";
        NSArray *sortedRanges = [self.videoSupportedFrameRateRanges
            sortedArrayUsingComparator:^NSComparisonResult(AVFrameRateRange *_Nonnull lhs,
                                                           AVFrameRateRange *_Nonnull rhs) {
                if (lhs.maxFrameRate > rhs.maxFrameRate) {
                    return NSOrderedDescending;
                } else if (lhs.maxFrameRate < rhs.maxFrameRate) {
                    return NSOrderedAscending;
                }
                if (lhs.minFrameRate > rhs.minFrameRate) {
                    return NSOrderedDescending;
                } else if (lhs.minFrameRate < rhs.minFrameRate) {
                    return NSOrderedAscending;
                }
                return NSOrderedSame;
            }];
        for (AVFrameRateRange *range in sortedRanges) {
            frameRatesInternalDescription = [frameRatesInternalDescription
                stringByAppendingFormat:@"%.3f-%.3f", range.minFrameRate, range.maxFrameRate];
        }
        NSString *internalRepresentation =
            [NSString stringWithFormat:@"%dx%d %@ %i %i", formatDimensions.width, formatDimensions.height,
                                       frameRatesInternalDescription, deviceColorSpace, formatSubType];
        NSData *internalRepresentationBytes = [internalRepresentation dataUsingEncoding:NSUTF8StringEncoding];
        NSString *base64String = [internalRepresentationBytes base64EncodedStringWithOptions:0];
        objc_setAssociatedObject(self, @selector(obsPropertyListInternalRepresentation), base64String,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return objc_getAssociatedObject(self, @selector(obsPropertyListInternalRepresentation));
}

- (NSNumber *)bitsPerPixel
{
    if (!objc_getAssociatedObject(self, @selector(bitsPerPixel))) {
        CMFormatDescriptionRef formatDescription = self.formatDescription;
        FourCharCode subtype = CMFormatDescriptionGetMediaSubType(formatDescription);
        UInt64 value;
        switch (subtype) {
            case kCVPixelFormatType_422YpCbCr8:
            case kCVPixelFormatType_422YpCbCr8_yuvs:
                //2x2 block, 4 bytes luma, 2 bytes Cr, 2 bytes Cb; 64 / 4
                value = 16;
            case kCVPixelFormatType_32BGRA:
                value = 32;
            case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
            case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
                //2x2 block, 4 bytes luma, 1 byte Cr, 1 byte Cb; 48 / 4
                value = 12;
            case kCVPixelFormatType_420YpCbCr10BiPlanarFullRange:
            case kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange:
                //2x2 block, 8 bytes luma, 2 bytes Cr, 2 bytes Cb; 96 / 4
                value = 24;
            default:
                //what other formats do we need to possibly account for?
                value = 32;
        }
        objc_setAssociatedObject(self, @selector(bitsPerPixel), @(value), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return objc_getAssociatedObject(self, @selector(bitsPerPixel));
}

- (NSNumber *)pixelBandwidthComparisonValue
{
    if (!objc_getAssociatedObject(self, @selector(pixelBandwidthComparisonValue))) {
        CMFormatDescriptionRef formatDescription = self.formatDescription;
        CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(formatDescription);
        NSNumber *bandwidth =
            @(dimensions.width * dimensions.height * self.videoSupportedFrameRateRanges.firstObject.maxFrameRate);
        objc_setAssociatedObject(self, @selector(pixelBandwidthComparisonValue), bandwidth,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return objc_getAssociatedObject(self, @selector(pixelBandwidthComparisonValue));
}

- (NSNumber *)aspectRatioComparisonValue
{
    if (!objc_getAssociatedObject(self, @selector(aspectRatioComparisonValue))) {
        CMFormatDescriptionRef formatDescription = self.formatDescription;
        CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(formatDescription);

        if (dimensions.height > 0) {
            //for sorting purposes, treat aspect ratios sufficiently close together as equivalent
            double ratio = (double) dimensions.width / (double) dimensions.height;
            double roundedRatio = round(ratio * 100) / 100;
            NSNumber *compareRatio = @(roundedRatio);

            objc_setAssociatedObject(self, @selector(aspectRatioComparisonValue), compareRatio,
                                     OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }
    return objc_getAssociatedObject(self, @selector(aspectRatioComparisonValue));
}

@end
