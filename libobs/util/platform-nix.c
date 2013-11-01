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

#include <stdlib.h>

#include "dstr.h"
#include "platform.h"

void *os_dlopen(const char *path)
{
	/* TODO */
	return NULL;
}

void *os_dlsym(void *module, const char *func)
{
	/* TODO */
	return NULL;
}

void os_dlclose(void *module)
{
	/* TODO */
}

void os_sleepto_ns(uint64_t time_target)
{
	/* TODO */
}

void os_sleep_ms(uint32_t duration)
{
	/* TODO */
}

uint64_t os_gettime_ns(void)
{
	/* TODO */
	return 0;
}

uint64_t os_gettime_ms(void)
{
	/* TODO */
	return 0;
}

/* should return $HOME/ */
char *os_get_home_path(void)
{
	/* TODO */
	return NULL;
}
