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
#include <QSystemTrayIcon>
#include <obs.hpp>
#include <vector>
#include <memory>
#include "window-main.hpp"
#include "window-basic-interaction.hpp"
#include "window-basic-properties.hpp"
#include "window-basic-transform.hpp"
#include "window-basic-adv-audio.hpp"
#include "window-basic-filters.hpp"

#include <obs-frontend-internal.hpp>

#include <util/platform.h>
#include <util/threading.h>
#include <util/util.hpp>

#include <QPointer>

class QMessageBox;
class QListWidgetItem;
class VolControl;
class QNetworkReply;
class OBSBasicStats;

#include "ui_OBSBasic.h"

#define DESKTOP_AUDIO_1 Str("DesktopAudioDevice1")
#define DESKTOP_AUDIO_2 Str("DesktopAudioDevice2")
#define AUX_AUDIO_1     Str("AuxAudioDevice1")
#define AUX_AUDIO_2     Str("AuxAudioDevice2")
#define AUX_AUDIO_3     Str("AuxAudioDevice3")

#define SIMPLE_ENCODER_X264                    "x264"
#define SIMPLE_ENCODER_X264_LOWCPU             "x264_lowcpu"
#define SIMPLE_ENCODER_QSV                     "qsv"
#define SIMPLE_ENCODER_NVENC                   "nvenc"
#define SIMPLE_ENCODER_AMD                     "amd"

#define PREVIEW_EDGE_SIZE 10

struct BasicOutputHandler;

enum class QtDataRole {
	OBSRef = Qt::UserRole,
	OBSSignals,
};

struct QuickTransition {
	QPushButton *button = nullptr;
	OBSSource source;
	obs_hotkey_id hotkey = 0;
	int duration = 0;
	int id = 0;

	inline QuickTransition() {}
	inline QuickTransition(OBSSource source_, int duration_, int id_)
		: source   (source_),
		  duration (duration_),
		  id       (id_)
	{}
};

class OBSBasic : public OBSMainWindow {
	Q_OBJECT

	friend class OBSBasicPreview;
	friend class OBSBasicStatusBar;
	friend class OBSBasicSourceSelect;
	friend class OBSBasicSettings;
	friend struct OBSStudioAPI;

	enum class MoveDir {
		Up,
		Down,
		Left,
		Right
	};

	enum DropType {
		DropType_RawText,
		DropType_Text,
		DropType_Image,
		DropType_Media
	};

private:
	obs_frontend_callbacks *api = nullptr;

	std::vector<VolControl*> volumes;

	std::vector<OBSSignal> signalHandlers;

	std::vector<std::string> projectorArray;
	std::vector<int> previewProjectorArray;

	bool loaded = false;
	long disableSaving = 1;
	bool projectChanged = false;
	bool previewEnabled = true;

	const char *copyString;
	const char *copyFiltersString;
	bool copyVisible = true;

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
	bool streamingStopping = false;
	bool recordingStopping = false;
	bool replayBufferStopping = false;

	gs_vertbuffer_t *box = nullptr;
	gs_vertbuffer_t *boxLeft = nullptr;
	gs_vertbuffer_t *boxTop = nullptr;
	gs_vertbuffer_t *boxRight = nullptr;
	gs_vertbuffer_t *boxBottom = nullptr;
	gs_vertbuffer_t *circle = nullptr;

	bool          sceneChanging = false;
	bool          ignoreSelectionUpdate = false;

	int           previewX = 0,  previewY = 0;
	int           previewCX = 0, previewCY = 0;
	float         previewScale = 0.0f;

	ConfigFile    basicConfig;

	QPointer<QWidget> projectors[10];
	QList<QPointer<QWidget>> windowProjectors;

	QPointer<QWidget> stats;

	QPointer<QMenu> startStreamMenu;

	QPointer<QPushButton> replayBufferButton;

	QPointer<QSystemTrayIcon> trayIcon;
	QPointer<QAction>         sysTrayStream;
	QPointer<QAction>         sysTrayRecord;
	QPointer<QAction>         sysTrayReplayBuffer;
	QPointer<QAction>         showHide;
	QPointer<QAction>         exit;
	QPointer<QMenu>           trayMenu;

	void          DrawBackdrop(float cx, float cy);

	void          SetupEncoders();

	void          CreateFirstRunSources();
	void          CreateDefaultScene(bool firstStart);

	void          ClearVolumeControls();

	void          UploadLog(const char *file);

	void          Save(const char *file);
	void          Load(const char *file);

	void          InitHotkeys();
	void          CreateHotkeys();
	void          ClearHotkeys();

