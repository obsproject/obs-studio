/*
 * Copyright (c) 2013-2021, Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "OBSBasic.hpp"
#include "ui-config.h"
#include "ColorSelect.hpp"
#include "OBSBasicControls.hpp"
#include "OBSBasicStats.hpp"
#include "VolControl.hpp"

#ifdef YOUTUBE_ENABLED
#include <docks/YouTubeAppDock.hpp>
#endif
#include <dialogs/NameDialog.hpp>
#include <dialogs/OBSAbout.hpp>
#include <dialogs/OBSBasicAdvAudio.hpp>
#include <dialogs/OBSBasicFilters.hpp>
#include <dialogs/OBSBasicInteraction.hpp>
#include <dialogs/OBSBasicProperties.hpp>
#include <dialogs/OBSBasicTransform.hpp>
#include <models/SceneCollection.hpp>
#include <settings/OBSBasicSettings.hpp>
#include <utility/QuickTransition.hpp>
#include <utility/SceneRenameDelegate.hpp>
#if defined(_WIN32) || defined(WHATSNEW_ENABLED)
#include <utility/WhatsNewInfoThread.hpp>
#endif
#include <widgets/OBSProjector.hpp>

#include <OBSStudioAPI.hpp>
#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif
#ifdef ENABLE_WAYLAND
#include <obs-nix-platform.h>
#endif
#include <qt-wrappers.hpp>

#include <QActionGroup>
#include <QThread>
#include <QWidgetAction>

#ifdef _WIN32
#include <sstream>
#endif
#include <string>
#include <unordered_set>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

#include "moc_OBSBasic.cpp"

using namespace std;

extern bool portable_mode;
extern bool disable_3p_plugins;
extern bool opt_studio_mode;
extern bool opt_always_on_top;
extern bool opt_minimize_tray;
extern std::string opt_starting_profile;
extern std::string opt_starting_collection;

extern bool safe_mode;
extern bool opt_start_recording;
extern bool opt_start_replaybuffer;
extern bool opt_start_virtualcam;

extern volatile long insideEventLoop;
extern bool restart;

extern bool EncoderAvailable(const char *encoder);

extern void RegisterTwitchAuth();
extern void RegisterRestreamAuth();
#ifdef YOUTUBE_ENABLED
extern void RegisterYoutubeAuth();
#endif

struct QCef;

extern QCef *cef;
extern bool cef_js_avail;

extern void DestroyPanelCookieManager();
extern void CheckExistingCookieId();

static void AddExtraModulePaths()
{
	string plugins_path, plugins_data_path;
	char *s;

	s = getenv("OBS_PLUGINS_PATH");
	if (s)
		plugins_path = s;

	s = getenv("OBS_PLUGINS_DATA_PATH");
	if (s)
		plugins_data_path = s;

	if (!plugins_path.empty() && !plugins_data_path.empty()) {
#if defined(__APPLE__)
		plugins_path += "/%module%.plugin/Contents/MacOS";
		plugins_data_path += "/%module%.plugin/Contents/Resources";
		obs_add_module_path(plugins_path.c_str(), plugins_data_path.c_str());
#else
		string data_path_with_module_suffix;
		data_path_with_module_suffix += plugins_data_path;
		data_path_with_module_suffix += "/%module%";
		obs_add_module_path(plugins_path.c_str(), data_path_with_module_suffix.c_str());
#endif
	}

	if (portable_mode)
		return;

	char base_module_dir[512];
#if defined(_WIN32)
	int ret = GetProgramDataPath(base_module_dir, sizeof(base_module_dir), "obs-studio/plugins/%module%");
#elif defined(__APPLE__)
	int ret = GetAppConfigPath(base_module_dir, sizeof(base_module_dir), "obs-studio/plugins/%module%.plugin");
#else
	int ret = GetAppConfigPath(base_module_dir, sizeof(base_module_dir), "obs-studio/plugins/%module%");
#endif

	if (ret <= 0)
		return;

	string path = base_module_dir;
#if defined(__APPLE__)
	/* User Application Support Search Path */
	obs_add_module_path((path + "/Contents/MacOS").c_str(), (path + "/Contents/Resources").c_str());

#ifndef __aarch64__
	/* Legacy System Library Search Path */
	char system_legacy_module_dir[PATH_MAX];
	GetProgramDataPath(system_legacy_module_dir, sizeof(system_legacy_module_dir), "obs-studio/plugins/%module%");
	std::string path_system_legacy = system_legacy_module_dir;
	obs_add_module_path((path_system_legacy + "/bin").c_str(), (path_system_legacy + "/data").c_str());

	/* Legacy User Application Support Search Path */
	char user_legacy_module_dir[PATH_MAX];
	GetAppConfigPath(user_legacy_module_dir, sizeof(user_legacy_module_dir), "obs-studio/plugins/%module%");
	std::string path_user_legacy = user_legacy_module_dir;
	obs_add_module_path((path_user_legacy + "/bin").c_str(), (path_user_legacy + "/data").c_str());
#endif
#else
#if ARCH_BITS == 64
	obs_add_module_path((path + "/bin/64bit").c_str(), (path + "/data").c_str());
#else
	obs_add_module_path((path + "/bin/32bit").c_str(), (path + "/data").c_str());
#endif
#endif
}

/* First-party modules considered to be potentially unsafe to load in Safe Mode
 * due to them allowing external code (e.g. scripts) to modify OBS's state. */
static const unordered_set<string> unsafe_modules = {
	"frontend-tools", // Scripting
	"obs-websocket",  // Allows outside modifications
};

static void SetSafeModuleNames()
{
#ifndef SAFE_MODULES
	return;
#else
	string module;
	stringstream modules(SAFE_MODULES);

	while (getline(modules, module, '|')) {
		/* When only disallowing third-party plugins, still add
		 * "unsafe" bundled modules to the safe list. */
		if (disable_3p_plugins || !unsafe_modules.count(module))
			obs_add_safe_module(module.c_str());
	}
#endif
}

extern void setupDockAction(QDockWidget *dock);

OBSBasic::OBSBasic(QWidget *parent) : OBSMainWindow(parent), undo_s(ui), ui(new Ui::OBSBasic)
{
	collections = {};

	setAttribute(Qt::WA_NativeWindow);

#ifdef TWITCH_ENABLED
	RegisterTwitchAuth();
#endif
#ifdef RESTREAM_ENABLED
	RegisterRestreamAuth();
#endif
#ifdef YOUTUBE_ENABLED
	RegisterYoutubeAuth();
#endif

	setAcceptDrops(true);

	setContextMenuPolicy(Qt::CustomContextMenu);

	QEvent::registerEventType(QEvent::User + QEvent::Close);

	api = InitializeAPIInterface(this);

	ui->setupUi(this);
	ui->previewDisabledWidget->setVisible(false);

	/* Set up streaming connections */
	connect(
		this, &OBSBasic::StreamingStarting, this, [this] { this->streamingStarting = true; },
		Qt::DirectConnection);
	connect(
		this, &OBSBasic::StreamingStarted, this, [this] { this->streamingStarting = false; },
		Qt::DirectConnection);
	connect(
		this, &OBSBasic::StreamingStopped, this, [this] { this->streamingStarting = false; },
		Qt::DirectConnection);

	/* Set up recording connections */
	connect(
		this, &OBSBasic::RecordingStarted, this,
		[this]() {
			this->recordingStarted = true;
			this->recordingPaused = false;
		},
		Qt::DirectConnection);
	connect(
		this, &OBSBasic::RecordingPaused, this, [this]() { this->recordingPaused = true; },
		Qt::DirectConnection);
	connect(
		this, &OBSBasic::RecordingUnpaused, this, [this]() { this->recordingPaused = false; },
		Qt::DirectConnection);
	connect(
		this, &OBSBasic::RecordingStopped, this,
		[this]() {
			this->recordingStarted = false;
			this->recordingPaused = false;
		},
		Qt::DirectConnection);

	/* Add controls dock */
	OBSBasicControls *controls = new OBSBasicControls(this);
	controlsDock = new OBSDock(this);
	controlsDock->setObjectName(QString::fromUtf8("controlsDock"));
	controlsDock->setWindowTitle(QTStr("Basic.Main.Controls"));
	/* Parenting is done there so controls will be deleted alongside controlsDock */
	controlsDock->setWidget(controls);
	addDockWidget(Qt::BottomDockWidgetArea, controlsDock);

	connect(controls, &OBSBasicControls::StreamButtonClicked, this, &OBSBasic::StreamActionTriggered);

	connect(controls, &OBSBasicControls::StartStreamMenuActionClicked, this, &OBSBasic::StartStreaming);
	connect(controls, &OBSBasicControls::StopStreamMenuActionClicked, this, &OBSBasic::StopStreaming);
	connect(controls, &OBSBasicControls::ForceStopStreamMenuActionClicked, this, &OBSBasic::ForceStopStreaming);

	connect(controls, &OBSBasicControls::BroadcastButtonClicked, this, &OBSBasic::BroadcastButtonClicked);

	connect(controls, &OBSBasicControls::RecordButtonClicked, this, &OBSBasic::RecordActionTriggered);
	connect(controls, &OBSBasicControls::PauseRecordButtonClicked, this, &OBSBasic::RecordPauseToggled);

	connect(controls, &OBSBasicControls::ReplayBufferButtonClicked, this, &OBSBasic::ReplayBufferActionTriggered);
	connect(controls, &OBSBasicControls::SaveReplayBufferButtonClicked, this, &OBSBasic::ReplayBufferSave);

	connect(controls, &OBSBasicControls::VirtualCamButtonClicked, this, &OBSBasic::VirtualCamActionTriggered);
	connect(controls, &OBSBasicControls::VirtualCamConfigButtonClicked, this, &OBSBasic::OpenVirtualCamConfig);

	connect(controls, &OBSBasicControls::StudioModeButtonClicked, this, &OBSBasic::TogglePreviewProgramMode);

	connect(controls, &OBSBasicControls::SettingsButtonClicked, this, &OBSBasic::on_action_Settings_triggered);

	startingDockLayout = saveState();

	statsDock = new OBSDock();
	statsDock->setObjectName(QStringLiteral("statsDock"));
	statsDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
			       QDockWidget::DockWidgetFloatable);
	statsDock->setWindowTitle(QTStr("Basic.Stats"));
	addDockWidget(Qt::BottomDockWidgetArea, statsDock);
	statsDock->setVisible(false);
	statsDock->setFloating(true);
	statsDock->resize(700, 200);

	copyActionsDynamicProperties();

	qRegisterMetaType<int64_t>("int64_t");
	qRegisterMetaType<uint32_t>("uint32_t");
	qRegisterMetaType<OBSScene>("OBSScene");
	qRegisterMetaType<OBSSceneItem>("OBSSceneItem");
	qRegisterMetaType<OBSSource>("OBSSource");
	qRegisterMetaType<obs_hotkey_id>("obs_hotkey_id");
	qRegisterMetaType<SavedProjectorInfo *>("SavedProjectorInfo *");

	ui->scenes->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui->sources->setAttribute(Qt::WA_MacShowFocusRect, false);

	bool sceneGrid = config_get_bool(App()->GetUserConfig(), "BasicWindow", "gridMode");
	ui->scenes->SetGridMode(sceneGrid);

	if (sceneGrid)
		ui->actionSceneGridMode->setChecked(true);
	else
		ui->actionSceneListMode->setChecked(true);

	ui->scenes->setItemDelegate(new SceneRenameDelegate(ui->scenes));

	auto displayResize = [this]() {
		struct obs_video_info ovi;

		if (obs_get_video_info(&ovi))
			ResizePreview(ovi.base_width, ovi.base_height);

		UpdateContextBarVisibility();
		UpdatePreviewControls();
		dpi = devicePixelRatioF();
	};
	dpi = devicePixelRatioF();

	connect(windowHandle(), &QWindow::screenChanged, displayResize);
	connect(ui->preview, &OBSQTDisplay::DisplayResized, displayResize); // For horizontal

	// Connect DisplayResized for Vertical Preview
	auto displayResizeVertical = [this]() {
		if (App()->IsDualOutputActive()) {
			const obs_video_info* v_ovi = App()->GetVerticalVideoInfo();
			if (v_ovi && v_ovi->base_width > 0) {
				ResizePreview_V(v_ovi->base_width, v_ovi->base_height);
			} else if (v_ovi) { // OVI exists but might be zeroed, use current graphics output
				struct obs_video_info temp_ovi;
				obs_get_video_info(&temp_ovi); // Fallback, might not be ideal for vertical
				ResizePreview_V(temp_ovi.base_width, temp_ovi.base_height);
			}
		}
	};
	if (ui->mainPreview_v) {
		connect(ui->mainPreview_v, &OBSQTDisplay::DisplayResized, displayResizeVertical);
		// Also connect screenChanged to potentially update vertical preview if its size depends on screen
		connect(windowHandle(), &QWindow::screenChanged, displayResizeVertical);
	}


	/* TODO: Move these into window-basic-preview */
	/* Preview Scaling label */
	connect(ui->preview, &OBSBasicPreview::scalingChanged, ui->previewScalePercent,
		&OBSPreviewScalingLabel::PreviewScaleChanged);

	/* Preview Scaling dropdown */
	connect(ui->preview, &OBSBasicPreview::scalingChanged, ui->previewScalingMode,
		&OBSPreviewScalingComboBox::PreviewScaleChanged);

	connect(ui->preview, &OBSBasicPreview::fixedScalingChanged, ui->previewScalingMode,
		&OBSPreviewScalingComboBox::PreviewFixedScalingChanged);

	connect(ui->previewScalingMode, &OBSPreviewScalingComboBox::currentIndexChanged, this,
		&OBSBasic::PreviewScalingModeChanged);

	/* Preview Controls */
	connect(ui->previewXScrollBar, &QScrollBar::sliderMoved, ui->preview, &OBSBasicPreview::xScrollBarChanged);
	connect(ui->previewYScrollBar, &QScrollBar::valueChanged, ui->preview, &OBSBasicPreview::yScrollBarChanged);

	connect(ui->previewZoomInButton, &QPushButton::clicked, ui->preview, &OBSBasicPreview::increaseScalingLevel);
	connect(ui->previewZoomOutButton, &QPushButton::clicked, ui->preview, &OBSBasicPreview::decreaseScalingLevel);

	/* Preview Actions */
	connect(ui->actionScaleWindow, &QAction::triggered, this, &OBSBasic::setPreviewScalingWindow);
	connect(ui->actionScaleCanvas, &QAction::triggered, this, &OBSBasic::setPreviewScalingCanvas);
	connect(ui->actionScaleOutput, &QAction::triggered, this, &OBSBasic::setPreviewScalingOutput);

	connect(ui->actionPreviewZoomIn, &QAction::triggered, ui->preview, &OBSBasicPreview::increaseScalingLevel);
	connect(ui->actionPreviewZoomOut, &QAction::triggered, ui->preview, &OBSBasicPreview::decreaseScalingLevel);
	connect(ui->actionPreviewResetZoom, &QAction::triggered, ui->preview, &OBSBasicPreview::resetScalingLevel);

	connect(this, &OBSBasic::CanvasResized, ui->previewScalingMode, &OBSPreviewScalingComboBox::CanvasResized);
	connect(this, &OBSBasic::OutputResized, ui->previewScalingMode, &OBSPreviewScalingComboBox::OutputResized);

	// Connections for Vertical Preview controls
	if (ui->mainPreview_v) {
		connect(ui->previewXScrollBar_v, &QScrollBar::sliderMoved, ui->mainPreview_v, &OBSBasicPreview::xScrollBarChanged);
		connect(ui->previewYScrollBar_v, &QScrollBar::valueChanged, ui->mainPreview_v, &OBSBasicPreview::yScrollBarChanged);
		connect(ui->previewZoomInButton_v, &QPushButton::clicked, ui->mainPreview_v, &OBSBasicPreview::increaseScalingLevel);
		connect(ui->previewZoomOutButton_v, &QPushButton::clicked, ui->mainPreview_v, &OBSBasicPreview::decreaseScalingLevel);

		// Connect vertical preview scaling controls to their new slots
		connect(ui->previewScalingMode_v, &OBSPreviewScalingComboBox::currentIndexChanged, this, &OBSBasic::PreviewScalingModeChanged_V);
		// Assuming previewScalePercent_v is a QSpinBox or similar that emits valueChanged(int)
		// connect(ui->previewScalePercent_v, SIGNAL(valueChanged(int)), this, SLOT(PreviewScalePercentChanged_V(int))); // This widget might not exist or is a label

		// Connect actions for vertical preview scaling if they exist (e.g. in a context menu for vertical preview)
		// For now, we assume the main preview scaling actions might be conditionally used or new ones are needed.
		// If using the same actions, their triggered slots would need to check 'activePreviewPane'.
		// Example if new actions specific to vertical preview context menu were added:
		// connect(ui->actionScaleWindow_v, &QAction::triggered, this, &OBSBasic::setPreviewScalingWindow_V);
		// connect(ui->actionScaleCanvas_v, &QAction::triggered, this, &OBSBasic::setPreviewScalingCanvas_V);
		// connect(ui->actionScaleOutput_v, &QAction::triggered, this, &OBSBasic::setPreviewScalingOutput_V);

		// Connect the scaling label for vertical preview
		connect(ui->mainPreview_v, &OBSBasicPreview::scalingChanged, ui->previewScalePercent_v, &OBSPreviewScalingLabel::PreviewScaleChanged);
		connect(ui->mainPreview_v, &OBSBasicPreview::fixedScalingChanged, ui->previewScalingMode_v, &OBSPreviewScalingComboBox::PreviewFixedScalingChanged);

	}

	delete shortcutFilter;
	shortcutFilter = CreateShortcutFilter();
	installEventFilter(shortcutFilter);

	stringstream name;
	name << "OBS " << App()->GetVersionString();
	blog(LOG_INFO, "%s", name.str().c_str());
	blog(LOG_INFO, "---------------------------------");

	UpdateTitleBar();

	connect(ui->scenes->itemDelegate(), &QAbstractItemDelegate::closeEditor, this, &OBSBasic::SceneNameEdited);

	cpuUsageInfo = os_cpu_usage_info_start();
	cpuUsageTimer = new QTimer(this);
	connect(cpuUsageTimer.data(), &QTimer::timeout, ui->statusbar, &OBSBasicStatusBar::UpdateCPUUsage);
	cpuUsageTimer->start(3000);

	diskFullTimer = new QTimer(this);
	connect(diskFullTimer, &QTimer::timeout, this, &OBSBasic::CheckDiskSpaceRemaining);

	renameScene = new QAction(QTStr("Rename"), ui->scenesDock);
	renameScene->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(renameScene, &QAction::triggered, this, &OBSBasic::EditSceneName);
	ui->scenesDock->addAction(renameScene);

	renameSource = new QAction(QTStr("Rename"), ui->sourcesDock);
	renameSource->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(renameSource, &QAction::triggered, this, &OBSBasic::EditSceneItemName);
	ui->sourcesDock->addAction(renameSource);

#ifdef __APPLE__
	renameScene->setShortcut({Qt::Key_Return});
	renameSource->setShortcut({Qt::Key_Return});

	ui->actionRemoveSource->setShortcuts({Qt::Key_Backspace});
	ui->actionRemoveScene->setShortcuts({Qt::Key_Backspace});

	ui->actionCheckForUpdates->setMenuRole(QAction::AboutQtRole);
	ui->action_Settings->setMenuRole(QAction::PreferencesRole);
	ui->actionShowMacPermissions->setMenuRole(QAction::ApplicationSpecificRole);
	ui->actionE_xit->setMenuRole(QAction::QuitRole);
#else
	renameScene->setShortcut({Qt::Key_F2});
	renameSource->setShortcut({Qt::Key_F2});
#endif

#ifdef __linux__
	ui->actionE_xit->setShortcut(Qt::CTRL | Qt::Key_Q);
#endif

	auto addNudge = [this](const QKeySequence &seq, MoveDir direction, int distance) {
		QAction *nudge = new QAction(ui->preview);
		nudge->setShortcut(seq);
		nudge->setShortcutContext(Qt::WidgetShortcut);
		ui->preview->addAction(nudge);
		connect(nudge, &QAction::triggered, [this, distance, direction]() { Nudge(distance, direction); });
	};

	addNudge(Qt::Key_Up, MoveDir::Up, 1);
	addNudge(Qt::Key_Down, MoveDir::Down, 1);
	addNudge(Qt::Key_Left, MoveDir::Left, 1);
	addNudge(Qt::Key_Right, MoveDir::Right, 1);
	addNudge(Qt::SHIFT | Qt::Key_Up, MoveDir::Up, 10);
	addNudge(Qt::SHIFT | Qt::Key_Down, MoveDir::Down, 10);
	addNudge(Qt::SHIFT | Qt::Key_Left, MoveDir::Left, 10);
	addNudge(Qt::SHIFT | Qt::Key_Right, MoveDir::Right, 10);

	/* Setup dock toggle action
	 * And hide all docks before restoring parent geometry */
#define SETUP_DOCK(dock)                                    \
	setupDockAction(dock);                              \
	ui->menuDocks->addAction(dock->toggleViewAction()); \
	dock->setVisible(false);

	SETUP_DOCK(ui->scenesDock);
	SETUP_DOCK(ui->sourcesDock);
	SETUP_DOCK(ui->mixerDock);
	SETUP_DOCK(ui->transitionsDock);
	SETUP_DOCK(controlsDock);
	SETUP_DOCK(statsDock);
#undef SETUP_DOCK

	// Register shortcuts for Undo/Redo
	ui->actionMainUndo->setShortcut(Qt::CTRL | Qt::Key_Z);
	QList<QKeySequence> shrt;
	shrt << QKeySequence((Qt::CTRL | Qt::SHIFT) | Qt::Key_Z) << QKeySequence(Qt::CTRL | Qt::Key_Y);
	ui->actionMainRedo->setShortcuts(shrt);

	ui->actionMainUndo->setShortcutContext(Qt::ApplicationShortcut);
	ui->actionMainRedo->setShortcutContext(Qt::ApplicationShortcut);

	QPoint curPos;

	//restore parent window geometry
	const char *geometry = config_get_string(App()->GetUserConfig(), "BasicWindow", "geometry");
	if (geometry != NULL) {
		QByteArray byteArray = QByteArray::fromBase64(QByteArray(geometry));
		restoreGeometry(byteArray);

		QRect windowGeometry = normalGeometry();
		if (!WindowPositionValid(windowGeometry)) {
			QRect rect = QGuiApplication::primaryScreen()->geometry();
			setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), rect));
		}

		curPos = pos();
	} else {
		QRect desktopRect = QGuiApplication::primaryScreen()->geometry();
		QSize adjSize = desktopRect.size() / 2 - size() / 2;
		curPos = QPoint(adjSize.width(), adjSize.height());
	}

	QPoint curSize(width(), height());

	QPoint statsDockSize(statsDock->width(), statsDock->height());
	QPoint statsDockPos = curSize / 2 - statsDockSize / 2;
	QPoint newPos = curPos + statsDockPos;
	statsDock->move(newPos);

	ui->actionReleaseNotes->setVisible(true);

	ui->previewDisabledWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->enablePreviewButton, &QPushButton::clicked, this, &OBSBasic::TogglePreview);

	connect(ui->scenes, &SceneTree::scenesReordered, []() { OBSProjector::UpdateMultiviewProjectors(); });

	connect(App(), &OBSApp::StyleChanged, this, [this]() { OnEvent(OBS_FRONTEND_EVENT_THEME_CHANGED); });

	QActionGroup *actionGroup = new QActionGroup(this);
	actionGroup->addAction(ui->actionSceneListMode);
	actionGroup->addAction(ui->actionSceneGridMode);

	UpdatePreviewSafeAreas();
	UpdatePreviewSpacingHelpers();
	UpdatePreviewOverflowSettings();

	// Install event filters for preview panes
	if (ui->mainPreview_h) { // Renamed from ui->preview
		ui->mainPreview_h->installEventFilter(this);
	}
	if (ui->mainPreview_v) {
		ui->mainPreview_v->installEventFilter(this);
	}

	// Load ActivePreviewPane state
	int savedPaneIndex = config_get_int(App()->GetUserConfig(), "BasicWindow", "ActivePreviewPane", 0);
	activePreviewPane = (savedPaneIndex == 1) ? ActivePreview::VERTICAL : ActivePreview::HORIZONTAL;
	// RefreshSceneListDisplay(); // Will be called later in OBSInit after scenes are loaded.

	// Restore splitter state
	if (ui->previewSplitter) {
		QByteArray splitterState = QByteArray::fromBase64(config_get_string(App()->GetUserConfig(), "BasicWindow", "PreviewSplitterState", ""));
		if (!splitterState.isEmpty()) {
			ui->previewSplitter->restoreState(splitterState);
		}
	}
}

