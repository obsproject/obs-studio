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

#include "OBSApp.hpp"

#include <components/Multiview.hpp>
#include <utility/OBSEventFilter.hpp>
#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
#include <utility/models/branches.hpp>
#endif
#include <widgets/OBSBasic.hpp>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <obs-nix-platform.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
#include <qpa/qplatformnativeinterface.h>
#endif
#endif
#include <qt-wrappers.hpp>

#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
#include <QFile>
#endif

#ifdef _WIN32
#include <QSessionManager>
#else
#include <QSocketNotifier>
#endif

#ifdef _WIN32
#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif

#include "moc_OBSApp.cpp"

using namespace std;

string currentLogFile;
string lastLogFile;
string lastCrashLogFile;

extern bool portable_mode;
extern bool safe_mode;
extern bool disable_3p_plugins;
extern bool opt_disable_updater;
extern bool opt_disable_missing_files_check;
extern string opt_starting_collection;
extern string opt_starting_profile;

extern QPointer<OBSLogViewer> obsLogViewer;

#ifndef _WIN32
int OBSApp::sigintFd[2];
#endif

// GPU hint exports for AMD/NVIDIA laptops
#ifdef _MSC_VER
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

QObject *CreateShortcutFilter()
{
	return new OBSEventFilter([](QObject *obj, QEvent *event) {
		auto mouse_event = [](QMouseEvent &event) {
			if (!App()->HotkeysEnabledInFocus() && event.button() != Qt::LeftButton)
				return true;

			obs_key_combination_t hotkey = {0, OBS_KEY_NONE};
			bool pressed = event.type() == QEvent::MouseButtonPress;

			switch (event.button()) {
			case Qt::NoButton:
			case Qt::LeftButton:
			case Qt::RightButton:
			case Qt::AllButtons:
			case Qt::MouseButtonMask:
				return false;

			case Qt::MiddleButton:
				hotkey.key = OBS_KEY_MOUSE3;
				break;

#define MAP_BUTTON(i, j)                       \
	case Qt::ExtraButton##i:               \
		hotkey.key = OBS_KEY_MOUSE##j; \
		break;
				MAP_BUTTON(1, 4);
				MAP_BUTTON(2, 5);
				MAP_BUTTON(3, 6);
				MAP_BUTTON(4, 7);
				MAP_BUTTON(5, 8);
				MAP_BUTTON(6, 9);
				MAP_BUTTON(7, 10);
				MAP_BUTTON(8, 11);
				MAP_BUTTON(9, 12);
				MAP_BUTTON(10, 13);
				MAP_BUTTON(11, 14);
				MAP_BUTTON(12, 15);
				MAP_BUTTON(13, 16);
				MAP_BUTTON(14, 17);
				MAP_BUTTON(15, 18);
				MAP_BUTTON(16, 19);
				MAP_BUTTON(17, 20);
				MAP_BUTTON(18, 21);
				MAP_BUTTON(19, 22);
				MAP_BUTTON(20, 23);
				MAP_BUTTON(21, 24);
				MAP_BUTTON(22, 25);
				MAP_BUTTON(23, 26);
				MAP_BUTTON(24, 27);
#undef MAP_BUTTON
			}

			hotkey.modifiers = TranslateQtKeyboardEventModifiers(event.modifiers());

			obs_hotkey_inject_event(hotkey, pressed);
			return true;
		};

		auto key_event = [&](QKeyEvent *event) {
			int key = event->key();
			bool enabledInFocus = App()->HotkeysEnabledInFocus();

			if (key != Qt::Key_Enter && key != Qt::Key_Escape && key != Qt::Key_Return && !enabledInFocus)
				return true;

			QDialog *dialog = qobject_cast<QDialog *>(obj);

			obs_key_combination_t hotkey = {0, OBS_KEY_NONE};
			bool pressed = event->type() == QEvent::KeyPress;

			switch (key) {
			case Qt::Key_Shift:
			case Qt::Key_Control:
			case Qt::Key_Alt:
			case Qt::Key_Meta:
				break;

#ifdef __APPLE__
			case Qt::Key_CapsLock:
				// kVK_CapsLock == 57
				hotkey.key = obs_key_from_virtual_key(57);
				pressed = true;
				break;
#endif

			case Qt::Key_Enter:
			case Qt::Key_Escape:
			case Qt::Key_Return:
				if (dialog && pressed)
					return false;
				if (!enabledInFocus)
					return true;
				/* Falls through. */
			default:
				hotkey.key = obs_key_from_virtual_key(event->nativeVirtualKey());
			}

			if (event->isAutoRepeat())
				return true;

			hotkey.modifiers = TranslateQtKeyboardEventModifiers(event->modifiers());

			obs_hotkey_inject_event(hotkey, pressed);
			return true;
		};

		switch (event->type()) {
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
			return mouse_event(*static_cast<QMouseEvent *>(event));

		/*case QEvent::MouseButtonDblClick:
		case QEvent::Wheel:*/
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
			return key_event(static_cast<QKeyEvent *>(event));

		default:
			return false;
		}
	});
}

string CurrentDateTimeString()
{
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d, %X", &tstruct);
	return buf;
}

#define DEFAULT_LANG "en-US"

bool OBSApp::InitGlobalConfigDefaults()
{
	config_set_default_uint(appConfig, "General", "MaxLogs", 10);
	config_set_default_int(appConfig, "General", "InfoIncrement", -1);
	config_set_default_string(appConfig, "General", "ProcessPriority", "Normal");
	config_set_default_bool(appConfig, "General", "EnableAutoUpdates", true);
	config_set_default_bool(appConfig, "General", "DualOutputActive", false);

#if _WIN32
	config_set_default_string(appConfig, "Video", "Renderer", "Direct3D 11");
#else
	config_set_default_string(appConfig, "Video", "Renderer", "OpenGL");
#endif

#ifdef _WIN32
	config_set_default_bool(appConfig, "Audio", "DisableAudioDucking", true);
#endif

#if defined(_WIN32) || defined(__APPLE__) || defined(__linux__)
	config_set_default_bool(appConfig, "General", "BrowserHWAccel", true);
#endif

#ifdef __APPLE__
	config_set_default_bool(appConfig, "Video", "DisableOSXVSync", true);
	config_set_default_bool(appConfig, "Video", "ResetOSXVSyncOnExit", true);
#endif

	return true;
}

bool OBSApp::InitGlobalLocationDefaults()
{
	char path[512];

	int len = GetAppConfigPath(path, sizeof(path), nullptr);
	if (len <= 0) {
		OBSErrorBox(NULL, "Unable to get global configuration path.");
		return false;
	}

	config_set_default_string(appConfig, "Locations", "Configuration", path);
	config_set_default_string(appConfig, "Locations", "SceneCollections", path);
	config_set_default_string(appConfig, "Locations", "Profiles", path);

	return true;
}

void OBSApp::InitUserConfigDefaults()
{
	config_set_default_bool(userConfig, "General", "ConfirmOnExit", true);

	config_set_default_string(userConfig, "General", "HotkeyFocusType", "NeverDisableHotkeys");

	config_set_default_bool(userConfig, "BasicWindow", "PreviewEnabled", true);
	config_set_default_bool(userConfig, "BasicWindow", "PreviewProgramMode", false);
	config_set_default_bool(userConfig, "BasicWindow", "SceneDuplicationMode", true);
	config_set_default_bool(userConfig, "BasicWindow", "SwapScenesMode", true);
	config_set_default_bool(userConfig, "BasicWindow", "SnappingEnabled", true);
	config_set_default_bool(userConfig, "BasicWindow", "ScreenSnapping", true);
	config_set_default_bool(userConfig, "BasicWindow", "SourceSnapping", true);
	config_set_default_bool(userConfig, "BasicWindow", "CenterSnapping", false);
	config_set_default_double(userConfig, "BasicWindow", "SnapDistance", 10.0);
	config_set_default_bool(userConfig, "BasicWindow", "SpacingHelpersEnabled", true);
	config_set_default_bool(userConfig, "BasicWindow", "RecordWhenStreaming", false);
	config_set_default_bool(userConfig, "BasicWindow", "KeepRecordingWhenStreamStops", false);
	config_set_default_bool(userConfig, "BasicWindow", "SysTrayEnabled", true);
	config_set_default_bool(userConfig, "BasicWindow", "SysTrayWhenStarted", false);
	config_set_default_bool(userConfig, "BasicWindow", "SaveProjectors", false);
	config_set_default_bool(userConfig, "BasicWindow", "ShowTransitions", true);
	config_set_default_bool(userConfig, "BasicWindow", "ShowListboxToolbars", true);
	config_set_default_bool(userConfig, "BasicWindow", "ShowStatusBar", true);
	config_set_default_bool(userConfig, "BasicWindow", "ShowSourceIcons", true);
	config_set_default_bool(userConfig, "BasicWindow", "ShowContextToolbars", true);
	config_set_default_bool(userConfig, "BasicWindow", "StudioModeLabels", true);

	config_set_default_bool(userConfig, "BasicWindow", "VerticalVolControl", false);

	config_set_default_bool(userConfig, "BasicWindow", "MultiviewMouseSwitch", true);

	config_set_default_bool(userConfig, "BasicWindow", "MultiviewDrawNames", true);

	config_set_default_bool(userConfig, "BasicWindow", "MultiviewDrawAreas", true);

	config_set_default_bool(userConfig, "BasicWindow", "MediaControlsCountdownTimer", true);

	config_set_default_int(userConfig, "Appearance", "FontScale", 10);
	config_set_default_int(userConfig, "Appearance", "Density", 1);
}