	bool          InitService();

	bool          InitBasicConfigDefaults();
	bool          InitBasicConfig();

	void          InitOBSCallbacks();

	void          InitPrimitives();

	OBSSceneItem  GetSceneItem(QListWidgetItem *item);
	OBSSceneItem  GetCurrentSceneItem();

	bool          QueryRemoveSource(obs_source_t *source);

	void          TimedCheckForUpdates();
	void          CheckForUpdates(bool manualUpdate);

	void GetFPSCommon(uint32_t &num, uint32_t &den) const;
	void GetFPSInteger(uint32_t &num, uint32_t &den) const;
	void GetFPSFraction(uint32_t &num, uint32_t &den) const;
	void GetFPSNanoseconds(uint32_t &num, uint32_t &den) const;
	void GetConfigFPS(uint32_t &num, uint32_t &den) const;

	void UpdatePreviewScalingMenu();

	void UpdateSources(OBSScene scene);
	void InsertSceneItem(obs_sceneitem_t *item);

	void LoadSceneListOrder(obs_data_array_t *array);
	obs_data_array_t *SaveSceneListOrder();
	void ChangeSceneIndex(bool relative, int idx, int invalidIdx);

	void TempFileOutput(const char *path, int vBitrate, int aBitrate);
	void TempStreamOutput(const char *url, const char *key,
			int vBitrate, int aBitrate);

	void CreateInteractionWindow(obs_source_t *source);
	void CreatePropertiesWindow(obs_source_t *source);
	void CreateFiltersWindow(obs_source_t *source);

	void CloseDialogs();
	void ClearSceneData();

	void Nudge(int dist, MoveDir dir);
	void OpenProjector(obs_source_t *source, int monitor, bool window,
			QString title = nullptr);

	void GetAudioSourceFilters();
	void GetAudioSourceProperties();
	void VolControlContextMenu();

	void AddSceneCollection(bool create_new);
	void RefreshSceneCollections();
	void ChangeSceneCollection();
	void LogScenes();

	void LoadProfile();
	void ResetProfileData();
	bool AddProfile(bool create_new, const char *title, const char *text,
			const char *init_text = nullptr);
	void DeleteProfile(const char *profile_name, const char *profile_dir);
	void RefreshProfiles();
	void ChangeProfile();

	void SaveProjectNow();

	QListWidgetItem *GetTopSelectedSourceItem();

	obs_hotkey_pair_id streamingHotkeys, recordingHotkeys,
	                   replayBufHotkeys;
	obs_hotkey_id forceStreamingStopHotkey;

	void InitDefaultTransitions();
	void InitTransition(obs_source_t *transition);
	obs_source_t *FindTransition(const char *name);
	OBSSource GetCurrentTransition();
	obs_data_array_t *SaveTransitions();
	void LoadTransitions(obs_data_array_t *transitions);

	obs_source_t *fadeTransition;

	void CreateProgramDisplay();
	void CreateProgramOptions();
	void AddQuickTransitionId(int id);
	void AddQuickTransition();
	void AddQuickTransitionHotkey(QuickTransition *qt);
	void RemoveQuickTransitionHotkey(QuickTransition *qt);
	void LoadQuickTransitions(obs_data_array_t *array);
	obs_data_array_t *SaveQuickTransitions();
	void ClearQuickTransitionWidgets();
	void RefreshQuickTransitions();
	void CreateDefaultQuickTransitions();

	QuickTransition *GetQuickTransition(int id);
	int GetQuickTransitionIdx(int id);
	QMenu *CreateTransitionMenu(QWidget *parent, QuickTransition *qt);
	void ClearQuickTransitions();
	void QuickTransitionClicked();
	void QuickTransitionChange();
	void QuickTransitionChangeDuration(int value);
	void QuickTransitionRemoveClicked();

	void SetPreviewProgramMode(bool enabled);
	void ResizeProgram(uint32_t cx, uint32_t cy);
	void SetCurrentScene(obs_scene_t *scene, bool force = false);
	static void RenderProgram(void *data, uint32_t cx, uint32_t cy);

	std::vector<QuickTransition> quickTransitions;
	QPointer<QWidget> programOptions;
	QPointer<OBSQTDisplay> program;
	OBSWeakSource lastScene;
	OBSWeakSource swapScene;
	OBSWeakSource programScene;
	bool editPropertiesMode = false;
	bool sceneDuplicationMode = true;
	bool swapScenesMode = true;
	volatile bool previewProgramMode = false;
	obs_hotkey_id togglePreviewProgramHotkey = 0;
	obs_hotkey_id transitionHotkey = 0;
	int quickTransitionIdCounter = 1;

