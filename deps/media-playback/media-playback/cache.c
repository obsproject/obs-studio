/*
 * Copyright (c) 2023 Hugh Bailey <obs.jim@gmail.com>
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

#include <media-io/audio-io.h>
#include <util/platform.h>

#include "media-playback.h"
#include "cache.h"
#include "media.h"

extern bool mp_media_init2(mp_media_t *m);
extern bool mp_media_prepare_frames(mp_media_t *m);
extern bool mp_media_eof(mp_media_t *m);
extern void mp_media_next_video(mp_media_t *m, bool preload);
extern void mp_media_next_audio(mp_media_t *m);
extern bool mp_media_reset(mp_media_t *m);

static bool mp_cache_reset(mp_cache_t *c);

static int64_t base_sys_ts = 0;

#define v_eof(c) (c->cur_v_idx == c->video_frames.num)
#define a_eof(c) (c->cur_a_idx == c->audio_segments.num)

static inline int64_t mp_cache_get_next_min_pts(mp_cache_t *c)
{
	int64_t min_next_ns = 0x7FFFFFFFFFFFFFFFLL;

	if (c->has_video && !v_eof(c)) {
		min_next_ns = c->next_v_ts;
	}
	if (c->has_audio && !a_eof(c) && c->next_a_ts < min_next_ns) {
		min_next_ns = c->next_a_ts;
	}

	return min_next_ns;
}

static inline int64_t mp_cache_get_base_pts(mp_cache_t *c)
{
	int64_t base_ts = 0;

	if (c->has_video && c->next_v_ts > base_ts)
		base_ts = c->next_v_ts;
	if (c->has_audio && c->next_a_ts > base_ts)
		base_ts = c->next_a_ts;

	return base_ts;
}

static void reset_ts(mp_cache_t *c)
{
	c->base_ts += mp_cache_get_base_pts(c);
	c->play_sys_ts = (int64_t)os_gettime_ns();
	c->start_ts = c->next_pts_ns = mp_cache_get_next_min_pts(c);
	c->next_ns = 0;
}

static inline bool mp_cache_sleep(mp_cache_t *c)
{
	bool timeout = false;

	if (!c->next_ns) {
		c->next_ns = os_gettime_ns();
	} else {
		const uint64_t t = os_gettime_ns();
		if (c->next_ns > t) {
			const uint32_t delta_ms =
				(uint32_t)((c->next_ns - t + 500000) / 1000000);
			if (delta_ms > 0) {
				static const uint32_t timeout_ms = 200;
				timeout = delta_ms > timeout_ms;
				os_sleep_ms(timeout ? timeout_ms : delta_ms);
			}
		}
	}

	return timeout;
}

static bool mp_cache_eof(mp_cache_t *c)
{
	bool v_ended = !c->has_video || v_eof(c);
	bool a_ended = !c->has_audio || a_eof(c);
	bool eof = v_ended && a_ended;

	if (!eof)
		return false;

	pthread_mutex_lock(&c->mutex);
	if (!c->looping) {
		c->active = false;
		c->stopping = true;
	}
	pthread_mutex_unlock(&c->mutex);

	mp_cache_reset(c);
	return true;
}

bool mp_cache_decode(mp_cache_t *c)
{
	mp_media_t *m = &c->m;
	bool success = false;

	m->full_decode = true;

	mp_media_reset(m);

	while (!mp_media_eof(m)) {
		if (m->has_video)
			mp_media_next_video(m, false);
		if (m->has_audio)
			mp_media_next_audio(m);

		if (!mp_media_prepare_frames(m))
			goto fail;
	}

	success = true;

	c->start_time = c->m.fmt->start_time;
	if (c->start_time == AV_NOPTS_VALUE)
		c->start_time = 0;

fail:
	mp_media_free(m);
	return success;
}

static void seek_to(mp_cache_t *c, int64_t pos)
{
	size_t new_v_idx = 0;
	size_t new_a_idx = 0;

	if (pos > c->media_duration) {
		blog(LOG_WARNING, "MP: Invalid seek position");
		return;
	}

	if (c->has_video) {
		struct obs_source_frame *v;

		for (size_t i = 0; i < c->video_frames.num; i++) {
			v = &c->video_frames.array[i];
			new_v_idx = i;
			if ((int64_t)v->timestamp >= pos) {
				break;
			}
		}

		size_t next_idx = new_v_idx + 1;
		if (next_idx == c->video_frames.num) {
			c->next_v_ts =
				(int64_t)v->timestamp + c->final_v_duration;
		} else {
			struct obs_source_frame *next =
				&c->video_frames.array[next_idx];
			c->next_v_ts = (int64_t)next->timestamp;
		}
	}
	if (c->has_audio) {
		struct obs_source_audio *a;
		for (size_t i = 0; i < c->audio_segments.num; i++) {
			a = &c->audio_segments.array[i];
			new_a_idx = i;
			if ((int64_t)a->timestamp >= pos) {
				break;
			}
		}

		size_t next_idx = new_a_idx + 1;
		if (next_idx == c->audio_segments.num) {
			c->next_a_ts =
				(int64_t)a->timestamp + c->final_a_duration;
		} else {
			struct obs_source_audio *next =
				&c->audio_segments.array[next_idx];
			c->next_a_ts = (int64_t)next->timestamp;
		}
	}

	c->cur_v_idx = c->next_v_idx = new_v_idx;
	c->cur_a_idx = c->next_a_idx = new_a_idx;
}

/* maximum timestamp variance in nanoseconds */
#define MAX_TS_VAR 2000000000LL

