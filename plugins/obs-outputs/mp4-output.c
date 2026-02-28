/******************************************************************************
    Copyright (C) 2024 by Dennis Sädtler <dennis@obsproject.com>

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

#include "mp4-mux.h"

#include <inttypes.h>

#include <obs-module.h>
#include <util/platform.h>
#include <util/deque.h>
#include <util/dstr.h>
#include <util/threading.h>
#include <util/buffered-file-serializer.h>
#include <bpm.h>

#include <opts-parser.h>

#define do_log(level, format, ...)                                                                 \
	blog(level, "[%s output: '%s'] " format, out->muxer_flavor == FLAVOR_MOV ? "mov" : "mp4", \
	     obs_output_get_name(out->output), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

typedef DARRAY(struct encoder_packet) packet_array_t;

struct chapter {
	uint64_t ts;
	char *name;
};

struct mp4_output {
	obs_output_t *output;
	struct dstr path;

	/* File serializer buffer configuration */
	size_t buffer_size;
	size_t chunk_size;
	struct serializer serializer;

	bool enable_bpm;

	volatile bool active;
	volatile bool stopping;
	uint64_t stop_ts;

	bool allow_overwrite;
	uint64_t total_bytes;

	pthread_mutex_t mutex;

	struct mp4_mux *muxer;
	enum mp4_flavor muxer_flavor;
	int flags;

	size_t chapter_ctr;
	struct deque chapters;

	/* File splitting stuff */
	bool split_file_enabled;
	bool split_file_ready;
	volatile bool manual_split;

	size_t cur_size;
	size_t max_size;

	int64_t start_time;
	int64_t max_time;

	/* Replay buffer specifics */
	int64_t save_ts;
	bool discard_buffer;

	obs_hotkey_id hotkey;
	obs_hotkey_id hotkey_dump;

	volatile bool muxing;
	pthread_t mux_thread;
	bool mux_thread_joinable;

	struct deque packets;

	/* Buffer for packets while we reinitialise the muxer after splitting */
	packet_array_t split_buffer;
};

static inline bool stopping(struct mp4_output *out)
{
	return os_atomic_load_bool(&out->stopping);
}

static inline bool active(struct mp4_output *out)
{
	return os_atomic_load_bool(&out->active);
}

static inline int64_t packet_pts_usec(struct encoder_packet *packet)
{
	return packet->pts * 1000000 / packet->timebase_den;
}

static const char *mp4_output_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("MP4Output");
}

static const char *mov_output_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("MOVOutput");
}

static void mp4_clear_chapters(struct mp4_output *out)
{
	while (out->chapters.size) {
		struct chapter *chap = deque_data(&out->chapters, 0);
		bfree(chap->name);
		deque_pop_front(&out->chapters, NULL, sizeof(struct chapter));
	}
	out->chapter_ctr = 0;
}

static void mp4_output_destroy(void *data)
{
	struct mp4_output *out = data;

	pthread_mutex_destroy(&out->mutex);
	mp4_clear_chapters(out);
	deque_free(&out->chapters);
	dstr_free(&out->path);
	bfree(out);
}

void mp4_pkt_callback(obs_output_t *output, struct encoder_packet *pkt, struct encoder_packet_time *pkt_time,
		      void *param)
{
	UNUSED_PARAMETER(output);
	struct mp4_output *out = param;

	/* Only the primary video track is used as the reference for the chapter track. */
	if (!pkt_time || !pkt || pkt->type != OBS_ENCODER_VIDEO || pkt->track_idx != 0)
		return;

	pthread_mutex_lock(&out->mutex);
	struct chapter *chap = NULL;
	/* Write all queued chapters with a timestamp <= the current frame's composition time. */
	while (out->chapters.size) {
		chap = deque_data(&out->chapters, 0);
		if (chap->ts > pkt_time->cts)
			break;

		/* Video frames can be out of order (b-frames), so instead of using the video packet's dts_usec we need to calculate
		 * the chapter DTS from the frame's PTS (for chapters DTS == PTS). */
		int64_t chap_dts_usec = (pkt->pts * 1000000 / pkt->timebase_den) - out->start_time;
		int64_t chap_dts_msec = chap_dts_usec / 1000;
		int64_t chap_dts_sec = chap_dts_msec / 1000;

		int milliseconds = (int)(chap_dts_msec % 1000);
		int seconds = (int)chap_dts_sec % 60;
		int minutes = ((int)chap_dts_sec / 60) % 60;
		int hours = (int)chap_dts_sec / 3600;
		info("Adding chapter \"%s\" at %02d:%02d:%02d.%03d", chap->name, hours, minutes, seconds, milliseconds);

		mp4_mux_add_chapter(out->muxer, chap_dts_usec, chap->name);
		/* Free name and remove chapter from queue. */
		bfree(chap->name);
		deque_pop_front(&out->chapters, NULL, sizeof(struct chapter));
	}
	pthread_mutex_unlock(&out->mutex);
}