	int   programX = 0,  programY = 0;
	int   programCX = 0, programCY = 0;
	float programScale = 0.0f;

	int disableOutputsRef = 0;

	inline bool IsPreviewProgramMode() const
	{
		return os_atomic_load_bool(&previewProgramMode);
	}

	inline void OnActivate();
	inline void OnDeactivate();

	void AddDropSource(const char *file, DropType image);
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

	void ReplayBufferClicked();

	bool sysTrayMinimizeToTray();

	void EnumDialogs();

	QList<QDialog*> visDialogs;
	QList<QDialog*> modalDialogs;
	QList<QMessageBox*> visMsgBoxes;

	QList<QPoint> visDlgPositions;

	obs_data_array_t *SaveProjectors();
	void LoadSavedProjectors(obs_data_array_t *savedProjectors);

	obs_data_array_t *SavePreviewProjectors();
	void LoadSavedPreviewProjectors(
		obs_data_array_t *savedPreviewProjectors);

public slots:
	void StartStreaming();
	void StopStreaming();
	void ForceStopStreaming();

	void StreamDelayStarting(int sec);
	void StreamDelayStopping(int sec);

	void StreamingStart();
	void StreamStopping();
	void StreamingStop(int errorcode, QString last_error);

	void StartRecording();
	void StopRecording();

	void RecordingStart();
	void RecordStopping();
	void RecordingStop(int code);

	void StartReplayBuffer();
	void StopReplayBuffer();

	void ReplayBufferStart();
	void ReplayBufferStopping();
	void ReplayBufferStop(int code);

	void SaveProjectDeferred();
	void SaveProject();

	void SetTransition(OBSSource transition);
	void TransitionToScene(OBSScene scene, bool force = false);
	void TransitionToScene(OBSSource scene, bool force = false);
	void SetCurrentScene(OBSSource scene, bool force = false);

private slots:
	void AddSceneItem(OBSSceneItem item);
	void RemoveSceneItem(OBSSceneItem item);
	void AddScene(OBSSource source);
	void RemoveScene(OBSSource source);
	void RenameSources(QString newName, QString prevName);

	void SelectSceneItem(OBSScene scene, OBSSceneItem item, bool select);

	void ActivateAudioSource(OBSSource source);
	void DeactivateAudioSource(OBSSource source);

	void DuplicateSelectedScene();
	void RemoveSelectedScene();
	void RemoveSelectedSceneItem();

	void ToggleAlwaysOnTop();

	void ReorderSources(OBSScene scene);

	void ProcessHotkey(obs_hotkey_id id, bool pressed);

	void AddTransition();
	void RenameTransition();
	void TransitionClicked();
	void TransitionStopped();
	void TriggerQuickTransition(int id);

	void SetDeinterlacingMode();
	void SetDeinterlacingOrder();

	void SetScaleFilter();

	void IconActivated(QSystemTrayIcon::ActivationReason reason);
	void SetShowing(bool showing);

	void ToggleShowHide();

	void on_actionCopySource_triggered();
	void on_actionPasteRef_triggered();
	void on_actionPasteDup_triggered();

	void on_actionCopyFilters_triggered();
	void on_actionPasteFilters_triggered();

private:
	/* OBS Callbacks */
	static void SceneReordered(void *data, calldata_t *params);
	static void SceneItemAdded(void *data, calldata_t *params);
	static void SceneItemRemoved(void *data, calldata_t *params);
	static void SceneItemSelected(void *data, calldata_t *params);
	static void SceneItemDeselected(void *data, calldata_t *params);
	static void SourceLoaded(void *data, obs_source_t *source);
	static void SourceRemoved(void *data, calldata_t *params);
	static void SourceActivated(void *data, calldata_t *params);
	static void SourceDeactivated(void *data, calldata_t *params);
	static void SourceRenamed(void *data, calldata_t *params);
	static void RenderMain(void *data, uint32_t cx, uint32_t cy);

	void ResizePreview(uint32_t cx, uint32_t cy);

	void AddSource(const char *id);
	QMenu *CreateAddSourcePopupMenu();
	void AddSourcePopupMenu(const QPoint &pos);
	void copyActionsDynamicProperties();

	static void HotkeyTriggered(void *data, obs_hotkey_id id, bool pressed);

public:
	OBSScene      GetCurrentScene();

	void SysTrayNotify(const QString &text, QSystemTrayIcon::MessageIcon n);

