#include "SourceToolbar.hpp"

#include <widgets/OBSBasic.hpp>

#include "moc_SourceToolbar.cpp"

SourceToolbar::SourceToolbar(QWidget *parent, OBSSource source)
	: QWidget(parent),
	  weakSource(OBSGetWeakRef(source)),
	  props(obs_source_properties(source), obs_properties_destroy)
{
}

void SourceToolbar::SaveOldProperties(obs_source_t *source)
{
	oldData = obs_data_create();

	OBSDataAutoRelease oldSettings = obs_source_get_settings(source);
	obs_data_apply(oldData, oldSettings);
	obs_data_set_string(oldData, "undo_suuid", obs_source_get_uuid(source));
}

void SourceToolbar::SetUndoProperties(obs_source_t *source, bool repeatable)
{
	if (!oldData) {
		blog(LOG_ERROR, "%s: somehow oldData was null.", __FUNCTION__);
		return;
	}

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	OBSSource currentSceneSource = main->GetCurrentSceneSource();
	if (!currentSceneSource)
		return;
	std::string scene_uuid = obs_source_get_uuid(currentSceneSource);
	auto undo_redo = [scene_uuid = std::move(scene_uuid), main](const std::string &data) {
		OBSDataAutoRelease settings = obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(settings, "undo_suuid"));
		obs_source_reset_settings(source, settings);

		OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());
		main->SetCurrentScene(scene_source.Get(), true);

		main->UpdateContextBar();
	};

	OBSDataAutoRelease new_settings = obs_data_create();
	OBSDataAutoRelease curr_settings = obs_source_get_settings(source);
	obs_data_apply(new_settings, curr_settings);
	obs_data_set_string(new_settings, "undo_suuid", obs_source_get_uuid(source));

	std::string undo_data(obs_data_get_json(oldData));
	std::string redo_data(obs_data_get_json(new_settings));

	if (undo_data.compare(redo_data) != 0)
		main->undo_s.add_action(QTStr("Undo.Properties").arg(obs_source_get_name(source)), undo_redo, undo_redo,
					undo_data, redo_data, repeatable);

	oldData = nullptr;
}
