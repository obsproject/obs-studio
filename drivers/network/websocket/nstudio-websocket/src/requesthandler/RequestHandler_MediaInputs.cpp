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

bool IsMediaTimeValid(obs_source_t *input)
{
	auto mediaState = obs_source_media_get_state(input);
	return mediaState == OBS_MEDIA_STATE_PLAYING || mediaState == OBS_MEDIA_STATE_PAUSED;
}

/**
 * Gets the status of a media input.
 *
 * Media States:
 *
 * - `OBS_MEDIA_STATE_NONE`
 * - `OBS_MEDIA_STATE_PLAYING`
 * - `OBS_MEDIA_STATE_OPENING`
 * - `OBS_MEDIA_STATE_BUFFERING`
 * - `OBS_MEDIA_STATE_PAUSED`
 * - `OBS_MEDIA_STATE_STOPPED`
 * - `OBS_MEDIA_STATE_ENDED`
 * - `OBS_MEDIA_STATE_ERROR`
 *
 * @requestField ?inputName | String | Name of the media input
 * @requestField ?inputUuid | String | UUID of the media input
 *
 * @responseField mediaState    | String | State of the media input
 * @responseField mediaDuration | Number | Total duration of the playing media in milliseconds. `null` if not playing
 * @responseField mediaCursor   | Number | Position of the cursor in milliseconds. `null` if not playing
 *
 * @requestType GetMediaInputStatus
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category media inputs
 */
RequestResult RequestHandler::GetMediaInputStatus(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!input)
		return RequestResult::Error(statusCode, comment);

	json responseData;
	responseData["mediaState"] = obs_source_media_get_state(input);
	;

	if (IsMediaTimeValid(input)) {
		responseData["mediaDuration"] = obs_source_media_get_duration(input);
		responseData["mediaCursor"] = obs_source_media_get_time(input);
	} else {
		responseData["mediaDuration"] = nullptr;
		responseData["mediaCursor"] = nullptr;
	}

	return RequestResult::Success(responseData);
}

/**
 * Sets the cursor position of a media input.
 *
 * This request does not perform bounds checking of the cursor position.
 *
 * @requestField ?inputName  | String | Name of the media input
 * @requestField ?inputUuid  | String | UUID of the media input
 * @requestField mediaCursor | Number | New cursor position to set | >= 0
 *
 * @requestType SetMediaInputCursor
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category media inputs
 */
RequestResult RequestHandler::SetMediaInputCursor(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateNumber("mediaCursor", statusCode, comment, 0)))
		return RequestResult::Error(statusCode, comment);

	if (!IsMediaTimeValid(input))
		return RequestResult::Error(RequestStatus::InvalidResourceState,
					    "The media input must be playing or paused in order to set the cursor position.");

	int64_t mediaCursor = request.RequestData["mediaCursor"];

	// Yes, we're setting the time without checking if it's valid. Can't baby everything.
	obs_source_media_set_time(input, mediaCursor);

	return RequestResult::Success();
}

/**
 * Offsets the current cursor position of a media input by the specified value.
 *
 * This request does not perform bounds checking of the cursor position.
 *
 * @requestField ?inputName        | String | Name of the media input
 * @requestField ?inputUuid        | String | UUID of the media input
 * @requestField mediaCursorOffset | Number | Value to offset the current cursor position by | None
 *
 * @requestType OffsetMediaInputCursor
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category media inputs
 */
RequestResult RequestHandler::OffsetMediaInputCursor(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateNumber("mediaCursorOffset", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	if (!IsMediaTimeValid(input))
		return RequestResult::Error(RequestStatus::InvalidResourceState,
					    "The media input must be playing or paused in order to set the cursor position.");

	int64_t mediaCursorOffset = request.RequestData["mediaCursorOffset"];
	int64_t mediaCursor = obs_source_media_get_time(input) + mediaCursorOffset;

	if (mediaCursor < 0)
		mediaCursor = 0;

	obs_source_media_set_time(input, mediaCursor);

	return RequestResult::Success();
}

/**
 * Triggers an action on a media input.
 *
 * @requestField ?inputName  | String | Name of the media input
 * @requestField ?inputUuid  | String | UUID of the media input
 * @requestField mediaAction | String | Identifier of the `ObsMediaInputAction` enum
 *
 * @requestType TriggerMediaInputAction
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category media inputs
 */
RequestResult RequestHandler::TriggerMediaInputAction(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease input = request.ValidateInput(statusCode, comment);
	if (!(input && request.ValidateString("mediaAction", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	enum ObsMediaInputAction mediaAction = request.RequestData["mediaAction"];

	switch (mediaAction) {
	default:
	case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NONE:
		return RequestResult::Error(RequestStatus::InvalidRequestField,
					    "You have specified an invalid media input action.");
	case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PLAY:
		// Shoutout to whoever implemented this API call like this
		obs_source_media_play_pause(input, false);
		break;
	case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PAUSE:
		obs_source_media_play_pause(input, true);
		break;
	case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP:
		obs_source_media_stop(input);
		break;
	case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART:
		// I'm only implementing this because I'm nice. I think its a really dumb action.
		obs_source_media_restart(input);
		break;
	case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_NEXT:
		obs_source_media_next(input);
		break;
	case OBS_WEBSOCKET_MEDIA_INPUT_ACTION_PREVIOUS:
		obs_source_media_previous(input);
		break;
	}

	return RequestResult::Success();
}
