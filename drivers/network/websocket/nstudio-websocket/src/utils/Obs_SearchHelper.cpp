/*
obs-websocket
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

#include "Obs.h"
#include "plugin-macros.generated.h"

obs_hotkey_t *Utils::Obs::SearchHelper::GetHotkeyByName(std::string name, std::string context)
{
	if (name.empty())
		return nullptr;

	auto hotkeys = ArrayHelper::GetHotkeyList();

	for (auto hotkey : hotkeys) {
		if (obs_hotkey_get_name(hotkey) != name)
			continue;

		if (context.empty())
			return hotkey;

		auto type = obs_hotkey_get_registerer_type(hotkey);
		if (type == OBS_HOTKEY_REGISTERER_SOURCE) {
			OBSSourceAutoRelease source =
				obs_weak_source_get_source((obs_weak_source_t *)obs_hotkey_get_registerer(hotkey));
			if (!source)
				continue;

			if (context != obs_source_get_name(source))
				continue;

		} else if (type == OBS_HOTKEY_REGISTERER_OUTPUT) {
			OBSOutputAutoRelease output =
				obs_weak_output_get_output((obs_weak_output_t *)obs_hotkey_get_registerer(hotkey));
			if (!output)
				continue;

			if (context != obs_output_get_name(output))
				continue;

		} else if (type == OBS_HOTKEY_REGISTERER_ENCODER) {
			OBSEncoderAutoRelease encoder =
				obs_weak_encoder_get_encoder((obs_weak_encoder_t *)obs_hotkey_get_registerer(hotkey));
			if (!encoder)
				continue;

			if (context != obs_encoder_get_name(encoder))
				continue;

		} else if (type == OBS_HOTKEY_REGISTERER_SERVICE) {
			OBSServiceAutoRelease service =
				obs_weak_service_get_service((obs_weak_service_t *)obs_hotkey_get_registerer(hotkey));
			if (!service)
				continue;

			if (context != obs_service_get_name(service))
				continue;
		}
		return hotkey;
	}

	return nullptr;
}

// Increments source ref. Use OBSSourceAutoRelease
obs_source_t *Utils::Obs::SearchHelper::GetSceneTransitionByName(std::string name)
{
	obs_frontend_source_list transitionList = {};
	obs_frontend_get_transitions(&transitionList);

	obs_source_t *ret = nullptr;
	for (size_t i = 0; i < transitionList.sources.num; i++) {
		obs_source_t *transition = transitionList.sources.array[i];
		if (obs_source_get_name(transition) == name) {
			ret = obs_source_get_ref(transition);
			break;
		}
	}

	obs_frontend_source_list_free(&transitionList);

	return ret;
}

struct SceneItemSearchData {
	std::string name;
	int offset;
	obs_sceneitem_t *ret = nullptr;
};

// Increments item ref. Use OBSSceneItemAutoRelease
obs_sceneitem_t *Utils::Obs::SearchHelper::GetSceneItemByName(obs_scene_t *scene, std::string name, int offset)
{
	if (name.empty())
		return nullptr;

	SceneItemSearchData enumData;
	enumData.name = name;
	enumData.offset = offset;

	auto cb = [](obs_scene_t *, obs_sceneitem_t *sceneItem, void *param) {
		auto enumData = static_cast<SceneItemSearchData *>(param);

		OBSSource itemSource = obs_sceneitem_get_source(sceneItem);
		std::string sourceName = obs_source_get_name(itemSource);
		if (sourceName == enumData->name) {
			if (enumData->offset > 0) {
				enumData->offset--;
			} else {
				if (enumData->ret) // Release existing selection in the case of last match selection
					obs_sceneitem_release(enumData->ret);
				obs_sceneitem_addref(sceneItem);
				enumData->ret = sceneItem;
				if (enumData->offset == 0) // Only break if in normal selection mode (not offset == -1)
					return false;
			}
		}

		return true;
	};

	obs_scene_enum_items(scene, cb, &enumData);

	return enumData.ret;
}
