/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

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
#include "ui-config.h"

#include <cstddef>
#include <ctime>
#include <functional>
#include <unordered_set>
#include <obs-data.h>
#include <obs.h>
#include <obs.hpp>
#include <QGuiApplication>
#include <QMessageBox>
#include <QShowEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QScreen>
#include <QColorDialog>
#include <QSizePolicy>
#include <QScrollBar>
#include <QTextStream>
#include <QActionGroup>
#include <qt-wrappers.hpp>

#include <util/dstr.h>
#include <util/util.hpp>
#include <util/platform.h>
#include <util/profiler.hpp>
#include <util/dstr.hpp>

#include "obs-app.hpp"
#include "platform.hpp"
#include "visibility-item-widget.hpp"
#include "item-widget-helpers.hpp"
#include "basic-controls.hpp"
#include "window-basic-settings.hpp"
#include "window-namedialog.hpp"
#include "window-basic-auto-config.hpp"
#include "window-basic-source-select.hpp"
#include "window-basic-main.hpp"
#include "window-basic-stats.hpp"
#include "window-basic-main-outputs.hpp"
#include "window-basic-vcam-config.hpp"
#include "window-log-reply.hpp"
#ifdef __APPLE__
#include "window-permissions.hpp"
#endif
#include "window-projector.hpp"
#include "window-remux.hpp"
#ifdef YOUTUBE_ENABLED
#include "auth-youtube.hpp"
#include "window-youtube-actions.hpp"
#include "youtube-api-wrappers.hpp"
#endif
#include "window-whats-new.hpp"
#include "context-bar-controls.hpp"
#include "obs-proxy-style.hpp"
#include "display-helpers.hpp"
#include "volume-control.hpp"
#include "remote-text.hpp"
#include "ui-validation.hpp"
#include "media-controls.hpp"
#include "undo-stack-obs.hpp"
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include "update/win-update.hpp"
#include "update/shared-update.hpp"
#include "windows.h"
#endif

#ifdef WHATSNEW_ENABLED
#include "update/models/whatsnew.hpp"
#endif

#if !defined(_WIN32) && defined(WHATSNEW_ENABLED)
#include "update/shared-update.hpp"
#endif

#ifdef ENABLE_SPARKLE_UPDATER
#include "update/mac-update.hpp"
#endif

#include "ui_OBSBasic.h"
#include "ui_ColorSelect.h"

#include <QWindow>

#ifdef ENABLE_WAYLAND
#include <obs-nix-platform.h>
#endif

using namespace std;

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif

#include "ui-config.h"

struct QCef;
struct QCefCookieManager;

QCef *cef = nullptr;
QCefCookieManager *panel_cookies = nullptr;
bool cef_js_avail = false;

extern std::string opt_starting_profile;
extern std::string opt_starting_collection;

void DestroyPanelCookieManager();

namespace {

QPointer<OBSWhatsNew> obsWhatsNew;

template<typename OBSRef> struct SignalContainer {
	OBSRef ref;
	vector<shared_ptr<OBSSignal>> handlers;
};
} // namespace

extern volatile long insideEventLoop;

Q_DECLARE_METATYPE(OBSScene);
Q_DECLARE_METATYPE(OBSSceneItem);
Q_DECLARE_METATYPE(OBSSource);
Q_DECLARE_METATYPE(obs_order_movement);
Q_DECLARE_METATYPE(SignalContainer<OBSScene>);

QDataStream &operator<<(QDataStream &out, const SignalContainer<OBSScene> &v)
{
	out << v.ref;
	return out;
}

QDataStream &operator>>(QDataStream &in, SignalContainer<OBSScene> &v)
{
	in >> v.ref;
	return in;
}

template<typename T> static T GetOBSRef(QListWidgetItem *item)
{
	return item->data(static_cast<int>(QtDataRole::OBSRef)).value<T>();
}

template<typename T> static void SetOBSRef(QListWidgetItem *item, T &&val)
{
	item->setData(static_cast<int>(QtDataRole::OBSRef), QVariant::fromValue(val));
}

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
		string data_path_with_module_suffix;
		data_path_with_module_suffix += plugins_data_path;
		data_path_with_module_suffix += "/%module%";
		obs_add_module_path(plugins_path.c_str(), data_path_with_module_suffix.c_str());
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

extern obs_frontend_callbacks *InitializeAPIInterface(OBSBasic *main);

void assignDockToggle(QDockWidget *dock, QAction *action)
{
	auto handleWindowToggle = [action](bool vis) {
		action->blockSignals(true);
		action->setChecked(vis);
		action->blockSignals(false);
	};
	auto handleMenuToggle = [dock](bool check) {
		dock->blockSignals(true);
		dock->setVisible(check);
		dock->blockSignals(false);
	};

	dock->connect(dock->toggleViewAction(), &QAction::toggled, handleWindowToggle);
	dock->connect(action, &QAction::toggled, handleMenuToggle);
}

void setupDockAction(QDockWidget *dock)
{
	QAction *action = dock->toggleViewAction();

	auto neverDisable = [action]() {
		QSignalBlocker block(action);
		action->setEnabled(true);
	};

	auto newToggleView = [dock](bool check) {
		QSignalBlocker block(dock);
		dock->setVisible(check);
	};

	// Replace the slot connected by default
	QObject::disconnect(action, &QAction::triggered, nullptr, 0);
	dock->connect(action, &QAction::triggered, newToggleView);

	// Make the action unable to be disabled
	action->connect(action, &QAction::enabledChanged, neverDisable);
}

extern void RegisterTwitchAuth();
extern void RegisterRestreamAuth();
#ifdef YOUTUBE_ENABLED
extern void RegisterYoutubeAuth();
#endif

OBSBasic::OBSBasic(QWidget *parent) : OBSMainWindow(parent), undo_s(ui), ui(new Ui::OBSBasic)
{
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

	connect(controls, &OBSBasicControls::ExitButtonClicked, this, &QMainWindow::close);

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
		UpdatePreviewScrollbars();
		dpi = devicePixelRatioF();
	};
	dpi = devicePixelRatioF();

	connect(windowHandle(), &QWindow::screenChanged, displayResize);
	connect(ui->preview, &OBSQTDisplay::DisplayResized, displayResize);

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

	connect(this, &OBSBasic::CanvasResized, ui->previewScalingMode, &OBSPreviewScalingComboBox::CanvasResized);
	connect(this, &OBSBasic::OutputResized, ui->previewScalingMode, &OBSPreviewScalingComboBox::OutputResized);

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

#ifdef HAVE_OBSCONFIG_H
	ui->actionReleaseNotes->setVisible(true);
#endif

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
}

static void SaveAudioDevice(const char *name, int channel, obs_data_t *parent, vector<OBSSource> &audioSources)
{
	OBSSourceAutoRelease source = obs_get_output_source(channel);
	if (!source)
		return;

	audioSources.push_back(source.Get());

	OBSDataAutoRelease data = obs_save_source(source);

	obs_data_set_obj(parent, name, data);
}

static obs_data_t *GenerateSaveData(obs_data_array_t *sceneOrder, obs_data_array_t *quickTransitionData,
				    int transitionDuration, obs_data_array_t *transitions, OBSScene &scene,
				    OBSSource &curProgramScene, obs_data_array_t *savedProjectorList)
{
	obs_data_t *saveData = obs_data_create();

	vector<OBSSource> audioSources;
	audioSources.reserve(6);

	SaveAudioDevice(DESKTOP_AUDIO_1, 1, saveData, audioSources);
	SaveAudioDevice(DESKTOP_AUDIO_2, 2, saveData, audioSources);
	SaveAudioDevice(AUX_AUDIO_1, 3, saveData, audioSources);
	SaveAudioDevice(AUX_AUDIO_2, 4, saveData, audioSources);
	SaveAudioDevice(AUX_AUDIO_3, 5, saveData, audioSources);
	SaveAudioDevice(AUX_AUDIO_4, 6, saveData, audioSources);

	/* -------------------------------- */
	/* save non-group sources           */

	auto FilterAudioSources = [&](obs_source_t *source) {
		if (obs_source_is_group(source))
			return false;

		return find(begin(audioSources), end(audioSources), source) == end(audioSources);
	};
	using FilterAudioSources_t = decltype(FilterAudioSources);

	obs_data_array_t *sourcesArray = obs_save_sources_filtered(
		[](void *data, obs_source_t *source) {
			auto &func = *static_cast<FilterAudioSources_t *>(data);
			return func(source);
		},
		static_cast<void *>(&FilterAudioSources));

	/* -------------------------------- */
	/* save group sources separately    */

	/* saving separately ensures they won't be loaded in older versions */
	obs_data_array_t *groupsArray = obs_save_sources_filtered(
		[](void *, obs_source_t *source) { return obs_source_is_group(source); }, nullptr);

	/* -------------------------------- */

	OBSSourceAutoRelease transition = obs_get_output_source(0);
	obs_source_t *currentScene = obs_scene_get_source(scene);
	const char *sceneName = obs_source_get_name(currentScene);
	const char *programName = obs_source_get_name(curProgramScene);

	const char *sceneCollection = config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection");

	obs_data_set_string(saveData, "current_scene", sceneName);
	obs_data_set_string(saveData, "current_program_scene", programName);
	obs_data_set_array(saveData, "scene_order", sceneOrder);
	obs_data_set_string(saveData, "name", sceneCollection);
	obs_data_set_array(saveData, "sources", sourcesArray);
	obs_data_set_array(saveData, "groups", groupsArray);
	obs_data_set_array(saveData, "quick_transitions", quickTransitionData);
	obs_data_set_array(saveData, "transitions", transitions);
	obs_data_set_array(saveData, "saved_projectors", savedProjectorList);
	obs_data_array_release(sourcesArray);
	obs_data_array_release(groupsArray);

	obs_data_set_string(saveData, "current_transition", obs_source_get_name(transition));
	obs_data_set_int(saveData, "transition_duration", transitionDuration);

	return saveData;
}

void OBSBasic::copyActionsDynamicProperties()
{
	// Themes need the QAction dynamic properties
	for (QAction *x : ui->scenesToolbar->actions()) {
		QWidget *temp = ui->scenesToolbar->widgetForAction(x);

		if (!temp)
			continue;

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}

	for (QAction *x : ui->sourcesToolbar->actions()) {
		QWidget *temp = ui->sourcesToolbar->widgetForAction(x);

		if (!temp)
			continue;

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}

	for (QAction *x : ui->mixerToolbar->actions()) {
		QWidget *temp = ui->mixerToolbar->widgetForAction(x);

		if (!temp)
			continue;

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}
}

void OBSBasic::UpdateVolumeControlsDecayRate()
{
	double meterDecayRate = config_get_double(activeConfiguration, "Audio", "MeterDecayRate");

	for (const auto &vol : volumes)
		vol->SetMeterDecayRate(meterDecayRate);
}

void OBSBasic::UpdateVolumeControlsPeakMeterType()
{
	uint32_t peakMeterTypeIdx = config_get_uint(activeConfiguration, "Audio", "PeakMeterType");

	enum obs_peak_meter_type peakMeterType;
	switch (peakMeterTypeIdx) {
	case 0:
		peakMeterType = SAMPLE_PEAK_METER;
		break;
	case 1:
		peakMeterType = TRUE_PEAK_METER;
		break;
	default:
		peakMeterType = SAMPLE_PEAK_METER;
		break;
	}

	for (const auto &vol : volumes)
		vol->setPeakMeterType(peakMeterType);
}

void OBSBasic::ClearVolumeControls()
{
	for (VolControl *vol : volumes)
		delete vol;

	volumes.clear();
}

void OBSBasic::RefreshVolumeColors()
{
	for (VolControl *vol : volumes) {
		vol->refreshColors();
	}
}

obs_data_array_t *OBSBasic::SaveSceneListOrder()
{
	obs_data_array_t *sceneOrder = obs_data_array_create();

	for (int i = 0; i < ui->scenes->count(); i++) {
		OBSDataAutoRelease data = obs_data_create();
		obs_data_set_string(data, "name", QT_TO_UTF8(ui->scenes->item(i)->text()));
		obs_data_array_push_back(sceneOrder, data);
	}

	return sceneOrder;
}

obs_data_array_t *OBSBasic::SaveProjectors()
{
	obs_data_array_t *savedProjectors = obs_data_array_create();

	auto saveProjector = [savedProjectors](OBSProjector *projector) {
		if (!projector)
			return;

		OBSDataAutoRelease data = obs_data_create();
		ProjectorType type = projector->GetProjectorType();

		switch (type) {
		case ProjectorType::Scene:
		case ProjectorType::Source: {
			OBSSource source = projector->GetSource();
			const char *name = obs_source_get_name(source);
			obs_data_set_string(data, "name", name);
			break;
		}
		default:
			break;
		}

		obs_data_set_int(data, "monitor", projector->GetMonitor());
		obs_data_set_int(data, "type", static_cast<int>(type));
		obs_data_set_string(data, "geometry", projector->saveGeometry().toBase64().constData());

		if (projector->IsAlwaysOnTopOverridden())
			obs_data_set_bool(data, "alwaysOnTop", projector->IsAlwaysOnTop());

		obs_data_set_bool(data, "alwaysOnTopOverridden", projector->IsAlwaysOnTopOverridden());

		obs_data_array_push_back(savedProjectors, data);
	};

	for (const auto &proj : projectors)
		saveProjector(static_cast<OBSProjector *>(proj));

	return savedProjectors;
}

void OBSBasic::Save(const char *file)
{
	OBSScene scene = GetCurrentScene();
	OBSSource curProgramScene = OBSGetStrongRef(programScene);
	if (!curProgramScene)
		curProgramScene = obs_scene_get_source(scene);

	OBSDataArrayAutoRelease sceneOrder = SaveSceneListOrder();
	OBSDataArrayAutoRelease transitions = SaveTransitions();
	OBSDataArrayAutoRelease quickTrData = SaveQuickTransitions();
	OBSDataArrayAutoRelease savedProjectorList = SaveProjectors();
	OBSDataAutoRelease saveData = GenerateSaveData(sceneOrder, quickTrData, ui->transitionDuration->value(),
						       transitions, scene, curProgramScene, savedProjectorList);

	obs_data_set_bool(saveData, "preview_locked", ui->preview->Locked());
	obs_data_set_bool(saveData, "scaling_enabled", ui->preview->IsFixedScaling());
	obs_data_set_int(saveData, "scaling_level", ui->preview->GetScalingLevel());
	obs_data_set_double(saveData, "scaling_off_x", ui->preview->GetScrollX());
	obs_data_set_double(saveData, "scaling_off_y", ui->preview->GetScrollY());

	if (vcamEnabled) {
		OBSDataAutoRelease obj = obs_data_create();

		obs_data_set_int(obj, "type2", (int)vcamConfig.type);
		switch (vcamConfig.type) {
		case VCamOutputType::Invalid:
		case VCamOutputType::ProgramView:
		case VCamOutputType::PreviewOutput:
			break;
		case VCamOutputType::SceneOutput:
			obs_data_set_string(obj, "scene", vcamConfig.scene.c_str());
			break;
		case VCamOutputType::SourceOutput:
			obs_data_set_string(obj, "source", vcamConfig.source.c_str());
			break;
		}

		obs_data_set_obj(saveData, "virtual-camera", obj);
	}

	if (api) {
		if (!collectionModuleData)
			collectionModuleData = obs_data_create();

		api->on_save(collectionModuleData);
		obs_data_set_obj(saveData, "modules", collectionModuleData);
	}

	if (lastOutputResolution) {
		OBSDataAutoRelease res = obs_data_create();
		obs_data_set_int(res, "x", lastOutputResolution->first);
		obs_data_set_int(res, "y", lastOutputResolution->second);
		obs_data_set_obj(saveData, "resolution", res);
	}

	obs_data_set_int(saveData, "version", usingAbsoluteCoordinates ? 1 : 2);

	if (migrationBaseResolution && !usingAbsoluteCoordinates) {
		OBSDataAutoRelease res = obs_data_create();
		obs_data_set_int(res, "x", migrationBaseResolution->first);
		obs_data_set_int(res, "y", migrationBaseResolution->second);
		obs_data_set_obj(saveData, "migration_resolution", res);
	}

	if (!obs_data_save_json_pretty_safe(saveData, file, "tmp", "bak"))
		blog(LOG_ERROR, "Could not save scene data to %s", file);
}

void OBSBasic::DeferSaveBegin()
{
	os_atomic_inc_long(&disableSaving);
}

void OBSBasic::DeferSaveEnd()
{
	long result = os_atomic_dec_long(&disableSaving);
	if (result == 0) {
		SaveProject();
	}
}

static void LogFilter(obs_source_t *, obs_source_t *filter, void *v_val);

static void LoadAudioDevice(const char *name, int channel, obs_data_t *parent)
{
	OBSDataAutoRelease data = obs_data_get_obj(parent, name);
	if (!data)
		return;

	OBSSourceAutoRelease source = obs_load_source(data);
	if (!source)
		return;

	obs_set_output_source(channel, source);

	const char *source_name = obs_source_get_name(source);
	blog(LOG_INFO, "[Loaded global audio device]: '%s'", source_name);
	obs_source_enum_filters(source, LogFilter, (void *)(intptr_t)1);
	obs_monitoring_type monitoring_type = obs_source_get_monitoring_type(source);
	if (monitoring_type != OBS_MONITORING_TYPE_NONE) {
		const char *type = (monitoring_type == OBS_MONITORING_TYPE_MONITOR_ONLY) ? "monitor only"
											 : "monitor and output";

		blog(LOG_INFO, "    - monitoring: %s", type);
	}
}

static inline bool HasAudioDevices(const char *source_id)
{
	const char *output_id = source_id;
	obs_properties_t *props = obs_get_source_properties(output_id);
	size_t count = 0;

	if (!props)
		return false;

	obs_property_t *devices = obs_properties_get(props, "device_id");
	if (devices)
		count = obs_property_list_item_count(devices);

	obs_properties_destroy(props);

	return count != 0;
}

void OBSBasic::CreateFirstRunSources()
{
	bool hasDesktopAudio = HasAudioDevices(App()->OutputAudioSource());
	bool hasInputAudio = HasAudioDevices(App()->InputAudioSource());

#ifdef __APPLE__
	/* On macOS 13 and above, the SCK based audio capture provides a
	 * better alternative to the device-based audio capture. */
	if (__builtin_available(macOS 13.0, *)) {
		hasDesktopAudio = false;
	}
#endif

	if (hasDesktopAudio)
		ResetAudioDevice(App()->OutputAudioSource(), "default", Str("Basic.DesktopDevice1"), 1);
	if (hasInputAudio)
		ResetAudioDevice(App()->InputAudioSource(), "default", Str("Basic.AuxDevice1"), 3);
}

void OBSBasic::DisableRelativeCoordinates(bool enable)
{
	/* Allow disabling relative positioning to allow loading collections
	 * that cannot yet be migrated. */
	OBSDataAutoRelease priv = obs_get_private_data();
	obs_data_set_bool(priv, "AbsoluteCoordinates", enable);
	usingAbsoluteCoordinates = enable;

	ui->actionRemigrateSceneCollection->setText(enable ? QTStr("Basic.MainMenu.SceneCollection.Migrate")
							   : QTStr("Basic.MainMenu.SceneCollection.Remigrate"));
	ui->actionRemigrateSceneCollection->setEnabled(enable);
}

void OBSBasic::CreateDefaultScene(bool firstStart)
{
	disableSaving++;

	ClearSceneData();
	InitDefaultTransitions();
	CreateDefaultQuickTransitions();
	ui->transitionDuration->setValue(300);
	SetTransition(fadeTransition);

	DisableRelativeCoordinates(false);
	OBSSceneAutoRelease scene = obs_scene_create(Str("Basic.Scene"));

	if (firstStart)
		CreateFirstRunSources();

	SetCurrentScene(scene, true);

	disableSaving--;
}

static void ReorderItemByName(QListWidget *lw, const char *name, int newIndex)
{
	for (int i = 0; i < lw->count(); i++) {
		QListWidgetItem *item = lw->item(i);

		if (strcmp(name, QT_TO_UTF8(item->text())) == 0) {
			if (newIndex != i) {
				item = lw->takeItem(i);
				lw->insertItem(newIndex, item);
			}
			break;
		}
	}
}

void OBSBasic::LoadSceneListOrder(obs_data_array_t *array)
{
	size_t num = obs_data_array_count(array);

	for (size_t i = 0; i < num; i++) {
		OBSDataAutoRelease data = obs_data_array_item(array, i);
		const char *name = obs_data_get_string(data, "name");

		ReorderItemByName(ui->scenes, name, (int)i);
	}
}

void OBSBasic::LoadSavedProjectors(obs_data_array_t *array)
{
	for (SavedProjectorInfo *info : savedProjectorsArray) {
		delete info;
	}
	savedProjectorsArray.clear();

	size_t num = obs_data_array_count(array);

	for (size_t i = 0; i < num; i++) {
		OBSDataAutoRelease data = obs_data_array_item(array, i);

		SavedProjectorInfo *info = new SavedProjectorInfo();
		info->monitor = obs_data_get_int(data, "monitor");
		info->type = static_cast<ProjectorType>(obs_data_get_int(data, "type"));
		info->geometry = std::string(obs_data_get_string(data, "geometry"));
		info->name = std::string(obs_data_get_string(data, "name"));
		info->alwaysOnTop = obs_data_get_bool(data, "alwaysOnTop");
		info->alwaysOnTopOverridden = obs_data_get_bool(data, "alwaysOnTopOverridden");

		savedProjectorsArray.emplace_back(info);
	}
}

static void LogFilter(obs_source_t *, obs_source_t *filter, void *v_val)
{
	const char *name = obs_source_get_name(filter);
	const char *id = obs_source_get_id(filter);
	int val = (int)(intptr_t)v_val;
	string indent;

	for (int i = 0; i < val; i++)
		indent += "    ";

	blog(LOG_INFO, "%s- filter: '%s' (%s)", indent.c_str(), name, id);
}

static bool LogSceneItem(obs_scene_t *, obs_sceneitem_t *item, void *v_val)
{
	obs_source_t *source = obs_sceneitem_get_source(item);
	const char *name = obs_source_get_name(source);
	const char *id = obs_source_get_id(source);
	int indent_count = (int)(intptr_t)v_val;
	string indent;

	for (int i = 0; i < indent_count; i++)
		indent += "    ";

	blog(LOG_INFO, "%s- source: '%s' (%s)", indent.c_str(), name, id);

	obs_monitoring_type monitoring_type = obs_source_get_monitoring_type(source);

	if (monitoring_type != OBS_MONITORING_TYPE_NONE) {
		const char *type = (monitoring_type == OBS_MONITORING_TYPE_MONITOR_ONLY) ? "monitor only"
											 : "monitor and output";

		blog(LOG_INFO, "    %s- monitoring: %s", indent.c_str(), type);
	}
	int child_indent = 1 + indent_count;
	obs_source_enum_filters(source, LogFilter, (void *)(intptr_t)child_indent);

	obs_source_t *show_tn = obs_sceneitem_get_transition(item, true);
	obs_source_t *hide_tn = obs_sceneitem_get_transition(item, false);
	if (show_tn)
		blog(LOG_INFO, "    %s- show: '%s' (%s)", indent.c_str(), obs_source_get_name(show_tn),
		     obs_source_get_id(show_tn));
	if (hide_tn)
		blog(LOG_INFO, "    %s- hide: '%s' (%s)", indent.c_str(), obs_source_get_name(hide_tn),
		     obs_source_get_id(hide_tn));

	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, LogSceneItem, (void *)(intptr_t)child_indent);
	return true;
}

void OBSBasic::LogScenes()
{
	blog(LOG_INFO, "------------------------------------------------");
	blog(LOG_INFO, "Loaded scenes:");

	for (int i = 0; i < ui->scenes->count(); i++) {
		QListWidgetItem *item = ui->scenes->item(i);
		OBSScene scene = GetOBSRef<OBSScene>(item);

		obs_source_t *source = obs_scene_get_source(scene);
		const char *name = obs_source_get_name(source);

		blog(LOG_INFO, "- scene '%s':", name);
		obs_scene_enum_items(scene, LogSceneItem, (void *)(intptr_t)1);
		obs_source_enum_filters(source, LogFilter, (void *)(intptr_t)1);
	}

	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::Load(const char *file, bool remigrate)
{
	disableSaving++;
	lastOutputResolution.reset();
	migrationBaseResolution.reset();

	obs_data_t *data = obs_data_create_from_json_file_safe(file, "bak");
	if (!data) {
		disableSaving--;
		const auto path = filesystem::u8path(file);
		const string name = path.stem().u8string();
		/* Check if file exists but failed to load. */
		if (filesystem::exists(path)) {
			/* Assume the file is corrupt and rename it to allow
			 * for manual recovery if possible. */
			auto newPath = path;
			newPath.concat(".invalid");

			blog(LOG_WARNING,
			     "File exists but appears to be corrupt, renaming "
			     "to \"%s\" before continuing.",
			     newPath.filename().u8string().c_str());

			error_code ec;
			filesystem::rename(path, newPath, ec);
			if (ec) {
				blog(LOG_ERROR, "Failed renaming corrupt file with %d", ec.value());
			}
		}

		blog(LOG_INFO, "No scene file found, creating default scene");

		bool hasFirstRun = config_get_bool(App()->GetUserConfig(), "General", "FirstRun");

		CreateDefaultScene(!hasFirstRun);
		SaveProject();
		return;
	}

	LoadData(data, file, remigrate);
}

static inline void AddMissingFiles(void *data, obs_source_t *source)
{
	obs_missing_files_t *f = (obs_missing_files_t *)data;
	obs_missing_files_t *sf = obs_source_get_missing_files(source);

	obs_missing_files_append(f, sf);
	obs_missing_files_destroy(sf);
}

static void ClearRelativePosCb(obs_data_t *data, void *)
{
	const string_view id = obs_data_get_string(data, "id");
	if (id != "scene" && id != "group")
		return;

	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
	OBSDataArrayAutoRelease items = obs_data_get_array(settings, "items");

	obs_data_array_enum(
		items,
		[](obs_data_t *data, void *) {
			obs_data_unset_user_value(data, "pos_rel");
			obs_data_unset_user_value(data, "scale_rel");
			obs_data_unset_user_value(data, "scale_ref");
			obs_data_unset_user_value(data, "bounds_rel");
		},
		nullptr);
}

void OBSBasic::LoadData(obs_data_t *data, const char *file, bool remigrate)
{
	ClearSceneData();
	ClearContextBar();

	/* Exit OBS if clearing scene data failed for some reason. */
	if (clearingFailed) {
		OBSMessageBox::critical(this, QTStr("SourceLeak.Title"), QTStr("SourceLeak.Text"));
		close();
		return;
	}

	InitDefaultTransitions();

	if (devicePropertiesThread && devicePropertiesThread->isRunning()) {
		devicePropertiesThread->wait();
		devicePropertiesThread.reset();
	}

	QApplication::sendPostedEvents(nullptr);

	OBSDataAutoRelease modulesObj = obs_data_get_obj(data, "modules");
	if (api)
		api->on_preload(modulesObj);

	/* Keep a reference to "modules" data so plugins that are not loaded do
	 * not have their collection specific data lost. */
	collectionModuleData = obs_data_get_obj(data, "modules");

	OBSDataArrayAutoRelease sceneOrder = obs_data_get_array(data, "scene_order");
	OBSDataArrayAutoRelease sources = obs_data_get_array(data, "sources");
	OBSDataArrayAutoRelease groups = obs_data_get_array(data, "groups");
	OBSDataArrayAutoRelease transitions = obs_data_get_array(data, "transitions");
	const char *sceneName = obs_data_get_string(data, "current_scene");
	const char *programSceneName = obs_data_get_string(data, "current_program_scene");
	const char *transitionName = obs_data_get_string(data, "current_transition");

	if (!opt_starting_scene.empty()) {
		programSceneName = opt_starting_scene.c_str();
		if (!IsPreviewProgramMode())
			sceneName = opt_starting_scene.c_str();
	}

	int newDuration = obs_data_get_int(data, "transition_duration");
	if (!newDuration)
		newDuration = 300;

	if (!transitionName)
		transitionName = obs_source_get_name(fadeTransition);

	const char *curSceneCollection = config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection");

	obs_data_set_default_string(data, "name", curSceneCollection);

	const char *name = obs_data_get_string(data, "name");
	OBSSourceAutoRelease curScene;
	OBSSourceAutoRelease curProgramScene;
	obs_source_t *curTransition;

	if (!name || !*name)
		name = curSceneCollection;

	LoadAudioDevice(DESKTOP_AUDIO_1, 1, data);
	LoadAudioDevice(DESKTOP_AUDIO_2, 2, data);
	LoadAudioDevice(AUX_AUDIO_1, 3, data);
	LoadAudioDevice(AUX_AUDIO_2, 4, data);
	LoadAudioDevice(AUX_AUDIO_3, 5, data);
	LoadAudioDevice(AUX_AUDIO_4, 6, data);

	if (!sources) {
		sources = std::move(groups);
	} else {
		obs_data_array_push_back_array(sources, groups);
	}

	/* Reset relative coordinate data if forcefully remigrating. */
	if (remigrate) {
		obs_data_set_int(data, "version", 1);
		obs_data_array_enum(sources, ClearRelativePosCb, nullptr);
	}

	bool resetVideo = false;
	bool disableRelativeCoords = false;
	obs_video_info ovi;

	int64_t version = obs_data_get_int(data, "version");
	OBSDataAutoRelease res = obs_data_get_obj(data, "resolution");
	if (res) {
		lastOutputResolution = {obs_data_get_int(res, "x"), obs_data_get_int(res, "y")};
	}

	/* Only migrate legacy collection if resolution is saved. */
	if (version < 2 && lastOutputResolution) {
		obs_get_video_info(&ovi);

		uint32_t width = obs_data_get_int(res, "x");
		uint32_t height = obs_data_get_int(res, "y");

		migrationBaseResolution = {width, height};

		if (ovi.base_height != height || ovi.base_width != width) {
			ovi.base_width = width;
			ovi.base_height = height;

			/* Attempt to reset to last known canvas resolution for migration. */
			resetVideo = obs_reset_video(&ovi) == OBS_VIDEO_SUCCESS;
			disableRelativeCoords = !resetVideo;
		}

		/* If migration is possible, and it wasn't forced, back up the original file. */
		if (!disableRelativeCoords && !remigrate) {
			auto path = filesystem::u8path(file);
			auto backupPath = path.concat(".v1");
			if (!filesystem::exists(backupPath)) {
				if (!obs_data_save_json_pretty_safe(data, backupPath.u8string().c_str(), "tmp", NULL)) {
					blog(LOG_WARNING,
					     "Failed to create a backup of existing scene collection data!");
				}
			}
		}
	} else if (version < 2) {
		disableRelativeCoords = true;
	} else if (OBSDataAutoRelease migration_res = obs_data_get_obj(data, "migration_resolution")) {
		migrationBaseResolution = {obs_data_get_int(migration_res, "x"), obs_data_get_int(migration_res, "y")};
	}

	DisableRelativeCoordinates(disableRelativeCoords);

	obs_missing_files_t *files = obs_missing_files_create();
	obs_load_sources(sources, AddMissingFiles, files);

	if (resetVideo)
		ResetVideo();
	if (transitions)
		LoadTransitions(transitions, AddMissingFiles, files);
	if (sceneOrder)
		LoadSceneListOrder(sceneOrder);

	curTransition = FindTransition(transitionName);
	if (!curTransition)
		curTransition = fadeTransition;

	ui->transitionDuration->setValue(newDuration);
	SetTransition(curTransition);

retryScene:
	curScene = obs_get_source_by_name(sceneName);
	curProgramScene = obs_get_source_by_name(programSceneName);

	/* if the starting scene command line parameter is bad at all,
	 * fall back to original settings */
	if (!opt_starting_scene.empty() && (!curScene || !curProgramScene)) {
		sceneName = obs_data_get_string(data, "current_scene");
		programSceneName = obs_data_get_string(data, "current_program_scene");
		opt_starting_scene.clear();
		goto retryScene;
	}

	if (!curScene) {
		auto find_scene_cb = [](void *source_ptr, obs_source_t *scene) {
			*static_cast<OBSSourceAutoRelease *>(source_ptr) = obs_source_get_ref(scene);
			return false;
		};
		obs_enum_scenes(find_scene_cb, &curScene);
	}

	SetCurrentScene(curScene.Get(), true);

	if (!curProgramScene)
		curProgramScene = std::move(curScene);
	if (IsPreviewProgramMode())
		TransitionToScene(curProgramScene.Get(), true);

	/* ------------------- */

	bool projectorSave = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SaveProjectors");

	if (projectorSave) {
		OBSDataArrayAutoRelease savedProjectors = obs_data_get_array(data, "saved_projectors");

		if (savedProjectors) {
			LoadSavedProjectors(savedProjectors);
			OpenSavedProjectors();
			activateWindow();
		}
	}

	/* ------------------- */

	std::string file_base = strrchr(file, '/') + 1;
	file_base.erase(file_base.size() - 5, 5);

	config_set_string(App()->GetUserConfig(), "Basic", "SceneCollection", name);
	config_set_string(App()->GetUserConfig(), "Basic", "SceneCollectionFile", file_base.c_str());

	OBSDataArrayAutoRelease quickTransitionData = obs_data_get_array(data, "quick_transitions");
	LoadQuickTransitions(quickTransitionData);

	RefreshQuickTransitions();

	bool previewLocked = obs_data_get_bool(data, "preview_locked");
	ui->preview->SetLocked(previewLocked);
	ui->actionLockPreview->setChecked(previewLocked);

	/* ---------------------- */

	bool fixedScaling = obs_data_get_bool(data, "scaling_enabled");
	int scalingLevel = (int)obs_data_get_int(data, "scaling_level");
	float scrollOffX = (float)obs_data_get_double(data, "scaling_off_x");
	float scrollOffY = (float)obs_data_get_double(data, "scaling_off_y");

	if (fixedScaling) {
		ui->preview->SetScalingLevel(scalingLevel);
		ui->preview->SetScrollingOffset(scrollOffX, scrollOffY);
	}
	ui->preview->SetFixedScaling(fixedScaling);

	emit ui->preview->DisplayResized();

	if (vcamEnabled) {
		OBSDataAutoRelease obj = obs_data_get_obj(data, "virtual-camera");

		vcamConfig.type = (VCamOutputType)obs_data_get_int(obj, "type2");
		if (vcamConfig.type == VCamOutputType::Invalid)
			vcamConfig.type = (VCamOutputType)obs_data_get_int(obj, "type");

		if (vcamConfig.type == VCamOutputType::Invalid) {
			VCamInternalType internal = (VCamInternalType)obs_data_get_int(obj, "internal");

			switch (internal) {
			case VCamInternalType::Default:
				vcamConfig.type = VCamOutputType::ProgramView;
				break;
			case VCamInternalType::Preview:
				vcamConfig.type = VCamOutputType::PreviewOutput;
				break;
			}
		}
		vcamConfig.scene = obs_data_get_string(obj, "scene");
		vcamConfig.source = obs_data_get_string(obj, "source");
	}

	if (obs_data_has_user_value(data, "resolution")) {
		OBSDataAutoRelease res = obs_data_get_obj(data, "resolution");
		if (obs_data_has_user_value(res, "x") && obs_data_has_user_value(res, "y")) {
			lastOutputResolution = {obs_data_get_int(res, "x"), obs_data_get_int(res, "y")};
		}
	}

	/* ---------------------- */

	if (api)
		api->on_load(modulesObj);

	obs_data_release(data);

	if (!opt_starting_scene.empty())
		opt_starting_scene.clear();

	if (opt_start_streaming && !safe_mode) {
		blog(LOG_INFO, "Starting stream due to command line parameter");
		QMetaObject::invokeMethod(this, "StartStreaming", Qt::QueuedConnection);
		opt_start_streaming = false;
	}

	if (opt_start_recording && !safe_mode) {
		blog(LOG_INFO, "Starting recording due to command line parameter");
		QMetaObject::invokeMethod(this, "StartRecording", Qt::QueuedConnection);
		opt_start_recording = false;
	}

	if (opt_start_replaybuffer && !safe_mode) {
		QMetaObject::invokeMethod(this, "StartReplayBuffer", Qt::QueuedConnection);
		opt_start_replaybuffer = false;
	}

	if (opt_start_virtualcam && !safe_mode) {
		QMetaObject::invokeMethod(this, "StartVirtualCam", Qt::QueuedConnection);
		opt_start_virtualcam = false;
	}

	LogScenes();

	if (!App()->IsMissingFilesCheckDisabled())
		ShowMissingFilesDialog(files);

	disableSaving--;

	if (vcamEnabled)
		outputHandler->UpdateVirtualCamOutputSource();

	OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
}

constexpr std::string_view OBSServiceFileName = "service.json";

void OBSBasic::SaveService()
{
	if (!service)
		return;

	const OBSProfile &currentProfile = GetCurrentProfile();

	const std::filesystem::path jsonFilePath = currentProfile.path / std::filesystem::u8path(OBSServiceFileName);

	OBSDataAutoRelease data = obs_data_create();
	OBSDataAutoRelease settings = obs_service_get_settings(service);

	obs_data_set_string(data, "type", obs_service_get_type(service));
	obs_data_set_obj(data, "settings", settings);

	if (!obs_data_save_json_safe(data, jsonFilePath.u8string().c_str(), "tmp", "bak")) {
		blog(LOG_WARNING, "Failed to save service");
	}
}

bool OBSBasic::LoadService()
{
	OBSDataAutoRelease data;

	try {
		const OBSProfile &currentProfile = GetCurrentProfile();

		const std::filesystem::path jsonFilePath =
			currentProfile.path / std::filesystem::u8path(OBSServiceFileName);

		data = obs_data_create_from_json_file_safe(jsonFilePath.u8string().c_str(), "bak");

		if (!data) {
			return false;
		}
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	}

	const char *type;
	obs_data_set_default_string(data, "type", "rtmp_common");
	type = obs_data_get_string(data, "type");

	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
	OBSDataAutoRelease hotkey_data = obs_data_get_obj(data, "hotkeys");

	service = obs_service_create(type, "default_service", settings, hotkey_data);
	obs_service_release(service);

	if (!service)
		return false;

	/* Enforce Opus on WHIP if needed */
	if (strcmp(obs_service_get_protocol(service), "WHIP") == 0) {
		const char *option = config_get_string(activeConfiguration, "SimpleOutput", "StreamAudioEncoder");
		if (strcmp(option, "opus") != 0)
			config_set_string(activeConfiguration, "SimpleOutput", "StreamAudioEncoder", "opus");

		option = config_get_string(activeConfiguration, "AdvOut", "AudioEncoder");

		const char *encoder_codec = obs_get_encoder_codec(option);
		if (!encoder_codec || strcmp(encoder_codec, "opus") != 0)
			config_set_string(activeConfiguration, "AdvOut", "AudioEncoder", "ffmpeg_opus");
	}

	return true;
}

bool OBSBasic::InitService()
{
	ProfileScope("OBSBasic::InitService");

	if (LoadService())
		return true;

	service = obs_service_create("rtmp_common", "default_service", nullptr, nullptr);
	if (!service)
		return false;
	obs_service_release(service);

	return true;
}

static const double scaled_vals[] = {1.0, 1.25, (1.0 / 0.75), 1.5, (1.0 / 0.6), 1.75, 2.0, 2.25, 2.5, 2.75, 3.0, 0.0};

extern void CheckExistingCookieId();

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

extern bool EncoderAvailable(const char *encoder);

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

	signalHandlers.reserve(signalHandlers.size() + 9);
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
}

