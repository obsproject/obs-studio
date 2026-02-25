/******************************************************************************
 * OBS Studio — Audio Buffer Memory Pool (implementation)
 ******************************************************************************/

#include "obs-audio-pool.h"
#include "util/bmem.h"
#include "util/threading.h"

#include <string.h>
#include <stdlib.h>

/* -----------------------------------------------------------------------
 * Internal layout
 *
 * The pool maintains a singly-linked free-list of fixed-size blocks.
 * Each free block stores a single pointer (next) at offset 0 — this
 * is safe because blocks are always at least pointer-size bytes.
 *
 * Arenas are chained so the pool can grow without copying existing
 * allocations.
 * -------------------------------------------------------------------- */

#define POOL_ALIGNMENT 64   /* cache-line size; required for SIMD NT stores */

/* Each large slab of memory allocated from the system. */
struct pool_arena {
	struct pool_arena *next;    /* next arena in chain                  */
	uint8_t           *base;    /* 64-byte-aligned base of the slab     */
	size_t             n_blocks;/* number of blocks in this arena       */
};

struct obs_audio_pool {
	pthread_mutex_t    lock;
	void              *free_list;   /* head of free block linked list   */
	struct pool_arena *arenas;      /* singly-linked arena chain        */
	size_t             block_size;  /* padded block size (multiple of 64) */
};

/* Round sz up to the next multiple of POOL_ALIGNMENT. */
static inline size_t align64(size_t sz)
{
	return (sz + (POOL_ALIGNMENT - 1)) & ~(size_t)(POOL_ALIGNMENT - 1);
}

/* Allocate one arena of n_blocks blocks and push all blocks onto the
 * pool's free list.  Returns false on allocation failure. */
static bool pool_grow(struct obs_audio_pool *pool, size_t n_blocks)
{
	const size_t block_sz = pool->block_size;

	/* Allocate the arena header + extra POOL_ALIGNMENT bytes so we can
	 * align the data region manually even if the system allocator does
	 * not guarantee 64-byte alignment. */
	const size_t raw_sz = sizeof(struct pool_arena) +
	                      block_sz * n_blocks + POOL_ALIGNMENT;

	uint8_t *raw = (uint8_t *)bmalloc(raw_sz);
	if (!raw)
		return false;

	memset(raw, 0, raw_sz);

	struct pool_arena *arena = (struct pool_arena *)raw;
	arena->n_blocks = n_blocks;
	arena->next     = pool->arenas;
	pool->arenas    = arena;

	/* Align base to POOL_ALIGNMENT boundary. */
	uintptr_t base_addr = (uintptr_t)(raw + sizeof(struct pool_arena));
	base_addr = (base_addr + (POOL_ALIGNMENT - 1)) & ~(uintptr_t)(POOL_ALIGNMENT - 1);
	arena->base = (uint8_t *)base_addr;

	/* Push all blocks onto the free list (LIFO order). */
	for (size_t i = 0; i < n_blocks; i++) {
		void *block = arena->base + i * block_sz;
		*(void **)block  = pool->free_list;
		pool->free_list  = block;
	}

	return true;
}

struct obs_audio_pool *obs_audio_pool_create(size_t block_size,
                                             size_t initial_cap)
{
	if (!block_size || !initial_cap)
		return NULL;

	struct obs_audio_pool *pool =
		(struct obs_audio_pool *)bzalloc(sizeof(*pool));
	if (!pool)
		return NULL;

	if (pthread_mutex_init(&pool->lock, NULL) != 0) {
		bfree(pool);
		return NULL;
	}

	pool->block_size = align64(block_size);
	pool->free_list  = NULL;
	pool->arenas     = NULL;

	if (!pool_grow(pool, initial_cap)) {
		pthread_mutex_destroy(&pool->lock);
		bfree(pool);
		return NULL;
	}

	return pool;
}

void obs_audio_pool_destroy(struct obs_audio_pool *pool)
{
	if (!pool)
		return;

	pthread_mutex_lock(&pool->lock);

	struct pool_arena *arena = pool->arenas;
	while (arena) {
		struct pool_arena *next = arena->next;
		bfree(arena); /* frees the entire raw slab (header + data) */
		arena = next;
	}

	pthread_mutex_unlock(&pool->lock);
	pthread_mutex_destroy(&pool->lock);
	bfree(pool);
}

void *obs_audio_pool_alloc(struct obs_audio_pool *pool)
{
	if (!pool)
		return NULL;

	pthread_mutex_lock(&pool->lock);

	if (!pool->free_list) {
		/* Determine how many blocks are in the most-recent arena
		 * and double that count for the new one (geometric growth). */
		size_t n = pool->arenas ? pool->arenas->n_blocks * 2 : 16;
		if (!pool_grow(pool, n)) {
			pthread_mutex_unlock(&pool->lock);
			return NULL;
		}
	}

	void *block      = pool->free_list;
	pool->free_list  = *(void **)block;

	pthread_mutex_unlock(&pool->lock);

	/* Zero the block before handing it out (audio buffers must start 0). */
	memset(block, 0, pool->block_size);
	return block;
}

void obs_audio_pool_free(struct obs_audio_pool *pool, void *ptr)
{
	if (!pool || !ptr)
		return;

	pthread_mutex_lock(&pool->lock);
	*(void **)ptr   = pool->free_list;
	pool->free_list = ptr;
	pthread_mutex_unlock(&pool->lock);
}

size_t obs_audio_pool_block_size(const struct obs_audio_pool *pool)
{
	return pool ? pool->block_size : 0;
}
