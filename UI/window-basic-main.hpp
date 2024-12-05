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

#pragma once

#include <QBuffer>
#include <QAction>
#include <QThread>
#include <QWidgetAction>
#include <QSystemTrayIcon>
#include <QStyledItemDelegate>
#include <obs.hpp>
#include <vector>
#include <memory>
#include <future>
#include "window-main.hpp"
#include "window-basic-interaction.hpp"
#include "window-basic-vcam.hpp"
#include "window-basic-properties.hpp"
#include "window-basic-transform.hpp"
#include "window-basic-adv-audio.hpp"
#include "window-basic-filters.hpp"
#include "window-missing-files.hpp"
#include "window-projector.hpp"
#include "window-basic-about.hpp"
#ifdef YOUTUBE_ENABLED
#include "window-dock-youtube-app.hpp"
#endif
#include "auth-base.hpp"
#include "log-viewer.hpp"
#include "undo-stack-obs.hpp"

#include <obs-frontend-internal.hpp>

#include <util/platform.h>
#include <util/threading.h>
#include <util/util.hpp>

#include <QPointer>

class QMessageBox;
class QListWidgetItem;
class VolControl;
class OBSBasicStats;
class OBSBasicVCamConfig;

#include "ui_OBSBasic.h"
#include "ui_ColorSelect.h"

#define DESKTOP_AUDIO_1 Str("DesktopAudioDevice1")
#define DESKTOP_AUDIO_2 Str("DesktopAudioDevice2")
#define AUX_AUDIO_1 Str("AuxAudioDevice1")
#define AUX_AUDIO_2 Str("AuxAudioDevice2")
#define AUX_AUDIO_3 Str("AuxAudioDevice3")
#define AUX_AUDIO_4 Str("AuxAudioDevice4")

#define SIMPLE_ENCODER_X264 "x264"
#define SIMPLE_ENCODER_X264_LOWCPU "x264_lowcpu"
#define SIMPLE_ENCODER_QSV "qsv"
#define SIMPLE_ENCODER_QSV_AV1 "qsv_av1"
#define SIMPLE_ENCODER_NVENC "nvenc"
#define SIMPLE_ENCODER_NVENC_AV1 "nvenc_av1"
#define SIMPLE_ENCODER_NVENC_HEVC "nvenc_hevc"
#define SIMPLE_ENCODER_AMD "amd"
#define SIMPLE_ENCODER_AMD_HEVC "amd_hevc"
#define SIMPLE_ENCODER_AMD_AV1 "amd_av1"
#define SIMPLE_ENCODER_APPLE_H264 "apple_h264"
#define SIMPLE_ENCODER_APPLE_HEVC "apple_hevc"

#define PREVIEW_EDGE_SIZE 10

struct BasicOutputHandler;

enum class QtDataRole {
	OBSRef = Qt::UserRole,
	OBSSignals,
};

struct SavedProjectorInfo {
	ProjectorType type;
	int monitor;
	std::string geometry;
	std::string name;
	bool alwaysOnTop;
	bool alwaysOnTopOverridden;
};

struct SourceCopyInfo {
	OBSWeakSource weak_source;
	bool visible;
	obs_sceneitem_crop crop;
	obs_transform_info transform;
	obs_blending_method blend_method;
	obs_blending_type blend_mode;
};

struct QuickTransition {
	QPushButton *button = nullptr;
	OBSSource source;
	obs_hotkey_id hotkey = OBS_INVALID_HOTKEY_ID;
	int duration = 0;
	int id = 0;
	bool fadeToBlack = false;

	inline QuickTransition() {}
	inline QuickTransition(OBSSource source_, int duration_, int id_, bool fadeToBlack_ = false)
		: source(source_),
		  duration(duration_),
		  id(id_),
		  fadeToBlack(fadeToBlack_),
		  renamedSignal(std::make_shared<OBSSignal>(obs_source_get_signal_handler(source), "rename",
							    SourceRenamed, this))
	{
	}

private:
	static void SourceRenamed(void *param, calldata_t *data);
	std::shared_ptr<OBSSignal> renamedSignal;
};

struct OBSProfile {
	std::string name;
	std::string directoryName;
	std::filesystem::path path;
	std::filesystem::path profileFile;
};

struct OBSSceneCollection {
	std::string name;
	std::string fileName;
	std::filesystem::path collectionFile;
};

struct OBSPromptResult {
	bool success;
	std::string promptValue;
	bool optionValue;
};

struct OBSPromptRequest {
	std::string title;
	std::string prompt;
	std::string promptValue;
	bool withOption;
	std::string optionPrompt;
	bool optionValue;
};

using OBSPromptCallback = std::function<bool(const OBSPromptResult &result)>;

using OBSProfileCache = std::map<std::string, OBSProfile>;
using OBSSceneCollectionCache = std::map<std::string, OBSSceneCollection>;

class ColorSelect : public QWidget {

public:
	explicit ColorSelect(QWidget *parent = 0);

private:
	std::unique_ptr<Ui::ColorSelect> ui;
};

