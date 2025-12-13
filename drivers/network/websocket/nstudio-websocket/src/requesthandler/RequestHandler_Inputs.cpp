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

#include "RequestHandler.h"

/**
 * Gets an array of all inputs in OBS.
 *
 * @requestField ?inputKind | String | Restrict the array to only inputs of the specified kind | All kinds included
 *
 * @responseField inputs | Array<Object> | Array of inputs
 *
 * @requestType GetInputList
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputList(const Request &request)
{
	std::string inputKind;

	if (request.Contains("inputKind")) {
		RequestStatus::RequestStatus statusCode;
		std::string comment;
		if (!request.ValidateOptionalString("inputKind", statusCode, comment))
			return RequestResult::Error(statusCode, comment);

		inputKind = request.RequestData["inputKind"];
	}

	json responseData;
	responseData["inputs"] = Utils::Obs::ArrayHelper::GetInputList(inputKind);
	return RequestResult::Success(responseData);
}

/**
 * Gets an array of all available input kinds in OBS.
 *
 * @requestField ?unversioned | Boolean | True == Return all kinds as unversioned, False == Return with version suffixes (if available) | false
 *
 * @responseField inputKinds | Array<String> | Array of input kinds
 *
 * @requestType GetInputKindList
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputKindList(const Request &request)
{
	bool unversioned = false;

	if (request.Contains("unversioned")) {
		RequestStatus::RequestStatus statusCode;
		std::string comment;
		if (!request.ValidateOptionalBoolean("unversioned", statusCode, comment))
			return RequestResult::Error(statusCode, comment);

		unversioned = request.RequestData["unversioned"];
	}

	json responseData;
	responseData["inputKinds"] = Utils::Obs::ArrayHelper::GetInputKindList(unversioned);
	return RequestResult::Success(responseData);
}

/**
 * Gets the names of all special inputs.
 *
 * @responseField desktop1 | String | Name of the Desktop Audio input
 * @responseField desktop2 | String | Name of the Desktop Audio 2 input
 * @responseField mic1     | String | Name of the Mic/Auxiliary Audio input
 * @responseField mic2     | String | Name of the Mic/Auxiliary Audio 2 input
 * @responseField mic3     | String | Name of the Mic/Auxiliary Audio 3 input
 * @responseField mic4     | String | Name of the Mic/Auxiliary Audio 4 input
 *
 * @requestType GetSpecialInputs
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetSpecialInputs(const Request &)
{
	json responseData;

	std::vector<std::string> channels = {"desktop1", "desktop2", "mic1", "mic2", "mic3", "mic4"};

	uint32_t channelId = 1;
	for (auto &channel : channels) {
		OBSSourceAutoRelease input = obs_get_output_source(channelId);
		if (!input)
			responseData[channel] = nullptr;
		else
			responseData[channel] = obs_source_get_name(input);

		channelId++;
	}

	return RequestResult::Success(responseData);
}

/**
 * Creates a new input, adding it as a scene item to the specified scene.
 *
 * @requestField ?sceneName        | String | Name of the scene to add the input to as a scene item
 * @requestField ?sceneUuid        | String | UUID of the scene to add the input to as a scene item
 * @requestField inputName         | String | Name of the new input to created
 * @requestField inputKind         | String | The kind of input to be created
 * @requestField ?inputSettings    | Object | Settings object to initialize the input with                  | Default settings used
 * @requestField ?sceneItemEnabled | Boolean | Whether to set the created scene item to enabled or disabled | True
 *
 * @responseField inputUuid   | String | UUID of the newly created input
 * @responseField sceneItemId | Number | ID of the newly created scene item
 *
 * @requestType CreateInput
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::CreateInput(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease sceneSource = request.ValidateScene(statusCode, comment);
	if (!(sceneSource && request.ValidateString("inputName", statusCode, comment) &&
	      request.ValidateString("inputKind", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string inputName = request.RequestData["inputName"];
	OBSSourceAutoRelease existingInput = obs_get_source_by_name(inputName.c_str());
	if (existingInput)
		return RequestResult::Error(RequestStatus::ResourceAlreadyExists, "A source already exists by that input name.");

	std::string inputKind = request.RequestData["inputKind"];
	auto kinds = Utils::Obs::ArrayHelper::GetInputKindList();
	if (std::find(kinds.begin(), kinds.end(), inputKind) == kinds.end())
		return RequestResult::Error(
			RequestStatus::InvalidInputKind,
			"Your specified input kind is not supported by OBS. Check that your specified kind is properly versioned and that any necessary plugins are loaded.");

	OBSDataAutoRelease inputSettings = nullptr;
	if (request.Contains("inputSettings")) {
		if (!request.ValidateOptionalObject("inputSettings", statusCode, comment, true))
			return RequestResult::Error(statusCode, comment);

		inputSettings = Utils::Json::JsonToObsData(request.RequestData["inputSettings"]);
	}

	OBSScene scene = obs_scene_from_source(sceneSource);

	bool sceneItemEnabled = true;
	if (request.Contains("sceneItemEnabled")) {
		if (!request.ValidateOptionalBoolean("sceneItemEnabled", statusCode, comment))
			return RequestResult::Error(statusCode, comment);

		sceneItemEnabled = request.RequestData["sceneItemEnabled"];
	}

	// Create the input and add it as a scene item to the destination scene
	OBSSceneItemAutoRelease sceneItem =
		Utils::Obs::ActionHelper::CreateInput(inputName, inputKind, inputSettings, scene, sceneItemEnabled);

	if (!sceneItem)
		return RequestResult::Error(RequestStatus::ResourceCreationFailed, "Creation of the input or scene item failed.");

	json responseData;
	responseData["inputUuid"] = obs_source_get_uuid(obs_sceneitem_get_source(sceneItem));
	responseData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	return RequestResult::Success(responseData);
}

/**
 * Removes an existing input.
 *
 * Note: Will immediately remove all associated scene items.
 *
 * @requestField ?inputName | String | Name of the input to remove
 * @requestField ?inputUuid | String | UUID of the input to remove
 *
 * @requestType RemoveInput
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::RemoveInput(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	// Some implementations of removing sources release before remove, and some release after.
	// Releasing afterwards guarantees that we don't accidentally destroy the source before
	// remove if we happen to hold the last ref (very, very rare)
	obs_source_remove(input);

	return RequestResult::Success();
}

/**
 * Sets the name of an input (rename).
 *
 * @requestField ?inputName   | String | Current input name
 * @requestField ?inputUuid   | String | Current input UUID
 * @requestField newInputName | String | New name for the input
 *
 * @requestType SetInputName
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputName(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateString("newInputName", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string newInputName = request.RequestData["newInputName"];

	OBSSourceAutoRelease existingSource = obs_get_source_by_name(newInputName.c_str());
	if (existingSource)
		return RequestResult::Error(RequestStatus::ResourceAlreadyExists,
					    "A source already exists by that new input name.");

	obs_source_set_name(input, newInputName.c_str());

	return RequestResult::Success();
}

/**
 * Gets the default settings for an input kind.
 *
 * @requestField inputKind | String | Input kind to get the default settings for
 *
 * @responseField defaultInputSettings | Object | Object of default settings for the input kind
 *
 * @requestType GetInputDefaultSettings
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputDefaultSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("inputKind", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string inputKind = request.RequestData["inputKind"];
	auto kinds = Utils::Obs::ArrayHelper::GetInputKindList();
	if (std::find(kinds.begin(), kinds.end(), inputKind) == kinds.end())
		return RequestResult::Error(RequestStatus::InvalidInputKind);

	OBSDataAutoRelease defaultSettings = obs_get_source_defaults(inputKind.c_str());
	if (!defaultSettings)
		return RequestResult::Error(RequestStatus::InvalidInputKind);

	json responseData;
	responseData["defaultInputSettings"] = Utils::Json::ObsDataToJson(defaultSettings, true);
	return RequestResult::Success(responseData);
}

/**
 * Gets the settings of an input.
 *
 * Note: Does not include defaults. To create the entire settings object, overlay `inputSettings` over the `defaultInputSettings` provided by `GetInputDefaultSettings`.
 *
 * @requestField ?inputName | String | Name of the input to get the settings of
 * @requestField ?inputUuid | String | UUID of the input to get the settings of
 *
 * @responseField inputSettings | Object | Object of settings for the input
 * @responseField inputKind     | String | The kind of the input
 *
 * @requestType GetInputSettings
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	OBSDataAutoRelease inputSettings = obs_source_get_settings(input);

	json responseData;
	responseData["inputSettings"] = Utils::Json::ObsDataToJson(inputSettings);
	responseData["inputKind"] = obs_source_get_id(input);
	return RequestResult::Success(responseData);
}

/**
 * Sets the settings of an input.
 *
 * @requestField ?inputName    | String  | Name of the input to set the settings of
 * @requestField ?inputUuid    | String  | UUID of the input to set the settings of
 * @requestField inputSettings | Object  | Object of settings to apply
 * @requestField ?overlay      | Boolean | True == apply the settings on top of existing ones, False == reset the input to its defaults, then apply settings. | true
 *
 * @requestType SetInputSettings
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateObject("inputSettings", statusCode, comment, true)))
		return RequestResult::Error(statusCode, comment);

	bool overlay = true;
	if (request.Contains("overlay")) {
		if (!request.ValidateOptionalBoolean("overlay", statusCode, comment))
			return RequestResult::Error(statusCode, comment);

		overlay = request.RequestData["overlay"];
	}

	// Get the new settings and convert it to obs_data_t*
	OBSDataAutoRelease newSettings = Utils::Json::JsonToObsData(request.RequestData["inputSettings"]);
	if (!newSettings)
		// This should never happen
		return RequestResult::Error(RequestStatus::RequestProcessingFailed,
					    "An internal data conversion operation failed. Please report this!");

	if (overlay)
		// Applies the new settings on top of the existing user settings
		obs_source_update(input, newSettings);
	else
		// Clears all user settings (leaving defaults) then applies the new settings
		obs_source_reset_settings(input, newSettings);

	// Tells any open source properties windows to perform a UI refresh
	obs_source_update_properties(input);

	return RequestResult::Success();
}

/**
 * Gets the audio mute state of an input.
 *
 * @requestField ?inputName | String | Name of input to get the mute state of
 * @requestField ?inputUuid | String | UUID of input to get the mute state of
 *
 * @responseField inputMuted | Boolean | Whether the input is muted
 *
 * @requestType GetInputMute
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputMute(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	json responseData;
	responseData["inputMuted"] = obs_source_muted(input);
	return RequestResult::Success(responseData);
}

/**
 * Sets the audio mute state of an input.
 *
 * @requestField ?inputName | String  | Name of the input to set the mute state of
 * @requestField ?inputUuid | String  | UUID of the input to set the mute state of
 * @requestField inputMuted | Boolean | Whether to mute the input or not
 *
 * @requestType SetInputMute
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputMute(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateBoolean("inputMuted", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	obs_source_set_muted(input, request.RequestData["inputMuted"]);

	return RequestResult::Success();
}

/**
 * Toggles the audio mute state of an input.
 *
 * @requestField ?inputName | String | Name of the input to toggle the mute state of
 * @requestField ?inputUuid | String | UUID of the input to toggle the mute state of
 *
 * @responseField inputMuted | Boolean | Whether the input has been muted or unmuted
 *
 * @requestType ToggleInputMute
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::ToggleInputMute(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	bool inputMuted = !obs_source_muted(input);
	obs_source_set_muted(input, inputMuted);

	json responseData;
	responseData["inputMuted"] = inputMuted;
	return RequestResult::Success(responseData);
}

/**
 * Gets the current volume setting of an input.
 *
 * @requestField ?inputName | String | Name of the input to get the volume of
 * @requestField ?inputUuid | String | UUID of the input to get the volume of
 *
 * @responseField inputVolumeMul | Number | Volume setting in mul
 * @responseField inputVolumeDb  | Number | Volume setting in dB
 *
 * @requestType GetInputVolume
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputVolume(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	float inputVolumeMul = obs_source_get_volume(input);
	float inputVolumeDb = obs_mul_to_db(inputVolumeMul);
	if (inputVolumeDb == -INFINITY)
		inputVolumeDb = -100.0;

	json responseData;
	responseData["inputVolumeMul"] = inputVolumeMul;
	responseData["inputVolumeDb"] = inputVolumeDb;
	return RequestResult::Success(responseData);
}

/**
 * Sets the volume setting of an input.
 *
 * @requestField ?inputName      | String | Name of the input to set the volume of
 * @requestField ?inputUuid      | String | UUID of the input to set the volume of
 * @requestField ?inputVolumeMul | Number | Volume setting in mul | >= 0, <= 20     | `inputVolumeDb` should be specified
 * @requestField ?inputVolumeDb  | Number | Volume setting in dB  | >= -100, <= 26 | `inputVolumeMul` should be specified
 *
 * @requestType SetInputVolume
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputVolume(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	bool hasMul = request.Contains("inputVolumeMul");
	if (hasMul && !request.ValidateOptionalNumber("inputVolumeMul", statusCode, comment, 0, 20))
		return RequestResult::Error(statusCode, comment);

	bool hasDb = request.Contains("inputVolumeDb");
	if (hasDb && !request.ValidateOptionalNumber("inputVolumeDb", statusCode, comment, -100, 26))
		return RequestResult::Error(statusCode, comment);

	if (hasMul && hasDb)
		return RequestResult::Error(RequestStatus::TooManyRequestFields, "You may only specify one volume field.");

	if (!hasMul && !hasDb)
		return RequestResult::Error(RequestStatus::MissingRequestField, "You must specify one volume field.");

	float inputVolumeMul;
	if (hasMul)
		inputVolumeMul = request.RequestData["inputVolumeMul"];
	else
		inputVolumeMul = obs_db_to_mul(request.RequestData["inputVolumeDb"]);

	obs_source_set_volume(input, inputVolumeMul);

	return RequestResult::Success();
}

/**
 * Gets the audio balance of an input.
 *
 * @requestField ?inputName | String | Name of the input to get the audio balance of
 * @requestField ?inputUuid | String | UUID of the input to get the audio balance of
 *
 * @responseField inputAudioBalance | Number | Audio balance value from 0.0-1.0
 *
 * @requestType GetInputAudioBalance
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputAudioBalance(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	json responseData;
	responseData["inputAudioBalance"] = obs_source_get_balance_value(input);

	return RequestResult::Success(responseData);
}

/**
 * Sets the audio balance of an input.
 *
 * @requestField ?inputName        | String | Name of the input to set the audio balance of
 * @requestField ?inputUuid        | String | UUID of the input to set the audio balance of
 * @requestField inputAudioBalance | Number | New audio balance value | >= 0.0, <= 1.0
 *
 * @requestType SetInputAudioBalance
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputAudioBalance(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateNumber("inputAudioBalance", statusCode, comment, 0.0, 1.0)))
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	float inputAudioBalance = request.RequestData["inputAudioBalance"];
	obs_source_set_balance_value(input, inputAudioBalance);

	return RequestResult::Success();
}

/**
 * Gets the audio sync offset of an input.
 *
 * Note: The audio sync offset can be negative too!
 *
 * @requestField ?inputName | String | Name of the input to get the audio sync offset of
 * @requestField ?inputUuid | String | UUID of the input to get the audio sync offset of
 *
 * @responseField inputAudioSyncOffset | Number | Audio sync offset in milliseconds
 *
 * @requestType GetInputAudioSyncOffset
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputAudioSyncOffset(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	json responseData;
	//									   Offset is stored in nanoseconds in OBS.
	responseData["inputAudioSyncOffset"] = obs_source_get_sync_offset(input) / 1000000;

	return RequestResult::Success(responseData);
}

/**
 * Sets the audio sync offset of an input.
 *
 * @requestField ?inputName           | String | Name of the input to set the audio sync offset of
 * @requestField ?inputUuid           | String | UUID of the input to set the audio sync offset of
 * @requestField inputAudioSyncOffset | Number | New audio sync offset in milliseconds | >= -950, <= 20000
 *
 * @requestType SetInputAudioSyncOffset
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputAudioSyncOffset(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateNumber("inputAudioSyncOffset", statusCode, comment, -950, 20000)))
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	int64_t syncOffset = request.RequestData["inputAudioSyncOffset"];
	obs_source_set_sync_offset(input, syncOffset * 1000000);

	return RequestResult::Success();
}

/**
 * Gets the audio monitor type of an input.
 *
 * The available audio monitor types are:
 *
 * - `OBS_MONITORING_TYPE_NONE`
 * - `OBS_MONITORING_TYPE_MONITOR_ONLY`
 * - `OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT`
 *
 * @requestField ?inputName | String | Name of the input to get the audio monitor type of
 * @requestField ?inputUuid | String | UUID of the input to get the audio monitor type of
 *
 * @responseField monitorType | String | Audio monitor type
 *
 * @requestType GetInputAudioMonitorType
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputAudioMonitorType(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	json responseData;
	responseData["monitorType"] = obs_source_get_monitoring_type(input);

	return RequestResult::Success(responseData);
}

/**
 * Sets the audio monitor type of an input.
 *
 * @requestField ?inputName  | String | Name of the input to set the audio monitor type of
 * @requestField ?inputUuid  | String | UUID of the input to set the audio monitor type of
 * @requestField monitorType | String | Audio monitor type
 *
 * @requestType SetInputAudioMonitorType
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputAudioMonitorType(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateString("monitorType", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	if (!obs_audio_monitoring_available())
		return RequestResult::Error(RequestStatus::InvalidResourceState,
					    "Audio monitoring is not available on this platform.");

	enum obs_monitoring_type monitorType;
	std::string monitorTypeString = request.RequestData["monitorType"];
	if (monitorTypeString == "OBS_MONITORING_TYPE_NONE")
		monitorType = OBS_MONITORING_TYPE_NONE;
	else if (monitorTypeString == "OBS_MONITORING_TYPE_MONITOR_ONLY")
		monitorType = OBS_MONITORING_TYPE_MONITOR_ONLY;
	else if (monitorTypeString == "OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT")
		monitorType = OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT;
	else
		return RequestResult::Error(RequestStatus::InvalidRequestField,
					    std::string("Unknown monitor type: ") + monitorTypeString);

	obs_source_set_monitoring_type(input, monitorType);

	return RequestResult::Success();
}

/**
 * Gets the enable state of all audio tracks of an input.
 *
 * @requestField ?inputName | String | Name of the input
 * @requestField ?inputUuid | String | UUID of the input
 *
 * @responseField inputAudioTracks | Object | Object of audio tracks and associated enable states
 *
 * @requestType GetInputAudioTracks
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputAudioTracks(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	long long tracks = obs_source_get_audio_mixers(input);

	json inputAudioTracks;
	for (long long i = 0; i < MAX_AUDIO_MIXES; i++) {
		inputAudioTracks[std::to_string(i + 1)] = (bool)((tracks >> i) & 1);
	}

	json responseData;
	responseData["inputAudioTracks"] = inputAudioTracks;

	return RequestResult::Success(responseData);
}

/**
 * Sets the enable state of audio tracks of an input.
 *
 * @requestField ?inputName       | String | Name of the input
 * @requestField ?inputUuid       | String | UUID of the input
 * @requestField inputAudioTracks | Object | Track settings to apply
 *
 * @requestType SetInputAudioTracks
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputAudioTracks(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input || !request.ValidateObject("inputAudioTracks", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_AUDIO))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input does not support audio.");

	json inputAudioTracks = request.RequestData["inputAudioTracks"];

	uint32_t mixers = obs_source_get_audio_mixers(input);

	for (uint32_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		std::string track = std::to_string(i + 1);

		if (!Utils::Json::Contains(inputAudioTracks, track))
			continue;

		if (!inputAudioTracks[track].is_boolean())
			return RequestResult::Error(RequestStatus::InvalidRequestFieldType,
						    "The value of one of your tracks is not a boolean.");

		bool enabled = inputAudioTracks[track];

		if (enabled)
			mixers |= (1 << i);
		else
			mixers &= ~(1 << i);
	}

	// Decided that checking if tracks have actually changed is unnecessary
	obs_source_set_audio_mixers(input, mixers);

	return RequestResult::Success();
}

/**
 * Gets the deinterlace mode of an input.
 *
 * Deinterlace Modes:
 *
 * - `OBS_DEINTERLACE_MODE_DISABLE`
 * - `OBS_DEINTERLACE_MODE_DISCARD`
 * - `OBS_DEINTERLACE_MODE_RETRO`
 * - `OBS_DEINTERLACE_MODE_BLEND`
 * - `OBS_DEINTERLACE_MODE_BLEND_2X`
 * - `OBS_DEINTERLACE_MODE_LINEAR`
 * - `OBS_DEINTERLACE_MODE_LINEAR_2X`
 * - `OBS_DEINTERLACE_MODE_YADIF`
 * - `OBS_DEINTERLACE_MODE_YADIF_2X`
 *
 * Note: Deinterlacing functionality is restricted to async inputs only.
 *
 * @requestField ?inputName | String | Name of the input
 * @requestField ?inputUuid | String | UUID of the input
 *
 * @responseField inputDeinterlaceMode | String | Deinterlace mode of the input
 *
 * @requestType GetInputDeinterlaceMode
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.6.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputDeinterlaceMode(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_ASYNC))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input is not async.");

	json responseData;
	responseData["inputDeinterlaceMode"] = obs_source_get_deinterlace_mode(input);

	return RequestResult::Success(responseData);
}

/**
 * Sets the deinterlace mode of an input.
 *
 * Note: Deinterlacing functionality is restricted to async inputs only.
 *
 * @requestField ?inputName           | String | Name of the input
 * @requestField ?inputUuid           | String | UUID of the input
 * @requestField inputDeinterlaceMode | String | Deinterlace mode for the input
 *
 * @requestType SetInputDeinterlaceMode
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.6.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputDeinterlaceMode(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input || !request.ValidateString("inputDeinterlaceMode", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_ASYNC))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input is not async.");

	enum obs_deinterlace_mode deinterlaceMode = request.RequestData["inputDeinterlaceMode"];
	if (deinterlaceMode == OBS_DEINTERLACE_MODE_DISABLE &&
	    request.RequestData["inputDeinterlaceMode"] != "OBS_DEINTERLACE_MODE_DISABLE")
		return RequestResult::Error(RequestStatus::InvalidRequestField,
					    "The field inputDeinterlaceMode has an invalid value.");

	obs_source_set_deinterlace_mode(input, deinterlaceMode);

	return RequestResult::Success();
}

/**
 * Gets the deinterlace field order of an input.
 *
 * Deinterlace Field Orders:
 *
 * - `OBS_DEINTERLACE_FIELD_ORDER_TOP`
 * - `OBS_DEINTERLACE_FIELD_ORDER_BOTTOM`
 *
 * Note: Deinterlacing functionality is restricted to async inputs only.
 *
 * @requestField ?inputName | String | Name of the input
 * @requestField ?inputUuid | String | UUID of the input
 *
 * @responseField inputDeinterlaceFieldOrder | String | Deinterlace field order of the input
 *
 * @requestType GetInputDeinterlaceFieldOrder
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.6.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputDeinterlaceFieldOrder(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_ASYNC))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input is not async.");

	json responseData;
	responseData["inputDeinterlaceFieldOrder"] = obs_source_get_deinterlace_field_order(input);

	return RequestResult::Success(responseData);
}

/**
 * Sets the deinterlace field order of an input.
 *
 * Note: Deinterlacing functionality is restricted to async inputs only.
 *
 * @requestField ?inputName                 | String | Name of the input
 * @requestField ?inputUuid                 | String | UUID of the input
 * @requestField inputDeinterlaceFieldOrder | String | Deinterlace field order for the input
 *
 * @requestType SetInputDeinterlaceFieldOrder
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.6.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::SetInputDeinterlaceFieldOrder(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input || !request.ValidateString("inputDeinterlaceFieldOrder", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_ASYNC))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The specified input is not async.");

	enum obs_deinterlace_field_order deinterlaceFieldOrder = request.RequestData["inputDeinterlaceFieldOrder"];
	if (deinterlaceFieldOrder == OBS_DEINTERLACE_FIELD_ORDER_TOP &&
	    request.RequestData["inputDeinterlaceFieldOrder"] != "OBS_DEINTERLACE_FIELD_ORDER_TOP")
		return RequestResult::Error(RequestStatus::InvalidRequestField,
					    "The field inputDeinterlaceFieldOrder has an invalid value.");

	obs_source_set_deinterlace_field_order(input, deinterlaceFieldOrder);

	return RequestResult::Success();
}

/**
 * Gets the items of a list property from an input's properties.
 *
 * Note: Use this in cases where an input provides a dynamic, selectable list of items. For example, display capture, where it provides a list of available displays.
 *
 * @requestField ?inputName   | String | Name of the input
 * @requestField ?inputUuid   | String | UUID of the input
 * @requestField propertyName | String | Name of the list property to get the items of
 *
 * @responseField propertyItems | Array<Object> | Array of items in the list property
 *
 * @requestType GetInputPropertiesListPropertyItems
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::GetInputPropertiesListPropertyItems(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateString("propertyName", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string propertyName = request.RequestData["propertyName"];

	OBSPropertiesAutoDestroy inputProperties = obs_source_properties(input);
	obs_property_t *property = obs_properties_get(inputProperties, propertyName.c_str());
	if (!property)
		return RequestResult::Error(RequestStatus::ResourceNotFound, "Unable to find a property by that name.");
	if (obs_property_get_type(property) != OBS_PROPERTY_LIST)
		return RequestResult::Error(RequestStatus::InvalidResourceType, "The property found is not a list.");

	json responseData;
	responseData["propertyItems"] = Utils::Obs::ArrayHelper::GetListPropertyItems(property);

	return RequestResult::Success(responseData);
}

/**
 * Presses a button in the properties of an input.
 *
 * Some known `propertyName` values are:
 *
 * - `refreshnocache` - Browser source reload button
 *
 * Note: Use this in cases where there is a button in the properties of an input that cannot be accessed in any other way. For example, browser sources, where there is a refresh button.
 *
 * @requestField ?inputName   | String | Name of the input
 * @requestField ?inputUuid   | String | UUID of the input
 * @requestField propertyName | String | Name of the button property to press
 *
 * @requestType PressInputPropertiesButton
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category inputs
 */
RequestResult RequestHandler::PressInputPropertiesButton(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateString("propertyName", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string propertyName = request.RequestData["propertyName"];

	OBSPropertiesAutoDestroy inputProperties = obs_source_properties(input);
	obs_property_t *property = obs_properties_get(inputProperties, propertyName.c_str());
	if (!property)
		return RequestResult::Error(RequestStatus::ResourceNotFound, "Unable to find a property by that name.");
	if (obs_property_get_type(property) != OBS_PROPERTY_BUTTON)
		return RequestResult::Error(RequestStatus::InvalidResourceType, "The property found is not a button.");
	if (!obs_property_enabled(property))
		return RequestResult::Error(RequestStatus::InvalidResourceState, "The property item found is not enabled.");

	obs_property_button_clicked(property, input);

	return RequestResult::Success();
}
