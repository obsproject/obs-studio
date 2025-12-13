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
 * Gets an array of all scenes in OBS.
 *
 * @responseField currentProgramSceneName | String        | Current program scene name. Can be `null` if internal state desync
 * @responseField currentProgramSceneUuid | String        | Current program scene UUID. Can be `null` if internal state desync
 * @responseField currentPreviewSceneName | String        | Current preview scene name. `null` if not in studio mode
 * @responseField currentPreviewSceneUuid | String        | Current preview scene UUID. `null` if not in studio mode
 * @responseField scenes                  | Array<Object> | Array of scenes
 *
 * @requestType GetSceneList
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::GetSceneList(const Request &)
{
	json responseData;

	OBSSourceAutoRelease currentProgramScene = obs_frontend_get_current_scene();
	if (currentProgramScene) {
		responseData["currentProgramSceneName"] = obs_source_get_name(currentProgramScene);
		responseData["currentProgramSceneUuid"] = obs_source_get_uuid(currentProgramScene);
	} else {
		responseData["currentProgramSceneName"] = nullptr;
		responseData["currentProgramSceneUuid"] = nullptr;
	}

	OBSSourceAutoRelease currentPreviewScene = obs_frontend_get_current_preview_scene();
	if (currentPreviewScene) {
		responseData["currentPreviewSceneName"] = obs_source_get_name(currentPreviewScene);
		responseData["currentPreviewSceneUuid"] = obs_source_get_uuid(currentPreviewScene);
	} else {
		responseData["currentPreviewSceneName"] = nullptr;
		responseData["currentPreviewSceneUuid"] = nullptr;
	}

	responseData["scenes"] = Utils::Obs::ArrayHelper::GetSceneList();

	return RequestResult::Success(responseData);
}