	inline OBSSource GetCurrentSceneSource()
	{
		OBSScene curScene = GetCurrentScene();
		return OBSSource(obs_scene_get_source(curScene));
	}

	obs_service_t *GetService();
	void          SetService(obs_service_t *service);

	bool StreamingActive() const;
	bool Active() const;

	int  ResetVideo();
	bool ResetAudio();

	void ResetOutputs();

	void ResetAudioDevice(const char *sourceId, const char *deviceId,
			const char *deviceDesc, int channel);

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

	inline void EnableOutputs(bool enable)
	{
		if (enable) {
			if (--disableOutputsRef < 0)
				disableOutputsRef = 0;
		} else {
			disableOutputsRef++;
		}
	}

	void ReorderSceneItem(obs_sceneitem_t *item, size_t idx);

	QMenu *AddDeinterlacingMenu(obs_source_t *source);
	QMenu *AddScaleFilteringMenu(obs_sceneitem_t *item);
	void CreateSourcePopupMenu(QListWidgetItem *item, bool preview);

	void UpdateTitleBar();
	void UpdateSceneSelection(OBSSource source);

	void SystemTrayInit();
	void SystemTray(bool firstStarted);

	void OpenSavedProjectors();
	void RemoveSavedProjectors(int monitor);

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual void changeEvent(QEvent *event) override;

private slots:
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
	void on_actionCopyTransform_triggered();
	void on_actionPasteTransform_triggered();
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
	void on_actionSceneUp_triggered();
	void on_actionSceneDown_triggered();
	void on_sources_itemSelectionChanged();
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

	void on_actionLockPreview_triggered();

	void on_scalingMenu_aboutToShow();
	void on_actionScaleWindow_triggered();
	void on_actionScaleCanvas_triggered();
	void on_actionScaleOutput_triggered();

	void on_streamButton_clicked();
	void on_recordButton_clicked();
	void on_settingsButton_clicked();

	void on_actionWebsite_triggered();

	void on_preview_customContextMenuRequested(const QPoint &pos);
	void on_previewDisabledLabel_customContextMenuRequested(
			const QPoint &pos);

	void on_actionNewSceneCollection_triggered();
	void on_actionDupSceneCollection_triggered();
	void on_actionRenameSceneCollection_triggered();
	void on_actionRemoveSceneCollection_triggered();
	void on_actionImportSceneCollection_triggered();
	void on_actionExportSceneCollection_triggered();

	void on_actionNewProfile_triggered();
	void on_actionDupProfile_triggered();
	void on_actionRenameProfile_triggered();
	void on_actionRemoveProfile_triggered();
	void on_actionImportProfile_triggered();
	void on_actionExportProfile_triggered();

	void on_actionShowSettingsFolder_triggered();
	void on_actionShowProfileFolder_triggered();

	void on_actionAlwaysOnTop_triggered();

	void on_toggleSceneTransitions_toggled(bool visible);
	void on_toggleListboxToolbars_toggled(bool visible);
	void on_toggleStatusBar_toggled(bool visible);

	void on_transitions_currentIndexChanged(int index);
	void on_transitionAdd_clicked();
	void on_transitionRemove_clicked();
	void on_transitionProps_clicked();

	void on_modeSwitch_clicked();

	void on_autoConfigure_triggered();
	void on_stats_triggered();

	void logUploadFinished(const QString &text, const QString &error);

	void updateCheckFinished();

	void AddSourceFromAction();

	void MoveSceneToTop();
	void MoveSceneToBottom();

	void EditSceneName();
	void EditSceneItemName();

	void SceneNameEdited(QWidget *editor,
			QAbstractItemDelegate::EndEditHint endHint);
	void SceneItemNameEdited(QWidget *editor,
			QAbstractItemDelegate::EndEditHint endHint);

	void OpenSceneFilters();
	void OpenFilters();

	void EnablePreviewDisplay(bool enable);
	void TogglePreview();

	void NudgeUp();
	void NudgeDown();
	void NudgeLeft();
	void NudgeRight();

	void OpenPreviewProjector();
	void OpenSourceProjector();
	void OpenSceneProjector();

	void OpenPreviewWindow();
	void OpenSourceWindow();
	void OpenSceneWindow();

public slots:
	void on_actionResetTransform_triggered();

public:
	explicit OBSBasic(QWidget *parent = 0);
	virtual ~OBSBasic();

	virtual void OBSInit() override;

	virtual config_t *Config() const override;

	virtual int GetProfilePath(char *path, size_t size, const char *file)
		const override;

private:
	std::unique_ptr<Ui::OBSBasic> ui;
};
