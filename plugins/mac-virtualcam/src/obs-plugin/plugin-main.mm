@import CoreMediaIO;
@import SystemExtensions;

#include <obs-module.h>
#include "OBSDALMachServer.h"
#include "Defines.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("mac-virtualcam", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "macOS virtual webcam output";
}

NSString *const OBSDalDestination = @"/Library/CoreMediaIO/Plug-Ins/DAL";

static bool cmio_extension_supported()
{
	if (@available(macOS 13.0, *)) {
		return true;
	} else {
		return false;
	}
}

struct virtualcam_data {
	obs_output_t *output;
	obs_video_info videoInfo;
	CVPixelBufferPoolRef pool;

	// CMIO Extension (available with macOS 13)
	CMSimpleQueueRef queue;
	CMIODeviceID deviceID;
	CMIOStreamID streamID;
	CMFormatDescriptionRef formatDescription;
	id extensionDelegate;

	// Legacy DAL (deprecated since macOS 12.3)
	OBSDALMachServer *machServer;
};

@interface SystemExtensionActivationDelegate
	: NSObject <OSSystemExtensionRequestDelegate> {
@private
	struct virtualcam_data *_vcam;
}

@property (getter=isInstalled) BOOL installed;
@property NSString *lastErrorMessage;
- (instancetype)init __unavailable;
@end

@implementation SystemExtensionActivationDelegate

- (id)initWithVcam:(virtualcam_data *)vcam
{
	self = [super init];

	if (self) {
		_vcam = vcam;
		_installed = NO;
	}

	return self;
}

- (OSSystemExtensionReplacementAction)
			    request:(nonnull OSSystemExtensionRequest *)request
	actionForReplacingExtension:
		(nonnull OSSystemExtensionProperties *)existing
		      withExtension:(nonnull OSSystemExtensionProperties *)ext
{
	NSString *extVersion =
		[NSString stringWithFormat:@"%@.%@", [ext bundleShortVersion],
					   [ext bundleVersion]];
	NSString *existingVersion = [NSString
		stringWithFormat:@"%@.%@", [existing bundleShortVersion],
				 [existing bundleVersion]];

	if ([extVersion compare:existingVersion
			options:NSNumericSearch] == NSOrderedDescending) {
		return OSSystemExtensionReplacementActionReplace;
	} else {
		return OSSystemExtensionReplacementActionCancel;
	}
}

- (void)request:(nonnull OSSystemExtensionRequest *)request
	didFailWithError:(nonnull NSError *)error
{
	NSString *errorMessage;
	int severity;

	switch (error.code) {
	case OSSystemExtensionErrorRequestCanceled:
		errorMessage =
			@"macOS Camera Extension installation request cancelled.";
		severity = LOG_INFO;
		break;
	case OSSystemExtensionErrorUnsupportedParentBundleLocation:
		self.lastErrorMessage = [NSString
			stringWithUTF8String:
				obs_module_text(
					"Error.SystemExtension.WrongLocation")];
		errorMessage = self.lastErrorMessage;
		severity = LOG_WARNING;
		break;
	default:
		self.lastErrorMessage = error.localizedDescription;
		errorMessage = [NSString
			stringWithFormat:
				@"OSSystemExtensionErrorCode %ld (\"%s\")",
				error.code,
				error.localizedDescription.UTF8String];
		severity = LOG_ERROR;
		break;
	}

	blog(severity, "mac-camera-extension error: %s",
	     errorMessage.UTF8String);
}

- (void)request:(nonnull OSSystemExtensionRequest *)request
	didFinishWithResult:(OSSystemExtensionRequestResult)result
{
	self.installed = YES;
	blog(LOG_INFO, "macOS Camera Extension activated successfully.");
}

- (void)requestNeedsUserApproval:(nonnull OSSystemExtensionRequest *)request
{
	self.installed = NO;
	blog(LOG_INFO, "macOS Camera Extension user approval required.");
}

@end

static void install_cmio_system_extension(struct virtualcam_data *vcam)
{
	OSSystemExtensionRequest *request = [OSSystemExtensionRequest
		activationRequestForExtension:
			@"com.obsproject.obs-studio.mac-camera-extension"
					queue:dispatch_get_main_queue()];
	request.delegate = vcam->extensionDelegate;

	[[OSSystemExtensionManager sharedManager] submitRequest:request];
}

typedef enum {
	OBSDalPluginNotInstalled,
	OBSDalPluginInstalled,
	OBSDalPluginNeedsUpdate
} dal_plugin_status;

