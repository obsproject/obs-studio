#include <obs-module.h>
#include <util/dstr.h>
#include <util/threading.h>

#define S_MODE                          "mode"
#define S_MODE_SHOW                     "mode_show"
#define S_MODE_HIDE                     "mode_hide"
#define S_MODE_REPEAT                   "mode_repeat"
#define S_START_VISIBLE                 "start_visible"
#define S_TIMER_HOLD                    "timer_hold"
#define S_TIMER_DELAY                   "timer_delay"
#define S_TIMER                         "timer"
#define S_TR_SPEED                      "transition_speed"
#define S_TRANSITION                    "transition"

#define TR_CUT                          "cut"
#define TR_FADE                         "fade"
#define TR_SWIPE                        "swipe"
#define TR_SLIDE                        "slide"

#define T_TR_(text) obs_module_text("VisibilityFilter.Transition." text)
#define T_TR_CUT                        T_TR_("Cut")
#define T_TR_FADE                       T_TR_("Fade")
#define T_TR_SWIPE                      T_TR_("Swipe")
#define T_TR_SLIDE                      T_TR_("Slide")

#define T_(text) obs_module_text("VisibilityFilter." text)
#define T_MODE                          T_("Mode")
#define T_MODE_SHOW                     T_("ModeShow")
#define T_MODE_HIDE                     T_("ModeHide")
#define T_MODE_REPEAT                   T_("ModeRepeat")
#define T_START_VISIBLE                 T_("StartVisible")
#define T_TIMER_HOLD                    T_("TimerHold")
#define T_TIMER_DELAY                   T_("TimerDelay")
#define T_TIMER                         T_("Timer")
#define T_TR_SPEED                      T_("TransitionSpeed")
#define T_TRANSITION                    T_("Transition")

enum mode {
	MODE_SHOW,
	MODE_HIDE,
	MODE_REPEAT,
};

struct vis_filter_data {
	obs_source_t *context;

	enum mode mode;

	float vis_time;
	float hold_time;
	float delay_time;
	float elapsed;
	float time;

	bool active;
	bool hold;
	bool start_visible;
	bool delay;
	bool first;

	uint32_t tr_speed;
	const char *tr_name;
	obs_source_t *transition;

	uint32_t cx;
	uint32_t cy;

	pthread_mutex_t mutex;
};

static obs_source_t *get_transition(struct vis_filter_data *vis)
{
	obs_source_t *tr;

	pthread_mutex_lock(&vis->mutex);
	tr = vis->transition;
	obs_source_addref(tr);
	pthread_mutex_unlock(&vis->mutex);

	return tr;
}

static void do_transition(void *data, obs_source_t *target)
{
	struct vis_filter_data *vis = data;

	if (!vis->transition)
		return;

	obs_transition_start(vis->transition,
			OBS_TRANSITION_MODE_AUTO,
			vis->tr_speed,
			target);
}

static const char *vis_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("VisibilityFilter");
}

static void vis_filter_update(void *data, obs_data_t *settings)
{
	struct vis_filter_data *vis = data;
	obs_source_t *new_tr = NULL;
	obs_source_t *old_tr = NULL;
	const char *tr_name;
	uint32_t new_speed;
	const char *mode;
	uint32_t cx = 0;
	uint32_t cy = 0;

	obs_source_t *target = obs_filter_get_parent(vis->context);

	tr_name = obs_data_get_string(settings, S_TRANSITION);
	if (astrcmpi(tr_name, TR_CUT) == 0)
		tr_name = "cut_transition";
	else if (astrcmpi(tr_name, TR_SWIPE) == 0)
		tr_name = "swipe_transition";
	else if (astrcmpi(tr_name, TR_SLIDE) == 0)
		tr_name = "slide_transition";
	else
		tr_name = "fade_transition";

	if (!vis->tr_name || strcmp(tr_name, vis->tr_name) != 0)
		new_tr = obs_source_create_private(tr_name, NULL, NULL);

	new_speed = (uint32_t)obs_data_get_int(settings, S_TR_SPEED);

	mode = obs_data_get_string(settings, S_MODE);
	vis->start_visible =
			obs_data_get_bool(settings, S_START_VISIBLE);

	vis->elapsed = 0.0f;
	vis->hold = false;
	vis->delay = false;
	vis->first = false;

	pthread_mutex_lock(&vis->mutex);

	if (new_tr) {
		old_tr = vis->transition;
		vis->transition = new_tr;
	}

	vis->tr_speed = new_speed;
	vis->tr_name = tr_name;

	vis->vis_time = (float)obs_data_get_int(settings, S_TIMER)
			/ 1000.0f;
	vis->hold_time = (float)obs_data_get_int(settings, S_TIMER_HOLD)
			/ 1000.0f;
	vis->delay_time = (float)obs_data_get_int(settings, S_TIMER_DELAY)
			/ 1000.0f;

	pthread_mutex_unlock(&vis->mutex);

	if (old_tr)
		obs_source_release(old_tr);

	cx = obs_source_get_base_width(target);
	cy = obs_source_get_base_height(target);

	vis->cx = cx;
	vis->cy = cy;
	obs_transition_set_size(vis->transition, cx, cy);
	obs_transition_set_alignment(vis->transition, OBS_ALIGN_CENTER);
	obs_transition_set_scale_type(vis->transition,
			OBS_TRANSITION_SCALE_ASPECT);

	if (new_tr)
		obs_source_add_active_child(target, new_tr);

	if (astrcmpi(mode, S_MODE_SHOW) == 0) {
		vis->mode = MODE_SHOW;
		obs_transition_set(vis->transition, NULL);
	} else if (astrcmpi(mode, S_MODE_HIDE) == 0) {
		vis->mode = MODE_HIDE;
		vis->first = true;
		if (vis->delay_time > 0.0f) {
			obs_transition_set(vis->transition, NULL);
			vis->delay = true;
		} else {
			obs_transition_set(vis->transition, target);
		}
	} else { /* S_MODE_REPEAT */
		vis->mode = MODE_REPEAT;

		if (vis->start_visible)
			obs_transition_set(vis->transition, target);
		else
			obs_transition_set(vis->transition, NULL);
	}
}

