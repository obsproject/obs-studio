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

#include "ui_OBSBasic.h"
#include "OBSMainWindow.hpp"

#include <OBSApp.hpp>
#include <oauth/Auth.hpp>
#include <utility/BasicOutputHandler.hpp>
#include <utility/VCamConfig.hpp>
#include <utility/platform.hpp>
#include <utility/undo_stack.hpp>

#include <obs-frontend-internal.hpp>
#include <obs.hpp>

Q_DECLARE_METATYPE(OBSScene);
Q_DECLARE_METATYPE(OBSSceneItem);
Q_DECLARE_METATYPE(OBSSource);

#include <graphics/matrix4.h>
#include <util/platform.h>
#include <util/threading.h>
#include <util/util.hpp>

#include <QSystemTrayIcon>

#include <deque>

extern volatile bool recording_paused;

class ColorSelect;
class OBSAbout;
class OBSBasicAdvAudio;
class OBSBasicFilters;
class OBSBasicInteraction;
class OBSBasicProperties;
class OBSBasicTransform;
class OBSLogViewer;
class OBSMissingFiles;
class OBSProjector;
class VolControl;
#ifdef YOUTUBE_ENABLED
class YouTubeAppDock;
#endif
class QMessageBox;
class QWidgetAction;
struct QuickTransition;

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

enum class ProjectorType;

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

template<typename T> static T GetOBSRef(QListWidgetItem *item)
{
	return item->data(static_cast<int>(QtDataRole::OBSRef)).value<T>();
}

template<typename T> static void SetOBSRef(QListWidgetItem *item, T &&val)
{
	item->setData(static_cast<int>(QtDataRole::OBSRef), QVariant::fromValue(val));
}

static inline bool SourceMixerHidden(obs_source_t *source)
{
	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	bool hidden = obs_data_get_bool(priv_settings, "mixer_hidden");

	return hidden;
}

static inline void SetSourceMixerHidden(obs_source_t *source, bool hidden)
{
	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	obs_data_set_bool(priv_settings, "mixer_hidden", hidden);
}

static inline bool SourceVolumeLocked(obs_source_t *source)
{
	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	bool lock = obs_data_get_bool(priv_settings, "volume_locked");

	return lock;
}

#ifdef _WIN32
static inline void UpdateProcessPriority()
{
	const char *priority = config_get_string(App()->GetAppConfig(), "General", "ProcessPriority");
	if (priority && strcmp(priority, "Normal") != 0)
		SetProcessPriority(priority);
}

static inline void ClearProcessPriority()
{
	const char *priority = config_get_string(App()->GetAppConfig(), "General", "ProcessPriority");
	if (priority && strcmp(priority, "Normal") != 0)
		SetProcessPriority("Normal");
}
#else
#define UpdateProcessPriority() \
	do {                    \
	} while (false)
#define ClearProcessPriority() \
	do {                   \
	} while (false)
#endif

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
	friend class OBSBasicSettings;
	friend class Auth;
	friend class AutoConfig;
	friend class AutoConfigStreamPage;
	friend class ExtraBrowsersModel;
	friend class OBSYoutubeActions;
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
	/* -------------------------------------
	 * MARK: - General
	 * -------------------------------------
	 */
private:
	obs_frontend_callbacks *api = nullptr;
	std::vector<OBSSignal> signalHandlers;

	bool loaded = false;
	bool closing = false;

	// TODO: Remove, orphaned variable
	bool copyVisible = true;
	// TODO: Unused thread pointer, remove.
	QScopedPointer<QThread> devicePropertiesThread;

	QScopedPointer<QThread> logUploadThread;

	ConfigFile activeConfiguration;

	QScopedPointer<QThread> patronJsonThread;
	std::string patronJson;

	std::unique_ptr<Ui::OBSBasic> ui;

	void OnEvent(enum obs_frontend_event event);

	bool InitBasicConfigDefaults();
	void InitBasicConfigDefaults2();
	bool InitBasicConfig();

	void InitOBSCallbacks();

	void OnFirstLoad();

	void GetFPSCommon(uint32_t &num, uint32_t &den) const;
	void GetFPSInteger(uint32_t &num, uint32_t &den) const;
	void GetFPSFraction(uint32_t &num, uint32_t &den) const;
	void GetFPSNanoseconds(uint32_t &num, uint32_t &den) const;
	void GetConfigFPS(uint32_t &num, uint32_t &den) const;

	OBSPromptResult PromptForName(const OBSPromptRequest &request, const OBSPromptCallback &callback);

	// TODO: Remove, orphaned instance method
	void NewProject();
	// TODO: Remove, orphaned instance method
	void LoadProject();

public slots:
	void UpdatePatronJson(const QString &text, const QString &error);
	void UpdateEditMenu();

public:
	/* `undo_s` needs to be declared after `ui` to prevent an uninitialized
	 * warning for `ui` while initializing `undo_s`. */
	undo_stack undo_s;

	explicit OBSBasic(QWidget *parent = 0);
	virtual ~OBSBasic();

	virtual void OBSInit() override;

	virtual config_t *Config() const override;

	int ResetVideo();
	bool ResetAudio();

	void UpdateTitleBar();

	static OBSBasic *Get();

	void SetDisplayAffinity(QWindow *window);

	inline bool Closing() { return closing; }

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
	virtual void changeEvent(QEvent *event) override;

	/* -------------------------------------
	 * MARK: - OAuth
	 * -------------------------------------
	 */
