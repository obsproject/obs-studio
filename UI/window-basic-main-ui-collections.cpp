/******************************************************************************
    Copyright (C) 2015 by Hugh Bailey <obs.jim@gmail.com>

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

#include <obs.hpp>
#include <util/util.hpp>
#include <QMessageBox>
#include <QVariant>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDesktopWidget>
#include "item-widget-helpers.hpp"
#include "window-basic-main.hpp"
#include "window-namedialog.hpp"
#include "qt-wrappers.hpp"

using namespace std;

void EnumUICollections(std::function<bool(const char *, const char *)> &&cb)
{
	char path[512];
	os_glob_t *glob;

	int ret =
		GetConfigPath(path, sizeof(path), "obs-studio/basic/ui/*.json");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get config path for ui "
				  "collections");
		return;
	}

	if (os_glob(path, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob ui collections");
		return;
	}

	for (size_t i = 0; i < glob->gl_pathc; i++) {
		const char *filePath = glob->gl_pathv[i].path;

		if (glob->gl_pathv[i].directory)
			continue;

		obs_data_t *data =
			obs_data_create_from_json_file_safe(filePath, "bak");
		std::string name = obs_data_get_string(data, "name");

		/* if no name found, use the file name as the name
		 * (this only happens when switching to the new version) */
		if (name.empty()) {
			name = strrchr(filePath, '/') + 1;
			name.resize(name.size() - 5);
		}

		obs_data_release(data);

		if (!cb(name.c_str(), filePath))
			break;
	}

	os_globfree(glob);
}

static bool UICollectionExists(const char *findName)
{
	bool found = false;
	auto func = [&](const char *name, const char *) {
		if (strcmp(name, findName) == 0) {
			found = true;
			return false;
		}

		return true;
	};

	EnumUICollections(func);
	return found;
}

static bool GetUICollectionName(QWidget *parent, std::string &name,
				std::string &file,
				const char *oldName = nullptr)
{
	bool rename = oldName != nullptr;
	const char *title;
	const char *text;
	char path[512];
	size_t len;
	int ret;

	if (rename) {
		title = Str("Basic.Main.RenameUICollection.Title");
		text = Str("Basic.Main.AddUICollection.Text");
	} else {
		title = Str("Basic.Main.AddUICollection.Title");
		text = Str("Basic.Main.AddUICollection.Text");
	}

	for (;;) {
		bool success = NameDialog::AskForName(parent, title, text, name,
						      QT_UTF8(oldName));
		if (!success) {
			return false;
		}
		if (name.empty()) {
			OBSMessageBox::warning(parent,
					       QTStr("NoNameEntered.Title"),
					       QTStr("NoNameEntered.Text"));
			continue;
		}
		if (UICollectionExists(name.c_str())) {
			OBSMessageBox::warning(parent,
					       QTStr("NameExists.Title"),
					       QTStr("NameExists.Text"));
			continue;
		}
		break;
	}

	if (!GetFileSafeName(name.c_str(), file)) {
		blog(LOG_WARNING, "Failed to create safe file name for '%s'",
		     name.c_str());
		return false;
	}

	ret = GetConfigPath(path, sizeof(path), "obs-studio/basic/ui/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get ui collection config path");
		return false;
	}

	len = file.size();
	file.insert(0, path);

	if (!GetClosestUnusedFileName(file, "json")) {
		blog(LOG_WARNING, "Failed to get closest file name for %s",
		     file.c_str());
		return false;
	}

	file.erase(file.size() - 5, 5);
	file.erase(0, file.size() - len);
	return true;
}