void OBSBasic::InitPrimitives()
{
	ProfileScope("OBSBasic::InitPrimitives");

	obs_enter_graphics();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
	box = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	boxLeft = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(1.0f, 0.0f);
	boxTop = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
	boxRight = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f);
	boxBottom = gs_render_save();

	gs_render_start(true);
	for (int i = 0; i <= 360; i += (360 / 20)) {
		float pos = RAD(float(i));
		gs_vertex2f(cosf(pos), sinf(pos));
	}
	circle = gs_render_save();

	InitSafeAreas(&actionSafeMargin, &graphicsSafeMargin, &fourByThreeSafeMargin, &leftLine, &topLine, &rightLine);
	obs_leave_graphics();
}

void OBSBasic::ReplayBufferActionTriggered()
{
	if (outputHandler->ReplayBufferActive())
		StopReplayBuffer();
	else
		StartReplayBuffer();
};

void OBSBasic::ResetOutputs()
{
	ProfileScope("OBSBasic::ResetOutputs");

	const char *mode = config_get_string(activeConfiguration, "Output", "Mode");
	bool advOut = astrcmpi(mode, "Advanced") == 0;

	if ((!outputHandler || !outputHandler->Active()) &&
	    (!setupStreamingGuard.valid() ||
	     setupStreamingGuard.wait_for(std::chrono::seconds{0}) == std::future_status::ready)) {
		outputHandler.reset();
		outputHandler.reset(advOut ? CreateAdvancedOutputHandler(this) : CreateSimpleOutputHandler(this));

		emit ReplayBufEnabled(outputHandler->replayBuffer);

		if (sysTrayReplayBuffer)
			sysTrayReplayBuffer->setEnabled(!!outputHandler->replayBuffer);

		UpdateIsRecordingPausable();
	} else {
		outputHandler->Update();
	}
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
		const std::optional<OBSSceneCollection> configuredCollection =
			GetSceneCollectionByName(sceneCollectionName);
		const std::optional<OBSSceneCollection> foundCollection =
			GetSceneCollectionByName(opt_starting_collection);

		if (foundCollection) {
			ActivateSceneCollection(foundCollection.value());
		} else if (configuredCollection) {
			ActivateSceneCollection(configuredCollection.value());
		} else {
			disableSaving--;
			SetupNewSceneCollection(sceneCollectionName);
			disableSaving++;
		}

		disableSaving--;
		if (foundCollection || configuredCollection) {
			OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
			OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
		}
		OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
		OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
		disableSaving++;
	}

	loaded = true;

	previewEnabled = config_get_bool(App()->GetUserConfig(), "BasicWindow", "PreviewEnabled");

	if (!previewEnabled && !IsPreviewProgramMode())
		QMetaObject::invokeMethod(this, "EnablePreviewDisplay", Qt::QueuedConnection,
					  Q_ARG(bool, previewEnabled));
	else if (!previewEnabled && IsPreviewProgramMode())
		QMetaObject::invokeMethod(this, "EnablePreviewDisplay", Qt::QueuedConnection, Q_ARG(bool, true));

	disableSaving--;

	auto addDisplay = [this](OBSQTDisplay *window) {
		obs_display_add_draw_callback(window->GetDisplay(), OBSBasic::RenderMain, this);

		struct obs_video_info ovi;
		if (obs_get_video_info(&ovi))
			ResizePreview(ovi.base_width, ovi.base_height);
	};

	connect(ui->preview, &OBSQTDisplay::DisplayCreated, addDisplay);

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

	AddProjectorMenuMonitors(ui->multiviewProjectorMenu, this, &OBSBasic::OpenMultiviewProjector);
	connect(ui->viewMenu->menuAction(), &QAction::hovered, this, &OBSBasic::UpdateMultiviewProjectorMenu);

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

/* shows a "what's new" page on startup of new versions using CEF */
void OBSBasic::ReceivedIntroJson(const QString &text)
{
#ifdef WHATSNEW_ENABLED
	if (closing)
		return;

	WhatsNewList items;
	try {
		nlohmann::json json = nlohmann::json::parse(text.toStdString());
		items = json.get<WhatsNewList>();
	} catch (nlohmann::json::exception &e) {
		blog(LOG_WARNING, "Parsing whatsnew data failed: %s", e.what());
		return;
	}

	std::string info_url;
	int info_increment = -1;

	/* check to see if there's an info page for this version */
	for (const WhatsNewItem &item : items) {
		if (item.os) {
			WhatsNewPlatforms platforms = *item.os;
#ifdef _WIN32
			if (!platforms.windows)
				continue;
#elif defined(__APPLE__)
			if (!platforms.macos)
				continue;
#else
			if (!platforms.linux)
				continue;
#endif
		}

		int major = 0;
		int minor = 0;

		sscanf(item.version.c_str(), "%d.%d", &major, &minor);
		if (major == LIBOBS_API_MAJOR_VER && minor == LIBOBS_API_MINOR_VER &&
		    item.RC == OBS_RELEASE_CANDIDATE && item.Beta == OBS_BETA) {
			info_url = item.url;
			info_increment = item.increment;
		}
	}

	/* this version was not found, or no info for this version */
	if (info_increment == -1) {
		return;
	}

#if OBS_RELEASE_CANDIDATE > 0
	constexpr const char *lastInfoVersion = "InfoLastRCVersion";
#elif OBS_BETA > 0
	constexpr const char *lastInfoVersion = "InfoLastBetaVersion";
#else
	constexpr const char *lastInfoVersion = "InfoLastVersion";
#endif
	constexpr uint64_t currentVersion = (uint64_t)LIBOBS_API_VER << 16ULL | OBS_RELEASE_CANDIDATE << 8ULL |
					    OBS_BETA;
	uint64_t lastVersion = config_get_uint(App()->GetAppConfig(), "General", lastInfoVersion);
	int current_version_increment = -1;

	if ((lastVersion & ~0xFFFF0000ULL) < (currentVersion & ~0xFFFF0000ULL)) {
		config_set_int(App()->GetAppConfig(), "General", "InfoIncrement", -1);
		config_set_uint(App()->GetAppConfig(), "General", lastInfoVersion, currentVersion);
	} else {
		current_version_increment = config_get_int(App()->GetAppConfig(), "General", "InfoIncrement");
	}

	if (info_increment <= current_version_increment) {
		return;
	}

	config_set_int(App()->GetAppConfig(), "General", "InfoIncrement", info_increment);
	config_save_safe(App()->GetAppConfig(), "tmp", nullptr);

	cef->init_browser();

	WhatsNewBrowserInitThread *wnbit = new WhatsNewBrowserInitThread(QT_UTF8(info_url.c_str()));

	connect(wnbit, &WhatsNewBrowserInitThread::Result, this, &OBSBasic::ShowWhatsNew, Qt::QueuedConnection);

	whatsNewInitThread.reset(wnbit);
	whatsNewInitThread->start();

#else
	UNUSED_PARAMETER(text);
#endif
}

void OBSBasic::ShowWhatsNew(const QString &url)
{
#ifdef BROWSER_AVAILABLE
	if (closing)
		return;

	if (obsWhatsNew) {
		obsWhatsNew->close();
	}

	obsWhatsNew = new OBSWhatsNew(this, QT_TO_UTF8(url));
#else
	UNUSED_PARAMETER(url);
#endif
}

void OBSBasic::UpdateMultiviewProjectorMenu()
{
	ui->multiviewProjectorMenu->clear();
	AddProjectorMenuMonitors(ui->multiviewProjectorMenu, this, &OBSBasic::OpenMultiviewProjector);
}

void OBSBasic::InitHotkeys()
{
	ProfileScope("OBSBasic::InitHotkeys");

	struct obs_hotkeys_translations t = {};
	t.insert = Str("Hotkeys.Insert");
	t.del = Str("Hotkeys.Delete");
	t.home = Str("Hotkeys.Home");
	t.end = Str("Hotkeys.End");
	t.page_up = Str("Hotkeys.PageUp");
	t.page_down = Str("Hotkeys.PageDown");
	t.num_lock = Str("Hotkeys.NumLock");
	t.scroll_lock = Str("Hotkeys.ScrollLock");
	t.caps_lock = Str("Hotkeys.CapsLock");
	t.backspace = Str("Hotkeys.Backspace");
	t.tab = Str("Hotkeys.Tab");
	t.print = Str("Hotkeys.Print");
	t.pause = Str("Hotkeys.Pause");
	t.left = Str("Hotkeys.Left");
	t.right = Str("Hotkeys.Right");
	t.up = Str("Hotkeys.Up");
	t.down = Str("Hotkeys.Down");
#ifdef _WIN32
	t.meta = Str("Hotkeys.Windows");
#else
	t.meta = Str("Hotkeys.Super");
#endif
	t.menu = Str("Hotkeys.Menu");
	t.space = Str("Hotkeys.Space");
	t.numpad_num = Str("Hotkeys.NumpadNum");
	t.numpad_multiply = Str("Hotkeys.NumpadMultiply");
	t.numpad_divide = Str("Hotkeys.NumpadDivide");
	t.numpad_plus = Str("Hotkeys.NumpadAdd");
	t.numpad_minus = Str("Hotkeys.NumpadSubtract");
	t.numpad_decimal = Str("Hotkeys.NumpadDecimal");
	t.apple_keypad_num = Str("Hotkeys.AppleKeypadNum");
	t.apple_keypad_multiply = Str("Hotkeys.AppleKeypadMultiply");
	t.apple_keypad_divide = Str("Hotkeys.AppleKeypadDivide");
	t.apple_keypad_plus = Str("Hotkeys.AppleKeypadAdd");
	t.apple_keypad_minus = Str("Hotkeys.AppleKeypadSubtract");
	t.apple_keypad_decimal = Str("Hotkeys.AppleKeypadDecimal");
	t.apple_keypad_equal = Str("Hotkeys.AppleKeypadEqual");
	t.mouse_num = Str("Hotkeys.MouseButton");
	t.escape = Str("Hotkeys.Escape");
	obs_hotkeys_set_translations(&t);

	obs_hotkeys_set_audio_hotkeys_translations(Str("Mute"), Str("Unmute"), Str("Push-to-mute"),
						   Str("Push-to-talk"));

	obs_hotkeys_set_sceneitem_hotkeys_translations(Str("SceneItemShow"), Str("SceneItemHide"));

	obs_hotkey_enable_callback_rerouting(true);
	obs_hotkey_set_callback_routing_func(OBSBasic::HotkeyTriggered, this);
}

void OBSBasic::ProcessHotkey(obs_hotkey_id id, bool pressed)
{
	obs_hotkey_trigger_routed_callback(id, pressed);
}

void OBSBasic::HotkeyTriggered(void *data, obs_hotkey_id id, bool pressed)
{
	OBSBasic &basic = *static_cast<OBSBasic *>(data);
	QMetaObject::invokeMethod(&basic, "ProcessHotkey", Q_ARG(obs_hotkey_id, id), Q_ARG(bool, pressed));
}

void OBSBasic::CreateHotkeys()
{
	ProfileScope("OBSBasic::CreateHotkeys");

	auto LoadHotkeyData = [&](const char *name) -> OBSData {
		const char *info = config_get_string(activeConfiguration, "Hotkeys", name);
		if (!info)
			return {};

		OBSDataAutoRelease data = obs_data_create_from_json(info);
		if (!data)
			return {};

		return data.Get();
	};

	auto LoadHotkey = [&](obs_hotkey_id id, const char *name) {
		OBSDataArrayAutoRelease array = obs_data_get_array(LoadHotkeyData(name), "bindings");

		obs_hotkey_load(id, array);
	};

	auto LoadHotkeyPair = [&](obs_hotkey_pair_id id, const char *name0, const char *name1,
				  const char *oldName = NULL) {
		if (oldName) {
			const auto info = config_get_string(activeConfiguration, "Hotkeys", oldName);
			if (info) {
				config_set_string(activeConfiguration, "Hotkeys", name0, info);
				config_set_string(activeConfiguration, "Hotkeys", name1, info);
				config_remove_value(activeConfiguration, "Hotkeys", oldName);
				activeConfiguration.Save();
			}
		}
		OBSDataArrayAutoRelease array0 = obs_data_get_array(LoadHotkeyData(name0), "bindings");
		OBSDataArrayAutoRelease array1 = obs_data_get_array(LoadHotkeyData(name1), "bindings");

		obs_hotkey_pair_load(id, array0, array1);
	};

#define MAKE_CALLBACK(pred, method, log_action)                            \
	[](void *data, obs_hotkey_pair_id, obs_hotkey_t *, bool pressed) { \
		OBSBasic &basic = *static_cast<OBSBasic *>(data);          \
		if ((pred) && pressed) {                                   \
			blog(LOG_INFO, log_action " due to hotkey");       \
			method();                                          \
			return true;                                       \
		}                                                          \
		return false;                                              \
	}

	streamingHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartStreaming", Str("Basic.Main.StartStreaming"), "OBSBasic.StopStreaming",
		Str("Basic.Main.StopStreaming"),
		MAKE_CALLBACK(!basic.outputHandler->StreamingActive() && !basic.streamingStarting, basic.StartStreaming,
			      "Starting stream"),
		MAKE_CALLBACK(basic.outputHandler->StreamingActive() && !basic.streamingStarting, basic.StopStreaming,
			      "Stopping stream"),
		this, this);
	LoadHotkeyPair(streamingHotkeys, "OBSBasic.StartStreaming", "OBSBasic.StopStreaming");

	auto cb = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		OBSBasic &basic = *static_cast<OBSBasic *>(data);
		if (basic.outputHandler->StreamingActive() && pressed) {
			basic.ForceStopStreaming();
		}
	};

	forceStreamingStopHotkey = obs_hotkey_register_frontend("OBSBasic.ForceStopStreaming",
								Str("Basic.Main.ForceStopStreaming"), cb, this);
	LoadHotkey(forceStreamingStopHotkey, "OBSBasic.ForceStopStreaming");

	recordingHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartRecording", Str("Basic.Main.StartRecording"), "OBSBasic.StopRecording",
		Str("Basic.Main.StopRecording"),
		MAKE_CALLBACK(!basic.outputHandler->RecordingActive() && !basic.recordingStarted, basic.StartRecording,
			      "Starting recording"),
		MAKE_CALLBACK(basic.outputHandler->RecordingActive() && basic.recordingStarted, basic.StopRecording,
			      "Stopping recording"),
		this, this);
	LoadHotkeyPair(recordingHotkeys, "OBSBasic.StartRecording", "OBSBasic.StopRecording");

	pauseHotkeys =
		obs_hotkey_pair_register_frontend("OBSBasic.PauseRecording", Str("Basic.Main.PauseRecording"),
						  "OBSBasic.UnpauseRecording", Str("Basic.Main.UnpauseRecording"),
						  MAKE_CALLBACK(basic.isRecordingPausable && !basic.recordingPaused,
								basic.PauseRecording, "Pausing recording"),
						  MAKE_CALLBACK(basic.isRecordingPausable && basic.recordingPaused,
								basic.UnpauseRecording, "Unpausing recording"),
						  this, this);
	LoadHotkeyPair(pauseHotkeys, "OBSBasic.PauseRecording", "OBSBasic.UnpauseRecording");

	splitFileHotkey = obs_hotkey_register_frontend(
		"OBSBasic.SplitFile", Str("Basic.Main.SplitFile"),
		[](void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (pressed)
				obs_frontend_recording_split_file();
		},
		this);
	LoadHotkey(splitFileHotkey, "OBSBasic.SplitFile");

	addChapterHotkey = obs_hotkey_register_frontend(
		"OBSBasic.AddChapterMarker", Str("Basic.Main.AddChapterMarker"),
		[](void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (pressed)
				obs_frontend_recording_add_chapter(nullptr);
		},
		this);
	LoadHotkey(addChapterHotkey, "OBSBasic.AddChapterMarker");

	replayBufHotkeys =
		obs_hotkey_pair_register_frontend("OBSBasic.StartReplayBuffer", Str("Basic.Main.StartReplayBuffer"),
						  "OBSBasic.StopReplayBuffer", Str("Basic.Main.StopReplayBuffer"),
						  MAKE_CALLBACK(!basic.outputHandler->ReplayBufferActive(),
								basic.StartReplayBuffer, "Starting replay buffer"),
						  MAKE_CALLBACK(basic.outputHandler->ReplayBufferActive(),
								basic.StopReplayBuffer, "Stopping replay buffer"),
						  this, this);
	LoadHotkeyPair(replayBufHotkeys, "OBSBasic.StartReplayBuffer", "OBSBasic.StopReplayBuffer");

	if (vcamEnabled) {
		vcamHotkeys = obs_hotkey_pair_register_frontend(
			"OBSBasic.StartVirtualCam", Str("Basic.Main.StartVirtualCam"), "OBSBasic.StopVirtualCam",
			Str("Basic.Main.StopVirtualCam"),
			MAKE_CALLBACK(!basic.outputHandler->VirtualCamActive(), basic.StartVirtualCam,
				      "Starting virtual camera"),
			MAKE_CALLBACK(basic.outputHandler->VirtualCamActive(), basic.StopVirtualCam,
				      "Stopping virtual camera"),
			this, this);
		LoadHotkeyPair(vcamHotkeys, "OBSBasic.StartVirtualCam", "OBSBasic.StopVirtualCam");
	}

	togglePreviewHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.EnablePreview", Str("Basic.Main.PreviewConextMenu.Enable"), "OBSBasic.DisablePreview",
		Str("Basic.Main.Preview.Disable"),
		MAKE_CALLBACK(!basic.previewEnabled, basic.EnablePreview, "Enabling preview"),
		MAKE_CALLBACK(basic.previewEnabled, basic.DisablePreview, "Disabling preview"), this, this);
	LoadHotkeyPair(togglePreviewHotkeys, "OBSBasic.EnablePreview", "OBSBasic.DisablePreview");

	togglePreviewProgramHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.EnablePreviewProgram", Str("Basic.EnablePreviewProgramMode"),
		"OBSBasic.DisablePreviewProgram", Str("Basic.DisablePreviewProgramMode"),
		MAKE_CALLBACK(!basic.IsPreviewProgramMode(), basic.EnablePreviewProgram, "Enabling preview program"),
		MAKE_CALLBACK(basic.IsPreviewProgramMode(), basic.DisablePreviewProgram, "Disabling preview program"),
		this, this);
	LoadHotkeyPair(togglePreviewProgramHotkeys, "OBSBasic.EnablePreviewProgram", "OBSBasic.DisablePreviewProgram",
		       "OBSBasic.TogglePreviewProgram");

	contextBarHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.ShowContextBar", Str("Basic.Main.ShowContextBar"), "OBSBasic.HideContextBar",
		Str("Basic.Main.HideContextBar"),
		MAKE_CALLBACK(!basic.ui->contextContainer->isVisible(), basic.ShowContextBar, "Showing Context Bar"),
		MAKE_CALLBACK(basic.ui->contextContainer->isVisible(), basic.HideContextBar, "Hiding Context Bar"),
		this, this);
	LoadHotkeyPair(contextBarHotkeys, "OBSBasic.ShowContextBar", "OBSBasic.HideContextBar");
#undef MAKE_CALLBACK

	auto transition = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed)
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "TransitionClicked",
						  Qt::QueuedConnection);
	};

	transitionHotkey = obs_hotkey_register_frontend("OBSBasic.Transition", Str("Transition"), transition, this);
	LoadHotkey(transitionHotkey, "OBSBasic.Transition");

	auto resetStats = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed)
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "ResetStatsHotkey",
						  Qt::QueuedConnection);
	};

	statsHotkey =
		obs_hotkey_register_frontend("OBSBasic.ResetStats", Str("Basic.Stats.ResetStats"), resetStats, this);
	LoadHotkey(statsHotkey, "OBSBasic.ResetStats");

	auto screenshot = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed)
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "Screenshot", Qt::QueuedConnection);
	};

	screenshotHotkey = obs_hotkey_register_frontend("OBSBasic.Screenshot", Str("Screenshot"), screenshot, this);
	LoadHotkey(screenshotHotkey, "OBSBasic.Screenshot");

	auto screenshotSource = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed)
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "ScreenshotSelectedSource",
						  Qt::QueuedConnection);
	};

	sourceScreenshotHotkey = obs_hotkey_register_frontend("OBSBasic.SelectedSourceScreenshot",
							      Str("Screenshot.SourceHotkey"), screenshotSource, this);
	LoadHotkey(sourceScreenshotHotkey, "OBSBasic.SelectedSourceScreenshot");
}

