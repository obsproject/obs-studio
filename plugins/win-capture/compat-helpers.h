#pragma once

enum source_type {
	GAME_CAPTURE,
	WINDOW_CAPTURE_BITBLT,
	WINDOW_CAPTURE_WGC,
};

struct compat_result {
	char *message;
	enum obs_text_info_type severity;
};

extern struct compat_result *check_compatibility(const char *win_title,
						 const char *win_class,
						 const char *exe,
						 enum source_type type);
extern void compat_result_free(struct compat_result *res);
extern void compat_json_free();
