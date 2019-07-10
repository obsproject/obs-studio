/*
 * Copyright (c) 2015 John R. Bradley <jrb@turrettech.com>
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

#include <obs-module.h>
#include <util/platform.h>
#include <util/dstr.h>

#include "obs-ffmpeg-compat.h"
#include "obs-ffmpeg-formats.h"

#include <media-playback/media.h>

void start_video(void *data);
void ffmpeg_source_playlist_next(void *data);
void ffmpeg_source_play_pause(void *data, bool pause);
void ffmpeg_source_restart(void *data);
void ffmpeg_source_stop(void *data);

#define FF_LOG(level, format, ...) \
	blog(level, "[Media Source]: " format, ##__VA_ARGS__)
#define FF_LOG_S(source, level, format, ...)        \
	blog(level, "[Media Source '%s']: " format, \
	     obs_source_get_name(source), ##__VA_ARGS__)
#define FF_BLOG(level, format, ...) \
	FF_LOG_S(s->source, level, format, ##__VA_ARGS__)

enum behavior {
	BEHAVIOR_STOP_RESTART,
	BEHAVIOR_PAUSE_UNPAUSE,
	BEHAVIOR_ALWAYS_PLAY,
};

struct playlist_data {
	char *path;
	bool is_url;
};

struct ffmpeg_source {
	mp_media_t media;
	bool media_valid;
	bool destroy_media;

	struct SwsContext *sws_ctx;
	int sws_width;
	int sws_height;
	enum AVPixelFormat sws_format;
	uint8_t *sws_data;
	int sws_linesize;
	enum video_range_type range;
	obs_source_t *source;
	obs_hotkey_id hotkey;

	char *input;
	int buffering_mb;
	int speed_percent;
	bool is_looping;
	bool is_hw_decoding;
	bool is_clear_on_media_end;

	enum obs_media_state state;
	obs_hotkey_pair_id play_pause_hotkey;
	obs_hotkey_id stop_hotkey;
	obs_hotkey_id playlist_next_hotkey;
	obs_hotkey_id playlist_prev_hotkey;

	bool shuffle;
	bool next;
	bool restart;
	bool stop;
	size_t cur_item;
	DARRAY(struct playlist_data) files;
	enum behavior behavior;
};

static void set_media_state(void *data, enum obs_media_state state)
{
	struct ffmpeg_source *s = data;
	s->state = state;
}

static void add_file(struct ffmpeg_source *s, struct darray *array,
		     const char *path, bool url)
{
	DARRAY(struct playlist_data) new_files;
	struct playlist_data data;

	new_files.da = *array;

	data.path = bstrdup(path);
	data.is_url = url;
	da_push_back(new_files, &data);

	*array = new_files.da;

	UNUSED_PARAMETER(s);
}

static void free_files(struct darray *array)
{
	DARRAY(struct playlist_data) files;
	files.da = *array;

	for (size_t i = 0; i < files.num; i++)
		bfree(files.array[i].path);

	da_free(files);
}

static void ffmpeg_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "looping", true);
	obs_data_set_default_bool(settings, "clear_on_media_end", true);
#if defined(_WIN32)
	obs_data_set_default_bool(settings, "hw_decode", true);
#endif
	obs_data_set_default_int(settings, "buffering_mb", 2);
	obs_data_set_default_int(settings, "speed_percent", 100);
}

static const char *media_filter =
	" (*.mp4 *.ts *.mov *.flv *.mkv *.avi *.mp3 *.ogg *.aac *.wav *.gif *.webm);;";
static const char *video_filter =
	" (*.mp4 *.ts *.mov *.flv *.mkv *.avi *.gif *.webm);;";
static const char *audio_filter = " (*.mp3 *.aac *.ogg *.wav);;";

static obs_properties_t *ffmpeg_source_getproperties(void *data)
{
	struct ffmpeg_source *s = data;
	struct dstr filter = {0};
	struct dstr path = {0};
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);

	dstr_copy(&filter, obs_module_text("MediaFileFilter.AllMediaFiles"));
	dstr_cat(&filter, media_filter);
	dstr_cat(&filter, obs_module_text("MediaFileFilter.VideoFiles"));
	dstr_cat(&filter, video_filter);
	dstr_cat(&filter, obs_module_text("MediaFileFilter.AudioFiles"));
	dstr_cat(&filter, audio_filter);
	dstr_cat(&filter, obs_module_text("MediaFileFilter.AllFiles"));
	dstr_cat(&filter, " (*.*)");

	if (s && s->input && *s->input) {
		const char *slash;

		dstr_copy(&path, s->input);
		dstr_replace(&path, "\\", "/");
		slash = strrchr(path.array, '/');
		if (slash)
			dstr_resize(&path, slash - path.array + 1);
	}

	obs_property_t *prop;

	obs_properties_add_bool(props, "looping", obs_module_text("Looping"));
	obs_properties_add_bool(props, "shuffle", obs_module_text("Shuffle"));

	prop = obs_properties_add_list(props, "behavior", "Behavior",
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(
		prop, obs_module_text("BehaviorStopRestart"), "stop_restart");
	obs_property_list_add_string(prop,
				     obs_module_text("BehaviorPauseUnpause"),
				     "behavior_pause_unpause");
	obs_property_list_add_string(prop,
				     obs_module_text("BehaviorAlwaysPlay"),
				     "behavior_always_play");

	obs_properties_add_editable_list(props, "files", "Playlist",
					 OBS_EDITABLE_LIST_TYPE_FILES_AND_URLS,
					 filter.array, path.array);
	dstr_free(&filter);
	dstr_free(&path);

	prop = obs_properties_add_int_slider(props, "buffering_mb",
					     obs_module_text("BufferingMB"), 1,
					     16, 1);
	obs_property_int_set_suffix(prop, " MB");

#ifndef __APPLE__
	obs_properties_add_bool(props, "hw_decode",
				obs_module_text("HardwareDecode"));
#endif

	obs_properties_add_bool(props, "clear_on_media_end",
				obs_module_text("ClearOnMediaEnd"));

	obs_property_set_long_description(
		prop, obs_module_text("CloseFileWhenInactive.ToolTip"));

	prop = obs_properties_add_int_slider(props, "speed_percent",
					     obs_module_text("SpeedPercentage"),
					     1, 200, 1);
	obs_property_int_set_suffix(prop, "%");

	prop = obs_properties_add_list(props, "color_range",
				       obs_module_text("ColorRange"),
				       OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(prop, obs_module_text("ColorRange.Auto"),
				  VIDEO_RANGE_DEFAULT);
	obs_property_list_add_int(prop, obs_module_text("ColorRange.Partial"),
				  VIDEO_RANGE_PARTIAL);
	obs_property_list_add_int(prop, obs_module_text("ColorRange.Full"),
				  VIDEO_RANGE_FULL);

	return props;
}

static void dump_source_info(struct ffmpeg_source *s, const char *input)
{
	const char *behaviors[] = {"Stop/Restart", "Pause/Unpause",
				   "Always Play"};

	FF_BLOG(LOG_INFO,
		"settings:\n"
		"\tinput:                   %s\n"
		"\tspeed:                   %d\n"
		"\tis_looping:              %s\n"
		"\tshuffle:                 %s\n"
		"\tbehavior:                %s\n"
		"\tis_hw_decoding:          %s\n"
		"\tis_clear_on_media_end:   %s\n",
		input ? input : "(null)", s->speed_percent,
		s->is_looping ? "yes" : "no", s->shuffle ? "yes" : "no",
		behaviors[s->behavior], s->is_hw_decoding ? "yes" : "no",
		s->is_clear_on_media_end ? "yes" : "no");
}

static void get_frame(void *opaque, struct obs_source_frame *f)
{
	struct ffmpeg_source *s = opaque;
	obs_source_output_video(s->source, f);
}

static void preload_frame(void *opaque, struct obs_source_frame *f)
{
	struct ffmpeg_source *s = opaque;
	if (s->behavior == BEHAVIOR_STOP_RESTART)
		return;

	if (s->is_clear_on_media_end || s->is_looping)
		obs_source_preload_video(s->source, f);
}

static void get_audio(void *opaque, struct obs_source_audio *a)
{
	struct ffmpeg_source *s = opaque;
	obs_source_output_audio(s->source, a);
}

static void media_stopped(void *opaque)
{
	struct ffmpeg_source *s = opaque;

	if (s->cur_item == s->files.num - 1 && s->is_looping) {
		s->restart = true;
		return;
	}

	if (s->cur_item == s->files.num - 1 && s->is_clear_on_media_end &&
	    !s->is_looping) {
		s->stop = true;
		obs_source_media_ended(s->source);
		return;
	}

	if (s->cur_item == s->files.num - 1 && !s->is_clear_on_media_end &&
	    !s->is_looping) {
		s->state = OBS_MEDIA_STATE_STOPPED;
		obs_source_media_ended(s->source);
		return;
	}

	s->next = true;
}

static void ffmpeg_source_open(struct ffmpeg_source *s)
{
	if (s->input && *s->input) {
		struct mp_media_info info = {
			.opaque = s,
			.v_cb = get_frame,
			.v_preload_cb = preload_frame,
			.a_cb = get_audio,
			.stop_cb = media_stopped,
			.path = s->input,
			.format = NULL,
			.buffering = s->buffering_mb * 1024 * 1024,
			.speed = s->speed_percent,
			.force_range = s->range,
			.hardware_decoding = s->is_hw_decoding,
			.is_local_file = !s->files.array[s->cur_item].is_url};

		s->media_valid = mp_media_init(&s->media, &info);
	}
}

static void ffmpeg_source_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);

	struct ffmpeg_source *s = data;

	if (s->restart) {
		obs_source_media_restart(s->source);
		s->restart = false;
	}

	if (s->stop) {
		obs_source_media_stop(s->source);
		s->stop = false;
	}

	if (s->next) {
		obs_source_media_next(s->source);
		s->next = false;
	}
}

static void ffmpeg_source_start(struct ffmpeg_source *s)
{
	if (!s->media_valid)
		ffmpeg_source_open(s);

	if (s->media_valid) {
		mp_media_play(&s->media);
		if (!s->files.array[s->cur_item].is_url)
			obs_source_show_preloaded_video(s->source);
		set_media_state(s, OBS_MEDIA_STATE_PLAYING);
	}
}

static bool valid_extension(const char *ext)
{
	if (!ext)
		return false;
	return astrcmpi(ext, ".mp4") == 0 || astrcmpi(ext, ".ts") == 0 ||
	       astrcmpi(ext, ".mov") == 0 || astrcmpi(ext, ".flv") == 0 ||
	       astrcmpi(ext, ".mkv") == 0 || astrcmpi(ext, ".avi") == 0 ||
	       astrcmpi(ext, ".mp3") == 0 || astrcmpi(ext, ".ogg") == 0 ||
	       astrcmpi(ext, ".aac") == 0 || astrcmpi(ext, ".wav") == 0 ||
	       astrcmpi(ext, ".gif") == 0 || astrcmpi(ext, ".webm") == 0;
}

static void shuffle_playlist(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->files.num > 1 && s->shuffle) {
		DARRAY(struct playlist_data) new_files;
		DARRAY(size_t) idxs;

		da_init(new_files);
		da_init(idxs);
		da_resize(idxs, s->files.num);
		da_reserve(new_files, s->files.num);

		for (size_t i = 0; i < s->files.num; i++) {
			idxs.array[i] = i;
		}
		for (size_t i = idxs.num; i > 0; i--) {
			size_t val = rand() % i;
			size_t idx = idxs.array[val];
			da_push_back(new_files, &s->files.array[idx]);
			da_erase(idxs, val);
		}

		da_free(s->files);
		da_free(idxs);
		s->files.da = new_files.da;
	}
}

static void ffmpeg_source_update(void *data, obs_data_t *settings)
{
	DARRAY(struct playlist_data) new_files;
	DARRAY(struct playlist_data) old_files;
	obs_data_array_t *array;

	da_init(new_files);

	struct ffmpeg_source *s = data;

	const char *behavior;

	behavior = obs_data_get_string(settings, "behavior");

	if (astrcmpi(behavior, "behavior_pause_unpause") == 0) {
		s->behavior = BEHAVIOR_PAUSE_UNPAUSE;
	} else if (astrcmpi(behavior, "behavior_always_play") == 0) {
		s->behavior = BEHAVIOR_ALWAYS_PLAY;
	} else { /* S_BEHAVIOR_STOP_RESTART */
		s->behavior = BEHAVIOR_STOP_RESTART;
	}

	s->is_looping = obs_data_get_bool(settings, "looping");

