/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "CanvasMediator.hpp"

#include <widgets/OBSBasic.hpp>

#include <QListWidget>

#include <unordered_set>

namespace {
bool save_undo_source_enum(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *p)
{
	obs_source_t *source = obs_sceneitem_get_source(item);
	if (obs_obj_is_private(source) && !obs_source_removed(source)) {
		return true;
	}

	auto *array = static_cast<obs_data_array_t *>(p);

	/* check if the source is already stored in the array */
	const char *uuid = obs_source_get_uuid(source);
	const size_t count = obs_data_array_count(array);
	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease sourceData = obs_data_array_item(array, i);
		if (strcmp(uuid, obs_data_get_string(sourceData, "uuid")) == 0) {
			return true;
		}
	}

	if (obs_source_is_group(source)) {
		obs_scene_enum_items(obs_group_from_source(source), save_undo_source_enum, p);
	}

	OBSDataAutoRelease sourceData = obs_save_source(source);
	obs_data_array_push_back(array, sourceData);

	return true;
}
} // namespace

namespace OBS {
CanvasMediator::CanvasMediator(OBSApp *parent, obs_canvas_t *canvas) : QObject(parent)
{
	if (!canvas) {
		return;
	}

	uuid = obs_canvas_get_uuid(canvas);

	auto *mainHandler = obs_get_signal_handler();
	auto *canvasHandler = obs_canvas_get_signal_handler(canvas);

	signalHandlers.reserve(3);
	signalHandlers.emplace_back(canvasHandler, "source_add", CanvasMediator::obsCanvasSourceCreated, this);
	signalHandlers.emplace_back(canvasHandler, "source_remove", CanvasMediator::obsCanvasSourceRemoved, this);
	signalHandlers.emplace_back(mainHandler, "source_rename", CanvasMediator::obsSourceRenamed, this);
}

CanvasMediator::~CanvasMediator()
{
	signalHandlers.clear();
	sourceSignals.clear();
}

CanvasMediator *CanvasMediator::create(obs_canvas_t *canvas)
{
	auto *mediator = new CanvasMediator(App(), canvas);

	return mediator;
}

OBSCanvas CanvasMediator::getCanvas() const
{
	OBSCanvasAutoRelease canvas = obs_get_canvas_by_uuid(uuid.c_str());
	if (!canvas) {
		return nullptr;
	}

	return canvas.Get();
}

std::vector<OBSWeakSource> CanvasMediator::getScenes() const
{
	return canvas_state_.orderedScenes;
}

QInputDialog *CanvasMediator::createInputDialog()
{
	if (activeDialog) {
		activeDialog->deleteLater();
	}

	auto *inputDialog = new QInputDialog(OBSBasic::Get());
	activeDialog = inputDialog;

	inputDialog->setAttribute(Qt::WA_DeleteOnClose);

	inputDialog->setModal(true);
	inputDialog->setInputMode(QInputDialog::TextInput);

	inputDialog->adjustSize();
	inputDialog->setMinimumWidth(555);
	inputDialog->setMinimumHeight(100);

	return inputDialog;
}

void CanvasMediator::handleCreateScene(const QString &name)
{
	bool isNameEmpty = false;
	bool isNameExists = false;

	// Check if name is valid
	if (name.isEmpty()) {
		isNameEmpty = true;
	}

	OBSSourceAutoRelease existingSource = obs_get_source_by_name(name.toUtf8().constData());
	if (existingSource) {
		isNameExists = true;
	}

	// Name invalid, trigger duplicate again
	if (isNameEmpty) {
		auto *messageBox = OBSMessageBox::createWarning(OBSBasic::Get(), QTStr("NoNameEntered.Title"),
								QTStr("NoNameEntered.Text"));

		messageBox->open();

		connect(messageBox, &QMessageBox::finished, this, [this]() { requestCreateScene(); });
		return;
	}

	if (isNameExists) {
		auto *messageBox = OBSMessageBox::createWarning(OBSBasic::Get(), QTStr("NameExists.Title"),
								QTStr("NameExists.Text"));

		messageBox->open();

		connect(messageBox, &QMessageBox::finished, this, [this]() { requestCreateScene(); });
		return;
	}

	// FIXME: Needs a proper home in the future
	auto *main = OBSBasic::Get();
	auto undoHandler = [](const std::string &data) {
		OBSSourceAutoRelease tmp = obs_get_source_by_name(data.c_str());
		if (tmp) {
			obs_source_remove(tmp);
		}
	};

	auto redoHandler = [this](const std::string &data) {
		OBSSceneAutoRelease newScene = obs_scene_create(data.c_str());
		OBSSource source = obs_scene_get_source(newScene);

		OBSScene scene = newScene.Get();
		setPreviewScene(scene);
	};
	main->undo_s.add_action(QTStr("Undo.Add").arg(name), undoHandler, redoHandler, name.toStdString(),
				name.toStdString());

	// Create new scene
	OBSSceneAutoRelease newScene = obs_scene_create(name.toUtf8().constData());

	OBSScene scene = newScene.Get();
	setPreviewScene(scene);
}

void CanvasMediator::handleDuplicateName(const OBSScene &scene, const QString &name)
{
	bool isNameEmpty = false;
	bool isNameExists = false;

	// Check if name is valid
	if (name.isEmpty()) {
		isNameEmpty = true;
	}

	OBSSourceAutoRelease existingSource = obs_get_source_by_name(name.toUtf8().constData());
	if (existingSource) {
		isNameExists = false;
	}

	// Name invalid, trigger duplicate again
	if (isNameEmpty || isNameExists) {
		// TODO: Make warnings work too
		//OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
		//OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));

		if (scene) {
			requestDuplicateScene(scene);
		}
		return;
	}

	// Name valid, duplicate scene
	OBSSceneAutoRelease newScene = obs_scene_duplicate(scene, name.toUtf8().constData(), OBS_SCENE_DUP_REFS);

	// FIXME: Needs a proper home in the future
	auto *main = OBSBasic::Get();
	OBSSource source = obs_scene_get_source(newScene);
	main->SetCurrentScene(source, true);

	auto undoHandler = [](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
		obs_source_remove(source);
	};

	auto redoHandler = [name](const std::string &data) {
		auto *main = OBSBasic::Get();
		OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
		obs_scene_t *scene = obs_scene_from_source(source);
		scene = obs_scene_duplicate(scene, name.toUtf8().constData(), OBS_SCENE_DUP_REFS);
		source = obs_scene_get_source(scene);
		main->SetCurrentScene(source.Get(), true);
	};

	main->undo_s.add_action(QTStr("Undo.Scene.Duplicate").arg(obs_source_get_name(source)), undoHandler,
				redoHandler, obs_source_get_name(source),
				obs_source_get_name(obs_scene_get_source(scene)));
	// ----------
}

void CanvasMediator::handleDeleteScene(const OBSScene &scene)
{
	if (!scene) {
		return;
	}

	OBSSource source = obs_scene_get_source(scene);
	if (!source) {
		return;
	}

	// Scene is already flagged for removal
	if (obs_source_removed(source)) {
		return;
	}

	// FIXME: Needs a proper home in the future
	// Prepare data for undo/redo
	// - Save all sources in scene
	OBSDataArrayAutoRelease sourcesInDeletedScene = obs_data_array_create();

	obs_scene_enum_items(scene, save_undo_source_enum, sourcesInDeletedScene);

	OBSDataAutoRelease sceneData = obs_save_source(source);
	obs_data_array_push_back(sourcesInDeletedScene, sceneData);

	// Save all scenes and groups the scene is used in
	OBSDataArrayAutoRelease otherScenesContainingScene = obs_data_array_create();

	struct findOtherScenesData_t {
		obs_source_t *oldScene;
		obs_data_array_t *otherScenesContainingScene;
	} findOtherScenesData;
	findOtherScenesData.oldScene = source;
	findOtherScenesData.otherScenesContainingScene = otherScenesContainingScene;

	auto findOtherScenes = [](void *data_ptr, obs_source_t *scene) {
		auto *data = static_cast<findOtherScenesData_t *>(data_ptr);
		if (strcmp(obs_source_get_name(scene), obs_source_get_name(data->oldScene)) == 0) {
			return true;
		}
		obs_sceneitem_t *item = obs_scene_find_source(obs_group_or_scene_from_source(scene),
							      obs_source_get_name(data->oldScene));
		if (item) {
			OBSDataAutoRelease sceneData =
				obs_save_source(obs_scene_get_source(obs_sceneitem_get_scene(item)));
			obs_data_array_push_back(data->otherScenesContainingScene, sceneData);
		}
		return true;
	};
	obs_enum_scenes(findOtherScenes, &findOtherScenesData);

	// - Create Undo callback
	auto undoHandler = [](const std::string &json) {
		OBSDataAutoRelease base = obs_data_create_from_json(json.c_str());
		OBSDataArrayAutoRelease sourcesInDeletedScene = obs_data_get_array(base, "sourcesInDeletedScene");
		OBSDataArrayAutoRelease otherScenesContainingScene =
			obs_data_get_array(base, "otherScenesContainingScene");

		std::vector<OBSSource> recreatedSources;

		// Create missing sources
		size_t count = obs_data_array_count(sourcesInDeletedScene);
		recreatedSources.reserve(count);

		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease data = obs_data_array_item(sourcesInDeletedScene, i);
			const char *uuid = obs_data_get_string(data, "uuid");

			OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid);
			if (!source) {
				source = obs_load_source(data);
				recreatedSources.emplace_back(source.Get());
			}
		}

