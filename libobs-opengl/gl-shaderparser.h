/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef GL_SHADER_PARSER_H
#define GL_SHADER_PARSER_H

/*
 *   Parses shaders into GLSL.  Shaders are almost identical to HLSL
 * model 5 so it requires quite a bit of tweaking to convert correctly.
 * Takes the parsed shader data, and builds a GLSL string out of it.
 */

#include "util/dstr.h"
#include "graphics/shader-parser.h"

struct gl_shader_parser {
	struct dstr          gl_string;
	struct shader_parser parser;
};

static inline void gl_shader_parser_init(struct gl_shader_parser *glsp)
{
	shader_parser_init(&glsp->parser);
	dstr_init(&glsp->gl_string);
}

static inline void gl_shader_parser_free(struct gl_shader_parser *glsp)
{
	dstr_free(&glsp->gl_string);
	shader_parser_free(&glsp->parser);
}

extern bool gl_shader_parse(struct gl_shader_parser *glsp,
		const char *shader_str, const char *file);

#endif
