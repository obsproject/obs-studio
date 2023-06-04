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
#include <util/threading.h>
#include <util/platform.h>
#include <util/dstr.h>

#include "obs-ffmpeg-compat.h"
#include "obs-ffmpeg-formats.h"

#include <media-playback/media-playback.h>

#define FF_LOG_S(source, level, format, ...)        \
	blog(level, "[Media Source '%s']: " format, \
	     obs_source_get_name(source), ##__VA_ARGS__)
#define FF_BLOG(level, format, ...) \
	FF_LOG_S(s->source, level, format, ##__VA_ARGS__)

struct ffmpeg_source {
	media_playback_t *media;
	bool destroy_media;

	enum video_range_type range;
	bool is_linear_alpha;
	obs_source_t *source;
	obs_hotkey_id hotkey;

	char *input;
	char *input_format;
	char *ffmpeg_options;
	int buffering_mb;
	int speed_percent;
	bool is_looping;
	bool is_local_file;
	bool is_hw_decoding;
	bool full_decode;
	bool is_clear_on_media_end;
	bool restart_on_activate;
	bool close_when_inactive;
	bool seekable;
	bool is_stinger;
	bool log_changes;

	pthread_t reconnect_thread;
	pthread_mutex_t reconnect_mutex;
	bool reconnect_thread_valid;
	os_event_t *reconnect_stop_event;
	volatile bool reconnecting;
	int reconnect_delay_sec;

	enum obs_media_state state;
	obs_hotkey_pair_id play_pause_hotkey;
	obs_hotkey_id stop_hotkey;
};

// Used to safely cancel and join any active reconnect threads
// Use this to join any finished reconnect thread too!
static void stop_reconnect_thread(struct ffmpeg_source *s)
{
	if (s->is_local_file)
		return;
	pthread_mutex_lock(&s->reconnect_mutex);
	if (s->reconnect_thread_valid) {
		os_event_signal(s->reconnect_stop_event);
		pthread_join(s->reconnect_thread, NULL);
		s->reconnect_thread_valid = false;
		os_atomic_set_bool(&s->reconnecting, false);
		os_event_reset(s->reconnect_stop_event);
	}
	pthread_mutex_unlock(&s->reconnect_mutex);
}

static void set_media_state(void *data, enum obs_media_state state)
{
	struct ffmpeg_source *s = data;
	s->state = state;
}

static bool is_local_file_modified(obs_properties_t *props,
				   obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);

	bool enabled = obs_data_get_bool(settings, "is_local_file");
	obs_property_t *input = obs_properties_get(props, "input");
	obs_property_t *input_format =
		obs_properties_get(props, "input_format");
	obs_property_t *local_file = obs_properties_get(props, "local_file");
	obs_property_t *looping = obs_properties_get(props, "looping");
	obs_property_t *buffering = obs_properties_get(props, "buffering_mb");
	obs_property_t *seekable = obs_properties_get(props, "seekable");
	obs_property_t *speed = obs_properties_get(props, "speed_percent");
	obs_property_t *reconnect_delay_sec =
		obs_properties_get(props, "reconnect_delay_sec");
	obs_property_set_visible(input, !enabled);
	obs_property_set_visible(input_format, !enabled);
	obs_property_set_visible(buffering, !enabled);
	obs_property_set_visible(local_file, enabled);
	obs_property_set_visible(looping, enabled);
	obs_property_set_visible(speed, enabled);
	obs_property_set_visible(seekable, !enabled);
	obs_property_set_visible(reconnect_delay_sec, !enabled);

	return true;
}

static void ffmpeg_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "is_local_file", true);
	obs_data_set_default_bool(settings, "looping", false);
	obs_data_set_default_bool(settings, "clear_on_media_end", true);
	obs_data_set_default_bool(settings, "restart_on_activate", true);
	obs_data_set_default_bool(settings, "linear_alpha", false);
	obs_data_set_default_int(settings, "reconnect_delay_sec", 10);
	obs_data_set_default_int(settings, "buffering_mb", 2);
	obs_data_set_default_int(settings, "speed_percent", 100);
	obs_data_set_default_bool(settings, "log_changes", true);
}