static const double scaled_vals[] = {1.0, 1.25, (1.0 / 0.75), 1.5, (1.0 / 0.6), 1.75, 2.0, 2.25, 2.5, 2.75, 3.0, 0.0};

#ifdef __APPLE__
#define DEFAULT_CONTAINER "fragmented_mov"
#elif OBS_RELEASE_CANDIDATE == 0 && OBS_BETA == 0
#define DEFAULT_CONTAINER "mkv"
#else
#define DEFAULT_CONTAINER "hybrid_mp4"
#endif

bool OBSBasic::InitBasicConfigDefaults()
{
	QList<QScreen *> screens = QGuiApplication::screens();

	if (!screens.size()) {
		OBSErrorBox(NULL, "There appears to be no monitors.  Er, this "
				  "technically shouldn't be possible.");
		return false;
	}

	QScreen *primaryScreen = QGuiApplication::primaryScreen();

	uint32_t cx = primaryScreen->size().width();
	uint32_t cy = primaryScreen->size().height();

	cx *= devicePixelRatioF();
	cy *= devicePixelRatioF();

	bool oldResolutionDefaults = config_get_bool(App()->GetUserConfig(), "General", "Pre19Defaults");

	/* use 1920x1080 for new default base res if main monitor is above
	 * 1920x1080, but don't apply for people from older builds -- only to
	 * new users */
	if (!oldResolutionDefaults && (cx * cy) > (1920 * 1080)) {
		cx = 1920;
		cy = 1080;
	}

	bool changed = false;

	/* ----------------------------------------------------- */
	/* move over old FFmpeg track settings                   */
	if (config_has_user_value(activeConfiguration, "AdvOut", "FFAudioTrack") &&
	    !config_has_user_value(activeConfiguration, "AdvOut", "Pre22.1Settings")) {

		int track = (int)config_get_int(activeConfiguration, "AdvOut", "FFAudioTrack");
		config_set_int(activeConfiguration, "AdvOut", "FFAudioMixes", 1LL << (track - 1));
		config_set_bool(activeConfiguration, "AdvOut", "Pre22.1Settings", true);
		changed = true;
	}

	/* ----------------------------------------------------- */
	/* move over mixer values in advanced if older config */
	if (config_has_user_value(activeConfiguration, "AdvOut", "RecTrackIndex") &&
	    !config_has_user_value(activeConfiguration, "AdvOut", "RecTracks")) {

		uint64_t track = config_get_uint(activeConfiguration, "AdvOut", "RecTrackIndex");
		track = 1ULL << (track - 1);
		config_set_uint(activeConfiguration, "AdvOut", "RecTracks", track);
		config_remove_value(activeConfiguration, "AdvOut", "RecTrackIndex");
		changed = true;
	}

	/* ----------------------------------------------------- */
	/* set twitch chat extensions to "both" if prev version  */
	/* is under 24.1                                         */
	if (config_get_bool(App()->GetUserConfig(), "General", "Pre24.1Defaults") &&
	    !config_has_user_value(activeConfiguration, "Twitch", "AddonChoice")) {
		config_set_int(activeConfiguration, "Twitch", "AddonChoice", 3);
		changed = true;
	}

	/* ----------------------------------------------------- */
	/* move bitrate enforcement setting to new value         */
	if (config_has_user_value(activeConfiguration, "SimpleOutput", "EnforceBitrate") &&
	    !config_has_user_value(activeConfiguration, "Stream1", "IgnoreRecommended") &&
	    !config_has_user_value(activeConfiguration, "Stream1", "MovedOldEnforce")) {
		bool enforce = config_get_bool(activeConfiguration, "SimpleOutput", "EnforceBitrate");
		config_set_bool(activeConfiguration, "Stream1", "IgnoreRecommended", !enforce);
		config_set_bool(activeConfiguration, "Stream1", "MovedOldEnforce", true);
		changed = true;
	}

	/* ----------------------------------------------------- */
	/* enforce minimum retry delay of 1 second prior to 27.1 */
	if (config_has_user_value(activeConfiguration, "Output", "RetryDelay")) {
		int retryDelay = config_get_uint(activeConfiguration, "Output", "RetryDelay");
		if (retryDelay < 1) {
			config_set_uint(activeConfiguration, "Output", "RetryDelay", 1);
			changed = true;
		}
	}

	/* ----------------------------------------------------- */
	/* Migrate old container selection (if any) to new key.  */

	auto MigrateFormat = [&](const char *section) {
		bool has_old_key = config_has_user_value(activeConfiguration, section, "RecFormat");
		bool has_new_key = config_has_user_value(activeConfiguration, section, "RecFormat2");
		if (!has_new_key && !has_old_key)
			return;

		string old_format =
			config_get_string(activeConfiguration, section, has_new_key ? "RecFormat2" : "RecFormat");
		string new_format = old_format;
		if (old_format == "ts")
			new_format = "mpegts";
		else if (old_format == "m3u8")
			new_format = "hls";
		else if (old_format == "fmp4")
			new_format = "fragmented_mp4";
		else if (old_format == "fmov")
			new_format = "fragmented_mov";

		if (new_format != old_format || !has_new_key) {
			config_set_string(activeConfiguration, section, "RecFormat2", new_format.c_str());
			changed = true;
		}
	};

	MigrateFormat("AdvOut");
	MigrateFormat("SimpleOutput");

	/* ----------------------------------------------------- */
	/* Migrate output scale setting to GPU scaling options.  */

	if (config_get_bool(activeConfiguration, "AdvOut", "Rescale") &&
	    !config_has_user_value(activeConfiguration, "AdvOut", "RescaleFilter")) {
		config_set_int(activeConfiguration, "AdvOut", "RescaleFilter", OBS_SCALE_BILINEAR);
	}

	if (config_get_bool(activeConfiguration, "AdvOut", "RecRescale") &&
	    !config_has_user_value(activeConfiguration, "AdvOut", "RecRescaleFilter")) {
		config_set_int(activeConfiguration, "AdvOut", "RecRescaleFilter", OBS_SCALE_BILINEAR);
	}

	/* ----------------------------------------------------- */

	if (changed) {
		activeConfiguration.SaveSafe("tmp");
	}

	/* ----------------------------------------------------- */

	config_set_default_string(activeConfiguration, "Output", "Mode", "Simple");

	config_set_default_bool(activeConfiguration, "Stream1", "IgnoreRecommended", false);
	config_set_default_bool(activeConfiguration, "Stream1", "EnableMultitrackVideo", false);
	config_set_default_bool(activeConfiguration, "Stream1", "MultitrackVideoMaximumAggregateBitrateAuto", true);
	config_set_default_bool(activeConfiguration, "Stream1", "MultitrackVideoMaximumVideoTracksAuto", true);

	config_set_default_string(activeConfiguration, "SimpleOutput", "FilePath", GetDefaultVideoSavePath().c_str());
	config_set_default_string(activeConfiguration, "SimpleOutput", "RecFormat2", DEFAULT_CONTAINER);
	config_set_default_uint(activeConfiguration, "SimpleOutput", "VBitrate", 2500);
	config_set_default_uint(activeConfiguration, "SimpleOutput", "ABitrate", 160);
	config_set_default_bool(activeConfiguration, "SimpleOutput", "UseAdvanced", false);
	config_set_default_string(activeConfiguration, "SimpleOutput", "Preset", "veryfast");
	config_set_default_string(activeConfiguration, "SimpleOutput", "NVENCPreset2", "p5");
	config_set_default_string(activeConfiguration, "SimpleOutput", "RecQuality", "Stream");
	config_set_default_bool(activeConfiguration, "SimpleOutput", "RecRB", false);
	config_set_default_int(activeConfiguration, "SimpleOutput", "RecRBTime", 20);
	config_set_default_int(activeConfiguration, "SimpleOutput", "RecRBSize", 512);
	config_set_default_string(activeConfiguration, "SimpleOutput", "RecRBPrefix", "Replay");
	config_set_default_string(activeConfiguration, "SimpleOutput", "StreamAudioEncoder", "aac");
	config_set_default_string(activeConfiguration, "SimpleOutput", "RecAudioEncoder", "aac");
	config_set_default_uint(activeConfiguration, "SimpleOutput", "RecTracks", (1 << 0));

	config_set_default_bool(activeConfiguration, "AdvOut", "ApplyServiceSettings", true);
	config_set_default_bool(activeConfiguration, "AdvOut", "UseRescale", false);
	config_set_default_uint(activeConfiguration, "AdvOut", "TrackIndex", 1);
	config_set_default_uint(activeConfiguration, "AdvOut", "VodTrackIndex", 2);
	config_set_default_string(activeConfiguration, "AdvOut", "Encoder", "obs_x264");

	config_set_default_string(activeConfiguration, "AdvOut", "RecType", "Standard");

	config_set_default_string(activeConfiguration, "AdvOut", "RecFilePath", GetDefaultVideoSavePath().c_str());
	config_set_default_string(activeConfiguration, "AdvOut", "RecFormat2", DEFAULT_CONTAINER);
	config_set_default_bool(activeConfiguration, "AdvOut", "RecUseRescale", false);
	config_set_default_uint(activeConfiguration, "AdvOut", "RecTracks", (1 << 0));
	config_set_default_string(activeConfiguration, "AdvOut", "RecEncoder", "none");
	config_set_default_uint(activeConfiguration, "AdvOut", "FLVTrack", 1);
	config_set_default_uint(activeConfiguration, "AdvOut", "StreamMultiTrackAudioMixes", 1);
	config_set_default_bool(activeConfiguration, "AdvOut", "FFOutputToFile", true);
	config_set_default_string(activeConfiguration, "AdvOut", "FFFilePath", GetDefaultVideoSavePath().c_str());
	config_set_default_string(activeConfiguration, "AdvOut", "FFExtension", "mp4");
	config_set_default_uint(activeConfiguration, "AdvOut", "FFVBitrate", 2500);
	config_set_default_uint(activeConfiguration, "AdvOut", "FFVGOPSize", 250);
	config_set_default_bool(activeConfiguration, "AdvOut", "FFUseRescale", false);
	config_set_default_bool(activeConfiguration, "AdvOut", "FFIgnoreCompat", false);
	config_set_default_uint(activeConfiguration, "AdvOut", "FFABitrate", 160);
	config_set_default_uint(activeConfiguration, "AdvOut", "FFAudioMixes", 1);

	config_set_default_uint(activeConfiguration, "AdvOut", "Track1Bitrate", 160);
	config_set_default_uint(activeConfiguration, "AdvOut", "Track2Bitrate", 160);
	config_set_default_uint(activeConfiguration, "AdvOut", "Track3Bitrate", 160);
	config_set_default_uint(activeConfiguration, "AdvOut", "Track4Bitrate", 160);
	config_set_default_uint(activeConfiguration, "AdvOut", "Track5Bitrate", 160);
	config_set_default_uint(activeConfiguration, "AdvOut", "Track6Bitrate", 160);

	config_set_default_uint(activeConfiguration, "AdvOut", "RecSplitFileTime", 15);
	config_set_default_uint(activeConfiguration, "AdvOut", "RecSplitFileSize", 2048);

	config_set_default_bool(activeConfiguration, "AdvOut", "RecRB", false);
	config_set_default_uint(activeConfiguration, "AdvOut", "RecRBTime", 20);
	config_set_default_int(activeConfiguration, "AdvOut", "RecRBSize", 512);

	config_set_default_uint(activeConfiguration, "Video", "BaseCX", cx);
	config_set_default_uint(activeConfiguration, "Video", "BaseCY", cy);

	/* don't allow BaseCX/BaseCY to be susceptible to defaults changing */
	if (!config_has_user_value(activeConfiguration, "Video", "BaseCX") ||
	    !config_has_user_value(activeConfiguration, "Video", "BaseCY")) {
		config_set_uint(activeConfiguration, "Video", "BaseCX", cx);
		config_set_uint(activeConfiguration, "Video", "BaseCY", cy);
		config_save_safe(activeConfiguration, "tmp", nullptr);
	}

	config_set_default_string(activeConfiguration, "Output", "FilenameFormatting", "%CCYY-%MM-%DD %hh-%mm-%ss");

	config_set_default_bool(activeConfiguration, "Output", "DelayEnable", false);
	config_set_default_uint(activeConfiguration, "Output", "DelaySec", 20);
	config_set_default_bool(activeConfiguration, "Output", "DelayPreserve", true);

	config_set_default_bool(activeConfiguration, "Output", "Reconnect", true);
	config_set_default_uint(activeConfiguration, "Output", "RetryDelay", 2);
	config_set_default_uint(activeConfiguration, "Output", "MaxRetries", 25);

	config_set_default_string(activeConfiguration, "Output", "BindIP", "default");
	config_set_default_string(activeConfiguration, "Output", "IPFamily", "IPv4+IPv6");
	config_set_default_bool(activeConfiguration, "Output", "NewSocketLoopEnable", false);
	config_set_default_bool(activeConfiguration, "Output", "LowLatencyEnable", false);

	int i = 0;
	uint32_t scale_cx = cx;
	uint32_t scale_cy = cy;

	/* use a default scaled resolution that has a pixel count no higher
	 * than 1280x720 */
	while (((scale_cx * scale_cy) > (1280 * 720)) && scaled_vals[i] > 0.0) {
		double scale = scaled_vals[i++];
		scale_cx = uint32_t(double(cx) / scale);
		scale_cy = uint32_t(double(cy) / scale);
	}

	config_set_default_uint(activeConfiguration, "Video", "OutputCX", scale_cx);
	config_set_default_uint(activeConfiguration, "Video", "OutputCY", scale_cy);

	/* don't allow OutputCX/OutputCY to be susceptible to defaults
	 * changing */
	if (!config_has_user_value(activeConfiguration, "Video", "OutputCX") ||
	    !config_has_user_value(activeConfiguration, "Video", "OutputCY")) {
		config_set_uint(activeConfiguration, "Video", "OutputCX", scale_cx);
		config_set_uint(activeConfiguration, "Video", "OutputCY", scale_cy);
		config_save_safe(activeConfiguration, "tmp", nullptr);
	}

	config_set_default_uint(activeConfiguration, "Video", "FPSType", 0);
	config_set_default_string(activeConfiguration, "Video", "FPSCommon", "30");
	config_set_default_uint(activeConfiguration, "Video", "FPSInt", 30);
	config_set_default_uint(activeConfiguration, "Video", "FPSNum", 30);
	config_set_default_uint(activeConfiguration, "Video", "FPSDen", 1);
	config_set_default_string(activeConfiguration, "Video", "ScaleType", "bicubic");
	config_set_default_string(activeConfiguration, "Video", "ColorFormat", "NV12");
	config_set_default_string(activeConfiguration, "Video", "ColorSpace", "709");
	config_set_default_string(activeConfiguration, "Video", "ColorRange", "Partial");
	config_set_default_uint(activeConfiguration, "Video", "SdrWhiteLevel", 300);
	config_set_default_uint(activeConfiguration, "Video", "HdrNominalPeakLevel", 1000);

	// Defaults for Vertical Video Output (V_ prefix)
	config_set_default_uint(activeConfiguration, "Video", "V_BaseCX", cy); // Default to portrait of primary
	config_set_default_uint(activeConfiguration, "Video", "V_BaseCY", cx);
	config_set_default_uint(activeConfiguration, "Video", "V_OutputCX", scale_cy); // Default to scaled portrait
	config_set_default_uint(activeConfiguration, "Video", "V_OutputCY", scale_cx);
	config_set_default_uint(activeConfiguration, "Video", "V_FPSType", 0); // Common
	config_set_default_string(activeConfiguration, "Video", "V_FPSCommon", "30");
	config_set_default_uint(activeConfiguration, "Video", "V_FPSInt", 30);
	config_set_default_uint(activeConfiguration, "Video", "V_FPSNum", 30);
	config_set_default_uint(activeConfiguration, "Video", "V_FPSDen", 1);
	config_set_default_string(activeConfiguration, "Video", "V_ScaleType", "bicubic");
	config_set_default_string(activeConfiguration, "Video", "V_ColorFormat", "NV12");
	config_set_default_string(activeConfiguration, "Video", "V_ColorSpace", "709");
	config_set_default_string(activeConfiguration, "Video", "V_ColorRange", "Partial");
	// V_SdrWhiteLevel and V_HdrNominalPeakLevel could also be added if needed

	config_set_default_string(activeConfiguration, "Audio", "MonitoringDeviceId", "default");
	config_set_default_string(activeConfiguration, "Audio", "MonitoringDeviceName",
				  Str("Basic.Settings.Advanced.Audio.MonitoringDevice"
				      ".Default"));
	config_set_default_uint(activeConfiguration, "Audio", "SampleRate", 48000);
	config_set_default_string(activeConfiguration, "Audio", "ChannelSetup", "Stereo");
	config_set_default_double(activeConfiguration, "Audio", "MeterDecayRate", VOLUME_METER_DECAY_FAST);
	config_set_default_uint(activeConfiguration, "Audio", "PeakMeterType", 0);

	CheckExistingCookieId();

	return true;
}

void OBSBasic::InitBasicConfigDefaults2()
{
	bool oldEncDefaults = config_get_bool(App()->GetUserConfig(), "General", "Pre23Defaults");
	bool useNV = EncoderAvailable("ffmpeg_nvenc") && !oldEncDefaults;

	config_set_default_string(activeConfiguration, "SimpleOutput", "StreamEncoder",
				  useNV ? SIMPLE_ENCODER_NVENC : SIMPLE_ENCODER_X264);
	config_set_default_string(activeConfiguration, "SimpleOutput", "RecEncoder",
				  useNV ? SIMPLE_ENCODER_NVENC : SIMPLE_ENCODER_X264);

	const char *aac_default = "ffmpeg_aac";
	if (EncoderAvailable("CoreAudio_AAC"))
		aac_default = "CoreAudio_AAC";
	else if (EncoderAvailable("libfdk_aac"))
		aac_default = "libfdk_aac";

	config_set_default_string(activeConfiguration, "AdvOut", "AudioEncoder", aac_default);
	config_set_default_string(activeConfiguration, "AdvOut", "RecAudioEncoder", aac_default);
}

bool OBSBasic::InitBasicConfig()
{
	ProfileScope("OBSBasic::InitBasicConfig");

	RefreshProfiles(true);

	const std::string currentProfileName{config_get_string(App()->GetUserConfig(), "Basic", "Profile")};
	const std::optional<OBSProfile> currentProfile = GetProfileByName(currentProfileName);
	const std::optional<OBSProfile> foundProfile = GetProfileByName(opt_starting_profile);

	try {
		if (foundProfile) {
			ActivateProfile(foundProfile.value());
		} else if (currentProfile) {
			ActivateProfile(currentProfile.value());
		} else {
			const OBSProfile &newProfile = CreateProfile(currentProfileName);
			ActivateProfile(newProfile);
		}
	} catch (const std::logic_error &) {
		OBSErrorBox(NULL, "Failed to open basic.ini: %d", -1);
		return false;
	}

	return true;
}

void OBSBasic::InitOBSCallbacks()
{
	ProfileScope("OBSBasic::InitOBSCallbacks");

	signalHandlers.reserve(signalHandlers.size() + 10);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_create", OBSBasic::SourceCreated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_remove", OBSBasic::SourceRemoved, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_activate", OBSBasic::SourceActivated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_deactivate", OBSBasic::SourceDeactivated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_activate", OBSBasic::SourceAudioActivated,
				    this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_deactivate",
				    OBSBasic::SourceAudioDeactivated, this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_rename", OBSBasic::SourceRenamed, this);
	signalHandlers.emplace_back(
		obs_get_signal_handler(), "source_filter_add",
		[](void *data, calldata_t *) {
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "UpdateEditMenu",
						  Qt::QueuedConnection);
		},
		this);
	signalHandlers.emplace_back(
		obs_get_signal_handler(), "source_filter_remove",
		[](void *data, calldata_t *) {
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "UpdateEditMenu",
						  Qt::QueuedConnection);
		},
		this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "canvas_remove", OBSBasic::CanvasRemoved, this);
}

#define STARTUP_SEPARATOR "==== Startup complete ==============================================="
#define SHUTDOWN_SEPARATOR "==== Shutting down =================================================="

#define UNSUPPORTED_ERROR                                                     \
	"Failed to initialize video:\n\nRequired graphics API functionality " \
	"not found.  Your GPU may not be supported."

#define UNKNOWN_ERROR                                                  \
	"Failed to initialize video.  Your GPU may not be supported, " \
	"or your graphics drivers may need to be updated."

static inline void LogEncoders()
{
	constexpr uint32_t hide_flags = OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL;

	auto list_encoders = [](obs_encoder_type type) {
		size_t idx = 0;
		const char *encoder_type;

		while (obs_enum_encoder_types(idx++, &encoder_type)) {
			if (obs_get_encoder_caps(encoder_type) & hide_flags ||
			    obs_get_encoder_type(encoder_type) != type) {
				continue;
			}

			blog(LOG_INFO, "\t- %s (%s)", encoder_type, obs_encoder_get_display_name(encoder_type));
		}
	};

	blog(LOG_INFO, "---------------------------------");
	blog(LOG_INFO, "Available Encoders:");
	blog(LOG_INFO, "  Video Encoders:");
	list_encoders(OBS_ENCODER_VIDEO);
	blog(LOG_INFO, "  Audio Encoders:");
	list_encoders(OBS_ENCODER_AUDIO);
}

