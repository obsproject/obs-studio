#include "audio-helpers.h"

#include <util/dstr.h>

static inline bool settings_changed(obs_data_t *old_settings,
				    obs_data_t *new_settings)
{
	const char *old_window = obs_data_get_string(old_settings, "window");
	const char *new_window = obs_data_get_string(new_settings, "window");

	enum window_priority old_priority =
		obs_data_get_int(old_settings, "priority");
	enum window_priority new_priority =
		obs_data_get_int(new_settings, "priority");

	// Changes to priority only matter if a window is set
	return (old_priority != new_priority && *new_window) ||
	       strcmp(old_window, new_window) != 0;
}

static inline void reroute_wasapi_source(obs_source_t *wasapi,
					 obs_source_t *target)
{
	proc_handler_t *ph = obs_source_get_proc_handler(wasapi);
	calldata_t cd = {0};
	calldata_set_ptr(&cd, "target", target);
	proc_handler_call(ph, "reroute_audio", &cd);
	calldata_free(&cd);
}

void setup_audio_source(obs_source_t *parent, obs_source_t **child,
			const char *window, bool enabled,
			enum window_priority priority)
{
	if (enabled) {
		obs_data_t *wasapi_settings = NULL;

		if (window) {
			wasapi_settings = obs_data_create();
			obs_data_set_string(wasapi_settings, "window", window);
			obs_data_set_int(wasapi_settings, "priority", priority);
		}

		if (!*child) {
			struct dstr name = {0};
			dstr_printf(&name, "%s (%s)",
				    obs_source_get_name(parent),
				    TEXT_CAPTURE_AUDIO_SUFFIX);

			*child = obs_source_create_private(
				AUDIO_SOURCE_TYPE, name.array, wasapi_settings);

			// Ensure child gets activated/deactivated properly
			obs_source_add_active_child(parent, *child);
			// Reroute audio to come from window/game capture source
			reroute_wasapi_source(*child, parent);
			// Show source in mixer
			obs_source_set_audio_active(parent, true);

			dstr_free(&name);
		} else if (wasapi_settings) {
			obs_data_t *old_settings =
				obs_source_get_settings(*child);
			// Only bother updating if settings changed
			if (settings_changed(old_settings, wasapi_settings))
				obs_source_update(*child, wasapi_settings);

			obs_data_release(old_settings);
		}

		obs_data_release(wasapi_settings);
	} else {
		obs_source_set_audio_active(parent, false);

		if (*child) {
			reroute_wasapi_source(*child, NULL);
			obs_source_remove_active_child(parent, *child);
			obs_source_release(*child);
			*child = NULL;
		}
	}
}

static inline void encode_dstr(struct dstr *str)
{
	dstr_replace(str, "#", "#22");
	dstr_replace(str, ":", "#3A");
}

void reconfigure_audio_source(obs_source_t *source, HWND window)
{
	struct dstr title = {0};
	struct dstr class = {0};
	struct dstr exe = {0};
	struct dstr encoded = {0};

	ms_get_window_title(&title, window);
	ms_get_window_class(&class, window);
	ms_get_window_exe(&exe, window);

	encode_dstr(&title);
	encode_dstr(&class);
	encode_dstr(&exe);

	dstr_cat_dstr(&encoded, &title);
	dstr_cat(&encoded, ":");
	dstr_cat_dstr(&encoded, &class);
	dstr_cat(&encoded, ":");
	dstr_cat_dstr(&encoded, &exe);

	obs_data_t *audio_settings = obs_data_create();
	obs_data_set_string(audio_settings, "window", encoded.array);
	obs_data_set_int(audio_settings, "priority", WINDOW_PRIORITY_CLASS);

	obs_source_update(source, audio_settings);

	obs_data_release(audio_settings);
	dstr_free(&encoded);
	dstr_free(&title);
	dstr_free(&class);
	dstr_free(&exe);
}

void rename_audio_source(void *param, calldata_t *data)
{
	obs_source_t *src = *(obs_source_t **)param;
	if (!src)
		return;

	struct dstr name = {0};
	dstr_printf(&name, "%s (%s)", calldata_string(data, "new_name"),
		    TEXT_CAPTURE_AUDIO_SUFFIX);

	obs_source_set_name(src, name.array);
	dstr_free(&name);
}
