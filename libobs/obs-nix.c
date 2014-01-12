/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>

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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "util/dstr.h"
#include "obs.h"

static inline bool check_path(const char* data, const char *path, struct dstr * output)
{
	dstr_copy(output, path);
	dstr_cat(output, data);
	
    blog(LOG_INFO, "Attempting path: %s\n", output->array);
	
	return access(output->array, R_OK) == 0;
}

static inline bool check_lib_path(const char* data, const char *path, struct dstr *output)
{
	bool result = false;
	struct dstr tmp;
	
    dstr_init_copy(&tmp, "lib");
    dstr_cat(&tmp, data);
	dstr_cat(&tmp, ".so");
	result = check_path(tmp.array, path, output); 
	
	dstr_free(&tmp);
	
	return result;
}

/*
 *   /usr/local/lib/obs-plugins
 *   /usr/lib/obs-plugins
 */
char *find_plugin(const char *plugin)
{ 
	struct dstr output;
	dstr_init(&output);

	if (check_lib_path(plugin, "/usr/local/lib/obs-plugins/", &output))
		return output.array;

	if (check_lib_path(plugin, "/usr/lib/obs-plugins/", &output))
		return output.array;

	dstr_free(&output);
	return NULL;
}

/*
 *   /usr/local/share/libobs
 *   /usr/share/libobs
 */
char *find_libobs_data_file(const char *file)
{
	struct dstr output;
		dstr_init(&output);

	if (check_path(file, "/usr/local/share/libobs/", &output))
		return output.array;

	if (check_path(file, "/usr/share/libobs/", &output))
		return output.array;

	dstr_free(&output);
	return NULL;
}

/*
 *   /usr/local/share/obs-plugins
 *   /usr/share/obs-plugins
 */
char *obs_find_plugin_file(const char *file)
{ 	
    struct dstr output;
    dstr_init(&output);

    if (check_path(file, "/usr/local/share/obs-plugins/", &output))
        return output.array;

    if (check_path(file, "/usr/share/obs-plugins", &output))
        return output.array;

    dstr_free(&output);
    return NULL;
}