void OBSBasic::OBSInit()
{
	ProfileScope("OBSBasic::OBSInit");

	if (!InitBasicConfig())
		throw "Failed to load basic.ini";
	if (!ResetAudio())
		throw "Failed to initialize audio";

	int ret = 0;

	ret = ResetVideo();

	switch (ret) {
	case OBS_VIDEO_MODULE_NOT_FOUND:
		throw "Failed to initialize video:  Graphics module not found";
	case OBS_VIDEO_NOT_SUPPORTED:
		throw UNSUPPORTED_ERROR;
	case OBS_VIDEO_INVALID_PARAM:
		throw "Failed to initialize video:  Invalid parameters";
	default:
		if (ret != OBS_VIDEO_SUCCESS)
			throw UNKNOWN_ERROR;
	}

	/* load audio monitoring */
	if (obs_audio_monitoring_available()) {
		const char *device_name = config_get_string(activeConfiguration, "Audio", "MonitoringDeviceName");
		const char *device_id = config_get_string(activeConfiguration, "Audio", "MonitoringDeviceId");

		obs_set_audio_monitoring_device(device_name, device_id);

		blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s", device_name, device_id);
	}

	InitOBSCallbacks();
	InitHotkeys();
	ui->preview->Init();

	/* hack to prevent elgato from loading its own QtNetwork that it tries
	 * to ship with */
#if defined(_WIN32) && !defined(_DEBUG)
	LoadLibraryW(L"Qt6Network");
#endif
	struct obs_module_failure_info mfi;

	/* Safe Mode disables third-party plugins so we don't need to add earch
	 * paths outside the OBS bundle/installation. */
	if (safe_mode || disable_3p_plugins) {
		SetSafeModuleNames();
	} else {
		AddExtraModulePaths();
	}

	/* Modules can access frontend information (i.e. profile and scene collection data) during their initialization, and some modules (e.g. obs-websockets) are known to use the filesystem location of the current profile in their own code.
     
     Thus the profile and scene collection discovery needs to happen before any access to that information (but after intializing global settings) to ensure legacy code gets valid path information.
     */
	RefreshSceneCollections(true);

	blog(LOG_INFO, "---------------------------------");
	obs_load_all_modules2(&mfi);
	blog(LOG_INFO, "---------------------------------");
	obs_log_loaded_modules();
	blog(LOG_INFO, "---------------------------------");
	obs_post_load_modules();

	BPtr<char *> failed_modules = mfi.failed_modules;

#ifdef BROWSER_AVAILABLE
	cef = obs_browser_init_panel();
	cef_js_avail = cef && obs_browser_qcef_version() >= 3;
#endif

	vcamEnabled = (obs_get_output_flags(VIRTUAL_CAM_ID) & OBS_OUTPUT_VIDEO) != 0;
	if (vcamEnabled) {
		emit VirtualCamEnabled();
	}

	UpdateProfileEncoders();

	LogEncoders();

	blog(LOG_INFO, STARTUP_SEPARATOR);

	if (!InitService())
		throw "Failed to initialize service";

	ResetOutputs();
	CreateHotkeys();

	InitPrimitives();

	sceneDuplicationMode = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SceneDuplicationMode");
	swapScenesMode = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SwapScenesMode");
	editPropertiesMode = config_get_bool(App()->GetUserConfig(), "BasicWindow", "EditPropertiesMode");

	if (!opt_studio_mode) {
		SetPreviewProgramMode(config_get_bool(App()->GetUserConfig(), "BasicWindow", "PreviewProgramMode"));
	} else {
		SetPreviewProgramMode(true);
		opt_studio_mode = false;
	}

#define SET_VISIBILITY(name, control)                                                                \
	do {                                                                                         \
		if (config_has_user_value(App()->GetUserConfig(), "BasicWindow", name)) {            \
			bool visible = config_get_bool(App()->GetUserConfig(), "BasicWindow", name); \
			ui->control->setChecked(visible);                                            \
		}                                                                                    \
	} while (false)

	SET_VISIBILITY("ShowListboxToolbars", toggleListboxToolbars);
	SET_VISIBILITY("ShowStatusBar", toggleStatusBar);
#undef SET_VISIBILITY

	bool sourceIconsVisible = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ShowSourceIcons");
	ui->toggleSourceIcons->setChecked(sourceIconsVisible);

	bool contextVisible = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ShowContextToolbars");
	ui->toggleContextBar->setChecked(contextVisible);
	ui->contextContainer->setVisible(contextVisible);
	if (contextVisible)
		UpdateContextBar(true);
	UpdateEditMenu();

	{
		ProfileScope("OBSBasic::Load");
		const std::string sceneCollectionName{
			config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection")};
		std::optional<OBS::SceneCollection> configuredCollection =
			GetSceneCollectionByName(sceneCollectionName);
		std::optional<OBS::SceneCollection> foundCollection = GetSceneCollectionByName(opt_starting_collection);

		if (foundCollection) {
			ActivateSceneCollection(foundCollection.value());
		} else if (configuredCollection) {
			ActivateSceneCollection(configuredCollection.value());
		} else {
			disableSaving--;
			SetupNewSceneCollection(sceneCollectionName);
			disableSaving++;
		}

		if (foundCollection || configuredCollection) {
			disableSaving--;
			OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
			OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
			OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
			OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
			disableSaving++;
		}
	}

	// Load last active Horizontal and Vertical scenes
	const char *lastHSceneName = config_get_string(App()->GetUserConfig(), "BasicWindow", "CurrentHScene");
	if (lastHSceneName && *lastHSceneName) {
		obs_source_t *scene = obs_get_source_by_name(lastHSceneName);
		if (scene) {
			App()->SetCurrentHorizontalScene(scene);
			// If not dual active, or if horizontal pane is active, make this the UI selected one too
			if (!App()->IsDualOutputActive() || activePreviewPane == ActivePreview::HORIZONTAL) {
				SetCurrentScene(scene); // This updates UI selection and OBSBasic::currentScene
			}
			obs_source_release(scene);
		}
	}
	if (App()->IsDualOutputActive()) {
		const char *lastVSceneName = config_get_string(App()->GetUserConfig(), "BasicWindow", "CurrentVSceneName"); // Corrected key
		if (lastVSceneName && *lastVSceneName) { // Check if key exists and is not empty
			obs_source_t *scene = obs_get_source_by_name(lastVSceneName);
			if (scene) {
				App()->SetCurrentVerticalScene(scene);
				if (activePreviewPane == ActivePreview::VERTICAL) {
					SetCurrentScene(scene); // This updates UI selection
				}
				obs_source_release(scene);
			} else {
				blog(LOG_WARNING, "Failed to find saved vertical scene: %s", lastVSceneName);
			}
		}
	}
	// If no specific scenes were set (e.g. first run or cleared config), select first available in active pane
	RefreshSceneListDisplay(); // This will also handle initial selection if currentItem is null

	connect(App(), &OBSApp::horizontalSceneChanged, this, &OBSBasic::HandleHorizontalSceneChanged);
	connect(App(), &OBSApp::verticalSceneChanged, this, &OBSBasic::HandleVerticalSceneChanged);

	loaded = true;

	previewEnabled = config_get_bool(App()->GetUserConfig(), "BasicWindow", "PreviewEnabled");

	if (!previewEnabled && !IsPreviewProgramMode())
		QMetaObject::invokeMethod(this, "EnablePreviewDisplay", Qt::QueuedConnection,
					  Q_ARG(bool, previewEnabled));
	else if (!previewEnabled && IsPreviewProgramMode())
		QMetaObject::invokeMethod(this, "EnablePreviewDisplay", Qt::QueuedConnection, Q_ARG(bool, true));

	disableSaving--;

	auto addDisplayHorizontal = [this](OBSQTDisplay *window) { // Renamed lambda
		obs_display_add_draw_callback(window->GetDisplay(), OBSBasic::RenderHorizontalMain, this); // Changed to RenderHorizontalMain

		struct obs_video_info ovi;
		if (obs_get_video_info(&ovi)) // This ResizePreview might need to be specific if previews can have different base aspect ratios
			ResizePreview(ovi.base_width, ovi.base_height);
	};

	// Connect for mainPreview_h (renamed original ui->preview)
	// Note: ui->preview in the .ui file was renamed to mainPreview_h
	// The object name in OBSBasic.ui for the horizontal preview is 'preview'.
	// So we connect to ui->preview here.
	connect(ui->preview, &OBSQTDisplay::DisplayCreated, addDisplayHorizontal);


	// Setup for mainPreview_v
	if (ui->mainPreview_v) {
		auto addVerticalDisplay = [this](OBSQTDisplay *window) {
			obs_display_add_draw_callback(window->GetDisplay(), OBSBasic::RenderVerticalMain, this); // Changed to RenderVerticalMain

			// TODO: Set the source for the vertical display.
			// This depends on how OBSApp exposes the vertical video pipeline output.
			// Example:
			if (App()->IsDualOutputActive()) {
			//    obs_source_t *vertical_pipeline_source = App()->GetVerticalVideoSource(); // Placeholder
			//    if (vertical_pipeline_source) {
			//        obs_display_set_source(window->GetDisplay(), vertical_pipeline_source);
			//    } else {
			//        obs_display_set_source(window->GetDisplay(), nullptr);
			//    }
				// blog(LOG_INFO, "Dual Output Active: Vertical display created. TODO: Set obs_source for vertical display.");
				obs_display_set_source(window->GetDisplay(), App()->GetCurrentVerticalScene());
				blog(LOG_INFO, "Dual Output Active: Vertical display created and source set.");
			} else {
			   obs_display_set_source(window->GetDisplay(), nullptr); // Clear source if not dual output
			   window->setVisible(false); // Optionally hide it
			   blog(LOG_INFO, "Dual Output Inactive: Vertical display source cleared and hidden.");
			}

			// TODO: Potentially a different ResizePreview_V if vertical preview has different scaling logic or base dimensions
			const obs_video_info* v_ovi = App()->GetVerticalVideoInfo();
			if (v_ovi && v_ovi->base_width > 0) {
				ResizePreview_V(v_ovi->base_width, v_ovi->base_height);
				blog(LOG_INFO, "Vertical display created and resized based on App's vertical OVI.");
			} else {
				blog(LOG_WARNING, "Vertical display created, but vertical OVI not valid for resize.");
			}

		};
		connect(ui->mainPreview_v, &OBSQTDisplay::DisplayCreated, addVerticalDisplay);
		if (App()->IsDualOutputActive()) {
			ui->mainPreview_v->Init();
			ui->mainPreview_v->setVisible(true);
		} else {
			ui->mainPreview_v->setVisible(false); // Hide if not dual output mode
		}
	}


	/* Show the main window, unless the tray icon isn't available
	 * or neither the setting nor flag for starting minimized is set. */
	bool sysTrayEnabled = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayEnabled");
	bool sysTrayWhenStarted = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayWhenStarted");
	bool hideWindowOnStart = QSystemTrayIcon::isSystemTrayAvailable() && sysTrayEnabled &&
				 (opt_minimize_tray || sysTrayWhenStarted);

#ifdef _WIN32
	SetWin32DropStyle(this);

	if (!hideWindowOnStart)
		show();
#endif

	bool alwaysOnTop = config_get_bool(App()->GetUserConfig(), "BasicWindow", "AlwaysOnTop");

#ifdef ENABLE_WAYLAND
	bool isWayland = obs_get_nix_platform() == OBS_NIX_PLATFORM_WAYLAND;
#else
	bool isWayland = false;
#endif

	if (!isWayland && (alwaysOnTop || opt_always_on_top)) {
		SetAlwaysOnTop(this, true);
		ui->actionAlwaysOnTop->setChecked(true);
	} else if (isWayland) {
		if (opt_always_on_top)
			blog(LOG_INFO, "Always On Top not available on Wayland, ignoring.");
		ui->actionAlwaysOnTop->setEnabled(false);
		ui->actionAlwaysOnTop->setVisible(false);
	}

#ifndef _WIN32
	if (!hideWindowOnStart)
		show();
#endif

	/* setup stats dock */
	OBSBasicStats *statsDlg = new OBSBasicStats(statsDock, false);
	statsDock->setWidget(statsDlg);

	/* ----------------------------- */
	/* add custom browser docks      */
#if defined(BROWSER_AVAILABLE) && defined(YOUTUBE_ENABLED)
	YouTubeAppDock::CleanupYouTubeUrls();
#endif

#ifdef BROWSER_AVAILABLE
	if (cef) {
		QAction *action = new QAction(QTStr("Basic.MainMenu.Docks."
						    "CustomBrowserDocks"),
					      this);
		ui->menuDocks->insertAction(ui->scenesDock->toggleViewAction(), action);
		connect(action, &QAction::triggered, this, &OBSBasic::ManageExtraBrowserDocks);
		ui->menuDocks->insertSeparator(ui->scenesDock->toggleViewAction());

		LoadExtraBrowserDocks();
	}
#endif

#ifdef YOUTUBE_ENABLED
	/* setup YouTube app dock */
	if (YouTubeAppDock::IsYTServiceSelected())
		NewYouTubeAppDock();
#endif

	const char *dockStateStr = config_get_string(App()->GetUserConfig(), "BasicWindow", "DockState");

	if (!dockStateStr) {
		on_resetDocks_triggered(true);
	} else {
		QByteArray dockState = QByteArray::fromBase64(QByteArray(dockStateStr));
		if (!restoreState(dockState))
			on_resetDocks_triggered(true);
	}

	bool pre23Defaults = config_get_bool(App()->GetUserConfig(), "General", "Pre23Defaults");
	if (pre23Defaults) {
		bool resetDockLock23 = config_get_bool(App()->GetUserConfig(), "General", "ResetDockLock23");
		if (!resetDockLock23) {
			config_set_bool(App()->GetUserConfig(), "General", "ResetDockLock23", true);
			config_remove_value(App()->GetUserConfig(), "BasicWindow", "DocksLocked");
			config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
		}
	}

	bool docksLocked = config_get_bool(App()->GetUserConfig(), "BasicWindow", "DocksLocked");
	on_lockDocks_toggled(docksLocked);
	ui->lockDocks->blockSignals(true);
	ui->lockDocks->setChecked(docksLocked);
	ui->lockDocks->blockSignals(false);

	bool sideDocks = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SideDocks");
	on_sideDocks_toggled(sideDocks);
	ui->sideDocks->blockSignals(true);
	ui->sideDocks->setChecked(sideDocks);
	ui->sideDocks->blockSignals(false);

	SystemTray(true);

	TaskbarOverlayInit();

#ifdef __APPLE__
	disableColorSpaceConversion(this);
#endif

	bool has_last_version = config_has_user_value(App()->GetAppConfig(), "General", "LastVersion");
	bool first_run = config_get_bool(App()->GetUserConfig(), "General", "FirstRun");

	if (!first_run) {
		config_set_bool(App()->GetUserConfig(), "General", "FirstRun", true);
		config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
	}

	if (!first_run && !has_last_version && !Active())
		QMetaObject::invokeMethod(this, "on_autoConfigure_triggered", Qt::QueuedConnection);

#if (defined(_WIN32) || defined(__APPLE__)) && (OBS_RELEASE_CANDIDATE > 0 || OBS_BETA > 0)
	/* Automatically set branch to "beta" the first time a pre-release build is run. */
	if (!config_get_bool(App()->GetAppConfig(), "General", "AutoBetaOptIn")) {
		config_set_string(App()->GetAppConfig(), "General", "UpdateBranch", "beta");
		config_set_bool(App()->GetAppConfig(), "General", "AutoBetaOptIn", true);
		config_save_safe(App()->GetAppConfig(), "tmp", nullptr);
	}
#endif
	TimedCheckForUpdates();

	ToggleMixerLayout(config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl"));

	if (config_get_bool(activeConfiguration, "General", "OpenStatsOnStartup"))
		on_stats_triggered();

	OBSBasicStats::InitializeValues();

	/* ----------------------- */
	/* Add multiview menu      */

	ui->viewMenu->addSeparator();

	connect(ui->viewMenu->menuAction(), &QAction::hovered, this, &OBSBasic::updateMultiviewProjectorMenu);
	OBSBasic::updateMultiviewProjectorMenu();

	ui->sources->UpdateIcons();

#if !defined(_WIN32)
	delete ui->actionShowCrashLogs;
	delete ui->actionUploadLastCrashLog;
	delete ui->menuCrashLogs;
	delete ui->actionRepair;
	ui->actionShowCrashLogs = nullptr;
	ui->actionUploadLastCrashLog = nullptr;
	ui->menuCrashLogs = nullptr;
	ui->actionRepair = nullptr;
#if !defined(__APPLE__)
	delete ui->actionCheckForUpdates;
	ui->actionCheckForUpdates = nullptr;
#endif
#endif

#ifdef __APPLE__
	/* Remove OBS' Fullscreen Interface menu in favor of the one macOS adds by default */
	delete ui->actionFullscreenInterface;
	ui->actionFullscreenInterface = nullptr;
#else
	/* Don't show menu to raise macOS-only permissions dialog */
	delete ui->actionShowMacPermissions;
	ui->actionShowMacPermissions = nullptr;
#endif

#if defined(_WIN32) || defined(__APPLE__)
	if (App()->IsUpdaterDisabled()) {
		ui->actionCheckForUpdates->setEnabled(false);
#if defined(_WIN32)
		ui->actionRepair->setEnabled(false);
#endif
	}
#endif

#ifndef WHATSNEW_ENABLED
	delete ui->actionShowWhatsNew;
	ui->actionShowWhatsNew = nullptr;
#endif

	if (safe_mode) {
		ui->actionRestartSafe->setText(QTStr("Basic.MainMenu.Help.RestartNormal"));
	}

	UpdatePreviewProgramIndicators();
	OnFirstLoad();

	if (!hideWindowOnStart)
		activateWindow();

	// TODO: Connect ui->mainPreview_h and ui->mainPreview_v to SetHorizontalPreviewActive / SetVerticalPreviewActive
	// This might require installing an event filter on these widgets to capture mouse press or focus events.
	// For now, let's assume direct connection or will be handled by event filter later.
	// Example: connect(ui->mainPreview_h, &SomeSignalEmitter::clicked, this, &OBSBasic::SetHorizontalPreviewActive);
	// Example: connect(ui->mainPreview_v, &SomeSignalEmitter::clicked, this, &OBSBasic::SetVerticalPreviewActive);


	/* ------------------------------------------- */
	/* display warning message for failed modules  */

	if (mfi.count) {
		QString failed_plugins;

		char **plugin = mfi.failed_modules;
		while (*plugin) {
			failed_plugins += *plugin;
			failed_plugins += "\n";
			plugin++;
		}

		QString failed_msg = QTStr("PluginsFailedToLoad.Text").arg(failed_plugins);
		OBSMessageBox::warning(this, QTStr("PluginsFailedToLoad.Title"), failed_msg);
	}
}

void OBSBasic::OnFirstLoad()
{
	OnEvent(OBS_FRONTEND_EVENT_FINISHED_LOADING);

#ifdef WHATSNEW_ENABLED
	/* Attempt to load init screen if available */
	if (cef) {
		WhatsNewInfoThread *wnit = new WhatsNewInfoThread();
		connect(wnit, &WhatsNewInfoThread::Result, this, &OBSBasic::ReceivedIntroJson, Qt::QueuedConnection);

		introCheckThread.reset(wnit);
		introCheckThread->start();
	}
#endif

	Auth::Load();

	bool showLogViewerOnStartup = config_get_bool(App()->GetUserConfig(), "LogViewer", "ShowLogStartup");

	if (showLogViewerOnStartup)
		on_actionViewCurrentLog_triggered();
}

OBSBasic::~OBSBasic()
{
	/* clear out UI event queue */
	QApplication::sendPostedEvents(nullptr);
	QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

	if (updateCheckThread && updateCheckThread->isRunning())
		updateCheckThread->wait();

	if (patronJsonThread && patronJsonThread->isRunning())
		patronJsonThread->wait();

	delete screenshotData;
	delete previewProjector;
	delete studioProgramProjector;
	delete previewProjectorSource;
	delete previewProjectorMain;
	delete sourceProjector;
	delete sceneProjectorMenu;
	delete scaleFilteringMenu;
	delete blendingModeMenu;
	delete colorMenu;
	delete colorWidgetAction;
	delete colorSelect;
	delete deinterlaceMenu;
	delete perSceneTransitionMenu;
	delete shortcutFilter;
	delete trayMenu;
	delete programOptions;
	delete program;

	/* XXX: any obs data must be released before calling obs_shutdown.
	 * currently, we can't automate this with C++ RAII because of the
	 * delicate nature of obs_shutdown needing to be freed before the UI
	 * can be freed, and we have no control over the destruction order of
	 * the Qt UI stuff, so we have to manually clear any references to
	 * libobs. */
	delete cpuUsageTimer;
	os_cpu_usage_info_destroy(cpuUsageInfo);

	obs_hotkey_set_callback_routing_func(nullptr, nullptr);
	ClearHotkeys();

	service = nullptr;
	outputHandler.reset();

	delete interaction;
	delete properties;
	delete filters;
	delete transformWindow;
	delete advAudioWindow;
	delete about;
	delete remux;

	obs_display_remove_draw_callback(ui->preview->GetDisplay(), OBSBasic::RenderMain, this);

	obs_enter_graphics();
	gs_vertexbuffer_destroy(box);
	gs_vertexbuffer_destroy(boxLeft);
	gs_vertexbuffer_destroy(boxTop);
	gs_vertexbuffer_destroy(boxRight);
	gs_vertexbuffer_destroy(boxBottom);
	gs_vertexbuffer_destroy(circle);
	gs_vertexbuffer_destroy(actionSafeMargin);
	gs_vertexbuffer_destroy(graphicsSafeMargin);
	gs_vertexbuffer_destroy(fourByThreeSafeMargin);
	gs_vertexbuffer_destroy(leftLine);
	gs_vertexbuffer_destroy(topLine);
	gs_vertexbuffer_destroy(rightLine);
	obs_leave_graphics();

	/* When shutting down, sometimes source references can get in to the
	 * event queue, and if we don't forcibly process those events they
	 * won't get processed until after obs_shutdown has been called.  I
	 * really wish there were a more elegant way to deal with this via C++,
	 * but Qt doesn't use C++ in a normal way, so you can't really rely on
	 * normal C++ behavior for your data to be freed in the order that you
	 * expect or want it to. */
	QApplication::sendPostedEvents(nullptr);

	config_set_int(App()->GetAppConfig(), "General", "LastVersion", LIBOBS_API_VER);
	config_save_safe(App()->GetAppConfig(), "tmp", nullptr);

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "PreviewEnabled", previewEnabled);
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "AlwaysOnTop", ui->actionAlwaysOnTop->isChecked());
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "SceneDuplicationMode", sceneDuplicationMode);
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "SwapScenesMode", swapScenesMode);
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "EditPropertiesMode", editPropertiesMode);
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "PreviewProgramMode", IsPreviewProgramMode());
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "DocksLocked", ui->lockDocks->isChecked());
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "SideDocks", ui->sideDocks->isChecked());
	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);

#ifdef BROWSER_AVAILABLE
	DestroyPanelCookieManager();
	delete cef;
	cef = nullptr;
#endif
}

static inline int AttemptToResetVideo(struct obs_video_info *ovi)
{
	return obs_reset_video(ovi);
}

static inline enum obs_scale_type GetScaleType(ConfigFile &activeConfiguration)
{
	const char *scaleTypeStr = config_get_string(activeConfiguration, "Video", "ScaleType");

	if (astrcmpi(scaleTypeStr, "bilinear") == 0)
		return OBS_SCALE_BILINEAR;
	else if (astrcmpi(scaleTypeStr, "lanczos") == 0)
		return OBS_SCALE_LANCZOS;
	else if (astrcmpi(scaleTypeStr, "area") == 0)
		return OBS_SCALE_AREA;
	else
		return OBS_SCALE_BICUBIC;
}

static inline enum video_format GetVideoFormatFromName(const char *name)
{
	if (astrcmpi(name, "I420") == 0)
		return VIDEO_FORMAT_I420;
	else if (astrcmpi(name, "NV12") == 0)
		return VIDEO_FORMAT_NV12;
	else if (astrcmpi(name, "I444") == 0)
		return VIDEO_FORMAT_I444;
	else if (astrcmpi(name, "I010") == 0)
		return VIDEO_FORMAT_I010;
	else if (astrcmpi(name, "P010") == 0)
		return VIDEO_FORMAT_P010;
	else if (astrcmpi(name, "P216") == 0)
		return VIDEO_FORMAT_P216;
	else if (astrcmpi(name, "P416") == 0)
		return VIDEO_FORMAT_P416;
#if 0 //currently unsupported
	else if (astrcmpi(name, "YVYU") == 0)
		return VIDEO_FORMAT_YVYU;
	else if (astrcmpi(name, "YUY2") == 0)
		return VIDEO_FORMAT_YUY2;
	else if (astrcmpi(name, "UYVY") == 0)
		return VIDEO_FORMAT_UYVY;
#endif
	else
		return VIDEO_FORMAT_BGRA;
}

static inline enum video_colorspace GetVideoColorSpaceFromName(const char *name)
{
	enum video_colorspace colorspace = VIDEO_CS_SRGB;
	if (strcmp(name, "601") == 0)
		colorspace = VIDEO_CS_601;
	else if (strcmp(name, "709") == 0)
		colorspace = VIDEO_CS_709;
	else if (strcmp(name, "2100PQ") == 0)
		colorspace = VIDEO_CS_2100_PQ;
	else if (strcmp(name, "2100HLG") == 0)
		colorspace = VIDEO_CS_2100_HLG;

	return colorspace;
}

