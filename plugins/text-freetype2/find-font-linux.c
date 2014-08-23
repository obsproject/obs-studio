/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <fontconfig/fontconfig.h>

#include <util/darray.h>

#include "find-font.h"
#include "text-freetype2.h"

static inline void add_font(const char *path)
{
	FT_Face face;
	FT_Long idx = 0;
	FT_Long max_faces = 1;

	while (idx < max_faces) {
		if (FT_New_Face(ft2_lib, path, idx, &face) != 0)
			break;

		build_font_path_info(face, idx++, path);
		max_faces = face->num_faces;
		FT_Done_Face(face);
	}
}

void load_os_font_list(void)
{
	FcPattern *pat = FcPatternCreate();
	FcObjectSet *os = FcObjectSetCreate();
	FcObjectSetAdd(os, FC_FILE);

	FcFontSet *set = FcFontList(0, pat, os);

	for (int i = 0; i < set->nfont; ++i) {
		add_font((const char *) FcPatternFormat(
			set->fonts[i], (const FcChar8 *) "%{file}"));
	}

	FcFontSetDestroy(set);
	FcObjectSetDestroy(os);
	FcPatternDestroy(pat);
}