class OBSBasic : public OBSMainWindow {
	Q_OBJECT
	Q_PROPERTY(QIcon imageIcon READ GetImageIcon WRITE SetImageIcon DESIGNABLE true)
	Q_PROPERTY(QIcon colorIcon READ GetColorIcon WRITE SetColorIcon DESIGNABLE true)
	Q_PROPERTY(QIcon slideshowIcon READ GetSlideshowIcon WRITE SetSlideshowIcon DESIGNABLE true)
	Q_PROPERTY(QIcon audioInputIcon READ GetAudioInputIcon WRITE SetAudioInputIcon DESIGNABLE true)
	Q_PROPERTY(QIcon audioOutputIcon READ GetAudioOutputIcon WRITE SetAudioOutputIcon DESIGNABLE true)
	Q_PROPERTY(QIcon desktopCapIcon READ GetDesktopCapIcon WRITE SetDesktopCapIcon DESIGNABLE true)
	Q_PROPERTY(QIcon windowCapIcon READ GetWindowCapIcon WRITE SetWindowCapIcon DESIGNABLE true)
	Q_PROPERTY(QIcon gameCapIcon READ GetGameCapIcon WRITE SetGameCapIcon DESIGNABLE true)
	Q_PROPERTY(QIcon cameraIcon READ GetCameraIcon WRITE SetCameraIcon DESIGNABLE true)
	Q_PROPERTY(QIcon textIcon READ GetTextIcon WRITE SetTextIcon DESIGNABLE true)
	Q_PROPERTY(QIcon mediaIcon READ GetMediaIcon WRITE SetMediaIcon DESIGNABLE true)
	Q_PROPERTY(QIcon browserIcon READ GetBrowserIcon WRITE SetBrowserIcon DESIGNABLE true)
	Q_PROPERTY(QIcon groupIcon READ GetGroupIcon WRITE SetGroupIcon DESIGNABLE true)
	Q_PROPERTY(QIcon sceneIcon READ GetSceneIcon WRITE SetSceneIcon DESIGNABLE true)
	Q_PROPERTY(QIcon defaultIcon READ GetDefaultIcon WRITE SetDefaultIcon DESIGNABLE true)
	Q_PROPERTY(QIcon audioProcessOutputIcon READ GetAudioProcessOutputIcon WRITE SetAudioProcessOutputIcon
			   DESIGNABLE true)

	friend class OBSAbout;
	friend class OBSBasicPreview;
	friend class OBSBasicStatusBar;
	friend class OBSBasicSourceSelect;
	friend class OBSBasicTransform;
	friend class OBSBasicSettings;
	friend class Auth;
	friend class AutoConfig;
	friend class AutoConfigStreamPage;
	friend class RecordButton;
	friend class ControlsSplitButton;
	friend class ExtraBrowsersModel;
	friend class ExtraBrowsersDelegate;
	friend class DeviceCaptureToolbar;
	friend class OBSBasicSourceSelect;
	friend class OBSYoutubeActions;
	friend class OBSPermissions;
	friend struct BasicOutputHandler;
	friend struct OBSStudioAPI;
	friend class ScreenshotObj;

	enum class MoveDir { Up, Down, Left, Right };

	enum DropType {
		DropType_RawText,
		DropType_Text,
		DropType_Image,
		DropType_Media,
		DropType_Html,
		DropType_Url,
	};

	enum ContextBarSize { ContextBarSize_Minimized, ContextBarSize_Reduced, ContextBarSize_Normal };

	enum class CenterType {
		Scene,
		Vertical,
		Horizontal,
	};

private:
	obs_frontend_callbacks *api = nullptr;

	std::shared_ptr<Auth> auth;

	std::vector<VolControl *> volumes;

	std::vector<OBSSignal> signalHandlers;

	QList<QPointer<QDockWidget>> oldExtraDocks;
	QStringList oldExtraDockNames;

	OBSDataAutoRelease collectionModuleData;
	std::vector<OBSDataAutoRelease> safeModeTransitions;

	bool loaded = false;
	long disableSaving = 1;
	bool projectChanged = false;
	bool previewEnabled = true;
	ContextBarSize contextBarSize = ContextBarSize_Normal;

	std::deque<SourceCopyInfo> clipboard;
	OBSWeakSourceAutoRelease copyFiltersSource;
	bool copyVisible = true;
	obs_transform_info copiedTransformInfo;
	obs_sceneitem_crop copiedCropInfo;
	bool hasCopiedTransform = false;
	OBSWeakSourceAutoRelease copySourceTransition;
	int copySourceTransitionDuration;

	bool closing = false;
	bool clearingFailed = false;

	QScopedPointer<QThread> devicePropertiesThread;
	QScopedPointer<QThread> whatsNewInitThread;
	QScopedPointer<QThread> updateCheckThread;
	QScopedPointer<QThread> introCheckThread;
	QScopedPointer<QThread> logUploadThread;

	QPointer<OBSBasicInteraction> interaction;
	QPointer<OBSBasicProperties> properties;
	QPointer<OBSBasicTransform> transformWindow;
	QPointer<OBSBasicAdvAudio> advAudioWindow;
	QPointer<OBSBasicFilters> filters;
	QPointer<QDockWidget> statsDock;
#ifdef YOUTUBE_ENABLED
	QPointer<YouTubeAppDock> youtubeAppDock;
	uint64_t lastYouTubeAppDockCreationTime = 0;
#endif
	QPointer<OBSAbout> about;
	QPointer<OBSMissingFiles> missDialog;
	QPointer<OBSLogViewer> logView;

	QPointer<QTimer> cpuUsageTimer;
	QPointer<QTimer> diskFullTimer;

	QPointer<QTimer> nudge_timer;
	bool recent_nudge = false;

	os_cpu_usage_info_t *cpuUsageInfo = nullptr;

	OBSService service;
	std::unique_ptr<BasicOutputHandler> outputHandler;
	std::shared_future<void> setupStreamingGuard;
	bool streamingStopping = false;
	bool recordingStopping = false;
	bool replayBufferStopping = false;

	gs_vertbuffer_t *box = nullptr;
	gs_vertbuffer_t *boxLeft = nullptr;
	gs_vertbuffer_t *boxTop = nullptr;
	gs_vertbuffer_t *boxRight = nullptr;
	gs_vertbuffer_t *boxBottom = nullptr;
	gs_vertbuffer_t *circle = nullptr;

