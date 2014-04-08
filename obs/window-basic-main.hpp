/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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
#include "window-basic-properties.hpp"

#include <util/util.hpp>

class QListWidgetItem;

#include "ui_OBSBasic.h"

class OBSBasic : public OBSMainWindow {
	Q_OBJECT

private:
	std::unordered_map<obs_source_t, int> sourceSceneRefs;
	obs_output_t outputTest;
	obs_encoder_t aac;
	obs_encoder_t x264;
	bool         sceneChanging;

	int          previewX,  previewY;
	float        previewScale;
	int          resizeTimer;

	ConfigFile   basicConfig;

	OBSBasicProperties *properties;

	void GetFPSCommon(uint32_t &num, uint32_t &den) const;
	void GetFPSInteger(uint32_t &num, uint32_t &den) const;
	void GetFPSFraction(uint32_t &num, uint32_t &den) const;
	void GetFPSNanoseconds(uint32_t &num, uint32_t &den) const;
	void GetConfigFPS(uint32_t &num, uint32_t &den) const;

	bool         InitBasicConfigDefaults();
	bool         InitBasicConfig();

	OBSScene     GetCurrentScene();
	OBSSceneItem GetCurrentSceneItem();

	void UpdateSources(OBSScene scene);
	void InsertSceneItem(obs_sceneitem_t item);

public slots:
	void OutputStart();
	void OutputStop(int errorcode);

private slots:
	void AddSceneItem(OBSSceneItem item);
	void RemoveSceneItem(OBSSceneItem item);
	void AddScene(OBSSource source);
	void RemoveScene(OBSSource source);
	void UpdateSceneSelection(OBSSource source);

private:
	/* OBS Callbacks */
	static void SceneItemAdded(void *data, calldata_t params);
	static void SceneItemRemoved(void *data, calldata_t params);
	static void SourceAdded(void *data, calldata_t params);
	static void SourceRemoved(void *data, calldata_t params);
	static void ChannelChanged(void *data, calldata_t params);
	static void RenderMain(void *data, uint32_t cx, uint32_t cy);

	void ResizePreview(uint32_t cx, uint32_t cy);

	void AddSource(obs_scene_t scene, const char *id);
	void AddSourcePopupMenu(const QPoint &pos);

public:
	bool ResetVideo();
	bool ResetAudio();

	void ResetAudioDevice(const char *sourceId, const char *deviceName,
			int channel);
	void ResetAudioDevices();

	void NewProject();
	void SaveProject();
	void LoadProject();

	void UnloadProperties();

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual void changeEvent(QEvent *event) override;
	virtual void resizeEvent(QResizeEvent *event) override;
	virtual void timerEvent(QTimerEvent *event) override;

private slots:
	void on_action_New_triggered();
	void on_action_Open_triggered();
	void on_action_Save_triggered();
	void on_scenes_currentItemChanged(QListWidgetItem *current,
			QListWidgetItem *prev);
	void on_scenes_customContextMenuRequested(const QPoint &pos);
	void on_actionAddScene_triggered();
	void on_actionRemoveScene_triggered();
	void on_actionSceneProperties_triggered();
	void on_actionSceneUp_triggered();
	void on_actionSceneDown_triggered();
	void on_sources_currentItemChanged(QListWidgetItem *current,
			QListWidgetItem *prev);
	void on_sources_customContextMenuRequested(const QPoint &pos);
	void on_actionAddSource_triggered();
	void on_actionRemoveSource_triggered();
	void on_actionSourceProperties_triggered();
	void on_actionSourceUp_triggered();
	void on_actionSourceDown_triggered();
	void on_streamButton_clicked();
	void on_settingsButton_clicked();

public:
	explicit OBSBasic(QWidget *parent = 0);
	virtual ~OBSBasic();

	virtual void OBSInit() override;

	virtual config_t Config() const override;

private:
	std::unique_ptr<Ui::OBSBasic> ui;
};
