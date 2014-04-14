/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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
#include "obs-internal.h"

static inline bool check_path(const char* data, const char *path,
		struct dstr * output)
{
	dstr_copy(output, path);
	dstr_cat(output, data);

	blog(LOG_DEBUG, "Attempting path: %s\n", output->array);

	return os_file_exists(output->array);
}

static inline bool check_lib_path(const char* data, const char *path,
		struct dstr *output)
{
	bool result = false;
	struct dstr tmp;

	dstr_init_copy(&tmp, data);
	dstr_cat(&tmp, ".dll");
	result = check_path(tmp.array, path, output);

	dstr_free(&tmp);

	return result;
}

/* on windows, plugin files are located in [base directory]/plugins/[bit] */
char *find_plugin(const char *plugin)
{
	struct dstr path;
	dstr_init(&path);

#ifdef _WIN64
	if (check_lib_path(plugin, "obs-plugins/64bit/", &path))
#else
	if (check_lib_path(plugin, "obs-plugins/32bit/", &path))
#endif
		return path.array;

#ifdef _WIN64
	if (check_lib_path(plugin, "../../obs-plugins/64bit/", &path))
#else
	if (check_lib_path(plugin, "../../obs-plugins/32bit/", &path))
#endif
		return path.array;

	dstr_free(&path);
	return NULL;
}

/* on windows, points to [base directory]/libobs */
char *find_libobs_data_file(const char *file)
{
	struct dstr path;
	dstr_init(&path);

	if (check_path(file, "data/libobs/", &path))
		return path.array;

	if (check_path(file, "../../data/libobs/", &path))
		return path.array;

	dstr_free(&path);
	return NULL;
}

/* on windows, data files should always be in [base directory]/data */
char *obs_find_plugin_file(const char *file)
{
	struct dstr path;
	dstr_init(&path);

	if (check_path(file, "data/obs-plugins/", &path))
		return path.array;

	if (check_path(file, "../../data/obs-plugins/", &path))
		return path.array;

	dstr_free(&path);
	return NULL;
}
