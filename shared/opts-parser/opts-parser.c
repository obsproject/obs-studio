#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include "opts-parser.h"

static bool getparam(const char *param, char **name, const char **value)
{
	const char *assign;

	if (!param || !*param || (*param == '='))
		return false;

	assign = strchr(param, '=');
	if (!assign || !*assign || !*(assign + 1))
		return false;

	*name = bstrdup_n(param, assign - param);
	*value = assign + 1;
	return true;
}

struct obs_options obs_parse_options(const char *options_string)
{
	if (!options_string || !*options_string)
		goto failure;

	char **input_words = strlist_split(options_string, ' ', false);
	if (!input_words)
		goto failure;

	size_t input_option_count = 0;
	for (char **input_word = input_words; *input_word; ++input_word)
		input_option_count += 1;
	char **ignored_words =
		bmalloc(input_option_count * sizeof(*ignored_words));
	char **ignored_word = ignored_words;
	struct obs_option *out_options =
		bmalloc(input_option_count * sizeof(*out_options));
	struct obs_option *out_option = out_options;
	for (char **input_word = input_words; *input_word; ++input_word) {
		if (getparam(*input_word, &out_option->name,
			     (const char **)&out_option->value)) {
			++out_option;
		} else {
			*ignored_word = *input_word;
			++ignored_word;
		}
	}
	return (struct obs_options){
		.count = out_option - out_options,
		.options = out_options,
		.ignored_word_count = ignored_word - ignored_words,
		.ignored_words = ignored_words,
		.input_words = input_words,
	};

failure:
	return (struct obs_options){
		.count = 0,
		.options = NULL,
		.ignored_word_count = 0,
		.ignored_words = NULL,
		.input_words = NULL,
	};
}

void obs_free_options(struct obs_options options)
{
	for (size_t i = 0; i < options.count; ++i) {
		bfree(options.options[i].name);
	}
	bfree(options.options);
	bfree(options.ignored_words);
	strlist_free(options.input_words);
}