static inline bool mp_media_can_play_video(mp_cache_t *c)
{
	return !v_eof(c) && (c->next_v_ts <= c->next_pts_ns ||
			     (c->next_v_ts - c->next_pts_ns > MAX_TS_VAR));
}

static inline bool mp_media_can_play_audio(mp_cache_t *c)
{
	return !a_eof(c) && (c->next_a_ts <= c->next_pts_ns ||
			     (c->next_a_ts - c->next_pts_ns > MAX_TS_VAR));
}

static inline void calc_next_v_ts(mp_cache_t *c, struct obs_source_frame *frame)
{
	int64_t offset;
	if (c->next_v_idx < c->video_frames.num) {
		struct obs_source_frame *next =
			&c->video_frames.array[c->next_v_idx];
		offset = (int64_t)(next->timestamp - frame->timestamp);
	} else {
		offset = c->final_v_duration;
	}

	c->next_v_ts += offset;
}

static inline void calc_next_a_ts(mp_cache_t *c, struct obs_source_audio *audio)
{
	int64_t offset;
	if (c->next_a_idx < c->audio_segments.num) {
		struct obs_source_audio *next =
			&c->audio_segments.array[c->next_a_idx];
		offset = (int64_t)(next->timestamp - audio->timestamp);
	} else {
		offset = c->final_a_duration;
	}

	c->next_a_ts += offset;
}

static void mp_cache_next_video(mp_cache_t *c, bool preload)
{
	/* eof check */
	if (c->next_v_idx == c->video_frames.num) {
		if (mp_media_can_play_video(c))
			c->cur_v_idx = c->next_v_idx;
		return;
	}

	struct obs_source_frame *frame = &c->video_frames.array[c->next_v_idx];
	struct obs_source_frame dup = *frame;

	dup.timestamp = c->base_ts + dup.timestamp - c->start_ts +
			c->play_sys_ts - base_sys_ts;

	if (!preload) {
		if (!mp_media_can_play_video(c))
			return;

		if (c->v_cb)
			c->v_cb(c->opaque, &dup);

		if (c->cur_v_idx < c->next_v_idx)
			++c->cur_v_idx;
		++c->next_v_idx;
		calc_next_v_ts(c, frame);
	} else {
		if (c->seek_next_ts && c->v_seek_cb) {
			c->v_seek_cb(c->opaque, &dup);
		} else if (!c->request_preload) {
			c->v_preload_cb(c->opaque, &dup);
		}
	}
}