/**
 * Gets an array of all groups in OBS.
 *
 * Groups in OBS are actually scenes, but renamed and modified. In obs-websocket, we treat them as scenes where we can.
 *
 * @responseField groups | Array<String> | Array of group names
 *
 * @requestType GetGroupList
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::GetGroupList(const Request &)
{
	json responseData;

	responseData["groups"] = Utils::Obs::ArrayHelper::GetGroupList();

	return RequestResult::Success(responseData);
}

/**
 * Gets the current program scene.
 *
 * Note: This request is slated to have the `currentProgram`-prefixed fields removed from in an upcoming RPC version.
 *
 * @responseField sceneName               | String | Current program scene name
 * @responseField sceneUuid               | String | Current program scene UUID
 * @responseField currentProgramSceneName | String | Current program scene name (Deprecated)
 * @responseField currentProgramSceneUuid | String | Current program scene UUID (Deprecated)
 *
 * @requestType GetCurrentProgramScene
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::GetCurrentProgramScene(const Request &)
{
	json responseData;
	OBSSourceAutoRelease currentProgramScene = obs_frontend_get_current_scene();
	responseData["sceneName"] = responseData["currentProgramSceneName"] = obs_source_get_name(currentProgramScene);
	responseData["sceneUuid"] = responseData["currentProgramSceneUuid"] = obs_source_get_uuid(currentProgramScene);

	return RequestResult::Success(responseData);
}

/**
 * Sets the current program scene.
 *
 * @requestField ?sceneName | String | Scene name to set as the current program scene
 * @requestField ?sceneUuid | String | Scene UUID to set as the current program scene
 *
 * @requestType SetCurrentProgramScene
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::SetCurrentProgramScene(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease scene = request.ValidateScene(statusCode, comment);
	if (!scene)
		return RequestResult::Error(statusCode, comment);

	obs_frontend_set_current_scene(scene);

	return RequestResult::Success();
}

/**
 * Gets the current preview scene.
 *
 * Only available when studio mode is enabled.
 *
 * Note: This request is slated to have the `currentPreview`-prefixed fields removed from in an upcoming RPC version.
 *
 * @responseField sceneName               | String | Current preview scene name
 * @responseField sceneUuid               | String | Current preview scene UUID
 * @responseField currentPreviewSceneName | String | Current preview scene name
 * @responseField currentPreviewSceneUuid | String | Current preview scene UUID
 *
 * @requestType GetCurrentPreviewScene
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::GetCurrentPreviewScene(const Request &)
{
	if (!obs_frontend_preview_program_mode_active())
		return RequestResult::Error(RequestStatus::StudioModeNotActive);

	OBSSourceAutoRelease currentPreviewScene = obs_frontend_get_current_preview_scene();

	json responseData;
	responseData["sceneName"] = responseData["currentPreviewSceneName"] = obs_source_get_name(currentPreviewScene);
	responseData["sceneUuid"] = responseData["currentPreviewSceneUuid"] = obs_source_get_uuid(currentPreviewScene);

	return RequestResult::Success(responseData);
}

/**
 * Sets the current preview scene.
 *
 * Only available when studio mode is enabled.
 *
 * @requestField ?sceneName | String | Scene name to set as the current preview scene
 * @requestField ?sceneUuid | String | Scene UUID to set as the current preview scene
 *
 * @requestType SetCurrentPreviewScene
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::SetCurrentPreviewScene(const Request &request)
{
	if (!obs_frontend_preview_program_mode_active())
		return RequestResult::Error(RequestStatus::StudioModeNotActive);

	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease scene = request.ValidateScene(statusCode, comment);
	if (!scene)
		return RequestResult::Error(statusCode, comment);

	obs_frontend_set_current_preview_scene(scene);

	return RequestResult::Success();
}

/**
 * Creates a new scene in OBS.
 *
 * @requestField sceneName | String | Name for the new scene
 *
 * @responseField sceneUuid | String | UUID of the created scene
 *
 * @requestType CreateScene
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::CreateScene(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("sceneName", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string sceneName = request.RequestData["sceneName"];

	OBSSourceAutoRelease scene = obs_get_source_by_name(sceneName.c_str());
	if (scene)
		return RequestResult::Error(RequestStatus::ResourceAlreadyExists, "A source already exists by that scene name.");

	OBSSceneAutoRelease createdScene = obs_scene_create(sceneName.c_str());
	if (!createdScene)
		return RequestResult::Error(RequestStatus::ResourceCreationFailed, "Failed to create the scene.");

	json responseData;
	responseData["sceneUuid"] = obs_source_get_uuid(obs_scene_get_source(createdScene));

	return RequestResult::Success(responseData);
}

/**
 * Removes a scene from OBS.
 *
 * @requestField ?sceneName | String | Name of the scene to remove
 * @requestField ?sceneUuid | String | UUID of the scene to remove
 *
 * @requestType RemoveScene
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::RemoveScene(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease scene = request.ValidateScene(statusCode, comment);
	if (!scene)
		return RequestResult::Error(statusCode, comment);

	if (Utils::Obs::NumberHelper::GetSceneCount() < 2)
		return RequestResult::Error(RequestStatus::NotEnoughResources,
					    "You cannot remove the last scene in the collection.");

	obs_source_remove(scene);

	return RequestResult::Success();
}

/**
 * Sets the name of a scene (rename).
 *
 * @requestField ?sceneName   | String | Name of the scene to be renamed
 * @requestField ?sceneUuid   | String | UUID of the scene to be renamed
 * @requestField newSceneName | String | New name for the scene
 *
 * @requestType SetSceneName
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::SetSceneName(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease scene = request.ValidateScene(statusCode, comment);
	if (!(scene && request.ValidateString("newSceneName", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string newSceneName = request.RequestData["newSceneName"];

	OBSSourceAutoRelease existingSource = obs_get_source_by_name(newSceneName.c_str());
	if (existingSource)
		return RequestResult::Error(RequestStatus::ResourceAlreadyExists,
					    "A source already exists by that new scene name.");

	obs_source_set_name(scene, newSceneName.c_str());

	return RequestResult::Success();
}

/**
 * Gets the scene transition overridden for a scene.
 *
 * Note: A transition UUID response field is not currently able to be implemented as of 2024-1-18.
 *
 * @requestField ?sceneName | String | Name of the scene
 * @requestField ?sceneUuid | String | UUID of the scene
 *
 * @responseField transitionName     | String | Name of the overridden scene transition, else `null`
 * @responseField transitionDuration | Number | Duration of the overridden scene transition, else `null`
 *
 * @requestType GetSceneSceneTransitionOverride
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::GetSceneSceneTransitionOverride(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease scene = request.ValidateScene(statusCode, comment);
	if (!scene)
		return RequestResult::Error(statusCode, comment);

	OBSDataAutoRelease privateSettings = obs_source_get_private_settings(scene);

	json responseData;
	const char *transitionName = obs_data_get_string(privateSettings, "transition");
	if (transitionName && strlen(transitionName))
		responseData["transitionName"] = transitionName;
	else
		responseData["transitionName"] = nullptr;

	if (obs_data_has_user_value(privateSettings, "transition_duration"))
		responseData["transitionDuration"] = obs_data_get_int(privateSettings, "transition_duration");
	else
		responseData["transitionDuration"] = nullptr;

	return RequestResult::Success(responseData);
}

/**
 * Sets the scene transition overridden for a scene.
 *
 * @requestField ?sceneName          | String | Name of the scene
 * @requestField ?sceneUuid          | String | UUID of the scene
 * @requestField ?transitionName     | String | Name of the scene transition to use as override. Specify `null` to remove | Unchanged
 * @requestField ?transitionDuration | Number | Duration to use for any overridden transition. Specify `null` to remove | >= 50, <= 20000 | Unchanged
 *
 * @requestType SetSceneSceneTransitionOverride
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category scenes
 */
