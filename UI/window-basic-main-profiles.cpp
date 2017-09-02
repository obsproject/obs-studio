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
#include <util/platform.h>
#include <util/util.hpp>
#include <QMessageBox>
#include <QVariant>
#include <QFileDialog>
#include "window-basic-main.hpp"
#include "window-namedialog.hpp"
#include "qt-wrappers.hpp"

void EnumProfiles(std::function<bool (const char *, const char *)> &&cb)
{
	char path[512];
	os_glob_t *glob;

	int ret = GetConfigPath(path, sizeof(path),
			"obs-studio/basic/profiles/*");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return;
	}

	if (os_glob(path, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob profiles");
		return;
	}

	for (size_t i = 0; i < glob->gl_pathc; i++) {
		const char *filePath = glob->gl_pathv[i].path;
		const char *dirName = strrchr(filePath, '/') + 1;

		if (!glob->gl_pathv[i].directory)
			continue;

		if (strcmp(dirName,  ".") == 0 ||
		    strcmp(dirName, "..") == 0)
			continue;

		std::string file = filePath;
		file += "/basic.ini";

		ConfigFile config;
		int ret = config.Open(file.c_str(), CONFIG_OPEN_EXISTING);
		if (ret != CONFIG_SUCCESS)
			continue;

		const char *name = config_get_string(config, "General", "Name");
		if (!name)
			name = strrchr(filePath, '/') + 1;

		if (!cb(name, filePath))
			break;
	}

	os_globfree(glob);
}

static bool ProfileExists(const char *findName)
{
	bool found = false;
	auto func = [&](const char *name, const char*)
	{
		if (strcmp(name, findName) == 0) {
			found = true;
			return false;
		}
		return true;
	};

	EnumProfiles(func);
	return found;
}