	gs_vertbuffer_t *actionSafeMargin = nullptr;
	gs_vertbuffer_t *graphicsSafeMargin = nullptr;
	gs_vertbuffer_t *fourByThreeSafeMargin = nullptr;
	gs_vertbuffer_t *leftLine = nullptr;
	gs_vertbuffer_t *topLine = nullptr;
	gs_vertbuffer_t *rightLine = nullptr;

	int previewX = 0, previewY = 0;
	int previewCX = 0, previewCY = 0;
	float previewScale = 0.0f;

	ConfigFile activeConfiguration;

	std::vector<SavedProjectorInfo *> savedProjectorsArray;
	std::vector<OBSProjector *> projectors;

	QPointer<QWidget> stats;
	QPointer<QWidget> remux;
	QPointer<QWidget> extraBrowsers;
	QPointer<QWidget> importer;

	QPointer<QPushButton> transitionButton;

	bool vcamEnabled = false;
	VCamConfig vcamConfig;

	QScopedPointer<QSystemTrayIcon> trayIcon;
	QPointer<QAction> sysTrayStream;
	QPointer<QAction> sysTrayRecord;
	QPointer<QAction> sysTrayReplayBuffer;
	QPointer<QAction> sysTrayVirtualCam;
	QPointer<QAction> showHide;
	QPointer<QAction> exit;
	QPointer<QMenu> trayMenu;
	QPointer<QMenu> previewProjector;
	QPointer<QMenu> studioProgramProjector;
	QPointer<QMenu> previewProjectorSource;
	QPointer<QMenu> previewProjectorMain;
	QPointer<QMenu> sceneProjectorMenu;
	QPointer<QMenu> sourceProjector;
	QPointer<QMenu> scaleFilteringMenu;
	QPointer<QMenu> blendingMethodMenu;
	QPointer<QMenu> blendingModeMenu;
	QPointer<QMenu> colorMenu;
	QPointer<QWidgetAction> colorWidgetAction;
	QPointer<ColorSelect> colorSelect;
	QPointer<QMenu> deinterlaceMenu;
	QPointer<QMenu> perSceneTransitionMenu;
	QPointer<QObject> shortcutFilter;
	QPointer<QAction> renameScene;
	QPointer<QAction> renameSource;

	QPointer<QWidget> programWidget;
	QPointer<QVBoxLayout> programLayout;
	QPointer<QLabel> programLabel;

	QScopedPointer<QThread> patronJsonThread;
	std::string patronJson;

	std::atomic<obs_scene_t *> currentScene = nullptr;
	std::optional<std::pair<uint32_t, uint32_t>> lastOutputResolution;
	std::optional<std::pair<uint32_t, uint32_t>> migrationBaseResolution;
	bool usingAbsoluteCoordinates = false;

	void DisableRelativeCoordinates(bool disable);

	void OnEvent(enum obs_frontend_event event);

	void UpdateMultiviewProjectorMenu();

	void DrawBackdrop(float cx, float cy);

	void SetupEncoders();

	void CreateFirstRunSources();
	void CreateDefaultScene(bool firstStart);

	void UpdateVolumeControlsDecayRate();
	void UpdateVolumeControlsPeakMeterType();
	void ClearVolumeControls();

	void UploadLog(const char *subdir, const char *file, const bool crash);

	void Save(const char *file);
	void LoadData(obs_data_t *data, const char *file, bool remigrate = false);
	void Load(const char *file, bool remigrate = false);

	void InitHotkeys();
	void CreateHotkeys();
	void ClearHotkeys();

	bool InitService();

	bool InitBasicConfigDefaults();
	void InitBasicConfigDefaults2();
	bool InitBasicConfig();

	void InitOBSCallbacks();

	void InitPrimitives();

	void OnFirstLoad();

	OBSSceneItem GetSceneItem(QListWidgetItem *item);
	OBSSceneItem GetCurrentSceneItem();

	bool QueryRemoveSource(obs_source_t *source);

	void TimedCheckForUpdates();
	void CheckForUpdates(bool manualUpdate);

	void GetFPSCommon(uint32_t &num, uint32_t &den) const;
	void GetFPSInteger(uint32_t &num, uint32_t &den) const;
	void GetFPSFraction(uint32_t &num, uint32_t &den) const;
	void GetFPSNanoseconds(uint32_t &num, uint32_t &den) const;
	void GetConfigFPS(uint32_t &num, uint32_t &den) const;

	void UpdatePreviewScalingMenu();

	void LoadSceneListOrder(obs_data_array_t *array);
	obs_data_array_t *SaveSceneListOrder();
	void ChangeSceneIndex(bool relative, int idx, int invalidIdx);

	void TempFileOutput(const char *path, int vBitrate, int aBitrate);
	void TempStreamOutput(const char *url, const char *key, int vBitrate, int aBitrate);

	void CloseDialogs();
	void ClearSceneData();
	void ClearProjectors();

	void Nudge(int dist, MoveDir dir);

	OBSProjector *OpenProjector(obs_source_t *source, int monitor, ProjectorType type);

	void GetAudioSourceFilters();
	void GetAudioSourceProperties();
	void VolControlContextMenu();
	void ToggleVolControlLayout();
	void ToggleMixerLayout(bool vertical);

	void LogScenes();
	void SaveProjectNow();

	int GetTopSelectedSourceItem();

	QModelIndexList GetAllSelectedSourceItems();

	obs_hotkey_pair_id streamingHotkeys, recordingHotkeys, pauseHotkeys, replayBufHotkeys, vcamHotkeys,
		togglePreviewHotkeys, contextBarHotkeys;
	obs_hotkey_id forceStreamingStopHotkey, splitFileHotkey, addChapterHotkey;

