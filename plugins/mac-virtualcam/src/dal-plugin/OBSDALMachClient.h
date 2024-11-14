//
//  MachClient.h
//  dal-plugin
//
//  Created by John Boiles  on 5/5/20.
//

#import <Foundation/Foundation.h>
#import <CoreVideo/CoreVideo.h>

NS_ASSUME_NONNULL_BEGIN

@protocol MachClientDelegate

- (void)receivedPixelBuffer:(CVPixelBufferRef)frame
                  timestamp:(uint64_t)timestamp
               fpsNumerator:(uint32_t)fpsNumerator
             fpsDenominator:(uint32_t)fpsDenominator;
- (void)receivedStop;

@end

@interface OBSDALMachClient : NSObject

@property (nullable, weak) id<MachClientDelegate> delegate;

- (BOOL)isServerAvailable;

- (BOOL)connectToServer;

@end

NS_ASSUME_NONNULL_END