static bool do_mkdir(const char *path)
{
	if (os_mkdirs(path) == MKDIR_ERROR) {
		OBSErrorBox(NULL, "Failed to create directory %s", path);
		return false;
	}

	return true;
}

static bool MakeUserDirs()
{
	char path[512];

	if (GetAppConfigPath(path, sizeof(path), "obs-studio/basic") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	if (GetAppConfigPath(path, sizeof(path), "obs-studio/logs") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	if (GetAppConfigPath(path, sizeof(path), "obs-studio/profiler_data") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

#ifdef _WIN32
	if (GetAppConfigPath(path, sizeof(path), "obs-studio/crashes") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;
#endif

#ifdef WHATSNEW_ENABLED
	if (GetAppConfigPath(path, sizeof(path), "obs-studio/updates") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;
#endif

	if (GetAppConfigPath(path, sizeof(path), "obs-studio/plugin_config") <= 0)
		return false;
	if (!do_mkdir(path))
		return false;

	return true;
}

constexpr std::string_view OBSProfileSubDirectory = "obs-studio/basic/profiles";
constexpr std::string_view OBSScenesSubDirectory = "obs-studio/basic/scenes";

static bool MakeUserProfileDirs()
{
	const std::filesystem::path userProfilePath =
		App()->userProfilesLocation / std::filesystem::u8path(OBSProfileSubDirectory);
	const std::filesystem::path userScenesPath =
		App()->userScenesLocation / std::filesystem::u8path(OBSScenesSubDirectory);

	if (!std::filesystem::exists(userProfilePath)) {
		try {
			std::filesystem::create_directories(userProfilePath);
		} catch (const std::filesystem::filesystem_error &error) {
			blog(LOG_ERROR, "Failed to create user profile directory '%s'\n%s",
			     userProfilePath.u8string().c_str(), error.what());
			return false;
		}
	}

	if (!std::filesystem::exists(userScenesPath)) {
		try {
			std::filesystem::create_directories(userScenesPath);
		} catch (const std::filesystem::filesystem_error &error) {
			blog(LOG_ERROR, "Failed to create user scene collection directory '%s'\n%s",
			     userScenesPath.u8string().c_str(), error.what());
			return false;
		}
	}

	return true;
}

bool OBSApp::UpdatePre22MultiviewLayout(const char *layout)
{
	if (!layout)
		return false;

	if (astrcmpi(layout, "horizontaltop") == 0) {
		config_set_int(userConfig, "BasicWindow", "MultiviewLayout",
			       static_cast<int>(MultiviewLayout::HORIZONTAL_TOP_8_SCENES));
		return true;
	}

	if (astrcmpi(layout, "horizontalbottom") == 0) {
		config_set_int(userConfig, "BasicWindow", "MultiviewLayout",
			       static_cast<int>(MultiviewLayout::HORIZONTAL_BOTTOM_8_SCENES));
		return true;
	}

	if (astrcmpi(layout, "verticalleft") == 0) {
		config_set_int(userConfig, "BasicWindow", "MultiviewLayout",
			       static_cast<int>(MultiviewLayout::VERTICAL_LEFT_8_SCENES));
		return true;
	}

	if (astrcmpi(layout, "verticalright") == 0) {
		config_set_int(userConfig, "BasicWindow", "MultiviewLayout",
			       static_cast<int>(MultiviewLayout::VERTICAL_RIGHT_8_SCENES));
		return true;
	}

	return false;
}

bool OBSApp::InitGlobalConfig()
{
	char path[512];

	int len = GetAppConfigPath(path, sizeof(path), "obs-studio/global.ini");
	if (len <= 0) {
		return false;
	}

	int errorcode = appConfig.Open(path, CONFIG_OPEN_ALWAYS);
	if (errorcode != CONFIG_SUCCESS) {
		OBSErrorBox(NULL, "Failed to open global.ini: %d", errorcode);
		return false;
	}

	uint32_t lastVersion = config_get_int(appConfig, "General", "LastVersion");

	if (lastVersion && lastVersion < MAKE_SEMANTIC_VERSION(31, 0, 0)) {
		bool migratedUserSettings = config_get_bool(appConfig, "General", "Pre31Migrated");

		if (!migratedUserSettings) {
			bool migrated = MigrateGlobalSettings();

			config_set_bool(appConfig, "General", "Pre31Migrated", migrated);
			config_save_safe(appConfig, "tmp", nullptr);
		}
	}

	InitGlobalConfigDefaults();
	InitGlobalLocationDefaults();

	dualOutputActive = config_get_bool(appConfig, "General", "DualOutputActive");

	std::filesystem::path defaultUserConfigLocation =
		std::filesystem::u8path(config_get_default_string(appConfig, "Locations", "Configuration"));
	std::filesystem::path defaultUserScenesLocation =
		std::filesystem::u8path(config_get_default_string(appConfig, "Locations", "SceneCollections"));
	std::filesystem::path defaultUserProfilesLocation =
		std::filesystem::u8path(config_get_default_string(appConfig, "Locations", "Profiles"));

	if (IsPortableMode()) {
		userConfigLocation = std::move(defaultUserConfigLocation);
		userScenesLocation = std::move(defaultUserScenesLocation);
		userProfilesLocation = std::move(defaultUserProfilesLocation);
	} else {
		std::filesystem::path currentUserConfigLocation =
			std::filesystem::u8path(config_get_string(appConfig, "Locations", "Configuration"));
		std::filesystem::path currentUserScenesLocation =
			std::filesystem::u8path(config_get_string(appConfig, "Locations", "SceneCollections"));
		std::filesystem::path currentUserProfilesLocation =
			std::filesystem::u8path(config_get_string(appConfig, "Locations", "Profiles"));

		userConfigLocation = (std::filesystem::exists(currentUserConfigLocation))
					     ? std::move(currentUserConfigLocation)
					     : std::move(defaultUserConfigLocation);
		userScenesLocation = (std::filesystem::exists(currentUserScenesLocation))
					     ? std::move(currentUserScenesLocation)
					     : std::move(defaultUserScenesLocation);
		userProfilesLocation = (std::filesystem::exists(currentUserProfilesLocation))
					       ? std::move(currentUserProfilesLocation)
					       : std::move(defaultUserProfilesLocation);
	}

	bool userConfigResult = InitUserConfig(userConfigLocation, lastVersion);

	return userConfigResult;
}

bool OBSApp::InitUserConfig(std::filesystem::path &userConfigLocation, uint32_t lastVersion)
{
	const std::string userConfigFile = userConfigLocation.u8string() + "/obs-studio/user.ini";

	int errorCode = userConfig.Open(userConfigFile.c_str(), CONFIG_OPEN_ALWAYS);

	if (errorCode != CONFIG_SUCCESS) {
		OBSErrorBox(nullptr, "Failed to open user.ini: %d", errorCode);
		return false;
	}

	MigrateLegacySettings(lastVersion);
	InitUserConfigDefaults();

	return true;
}

void OBSApp::MigrateLegacySettings(const uint32_t lastVersion)
{
	bool hasChanges = false;

	const uint32_t v19 = MAKE_SEMANTIC_VERSION(19, 0, 0);
	const uint32_t v21 = MAKE_SEMANTIC_VERSION(21, 0, 0);
	const uint32_t v23 = MAKE_SEMANTIC_VERSION(23, 0, 0);
	const uint32_t v24 = MAKE_SEMANTIC_VERSION(24, 0, 0);
	const uint32_t v24_1 = MAKE_SEMANTIC_VERSION(24, 1, 0);

	const map<uint32_t, string> defaultsMap{
		{{v19, "Pre19Defaults"}, {v21, "Pre21Defaults"}, {v23, "Pre23Defaults"}, {v24_1, "Pre24.1Defaults"}}};

	for (auto &[version, configKey] : defaultsMap) {
		if (!config_has_user_value(userConfig, "General", configKey.c_str())) {
			bool useOldDefaults = lastVersion && lastVersion < version;
			config_set_bool(userConfig, "General", configKey.c_str(), useOldDefaults);

			hasChanges = true;
		}
	}

	if (config_has_user_value(userConfig, "BasicWindow", "MultiviewLayout")) {
		const char *layout = config_get_string(userConfig, "BasicWindow", "MultiviewLayout");

		bool layoutUpdated = UpdatePre22MultiviewLayout(layout);

		hasChanges = hasChanges | layoutUpdated;
	}

	if (lastVersion && lastVersion < v24) {
		bool disableHotkeysInFocus = config_get_bool(userConfig, "General", "DisableHotkeysInFocus");

		if (disableHotkeysInFocus) {
			config_set_string(userConfig, "General", "HotkeyFocusType", "DisableHotkeysInFocus");
		}

		hasChanges = true;
	}

	if (hasChanges) {
		userConfig.SaveSafe("tmp");
	}
}

static constexpr string_view OBSGlobalIniPath = "/obs-studio/global.ini";
static constexpr string_view OBSUserIniPath = "/obs-studio/user.ini";

bool OBSApp::MigrateGlobalSettings()
{
	char path[512];

	int len = GetAppConfigPath(path, sizeof(path), nullptr);
	if (len <= 0) {
		OBSErrorBox(nullptr, "Unable to get global configuration path.");
		return false;
	}

	std::string legacyConfigFileString;
	legacyConfigFileString.reserve(strlen(path) + OBSGlobalIniPath.size());
	legacyConfigFileString.append(path).append(OBSGlobalIniPath);

	const std::filesystem::path legacyGlobalConfigFile = std::filesystem::u8path(legacyConfigFileString);

	std::string configFileString;
	configFileString.reserve(strlen(path) + OBSUserIniPath.size());
	configFileString.append(path).append(OBSUserIniPath);

	const std::filesystem::path userConfigFile = std::filesystem::u8path(configFileString);

	if (std::filesystem::exists(userConfigFile)) {
		OBSErrorBox(nullptr,
			    "Unable to migrate global configuration - user configuration file already exists.");
		return false;
	}

	try {
		std::filesystem::copy(legacyGlobalConfigFile, userConfigFile);
	} catch (const std::filesystem::filesystem_error &) {
		OBSErrorBox(nullptr, "Unable to migrate global configuration - copy failed.");
		return false;
	}

	return true;
}

bool OBSApp::InitLocale()
{
	ProfileScope("OBSApp::InitLocale");

	const char *lang = config_get_string(userConfig, "General", "Language");
	bool userLocale = config_has_user_value(userConfig, "General", "Language");
	if (!userLocale || !lang || lang[0] == '\0')
		lang = DEFAULT_LANG;

	locale = lang;

	// set basic default application locale
	if (!locale.empty())
		QLocale::setDefault(QLocale(QString::fromStdString(locale).replace('-', '_')));

	string englishPath;
	if (!GetDataFilePath("locale/" DEFAULT_LANG ".ini", englishPath)) {
		OBSErrorBox(NULL, "Failed to find locale/" DEFAULT_LANG ".ini");
		return false;
	}

	textLookup = text_lookup_create(englishPath.c_str());
	if (!textLookup) {
		OBSErrorBox(NULL, "Failed to create locale from file '%s'", englishPath.c_str());
		return false;
	}

	bool defaultLang = astrcmpi(lang, DEFAULT_LANG) == 0;

	if (userLocale && defaultLang)
		return true;

	if (!userLocale && defaultLang) {
		for (auto &locale_ : GetPreferredLocales()) {
			if (locale_ == lang)
				return true;

			stringstream file;
			file << "locale/" << locale_ << ".ini";

			string path;
			if (!GetDataFilePath(file.str().c_str(), path))
				continue;

			if (!text_lookup_add(textLookup, path.c_str()))
				continue;

			blog(LOG_INFO, "Using preferred locale '%s'", locale_.c_str());
			locale = locale_;

			// set application default locale to the new choosen one
			if (!locale.empty())
				QLocale::setDefault(QLocale(QString::fromStdString(locale).replace('-', '_')));

			return true;
		}

		return true;
	}

	stringstream file;
	file << "locale/" << lang << ".ini";

	string path;
	if (GetDataFilePath(file.str().c_str(), path)) {
		if (!text_lookup_add(textLookup, path.c_str()))
			blog(LOG_ERROR, "Failed to add locale file '%s'", path.c_str());
	} else {
		blog(LOG_ERROR, "Could not find locale file '%s'", file.str().c_str());
	}

	return true;
}

#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
void ParseBranchesJson(const std::string &jsonString, vector<UpdateBranch> &out, std::string &error)
{
	JsonBranches branches;

	try {
		nlohmann::json json = nlohmann::json::parse(jsonString);
		branches = json.get<JsonBranches>();
	} catch (nlohmann::json::exception &e) {
		error = e.what();
		return;
	}

	for (const JsonBranch &json_branch : branches) {
#ifdef _WIN32
		if (!json_branch.windows)
			continue;
#elif defined(__APPLE__)
		if (!json_branch.macos)
			continue;
#endif

		UpdateBranch branch = {
			QString::fromStdString(json_branch.name),
			QString::fromStdString(json_branch.display_name),
			QString::fromStdString(json_branch.description),
			json_branch.enabled,
			json_branch.visible,
		};
		out.push_back(branch);
	}
}

bool LoadBranchesFile(vector<UpdateBranch> &out)
{
	string error;
	string branchesText;

	BPtr<char> branchesFilePath = GetAppConfigPathPtr("obs-studio/updates/branches.json");

	QFile branchesFile(branchesFilePath.Get());
	if (!branchesFile.open(QIODevice::ReadOnly)) {
		error = "Opening file failed.";
		goto fail;
	}

	branchesText = branchesFile.readAll();
	if (branchesText.empty()) {
		error = "File empty.";
		goto fail;
	}

	ParseBranchesJson(branchesText, out, error);
	if (error.empty())
		return !out.empty();

fail:
	blog(LOG_WARNING, "Loading branches from file failed: %s", error.c_str());
	return false;
}
#endif

void OBSApp::SetBranchData(const string &data)
{
#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
	string error;
	vector<UpdateBranch> result;

	ParseBranchesJson(data, result, error);

	if (!error.empty()) {
		blog(LOG_WARNING, "Reading branches JSON response failed: %s", error.c_str());
		return;
	}

	if (!result.empty())
		updateBranches = result;

	branches_loaded = true;
#else
	UNUSED_PARAMETER(data);
#endif
}

void OBSApp::SetDualOutputActive(bool active)
{
	if (dualOutputActive == active)
		return;

	dualOutputActive = active;
	config_set_bool(appConfig, "General", "DualOutputActive", active); // Save to global config
	config_save_safe(appConfig, "tmp", nullptr);

	blog(LOG_INFO, "Dual Output mode %s.", active ? "activated" : "deactivated");

	// TODO: Trigger a video system reset if OBS is initialized.
	// This would typically involve calling something like OBSBasic::Get()->ResetVideo()
	// or a dedicated function in OBSApp that manages video re-initialization.
	// This reset needs to happen *after* the main window and settings are loaded
	// so that horizontal_ovi and vertical_ovi can be correctly populated from config.
	if (libobs_initialized && mainWindow) {
		blog(LOG_INFO, "TODO: Request video system reset due to DualOutputActive change.");
		// Example: OBSBasic::Get()->ResetVideo(); // This needs to be callable and safe
		// For now, this is a placeholder. The actual reset should be triggered
		// after settings are applied and OBS is ready.
		// A signal/slot mechanism or a deferred call might be appropriate.
		QMetaObject::invokeMethod(OBSBasic::Get(), "ResetVideo", Qt::QueuedConnection);
		// Also need to re-evaluate and setup outputs based on the new state
		QMetaObject::invokeMethod(OBSBasic::Get(), "ResetOutputs", Qt::QueuedConnection);
	}
}

const obs_video_info* OBSApp::GetHorizontalVideoInfo() const
{
	return &horizontal_ovi;
}

const obs_video_info* OBSApp::GetVerticalVideoInfo() const
{
	return &vertical_ovi;
}

void OBSApp::UpdateHorizontalVideoInfo(const obs_video_info& ovi)
{
	horizontal_ovi = ovi;
	// TODO: Potentially save this to config or mark config dirty if app manages saving directly
	// For now, horizontal_ovi is typically derived from profile config loaded by OBSBasic
	// and set via OBSBasic::ResetVideo -> App()->UpdateHorizontalVideoInfo.
	// If it needs to be saved directly from here, similar logic to UpdateVerticalVideoInfo would apply.
	blog(LOG_INFO, "Horizontal video info updated: %dx%d @ %d/%d fps", ovi.base_width, ovi.base_height, ovi.fps_num, ovi.fps_den);
}

void OBSApp::UpdateVerticalVideoInfo(const obs_video_info& ovi)
{
	vertical_ovi = ovi;
	if (OBSBasic::Get() && OBSBasic::Get()->Config()) {
		config_t *profile_config = OBSBasic::Get()->Config();
		config_set_uint(profile_config, "Video", "V_BaseCX", vertical_ovi.base_width);
		config_set_uint(profile_config, "Video", "V_BaseCY", vertical_ovi.base_height);
		config_set_uint(profile_config, "Video", "V_OutputCX", vertical_ovi.output_width);
		config_set_uint(profile_config, "Video", "V_OutputCY", vertical_ovi.output_height);
		// Assuming FPS and Color settings might be shared or handled by UI separately for now.
		// If they need to be distinct and saved:
		// config_set_string(profile_config, "Video", "V_FPSCommon", ...); // Needs conversion from num/den
		// config_set_string(profile_config, "Video", "V_ScaleType", ...); // Needs conversion to string
		// config_set_string(profile_config, "Video", "V_ColorFormat", ...); // Needs conversion to string

		OBSBasic::Get()->SaveProjectDeferred(); // Mark profile as dirty
		blog(LOG_INFO, "Vertical video info updated and saved to profile: %dx%d @ %d/%d fps",
			 vertical_ovi.base_width, vertical_ovi.base_height, vertical_ovi.fps_num, vertical_ovi.fps_den);
	} else {
		blog(LOG_WARNING, "Vertical video info updated, but profile config not available for saving: %dx%d @ %d/%d fps",
			 vertical_ovi.base_width, vertical_ovi.base_height, vertical_ovi.fps_num, vertical_ovi.fps_den);
	}
}

obs_source_t* OBSApp::GetCurrentHorizontalScene() const
{
	return current_horizontal_scene;
}

void OBSApp::SetCurrentHorizontalScene(obs_source_t *scene)
{
	obs_source_t *old_scene = current_horizontal_scene;
	current_horizontal_scene = scene;
	if (current_horizontal_scene)
		obs_source_addref(current_horizontal_scene);
	if (old_scene)
		obs_source_release(old_scene);

	blog(LOG_INFO, "Current horizontal scene set.");

	// If horizontal stream output is active, update its video source
	// Note: The main OBS video pipeline (obs_get_video()) is what's typically fed to outputs
	// that don't have a specific source set. Transitions update the source feeding into obs_get_video().
	// Explicitly setting the source on the output here might override transitions if not handled carefully.
	// However, for direct scene-to-output mapping without main transitions, this would be needed.
	// Given current transition logic updates obs_core_video()->output_source, this explicit call might
	// conflict or be redundant for the horizontal output if it's meant to follow main transitions.
	// For now, let's assume the main output follows the core transition mechanism.
	// If direct control is needed:
	// if (horizontal_stream_output && obs_output_active(horizontal_stream_output)) {
	// 	obs_output_set_video_source(horizontal_stream_output, current_horizontal_scene);
	// 	blog(LOG_INFO, "Updated active horizontal stream output video source to: %s",
	// 	     current_horizontal_scene ? obs_source_get_name(current_horizontal_scene) : "(none)");
	// }

	emit horizontalSceneChanged(current_horizontal_scene);
}

obs_source_t* OBSApp::GetCurrentVerticalScene() const
{
	return current_vertical_scene;
}

void OBSApp::SetCurrentVerticalScene(obs_source_t *scene)
{
	obs_source_t *old_scene = current_vertical_scene;
	current_vertical_scene = scene;
	if (current_vertical_scene)
		obs_source_addref(current_vertical_scene);
	if (old_scene)
		obs_source_release(old_scene);

	blog(LOG_INFO, "Current vertical scene set.");

	// If vertical stream output is active, update its video source
	if (vertical_stream_output && obs_output_active(vertical_stream_output)) {
		obs_output_set_video_source(vertical_stream_output, current_vertical_scene);
		blog(LOG_INFO, "Updated active vertical stream output video source to: %s",
		     current_vertical_scene ? obs_source_get_name(current_vertical_scene) : "(none)");
	}

	emit verticalSceneChanged(current_vertical_scene);
}

void OBSApp::SetupOutputs()
{
	blog(LOG_INFO, "OBSApp::SetupOutputs: Initializing outputs.");

	// Ensure existing resources are released before creating new ones
	obs_output_release(horizontal_stream_output); horizontal_stream_output = nullptr;
	obs_service_release(horizontal_stream_service); horizontal_stream_service = nullptr;
	obs_output_release(vertical_stream_output); vertical_stream_output = nullptr;
	obs_service_release(vertical_stream_service); vertical_stream_service = nullptr;

	config_t *profile_config = OBSBasic::Get()->Config();
	bool simple_mode = config_get_bool(profile_config, "Output", "ModeSimple");

	// --------------------- HORIZONTAL STREAM ---------------------
	const char *h_service_type_from_stream_page = config_get_string(profile_config, "Stream", "Service");
	if (h_service_type_from_stream_page && *h_service_type_from_stream_page) {
		OBSDataAutoRelease h_service_settings = obs_data_create();
		// obs_data_set_string(h_service_settings, "service", h_service_type_from_stream_page); // This is not how obs_service_create works; type is passed directly.
		obs_data_set_string(h_service_settings, "server", config_get_string(profile_config, "Stream", "Server"));
		obs_data_set_string(h_service_settings, "key", config_get_string(profile_config, "Stream", "Key"));
		if (config_get_bool(profile_config, "Stream", "UseAuth")) {
			obs_data_set_bool(h_service_settings, "use_auth", true);
			obs_data_set_string(h_service_settings, "username", config_get_string(profile_config, "Stream", "Username"));
			obs_data_set_string(h_service_settings, "password", config_get_string(profile_config, "Stream", "Password"));
		}


		horizontal_stream_service = obs_service_create(h_service_type_from_stream_page, "horizontal_stream_service_internal", h_service_settings, nullptr);

		if (horizontal_stream_service) {
			// obs_service_apply_settings(horizontal_stream_service, h_service_settings); // Settings are applied during creation or via obs_service_update

			const char *h_encoder_id = nullptr;
			OBSDataAutoRelease h_encoder_settings = obs_data_create();

			if (simple_mode) {
				h_encoder_id = config_get_string(profile_config, "Output", "StreamEncoder");
				obs_data_set_int(h_encoder_settings, "bitrate", config_get_int(profile_config, "Output", "VBitrate"));
				// TODO: Apply other simple mode encoder settings (preset, tune, custom from "Output" section)
				if (config_get_bool(profile_config, "Output", "UseAdvanced")) {
					const char* preset = config_get_string(profile_config, "Output", "Preset");
					if (preset && *preset) obs_data_set_string(h_encoder_settings, "preset", preset);
					const char* tune = config_get_string(profile_config, "Output", "Tune");
					if (tune && *tune && strcmp(tune, Str("None")) != 0) obs_data_set_string(h_encoder_settings, "tune", tune);
					const char* custom_settings = config_get_string(profile_config, "Output", "StreamCustom");
					if (custom_settings && *custom_settings) obs_data_set_string(h_encoder_settings, "custom", custom_settings); // Encoder might look for "custom" or parse it
				}
			} else { // Advanced mode
				h_encoder_id = config_get_string(profile_config, "AdvOut", "Encoder");
				BPtr<char> profile_config_path = obs_frontend_get_profile_config_path();
				std::string json_settings_path = profile_config_path.Get();
				json_settings_path += "/streamEncoder.json";

				OBSDataAutoRelease loaded_json_settings = obs_data_create_from_json_file_safe(json_settings_path.c_str(), nullptr);
				if (loaded_json_settings) {
					obs_data_apply(h_encoder_settings, loaded_json_settings);
					blog(LOG_INFO, "Horizontal Adv. Encoder: %s. Loaded settings from %s.", h_encoder_id, json_settings_path.c_str());
				} else {
					blog(LOG_WARNING, "Horizontal Adv. Encoder: %s. Failed to load settings from %s.", h_encoder_id, json_settings_path.c_str());
				}
			}

			if (h_encoder_id && *h_encoder_id) {
				horizontal_stream_output = obs_output_create(h_encoder_id, "horizontal_stream_output_internal", h_encoder_settings, nullptr);
				if (horizontal_stream_output) {
					obs_output_set_service(horizontal_stream_output, horizontal_stream_service);
					obs_output_set_media(horizontal_stream_output, obs_get_video(), obs_get_audio());
					blog(LOG_INFO, "Horizontal stream output '%s' created for service '%s'.", h_encoder_id, h_service_type_from_stream_page);
				} else {
					blog(LOG_WARNING, "Failed to create horizontal stream output with encoder '%s'.", h_encoder_id);
					obs_service_release(horizontal_stream_service); horizontal_stream_service = nullptr;
				}
			} else {
				blog(LOG_WARNING, "Horizontal stream encoder ID is missing or invalid.");
				obs_service_release(horizontal_stream_service); horizontal_stream_service = nullptr;
			}
		} else {
			blog(LOG_WARNING, "Failed to create horizontal stream service type '%s'.", h_service_type_from_stream_page);
		}
	} else {
		blog(LOG_INFO, "Horizontal stream service type not configured (from main Stream page).");
	}


	// --------------------- VERTICAL STREAM ---------------------
	if (IsDualOutputActive()) {
		const char *v_service_type = nullptr;
		OBSDataAutoRelease v_service_settings = obs_data_create();

		if (simple_mode) {
			v_service_type = config_get_string(profile_config, "Output", "Service_V_Stream"); // This key needs to be saved by Simple Output settings
			obs_data_set_string(v_service_settings, "server", config_get_string(profile_config, "Output", "Server_V_Stream"));
			obs_data_set_string(v_service_settings, "key", config_get_string(profile_config, "Output", "Key_V_Stream"));
			// Note: Simple mode UI for vertical stream doesn't have separate auth. Assumes key-based or no auth.
		} else { // Advanced mode for vertical stream
			v_service_type = config_get_string(profile_config, "AdvOut", "VStream_ServiceType");
			obs_data_set_string(v_service_settings, "server", config_get_string(profile_config, "AdvOut", "VStream_Server"));
			obs_data_set_string(v_service_settings, "key", config_get_string(profile_config, "AdvOut", "VStream_Key"));
			if (config_get_bool(profile_config, "AdvOut", "VStream_UseAuth")) {
				obs_data_set_bool(v_service_settings, "use_auth", true);
				obs_data_set_string(v_service_settings, "username", config_get_string(profile_config, "AdvOut", "VStream_Username"));
				obs_data_set_string(v_service_settings, "password", config_get_string(profile_config, "AdvOut", "VStream_Password"));
			}
		}

		// Apply audio settings to v_service_settings before creating the service
		if (simple_mode) {
			obs_data_set_int(v_service_settings, "abitrate", config_get_int(profile_config, "Output", "ABitrate_V_Stream"));
			// obs_data_set_string(v_service_settings, "audio_encoder_id", "ffmpeg_aac"); // Example, if needed by service
		} else { // Advanced Mode
			const char* audio_encoder_id_v = config_get_string(profile_config, "AdvOut", "AEncoder_V_Stream");
			if (audio_encoder_id_v && *audio_encoder_id_v) {
				obs_data_set_string(v_service_settings, "audio_encoder_id", audio_encoder_id_v);
			}

			int audio_track_v = config_get_int(profile_config, "AdvOut", "TrackIndex_V_Stream"); // 1-6
			if (audio_track_v >= 1 && audio_track_v <= 6) {
				char key[32];
				snprintf(key, sizeof(key), "AudioBitrateTrack%d", audio_track_v);
				long long track_bitrate = config_get_int(profile_config, "AdvOut", key);
				obs_data_set_int(v_service_settings, "abitrate", track_bitrate);
				// The service itself would need to be aware of which track to encode if not just taking the main mix from obs_get_audio().
				// This might involve a service-specific property like "audio_track_index".
				// For now, we assume TrackIndex_V_Stream primarily informs bitrate selection from global track settings.
			}
		}


		if (v_service_type && *v_service_type) {
			vertical_stream_service = obs_service_create(v_service_type, "vertical_stream_service_internal", v_service_settings, nullptr);

			if (vertical_stream_service) {
				// obs_service_apply_settings(vertical_stream_service, v_service_settings); // Applied at creation
				// Audio encoder settings are now part of v_service_settings or handled by service defaults.

				const char *v_encoder_id = nullptr;
				OBSDataAutoRelease v_encoder_settings = obs_data_create();

				if (simple_mode) {
					v_encoder_id = config_get_string(profile_config, "Output", "StreamEncoder_V_Stream");
					obs_data_set_int(v_encoder_settings, "bitrate", config_get_int(profile_config, "Output", "VBitrate_V_Stream"));
					// TODO: Apply other simple mode encoder settings for vertical (preset, tune, custom from "Output" section with _V_Stream suffix)
					if (config_get_bool(profile_config, "Output", "UseAdvanced_V_Stream")) {
						const char* preset_v = config_get_string(profile_config, "Output", "Preset_V_Stream");
						if (preset_v && *preset_v) obs_data_set_string(v_encoder_settings, "preset", preset_v);
						const char* tune_v = config_get_string(profile_config, "Output", "Tune_V_Stream");
						if (tune_v && *tune_v && strcmp(tune_v, Str("None")) != 0) obs_data_set_string(v_encoder_settings, "tune", tune_v);
						const char* custom_settings_v = config_get_string(profile_config, "Output", "StreamCustom_V_Stream");
						if (custom_settings_v && *custom_settings_v) obs_data_set_string(v_encoder_settings, "custom", custom_settings_v);
					}
				} else { // Advanced mode for vertical stream
					v_encoder_id = config_get_string(profile_config, "AdvOut", "Encoder_V_Stream");
					BPtr<char> profile_config_path = obs_frontend_get_profile_config_path();
					std::string v_json_settings_path = profile_config_path.Get();
					v_json_settings_path += "/streamEncoder_v_stream.json";

					OBSDataAutoRelease v_loaded_json_settings = obs_data_create_from_json_file_safe(v_json_settings_path.c_str(), nullptr);
					if (v_loaded_json_settings) {
						obs_data_apply(v_encoder_settings, v_loaded_json_settings);
						blog(LOG_INFO, "Vertical Adv. Encoder: %s. Loaded settings from %s.", v_encoder_id, v_json_settings_path.c_str());
					} else {
						blog(LOG_WARNING, "Vertical Adv. Encoder: %s. Failed to load settings from %s.", v_encoder_id, v_json_settings_path.c_str());
						// Fallback for bitrate if JSON is missing/empty and encoder ID is valid
						if (v_encoder_id && *v_encoder_id && !obs_data_has_user_value(v_encoder_settings, "bitrate")) {
							// Try to get a bitrate from a general advanced vertical stream bitrate field if one exists,
							// or fallback to a sensible default. This part is tricky without a dedicated UI field.
							// For now, we rely on the JSON or encoder defaults.
						}
					}
				}

				if (v_encoder_id && *v_encoder_id) {
					vertical_stream_output = obs_output_create(v_encoder_id, "vertical_stream_output_internal", v_encoder_settings, nullptr);
					if (vertical_stream_output) {
						obs_output_set_service(vertical_stream_output, vertical_stream_service);

						const obs_video_info* v_ovi = App()->GetVerticalVideoInfo();
						if (v_ovi && v_ovi->base_width > 0) {
							obs_output_override_video_settings(vertical_stream_output, v_ovi);
							blog(LOG_INFO, "Vertical output video settings overridden to: %dx%d @ %d/%d fps",
								v_ovi->output_width, v_ovi->output_height, v_ovi->fps_num, v_ovi->fps_den);
						} else {
							blog(LOG_WARNING, "Vertical OVI not valid for overriding output settings. Output may fail or use unexpected defaults.");
						}

						obs_source_t *vertical_scene_to_stream = App()->GetCurrentVerticalScene();
						if (vertical_scene_to_stream) {
							obs_output_set_video_source(vertical_stream_output, vertical_scene_to_stream);
							blog(LOG_INFO, "Vertical stream output video source set to: %s", obs_source_get_name(vertical_scene_to_stream));
						} else {
							obs_output_set_video_source(vertical_stream_output, nullptr);
							blog(LOG_WARNING, "No current vertical scene set for vertical stream output.");
						}
						obs_output_set_audio_source(vertical_stream_output, obs_get_audio());

						blog(LOG_INFO, "Vertical stream output '%s' created for service '%s'.", v_encoder_id, v_service_type);
					} else {
						blog(LOG_WARNING, "Failed to create vertical stream output with encoder '%s'.", v_encoder_id);
						obs_service_release(vertical_stream_service); vertical_stream_service = nullptr;
					}
				} else {
					blog(LOG_WARNING, "Vertical stream encoder ID is missing or invalid.");
					obs_service_release(vertical_stream_service); vertical_stream_service = nullptr;
				}
			} else {
				blog(LOG_WARNING, "Failed to create vertical stream service type '%s'.", v_service_type);
			}
		} else {
			blog(LOG_INFO, "Vertical stream service type not configured (dual output is active).");
		}
	}
	blog(LOG_INFO, "OBSApp::SetupOutputs finished processing.");
}

bool OBSApp::StartStreamingInternal()
{
	blog(LOG_INFO, "OBSApp::StartStreamingInternal called");
	bool success_h = false;
	bool success_v = false;

	if (horizontal_stream_output) {
		if (obs_output_start(horizontal_stream_output)) {
			success_h = true;
			blog(LOG_INFO, "Horizontal stream started successfully.");
		} else {
			blog(LOG_WARNING, "Failed to start horizontal stream: %s", obs_output_get_last_error(horizontal_stream_output));
		}
	} else {
		blog(LOG_WARNING, "Horizontal stream output not configured.");
	}

	if (IsDualOutputActive()) {
		if (vertical_stream_output) {
			if (obs_output_start(vertical_stream_output)) {
				success_v = true;
				blog(LOG_INFO, "Vertical stream started successfully.");
			} else {
				blog(LOG_WARNING, "Failed to start vertical stream: %s", obs_output_get_last_error(vertical_stream_output));
			}
		} else {
			blog(LOG_WARNING, "Vertical stream output not configured (dual output is active).");
		}
		return success_h && success_v; // Both must succeed if dual is active and configured
	}

	return success_h; // Only horizontal matters if not dual or vertical not configured
}

void OBSApp::StopStreamingInternal(bool force)
{
	blog(LOG_INFO, "OBSApp::StopStreamingInternal called (force: %s)", force ? "true" : "false");
	if (horizontal_stream_output) {
		obs_output_stop(horizontal_stream_output); // obs_output_stop handles force internally if needed by type
		blog(LOG_INFO, "Horizontal stream stopped.");
	}
	if (vertical_stream_output) { // Only stop if it exists
		obs_output_stop(vertical_stream_output);
		blog(LOG_INFO, "Vertical stream stopped.");
	}
}

std::vector<UpdateBranch> OBSApp::GetBranches()
{
	vector<UpdateBranch> out;
	/* Always ensure the default branch exists */
	out.push_back(UpdateBranch{"stable", "", "", true, true});

#if defined(_WIN32) || defined(ENABLE_SPARKLE_UPDATER)
	if (!branches_loaded) {
		vector<UpdateBranch> result;
		if (LoadBranchesFile(result))
			updateBranches = result;

		branches_loaded = true;
	}
#endif

	/* Copy additional branches to result (if any) */
	if (!updateBranches.empty())
		out.insert(out.end(), updateBranches.begin(), updateBranches.end());

	return out;
}

OBSApp::OBSApp(int &argc, char **argv, profiler_name_store_t *store)
	: QApplication(argc, argv),
	  profilerNameStore(store)
{
	/* fix float handling */
#if defined(Q_OS_UNIX)
	if (!setlocale(LC_NUMERIC, "C"))
		blog(LOG_WARNING, "Failed to set LC_NUMERIC to C locale");
#endif

#ifndef _WIN32
	/* Handle SIGINT properly */
	socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd);
	snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
	connect(snInt, &QSocketNotifier::activated, this, &OBSApp::ProcessSigInt);
#else
	connect(qApp, &QGuiApplication::commitDataRequest, this, &OBSApp::commitData);
#endif

	sleepInhibitor = os_inhibit_sleep_create("OBS Video/audio");

#ifndef __APPLE__
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
#endif

	setDesktopFileName("com.obsproject.Studio");
}

OBSApp::~OBSApp()
{
#ifdef _WIN32
	bool disableAudioDucking = config_get_bool(appConfig, "Audio", "DisableAudioDucking");
	if (disableAudioDucking)
		DisableAudioDucking(false);
#else
	delete snInt;
	close(sigintFd[0]);
	close(sigintFd[1]);
#endif

#ifdef __APPLE__
	bool vsyncDisabled = config_get_bool(appConfig, "Video", "DisableOSXVSync");
	bool resetVSync = config_get_bool(appConfig, "Video", "ResetOSXVSyncOnExit");
	if (vsyncDisabled && resetVSync)
		EnableOSXVSync(true);
#endif

	os_inhibit_sleep_set_active(sleepInhibitor, false);
	os_inhibit_sleep_destroy(sleepInhibitor);

	if (libobs_initialized)
		obs_shutdown();
}

static void move_basic_to_profiles(void)
{
	char path[512];

	if (GetAppConfigPath(path, 512, "obs-studio/basic") <= 0) {
		return;
	}

	const std::filesystem::path basicPath = std::filesystem::u8path(path);

	if (!std::filesystem::exists(basicPath)) {
		return;
	}

	const std::filesystem::path profilesPath =
		App()->userProfilesLocation / std::filesystem::u8path("obs-studio/basic/profiles");

	if (std::filesystem::exists(profilesPath)) {
		return;
	}

	try {
		std::filesystem::create_directories(profilesPath);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_ERROR, "Failed to create profiles directory for migration from basic profile\n%s",
		     error.what());
		return;
	}

	const std::filesystem::path newProfilePath = profilesPath / std::filesystem::u8path(Str("Untitled"));

	for (auto &entry : std::filesystem::directory_iterator(basicPath)) {
		if (entry.is_directory()) {
			continue;
		}

		if (entry.path().filename().u8string() == "scenes.json") {
			continue;
		}

		if (!std::filesystem::exists(newProfilePath)) {
			try {
				std::filesystem::create_directory(newProfilePath);
			} catch (const std::filesystem::filesystem_error &error) {
				blog(LOG_ERROR, "Failed to create profile directory for 'Untitled'\n%s", error.what());
				return;
			}
		}

		const filesystem::path destinationFile = newProfilePath / entry.path().filename();

		const auto copyOptions = std::filesystem::copy_options::overwrite_existing;

		try {
			std::filesystem::copy(entry.path(), destinationFile, copyOptions);
		} catch (const std::filesystem::filesystem_error &error) {
			blog(LOG_ERROR, "Failed to copy basic profile file '%s' to new profile 'Untitled'\n%s",
			     entry.path().filename().u8string().c_str(), error.what());

			return;
		}
	}
}

static void move_basic_to_scene_collections(void)
{
	char path[512];

	if (GetAppConfigPath(path, 512, "obs-studio/basic") <= 0) {
		return;
	}

	const std::filesystem::path basicPath = std::filesystem::u8path(path);

	if (!std::filesystem::exists(basicPath)) {
		return;
	}

	const std::filesystem::path sceneCollectionPath =
		App()->userScenesLocation / std::filesystem::u8path("obs-studio/basic/scenes");

	if (std::filesystem::exists(sceneCollectionPath)) {
		return;
	}

	try {
		std::filesystem::create_directories(sceneCollectionPath);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_ERROR,
		     "Failed to create scene collection directory for migration from basic scene collection\n%s",
		     error.what());
		return;
	}

	const std::filesystem::path sourceFile = basicPath / std::filesystem::u8path("scenes.json");
	const std::filesystem::path destinationFile =
		(sceneCollectionPath / std::filesystem::u8path(Str("Untitled"))).replace_extension(".json");

	try {
		std::filesystem::rename(sourceFile, destinationFile);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_ERROR, "Failed to rename basic scene collection file:\n%s", error.what());
		return;
	}
}

void OBSApp::AppInit()
{
	ProfileScope("OBSApp::AppInit");

	if (!MakeUserDirs())
		throw "Failed to create required user directories";
	if (!InitGlobalConfig())
		throw "Failed to initialize global config";
	if (!InitLocale())
		throw "Failed to load locale";
	if (!InitTheme())
		throw "Failed to load theme";

	config_set_default_string(userConfig, "Basic", "Profile", Str("Untitled"));
	config_set_default_string(userConfig, "Basic", "ProfileDir", Str("Untitled"));
	config_set_default_string(userConfig, "Basic", "SceneCollection", Str("Untitled"));
	config_set_default_string(userConfig, "Basic", "SceneCollectionFile", Str("Untitled"));
	config_set_default_bool(userConfig, "Basic", "ConfigOnNewProfile", true);

	const std::string_view profileName{config_get_string(userConfig, "Basic", "Profile")};

	if (profileName.empty()) {
		config_set_string(userConfig, "Basic", "Profile", Str("Untitled"));
		config_set_string(userConfig, "Basic", "ProfileDir", Str("Untitled"));
	}

	const std::string_view sceneCollectionName{config_get_string(userConfig, "Basic", "SceneCollection")};

	if (sceneCollectionName.empty()) {
		config_set_string(userConfig, "Basic", "SceneCollection", Str("Untitled"));
		config_set_string(userConfig, "Basic", "SceneCollectionFile", Str("Untitled"));
	}

#ifdef _WIN32
	bool disableAudioDucking = config_get_bool(appConfig, "Audio", "DisableAudioDucking");
	if (disableAudioDucking)
		DisableAudioDucking(true);
#endif

#ifdef __APPLE__
	if (config_get_bool(appConfig, "Video", "DisableOSXVSync"))
		EnableOSXVSync(false);
#endif

	UpdateHotkeyFocusSetting(false);

	move_basic_to_profiles();
	move_basic_to_scene_collections();

	if (!MakeUserProfileDirs())
		throw "Failed to create profile directories";
}

const char *OBSApp::GetRenderModule() const
{
	const char *renderer = config_get_string(appConfig, "Video", "Renderer");

	return (astrcmpi(renderer, "Direct3D 11") == 0) ? DL_D3D11 : DL_OPENGL;
}

static bool StartupOBS(const char *locale, profiler_name_store_t *store)
{
	char path[512];

	if (GetAppConfigPath(path, sizeof(path), "obs-studio/plugin_config") <= 0)
		return false;

	return obs_startup(locale, path, store);
}

inline void OBSApp::ResetHotkeyState(bool inFocus)
{
	obs_hotkey_enable_background_press((inFocus && enableHotkeysInFocus) || (!inFocus && enableHotkeysOutOfFocus));
}

void OBSApp::UpdateHotkeyFocusSetting(bool resetState)
{
	enableHotkeysInFocus = true;
	enableHotkeysOutOfFocus = true;

	const char *hotkeyFocusType = config_get_string(userConfig, "General", "HotkeyFocusType");

	if (astrcmpi(hotkeyFocusType, "DisableHotkeysInFocus") == 0) {
		enableHotkeysInFocus = false;
	} else if (astrcmpi(hotkeyFocusType, "DisableHotkeysOutOfFocus") == 0) {
		enableHotkeysOutOfFocus = false;
	}

	if (resetState)
		ResetHotkeyState(applicationState() == Qt::ApplicationActive);
}

void OBSApp::DisableHotkeys()
{
	enableHotkeysInFocus = false;
	enableHotkeysOutOfFocus = false;
	ResetHotkeyState(applicationState() == Qt::ApplicationActive);
}

void OBSApp::Exec(VoidFunc func)
{
	func();
}

static void ui_task_handler(obs_task_t task, void *param, bool wait)
{
	auto doTask = [=]() {
		/* to get clang-format to behave */
		task(param);
	};
	QMetaObject::invokeMethod(App(), "Exec", wait ? WaitConnection() : Qt::AutoConnection, Q_ARG(VoidFunc, doTask));
}

bool OBSApp::OBSInit()
{
	ProfileScope("OBSApp::OBSInit");

	qRegisterMetaType<VoidFunc>("VoidFunc");

#if !defined(_WIN32) && !defined(__APPLE__)
	if (QApplication::platformName() == "xcb") {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
		auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();

		obs_set_nix_platform_display(native->display());
#endif

		obs_set_nix_platform(OBS_NIX_PLATFORM_X11_EGL);

		blog(LOG_INFO, "Using EGL/X11");
	}

#ifdef ENABLE_WAYLAND
	if (QApplication::platformName().contains("wayland")) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
		auto native = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();

		obs_set_nix_platform_display(native->display());
#endif

		obs_set_nix_platform(OBS_NIX_PLATFORM_WAYLAND);
		setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

		blog(LOG_INFO, "Platform: Wayland");
	}
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
	QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
	obs_set_nix_platform_display(native->nativeResourceForIntegration("display"));
#endif
#endif

#ifdef __APPLE__
	setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif

	if (!StartupOBS(locale.c_str(), GetProfilerNameStore()))
		return false;

	libobs_initialized = true;

	// 이전 obs_reset_video 호출을 로드된 설정으로 채워진 obs_video_info 구조체를 사용하여 OBSBasic::ResetVideoLogic()과 같은 새로운 함수로 옮기거나 래핑하는 것을 고려해 보세요.
	// OBSBasic::Init()에서 해당 정보를 로드한 후 OBSBasic::ResetVideoLogic()을 호출할 수 있습니다.
	// 지금은 App()->horizontal_ovi 및 App()->vertical_ovi가 설정 UI에서 로드된 값으로 채워졌다고 가정합니다.

	if (dualOutputActive) {
		blog(LOG_INFO, "Dual output mode active. Initializing both video pipelines.");
		// 여기에 libobs에 두 개의 비디오 파이프라인을 설정하도록 지시하는 가상 코드가 들어갑니다.
		// 예: obs_reset_video_dual(&horizontal_ovi, &vertical_ovi);
		// 또는 obs_graphics_context_create_secondary() 같은 것 다음 obs_reset_video()를 두 번 호출합니다.
		// 이 부분은 libobs의 상당한 변경이 필요합니다.
		// 임시로, 기본 파이프라인만 초기화될 것이라고 가정하고 로그를 남깁니다.
		blog(LOG_INFO, "Conceptual: obs_reset_video_dual(&horizontal_ovi, &vertical_ovi) called here.");
		// 실제로는 OBSBasic::ResetVideo() 또는 obs_reset_video()를 horizontal_ovi로 호출하는 로직이 여기에 오거나 아래 공통 경로로 이동합니다.
		// 지금은 핵심 로직이 OBSBasic::ResetVideo()에 있다고 가정하고, 해당 함수가 dualOutputActive를 확인해야 합니다.
	} else {
		blog(LOG_INFO, "Single output mode active. Initializing primary video pipeline.");
		// 기존 로직: obs_reset_video(&horizontal_ovi);
		// 이 또한 OBSBasic::ResetVideo()로 옮겨질 수 있습니다.
		blog(LOG_INFO, "Conceptual: obs_reset_video(&horizontal_ovi) called here.");
	}
	// obs_reset_video()에 대한 실제 호출은 OBSBasic::LoadConfig() 또는 유사한 함수에서 수행될 가능성이 높습니다.
	// OVI 구조체가 채워진 후입니다. OBSApp::OBSInit()는 너무 이를 수 있습니다.
	// 이 단계에서는 dualOutputActive 플래그를 설정하고 로깅하는 데 중점을 둡니다.
	// 실제 비디오 재설정은 OBSBasic에서 처리되어야 합니다.

	obs_set_ui_task_handler(ui_task_handler);

#if defined(_WIN32) || defined(__APPLE__) || defined(__linux__)
	bool browserHWAccel = config_get_bool(appConfig, "General", "BrowserHWAccel");

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_bool(settings, "BrowserHWAccel", browserHWAccel);
	obs_apply_private_data(settings);

	blog(LOG_INFO, "Current Date/Time: %s", CurrentDateTimeString().c_str());

	blog(LOG_INFO, "Browser Hardware Acceleration: %s", browserHWAccel ? "true" : "false");
#endif
#ifdef _WIN32
	bool hideFromCapture = config_get_bool(userConfig, "BasicWindow", "HideOBSWindowsFromCapture");
	blog(LOG_INFO, "Hide OBS windows from screen capture: %s", hideFromCapture ? "true" : "false");
#endif

	blog(LOG_INFO, "Qt Version: %s (runtime), %s (compiled)", qVersion(), QT_VERSION_STR);
	blog(LOG_INFO, "Portable mode: %s", portable_mode ? "true" : "false");

	if (safe_mode) {
		blog(LOG_WARNING, "Safe Mode enabled.");
	} else if (disable_3p_plugins) {
		blog(LOG_WARNING, "Third-party plugins disabled.");
	}

	setQuitOnLastWindowClosed(false);

	mainWindow = new OBSBasic();

	mainWindow->setAttribute(Qt::WA_DeleteOnClose, true);
	connect(mainWindow, &OBSBasic::destroyed, this, &OBSApp::quit);

	mainWindow->OBSInit();

	connect(this, &QGuiApplication::applicationStateChanged,
		[this](Qt::ApplicationState state) { ResetHotkeyState(state == Qt::ApplicationActive); });
	ResetHotkeyState(applicationState() == Qt::ApplicationActive);
	return true;
}