#ifndef __APPLE__
	s->is_hw_decoding = obs_data_get_bool(settings, "hw_decode");
#endif

	s->is_clear_on_media_end =
		obs_data_get_bool(settings, "clear_on_media_end");
	s->range = (enum video_range_type)obs_data_get_int(settings,
							   "color_range");
	s->buffering_mb = (int)obs_data_get_int(settings, "buffering_mb");
	s->speed_percent = (int)obs_data_get_int(settings, "speed_percent");

	if (s->speed_percent < 1 || s->speed_percent > 200)
		s->speed_percent = 100;

	/* ------------------------------------- */
	/* create new playlist */

	array = obs_data_get_array(settings, "files");
	size_t count = obs_data_array_count(array);

	for (size_t i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(array, i);
		const char *path = obs_data_get_string(item, "value");
		os_dir_t *dir = os_opendir(path);

		if (dir) {
			struct dstr dir_path = {0};
			struct os_dirent *ent;

			for (;;) {
				const char *ext;

				ent = os_readdir(dir);
				if (!ent)
					break;
				if (ent->directory)
					continue;

				ext = os_get_path_extension(ent->d_name);
				if (!valid_extension(ext))
					continue;

				dstr_copy(&dir_path, path);
				dstr_cat_ch(&dir_path, '/');
				dstr_cat(&dir_path, ent->d_name);
				add_file(s, &new_files.da, dir_path.array,
					 false);
			}

			dstr_free(&dir_path);
			os_closedir(dir);
		} else {
			bool url = os_file_exists(path);
			add_file(s, &new_files.da, path, !url);
		}

		obs_data_release(item);
	}

	old_files.da = s->files.da;
	s->files.da = new_files.da;

	obs_data_array_release(array);
	free_files(&old_files.da);

	s->shuffle = obs_data_get_bool(settings, "shuffle");

	ffmpeg_source_restart(s);
}