static void mp_cache_next_audio(mp_cache_t *c)
{
	/* eof check */
	if (c->next_a_idx == c->audio_segments.num) {
		if (mp_media_can_play_audio(c))
			c->cur_a_idx = c->next_a_idx;
		return;
	}

	if (!mp_media_can_play_audio(c))
		return;

	struct obs_source_audio *audio =
		&c->audio_segments.array[c->next_a_idx];
	struct obs_source_audio dup = *audio;

	dup.timestamp = c->base_ts + dup.timestamp - c->start_ts +
			c->play_sys_ts - base_sys_ts;
	if (c->a_cb)
		c->a_cb(c->opaque, &dup);

	if (c->cur_a_idx < c->next_a_idx)
		++c->cur_a_idx;
	++c->next_a_idx;
	calc_next_a_ts(c, audio);
}

static bool mp_cache_reset(mp_cache_t *c)
{
	bool stopping;
	bool active;

	int64_t next_ts = mp_cache_get_base_pts(c);
	int64_t offset = next_ts - c->next_pts_ns;
	int64_t start_time = c->start_time;

	c->eof = false;
	c->base_ts += next_ts;
	c->seek_next_ts = false;

	seek_to(c, start_time);

	pthread_mutex_lock(&c->mutex);
	stopping = c->stopping;
	active = c->active;
	c->stopping = false;
	pthread_mutex_unlock(&c->mutex);

	if (c->has_video) {
		size_t next_idx = c->video_frames.num > 1 ? 1 : 0;
		c->cur_v_idx = c->next_v_idx = 0;
		c->next_v_ts = c->video_frames.array[next_idx].timestamp;
	}
	if (c->has_audio) {
		size_t next_idx = c->audio_segments.num > 1 ? 1 : 0;
		c->cur_a_idx = c->next_a_idx = 0;
		c->next_a_ts = c->audio_segments.array[next_idx].timestamp;
	}

	if (active) {
		if (!c->play_sys_ts)
			c->play_sys_ts = (int64_t)os_gettime_ns();
		c->start_ts = c->next_pts_ns = mp_cache_get_next_min_pts(c);
		if (c->next_ns)
			c->next_ns += offset;
	} else {
		c->start_ts = c->next_pts_ns = mp_cache_get_next_min_pts(c);
		c->play_sys_ts = (int64_t)os_gettime_ns();
		c->next_ns = 0;
	}

	c->pause = false;

	if (!active && c->v_preload_cb)
		mp_cache_next_video(c, true);
	if (stopping && c->stop_cb)
		c->stop_cb(c->opaque);
	return true;
}

static void mp_cache_calc_next_ns(mp_cache_t *c)
{
	int64_t min_next_ns = mp_cache_get_next_min_pts(c);
	int64_t delta = min_next_ns - c->next_pts_ns;

	if (c->seek_next_ts) {
		delta = 0;
		c->seek_next_ts = false;
	} else {
#ifdef _DEBUG
		assert(delta >= 0);
#endif
		if (delta < 0)
			delta = 0;
		if (delta > 3000000000)
			delta = 0;
	}

	c->next_ns += delta;
	c->next_pts_ns = min_next_ns;
}