	void InitDefaultTransitions();
	void InitTransition(obs_source_t *transition);
	obs_source_t *FindTransition(const char *name);
	OBSSource GetCurrentTransition();
	obs_data_array_t *SaveTransitions();
	void LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data);

	obs_source_t *fadeTransition;
	obs_source_t *cutTransition;

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
	void DisableQuickTransitionWidgets();
	void EnableTransitionWidgets(bool enable);
	void CreateDefaultQuickTransitions();

	void PasteShowHideTransition(obs_sceneitem_t *item, bool show, obs_source_t *tr, int duration);
	QMenu *CreatePerSceneTransitionMenu();
	QMenu *CreateVisibilityTransitionMenu(bool visible);

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
	OBSWeakSource lastProgramScene;
	bool editPropertiesMode = false;
	bool sceneDuplicationMode = true;
	bool swapScenesMode = true;
	volatile bool previewProgramMode = false;
	obs_hotkey_pair_id togglePreviewProgramHotkeys = 0;
	obs_hotkey_id transitionHotkey = 0;
	obs_hotkey_id statsHotkey = 0;
	obs_hotkey_id screenshotHotkey = 0;
	obs_hotkey_id previewScreenshotHotkey = 0;
	obs_hotkey_id sourceScreenshotHotkey = 0;
	int quickTransitionIdCounter = 1;
	bool overridingTransition = false;

	int programX = 0, programY = 0;
	int programCX = 0, programCY = 0;
	float programScale = 0.0f;

	int disableOutputsRef = 0;

	inline void OnActivate(bool force = false);
	inline void OnDeactivate();

	void AddDropSource(const char *file, DropType image);
	void AddDropURL(const char *url, QString &name, obs_data_t *settings, const obs_video_info &ovi);
	void ConfirmDropUrl(const QString &url);
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

	bool sysTrayMinimizeToTray();

	void EnumDialogs();

	QList<QDialog *> visDialogs;
	QList<QDialog *> modalDialogs;
	QList<QMessageBox *> visMsgBoxes;

	QList<QPoint> visDlgPositions;

	QByteArray startingDockLayout;

	obs_data_array_t *SaveProjectors();
	void LoadSavedProjectors(obs_data_array_t *savedProjectors);

	void MacBranchesFetched(const QString &branch, bool manualUpdate);
	void ReceivedIntroJson(const QString &text);
	void ShowWhatsNew(const QString &url);

	void UpdatePreviewProgramIndicators();

	QStringList extraDockNames;
	QList<std::shared_ptr<QDockWidget>> extraDocks;

	QStringList extraCustomDockNames;
	QList<QPointer<QDockWidget>> extraCustomDocks;

#ifdef BROWSER_AVAILABLE
	QPointer<QAction> extraBrowserMenuDocksSeparator;

	QList<std::shared_ptr<QDockWidget>> extraBrowserDocks;
	QStringList extraBrowserDockNames;
	QStringList extraBrowserDockTargets;

	void ClearExtraBrowserDocks();
	void LoadExtraBrowserDocks();
	void SaveExtraBrowserDocks();
	void ManageExtraBrowserDocks();
	void AddExtraBrowserDock(const QString &title, const QString &url, const QString &uuid, bool firstCreate);
#endif

	QIcon imageIcon;
	QIcon colorIcon;
	QIcon slideshowIcon;
	QIcon audioInputIcon;
	QIcon audioOutputIcon;
	QIcon desktopCapIcon;
	QIcon windowCapIcon;
	QIcon gameCapIcon;
	QIcon cameraIcon;
	QIcon textIcon;
	QIcon mediaIcon;
	QIcon browserIcon;
	QIcon groupIcon;
	QIcon sceneIcon;
	QIcon defaultIcon;
	QIcon audioProcessOutputIcon;

	QIcon GetImageIcon() const;
	QIcon GetColorIcon() const;
	QIcon GetSlideshowIcon() const;
	QIcon GetAudioInputIcon() const;
	QIcon GetAudioOutputIcon() const;
	QIcon GetDesktopCapIcon() const;
	QIcon GetWindowCapIcon() const;
	QIcon GetGameCapIcon() const;
	QIcon GetCameraIcon() const;
	QIcon GetTextIcon() const;
	QIcon GetMediaIcon() const;
	QIcon GetBrowserIcon() const;
	QIcon GetDefaultIcon() const;
	QIcon GetAudioProcessOutputIcon() const;

	QSlider *tBar;
	bool tBarActive = false;

	OBSSource GetOverrideTransition(OBSSource source);
	int GetOverrideTransitionDuration(OBSSource source);

	void UpdateProjectorHideCursor();
	void UpdateProjectorAlwaysOnTop(bool top);
	void ResetProjectors();

	QPointer<QObject> screenshotData;

	void MoveSceneItem(enum obs_order_movement movement, const QString &action_name);

	bool autoStartBroadcast = true;
	bool autoStopBroadcast = true;
	bool broadcastActive = false;
	bool broadcastReady = false;
	QPointer<QThread> youtubeStreamCheckThread;
#ifdef YOUTUBE_ENABLED
	void YoutubeStreamCheck(const std::string &key);
	void ShowYouTubeAutoStartWarning();
	void YouTubeActionDialogOk(const QString &broadcast_id, const QString &stream_id, const QString &key,
				   bool autostart, bool autostop, bool start_now);
