#pragma once

#include <obs-properties.h>
#include <util/c99defs.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <Windows.h>
#include <jansson.h>

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

enum window_match_mask {
	WINDOW_MATCH_EXE = 0x01,
	WINDOW_MATCH_TITLE = 0x02,
	WINDOW_MATCH_CLASS = 0x04,
};

enum window_match_type {
	WINDOW_MATCH_CAPTURE = 0x10,
	WINDOW_MATCH_IGNORE = 0x20
};

struct game_capture_matching_rule {
	int power;
	int mask;
	enum window_match_type type;
	struct dstr title;
	struct dstr classW;
	struct dstr executable;
};

typedef DARRAY(struct game_capture_matching_rule)
	game_capture_matching_rule_array_t;
typedef DARRAY(HWND) window_handles_t;

EXPORT bool ms_get_window_exe(struct dstr *name, HWND window);
EXPORT void ms_get_window_title(struct dstr *name, HWND hwnd);
EXPORT void ms_get_window_class(struct dstr *window_class, HWND hwnd);
EXPORT bool ms_is_uwp_window(HWND hwnd);
EXPORT HWND ms_get_uwp_actual_window(HWND parent);

EXPORT struct game_capture_matching_rule
	convert_json_to_matching_rule(json_t *json_rule);

EXPORT void get_captured_window_line(HWND hwnd, struct dstr *window_line);

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
EXPORT HWND find_matching_window(enum window_search_mode mode,
		     game_capture_matching_rule_array_t *matching_rules,
		     window_handles_t *checked_windows);
EXPORT HWND ms_find_window_top_level(enum window_search_mode mode,
				     enum window_priority priority,
				     const char *window_class,
				     const char *title, const char *exe);

extern int find_matching_rule_for_window(HWND window,
			game_capture_matching_rule_array_t *matching_rules,
			int *found_index, int already_matched_power);

EXPORT int get_rule_match_power(struct game_capture_matching_rule* rule);

#ifdef __cplusplus
}
#endif