static inline bool mp_cache_thread(mp_cache_t *c)
{
	os_set_thread_name("mp_cache_thread");

	if (!mp_cache_decode(c)) {
		return false;
	}

	for (;;) {
		bool reset, kill, is_active, seek, pause, reset_time,
			preload_frame;
		int64_t seek_pos;
		bool timeout = false;

		pthread_mutex_lock(&c->mutex);
		is_active = c->active;
		pause = c->pause;
		pthread_mutex_unlock(&c->mutex);

		if (!is_active || pause) {
			if (os_sem_wait(c->sem) < 0)
				return false;
			if (pause)
				reset_ts(c);
		} else {
			timeout = mp_cache_sleep(c);
		}

		pthread_mutex_lock(&c->mutex);

		reset = c->reset;
		kill = c->kill;
		c->reset = false;
		c->kill = false;

		preload_frame = c->preload_frame;
		pause = c->pause;
		seek_pos = c->seek_pos;
		seek = c->seek;
		reset_time = c->reset_ts;
		c->preload_frame = false;
		c->seek = false;
		c->reset_ts = false;

		pthread_mutex_unlock(&c->mutex);

		if (kill) {
			break;
		}
		if (reset) {
			mp_cache_reset(c);
			continue;
		}

		if (seek) {
			c->seek_next_ts = true;
			seek_to(c, seek_pos);
			continue;
		}

		if (reset_time) {
			reset_ts(c);
			continue;
		}

		if (pause)
			continue;

		if (preload_frame)
			c->v_preload_cb(c->opaque, &c->video_frames.array[0]);

		/* frames are ready */
		if (is_active && !timeout) {
			if (c->has_video)
				mp_cache_next_video(c, false);
			if (c->has_audio)
				mp_cache_next_audio(c);

			if (mp_cache_eof(c))
				continue;

			mp_cache_calc_next_ns(c);
		}
	}

	return true;
}

static void *mp_cache_thread_start(void *opaque)
{
	mp_cache_t *c = opaque;

	if (!mp_cache_thread(c)) {
		if (c->stop_cb) {
			c->stop_cb(c->opaque);
		}
	}

	return NULL;
}

static void fill_video(void *data, struct obs_source_frame *frame)
{
	mp_cache_t *c = data;
	struct obs_source_frame dup;

	obs_source_frame_init(&dup, frame->format, frame->width, frame->height);
	obs_source_frame_copy(&dup, frame);

	dup.timestamp = frame->timestamp;

	c->final_v_duration = c->m.v.last_duration;

	da_push_back(c->video_frames, &dup);
}

static void fill_audio(void *data, struct obs_source_audio *audio)
{
	mp_cache_t *c = data;
	struct obs_source_audio dup = *audio;

	size_t size =
		get_total_audio_size(dup.format, dup.speakers, dup.frames);
	dup.data[0] = bmalloc(size);

	size_t planes = get_audio_planes(dup.format, dup.speakers);
	if (planes > 1) {
		size = get_audio_bytes_per_channel(dup.format) * dup.frames;
		uint8_t *out = (uint8_t *)dup.data[0];

		for (size_t i = 0; i < planes; i++) {
			if (i > 0)
				dup.data[i] = out;

			memcpy(out, audio->data[i], size);
			out += size;
		}
	} else {
		memcpy((uint8_t *)dup.data[0], audio->data[0], size);
	}

	c->final_a_duration = c->m.a.last_duration;

	da_push_back(c->audio_segments, &dup);
}

static inline bool mp_cache_init_internal(mp_cache_t *c,
					  const struct mp_media_info *info)
{
	if (pthread_mutex_init(&c->mutex, NULL) != 0) {
		blog(LOG_WARNING, "MP: Failed to init mutex");
		return false;
	}
	if (os_sem_init(&c->sem, 0) != 0) {
		blog(LOG_WARNING, "MP: Failed to init semaphore");
		return false;
	}

	c->path = info->path ? bstrdup(info->path) : NULL;
	c->format_name = info->format ? bstrdup(info->format) : NULL;

	if (pthread_create(&c->thread, NULL, mp_cache_thread_start, c) != 0) {
		blog(LOG_WARNING, "MP: Could not create media thread");
		return false;
	}

	c->thread_valid = true;
	return true;
}

