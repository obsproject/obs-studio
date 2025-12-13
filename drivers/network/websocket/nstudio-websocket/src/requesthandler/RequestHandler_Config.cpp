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

#include <QMainWindow>
#include <util/config-file.h>

#include "RequestHandler.h"

#define GLOBAL_PERSISTENT_DATA_FILE_NAME "persistent_data.json"

/**
 * Gets the value of a "slot" from the selected persistent data realm.
 *
 * @requestField realm    | String | The data realm to select. `OBS_WEBSOCKET_DATA_REALM_GLOBAL` or `OBS_WEBSOCKET_DATA_REALM_PROFILE`
 * @requestField slotName | String | The name of the slot to retrieve data from
 *
 * @responseField slotValue | Any | Value associated with the slot. `null` if not set
 *
 * @requestType GetPersistentData
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::GetPersistentData(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!(request.ValidateString("realm", statusCode, comment) && request.ValidateString("slotName", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string realm = request.RequestData["realm"];
	std::string slotName = request.RequestData["slotName"];

	std::string persistentDataPath;
	if (realm == "OBS_WEBSOCKET_DATA_REALM_GLOBAL")
		persistentDataPath = Utils::Obs::StringHelper::GetModuleConfigPath(GLOBAL_PERSISTENT_DATA_FILE_NAME);
	else if (realm == "OBS_WEBSOCKET_DATA_REALM_PROFILE")
		persistentDataPath = Utils::Obs::StringHelper::GetCurrentProfilePath() + "/obsWebSocketPersistentData.json";
	else
		return RequestResult::Error(RequestStatus::ResourceNotFound,
					    "You have specified an invalid persistent data realm.");

	json responseData;
	json persistentData;
	if (Utils::Json::GetJsonFileContent(persistentDataPath, persistentData) && persistentData.contains(slotName))
		responseData["slotValue"] = persistentData[slotName];
	else
		responseData["slotValue"] = nullptr;

	return RequestResult::Success(responseData);
}

/**
 * Sets the value of a "slot" from the selected persistent data realm.
 *
 * @requestField realm     | String | The data realm to select. `OBS_WEBSOCKET_DATA_REALM_GLOBAL` or `OBS_WEBSOCKET_DATA_REALM_PROFILE`
 * @requestField slotName  | String | The name of the slot to retrieve data from
 * @requestField slotValue | Any    | The value to apply to the slot
 *
 * @requestType SetPersistentData
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::SetPersistentData(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!(request.ValidateString("realm", statusCode, comment) && request.ValidateString("slotName", statusCode, comment) &&
	      request.ValidateBasic("slotValue", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string realm = request.RequestData["realm"];
	std::string slotName = request.RequestData["slotName"];
	json slotValue = request.RequestData["slotValue"];

	std::string persistentDataPath;
	if (realm == "OBS_WEBSOCKET_DATA_REALM_GLOBAL")
		persistentDataPath = Utils::Obs::StringHelper::GetModuleConfigPath(GLOBAL_PERSISTENT_DATA_FILE_NAME);
	else if (realm == "OBS_WEBSOCKET_DATA_REALM_PROFILE")
		persistentDataPath = Utils::Obs::StringHelper::GetCurrentProfilePath() + "/obsWebSocketPersistentData.json";
	else
		return RequestResult::Error(RequestStatus::ResourceNotFound,
					    "You have specified an invalid persistent data realm.");

	json persistentData;
	Utils::Json::GetJsonFileContent(persistentDataPath, persistentData);
	persistentData[slotName] = slotValue;
	if (!Utils::Json::SetJsonFileContent(persistentDataPath, persistentData))
		return RequestResult::Error(RequestStatus::RequestProcessingFailed,
					    "Unable to write persistent data. No permissions?");

	return RequestResult::Success();
}

/**
 * Gets an array of all scene collections
 *
 * @responseField currentSceneCollectionName | String        | The name of the current scene collection
 * @responseField sceneCollections           | Array<String> | Array of all available scene collections
 *
 * @requestType GetSceneCollectionList
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::GetSceneCollectionList(const Request &)
{
	json responseData;
	responseData["currentSceneCollectionName"] = Utils::Obs::StringHelper::GetCurrentSceneCollection();
	responseData["sceneCollections"] = Utils::Obs::ArrayHelper::GetSceneCollectionList();
	return RequestResult::Success(responseData);
}

/**
 * Switches to a scene collection.
 *
 * Note: This will block until the collection has finished changing.
 *
 * @requestField sceneCollectionName | String | Name of the scene collection to switch to
 *
 * @requestType SetCurrentSceneCollection
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::SetCurrentSceneCollection(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("sceneCollectionName", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string sceneCollectionName = request.RequestData["sceneCollectionName"];

	auto sceneCollections = Utils::Obs::ArrayHelper::GetSceneCollectionList();
	if (std::find(sceneCollections.begin(), sceneCollections.end(), sceneCollectionName) == sceneCollections.end())
		return RequestResult::Error(RequestStatus::ResourceNotFound);

	std::string currentSceneCollectionName = Utils::Obs::StringHelper::GetCurrentSceneCollection();
	// Avoid queueing tasks if nothing will change
	if (currentSceneCollectionName != sceneCollectionName) {
		obs_queue_task(
			OBS_TASK_UI,
			[](void *param) { obs_frontend_set_current_scene_collection(static_cast<const char *>(param)); },
			(void *)sceneCollectionName.c_str(), true);
	}

	return RequestResult::Success();
}

/**
 * Creates a new scene collection, switching to it in the process.
 *
 * Note: This will block until the collection has finished changing.
 *
 * @requestField sceneCollectionName | String | Name for the new scene collection
 *
 * @requestType CreateSceneCollection
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::CreateSceneCollection(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("sceneCollectionName", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string sceneCollectionName = request.RequestData["sceneCollectionName"];

	auto sceneCollections = Utils::Obs::ArrayHelper::GetSceneCollectionList();
	if (std::find(sceneCollections.begin(), sceneCollections.end(), sceneCollectionName) != sceneCollections.end())
		return RequestResult::Error(RequestStatus::ResourceAlreadyExists);

	bool success = obs_frontend_add_scene_collection(sceneCollectionName.c_str());
	if (!success)
		return RequestResult::Error(RequestStatus::ResourceCreationFailed, "Failed to create the scene collection.");

	return RequestResult::Success();
}

/**
 * Gets an array of all profiles
 *
 * @responseField currentProfileName | String        | The name of the current profile
 * @responseField profiles           | Array<String> | Array of all available profiles
 *
 * @requestType GetProfileList
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::GetProfileList(const Request &)
{
	json responseData;
	responseData["currentProfileName"] = Utils::Obs::StringHelper::GetCurrentProfile();
	responseData["profiles"] = Utils::Obs::ArrayHelper::GetProfileList();
	return RequestResult::Success(responseData);
}

/**
 * Switches to a profile.
 *
 * @requestField profileName | String | Name of the profile to switch to
 *
 * @requestType SetCurrentProfile
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::SetCurrentProfile(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("profileName", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string profileName = request.RequestData["profileName"];

	auto profiles = Utils::Obs::ArrayHelper::GetProfileList();
	if (std::find(profiles.begin(), profiles.end(), profileName) == profiles.end())
		return RequestResult::Error(RequestStatus::ResourceNotFound);

	std::string currentProfileName = Utils::Obs::StringHelper::GetCurrentProfile();
	// Avoid queueing tasks if nothing will change
	if (currentProfileName != profileName) {
		obs_queue_task(
			OBS_TASK_UI, [](void *param) { obs_frontend_set_current_profile(static_cast<const char *>(param)); },
			(void *)profileName.c_str(), true);
	}

	return RequestResult::Success();
}

/**
 * Creates a new profile, switching to it in the process
 *
 * @requestField profileName | String | Name for the new profile
 *
 * @requestType CreateProfile
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::CreateProfile(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("profileName", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string profileName = request.RequestData["profileName"];

	auto profiles = Utils::Obs::ArrayHelper::GetProfileList();
	if (std::find(profiles.begin(), profiles.end(), profileName) != profiles.end())
		return RequestResult::Error(RequestStatus::ResourceAlreadyExists);

	obs_frontend_create_profile(profileName.c_str());

	return RequestResult::Success();
}

/**
 * Removes a profile. If the current profile is chosen, it will change to a different profile first.
 *
 * @requestField profileName | String | Name of the profile to remove
 *
 * @requestType RemoveProfile
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::RemoveProfile(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("profileName", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string profileName = request.RequestData["profileName"];

	auto profiles = Utils::Obs::ArrayHelper::GetProfileList();
	if (std::find(profiles.begin(), profiles.end(), profileName) == profiles.end())
		return RequestResult::Error(RequestStatus::ResourceNotFound);

	if (profiles.size() < 2)
		return RequestResult::Error(RequestStatus::NotEnoughResources);

	obs_frontend_delete_profile(profileName.c_str());

	return RequestResult::Success();
}

/**
 * Gets a parameter from the current profile's configuration.
 *
 * @requestField parameterCategory | String | Category of the parameter to get
 * @requestField parameterName     | String | Name of the parameter to get
 *
 * @responseField parameterValue        | String | Value associated with the parameter. `null` if not set and no default
 * @responseField defaultParameterValue | String | Default value associated with the parameter. `null` if no default
 *
 * @requestType GetProfileParameter
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::GetProfileParameter(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!(request.ValidateString("parameterCategory", statusCode, comment) &&
	      request.ValidateString("parameterName", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string parameterCategory = request.RequestData["parameterCategory"];
	std::string parameterName = request.RequestData["parameterName"];

	config_t *profile = obs_frontend_get_profile_config();

	if (!profile)
		blog(LOG_ERROR, "[RequestHandler::GetProfileParameter] Profile is invalid.");

	json responseData;
	if (config_has_default_value(profile, parameterCategory.c_str(), parameterName.c_str())) {
		responseData["parameterValue"] = config_get_string(profile, parameterCategory.c_str(), parameterName.c_str());
		responseData["defaultParameterValue"] =
			config_get_default_string(profile, parameterCategory.c_str(), parameterName.c_str());
	} else if (config_has_user_value(profile, parameterCategory.c_str(), parameterName.c_str())) {
		responseData["parameterValue"] = config_get_string(profile, parameterCategory.c_str(), parameterName.c_str());
		responseData["defaultParameterValue"] = nullptr;
	} else {
		responseData["parameterValue"] = nullptr;
		responseData["defaultParameterValue"] = nullptr;
	}

	return RequestResult::Success(responseData);
}

/**
 * Sets the value of a parameter in the current profile's configuration.
 *
 * @requestField parameterCategory | String | Category of the parameter to set
 * @requestField parameterName     | String | Name of the parameter to set
 * @requestField parameterValue    | String | Value of the parameter to set. Use `null` to delete
 *
 * @requestType SetProfileParameter
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::SetProfileParameter(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!(request.ValidateString("parameterCategory", statusCode, comment) &&
	      request.ValidateString("parameterName", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string parameterCategory = request.RequestData["parameterCategory"];
	std::string parameterName = request.RequestData["parameterName"];

	config_t *profile = obs_frontend_get_profile_config();

	// Using check helpers here would just make the logic more complicated
	if (!request.RequestData.contains("parameterValue") || request.RequestData["parameterValue"].is_null()) {
		if (!config_remove_value(profile, parameterCategory.c_str(), parameterName.c_str()))
			return RequestResult::Error(RequestStatus::ResourceNotFound,
						    "There are no existing instances of that profile parameter.");
	} else if (request.RequestData["parameterValue"].is_string()) {
		std::string parameterValue = request.RequestData["parameterValue"];
		config_set_string(profile, parameterCategory.c_str(), parameterName.c_str(), parameterValue.c_str());
	} else {
		return RequestResult::Error(RequestStatus::InvalidRequestFieldType, "The field `parameterValue` must be a string.");
	}

	config_save(profile);

	return RequestResult::Success();
}

/**
 * Gets the current video settings.
 *
 * Note: To get the true FPS value, divide the FPS numerator by the FPS denominator. Example: `60000/1001`
 *
 * @responseField fpsNumerator   | Number | Numerator of the fractional FPS value
 * @responseField fpsDenominator | Number | Denominator of the fractional FPS value
 * @responseField baseWidth      | Number | Width of the base (canvas) resolution in pixels
 * @responseField baseHeight     | Number | Height of the base (canvas) resolution in pixels
 * @responseField outputWidth    | Number | Width of the output resolution in pixels
 * @responseField outputHeight   | Number | Height of the output resolution in pixels
 *
 * @requestType GetVideoSettings
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::GetVideoSettings(const Request &)
{
	struct obs_video_info ovi;
	if (!obs_get_video_info(&ovi))
		return RequestResult::Error(RequestStatus::RequestProcessingFailed, "Unable to get internal OBS video info.");

	json responseData;
	responseData["fpsNumerator"] = ovi.fps_num;
	responseData["fpsDenominator"] = ovi.fps_den;
	responseData["baseWidth"] = ovi.base_width;
	responseData["baseHeight"] = ovi.base_height;
	responseData["outputWidth"] = ovi.output_width;
	responseData["outputHeight"] = ovi.output_height;

	return RequestResult::Success(responseData);
}

/**
 * Sets the current video settings.
 *
 * Note: Fields must be specified in pairs. For example, you cannot set only `baseWidth` without needing to specify `baseHeight`.
 *
 * @requestField ?fpsNumerator   | Number | Numerator of the fractional FPS value            | >= 1          | Not changed
 * @requestField ?fpsDenominator | Number | Denominator of the fractional FPS value          | >= 1          | Not changed
 * @requestField ?baseWidth      | Number | Width of the base (canvas) resolution in pixels  | >= 1, <= 4096 | Not changed
 * @requestField ?baseHeight     | Number | Height of the base (canvas) resolution in pixels | >= 1, <= 4096 | Not changed
 * @requestField ?outputWidth    | Number | Width of the output resolution in pixels         | >= 1, <= 4096 | Not changed
 * @requestField ?outputHeight   | Number | Height of the output resolution in pixels        | >= 1, <= 4096 | Not changed
 *
 * @requestType SetVideoSettings
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::SetVideoSettings(const Request &request)
{
	if (obs_video_active())
		return RequestResult::Error(RequestStatus::OutputRunning,
					    "Video settings cannot be changed while an output is active.");

	RequestStatus::RequestStatus statusCode = RequestStatus::NoError;
	std::string comment;
	bool changeFps = (request.Contains("fpsNumerator") && request.Contains("fpsDenominator"));
	if (changeFps && !(request.ValidateOptionalNumber("fpsNumerator", statusCode, comment, 1) &&
			   request.ValidateOptionalNumber("fpsDenominator", statusCode, comment, 1)))
		return RequestResult::Error(statusCode, comment);

	bool changeBaseRes = (request.Contains("baseWidth") && request.Contains("baseHeight"));
	if (changeBaseRes && !(request.ValidateOptionalNumber("baseWidth", statusCode, comment, 8, 4096) &&
			       request.ValidateOptionalNumber("baseHeight", statusCode, comment, 8, 4096)))
		return RequestResult::Error(statusCode, comment);

	bool changeOutputRes = (request.Contains("outputWidth") && request.Contains("outputHeight"));
	if (changeOutputRes && !(request.ValidateOptionalNumber("outputWidth", statusCode, comment, 8, 4096) &&
				 request.ValidateOptionalNumber("outputHeight", statusCode, comment, 8, 4096)))
		return RequestResult::Error(statusCode, comment);

	config_t *config = obs_frontend_get_profile_config();

	if (changeFps) {
		config_set_uint(config, "Video", "FPSType", 2);
		config_set_uint(config, "Video", "FPSNum", request.RequestData["fpsNumerator"]);
		config_set_uint(config, "Video", "FPSDen", request.RequestData["fpsDenominator"]);
	}

	if (changeBaseRes) {
		config_set_uint(config, "Video", "BaseCX", request.RequestData["baseWidth"]);
		config_set_uint(config, "Video", "BaseCY", request.RequestData["baseHeight"]);
	}

	if (changeOutputRes) {
		config_set_uint(config, "Video", "OutputCX", request.RequestData["outputWidth"]);
		config_set_uint(config, "Video", "OutputCY", request.RequestData["outputHeight"]);
	}

	if (changeFps || changeBaseRes || changeOutputRes) {
		config_save_safe(config, "tmp", nullptr);
		obs_frontend_reset_video();
		return RequestResult::Success();
	}

	return RequestResult::Error(RequestStatus::MissingRequestField, "You must specify at least one video-changing pair.");
}

/**
 * Gets the current stream service settings (stream destination).
 *
 * @responseField streamServiceType     | String | Stream service type, like `rtmp_custom` or `rtmp_common`
 * @responseField streamServiceSettings | Object | Stream service settings
 *
 * @requestType GetStreamServiceSettings
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::GetStreamServiceSettings(const Request &)
{
	json responseData;

	OBSService service = obs_frontend_get_streaming_service();
	responseData["streamServiceType"] = obs_service_get_type(service);
	OBSDataAutoRelease serviceSettings = obs_service_get_settings(service);
	responseData["streamServiceSettings"] = Utils::Json::ObsDataToJson(serviceSettings, true);

	return RequestResult::Success(responseData);
}

/**
 * Sets the current stream service settings (stream destination).
 *
 * Note: Simple RTMP settings can be set with type `rtmp_custom` and the settings fields `server` and `key`.
 *
 * @requestField streamServiceType     | String | Type of stream service to apply. Example: `rtmp_common` or `rtmp_custom`
 * @requestField streamServiceSettings | Object | Settings to apply to the service
 *
 * @requestType SetStreamServiceSettings
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category config
 * @api requests
 */
