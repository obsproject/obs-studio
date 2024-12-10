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
#include <util/dstr.h>
#include <util/threading.h>
#include <util/buffered-file-serializer.h>

#include <opts-parser.h>

#define do_log(level, format, ...) \
	blog(level, "[mp4 output: '%s'] " format, obs_output_get_name(out->output), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

struct chapter {
	int64_t dts_usec;
	char *name;
};

struct mp4_output {
	obs_output_t *output;
	struct dstr path;

	struct serializer serializer;

	volatile bool active;
	volatile bool stopping;
	uint64_t stop_ts;

	bool allow_overwrite;
	uint64_t total_bytes;

	pthread_mutex_t mutex;

	struct mp4_mux *muxer;
	int flags;

	int64_t last_dts_usec;
	DARRAY(struct chapter) chapters;

	/* File splitting stuff */
	bool split_file_enabled;
	bool split_file_ready;
	volatile bool manual_split;

	size_t cur_size;
	size_t max_size;

	int64_t start_time;
	int64_t max_time;

	bool found_video[MAX_OUTPUT_VIDEO_ENCODERS];
	bool found_audio[MAX_OUTPUT_AUDIO_ENCODERS];
	int64_t video_pts_offsets[MAX_OUTPUT_VIDEO_ENCODERS];
	int64_t audio_dts_offsets[MAX_OUTPUT_AUDIO_ENCODERS];

	/* Buffer for packets while we reinitialise the muxer after splitting */
	DARRAY(struct encoder_packet) split_buffer;
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

static inline void ts_offset_clear(struct mp4_output *out)
{
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		out->found_video[i] = false;
		out->video_pts_offsets[i] = 0;
	}

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		out->found_audio[i] = false;
		out->audio_dts_offsets[i] = 0;
	}
}

static inline void ts_offset_update(struct mp4_output *out, struct encoder_packet *packet)
{
	int64_t *offset;
	int64_t ts;
	bool *found;

	if (packet->type == OBS_ENCODER_VIDEO) {
		offset = &out->video_pts_offsets[packet->track_idx];
		found = &out->found_video[packet->track_idx];
		ts = packet->pts;
	} else {
		offset = &out->audio_dts_offsets[packet->track_idx];
		found = &out->found_audio[packet->track_idx];
		ts = packet->dts;
	}

	if (*found)
		return;

	*offset = ts;
	*found = true;
}

static const char *mp4_output_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("MP4Output");
}

static void mp4_output_destory(void *data)
{
	struct mp4_output *out = data;

	for (size_t i = 0; i < out->chapters.num; i++)
		bfree(out->chapters.array[i].name);
	da_free(out->chapters);

	pthread_mutex_destroy(&out->mutex);
	dstr_free(&out->path);
	bfree(out);
}

