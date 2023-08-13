/******************************************************************************
    Copyright (C) 2023 by Dennis Sädtler <dennis@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "source-profiler.h"

#include "darray.h"
#include "obs-internal.h"
#include "platform.h"
#include "threading.h"
#include "uthash.h"

struct frame_sample {
	uint64_t tick;
	DARRAY(uint64_t) render_cpu;
	DARRAY(gs_timer_t *) render_timers;
};

/* Buffer frame data collection to give GPU time to finish rendering.
 * Set to the same as the rendering buffer (NUM_TEXTURES) */
#define FRAME_BUFFER_SIZE NUM_TEXTURES

struct source_samples {
	/* the pointer address of the source is the hashtable key */
	uintptr_t key;

	uint8_t frame_idx;
	struct frame_sample *frames[FRAME_BUFFER_SIZE];

	UT_hash_handle hh;
};

/* Basic fixed-size circular buffer to hold most recent N uint64_t values
 * (older items will be overwritten). */
struct ucirclebuf {
	size_t idx;
	size_t capacity;
	size_t num;

	uint64_t *array;
};

struct profiler_entry {
	/* the pointer address of the source is the hashtable key */
	uintptr_t key;

	/* Tick times for last N frames */
	struct ucirclebuf tick;
	/* Time of first render pass in a frame, for last N frames */
	struct ucirclebuf render_cpu;
	struct ucirclebuf render_gpu;
	/* Sum of all render passes in a frame, for last N frames */
	struct ucirclebuf render_cpu_sum;
	struct ucirclebuf render_gpu_sum;
	/* Timestamps of last N async frame submissions */
	struct ucirclebuf async_frame_ts;
	/* Timestamps of last N async frames rendered */
	struct ucirclebuf async_rendered_ts;

	UT_hash_handle hh;
};

/* Hashmaps */
struct source_samples *hm_samples = NULL;
struct profiler_entry *hm_entries = NULL;

/* GPU timer ranges (only required for DirectX) */
static uint8_t timer_idx = 0;
static gs_timer_range_t *timer_ranges[FRAME_BUFFER_SIZE] = {0};

static uint64_t profiler_samples = 0;
/* Sources can be rendered more than once per frame, to avoid reallocating
 * memory in the majority of cases, reserve at least two. */
static const size_t render_times_reservation = 2;

pthread_rwlock_t hm_rwlock = PTHREAD_RWLOCK_INITIALIZER;

static bool enabled = false;
static bool gpu_enabled = false;
/* These can be set from other threads, mark them volatile */
static volatile bool enable_next = false;
static volatile bool gpu_enable_next = false;

void ucirclebuf_init(struct ucirclebuf *buf, size_t capacity)
{
	if (!capacity)
		return;

	memset(buf, 0, sizeof(struct ucirclebuf));
	buf->capacity = capacity;
	buf->array = bmalloc(sizeof(uint64_t) * capacity);
}

void ucirclebuf_free(struct ucirclebuf *buf)
{
	bfree(buf->array);
	memset(buf, 0, sizeof(struct ucirclebuf));
}

void ucirclebuf_push(struct ucirclebuf *buf, uint64_t val)
{
	if (buf->num == buf->capacity) {
		buf->idx %= buf->capacity;
		buf->array[buf->idx++] = val;
		return;
	}

	buf->array[buf->idx++] = val;
	buf->num++;
}

static struct frame_sample *frame_sample_create(void)
{
	struct frame_sample *smp = bzalloc(sizeof(struct frame_sample));
	da_reserve(smp->render_cpu, render_times_reservation);
	da_reserve(smp->render_timers, render_times_reservation);
	return smp;
}

static void frame_sample_destroy(struct frame_sample *sample)
{
	if (sample->render_timers.num) {
		gs_enter_context(obs->video.graphics);
		for (size_t i = 0; i < sample->render_timers.num; i++)
			gs_timer_destroy(sample->render_timers.array[i]);
		gs_leave_context();
	}

	da_free(sample->render_cpu);
	da_free(sample->render_timers);
	bfree(sample);
}

struct source_samples *source_samples_create(const uintptr_t key)
{
	struct source_samples *smps = bzalloc(sizeof(struct source_samples));

	smps->key = key;
	for (size_t i = 0; i < FRAME_BUFFER_SIZE; i++)
		smps->frames[i] = frame_sample_create();

	return smps;
}

static void source_samples_destroy(struct source_samples *sample)
{
	for (size_t i = 0; i < FRAME_BUFFER_SIZE; i++)
		frame_sample_destroy(sample->frames[i]);

	bfree(sample);
}

