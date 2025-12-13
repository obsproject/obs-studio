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

#include "EventHandler.h"

void EventHandler::FilterAddMultiHandler(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	obs_source_t *filter = GetCalldataPointer<obs_source_t>(data, "filter");

	if (!(source && filter))
		return;

	eventHandler->ConnectSourceSignals(filter);

	eventHandler->HandleSourceFilterCreated(source, filter);
}

void EventHandler::FilterRemoveMultiHandler(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	obs_source_t *filter = GetCalldataPointer<obs_source_t>(data, "filter");

	if (!(source && filter))
		return;

	eventHandler->DisconnectSourceSignals(filter);

	eventHandler->HandleSourceFilterRemoved(source, filter);
}

/**
 * A source's filter list has been reindexed.
 *
 * @dataField sourceName | String        | Name of the source
 * @dataField filters    | Array<Object> | Array of filter objects
 *
 * @eventType SourceFilterListReindexed
 * @eventSubscription Filters
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category filters
 */
void EventHandler::HandleSourceFilterListReindexed(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	json eventData;
	eventData["sourceName"] = obs_source_get_name(source);
	eventData["filters"] = Utils::Obs::ArrayHelper::GetSourceFilterList(source);
	eventHandler->BroadcastEvent(EventSubscription::Filters, "SourceFilterListReindexed", eventData);
}

/**
 * A filter has been added to a source.
 *
 * @dataField sourceName            | String | Name of the source the filter was added to
 * @dataField filterName            | String | Name of the filter
 * @dataField filterKind            | String | The kind of the filter
 * @dataField filterIndex           | Number | Index position of the filter
 * @dataField filterSettings        | Object | The settings configured to the filter when it was created
 * @dataField defaultFilterSettings | Object | The default settings for the filter
 *
 * @eventType SourceFilterCreated
 * @eventSubscription Filters
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category filters
 */
void EventHandler::HandleSourceFilterCreated(obs_source_t *source, obs_source_t *filter)
{
	std::string filterKind = obs_source_get_id(filter);
	OBSDataAutoRelease filterSettings = obs_source_get_settings(filter);
	OBSDataAutoRelease defaultFilterSettings = obs_get_source_defaults(filterKind.c_str());

	json eventData;
	eventData["sourceName"] = obs_source_get_name(source);
	eventData["filterName"] = obs_source_get_name(filter);
	eventData["filterKind"] = filterKind;
	eventData["filterIndex"] = Utils::Obs::NumberHelper::GetSourceFilterIndex(source, filter);
	eventData["filterSettings"] = Utils::Json::ObsDataToJson(filterSettings);
	eventData["defaultFilterSettings"] = Utils::Json::ObsDataToJson(defaultFilterSettings, true);
	BroadcastEvent(EventSubscription::Filters, "SourceFilterCreated", eventData);
}

/**
 * A filter has been removed from a source.
 *
 * @dataField sourceName | String | Name of the source the filter was on
 * @dataField filterName | String | Name of the filter
 *
 * @eventType SourceFilterRemoved
 * @eventSubscription Filters
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category filters
 */
void EventHandler::HandleSourceFilterRemoved(obs_source_t *source, obs_source_t *filter)
{
	json eventData;
	eventData["sourceName"] = obs_source_get_name(source);
	eventData["filterName"] = obs_source_get_name(filter);
	BroadcastEvent(EventSubscription::Filters, "SourceFilterRemoved", eventData);
}

/**
 * The name of a source filter has changed.
 *
 * @dataField sourceName    | String | The source the filter is on
 * @dataField oldFilterName | String | Old name of the filter
 * @dataField filterName    | String | New name of the filter
 *
 * @eventType SourceFilterNameChanged
 * @eventSubscription Filters
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category filters
 */
void EventHandler::HandleSourceFilterNameChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *filter = GetCalldataPointer<obs_source_t>(data, "source");
	if (!filter)
		return;

	json eventData;
	eventData["sourceName"] = obs_source_get_name(obs_filter_get_parent(filter));
	eventData["oldFilterName"] = calldata_string(data, "prev_name");
	eventData["filterName"] = calldata_string(data, "new_name");
	eventHandler->BroadcastEvent(EventSubscription::Filters, "SourceFilterNameChanged", eventData);
}

/**
 * An source filter's settings have changed (been updated).
 *
 * @dataField sourceName     | String | Name of the source the filter is on
 * @dataField filterName     | String | Name of the filter
 * @dataField filterSettings | Object | New settings object of the filter
 *
 * @eventType SourceFilterSettingsChanged
 * @eventSubscription Filters
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.4.0
 * @api events
 * @category filters
 */
void EventHandler::HandleSourceFilterSettingsChanged(obs_source_t *source)
{
	OBSDataAutoRelease filterSettings = obs_source_get_settings(source);

	json eventData;
	eventData["sourceName"] = obs_source_get_name(obs_filter_get_parent(source));
	eventData["filterName"] = obs_source_get_name(source);
	eventData["filterSettings"] = Utils::Json::ObsDataToJson(filterSettings);
	BroadcastEvent(EventSubscription::Filters, "SourceFilterSettingsChanged", eventData);
}

/**
 * A source filter's enable state has changed.
 *
 * @dataField sourceName    | String  | Name of the source the filter is on
 * @dataField filterName    | String  | Name of the filter
 * @dataField filterEnabled | Boolean | Whether the filter is enabled
 *
 * @eventType SourceFilterEnableStateChanged
 * @eventSubscription Filters
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category filters
 */
void EventHandler::HandleSourceFilterEnableStateChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *filter = GetCalldataPointer<obs_source_t>(data, "source");
	if (!filter)
		return;

	// Not OBSSourceAutoRelease as get_parent doesn't increment refcount
	obs_source_t *source = obs_filter_get_parent(filter);
	if (!source)
		return;

	bool filterEnabled = calldata_bool(data, "enabled");

	json eventData;
	eventData["sourceName"] = obs_source_get_name(source);
	eventData["filterName"] = obs_source_get_name(filter);
	eventData["filterEnabled"] = filterEnabled;
	eventHandler->BroadcastEvent(EventSubscription::Filters, "SourceFilterEnableStateChanged", eventData);
}
