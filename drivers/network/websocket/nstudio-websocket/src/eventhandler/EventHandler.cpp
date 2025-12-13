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

#include "EventHandler.h"

EventHandler::EventHandler()
{
	blog_debug("[EventHandler::EventHandler] Setting up...");

	obs_frontend_add_event_callback(OnFrontendEvent, this);

	signal_handler_t *coreSignalHandler = obs_get_signal_handler();
	if (coreSignalHandler) {
		coreSignals.emplace_back(coreSignalHandler, "source_create", SourceCreatedMultiHandler, this);
		coreSignals.emplace_back(coreSignalHandler, "source_destroy", SourceDestroyedMultiHandler, this);
		coreSignals.emplace_back(coreSignalHandler, "source_remove", SourceRemovedMultiHandler, this);
		coreSignals.emplace_back(coreSignalHandler, "source_rename", SourceRenamedMultiHandler, this);
		coreSignals.emplace_back(coreSignalHandler, "source_update", SourceUpdatedMultiHandler, this);
	} else {
		blog(LOG_ERROR, "[EventHandler::EventHandler] Unable to get libobs signal handler!");
	}

	blog_debug("[EventHandler::EventHandler] Finished.");
}

EventHandler::~EventHandler()
{
	blog_debug("[EventHandler::~EventHandler] Shutting down...");

	obs_frontend_remove_event_callback(OnFrontendEvent, this);

	coreSignals.clear();

	// Revoke callbacks of all inputs and scenes, in case some still have our callbacks attached
	auto enumInputs = [](void *param, obs_source_t *source) {
		auto eventHandler = static_cast<EventHandler *>(param);
		eventHandler->DisconnectSourceSignals(source);
		return true;
	};
	obs_enum_sources(enumInputs, this);
	auto enumScenes = [](void *param, obs_source_t *source) {
		auto eventHandler = static_cast<EventHandler *>(param);
		eventHandler->DisconnectSourceSignals(source);
		return true;
	};
	obs_enum_scenes(enumScenes, this);

	blog_debug("[EventHandler::~EventHandler] Finished.");
}

// Function to increment or decrement refcounts for high volume event subscriptions
void EventHandler::ProcessSubscriptionChange(bool type, uint64_t eventSubscriptions)
{
	if (type) {
		if ((eventSubscriptions & EventSubscription::InputVolumeMeters) != 0) {
			if (_inputVolumeMetersRef.fetch_add(1) == 0) {
				if (_inputVolumeMetersHandler)
					blog(LOG_WARNING,
					     "[EventHandler::ProcessSubscription] Input volume meter handler already exists!");
				else
					_inputVolumeMetersHandler = std::make_unique<Utils::Obs::VolumeMeter::Handler>(
						std::bind(&EventHandler::HandleInputVolumeMeters, this, std::placeholders::_1));
			}
		}
		if ((eventSubscriptions & EventSubscription::InputActiveStateChanged) != 0)
			_inputActiveStateChangedRef++;
		if ((eventSubscriptions & EventSubscription::InputShowStateChanged) != 0)
			_inputShowStateChangedRef++;
		if ((eventSubscriptions & EventSubscription::SceneItemTransformChanged) != 0)
			_sceneItemTransformChangedRef++;
	} else {
		if ((eventSubscriptions & EventSubscription::InputVolumeMeters) != 0) {
			if (_inputVolumeMetersRef.fetch_sub(1) == 1)
				_inputVolumeMetersHandler.reset();
		}
		if ((eventSubscriptions & EventSubscription::InputActiveStateChanged) != 0)
			_inputActiveStateChangedRef--;
		if ((eventSubscriptions & EventSubscription::InputShowStateChanged) != 0)
			_inputShowStateChangedRef--;
		if ((eventSubscriptions & EventSubscription::SceneItemTransformChanged) != 0)
			_sceneItemTransformChangedRef--;
	}
}