string OBSApp::GetVersionString(bool platform) const
{
	stringstream ver;

	ver << obs_get_version_string();

	if (platform) {
		ver << " (";
#ifdef _WIN32
		if (sizeof(void *) == 8)
			ver << "64-bit, ";
		else
			ver << "32-bit, ";

		ver << "windows)";
#elif __APPLE__
		ver << "mac)";
#elif __OpenBSD__
		ver << "openbsd)";
#elif __FreeBSD__
		ver << "freebsd)";
#else /* assume linux for the time being */
		ver << "linux)";
#endif
	}

	return ver.str();
}

bool OBSApp::IsPortableMode()
{
	return portable_mode;
}

bool OBSApp::IsUpdaterDisabled()
{
	return opt_disable_updater;
}

bool OBSApp::IsMissingFilesCheckDisabled()
{
	return opt_disable_missing_files_check;
}

#ifdef __APPLE__
#define INPUT_AUDIO_SOURCE "coreaudio_input_capture"
#define OUTPUT_AUDIO_SOURCE "coreaudio_output_capture"
#elif _WIN32
#define INPUT_AUDIO_SOURCE "wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE "wasapi_output_capture"
#else
#define INPUT_AUDIO_SOURCE "pulse_input_capture"
#define OUTPUT_AUDIO_SOURCE "pulse_output_capture"
#endif

