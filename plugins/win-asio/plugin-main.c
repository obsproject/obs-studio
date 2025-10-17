/* Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 * This file is part of win-asio.
 *
 * This file uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/threading.h>

const char *PLUGIN_VERSION = "1.0.0";

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-asio", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "ASIO audio plugin";
}

extern os_event_t *shutting_down;
extern struct obs_source_info asio_input_capture;
extern struct obs_output_info asio_output;
bool retrieve_device_list();
void free_device_list();
void OBSEvent(enum obs_frontend_event event, void *);

bool obs_module_load(void)
{
	if (!retrieve_device_list())
		return false;

	obs_register_source(&asio_input_capture);
	blog(LOG_INFO, "ASIO plugin loaded successfully (version %s)", PLUGIN_VERSION);

	if (os_event_init(&shutting_down, OS_EVENT_TYPE_AUTO))
		return false;

	obs_frontend_add_event_callback(OBSEvent, NULL);
	obs_register_output(&asio_output);
	return true;
}

void obs_module_unload()
{
	free_device_list();
	os_event_destroy(shutting_down);
	obs_frontend_remove_event_callback(OBSEvent, NULL);
}
