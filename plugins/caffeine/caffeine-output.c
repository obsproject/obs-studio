
#include <obs-module.h>

#include <util/base.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/threading.h>

#include <caffeine.h>

#include "caffeine-foreground-process.h"
#include "caffeine-settings.h"

/* Uncomment this to log each call to raw_audio/video
#define TRACE_FRAMES
/**/

#define do_log(level, format, ...) \
	blog(level, "[caffeine output] " format, ##__VA_ARGS__)

#define log_error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define log_warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define log_info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define log_debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

#define trace() log_debug("%s", __func__)

#define set_error(output, fmt, ...)                                 \
	do {                                                        \
		struct dstr message;                                \
		dstr_init(&message);                                \
		dstr_printf(&message, (fmt), ##__VA_ARGS__);        \
		log_error("%s", message.array);                     \
		obs_output_set_last_error((output), message.array); \
		dstr_free(&message);                                \
	} while (false)

#define CAFF_AUDIO_FORMAT AUDIO_FORMAT_16BIT
#define CAFF_AUDIO_FORMAT_TYPE int16_t
#define CAFF_AUDIO_LAYOUT SPEAKERS_STEREO
#define CAFF_AUDIO_LAYOUT_MUL 2
#define CAFF_AUDIO_SAMPLERATE 48000ul
#define NANOSECONDS 1000000000ull

static int const enforced_height = 720;

struct caffeine_audio {
	struct audio_data *frames;
	struct caffeine_audio *next;
};

struct caffeine_output {
	obs_output_t *output;
	caff_InstanceHandle instance;
	struct obs_video_info video_info;
	uint64_t start_timestamp;
	size_t audio_planes;
	size_t audio_size;

	volatile bool is_online;
	pthread_t monitor_thread;
	char *foreground_process;
	char *game_id;

	pthread_t audio_thread;
	struct caffeine_audio *audio_queue;
	pthread_mutex_t audio_lock;
	pthread_cond_t audio_cond;
	bool audio_stop;
};

static void audio_data_copy(struct audio_data *left, struct audio_data *right)
{
	for (int idx = 0; idx < MAX_AV_PLANES; idx++) {
		if (!left->data[idx]) {
			right->data[idx] = NULL;
		} else {
			size_t size = left->frames *
				      sizeof(CAFF_AUDIO_FORMAT_TYPE) *
				      CAFF_AUDIO_LAYOUT_MUL;
			right->data[idx] = bmalloc(size);
			memcpy(right->data[idx], left->data[idx], size);
		}
	}
	right->frames = left->frames;
	right->timestamp = left->timestamp;
}

static void audio_data_free(struct audio_data *ptr)
{
	for (int idx = 0; idx < MAX_AV_PLANES; idx++) {
		if (!ptr->data[idx])
			continue;
		bfree(ptr->data[idx]);
	}
	bfree(ptr);
}

static const char *caffeine_get_name(void *data)
{
	UNUSED_PARAMETER(data);

	return obs_module_text("CaffeineOutput");
}

static int caffeine_to_obs_error(caff_Result error)
{
	switch (error) {
	case caff_ResultSuccess:
		return OBS_OUTPUT_SUCCESS;
	case caff_ResultOutOfCapacity:
	case caff_ResultFailure:
	case caff_ResultBroadcastFailed:
		return OBS_OUTPUT_CONNECT_FAILED;
	case caff_ResultDisconnected:
		return OBS_OUTPUT_DISCONNECTED;
	case caff_ResultTakeover:
	default:
		return OBS_OUTPUT_ERROR;
	}
}

caff_VideoFormat obs_to_caffeine_format(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_I420:
		return caff_VideoFormatI420;
	case VIDEO_FORMAT_NV12:
		return caff_VideoFormatNv12;
	case VIDEO_FORMAT_YUY2:
		return caff_VideoFormatYuy2;
	case VIDEO_FORMAT_UYVY:
		return caff_VideoFormatUyvy;
	case VIDEO_FORMAT_BGRA:
		return caff_VideoFormatBgra;

	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_I444:
	case VIDEO_FORMAT_Y800:
	case VIDEO_FORMAT_BGRX:
	case VIDEO_FORMAT_YVYU:
	default:
		return caff_VideoFormatUnknown;
	}
}

static bool prepare_audio(struct caffeine_output *context,
			  const struct audio_data *frame,
			  struct audio_data *output)
{
	/* This fixes an issue where unencoded outputs have video & audio out of sync
	 *
	 * Copied/adapted from obs-outputs/flv-output
	 */

	*output = *frame;

	if (frame->timestamp < context->start_timestamp) {
		uint64_t duration = ((uint64_t)frame->frames) * NANOSECONDS /
				    CAFF_AUDIO_SAMPLERATE;
		uint64_t end_ts = (frame->timestamp + duration);
		uint64_t cutoff;

		if (end_ts <= context->start_timestamp)
			return false;

		cutoff = context->start_timestamp - frame->timestamp;
		output->timestamp += cutoff;

		cutoff = cutoff * CAFF_AUDIO_SAMPLERATE / NANOSECONDS;

		for (size_t i = 0; i < context->audio_planes; i++)
			output->data[i] +=
				context->audio_size * (uint32_t)cutoff;
		output->frames -= (uint32_t)cutoff;
	}

	return true;
}

static void *PTW32_CDECL caffeine_handle_audio(void *ptr)
{
	struct caffeine_output *context = (struct caffeine_output *)ptr;

	pthread_mutex_lock(&context->audio_lock);
	while (!context->audio_stop) {
		// Wait for a signal.
		pthread_cond_wait(&context->audio_cond, &context->audio_lock);
		if (context->audio_stop || !context->audio_queue)
			continue;

		// Dequeue the front element.
		while (context->audio_queue) {
			// Grab the first element and unlock the mutex.
			struct caffeine_audio *here = context->audio_queue;
			context->audio_queue = here->next;
			pthread_mutex_unlock(&context->audio_lock);

			// Send off the audio.
			caff_sendAudio(context->instance, here->frames->data[0],
				       here->frames->frames);

			// Delete the dequeued element.
			audio_data_free(here->frames);
			bfree(here);

			// Lock the mutex again.
			pthread_mutex_lock(&context->audio_lock);
		}
	}
	pthread_mutex_unlock(&context->audio_lock);

	return NULL;
}

static void *caffeine_create(obs_data_t *settings, obs_output_t *output)
{
	trace();
	UNUSED_PARAMETER(settings);

	struct caffeine_output *context =
		bzalloc(sizeof(struct caffeine_output));
	context->output = output;

	/* TODO: can we get this from the CaffeineAuth object somehow? */
	context->instance = caff_createInstance();

	return context;
}

static void caffeine_stream_started(void *data);
static void caffeine_stream_failed(void *data, caff_Result error);

static bool caffeine_authenticate(struct caffeine_output *context)
{
	trace();

	obs_output_t *output = context->output;

	obs_service_t *service = obs_output_get_service(output);
	char const *refresh_token = obs_service_get_key(service);

	switch (caff_refreshAuth(context->instance, refresh_token)) {
	case caff_ResultSuccess:
		return true;
	case caff_ResultInfoIncorrect:
		set_error(output, "%s", obs_module_text("SigninFailed"));
		return false;
	case caff_ResultRefreshTokenRequired:
		set_error(output, "%s", obs_module_text("ErrorMustSignIn"));
		return false;
	case caff_ResultFailure:
	case caff_ResultAlreadyBroadcasting:
		log_warn("%s", obs_module_text("WarningAlreadyStreaming"));
		return false;
	default:
		set_error(output, "%s", obs_module_text("SigninFailed"));
		return false;
	}
}

static bool caffeine_start(void *data)
{
	trace();
	struct caffeine_output *context = data;
	obs_output_t *output = context->output;

	switch (caff_checkVersion()) {
	case caff_ResultSuccess:
		break;
	case caff_ResultOldVersion:
		set_error(output, "%s", obs_module_text("ErrorOldVersion"));
		return false;
	default:
		log_warn("Failed to complete Caffeine version check");
	}

	if (!caffeine_authenticate(context))
		return false;

	if (!obs_get_video_info(&context->video_info)) {
		set_error(output, "Failed to get video info");
		return false;
	}

	if (context->video_info.output_height > enforced_height)
		log_warn("For best video quality and reduced CPU usage,"
			 " set output resolution to 720p or below");

	caff_VideoFormat format =
		obs_to_caffeine_format(context->video_info.output_format);

	if (format == caff_VideoFormatUnknown) {
		set_error(output, "%s %s", obs_module_text("ErrorVideoFormat"),
			  get_video_format_name(
				  context->video_info.output_format));
		return false;
	}

	struct audio_convert_info conversion = {.format = CAFF_AUDIO_FORMAT,
						.speakers = CAFF_AUDIO_LAYOUT,
						.samples_per_sec =
							CAFF_AUDIO_SAMPLERATE};
	obs_output_set_audio_conversion(output, &conversion);

	context->audio_planes =
		get_audio_planes(conversion.format, conversion.speakers);
	context->audio_size =
		get_audio_size(conversion.format, conversion.speakers, 1);

	if (!obs_output_can_begin_data_capture(output, 0))
		return false;

	{ // Initialize Audio
		context->audio_stop = false;
		context->audio_queue = NULL;
		pthread_mutex_init(&context->audio_lock, NULL);
		pthread_cond_init(&context->audio_cond, NULL);
		pthread_create(&context->audio_thread, NULL,
			       &caffeine_handle_audio, context);
	}

	obs_service_t *service = obs_output_get_service(output);
	obs_data_t *settings = obs_service_get_settings(service);
	char const *title = obs_data_get_string(settings, BROADCAST_TITLE_KEY);

	caff_Rating rating =
		(caff_Rating)obs_data_get_int(settings, BROADCAST_RATING_KEY);
	obs_data_release(settings);

	caff_Result result = caff_startBroadcast(context->instance, context,
						 title, rating, NULL,
						 caffeine_stream_started,
						 caffeine_stream_failed);

	if (result) {
		set_error(output, "%s", caff_resultString(result));
		return false;
	}

	return true;
}

static void enumerate_games(void *data, char const *process_name,
			    char const *game_id, char const *game_name)
{
	struct caffeine_output *context = data;
	if (strcmp(process_name, context->foreground_process) == 0) {
		log_debug("Detected game [%s]: %s", game_id, game_name);
		bfree(context->game_id);
		context->game_id = bstrdup(game_id);
	}
}

static void *monitor_thread(void *data)
{
	trace();
	struct caffeine_output *context = data;
	const uint64_t millisPerNano = 1000000;
	const uint64_t stop_interval = 100 /*ms*/ * millisPerNano;
	const uint64_t game_interval = 5000 /*ms*/ * millisPerNano;
	const uint64_t service_interval = 1000 /*ms*/ * millisPerNano;

	uint64_t last_ts = os_gettime_ns();
	uint64_t cur_game_interval = 0;
	uint64_t cur_service_interval = 0;
	while (context->is_online) {
		os_sleepto_ns(last_ts + stop_interval);

		// Update Timers
		uint64_t now_ts = os_gettime_ns();
		int64_t delta_ts = now_ts - last_ts;
		last_ts = now_ts;
		cur_game_interval += delta_ts;
		cur_service_interval += delta_ts;

		// Game Update
		if (cur_game_interval > game_interval) {
			cur_game_interval = 0;
			context->foreground_process =
				get_foreground_process_name();
			if (context->foreground_process) {
				caff_enumerateGames(context->instance, context,
						    enumerate_games);
				bfree(context->foreground_process);
				context->foreground_process = NULL;
			}
			caff_setGameId(context->instance, context->game_id);
			bfree(context->game_id);
			context->game_id = NULL;
		}

		// Service Update
		if (cur_service_interval > service_interval) {
			cur_service_interval = 0;
			obs_service_t *service =
				obs_output_get_service(context->output);
			if (service) {
				obs_data_t *data =
					obs_service_get_settings(service);
				caff_setTitle(context->instance,
					      obs_data_get_string(
						      data,
						      BROADCAST_TITLE_KEY));
				caff_setRating(
					context->instance,
					obs_data_get_int(data,
							 BROADCAST_RATING_KEY));
				obs_data_release(data);
			}
		}
	}

	return NULL;
}

static void caffeine_stream_started(void *data)
{
	trace();
	struct caffeine_output *context = data;
	context->is_online = true;
	pthread_create(&context->monitor_thread, NULL, monitor_thread, context);
	obs_output_begin_data_capture(context->output, 0);
}

static void caffeine_stream_failed(void *data, caff_Result error)
{
	struct caffeine_output *context = data;

	if (!obs_output_get_last_error(context->output)) {
		set_error(context->output, "%s: [%d] %s",
			  obs_module_text("ErrorStartStream"), error,
			  caff_resultString(error));
	}

	if (context->is_online) {
		context->is_online = false;
		pthread_join(context->monitor_thread, NULL);
	}

	obs_output_signal_stop(context->output, caffeine_to_obs_error(error));
}

static void caffeine_raw_video(void *data, struct video_data *frame)
{
#ifdef TRACE_FRAMES
	trace();
#endif
	struct caffeine_output *context = data;

	uint32_t width = context->video_info.output_width;
	uint32_t height = context->video_info.output_height;
	size_t total_bytes = frame->linesize[0] * height;
	caff_VideoFormat format =
		obs_to_caffeine_format(context->video_info.output_format);
	int64_t timestampMicros = frame->timestamp / 1000;

	if (!context->start_timestamp)
		context->start_timestamp = frame->timestamp;

	caff_sendVideo(context->instance, format, frame->data[0], total_bytes,
		       width, height, timestampMicros);
}

static void caffeine_raw_audio(void *data, struct audio_data *frames)
{
#ifdef TRACE_FRAMES
	trace();
#endif
	struct caffeine_output *context = data;

	// Ensure that everything is initialized and still available.
	if (context->audio_stop || !context->audio_lock || !context->audio_cond)
		return;

	// Ensure that we are actually live and have started streaming.
	if (!context->start_timestamp)
		return;

	// Cut off or abort audio data if it does not start at our intended time.
	struct audio_data in;
	if (!prepare_audio(context, frames, &in)) {
		return;
	}

	// Create a copy of the audio data for queuing.
	// ToDo: Can this be optimized to use circlebuf for data?
	struct caffeine_audio *ca =
		(struct caffeine_audio *)bmalloc(sizeof(struct caffeine_audio));
	ca->next = NULL;
	ca->frames = (struct audio_data *)bmalloc(sizeof(struct audio_data));
	audio_data_copy(&in, ca->frames);

	// Enqueue Audio
	pthread_mutex_lock(&context->audio_lock);
	{
		// Find the last element that we can write to. This looks a bit weird,
		// but it is a pointer to the last 'next' entry that is valid.
		struct caffeine_audio **tgt = &context->audio_queue;
		while (((*tgt) != NULL) && ((*tgt)->next != NULL))
			tgt = &((*tgt)->next);
		*tgt = ca;
	}
	pthread_cond_signal(&context->audio_cond);
	pthread_mutex_unlock(&context->audio_lock);
}

static void caffeine_stop(void *data, uint64_t ts)
{
	trace();
	/* TODO: do something with this? */
	UNUSED_PARAMETER(ts);

	struct caffeine_output *context = data;
	obs_output_t *output = context->output;

	if (context->is_online) {
		context->is_online = false;
		pthread_join(context->monitor_thread, NULL);
	}

	{         // Clean up Audio
		{ // Signal thread to stop and join with it.
			pthread_mutex_lock(&context->audio_lock);
			context->audio_stop = true;
			pthread_cond_signal(&context->audio_cond);
			pthread_mutex_unlock(&context->audio_lock);
			pthread_join(context->audio_thread, NULL);
			pthread_mutex_destroy(&context->audio_lock);
			pthread_cond_destroy(&context->audio_cond);
		}
		while (context->audio_queue) {
			// clean up any remaining data.
			struct caffeine_audio *here = context->audio_queue;
			context->audio_queue = here->next;
			audio_data_free(here->frames);
			bfree(here);
		}
	}

	caff_endBroadcast(context->instance);

	obs_output_end_data_capture(output);
}

static void caffeine_destroy(void *data)
{
	trace();
	struct caffeine_output *context = data;
	caff_freeInstance(&context->instance);

	bfree(data);
}

static float caffeine_get_congestion(void *data)
{
	struct caffeine_output *context = data;

	caff_ConnectionQuality quality =
		caff_getConnectionQuality(context->instance);

	switch (quality) {
	case caff_ConnectionQualityGood:
		return 0.f;
	case caff_ConnectionQualityPoor:
		return 1.f;
	default:
		return 0.5f;
	}
}

struct obs_output_info caffeine_output_info = {
	.id = "caffeine_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_SERVICE |
		 OBS_OUTPUT_BANDWIDTH_TEST_DISABLED |
		 OBS_OUTPUT_HARDWARE_ENCODING_DISABLED,

	.get_name = caffeine_get_name,

	.create = caffeine_create,
	.destroy = caffeine_destroy,

	.start = caffeine_start,
	.stop = caffeine_stop,

	.raw_video = caffeine_raw_video,
	.raw_audio = caffeine_raw_audio,
};