// Function required in order to use default arguments
void EventHandler::BroadcastEvent(uint64_t requiredIntent, std::string eventType, json eventData, uint8_t rpcVersion)
{
	if (!_eventCallback)
		return;

	_eventCallback(requiredIntent, eventType, eventData, rpcVersion);
}

// Connect source signals for Inputs, Scenes, and Transitions. Filters are automatically connected.
void EventHandler::ConnectSourceSignals(obs_source_t *source) // Applies to inputs and scenes
{
	if (!source || obs_source_removed(source))
		return;

	// Disconnect all existing signals from the source to prevent multiple connections
	DisconnectSourceSignals(source);

	signal_handler_t *sh = obs_source_get_signal_handler(source);

	obs_source_type sourceType = obs_source_get_type(source);

	// Inputs
	if (sourceType == OBS_SOURCE_TYPE_INPUT) {
		signal_handler_connect(sh, "activate", HandleInputActiveStateChanged, this);
		signal_handler_connect(sh, "deactivate", HandleInputActiveStateChanged, this);
		signal_handler_connect(sh, "show", HandleInputShowStateChanged, this);
		signal_handler_connect(sh, "hide", HandleInputShowStateChanged, this);
		signal_handler_connect(sh, "mute", HandleInputMuteStateChanged, this);
		signal_handler_connect(sh, "volume", HandleInputVolumeChanged, this);
		signal_handler_connect(sh, "audio_balance", HandleInputAudioBalanceChanged, this);
		signal_handler_connect(sh, "audio_sync", HandleInputAudioSyncOffsetChanged, this);
		signal_handler_connect(sh, "audio_mixers", HandleInputAudioTracksChanged, this);
		signal_handler_connect(sh, "audio_monitoring", HandleInputAudioMonitorTypeChanged, this);
		signal_handler_connect(sh, "media_started", HandleMediaInputPlaybackStarted, this);
		signal_handler_connect(sh, "media_ended", HandleMediaInputPlaybackEnded, this);
		signal_handler_connect(sh, "media_pause", SourceMediaPauseMultiHandler, this);
		signal_handler_connect(sh, "media_play", SourceMediaPlayMultiHandler, this);
		signal_handler_connect(sh, "media_restart", SourceMediaRestartMultiHandler, this);
		signal_handler_connect(sh, "media_stopped", SourceMediaStopMultiHandler, this);
		signal_handler_connect(sh, "media_next", SourceMediaNextMultiHandler, this);
		signal_handler_connect(sh, "media_previous", SourceMediaPreviousMultiHandler, this);
	}

	// Scenes
	if (sourceType == OBS_SOURCE_TYPE_SCENE) {
		signal_handler_connect(sh, "item_add", HandleSceneItemCreated, this);
		signal_handler_connect(sh, "item_remove", HandleSceneItemRemoved, this);
		signal_handler_connect(sh, "reorder", HandleSceneItemListReindexed, this);
		signal_handler_connect(sh, "item_visible", HandleSceneItemEnableStateChanged, this);
		signal_handler_connect(sh, "item_locked", HandleSceneItemLockStateChanged, this);
		signal_handler_connect(sh, "item_select", HandleSceneItemSelected, this);
		signal_handler_connect(sh, "item_transform", HandleSceneItemTransformChanged, this);
	}

	// Scenes and Inputs
	if (sourceType == OBS_SOURCE_TYPE_INPUT || sourceType == OBS_SOURCE_TYPE_SCENE) {
		signal_handler_connect(sh, "reorder_filters", HandleSourceFilterListReindexed, this);
		signal_handler_connect(sh, "filter_add", FilterAddMultiHandler, this);
		signal_handler_connect(sh, "filter_remove", FilterRemoveMultiHandler, this);
		auto enumFilters = [](obs_source_t *, obs_source_t *filter, void *param) {
			auto eventHandler = static_cast<EventHandler *>(param);
			eventHandler->ConnectSourceSignals(filter);
		};
		obs_source_enum_filters(source, enumFilters, this);
	}

	// Transitions
	if (sourceType == OBS_SOURCE_TYPE_TRANSITION) {
		signal_handler_connect(sh, "transition_start", HandleSceneTransitionStarted, this);
		signal_handler_connect(sh, "transition_stop", HandleSceneTransitionEnded, this);
		signal_handler_connect(sh, "transition_video_stop", HandleSceneTransitionVideoEnded, this);
	}

	// Filters
	if (sourceType == OBS_SOURCE_TYPE_FILTER) {
		signal_handler_connect(sh, "enable", HandleSourceFilterEnableStateChanged, this);
		signal_handler_connect(sh, "rename", HandleSourceFilterNameChanged, this);
	}
}