static void mp4_add_chapter_proc(void *data, calldata_t *cd)
{
	struct mp4_output *out = data;
	struct dstr name = {0};

	dstr_copy(&name, calldata_string(cd, "chapter_name"));

	if (name.len == 0) {
		/* Generate name if none provided. */
		dstr_catf(&name, "%s %zu", obs_module_text("MP4Output.UnnamedChapter"), out->chapter_ctr + 1);
	}

	pthread_mutex_lock(&out->mutex);
	/* Enqueue chapter marker to be inserted once a packet containing the video frame with the corresponding timestamp is
	 * being processed. */
	struct chapter chap = {0};
	chap.ts = obs_get_video_frame_time();
	chap.name = name.array;
	deque_push_back(&out->chapters, &chap, sizeof(struct chapter));
	out->chapter_ctr++;
	pthread_mutex_unlock(&out->mutex);
}

static void split_file_proc(void *data, calldata_t *cd)
{
	struct mp4_output *out = data;

	calldata_set_bool(cd, "split_file_enabled", out->split_file_enabled);
	if (!out->split_file_enabled)
		return;

	os_atomic_set_bool(&out->manual_split, true);
}

static void *mp4_output_create_internal(obs_data_t *settings, obs_output_t *output, enum mp4_flavor flavor)
{
	struct mp4_output *out = bzalloc(sizeof(struct mp4_output));
	out->output = output;
	out->muxer_flavor = flavor;
	pthread_mutex_init(&out->mutex, NULL);

	signal_handler_t *sh = obs_output_get_signal_handler(output);
	signal_handler_add(sh, "void file_changed(string next_file)");

	proc_handler_t *ph = obs_output_get_proc_handler(output);
	proc_handler_add(ph, "void split_file(out bool split_file_enabled)", split_file_proc, out);
	proc_handler_add(ph, "void add_chapter(string chapter_name)", mp4_add_chapter_proc, out);

	UNUSED_PARAMETER(settings);
	return out;
}

static void *mp4_output_create(obs_data_t *settings, obs_output_t *output)
{
	return mp4_output_create_internal(settings, output, FLAVOR_MP4);
}

static void *mov_output_create(obs_data_t *settings, obs_output_t *output)
{
	return mp4_output_create_internal(settings, output, FLAVOR_MOV);
}

static inline void apply_flag(int *flags, const char *value, int flag_value)
{
	if (atoi(value))
		*flags |= flag_value;
	else
		*flags &= ~flag_value;
}

static void parse_custom_options(struct mp4_output *out, const char *opts_str)
{
	int flags = MP4_USE_NEGATIVE_CTS;

	struct obs_options opts = obs_parse_options(opts_str);

	for (size_t i = 0; i < opts.count; i++) {
		struct obs_option opt = opts.options[i];

		if (strcmp(opt.name, "skip_soft_remux") == 0) {
			apply_flag(&flags, opt.value, MP4_SKIP_FINALISATION);
		} else if (strcmp(opt.name, "write_encoder_info") == 0) {
			apply_flag(&flags, opt.value, MP4_WRITE_ENCODER_INFO);
		} else if (strcmp(opt.name, "use_metadata_tags") == 0) {
			apply_flag(&flags, opt.value, MP4_USE_MDTA_KEY_VALUE);
		} else if (strcmp(opt.name, "use_negative_cts") == 0) {
			apply_flag(&flags, opt.value, MP4_USE_NEGATIVE_CTS);
		} else if (strcmp(opt.name, "buffer_size") == 0) {
			out->buffer_size = strtoull(opt.value, 0, 10) * 1048576ULL;
		} else if (strcmp(opt.name, "chunk_size") == 0) {
			out->chunk_size = strtoull(opt.value, 0, 10) * 1048576ULL;
		} else if (strcmp(opt.name, "bpm") == 0) {
			out->enable_bpm = !!atoi(opt.value);
		} else {
			blog(LOG_WARNING, "Unknown muxer option: %s = %s", opt.name, opt.value);
		}
	}

	obs_free_options(opts);

	out->flags = flags;
}

