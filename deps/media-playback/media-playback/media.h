/*
 * Copyright (c) 2017 Hugh Bailey <obs.jim@gmail.com>
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
#include "decode.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4204)
#endif

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <util/threading.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

typedef void (*mp_video_cb)(void *opaque, struct obs_source_frame *frame);
typedef void (*mp_audio_cb)(void *opaque, struct obs_source_audio *audio);
typedef void (*mp_stop_cb)(void *opaque);

struct mp_media {
	AVFormatContext *fmt;

	mp_video_cb v_preload_cb;
	mp_stop_cb stop_cb;
	mp_video_cb v_cb;
	mp_audio_cb a_cb;
	void *opaque;

	char *path;
	char *format_name;
	int buffering;

	enum AVPixelFormat scale_format;
	struct SwsContext *swscale;
	int scale_linesizes[4];
	uint8_t *scale_pic[4];

	struct mp_decode v;
	struct mp_decode a;
	bool is_network;
	bool has_video;
	bool has_audio;
	bool is_file;
	bool eof;
	bool hw;

	struct obs_source_frame obsframe;
	enum video_colorspace cur_space;
	enum video_range_type cur_range;
	enum video_range_type force_range;

	int64_t play_sys_ts;
	int64_t next_pts_ns;
	uint64_t next_ns;
	int64_t start_ts;
	int64_t base_ts;

	pthread_mutex_t mutex;
	os_sem_t *sem;
	bool stopping;
	bool looping;
	bool active;
	bool reset;
	bool kill;

	bool thread_valid;
	pthread_t thread;
};

typedef struct mp_media mp_media_t;

extern bool mp_media_init(mp_media_t *media,
		const char *path,
		const char *format,
		int buffering,
		void *opaque,
		mp_video_cb v_cb,
		mp_audio_cb a_cb,
		mp_stop_cb stop_cb,
		mp_video_cb v_preload_cb,
		bool hardware_decoding,
		enum video_range_type force_range);
extern void mp_media_free(mp_media_t *media);

extern void mp_media_play(mp_media_t *media, bool loop);
extern void mp_media_stop(mp_media_t *media);

/* #define DETAILED_DEBUG_INFO */

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101)
#define USE_NEW_FFMPEG_DECODE_API
#endif

#ifdef __cplusplus
}
#endif
