#include "screen-utils.h"

static inline void update_window_params(struct screen_capture *dc)
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
static inline void display_stream_update(struct screen_capture *dc,
					 CGDisplayStreamFrameStatus status,
					 uint64_t display_time,
					 IOSurfaceRef frame_surface,
					 CGDisplayStreamUpdateRef update_ref)
{
	UNUSED_PARAMETER(display_time);
	UNUSED_PARAMETER(update_ref);

	if (status == kCGDisplayStreamFrameStatusStopped) {
		os_event_signal(dc->disp_finished);
		return;
	}

	IOSurfaceRef prev_current = NULL;

	if (frame_surface && !pthread_mutex_lock(&dc->mutex)) {
		prev_current = dc->current;
		dc->current = frame_surface;
		CFRetain(dc->current);
		IOSurfaceIncrementUseCount(dc->current);

		update_window_params(dc);

		pthread_mutex_unlock(&dc->mutex);
	}

	if (prev_current) {
		IOSurfaceDecrementUseCount(prev_current);
		CFRelease(prev_current);
	}

	size_t dropped_frames = CGDisplayStreamUpdateGetDropCount(update_ref);
	if (dropped_frames > 0)
		blog(LOG_INFO, "%s: Dropped %zu frames",
		     obs_source_get_name(dc->source), dropped_frames);
}

bool init_screen_stream(struct screen_capture *dc)
{
	if (dc->display >= [NSScreen screens].count) {
		blog(LOG_INFO, "[display-capture], dc->display is %d > screen count, exiting", dc->display);
		return false;
	}

	blog(LOG_INFO, "[screen-capture] init_screen_stream");
	dc->screen = [[NSScreen screens][dc->display] retain];

	dc->frame = [dc->screen convertRectToBacking:dc->screen.frame];

	NSNumber *screen_num = dc->screen.deviceDescription[@"NSScreenNumber"];
	CGDirectDisplayID disp_id = (CGDirectDisplayID)screen_num.pointerValue;

	NSDictionary *rect_dict =
		CFBridgingRelease(CGRectCreateDictionaryRepresentation(
			CGRectMake(0, 0, dc->screen.frame.size.width,
				   dc->screen.frame.size.height)));

	CFBooleanRef show_cursor_cf = dc->hide_cursor ? kCFBooleanFalse
						      : kCFBooleanTrue;

	NSDictionary *dict = @{
		(__bridge NSString *)kCGDisplayStreamSourceRect: rect_dict,
		(__bridge NSString *)kCGDisplayStreamQueueDepth: @5,
		(__bridge NSString *)
		kCGDisplayStreamShowCursor: (id)show_cursor_cf,
	};

	os_event_init(&dc->disp_finished, OS_EVENT_TYPE_MANUAL);

	const CGSize *size = &dc->frame.size;
	// https://developer.apple.com/forums/thread/127374 -> popup permission
	dc->disp = CGDisplayStreamCreateWithDispatchQueue(
		disp_id, size->width, size->height, 'BGRA',
		(__bridge CFDictionaryRef)dict,
		dispatch_queue_create(NULL, NULL),
		^(CGDisplayStreamFrameStatus status, uint64_t displayTime,
		  IOSurfaceRef frameSurface,
		  CGDisplayStreamUpdateRef updateRef) {
			display_stream_update(dc, status, displayTime,
					      frameSurface, updateRef);
		});

	return !CGDisplayStreamStart(dc->disp);
}