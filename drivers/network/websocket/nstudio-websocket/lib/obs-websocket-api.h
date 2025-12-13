/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2022 Kyle Manning <tt2468@gmail.com>

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

#ifndef _OBS_WEBSOCKET_API_H
#define _OBS_WEBSOCKET_API_H

#include <obs.h>

#define OBS_WEBSOCKET_API_VERSION 3

#ifdef __cplusplus
extern "C" {
#endif

typedef void *obs_websocket_vendor;
typedef void (*obs_websocket_request_callback_function)(obs_data_t *, obs_data_t *, void *);
typedef void (*obs_websocket_event_callback_function)(uint64_t, const char *, const char *, void *);

struct obs_websocket_request_response {
	unsigned int status_code;
	char *comment;
	char *response_data; // JSON string, because obs_data_t* only supports array<object>, so conversions would break API.
};

/* ==================== INTERNAL DEFINITIONS ==================== */

struct obs_websocket_request_callback {
	obs_websocket_request_callback_function callback;
	void *priv_data;
};

struct obs_websocket_event_callback {
	obs_websocket_event_callback_function callback;
	void *priv_data;
};

static proc_handler_t *_ph;

/* ==================== INTERNAL API FUNCTIONS ==================== */

static inline proc_handler_t *obs_websocket_get_ph(void)
{
	proc_handler_t *global_ph = obs_get_proc_handler();
	assert(global_ph != NULL);

	calldata_t cd = {0, 0, 0, 0};
	if (!proc_handler_call(global_ph, "obs_websocket_api_get_ph", &cd))
		blog(LOG_DEBUG, "Unable to fetch obs-websocket proc handler object. obs-websocket not installed?");
	proc_handler_t *ret = (proc_handler_t *)calldata_ptr(&cd, "ph");
	calldata_free(&cd);

	return ret;
}

static inline bool obs_websocket_ensure_ph(void)
{
	if (!_ph)
		_ph = obs_websocket_get_ph();
	return _ph != NULL;
}

static inline bool obs_websocket_vendor_run_simple_proc(obs_websocket_vendor vendor, const char *proc_name, calldata_t *cd)
{
	if (!obs_websocket_ensure_ph())
		return false;

	if (!vendor || !proc_name || !strlen(proc_name) || !cd)
		return false;

	calldata_set_ptr(cd, "vendor", vendor);

	proc_handler_call(_ph, proc_name, cd);
	return calldata_bool(cd, "success");
}

/* ==================== GENERAL API FUNCTIONS ==================== */

// Gets the API version built with the obs-websocket plugin
static inline unsigned int obs_websocket_get_api_version(void)
{
	if (!obs_websocket_ensure_ph())
		return 0;

	calldata_t cd = {0, 0, 0, 0};

	if (!proc_handler_call(_ph, "get_api_version", &cd))
		return 1; // API v1 does not include get_api_version

	unsigned int ret = (unsigned int)calldata_int(&cd, "version");

	calldata_free(&cd);

	return ret;
}

// Calls an obs-websocket request. Free response with `obs_websocket_request_response_free()`
static inline struct obs_websocket_request_response *obs_websocket_call_request(const char *request_type, obs_data_t *request_data
#ifdef __cplusplus
													  = NULL
#endif
)
{
	if (!obs_websocket_ensure_ph())
		return NULL;

	const char *request_data_string = NULL;
	if (request_data)
		request_data_string = obs_data_get_json(request_data);

	calldata_t cd = {0, 0, 0, 0};
	calldata_set_string(&cd, "request_type", request_type);
	calldata_set_string(&cd, "request_data", request_data_string);

	proc_handler_call(_ph, "call_request", &cd);

	struct obs_websocket_request_response *ret = (struct obs_websocket_request_response *)calldata_ptr(&cd, "response");

	calldata_free(&cd);

	return ret;
}

// Free a request response object returned by `obs_websocket_call_request()`
static inline void obs_websocket_request_response_free(struct obs_websocket_request_response *response)
{
	if (!response)
		return;

	if (response->comment)
		bfree(response->comment);
	if (response->response_data)
		bfree(response->response_data);
	bfree(response);
}

// Register an event handler to receive obs-websocket events
static inline bool obs_websocket_register_event_callback(obs_websocket_event_callback_function event_callback, void *priv_data)
{
	if (!obs_websocket_ensure_ph())
		return false;

	struct obs_websocket_event_callback cb = {event_callback, priv_data};

	calldata_t cd = {0, 0, 0, 0};
	calldata_set_ptr(&cd, "callback", &cb);

	proc_handler_call(_ph, "register_event_callback", &cd);

	bool ret = calldata_bool(&cd, "success");

	calldata_free(&cd);

	return ret;
}

// Unregister an existing event handler
static inline bool obs_websocket_unregister_event_callback(obs_websocket_event_callback_function event_callback, void *priv_data)
{
	if (!obs_websocket_ensure_ph())
		return false;

	struct obs_websocket_event_callback cb = {event_callback, priv_data};

	calldata_t cd = {0, 0, 0, 0};
	calldata_set_ptr(&cd, "callback", &cb);

	proc_handler_call(_ph, "unregister_event_callback", &cd);

	bool ret = calldata_bool(&cd, "success");

	calldata_free(&cd);

	return ret;
}

/* ==================== VENDOR API FUNCTIONS ==================== */

// ALWAYS CALL ONLY VIA `obs_module_post_load()` CALLBACK!
// Registers a new "vendor" (Example: obs-ndi)
static inline obs_websocket_vendor obs_websocket_register_vendor(const char *vendor_name)
{
	if (!obs_websocket_ensure_ph())
		return NULL;

	calldata_t cd = {0, 0, 0, 0};
	calldata_set_string(&cd, "name", vendor_name);

	proc_handler_call(_ph, "vendor_register", &cd);
	obs_websocket_vendor ret = calldata_ptr(&cd, "vendor");
	calldata_free(&cd);

	return ret;
}

// Registers a new request for a vendor
static inline bool obs_websocket_vendor_register_request(obs_websocket_vendor vendor, const char *request_type,
							 obs_websocket_request_callback_function request_callback, void *priv_data)
{
	struct obs_websocket_request_callback cb = {request_callback, priv_data};

	calldata_t cd = {0, 0, 0, 0};
	calldata_set_string(&cd, "type", request_type);
	calldata_set_ptr(&cd, "callback", &cb);

	bool success = obs_websocket_vendor_run_simple_proc(vendor, "vendor_request_register", &cd);
	calldata_free(&cd);

	return success;
}

// Unregisters an existing vendor request
static inline bool obs_websocket_vendor_unregister_request(obs_websocket_vendor vendor, const char *request_type)
{
	calldata_t cd = {0, 0, 0, 0};
	calldata_set_string(&cd, "type", request_type);

	bool success = obs_websocket_vendor_run_simple_proc(vendor, "vendor_request_unregister", &cd);
	calldata_free(&cd);

	return success;
}

// Does not affect event_data refcount.
// Emits an event under the vendor's name
static inline bool obs_websocket_vendor_emit_event(obs_websocket_vendor vendor, const char *event_name, obs_data_t *event_data)
{
	calldata_t cd = {0, 0, 0, 0};
	calldata_set_string(&cd, "type", event_name);
	calldata_set_ptr(&cd, "data", (void *)event_data);

	bool success = obs_websocket_vendor_run_simple_proc(vendor, "vendor_event_emit", &cd);
	calldata_free(&cd);

	return success;
}

/* ==================== END API FUNCTIONS ==================== */

#ifdef __cplusplus
}
#endif

#endif
