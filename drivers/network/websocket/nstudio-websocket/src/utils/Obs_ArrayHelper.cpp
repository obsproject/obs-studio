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

#include <algorithm>

#include "Obs.h"
#include "plugin-macros.generated.h"

static std::vector<std::string> ConvertStringArray(char **array)
{
	std::vector<std::string> ret;
	if (!array)
		return ret;

	size_t index = 0;
	char *value = nullptr;
	do {
		value = array[index];
		if (value)
			ret.push_back(value);
		index++;
	} while (value);

	return ret;
}

std::vector<std::string> Utils::Obs::ArrayHelper::GetSceneCollectionList()
{
	char **sceneCollections = obs_frontend_get_scene_collections();
	auto ret = ConvertStringArray(sceneCollections);
	bfree(sceneCollections);
	return ret;
}

std::vector<std::string> Utils::Obs::ArrayHelper::GetProfileList()
{
	char **profiles = obs_frontend_get_profiles();
	auto ret = ConvertStringArray(profiles);
	bfree(profiles);
	return ret;
}

std::vector<obs_hotkey_t *> Utils::Obs::ArrayHelper::GetHotkeyList()
{
	std::vector<obs_hotkey_t *> ret;

	auto cb = [](void *data, obs_hotkey_id, obs_hotkey_t *hotkey) {
		auto ret = static_cast<std::vector<obs_hotkey_t *> *>(data);

		ret->push_back(hotkey);

		return true;
	};

	obs_enum_hotkeys(cb, &ret);

	return ret;
}

std::vector<std::string> Utils::Obs::ArrayHelper::GetHotkeyNameList()
{
	auto hotkeys = GetHotkeyList();

	std::vector<std::string> ret;
	for (auto hotkey : hotkeys)
		ret.emplace_back(obs_hotkey_get_name(hotkey));

	return ret;
}

std::vector<json> Utils::Obs::ArrayHelper::GetSceneList()
{
	obs_frontend_source_list sceneList = {};
	obs_frontend_get_scenes(&sceneList);

	std::vector<json> ret;
	ret.reserve(sceneList.sources.num);
	for (size_t i = 0; i < sceneList.sources.num; i++) {
		obs_source_t *scene = sceneList.sources.array[i];

		json sceneJson;
		sceneJson["sceneName"] = obs_source_get_name(scene);
		sceneJson["sceneUuid"] = obs_source_get_uuid(scene);
		sceneJson["sceneIndex"] = sceneList.sources.num - i - 1;

		ret.push_back(sceneJson);
	}

	obs_frontend_source_list_free(&sceneList);

	// Reverse the vector order to match other array returns
	std::reverse(ret.begin(), ret.end());

	return ret;
}

std::vector<std::string> Utils::Obs::ArrayHelper::GetGroupList()
{
	std::vector<std::string> ret;

	auto cb = [](void *priv_data, obs_source_t *scene) {
		auto ret = static_cast<std::vector<std::string> *>(priv_data);

		if (!obs_source_is_group(scene))
			return true;

		ret->emplace_back(obs_source_get_name(scene));

		return true;
	};

	obs_enum_scenes(cb, &ret);

	return ret;
}

