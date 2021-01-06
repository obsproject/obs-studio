#include <obs-module.h>
#include <util/dstr.h>

#define TIMING_TIME 0
#define TIMING_FRAME 1

enum fade_style { FADE_STYLE_FADE_OUT_FADE_IN, FADE_STYLE_CROSS_FADE };

struct stinger_info {
	obs_source_t *source;

	obs_source_t *media_source;

	uint64_t duration_ns;
	uint64_t duration_frames;
	uint64_t transition_point_ns;
	uint64_t transition_point_frame;
	float transition_point;
	float transition_a_mul;
	float transition_b_mul;
	bool transitioning;
	bool transition_point_is_frame;
	int monitoring_type;
	enum fade_style fade_style;

	float (*mix_a)(void *data, float t);
	float (*mix_b)(void *data, float t);
};

static const char *stinger_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("StingerTransition");
}

static float mix_a_fade_in_out(void *data, float t);
static float mix_b_fade_in_out(void *data, float t);
static float mix_a_cross_fade(void *data, float t);
static float mix_b_cross_fade(void *data, float t);

static void stinger_update(void *data, obs_data_t *settings)
{
	struct stinger_info *s = data;
	const char *path = obs_data_get_string(settings, "path");
	bool hw_decode = obs_data_get_bool(settings, "hw_decode");

	obs_data_t *media_settings = obs_data_create();
	obs_data_set_string(media_settings, "local_file", path);
	obs_data_set_bool(media_settings, "hw_decode", hw_decode);

	obs_source_release(s->media_source);
	struct dstr name;
	dstr_init_copy(&name, obs_source_get_name(s->source));
	dstr_cat(&name, " (Stinger)");
	s->media_source = obs_source_create_private("ffmpeg_source", name.array,
						    media_settings);
	dstr_free(&name);
	obs_data_release(media_settings);

	int64_t point = obs_data_get_int(settings, "transition_point");

	s->transition_point_is_frame = obs_data_get_int(settings, "tp_type") ==
				       TIMING_FRAME;

	if (s->transition_point_is_frame)
		s->transition_point_frame = (uint64_t)point;
	else
		s->transition_point_ns = (uint64_t)(point * 1000000LL);

	s->monitoring_type =
		(int)obs_data_get_int(settings, "audio_monitoring");
	obs_source_set_monitoring_type(s->media_source, s->monitoring_type);

	s->fade_style =
		(enum fade_style)obs_data_get_int(settings, "audio_fade_style");

	switch (s->fade_style) {
	default:
	case FADE_STYLE_FADE_OUT_FADE_IN:
		s->mix_a = mix_a_fade_in_out;
		s->mix_b = mix_b_fade_in_out;
		break;
	case FADE_STYLE_CROSS_FADE:
		s->mix_a = mix_a_cross_fade;
		s->mix_b = mix_b_cross_fade;
		break;
	}
}

static void *stinger_create(obs_data_t *settings, obs_source_t *source)
{
	struct stinger_info *s = bzalloc(sizeof(*s));

	s->source = source;
	s->mix_a = mix_a_fade_in_out;
	s->mix_b = mix_b_fade_in_out;

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

static void stinger_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "hw_decode", true);
}

