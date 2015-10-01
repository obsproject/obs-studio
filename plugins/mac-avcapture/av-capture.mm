#import <AVFoundation/AVFoundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <obs-module.h>
#include <media-io/video-io.h>

#include <memory>

using namespace std;

#define TEXT_AVCAPTURE  obs_module_text("AVCapture")
#define TEXT_DEVICE     obs_module_text("Device")
#define TEXT_USE_PRESET obs_module_text("UsePreset")
#define TEXT_PRESET     obs_module_text("Preset")

#define MILLI_TIMESCALE 1000
#define MICRO_TIMESCALE (MILLI_TIMESCALE * 1000)
#define NANO_TIMESCALE  (MICRO_TIMESCALE * 1000)

#define AV_FOURCC_STR(code) \
	(char[5]) { \
		static_cast<char>((code >> 24) & 0xFF), \
		static_cast<char>((code >> 16) & 0xFF), \
		static_cast<char>((code >>  8) & 0xFF), \
		static_cast<char>( code        & 0xFF), \
		                                    0  \
	}

struct av_capture;

#define AVLOG(level, format, ...) \
	blog(level, "%s: " format, \
			obs_source_get_name(capture->source), ##__VA_ARGS__)

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

namespace {

static auto remove_observer = [](id observer)
{
	[[NSNotificationCenter defaultCenter] removeObserver:observer];
};

struct observer_handle :
	unique_ptr<remove_pointer<id>::type, decltype(remove_observer)> {
	
	using base = unique_ptr<remove_pointer<id>::type,
				decltype(remove_observer)>;

	explicit observer_handle(id observer = nullptr)
		: base(observer, remove_observer)
	{}
};

}

struct av_capture {
	OBSAVCaptureDelegate *delegate;
	dispatch_queue_t queue;
	bool has_clock;

	AVCaptureVideoDataOutput *out;
	AVCaptureDevice          *device;
	AVCaptureDeviceInput     *device_input;
	AVCaptureSession         *session;
	
	NSString *uid;
	observer_handle connect_observer;
	observer_handle disconnect_observer;

	FourCharCode fourcc;
	video_format video_format;
	video_colorspace colorspace;
	video_range_type video_range;

	obs_source_t *source;

	obs_source_frame frame;
};

static inline video_format format_from_subtype(FourCharCode subtype)
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

static inline video_colorspace get_colorspace(CMFormatDescriptionRef desc)
{
	CFPropertyListRef matrix = CMFormatDescriptionGetExtension(desc,
			kCMFormatDescriptionExtension_YCbCrMatrix);

	if (!matrix)
		return VIDEO_CS_DEFAULT;

	if (CFStringCompare(static_cast<CFStringRef>(matrix),
				kCVImageBufferYCbCrMatrix_ITU_R_709_2, 0)
			== kCFCompareEqualTo)
		return VIDEO_CS_709;

	return VIDEO_CS_601;
}

static inline bool update_colorspace(av_capture *capture,
		obs_source_frame *frame, CMFormatDescriptionRef desc,
		bool full_range)
{
	video_colorspace colorspace = get_colorspace(desc);
	video_range_type range      = full_range ?
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

static inline bool update_frame(av_capture *capture,
		obs_source_frame *frame, CMSampleBufferRef sample_buffer)
{
	CMFormatDescriptionRef desc =
		CMSampleBufferGetFormatDescription(sample_buffer);

	FourCharCode    fourcc = CMFormatDescriptionGetMediaSubType(desc);
	video_format    format = format_from_subtype(fourcc);
	CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(desc);

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
		frame->data[0]     = static_cast<uint8_t*>(
				CVPixelBufferGetBaseAddress(img));
		return true;
	}

	size_t count = CVPixelBufferGetPlaneCount(img);
	for (size_t i = 0; i < count; i++) {
		frame->linesize[i] = CVPixelBufferGetBytesPerRowOfPlane(img, i);
		frame->data[i]     = static_cast<uint8_t*>(
				CVPixelBufferGetBaseAddressOfPlane(img, i));
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

	obs_source_frame *frame = &capture->frame;

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

static void av_capture_enable_buffering(av_capture *capture, bool enabled)
{
	obs_source_t *source = capture->source;
	uint32_t flags = obs_source_get_flags(source);
	if (enabled)
		flags &= ~OBS_SOURCE_FLAG_UNBUFFERED;
	else
		flags |= OBS_SOURCE_FLAG_UNBUFFERED;
	obs_source_set_flags(source, flags);
}

static const char *av_capture_getname(void*)
{
	return TEXT_AVCAPTURE;
}

static void remove_device(av_capture *capture)
{
	[capture->session stopRunning];
	[capture->session removeInput:capture->device_input];

	capture->device_input = nullptr;
	capture->device = nullptr;

	obs_source_output_video(capture->source, nullptr);
}

static void av_capture_destroy(void *data)
{
	auto capture = static_cast<av_capture*>(data);

	delete capture;
}

static NSString *get_string(obs_data_t *data, char const *name)
{
	return @(obs_data_get_string(data, name));
}

static bool init_session(av_capture *capture)
{
	auto session = [[AVCaptureSession alloc] init];
	if (!session) {
		AVLOG(LOG_ERROR, "Could not create AVCaptureSession");
		return false;
	}

	auto delegate = [[OBSAVCaptureDelegate alloc] init];
	if (!delegate) {
		AVLOG(LOG_ERROR, "Could not create OBSAVCaptureDelegate");
		return false;
	}

	delegate->capture = capture;

	auto out = [[AVCaptureVideoDataOutput alloc] init];
	if (!out) {
		AVLOG(LOG_ERROR, "Could not create AVCaptureVideoDataOutput");
		return false;
	}

	auto queue = dispatch_queue_create(NULL, NULL);
	if (!queue) {
		AVLOG(LOG_ERROR, "Could not create dispatch queue");
		return false;
	}

	capture->session = session;
	capture->delegate = delegate;
	capture->out = out;
	capture->queue = queue;

	[capture->session addOutput:capture->out];
	[capture->out
		setSampleBufferDelegate:capture->delegate
				  queue:capture->queue];

	return true;
}

static bool init_format(av_capture *capture, AVCaptureDevice *dev);

static bool init_device_input(av_capture *capture, AVCaptureDevice *dev)
{
	NSError *err = nil;
	AVCaptureDeviceInput *device_input = [AVCaptureDeviceInput
		deviceInputWithDevice:dev error:&err];
	if (!device_input) {
		AVLOG(LOG_ERROR, "Error while initializing device input: %s",
				err.localizedFailureReason.UTF8String);
		return false;
	}

	[capture->session addInput:device_input];

	if (!init_format(capture, dev)) {
		[capture->session removeInput:device_input];
		return false;
	}

	capture->device_input = device_input;

	return true;
}

static uint32_t uint_from_dict(NSDictionary *dict, CFStringRef key)
{
	return ((NSNumber*)dict[(__bridge NSString*)key]).unsignedIntValue;
}

static bool init_format(av_capture *capture, AVCaptureDevice *dev)
{
	AVCaptureDeviceFormat *format = dev.activeFormat;

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

static void capture_device(av_capture *capture, AVCaptureDevice *dev,
		obs_data_t *settings)
{
	const char *name = dev.localizedName.UTF8String;
	obs_data_set_string(settings, "device_name", name);
	obs_data_set_string(settings, "device", dev.uniqueID.UTF8String);
	AVLOG(LOG_INFO, "Selected device '%s'", name);

	if (obs_data_get_bool(settings, "use_preset")) {
		NSString *preset = get_string(settings, "preset");
		if (![dev supportsAVCaptureSessionPreset:preset]) {
			AVLOG(LOG_WARNING, "Preset %s not available",
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

	if (!init_device_input(capture, dev))
		return;

	AVCaptureInputPort *port = capture->device_input.ports[0];
	capture->has_clock = [port respondsToSelector:@selector(clock)];

	capture->device = dev;
	[capture->session startRunning];
	return;
}

static inline void handle_disconnect_capture(av_capture *capture,
		AVCaptureDevice *dev)
{
	if (![dev.uniqueID isEqualTo:capture->uid])
		return;

	if (!capture->device) {
		AVLOG(LOG_INFO, "Received disconnect for inactive device '%s'",
				capture->uid.UTF8String);
		return;
	}

	AVLOG(LOG_WARNING, "Device with unique ID '%s' disconnected",
			dev.uniqueID.UTF8String);

	remove_device(capture);
}

static inline void handle_disconnect(av_capture *capture,
		AVCaptureDevice *dev)
{
	if (!dev)
		return;

	handle_disconnect_capture(capture, dev);
	obs_source_update_properties(capture->source);
}

static inline void handle_connect_capture(av_capture *capture,
		AVCaptureDevice *dev, obs_data_t *settings)
{
	if (![dev.uniqueID isEqualTo:capture->uid])
		return;

	if (capture->device) {
		AVLOG(LOG_ERROR, "Received connect for in-use device '%s'",
				capture->uid.UTF8String);
		return;
	}

	AVLOG(LOG_INFO, "Device with unique ID '%s' connected, "
			"resuming capture", dev.uniqueID.UTF8String);

	capture_device(capture, dev, settings);
}

static inline void handle_connect(av_capture *capture,
		AVCaptureDevice *dev, obs_data_t *settings)
{
	if (!dev)
		return;

	handle_connect_capture(capture, dev, settings);
	obs_source_update_properties(capture->source);
}

static bool av_capture_init(av_capture *capture, obs_data_t *settings)
{
	if (!init_session(capture))
		return false;

	capture->uid = get_string(settings, "device");

	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	capture->disconnect_observer.reset([nc
		addObserverForName:AVCaptureDeviceWasDisconnectedNotification
			    object:nil
			     queue:[NSOperationQueue mainQueue]
			usingBlock:^(NSNotification *note)
			{
				handle_disconnect(capture, note.object);
			}
	]);

	capture->connect_observer.reset([nc
		addObserverForName:AVCaptureDeviceWasConnectedNotification
			    object:nil
			     queue:[NSOperationQueue mainQueue]
			usingBlock:^(NSNotification *note)
			{
				handle_connect(capture, note.object, settings);
			}
	]);

	AVCaptureDevice *dev =
		[AVCaptureDevice deviceWithUniqueID:capture->uid];

	if (!dev) {
		if (capture->uid.length < 1)
			AVLOG(LOG_INFO, "No device selected");
		else
			AVLOG(LOG_WARNING, "Could not initialize device " \
					"with unique ID '%s'",
					capture->uid.UTF8String);
		return true;
	}

	capture_device(capture, dev, settings);

	return true;
}

static void *av_capture_create(obs_data_t *settings, obs_source_t *source)
{
	unique_ptr<av_capture> capture;

	try {
		capture.reset(new av_capture());
	} catch (...) {
		return capture.release();
	}

	capture->source = source;

	if (!av_capture_init(capture.get(), settings)) {
		AVLOG(LOG_ERROR, "av_capture_init failed");
		return nullptr;
	}

	av_capture_enable_buffering(capture.get(),
			obs_data_get_bool(settings, "buffering"));

	return capture.release();
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


static void av_capture_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "uid", "");
	obs_data_set_default_bool(settings, "use_preset", true);
	obs_data_set_default_string(settings, "preset",
			AVCaptureSessionPreset1280x720.UTF8String);
}

static bool update_device_list(obs_property_t *list,
		NSString *uid, NSString *name, bool disconnected)
{
	bool dev_found     = false;
	bool list_modified = false;

	size_t size = obs_property_list_item_count(list);
	for (size_t i = 0; i < size;) {
		const char *uid_ = obs_property_list_item_string(list, i);
		bool found       = [uid isEqualToString:@(uid_ ? uid_ : "")];
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

static void fill_presets(AVCaptureDevice *dev, obs_property_t *list,
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
		obs_property_t *list, obs_data_t *settings)
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

static bool autoselect_preset(AVCaptureDevice *dev, obs_data_t *settings)
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

static bool properties_device_changed(obs_properties_t *props, obs_property_t *p,
		obs_data_t *settings)
{
	NSString *uid = get_string(settings, "device");
	AVCaptureDevice *dev = [AVCaptureDevice deviceWithUniqueID:uid];

	NSString *name = get_string(settings, "device_name");
	bool dev_list_updated = update_device_list(p, uid, name,
			!dev && uid.length);

	p = obs_properties_get(props, "preset");
	bool preset_list_changed = check_preset(dev, p, settings);
	bool autoselect_changed  = autoselect_preset(dev, settings);

	return preset_list_changed || autoselect_changed || dev_list_updated;
}

static bool properties_preset_changed(obs_properties_t *, obs_property_t *p,
		obs_data_t *settings)
{
	NSString *uid = get_string(settings, "device");
	AVCaptureDevice *dev = [AVCaptureDevice deviceWithUniqueID:uid];

	bool preset_list_changed = check_preset(dev, p, settings);
	bool autoselect_changed  = autoselect_preset(dev, settings);

	return preset_list_changed || autoselect_changed;
}

static obs_properties_t *av_capture_properties(void*)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *dev_list = obs_properties_add_list(props, "device",
			TEXT_DEVICE, OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(dev_list, "", "");
	for (AVCaptureDevice *dev in [AVCaptureDevice
			devicesWithMediaType:AVMediaTypeVideo]) {
		obs_property_list_add_string(dev_list,
				dev.localizedName.UTF8String,
				dev.uniqueID.UTF8String);
	}

	obs_property_set_modified_callback(dev_list,
			properties_device_changed);

	obs_property_t *use_preset = obs_properties_add_bool(props,
			"use_preset", TEXT_USE_PRESET);
	// TODO: implement manual configuration
	obs_property_set_enabled(use_preset, false);

	obs_property_t *preset_list = obs_properties_add_list(props, "preset",
			TEXT_PRESET, OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_STRING);
	for (NSString *preset in presets())
		obs_property_list_add_string(preset_list,
				preset_names(preset).UTF8String,
				preset.UTF8String);

	obs_property_set_modified_callback(preset_list,
			properties_preset_changed);

	obs_properties_add_bool(props, "buffering",
			obs_module_text("Buffering"));

	return props;
}

static void switch_device(av_capture *capture, NSString *uid,
		obs_data_t *settings)
{
	if (!uid)
		return;

	if (capture->device)
		remove_device(capture);

	capture->uid = uid;

	if (!uid.length) {
		AVLOG(LOG_INFO, "No device selected, stopping capture");
		return;
	}

	AVCaptureDevice *dev = [AVCaptureDevice deviceWithUniqueID:uid];
	if (!dev) {
		AVLOG(LOG_WARNING, "Device with unique id '%s' not found",
				uid.UTF8String);
		return;
	}

	capture_device(capture, dev, settings);
}

static void av_capture_update(void *data, obs_data_t *settings)
{
	auto capture = static_cast<av_capture*>(data);

	NSString *uid = get_string(settings, "device");

	if (!capture->device || ![capture->device.uniqueID isEqualToString:uid])
		return switch_device(capture, uid, settings);

	NSString *preset = get_string(settings, "preset");
	if (![capture->device supportsAVCaptureSessionPreset:preset]) {
		AVLOG(LOG_WARNING, "Preset %s not available",
				preset.UTF8String);
		preset = select_preset(capture->device, preset);
	}

	capture->session.sessionPreset = preset;
	AVLOG(LOG_INFO, "Selected preset %s", preset.UTF8String);

	av_capture_enable_buffering(capture,
			obs_data_get_bool(settings, "buffering"));
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("mac-avcapture", "en-US")

bool obs_module_load(void)
{
	obs_source_info av_capture_info = {
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

	obs_register_source(&av_capture_info);
	return true;
}
