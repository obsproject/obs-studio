//
//  MachServer.h
//  obs-mac-virtualcam
//
//  Created by John Boiles  on 5/5/20.
//

@import Foundation;
@import CoreVideo;

NS_ASSUME_NONNULL_BEGIN

@interface OBSDALMachServer : NSObject

- (void)run;

/*!
 Will eventually be used for sending frames to all connected clients
 */
- (void)sendPixelBuffer:(CVPixelBufferRef)frame
	      timestamp:(uint64_t)timestamp
	   fpsNumerator:(uint32_t)fpsNumerator
	 fpsDenominator:(uint32_t)fpsDenominator;

- (void)stop;

@end

NS_ASSUME_NONNULL_END