static dal_plugin_status check_dal_plugin()
{
	NSFileManager *fileManager = [NSFileManager defaultManager];

	NSString *dalPluginFileName = [OBSDalDestination
		stringByAppendingString:@"/obs-mac-virtualcam.plugin"];

	BOOL dalPluginInstalled =
		[fileManager fileExistsAtPath:dalPluginFileName];

	if (dalPluginInstalled) {
		NSDictionary *dalPluginInfoPlist = [NSDictionary
			dictionaryWithContentsOfURL:
				[NSURL fileURLWithPath:
						[OBSDalDestination
							stringByAppendingString:
								@"/obs-mac-virtualcam.plugin/Contents/Info.plist"]]];

		NSString *dalPluginVersion = [dalPluginInfoPlist
			valueForKey:@"CFBundleShortVersionString"];
		NSString *dalPluginBuild =
			[dalPluginInfoPlist valueForKey:@"CFBundleVersion"];

		NSString *obsVersion = [[[NSBundle mainBundle] infoDictionary]
			objectForKey:@"CFBundleShortVersionString"];
		NSString *obsBuild = [[[NSBundle mainBundle] infoDictionary]
			objectForKey:(NSString *)kCFBundleVersionKey];
		BOOL dalPluginUpdateNeeded =
			!([dalPluginVersion isEqualToString:obsVersion] &&
			  [dalPluginBuild isEqualToString:obsBuild]);

		return dalPluginUpdateNeeded ? OBSDalPluginNeedsUpdate
					     : OBSDalPluginInstalled;
	}

	return OBSDalPluginNotInstalled;
}

static bool install_dal_plugin(bool update)
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	BOOL dalPluginDirExists =
		[fileManager fileExistsAtPath:OBSDalDestination];

	NSURL *bundleURL = [[NSBundle mainBundle] bundleURL];
	NSString *pluginPath = @"Contents/Resources/obs-mac-virtualcam.plugin";

	NSURL *pluginUrl = [bundleURL URLByAppendingPathComponent:pluginPath];
	NSString *dalPluginSourcePath = [pluginUrl path];

	NSString *createPluginDirCmd =
		(!dalPluginDirExists)
			? [NSString stringWithFormat:@"mkdir -p '%@' && ",
						     OBSDalDestination]
			: @"";
	NSString *deleteOldPluginCmd =
		(update) ? [NSString stringWithFormat:@"rm -rf '%@' && ",
						      OBSDalDestination]
			 : @"";
	NSString *copyPluginCmd = [NSString
		stringWithFormat:@"cp -R '%@' '%@'", dalPluginSourcePath,
				 OBSDalDestination];

	if ([fileManager fileExistsAtPath:dalPluginSourcePath]) {
		NSString *copyCmd = [NSString
			stringWithFormat:
				@"do shell script \"%@%@%@\" with administrator privileges",
				createPluginDirCmd, deleteOldPluginCmd,
				copyPluginCmd];

		NSDictionary *errorDict;
		NSAppleScript *scriptObject =
			[[NSAppleScript alloc] initWithSource:copyCmd];
		[scriptObject executeAndReturnError:&errorDict];
		if (errorDict != nil) {
			const char *errorMessage = [[errorDict
				objectForKey:@"NSAppleScriptErrorMessage"]
				UTF8String];

			blog(LOG_INFO,
			     "[macOS] VirtualCam DAL Plugin Installation status: %s",
			     errorMessage);
			return false;
		} else {
			return true;
		}
	} else {
		blog(LOG_INFO,
		     "[macOS] VirtualCam DAL Plugin not shipped with OBS");
		return false;
	}
}

static bool uninstall_dal_plugin()
{
	NSAppleScript *scriptObject = [[NSAppleScript alloc]
		initWithSource:
			[NSString
				stringWithFormat:
					@"do shell script \"rm -rf %@/obs-mac-virtualcam.plugin\" with administrator privileges",
					OBSDalDestination]];

	NSDictionary *errorDict;

	[scriptObject executeAndReturnError:&errorDict];
	if (errorDict) {
		blog(LOG_INFO,
		     "[macOS] VirtualCam DAL Plugin could not be uninstalled: %s",
		     [[errorDict objectForKey:NSAppleScriptErrorMessage]
			     UTF8String]);
		return false;
	} else {
		return true;
	}
}