static void generate_filename(struct mp4_output *out, struct dstr *dst, bool overwrite);

static bool mp4_output_start(void *data)
{
	struct mp4_output *out = data;

	if (!obs_output_can_begin_data_capture(out->output, 0))
		return false;
	if (!obs_output_initialize_encoders(out->output, 0))
		return false;

	os_atomic_set_bool(&out->stopping, false);

	obs_data_t *settings = obs_output_get_settings(out->output);
	out->max_time = obs_data_get_int(settings, "max_time_sec") * 1000000LL;
	out->max_size = obs_data_get_int(settings, "max_size_mb") * 1024 * 1024;
	out->split_file_enabled = obs_data_get_bool(settings, "split_file");
	out->allow_overwrite = obs_data_get_bool(settings, "allow_overwrite");
	out->cur_size = 0;
	out->start_time = 0;

	/* Get path */
	const char *path = obs_data_get_string(settings, "path");
	if (path && *path) {
		dstr_copy(&out->path, path);
	} else {
		generate_filename(out, &out->path, out->allow_overwrite);
		info("Output path not specified. Using generated path '%s'", out->path.array);
	}

	/* Allow skipping the remux step for debugging purposes. */
	const char *muxer_settings = obs_data_get_string(settings, "muxer_settings");
	parse_custom_options(out, muxer_settings);

	obs_data_release(settings);

	if (out->enable_bpm) {
		info("Enabling BPM");
		obs_output_add_packet_callback(out->output, bpm_inject, NULL);
	}

	if (!buffered_file_serializer_init(&out->serializer, out->path.array, out->buffer_size, out->chunk_size)) {
		warn("Unable to open file '%s'", out->path.array);
		return false;
	}

	/* Add packet callback for accurate chapter markers. */
	obs_output_add_packet_callback(out->output, mp4_pkt_callback, (void *)out);

	/* Initialise muxer and start capture */
	out->muxer = mp4_mux_create(out->output, &out->serializer, out->flags, out->muxer_flavor);
	os_atomic_set_bool(&out->active, true);
	obs_output_begin_data_capture(out->output, 0);

	info("Writing Hybrid MP4/MOV file '%s'...", out->path.array);
	return true;
}

static inline bool should_split(struct mp4_output *out, struct encoder_packet *packet)
{
	/* split at video frame on primary track */
	if (packet->type != OBS_ENCODER_VIDEO || packet->track_idx > 0)
		return false;

	/* don't split group of pictures */
	if (!packet->keyframe)
		return false;

	if (os_atomic_load_bool(&out->manual_split))
		return true;

	/* reached maximum file size */
	if (out->max_size > 0 && out->cur_size + (int64_t)packet->size >= out->max_size)
		return true;

	/* reached maximum duration */
	if (out->max_time > 0 && packet->dts_usec - out->start_time >= out->max_time)
		return true;

	return false;
}

static void find_best_filename(struct dstr *path, bool space)
{
	int num = 2;

	if (!os_file_exists(path->array))
		return;

	const char *ext = strrchr(path->array, '.');
	if (!ext)
		return;

	size_t extstart = ext - path->array;
	struct dstr testpath;
	dstr_init_copy_dstr(&testpath, path);
	for (;;) {
		dstr_resize(&testpath, extstart);
		dstr_catf(&testpath, space ? " (%d)" : "_%d", num++);
		dstr_cat(&testpath, ext);

		if (!os_file_exists(testpath.array)) {
			dstr_free(path);
			dstr_init_move(path, &testpath);
			break;
		}
	}
}

