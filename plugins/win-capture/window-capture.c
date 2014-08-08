#include <stdlib.h>
#include <util/dstr.h>
#include "dc-capture.h"
#include <psapi.h>

#define TEXT_WINDOW_CAPTURE obs_module_text("WindowCapture")
#define TEXT_WINDOW         obs_module_text("WindowCapture.Window")
#define TEXT_MATCH_PRIORITY obs_module_text("WindowCapture.Priority")
#define TEXT_MATCH_TITLE    obs_module_text("WindowCapture.Priority.Title")
#define TEXT_MATCH_CLASS    obs_module_text("WindowCapture.Priority.Class")
#define TEXT_MATCH_EXE      obs_module_text("WindowCapture.Priority.Exe")
#define TEXT_CAPTURE_CURSOR obs_module_text("CaptureCursor")
#define TEXT_COMPATIBILITY  obs_module_text("Compatibility")

enum window_priority {
	WINDOW_PRIORITY_CLASS,
	WINDOW_PRIORITY_TITLE,
	WINDOW_PRIORITY_EXE,
};

struct window_capture {
	obs_source_t         source;

	char                 *title;
	char                 *class;
	char                 *executable;
	enum window_priority priority;
	bool                 cursor;
	bool                 compatibility;
	bool                 use_wildcards; /* TODO */

	struct dc_capture    capture;

	float                resize_timer;

	gs_effect_t          opaque_effect;

	HWND                 window;
	RECT                 last_rect;
};

void encode_dstr(struct dstr *str)
{
	dstr_replace(str, "#", "#22");
	dstr_replace(str, ":", "#3A");
}

char *decode_str(const char *src)
{
	struct dstr str = {0};
	dstr_copy(&str, src);
	dstr_replace(&str, "#3A", ":");
	dstr_replace(&str, "#22", "#");
	return str.array;
}

static void update_settings(struct window_capture *wc, obs_data_t s)
{
	const char *window     = obs_data_get_string(s, "window");
	int        priority    = (int)obs_data_get_int(s, "priority");

	bfree(wc->title);
	bfree(wc->class);
	bfree(wc->executable);
	wc->title      = NULL;
	wc->class      = NULL;
	wc->executable = NULL;

	if (window) {
		char **strlist = strlist_split(window, ':', true);

		if (strlist && strlist[0] && strlist[1] && strlist[2]) {
			wc->title      = decode_str(strlist[0]);
			wc->class      = decode_str(strlist[1]);
			wc->executable = decode_str(strlist[2]);
		}

		strlist_free(strlist);
	}

	wc->priority      = (enum window_priority)priority;
	wc->cursor        = obs_data_get_bool(s, "cursor");
	wc->use_wildcards = obs_data_get_bool(s, "use_wildcards");
}

static bool get_exe_name(struct dstr *name, HWND window)
{
	wchar_t     wname[MAX_PATH];
	struct dstr temp    = {0};
	bool        success = false;
	HANDLE      process = NULL;
	char        *slash;
	DWORD       id;

	GetWindowThreadProcessId(window, &id);
	if (id == GetCurrentProcessId())
		return false;

	process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, id);
	if (!process)
		goto fail;

	if (!GetProcessImageFileNameW(process, wname, MAX_PATH))
		goto fail;

	dstr_from_wcs(&temp, wname);
	slash = strrchr(temp.array, '\\');
	if (!slash)
		goto fail;

	dstr_copy(name, slash+1);
	success = true;

fail:
	if (!success)
		dstr_copy(name, "unknown");

	dstr_free(&temp);
	CloseHandle(process);
	return true;
}

static void get_window_title(struct dstr *name, HWND hwnd)
{
	wchar_t *temp;
	int len;

	len = GetWindowTextLengthW(hwnd);
	if (!len)
		return;

	temp = malloc(sizeof(wchar_t) * (len+1));
	GetWindowTextW(hwnd, temp, len+1);
	dstr_from_wcs(name, temp);
	free(temp);
}

static void get_window_class(struct dstr *class, HWND hwnd)
{
	wchar_t temp[256];

	temp[0] = 0;
	GetClassNameW(hwnd, temp, sizeof(temp));
	dstr_from_wcs(class, temp);
}