// Disconnect source signals for Inputs, Scenes, and Transitions. Filters are automatically disconnected.
void EventHandler::DisconnectSourceSignals(obs_source_t *source)
{
	if (!source)
		return;

	signal_handler_t *sh = obs_source_get_signal_handler(source);

	obs_source_type sourceType = obs_source_get_type(source);

	// Inputs
	if (sourceType == OBS_SOURCE_TYPE_INPUT) {
		signal_handler_disconnect(sh, "activate", HandleInputActiveStateChanged, this);
		signal_handler_disconnect(sh, "deactivate", HandleInputActiveStateChanged, this);
		signal_handler_disconnect(sh, "show", HandleInputShowStateChanged, this);
		signal_handler_disconnect(sh, "hide", HandleInputShowStateChanged, this);
		signal_handler_disconnect(sh, "mute", HandleInputMuteStateChanged, this);
		signal_handler_disconnect(sh, "volume", HandleInputVolumeChanged, this);
		signal_handler_disconnect(sh, "audio_balance", HandleInputAudioBalanceChanged, this);
		signal_handler_disconnect(sh, "audio_sync", HandleInputAudioSyncOffsetChanged, this);
		signal_handler_disconnect(sh, "audio_mixers", HandleInputAudioTracksChanged, this);
		signal_handler_disconnect(sh, "audio_monitoring", HandleInputAudioMonitorTypeChanged, this);
		signal_handler_disconnect(sh, "media_started", HandleMediaInputPlaybackStarted, this);
		signal_handler_disconnect(sh, "media_ended", HandleMediaInputPlaybackEnded, this);
		signal_handler_disconnect(sh, "media_pause", SourceMediaPauseMultiHandler, this);
		signal_handler_disconnect(sh, "media_play", SourceMediaPlayMultiHandler, this);
		signal_handler_disconnect(sh, "media_restart", SourceMediaRestartMultiHandler, this);
		signal_handler_disconnect(sh, "media_stopped", SourceMediaStopMultiHandler, this);
		signal_handler_disconnect(sh, "media_next", SourceMediaNextMultiHandler, this);
		signal_handler_disconnect(sh, "media_previous", SourceMediaPreviousMultiHandler, this);
	}

	// Scenes
	if (sourceType == OBS_SOURCE_TYPE_SCENE) {
		signal_handler_disconnect(sh, "item_add", HandleSceneItemCreated, this);
		signal_handler_disconnect(sh, "item_remove", HandleSceneItemRemoved, this);
		signal_handler_disconnect(sh, "reorder", HandleSceneItemListReindexed, this);
		signal_handler_disconnect(sh, "item_visible", HandleSceneItemEnableStateChanged, this);
		signal_handler_disconnect(sh, "item_locked", HandleSceneItemLockStateChanged, this);
		signal_handler_disconnect(sh, "item_select", HandleSceneItemSelected, this);
		signal_handler_disconnect(sh, "item_transform", HandleSceneItemTransformChanged, this);
	}

	// Inputs and Scenes
	if (sourceType == OBS_SOURCE_TYPE_INPUT || sourceType == OBS_SOURCE_TYPE_SCENE) {
		signal_handler_disconnect(sh, "reorder_filters", HandleSourceFilterListReindexed, this);
		signal_handler_disconnect(sh, "filter_add", FilterAddMultiHandler, this);
		signal_handler_disconnect(sh, "filter_remove", FilterRemoveMultiHandler, this);
		auto enumFilters = [](obs_source_t *, obs_source_t *filter, void *param) {
			auto eventHandler = static_cast<EventHandler *>(param);
			eventHandler->DisconnectSourceSignals(filter);
		};
		obs_source_enum_filters(source, enumFilters, this);
	}

	// Transitions
	if (sourceType == OBS_SOURCE_TYPE_TRANSITION) {
		signal_handler_disconnect(sh, "transition_start", HandleSceneTransitionStarted, this);
		signal_handler_disconnect(sh, "transition_stop", HandleSceneTransitionEnded, this);
		signal_handler_disconnect(sh, "transition_video_stop", HandleSceneTransitionVideoEnded, this);
	}

	// Filters
	if (sourceType == OBS_SOURCE_TYPE_FILTER) {
		signal_handler_disconnect(sh, "enable", HandleSourceFilterEnableStateChanged, this);
		signal_handler_disconnect(sh, "rename", HandleSourceFilterNameChanged, this);
	}
}

