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

#include <obs.hpp>
#include <util/util.hpp>
#include <QMessageBox>
#include <QVariant>
#include <QFileDialog>
#include <QStandardPaths>
#include <qt-wrappers.hpp>
#include "item-widget-helpers.hpp"
#include "window-basic-main.hpp"
#include "window-importer.hpp"
#include "window-namedialog.hpp"
#include "scene-collections-util.hpp"

using namespace std;

void OBSBasic::RefreshSceneCollections(bool refreshDialog)
{
	QList<QAction *> menuActions = ui->sceneCollectionMenu->actions();
	int count = 0;

	for (int i = menuActions.count() - 1; i >= 0; i--) {
		QVariant v = menuActions[i]->property("file_name");
		if (v.typeName() != nullptr)
			delete menuActions[i];
	}

	const char *cur_name = config_get_string(App()->GlobalConfig(), "Basic",
						 "SceneCollection");

	auto addCollection = [&](const char *name, const char *path, time_t) {
		// Only show the 10 most recent collections
		if (count >= 10)
			return false;

		std::string file = strrchr(path, '/') + 1;
		file.erase(file.size() - 5, 5);

		QAction *action = new QAction(QT_UTF8(name), this);
		action->setProperty("file_name", QT_UTF8(path));
		connect(action, &QAction::triggered, this,
			&OBSBasic::ActionChangeSceneCollection);
		action->setCheckable(true);

		action->setChecked(strcmp(name, cur_name) == 0);

		ui->sceneCollectionMenu->addAction(action);
		QAction *before = ui->sceneCollectionMenu->actions()[count];
		ui->sceneCollectionMenu->insertAction(before, action);
		count++;
		return true;
	};

	EnumSceneCollectionsOrdered(kSceneCollectionOrderLastUsed,
				    addCollection);

	/* force saving of first scene collection on first run, otherwise
	 * no scene collections will show up */
	if (!count) {
		long prevDisableVal = disableSaving;

		disableSaving = 0;
		SaveProjectNow();
		disableSaving = prevDisableVal;

		EnumSceneCollectionsOrdered(kSceneCollectionOrderLastUsed,
					    addCollection);
	}

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	main->ui->actionPasteFilters->setEnabled(false);
	main->ui->actionPasteRef->setEnabled(false);
	main->ui->actionPasteDup->setEnabled(false);

	if (refreshDialog && sceneCollectionsDialog)
		sceneCollectionsDialog->refreshList();
}

void OBSBasic::on_actionManageSceneCollections_triggered()
{
	if (sceneCollectionsDialog) {
		sceneCollectionsDialog->raise();
	} else {
		sceneCollectionsDialog = new OBSSceneCollections(this);
		connect(sceneCollectionsDialog.data(),
			&OBSSceneCollections::collectionsChanged, [this]() {
				UpdateTitleBar();
				RefreshSceneCollections(false);
			});
		sceneCollectionsDialog->show();
	}
}

void OBSBasic::ActionChangeSceneCollection()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	std::string fileName;

	if (!action)
		return;

	fileName = QT_TO_UTF8(action->property("file_name").value<QString>());
	if (fileName.empty())
		return;

	const char *oldName = config_get_string(App()->GlobalConfig(), "Basic",
						"SceneCollection");

	if (action->text().compare(QT_UTF8(oldName)) == 0) {
		action->setChecked(true);
		return;
	}

	ChangeSceneCollection(fileName);
}

void OBSBasic::ChangeSceneCollection(const std::string &fileName)
{
	if (api)
		api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

	SaveProjectNow();

	Load(fileName.c_str());
	RefreshSceneCollections(true);

	const char *newName = config_get_string(App()->GlobalConfig(), "Basic",
						"SceneCollection");
	const char *newFile = config_get_string(App()->GlobalConfig(), "Basic",
						"SceneCollectionFile");

	blog(LOG_INFO, "Switched to scene collection '%s' (%s.json)", newName,
	     newFile);
	blog(LOG_INFO, "------------------------------------------------");

	UpdateTitleBar();

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
}