#endif
	void BroadcastButtonClicked();
	void SetBroadcastFlowEnabled(bool enabled);

	void UpdatePreviewSafeAreas();
	bool drawSafeAreas = false;

	void CenterSelectedSceneItems(const CenterType &centerType);
	void ShowMissingFilesDialog(obs_missing_files_t *files);

	QColor selectionColor;
	QColor cropColor;
	QColor hoverColor;

	QColor GetCropColor() const;
	QColor GetHoverColor() const;

	void UpdatePreviewSpacingHelpers();
	bool drawSpacingHelpers = true;

	float GetDevicePixelRatio();
	void SourceToolBarActionsSetEnabled();

	std::string lastScreenshot;
	std::string lastReplay;

	void UpdatePreviewOverflowSettings();
	void UpdatePreviewScrollbars();

	bool streamingStarting = false;

	bool recordingStarted = false;
	bool isRecordingPausable = false;
	bool recordingPaused = false;

	bool restartingVCam = false;

public slots:
	void DeferSaveBegin();
	void DeferSaveEnd();

	void DisplayStreamStartError();

	void SetupBroadcast();

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
	void RecordingStop(int code, QString last_error);
	void RecordingFileChanged(QString lastRecordingPath);

	void ShowReplayBufferPauseWarning();
	void StartReplayBuffer();
	void StopReplayBuffer();

	void ReplayBufferStart();
	void ReplayBufferSave();
	void ReplayBufferSaved();
	void ReplayBufferStopping();
	void ReplayBufferStop(int code);

	void StartVirtualCam();
	void StopVirtualCam();

	void OnVirtualCamStart();
	void OnVirtualCamStop(int code);

	void SaveProjectDeferred();
	void SaveProject();

	void SetTransition(OBSSource transition);
	void OverrideTransition(OBSSource transition);
	void TransitionToScene(OBSScene scene, bool force = false);
	void TransitionToScene(OBSSource scene, bool force = false, bool quickTransition = false, int quickDuration = 0,
			       bool black = false, bool manual = false);
	void SetCurrentScene(OBSSource scene, bool force = false);

	void UpdatePatronJson(const QString &text, const QString &error);

	void ShowContextBar();
	void HideContextBar();
	void PauseRecording();
	void UnpauseRecording();

	void UpdateEditMenu();

private slots:

	void on_actionMainUndo_triggered();
	void on_actionMainRedo_triggered();

	void AddSceneItem(OBSSceneItem item);
	void AddScene(OBSSource source);
	void RemoveScene(OBSSource source);
	void RenameSources(OBSSource source, QString newName, QString prevName);

	void ActivateAudioSource(OBSSource source);
	void DeactivateAudioSource(OBSSource source);

	void DuplicateSelectedScene();
	void RemoveSelectedScene();

	void ToggleAlwaysOnTop();

	void ReorderSources(OBSScene scene);
	void RefreshSources(OBSScene scene);

	void ProcessHotkey(obs_hotkey_id id, bool pressed);

	void AddTransition(const char *id);
	void RenameTransition(OBSSource transition);
	void TransitionClicked();
	void TransitionStopped();
	void TransitionFullyStopped();
	void TriggerQuickTransition(int id);

	void SetDeinterlacingMode();
	void SetDeinterlacingOrder();

	void SetScaleFilter();

	void SetBlendingMethod();
	void SetBlendingMode();

	void IconActivated(QSystemTrayIcon::ActivationReason reason);
	void SetShowing(bool showing);

	void ToggleShowHide();

	void HideAudioControl();
	void UnhideAllAudioControls();
	void ToggleHideMixer();

	void MixerRenameSource();

	void on_vMixerScrollArea_customContextMenuRequested();
	void on_hMixerScrollArea_customContextMenuRequested();

	void on_actionCopySource_triggered();
	void on_actionPasteRef_triggered();
	void on_actionPasteDup_triggered();

	void on_actionCopyFilters_triggered();
	void on_actionPasteFilters_triggered();
	void AudioMixerCopyFilters();
	void AudioMixerPasteFilters();
	void SourcePasteFilters(OBSSource source, OBSSource dstSource);

	void on_previewXScrollBar_valueChanged(int value);
	void on_previewYScrollBar_valueChanged(int value);

	void PreviewScalingModeChanged(int value);

	void ColorChange();

	SourceTreeItem *GetItemWidgetFromSceneItem(obs_sceneitem_t *sceneItem);

	void on_actionShowAbout_triggered();

	void EnablePreview();
	void DisablePreview();

	void EnablePreviewProgram();
	void DisablePreviewProgram();

	void SceneCopyFilters();
	void ScenePasteFilters();

	void CheckDiskSpaceRemaining();
	void OpenSavedProjector(SavedProjectorInfo *info);

	void ResetStatsHotkey();

	void SetImageIcon(const QIcon &icon);
	void SetColorIcon(const QIcon &icon);
	void SetSlideshowIcon(const QIcon &icon);
	void SetAudioInputIcon(const QIcon &icon);
	void SetAudioOutputIcon(const QIcon &icon);
	void SetDesktopCapIcon(const QIcon &icon);
	void SetWindowCapIcon(const QIcon &icon);
	void SetGameCapIcon(const QIcon &icon);
	void SetCameraIcon(const QIcon &icon);
	void SetTextIcon(const QIcon &icon);
	void SetMediaIcon(const QIcon &icon);
	void SetBrowserIcon(const QIcon &icon);
	void SetGroupIcon(const QIcon &icon);
	void SetSceneIcon(const QIcon &icon);
	void SetDefaultIcon(const QIcon &icon);
	void SetAudioProcessOutputIcon(const QIcon &icon);

	void TBarChanged(int value);
	void TBarReleased();

	void LockVolumeControl(bool lock);

	void UpdateVirtualCamConfig(const VCamConfig &config);
	void RestartVirtualCam(const VCamConfig &config);
	void RestartingVirtualCam();

