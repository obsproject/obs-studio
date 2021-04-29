#ifndef WINDOW_UTILS_H_
#define WINDOW_UTILS_H_
#import <CoreGraphics/CGWindow.h>
#import <Cocoa/Cocoa.h>

#include <util/threading.h>
#include <obs-module.h>

struct cocoa_window {
	CGWindowID window_id;
	int owner_pid;

	pthread_mutex_t name_lock;
	NSString *owner_name;
	NSString *window_name;

	uint64_t next_search_time;
};

enum crop_mode {
	CROP_NONE,
	CROP_MANUAL,
	CROP_TO_WINDOW,
	CROP_TO_WINDOW_AND_MANUAL,
	CROP_INVALID
};

struct display_capture {
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

typedef struct cocoa_window *cocoa_window_t;

NSArray *enumerate_cocoa_windows(void);

bool find_window(cocoa_window_t cw, obs_data_t *settings, bool force);

void init_window(cocoa_window_t cw, obs_data_t *settings);

void destroy_window(cocoa_window_t cw);

void update_window(cocoa_window_t cw, obs_data_t *settings);

void window_defaults(obs_data_t *settings);

void add_window_properties(obs_properties_t *props);

void show_window_properties(obs_properties_t *props, bool show);

#endif