private:
	std::shared_ptr<Auth> auth;

public:
	inline Auth *GetAuth() { return auth.get(); }

	/* -------------------------------------
	 * MARK: - OBSBasic_Browser
	 * -------------------------------------
	 */
private:
	QPointer<QWidget> extraBrowsers;

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

public:
	static void InitBrowserPanelSafeBlock();

	/* -------------------------------------
	 * MARK: - OBSBasic_Clipboard
	 * -------------------------------------
	 */
private:
	std::deque<SourceCopyInfo> clipboard;
	OBSWeakSourceAutoRelease copyFiltersSource;
	obs_transform_info copiedTransformInfo;
	obs_sceneitem_crop copiedCropInfo;
	bool hasCopiedTransform = false;
	int copySourceTransitionDuration;
	OBSWeakSourceAutoRelease copySourceTransition;

private slots:
	void on_actionCopySource_triggered();
	void on_actionPasteRef_triggered();
	void on_actionPasteDup_triggered();

	void on_actionCopyFilters_triggered();
	void on_actionPasteFilters_triggered();
	void AudioMixerCopyFilters();
	void AudioMixerPasteFilters();
	void SourcePasteFilters(OBSSource source, OBSSource dstSource);

	void SceneCopyFilters();
	void ScenePasteFilters();

	void on_actionCopyTransform_triggered();
	void on_actionPasteTransform_triggered();

public:
	OBSWeakSource copyFilter;
	void CreateFilterPasteUndoRedoAction(const QString &text, obs_source_t *source, obs_data_array_t *undo_array,
					     obs_data_array_t *redo_array);

	/* -------------------------------------
	 * MARK: - OBSBasic_ContextToolbar
	 * -------------------------------------
	 */
private:
	ContextBarSize contextBarSize = ContextBarSize_Normal;

	void SourceToolBarActionsSetEnabled();

	void copyActionsDynamicProperties();

private slots:
	void on_toggleContextBar_toggled(bool visible);

public slots:
	void ShowContextBar();
	void HideContextBar();

	void ClearContextBar();
	void UpdateContextBar(bool force = false);
	void UpdateContextBarDeferred(bool force = false);
	void UpdateContextBarVisibility();

	/* -------------------------------------
	 * MARK: - OBSBasic_Docks
	 * -------------------------------------
	 */
private:
	QList<QPointer<QDockWidget>> oldExtraDocks;
	QStringList oldExtraDockNames;
	QPointer<QDockWidget> statsDock;
	QByteArray startingDockLayout;
	QStringList extraDockNames;
	QList<std::shared_ptr<QDockWidget>> extraDocks;

	QStringList extraCustomDockNames;
	QList<QPointer<QDockWidget>> extraCustomDocks;

	QPointer<OBSDock> controlsDock;

public:
	QAction *AddDockWidget(QDockWidget *dock);
	void AddDockWidget(QDockWidget *dock, Qt::DockWidgetArea area, bool extraBrowser = false);
	void RemoveDockWidget(const QString &name);
	bool IsDockObjectNameUsed(const QString &name);
	void AddCustomDockWidget(QDockWidget *dock);

private slots:
	void on_resetDocks_triggered(bool force = false);
	void on_lockDocks_toggled(bool lock);
	void on_sideDocks_toggled(bool side);

	void RepairOldExtraDockName();
	void RepairCustomExtraDockName();

	/* -------------------------------------
	 * MARK: - OBSBasic_Dropfiles
	 * -------------------------------------
	 */
private:
	void AddDropSource(const char *file, DropType image);
	void AddDropURL(const char *url, QString &name, obs_data_t *settings, const obs_video_info &ovi);
	void ConfirmDropUrl(const QString &url);
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

	/* -------------------------------------
	 * MARK: - OBSBasic_Hotkeys
	 * -------------------------------------
	 */
private:
	QPointer<QObject> shortcutFilter;
	obs_hotkey_id statsHotkey = 0;
	obs_hotkey_id screenshotHotkey = 0;
	obs_hotkey_id sourceScreenshotHotkey = 0;

	obs_hotkey_pair_id streamingHotkeys, recordingHotkeys, pauseHotkeys, replayBufHotkeys, vcamHotkeys,
		togglePreviewHotkeys, contextBarHotkeys;
	obs_hotkey_id forceStreamingStopHotkey, splitFileHotkey, addChapterHotkey;

	void InitHotkeys();
	void CreateHotkeys();
	void ClearHotkeys();

	static void HotkeyTriggered(void *data, obs_hotkey_id id, bool pressed);

private slots:
	void ProcessHotkey(obs_hotkey_id id, bool pressed);
	void ResetStatsHotkey();

	/* -------------------------------------
	 * MARK: - OBSBasic_Icons
	 * -------------------------------------
	 */
private:
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

private slots:
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

public:
	QIcon GetSourceIcon(const char *id) const;
	QIcon GetGroupIcon() const;
	QIcon GetSceneIcon() const;

	/* -------------------------------------
	 * MARK: - OBSBasic_MainControls
	 * -------------------------------------
	 */