const char *OBSApp::InputAudioSource() const
{
	return INPUT_AUDIO_SOURCE;
}

const char *OBSApp::OutputAudioSource() const
{
	return OUTPUT_AUDIO_SOURCE;
}

const char *OBSApp::GetLastLog() const
{
	return lastLogFile.c_str();
}

const char *OBSApp::GetCurrentLog() const
{
	return currentLogFile.c_str();
}

const char *OBSApp::GetLastCrashLog() const
{
	return lastCrashLogFile.c_str();
}

bool OBSApp::TranslateString(const char *lookupVal, const char **out) const
{
	for (obs_frontend_translate_ui_cb cb : translatorHooks) {
		if (cb(lookupVal, out))
			return true;
	}

	return text_lookup_getstr(App()->GetTextLookup(), lookupVal, out);
}

// Global handler to receive all QEvent::Show events so we can apply
// display affinity on any newly created windows and dialogs without
// caring where they are coming from (e.g. plugins).
bool OBSApp::notify(QObject *receiver, QEvent *e)
{
	QWidget *w;
	QWindow *window;
	int windowType;

	if (!receiver->isWidgetType())
		goto skip;

	if (e->type() != QEvent::Show)
		goto skip;

	w = qobject_cast<QWidget *>(receiver);

	if (!w->isWindow())
		goto skip;

	window = w->windowHandle();
	if (!window)
		goto skip;

	windowType = window->flags() & Qt::WindowType::WindowType_Mask;

	if (windowType == Qt::WindowType::Dialog || windowType == Qt::WindowType::Window ||
	    windowType == Qt::WindowType::Tool) {
		OBSBasic *main = OBSBasic::Get();
		if (main)
			main->SetDisplayAffinity(window);
	}

skip:
	return QApplication::notify(receiver, e);
}

