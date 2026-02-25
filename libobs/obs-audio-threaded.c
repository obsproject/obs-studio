/******************************************************************************
 * OBS Studio — Multi-threaded Audio Pipeline implementation (Phase 6.4)
 *
 * See obs-audio-threaded.h for architecture overview.
 *
 * NOTE: Uses OBS's os_atomic_* wrappers instead of C11 <stdatomic.h> to
 *       remain compatible with MSVC in C99/C11 mode.
 ******************************************************************************/

#include "obs-audio-threaded.h"
#include "util/threading.h"
#include "util/bmem.h"
#include "util/base.h"
#include "util/platform.h"

#include <stdlib.h>
#include <string.h>

/* ── Constants ───────────────────────────────────────────────────────────── */
#define DEFAULT_QUEUE_CAP  256u
#define MAX_THREADS         16u

/* ── Job ring-queue ──────────────────────────────────────────────────────── */
struct job_queue {
	struct audio_job *buf;   /* ring buffer, capacity = cap entries    */
	size_t            cap;   /* must be power of 2                     */
	size_t            head;  /* producer writes here                   */
	size_t            tail;  /* consumer reads from here               */
	pthread_mutex_t   mutex;
	pthread_cond_t    not_empty;
};

static bool job_queue_init(struct job_queue *q, size_t cap)
{
	if (!cap || (cap & (cap - 1)))
		cap = DEFAULT_QUEUE_CAP;

	q->buf  = bmalloc(cap * sizeof(struct audio_job));
	if (!q->buf)
		return false;

	q->cap  = cap;
	q->head = 0;
	q->tail = 0;
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->not_empty, NULL);
	return true;
}

static void job_queue_free(struct job_queue *q)
{
	bfree(q->buf);
	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->not_empty);
}

/* Returns false if queue is full (non-blocking). */
static bool job_queue_push(struct job_queue *q, struct audio_job job)
{
	pthread_mutex_lock(&q->mutex);
	size_t next = (q->head + 1) & (q->cap - 1);
	if (next == q->tail) {
		pthread_mutex_unlock(&q->mutex);
		return false;
	}
	q->buf[q->head] = job;
	q->head = next;
	pthread_cond_signal(&q->not_empty);
	pthread_mutex_unlock(&q->mutex);
	return true;
}

/* Blocks until a job is available or shutdown flag is set. */
static bool job_queue_pop(struct job_queue *q, struct audio_job *out,
			  volatile bool *shutdown)
{
	pthread_mutex_lock(&q->mutex);
	while (q->head == q->tail) {
		if (*shutdown) {
			pthread_mutex_unlock(&q->mutex);
			return false;
		}
		pthread_cond_wait(&q->not_empty, &q->mutex);
	}
	*out    = q->buf[q->tail];
	q->tail = (q->tail + 1) & (q->cap - 1);
	pthread_mutex_unlock(&q->mutex);
	return true;
}

/* ── Thread pool ─────────────────────────────────────────────────────────── */
struct obs_audio_threadpool {
	pthread_t        *threads;
	size_t            num_threads;

	struct job_queue  queue;
	volatile bool     shutdown;

	/* Completion barrier */
	pthread_mutex_t   barrier_mutex;
	pthread_cond_t    barrier_cond;

	/* Jobs submitted but not yet completed.
	 * Using OBS os_atomic_* (volatile long) instead of C11 _Atomic. */
	volatile long     pending;

	/* Diagnostics: peak queue depth */
	volatile long     peak_depth;
};

