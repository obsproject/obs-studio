/******************************************************************************
  Copyright (c) 2013 by Ruwen Hahn <palana@stunned.de>

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

#include "base.h"
#include "platform.h"
#include "dstr.h"

#include <dlfcn.h>
#include <time.h>
#include <unistd.h>

#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

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
	uint64_t t = mach_absolute_time();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"	
	Nanoseconds nano = AbsoluteToNanoseconds(*(AbsoluteTime*) &t);
#pragma clang diagnostic pop
	return *(uint64_t*) &nano;
}

uint64_t os_gettime_ms(void)
{
	return  os_gettime_ns()/1000000;
}

