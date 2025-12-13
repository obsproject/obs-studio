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

/**
 * An input has been created.
 *
 * @dataField inputName            | String | Name of the input
 * @dataField inputUuid            | String | UUID of the input
 * @dataField inputKind            | String | The kind of the input
 * @dataField unversionedInputKind | String | The unversioned kind of input (aka no `_v2` stuff)
 * @dataField inputKindCaps        | Number | Bitflag value for the caps that an input supports. See obs_source_info.output_flags in the libobs docs
 * @dataField inputSettings        | Object | The settings configured to the input when it was created
 * @dataField defaultInputSettings | Object | The default settings for the input
 *
 * @eventType InputCreated
 * @eventSubscription Inputs
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputCreated(obs_source_t *source)
{
	std::string inputKind = obs_source_get_id(source);
	OBSDataAutoRelease inputSettings = obs_source_get_settings(source);
	OBSDataAutoRelease defaultInputSettings = obs_get_source_defaults(inputKind.c_str());

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["inputKind"] = inputKind;
	eventData["unversionedInputKind"] = obs_source_get_unversioned_id(source);
	eventData["inputKindCaps"] = obs_source_get_output_flags(source);
	eventData["inputSettings"] = Utils::Json::ObsDataToJson(inputSettings);
	eventData["defaultInputSettings"] = Utils::Json::ObsDataToJson(defaultInputSettings, true);
	BroadcastEvent(EventSubscription::Inputs, "InputCreated", eventData);
}

/**
 * An input has been removed.
 *
 * @dataField inputName | String | Name of the input
 * @dataField inputUuid | String | UUID of the input
 *
 * @eventType InputRemoved
 * @eventSubscription Inputs
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputRemoved(obs_source_t *source)
{
	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	BroadcastEvent(EventSubscription::Inputs, "InputRemoved", eventData);
}

/**
 * The name of an input has changed.
 *
 * @dataField inputUuid    | String | UUID of the input
 * @dataField oldInputName | String | Old name of the input
 * @dataField inputName    | String | New name of the input
 *
 * @eventType InputNameChanged
 * @eventSubscription Inputs
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputNameChanged(obs_source_t *source, std::string oldInputName, std::string inputName)
{
	json eventData;
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["oldInputName"] = oldInputName;
	eventData["inputName"] = inputName;
	BroadcastEvent(EventSubscription::Inputs, "InputNameChanged", eventData);
}

/**
 * An input's settings have changed (been updated).
 *
 * Note: On some inputs, changing values in the properties dialog will cause an immediate update. Pressing the "Cancel" button will revert the settings, resulting in another event being fired.
 *
 * @dataField inputName     | String | Name of the input
 * @dataField inputUuid     | String | UUID of the input
 * @dataField inputSettings | Object | New settings object of the input
 *
 * @eventType InputSettingsChanged
 * @eventSubscription Inputs
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.4.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputSettingsChanged(obs_source_t *source)
{
	OBSDataAutoRelease inputSettings = obs_source_get_settings(source);

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["inputSettings"] = Utils::Json::ObsDataToJson(inputSettings);
	BroadcastEvent(EventSubscription::Inputs, "InputSettingsChanged", eventData);
}

/**
 * An input's active state has changed.
 *
 * When an input is active, it means it's being shown by the program feed.
 *
 * @dataField inputName   | String  | Name of the input
 * @dataField inputUuid   | String  | UUID of the input
 * @dataField videoActive | Boolean | Whether the input is active
 *
 * @eventType InputActiveStateChanged
 * @eventSubscription InputActiveStateChanged
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputActiveStateChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	if (!eventHandler->_inputActiveStateChangedRef.load())
		return;

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT)
		return;

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["videoActive"] = obs_source_active(source);
	eventHandler->BroadcastEvent(EventSubscription::InputActiveStateChanged, "InputActiveStateChanged", eventData);
}

/**
 * An input's show state has changed.
 *
 * When an input is showing, it means it's being shown by the preview or a dialog.
 *
 * @dataField inputName    | String  | Name of the input
 * @dataField inputUuid    | String  | UUID of the input
 * @dataField videoShowing | Boolean | Whether the input is showing
 *
 * @eventType InputShowStateChanged
 * @eventSubscription InputShowStateChanged
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputShowStateChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	if (!eventHandler->_inputShowStateChangedRef.load())
		return;

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT)
		return;

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["videoShowing"] = obs_source_showing(source);
	eventHandler->BroadcastEvent(EventSubscription::InputShowStateChanged, "InputShowStateChanged", eventData);
}

/**
 * An input's mute state has changed.
 *
 * @dataField inputName  | String  | Name of the input
 * @dataField inputUuid  | String  | UUID of the input
 * @dataField inputMuted | Boolean | Whether the input is muted
 *
 * @eventType InputMuteStateChanged
 * @eventSubscription Inputs
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputMuteStateChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT)
		return;

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["inputMuted"] = obs_source_muted(source);
	eventHandler->BroadcastEvent(EventSubscription::Inputs, "InputMuteStateChanged", eventData);
}

/**
 * An input's volume level has changed.
 *
 * @dataField inputName      | String | Name of the input
 * @dataField inputUuid      | String | UUID of the input
 * @dataField inputVolumeMul | Number | New volume level multiplier
 * @dataField inputVolumeDb  | Number | New volume level in dB
 *
 * @eventType InputVolumeChanged
 * @eventSubscription Inputs
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputVolumeChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT)
		return;

	// Volume must be grabbed from the calldata. Running obs_source_get_volume() will return the previous value.
	double inputVolumeMul = calldata_float(data, "volume");

	double inputVolumeDb = obs_mul_to_db((float)inputVolumeMul);
	if (inputVolumeDb == -INFINITY)
		inputVolumeDb = -100;

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["inputVolumeMul"] = inputVolumeMul;
	eventData["inputVolumeDb"] = inputVolumeDb;
	eventHandler->BroadcastEvent(EventSubscription::Inputs, "InputVolumeChanged", eventData);
}

/**
 * The audio balance value of an input has changed.
 *
 * @dataField inputName         | String | Name of the input
 * @dataField inputUuid         | String | UUID of the input
 * @dataField inputAudioBalance | Number | New audio balance value of the input
 *
 * @eventType InputAudioBalanceChanged
 * @eventSubscription Inputs
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @category inputs
 * @api events
 */
void EventHandler::HandleInputAudioBalanceChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT)
		return;

	float inputAudioBalance = (float)calldata_float(data, "balance");

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["inputAudioBalance"] = inputAudioBalance;
	eventHandler->BroadcastEvent(EventSubscription::Inputs, "InputAudioBalanceChanged", eventData);
}

/**
 * The sync offset of an input has changed.
 *
 * @dataField inputName            | String | Name of the input
 * @dataField inputUuid            | String | UUID of the input
 * @dataField inputAudioSyncOffset | Number | New sync offset in milliseconds
 *
 * @eventType InputAudioSyncOffsetChanged
 * @eventSubscription Inputs
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputAudioSyncOffsetChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT)
		return;

	long long inputAudioSyncOffset = calldata_int(data, "offset");

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["inputAudioSyncOffset"] = inputAudioSyncOffset / 1000000;
	eventHandler->BroadcastEvent(EventSubscription::Inputs, "InputAudioSyncOffsetChanged", eventData);
}

/**
 * The audio tracks of an input have changed.
 *
 * @dataField inputName        | String | Name of the input
 * @dataField inputUuid        | String | UUID of the input
 * @dataField inputAudioTracks | Object | Object of audio tracks along with their associated enable states
 *
 * @eventType InputAudioTracksChanged
 * @eventSubscription Inputs
 * @complexity 3
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputAudioTracksChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT)
		return;

	long long tracks = calldata_int(data, "mixers");

	json inputAudioTracks;
	for (long long i = 0; i < MAX_AUDIO_MIXES; i++) {
		inputAudioTracks[std::to_string(i + 1)] = (bool)((tracks >> i) & 1);
	}

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["inputAudioTracks"] = inputAudioTracks;
	eventHandler->BroadcastEvent(EventSubscription::Inputs, "InputAudioTracksChanged", eventData);
}

/**
 * The monitor type of an input has changed.
 *
 * Available types are:
 *
 * - `OBS_MONITORING_TYPE_NONE`
 * - `OBS_MONITORING_TYPE_MONITOR_ONLY`
 * - `OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT`
 *
 * @dataField inputName   | String | Name of the input
 * @dataField inputUuid   | String | UUID of the input
 * @dataField monitorType | String | New monitor type of the input
 *
 * @eventType InputAudioMonitorTypeChanged
 * @eventSubscription Inputs
 * @complexity 2
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputAudioMonitorTypeChanged(void *param, calldata_t *data)
{
	auto eventHandler = static_cast<EventHandler *>(param);

	obs_source_t *source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT)
		return;

	enum obs_monitoring_type monitorType = (obs_monitoring_type)calldata_int(data, "type");

	json eventData;
	eventData["inputName"] = obs_source_get_name(source);
	eventData["inputUuid"] = obs_source_get_uuid(source);
	eventData["monitorType"] = monitorType;
	eventHandler->BroadcastEvent(EventSubscription::Inputs, "InputAudioMonitorTypeChanged", eventData);
}

/**
 * A high-volume event providing volume levels of all active inputs every 50 milliseconds.
 *
 * @dataField inputs | Array<Object> | Array of active inputs with their associated volume levels
 *
 * @eventType InputVolumeMeters
 * @eventSubscription InputVolumeMeters
 * @complexity 4
 * @rpcVersion -1
 * @initialVersion 5.0.0
 * @api events
 * @category inputs
 */
void EventHandler::HandleInputVolumeMeters(std::vector<json> &inputs)
{
	json eventData;
	eventData["inputs"] = inputs;
	BroadcastEvent(EventSubscription::InputVolumeMeters, "InputVolumeMeters", eventData);
}
