#pragma once

#include <util/dstr.h>
#include <jansson.h>

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
	struct dstr class;
	struct dstr executable;
};

extern bool get_window_exe(struct dstr *name, HWND window);
extern void get_window_title(struct dstr *name, HWND hwnd);
extern void get_window_class(struct dstr *class, HWND hwnd);
extern bool is_uwp_window(HWND hwnd);
extern HWND get_uwp_actual_window(HWND parent);

extern struct game_capture_matching_rule convert_json_to_matching_rule(json_t *json_rule);

extern void get_captured_window_line(HWND hwnd, struct dstr * window_line);

typedef bool (*add_window_cb)(const char *title, const char *class,
			      const char *exe);

extern void fill_window_list(obs_property_t *p, enum window_search_mode mode,
			     add_window_cb callback);

extern void build_window_strings(const char *str, char **class, char **title,
				 char **exe);

extern HWND find_window(enum window_search_mode mode,
			enum window_priority priority, const char *class,
			const char *title, const char *exe);

extern HWND find_matching_window(enum window_search_mode mode,
			DARRAY(struct game_capture_matching_rule) * matching_rules,
			DARRAY(HWND) * checked_windows);

extern HWND find_window_top_level(enum window_search_mode mode,
				  enum window_priority priority,
				  const char *class, const char *title,
				  const char *exe);

extern int find_matching_rule_for_window(HWND window, 
			const DARRAY(struct game_capture_matching_rule) * matching_rules, 
			int *found_index, int already_matched_power);

extern int get_rule_match_power(struct game_capture_matching_rule* rule);
