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

#include <QBuffer>
#include <QAction>
#include <obs.hpp>
#include <unordered_map>
#include <vector>
#include <memory>
#include "window-main.hpp"
#include "window-basic-interaction.hpp"
#include "window-basic-properties.hpp"
#include "window-basic-transform.hpp"
#include "window-basic-adv-audio.hpp"
#include "window-basic-filters.hpp"

#include <util/platform.h>
#include <util/util.hpp>

#include <QPointer>

class QListWidgetItem;
class VolControl;
class QNetworkReply;

#include "ui_OBSBasic.h"

#define DESKTOP_AUDIO_1 Str("DesktopAudioDevice1")
#define DESKTOP_AUDIO_2 Str("DesktopAudioDevice2")
#define AUX_AUDIO_1     Str("AuxAudioDevice1")
#define AUX_AUDIO_2     Str("AuxAudioDevice2")
#define AUX_AUDIO_3     Str("AuxAudioDevice3")

struct BasicOutputHandler;

class OBSBasic : public OBSMainWindow {
	Q_OBJECT

	friend class OBSBasicPreview;

	enum class MoveDir {
		Up,
		Down,
		Left,
		Right
	};

private:
	std::unordered_map<obs_source_t*, int> sourceSceneRefs;

	std::vector<VolControl*> volumes;

	bool loaded = false;

	QPointer<QTimer> saveTimer;

	QPointer<QThread> updateCheckThread;
	QPointer<QThread> logUploadThread;

	QPointer<OBSBasicInteraction> interaction;
	QPointer<OBSBasicProperties> properties;
	QPointer<OBSBasicTransform> transformWindow;
	QPointer<OBSBasicAdvAudio> advAudioWindow;
	QPointer<OBSBasicFilters> filters;

	QPointer<QTimer>    cpuUsageTimer;
	os_cpu_usage_info_t *cpuUsageInfo = nullptr;

	OBSService service;
	std::unique_ptr<BasicOutputHandler> outputHandler;

	gs_vertbuffer_t *box = nullptr;
	gs_vertbuffer_t *circle = nullptr;

	bool          sceneChanging = false;

	int           previewX = 0,  previewY = 0;
	int           previewCX = 0, previewCY = 0;
	float         previewScale = 0.0f;
	int           resizeTimer = 0;

	ConfigFile    basicConfig;

	QPointer<QWidget> projectors[10];

	void          DrawBackdrop(float cx, float cy);

	void          SetupEncoders();

	void          CreateDefaultScene();

	void          ClearVolumeControls();

	void          UploadLog(const char *file);

	void          Save(const char *file);
	void          Load(const char *file);

	void          InitHotkeys();
	void          CreateHotkeys();

	bool          InitService();

	bool          InitBasicConfigDefaults();
	bool          InitBasicConfig();

	void          InitOBSCallbacks();

	void          InitPrimitives();

	OBSSceneItem  GetSceneItem(QListWidgetItem *item);
	OBSSceneItem  GetCurrentSceneItem();

	bool          QueryRemoveSource(obs_source_t *source);

	void          TimedCheckForUpdates();
	void          CheckForUpdates();

	void GetFPSCommon(uint32_t &num, uint32_t &den) const;
	void GetFPSInteger(uint32_t &num, uint32_t &den) const;
	void GetFPSFraction(uint32_t &num, uint32_t &den) const;
	void GetFPSNanoseconds(uint32_t &num, uint32_t &den) const;
	void GetConfigFPS(uint32_t &num, uint32_t &den) const;

	void UpdateSources(OBSScene scene);
	void InsertSceneItem(obs_sceneitem_t *item);

	void TempFileOutput(const char *path, int vBitrate, int aBitrate);
	void TempStreamOutput(const char *url, const char *key,
			int vBitrate, int aBitrate);

	void CreateInteractionWindow(obs_source_t *source);
	void CreatePropertiesWindow(obs_source_t *source);
	void CreateFiltersWindow(obs_source_t *source);

	void Nudge(int dist, MoveDir dir);
	void OpenProjector(obs_source_t *source, int monitor);

	obs_hotkey_pair_id streamingHotkeys, recordingHotkeys;

public slots:
	void StartStreaming();
	void StopStreaming();

	void StreamingStart();
	void StreamingStop(int errorcode);

	void StartRecording();
	void StopRecording();

	void RecordingStart();
	void RecordingStop(int code);

	void SaveProject();

private slots:
	void AddSceneItem(OBSSceneItem item);
	void RemoveSceneItem(OBSSceneItem item);
	void AddScene(OBSSource source);
	void RemoveScene(OBSSource source);
	void UpdateSceneSelection(OBSSource source);
	void RenameSources(QString newName, QString prevName);

	void SelectSceneItem(OBSScene scene, OBSSceneItem item, bool select);
	void MoveSceneItem(OBSSceneItem item, obs_order_movement movement);

	void ActivateAudioSource(OBSSource source);
	void DeactivateAudioSource(OBSSource source);

	void RemoveSelectedScene();
	void RemoveSelectedSceneItem();

	void ReorderSources(OBSScene scene);