static const char *media_filter =
	" (*.mp4 *.m4v *.ts *.mov *.mxf *.flv *.mkv *.avi *.mp3 *.ogg *.aac *.wav *.gif *.webm);;";
static const char *video_filter =
	" (*.mp4 *.m4v *.ts *.mov *.mxf *.flv *.mkv *.avi *.gif *.webm);;";
static const char *audio_filter = " (*.mp3 *.aac *.ogg *.wav);;";

static obs_properties_t *ffmpeg_source_getproperties(void *data)
{
	struct ffmpeg_source *s = data;
	struct dstr filter = {0};
	struct dstr path = {0};

	obs_properties_t *props = obs_properties_create();

	obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);

	obs_property_t *prop;
	// use this when obs allows non-readonly paths
	prop = obs_properties_add_bool(props, "is_local_file",
				       obs_module_text("LocalFile"));

	obs_property_set_modified_callback(prop, is_local_file_modified);

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

	obs_properties_add_path(props, "local_file",
				obs_module_text("LocalFile"), OBS_PATH_FILE,
				filter.array, path.array);
	dstr_free(&filter);
	dstr_free(&path);

	obs_properties_add_bool(props, "looping", obs_module_text("Looping"));

	obs_properties_add_bool(props, "restart_on_activate",
				obs_module_text("RestartWhenActivated"));

	prop = obs_properties_add_int_slider(props, "buffering_mb",
					     obs_module_text("BufferingMB"), 0,
					     16, 1);
	obs_property_int_set_suffix(prop, " MB");

	obs_properties_add_text(props, "input", obs_module_text("Input"),
				OBS_TEXT_DEFAULT);

	obs_properties_add_text(props, "input_format",
				obs_module_text("InputFormat"),
				OBS_TEXT_DEFAULT);

	prop = obs_properties_add_int_slider(
		props, "reconnect_delay_sec",
		obs_module_text("ReconnectDelayTime"), 1, 60, 1);
	obs_property_int_set_suffix(prop, " S");

	obs_properties_add_bool(props, "hw_decode",
				obs_module_text("HardwareDecode"));

	obs_properties_add_bool(props, "clear_on_media_end",
				obs_module_text("ClearOnMediaEnd"));

	prop = obs_properties_add_bool(
		props, "close_when_inactive",
		obs_module_text("CloseFileWhenInactive"));

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

	obs_properties_add_bool(props, "linear_alpha",
				obs_module_text("LinearAlpha"));

	obs_properties_add_bool(props, "seekable", obs_module_text("Seekable"));

	prop = obs_properties_add_text(props, "ffmpeg_options",
				       obs_module_text("FFmpegOpts"),
				       OBS_TEXT_DEFAULT);
	obs_property_set_long_description(
		prop, obs_module_text("FFmpegOpts.ToolTip.Source"));

	return props;
}

static void dump_source_info(struct ffmpeg_source *s, const char *input,
			     const char *input_format)
{
	if (!s->log_changes)
		return;
	FF_BLOG(LOG_INFO,
		"settings:\n"
		"\tinput:                   %s\n"
		"\tinput_format:            %s\n"
		"\tspeed:                   %d\n"
		"\tis_looping:              %s\n"
		"\tis_linear_alpha:         %s\n"
		"\tis_hw_decoding:          %s\n"
		"\tis_clear_on_media_end:   %s\n"
		"\trestart_on_activate:     %s\n"
		"\tclose_when_inactive:     %s\n"
		"\tfull_decode:             %s\n"
		"\tffmpeg_options:          %s",
		input ? input : "(null)",
		input_format ? input_format : "(null)", s->speed_percent,
		s->is_looping ? "yes" : "no", s->is_linear_alpha ? "yes" : "no",
		s->is_hw_decoding ? "yes" : "no",
		s->is_clear_on_media_end ? "yes" : "no",
		s->restart_on_activate ? "yes" : "no",
		s->close_when_inactive ? "yes" : "no",
		s->full_decode ? "yes" : "no", s->ffmpeg_options);
}

