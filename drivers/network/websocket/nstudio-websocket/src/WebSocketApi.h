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

#pragma once

#include <functional>
#include <string>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <obs.h>
#include <obs-websocket-api.h>

#include "utils/Json.h"
#include "plugin-macros.generated.h"

class WebSocketApi {
public:
	enum RequestReturnCode {
		Normal,
		NoVendor,
		NoVendorRequest,
	};

	struct Vendor {
		std::shared_mutex _mutex;
		std::string _name;
		std::map<std::string, obs_websocket_request_callback> _requests;
	};

	WebSocketApi();
	~WebSocketApi();
	void BroadcastEvent(uint64_t requiredIntent, const std::string &eventType, const json &eventData = nullptr,
			    uint8_t rpcVersion = 0);
	void SetObsReady(bool ready) { _obsReady = ready; }
	enum RequestReturnCode PerformVendorRequest(std::string vendorName, std::string requestName, obs_data_t *requestData,
						    obs_data_t *responseData);

	// Callback for when a vendor emits an event
	typedef std::function<void(std::string, std::string, obs_data_t *)> VendorEventCallback;
	inline void SetVendorEventCallback(VendorEventCallback cb) { _vendorEventCallback = cb; }

private:
	inline int64_t GetEventCallbackIndex(obs_websocket_event_callback &cb)
	{
		for (int64_t i = 0; i < (int64_t)_eventCallbacks.size(); i++) {
			auto currentCb = _eventCallbacks[i];
			if (currentCb.callback == cb.callback && currentCb.priv_data == cb.priv_data)
				return i;
		}
		return -1;
	}

	// Proc handlers
	static void get_ph_cb(void *priv_data, calldata_t *cd);
	static void get_api_version(void *, calldata_t *cd);
	static void call_request(void *, calldata_t *cd);
	static void register_event_callback(void *, calldata_t *cd);
	static void unregister_event_callback(void *, calldata_t *cd);
	static void vendor_register_cb(void *priv_data, calldata_t *cd);
	static void vendor_request_register_cb(void *priv_data, calldata_t *cd);
	static void vendor_request_unregister_cb(void *priv_data, calldata_t *cd);
	static void vendor_event_emit_cb(void *priv_data, calldata_t *cd);

	std::shared_mutex _mutex;
	proc_handler_t *_procHandler;
	std::map<std::string, Vendor *> _vendors;
	std::vector<obs_websocket_event_callback> _eventCallbacks;

	std::atomic<bool> _obsReady = false;

	VendorEventCallback _vendorEventCallback;
};
