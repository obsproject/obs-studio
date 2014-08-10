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

#include "format-conversion.h"
#include "video-io.h"
#include "video-frame.h"
#include "video-scaler.h"

#define MAX_CONVERT_BUFFERS 3

struct video_input {
	struct video_scale_info   conversion;
	video_scaler_t            scaler;
	struct video_frame        frame[MAX_CONVERT_BUFFERS];
	int                       cur_frame;

	void (*callback)(void *param, struct video_data *frame);
	void *param;
};

static inline void video_input_free(struct video_input *input)
{
	for (size_t i = 0; i < MAX_CONVERT_BUFFERS; i++)
		video_frame_free(&input->frame[i]);
	video_scaler_destroy(input->scaler);
}

struct video_output {
	struct video_output_info   info;

	pthread_t                  thread;
	pthread_mutex_t            data_mutex;
	os_event_t                 stop_event;

	struct video_data          cur_frame;
	struct video_data          next_frame;
	bool                       new_frame;

	os_event_t                 update_event;
	uint64_t                   frame_time;
	volatile uint64_t          cur_video_time;
	uint32_t                   skipped_frames;
	uint32_t                   total_frames;

	bool                       initialized;

	pthread_mutex_t            input_mutex;
	DARRAY(struct video_input) inputs;
};

/* ------------------------------------------------------------------------- */

static inline void video_swapframes(struct video_output *video)
{
	if (video->new_frame) {
		video->cur_frame = video->next_frame;
		video->new_frame = false;
	}
}

static inline bool scale_video_output(struct video_input *input,
		struct video_data *data)
{
	bool success = true;

	if (input->scaler) {
		struct video_frame *frame;

		if (++input->cur_frame == MAX_CONVERT_BUFFERS)
			input->cur_frame = 0;

		frame = &input->frame[input->cur_frame];

		success = video_scaler_scale(input->scaler,
				frame->data, frame->linesize,
				(const uint8_t * const*)data->data,
				data->linesize);

		if (success) {
			for (size_t i = 0; i < MAX_AV_PLANES; i++) {
				data->data[i]     = frame->data[i];
				data->linesize[i] = frame->linesize[i];
			}
		} else {
			blog(LOG_WARNING, "video-io: Could not scale frame!");
		}
	}

	return success;
}

static inline void video_output_cur_frame(struct video_output *video)
{
	if (!video->cur_frame.data[0])
		return;

	pthread_mutex_lock(&video->input_mutex);

	for (size_t i = 0; i < video->inputs.num; i++) {
		struct video_input *input = video->inputs.array+i;
		struct video_data frame = video->cur_frame;

		if (scale_video_output(input, &frame))
			input->callback(input->param, &frame);
	}

	pthread_mutex_unlock(&video->input_mutex);
}

#define MAX_MISSED_TIMINGS 8

static inline bool safe_sleepto(uint64_t t, uint32_t *missed_timings)
{
	if (!os_sleepto_ns(t))
		(*missed_timings)++;
	else
		*missed_timings = 0;

	return *missed_timings <= MAX_MISSED_TIMINGS;
}