static void get_frame(void *opaque, struct obs_source_frame *f)
{
	struct ffmpeg_source *s = opaque;
	obs_source_output_video(s->source, f);
}

static void preload_frame(void *opaque, struct obs_source_frame *f)
{
	struct ffmpeg_source *s = opaque;
	if (s->close_when_inactive)
		return;

	if (s->is_clear_on_media_end || s->is_looping)
		obs_source_preload_video(s->source, f);

	if (!s->is_local_file && os_atomic_set_bool(&s->reconnecting, false))
		FF_BLOG(LOG_INFO, "Reconnected.");
}

static void seek_frame(void *opaque, struct obs_source_frame *f)
{
	struct ffmpeg_source *s = opaque;
	obs_source_set_video_frame(s->source, f);
}

static void get_audio(void *opaque, struct obs_source_audio *a)
{
	struct ffmpeg_source *s = opaque;
	obs_source_output_audio(s->source, a);

	if (!s->is_local_file && os_atomic_set_bool(&s->reconnecting, false))
		FF_BLOG(LOG_INFO, "Reconnected.");
}

static void media_stopped(void *opaque)
{
	struct ffmpeg_source *s = opaque;
	if (s->is_clear_on_media_end && !s->is_stinger) {
		obs_source_output_video(s->source, NULL);
	}

	if ((s->close_when_inactive || !s->is_local_file) && s->media)
		s->destroy_media = true;

	set_media_state(s, OBS_MEDIA_STATE_ENDED);
	obs_source_media_ended(s->source);
}

static void ffmpeg_source_open(struct ffmpeg_source *s)
{
	if (s->input && *s->input) {
		struct mp_media_info info = {
			.opaque = s,
			.v_cb = get_frame,
			.v_preload_cb = preload_frame,
			.v_seek_cb = seek_frame,
			.a_cb = get_audio,
			.stop_cb = media_stopped,
			.path = s->input,
			.format = s->input_format,
			.buffering = s->buffering_mb * 1024 * 1024,
			.speed = s->speed_percent,
			.force_range = s->range,
			.is_linear_alpha = s->is_linear_alpha,
			.hardware_decoding = s->is_hw_decoding,
			.ffmpeg_options = s->ffmpeg_options,
			.is_local_file = s->is_local_file || s->seekable,
			.reconnecting = s->reconnecting,
			.request_preload = s->is_stinger,
			.full_decode = s->full_decode,
		};

		s->media = media_playback_create(&info);
	}
}

static void ffmpeg_source_start(struct ffmpeg_source *s)
{
	if (!s->media)
		ffmpeg_source_open(s);

	if (!s->media)
		return;

	media_playback_play(s->media, s->is_looping, s->reconnecting);
	if (s->is_local_file && media_playback_has_video(s->media) &&
	    (s->is_clear_on_media_end || s->is_looping))
		obs_source_show_preloaded_video(s->source);
	else
		obs_source_output_video(s->source, NULL);
	set_media_state(s, OBS_MEDIA_STATE_PLAYING);
	obs_source_media_started(s->source);
}

static void *ffmpeg_source_reconnect(void *data)
{
	struct ffmpeg_source *s = data;

	int ret = os_event_timedwait(s->reconnect_stop_event,
				     s->reconnect_delay_sec * 1000);
	if (ret == 0 || s->media)
		return NULL;

	bool active = obs_source_active(s->source);
	if (!s->close_when_inactive || active)
		ffmpeg_source_open(s);

	if (!s->restart_on_activate || active)
		ffmpeg_source_start(s);

	return NULL;
}