bool OBSBasic::AddUICollection(bool create_new, const QString &qname)
{
	std::string name;
	std::string file;

	if (qname.isEmpty()) {
		if (!GetUICollectionName(this, name, file))
			return false;
	} else {
		name = QT_TO_UTF8(qname);
		if (UICollectionExists(name.c_str()))
			return false;
	}

	SaveUI();

	config_set_string(App()->GlobalConfig(), "Basic", "UICollection",
			  name.c_str());
	config_set_string(App()->GlobalConfig(), "Basic", "UICollectionFile",
			  file.c_str());
	if (create_new) {
		CreateDefaultUI(false);
	}
	SaveUI();
	RefreshUICollections();

	blog(LOG_INFO, "Added ui collection '%s' (%s, %s.json)", name.c_str(),
	     create_new ? "clean" : "duplicate", file.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	//if (api) { TODO
	//	api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
	//	api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
	//}

	return true;
}

void OBSBasic::RefreshUICollections()
{
	QList<QAction *> menuActions = ui->uiCollectionMenu->actions();
	int count = 0;

	for (int i = 0; i < menuActions.count(); i++) {
		QVariant v = menuActions[i]->property("file_name");
		if (v.typeName() != nullptr)
			delete menuActions[i];
	}

	const char *cur_name = config_get_string(App()->GlobalConfig(), "Basic",
						 "UICollection");

	auto addCollection = [&](const char *name, const char *path) {
		std::string file = strrchr(path, '/') + 1;
		file.erase(file.size() - 5, 5);

		QAction *action = new QAction(QT_UTF8(name), this);
		action->setProperty("file_name", QT_UTF8(path));
		connect(action, &QAction::triggered, this,
			&OBSBasic::ChangeUICollection);
		action->setCheckable(true);

		action->setChecked(strcmp(name, cur_name) == 0);

		ui->uiCollectionMenu->addAction(action);
		count++;
		return true;
	};

	EnumUICollections(addCollection);

	/* force saving of first ui collection on first run, otherwise
	 * no ui collections will show up */
	if (!count) {
		SaveUI();
		EnumUICollections(addCollection);
	}

	ui->actionRemoveUICollection->setEnabled(count > 1);

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	main->ui->actionPasteFilters->setEnabled(false);
	main->ui->actionPasteRef->setEnabled(false);
	main->ui->actionPasteDup->setEnabled(false);
}

void OBSBasic::on_actionNewUICollection_triggered()
{
	AddUICollection(true, QString(""));
}

void OBSBasic::on_actionDupUICollection_triggered()
{
	AddUICollection(false, QString(""));
}

void OBSBasic::on_actionRenameUICollection_triggered()
{
	std::string name;
	std::string file;

	std::string oldFile = config_get_string(App()->GlobalConfig(), "Basic",
						"UICollectionFile");
	const char *oldName = config_get_string(App()->GlobalConfig(), "Basic",
						"UICollection");

	bool success = GetUICollectionName(this, name, file, oldName);
	if (!success)
		return;

	config_set_string(App()->GlobalConfig(), "Basic", "UICollection",
			  name.c_str());
	config_set_string(App()->GlobalConfig(), "Basic", "UICollectionFile",
			  file.c_str());
	SaveUI();

	char path[512];
	int ret = GetConfigPath(path, 512, "obs-studio/basic/ui/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get ui collection config path");
		return;
	}

	oldFile.insert(0, path);
	oldFile += ".json";
	os_unlink(oldFile.c_str());
	oldFile += ".bak";
	os_unlink(oldFile.c_str());

	blog(LOG_INFO, "------------------------------------------------");
	blog(LOG_INFO, "Renamed ui collection to '%s' (%s.json)", name.c_str(),
	     file.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	RefreshUICollections();

	//if (api) { TODO
	//	api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
	//	api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
	//}
}

void OBSBasic::on_actionRemoveUICollection_triggered()
{
	std::string newName;
	std::string newPath;

	std::string oldFile = config_get_string(App()->GlobalConfig(), "Basic",
						"UICollectionFile");
	std::string oldName = config_get_string(App()->GlobalConfig(), "Basic",
						"UICollection");

	auto cb = [&](const char *name, const char *filePath) {
		if (strcmp(oldName.c_str(), name) != 0) {
			newName = name;
			newPath = filePath;
			return false;
		}

		return true;
	};

	EnumUICollections(cb);

	/* this should never be true due to menu item being grayed out */
	if (newPath.empty())
		return;

	QString text = QTStr("ConfirmRemove.Text");
	text.replace("$1", QT_UTF8(oldName.c_str()));

	QMessageBox::StandardButton button = OBSMessageBox::question(
		this, QTStr("ConfirmRemove.Title"), text);
	if (button == QMessageBox::No)
		return;

	char path[512];
	int ret = GetConfigPath(path, 512, "obs-studio/basic/ui/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get ui collection config path");
		return;
	}

	oldFile.insert(0, path);
	oldFile += ".json";
	os_unlink(oldFile.c_str());
	oldFile += ".bak";
	os_unlink(oldFile.c_str());

	LoadUI(newPath.c_str());
	RefreshUICollections();

	const char *newFile = config_get_string(App()->GlobalConfig(), "Basic",
						"UICollectionFile");

	blog(LOG_INFO,
	     "Removed ui collection '%s' (%s), "
	     "switched to '%s' (%s.json)",
	     oldName.c_str(), oldFile.c_str(), newName.c_str(), newFile);
	blog(LOG_INFO, "------------------------------------------------");

	blog(LOG_INFO, "%s",
	     config_get_string(App()->GlobalConfig(), "Basic",
			       "UICollectionFile"));
	//if (api) { TODO
	//	api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
	//	api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
	//}
}

