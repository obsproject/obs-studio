/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <inttypes.h>
#include <stdio.h>
#include <wchar.h>

#include "config-file.h"
#include "threading.h"
#include "platform.h"
#include "base.h"
#include "bmem.h"
#include "lexer.h"
#include "dstr.h"
#include "uthash.h"

struct config_item {
	char *name;
	char *value;
	UT_hash_handle hh;
};

static inline void config_item_free(struct config_item *item)
{
	bfree(item->name);
	bfree(item->value);
	bfree(item);
}

struct config_section {
	char *name;
	struct config_item *items;
	UT_hash_handle hh;
};

static inline void config_section_free(struct config_section *section)
{
	struct config_item *item;
	struct config_item *temp;
	HASH_ITER (hh, section->items, item, temp) {
		HASH_DELETE(hh, section->items, item);
		config_item_free(item);
	}

	bfree(section->name);
	bfree(section);
}

struct config_data {
	char *file;
	struct config_section *sections;
	struct config_section *defaults;
	pthread_mutex_t mutex;
};

config_t *config_create(const char *file)
{
	struct config_data *config;
	FILE *f;

	f = os_fopen(file, "wb");
	if (!f)
		return NULL;
	fclose(f);

	config = bzalloc(sizeof(struct config_data));

	if (pthread_mutex_init_recursive(&config->mutex) != 0) {
		bfree(config);
		return NULL;
	}

	config->file = bstrdup(file);
	return config;
}

static bool config_parse_string(struct lexer *lex, struct strref *ref, char end)
{
	bool success = end != 0;
	struct base_token token;
	base_token_clear(&token);

	while (lexer_getbasetoken(lex, &token, PARSE_WHITESPACE)) {
		if (end) {
			if (*token.text.array == end) {
				success = true;
				break;
			} else if (is_newline(*token.text.array)) {
				success = false;
				break;
			}
		} else {
			if (is_newline(*token.text.array)) {
				success = true;
				break;
			}
		}

		strref_add(ref, &token.text);
	}

	//remove_ref_whitespace(ref);
	return success;
}

static void unescape(struct dstr *str)
{
	char *read = str->array;
	char *write = str->array;

	for (; *read; read++, write++) {
		char cur = *read;
		if (cur == '\\') {
			char next = read[1];
			if (next == '\\') {
				read++;
			} else if (next == 'r') {
				cur = '\r';
				read++;
			} else if (next == 'n') {
				cur = '\n';
				read++;
			}
		}

		if (read != write)
			*write = cur;
	}

	if (read != write)
		*write = '\0';
}

static void config_add_item(struct config_item **items, struct strref *name, struct strref *value)
{
	struct config_item *item;
	struct dstr item_value;

	item = bzalloc(sizeof(struct config_item));
	item->name = bstrdup_n(name->array, name->len);

	if (!strref_is_empty(value)) {
		dstr_init_copy_strref(&item_value, value);
		unescape(&item_value);
		item->value = bstrdup_n(item_value.array, item_value.len);
		dstr_free(&item_value);
	} else {
		item->value = bzalloc(1);
	}

	HASH_ADD_STR(*items, name, item);
}

static void config_parse_section(struct config_section *section, struct lexer *lex)
{
	struct base_token token;

	while (lexer_getbasetoken(lex, &token, PARSE_WHITESPACE)) {
		struct strref name, value;

		while (token.type == BASETOKEN_WHITESPACE) {
			if (!lexer_getbasetoken(lex, &token, PARSE_WHITESPACE))
				return;
		}

		if (token.type == BASETOKEN_OTHER) {
			if (*token.text.array == '#') {
				do {
					if (!lexer_getbasetoken(lex, &token, PARSE_WHITESPACE))
						return;
				} while (!is_newline(*token.text.array));

				continue;
			} else if (*token.text.array == '[') {
				lex->offset--;
				return;
			}
		}

		strref_copy(&name, &token.text);
		if (!config_parse_string(lex, &name, '='))
			continue;

		strref_clear(&value);
		config_parse_string(lex, &value, 0);
		config_add_item(&section->items, &name, &value);
	}
}

static void parse_config_data(struct config_section **sections, struct lexer *lex)
{
	struct strref section_name;
	struct base_token token;

	base_token_clear(&token);

	while (lexer_getbasetoken(lex, &token, PARSE_WHITESPACE)) {
		struct config_section *section;

		while (token.type == BASETOKEN_WHITESPACE) {
			if (!lexer_getbasetoken(lex, &token, PARSE_WHITESPACE))
				return;
		}

		if (*token.text.array != '[') {
			while (!is_newline(*token.text.array)) {
				if (!lexer_getbasetoken(lex, &token, PARSE_WHITESPACE))
					return;
			}

			continue;
		}

		strref_clear(&section_name);
		config_parse_string(lex, &section_name, ']');
		if (!section_name.len)
			return;

		section = bzalloc(sizeof(struct config_section));
		section->name = bstrdup_n(section_name.array, section_name.len);

		config_parse_section(section, lex);

		HASH_ADD_STR(*sections, name, section);
	}
}

