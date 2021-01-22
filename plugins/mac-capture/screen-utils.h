#ifndef _SCREEN_UTILS_
#define _SCREEN_UTILS_
#include <obs-module.h>
#include <util/darray.h>
#include <util/threading.h>
#include <util/platform.h>

#import <CoreGraphics/CGWindow.h>
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CGDisplayStream.h>

#include <util/threading.h>
#include <obs-module.h>

#include "window-utils.h"

struct screen_capture {
	obs_source_t *source;

	gs_samplerstate_t *sampler;
	gs_effect_t *effect;
	gs_texture_t *tex;
	gs_vertbuffer_t *vertbuf;

	NSScreen *screen;
	unsigned display;
	NSRect frame;
	bool hide_cursor;

	enum crop_mode crop;
	CGRect crop_rect;

	struct cocoa_window window;
	CGRect window_rect;
	bool on_screen;
	bool hide_when_minimized;

	os_event_t *disp_finished;
	CGDisplayStreamRef disp;
	IOSurfaceRef current, prev;

	pthread_mutex_t mutex;
};

bool init_screen_stream(struct screen_capture *dc);

#endif