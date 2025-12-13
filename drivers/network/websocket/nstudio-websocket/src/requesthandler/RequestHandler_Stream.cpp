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
 * Gets the status of the stream output.
 *
 * @responseField outputActive        | Boolean | Whether the output is active
 * @responseField outputReconnecting  | Boolean | Whether the output is currently reconnecting
 * @responseField outputTimecode      | String  | Current formatted timecode string for the output
 * @responseField outputDuration      | Number  | Current duration in milliseconds for the output
 * @responseField outputCongestion    | Number  | Congestion of the output
 * @responseField outputBytes         | Number  | Number of bytes sent by the output
 * @responseField outputSkippedFrames | Number  | Number of frames skipped by the output's process
 * @responseField outputTotalFrames   | Number  | Total number of frames delivered by the output's process
 *
 * @requestType GetStreamStatus
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category stream
 */
RequestResult RequestHandler::GetStreamStatus(const Request &)
{
	OBSOutputAutoRelease streamOutput = obs_frontend_get_streaming_output();

	uint64_t outputDuration = Utils::Obs::NumberHelper::GetOutputDuration(streamOutput);

	float outputCongestion = obs_output_get_congestion(streamOutput);
	if (std::isnan(outputCongestion)) // libobs does not handle NaN, so we're handling it here
		outputCongestion = 0.0f;

	json responseData;
	responseData["outputActive"] = obs_output_active(streamOutput);
	responseData["outputReconnecting"] = obs_output_reconnecting(streamOutput);
	responseData["outputTimecode"] = Utils::Obs::StringHelper::DurationToTimecode(outputDuration);
	responseData["outputDuration"] = outputDuration;
	responseData["outputCongestion"] = outputCongestion;
	responseData["outputBytes"] = (uint64_t)obs_output_get_total_bytes(streamOutput);
	responseData["outputSkippedFrames"] = obs_output_get_frames_dropped(streamOutput);
	responseData["outputTotalFrames"] = obs_output_get_total_frames(streamOutput);

	return RequestResult::Success(responseData);
}

/**
 * Toggles the status of the stream output.
 *
 * @responseField outputActive | Boolean | New state of the stream output
 *
 * @requestType ToggleStream
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category stream
 */
RequestResult RequestHandler::ToggleStream(const Request &)
{
	json responseData;
	if (obs_frontend_streaming_active()) {
		obs_frontend_streaming_stop();
		responseData["outputActive"] = false;
	} else {
		obs_frontend_streaming_start();
		responseData["outputActive"] = true;
	}

	return RequestResult::Success(responseData);
}

/**
 * Starts the stream output.
 *
 * @requestType StartStream
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category stream
 */
RequestResult RequestHandler::StartStream(const Request &)
{
	if (obs_frontend_streaming_active())
		return RequestResult::Error(RequestStatus::OutputRunning);

	// TODO: Call signal directly to perform blocking wait
	obs_frontend_streaming_start();

	return RequestResult::Success();
}

/**
 * Stops the stream output.
 *
 * @requestType StopStream
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category stream
 */
RequestResult RequestHandler::StopStream(const Request &)
{
	if (!obs_frontend_streaming_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	// TODO: Call signal directly to perform blocking wait
	obs_frontend_streaming_stop();

	return RequestResult::Success();
}

/**
 * Sends CEA-608 caption text over the stream output.
 *
 * @requestField captionText | String | Caption text
 *
 * @requestType SendStreamCaption
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category stream
 * @api requests
 */
RequestResult RequestHandler::SendStreamCaption(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("captionText", statusCode, comment, true))
		return RequestResult::Error(statusCode, comment);

	if (!obs_frontend_streaming_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	std::string captionText = request.RequestData["captionText"];

	OBSOutputAutoRelease output = obs_frontend_get_streaming_output();

	// 0.0 means no delay until the next caption can be sent
	obs_output_output_caption_text2(output, captionText.c_str(), 0.0);

	return RequestResult::Success();
}
