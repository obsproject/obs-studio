/******************************************************************************
 * Lock-free Single-Producer / Single-Consumer ring queue
 *
 * Uses C11 stdatomic with acquire/release semantics.  Safe as long as exactly
 * one thread calls spsc_enqueue() and exactly one (different) thread calls
 * spsc_dequeue() at any given time.
 *
 * The queue stores up to (capacity - 1) items; capacity MUST be a power of 2.
 ******************************************************************************/

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Atomic helpers — thin wrappers so the header compiles as both C11 and C++
 * (C++ uses <atomic> via a cast, C uses <stdatomic.h>).
 * ---------------------------------------------------------------------- */
#ifdef __cplusplus
#  include <atomic>
typedef std::atomic<size_t> spsc_atomic_size_t;
#  define spsc_load_acquire(ptr)       (ptr)->load(std::memory_order_acquire)
#  define spsc_store_release(ptr, val) (ptr)->store((val), std::memory_order_release)
#else
#  include <stdatomic.h>
typedef _Atomic size_t spsc_atomic_size_t;
#  define spsc_load_acquire(ptr)       atomic_load_explicit((ptr), memory_order_acquire)
#  define spsc_store_release(ptr, val) atomic_store_explicit((ptr), (val), memory_order_release)
#endif

/* -------------------------------------------------------------------------
 * spsc_queue — variable-element-size lock-free SPSC ring buffer
 *
 * Each slot stores a fixed-size copy of the element (item_size bytes).
 * Allocate with spsc_queue_create(), free with spsc_queue_destroy().
 * ---------------------------------------------------------------------- */
struct spsc_queue {
	uint8_t             *buf;        /* raw storage: capacity * item_size bytes */
	size_t               capacity;   /* number of slots (power-of-2)            */
	size_t               item_size;  /* bytes per element                        */
	size_t               mask;       /* capacity - 1, for fast modulo            */

	/* Producer writes head; consumer reads it. */
	/* Consumer writes tail; producer reads it. */
#ifdef __cplusplus
	std::atomic<size_t> head;
	std::atomic<size_t> tail;
#else
	_Atomic size_t       head;
	_Atomic size_t       tail;
#endif
};

/* Round n up to the next power of two (minimum 2). */
static inline size_t spsc_next_pow2(size_t n)
{
	if (n < 2)
		return 2;
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFFu
	n |= n >> 32;
#endif
	return n + 1;
}

/*
 * spsc_queue_init — initialise a queue that can hold at least min_capacity
 * items of item_size bytes each.  buf must point to a pre-allocated block of
 * at least spsc_queue_buf_size(min_capacity, item_size) bytes, or pass NULL
 * and use heap allocation via spsc_queue_create().
 */
static inline bool spsc_queue_init(struct spsc_queue *q, size_t min_capacity,
                                   size_t item_size, uint8_t *storage)
{
	size_t cap = spsc_next_pow2(min_capacity + 1); /* +1: one slot always empty */
	if (!storage)
		return false;

	q->buf       = storage;
	q->capacity  = cap;
	q->item_size = item_size;
	q->mask      = cap - 1;

#ifdef __cplusplus
	q->head.store(0, std::memory_order_relaxed);
	q->tail.store(0, std::memory_order_relaxed);
#else
	atomic_init(&q->head, 0);
	atomic_init(&q->tail, 0);
#endif
	return true;
}

/* Returns the number of bytes of storage required for the slot buffer. */
static inline size_t spsc_queue_buf_size(size_t min_capacity, size_t item_size)
{
	return spsc_next_pow2(min_capacity + 1) * item_size;
}

/*
 * spsc_enqueue — copy item_size bytes from src into the next free slot.
 * Returns true on success, false if the queue is full.
 * Must only be called from the producer thread.
 */
static inline bool spsc_enqueue(struct spsc_queue *q, const void *src)
{
	const size_t head = spsc_load_acquire(&q->head);
	const size_t next = (head + 1) & q->mask;

	if (next == spsc_load_acquire(&q->tail))
		return false; /* full */

	memcpy(q->buf + head * q->item_size, src, q->item_size);
	spsc_store_release(&q->head, next);
	return true;
}

/*
 * spsc_dequeue — copy the oldest item into dst and advance the tail.
 * Returns true on success, false if the queue is empty.
 * Must only be called from the consumer thread.
 */
static inline bool spsc_dequeue(struct spsc_queue *q, void *dst)
{
	const size_t tail = spsc_load_acquire(&q->tail);

	if (tail == spsc_load_acquire(&q->head))
		return false; /* empty */

	memcpy(dst, q->buf + tail * q->item_size, q->item_size);
	spsc_store_release(&q->tail, (tail + 1) & q->mask);
	return true;
}

/*
 * spsc_size — approximate number of items currently in the queue.
 * Safe to call from either thread but may be stale by the time it returns.
 */
static inline size_t spsc_size(const struct spsc_queue *q)
{
	const size_t head = spsc_load_acquire((spsc_atomic_size_t *)&q->head);
	const size_t tail = spsc_load_acquire((spsc_atomic_size_t *)&q->tail);
	return (head - tail) & q->mask;
}

/* spsc_empty — returns true if there are no items in the queue. */
static inline bool spsc_empty(const struct spsc_queue *q)
{
	return spsc_load_acquire((spsc_atomic_size_t *)&q->head) ==
	       spsc_load_acquire((spsc_atomic_size_t *)&q->tail);
}

#ifdef __cplusplus
}
#endif
