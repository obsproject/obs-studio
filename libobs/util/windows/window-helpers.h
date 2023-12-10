#pragma once

#include <obs-properties.h>
#include <util/c99defs.h>
#include <util/dstr.h>
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

enum window_priority {
	WINDOW_PRIORITY_CLASS,
	WINDOW_PRIORITY_TITLE,
	WINDOW_PRIORITY_EXE,
};

enum window_search_mode {
	INCLUDE_MINIMIZED,
	EXCLUDE_MINIMIZED,
};

EXPORT bool ms_get_window_exe(struct dstr *name, HWND window);
EXPORT void ms_get_window_title(struct dstr *name, HWND hwnd);
EXPORT void ms_get_window_class(struct dstr *window_class, HWND hwnd);
EXPORT bool ms_is_uwp_window(HWND hwnd);
EXPORT HWND ms_get_uwp_actual_window(HWND parent);

typedef bool (*add_window_cb)(const char *title, const char *window_class,
			      const char *exe);

EXPORT void ms_fill_window_list(obs_property_t *p, enum window_search_mode mode,
				add_window_cb callback);

EXPORT void ms_build_window_strings(const char *str, char **window_class,
				    char **title, char **exe);

EXPORT bool ms_check_window_property_setting(obs_properties_t *ppts,
					     obs_property_t *p,
					     obs_data_t *settings,
					     const char *val, size_t idx);

EXPORT void ms_build_window_strings(const char *str, char **window_class,
				    char **title, char **exe);

EXPORT HWND ms_find_window(enum window_search_mode mode,
			   enum window_priority priority,
			   const char *window_class, const char *title,
			   const char *exe);
EXPORT HWND ms_find_window_top_level(enum window_search_mode mode,
				     enum window_priority priority,
				     const char *window_class,
				     const char *title, const char *exe);

#ifdef __cplusplus
}
#endif
