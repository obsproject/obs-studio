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

static bool VirtualCamAvailable()
{
	OBSOutputAutoRelease output = obs_frontend_get_virtualcam_output();
	return output != nullptr;
}

static bool ReplayBufferAvailable()
{
	OBSOutputAutoRelease output = obs_frontend_get_replay_buffer_output();
	return output != nullptr;
}

/**
 * Gets the status of the virtualcam output.
 *
 * @responseField outputActive | Boolean | Whether the output is active
 *
 * @requestType GetVirtualCamStatus
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category outputs
 * @api requests
 */
RequestResult RequestHandler::GetVirtualCamStatus(const Request &)
{
	if (!VirtualCamAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "VirtualCam is not available.");

	json responseData;
	responseData["outputActive"] = obs_frontend_virtualcam_active();
	return RequestResult::Success(responseData);
}

/**
 * Toggles the state of the virtualcam output.
 *
 * @responseField outputActive | Boolean | Whether the output is active
 *
 * @requestType ToggleVirtualCam
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category outputs
 * @api requests
 */
RequestResult RequestHandler::ToggleVirtualCam(const Request &)
{
	if (!VirtualCamAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "VirtualCam is not available.");

	bool outputActive = obs_frontend_virtualcam_active();

	if (outputActive)
		obs_frontend_stop_virtualcam();
	else
		obs_frontend_start_virtualcam();

	json responseData;
	responseData["outputActive"] = !outputActive;
	return RequestResult::Success(responseData);
}

/**
 * Starts the virtualcam output.
 *
 * @requestType StartVirtualCam
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::StartVirtualCam(const Request &)
{
	if (!VirtualCamAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "VirtualCam is not available.");

	if (obs_frontend_virtualcam_active())
		return RequestResult::Error(RequestStatus::OutputRunning);

	obs_frontend_start_virtualcam();

	return RequestResult::Success();
}

/**
 * Stops the virtualcam output.
 *
 * @requestType StopVirtualCam
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::StopVirtualCam(const Request &)
{
	if (!VirtualCamAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "VirtualCam is not available.");

	if (!obs_frontend_virtualcam_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	obs_frontend_stop_virtualcam();

	return RequestResult::Success();
}

/**
 * Gets the status of the replay buffer output.
 *
 * @responseField outputActive | Boolean | Whether the output is active
 *
 * @requestType GetReplayBufferStatus
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category outputs
 * @api requests
 */
RequestResult RequestHandler::GetReplayBufferStatus(const Request &)
{
	if (!ReplayBufferAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "Replay buffer is not available.");

	json responseData;
	responseData["outputActive"] = obs_frontend_replay_buffer_active();
	return RequestResult::Success(responseData);
}

/**
 * Toggles the state of the replay buffer output.
 *
 * @responseField outputActive | Boolean | Whether the output is active
 *
 * @requestType ToggleReplayBuffer
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category outputs
 * @api requests
 */
RequestResult RequestHandler::ToggleReplayBuffer(const Request &)
{
	if (!ReplayBufferAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "Replay buffer is not available.");

	bool outputActive = obs_frontend_replay_buffer_active();

	if (outputActive)
		obs_frontend_replay_buffer_stop();
	else
		obs_frontend_replay_buffer_start();

	json responseData;
	responseData["outputActive"] = !outputActive;
	return RequestResult::Success(responseData);
}

