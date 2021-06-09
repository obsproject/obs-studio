#include <stdlib.h>
#include <obs-module.h>
#include <util/threading.h>
#include <pthread.h>

#import <CoreGraphics/CGDisplayStream.h>
#import <Cocoa/Cocoa.h>
#import <mach/mach_time.h>

#include "window-utils.h"

enum crop_mode {
	CROP_NONE,
	CROP_MANUAL,
	CROP_TO_WINDOW,
	CROP_TO_WINDOW_AND_MANUAL,
	CROP_INVALID
};

static inline bool requires_window(enum crop_mode mode)
{
	return mode == CROP_TO_WINDOW || mode == CROP_TO_WINDOW_AND_MANUAL;
}

struct display_capture {
	obs_source_t *source;

	NSScreen *screen;
	unsigned display;
	NSRect screen_rect;
	bool hide_cursor;

	enum crop_mode crop;
	CGRect crop_rect;

	struct cocoa_window window;
	CGRect window_rect;
	bool on_screen;

	os_event_t *disp_finished;
	CGDisplayStreamRef disp;

	mach_timebase_info_data_t timebase_info;

	CGRect capture_rect;

	double frame_interval;
};

static inline bool crop_mode_valid(enum crop_mode mode)
{
	return CROP_NONE <= mode && mode < CROP_INVALID;
}

static inline bool is_valid_capture_rect(CGRect *rect)
{
	return !(rect->size.width <= 0.0f || rect->size.height <= 0.0f);
}

static void destroy_display_stream(struct display_capture *dc)
{
	if (dc->disp) {
		CGDisplayStreamStop(dc->disp);
		os_event_wait(dc->disp_finished);

		CFRelease(dc->disp);
		dc->disp = NULL;
	}

	if (dc->screen) {
		[dc->screen release];
		dc->screen = nil;
	}

	os_event_destroy(dc->disp_finished);
}

static void display_capture_destroy(void *data)
{
	struct display_capture *dc = data;

	if (!dc)
		return;

	obs_enter_graphics();

	destroy_display_stream(dc);

	obs_leave_graphics();

	destroy_window(&dc->window);

	bfree(dc);
}

static void update_capture_rect(struct display_capture *dc);

static inline void update_window_params(struct display_capture *dc)
{
	if (!requires_window(dc->crop))
		return;

	NSArray *arr = (NSArray *)CGWindowListCopyWindowInfo(
		kCGWindowListOptionIncludingWindow, dc->window.window_id);

	if (arr.count) {
		NSDictionary *dict = arr[0];
		NSDictionary *ref = dict[(NSString *)kCGWindowBounds];
		CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)ref,
						       &dc->window_rect);
		dc->on_screen = dict[(NSString *)kCGWindowIsOnscreen] != nil;
		dc->window_rect =
			[dc->screen convertRectToBacking:dc->window_rect];

	} else {
		if (find_window(&dc->window, NULL, false))
			update_window_params(dc);
		else
			dc->on_screen = false;
	}

	[arr release];
}

static bool init_display_stream(struct display_capture *dc);

static inline void display_stream_update(struct display_capture *dc,
					 CGDisplayStreamFrameStatus status,
					 uint64_t display_time,
					 IOSurfaceRef frame_surface,
					 CGDisplayStreamUpdateRef update_ref)
{
	if (status == kCGDisplayStreamFrameStatusStopped) {
		os_event_signal(dc->disp_finished);
		return;
	}

	if (requires_window(dc->crop)) {
		update_window_params(dc);
	}
	update_capture_rect(dc);

	if (frame_surface) {
		// Convert mach timestamp to nanosecond timestamp
		uint64_t ts =
			(uint64_t)((display_time * dc->timebase_info.numer) /
				   (double)dc->timebase_info.denom);
		uint8_t *address = IOSurfaceGetBaseAddress(frame_surface) +
				   (int)(dc->screen_rect.size.width *
					 dc->capture_rect.origin.y * 4) +
				   (int)(dc->capture_rect.origin.x * 4);

		struct obs_source_frame frame = {
			.format = VIDEO_FORMAT_BGRA,
			.width = dc->capture_rect.size.width,
			.height = dc->capture_rect.size.height,
			.data[0] = address,
			.linesize[0] = (size_t)(dc->screen_rect.size.width * 4),
			.timestamp = ts,
		};

		IOSurfaceLock(frame_surface, kIOSurfaceLockReadOnly, NULL);
		obs_source_output_video(dc->source, &frame);
		IOSurfaceUnlock(frame_surface, kIOSurfaceLockReadOnly, NULL);
	}

	size_t dropped_frames = CGDisplayStreamUpdateGetDropCount(update_ref);
	if (dropped_frames > 0)
		blog(LOG_INFO, "%s: Dropped %zu frames",
		     obs_source_get_name(dc->source), dropped_frames);
}