static struct profiler_entry *entry_create(const uintptr_t key)
{
	struct profiler_entry *ent = bzalloc(sizeof(struct profiler_entry));
	ent->key = key;
	ucirclebuf_init(&ent->tick, profiler_samples);
	ucirclebuf_init(&ent->render_cpu, profiler_samples);
	ucirclebuf_init(&ent->render_gpu, profiler_samples);
	ucirclebuf_init(&ent->render_cpu_sum, profiler_samples);
	ucirclebuf_init(&ent->render_gpu_sum, profiler_samples);
	ucirclebuf_init(&ent->async_frame_ts, profiler_samples);
	ucirclebuf_init(&ent->async_rendered_ts, profiler_samples);
	return ent;
}

static void entry_destroy(struct profiler_entry *entry)
{
	ucirclebuf_free(&entry->tick);
	ucirclebuf_free(&entry->render_cpu);
	ucirclebuf_free(&entry->render_gpu);
	ucirclebuf_free(&entry->render_cpu_sum);
	ucirclebuf_free(&entry->render_gpu_sum);
	ucirclebuf_free(&entry->async_frame_ts);
	ucirclebuf_free(&entry->async_rendered_ts);
	bfree(entry);
}

static void reset_gpu_timers(void)
{
	gs_enter_context(obs->video.graphics);
	for (int i = 0; i < FRAME_BUFFER_SIZE; i++) {
		if (timer_ranges[i]) {
			gs_timer_range_destroy(timer_ranges[i]);
			timer_ranges[i] = NULL;
		}
	}
	gs_leave_context();
}

static void profiler_shutdown(void)
{
	struct source_samples *smp, *tmp;
	HASH_ITER (hh, hm_samples, smp, tmp) {
		HASH_DEL(hm_samples, smp);
		source_samples_destroy(smp);
	}

	pthread_rwlock_wrlock(&hm_rwlock);
	struct profiler_entry *ent, *etmp;
	HASH_ITER (hh, hm_entries, ent, etmp) {
		HASH_DEL(hm_entries, ent);
		entry_destroy(ent);
	}
	pthread_rwlock_unlock(&hm_rwlock);

	reset_gpu_timers();
}

void source_profiler_enable(bool enable)
{
	enable_next = enable;
}

void source_profiler_gpu_enable(bool enable)
{
	gpu_enable_next = enable && enable_next;
}

void source_profiler_reset_video(struct obs_video_info *ovi)
{
	double fps = ceil((double)ovi->fps_num / (double)ovi->fps_den);
	profiler_samples = (uint64_t)(fps * 5);

	/* This is fine because the video thread won't be running at this point */
	profiler_shutdown();
}

void source_profiler_render_begin(void)
{
	if (!gpu_enabled)
		return;

	gs_enter_context(obs->video.graphics);
	if (!timer_ranges[timer_idx])
		timer_ranges[timer_idx] = gs_timer_range_create();

	gs_timer_range_begin(timer_ranges[timer_idx]);
	gs_leave_context();
}

void source_profiler_render_end(void)
{
	if (!gpu_enabled || !timer_ranges[timer_idx])
		return;

	gs_enter_context(obs->video.graphics);
	gs_timer_range_end(timer_ranges[timer_idx]);
	gs_leave_context();
}

void source_profiler_frame_begin(void)
{
	if (!enabled && enable_next)
		enabled = true;

	if (!gpu_enabled && enabled && gpu_enable_next) {
		gpu_enabled = true;
	} else if (gpu_enabled) {
		/* Advance timer idx if gpu enabled */
		timer_idx = (timer_idx + 1) % FRAME_BUFFER_SIZE;
	}
}

static inline bool is_async_video_source(const struct obs_source *source)
{
	return (source->info.output_flags & OBS_SOURCE_ASYNC_VIDEO) ==
	       OBS_SOURCE_ASYNC_VIDEO;
}

static const char *source_profiler_frame_collect_name =
	"source_profiler_frame_collect";
