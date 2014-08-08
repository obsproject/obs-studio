#include <util/dstr.h>
#include "dc-capture.h"

struct monitor_capture {
	obs_source_t      source;

	int               monitor;
	bool              capture_cursor;
	bool              compatibility;

	struct dc_capture data;

	gs_effect_t       opaque_effect;
};

struct monitor_info {
	int               cur_id;
	int               desired_id;
	int               id;
	RECT              rect;
};

/* ------------------------------------------------------------------------- */

static inline void do_log(int level, const char *msg, ...)
{
	va_list args;
	struct dstr str = {0};

	va_start(args, msg);

	dstr_copy(&str, "[GDI monitor capture]: ");
	dstr_vcatf(&str, msg, args);
	blog(level, "%s", str.array);
	dstr_free(&str);

	va_end(args);
}

static BOOL CALLBACK enum_monitor(HMONITOR handle, HDC hdc, LPRECT rect,
		LPARAM param)
{
	struct monitor_info *monitor = (struct monitor_info *)param;

	if (monitor->cur_id == 0 || monitor->desired_id == monitor->cur_id) {
		monitor->rect = *rect;
		monitor->id   = monitor->cur_id;
	}

	UNUSED_PARAMETER(hdc);
	UNUSED_PARAMETER(handle);
	return (monitor->desired_id < monitor->cur_id++);
}

static void update_monitor(struct monitor_capture *capture,
		obs_data_t settings)
{
	struct monitor_info monitor = {0};
	uint32_t width, height;

	monitor.desired_id = (int)obs_data_get_int(settings, "monitor");
	EnumDisplayMonitors(NULL, NULL, enum_monitor, (LPARAM)&monitor);

	capture->monitor = monitor.id;

	width  = monitor.rect.right  - monitor.rect.left;
	height = monitor.rect.bottom - monitor.rect.top;

	dc_capture_init(&capture->data, monitor.rect.left, monitor.rect.top,
			width, height, capture->capture_cursor,
			capture->compatibility);
}

static inline void update_settings(struct monitor_capture *capture,
		obs_data_t settings)
{
	capture->capture_cursor = obs_data_get_bool(settings, "capture_cursor");
	capture->compatibility  = obs_data_get_bool(settings, "compatibility");

	dc_capture_free(&capture->data);
	update_monitor(capture, settings);
}

/* ------------------------------------------------------------------------- */

static const char *monitor_capture_getname(void)
{
	return obs_module_text("MonitorCapture");
}

static void monitor_capture_destroy(void *data)
{
	struct monitor_capture *capture = data;

	obs_enter_graphics();

	dc_capture_free(&capture->data);
	gs_effect_destroy(capture->opaque_effect);

	obs_leave_graphics();

	bfree(capture);
}

static void monitor_capture_defaults(obs_data_t settings)
{
	obs_data_set_default_int(settings, "monitor", 0);
	obs_data_set_default_bool(settings, "capture_cursor", true);
	obs_data_set_default_bool(settings, "compatibility", false);
}

static void *monitor_capture_create(obs_data_t settings, obs_source_t source)
{
	struct monitor_capture *capture;
	gs_effect_t opaque_effect = create_opaque_effect();

	if (!opaque_effect)
		return NULL;

	capture = bzalloc(sizeof(struct monitor_capture));
	capture->opaque_effect = opaque_effect;

	update_settings(capture, settings);

	UNUSED_PARAMETER(source);
	return capture;
}

static void monitor_capture_tick(void *data, float seconds)
{
	struct monitor_capture *capture = data;

	obs_enter_graphics();
	dc_capture_capture(&capture->data, NULL);
	obs_leave_graphics();

	UNUSED_PARAMETER(seconds);
}

static void monitor_capture_render(void *data, gs_effect_t effect)
{
	struct monitor_capture *capture = data;
	dc_capture_render(&capture->data, capture->opaque_effect);

	UNUSED_PARAMETER(effect);
}

static uint32_t monitor_capture_width(void *data)
{
	struct monitor_capture *capture = data;
	return capture->data.width;
}

static uint32_t monitor_capture_height(void *data)
{
	struct monitor_capture *capture = data;
	return capture->data.height;
}

struct obs_source_info monitor_capture_info = {
	.id           = "monitor_capture",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.get_name     = monitor_capture_getname,
	.create       = monitor_capture_create,
	.destroy      = monitor_capture_destroy,
	.video_render = monitor_capture_render,
	.video_tick   = monitor_capture_tick,
	.get_width    = monitor_capture_width,
	.get_height   = monitor_capture_height,
	.get_defaults = monitor_capture_defaults
};
