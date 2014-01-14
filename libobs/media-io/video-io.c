/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include <assert.h>
#include "../util/bmem.h"
#include "../util/platform.h"
#include "../util/threading.h"
#include "../util/darray.h"

#include "video-io.h"

struct video_input {
	struct video_info format;
	void (*callback)(void *param, struct video_frame *frame);
	void *param;
};

struct video_output {
	struct video_output_info   info;

	pthread_t                  thread;
	pthread_mutex_t            data_mutex;
	event_t                    stop_event;

	struct video_frame         *cur_frame;
	struct video_frame         *next_frame;
	event_t                    update_event;
	uint64_t                   frame_time;
	volatile uint64_t          cur_video_time;

	bool                       initialized;

	pthread_mutex_t            input_mutex;
	DARRAY(struct video_input) inputs;
};

/* ------------------------------------------------------------------------- */

static inline void video_swapframes(struct video_output *video)
{
	pthread_mutex_lock(&video->data_mutex);

	if (video->next_frame) {
		video->cur_frame = video->next_frame;
		video->next_frame = NULL;
	}

	pthread_mutex_unlock(&video->data_mutex);
}

static inline void video_output_cur_frame(struct video_output *video)
{
	if (!video->cur_frame)
		return;

	pthread_mutex_lock(&video->input_mutex);

	/* TODO: conversion */
	for (size_t i = 0; i < video->inputs.num; i++) {
		struct video_input *input = video->inputs.array+i;
		input->callback(input->param, video->cur_frame);
	}

	pthread_mutex_unlock(&video->input_mutex);
}

static void *video_thread(void *param)
{
	struct video_output *video = param;
	uint64_t cur_time = os_gettime_ns();

	while (event_try(&video->stop_event) == EAGAIN) {
		/* wait half a frame, update frame */
		os_sleepto_ns(cur_time += (video->frame_time/2));
		video->cur_video_time = cur_time;
		event_signal(&video->update_event);

		/* wait another half a frame, swap and output frames */
		os_sleepto_ns(cur_time += (video->frame_time/2));
		video_swapframes(video);
		video_output_cur_frame(video);
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

static inline bool valid_video_params(struct video_output_info *info)
{
	return info->height != 0 && info->width != 0 && info->fps_den != 0 &&
	       info->fps_num != 0;
}

int video_output_open(video_t *video, struct video_output_info *info)
{
	struct video_output *out;

	if (!valid_video_params(info))
		return VIDEO_OUTPUT_INVALIDPARAM;

	out = bmalloc(sizeof(struct video_output));
	memset(out, 0, sizeof(struct video_output));

	memcpy(&out->info, info, sizeof(struct video_info));
	out->frame_time = (uint64_t)(1000000000.0 * (double)info->fps_den /
		(double)info->fps_num);
	out->initialized = false;

	if (pthread_mutex_init(&out->data_mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&out->input_mutex, NULL) != 0)
		goto fail;
	if (event_init(&out->stop_event, EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (event_init(&out->update_event, EVENT_TYPE_AUTO) != 0)
		goto fail;
	if (pthread_create(&out->thread, NULL, video_thread, out) != 0)
		goto fail;

	out->initialized = true;
	*video = out;
	return VIDEO_OUTPUT_SUCCESS;

fail:
	video_output_close(out);
	return VIDEO_OUTPUT_FAIL;
}

void video_output_close(video_t video)
{
	if (!video)
		return;

	video_output_stop(video);

	da_free(video->inputs);
	event_destroy(&video->update_event);
	event_destroy(&video->stop_event);
	pthread_mutex_destroy(&video->data_mutex);
	pthread_mutex_destroy(&video->input_mutex);
	bfree(video);
}

static size_t video_get_input_idx(video_t video,
		void (*callback)(void *param, struct video_frame *frame),
		void *param)
{
	for (size_t i = 0; i < video->inputs.num; i++) {
		struct video_input *input = video->inputs.array+i;
		if (input->callback == callback && input->param == param)
			return i;
	}

	return DARRAY_INVALID;
}

void video_output_connect(video_t video, struct video_info *format,
		void (*callback)(void *param, struct video_frame *frame),
		void *param)
{
	pthread_mutex_lock(&video->input_mutex);

	if (video_get_input_idx(video, callback, param) != DARRAY_INVALID) {
		struct video_input input;
		input.callback = callback;
		input.param    = param;

		/* TODO: conversion */
		if (format) {
			input.format = *format;
		} else {
			input.format.type   = video->info.type;
			input.format.height = video->info.height;
			input.format.width  = video->info.width;
		}

		da_push_back(video->inputs, &input);
	}

	pthread_mutex_unlock(&video->input_mutex);
}

void video_output_disconnect(video_t video,
		void (*callback)(void *param, struct video_frame *frame),
		void *param)
{
	pthread_mutex_lock(&video->input_mutex);

	size_t idx = video_get_input_idx(video, callback, param);
	if (idx != DARRAY_INVALID)
		da_erase(video->inputs, idx);

	pthread_mutex_unlock(&video->input_mutex);
}

const struct video_output_info *video_output_getinfo(video_t video)
{
	return &video->info;
}

void video_output_frame(video_t video, struct video_frame *frame)
{
	pthread_mutex_lock(&video->data_mutex);
	video->next_frame = frame;
	pthread_mutex_unlock(&video->data_mutex);
}

bool video_output_wait(video_t video)
{
	event_wait(&video->update_event);
	return event_try(&video->stop_event) == EAGAIN;
}

uint64_t video_getframetime(video_t video)
{
	return video->frame_time;
}

uint64_t video_gettime(video_t video)
{
	return video->cur_video_time;
}

void video_output_stop(video_t video)
{
	void *thread_ret;

	if (!video)
		return;

	if (video->initialized) {
		event_signal(&video->stop_event);
		pthread_join(video->thread, &thread_ret);
		event_signal(&video->update_event);
	}
}