void OBSBasic::ClearHotkeys()
{
	obs_hotkey_pair_unregister(streamingHotkeys);
	obs_hotkey_pair_unregister(recordingHotkeys);
	obs_hotkey_pair_unregister(pauseHotkeys);
	obs_hotkey_unregister(splitFileHotkey);
	obs_hotkey_unregister(addChapterHotkey);
	obs_hotkey_pair_unregister(replayBufHotkeys);
	obs_hotkey_pair_unregister(vcamHotkeys);
	obs_hotkey_pair_unregister(togglePreviewHotkeys);
	obs_hotkey_pair_unregister(contextBarHotkeys);
	obs_hotkey_pair_unregister(togglePreviewProgramHotkeys);
	obs_hotkey_unregister(forceStreamingStopHotkey);
	obs_hotkey_unregister(transitionHotkey);
	obs_hotkey_unregister(statsHotkey);
	obs_hotkey_unregister(screenshotHotkey);
	obs_hotkey_unregister(sourceScreenshotHotkey);
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

void OBSBasic::SaveProjectNow()
{
	if (disableSaving)
		return;

	projectChanged = true;
	SaveProjectDeferred();
}

void OBSBasic::SaveProject()
{
	if (disableSaving)
		return;

	projectChanged = true;
	QMetaObject::invokeMethod(this, "SaveProjectDeferred", Qt::QueuedConnection);
}

void OBSBasic::SaveProjectDeferred()
{
	if (disableSaving)
		return;

	if (!projectChanged)
		return;

	projectChanged = false;

	try {
		const OBSSceneCollection &currentCollection = GetCurrentSceneCollection();

		Save(currentCollection.collectionFile.u8string().c_str());
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

OBSSource OBSBasic::GetProgramSource()
{
	return OBSGetStrongRef(programScene);
}

OBSScene OBSBasic::GetCurrentScene()
{
	return currentScene.load();
}

OBSSceneItem OBSBasic::GetSceneItem(QListWidgetItem *item)
{
	return item ? GetOBSRef<OBSSceneItem>(item) : nullptr;
}

OBSSceneItem OBSBasic::GetCurrentSceneItem()
{
	return ui->sources->Get(GetTopSelectedSourceItem());
}

void OBSBasic::UpdatePreviewScalingMenu()
{
	bool fixedScaling = ui->preview->IsFixedScaling();
	float scalingAmount = ui->preview->GetScalingAmount();
	if (!fixedScaling) {
		ui->actionScaleWindow->setChecked(true);
		ui->actionScaleCanvas->setChecked(false);
		ui->actionScaleOutput->setChecked(false);
		return;
	}

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	ui->actionScaleWindow->setChecked(false);
	ui->actionScaleCanvas->setChecked(scalingAmount == 1.0f);
	ui->actionScaleOutput->setChecked(scalingAmount == float(ovi.output_width) / float(ovi.base_width));
}

void OBSBasic::CreateInteractionWindow(obs_source_t *source)
{
	bool closed = true;
	if (interaction)
		closed = interaction->close();

	if (!closed)
		return;

	interaction = new OBSBasicInteraction(this, source);
	interaction->Init();
	interaction->setAttribute(Qt::WA_DeleteOnClose, true);
}

void OBSBasic::CreatePropertiesWindow(obs_source_t *source)
{
	bool closed = true;
	if (properties)
		closed = properties->close();

	if (!closed)
		return;

	properties = new OBSBasicProperties(this, source);
	properties->Init();
	properties->setAttribute(Qt::WA_DeleteOnClose, true);
}

void OBSBasic::CreateFiltersWindow(obs_source_t *source)
{
	bool closed = true;
	if (filters)
		closed = filters->close();

	if (!closed)
		return;

	filters = new OBSBasicFilters(this, source);
	filters->Init();
	filters->setAttribute(Qt::WA_DeleteOnClose, true);
}

/* Qt callbacks for invokeMethod */

void OBSBasic::AddScene(OBSSource source)
{
	const char *name = obs_source_get_name(source);
	obs_scene_t *scene = obs_scene_from_source(source);

	QListWidgetItem *item = new QListWidgetItem(QT_UTF8(name));
	SetOBSRef(item, OBSScene(scene));
	ui->scenes->insertItem(ui->scenes->currentRow() + 1, item);

	obs_hotkey_register_source(
		source, "OBSBasic.SelectScene", Str("Basic.Hotkeys.SelectScene"),
		[](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

			auto potential_source = static_cast<obs_source_t *>(data);
			OBSSourceAutoRelease source = obs_source_get_ref(potential_source);
			if (source && pressed)
				main->SetCurrentScene(source.Get());
		},
		static_cast<obs_source_t *>(source));

	signal_handler_t *handler = obs_source_get_signal_handler(source);

	SignalContainer<OBSScene> container;
	container.ref = scene;
	container.handlers.assign({
		std::make_shared<OBSSignal>(handler, "item_add", OBSBasic::SceneItemAdded, this),
		std::make_shared<OBSSignal>(handler, "reorder", OBSBasic::SceneReordered, this),
		std::make_shared<OBSSignal>(handler, "refresh", OBSBasic::SceneRefreshed, this),
	});

	item->setData(static_cast<int>(QtDataRole::OBSSignals), QVariant::fromValue(container));

	/* if the scene already has items (a duplicated scene) add them */
	auto addSceneItem = [this](obs_sceneitem_t *item) {
		AddSceneItem(item);
	};

	using addSceneItem_t = decltype(addSceneItem);

	obs_scene_enum_items(
		scene,
		[](obs_scene_t *, obs_sceneitem_t *item, void *param) {
			addSceneItem_t *func;
			func = reinterpret_cast<addSceneItem_t *>(param);
			(*func)(item);
			return true;
		},
		&addSceneItem);

	SaveProject();

	if (!disableSaving) {
		obs_source_t *source = obs_scene_get_source(scene);
		blog(LOG_INFO, "User added scene '%s'", obs_source_get_name(source));

		OBSProjector::UpdateMultiviewProjectors();
	}

	OnEvent(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED);
}

void OBSBasic::RemoveScene(OBSSource source)
{
	obs_scene_t *scene = obs_scene_from_source(source);

	QListWidgetItem *sel = nullptr;
	int count = ui->scenes->count();

	for (int i = 0; i < count; i++) {
		auto item = ui->scenes->item(i);
		auto cur_scene = GetOBSRef<OBSScene>(item);
		if (cur_scene != scene)
			continue;

		sel = item;
		break;
	}

	if (sel != nullptr) {
		if (sel == ui->scenes->currentItem())
			ui->sources->Clear();
		delete sel;
	}

	SaveProject();

	if (!disableSaving) {
		blog(LOG_INFO, "User Removed scene '%s'", obs_source_get_name(source));

		OBSProjector::UpdateMultiviewProjectors();
	}

	OnEvent(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED);
}

static bool select_one(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	obs_sceneitem_t *selectedItem = reinterpret_cast<obs_sceneitem_t *>(param);
	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, select_one, param);

	obs_sceneitem_select(item, (selectedItem == item));

	return true;
}

void OBSBasic::AddSceneItem(OBSSceneItem item)
{
	obs_scene_t *scene = obs_sceneitem_get_scene(item);

	if (GetCurrentScene() == scene)
		ui->sources->Add(item);

	SaveProject();

	if (!disableSaving) {
		obs_source_t *sceneSource = obs_scene_get_source(scene);
		obs_source_t *itemSource = obs_sceneitem_get_source(item);
		blog(LOG_INFO, "User added source '%s' (%s) to scene '%s'", obs_source_get_name(itemSource),
		     obs_source_get_id(itemSource), obs_source_get_name(sceneSource));

		obs_scene_enum_items(scene, select_one, (obs_sceneitem_t *)item);
	}
}

static void RenameListValues(QListWidget *listWidget, const QString &newName, const QString &prevName)
{
	QList<QListWidgetItem *> items = listWidget->findItems(prevName, Qt::MatchExactly);

	for (int i = 0; i < items.count(); i++)
		items[i]->setText(newName);
}

void OBSBasic::RenameSources(OBSSource source, QString newName, QString prevName)
{
	RenameListValues(ui->scenes, newName, prevName);

	if (vcamConfig.type == VCamOutputType::SourceOutput && prevName == QString::fromStdString(vcamConfig.source))
		vcamConfig.source = newName.toStdString();
	if (vcamConfig.type == VCamOutputType::SceneOutput && prevName == QString::fromStdString(vcamConfig.scene))
		vcamConfig.scene = newName.toStdString();

	SaveProject();

	obs_scene_t *scene = obs_scene_from_source(source);
	if (scene)
		OBSProjector::UpdateMultiviewProjectors();

	UpdateContextBar();
	UpdatePreviewProgramIndicators();
}

void OBSBasic::ClearContextBar()
{
	QLayoutItem *la = ui->emptySpace->layout()->itemAt(0);
	if (la) {
		delete la->widget();
		ui->emptySpace->layout()->removeItem(la);
	}
}

void OBSBasic::UpdateContextBarVisibility()
{
	int width = ui->centralwidget->size().width();

	ContextBarSize contextBarSizeNew;
	if (width >= 740) {
		contextBarSizeNew = ContextBarSize_Normal;
	} else if (width >= 600) {
		contextBarSizeNew = ContextBarSize_Reduced;
	} else {
		contextBarSizeNew = ContextBarSize_Minimized;
	}

	if (contextBarSize == contextBarSizeNew)
		return;

	contextBarSize = contextBarSizeNew;
	UpdateContextBarDeferred();
}

static bool is_network_media_source(obs_source_t *source, const char *id)
{
	if (strcmp(id, "ffmpeg_source") != 0)
		return false;

	OBSDataAutoRelease s = obs_source_get_settings(source);
	bool is_local_file = obs_data_get_bool(s, "is_local_file");

	return !is_local_file;
}

void OBSBasic::UpdateContextBarDeferred(bool force)
{
	QMetaObject::invokeMethod(this, "UpdateContextBar", Qt::QueuedConnection, Q_ARG(bool, force));
}

void OBSBasic::SourceToolBarActionsSetEnabled()
{
	bool enable = false;
	bool disableProps = false;

	OBSSceneItem item = GetCurrentSceneItem();

	if (item) {
		OBSSource source = obs_sceneitem_get_source(item);
		disableProps = !obs_source_configurable(source);

		enable = true;
	}

	if (disableProps)
		ui->actionSourceProperties->setEnabled(false);
	else
		ui->actionSourceProperties->setEnabled(enable);

	ui->actionRemoveSource->setEnabled(enable);
	ui->actionSourceUp->setEnabled(enable);
	ui->actionSourceDown->setEnabled(enable);

	RefreshToolBarStyling(ui->sourcesToolbar);
}

void OBSBasic::UpdateContextBar(bool force)
{
	SourceToolBarActionsSetEnabled();

	if (!ui->contextContainer->isVisible() && !force)
		return;

	OBSSceneItem item = GetCurrentSceneItem();

	if (item) {
		obs_source_t *source = obs_sceneitem_get_source(item);

		bool updateNeeded = true;
		QLayoutItem *la = ui->emptySpace->layout()->itemAt(0);
		if (la) {
			if (SourceToolbar *toolbar = dynamic_cast<SourceToolbar *>(la->widget())) {
				if (toolbar->GetSource() == source)
					updateNeeded = false;
			} else if (MediaControls *toolbar = dynamic_cast<MediaControls *>(la->widget())) {
				if (toolbar->GetSource() == source)
					updateNeeded = false;
			}
		}

		const char *id = obs_source_get_unversioned_id(source);
		uint32_t flags = obs_source_get_output_flags(source);

		ui->sourceInteractButton->setVisible(flags & OBS_SOURCE_INTERACTION);

		if (contextBarSize >= ContextBarSize_Reduced && (updateNeeded || force)) {
			ClearContextBar();
			if (flags & OBS_SOURCE_CONTROLLABLE_MEDIA) {
				if (!is_network_media_source(source, id)) {
					MediaControls *mediaControls = new MediaControls(ui->emptySpace);
					mediaControls->SetSource(source);

					ui->emptySpace->layout()->addWidget(mediaControls);
				}
			} else if (strcmp(id, "browser_source") == 0) {
				BrowserToolbar *c = new BrowserToolbar(ui->emptySpace, source);
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "wasapi_input_capture") == 0 ||
				   strcmp(id, "wasapi_output_capture") == 0 ||
				   strcmp(id, "coreaudio_input_capture") == 0 ||
				   strcmp(id, "coreaudio_output_capture") == 0 ||
				   strcmp(id, "pulse_input_capture") == 0 || strcmp(id, "pulse_output_capture") == 0 ||
				   strcmp(id, "alsa_input_capture") == 0) {
				AudioCaptureToolbar *c = new AudioCaptureToolbar(ui->emptySpace, source);
				c->Init();
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "wasapi_process_output_capture") == 0) {
				ApplicationAudioCaptureToolbar *c =
					new ApplicationAudioCaptureToolbar(ui->emptySpace, source);
				c->Init();
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "window_capture") == 0 || strcmp(id, "xcomposite_input") == 0) {
				WindowCaptureToolbar *c = new WindowCaptureToolbar(ui->emptySpace, source);
				c->Init();
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "monitor_capture") == 0 || strcmp(id, "display_capture") == 0 ||
				   strcmp(id, "xshm_input") == 0) {
				DisplayCaptureToolbar *c = new DisplayCaptureToolbar(ui->emptySpace, source);
				c->Init();
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "dshow_input") == 0) {
				DeviceCaptureToolbar *c = new DeviceCaptureToolbar(ui->emptySpace, source);
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "game_capture") == 0) {
				GameCaptureToolbar *c = new GameCaptureToolbar(ui->emptySpace, source);
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "image_source") == 0) {
				ImageSourceToolbar *c = new ImageSourceToolbar(ui->emptySpace, source);
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "color_source") == 0) {
				ColorSourceToolbar *c = new ColorSourceToolbar(ui->emptySpace, source);
				ui->emptySpace->layout()->addWidget(c);

			} else if (strcmp(id, "text_ft2_source") == 0 || strcmp(id, "text_gdiplus") == 0) {
				TextSourceToolbar *c = new TextSourceToolbar(ui->emptySpace, source);
				ui->emptySpace->layout()->addWidget(c);
			}
		} else if (contextBarSize == ContextBarSize_Minimized) {
			ClearContextBar();
		}

		QIcon icon;

		if (strcmp(id, "scene") == 0)
			icon = GetSceneIcon();
		else if (strcmp(id, "group") == 0)
			icon = GetGroupIcon();
		else
			icon = GetSourceIcon(id);

		QPixmap pixmap = icon.pixmap(QSize(16, 16));
		ui->contextSourceIcon->setPixmap(pixmap);
		ui->contextSourceIconSpacer->hide();
		ui->contextSourceIcon->show();

		const char *name = obs_source_get_name(source);
		ui->contextSourceLabel->setText(name);

		ui->sourceFiltersButton->setEnabled(true);
		ui->sourcePropertiesButton->setEnabled(obs_source_configurable(source));
	} else {
		ClearContextBar();
		ui->contextSourceIcon->hide();
		ui->contextSourceIconSpacer->show();
		ui->contextSourceLabel->setText(QTStr("ContextBar.NoSelectedSource"));

		ui->sourceFiltersButton->setEnabled(false);
		ui->sourcePropertiesButton->setEnabled(false);
		ui->sourceInteractButton->setVisible(false);
	}

	if (contextBarSize == ContextBarSize_Normal) {
		ui->sourcePropertiesButton->setText(QTStr("Properties"));
		ui->sourceFiltersButton->setText(QTStr("Filters"));
		ui->sourceInteractButton->setText(QTStr("Interact"));
	} else {
		ui->sourcePropertiesButton->setText("");
		ui->sourceFiltersButton->setText("");
		ui->sourceInteractButton->setText("");
	}
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

void OBSBasic::GetAudioSourceFilters()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *source = vol->GetSource();

	CreateFiltersWindow(source);
}

void OBSBasic::GetAudioSourceProperties()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *source = vol->GetSource();

	CreatePropertiesWindow(source);
}

void OBSBasic::HideAudioControl()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *source = vol->GetSource();

	if (!SourceMixerHidden(source)) {
		SetSourceMixerHidden(source, true);
		DeactivateAudioSource(source);
	}
}

void OBSBasic::UnhideAllAudioControls()
{
	auto UnhideAudioMixer = [this](obs_source_t *source) /* -- */
	{
		if (!obs_source_active(source))
			return true;
		if (!SourceMixerHidden(source))
			return true;

		SetSourceMixerHidden(source, false);
		ActivateAudioSource(source);
		return true;
	};

	using UnhideAudioMixer_t = decltype(UnhideAudioMixer);

	auto PreEnum = [](void *data, obs_source_t *source) -> bool /* -- */
	{
		return (*reinterpret_cast<UnhideAudioMixer_t *>(data))(source);
	};

	obs_enum_sources(PreEnum, &UnhideAudioMixer);
}

void OBSBasic::ToggleHideMixer()
{
	OBSSceneItem item = GetCurrentSceneItem();
	OBSSource source = obs_sceneitem_get_source(item);

	if (!SourceMixerHidden(source)) {
		SetSourceMixerHidden(source, true);
		DeactivateAudioSource(source);
	} else {
		SetSourceMixerHidden(source, false);
		ActivateAudioSource(source);
	}
}

void OBSBasic::MixerRenameSource()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	OBSSource source = vol->GetSource();

	const char *prevName = obs_source_get_name(source);

	for (;;) {
		string name;
		bool accepted = NameDialog::AskForName(this, QTStr("Basic.Main.MixerRename.Title"),
						       QTStr("Basic.Main.MixerRename.Text"), name, QT_UTF8(prevName));
		if (!accepted)
			return;

		if (name.empty()) {
			OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
			continue;
		}

		OBSSourceAutoRelease sourceTest = obs_get_source_by_name(name.c_str());

		if (sourceTest) {
			OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));
			continue;
		}

		obs_source_set_name(source, name.c_str());
		break;
	}
}

static inline bool SourceVolumeLocked(obs_source_t *source)
{
	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	bool lock = obs_data_get_bool(priv_settings, "volume_locked");

	return lock;
}

void OBSBasic::LockVolumeControl(bool lock)
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *source = vol->GetSource();

	OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
	obs_data_set_bool(priv_settings, "volume_locked", lock);

	vol->EnableSlider(!lock);
}

void OBSBasic::VolControlContextMenu()
{
	VolControl *vol = reinterpret_cast<VolControl *>(sender());

	/* ------------------- */

	QAction lockAction(QTStr("LockVolume"), this);
	lockAction.setCheckable(true);
	lockAction.setChecked(SourceVolumeLocked(vol->GetSource()));

	QAction hideAction(QTStr("Hide"), this);
	QAction unhideAllAction(QTStr("UnhideAll"), this);
	QAction mixerRenameAction(QTStr("Rename"), this);

	QAction copyFiltersAction(QTStr("Copy.Filters"), this);
	QAction pasteFiltersAction(QTStr("Paste.Filters"), this);

	QAction filtersAction(QTStr("Filters"), this);
	QAction propertiesAction(QTStr("Properties"), this);
	QAction advPropAction(QTStr("Basic.MainMenu.Edit.AdvAudio"), this);

	QAction toggleControlLayoutAction(QTStr("VerticalLayout"), this);
	toggleControlLayoutAction.setCheckable(true);
	toggleControlLayoutAction.setChecked(
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl"));

	/* ------------------- */

	connect(&hideAction, &QAction::triggered, this, &OBSBasic::HideAudioControl, Qt::DirectConnection);
	connect(&unhideAllAction, &QAction::triggered, this, &OBSBasic::UnhideAllAudioControls, Qt::DirectConnection);
	connect(&lockAction, &QAction::toggled, this, &OBSBasic::LockVolumeControl, Qt::DirectConnection);
	connect(&mixerRenameAction, &QAction::triggered, this, &OBSBasic::MixerRenameSource, Qt::DirectConnection);

	connect(&copyFiltersAction, &QAction::triggered, this, &OBSBasic::AudioMixerCopyFilters, Qt::DirectConnection);
	connect(&pasteFiltersAction, &QAction::triggered, this, &OBSBasic::AudioMixerPasteFilters,
		Qt::DirectConnection);

	connect(&filtersAction, &QAction::triggered, this, &OBSBasic::GetAudioSourceFilters, Qt::DirectConnection);
	connect(&propertiesAction, &QAction::triggered, this, &OBSBasic::GetAudioSourceProperties,
		Qt::DirectConnection);
	connect(&advPropAction, &QAction::triggered, this, &OBSBasic::on_actionAdvAudioProperties_triggered,
		Qt::DirectConnection);

	/* ------------------- */

	connect(&toggleControlLayoutAction, &QAction::changed, this, &OBSBasic::ToggleVolControlLayout,
		Qt::DirectConnection);

	/* ------------------- */

	hideAction.setProperty("volControl", QVariant::fromValue<VolControl *>(vol));
	lockAction.setProperty("volControl", QVariant::fromValue<VolControl *>(vol));
	mixerRenameAction.setProperty("volControl", QVariant::fromValue<VolControl *>(vol));

	copyFiltersAction.setProperty("volControl", QVariant::fromValue<VolControl *>(vol));
	pasteFiltersAction.setProperty("volControl", QVariant::fromValue<VolControl *>(vol));

	filtersAction.setProperty("volControl", QVariant::fromValue<VolControl *>(vol));
	propertiesAction.setProperty("volControl", QVariant::fromValue<VolControl *>(vol));

	/* ------------------- */

	copyFiltersAction.setEnabled(obs_source_filter_count(vol->GetSource()) > 0);
	pasteFiltersAction.setEnabled(!obs_weak_source_expired(copyFiltersSource));

	QMenu popup;
	vol->SetContextMenu(&popup);
	popup.addAction(&lockAction);
	popup.addSeparator();
	popup.addAction(&unhideAllAction);
	popup.addAction(&hideAction);
	popup.addAction(&mixerRenameAction);
	popup.addSeparator();
	popup.addAction(&copyFiltersAction);
	popup.addAction(&pasteFiltersAction);
	popup.addSeparator();
	popup.addAction(&toggleControlLayoutAction);
	popup.addSeparator();
	popup.addAction(&filtersAction);
	popup.addAction(&propertiesAction);
	popup.addAction(&advPropAction);

	// toggleControlLayoutAction deletes and re-creates the volume controls
	// meaning that "vol" would be pointing to freed memory.
	if (popup.exec(QCursor::pos()) != &toggleControlLayoutAction)
		vol->SetContextMenu(nullptr);
}

void OBSBasic::on_hMixerScrollArea_customContextMenuRequested()
{
	StackedMixerAreaContextMenuRequested();
}

void OBSBasic::on_vMixerScrollArea_customContextMenuRequested()
{
	StackedMixerAreaContextMenuRequested();
}

void OBSBasic::StackedMixerAreaContextMenuRequested()
{
	QAction unhideAllAction(QTStr("UnhideAll"), this);

	QAction advPropAction(QTStr("Basic.MainMenu.Edit.AdvAudio"), this);

	QAction toggleControlLayoutAction(QTStr("VerticalLayout"), this);
	toggleControlLayoutAction.setCheckable(true);
	toggleControlLayoutAction.setChecked(
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl"));

	/* ------------------- */

	connect(&unhideAllAction, &QAction::triggered, this, &OBSBasic::UnhideAllAudioControls, Qt::DirectConnection);

	connect(&advPropAction, &QAction::triggered, this, &OBSBasic::on_actionAdvAudioProperties_triggered,
		Qt::DirectConnection);

	/* ------------------- */

	connect(&toggleControlLayoutAction, &QAction::changed, this, &OBSBasic::ToggleVolControlLayout,
		Qt::DirectConnection);

	/* ------------------- */

	QMenu popup;
	popup.addAction(&unhideAllAction);
	popup.addSeparator();
	popup.addAction(&toggleControlLayoutAction);
	popup.addSeparator();
	popup.addAction(&advPropAction);
	popup.exec(QCursor::pos());
}

void OBSBasic::ToggleMixerLayout(bool vertical)
{
	if (vertical) {
		ui->stackedMixerArea->setMinimumSize(180, 220);
		ui->stackedMixerArea->setCurrentIndex(1);
	} else {
		ui->stackedMixerArea->setMinimumSize(220, 0);
		ui->stackedMixerArea->setCurrentIndex(0);
	}
}

void OBSBasic::ToggleVolControlLayout()
{
	bool vertical = !config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl", vertical);
	ToggleMixerLayout(vertical);

	// We need to store it so we can delete current and then add
	// at the right order
	vector<OBSSource> sources;
	for (const auto &vol : volumes)
		sources.emplace_back(vol->GetSource());

	ClearVolumeControls();

	for (const auto &source : sources)
		ActivateAudioSource(source);
}

void OBSBasic::ActivateAudioSource(OBSSource source)
{
	if (SourceMixerHidden(source))
		return;
	if (!obs_source_active(source))
		return;
	if (!obs_source_audio_active(source))
		return;

	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");
	VolControl *vol = new VolControl(source, true, vertical);

	vol->EnableSlider(!SourceVolumeLocked(source));

	double meterDecayRate = config_get_double(activeConfiguration, "Audio", "MeterDecayRate");
	vol->SetMeterDecayRate(meterDecayRate);

	uint32_t peakMeterTypeIdx = config_get_uint(activeConfiguration, "Audio", "PeakMeterType");

	enum obs_peak_meter_type peakMeterType;
	switch (peakMeterTypeIdx) {
	case 0:
		peakMeterType = SAMPLE_PEAK_METER;
		break;
	case 1:
		peakMeterType = TRUE_PEAK_METER;
		break;
	default:
		peakMeterType = SAMPLE_PEAK_METER;
		break;
	}

	vol->setPeakMeterType(peakMeterType);

	vol->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(vol, &QWidget::customContextMenuRequested, this, &OBSBasic::VolControlContextMenu);
	connect(vol, &VolControl::ConfigClicked, this, &OBSBasic::VolControlContextMenu);

	InsertQObjectByName(volumes, vol);

	for (auto volume : volumes) {
		if (vertical)
			ui->vVolControlLayout->addWidget(volume);
		else
			ui->hVolControlLayout->addWidget(volume);
	}
}

void OBSBasic::DeactivateAudioSource(OBSSource source)
{
	for (size_t i = 0; i < volumes.size(); i++) {
		if (volumes[i]->GetSource() == source) {
			delete volumes[i];
			volumes.erase(volumes.begin() + i);
			break;
		}
	}
}

bool OBSBasic::QueryRemoveSource(obs_source_t *source)
{
	if (obs_source_get_type(source) == OBS_SOURCE_TYPE_SCENE && !obs_source_is_group(source)) {
		int count = ui->scenes->count();

		if (count == 1) {
			OBSMessageBox::information(this, QTStr("FinalScene.Title"), QTStr("FinalScene.Text"));
			return false;
		}
	}

	const char *name = obs_source_get_name(source);

	QString text = QTStr("ConfirmRemove.Text").arg(QT_UTF8(name));

	QMessageBox remove_source(this);
	remove_source.setText(text);
	QPushButton *Yes = remove_source.addButton(QTStr("Yes"), QMessageBox::YesRole);
	remove_source.setDefaultButton(Yes);
	remove_source.addButton(QTStr("No"), QMessageBox::NoRole);
	remove_source.setIcon(QMessageBox::Question);
	remove_source.setWindowTitle(QTStr("ConfirmRemove.Title"));
	remove_source.exec();

	return Yes == remove_source.clickedButton();
}

#define UPDATE_CHECK_INTERVAL (60 * 60 * 24 * 4) /* 4 days */

void OBSBasic::TimedCheckForUpdates()
{
	if (App()->IsUpdaterDisabled())
		return;
	if (!config_get_bool(App()->GetAppConfig(), "General", "EnableAutoUpdates"))
		return;

#if defined(ENABLE_SPARKLE_UPDATER)
	CheckForUpdates(false);
#elif _WIN32
	long long lastUpdate = config_get_int(App()->GetAppConfig(), "General", "LastUpdateCheck");
	uint32_t lastVersion = config_get_int(App()->GetAppConfig(), "General", "LastVersion");

	if (lastVersion < LIBOBS_API_VER) {
		lastUpdate = 0;
		config_set_int(App()->GetAppConfig(), "General", "LastUpdateCheck", 0);
	}

	long long t = (long long)time(nullptr);
	long long secs = t - lastUpdate;

	if (secs > UPDATE_CHECK_INTERVAL)
		CheckForUpdates(false);
#endif
}

void OBSBasic::CheckForUpdates(bool manualUpdate)
{
#if _WIN32
	ui->actionCheckForUpdates->setEnabled(false);
	ui->actionRepair->setEnabled(false);

	if (updateCheckThread && updateCheckThread->isRunning())
		return;
	updateCheckThread.reset(new AutoUpdateThread(manualUpdate));
	updateCheckThread->start();
#elif defined(ENABLE_SPARKLE_UPDATER)
	ui->actionCheckForUpdates->setEnabled(false);

	if (updateCheckThread && updateCheckThread->isRunning())
		return;

	MacUpdateThread *mut = new MacUpdateThread(manualUpdate);
	connect(mut, &MacUpdateThread::Result, this, &OBSBasic::MacBranchesFetched, Qt::QueuedConnection);
	updateCheckThread.reset(mut);
	updateCheckThread->start();
#else
	UNUSED_PARAMETER(manualUpdate);
#endif
}

void OBSBasic::MacBranchesFetched(const QString &branch, bool manualUpdate)
{
#ifdef ENABLE_SPARKLE_UPDATER
	static OBSSparkle *updater;

	if (!updater) {
		updater = new OBSSparkle(QT_TO_UTF8(branch), ui->actionCheckForUpdates);
		return;
	}

	updater->setBranch(QT_TO_UTF8(branch));
	updater->checkForUpdates(manualUpdate);
#else
	UNUSED_PARAMETER(branch);
	UNUSED_PARAMETER(manualUpdate);
#endif
}

void OBSBasic::updateCheckFinished()
{
	ui->actionCheckForUpdates->setEnabled(true);
	ui->actionRepair->setEnabled(true);
}

void OBSBasic::DuplicateSelectedScene()
{
	OBSScene curScene = GetCurrentScene();

	if (!curScene)
		return;

	OBSSource curSceneSource = obs_scene_get_source(curScene);
	QString format{obs_source_get_name(curSceneSource)};
	format += " %1";

	int i = 2;
	QString placeHolderText = format.arg(i);
	OBSSourceAutoRelease source = nullptr;
	while ((source = obs_get_source_by_name(QT_TO_UTF8(placeHolderText)))) {
		placeHolderText = format.arg(++i);
	}

	for (;;) {
		string name;
		bool accepted = NameDialog::AskForName(this, QTStr("Basic.Main.AddSceneDlg.Title"),
						       QTStr("Basic.Main.AddSceneDlg.Text"), name, placeHolderText);
		if (!accepted)
			return;

		if (name.empty()) {
			OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
			continue;
		}

		obs_source_t *source = obs_get_source_by_name(name.c_str());
		if (source) {
			OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));

			obs_source_release(source);
			continue;
		}

		OBSSceneAutoRelease scene = obs_scene_duplicate(curScene, name.c_str(), OBS_SCENE_DUP_REFS);
		source = obs_scene_get_source(scene);
		SetCurrentScene(source, true);

		auto undo = [](const std::string &data) {
			OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
			obs_source_remove(source);
		};

		auto redo = [this, name](const std::string &data) {
			OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
			obs_scene_t *scene = obs_scene_from_source(source);
			scene = obs_scene_duplicate(scene, name.c_str(), OBS_SCENE_DUP_REFS);
			source = obs_scene_get_source(scene);
			SetCurrentScene(source.Get(), true);
		};

		undo_s.add_action(QTStr("Undo.Scene.Duplicate").arg(obs_source_get_name(source)), undo, redo,
				  obs_source_get_name(source), obs_source_get_name(obs_scene_get_source(curScene)));

		break;
	}
}

static bool save_undo_source_enum(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *p)
{
	obs_source_t *source = obs_sceneitem_get_source(item);
	if (obs_obj_is_private(source) && !obs_source_removed(source))
		return true;

	obs_data_array_t *array = (obs_data_array_t *)p;

	/* check if the source is already stored in the array */
	const char *name = obs_source_get_name(source);
	const size_t count = obs_data_array_count(array);
	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease sourceData = obs_data_array_item(array, i);
		if (strcmp(name, obs_data_get_string(sourceData, "name")) == 0)
			return true;
	}

	if (obs_source_is_group(source))
		obs_scene_enum_items(obs_group_from_source(source), save_undo_source_enum, p);

	OBSDataAutoRelease source_data = obs_save_source(source);
	obs_data_array_push_back(array, source_data);
	return true;
}

static inline void RemoveSceneAndReleaseNested(obs_source_t *source)
{
	obs_source_remove(source);
	auto cb = [](void *, obs_source_t *source) {
		if (strcmp(obs_source_get_id(source), "scene") == 0)
			obs_scene_prune_sources(obs_scene_from_source(source));
		return true;
	};
	obs_enum_scenes(cb, NULL);
}

void OBSBasic::RemoveSelectedScene()
{
	OBSScene scene = GetCurrentScene();
	obs_source_t *source = obs_scene_get_source(scene);

	if (!source || !QueryRemoveSource(source)) {
		return;
	}

	/* ------------------------------ */
	/* save all sources in scene      */

	OBSDataArrayAutoRelease sources_in_deleted_scene = obs_data_array_create();

	obs_scene_enum_items(scene, save_undo_source_enum, sources_in_deleted_scene);

	OBSDataAutoRelease scene_data = obs_save_source(source);
	obs_data_array_push_back(sources_in_deleted_scene, scene_data);

	/* ----------------------------------------------- */
	/* save all scenes and groups the scene is used in */

	OBSDataArrayAutoRelease scene_used_in_other_scenes = obs_data_array_create();

	struct other_scenes_cb_data {
		obs_source_t *oldScene;
		obs_data_array_t *scene_used_in_other_scenes;
	} other_scenes_cb_data;
	other_scenes_cb_data.oldScene = source;
	other_scenes_cb_data.scene_used_in_other_scenes = scene_used_in_other_scenes;

	auto other_scenes_cb = [](void *data_ptr, obs_source_t *scene) {
		struct other_scenes_cb_data *data = (struct other_scenes_cb_data *)data_ptr;
		if (strcmp(obs_source_get_name(scene), obs_source_get_name(data->oldScene)) == 0)
			return true;
		obs_sceneitem_t *item = obs_scene_find_source(obs_group_or_scene_from_source(scene),
							      obs_source_get_name(data->oldScene));
		if (item) {
			OBSDataAutoRelease scene_data =
				obs_save_source(obs_scene_get_source(obs_sceneitem_get_scene(item)));
			obs_data_array_push_back(data->scene_used_in_other_scenes, scene_data);
		}
		return true;
	};
	obs_enum_scenes(other_scenes_cb, &other_scenes_cb_data);

	/* --------------------------- */
	/* undo/redo                   */

	auto undo = [this](const std::string &json) {
		OBSDataAutoRelease base = obs_data_create_from_json(json.c_str());
		OBSDataArrayAutoRelease sources_in_deleted_scene = obs_data_get_array(base, "sources_in_deleted_scene");
		OBSDataArrayAutoRelease scene_used_in_other_scenes =
			obs_data_get_array(base, "scene_used_in_other_scenes");
		int savedIndex = (int)obs_data_get_int(base, "index");
		std::vector<OBSSource> sources;

		/* create missing sources */
		size_t count = obs_data_array_count(sources_in_deleted_scene);
		sources.reserve(count);

		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease data = obs_data_array_item(sources_in_deleted_scene, i);
			const char *name = obs_data_get_string(data, "name");

			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			if (!source) {
				source = obs_load_source(data);
				sources.push_back(source.Get());
			}
		}

		/* actually load sources now */
		for (obs_source_t *source : sources)
			obs_source_load2(source);

		/* Add scene to scenes and groups it was nested in */
		for (size_t i = 0; i < obs_data_array_count(scene_used_in_other_scenes); i++) {
			OBSDataAutoRelease data = obs_data_array_item(scene_used_in_other_scenes, i);
			const char *name = obs_data_get_string(data, "name");
			OBSSourceAutoRelease source = obs_get_source_by_name(name);

			OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
			OBSDataArrayAutoRelease items = obs_data_get_array(settings, "items");

			/* Clear scene, but keep a reference to all sources in the scene to make sure they don't get destroyed */
			std::vector<OBSSource> existing_sources;
			auto cb = [](obs_scene_t *, obs_sceneitem_t *item, void *data) {
				std::vector<OBSSource> *existing = (std::vector<OBSSource> *)data;
				OBSSource source = obs_sceneitem_get_source(item);
				obs_sceneitem_remove(item);
				existing->push_back(source);
				return true;
			};
			obs_scene_enum_items(obs_group_or_scene_from_source(source), cb, (void *)&existing_sources);

			/* Re-add sources to the scene */
			obs_sceneitems_add(obs_group_or_scene_from_source(source), items);
		}

		obs_source_t *scene_source = sources.back();
		OBSScene scene = obs_scene_from_source(scene_source);
		SetCurrentScene(scene, true);

		/* set original index in list box */
		ui->scenes->blockSignals(true);
		int curIndex = ui->scenes->currentRow();
		QListWidgetItem *item = ui->scenes->takeItem(curIndex);
		ui->scenes->insertItem(savedIndex, item);
		ui->scenes->setCurrentRow(savedIndex);
		currentScene = scene.Get();
		ui->scenes->blockSignals(false);
	};

	auto redo = [](const std::string &name) {
		OBSSourceAutoRelease source = obs_get_source_by_name(name.c_str());
		RemoveSceneAndReleaseNested(source);
	};

	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_array(data, "sources_in_deleted_scene", sources_in_deleted_scene);
	obs_data_set_array(data, "scene_used_in_other_scenes", scene_used_in_other_scenes);
	obs_data_set_int(data, "index", ui->scenes->currentRow());

	const char *scene_name = obs_source_get_name(source);
	undo_s.add_action(QTStr("Undo.Delete").arg(scene_name), undo, redo, obs_data_get_json(data), scene_name);

	/* --------------------------- */
	/* remove                      */

	RemoveSceneAndReleaseNested(source);

	OnEvent(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED);
}

void OBSBasic::ReorderSources(OBSScene scene)
{
	if (scene != GetCurrentScene() || ui->sources->IgnoreReorder())
		return;

	ui->sources->ReorderItems();
	SaveProject();
}

void OBSBasic::RefreshSources(OBSScene scene)
{
	if (scene != GetCurrentScene() || ui->sources->IgnoreReorder())
		return;

	ui->sources->RefreshItems();
	SaveProject();
}

/* OBS Callbacks */

void OBSBasic::SceneReordered(void *data, calldata_t *params)
{
	OBSBasic *window = static_cast<OBSBasic *>(data);

	obs_scene_t *scene = (obs_scene_t *)calldata_ptr(params, "scene");

	QMetaObject::invokeMethod(window, "ReorderSources", Q_ARG(OBSScene, OBSScene(scene)));
}

void OBSBasic::SceneRefreshed(void *data, calldata_t *params)
{
	OBSBasic *window = static_cast<OBSBasic *>(data);

	obs_scene_t *scene = (obs_scene_t *)calldata_ptr(params, "scene");

	QMetaObject::invokeMethod(window, "RefreshSources", Q_ARG(OBSScene, OBSScene(scene)));
}

void OBSBasic::SceneItemAdded(void *data, calldata_t *params)
{
	OBSBasic *window = static_cast<OBSBasic *>(data);

	obs_sceneitem_t *item = (obs_sceneitem_t *)calldata_ptr(params, "item");

	QMetaObject::invokeMethod(window, "AddSceneItem", Q_ARG(OBSSceneItem, OBSSceneItem(item)));
}

