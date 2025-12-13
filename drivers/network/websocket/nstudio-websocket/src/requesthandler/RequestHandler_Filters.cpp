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
 * Gets an array of all available source filter kinds.
 *
 * Similar to `GetInputKindList`
 *
 * @responseField sourceFilterKinds | Array<String> | Array of source filter kinds
 *
 * @requestType GetSourceFilterKindList
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.4.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::GetSourceFilterKindList(const Request &)
{
	json responseData;
	responseData["sourceFilterKinds"] = Utils::Obs::ArrayHelper::GetFilterKindList();
	return RequestResult::Success(responseData);
}

/**
 * Gets an array of all of a source's filters.
 *
 * @requestField ?sourceName | String | Name of the source
 * @requestField ?sourceUuid | String | UUID of the source
 *
 * @responseField filters | Array<Object> | Array of filters
 *
 * @requestType GetSourceFilterList
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::GetSourceFilterList(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	OBSSourceAutoRelease source = request.ValidateSource("sourceName", "sourceUuid", statusCode, comment);
	if (!source)
		return RequestResult::Error(statusCode, comment);

	json responseData;
	responseData["filters"] = Utils::Obs::ArrayHelper::GetSourceFilterList(source);

	return RequestResult::Success(responseData);
}

/**
 * Gets the default settings for a filter kind.
 *
 * @requestField filterKind | String | Filter kind to get the default settings for
 *
 * @responseField defaultFilterSettings | Object | Object of default settings for the filter kind
 *
 * @requestType GetSourceFilterDefaultSettings
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::GetSourceFilterDefaultSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	if (!request.ValidateString("filterKind", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string filterKind = request.RequestData["filterKind"];
	auto kinds = Utils::Obs::ArrayHelper::GetFilterKindList();
	if (std::find(kinds.begin(), kinds.end(), filterKind) == kinds.end())
		return RequestResult::Error(RequestStatus::InvalidFilterKind);

	OBSDataAutoRelease defaultSettings = obs_get_source_defaults(filterKind.c_str());
	if (!defaultSettings)
		return RequestResult::Error(RequestStatus::InvalidFilterKind);

	json responseData;
	responseData["defaultFilterSettings"] = Utils::Json::ObsDataToJson(defaultSettings, true);
	return RequestResult::Success(responseData);
}

/**
 * Creates a new filter, adding it to the specified source.
 *
 * @requestField ?sourceName     | String | Name of the source to add the filter to
 * @requestField ?sourceUuid     | String | UUID of the source to add the filter to
 * @requestField filterName      | String | Name of the new filter to be created
 * @requestField filterKind      | String | The kind of filter to be created
 * @requestField ?filterSettings | Object | Settings object to initialize the filter with | Default settings used
 *
 * @requestType CreateSourceFilter
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::CreateSourceFilter(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;

	OBSSourceAutoRelease source = request.ValidateSource("sourceName", "sourceUuid", statusCode, comment);
	if (!(source && request.ValidateString("filterName", statusCode, comment) &&
	      request.ValidateString("filterKind", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	std::string filterName = request.RequestData["filterName"];
	OBSSourceAutoRelease existingFilter = obs_source_get_filter_by_name(source, filterName.c_str());
	if (existingFilter)
		return RequestResult::Error(RequestStatus::ResourceAlreadyExists, "A filter already exists by that name.");

	std::string filterKind = request.RequestData["filterKind"];
	auto kinds = Utils::Obs::ArrayHelper::GetFilterKindList();
	if (std::find(kinds.begin(), kinds.end(), filterKind) == kinds.end())
		return RequestResult::Error(
			RequestStatus::InvalidFilterKind,
			"Your specified filter kind is not supported by OBS. Check that any necessary plugins are loaded.");

	OBSDataAutoRelease filterSettings = nullptr;
	if (request.Contains("filterSettings")) {
		if (!request.ValidateOptionalObject("filterSettings", statusCode, comment, true))
			return RequestResult::Error(statusCode, comment);

		filterSettings = Utils::Json::JsonToObsData(request.RequestData["filterSettings"]);
	}

	OBSSourceAutoRelease filter = Utils::Obs::ActionHelper::CreateSourceFilter(source, filterName, filterKind, filterSettings);

	if (!filter)
		return RequestResult::Error(RequestStatus::ResourceCreationFailed, "Creation of the filter failed.");

	return RequestResult::Success();
}

/**
 * Removes a filter from a source.
 *
 * @requestField ?sourceName | String | Name of the source the filter is on
 * @requestField ?sourceUuid | String | UUID of the source the filter is on
 * @requestField filterName  | String | Name of the filter to remove
 *
 * @requestType RemoveSourceFilter
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::RemoveSourceFilter(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	FilterPair pair = request.ValidateFilter(statusCode, comment);
	if (!pair.filter)
		return RequestResult::Error(statusCode, comment);

	obs_source_filter_remove(pair.source, pair.filter);

	return RequestResult::Success();
}

/**
 * Sets the name of a source filter (rename).
 *
 * @requestField ?sourceName   | String | Name of the source the filter is on
 * @requestField ?sourceUuid   | String | UUID of the source the filter is on
 * @requestField filterName    | String | Current name of the filter
 * @requestField newFilterName | String | New name for the filter
 *
 * @requestType SetSourceFilterName
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::SetSourceFilterName(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	FilterPair pair = request.ValidateFilter(statusCode, comment);
	if (!pair.filter || !request.ValidateString("newFilterName", statusCode, comment))
		return RequestResult::Error(statusCode, comment);

	std::string newFilterName = request.RequestData["newFilterName"];

	OBSSourceAutoRelease existingFilter = obs_source_get_filter_by_name(pair.source, newFilterName.c_str());
	if (existingFilter)
		return RequestResult::Error(RequestStatus::ResourceAlreadyExists, "A filter already exists by that new name.");

	obs_source_set_name(pair.filter, newFilterName.c_str());

	return RequestResult::Success();
}

/**
 * Gets the info for a specific source filter.
 *
 * @requestField ?sourceName | String | Name of the source
 * @requestField ?sourceUuid | String | UUID of the source
 * @requestField filterName  | String | Name of the filter
 *
 * @responseField filterEnabled  | Boolean | Whether the filter is enabled
 * @responseField filterIndex    | Number  | Index of the filter in the list, beginning at 0
 * @responseField filterKind     | String  | The kind of filter
 * @responseField filterSettings | Object  | Settings object associated with the filter
 *
 * @requestType GetSourceFilter
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::GetSourceFilter(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	FilterPair pair = request.ValidateFilter(statusCode, comment);
	if (!pair.filter)
		return RequestResult::Error(statusCode, comment);

	json responseData;
	responseData["filterEnabled"] = obs_source_enabled(pair.filter);
	responseData["filterIndex"] = Utils::Obs::NumberHelper::GetSourceFilterIndex(
		pair.source,
		pair.filter); // Todo: Use `GetSourceFilterlist` to select this filter maybe
	responseData["filterKind"] = obs_source_get_id(pair.filter);

	OBSDataAutoRelease filterSettings = obs_source_get_settings(pair.filter);
	responseData["filterSettings"] = Utils::Json::ObsDataToJson(filterSettings);

	return RequestResult::Success(responseData);
}

/**
 * Sets the index position of a filter on a source.
 *
 * @requestField ?sourceName | String | Name of the source the filter is on
 * @requestField ?sourceUuid | String | UUID of the source the filter is on
 * @requestField filterName  | String | Name of the filter
 * @requestField filterIndex | Number | New index position of the filter | >= 0
 *
 * @requestType SetSourceFilterIndex
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::SetSourceFilterIndex(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	FilterPair pair = request.ValidateFilter(statusCode, comment);
	if (!(pair.filter && request.ValidateNumber("filterIndex", statusCode, comment, 0, 8192)))
		return RequestResult::Error(statusCode, comment);

	int filterIndex = request.RequestData["filterIndex"];

	Utils::Obs::ActionHelper::SetSourceFilterIndex(pair.source, pair.filter, filterIndex);

	return RequestResult::Success();
}

/**
 * Sets the settings of a source filter.
 *
 * @requestField ?sourceName    | String  | Name of the source the filter is on
 * @requestField ?sourceUuid    | String  | UUID of the source the filter is on
 * @requestField filterName     | String  | Name of the filter to set the settings of
 * @requestField filterSettings | Object  | Object of settings to apply
 * @requestField ?overlay       | Boolean | True == apply the settings on top of existing ones, False == reset the input to its defaults, then apply settings. | true
 *
 * @requestType SetSourceFilterSettings
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::SetSourceFilterSettings(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	FilterPair pair = request.ValidateFilter(statusCode, comment);
	if (!(pair.filter && request.ValidateObject("filterSettings", statusCode, comment, true)))
		return RequestResult::Error(statusCode, comment);

	// Almost identical to SetInputSettings

	bool overlay = true;
	if (request.Contains("overlay")) {
		if (!request.ValidateOptionalBoolean("overlay", statusCode, comment))
			return RequestResult::Error(statusCode, comment);

		overlay = request.RequestData["overlay"];
	}

	OBSDataAutoRelease newSettings = Utils::Json::JsonToObsData(request.RequestData["filterSettings"]);
	if (!newSettings)
		return RequestResult::Error(RequestStatus::RequestProcessingFailed,
					    "An internal data conversion operation failed. Please report this!");

	if (overlay)
		obs_source_update(pair.filter, newSettings);
	else
		obs_source_reset_settings(pair.filter, newSettings);

	obs_source_update_properties(pair.filter);

	return RequestResult::Success();
}

/**
 * Sets the enable state of a source filter.
 *
 * @requestField ?sourceName   | String  | Name of the source the filter is on
 * @requestField ?sourceUuid   | String  | UUID of the source the filter is on
 * @requestField filterName    | String  | Name of the filter
 * @requestField filterEnabled | Boolean | New enable state of the filter
 *
 * @requestType SetSourceFilterEnabled
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api requests
 * @category filters
 */
RequestResult RequestHandler::SetSourceFilterEnabled(const Request &request)
{
	RequestStatus::RequestStatus statusCode;
	std::string comment;
	FilterPair pair = request.ValidateFilter(statusCode, comment);
	if (!(pair.filter && request.ValidateBoolean("filterEnabled", statusCode, comment)))
		return RequestResult::Error(statusCode, comment);

	bool filterEnabled = request.RequestData["filterEnabled"];

	obs_source_set_enabled(pair.filter, filterEnabled);

	return RequestResult::Success();
}
