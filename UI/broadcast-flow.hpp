// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <obs-frontend-api.h>

class OBSBroadcastFlow {
	const obs_service_t *boundedService = nullptr;

	obs_frontend_broadcast_flow flow = {0};

	obs_broadcast_state broadcastState;
	obs_broadcast_start broadcastStartType;
	obs_broadcast_stop broadcastStopType;

public:
	inline OBSBroadcastFlow(const obs_service_t *service,
				const obs_frontend_broadcast_flow &flow)
		: boundedService(service),
		  flow(flow),
		  broadcastState(flow.get_broadcast_state(flow.priv)),
		  broadcastStartType(flow.get_broadcast_start_type(flow.priv)),
		  broadcastStopType(flow.get_broadcast_stop_type(flow.priv)){};
	inline ~OBSBroadcastFlow() {}

	const obs_service_t *GetBondedService() { return boundedService; }

	void ManageBroadcast(bool streamingActive);

	bool AllowManagingWhileStreaming();

	obs_broadcast_state BroadcastState();
	obs_broadcast_start BroadcastStartType();
	obs_broadcast_stop BroadcastStopType();

	bool DifferedStartBroadcast();
	obs_broadcast_stream_state IsBroadcastStreamActive();
	bool DifferedStopBroadcast();

	void StopStreaming();

	std::string GetLastError();
};