private:
	/* OBS Callbacks */
	static void SceneReordered(void *data, calldata_t *params);
	static void SceneRefreshed(void *data, calldata_t *params);
	static void SceneItemAdded(void *data, calldata_t *params);
	static void SourceCreated(void *data, calldata_t *params);
	static void SourceRemoved(void *data, calldata_t *params);
	static void SourceActivated(void *data, calldata_t *params);
	static void SourceDeactivated(void *data, calldata_t *params);
	static void SourceAudioActivated(void *data, calldata_t *params);
	static void SourceAudioDeactivated(void *data, calldata_t *params);
	static void SourceRenamed(void *data, calldata_t *params);
	static void RenderMain(void *data, uint32_t cx, uint32_t cy);

	void ResizePreview(uint32_t cx, uint32_t cy);

	void AddSource(const char *id);
	QMenu *CreateAddSourcePopupMenu();
	void AddSourcePopupMenu(const QPoint &pos);
	void copyActionsDynamicProperties();

	static void HotkeyTriggered(void *data, obs_hotkey_id id, bool pressed);

	void AutoRemux(QString input, bool no_show = false);

	void UpdateIsRecordingPausable();

	bool IsFFmpegOutputToURL() const;
	bool OutputPathValid();
	void OutputPathInvalidMessage();

	bool LowDiskSpace();
	void DiskSpaceMessage();

	OBSSource prevFTBSource = nullptr;

	float dpi = 1.0;

public:
	OBSSource GetProgramSource();
	OBSScene GetCurrentScene();

	void SysTrayNotify(const QString &text, QSystemTrayIcon::MessageIcon n);

	inline OBSSource GetCurrentSceneSource()
	{
		OBSScene curScene = GetCurrentScene();
		return OBSSource(obs_scene_get_source(curScene));
	}

	obs_service_t *GetService();
	void SetService(obs_service_t *service);

	int GetTransitionDuration();
	int GetTbarPosition();

	inline bool IsPreviewProgramMode() const { return os_atomic_load_bool(&previewProgramMode); }

	inline bool VCamEnabled() const { return vcamEnabled; }

	bool Active() const;

	void ResetUI();
	int ResetVideo();
	bool ResetAudio();

	void ResetOutputs();

	void RefreshVolumeColors();

	void ResetAudioDevice(const char *sourceId, const char *deviceId, const char *deviceDesc, int channel);

	void NewProject();
	void LoadProject();

	inline void GetDisplayRect(int &x, int &y, int &cx, int &cy)
	{
		x = previewX;
		y = previewY;
		cx = previewCX;
		cy = previewCY;
	}

	inline bool SavingDisabled() const { return disableSaving; }

	inline double GetCPUUsage() const { return os_cpu_usage_info_query(cpuUsageInfo); }

	void SaveService();
	bool LoadService();

	inline Auth *GetAuth() { return auth.get(); }

	inline void EnableOutputs(bool enable)
	{
		if (enable) {
			if (--disableOutputsRef < 0)
				disableOutputsRef = 0;
		} else {
			disableOutputsRef++;
		}
	}

	QMenu *AddDeinterlacingMenu(QMenu *menu, obs_source_t *source);
	QMenu *AddScaleFilteringMenu(QMenu *menu, obs_sceneitem_t *item);
	QMenu *AddBlendingMethodMenu(QMenu *menu, obs_sceneitem_t *item);
	QMenu *AddBlendingModeMenu(QMenu *menu, obs_sceneitem_t *item);
	QMenu *AddBackgroundColorMenu(QMenu *menu, QWidgetAction *widgetAction, ColorSelect *select,
				      obs_sceneitem_t *item);
	void CreateSourcePopupMenu(int idx, bool preview);

	void UpdateTitleBar();

	void SystemTrayInit();
	void SystemTray(bool firstStarted);

	void OpenSavedProjectors();

	void CreateInteractionWindow(obs_source_t *source);
	void CreatePropertiesWindow(obs_source_t *source);
	void CreateFiltersWindow(obs_source_t *source);
	void CreateEditTransformWindow(obs_sceneitem_t *item);

	QAction *AddDockWidget(QDockWidget *dock);
	void AddDockWidget(QDockWidget *dock, Qt::DockWidgetArea area, bool extraBrowser = false);
	void RemoveDockWidget(const QString &name);
	bool IsDockObjectNameUsed(const QString &name);
	void AddCustomDockWidget(QDockWidget *dock);

	static OBSBasic *Get();

	const char *GetCurrentOutputPath();

	void DeleteProjector(OBSProjector *projector);

	static QList<QString> GetProjectorMenuMonitorsFormatted();
	template<typename Receiver, typename... Args>
	static void AddProjectorMenuMonitors(QMenu *parent, Receiver *target, void (Receiver::*slot)(Args...))
	{
		auto projectors = GetProjectorMenuMonitorsFormatted();
		for (int i = 0; i < projectors.size(); i++) {
			QString str = projectors[i];
			QAction *action = parent->addAction(str, target, slot);
			action->setProperty("monitor", i);
		}
	}

	QIcon GetSourceIcon(const char *id) const;
	QIcon GetGroupIcon() const;
	QIcon GetSceneIcon() const;

	OBSWeakSource copyFilter;

	void ShowStatusBarMessage(const QString &message);

	static OBSData BackupScene(obs_scene_t *scene, std::vector<obs_source_t *> *sources = nullptr);
	void CreateSceneUndoRedoAction(const QString &action_name, OBSData undo_data, OBSData redo_data);

	static inline OBSData BackupScene(obs_source_t *scene_source, std::vector<obs_source_t *> *sources = nullptr)
	{
		obs_scene_t *scene = obs_scene_from_source(scene_source);
		return BackupScene(scene, sources);
	}

	void CreateFilterPasteUndoRedoAction(const QString &text, obs_source_t *source, obs_data_array_t *undo_array,
					     obs_data_array_t *redo_array);

	void SetDisplayAffinity(QWindow *window);

	QColor GetSelectionColor() const;
	inline bool Closing() { return closing; }

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
	virtual void changeEvent(QEvent *event) override;