		// Load sources now
		for (obs_source_t *source : recreatedSources) {
			obs_source_load2(source);
		}

		// Add scene back to scenes and groups it was nested in
		for (size_t i = 0; i < obs_data_array_count(otherScenesContainingScene); i++) {
			OBSDataAutoRelease data = obs_data_array_item(otherScenesContainingScene, i);
			const char *uuid = obs_data_get_string(data, "uuid");
			OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid);

			OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
			OBSDataArrayAutoRelease items = obs_data_get_array(settings, "items");

			// Clear scene, but keep a reference to all sources in the scene to make sure they don't get destroyed
			std::vector<OBSSource> existingSources;
			auto cb = [](obs_scene_t *, obs_sceneitem_t *item, void *data) {
				auto *existing = static_cast<std::vector<OBSSource> *>(data);
				OBSSource source = obs_sceneitem_get_source(item);
				obs_sceneitem_remove(item);
				existing->push_back(source);
				return true;
			};
			obs_scene_enum_items(obs_group_or_scene_from_source(source), cb,
					     static_cast<void *>(&existingSources));

			// Re-add sources to the scene
			obs_sceneitems_add(obs_group_or_scene_from_source(source), items);
		}
	};

	// - Create Redo callback
	auto redoHandler = [](const std::string &uuid) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());

		obs_source_remove(source);

		auto cb = [](void *, obs_source_t *source) {
			if (strcmp(obs_source_get_id(source), "scene") == 0) {
				obs_scene_prune_sources(obs_scene_from_source(source));
			}
			return true;
		};
		obs_enum_scenes(cb, nullptr);
	};

	// - Prepare data
	auto *main = OBSBasic::Get();
	const char *sceneName = obs_source_get_name(source);
	const char *sceneUuid = obs_source_get_uuid(source);

	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_array(data, "sourcesInDeletedScene", sourcesInDeletedScene);
	obs_data_set_array(data, "otherScenesContainingScene", otherScenesContainingScene);
	OBSWeakSource weak = OBSGetWeakRef(source);
	obs_data_set_int(data, "index", canvas_state_.getSceneOrder(weak));

	main->undo_s.add_action(QTStr("Undo.Delete").arg(sceneName), undoHandler, redoHandler, obs_data_get_json(data),
				sceneUuid);

	// ----------

	// Mark scene to be removed
	obs_source_remove(source);

	auto enumCallback = [](void *, obs_source_t *source) {
		if (strcmp(obs_source_get_id(source), "scene") == 0) {
			obs_scene_prune_sources(obs_scene_from_source(source));
		}
		return true;
	};
	obs_enum_scenes(enumCallback, nullptr);
}