void OBSBasic::SourceCreated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	if (obs_scene_from_source(source) != NULL)
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "AddScene", WaitConnection(),
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceRemoved(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	if (obs_scene_from_source(source) != NULL)
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "RemoveScene",
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO)
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "ActivateAudioSource",
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO)
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "DeactivateAudioSource",
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceAudioActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	if (obs_source_active(source))
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "ActivateAudioSource",
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceAudioDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "DeactivateAudioSource",
				  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceRenamed(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	const char *newName = calldata_string(params, "new_name");
	const char *prevName = calldata_string(params, "prev_name");

	QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "RenameSources", Q_ARG(OBSSource, source),
				  Q_ARG(QString, QT_UTF8(newName)), Q_ARG(QString, QT_UTF8(prevName)));

	blog(LOG_INFO, "Source '%s' renamed to '%s'", prevName, newName);
}

void OBSBasic::DrawBackdrop(float cx, float cy)
{
	if (!box)
		return;

	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawBackdrop");

	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *color = gs_effect_get_param_by_name(solid, "color");
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	vec4 colorVal;
	vec4_set(&colorVal, 0.0f, 0.0f, 0.0f, 1.0f);
	gs_effect_set_vec4(color, &colorVal);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_scale3f(float(cx), float(cy), 1.0f);

	gs_load_vertexbuffer(box);
	gs_draw(GS_TRISTRIP, 0, 0);

	gs_matrix_pop();
	gs_technique_end_pass(tech);
	gs_technique_end(tech);

	gs_load_vertexbuffer(nullptr);

	GS_DEBUG_MARKER_END();
}

void OBSBasic::RenderMain(void *data, uint32_t, uint32_t)
{
	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "RenderMain");

	OBSBasic *window = static_cast<OBSBasic *>(data);
	obs_video_info ovi;

	obs_get_video_info(&ovi);

	window->previewCX = int(window->previewScale * float(ovi.base_width));
	window->previewCY = int(window->previewScale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();

	obs_display_t *display = window->ui->preview->GetDisplay();
	uint32_t width, height;
	obs_display_size(display, &width, &height);
	float right = float(width) - window->previewX;
	float bottom = float(height) - window->previewY;

	gs_ortho(-window->previewX, right, -window->previewY, bottom, -100.0f, 100.0f);

	window->ui->preview->DrawOverflow();

	/* --------------------------------------- */

	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
	gs_set_viewport(window->previewX, window->previewY, window->previewCX, window->previewCY);

	if (window->IsPreviewProgramMode()) {
		window->DrawBackdrop(float(ovi.base_width), float(ovi.base_height));

		OBSScene scene = window->GetCurrentScene();
		obs_source_t *source = obs_scene_get_source(scene);
		if (source)
			obs_source_video_render(source);
	} else {
		obs_render_main_texture_src_color_only();
	}
	gs_load_vertexbuffer(nullptr);

	/* --------------------------------------- */

	gs_ortho(-window->previewX, right, -window->previewY, bottom, -100.0f, 100.0f);
	gs_reset_viewport();

	uint32_t targetCX = window->previewCX;
	uint32_t targetCY = window->previewCY;

	if (window->drawSafeAreas) {
		RenderSafeAreas(window->actionSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->graphicsSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->fourByThreeSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->leftLine, targetCX, targetCY);
		RenderSafeAreas(window->topLine, targetCX, targetCY);
		RenderSafeAreas(window->rightLine, targetCX, targetCY);
	}

	window->ui->preview->DrawSceneEditing();

	if (window->drawSpacingHelpers)
		window->ui->preview->DrawSpacingHelpers();

	/* --------------------------------------- */

	gs_projection_pop();
	gs_viewport_pop();

	GS_DEBUG_MARKER_END();
}

/* Main class functions */

obs_service_t *OBSBasic::GetService()
{
	if (!service) {
		service = obs_service_create("rtmp_common", NULL, NULL, nullptr);
		obs_service_release(service);
	}
	return service;
}

void OBSBasic::SetService(obs_service_t *newService)
{
	if (newService) {
		service = newService;
	}
}

int OBSBasic::GetTransitionDuration()
{
	return ui->transitionDuration->value();
}

bool OBSBasic::Active() const
{
	if (!outputHandler)
		return false;
	return outputHandler->Active();
}

#ifdef _WIN32
#define IS_WIN32 1
#else
#define IS_WIN32 0
#endif

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

void OBSBasic::ResetUI()
{
	bool studioPortraitLayout = config_get_bool(App()->GetUserConfig(), "BasicWindow", "StudioPortraitLayout");

	if (studioPortraitLayout)
		ui->previewLayout->setDirection(QBoxLayout::BottomToTop);
	else
		ui->previewLayout->setDirection(QBoxLayout::LeftToRight);

	UpdatePreviewProgramIndicators();
}

int OBSBasic::ResetVideo()
{
	if (outputHandler && outputHandler->Active())
		return OBS_VIDEO_CURRENTLY_ACTIVE;

	ProfileScope("OBSBasic::ResetVideo");

	struct obs_video_info ovi;
	int ret;

	GetConfigFPS(ovi.fps_num, ovi.fps_den);

	const char *colorFormat = config_get_string(activeConfiguration, "Video", "ColorFormat");
	const char *colorSpace = config_get_string(activeConfiguration, "Video", "ColorSpace");
	const char *colorRange = config_get_string(activeConfiguration, "Video", "ColorRange");

	ovi.graphics_module = App()->GetRenderModule();
	ovi.base_width = (uint32_t)config_get_uint(activeConfiguration, "Video", "BaseCX");
	ovi.base_height = (uint32_t)config_get_uint(activeConfiguration, "Video", "BaseCY");
	ovi.output_width = (uint32_t)config_get_uint(activeConfiguration, "Video", "OutputCX");
	ovi.output_height = (uint32_t)config_get_uint(activeConfiguration, "Video", "OutputCY");
	ovi.output_format = GetVideoFormatFromName(colorFormat);
	ovi.colorspace = GetVideoColorSpaceFromName(colorSpace);
	ovi.range = astrcmpi(colorRange, "Full") == 0 ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
	ovi.adapter = config_get_uint(App()->GetUserConfig(), "Video", "AdapterIdx");
	ovi.gpu_conversion = true;
	ovi.scale_type = GetScaleType(activeConfiguration);

	if (ovi.base_width < 32 || ovi.base_height < 32) {
		ovi.base_width = 1920;
		ovi.base_height = 1080;
		config_set_uint(activeConfiguration, "Video", "BaseCX", 1920);
		config_set_uint(activeConfiguration, "Video", "BaseCY", 1080);
	}

	if (ovi.output_width < 32 || ovi.output_height < 32) {
		ovi.output_width = ovi.base_width;
		ovi.output_height = ovi.base_height;
		config_set_uint(activeConfiguration, "Video", "OutputCX", ovi.base_width);
		config_set_uint(activeConfiguration, "Video", "OutputCY", ovi.base_height);
	}

	ret = AttemptToResetVideo(&ovi);
	if (ret == OBS_VIDEO_CURRENTLY_ACTIVE) {
		blog(LOG_WARNING, "Tried to reset when already active");
		return ret;
	}

	if (ret == OBS_VIDEO_SUCCESS) {
		ResizePreview(ovi.base_width, ovi.base_height);
		if (program)
			ResizeProgram(ovi.base_width, ovi.base_height);

		const float sdr_white_level = (float)config_get_uint(activeConfiguration, "Video", "SdrWhiteLevel");
		const float hdr_nominal_peak_level =
			(float)config_get_uint(activeConfiguration, "Video", "HdrNominalPeakLevel");
		obs_set_video_levels(sdr_white_level, hdr_nominal_peak_level);
		OBSBasicStats::InitializeValues();
		OBSProjector::UpdateMultiviewProjectors();

		bool canMigrate = usingAbsoluteCoordinates ||
				  (migrationBaseResolution && (migrationBaseResolution->first != ovi.base_width ||
							       migrationBaseResolution->second != ovi.base_height));
		ui->actionRemigrateSceneCollection->setEnabled(canMigrate);

		emit CanvasResized(ovi.base_width, ovi.base_height);
		emit OutputResized(ovi.output_width, ovi.output_height);
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

extern char *get_new_source_name(const char *name, const char *format);

void OBSBasic::ResetAudioDevice(const char *sourceId, const char *deviceId, const char *deviceDesc, int channel)
{
	bool disable = deviceId && strcmp(deviceId, "disabled") == 0;
	OBSSourceAutoRelease source;
	OBSDataAutoRelease settings;

	source = obs_get_output_source(channel);
	if (source) {
		if (disable) {
			obs_set_output_source(channel, nullptr);
		} else {
			settings = obs_source_get_settings(source);
			const char *oldId = obs_data_get_string(settings, "device_id");
			if (strcmp(oldId, deviceId) != 0) {
				obs_data_set_string(settings, "device_id", deviceId);
				obs_source_update(source, settings);
			}
		}

	} else if (!disable) {
		BPtr<char> name = get_new_source_name(deviceDesc, "%s (%d)");

		settings = obs_data_create();
		obs_data_set_string(settings, "device_id", deviceId);
		source = obs_source_create(sourceId, name, settings, nullptr);

		obs_set_output_source(channel, source);
	}
}

void OBSBasic::ResizePreview(uint32_t cx, uint32_t cy)
{
	QSize targetSize;
	bool isFixedScaling;
	obs_video_info ovi;

	/* resize preview panel to fix to the top section of the window */
	targetSize = GetPixelSize(ui->preview);

	isFixedScaling = ui->preview->IsFixedScaling();
	obs_get_video_info(&ovi);

	if (isFixedScaling) {
		previewScale = ui->preview->GetScalingAmount();

		ui->preview->ClampScrollingOffsets();

		GetCenterPosFromFixedScale(int(cx), int(cy), targetSize.width() - PREVIEW_EDGE_SIZE * 2,
					   targetSize.height() - PREVIEW_EDGE_SIZE * 2, previewX, previewY,
					   previewScale);
		previewX += ui->preview->GetScrollX();
		previewY += ui->preview->GetScrollY();

	} else {
		GetScaleAndCenterPos(int(cx), int(cy), targetSize.width() - PREVIEW_EDGE_SIZE * 2,
				     targetSize.height() - PREVIEW_EDGE_SIZE * 2, previewX, previewY, previewScale);
	}

	ui->preview->SetScalingAmount(previewScale);

	previewX += float(PREVIEW_EDGE_SIZE);
	previewY += float(PREVIEW_EDGE_SIZE);
}

void OBSBasic::CloseDialogs()
{
	QList<QDialog *> childDialogs = this->findChildren<QDialog *>();
	if (!childDialogs.isEmpty()) {
		for (const auto &dialog : childDialogs) {
			dialog->close();
		}
	}

	if (!stats.isNull())
		stats->close(); //call close to save Stats geometry
	if (!remux.isNull())
		remux->close();
}

void OBSBasic::EnumDialogs()
{
	visDialogs.clear();
	modalDialogs.clear();
	visMsgBoxes.clear();

	/* fill list of Visible dialogs and Modal dialogs */
	QList<QDialog *> dialogs = findChildren<QDialog *>();
	for (QDialog *dialog : dialogs) {
		if (dialog->isVisible())
			visDialogs.append(dialog);
		if (dialog->isModal())
			modalDialogs.append(dialog);
	}

	/* fill list of Visible message boxes */
	QList<QMessageBox *> msgBoxes = findChildren<QMessageBox *>();
	for (QMessageBox *msgbox : msgBoxes) {
		if (msgbox->isVisible())
			visMsgBoxes.append(msgbox);
	}
}

void OBSBasic::ClearProjectors()
{
	for (const auto &proj : projectors)
		if (proj)
			delete proj;

	projectors.clear();
}

void OBSBasic::ClearSceneData()
{
	disableSaving++;

	setCursor(Qt::WaitCursor);

	CloseDialogs();

	ClearVolumeControls();
	ClearListItems(ui->scenes);
	ui->sources->Clear();
	ClearQuickTransitions();
	ui->transitions->clear();

	ClearProjectors();

	for (int i = 0; i < MAX_CHANNELS; i++)
		obs_set_output_source(i, nullptr);

	/* Reset VCam to default to clear its private scene and any references
	 * it holds. It will be reconfigured during loading. */
	if (vcamEnabled) {
		vcamConfig.type = VCamOutputType::ProgramView;
		outputHandler->UpdateVirtualCamOutputSource();
	}

	collectionModuleData = nullptr;
	lastScene = nullptr;
	swapScene = nullptr;
	programScene = nullptr;
	prevFTBSource = nullptr;

	clipboard.clear();
	copyFiltersSource = nullptr;
	copyFilter = nullptr;

	auto cb = [](void *, obs_source_t *source) {
		obs_source_remove(source);
		return true;
	};

	obs_enum_scenes(cb, nullptr);
	obs_enum_sources(cb, nullptr);

	OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP);

	undo_s.clear();

	/* using QEvent::DeferredDelete explicitly is the only way to ensure
	 * that deleteLater events are processed at this point */
	QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

	do {
		QApplication::sendPostedEvents(nullptr);
	} while (obs_wait_for_destroy_queue());

	/* Pump Qt events one final time to give remaining signals time to be
	 * processed (since this happens after the destroy thread finishes and
	 * the audio/video threads have processed their tasks). */
	QApplication::sendPostedEvents(nullptr);

	unsetCursor();

	/* If scene data wasn't actually cleared, e.g. faulty plugin holding a
	 * reference, they will still be in the hash table, enumerate them and
	 * store the names for logging purposes. */
	auto cb2 = [](void *param, obs_source_t *source) {
		auto orphans = static_cast<vector<string> *>(param);
		orphans->push_back(obs_source_get_name(source));
		return true;
	};

	vector<string> orphan_sources;
	obs_enum_sources(cb2, &orphan_sources);

	if (!orphan_sources.empty()) {
		/* Avoid logging list twice in case it gets called after
		 * setting the flag the first time. */
		if (!clearingFailed) {
			/* This ugly mess exists to join a vector of strings
			 * with a user-defined delimiter. */
			string orphan_names =
				std::accumulate(orphan_sources.begin(), orphan_sources.end(), string(""),
						[](string a, string b) { return std::move(a) + "\n- " + b; });

			blog(LOG_ERROR, "Not all sources were cleared when clearing scene data:\n%s\n",
			     orphan_names.c_str());
		}

		/* We do not decrement disableSaving here to avoid OBS
		 * overwriting user data with garbage. */
		clearingFailed = true;
	} else {
		disableSaving--;

		blog(LOG_INFO, "All scene data cleared");
		blog(LOG_INFO, "------------------------------------------------");
	}
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

void OBSBasic::on_actionShow_Recordings_triggered()
{
	const char *mode = config_get_string(activeConfiguration, "Output", "Mode");
	const char *type = config_get_string(activeConfiguration, "AdvOut", "RecType");
	const char *adv_path = strcmp(type, "Standard")
				       ? config_get_string(activeConfiguration, "AdvOut", "FFFilePath")
				       : config_get_string(activeConfiguration, "AdvOut", "RecFilePath");
	const char *path = strcmp(mode, "Advanced") ? config_get_string(activeConfiguration, "SimpleOutput", "FilePath")
						    : adv_path;
	QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void OBSBasic::on_actionRemux_triggered()
{
	if (!remux.isNull()) {
		remux->show();
		remux->raise();
		return;
	}

	const char *mode = config_get_string(activeConfiguration, "Output", "Mode");
	const char *path = strcmp(mode, "Advanced") ? config_get_string(activeConfiguration, "SimpleOutput", "FilePath")
						    : config_get_string(activeConfiguration, "AdvOut", "RecFilePath");

	OBSRemux *remuxDlg;
	remuxDlg = new OBSRemux(path, this);
	remuxDlg->show();
	remux = remuxDlg;
}

void OBSBasic::on_action_Settings_triggered()
{
	static bool settings_already_executing = false;

	/* Do not load settings window if inside of a temporary event loop
	 * because we could be inside of an Auth::LoadUI call.  Keep trying
	 * once per second until we've exit any known sub-loops. */
	if (os_atomic_load_long(&insideEventLoop) != 0) {
		QTimer::singleShot(1000, this, &OBSBasic::on_action_Settings_triggered);
		return;
	}

	if (settings_already_executing) {
		return;
	}

	settings_already_executing = true;

	{
		OBSBasicSettings settings(this);
		settings.exec();
	}

	settings_already_executing = false;

	if (restart) {
		QMessageBox::StandardButton button =
			OBSMessageBox::question(this, QTStr("Restart"), QTStr("NeedsRestart"));

		if (button == QMessageBox::Yes)
			close();
		else
			restart = false;
	}
}

void OBSBasic::on_actionShowMacPermissions_triggered()
{
#ifdef __APPLE__
	OBSPermissions check(this, CheckPermission(kScreenCapture), CheckPermission(kVideoDeviceAccess),
			     CheckPermission(kAudioDeviceAccess), CheckPermission(kAccessibility));
	check.exec();
#endif
}

void OBSBasic::ShowMissingFilesDialog(obs_missing_files_t *files)
{
	if (obs_missing_files_count(files) > 0) {
		/* When loading the missing files dialog on launch, the
		* window hasn't fully initialized by this point on macOS,
		* so put this at the end of the current task queue. Fixes
		* a bug where the window is behind OBS on startup. */
		QTimer::singleShot(0, [this, files] {
			missDialog = new OBSMissingFiles(files, this);
			missDialog->setAttribute(Qt::WA_DeleteOnClose, true);
			missDialog->show();
			missDialog->raise();
		});
	} else {
		obs_missing_files_destroy(files);

		/* Only raise dialog if triggered manually */
		if (!disableSaving)
			OBSMessageBox::information(this, QTStr("MissingFiles.NoMissing.Title"),
						   QTStr("MissingFiles.NoMissing.Text"));
	}
}

void OBSBasic::on_actionShowMissingFiles_triggered()
{
	obs_missing_files_t *files = obs_missing_files_create();

	auto cb_sources = [](void *data, obs_source_t *source) {
		AddMissingFiles(data, source);
		return true;
	};

	obs_enum_all_sources(cb_sources, files);
	ShowMissingFilesDialog(files);
}

void OBSBasic::on_actionAdvAudioProperties_triggered()
{
	if (advAudioWindow != nullptr) {
		advAudioWindow->raise();
		return;
	}

	bool iconsVisible = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ShowSourceIcons");

	advAudioWindow = new OBSBasicAdvAudio(this);
	advAudioWindow->show();
	advAudioWindow->setAttribute(Qt::WA_DeleteOnClose, true);
	advAudioWindow->SetIconsVisible(iconsVisible);
}

void OBSBasic::on_actionMixerToolbarAdvAudio_triggered()
{
	on_actionAdvAudioProperties_triggered();
}

void OBSBasic::on_actionMixerToolbarMenu_triggered()
{
	QAction unhideAllAction(QTStr("UnhideAll"), this);
	connect(&unhideAllAction, &QAction::triggered, this, &OBSBasic::UnhideAllAudioControls, Qt::DirectConnection);

	QAction toggleControlLayoutAction(QTStr("VerticalLayout"), this);
	toggleControlLayoutAction.setCheckable(true);
	toggleControlLayoutAction.setChecked(
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl"));
	connect(&toggleControlLayoutAction, &QAction::changed, this, &OBSBasic::ToggleVolControlLayout,
		Qt::DirectConnection);

	QMenu popup;
	popup.addAction(&unhideAllAction);
	popup.addSeparator();
	popup.addAction(&toggleControlLayoutAction);
	popup.exec(QCursor::pos());
}

void OBSBasic::on_scenes_currentItemChanged(QListWidgetItem *current, QListWidgetItem *)
{
	OBSSource source;

	if (current) {
		OBSScene scene = GetOBSRef<OBSScene>(current);
		source = obs_scene_get_source(scene);

		currentScene = scene;
	} else {
		currentScene = NULL;
	}

	SetCurrentScene(source);

	if (vcamEnabled && vcamConfig.type == VCamOutputType::PreviewOutput)
		outputHandler->UpdateVirtualCamOutputSource();

	OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);

	UpdateContextBar();
}

void OBSBasic::EditSceneName()
{
	ui->scenesDock->removeAction(renameScene);
	QListWidgetItem *item = ui->scenes->currentItem();
	Qt::ItemFlags flags = item->flags();

	item->setFlags(flags | Qt::ItemIsEditable);
	ui->scenes->editItem(item);
	item->setFlags(flags);
}

QList<QString> OBSBasic::GetProjectorMenuMonitorsFormatted()
{
	QList<QString> projectorsFormatted;
	QList<QScreen *> screens = QGuiApplication::screens();
	for (int i = 0; i < screens.size(); i++) {
		QScreen *screen = screens[i];
		QRect screenGeometry = screen->geometry();
		qreal ratio = screen->devicePixelRatio();
		QString name = "";
#if defined(__APPLE__) || defined(_WIN32)
		name = screen->name();
#else
		name = screen->model().simplified();

		if (name.length() > 1 && name.endsWith("-"))
			name.chop(1);
#endif
		name = name.simplified();

		if (name.length() == 0) {
			name = QString("%1 %2").arg(QTStr("Display")).arg(QString::number(i + 1));
		}
		QString str = QString("%1: %2x%3 @ %4,%5")
				      .arg(name, QString::number(screenGeometry.width() * ratio),
					   QString::number(screenGeometry.height() * ratio),
					   QString::number(screenGeometry.x()), QString::number(screenGeometry.y()));
		projectorsFormatted.push_back(str);
	}
	return projectorsFormatted;
}

void OBSBasic::on_scenes_customContextMenuRequested(const QPoint &pos)
{
	QListWidgetItem *item = ui->scenes->itemAt(pos);

	QMenu popup(this);
	QMenu order(QTStr("Basic.MainMenu.Edit.Order"), this);

	popup.addAction(QTStr("Add"), this, &OBSBasic::on_actionAddScene_triggered);

	if (item) {
		QAction *copyFilters = new QAction(QTStr("Copy.Filters"), this);
		copyFilters->setEnabled(false);
		connect(copyFilters, &QAction::triggered, this, &OBSBasic::SceneCopyFilters);
		QAction *pasteFilters = new QAction(QTStr("Paste.Filters"), this);
		pasteFilters->setEnabled(!obs_weak_source_expired(copyFiltersSource));
		connect(pasteFilters, &QAction::triggered, this, &OBSBasic::ScenePasteFilters);

		popup.addSeparator();
		popup.addAction(QTStr("Duplicate"), this, &OBSBasic::DuplicateSelectedScene);
		popup.addAction(copyFilters);
		popup.addAction(pasteFilters);
		popup.addSeparator();
		popup.addAction(renameScene);
		popup.addAction(ui->actionRemoveScene);
		popup.addSeparator();

		order.addAction(QTStr("Basic.MainMenu.Edit.Order.MoveUp"), this, &OBSBasic::on_actionSceneUp_triggered);
		order.addAction(QTStr("Basic.MainMenu.Edit.Order.MoveDown"), this,
				&OBSBasic::on_actionSceneDown_triggered);
		order.addSeparator();
		order.addAction(QTStr("Basic.MainMenu.Edit.Order.MoveToTop"), this, &OBSBasic::MoveSceneToTop);
		order.addAction(QTStr("Basic.MainMenu.Edit.Order.MoveToBottom"), this, &OBSBasic::MoveSceneToBottom);
		popup.addMenu(&order);

		popup.addSeparator();

		delete sceneProjectorMenu;
		sceneProjectorMenu = new QMenu(QTStr("SceneProjector"));
		AddProjectorMenuMonitors(sceneProjectorMenu, this, &OBSBasic::OpenSceneProjector);
		popup.addMenu(sceneProjectorMenu);

		QAction *sceneWindow = popup.addAction(QTStr("SceneWindow"), this, &OBSBasic::OpenSceneWindow);

		popup.addAction(sceneWindow);
		popup.addAction(QTStr("Screenshot.Scene"), this, &OBSBasic::ScreenshotScene);
		popup.addSeparator();
		popup.addAction(QTStr("Filters"), this, &OBSBasic::OpenSceneFilters);

		popup.addSeparator();

		delete perSceneTransitionMenu;
		perSceneTransitionMenu = CreatePerSceneTransitionMenu();
		popup.addMenu(perSceneTransitionMenu);

		/* ---------------------- */

		QAction *multiviewAction = popup.addAction(QTStr("ShowInMultiview"));

		OBSSource source = GetCurrentSceneSource();
		OBSDataAutoRelease data = obs_source_get_private_settings(source);

		obs_data_set_default_bool(data, "show_in_multiview", true);
		bool show = obs_data_get_bool(data, "show_in_multiview");

		multiviewAction->setCheckable(true);
		multiviewAction->setChecked(show);

		auto showInMultiview = [](OBSData data) {
			bool show = obs_data_get_bool(data, "show_in_multiview");
			obs_data_set_bool(data, "show_in_multiview", !show);
			OBSProjector::UpdateMultiviewProjectors();
		};

		connect(multiviewAction, &QAction::triggered, std::bind(showInMultiview, data.Get()));

		copyFilters->setEnabled(obs_source_filter_count(source) > 0);
	}

	popup.addSeparator();

	bool grid = ui->scenes->GetGridMode();

	QAction *gridAction = new QAction(grid ? QTStr("Basic.Main.ListMode") : QTStr("Basic.Main.GridMode"), this);
	connect(gridAction, &QAction::triggered, this, &OBSBasic::GridActionClicked);
	popup.addAction(gridAction);

	popup.exec(QCursor::pos());
}

void OBSBasic::on_actionSceneListMode_triggered()
{
	ui->scenes->SetGridMode(false);
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "gridMode", false);
}

void OBSBasic::on_actionSceneGridMode_triggered()
{
	ui->scenes->SetGridMode(true);
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "gridMode", true);
}

void OBSBasic::GridActionClicked()
{
	bool gridMode = !ui->scenes->GetGridMode();
	ui->scenes->SetGridMode(gridMode);

	if (gridMode)
		ui->actionSceneGridMode->setChecked(true);
	else
		ui->actionSceneListMode->setChecked(true);

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "gridMode", gridMode);
}

void OBSBasic::on_actionAddScene_triggered()
{
	string name;
	QString format{QTStr("Basic.Main.DefaultSceneName.Text")};

	int i = 2;
	QString placeHolderText = format.arg(i);
	OBSSourceAutoRelease source = nullptr;
	while ((source = obs_get_source_by_name(QT_TO_UTF8(placeHolderText)))) {
		placeHolderText = format.arg(++i);
	}

	bool accepted = NameDialog::AskForName(this, QTStr("Basic.Main.AddSceneDlg.Title"),
					       QTStr("Basic.Main.AddSceneDlg.Text"), name, placeHolderText);

	if (accepted) {
		if (name.empty()) {
			OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
			on_actionAddScene_triggered();
			return;
		}

		OBSSourceAutoRelease source = obs_get_source_by_name(name.c_str());
		if (source) {
			OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));

			on_actionAddScene_triggered();
			return;
		}

		auto undo_fn = [](const std::string &data) {
			obs_source_t *t = obs_get_source_by_name(data.c_str());
			if (t) {
				obs_source_remove(t);
				obs_source_release(t);
			}
		};

		auto redo_fn = [this](const std::string &data) {
			OBSSceneAutoRelease scene = obs_scene_create(data.c_str());
			obs_source_t *source = obs_scene_get_source(scene);
			SetCurrentScene(source, true);
		};
		undo_s.add_action(QTStr("Undo.Add").arg(QString(name.c_str())), undo_fn, redo_fn, name, name);

		OBSSceneAutoRelease scene = obs_scene_create(name.c_str());
		obs_source_t *scene_source = obs_scene_get_source(scene);
		SetCurrentScene(scene_source);
	}
}

void OBSBasic::on_actionRemoveScene_triggered()
{
	RemoveSelectedScene();
}

void OBSBasic::ChangeSceneIndex(bool relative, int offset, int invalidIdx)
{
	int idx = ui->scenes->currentRow();
	if (idx == -1 || idx == invalidIdx)
		return;

	ui->scenes->blockSignals(true);
	QListWidgetItem *item = ui->scenes->takeItem(idx);

	if (!relative)
		idx = 0;

	ui->scenes->insertItem(idx + offset, item);
	ui->scenes->setCurrentRow(idx + offset);
	item->setSelected(true);
	currentScene = GetOBSRef<OBSScene>(item).Get();
	ui->scenes->blockSignals(false);

	OBSProjector::UpdateMultiviewProjectors();
}

void OBSBasic::on_actionSceneUp_triggered()
{
	ChangeSceneIndex(true, -1, 0);
}

void OBSBasic::on_actionSceneDown_triggered()
{
	ChangeSceneIndex(true, 1, ui->scenes->count() - 1);
}

void OBSBasic::MoveSceneToTop()
{
	ChangeSceneIndex(false, 0, 0);
}

void OBSBasic::MoveSceneToBottom()
{
	ChangeSceneIndex(false, ui->scenes->count() - 1, ui->scenes->count() - 1);
}

void OBSBasic::EditSceneItemName()
{
	int idx = GetTopSelectedSourceItem();
	ui->sources->Edit(idx);
}

void OBSBasic::SetDeinterlacingMode()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_deinterlace_mode mode = (obs_deinterlace_mode)action->property("mode").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();
	obs_source_t *source = obs_sceneitem_get_source(sceneItem);

	obs_source_set_deinterlace_mode(source, mode);
}

void OBSBasic::SetDeinterlacingOrder()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_deinterlace_field_order order = (obs_deinterlace_field_order)action->property("order").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();
	obs_source_t *source = obs_sceneitem_get_source(sceneItem);

	obs_source_set_deinterlace_field_order(source, order);
}

QMenu *OBSBasic::AddDeinterlacingMenu(QMenu *menu, obs_source_t *source)
{
	obs_deinterlace_mode deinterlaceMode = obs_source_get_deinterlace_mode(source);
	obs_deinterlace_field_order deinterlaceOrder = obs_source_get_deinterlace_field_order(source);
	QAction *action;

#define ADD_MODE(name, mode)                                                             \
	action = menu->addAction(QTStr("" name), this, &OBSBasic::SetDeinterlacingMode); \
	action->setProperty("mode", (int)mode);                                          \
	action->setCheckable(true);                                                      \
	action->setChecked(deinterlaceMode == mode);

	ADD_MODE("Disable", OBS_DEINTERLACE_MODE_DISABLE);
	ADD_MODE("Deinterlacing.Discard", OBS_DEINTERLACE_MODE_DISCARD);
	ADD_MODE("Deinterlacing.Retro", OBS_DEINTERLACE_MODE_RETRO);
	ADD_MODE("Deinterlacing.Blend", OBS_DEINTERLACE_MODE_BLEND);
	ADD_MODE("Deinterlacing.Blend2x", OBS_DEINTERLACE_MODE_BLEND_2X);
	ADD_MODE("Deinterlacing.Linear", OBS_DEINTERLACE_MODE_LINEAR);
	ADD_MODE("Deinterlacing.Linear2x", OBS_DEINTERLACE_MODE_LINEAR_2X);
	ADD_MODE("Deinterlacing.Yadif", OBS_DEINTERLACE_MODE_YADIF);
	ADD_MODE("Deinterlacing.Yadif2x", OBS_DEINTERLACE_MODE_YADIF_2X);
#undef ADD_MODE

	menu->addSeparator();

#define ADD_ORDER(name, order)                                                                          \
	action = menu->addAction(QTStr("Deinterlacing." name), this, &OBSBasic::SetDeinterlacingOrder); \
	action->setProperty("order", (int)order);                                                       \
	action->setCheckable(true);                                                                     \
	action->setChecked(deinterlaceOrder == order);

	ADD_ORDER("TopFieldFirst", OBS_DEINTERLACE_FIELD_ORDER_TOP);
	ADD_ORDER("BottomFieldFirst", OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);
#undef ADD_ORDER

	return menu;
}

void OBSBasic::SetScaleFilter()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_scale_type mode = (obs_scale_type)action->property("mode").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();

	obs_sceneitem_set_scale_filter(sceneItem, mode);
}

QMenu *OBSBasic::AddScaleFilteringMenu(QMenu *menu, obs_sceneitem_t *item)
{
	obs_scale_type scaleFilter = obs_sceneitem_get_scale_filter(item);
	QAction *action;

#define ADD_MODE(name, mode)                                                       \
	action = menu->addAction(QTStr("" name), this, &OBSBasic::SetScaleFilter); \
	action->setProperty("mode", (int)mode);                                    \
	action->setCheckable(true);                                                \
	action->setChecked(scaleFilter == mode);

	ADD_MODE("Disable", OBS_SCALE_DISABLE);
	ADD_MODE("ScaleFiltering.Point", OBS_SCALE_POINT);
	ADD_MODE("ScaleFiltering.Bilinear", OBS_SCALE_BILINEAR);
	ADD_MODE("ScaleFiltering.Bicubic", OBS_SCALE_BICUBIC);
	ADD_MODE("ScaleFiltering.Lanczos", OBS_SCALE_LANCZOS);
	ADD_MODE("ScaleFiltering.Area", OBS_SCALE_AREA);
#undef ADD_MODE

	return menu;
}

void OBSBasic::SetBlendingMethod()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_blending_method method = (obs_blending_method)action->property("method").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();

	obs_sceneitem_set_blending_method(sceneItem, method);
}

QMenu *OBSBasic::AddBlendingMethodMenu(QMenu *menu, obs_sceneitem_t *item)
{
	obs_blending_method blendingMethod = obs_sceneitem_get_blending_method(item);
	QAction *action;

#define ADD_MODE(name, method)                                                        \
	action = menu->addAction(QTStr("" name), this, &OBSBasic::SetBlendingMethod); \
	action->setProperty("method", (int)method);                                   \
	action->setCheckable(true);                                                   \
	action->setChecked(blendingMethod == method);

	ADD_MODE("BlendingMethod.Default", OBS_BLEND_METHOD_DEFAULT);
	ADD_MODE("BlendingMethod.SrgbOff", OBS_BLEND_METHOD_SRGB_OFF);
#undef ADD_MODE

	return menu;
}