private slots:
	void on_actionFullscreenInterface_triggered();

	void on_actionShow_Recordings_triggered();
	void on_actionRemux_triggered();
	void on_action_Settings_triggered();
	void on_actionShowMacPermissions_triggered();
	void on_actionShowMissingFiles_triggered();
	void on_actionAdvAudioProperties_triggered();
	void on_actionMixerToolbarAdvAudio_triggered();
	void on_actionMixerToolbarMenu_triggered();
	void on_actionShowLogs_triggered();
	void on_actionUploadCurrentLog_triggered();
	void on_actionUploadLastLog_triggered();
	void on_actionViewCurrentLog_triggered();
	void on_actionCheckForUpdates_triggered();
	void on_actionRepair_triggered();
	void on_actionShowWhatsNew_triggered();
	void on_actionRestartSafe_triggered();

	void on_actionShowCrashLogs_triggered();
	void on_actionUploadLastCrashLog_triggered();

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
	void on_actionVerticalCenter_triggered();
	void on_actionHorizontalCenter_triggered();
	void on_actionSceneFilters_triggered();

	void on_OBSBasic_customContextMenuRequested(const QPoint &pos);

	void on_scenes_currentItemChanged(QListWidgetItem *current, QListWidgetItem *prev);
	void on_scenes_customContextMenuRequested(const QPoint &pos);
	void GridActionClicked();
	void on_actionSceneListMode_triggered();
	void on_actionSceneGridMode_triggered();
	void on_actionAddScene_triggered();
	void on_actionRemoveScene_triggered();
	void on_actionSceneUp_triggered();
	void on_actionSceneDown_triggered();
	void on_sources_customContextMenuRequested(const QPoint &pos);
	void on_scenes_itemDoubleClicked(QListWidgetItem *item);
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

	void Screenshot(OBSSource source_ = nullptr);
	void ScreenshotSelectedSource();
	void ScreenshotProgram();
	void ScreenshotScene();

	void on_actionHelpPortal_triggered();
	void on_actionWebsite_triggered();
	void on_actionDiscord_triggered();
	void on_actionReleaseNotes_triggered();

	void on_preview_customContextMenuRequested();
	void ProgramViewContextMenuRequested();
	void on_previewDisabledWidget_customContextMenuRequested();

	void on_actionShowSettingsFolder_triggered();
	void on_actionShowProfileFolder_triggered();

	void on_actionAlwaysOnTop_triggered();

	void on_toggleListboxToolbars_toggled(bool visible);
	void on_toggleContextBar_toggled(bool visible);
	void on_toggleStatusBar_toggled(bool visible);
	void on_toggleSourceIcons_toggled(bool visible);

	void on_transitions_currentIndexChanged(int index);
	void on_transitionAdd_clicked();
	void on_transitionRemove_clicked();
	void on_transitionProps_clicked();
	void on_transitionDuration_valueChanged();

	void ShowTransitionProperties();
	void HideTransitionProperties();

	// Source Context Buttons
	void on_sourcePropertiesButton_clicked();
	void on_sourceFiltersButton_clicked();
	void on_sourceInteractButton_clicked();

	void on_autoConfigure_triggered();
	void on_stats_triggered();

	void on_resetUI_triggered();
	void on_resetDocks_triggered(bool force = false);
	void on_lockDocks_toggled(bool lock);
	void on_multiviewProjectorWindowed_triggered();
	void on_sideDocks_toggled(bool side);

	void logUploadFinished(const QString &text, const QString &error);
	void crashUploadFinished(const QString &text, const QString &error);
	void openLogDialog(const QString &text, const bool crash);

	void updateCheckFinished();

	void MoveSceneToTop();
	void MoveSceneToBottom();

	void EditSceneName();
	void EditSceneItemName();

	void SceneNameEdited(QWidget *editor);

	void OpenSceneFilters();
	void OpenFilters(OBSSource source = nullptr);
	void OpenProperties(OBSSource source = nullptr);
	void OpenInteraction(OBSSource source = nullptr);
	void OpenEditTransform(OBSSceneItem item = nullptr);

	void EnablePreviewDisplay(bool enable);
	void TogglePreview();

	void OpenStudioProgramProjector();
	void OpenPreviewProjector();
	void OpenSourceProjector();
	void OpenMultiviewProjector();
	void OpenSceneProjector();

	void OpenStudioProgramWindow();
	void OpenPreviewWindow();
	void OpenSourceWindow();
	void OpenSceneWindow();

	void StackedMixerAreaContextMenuRequested();

	void ResizeOutputSizeOfSource();

	void RepairOldExtraDockName();
	void RepairCustomExtraDockName();

	/* Stream action (start/stop) slot */
	void StreamActionTriggered();

	/* Record action (start/stop) slot */
	void RecordActionTriggered();

	/* Record pause (pause/unpause) slot */
	void RecordPauseToggled();

	/* Replay Buffer action (start/stop) slot */
	void ReplayBufferActionTriggered();

	/* Virtual Cam action (start/stop) slots */
	void VirtualCamActionTriggered();

	void OpenVirtualCamConfig();

	/* Studio Mode toggle slot */
	void TogglePreviewProgramMode();

public slots:
	void on_actionResetTransform_triggered();

	bool StreamingActive();
	bool RecordingActive();
	bool ReplayBufferActive();
	bool VirtualCamActive();

	void ClearContextBar();
	void UpdateContextBar(bool force = false);
	void UpdateContextBarDeferred(bool force = false);
	void UpdateContextBarVisibility();