bool mp_cache_init(mp_cache_t *c, const struct mp_media_info *info)
{
	struct mp_media_info info2 = *info;

	info2.opaque = c;
	info2.v_cb = fill_video;
	info2.a_cb = fill_audio;
	info2.v_preload_cb = NULL;
	info2.v_seek_cb = NULL;
	info2.stop_cb = NULL;
	info2.full_decode = true;

	mp_media_t *m = &c->m;
	if (!mp_media_init(m, &info2)) {
		mp_cache_free(c);
		return false;
	}
	if (!mp_media_init2(m)) {
		mp_cache_free(c);
		return false;
	}

	pthread_mutex_init_value(&c->mutex);
	c->opaque = info->opaque;
	c->v_cb = info->v_cb;
	c->a_cb = info->a_cb;
	c->stop_cb = info->stop_cb;
	c->ffmpeg_options = info->ffmpeg_options;
	c->v_seek_cb = info->v_seek_cb;
	c->v_preload_cb = info->v_preload_cb;
	c->request_preload = info->request_preload;
	c->speed = info->speed;
	c->media_duration = m->fmt->duration;

	c->has_video = m->has_video;
	c->has_audio = m->has_audio;

	if (!base_sys_ts)
		base_sys_ts = (int64_t)os_gettime_ns();

	if (!mp_cache_init_internal(c, info)) {
		mp_cache_free(c);
		return false;
	}

	return true;
}

static void mp_kill_thread(mp_cache_t *c)
{
	if (c->thread_valid) {
		pthread_mutex_lock(&c->mutex);
		c->kill = true;
		pthread_mutex_unlock(&c->mutex);
		os_sem_post(c->sem);

		pthread_join(c->thread, NULL);
	}
}

void mp_cache_free(mp_cache_t *c)
{
	if (!c)
		return;

	mp_cache_stop(c);
	mp_kill_thread(c);

	if (c->m.fmt)
		mp_media_free(&c->m);

	for (size_t i = 0; i < c->video_frames.num; i++) {
		struct obs_source_frame *f = &c->video_frames.array[i];
		obs_source_frame_free(f);
	}
	for (size_t i = 0; i < c->audio_segments.num; i++) {
		struct obs_source_audio *a = &c->audio_segments.array[i];
		bfree((void *)a->data[0]);
	}
	da_free(c->video_frames);
	da_free(c->audio_segments);

	bfree(c->path);
	bfree(c->format_name);
	pthread_mutex_destroy(&c->mutex);
	os_sem_destroy(c->sem);
	memset(c, 0, sizeof(*c));
}

void mp_cache_play(mp_cache_t *c, bool loop)
{
	pthread_mutex_lock(&c->mutex);

	if (c->active)
		c->reset = true;

	c->looping = loop;
	c->active = true;

	pthread_mutex_unlock(&c->mutex);

	os_sem_post(c->sem);
}

void mp_cache_play_pause(mp_cache_t *c, bool pause)
{
	pthread_mutex_lock(&c->mutex);
	if (c->active) {
		c->pause = pause;
		c->reset_ts = !pause;
	}
	pthread_mutex_unlock(&c->mutex);

	os_sem_post(c->sem);
}

void mp_cache_stop(mp_cache_t *c)
{
	pthread_mutex_lock(&c->mutex);
	if (c->active) {
		c->reset = true;
		c->active = false;
		c->stopping = true;
	}
	pthread_mutex_unlock(&c->mutex);

	os_sem_post(c->sem);
}

void mp_cache_preload_frame(mp_cache_t *c)
{
	if (c->request_preload && c->thread_valid && c->v_preload_cb) {
		pthread_mutex_lock(&c->mutex);
		c->preload_frame = true;
		pthread_mutex_unlock(&c->mutex);
		os_sem_post(c->sem);
	}
}

int64_t mp_cache_get_current_time(mp_cache_t *c)
{
	return mp_cache_get_base_pts(c) * (int64_t)c->speed / 100000000LL;
}

void mp_cache_seek(mp_cache_t *c, int64_t pos)
{
	pthread_mutex_lock(&c->mutex);
	if (c->active) {
		c->seek = true;
		c->seek_pos = pos * 1000;
	}
	pthread_mutex_unlock(&c->mutex);

	os_sem_post(c->sem);
}

int64_t mp_cache_get_frames(mp_cache_t *c)
{
	return c->video_frames.num;
}

int64_t mp_cache_get_duration(mp_cache_t *c)
{
	return c->media_duration;
}