void OBSBasic::on_actionImportUICollection_triggered()
{
	char path[512];

	QString qhome = QDir::homePath();

	int ret = GetConfigPath(path, 512, "obs-studio/basic/ui/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get ui collection config path");
		return;
	}

	QString qfilePath = QFileDialog::getOpenFileName(
		this, QTStr("Basic.MainMenu.UICollection.Import"), qhome,
		"JSON Files (*.json)");

	QFileInfo finfo(qfilePath);
	QString qfilename = finfo.fileName();
	QString qpath = QT_UTF8(path);
	QFileInfo destinfo(QT_UTF8(path) + qfilename);

	if (!qfilePath.isEmpty() && !qfilePath.isNull()) {
		string absPath = QT_TO_UTF8(finfo.absoluteFilePath());
		OBSData scenedata =
			obs_data_create_from_json_file(absPath.c_str());
		obs_data_release(scenedata);

		string origName = obs_data_get_string(scenedata, "name");
		string name = origName;
		string file;
		int inc = 1;

		while (UICollectionExists(name.c_str())) {
			name = origName + " (" + to_string(++inc) + ")";
		}

		obs_data_set_string(scenedata, "name", name.c_str());

		if (!GetFileSafeName(name.c_str(), file)) {
			blog(LOG_WARNING,
			     "Failed to create "
			     "safe file name for '%s'",
			     name.c_str());
			return;
		}

		string filePath = path + file;

		if (!GetClosestUnusedFileName(filePath, "json")) {
			blog(LOG_WARNING,
			     "Failed to get "
			     "closest file name for %s",
			     file.c_str());
			return;
		}

		obs_data_save_json_safe(scenedata, filePath.c_str(), "tmp",
					"bak");
		RefreshUICollections();
	}
}

void OBSBasic::on_actionExportUICollection_triggered()
{
	SaveProjectNow();

	char path[512];

	QString home = QDir::homePath();

	QString currentFile = QT_UTF8(config_get_string(
		App()->GlobalConfig(), "Basic", "UICollectionFile"));

	int ret = GetConfigPath(path, 512, "obs-studio/basic/ui/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get ui collection config path");
		return;
	}

	QString exportFile = QFileDialog::getSaveFileName(
		this, QTStr("Basic.MainMenu.UICollection.Export"),
		home + "/" + currentFile, "JSON Files (*.json)");

	string file = QT_TO_UTF8(exportFile);

	if (!exportFile.isEmpty() && !exportFile.isNull()) {
		if (QFile::exists(exportFile))
			QFile::remove(exportFile);

		QFile::copy(path + currentFile + ".json", exportFile);
	}
}

void OBSBasic::ChangeUICollection()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	std::string fileName;

	if (!action)
		return;

	fileName = QT_TO_UTF8(action->property("file_name").value<QString>());
	if (fileName.empty())
		return;

	const char *oldName = config_get_string(App()->GlobalConfig(), "Basic",
						"UICollection");
	if (action->text().compare(QT_UTF8(oldName)) == 0) {
		action->setChecked(true);
		return;
	}
	SaveUI();
	LoadUI(fileName.c_str());
	RefreshUICollections();

	const char *newName = config_get_string(App()->GlobalConfig(), "Basic",
						"UICollection");
	const char *newFile = config_get_string(App()->GlobalConfig(), "Basic",
						"UICollectionFile");

	blog(LOG_INFO, "Switched to ui collection '%s' (%s.json)", newName,
	     newFile);
	blog(LOG_INFO, "------------------------------------------------");

	//if (api) TODO
	//	api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
}

void OBSBasic::SaveUI()
{
	const char *uiCollection = config_get_string(
		App()->GlobalConfig(), "Basic", "UICollectionFile");

	char savePath[512];
	char fileName[512];
	int ret;

	if (!uiCollection)
		return;

	ret = snprintf(fileName, 512, "obs-studio/basic/ui/%s.json",
		       uiCollection);
	if (ret <= 0)
		return;

	ret = GetConfigPath(savePath, sizeof(savePath), fileName);
	if (ret <= 0)
		return;

	if (isVisible()) {
		const char *dockState = saveState().toBase64().constData();
		obs_data_t *saveData = obs_data_create();
		obs_data_set_string(saveData, "DockState", dockState);
		obs_data_set_string(saveData, "ExtraBrowserDocks",
				    ExtraBrowserSaveString().c_str());

		if (!obs_data_save_json_safe(saveData, savePath, "tmp", "bak"))
			blog(LOG_ERROR, "Could not save ui data to %s",
			     savePath);

		obs_data_release(saveData);
	}
}

void OBSBasic::LoadUI(const char *file)
{
	obs_data_t *data = obs_data_create_from_json_file_safe(file, "bak");
	if (!data) {
		blog(LOG_INFO, "No ui file found, creating default ui");
		CreateDefaultUI(false);
		SaveUI();
		obs_data_release(data);
		return;
	}

	LoadExtraBrowserDocks(obs_data_get_string(data, "ExtraBrowserDocks"));

	const char *dockStateStr =
			obs_data_get_string(data, "DockState");
	if (!dockStateStr) {
		CreateDefaultUI(false);
		blog(LOG_WARNING, "No dock state to hydrate");
	} else {
		QByteArray dockState =
			QByteArray::fromBase64(QByteArray(dockStateStr));
		if (!restoreState(dockState)) {
			blog(LOG_WARNING, "Unable to restore dock state");
			CreateDefaultUI(false);
		}
	}
	std::string name;
	name = strrchr(file, '/') + 1;
	name.resize(name.size() - 5);

	config_set_string(App()->GlobalConfig(), "Basic", "UICollection",
			  name.c_str());
	config_set_string(App()->GlobalConfig(), "Basic", "UICollectionFile",
			  name.c_str());

	obs_data_release(data);
}
