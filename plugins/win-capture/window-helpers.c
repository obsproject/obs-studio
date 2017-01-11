#define PSAPI_VERSION 1
#include <obs.h>
#include <util/dstr.h>

#include <windows.h>
#include <psapi.h>
#include "window-helpers.h"
#include "obfuscate.h"

static inline void encode_dstr(struct dstr *str)
{
	dstr_replace(str, "#", "#22");
	dstr_replace(str, ":", "#3A");
}

static inline char *decode_str(const char *src)
{
	struct dstr str = {0};
	dstr_copy(&str, src);
	dstr_replace(&str, "#3A", ":");
	dstr_replace(&str, "#22", "#");
	return str.array;
}

extern void build_window_strings(const char *str,
		char **class,
		char **title,
		char **exe)
{
	char **strlist;

	*class = NULL;
	*title = NULL;
	*exe   = NULL;

	if (!str) {
		return;
	}

	strlist = strlist_split(str, ':', true);

	if (strlist && strlist[0] && strlist[1] && strlist[2]) {
		*title = decode_str(strlist[0]);
		*class = decode_str(strlist[1]);
		*exe   = decode_str(strlist[2]);
	}

	strlist_free(strlist);
}

static HMODULE kernel32(void)
{
	static HMODULE kernel32_handle = NULL;
	if (!kernel32_handle)
		kernel32_handle = GetModuleHandleA("kernel32");
	return kernel32_handle;
}

static inline HANDLE open_process(DWORD desired_access, bool inherit_handle,
		DWORD process_id)
{
	static HANDLE (WINAPI *open_process_proc)(DWORD, BOOL, DWORD) = NULL;
	if (!open_process_proc)
		open_process_proc = get_obfuscated_func(kernel32(),
				"B}caZyah`~q", 0x2D5BEBAF6DDULL);

	return open_process_proc(desired_access, inherit_handle, process_id);
}

bool get_window_exe(struct dstr *name, HWND window)
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

	process = open_process(PROCESS_QUERY_LIMITED_INFORMATION, false, id);
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

void get_window_title(struct dstr *name, HWND hwnd)
{
	wchar_t *temp;
	int len;

	len = GetWindowTextLengthW(hwnd);
	if (!len)
		return;

	temp = malloc(sizeof(wchar_t) * (len+1));
	if (GetWindowTextW(hwnd, temp, len+1))
		dstr_from_wcs(name, temp);
	free(temp);
}

void get_window_class(struct dstr *class, HWND hwnd)
{
	wchar_t temp[256];

	temp[0] = 0;
	if (GetClassNameW(hwnd, temp, sizeof(temp) / sizeof(wchar_t)))
		dstr_from_wcs(class, temp);
}

/* not capturable or internal windows */
static const char *internal_microsoft_exes[] = {
	"applicationframehost",
	"shellexperiencehost",
	"winstore.app",
	"searchui",
	NULL
};

static bool is_microsoft_internal_window_exe(const char *exe)
{
	char cur_exe[MAX_PATH];

	if (!exe)
		return false;

	for (const char **vals = internal_microsoft_exes; *vals; vals++) {
		strcpy(cur_exe, *vals);
		strcat(cur_exe, ".exe");

		if (strcmpi(cur_exe, exe) == 0)
			return true;
	}

	return false;
}

static void add_window(obs_property_t *p, HWND hwnd, add_window_cb callback)
{
	struct dstr class   = {0};
	struct dstr title   = {0};
	struct dstr exe     = {0};
	struct dstr encoded = {0};
	struct dstr desc    = {0};

	if (!get_window_exe(&exe, hwnd))
		return;
	if (is_microsoft_internal_window_exe(exe.array)) {
		dstr_free(&exe);
		return;
	}
	get_window_title(&title, hwnd);
	get_window_class(&class, hwnd);

	if (callback && !callback(title.array, class.array, exe.array)) {
		dstr_free(&title);
		dstr_free(&class);
		dstr_free(&exe);
		return;
	}

	dstr_printf(&desc, "[%s]: %s", exe.array, title.array);

	encode_dstr(&title);
	encode_dstr(&class);
	encode_dstr(&exe);

	dstr_cat_dstr(&encoded, &title);
	dstr_cat(&encoded, ":");
	dstr_cat_dstr(&encoded, &class);
	dstr_cat(&encoded, ":");
	dstr_cat_dstr(&encoded, &exe);

	obs_property_list_add_string(p, desc.array, encoded.array);

	dstr_free(&encoded);
	dstr_free(&desc);
	dstr_free(&class);
	dstr_free(&title);
	dstr_free(&exe);
}