static void generate_filename(struct mp4_output *out, struct dstr *dst, bool overwrite)
{
	obs_data_t *settings = obs_output_get_settings(out->output);
	const char *dir = obs_data_get_string(settings, "directory");
	const char *fmt = obs_data_get_string(settings, "format");
	const char *ext = obs_data_get_string(settings, "extension");
	bool space = obs_data_get_bool(settings, "allow_spaces");

	char *filename = os_generate_formatted_filename(ext, space, fmt);

	dstr_copy(dst, dir);
	dstr_replace(dst, "\\", "/");
	if (dstr_end(dst) != '/')
		dstr_cat_ch(dst, '/');
	dstr_cat(dst, filename);

	char *slash = strrchr(dst->array, '/');
	if (slash) {
		*slash = 0;
		os_mkdirs(dst->array);
		*slash = '/';
	}

	if (!overwrite)
		find_best_filename(dst, space);

	bfree(filename);
	obs_data_release(settings);
}

static bool change_file(struct mp4_output *out, struct encoder_packet *pkt)
{
	uint64_t start_time = os_gettime_ns();

	/* finalise file */
	mp4_mux_finalise(out->muxer);

	info("Waiting for file writer to finish...");

	/* flush/close file and destroy old muxer */
	buffered_file_serializer_free(&out->serializer);
	mp4_mux_destroy(out->muxer);
	mp4_clear_chapters(out);

	info("File split complete. Finalization took %" PRIu64 " ms.", (os_gettime_ns() - start_time) / 1000000);

	/* open new file */
	generate_filename(out, &out->path, out->allow_overwrite);
	info("Changing output file to '%s'", out->path.array);

	if (!buffered_file_serializer_init(&out->serializer, out->path.array, out->buffer_size, out->chunk_size)) {
		warn("Unable to open file '%s'", out->path.array);
		return false;
	}

	out->muxer = mp4_mux_create(out->output, &out->serializer, out->flags, out->muxer_flavor);

	calldata_t cd = {0};
	signal_handler_t *sh = obs_output_get_signal_handler(out->output);
	calldata_set_string(&cd, "next_file", out->path.array);
	signal_handler_signal(sh, "file_changed", &cd);
	calldata_free(&cd);

	out->cur_size = 0;
	out->start_time = pkt->dts_usec;

	return true;
}

static void mp4_output_stop(void *data, uint64_t ts)
{
	struct mp4_output *out = data;
	out->stop_ts = ts / 1000;
	os_atomic_set_bool(&out->stopping, true);
}

static void mp4_mux_destroy_task(void *ptr)
{
	struct mp4_mux *muxer = ptr;
	mp4_mux_destroy(muxer);
}

static void mp4_output_actual_stop(struct mp4_output *out, int code)
{
	os_atomic_set_bool(&out->active, false);
	obs_output_remove_packet_callback(out->output, mp4_pkt_callback, NULL);

	uint64_t start_time = os_gettime_ns();

	mp4_mux_finalise(out->muxer);

	if (out->enable_bpm) {
		obs_output_remove_packet_callback(out->output, bpm_inject, NULL);
		bpm_destroy(out->output);
	}

	if (code) {
		obs_output_signal_stop(out->output, code);
	} else {
		obs_output_end_data_capture(out->output);
	}

	info("Waiting for file writer to finish...");

	/* Flush/close output file and destroy muxer */
	buffered_file_serializer_free(&out->serializer);
	obs_queue_task(OBS_TASK_DESTROY, mp4_mux_destroy_task, out->muxer, false);
	out->muxer = NULL;

	/* Clear chapter data */
	mp4_clear_chapters(out);

	info("File output complete. Finalization took %" PRIu64 " ms.", (os_gettime_ns() - start_time) / 1000000);
}

static void push_back_packet(struct mp4_output *out, struct encoder_packet *packet)
{
	struct encoder_packet pkt;
	obs_encoder_packet_ref(&pkt, packet);
	da_push_back(out->split_buffer, &pkt);
}