static void *video_thread(void *param)
{
	struct video_output *video         = param;
	uint64_t            cur_time       = os_gettime_ns();
	uint32_t            missed_timings = 0;

	while (os_event_try(video->stop_event) == EAGAIN) {
		/* wait half a frame, update frame */
		cur_time += (video->frame_time/2);

		if (safe_sleepto(cur_time, &missed_timings)) {
			video->cur_video_time = cur_time;
			os_event_signal(video->update_event);
		} else {
			video->skipped_frames++;
		}

		/* wait another half a frame, swap and output frames */
		cur_time += (video->frame_time/2);
		safe_sleepto(cur_time, &missed_timings);

		pthread_mutex_lock(&video->data_mutex);

		video_swapframes(video);
		video_output_cur_frame(video);

		pthread_mutex_unlock(&video->data_mutex);

		video->total_frames++;
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

	out = bzalloc(sizeof(struct video_output));
	if (!out)
		goto fail;

	memcpy(&out->info, info, sizeof(struct video_output_info));
	out->frame_time = (uint64_t)(1000000000.0 * (double)info->fps_den /
		(double)info->fps_num);
	out->initialized = false;

	if (pthread_mutex_init(&out->data_mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&out->input_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&out->stop_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (os_event_init(&out->update_event, OS_EVENT_TYPE_AUTO) != 0)
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

	for (size_t i = 0; i < video->inputs.num; i++)
		video_input_free(&video->inputs.array[i]);
	da_free(video->inputs);

	os_event_destroy(video->update_event);
	os_event_destroy(video->stop_event);
	pthread_mutex_destroy(&video->data_mutex);
	pthread_mutex_destroy(&video->input_mutex);
	bfree(video);
}

static size_t video_get_input_idx(video_t video,
		void (*callback)(void *param, struct video_data *frame),
		void *param)
{
	for (size_t i = 0; i < video->inputs.num; i++) {
		struct video_input *input = video->inputs.array+i;
		if (input->callback == callback && input->param == param)
			return i;
	}

	return DARRAY_INVALID;
}

static inline bool video_input_init(struct video_input *input,
		struct video_output *video)
{
	if (input->conversion.width  != video->info.width ||
	    input->conversion.height != video->info.height ||
	    input->conversion.format != video->info.format) {
		struct video_scale_info from = {
			.format = video->info.format,
			.width  = video->info.width,
			.height = video->info.height,
		};

		int ret = video_scaler_create(&input->scaler,
				&input->conversion, &from,
				VIDEO_SCALE_FAST_BILINEAR);
		if (ret != VIDEO_SCALER_SUCCESS) {
			if (ret == VIDEO_SCALER_BAD_CONVERSION)
				blog(LOG_ERROR, "video_input_init: Bad "
				                "scale conversion type");
			else
				blog(LOG_ERROR, "video_input_init: Failed to "
				                "create scaler");

			return false;
		}

		for (size_t i = 0; i < MAX_CONVERT_BUFFERS; i++)
			video_frame_init(&input->frame[i],
					input->conversion.format,
					input->conversion.width,
					input->conversion.height);
	}

	return true;
}

bool video_output_connect(video_t video,
		const struct video_scale_info *conversion,
		void (*callback)(void *param, struct video_data *frame),
		void *param)
{
	bool success = false;

	if (!video || !callback)
		return false;

	pthread_mutex_lock(&video->input_mutex);

	if (video_get_input_idx(video, callback, param) == DARRAY_INVALID) {
		struct video_input input;
		memset(&input, 0, sizeof(input));

		input.callback = callback;
		input.param    = param;

		if (conversion) {
			input.conversion = *conversion;
		} else {
			input.conversion.format    = video->info.format;
			input.conversion.width     = video->info.width;
			input.conversion.height    = video->info.height;
		}

		if (input.conversion.width == 0)
			input.conversion.width = video->info.width;
		if (input.conversion.height == 0)
			input.conversion.height = video->info.height;

		success = video_input_init(&input, video);
		if (success)
			da_push_back(video->inputs, &input);
	}

	pthread_mutex_unlock(&video->input_mutex);

	return success;
}

void video_output_disconnect(video_t video,
		void (*callback)(void *param, struct video_data *frame),
		void *param)
{
	if (!video || !callback)
		return;

	pthread_mutex_lock(&video->input_mutex);

	size_t idx = video_get_input_idx(video, callback, param);
	if (idx != DARRAY_INVALID) {
		video_input_free(video->inputs.array+idx);
		da_erase(video->inputs, idx);
	}

	pthread_mutex_unlock(&video->input_mutex);
}

bool video_output_active(video_t video)
{
	if (!video) return false;
	return video->inputs.num != 0;
}

const struct video_output_info *video_output_get_info(video_t video)
{
	return video ? &video->info : NULL;
}

void video_output_swap_frame(video_t video, struct video_data *frame)
{
	if (!video) return;

	pthread_mutex_lock(&video->data_mutex);
	video->next_frame = *frame;
	video->new_frame = true;
	pthread_mutex_unlock(&video->data_mutex);
}

bool video_output_wait(video_t video)
{
	if (!video) return false;

	os_event_wait(video->update_event);
	return os_event_try(video->stop_event) == EAGAIN;
}

uint64_t video_output_get_frame_time(video_t video)
{
	return video ? video->frame_time : 0;
}

uint64_t video_output_get_time(video_t video)
{
	return video ? video->cur_video_time : 0;
}

void video_output_stop(video_t video)
{
	void *thread_ret;

	if (!video)
		return;

	if (video->initialized) {
		video->initialized = false;
		os_event_signal(video->stop_event);
		pthread_join(video->thread, &thread_ret);
		os_event_signal(video->update_event);
	}
}

enum video_format video_output_get_format(video_t video)
{
	return video ? video->info.format : VIDEO_FORMAT_NONE;
}

uint32_t video_output_get_width(video_t video)
{
	return video ? video->info.width : 0;
}

uint32_t video_output_get_height(video_t video)
{
	return video ? video->info.height : 0;
}

double video_output_get_frame_rate(video_t video)
{
	if (!video)
		return 0.0;

	return (double)video->info.fps_num / (double)video->info.fps_den;
}

uint32_t video_output_get_skipped_frames(video_t video)
{
	return video->skipped_frames;
}

uint32_t video_output_get_total_frames(video_t video)
{
	return video->total_frames;
}