static int config_parse_file(struct config_section **sections, const char *file, bool always_open)
{
	char *file_data;
	struct lexer lex;
	FILE *f;

	f = os_fopen(file, "rb");
	if (always_open && !f)
		f = os_fopen(file, "w+");
	if (!f)
		return CONFIG_FILENOTFOUND;

	os_fread_utf8(f, &file_data);
	fclose(f);

	if (!file_data)
		return CONFIG_SUCCESS;

	lexer_init(&lex);
	lexer_start_move(&lex, file_data);

	parse_config_data(sections, &lex);

	lexer_free(&lex);
	return CONFIG_SUCCESS;
}

int config_open(config_t **config, const char *file, enum config_open_type open_type)
{
	int errorcode;
	bool always_open = open_type == CONFIG_OPEN_ALWAYS;

	if (!config)
		return CONFIG_ERROR;

	*config = bzalloc(sizeof(struct config_data));
	if (!*config)
		return CONFIG_ERROR;

	if (pthread_mutex_init_recursive(&(*config)->mutex) != 0) {
		bfree(*config);
		return CONFIG_ERROR;
	}

	(*config)->file = bstrdup(file);

	errorcode = config_parse_file(&(*config)->sections, file, always_open);

	if (errorcode != CONFIG_SUCCESS) {
		config_close(*config);
		*config = NULL;
	}

	return errorcode;
}

int config_open_string(config_t **config, const char *str)
{
	struct lexer lex;

	if (!config)
		return CONFIG_ERROR;

	*config = bzalloc(sizeof(struct config_data));
	if (!*config)
		return CONFIG_ERROR;

	if (pthread_mutex_init_recursive(&(*config)->mutex) != 0) {
		bfree(*config);
		return CONFIG_ERROR;
	}

	(*config)->file = NULL;

	lexer_init(&lex);
	lexer_start(&lex, str);
	parse_config_data(&(*config)->sections, &lex);
	lexer_free(&lex);

	return CONFIG_SUCCESS;
}

int config_open_defaults(config_t *config, const char *file)
{
	if (!config)
		return CONFIG_ERROR;

	return config_parse_file(&config->defaults, file, false);
}

int config_save(config_t *config)
{
	FILE *f;
	struct dstr str, tmp;
	int ret = CONFIG_ERROR;

	if (!config)
		return CONFIG_ERROR;
	if (!config->file)
		return CONFIG_ERROR;

	dstr_init(&str);
	dstr_init(&tmp);

	pthread_mutex_lock(&config->mutex);

	f = os_fopen(config->file, "wb");
	if (!f) {
		pthread_mutex_unlock(&config->mutex);
		return CONFIG_FILENOTFOUND;
	}

	struct config_section *section, *stmp;
	struct config_item *item, *itmp;

	int idx = 0;
	HASH_ITER (hh, config->sections, section, stmp) {
		if (idx++)
			dstr_cat(&str, "\n");

		dstr_cat(&str, "[");
		dstr_cat(&str, section->name);
		dstr_cat(&str, "]\n");

		HASH_ITER (hh, section->items, item, itmp) {
			dstr_copy(&tmp, item->value ? item->value : "");
			dstr_replace(&tmp, "\\", "\\\\");
			dstr_replace(&tmp, "\r", "\\r");
			dstr_replace(&tmp, "\n", "\\n");

			dstr_cat(&str, item->name);
			dstr_cat(&str, "=");
			dstr_cat(&str, tmp.array);
			dstr_cat(&str, "\n");
		}
	}

#ifdef _WIN32
	if (fwrite("\xEF\xBB\xBF", 3, 1, f) != 1)
		goto cleanup;
#endif
	if (fwrite(str.array, str.len, 1, f) != 1)
		goto cleanup;

	ret = CONFIG_SUCCESS;

cleanup:
	fclose(f);

	pthread_mutex_unlock(&config->mutex);

	dstr_free(&tmp);
	dstr_free(&str);

	return ret;
}

