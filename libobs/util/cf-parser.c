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

#include "cf-parser.h"

void cf_adderror(struct cf_parser *p, const char *error, int level,
		const char *val1, const char *val2, const char *val3)
{
	uint32_t row, col;
	lexer_getstroffset(&p->cur_token->lex->base_lexer,
			p->cur_token->unmerged_str.array,
			&row, &col);

	if (!val1 && !val2 && !val3) {
		error_data_add(&p->error_list, p->cur_token->lex->file,
				row, col, error, level);
	} else {
		struct dstr formatted;
		dstr_init(&formatted);
		dstr_safe_printf(&formatted, error, val1, val2, val3, NULL);

		error_data_add(&p->error_list, p->cur_token->lex->file,
				row, col, formatted.array, level);

		dstr_free(&formatted);
	}
}

bool pass_pair(struct cf_parser *p, char in, char out)
{
	if (p->cur_token->type != CFTOKEN_OTHER ||
	    *p->cur_token->str.array != in)
		return p->cur_token->type != CFTOKEN_NONE;

	p->cur_token++;

	while (p->cur_token->type != CFTOKEN_NONE) {
		if (*p->cur_token->str.array == in) {
			if (!pass_pair(p, in, out))
				break;
			continue;

		} else if(*p->cur_token->str.array == out) {
			p->cur_token++;
			return true;
		}

		p->cur_token++;
	}

	return false;
}
