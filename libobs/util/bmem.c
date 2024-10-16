/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
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

#include <stdlib.h>
#include <string.h>
#include "base.h"
#include "bmem.h"
#include "platform.h"
#include "threading.h"

#ifdef _WIN32
#include <Windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#endif

/*
 * NOTE: totally jacked the mem alignment trick from ffmpeg, credit to them:
 *   http://www.ffmpeg.org/
 */

#define ALIGNMENT 32

/*
 * Attention, intrepid adventurers, exploring the depths of the libobs code!
 *
 * There used to be a TODO comment here saying that we should use memalign on
 * non-Windows platforms. However, since *nix/POSIX systems do not provide an
 * aligned realloc(), this is currently not (easily) achievable.
 * So while the use of posix_memalign()/memalign() would be a fairly trivial
 * change, it would also ruin our memory alignment for some reallocated memory
 * on those platforms.
 */
#if defined(_WIN32)
#define ALIGNED_MALLOC 1
#else
#define ALIGNMENT_HACK 1
#endif

#define BMEM_TRACE_DEPTH 9

struct bmem_trace {
	struct bmem_trace *next;
	struct bmem_trace **prev_next;
	void *buffer[BMEM_TRACE_DEPTH];
	int nptrs;
	size_t size;
};

#define BMEM_TRACE_SIZE_BYTE \
	((sizeof(struct bmem_trace) + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT)
#define BMEM_OVERRUN_TEST_BYTE ALIGNMENT

// TODO: Currently using constant value. Use `calc_crc32` to check contents of `struct bmem_trace`.
#define BMEM_OVERRUN_TEST_CODE 0xB3

static struct bmem_trace *trace_first = NULL;
static pthread_mutex_t bmem_trace_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool bmem_trace_enabled = false;

void bmem_trace_enable()
{
	if (bnum_allocs() || trace_first) {
		blog(LOG_ERROR,
		     "bmem_trace_enable: tried to enable bmem_trace with existing allocations");
		return;
	}
	bmem_trace_enabled = true;
}

bool bmem_trace_is_enabled()
{
	return bmem_trace_enabled;
}

static void bmem_trace_dump_once(int log_level, struct bmem_trace *bt);

#ifdef _WIN32

static inline int backtrace(void **buffer, int size)
{
	return (int)CaptureStackBackTrace(1, size, buffer, NULL);
}

typedef BOOL(WINAPI *SYMINITIALIZE)(HANDLE process, PCTSTR user_search_path,
				    BOOL invade_process);
typedef BOOL(WINAPI *SYMFROMADDR)(HANDLE process, DWORD64 address,
				  PDWORD64 displacement, PSYMBOL_INFOW symbol);

typedef BOOL(WINAPI *SYMREFRESHMODULELIST)(HANDLE process);

extern bool sym_initialize_called;

char **backtrace_symbols(const void **stack, int size)
{
	int frame;
	DWORD64 displacement = 0;
	char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	SYMBOL_INFOW *pSymbol = (SYMBOL_INFOW *)buf;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	IMAGEHLP_LINE64 line = {0};
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	HANDLE process = GetCurrentProcess();

	HMODULE dbghelp = LoadLibraryW(L"DbgHelp");
	if (!dbghelp)
		return NULL;

	SYMINITIALIZE sym_initialize =
		(SYMINITIALIZE)GetProcAddress(dbghelp, "SymInitialize");
	SYMREFRESHMODULELIST sym_refresh_module_list =
		(SYMREFRESHMODULELIST)GetProcAddress(dbghelp,
						     "SymRefreshModuleList");
	SYMFROMADDR sym_from_addr =
		(SYMFROMADDR)GetProcAddress(dbghelp, "SymFromAddrW");

	if (!sym_initialize_called)
		sym_initialize(process, NULL, TRUE);
	else
		sym_refresh_module_list(process);

	char *buffer = malloc(sizeof(char *) * size +
			      MAX_SYM_NAME * sizeof(TCHAR) * size);
	if (!buffer)
		return NULL;

	for (frame = 0; frame < size; frame++) {
		char *pos = buffer + sizeof(char *) * size +
			    MAX_SYM_NAME * sizeof(TCHAR) * frame;
		pos[0] = 0;
		((char **)buffer)[frame] = pos;
		DWORD64 address = (DWORD64)(stack[frame]);
		if (!address)
			continue;

		//get symbol name for address
		if (!sym_from_addr(process, address, (PDWORD64)&displacement,
				   pSymbol))
			continue;

		os_wcs_to_utf8(pSymbol->Name, pSymbol->NameLen, pos,
			       MAX_SYM_NAME * sizeof(TCHAR));
	}
	return (char **)buffer;
}
#endif

static inline void register_trace(void *ptr, size_t size)
{
	struct bmem_trace *bt = ptr;
	void *buffer[BMEM_TRACE_DEPTH + 2];
	int nptrs = backtrace(buffer, BMEM_TRACE_DEPTH + 2);
	if (nptrs > 2) {
		bt->nptrs = nptrs - 2;
		memcpy(bt->buffer, buffer + 2, (nptrs - 2) * sizeof(void *));
	} else {
		bt->nptrs = 0;
	}
	bt->size = size;

	pthread_mutex_lock(&bmem_trace_mutex);
	bt->prev_next = &trace_first;
	bt->next = trace_first;
	trace_first = bt;
	if (bt->next)
		bt->next->prev_next = &bt->next;
	pthread_mutex_unlock(&bmem_trace_mutex);
}

static void unregister_trace(void *ptr)
{
	struct bmem_trace *bt = ptr;
	pthread_mutex_lock(&bmem_trace_mutex);
	if (*bt->prev_next != bt) {
		blog(LOG_ERROR,
		     "unregister_trace corrupted *prev_next=%p expected %p prev_next: %p next: %p",
		     *bt->prev_next, bt, bt->prev_next, bt->next);
		bmem_trace_dump_once(LOG_ERROR, bt);
		if (bt->next)
			bmem_trace_dump_once(LOG_ERROR, bt->next);
		pthread_mutex_unlock(&bmem_trace_mutex);
		return;
	}
	*bt->prev_next = bt->next;
	if (bt->next)
		bt->next->prev_next = bt->prev_next;
	bt->prev_next = NULL;
	bt->next = NULL;
	pthread_mutex_unlock(&bmem_trace_mutex);
}

static void reregister_trace(void *ptr, size_t size)
{
	struct bmem_trace *bt = ptr;
	bt->size = size;
	if (bt->next)
		bt->next->prev_next = &bt->next;
	*bt->prev_next = bt;
}

static void bmem_trace_dump_once(int log_level, struct bmem_trace *bt)
{
	int nptrs = bt->nptrs;
	if (nptrs <= 0 || nptrs > BMEM_TRACE_DEPTH) {
		blog(LOG_ERROR, "backtrace buffer broken %p nptrs=%d", bt,
		     bt->nptrs);
		return;
	}
	char **strings = backtrace_symbols(bt->buffer, nptrs);
	if (!strings) {
		blog(LOG_ERROR, "backtrace_symbols for memory returns NULL");
	} else {
		for (int i = 0; i < nptrs; i++) {
			blog(log_level, "memory leak trace[%d]: %s", i,
			     strings[i]);
		}
		free(strings);
	}
}

static void bmem_overrun_test_set(uint8_t *ptr)
{
	for (uint8_t i = 0; i < BMEM_OVERRUN_TEST_BYTE; i++)
		ptr[i] = BMEM_OVERRUN_TEST_CODE + i;
}

static void bmem_overrun_test_check(uint8_t *ptr)
{
	bool pass = true;
	for (size_t i = 0; i < BMEM_OVERRUN_TEST_BYTE; i++)
		pass &= ptr[i] == BMEM_OVERRUN_TEST_CODE + i;
	if (!pass) {
		blog(LOG_ERROR, "bmem_overrun_test_check: failed at %p", ptr);
	}
}

void bmem_trace_dump(int log_level)
{
	if (!bmem_trace_enabled) {
		blog(LOG_ERROR, "bmem_trace_dump: bmem_trace not enabled");
		return;
	}

	pthread_mutex_lock(&bmem_trace_mutex);
	int n = 0;
	for (struct bmem_trace *bt = trace_first; bt; bt = bt->next) {
		blog(log_level, "memory leak[%d] %p", n, bt);
		bmem_trace_dump_once(log_level, bt);
		n++;
	}
	pthread_mutex_unlock(&bmem_trace_mutex);
}

static void *a_malloc(size_t size)
{
	size_t mem_size = size;

	if (bmem_trace_enabled)
		mem_size += BMEM_TRACE_SIZE_BYTE + BMEM_OVERRUN_TEST_BYTE;

#ifdef ALIGNED_MALLOC
	void *ptr = _aligned_malloc(mem_size, ALIGNMENT);
#elif ALIGNMENT_HACK
	void *ptr = NULL;
	long diff;

	ptr = malloc(mem_size + ALIGNMENT);
	if (ptr) {
		diff = ((~(long)ptr) & (ALIGNMENT - 1)) + 1;
		ptr = (char *)ptr + diff;
		((char *)ptr)[-1] = (char)diff;
	}
#else
	void *ptr = malloc(mem_size);
#endif

	if (bmem_trace_enabled && ptr) {
		register_trace(ptr, size);
		ptr = (uint8_t *)ptr + BMEM_TRACE_SIZE_BYTE;
		bmem_overrun_test_set((uint8_t *)ptr + size);
	}

	return ptr;
}

static void *a_realloc(void *ptr, size_t size)
{
	if (!ptr)
		return a_malloc(size);

	size_t mem_size = size;

	if (bmem_trace_enabled) {
		struct bmem_trace *trace =
			(struct bmem_trace *)((uint8_t *)ptr -
					      BMEM_TRACE_SIZE_BYTE);
		bmem_overrun_test_check((uint8_t *)ptr + trace->size);
		ptr = (uint8_t *)ptr - BMEM_TRACE_SIZE_BYTE;
		mem_size += BMEM_TRACE_SIZE_BYTE + BMEM_OVERRUN_TEST_BYTE;
		pthread_mutex_lock(&bmem_trace_mutex);
	}

#ifdef ALIGNED_MALLOC
	ptr = _aligned_realloc(ptr, mem_size, ALIGNMENT);
#elif ALIGNMENT_HACK
	long diff = ((unsigned char *)ptr)[-1];
	ptr = realloc((char *)ptr - diff, mem_size + diff);
	if (ptr)
		ptr = (char *)ptr + diff;
#else
	ptr = realloc(ptr, mem_size);
#endif
	if (bmem_trace_enabled && ptr) {
		reregister_trace(ptr, size);
		pthread_mutex_unlock(&bmem_trace_mutex);
		ptr = (uint8_t *)ptr + BMEM_TRACE_SIZE_BYTE;
		bmem_overrun_test_set((uint8_t *)ptr + size);
	} else if (bmem_trace_enabled) {
		pthread_mutex_unlock(&bmem_trace_mutex);
	}
	return ptr;
}

static void a_free(void *ptr)
{
	if (!ptr)
		return;

	if (bmem_trace_enabled) {
		struct bmem_trace *trace =
			(struct bmem_trace *)((uint8_t *)ptr -
					      BMEM_TRACE_SIZE_BYTE);
		bmem_overrun_test_check((uint8_t *)ptr + trace->size);
		ptr = (uint8_t *)ptr - BMEM_TRACE_SIZE_BYTE;
		unregister_trace((uint8_t *)ptr);
	}

#ifdef ALIGNED_MALLOC
	_aligned_free(ptr);
#elif ALIGNMENT_HACK
	free((char *)ptr - ((char *)ptr)[-1]);
#else
	free(ptr);
#endif
}

static long num_allocs = 0;

void *bmalloc(size_t size)
{
	if (!size) {
		os_breakpoint();
		bcrash("bmalloc: Allocating 0 bytes is broken behavior, please fix your code!");
	}

	void *ptr = a_malloc(size);

	if (!ptr) {
		os_breakpoint();
		bcrash("Out of memory while trying to allocate %lu bytes", (unsigned long)size);
	}

	os_atomic_inc_long(&num_allocs);
	return ptr;
}

void *brealloc(void *ptr, size_t size)
{
	if (!ptr)
		os_atomic_inc_long(&num_allocs);

	if (!size) {
		os_breakpoint();
		bcrash("brealloc: Allocating 0 bytes is broken behavior, please fix your code!");
	}

	ptr = a_realloc(ptr, size);

	if (!ptr) {
		os_breakpoint();
		bcrash("Out of memory while trying to allocate %lu bytes", (unsigned long)size);
	}

	return ptr;
}

void bfree(void *ptr)
{
	if (ptr) {
		os_atomic_dec_long(&num_allocs);
		a_free(ptr);
	}
}

long bnum_allocs(void)
{
	return num_allocs;
}

int base_get_alignment(void)
{
	return ALIGNMENT;
}

void *bmemdup(const void *ptr, size_t size)
{
	void *out = bmalloc(size);
	if (size)
		memcpy(out, ptr, size);

	return out;
}