static void stinger_video_render(void *data, gs_effect_t *effect)
{
	struct stinger_info *s = data;

	float t = obs_transition_get_time(s->source);
	bool use_a = t < s->transition_point;

	enum obs_transition_target target = use_a ? OBS_TRANSITION_SOURCE_A
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

static float mix_a_fade_in_out(void *data, float t)
{
	struct stinger_info *s = data;
	return 1.0f - calc_fade(t, s->transition_a_mul);
}

static float mix_b_fade_in_out(void *data, float t)
{
	struct stinger_info *s = data;
	return 1.0f - calc_fade(1.0f - t, s->transition_b_mul);
}

static float mix_a_cross_fade(void *data, float t)
{
	UNUSED_PARAMETER(data);
	return 1.0f - t;
}

static float mix_b_cross_fade(void *data, float t)
{
	UNUSED_PARAMETER(data);
	return t;
}

static bool stinger_audio_render(void *data, uint64_t *ts_out,
				 struct obs_source_audio_mix *audio,
				 uint32_t mixers, size_t channels,
				 size_t sample_rate)
{
	struct stinger_info *s = data;
	uint64_t ts = 0;

	if (!obs_source_audio_pending(s->media_source)) {
		ts = obs_source_get_audio_timestamp(s->media_source);
		if (!ts)
			return false;
	}

	bool success = obs_transition_audio_render(s->source, ts_out, audio,
						   mixers, channels,
						   sample_rate, s->mix_a,
						   s->mix_b);
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
		proc_handler_call(ph, "get_nb_frames", &cd);
		s->duration_ns =
			(uint64_t)calldata_int(&cd, "duration") + 250000000ULL;
		s->duration_frames = (uint64_t)calldata_int(&cd, "num_frames");

		if (s->transition_point_is_frame)
			s->transition_point =
				(float)((long double)s->transition_point_frame /
					(long double)s->duration_frames);
		else
			s->transition_point =
				(float)((long double)s->transition_point_ns /
					(long double)s->duration_ns);

		if (s->transition_point > 0.999f)
			s->transition_point = 0.999f;
		else if (s->transition_point < 0.001f)
			s->transition_point = 0.001f;

		s->transition_a_mul = (1.0f / s->transition_point);
		s->transition_b_mul = (1.0f / (1.0f - s->transition_point));

		obs_transition_enable_fixed(
			s->source, true, (uint32_t)(s->duration_ns / 1000000));

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
					obs_source_enum_proc_t enum_callback,
					void *param)
{
	struct stinger_info *s = data;
	if (s->media_source && s->transitioning)
		enum_callback(s->source, s->media_source, param);
}

static void stinger_enum_all_sources(void *data,
				     obs_source_enum_proc_t enum_callback,
				     void *param)
{
	struct stinger_info *s = data;
	if (s->media_source)
		enum_callback(s->source, s->media_source, param);
}

#define FILE_FILTER \
	"Video Files (*.mp4 *.ts *.mov *.wmv *.flv *.mkv *.avi *.gif *.webm);;"

static bool transition_point_type_modified(obs_properties_t *ppts,
					   obs_property_t *p, obs_data_t *s)
{
	int64_t type = obs_data_get_int(s, "tp_type");
	p = obs_properties_get(ppts, "transition_point");

	if (type == TIMING_TIME) {
		obs_property_set_description(
			p, obs_module_text("TransitionPoint"));
		obs_property_int_set_suffix(p, " ms");
	} else {
		obs_property_set_description(
			p, obs_module_text("TransitionPointFrame"));
		obs_property_int_set_suffix(p, "");
	}
	return true;
}

static obs_properties_t *stinger_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();

	obs_properties_set_flags(ppts, OBS_PROPERTIES_DEFER_UPDATE);

	obs_properties_add_path(ppts, "path", obs_module_text("VideoFile"),
				OBS_PATH_FILE, FILE_FILTER, NULL);
	obs_property_t *p = obs_properties_add_list(
		ppts, "tp_type", obs_module_text("TransitionPointType"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
#ifndef __APPLE__
	obs_properties_add_bool(ppts, "hw_decode",
				obs_module_text("HardwareDecode"));
#endif
	obs_property_list_add_int(p, obs_module_text("TransitionPointTypeTime"),
				  TIMING_TIME);
	obs_property_list_add_int(
		p, obs_module_text("TransitionPointTypeFrame"), TIMING_FRAME);

	obs_property_set_modified_callback(p, transition_point_type_modified);

	obs_properties_add_int(ppts, "transition_point",
			       obs_module_text("TransitionPoint"), 0, 120000,
			       1);

	obs_property_t *monitor_list = obs_properties_add_list(
		ppts, "audio_monitoring", obs_module_text("AudioMonitoring"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(monitor_list,
				  obs_module_text("AudioMonitoring.None"),
				  OBS_MONITORING_TYPE_NONE);
	obs_property_list_add_int(
		monitor_list, obs_module_text("AudioMonitoring.MonitorOnly"),
		OBS_MONITORING_TYPE_MONITOR_ONLY);
	obs_property_list_add_int(monitor_list,
				  obs_module_text("AudioMonitoring.Both"),
				  OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);

	obs_property_t *audio_fade_style = obs_properties_add_list(
		ppts, "audio_fade_style", obs_module_text("AudioFadeStyle"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(
		audio_fade_style,
		obs_module_text("AudioFadeStyle.FadeOutFadeIn"),
		FADE_STYLE_FADE_OUT_FADE_IN);
	obs_property_list_add_int(audio_fade_style,
				  obs_module_text("AudioFadeStyle.CrossFade"),
				  FADE_STYLE_CROSS_FADE);

	UNUSED_PARAMETER(data);
	return ppts;
}

struct obs_source_info stinger_transition = {
	.id = "obs_stinger_transition",
	.type = OBS_SOURCE_TYPE_TRANSITION,
	.get_name = stinger_get_name,
	.create = stinger_create,
	.destroy = stinger_destroy,
	.update = stinger_update,
	.get_defaults = stinger_defaults,
	.video_render = stinger_video_render,
	.audio_render = stinger_audio_render,
	.get_properties = stinger_properties,
	.enum_active_sources = stinger_enum_active_sources,
	.enum_all_sources = stinger_enum_all_sources,
	.transition_start = stinger_transition_start,
	.transition_stop = stinger_transition_stop,
};