static bool init_display_stream(struct display_capture *dc)
{
	if (dc->display >= [NSScreen screens].count)
		return false;

	dc->screen = [[NSScreen screens][dc->display] retain];
	dc->screen_rect = [dc->screen convertRectToBacking:dc->screen.frame];

	NSNumber *screen_num = dc->screen.deviceDescription[@"NSScreenNumber"];
	CGDirectDisplayID disp_id = [screen_num unsignedIntValue];

	CGRect screen_capture_rect = dc->screen_rect;
	screen_capture_rect.origin = CGPointZero;
	NSDictionary *rect_dict = CFBridgingRelease(
		CGRectCreateDictionaryRepresentation(screen_capture_rect));

	NSNumber *frame_interval =
		[NSNumber numberWithDouble:dc->frame_interval];
	CFBooleanRef show_cursor_cf = dc->hide_cursor ? kCFBooleanFalse
						      : kCFBooleanTrue;

	NSDictionary *dict = @{
		(__bridge NSString *)kCGDisplayStreamSourceRect: rect_dict,
		(__bridge NSString *)
		kCGDisplayStreamMinimumFrameTime: frame_interval,
		(__bridge NSString *)kCGDisplayStreamQueueDepth: @5,
		(__bridge NSString *)
		kCGDisplayStreamShowCursor: (id)show_cursor_cf,
	};

	os_event_init(&dc->disp_finished, OS_EVENT_TYPE_MANUAL);

	dispatch_queue_attr_t dispatchQueueAttr =
		dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL,
							QOS_CLASS_UTILITY, 0);
	dispatch_queue_t displayCaptureDispatchQueue =
		dispatch_queue_create(DISPATCH_QUEUE_SERIAL, dispatchQueueAttr);

	dc->disp = CGDisplayStreamCreateWithDispatchQueue(
		disp_id, dc->screen_rect.size.width,
		dc->screen_rect.size.height, 'BGRA',
		(__bridge CFDictionaryRef)dict, displayCaptureDispatchQueue,
		^(CGDisplayStreamFrameStatus status, uint64_t displayTime,
		  IOSurfaceRef frameSurface,
		  CGDisplayStreamUpdateRef updateRef) {
			display_stream_update(dc, status, displayTime,
					      frameSurface, updateRef);
		});

	CGError err = CGDisplayStreamStart(dc->disp);
	return !err;
}

void load_crop(struct display_capture *dc, obs_data_t *settings);

static void *display_capture_create(obs_data_t *settings, obs_source_t *source)
{
	struct display_capture *dc = bzalloc(sizeof(struct display_capture));

	dc->source = source;
	dc->hide_cursor = !obs_data_get_bool(settings, "show_cursor");
	dc->display = obs_data_get_int(settings, "display");

	struct obs_video_info ovi = {};
	if (obs_get_video_info(&ovi)) {
		dc->frame_interval = (double)ovi.fps_den / (double)ovi.fps_num;

		// Truncate value to 3 decimal places (ms accuracy)
		dc->frame_interval = floorf(dc->frame_interval * 1000) / 1000;
	}

	if (mach_timebase_info(&dc->timebase_info))
		goto fail;

	init_window(&dc->window, settings);
	load_crop(dc, settings);

	if (!init_display_stream(dc))
		goto fail;

	return dc;

fail:
	display_capture_destroy(dc);
	return NULL;
}