string GenerateTimeDateFilename(const char *extension, bool noSpace)
{
	time_t now = time(0);
	char file[256] = {};
	struct tm *cur_time;

	cur_time = localtime(&now);
	snprintf(file, sizeof(file), "%d-%02d-%02d%c%02d-%02d-%02d.%s", cur_time->tm_year + 1900, cur_time->tm_mon + 1,
		 cur_time->tm_mday, noSpace ? '_' : ' ', cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec,
		 extension);

	return string(file);
}

string GenerateSpecifiedFilename(const char *extension, bool noSpace, const char *format)
{
	BPtr<char> filename = os_generate_formatted_filename(extension, !noSpace, format);
	return string(filename);
}

static void FindBestFilename(string &strPath, bool noSpace)
{
	int num = 2;

	if (!os_file_exists(strPath.c_str()))
		return;

	const char *ext = strrchr(strPath.c_str(), '.');
	if (!ext)
		return;

	int extStart = int(ext - strPath.c_str());
	for (;;) {
		string testPath = strPath;
		string numStr;

		numStr = noSpace ? "_" : " (";
		numStr += to_string(num++);
		if (!noSpace)
			numStr += ")";

		testPath.insert(extStart, numStr);

		if (!os_file_exists(testPath.c_str())) {
			strPath = testPath;
			break;
		}
	}
}