void OBSBasic::SetBlendingMode()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_blending_type mode = (obs_blending_type)action->property("mode").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();

	obs_sceneitem_set_blending_mode(sceneItem, mode);
}

QMenu *OBSBasic::AddBlendingModeMenu(QMenu *menu, obs_sceneitem_t *item)
{
	obs_blending_type blendingMode = obs_sceneitem_get_blending_mode(item);
	QAction *action;

#define ADD_MODE(name, mode)                                                        \
	action = menu->addAction(QTStr("" name), this, &OBSBasic::SetBlendingMode); \
	action->setProperty("mode", (int)mode);                                     \
	action->setCheckable(true);                                                 \
	action->setChecked(blendingMode == mode);

	ADD_MODE("BlendingMode.Normal", OBS_BLEND_NORMAL);
	ADD_MODE("BlendingMode.Additive", OBS_BLEND_ADDITIVE);
	ADD_MODE("BlendingMode.Subtract", OBS_BLEND_SUBTRACT);
	ADD_MODE("BlendingMode.Screen", OBS_BLEND_SCREEN);
	ADD_MODE("BlendingMode.Multiply", OBS_BLEND_MULTIPLY);
	ADD_MODE("BlendingMode.Lighten", OBS_BLEND_LIGHTEN);
	ADD_MODE("BlendingMode.Darken", OBS_BLEND_DARKEN);
#undef ADD_MODE

	return menu;
}

QMenu *OBSBasic::AddBackgroundColorMenu(QMenu *menu, QWidgetAction *widgetAction, ColorSelect *select,
					obs_sceneitem_t *item)
{
	QAction *action;

	menu->setStyleSheet(QString("*[bgColor=\"1\"]{background-color:rgba(255,68,68,33%);}"
				    "*[bgColor=\"2\"]{background-color:rgba(255,255,68,33%);}"
				    "*[bgColor=\"3\"]{background-color:rgba(68,255,68,33%);}"
				    "*[bgColor=\"4\"]{background-color:rgba(68,255,255,33%);}"
				    "*[bgColor=\"5\"]{background-color:rgba(68,68,255,33%);}"
				    "*[bgColor=\"6\"]{background-color:rgba(255,68,255,33%);}"
				    "*[bgColor=\"7\"]{background-color:rgba(68,68,68,33%);}"
				    "*[bgColor=\"8\"]{background-color:rgba(255,255,255,33%);}"));

	obs_data_t *privData = obs_sceneitem_get_private_settings(item);
	obs_data_release(privData);

	obs_data_set_default_int(privData, "color-preset", 0);
	int preset = obs_data_get_int(privData, "color-preset");

	action = menu->addAction(QTStr("Clear"), this, &OBSBasic::ColorChange);
	action->setCheckable(true);
	action->setProperty("bgColor", 0);
	action->setChecked(preset == 0);

	action = menu->addAction(QTStr("CustomColor"), this, &OBSBasic::ColorChange);
	action->setCheckable(true);
	action->setProperty("bgColor", 1);
	action->setChecked(preset == 1);

	menu->addSeparator();

	widgetAction->setDefaultWidget(select);

	for (int i = 1; i < 9; i++) {
		stringstream button;
		button << "preset" << i;
		QPushButton *colorButton = select->findChild<QPushButton *>(button.str().c_str());
		if (preset == i + 1)
			colorButton->setStyleSheet("border: 2px solid black");

		colorButton->setProperty("bgColor", i);
		select->connect(colorButton, &QPushButton::released, this, &OBSBasic::ColorChange);
	}

	menu->addAction(widgetAction);

	return menu;
}

ColorSelect::ColorSelect(QWidget *parent) : QWidget(parent), ui(new Ui::ColorSelect)
{
	ui->setupUi(this);
}

void OBSBasic::CreateSourcePopupMenu(int idx, bool preview)
{
	QMenu popup(this);
	delete previewProjectorSource;
	delete sourceProjector;
	delete scaleFilteringMenu;
	delete blendingMethodMenu;
	delete blendingModeMenu;
	delete colorMenu;
	delete colorWidgetAction;
	delete colorSelect;
	delete deinterlaceMenu;

	if (preview) {
		QAction *action =
			popup.addAction(QTStr("Basic.Main.PreviewConextMenu.Enable"), this, &OBSBasic::TogglePreview);
		action->setCheckable(true);
		action->setChecked(obs_display_enabled(ui->preview->GetDisplay()));
		if (IsPreviewProgramMode())
			action->setEnabled(false);

		popup.addAction(ui->actionLockPreview);
		popup.addMenu(ui->scalingMenu);

		previewProjectorSource = new QMenu(QTStr("PreviewProjector"));
		AddProjectorMenuMonitors(previewProjectorSource, this, &OBSBasic::OpenPreviewProjector);

		popup.addMenu(previewProjectorSource);

		QAction *previewWindow = popup.addAction(QTStr("PreviewWindow"), this, &OBSBasic::OpenPreviewWindow);

		popup.addAction(previewWindow);

		popup.addAction(QTStr("Screenshot.Preview"), this, &OBSBasic::ScreenshotScene);

		popup.addSeparator();
	}

	QPointer<QMenu> addSourceMenu = CreateAddSourcePopupMenu();
	if (addSourceMenu)
		popup.addMenu(addSourceMenu);

	if (ui->sources->MultipleBaseSelected()) {
		popup.addSeparator();
		popup.addAction(QTStr("Basic.Main.GroupItems"), ui->sources, &SourceTree::GroupSelectedItems);

	} else if (ui->sources->GroupsSelected()) {
		popup.addSeparator();
		popup.addAction(QTStr("Basic.Main.Ungroup"), ui->sources, &SourceTree::UngroupSelectedGroups);
	}

	popup.addSeparator();
	popup.addAction(ui->actionCopySource);
	popup.addAction(ui->actionPasteRef);
	popup.addAction(ui->actionPasteDup);
	popup.addSeparator();

	popup.addSeparator();
	popup.addAction(ui->actionCopyFilters);
	popup.addAction(ui->actionPasteFilters);
	popup.addSeparator();

	if (idx != -1) {
		if (addSourceMenu)
			popup.addSeparator();

		OBSSceneItem sceneItem = ui->sources->Get(idx);
		obs_source_t *source = obs_sceneitem_get_source(sceneItem);
		uint32_t flags = obs_source_get_output_flags(source);
		bool isAsyncVideo = (flags & OBS_SOURCE_ASYNC_VIDEO) == OBS_SOURCE_ASYNC_VIDEO;
		bool hasAudio = (flags & OBS_SOURCE_AUDIO) == OBS_SOURCE_AUDIO;
		bool hasVideo = (flags & OBS_SOURCE_VIDEO) == OBS_SOURCE_VIDEO;

		colorMenu = new QMenu(QTStr("ChangeBG"));
		colorWidgetAction = new QWidgetAction(colorMenu);
		colorSelect = new ColorSelect(colorMenu);
		popup.addMenu(AddBackgroundColorMenu(colorMenu, colorWidgetAction, colorSelect, sceneItem));
		popup.addAction(renameSource);
		popup.addAction(ui->actionRemoveSource);
		popup.addSeparator();

		popup.addMenu(ui->orderMenu);

		if (hasVideo)
			popup.addMenu(ui->transformMenu);

		popup.addSeparator();

		if (hasAudio) {
			QAction *actionHideMixer =
				popup.addAction(QTStr("HideMixer"), this, &OBSBasic::ToggleHideMixer);
			actionHideMixer->setCheckable(true);
			actionHideMixer->setChecked(SourceMixerHidden(source));
			popup.addSeparator();
		}

		if (hasVideo) {
			QAction *resizeOutput = popup.addAction(QTStr("ResizeOutputSizeOfSource"), this,
								&OBSBasic::ResizeOutputSizeOfSource);

			int width = obs_source_get_width(source);
			int height = obs_source_get_height(source);

			resizeOutput->setEnabled(!obs_video_active());

			if (width < 32 || height < 32)
				resizeOutput->setEnabled(false);

			scaleFilteringMenu = new QMenu(QTStr("ScaleFiltering"));
			popup.addMenu(AddScaleFilteringMenu(scaleFilteringMenu, sceneItem));
			blendingModeMenu = new QMenu(QTStr("BlendingMode"));
			popup.addMenu(AddBlendingModeMenu(blendingModeMenu, sceneItem));
			blendingMethodMenu = new QMenu(QTStr("BlendingMethod"));
			popup.addMenu(AddBlendingMethodMenu(blendingMethodMenu, sceneItem));
			if (isAsyncVideo) {
				deinterlaceMenu = new QMenu(QTStr("Deinterlacing"));
				popup.addMenu(AddDeinterlacingMenu(deinterlaceMenu, source));
			}

			popup.addSeparator();

			popup.addMenu(CreateVisibilityTransitionMenu(true));
			popup.addMenu(CreateVisibilityTransitionMenu(false));
			popup.addSeparator();

			sourceProjector = new QMenu(QTStr("SourceProjector"));
			AddProjectorMenuMonitors(sourceProjector, this, &OBSBasic::OpenSourceProjector);
			popup.addMenu(sourceProjector);
			popup.addAction(QTStr("SourceWindow"), this, &OBSBasic::OpenSourceWindow);

			popup.addAction(QTStr("Screenshot.Source"), this, &OBSBasic::ScreenshotSelectedSource);
		}

		popup.addSeparator();

		if (flags & OBS_SOURCE_INTERACTION)
			popup.addAction(QTStr("Interact"), this, &OBSBasic::on_actionInteract_triggered);

		popup.addAction(QTStr("Filters"), this, [&]() { OpenFilters(); });
		QAction *action =
			popup.addAction(QTStr("Properties"), this, &OBSBasic::on_actionSourceProperties_triggered);
		action->setEnabled(obs_source_configurable(source));
	}

	popup.exec(QCursor::pos());
}

void OBSBasic::on_sources_customContextMenuRequested(const QPoint &pos)
{
	if (ui->scenes->count()) {
		QModelIndex idx = ui->sources->indexAt(pos);
		CreateSourcePopupMenu(idx.row(), false);
	}
}

void OBSBasic::on_scenes_itemDoubleClicked(QListWidgetItem *witem)
{
	if (!witem)
		return;

	if (IsPreviewProgramMode()) {
		bool doubleClickSwitch =
			config_get_bool(App()->GetUserConfig(), "BasicWindow", "TransitionOnDoubleClick");

		if (doubleClickSwitch)
			TransitionClicked();
	}
}

static inline bool should_show_properties(obs_source_t *source, const char *id)
{
	if (!source)
		return false;
	if (strcmp(id, "group") == 0)
		return false;
	if (!obs_source_configurable(source))
		return false;

	uint32_t caps = obs_source_get_output_flags(source);
	if ((caps & OBS_SOURCE_CAP_DONT_SHOW_PROPERTIES) != 0)
		return false;

	return true;
}

void OBSBasic::AddSource(const char *id)
{
	if (id && *id) {
		OBSBasicSourceSelect sourceSelect(this, id, undo_s);
		sourceSelect.exec();
		if (should_show_properties(sourceSelect.newSource, id)) {
			CreatePropertiesWindow(sourceSelect.newSource);
		}
	}
}

QMenu *OBSBasic::CreateAddSourcePopupMenu()
{
	const char *unversioned_type;
	const char *type;
	bool foundValues = false;
	bool foundDeprecated = false;
	size_t idx = 0;

	QMenu *popup = new QMenu(QTStr("Add"), this);
	QMenu *deprecated = new QMenu(QTStr("Deprecated"), popup);

	auto getActionAfter = [](QMenu *menu, const QString &name) {
		QList<QAction *> actions = menu->actions();

		for (QAction *menuAction : actions) {
			if (menuAction->text().compare(name, Qt::CaseInsensitive) >= 0)
				return menuAction;
		}

		return (QAction *)nullptr;
	};

	auto addSource = [this, getActionAfter](QMenu *popup, const char *type, const char *name) {
		QString qname = QT_UTF8(name);
		QAction *popupItem = new QAction(qname, this);
		connect(popupItem, &QAction::triggered, [this, type]() { AddSource(type); });

		QIcon icon;

		if (strcmp(type, "scene") == 0)
			icon = GetSceneIcon();
		else
			icon = GetSourceIcon(type);

		popupItem->setIcon(icon);

		QAction *after = getActionAfter(popup, qname);
		popup->insertAction(after, popupItem);
	};

	while (obs_enum_input_types2(idx++, &type, &unversioned_type)) {
		const char *name = obs_source_get_display_name(type);
		uint32_t caps = obs_get_source_output_flags(type);

		if ((caps & OBS_SOURCE_CAP_DISABLED) != 0)
			continue;

		if ((caps & OBS_SOURCE_DEPRECATED) == 0) {
			addSource(popup, unversioned_type, name);
		} else {
			addSource(deprecated, unversioned_type, name);
			foundDeprecated = true;
		}
		foundValues = true;
	}

	addSource(popup, "scene", Str("Basic.Scene"));

	popup->addSeparator();
	QAction *addGroup = new QAction(QTStr("Group"), this);
	addGroup->setIcon(GetGroupIcon());
	connect(addGroup, &QAction::triggered, [this]() { AddSource("group"); });
	popup->addAction(addGroup);

	if (!foundDeprecated) {
		delete deprecated;
		deprecated = nullptr;
	}

	if (!foundValues) {
		delete popup;
		popup = nullptr;

	} else if (foundDeprecated) {
		popup->addSeparator();
		popup->addMenu(deprecated);
	}

	return popup;
}

void OBSBasic::AddSourcePopupMenu(const QPoint &pos)
{
	if (!GetCurrentScene()) {
		// Tell the user he needs a scene first (help beginners).
		OBSMessageBox::information(this, QTStr("Basic.Main.AddSourceHelp.Title"),
					   QTStr("Basic.Main.AddSourceHelp.Text"));
		return;
	}

	QScopedPointer<QMenu> popup(CreateAddSourcePopupMenu());
	if (popup)
		popup->exec(pos);
}

void OBSBasic::on_actionAddSource_triggered()
{
	AddSourcePopupMenu(QCursor::pos());
}

static bool remove_items(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	vector<OBSSceneItem> &items = *reinterpret_cast<vector<OBSSceneItem> *>(param);

	if (obs_sceneitem_selected(item)) {
		items.emplace_back(item);
	} else if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, remove_items, &items);
	}
	return true;
};

OBSData OBSBasic::BackupScene(obs_scene_t *scene, std::vector<obs_source_t *> *sources)
{
	OBSDataArrayAutoRelease undo_array = obs_data_array_create();

	if (!sources) {
		obs_scene_enum_items(scene, save_undo_source_enum, undo_array);
	} else {
		for (obs_source_t *source : *sources) {
			obs_data_t *source_data = obs_save_source(source);
			obs_data_array_push_back(undo_array, source_data);
			obs_data_release(source_data);
		}
	}

	OBSDataAutoRelease scene_data = obs_save_source(obs_scene_get_source(scene));
	obs_data_array_push_back(undo_array, scene_data);

	OBSDataAutoRelease data = obs_data_create();

	obs_data_set_array(data, "array", undo_array);
	obs_data_get_json(data);
	return data.Get();
}

static bool add_source_enum(obs_scene_t *, obs_sceneitem_t *item, void *p)
{
	auto sources = static_cast<std::vector<OBSSource> *>(p);
	sources->push_back(obs_sceneitem_get_source(item));
	return true;
}

void OBSBasic::CreateSceneUndoRedoAction(const QString &action_name, OBSData undo_data, OBSData redo_data)
{
	auto undo_redo = [this](const std::string &json) {
		OBSDataAutoRelease base = obs_data_create_from_json(json.c_str());
		OBSDataArrayAutoRelease array = obs_data_get_array(base, "array");
		std::vector<OBSSource> sources;
		std::vector<OBSSource> old_sources;

		/* create missing sources */
		const size_t count = obs_data_array_count(array);
		sources.reserve(count);

		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease data = obs_data_array_item(array, i);
			const char *name = obs_data_get_string(data, "name");

			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			if (!source)
				source = obs_load_source(data);

			sources.push_back(source.Get());

			/* update scene/group settings to restore their
			 * contents to their saved settings */
			obs_scene_t *scene = obs_group_or_scene_from_source(source);
			if (scene) {
				obs_scene_enum_items(scene, add_source_enum, &old_sources);
				OBSDataAutoRelease scene_settings = obs_data_get_obj(data, "settings");
				obs_source_update(source, scene_settings);
			}
		}

		/* actually load sources now */
		for (obs_source_t *source : sources)
			obs_source_load2(source);

		ui->sources->RefreshItems();
	};

	const char *undo_json = obs_data_get_last_json(undo_data);
	const char *redo_json = obs_data_get_last_json(redo_data);

	undo_s.add_action(action_name, undo_redo, undo_redo, undo_json, redo_json);
}

void OBSBasic::on_actionRemoveSource_triggered()
{
	vector<OBSSceneItem> items;
	OBSScene scene = GetCurrentScene();
	obs_source_t *scene_source = obs_scene_get_source(scene);

	obs_scene_enum_items(scene, remove_items, &items);

	if (!items.size())
		return;

	/* ------------------------------------- */
	/* confirm action with user              */

	bool confirmed = false;

	if (items.size() > 1) {
		QString text = QTStr("ConfirmRemove.TextMultiple").arg(QString::number(items.size()));

		QMessageBox remove_items(this);
		remove_items.setText(text);
		QPushButton *Yes = remove_items.addButton(QTStr("Yes"), QMessageBox::YesRole);
		remove_items.setDefaultButton(Yes);
		remove_items.addButton(QTStr("No"), QMessageBox::NoRole);
		remove_items.setIcon(QMessageBox::Question);
		remove_items.setWindowTitle(QTStr("ConfirmRemove.Title"));
		remove_items.exec();

		confirmed = Yes == remove_items.clickedButton();
	} else {
		OBSSceneItem &item = items[0];
		obs_source_t *source = obs_sceneitem_get_source(item);
		if (source && QueryRemoveSource(source))
			confirmed = true;
	}
	if (!confirmed)
		return;

	/* ----------------------------------------------- */
	/* save undo data                                  */

	OBSData undo_data = BackupScene(scene_source);

	/* ----------------------------------------------- */
	/* remove items                                    */

	for (auto &item : items)
		obs_sceneitem_remove(item);

	/* ----------------------------------------------- */
	/* save redo data                                  */

	OBSData redo_data = BackupScene(scene_source);

	/* ----------------------------------------------- */
	/* add undo/redo action                            */

	QString action_name;
	if (items.size() > 1) {
		action_name = QTStr("Undo.Sources.Multi").arg(QString::number(items.size()));
	} else {
		QString str = QTStr("Undo.Delete");
		action_name = str.arg(obs_source_get_name(obs_sceneitem_get_source(items[0])));
	}

	CreateSceneUndoRedoAction(action_name, undo_data, redo_data);
}

void OBSBasic::on_actionInteract_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();
	OBSSource source = obs_sceneitem_get_source(item);

	if (source)
		CreateInteractionWindow(source);
}

void OBSBasic::on_actionSourceProperties_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();
	OBSSource source = obs_sceneitem_get_source(item);

	if (source)
		CreatePropertiesWindow(source);
}

void OBSBasic::MoveSceneItem(enum obs_order_movement movement, const QString &action_name)
{
	OBSSceneItem item = GetCurrentSceneItem();
	obs_source_t *source = obs_sceneitem_get_source(item);

	if (!source)
		return;

	OBSScene scene = GetCurrentScene();
	std::vector<obs_source_t *> sources;
	if (scene != obs_sceneitem_get_scene(item))
		sources.push_back(obs_scene_get_source(obs_sceneitem_get_scene(item)));

	OBSData undo_data = BackupScene(scene, &sources);

	obs_sceneitem_set_order(item, movement);

	const char *source_name = obs_source_get_name(source);
	const char *scene_name = obs_source_get_name(obs_scene_get_source(scene));

	OBSData redo_data = BackupScene(scene, &sources);
	CreateSceneUndoRedoAction(action_name.arg(source_name, scene_name), undo_data, redo_data);
}

void OBSBasic::on_actionSourceUp_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_UP, QTStr("Undo.MoveUp"));
}

void OBSBasic::on_actionSourceDown_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_DOWN, QTStr("Undo.MoveDown"));
}

void OBSBasic::on_actionMoveUp_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_UP, QTStr("Undo.MoveUp"));
}

void OBSBasic::on_actionMoveDown_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_DOWN, QTStr("Undo.MoveDown"));
}

void OBSBasic::on_actionMoveToTop_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_TOP, QTStr("Undo.MoveToTop"));
}

void OBSBasic::on_actionMoveToBottom_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_BOTTOM, QTStr("Undo.MoveToBottom"));
}

static BPtr<char> ReadLogFile(const char *subdir, const char *log)
{
	char logDir[512];
	if (GetAppConfigPath(logDir, sizeof(logDir), subdir) <= 0)
		return nullptr;

	string path = logDir;
	path += "/";
	path += log;

	BPtr<char> file = os_quick_read_utf8_file(path.c_str());
	if (!file)
		blog(LOG_WARNING, "Failed to read log file %s", path.c_str());

	return file;
}

void OBSBasic::UploadLog(const char *subdir, const char *file, const bool crash)
{
	BPtr<char> fileString{ReadLogFile(subdir, file)};

	if (!fileString)
		return;

	if (!*fileString)
		return;

	ui->menuLogFiles->setEnabled(false);
#if defined(_WIN32)
	ui->menuCrashLogs->setEnabled(false);
#endif

	stringstream ss;
	ss << "OBS " << App()->GetVersionString(false) << " log file uploaded at " << CurrentDateTimeString() << "\n\n"
	   << fileString;

	if (logUploadThread) {
		logUploadThread->wait();
	}

	RemoteTextThread *thread = new RemoteTextThread("https://obsproject.com/logs/upload", "text/plain", ss.str());

	logUploadThread.reset(thread);
	if (crash) {
		connect(thread, &RemoteTextThread::Result, this, &OBSBasic::crashUploadFinished);
	} else {
		connect(thread, &RemoteTextThread::Result, this, &OBSBasic::logUploadFinished);
	}
	logUploadThread->start();
}

void OBSBasic::on_actionShowLogs_triggered()
{
	char logDir[512];
	if (GetAppConfigPath(logDir, sizeof(logDir), "obs-studio/logs") <= 0)
		return;

	QUrl url = QUrl::fromLocalFile(QT_UTF8(logDir));
	QDesktopServices::openUrl(url);
}

void OBSBasic::on_actionUploadCurrentLog_triggered()
{
	UploadLog("obs-studio/logs", App()->GetCurrentLog(), false);
}

void OBSBasic::on_actionUploadLastLog_triggered()
{
	UploadLog("obs-studio/logs", App()->GetLastLog(), false);
}

void OBSBasic::on_actionViewCurrentLog_triggered()
{
	if (!logView)
		logView = new OBSLogViewer();

	logView->show();
	logView->setWindowState((logView->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	logView->activateWindow();
	logView->raise();
}

void OBSBasic::on_actionShowCrashLogs_triggered()
{
	char logDir[512];
	if (GetAppConfigPath(logDir, sizeof(logDir), "obs-studio/crashes") <= 0)
		return;

	QUrl url = QUrl::fromLocalFile(QT_UTF8(logDir));
	QDesktopServices::openUrl(url);
}

void OBSBasic::on_actionUploadLastCrashLog_triggered()
{
	UploadLog("obs-studio/crashes", App()->GetLastCrashLog(), true);
}

void OBSBasic::on_actionCheckForUpdates_triggered()
{
	CheckForUpdates(true);
}

void OBSBasic::on_actionRepair_triggered()
{
#if defined(_WIN32)
	ui->actionCheckForUpdates->setEnabled(false);
	ui->actionRepair->setEnabled(false);

	if (updateCheckThread && updateCheckThread->isRunning())
		return;

	updateCheckThread.reset(new AutoUpdateThread(false, true));
	updateCheckThread->start();
#endif
}

void OBSBasic::on_actionRestartSafe_triggered()
{
	QMessageBox::StandardButton button = OBSMessageBox::question(
		this, QTStr("Restart"), safe_mode ? QTStr("SafeMode.RestartNormal") : QTStr("SafeMode.Restart"));

	if (button == QMessageBox::Yes) {
		restart = safe_mode;
		restart_safe = !safe_mode;
		close();
	}
}

void OBSBasic::logUploadFinished(const QString &text, const QString &error)
{
	ui->menuLogFiles->setEnabled(true);
#if defined(_WIN32)
	ui->menuCrashLogs->setEnabled(true);
#endif

	if (text.isEmpty()) {
		OBSMessageBox::critical(this, QTStr("LogReturnDialog.ErrorUploadingLog"), error);
		return;
	}
	openLogDialog(text, false);
}

void OBSBasic::crashUploadFinished(const QString &text, const QString &error)
{
	ui->menuLogFiles->setEnabled(true);
#if defined(_WIN32)
	ui->menuCrashLogs->setEnabled(true);
#endif

	if (text.isEmpty()) {
		OBSMessageBox::critical(this, QTStr("LogReturnDialog.ErrorUploadingLog"), error);
		return;
	}
	openLogDialog(text, true);
}

void OBSBasic::openLogDialog(const QString &text, const bool crash)
{

	OBSDataAutoRelease returnData = obs_data_create_from_json(QT_TO_UTF8(text));
	string resURL = obs_data_get_string(returnData, "url");
	QString logURL = resURL.c_str();

	OBSLogReply logDialog(this, logURL, crash);
	logDialog.exec();
}

static void RenameListItem(OBSBasic *parent, QListWidget *listWidget, obs_source_t *source, const string &name)
{
	const char *prevName = obs_source_get_name(source);
	if (name == prevName)
		return;

	OBSSourceAutoRelease foundSource = obs_get_source_by_name(name.c_str());
	QListWidgetItem *listItem = listWidget->currentItem();

	if (foundSource || name.empty()) {
		listItem->setText(QT_UTF8(prevName));

		if (foundSource) {
			OBSMessageBox::warning(parent, QTStr("NameExists.Title"), QTStr("NameExists.Text"));
		} else if (name.empty()) {
			OBSMessageBox::warning(parent, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
		}
	} else {
		auto undo = [prev = std::string(prevName)](const std::string &data) {
			OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
			obs_source_set_name(source, prev.c_str());
		};

		auto redo = [name](const std::string &data) {
			OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
			obs_source_set_name(source, name.c_str());
		};

		std::string source_uuid(obs_source_get_uuid(source));
		parent->undo_s.add_action(QTStr("Undo.Rename").arg(name.c_str()), undo, redo, source_uuid, source_uuid);

		listItem->setText(QT_UTF8(name.c_str()));
		obs_source_set_name(source, name.c_str());
	}
}

void OBSBasic::SceneNameEdited(QWidget *editor)
{
	OBSScene scene = GetCurrentScene();
	QLineEdit *edit = qobject_cast<QLineEdit *>(editor);
	string text = QT_TO_UTF8(edit->text().trimmed());

	if (!scene)
		return;

	obs_source_t *source = obs_scene_get_source(scene);
	RenameListItem(this, ui->scenes, source, text);

	ui->scenesDock->addAction(renameScene);

	OnEvent(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED);
}

void OBSBasic::OpenFilters(OBSSource source)
{
	if (source == nullptr) {
		OBSSceneItem item = GetCurrentSceneItem();
		source = obs_sceneitem_get_source(item);
	}
	CreateFiltersWindow(source);
}

void OBSBasic::OpenProperties(OBSSource source)
{
	if (source == nullptr) {
		OBSSceneItem item = GetCurrentSceneItem();
		source = obs_sceneitem_get_source(item);
	}
	CreatePropertiesWindow(source);
}

void OBSBasic::OpenInteraction(OBSSource source)
{
	if (source == nullptr) {
		OBSSceneItem item = GetCurrentSceneItem();
		source = obs_sceneitem_get_source(item);
	}
	CreateInteractionWindow(source);
}

void OBSBasic::OpenEditTransform(OBSSceneItem item)
{
	if (!item)
		item = GetCurrentSceneItem();
	if (!item)
		return;
	CreateEditTransformWindow(item);
}

void OBSBasic::OpenSceneFilters()
{
	OBSScene scene = GetCurrentScene();
	OBSSource source = obs_scene_get_source(scene);

	CreateFiltersWindow(source);
}

#define RECORDING_START "==== Recording Start ==============================================="
#define RECORDING_STOP "==== Recording Stop ================================================"
#define REPLAY_BUFFER_START "==== Replay Buffer Start ==========================================="
#define REPLAY_BUFFER_STOP "==== Replay Buffer Stop ============================================"
#define STREAMING_START "==== Streaming Start ==============================================="
#define STREAMING_STOP "==== Streaming Stop ================================================"
#define VIRTUAL_CAM_START "==== Virtual Camera Start =========================================="
#define VIRTUAL_CAM_STOP "==== Virtual Camera Stop ==========================================="

void OBSBasic::DisplayStreamStartError()
{
	QString message = !outputHandler->lastError.empty() ? QTStr(outputHandler->lastError.c_str())
							    : QTStr("Output.StartFailedGeneric");

	emit StreamingStopped();

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StartStreaming"));
		sysTrayStream->setEnabled(true);
	}

	QMessageBox::critical(this, QTStr("Output.StartStreamFailed"), message);
}

#ifdef YOUTUBE_ENABLED
void OBSBasic::YouTubeActionDialogOk(const QString &broadcast_id, const QString &stream_id, const QString &key,
				     bool autostart, bool autostop, bool start_now)
{
	//blog(LOG_DEBUG, "Stream key: %s", QT_TO_UTF8(key));
	obs_service_t *service_obj = GetService();
	OBSDataAutoRelease settings = obs_service_get_settings(service_obj);

	const std::string a_key = QT_TO_UTF8(key);
	obs_data_set_string(settings, "key", a_key.c_str());

	const std::string b_id = QT_TO_UTF8(broadcast_id);
	obs_data_set_string(settings, "broadcast_id", b_id.c_str());

	const std::string s_id = QT_TO_UTF8(stream_id);
	obs_data_set_string(settings, "stream_id", s_id.c_str());

	obs_service_update(service_obj, settings);
	autoStartBroadcast = autostart;
	autoStopBroadcast = autostop;
	broadcastReady = true;

	emit BroadcastStreamReady(broadcastReady);

	if (start_now)
		QMetaObject::invokeMethod(this, "StartStreaming");
}

void OBSBasic::YoutubeStreamCheck(const std::string &key)
{
	YoutubeApiWrappers *apiYouTube(dynamic_cast<YoutubeApiWrappers *>(GetAuth()));
	if (!apiYouTube) {
		/* technically we should never get here -Lain */
		QMetaObject::invokeMethod(this, "ForceStopStreaming", Qt::QueuedConnection);
		youtubeStreamCheckThread->deleteLater();
		blog(LOG_ERROR, "==========================================");
		blog(LOG_ERROR, "%s: Uh, hey, we got here", __FUNCTION__);
		blog(LOG_ERROR, "==========================================");
		return;
	}

	int timeout = 0;
	json11::Json json;
	QString id = key.c_str();

	while (StreamingActive()) {
		if (timeout == 14) {
			QMetaObject::invokeMethod(this, "ForceStopStreaming", Qt::QueuedConnection);
			break;
		}

		if (!apiYouTube->FindStream(id, json)) {
			QMetaObject::invokeMethod(this, "DisplayStreamStartError", Qt::QueuedConnection);
			QMetaObject::invokeMethod(this, "StopStreaming", Qt::QueuedConnection);
			break;
		}

		auto item = json["items"][0];
		auto status = item["status"]["streamStatus"].string_value();
		if (status == "active") {
			emit BroadcastStreamActive();
			break;
		} else {
			QThread::sleep(1);
			timeout++;
		}
	}

	youtubeStreamCheckThread->deleteLater();
}

void OBSBasic::ShowYouTubeAutoStartWarning()
{
	auto msgBox = []() {
		QMessageBox msgbox(App()->GetMainWindow());
		msgbox.setWindowTitle(QTStr("YouTube.Actions.AutoStartStreamingWarning.Title"));
		msgbox.setText(QTStr("YouTube.Actions.AutoStartStreamingWarning"));
		msgbox.setIcon(QMessageBox::Icon::Information);
		msgbox.addButton(QMessageBox::Ok);

		QCheckBox *cb = new QCheckBox(QTStr("DoNotShowAgain"));
		msgbox.setCheckBox(cb);

		msgbox.exec();

		if (cb->isChecked()) {
			config_set_bool(App()->GetUserConfig(), "General", "WarnedAboutYouTubeAutoStart", true);
			config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
		}
	};

	bool warned = config_get_bool(App()->GetUserConfig(), "General", "WarnedAboutYouTubeAutoStart");
	if (!warned) {
		QMetaObject::invokeMethod(App(), "Exec", Qt::QueuedConnection, Q_ARG(VoidFunc, msgBox));
	}
}
#endif

void OBSBasic::StartStreaming()
{
	if (outputHandler->StreamingActive())
		return;
	if (disableOutputsRef)
		return;

	if (auth && auth->broadcastFlow()) {
		if (!broadcastActive && !broadcastReady) {
			QMessageBox no_broadcast(this);
			no_broadcast.setText(QTStr("Output.NoBroadcast.Text"));
			QPushButton *SetupBroadcast =
				no_broadcast.addButton(QTStr("Basic.Main.SetupBroadcast"), QMessageBox::YesRole);
			no_broadcast.setDefaultButton(SetupBroadcast);
			no_broadcast.addButton(QTStr("Close"), QMessageBox::NoRole);
			no_broadcast.setIcon(QMessageBox::Information);
			no_broadcast.setWindowTitle(QTStr("Output.NoBroadcast.Title"));
			no_broadcast.exec();

			if (no_broadcast.clickedButton() == SetupBroadcast)
				QMetaObject::invokeMethod(this, "SetupBroadcast");
			return;
		}
	}

	emit StreamingPreparing();

	if (sysTrayStream) {
		sysTrayStream->setEnabled(false);
		sysTrayStream->setText("Basic.Main.PreparingStream");
	}

	auto finish_stream_setup = [&](bool setupStreamingResult) {
		if (!setupStreamingResult) {
			DisplayStreamStartError();
			return;
		}

		OnEvent(OBS_FRONTEND_EVENT_STREAMING_STARTING);

		SaveProject();

		emit StreamingStarting(autoStartBroadcast);

		if (sysTrayStream)
			sysTrayStream->setText("Basic.Main.Connecting");

		if (!outputHandler->StartStreaming(service)) {
			DisplayStreamStartError();
			return;
		}

		if (autoStartBroadcast) {
			emit BroadcastStreamStarted(autoStopBroadcast);
			broadcastActive = true;
		}

		bool recordWhenStreaming =
			config_get_bool(App()->GetUserConfig(), "BasicWindow", "RecordWhenStreaming");
		if (recordWhenStreaming)
			StartRecording();

		bool replayBufferWhileStreaming =
			config_get_bool(App()->GetUserConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
		if (replayBufferWhileStreaming)
			StartReplayBuffer();

#ifdef YOUTUBE_ENABLED
		if (!autoStartBroadcast)
			OBSBasic::ShowYouTubeAutoStartWarning();
#endif
	};

	setupStreamingGuard = outputHandler->SetupStreaming(service, finish_stream_setup);
}

void OBSBasic::BroadcastButtonClicked()
{
	if (!broadcastReady || (!broadcastActive && !outputHandler->StreamingActive())) {
		SetupBroadcast();
		return;
	}

	if (!autoStartBroadcast) {
#ifdef YOUTUBE_ENABLED
		std::shared_ptr<YoutubeApiWrappers> ytAuth = dynamic_pointer_cast<YoutubeApiWrappers>(auth);
		if (ytAuth.get()) {
			if (!ytAuth->StartLatestBroadcast()) {
				auto last_error = ytAuth->GetLastError();
				if (last_error.isEmpty())
					last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
				if (!ytAuth->GetTranslatedError(last_error))
					last_error = QTStr("YouTube.Actions.Error.BroadcastTransitionFailed")
							     .arg(last_error, ytAuth->GetBroadcastId());

				OBSMessageBox::warning(this, QTStr("Output.BroadcastStartFailed"), last_error, true);
				return;
			}
		}
#endif
		broadcastActive = true;
		autoStartBroadcast = true; // and clear the flag

		emit BroadcastStreamStarted(autoStopBroadcast);
	} else if (!autoStopBroadcast) {
#ifdef YOUTUBE_ENABLED
		bool confirm = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingStream");
		if (confirm && isVisible()) {
			QMessageBox::StandardButton button = OBSMessageBox::question(
				this, QTStr("ConfirmStop.Title"), QTStr("YouTube.Actions.AutoStopStreamingWarning"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;
		}

		std::shared_ptr<YoutubeApiWrappers> ytAuth = dynamic_pointer_cast<YoutubeApiWrappers>(auth);
		if (ytAuth.get()) {
			if (!ytAuth->StopLatestBroadcast()) {
				auto last_error = ytAuth->GetLastError();
				if (last_error.isEmpty())
					last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
				if (!ytAuth->GetTranslatedError(last_error))
					last_error = QTStr("YouTube.Actions.Error.BroadcastTransitionFailed")
							     .arg(last_error, ytAuth->GetBroadcastId());

				OBSMessageBox::warning(this, QTStr("Output.BroadcastStopFailed"), last_error, true);
			}
		}
#endif
		broadcastActive = false;
		broadcastReady = false;

		autoStopBroadcast = true;
		QMetaObject::invokeMethod(this, "StopStreaming");
		emit BroadcastStreamReady(broadcastReady);
		SetBroadcastFlowEnabled(true);
	}
}

void OBSBasic::SetBroadcastFlowEnabled(bool enabled)
{
	emit BroadcastFlowEnabled(enabled);
}

void OBSBasic::SetupBroadcast()
{
#ifdef YOUTUBE_ENABLED
	Auth *const auth = GetAuth();
	if (IsYouTubeService(auth->service())) {
		OBSYoutubeActions dialog(this, auth, broadcastReady);
		connect(&dialog, &OBSYoutubeActions::ok, this, &OBSBasic::YouTubeActionDialogOk);
		dialog.exec();
	}
#endif
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

inline void OBSBasic::OnActivate(bool force)
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
			trayIcon->setIcon(QIcon::fromTheme("obs-tray-active", QIcon(":/res/images/tray_active.png")));
#endif
		}
	}
}

extern volatile bool recording_paused;
extern volatile bool replaybuf_active;

inline void OBSBasic::OnDeactivate()
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

void OBSBasic::StopStreaming()
{
	SaveProject();

	if (outputHandler->StreamingActive())
		outputHandler->StopStreaming(streamingStopping);

	// special case: force reset broadcast state if
	// no autostart and no autostop selected
	if (!autoStartBroadcast && !broadcastActive) {
		broadcastActive = false;
		autoStartBroadcast = true;
		autoStopBroadcast = true;
		broadcastReady = false;
	}

	if (autoStopBroadcast) {
		broadcastActive = false;
		broadcastReady = false;
	}

	emit BroadcastStreamReady(broadcastReady);

	OnDeactivate();

	bool recordWhenStreaming = config_get_bool(App()->GetUserConfig(), "BasicWindow", "RecordWhenStreaming");
	bool keepRecordingWhenStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
	if (recordWhenStreaming && !keepRecordingWhenStreamStops)
		StopRecording();

	bool replayBufferWhileStreaming =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
	bool keepReplayBufferStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
	if (replayBufferWhileStreaming && !keepReplayBufferStreamStops)
		StopReplayBuffer();
}

void OBSBasic::ForceStopStreaming()
{
	SaveProject();

	if (outputHandler->StreamingActive())
		outputHandler->StopStreaming(true);

	// special case: force reset broadcast state if
	// no autostart and no autostop selected
	if (!autoStartBroadcast && !broadcastActive) {
		broadcastActive = false;
		autoStartBroadcast = true;
		autoStopBroadcast = true;
		broadcastReady = false;
	}

	if (autoStopBroadcast) {
		broadcastActive = false;
		broadcastReady = false;
	}

	emit BroadcastStreamReady(broadcastReady);

	OnDeactivate();

	bool recordWhenStreaming = config_get_bool(App()->GetUserConfig(), "BasicWindow", "RecordWhenStreaming");
	bool keepRecordingWhenStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
	if (recordWhenStreaming && !keepRecordingWhenStreamStops)
		StopRecording();

	bool replayBufferWhileStreaming =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
	bool keepReplayBufferStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
	if (replayBufferWhileStreaming && !keepReplayBufferStreamStops)
		StopReplayBuffer();
}

void OBSBasic::StreamDelayStarting(int sec)
{
	emit StreamingStarted(true);

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StopStreaming"));
		sysTrayStream->setEnabled(true);
	}

	ui->statusbar->StreamDelayStarting(sec);

	OnActivate();
}

void OBSBasic::StreamDelayStopping(int sec)
{
	emit StreamingStopped(true);

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StartStreaming"));
		sysTrayStream->setEnabled(true);
	}

	ui->statusbar->StreamDelayStopping(sec);

	OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPING);
}