static bool check_window_valid(HWND window, enum window_search_mode mode)
{
	DWORD styles, ex_styles;
	RECT  rect;

	if (!IsWindowVisible(window) ||
	    (mode == EXCLUDE_MINIMIZED && IsIconic(window)))
		return false;

	GetClientRect(window, &rect);
	styles    = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
	ex_styles = (DWORD)GetWindowLongPtr(window, GWL_EXSTYLE);

	if (ex_styles & WS_EX_TOOLWINDOW)
		return false;
	if (styles & WS_CHILD)
		return false;
	if (mode == EXCLUDE_MINIMIZED && (rect.bottom == 0 || rect.right == 0))
		return false;

	return true;
}

bool is_uwp_window(HWND hwnd)
{
	wchar_t name[256];

	name[0] = 0;
	if (!GetClassNameW(hwnd, name, sizeof(name) / sizeof(wchar_t)))
		return false;

	return wcscmp(name, L"ApplicationFrameWindow") == 0;
}

HWND get_uwp_actual_window(HWND parent)
{
	DWORD parent_id = 0;
	HWND child;

	GetWindowThreadProcessId(parent, &parent_id);
	child = FindWindowEx(parent, NULL, NULL, NULL);

	while (child) {
		DWORD child_id = 0;
		GetWindowThreadProcessId(child, &child_id);

		if (child_id != parent_id)
			return child;

		child = FindWindowEx(parent, child, NULL, NULL);
	}

	return NULL;
}

static inline HWND next_window(HWND window, enum window_search_mode mode,
		HWND *parent)
{
	if (*parent) {
		window = *parent;
		*parent = NULL;
	}

	while (true) {
		window = FindWindowEx(GetDesktopWindow(), window, NULL, NULL);
		if (!window || check_window_valid(window, mode))
			break;
	}

	if (is_uwp_window(window)) {
		HWND child = get_uwp_actual_window(window);
		if (child) {
			*parent = window;
			return child;
		}
	}

	return window;
}

static inline HWND first_window(enum window_search_mode mode, HWND *parent)
{
	HWND window = FindWindowEx(GetDesktopWindow(), NULL, NULL, NULL);

	*parent = NULL;

	if (!check_window_valid(window, mode))
		window = next_window(window, mode, parent);

	if (is_uwp_window(window)) {
		HWND child = get_uwp_actual_window(window);
		if (child) {
			*parent = window;
			return child;
		}
	}

	return window;
}

void fill_window_list(obs_property_t *p, enum window_search_mode mode,
		add_window_cb callback)
{
	HWND parent;
	HWND window = first_window(mode, &parent);

	while (window) {
		add_window(p, window, callback);
		window = next_window(window, mode, &parent);
	}
}

static int window_rating(HWND window,
		enum window_priority priority,
		const char *class,
		const char *title,
		const char *exe,
		bool uwp_window)
{
	struct dstr cur_class = {0};
	struct dstr cur_title = {0};
	struct dstr cur_exe   = {0};
	int         class_val = 1;
	int         title_val = 1;
	int         exe_val   = 0;
	int         total     = 0;

	if (!get_window_exe(&cur_exe, window))
		return 0;
	get_window_title(&cur_title, window);
	get_window_class(&cur_class, window);

	if (priority == WINDOW_PRIORITY_CLASS)
		class_val += 3;
	else if (priority == WINDOW_PRIORITY_TITLE)
		title_val += 3;
	else
		exe_val += 3;

	if (uwp_window) {
		if (dstr_cmpi(&cur_title, title) == 0 &&
		    dstr_cmpi(&cur_exe, exe) == 0)
			total += exe_val + title_val + class_val;
	} else {
		if (dstr_cmpi(&cur_class, class) == 0)
			total += class_val;
		if (dstr_cmpi(&cur_title, title) == 0)
			total += title_val;
		if (dstr_cmpi(&cur_exe, exe) == 0)
			total += exe_val;
	}

	dstr_free(&cur_class);
	dstr_free(&cur_title);
	dstr_free(&cur_exe);

	return total;
}

HWND find_window(enum window_search_mode mode,
		enum window_priority priority,
		const char *class,
		const char *title,
		const char *exe)
{
	HWND parent;
	HWND window      = first_window(mode, &parent);
	HWND best_window = NULL;
	int  best_rating = 0;

	if (!class)
		return NULL;

	bool uwp_window  = strcmp(class, "Windows.UI.Core.CoreWindow") == 0;

	while (window) {
		int rating = window_rating(window, priority, class, title, exe,
				uwp_window);
		if (rating > best_rating) {
			best_rating = rating;
			best_window = window;
		}

		window = next_window(window, mode, &parent);
	}

	return best_window;
}