static const char *ffmpeg_source_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFMpegSource");
}

void ffmpeg_source_restart(void *data)
{
	struct ffmpeg_source *s = data;

	shuffle_playlist(s);
	s->cur_item = 0;
	start_video(s);

	set_media_state(s, OBS_MEDIA_STATE_PLAYING);
}

static void restart_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey,
			   bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct ffmpeg_source *s = data;
	if (pressed && obs_source_active(s->source))
		obs_source_media_restart(s->source);
}

static void restart_proc(void *data, calldata_t *cd)
{
	restart_hotkey(data, 0, NULL, true);
	UNUSED_PARAMETER(cd);
}

void ffmpeg_source_play_pause(void *data, bool pause)
{
	struct ffmpeg_source *s = data;

	mp_media_play_pause(&s->media, pause);

	if (pause)
		set_media_state(s, OBS_MEDIA_STATE_PAUSED);
	else
		set_media_state(s, OBS_MEDIA_STATE_PLAYING);
}

void ffmpeg_source_stop(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->media_valid) {
		mp_media_free(&s->media);
		s->media_valid = false;
		obs_source_output_video(s->source, NULL);
		set_media_state(s, OBS_MEDIA_STATE_STOPPED);
	}
}

void ffmpeg_source_playlist_next(void *data)
{
	struct ffmpeg_source *s = data;

	if (++s->cur_item >= s->files.num)
		s->cur_item = 0;

	start_video(s);
}

