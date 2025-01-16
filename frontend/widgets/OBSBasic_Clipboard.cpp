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

#include <dialogs/OBSBasicFilters.hpp>
#include <dialogs/OBSBasicSourceSelect.hpp>
#include <widgets/VolControl.hpp>

extern void undo_redo(const std::string &data);

void OBSBasic::on_actionCopyTransform_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();

	obs_sceneitem_get_info2(item, &copiedTransformInfo);
	obs_sceneitem_get_crop(item, &copiedCropInfo);

	ui->actionPasteTransform->setEnabled(true);
	hasCopiedTransform = true;
}

void OBSBasic::on_actionPasteTransform_triggered()
{
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	auto func = [](obs_scene_t *, obs_sceneitem_t *item, void *data) {
		if (!obs_sceneitem_selected(item))
			return true;
		if (obs_sceneitem_locked(item))
			return true;

		OBSBasic *main = reinterpret_cast<OBSBasic *>(data);

		obs_sceneitem_defer_update_begin(item);
		obs_sceneitem_set_info2(item, &main->copiedTransformInfo);
		obs_sceneitem_set_crop(item, &main->copiedCropInfo);
		obs_sceneitem_defer_update_end(item);

		return true;
	};

	obs_scene_enum_items(GetCurrentScene(), func, this);

	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(QTStr("Undo.Transform.Paste").arg(obs_source_get_name(GetCurrentSceneSource())), undo_redo,
			  undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionCopySource_triggered()
{
	clipboard.clear();

	for (auto &selectedSource : GetAllSelectedSourceItems()) {
		OBSSceneItem item = ui->sources->Get(selectedSource.row());
		if (!item)
			continue;

		OBSSource source = obs_sceneitem_get_source(item);

		SourceCopyInfo copyInfo;
		copyInfo.weak_source = OBSGetWeakRef(source);
		obs_sceneitem_get_info2(item, &copyInfo.transform);
		obs_sceneitem_get_crop(item, &copyInfo.crop);
		copyInfo.blend_method = obs_sceneitem_get_blending_method(item);
		copyInfo.blend_mode = obs_sceneitem_get_blending_mode(item);
		copyInfo.visible = obs_sceneitem_visible(item);

		clipboard.push_back(copyInfo);
	}

	UpdateEditMenu();
}

void OBSBasic::on_actionPasteRef_triggered()
{
	OBSSource scene_source = GetCurrentSceneSource();
	OBSData undo_data = BackupScene(scene_source);
	OBSScene scene = GetCurrentScene();

	undo_s.push_disabled();

	for (size_t i = clipboard.size(); i > 0; i--) {
		SourceCopyInfo &copyInfo = clipboard[i - 1];

		OBSSource source = OBSGetStrongRef(copyInfo.weak_source);
		if (!source)
			continue;

		const char *name = obs_source_get_name(source);

		/* do not allow duplicate refs of the same group in the same
		 * scene */
		if (!!obs_scene_get_group(scene, name)) {
			continue;
		}

		OBSBasicSourceSelect::SourcePaste(copyInfo, false);
	}

	undo_s.pop_disabled();

	QString action_name = QTStr("Undo.PasteSourceRef");
	const char *scene_name = obs_source_get_name(scene_source);

	OBSData redo_data = BackupScene(scene_source);
	CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}

void OBSBasic::on_actionPasteDup_triggered()
{
	OBSSource scene_source = GetCurrentSceneSource();
	OBSData undo_data = BackupScene(scene_source);

	undo_s.push_disabled();

	for (size_t i = clipboard.size(); i > 0; i--) {
		SourceCopyInfo &copyInfo = clipboard[i - 1];
		OBSBasicSourceSelect::SourcePaste(copyInfo, true);
	}

	undo_s.pop_disabled();

	QString action_name = QTStr("Undo.PasteSource");
	const char *scene_name = obs_source_get_name(scene_source);

	OBSData redo_data = BackupScene(scene_source);
	CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}

void OBSBasic::SourcePasteFilters(OBSSource source, OBSSource dstSource)
{
	if (source == dstSource)
		return;

	OBSDataArrayAutoRelease undo_array = obs_source_backup_filters(dstSource);
	obs_source_copy_filters(dstSource, source);
	OBSDataArrayAutoRelease redo_array = obs_source_backup_filters(dstSource);

	const char *srcName = obs_source_get_name(source);
	const char *dstName = obs_source_get_name(dstSource);
	QString text = QTStr("Undo.Filters.Paste.Multiple").arg(srcName, dstName);

	CreateFilterPasteUndoRedoAction(text, dstSource, undo_array, redo_array);
}

void OBSBasic::AudioMixerCopyFilters()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *source = vol->GetSource();

	copyFiltersSource = obs_source_get_weak_source(source);
	ui->actionPasteFilters->setEnabled(true);
}

void OBSBasic::AudioMixerPasteFilters()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *dstSource = vol->GetSource();

	OBSSourceAutoRelease source = obs_weak_source_get_source(copyFiltersSource);

	SourcePasteFilters(source.Get(), dstSource);
}

void OBSBasic::SceneCopyFilters()
{
	copyFiltersSource = obs_source_get_weak_source(GetCurrentSceneSource());
	ui->actionPasteFilters->setEnabled(true);
}

void OBSBasic::ScenePasteFilters()
{
	OBSSourceAutoRelease source = obs_weak_source_get_source(copyFiltersSource);

	OBSSource dstSource = GetCurrentSceneSource();

	SourcePasteFilters(source.Get(), dstSource);
}

void OBSBasic::on_actionCopyFilters_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();

	if (!item)
		return;

	OBSSource source = obs_sceneitem_get_source(item);

	copyFiltersSource = obs_source_get_weak_source(source);

	ui->actionPasteFilters->setEnabled(true);
}

void OBSBasic::CreateFilterPasteUndoRedoAction(const QString &text, obs_source_t *source, obs_data_array_t *undo_array,
					       obs_data_array_t *redo_array)
{
	auto undo_redo = [this](const std::string &json) {
		OBSDataAutoRelease data = obs_data_create_from_json(json.c_str());
		OBSDataArrayAutoRelease array = obs_data_get_array(data, "array");
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(data, "uuid"));

		obs_source_restore_filters(source, array);

		if (filters)
			filters->UpdateSource(source);
	};

	const char *uuid = obs_source_get_uuid(source);

	OBSDataAutoRelease undo_data = obs_data_create();
	OBSDataAutoRelease redo_data = obs_data_create();
	obs_data_set_array(undo_data, "array", undo_array);
	obs_data_set_array(redo_data, "array", redo_array);
	obs_data_set_string(undo_data, "uuid", uuid);
	obs_data_set_string(redo_data, "uuid", uuid);

	undo_s.add_action(text, undo_redo, undo_redo, obs_data_get_json(undo_data), obs_data_get_json(redo_data));
}

void OBSBasic::on_actionPasteFilters_triggered()
{
	OBSSourceAutoRelease source = obs_weak_source_get_source(copyFiltersSource);

	OBSSceneItem sceneItem = GetCurrentSceneItem();
	OBSSource dstSource = obs_sceneitem_get_source(sceneItem);

	SourcePasteFilters(source.Get(), dstSource);
}