RequestResult RequestHandler::SetStreamServiceSettings(const Request &request)
{
	if (obs_frontend_streaming_active())
		return RequestResult::Error(RequestStatus::OutputRunning,
					    "You cannot change stream service settings while streaming.");

	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!(request.ValidateString("streamServiceType", statusCode, comment) &&
	      request.ValidateObject("streamServiceSettings", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	OBSService currentStreamService = obs_frontend_get_streaming_service();

	std::string streamServiceType = obs_service_get_type(currentStreamService);
	std::string requestedStreamServiceType = request.RequestData["streamServiceType"];
	OBSDataAutoRelease requestedStreamServiceSettings =
		Utils::Json::JsonToObsData(request.RequestData["streamServiceSettings"]);

	// Don't create a new service if the current service is the same type.
	if (streamServiceType == requestedStreamServiceType) {
		OBSDataAutoRelease currentStreamServiceSettings = obs_service_get_settings(currentStreamService);

		// TODO: Add `overlay` field
		OBSDataAutoRelease newStreamServiceSettings = obs_data_create();
		obs_data_apply(newStreamServiceSettings, currentStreamServiceSettings);
		obs_data_apply(newStreamServiceSettings, requestedStreamServiceSettings);

		obs_service_update(currentStreamService, newStreamServiceSettings);
	} else {
		OBSServiceAutoRelease newStreamService = obs_service_create(requestedStreamServiceType.c_str(),
									    "obs_websocket_custom_service",
									    requestedStreamServiceSettings, nullptr);
		// TODO: Check service type here, instead of relying on service creation to fail.
		if (!newStreamService)
			return RequestResult::Error(
				RequestStatus::ResourceCreationFailed,
				"Failed to create the stream service with the requested streamServiceType. It may be an invalid type.");

		obs_frontend_set_streaming_service(newStreamService);
	}

	obs_frontend_save_streaming_service();

	return RequestResult::Success();
}

/**
 * Gets the current directory that the record output is set to.
 *
 * @responseField recordDirectory | String | Output directory
 *
 * @requestType GetRecordDirectory
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category config
 */
RequestResult RequestHandler::GetRecordDirectory(const Request &)
{
	json responseData;
	responseData["recordDirectory"] = Utils::Obs::StringHelper::GetCurrentRecordOutputPath();

	return RequestResult::Success(responseData);
}

/**
 * Sets the current directory that the record output writes files to.
 *
 * @requestField recordDirectory | String | Output directory
 *
 * @requestType SetRecordDirectory
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.3.0
 * @api requests
 * @category config
 */
RequestResult RequestHandler::SetRecordDirectory(const Request &request)
{
	if (obs_frontend_recording_active())
		return RequestResult::Error(RequestStatus::OutputRunning);

	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("recordDirectory", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string recordDirectory = request.RequestData["recordDirectory"];

	config_t *config = obs_frontend_get_profile_config();
	config_set_string(config, "AdvOut", "RecFilePath", recordDirectory.c_str());
	config_set_string(config, "SimpleOutput", "FilePath", recordDirectory.c_str());
	config_save(config);

	return RequestResult::Success();
}
