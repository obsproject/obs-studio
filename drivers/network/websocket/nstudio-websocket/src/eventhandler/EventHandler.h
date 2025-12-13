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

#include <atomic>
#include <obs.hpp>
#include <obs-frontend-api.h>

#include "types/EventSubscription.h"
#include "../obs-websocket.h"
#include "../utils/Obs.h"
#include "../utils/Obs_VolumeMeter.h"
#include "plugin-macros.generated.h"

class EventHandler {
public:
	EventHandler();
	~EventHandler();

	void ProcessSubscriptionChange(bool type, uint64_t eventSubscriptions);

	// Callback when an event fires
	typedef std::function<void(uint64_t, std::string, json, uint8_t)>
		EventCallback; // uint64_t requiredIntent, std::string eventType, json eventData, uint8_t rpcVersion
	inline void SetEventCallback(EventCallback cb) { _eventCallback = cb; }

	// Callback when OBS becomes ready or non-ready
	typedef std::function<void(bool)> ObsReadyCallback; // bool ready
	inline void SetObsReadyCallback(ObsReadyCallback cb) { _obsReadyCallback = cb; }

private:
	EventCallback _eventCallback;
	ObsReadyCallback _obsReadyCallback;

	std::atomic<bool> _obsReady = false;

	std::vector<OBSSignal> coreSignals;
	OBSSignal recordFileChangedSignal;

	std::unique_ptr<Utils::Obs::VolumeMeter::Handler> _inputVolumeMetersHandler;
	std::atomic<uint64_t> _inputVolumeMetersRef = 0;
	std::atomic<uint64_t> _inputActiveStateChangedRef = 0;
	std::atomic<uint64_t> _inputShowStateChangedRef = 0;
	std::atomic<uint64_t> _sceneItemTransformChangedRef = 0;

	void ConnectSourceSignals(obs_source_t *source);
	void DisconnectSourceSignals(obs_source_t *source);

	void BroadcastEvent(uint64_t requiredIntent, std::string eventType, json eventData = nullptr, uint8_t rpcVersion = 0);

	// Signal handler: frontend
	static void OnFrontendEvent(enum obs_frontend_event event, void *private_data);
	void FrontendFinishedLoadingMultiHandler();
	void FrontendExitMultiHandler();

	// Signal handler: libobs
	static void SourceCreatedMultiHandler(void *param, calldata_t *data);
	static void SourceDestroyedMultiHandler(void *param, calldata_t *data);
	static void SourceRemovedMultiHandler(void *param, calldata_t *data);
	static void SourceRenamedMultiHandler(void *param, calldata_t *data);
	static void SourceUpdatedMultiHandler(void *param, calldata_t *data);

	// Signal handler: media sources
	static void SourceMediaPauseMultiHandler(void *param, calldata_t *data);
	static void SourceMediaPlayMultiHandler(void *param, calldata_t *data);
	static void SourceMediaRestartMultiHandler(void *param, calldata_t *data);
	static void SourceMediaStopMultiHandler(void *param, calldata_t *data);
	static void SourceMediaNextMultiHandler(void *param, calldata_t *data);
	static void SourceMediaPreviousMultiHandler(void *param, calldata_t *data);

	// Signal handler: output
	static void StreamOutputReconnectHandler(void *param, calldata_t *data);
	static void StreamOutputReconnectSuccessHandler(void *param, calldata_t *data);

	// General
	void HandleExitStarted();

	// Config
	void HandleCurrentSceneCollectionChanging();
	void HandleCurrentSceneCollectionChanged();
	void HandleSceneCollectionListChanged();
	void HandleCurrentProfileChanging();
	void HandleCurrentProfileChanged();
	void HandleProfileListChanged();

	// Scenes
	void HandleSceneCreated(obs_source_t *source);
	void HandleSceneRemoved(obs_source_t *source);
	void HandleSceneNameChanged(obs_source_t *source, std::string oldSceneName, std::string sceneName);
	void HandleCurrentProgramSceneChanged();
	void HandleCurrentPreviewSceneChanged();
	void HandleSceneListChanged();