FourCharCode convert_video_format_to_mac(enum video_format format,
					 enum video_range_type range)
{
	switch (format) {
	case VIDEO_FORMAT_I420:
		return (range == VIDEO_RANGE_FULL)
			       ? kCVPixelFormatType_420YpCbCr8PlanarFullRange
			       : kCVPixelFormatType_420YpCbCr8Planar;
	case VIDEO_FORMAT_NV12:
		return (range == VIDEO_RANGE_FULL)
			       ? kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
			       : kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
	case VIDEO_FORMAT_UYVY:
		return (range == VIDEO_RANGE_FULL)
			       ? kCVPixelFormatType_422YpCbCr8FullRange
			       : kCVPixelFormatType_422YpCbCr8;
	case VIDEO_FORMAT_P010:
		return (range == VIDEO_RANGE_FULL)
			       ? kCVPixelFormatType_420YpCbCr10BiPlanarFullRange
			       : kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange;
	default:
		// Zero indicates that the format is not supported on macOS
		// Note that some formats do have an associated constant, but
		// constructing such formats fails with kCVReturnInvalidPixelFormat.
		return 0;
	}
}

static const char *virtualcam_output_get_name(void *type_data)
{
	(void)type_data;
	return obs_module_text("Plugin_Name");
}

static void *virtualcam_output_create(obs_data_t *settings,
				      obs_output_t *output)
{
	UNUSED_PARAMETER(settings);

	struct virtualcam_data *vcam =
		(struct virtualcam_data *)bzalloc(sizeof(*vcam));

	vcam->output = output;

	if (cmio_extension_supported()) {
		vcam->extensionDelegate =
			[[SystemExtensionActivationDelegate alloc]
				initWithVcam:vcam];
		install_cmio_system_extension(vcam);
	} else {
		vcam->machServer = [[OBSDALMachServer alloc] init];
	}

	return vcam;
}

static void virtualcam_output_destroy(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;

	if (cmio_extension_supported()) {
		vcam->extensionDelegate = nil;
	} else {
		vcam->machServer = nil;
	}

	bfree(vcam);
}

