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

#include "media-playback.h"
#include "media.h"
#include "cache.h"

struct media_playback {
	bool is_cached;
	union {
		mp_media_t media;
		mp_cache_t cache;
	};
};

media_playback_t *media_playback_create(const struct mp_media_info *info)
{
	media_playback_t *mp = bzalloc(sizeof(*mp));
	mp->is_cached = info->is_local_file && info->full_decode;

	if ((mp->is_cached && !mp_cache_init(&mp->cache, info)) ||
	    (!mp->is_cached && !mp_media_init(&mp->media, info))) {
		bfree(mp);
		return NULL;
	}

	return mp;
}

void media_playback_destroy(media_playback_t *mp)
{
	if (!mp)
		return;

	if (mp->is_cached)
		mp_cache_free(&mp->cache);
	else
		mp_media_free(&mp->media);
	bfree(mp);
}

void media_playback_play(media_playback_t *mp, bool looping, bool reconnecting)
{
	if (!mp)
		return;

	if (mp->is_cached)
		mp_cache_play(&mp->cache, looping);
	else
		mp_media_play(&mp->media, looping, reconnecting);
}

void media_playback_play_pause(media_playback_t *mp, bool pause)
{
	if (!mp)
		return;

	if (mp->is_cached)
		mp_cache_play_pause(&mp->cache, pause);
	else
		mp_media_play_pause(&mp->media, pause);
}

void media_playback_stop(media_playback_t *mp)
{
	if (!mp)
		return;

	if (mp->is_cached)
		mp_cache_stop(&mp->cache);
	else
		mp_media_stop(&mp->media);
}

void media_playback_preload_frame(media_playback_t *mp)
{
	if (!mp)
		return;

	if (mp->is_cached)
		mp_cache_preload_frame(&mp->cache);
	else
		mp_media_preload_frame(&mp->media);
}

int64_t media_playback_get_current_time(media_playback_t *mp)
{
	if (!mp)
		return 0;

	if (mp->is_cached)
		return mp_cache_get_current_time(&mp->cache);
	else
		return mp_media_get_current_time(&mp->media);
}

void media_playback_seek(media_playback_t *mp, int64_t pos)
{
	if (mp->is_cached)
		mp_cache_seek(&mp->cache, pos);
	else
		mp_media_seek(&mp->media, pos);
}

int64_t media_playback_get_frames(media_playback_t *mp)
{
	if (!mp)
		return 0;

	if (mp->is_cached)
		return mp_cache_get_frames(&mp->cache);
	else
		return mp_media_get_frames(&mp->media);
}

int64_t media_playback_get_duration(media_playback_t *mp)
{
	if (!mp)
		return 0;

	if (mp->is_cached)
		return mp_cache_get_duration(&mp->cache);
	else
		return mp_media_get_duration(&mp->media);
}

bool media_playback_has_video(media_playback_t *mp)
{
	if (!mp)
		return false;

	if (mp->is_cached)
		return mp->cache.has_video;
	else
		return mp->media.has_video;
}

bool media_playback_has_audio(media_playback_t *mp)
{
	if (!mp)
		return false;

	if (mp->is_cached)
		return mp->cache.has_audio;
	else
		return mp->media.has_audio;
}