private:
	QPointer<OBSBasicInteraction> interaction;
	QPointer<OBSBasicProperties> properties;
	QPointer<OBSBasicTransform> transformWindow;
	QPointer<OBSBasicAdvAudio> advAudioWindow;
	QPointer<OBSBasicFilters> filters;
	QPointer<OBSAbout> about;
	QPointer<OBSLogViewer> logView;
	QPointer<QWidget> stats;
	QPointer<QWidget> remux;
	QPointer<QWidget> importer;
	QPointer<QAction> showHide;
	QPointer<QAction> exit;

	QPointer<QMenu> scaleFilteringMenu;
	QPointer<QMenu> blendingMethodMenu;
	QPointer<QMenu> blendingModeMenu;
	QPointer<QMenu> colorMenu;
	QPointer<QMenu> deinterlaceMenu;

	QPointer<QWidgetAction> colorWidgetAction;
	QPointer<ColorSelect> colorSelect;

	QList<QDialog *> visDialogs;
	QList<QDialog *> modalDialogs;
	QList<QMessageBox *> visMsgBoxes;

	QList<QPoint> visDlgPositions;

	void UploadLog(const char *subdir, const char *file, const bool crash);
	void CloseDialogs();
	void EnumDialogs();

private slots:
	void on_actionMainUndo_triggered();
	void on_actionMainRedo_triggered();
	void ToggleAlwaysOnTop();

	void SetShowing(bool showing);

	void ToggleShowHide();

	void on_actionShowAbout_triggered();

	void on_actionFullscreenInterface_triggered();
	void on_actionRemux_triggered();
	void on_action_Settings_triggered();
	void on_actionShowMacPermissions_triggered();
	void on_actionAdvAudioProperties_triggered();
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

	void on_OBSBasic_customContextMenuRequested(const QPoint &pos);

	void on_actionHelpPortal_triggered();
	void on_actionWebsite_triggered();
	void on_actionDiscord_triggered();
	void on_actionReleaseNotes_triggered();

	void on_actionShowSettingsFolder_triggered();
	void on_actionShowProfileFolder_triggered();

	void on_actionAlwaysOnTop_triggered();

	void on_toggleListboxToolbars_toggled(bool visible);
	void on_toggleStatusBar_toggled(bool visible);

	void on_autoConfigure_triggered();
	void on_stats_triggered();

	void on_resetUI_triggered();

	void logUploadFinished(const QString &text, const QString &error);
	void crashUploadFinished(const QString &text, const QString &error);
	void openLogDialog(const QString &text, const bool crash);

	void updateCheckFinished();

public:
	void ResetUI();

	void CreateInteractionWindow(obs_source_t *source);
	void CreateFiltersWindow(obs_source_t *source);
	void CreateEditTransformWindow(obs_sceneitem_t *item);
	void CreatePropertiesWindow(obs_source_t *source);

	/* -------------------------------------
	 * MARK: - OBSBasic_OutputHandler
	 * -------------------------------------
	 */
private:
	std::unique_ptr<BasicOutputHandler> outputHandler;
	std::optional<std::pair<uint32_t, uint32_t>> lastOutputResolution;

	int disableOutputsRef = 0;

	inline void OnActivate(bool force = false)
	{
		if (ui->profileMenu->isEnabled() || force) {
			ui->profileMenu->setEnabled(false);
			ui->autoConfigure->setEnabled(false);
			App()->IncrementSleepInhibition();
			UpdateProcessPriority();

			struct obs_video_info ovi;
			obs_get_video_info(&ovi);
			lastOutputResolution = {ovi.base_width, ovi.base_height};

			TaskbarOverlaySetStatus(TaskbarOverlayStatusActive);
			if (trayIcon && trayIcon->isVisible()) {
#ifdef __APPLE__
				QIcon trayMask = QIcon(":/res/images/tray_active_macos.svg");
				trayMask.setIsMask(true);
				trayIcon->setIcon(QIcon::fromTheme("obs-tray", trayMask));
#else
				trayIcon->setIcon(
					QIcon::fromTheme("obs-tray-active", QIcon(":/res/images/tray_active.png")));
#endif
			}
		}
	}

	inline void OnDeactivate()
	{
		if (!outputHandler->Active() && !ui->profileMenu->isEnabled()) {
			ui->profileMenu->setEnabled(true);
			ui->autoConfigure->setEnabled(true);
			App()->DecrementSleepInhibition();
			ClearProcessPriority();

			TaskbarOverlaySetStatus(TaskbarOverlayStatusInactive);
			if (trayIcon && trayIcon->isVisible()) {
#ifdef __APPLE__
				QIcon trayIconFile = QIcon(":/res/images/obs_macos.svg");
				trayIconFile.setIsMask(true);
#else
				QIcon trayIconFile = QIcon(":/res/images/obs.png");
#endif
				trayIcon->setIcon(QIcon::fromTheme("obs-tray", trayIconFile));
			}
		} else if (outputHandler->Active() && trayIcon && trayIcon->isVisible()) {
			if (os_atomic_load_bool(&recording_paused)) {
#ifdef __APPLE__
				QIcon trayIconFile = QIcon(":/res/images/obs_paused_macos.svg");
				trayIconFile.setIsMask(true);
#else
				QIcon trayIconFile = QIcon(":/res/images/obs_paused.png");
#endif
				trayIcon->setIcon(QIcon::fromTheme("obs-tray-paused", trayIconFile));
				TaskbarOverlaySetStatus(TaskbarOverlayStatusPaused);
			} else {
#ifdef __APPLE__
				QIcon trayIconFile = QIcon(":/res/images/tray_active_macos.svg");
				trayIconFile.setIsMask(true);
#else
				QIcon trayIconFile = QIcon(":/res/images/tray_active.png");
#endif
				trayIcon->setIcon(QIcon::fromTheme("obs-tray-active", trayIconFile));
				TaskbarOverlaySetStatus(TaskbarOverlayStatusActive);
			}
		}
	}

	bool IsFFmpegOutputToURL() const;
	bool OutputPathValid();
	void OutputPathInvalidMessage();

	// TODO: Unimplemented, remove.
	void SetupEncoders();
	// TODO: Unimplemented, remove.
	void TempFileOutput(const char *path, int vBitrate, int aBitrate);
	// TODO: Unimplemented, remove.
	void TempStreamOutput(const char *url, const char *key, int vBitrate, int aBitrate);