int OBSBasic::ResetVideo()
{
	if (outputHandler && outputHandler->Active())
		return OBS_VIDEO_CURRENTLY_ACTIVE;

	ProfileScope("OBSBasic::ResetVideo");

	obs_video_info ovi_to_reset; // This will be the one passed to obs_reset_video
	int ret;

	// Always get horizontal settings first, as it's the primary for obs_reset_video
	const obs_video_info* h_ovi_ptr = App()->GetHorizontalVideoInfo();
	if (!h_ovi_ptr || h_ovi_ptr->base_width == 0) { // Check if it's initialized
		// Fallback to reading directly from config if App's version isn't ready
		// This path might be taken on initial startup before settings dialog is first saved.
		blog(LOG_INFO, "Horizontal OVI in App is not ready, reading from current profile config for ResetVideo.");
		struct obs_video_info temp_h_ovi;
		GetConfigFPS(temp_h_ovi.fps_num, temp_h_ovi.fps_den); // Reads from activeConfiguration (profile)
		const char *colorFormat = config_get_string(activeConfiguration, "Video", "ColorFormat");
		const char *colorSpace = config_get_string(activeConfiguration, "Video", "ColorSpace");
		const char *colorRange = config_get_string(activeConfiguration, "Video", "ColorRange");

		temp_h_ovi.graphics_module = App()->GetRenderModule();
		temp_h_ovi.base_width = (uint32_t)config_get_uint(activeConfiguration, "Video", "BaseCX");
		temp_h_ovi.base_height = (uint32_t)config_get_uint(activeConfiguration, "Video", "BaseCY");
		temp_h_ovi.output_width = (uint32_t)config_get_uint(activeConfiguration, "Video", "OutputCX");
		temp_h_ovi.output_height = (uint32_t)config_get_uint(activeConfiguration, "Video", "OutputCY");
		temp_h_ovi.output_format = GetVideoFormatFromName(colorFormat);
		temp_h_ovi.colorspace = GetVideoColorSpaceFromName(colorSpace);
		temp_h_ovi.range = astrcmpi(colorRange, "Full") == 0 ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
		temp_h_ovi.adapter = config_get_uint(App()->GetUserConfig(), "Video", "AdapterIdx");
		temp_h_ovi.gpu_conversion = true;
		temp_h_ovi.scale_type = GetScaleType(activeConfiguration); // Reads from activeConfiguration
		ovi_to_reset = temp_h_ovi;
	} else {
		ovi_to_reset = *h_ovi_ptr;
	}

	// Validate and clamp resolutions for the primary reset
	if (ovi_to_reset.base_width < 32 || ovi_to_reset.base_height < 32) {
		ovi_to_reset.base_width = 1920;
		ovi_to_reset.base_height = 1080;
		// Also update config if this was a fallback read, though ideally settings dialog handles this.
	}
	if (ovi_to_reset.output_width < 32 || ovi_to_reset.output_height < 32) {
		ovi_to_reset.output_width = ovi_to_reset.base_width;
		ovi_to_reset.output_height = ovi_to_reset.base_height;
	}

	blog(LOG_INFO, "ResetVideo: Using Horizontal/Primary OVI: %dx%d -> %dx%d @ %d/%d fps",
		ovi_to_reset.base_width, ovi_to_reset.base_height,
		ovi_to_reset.output_width, ovi_to_reset.output_height,
		ovi_to_reset.fps_num, ovi_to_reset.fps_den);

	ret = AttemptToResetVideo(&ovi_to_reset);
	if (ret == OBS_VIDEO_CURRENTLY_ACTIVE) {
		blog(LOG_WARNING, "Tried to reset video when already active");
		return ret;
	}

	if (ret == OBS_VIDEO_SUCCESS) {
		ResizePreview(ovi_to_reset.base_width, ovi_to_reset.base_height); // Resizes horizontal preview
		if (program) // Handle studio mode program view
			ResizeProgram(ovi_to_reset.base_width, ovi_to_reset.base_height);

		const float sdr_white_level = (float)config_get_uint(activeConfiguration, "Video", "SdrWhiteLevel");
		const float hdr_nominal_peak_level =
			(float)config_get_uint(activeConfiguration, "Video", "HdrNominalPeakLevel");
		obs_set_video_levels(sdr_white_level, hdr_nominal_peak_level);

		OBSBasicStats::InitializeValues();
		OBSProjector::UpdateMultiviewProjectors();

		if (!collections.empty()) {
			const OBS::SceneCollection currentSceneCollection = OBSBasic::GetCurrentSceneCollection();
			bool usingAbsoluteCoordinates = currentSceneCollection.getCoordinateMode() == OBS::SceneCoordinateMode::Absolute;
			OBS::Rect migrationResolution = currentSceneCollection.getMigrationResolution();
			OBS::Rect videoResolution = OBS::Rect(ovi_to_reset.base_width, ovi_to_reset.base_height);
			bool canMigrate = usingAbsoluteCoordinates || (!migrationResolution.isZero() && migrationResolution != videoResolution);
			ui->actionRemigrateSceneCollection->setEnabled(canMigrate);
		} else {
			ui->actionRemigrateSceneCollection->setEnabled(false);
		}

		emit CanvasResized(ovi_to_reset.base_width, ovi_to_reset.base_height);       // For horizontal
		emit OutputResized(ovi_to_reset.output_width, ovi_to_reset.output_height); // For horizontal

		// Handle vertical pipeline/preview visibility and setup
		if (App()->IsDualOutputActive()) {
			obs_video_info v_ovi;
			// Read V_ prefixed settings from activeConfiguration (profile)
			// Similar to how GetConfigFPS works, create a GetConfigFPS_V or adapt
			// For now, direct read, assuming V_FPSType etc. exist from defaults
			uint32_t v_fps_type = config_get_uint(activeConfiguration, "Video", "V_FPSType");
			if (v_fps_type == 1) { // Integer
				v_ovi.fps_num = (uint32_t)config_get_uint(activeConfiguration, "Video", "V_FPSInt");
				v_ovi.fps_den = 1;
			} else if (v_fps_type == 2) { // Fraction
				v_ovi.fps_num = (uint32_t)config_get_uint(activeConfiguration, "Video", "V_FPSNum");
				v_ovi.fps_den = (uint32_t)config_get_uint(activeConfiguration, "Video", "V_FPSDen");
			} else { // Common
				const char *val = config_get_string(activeConfiguration, "Video", "V_FPSCommon");
				// Simplified version of GetFPSCommon, adapt as needed
				if (strcmp(val, "60") == 0) { v_ovi.fps_num = 60; v_ovi.fps_den = 1; }
				// ... other common FPS values for vertical ...
				else { v_ovi.fps_num = 30; v_ovi.fps_den = 1; } // Default common
			}

			const char *v_colorFormat = config_get_string(activeConfiguration, "Video", "V_ColorFormat");
			const char *v_colorSpace = config_get_string(activeConfiguration, "Video", "V_ColorSpace");
			const char *v_colorRange = config_get_string(activeConfiguration, "Video", "V_ColorRange");
			const char *v_scaleType = config_get_string(activeConfiguration, "Video", "V_ScaleType");


			v_ovi.graphics_module = App()->GetRenderModule(); // Should be same as horizontal
			v_ovi.base_width = (uint32_t)config_get_uint(activeConfiguration, "Video", "V_BaseCX");
			v_ovi.base_height = (uint32_t)config_get_uint(activeConfiguration, "Video", "V_BaseCY");
			v_ovi.output_width = (uint32_t)config_get_uint(activeConfiguration, "Video", "V_OutputCX");
			v_ovi.output_height = (uint32_t)config_get_uint(activeConfiguration, "Video", "V_OutputCY");
			v_ovi.output_format = GetVideoFormatFromName(v_colorFormat);
			v_ovi.colorspace = GetVideoColorSpaceFromName(v_colorSpace);
			v_ovi.range = astrcmpi(v_colorRange, "Full") == 0 ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
			v_ovi.adapter = ovi_to_reset.adapter; // Assuming same adapter
			v_ovi.gpu_conversion = true;
			if (astrcmpi(v_scaleType, "bilinear") == 0) v_ovi.scale_type = OBS_SCALE_BILINEAR;
			else if (astrcmpi(v_scaleType, "lanczos") == 0) v_ovi.scale_type = OBS_SCALE_LANCZOS;
			else if (astrcmpi(v_scaleType, "area") == 0) v_ovi.scale_type = OBS_SCALE_AREA;
			else v_ovi.scale_type = OBS_SCALE_BICUBIC; // Default bicubic

			App()->UpdateVerticalVideoInfo(v_ovi); // This will also save it back if changed by UI later

			// Now that App's vertical_ovi is updated, use it for UI
			const obs_video_info* v_ovi_ptr_updated = App()->GetVerticalVideoInfo();
			if (v_ovi_ptr_updated && v_ovi_ptr_updated->base_width > 0) {
				blog(LOG_INFO, "Dual Output Active: Vertical OVI updated to: %dx%d -> %dx%d @ %d/%d fps",
					v_ovi_ptr_updated->base_width, v_ovi_ptr_updated->base_height,
					v_ovi_ptr_updated->output_width, v_ovi_ptr_updated->output_height,
					v_ovi_ptr_updated->fps_num, v_ovi_ptr_updated->fps_den);

				if (ui->mainPreview_v) {
					ui->mainPreview_v->setVisible(true);
					ResizePreview_V(v_ovi_ptr_updated->base_width, v_ovi_ptr_updated->base_height);
					// TODO: Ensure vertical display source is set correctly after video reset.
					// This might involve App()->SetCurrentVerticalScene(App()->GetCurrentVerticalScene())
					// to re-trigger source setting on the output if it was recreated.
					blog(LOG_INFO, "Vertical preview resized and made visible.");
				}
			} else {
				blog(LOG_WARNING, "Dual Output Active, but vertical video info is not properly initialized after update.");
				if (ui->mainPreview_v) ui->mainPreview_v->setVisible(false);
			}
		} else { // Dual output not active
			if (ui->mainPreview_v) {
				ui->mainPreview_v->setVisible(false);
				obs_display_set_source(ui->mainPreview_v->GetDisplay(), nullptr); // Clear source
			}
		}
	}

	return ret;
}

bool OBSBasic::ResetAudio()
{
	ProfileScope("OBSBasic::ResetAudio");

	struct obs_audio_info2 ai = {};
	ai.samples_per_sec = config_get_uint(activeConfiguration, "Audio", "SampleRate");

	const char *channelSetupStr = config_get_string(activeConfiguration, "Audio", "ChannelSetup");

	if (strcmp(channelSetupStr, "Mono") == 0)
		ai.speakers = SPEAKERS_MONO;
	else if (strcmp(channelSetupStr, "2.1") == 0)
		ai.speakers = SPEAKERS_2POINT1;
	else if (strcmp(channelSetupStr, "4.0") == 0)
		ai.speakers = SPEAKERS_4POINT0;
	else if (strcmp(channelSetupStr, "4.1") == 0)
		ai.speakers = SPEAKERS_4POINT1;
	else if (strcmp(channelSetupStr, "5.1") == 0)
		ai.speakers = SPEAKERS_5POINT1;
	else if (strcmp(channelSetupStr, "7.1") == 0)
		ai.speakers = SPEAKERS_7POINT1;
	else
		ai.speakers = SPEAKERS_STEREO;

	bool lowLatencyAudioBuffering = config_get_bool(App()->GetUserConfig(), "Audio", "LowLatencyAudioBuffering");
	if (lowLatencyAudioBuffering) {
		ai.max_buffering_ms = 20;
		ai.fixed_buffering = true;
	}

	return obs_reset_audio2(&ai);
}

void OBSBasic::closeEvent(QCloseEvent *event)
{
	/* Wait for multitrack video stream to start/finish processing in the background */
	if (setupStreamingGuard.valid() &&
	    setupStreamingGuard.wait_for(std::chrono::seconds{0}) != std::future_status::ready) {
		QTimer::singleShot(1000, this, &OBSBasic::close);
		event->ignore();
		return;
	}

	/* Do not close window if inside of a temporary event loop because we
	 * could be inside of an Auth::LoadUI call.  Keep trying once per
	 * second until we've exit any known sub-loops. */
	if (os_atomic_load_long(&insideEventLoop) != 0) {
		QTimer::singleShot(1000, this, &OBSBasic::close);
		event->ignore();
		return;
	}

#ifdef YOUTUBE_ENABLED
	/* Also don't close the window if the youtube stream check is active */
	if (youtubeStreamCheckThread) {
		QTimer::singleShot(1000, this, &OBSBasic::close);
		event->ignore();
		return;
	}
#endif

	if (isVisible())
		config_set_string(App()->GetUserConfig(), "BasicWindow", "geometry",
				  saveGeometry().toBase64().constData());

	bool confirmOnExit = config_get_bool(App()->GetUserConfig(), "General", "ConfirmOnExit");

	if (confirmOnExit && outputHandler && outputHandler->Active() && !clearingFailed) {
		SetShowing(true);

		QMessageBox::StandardButton button =
			OBSMessageBox::question(this, QTStr("ConfirmExit.Title"), QTStr("ConfirmExit.Text"));

		if (button == QMessageBox::No) {
			event->ignore();
			restart = false;
			return;
		}
	}

	if (remux && !remux->close()) {
		event->ignore();
		restart = false;
		return;
	}

	QWidget::closeEvent(event);
	if (!event->isAccepted())
		return;

	blog(LOG_INFO, SHUTDOWN_SEPARATOR);

	closing = true;

	/* While closing, a resize event to OBSQTDisplay could be triggered.
	 * The graphics thread on macOS dispatches a lambda function to be
	 * executed asynchronously in the main thread. However, the display is
	 * sometimes deleted before the lambda function is actually executed.
	 * To avoid such a case, destroy displays earlier than others such as
	 * deleting browser docks. */
	ui->preview->DestroyDisplay();
	if (program)
		program->DestroyDisplay();

	if (outputHandler->VirtualCamActive())
		outputHandler->StopVirtualCam();

	if (introCheckThread)
		introCheckThread->wait();
	if (whatsNewInitThread)
		whatsNewInitThread->wait();
	if (updateCheckThread)
		updateCheckThread->wait();
	if (logUploadThread)
		logUploadThread->wait();
	if (devicePropertiesThread && devicePropertiesThread->isRunning()) {
		devicePropertiesThread->wait();
		devicePropertiesThread.reset();
	}

	QApplication::sendPostedEvents(nullptr);

	signalHandlers.clear();

	Auth::Save();
	SaveProjectNow();
	auth.reset();

	delete extraBrowsers;

	config_set_string(App()->GetUserConfig(), "BasicWindow", "DockState", saveState().toBase64().constData());

	// Save ActivePreviewPane state
	config_set_int(App()->GetUserConfig(), "BasicWindow", "ActivePreviewPane", static_cast<int>(activePreviewPane));

	// Save current horizontal and vertical scene names
	obs_source_t *h_scene = App()->GetCurrentHorizontalScene();
	if (h_scene) {
		config_set_string(App()->GetUserConfig(), "BasicWindow", "CurrentHScene", obs_source_get_name(h_scene));
	} else {
		config_remove_value(App()->GetUserConfig(), "BasicWindow", "CurrentHScene");
	}

	obs_source_t *v_scene = App()->GetCurrentVerticalScene();
	if (v_scene && App()->IsDualOutputActive()) { // Only save VScene if dual output is active
		config_set_string(App()->GetUserConfig(), "BasicWindow", "CurrentVSceneName", obs_source_get_name(v_scene));
	} else {
		config_remove_value(App()->GetUserConfig(), "BasicWindow", "CurrentVSceneName");
	}

	// Save splitter state
	if (ui->previewSplitter) {
		QByteArray splitterState = ui->previewSplitter->saveState();
		config_set_string(App()->GetUserConfig(), "BasicWindow", "PreviewSplitterState", splitterState.toBase64().constData());
	}


#ifdef BROWSER_AVAILABLE
	if (cef)
		SaveExtraBrowserDocks();

	ClearExtraBrowserDocks();
#endif

	OnEvent(OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN);

	disableSaving++;

	/* Clear all scene data (dialogs, widgets, widget sub-items, scenes,
	 * sources, etc) so that all references are released before shutdown */
	ClearSceneData();

	OnEvent(OBS_FRONTEND_EVENT_EXIT);

	// Destroys the frontend API so plugins can't continue calling it
	obs_frontend_set_callbacks_internal(nullptr);
	api = nullptr;

	QMetaObject::invokeMethod(App(), "quit", Qt::QueuedConnection);
}

bool OBSBasic::nativeEvent(const QByteArray &, void *message, qintptr *)
{
#ifdef _WIN32
	const MSG &msg = *static_cast<MSG *>(message);
	switch (msg.message) {
	case WM_MOVE:
		for (OBSQTDisplay *const display : findChildren<OBSQTDisplay *>()) {
			display->OnMove();
		}
		break;
	case WM_DISPLAYCHANGE:
		for (OBSQTDisplay *const display : findChildren<OBSQTDisplay *>()) {
			display->OnDisplayChange();
		}
	}
#else
	UNUSED_PARAMETER(message);
#endif

	return false;
}

void OBSBasic::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::WindowStateChange) {
		QWindowStateChangeEvent *stateEvent = (QWindowStateChangeEvent *)event;

		if (isMinimized()) {
			if (trayIcon && trayIcon->isVisible() && sysTrayMinimizeToTray()) {
				ToggleShowHide();
				return;
			}

			if (previewEnabled)
				EnablePreviewDisplay(false);
		} else if (stateEvent->oldState() & Qt::WindowMinimized && isVisible()) {
			if (previewEnabled)
				EnablePreviewDisplay(true);
		}
	}
}

void OBSBasic::GetFPSCommon(uint32_t &num, uint32_t &den) const
{
	const char *val = config_get_string(activeConfiguration, "Video", "FPSCommon");

	if (strcmp(val, "10") == 0) {
		num = 10;
		den = 1;
	} else if (strcmp(val, "20") == 0) {
		num = 20;
		den = 1;
	} else if (strcmp(val, "24 NTSC") == 0) {
		num = 24000;
		den = 1001;
	} else if (strcmp(val, "25 PAL") == 0) {
		num = 25;
		den = 1;
	} else if (strcmp(val, "29.97") == 0) {
		num = 30000;
		den = 1001;
	} else if (strcmp(val, "48") == 0) {
		num = 48;
		den = 1;
	} else if (strcmp(val, "50 PAL") == 0) {
		num = 50;
		den = 1;
	} else if (strcmp(val, "59.94") == 0) {
		num = 60000;
		den = 1001;
	} else if (strcmp(val, "60") == 0) {
		num = 60;
		den = 1;
	} else {
		num = 30;
		den = 1;
	}
}

void OBSBasic::GetFPSInteger(uint32_t &num, uint32_t &den) const
{
	num = (uint32_t)config_get_uint(activeConfiguration, "Video", "FPSInt");
	den = 1;
}

void OBSBasic::GetFPSFraction(uint32_t &num, uint32_t &den) const
{
	num = (uint32_t)config_get_uint(activeConfiguration, "Video", "FPSNum");
	den = (uint32_t)config_get_uint(activeConfiguration, "Video", "FPSDen");
}

void OBSBasic::GetFPSNanoseconds(uint32_t &num, uint32_t &den) const
{
	num = 1000000000;
	den = (uint32_t)config_get_uint(activeConfiguration, "Video", "FPSNS");
}

void OBSBasic::GetConfigFPS(uint32_t &num, uint32_t &den) const
{
	uint32_t type = config_get_uint(activeConfiguration, "Video", "FPSType");

	if (type == 1) //"Integer"
		GetFPSInteger(num, den);
	else if (type == 2) //"Fraction"
		GetFPSFraction(num, den);
	/*
	 * 	else if (false) //"Nanoseconds", currently not implemented
	 *		GetFPSNanoseconds(num, den);
	 */
	else
		GetFPSCommon(num, den);
}

config_t *OBSBasic::Config() const
{
	return activeConfiguration;
}

void OBSBasic::UpdateEditMenu()
{
	QModelIndexList items = GetAllSelectedSourceItems();
	int totalCount = items.count();
	size_t filter_count = 0;

	if (totalCount == 1) {
		OBSSceneItem sceneItem = ui->sources->Get(GetTopSelectedSourceItem());
		OBSSource source = obs_sceneitem_get_source(sceneItem);
		filter_count = obs_source_filter_count(source);
	}

	bool allowPastingDuplicate = !!clipboard.size();
	for (size_t i = clipboard.size(); i > 0; i--) {
		const size_t idx = i - 1;
		OBSWeakSource &weak = clipboard[idx].weak_source;
		if (obs_weak_source_expired(weak)) {
			clipboard.erase(clipboard.begin() + idx);
			continue;
		}
		OBSSourceAutoRelease strong = obs_weak_source_get_source(weak.Get());
		if (allowPastingDuplicate && obs_source_get_output_flags(strong) & OBS_SOURCE_DO_NOT_DUPLICATE)
			allowPastingDuplicate = false;
	}

	int videoCount = 0;
	bool canTransformMultiple = false;
	for (int i = 0; i < totalCount; i++) {
		OBSSceneItem item = ui->sources->Get(items.value(i).row());
		OBSSource source = obs_sceneitem_get_source(item);
		const uint32_t flags = obs_source_get_output_flags(source);
		const bool hasVideo = (flags & OBS_SOURCE_VIDEO) != 0;
		if (hasVideo && !obs_sceneitem_locked(item))
			canTransformMultiple = true;

		if (hasVideo)
			videoCount++;
	}
	const bool canTransformSingle = videoCount == 1 && totalCount == 1;

	OBSSceneItem curItem = GetCurrentSceneItem();
	bool locked = curItem && obs_sceneitem_locked(curItem);

	ui->actionCopySource->setEnabled(totalCount > 0);
	ui->actionEditTransform->setEnabled(canTransformSingle && !locked);
	ui->actionCopyTransform->setEnabled(canTransformSingle);
	ui->actionPasteTransform->setEnabled(canTransformMultiple && hasCopiedTransform && videoCount > 0);
	ui->actionCopyFilters->setEnabled(filter_count > 0);
	ui->actionPasteFilters->setEnabled(!obs_weak_source_expired(copyFiltersSource) && totalCount > 0);
	ui->actionPasteRef->setEnabled(!!clipboard.size());
	ui->actionPasteDup->setEnabled(allowPastingDuplicate);

	ui->actionMoveUp->setEnabled(totalCount > 0);
	ui->actionMoveDown->setEnabled(totalCount > 0);
	ui->actionMoveToTop->setEnabled(totalCount > 0);
	ui->actionMoveToBottom->setEnabled(totalCount > 0);

	ui->actionResetTransform->setEnabled(canTransformMultiple);
	ui->actionRotate90CW->setEnabled(canTransformMultiple);
	ui->actionRotate90CCW->setEnabled(canTransformMultiple);
	ui->actionRotate180->setEnabled(canTransformMultiple);
	ui->actionFlipHorizontal->setEnabled(canTransformMultiple);
	ui->actionFlipVertical->setEnabled(canTransformMultiple);
	ui->actionFitToScreen->setEnabled(canTransformMultiple);
	ui->actionStretchToScreen->setEnabled(canTransformMultiple);
	ui->actionCenterToScreen->setEnabled(canTransformMultiple);
	ui->actionVerticalCenter->setEnabled(canTransformMultiple);
	ui->actionHorizontalCenter->setEnabled(canTransformMultiple);
}

void OBSBasic::UpdateTitleBar()
{
	stringstream name;

	const char *profile = config_get_string(App()->GetUserConfig(), "Basic", "Profile");
	const char *sceneCollection = config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection");

	name << "OBS ";
	if (previewProgramMode)
		name << "Studio ";

	name << App()->GetVersionString(false);
	if (safe_mode)
		name << " (" << Str("TitleBar.SafeMode") << ")";
	if (App()->IsPortableMode())
		name << " - " << Str("TitleBar.PortableMode");

	name << " - " << Str("TitleBar.Profile") << ": " << profile;
	name << " - " << Str("TitleBar.Scenes") << ": " << sceneCollection;

	setWindowTitle(QT_UTF8(name.str().c_str()));
}

OBSBasic *OBSBasic::Get()
{
	return reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
}

void OBSBasic::UpdatePatronJson(const QString &text, const QString &error)
{
	if (!error.isEmpty())
		return;

	patronJson = QT_TO_UTF8(text);
}

void OBSBasic::SetDisplayAffinity(QWindow *window)
{
	if (!SetDisplayAffinitySupported())
		return;

	bool hideFromCapture = config_get_bool(App()->GetUserConfig(), "BasicWindow", "HideOBSWindowsFromCapture");

	// Don't hide projectors, those are designed to be visible / captured
	if (window->property("isOBSProjectorWindow") == true)
		return;

#ifdef _WIN32
	HWND hwnd = (HWND)window->winId();

	DWORD curAffinity;
	if (GetWindowDisplayAffinity(hwnd, &curAffinity)) {
		if (hideFromCapture && curAffinity != WDA_EXCLUDEFROMCAPTURE)
			SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
		else if (!hideFromCapture && curAffinity != WDA_NONE)
			SetWindowDisplayAffinity(hwnd, WDA_NONE);
	}

#else
	// TODO: Implement for other platforms if possible. Don't forget to
	// implement SetDisplayAffinitySupported too!
	UNUSED_PARAMETER(hideFromCapture);
#endif
}

void OBSBasic::OnEvent(enum obs_frontend_event event)
{
	if (api)
		api->on_event(event);
}

OBSPromptResult OBSBasic::PromptForName(const OBSPromptRequest &request, const OBSPromptCallback &callback)
{
	OBSPromptResult result;

	for (;;) {
		result.success = false;

		if (request.withOption && !request.optionPrompt.empty()) {
			result.optionValue = request.optionValue;

			result.success = NameDialog::AskForNameWithOption(
				this, request.title.c_str(), request.prompt.c_str(), result.promptValue,
				request.optionPrompt.c_str(), result.optionValue,
				(request.promptValue.empty() ? nullptr : request.promptValue.c_str()));

		} else {
			result.success = NameDialog::AskForName(
				this, request.title.c_str(), request.prompt.c_str(), result.promptValue,
				(request.promptValue.empty() ? nullptr : request.promptValue.c_str()));
		}

		if (!result.success) {
			break;
		}

		if (result.promptValue.empty()) {
			OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
			continue;
		}

		if (!callback(result)) {
			OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));
			continue;
		}

		break;
	}

	return result;
}

void OBSBasic::SetHorizontalPreviewActive()
{
	if (activePreviewPane == ActivePreview::HORIZONTAL && !App()->IsDualOutputActive()) // Only refresh if actually changed or dual output just enabled/disabled
		return;

	activePreviewPane = ActivePreview::HORIZONTAL;
	blog(LOG_INFO, "Horizontal preview activated");

	// TODO: Update UI styling if necessary (e.g., border highlight)
	// ui->mainPreview_h->setProperty("active", true);
	// ui->mainPreview_v->setProperty("active", false);
	// style()->unpolish(ui->mainPreview_h); style()->polish(ui->mainPreview_h);
	// style()->unpolish(ui->mainPreview_v); style()->polish(ui->mainPreview_v);


	// TODO: Refresh scene list for horizontal scenes
	RefreshSceneListDisplay();

	// TODO: Refresh source list for the currently selected horizontal scene
	// RefreshSources(GetCurrentScene()); // GetCurrentScene will also need to be context-aware
}