static inline bool submit_packet(struct mp4_output *out, struct encoder_packet *pkt)
{
	out->total_bytes += pkt->size;
	out->cur_size += pkt->size;

	return mp4_mux_submit_packet(out->muxer, pkt);
}

static void mp4_output_packet(void *data, struct encoder_packet *packet)
{
	struct mp4_output *out = data;

	pthread_mutex_lock(&out->mutex);

	if (!active(out))
		goto unlock;

	if (!packet) {
		mp4_output_actual_stop(out, OBS_OUTPUT_ENCODE_ERROR);
		goto unlock;
	}

	if (stopping(out)) {
		if (packet->sys_dts_usec >= (int64_t)out->stop_ts) {
			mp4_output_actual_stop(out, 0);
			goto unlock;
		}
	}

	if (out->split_file_enabled) {
		if (out->split_buffer.num) {
			int64_t pts_usec = packet_pts_usec(packet);
			struct encoder_packet *first_pkt = out->split_buffer.array;
			int64_t first_pts_usec = packet_pts_usec(first_pkt);

			if (pts_usec >= first_pts_usec) {
				if (packet->type != OBS_ENCODER_AUDIO) {
					push_back_packet(out, packet);
					goto unlock;
				}

				if (!change_file(out, first_pkt)) {
					mp4_output_actual_stop(out, OBS_OUTPUT_ERROR);
					goto unlock;
				}
				out->split_file_ready = true;
			}
		} else if (should_split(out, packet)) {
			push_back_packet(out, packet);
			goto unlock;
		}
	}

	if (out->split_file_ready) {
		for (size_t i = 0; i < out->split_buffer.num; i++) {
			struct encoder_packet *pkt = &out->split_buffer.array[i];
			submit_packet(out, pkt);
			obs_encoder_packet_release(pkt);
		}

		da_free(out->split_buffer);
		out->split_file_ready = false;
		os_atomic_set_bool(&out->manual_split, false);
	}

	submit_packet(out, packet);

	if (serializer_get_pos(&out->serializer) == -1)
		mp4_output_actual_stop(out, OBS_OUTPUT_ERROR);

unlock:
	pthread_mutex_unlock(&out->mutex);
}

static obs_properties_t *mp4_output_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "path", obs_module_text("MP4Output.FilePath"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "muxer_settings", "muxer_settings", OBS_TEXT_DEFAULT);
	return props;
}

uint64_t mp4_output_total_bytes(void *data)
{
	struct mp4_output *out = data;
	return out->total_bytes;
}

struct obs_output_info mp4_output_info = {
	.id = "mp4_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK_AV | OBS_OUTPUT_CAN_PAUSE,
	.encoded_video_codecs = "h264;hevc;av1",
	.encoded_audio_codecs = "aac;alac;flac;opus",
	.get_name = mp4_output_name,
	.create = mp4_output_create,
	.destroy = mp4_output_destroy,
	.start = mp4_output_start,
	.stop = mp4_output_stop,
	.encoded_packet = mp4_output_packet,
	.get_properties = mp4_output_properties,
	.get_total_bytes = mp4_output_total_bytes,
};

struct obs_output_info mov_output_info = {
	.id = "mov_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK_AV | OBS_OUTPUT_CAN_PAUSE,
	.encoded_video_codecs = "h264;hevc;prores",
	.encoded_audio_codecs = "aac;alac",
	.get_name = mov_output_name,
	.create = mov_output_create,
	.destroy = mp4_output_destroy,
	.start = mp4_output_start,
	.stop = mp4_output_stop,
	.encoded_packet = mp4_output_packet,
	.get_properties = mp4_output_properties,
	.get_total_bytes = mp4_output_total_bytes,
};

/* ------------------------------------------------------------------------ */
/* Replay Buffer */
/* ------------------------------------------------------------------------ */

struct replay_thread_params {
	volatile bool *muxing;
	packet_array_t packets;
	obs_output_t *output;
	enum mp4_flavor flavor;
	int muxer_flags;
	struct dstr path;
};

static void free_thread_params(struct replay_thread_params *params)
{
	da_free(params->packets);
	dstr_free(&params->path);
	bfree(params);
}

static const char *mp4_replay_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("MP4Replay");
}