static void ffmpeg_source_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);

	struct ffmpeg_source *s = data;
	if (s->destroy_media) {
		if (s->media) {
			media_playback_destroy(s->media);
			s->media = NULL;
		}

		s->destroy_media = false;

		if (!s->is_local_file) {
			pthread_mutex_lock(&s->reconnect_mutex);
			if (!os_atomic_set_bool(&s->reconnecting, true))
				FF_BLOG(LOG_WARNING, "Disconnected. "
						     "Reconnecting...");
			if (s->reconnect_thread_valid) {
				os_event_signal(s->reconnect_stop_event);
				pthread_join(s->reconnect_thread, NULL);
				s->reconnect_thread_valid = false;
				os_event_reset(s->reconnect_stop_event);
			}
			if (pthread_create(&s->reconnect_thread, NULL,
					   ffmpeg_source_reconnect, s) != 0) {
				FF_BLOG(LOG_WARNING, "Could not create "
						     "reconnect thread");
				pthread_mutex_unlock(&s->reconnect_mutex);
				return;
			}
			s->reconnect_thread_valid = true;
			pthread_mutex_unlock(&s->reconnect_mutex);
		}
	}
}

#define SRT_PROTO "srt"
#define RIST_PROTO "rist"

static bool requires_mpegts(const char *path)
{
	return !astrcmpi_n(path, SRT_PROTO, sizeof(SRT_PROTO) - 1) ||
	       !astrcmpi_n(path, RIST_PROTO, sizeof(RIST_PROTO) - 1);
}

static void ffmpeg_source_update(void *data, obs_data_t *settings)
{
	struct ffmpeg_source *s = data;

	bool is_local_file = obs_data_get_bool(settings, "is_local_file");
	bool is_stinger = obs_data_get_bool(settings, "is_stinger");

	const char *input;
	const char *input_format;
	const char *ffmpeg_options;

	bfree(s->input);
	bfree(s->input_format);
	bfree(s->ffmpeg_options);

	if (is_local_file) {
		input = obs_data_get_string(settings, "local_file");
		input_format = NULL;
		s->is_looping = obs_data_get_bool(settings, "looping");
	} else {
		input = obs_data_get_string(settings, "input");
		input_format = obs_data_get_string(settings, "input_format");
		if (requires_mpegts(input)) {
			input_format = "mpegts";
			obs_data_set_string(settings, "input_format", "mpegts");
		}
		s->reconnect_delay_sec =
			(int)obs_data_get_int(settings, "reconnect_delay_sec");
		s->reconnect_delay_sec = s->reconnect_delay_sec == 0
						 ? 10
						 : s->reconnect_delay_sec;
		s->is_looping = false;
	}

	stop_reconnect_thread(s);

	ffmpeg_options = obs_data_get_string(settings, "ffmpeg_options");

	s->close_when_inactive =
		obs_data_get_bool(settings, "close_when_inactive");

	s->input = input ? bstrdup(input) : NULL;
	s->input_format = input_format ? bstrdup(input_format) : NULL;
	s->is_hw_decoding = obs_data_get_bool(settings, "hw_decode");
	s->full_decode = obs_data_get_bool(settings, "full_decode");
	s->is_clear_on_media_end =
		obs_data_get_bool(settings, "clear_on_media_end");
	s->restart_on_activate =
		!astrcmpi_n(input, RIST_PROTO, sizeof(RIST_PROTO) - 1)
			? false
			: obs_data_get_bool(settings, "restart_on_activate");
	s->range = (enum video_range_type)obs_data_get_int(settings,
							   "color_range");
	s->is_linear_alpha = obs_data_get_bool(settings, "linear_alpha");
	s->buffering_mb = (int)obs_data_get_int(settings, "buffering_mb");
	s->speed_percent = (int)obs_data_get_int(settings, "speed_percent");
	s->is_local_file = is_local_file;
	s->seekable = obs_data_get_bool(settings, "seekable");
	s->ffmpeg_options = ffmpeg_options ? bstrdup(ffmpeg_options) : NULL;
	s->is_stinger = is_stinger;
	s->log_changes = obs_data_get_bool(settings, "log_changes");

	if (s->speed_percent < 1 || s->speed_percent > 200)
		s->speed_percent = 100;

	if (s->media) {
		media_playback_destroy(s->media);
		s->media = NULL;
	}

	bool active = obs_source_active(s->source);
	if (!s->close_when_inactive || active)
		ffmpeg_source_open(s);

	dump_source_info(s, input, input_format);
	if (!s->restart_on_activate || active)
		ffmpeg_source_start(s);
}