void OBSBasic::SetVerticalPreviewActive()
{
	if (!App()->IsDualOutputActive()) // Cannot activate vertical if dual output is off
		return;
	if (activePreviewPane == ActivePreview::VERTICAL)
		return;

	activePreviewPane = ActivePreview::VERTICAL;
	blog(LOG_INFO, "Vertical preview activated");

	// TODO: Update UI styling if necessary
	// ui->mainPreview_h->setProperty("active", false);
	// ui->mainPreview_v->setProperty("active", true);
	// style()->unpolish(ui->mainPreview_h); style()->polish(ui->mainPreview_h);
	// style()->unpolish(ui->mainPreview_v); style()->polish(ui->mainPreview_v);

	// TODO: Refresh scene list for vertical scenes
	RefreshSceneListDisplay();

	// TODO: Refresh source list for the currently selected vertical scene
	// RefreshSources(GetCurrentScene());
}

bool OBSBasic::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress) {
		if (obj == ui->mainPreview_h) {
			SetHorizontalPreviewActive();
		} else if (obj == ui->mainPreview_v) {
			SetVerticalPreviewActive();
		}
	}
	return QMainWindow::eventFilter(obj, event);
}

void OBSBasic::HandleHorizontalSceneChanged(obs_source_t *new_scene)
{
	blog(LOG_INFO, "HandleHorizontalSceneChanged: New scene: %s", new_scene ? obs_source_get_name(new_scene) : "(none)");
	if (activePreviewPane == ActivePreview::HORIZONTAL) {
		// If the main preview (horizontal) is active, update the scene list selection
		SetCurrentScene(new_scene, true); // true to force, as this is a direct update
	}
	// Potentially update other UI elements specific to horizontal output
}

void OBSBasic::HandleVerticalSceneChanged(obs_source_t *new_scene)
{
	blog(LOG_INFO, "HandleVerticalSceneChanged: New scene: %s", new_scene ? obs_source_get_name(new_scene) : "(none)");
	if (activePreviewPane == ActivePreview::VERTICAL) {
		// If the vertical preview is active, update the scene list selection
		SetCurrentScene(new_scene, true); // true to force
	}
	// Update the vertical preview display source
	if (ui->mainPreview_v && App()->IsDualOutputActive()) {
		obs_display_set_source(ui->mainPreview_v->GetDisplay(), new_scene);
	}
}
// Function to replace - STARTS
void OBSBasic::SetCurrentScene(obs_source_t *scene, bool force)
{
	if (!force && scene == currentScene)
		return;

	/* ------------------------------------------------------ */
	/* save previous scene                                    */

	OBSScene previousScene = currentScene;
	if (previousScene)
		obs_source_set_monitoring_type(previousScene, OBS_MONITORING_TYPE_NONE);

	/* ------------------------------------------------------ */
	/* activate new scene                                     */

	OBSScene nextScene = scene;
	if (nextScene) {
		obs_source_inc_showing(nextScene);
		obs_source_set_monitoring_type(nextScene, GetAudioMonitoringTypeFromSettings());
	}

	/* ------------------------------------------------------ */
	/* do the actual scene switch                             */

	if (previewProgramMode) {
		obs_frontend_set_current_preview_scene(nextScene);
		programScene = obs_frontend_get_current_program_scene();

		if (lastProgramScene && lastProgramScene != programScene)
			obs_source_set_monitoring_type(lastProgramScene, OBS_MONITORING_TYPE_NONE);

		lastProgramScene = programScene;

		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

	} else {
		obs_frontend_set_current_scene(nextScene);
	}

	/* ------------------------------------------------------ */

	RefreshSources(nextScene);
	UpdateContextBar();
	UpdateSourceListToolbar();

	if (loaded) {
		QListWidgetItem *item = ui->scenes->FindScene(nextScene);
		if (item) {
			ui->scenes->blockSignals(true);
			ui->scenes->setCurrentItem(item);
			ui->scenes->blockSignals(false);
		}
	}

	OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	if (previewProgramMode)
		OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
}
// Function to replace - ENDS

void OBSBasic::SetTransition(OBSSource transition)
{
	if (transition == current_transition)
		return;

	current_transition = transition;
	if (transition) {
		obs_source_set_monitoring_type(transition, GetAudioMonitoringTypeFromSettings());
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

void OBSBasic::OverrideTransition(OBSSource transition)
{
	if (transition == override_transition)
		return;

	override_transition = transition;
	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_OVERRIDE_CHANGED);
}

void OBSBasic::TransitionToScene(OBSScene scene, bool force)
{
	TransitionToScene(obs_scene_get_source(scene), force);
}

void OBSBasic::TransitionToScene(OBSSource scene, bool force, bool quickTransition, int quickDuration, bool black,
				 bool manual)
{
	if (!scene)
		return;

	OBSSource curScene = GetCurrentSceneSource();
	if (scene == curScene && !force)
		return;

	if (previewProgramMode && !manual) {
		if (scene == programScene) {
			if (quickTransition && quickDuration) {
				if (IsTransitionActive())
					return;

				OBSSource trans = GetCurrentTransition();
				obs_source_set_duration(trans, quickDuration);
				obs_transition_start(trans);
			}
			return;
		}

		SetCurrentScene(scene, true);
	} else {
		OBSSource trans = GetCurrentTransition();
		if (quickTransition && quickDuration) {
			if (IsTransitionActive())
				return;
			obs_source_set_duration(trans, quickDuration);
		}

		if (black)
			obs_transition_set_destination(trans, nullptr, OBS_TRANSITION_MODE_OVERRIDE);
		else
			obs_transition_set(trans, scene);
	}
}

void OBSBasic::AddTransition(const char *id)
{
	OBSSource transition = obs_source_create_private(id, nullptr, nullptr);
	if (!transition)
		return;

	if (!obs_source_is_transition(transition)) {
		OBSErrorBox(this, "Not a transition: %s", id);
		return;
	}

	if (safeModeTransitions.size()) {
		for (auto &t : safeModeTransitions) {
			if (t == transition)
				return;
		}
	}

	const char *name = obs_source_get_name(transition);
	ui->transitions->addItem(QT_UTF8(name), QVariant::fromValue(transition));
	ui->transitions->setCurrentIndex(ui->transitions->count() - 1);

	if (safeModeTransitions.size())
		safeModeTransitions.push_back(std::move(transition));
}

void OBSBasic::RenameTransition(OBSSource transition)
{
	int idx = ui->transitions->findData(QVariant::fromValue(transition));
	if (idx == -1)
		return;

	const char *name = obs_source_get_name(transition);
	ui->transitions->setItemText(idx, QT_UTF8(name));

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_RENAMED);
}

void OBSBasic::TransitionClicked()
{
	if (IsTransitionActive())
		return;

	if (previewProgramMode) {
		TransitionToScene(GetCurrentSceneSource(), true, false, 0, false, true);
	}
}

void OBSBasic::TransitionStopped()
{
	if (!loaded)
		return;

	if (previewProgramMode) {
		programScene = obs_frontend_get_current_program_scene();
		lastProgramScene = programScene;
		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

		OnEvent(OBS_FRONTEND_EVENT_PROGRAM_SCENE_CHANGED);
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
}

void OBSBasic::TransitionFullyStopped()
{
	if (!loaded)
		return;

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_FULLY_STOPPED);
}

void OBSBasic::AddQuickTransitionId(int id)
{
	if (quickTransitionIdCounter <= id)
		quickTransitionIdCounter = id + 1;
}

void OBSBasic::AddQuickTransition()
{
	QuickTransition qt;
	qt.id = quickTransitionIdCounter++;
	qt.name = QTStr("Basic.QuickTransition.DefaultName").arg(QString::number(qt.id));
	qt.duration = GetTransitionDuration();
	qt.hotkeys[0] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[1] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[2] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[3] = OBS_INVALID_HOTKEY_ID;
	qt.transition = current_transition;

	quickTransitions.push_back(qt);
	AddQuickTransitionHotkey(&quickTransitions.back());
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_ADDED);
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	obs_frontend_source_context context;
	context.id = qt->id;
	context.type = OBS_FRONTEND_SOURCE_CONTEXT_QUICK_TRANSITION;

	qt->hotkeys[0] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition1",
		QTStr("Basic.QuickTransition1").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[1] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition2",
		QTStr("Basic.QuickTransition2").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[2] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition3",
		QTStr("Basic.QuickTransition3").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[3] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition4",
		QTStr("Basic.QuickTransition4").arg(qt->name).toUtf8().constData(), context);
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	for (int i = 0; i < 4; i++) {
		obs_hotkey_unregister(qt->hotkeys[i]);
		qt->hotkeys[i] = OBS_INVALID_HOTKEY_ID;
	}
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		if (!s)
			continue;

		OBSDataAutoRelease data = obs_source_save(s);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data)
{
	if (!transitions)
		return;

	for (size_t i = 0; i < obs_data_array_count(transitions); i++) {
		OBSDataAutoRelease data = obs_data_array_item(transitions, i);
		OBSSource s = obs_load_source(data, cb, private_data);
		if (!s)
			continue;

		InitTransition(s);
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < quickTransitions.size(); i++) {
		OBSDataAutoRelease data = obs_data_create();
		quickTransitions[i].Save(data);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *arr)
{
	ClearQuickTransitions();
	if (!arr)
		return;

	for (size_t i = 0; i < obs_data_array_count(arr); i++) {
		OBSDataAutoRelease data = obs_data_array_item(arr, i);
		QuickTransition qt;
		qt.Load(data);
		AddQuickTransitionId(qt.id);
		AddQuickTransitionHotkey(&qt);
		quickTransitions.push_back(qt);
	}

	RefreshQuickTransitions();
}

QuickTransition *OBSBasic::GetQuickTransition(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return &quickTransitions[i];
	}

	return nullptr;
}

int OBSBasic::GetQuickTransitionIdx(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return (int)i;
	}

	return -1;
}

void OBSBasic::QuickTransitionClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::QuickTransitionChange()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int transIdx = action->property("idx").toInt();

	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->transition = ui->transitions->itemData(transIdx).value<OBSSource>();
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionChangeDuration(int value)
{
	QSpinBox *box = qobject_cast<QSpinBox *>(sender());
	if (!box)
		return;

	int id = box->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->duration = value;

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionRemoveClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int idx = GetQuickTransitionIdx(id);
	if (idx == -1)
		return;

	RemoveQuickTransitionHotkey(&quickTransitions[idx]);
	quickTransitions.erase(quickTransitions.begin() + idx);
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_REMOVED);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::TBarChanged(int value)
{
	float t = float(value) / float(ui->tBar->maximum());
	obs_transition_set_time(current_transition, t);
	tBarActive = true;
}

void OBSBasic::TBarReleased()
{
	tBarActive = false;
}

void OBSBasic::on_transitions_currentIndexChanged(int index)
{
	if (index == -1)
		return;

	OBSSource s = ui->transitions->itemData(index).value<OBSSource>();
	SetTransition(s);
}

void OBSBasic::on_transitionAdd_clicked()
{
	QMenu menu(this);
	obs_frontend_populate_transition_types(&menu, this, SLOT(AddTransition(const char *)));
	menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	int idx = ui->transitions->currentIndex();
	if (idx == -1)
		return;

	if (ui->transitions->count() == 1) {
		OBSErrorBox(this, "Can't remove the last transition!");
		return;
	}

	OBSSource s = ui->transitions->itemData(idx).value<OBSSource>();

	ui->transitions->removeItem(idx);

	for (auto &qt : quickTransitions) {
		if (qt.transition == s)
			qt.transition = ui->transitions->itemData(0).value<OBSSource>();
	}

	RefreshQuickTransitions();
	obs_source_remove(s);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource s = ui->transitions->currentData().value<OBSSource>();
	if (s)
		CreatePropertiesWindow(s);
}

void OBSBasic::on_transitionDuration_valueChanged()
{
	if (loading)
		return;

	obs_source_set_transition_duration(current_transition, ui->transitionDuration->value());

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED);
}

void OBSBasic::ShowTransitionProperties()
{
	ui->transitionProps->setVisible(true);
}

void OBSBasic::HideTransitionProperties()
{
	ui->transitionProps->setVisible(false);
}

int OBSBasic::GetTransitionDuration()
{
	return ui->transitionDuration->value();
}

int OBSBasic::GetTbarPosition()
{
	return ui->tBar->value();
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		QAction *action = menu->addAction(QT_UTF8(name));

		action->setProperty("id", qt->id);
		action->setProperty("idx", i);
		connect(action, SIGNAL(triggered()), this, SLOT(QuickTransitionChange()));
	}
	return menu;
}

void OBSBasic::ClearQuickTransitionWidgets()
{
	for (int i = 0; i < 4; i++) {
		ui->quickTransitions[i]->menu()->clear();
		ui->quickTransitions[i]->setText(QTStr("Basic.QuickTransition.None"));
		ui->quickTransitions[i]->setEnabled(false);
		ui->transitionDurations[i]->setEnabled(false);
		ui->transitionRemove[i]->setEnabled(false);
	}
}

void OBSBasic::RefreshQuickTransitions()
{
	ClearQuickTransitionWidgets();

	for (size_t i = 0; i < quickTransitions.size() && i < 4; i++) {
		QuickTransition *qt = &quickTransitions[i];
		const char *name = obs_source_get_name(qt->transition);

		ui->quickTransitions[i]->setText(QT_UTF8(name));
		ui->quickTransitions[i]->setEnabled(true);
		ui->quickTransitions[i]->setMenu(CreateTransitionMenu(ui->quickTransitions[i], qt));
		ui->transitionDurations[i]->setEnabled(true);
		ui->transitionDurations[i]->setValue(qt->duration);
		ui->transitionDurations[i]->setProperty("id", qt->id);
		ui->transitionRemove[i]->setEnabled(true);
		ui->transitionRemove[i]->setProperty("id", qt->id);
	}
}

void OBSBasic::ClearQuickTransitions()
{
	for (auto &qt : quickTransitions)
		RemoveQuickTransitionHotkey(&qt);
	quickTransitions.clear();
	RefreshQuickTransitions();
}

OBSSource OBSBasic::GetOverrideTransition(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	const char *name = obs_data_get_string(data, "transition_override");
	OBSSource transition = FindTransition(name);
	obs_data_release(data);
	return transition;
}

int OBSBasic::GetOverrideTransitionDuration(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	int duration = (int)obs_data_get_int(data, "transition_duration");
	obs_data_release(data);
	return duration;
}

QMenu *OBSBasic::CreatePerSceneTransitionMenu()
{
	QMenu *menu = new QMenu(this);
	QAction *action;

	action = menu->addAction(QTStr("Basic.TransitionOverride.None"));
	action->setProperty("duration", QVariant(0));
	connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideClear()));

	menu->addSeparator();

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_name", QVariant(name));
		connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideSelected()));
	}

	return menu;
}

void OBSBasic::SetCurrentScene(OBSSource scene, bool force)
{
	if (!force && scene == currentScene)
		return;

	/* ------------------------------------------------------ */
	/* save previous scene                                    */

	OBSScene previousScene = currentScene;
	if (previousScene)
		obs_source_set_monitoring_type(previousScene, OBS_MONITORING_TYPE_NONE);

	/* ------------------------------------------------------ */
	/* activate new scene                                     */

	OBSScene nextScene = scene;
	if (nextScene) {
		obs_source_inc_showing(nextScene);
		obs_source_set_monitoring_type(nextScene, GetAudioMonitoringTypeFromSettings());
	}

	/* ------------------------------------------------------ */
	/* do the actual scene switch                             */

	if (previewProgramMode) {
		obs_frontend_set_current_preview_scene(nextScene);
		programScene = obs_frontend_get_current_program_scene();

		if (lastProgramScene && lastProgramScene != programScene)
			obs_source_set_monitoring_type(lastProgramScene, OBS_MONITORING_TYPE_NONE);

		lastProgramScene = programScene;

		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

	} else {
		obs_frontend_set_current_scene(nextScene);
	}

	/* ------------------------------------------------------ */

	RefreshSources(nextScene);
	UpdateContextBar();
	UpdateSourceListToolbar();

	if (loaded) {
		QListWidgetItem *item = ui->scenes->FindScene(nextScene);
		if (item) {
			ui->scenes->blockSignals(true);
			ui->scenes->setCurrentItem(item);
			ui->scenes->blockSignals(false);
		}
	}

	OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	if (previewProgramMode)
		OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
}

void OBSBasic::SetTransition(OBSSource transition)
{
	if (transition == current_transition)
		return;

	current_transition = transition;
	if (transition) {
		obs_source_set_monitoring_type(transition, GetAudioMonitoringTypeFromSettings());
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

void OBSBasic::OverrideTransition(OBSSource transition)
{
	if (transition == override_transition)
		return;

	override_transition = transition;
	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_OVERRIDE_CHANGED);
}

void OBSBasic::TransitionToScene(OBSScene scene, bool force)
{
	TransitionToScene(obs_scene_get_source(scene), force);
}

void OBSBasic::TransitionToScene(OBSSource scene, bool force, bool quickTransition, int quickDuration, bool black,
				 bool manual)
{
	if (!scene)
		return;

	OBSSource curScene = GetCurrentSceneSource();
	if (scene == curScene && !force)
		return;

	if (previewProgramMode && !manual) {
		if (scene == programScene) {
			if (quickTransition && quickDuration) {
				if (IsTransitionActive())
					return;

				OBSSource trans = GetCurrentTransition();
				obs_source_set_duration(trans, quickDuration);
				obs_transition_start(trans);
			}
			return;
		}

		SetCurrentScene(scene, true);
	} else {
		OBSSource trans = GetCurrentTransition();
		if (quickTransition && quickDuration) {
			if (IsTransitionActive())
				return;
			obs_source_set_duration(trans, quickDuration);
		}

		if (black)
			obs_transition_set_destination(trans, nullptr, OBS_TRANSITION_MODE_OVERRIDE);
		else
			obs_transition_set(trans, scene);
	}
}

void OBSBasic::AddTransition(const char *id)
{
	OBSSource transition = obs_source_create_private(id, nullptr, nullptr);
	if (!transition)
		return;

	if (!obs_source_is_transition(transition)) {
		OBSErrorBox(this, "Not a transition: %s", id);
		return;
	}

	if (safeModeTransitions.size()) {
		for (auto &t : safeModeTransitions) {
			if (t == transition)
				return;
		}
	}

	const char *name = obs_source_get_name(transition);
	ui->transitions->addItem(QT_UTF8(name), QVariant::fromValue(transition));
	ui->transitions->setCurrentIndex(ui->transitions->count() - 1);

	if (safeModeTransitions.size())
		safeModeTransitions.push_back(std::move(transition));
}

void OBSBasic::RenameTransition(OBSSource transition)
{
	int idx = ui->transitions->findData(QVariant::fromValue(transition));
	if (idx == -1)
		return;

	const char *name = obs_source_get_name(transition);
	ui->transitions->setItemText(idx, QT_UTF8(name));

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_RENAMED);
}

void OBSBasic::TransitionClicked()
{
	if (IsTransitionActive())
		return;

	if (previewProgramMode) {
		TransitionToScene(GetCurrentSceneSource(), true, false, 0, false, true);
	}
}

void OBSBasic::TransitionStopped()
{
	if (!loaded)
		return;

	if (previewProgramMode) {
		programScene = obs_frontend_get_current_program_scene();
		lastProgramScene = programScene;
		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

		OnEvent(OBS_FRONTEND_EVENT_PROGRAM_SCENE_CHANGED);
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
}

void OBSBasic::TransitionFullyStopped()
{
	if (!loaded)
		return;

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_FULLY_STOPPED);
}

void OBSBasic::AddQuickTransitionId(int id)
{
	if (quickTransitionIdCounter <= id)
		quickTransitionIdCounter = id + 1;
}

void OBSBasic::AddQuickTransition()
{
	QuickTransition qt;
	qt.id = quickTransitionIdCounter++;
	qt.name = QTStr("Basic.QuickTransition.DefaultName").arg(QString::number(qt.id));
	qt.duration = GetTransitionDuration();
	qt.hotkeys[0] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[1] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[2] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[3] = OBS_INVALID_HOTKEY_ID;
	qt.transition = current_transition;

	quickTransitions.push_back(qt);
	AddQuickTransitionHotkey(&quickTransitions.back());
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_ADDED);
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	obs_frontend_source_context context;
	context.id = qt->id;
	context.type = OBS_FRONTEND_SOURCE_CONTEXT_QUICK_TRANSITION;

	qt->hotkeys[0] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition1",
		QTStr("Basic.QuickTransition1").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[1] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition2",
		QTStr("Basic.QuickTransition2").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[2] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition3",
		QTStr("Basic.QuickTransition3").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[3] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition4",
		QTStr("Basic.QuickTransition4").arg(qt->name).toUtf8().constData(), context);
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	for (int i = 0; i < 4; i++) {
		obs_hotkey_unregister(qt->hotkeys[i]);
		qt->hotkeys[i] = OBS_INVALID_HOTKEY_ID;
	}
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		if (!s)
			continue;

		OBSDataAutoRelease data = obs_source_save(s);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data)
{
	if (!transitions)
		return;

	for (size_t i = 0; i < obs_data_array_count(transitions); i++) {
		OBSDataAutoRelease data = obs_data_array_item(transitions, i);
		OBSSource s = obs_load_source(data, cb, private_data);
		if (!s)
			continue;

		InitTransition(s);
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < quickTransitions.size(); i++) {
		OBSDataAutoRelease data = obs_data_create();
		quickTransitions[i].Save(data);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *arr)
{
	ClearQuickTransitions();
	if (!arr)
		return;

	for (size_t i = 0; i < obs_data_array_count(arr); i++) {
		OBSDataAutoRelease data = obs_data_array_item(arr, i);
		QuickTransition qt;
		qt.Load(data);
		AddQuickTransitionId(qt.id);
		AddQuickTransitionHotkey(&qt);
		quickTransitions.push_back(qt);
	}

	RefreshQuickTransitions();
}

QuickTransition *OBSBasic::GetQuickTransition(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return &quickTransitions[i];
	}

	return nullptr;
}

int OBSBasic::GetQuickTransitionIdx(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return (int)i;
	}

	return -1;
}

void OBSBasic::QuickTransitionClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::QuickTransitionChange()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int transIdx = action->property("idx").toInt();

	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->transition = ui->transitions->itemData(transIdx).value<OBSSource>();
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionChangeDuration(int value)
{
	QSpinBox *box = qobject_cast<QSpinBox *>(sender());
	if (!box)
		return;

	int id = box->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->duration = value;

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionRemoveClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int idx = GetQuickTransitionIdx(id);
	if (idx == -1)
		return;

	RemoveQuickTransitionHotkey(&quickTransitions[idx]);
	quickTransitions.erase(quickTransitions.begin() + idx);
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_REMOVED);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::TBarChanged(int value)
{
	float t = float(value) / float(ui->tBar->maximum());
	obs_transition_set_time(current_transition, t);
	tBarActive = true;
}

void OBSBasic::TBarReleased()
{
	tBarActive = false;
}

void OBSBasic::on_transitions_currentIndexChanged(int index)
{
	if (index == -1)
		return;

	OBSSource s = ui->transitions->itemData(index).value<OBSSource>();
	SetTransition(s);
}

void OBSBasic::on_transitionAdd_clicked()
{
	QMenu menu(this);
	obs_frontend_populate_transition_types(&menu, this, SLOT(AddTransition(const char *)));
	menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	int idx = ui->transitions->currentIndex();
	if (idx == -1)
		return;

	if (ui->transitions->count() == 1) {
		OBSErrorBox(this, "Can't remove the last transition!");
		return;
	}

	OBSSource s = ui->transitions->itemData(idx).value<OBSSource>();

	ui->transitions->removeItem(idx);

	for (auto &qt : quickTransitions) {
		if (qt.transition == s)
			qt.transition = ui->transitions->itemData(0).value<OBSSource>();
	}

	RefreshQuickTransitions();
	obs_source_remove(s);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource s = ui->transitions->currentData().value<OBSSource>();
	if (s)
		CreatePropertiesWindow(s);
}

void OBSBasic::on_transitionDuration_valueChanged()
{
	if (loading)
		return;

	obs_source_set_transition_duration(current_transition, ui->transitionDuration->value());

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED);
}

void OBSBasic::ShowTransitionProperties()
{
	ui->transitionProps->setVisible(true);
}

void OBSBasic::HideTransitionProperties()
{
	ui->transitionProps->setVisible(false);
}