static void mp4_add_chapter_proc(void *data, calldata_t *cd)
{
	struct mp4_output *out = data;
	struct dstr name = {0};

	dstr_copy(&name, calldata_string(cd, "chapter_name"));

	if (name.len == 0) {
		/* Generate name if none provided. */
		dstr_catf(&name, "%s %zu", obs_module_text("MP4Output.UnnamedChapter"), out->chapters.num + 1);
	}

	int64_t totalRecordSeconds = out->last_dts_usec / 1000 / 1000;
	int seconds = (int)totalRecordSeconds % 60;
	int totalMinutes = (int)totalRecordSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	info("Adding chapter \"%s\" at %02d:%02d:%02d", name.array, hours, minutes, seconds);

	pthread_mutex_lock(&out->mutex);
	struct chapter *chap = da_push_back_new(out->chapters);
	chap->dts_usec = out->last_dts_usec;
	chap->name = name.array;
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

static void *mp4_output_create(obs_data_t *settings, obs_output_t *output)
{
	struct mp4_output *out = bzalloc(sizeof(struct mp4_output));
	out->output = output;
	pthread_mutex_init(&out->mutex, NULL);

	signal_handler_t *sh = obs_output_get_signal_handler(output);
	signal_handler_add(sh, "void file_changed(string next_file)");

	proc_handler_t *ph = obs_output_get_proc_handler(output);
	proc_handler_add(ph, "void split_file(out bool split_file_enabled)", split_file_proc, out);
	proc_handler_add(ph, "void add_chapter(string chapter_name)", mp4_add_chapter_proc, out);

	UNUSED_PARAMETER(settings);
	return out;
}

static inline void apply_flag(int *flags, const char *value, int flag_value)
{
	if (atoi(value))
		*flags |= flag_value;
	else
		*flags &= ~flag_value;
}

static int parse_custom_options(const char *opts_str)
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
		} else {
			blog(LOG_WARNING, "Unknown muxer option: %s = %s", opt.name, opt.value);
		}
	}

	obs_free_options(opts);

	return flags;
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
	out->flags = parse_custom_options(muxer_settings);

	obs_data_release(settings);

	if (!buffered_file_serializer_init_defaults(&out->serializer, out->path.array)) {
		warn("Unable to open MP4 file '%s'", out->path.array);
		return false;
	}

	/* Initialise muxer and start capture */
	out->muxer = mp4_mux_create(out->output, &out->serializer, out->flags);
	os_atomic_set_bool(&out->active, true);
	obs_output_begin_data_capture(out->output, 0);

	info("Writing Hybrid MP4 file '%s'...", out->path.array);
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
	for (size_t i = 0; i < out->chapters.num; i++) {
		struct chapter *chap = &out->chapters.array[i];
		mp4_mux_add_chapter(out->muxer, chap->dts_usec, chap->name);
	}

	mp4_mux_finalise(out->muxer);

	info("Waiting for file writer to finish...");

	/* flush/close file and destroy old muxer */
	buffered_file_serializer_free(&out->serializer);
	mp4_mux_destroy(out->muxer);

	for (size_t i = 0; i < out->chapters.num; i++)
		bfree(out->chapters.array[i].name);

	da_clear(out->chapters);

	info("MP4 file split complete. Finalization took %" PRIu64 " ms.", (os_gettime_ns() - start_time) / 1000000);

	/* open new file */
	generate_filename(out, &out->path, out->allow_overwrite);
	info("Changing output file to '%s'", out->path.array);

	if (!buffered_file_serializer_init_defaults(&out->serializer, out->path.array)) {
		warn("Unable to open MP4 file '%s'", out->path.array);
		return false;
	}

	out->muxer = mp4_mux_create(out->output, &out->serializer, out->flags);

	calldata_t cd = {0};
	signal_handler_t *sh = obs_output_get_signal_handler(out->output);
	calldata_set_string(&cd, "next_file", out->path.array);
	signal_handler_signal(sh, "file_changed", &cd);
	calldata_free(&cd);

	out->cur_size = 0;
	out->start_time = pkt->dts_usec;
	ts_offset_clear(out);

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

	uint64_t start_time = os_gettime_ns();

	for (size_t i = 0; i < out->chapters.num; i++) {
		struct chapter *chap = &out->chapters.array[i];
		mp4_mux_add_chapter(out->muxer, chap->dts_usec, chap->name);
	}

	mp4_mux_finalise(out->muxer);

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
	for (size_t i = 0; i < out->chapters.num; i++)
		bfree(out->chapters.array[i].name);

	da_clear(out->chapters);

	info("MP4 file output complete. Finalization took %" PRIu64 " ms.", (os_gettime_ns() - start_time) / 1000000);
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

	if (!out->split_file_enabled)
		return mp4_mux_submit_packet(out->muxer, pkt);

	out->cur_size += pkt->size;

	/* Apply DTS/PTS offset local packet copy */
	struct encoder_packet modified = *pkt;

	if (modified.type == OBS_ENCODER_VIDEO) {
		modified.dts -= out->video_pts_offsets[modified.track_idx];
		modified.pts -= out->video_pts_offsets[modified.track_idx];
	} else {
		modified.dts -= out->audio_dts_offsets[modified.track_idx];
		modified.pts -= out->audio_dts_offsets[modified.track_idx];
	}

	return mp4_mux_submit_packet(out->muxer, &modified);
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
			ts_offset_update(out, pkt);
			submit_packet(out, pkt);
			obs_encoder_packet_release(pkt);
		}

		da_free(out->split_buffer);
		out->split_file_ready = false;
		os_atomic_set_bool(&out->manual_split, false);
	}

	if (out->split_file_enabled)
		ts_offset_update(out, packet);

	/* Update PTS for chapter markers */
	if (packet->type == OBS_ENCODER_VIDEO && packet->track_idx == 0)
		out->last_dts_usec = packet->dts_usec - out->start_time;

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
	.encoded_audio_codecs = "aac",
	.get_name = mp4_output_name,
	.create = mp4_output_create,
	.destroy = mp4_output_destory,
	.start = mp4_output_start,
	.stop = mp4_output_stop,
	.encoded_packet = mp4_output_packet,
	.get_properties = mp4_output_properties,
	.get_total_bytes = mp4_output_total_bytes,
};