static void ffmpeg_source_playlist_prev(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->cur_item == 0)
		s->cur_item = s->files.num - 1;
	else
		--s->cur_item;

	start_video(s);
}

static void get_duration(void *data, calldata_t *cd)
{
	struct ffmpeg_source *s = data;
	int64_t dur = 0;
	if (s->media.fmt)
		dur = s->media.fmt->duration;

	calldata_set_int(cd, "duration", dur * 1000);
}

static void get_nb_frames(void *data, calldata_t *cd)
{
	struct ffmpeg_source *s = data;
	int64_t frames = 0;

	if (!s->media.fmt) {
		calldata_set_int(cd, "num_frames", frames);
		return;
	}

	int video_stream_index = av_find_best_stream(
		s->media.fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

	if (video_stream_index < 0) {
		FF_BLOG(LOG_WARNING, "Getting number of frames failed: No "
				     "video stream in media file!");
		calldata_set_int(cd, "num_frames", frames);
		return;
	}

	AVStream *stream = s->media.fmt->streams[video_stream_index];

	if (stream->nb_frames > 0) {
		frames = stream->nb_frames;
	} else {
		FF_BLOG(LOG_DEBUG, "nb_frames not set, estimating using frame "
				   "rate and duration");
		AVRational avg_frame_rate = stream->avg_frame_rate;
		frames = (int64_t)ceil((double)s->media.fmt->duration /
				       (double)AV_TIME_BASE *
				       (double)avg_frame_rate.num /
				       (double)avg_frame_rate.den);
	}

	calldata_set_int(cd, "num_frames", frames);
}

void start_video(void *data)
{
	struct ffmpeg_source *s = data;

	if (!s->files.num)
		return;

	if (s->media_valid) {
		mp_media_free(&s->media);
		s->media_valid = false;
	}

	bfree(s->input);

	char *input = s->files.array[s->cur_item].path;
	s->input = input ? bstrdup(input) : NULL;

	ffmpeg_source_start(s);

	dump_source_info(s, input);

	obs_source_media_started(s->source);
}

static int64_t ffmpeg_source_get_duration(void *data)
{
	struct ffmpeg_source *s = data;
	int64_t dur = 0;

	if (s->media.fmt)
		dur = (float)s->media.fmt->duration / 1000.0f;

	return dur;
}

static int64_t ffmpeg_source_get_time(void *data)
{
	struct ffmpeg_source *s = data;

	return mp_get_current_time(&s->media);
}

static void ffmpeg_source_set_time(void *data, int64_t ms)
{
	struct ffmpeg_source *s = data;

	mp_media_seek_to(&s->media, ms);
}

static enum obs_media_state ffmpeg_source_get_state(void *data)
{
	struct ffmpeg_source *s = data;

	return s->state;
}

static void ffmpeg_source_play_hotkey(void *data, obs_hotkey_pair_id id,
				      obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct ffmpeg_source *s = data;

	if (s->state == OBS_MEDIA_STATE_PLAYING)
		return;

	if (pressed && obs_source_active(s->source))
		obs_source_media_play_pause(s->source, false);
}

static void ffmpeg_source_pause_hotkey(void *data, obs_hotkey_pair_id id,
				       obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct ffmpeg_source *s = data;

	if (s->state != OBS_MEDIA_STATE_PLAYING)
		return;

	if (pressed && obs_source_active(s->source))
		obs_source_media_play_pause(s->source, true);
}

static void ffmpeg_source_stop_hotkey(void *data, obs_hotkey_id id,
				      obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct ffmpeg_source *s = data;

	if (pressed && obs_source_active(s->source))
		obs_source_media_stop(s->source);
}

static void ffmpeg_source_playlist_next_hotkey(void *data, obs_hotkey_id id,
					       obs_hotkey_t *hotkey,
					       bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct ffmpeg_source *s = data;

	if (pressed && obs_source_active(s->source))
		obs_source_media_next(s->source);
}

static void ffmpeg_source_playlist_prev_hotkey(void *data, obs_hotkey_id id,
					       obs_hotkey_t *hotkey,
					       bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct ffmpeg_source *s = data;

	if (pressed && obs_source_active(s->source))
		obs_source_media_previous(s->source);
}

static void *ffmpeg_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct ffmpeg_source *s = bzalloc(sizeof(struct ffmpeg_source));
	s->source = source;

	s->cur_item = 0;

	s->play_pause_hotkey = obs_hotkey_pair_register_source(
		s->source, "MediaSource.Play", obs_module_text("Play"),
		"MediaSource.Pause", obs_module_text("Pause"),
		ffmpeg_source_play_hotkey, ffmpeg_source_pause_hotkey, s, s);

	s->hotkey = obs_hotkey_register_source(source, "MediaSource.Restart",
					       obs_module_text("Restart"),
					       restart_hotkey, s);

	s->stop_hotkey = obs_hotkey_register_source(source, "MediaSource.Stop",
						    obs_module_text("Stop"),
						    ffmpeg_source_stop_hotkey,
						    s);

	s->playlist_next_hotkey = obs_hotkey_register_source(
		source, "MediaSource.PlaylistNext",
		obs_module_text("PlaylistNext"),
		ffmpeg_source_playlist_next_hotkey, s);

	s->playlist_prev_hotkey = obs_hotkey_register_source(
		source, "MediaSource.PlaylistPrev",
		obs_module_text("PlaylistPrev"),
		ffmpeg_source_playlist_prev_hotkey, s);

	proc_handler_t *ph = obs_source_get_proc_handler(source);
	proc_handler_add(ph, "void restart()", restart_proc, s);
	proc_handler_add(ph, "void get_duration(out int duration)",
			 get_duration, s);
	proc_handler_add(ph, "void get_nb_frames(out int num_frames)",
			 get_nb_frames, s);

	/* ------------------------------------- */
	/* Make source backwards compatible with previous media sources */

	const char *old_input = obs_data_get_string(settings, "local_file");
	const char *old_url = obs_data_get_string(settings, "input");

	obs_data_array_t *array = obs_data_get_array(settings, "files");
	obs_data_t *item = obs_data_create();

	if (old_input && *old_input) {
		obs_data_set_string(item, "value", old_input);
		obs_data_unset_user_value(settings, "local_file");
		obs_data_array_insert(array, 0, item);
	} else if (old_url && *old_url) {
		obs_data_set_string(item, "value", old_url);
		obs_data_unset_user_value(settings, "input");
		obs_data_array_insert(array, 0, item);
	}

	obs_data_release(item);
	obs_data_array_release(array);

	set_media_state(s, OBS_MEDIA_STATE_NONE);
	ffmpeg_source_update(s, settings);
	return s;
}

