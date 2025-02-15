#include "window-helpers.h"

#include <util/windows/obfuscate.h>

#include <dwmapi.h>
#include <psapi.h>

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

void ms_build_window_strings(const char *str, char **class, char **title, char **exe)
{
	char **strlist;

	*class = NULL;
	*title = NULL;
	*exe = NULL;

	if (!str) {
		return;
	}

	strlist = strlist_split(str, ':', true);

	if (strlist && strlist[0] && strlist[1] && strlist[2]) {
		*title = decode_str(strlist[0]);
		*class = decode_str(strlist[1]);
		*exe = decode_str(strlist[2]);
	}

	strlist_free(strlist);
}

static void insert_preserved_val(obs_property_t *p, const char *val, size_t idx)
{
	char *window_class = NULL;
	char *title = NULL;
	char *executable = NULL;
	struct dstr desc = {0};

	ms_build_window_strings(val, &window_class, &title, &executable);

	dstr_printf(&desc, "[%s]: %s", executable, title);
	obs_property_list_insert_string(p, idx, desc.array, val);
	obs_property_list_item_disable(p, idx, true);

	dstr_free(&desc);
	bfree(window_class);
	bfree(title);
	bfree(executable);
}

bool ms_check_window_property_setting(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings, const char *val,
				      size_t idx)
{
	const char *cur_val;
	bool match = false;
	size_t i = 0;

	cur_val = obs_data_get_string(settings, val);
	if (!cur_val) {
		return false;
	}

	for (;;) {
		const char *val = obs_property_list_item_string(p, i++);
		if (!val)
			break;

		if (strcmp(val, cur_val) == 0) {
			match = true;
			break;
		}
	}

	if (cur_val && *cur_val && !match) {
		insert_preserved_val(p, cur_val, idx);
		return true;
	}

	UNUSED_PARAMETER(ppts);
	return false;
}

static HMODULE kernel32(void)
{
	static HMODULE kernel32_handle = NULL;
	if (!kernel32_handle)
		kernel32_handle = GetModuleHandleA("kernel32");
	return kernel32_handle;
}

static inline HANDLE open_process(DWORD desired_access, bool inherit_handle, DWORD process_id)
{
	typedef HANDLE(WINAPI * PFN_OpenProcess)(DWORD, BOOL, DWORD);
	static PFN_OpenProcess open_process_proc = NULL;
	if (!open_process_proc)
		open_process_proc =
			(PFN_OpenProcess)ms_get_obfuscated_func(kernel32(), "B}caZyah`~q", 0x2D5BEBAF6DDULL);

	return open_process_proc(desired_access, inherit_handle, process_id);
}

bool ms_get_window_exe(struct dstr *name, HWND window)
{
	wchar_t wname[MAX_PATH];
	struct dstr temp = {0};
	bool success = false;
	HANDLE process = NULL;
	char *slash;
	DWORD id;

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

	dstr_copy(name, slash + 1);
	success = true;

fail:
	if (!success)
		dstr_copy(name, "unknown");

	dstr_free(&temp);
	CloseHandle(process);
	return true;
}

void ms_get_window_title(struct dstr *name, HWND hwnd)
{
	int len;

	len = GetWindowTextLengthW(hwnd);
	if (!len)
		return;

	if (len > 1024) {
		wchar_t *temp;

		temp = malloc(sizeof(wchar_t) * (len + 1));
		if (!temp)
			return;

		if (GetWindowTextW(hwnd, temp, len + 1))
			dstr_from_wcs(name, temp);

		free(temp);
	} else {
		wchar_t temp[1024 + 1];

		if (GetWindowTextW(hwnd, temp, len + 1))
			dstr_from_wcs(name, temp);
	}
}

void ms_get_window_class(struct dstr *class, HWND hwnd)
{
	wchar_t temp[256];

	temp[0] = 0;
	if (GetClassNameW(hwnd, temp, sizeof(temp) / sizeof(wchar_t)))
		dstr_from_wcs(class, temp);
}

/* not capturable or internal windows, exact executable names */
static const char *internal_microsoft_exes_exact[] = {
	"startmenuexperiencehost.exe",
	"applicationframehost.exe",
	"peopleexperiencehost.exe",
	"shellexperiencehost.exe",
	"microsoft.notes.exe",
	"systemsettings.exe",
	"textinputhost.exe",
	"searchapp.exe",
	"video.ui.exe",
	"searchui.exe",
	"lockapp.exe",
	"cortana.exe",
	"gamebar.exe",
	"tabtip.exe",
	"time.exe",
	NULL,
};

