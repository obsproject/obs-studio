/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

std::vector<std::string> CanvasMediator::getScenes() const
{
	return canvas_state_.orderedSceneUuids;
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
	obs_data_set_int(data, "index", canvas_state_.getSceneOrder(sceneUuid));

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
	const char *uuidPointer = obs_source_get_uuid(source);

	if (obs_source_is_scene(source) && !obs_source_is_group(source)) {
		QMetaObject::invokeMethod(static_cast<CanvasMediator *>(data), "onSceneAdded", Qt::QueuedConnection,
					  Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}

void CanvasMediator::obsCanvasSourceRemoved(void *data, calldata_t *params)
{
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	const char *uuidPointer = obs_source_get_uuid(source);

	if (obs_source_is_scene(source) && !obs_source_is_group(source)) {
		QMetaObject::invokeMethod(static_cast<CanvasMediator *>(data), "onSceneRemoved", Qt::QueuedConnection,
					  Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}

void CanvasMediator::obsSourceRenamed(void *data, calldata_t *params)
{
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	const char *newName = calldata_string(params, "new_name");

	const char *uuidPointer = obs_source_get_uuid(source);

	if (obs_source_is_scene(source) && !obs_source_is_group(source)) {
		QMetaObject::invokeMethod(static_cast<CanvasMediator *>(data), "onSourceRenamed", Qt::QueuedConnection,
					  Q_ARG(QString, QString::fromUtf8(uuidPointer)),
					  Q_ARG(QString, QString::fromUtf8(newName)));
	}
}

void CanvasMediator::registerSceneHotkey(const std::string &uuid)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
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

			std::optional<std::string> uuid = mediator->getSceneForHotkeyId(id);

			if (!uuid.has_value()) {
				return;
			}

			OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.value().c_str());
			OBSScene scene = obs_scene_from_source(source);

			if (!scene) {
				return;
			}

			mediator->setPreviewScene(scene);
		},
		static_cast<CanvasMediator *>(this));

	hotkeyScenes[hotkeyId] = uuid;
}

void CanvasMediator::unregisterSceneHotkey(const std::string &uuid)
{
	auto it = std::find_if(hotkeyScenes.begin(), hotkeyScenes.end(),
			       [uuid](const auto &pair) { return pair.second == uuid; });

	if (it == hotkeyScenes.end()) {
		return;
	}

	auto hotkeyId = it->first;

	obs_hotkey_unregister(hotkeyId);
	hotkeyScenes.erase(it);
}

