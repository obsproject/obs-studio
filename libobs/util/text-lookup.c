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

#include <ctype.h>

#include "dstr.h"
#include "text-lookup.h"
#include "lexer.h"
#include "platform.h"
#include "uthash.h"

/* ------------------------------------------------------------------------- */

struct text_item {
	char *lookup, *value;
	UT_hash_handle hh;
};

static inline void text_item_destroy(struct text_item *item)
{
	bfree(item->lookup);
	bfree(item->value);
	bfree(item);
}

/* ------------------------------------------------------------------------- */

struct text_lookup {
	struct dstr language;
	struct text_item *items;
};

static void lookup_getstringtoken(struct lexer *lex, struct strref *token)
{
	const char *temp = lex->offset;
	bool was_backslash = false;

	while (*temp != 0 && *temp != '\n') {
		if (!was_backslash) {
			if (*temp == '\\') {
				was_backslash = true;
			} else if (*temp == '"') {
				temp++;
				break;
			}
		} else {
			was_backslash = false;
		}

		++temp;
	}

	token->len += (size_t)(temp - lex->offset);

	if (*token->array == '"') {
		token->array++;
		token->len--;

		if (*(temp - 1) == '"')
			token->len--;
	}

	lex->offset = temp;
}

static bool lookup_gettoken(struct lexer *lex, struct strref *str)
{
	struct base_token temp;

	base_token_clear(&temp);
	strref_clear(str);

	while (lexer_getbasetoken(lex, &temp, PARSE_WHITESPACE)) {
		char ch = *temp.text.array;

		if (!str->array) {
			/* comments are designated with a #, and end at LF */
			if (ch == '#') {
				while (ch != '\n' && ch != 0)
					ch = *(++lex->offset);
			} else if (temp.type == BASETOKEN_WHITESPACE) {
				strref_copy(str, &temp.text);
				break;
			} else {
				strref_copy(str, &temp.text);
				if (ch == '"') {
					lookup_getstringtoken(lex, str);
					break;
				} else if (ch == '=') {
					break;
				}
			}
		} else {
			if (temp.type == BASETOKEN_WHITESPACE ||
			    *temp.text.array == '=') {
				lex->offset -= temp.text.len;
				break;
			}

			if (ch == '#') {
				lex->offset--;
				break;
			}

			str->len += temp.text.len;
		}
	}

	return (str->len != 0);
}

static inline bool lookup_goto_nextline(struct lexer *p)
{
	struct strref val;
	bool success = true;

	strref_clear(&val);

	while (true) {
		if (!lookup_gettoken(p, &val)) {
			success = false;
			break;
		}
		if (*val.array == '\n')
			break;
	}

	return success;
}

static char *convert_string(const char *str, size_t len)
{
	struct dstr out;
	out.array = bstrdup_n(str, len);
	out.capacity = len + 1;
	out.len = len;

	dstr_replace(&out, "\\n", "\n");
	dstr_replace(&out, "\\t", "\t");
	dstr_replace(&out, "\\r", "\r");
	dstr_replace(&out, "\\\"", "\"");

	return out.array;
}

static void lookup_addfiledata(struct text_lookup *lookup,
			       const char *file_data)
{
	struct lexer lex;
	struct strref name, value;

	lexer_init(&lex);
	lexer_start(&lex, file_data);
	strref_clear(&name);
	strref_clear(&value);

	while (lookup_gettoken(&lex, &name)) {
		struct text_item *item;
		struct text_item *old;
		bool got_eq = false;

		if (*name.array == '\n')
			continue;
	getval:
		if (!lookup_gettoken(&lex, &value))
			break;
		if (*value.array == '\n')
			continue;
		else if (!got_eq && *value.array == '=') {
			got_eq = true;
			goto getval;
		}

		item = bzalloc(sizeof(struct text_item));
		item->lookup = bstrdup_n(name.array, name.len);
		item->value = convert_string(value.array, value.len);

		HASH_REPLACE_STR(lookup->items, lookup, item, old);

		if (old)
			text_item_destroy(old);

		if (!lookup_goto_nextline(&lex))
			break;
	}

	lexer_free(&lex);
}

static inline bool lookup_getstring(const char *lookup_val, const char **out,
				    struct text_lookup *lookup)
{
	struct text_item *item;

	if (!lookup->items)
		return false;

	HASH_FIND_STR(lookup->items, lookup_val, item);

	if (!item)
		return false;

	*out = item->value;
	return true;
}

/* ------------------------------------------------------------------------- */

lookup_t *text_lookup_create(const char *path)
{
	struct text_lookup *lookup = bzalloc(sizeof(struct text_lookup));

	if (!text_lookup_add(lookup, path)) {
		bfree(lookup);
		lookup = NULL;
	}

	return lookup;
}

bool text_lookup_add(lookup_t *lookup, const char *path)
{
	struct dstr file_str;
	char *temp = NULL;
	FILE *file;

	file = os_fopen(path, "rb");
	if (!file)
		return false;

	os_fread_utf8(file, &temp);
	dstr_init_move_array(&file_str, temp);
	fclose(file);

	if (!file_str.array)
		return false;

	dstr_replace(&file_str, "\r", " ");
	lookup_addfiledata(lookup, file_str.array);
	dstr_free(&file_str);

	return true;
}

void text_lookup_destroy(lookup_t *lookup)
{
	if (lookup) {
		struct text_item *item, *tmp;
		HASH_ITER (hh, lookup->items, item, tmp) {
			HASH_DELETE(hh, lookup->items, item);
			text_item_destroy(item);
		}

		dstr_free(&lookup->language);
		bfree(lookup);
	}
}

bool text_lookup_getstr(lookup_t *lookup, const char *lookup_val,
			const char **out)
{
	if (lookup)
		return lookup_getstring(lookup_val, out, lookup);
	return false;
}
