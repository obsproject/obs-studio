#import "SyphonOBSClient.h"

@implementation SyphonOBSClient

- (id)initWithServerDescription:(NSDictionary<NSString *, id> *)description
                        options:(NSDictionary<NSString *, id> *)options
                newFrameHandler:(void (^)(SyphonOBSClient *))handler
{
    self = [super initWithServerDescription:description options:options newFrameHandler:handler];
    return self;
}

- (IOSurfaceRef)newFrameImage
{
    return [self newSurface];
}
@end