static void vis_filter_destroy(void *data)
{
	struct vis_filter_data *vis = data;

	obs_source_release(vis->transition);
	pthread_mutex_destroy(&vis->mutex);
	bfree(vis);
}

static void *vis_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct vis_filter_data *vis =
			bzalloc(sizeof(struct vis_filter_data));

	vis->context = context;

	pthread_mutex_init_value(&vis->mutex);
	if (pthread_mutex_init(&vis->mutex, NULL) != 0)
		goto error;

	vis->active = false;
	vis->hold = false;
	vis->delay = false;
	vis->first = false;

	vis_filter_update(vis, settings);

	return vis;

error:
	vis_filter_destroy(vis);
	return NULL;
}

static void vis_video_render(void *data, gs_effect_t *effect)
{
	struct vis_filter_data *vis = data;
	obs_source_t *transition = get_transition(vis);

	if (transition) {
		obs_source_video_render(transition);
		obs_source_release(transition);
	}

	UNUSED_PARAMETER(effect);
}

static void vis_filter_tick(void *data, float seconds)
{
	struct vis_filter_data *vis = data;

	if (!vis->transition)
		return;

	obs_source_t *target = obs_filter_get_parent(vis->context);

	if (vis->hold && vis->mode == MODE_REPEAT)
		vis->time = vis->hold_time;
	else if (vis->delay && vis->mode == MODE_HIDE)
		vis->time = vis->delay_time;
	else
		vis->time = vis->vis_time;

	if (vis->active != obs_source_active(target)) {
		vis->elapsed = 0.0f;
		vis->hold = false;
		vis->delay = false;
		vis->first = true;

		if (vis->delay_time > 0.0f && vis->mode == MODE_HIDE)
			vis->delay = true;

		if (vis->mode == MODE_SHOW)
			obs_transition_set(vis->transition, NULL);
		else if (vis->mode == MODE_HIDE)
			if (vis->delay)
				obs_transition_set(vis->transition, NULL);
			else
				obs_transition_set(vis->transition, target);
		else if (vis->mode == MODE_REPEAT)
			obs_transition_set(vis->transition, NULL);
	} else {
		vis->elapsed += seconds;
	}

	if (vis->elapsed > vis->time) {
		vis->elapsed = 0.0f;

		if (vis->mode == MODE_SHOW) {
			do_transition(vis, target);
		} else if (vis->mode == MODE_HIDE) {
			if (vis->delay) {
				do_transition(vis, target);
				vis->delay = false;
			} else {
				if (vis->first) {
					obs_transition_set(vis->transition,
							target);
					do_transition(vis, NULL);
					vis->first = false;
				}
			}

		} else if (vis->mode == MODE_REPEAT) {
			if (vis->hold && !vis->start_visible) {
				obs_transition_set(vis->transition, target);
				do_transition(vis, NULL);
			} else if (!vis->hold && !vis->start_visible) {
				obs_transition_set(vis->transition, NULL);
				do_transition(vis, target);
			} else if (!vis->hold && vis->start_visible) {
				obs_transition_set(vis->transition, target);
				do_transition(vis, NULL);
			} else if (vis->hold && vis->start_visible) {
				obs_transition_set(vis->transition, NULL);
				do_transition(vis, target);
			}

			vis->hold = !vis->hold;
		}
	}

	vis->active = obs_source_active(target);
}