int OBSBasic::GetTransitionDuration()
{
	return ui->transitionDuration->value();
}

int OBSBasic::GetTbarPosition()
{
	return ui->tBar->value();
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		QAction *action = menu->addAction(QT_UTF8(name));

		action->setProperty("id", qt->id);
		action->setProperty("idx", i);
		connect(action, SIGNAL(triggered()), this, SLOT(QuickTransitionChange()));
	}
	return menu;
}

void OBSBasic::ClearQuickTransitionWidgets()
{
	for (int i = 0; i < 4; i++) {
		ui->quickTransitions[i]->menu()->clear();
		ui->quickTransitions[i]->setText(QTStr("Basic.QuickTransition.None"));
		ui->quickTransitions[i]->setEnabled(false);
		ui->transitionDurations[i]->setEnabled(false);
		ui->transitionRemove[i]->setEnabled(false);
	}
}

void OBSBasic::RefreshQuickTransitions()
{
	ClearQuickTransitionWidgets();

	for (size_t i = 0; i < quickTransitions.size() && i < 4; i++) {
		QuickTransition *qt = &quickTransitions[i];
		const char *name = obs_source_get_name(qt->transition);

		ui->quickTransitions[i]->setText(QT_UTF8(name));
		ui->quickTransitions[i]->setEnabled(true);
		ui->quickTransitions[i]->setMenu(CreateTransitionMenu(ui->quickTransitions[i], qt));
		ui->transitionDurations[i]->setEnabled(true);
		ui->transitionDurations[i]->setValue(qt->duration);
		ui->transitionDurations[i]->setProperty("id", qt->id);
		ui->transitionRemove[i]->setEnabled(true);
		ui->transitionRemove[i]->setProperty("id", qt->id);
	}
}

void OBSBasic::ClearQuickTransitions()
{
	for (auto &qt : quickTransitions)
		RemoveQuickTransitionHotkey(&qt);
	quickTransitions.clear();
	RefreshQuickTransitions();
}

OBSSource OBSBasic::GetOverrideTransition(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	const char *name = obs_data_get_string(data, "transition_override");
	OBSSource transition = FindTransition(name);
	obs_data_release(data);
	return transition;
}

int OBSBasic::GetOverrideTransitionDuration(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	int duration = (int)obs_data_get_int(data, "transition_duration");
	obs_data_release(data);
	return duration;
}

QMenu *OBSBasic::CreatePerSceneTransitionMenu()
{
	QMenu *menu = new QMenu(this);
	QAction *action;

	action = menu->addAction(QTStr("Basic.TransitionOverride.None"));
	action->setProperty("duration", QVariant(0));
	connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideClear()));

	menu->addSeparator();

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_name", QVariant(name));
		connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideSelected()));
	}

	return menu;
}

void OBSBasic::SetCurrentScene(obs_source_t *scene, bool force)
{
	if (!force && scene == currentScene) // This 'currentScene' is an atomic<obs_scene_t*> from OBSBasic.hpp
		return;

	/* ------------------------------------------------------ */
	/* save previous scene                                    */

	OBSScene previousScene = currentScene; // This is the atomic one
	if (previousScene)
		obs_source_set_monitoring_type(previousScene, OBS_MONITORING_TYPE_NONE);

	/* ------------------------------------------------------ */
	/* activate new scene                                     */

	OBSScene nextScene = scene;
	if (nextScene) {
		obs_source_inc_showing(nextScene); // Should this be tied to the specific output pane?
		obs_source_set_monitoring_type(nextScene, GetAudioMonitoringTypeFromSettings());
	}

	/* ------------------------------------------------------ */
	/* do the actual scene switch                             */

	if (previewProgramMode) {
		obs_frontend_set_current_preview_scene(nextScene);
		programScene = obs_frontend_get_current_program_scene();

		if (lastProgramScene && lastProgramScene != programScene)
			obs_source_set_monitoring_type(lastProgramScene, OBS_MONITORING_TYPE_NONE);

		lastProgramScene = programScene;

		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

	} else {
		obs_frontend_set_current_scene(nextScene);
	}

	/* ------------------------------------------------------ */

	RefreshSources(nextScene);
	UpdateContextBar();
	UpdateSourceListToolbar();

	if (loaded) {
		QListWidgetItem *item = ui->scenes->FindScene(nextScene);
		if (item) {
			ui->scenes->blockSignals(true);
			ui->scenes->setCurrentItem(item);
			ui->scenes->blockSignals(false);
		}
	}

	OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	if (previewProgramMode)
		OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
}

void OBSBasic::SetTransition(OBSSource transition)
{
	if (transition == current_transition)
		return;

	current_transition = transition;
	if (transition) {
		obs_source_set_monitoring_type(transition, GetAudioMonitoringTypeFromSettings());
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

void OBSBasic::OverrideTransition(OBSSource transition)
{
	if (transition == override_transition)
		return;

	override_transition = transition;
	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_OVERRIDE_CHANGED);
}

void OBSBasic::TransitionToScene(OBSScene scene, bool force)
{
	TransitionToScene(obs_scene_get_source(scene), force);
}

void OBSBasic::TransitionToScene(OBSSource scene, bool force, bool quickTransition, int quickDuration, bool black,
				 bool manual)
{
	if (!scene)
		return;

	OBSSource curScene = GetCurrentSceneSource();
	if (scene == curScene && !force)
		return;

	if (previewProgramMode && !manual) {
		if (scene == programScene) {
			if (quickTransition && quickDuration) {
				if (IsTransitionActive())
					return;

				OBSSource trans = GetCurrentTransition();
				obs_source_set_duration(trans, quickDuration);
				obs_transition_start(trans);
			}
			return;
		}

		SetCurrentScene(scene, true);
	} else {
		OBSSource trans = GetCurrentTransition();
		if (quickTransition && quickDuration) {
			if (IsTransitionActive())
				return;
			obs_source_set_duration(trans, quickDuration);
		}

		if (black)
			obs_transition_set_destination(trans, nullptr, OBS_TRANSITION_MODE_OVERRIDE);
		else
			obs_transition_set(trans, scene);
	}
}

void OBSBasic::AddTransition(const char *id)
{
	OBSSource transition = obs_source_create_private(id, nullptr, nullptr);
	if (!transition)
		return;

	if (!obs_source_is_transition(transition)) {
		OBSErrorBox(this, "Not a transition: %s", id);
		return;
	}

	if (safeModeTransitions.size()) {
		for (auto &t : safeModeTransitions) {
			if (t == transition)
				return;
		}
	}

	const char *name = obs_source_get_name(transition);
	ui->transitions->addItem(QT_UTF8(name), QVariant::fromValue(transition));
	ui->transitions->setCurrentIndex(ui->transitions->count() - 1);

	if (safeModeTransitions.size())
		safeModeTransitions.push_back(std::move(transition));
}

void OBSBasic::RenameTransition(OBSSource transition)
{
	int idx = ui->transitions->findData(QVariant::fromValue(transition));
	if (idx == -1)
		return;

	const char *name = obs_source_get_name(transition);
	ui->transitions->setItemText(idx, QT_UTF8(name));

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_RENAMED);
}

void OBSBasic::TransitionClicked()
{
	if (IsTransitionActive())
		return;

	if (previewProgramMode) {
		TransitionToScene(GetCurrentSceneSource(), true, false, 0, false, true);
	}
}

void OBSBasic::TransitionStopped()
{
	if (!loaded)
		return;

	if (previewProgramMode) {
		programScene = obs_frontend_get_current_program_scene();
		lastProgramScene = programScene;
		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

		OnEvent(OBS_FRONTEND_EVENT_PROGRAM_SCENE_CHANGED);
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
}

void OBSBasic::TransitionFullyStopped()
{
	if (!loaded)
		return;

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_FULLY_STOPPED);
}

void OBSBasic::AddQuickTransitionId(int id)
{
	if (quickTransitionIdCounter <= id)
		quickTransitionIdCounter = id + 1;
}

void OBSBasic::AddQuickTransition()
{
	QuickTransition qt;
	qt.id = quickTransitionIdCounter++;
	qt.name = QTStr("Basic.QuickTransition.DefaultName").arg(QString::number(qt.id));
	qt.duration = GetTransitionDuration();
	qt.hotkeys[0] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[1] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[2] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[3] = OBS_INVALID_HOTKEY_ID;
	qt.transition = current_transition;

	quickTransitions.push_back(qt);
	AddQuickTransitionHotkey(&quickTransitions.back());
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_ADDED);
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	obs_frontend_source_context context;
	context.id = qt->id;
	context.type = OBS_FRONTEND_SOURCE_CONTEXT_QUICK_TRANSITION;

	qt->hotkeys[0] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition1",
		QTStr("Basic.QuickTransition1").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[1] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition2",
		QTStr("Basic.QuickTransition2").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[2] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition3",
		QTStr("Basic.QuickTransition3").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[3] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition4",
		QTStr("Basic.QuickTransition4").arg(qt->name).toUtf8().constData(), context);
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	for (int i = 0; i < 4; i++) {
		obs_hotkey_unregister(qt->hotkeys[i]);
		qt->hotkeys[i] = OBS_INVALID_HOTKEY_ID;
	}
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		if (!s)
			continue;

		OBSDataAutoRelease data = obs_source_save(s);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data)
{
	if (!transitions)
		return;

	for (size_t i = 0; i < obs_data_array_count(transitions); i++) {
		OBSDataAutoRelease data = obs_data_array_item(transitions, i);
		OBSSource s = obs_load_source(data, cb, private_data);
		if (!s)
			continue;

		InitTransition(s);
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < quickTransitions.size(); i++) {
		OBSDataAutoRelease data = obs_data_create();
		quickTransitions[i].Save(data);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *arr)
{
	ClearQuickTransitions();
	if (!arr)
		return;

	for (size_t i = 0; i < obs_data_array_count(arr); i++) {
		OBSDataAutoRelease data = obs_data_array_item(arr, i);
		QuickTransition qt;
		qt.Load(data);
		AddQuickTransitionId(qt.id);
		AddQuickTransitionHotkey(&qt);
		quickTransitions.push_back(qt);
	}

	RefreshQuickTransitions();
}

QuickTransition *OBSBasic::GetQuickTransition(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return &quickTransitions[i];
	}

	return nullptr;
}

int OBSBasic::GetQuickTransitionIdx(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return (int)i;
	}

	return -1;
}

void OBSBasic::QuickTransitionClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::QuickTransitionChange()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int transIdx = action->property("idx").toInt();

	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->transition = ui->transitions->itemData(transIdx).value<OBSSource>();
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionChangeDuration(int value)
{
	QSpinBox *box = qobject_cast<QSpinBox *>(sender());
	if (!box)
		return;

	int id = box->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->duration = value;

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionRemoveClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int idx = GetQuickTransitionIdx(id);
	if (idx == -1)
		return;

	RemoveQuickTransitionHotkey(&quickTransitions[idx]);
	quickTransitions.erase(quickTransitions.begin() + idx);
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_REMOVED);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::TBarChanged(int value)
{
	float t = float(value) / float(ui->tBar->maximum());
	obs_transition_set_time(current_transition, t);
	tBarActive = true;
}

void OBSBasic::TBarReleased()
{
	tBarActive = false;
}

void OBSBasic::on_transitions_currentIndexChanged(int index)
{
	if (index == -1)
		return;

	OBSSource s = ui->transitions->itemData(index).value<OBSSource>();
	SetTransition(s);
}

void OBSBasic::on_transitionAdd_clicked()
{
	QMenu menu(this);
	obs_frontend_populate_transition_types(&menu, this, SLOT(AddTransition(const char *)));
	menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	int idx = ui->transitions->currentIndex();
	if (idx == -1)
		return;

	if (ui->transitions->count() == 1) {
		OBSErrorBox(this, "Can't remove the last transition!");
		return;
	}

	OBSSource s = ui->transitions->itemData(idx).value<OBSSource>();

	ui->transitions->removeItem(idx);

	for (auto &qt : quickTransitions) {
		if (qt.transition == s)
			qt.transition = ui->transitions->itemData(0).value<OBSSource>();
	}

	RefreshQuickTransitions();
	obs_source_remove(s);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource s = ui->transitions->currentData().value<OBSSource>();
	if (s)
		CreatePropertiesWindow(s);
}

void OBSBasic::on_transitionDuration_valueChanged()
{
	if (loading)
		return;

	obs_source_set_transition_duration(current_transition, ui->transitionDuration->value());

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED);
}

void OBSBasic::ShowTransitionProperties()
{
	ui->transitionProps->setVisible(true);
}

void OBSBasic::HideTransitionProperties()
{
	ui->transitionProps->setVisible(false);
}

int OBSBasic::GetTransitionDuration()
{
	return ui->transitionDuration->value();
}

int OBSBasic::GetTbarPosition()
{
	return ui->tBar->value();
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		QAction *action = menu->addAction(QT_UTF8(name));

		action->setProperty("id", qt->id);
		action->setProperty("idx", i);
		connect(action, SIGNAL(triggered()), this, SLOT(QuickTransitionChange()));
	}
	return menu;
}

void OBSBasic::ClearQuickTransitionWidgets()
{
	for (int i = 0; i < 4; i++) {
		ui->quickTransitions[i]->menu()->clear();
		ui->quickTransitions[i]->setText(QTStr("Basic.QuickTransition.None"));
		ui->quickTransitions[i]->setEnabled(false);
		ui->transitionDurations[i]->setEnabled(false);
		ui->transitionRemove[i]->setEnabled(false);
	}
}

void OBSBasic::RefreshQuickTransitions()
{
	ClearQuickTransitionWidgets();

	for (size_t i = 0; i < quickTransitions.size() && i < 4; i++) {
		QuickTransition *qt = &quickTransitions[i];
		const char *name = obs_source_get_name(qt->transition);

		ui->quickTransitions[i]->setText(QT_UTF8(name));
		ui->quickTransitions[i]->setEnabled(true);
		ui->quickTransitions[i]->setMenu(CreateTransitionMenu(ui->quickTransitions[i], qt));
		ui->transitionDurations[i]->setEnabled(true);
		ui->transitionDurations[i]->setValue(qt->duration);
		ui->transitionDurations[i]->setProperty("id", qt->id);
		ui->transitionRemove[i]->setEnabled(true);
		ui->transitionRemove[i]->setProperty("id", qt->id);
	}
}

void OBSBasic::ClearQuickTransitions()
{
	for (auto &qt : quickTransitions)
		RemoveQuickTransitionHotkey(&qt);
	quickTransitions.clear();
	RefreshQuickTransitions();
}

OBSSource OBSBasic::GetOverrideTransition(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	const char *name = obs_data_get_string(data, "transition_override");
	OBSSource transition = FindTransition(name);
	obs_data_release(data);
	return transition;
}

int OBSBasic::GetOverrideTransitionDuration(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	int duration = (int)obs_data_get_int(data, "transition_duration");
	obs_data_release(data);
	return duration;
}

QMenu *OBSBasic::CreatePerSceneTransitionMenu()
{
	QMenu *menu = new QMenu(this);
	QAction *action;

	action = menu->addAction(QTStr("Basic.TransitionOverride.None"));
	action->setProperty("duration", QVariant(0));
	connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideClear()));

	menu->addSeparator();

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_name", QVariant(name));
		connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideSelected()));
	}

	return menu;
}

void OBSBasic::SetCurrentScene(obs_source_t *scene, bool force)
{
	if (!force && scene == currentScene) // This 'currentScene' is an atomic<obs_scene_t*> from OBSBasic.hpp
		return;

	/* ------------------------------------------------------ */
	/* save previous scene                                    */

	OBSScene previousScene = currentScene; // This is the atomic one
	if (previousScene)
		obs_source_set_monitoring_type(previousScene, OBS_MONITORING_TYPE_NONE);

	/* ------------------------------------------------------ */
	/* activate new scene                                     */

	OBSScene nextScene = scene;
	if (nextScene) {
		obs_source_inc_showing(nextScene); // Should this be tied to the specific output pane?
		obs_source_set_monitoring_type(nextScene, GetAudioMonitoringTypeFromSettings());
	}

	/* ------------------------------------------------------ */
	/* do the actual scene switch                             */

	if (previewProgramMode) {
		obs_frontend_set_current_preview_scene(nextScene);
		programScene = obs_frontend_get_current_program_scene();

		if (lastProgramScene && lastProgramScene != programScene)
			obs_source_set_monitoring_type(lastProgramScene, OBS_MONITORING_TYPE_NONE);

		lastProgramScene = programScene;

		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

	} else {
		obs_frontend_set_current_scene(nextScene);
	}

	/* ------------------------------------------------------ */

	RefreshSources(nextScene);
	UpdateContextBar();
	UpdateSourceListToolbar();

	if (loaded) {
		QListWidgetItem *item = ui->scenes->FindScene(nextScene);
		if (item) {
			ui->scenes->blockSignals(true);
			ui->scenes->setCurrentItem(item);
			ui->scenes->blockSignals(false);
		}
	}

	OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	if (previewProgramMode)
		OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
}

void OBSBasic::SetTransition(OBSSource transition)
{
	if (transition == current_transition)
		return;

	current_transition = transition;
	if (transition) {
		obs_source_set_monitoring_type(transition, GetAudioMonitoringTypeFromSettings());
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

void OBSBasic::OverrideTransition(OBSSource transition)
{
	if (transition == override_transition)
		return;

	override_transition = transition;
	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_OVERRIDE_CHANGED);
}

void OBSBasic::TransitionToScene(OBSScene scene, bool force)
{
	TransitionToScene(obs_scene_get_source(scene), force);
}

void OBSBasic::TransitionToScene(OBSSource scene, bool force, bool quickTransition, int quickDuration, bool black,
				 bool manual)
{
	if (!scene)
		return;

	OBSSource curScene = GetCurrentSceneSource();
	if (scene == curScene && !force)
		return;

	if (previewProgramMode && !manual) {
		if (scene == programScene) {
			if (quickTransition && quickDuration) {
				if (IsTransitionActive())
					return;

				OBSSource trans = GetCurrentTransition();
				obs_source_set_duration(trans, quickDuration);
				obs_transition_start(trans);
			}
			return;
		}

		SetCurrentScene(scene, true);
	} else {
		OBSSource trans = GetCurrentTransition();
		if (quickTransition && quickDuration) {
			if (IsTransitionActive())
				return;
			obs_source_set_duration(trans, quickDuration);
		}

		if (black)
			obs_transition_set_destination(trans, nullptr, OBS_TRANSITION_MODE_OVERRIDE);
		else
			obs_transition_set(trans, scene);
	}
}

void OBSBasic::AddTransition(const char *id)
{
	OBSSource transition = obs_source_create_private(id, nullptr, nullptr);
	if (!transition)
		return;

	if (!obs_source_is_transition(transition)) {
		OBSErrorBox(this, "Not a transition: %s", id);
		return;
	}

	if (safeModeTransitions.size()) {
		for (auto &t : safeModeTransitions) {
			if (t == transition)
				return;
		}
	}

	const char *name = obs_source_get_name(transition);
	ui->transitions->addItem(QT_UTF8(name), QVariant::fromValue(transition));
	ui->transitions->setCurrentIndex(ui->transitions->count() - 1);

	if (safeModeTransitions.size())
		safeModeTransitions.push_back(std::move(transition));
}

void OBSBasic::RenameTransition(OBSSource transition)
{
	int idx = ui->transitions->findData(QVariant::fromValue(transition));
	if (idx == -1)
		return;

	const char *name = obs_source_get_name(transition);
	ui->transitions->setItemText(idx, QT_UTF8(name));

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_RENAMED);
}

void OBSBasic::TransitionClicked()
{
	if (IsTransitionActive())
		return;

	if (previewProgramMode) {
		TransitionToScene(GetCurrentSceneSource(), true, false, 0, false, true);
	}
}

void OBSBasic::TransitionStopped()
{
	if (!loaded)
		return;

	if (previewProgramMode) {
		programScene = obs_frontend_get_current_program_scene();
		lastProgramScene = programScene;
		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

		OnEvent(OBS_FRONTEND_EVENT_PROGRAM_SCENE_CHANGED);
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
}

void OBSBasic::TransitionFullyStopped()
{
	if (!loaded)
		return;

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_FULLY_STOPPED);
}

void OBSBasic::AddQuickTransitionId(int id)
{
	if (quickTransitionIdCounter <= id)
		quickTransitionIdCounter = id + 1;
}

void OBSBasic::AddQuickTransition()
{
	QuickTransition qt;
	qt.id = quickTransitionIdCounter++;
	qt.name = QTStr("Basic.QuickTransition.DefaultName").arg(QString::number(qt.id));
	qt.duration = GetTransitionDuration();
	qt.hotkeys[0] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[1] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[2] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[3] = OBS_INVALID_HOTKEY_ID;
	qt.transition = current_transition;

	quickTransitions.push_back(qt);
	AddQuickTransitionHotkey(&quickTransitions.back());
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_ADDED);
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	obs_frontend_source_context context;
	context.id = qt->id;
	context.type = OBS_FRONTEND_SOURCE_CONTEXT_QUICK_TRANSITION;

	qt->hotkeys[0] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition1",
		QTStr("Basic.QuickTransition1").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[1] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition2",
		QTStr("Basic.QuickTransition2").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[2] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition3",
		QTStr("Basic.QuickTransition3").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[3] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition4",
		QTStr("Basic.QuickTransition4").arg(qt->name).toUtf8().constData(), context);
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	for (int i = 0; i < 4; i++) {
		obs_hotkey_unregister(qt->hotkeys[i]);
		qt->hotkeys[i] = OBS_INVALID_HOTKEY_ID;
	}
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		if (!s)
			continue;

		OBSDataAutoRelease data = obs_source_save(s);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data)
{
	if (!transitions)
		return;

	for (size_t i = 0; i < obs_data_array_count(transitions); i++) {
		OBSDataAutoRelease data = obs_data_array_item(transitions, i);
		OBSSource s = obs_load_source(data, cb, private_data);
		if (!s)
			continue;

		InitTransition(s);
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < quickTransitions.size(); i++) {
		OBSDataAutoRelease data = obs_data_create();
		quickTransitions[i].Save(data);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *arr)
{
	ClearQuickTransitions();
	if (!arr)
		return;

	for (size_t i = 0; i < obs_data_array_count(arr); i++) {
		OBSDataAutoRelease data = obs_data_array_item(arr, i);
		QuickTransition qt;
		qt.Load(data);
		AddQuickTransitionId(qt.id);
		AddQuickTransitionHotkey(&qt);
		quickTransitions.push_back(qt);
	}

	RefreshQuickTransitions();
}

QuickTransition *OBSBasic::GetQuickTransition(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return &quickTransitions[i];
	}

	return nullptr;
}

int OBSBasic::GetQuickTransitionIdx(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return (int)i;
	}

	return -1;
}

void OBSBasic::QuickTransitionClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::QuickTransitionChange()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int transIdx = action->property("idx").toInt();

	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->transition = ui->transitions->itemData(transIdx).value<OBSSource>();
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionChangeDuration(int value)
{
	QSpinBox *box = qobject_cast<QSpinBox *>(sender());
	if (!box)
		return;

	int id = box->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->duration = value;

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionRemoveClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int idx = GetQuickTransitionIdx(id);
	if (idx == -1)
		return;

	RemoveQuickTransitionHotkey(&quickTransitions[idx]);
	quickTransitions.erase(quickTransitions.begin() + idx);
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_REMOVED);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::TBarChanged(int value)
{
	float t = float(value) / float(ui->tBar->maximum());
	obs_transition_set_time(current_transition, t);
	tBarActive = true;
}

void OBSBasic::TBarReleased()
{
	tBarActive = false;
}

void OBSBasic::on_transitions_currentIndexChanged(int index)
{
	if (index == -1)
		return;

	OBSSource s = ui->transitions->itemData(index).value<OBSSource>();
	SetTransition(s);
}

void OBSBasic::on_transitionAdd_clicked()
{
	QMenu menu(this);
	obs_frontend_populate_transition_types(&menu, this, SLOT(AddTransition(const char *)));
	menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	int idx = ui->transitions->currentIndex();
	if (idx == -1)
		return;

	if (ui->transitions->count() == 1) {
		OBSErrorBox(this, "Can't remove the last transition!");
		return;
	}

	OBSSource s = ui->transitions->itemData(idx).value<OBSSource>();

	ui->transitions->removeItem(idx);

	for (auto &qt : quickTransitions) {
		if (qt.transition == s)
			qt.transition = ui->transitions->itemData(0).value<OBSSource>();
	}

	RefreshQuickTransitions();
	obs_source_remove(s);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource s = ui->transitions->currentData().value<OBSSource>();
	if (s)
		CreatePropertiesWindow(s);
}