static void update_capture_rect(struct display_capture *dc)
{
	switch (dc->crop) {
	case CROP_NONE: {
		dc->capture_rect.origin = CGPointZero;
		dc->capture_rect.size = dc->screen.frame.size;
	} break;
	case CROP_MANUAL: {
		float crop_width =
			dc->crop_rect.origin.x + dc->crop_rect.size.width;
		float crop_height =
			dc->crop_rect.origin.y + dc->crop_rect.size.height;
		dc->capture_rect.origin = dc->crop_rect.origin;
		dc->capture_rect.size.width =
			fabs(dc->screen_rect.size.width - crop_width);
		dc->capture_rect.size.height =
			fabs(dc->screen_rect.size.height - crop_height);
	} break;
	case CROP_TO_WINDOW: {
		if (is_valid_capture_rect(&dc->window_rect))
			dc->capture_rect = dc->window_rect;
	} break;
	case CROP_TO_WINDOW_AND_MANUAL: {
		if (is_valid_capture_rect(&dc->window_rect)) {
			dc->capture_rect = dc->window_rect;
		} else {
			dc->capture_rect.origin = CGPointZero;
			dc->capture_rect.size = dc->screen.frame.size;
		}
		float crop_width =
			dc->crop_rect.origin.x + dc->crop_rect.size.width;
		float crop_height =
			dc->crop_rect.origin.y + dc->crop_rect.size.height;
		dc->capture_rect.origin.x += dc->crop_rect.origin.x;
		dc->capture_rect.origin.y += dc->crop_rect.origin.y;
		dc->capture_rect.size.width =
			fabs(dc->capture_rect.size.width - crop_width);
		dc->capture_rect.size.height =
			fabs(dc->capture_rect.size.height - crop_height);
	} break;
	case CROP_INVALID:
	default: {
	} break;
	}

	CGFloat right_edge =
		dc->capture_rect.origin.x + dc->capture_rect.size.width;
	CGFloat bottom_edge =
		dc->capture_rect.origin.y + dc->capture_rect.size.height;
	CGFloat right_diff = right_edge - dc->screen_rect.size.width;
	CGFloat bottom_diff = bottom_edge - dc->screen_rect.size.height;

	if (dc->capture_rect.origin.x < 0) {
		dc->capture_rect.size.width += dc->capture_rect.origin.x;
		dc->capture_rect.origin.x = 0;
	} else if (right_diff > 0) {
		dc->capture_rect.size.width -= right_diff;
	}

	if (dc->capture_rect.origin.y < 0) {
		dc->capture_rect.size.height += dc->capture_rect.origin.y;
		dc->capture_rect.origin.y = 0;
	} else if (bottom_diff > 0) {
		dc->capture_rect.size.height -= bottom_diff;
	}
}

static const char *display_capture_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("DisplayCapture");
}

static void display_capture_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "display", 0);
	obs_data_set_default_bool(settings, "show_cursor", true);
	obs_data_set_default_int(settings, "crop_mode", CROP_NONE);

	window_defaults(settings);
}

void load_crop_mode(enum crop_mode *mode, obs_data_t *settings)
{
	*mode = obs_data_get_int(settings, "crop_mode");
	if (!crop_mode_valid(*mode))
		*mode = CROP_NONE;
}

void load_crop(struct display_capture *dc, obs_data_t *settings)
{
	load_crop_mode(&dc->crop, settings);

#define CROP_VAR_NAME(var, mode) (mode "." #var)
#define LOAD_CROP_VAR(var, mode) \
	dc->crop_rect.var =      \
		obs_data_get_double(settings, CROP_VAR_NAME(var, mode));
	switch (dc->crop) {
	case CROP_MANUAL:
		LOAD_CROP_VAR(origin.x, "manual");
		LOAD_CROP_VAR(origin.y, "manual");
		LOAD_CROP_VAR(size.width, "manual");
		LOAD_CROP_VAR(size.height, "manual");
		break;

	case CROP_TO_WINDOW_AND_MANUAL:
		LOAD_CROP_VAR(origin.x, "window");
		LOAD_CROP_VAR(origin.y, "window");
		LOAD_CROP_VAR(size.width, "window");
		LOAD_CROP_VAR(size.height, "window");
		break;

	case CROP_NONE:
	case CROP_TO_WINDOW:
	case CROP_INVALID:
		break;
	}
#undef LOAD_CROP_VAR
}

static void display_capture_update(void *data, obs_data_t *settings)
{
	struct display_capture *dc = data;

	load_crop(dc, settings);

	unsigned display = obs_data_get_int(settings, "display");
	bool show_cursor = obs_data_get_bool(settings, "show_cursor");

	if ((dc->display != display) || (dc->hide_cursor == show_cursor)) {
		obs_enter_graphics();

		destroy_display_stream(dc);
		dc->display = display;
		dc->hide_cursor = !show_cursor;
		init_display_stream(dc);

		obs_leave_graphics();
	}
}

