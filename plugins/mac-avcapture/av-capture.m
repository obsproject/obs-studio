#import <AVFoundation/AVFoundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <arpa/inet.h>

#include <obs-module.h>
#include <media-io/video-io.h>

#import "AVCaptureInputPort+PreMavericksCompat.h"

#define TEXT_AVCAPTURE  obs_module_text("AVCapture")
#define TEXT_DEVICE     obs_module_text("Device")
#define TEXT_USE_PRESET obs_module_text("UsePreset")
#define TEXT_PRESET     obs_module_text("Preset")

#define MILLI_TIMESCALE 1000
#define MICRO_TIMESCALE (MILLI_TIMESCALE * 1000)
#define NANO_TIMESCALE  (MICRO_TIMESCALE * 1000)

#define AV_FOURCC_STR(code) \
	(char[5]) { \
		(code >> 24) & 0xFF, \
		(code >> 16) & 0xFF, \
		(code >>  8) & 0xFF, \
		 code        & 0xFF, \
		                  0  \
	}

struct av_capture;

#define AVLOG(level, format, ...) \
	blog(level, "%s: " format, \
			obs_source_get_name(capture->source), ##__VA_ARGS__)

#define AVFREE(x) {[x release]; x = nil;}

@interface OBSAVCaptureDelegate :
	NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>
{
@public
	struct av_capture *capture;
}
- (void)captureOutput:(AVCaptureOutput *)out
        didDropSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection;
- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection;
@end

struct av_capture {
	AVCaptureSession         *session;
	AVCaptureDevice          *device;
	AVCaptureDeviceInput     *device_input;
	AVCaptureVideoDataOutput *out;
	
	OBSAVCaptureDelegate *delegate;
	dispatch_queue_t queue;
	bool has_clock;

	NSString *uid;
	id connect_observer;
	id disconnect_observer;

	FourCharCode fourcc;
	enum video_format video_format;
	enum video_colorspace colorspace;
	enum video_range_type video_range;

	obs_source_t source;

	struct obs_source_frame frame;
};

static inline enum video_format format_from_subtype(FourCharCode subtype)
{
	//TODO: uncomment VIDEO_FORMAT_NV12 and VIDEO_FORMAT_ARGB once libobs
	//      gains matching GPU conversions or a CPU fallback is implemented
	switch (subtype) {
	case kCVPixelFormatType_422YpCbCr8:
		return VIDEO_FORMAT_UYVY;
	case kCVPixelFormatType_422YpCbCr8_yuvs:
		return VIDEO_FORMAT_YUY2;
	/*case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
	case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
		return VIDEO_FORMAT_NV12;*/
	/*case kCVPixelFormatType_32ARGB:
		return VIDEO_FORMAT_ARGB;*/
	case kCVPixelFormatType_32BGRA:
		return VIDEO_FORMAT_BGRA;
	default:
		return VIDEO_FORMAT_NONE;
	}
}

static inline bool is_fullrange_yuv(FourCharCode pixel_format)
{
	switch (pixel_format) {
	case kCVPixelFormatType_420YpCbCr8PlanarFullRange:
	case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
	case kCVPixelFormatType_422YpCbCr8FullRange:
		return true;

	default:
		return false;
	}
}

static inline enum video_colorspace get_colorspace(CMFormatDescriptionRef desc)
{
	CFPropertyListRef matrix = CMFormatDescriptionGetExtension(desc,
			kCMFormatDescriptionExtension_YCbCrMatrix);

	if (!matrix)
		return VIDEO_CS_DEFAULT;

	if (CFStringCompare(matrix, kCVImageBufferYCbCrMatrix_ITU_R_709_2, 0)
			== kCFCompareEqualTo)
		return VIDEO_CS_709;

	return VIDEO_CS_601;
}

static inline bool update_colorspace(struct av_capture *capture,
		struct obs_source_frame *frame, CMFormatDescriptionRef desc,
		bool full_range)
{
	enum video_colorspace colorspace = get_colorspace(desc);
	enum video_range_type range      = full_range ?
		VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
	if (colorspace == capture->colorspace && range == capture->video_range)
		return true;

	frame->full_range = full_range;

	if (!video_format_get_parameters(colorspace, range,
				frame->color_matrix,
				frame->color_range_min,
				frame->color_range_max)) {
		AVLOG(LOG_ERROR, "Failed to get colorspace parameters for "
				 "colorspace %u range %u", colorspace, range);
		return false;
	}

	capture->colorspace  = colorspace;
	capture->video_range = range;

	return true;
}

static inline bool update_frame(struct av_capture *capture,
		struct obs_source_frame *frame, CMSampleBufferRef sample_buffer)
{
	CMFormatDescriptionRef desc =
		CMSampleBufferGetFormatDescription(sample_buffer);

	FourCharCode      fourcc = CMFormatDescriptionGetMediaSubType(desc);
	enum video_format format = format_from_subtype(fourcc);
	CMVideoDimensions   dims = CMVideoFormatDescriptionGetDimensions(desc);

	CVImageBufferRef     img = CMSampleBufferGetImageBuffer(sample_buffer);

	if (format == VIDEO_FORMAT_NONE) {
		if (capture->fourcc == fourcc)
			return false;

		capture->fourcc = fourcc;
		AVLOG(LOG_ERROR, "Unhandled fourcc: %s (0x%x) (%zu planes)",
				AV_FOURCC_STR(fourcc), fourcc,
				CVPixelBufferGetPlaneCount(img));
		return false;
	}

	if (frame->format != format)
		AVLOG(LOG_DEBUG, "Switching fourcc: "
				"'%s' (0x%x) -> '%s' (0x%x)",
				AV_FOURCC_STR(capture->fourcc), capture->fourcc,
				AV_FOURCC_STR(fourcc), fourcc);

	capture->fourcc = fourcc;
	frame->format   = format;
	frame->width    = dims.width;
	frame->height   = dims.height;

	if (format_is_yuv(format) && !update_colorspace(capture, frame, desc,
				is_fullrange_yuv(fourcc)))
		return false;

	CVPixelBufferLockBaseAddress(img, kCVPixelBufferLock_ReadOnly);

	if (!CVPixelBufferIsPlanar(img)) {
		frame->linesize[0] = CVPixelBufferGetBytesPerRow(img);
		frame->data[0]     = CVPixelBufferGetBaseAddress(img);
		return true;
	}

	size_t count = CVPixelBufferGetPlaneCount(img);
	for (size_t i = 0; i < count; i++) {
		frame->linesize[i] = CVPixelBufferGetBytesPerRowOfPlane(img, i);
		frame->data[i]     = CVPixelBufferGetBaseAddressOfPlane(img, i);
	}
	return true;
}

@implementation OBSAVCaptureDelegate
- (void)captureOutput:(AVCaptureOutput *)out
        didDropSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection
{
	UNUSED_PARAMETER(out);
	UNUSED_PARAMETER(sampleBuffer);
	UNUSED_PARAMETER(connection);
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection
{
	UNUSED_PARAMETER(captureOutput);
	UNUSED_PARAMETER(connection);

	CMItemCount count = CMSampleBufferGetNumSamples(sampleBuffer);
	if (count < 1 || !capture)
		return;

	struct obs_source_frame *frame = &capture->frame;

	CMTime target_pts =
		CMSampleBufferGetOutputPresentationTimeStamp(sampleBuffer);
	CMTime target_pts_nano = CMTimeConvertScale(target_pts, NANO_TIMESCALE,
			kCMTimeRoundingMethod_Default);
	frame->timestamp = target_pts_nano.value;

	if (!update_frame(capture, frame, sampleBuffer))
		return;

	obs_source_output_video(capture->source, frame);

	CVImageBufferRef img = CMSampleBufferGetImageBuffer(sampleBuffer);
	CVPixelBufferUnlockBaseAddress(img, kCVPixelBufferLock_ReadOnly);
}
@end

static const char *av_capture_getname(void)
{
	return TEXT_AVCAPTURE;
}

static void remove_device(struct av_capture *capture)
{
	[capture->session stopRunning];
	[capture->session removeInput:capture->device_input];

	AVFREE(capture->device_input);
	AVFREE(capture->device);
}

static void av_capture_destroy(void *data)
{
	struct av_capture *capture = data;
	if (!capture)
		return;

	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	[nc removeObserver:capture->connect_observer];
	[nc removeObserver:capture->disconnect_observer];

	remove_device(capture);
	AVFREE(capture->out);

	if (capture->queue)
		dispatch_release(capture->queue);

	AVFREE(capture->delegate);
	AVFREE(capture->session);

	AVFREE(capture->uid);

	bfree(capture);
}

static NSString *get_string(obs_data_t data, char const *name)
{
	return @(obs_data_get_string(data, name));
}

static bool init_session(struct av_capture *capture)
{
	capture->session = [[AVCaptureSession alloc] init];
	if (!capture->session) {
		AVLOG(LOG_ERROR, "Could not create AVCaptureSession");
		goto error;
	}

	capture->delegate = [[OBSAVCaptureDelegate alloc] init];
	if (!capture->delegate) {
		AVLOG(LOG_ERROR, "Could not create OBSAVCaptureDelegate");
		goto error;
	}

	capture->delegate->capture = capture;

	capture->out = [[AVCaptureVideoDataOutput alloc] init];
	if (!capture->out) {
		AVLOG(LOG_ERROR, "Could not create AVCaptureVideoDataOutput");
		goto error;
	}

	capture->queue = dispatch_queue_create(NULL, NULL);
	if (!capture->queue) {
		AVLOG(LOG_ERROR, "Could not create dispatch queue");
		goto error;
	}

	[capture->session addOutput:capture->out];
	[capture->out
		setSampleBufferDelegate:capture->delegate
				  queue:capture->queue];

	return true;

error:
	AVFREE(capture->session);
	AVFREE(capture->delegate);
	AVFREE(capture->out);
	return false;
}

static bool init_device_input(struct av_capture *capture)
{
	NSError *err = nil;
	capture->device_input = [[AVCaptureDeviceInput
		deviceInputWithDevice:capture->device error:&err] retain];
	if (!capture->device_input) {
		AVLOG(LOG_ERROR, "Error while initializing device input: %s",
				err.localizedFailureReason.UTF8String);
		return false;
	}

	[capture->session addInput:capture->device_input];

	return true;
}

static uint32_t uint_from_dict(NSDictionary *dict, CFStringRef key)
{
	return ((NSNumber*)dict[(__bridge NSString*)key]).unsignedIntValue;
}

static bool init_format(struct av_capture *capture)
{
	AVCaptureDeviceFormat *format = capture->device.activeFormat;

	CMMediaType mtype = CMFormatDescriptionGetMediaType(
			format.formatDescription);
	// TODO: support other media types
	if (mtype != kCMMediaType_Video) {
		AVLOG(LOG_ERROR, "CMMediaType '%s' is unsupported",
				AV_FOURCC_STR(mtype));
		return false;
	}

	capture->out.videoSettings = nil;
	FourCharCode subtype = uint_from_dict(capture->out.videoSettings,
					kCVPixelBufferPixelFormatTypeKey);
	if (format_from_subtype(subtype) != VIDEO_FORMAT_NONE) {
		AVLOG(LOG_DEBUG, "Using native fourcc '%s'",
				AV_FOURCC_STR(subtype));
		return true;
	}

	AVLOG(LOG_DEBUG, "Using fallback fourcc '%s' ('%s' 0x%08x unsupported)",
			AV_FOURCC_STR(kCVPixelFormatType_32BGRA),
			AV_FOURCC_STR(subtype), subtype);

	capture->out.videoSettings = @{
		(__bridge NSString*)kCVPixelBufferPixelFormatTypeKey:
			@(kCVPixelFormatType_32BGRA)
	};
	return true;
}

static NSArray *presets(void);
static NSString *preset_names(NSString *preset);

static NSString *select_preset(AVCaptureDevice *dev, NSString *cur_preset)
{
	NSString *new_preset = nil;
	bool found_previous_preset = false;
	for (NSString *preset in presets().reverseObjectEnumerator) {
		if (!found_previous_preset)
			found_previous_preset =
				[cur_preset isEqualToString:preset];

		if (![dev supportsAVCaptureSessionPreset:preset])
			continue;

		if (!new_preset || !found_previous_preset)
			new_preset = preset;
	}

	return new_preset;
}

static void capture_device(struct av_capture *capture, AVCaptureDevice *dev,
		obs_data_t settings)
{
	capture->device = dev;

	const char *name = capture->device.localizedName.UTF8String;
	obs_data_set_string(settings, "device_name", name);
	obs_data_set_string(settings, "device",
			capture->device.uniqueID.UTF8String);
	AVLOG(LOG_INFO, "Selected device '%s'", name);

	if (obs_data_get_bool(settings, "use_preset")) {
		NSString *preset = get_string(settings, "preset");
		if (![dev supportsAVCaptureSessionPreset:preset]) {
			AVLOG(LOG_ERROR, "Preset %s not available",
					preset_names(preset).UTF8String);
			preset = select_preset(dev, preset);
		}

		if (!preset) {
			AVLOG(LOG_ERROR, "Could not select a preset, "
					"initialization failed");
			return;
		}

		capture->session.sessionPreset = preset;
		AVLOG(LOG_INFO, "Using preset %s",
				preset_names(preset).UTF8String);
	}

	if (!init_device_input(capture))
		goto error_input;

	AVCaptureInputPort *port = capture->device_input.ports[0];
	capture->has_clock = [port respondsToSelector:@selector(clock)];

	if (!init_format(capture))
		goto error;

	[capture->session startRunning];
	return;

error:
	[capture->session removeInput:capture->device_input];
	AVFREE(capture->device_input);
error_input:
	AVFREE(capture->device);
}

static inline void handle_disconnect(struct av_capture* capture,
		AVCaptureDevice *dev)
{
	if (!dev)
		return;

	if (![dev.uniqueID isEqualTo:capture->uid])
		return;

	if (!capture->device) {
		AVLOG(LOG_ERROR, "Received disconnect for unused device '%s'",
				capture->uid.UTF8String);
		return;
	}

	AVLOG(LOG_ERROR, "Device with unique ID '%s' disconnected",
			dev.uniqueID.UTF8String);

	remove_device(capture);
}

static inline void handle_connect(struct av_capture *capture,
		AVCaptureDevice *dev, obs_data_t settings)
{
	if (!dev)
		return;

	if (![dev.uniqueID isEqualTo:capture->uid])
		return;

	if (capture->device) {
		AVLOG(LOG_ERROR, "Received connect for in-use device '%s'",
				capture->uid.UTF8String);
		return;
	}

	AVLOG(LOG_INFO, "Device with unique ID '%s' connected, "
			"resuming capture", dev.uniqueID.UTF8String);

	capture_device(capture, [dev retain], settings);
}

static void av_capture_init(struct av_capture *capture, obs_data_t settings)
{
	if (!init_session(capture))
		return;

	capture->uid = [get_string(settings, "device") retain];

	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	capture->disconnect_observer = [nc
		addObserverForName:AVCaptureDeviceWasDisconnectedNotification
			    object:nil
			     queue:[NSOperationQueue mainQueue]
			usingBlock:^(NSNotification *note)
			{
				handle_disconnect(capture, note.object);
			}
	];

	capture->connect_observer = [nc
		addObserverForName:AVCaptureDeviceWasConnectedNotification
			    object:nil
			     queue:[NSOperationQueue mainQueue]
			usingBlock:^(NSNotification *note)
			{
				handle_connect(capture, note.object, settings);
			}
	];

	AVCaptureDevice *dev =
		[[AVCaptureDevice deviceWithUniqueID:capture->uid] retain];

	if (!dev) {
		if (capture->uid.length < 1)
			AVLOG(LOG_ERROR, "No device selected");
		else
			AVLOG(LOG_ERROR, "Could not initialize device " \
					"with unique ID '%s'",
					capture->uid.UTF8String);
		return;
	}

	capture_device(capture, dev, settings);
}

static void *av_capture_create(obs_data_t settings, obs_source_t source)
{
	UNUSED_PARAMETER(source);

	struct av_capture *capture = bzalloc(sizeof(struct av_capture));
	capture->source = source;

	av_capture_init(capture, settings);

	if (!capture->session) {
		AVLOG(LOG_ERROR, "No valid session, returning NULL context");
		av_capture_destroy(capture);
		bfree(capture);
		capture = NULL;
	}

	return capture;
}

static NSArray *presets(void)
{
	return @[
		//AVCaptureSessionPresetiFrame1280x720,
		//AVCaptureSessionPresetiFrame960x540,
		AVCaptureSessionPreset1280x720,
		AVCaptureSessionPreset960x540,
		AVCaptureSessionPreset640x480,
		AVCaptureSessionPreset352x288,
		AVCaptureSessionPreset320x240,
		//AVCaptureSessionPresetHigh,
		//AVCaptureSessionPresetMedium,
		//AVCaptureSessionPresetLow,
		//AVCaptureSessionPresetPhoto,
	];
}

static NSString *preset_names(NSString *preset)
{
	NSDictionary *preset_names = @{
		AVCaptureSessionPresetLow:@"Low",
		AVCaptureSessionPresetMedium:@"Medium",
		AVCaptureSessionPresetHigh:@"High",
		AVCaptureSessionPreset320x240:@"320x240",
		AVCaptureSessionPreset352x288:@"352x288",
		AVCaptureSessionPreset640x480:@"640x480",
		AVCaptureSessionPreset960x540:@"960x540",
		AVCaptureSessionPreset1280x720:@"1280x720",
	};
	NSString *name = preset_names[preset];
	if (name)
		return name;
	return [NSString stringWithFormat:@"Unknown (%@)", preset];
}


static void av_capture_defaults(obs_data_t settings)
{
	obs_data_set_default_bool(settings, "use_preset", true);
	obs_data_set_default_string(settings, "preset",
			AVCaptureSessionPreset1280x720.UTF8String);
}

static bool update_device_list(obs_property_t list,
		NSString *uid, NSString *name, bool disconnected)
{
	bool dev_found     = false;
	bool list_modified = false;

	size_t size = obs_property_list_item_count(list);
	for (size_t i = 0; i < size;) {
		const char *uid_ = obs_property_list_item_string(list, i);
		bool found       = [uid isEqualToString:@(uid_)];
		bool disabled    = obs_property_list_item_disabled(list, i);
		if (!found && !disabled) {
			i += 1;
			continue;
		}

		if (disabled && !found) {
			list_modified = true;
			obs_property_list_item_remove(list, i);
			continue;
		}
		
		if (disabled != disconnected)
			list_modified = true;

		dev_found = true;
		obs_property_list_item_disable(list, i, disconnected);
		i += 1;
	}

	if (dev_found)
		return list_modified;

	size_t idx = obs_property_list_add_string(list, name.UTF8String,
			uid.UTF8String);
	obs_property_list_item_disable(list, idx, disconnected);

	return true;
}

static void fill_presets(AVCaptureDevice *dev, obs_property_t list,
		NSString *current_preset)
{
	obs_property_list_clear(list);

	bool preset_found = false;
	for (NSString *preset in presets()) {
		bool is_current = [preset isEqualToString:current_preset];
		bool supported  = dev &&
			[dev supportsAVCaptureSessionPreset:preset];

		if (is_current)
			preset_found = true;

		if (!supported && !is_current)
			continue;

		size_t idx = obs_property_list_add_string(list,
				preset_names(preset).UTF8String,
				preset.UTF8String);
		obs_property_list_item_disable(list, idx, !supported);
	}

	if (preset_found)
		return;

	size_t idx = obs_property_list_add_string(list,
			preset_names(current_preset).UTF8String,
			current_preset.UTF8String);
	obs_property_list_item_disable(list, idx, true);
}

static bool check_preset(AVCaptureDevice *dev,
		obs_property_t list, obs_data_t settings)
{
	NSString *current_preset = get_string(settings, "preset");

	size_t size = obs_property_list_item_count(list);
	NSMutableSet *listed = [NSMutableSet setWithCapacity:size];

	for (size_t i = 0; i < size; i++)
		[listed addObject:@(obs_property_list_item_string(list, i))];

	bool presets_changed = false;
	for (NSString *preset in presets()) {
		bool is_listed = [listed member:preset] != nil;
		bool supported = dev && 
			[dev supportsAVCaptureSessionPreset:preset];

		if (supported == is_listed)
			continue;

		presets_changed = true;
	}

	if (!presets_changed && [listed member:current_preset] != nil)
		return false;

	fill_presets(dev, list, current_preset);
	return true;
}

static bool autoselect_preset(AVCaptureDevice *dev, obs_data_t settings)
{
	NSString *preset = get_string(settings, "preset");
	if (!dev || [dev supportsAVCaptureSessionPreset:preset]) {
		if (obs_data_has_autoselect_value(settings, "preset")) {
			obs_data_unset_autoselect_value(settings, "preset");
			return true;
		}

	} else {
		preset = select_preset(dev, preset);
		const char *autoselect =
			obs_data_get_autoselect_string(settings, "preset");
		if (![preset isEqualToString:@(autoselect)]) {
			obs_data_set_autoselect_string(settings, "preset",
				preset.UTF8String);
			return true;
		}
	}

	return false;
}

static bool properties_device_changed(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	NSString *uid = get_string(settings, "device");
	AVCaptureDevice *dev = [AVCaptureDevice deviceWithUniqueID:uid];

	NSString *name = get_string(settings, "device_name");
	bool dev_list_updated = update_device_list(p, uid, name, !dev);

	p = obs_properties_get(props, "preset");
	bool preset_list_changed = check_preset(dev, p, settings);
	bool autoselect_changed  = autoselect_preset(dev, settings);

	return preset_list_changed || autoselect_changed || dev_list_updated;
}

static bool properties_preset_changed(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	UNUSED_PARAMETER(props);

	NSString *uid = get_string(settings, "device");
	AVCaptureDevice *dev = [AVCaptureDevice deviceWithUniqueID:uid];

	bool preset_list_changed = check_preset(dev, p, settings);
	bool autoselect_changed  = autoselect_preset(dev, settings);

	return preset_list_changed || autoselect_changed;
}

static obs_properties_t av_capture_properties(void)
{
	obs_properties_t props = obs_properties_create();

	obs_property_t dev_list = obs_properties_add_list(props, "device",
			TEXT_DEVICE, OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_STRING);
	for (AVCaptureDevice *dev in [AVCaptureDevice
			devicesWithMediaType:AVMediaTypeVideo]) {
		obs_property_list_add_string(dev_list,
				dev.localizedName.UTF8String,
				dev.uniqueID.UTF8String);
	}

	obs_property_set_modified_callback(dev_list,
			properties_device_changed);

	obs_property_t use_preset = obs_properties_add_bool(props,
			"use_preset", TEXT_USE_PRESET);
	// TODO: implement manual configuration
	obs_property_set_enabled(use_preset, false);

	obs_property_t preset_list = obs_properties_add_list(props, "preset",
			TEXT_PRESET, OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_STRING);
	for (NSString *preset in presets())
		obs_property_list_add_string(preset_list,
				preset_names(preset).UTF8String,
				preset.UTF8String);

	obs_property_set_modified_callback(preset_list,
			properties_preset_changed);

	return props;
}

static void switch_device(struct av_capture *capture, NSString *uid,
		obs_data_t settings)
{
	if (!uid)
		return;

	if (capture->device)
		remove_device(capture);

	AVFREE(capture->uid);
	capture->uid = [uid retain];

	AVCaptureDevice *dev = [AVCaptureDevice deviceWithUniqueID:uid];
	if (!dev) {
		AVLOG(LOG_ERROR, "Device with unique id '%s' not found",
				uid.UTF8String);
		return;
	}

	capture_device(capture, [dev retain], settings);
}

static void av_capture_update(void *data, obs_data_t settings)
{
	struct av_capture *capture = data;

	NSString *uid = get_string(settings, "device");

	if (!capture->device || ![capture->device.uniqueID isEqualToString:uid])
		return switch_device(capture, uid, settings);

	NSString *preset = get_string(settings, "preset");
	if (![capture->device supportsAVCaptureSessionPreset:preset]) {
		AVLOG(LOG_ERROR, "Preset %s not available", preset.UTF8String);
		preset = select_preset(capture->device, preset);
	}

	capture->session.sessionPreset = preset;
	AVLOG(LOG_INFO, "Selected preset %s", preset.UTF8String);
}

struct obs_source_info av_capture_info = {
	.id             = "av_capture_input",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_ASYNC_VIDEO,
	.get_name       = av_capture_getname,
	.create         = av_capture_create,
	.destroy        = av_capture_destroy,
	.get_defaults   = av_capture_defaults,
	.get_properties = av_capture_properties,
	.update         = av_capture_update,
};

