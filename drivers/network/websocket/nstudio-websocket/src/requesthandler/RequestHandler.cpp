/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
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

#ifdef PLUGIN_TESTS
#include <util/profiler.hpp>
#endif

#include "RequestHandler.h"

const std::unordered_map<std::string, RequestMethodHandler> RequestHandler::_handlerMap{
	// General
	{"GetVersion", &RequestHandler::GetVersion},
	{"GetStats", &RequestHandler::GetStats},
	{"BroadcastCustomEvent", &RequestHandler::BroadcastCustomEvent},
	{"CallVendorRequest", &RequestHandler::CallVendorRequest},
	{"GetHotkeyList", &RequestHandler::GetHotkeyList},
	{"TriggerHotkeyByName", &RequestHandler::TriggerHotkeyByName},
	{"TriggerHotkeyByKeySequence", &RequestHandler::TriggerHotkeyByKeySequence},
	{"Sleep", &RequestHandler::Sleep},

	// Config
	{"GetPersistentData", &RequestHandler::GetPersistentData},
	{"SetPersistentData", &RequestHandler::SetPersistentData},
	{"GetSceneCollectionList", &RequestHandler::GetSceneCollectionList},
	{"SetCurrentSceneCollection", &RequestHandler::SetCurrentSceneCollection},
	{"CreateSceneCollection", &RequestHandler::CreateSceneCollection},
	{"GetProfileList", &RequestHandler::GetProfileList},
	{"SetCurrentProfile", &RequestHandler::SetCurrentProfile},
	{"CreateProfile", &RequestHandler::CreateProfile},
	{"RemoveProfile", &RequestHandler::RemoveProfile},
	{"GetProfileParameter", &RequestHandler::GetProfileParameter},
	{"SetProfileParameter", &RequestHandler::SetProfileParameter},
	{"GetVideoSettings", &RequestHandler::GetVideoSettings},
	{"SetVideoSettings", &RequestHandler::SetVideoSettings},
	{"GetStreamServiceSettings", &RequestHandler::GetStreamServiceSettings},
	{"SetStreamServiceSettings", &RequestHandler::SetStreamServiceSettings},
	{"GetRecordDirectory", &RequestHandler::GetRecordDirectory},
	{"SetRecordDirectory", &RequestHandler::SetRecordDirectory},

	// Sources
	{"GetSourceActive", &RequestHandler::GetSourceActive},
	{"GetSourceScreenshot", &RequestHandler::GetSourceScreenshot},
	{"SaveSourceScreenshot", &RequestHandler::SaveSourceScreenshot},
	{"GetSourcePrivateSettings", &RequestHandler::GetSourcePrivateSettings},
	{"SetSourcePrivateSettings", &RequestHandler::SetSourcePrivateSettings},

	// Scenes
	{"GetSceneList", &RequestHandler::GetSceneList},
	{"GetGroupList", &RequestHandler::GetGroupList},
	{"GetCurrentProgramScene", &RequestHandler::GetCurrentProgramScene},
	{"SetCurrentProgramScene", &RequestHandler::SetCurrentProgramScene},
	{"GetCurrentPreviewScene", &RequestHandler::GetCurrentPreviewScene},
	{"SetCurrentPreviewScene", &RequestHandler::SetCurrentPreviewScene},
	{"CreateScene", &RequestHandler::CreateScene},
	{"RemoveScene", &RequestHandler::RemoveScene},
	{"SetSceneName", &RequestHandler::SetSceneName},
	{"GetSceneSceneTransitionOverride", &RequestHandler::GetSceneSceneTransitionOverride},
	{"SetSceneSceneTransitionOverride", &RequestHandler::SetSceneSceneTransitionOverride},

	// Inputs
	{"GetInputList", &RequestHandler::GetInputList},
	{"GetInputKindList", &RequestHandler::GetInputKindList},
	{"GetSpecialInputs", &RequestHandler::GetSpecialInputs},
	{"CreateInput", &RequestHandler::CreateInput},
	{"RemoveInput", &RequestHandler::RemoveInput},
	{"SetInputName", &RequestHandler::SetInputName},
	{"GetInputDefaultSettings", &RequestHandler::GetInputDefaultSettings},
	{"GetInputSettings", &RequestHandler::GetInputSettings},
	{"SetInputSettings", &RequestHandler::SetInputSettings},
	{"GetInputMute", &RequestHandler::GetInputMute},
	{"SetInputMute", &RequestHandler::SetInputMute},
	{"ToggleInputMute", &RequestHandler::ToggleInputMute},
	{"GetInputVolume", &RequestHandler::GetInputVolume},
	{"SetInputVolume", &RequestHandler::SetInputVolume},
	{"GetInputAudioBalance", &RequestHandler::GetInputAudioBalance},
	{"SetInputAudioBalance", &RequestHandler::SetInputAudioBalance},
	{"GetInputAudioSyncOffset", &RequestHandler::GetInputAudioSyncOffset},
	{"SetInputAudioSyncOffset", &RequestHandler::SetInputAudioSyncOffset},
	{"GetInputAudioMonitorType", &RequestHandler::GetInputAudioMonitorType},
	{"SetInputAudioMonitorType", &RequestHandler::SetInputAudioMonitorType},
	{"GetInputAudioTracks", &RequestHandler::GetInputAudioTracks},
	{"SetInputAudioTracks", &RequestHandler::SetInputAudioTracks},
	{"GetInputDeinterlaceMode", &RequestHandler::GetInputDeinterlaceMode},
	{"SetInputDeinterlaceMode", &RequestHandler::SetInputDeinterlaceMode},
	{"GetInputDeinterlaceFieldOrder", &RequestHandler::GetInputDeinterlaceFieldOrder},
	{"SetInputDeinterlaceFieldOrder", &RequestHandler::SetInputDeinterlaceFieldOrder},
	{"GetInputPropertiesListPropertyItems", &RequestHandler::GetInputPropertiesListPropertyItems},
	{"PressInputPropertiesButton", &RequestHandler::PressInputPropertiesButton},

	// Transitions
	{"GetTransitionKindList", &RequestHandler::GetTransitionKindList},
	{"GetSceneTransitionList", &RequestHandler::GetSceneTransitionList},
	{"GetCurrentSceneTransition", &RequestHandler::GetCurrentSceneTransition},
	{"SetCurrentSceneTransition", &RequestHandler::SetCurrentSceneTransition},
	{"SetCurrentSceneTransitionDuration", &RequestHandler::SetCurrentSceneTransitionDuration},
	{"SetCurrentSceneTransitionSettings", &RequestHandler::SetCurrentSceneTransitionSettings},
	{"GetCurrentSceneTransitionCursor", &RequestHandler::GetCurrentSceneTransitionCursor},
	{"TriggerStudioModeTransition", &RequestHandler::TriggerStudioModeTransition},
	{"SetTBarPosition", &RequestHandler::SetTBarPosition},

	// Filters
	{"GetSourceFilterKindList", &RequestHandler::GetSourceFilterKindList},
	{"GetSourceFilterList", &RequestHandler::GetSourceFilterList},
	{"GetSourceFilterDefaultSettings", &RequestHandler::GetSourceFilterDefaultSettings},
	{"CreateSourceFilter", &RequestHandler::CreateSourceFilter},
	{"RemoveSourceFilter", &RequestHandler::RemoveSourceFilter},
	{"SetSourceFilterName", &RequestHandler::SetSourceFilterName},
	{"GetSourceFilter", &RequestHandler::GetSourceFilter},
	{"SetSourceFilterIndex", &RequestHandler::SetSourceFilterIndex},
	{"SetSourceFilterSettings", &RequestHandler::SetSourceFilterSettings},
	{"SetSourceFilterEnabled", &RequestHandler::SetSourceFilterEnabled},

	// Scene Items
	{"GetSceneItemList", &RequestHandler::GetSceneItemList},
	{"GetGroupSceneItemList", &RequestHandler::GetGroupSceneItemList},
	{"GetSceneItemId", &RequestHandler::GetSceneItemId},
	{"GetSceneItemSource", &RequestHandler::GetSceneItemSource},
	{"CreateSceneItem", &RequestHandler::CreateSceneItem},
	{"RemoveSceneItem", &RequestHandler::RemoveSceneItem},
	{"DuplicateSceneItem", &RequestHandler::DuplicateSceneItem},
	{"GetSceneItemTransform", &RequestHandler::GetSceneItemTransform},
	{"SetSceneItemTransform", &RequestHandler::SetSceneItemTransform},
	{"GetSceneItemEnabled", &RequestHandler::GetSceneItemEnabled},
	{"SetSceneItemEnabled", &RequestHandler::SetSceneItemEnabled},
	{"GetSceneItemLocked", &RequestHandler::GetSceneItemLocked},
	{"SetSceneItemLocked", &RequestHandler::SetSceneItemLocked},
	{"GetSceneItemIndex", &RequestHandler::GetSceneItemIndex},
	{"SetSceneItemIndex", &RequestHandler::SetSceneItemIndex},
	{"GetSceneItemBlendMode", &RequestHandler::GetSceneItemBlendMode},
	{"SetSceneItemBlendMode", &RequestHandler::SetSceneItemBlendMode},
	{"GetSceneItemPrivateSettings", &RequestHandler::GetSceneItemPrivateSettings},
	{"SetSceneItemPrivateSettings", &RequestHandler::SetSceneItemPrivateSettings},

	// Outputs
	{"GetVirtualCamStatus", &RequestHandler::GetVirtualCamStatus},
	{"ToggleVirtualCam", &RequestHandler::ToggleVirtualCam},
	{"StartVirtualCam", &RequestHandler::StartVirtualCam},
	{"StopVirtualCam", &RequestHandler::StopVirtualCam},
	{"GetReplayBufferStatus", &RequestHandler::GetReplayBufferStatus},
	{"ToggleReplayBuffer", &RequestHandler::ToggleReplayBuffer},
	{"StartReplayBuffer", &RequestHandler::StartReplayBuffer},
	{"StopReplayBuffer", &RequestHandler::StopReplayBuffer},
	{"SaveReplayBuffer", &RequestHandler::SaveReplayBuffer},
	{"GetLastReplayBufferReplay", &RequestHandler::GetLastReplayBufferReplay},
	{"GetOutputList", &RequestHandler::GetOutputList},
	{"GetOutputStatus", &RequestHandler::GetOutputStatus},
	{"ToggleOutput", &RequestHandler::ToggleOutput},
	{"StartOutput", &RequestHandler::StartOutput},
	{"StopOutput", &RequestHandler::StopOutput},
	{"GetOutputSettings", &RequestHandler::GetOutputSettings},
	{"SetOutputSettings", &RequestHandler::SetOutputSettings},

	// Stream
	{"GetStreamStatus", &RequestHandler::GetStreamStatus},
	{"ToggleStream", &RequestHandler::ToggleStream},
	{"StartStream", &RequestHandler::StartStream},
	{"StopStream", &RequestHandler::StopStream},
	{"SendStreamCaption", &RequestHandler::SendStreamCaption},

	// Record
	{"GetRecordStatus", &RequestHandler::GetRecordStatus},
	{"ToggleRecord", &RequestHandler::ToggleRecord},
	{"StartRecord", &RequestHandler::StartRecord},
	{"StopRecord", &RequestHandler::StopRecord},
	{"ToggleRecordPause", &RequestHandler::ToggleRecordPause},
	{"PauseRecord", &RequestHandler::PauseRecord},
	{"ResumeRecord", &RequestHandler::ResumeRecord},
	{"SplitRecordFile", &RequestHandler::SplitRecordFile},
	{"CreateRecordChapter", &RequestHandler::CreateRecordChapter},

	// Media Inputs
	{"GetMediaInputStatus", &RequestHandler::GetMediaInputStatus},
	{"SetMediaInputCursor", &RequestHandler::SetMediaInputCursor},
	{"OffsetMediaInputCursor", &RequestHandler::OffsetMediaInputCursor},
	{"TriggerMediaInputAction", &RequestHandler::TriggerMediaInputAction},

	// Ui
	{"GetStudioModeEnabled", &RequestHandler::GetStudioModeEnabled},
	{"SetStudioModeEnabled", &RequestHandler::SetStudioModeEnabled},
	{"OpenInputPropertiesDialog", &RequestHandler::OpenInputPropertiesDialog},
	{"OpenInputFiltersDialog", &RequestHandler::OpenInputFiltersDialog},
	{"OpenInputInteractDialog", &RequestHandler::OpenInputInteractDialog},
	{"GetMonitorList", &RequestHandler::GetMonitorList},
	{"OpenVideoMixProjector", &RequestHandler::OpenVideoMixProjector},
	{"OpenSourceProjector", &RequestHandler::OpenSourceProjector},
};

RequestHandler::RequestHandler(SessionPtr session) : _session(session) {}

RequestResult RequestHandler::ProcessRequest(const Request &request)
{
#ifdef PLUGIN_TESTS
	ScopeProfiler prof{"obs_websocket_request_processing"};
#endif

	if (!request.RequestData.is_object() && !request.RequestData.is_null())
		return RequestResult::Error(RequestStatus::InvalidRequestFieldType, "Your request data is not an object.");

	if (request.RequestType.empty())
		return RequestResult::Error(RequestStatus::MissingRequestType, "Your request's `requestType` may not be empty.");

	RequestMethodHandler handler;
	try {
		handler = _handlerMap.at(request.RequestType);
	} catch (const std::out_of_range &oor) {
		UNUSED_PARAMETER(oor);
		return RequestResult::Error(RequestStatus::UnknownRequestType, "Your request type is not valid.");
	}

	return std::bind(handler, this, std::placeholders::_1)(request);
}

std::vector<std::string> RequestHandler::GetRequestList()
{
	std::vector<std::string> ret;
	for (auto const &[key, val] : _handlerMap)
		ret.push_back(key);

	return ret;
}
