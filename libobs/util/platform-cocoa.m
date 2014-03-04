/*
 * Copyright (c) 2013 Ruwen Hahn <palana@stunned.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "base.h"
#include "platform.h"
#include "dstr.h"

#include <dlfcn.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>

#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#import <Cocoa/Cocoa.h>

void *os_dlopen(const char *path)
{
	struct dstr dylib_name;

	if (!path)
		return NULL;

	dstr_init_copy(&dylib_name, path);
	if (!dstr_find(&dylib_name, ".so"))
		dstr_cat(&dylib_name, ".so");

	void *res = dlopen(dylib_name.array, RTLD_LAZY);
	if (!res)
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

bool os_sleepto_ns(uint64_t time_target)
{
	uint64_t current = os_gettime_ns();
	if (time_target < current)
		return false;

	time_target -= current;

	struct timespec req, remain;
	memset(&req, 0, sizeof(req));
	memset(&remain, 0, sizeof(remain));
	req.tv_sec = time_target/1000000000;
	req.tv_nsec = time_target%1000000000;

	while (nanosleep(&req, &remain)) {
		req = remain;
		memset(&remain, 0, sizeof(remain));
	}

	return true;
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

/* gets the location ~/Library/Application Support/[name] */
char *os_get_config_path(const char *name)
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(
			NSApplicationSupportDirectory, NSUserDomainMask, YES);

	if([paths count] == 0)
		bcrash("Could not get home directory (platform-cocoa)");

	NSString *application_support = paths[0];

	NSUInteger len = [application_support
		lengthOfBytesUsingEncoding:NSUTF8StringEncoding];

	char *path_ptr = bmalloc(len+1);

	path_ptr[len] = 0;

	memcpy(path_ptr, [application_support UTF8String], len);

	struct dstr path;
	dstr_init_move_array(&path, path_ptr);
	dstr_cat(&path, "/");
	dstr_cat(&path, name);
	return path.array;
}

bool os_file_exists(const char *path)
{
	return access(path, F_OK) == 0;
}

int os_mkdir(const char *path)
{
	if(!mkdir(path, 0777))
		return MKDIR_SUCCESS;

	if(errno == EEXIST)
		return MKDIR_EXISTS;

	return MKDIR_ERROR;
}