void CanvasMediator::obsCanvasSourceCreated(void *data, calldata_t *params)
{
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));

	if (obs_source_is_scene(source) && !obs_source_is_group(source)) {
		QMetaObject::invokeMethod(static_cast<CanvasMediator *>(data), "onSceneAdded", Qt::QueuedConnection,
					  Q_ARG(OBSSource, source));
	}
}

void CanvasMediator::obsCanvasSourceRemoved(void *data, calldata_t *params)
{
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));

	if (obs_source_is_scene(source) && !obs_source_is_group(source)) {
		QMetaObject::invokeMethod(static_cast<CanvasMediator *>(data), "onSceneRemoved", Qt::QueuedConnection,
					  Q_ARG(OBSSource, source));
	}
}

void CanvasMediator::obsSourceRenamed(void *data, calldata_t *params)
{
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	const char *newName = calldata_string(params, "new_name");

	if (obs_source_is_scene(source) && !obs_source_is_group(source)) {
		QMetaObject::invokeMethod(static_cast<CanvasMediator *>(data), "onSourceRenamed", Qt::QueuedConnection,
					  Q_ARG(OBSSource, source), Q_ARG(QString, QString::fromUtf8(newName)));
	}
}

void CanvasMediator::registerSceneHotkey(const OBSWeakSource &weak)
{
	OBSSource source = OBSGetStrongRef(weak);
	if (!source) {
		return;
	}

	obs_hotkey_id hotkeyId = obs_hotkey_register_source(
		source, "OBSBasic.SelectScene", Str("Basic.Hotkeys.SelectScene"),
		[](void *data, obs_hotkey_id id, obs_hotkey_t *, bool pressed) {
			if (!pressed) {
				return;
			}

			auto *mediator = static_cast<CanvasMediator *>(data);

			std::optional<OBSWeakSource> weak = mediator->getSceneForHotkeyId(id);

			if (!weak.has_value()) {
				return;
			}

			OBSSource source = OBSGetStrongRef(weak.value());
			OBSScene scene = obs_scene_from_source(source);

			if (!scene) {
				return;
			}

			mediator->setPreviewScene(scene);
		},
		static_cast<CanvasMediator *>(this));

	hotkeyScenes[hotkeyId] = weak;
}