int config_save_safe(config_t *config, const char *temp_ext, const char *backup_ext)
{
	struct dstr temp_file = {0};
	struct dstr backup_file = {0};
	char *file = config->file;
	int ret;

	if (!temp_ext || !*temp_ext) {
		blog(LOG_ERROR, "config_save_safe: invalid "
				"temporary extension specified");
		return CONFIG_ERROR;
	}

	pthread_mutex_lock(&config->mutex);

	dstr_copy(&temp_file, config->file);
	if (*temp_ext != '.')
		dstr_cat(&temp_file, ".");
	dstr_cat(&temp_file, temp_ext);

	config->file = temp_file.array;
	ret = config_save(config);
	config->file = file;

	if (ret != CONFIG_SUCCESS) {
		blog(LOG_ERROR,
		     "config_save_safe: failed to "
		     "write to %s",
		     temp_file.array);
		goto cleanup;
	}

	if (backup_ext && *backup_ext) {
		dstr_copy(&backup_file, config->file);
		if (*backup_ext != '.')
			dstr_cat(&backup_file, ".");
		dstr_cat(&backup_file, backup_ext);
	}

	if (os_safe_replace(file, temp_file.array, backup_file.array) != 0)
		ret = CONFIG_ERROR;

cleanup:
	pthread_mutex_unlock(&config->mutex);
	dstr_free(&temp_file);
	dstr_free(&backup_file);
	return ret;
}

void config_close(config_t *config)
{
	struct config_section *section, *temp;

	if (!config)
		return;

	HASH_ITER (hh, config->sections, section, temp) {
		HASH_DELETE(hh, config->sections, section);
		config_section_free(section);
	}

	HASH_ITER (hh, config->defaults, section, temp) {
		HASH_DELETE(hh, config->defaults, section);
		config_section_free(section);
	}

	bfree(config->file);
	pthread_mutex_destroy(&config->mutex);
	bfree(config);
}

size_t config_num_sections(config_t *config)
{
	return HASH_CNT(hh, config->sections);
}

const char *config_get_section(config_t *config, size_t idx)
{
	struct config_section *section;
	struct config_section *temp;
	const char *name = NULL;
	size_t ctr = 0;

	pthread_mutex_lock(&config->mutex);

	if (idx >= config_num_sections(config))
		goto unlock;

	HASH_ITER (hh, config->sections, section, temp) {
		if (idx == ctr++) {
			name = section->name;
			break;
		}
	}

unlock:
	pthread_mutex_unlock(&config->mutex);
	return name;
}

static const struct config_item *config_find_item(const struct config_section *sections, const char *section,
						  const char *name)
{
	struct config_section *sec;
	struct config_item *res;

	HASH_FIND_STR(sections, section, sec);
	if (!sec)
		return NULL;

	HASH_FIND_STR(sec->items, name, res);

	return res;
}

static void config_set_item(config_t *config, struct config_section **sections, const char *section, const char *name,
			    char *value)
{
	struct config_section *sec;
	struct config_item *item;

	pthread_mutex_lock(&config->mutex);

	HASH_FIND_STR(*sections, section, sec);
	if (!sec) {
		sec = bzalloc(sizeof(struct config_section));
		sec->name = bstrdup(section);

		HASH_ADD_STR(*sections, name, sec);
	}

	HASH_FIND_STR(sec->items, name, item);
	if (!item) {
		item = bzalloc(sizeof(struct config_item));
		item->name = bstrdup(name);
		item->value = value;

		HASH_ADD_STR(sec->items, name, item);
	} else {
		bfree(item->value);
		item->value = value;
	}

	pthread_mutex_unlock(&config->mutex);
}

static void config_set_item_default(config_t *config, const char *section, const char *name, char *value)
{
	config_set_item(config, &config->defaults, section, name, value);

	if (!config_has_user_value(config, section, name))
		config_set_item(config, &config->sections, section, name, bstrdup(value));
}

void config_set_string(config_t *config, const char *section, const char *name, const char *value)
{
	if (!value)
		value = "";
	config_set_item(config, &config->sections, section, name, bstrdup(value));
}

void config_set_int(config_t *config, const char *section, const char *name, int64_t value)
{
	struct dstr str;
	dstr_init(&str);
	dstr_printf(&str, "%" PRId64, value);
	config_set_item(config, &config->sections, section, name, str.array);
}

void config_set_uint(config_t *config, const char *section, const char *name, uint64_t value)
{
	struct dstr str;
	dstr_init(&str);
	dstr_printf(&str, "%" PRIu64, value);
	config_set_item(config, &config->sections, section, name, str.array);
}

void config_set_bool(config_t *config, const char *section, const char *name, bool value)
{
	char *str = bstrdup(value ? "true" : "false");
	config_set_item(config, &config->sections, section, name, str);
}

void config_set_double(config_t *config, const char *section, const char *name, double value)
{
	char *str = bzalloc(64);
	os_dtostr(value, str, 64);
	config_set_item(config, &config->sections, section, name, str);
}

void config_set_default_string(config_t *config, const char *section, const char *name, const char *value)
{
	if (!value)
		value = "";
	config_set_item_default(config, section, name, bstrdup(value));
}