/* Worker thread entry */
static void *worker_thread(void *arg)
{
	struct obs_audio_threadpool *pool = arg;
	os_set_thread_name("obs-audio-worker");

	while (true) {
		struct audio_job job;
		if (!job_queue_pop(&pool->queue, &job, &pool->shutdown))
			break;

		job.fn(job.param);

		/* Decrement pending; if we hit zero, wake the coordinator. */
		long remaining = os_atomic_dec_long(&pool->pending);
		if (remaining == 0) {
			pthread_mutex_lock(&pool->barrier_mutex);
			pthread_cond_broadcast(&pool->barrier_cond);
			pthread_mutex_unlock(&pool->barrier_mutex);
		}
	}
	return NULL;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

struct obs_audio_threadpool *obs_audio_threadpool_create(size_t num_threads,
							 size_t queue_cap)
{
	if (num_threads == 0) {
		size_t cores = (size_t)os_get_logical_cores();
		num_threads  = (cores > 1) ? (cores - 1) : 1;
		if (num_threads > MAX_THREADS)
			num_threads = MAX_THREADS;
	}

	if (queue_cap == 0)
		queue_cap = DEFAULT_QUEUE_CAP;

	/* Round up to power of 2 */
	if (queue_cap & (queue_cap - 1)) {
		size_t p = 1;
		while (p < queue_cap)
			p <<= 1;
		queue_cap = p;
	}

	struct obs_audio_threadpool *pool =
		bzalloc(sizeof(struct obs_audio_threadpool));
	if (!pool)
		return NULL;

	pool->threads = bmalloc(num_threads * sizeof(pthread_t));
	if (!pool->threads) {
		bfree(pool);
		return NULL;
	}

	pool->num_threads = num_threads;
	pool->shutdown    = false;
	pool->pending     = 0;
	pool->peak_depth  = 0;

	pthread_mutex_init(&pool->barrier_mutex, NULL);
	pthread_cond_init(&pool->barrier_cond, NULL);

	if (!job_queue_init(&pool->queue, queue_cap)) {
		bfree(pool->threads);
		bfree(pool);
		return NULL;
	}

	for (size_t i = 0; i < num_threads; i++) {
		if (pthread_create(&pool->threads[i], NULL, worker_thread,
				   pool) != 0) {
			pool->shutdown = true;
			pthread_cond_broadcast(&pool->queue.not_empty);
			for (size_t j = 0; j < i; j++)
				pthread_join(pool->threads[j], NULL);
			job_queue_free(&pool->queue);
			pthread_mutex_destroy(&pool->barrier_mutex);
			pthread_cond_destroy(&pool->barrier_cond);
			bfree(pool->threads);
			bfree(pool);
			return NULL;
		}
	}

	blog(LOG_INFO,
	     "obs-audio-threaded: pool created — %zu worker thread(s), "
	     "queue capacity %zu",
	     num_threads, queue_cap);

	return pool;
}

void obs_audio_threadpool_destroy(struct obs_audio_threadpool *pool)
{
	if (!pool)
		return;

	obs_audio_threadpool_wait(pool);

	pool->shutdown = true;
	pthread_mutex_lock(&pool->queue.mutex);
	pthread_cond_broadcast(&pool->queue.not_empty);
	pthread_mutex_unlock(&pool->queue.mutex);

	for (size_t i = 0; i < pool->num_threads; i++)
		pthread_join(pool->threads[i], NULL);

	job_queue_free(&pool->queue);
	pthread_mutex_destroy(&pool->barrier_mutex);
	pthread_cond_destroy(&pool->barrier_cond);
	bfree(pool->threads);
	bfree(pool);
}

bool obs_audio_threadpool_submit(struct obs_audio_threadpool *pool,
				 audio_job_fn fn, void *param)
{
	if (!pool || !fn)
		return false;

	struct audio_job job = {fn, param};

	/* Increment before enqueue so workers never see pending==0 while
	 * a job is still sitting in the queue. */
	os_atomic_inc_long(&pool->pending);

	if (!job_queue_push(&pool->queue, job)) {
		os_atomic_dec_long(&pool->pending);
		blog(LOG_WARNING,
		     "obs-audio-threaded: job queue full — "
		     "running synchronously");
		fn(param);
		return false;
	}

	/* Update peak_depth diagnostic */
	long qd = (long)((pool->queue.head - pool->queue.tail) &
			 (pool->queue.cap - 1));
	long cur_peak;
	do {
		cur_peak = os_atomic_load_long(&pool->peak_depth);
		if (qd <= cur_peak)
			break;
	} while (!os_atomic_compare_exchange_long(&pool->peak_depth,
						  &cur_peak, qd));

	return true;
}

void obs_audio_threadpool_wait(struct obs_audio_threadpool *pool)
{
	if (!pool)
		return;

	pthread_mutex_lock(&pool->barrier_mutex);
	while (os_atomic_load_long(&pool->pending) > 0)
		pthread_cond_wait(&pool->barrier_cond, &pool->barrier_mutex);
	pthread_mutex_unlock(&pool->barrier_mutex);
}

size_t obs_audio_threadpool_num_threads(const struct obs_audio_threadpool *pool)
{
	return pool ? pool->num_threads : 0;
}

size_t obs_audio_threadpool_peak_depth(const struct obs_audio_threadpool *pool)
{
	return pool ? (size_t)os_atomic_load_long(
			      (volatile long *)&pool->peak_depth)
		    : 0;
}

void obs_audio_threadpool_reset_stats(struct obs_audio_threadpool *pool)
{
	if (pool)
		os_atomic_set_long(&pool->peak_depth, 0);
}
