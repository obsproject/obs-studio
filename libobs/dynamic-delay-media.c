// dynamic-delay-media.c
#include "dynamic-delay-media.h"
#include "obs-internal.h"
#include "util/deque.h"
#include "util/threading.h"
#include "media-io/audio-io.h"
#include "graphics/vec2.h"

struct dyn_delay_media {
	obs_source_t *source;
	obs_scene_t *scene;
	obs_view_t *view;
	video_t *video;
	audio_t *audio;
	obs_encoder_t *venc;
	obs_encoder_t *aenc;

	dyn_delay_media_packet_cb cb;
	void *param;
	bool started;

	/* bridges the source's decoded audio (already resampled by libobs to
	 * the main audio format) into the standalone audio_t that feeds the
	 * media audio encoder */
	pthread_mutex_t audio_mutex;
	struct deque audio_buf[MAX_AUDIO_CHANNELS];
	size_t audio_channels;
	size_t audio_max_bytes;
};

static void media_encoded_packet(void *param, struct encoder_packet *packet, struct encoder_packet_time *frame_time)
{
	struct dyn_delay_media *media = param;

	UNUSED_PARAMETER(frame_time);

	media->cb(media->param, packet);
}

static void media_audio_capture(void *param, obs_source_t *source, const struct audio_data *audio, bool muted)
{
	struct dyn_delay_media *media = param;

	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(muted);

	pthread_mutex_lock(&media->audio_mutex);
	for (size_t ch = 0; ch < media->audio_channels; ch++) {
		if (!audio->data[ch])
			continue;
		deque_push_back(&media->audio_buf[ch], audio->data[ch], audio->frames * sizeof(float));
		if (media->audio_buf[ch].size > media->audio_max_bytes)
			deque_pop_front(&media->audio_buf[ch], NULL,
					media->audio_buf[ch].size - media->audio_max_bytes);
	}
	pthread_mutex_unlock(&media->audio_mutex);
}

static bool media_audio_input(void *param, uint64_t start_ts, uint64_t end_ts, uint64_t *new_ts, uint32_t active_mixers,
			      struct audio_output_data *mixes)
{
	struct dyn_delay_media *media = param;
	size_t bytes = AUDIO_OUTPUT_FRAMES * sizeof(float);

	UNUSED_PARAMETER(end_ts);

	/* mix buffers arrive zeroed; underruns simply leave silence */
	pthread_mutex_lock(&media->audio_mutex);
	if ((active_mixers & 1) != 0) {
		for (size_t ch = 0; ch < media->audio_channels; ch++) {
			size_t avail = media->audio_buf[ch].size;
			size_t copy = (avail < bytes) ? avail : bytes;
			if (copy && mixes[0].data[ch])
				deque_pop_front(&media->audio_buf[ch], mixes[0].data[ch], copy);
		}
	}
	pthread_mutex_unlock(&media->audio_mutex);

	*new_ts = start_ts;
	return true;
}

