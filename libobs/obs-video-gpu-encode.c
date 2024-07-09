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

#include "obs-internal.h"

#define NBSP "\xC2\xA0"
static const char *gpu_encode_frame_name = "gpu_encode_frame";
static void *gpu_encode_thread(void *data)
{
	struct obs_core_video_mix *video = data;
	uint64_t interval = video_output_get_frame_time(video->video);
	DARRAY(obs_encoder_t *) encoders;
	int wait_frames = NUM_ENCODE_TEXTURE_FRAMES_TO_WAIT;

	da_init(encoders);

	os_set_thread_name("obs gpu encode thread");
	const char *gpu_encode_thread_name = profile_store_name(
		obs_get_profiler_name_store(),
		"obs_gpu_encode_thread(%g" NBSP "ms)", interval / 1000000.);
	profile_register_root(gpu_encode_thread_name, interval);

	while (os_sem_wait(video->gpu_encode_semaphore) == 0) {
		struct obs_tex_frame tf;
		uint64_t timestamp;
		uint64_t lock_key;
		uint64_t next_key;
		size_t lock_count = 0;
		uint64_t fer_ts = 0;

		if (os_atomic_load_bool(&video->gpu_encode_stop))
			break;

		if (wait_frames) {
			wait_frames--;
			continue;
		}

		profile_start(gpu_encode_thread_name);

		os_event_reset(video->gpu_encode_inactive);

		/* -------------- */

		pthread_mutex_lock(&video->gpu_encoder_mutex);

		deque_pop_front(&video->gpu_encoder_queue, &tf, sizeof(tf));
		timestamp = tf.timestamp;
		lock_key = tf.lock_key;
		next_key = tf.lock_key;

		video_output_inc_texture_frames(video->video);

		for (size_t i = 0; i < video->gpu_encoders.num; i++) {
			obs_encoder_t *encoder = obs_encoder_get_ref(
				video->gpu_encoders.array[i]);
			if (encoder)
				da_push_back(encoders, &encoder);
		}

		pthread_mutex_unlock(&video->gpu_encoder_mutex);

		/* -------------- */

		for (size_t i = 0; i < encoders.num; i++) {
			struct encoder_packet pkt = {0};
			bool received = false;
			bool success = false;
			uint32_t skip = 0;

			obs_encoder_t *encoder = encoders.array[i];
			obs_weak_encoder_t **paired =
				encoder->paired_encoders.array;
			size_t num_paired = encoder->paired_encoders.num;

			pkt.timebase_num = encoder->timebase_num *
					   encoder->frame_rate_divisor;
			pkt.timebase_den = encoder->timebase_den;
			pkt.encoder = encoder;

			if (encoder->encoder_group && !encoder->start_ts) {
				struct obs_encoder_group *group =
					encoder->encoder_group;
				bool ready = false;
				pthread_mutex_lock(&group->mutex);
				ready = group->start_timestamp == timestamp;
				pthread_mutex_unlock(&group->mutex);
				if (!ready)
					continue;
			}

			if (!encoder->first_received && num_paired) {
				bool wait_for_audio = false;

				for (size_t idx = 0;
				     !wait_for_audio && idx < num_paired;
				     idx++) {
					obs_encoder_t *enc =
						obs_weak_encoder_get_encoder(
							paired[idx]);
					if (!enc)
						continue;

					if (!enc->first_received ||
					    enc->first_raw_ts > timestamp) {
						wait_for_audio = true;
					}

					obs_encoder_release(enc);
				}

				if (wait_for_audio)
					continue;
			}

			if (video_pause_check(&encoder->pause, timestamp))
				continue;

			if (encoder->reconfigure_requested) {
				encoder->reconfigure_requested = false;
				encoder->info.update(encoder->context.data,
						     encoder->context.settings);
			}

			// an explicit counter is used instead of remainder calculation
			// to allow multiple encoders started at the same time to start on
			// the same frame
			skip = encoder->frame_rate_divisor_counter++;
			if (encoder->frame_rate_divisor_counter ==
			    encoder->frame_rate_divisor)
				encoder->frame_rate_divisor_counter = 0;
			if (skip)
				continue;

			if (!encoder->start_ts)
				encoder->start_ts = timestamp;

			if (++lock_count == encoders.num)
				next_key = 0;
			else
				next_key++;

			/* Get the frame encode request timestamp. This
			 * needs to be read just before the encode request.
			 */
			fer_ts = os_gettime_ns();

			profile_start(gpu_encode_frame_name);
			if (encoder->info.encode_texture2) {
				struct encoder_texture tex = {0};

				tex.handle = tf.handle;
				tex.tex[0] = tf.tex;
				tex.tex[1] = tf.tex_uv;
				tex.tex[2] = NULL;
				success = encoder->info.encode_texture2(
					encoder->context.data, &tex,
					encoder->cur_pts, lock_key, &next_key,
					&pkt, &received);
			} else {
				success = encoder->info.encode_texture(
					encoder->context.data, tf.handle,
					encoder->cur_pts, lock_key, &next_key,
					&pkt, &received);
			}
			profile_end(gpu_encode_frame_name);

			/* Generate and enqueue the frame timing metrics, namely
			 * the CTS (composition time), FER (frame encode request), FERC
			 * (frame encode request complete) and current PTS. PTS is used to
			 * associate the frame timing data with the encode packet. */
			if (tf.timestamp) {
				struct encoder_packet_time *ept =
					da_push_back_new(
						encoder->encoder_packet_times);
				// Get the frame encode request complete timestamp
				if (success) {
					ept->ferc = os_gettime_ns();
				} else {
					// Encode had error, set ferc to 0
					ept->ferc = 0;
				}

				ept->pts = encoder->cur_pts;
				ept->cts = tf.timestamp;
				ept->fer = fer_ts;
			}

			send_off_encoder_packet(encoder, success, received,
						&pkt);

			lock_key = next_key;

			encoder->cur_pts += encoder->timebase_num *
					    encoder->frame_rate_divisor;
		}

		/* -------------- */

		pthread_mutex_lock(&video->gpu_encoder_mutex);

		tf.lock_key = next_key;

		if (--tf.count) {
			tf.timestamp += interval;
			deque_push_front(&video->gpu_encoder_queue, &tf,
					 sizeof(tf));

			video_output_inc_texture_skipped_frames(video->video);
		} else {
			deque_push_back(&video->gpu_encoder_avail_queue, &tf,
					sizeof(tf));
		}

		pthread_mutex_unlock(&video->gpu_encoder_mutex);

		/* -------------- */

		os_event_signal(video->gpu_encode_inactive);

		for (size_t i = 0; i < encoders.num; i++)
			obs_encoder_release(encoders.array[i]);

		da_resize(encoders, 0);

		profile_end(gpu_encode_thread_name);
		profile_reenable_thread();
	}

	da_free(encoders);
	return NULL;
}