void EventHandler::OnFrontendEvent(enum obs_frontend_event event, void *private_data)
{
	auto eventHandler = static_cast<EventHandler *>(private_data);

	switch (event) {
	// General
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		eventHandler->FrontendFinishedLoadingMultiHandler();
		break;
	case OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN:
		eventHandler->FrontendExitMultiHandler();
		break;

	// Config
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING: {
		obs_frontend_source_list transitions = {};
		obs_frontend_get_transitions(&transitions);
		for (size_t i = 0; i < transitions.sources.num; i++) {
			obs_source_t *transition = transitions.sources.array[i];
			eventHandler->DisconnectSourceSignals(transition);
		}
		obs_frontend_source_list_free(&transitions);
	}
		// Before ready update to allow event to broadcast
		eventHandler->HandleCurrentSceneCollectionChanging();
		eventHandler->_obsReady = false;
		if (eventHandler->_obsReadyCallback)
			eventHandler->_obsReadyCallback(false);
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED: {
		obs_frontend_source_list transitions = {};
		obs_frontend_get_transitions(&transitions);
		for (size_t i = 0; i < transitions.sources.num; i++) {
			obs_source_t *transition = transitions.sources.array[i];
			eventHandler->ConnectSourceSignals(transition);
		}
		obs_frontend_source_list_free(&transitions);
	}
		eventHandler->_obsReady = true;
		if (eventHandler->_obsReadyCallback)
			eventHandler->_obsReadyCallback(true);
		eventHandler->HandleCurrentSceneCollectionChanged();
		break;
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED:
		eventHandler->HandleSceneCollectionListChanged();
		break;
	case OBS_FRONTEND_EVENT_PROFILE_CHANGING:
		eventHandler->HandleCurrentProfileChanging();
		break;
	case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
		eventHandler->HandleCurrentProfileChanged();
		break;
	case OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED:
		eventHandler->HandleProfileListChanged();
		break;

	// Scenes
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		eventHandler->HandleCurrentProgramSceneChanged();
		break;
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		eventHandler->HandleCurrentPreviewSceneChanged();
		break;
	case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
		eventHandler->HandleSceneListChanged();
		break;

	// Transitions
	case OBS_FRONTEND_EVENT_TRANSITION_CHANGED:
		eventHandler->HandleCurrentSceneTransitionChanged();
		break;
	case OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED: {
		obs_frontend_source_list transitions = {};
		obs_frontend_get_transitions(&transitions);
		for (size_t i = 0; i < transitions.sources.num; i++) {
			obs_source_t *transition = transitions.sources.array[i];
			eventHandler->ConnectSourceSignals(transition);
		}
		obs_frontend_source_list_free(&transitions);
	} break;
	case OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED:
		eventHandler->HandleCurrentSceneTransitionDurationChanged();
		break;

	// Outputs
	case OBS_FRONTEND_EVENT_STREAMING_STARTING:
		eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_STARTING);
		{
			// Connect signals for stream output reconnects (hacky)
			OBSOutputAutoRelease streamOutput = obs_frontend_get_streaming_output();
			if (streamOutput) {
				signal_handler_t *sh = obs_output_get_signal_handler(streamOutput);
				signal_handler_connect(sh, "reconnect", StreamOutputReconnectHandler, private_data);
				signal_handler_connect(sh, "reconnect_success", StreamOutputReconnectSuccessHandler, private_data);
			}
		}
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_STARTED);
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPING);
		{
			// Disconnect signals for stream output reconnects
			OBSOutputAutoRelease streamOutput = obs_frontend_get_streaming_output();
			if (streamOutput) {
				signal_handler_t *sh = obs_output_get_signal_handler(streamOutput);
				signal_handler_disconnect(sh, "reconnect", StreamOutputReconnectHandler, private_data);
				signal_handler_disconnect(sh, "reconnect_success", StreamOutputReconnectSuccessHandler,
							  private_data);
			}
		}
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPED);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTING:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_STARTING);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_STARTED);
		{
			OBSOutputAutoRelease recordOutput = obs_frontend_get_recording_output();
			if (recordOutput) {
				signal_handler_t *sh = obs_output_get_signal_handler(recordOutput);
				eventHandler->recordFileChangedSignal.Connect(sh, "file_changed", HandleRecordFileChanged,
									      private_data);
			}
		}
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPING:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPING);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPED);
		eventHandler->recordFileChangedSignal.Disconnect();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_PAUSED);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_RESUMED);
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING:
		eventHandler->HandleReplayBufferStateChanged(OBS_WEBSOCKET_OUTPUT_STARTING);
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED:
		eventHandler->HandleReplayBufferStateChanged(OBS_WEBSOCKET_OUTPUT_STARTED);
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING:
		eventHandler->HandleReplayBufferStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPING);
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED:
		eventHandler->HandleReplayBufferStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPED);
		break;
	case OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED:
		eventHandler->HandleVirtualcamStateChanged(OBS_WEBSOCKET_OUTPUT_STARTED);
		break;
	case OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED:
		eventHandler->HandleVirtualcamStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPED);
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED:
		eventHandler->HandleReplayBufferSaved();
		break;

	// Ui
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
		eventHandler->HandleStudioModeStateChanged(true);
		break;
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		eventHandler->HandleStudioModeStateChanged(false);
		break;
	case OBS_FRONTEND_EVENT_SCREENSHOT_TAKEN:
		eventHandler->HandleScreenshotSaved();
		break;

	default:
		break;
	}
}

