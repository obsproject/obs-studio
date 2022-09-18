//
//  MachClient.m
//  dal-plugin
//
//  Created by John Boiles  on 5/5/20.
//

#import "OBSDALMachClient.h"
#import "MachProtocol.h"
#import "Logging.h"

@interface OBSDALMachClient () <NSPortDelegate> {
	uint32_t _seed;
	NSPort *_receivePort;
}
@end

@implementation OBSDALMachClient

- (void)dealloc
{
	DLogFunc(@"");
	_receivePort.delegate = nil;
}

- (NSPort *)serverPort
{
// See note in MachServer.mm and don't judge me
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	return [[NSMachBootstrapServer sharedInstance]
		portForName:@MACH_SERVICE_NAME];
#pragma clang diagnostic pop
}

- (BOOL)isServerAvailable
{
	return [self serverPort] != nil;
}

- (NSPort *)receivePort
{
	if (_receivePort == nil) {
		NSPort *receivePort = [NSMachPort port];
		_receivePort = receivePort;
		_receivePort.delegate = self;
		__weak __typeof(self) weakSelf = self;
		dispatch_async(
			dispatch_get_global_queue(
				DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
			^{
				NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
				[runLoop addPort:receivePort
					 forMode:NSDefaultRunLoopMode];
				// weakSelf should become nil when this object gets destroyed
				while (weakSelf) {
					[[NSRunLoop currentRunLoop]
						runUntilDate:
							[NSDate dateWithTimeIntervalSinceNow:
									0.1]];
				}
				DLog(@"Shutting down receive run loop");
			});
		DLog(@"Initialized mach port %d for receiving",
		     ((NSMachPort *)_receivePort).machPort);
	}
	return _receivePort;
}

- (BOOL)connectToServer
{
	DLogFunc(@"");

	NSPort *sendPort = [self serverPort];
	if (sendPort == nil) {
		ELog(@"Unable to connect to server port");
		return NO;
	}

	NSPortMessage *message = [[NSPortMessage alloc]
		initWithSendPort:sendPort
		     receivePort:self.receivePort
		      components:nil];
	message.msgid = MachMsgIdConnect;

	NSDate *timeout = [NSDate dateWithTimeIntervalSinceNow:5.0];
	if (![message sendBeforeDate:timeout]) {
		ELog(@"sendBeforeDate failed");
		return NO;
	}

	return YES;
}

- (void)handlePortMessage:(NSPortMessage *)message
{
	VLogFunc(@"");
	NSArray *components = message.components;
	switch (message.msgid) {
	case MachMsgIdConnect:
		DLog(@"Received connect response");
		break;
	case MachMsgIdFrame:
		VLog(@"Received frame message");

		if (components.count < 4)
			return;

		@autoreleasepool {
			NSMachPort *framePort = (NSMachPort *)components[0];

			if (!framePort)
				return;

			IOSurfaceRef surface = IOSurfaceLookupFromMachPort(
				[framePort machPort]);
			mach_port_deallocate(mach_task_self(),
					     [framePort machPort]);

			if (!surface) {
				ELog(@"Failed to obtain IOSurface from Mach port");
				return;
			}

			IOSurfaceLock(surface, 0, &_seed);
			CVPixelBufferRef frame;
			CVPixelBufferCreateWithIOSurface(kCFAllocatorDefault,
							 surface, NULL, &frame);
			IOSurfaceUnlock(surface, 0, &_seed);
			CFRelease(surface);

			uint64_t timestamp;
			[components[1] getBytes:&timestamp
					 length:sizeof(timestamp)];

			VLog(@"Received frame data: %zux%zu (%llu)",
			     CVPixelBufferGetWidth(frame),
			     CVPixelBufferGetHeight(frame), timestamp);

			uint32_t fpsNumerator;
			[components[2] getBytes:&fpsNumerator
					 length:sizeof(fpsNumerator)];
			uint32_t fpsDenominator;
			[components[3] getBytes:&fpsDenominator
					 length:sizeof(fpsDenominator)];

			[self.delegate receivedPixelBuffer:frame
						 timestamp:timestamp
					      fpsNumerator:fpsNumerator
					    fpsDenominator:fpsDenominator];

			CVPixelBufferRelease(frame);
		}
		break;
	case MachMsgIdStop:
		DLog(@"Received stop message");
		[self.delegate receivedStop];
		break;
	default:
		ELog(@"Received unexpected response msgid %u",
		     (unsigned)message.msgid);
		break;
	}
}

@end