	// Inputs
	void HandleInputCreated(obs_source_t *source);
	void HandleInputRemoved(obs_source_t *source);
	void HandleInputNameChanged(obs_source_t *source, std::string oldInputName, std::string inputName);
	void HandleInputSettingsChanged(obs_source_t *source);
	static void HandleInputActiveStateChanged(void *param,
						  calldata_t *data); // Direct callback
	static void HandleInputShowStateChanged(void *param,
						calldata_t *data); // Direct callback
	static void HandleInputMuteStateChanged(void *param,
						calldata_t *data); // Direct callback
	static void HandleInputVolumeChanged(void *param,
					     calldata_t *data); // Direct callback
	static void HandleInputAudioBalanceChanged(void *param,
						   calldata_t *data); // Direct callback
	static void HandleInputAudioSyncOffsetChanged(void *param,
						      calldata_t *data); // Direct callback
	static void HandleInputAudioTracksChanged(void *param,
						  calldata_t *data); // Direct callback
	static void HandleInputAudioMonitorTypeChanged(void *param,
						       calldata_t *data); // Direct callback
	void HandleInputVolumeMeters(std::vector<json> &inputs);          // AudioMeter::Handler callback

	// Transitions
	void HandleCurrentSceneTransitionChanged();
	void HandleCurrentSceneTransitionDurationChanged();
	static void HandleSceneTransitionStarted(void *param,
						 calldata_t *data); // Direct callback
	static void HandleSceneTransitionEnded(void *param,
					       calldata_t *data); // Direct callback
	static void HandleSceneTransitionVideoEnded(void *param,
						    calldata_t *data); // Direct callback

	// Filters
	static void FilterAddMultiHandler(void *param,
					  calldata_t *data); // Direct callback
	static void FilterRemoveMultiHandler(void *param,
					     calldata_t *data); // Direct callback
	static void HandleSourceFilterListReindexed(void *param,
						    calldata_t *data); // Direct callback
	void HandleSourceFilterCreated(obs_source_t *source, obs_source_t *filter);
	void HandleSourceFilterRemoved(obs_source_t *source, obs_source_t *filter);
	static void HandleSourceFilterNameChanged(void *param,
						  calldata_t *data); // Direct callback
	void HandleSourceFilterSettingsChanged(obs_source_t *source);
	static void HandleSourceFilterEnableStateChanged(void *param, calldata_t *data); // Direct callback

	// Outputs
	void HandleStreamStateChanged(ObsOutputState state);
	void HandleRecordStateChanged(ObsOutputState state);
	static void HandleRecordFileChanged(void *param, calldata_t *data); // Direct callback
	void HandleReplayBufferStateChanged(ObsOutputState state);
	void HandleVirtualcamStateChanged(ObsOutputState state);
	void HandleReplayBufferSaved();

	// Scene Items
	static void HandleSceneItemCreated(void *param,
					   calldata_t *data); // Direct callback
	static void HandleSceneItemRemoved(void *param,
					   calldata_t *data); // Direct callback
	static void HandleSceneItemListReindexed(void *param,
						 calldata_t *data); // Direct callback
	static void HandleSceneItemEnableStateChanged(void *param,
						      calldata_t *data); // Direct callback
	static void HandleSceneItemLockStateChanged(void *param,
						    calldata_t *data); // Direct callback
	static void HandleSceneItemSelected(void *param,
					    calldata_t *data); // Direct callback
	static void HandleSceneItemTransformChanged(void *param,
						    calldata_t *data); // Direct callback

	// Media Inputs
	static void HandleMediaInputPlaybackStarted(void *param,
						    calldata_t *data); // Direct callback
	static void HandleMediaInputPlaybackEnded(void *param,
						  calldata_t *data); // Direct callback
	void HandleMediaInputActionTriggered(obs_source_t *source, ObsMediaInputAction action);

	// Ui
	void HandleStudioModeStateChanged(bool enabled);
	void HandleScreenshotSaved();
};