bool init_gpu_encoding(struct obs_core_video_mix *video)
{
	const struct video_output_info *info =
		video_output_get_info(video->video);

	video->gpu_encode_stop = false;

	deque_reserve(&video->gpu_encoder_avail_queue, NUM_ENCODE_TEXTURES);
	for (size_t i = 0; i < NUM_ENCODE_TEXTURES; i++) {
		gs_texture_t *tex;
		gs_texture_t *tex_uv;

		if (info->format == VIDEO_FORMAT_P010) {
			gs_texture_create_p010(
				&tex, &tex_uv, info->width, info->height,
				GS_RENDER_TARGET | GS_SHARED_KM_TEX);
		} else {
			gs_texture_create_nv12(
				&tex, &tex_uv, info->width, info->height,
				GS_RENDER_TARGET | GS_SHARED_KM_TEX);
		}
		if (!tex) {
			return false;
		}

#ifdef _WIN32
		uint32_t handle = gs_texture_get_shared_handle(tex);
#else
		uint32_t handle = (uint32_t)-1;
#endif

		struct obs_tex_frame frame = {.tex = tex,
					      .tex_uv = tex_uv,
					      .handle = handle};

		deque_push_back(&video->gpu_encoder_avail_queue, &frame,
				sizeof(frame));
	}

	if (os_sem_init(&video->gpu_encode_semaphore, 0) != 0)
		return false;
	if (os_event_init(&video->gpu_encode_inactive, OS_EVENT_TYPE_MANUAL) !=
	    0)
		return false;
	if (pthread_create(&video->gpu_encode_thread, NULL, gpu_encode_thread,
			   video) != 0)
		return false;

	os_event_signal(video->gpu_encode_inactive);

	video->gpu_encode_thread_initialized = true;
	return true;
}

void stop_gpu_encoding_thread(struct obs_core_video_mix *video)
{
	if (video->gpu_encode_thread_initialized) {
		os_atomic_set_bool(&video->gpu_encode_stop, true);
		os_sem_post(video->gpu_encode_semaphore);
		pthread_join(video->gpu_encode_thread, NULL);
		video->gpu_encode_thread_initialized = false;
	}
}

void free_gpu_encoding(struct obs_core_video_mix *video)
{
	if (video->gpu_encode_semaphore) {
		os_sem_destroy(video->gpu_encode_semaphore);
		video->gpu_encode_semaphore = NULL;
	}
	if (video->gpu_encode_inactive) {
		os_event_destroy(video->gpu_encode_inactive);
		video->gpu_encode_inactive = NULL;
	}

#define free_deque(x)                                               \
	do {                                                        \
		while (x.size) {                                    \
			struct obs_tex_frame frame;                 \
			deque_pop_front(&x, &frame, sizeof(frame)); \
			gs_texture_destroy(frame.tex);              \
			gs_texture_destroy(frame.tex_uv);           \
		}                                                   \
		deque_free(&x);                                     \
	} while (false)

	free_deque(video->gpu_encoder_queue);
	free_deque(video->gpu_encoder_avail_queue);
#undef free_deque
}