static void ffmpeg_source_destroy(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->hotkey)
		obs_hotkey_unregister(s->hotkey);
	if (s->media_valid)
		mp_media_free(&s->media);

	if (s->sws_ctx != NULL)
		sws_freeContext(s->sws_ctx);
	free_files(&s->files.da);
	bfree(s->sws_data);
	bfree(s->input);
	bfree(s);
}

static void ffmpeg_source_activate(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->behavior == BEHAVIOR_STOP_RESTART) {
		obs_source_media_restart(s->source);
	} else if (s->behavior == BEHAVIOR_PAUSE_UNPAUSE) {
		obs_source_media_play_pause(s->source, false);
	}
}

static void ffmpeg_source_deactivate(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->behavior == BEHAVIOR_STOP_RESTART) {
		obs_source_media_stop(s->source);
	} else if (s->behavior == BEHAVIOR_PAUSE_UNPAUSE) {
		obs_source_media_play_pause(s->source, true);
	}
}

struct obs_source_info ffmpeg_source = {
	.id = "ffmpeg_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO |
			OBS_SOURCE_DO_NOT_DUPLICATE |
			OBS_SOURCE_CONTROLLABLE_MEDIA,
	.get_name = ffmpeg_source_getname,
	.create = ffmpeg_source_create,
	.destroy = ffmpeg_source_destroy,
	.get_defaults = ffmpeg_source_defaults,
	.get_properties = ffmpeg_source_getproperties,
	.activate = ffmpeg_source_activate,
	.deactivate = ffmpeg_source_deactivate,
	.video_tick = ffmpeg_source_tick,
	.update = ffmpeg_source_update,
	.media_play_pause = ffmpeg_source_play_pause,
	.media_restart = ffmpeg_source_restart,
	.media_stop = ffmpeg_source_stop,
	.media_next = ffmpeg_source_playlist_next,
	.media_previous = ffmpeg_source_playlist_prev,
	.media_get_duration = ffmpeg_source_get_duration,
	.media_get_time = ffmpeg_source_get_time,
	.media_set_time = ffmpeg_source_set_time,
	.media_get_state = ffmpeg_source_get_state};
