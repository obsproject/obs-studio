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

#include <math.h>

#include "RequestHandler.h"

/**
 * Gets an array of all available transition kinds.
 *
 * Similar to `GetInputKindList`
 *
 * @responseField transitionKinds | Array<String> | Array of transition kinds
 *
 * @requestType GetTransitionKindList
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::GetTransitionKindList(const Request &)
{
	json responseData;
	responseData["transitionKinds"] = Utils::Obs::ArrayHelper::GetTransitionKindList();
	return RequestResult::Success(responseData);
}

/**
 * Gets an array of all scene transitions in OBS.
 *
 * @responseField currentSceneTransitionName | String         | Name of the current scene transition. Can be null
 * @responseField currentSceneTransitionUuid | String         | UUID of the current scene transition. Can be null
 * @responseField currentSceneTransitionKind | String         | Kind of the current scene transition. Can be null
 * @responseField transitions                | Array<Object> | Array of transitions
 *
 * @requestType GetSceneTransitionList
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::GetSceneTransitionList(const Request &)
{
	json responseData;

	OBSSourceAutoRelease transition = obs_frontend_get_current_transition();
	if (transition) {
		responseData["currentSceneTransitionName"] = obs_source_get_name(transition);
		responseData["currentSceneTransitionUuid"] = obs_source_get_uuid(transition);
		responseData["currentSceneTransitionKind"] = obs_source_get_id(transition);
	} else {
		responseData["currentSceneTransitionName"] = nullptr;
		responseData["currentSceneTransitionUuid"] = nullptr;
		responseData["currentSceneTransitionKind"] = nullptr;
	}

	responseData["transitions"] = Utils::Obs::ArrayHelper::GetSceneTransitionList();

	return RequestResult::Success(responseData);
}

/**
 * Gets information about the current scene transition.
 *
 * @responseField transitionName         | String  | Name of the transition
 * @responseField transitionUuid         | String  | UUID of the transition
 * @responseField transitionKind         | String  | Kind of the transition
 * @responseField transitionFixed        | Boolean | Whether the transition uses a fixed (unconfigurable) duration
 * @responseField transitionDuration     | Number  | Configured transition duration in milliseconds. `null` if transition is fixed
 * @responseField transitionConfigurable | Boolean | Whether the transition supports being configured
 * @responseField transitionSettings     | Object  | Object of settings for the transition. `null` if transition is not configurable
 *
 * @requestType GetCurrentSceneTransition
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::GetCurrentSceneTransition(const Request &)
{
	OBSSourceAutoRelease transition = obs_frontend_get_current_transition();
	if (!transition)
		return RequestResult::Error(RequestStatus::InvalidResourceState,
					    "OBS does not currently have a scene transition set."); // This should not happen!

	json responseData;
	responseData["transitionName"] = obs_source_get_name(transition);
	responseData["transitionUuid"] = obs_source_get_uuid(transition);
	responseData["transitionKind"] = obs_source_get_id(transition);

	if (obs_transition_fixed(transition)) {
		responseData["transitionFixed"] = true;
		responseData["transitionDuration"] = nullptr;
	} else {
		responseData["transitionFixed"] = false;
		responseData["transitionDuration"] = obs_frontend_get_transition_duration();
	}

	if (obs_source_configurable(transition)) {
		responseData["transitionConfigurable"] = true;
		OBSDataAutoRelease transitionSettings = obs_source_get_settings(transition);
		responseData["transitionSettings"] = Utils::Json::ObsDataToJson(transitionSettings);
	} else {
		responseData["transitionConfigurable"] = false;
		responseData["transitionSettings"] = nullptr;
	}

	return RequestResult::Success(responseData);
}

/**
 * Sets the current scene transition.
 *
 * Small note: While the namespace of scene transitions is generally unique, that uniqueness is not a guarantee as it is with other resources like inputs.
 *
 * @requestField transitionName | String | Name of the transition to make active
 *
 * @requestType SetCurrentSceneTransition
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::SetCurrentSceneTransition(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("transitionName", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string transitionName = request.RequestData["transitionName"];

	OBSSourceAutoRelease transition = Utils::Obs::SearchHelper::GetSceneTransitionByName(transitionName);
	if (!transition)
		return RequestResult::Error(RequestStatus::ResourceNotFound, "No scene transition was found by that name.");

	obs_frontend_set_current_transition(transition);

	return RequestResult::Success();
}

/**
 * Sets the duration of the current scene transition, if it is not fixed.
 *
 * @requestField transitionDuration | Number | Duration in milliseconds | >= 50, <= 20000
 *
 * @requestType SetCurrentSceneTransitionDuration
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::SetCurrentSceneTransitionDuration(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateNumber("transitionDuration", statusCode, comment, 50, 20000))
		return RequestResult::Error(statusCode, comment);

	int transitionDuration = request.RequestData["transitionDuration"];

	obs_frontend_set_transition_duration(transitionDuration);

	return RequestResult::Success();
}

/**
 * Sets the settings of the current scene transition.
 *
 * @requestField transitionSettings | Object  | Settings object to apply to the transition. Can be `{}`
 * @requestField ?overlay           | Boolean | Whether to overlay over the current settings or replace them | true
 *
 * @requestType SetCurrentSceneTransitionSettings
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::SetCurrentSceneTransitionSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateObject("transitionSettings", statusCode, comment, true))
		return RequestResult::Error(statusCode, comment);

	OBSSourceAutoRelease transition = obs_frontend_get_current_transition();
	if (!transition)
		return RequestResult::Error(RequestStatus::InvalidResourceState,
					    "OBS does not currently have a scene transition set."); // This should not happen!

	if (!obs_source_configurable(transition))
		return RequestResult::Error(RequestStatus::ResourceNotConfigurable,
					    "The current transition does not support custom settings.");

	bool overlay = true;
	if (request.Contains("overlay")) {
		if (!request.ValidateOptionalBoolean("overlay", statusCode, comment))
			return RequestResult::Error(statusCode, comment);

		overlay = request.RequestData["overlay"];
	}

	OBSDataAutoRelease newSettings = Utils::Json::JsonToObsData(request.RequestData["transitionSettings"]);
	if (!newSettings)
		return RequestResult::Error(RequestStatus::RequestProcessingFailed,
					    "An internal data conversion operation failed. Please report this!");

	if (overlay)
		obs_source_update(transition, newSettings);
	else
		obs_source_reset_settings(transition, newSettings);

	obs_source_update_properties(transition);

	return RequestResult::Success();
}

/**
 * Gets the cursor position of the current scene transition.
 *
 * Note: `transitionCursor` will return 1.0 when the transition is inactive.
 *
 * @responseField transitionCursor | Number | Cursor position, between 0.0 and 1.0
 *
 * @requestType GetCurrentSceneTransitionCursor
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::GetCurrentSceneTransitionCursor(const Request &)
{
	OBSSourceAutoRelease transition = obs_frontend_get_current_transition();
	if (!transition)
		return RequestResult::Error(RequestStatus::InvalidResourceState,
					    "OBS does not currently have a scene transition set."); // This should not happen!

	json responseData;
	responseData["transitionCursor"] = obs_transition_get_time(transition);

	return RequestResult::Success(responseData);
}

/**
 * Triggers the current scene transition. Same functionality as the `Transition` button in studio mode.
 *
 * @requestType TriggerStudioModeTransition
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::TriggerStudioModeTransition(const Request &)
{
	if (!obs_frontend_preview_program_mode_active())
		return RequestResult::Error(RequestStatus::StudioModeNotActive);

	OBSSourceAutoRelease previewScene = obs_frontend_get_current_preview_scene();

	obs_frontend_set_current_scene(previewScene);

	return RequestResult::Success();
}

/**
 * Sets the position of the TBar.
 *
 * **Very important note**: This will be deprecated and replaced in a future version of obs-websocket.
 *
 * @requestField position | Number  | New position | >= 0.0, <= 1.0
 * @requestField ?release | Boolean | Whether to release the TBar. Only set `false` if you know that you will be sending another position update | `true`
 *
 * @requestType SetTBarPosition
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category transitions
 */
RequestResult RequestHandler::SetTBarPosition(const Request &request)
{
	if (!obs_frontend_preview_program_mode_active())
		return RequestResult::Error(RequestStatus::StudioModeNotActive);

	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateNumber("position", statusCode, comment, 0.0, 1.0))
		return RequestResult::Error(statusCode, comment);

	bool release = true;
	if (request.Contains("release")) {
		if (!request.ValidateOptionalBoolean("release", statusCode, comment))
			return RequestResult::Error(statusCode, comment);
	}

	OBSSourceAutoRelease transition = obs_frontend_get_current_transition();
	if (!transition)
		return RequestResult::Error(RequestStatus::InvalidResourceState,
					    "OBS does not currently have a scene transition set."); // This should not happen!

	float position = request.RequestData["position"];

	obs_frontend_set_tbar_position((int)round(position * 1024.0));

	if (release)
		obs_frontend_release_tbar();

	return RequestResult::Success();
}
