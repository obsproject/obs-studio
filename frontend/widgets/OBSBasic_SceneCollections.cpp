/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include <dialogs/OBSMissingFiles.hpp>
#include <importer/OBSImporter.hpp>
#include <utility/item-widget-helpers.hpp>

#include <qt-wrappers.hpp>

#include <QDir>

#include <filesystem>
#include <string>
#include <vector>

extern bool safe_mode;
extern bool opt_start_streaming;
extern bool opt_start_recording;
extern bool opt_start_virtualcam;
extern bool opt_start_replaybuffer;
extern std::string opt_starting_scene;

// MARK: Constant Expressions

constexpr std::string_view OBSSceneCollectionPath = "/obs-studio/basic/scenes/";

// MARK: - Anonymous Namespace
namespace {
QList<QString> sortedSceneCollections{};

void updateSortedSceneCollections(const OBSSceneCollectionCache &collections)
{
	const QLocale locale = QLocale::system();
	QList<QString> newList{};

	for (auto [collectionName, _] : collections) {
		QString entry = QString::fromStdString(collectionName);
		newList.append(entry);
	}

	std::sort(newList.begin(), newList.end(), [&locale](const QString &lhs, const QString &rhs) -> bool {
		int result = QString::localeAwareCompare(locale.toLower(lhs), locale.toLower(rhs));

		return (result < 0);
	});

	sortedSceneCollections.swap(newList);
}

void cleanBackupCollision(const OBSSceneCollection &collection)
{
	std::filesystem::path backupFilePath = collection.collectionFile;
	backupFilePath.replace_extension(".json.bak");

	if (std::filesystem::exists(backupFilePath)) {
		try {
			std::filesystem::remove(backupFilePath);
		} catch (std::filesystem::filesystem_error &) {
			throw std::logic_error("Failed to remove pre-existing scene collection backup file: " +
					       backupFilePath.u8string());
		}
	}
}
} // namespace

// MARK: - Main Scene Collection Management Functions

