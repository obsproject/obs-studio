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

#include "../types/RequestStatus.h"
#include "../types/RequestBatchExecutionType.h"
#include "../../utils/Json.h"

enum ObsWebSocketSceneFilter {
	OBS_WEBSOCKET_SCENE_FILTER_SCENE_ONLY,
	OBS_WEBSOCKET_SCENE_FILTER_GROUP_ONLY,
	OBS_WEBSOCKET_SCENE_FILTER_SCENE_OR_GROUP,
};

// We return filters as a pair because `obs_filter_get_parent()` is apparently volatile
struct FilterPair {
	OBSSourceAutoRelease source;
	OBSSourceAutoRelease filter;
};

struct Request {
	Request(const std::string &requestType, const json &requestData = nullptr,
		const RequestBatchExecutionType::RequestBatchExecutionType executionType = RequestBatchExecutionType::None);

	// Contains the key and is not null
	bool Contains(const std::string &keyName) const;

	bool ValidateBasic(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment) const;
	bool ValidateOptionalNumber(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment,
				    const double minValue = -INFINITY, const double maxValue = INFINITY) const;
	bool ValidateNumber(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment,
			    const double minValue = -INFINITY, const double maxValue = INFINITY) const;
	bool ValidateOptionalString(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment,
				    const bool allowEmpty = false) const;
	bool ValidateString(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment,
			    const bool allowEmpty = false) const;
	bool ValidateOptionalBoolean(const std::string &keyName, RequestStatus::RequestStatus &statusCode,
				     std::string &comment) const;
	bool ValidateBoolean(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment) const;
	bool ValidateOptionalObject(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment,
				    const bool allowEmpty = false) const;
	bool ValidateObject(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment,
			    const bool allowEmpty = false) const;
	bool ValidateOptionalArray(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment,
				   const bool allowEmpty = false) const;
	bool ValidateArray(const std::string &keyName, RequestStatus::RequestStatus &statusCode, std::string &comment,
			   const bool allowEmpty = false) const;

	// All return values have incremented refcounts
	obs_source_t *ValidateSource(const std::string &nameKeyName, const std::string &uuidKeyName,
				     RequestStatus::RequestStatus &statusCode, std::string &comment) const;
	obs_source_t *ValidateScene(RequestStatus::RequestStatus &statusCode, std::string &comment,
				    const ObsWebSocketSceneFilter filter = OBS_WEBSOCKET_SCENE_FILTER_SCENE_ONLY) const;
	obs_scene_t *ValidateScene2(RequestStatus::RequestStatus &statusCode, std::string &comment,
				    const ObsWebSocketSceneFilter filter = OBS_WEBSOCKET_SCENE_FILTER_SCENE_ONLY) const;
	obs_source_t *ValidateInput(RequestStatus::RequestStatus &statusCode, std::string &comment) const;
	FilterPair ValidateFilter(RequestStatus::RequestStatus &statusCode, std::string &comment) const;
	obs_sceneitem_t *ValidateSceneItem(RequestStatus::RequestStatus &statusCode, std::string &comment,
					   const ObsWebSocketSceneFilter filter = OBS_WEBSOCKET_SCENE_FILTER_SCENE_ONLY) const;
	obs_output_t *ValidateOutput(const std::string &keyName, RequestStatus::RequestStatus &statusCode,
				     std::string &comment) const;

	std::string RequestType;
	bool HasRequestData;
	json RequestData;
	RequestBatchExecutionType::RequestBatchExecutionType ExecutionType;
};