void CanvasMediator::unregisterSceneHotkey(const OBSWeakSource &weak)
{
	auto it = std::find_if(hotkeyScenes.begin(), hotkeyScenes.end(),
			       [weak](const auto &pair) { return pair.second == weak; });

	if (it == hotkeyScenes.end()) {
		return;
	}

	auto hotkeyId = it->first;

	obs_hotkey_unregister(hotkeyId);
	hotkeyScenes.erase(it);
}

std::optional<OBSWeakSource> CanvasMediator::getSceneForHotkeyId(obs_hotkey_id id)
{
	auto it = hotkeyScenes.find(id);
	if (it != hotkeyScenes.end()) {
		return it->second;
	}

	return std::nullopt;
}

void CanvasMediator::requestCreateScene()
{
	if (activeDialog && activeDialog->isVisible()) {
		// A dialog is already active, deny request
		return;
	}

	QString format{QTStr("Basic.Main.DefaultSceneName.Text")};

	int i = 2;
	QString placeHolderText = format.arg(i);
	OBSSourceAutoRelease source = obs_get_source_by_name(placeHolderText.toUtf8().constData());
	while (source) {
		placeHolderText = format.arg(++i);
		source = obs_get_source_by_name(placeHolderText.toUtf8().constData());
	}

	QInputDialog *nameDialog = createInputDialog();
	nameDialog->setWindowTitle(QTStr("Basic.Main.AddSceneDlg.Title"));
	nameDialog->setLabelText(QTStr("Basic.Main.AddSceneDlg.Text"));
	nameDialog->setTextValue(placeHolderText);

	nameDialog->open();

	connect(nameDialog, &QInputDialog::textValueSelected, this, &CanvasMediator::handleCreateScene);
}

