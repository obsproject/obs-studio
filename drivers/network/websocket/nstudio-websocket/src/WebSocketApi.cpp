/*
obs-websocket
Copyright (C) 2020-2023 Kyle Manning <tt2468@gmail.com>

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

#include "WebSocketApi.h"
#include "requesthandler/RequestHandler.h"
#include "utils/Json.h"

#define RETURN_STATUS(status)                             \
	{                                                 \
		calldata_set_bool(cd, "success", status); \
		return;                                   \
	}
#define RETURN_SUCCESS() RETURN_STATUS(true);
#define RETURN_FAILURE() RETURN_STATUS(false);

WebSocketApi::Vendor *get_vendor(calldata_t *cd)
{
	void *voidVendor;
	if (!calldata_get_ptr(cd, "vendor", &voidVendor)) {
		blog(LOG_WARNING, "[WebSocketApi: get_vendor] Failed due to missing `vendor` pointer.");
		return nullptr;
	}

	return static_cast<WebSocketApi::Vendor *>(voidVendor);
}

WebSocketApi::WebSocketApi()
{
	blog_debug("[WebSocketApi::WebSocketApi] Setting up...");

	_procHandler = proc_handler_create();

	proc_handler_add(_procHandler, "bool get_api_version(out int version)", &get_api_version, nullptr);
	proc_handler_add(_procHandler, "bool call_request(in string request_type, in string request_data, out ptr response)",
			 &call_request, this);
	proc_handler_add(_procHandler, "bool register_event_callback(in ptr callback, out bool success)", &register_event_callback,
			 this);
	proc_handler_add(_procHandler, "bool unregister_event_callback(in ptr callback, out bool success)",
			 &unregister_event_callback, this);
	proc_handler_add(_procHandler, "bool vendor_register(in string name, out ptr vendor)", &vendor_register_cb, this);
	proc_handler_add(_procHandler,
			 "bool vendor_request_register(in ptr vendor, in string type, in ptr callback, out bool success)",
			 &vendor_request_register_cb, this);
	proc_handler_add(_procHandler, "bool vendor_request_unregister(in ptr vendor, in string type, out bool success)",
			 &vendor_request_unregister_cb, this);
	proc_handler_add(_procHandler, "bool vendor_event_emit(in ptr vendor, in string type, in ptr data, out bool success)",
			 &vendor_event_emit_cb, this);

	proc_handler_t *ph = obs_get_proc_handler();
	assert(ph != NULL);

	proc_handler_add(ph, "bool obs_websocket_api_get_ph(out ptr ph)", &get_ph_cb, this);

	blog_debug("[WebSocketApi::WebSocketApi] Finished.");
}

WebSocketApi::~WebSocketApi()
{
	blog_debug("[WebSocketApi::~WebSocketApi] Shutting down...");

	proc_handler_destroy(_procHandler);

	size_t numEventCallbacks = _eventCallbacks.size();
	_eventCallbacks.clear();
	blog_debug("[WebSocketApi::~WebSocketApi] Deleted %ld event callbacks", numEventCallbacks);

	for (auto vendor : _vendors) {
		blog_debug("[WebSocketApi::~WebSocketApi] Deleting vendor: %s", vendor.first.c_str());
		delete vendor.second;
	}

	blog_debug("[WebSocketApi::~WebSocketApi] Finished.");
}

void WebSocketApi::BroadcastEvent(uint64_t requiredIntent, const std::string &eventType, const json &eventData, uint8_t rpcVersion)
{
	if (!_obsReady)
		return;

	// Only broadcast events applicable to the latest RPC version
	if (rpcVersion && rpcVersion != CURRENT_RPC_VERSION)
		return;

	std::string eventDataString = eventData.dump();

	std::shared_lock l(_mutex);

	for (auto &cb : _eventCallbacks)
		cb.callback(requiredIntent, eventType.c_str(), eventDataString.c_str(), cb.priv_data);
}

enum WebSocketApi::RequestReturnCode WebSocketApi::PerformVendorRequest(std::string vendorName, std::string requestType,
									obs_data_t *requestData, obs_data_t *responseData)
{
	std::shared_lock l(_mutex);

	if (_vendors.count(vendorName) == 0)
		return RequestReturnCode::NoVendor;

	auto v = _vendors[vendorName];

	l.unlock();

	std::shared_lock v_l(v->_mutex);

	if (v->_requests.count(requestType) == 0)
		return RequestReturnCode::NoVendorRequest;

	auto cb = v->_requests[requestType];

	v_l.unlock();

	cb.callback(requestData, responseData, cb.priv_data);

	return RequestReturnCode::Normal;
}

void WebSocketApi::get_ph_cb(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<WebSocketApi *>(priv_data);

	calldata_set_ptr(cd, "ph", (void *)c->_procHandler);

	RETURN_SUCCESS();
}

void WebSocketApi::get_api_version(void *, calldata_t *cd)
{
	calldata_set_int(cd, "version", OBS_WEBSOCKET_API_VERSION);

	RETURN_SUCCESS();
}

void WebSocketApi::call_request(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<WebSocketApi *>(priv_data);

#if !defined(PLUGIN_TESTS)
	if (!c->_obsReady)
		RETURN_FAILURE();
#endif

	const char *request_type = calldata_string(cd, "request_type");
	const char *request_data = calldata_string(cd, "request_data");

	if (!request_type)
		RETURN_FAILURE();

#ifdef PLUGIN_TESTS
	// Allow plugin tests to complete, even though OBS wouldn't be ready at the time of the test
	if (!c->_obsReady && std::string(request_type) != "GetVersion")
		RETURN_FAILURE();
#endif

	auto response = static_cast<obs_websocket_request_response *>(bzalloc(sizeof(struct obs_websocket_request_response)));
	if (!response)
		RETURN_FAILURE();

	json requestData;
	if (request_data)
		requestData = json::parse(request_data);

	RequestHandler requestHandler;
	Request request(request_type, requestData);
	RequestResult requestResult = requestHandler.ProcessRequest(request);

	response->status_code = (unsigned int)requestResult.StatusCode;
	if (!requestResult.Comment.empty())
		response->comment = bstrdup(requestResult.Comment.c_str());
	if (requestResult.ResponseData.is_object()) {
		std::string responseData = requestResult.ResponseData.dump();
		response->response_data = bstrdup(responseData.c_str());
	}

	calldata_set_ptr(cd, "response", response);

	blog_debug("[WebSocketApi::call_request] Request %s called, response status code is %u", request_type,
		   response->status_code);

	RETURN_SUCCESS();
}

void WebSocketApi::register_event_callback(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<WebSocketApi *>(priv_data);

	void *voidCallback;
	if (!calldata_get_ptr(cd, "callback", &voidCallback) || !voidCallback) {
		blog(LOG_WARNING, "[WebSocketApi::register_event_callback] Failed due to missing `callback` pointer.");
		RETURN_FAILURE();
	}

	auto cb = static_cast<obs_websocket_event_callback *>(voidCallback);

	std::unique_lock l(c->_mutex);

	int64_t foundIndex = c->GetEventCallbackIndex(*cb);
	if (foundIndex != -1)
		RETURN_FAILURE();

	c->_eventCallbacks.push_back(*cb);

	RETURN_SUCCESS();
}

void WebSocketApi::unregister_event_callback(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<WebSocketApi *>(priv_data);

	void *voidCallback;
	if (!calldata_get_ptr(cd, "callback", &voidCallback) || !voidCallback) {
		blog(LOG_WARNING, "[WebSocketApi::register_event_callback] Failed due to missing `callback` pointer.");
		RETURN_FAILURE();
	}

	auto cb = static_cast<obs_websocket_event_callback *>(voidCallback);

	std::unique_lock l(c->_mutex);

	int64_t foundIndex = c->GetEventCallbackIndex(*cb);
	if (foundIndex == -1)
		RETURN_FAILURE();

	c->_eventCallbacks.erase(c->_eventCallbacks.begin() + foundIndex);

	RETURN_SUCCESS();
}

void WebSocketApi::vendor_register_cb(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<WebSocketApi *>(priv_data);

	const char *vendorName;
	if (!calldata_get_string(cd, "name", &vendorName) || strlen(vendorName) == 0) {
		blog(LOG_WARNING, "[WebSocketApi::vendor_register_cb] Failed due to missing `name` string.");
		RETURN_FAILURE();
	}

	// Theoretically doesn't need a mutex due to module load being single-thread, but it's good to be safe.
	std::unique_lock l(c->_mutex);

	if (c->_vendors.count(vendorName)) {
		blog(LOG_WARNING, "[WebSocketApi::vendor_register_cb] Failed because `%s` is already a registered vendor.",
		     vendorName);
		RETURN_FAILURE();
	}

	Vendor *v = new Vendor();
	v->_name = vendorName;

	c->_vendors[vendorName] = v;

	blog_debug("[WebSocketApi::vendor_register_cb] [vendorName: %s] Registered new vendor.", v->_name.c_str());

	calldata_set_ptr(cd, "vendor", static_cast<void *>(v));

	RETURN_SUCCESS();
}

void WebSocketApi::vendor_request_register_cb(void *, calldata_t *cd)
{
	Vendor *v = get_vendor(cd);
	if (!v)
		RETURN_FAILURE();

	const char *requestType;
	if (!calldata_get_string(cd, "type", &requestType) || strlen(requestType) == 0) {
		blog(LOG_WARNING,
		     "[WebSocketApi::vendor_request_register_cb] [vendorName: %s] Failed due to missing or empty `type` string.",
		     v->_name.c_str());
		RETURN_FAILURE();
	}

	void *voidCallback;
	if (!calldata_get_ptr(cd, "callback", &voidCallback) || !voidCallback) {
		blog(LOG_WARNING,
		     "[WebSocketApi::vendor_request_register_cb] [vendorName: %s] Failed due to missing `callback` pointer.",
		     v->_name.c_str());
		RETURN_FAILURE();
	}

	auto cb = static_cast<obs_websocket_request_callback *>(voidCallback);

	std::unique_lock l(v->_mutex);

	if (v->_requests.count(requestType)) {
		blog(LOG_WARNING,
		     "[WebSocketApi::vendor_request_register_cb] [vendorName: %s] Failed because `%s` is already a registered request.",
		     v->_name.c_str(), requestType);
		RETURN_FAILURE();
	}

	v->_requests[requestType] = *cb;

	blog_debug("[WebSocketApi::vendor_request_register_cb] [vendorName: %s] Registered new vendor request: %s",
		   v->_name.c_str(), requestType);

	RETURN_SUCCESS();
}

void WebSocketApi::vendor_request_unregister_cb(void *, calldata_t *cd)
{
	Vendor *v = get_vendor(cd);
	if (!v)
		RETURN_FAILURE();

	const char *requestType;
	if (!calldata_get_string(cd, "type", &requestType) || strlen(requestType) == 0) {
		blog(LOG_WARNING,
		     "[WebSocketApi::vendor_request_unregister_cb] [vendorName: %s] Failed due to missing `type` string.",
		     v->_name.c_str());
		RETURN_FAILURE();
	}

	std::unique_lock l(v->_mutex);

	if (!v->_requests.count(requestType)) {
		blog(LOG_WARNING,
		     "[WebSocketApi::vendor_request_register_cb] [vendorName: %s] Failed because `%s` is not a registered request.",
		     v->_name.c_str(), requestType);
		RETURN_FAILURE();
	}

	v->_requests.erase(requestType);

	blog_debug("[WebSocketApi::vendor_request_unregister_cb] [vendorName: %s] Unregistered vendor request: %s",
		   v->_name.c_str(), requestType);

	RETURN_SUCCESS();
}

void WebSocketApi::vendor_event_emit_cb(void *priv_data, calldata_t *cd)
{
	auto c = static_cast<WebSocketApi *>(priv_data);

	Vendor *v = get_vendor(cd);
	if (!v)
		RETURN_FAILURE();

	const char *eventType;
	if (!calldata_get_string(cd, "type", &eventType) || strlen(eventType) == 0) {
		blog(LOG_WARNING, "[WebSocketApi::vendor_event_emit_cb] [vendorName: %s] Failed due to missing `type` string.",
		     v->_name.c_str());
		RETURN_FAILURE();
	}

	void *voidEventData;
	if (!calldata_get_ptr(cd, "data", &voidEventData)) {
		blog(LOG_WARNING, "[WebSocketApi::vendor_event_emit_cb] [vendorName: %s] Failed due to missing `data` pointer.",
		     v->_name.c_str());
		RETURN_FAILURE();
	}

	auto eventData = static_cast<obs_data_t *>(voidEventData);

	if (!c->_vendorEventCallback)
		RETURN_FAILURE();

	c->_vendorEventCallback(v->_name, eventType, eventData);

	RETURN_SUCCESS();
}
