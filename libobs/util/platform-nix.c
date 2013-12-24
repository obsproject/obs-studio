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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <time.h>

#include "dstr.h"
#include "platform.h"

void *os_dlopen(const char *path)
{
	struct dstr dylib_name;
	dstr_init_copy(&dylib_name, path);
	if(!dstr_find(&dylib_name, ".so"))
		dstr_cat(&dylib_name, ".so");

	void *res = dlopen(dylib_name.array, RTLD_LAZY);
	if(!res)
		blog(LOG_ERROR, "os_dlopen(%s->%s): %s\n",
				path, dylib_name.array, dlerror());

	dstr_free(&dylib_name);
	return res;
}

void *os_dlsym(void *module, const char *func)
{
	return dlsym(module, func);
}

void os_dlclose(void *module)
{
	dlclose(module);
}

void os_sleepto_ns(uint64_t time_target)
{
	uint64_t current = os_gettime_ns();
	if(time_target < current)
		return;
	time_target -= current;
	struct timespec req,
			remain;
	memset(&req, 0, sizeof(req));
	memset(&remain, 0, sizeof(remain));
	req.tv_sec = time_target/1000000000;
	req.tv_nsec = time_target%1000000000;
	while(nanosleep(&req, &remain))
	{
		req = remain;
		memset(&remain, 0, sizeof(remain));
	}
}

void os_sleep_ms(uint32_t duration)
{
	usleep(duration*1000);
}

uint64_t os_gettime_ns(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return tp.tv_nsec;
}

/* should return $HOME/.[name] */
char *os_get_config_path(const char *name)
{
	char *path_ptr = getenv("HOME");
	if (path_ptr == NULL)
		bcrash("Could not get $HOME\n");

	struct dstr path;
	dstr_init_copy(&path, path_ptr);
	dstr_cat(&path, ".");
	dstr_cat(&path, name);
	return path.array;
}

bool os_file_exists(const char *path)
{
	return access(path, F_OK) == 0;
}

int os_mkdir(const char *path)
{
	int errorcode = mkdir(path, S_IRWXU);
	if (errorcode == 0)
		return MKDIR_SUCCESS;

	return (errno == EEXIST) ? MKDIR_EXISTS : MKDIR_ERROR;
}
