// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "broadcast-flow.hpp"

void OBSBroadcastFlow::ManageBroadcast(bool streamingActive)
{
	if (flow.manage_broadcast2) {
		flow.manage_broadcast2(flow.priv, streamingActive);
	} else if (!streamingActive) {
		flow.manage_broadcast(flow.priv);
	}

	if (streamingActive)
		return;

	broadcastState = flow.get_broadcast_state(flow.priv);
	broadcastStartType = flow.get_broadcast_start_type(flow.priv);
	broadcastStopType = flow.get_broadcast_stop_type(flow.priv);
}

bool OBSBroadcastFlow::AllowManagingWhileStreaming()
{
	return (flow.flags & OBS_BROADCAST_FLOW_ALLOW_MANAGE_WHILE_STREAMING) !=
	       0;
}

obs_broadcast_state OBSBroadcastFlow::BroadcastState()
{
	if ((flow.flags & OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START) ==
		    0 &&
	    broadcastState == OBS_BROADCAST_INACTIVE) {
		return OBS_BROADCAST_NONE;
	}

	return broadcastState;
}

obs_broadcast_start OBSBroadcastFlow::BroadcastStartType()
{
	if ((flow.flags & OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START) ==
		    0 &&
	    broadcastStartType == OBS_BROADCAST_START_DIFFER_FROM_STREAM) {
		return OBS_BROADCAST_START_WITH_STREAM;
	}

	return broadcastStartType;
}

obs_broadcast_stop OBSBroadcastFlow::BroadcastStopType()
{
	if ((flow.flags & OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_STOP) ==
		    0 &&
	    broadcastStopType == OBS_BROADCAST_STOP_DIFFER_FROM_STREAM) {
		return OBS_BROADCAST_STOP_NEVER;
	}

	return broadcastStopType;
}
bool OBSBroadcastFlow::DifferedStartBroadcast()
{
	if (BroadcastStartType() != OBS_BROADCAST_START_DIFFER_FROM_STREAM) {
		return false;
	}

	flow.differed_start_broadcast(flow.priv);
	broadcastState = flow.get_broadcast_state(flow.priv);

	/* If not active after start, failure */
	return broadcastState == OBS_BROADCAST_ACTIVE;
}

obs_broadcast_stream_state OBSBroadcastFlow::IsBroadcastStreamActive()
{
	if (BroadcastStartType() != OBS_BROADCAST_START_DIFFER_FROM_STREAM) {
		return OBS_BROADCAST_STREAM_FAILURE;
	}

	return flow.is_broadcast_stream_active(flow.priv);
}

bool OBSBroadcastFlow::DifferedStopBroadcast()
{
	if (BroadcastStopType() != OBS_BROADCAST_STOP_DIFFER_FROM_STREAM)
		return false;

	bool ret = flow.differed_stop_broadcast(flow.priv);

	broadcastState = flow.get_broadcast_state(flow.priv);
	broadcastStartType = flow.get_broadcast_start_type(flow.priv);
	broadcastStopType = flow.get_broadcast_stop_type(flow.priv);

	return ret;
}

void OBSBroadcastFlow::StopStreaming()
{
	flow.stopped_streaming(flow.priv);

	broadcastState = flow.get_broadcast_state(flow.priv);
	broadcastStartType = flow.get_broadcast_start_type(flow.priv);
	broadcastStopType = flow.get_broadcast_stop_type(flow.priv);
}

std::string OBSBroadcastFlow::GetLastError()
{
	if (!flow.get_last_error)
		return "";

	const char *error = flow.get_last_error(flow.priv);
	if (!error)
		return "";

	return error;
}