struct dyn_delay_media *dyn_delay_media_create(const char *media_path, obs_encoder_t *main_venc,
					       obs_encoder_t *main_aenc, dyn_delay_media_packet_cb cb, void *param)
{
	struct obs_video_info ovi;
	uint32_t width, height;

	if (!media_path || !*media_path || !main_venc || !cb)
		return NULL;
	if (!obs_get_video_info(&ovi))
		return NULL;

	width = obs_encoder_get_width(main_venc);
	height = obs_encoder_get_height(main_venc);
	if (!width || !height)
		return NULL;

	struct dyn_delay_media *media = bzalloc(sizeof(struct dyn_delay_media));
	media->cb = cb;
	media->param = param;
	pthread_mutex_init_value(&media->audio_mutex);
	if (pthread_mutex_init(&media->audio_mutex, NULL) != 0) {
		bfree(media);
		return NULL;
	}

	/* media source: plays immediately, loops, independent of activation */
	obs_data_t *src_settings = obs_data_create();
	obs_data_set_bool(src_settings, "is_local_file", true);
	obs_data_set_string(src_settings, "local_file", media_path);
	obs_data_set_bool(src_settings, "looping", true);
	obs_data_set_bool(src_settings, "restart_on_activate", false);
	obs_data_set_bool(src_settings, "close_when_inactive", false);
	media->source = obs_source_create_private("ffmpeg_source", "dynamic-delay-waiting-media", src_settings);
	obs_data_release(src_settings);
	if (!media->source) {
		blog(LOG_WARNING, "Dynamic Delay: failed to create media source for '%s'", media_path);
		goto fail;
	}

	/* wrap in a private scene so the media scales to the canvas */
	media->scene = obs_scene_create_private("dynamic-delay-waiting-scene");
	if (!media->scene)
		goto fail;

	obs_sceneitem_t *item = obs_scene_add(media->scene, media->source);
	if (item) {
		struct vec2 bounds;
		vec2_set(&bounds, (float)width, (float)height);
		obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
		obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
		obs_sceneitem_set_bounds(item, &bounds);
	}

	/* auxiliary view + video mix matching the main encoder's output size,
	 * so the media encoder needs no rescale */
	ovi.base_width = width;
	ovi.base_height = height;
	ovi.output_width = width;
	ovi.output_height = height;

	media->view = obs_view_create();
	if (!media->view)
		goto fail;
	obs_view_set_source(media->view, 0, obs_scene_get_source(media->scene));
	media->video = obs_view_add2(media->view, &ovi);
	if (!media->video)
		goto fail;

	/* standalone audio track fed by the source's audio capture callback */
	const struct audio_output_info *main_aoi = audio_output_get_info(obs_get_audio());
	if (main_aoi) {
		struct audio_output_info aoi = {0};
		aoi.name = "dynamic-delay-media-audio";
		aoi.samples_per_sec = main_aoi->samples_per_sec;
		aoi.format = AUDIO_FORMAT_FLOAT_PLANAR;
		aoi.speakers = main_aoi->speakers;
		aoi.input_callback = media_audio_input;
		aoi.input_param = media;

		media->audio_channels = get_audio_channels(aoi.speakers);
		media->audio_max_bytes = aoi.samples_per_sec * sizeof(float); /* ~1s per channel */

		if (audio_output_open(&media->audio, &aoi) == AUDIO_OUTPUT_SUCCESS)
			obs_source_add_audio_capture_callback(media->source, media_audio_capture, media);
		else
			media->audio = NULL;
	}

	/* mirror the main encoders so the wire format stays identical */
	obs_data_t *vsettings = obs_encoder_get_settings(main_venc);
	media->venc =
		obs_video_encoder_create(obs_encoder_get_id(main_venc), "dynamic-delay-media-video", vsettings, NULL);
	obs_data_release(vsettings);
	if (!media->venc)
		goto fail;
	obs_encoder_set_video(media->venc, media->video);

	if (main_aenc && media->audio) {
		obs_data_t *asettings = obs_encoder_get_settings(main_aenc);
		media->aenc = obs_audio_encoder_create(obs_encoder_get_id(main_aenc), "dynamic-delay-media-audio",
						       asettings, 0, NULL);
		obs_data_release(asettings);
		if (media->aenc)
			obs_encoder_set_audio(media->aenc, media->audio);
	}

	if (!obs_encoder_initialize(media->venc)) {
		blog(LOG_WARNING, "Dynamic Delay: failed to initialize media video encoder");
		goto fail;
	}
	if (media->aenc && !obs_encoder_initialize(media->aenc)) {
		blog(LOG_WARNING, "Dynamic Delay: failed to initialize media audio encoder, media will be silent");
		obs_encoder_release(media->aenc);
		media->aenc = NULL;
	}

	obs_encoder_start(media->venc, media_encoded_packet, media);
	if (media->aenc)
		obs_encoder_start(media->aenc, media_encoded_packet, media);
	media->started = true;

	blog(LOG_INFO, "Dynamic Delay: waiting media pipeline started ('%s', %ux%u)", media_path, width, height);
	return media;

fail:
	dyn_delay_media_destroy(media);
	return NULL;
}

void dyn_delay_media_stop(struct dyn_delay_media *media)
{
	if (!media || !media->started)
		return;

	media->started = false;
	if (media->venc)
		obs_encoder_stop(media->venc, media_encoded_packet, media);
	if (media->aenc)
		obs_encoder_stop(media->aenc, media_encoded_packet, media);
}

void dyn_delay_media_destroy(struct dyn_delay_media *media)
{
	if (!media)
		return;

	dyn_delay_media_stop(media);

	if (media->source)
		obs_source_remove_audio_capture_callback(media->source, media_audio_capture, media);

	obs_encoder_release(media->venc);
	obs_encoder_release(media->aenc);

	if (media->view) {
		obs_view_set_source(media->view, 0, NULL);
		if (media->video)
			obs_view_remove(media->view);
		obs_view_destroy(media->view);
	}

	if (media->audio)
		audio_output_close(media->audio);

	if (media->scene)
		obs_scene_release(media->scene);
	if (media->source)
		obs_source_release(media->source);

	for (size_t ch = 0; ch < MAX_AUDIO_CHANNELS; ch++)
		deque_free(&media->audio_buf[ch]);

	pthread_mutex_destroy(&media->audio_mutex);
	bfree(media);
}
