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
 * Gets the status of the record output.
 *
 * @responseField outputActive        | Boolean | Whether the output is active
 * @responseField outputPaused        | Boolean | Whether the output is paused
 * @responseField outputTimecode      | String  | Current formatted timecode string for the output
 * @responseField outputDuration      | Number  | Current duration in milliseconds for the output
 * @responseField outputBytes         | Number  | Number of bytes sent by the output
 *
 * @requestType GetRecordStatus
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::GetRecordStatus(const Request &)
{
	OBSOutputAutoRelease recordOutput = obs_frontend_get_recording_output();

	uint64_t outputDuration = Utils::Obs::NumberHelper::GetOutputDuration(recordOutput);

	json responseData;
	responseData["outputActive"] = obs_output_active(recordOutput);
	responseData["outputPaused"] = obs_output_paused(recordOutput);
	responseData["outputTimecode"] = Utils::Obs::StringHelper::DurationToTimecode(outputDuration);
	responseData["outputDuration"] = outputDuration;
	responseData["outputBytes"] = (uint64_t)obs_output_get_total_bytes(recordOutput);

	return RequestResult::Success(responseData);
}

/**
 * Toggles the status of the record output.
 *
 * @responseField outputActive | Boolean | The new active state of the output
 *
 * @requestType ToggleRecord
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::ToggleRecord(const Request &)
{
	json responseData;
	if (obs_frontend_recording_active()) {
		obs_frontend_recording_stop();
		responseData["outputActive"] = false;
	} else {
		obs_frontend_recording_start();
		responseData["outputActive"] = true;
	}

	return RequestResult::Success(responseData);
}

/**
 * Starts the record output.
 *
 * @requestType StartRecord
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::StartRecord(const Request &)
{
	if (obs_frontend_recording_active())
		return RequestResult::Error(RequestStatus::OutputRunning);

	// TODO: Call signal directly to perform blocking wait
	obs_frontend_recording_start();

	return RequestResult::Success();
}

/**
 * Stops the record output.
 *
 * @responseField outputPath | String | File name for the saved recording
 *
 * @requestType StopRecord
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::StopRecord(const Request &)
{
	if (!obs_frontend_recording_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	// TODO: Call signal directly to perform blocking wait
	obs_frontend_recording_stop();

	json responseData;
	responseData["outputPath"] = Utils::Obs::StringHelper::GetLastRecordFileName();

	return RequestResult::Success(responseData);
}

/**
 * Toggles pause on the record output.
 *
 * @requestType ToggleRecordPause
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::ToggleRecordPause(const Request &)
{
	json responseData;
	if (obs_frontend_recording_paused()) {
		obs_frontend_recording_pause(false);
		responseData["outputPaused"] = false;
	} else {
		obs_frontend_recording_pause(true);
		responseData["outputPaused"] = true;
	}

	return RequestResult::Success(responseData);
}

/**
 * Pauses the record output.
 *
 * @requestType PauseRecord
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::PauseRecord(const Request &)
{
	if (obs_frontend_recording_paused())
		return RequestResult::Error(RequestStatus::OutputPaused);

	// TODO: Call signal directly to perform blocking wait
	obs_frontend_recording_pause(true);

	return RequestResult::Success();
}

/**
 * Resumes the record output.
 *
 * @requestType ResumeRecord
 * @complexity 1
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::ResumeRecord(const Request &)
{
	if (!obs_frontend_recording_paused())
		return RequestResult::Error(RequestStatus::OutputNotPaused);

	// TODO: Call signal directly to perform blocking wait
	obs_frontend_recording_pause(false);

	return RequestResult::Success();
}

/**
 * Splits the current file being recorded into a new file.
 *
 * @requestType SplitRecordFile
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.5.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::SplitRecordFile(const Request &)
{
	if (!obs_frontend_recording_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	if (!obs_frontend_recording_split_file())
		return RequestResult::Error(RequestStatus::RequestProcessingFailed,
					    "Verify that file splitting is enabled in the output settings.");

	return RequestResult::Success();
}

/**
 * Adds a new chapter marker to the file currently being recorded.
 *
 * Note: As of OBS 30.2.0, the only file format supporting this feature is Hybrid MP4.
 *
 * @requestField ?chapterName | String | Name of the new chapter
 *
 * @requestType CreateRecordChapter
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.5.0
 * @api requests
 * @category record
 */
RequestResult RequestHandler::CreateRecordChapter(const Request &request)
{
	std::string chapterName;
	if (request.Contains("chapterName")) {
		RequestStatus::RequestStatus statusCode;
		std::string comment;
		if (!request.ValidateOptionalString("chapterName", statusCode, comment))
			return RequestResult::Error(statusCode, comment);
		chapterName = request.RequestData["chapterName"];
	}

	if (!obs_frontend_recording_active())
		return RequestResult::Error(RequestStatus::OutputNotRunning);

	if (!obs_frontend_recording_add_chapter(chapterName.empty() ? nullptr : chapterName.c_str()))
		return RequestResult::Error(RequestStatus::RequestProcessingFailed,
					    "Verify that the output being used supports chapter markers.");

	return RequestResult::Success();
}