void source_profiler_frame_collect(void)
{
	if (!enabled)
		return;

	profile_start(source_profiler_frame_collect_name);
	bool gpu_disjoint = false;
	bool gpu_ready = false;
	uint64_t freq = 0;

	if (gpu_enabled) {
		uint8_t timer_range_idx = (timer_idx + 1) % FRAME_BUFFER_SIZE;

		if (timer_ranges[timer_range_idx]) {
			gpu_ready = true;
			gs_enter_context(obs->video.graphics);
			gs_timer_range_get_data(timer_ranges[timer_range_idx],
						&gpu_disjoint, &freq);
		}

		if (gpu_disjoint) {
			blog(LOG_WARNING,
			     "GPU Timers were disjoint, discarding samples.");
		}
	}

	pthread_rwlock_wrlock(&hm_rwlock);

	struct source_samples *smps = hm_samples;
	while (smps) {
		/* processing is delayed by FRAME_BUFFER_SIZE - 1 frames */
		uint8_t frame_idx = (smps->frame_idx + 1) % FRAME_BUFFER_SIZE;
		struct frame_sample *smp = smps->frames[frame_idx];

		if (!smp->tick) {
			/* No data yet */
			smps = smps->hh.next;
			continue;
		}

		struct profiler_entry *ent;
		HASH_FIND_PTR(hm_entries, &smps->key, ent);
		if (!ent) {
			ent = entry_create(smps->key);
			HASH_ADD_PTR(hm_entries, key, ent);
		}

		ucirclebuf_push(&ent->tick, smp->tick);

		if (smp->render_cpu.num) {
			uint64_t sum = 0;
			for (size_t idx = 0; idx < smp->render_cpu.num; idx++) {
				sum += smp->render_cpu.array[idx];
			}
			ucirclebuf_push(&ent->render_cpu,
					smp->render_cpu.array[0]);
			ucirclebuf_push(&ent->render_cpu_sum, sum);
			da_clear(smp->render_cpu);
		}

		/* Note that we still check this even if GPU profiling has been
		 * disabled to destroy leftover timers. */
		if (smp->render_timers.num) {
			uint64_t sum = 0, first = 0, ticks = 0;

			for (size_t i = 0; i < smp->render_timers.num; i++) {
				gs_timer_t *timer = smp->render_timers.array[i];

				if (gpu_ready && !gpu_disjoint &&
				    gs_timer_get_data(timer, &ticks)) {
					/* Convert ticks to ns */
					sum += util_mul_div64(
						ticks, 1000000000ULL, freq);
					if (!first)
						first = sum;
				}

				gs_timer_destroy(timer);
			}

			if (first) {
				ucirclebuf_push(&ent->render_gpu, first);
				ucirclebuf_push(&ent->render_gpu_sum, sum);
			}
			da_clear(smp->render_timers);
		}

		const obs_source_t *src = *(const obs_source_t **)smps->hh.key;
		if (is_async_video_source(src)) {
			uint64_t ts = obs_source_get_last_async_ts(src);
			ucirclebuf_push(&ent->async_rendered_ts, ts);
		}

		smps = smps->hh.next;
	}

	pthread_rwlock_unlock(&hm_rwlock);

	if (gpu_enabled && gpu_ready)
		gs_leave_context();

	/* Apply updated states for next frame */
	if (!enable_next) {
		enabled = gpu_enabled = false;
		profiler_shutdown();
	} else if (!gpu_enable_next) {
		gpu_enabled = false;
		reset_gpu_timers();
	}

	profile_end(source_profiler_frame_collect_name);
}

void source_profiler_async_frame_received(obs_source_t *source)
{
	if (!enabled)
		return;

	uint64_t ts = os_gettime_ns();

	pthread_rwlock_wrlock(&hm_rwlock);

	struct profiler_entry *ent;
	HASH_FIND_PTR(hm_entries, &source, ent);
	if (ent)
		ucirclebuf_push(&ent->async_frame_ts, ts);

	pthread_rwlock_unlock(&hm_rwlock);
}

uint64_t source_profiler_source_tick_start(void)
{
	if (!enabled)
		return 0;

	return os_gettime_ns();
}

void source_profiler_source_tick_end(obs_source_t *source, uint64_t start)
{
	if (!enabled)
		return;

	const uint64_t delta = os_gettime_ns() - start;

	struct source_samples *smp = NULL;
	HASH_FIND_PTR(hm_samples, &source, smp);
	if (!smp) {
		smp = source_samples_create((uintptr_t)source);
		HASH_ADD_PTR(hm_samples, key, smp);
	} else {
		/* Advance index here since tick happens first and only once
		 * at the start of each frame. */
		smp->frame_idx = (smp->frame_idx + 1) % FRAME_BUFFER_SIZE;
	}

	smp->frames[smp->frame_idx]->tick = delta;
}

uint64_t source_profiler_source_render_begin(gs_timer_t **timer)
{
	if (!enabled)
		return 0;

	if (gpu_enabled) {
		*timer = gs_timer_create();
		gs_timer_begin(*timer);
	} else {
		*timer = NULL;
	}

	return os_gettime_ns();
}

void source_profiler_source_render_end(obs_source_t *source, uint64_t start,
				       gs_timer_t *timer)
{
	if (!enabled)
		return;
	if (timer)
		gs_timer_end(timer);

	const uint64_t delta = os_gettime_ns() - start;

	struct source_samples *smp;
	HASH_FIND_PTR(hm_samples, &source, smp);

	if (smp) {
		da_push_back(smp->frames[smp->frame_idx]->render_cpu, &delta);
		if (timer) {
			da_push_back(smp->frames[smp->frame_idx]->render_timers,
				     &timer);
		}
	} else if (timer) {
		gs_timer_destroy(timer);
	}
}