static bool GetProfileName(QWidget *parent, std::string &name,
		std::string &file, const char *title, const char *text,
		const char *oldName = nullptr)
{
	char path[512];
	int ret;

	for (;;) {
		bool success = NameDialog::AskForName(parent, title, text,
				name, QT_UTF8(oldName));
		if (!success) {
			return false;
		}
		if (name.empty()) {
			OBSMessageBox::information(parent,
					QTStr("NoNameEntered.Title"),
					QTStr("NoNameEntered.Text"));
			continue;
		}
		if (ProfileExists(name.c_str())) {
			OBSMessageBox::information(parent,
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

	ret = GetConfigPath(path, sizeof(path), "obs-studio/basic/profiles/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return false;
	}

	file.insert(0, path);

	if (!GetClosestUnusedFileName(file, nullptr)) {
		blog(LOG_WARNING, "Failed to get closest file name for %s",
				file.c_str());
		return false;
	}

	file.erase(0, ret);
	return true;
}

static bool CopyProfile(const char *fromPartial, const char *to)
{
	os_glob_t *glob;
	char path[512];
	int ret;

	ret = GetConfigPath(path, sizeof(path), "obs-studio/basic/profiles/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return false;
	}

	strcat(path, fromPartial);
	strcat(path, "/*");

	if (os_glob(path, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob profile '%s'", fromPartial);
		return false;
	}

	for (size_t i = 0; i < glob->gl_pathc; i++) {
		const char *filePath = glob->gl_pathv[i].path;
		if (glob->gl_pathv[i].directory)
			continue;

		ret = snprintf(path, sizeof(path), "%s/%s",
				to, strrchr(filePath, '/') + 1);
		if (ret > 0) {
			if (os_copyfile(filePath, path) != 0) {
				blog(LOG_WARNING, "CopyProfile: Failed to "
				                  "copy file %s to %s",
				                  filePath, path);
			}
		}
	}

	os_globfree(glob);

	return true;
}

bool OBSBasic::AddProfile(bool create_new, const char *title, const char *text,
		const char *init_text)
{
	std::string newName;
	std::string newDir;
	ConfigFile config;

	if (!GetProfileName(this, newName, newDir, title, text, init_text))
		return false;

	std::string curDir = config_get_string(App()->GlobalConfig(),
			"Basic", "ProfileDir");

	char newPath[512];
	int ret = GetConfigPath(newPath, 512, "obs-studio/basic/profiles/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return false;
	}

	strcat(newPath, newDir.c_str());

	if (os_mkdir(newPath) < 0) {
		blog(LOG_WARNING, "Failed to create profile directory '%s'",
				newDir.c_str());
		return false;
	}

	if (!create_new)
		CopyProfile(curDir.c_str(), newPath);

	strcat(newPath, "/basic.ini");

	if (config.Open(newPath, CONFIG_OPEN_ALWAYS) != 0) {
		blog(LOG_ERROR, "Failed to open new config file '%s'",
				newDir.c_str());
		return false;
	}

	config_set_string(App()->GlobalConfig(), "Basic", "Profile",
			newName.c_str());
	config_set_string(App()->GlobalConfig(), "Basic", "ProfileDir",
			newDir.c_str());

	config_set_string(config, "General", "Name", newName.c_str());
	config.SaveSafe("tmp");
	config.Swap(basicConfig);
	InitBasicConfigDefaults();
	RefreshProfiles();

	if (create_new)
		ResetProfileData();

	blog(LOG_INFO, "Created profile '%s' (%s, %s)", newName.c_str(),
			create_new ? "clean" : "duplicate", newDir.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	config_save_safe(App()->GlobalConfig(), "tmp", nullptr);
	UpdateTitleBar();

	if (api) {
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
	}
	return true;
}

void OBSBasic::DeleteProfile(const char *profileName, const char *profileDir)
{
	char profilePath[512];
	char basePath[512];

	int ret = GetConfigPath(basePath, 512, "obs-studio/basic/profiles");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return;
	}

	ret = snprintf(profilePath, 512, "%s/%s/*", basePath, profileDir);
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get path for profile dir '%s'",
				profileDir);
		return;
	}

	os_glob_t *glob;
	if (os_glob(profilePath, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob profile dir '%s'",
				profileDir);
		return;
	}

	for (size_t i = 0; i < glob->gl_pathc; i++) {
		const char *filePath = glob->gl_pathv[i].path;

		if (glob->gl_pathv[i].directory)
			continue;

		os_unlink(filePath);
	}

	os_globfree(glob);

	ret = snprintf(profilePath, 512, "%s/%s", basePath, profileDir);
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get path for profile dir '%s'",
				profileDir);
		return;
	}

	os_rmdir(profilePath);

	blog(LOG_INFO, "------------------------------------------------");
	blog(LOG_INFO, "Removed profile '%s' (%s)",
			profileName, profileDir);
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::RefreshProfiles()
{
	QList<QAction*> menuActions = ui->profileMenu->actions();
	int count = 0;

	for (int i = 0; i < menuActions.count(); i++) {
		QVariant v = menuActions[i]->property("file_name");
		if (v.typeName() != nullptr)
			delete menuActions[i];
	}

	const char *curName = config_get_string(App()->GlobalConfig(),
			"Basic", "Profile");

	auto addProfile = [&](const char *name, const char *path)
	{
		std::string file = strrchr(path, '/') + 1;

		QAction *action = new QAction(QT_UTF8(name), this);
		action->setProperty("file_name", QT_UTF8(path));
		connect(action, &QAction::triggered,
				this, &OBSBasic::ChangeProfile);
		action->setCheckable(true);

		action->setChecked(strcmp(name, curName) == 0);

		ui->profileMenu->addAction(action);
		count++;
		return true;
	};

	EnumProfiles(addProfile);

	ui->actionRemoveProfile->setEnabled(count > 1);
}

void OBSBasic::ResetProfileData()
{
	ResetVideo();
	service = nullptr;
	InitService();
	ResetOutputs();
	ClearHotkeys();
	CreateHotkeys();
}

void OBSBasic::on_actionNewProfile_triggered()
{
	AddProfile(true, Str("AddProfile.Title"), Str("AddProfile.Text"));
}

void OBSBasic::on_actionDupProfile_triggered()
{
	AddProfile(false, Str("AddProfile.Title"), Str("AddProfile.Text"));
}

void OBSBasic::on_actionRenameProfile_triggered()
{
	std::string curDir = config_get_string(App()->GlobalConfig(),
			"Basic", "ProfileDir");
	std::string curName = config_get_string(App()->GlobalConfig(),
			"Basic", "Profile");

	/* Duplicate and delete in case there are any issues in the process */
	bool success = AddProfile(false, Str("RenameProfile.Title"),
			Str("AddProfile.Text"), curName.c_str());
	if (success) {
		DeleteProfile(curName.c_str(), curDir.c_str());
		RefreshProfiles();
	}

	if (api) {
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
	}
}

void OBSBasic::on_actionRemoveProfile_triggered()
{
	std::string newName;
	std::string newPath;
	ConfigFile config;

	std::string oldDir = config_get_string(App()->GlobalConfig(),
			"Basic", "ProfileDir");
	std::string oldName = config_get_string(App()->GlobalConfig(),
			"Basic", "Profile");

	auto cb = [&](const char *name, const char *filePath)
	{
		if (strcmp(oldName.c_str(), name) != 0) {
			newName = name;
			newPath = filePath;
			return false;
		}

		return true;
	};

	EnumProfiles(cb);

	/* this should never be true due to menu item being grayed out */
	if (newPath.empty())
		return;

	QString text = QTStr("ConfirmRemove.Text");
	text.replace("$1", QT_UTF8(oldName.c_str()));

	QMessageBox::StandardButton button = OBSMessageBox::question(this,
			QTStr("ConfirmRemove.Title"), text);
	if (button == QMessageBox::No)
		return;

	size_t newPath_len = newPath.size();
	newPath += "/basic.ini";

	if (config.Open(newPath.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
		blog(LOG_ERROR, "ChangeProfile: Failed to load file '%s'",
				newPath.c_str());
		return;
	}

	newPath.resize(newPath_len);

	const char *newDir = strrchr(newPath.c_str(), '/') + 1;

	config_set_string(App()->GlobalConfig(), "Basic", "Profile",
			newName.c_str());
	config_set_string(App()->GlobalConfig(), "Basic", "ProfileDir",
			newDir);

	config.Swap(basicConfig);
	InitBasicConfigDefaults();
	ResetProfileData();
	DeleteProfile(oldName.c_str(), oldDir.c_str());
	RefreshProfiles();
	config_save_safe(App()->GlobalConfig(), "tmp", nullptr);

	blog(LOG_INFO, "Switched to profile '%s' (%s)",
			newName.c_str(), newDir);
	blog(LOG_INFO, "------------------------------------------------");

	UpdateTitleBar();

	if (api) {
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
	}
}

void OBSBasic::on_actionImportProfile_triggered()
{
	char path[512];

	QString home = QDir::homePath();

	int ret = GetConfigPath(path, 512, "obs-studio/basic/profiles/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profile config path");
		return;
	}

	QString dir = QFileDialog::getExistingDirectory(
			this,
			QTStr("Basic.MainMenu.Profile.Import"),
			home,
			QFileDialog::ShowDirsOnly |
			QFileDialog::DontResolveSymlinks);

	if (!dir.isEmpty() && !dir.isNull()) {
		QString inputPath = QString::fromUtf8(path);
		QFileInfo finfo(dir);
		QString directory = finfo.fileName();
		QString profileDir = inputPath + directory;
		QDir folder(profileDir);

		if (!folder.exists()) {
			folder.mkpath(profileDir);
			QFile::copy(dir + "/basic.ini",
					profileDir + "/basic.ini");
			QFile::copy(dir + "/service.json",
					profileDir + "/service.json");
			QFile::copy(dir + "/streamEncoder.json",
					profileDir + "/streamEncoder.json");
			QFile::copy(dir + "/recordEncoder.json",
					profileDir + "/recordEncoder.json");
			RefreshProfiles();
		} else {
			OBSMessageBox::information(this,
					QTStr("Basic.MainMenu.Profile.Import"),
					QTStr("Basic.MainMenu.Profile.Exists"));
		}
	}
}

void OBSBasic::on_actionExportProfile_triggered()
{
	char path[512];

	QString home = QDir::homePath();

	QString currentProfile =
		QString::fromUtf8(config_get_string(App()->GlobalConfig(),
		"Basic", "ProfileDir"));

	int ret = GetConfigPath(path, 512, "obs-studio/basic/profiles/");
	if (ret <= 0) {
		blog(LOG_WARNING, "Failed to get profile config path");
		return;
	}

	QString dir = QFileDialog::getExistingDirectory(
			this,
			QTStr("Basic.MainMenu.Profile.Export"),
			home,
			QFileDialog::ShowDirsOnly |
			QFileDialog::DontResolveSymlinks);

	if (!dir.isEmpty() && !dir.isNull()) {
		QString outputDir = dir + "/" + currentProfile;
		QString inputPath = QString::fromUtf8(path);
		QDir folder(outputDir);

		if (!folder.exists()) {
			folder.mkpath(outputDir);
		} else {
			if (QFile::exists(outputDir + "/basic.ini"))
				QFile::remove(outputDir + "/basic.ini");

			if (QFile::exists(outputDir + "/service.json"))
				QFile::remove(outputDir + "/service.json");

			if (QFile::exists(outputDir + "/streamEncoder.json"))
				QFile::remove(outputDir + "/streamEncoder.json");

			if (QFile::exists(outputDir + "/recordEncoder.json"))
				QFile::remove(outputDir + "/recordEncoder.json");
		}

		QFile::copy(inputPath + currentProfile + "/basic.ini",
				outputDir + "/basic.ini");
		QFile::copy(inputPath + currentProfile + "/service.json",
				outputDir + "/service.json");
		QFile::copy(inputPath + currentProfile + "/streamEncoder.json",
				outputDir + "/streamEncoder.json");
		QFile::copy(inputPath + currentProfile + "/recordEncoder.json",
				outputDir + "/recordEncoder.json");
	}
}

void OBSBasic::ChangeProfile()
{
	QAction *action = reinterpret_cast<QAction*>(sender());
	ConfigFile config;
	std::string path;

	if (!action)
		return;

	path = QT_TO_UTF8(action->property("file_name").value<QString>());
	if (path.empty())
		return;

	const char *oldName = config_get_string(App()->GlobalConfig(),
			"Basic", "Profile");
	if (action->text().compare(QT_UTF8(oldName)) == 0) {
		action->setChecked(true);
		return;
	}

	size_t path_len = path.size();
	path += "/basic.ini";

	if (config.Open(path.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
		blog(LOG_ERROR, "ChangeProfile: Failed to load file '%s'",
				path.c_str());
		return;
	}

	path.resize(path_len);

	const char *newName = config_get_string(config, "General", "Name");
	const char *newDir = strrchr(path.c_str(), '/') + 1;

	config_set_string(App()->GlobalConfig(), "Basic", "Profile", newName);
	config_set_string(App()->GlobalConfig(), "Basic", "ProfileDir",
			newDir);

	config.Swap(basicConfig);
	InitBasicConfigDefaults();
	ResetProfileData();
	RefreshProfiles();
	config_save_safe(App()->GlobalConfig(), "tmp", nullptr);
	UpdateTitleBar();

	blog(LOG_INFO, "Switched to profile '%s' (%s)",
			newName, newDir);
	blog(LOG_INFO, "------------------------------------------------");

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
}