void EventHandler::FrontendFinishedLoadingMultiHandler()
{
	blog_debug(
		"[EventHandler::FrontendFinishedLoadingMultiHandler] OBS has finished loading. Connecting final handlers and enabling events...");

	// Enumerate all scene transitions and connect each one
	{
		obs_frontend_source_list transitions = {};
		obs_frontend_get_transitions(&transitions);
		for (size_t i = 0; i < transitions.sources.num; i++) {
			obs_source_t *transition = transitions.sources.array[i];
			ConnectSourceSignals(transition);
		}
		obs_frontend_source_list_free(&transitions);
	}

	_obsReady = true;
	if (_obsReadyCallback)
		_obsReadyCallback(true);

	blog_debug("[EventHandler::FrontendFinishedLoadingMultiHandler] Finished.");
}

void EventHandler::FrontendExitMultiHandler()
{
	blog_debug("[EventHandler::FrontendExitMultiHandler] OBS is unloading. Disabling events...");

	HandleExitStarted();

	// Disconnect source signals and disable events when OBS starts unloading (to reduce extra logging).
	_obsReady = false;
	if (_obsReadyCallback)
		_obsReadyCallback(false);

	// Enumerate all scene transitions and disconnect each one
	{
		obs_frontend_source_list transitions = {};
		obs_frontend_get_transitions(&transitions);
		for (size_t i = 0; i < transitions.sources.num; i++) {
			obs_source_t *transition = transitions.sources.array[i];
			DisconnectSourceSignals(transition);
		}
		obs_frontend_source_list_free(&transitions);
	}

	blog_debug("[EventHandler::FrontendExitMultiHandler] Finished.");
}