static void add_window(obs_property_t p, HWND hwnd,
		struct dstr *title,
		struct dstr *class,
		struct dstr *executable)
{
	struct dstr encoded    = {0};
	struct dstr desc       = {0};

	if (!get_exe_name(executable, hwnd))
		return;
	get_window_title(title, hwnd);
	get_window_class(class, hwnd);

	dstr_printf(&desc, "[%s]: %s", executable->array, title->array);

	encode_dstr(title);
	encode_dstr(class);
	encode_dstr(executable);

	dstr_cat_dstr(&encoded, title);
	dstr_cat(&encoded, ":");
	dstr_cat_dstr(&encoded, class);
	dstr_cat(&encoded, ":");
	dstr_cat_dstr(&encoded, executable);

	obs_property_list_add_string(p, desc.array, encoded.array);

	dstr_free(&encoded);
	dstr_free(&desc);
}

static bool check_window_valid(HWND window,
		struct dstr *title,
		struct dstr *class,
		struct dstr *executable)
{
	DWORD styles, ex_styles;
	RECT  rect;

	if (!IsWindowVisible(window) || IsIconic(window))
		return false;

	GetClientRect(window, &rect);
	styles    = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
	ex_styles = (DWORD)GetWindowLongPtr(window, GWL_EXSTYLE);

	if (ex_styles & WS_EX_TOOLWINDOW)
		return false;
	if (styles & WS_CHILD)
		return false;
	if (rect.bottom == 0 || rect.right == 0)
		return false;

	if (!get_exe_name(executable, window))
		return false;
	get_window_title(title, window);
	get_window_class(class, window);
	return true;
}

static inline HWND next_window(HWND window,
		struct dstr *title,
		struct dstr *class,
		struct dstr *exe)
{
	while (true) {
		window = GetNextWindow(window, GW_HWNDNEXT);
		if (!window || check_window_valid(window, title, class, exe))
			break;
	}

	return window;
}

static inline HWND first_window(
		struct dstr *title,
		struct dstr *class,
		struct dstr *executable)
{
	HWND window = GetWindow(GetDesktopWindow(), GW_CHILD);
	if (!check_window_valid(window, title, class, executable))
		window = next_window(window, title, class, executable);
	return window;
}

static void fill_window_list(obs_property_t p)
{
	struct dstr title      = {0};
	struct dstr class      = {0};
	struct dstr executable = {0};

	HWND window = first_window(&title, &class, &executable);

	while (window) {
		add_window(p, window, &title, &class, &executable);
		window = next_window(window, &title, &class, &executable);
	}

	dstr_free(&title);
	dstr_free(&class);
	dstr_free(&executable);
}

static int window_rating(struct window_capture *wc,
		struct dstr *title,
		struct dstr *class,
		struct dstr *executable)
{
	int class_val = 1;
	int title_val = 1;
	int exe_val   = 0;
	int total     = 0;

	if (wc->priority == WINDOW_PRIORITY_CLASS)
		class_val += 3;
	else if (wc->priority == WINDOW_PRIORITY_TITLE)
		title_val += 3;
	else
		exe_val += 3;

	if (dstr_cmpi(class, wc->class) == 0)
		total += class_val;
	if (dstr_cmpi(title, wc->title) == 0)
		total += title_val;
	if (dstr_cmpi(executable, wc->executable) == 0)
		total += exe_val;

	return total;
}

static HWND find_window(struct window_capture *wc)
{
	struct dstr title = {0};
	struct dstr class = {0};
	struct dstr exe   = {0};

	HWND window      = first_window(&title, &class, &exe);
	HWND best_window = NULL;
	int  best_rating = 0;

	while (window) {
		int rating = window_rating(wc, &title, &class, &exe);
		if (rating > best_rating) {
			best_rating = rating;
			best_window = window;
		}

		window = next_window(window, &title, &class, &exe);
	}

	dstr_free(&title);
	dstr_free(&class);
	dstr_free(&exe);

	return best_window;
}

/* ------------------------------------------------------------------------- */

static const char *wc_getname(void)
{
	return TEXT_WINDOW_CAPTURE;
}