std::vector<json> Utils::Obs::ArrayHelper::GetSceneItemList(obs_scene_t *scene, bool basic)
{
	std::pair<std::vector<json>, bool> enumData;
	enumData.second = basic;

	auto cb = [](obs_scene_t *, obs_sceneitem_t *sceneItem, void *param) {
		auto enumData = static_cast<std::pair<std::vector<json>, bool> *>(param);

		// TODO: Make ObjectHelper util for scene items

		json item;
		item["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
		item["sceneItemIndex"] =
			enumData->first.size(); // Should be slightly faster than calling obs_sceneitem_get_order_position()
		if (!enumData->second) {
			item["sceneItemEnabled"] = obs_sceneitem_visible(sceneItem);
			item["sceneItemLocked"] = obs_sceneitem_locked(sceneItem);
			item["sceneItemTransform"] = ObjectHelper::GetSceneItemTransform(sceneItem);
			item["sceneItemBlendMode"] = obs_sceneitem_get_blending_mode(sceneItem);
			OBSSource itemSource = obs_sceneitem_get_source(sceneItem);
			item["sourceName"] = obs_source_get_name(itemSource);
			item["sourceUuid"] = obs_source_get_uuid(itemSource);
			item["sourceType"] = obs_source_get_type(itemSource);
			if (obs_source_get_type(itemSource) == OBS_SOURCE_TYPE_INPUT)
				item["inputKind"] = obs_source_get_id(itemSource);
			else
				item["inputKind"] = nullptr;
			if (obs_source_get_type(itemSource) == OBS_SOURCE_TYPE_SCENE)
				item["isGroup"] = obs_source_is_group(itemSource);
			else
				item["isGroup"] = nullptr;
		}

		enumData->first.push_back(item);

		return true;
	};

	obs_scene_enum_items(scene, cb, &enumData);

	return enumData.first;
}

struct EnumInputInfo {
	std::string inputKind; // For searching by input kind
	std::vector<json> inputs;
};

std::vector<json> Utils::Obs::ArrayHelper::GetInputList(std::string inputKind)
{
	EnumInputInfo inputInfo;
	inputInfo.inputKind = inputKind;

	auto cb = [](void *param, obs_source_t *input) {
		// Sanity check in case the API changes
		if (obs_source_get_type(input) != OBS_SOURCE_TYPE_INPUT)
			return true;

		auto inputInfo = static_cast<EnumInputInfo *>(param);

		std::string inputKind = obs_source_get_id(input);

		if (!inputInfo->inputKind.empty() && inputInfo->inputKind != inputKind)
			return true;

		json inputJson;
		inputJson["inputName"] = obs_source_get_name(input);
		inputJson["inputUuid"] = obs_source_get_uuid(input);
		inputJson["inputKind"] = inputKind;
		inputJson["unversionedInputKind"] = obs_source_get_unversioned_id(input);
		inputJson["inputKindCaps"] = obs_source_get_output_flags(input);

		inputInfo->inputs.push_back(inputJson);
		return true;
	};

	// Actually enumerates only public inputs, despite the name
	obs_enum_sources(cb, &inputInfo);

	return inputInfo.inputs;
}

std::vector<std::string> Utils::Obs::ArrayHelper::GetInputKindList(bool unversioned, bool includeDisabled)
{
	std::vector<std::string> ret;

	size_t idx = 0;
	const char *kind;
	const char *unversioned_kind;
	while (obs_enum_input_types2(idx++, &kind, &unversioned_kind)) {
		uint32_t caps = obs_get_source_output_flags(kind);

		if (!includeDisabled && (caps & OBS_SOURCE_CAP_DISABLED) != 0)
			continue;

		if (unversioned)
			ret.push_back(unversioned_kind);
		else
			ret.push_back(kind);
	}

	return ret;
}

std::vector<json> Utils::Obs::ArrayHelper::GetListPropertyItems(obs_property_t *property)
{
	std::vector<json> ret;

	enum obs_combo_format itemFormat = obs_property_list_format(property);
	size_t itemCount = obs_property_list_item_count(property);

	ret.reserve(itemCount);

	for (size_t i = 0; i < itemCount; i++) {
		json itemData;
		itemData["itemName"] = obs_property_list_item_name(property, i);
		itemData["itemEnabled"] = !obs_property_list_item_disabled(property, i);
		if (itemFormat == OBS_COMBO_FORMAT_INT) {
			itemData["itemValue"] = obs_property_list_item_int(property, i);
		} else if (itemFormat == OBS_COMBO_FORMAT_FLOAT) {
			itemData["itemValue"] = obs_property_list_item_float(property, i);
		} else if (itemFormat == OBS_COMBO_FORMAT_STRING) {
			itemData["itemValue"] = obs_property_list_item_string(property, i);
		} else {
			itemData["itemValue"] = nullptr;
		}
		ret.push_back(itemData);
	}

	return ret;
}

std::vector<std::string> Utils::Obs::ArrayHelper::GetTransitionKindList()
{
	std::vector<std::string> ret;

	size_t idx = 0;
	const char *kind;
	while (obs_enum_transition_types(idx++, &kind))
		ret.emplace_back(kind);

	return ret;
}

std::vector<json> Utils::Obs::ArrayHelper::GetSceneTransitionList()
{
	obs_frontend_source_list transitionList = {};
	obs_frontend_get_transitions(&transitionList);

	std::vector<json> ret;
	ret.reserve(transitionList.sources.num);
	for (size_t i = 0; i < transitionList.sources.num; i++) {
		obs_source_t *transition = transitionList.sources.array[i];
		json transitionJson;
		transitionJson["transitionName"] = obs_source_get_name(transition);
		transitionJson["transitionUuid"] = obs_source_get_uuid(transition);
		transitionJson["transitionKind"] = obs_source_get_id(transition);
		transitionJson["transitionFixed"] = obs_transition_fixed(transition);
		transitionJson["transitionConfigurable"] = obs_source_configurable(transition);
		ret.push_back(transitionJson);
	}

	obs_frontend_source_list_free(&transitionList);

	return ret;
}

std::vector<std::string> Utils::Obs::ArrayHelper::GetFilterKindList()
{
	std::vector<std::string> ret;

	size_t idx = 0;
	const char *kind;
	while (obs_enum_filter_types(idx++, &kind))
		ret.push_back(kind);

	return ret;
}

std::vector<json> Utils::Obs::ArrayHelper::GetSourceFilterList(obs_source_t *source)
{
	std::vector<json> filters;

	auto cb = [](obs_source_t *, obs_source_t *filter, void *param) {
		auto filters = reinterpret_cast<std::vector<json> *>(param);

		json filterJson;
		filterJson["filterEnabled"] = obs_source_enabled(filter);
		filterJson["filterIndex"] = filters->size();
		filterJson["filterKind"] = obs_source_get_id(filter);
		filterJson["filterName"] = obs_source_get_name(filter);

		OBSDataAutoRelease filterSettings = obs_source_get_settings(filter);
		filterJson["filterSettings"] = Utils::Json::ObsDataToJson(filterSettings);

		filters->push_back(filterJson);
	};

	obs_source_enum_filters(source, cb, &filters);

	return filters;
}

std::vector<json> Utils::Obs::ArrayHelper::GetOutputList()
{
	std::vector<json> outputs;

	auto cb = [](void *param, obs_output_t *output) {
		auto outputs = reinterpret_cast<std::vector<json> *>(param);

		auto rawFlags = obs_output_get_flags(output);
		json flags;
		flags["OBS_OUTPUT_AUDIO"] = !!(rawFlags & OBS_OUTPUT_AUDIO);
		flags["OBS_OUTPUT_VIDEO"] = !!(rawFlags & OBS_OUTPUT_VIDEO);
		flags["OBS_OUTPUT_ENCODED"] = !!(rawFlags & OBS_OUTPUT_ENCODED);
		flags["OBS_OUTPUT_MULTI_TRACK"] = !!(rawFlags & OBS_OUTPUT_MULTI_TRACK);
		flags["OBS_OUTPUT_SERVICE"] = !!(rawFlags & OBS_OUTPUT_SERVICE);

		json outputJson;
		outputJson["outputName"] = obs_output_get_name(output);
		outputJson["outputKind"] = obs_output_get_id(output);
		outputJson["outputWidth"] = obs_output_get_width(output);
		outputJson["outputHeight"] = obs_output_get_height(output);
		outputJson["outputActive"] = obs_output_active(output);
		outputJson["outputFlags"] = flags;

		outputs->push_back(outputJson);
		return true;
	};

	obs_enum_outputs(cb, &outputs);

	return outputs;
}