static inline bool vis_audio_render_(obs_source_t *transition, uint64_t *ts_out,
		struct obs_source_audio_mix *audio_output,
		uint32_t mixers, size_t channels, size_t sample_rate)
{
	struct obs_source_audio_mix child_audio;
	uint64_t source_ts;

	if (obs_source_audio_pending(transition))
		return false;

	source_ts = obs_source_get_audio_timestamp(transition);
	if (!source_ts)
		return false;

	obs_source_get_audio_mix(transition, &child_audio);
	for (size_t mix = 0; mix < MAX_AUDIO_MIXES; mix++) {
		if ((mixers & (1 << mix)) == 0)
			continue;

		for (size_t ch = 0; ch < channels; ch++) {
			float *out = audio_output->output[mix].data[ch];
			float *in = child_audio.output[mix].data[ch];

			memcpy(out, in, AUDIO_OUTPUT_FRAMES *
					MAX_AUDIO_CHANNELS * sizeof(float));
		}
	}

	*ts_out = source_ts;

	UNUSED_PARAMETER(sample_rate);
	return true;
}

static bool vis_audio_render(void *data, uint64_t *ts_out,
		struct obs_source_audio_mix *audio_output,
		uint32_t mixers, size_t channels, size_t sample_rate)
{
	struct vis_filter_data *vis = data;
	obs_source_t *transition = get_transition(vis);
	bool success;

	if (!transition)
		return false;

	success = vis_audio_render_(transition, ts_out, audio_output, mixers,
			channels, sample_rate);

	obs_source_release(transition);
	return success;
}

static bool settings_modified(obs_properties_t *props,
		obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);

	const char *mode = obs_data_get_string(settings, S_MODE);
	obs_property_t *delay = obs_properties_get(props, S_TIMER_DELAY);

	if (astrcmpi(mode, S_MODE_HIDE) == 0)
		obs_property_set_visible(delay, true);
	else
		obs_property_set_visible(delay, false);

	bool enabled = (astrcmpi(mode, S_MODE_REPEAT) == 0);

	obs_property_t *start = obs_properties_get(props, S_START_VISIBLE);
	obs_property_t *hold = obs_properties_get(props, S_TIMER_HOLD);

	obs_property_set_visible(start, enabled);
	obs_property_set_visible(hold, enabled);

	return true;
}

static void vis_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_TIMER, 10000);
	obs_data_set_default_int(settings, S_TIMER_HOLD, 10000);
	obs_data_set_default_int(settings, S_TIMER_DELAY, 0);
	obs_data_set_default_bool(settings, S_START_VISIBLE, true);
	obs_data_set_default_int(settings, S_TR_SPEED, 700);
	obs_data_set_default_string(settings, S_TRANSITION, "fade");
}

static obs_properties_t *vis_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;
	obs_property_t *prop;

	p = obs_properties_add_list(props, S_MODE, T_MODE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_MODE_HIDE, S_MODE_HIDE);
	obs_property_list_add_string(p, T_MODE_SHOW, S_MODE_SHOW);
	obs_property_list_add_string(p, T_MODE_REPEAT, S_MODE_REPEAT);

	prop = obs_properties_add_list(props, S_TRANSITION, T_TRANSITION,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(prop, T_TR_CUT, TR_CUT);
	obs_property_list_add_string(prop, T_TR_FADE, TR_FADE);
	obs_property_list_add_string(prop, T_TR_SWIPE, TR_SWIPE);
	obs_property_list_add_string(prop, T_TR_SLIDE, TR_SLIDE);

	obs_properties_add_int(props, S_TR_SPEED, T_TR_SPEED,
			0, 3600000, 50);

	obs_property_set_modified_callback(p, settings_modified);

	obs_properties_add_bool(props, S_START_VISIBLE, T_START_VISIBLE);

	obs_properties_add_int(props, S_TIMER, T_TIMER,
			1, 3600000, 1);

	obs_properties_add_int(props, S_TIMER_HOLD, T_TIMER_HOLD,
			1, 3600000, 1);

	obs_properties_add_int(props, S_TIMER_DELAY, T_TIMER_DELAY,
			0, 3600000, 1);

	UNUSED_PARAMETER(data);
	return props;
}

struct obs_source_info visibility_filter = {
	.id = "visibility_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = vis_filter_name,
	.create = vis_filter_create,
	.destroy = vis_filter_destroy,
	.update = vis_filter_update,
	.get_properties = vis_filter_properties,
	.get_defaults = vis_filter_defaults,
	.video_tick = vis_filter_tick,
	.video_render = vis_video_render,
	.audio_render = vis_audio_render
};