void OBSBasic::StreamingStart()
{
	emit StreamingStarted();
	OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
	ui->statusbar->StreamStarted(output);

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StopStreaming"));
		sysTrayStream->setEnabled(true);
	}

#ifdef YOUTUBE_ENABLED
	if (!autoStartBroadcast) {
		// get a current stream key
		obs_service_t *service_obj = GetService();
		OBSDataAutoRelease settings = obs_service_get_settings(service_obj);
		std::string key = obs_data_get_string(settings, "stream_id");
		if (!key.empty() && !youtubeStreamCheckThread) {
			youtubeStreamCheckThread = CreateQThread([this, key] { YoutubeStreamCheck(key); });
			youtubeStreamCheckThread->setObjectName("YouTubeStreamCheckThread");
			youtubeStreamCheckThread->start();
		}
	}
#endif

	OnEvent(OBS_FRONTEND_EVENT_STREAMING_STARTED);

	OnActivate();

#ifdef YOUTUBE_ENABLED
	if (YouTubeAppDock::IsYTServiceSelected())
		youtubeAppDock->IngestionStarted();
#endif

	blog(LOG_INFO, STREAMING_START);
}

void OBSBasic::StreamStopping()
{
	emit StreamingStopping();

	if (sysTrayStream)
		sysTrayStream->setText(QTStr("Basic.Main.StoppingStreaming"));

	streamingStopping = true;
	OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPING);
}

void OBSBasic::StreamingStop(int code, QString last_error)
{
	const char *errorDescription = "";
	DStr errorMessage;
	bool use_last_error = false;
	bool encode_error = false;

	switch (code) {
	case OBS_OUTPUT_BAD_PATH:
		errorDescription = Str("Output.ConnectFail.BadPath");
		break;

	case OBS_OUTPUT_CONNECT_FAILED:
		use_last_error = true;
		errorDescription = Str("Output.ConnectFail.ConnectFailed");
		break;

	case OBS_OUTPUT_INVALID_STREAM:
		errorDescription = Str("Output.ConnectFail.InvalidStream");
		break;

	case OBS_OUTPUT_ENCODE_ERROR:
		encode_error = true;
		break;

	case OBS_OUTPUT_HDR_DISABLED:
		errorDescription = Str("Output.ConnectFail.HdrDisabled");
		break;

	default:
	case OBS_OUTPUT_ERROR:
		use_last_error = true;
		errorDescription = Str("Output.ConnectFail.Error");
		break;

	case OBS_OUTPUT_DISCONNECTED:
		/* doesn't happen if output is set to reconnect.  note that
		 * reconnects are handled in the output, not in the UI */
		use_last_error = true;
		errorDescription = Str("Output.ConnectFail.Disconnected");
	}

	if (use_last_error && !last_error.isEmpty())
		dstr_printf(errorMessage, "%s\n\n%s", errorDescription, QT_TO_UTF8(last_error));
	else
		dstr_copy(errorMessage, errorDescription);

	ui->statusbar->StreamStopped();

	emit StreamingStopped();

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StartStreaming"));
		sysTrayStream->setEnabled(true);
	}

	streamingStopping = false;
	OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPED);

	OnDeactivate();

#ifdef YOUTUBE_ENABLED
	if (YouTubeAppDock::IsYTServiceSelected())
		youtubeAppDock->IngestionStopped();
#endif

	blog(LOG_INFO, STREAMING_STOP);

	if (encode_error) {
		QString msg = last_error.isEmpty() ? QTStr("Output.StreamEncodeError.Msg")
						   : QTStr("Output.StreamEncodeError.Msg.LastError").arg(last_error);
		OBSMessageBox::information(this, QTStr("Output.StreamEncodeError.Title"), msg);

	} else if (code != OBS_OUTPUT_SUCCESS && isVisible()) {
		OBSMessageBox::information(this, QTStr("Output.ConnectFail.Title"), QT_UTF8(errorMessage));

	} else if (code != OBS_OUTPUT_SUCCESS && !isVisible()) {
		SysTrayNotify(QT_UTF8(errorDescription), QSystemTrayIcon::Warning);
	}

	// Reset broadcast button state/text
	if (!broadcastActive)
		SetBroadcastFlowEnabled(auth && auth->broadcastFlow());
}

void OBSBasic::AutoRemux(QString input, bool no_show)
{
	auto config = Config();

	bool autoRemux = config_get_bool(config, "Video", "AutoRemux");

	if (!autoRemux)
		return;

	bool isSimpleMode = false;

	const char *mode = config_get_string(config, "Output", "Mode");
	if (!mode) {
		isSimpleMode = true;
	} else {
		isSimpleMode = strcmp(mode, "Simple") == 0;
	}

	if (!isSimpleMode) {
		const char *recType = config_get_string(config, "AdvOut", "RecType");

		bool ffmpegOutput = astrcmpi(recType, "FFmpeg") == 0;

		if (ffmpegOutput)
			return;
	}

	if (input.isEmpty())
		return;

	QFileInfo fi(input);
	QString suffix = fi.suffix();

	/* do not remux if lossless */
	if (suffix.compare("avi", Qt::CaseInsensitive) == 0) {
		return;
	}

	QString path = fi.path();

	QString output = input;
	output.resize(output.size() - suffix.size());

	const obs_encoder_t *videoEncoder = obs_output_get_video_encoder(outputHandler->fileOutput);
	const char *vCodecName = obs_encoder_get_codec(videoEncoder);
	const char *format = config_get_string(config, isSimpleMode ? "SimpleOutput" : "AdvOut", "RecFormat2");

	/* Retain original container for fMP4/fMOV */
	if (strncmp(format, "fragmented", 10) == 0) {
		output += "remuxed." + suffix;
	} else if (strcmp(vCodecName, "prores") == 0) {
		output += "mov";
	} else {
		output += "mp4";
	}

	OBSRemux *remux = new OBSRemux(QT_TO_UTF8(path), this, true);
	if (!no_show)
		remux->show();
	remux->AutoRemux(input, output);
}

void OBSBasic::StartRecording()
{
	if (outputHandler->RecordingActive())
		return;
	if (disableOutputsRef)
		return;

	if (!OutputPathValid()) {
		OutputPathInvalidMessage();
		return;
	}

	if (!IsFFmpegOutputToURL() && LowDiskSpace()) {
		DiskSpaceMessage();
		return;
	}

	OnEvent(OBS_FRONTEND_EVENT_RECORDING_STARTING);

	SaveProject();

	outputHandler->StartRecording();
}

void OBSBasic::RecordStopping()
{
	emit RecordingStopping();

	if (sysTrayRecord)
		sysTrayRecord->setText(QTStr("Basic.Main.StoppingRecording"));

	recordingStopping = true;
	OnEvent(OBS_FRONTEND_EVENT_RECORDING_STOPPING);
}

void OBSBasic::StopRecording()
{
	SaveProject();

	if (outputHandler->RecordingActive())
		outputHandler->StopRecording(recordingStopping);

	OnDeactivate();
}

void OBSBasic::RecordingStart()
{
	ui->statusbar->RecordingStarted(outputHandler->fileOutput);
	emit RecordingStarted(isRecordingPausable);

	if (sysTrayRecord)
		sysTrayRecord->setText(QTStr("Basic.Main.StopRecording"));

	recordingStopping = false;
	OnEvent(OBS_FRONTEND_EVENT_RECORDING_STARTED);

	if (!diskFullTimer->isActive())
		diskFullTimer->start(1000);

	OnActivate();

	blog(LOG_INFO, RECORDING_START);
}

void OBSBasic::RecordingStop(int code, QString last_error)
{
	ui->statusbar->RecordingStopped();
	emit RecordingStopped();

	if (sysTrayRecord)
		sysTrayRecord->setText(QTStr("Basic.Main.StartRecording"));

	blog(LOG_INFO, RECORDING_STOP);

	if (code == OBS_OUTPUT_UNSUPPORTED && isVisible()) {
		OBSMessageBox::critical(this, QTStr("Output.RecordFail.Title"), QTStr("Output.RecordFail.Unsupported"));

	} else if (code == OBS_OUTPUT_ENCODE_ERROR && isVisible()) {
		QString msg = last_error.isEmpty()
				      ? QTStr("Output.RecordError.EncodeErrorMsg")
				      : QTStr("Output.RecordError.EncodeErrorMsg.LastError").arg(last_error);
		OBSMessageBox::warning(this, QTStr("Output.RecordError.Title"), msg);

	} else if (code == OBS_OUTPUT_NO_SPACE && isVisible()) {
		OBSMessageBox::warning(this, QTStr("Output.RecordNoSpace.Title"), QTStr("Output.RecordNoSpace.Msg"));

	} else if (code != OBS_OUTPUT_SUCCESS && isVisible()) {

		const char *errorDescription;
		DStr errorMessage;
		bool use_last_error = true;

		errorDescription = Str("Output.RecordError.Msg");

		if (use_last_error && !last_error.isEmpty())
			dstr_printf(errorMessage, "%s<br><br>%s", errorDescription, QT_TO_UTF8(last_error));
		else
			dstr_copy(errorMessage, errorDescription);

		OBSMessageBox::critical(this, QTStr("Output.RecordError.Title"), QT_UTF8(errorMessage));

	} else if (code == OBS_OUTPUT_UNSUPPORTED && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordFail.Unsupported"), QSystemTrayIcon::Warning);

	} else if (code == OBS_OUTPUT_NO_SPACE && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordNoSpace.Msg"), QSystemTrayIcon::Warning);

	} else if (code != OBS_OUTPUT_SUCCESS && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordError.Msg"), QSystemTrayIcon::Warning);
	} else if (code == OBS_OUTPUT_SUCCESS) {
		if (outputHandler) {
			std::string path = outputHandler->lastRecordingPath;
			QString str = QTStr("Basic.StatusBar.RecordingSavedTo");
			ShowStatusBarMessage(str.arg(QT_UTF8(path.c_str())));
		}
	}

	OnEvent(OBS_FRONTEND_EVENT_RECORDING_STOPPED);

	if (diskFullTimer->isActive())
		diskFullTimer->stop();

	AutoRemux(outputHandler->lastRecordingPath.c_str());

	OnDeactivate();
}

void OBSBasic::RecordingFileChanged(QString lastRecordingPath)
{
	QString str = QTStr("Basic.StatusBar.RecordingSavedTo");
	ShowStatusBarMessage(str.arg(lastRecordingPath));

	AutoRemux(lastRecordingPath, true);
}

void OBSBasic::ShowReplayBufferPauseWarning()
{
	auto msgBox = []() {
		QMessageBox msgbox(App()->GetMainWindow());
		msgbox.setWindowTitle(QTStr("Output.ReplayBuffer."
					    "PauseWarning.Title"));
		msgbox.setText(QTStr("Output.ReplayBuffer."
				     "PauseWarning.Text"));
		msgbox.setIcon(QMessageBox::Icon::Information);
		msgbox.addButton(QMessageBox::Ok);

		QCheckBox *cb = new QCheckBox(QTStr("DoNotShowAgain"));
		msgbox.setCheckBox(cb);

		msgbox.exec();

		if (cb->isChecked()) {
			config_set_bool(App()->GetUserConfig(), "General", "WarnedAboutReplayBufferPausing", true);
			config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
		}
	};

	bool warned = config_get_bool(App()->GetUserConfig(), "General", "WarnedAboutReplayBufferPausing");
	if (!warned) {
		QMetaObject::invokeMethod(App(), "Exec", Qt::QueuedConnection, Q_ARG(VoidFunc, msgBox));
	}
}

void OBSBasic::StartReplayBuffer()
{
	if (!outputHandler || !outputHandler->replayBuffer)
		return;
	if (outputHandler->ReplayBufferActive())
		return;
	if (disableOutputsRef)
		return;

	if (!UIValidation::NoSourcesConfirmation(this))
		return;

	if (!OutputPathValid()) {
		OutputPathInvalidMessage();
		return;
	}

	if (LowDiskSpace()) {
		DiskSpaceMessage();
		return;
	}

	OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING);

	SaveProject();

	if (outputHandler->StartReplayBuffer() && os_atomic_load_bool(&recording_paused)) {
		ShowReplayBufferPauseWarning();
	}
}

void OBSBasic::ReplayBufferStopping()
{
	if (!outputHandler || !outputHandler->replayBuffer)
		return;

	emit ReplayBufStopping();

	if (sysTrayReplayBuffer)
		sysTrayReplayBuffer->setText(QTStr("Basic.Main.StoppingReplayBuffer"));

	replayBufferStopping = true;
	OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING);
}

void OBSBasic::StopReplayBuffer()
{
	if (!outputHandler || !outputHandler->replayBuffer)
		return;

	SaveProject();

	if (outputHandler->ReplayBufferActive())
		outputHandler->StopReplayBuffer(replayBufferStopping);

	OnDeactivate();
}

void OBSBasic::ReplayBufferStart()
{
	if (!outputHandler || !outputHandler->replayBuffer)
		return;

	emit ReplayBufStarted();

	if (sysTrayReplayBuffer)
		sysTrayReplayBuffer->setText(QTStr("Basic.Main.StopReplayBuffer"));

	replayBufferStopping = false;
	OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED);

	OnActivate();

	blog(LOG_INFO, REPLAY_BUFFER_START);
}

void OBSBasic::ReplayBufferSave()
{
	if (!outputHandler || !outputHandler->replayBuffer)
		return;
	if (!outputHandler->ReplayBufferActive())
		return;

	calldata_t cd = {0};
	proc_handler_t *ph = obs_output_get_proc_handler(outputHandler->replayBuffer);
	proc_handler_call(ph, "save", &cd);
	calldata_free(&cd);
}

void OBSBasic::ReplayBufferSaved()
{
	if (!outputHandler || !outputHandler->replayBuffer)
		return;
	if (!outputHandler->ReplayBufferActive())
		return;

	calldata_t cd = {0};
	proc_handler_t *ph = obs_output_get_proc_handler(outputHandler->replayBuffer);
	proc_handler_call(ph, "get_last_replay", &cd);
	std::string path = calldata_string(&cd, "path");
	QString msg = QTStr("Basic.StatusBar.ReplayBufferSavedTo").arg(QT_UTF8(path.c_str()));
	ShowStatusBarMessage(msg);
	lastReplay = path;
	calldata_free(&cd);

	OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED);

	AutoRemux(QT_UTF8(path.c_str()));
}

void OBSBasic::ReplayBufferStop(int code)
{
	if (!outputHandler || !outputHandler->replayBuffer)
		return;

	emit ReplayBufStopped();

	if (sysTrayReplayBuffer)
		sysTrayReplayBuffer->setText(QTStr("Basic.Main.StartReplayBuffer"));

	blog(LOG_INFO, REPLAY_BUFFER_STOP);

	if (code == OBS_OUTPUT_UNSUPPORTED && isVisible()) {
		OBSMessageBox::critical(this, QTStr("Output.RecordFail.Title"), QTStr("Output.RecordFail.Unsupported"));

	} else if (code == OBS_OUTPUT_NO_SPACE && isVisible()) {
		OBSMessageBox::warning(this, QTStr("Output.RecordNoSpace.Title"), QTStr("Output.RecordNoSpace.Msg"));

	} else if (code != OBS_OUTPUT_SUCCESS && isVisible()) {
		OBSMessageBox::critical(this, QTStr("Output.RecordError.Title"), QTStr("Output.RecordError.Msg"));

	} else if (code == OBS_OUTPUT_UNSUPPORTED && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordFail.Unsupported"), QSystemTrayIcon::Warning);

	} else if (code == OBS_OUTPUT_NO_SPACE && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordNoSpace.Msg"), QSystemTrayIcon::Warning);

	} else if (code != OBS_OUTPUT_SUCCESS && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordError.Msg"), QSystemTrayIcon::Warning);
	}

	OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED);

	OnDeactivate();
}

void OBSBasic::StartVirtualCam()
{
	if (!outputHandler || !outputHandler->virtualCam)
		return;
	if (outputHandler->VirtualCamActive())
		return;
	if (disableOutputsRef)
		return;

	SaveProject();

	outputHandler->StartVirtualCam();
}

void OBSBasic::StopVirtualCam()
{
	if (!outputHandler || !outputHandler->virtualCam)
		return;

	SaveProject();

	if (outputHandler->VirtualCamActive())
		outputHandler->StopVirtualCam();

	OnDeactivate();
}

void OBSBasic::OnVirtualCamStart()
{
	if (!outputHandler || !outputHandler->virtualCam)
		return;

	emit VirtualCamStarted();

	if (sysTrayVirtualCam)
		sysTrayVirtualCam->setText(QTStr("Basic.Main.StopVirtualCam"));

	OnEvent(OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED);

	OnActivate();

	blog(LOG_INFO, VIRTUAL_CAM_START);
}

void OBSBasic::OnVirtualCamStop(int)
{
	if (!outputHandler || !outputHandler->virtualCam)
		return;

	emit VirtualCamStopped();

	if (sysTrayVirtualCam)
		sysTrayVirtualCam->setText(QTStr("Basic.Main.StartVirtualCam"));

	OnEvent(OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED);

	blog(LOG_INFO, VIRTUAL_CAM_STOP);

	OnDeactivate();

	if (!restartingVCam)
		return;

	/* Restarting needs to be delayed to make sure that the virtual camera
	 * implementation is stopped and avoid race condition. */
	QTimer::singleShot(100, this, &OBSBasic::RestartingVirtualCam);
}