public:
	bool Active() const;
	void ResetOutputs();

	inline void EnableOutputs(bool enable)
	{
		if (enable) {
			if (--disableOutputsRef < 0)
				disableOutputsRef = 0;
		} else {
			disableOutputsRef++;
		}
	}

	const char *GetCurrentOutputPath();

private slots:
	void ResizeOutputSizeOfSource();

	/* -------------------------------------
	 * MARK: - OBSBasic_Preview
	 * -------------------------------------
	 */
private:
	bool previewEnabled = true;
	QPointer<QTimer> nudge_timer;
	bool recent_nudge = false;

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
	bool drawSafeAreas = false;

	QColor selectionColor;
	QColor cropColor;
	QColor hoverColor;

	bool drawSpacingHelpers = true;

	float dpi = 1.0;

	void DrawBackdrop(float cx, float cy);
	void InitPrimitives();
	void UpdatePreviewScalingMenu();

	void Nudge(int dist, MoveDir dir);

	void UpdateProjectorHideCursor();
	void UpdateProjectorAlwaysOnTop(bool top);
	void ResetProjectors();

	void UpdatePreviewSafeAreas();

	QColor GetCropColor() const;
	QColor GetHoverColor() const;

	void UpdatePreviewSpacingHelpers();

	float GetDevicePixelRatio();

	void UpdatePreviewOverflowSettings();
	void UpdatePreviewScrollbars();

	/* OBS Callbacks */
	static void RenderMain(void *data, uint32_t cx, uint32_t cy);

	void ResizePreview(uint32_t cx, uint32_t cy);

private slots:
	void on_previewXScrollBar_valueChanged(int value);
	void on_previewYScrollBar_valueChanged(int value);

	void PreviewScalingModeChanged(int value);

	void ColorChange();

	void EnablePreview();
	void DisablePreview();

	void on_actionLockPreview_triggered();

	void on_scalingMenu_aboutToShow();
	void on_actionScaleWindow_triggered();
	void on_actionScaleCanvas_triggered();
	void on_actionScaleOutput_triggered();

	void on_preview_customContextMenuRequested();
	void on_previewDisabledWidget_customContextMenuRequested();

	void EnablePreviewDisplay(bool enable);
	void TogglePreview();

public:
	inline void GetDisplayRect(int &x, int &y, int &cx, int &cy)
	{
		x = previewX;
		y = previewY;
		cx = previewCX;
		cy = previewCY;
	}

	QColor GetSelectionColor() const;

signals:
	void CanvasResized(uint32_t width, uint32_t height);
	void OutputResized(uint32_t width, uint32_t height);

	/* Preview signals */
	void PreviewXScrollBarMoved(int value);
	void PreviewYScrollBarMoved(int value);

	/* -------------------------------------
	 * MARK: - OBSBasic_Profiles
	 * -------------------------------------
	 */
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

	/* -------------------------------------
	 * MARK: - OBSBasic_Projectors
	 * -------------------------------------
	 */
private:
	std::vector<SavedProjectorInfo *> savedProjectorsArray;
	std::vector<OBSProjector *> projectors;
	QPointer<QMenu> previewProjector;
	QPointer<QMenu> previewProjectorSource;
	QPointer<QMenu> previewProjectorMain;

	void UpdateMultiviewProjectorMenu();
	void ClearProjectors();
	OBSProjector *OpenProjector(obs_source_t *source, int monitor, ProjectorType type);

	obs_data_array_t *SaveProjectors();
	void LoadSavedProjectors(obs_data_array_t *savedProjectors);

private slots:
	void OpenSavedProjector(SavedProjectorInfo *info);
	void on_multiviewProjectorWindowed_triggered();

	void OpenPreviewProjector();
	void OpenSourceProjector();
	void OpenMultiviewProjector();
	void OpenSceneProjector();

	void OpenPreviewWindow();
	void OpenSourceWindow();
	void OpenSceneWindow();

public:
	void OpenSavedProjectors();
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

	/* -------------------------------------
	 * MARK: - OBSBasic_Recording
	 * -------------------------------------
	 */
private:
	QPointer<QTimer> diskFullTimer;
	bool recordingStopping = false;
	bool recordingStarted = false;
	bool isRecordingPausable = false;
	bool recordingPaused = false;

	void AutoRemux(QString input, bool no_show = false);
	void UpdateIsRecordingPausable();

	bool LowDiskSpace();
	void DiskSpaceMessage();