static void ensure_directory_exists(string &path)
{
	replace(path.begin(), path.end(), '\\', '/');

	size_t last = path.rfind('/');
	if (last == string::npos)
		return;

	string directory = path.substr(0, last);
	os_mkdirs(directory.c_str());
}

static void remove_reserved_file_characters(string &s)
{
	replace(s.begin(), s.end(), '\\', '/');
	replace(s.begin(), s.end(), '*', '_');
	replace(s.begin(), s.end(), '?', '_');
	replace(s.begin(), s.end(), '"', '_');
	replace(s.begin(), s.end(), '|', '_');
	replace(s.begin(), s.end(), ':', '_');
	replace(s.begin(), s.end(), '>', '_');
	replace(s.begin(), s.end(), '<', '_');
}

string GetFormatString(const char *format, const char *prefix, const char *suffix)
{
	string f;

	f = format;

	if (prefix && *prefix) {
		string str_prefix = prefix;

		if (str_prefix.back() != ' ')
			str_prefix += " ";

		size_t insert_pos = 0;
		size_t tmp;

		tmp = f.find_last_of('/');
		if (tmp != string::npos && tmp > insert_pos)
			insert_pos = tmp + 1;

		tmp = f.find_last_of('\\');
		if (tmp != string::npos && tmp > insert_pos)
			insert_pos = tmp + 1;

		f.insert(insert_pos, str_prefix);
	}

	if (suffix && *suffix) {
		if (*suffix != ' ')
			f += " ";
		f += suffix;
	}

	remove_reserved_file_characters(f);

	return f;
}

