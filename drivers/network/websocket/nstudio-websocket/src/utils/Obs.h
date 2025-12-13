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

#include <string>
#include <obs.hpp>
#include <obs-frontend-api.h>

#include "Json.h"

// Autorelease object definitions
inline void ___properties_dummy_addref(obs_properties_t *) {}
using OBSPropertiesAutoDestroy = OBSRef<obs_properties_t *, ___properties_dummy_addref, obs_properties_destroy>;

template<typename T> T *GetCalldataPointer(const calldata_t *data, const char *name)
{
	void *ptr = nullptr;
	calldata_get_ptr(data, name, &ptr);
	return static_cast<T *>(ptr);
}

enum ObsOutputState {
	/**
	* Unknown state.
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_UNKNOWN
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_UNKNOWN,
	/**
	* The output is starting.
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_STARTING
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_STARTING,
	/**
	* The input has started.
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_STARTED
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_STARTED,
	/**
	* The output is stopping.
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_STOPPING
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_STOPPING,
	/**
	* The output has stopped.
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_STOPPED
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_STOPPED,
	/**
	* The output has disconnected and is reconnecting.
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_RECONNECTING
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_RECONNECTING,
	/**
	* The output has reconnected successfully.
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_RECONNECTED
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.1.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_RECONNECTED,
	/**
	* The output is now paused.
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_PAUSED
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.1.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_PAUSED,
	/**
	* The output has been resumed (unpaused).
	*
	* @enumIdentifier OBS_WEBSOCKET_OUTPUT_RESUMED
	* @enumType ObsOutputState
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_OUTPUT_RESUMED,
};
NLOHMANN_JSON_SERIALIZE_ENUM(ObsOutputState, {
						     {OBS_WEBSOCKET_OUTPUT_UNKNOWN, "OBS_WEBSOCKET_OUTPUT_UNKNOWN"},
						     {OBS_WEBSOCKET_OUTPUT_STARTING, "OBS_WEBSOCKET_OUTPUT_STARTING"},
						     {OBS_WEBSOCKET_OUTPUT_STARTED, "OBS_WEBSOCKET_OUTPUT_STARTED"},
						     {OBS_WEBSOCKET_OUTPUT_STOPPING, "OBS_WEBSOCKET_OUTPUT_STOPPING"},
						     {OBS_WEBSOCKET_OUTPUT_STOPPED, "OBS_WEBSOCKET_OUTPUT_STOPPED"},
						     {OBS_WEBSOCKET_OUTPUT_RECONNECTING, "OBS_WEBSOCKET_OUTPUT_RECONNECTING"},
						     {OBS_WEBSOCKET_OUTPUT_RECONNECTED, "OBS_WEBSOCKET_OUTPUT_RECONNECTED"},
						     {OBS_WEBSOCKET_OUTPUT_PAUSED, "OBS_WEBSOCKET_OUTPUT_PAUSED"},
						     {OBS_WEBSOCKET_OUTPUT_RESUMED, "OBS_WEBSOCKET_OUTPUT_RESUMED"},
					     })

enum ObsMediaInputAction {
	/**
	* No action.
	*
	* @enumIdentifier OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE
	* @enumType ObsMediaInputAction
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE,
	/**
	* Play the media input.
	*
	* @enumIdentifier OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY
	* @enumType ObsMediaInputAction
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY,
	/**
	* Pause the media input.
	*
	* @enumIdentifier OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE
	* @enumType ObsMediaInputAction
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE,
	/**
	* Stop the media input.
	*
	* @enumIdentifier OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP
	* @enumType ObsMediaInputAction
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP,
	/**
	* Restart the media input.
	*
	* @enumIdentifier OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART
	* @enumType ObsMediaInputAction
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART,
	/**
	* Go to the next playlist item.
	*
	* @enumIdentifier OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT
	* @enumType ObsMediaInputAction
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT,
	/**
	* Go to the previous playlist item.
	*
	* @enumIdentifier OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS
	* @enumType ObsMediaInputAction
	* @rpcVersion -1
	* @initialVersion 5.0.0
	* @api enums
	*/
	OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS,
};
NLOHMANN_JSON_SERIALIZE_ENUM(ObsMediaInputAction,
			     {
				     {OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE, "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE"},
				     {OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY, "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY"},
				     {OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE, "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE"},
				     {OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP, "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP"},
				     {OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART, "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART"},
				     {OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT, "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT"},
				     {OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS, "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS"},
			     })

namespace Utils {
	namespace Obs {
		namespace StringHelper {
			std::string GetObsVersion();
			std::string GetModuleConfigPath(std::string fileName);
			std::string GetCurrentSceneCollection();
			std::string GetCurrentProfile();
			std::string GetCurrentProfilePath();
			std::string GetCurrentRecordOutputPath();
			std::string GetLastRecordFileName();
			std::string GetLastReplayBufferFileName();
			std::string GetLastScreenshotFileName();
			std::string DurationToTimecode(uint64_t);
		}

		namespace NumberHelper {
			uint64_t GetOutputDuration(obs_output_t *output);
			size_t GetSceneCount();
			size_t GetSourceFilterIndex(obs_source_t *source, obs_source_t *filter);
		}

		namespace ArrayHelper {
			std::vector<std::string> GetSceneCollectionList();
			std::vector<std::string> GetProfileList();
			std::vector<obs_hotkey_t *> GetHotkeyList();
			std::vector<std::string> GetHotkeyNameList();
			std::vector<json> GetSceneList();
			std::vector<std::string> GetGroupList();
			std::vector<json> GetSceneItemList(obs_scene_t *scene, bool basic = false);
			std::vector<json> GetInputList(std::string inputKind = "");
			std::vector<std::string> GetInputKindList(bool unversioned = false, bool includeDisabled = false);
			std::vector<json> GetListPropertyItems(obs_property_t *property);
			std::vector<std::string> GetTransitionKindList();
			std::vector<json> GetSceneTransitionList();
			std::vector<json> GetSourceFilterList(obs_source_t *source);
			std::vector<std::string> GetFilterKindList();
			std::vector<json> GetOutputList();
		}

		namespace ObjectHelper {
			json GetStats();
			json GetSceneItemTransform(obs_sceneitem_t *item);
		}

		namespace SearchHelper {
			obs_hotkey_t *GetHotkeyByName(std::string name, std::string context);
			obs_source_t *GetSceneTransitionByName(std::string name); // Increments source ref. Use OBSSourceAutoRelease
			obs_sceneitem_t *GetSceneItemByName(obs_scene_t *scene, std::string name,
							    int offset = 0); // Increments ref. Use OBSSceneItemAutoRelease
		}

		namespace ActionHelper {
			obs_sceneitem_t *
			CreateSceneItem(obs_source_t *source, obs_scene_t *scene, bool sceneItemEnabled = true,
					obs_transform_info *sceneItemTransform = nullptr,
					obs_sceneitem_crop *sceneItemCrop = nullptr); // Increments ref. Use OBSSceneItemAutoRelease
			obs_sceneitem_t *CreateInput(std::string inputName, std::string inputKind, obs_data_t *inputSettings,
						     obs_scene_t *scene,
						     bool sceneItemEnabled = true); // Increments ref. Use OBSSceneItemAutoRelease
			obs_source_t *
			CreateSourceFilter(obs_source_t *source, std::string filterName, std::string filterKind,
					   obs_data_t *filterSettings); // Increments source ref. Use OBSSourceAutoRelease
			void SetSourceFilterIndex(obs_source_t *source, obs_source_t *filter, size_t index);
		}
	}
}
