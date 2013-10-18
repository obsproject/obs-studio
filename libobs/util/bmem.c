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

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "base.h"
#include "bmem.h"

/*
 * NOTE: totally jacked the mem alignment trick from ffmpeg, credit to them:
 *   http://www.ffmpeg.org/
 */

#define ALIGNMENT 16

#if defined(_WIN32) && !defined(_WIN64)
#define ALIGNED_MALLOC 1
#elif !defined(__LP64__)
#define ALIGNMENT_HACK 1
#endif

static void *a_malloc(size_t size)
{
#ifdef ALIGNED_MALLOC
	return _aligned_malloc(size, ALIGNMENT);
#elif ALIGNMENT_HACK
	void *ptr = NULL;
	long diff;

	ptr  = malloc(size + ALIGNMENT);
	diff = ((~(long)ptr) & (ALIGNMENT - 1)) + 1;
	ptr  = (char *)ptr + diff;
	((char *)ptr)[-1] = (char)diff;

	return ptr;
#else
	return malloc(size);
#endif
}

static void *a_realloc(void *ptr, size_t size)
{
#ifdef ALIGNED_MALLOC
	return _aligned_realloc(ptr, size, ALIGNMENT);
#elif ALIGNMENT_HACK
	long diff;

	if (!ptr)
		return a_malloc(size);
	diff = ((char *)ptr)[-1];
	ptr = realloc((char*)ptr - diff, size + diff);
	if (ptr)
		ptr = (char *)ptr + diff;
	return ptr;
#else
	return realloc(ptr, size);
#endif
}

static void a_free(void *ptr)
{
#ifdef ALIGNED_MALLOC
	_aligned_free(ptr);
#elif ALIGNMENT_HACK
	if (ptr)
		free((char *)ptr - ((char*)ptr)[-1]);
#else
	free(ptr);
#endif
}

static struct base_allocator alloc = {a_malloc, a_realloc, a_free};
static size_t num_allocs = 0;

void base_set_allocator(struct base_allocator *defs)
{
	memcpy(&alloc, defs, sizeof(struct base_allocator));
}

void *bmalloc(size_t size)
{
	void *ptr = alloc.malloc(size);
	if (!ptr && !size)
		ptr = alloc.malloc(1);
	if (!ptr)
		bcrash("Out of memory while trying to allocate %lu bytes",
				(unsigned long)size);

	num_allocs++;
	return ptr;
}

void *brealloc(void *ptr, size_t size)
{
	if (!ptr)
		num_allocs++;

	ptr = alloc.realloc(ptr, size);
	if (!ptr && !size)
		ptr = alloc.realloc(ptr, 1);
	if (!ptr)
		bcrash("Out of memory while trying to allocate %lu bytes",
				(unsigned long)size);

	return ptr;
}

void bfree(void *ptr)
{
	if (ptr)
		num_allocs--;
	alloc.free(ptr);
}

size_t bnum_allocs(void)
{
	return num_allocs;
}

void *bmemdup(const void *ptr, size_t size)
{
	void *out = bmalloc(size);
	if (size)
		memcpy(out, ptr, size);

	return out;
}