string GetFormatExt(const char *container)
{
	string ext = container;
	if (ext == "fragmented_mp4")
		ext = "mp4";
	if (ext == "hybrid_mp4")
		ext = "mp4";
	else if (ext == "fragmented_mov")
		ext = "mov";
	else if (ext == "hls")
		ext = "m3u8";
	else if (ext == "mpegts")
		ext = "ts";

	return ext;
}

string GetOutputFilename(const char *path, const char *container, bool noSpace, bool overwrite, const char *format)
{
	OBSBasic *main = OBSBasic::Get();

	os_dir_t *dir = path && path[0] ? os_opendir(path) : nullptr;

	if (!dir) {
		if (main->isVisible())
			OBSMessageBox::warning(main, QTStr("Output.BadPath.Title"), QTStr("Output.BadPath.Text"));
		else
			main->SysTrayNotify(QTStr("Output.BadPath.Text"), QSystemTrayIcon::Warning);
		return "";
	}

	os_closedir(dir);

	string strPath;
	strPath += path;

	char lastChar = strPath.back();
	if (lastChar != '/' && lastChar != '\\')
		strPath += "/";

	string ext = GetFormatExt(container);
	strPath += GenerateSpecifiedFilename(ext.c_str(), noSpace, format);
	ensure_directory_exists(strPath);
	if (!overwrite)
		FindBestFilename(strPath, noSpace);

	return strPath;
}