void config_set_default_int(config_t *config, const char *section, const char *name, int64_t value)
{
	struct dstr str;
	dstr_init(&str);
	dstr_printf(&str, "%" PRId64, value);
	config_set_item_default(config, section, name, str.array);
}

void config_set_default_uint(config_t *config, const char *section, const char *name, uint64_t value)
{
	struct dstr str;
	dstr_init(&str);
	dstr_printf(&str, "%" PRIu64, value);
	config_set_item_default(config, section, name, str.array);
}

void config_set_default_bool(config_t *config, const char *section, const char *name, bool value)
{
	char *str = bstrdup(value ? "true" : "false");
	config_set_item_default(config, section, name, str);
}

void config_set_default_double(config_t *config, const char *section, const char *name, double value)
{
	struct dstr str;
	dstr_init(&str);
	dstr_printf(&str, "%g", value);
	config_set_item_default(config, section, name, str.array);
}

const char *config_get_string(config_t *config, const char *section, const char *name)
{
	const struct config_item *item;
	const char *value = NULL;

	pthread_mutex_lock(&config->mutex);

	item = config_find_item(config->sections, section, name);
	if (!item)
		item = config_find_item(config->defaults, section, name);
	if (item)
		value = item->value;

	pthread_mutex_unlock(&config->mutex);
	return value;
}

static inline int64_t str_to_int64(const char *str)
{
	if (!str || !*str)
		return 0;

	if (str[0] == '0' && str[1] == 'x')
		return strtoll(str + 2, NULL, 16);
	else
		return strtoll(str, NULL, 10);
}

static inline uint64_t str_to_uint64(const char *str)
{
	if (!str || !*str)
		return 0;

	if (str[0] == '0' && str[1] == 'x')
		return strtoull(str + 2, NULL, 16);
	else
		return strtoull(str, NULL, 10);
}

int64_t config_get_int(config_t *config, const char *section, const char *name)
{
	const char *value = config_get_string(config, section, name);
	if (value)
		return str_to_int64(value);

	return 0;
}

uint64_t config_get_uint(config_t *config, const char *section, const char *name)
{
	const char *value = config_get_string(config, section, name);
	if (value)
		return str_to_uint64(value);

	return 0;
}

bool config_get_bool(config_t *config, const char *section, const char *name)
{
	const char *value = config_get_string(config, section, name);
	if (value)
		return astrcmpi(value, "true") == 0 || !!str_to_uint64(value);

	return false;
}

double config_get_double(config_t *config, const char *section, const char *name)
{
	const char *value = config_get_string(config, section, name);
	if (value)
		return os_strtod(value);

	return 0.0;
}

bool config_remove_value(config_t *config, const char *section, const char *name)
{
	struct config_section *sec;
	struct config_item *item;
	bool success = false;

	pthread_mutex_lock(&config->mutex);

	HASH_FIND_STR(config->sections, section, sec);
	if (sec) {
		HASH_FIND_STR(sec->items, name, item);
		if (item) {
			HASH_DELETE(hh, sec->items, item);
			config_item_free(item);
			success = true;
		}
	}

	pthread_mutex_unlock(&config->mutex);
	return success;
}

const char *config_get_default_string(config_t *config, const char *section, const char *name)
{
	const struct config_item *item;
	const char *value = NULL;

	pthread_mutex_lock(&config->mutex);

	item = config_find_item(config->defaults, section, name);
	if (item)
		value = item->value;

	pthread_mutex_unlock(&config->mutex);
	return value;
}

int64_t config_get_default_int(config_t *config, const char *section, const char *name)
{
	const char *value = config_get_default_string(config, section, name);
	if (value)
		return str_to_int64(value);

	return 0;
}

uint64_t config_get_default_uint(config_t *config, const char *section, const char *name)
{
	const char *value = config_get_default_string(config, section, name);
	if (value)
		return str_to_uint64(value);

	return 0;
}

bool config_get_default_bool(config_t *config, const char *section, const char *name)
{
	const char *value = config_get_default_string(config, section, name);
	if (value)
		return astrcmpi(value, "true") == 0 || !!str_to_uint64(value);

	return false;
}

double config_get_default_double(config_t *config, const char *section, const char *name)
{
	const char *value = config_get_default_string(config, section, name);
	if (value)
		return os_strtod(value);

	return 0.0;
}

bool config_has_user_value(config_t *config, const char *section, const char *name)
{
	bool success;
	pthread_mutex_lock(&config->mutex);
	success = config_find_item(config->sections, section, name) != NULL;
	pthread_mutex_unlock(&config->mutex);
	return success;
}

bool config_has_default_value(config_t *config, const char *section, const char *name)
{
	bool success;
	pthread_mutex_lock(&config->mutex);
	success = config_find_item(config->defaults, section, name) != NULL;
	pthread_mutex_unlock(&config->mutex);
	return success;
}