void OBSBasic::StreamActionTriggered()
{
	if (outputHandler->StreamingActive()) {
		bool confirm = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingStream");

#ifdef YOUTUBE_ENABLED
		if (isVisible() && auth && IsYouTubeService(auth->service()) && autoStopBroadcast) {
			QMessageBox::StandardButton button = OBSMessageBox::question(
				this, QTStr("ConfirmStop.Title"), QTStr("YouTube.Actions.AutoStopStreamingWarning"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;

			confirm = false;
		}
#endif
		if (confirm && isVisible()) {
			QMessageBox::StandardButton button =
				OBSMessageBox::question(this, QTStr("ConfirmStop.Title"), QTStr("ConfirmStop.Text"),
							QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;
		}

		StopStreaming();
	} else {
		if (!UIValidation::NoSourcesConfirmation(this))
			return;

		Auth *auth = GetAuth();

		auto action = (auth && auth->external()) ? StreamSettingsAction::ContinueStream
							 : UIValidation::StreamSettingsConfirmation(this, service);
		switch (action) {
		case StreamSettingsAction::ContinueStream:
			break;
		case StreamSettingsAction::OpenSettings:
			on_action_Settings_triggered();
			return;
		case StreamSettingsAction::Cancel:
			return;
		}

		bool confirm = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStartingStream");

		bool bwtest = false;

		if (this->auth) {
			OBSDataAutoRelease settings = obs_service_get_settings(service);
			bwtest = obs_data_get_bool(settings, "bwtest");
			// Disable confirmation if this is going to open broadcast setup
			if (auth && auth->broadcastFlow() && !broadcastReady && !broadcastActive)
				confirm = false;
		}

		if (bwtest && isVisible()) {
			QMessageBox::StandardButton button = OBSMessageBox::question(this, QTStr("ConfirmBWTest.Title"),
										     QTStr("ConfirmBWTest.Text"));

			if (button == QMessageBox::No)
				return;
		} else if (confirm && isVisible()) {
			QMessageBox::StandardButton button =
				OBSMessageBox::question(this, QTStr("ConfirmStart.Title"), QTStr("ConfirmStart.Text"),
							QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;
		}

		StartStreaming();
	}
}

void OBSBasic::RecordActionTriggered()
{
	if (outputHandler->RecordingActive()) {
		bool confirm = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingRecord");

		if (confirm && isVisible()) {
			QMessageBox::StandardButton button = OBSMessageBox::question(
				this, QTStr("ConfirmStopRecord.Title"), QTStr("ConfirmStopRecord.Text"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;
		}
		StopRecording();
	} else {
		if (!UIValidation::NoSourcesConfirmation(this))
			return;

		StartRecording();
	}
}

void OBSBasic::VirtualCamActionTriggered()
{
	if (outputHandler->VirtualCamActive()) {
		StopVirtualCam();
	} else {
		if (!UIValidation::NoSourcesConfirmation(this))
			return;

		StartVirtualCam();
	}
}

void OBSBasic::OpenVirtualCamConfig()
{
	OBSBasicVCamConfig dialog(vcamConfig, outputHandler->VirtualCamActive(), this);

	connect(&dialog, &OBSBasicVCamConfig::Accepted, this, &OBSBasic::UpdateVirtualCamConfig);
	connect(&dialog, &OBSBasicVCamConfig::AcceptedAndRestart, this, &OBSBasic::RestartVirtualCam);

	dialog.exec();
}

void log_vcam_changed(const VCamConfig &config, bool starting)
{
	const char *action = starting ? "Starting" : "Changing";

	switch (config.type) {
	case VCamOutputType::Invalid:
		break;
	case VCamOutputType::ProgramView:
		blog(LOG_INFO, "%s Virtual Camera output to Program", action);
		break;
	case VCamOutputType::PreviewOutput:
		blog(LOG_INFO, "%s Virtual Camera output to Preview", action);
		break;
	case VCamOutputType::SceneOutput:
		blog(LOG_INFO, "%s Virtual Camera output to Scene : %s", action, config.scene.c_str());
		break;
	case VCamOutputType::SourceOutput:
		blog(LOG_INFO, "%s Virtual Camera output to Source : %s", action, config.source.c_str());
		break;
	}
}

void OBSBasic::UpdateVirtualCamConfig(const VCamConfig &config)
{
	vcamConfig = config;

	outputHandler->UpdateVirtualCamOutputSource();
	log_vcam_changed(config, false);
}

void OBSBasic::RestartVirtualCam(const VCamConfig &config)
{
	restartingVCam = true;

	StopVirtualCam();

	vcamConfig = config;
}

void OBSBasic::RestartingVirtualCam()
{
	if (!restartingVCam)
		return;

	outputHandler->UpdateVirtualCamOutputSource();
	StartVirtualCam();
	restartingVCam = false;
}

void OBSBasic::on_actionHelpPortal_triggered()
{
	QUrl url = QUrl("https://obsproject.com/help", QUrl::TolerantMode);
	QDesktopServices::openUrl(url);
}

void OBSBasic::on_actionWebsite_triggered()
{
	QUrl url = QUrl("https://obsproject.com", QUrl::TolerantMode);
	QDesktopServices::openUrl(url);
}

void OBSBasic::on_actionDiscord_triggered()
{
	QUrl url = QUrl("https://obsproject.com/discord", QUrl::TolerantMode);
	QDesktopServices::openUrl(url);
}

void OBSBasic::on_actionShowWhatsNew_triggered()
{
#ifdef WHATSNEW_ENABLED
	if (introCheckThread && introCheckThread->isRunning())
		return;
	if (!cef)
		return;

	config_set_int(App()->GetAppConfig(), "General", "InfoIncrement", -1);

	WhatsNewInfoThread *wnit = new WhatsNewInfoThread();
	connect(wnit, &WhatsNewInfoThread::Result, this, &OBSBasic::ReceivedIntroJson, Qt::QueuedConnection);

	introCheckThread.reset(wnit);
	introCheckThread->start();
#endif
}

void OBSBasic::on_actionReleaseNotes_triggered()
{
	QString addr("https://github.com/obsproject/obs-studio/releases");
	QUrl url(QString("%1/%2").arg(addr, obs_get_version_string()), QUrl::TolerantMode);
	QDesktopServices::openUrl(url);
}

void OBSBasic::on_actionShowSettingsFolder_triggered()
{
	const std::string userConfigPath = App()->userConfigLocation.u8string() + "/obs-studio";
	const QString userConfigLocation = QString::fromStdString(userConfigPath);

	QDesktopServices::openUrl(QUrl::fromLocalFile(userConfigLocation));
}

void OBSBasic::on_actionShowProfileFolder_triggered()
{
	try {
		const OBSProfile &currentProfile = GetCurrentProfile();
		QString currentProfileLocation = QString::fromStdString(currentProfile.path.u8string());

		QDesktopServices::openUrl(QUrl::fromLocalFile(currentProfileLocation));
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

int OBSBasic::GetTopSelectedSourceItem()
{
	QModelIndexList selectedItems = ui->sources->selectionModel()->selectedIndexes();
	return selectedItems.count() ? selectedItems[0].row() : -1;
}

QModelIndexList OBSBasic::GetAllSelectedSourceItems()
{
	return ui->sources->selectionModel()->selectedIndexes();
}

void OBSBasic::on_preview_customContextMenuRequested()
{
	CreateSourcePopupMenu(GetTopSelectedSourceItem(), true);
}

void OBSBasic::ProgramViewContextMenuRequested()
{
	QMenu popup(this);
	QPointer<QMenu> studioProgramProjector;

	studioProgramProjector = new QMenu(QTStr("StudioProgramProjector"));
	AddProjectorMenuMonitors(studioProgramProjector, this, &OBSBasic::OpenStudioProgramProjector);

	popup.addMenu(studioProgramProjector);
	popup.addAction(QTStr("StudioProgramWindow"), this, &OBSBasic::OpenStudioProgramWindow);
	popup.addAction(QTStr("Screenshot.StudioProgram"), this, &OBSBasic::ScreenshotProgram);

	popup.exec(QCursor::pos());
}

void OBSBasic::on_previewDisabledWidget_customContextMenuRequested()
{
	QMenu popup(this);
	delete previewProjectorMain;

	QAction *action = popup.addAction(QTStr("Basic.Main.PreviewConextMenu.Enable"), this, &OBSBasic::TogglePreview);
	action->setCheckable(true);
	action->setChecked(obs_display_enabled(ui->preview->GetDisplay()));

	previewProjectorMain = new QMenu(QTStr("PreviewProjector"));
	AddProjectorMenuMonitors(previewProjectorMain, this, &OBSBasic::OpenPreviewProjector);

	QAction *previewWindow = popup.addAction(QTStr("PreviewWindow"), this, &OBSBasic::OpenPreviewWindow);

	popup.addMenu(previewProjectorMain);
	popup.addAction(previewWindow);
	popup.exec(QCursor::pos());
}

void OBSBasic::on_actionAlwaysOnTop_triggered()
{
#ifndef _WIN32
	/* Make sure all dialogs are safely and successfully closed before
	 * switching the always on top mode due to the fact that windows all
	 * have to be recreated, so queue the actual toggle to happen after
	 * all events related to closing the dialogs have finished */
	CloseDialogs();
#endif

	QMetaObject::invokeMethod(this, "ToggleAlwaysOnTop", Qt::QueuedConnection);
}

void OBSBasic::ToggleAlwaysOnTop()
{
	bool isAlwaysOnTop = IsAlwaysOnTop(this);

	ui->actionAlwaysOnTop->setChecked(!isAlwaysOnTop);
	SetAlwaysOnTop(this, !isAlwaysOnTop);

	show();
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

#ifdef YOUTUBE_ENABLED
YouTubeAppDock *OBSBasic::GetYouTubeAppDock()
{
	return youtubeAppDock;
}

#ifndef SEC_TO_NSEC
#define SEC_TO_NSEC 1000000000
#endif

void OBSBasic::NewYouTubeAppDock()
{
	if (!cef_js_avail)
		return;

	/* make sure that the youtube app dock can't be immediately recreated.
	 * dumb hack. blame chromium. or this particular dock. or both. if CEF
	 * creates/destroys/creates a widget too quickly it can lead to a
	 * crash. */
	uint64_t ts = os_gettime_ns();
	if ((ts - lastYouTubeAppDockCreationTime) < (5ULL * SEC_TO_NSEC))
		return;

	lastYouTubeAppDockCreationTime = ts;

	if (youtubeAppDock)
		RemoveDockWidget(youtubeAppDock->objectName());

	youtubeAppDock = new YouTubeAppDock("YouTube Live Control Panel");
}

void OBSBasic::DeleteYouTubeAppDock()
{
	if (!cef_js_avail)
		return;

	if (youtubeAppDock)
		RemoveDockWidget(youtubeAppDock->objectName());

	youtubeAppDock = nullptr;
}
#endif

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

void OBSBasic::on_actionEditTransform_triggered()
{
	const auto item = GetCurrentSceneItem();
	if (!item)
		return;
	CreateEditTransformWindow(item);
}

void OBSBasic::CreateEditTransformWindow(obs_sceneitem_t *item)
{
	if (transformWindow)
		transformWindow->close();
	transformWindow = new OBSBasicTransform(item, this);
	connect(ui->scenes, &QListWidget::currentItemChanged, transformWindow, &OBSBasicTransform::OnSceneChanged);
	transformWindow->show();
	transformWindow->setAttribute(Qt::WA_DeleteOnClose, true);
}

void OBSBasic::on_actionCopyTransform_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();

	obs_sceneitem_get_info2(item, &copiedTransformInfo);
	obs_sceneitem_get_crop(item, &copiedCropInfo);

	ui->actionPasteTransform->setEnabled(true);
	hasCopiedTransform = true;
}

void undo_redo(const std::string &data)
{
	OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
	OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
	reinterpret_cast<OBSBasic *>(App()->GetMainWindow())->SetCurrentScene(source.Get(), true);

	obs_scene_load_transform_states(data.c_str());
}

void OBSBasic::on_actionPasteTransform_triggered()
{
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	auto func = [](obs_scene_t *, obs_sceneitem_t *item, void *data) {
		if (!obs_sceneitem_selected(item))
			return true;
		if (obs_sceneitem_locked(item))
			return true;

		OBSBasic *main = reinterpret_cast<OBSBasic *>(data);

		obs_sceneitem_defer_update_begin(item);
		obs_sceneitem_set_info2(item, &main->copiedTransformInfo);
		obs_sceneitem_set_crop(item, &main->copiedCropInfo);
		obs_sceneitem_defer_update_end(item);

		return true;
	};

	obs_scene_enum_items(GetCurrentScene(), func, this);

	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(QTStr("Undo.Transform.Paste").arg(obs_source_get_name(GetCurrentSceneSource())), undo_redo,
			  undo_redo, undo_data, redo_data);
}

static bool reset_tr(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *)
{
	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, reset_tr, nullptr);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	obs_sceneitem_defer_update_begin(item);

	obs_transform_info info;
	vec2_set(&info.pos, 0.0f, 0.0f);
	vec2_set(&info.scale, 1.0f, 1.0f);
	info.rot = 0.0f;
	info.alignment = OBS_ALIGN_TOP | OBS_ALIGN_LEFT;
	info.bounds_type = OBS_BOUNDS_NONE;
	info.bounds_alignment = OBS_ALIGN_CENTER;
	info.crop_to_bounds = false;
	vec2_set(&info.bounds, 0.0f, 0.0f);
	obs_sceneitem_set_info2(item, &info);

	obs_sceneitem_crop crop = {};
	obs_sceneitem_set_crop(item, &crop);

	obs_sceneitem_defer_update_end(item);

	return true;
}

void OBSBasic::on_actionResetTransform_triggered()
{
	OBSScene scene = GetCurrentScene();

	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(scene, false);
	obs_scene_enum_items(scene, reset_tr, nullptr);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(scene, false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(QTStr("Undo.Transform.Reset").arg(obs_source_get_name(obs_scene_get_source(scene))),
			  undo_redo, undo_redo, undo_data, redo_data);

	obs_scene_enum_items(GetCurrentScene(), reset_tr, nullptr);
}

static void GetItemBox(obs_sceneitem_t *item, vec3 &tl, vec3 &br)
{
	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	vec3_set(&tl, M_INFINITE, M_INFINITE, 0.0f);
	vec3_set(&br, -M_INFINITE, -M_INFINITE, 0.0f);

	auto GetMinPos = [&](float x, float y) {
		vec3 pos;
		vec3_set(&pos, x, y, 0.0f);
		vec3_transform(&pos, &pos, &boxTransform);
		vec3_min(&tl, &tl, &pos);
		vec3_max(&br, &br, &pos);
	};

	GetMinPos(0.0f, 0.0f);
	GetMinPos(1.0f, 0.0f);
	GetMinPos(0.0f, 1.0f);
	GetMinPos(1.0f, 1.0f);
}

static vec3 GetItemTL(obs_sceneitem_t *item)
{
	vec3 tl, br;
	GetItemBox(item, tl, br);
	return tl;
}

static void SetItemTL(obs_sceneitem_t *item, const vec3 &tl)
{
	vec3 newTL;
	vec2 pos;

	obs_sceneitem_get_pos(item, &pos);
	newTL = GetItemTL(item);
	pos.x += tl.x - newTL.x;
	pos.y += tl.y - newTL.y;
	obs_sceneitem_set_pos(item, &pos);
}

static bool RotateSelectedSources(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, RotateSelectedSources, param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	float rot = *reinterpret_cast<float *>(param);

	vec3 tl = GetItemTL(item);

	rot += obs_sceneitem_get_rot(item);
	if (rot >= 360.0f)
		rot -= 360.0f;
	else if (rot <= -360.0f)
		rot += 360.0f;
	obs_sceneitem_set_rot(item, rot);

	obs_sceneitem_force_update_transform(item);

	SetItemTL(item, tl);

	return true;
};

void OBSBasic::on_actionRotate90CW_triggered()
{
	float f90CW = 90.0f;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), RotateSelectedSources, &f90CW);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.Rotate").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionRotate90CCW_triggered()
{
	float f90CCW = -90.0f;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), RotateSelectedSources, &f90CCW);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.Rotate").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionRotate180_triggered()
{
	float f180 = 180.0f;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), RotateSelectedSources, &f180);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.Rotate").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

static bool MultiplySelectedItemScale(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	vec2 &mul = *reinterpret_cast<vec2 *>(param);

	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, MultiplySelectedItemScale, param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	vec3 tl = GetItemTL(item);

	vec2 scale;
	obs_sceneitem_get_scale(item, &scale);
	vec2_mul(&scale, &scale, &mul);
	obs_sceneitem_set_scale(item, &scale);

	obs_sceneitem_force_update_transform(item);

	SetItemTL(item, tl);

	return true;
}

void OBSBasic::on_actionFlipHorizontal_triggered()
{
	vec2 scale;
	vec2_set(&scale, -1.0f, 1.0f);
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), MultiplySelectedItemScale, &scale);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.HFlip").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionFlipVertical_triggered()
{
	vec2 scale;
	vec2_set(&scale, 1.0f, -1.0f);
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), MultiplySelectedItemScale, &scale);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.VFlip").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

static bool CenterAlignSelectedItems(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	obs_bounds_type boundsType = *reinterpret_cast<obs_bounds_type *>(param);

	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, CenterAlignSelectedItems, param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	obs_transform_info itemInfo;
	vec2_set(&itemInfo.pos, 0.0f, 0.0f);
	vec2_set(&itemInfo.scale, 1.0f, 1.0f);
	itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
	itemInfo.rot = 0.0f;

	vec2_set(&itemInfo.bounds, float(ovi.base_width), float(ovi.base_height));
	itemInfo.bounds_type = boundsType;
	itemInfo.bounds_alignment = OBS_ALIGN_CENTER;
	itemInfo.crop_to_bounds = obs_sceneitem_get_bounds_crop(item);

	obs_sceneitem_set_info2(item, &itemInfo);

	return true;
}

void OBSBasic::on_actionFitToScreen_triggered()
{
	obs_bounds_type boundsType = OBS_BOUNDS_SCALE_INNER;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), CenterAlignSelectedItems, &boundsType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.FitToScreen").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionStretchToScreen_triggered()
{
	obs_bounds_type boundsType = OBS_BOUNDS_STRETCH;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), CenterAlignSelectedItems, &boundsType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(QTStr("Undo.Transform.StretchToScreen")
				  .arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
			  undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::CenterSelectedSceneItems(const CenterType &centerType)
{
	QModelIndexList selectedItems = GetAllSelectedSourceItems();

	if (!selectedItems.count())
		return;

	vector<OBSSceneItem> items;

	// Filter out items that have no size
	for (int x = 0; x < selectedItems.count(); x++) {
		OBSSceneItem item = ui->sources->Get(selectedItems[x].row());
		obs_transform_info oti;
		obs_sceneitem_get_info2(item, &oti);

		obs_source_t *source = obs_sceneitem_get_source(item);
		float width = float(obs_source_get_width(source)) * oti.scale.x;
		float height = float(obs_source_get_height(source)) * oti.scale.y;

		if (width == 0.0f || height == 0.0f)
			continue;

		items.emplace_back(item);
	}

	if (!items.size())
		return;

	// Get center x, y coordinates of items
	vec3 center;

	float top = M_INFINITE;
	float left = M_INFINITE;
	float right = 0.0f;
	float bottom = 0.0f;

	for (auto &item : items) {
		vec3 tl, br;

		GetItemBox(item, tl, br);

		left = std::min(tl.x, left);
		top = std::min(tl.y, top);
		right = std::max(br.x, right);
		bottom = std::max(br.y, bottom);
	}

	center.x = (right + left) / 2.0f;
	center.y = (top + bottom) / 2.0f;
	center.z = 0.0f;

	// Get coordinates of screen center
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	vec3 screenCenter;
	vec3_set(&screenCenter, float(ovi.base_width), float(ovi.base_height), 0.0f);

	vec3_mulf(&screenCenter, &screenCenter, 0.5f);

	// Calculate difference between screen center and item center
	vec3 offset;
	vec3_sub(&offset, &screenCenter, &center);

	// Shift items by offset
	for (auto &item : items) {
		vec3 tl, br;

		GetItemBox(item, tl, br);

		vec3_add(&tl, &tl, &offset);

		vec3 itemTL = GetItemTL(item);

		if (centerType == CenterType::Vertical)
			tl.x = itemTL.x;
		else if (centerType == CenterType::Horizontal)
			tl.y = itemTL.y;

		SetItemTL(item, tl);
	}
}

void OBSBasic::on_actionCenterToScreen_triggered()
{
	CenterType centerType = CenterType::Scene;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	CenterSelectedSceneItems(centerType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.Center").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionVerticalCenter_triggered()
{
	CenterType centerType = CenterType::Vertical;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	CenterSelectedSceneItems(centerType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.VCenter").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionHorizontalCenter_triggered()
{
	CenterType centerType = CenterType::Horizontal;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	CenterSelectedSceneItems(centerType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.HCenter").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::EnablePreviewDisplay(bool enable)
{
	obs_display_set_enabled(ui->preview->GetDisplay(), enable);
	ui->previewContainer->setVisible(enable);
	ui->previewDisabledWidget->setVisible(!enable);
}

void OBSBasic::TogglePreview()
{
	previewEnabled = !previewEnabled;
	EnablePreviewDisplay(previewEnabled);
}

void OBSBasic::EnablePreview()
{
	if (previewProgramMode)
		return;

	previewEnabled = true;
	EnablePreviewDisplay(true);
}

void OBSBasic::DisablePreview()
{
	if (previewProgramMode)
		return;

	previewEnabled = false;
	EnablePreviewDisplay(false);
}

void OBSBasic::EnablePreviewProgram()
{
	SetPreviewProgramMode(true);
}

void OBSBasic::DisablePreviewProgram()
{
	SetPreviewProgramMode(false);
}

static bool nudge_callback(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	if (obs_sceneitem_locked(item))
		return true;

	struct vec2 &offset = *reinterpret_cast<struct vec2 *>(param);
	struct vec2 pos;

	if (!obs_sceneitem_selected(item)) {
		if (obs_sceneitem_is_group(item)) {
			struct vec3 offset3;
			vec3_set(&offset3, offset.x, offset.y, 0.0f);

			struct matrix4 matrix;
			obs_sceneitem_get_draw_transform(item, &matrix);
			vec4_set(&matrix.t, 0.0f, 0.0f, 0.0f, 1.0f);
			matrix4_inv(&matrix, &matrix);
			vec3_transform(&offset3, &offset3, &matrix);

			struct vec2 new_offset;
			vec2_set(&new_offset, offset3.x, offset3.y);
			obs_sceneitem_group_enum_items(item, nudge_callback, &new_offset);
		}

		return true;
	}

	obs_sceneitem_get_pos(item, &pos);
	vec2_add(&pos, &pos, &offset);
	obs_sceneitem_set_pos(item, &pos);
	return true;
}

void OBSBasic::Nudge(int dist, MoveDir dir)
{
	if (ui->preview->Locked())
		return;

	struct vec2 offset;
	vec2_set(&offset, 0.0f, 0.0f);

	switch (dir) {
	case MoveDir::Up:
		offset.y = (float)-dist;
		break;
	case MoveDir::Down:
		offset.y = (float)dist;
		break;
	case MoveDir::Left:
		offset.x = (float)-dist;
		break;
	case MoveDir::Right:
		offset.x = (float)dist;
		break;
	}

	if (!recent_nudge) {
		recent_nudge = true;
		OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), true);
		std::string undo_data(obs_data_get_json(wrapper));

		nudge_timer = new QTimer;
		QObject::connect(nudge_timer, &QTimer::timeout, [this, &recent_nudge = recent_nudge, undo_data]() {
			OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), true);
			std::string redo_data(obs_data_get_json(rwrapper));

			undo_s.add_action(QTStr("Undo.Transform").arg(obs_source_get_name(GetCurrentSceneSource())),
					  undo_redo, undo_redo, undo_data, redo_data);

			recent_nudge = false;
		});
		connect(nudge_timer, &QTimer::timeout, nudge_timer, &QTimer::deleteLater);
		nudge_timer->setSingleShot(true);
	}

	if (nudge_timer) {
		nudge_timer->stop();
		nudge_timer->start(1000);
	} else {
		blog(LOG_ERROR, "No nudge timer!");
	}

	obs_scene_enum_items(GetCurrentScene(), nudge_callback, &offset);
}

void OBSBasic::DeleteProjector(OBSProjector *projector)
{
	for (size_t i = 0; i < projectors.size(); i++) {
		if (projectors[i] == projector) {
			projectors[i]->deleteLater();
			projectors.erase(projectors.begin() + i);
			break;
		}
	}
}

OBSProjector *OBSBasic::OpenProjector(obs_source_t *source, int monitor, ProjectorType type)
{
	/* seriously?  10 monitors? */
	if (monitor > 9 || monitor > QGuiApplication::screens().size() - 1)
		return nullptr;

	bool closeProjectors = config_get_bool(App()->GetUserConfig(), "BasicWindow", "CloseExistingProjectors");

	if (closeProjectors && monitor > -1) {
		for (size_t i = projectors.size(); i > 0; i--) {
			size_t idx = i - 1;
			if (projectors[idx]->GetMonitor() == monitor)
				DeleteProjector(projectors[idx]);
		}
	}

	OBSProjector *projector = new OBSProjector(nullptr, source, monitor, type);

	projectors.emplace_back(projector);

	return projector;
}

void OBSBasic::OpenStudioProgramProjector()
{
	int monitor = sender()->property("monitor").toInt();
	OpenProjector(nullptr, monitor, ProjectorType::StudioProgram);
}

void OBSBasic::OpenPreviewProjector()
{
	int monitor = sender()->property("monitor").toInt();
	OpenProjector(nullptr, monitor, ProjectorType::Preview);
}

void OBSBasic::OpenSourceProjector()
{
	int monitor = sender()->property("monitor").toInt();
	OBSSceneItem item = GetCurrentSceneItem();
	if (!item)
		return;

	OpenProjector(obs_sceneitem_get_source(item), monitor, ProjectorType::Source);
}

void OBSBasic::OpenMultiviewProjector()
{
	int monitor = sender()->property("monitor").toInt();
	OpenProjector(nullptr, monitor, ProjectorType::Multiview);
}

void OBSBasic::OpenSceneProjector()
{
	int monitor = sender()->property("monitor").toInt();
	OBSScene scene = GetCurrentScene();
	if (!scene)
		return;

	OpenProjector(obs_scene_get_source(scene), monitor, ProjectorType::Scene);
}

void OBSBasic::OpenStudioProgramWindow()
{
	OpenProjector(nullptr, -1, ProjectorType::StudioProgram);
}

void OBSBasic::OpenPreviewWindow()
{
	OpenProjector(nullptr, -1, ProjectorType::Preview);
}

void OBSBasic::OpenSourceWindow()
{
	OBSSceneItem item = GetCurrentSceneItem();
	if (!item)
		return;

	OBSSource source = obs_sceneitem_get_source(item);

	OpenProjector(obs_sceneitem_get_source(item), -1, ProjectorType::Source);
}

void OBSBasic::OpenSceneWindow()
{
	OBSScene scene = GetCurrentScene();
	if (!scene)
		return;

	OBSSource source = obs_scene_get_source(scene);

	OpenProjector(obs_scene_get_source(scene), -1, ProjectorType::Scene);
}

void OBSBasic::OpenSavedProjectors()
{
	for (SavedProjectorInfo *info : savedProjectorsArray) {
		OpenSavedProjector(info);
	}
}

void OBSBasic::OpenSavedProjector(SavedProjectorInfo *info)
{
	if (info) {
		OBSProjector *projector = nullptr;
		switch (info->type) {
		case ProjectorType::Source:
		case ProjectorType::Scene: {
			OBSSourceAutoRelease source = obs_get_source_by_name(info->name.c_str());
			if (!source)
				return;

			projector = OpenProjector(source, info->monitor, info->type);
			break;
		}
		default: {
			projector = OpenProjector(nullptr, info->monitor, info->type);
			break;
		}
		}

		if (projector && !info->geometry.empty() && info->monitor < 0) {
			QByteArray byteArray = QByteArray::fromBase64(QByteArray(info->geometry.c_str()));
			projector->restoreGeometry(byteArray);

			if (!WindowPositionValid(projector->normalGeometry())) {
				QRect rect = QGuiApplication::primaryScreen()->geometry();
				projector->setGeometry(
					QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), rect));
			}

			if (info->alwaysOnTopOverridden)
				projector->SetIsAlwaysOnTop(info->alwaysOnTop, true);
		}
	}
}

void OBSBasic::on_actionFullscreenInterface_triggered()
{
	if (!isFullScreen())
		showFullScreen();
	else
		showNormal();
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

int OBSBasic::GetProfilePath(char *path, size_t size, const char *file) const
{
	char profiles_path[512];
	const char *profile = config_get_string(App()->GetUserConfig(), "Basic", "ProfileDir");
	int ret;

	if (!profile)
		return -1;
	if (!path)
		return -1;
	if (!file)
		file = "";

	ret = GetAppConfigPath(profiles_path, 512, "obs-studio/basic/profiles");
	if (ret <= 0)
		return ret;

	if (!*file)
		return snprintf(path, size, "%s/%s", profiles_path, profile);

	return snprintf(path, size, "%s/%s/%s", profiles_path, profile, file);
}

void OBSBasic::on_resetDocks_triggered(bool force)
{
	/* prune deleted extra docks */
	for (int i = oldExtraDocks.size() - 1; i >= 0; i--) {
		if (!oldExtraDocks[i]) {
			oldExtraDocks.removeAt(i);
			oldExtraDockNames.removeAt(i);
		}
	}

#ifdef BROWSER_AVAILABLE
	if ((oldExtraDocks.size() || extraDocks.size() || extraCustomDocks.size() || extraBrowserDocks.size()) &&
	    !force)
#else
	if ((oldExtraDocks.size() || extraDocks.size() || extraCustomDocks.size()) && !force)
#endif
	{
		QMessageBox::StandardButton button =
			OBSMessageBox::question(this, QTStr("ResetUIWarning.Title"), QTStr("ResetUIWarning.Text"));

		if (button == QMessageBox::No)
			return;
	}

	/* undock/hide/center extra docks */
	for (int i = oldExtraDocks.size() - 1; i >= 0; i--) {
		if (oldExtraDocks[i]) {
			oldExtraDocks[i]->setVisible(true);
			oldExtraDocks[i]->setFloating(true);
			oldExtraDocks[i]->move(frameGeometry().topLeft() + rect().center() -
					       oldExtraDocks[i]->rect().center());
			oldExtraDocks[i]->setVisible(false);
		}
	}

#define RESET_DOCKLIST(dockList)                                                                               \
	for (int i = dockList.size() - 1; i >= 0; i--) {                                                       \
		dockList[i]->setVisible(true);                                                                 \
		dockList[i]->setFloating(true);                                                                \
		dockList[i]->move(frameGeometry().topLeft() + rect().center() - dockList[i]->rect().center()); \
		dockList[i]->setVisible(false);                                                                \
	}

	RESET_DOCKLIST(extraDocks)
	RESET_DOCKLIST(extraCustomDocks)
#ifdef BROWSER_AVAILABLE
	RESET_DOCKLIST(extraBrowserDocks)
#endif
#undef RESET_DOCKLIST

	restoreState(startingDockLayout);

	int cx = width();
	int cy = height();

	int cx22_5 = cx * 225 / 1000;
	int cx5 = cx * 5 / 100;
	int cx21 = cx * 21 / 100;

	cy = cy * 225 / 1000;

	int mixerSize = cx - (cx22_5 * 2 + cx5 + cx21);

	QList<QDockWidget *> docks{ui->scenesDock, ui->sourcesDock, ui->mixerDock, ui->transitionsDock, controlsDock};

	QList<int> sizes{cx22_5, cx22_5, mixerSize, cx5, cx21};

	ui->scenesDock->setVisible(true);
	ui->sourcesDock->setVisible(true);
	ui->mixerDock->setVisible(true);
	ui->transitionsDock->setVisible(true);
	controlsDock->setVisible(true);
	statsDock->setVisible(false);
	statsDock->setFloating(true);

	resizeDocks(docks, {cy, cy, cy, cy, cy}, Qt::Vertical);
	resizeDocks(docks, sizes, Qt::Horizontal);

	activateWindow();
}

void OBSBasic::on_lockDocks_toggled(bool lock)
{
	QDockWidget::DockWidgetFeatures features =
		lock ? QDockWidget::NoDockWidgetFeatures
		     : (QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
			QDockWidget::DockWidgetFloatable);

	QDockWidget::DockWidgetFeatures mainFeatures = features;
	mainFeatures &= ~QDockWidget::QDockWidget::DockWidgetClosable;

	ui->scenesDock->setFeatures(mainFeatures);
	ui->sourcesDock->setFeatures(mainFeatures);
	ui->mixerDock->setFeatures(mainFeatures);
	ui->transitionsDock->setFeatures(mainFeatures);
	controlsDock->setFeatures(mainFeatures);
	statsDock->setFeatures(features);

	for (int i = extraDocks.size() - 1; i >= 0; i--)
		extraDocks[i]->setFeatures(features);

	for (int i = extraCustomDocks.size() - 1; i >= 0; i--)
		extraCustomDocks[i]->setFeatures(features);

#ifdef BROWSER_AVAILABLE
	for (int i = extraBrowserDocks.size() - 1; i >= 0; i--)
		extraBrowserDocks[i]->setFeatures(features);
#endif

	for (int i = oldExtraDocks.size() - 1; i >= 0; i--) {
		if (!oldExtraDocks[i]) {
			oldExtraDocks.removeAt(i);
			oldExtraDockNames.removeAt(i);
		} else {
			oldExtraDocks[i]->setFeatures(features);
		}
	}
}

void OBSBasic::on_sideDocks_toggled(bool side)
{
	if (side) {
		setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
		setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
		setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
		setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	} else {
		setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
		setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
		setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
		setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
	}
}

void OBSBasic::on_resetUI_triggered()
{
	on_resetDocks_triggered();

	ui->toggleListboxToolbars->setChecked(true);
	ui->toggleContextBar->setChecked(true);
	ui->toggleSourceIcons->setChecked(true);
	ui->toggleStatusBar->setChecked(true);
	ui->scenes->SetGridMode(false);
	ui->actionSceneListMode->setChecked(true);

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "gridMode", false);
}

void OBSBasic::on_multiviewProjectorWindowed_triggered()
{
	OpenProjector(nullptr, -1, ProjectorType::Multiview);
}

void OBSBasic::on_toggleListboxToolbars_toggled(bool visible)
{
	ui->sourcesToolbar->setVisible(visible);
	ui->scenesToolbar->setVisible(visible);
	ui->mixerToolbar->setVisible(visible);

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "ShowListboxToolbars", visible);
}

void OBSBasic::ShowContextBar()
{
	on_toggleContextBar_toggled(true);
	ui->toggleContextBar->setChecked(true);
}

void OBSBasic::HideContextBar()
{
	on_toggleContextBar_toggled(false);
	ui->toggleContextBar->setChecked(false);
}

void OBSBasic::on_toggleContextBar_toggled(bool visible)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "ShowContextToolbars", visible);
	this->ui->contextContainer->setVisible(visible);
	UpdateContextBar(true);
}

void OBSBasic::on_toggleStatusBar_toggled(bool visible)
{
	ui->statusbar->setVisible(visible);

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "ShowStatusBar", visible);
}

void OBSBasic::on_toggleSourceIcons_toggled(bool visible)
{
	ui->sources->SetIconsVisible(visible);
	if (advAudioWindow != nullptr)
		advAudioWindow->SetIconsVisible(visible);

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "ShowSourceIcons", visible);
}

void OBSBasic::on_actionLockPreview_triggered()
{
	ui->preview->ToggleLocked();
	ui->actionLockPreview->setChecked(ui->preview->Locked());
}

void OBSBasic::on_scalingMenu_aboutToShow()
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	QAction *action = ui->actionScaleCanvas;
	QString text = QTStr("Basic.MainMenu.Edit.Scale.Canvas");
	text = text.arg(QString::number(ovi.base_width), QString::number(ovi.base_height));
	action->setText(text);

	action = ui->actionScaleOutput;
	text = QTStr("Basic.MainMenu.Edit.Scale.Output");
	text = text.arg(QString::number(ovi.output_width), QString::number(ovi.output_height));
	action->setText(text);
	action->setVisible(!(ovi.output_width == ovi.base_width && ovi.output_height == ovi.base_height));

	UpdatePreviewScalingMenu();
}

void OBSBasic::on_actionScaleWindow_triggered()
{
	ui->preview->SetFixedScaling(false);
	ui->preview->ResetScrollingOffset();

	emit ui->preview->DisplayResized();
}

void OBSBasic::on_actionScaleCanvas_triggered()
{
	ui->preview->SetFixedScaling(true);
	ui->preview->SetScalingLevel(0);

	emit ui->preview->DisplayResized();
}

void OBSBasic::on_actionScaleOutput_triggered()
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	ui->preview->SetFixedScaling(true);
	float scalingAmount = float(ovi.output_width) / float(ovi.base_width);
	// log base ZOOM_SENSITIVITY of x = log(x) / log(ZOOM_SENSITIVITY)
	int32_t approxScalingLevel = int32_t(round(log(scalingAmount) / log(ZOOM_SENSITIVITY)));
	ui->preview->SetScalingLevelAndAmount(approxScalingLevel, scalingAmount);
	emit ui->preview->DisplayResized();
}

void OBSBasic::SetShowing(bool showing)
{
	if (!showing && isVisible()) {
		config_set_string(App()->GetUserConfig(), "BasicWindow", "geometry",
				  saveGeometry().toBase64().constData());

		/* hide all visible child dialogs */
		visDlgPositions.clear();
		if (!visDialogs.isEmpty()) {
			for (QDialog *dlg : visDialogs) {
				visDlgPositions.append(dlg->pos());
				dlg->hide();
			}
		}

		if (showHide)
			showHide->setText(QTStr("Basic.SystemTray.Show"));
		QTimer::singleShot(0, this, &OBSBasic::hide);

		if (previewEnabled)
			EnablePreviewDisplay(false);

#ifdef __APPLE__
		EnableOSXDockIcon(false);
#endif

	} else if (showing && !isVisible()) {
		if (showHide)
			showHide->setText(QTStr("Basic.SystemTray.Hide"));
		QTimer::singleShot(0, this, &OBSBasic::show);

		if (previewEnabled)
			EnablePreviewDisplay(true);

#ifdef __APPLE__
		EnableOSXDockIcon(true);
#endif

		/* raise and activate window to ensure it is on top */
		raise();
		activateWindow();

		/* show all child dialogs that was visible earlier */
		if (!visDialogs.isEmpty()) {
			for (int i = 0; i < visDialogs.size(); ++i) {
				QDialog *dlg = visDialogs[i];
				dlg->move(visDlgPositions[i]);
				dlg->show();
			}
		}

		/* Unminimize window if it was hidden to tray instead of task
		 * bar. */
		if (sysTrayMinimizeToTray()) {
			Qt::WindowStates state;
			state = windowState() & ~Qt::WindowMinimized;
			state |= Qt::WindowActive;
			setWindowState(state);
		}
	}
}

void OBSBasic::ToggleShowHide()
{
	bool showing = isVisible();
	if (showing) {
		/* check for modal dialogs */
		EnumDialogs();
		if (!modalDialogs.isEmpty() || !visMsgBoxes.isEmpty())
			return;
	}
	SetShowing(!showing);
}

void OBSBasic::SystemTrayInit()
{
#ifdef __APPLE__
	QIcon trayIconFile = QIcon(":/res/images/obs_macos.svg");
	trayIconFile.setIsMask(true);
#else
	QIcon trayIconFile = QIcon(":/res/images/obs.png");
#endif
	trayIcon.reset(new QSystemTrayIcon(QIcon::fromTheme("obs-tray", trayIconFile), this));
	trayIcon->setToolTip("OBS Studio");

	showHide = new QAction(QTStr("Basic.SystemTray.Show"), trayIcon.data());
	sysTrayStream =
		new QAction(StreamingActive() ? QTStr("Basic.Main.StopStreaming") : QTStr("Basic.Main.StartStreaming"),
			    trayIcon.data());
	sysTrayRecord =
		new QAction(RecordingActive() ? QTStr("Basic.Main.StopRecording") : QTStr("Basic.Main.StartRecording"),
			    trayIcon.data());
	sysTrayReplayBuffer = new QAction(ReplayBufferActive() ? QTStr("Basic.Main.StopReplayBuffer")
							       : QTStr("Basic.Main.StartReplayBuffer"),
					  trayIcon.data());
	sysTrayVirtualCam = new QAction(VirtualCamActive() ? QTStr("Basic.Main.StopVirtualCam")
							   : QTStr("Basic.Main.StartVirtualCam"),
					trayIcon.data());
	exit = new QAction(QTStr("Exit"), trayIcon.data());

	trayMenu = new QMenu;
	previewProjector = new QMenu(QTStr("PreviewProjector"));
	studioProgramProjector = new QMenu(QTStr("StudioProgramProjector"));
	AddProjectorMenuMonitors(previewProjector, this, &OBSBasic::OpenPreviewProjector);
	AddProjectorMenuMonitors(studioProgramProjector, this, &OBSBasic::OpenStudioProgramProjector);
	trayMenu->addAction(showHide);
	trayMenu->addSeparator();
	trayMenu->addMenu(previewProjector);
	trayMenu->addMenu(studioProgramProjector);
	trayMenu->addSeparator();
	trayMenu->addAction(sysTrayStream);
	trayMenu->addAction(sysTrayRecord);
	trayMenu->addAction(sysTrayReplayBuffer);
	trayMenu->addAction(sysTrayVirtualCam);
	trayMenu->addSeparator();
	trayMenu->addAction(exit);
	trayIcon->setContextMenu(trayMenu);
	trayIcon->show();

	if (outputHandler && !outputHandler->replayBuffer)
		sysTrayReplayBuffer->setEnabled(false);

	sysTrayVirtualCam->setEnabled(vcamEnabled);

	if (Active())
		OnActivate(true);

	connect(trayIcon.data(), &QSystemTrayIcon::activated, this, &OBSBasic::IconActivated);
	connect(showHide, &QAction::triggered, this, &OBSBasic::ToggleShowHide);
	connect(sysTrayStream, &QAction::triggered, this, &OBSBasic::StreamActionTriggered);
	connect(sysTrayRecord, &QAction::triggered, this, &OBSBasic::RecordActionTriggered);
	connect(sysTrayReplayBuffer.data(), &QAction::triggered, this, &OBSBasic::ReplayBufferActionTriggered);
	connect(sysTrayVirtualCam.data(), &QAction::triggered, this, &OBSBasic::VirtualCamActionTriggered);
	connect(exit, &QAction::triggered, this, &OBSBasic::close);
}