static const char *mov_replay_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("MOVReplay");
}

static void mp4_replay_save_hotkey_internal(void *data, bool discard)
{
	struct mp4_output *out = data;

	if (os_atomic_load_bool(&out->active)) {
		obs_encoder_t *vencoder = obs_output_get_video_encoder(out->output);
		if (obs_encoder_paused(vencoder)) {
			info("Could not save buffer because encoders paused");
			return;
		}

		out->save_ts = os_gettime_ns() / 1000LL;
		out->discard_buffer = discard;
	}
}

static void mp4_replay_save_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (pressed) {
		mp4_replay_save_hotkey_internal(data, false);
	}
}

static void mp4_replay_save_discard_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (pressed) {
		mp4_replay_save_hotkey_internal(data, true);
	}
}

static void mp4_replay_save_proc(void *data, calldata_t *cd)
{
	mp4_replay_save_hotkey(data, 0, NULL, true);
	UNUSED_PARAMETER(cd);
}

static void mp4_replay_save_discard_proc(void *data, calldata_t *cd)
{
	mp4_replay_save_discard_hotkey(data, 0, NULL, true);
	UNUSED_PARAMETER(cd);
}

static void mp4_replay_last_file(void *data, calldata_t *cd)
{
	struct mp4_output *out = data;

	if (!os_atomic_load_bool(&out->muxing))
		calldata_set_string(cd, "path", out->path.array);
}

static void *mp4_replay_create_internal(obs_data_t *settings, obs_output_t *output, enum mp4_flavor flavor)
{
	struct mp4_output *out = bzalloc(sizeof(struct mp4_output));
	out->output = output;
	out->muxer_flavor = flavor;
	pthread_mutex_init(&out->mutex, NULL);

	out->hotkey = obs_hotkey_register_output(output, "MP4Replay.Save", obs_module_text("MP4Replay.Save"),
						 mp4_replay_save_hotkey, out);
	out->hotkey_dump = obs_hotkey_register_output(output, "MP4Replay.SaveDiscard",
						      obs_module_text("MP4Replay.SaveDiscard"),
						      mp4_replay_save_discard_hotkey, out);

	proc_handler_t *ph = obs_output_get_proc_handler(output);
	proc_handler_add(ph, "void save()", mp4_replay_save_proc, out);
	proc_handler_add(ph, "void save_discard()", mp4_replay_save_discard_proc, out);
	proc_handler_add(ph, "void get_last_replay(out string path)", mp4_replay_last_file, out);

	signal_handler_t *sh = obs_output_get_signal_handler(output);
	signal_handler_add(sh, "void saved()");

	UNUSED_PARAMETER(settings);
	return out;
}

static void *mp4_replay_create(obs_data_t *settings, obs_output_t *output)
{
	return mp4_replay_create_internal(settings, output, FLAVOR_MP4);
}

static void *mov_replay_create(obs_data_t *settings, obs_output_t *output)
{
	return mp4_replay_create_internal(settings, output, FLAVOR_MOV);
}

static void mp4_replay_buffer_clear(struct mp4_output *out)
{
	while (out->packets.size > 0) {
		struct encoder_packet pkt;
		deque_pop_front(&out->packets, &pkt, sizeof(pkt));
		obs_encoder_packet_release(&pkt);
	}

	deque_free(&out->packets);
}

static void mp4_replay_destroy(void *data)
{
	struct mp4_output *out = data;

	if (out->hotkey)
		obs_hotkey_unregister(out->hotkey);
	if (out->hotkey_dump)
		obs_hotkey_unregister(out->hotkey_dump);
	if (out->mux_thread_joinable)
		pthread_join(out->mux_thread, NULL);

	mp4_replay_buffer_clear(out);
	mp4_output_destroy(data);
}