/* partial matches start from the beginning of the executable name */
static const char *internal_microsoft_exes_partial[] = {
	"windowsinternal",
	NULL,
};

static bool is_microsoft_internal_window_exe(const char *exe)
{
	if (!exe)
		return false;

	for (const char **vals = internal_microsoft_exes_exact; *vals; vals++) {
		if (astrcmpi(exe, *vals) == 0)
			return true;
	}

	for (const char **vals = internal_microsoft_exes_partial; *vals; vals++) {
		if (astrcmpi_n(exe, *vals, strlen(*vals)) == 0)
			return true;
	}

	return false;
}

static void add_window(obs_property_t *p, HWND hwnd, add_window_cb callback)
{
	struct dstr class = {0};
	struct dstr title = {0};
	struct dstr exe = {0};
	struct dstr encoded = {0};
	struct dstr desc = {0};

	if (!ms_get_window_exe(&exe, hwnd))
		return;
	if (is_microsoft_internal_window_exe(exe.array)) {
		dstr_free(&exe);
		return;
	}

	ms_get_window_title(&title, hwnd);
	if (dstr_cmp(&exe, "explorer.exe") == 0 && dstr_is_empty(&title)) {
		dstr_free(&exe);
		dstr_free(&title);
		return;
	}

	ms_get_window_class(&class, hwnd);

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

static inline bool IsWindowCloaked(HWND window)
{
	DWORD cloaked;
	HRESULT hr = DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
	return SUCCEEDED(hr) && cloaked;
}

static bool check_window_valid(HWND window, enum window_search_mode mode)
{
	DWORD styles, ex_styles;
	RECT rect;

	if (!IsWindowVisible(window) || (mode == EXCLUDE_MINIMIZED && (IsIconic(window) || IsWindowCloaked(window))))
		return false;

	GetClientRect(window, &rect);
	styles = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
	ex_styles = (DWORD)GetWindowLongPtr(window, GWL_EXSTYLE);

	if (ex_styles & WS_EX_TOOLWINDOW)
		return false;
	if (styles & WS_CHILD)
		return false;
	if (mode == EXCLUDE_MINIMIZED && (rect.bottom == 0 || rect.right == 0))
		return false;

	return true;
}

bool ms_is_uwp_window(HWND hwnd)
{
	wchar_t name[256];

	name[0] = 0;
	if (!GetClassNameW(hwnd, name, sizeof(name) / sizeof(wchar_t)))
		return false;

	return wcscmp(name, L"ApplicationFrameWindow") == 0 || wcscmp(name, L"WinUIDesktopWin32WindowClass") == 0;
}

HWND ms_get_uwp_actual_window(HWND parent)
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

static HWND next_window(HWND window, enum window_search_mode mode, HWND *parent, bool use_findwindowex)
{
	if (*parent) {
		window = *parent;
		*parent = NULL;
	}

	while (true) {
		if (use_findwindowex)
			window = FindWindowEx(GetDesktopWindow(), window, NULL, NULL);
		else
			window = GetNextWindow(window, GW_HWNDNEXT);

		if (!window || check_window_valid(window, mode))
			break;
	}

	if (ms_is_uwp_window(window)) {
		HWND child = ms_get_uwp_actual_window(window);
		if (child) {
			*parent = window;
			return child;
		}
	}

	return window;
}

static HWND first_window(enum window_search_mode mode, HWND *parent, bool *use_findwindowex)
{
	HWND window = FindWindowEx(GetDesktopWindow(), NULL, NULL, NULL);

	if (!window) {
		*use_findwindowex = false;
		window = GetWindow(GetDesktopWindow(), GW_CHILD);
	} else {
		*use_findwindowex = true;
	}

	*parent = NULL;

	if (!check_window_valid(window, mode)) {
		window = next_window(window, mode, parent, *use_findwindowex);

		if (!window && *use_findwindowex) {
			*use_findwindowex = false;

			window = GetWindow(GetDesktopWindow(), GW_CHILD);
			if (!check_window_valid(window, mode))
				window = next_window(window, mode, parent, *use_findwindowex);
		}
	}

	if (ms_is_uwp_window(window)) {
		HWND child = ms_get_uwp_actual_window(window);
		if (child) {
			*parent = window;
			return child;
		}
	}

	return window;
}

void ms_fill_window_list(obs_property_t *p, enum window_search_mode mode, add_window_cb callback)
{
	HWND parent;
	bool use_findwindowex = false;

	HWND window = first_window(mode, &parent, &use_findwindowex);

	while (window) {
		add_window(p, window, callback);
		window = next_window(window, mode, &parent, use_findwindowex);
	}
}

static int window_rating(HWND window, enum window_priority priority, const char *class, const char *title,
			 const char *exe, bool uwp_window, bool generic_class)
{
	struct dstr cur_class = {0};
	struct dstr cur_title = {0};
	struct dstr cur_exe = {0};
	int val = 0x7FFFFFFF;

	if (!ms_get_window_exe(&cur_exe, window))
		return 0x7FFFFFFF;
	ms_get_window_title(&cur_title, window);
	ms_get_window_class(&cur_class, window);

	bool class_matches = dstr_cmpi(&cur_class, class) == 0;
	bool exe_matches = dstr_cmpi(&cur_exe, exe) == 0;
	int title_val = abs(dstr_cmpi(&cur_title, title));

	if (generic_class && (priority == WINDOW_PRIORITY_CLASS))
		priority = WINDOW_PRIORITY_TITLE;

	/* always match by name with UWP windows */
	if (uwp_window) {
		if (priority == WINDOW_PRIORITY_EXE && !exe_matches)
			val = 0x7FFFFFFF;
		else
			val = title_val == 0 ? 0 : 0x7FFFFFFF;

	} else if (priority == WINDOW_PRIORITY_CLASS) {
		val = class_matches ? title_val : 0x7FFFFFFF;
		if (val != 0x7FFFFFFF && !exe_matches)
			val += 0x1000;

	} else if (priority == WINDOW_PRIORITY_TITLE) {
		val = title_val == 0 ? 0 : 0x7FFFFFFF;

	} else if (priority == WINDOW_PRIORITY_EXE) {
		val = exe_matches ? title_val : 0x7FFFFFFF;
	}

	dstr_free(&cur_class);
	dstr_free(&cur_title);
	dstr_free(&cur_exe);

	return val;
}

static const char *generic_class_substrings[] = {
	"Chrome",
	NULL,
};

static bool is_generic_class(const char *current_class)
{
	const char **class = generic_class_substrings;
	while (*class) {
		if (astrstri(current_class, *class) != NULL) {
			return true;
		}
		class ++;
	}

	return false;
}

static bool is_uwp_class(const char *window_class)
{
	return strcmp(window_class, "Windows.UI.Core.CoreWindow") == 0 ||
	       strcmp(window_class, "WinUIDesktopWin32WindowClass") == 0;
}

HWND ms_find_window(enum window_search_mode mode, enum window_priority priority, const char *class, const char *title,
		    const char *exe)
{
	HWND parent;
	bool use_findwindowex = false;

	HWND window = first_window(mode, &parent, &use_findwindowex);
	HWND best_window = NULL;
	int best_rating = 0x7FFFFFFF;

	if (!class)
		return NULL;

	const bool uwp_window = is_uwp_class(class);
	const bool generic_class = is_generic_class(class);

	while (window) {
		int rating = window_rating(window, priority, class, title, exe, uwp_window, generic_class);
		if (rating < best_rating) {
			best_rating = rating;
			best_window = window;
			if (rating == 0)
				break;
		}

		window = next_window(window, mode, &parent, use_findwindowex);
	}

	return best_window;
}

struct top_level_enum_data {
	enum window_search_mode mode;
	enum window_priority priority;
	const char *class;
	const char *title;
	const char *exe;
	bool uwp_window;
	bool generic_class;
	HWND best_window;
	int best_rating;
};

BOOL CALLBACK enum_windows_proc(HWND window, LPARAM lParam)
{
	struct top_level_enum_data *data = (struct top_level_enum_data *)lParam;

	if (!check_window_valid(window, data->mode))
		return TRUE;

	if (IsWindowCloaked(window))
		return TRUE;

	const int rating = window_rating(window, data->priority, data->class, data->title, data->exe, data->uwp_window,
					 data->generic_class);
	if (rating < data->best_rating) {
		data->best_rating = rating;
		data->best_window = window;
	}

	return rating > 0;
}

HWND ms_find_window_top_level(enum window_search_mode mode, enum window_priority priority, const char *class,
			      const char *title, const char *exe)
{
	if (!class)
		return NULL;

	struct top_level_enum_data data;
	data.mode = mode;
	data.priority = priority;
	data.class = class;
	data.title = title;
	data.exe = exe;
	data.uwp_window = is_uwp_class(class);
	data.generic_class = is_generic_class(class);
	data.best_window = NULL;
	data.best_rating = 0x7FFFFFFF;
	EnumWindows(enum_windows_proc, (LPARAM)&data);
	return data.best_window;
}