static bool virtualcam_output_start(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;

	dal_plugin_status dal_status = check_dal_plugin();

	if (cmio_extension_supported()) {
		if (dal_status != OBSDalPluginNotInstalled) {
			if (!uninstall_dal_plugin()) {
				obs_output_set_last_error(
					vcam->output,
					obs_module_text(
						"Error.DAL.NotUninstalled"));
				return false;
			}
		}

		SystemExtensionActivationDelegate *delegate =
			vcam->extensionDelegate;

		if (!delegate.installed) {
			if (delegate.lastErrorMessage) {
				obs_output_set_last_error(
					vcam->output,
					[NSString
						stringWithFormat:
							@"%s\n\n%@",
							obs_module_text(
								"Error.SystemExtension.InstallationError"),
							delegate.lastErrorMessage]
						.UTF8String);
			} else {
				obs_output_set_last_error(
					vcam->output,
					obs_module_text(
						"Error.SystemExtension.NotInstalled"));
			}

			return false;
		}
	} else {
		bool success = false;
		if (dal_status == OBSDalPluginNotInstalled) {
			success = install_dal_plugin(false);
		} else if (dal_status == OBSDalPluginNeedsUpdate) {
			success = install_dal_plugin(true);
		} else {
			success = true;
		}

		if (!success) {
			obs_output_set_last_error(vcam->output,
						  "Error.DAL.NotInstalled");
			return false;
		}
	}

	obs_get_video_info(&vcam->videoInfo);

	FourCharCode video_format = convert_video_format_to_mac(
		vcam->videoInfo.output_format, vcam->videoInfo.range);

	struct video_scale_info conversion = {};
	conversion.width = vcam->videoInfo.output_width;
	conversion.height = vcam->videoInfo.output_height;
	conversion.colorspace = vcam->videoInfo.colorspace;
	conversion.range = vcam->videoInfo.range;

	if (!video_format) {
		// Selected output format is not supported natively by CoreVideo, CPU conversion necessary
		blog(LOG_WARNING,
		     "Selected output format (%s) not supported by CoreVideo, enabling CPU transcoding...",
		     get_video_format_name(vcam->videoInfo.output_format));

		conversion.format = VIDEO_FORMAT_NV12;
		video_format = convert_video_format_to_mac(conversion.format,
							   conversion.range);
	} else {
		conversion.format = vcam->videoInfo.output_format;
	}
	obs_output_set_video_conversion(vcam->output, &conversion);

	NSDictionary *pAttr = @{};
	NSDictionary *pbAttr = @{
		(id)kCVPixelBufferPixelFormatTypeKey: @(video_format),
		(id)kCVPixelBufferWidthKey: @(vcam->videoInfo.output_width),
		(id)kCVPixelBufferHeightKey: @(vcam->videoInfo.output_height),
		(id)kCVPixelBufferIOSurfacePropertiesKey: @{}
	};
	CVReturn status = CVPixelBufferPoolCreate(
		kCFAllocatorDefault, (__bridge CFDictionaryRef)pAttr,
		(__bridge CFDictionaryRef)pbAttr, &vcam->pool);

	if (status != kCVReturnSuccess) {
		blog(LOG_ERROR,
		     "unable to allocate pixel buffer pool (error %d)", status);
		return false;
	}

	if (cmio_extension_supported()) {
		UInt32 size;
		UInt32 used;

		CMIOObjectPropertyAddress address{
			.mSelector = kCMIOHardwarePropertyDevices,
			.mScope = kCMIOObjectPropertyScopeGlobal,
			.mElement = kCMIOObjectPropertyElementMain};
		CMIOObjectGetPropertyDataSize(kCMIOObjectSystemObject, &address,
					      0, NULL, &size);
		size_t num_devices = size / sizeof(CMIOObjectID);
		CMIOObjectID cmio_devices[num_devices];
		CMIOObjectGetPropertyData(kCMIOObjectSystemObject, &address, 0,
					  NULL, size, &used, &cmio_devices);

		vcam->deviceID = 0;

		NSString *OBSVirtualCamUUID = [[NSBundle
			bundleWithIdentifier:@"com.obsproject.mac-virtualcam"]
			objectForInfoDictionaryKey:@"OBSCameraDeviceUUID"];
		for (size_t i = 0; i < num_devices; i++) {
			CMIOObjectID cmio_device = cmio_devices[i];
			address.mSelector = kCMIODevicePropertyDeviceUID;

			UInt32 device_name_size;
			CMIOObjectGetPropertyDataSize(cmio_device, &address, 0,
						      NULL, &device_name_size);

			CFStringRef uid;
			CMIOObjectGetPropertyData(cmio_device, &address, 0,
						  NULL, device_name_size, &used,
						  &uid);

			const char *uid_string = CFStringGetCStringPtr(
				uid, kCFStringEncodingUTF8);
			if (uid_string &&
			    strcmp(uid_string, OBSVirtualCamUUID.UTF8String) ==
				    0) {
				vcam->deviceID = cmio_device;
				CFRelease(uid);
				break;
			} else {
				CFRelease(uid);
			}
		}

		if (!vcam->deviceID) {
			obs_output_set_last_error(
				vcam->output,
				obs_module_text(
					"Error.SystemExtension.CameraUnavailable"));
			return false;
		}

		address.mSelector = kCMIODevicePropertyStreams;
		CMIOObjectGetPropertyDataSize(vcam->deviceID, &address, 0, NULL,
					      &size);
		CMIOStreamID stream_ids[(size / sizeof(CMIOStreamID))];

		CMIOObjectGetPropertyData(vcam->deviceID, &address, 0, NULL,
					  size, &used, &stream_ids);

		vcam->streamID = stream_ids[1];

		CMIOStreamCopyBufferQueue(
			vcam->streamID, [](CMIOStreamID, void *, void *) {},
			NULL, &vcam->queue);
		CMVideoFormatDescriptionCreate(kCFAllocatorDefault,
					       video_format,
					       vcam->videoInfo.output_width,
					       vcam->videoInfo.output_height,
					       NULL, &vcam->formatDescription);

		OSStatus result =
			CMIODeviceStartStream(vcam->deviceID, vcam->streamID);

		if (result != noErr) {
			obs_output_set_last_error(
				vcam->output,
				obs_module_text(
					"Error.SystemExtension.CameraNotStarted"));
			return false;
		}
	} else {
		[vcam->machServer run];
	}

	if (!obs_output_begin_data_capture(vcam->output, 0)) {
		return false;
	}

	return true;
}

static void virtualcam_output_stop(void *data, uint64_t ts)
{
	UNUSED_PARAMETER(ts);

	struct virtualcam_data *vcam = (struct virtualcam_data *)data;

	obs_output_end_data_capture(vcam->output);
	if (cmio_extension_supported()) {
		CMIODeviceStopStream(vcam->deviceID, vcam->streamID);
		CFRelease(vcam->formatDescription);
		CVPixelBufferPoolRelease(vcam->pool);
	} else {
		[vcam->machServer stop];
	}
}