signals:
	/* Streaming signals */
	void StreamingPreparing();
	void StreamingStarting(bool broadcastAutoStart);
	void StreamingStarted(bool withDelay = false);
	void StreamingStopping();
	void StreamingStopped(bool withDelay = false);

	/* Broadcast Flow signals */
	void BroadcastFlowEnabled(bool enabled);
	void BroadcastStreamReady(bool ready);
	void BroadcastStreamActive();
	void BroadcastStreamStarted(bool autoStop);

	/* Recording signals */
	void RecordingStarted(bool pausable = false);
	void RecordingPaused();
	void RecordingUnpaused();
	void RecordingStopping();
	void RecordingStopped();

	/* Replay Buffer signals */
	void ReplayBufEnabled(bool enabled);
	void ReplayBufStarted();
	void ReplayBufStopping();
	void ReplayBufStopped();

	/* Virtual Camera signals */
	void VirtualCamEnabled();
	void VirtualCamStarted();
	void VirtualCamStopped();

	/* Studio Mode signal */
	void PreviewProgramModeChanged(bool enabled);
	void CanvasResized(uint32_t width, uint32_t height);
	void OutputResized(uint32_t width, uint32_t height);

	/* Preview signals */
	void PreviewXScrollBarMoved(int value);
	void PreviewYScrollBarMoved(int value);

private:
	std::unique_ptr<Ui::OBSBasic> ui;

	QPointer<OBSDock> controlsDock;

public:
	/* `undo_s` needs to be declared after `ui` to prevent an uninitialized
	 * warning for `ui` while initializing `undo_s`. */
	undo_stack undo_s;

	explicit OBSBasic(QWidget *parent = 0);
	virtual ~OBSBasic();

	virtual void OBSInit() override;

	virtual config_t *Config() const override;

	virtual int GetProfilePath(char *path, size_t size, const char *file) const override;

	static void InitBrowserPanelSafeBlock();
#ifdef YOUTUBE_ENABLED
	void NewYouTubeAppDock();
	void DeleteYouTubeAppDock();
	YouTubeAppDock *GetYouTubeAppDock();
#endif
	// MARK: - Generic UI Helper Functions
	OBSPromptResult PromptForName(const OBSPromptRequest &request, const OBSPromptCallback &callback);

	// MARK: - OBS Profile Management
private:
	OBSProfileCache profiles{};

	void SetupNewProfile(const std::string &profileName, bool useWizard = false);
	void SetupDuplicateProfile(const std::string &profileName);
	void SetupRenameProfile(const std::string &profileName);

	const OBSProfile &CreateProfile(const std::string &profileName);
	void RemoveProfile(OBSProfile profile);

	void ChangeProfile();

	void RefreshProfileCache();

	void RefreshProfiles(bool refreshCache = false);

	void ActivateProfile(const OBSProfile &profile, bool reset = false);
	void UpdateProfileEncoders();
	std::vector<std::string> GetRestartRequirements(const ConfigFile &config) const;
	void ResetProfileData();
	void CheckForSimpleModeX264Fallback();

public:
	inline const OBSProfileCache &GetProfileCache() const noexcept { return profiles; };

	const OBSProfile &GetCurrentProfile() const;

	std::optional<OBSProfile> GetProfileByName(const std::string &profileName) const;
	std::optional<OBSProfile> GetProfileByDirectoryName(const std::string &directoryName) const;

private slots:
	void on_actionNewProfile_triggered();
	void on_actionDupProfile_triggered();
	void on_actionRenameProfile_triggered();
	void on_actionRemoveProfile_triggered(bool skipConfirmation = false);
	void on_actionImportProfile_triggered();
	void on_actionExportProfile_triggered();

public slots:
	bool CreateNewProfile(const QString &name);
	bool CreateDuplicateProfile(const QString &name);
	void DeleteProfile(const QString &profileName);

	// MARK: - OBS Scene Collection Management
private:
	OBSSceneCollectionCache collections{};

	void SetupNewSceneCollection(const std::string &collectionName);
	void SetupDuplicateSceneCollection(const std::string &collectionName);
	void SetupRenameSceneCollection(const std::string &collectionName);

	const OBSSceneCollection &CreateSceneCollection(const std::string &collectionName);
	void RemoveSceneCollection(OBSSceneCollection collection);

	bool CreateDuplicateSceneCollection(const QString &name);
	void DeleteSceneCollection(const QString &name);
	void ChangeSceneCollection();

	void RefreshSceneCollectionCache();

	void RefreshSceneCollections(bool refreshCache = false);
	void ActivateSceneCollection(const OBSSceneCollection &collection);

public:
	inline const OBSSceneCollectionCache &GetSceneCollectionCache() const noexcept { return collections; };

	const OBSSceneCollection &GetCurrentSceneCollection() const;

	std::optional<OBSSceneCollection> GetSceneCollectionByName(const std::string &collectionName) const;
	std::optional<OBSSceneCollection> GetSceneCollectionByFileName(const std::string &fileName) const;

private slots:
	void on_actionNewSceneCollection_triggered();
	void on_actionDupSceneCollection_triggered();
	void on_actionRenameSceneCollection_triggered();
	void on_actionRemoveSceneCollection_triggered(bool skipConfirmation = false);
	void on_actionImportSceneCollection_triggered();
	void on_actionExportSceneCollection_triggered();
	void on_actionRemigrateSceneCollection_triggered();

public slots:
	bool CreateNewSceneCollection(const QString &name);
};

extern bool cef_js_avail;

class SceneRenameDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	SceneRenameDelegate(QObject *parent);
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;

protected:
	virtual bool eventFilter(QObject *editor, QEvent *event) override;
};