private slots:
	void on_actionShow_Recordings_triggered();

	/* Record action (start/stop) slot */
	void RecordActionTriggered();

	/* Record pause (pause/unpause) slot */
	void RecordPauseToggled();

public slots:
	void StartRecording();
	void StopRecording();

	void RecordingStart();
	void RecordStopping();
	void RecordingStop(int code, QString last_error);
	void RecordingFileChanged(QString lastRecordingPath);

	void PauseRecording();
	void UnpauseRecording();

	void CheckDiskSpaceRemaining();

	bool RecordingActive();

signals:
	/* Recording signals */
	void RecordingStarted(bool pausable = false);
	void RecordingPaused();
	void RecordingUnpaused();
	void RecordingStopping();
	void RecordingStopped();

	/* -------------------------------------
	 * MARK: - OBSBasic_ReplayBuffer
	 * -------------------------------------
	 */
private:
	bool replayBufferStopping = false;
	std::string lastReplay;

public slots:
	void ShowReplayBufferPauseWarning();
	void StartReplayBuffer();
	void StopReplayBuffer();

	void ReplayBufferStart();
	void ReplayBufferSave();
	void ReplayBufferSaved();
	void ReplayBufferStopping();
	void ReplayBufferStop(int code);

	bool ReplayBufferActive();

private slots:
	/* Replay Buffer action (start/stop) slot */
	void ReplayBufferActionTriggered();

signals:
	/* Replay Buffer signals */
	void ReplayBufEnabled(bool enabled);
	void ReplayBufStarted();
	void ReplayBufStopping();
	void ReplayBufStopped();

	/* -------------------------------------
	 * MARK: - OBSBasic_SceneCollections
	 * -------------------------------------
	 */
private:
	OBSDataAutoRelease collectionModuleData;
	long disableSaving = 1;
	bool projectChanged = false;
	bool clearingFailed = false;

	QPointer<OBSMissingFiles> missDialog;
	std::optional<std::pair<uint32_t, uint32_t>> migrationBaseResolution;
	bool usingAbsoluteCoordinates = false;

	OBSSceneCollectionCache collections{};

	void DisableRelativeCoordinates(bool disable);
	void CreateDefaultScene(bool firstStart);
	void Save(const char *file);
	void LoadData(obs_data_t *data, const char *file, bool remigrate = false);
	void Load(const char *file, bool remigrate = false);

	void ClearSceneData();
	void LogScenes();
	void SaveProjectNow();
	void ShowMissingFilesDialog(obs_missing_files_t *files);

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

public slots:
	void DeferSaveBegin();
	void DeferSaveEnd();

	void SaveProjectDeferred();
	void SaveProject();

	bool CreateNewSceneCollection(const QString &name);

private slots:
	void on_actionShowMissingFiles_triggered();

	void on_actionNewSceneCollection_triggered();
	void on_actionDupSceneCollection_triggered();
	void on_actionRenameSceneCollection_triggered();
	void on_actionRemoveSceneCollection_triggered(bool skipConfirmation = false);
	void on_actionImportSceneCollection_triggered();
	void on_actionExportSceneCollection_triggered();
	void on_actionRemigrateSceneCollection_triggered();

public:
	inline bool SavingDisabled() const { return disableSaving; }

	inline const OBSSceneCollectionCache &GetSceneCollectionCache() const noexcept { return collections; };

	const OBSSceneCollection &GetCurrentSceneCollection() const;

	std::optional<OBSSceneCollection> GetSceneCollectionByName(const std::string &collectionName) const;
	std::optional<OBSSceneCollection> GetSceneCollectionByFileName(const std::string &fileName) const;

	/* -------------------------------------
	 * MARK: - OBSBasic_SceneItems
	 * -------------------------------------
	 */
private:
	QPointer<QMenu> sourceProjector;
	QPointer<QAction> renameSource;

	void CreateFirstRunSources();

	OBSSceneItem GetSceneItem(QListWidgetItem *item);
	OBSSceneItem GetCurrentSceneItem();

	bool QueryRemoveSource(obs_source_t *source);
	int GetTopSelectedSourceItem();

	void GetAudioSourceFilters();
	void GetAudioSourceProperties();

	QModelIndexList GetAllSelectedSourceItems();

	// TODO: Move back to transitions
	QMenu *CreateVisibilityTransitionMenu(bool visible);
	void CenterSelectedSceneItems(const CenterType &centerType);

	/* OBS Callbacks */
	static void SourceCreated(void *data, calldata_t *params);
	static void SourceRemoved(void *data, calldata_t *params);
	static void SourceActivated(void *data, calldata_t *params);
	static void SourceDeactivated(void *data, calldata_t *params);
	static void SourceAudioActivated(void *data, calldata_t *params);
	static void SourceAudioDeactivated(void *data, calldata_t *params);
	static void SourceRenamed(void *data, calldata_t *params);

	void AddSource(const char *id);
	QMenu *CreateAddSourcePopupMenu();
	void AddSourcePopupMenu(const QPoint &pos);