void CanvasMediator::requestDuplicateScene(const OBSScene &scene)
{
	if (!scene) {
		return;
	}

	if (activeDialog && activeDialog->isVisible()) {
		// A dialog is already active, deny request
		return;
	}

	OBSSource sceneSource = obs_scene_get_source(scene);
	QString format{obs_source_get_name(sceneSource)};
	format += " %1";

	int suffix = 2;
	QString placeHolderText = format.arg(suffix);
	OBSSourceAutoRelease source = obs_get_source_by_name(placeHolderText.toUtf8().constData());
	while (source) {
		placeHolderText = format.arg(++suffix);
		source = obs_get_source_by_name(placeHolderText.toUtf8().constData());
	}

	QInputDialog *nameDialog = createInputDialog();
	nameDialog->setWindowTitle(QTStr("Basic.Main.AddSceneDlg.Title"));
	nameDialog->setLabelText(QTStr("Basic.Main.AddSceneDlg.Text"));
	nameDialog->setTextValue(placeHolderText);

	nameDialog->open();

	connect(nameDialog, &QInputDialog::textValueSelected, this,
		[this, scene](const QString &name) { handleDuplicateName(scene, name); });
}

void CanvasMediator::requestDeleteScene(const OBSScene &scene)
{
	if (!scene) {
		return;
	}

	if (activeDialog && activeDialog->isVisible()) {
		// A dialog is already active, deny request
		return;
	}

	if (canvas_state_.sceneOrderData.size() <= 1) {
		auto *messageBox = OBSMessageBox::createInformation(OBSBasic::Get(), QTStr("FinalScene.Title"),
								    QTStr("FinalScene.Text"));

		messageBox->open();

		return;
	}

	OBSSource source = obs_scene_get_source(scene);
	const char *name = obs_source_get_name(source);

	QString text = QTStr("ConfirmRemove.Text").arg(QT_UTF8(name));

	if (activeDialog) {
		activeDialog->deleteLater();
	}

	activeDialog = new QMessageBox(OBSBasic::Get());

	auto *confirmDelete = qobject_cast<QMessageBox *>(activeDialog);
	confirmDelete->setAttribute(Qt::WA_DeleteOnClose);
	confirmDelete->setText(text);
	confirmDelete->addButton(QTStr("Yes"), QMessageBox::AcceptRole);
	confirmDelete->addButton(QTStr("No"), QMessageBox::RejectRole);
	confirmDelete->setIcon(QMessageBox::Question);
	confirmDelete->setWindowTitle(QTStr("ConfirmRemove.Title"));

	confirmDelete->open();

	connect(confirmDelete, &QMessageBox::accepted, this, [this, scene]() { handleDeleteScene(scene); });
}

void CanvasMediator::requestRenameScene(const OBSScene &scene, const QString &name)
{
	if (!scene) {
		return;
	}

	OBSSource source = obs_scene_get_source(scene);
	if (!source) {
		return;
	}

	const char *prevName = obs_source_get_name(source);

	OBSSourceAutoRelease nameExists = obs_get_source_by_name(name.toUtf8().constData());
	if (nameExists) {
		auto *messageBox = OBSMessageBox::createWarning(OBSBasic::Get(), QTStr("NameExists.Title"),
								QTStr("NameExists.Text"));

		messageBox->open();
		return;
	}

	if (name.isEmpty()) {
		auto *messageBox = OBSMessageBox::createWarning(OBSBasic::Get(), QTStr("NoNameEntered.Title"),
								QTStr("NoNameEntered.Text"));

		messageBox->open();
		return;
	}

	auto undoHandler = [prev = std::string(prevName)](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
		obs_source_set_name(source, prev.c_str());
	};

	std::string newName = name.toUtf8().constData();
	auto redoHandler = [newName](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
		obs_source_set_name(source, newName.c_str());
	};

	auto *main = OBSBasic::Get();
	const char *sourceUuid = obs_source_get_uuid(source);
	main->undo_s.add_action(QTStr("Undo.Rename").arg(newName.c_str()), undoHandler, redoHandler, sourceUuid,
				sourceUuid);

	obs_source_set_name(source, newName.c_str());
}

