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

#pragma once

#include <util/threading.h>
#include <util/darray.h>
#include <obs.h>

#include "media.h"

struct mp_cache {
	mp_video_cb v_preload_cb;
	mp_video_cb v_seek_cb;
	mp_stop_cb stop_cb;
	mp_video_cb v_cb;
	mp_audio_cb a_cb;
	void *opaque;
	bool request_preload;
	bool has_video;
	bool has_audio;

	char *path;
	char *format_name;
	char *ffmpeg_options;
	int buffering;
	int speed;

	pthread_mutex_t mutex;
	os_sem_t *sem;
	bool preload_frame;
	bool stopping;
	bool looping;
	bool active;
	bool reset;
	bool kill;

	bool thread_valid;
	pthread_t thread;

	DARRAY(struct obs_source_frame) video_frames;
	DARRAY(struct obs_source_audio) audio_segments;

	size_t cur_v_idx;
	size_t cur_a_idx;
	size_t next_v_idx;
	size_t next_a_idx;
	int64_t next_v_ts;
	int64_t next_a_ts;

	int64_t final_v_duration;
	int64_t final_a_duration;

	int64_t play_sys_ts;
	int64_t next_pts_ns;
	uint64_t next_ns;
	int64_t start_ts;
	int64_t base_ts;

	bool pause;
	bool reset_ts;
	bool seek;
	bool seek_next_ts;
	bool eof;
	int64_t seek_pos;
	int64_t start_time;
	int64_t media_duration;

	mp_media_t m;
};

typedef struct mp_cache mp_cache_t;

extern bool mp_cache_init(mp_cache_t *c, const struct mp_media_info *info);
extern void mp_cache_free(mp_cache_t *c);

extern void mp_cache_play(mp_cache_t *c, bool loop);
extern void mp_cache_play_pause(mp_cache_t *c, bool pause);
extern void mp_cache_stop(mp_cache_t *c);
extern void mp_cache_preload_frame(mp_cache_t *c);
extern int64_t mp_cache_get_current_time(mp_cache_t *c);
extern void mp_cache_seek(mp_cache_t *c, int64_t pos);
extern int64_t mp_cache_get_frames(mp_cache_t *c);
extern int64_t mp_cache_get_duration(mp_cache_t *c);
