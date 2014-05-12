#import <AVFoundation/AVFoundation.h>

#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && \
				__MAC_OS_X_VERSION_MAX_ALLOWED < 1090
@interface AVCaptureInputPort (PreMavericksCompat)
@property(nonatomic, readonly) CMClockRef clock;
@end
#endif