/**
 * Starts the replay buffer output.
 *
 * @requestType StartReplayBuffer
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::StartReplayBuffer(const Request &)
{
	if (!ReplayBufferAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "Replay buffer is not available.");

	if (obs_frontend_replay_buffer_active())
		return RequestResult::Error(RequestStatus::OutputRunning);

	obs_frontend_replay_buffer_start();

	return RequestResult::Success();
}

/**
 * Stops the replay buffer output.
 *
 * @requestType StopReplayBuffer
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::StopReplayBuffer(const Request &)
{
	if (!ReplayBufferAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "Replay buffer is not available.");

	if (!obs_frontend_replay_buffer_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	obs_frontend_replay_buffer_stop();

	return RequestResult::Success();
}

/**
 * Saves the contents of the replay buffer output.
 *
 * @requestType SaveReplayBuffer
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::SaveReplayBuffer(const Request &)
{
	if (!ReplayBufferAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "Replay buffer is not available.");

	if (!obs_frontend_replay_buffer_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	obs_frontend_replay_buffer_save();

	return RequestResult::Success();
}

/**
 * Gets the filename of the last replay buffer save file.
 *
 * @responseField savedReplayPath | String | File path
 *
 * @requestType GetLastReplayBufferReplay
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::GetLastReplayBufferReplay(const Request &)
{
	if (!ReplayBufferAvailable())
		return RequestResult::Error(RequestStatus::InvalidResourceState, "Replay buffer is not available.");

	if (!obs_frontend_replay_buffer_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	json responseData;
	responseData["savedReplayPath"] = Utils::Obs::StringHelper::GetLastReplayBufferFileName();
	return RequestResult::Success(responseData);
}

/**
 * Gets the list of available outputs.
 * 
 * @responseField outputs | Array<Object> | Array of outputs
 *
 * @requestType GetOutputList
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::GetOutputList(const Request &)
{
	json responseData;
	responseData["outputs"] = Utils::Obs::ArrayHelper::GetOutputList();
	return RequestResult::Success(responseData);
}

/**
 * Gets the status of an output.
 *
 * @requestField outputName | String | Output name
 *
 * @responseField outputActive        | Boolean | Whether the output is active
 * @responseField outputReconnecting  | Boolean | Whether the output is reconnecting
 * @responseField outputTimecode      | String  | Current formatted timecode string for the output
 * @responseField outputDuration      | Number  | Current duration in milliseconds for the output
 * @responseField outputCongestion    | Number  | Congestion of the output
 * @responseField outputBytes         | Number  | Number of bytes sent by the output
 * @responseField outputSkippedFrames | Number  | Number of frames skipped by the output's process
 * @responseField outputTotalFrames   | Number  | Total number of frames delivered by the output's process
 *
 * @requestType GetOutputStatus
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::GetOutputStatus(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSOutputAutoRelease output = request.ValidateOutput("outputName", statusCode, comment);
	if (!output)
		return RequestResult::Error(statusCode, comment);

	uint64_t outputDuration = Utils::Obs::NumberHelper::GetOutputDuration(output);

	json responseData;
	responseData["outputActive"] = obs_output_active(output);
	responseData["outputReconnecting"] = obs_output_reconnecting(output);
	responseData["outputTimecode"] = Utils::Obs::StringHelper::DurationToTimecode(outputDuration);
	responseData["outputDuration"] = outputDuration;
	responseData["outputCongestion"] = obs_output_get_congestion(output);
	responseData["outputBytes"] = obs_output_get_total_bytes(output);
	responseData["outputSkippedFrames"] = obs_output_get_frames_dropped(output);
	responseData["outputTotalFrames"] = obs_output_get_total_frames(output);

	return RequestResult::Success(responseData);
}

/**
 * Toggles the status of an output.
 *
 * @requestField outputName | String | Output name
 *
 * @responseField outputActive | Boolean | Whether the output is active
 * 
 * @requestType ToggleOutput
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::ToggleOutput(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSOutputAutoRelease output = request.ValidateOutput("outputName", statusCode, comment);
	if (!output)
		return RequestResult::Error(statusCode, comment);

	bool outputActive = obs_output_active(output);
	if (outputActive)
		obs_output_stop(output);
	else
		obs_output_start(output);

	json responseData;
	responseData["outputActive"] = !outputActive;
	return RequestResult::Success(responseData);
}

/**
 * Starts an output.
 *
 * @requestField outputName | String | Output name
 *
 * @requestType StartOutput
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::StartOutput(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSOutputAutoRelease output = request.ValidateOutput("outputName", statusCode, comment);
	if (!output)
		return RequestResult::Error(statusCode, comment);

	if (obs_output_active(output))
		return RequestResult::Error(RequestStatus::OutputRunning);

	obs_output_start(output);

	return RequestResult::Success();
}

/**
 * Stops an output.
 *
 * @requestField outputName | String | Output name
 *
 * @requestType StopOutput
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::StopOutput(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSOutputAutoRelease output = request.ValidateOutput("outputName", statusCode, comment);
	if (!output)
		return RequestResult::Error(statusCode, comment);

	if (!obs_output_active(output))
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	obs_output_stop(output);

	return RequestResult::Success();
}

/**
 * Gets the settings of an output.
 *
 * @requestField outputName | String | Output name
 *
 * @responseField outputSettings | Object | Output settings
 *
 * @requestType GetOutputSettings
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::GetOutputSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSOutputAutoRelease output = request.ValidateOutput("outputName", statusCode, comment);
	if (!output)
		return RequestResult::Error(statusCode, comment);

	OBSDataAutoRelease outputSettings = obs_output_get_settings(output);

	json responseData;
	responseData["outputSettings"] = Utils::Json::ObsDataToJson(outputSettings);
	return RequestResult::Success(responseData);
}

/**
 * Sets the settings of an output.
 *
 * @requestField outputName     | String | Output name
 * @requestField outputSettings | Object | Output settings
 *
 * @requestType SetOutputSettings
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category outputs
 */
RequestResult RequestHandler::SetOutputSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSOutputAutoRelease output = request.ValidateOutput("outputName", statusCode, comment);
	if (!(output && request.ValidateObject("outputSettings", statusCode, comment, true)))
		return RequestResult::Error(statusCode, comment);

	OBSDataAutoRelease newSettings = Utils::Json::JsonToObsData(request.RequestData["outputSettings"]);
	if (!newSettings)
		// This should never happen
		return RequestResult::Error(RequestStatus::RequestProcessingFailed,
					    "An internal data conversion operation failed. Please report this!");

	obs_output_update(output, newSettings);

	return RequestResult::Success();
}
