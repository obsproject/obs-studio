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

#pragma once

#include <unordered_map>
#include <obs.hpp>
#include <obs-frontend-api.h>

#include "rpc/Request.h"
#include "rpc/RequestResult.h"
#include "types/RequestStatus.h"
#include "types/RequestBatchExecutionType.h"
#include "../websocketserver/rpc/WebSocketSession.h"
#include "../obs-websocket.h"
#include "../utils/Obs.h"
#include "plugin-macros.generated.h"

class RequestHandler;
typedef RequestResult (RequestHandler::*RequestMethodHandler)(const Request &);

class RequestHandler {
public:
	RequestHandler(SessionPtr session = nullptr);

	RequestResult ProcessRequest(const Request &request);
	std::vector<std::string> GetRequestList();

private:
	// General
	RequestResult GetVersion(const Request &);
	RequestResult GetStats(const Request &);
	RequestResult BroadcastCustomEvent(const Request &);
	RequestResult CallVendorRequest(const Request &);
	RequestResult GetHotkeyList(const Request &);
	RequestResult TriggerHotkeyByName(const Request &);
	RequestResult TriggerHotkeyByKeySequence(const Request &);
	RequestResult Sleep(const Request &);

	// Config
	RequestResult GetPersistentData(const Request &);
	RequestResult SetPersistentData(const Request &);
	RequestResult GetSceneCollectionList(const Request &);
	RequestResult SetCurrentSceneCollection(const Request &);
	RequestResult CreateSceneCollection(const Request &);
	RequestResult GetProfileList(const Request &);
	RequestResult SetCurrentProfile(const Request &);
	RequestResult CreateProfile(const Request &);
	RequestResult RemoveProfile(const Request &);
	RequestResult GetProfileParameter(const Request &);
	RequestResult SetProfileParameter(const Request &);
	RequestResult GetVideoSettings(const Request &);
	RequestResult SetVideoSettings(const Request &);
	RequestResult GetStreamServiceSettings(const Request &);
	RequestResult SetStreamServiceSettings(const Request &);
	RequestResult GetRecordDirectory(const Request &);
	RequestResult SetRecordDirectory(const Request &);

	// Sources
	RequestResult GetSourceActive(const Request &);
	RequestResult GetSourceScreenshot(const Request &);
	RequestResult SaveSourceScreenshot(const Request &);
	RequestResult GetSourcePrivateSettings(const Request &);
	RequestResult SetSourcePrivateSettings(const Request &);

	// Scenes
	RequestResult GetSceneList(const Request &);
	RequestResult GetGroupList(const Request &);
	RequestResult GetCurrentProgramScene(const Request &);
	RequestResult SetCurrentProgramScene(const Request &);
	RequestResult GetCurrentPreviewScene(const Request &);
	RequestResult SetCurrentPreviewScene(const Request &);
	RequestResult CreateScene(const Request &);
	RequestResult RemoveScene(const Request &);
	RequestResult SetSceneName(const Request &);
	RequestResult GetSceneSceneTransitionOverride(const Request &);
	RequestResult SetSceneSceneTransitionOverride(const Request &);

	// Inputs
	RequestResult GetInputList(const Request &);
	RequestResult GetInputKindList(const Request &);
	RequestResult GetSpecialInputs(const Request &);
	RequestResult CreateInput(const Request &);
	RequestResult RemoveInput(const Request &);
	RequestResult SetInputName(const Request &);
	RequestResult GetInputDefaultSettings(const Request &);
	RequestResult GetInputSettings(const Request &);
	RequestResult SetInputSettings(const Request &);
	RequestResult GetInputMute(const Request &);
	RequestResult SetInputMute(const Request &);
	RequestResult ToggleInputMute(const Request &);
	RequestResult GetInputVolume(const Request &);
	RequestResult SetInputVolume(const Request &);
	RequestResult GetInputAudioBalance(const Request &);
	RequestResult SetInputAudioBalance(const Request &);
	RequestResult GetInputAudioSyncOffset(const Request &);
	RequestResult SetInputAudioSyncOffset(const Request &);
	RequestResult GetInputAudioMonitorType(const Request &);
	RequestResult SetInputAudioMonitorType(const Request &);
	RequestResult GetInputAudioTracks(const Request &);
	RequestResult SetInputAudioTracks(const Request &);
	RequestResult GetInputDeinterlaceMode(const Request &);
	RequestResult SetInputDeinterlaceMode(const Request &);
	RequestResult GetInputDeinterlaceFieldOrder(const Request &);
	RequestResult SetInputDeinterlaceFieldOrder(const Request &);
	RequestResult GetInputPropertiesListPropertyItems(const Request &);
	RequestResult PressInputPropertiesButton(const Request &);

