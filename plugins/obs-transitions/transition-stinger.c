#include <obs-module.h>

struct stinger_info {
	obs_source_t *source;

	obs_source_t *media_source;

	uint64_t duration_ns;
	uint64_t transition_point_ns;
	float transition_point;
	float transition_a_mul;
	float transition_b_mul;
	bool transitioning;
};

static const char *stinger_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("StingerTransition");
}

static void stinger_update(void *data, obs_data_t *settings)
{
	struct stinger_info *s = data;
	const char *path = obs_data_get_string(settings, "path");

	obs_data_t *media_settings = obs_data_create();
	obs_data_set_string(media_settings, "local_file", path);

	obs_source_release(s->media_source);
	s->media_source = obs_source_create_private("ffmpeg_source", NULL,
			media_settings);
	obs_data_release(media_settings);

	int64_t point_ms = obs_data_get_int(settings, "transition_point");

	s->transition_point_ns = (uint64_t)(point_ms * 1000000LL);
}

static void *stinger_create(obs_data_t *settings, obs_source_t *source)
{
	struct stinger_info *s = bzalloc(sizeof(*s));

	s->source = source;

	obs_transition_enable_fixed(s->source, true, 0);
	obs_source_update(source, settings);
	return s;
}

static void stinger_destroy(void *data)
{
	struct stinger_info *s = data;
	obs_source_release(s->media_source);
	bfree(s);
}

static void stinger_video_render(void *data, gs_effect_t *effect)
{
	struct stinger_info *s = data;

	float t = obs_transition_get_time(s->source);
	bool use_a = t < s->transition_point;

	enum obs_transition_target target = use_a
		? OBS_TRANSITION_SOURCE_A
		: OBS_TRANSITION_SOURCE_B;

	if (!obs_transition_video_render_direct(s->source, target))
		return;

	/* --------------------- */

	float source_cx = (float)obs_source_get_width(s->source);
	float source_cy = (float)obs_source_get_height(s->source);
	uint32_t media_cx = obs_source_get_width(s->media_source);
	uint32_t media_cy = obs_source_get_height(s->media_source);

	if (!media_cx || !media_cy)
		return;

	float scale_x = source_cx / (float)media_cx;
	float scale_y = source_cy / (float)media_cy;

	gs_matrix_push();
	gs_matrix_scale3f(scale_x, scale_y, 1.0f);
	obs_source_video_render(s->media_source);
	gs_matrix_pop();

	UNUSED_PARAMETER(effect);
}

static inline float calc_fade(float t, float mul)
{
	t *= mul;
	return t > 1.0f ? 1.0f : t;
}

static float mix_a(void *data, float t)
{
	struct stinger_info *s = data;
	return 1.0f - calc_fade(t, s->transition_a_mul);
}

static float mix_b(void *data, float t)
{
	struct stinger_info *s = data;
	return 1.0f - calc_fade(1.0f - t, s->transition_b_mul);
}

static bool stinger_audio_render(void *data, uint64_t *ts_out,
		struct obs_source_audio_mix *audio, uint32_t mixers,
		size_t channels, size_t sample_rate)
{
	struct stinger_info *s = data;
	uint64_t ts = 0;

	if (!obs_source_audio_pending(s->media_source)) {
		ts = obs_source_get_audio_timestamp(s->media_source);
		if (!ts)
			return false;
	}

	bool success = obs_transition_audio_render(s->source, ts_out,
		audio, mixers, channels, sample_rate, mix_a, mix_b);
	if (!ts)
		return success;

	if (!*ts_out || ts < *ts_out)
		*ts_out = ts;

	struct obs_source_audio_mix child_audio;
	obs_source_get_audio_mix(s->media_source, &child_audio);

	for (size_t mix = 0; mix < MAX_AUDIO_MIXES; mix++) {
		if ((mixers & (1 << mix)) == 0)
			continue;

		for (size_t ch = 0; ch < channels; ch++) {
			register float *out = audio->output[mix].data[ch];
			register float *in = child_audio.output[mix].data[ch];
			register float *end = in + AUDIO_OUTPUT_FRAMES;

			while (in < end)
				*(out++) += *(in++);
		}
	}

	return true;
}

static void stinger_transition_start(void *data)
{
	struct stinger_info *s = data;

	if (s->media_source) {
		calldata_t cd = {0};

		proc_handler_t *ph =
			obs_source_get_proc_handler(s->media_source);

		if (s->transitioning) {
			proc_handler_call(ph, "restart", &cd);
			return;
		}

		proc_handler_call(ph, "get_duration", &cd);

		s->duration_ns = (uint64_t)calldata_int(&cd, "duration");

		s->transition_point = (float)(
			(long double)s->transition_point_ns /
			(long double)s->duration_ns);
		if (s->transition_point > 1.0f)
			s->transition_point = 1.0f;
		else if (s->transition_point < 0.001f)
			s->transition_point = 0.001f;

		s->transition_a_mul = (1.0f / s->transition_point);
		s->transition_b_mul = (1.0f / (1.0f - s->transition_point));

		obs_transition_enable_fixed(s->source, true,
				(uint32_t)(s->duration_ns / 1000000));

		calldata_free(&cd);

		obs_source_add_active_child(s->source, s->media_source);
	}

	s->transitioning = true;
}

static void stinger_transition_stop(void *data)
{
	struct stinger_info *s = data;

	if (s->media_source)
		obs_source_remove_active_child(s->source, s->media_source);

	s->transitioning = false;
}

static void stinger_enum_active_sources(void *data,
		obs_source_enum_proc_t enum_callback, void *param)
{
	struct stinger_info *s = data;
	if (s->media_source && s->transitioning)
		enum_callback(s->source, s->media_source, param);
}

static void stinger_enum_all_sources(void *data,
		obs_source_enum_proc_t enum_callback, void *param)
{
	struct stinger_info *s = data;
	if (s->media_source)
		enum_callback(s->source, s->media_source, param);
}

#define FILE_FILTER \
	"Video Files (*.mp4 *.ts *.mov *.wmv *.flv *.mkv *.avi *.gif *.webm);;"

static obs_properties_t *stinger_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_set_flags(ppts, OBS_PROPERTIES_DEFER_UPDATE);

	obs_properties_add_path(ppts, "path",
			obs_module_text("VideoFile"),
			OBS_PATH_FILE,
			FILE_FILTER, NULL);
	obs_properties_add_int(ppts, "transition_point",
			obs_module_text("TransitionPoint"),
			0, 120000, 1);

	UNUSED_PARAMETER(data);
	return ppts;
}

struct obs_source_info stinger_transition = {
	.id = "stinger_transition",
	.type = OBS_SOURCE_TYPE_TRANSITION,
	.get_name = stinger_get_name,
	.create = stinger_create,
	.destroy = stinger_destroy,
	.update = stinger_update,
	.video_render = stinger_video_render,
	.audio_render = stinger_audio_render,
	.get_properties = stinger_properties,
	.enum_active_sources = stinger_enum_active_sources,
	.enum_all_sources = stinger_enum_all_sources,
	.transition_start = stinger_transition_start,
	.transition_stop = stinger_transition_stop
};
