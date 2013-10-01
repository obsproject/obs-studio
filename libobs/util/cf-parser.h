/******************************************************************************
  Copyright (c) 2013 by Hugh Bailey <obs.jim@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

     1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

     2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

     3. This notice may not be removed or altered from any source
     distribution.
******************************************************************************/

#ifndef CF_PARSER_H
#define CF_PARSER_H

#include "cf-lexer.h"

/*
 * C-family parser
 *
 *   Handles preprocessing/lexing/errors when parsing a file, and provides a
 * set of parsing functions to be able to go through all the resulting tokens
 * more easily.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define PARSE_SUCCESS              0
#define PARSE_CONTINUE            -1
#define PARSE_BREAK               -2
#define PARSE_UNEXPECTED_CONTINUE -3
#define PARSE_UNEXPECTED_BREAK    -4
#define PARSE_EOF                 -5

struct cf_parser {
	struct cf_lexer        lex;
	struct cf_preprocessor pp;
	struct error_data      error_list;

	struct cf_token        *cur_token;
};

static inline void cf_parser_init(struct cf_parser *parser)
{
	cf_lexer_init(&parser->lex);
	cf_preprocessor_init(&parser->pp);
	error_data_init(&parser->error_list);

	parser->cur_token = NULL;
}

static inline void cf_parser_free(struct cf_parser *parser)
{
	cf_lexer_free(&parser->lex);
	cf_preprocessor_free(&parser->pp);
	error_data_free(&parser->error_list);

	parser->cur_token = NULL;
}

static inline bool cf_parser_parse(struct cf_parser *parser,
		const char *str, const char *file)
{
	if (!cf_lexer_lex(&parser->lex, str, file))
		return false;

	if (!cf_preprocess(&parser->pp, &parser->lex, &parser->error_list))
		return false;

	parser->cur_token = cf_preprocessor_gettokens(&parser->pp);
	return true;
}

EXPORT void cf_adderror(struct cf_parser *parser, const char *error,
		int level, const char *val1, const char *val2,
		const char *val3);

static inline void cf_adderror_expecting(struct cf_parser *p,
		const char *expected)
{
	cf_adderror(p, "Expected $1", LEVEL_ERROR, expected, NULL, NULL);
}

static inline void cf_adderror_unexpected_eof(struct cf_parser *p)
{
	cf_adderror(p, "Unexpected end of file", LEVEL_ERROR,
			NULL, NULL, NULL);
}

static inline void cf_adderror_syntax_error(struct cf_parser *p)
{
	cf_adderror(p, "Syntax error", LEVEL_ERROR,
			NULL, NULL, NULL);
}

static inline bool next_token(struct cf_parser *p)
{
	if (p->cur_token->type != CFTOKEN_SPACETAB &&
	    p->cur_token->type != CFTOKEN_NEWLINE &&
	    p->cur_token->type != CFTOKEN_NONE)
		p->cur_token++;

	while (p->cur_token->type == CFTOKEN_SPACETAB ||
	       p->cur_token->type == CFTOKEN_NEWLINE)
		p->cur_token++;

	return p->cur_token->type != CFTOKEN_NONE;
}

static inline bool next_valid_token(struct cf_parser *p)
{
	if (!next_token(p)) {
		cf_adderror_unexpected_eof(p);
		return false;
	}

	return true;
}

EXPORT bool pass_pair(struct cf_parser *p, char in, char out);

static inline bool go_to_token(struct cf_parser *p,
		const char *str1, const char *str2)
{
	while (next_token(p)) {
		if (strref_cmp(&p->cur_token->str, str1) == 0) {
			return true;
		} else if (str2 && strref_cmp(&p->cur_token->str, str2) == 0) {
			return true;
		} else if (*p->cur_token->str.array == '{') {
			if (!pass_pair(p, '{', '}'))
				break;
		}
	}

	return false;
}

static inline bool go_to_valid_token(struct cf_parser *p,
		const char *str1, const char *str2)
{
	if (!go_to_token(p, str1, str2)) {
		cf_adderror_unexpected_eof(p);
		return false;
	}

	return true;
}

static inline bool go_to_token_type(struct cf_parser *p,
		enum cf_token_type type)
{
	while (p->cur_token->type != CFTOKEN_NONE &&
	       p->cur_token->type != CFTOKEN_NEWLINE)
		p->cur_token++;

	return p->cur_token->type != CFTOKEN_NONE;
}

static inline int next_token_should_be(struct cf_parser *p,
		const char *str, const char *goto1, const char *goto2)
{
	if (!next_token(p)) {
		cf_adderror_unexpected_eof(p);
		return PARSE_EOF;
	} else if (strref_cmp(&p->cur_token->str, str) == 0) {
		return PARSE_SUCCESS;
	}

	if (goto1)
		go_to_token(p, goto1, goto2);

	cf_adderror_expecting(p, str);
	return PARSE_CONTINUE;
}

static inline bool peek_token(struct cf_parser *p, struct cf_token *peek)
{
	struct cf_token *cur_token = p->cur_token;
	bool success = next_token(p);

	*peek = *p->cur_token;
	p->cur_token = cur_token;

	return success;
}

static inline bool peek_valid_token(struct cf_parser *p,
		struct cf_token *peek)
{
	bool success = peek_token(p, peek);
	if (!success)
		cf_adderror_unexpected_eof(p);
	return success;
}

static inline bool token_is(struct cf_parser *p, const char *val)
{
	return strref_cmp(&p->cur_token->str, val) == 0;
}

static inline int token_is_type(struct cf_parser *p,
		enum cf_token_type type, const char *type_expected,
		const char *goto_token)
{
	if (p->cur_token->type != type) {
		cf_adderror_expecting(p, type_expected);
		if (goto_token) {
			if (!go_to_valid_token(p, goto_token, NULL))
				return PARSE_EOF;
		}
		return PARSE_CONTINUE;
	}

	return PARSE_SUCCESS;
}

static inline void copy_token(struct cf_parser *p, char **dst)
{
	*dst = bstrdup_n(p->cur_token->str.array, p->cur_token->str.len);
}

static inline int get_name(struct cf_parser *p, char **dst,
		const char *name, const char *goto_token)
{
	int errcode;

	errcode = token_is_type(p, CFTOKEN_NAME, name, goto_token);
	if (errcode != PARSE_SUCCESS)
		return errcode;

	*dst = bstrdup_n(p->cur_token->str.array, p->cur_token->str.len);
	return PARSE_SUCCESS;
}

static inline int next_name(struct cf_parser *p, char **dst,
		const char *name, const char *goto_token)
{
	if (!next_valid_token(p))
		return PARSE_EOF;

	return get_name(p, dst, name, goto_token);
}

static inline int get_name_ref(struct cf_parser *p, struct strref *dst,
		const char *name, const char *goto_token)
{
	int errcode;

	errcode = token_is_type(p, CFTOKEN_NAME, name, goto_token);
	if (errcode != PARSE_SUCCESS)
		return errcode;

	strref_copy(dst, &p->cur_token->str);
	return PARSE_SUCCESS;
}

static inline int next_name_ref(struct cf_parser *p, struct strref *dst,
		const char *name, const char *goto_token)
{
	if (!next_valid_token(p))
		return PARSE_EOF;

	return get_name_ref(p, dst, name, goto_token);
}

#ifdef __cplusplus
}
#endif

#endif
