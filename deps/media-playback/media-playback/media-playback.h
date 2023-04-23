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

#pragma once

#include <obs.h>

struct media_playback;
typedef struct media_playback media_playback_t;

typedef void (*mp_video_cb)(void *opaque, struct obs_source_frame *frame);
typedef void (*mp_audio_cb)(void *opaque, struct obs_source_audio *audio);
typedef void (*mp_stop_cb)(void *opaque);

struct mp_media_info {
	void *opaque;

	mp_video_cb v_cb;
	mp_video_cb v_preload_cb;
	mp_video_cb v_seek_cb;
	mp_audio_cb a_cb;
	mp_stop_cb stop_cb;

	const char *path;
	const char *format;
	char *ffmpeg_options;
	int buffering;
	int speed;
	enum video_range_type force_range;
	bool is_linear_alpha;
	bool hardware_decoding;
	bool is_local_file;
	bool reconnecting;
	bool request_preload;
	bool full_decode;
};

extern media_playback_t *
media_playback_create(const struct mp_media_info *info);
extern void media_playback_destroy(media_playback_t *mp);

extern void media_playback_play(media_playback_t *mp, bool looping,
				bool reconnecting);
extern void media_playback_play_pause(media_playback_t *mp, bool pause);
extern void media_playback_stop(media_playback_t *mp);
extern void media_playback_preload_frame(media_playback_t *mp);
extern int64_t media_playback_get_current_time(media_playback_t *mp);
extern void media_playback_seek(media_playback_t *mp, int64_t pos);
extern int64_t media_playback_get_frames(media_playback_t *mp);
extern int64_t media_playback_get_duration(media_playback_t *mp);
extern bool media_playback_has_video(media_playback_t *mp);
extern bool media_playback_has_audio(media_playback_t *mp);