OBSDataAutoRelease CanvasMediator::serializeStateToOBSData() const
{
	obs_data_t *data = obs_data_create();

	OBSSourceAutoRelease preview = obs_weak_source_get_source(canvas_state_.previewScene);
	OBSSourceAutoRelease program = obs_weak_source_get_source(canvas_state_.programScene);

	if (preview) {
		obs_data_set_string(data, "preview_scene", obs_source_get_uuid(preview));
	}

	if (program) {
		obs_data_set_string(data, "program_scene", obs_source_get_uuid(program));
	}

	OBSDataArrayAutoRelease sceneOrder = obs_data_array_create();
	for (const auto &weak : canvas_state_.orderedScenes) {
		OBSDataAutoRelease orderEntry = obs_data_create();
		OBSSourceAutoRelease source = obs_weak_source_get_source(weak);
		if (!source) {
			continue;
		}

		obs_data_set_string(orderEntry, "uuid", obs_source_get_uuid(source));
		obs_data_array_push_back(sceneOrder, orderEntry);
	}

	obs_data_set_array(data, "scene_order", sceneOrder);

	return data;
}

void CanvasMediator::loadStateFromOBSData(obs_data_t *data)
{
	OBSDataArrayAutoRelease sceneOrderData = obs_data_get_array(data, "scene_order");
	rebuildSceneOrderFromData(sceneOrderData.Get());

	commitSceneOrderChanges();

	if (canvas_state_.orderedScenes.empty()) {
		// Scene order failed to load. Fall back to enumerating scenes in the canvas.
		auto callback = [](void *param, obs_source_t *source) -> bool {
			auto *vector = static_cast<std::vector<OBSWeakSource> *>(param);

			if (!obs_source_is_scene(source)) {
				return true;
			}

			OBSWeakSource weak = OBSGetWeakRef(source);

			vector->emplace_back(weak);

			return true;
		};

		obs_canvas_enum_scenes(getCanvas(), callback, &canvas_state_.orderedScenes);
	}

	if (canvas_state_.orderedScenes.empty()) {
		// No scenes in canvas?
		return;
	}

	const char *previewUuid = obs_data_get_string(data, "preview_scene");

	OBSSource fallbackProgram;
	OBSSourceAutoRelease previewSource = obs_get_source_by_uuid(previewUuid);
	if (previewSource && obs_source_is_scene(previewSource)) {
		setPreviewScene(obs_scene_from_source(previewSource));

		fallbackProgram = previewSource;
	} else {
		OBSSource firstSource = OBSGetStrongRef(canvas_state_.orderedScenes[0]);
		setPreviewScene(obs_scene_from_source(firstSource));

		fallbackProgram = firstSource;
	}

	const char *programUuid = obs_data_get_string(data, "program_scene");
	OBSSourceAutoRelease programSource = obs_get_source_by_uuid(programUuid);
	if (programSource && obs_source_is_scene(programSource)) {
		setProgramScene(obs_scene_from_source(programSource));
	} else {
		setProgramScene(obs_scene_from_source(fallbackProgram));
	}
}

void CanvasMediator::reorderSceneUp(const OBSScene &scene)
{
	OBSSource source = obs_scene_get_source(scene);
	if (!source) {
		return;
	}

	OBSWeakSource weak = OBSGetWeakRef(source);

	int currentIndex = canvas_state_.getSceneOrder(weak);
	if (currentIndex == 0) {
		return;
	}

	reorderSceneToIndex(scene, currentIndex - 1);
	commitSceneOrderChanges();
}

void CanvasMediator::reorderSceneDown(const OBSScene &scene)
{
	OBSSource source = obs_scene_get_source(scene);
	if (!source) {
		return;
	}

	OBSWeakSource weak = OBSGetWeakRef(source);

	int currentIndex = canvas_state_.getSceneOrder(weak);
	if (currentIndex >= static_cast<int>(canvas_state_.sceneOrderData.size() - 1)) {
		return;
	}

	reorderSceneToIndex(scene, currentIndex + 1);
	commitSceneOrderChanges();
}

void CanvasMediator::reorderSceneToTop(const OBSScene &scene)
{
	reorderSceneToIndex(scene, 0);
	commitSceneOrderChanges();
}

void CanvasMediator::reorderSceneToBottom(const OBSScene &scene)
{
	reorderSceneToIndex(scene, static_cast<int>(canvas_state_.sceneOrderData.size() - 1));
	commitSceneOrderChanges();
}