void OBSBasic::on_transitionDuration_valueChanged()
{
	if (loading)
		return;

	obs_source_set_transition_duration(current_transition, ui->transitionDuration->value());

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED);
}

void OBSBasic::ShowTransitionProperties()
{
	ui->transitionProps->setVisible(true);
}

void OBSBasic::HideTransitionProperties()
{
	ui->transitionProps->setVisible(false);
}

int OBSBasic::GetTransitionDuration()
{
	return ui->transitionDuration->value();
}

int OBSBasic::GetTbarPosition()
{
	return ui->tBar->value();
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		QAction *action = menu->addAction(QT_UTF8(name));

		action->setProperty("id", qt->id);
		action->setProperty("idx", i);
		connect(action, SIGNAL(triggered()), this, SLOT(QuickTransitionChange()));
	}
	return menu;
}

void OBSBasic::ClearQuickTransitionWidgets()
{
	for (int i = 0; i < 4; i++) {
		ui->quickTransitions[i]->menu()->clear();
		ui->quickTransitions[i]->setText(QTStr("Basic.QuickTransition.None"));
		ui->quickTransitions[i]->setEnabled(false);
		ui->transitionDurations[i]->setEnabled(false);
		ui->transitionRemove[i]->setEnabled(false);
	}
}

void OBSBasic::RefreshQuickTransitions()
{
	ClearQuickTransitionWidgets();

	for (size_t i = 0; i < quickTransitions.size() && i < 4; i++) {
		QuickTransition *qt = &quickTransitions[i];
		const char *name = obs_source_get_name(qt->transition);

		ui->quickTransitions[i]->setText(QT_UTF8(name));
		ui->quickTransitions[i]->setEnabled(true);
		ui->quickTransitions[i]->setMenu(CreateTransitionMenu(ui->quickTransitions[i], qt));
		ui->transitionDurations[i]->setEnabled(true);
		ui->transitionDurations[i]->setValue(qt->duration);
		ui->transitionDurations[i]->setProperty("id", qt->id);
		ui->transitionRemove[i]->setEnabled(true);
		ui->transitionRemove[i]->setProperty("id", qt->id);
	}
}

void OBSBasic::ClearQuickTransitions()
{
	for (auto &qt : quickTransitions)
		RemoveQuickTransitionHotkey(&qt);
	quickTransitions.clear();
	RefreshQuickTransitions();
}

OBSSource OBSBasic::GetOverrideTransition(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	const char *name = obs_data_get_string(data, "transition_override");
	OBSSource transition = FindTransition(name);
	obs_data_release(data);
	return transition;
}

int OBSBasic::GetOverrideTransitionDuration(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	int duration = (int)obs_data_get_int(data, "transition_duration");
	obs_data_release(data);
	return duration;
}

QMenu *OBSBasic::CreatePerSceneTransitionMenu()
{
	QMenu *menu = new QMenu(this);
	QAction *action;

	action = menu->addAction(QTStr("Basic.TransitionOverride.None"));
	action->setProperty("duration", QVariant(0));
	connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideClear()));

	menu->addSeparator();

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_name", QVariant(name));
		connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideSelected()));
	}

	return menu;
}

void OBSBasic::SetCurrentScene(obs_source_t *scene, bool force)
{
	OBSSource currentTargetScene = nullptr;
	bool isVerticalPaneActive = (activePreviewPane == ActivePreview::VERTICAL && App()->IsDualOutputActive());

	if (isVerticalPaneActive) {
		currentTargetScene = App()->GetCurrentVerticalScene();
	} else {
		// If horizontal pane is active, or if dual output is not active,
		// the target is the main/horizontal scene.
		// In studio mode, this refers to the preview scene.
		currentTargetScene = previewProgramMode ? obs_frontend_get_current_preview_scene()
		                                       : obs_frontend_get_current_scene();
		// Fallback if App()->GetCurrentHorizontalScene() isn't set yet (e.g. initial load or if it's not tracking libobs preview/program)
		if (!currentTargetScene) currentTargetScene = App()->GetCurrentHorizontalScene();
	}

	if (!force && scene == currentTargetScene) {
		// If the UI (scene list) is trying to select the scene that's already
		// active for this pane, ensure the UI list selection matches, but don't re-trigger app logic.
		if (loaded) {
			QListWidgetItem *item = ui->scenes->FindScene(scene);
			if (item && ui->scenes->currentItem() != item) {
				ui->scenes->blockSignals(true);
				ui->scenes->setCurrentItem(item);
				ui->scenes->blockSignals(false);
			}
		}
		return;
	}

	OBSScene previousSceneForMonitoring = nullptr;

	if (isVerticalPaneActive) {
		previousSceneForMonitoring = App()->GetCurrentVerticalScene();
		App()->SetCurrentVerticalScene(scene); // This will emit verticalSceneChanged
		                                     // and handle output source if active.
		                                     // Does not interact with libobs global current scene/preview.
		// For vertical pane, audio monitoring is tied to the scene selected for it.
		if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());

	} else { // Horizontal pane active or dual output off
		previousSceneForMonitoring = previewProgramMode ? obs_frontend_get_current_preview_scene()
		                                            : obs_frontend_get_current_scene();
		if (!previousSceneForMonitoring) previousSceneForMonitoring = App()->GetCurrentHorizontalScene();


		App()->SetCurrentHorizontalScene(scene); // Emits horizontalSceneChanged

		// Update the core libobs current scene (preview or program)
		if (previewProgramMode) {
			obs_frontend_set_current_preview_scene(scene);
			// Program scene is updated by transitions.
			// Update monitoring for the new preview scene.
			if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());
			OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
		} else {
			obs_frontend_set_current_scene(scene);
			if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());
			OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
		}
	}

	// Stop monitoring on the previous scene for the specific pane/context if it's different
	if (previousSceneForMonitoring && previousSceneForMonitoring != scene) {
		obs_source_set_monitoring_type(previousSceneForMonitoring, OBS_MONITORING_TYPE_NONE);
	}

	// Common UI updates: Refresh source list for the newly selected scene in the UI.
	// Update context bar and source toolbar as they might depend on the selected scene.
	RefreshSources(scene);
	UpdateContextBar();
	UpdateSourceListToolbar();

	// Update the scene list widget selection to reflect the change.
	if (loaded) {
		QListWidgetItem *item = ui->scenes->FindScene(scene);
		if (item) {
			ui->scenes->blockSignals(true);
			ui->scenes->setCurrentItem(item);
			ui->scenes->blockSignals(false);
		}
	}
}

void OBSBasic::SetTransition(OBSSource transition)
{
	if (transition == current_transition)
		return;

	current_transition = transition;
	if (transition) {
		obs_source_set_monitoring_type(transition, GetAudioMonitoringTypeFromSettings());
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

void OBSBasic::OverrideTransition(OBSSource transition)
{
	if (transition == override_transition)
		return;

	override_transition = transition;
	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_OVERRIDE_CHANGED);
}

void OBSBasic::TransitionToScene(OBSScene scene, bool force)
{
	TransitionToScene(obs_scene_get_source(scene), force);
}

void OBSBasic::TransitionToScene(OBSSource scene, bool force, bool quickTransition, int quickDuration, bool black,
				 bool manual)
{
	if (!scene)
		return;

	OBSSource curScene = GetCurrentSceneSource();
	if (scene == curScene && !force)
		return;

	if (previewProgramMode && !manual) {
		if (scene == programScene) {
			if (quickTransition && quickDuration) {
				if (IsTransitionActive())
					return;

				OBSSource trans = GetCurrentTransition();
				obs_source_set_duration(trans, quickDuration);
				obs_transition_start(trans);
			}
			return;
		}

		SetCurrentScene(scene, true);
	} else {
		OBSSource trans = GetCurrentTransition();
		if (quickTransition && quickDuration) {
			if (IsTransitionActive())
				return;
			obs_source_set_duration(trans, quickDuration);
		}

		if (black)
			obs_transition_set_destination(trans, nullptr, OBS_TRANSITION_MODE_OVERRIDE);
		else
			obs_transition_set(trans, scene);
	}
}

void OBSBasic::AddTransition(const char *id)
{
	OBSSource transition = obs_source_create_private(id, nullptr, nullptr);
	if (!transition)
		return;

	if (!obs_source_is_transition(transition)) {
		OBSErrorBox(this, "Not a transition: %s", id);
		return;
	}

	if (safeModeTransitions.size()) {
		for (auto &t : safeModeTransitions) {
			if (t == transition)
				return;
		}
	}

	const char *name = obs_source_get_name(transition);
	ui->transitions->addItem(QT_UTF8(name), QVariant::fromValue(transition));
	ui->transitions->setCurrentIndex(ui->transitions->count() - 1);

	if (safeModeTransitions.size())
		safeModeTransitions.push_back(std::move(transition));
}

void OBSBasic::RenameTransition(OBSSource transition)
{
	int idx = ui->transitions->findData(QVariant::fromValue(transition));
	if (idx == -1)
		return;

	const char *name = obs_source_get_name(transition);
	ui->transitions->setItemText(idx, QT_UTF8(name));

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_RENAMED);
}

void OBSBasic::TransitionClicked()
{
	if (IsTransitionActive())
		return;

	if (previewProgramMode) {
		TransitionToScene(GetCurrentSceneSource(), true, false, 0, false, true);
	}
}

void OBSBasic::TransitionStopped()
{
	if (!loaded)
		return;

	if (previewProgramMode) {
		programScene = obs_frontend_get_current_program_scene();
		lastProgramScene = programScene;
		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

		OnEvent(OBS_FRONTEND_EVENT_PROGRAM_SCENE_CHANGED);
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
}

void OBSBasic::TransitionFullyStopped()
{
	if (!loaded)
		return;

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_FULLY_STOPPED);
}

void OBSBasic::AddQuickTransitionId(int id)
{
	if (quickTransitionIdCounter <= id)
		quickTransitionIdCounter = id + 1;
}

void OBSBasic::AddQuickTransition()
{
	QuickTransition qt;
	qt.id = quickTransitionIdCounter++;
	qt.name = QTStr("Basic.QuickTransition.DefaultName").arg(QString::number(qt.id));
	qt.duration = GetTransitionDuration();
	qt.hotkeys[0] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[1] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[2] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[3] = OBS_INVALID_HOTKEY_ID;
	qt.transition = current_transition;

	quickTransitions.push_back(qt);
	AddQuickTransitionHotkey(&quickTransitions.back());
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_ADDED);
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	obs_frontend_source_context context;
	context.id = qt->id;
	context.type = OBS_FRONTEND_SOURCE_CONTEXT_QUICK_TRANSITION;

	qt->hotkeys[0] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition1",
		QTStr("Basic.QuickTransition1").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[1] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition2",
		QTStr("Basic.QuickTransition2").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[2] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition3",
		QTStr("Basic.QuickTransition3").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[3] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition4",
		QTStr("Basic.QuickTransition4").arg(qt->name).toUtf8().constData(), context);
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	for (int i = 0; i < 4; i++) {
		obs_hotkey_unregister(qt->hotkeys[i]);
		qt->hotkeys[i] = OBS_INVALID_HOTKEY_ID;
	}
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		if (!s)
			continue;

		OBSDataAutoRelease data = obs_source_save(s);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data)
{
	if (!transitions)
		return;

	for (size_t i = 0; i < obs_data_array_count(transitions); i++) {
		OBSDataAutoRelease data = obs_data_array_item(transitions, i);
		OBSSource s = obs_load_source(data, cb, private_data);
		if (!s)
			continue;

		InitTransition(s);
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < quickTransitions.size(); i++) {
		OBSDataAutoRelease data = obs_data_create();
		quickTransitions[i].Save(data);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *arr)
{
	ClearQuickTransitions();
	if (!arr)
		return;

	for (size_t i = 0; i < obs_data_array_count(arr); i++) {
		OBSDataAutoRelease data = obs_data_array_item(arr, i);
		QuickTransition qt;
		qt.Load(data);
		AddQuickTransitionId(qt.id);
		AddQuickTransitionHotkey(&qt);
		quickTransitions.push_back(qt);
	}

	RefreshQuickTransitions();
}

QuickTransition *OBSBasic::GetQuickTransition(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return &quickTransitions[i];
	}

	return nullptr;
}

int OBSBasic::GetQuickTransitionIdx(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return (int)i;
	}

	return -1;
}

void OBSBasic::QuickTransitionClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::QuickTransitionChange()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int transIdx = action->property("idx").toInt();

	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->transition = ui->transitions->itemData(transIdx).value<OBSSource>();
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionChangeDuration(int value)
{
	QSpinBox *box = qobject_cast<QSpinBox *>(sender());
	if (!box)
		return;

	int id = box->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->duration = value;

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionRemoveClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int idx = GetQuickTransitionIdx(id);
	if (idx == -1)
		return;

	RemoveQuickTransitionHotkey(&quickTransitions[idx]);
	quickTransitions.erase(quickTransitions.begin() + idx);
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_REMOVED);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::TBarChanged(int value)
{
	float t = float(value) / float(ui->tBar->maximum());
	obs_transition_set_time(current_transition, t);
	tBarActive = true;
}

void OBSBasic::TBarReleased()
{
	tBarActive = false;
}

void OBSBasic::on_transitions_currentIndexChanged(int index)
{
	if (index == -1)
		return;

	OBSSource s = ui->transitions->itemData(index).value<OBSSource>();
	SetTransition(s);
}

void OBSBasic::on_transitionAdd_clicked()
{
	QMenu menu(this);
	obs_frontend_populate_transition_types(&menu, this, SLOT(AddTransition(const char *)));
	menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	int idx = ui->transitions->currentIndex();
	if (idx == -1)
		return;

	if (ui->transitions->count() == 1) {
		OBSErrorBox(this, "Can't remove the last transition!");
		return;
	}

	OBSSource s = ui->transitions->itemData(idx).value<OBSSource>();

	ui->transitions->removeItem(idx);

	for (auto &qt : quickTransitions) {
		if (qt.transition == s)
			qt.transition = ui->transitions->itemData(0).value<OBSSource>();
	}

	RefreshQuickTransitions();
	obs_source_remove(s);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource s = ui->transitions->currentData().value<OBSSource>();
	if (s)
		CreatePropertiesWindow(s);
}

void OBSBasic::on_transitionDuration_valueChanged()
{
	if (loading)
		return;

	obs_source_set_transition_duration(current_transition, ui->transitionDuration->value());

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED);
}

void OBSBasic::ShowTransitionProperties()
{
	ui->transitionProps->setVisible(true);
}

void OBSBasic::HideTransitionProperties()
{
	ui->transitionProps->setVisible(false);
}

int OBSBasic::GetTransitionDuration()
{
	return ui->transitionDuration->value();
}

int OBSBasic::GetTbarPosition()
{
	return ui->tBar->value();
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		QAction *action = menu->addAction(QT_UTF8(name));

		action->setProperty("id", qt->id);
		action->setProperty("idx", i);
		connect(action, SIGNAL(triggered()), this, SLOT(QuickTransitionChange()));
	}
	return menu;
}

void OBSBasic::ClearQuickTransitionWidgets()
{
	for (int i = 0; i < 4; i++) {
		ui->quickTransitions[i]->menu()->clear();
		ui->quickTransitions[i]->setText(QTStr("Basic.QuickTransition.None"));
		ui->quickTransitions[i]->setEnabled(false);
		ui->transitionDurations[i]->setEnabled(false);
		ui->transitionRemove[i]->setEnabled(false);
	}
}

void OBSBasic::RefreshQuickTransitions()
{
	ClearQuickTransitionWidgets();

	for (size_t i = 0; i < quickTransitions.size() && i < 4; i++) {
		QuickTransition *qt = &quickTransitions[i];
		const char *name = obs_source_get_name(qt->transition);

		ui->quickTransitions[i]->setText(QT_UTF8(name));
		ui->quickTransitions[i]->setEnabled(true);
		ui->quickTransitions[i]->setMenu(CreateTransitionMenu(ui->quickTransitions[i], qt));
		ui->transitionDurations[i]->setEnabled(true);
		ui->transitionDurations[i]->setValue(qt->duration);
		ui->transitionDurations[i]->setProperty("id", qt->id);
		ui->transitionRemove[i]->setEnabled(true);
		ui->transitionRemove[i]->setProperty("id", qt->id);
	}
}

void OBSBasic::ClearQuickTransitions()
{
	for (auto &qt : quickTransitions)
		RemoveQuickTransitionHotkey(&qt);
	quickTransitions.clear();
	RefreshQuickTransitions();
}

OBSSource OBSBasic::GetOverrideTransition(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	const char *name = obs_data_get_string(data, "transition_override");
	OBSSource transition = FindTransition(name);
	obs_data_release(data);
	return transition;
}

int OBSBasic::GetOverrideTransitionDuration(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	int duration = (int)obs_data_get_int(data, "transition_duration");
	obs_data_release(data);
	return duration;
}

QMenu *OBSBasic::CreatePerSceneTransitionMenu()
{
	QMenu *menu = new QMenu(this);
	QAction *action;

	action = menu->addAction(QTStr("Basic.TransitionOverride.None"));
	action->setProperty("duration", QVariant(0));
	connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideClear()));

	menu->addSeparator();

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_name", QVariant(name));
		connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideSelected()));
	}

	return menu;
}

void OBSBasic::SetCurrentScene(obs_source_t *scene, bool force)
{
	OBSSource currentTargetScene = nullptr;
	bool isVerticalPaneActive = (activePreviewPane == ActivePreview::VERTICAL && App()->IsDualOutputActive());

	if (isVerticalPaneActive) {
		currentTargetScene = App()->GetCurrentVerticalScene();
	} else {
		// If horizontal pane is active, or if dual output is not active,
		// the target is the main/horizontal scene.
		// In studio mode, this refers to the preview scene.
		currentTargetScene = previewProgramMode ? obs_frontend_get_current_preview_scene()
		                                       : obs_frontend_get_current_scene();
		// Fallback if App()->GetCurrentHorizontalScene() isn't set yet (e.g. initial load or if it's not tracking libobs preview/program)
		if (!currentTargetScene) currentTargetScene = App()->GetCurrentHorizontalScene();
	}

	if (!force && scene == currentTargetScene) {
		// If the UI (scene list) is trying to select the scene that's already
		// active for this pane, ensure the UI list selection matches, but don't re-trigger app logic.
		if (loaded) {
			QListWidgetItem *item = ui->scenes->FindScene(scene);
			if (item && ui->scenes->currentItem() != item) {
				ui->scenes->blockSignals(true);
				ui->scenes->setCurrentItem(item);
				ui->scenes->blockSignals(false);
			}
		}
		return;
	}

	OBSScene previousSceneForMonitoring = nullptr;

	if (isVerticalPaneActive) {
		previousSceneForMonitoring = App()->GetCurrentVerticalScene();
		App()->SetCurrentVerticalScene(scene); // This will emit verticalSceneChanged
		                                     // and handle output source if active.
		                                     // Does not interact with libobs global current scene/preview.
		// For vertical pane, audio monitoring is tied to the scene selected for it.
		if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());

	} else { // Horizontal pane active or dual output off
		previousSceneForMonitoring = previewProgramMode ? obs_frontend_get_current_preview_scene()
		                                            : obs_frontend_get_current_scene();
		if (!previousSceneForMonitoring) previousSceneForMonitoring = App()->GetCurrentHorizontalScene();


		App()->SetCurrentHorizontalScene(scene); // Emits horizontalSceneChanged

		// Update the core libobs current scene (preview or program)
		if (previewProgramMode) {
			obs_frontend_set_current_preview_scene(scene);
			// Program scene is updated by transitions.
			// Update monitoring for the new preview scene.
			if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());
			OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
		} else {
			obs_frontend_set_current_scene(scene);
			if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());
			OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
		}
	}

	// Stop monitoring on the previous scene for the specific pane/context if it's different
	if (previousSceneForMonitoring && previousSceneForMonitoring != scene) {
		obs_source_set_monitoring_type(previousSceneForMonitoring, OBS_MONITORING_TYPE_NONE);
	}

	// Common UI updates: Refresh source list for the newly selected scene in the UI.
	// Update context bar and source toolbar as they might depend on the selected scene.
	RefreshSources(scene);
	UpdateContextBar();
	UpdateSourceListToolbar();

	// Update the scene list widget selection to reflect the change.
	if (loaded) {
		QListWidgetItem *item = ui->scenes->FindScene(scene);
		if (item) {
			ui->scenes->blockSignals(true);
			ui->scenes->setCurrentItem(item);
			ui->scenes->blockSignals(false);
		}
	}
}

void OBSBasic::SetTransition(OBSSource transition)
{
	if (transition == current_transition)
		return;

	current_transition = transition;
	if (transition) {
		obs_source_set_monitoring_type(transition, GetAudioMonitoringTypeFromSettings());
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

void OBSBasic::OverrideTransition(OBSSource transition)
{
	if (transition == override_transition)
		return;

	override_transition = transition;
	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_OVERRIDE_CHANGED);
}

void OBSBasic::TransitionToScene(OBSScene scene, bool force)
{
	TransitionToScene(obs_scene_get_source(scene), force);
}

void OBSBasic::TransitionToScene(OBSSource scene, bool force, bool quickTransition, int quickDuration, bool black,
				 bool manual)
{
	if (!scene)
		return;

	OBSSource curScene = GetCurrentSceneSource();
	if (scene == curScene && !force)
		return;

	if (previewProgramMode && !manual) {
		if (scene == programScene) {
			if (quickTransition && quickDuration) {
				if (IsTransitionActive())
					return;

				OBSSource trans = GetCurrentTransition();
				obs_source_set_duration(trans, quickDuration);
				obs_transition_start(trans);
			}
			return;
		}

		SetCurrentScene(scene, true);
	} else {
		OBSSource trans = GetCurrentTransition();
		if (quickTransition && quickDuration) {
			if (IsTransitionActive())
				return;
			obs_source_set_duration(trans, quickDuration);
		}

		if (black)
			obs_transition_set_destination(trans, nullptr, OBS_TRANSITION_MODE_OVERRIDE);
		else
			obs_transition_set(trans, scene);
	}
}

void OBSBasic::AddTransition(const char *id)
{
	OBSSource transition = obs_source_create_private(id, nullptr, nullptr);
	if (!transition)
		return;

	if (!obs_source_is_transition(transition)) {
		OBSErrorBox(this, "Not a transition: %s", id);
		return;
	}

	if (safeModeTransitions.size()) {
		for (auto &t : safeModeTransitions) {
			if (t == transition)
				return;
		}
	}

	const char *name = obs_source_get_name(transition);
	ui->transitions->addItem(QT_UTF8(name), QVariant::fromValue(transition));
	ui->transitions->setCurrentIndex(ui->transitions->count() - 1);

	if (safeModeTransitions.size())
		safeModeTransitions.push_back(std::move(transition));
}

void OBSBasic::RenameTransition(OBSSource transition)
{
	int idx = ui->transitions->findData(QVariant::fromValue(transition));
	if (idx == -1)
		return;

	const char *name = obs_source_get_name(transition);
	ui->transitions->setItemText(idx, QT_UTF8(name));

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_RENAMED);
}

void OBSBasic::TransitionClicked()
{
	if (IsTransitionActive())
		return;

	if (previewProgramMode) {
		TransitionToScene(GetCurrentSceneSource(), true, false, 0, false, true);
	}
}

void OBSBasic::TransitionStopped()
{
	if (!loaded)
		return;

	if (previewProgramMode) {
		programScene = obs_frontend_get_current_program_scene();
		lastProgramScene = programScene;
		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

		OnEvent(OBS_FRONTEND_EVENT_PROGRAM_SCENE_CHANGED);
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
}

void OBSBasic::TransitionFullyStopped()
{
	if (!loaded)
		return;

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_FULLY_STOPPED);
}

void OBSBasic::AddQuickTransitionId(int id)
{
	if (quickTransitionIdCounter <= id)
		quickTransitionIdCounter = id + 1;
}