static bool switch_crop_mode(obs_properties_t *props, obs_property_t *p,
			     obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	enum crop_mode crop;
	load_crop_mode(&crop, settings);

	const char *name;
	bool visible;
#define LOAD_CROP_VAR(var, mode)         \
	name = CROP_VAR_NAME(var, mode); \
	obs_property_set_visible(obs_properties_get(props, name), visible);

	visible = crop == CROP_MANUAL;
	LOAD_CROP_VAR(origin.x, "manual");
	LOAD_CROP_VAR(origin.y, "manual");
	LOAD_CROP_VAR(size.width, "manual");
	LOAD_CROP_VAR(size.height, "manual");

	visible = crop == CROP_TO_WINDOW_AND_MANUAL;
	LOAD_CROP_VAR(origin.x, "window");
	LOAD_CROP_VAR(origin.y, "window");
	LOAD_CROP_VAR(size.width, "window");
	LOAD_CROP_VAR(size.height, "window");
#undef LOAD_CROP_VAR

	show_window_properties(props, visible || crop == CROP_TO_WINDOW);
	return true;
}

static const char *crop_names[] = {"CropMode.None", "CropMode.Manual",
				   "CropMode.ToWindow",
				   "CropMode.ToWindowAndManual"};

#ifndef COUNTOF
#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))
#endif
static obs_properties_t *display_capture_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_property_t *list = obs_properties_add_list(
		props, "display", obs_module_text("DisplayCapture.Display"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	[[NSScreen screens] enumerateObjectsUsingBlock:^(
				    NSScreen *_Nonnull screen, NSUInteger index,
				    BOOL *_Nonnull stop
				    __attribute__((unused))) {
		char dimension_buffer[4][12];
		char name_buffer[256];
		sprintf(dimension_buffer[0], "%u",
			(uint32_t)[screen frame].size.width);
		sprintf(dimension_buffer[1], "%u",
			(uint32_t)[screen frame].size.height);
		sprintf(dimension_buffer[2], "%d",
			(int32_t)[screen frame].origin.x);
		sprintf(dimension_buffer[3], "%d",
			(int32_t)[screen frame].origin.y);
		sprintf(name_buffer, "%.200s: %.12sx%.12s @ %.12s,%.12s",
			[[screen localizedName] UTF8String],
			dimension_buffer[0], dimension_buffer[1],
			dimension_buffer[2], dimension_buffer[3]);
		obs_property_list_add_int(list, name_buffer, index);
	}];

	obs_properties_add_bool(props, "show_cursor",
				obs_module_text("DisplayCapture.ShowCursor"));

	obs_property_t *crop = obs_properties_add_list(
		props, "crop_mode", obs_module_text("CropMode"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_set_modified_callback(crop, switch_crop_mode);

	for (unsigned i = 0; i < COUNTOF(crop_names); i++) {
		const char *name = obs_module_text(crop_names[i]);
		obs_property_list_add_int(crop, name, i);
	}

	add_window_properties(props);
	show_window_properties(props, false);

	obs_property_t *p;
	const char *name;
	float min;
#define LOAD_CROP_VAR(var, mode)                                               \
	name = CROP_VAR_NAME(var, mode);                                       \
	p = obs_properties_add_float(                                          \
		props, name, obs_module_text("Crop." #var), min, 4096.f, .5f); \
	obs_property_set_visible(p, false);

	min = 0.f;
	LOAD_CROP_VAR(origin.x, "manual");
	LOAD_CROP_VAR(origin.y, "manual");
	LOAD_CROP_VAR(size.width, "manual");
	LOAD_CROP_VAR(size.height, "manual");

	min = -4096.f;
	LOAD_CROP_VAR(origin.x, "window");
	LOAD_CROP_VAR(origin.y, "window");
	LOAD_CROP_VAR(size.width, "window");
	LOAD_CROP_VAR(size.height, "window");
#undef LOAD_CROP_VAR

	return props;
}

struct obs_source_info display_capture_info = {
	.id = "display_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.get_name = display_capture_getname,

	.create = display_capture_create,
	.destroy = display_capture_destroy,

	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC_VIDEO,
	.video_tick = NULL,

	.get_defaults = display_capture_defaults,
	.get_properties = display_capture_properties,
	.update = display_capture_update,
	.icon_type = OBS_ICON_TYPE_DESKTOP_CAPTURE,
};
