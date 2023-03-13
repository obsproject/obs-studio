#include <jansson.h>

#include <obs-module.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/windows/window-helpers.h>

#include "compat-helpers.h"
#include "compat-format-ver.h"

enum match_flags {
	MATCH_EXE = 1 << 0,
	MATCH_TITLE = 1 << 1,
	MATCH_CLASS = 1 << 2,
};

static inline const char *get_string_val(json_t *entry, const char *key)
{
	json_t *str_val = json_object_get(entry, key);
	if (!str_val || !json_is_string(str_val))
		return NULL;

	return json_string_value(str_val);
}

static inline int get_int_val(json_t *entry, const char *key)
{
	json_t *integer_val = json_object_get(entry, key);
	if (!integer_val || !json_is_integer(integer_val))
		return 0;

	return (int)json_integer_value(integer_val);
}

static inline bool get_bool_val(json_t *entry, const char *key)
{
	json_t *bool_val = json_object_get(entry, key);
	if (!bool_val || !json_is_boolean(bool_val))
		return false;

	return json_is_true(bool_val);
}

static json_t *open_json_file(const char *file)
{
	char *file_data = os_quick_read_utf8_file(file);
	json_error_t error;
	json_t *root;
	json_t *list;
	int format_ver;

	if (!file_data)
		return NULL;

	root = json_loads(file_data, JSON_REJECT_DUPLICATES, &error);
	bfree(file_data);

	if (!root) {
		blog(LOG_WARNING,
		     "compat-helpers.c: [open_json_file] "
		     "Error reading JSON file (%d): %s",
		     error.line, error.text);
		return NULL;
	}

	format_ver = get_int_val(root, "format_version");

	if (format_ver != COMPAT_FORMAT_VERSION) {
		blog(LOG_DEBUG,
		     "compat-helpers.c: [open_json_file] "
		     "Wrong format version (%d), expected %d",
		     format_ver, COMPAT_FORMAT_VERSION);
		json_decref(root);
		return NULL;
	}

	list = json_object_get(root, "entries");
	if (list)
		json_incref(list);
	json_decref(root);

	if (!list) {
		blog(LOG_WARNING, "compat-helpers.c: [open_json_file] "
				  "No compatibility list");
		return NULL;
	}

	return list;
}

static json_t *open_compat_file(void)
{
	char *file;
	json_t *root = NULL;

	file = obs_module_config_path("compatibility.json");
	if (file) {
		root = open_json_file(file);
		bfree(file);
	}

	if (!root) {
		file = obs_module_file("compatibility.json");
		if (file) {
			root = open_json_file(file);
			bfree(file);
		}
	}

	return root;
}

static json_t *compat_entries;
struct compat_result *check_compatibility(const char *win_title,
					  const char *win_class,
					  const char *exe,
					  enum source_type type)
{
	if (!compat_entries) {
		json_t *root = open_compat_file();
		if (!root)
			return NULL;

		compat_entries = root;
	}

	struct dstr message;
	struct compat_result *res = NULL;
	json_t *entry;
	size_t index;

	json_array_foreach (compat_entries, index, entry) {
		if (type == GAME_CAPTURE &&
		    !get_bool_val(entry, "game_capture"))
			continue;
		if (type == WINDOW_CAPTURE_WGC &&
		    !get_bool_val(entry, "window_capture_wgc"))
			continue;
		if (type == WINDOW_CAPTURE_BITBLT &&
		    !get_bool_val(entry, "window_capture"))
			continue;

		int match_flags = get_int_val(entry, "match_flags");
		const char *j_exe = get_string_val(entry, "executable");
		const char *j_title = get_string_val(entry, "window_title");
		const char *j_class = get_string_val(entry, "window_class");

		if (win_class && (match_flags & MATCH_CLASS) &&
		    strcmp(win_class, j_class) != 0)
			continue;
		if (exe && (match_flags & MATCH_EXE) &&
		    astrcmpi(exe, j_exe) != 0)
			continue;
		/* Title supports partial matches as some games append additional
		 * information after the title, e.g., "Minecraft 1.18". */
		if (win_title && (match_flags & MATCH_TITLE) &&
		    astrcmpi_n(win_title, j_title, strlen(j_title)) != 0)
			continue;

		/* Attempt to translate and compile message */
		const char *key = get_string_val(entry, "translation_key");
		const char *msg = get_string_val(entry, "message");
		obs_module_get_string(key, &msg);

		dstr_init_copy(&message, msg);

		const char *name = get_string_val(entry, "name");
		/* Replace placeholders in generic messages */
		if (name && dstr_find(&message, "%") != NULL) {
			dstr_replace(&message, "%name%", name);
		}

		const char *url = get_string_val(entry, "url");
		/* Append clickable URL in Qt rich text */
		if (url && strncmp(url, "https://", 8) == 0) {
			dstr_catf(&message, "<br>\n<a href=\"%s\">%s</a>", url,
				  url + 8);
		}

		res = bzalloc(sizeof(struct compat_result));
		res->severity = get_int_val(entry, "severity");
		res->message = message.array;

		break;
	}

	return res;
}

void compat_result_free(struct compat_result *res)
{
	bfree(res->message);
	bfree(res);
}

void compat_json_free()
{
	json_decref(compat_entries);
}