// Only called for creation of a public source
void EventHandler::SourceCreatedMultiHandler(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	eventHandler->ConnectSourceSignals(source);

	switch (obs_source_get_type(source)) {
	case OBS_SOURCE_TYPE_INPUT:
		eventHandler->HandleInputCreated(source);
		break;
	case OBS_SOURCE_TYPE_SCENE:
		eventHandler->HandleSceneCreated(source);
		break;
	default:
		break;
	}
}

// Only called for destruction of a public sourcs
// Used as a fallback if an input/scene is not explicitly removed
void EventHandler::SourceDestroyedMultiHandler(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	// We can't use any smart types here because releasing the source will cause infinite recursion
	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	// Disconnect all signals from the source
	eventHandler->DisconnectSourceSignals(source);

	switch (obs_source_get_type(source)) {
	case OBS_SOURCE_TYPE_INPUT:
		// Only emit removed if the input has not already been removed. This is the case when removing the last scene item of an input.
		if (!obs_source_removed(source))
			eventHandler->HandleInputRemoved(source);
		break;
	case OBS_SOURCE_TYPE_SCENE:
		// Only emit removed if the scene has not already been removed.
		if (!obs_source_removed(source))
			eventHandler->HandleSceneRemoved(source);
		break;
	default:
		break;
	}
}

// We prefer remove signals over destroy signals because they are more time-accurate.
// For example, if an input is "removed" but there is a dangling ref, you still want to know that it shouldn't exist, but it's not guaranteed to be destroyed.
void EventHandler::SourceRemovedMultiHandler(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	switch (obs_source_get_type(source)) {
	case OBS_SOURCE_TYPE_INPUT:
		eventHandler->HandleInputRemoved(source);
		break;
	case OBS_SOURCE_TYPE_SCENE:
		eventHandler->HandleSceneRemoved(source);
		break;
	default:
		break;
	}
}

void EventHandler::SourceRenamedMultiHandler(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	std::string oldSourceName = calldata_string(data, "prev_name");
	std::string sourceName = calldata_string(data, "new_name");
	if (oldSourceName.empty() || sourceName.empty())
		return;

	switch (obs_source_get_type(source)) {
	case OBS_SOURCE_TYPE_INPUT:
		eventHandler->HandleInputNameChanged(source, oldSourceName, sourceName);
		break;
	case OBS_SOURCE_TYPE_TRANSITION:
		break;
	case OBS_SOURCE_TYPE_SCENE:
		eventHandler->HandleSceneNameChanged(source, oldSourceName, sourceName);
		break;
	default:
		break;
	}
}

void EventHandler::SourceUpdatedMultiHandler(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	switch (obs_source_get_type(source)) {
	case OBS_SOURCE_TYPE_INPUT:
		eventHandler->HandleInputSettingsChanged(source);
		break;
	case OBS_SOURCE_TYPE_FILTER:
		eventHandler->HandleSourceFilterSettingsChanged(source);
		break;
	default:
		break;
	}
}

void EventHandler::StreamOutputReconnectHandler(void *param, calldata_t *)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_RECONNECTING);
}

void EventHandler::StreamOutputReconnectSuccessHandler(void *param, calldata_t *)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_RECONNECTED);
}