private slots:
	void RenameSources(OBSSource source, QString newName, QString prevName);

	void ActivateAudioSource(OBSSource source);
	void DeactivateAudioSource(OBSSource source);

	void ReorderSources(OBSScene scene);
	void RefreshSources(OBSScene scene);

	void SetDeinterlacingMode();
	void SetDeinterlacingOrder();

	void SetScaleFilter();

	void SetBlendingMethod();
	void SetBlendingMode();

	void MixerRenameSource();

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

	void on_actionEditTransform_triggered();

	void on_sources_customContextMenuRequested(const QPoint &pos);

	// Source Context Buttons
	void on_sourcePropertiesButton_clicked();
	void on_sourceFiltersButton_clicked();
	void on_sourceInteractButton_clicked();

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

	void on_toggleSourceIcons_toggled(bool visible);

	void OpenFilters(OBSSource source = nullptr);
	void OpenProperties(OBSSource source = nullptr);
	void OpenInteraction(OBSSource source = nullptr);
	void OpenEditTransform(OBSSceneItem item = nullptr);

public:
	void ResetAudioDevice(const char *sourceId, const char *deviceId, const char *deviceDesc, int channel);

	QMenu *AddDeinterlacingMenu(QMenu *menu, obs_source_t *source);
	QMenu *AddScaleFilteringMenu(QMenu *menu, obs_sceneitem_t *item);
	QMenu *AddBlendingMethodMenu(QMenu *menu, obs_sceneitem_t *item);
	QMenu *AddBlendingModeMenu(QMenu *menu, obs_sceneitem_t *item);
	QMenu *AddBackgroundColorMenu(QMenu *menu, QWidgetAction *widgetAction, ColorSelect *select,
				      obs_sceneitem_t *item);
	void CreateSourcePopupMenu(int idx, bool preview);

	/* -------------------------------------
	 * MARK: - OBSBasic_Scenes
	 * -------------------------------------
	 */
private:
	QPointer<QMenu> sceneProjectorMenu;
	QPointer<QAction> renameScene;
	std::atomic<obs_scene_t *> currentScene = nullptr;
	OBSWeakSource lastScene;
	OBSWeakSource swapScene;

	void LoadSceneListOrder(obs_data_array_t *array);
	obs_data_array_t *SaveSceneListOrder();
	void ChangeSceneIndex(bool relative, int idx, int invalidIdx);

	void MoveSceneItem(enum obs_order_movement movement, const QString &action_name);

	/* OBS Callbacks */
	static void SceneReordered(void *data, calldata_t *params);
	static void SceneRefreshed(void *data, calldata_t *params);
	static void SceneItemAdded(void *data, calldata_t *params);

public slots:
	void on_actionResetTransform_triggered();

private slots:
	void AddSceneItem(OBSSceneItem item);
	void AddScene(OBSSource source);
	void RemoveScene(OBSSource source);

	void DuplicateSelectedScene();
	void RemoveSelectedScene();

	SourceTreeItem *GetItemWidgetFromSceneItem(obs_sceneitem_t *sceneItem);

	void on_actionSceneFilters_triggered();

	void on_scenes_currentItemChanged(QListWidgetItem *current, QListWidgetItem *prev);
	void on_scenes_customContextMenuRequested(const QPoint &pos);

	void GridActionClicked();
	void on_actionSceneListMode_triggered();
	void on_actionSceneGridMode_triggered();
	void on_actionAddScene_triggered();
	void on_actionRemoveScene_triggered();
	void on_actionSceneUp_triggered();
	void on_actionSceneDown_triggered();
	void on_scenes_itemDoubleClicked(QListWidgetItem *item);

	void MoveSceneToTop();
	void MoveSceneToBottom();

	void EditSceneName();
	void EditSceneItemName();

	void SceneNameEdited(QWidget *editor);
	void OpenSceneFilters();

public:
	static OBSData BackupScene(obs_scene_t *scene, std::vector<obs_source_t *> *sources = nullptr);
	static inline OBSData BackupScene(obs_source_t *scene_source, std::vector<obs_source_t *> *sources = nullptr)
	{
		obs_scene_t *scene = obs_scene_from_source(scene_source);
		return BackupScene(scene, sources);
	}

	OBSScene GetCurrentScene();

	inline OBSSource GetCurrentSceneSource()
	{
		OBSScene curScene = GetCurrentScene();
		return OBSSource(obs_scene_get_source(curScene));
	}

	void CreateSceneUndoRedoAction(const QString &action_name, OBSData undo_data, OBSData redo_data);

	/* -------------------------------------
	 * MARK: - OBSBasic_Screenshots
	 * -------------------------------------
	 */
private:
	QPointer<QObject> screenshotData;
	std::string lastScreenshot;

private slots:
	void Screenshot(OBSSource source_ = nullptr);
	void ScreenshotSelectedSource();
	void ScreenshotProgram();
	void ScreenshotScene();

	/* -------------------------------------
	 * MARK: - OBSBasic_Service
	 * -------------------------------------
	 */
private:
	OBSService service;

	bool InitService();

public:
	obs_service_t *GetService();
	void SetService(obs_service_t *service);

	void SaveService();
	bool LoadService();

	/* -------------------------------------
	 * MARK: - OBSBasic_StatusBar
	 * -------------------------------------
	 */
private:
	QPointer<QTimer> cpuUsageTimer;
	os_cpu_usage_info_t *cpuUsageInfo = nullptr;