std::optional<std::string> CanvasMediator::getSceneForHotkeyId(obs_hotkey_id id)
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

	obs_data_set_string(data, "preview_scene", canvas_state_.previewSceneUuid.c_str());
	obs_data_set_string(data, "program_scene", canvas_state_.programSceneUuid.c_str());

	OBSDataArrayAutoRelease sceneOrder = obs_data_array_create();
	for (const auto &uuid : canvas_state_.orderedSceneUuids) {
		OBSDataAutoRelease orderEntry = obs_data_create();
		obs_data_set_string(orderEntry, "uuid", uuid.c_str());
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

	if (canvas_state_.orderedSceneUuids.empty()) {
		// Scene order failed to load. Fall back to enumerating scenes in the canvas.
		auto callback = [](void *param, obs_source_t *source) -> bool {
			auto *vector = static_cast<std::vector<std::string> *>(param);

			if (!obs_source_is_scene(source)) {
				return true;
			}

			const char *uuid = obs_source_get_uuid(source);
			vector->emplace_back(uuid);

			return true;
		};

		obs_canvas_enum_scenes(getCanvas(), callback, &canvas_state_.orderedSceneUuids);
	}

	if (canvas_state_.orderedSceneUuids.empty()) {
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
		OBSSourceAutoRelease firstSource = obs_get_source_by_uuid(canvas_state_.orderedSceneUuids[0].c_str());
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

	const char *uuid = obs_source_get_uuid(source);

	int currentIndex = canvas_state_.getSceneOrder(uuid);
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

	const char *uuid = obs_source_get_uuid(source);

	int currentIndex = canvas_state_.getSceneOrder(uuid);
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

	if (static_cast<size_t>(newIndex) >= canvas_state_.orderedSceneUuids.size()) {
		return false;
	}

	OBSSource source = obs_scene_get_source(scene);
	if (!source) {
		return false;
	}

	const char *uuidPtr = obs_source_get_uuid(source);
	if (!uuidPtr) {
		return false;
	}

	std::string uuid(uuidPtr);

	auto it = std::find(canvas_state_.orderedSceneUuids.begin(), canvas_state_.orderedSceneUuids.end(), uuid);
	if (it == canvas_state_.orderedSceneUuids.end()) {
		return false;
	}

	int oldIndex = static_cast<int>(std::distance(canvas_state_.orderedSceneUuids.begin(), it));
	if (oldIndex == newIndex) {
		return false;
	}

	canvas_state_.orderedSceneUuids.erase(it);
	canvas_state_.orderedSceneUuids.insert(canvas_state_.orderedSceneUuids.begin() + newIndex, uuid);

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

	const char *uuidPtr = obs_source_get_uuid(source);

	canvas_state_.previewSceneUuid = uuidPtr;

	emit previewChanged(QString::fromUtf8(uuidPtr));
}

OBSScene CanvasMediator::getPreviewScene() const
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(canvas_state_.previewSceneUuid.c_str());
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

	OBSWeakSourceAutoRelease weak = obs_source_get_weak_source(source);
	if (!weak) {
		return;
	}

	const char *uuidPtr = obs_source_get_uuid(source);
	canvas_state_.programSceneUuid = uuidPtr;

	emit programChanged(QString::fromUtf8(uuidPtr));
}

OBSScene CanvasMediator::getProgramScene() const
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(canvas_state_.programSceneUuid.c_str());
	if (!source) {
		return nullptr;
	}

	return OBSScene(obs_scene_from_source(source));
}

std::unordered_map<std::string, CanvasMediator::State::SceneOrderInfo> CanvasMediator::getSceneOrderData() const
{
	return canvas_state_.sceneOrderData;
}

void CanvasMediator::rebuildSceneOrderFromData(const OBSDataArray &data)
{
	canvas_state_.orderedSceneUuids.clear();

	auto callback = [](obs_data_t *data, void *param) -> void {
		auto *vector = static_cast<std::vector<std::string> *>(param);
		const char *uuid = obs_data_get_string(data, "uuid");
		if (uuid && uuid[0] != '\0') {
			vector->emplace_back(uuid);
		} else {
			// Try name for legacy data
			const char *name = obs_data_get_string(data, "name");
			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			if (source) {
				vector->emplace_back(obs_source_get_uuid(source));
			}
		}
	};

	obs_data_array_enum(data, callback, &canvas_state_.orderedSceneUuids);
}

void CanvasMediator::normalizeSceneOrder()
{
	std::unordered_set<std::string> seen;
	std::vector<std::string> normalizedList;

	normalizedList.reserve(canvas_state_.orderedSceneUuids.size());

	for (const auto &uuid : canvas_state_.orderedSceneUuids) {
		if (seen.insert(uuid).second) {
			normalizedList.push_back(uuid);
		}
	}

	canvas_state_.orderedSceneUuids = std::move(normalizedList);

	for (size_t i = 0; i < canvas_state_.orderedSceneUuids.size(); ++i) {
		canvas_state_.sceneOrderData[canvas_state_.orderedSceneUuids[i]].order = static_cast<int>(i);
	}
}