vector<pair<string, string>> GetLocaleNames()
{
	string path;
	if (!GetDataFilePath("locale.ini", path))
		throw "Could not find locale.ini path";

	ConfigFile ini;
	if (ini.Open(path.c_str(), CONFIG_OPEN_EXISTING) != 0)
		throw "Could not open locale.ini";

	size_t sections = config_num_sections(ini);

	vector<pair<string, string>> names;
	names.reserve(sections);
	for (size_t i = 0; i < sections; i++) {
		const char *tag = config_get_section(ini, i);
		const char *name = config_get_string(ini, tag, "Name");
		names.emplace_back(tag, name);
	}

	return names;
}

#if defined(__APPLE__) || defined(__linux__)
#define BASE_PATH ".."
#else
#define BASE_PATH "../.."
#endif

#define CONFIG_PATH BASE_PATH "/config"

#if defined(ENABLE_PORTABLE_CONFIG) || defined(_WIN32)
#define ALLOW_PORTABLE_MODE 1
#else
#define ALLOW_PORTABLE_MODE 0
#endif

int GetAppConfigPath(char *path, size_t size, const char *name)
{
#if ALLOW_PORTABLE_MODE
	if (portable_mode) {
		if (name && *name) {
			return snprintf(path, size, CONFIG_PATH "/%s", name);
		} else {
			return snprintf(path, size, CONFIG_PATH);
		}
	} else {
		return os_get_config_path(path, size, name);
	}
#else
	return os_get_config_path(path, size, name);
#endif
}

char *GetAppConfigPathPtr(const char *name)
{
#if ALLOW_PORTABLE_MODE
	if (portable_mode) {
		char path[512];

		if (snprintf(path, sizeof(path), CONFIG_PATH "/%s", name) > 0) {
			return bstrdup(path);
		} else {
			return NULL;
		}
	} else {
		return os_get_config_path_ptr(name);
	}
#else
	return os_get_config_path_ptr(name);
#endif
}

int GetProgramDataPath(char *path, size_t size, const char *name)
{
	return os_get_program_data_path(path, size, name);
}

char *GetProgramDataPathPtr(const char *name)
{
	return os_get_program_data_path_ptr(name);
}

bool GetFileSafeName(const char *name, std::string &file)
{
	size_t base_len = strlen(name);
	size_t len = os_utf8_to_wcs(name, base_len, nullptr, 0);
	std::wstring wfile;

	if (!len)
		return false;

	wfile.resize(len);
	os_utf8_to_wcs(name, base_len, &wfile[0], len + 1);

	for (size_t i = wfile.size(); i > 0; i--) {
		size_t im1 = i - 1;

		if (iswspace(wfile[im1])) {
			wfile[im1] = '_';
		} else if (wfile[im1] != '_' && !iswalnum(wfile[im1])) {
			wfile.erase(im1, 1);
		}
	}

	if (wfile.size() == 0)
		wfile = L"characters_only";

	len = os_wcs_to_utf8(wfile.c_str(), wfile.size(), nullptr, 0);
	if (!len)
		return false;

	file.resize(len);
	os_wcs_to_utf8(wfile.c_str(), wfile.size(), &file[0], len + 1);
	return true;
}

bool GetClosestUnusedFileName(std::string &path, const char *extension)
{
	size_t len = path.size();
	if (extension) {
		path += ".";
		path += extension;
	}

	if (!os_file_exists(path.c_str()))
		return true;

	int index = 1;

	do {
		path.resize(len);
		path += std::to_string(++index);
		if (extension) {
			path += ".";
			path += extension;
		}
	} while (os_file_exists(path.c_str()));

	return true;
}

bool WindowPositionValid(QRect rect)
{
	for (QScreen *screen : QGuiApplication::screens()) {
		if (screen->availableGeometry().intersects(rect))
			return true;
	}
	return false;
}

#ifndef _WIN32
void OBSApp::SigIntSignalHandler(int s)
{
	/* Handles SIGINT and writes to a socket. Qt will read
	 * from the socket in the main thread event loop and trigger
	 * a call to the ProcessSigInt slot, where we can safely run
	 * shutdown code without signal safety issues. */
	UNUSED_PARAMETER(s);

	char a = 1;
	send(sigintFd[0], &a, sizeof(a), 0);
}
#endif

void OBSApp::ProcessSigInt(void)
{
	/* This looks weird, but we can't ifdef a Qt slot function so
	 * the SIGINT handler simply does nothing on Windows. */
#ifndef _WIN32
	char tmp;
	recv(sigintFd[1], &tmp, sizeof(tmp), 0);

	OBSBasic *main = OBSBasic::Get();
	if (main)
		main->close();
#endif
}

#ifdef _WIN32
void OBSApp::commitData(QSessionManager &manager)
{
	if (auto main = App()->GetMainWindow()) {
		QMetaObject::invokeMethod(main, "close", Qt::QueuedConnection);
		manager.cancel();
	}
}
#endif
