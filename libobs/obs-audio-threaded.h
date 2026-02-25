/******************************************************************************
 * OBS Studio — Multi-threaded Audio Pipeline (Phase 6.4)
 *
 * A fixed-size thread pool that processes audio sources in parallel,
 * replacing the sequential per-source render loop in obs-audio.c.
 *
 * Architecture:
 *   - One coordinator thread (the existing OBS audio thread) submits jobs.
 *   - N worker threads (default: min(logical_cores - 1, 8)) each pop one
 *     source from the job queue and call obs_source_audio_render() on it.
 *   - After all jobs are dispatched, the coordinator blocks on a completion
 *     barrier.  Workers signal the barrier when the queue empties.
 *   - Audio mixing is performed serially AFTER all parallel renders complete,
 *     because mix_audio() accumulates into shared output buffers.
 *
 * Expected improvement: 20-30% on systems with 6+ physical cores when
 * mixing 4+ sources simultaneously.
 *
 * Thread safety:
 *   - obs_audio_threadpool_submit() is called from one thread at a time.
 *   - The job queue uses the SPSC queue from util/spsc-queue.h for zero-
 *     overhead dispatch when there is only one submitter.  For multiple
 *     concurrent submitters, a lightweight mutex guards the enqueue side.
 *   - Worker threads dequeue with a mutex (MPSC dequeue pattern).
 ******************************************************************************/

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Opaque pool handle ──────────────────────────────────────────────────── */
struct obs_audio_threadpool;

/* ── Job descriptor ──────────────────────────────────────────────────────── */
typedef void (*audio_job_fn)(void *param);

struct audio_job {
	audio_job_fn fn;    /* function to call                              */
	void        *param; /* opaque parameter forwarded to fn()            */
};

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

/*
 * obs_audio_threadpool_create
 *
 * @num_threads   Number of worker threads to spawn.  Pass 0 to auto-detect
 *               (= logical-core-count - 1, clamped to [1, 16]).
 * @queue_cap     Capacity of the internal job ring-queue.  Must be a power of
 *               2.  Pass 0 for the default (256 jobs, enough for any OBS
 *               scene).
 *
 * Returns NULL on failure.
 */
struct obs_audio_threadpool *obs_audio_threadpool_create(size_t num_threads,
							 size_t queue_cap);

/*
 * obs_audio_threadpool_destroy
 *
 * Signals all workers to stop, waits for them to drain the queue, then joins
 * and frees everything.  Safe to call with NULL.
 */
void obs_audio_threadpool_destroy(struct obs_audio_threadpool *pool);

/* ── Job submission ──────────────────────────────────────────────────────── */

/*
 * obs_audio_threadpool_submit
 *
 * Enqueue one job.  Returns false if the queue is full.
 * Must be called from a single thread (the audio coordinator).
 */
bool obs_audio_threadpool_submit(struct obs_audio_threadpool *pool,
				 audio_job_fn fn, void *param);

/*
 * obs_audio_threadpool_wait
 *
 * Block until all previously submitted jobs have completed.
 * Call this after all jobs for one audio tick have been submitted.
 */
void obs_audio_threadpool_wait(struct obs_audio_threadpool *pool);

/* ── Diagnostics ─────────────────────────────────────────────────────────── */

/* Return the number of worker threads. */
size_t obs_audio_threadpool_num_threads(const struct obs_audio_threadpool *pool);

/* Return peak job queue depth observed since last reset. */
size_t obs_audio_threadpool_peak_depth(const struct obs_audio_threadpool *pool);

/* Reset peak-depth counter. */
void obs_audio_threadpool_reset_stats(struct obs_audio_threadpool *pool);

#ifdef __cplusplus
}
#endif