bool CanvasMediator::reorderSceneToIndex(const OBSScene &scene, int newIndex)
{
	if (newIndex < 0) {
		return false;
	}

	if (static_cast<size_t>(newIndex) >= canvas_state_.orderedScenes.size()) {
		return false;
	}

	OBSSource source = obs_scene_get_source(scene);
	if (!source) {
		return false;
	}

	OBSWeakSource weak = OBSGetWeakRef(source);

	auto it = std::find(canvas_state_.orderedScenes.begin(), canvas_state_.orderedScenes.end(), weak);
	if (it == canvas_state_.orderedScenes.end()) {
		return false;
	}

	int oldIndex = static_cast<int>(std::distance(canvas_state_.orderedScenes.begin(), it));
	if (oldIndex == newIndex) {
		return false;
	}

	canvas_state_.orderedScenes.erase(it);
	canvas_state_.orderedScenes.insert(canvas_state_.orderedScenes.begin() + newIndex, weak.Get());

	return true;
}

void CanvasMediator::setPreviewScene(const OBSScene &scene)
{
	if (!scene) {
		return;
	}

	OBSSource source = obs_scene_get_source(scene);
	if (!source) {
		return;
	}

	OBSWeakSource weak = OBSGetWeakRef(source);

	canvas_state_.previewScene = weak;

	emit previewChanged(weak);
}

OBSScene CanvasMediator::getPreviewScene() const
{
	OBSSource source = OBSGetStrongRef(canvas_state_.previewScene);
	if (!source) {
		return nullptr;
	}

	return OBSScene(obs_scene_from_source(source));
}

void CanvasMediator::setProgramScene(const OBSScene &scene)
{
	if (!scene) {
		return;
	}

	OBSSource source = obs_scene_get_source(scene);
	if (!source) {
		return;
	}

	OBSWeakSource weak = OBSGetWeakRef(source);

	canvas_state_.programScene = weak;

	emit programChanged(weak.Get());
}

OBSScene CanvasMediator::getProgramScene() const
{
	OBSSource source = OBSGetStrongRef(canvas_state_.programScene);
	if (!source) {
		return nullptr;
	}

	return OBSScene(obs_scene_from_source(source));
}

std::unordered_map<const obs_weak_source_t *, CanvasMediator::State::SceneOrderInfo>
CanvasMediator::getSceneOrderData() const
{
	return canvas_state_.sceneOrderData;
}

void CanvasMediator::rebuildSceneOrderFromData(const OBSDataArray &data)
{
	canvas_state_.orderedScenes.clear();

	auto callback = [](obs_data_t *data, void *param) -> void {
		auto *vector = static_cast<std::vector<OBSWeakSource> *>(param);
		const char *uuid = obs_data_get_string(data, "uuid");
		if (uuid && uuid[0] != '\0') {
			OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid);
			if (source) {
				vector->emplace_back(OBSGetWeakRef(source));
			}
		} else {
			// Try name for legacy data
			const char *name = obs_data_get_string(data, "name");
			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			if (source) {
				vector->emplace_back(OBSGetWeakRef(source));
			}
		}
	};

	obs_data_array_enum(data, callback, &canvas_state_.orderedScenes);
}

void CanvasMediator::normalizeSceneOrder()
{
	std::unordered_set<obs_weak_source_t *> seen;
	std::vector<OBSWeakSource> normalizedList;

	normalizedList.reserve(canvas_state_.orderedScenes.size());

	for (const auto &weak : canvas_state_.orderedScenes) {
		if (seen.insert(weak).second) {
			normalizedList.push_back(weak);
		}
	}

	canvas_state_.orderedScenes = std::move(normalizedList);

	for (size_t i = 0; i < canvas_state_.orderedScenes.size(); ++i) {
		canvas_state_.sceneOrderData[canvas_state_.orderedScenes[i]].order = static_cast<int>(i);
	}
}