static void *wc_create(obs_data_t settings, obs_source_t source)
{
	struct window_capture *wc;
	gs_effect_t opaque_effect = create_opaque_effect();
	if (!opaque_effect)
		return NULL;

	wc                = bzalloc(sizeof(struct window_capture));
	wc->source        = source;
	wc->opaque_effect = opaque_effect;

	update_settings(wc, settings);
	return wc;
}

static void wc_destroy(void *data)
{
	struct window_capture *wc = data;

	if (wc) {
		dc_capture_free(&wc->capture);

		bfree(wc->title);
		bfree(wc->class);
		bfree(wc->executable);

		obs_enter_graphics();
		gs_effect_destroy(wc->opaque_effect);
		obs_leave_graphics();

		bfree(wc);
	}
}

static void wc_update(void *data, obs_data_t settings)
{
	struct window_capture *wc = data;
	update_settings(wc, settings);

	/* forces a reset */
	wc->window = NULL;
}

static uint32_t wc_width(void *data)
{
	struct window_capture *wc = data;
	return wc->capture.width;
}

static uint32_t wc_height(void *data)
{
	struct window_capture *wc = data;
	return wc->capture.height;
}

static void wc_defaults(obs_data_t defaults)
{
	obs_data_set_default_bool(defaults, "cursor", true);
	obs_data_set_default_bool(defaults, "compatibility", false);
}

static obs_properties_t wc_properties(void)
{
	obs_properties_t ppts = obs_properties_create();
	obs_property_t p;

	p = obs_properties_add_list(ppts, "window", TEXT_WINDOW,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	fill_window_list(p);

	p = obs_properties_add_list(ppts, "priority", TEXT_MATCH_PRIORITY,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, TEXT_MATCH_TITLE, WINDOW_PRIORITY_TITLE);
	obs_property_list_add_int(p, TEXT_MATCH_CLASS, WINDOW_PRIORITY_CLASS);
	obs_property_list_add_int(p, TEXT_MATCH_EXE,   WINDOW_PRIORITY_EXE);

	obs_properties_add_bool(ppts, "cursor", TEXT_CAPTURE_CURSOR);

	obs_properties_add_bool(ppts, "compatibility", TEXT_COMPATIBILITY);

	return ppts;
}

#define RESIZE_CHECK_TIME 0.2f

static void wc_tick(void *data, float seconds)
{
	struct window_capture *wc = data;
	RECT rect;
	bool reset_capture = false;

	if (!wc->window || !IsWindow(wc->window)) {
		if (!wc->title && !wc->class)
			return;

		wc->window = find_window(wc);
		if (!wc->window)
			return;

		reset_capture = true;

	} else if (IsIconic(wc->window)) {
		return;
	}

	obs_enter_graphics();

	GetClientRect(wc->window, &rect);

	if (!reset_capture) {
		wc->resize_timer += seconds;

		if (wc->resize_timer >= RESIZE_CHECK_TIME) {
			if (rect.bottom != wc->last_rect.bottom ||
			    rect.right  != wc->last_rect.right)
				reset_capture = true;

			wc->resize_timer = 0.0f;
		}
	}

	if (reset_capture) {
		wc->resize_timer = 0.0f;
		wc->last_rect = rect;
		dc_capture_free(&wc->capture);
		dc_capture_init(&wc->capture, 0, 0, rect.right, rect.bottom,
				wc->cursor, wc->compatibility);
	}

	dc_capture_capture(&wc->capture, wc->window);
	obs_leave_graphics();
}

static void wc_render(void *data, gs_effect_t effect)
{
	struct window_capture *wc = data;
	dc_capture_render(&wc->capture, wc->opaque_effect);

	UNUSED_PARAMETER(effect);
}

struct obs_source_info window_capture_info = {
	.id             = "window_capture",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.get_name       = wc_getname,
	.create         = wc_create,
	.destroy        = wc_destroy,
	.update         = wc_update,
	.video_render   = wc_render,
	.video_tick     = wc_tick,
	.get_width      = wc_width,
	.get_height     = wc_height,
	.get_defaults   = wc_defaults,
	.get_properties = wc_properties
};