void OBSBasic::SetupNewSceneCollection(const std::string &collectionName)
{
	const OBSSceneCollection &newCollection = CreateSceneCollection(collectionName);

	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

	cleanBackupCollision(newCollection);
	ActivateSceneCollection(newCollection);

	blog(LOG_INFO, "Created scene collection '%s' (clean, %s)", newCollection.name.c_str(),
	     newCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::SetupDuplicateSceneCollection(const std::string &collectionName)
{
	const OBSSceneCollection &newCollection = CreateSceneCollection(collectionName);
	const OBSSceneCollection &currentCollection = GetCurrentSceneCollection();

	SaveProjectNow();

	const auto copyOptions = std::filesystem::copy_options::overwrite_existing;

	try {
		std::filesystem::copy(currentCollection.collectionFile, newCollection.collectionFile, copyOptions);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to copy file for cloned scene collection: " + newCollection.name);
	}

	OBSDataAutoRelease collection = obs_data_create_from_json_file(newCollection.collectionFile.u8string().c_str());

	obs_data_set_string(collection, "name", newCollection.name.c_str());

	OBSDataArrayAutoRelease sources = obs_data_get_array(collection, "sources");

	if (sources) {
		obs_data_erase(collection, "sources");

		obs_data_array_enum(
			sources,
			[](obs_data_t *data, void *) -> void {
				const char *uuid = os_generate_uuid();

				obs_data_set_string(data, "uuid", uuid);

				bfree((void *)uuid);
			},
			nullptr);

		obs_data_set_array(collection, "sources", sources);
	}

	obs_data_save_json_safe(collection, newCollection.collectionFile.u8string().c_str(), "tmp", nullptr);

	cleanBackupCollision(newCollection);
	ActivateSceneCollection(newCollection);

	blog(LOG_INFO, "Created scene collection '%s' (duplicate, %s)", newCollection.name.c_str(),
	     newCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::SetupRenameSceneCollection(const std::string &collectionName)
{
	const OBSSceneCollection &newCollection = CreateSceneCollection(collectionName);
	const OBSSceneCollection currentCollection = GetCurrentSceneCollection();

	SaveProjectNow();

	const auto copyOptions = std::filesystem::copy_options::overwrite_existing;

	try {
		std::filesystem::copy(currentCollection.collectionFile, newCollection.collectionFile, copyOptions);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to copy file for scene collection: " + currentCollection.name);
	}

	collections.erase(currentCollection.name);

	OBSDataAutoRelease collection = obs_data_create_from_json_file(newCollection.collectionFile.u8string().c_str());

	obs_data_set_string(collection, "name", newCollection.name.c_str());

	obs_data_save_json_safe(collection, newCollection.collectionFile.u8string().c_str(), "tmp", nullptr);

	cleanBackupCollision(newCollection);
	ActivateSceneCollection(newCollection);
	RemoveSceneCollection(currentCollection);

	blog(LOG_INFO, "Renamed scene collection '%s' to '%s' (%s)", currentCollection.name.c_str(),
	     newCollection.name.c_str(), newCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_RENAMED);
}

// MARK: - Scene Collection File Management Functions

const OBSSceneCollection &OBSBasic::CreateSceneCollection(const std::string &collectionName)
{
	if (const auto &foundCollection = GetSceneCollectionByName(collectionName)) {
		throw std::invalid_argument("Scene collection already exists: " + collectionName);
	}

	std::string fileName;
	if (!GetFileSafeName(collectionName.c_str(), fileName)) {
		throw std::invalid_argument("Failed to create safe directory for new scene collection: " +
					    collectionName);
	}

	std::string collectionFile;
	collectionFile.reserve(App()->userScenesLocation.u8string().size() + OBSSceneCollectionPath.size() +
			       fileName.size());
	collectionFile.append(App()->userScenesLocation.u8string()).append(OBSSceneCollectionPath).append(fileName);

	if (!GetClosestUnusedFileName(collectionFile, "json")) {
		throw std::invalid_argument("Failed to get closest file name for new scene collection: " + fileName);
	}

	const std::filesystem::path collectionFilePath = std::filesystem::u8path(collectionFile);

	auto [iterator, success] = collections.try_emplace(
		collectionName,
		OBSSceneCollection{collectionName, collectionFilePath.filename().u8string(), collectionFilePath});

	return iterator->second;
}

void OBSBasic::RemoveSceneCollection(OBSSceneCollection collection)
{
	try {
		std::filesystem::remove(collection.collectionFile);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to remove scene collection file: " + collection.fileName);
	}

	blog(LOG_INFO, "Removed scene collection '%s' (%s)", collection.name.c_str(), collection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

// MARK: - Scene Collection UI Handling Functions

bool OBSBasic::CreateNewSceneCollection(const QString &name)
{
	try {
		SetupNewSceneCollection(name.toStdString());
		return true;
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	}
}

bool OBSBasic::CreateDuplicateSceneCollection(const QString &name)
{
	try {
		SetupDuplicateSceneCollection(name.toStdString());
		return true;
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	}
}

void OBSBasic::DeleteSceneCollection(const QString &name)
{
	const std::string_view currentCollectionName{
		config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection")};

	if (currentCollectionName == name.toStdString()) {
		on_actionRemoveSceneCollection_triggered();
		return;
	}

	OBSSceneCollection currentCollection = GetCurrentSceneCollection();

	RemoveSceneCollection(currentCollection);

	collections.erase(name.toStdString());

	RefreshSceneCollections();

	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
}

void OBSBasic::ChangeSceneCollection()
{
	QAction *action = reinterpret_cast<QAction *>(sender());

	if (!action) {
		return;
	}

	const std::string_view currentCollectionName{
		config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection")};
	const QVariant qCollectionName = action->property("collection_name");
	const std::string selectedCollectionName{qCollectionName.toString().toStdString()};

	if (currentCollectionName == selectedCollectionName) {
		action->setChecked(true);
		return;
	}

	const std::optional<OBSSceneCollection> foundCollection = GetSceneCollectionByName(selectedCollectionName);

	if (!foundCollection) {
		const std::string errorMessage{"Selected scene collection not found: "};

		throw std::invalid_argument(errorMessage + currentCollectionName.data());
	}

	const OBSSceneCollection &selectedCollection = foundCollection.value();

	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

	ActivateSceneCollection(selectedCollection);

	blog(LOG_INFO, "Switched to scene collection '%s' (%s)", selectedCollection.name.c_str(),
	     selectedCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::RefreshSceneCollections(bool refreshCache)
{
	std::string_view currentCollectionName{config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection")};

	QList<QAction *> menuActions = ui->sceneCollectionMenu->actions();

	for (auto &action : menuActions) {
		QVariant variant = action->property("file_name");
		if (variant.typeName() != nullptr) {
			delete action;
		}
	}

	if (refreshCache) {
		RefreshSceneCollectionCache();
	}

	updateSortedSceneCollections(collections);

	size_t numAddedCollections = 0;
	for (auto &name : sortedSceneCollections) {
		const std::string collectionName = name.toStdString();
		try {
			const OBSSceneCollection &collection = collections.at(collectionName);
			const QString qCollectionName = QString().fromStdString(collectionName);

			QAction *action = new QAction(qCollectionName, this);
			action->setProperty("collection_name", qCollectionName);
			action->setProperty("file_name", QString().fromStdString(collection.fileName));
			connect(action, &QAction::triggered, this, &OBSBasic::ChangeSceneCollection);
			action->setCheckable(true);
			action->setChecked(collectionName == currentCollectionName);

			ui->sceneCollectionMenu->addAction(action);

			numAddedCollections += 1;
		} catch (const std::out_of_range &error) {
			blog(LOG_ERROR, "No scene collection with name %s found in scene collection cache.\n%s",
			     collectionName.c_str(), error.what());
		}
	}

	ui->actionRemoveSceneCollection->setEnabled(numAddedCollections > 1);

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	main->ui->actionPasteFilters->setEnabled(false);
	main->ui->actionPasteRef->setEnabled(false);
	main->ui->actionPasteDup->setEnabled(false);
}

// MARK: - Scene Collection Cache Functions

void OBSBasic::RefreshSceneCollectionCache()
{
	OBSSceneCollectionCache foundCollections{};

	const std::filesystem::path collectionsPath =
		App()->userScenesLocation / std::filesystem::u8path(OBSSceneCollectionPath.substr(1));

	if (!std::filesystem::exists(collectionsPath)) {
		blog(LOG_WARNING, "Failed to get scene collections config path");
		return;
	}

	for (const auto &entry : std::filesystem::directory_iterator(collectionsPath)) {
		if (entry.is_directory()) {
			continue;
		}

		if (entry.path().extension().u8string() != ".json") {
			continue;
		}

		OBSDataAutoRelease collectionData =
			obs_data_create_from_json_file_safe(entry.path().u8string().c_str(), "bak");

		std::string candidateName;
		const char *collectionName = obs_data_get_string(collectionData, "name");

		if (!collectionName) {
			candidateName = entry.path().filename().u8string();
		} else {
			candidateName = collectionName;
		}

		foundCollections.try_emplace(candidateName,
					     OBSSceneCollection{candidateName, entry.path().filename().u8string(),
								entry.path()});
	}

	collections.swap(foundCollections);
}

const OBSSceneCollection &OBSBasic::GetCurrentSceneCollection() const
{
	std::string currentCollectionName{config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection")};

	if (currentCollectionName.empty()) {
		throw std::invalid_argument("No valid scene collection name in configuration Basic->SceneCollection");
	}

	const auto &foundCollection = collections.find(currentCollectionName);

	if (foundCollection != collections.end()) {
		return foundCollection->second;
	} else {
		throw std::invalid_argument("Scene collection not found in collection list: " + currentCollectionName);
	}
}

std::optional<OBSSceneCollection> OBSBasic::GetSceneCollectionByName(const std::string &collectionName) const
{
	auto foundCollection = collections.find(collectionName);

	if (foundCollection == collections.end()) {
		return {};
	} else {
		return foundCollection->second;
	}
}

std::optional<OBSSceneCollection> OBSBasic::GetSceneCollectionByFileName(const std::string &fileName) const
{
	for (auto &[iterator, collection] : collections) {
		if (collection.fileName == fileName) {
			return collection;
		}
	}

	return {};
}

// MARK: - Qt Slot Functions

void OBSBasic::on_actionNewSceneCollection_triggered()
{
	const OBSPromptCallback sceneCollectionCallback = [this](const OBSPromptResult &result) {
		if (GetSceneCollectionByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request{Str("Basic.Main.AddSceneCollection.Title"),
				       Str("Basic.Main.AddSceneCollection.Text")};

	OBSPromptResult result = PromptForName(request, sceneCollectionCallback);

	if (!result.success) {
		return;
	}

	try {
		SetupNewSceneCollection(result.promptValue);
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

void OBSBasic::on_actionDupSceneCollection_triggered()
{
	const OBSPromptCallback sceneCollectionCallback = [this](const OBSPromptResult &result) {
		if (GetSceneCollectionByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request{Str("Basic.Main.AddSceneCollection.Title"),
				       Str("Basic.Main.AddSceneCollection.Text")};

	OBSPromptResult result = PromptForName(request, sceneCollectionCallback);

	if (!result.success) {
		return;
	}

	try {
		SetupDuplicateSceneCollection(result.promptValue);
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

void OBSBasic::on_actionRenameSceneCollection_triggered()
{
	const OBSSceneCollection &currentCollection = GetCurrentSceneCollection();

	const OBSPromptCallback sceneCollectionCallback = [this](const OBSPromptResult &result) {
		if (GetSceneCollectionByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request{Str("Basic.Main.RenameSceneCollection.Title"),
				       Str("Basic.Main.AddSceneCollection.Text"), currentCollection.name};

	OBSPromptResult result = PromptForName(request, sceneCollectionCallback);

	if (!result.success) {
		return;
	}

	try {
		SetupRenameSceneCollection(result.promptValue);
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

void OBSBasic::on_actionRemoveSceneCollection_triggered(bool skipConfirmation)
{
	if (collections.size() < 2) {
		return;
	}

	OBSSceneCollection currentCollection;

	try {
		currentCollection = GetCurrentSceneCollection();

		if (!skipConfirmation) {
			const QString confirmationText =
				QTStr("ConfirmRemove.Text").arg(QString::fromStdString(currentCollection.name));
			const QMessageBox::StandardButton button =
				OBSMessageBox::question(this, QTStr("ConfirmRemove.Title"), confirmationText);

			if (button == QMessageBox::No) {
				return;
			}
		}

		OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

		collections.erase(currentCollection.name);
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
	}

	const OBSSceneCollection &newCollection = collections.begin()->second;

	ActivateSceneCollection(newCollection);
	RemoveSceneCollection(currentCollection);

	blog(LOG_INFO, "Switched to scene collection '%s' (%s)", newCollection.name.c_str(),
	     newCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::on_actionImportSceneCollection_triggered()
{
	OBSImporter imp(this);
	imp.exec();

	RefreshSceneCollections(true);
}

void OBSBasic::on_actionExportSceneCollection_triggered()
{
	SaveProjectNow();

	const OBSSceneCollection &currentCollection = GetCurrentSceneCollection();

	const QString home = QDir::homePath();

	const QString destinationFileName = SaveFile(this, QTStr("Basic.MainMenu.SceneCollection.Export"),
						     home + "/" + currentCollection.fileName.c_str(),
						     "JSON Files (*.json)");

	if (!destinationFileName.isEmpty() && !destinationFileName.isNull()) {
		const std::filesystem::path sourceFile = currentCollection.collectionFile;
		const std::filesystem::path destinationFile =
			std::filesystem::u8path(destinationFileName.toStdString());

		OBSDataAutoRelease collection = obs_data_create_from_json_file(sourceFile.u8string().c_str());

		OBSDataArrayAutoRelease sources = obs_data_get_array(collection, "sources");
		if (!sources) {
			blog(LOG_WARNING, "No sources in exported scene collection");
			return;
		}

		obs_data_erase(collection, "sources");

		using OBSDataVector = std::vector<OBSData>;

		OBSDataVector sourceItems;
		obs_data_array_enum(
			sources,
			[](obs_data_t *data, void *vector) -> void {
				OBSDataVector &sourceItems{*static_cast<OBSDataVector *>(vector)};
				sourceItems.push_back(data);
			},
			&sourceItems);

		std::sort(sourceItems.begin(), sourceItems.end(), [](const OBSData &a, const OBSData &b) {
			return astrcmpi(obs_data_get_string(a, "name"), obs_data_get_string(b, "name")) < 0;
		});

		OBSDataArrayAutoRelease newSources = obs_data_array_create();
		for (auto &item : sourceItems) {
			obs_data_array_push_back(newSources, item);
		}

		obs_data_set_array(collection, "sources", newSources);
		obs_data_save_json_pretty_safe(collection, destinationFile.u8string().c_str(), "tmp", "bak");
	}
}

void OBSBasic::on_actionRemigrateSceneCollection_triggered()
{
	if (Active()) {
		OBSMessageBox::warning(this, QTStr("Basic.Main.RemigrateSceneCollection.Title"),
				       QTStr("Basic.Main.RemigrateSceneCollection.CannotMigrate.Active"));
		return;
	}

	OBSDataAutoRelease priv = obs_get_private_data();

	if (!usingAbsoluteCoordinates && !migrationBaseResolution) {
		OBSMessageBox::warning(
			this, QTStr("Basic.Main.RemigrateSceneCollection.Title"),
			QTStr("Basic.Main.RemigrateSceneCollection.CannotMigrate.UnknownBaseResolution"));
		return;
	}

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	if (!usingAbsoluteCoordinates && migrationBaseResolution->first == ovi.base_width &&
	    migrationBaseResolution->second == ovi.base_height) {
		OBSMessageBox::warning(
			this, QTStr("Basic.Main.RemigrateSceneCollection.Title"),
			QTStr("Basic.Main.RemigrateSceneCollection.CannotMigrate.BaseResolutionMatches"));
		return;
	}

	const OBSSceneCollection &currentCollection = GetCurrentSceneCollection();

	QString name = QString::fromStdString(currentCollection.name);
	QString message =
		QTStr("Basic.Main.RemigrateSceneCollection.Text").arg(name).arg(ovi.base_width).arg(ovi.base_height);

	auto answer = OBSMessageBox::question(this, QTStr("Basic.Main.RemigrateSceneCollection.Title"), message);

	if (answer == QMessageBox::No)
		return;

	lastOutputResolution = {ovi.base_width, ovi.base_height};

	if (!usingAbsoluteCoordinates) {
		/* Temporarily change resolution to migration resolution */
		ovi.base_width = migrationBaseResolution->first;
		ovi.base_height = migrationBaseResolution->second;

		if (obs_reset_video(&ovi) != OBS_VIDEO_SUCCESS) {
			OBSMessageBox::critical(
				this, QTStr("Basic.Main.RemigrateSceneCollection.Title"),
				QTStr("Basic.Main.RemigrateSceneCollection.CannotMigrate.FailedVideoReset"));
			return;
		}
	}

	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

	/* Save and immediately reload to (re-)run migrations. */
	SaveProjectNow();
	/* Reset video if we potentially changed to a temporary resolution */
	if (!usingAbsoluteCoordinates) {
		ResetVideo();
	}

	ActivateSceneCollection(currentCollection);
}

// MARK: - Scene Collection Management Helper Functions

void OBSBasic::ActivateSceneCollection(const OBSSceneCollection &collection)
{
	const std::string currentCollectionName{config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection")};

	if (auto foundCollection = GetSceneCollectionByName(currentCollectionName)) {
		if (collection.name != foundCollection.value().name) {
			SaveProjectNow();
		}
	}

	config_set_string(App()->GetUserConfig(), "Basic", "SceneCollection", collection.name.c_str());
	config_set_string(App()->GetUserConfig(), "Basic", "SceneCollectionFile", collection.fileName.c_str());

	Load(collection.collectionFile.u8string().c_str());

	RefreshSceneCollections();

	UpdateTitleBar();

	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
}

// MARK: - OBSBasic Scene Collection Functions

using namespace std;

static void SaveAudioDevice(const char *name, int channel, obs_data_t *parent, vector<OBSSource> &audioSources)
{
	OBSSourceAutoRelease source = obs_get_output_source(channel);
	if (!source)
		return;

	audioSources.push_back(source.Get());

	OBSDataAutoRelease data = obs_save_source(source);

	obs_data_set_obj(parent, name, data);
}

static obs_data_t *GenerateSaveData(obs_data_array_t *sceneOrder, obs_data_array_t *quickTransitionData,
				    int transitionDuration, obs_data_array_t *transitions, OBSScene &scene,
				    OBSSource &curProgramScene, obs_data_array_t *savedProjectorList)
{
	obs_data_t *saveData = obs_data_create();

	vector<OBSSource> audioSources;
	audioSources.reserve(6);

	SaveAudioDevice(DESKTOP_AUDIO_1, 1, saveData, audioSources);
	SaveAudioDevice(DESKTOP_AUDIO_2, 2, saveData, audioSources);
	SaveAudioDevice(AUX_AUDIO_1, 3, saveData, audioSources);
	SaveAudioDevice(AUX_AUDIO_2, 4, saveData, audioSources);
	SaveAudioDevice(AUX_AUDIO_3, 5, saveData, audioSources);
	SaveAudioDevice(AUX_AUDIO_4, 6, saveData, audioSources);

	/* -------------------------------- */
	/* save non-group sources           */

	auto FilterAudioSources = [&](obs_source_t *source) {
		if (obs_source_is_group(source))
			return false;

		return find(begin(audioSources), end(audioSources), source) == end(audioSources);
	};
	using FilterAudioSources_t = decltype(FilterAudioSources);

	obs_data_array_t *sourcesArray = obs_save_sources_filtered(
		[](void *data, obs_source_t *source) {
			auto &func = *static_cast<FilterAudioSources_t *>(data);
			return func(source);
		},
		static_cast<void *>(&FilterAudioSources));

	/* -------------------------------- */
	/* save group sources separately    */

	/* saving separately ensures they won't be loaded in older versions */
	obs_data_array_t *groupsArray = obs_save_sources_filtered(
		[](void *, obs_source_t *source) { return obs_source_is_group(source); }, nullptr);

	/* -------------------------------- */

	OBSSourceAutoRelease transition = obs_get_output_source(0);
	obs_source_t *currentScene = obs_scene_get_source(scene);
	const char *sceneName = obs_source_get_name(currentScene);
	const char *programName = obs_source_get_name(curProgramScene);

	const char *sceneCollection = config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection");

	obs_data_set_string(saveData, "current_scene", sceneName);
	obs_data_set_string(saveData, "current_program_scene", programName);
	obs_data_set_array(saveData, "scene_order", sceneOrder);
	obs_data_set_string(saveData, "name", sceneCollection);
	obs_data_set_array(saveData, "sources", sourcesArray);
	obs_data_set_array(saveData, "groups", groupsArray);
	obs_data_set_array(saveData, "quick_transitions", quickTransitionData);
	obs_data_set_array(saveData, "transitions", transitions);
	obs_data_set_array(saveData, "saved_projectors", savedProjectorList);
	obs_data_array_release(sourcesArray);
	obs_data_array_release(groupsArray);

	obs_data_set_string(saveData, "current_transition", obs_source_get_name(transition));
	obs_data_set_int(saveData, "transition_duration", transitionDuration);

	return saveData;
}

void OBSBasic::Save(const char *file)
{
	OBSScene scene = GetCurrentScene();
	OBSSource curProgramScene = OBSGetStrongRef(programScene);
	if (!curProgramScene)
		curProgramScene = obs_scene_get_source(scene);

	OBSDataArrayAutoRelease sceneOrder = SaveSceneListOrder();
	OBSDataArrayAutoRelease transitions = SaveTransitions();
	OBSDataArrayAutoRelease quickTrData = SaveQuickTransitions();
	OBSDataArrayAutoRelease savedProjectorList = SaveProjectors();
	OBSDataAutoRelease saveData = GenerateSaveData(sceneOrder, quickTrData, ui->transitionDuration->value(),
						       transitions, scene, curProgramScene, savedProjectorList);

	obs_data_set_bool(saveData, "preview_locked", ui->preview->Locked());
	obs_data_set_bool(saveData, "scaling_enabled", ui->preview->IsFixedScaling());
	obs_data_set_int(saveData, "scaling_level", ui->preview->GetScalingLevel());
	obs_data_set_double(saveData, "scaling_off_x", ui->preview->GetScrollX());
	obs_data_set_double(saveData, "scaling_off_y", ui->preview->GetScrollY());

	if (vcamEnabled) {
		OBSDataAutoRelease obj = obs_data_create();

		obs_data_set_int(obj, "type2", (int)vcamConfig.type);
		switch (vcamConfig.type) {
		case VCamOutputType::Invalid:
		case VCamOutputType::ProgramView:
		case VCamOutputType::PreviewOutput:
			break;
		case VCamOutputType::SceneOutput:
			obs_data_set_string(obj, "scene", vcamConfig.scene.c_str());
			break;
		case VCamOutputType::SourceOutput:
			obs_data_set_string(obj, "source", vcamConfig.source.c_str());
			break;
		}

		obs_data_set_obj(saveData, "virtual-camera", obj);
	}

	if (api) {
		if (!collectionModuleData)
			collectionModuleData = obs_data_create();

		api->on_save(collectionModuleData);
		obs_data_set_obj(saveData, "modules", collectionModuleData);
	}

	if (lastOutputResolution) {
		OBSDataAutoRelease res = obs_data_create();
		obs_data_set_int(res, "x", lastOutputResolution->first);
		obs_data_set_int(res, "y", lastOutputResolution->second);
		obs_data_set_obj(saveData, "resolution", res);
	}

	obs_data_set_int(saveData, "version", usingAbsoluteCoordinates ? 1 : 2);

	if (migrationBaseResolution && !usingAbsoluteCoordinates) {
		OBSDataAutoRelease res = obs_data_create();
		obs_data_set_int(res, "x", migrationBaseResolution->first);
		obs_data_set_int(res, "y", migrationBaseResolution->second);
		obs_data_set_obj(saveData, "migration_resolution", res);
	}

	if (!obs_data_save_json_pretty_safe(saveData, file, "tmp", "bak"))
		blog(LOG_ERROR, "Could not save scene data to %s", file);
}

void OBSBasic::DeferSaveBegin()
{
	os_atomic_inc_long(&disableSaving);
}

void OBSBasic::DeferSaveEnd()
{
	long result = os_atomic_dec_long(&disableSaving);
	if (result == 0) {
		SaveProject();
	}
}

static void LogFilter(obs_source_t *, obs_source_t *filter, void *v_val);

static void LoadAudioDevice(const char *name, int channel, obs_data_t *parent)
{
	OBSDataAutoRelease data = obs_data_get_obj(parent, name);
	if (!data)
		return;

	OBSSourceAutoRelease source = obs_load_source(data);
	if (!source)
		return;

	obs_set_output_source(channel, source);

	const char *source_name = obs_source_get_name(source);
	blog(LOG_INFO, "[Loaded global audio device]: '%s'", source_name);
	obs_source_enum_filters(source, LogFilter, (void *)(intptr_t)1);
	obs_monitoring_type monitoring_type = obs_source_get_monitoring_type(source);
	if (monitoring_type != OBS_MONITORING_TYPE_NONE) {
		const char *type = (monitoring_type == OBS_MONITORING_TYPE_MONITOR_ONLY) ? "monitor only"
											 : "monitor and output";

		blog(LOG_INFO, "    - monitoring: %s", type);
	}
}

void OBSBasic::DisableRelativeCoordinates(bool enable)
{
	/* Allow disabling relative positioning to allow loading collections
	 * that cannot yet be migrated. */
	OBSDataAutoRelease priv = obs_get_private_data();
	obs_data_set_bool(priv, "AbsoluteCoordinates", enable);
	usingAbsoluteCoordinates = enable;

	ui->actionRemigrateSceneCollection->setText(enable ? QTStr("Basic.MainMenu.SceneCollection.Migrate")
							   : QTStr("Basic.MainMenu.SceneCollection.Remigrate"));
	ui->actionRemigrateSceneCollection->setEnabled(enable);
}

void OBSBasic::CreateDefaultScene(bool firstStart)
{
	disableSaving++;

	ClearSceneData();
	InitDefaultTransitions();
	CreateDefaultQuickTransitions();
	ui->transitionDuration->setValue(300);
	SetTransition(fadeTransition);

	DisableRelativeCoordinates(false);
	OBSSceneAutoRelease scene = obs_scene_create(Str("Basic.Scene"));

	if (firstStart)
		CreateFirstRunSources();

	SetCurrentScene(scene, true);

	disableSaving--;
}

static void LogFilter(obs_source_t *, obs_source_t *filter, void *v_val)
{
	const char *name = obs_source_get_name(filter);
	const char *id = obs_source_get_id(filter);
	int val = (int)(intptr_t)v_val;
	string indent;

	for (int i = 0; i < val; i++)
		indent += "    ";

	blog(LOG_INFO, "%s- filter: '%s' (%s)", indent.c_str(), name, id);
}

static bool LogSceneItem(obs_scene_t *, obs_sceneitem_t *item, void *v_val)
{
	obs_source_t *source = obs_sceneitem_get_source(item);
	const char *name = obs_source_get_name(source);
	const char *id = obs_source_get_id(source);
	int indent_count = (int)(intptr_t)v_val;
	string indent;

	for (int i = 0; i < indent_count; i++)
		indent += "    ";

	blog(LOG_INFO, "%s- source: '%s' (%s)", indent.c_str(), name, id);

	obs_monitoring_type monitoring_type = obs_source_get_monitoring_type(source);

	if (monitoring_type != OBS_MONITORING_TYPE_NONE) {
		const char *type = (monitoring_type == OBS_MONITORING_TYPE_MONITOR_ONLY) ? "monitor only"
											 : "monitor and output";

		blog(LOG_INFO, "    %s- monitoring: %s", indent.c_str(), type);
	}
	int child_indent = 1 + indent_count;
	obs_source_enum_filters(source, LogFilter, (void *)(intptr_t)child_indent);

	obs_source_t *show_tn = obs_sceneitem_get_transition(item, true);
	obs_source_t *hide_tn = obs_sceneitem_get_transition(item, false);
	if (show_tn)
		blog(LOG_INFO, "    %s- show: '%s' (%s)", indent.c_str(), obs_source_get_name(show_tn),
		     obs_source_get_id(show_tn));
	if (hide_tn)
		blog(LOG_INFO, "    %s- hide: '%s' (%s)", indent.c_str(), obs_source_get_name(hide_tn),
		     obs_source_get_id(hide_tn));

	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, LogSceneItem, (void *)(intptr_t)child_indent);
	return true;
}

void OBSBasic::LogScenes()
{
	blog(LOG_INFO, "------------------------------------------------");
	blog(LOG_INFO, "Loaded scenes:");

	for (int i = 0; i < ui->scenes->count(); i++) {
		QListWidgetItem *item = ui->scenes->item(i);
		OBSScene scene = GetOBSRef<OBSScene>(item);

		obs_source_t *source = obs_scene_get_source(scene);
		const char *name = obs_source_get_name(source);

		blog(LOG_INFO, "- scene '%s':", name);
		obs_scene_enum_items(scene, LogSceneItem, (void *)(intptr_t)1);
		obs_source_enum_filters(source, LogFilter, (void *)(intptr_t)1);
	}

	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::Load(const char *file, bool remigrate)
{
	disableSaving++;
	lastOutputResolution.reset();
	migrationBaseResolution.reset();

	obs_data_t *data = obs_data_create_from_json_file_safe(file, "bak");
	if (!data) {
		disableSaving--;
		const auto path = filesystem::u8path(file);
		const string name = path.stem().u8string();
		/* Check if file exists but failed to load. */
		if (filesystem::exists(path)) {
			/* Assume the file is corrupt and rename it to allow
			 * for manual recovery if possible. */
			auto newPath = path;
			newPath.concat(".invalid");

			blog(LOG_WARNING,
			     "File exists but appears to be corrupt, renaming "
			     "to \"%s\" before continuing.",
			     newPath.filename().u8string().c_str());

			error_code ec;
			filesystem::rename(path, newPath, ec);
			if (ec) {
				blog(LOG_ERROR, "Failed renaming corrupt file with %d", ec.value());
			}
		}

		blog(LOG_INFO, "No scene file found, creating default scene");

		bool hasFirstRun = config_get_bool(App()->GetUserConfig(), "General", "FirstRun");

		CreateDefaultScene(!hasFirstRun);
		SaveProject();
		return;
	}

	LoadData(data, file, remigrate);
}

static inline void AddMissingFiles(void *data, obs_source_t *source)
{
	obs_missing_files_t *f = (obs_missing_files_t *)data;
	obs_missing_files_t *sf = obs_source_get_missing_files(source);

	obs_missing_files_append(f, sf);
	obs_missing_files_destroy(sf);
}

static void ClearRelativePosCb(obs_data_t *data, void *)
{
	const string_view id = obs_data_get_string(data, "id");
	if (id != "scene" && id != "group")
		return;

	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
	OBSDataArrayAutoRelease items = obs_data_get_array(settings, "items");

	obs_data_array_enum(
		items,
		[](obs_data_t *data, void *) {
			obs_data_unset_user_value(data, "pos_rel");
			obs_data_unset_user_value(data, "scale_rel");
			obs_data_unset_user_value(data, "scale_ref");
			obs_data_unset_user_value(data, "bounds_rel");
		},
		nullptr);
}

void OBSBasic::LoadData(obs_data_t *data, const char *file, bool remigrate)
{
	ClearSceneData();
	ClearContextBar();

	/* Exit OBS if clearing scene data failed for some reason. */
	if (clearingFailed) {
		OBSMessageBox::critical(this, QTStr("SourceLeak.Title"), QTStr("SourceLeak.Text"));
		close();
		return;
	}

	InitDefaultTransitions();

	if (devicePropertiesThread && devicePropertiesThread->isRunning()) {
		devicePropertiesThread->wait();
		devicePropertiesThread.reset();
	}

	QApplication::sendPostedEvents(nullptr);

	OBSDataAutoRelease modulesObj = obs_data_get_obj(data, "modules");
	if (api)
		api->on_preload(modulesObj);

	/* Keep a reference to "modules" data so plugins that are not loaded do
	 * not have their collection specific data lost. */
	collectionModuleData = obs_data_get_obj(data, "modules");

	OBSDataArrayAutoRelease sceneOrder = obs_data_get_array(data, "scene_order");
	OBSDataArrayAutoRelease sources = obs_data_get_array(data, "sources");
	OBSDataArrayAutoRelease groups = obs_data_get_array(data, "groups");
	OBSDataArrayAutoRelease transitions = obs_data_get_array(data, "transitions");
	const char *sceneName = obs_data_get_string(data, "current_scene");
	const char *programSceneName = obs_data_get_string(data, "current_program_scene");
	const char *transitionName = obs_data_get_string(data, "current_transition");

	if (!opt_starting_scene.empty()) {
		programSceneName = opt_starting_scene.c_str();
		if (!IsPreviewProgramMode())
			sceneName = opt_starting_scene.c_str();
	}

	int newDuration = obs_data_get_int(data, "transition_duration");
	if (!newDuration)
		newDuration = 300;

	if (!transitionName)
		transitionName = obs_source_get_name(fadeTransition);

	const char *curSceneCollection = config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection");

	obs_data_set_default_string(data, "name", curSceneCollection);

	const char *name = obs_data_get_string(data, "name");
	OBSSourceAutoRelease curScene;
	OBSSourceAutoRelease curProgramScene;
	obs_source_t *curTransition;

	if (!name || !*name)
		name = curSceneCollection;

	LoadAudioDevice(DESKTOP_AUDIO_1, 1, data);
	LoadAudioDevice(DESKTOP_AUDIO_2, 2, data);
	LoadAudioDevice(AUX_AUDIO_1, 3, data);
	LoadAudioDevice(AUX_AUDIO_2, 4, data);
	LoadAudioDevice(AUX_AUDIO_3, 5, data);
	LoadAudioDevice(AUX_AUDIO_4, 6, data);

	if (!sources) {
		sources = std::move(groups);
	} else {
		obs_data_array_push_back_array(sources, groups);
	}

	/* Reset relative coordinate data if forcefully remigrating. */
	if (remigrate) {
		obs_data_set_int(data, "version", 1);
		obs_data_array_enum(sources, ClearRelativePosCb, nullptr);
	}

	bool resetVideo = false;
	bool disableRelativeCoords = false;
	obs_video_info ovi;

	int64_t version = obs_data_get_int(data, "version");
	OBSDataAutoRelease res = obs_data_get_obj(data, "resolution");
	if (res) {
		lastOutputResolution = {obs_data_get_int(res, "x"), obs_data_get_int(res, "y")};
	}

	/* Only migrate legacy collection if resolution is saved. */
	if (version < 2 && lastOutputResolution) {
		obs_get_video_info(&ovi);

		uint32_t width = obs_data_get_int(res, "x");
		uint32_t height = obs_data_get_int(res, "y");

		migrationBaseResolution = {width, height};

		if (ovi.base_height != height || ovi.base_width != width) {
			ovi.base_width = width;
			ovi.base_height = height;

			/* Attempt to reset to last known canvas resolution for migration. */
			resetVideo = obs_reset_video(&ovi) == OBS_VIDEO_SUCCESS;
			disableRelativeCoords = !resetVideo;
		}

		/* If migration is possible, and it wasn't forced, back up the original file. */
		if (!disableRelativeCoords && !remigrate) {
			auto path = filesystem::u8path(file);
			auto backupPath = path.concat(".v1");
			if (!filesystem::exists(backupPath)) {
				if (!obs_data_save_json_pretty_safe(data, backupPath.u8string().c_str(), "tmp", NULL)) {
					blog(LOG_WARNING,
					     "Failed to create a backup of existing scene collection data!");
				}
			}
		}
	} else if (version < 2) {
		disableRelativeCoords = true;
	} else if (OBSDataAutoRelease migration_res = obs_data_get_obj(data, "migration_resolution")) {
		migrationBaseResolution = {obs_data_get_int(migration_res, "x"), obs_data_get_int(migration_res, "y")};
	}

	DisableRelativeCoordinates(disableRelativeCoords);

	obs_missing_files_t *files = obs_missing_files_create();
	obs_load_sources(sources, AddMissingFiles, files);

	if (resetVideo)
		ResetVideo();
	if (transitions)
		LoadTransitions(transitions, AddMissingFiles, files);
	if (sceneOrder)
		LoadSceneListOrder(sceneOrder);

	curTransition = FindTransition(transitionName);
	if (!curTransition)
		curTransition = fadeTransition;

	ui->transitionDuration->setValue(newDuration);
	SetTransition(curTransition);

retryScene:
	curScene = obs_get_source_by_name(sceneName);
	curProgramScene = obs_get_source_by_name(programSceneName);

	/* if the starting scene command line parameter is bad at all,
	 * fall back to original settings */
	if (!opt_starting_scene.empty() && (!curScene || !curProgramScene)) {
		sceneName = obs_data_get_string(data, "current_scene");
		programSceneName = obs_data_get_string(data, "current_program_scene");
		opt_starting_scene.clear();
		goto retryScene;
	}

	if (!curScene) {
		auto find_scene_cb = [](void *source_ptr, obs_source_t *scene) {
			*static_cast<OBSSourceAutoRelease *>(source_ptr) = obs_source_get_ref(scene);
			return false;
		};
		obs_enum_scenes(find_scene_cb, &curScene);
	}

	SetCurrentScene(curScene.Get(), true);

	if (!curProgramScene)
		curProgramScene = std::move(curScene);
	if (IsPreviewProgramMode())
		TransitionToScene(curProgramScene.Get(), true);

	/* ------------------- */

	bool projectorSave = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SaveProjectors");

	if (projectorSave) {
		OBSDataArrayAutoRelease savedProjectors = obs_data_get_array(data, "saved_projectors");

		if (savedProjectors) {
			LoadSavedProjectors(savedProjectors);
			OpenSavedProjectors();
			activateWindow();
		}
	}

	/* ------------------- */

	std::string file_base = strrchr(file, '/') + 1;
	file_base.erase(file_base.size() - 5, 5);

	config_set_string(App()->GetUserConfig(), "Basic", "SceneCollection", name);
	config_set_string(App()->GetUserConfig(), "Basic", "SceneCollectionFile", file_base.c_str());

	OBSDataArrayAutoRelease quickTransitionData = obs_data_get_array(data, "quick_transitions");
	LoadQuickTransitions(quickTransitionData);

	RefreshQuickTransitions();

	bool previewLocked = obs_data_get_bool(data, "preview_locked");
	ui->preview->SetLocked(previewLocked);
	ui->actionLockPreview->setChecked(previewLocked);

	/* ---------------------- */

	bool fixedScaling = obs_data_get_bool(data, "scaling_enabled");
	int scalingLevel = (int)obs_data_get_int(data, "scaling_level");
	float scrollOffX = (float)obs_data_get_double(data, "scaling_off_x");
	float scrollOffY = (float)obs_data_get_double(data, "scaling_off_y");

	if (fixedScaling) {
		ui->preview->SetScalingLevel(scalingLevel);
		ui->preview->SetScrollingOffset(scrollOffX, scrollOffY);
	}
	ui->preview->SetFixedScaling(fixedScaling);

	emit ui->preview->DisplayResized();

	if (vcamEnabled) {
		OBSDataAutoRelease obj = obs_data_get_obj(data, "virtual-camera");

		vcamConfig.type = (VCamOutputType)obs_data_get_int(obj, "type2");
		if (vcamConfig.type == VCamOutputType::Invalid)
			vcamConfig.type = (VCamOutputType)obs_data_get_int(obj, "type");

		if (vcamConfig.type == VCamOutputType::Invalid) {
			VCamInternalType internal = (VCamInternalType)obs_data_get_int(obj, "internal");

			switch (internal) {
			case VCamInternalType::Default:
				vcamConfig.type = VCamOutputType::ProgramView;
				break;
			case VCamInternalType::Preview:
				vcamConfig.type = VCamOutputType::PreviewOutput;
				break;
			}
		}
		vcamConfig.scene = obs_data_get_string(obj, "scene");
		vcamConfig.source = obs_data_get_string(obj, "source");
	}

	if (obs_data_has_user_value(data, "resolution")) {
		OBSDataAutoRelease res = obs_data_get_obj(data, "resolution");
		if (obs_data_has_user_value(res, "x") && obs_data_has_user_value(res, "y")) {
			lastOutputResolution = {obs_data_get_int(res, "x"), obs_data_get_int(res, "y")};
		}
	}

	/* ---------------------- */

	if (api)
		api->on_load(modulesObj);

	obs_data_release(data);

	if (!opt_starting_scene.empty())
		opt_starting_scene.clear();

	if (opt_start_streaming && !safe_mode) {
		blog(LOG_INFO, "Starting stream due to command line parameter");
		QMetaObject::invokeMethod(this, "StartStreaming", Qt::QueuedConnection);
		opt_start_streaming = false;
	}

	if (opt_start_recording && !safe_mode) {
		blog(LOG_INFO, "Starting recording due to command line parameter");
		QMetaObject::invokeMethod(this, "StartRecording", Qt::QueuedConnection);
		opt_start_recording = false;
	}

	if (opt_start_replaybuffer && !safe_mode) {
		QMetaObject::invokeMethod(this, "StartReplayBuffer", Qt::QueuedConnection);
		opt_start_replaybuffer = false;
	}

	if (opt_start_virtualcam && !safe_mode) {
		QMetaObject::invokeMethod(this, "StartVirtualCam", Qt::QueuedConnection);
		opt_start_virtualcam = false;
	}

	LogScenes();

	if (!App()->IsMissingFilesCheckDisabled())
		ShowMissingFilesDialog(files);

	disableSaving--;

	if (vcamEnabled)
		outputHandler->UpdateVirtualCamOutputSource();

	OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
}

void OBSBasic::SaveProjectNow()
{
	if (disableSaving)
		return;

	projectChanged = true;
	SaveProjectDeferred();
}

void OBSBasic::SaveProject()
{
	if (disableSaving)
		return;

	projectChanged = true;
	QMetaObject::invokeMethod(this, "SaveProjectDeferred", Qt::QueuedConnection);
}

void OBSBasic::SaveProjectDeferred()
{
	if (disableSaving)
		return;

	if (!projectChanged)
		return;

	projectChanged = false;

	try {
		const OBSSceneCollection &currentCollection = GetCurrentSceneCollection();

		Save(currentCollection.collectionFile.u8string().c_str());
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

void OBSBasic::ClearSceneData()
{
	disableSaving++;

	setCursor(Qt::WaitCursor);

	CloseDialogs();

	ClearVolumeControls();
	ClearListItems(ui->scenes);
	ui->sources->Clear();
	ClearQuickTransitions();
	ui->transitions->clear();

	ClearProjectors();

	for (int i = 0; i < MAX_CHANNELS; i++)
		obs_set_output_source(i, nullptr);

	/* Reset VCam to default to clear its private scene and any references
	 * it holds. It will be reconfigured during loading. */
	if (vcamEnabled) {
		vcamConfig.type = VCamOutputType::ProgramView;
		outputHandler->UpdateVirtualCamOutputSource();
	}

	collectionModuleData = nullptr;
	lastScene = nullptr;
	swapScene = nullptr;
	programScene = nullptr;
	prevFTBSource = nullptr;

	clipboard.clear();
	copyFiltersSource = nullptr;
	copyFilter = nullptr;

	auto cb = [](void *, obs_source_t *source) {
		obs_source_remove(source);
		return true;
	};

	obs_enum_scenes(cb, nullptr);
	obs_enum_sources(cb, nullptr);

	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP);

	undo_s.clear();

	/* using QEvent::DeferredDelete explicitly is the only way to ensure
	 * that deleteLater events are processed at this point */
	QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

	do {
		QApplication::sendPostedEvents(nullptr);
	} while (obs_wait_for_destroy_queue());

	/* Pump Qt events one final time to give remaining signals time to be
	 * processed (since this happens after the destroy thread finishes and
	 * the audio/video threads have processed their tasks). */
	QApplication::sendPostedEvents(nullptr);

	unsetCursor();

	/* If scene data wasn't actually cleared, e.g. faulty plugin holding a
	 * reference, they will still be in the hash table, enumerate them and
	 * store the names for logging purposes. */
	auto cb2 = [](void *param, obs_source_t *source) {
		auto orphans = static_cast<vector<string> *>(param);
		orphans->push_back(obs_source_get_name(source));
		return true;
	};

	vector<string> orphan_sources;
	obs_enum_sources(cb2, &orphan_sources);

	if (!orphan_sources.empty()) {
		/* Avoid logging list twice in case it gets called after
		 * setting the flag the first time. */
		if (!clearingFailed) {
			/* This ugly mess exists to join a vector of strings
			 * with a user-defined delimiter. */
			string orphan_names =
				std::accumulate(orphan_sources.begin(), orphan_sources.end(), string(""),
						[](string a, string b) { return std::move(a) + "\n- " + b; });

			blog(LOG_ERROR, "Not all sources were cleared when clearing scene data:\n%s\n",
			     orphan_names.c_str());
		}

		/* We do not decrement disableSaving here to avoid OBS
		 * overwriting user data with garbage. */
		clearingFailed = true;
	} else {
		disableSaving--;

		blog(LOG_INFO, "All scene data cleared");
		blog(LOG_INFO, "------------------------------------------------");
	}
}

void OBSBasic::ShowMissingFilesDialog(obs_missing_files_t *files)
{
	if (obs_missing_files_count(files) > 0) {
		/* When loading the missing files dialog on launch, the
		* window hasn't fully initialized by this point on macOS,
		* so put this at the end of the current task queue. Fixes
		* a bug where the window is behind OBS on startup. */
		QTimer::singleShot(0, [this, files] {
			missDialog = new OBSMissingFiles(files, this);
			missDialog->setAttribute(Qt::WA_DeleteOnClose, true);
			missDialog->show();
			missDialog->raise();
		});
	} else {
		obs_missing_files_destroy(files);

		/* Only raise dialog if triggered manually */
		if (!disableSaving)
			OBSMessageBox::information(this, QTStr("MissingFiles.NoMissing.Title"),
						   QTStr("MissingFiles.NoMissing.Text"));
	}
}

void OBSBasic::on_actionShowMissingFiles_triggered()
{
	obs_missing_files_t *files = obs_missing_files_create();

	auto cb_sources = [](void *data, obs_source_t *source) {
		AddMissingFiles(data, source);
		return true;
	};

	obs_enum_all_sources(cb_sources, files);
	ShowMissingFilesDialog(files);
}
