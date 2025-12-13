/*
obs-websocket
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

// Similar example code can be found in ../../src/obs-websocket.cpp
// You can test that sample code by specifying -DPLUGIN_TESTS=TRUE

#include <obs-module.h>

#include "../obs-websocket-api.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

obs_websocket_vendor vendor;

bool obs_module_load(void)
{
	blog(LOG_INFO, "Example obs-websocket-api plugin loaded!");
	return true;
}

void example_request_cb(obs_data_t *request_data, obs_data_t *response_data, void *priv_data);
void obs_module_post_load(void)
{
	vendor = obs_websocket_register_vendor("api_example_plugin");
	if (!vendor) {
		blog(LOG_ERROR, "Vendor registration failed! (obs-websocket should have logged something if installed properly.)");
		return;
	}

	if (!obs_websocket_vendor_register_request(vendor, "example_request", example_request_cb, NULL))
		blog(LOG_ERROR, "Failed to register `example_request` request with obs-websocket.");

	uint api_version = obs_websocket_get_api_version();
	if (api_version == 0) {
		blog(LOG_ERROR, "Unable to fetch obs-websocket plugin API version.");
		return;
	} else if (api_version == 1) {
		blog(LOG_WARNING, "Unsupported obs-websocket plugin API version for calling requests.");
		return;
	}

	struct obs_websocket_request_response *response = obs_websocket_call_request("GetVersion");
	if (!response) {
		blog(LOG_ERROR, "Failed to call GetVersion due to obs-websocket not being installed.");
		return;
	}
	blog(LOG_INFO, "[obs_module_post_load] Called GetVersion. Status Code: %u | Comment: %s | Response Data: %s",
	     response->status_code, response->comment, response->response_data);
	obs_websocket_request_response_free(response);
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "Example obs-websocket-api plugin unloaded!");
}

void example_request_cb(obs_data_t *request_data, obs_data_t *response_data, void *priv_data)
{
	if (obs_data_has_user_value(request_data, "ping"))
		obs_data_set_bool(response_data, "pong", true);

	UNUSED_PARAMETER(priv_data);
}
