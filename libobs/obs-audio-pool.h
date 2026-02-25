/******************************************************************************
 * OBS Studio — Audio Buffer Memory Pool
 *
 * Pre-allocates a contiguous arena of 64-byte-aligned fixed-size blocks for
 * the per-source audio output and mix buffers.  Replacing per-source heap
 * allocation with pool allocation gives:
 *
 *   • Zero malloc/free overhead on the hot source-create/destroy path
 *   • Guaranteed 64-byte (cache-line) alignment for SIMD loads/stores
 *   • Better cache locality — sequential sources share the same arena
 *   • No heap fragmentation from repeated large-block alloc/free cycles
 *
 * Thread safety: pool_alloc / pool_free are protected by a spin-mutex and
 * are safe to call from any thread.  They are NOT called from the realtime
 * audio callback, only from source creation/destruction.
 ******************************************************************************/

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * obs_audio_pool — opaque pool handle
 * -------------------------------------------------------------------- */
struct obs_audio_pool;

/*
 * obs_audio_pool_create
 *
 * @block_size    Bytes per block (will be rounded up to 64-byte boundary).
 * @initial_cap   Number of blocks to pre-allocate.  The pool grows
 *                automatically in doublings if exhausted.
 *
 * Returns NULL on allocation failure.
 */
struct obs_audio_pool *obs_audio_pool_create(size_t block_size,
                                             size_t initial_cap);

/* obs_audio_pool_destroy — free all memory owned by the pool. */
void obs_audio_pool_destroy(struct obs_audio_pool *pool);

/*
 * obs_audio_pool_alloc — obtain one zeroed, aligned block from the pool.
 * Returns NULL only if system memory is exhausted.
 */
void *obs_audio_pool_alloc(struct obs_audio_pool *pool);

/*
 * obs_audio_pool_free — return a block to the pool.
 * Passing NULL is a no-op.
 */
void obs_audio_pool_free(struct obs_audio_pool *pool, void *ptr);

/* obs_audio_pool_block_size — actual (padded) block size in bytes. */
size_t obs_audio_pool_block_size(const struct obs_audio_pool *pool);

#ifdef __cplusplus
}
#endif
