#import <Syphon/Syphon.h>
#import <Syphon/SyphonSubclassing.h>
#import <IOSurface/IOSurface.h>

@interface SyphonOBSClient : SyphonClientBase
- (id)initWithServerDescription:(NSDictionary<NSString *, id> *)description
                        options:(NSDictionary<NSString *, id> *)options
                newFrameHandler:(void (^)(SyphonOBSClient *))handler;

- (IOSurfaceRef)newFrameImage;
@end