public:
	inline double GetCPUUsage() const { return os_cpu_usage_info_query(cpuUsageInfo); }
	void ShowStatusBarMessage(const QString &message);

	/* -------------------------------------
	 * MARK: - OBSBasic_Streaming
	 * -------------------------------------
	 */
private:
	std::shared_future<void> setupStreamingGuard;
	bool streamingStopping = false;
	bool streamingStarting = false;

public slots:
	void DisplayStreamStartError();
	void StartStreaming();
	void StopStreaming();
	void ForceStopStreaming();

	void StreamDelayStarting(int sec);
	void StreamDelayStopping(int sec);

	void StreamingStart();
	void StreamStopping();
	void StreamingStop(int errorcode, QString last_error);

	bool StreamingActive();

private slots:
	/* Stream action (start/stop) slot */
	void StreamActionTriggered();

signals:
	/* Streaming signals */
	void StreamingPreparing();
	void StreamingStarting(bool broadcastAutoStart);
	void StreamingStarted(bool withDelay = false);
	void StreamingStopping();
	void StreamingStopped(bool withDelay = false);

	/* -------------------------------------
	 * MARK: - OBSBasic_StudioMode
	 * -------------------------------------
	 */
private:
	QPointer<QMenu> studioProgramProjector;
	QPointer<QWidget> programWidget;
	QPointer<QVBoxLayout> programLayout;
	QPointer<QLabel> programLabel;
	QPointer<QWidget> programOptions;
	QPointer<OBSQTDisplay> program;
	OBSWeakSource lastProgramScene;

	bool editPropertiesMode = false;
	bool sceneDuplicationMode = true;

	OBSWeakSource programScene;
	volatile bool previewProgramMode = false;

	obs_hotkey_pair_id togglePreviewProgramHotkeys = 0;

	int programX = 0, programY = 0;
	int programCX = 0, programCY = 0;
	float programScale = 0.0f;

	void CreateProgramDisplay();
	void CreateProgramOptions();
	void SetPreviewProgramMode(bool enabled);
	void ResizeProgram(uint32_t cx, uint32_t cy);
	static void RenderProgram(void *data, uint32_t cx, uint32_t cy);

	void UpdatePreviewProgramIndicators();

private slots:
	void EnablePreviewProgram();
	void DisablePreviewProgram();

	void ProgramViewContextMenuRequested();

	void OpenStudioProgramProjector();
	void OpenStudioProgramWindow();

	/* Studio Mode toggle slot */
	void TogglePreviewProgramMode();

public:
	OBSSource GetProgramSource();

	inline bool IsPreviewProgramMode() const { return os_atomic_load_bool(&previewProgramMode); }

signals:
	/* Studio Mode signal */
	void PreviewProgramModeChanged(bool enabled);

	/* -------------------------------------
	 * MARK: - OBSBasic_SysTray
	 * -------------------------------------
	 */
private:
	QScopedPointer<QSystemTrayIcon> trayIcon;
	QPointer<QAction> sysTrayStream;
	QPointer<QAction> sysTrayRecord;
	QPointer<QAction> sysTrayReplayBuffer;
	QPointer<QAction> sysTrayVirtualCam;
	QPointer<QMenu> trayMenu;

	bool sysTrayMinimizeToTray();

private slots:
	void IconActivated(QSystemTrayIcon::ActivationReason reason);

public:
	void SysTrayNotify(const QString &text, QSystemTrayIcon::MessageIcon n);

	void SystemTrayInit();
	void SystemTray(bool firstStarted);

	/* -------------------------------------
	 * MARK: - OBSBasic_Transitions
	 * -------------------------------------
	 */
private:
	std::vector<OBSDataAutoRelease> safeModeTransitions;
	QPointer<QPushButton> transitionButton;
	QPointer<QMenu> perSceneTransitionMenu;
	obs_source_t *fadeTransition;
	obs_source_t *cutTransition;
	std::vector<QuickTransition> quickTransitions;
	bool swapScenesMode = true;

	obs_hotkey_id transitionHotkey = 0;

	int quickTransitionIdCounter = 1;
	bool overridingTransition = false;

	QSlider *tBar;
	bool tBarActive = false;

	OBSSource prevFTBSource = nullptr;

	void InitDefaultTransitions();
	void InitTransition(obs_source_t *transition);
	obs_source_t *FindTransition(const char *name);
	OBSSource GetCurrentTransition();
	obs_data_array_t *SaveTransitions();
	void LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data);

	void AddQuickTransitionId(int id);
	void AddQuickTransition();
	void AddQuickTransitionHotkey(QuickTransition *qt);
	void RemoveQuickTransitionHotkey(QuickTransition *qt);
	void LoadQuickTransitions(obs_data_array_t *array);
	obs_data_array_t *SaveQuickTransitions();
	void ClearQuickTransitionWidgets();
	void RefreshQuickTransitions();
	// TODO: Remove orphaned method.
	void DisableQuickTransitionWidgets();
	void EnableTransitionWidgets(bool enable);
	void CreateDefaultQuickTransitions();

	QuickTransition *GetQuickTransition(int id);
	int GetQuickTransitionIdx(int id);
	QMenu *CreateTransitionMenu(QWidget *parent, QuickTransition *qt);
	void ClearQuickTransitions();
	void QuickTransitionClicked();
	void QuickTransitionChange();
	void QuickTransitionChangeDuration(int value);
	void QuickTransitionRemoveClicked();

	OBSSource GetOverrideTransition(OBSSource source);
	int GetOverrideTransitionDuration(OBSSource source);

	QMenu *CreatePerSceneTransitionMenu();

	void SetCurrentScene(obs_scene_t *scene, bool force = false);

	void PasteShowHideTransition(obs_sceneitem_t *item, bool show, obs_source_t *tr, int duration);

