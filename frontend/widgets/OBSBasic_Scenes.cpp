/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "OBSBasic.hpp"

#include <qt-wrappers.hpp>

#include <vector>

namespace {
bool save_undo_source_enum(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *p)
{
	obs_source_t *source = obs_sceneitem_get_source(item);
	if (obs_obj_is_private(source) && !obs_source_removed(source))
		return true;

	obs_data_array_t *array = (obs_data_array_t *)p;

	/* check if the source is already stored in the array */
	const char *name = obs_source_get_name(source);
	const size_t count = obs_data_array_count(array);
	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease sourceData = obs_data_array_item(array, i);
		if (strcmp(name, obs_data_get_string(sourceData, "name")) == 0)
			return true;
	}

	if (obs_source_is_group(source))
		obs_scene_enum_items(obs_group_from_source(source), save_undo_source_enum, p);

	OBSDataAutoRelease source_data = obs_save_source(source);
	obs_data_array_push_back(array, source_data);
	return true;
}

bool add_source_enum(obs_scene_t *, obs_sceneitem_t *item, void *p)
{
	auto sources = static_cast<std::vector<OBSSource> *>(p);
	sources->push_back(obs_sceneitem_get_source(item));
	return true;
}
} // namespace

OBSData OBSBasic::BackupScene(obs_scene_t *scene, std::vector<obs_source_t *> *sources)
{
	OBSDataArrayAutoRelease undo_array = obs_data_array_create();

	if (!sources) {
		obs_scene_enum_items(scene, save_undo_source_enum, undo_array);
	} else {
		for (obs_source_t *source : *sources) {
			obs_data_t *source_data = obs_save_source(source);
			obs_data_array_push_back(undo_array, source_data);
			obs_data_release(source_data);
		}
	}

	OBSDataAutoRelease scene_data = obs_save_source(obs_scene_get_source(scene));
	obs_data_array_push_back(undo_array, scene_data);

	OBSDataAutoRelease data = obs_data_create();

	obs_data_set_array(data, "array", undo_array);
	obs_data_get_json(data);
	return data.Get();
}

void OBSBasic::CreateSceneUndoRedoAction(const QString &action_name, OBSData undo_data, OBSData redo_data)
{
	auto undo_redo = [this](const std::string &json) {
		OBSDataAutoRelease base = obs_data_create_from_json(json.c_str());
		OBSDataArrayAutoRelease array = obs_data_get_array(base, "array");
		std::vector<OBSSource> sources;
		std::vector<OBSSource> old_sources;

		/* create missing sources */
		const size_t count = obs_data_array_count(array);
		sources.reserve(count);

		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease data = obs_data_array_item(array, i);
			const char *name = obs_data_get_string(data, "name");

			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			if (!source)
				source = obs_load_source(data);

			sources.push_back(source.Get());

			/* update scene/group settings to restore their
			 * contents to their saved settings */
			obs_scene_t *scene = obs_group_or_scene_from_source(source);
			if (scene) {
				obs_scene_enum_items(scene, add_source_enum, &old_sources);
				OBSDataAutoRelease scene_settings = obs_data_get_obj(data, "settings");
				obs_source_update(source, scene_settings);
			}
		}

		/* actually load sources now */
		for (obs_source_t *source : sources)
			obs_source_load2(source);

		ui->sources->RefreshItems();
	};

	const char *undo_json = obs_data_get_last_json(undo_data);
	const char *redo_json = obs_data_get_last_json(redo_data);

	undo_s.add_action(action_name, undo_redo, undo_redo, undo_json, redo_json);
}
