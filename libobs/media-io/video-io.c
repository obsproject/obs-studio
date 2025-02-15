/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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
#include <inttypes.h>
#include "../util/bmem.h"
#include "../util/platform.h"
#include "../util/profiler.h"
#include "../util/threading.h"
#include "../util/darray.h"
#include "../util/util_uint64.h"

#include "format-conversion.h"
#include "video-io.h"
#include "video-frame.h"
#include "video-scaler.h"

extern profiler_name_store_t *obs_get_profiler_name_store(void);

#define MAX_CONVERT_BUFFERS 3
#define MAX_CACHE_SIZE 16

struct cached_frame_info {
	struct video_data frame;
	int skipped;
	int count;
};

struct video_input {
	struct video_scale_info conversion;
	video_scaler_t *scaler;
	struct video_frame frame[MAX_CONVERT_BUFFERS];
	int cur_frame;

	// allow outputting at fractions of main composition FPS,
	// e.g. 60 FPS with frame_rate_divisor = 1 turns into 30 FPS
	//
	// a separate counter is used in favor of using remainder calculations
	// to allow "inputs" started at the same time to start on the same frame
	// whereas with remainder calculation the frame alignment would depend on
	// the total frame count at the time the encoder was started
	uint32_t frame_rate_divisor;
	uint32_t frame_rate_divisor_counter;

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
	struct video_output_info info;

	pthread_t thread;
	pthread_mutex_t data_mutex;
	bool stop;

	os_sem_t *update_semaphore;
	uint64_t frame_time;
	volatile long skipped_frames;
	volatile long total_frames;

	pthread_mutex_t input_mutex;
	DARRAY(struct video_input) inputs;

	size_t available_frames;
	size_t first_added;
	size_t last_added;
	struct cached_frame_info cache[MAX_CACHE_SIZE];

	struct video_output *parent;

	volatile bool raw_active;
	volatile long gpu_refs;
};

/* ------------------------------------------------------------------------- */

static inline bool scale_video_output(struct video_input *input, struct video_data *data)
{
	bool success = true;

	if (input->scaler) {
		struct video_frame *frame;

		if (++input->cur_frame == MAX_CONVERT_BUFFERS)
			input->cur_frame = 0;

		frame = &input->frame[input->cur_frame];

		success = video_scaler_scale(input->scaler, frame->data, frame->linesize,
					     (const uint8_t *const *)data->data, data->linesize);

		if (success) {
			for (size_t i = 0; i < MAX_AV_PLANES; i++) {
				data->data[i] = frame->data[i];
				data->linesize[i] = frame->linesize[i];
			}
		} else {
			blog(LOG_WARNING, "video-io: Could not scale frame!");
		}
	}

	return success;
}

static inline bool video_output_cur_frame(struct video_output *video)
{
	struct cached_frame_info *frame_info;
	bool complete;
	bool skipped;

	/* -------------------------------- */

	pthread_mutex_lock(&video->data_mutex);

	frame_info = &video->cache[video->first_added];

	pthread_mutex_unlock(&video->data_mutex);

	/* -------------------------------- */

	pthread_mutex_lock(&video->input_mutex);

	for (size_t i = 0; i < video->inputs.num; i++) {
		struct video_input *input = video->inputs.array + i;
		struct video_data frame = frame_info->frame;

		// an explicit counter is used instead of remainder calculation
		// to allow multiple encoders started at the same time to start on
		// the same frame
		uint32_t skip = input->frame_rate_divisor_counter++;
		if (input->frame_rate_divisor_counter == input->frame_rate_divisor)
			input->frame_rate_divisor_counter = 0;

		if (skip)
			continue;

		if (scale_video_output(input, &frame))
			input->callback(input->param, &frame);
	}

	pthread_mutex_unlock(&video->input_mutex);

	/* -------------------------------- */

	pthread_mutex_lock(&video->data_mutex);

	frame_info->frame.timestamp += video->frame_time;
	complete = --frame_info->count == 0;
	skipped = frame_info->skipped > 0;

	if (complete) {
		if (++video->first_added == video->info.cache_size)
			video->first_added = 0;

		if (++video->available_frames == video->info.cache_size)
			video->last_added = video->first_added;
	} else if (skipped) {
		--frame_info->skipped;
		os_atomic_inc_long(&video->skipped_frames);
	}

	pthread_mutex_unlock(&video->data_mutex);

	/* -------------------------------- */

	return complete;
}