void OBSBasic::IconActivated(QSystemTrayIcon::ActivationReason reason)
{
	// Refresh projector list
	previewProjector->clear();
	studioProgramProjector->clear();
	AddProjectorMenuMonitors(previewProjector, this, &OBSBasic::OpenPreviewProjector);
	AddProjectorMenuMonitors(studioProgramProjector, this, &OBSBasic::OpenStudioProgramProjector);

#ifdef __APPLE__
	UNUSED_PARAMETER(reason);
#else
	if (reason == QSystemTrayIcon::Trigger) {
		EnablePreviewDisplay(previewEnabled && !isVisible());
		ToggleShowHide();
	}
#endif
}

void OBSBasic::SysTrayNotify(const QString &text, QSystemTrayIcon::MessageIcon n)
{
	if (trayIcon && trayIcon->isVisible() && QSystemTrayIcon::supportsMessages()) {
		QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(n);
		trayIcon->showMessage("OBS Studio", text, icon, 10000);
	}
}

void OBSBasic::SystemTray(bool firstStarted)
{
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;
	if (!trayIcon && !firstStarted)
		return;

	bool sysTrayWhenStarted = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayWhenStarted");
	bool sysTrayEnabled = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayEnabled");

	if (firstStarted)
		SystemTrayInit();

	if (!sysTrayEnabled) {
		trayIcon->hide();
	} else {
		trayIcon->show();
		if (firstStarted && (sysTrayWhenStarted || opt_minimize_tray)) {
			EnablePreviewDisplay(false);
#ifdef __APPLE__
			EnableOSXDockIcon(false);
#endif
			opt_minimize_tray = false;
		}
	}

	if (isVisible())
		showHide->setText(QTStr("Basic.SystemTray.Hide"));
	else
		showHide->setText(QTStr("Basic.SystemTray.Show"));
}

bool OBSBasic::sysTrayMinimizeToTray()
{
	return config_get_bool(App()->GetUserConfig(), "BasicWindow", "SysTrayMinimizeToTray");
}

void OBSBasic::on_actionMainUndo_triggered()
{
	undo_s.undo();
}

void OBSBasic::on_actionMainRedo_triggered()
{
	undo_s.redo();
}

void OBSBasic::on_actionCopySource_triggered()
{
	clipboard.clear();

	for (auto &selectedSource : GetAllSelectedSourceItems()) {
		OBSSceneItem item = ui->sources->Get(selectedSource.row());
		if (!item)
			continue;

		OBSSource source = obs_sceneitem_get_source(item);

		SourceCopyInfo copyInfo;
		copyInfo.weak_source = OBSGetWeakRef(source);
		obs_sceneitem_get_info2(item, &copyInfo.transform);
		obs_sceneitem_get_crop(item, &copyInfo.crop);
		copyInfo.blend_method = obs_sceneitem_get_blending_method(item);
		copyInfo.blend_mode = obs_sceneitem_get_blending_mode(item);
		copyInfo.visible = obs_sceneitem_visible(item);

		clipboard.push_back(copyInfo);
	}

	UpdateEditMenu();
}

void OBSBasic::on_actionPasteRef_triggered()
{
	OBSSource scene_source = GetCurrentSceneSource();
	OBSData undo_data = BackupScene(scene_source);
	OBSScene scene = GetCurrentScene();

	undo_s.push_disabled();

	for (size_t i = clipboard.size(); i > 0; i--) {
		SourceCopyInfo &copyInfo = clipboard[i - 1];

		OBSSource source = OBSGetStrongRef(copyInfo.weak_source);
		if (!source)
			continue;

		const char *name = obs_source_get_name(source);

		/* do not allow duplicate refs of the same group in the same
		 * scene */
		if (!!obs_scene_get_group(scene, name)) {
			continue;
		}

		OBSBasicSourceSelect::SourcePaste(copyInfo, false);
	}

	undo_s.pop_disabled();

	QString action_name = QTStr("Undo.PasteSourceRef");
	const char *scene_name = obs_source_get_name(scene_source);

	OBSData redo_data = BackupScene(scene_source);
	CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}

void OBSBasic::on_actionPasteDup_triggered()
{
	OBSSource scene_source = GetCurrentSceneSource();
	OBSData undo_data = BackupScene(scene_source);

	undo_s.push_disabled();

	for (size_t i = clipboard.size(); i > 0; i--) {
		SourceCopyInfo &copyInfo = clipboard[i - 1];
		OBSBasicSourceSelect::SourcePaste(copyInfo, true);
	}

	undo_s.pop_disabled();

	QString action_name = QTStr("Undo.PasteSource");
	const char *scene_name = obs_source_get_name(scene_source);

	OBSData redo_data = BackupScene(scene_source);
	CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}

void OBSBasic::SourcePasteFilters(OBSSource source, OBSSource dstSource)
{
	if (source == dstSource)
		return;

	OBSDataArrayAutoRelease undo_array = obs_source_backup_filters(dstSource);
	obs_source_copy_filters(dstSource, source);
	OBSDataArrayAutoRelease redo_array = obs_source_backup_filters(dstSource);

	const char *srcName = obs_source_get_name(source);
	const char *dstName = obs_source_get_name(dstSource);
	QString text = QTStr("Undo.Filters.Paste.Multiple").arg(srcName, dstName);

	CreateFilterPasteUndoRedoAction(text, dstSource, undo_array, redo_array);
}

void OBSBasic::AudioMixerCopyFilters()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *source = vol->GetSource();

	copyFiltersSource = obs_source_get_weak_source(source);
	ui->actionPasteFilters->setEnabled(true);
}

void OBSBasic::AudioMixerPasteFilters()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *dstSource = vol->GetSource();

	OBSSourceAutoRelease source = obs_weak_source_get_source(copyFiltersSource);

	SourcePasteFilters(source.Get(), dstSource);
}

void OBSBasic::SceneCopyFilters()
{
	copyFiltersSource = obs_source_get_weak_source(GetCurrentSceneSource());
	ui->actionPasteFilters->setEnabled(true);
}

void OBSBasic::ScenePasteFilters()
{
	OBSSourceAutoRelease source = obs_weak_source_get_source(copyFiltersSource);

	OBSSource dstSource = GetCurrentSceneSource();

	SourcePasteFilters(source.Get(), dstSource);
}

void OBSBasic::on_actionCopyFilters_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();

	if (!item)
		return;

	OBSSource source = obs_sceneitem_get_source(item);

	copyFiltersSource = obs_source_get_weak_source(source);

	ui->actionPasteFilters->setEnabled(true);
}

void OBSBasic::CreateFilterPasteUndoRedoAction(const QString &text, obs_source_t *source, obs_data_array_t *undo_array,
					       obs_data_array_t *redo_array)
{
	auto undo_redo = [this](const std::string &json) {
		OBSDataAutoRelease data = obs_data_create_from_json(json.c_str());
		OBSDataArrayAutoRelease array = obs_data_get_array(data, "array");
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(data, "uuid"));

		obs_source_restore_filters(source, array);

		if (filters)
			filters->UpdateSource(source);
	};

	const char *uuid = obs_source_get_uuid(source);

	OBSDataAutoRelease undo_data = obs_data_create();
	OBSDataAutoRelease redo_data = obs_data_create();
	obs_data_set_array(undo_data, "array", undo_array);
	obs_data_set_array(redo_data, "array", redo_array);
	obs_data_set_string(undo_data, "uuid", uuid);
	obs_data_set_string(redo_data, "uuid", uuid);

	undo_s.add_action(text, undo_redo, undo_redo, obs_data_get_json(undo_data), obs_data_get_json(redo_data));
}

void OBSBasic::on_actionPasteFilters_triggered()
{
	OBSSourceAutoRelease source = obs_weak_source_get_source(copyFiltersSource);

	OBSSceneItem sceneItem = GetCurrentSceneItem();
	OBSSource dstSource = obs_sceneitem_get_source(sceneItem);

	SourcePasteFilters(source.Get(), dstSource);
}

static void ConfirmColor(SourceTree *sources, const QColor &color, QModelIndexList selectedItems)
{
	for (int x = 0; x < selectedItems.count(); x++) {
		SourceTreeItem *treeItem = sources->GetItemWidget(selectedItems[x].row());
		treeItem->setStyleSheet("background: " + color.name(QColor::HexArgb));
		treeItem->style()->unpolish(treeItem);
		treeItem->style()->polish(treeItem);

		OBSSceneItem sceneItem = sources->Get(selectedItems[x].row());
		OBSDataAutoRelease privData = obs_sceneitem_get_private_settings(sceneItem);
		obs_data_set_int(privData, "color-preset", 1);
		obs_data_set_string(privData, "color", QT_TO_UTF8(color.name(QColor::HexArgb)));
	}
}

void OBSBasic::ColorChange()
{
	QModelIndexList selectedItems = ui->sources->selectionModel()->selectedIndexes();
	QAction *action = qobject_cast<QAction *>(sender());
	QPushButton *colorButton = qobject_cast<QPushButton *>(sender());

	if (selectedItems.count() == 0)
		return;

	if (colorButton) {
		int preset = colorButton->property("bgColor").value<int>();

		for (int x = 0; x < selectedItems.count(); x++) {
			SourceTreeItem *treeItem = ui->sources->GetItemWidget(selectedItems[x].row());
			treeItem->setStyleSheet("");
			treeItem->setProperty("bgColor", preset);
			treeItem->style()->unpolish(treeItem);
			treeItem->style()->polish(treeItem);

			OBSSceneItem sceneItem = ui->sources->Get(selectedItems[x].row());
			OBSDataAutoRelease privData = obs_sceneitem_get_private_settings(sceneItem);
			obs_data_set_int(privData, "color-preset", preset + 1);
			obs_data_set_string(privData, "color", "");
		}

		for (int i = 1; i < 9; i++) {
			stringstream button;
			button << "preset" << i;
			QPushButton *cButton =
				colorButton->parentWidget()->findChild<QPushButton *>(button.str().c_str());
			cButton->setStyleSheet("border: 1px solid black");
		}

		colorButton->setStyleSheet("border: 2px solid black");
	} else if (action) {
		int preset = action->property("bgColor").value<int>();

		if (preset == 1) {
			OBSSceneItem curSceneItem = GetCurrentSceneItem();
			SourceTreeItem *curTreeItem = GetItemWidgetFromSceneItem(curSceneItem);
			OBSDataAutoRelease curPrivData = obs_sceneitem_get_private_settings(curSceneItem);

			int oldPreset = obs_data_get_int(curPrivData, "color-preset");
			const QString oldSheet = curTreeItem->styleSheet();

			auto liveChangeColor = [=](const QColor &color) {
				if (color.isValid()) {
					curTreeItem->setStyleSheet("background: " + color.name(QColor::HexArgb));
				}
			};

			auto changedColor = [=](const QColor &color) {
				if (color.isValid()) {
					ConfirmColor(ui->sources, color, selectedItems);
				}
			};

			auto rejected = [=]() {
				if (oldPreset == 1) {
					curTreeItem->setStyleSheet(oldSheet);
					curTreeItem->setProperty("bgColor", 0);
				} else if (oldPreset == 0) {
					curTreeItem->setStyleSheet("background: none");
					curTreeItem->setProperty("bgColor", 0);
				} else {
					curTreeItem->setStyleSheet("");
					curTreeItem->setProperty("bgColor", oldPreset - 1);
				}

				curTreeItem->style()->unpolish(curTreeItem);
				curTreeItem->style()->polish(curTreeItem);
			};

			QColorDialog::ColorDialogOptions options = QColorDialog::ShowAlphaChannel;

			const char *oldColor = obs_data_get_string(curPrivData, "color");
			const char *customColor = *oldColor != 0 ? oldColor : "#55FF0000";
#ifdef __linux__
			// TODO: Revisit hang on Ubuntu with native dialog
			options |= QColorDialog::DontUseNativeDialog;
#endif

			QColorDialog *colorDialog = new QColorDialog(this);
			colorDialog->setOptions(options);
			colorDialog->setCurrentColor(QColor(customColor));
			connect(colorDialog, &QColorDialog::currentColorChanged, liveChangeColor);
			connect(colorDialog, &QColorDialog::colorSelected, changedColor);
			connect(colorDialog, &QColorDialog::rejected, rejected);
			colorDialog->open();
		} else {
			for (int x = 0; x < selectedItems.count(); x++) {
				SourceTreeItem *treeItem = ui->sources->GetItemWidget(selectedItems[x].row());
				treeItem->setStyleSheet("background: none");
				treeItem->setProperty("bgColor", preset);
				treeItem->style()->unpolish(treeItem);
				treeItem->style()->polish(treeItem);

				OBSSceneItem sceneItem = ui->sources->Get(selectedItems[x].row());
				OBSDataAutoRelease privData = obs_sceneitem_get_private_settings(sceneItem);
				obs_data_set_int(privData, "color-preset", preset);
				obs_data_set_string(privData, "color", "");
			}
		}
	}
}

SourceTreeItem *OBSBasic::GetItemWidgetFromSceneItem(obs_sceneitem_t *sceneItem)
{
	int i = 0;
	SourceTreeItem *treeItem = ui->sources->GetItemWidget(i);
	OBSSceneItem item = ui->sources->Get(i);
	int64_t id = obs_sceneitem_get_id(sceneItem);
	while (treeItem && obs_sceneitem_get_id(item) != id) {
		i++;
		treeItem = ui->sources->GetItemWidget(i);
		item = ui->sources->Get(i);
	}
	if (treeItem)
		return treeItem;

	return nullptr;
}

void OBSBasic::on_autoConfigure_triggered()
{
	AutoConfig test(this);
	test.setModal(true);
	test.show();
	test.exec();
}

void OBSBasic::on_stats_triggered()
{
	if (!stats.isNull()) {
		stats->show();
		stats->raise();
		return;
	}

	OBSBasicStats *statsDlg;
	statsDlg = new OBSBasicStats(nullptr);
	statsDlg->show();
	stats = statsDlg;
}

void OBSBasic::on_actionShowAbout_triggered()
{
	if (about)
		about->close();

	about = new OBSAbout(this);
	about->show();

	about->setAttribute(Qt::WA_DeleteOnClose, true);
}

void OBSBasic::ResizeOutputSizeOfSource()
{
	if (obs_video_active())
		return;

	QMessageBox resize_output(this);
	resize_output.setText(QTStr("ResizeOutputSizeOfSource.Text") + "\n\n" +
			      QTStr("ResizeOutputSizeOfSource.Continue"));
	QAbstractButton *Yes = resize_output.addButton(QTStr("Yes"), QMessageBox::YesRole);
	resize_output.addButton(QTStr("No"), QMessageBox::NoRole);
	resize_output.setIcon(QMessageBox::Warning);
	resize_output.setWindowTitle(QTStr("ResizeOutputSizeOfSource"));
	resize_output.exec();

	if (resize_output.clickedButton() != Yes)
		return;

	OBSSource source = obs_sceneitem_get_source(GetCurrentSceneItem());

	int width = obs_source_get_width(source);
	int height = obs_source_get_height(source);

	config_set_uint(activeConfiguration, "Video", "BaseCX", width);
	config_set_uint(activeConfiguration, "Video", "BaseCY", height);
	config_set_uint(activeConfiguration, "Video", "OutputCX", width);
	config_set_uint(activeConfiguration, "Video", "OutputCY", height);

	ResetVideo();
	ResetOutputs();
	activeConfiguration.SaveSafe("tmp");
	on_actionFitToScreen_triggered();
}

QAction *OBSBasic::AddDockWidget(QDockWidget *dock)
{
	// Prevent the object name from being changed
	connect(dock, &QObject::objectNameChanged, this, &OBSBasic::RepairOldExtraDockName);

#ifdef BROWSER_AVAILABLE
	QAction *action = new QAction(dock->windowTitle(), ui->menuDocks);

	if (!extraBrowserMenuDocksSeparator.isNull())
		ui->menuDocks->insertAction(extraBrowserMenuDocksSeparator, action);
	else
		ui->menuDocks->addAction(action);
#else
	QAction *action = ui->menuDocks->addAction(dock->windowTitle());
#endif
	action->setCheckable(true);
	assignDockToggle(dock, action);
	oldExtraDocks.push_back(dock);
	oldExtraDockNames.push_back(dock->objectName());

	bool lock = ui->lockDocks->isChecked();
	QDockWidget::DockWidgetFeatures features =
		lock ? QDockWidget::NoDockWidgetFeatures
		     : (QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
			QDockWidget::DockWidgetFloatable);

	dock->setFeatures(features);

	/* prune deleted docks */
	for (int i = oldExtraDocks.size() - 1; i >= 0; i--) {
		if (!oldExtraDocks[i]) {
			oldExtraDocks.removeAt(i);
			oldExtraDockNames.removeAt(i);
		}
	}

	return action;
}

void OBSBasic::RepairOldExtraDockName()
{
	QDockWidget *dock = reinterpret_cast<QDockWidget *>(sender());
	int idx = oldExtraDocks.indexOf(dock);
	QSignalBlocker block(dock);

	if (idx == -1) {
		blog(LOG_WARNING, "A dock got its object name changed");
		return;
	}

	blog(LOG_WARNING, "The dock '%s' got its object name restored", QT_TO_UTF8(oldExtraDockNames[idx]));

	dock->setObjectName(oldExtraDockNames[idx]);
}

void OBSBasic::AddDockWidget(QDockWidget *dock, Qt::DockWidgetArea area, bool extraBrowser)
{
	if (dock->objectName().isEmpty())
		return;

	bool lock = ui->lockDocks->isChecked();
	QDockWidget::DockWidgetFeatures features =
		lock ? QDockWidget::NoDockWidgetFeatures
		     : (QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
			QDockWidget::DockWidgetFloatable);

	setupDockAction(dock);
	dock->setFeatures(features);
	addDockWidget(area, dock);

#ifdef BROWSER_AVAILABLE
	if (extraBrowser && extraBrowserMenuDocksSeparator.isNull())
		extraBrowserMenuDocksSeparator = ui->menuDocks->addSeparator();

	if (!extraBrowser && !extraBrowserMenuDocksSeparator.isNull())
		ui->menuDocks->insertAction(extraBrowserMenuDocksSeparator, dock->toggleViewAction());
	else
		ui->menuDocks->addAction(dock->toggleViewAction());

	if (extraBrowser)
		return;
#else
	UNUSED_PARAMETER(extraBrowser);

	ui->menuDocks->addAction(dock->toggleViewAction());
#endif

	extraDockNames.push_back(dock->objectName());
	extraDocks.push_back(std::shared_ptr<QDockWidget>(dock));
}

void OBSBasic::RemoveDockWidget(const QString &name)
{
	if (extraDockNames.contains(name)) {
		int idx = extraDockNames.indexOf(name);
		extraDockNames.removeAt(idx);
		extraDocks[idx].reset();
		extraDocks.removeAt(idx);
	} else if (extraCustomDockNames.contains(name)) {
		int idx = extraCustomDockNames.indexOf(name);
		extraCustomDockNames.removeAt(idx);
		removeDockWidget(extraCustomDocks[idx]);
		extraCustomDocks.removeAt(idx);
	}
}

bool OBSBasic::IsDockObjectNameUsed(const QString &name)
{
	QStringList list;
	list << "scenesDock"
	     << "sourcesDock"
	     << "mixerDock"
	     << "transitionsDock"
	     << "controlsDock"
	     << "statsDock";
	list << oldExtraDockNames;
	list << extraDockNames;
	list << extraCustomDockNames;

	return list.contains(name);
}

void OBSBasic::AddCustomDockWidget(QDockWidget *dock)
{
	// Prevent the object name from being changed
	connect(dock, &QObject::objectNameChanged, this, &OBSBasic::RepairCustomExtraDockName);

	bool lock = ui->lockDocks->isChecked();
	QDockWidget::DockWidgetFeatures features =
		lock ? QDockWidget::NoDockWidgetFeatures
		     : (QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
			QDockWidget::DockWidgetFloatable);

	dock->setFeatures(features);
	addDockWidget(Qt::RightDockWidgetArea, dock);

	extraCustomDockNames.push_back(dock->objectName());
	extraCustomDocks.push_back(dock);
}

void OBSBasic::RepairCustomExtraDockName()
{
	QDockWidget *dock = reinterpret_cast<QDockWidget *>(sender());
	int idx = extraCustomDocks.indexOf(dock);
	QSignalBlocker block(dock);

	if (idx == -1) {
		blog(LOG_WARNING, "A custom dock got its object name changed");
		return;
	}

	blog(LOG_WARNING, "The custom dock '%s' got its object name restored", QT_TO_UTF8(extraCustomDockNames[idx]));

	dock->setObjectName(extraCustomDockNames[idx]);
}

OBSBasic *OBSBasic::Get()
{
	return reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
}

bool OBSBasic::StreamingActive()
{
	if (!outputHandler)
		return false;
	return outputHandler->StreamingActive();
}

bool OBSBasic::RecordingActive()
{
	if (!outputHandler)
		return false;
	return outputHandler->RecordingActive();
}

bool OBSBasic::ReplayBufferActive()
{
	if (!outputHandler)
		return false;
	return outputHandler->ReplayBufferActive();
}

bool OBSBasic::VirtualCamActive()
{
	if (!outputHandler)
		return false;
	return outputHandler->VirtualCamActive();
}

SceneRenameDelegate::SceneRenameDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void SceneRenameDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QStyledItemDelegate::setEditorData(editor, index);
	QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
	if (lineEdit)
		lineEdit->selectAll();
}

bool SceneRenameDelegate::eventFilter(QObject *editor, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		switch (keyEvent->key()) {
		case Qt::Key_Escape: {
			QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
			if (lineEdit)
				lineEdit->undo();
			break;
		}
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			return false;
		}
	}

	return QStyledItemDelegate::eventFilter(editor, event);
}

void OBSBasic::UpdatePatronJson(const QString &text, const QString &error)
{
	if (!error.isEmpty())
		return;

	patronJson = QT_TO_UTF8(text);
}

void OBSBasic::PauseRecording()
{
	if (!isRecordingPausable || !outputHandler || !outputHandler->fileOutput ||
	    os_atomic_load_bool(&recording_paused))
		return;

	obs_output_t *output = outputHandler->fileOutput;

	if (obs_output_pause(output, true)) {
		os_atomic_set_bool(&recording_paused, true);

		emit RecordingPaused();

		ui->statusbar->RecordingPaused();

		TaskbarOverlaySetStatus(TaskbarOverlayStatusPaused);
		if (trayIcon && trayIcon->isVisible()) {
#ifdef __APPLE__
			QIcon trayIconFile = QIcon(":/res/images/obs_paused_macos.svg");
			trayIconFile.setIsMask(true);
#else
			QIcon trayIconFile = QIcon(":/res/images/obs_paused.png");
#endif
			trayIcon->setIcon(QIcon::fromTheme("obs-tray-paused", trayIconFile));
		}

		OnEvent(OBS_FRONTEND_EVENT_RECORDING_PAUSED);

		if (os_atomic_load_bool(&replaybuf_active))
			ShowReplayBufferPauseWarning();
	}
}

void OBSBasic::UnpauseRecording()
{
	if (!isRecordingPausable || !outputHandler || !outputHandler->fileOutput ||
	    !os_atomic_load_bool(&recording_paused))
		return;

	obs_output_t *output = outputHandler->fileOutput;

	if (obs_output_pause(output, false)) {
		os_atomic_set_bool(&recording_paused, false);

		emit RecordingUnpaused();

		ui->statusbar->RecordingUnpaused();

		TaskbarOverlaySetStatus(TaskbarOverlayStatusActive);
		if (trayIcon && trayIcon->isVisible()) {
#ifdef __APPLE__
			QIcon trayIconFile = QIcon(":/res/images/tray_active_macos.svg");
			trayIconFile.setIsMask(true);
#else
			QIcon trayIconFile = QIcon(":/res/images/tray_active.png");
#endif
			trayIcon->setIcon(QIcon::fromTheme("obs-tray-active", trayIconFile));
		}

		OnEvent(OBS_FRONTEND_EVENT_RECORDING_UNPAUSED);
	}
}

void OBSBasic::RecordPauseToggled()
{
	if (!isRecordingPausable || !outputHandler || !outputHandler->fileOutput)
		return;

	obs_output_t *output = outputHandler->fileOutput;
	bool enable = !obs_output_paused(output);

	if (enable)
		PauseRecording();
	else
		UnpauseRecording();
}

void OBSBasic::UpdateIsRecordingPausable()
{
	const char *mode = config_get_string(activeConfiguration, "Output", "Mode");
	bool adv = astrcmpi(mode, "Advanced") == 0;
	bool shared = true;

	if (adv) {
		const char *recType = config_get_string(activeConfiguration, "AdvOut", "RecType");

		if (astrcmpi(recType, "FFmpeg") == 0) {
			shared = config_get_bool(activeConfiguration, "AdvOut", "FFOutputToFile");
		} else {
			const char *recordEncoder = config_get_string(activeConfiguration, "AdvOut", "RecEncoder");
			shared = astrcmpi(recordEncoder, "none") == 0;
		}
	} else {
		const char *quality = config_get_string(activeConfiguration, "SimpleOutput", "RecQuality");
		shared = strcmp(quality, "Stream") == 0;
	}

	isRecordingPausable = !shared;
}

#define MBYTE (1024ULL * 1024ULL)
#define MBYTES_LEFT_STOP_REC 50ULL
#define MAX_BYTES_LEFT (MBYTES_LEFT_STOP_REC * MBYTE)

const char *OBSBasic::GetCurrentOutputPath()
{
	const char *path = nullptr;
	const char *mode = config_get_string(Config(), "Output", "Mode");

	if (strcmp(mode, "Advanced") == 0) {
		const char *advanced_mode = config_get_string(Config(), "AdvOut", "RecType");

		if (strcmp(advanced_mode, "FFmpeg") == 0) {
			path = config_get_string(Config(), "AdvOut", "FFFilePath");
		} else {
			path = config_get_string(Config(), "AdvOut", "RecFilePath");
		}
	} else {
		path = config_get_string(Config(), "SimpleOutput", "FilePath");
	}

	return path;
}

void OBSBasic::OutputPathInvalidMessage()
{
	blog(LOG_ERROR, "Recording stopped because of bad output path");

	OBSMessageBox::critical(this, QTStr("Output.BadPath.Title"), QTStr("Output.BadPath.Text"));
}

bool OBSBasic::IsFFmpegOutputToURL() const
{
	const char *mode = config_get_string(Config(), "Output", "Mode");
	if (strcmp(mode, "Advanced") == 0) {
		const char *advanced_mode = config_get_string(Config(), "AdvOut", "RecType");
		if (strcmp(advanced_mode, "FFmpeg") == 0) {
			bool is_local = config_get_bool(Config(), "AdvOut", "FFOutputToFile");
			if (!is_local)
				return true;
		}
	}

	return false;
}

bool OBSBasic::OutputPathValid()
{
	if (IsFFmpegOutputToURL())
		return true;

	const char *path = GetCurrentOutputPath();
	return path && *path && QDir(path).exists();
}

void OBSBasic::DiskSpaceMessage()
{
	blog(LOG_ERROR, "Recording stopped because of low disk space");

	OBSMessageBox::critical(this, QTStr("Output.RecordNoSpace.Title"), QTStr("Output.RecordNoSpace.Msg"));
}

bool OBSBasic::LowDiskSpace()
{
	const char *path;

	path = GetCurrentOutputPath();
	if (!path)
		return false;

	uint64_t num_bytes = os_get_free_disk_space(path);

	if (num_bytes < (MAX_BYTES_LEFT))
		return true;
	else
		return false;
}

void OBSBasic::CheckDiskSpaceRemaining()
{
	if (LowDiskSpace()) {
		StopRecording();
		StopReplayBuffer();

		DiskSpaceMessage();
	}
}

void OBSBasic::ResetStatsHotkey()
{
	const QList<OBSBasicStats *> list = findChildren<OBSBasicStats *>();

	for (OBSBasicStats *s : list) {
		s->Reset();
	}
}

void OBSBasic::on_OBSBasic_customContextMenuRequested(const QPoint &pos)
{
	QWidget *widget = childAt(pos);
	const char *className = nullptr;
	QString objName;
	if (widget != nullptr) {
		className = widget->metaObject()->className();
		objName = widget->objectName();
	}

	QPoint globalPos = mapToGlobal(pos);
	if (className && strstr(className, "Dock") != nullptr && !objName.isEmpty()) {
		if (objName.compare("scenesDock") == 0) {
			ui->scenes->customContextMenuRequested(globalPos);
		} else if (objName.compare("sourcesDock") == 0) {
			ui->sources->customContextMenuRequested(globalPos);
		} else if (objName.compare("mixerDock") == 0) {
			StackedMixerAreaContextMenuRequested();
		}
	} else if (!className) {
		ui->menuDocks->exec(globalPos);
	}
}

void OBSBasic::UpdateProjectorHideCursor()
{
	for (const auto &proj : projectors)
		proj->SetHideCursor();
}

void OBSBasic::UpdateProjectorAlwaysOnTop(bool top)
{
	for (const auto &proj : projectors)
		SetAlwaysOnTop(proj, top);
}

void OBSBasic::ResetProjectors()
{
	OBSDataArrayAutoRelease savedProjectorList = SaveProjectors();
	ClearProjectors();
	LoadSavedProjectors(savedProjectorList);
	OpenSavedProjectors();
}

void OBSBasic::on_sourcePropertiesButton_clicked()
{
	on_actionSourceProperties_triggered();
}

void OBSBasic::on_sourceFiltersButton_clicked()
{
	OpenFilters();
}

void OBSBasic::on_actionSceneFilters_triggered()
{
	OBSSource sceneSource = GetCurrentSceneSource();

	if (sceneSource)
		OpenFilters(sceneSource);
}

void OBSBasic::on_sourceInteractButton_clicked()
{
	on_actionInteract_triggered();
}

void OBSBasic::ShowStatusBarMessage(const QString &message)
{
	ui->statusbar->clearMessage();
	ui->statusbar->showMessage(message, 10000);
}

void OBSBasic::UpdatePreviewSafeAreas()
{
	drawSafeAreas = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ShowSafeAreas");
}

void OBSBasic::UpdatePreviewOverflowSettings()
{
	bool hidden = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowHidden");
	bool select = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowSelectionHidden");
	bool always = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowAlwaysVisible");

	ui->preview->SetOverflowHidden(hidden);
	ui->preview->SetOverflowSelectionHidden(select);
	ui->preview->SetOverflowAlwaysVisible(always);
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

static inline QColor color_from_int(long long val)
{
	return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) & 0xff);
}

QColor OBSBasic::GetSelectionColor() const
{
	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		return color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "SelectRed"));
	} else {
		return QColor::fromRgb(255, 0, 0);
	}
}

QColor OBSBasic::GetCropColor() const
{
	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		return color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "SelectGreen"));
	} else {
		return QColor::fromRgb(0, 255, 0);
	}
}

QColor OBSBasic::GetHoverColor() const
{
	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		return color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "SelectBlue"));
	} else {
		return QColor::fromRgb(0, 127, 255);
	}
}

void OBSBasic::UpdatePreviewSpacingHelpers()
{
	drawSpacingHelpers = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SpacingHelpersEnabled");
}

float OBSBasic::GetDevicePixelRatio()
{
	return dpi;
}

void OBSBasic::OnEvent(enum obs_frontend_event event)
{
	if (api)
		api->on_event(event);
}

void OBSBasic::UpdatePreviewScrollbars()
{
	if (!ui->preview->IsFixedScaling()) {
		ui->previewXScrollBar->setRange(0, 0);
		ui->previewYScrollBar->setRange(0, 0);
	}
}

void OBSBasic::on_previewXScrollBar_valueChanged(int value)
{
	emit PreviewXScrollBarMoved(value);
}

void OBSBasic::on_previewYScrollBar_valueChanged(int value)
{
	emit PreviewYScrollBarMoved(value);
}

void OBSBasic::PreviewScalingModeChanged(int value)
{
	switch (value) {
	case 0:
		on_actionScaleWindow_triggered();
		break;
	case 1:
		on_actionScaleCanvas_triggered();
		break;
	case 2:
		on_actionScaleOutput_triggered();
		break;
	};
}

// MARK: - Generic UI Helper Functions

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