	void ProcessHotkey(obs_hotkey_id id, bool pressed);

private:
	/* OBS Callbacks */
	static void SceneReordered(void *data, calldata_t *params);
	static void SceneItemAdded(void *data, calldata_t *params);
	static void SceneItemRemoved(void *data, calldata_t *params);
	static void SceneItemSelected(void *data, calldata_t *params);
	static void SceneItemDeselected(void *data, calldata_t *params);
	static void SourceAdded(void *data, calldata_t *params);
	static void SourceRemoved(void *data, calldata_t *params);
	static void SourceActivated(void *data, calldata_t *params);
	static void SourceDeactivated(void *data, calldata_t *params);
	static void SourceRenamed(void *data, calldata_t *params);
	static void ChannelChanged(void *data, calldata_t *params);
	static void RenderMain(void *data, uint32_t cx, uint32_t cy);

	void ResizePreview(uint32_t cx, uint32_t cy);

	void AddSource(const char *id);
	QMenu *CreateAddSourcePopupMenu();
	void AddSourcePopupMenu(const QPoint &pos);
	void copyActionsDynamicProperties();

	static void HotkeyTriggered(void *data, obs_hotkey_id id, bool pressed);

public:
	OBSScene      GetCurrentScene();

	obs_service_t *GetService();
	void          SetService(obs_service_t *service);

	bool StreamingActive();

	int  ResetVideo();
	bool ResetAudio();

	void ResetOutputs();

	void ResetAudioDevice(const char *sourceId, const char *deviceName,
			const char *deviceDesc, int channel);
	void ResetAudioDevices();

	void NewProject();
	void LoadProject();

	inline void GetDisplayRect(int &x, int &y, int &cx, int &cy)
	{
		x  = previewX;
		y  = previewY;
		cx = previewCX;
		cy = previewCY;
	}

	inline double GetCPUUsage() const
	{
		return os_cpu_usage_info_query(cpuUsageInfo);
	}

	void SaveService();
	bool LoadService();

	void ReorderSceneItem(obs_sceneitem_t *item, size_t idx);

	void CreateSourcePopupMenu(QListWidgetItem *item, bool preview);

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual void changeEvent(QEvent *event) override;
	virtual void resizeEvent(QResizeEvent *event) override;
	virtual void timerEvent(QTimerEvent *event) override;

private slots:
	void on_action_New_triggered();
	void on_action_Open_triggered();
	void on_action_Save_triggered();
	void on_actionShow_Recordings_triggered();
	void on_actionRemux_triggered();
	void on_action_Settings_triggered();
	void on_actionAdvAudioProperties_triggered();
	void on_advAudioProps_clicked();
	void on_advAudioProps_destroyed();
	void on_actionShowLogs_triggered();
	void on_actionUploadCurrentLog_triggered();
	void on_actionUploadLastLog_triggered();
	void on_actionViewCurrentLog_triggered();
	void on_actionCheckForUpdates_triggered();

	void on_actionEditTransform_triggered();
	void on_actionResetTransform_triggered();
	void on_actionRotate90CW_triggered();
	void on_actionRotate90CCW_triggered();
	void on_actionRotate180_triggered();
	void on_actionFlipHorizontal_triggered();
	void on_actionFlipVertical_triggered();
	void on_actionFitToScreen_triggered();
	void on_actionStretchToScreen_triggered();
	void on_actionCenterToScreen_triggered();

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
	void on_sources_itemDoubleClicked(QListWidgetItem *item);
	void on_actionAddSource_triggered();
	void on_actionRemoveSource_triggered();
	void on_actionInteract_triggered();
	void on_actionSourceProperties_triggered();
	void on_actionSourceUp_triggered();
	void on_actionSourceDown_triggered();

	void on_actionMoveUp_triggered();
	void on_actionMoveDown_triggered();
	void on_actionMoveToTop_triggered();
	void on_actionMoveToBottom_triggered();

	void on_streamButton_clicked();
	void on_recordButton_clicked();
	void on_settingsButton_clicked();

	void on_preview_customContextMenuRequested(const QPoint &pos);
	void on_previewDisabledLabel_customContextMenuRequested(
			const QPoint &pos);

	void logUploadFinished(const QString &text, const QString &error);

	void updateFileFinished(const QString &text, const QString &error);

	void AddSourceFromAction();

	void EditSceneName();
	void EditSceneItemName();

	void SceneNameEdited(QWidget *editor,
			QAbstractItemDelegate::EndEditHint endHint);
	void SceneItemNameEdited(QWidget *editor,
			QAbstractItemDelegate::EndEditHint endHint);

	void OpenSceneFilters();
	void OpenFilters();

	void TogglePreview();

	void NudgeUp();
	void NudgeDown();
	void NudgeLeft();
	void NudgeRight();

	void OpenPreviewProjector();
	void OpenSourceProjector();
	void OpenSceneProjector();

public:
	explicit OBSBasic(QWidget *parent = 0);
	virtual ~OBSBasic();

	virtual void OBSInit() override;

	virtual config_t *Config() const override;

private:
	std::unique_ptr<Ui::OBSBasic> ui;
};
