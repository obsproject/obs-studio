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

#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <sstream>

#include "RequestHandler.h"

/**
 * Gets whether studio is enabled.
 *
 * @responseField studioModeEnabled | Boolean | Whether studio mode is enabled
 *
 * @requestType GetStudioModeEnabled
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api requests
 */
RequestResult RequestHandler::GetStudioModeEnabled(const Request &)
{
	json responseData;
	responseData["studioModeEnabled"] = obs_frontend_preview_program_mode_active();
	return RequestResult::Success(responseData);
}

/**
 * Enables or disables studio mode
 *
 * @requestField studioModeEnabled | Boolean | True == Enabled, False == Disabled
 *
 * @requestType SetStudioModeEnabled
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api requests
 */
RequestResult RequestHandler::SetStudioModeEnabled(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateBoolean("studioModeEnabled", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	// Avoid queueing tasks if nothing will change
	if (obs_frontend_preview_program_mode_active() != request.RequestData["studioModeEnabled"]) {
		// (Bad) Create a boolean then pass it as a reference to the task. Requires `wait` in obs_queue_task() to be true, else undefined behavior
		bool studioModeEnabled = request.RequestData["studioModeEnabled"];
		// Queue the task inside of the UI thread to prevent race conditions
		obs_queue_task(
			OBS_TASK_UI,
			[](void *param) {
				auto studioModeEnabled = (bool *)param;
				obs_frontend_set_preview_program_mode(*studioModeEnabled);
			},
			&studioModeEnabled, true);
	}

	return RequestResult::Success();
}

/**
 * Opens the properties dialog of an input.
 *
 * @requestField ?inputName | String | Name of the input to open the dialog of
 * @requestField ?inputUuid | String | UUID of the input to open the dialog of
 *
 * @requestType OpenInputPropertiesDialog
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api requests
 */
RequestResult RequestHandler::OpenInputPropertiesDialog(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	obs_frontend_open_source_properties(input);

	return RequestResult::Success();
}

/**
 * Opens the filters dialog of an input.
 *
 * @requestField ?inputName | String | Name of the input to open the dialog of
 * @requestField ?inputUuid | String | UUID of the input to open the dialog of
 *
 * @requestType OpenInputFiltersDialog
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api requests
 */
RequestResult RequestHandler::OpenInputFiltersDialog(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	obs_frontend_open_source_filters(input);

	return RequestResult::Success();
}

/**
 * Opens the interact dialog of an input.
 *
 * @requestField ?inputName | String | Name of the input to open the dialog of
 * @requestField ?inputUuid | String | UUID of the input to open the dialog of
 *
 * @requestType OpenInputInteractDialog
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api requests
 */
RequestResult RequestHandler::OpenInputInteractDialog(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	if (!(obs_source_get_output_flags(input) & OBS_SOURCE_INTERACTION))
		return RequestResult::Error(RequestStatus::InvalidResourceState,
					    "The specified input does not support interaction.");

	obs_frontend_open_source_interaction(input);

	return RequestResult::Success();
}

/**
 * Gets a list of connected monitors and information about them.
 *
 * @responseField monitors | Array<Object> | a list of detected monitors with some information
 *
 * @requestType GetMonitorList
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api requests
 */
RequestResult RequestHandler::GetMonitorList(const Request &)
{
	json responseData;
	std::vector<json> monitorsData;
	QList<QScreen *> screensList = QGuiApplication::screens();
	for (int screenIndex = 0; screenIndex < screensList.size(); screenIndex++) {
		json screenData;
		QScreen const *screen = screensList[screenIndex];
		std::stringstream nameAndIndex;
		nameAndIndex << screen->name().toStdString();
		nameAndIndex << '(' << screenIndex << ')';
		screenData["monitorName"] = nameAndIndex.str();
		screenData["monitorIndex"] = screenIndex;
		const QRect screenGeometry = screen->geometry();
		screenData["monitorWidth"] = screenGeometry.width();
		screenData["monitorHeight"] = screenGeometry.height();
		screenData["monitorPositionX"] = screenGeometry.x();
		screenData["monitorPositionY"] = screenGeometry.y();
		monitorsData.push_back(screenData);
	}
	responseData["monitors"] = monitorsData;
	return RequestResult::Success(responseData);
}

/**
 * Opens a projector for a specific output video mix.
 *
 * Mix types:
 *
 * - `OBS_WEBSOCKET_VIDEO_MIX_TYPE_PREVIEW`
 * - `OBS_WEBSOCKET_VIDEO_MIX_TYPE_PROGRAM`
 * - `OBS_WEBSOCKET_VIDEO_MIX_TYPE_MULTIVIEW`
 *
 * Note: This request serves to provide feature parity with 4.x. It is very likely to be changed/deprecated in a future release.
 *
 * @requestField videoMixType            | String | Type of mix to open
 * @requestField ?monitorIndex      | Number | Monitor index, use `GetMonitorList` to obtain index | None | -1: Opens projector in windowed mode
 * @requestField ?projectorGeometry | String | Size/Position data for a windowed projector, in Qt Base64 encoded format. Mutually exclusive with `monitorIndex` | N/A
 *
 * @requestType OpenVideoMixProjector
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api requests
 */
RequestResult RequestHandler::OpenVideoMixProjector(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("videoMixType", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string videoMixType = request.RequestData["videoMixType"];
	const char *projectorType;
	if (videoMixType == "OBS_WEBSOCKET_VIDEO_MIX_TYPE_PREVIEW")
		projectorType = "Preview";
	else if (videoMixType == "OBS_WEBSOCKET_VIDEO_MIX_TYPE_PROGRAM")
		projectorType = "StudioProgram";
	else if (videoMixType == "OBS_WEBSOCKET_VIDEO_MIX_TYPE_MULTIVIEW")
		projectorType = "Multiview";
	else
		return RequestResult::Error(RequestStatus::InvalidRequestField,
					    "The field `videoMixType` has an invalid enum value.");

	int monitorIndex = -1;
	if (request.Contains("monitorIndex")) {
		if (!request.ValidateOptionalNumber("monitorIndex", statusCode, comment, -1, 9))
			return RequestResult::Error(statusCode, comment);
		monitorIndex = request.RequestData["monitorIndex"];
	}

	std::string projectorGeometry;
	if (request.Contains("projectorGeometry")) {
		if (!request.ValidateOptionalString("projectorGeometry", statusCode, comment))
			return RequestResult::Error(statusCode, comment);
		if (monitorIndex != -1)
			return RequestResult::Error(RequestStatus::TooManyRequestFields,
						    "`monitorIndex` and `projectorGeometry` are mutually exclusive.");
		projectorGeometry = request.RequestData["projectorGeometry"];
	}

	obs_frontend_open_projector(projectorType, monitorIndex, projectorGeometry.c_str(), nullptr);

	return RequestResult::Success();
}

/**
 * Opens a projector for a source.
 *
 * Note: This request serves to provide feature parity with 4.x. It is very likely to be changed/deprecated in a future release.
 *
 * @requestField ?sourceName        | String | Name of the source to open a projector for
 * @requestField ?sourceUuid        | String | UUID of the source to open a projector for
 * @requestField ?monitorIndex      | Number | Monitor index, use `GetMonitorList` to obtain index | None | -1: Opens projector in windowed mode
 * @requestField ?projectorGeometry | String | Size/Position data for a windowed projector, in Qt Base64 encoded format. Mutually exclusive with `monitorIndex` | N/A
 *
 * @requestType OpenSourceProjector
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category ui
 * @api requests
 */
RequestResult RequestHandler::OpenSourceProjector(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease source = request.ValidateSource("sourceName", "sourceUuid", statusCode, comment);
	if (!source)
		return RequestResult::Error(statusCode, comment);

	int monitorIndex = -1;
	if (request.Contains("monitorIndex")) {
		if (!request.ValidateOptionalNumber("monitorIndex", statusCode, comment, -1, 9))
			return RequestResult::Error(statusCode, comment);
		monitorIndex = request.RequestData["monitorIndex"];
	}

	std::string projectorGeometry;
	if (request.Contains("projectorGeometry")) {
		if (!request.ValidateOptionalString("projectorGeometry", statusCode, comment))
			return RequestResult::Error(statusCode, comment);
		if (monitorIndex != -1)
			return RequestResult::Error(RequestStatus::TooManyRequestFields,
						    "`monitorIndex` and `projectorGeometry` are mutually exclusive.");
		projectorGeometry = request.RequestData["projectorGeometry"];
	}

	obs_frontend_open_projector("Source", monitorIndex, projectorGeometry.c_str(), obs_source_get_name(source));

	return RequestResult::Success();
}