public slots:
	void SetCurrentScene(OBSSource scene, bool force = false);

	void SetTransition(OBSSource transition);
	void OverrideTransition(OBSSource transition);
	void TransitionToScene(OBSScene scene, bool force = false);
	void TransitionToScene(OBSSource scene, bool force = false, bool quickTransition = false, int quickDuration = 0,
			       bool black = false, bool manual = false);

private slots:
	void AddTransition(const char *id);
	void RenameTransition(OBSSource transition);

	void TransitionClicked();
	void TransitionStopped();
	void TransitionFullyStopped();
	void TriggerQuickTransition(int id);

	void TBarChanged(int value);
	void TBarReleased();

	void on_transitions_currentIndexChanged(int index);
	void on_transitionAdd_clicked();
	void on_transitionRemove_clicked();
	void on_transitionProps_clicked();
	void on_transitionDuration_valueChanged();

	void ShowTransitionProperties();
	void HideTransitionProperties();

public:
	int GetTransitionDuration();
	int GetTbarPosition();

	/* -------------------------------------
	 * MARK: - OBSBasic_Updater
	 * -------------------------------------
	 */
private:
	QScopedPointer<QThread> whatsNewInitThread;
	QScopedPointer<QThread> updateCheckThread;
	QScopedPointer<QThread> introCheckThread;

	void TimedCheckForUpdates();
	void CheckForUpdates(bool manualUpdate);

	void MacBranchesFetched(const QString &branch, bool manualUpdate);
	void ReceivedIntroJson(const QString &text);
	void ShowWhatsNew(const QString &url);

	/* -------------------------------------
	 * MARK: - OBSBasic_VirtualCam
	 * -------------------------------------
	 */
private:
	bool vcamEnabled = false;
	VCamConfig vcamConfig;
	bool restartingVCam = false;

public slots:
	void StartVirtualCam();
	void StopVirtualCam();

	void OnVirtualCamStart();
	void OnVirtualCamStop(int code);

	bool VirtualCamActive();

private slots:
	void UpdateVirtualCamConfig(const VCamConfig &config);
	void RestartVirtualCam(const VCamConfig &config);
	void RestartingVirtualCam();

	/* Virtual Cam action (start/stop) slots */
	void VirtualCamActionTriggered();

	void OpenVirtualCamConfig();

public:
	inline bool VCamEnabled() const { return vcamEnabled; }

signals:
	/* Virtual Camera signals */
	void VirtualCamEnabled();
	void VirtualCamStarted();
	void VirtualCamStopped();

	/* -------------------------------------
	 * MARK: - OBSBasic_VolControl
	 * -------------------------------------
	 */
private:
	std::vector<VolControl *> volumes;

	void UpdateVolumeControlsDecayRate();
	void UpdateVolumeControlsPeakMeterType();
	void ClearVolumeControls();
	void VolControlContextMenu();
	void ToggleVolControlLayout();
	void ToggleMixerLayout(bool vertical);

private slots:
	void HideAudioControl();
	void UnhideAllAudioControls();
	void ToggleHideMixer();

	void on_vMixerScrollArea_customContextMenuRequested();
	void on_hMixerScrollArea_customContextMenuRequested();

	void LockVolumeControl(bool lock);

	void on_actionMixerToolbarAdvAudio_triggered();
	void on_actionMixerToolbarMenu_triggered();

	void StackedMixerAreaContextMenuRequested();

public:
	void RefreshVolumeColors();

	/* -------------------------------------
	 * MARK: - OBSBasic_YouTube
	 * -------------------------------------
	 */

private:
	bool autoStartBroadcast = true;
	bool autoStopBroadcast = true;
	bool broadcastActive = false;
	bool broadcastReady = false;
	QPointer<QThread> youtubeStreamCheckThread;

#ifdef YOUTUBE_ENABLED
	QPointer<YouTubeAppDock> youtubeAppDock;
	uint64_t lastYouTubeAppDockCreationTime = 0;

	void YoutubeStreamCheck(const std::string &key);
	void ShowYouTubeAutoStartWarning();
	void YouTubeActionDialogOk(const QString &broadcast_id, const QString &stream_id, const QString &key,
				   bool autostart, bool autostop, bool start_now);
#endif

	void BroadcastButtonClicked();
	void SetBroadcastFlowEnabled(bool enabled);

public:
#ifdef YOUTUBE_ENABLED
	void NewYouTubeAppDock();
	void DeleteYouTubeAppDock();
	YouTubeAppDock *GetYouTubeAppDock();
#endif

public slots:
	void SetupBroadcast();

signals:
	/* Broadcast Flow signals */
	void BroadcastFlowEnabled(bool enabled);
	void BroadcastStreamReady(bool ready);
	void BroadcastStreamActive();
	void BroadcastStreamStarted(bool autoStop);
};
