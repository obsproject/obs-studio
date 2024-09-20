//
//  OBSAVCapturePresetInfo.m
//  mac-avcapture
//
//  Created by Patrick Heyer on 2023-03-07.
//

#import <OBSAVCapturePresetInfo.h>

@implementation OBSAVCapturePresetInfo

- (instancetype)init
{
    self = [super init];

    if (self) {
        _activeFormat = nil;
        _minFrameRate = CMTimeMake(10000, 300);
        _maxFrameRate = CMTimeMake(10000, 300);
    }

    return self;
}

@end