void OBSBasic::AddQuickTransition()
{
	QuickTransition qt;
	qt.id = quickTransitionIdCounter++;
	qt.name = QTStr("Basic.QuickTransition.DefaultName").arg(QString::number(qt.id));
	qt.duration = GetTransitionDuration();
	qt.hotkeys[0] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[1] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[2] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[3] = OBS_INVALID_HOTKEY_ID;
	qt.transition = current_transition;

	quickTransitions.push_back(qt);
	AddQuickTransitionHotkey(&quickTransitions.back());
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_ADDED);
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	obs_frontend_source_context context;
	context.id = qt->id;
	context.type = OBS_FRONTEND_SOURCE_CONTEXT_QUICK_TRANSITION;

	qt->hotkeys[0] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition1",
		QTStr("Basic.QuickTransition1").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[1] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition2",
		QTStr("Basic.QuickTransition2").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[2] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition3",
		QTStr("Basic.QuickTransition3").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[3] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition4",
		QTStr("Basic.QuickTransition4").arg(qt->name).toUtf8().constData(), context);
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	for (int i = 0; i < 4; i++) {
		obs_hotkey_unregister(qt->hotkeys[i]);
		qt->hotkeys[i] = OBS_INVALID_HOTKEY_ID;
	}
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		if (!s)
			continue;

		OBSDataAutoRelease data = obs_source_save(s);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data)
{
	if (!transitions)
		return;

	for (size_t i = 0; i < obs_data_array_count(transitions); i++) {
		OBSDataAutoRelease data = obs_data_array_item(transitions, i);
		OBSSource s = obs_load_source(data, cb, private_data);
		if (!s)
			continue;

		InitTransition(s);
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < quickTransitions.size(); i++) {
		OBSDataAutoRelease data = obs_data_create();
		quickTransitions[i].Save(data);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *arr)
{
	ClearQuickTransitions();
	if (!arr)
		return;

	for (size_t i = 0; i < obs_data_array_count(arr); i++) {
		OBSDataAutoRelease data = obs_data_array_item(arr, i);
		QuickTransition qt;
		qt.Load(data);
		AddQuickTransitionId(qt.id);
		AddQuickTransitionHotkey(&qt);
		quickTransitions.push_back(qt);
	}

	RefreshQuickTransitions();
}

QuickTransition *OBSBasic::GetQuickTransition(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return &quickTransitions[i];
	}

	return nullptr;
}

int OBSBasic::GetQuickTransitionIdx(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return (int)i;
	}

	return -1;
}

void OBSBasic::QuickTransitionClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::QuickTransitionChange()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int transIdx = action->property("idx").toInt();

	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->transition = ui->transitions->itemData(transIdx).value<OBSSource>();
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionChangeDuration(int value)
{
	QSpinBox *box = qobject_cast<QSpinBox *>(sender());
	if (!box)
		return;

	int id = box->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->duration = value;

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionRemoveClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int idx = GetQuickTransitionIdx(id);
	if (idx == -1)
		return;

	RemoveQuickTransitionHotkey(&quickTransitions[idx]);
	quickTransitions.erase(quickTransitions.begin() + idx);
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_REMOVED);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::TBarChanged(int value)
{
	float t = float(value) / float(ui->tBar->maximum());
	obs_transition_set_time(current_transition, t);
	tBarActive = true;
}

void OBSBasic::TBarReleased()
{
	tBarActive = false;
}

void OBSBasic::on_transitions_currentIndexChanged(int index)
{
	if (index == -1)
		return;

	OBSSource s = ui->transitions->itemData(index).value<OBSSource>();
	SetTransition(s);
}

void OBSBasic::on_transitionAdd_clicked()
{
	QMenu menu(this);
	obs_frontend_populate_transition_types(&menu, this, SLOT(AddTransition(const char *)));
	menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	int idx = ui->transitions->currentIndex();
	if (idx == -1)
		return;

	if (ui->transitions->count() == 1) {
		OBSErrorBox(this, "Can't remove the last transition!");
		return;
	}

	OBSSource s = ui->transitions->itemData(idx).value<OBSSource>();

	ui->transitions->removeItem(idx);

	for (auto &qt : quickTransitions) {
		if (qt.transition == s)
			qt.transition = ui->transitions->itemData(0).value<OBSSource>();
	}

	RefreshQuickTransitions();
	obs_source_remove(s);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource s = ui->transitions->currentData().value<OBSSource>();
	if (s)
		CreatePropertiesWindow(s);
}

void OBSBasic::on_transitionDuration_valueChanged()
{
	if (loading)
		return;

	obs_source_set_transition_duration(current_transition, ui->transitionDuration->value());

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED);
}

void OBSBasic::ShowTransitionProperties()
{
	ui->transitionProps->setVisible(true);
}

void OBSBasic::HideTransitionProperties()
{
	ui->transitionProps->setVisible(false);
}

int OBSBasic::GetTransitionDuration()
{
	return ui->transitionDuration->value();
}

int OBSBasic::GetTbarPosition()
{
	return ui->tBar->value();
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		QAction *action = menu->addAction(QT_UTF8(name));

		action->setProperty("id", qt->id);
		action->setProperty("idx", i);
		connect(action, SIGNAL(triggered()), this, SLOT(QuickTransitionChange()));
	}
	return menu;
}

void OBSBasic::ClearQuickTransitionWidgets()
{
	for (int i = 0; i < 4; i++) {
		ui->quickTransitions[i]->menu()->clear();
		ui->quickTransitions[i]->setText(QTStr("Basic.QuickTransition.None"));
		ui->quickTransitions[i]->setEnabled(false);
		ui->transitionDurations[i]->setEnabled(false);
		ui->transitionRemove[i]->setEnabled(false);
	}
}

void OBSBasic::RefreshQuickTransitions()
{
	ClearQuickTransitionWidgets();

	for (size_t i = 0; i < quickTransitions.size() && i < 4; i++) {
		QuickTransition *qt = &quickTransitions[i];
		const char *name = obs_source_get_name(qt->transition);

		ui->quickTransitions[i]->setText(QT_UTF8(name));
		ui->quickTransitions[i]->setEnabled(true);
		ui->quickTransitions[i]->setMenu(CreateTransitionMenu(ui->quickTransitions[i], qt));
		ui->transitionDurations[i]->setEnabled(true);
		ui->transitionDurations[i]->setValue(qt->duration);
		ui->transitionDurations[i]->setProperty("id", qt->id);
		ui->transitionRemove[i]->setEnabled(true);
		ui->transitionRemove[i]->setProperty("id", qt->id);
	}
}

void OBSBasic::ClearQuickTransitions()
{
	for (auto &qt : quickTransitions)
		RemoveQuickTransitionHotkey(&qt);
	quickTransitions.clear();
	RefreshQuickTransitions();
}

OBSSource OBSBasic::GetOverrideTransition(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	const char *name = obs_data_get_string(data, "transition_override");
	OBSSource transition = FindTransition(name);
	obs_data_release(data);
	return transition;
}

int OBSBasic::GetOverrideTransitionDuration(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	int duration = (int)obs_data_get_int(data, "transition_duration");
	obs_data_release(data);
	return duration;
}

QMenu *OBSBasic::CreatePerSceneTransitionMenu()
{
	QMenu *menu = new QMenu(this);
	QAction *action;

	action = menu->addAction(QTStr("Basic.TransitionOverride.None"));
	action->setProperty("duration", QVariant(0));
	connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideClear()));

	menu->addSeparator();

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_name", QVariant(name));
		connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideSelected()));
	}

	return menu;
}

void OBSBasic::SetCurrentScene(obs_source_t *scene, bool force)
{
	OBSSource currentTargetScene = nullptr;
	bool isVerticalPaneActive = (activePreviewPane == ActivePreview::VERTICAL && App()->IsDualOutputActive());

	if (isVerticalPaneActive) {
		currentTargetScene = App()->GetCurrentVerticalScene();
	} else {
		// If horizontal pane is active, or if dual output is not active,
		// the target is the main/horizontal scene.
		// In studio mode, this refers to the preview scene.
		currentTargetScene = previewProgramMode ? obs_frontend_get_current_preview_scene()
		                                       : obs_frontend_get_current_scene();
		// Fallback if App()->GetCurrentHorizontalScene() isn't set yet (e.g. initial load or if it's not tracking libobs preview/program)
		if (!currentTargetScene) currentTargetScene = App()->GetCurrentHorizontalScene();
	}

	if (!force && scene == currentTargetScene) {
		// If the UI (scene list) is trying to select the scene that's already
		// active for this pane, ensure the UI list selection matches, but don't re-trigger app logic.
		if (loaded) {
			QListWidgetItem *item = ui->scenes->FindScene(scene);
			if (item && ui->scenes->currentItem() != item) {
				ui->scenes->blockSignals(true);
				ui->scenes->setCurrentItem(item);
				ui->scenes->blockSignals(false);
			}
		}
		return;
	}

	OBSScene previousSceneForMonitoring = nullptr;

	if (isVerticalPaneActive) {
		previousSceneForMonitoring = App()->GetCurrentVerticalScene();
		App()->SetCurrentVerticalScene(scene); // This will emit verticalSceneChanged
		                                     // and handle output source if active.
		                                     // Does not interact with libobs global current scene/preview.
		// For vertical pane, audio monitoring is tied to the scene selected for it.
		if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());

	} else { // Horizontal pane active or dual output off
		previousSceneForMonitoring = previewProgramMode ? obs_frontend_get_current_preview_scene()
		                                            : obs_frontend_get_current_scene();
		if (!previousSceneForMonitoring) previousSceneForMonitoring = App()->GetCurrentHorizontalScene();


		App()->SetCurrentHorizontalScene(scene); // Emits horizontalSceneChanged

		// Update the core libobs current scene (preview or program)
		if (previewProgramMode) {
			obs_frontend_set_current_preview_scene(scene);
			// Program scene is updated by transitions.
			// Update monitoring for the new preview scene.
			if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());
			OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
		} else {
			obs_frontend_set_current_scene(scene);
			if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());
			OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
		}
	}

	// Stop monitoring on the previous scene for the specific pane/context if it's different
	if (previousSceneForMonitoring && previousSceneForMonitoring != scene) {
		obs_source_set_monitoring_type(previousSceneForMonitoring, OBS_MONITORING_TYPE_NONE);
	}

	// Common UI updates: Refresh source list for the newly selected scene in the UI.
	// Update context bar and source toolbar as they might depend on the selected scene.
	RefreshSources(scene);
	UpdateContextBar();
	UpdateSourceListToolbar();

	// Update the scene list widget selection to reflect the change.
	if (loaded) {
		QListWidgetItem *item = ui->scenes->FindScene(scene);
		if (item) {
			ui->scenes->blockSignals(true);
			ui->scenes->setCurrentItem(item);
			ui->scenes->blockSignals(false);
		}
	}
}

void OBSBasic::SetTransition(OBSSource transition)
{
	if (transition == current_transition)
		return;

	current_transition = transition;
	if (transition) {
		obs_source_set_monitoring_type(transition, GetAudioMonitoringTypeFromSettings());
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

void OBSBasic::OverrideTransition(OBSSource transition)
{
	if (transition == override_transition)
		return;

	override_transition = transition;
	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_OVERRIDE_CHANGED);
}

void OBSBasic::TransitionToScene(OBSScene scene, bool force)
{
	TransitionToScene(obs_scene_get_source(scene), force);
}

void OBSBasic::TransitionToScene(OBSSource scene, bool force, bool quickTransition, int quickDuration, bool black,
				 bool manual)
{
	if (!scene)
		return;

	OBSSource curScene = GetCurrentSceneSource();
	if (scene == curScene && !force)
		return;

	if (previewProgramMode && !manual) {
		if (scene == programScene) {
			if (quickTransition && quickDuration) {
				if (IsTransitionActive())
					return;

				OBSSource trans = GetCurrentTransition();
				obs_source_set_duration(trans, quickDuration);
				obs_transition_start(trans);
			}
			return;
		}

		SetCurrentScene(scene, true);
	} else {
		OBSSource trans = GetCurrentTransition();
		if (quickTransition && quickDuration) {
			if (IsTransitionActive())
				return;
			obs_source_set_duration(trans, quickDuration);
		}

		if (black)
			obs_transition_set_destination(trans, nullptr, OBS_TRANSITION_MODE_OVERRIDE);
		else
			obs_transition_set(trans, scene);
	}
}

void OBSBasic::AddTransition(const char *id)
{
	OBSSource transition = obs_source_create_private(id, nullptr, nullptr);
	if (!transition)
		return;

	if (!obs_source_is_transition(transition)) {
		OBSErrorBox(this, "Not a transition: %s", id);
		return;
	}

	if (safeModeTransitions.size()) {
		for (auto &t : safeModeTransitions) {
			if (t == transition)
				return;
		}
	}

	const char *name = obs_source_get_name(transition);
	ui->transitions->addItem(QT_UTF8(name), QVariant::fromValue(transition));
	ui->transitions->setCurrentIndex(ui->transitions->count() - 1);

	if (safeModeTransitions.size())
		safeModeTransitions.push_back(std::move(transition));
}

void OBSBasic::RenameTransition(OBSSource transition)
{
	int idx = ui->transitions->findData(QVariant::fromValue(transition));
	if (idx == -1)
		return;

	const char *name = obs_source_get_name(transition);
	ui->transitions->setItemText(idx, QT_UTF8(name));

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_RENAMED);
}

void OBSBasic::TransitionClicked()
{
	if (IsTransitionActive())
		return;

	if (previewProgramMode) {
		TransitionToScene(GetCurrentSceneSource(), true, false, 0, false, true);
	}
}

void OBSBasic::TransitionStopped()
{
	if (!loaded)
		return;

	if (previewProgramMode) {
		programScene = obs_frontend_get_current_program_scene();
		lastProgramScene = programScene;
		if (programScene)
			obs_source_set_monitoring_type(programScene, GetAudioMonitoringTypeFromSettings());

		OnEvent(OBS_FRONTEND_EVENT_PROGRAM_SCENE_CHANGED);
	}

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
}

void OBSBasic::TransitionFullyStopped()
{
	if (!loaded)
		return;

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_FULLY_STOPPED);
}

void OBSBasic::AddQuickTransitionId(int id)
{
	if (quickTransitionIdCounter <= id)
		quickTransitionIdCounter = id + 1;
}

void OBSBasic::AddQuickTransition()
{
	QuickTransition qt;
	qt.id = quickTransitionIdCounter++;
	qt.name = QTStr("Basic.QuickTransition.DefaultName").arg(QString::number(qt.id));
	qt.duration = GetTransitionDuration();
	qt.hotkeys[0] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[1] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[2] = OBS_INVALID_HOTKEY_ID;
	qt.hotkeys[3] = OBS_INVALID_HOTKEY_ID;
	qt.transition = current_transition;

	quickTransitions.push_back(qt);
	AddQuickTransitionHotkey(&quickTransitions.back());
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_ADDED);
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	obs_frontend_source_context context;
	context.id = qt->id;
	context.type = OBS_FRONTEND_SOURCE_CONTEXT_QUICK_TRANSITION;

	qt->hotkeys[0] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition1",
		QTStr("Basic.QuickTransition1").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[1] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition2",
		QTStr("Basic.QuickTransition2").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[2] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition3",
		QTStr("Basic.QuickTransition3").arg(qt->name).toUtf8().constData(), context);

	qt->hotkeys[3] = obs_hotkey_register_frontend_source(
		"OBSBasic.QuickTransition4",
		QTStr("Basic.QuickTransition4").arg(qt->name).toUtf8().constData(), context);
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	if (!qt)
		return;

	for (int i = 0; i < 4; i++) {
		obs_hotkey_unregister(qt->hotkeys[i]);
		qt->hotkeys[i] = OBS_INVALID_HOTKEY_ID;
	}
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		if (!s)
			continue;

		OBSDataAutoRelease data = obs_source_save(s);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data)
{
	if (!transitions)
		return;

	for (size_t i = 0; i < obs_data_array_count(transitions); i++) {
		OBSDataAutoRelease data = obs_data_array_item(transitions, i);
		OBSSource s = obs_load_source(data, cb, private_data);
		if (!s)
			continue;

		InitTransition(s);
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *arr = obs_data_array_create();

	for (size_t i = 0; i < quickTransitions.size(); i++) {
		OBSDataAutoRelease data = obs_data_create();
		quickTransitions[i].Save(data);
		obs_data_array_push_back(arr, data);
	}

	return arr;
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *arr)
{
	ClearQuickTransitions();
	if (!arr)
		return;

	for (size_t i = 0; i < obs_data_array_count(arr); i++) {
		OBSDataAutoRelease data = obs_data_array_item(arr, i);
		QuickTransition qt;
		qt.Load(data);
		AddQuickTransitionId(qt.id);
		AddQuickTransitionHotkey(&qt);
		quickTransitions.push_back(qt);
	}

	RefreshQuickTransitions();
}

QuickTransition *OBSBasic::GetQuickTransition(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return &quickTransitions[i];
	}

	return nullptr;
}

int OBSBasic::GetQuickTransitionIdx(int id)
{
	for (size_t i = 0; i < quickTransitions.size(); i++) {
		if (quickTransitions[i].id == id)
			return (int)i;
	}

	return -1;
}

void OBSBasic::QuickTransitionClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::QuickTransitionChange()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int transIdx = action->property("idx").toInt();

	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->transition = ui->transitions->itemData(transIdx).value<OBSSource>();
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionChangeDuration(int value)
{
	QSpinBox *box = qobject_cast<QSpinBox *>(sender());
	if (!box)
		return;

	int id = box->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	qt->duration = value;

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_CHANGED);
}

void OBSBasic::QuickTransitionRemoveClicked()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	int id = action->property("id").toInt();
	int idx = GetQuickTransitionIdx(id);
	if (idx == -1)
		return;

	RemoveQuickTransitionHotkey(&quickTransitions[idx]);
	quickTransitions.erase(quickTransitions.begin() + idx);
	RefreshQuickTransitions();

	OnEvent(OBS_FRONTEND_EVENT_QUICK_TRANSITION_REMOVED);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	if (IsTransitionActive())
		return;

	OBSSource scene = GetCurrentSceneSource();
	if (previewProgramMode)
		scene = programScene;

	TransitionToScene(scene, false, true, qt->duration, false, true);
}

void OBSBasic::TBarChanged(int value)
{
	float t = float(value) / float(ui->tBar->maximum());
	obs_transition_set_time(current_transition, t);
	tBarActive = true;
}

void OBSBasic::TBarReleased()
{
	tBarActive = false;
}

void OBSBasic::on_transitions_currentIndexChanged(int index)
{
	if (index == -1)
		return;

	OBSSource s = ui->transitions->itemData(index).value<OBSSource>();
	SetTransition(s);
}

void OBSBasic::on_transitionAdd_clicked()
{
	QMenu menu(this);
	obs_frontend_populate_transition_types(&menu, this, SLOT(AddTransition(const char *)));
	menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	int idx = ui->transitions->currentIndex();
	if (idx == -1)
		return;

	if (ui->transitions->count() == 1) {
		OBSErrorBox(this, "Can't remove the last transition!");
		return;
	}

	OBSSource s = ui->transitions->itemData(idx).value<OBSSource>();

	ui->transitions->removeItem(idx);

	for (auto &qt : quickTransitions) {
		if (qt.transition == s)
			qt.transition = ui->transitions->itemData(0).value<OBSSource>();
	}

	RefreshQuickTransitions();
	obs_source_remove(s);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource s = ui->transitions->currentData().value<OBSSource>();
	if (s)
		CreatePropertiesWindow(s);
}

void OBSBasic::on_transitionDuration_valueChanged()
{
	if (loading)
		return;

	obs_source_set_transition_duration(current_transition, ui->transitionDuration->value());

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED);
}

void OBSBasic::ShowTransitionProperties()
{
	ui->transitionProps->setVisible(true);
}

void OBSBasic::HideTransitionProperties()
{
	ui->transitionProps->setVisible(false);
}

int OBSBasic::GetTransitionDuration()
{
	return ui->transitionDuration->value();
}

int OBSBasic::GetTbarPosition()
{
	return ui->tBar->value();
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		QAction *action = menu->addAction(QT_UTF8(name));

		action->setProperty("id", qt->id);
		action->setProperty("idx", i);
		connect(action, SIGNAL(triggered()), this, SLOT(QuickTransitionChange()));
	}
	return menu;
}

void OBSBasic::ClearQuickTransitionWidgets()
{
	for (int i = 0; i < 4; i++) {
		ui->quickTransitions[i]->menu()->clear();
		ui->quickTransitions[i]->setText(QTStr("Basic.QuickTransition.None"));
		ui->quickTransitions[i]->setEnabled(false);
		ui->transitionDurations[i]->setEnabled(false);
		ui->transitionRemove[i]->setEnabled(false);
	}
}

void OBSBasic::RefreshQuickTransitions()
{
	ClearQuickTransitionWidgets();

	for (size_t i = 0; i < quickTransitions.size() && i < 4; i++) {
		QuickTransition *qt = &quickTransitions[i];
		const char *name = obs_source_get_name(qt->transition);

		ui->quickTransitions[i]->setText(QT_UTF8(name));
		ui->quickTransitions[i]->setEnabled(true);
		ui->quickTransitions[i]->setMenu(CreateTransitionMenu(ui->quickTransitions[i], qt));
		ui->transitionDurations[i]->setEnabled(true);
		ui->transitionDurations[i]->setValue(qt->duration);
		ui->transitionDurations[i]->setProperty("id", qt->id);
		ui->transitionRemove[i]->setEnabled(true);
		ui->transitionRemove[i]->setProperty("id", qt->id);
	}
}

void OBSBasic::ClearQuickTransitions()
{
	for (auto &qt : quickTransitions)
		RemoveQuickTransitionHotkey(&qt);
	quickTransitions.clear();
	RefreshQuickTransitions();
}

OBSSource OBSBasic::GetOverrideTransition(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	const char *name = obs_data_get_string(data, "transition_override");
	OBSSource transition = FindTransition(name);
	obs_data_release(data);
	return transition;
}

int OBSBasic::GetOverrideTransitionDuration(OBSSource source)
{
	obs_data_t *data = obs_source_get_private_settings(source);
	int duration = (int)obs_data_get_int(data, "transition_duration");
	obs_data_release(data);
	return duration;
}

QMenu *OBSBasic::CreatePerSceneTransitionMenu()
{
	QMenu *menu = new QMenu(this);
	QAction *action;

	action = menu->addAction(QTStr("Basic.TransitionOverride.None"));
	action->setProperty("duration", QVariant(0));
	connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideClear()));

	menu->addSeparator();

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource s = ui->transitions->itemData(i).value<OBSSource>();
		const char *name = obs_source_get_name(s);
		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_name", QVariant(name));
		connect(action, SIGNAL(triggered()), this, SLOT(TransitionOverrideSelected()));
	}

	return menu;
}

void OBSBasic::SetCurrentScene(obs_source_t *scene, bool force)
{
	OBSSource currentTargetScene = nullptr;
	bool isVerticalPaneActive = (activePreviewPane == ActivePreview::VERTICAL && App()->IsDualOutputActive());

	if (isVerticalPaneActive) {
		currentTargetScene = App()->GetCurrentVerticalScene();
	} else {
		// If horizontal pane is active, or if dual output is not active,
		// the target is the main/horizontal scene.
		// In studio mode, this refers to the preview scene.
		currentTargetScene = previewProgramMode ? obs_frontend_get_current_preview_scene()
		                                       : obs_frontend_get_current_scene();
		// Fallback if App()->GetCurrentHorizontalScene() isn't set yet (e.g. initial load or if it's not tracking libobs preview/program)
		if (!currentTargetScene) currentTargetScene = App()->GetCurrentHorizontalScene();
	}

	if (!force && scene == currentTargetScene) {
		// If the UI (scene list) is trying to select the scene that's already
		// active for this pane, ensure the UI list selection matches, but don't re-trigger app logic.
		if (loaded) {
			QListWidgetItem *item = ui->scenes->FindScene(scene);
			if (item && ui->scenes->currentItem() != item) {
				ui->scenes->blockSignals(true);
				ui->scenes->setCurrentItem(item);
				ui->scenes->blockSignals(false);
			}
		}
		return;
	}

	OBSScene previousSceneForMonitoring = nullptr;

	if (isVerticalPaneActive) {
		previousSceneForMonitoring = App()->GetCurrentVerticalScene();
		App()->SetCurrentVerticalScene(scene); // This will emit verticalSceneChanged
		                                     // and handle output source if active.
		                                     // Does not interact with libobs global current scene/preview.
		// For vertical pane, audio monitoring is tied to the scene selected for it.
		if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());

	} else { // Horizontal pane active or dual output off
		previousSceneForMonitoring = previewProgramMode ? obs_frontend_get_current_preview_scene()
		                                            : obs_frontend_get_current_scene();
		if (!previousSceneForMonitoring) previousSceneForMonitoring = App()->GetCurrentHorizontalScene();


		App()->SetCurrentHorizontalScene(scene); // Emits horizontalSceneChanged

		// Update the core libobs current scene (preview or program)
		if (previewProgramMode) {
			obs_frontend_set_current_preview_scene(scene);
			// Program scene is updated by transitions.
			// Update monitoring for the new preview scene.
			if (scene) obs_source_set_monitoring_type(scene, GetAudioMonitoringTypeFromSettings());
			OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
		} else {
			obs_frontend_set_current_scene(scene);
			if (scene) obs_source_