static void virtualcam_output_raw_video(void *data, struct video_data *frame)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;

	CVPixelBufferRef frameRef = nil;
	CVReturn status = CVPixelBufferPoolCreatePixelBuffer(
		kCFAllocatorDefault, vcam->pool, &frameRef);

	if (status != kCVReturnSuccess) {
		blog(LOG_ERROR, "unable to allocate pixel buffer (error %d)",
		     status);
		return;
	}

	// Copy all planes into pixel buffer
	size_t planeCount = CVPixelBufferGetPlaneCount(frameRef);
	CVPixelBufferLockBaseAddress(frameRef, 0);

	if (planeCount == 0) {
		uint8_t *src = frame->data[0];
		uint8_t *dst = (uint8_t *)CVPixelBufferGetBaseAddress(frameRef);

		size_t destBytesPerRow = CVPixelBufferGetBytesPerRow(frameRef);
		size_t srcBytesPerRow = frame->linesize[0];
		size_t height = CVPixelBufferGetHeight(frameRef);

		// Sometimes CVPixelBufferCreate will create a pixel buffer that's a different
		// size than necessary to hold the frame (probably for some optimization reason).
		// If that is the case this will do a row-by-row copy into the buffer.
		if (destBytesPerRow == srcBytesPerRow) {
			memcpy(dst, src, destBytesPerRow * height);
		} else {
			for (int line = 0; (size_t)line < height; line++) {
				memcpy(dst, src, srcBytesPerRow);
				src += srcBytesPerRow;
				dst += destBytesPerRow;
			}
		}
	} else {
		for (size_t plane = 0; plane < planeCount; plane++) {
			uint8_t *src = frame->data[plane];

			if (!src) {
				blog(LOG_WARNING,
				     "Video data from OBS contains less planes than CVPixelBuffer");
				break;
			}

			uint8_t *dst =
				(uint8_t *)CVPixelBufferGetBaseAddressOfPlane(
					frameRef, plane);

			size_t destBytesPerRow =
				CVPixelBufferGetBytesPerRowOfPlane(frameRef,
								   plane);
			size_t srcBytesPerRow = frame->linesize[plane];
			size_t height =
				CVPixelBufferGetHeightOfPlane(frameRef, plane);

			if (destBytesPerRow == srcBytesPerRow) {
				memcpy(dst, src, destBytesPerRow * height);
			} else {
				for (int line = 0; (size_t)line < height;
				     line++) {
					memcpy(dst, src, srcBytesPerRow);
					src += srcBytesPerRow;
					dst += destBytesPerRow;
				}
			}
		}
	}

	CVPixelBufferUnlockBaseAddress(frameRef, 0);

	if (cmio_extension_supported()) {
		CMSampleBufferRef sampleBuffer;
		CMSampleTimingInfo timingInfo{
			.presentationTimeStamp =
				CMTimeMake(frame->timestamp, NSEC_PER_SEC)};

		CMSampleBufferCreateForImageBuffer(kCFAllocatorDefault,
						   frameRef, true, NULL, NULL,
						   vcam->formatDescription,
						   &timingInfo, &sampleBuffer);
		CMSimpleQueueEnqueue(vcam->queue, sampleBuffer);
	} else {
		// Share pixel buffer with clients
		[vcam->machServer sendPixelBuffer:frameRef
					timestamp:frame->timestamp
				     fpsNumerator:vcam->videoInfo.fps_num
				   fpsDenominator:vcam->videoInfo.fps_den];
	}

	CVPixelBufferRelease(frameRef);
}

struct obs_output_info virtualcam_output_info = {
	.id = "virtualcam_output",
	.flags = OBS_OUTPUT_VIDEO,
	.get_name = virtualcam_output_get_name,
	.create = virtualcam_output_create,
	.destroy = virtualcam_output_destroy,
	.start = virtualcam_output_start,
	.stop = virtualcam_output_stop,
	.raw_video = virtualcam_output_raw_video,
};

bool obs_module_load(void)
{
	obs_register_output(&virtualcam_output_info);

	obs_data_t *obs_settings = obs_data_create();
	obs_data_set_bool(obs_settings, "vcamEnabled", true);
	obs_apply_private_data(obs_settings);
	obs_data_release(obs_settings);

	return true;
}