RequestResult RequestHandler::SetSceneSceneTransitionOverride(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease scene = request.ValidateScene(statusCode, comment);
	if (!scene)
		return RequestResult::Error(statusCode, comment);

	OBSDataAutoRelease privateSettings = obs_source_get_private_settings(scene);

	bool hasName = request.RequestData.contains("transitionName");
	if (hasName && !request.RequestData["transitionName"].is_null()) {
		if (!request.ValidateOptionalString("transitionName", statusCode, comment))
			return RequestResult::Error(statusCode, comment);
		OBSSourceAutoRelease transition =
			Utils::Obs::SearchHelper::GetSceneTransitionByName(request.RequestData["transitionName"]);
		if (!transition)
			return RequestResult::Error(RequestStatus::ResourceNotFound, "No scene transition was found by that name.");
	}

	bool hasDuration = request.RequestData.contains("transitionDuration");
	if (hasDuration && !request.RequestData["transitionDuration"].is_null()) {
		if (!request.ValidateOptionalNumber("transitionDuration", statusCode, comment, 50, 20000))
			return RequestResult::Error(statusCode, comment);
	}

	if (!hasName && !hasDuration)
		return RequestResult::Error(RequestStatus::MissingRequestField,
					    "Your request data must include either `transitionName` or `transitionDuration`.");

	if (hasName) {
		if (request.RequestData["transitionName"].is_null()) {
			obs_data_erase(privateSettings, "transition");
		} else {
			std::string transitionName = request.RequestData["transitionName"];
			obs_data_set_string(privateSettings, "transition", transitionName.c_str());
		}
	}

	if (hasDuration) {
		if (request.RequestData["transitionDuration"].is_null()) {
			obs_data_erase(privateSettings, "transition_duration");
		} else {
			obs_data_set_int(privateSettings, "transition_duration", request.RequestData["transitionDuration"]);
		}
	}

	return RequestResult::Success();
}
