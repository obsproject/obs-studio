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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "c99defs.h"
#include "base.h"

static enum log_type log_output_level = LOG_WARNING;
static int  crashing = 0;

static void def_log_handler(enum log_type type, const char *format,
		va_list args)
{
	char out[4096];
	vsnprintf(out, sizeof(out), format, args);

	if (type >= log_output_level) {
		switch (type) {
		case LOG_DEBUG:
			printf("debug: %s\n", out);
			break;

		case LOG_INFO:
			printf("info: %s\n", out);
			break;

		case LOG_WARNING:
			fprintf(stderr, "warning: %s\n", out);
			break;

		case LOG_ERROR:
			fprintf(stderr, "error: %s\n", out);
		}
	}
}

static void def_crash_handler(const char *format, va_list args)
{
	vfprintf(stderr, format, args);
	exit(0);
}

static void (*log_handler)(enum log_type, const char *, va_list) =
		def_log_handler;
static void (*crash_handler)(const char *, va_list) = def_crash_handler;

void base_set_log_handler(
	void (*handler)(enum log_type, const char *, va_list))
{
	log_handler = handler;
}

void base_set_crash_handler(void (*handler)(const char *, va_list))
{
	crash_handler = handler;
}

void bcrash(const char *format, ...)
{
	va_list args;

	if (crashing) {
		fputs("Crashed in the crash handler", stderr);
		exit(2);
	}

	crashing = 1;
	va_start(args, format);
	crash_handler(format, args);
	va_end(args);
}

void blog(enum log_type type, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	log_handler(type, format, args);
	va_end(args);
}