static void task_delete_source(void *key)
{
	struct source_samples *smp;
	HASH_FIND_PTR(hm_samples, &key, smp);
	if (smp) {
		HASH_DEL(hm_samples, smp);
		source_samples_destroy(smp);
	}

	pthread_rwlock_rdlock(&hm_rwlock);
	struct profiler_entry *ent = NULL;
	HASH_FIND_PTR(hm_entries, &key, ent);
	if (ent) {
		HASH_DEL(hm_entries, ent);
		entry_destroy(ent);
	}
	pthread_rwlock_unlock(&hm_rwlock);
}

void source_profiler_remove_source(obs_source_t *source)
{
	if (!enabled)
		return;
	/* Schedule deletion task on graphics thread */
	obs_queue_task(OBS_TASK_GRAPHICS, task_delete_source, source, false);
}

static inline void calculate_tick(struct profiler_entry *ent,
				  struct profiler_result *result)
{
	size_t idx = 0;
	uint64_t sum = 0;

	for (; idx < ent->tick.num; idx++) {
		const uint64_t delta = ent->tick.array[idx];
		if (delta > result->tick_max)
			result->tick_max = delta;

		sum += delta;
	}

	if (idx)
		result->tick_avg = sum / idx;
}

static inline void calculate_render(struct profiler_entry *ent,
				    struct profiler_result *result)
{
	size_t idx;
	uint64_t sum = 0, sum_sum = 0;

	for (idx = 0; idx < ent->render_cpu.num; idx++) {
		const uint64_t delta = ent->render_cpu.array[idx];
		if (delta > result->render_max)
			result->render_max = delta;

		sum += delta;
		sum_sum += ent->render_cpu_sum.array[idx];
	}

	if (idx) {
		result->render_avg = sum / idx;
		result->render_sum = sum_sum / idx;
	}

	if (!gpu_enabled)
		return;

	sum = sum_sum = 0;
	for (idx = 0; idx < ent->render_gpu.num; idx++) {
		const uint64_t delta = ent->render_gpu.array[idx];
		if (delta > result->render_gpu_max)
			result->render_gpu_max = delta;

		sum += delta;
		sum_sum += ent->render_gpu_sum.array[idx];
	}

	if (idx) {
		result->render_gpu_avg = sum / idx;
		result->render_gpu_sum = sum_sum / idx;
	}
}

static inline void calculate_fps(const struct ucirclebuf *frames, double *avg,
				 uint64_t *best, uint64_t *worst)
{
	uint64_t deltas = 0, delta_sum = 0, best_delta = 0, worst_delta = 0;

	for (size_t idx = 0; idx < frames->num; idx++) {
		const uint64_t ts = frames->array[idx];
		if (!ts)
			break;

		size_t prev_idx = idx ? idx - 1 : frames->num - 1;
		const uint64_t prev_ts = frames->array[prev_idx];
		if (!prev_ts || prev_ts >= ts)
			continue;

		uint64_t delta = (ts - prev_ts);
		if (delta < best_delta || !best_delta)
			best_delta = delta;
		if (delta > worst_delta)
			worst_delta = delta;

		delta_sum += delta;
		deltas++;
	}

	if (deltas && delta_sum) {
		*avg = 1.0E9 / ((double)delta_sum / (double)deltas);
		*best = best_delta;
		*worst = worst_delta;
	}
}

bool source_profiler_fill_result(obs_source_t *source,
				 struct profiler_result *result)
{
	if (!enabled || !result)
		return false;

	memset(result, 0, sizeof(struct profiler_result));
	/* No or only stale data available */
	if (!obs_source_enabled(source))
		return true;

	pthread_rwlock_rdlock(&hm_rwlock);

	struct profiler_entry *ent = NULL;
	HASH_FIND_PTR(hm_entries, &source, ent);
	if (ent) {
		calculate_tick(ent, result);
		calculate_render(ent, result);

		if (is_async_video_source(source)) {
			calculate_fps(&ent->async_frame_ts,
				      &result->async_input,
				      &result->async_input_best,
				      &result->async_input_worst);
			calculate_fps(&ent->async_rendered_ts,
				      &result->async_rendered,
				      &result->async_rendered_best,
				      &result->async_rendered_worst);
		}
	}

	pthread_rwlock_unlock(&hm_rwlock);

	return !!ent;
}

profiler_result_t *source_profiler_get_result(obs_source_t *source)
{
	profiler_result_t *ret = bmalloc(sizeof(profiler_result_t));
	if (!source_profiler_fill_result(source, ret)) {
		bfree(ret);
		return NULL;
	}
	return ret;
}