static const char *ffmpeg_source_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegSource");
}

static void restart_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey,
			   bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (!pressed)
		return;

	struct ffmpeg_source *s = data;
	if (obs_source_showing(s->source))
		obs_source_media_restart(s->source);
}

static void restart_proc(void *data, calldata_t *cd)
{
	restart_hotkey(data, 0, NULL, true);
	UNUSED_PARAMETER(cd);
}

static void preload_first_frame_proc(void *data, calldata_t *cd)
{
	struct ffmpeg_source *s = data;
	if (s->is_stinger)
		obs_source_output_video(s->source, NULL);
	media_playback_preload_frame(s->media);
	UNUSED_PARAMETER(cd);
}

static void get_duration(void *data, calldata_t *cd)
{
	struct ffmpeg_source *s = data;
	int64_t dur = 0;
	if (s->media)
		dur = media_playback_get_duration(s->media);

	calldata_set_int(cd, "duration", dur * 1000);
}

static void get_nb_frames(void *data, calldata_t *cd)
{
	struct ffmpeg_source *s = data;
	int64_t frames = media_playback_get_frames(s->media);
	calldata_set_int(cd, "num_frames", frames);
}

static bool ffmpeg_source_play_hotkey(void *data, obs_hotkey_pair_id id,
				      obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (!pressed)
		return false;

	struct ffmpeg_source *s = data;

	if (s->state == OBS_MEDIA_STATE_PLAYING ||
	    !obs_source_showing(s->source))
		return false;

	obs_source_media_play_pause(s->source, false);
	return true;
}

static bool ffmpeg_source_pause_hotkey(void *data, obs_hotkey_pair_id id,
				       obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (!pressed)
		return false;

	struct ffmpeg_source *s = data;

	if (s->state != OBS_MEDIA_STATE_PLAYING ||
	    !obs_source_showing(s->source))
		return false;

	obs_source_media_play_pause(s->source, true);
	return true;
}

static void ffmpeg_source_stop_hotkey(void *data, obs_hotkey_id id,
				      obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (!pressed)
		return;

	struct ffmpeg_source *s = data;

	if (obs_source_showing(s->source))
		obs_source_media_stop(s->source);
}

static void *ffmpeg_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct ffmpeg_source *s = bzalloc(sizeof(struct ffmpeg_source));
	s->source = source;

	// Manual type since the event can be signalled without an active thread
	if (os_event_init(&s->reconnect_stop_event, OS_EVENT_TYPE_MANUAL)) {
		FF_BLOG(LOG_ERROR, "Failed to initialize reconnect stop event");
		bfree(s);
		return NULL;
	}

	if (pthread_mutex_init(&s->reconnect_mutex, NULL)) {
		FF_BLOG(LOG_ERROR, "Failed to initialize reconnect mutex");
		os_event_destroy(s->reconnect_stop_event);
		bfree(s);
		return NULL;
	}

	s->hotkey = obs_hotkey_register_source(source, "MediaSource.Restart",
					       obs_module_text("RestartMedia"),
					       restart_hotkey, s);

	s->play_pause_hotkey = obs_hotkey_pair_register_source(
		s->source, "MediaSource.Play", obs_module_text("Play"),
		"MediaSource.Pause", obs_module_text("Pause"),
		ffmpeg_source_play_hotkey, ffmpeg_source_pause_hotkey, s, s);

	s->stop_hotkey = obs_hotkey_register_source(source, "MediaSource.Stop",
						    obs_module_text("Stop"),
						    ffmpeg_source_stop_hotkey,
						    s);

	proc_handler_t *ph = obs_source_get_proc_handler(source);
	proc_handler_add(ph, "void restart()", restart_proc, s);
	proc_handler_add(ph, "void preload_first_frame()",
			 preload_first_frame_proc, s);
	proc_handler_add(ph, "void get_duration(out int duration)",
			 get_duration, s);
	proc_handler_add(ph, "void get_nb_frames(out int num_frames)",
			 get_nb_frames, s);

	ffmpeg_source_update(s, settings);
	return s;
}