static bool mp4_replay_start(void *data)
{
	struct mp4_output *out = data;

	if (!obs_output_can_begin_data_capture(out->output, 0))
		return false;
	if (!obs_output_initialize_encoders(out->output, 0))
		return false;

	obs_data_t *s = obs_output_get_settings(out->output);
	out->max_time = obs_data_get_int(s, "max_time_sec") * 1000000LL;
	out->max_size = obs_data_get_int(s, "max_size_mb") * (1024 * 1024);
	/* Parse custom options. */
	const char *muxer_settings = obs_data_get_string(s, "muxer_settings");
	parse_custom_options(out, muxer_settings);
	obs_data_release(s);

	os_atomic_set_bool(&out->active, true);
	out->total_bytes = 0;
	out->cur_size = 0;
	out->start_time = 0;
	out->save_ts = 0;

	obs_output_begin_data_capture(out->output, 0);

	return true;
}

static inline void purge_one(struct mp4_output *out)
{
	struct encoder_packet removed, first;

	deque_pop_front(&out->packets, &removed, sizeof(struct encoder_packet));
	out->cur_size -= (int64_t)removed.size;
	obs_encoder_packet_release(&removed);

	if (!out->packets.size) {
		out->start_time = 0;
		return;
	}

	/* Update start time to current front packet */
	deque_peek_front(&out->packets, &first, sizeof(struct encoder_packet));
	out->start_time = first.dts_usec;
}

static inline bool buffer_too_large(struct mp4_output *out, struct encoder_packet *packet)
{
	if (!out->packets.size)
		return false;
	if (out->max_size && (out->cur_size + packet->size) < out->max_size)
		return false;
	if (out->max_time && (packet->dts_usec - out->start_time) < out->max_time)
		return false;

	return true;
}

static void mp4_replay_buffer_purge(struct mp4_output *out, struct encoder_packet *packet)
{
	struct encoder_packet *front = deque_data(&out->packets, 0);

	/* Keep dropping packets until the buffer is below the thresholds *and* the first packet is a video keyframe. */
	while (front && (buffer_too_large(out, packet) || front->type != OBS_ENCODER_VIDEO || !front->keyframe)) {
		purge_one(out);
		front = deque_data(&out->packets, 0);
	}
}

static void *mp4_replay_mux_thread(void *data)
{
	bool error = false;
	struct replay_thread_params *params = data;
	struct mp4_mux *muxer = NULL;
	struct serializer serializer = {0};

	if (!buffered_file_serializer_init(&serializer, params->path.array, 0, 0)) {
		blog(LOG_ERROR, "[replay muxer thread] Cannot open file '%s'", params->path.array);
		error = true;
	}

	if (!error) {
		muxer = mp4_mux_create(params->output, &serializer, params->muxer_flags, params->flavor);

		for (size_t idx = 0; idx < params->packets.num; idx++) {
			struct encoder_packet *pkt = &params->packets.array[idx];
			if (!error)
				error = mp4_mux_submit_packet(muxer, pkt);
			/* Even if there was an error we still need to release the packets. */
			obs_encoder_packet_release(pkt);
		}

		if (!error)
			error = mp4_mux_finalise(muxer);
	}

	os_atomic_set_bool(params->muxing, false);

	if (!error) {
		calldata_t cd = {0};
		signal_handler_t *sh = obs_output_get_signal_handler(params->output);
		signal_handler_signal(sh, "saved", &cd);
	}

	buffered_file_serializer_free(&serializer);
	free_thread_params(params);

	if (muxer)
		obs_queue_task(OBS_TASK_DESTROY, mp4_mux_destroy_task, muxer, false);

	return NULL;
}

static inline bool first_packet_is_less_than(struct deque *dq, int64_t dts_usec)
{
	if (!dq->size)
		return false;

	struct encoder_packet *pkt = deque_data(dq, 0);
	return pkt->dts_usec < dts_usec;
}

