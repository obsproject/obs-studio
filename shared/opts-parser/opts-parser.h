#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct obs_option {
	char *name;
	char *value;
};

struct obs_options {
	size_t count;
	struct obs_option *options;
	size_t ignored_word_count;
	char **ignored_words;
	char **input_words;
};

struct obs_options obs_parse_options(const char *options_string);
void obs_free_options(struct obs_options options);

#ifdef __cplusplus
}
#endif