void CanvasMediator::ensureValidPreviewScene()
{
	auto *main = OBSBasic::Get();
	if (main->SavingDisabled()) {
		// Skip during teardown
		return;
	}

	OBSSource previewSource = OBSGetStrongRef(canvas_state_.previewScene);
	bool previewRemoved = previewSource == nullptr || obs_source_removed(previewSource);

	if (previewRemoved) {
		auto it = std::find(canvas_state_.orderedScenes.begin(), canvas_state_.orderedScenes.end(),
				    canvas_state_.previewScene);
		OBSWeakSource nextWeak;

		if (it != canvas_state_.orderedScenes.end()) {
			if (std::next(it) != canvas_state_.orderedScenes.end()) {
				nextWeak = *std::next(it);
			} else if (it != canvas_state_.orderedScenes.begin()) {
				nextWeak = *std::prev(it);
			}
		}

		if (nextWeak == nullptr) {
			// No more scenes, set to null
			setPreviewScene(nullptr);
			return;
		}

		OBSSource newPreviewSource = OBSGetStrongRef(nextWeak);
		if (!newPreviewSource) {
			return;
		}

		OBSScene newPreview = obs_scene_from_source(newPreviewSource);
		if (newPreview) {
			setPreviewScene(newPreview);
		}
	}
}

void CanvasMediator::onSceneAdded(const OBSSource &source)
{
	const char *name = obs_source_get_name(source);
	OBSScene scene = obs_scene_from_source(source);

	OBSWeakSource weak = OBSGetWeakRef(source);

	blog(LOG_DEBUG, "Registering '%s' in canvas mediator", name);

	auto it = std::find(canvas_state_.orderedScenes.begin(), canvas_state_.orderedScenes.end(), weak);
	if (it == canvas_state_.orderedScenes.end()) {
		canvas_state_.orderedScenes.push_back(weak);
	}

	commitSceneOrderChanges();

	registerSceneHotkey(weak);

	// ----------

	auto *main = OBSBasic::Get();
	// FIXME: Needs a proper home in the future
	// Proposal: SceneMediator
	signal_handler_t *handler = obs_source_get_signal_handler(source);
	std::vector<OBSSignal> sceneHandlers;

	sceneHandlers.emplace_back(handler, "item_add", OBSBasic::SceneItemAdded, main);
	sceneHandlers.emplace_back(handler, "reorder", OBSBasic::SceneItemAdded, main);
	sceneHandlers.emplace_back(handler, "refresh", OBSBasic::SceneRefreshed, main);

	sourceSignals.insert({weak, std::move(sceneHandlers)});
	// ----------

	main->SaveProject();

	if (!main->SavingDisabled()) {
		obs_source_t *source = obs_scene_get_source(scene);
		blog(LOG_INFO, "Scene '%s' registered in mediator", obs_source_get_name(source));

		main->updateMultiviewProjectors();
	}
	// ----------

	emit sceneAdded(weak);
}

void CanvasMediator::onSceneRemoved(const OBSSource &source)
{
	ensureValidPreviewScene();

	OBSWeakSource weak = OBSGetWeakRef(source);

	sourceSignals.erase(weak);

	auto it = std::find(canvas_state_.orderedScenes.begin(), canvas_state_.orderedScenes.end(), weak);
	if (it != canvas_state_.orderedScenes.end()) {
		canvas_state_.orderedScenes.erase(it);
	}
	canvas_state_.sceneOrderData.erase(weak);

	unregisterSceneHotkey(weak);

	// FIXME: Needs a proper home in the future
	auto *main = OBSBasic::Get();
	main->SaveProject();

	if (!main->SavingDisabled()) {
		main->updateMultiviewProjectors();
	}
	// ----------

	if (!main->SavingDisabled()) {
		commitSceneOrderChanges();
		emit sceneRemoved(weak);
	}
}

void CanvasMediator::onSourceRenamed(const OBSSource &source, const QString &)
{
	OBSCanvas canvas = getCanvas();
	OBSWeakSource weak = OBSGetWeakRef(source);

	OBSScene scene = obs_scene_from_source(source);
	if (!scene) {
		return;
	}

	OBSCanvasAutoRelease sourceCanvas = obs_source_get_canvas(source);
	if (sourceCanvas != canvas) {
		return;
	}

	emit sceneRenamed(weak);
}

void CanvasMediator::commitSceneOrderChanges()
{
	normalizeSceneOrder();

	emit sceneOrderChanged();
}
} // namespace OBS
