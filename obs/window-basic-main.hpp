/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#pragma once

#include <obs.hpp>
#include <unordered_map>
#include <memory>
#include "window-main.hpp"

class QListWidgetItem;

#include "ui_OBSBasic.h"

class OBSBasic : public OBSMainWindow {
	Q_OBJECT

private:
	std::unordered_map<obs_source_t, int> sourceSceneRefs;

	obs_scene_t GetCurrentScene();
	void AddSceneItem(obs_sceneitem_t item);
	void RemoveSceneItem(obs_sceneitem_t item);
	void AddScene(obs_source_t scene);
	void RemoveScene(obs_source_t scene);
	void UpdateSources(obs_scene_t scene);
	void UpdateSceneSelection(obs_source_t source);

	/* OBS Callbacks */
	static void SceneItemAdded(void *data, calldata_t params);
	static void SceneItemRemoved(void *data, calldata_t params);
	static void SourceAdded(void *data, calldata_t params);
	static void SourceDestroyed(void *data, calldata_t params);
	static void ChannelChanged(void *data, calldata_t params);

	void ResizePreview(uint32_t cx, uint32_t cy);

	void AddSource(obs_scene_t scene, const char *id);
	void AddSourcePopupMenu(const QPoint &pos);

	bool InitGraphics();
	bool InitAudio();

	void NewProject();
	void SaveProject();
	void LoadProject();

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual void changeEvent(QEvent *event) override;
	virtual void resizeEvent(QResizeEvent *event) override;

private slots:
	void on_action_New_triggered();
	void on_action_Open_triggered();
	void on_action_Save_triggered();
	void on_scenes_itemChanged(QListWidgetItem *item);
	void on_scenes_customContextMenuRequested(const QPoint &pos);
	void on_actionAddScene_triggered();
	void on_actionRemoveScene_triggered();
	void on_actionSceneProperties_triggered();
	void on_actionSceneUp_triggered();
	void on_actionSceneDown_triggered();
	void on_sources_itemChanged(QListWidgetItem *item);
	void on_sources_customContextMenuRequested(const QPoint &pos);
	void on_actionAddSource_triggered();
	void on_actionRemoveSource_triggered();
	void on_actionSourceProperties_triggered();
	void on_actionSourceUp_triggered();
	void on_actionSourceDown_triggered();
	void on_settingsButton_clicked();

public:
	explicit OBSBasic(QWidget *parent = 0);
	~OBSBasic();

	virtual void OBSInit() override;

private:
	std::unique_ptr<Ui::OBSBasic> ui;
};