static void ffmpeg_source_destroy(void *data)
{
	struct ffmpeg_source *s = data;

	stop_reconnect_thread(s);

	if (s->hotkey)
		obs_hotkey_unregister(s->hotkey);
	if (s->media)
		media_playback_destroy(s->media);

	pthread_mutex_destroy(&s->reconnect_mutex);
	os_event_destroy(s->reconnect_stop_event);
	bfree(s->input);
	bfree(s->input_format);
	bfree(s->ffmpeg_options);
	bfree(s);
}

static void ffmpeg_source_activate(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->restart_on_activate)
		obs_source_media_restart(s->source);
}

static void ffmpeg_source_deactivate(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->restart_on_activate) {
		if (s->media) {
			media_playback_stop(s->media);

			if (s->is_clear_on_media_end)
				obs_source_output_video(s->source, NULL);
		}
	}
}

static void ffmpeg_source_play_pause(void *data, bool pause)
{
	struct ffmpeg_source *s = data;

	if (!s->media)
		ffmpeg_source_open(s);

	if (!s->media)
		return;

	media_playback_play_pause(s->media, pause);

	if (pause) {

		set_media_state(s, OBS_MEDIA_STATE_PAUSED);
	} else {

		set_media_state(s, OBS_MEDIA_STATE_PLAYING);
		obs_source_media_started(s->source);
	}
}

static void ffmpeg_source_stop(void *data)
{
	struct ffmpeg_source *s = data;

	if (s->media) {
		media_playback_stop(s->media);
		obs_source_output_video(s->source, NULL);
		set_media_state(s, OBS_MEDIA_STATE_STOPPED);
	}
}

static void ffmpeg_source_restart(void *data)
{
	struct ffmpeg_source *s = data;

	if (obs_source_showing(s->source))
		ffmpeg_source_start(s);

	set_media_state(s, OBS_MEDIA_STATE_PLAYING);
}

static int64_t ffmpeg_source_get_duration(void *data)
{
	struct ffmpeg_source *s = data;
	int64_t dur = 0;

	if (s->media)
		dur = media_playback_get_duration(s->media) / INT64_C(1000);

	return dur;
}

static int64_t ffmpeg_source_get_time(void *data)
{
	struct ffmpeg_source *s = data;

	return media_playback_get_current_time(s->media);
}

static void ffmpeg_source_set_time(void *data, int64_t ms)
{
	struct ffmpeg_source *s = data;

	if (!s->media)
		return;

	media_playback_seek(s->media, ms);
}

static enum obs_media_state ffmpeg_source_get_state(void *data)
{
	struct ffmpeg_source *s = data;

	return s->state;
}

static void missing_file_callback(void *src, const char *new_path, void *data)
{
	struct ffmpeg_source *s = src;

	obs_source_t *source = s->source;
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_set_string(settings, "local_file", new_path);
	obs_source_update(source, settings);
	obs_data_release(settings);

	UNUSED_PARAMETER(data);
}

static obs_missing_files_t *ffmpeg_source_missingfiles(void *data)
{
	struct ffmpeg_source *s = data;
	obs_missing_files_t *files = obs_missing_files_create();

	if (s->is_local_file && strcmp(s->input, "") != 0) {
		if (!os_file_exists(s->input)) {
			obs_missing_file_t *file = obs_missing_file_create(
				s->input, missing_file_callback,
				OBS_MISSING_FILE_SOURCE, s->source, NULL);

			obs_missing_files_add_file(files, file);
		}
	}

	return files;
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
	.missing_files = ffmpeg_source_missingfiles,
	.update = ffmpeg_source_update,
	.icon_type = OBS_ICON_TYPE_MEDIA,
	.media_play_pause = ffmpeg_source_play_pause,
	.media_restart = ffmpeg_source_restart,
	.media_stop = ffmpeg_source_stop,
	.media_get_duration = ffmpeg_source_get_duration,
	.media_get_time = ffmpeg_source_get_time,
	.media_set_time = ffmpeg_source_set_time,
	.media_get_state = ffmpeg_source_get_state,
};