static void mp4_replay_save(struct mp4_output *out)
{
	const size_t size = sizeof(struct encoder_packet);
	size_t num_packets = out->packets.size / size;

	struct replay_thread_params *tp = bzalloc(sizeof(struct replay_thread_params));
	tp->muxing = &out->muxing;
	tp->flavor = out->muxer_flavor;
	tp->output = out->output;
	tp->muxer_flags = out->flags;

	/* Store all packets we want to mux */
	int64_t last_idr_dts = 0;

	for (size_t idx = 0; idx < num_packets; idx++) {
		struct encoder_packet *pkt = deque_data(&out->packets, idx * size);
		if (pkt->type == OBS_ENCODER_VIDEO && pkt->keyframe)
			last_idr_dts = pkt->dts_usec;

		struct encoder_packet *arr_pkt = da_push_back_new(tp->packets);
		obs_encoder_packet_ref(arr_pkt, pkt);
	}

	/* If discard hotkey was used, discard all frames up to the last IDR. */
	while (out->discard_buffer && first_packet_is_less_than(&out->packets, last_idr_dts)) {
		purge_one(out);
	}

	generate_filename(out, &out->path, true);
	dstr_copy_dstr(&tp->path, &out->path);

	os_atomic_set_bool(&out->muxing, true);
	out->mux_thread_joinable = pthread_create(&out->mux_thread, NULL, mp4_replay_mux_thread, tp) == 0;
	if (!out->mux_thread_joinable) {
		warn("Failed to create muxer thread");
		os_atomic_set_bool(&out->muxing, false);
	}
}

static void mp4_replay_deactivate(struct mp4_output *out, int code)
{
	if (code) {
		obs_output_signal_stop(out->output, code);
	} else if (stopping(out)) {
		obs_output_end_data_capture(out->output);
	}

	os_atomic_set_bool(&out->active, false);
	os_atomic_set_bool(&out->stopping, false);
	mp4_replay_buffer_clear(out);
}

static void mp4_replay_packet(void *data, struct encoder_packet *packet)
{
	struct mp4_output *out = data;

	struct encoder_packet pkt;

	if (!active(out))
		return;

	/* encoder failure */
	if (!packet) {
		mp4_replay_deactivate(out, OBS_OUTPUT_ENCODE_ERROR);
		return;
	}

	if (stopping(out)) {
		if (packet->sys_dts_usec >= (int64_t)out->stop_ts) {
			mp4_replay_deactivate(out, 0);
			return;
		}
	}

	obs_encoder_packet_ref(&pkt, packet);
	mp4_replay_buffer_purge(out, packet);

	if (!out->packets.size)
		out->start_time = pkt.dts_usec;

	out->cur_size += pkt.size;

	deque_push_back(&out->packets, packet, sizeof(struct encoder_packet));

	if (out->save_ts && packet->sys_dts_usec >= out->save_ts) {
		if (os_atomic_load_bool(&out->muxing))
			return;

		if (out->mux_thread_joinable) {
			pthread_join(out->mux_thread, NULL);
			out->mux_thread_joinable = false;
		}

		out->save_ts = 0;
		mp4_replay_save(out);
	}
}

static void mp4_replay_defaults(obs_data_t *s)
{
	obs_data_set_default_int(s, "max_time_sec", 15);
	obs_data_set_default_int(s, "max_size_mb", 500);
	obs_data_set_default_string(s, "format", "%CCYY-%MM-%DD %hh-%mm-%ss");
	obs_data_set_default_bool(s, "allow_spaces", true);
}

struct obs_output_info mp4_replay_info = {
	.id = "mp4_replay_buffer",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK_AV | OBS_OUTPUT_CAN_PAUSE,
	.encoded_video_codecs = "h264;hevc;av1",
	.encoded_audio_codecs = "aac;alac;flac;opus",
	.get_name = mp4_replay_name,
	.create = mp4_replay_create,
	.destroy = mp4_replay_destroy,
	.start = mp4_replay_start,
	.stop = mp4_output_stop,
	.encoded_packet = mp4_replay_packet,
	.get_total_bytes = mp4_output_total_bytes,
	.get_defaults = mp4_replay_defaults,
};

struct obs_output_info mov_replay_info = {
	.id = "mov_replay_buffer",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK_AV | OBS_OUTPUT_CAN_PAUSE,
	.encoded_video_codecs = "h264;hevc;prores",
	.encoded_audio_codecs = "aac;alac",
	.get_name = mov_replay_name,
	.create = mov_replay_create,
	.destroy = mp4_replay_destroy,
	.start = mp4_replay_start,
	.stop = mp4_output_stop,
	.encoded_packet = mp4_replay_packet,
	.get_total_bytes = mp4_output_total_bytes,
	.get_defaults = mp4_replay_defaults,
};
