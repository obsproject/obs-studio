#pragma once

#include <stddef.h>

struct obs_x265_option {
	char *name;
	char *value;
};

struct obs_x265_options {
	size_t count;
	struct obs_x265_option *options;
	size_t ignored_word_count;
	char **ignored_words;
	char **input_words;
};

struct obs_x265_options obs_x265_parse_options(const char *options_string);
void obs_x265_free_options(struct obs_x265_options options);
