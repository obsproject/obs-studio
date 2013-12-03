/******************************************************************************
    Copyright (C) 2013 by Ruwen Hahn <palana@stunned.de>

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
******************************************************************************/

#include "util/platform.h"
#include "util/dstr.h"
#include "obs.h"
#include "obs-data.h"

#include <unistd.h>

// support both foo.so and libfoo.so for now
static const char *plugin_patterns[] = {
	"../plugins/%s.so",
	"../plugins/lib%s.so"
};

static const int plugin_patterns_size =
	sizeof(plugin_patterns)/sizeof(plugin_patterns[0]);

char *find_plugin(const char *plugin)
{
	struct dstr path;
	dstr_init(&path);
	for(int i = 0; i < plugin_patterns_size; i++) {
		dstr_printf(&path, plugin_patterns[i], plugin);
		if(!access(path.array, F_OK))
			break;
	}

	return path.array;
}

char *find_libobs_data_file(const char *file)
{
	struct dstr path;
	dstr_init_copy(&path, "../libobs/");
	dstr_cat(&path, file);
	return path.array;
}

char *obs_find_plugin_file(const char *file)
{
	struct dstr path;
	dstr_init_copy(&path, "../data/");
	dstr_cat(&path, file);
	return path.array;
}
