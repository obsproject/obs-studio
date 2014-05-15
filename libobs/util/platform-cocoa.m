/*
 * Copyright (c) 2013-2014 Ruwen Hahn <palana@stunned.de>
 *                         Hugh "Jim" Bailey <obs.jim@gmail.com>
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

#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#import <Cocoa/Cocoa.h>

/* clock function selection taken from libc++ */
static uint64_t ns_time_simple()
{
	return mach_absolute_time();
}

static double ns_time_compute_factor()
{
	mach_timebase_info_data_t info = {1, 1};
	mach_timebase_info(&info);
	return ((double)info.numer) / info.denom;
}

static uint64_t ns_time_full()
{
	static double factor = 0.;
	if (factor == 0.) factor = ns_time_compute_factor();
	return (uint64_t)(mach_absolute_time() * factor);
}

typedef uint64_t (*time_func)();

static time_func ns_time_select_func()
{
	mach_timebase_info_data_t info = {1, 1};
	mach_timebase_info(&info);
	if (info.denom == info.numer)
		return ns_time_simple;
	return ns_time_full;
}

uint64_t os_gettime_ns(void)
{
	static time_func f = NULL;
	if (!f) f = ns_time_select_func();
	return f();
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