	// Transitions
	RequestResult GetTransitionKindList(const Request &);
	RequestResult GetSceneTransitionList(const Request &);
	RequestResult GetCurrentSceneTransition(const Request &);
	RequestResult SetCurrentSceneTransition(const Request &);
	RequestResult SetCurrentSceneTransitionDuration(const Request &);
	RequestResult SetCurrentSceneTransitionSettings(const Request &);
	RequestResult GetCurrentSceneTransitionCursor(const Request &);
	RequestResult TriggerStudioModeTransition(const Request &);
	RequestResult SetTBarPosition(const Request &);

	// Filters
	RequestResult GetSourceFilterKindList(const Request &);
	RequestResult GetSourceFilterList(const Request &);
	RequestResult GetSourceFilterDefaultSettings(const Request &);
	RequestResult CreateSourceFilter(const Request &);
	RequestResult RemoveSourceFilter(const Request &);
	RequestResult SetSourceFilterName(const Request &);
	RequestResult GetSourceFilter(const Request &);
	RequestResult SetSourceFilterIndex(const Request &);
	RequestResult SetSourceFilterSettings(const Request &);
	RequestResult SetSourceFilterEnabled(const Request &);

	// Scene Items
	RequestResult GetSceneItemList(const Request &);
	RequestResult GetGroupSceneItemList(const Request &);
	RequestResult GetSceneItemId(const Request &);
	RequestResult GetSceneItemSource(const Request &);
	RequestResult CreateSceneItem(const Request &);
	RequestResult RemoveSceneItem(const Request &);
	RequestResult DuplicateSceneItem(const Request &);
	RequestResult GetSceneItemTransform(const Request &);
	RequestResult SetSceneItemTransform(const Request &);
	RequestResult GetSceneItemEnabled(const Request &);
	RequestResult SetSceneItemEnabled(const Request &);
	RequestResult GetSceneItemLocked(const Request &);
	RequestResult SetSceneItemLocked(const Request &);
	RequestResult GetSceneItemIndex(const Request &);
	RequestResult SetSceneItemIndex(const Request &);
	RequestResult GetSceneItemBlendMode(const Request &);
	RequestResult SetSceneItemBlendMode(const Request &);
	RequestResult GetSceneItemPrivateSettings(const Request &);
	RequestResult SetSceneItemPrivateSettings(const Request &);

	// Outputs
	RequestResult GetVirtualCamStatus(const Request &);
	RequestResult ToggleVirtualCam(const Request &);
	RequestResult StartVirtualCam(const Request &);
	RequestResult StopVirtualCam(const Request &);
	RequestResult GetReplayBufferStatus(const Request &);
	RequestResult ToggleReplayBuffer(const Request &);
	RequestResult StartReplayBuffer(const Request &);
	RequestResult StopReplayBuffer(const Request &);
	RequestResult SaveReplayBuffer(const Request &);
	RequestResult GetLastReplayBufferReplay(const Request &);
	RequestResult GetOutputList(const Request &);
	RequestResult GetOutputStatus(const Request &);
	RequestResult ToggleOutput(const Request &);
	RequestResult StartOutput(const Request &);
	RequestResult StopOutput(const Request &);
	RequestResult GetOutputSettings(const Request &);
	RequestResult SetOutputSettings(const Request &);

	// Stream
	RequestResult GetStreamStatus(const Request &);
	RequestResult ToggleStream(const Request &);
	RequestResult StartStream(const Request &);
	RequestResult StopStream(const Request &);
	RequestResult SendStreamCaption(const Request &);

	// Record
	RequestResult GetRecordStatus(const Request &);
	RequestResult ToggleRecord(const Request &);
	RequestResult StartRecord(const Request &);
	RequestResult StopRecord(const Request &);
	RequestResult ToggleRecordPause(const Request &);
	RequestResult PauseRecord(const Request &);
	RequestResult ResumeRecord(const Request &);
	RequestResult SplitRecordFile(const Request &);
	RequestResult CreateRecordChapter(const Request &);

	// Media Inputs
	RequestResult GetMediaInputStatus(const Request &);
	RequestResult SetMediaInputCursor(const Request &);
	RequestResult OffsetMediaInputCursor(const Request &);
	RequestResult TriggerMediaInputAction(const Request &);

	// Ui
	RequestResult GetStudioModeEnabled(const Request &);
	RequestResult SetStudioModeEnabled(const Request &);
	RequestResult OpenInputPropertiesDialog(const Request &);
	RequestResult OpenInputFiltersDialog(const Request &);
	RequestResult OpenInputInteractDialog(const Request &);
	RequestResult GetMonitorList(const Request &);
	RequestResult OpenVideoMixProjector(const Request &);
	RequestResult OpenSourceProjector(const Request &);

	SessionPtr _session;
	static const std::unordered_map<std::string, RequestMethodHandler> _handlerMap;
};