void CanvasMediator::ensureValidPreviewScene()
{
	auto *main = OBSBasic::Get();
	if (main->SavingDisabled()) {
		// Skip during teardown
		return;
	}

	OBSSourceAutoRelease previewSource = obs_get_source_by_uuid(canvas_state_.previewSceneUuid.c_str());
	bool previewRemoved = previewSource == nullptr || obs_source_removed(previewSource);

	if (previewRemoved) {
		auto it = std::find(canvas_state_.orderedSceneUuids.begin(), canvas_state_.orderedSceneUuids.end(),
				    canvas_state_.previewSceneUuid);
		std::string nextUuid;

		if (it != canvas_state_.orderedSceneUuids.end()) {
			if (std::next(it) != canvas_state_.orderedSceneUuids.end()) {
				nextUuid = *std::next(it);
			} else if (it != canvas_state_.orderedSceneUuids.begin()) {
				nextUuid = *std::prev(it);
			}
		}

		if (nextUuid.empty()) {
			// No more scenes, set to null
			setPreviewScene(nullptr);
			return;
		}

		OBSSourceAutoRelease newPreviewSource = obs_get_source_by_uuid(nextUuid.c_str());
		if (!newPreviewSource) {
			return;
		}

		OBSScene newPreview = obs_scene_from_source(newPreviewSource);
		if (newPreview) {
			setPreviewScene(newPreview);
		}
	}
}

void CanvasMediator::onSceneAdded(const QString &uuid)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	if (!source) {
		return;
	}

	const char *name = obs_source_get_name(source);
	OBSScene scene = obs_scene_from_source(source);

	blog(LOG_DEBUG, "Registering '%s' in canvas mediator", name);

	std::string uuidStr(uuid.toUtf8().constData());
	auto it = std::find(canvas_state_.orderedSceneUuids.begin(), canvas_state_.orderedSceneUuids.end(), uuidStr);
	if (it == canvas_state_.orderedSceneUuids.end()) {
		canvas_state_.orderedSceneUuids.push_back(uuidStr);
	}

	commitSceneOrderChanges();

	registerSceneHotkey(uuidStr);

	// ----------

	auto *main = OBSBasic::Get();
	// FIXME: Needs a proper home in the future
	// Proposal: SceneMediator
	signal_handler_t *handler = obs_source_get_signal_handler(source);
	std::vector<OBSSignal> sceneHandlers;

	sceneHandlers.emplace_back(handler, "item_add", OBSBasic::SceneItemAdded, main);
	sceneHandlers.emplace_back(handler, "reorder", OBSBasic::SceneItemAdded, main);
	sceneHandlers.emplace_back(handler, "refresh", OBSBasic::SceneRefreshed, main);

	sourceSignals.insert({uuid.toStdString(), std::move(sceneHandlers)});
	// ----------

	main->SaveProject();

	if (!main->SavingDisabled()) {
		obs_source_t *source = obs_scene_get_source(scene);
		blog(LOG_INFO, "Scene '%s' registered in mediator", obs_source_get_name(source));

		main->updateMultiviewProjectors();
	}
	// ----------

	emit sceneAdded(uuid);
}

void CanvasMediator::onSceneRemoved(const QString &uuid)
{
	ensureValidPreviewScene();

	sourceSignals.erase(uuid.toStdString());

	auto it = std::find(canvas_state_.orderedSceneUuids.begin(), canvas_state_.orderedSceneUuids.end(),
			    uuid.toStdString());
	if (it != canvas_state_.orderedSceneUuids.end()) {
		canvas_state_.orderedSceneUuids.erase(it);
	}
	canvas_state_.sceneOrderData.erase(uuid.toStdString());

	unregisterSceneHotkey(uuid.toStdString());

	// FIXME: Needs a proper home in the future
	auto *main = OBSBasic::Get();
	main->SaveProject();

	if (!main->SavingDisabled()) {
		main->updateMultiviewProjectors();
	}
	// ----------

	if (!main->SavingDisabled()) {
		commitSceneOrderChanges();
		emit sceneRemoved(uuid);
	}
}

void CanvasMediator::onSourceRenamed(const QString &uuid, const QString &)
{
	OBSCanvas canvas = getCanvas();
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	if (!source) {
		return;
	}

	OBSScene scene = obs_scene_from_source(source);
	if (!scene) {
		return;
	}

	OBSCanvasAutoRelease sourceCanvas = obs_source_get_canvas(source);
	if (sourceCanvas != canvas) {
		return;
	}

	emit sceneRenamed(uuid);
}

void CanvasMediator::commitSceneOrderChanges()
{
	normalizeSceneOrder();

	emit sceneOrderChanged();
}
} // namespace OBS