static void *video_thread(void *param)
{
	struct video_output *video = param;

	os_set_thread_name("video-io: video thread");

	const char *video_thread_name =
		profile_store_name(obs_get_profiler_name_store(), "video_thread(%s)", video->info.name);

	while (os_sem_wait(video->update_semaphore) == 0) {
		if (video->stop)
			break;

		profile_start(video_thread_name);
		while (!video->stop && !video_output_cur_frame(video)) {
			os_atomic_inc_long(&video->total_frames);
		}

		os_atomic_inc_long(&video->total_frames);
		profile_end(video_thread_name);

		profile_reenable_thread();
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

static inline bool valid_video_params(const struct video_output_info *info)
{
	return info->height != 0 && info->width != 0 && info->fps_den != 0 && info->fps_num != 0;
}

static inline void init_cache(struct video_output *video)
{
	if (video->info.cache_size > MAX_CACHE_SIZE)
		video->info.cache_size = MAX_CACHE_SIZE;

	for (size_t i = 0; i < video->info.cache_size; i++) {
		struct video_frame *frame;
		frame = (struct video_frame *)&video->cache[i];

		video_frame_init(frame, video->info.format, video->info.width, video->info.height);
	}

	video->available_frames = video->info.cache_size;
}

int video_output_open(video_t **video, struct video_output_info *info)
{
	struct video_output *out;

	if (!valid_video_params(info))
		return VIDEO_OUTPUT_INVALIDPARAM;

	out = bzalloc(sizeof(struct video_output));
	if (!out)
		goto fail0;

	memcpy(&out->info, info, sizeof(struct video_output_info));
	out->frame_time = util_mul_div64(1000000000ULL, info->fps_den, info->fps_num);

	if (pthread_mutex_init_recursive(&out->data_mutex) != 0)
		goto fail0;
	if (pthread_mutex_init_recursive(&out->input_mutex) != 0)
		goto fail1;
	if (os_sem_init(&out->update_semaphore, 0) != 0)
		goto fail2;
	if (pthread_create(&out->thread, NULL, video_thread, out) != 0)
		goto fail3;

	init_cache(out);

	*video = out;
	return VIDEO_OUTPUT_SUCCESS;

fail3:
	os_sem_destroy(out->update_semaphore);
fail2:
	pthread_mutex_destroy(&out->input_mutex);
fail1:
	pthread_mutex_destroy(&out->data_mutex);
fail0:
	bfree(out);
	return VIDEO_OUTPUT_FAIL;
}

void video_output_close(video_t *video)
{
	if (!video)
		return;

	video_output_stop(video);

	pthread_mutex_lock(&video->input_mutex);

	for (size_t i = 0; i < video->inputs.num; i++)
		video_input_free(&video->inputs.array[i]);
	da_free(video->inputs);

	for (size_t i = 0; i < video->info.cache_size; i++)
		video_frame_free((struct video_frame *)&video->cache[i]);

	pthread_mutex_unlock(&video->input_mutex);
	os_sem_destroy(video->update_semaphore);
	pthread_mutex_destroy(&video->data_mutex);
	pthread_mutex_destroy(&video->input_mutex);

	bfree(video);
}

static size_t video_get_input_idx(const video_t *video, void (*callback)(void *param, struct video_data *frame),
				  void *param)
{
	for (size_t i = 0; i < video->inputs.num; i++) {
		struct video_input *input = video->inputs.array + i;
		if (input->callback == callback && input->param == param)
			return i;
	}

	return DARRAY_INVALID;
}

static bool match_range(enum video_range_type a, enum video_range_type b)
{
	return (a == VIDEO_RANGE_FULL) == (b == VIDEO_RANGE_FULL);
}

static enum video_colorspace collapse_space(enum video_colorspace cs)
{
	switch (cs) {
	case VIDEO_CS_SRGB:
		cs = VIDEO_CS_709;
		break;
	case VIDEO_CS_2100_HLG:
		cs = VIDEO_CS_2100_PQ;
		break;
	default:
		break;
	}

	return cs;
}

static bool match_space(enum video_colorspace a, enum video_colorspace b)
{
	return (a == VIDEO_CS_DEFAULT) || (b == VIDEO_CS_DEFAULT) || (collapse_space(a) == collapse_space(b));
}

static inline bool video_input_init(struct video_input *input, struct video_output *video)
{
	if (input->conversion.width != video->info.width || input->conversion.height != video->info.height ||
	    input->conversion.format != video->info.format ||
	    !match_range(input->conversion.range, video->info.range) ||
	    !match_space(input->conversion.colorspace, video->info.colorspace)) {
		struct video_scale_info from = {.format = video->info.format,
						.width = video->info.width,
						.height = video->info.height,
						.range = video->info.range,
						.colorspace = video->info.colorspace};

		int ret = video_scaler_create(&input->scaler, &input->conversion, &from, VIDEO_SCALE_FAST_BILINEAR);
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
			video_frame_init(&input->frame[i], input->conversion.format, input->conversion.width,
					 input->conversion.height);
	}

	return true;
}

static inline void reset_frames(video_t *video)
{
	os_atomic_set_long(&video->skipped_frames, 0);
	os_atomic_set_long(&video->total_frames, 0);
}

static const video_t *get_const_root(const video_t *video)
{
	while (video->parent)
		video = video->parent;
	return video;
}

static video_t *get_root(video_t *video)
{
	while (video->parent)
		video = video->parent;
	return video;
}

bool video_output_connect(video_t *video, const struct video_scale_info *conversion,
			  void (*callback)(void *param, struct video_data *frame), void *param)
{
	return video_output_connect2(video, conversion, 1, callback, param);
}

bool video_output_connect2(video_t *video, const struct video_scale_info *conversion, uint32_t frame_rate_divisor,
			   void (*callback)(void *param, struct video_data *frame), void *param)
{
	bool success = false;

	video = get_root(video);

	if (!video || !callback || frame_rate_divisor == 0)
		return false;

	pthread_mutex_lock(&video->input_mutex);

	if (video_get_input_idx(video, callback, param) == DARRAY_INVALID) {
		struct video_input input;
		memset(&input, 0, sizeof(input));

		input.callback = callback;
		input.param = param;

		input.frame_rate_divisor = frame_rate_divisor;

		if (conversion) {
			input.conversion = *conversion;
		} else {
			input.conversion.format = video->info.format;
			input.conversion.width = video->info.width;
			input.conversion.height = video->info.height;
			input.conversion.range = video->info.range;
			input.conversion.colorspace = video->info.colorspace;
		}

		if (input.conversion.width == 0)
			input.conversion.width = video->info.width;
		if (input.conversion.height == 0)
			input.conversion.height = video->info.height;

		success = video_input_init(&input, video);
		if (success) {
			if (video->inputs.num == 0) {
				if (!os_atomic_load_long(&video->gpu_refs)) {
					reset_frames(video);
				}
				os_atomic_set_bool(&video->raw_active, true);
			}
			da_push_back(video->inputs, &input);
		}
	}

	pthread_mutex_unlock(&video->input_mutex);

	return success;
}

static void log_skipped(video_t *video)
{
	long skipped = os_atomic_load_long(&video->skipped_frames);
	double percentage_skipped = (double)skipped / (double)os_atomic_load_long(&video->total_frames) * 100.0;

	if (skipped)
		blog(LOG_INFO,
		     "Video stopped, number of "
		     "skipped frames due "
		     "to encoding lag: "
		     "%ld/%ld (%0.1f%%)",
		     video->skipped_frames, video->total_frames, percentage_skipped);
}

void video_output_disconnect(video_t *video, void (*callback)(void *param, struct video_data *frame), void *param)
{
	video_output_disconnect2(video, callback, param);
}

bool video_output_disconnect2(video_t *video, void (*callback)(void *param, struct video_data *frame), void *param)
{
	if (!video || !callback)
		return false;

	video = get_root(video);

	pthread_mutex_lock(&video->input_mutex);

	size_t idx = video_get_input_idx(video, callback, param);
	if (idx != DARRAY_INVALID) {
		video_input_free(video->inputs.array + idx);
		da_erase(video->inputs, idx);

		if (video->inputs.num == 0) {
			os_atomic_set_bool(&video->raw_active, false);
			if (!os_atomic_load_long(&video->gpu_refs)) {
				log_skipped(video);
			}
		}
	}

	pthread_mutex_unlock(&video->input_mutex);

	return idx != DARRAY_INVALID;
}

bool video_output_active(const video_t *video)
{
	if (!video)
		return false;
	return os_atomic_load_bool(&get_const_root(video)->raw_active);
}

const struct video_output_info *video_output_get_info(const video_t *video)
{
	return video ? &video->info : NULL;
}

bool video_output_lock_frame(video_t *video, struct video_frame *frame, int count, uint64_t timestamp)
{
	struct cached_frame_info *cfi;
	bool locked;

	if (!video)
		return false;

	video = get_root(video);

	pthread_mutex_lock(&video->data_mutex);

	if (video->available_frames == 0) {
		video->cache[video->last_added].count += count;
		video->cache[video->last_added].skipped += count;
		locked = false;

	} else {
		if (video->available_frames != video->info.cache_size) {
			if (++video->last_added == video->info.cache_size)
				video->last_added = 0;
		}

		cfi = &video->cache[video->last_added];
		cfi->frame.timestamp = timestamp;
		cfi->count = count;
		cfi->skipped = 0;

		memcpy(frame, &cfi->frame, sizeof(*frame));

		locked = true;
	}

	pthread_mutex_unlock(&video->data_mutex);

	return locked;
}

void video_output_unlock_frame(video_t *video)
{
	if (!video)
		return;

	video = get_root(video);

	pthread_mutex_lock(&video->data_mutex);

	video->available_frames--;
	os_sem_post(video->update_semaphore);

	pthread_mutex_unlock(&video->data_mutex);
}

uint64_t video_output_get_frame_time(const video_t *video)
{
	return video ? video->frame_time : 0;
}

void video_output_stop(video_t *video)
{
	void *thread_ret;

	if (!video)
		return;

	video = get_root(video);

	if (!video->stop) {
		video->stop = true;
		os_sem_post(video->update_semaphore);
		pthread_join(video->thread, &thread_ret);
	}
}

bool video_output_stopped(video_t *video)
{
	if (!video)
		return true;

	return get_root(video)->stop;
}

enum video_format video_output_get_format(const video_t *video)
{
	return video ? get_const_root(video)->info.format : VIDEO_FORMAT_NONE;
}

uint32_t video_output_get_width(const video_t *video)
{
	return video ? get_const_root(video)->info.width : 0;
}

uint32_t video_output_get_height(const video_t *video)
{
	return video ? get_const_root(video)->info.height : 0;
}

double video_output_get_frame_rate(const video_t *video)
{
	if (!video)
		return 0.0;

	video = get_const_root(video);

	return (double)video->info.fps_num / (double)video->info.fps_den;
}

uint32_t video_output_get_skipped_frames(const video_t *video)
{
	return (uint32_t)os_atomic_load_long(&get_const_root(video)->skipped_frames);
}

uint32_t video_output_get_total_frames(const video_t *video)
{
	return (uint32_t)os_atomic_load_long(&get_const_root(video)->total_frames);
}

/* Note: These four functions below are a very slight bit of a hack.  If the
 * texture encoder thread is active while the raw encoder thread is active, the
 * total frame count will just be doubled while they're both active.  Which is
 * fine.  What's more important is having a relatively accurate skipped frame
 * count. */

void video_output_inc_texture_encoders(video_t *video)
{
	video = get_root(video);

	if (os_atomic_inc_long(&video->gpu_refs) == 1 && !os_atomic_load_bool(&video->raw_active)) {
		reset_frames(video);
	}
}

void video_output_dec_texture_encoders(video_t *video)
{
	video = get_root(video);

	if (os_atomic_dec_long(&video->gpu_refs) == 0 && !os_atomic_load_bool(&video->raw_active)) {
		log_skipped(video);
	}
}

void video_output_inc_texture_frames(video_t *video)
{
	os_atomic_inc_long(&get_root(video)->total_frames);
}

void video_output_inc_texture_skipped_frames(video_t *video)
{
	os_atomic_inc_long(&get_root(video)->skipped_frames);
}

video_t *video_output_create_with_frame_rate_divisor(video_t *video, uint32_t divisor)
{
	// `divisor == 1` would result in the same frame rate,
	// resulting in an unnecessary additional video output
	if (!video || divisor == 0 || divisor == 1)
		return NULL;

	video_t *new_video = bzalloc(sizeof(video_t));
	memcpy(new_video, video, sizeof(*new_video));
	new_video->parent = video;
	new_video->info.fps_den *= divisor;

	return new_video;
}

void video_output_free_frame_rate_divisor(video_t *video)
{
	if (video && video->parent)
		bfree(video);
}
