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

#include <utility/OBSTheme.hpp>
#include <widgets/OBSMainWindow.hpp>

#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/profiler.hpp>
#include <util/util.hpp>

#include <QApplication>
#include <QPalette>
#include <QPointer>
#include <QUuid>

#include <deque>
#include <functional>
#include <string>
#include <vector>

typedef std::function<void()> VoidFunc;

Q_DECLARE_METATYPE(VoidFunc)

class QFileSystemWatcher;
class QSocketNotifier;

namespace OBS {
class CrashHandler;

enum class LogFileType { NoType, CurrentAppLog, LastAppLog, CrashLog };
enum class LogFileState { NoState, New, Uploaded };
class PluginManager;
} // namespace OBS

struct UpdateBranch {
	QString name;
	QString display_name;
	QString description;
	bool is_enabled;
	bool is_visible;
};

class OBSApp : public QApplication {
	Q_OBJECT

private:
	QUuid appLaunchUUID_;
	std::unique_ptr<OBS::CrashHandler> crashHandler_;

	std::string locale;

	ConfigFile appConfig;
	ConfigFile userConfig;
	TextLookup textLookup;
	QPointer<OBSMainWindow> mainWindow;
	profiler_name_store_t *profilerNameStore = nullptr;
	std::vector<UpdateBranch> updateBranches;
	bool branches_loaded = false;

	bool libobs_initialized = false;

	os_inhibit_t *sleepInhibitor = nullptr;
	int sleepInhibitRefs = 0;

	bool enableHotkeysInFocus = true;
	bool enableHotkeysOutOfFocus = true;

	std::deque<obs_frontend_translate_ui_cb> translatorHooks;

	std::unique_ptr<OBS::PluginManager> pluginManager_;

	bool UpdatePre22MultiviewLayout(const char *layout);

	bool InitGlobalConfig();
	bool InitGlobalConfigDefaults();
	bool InitGlobalLocationDefaults();

	bool MigrateGlobalSettings();
	void MigrateLegacySettings(uint32_t lastVersion);

	bool InitUserConfig(std::filesystem::path &userConfigLocation, uint32_t lastVersion);
	void InitUserConfigDefaults();

	bool InitLocale();
	bool InitTheme();

	inline void ResetHotkeyState(bool inFocus);

	QPalette defaultPalette;
	OBSTheme *currentTheme = nullptr;
	QHash<QString, OBSTheme> themes;
	QPointer<QFileSystemWatcher> themeWatcher;
	std::unique_ptr<QStyle> invisibleCursorStyle;

	void FindThemes();

	bool notify(QObject *receiver, QEvent *e) override;

#ifndef _WIN32
	static int sigintFd[2];
	QSocketNotifier *snInt = nullptr;
#else
private slots:
	void commitData(QSessionManager &manager);
#endif

private slots:
	void addLogLine(int logLevel, const QString &message);
	void themeFileChanged(const QString &);
	void applicationShutdown() noexcept;

public:
	OBSApp(int &argc, char **argv, profiler_name_store_t *store);
	~OBSApp();

	void AppInit();
	void checkForUncleanShutdown();
	bool OBSInit();

	void UpdateHotkeyFocusSetting(bool reset = true);
	void DisableHotkeys();

	inline bool HotkeysEnabledInFocus() const { return enableHotkeysInFocus; }

	inline QMainWindow *GetMainWindow() const { return mainWindow.data(); }

	inline config_t *GetAppConfig() const { return appConfig; }
	inline config_t *GetUserConfig() const { return userConfig; }
	std::filesystem::path userConfigLocation;
	std::filesystem::path userScenesLocation;
	std::filesystem::path userProfilesLocation;
	std::filesystem::path userPluginManagerSettingsLocation;

	inline const char *GetLocale() const { return locale.c_str(); }

	OBSTheme *GetTheme() const { return currentTheme; }
	QList<OBSTheme> GetThemes() const { return themes.values(); }
	OBSTheme *GetTheme(const QString &name);
	bool SetTheme(const QString &name);
	bool IsThemeDark() const { return currentTheme ? currentTheme->isDark : false; }
	QStyle *GetInvisibleCursorStyle();

	void SetBranchData(const std::string &data);
	std::vector<UpdateBranch> GetBranches();

	inline lookup_t *GetTextLookup() const { return textLookup; }

	inline const char *GetString(const char *lookupVal) const { return textLookup.GetString(lookupVal); }

	bool TranslateString(const char *lookupVal, const char **out) const;

	profiler_name_store_t *GetProfilerNameStore() const { return profilerNameStore; }

	const char *GetLastLog() const;
	const char *GetCurrentLog() const;

	void openCrashLogDirectory() const;
	void uploadLastAppLog() const;
	void uploadCurrentAppLog() const;
	void uploadLastCrashLog();

	OBS::LogFileState getLogFileState(OBS::LogFileType type) const;

	std::string GetVersionString(bool platform = true) const;
	bool IsPortableMode();
	bool IsUpdaterDisabled();
	bool IsMissingFilesCheckDisabled();

	const char *InputAudioSource() const;
	const char *OutputAudioSource() const;

	const char *GetRenderModule() const;

	inline void IncrementSleepInhibition()
	{
		if (!sleepInhibitor)
			return;
		if (sleepInhibitRefs++ == 0)
			os_inhibit_sleep_set_active(sleepInhibitor, true);
	}

	inline void DecrementSleepInhibition()
	{
		if (!sleepInhibitor)
			return;
		if (sleepInhibitRefs == 0)
			return;
		if (--sleepInhibitRefs == 0)
			os_inhibit_sleep_set_active(sleepInhibitor, false);
	}

	inline void PushUITranslation(obs_frontend_translate_ui_cb cb) { translatorHooks.emplace_front(cb); }

	inline void PopUITranslation() { translatorHooks.pop_front(); }
#ifndef _WIN32
	static void SigIntSignalHandler(int);
#endif

	void loadAppModules(struct obs_module_failure_info &mfi);

	// Plugin Manager Accessors
	void pluginManagerOpenDialog();

public slots:
	void Exec(VoidFunc func);
	void ProcessSigInt();

signals:
	void logLineAdded(int logLevel, const QString &message);
	void StyleChanged();

	void logUploadFinished(OBS::LogFileType, const QString &fileUrl);
	void logUploadFailed(OBS::LogFileType, const QString &errorMessage);
};

int GetAppConfigPath(char *path, size_t size, const char *name);
char *GetAppConfigPathPtr(const char *name);

inline OBSApp *App()
{
	return static_cast<OBSApp *>(qApp);
}

std::vector<std::pair<std::string, std::string>> GetLocaleNames();
inline const char *Str(const char *lookup)
{
	return App()->GetString(lookup);
}
inline QString QTStr(const char *lookupVal)
{
	return QString::fromUtf8(Str(lookupVal));
}

int GetProgramDataPath(char *path, size_t size, const char *name);
char *GetProgramDataPathPtr(const char *name);

bool GetFileSafeName(const char *name, std::string &file);
bool GetClosestUnusedFileName(std::string &path, const char *extension);

bool WindowPositionValid(QRect rect);

#ifdef _WIN32
extern "C" void install_dll_blocklist_hook(void);
extern "C" void log_blocked_dlls(void);
#endif

std::string CurrentDateTimeString();
std::string GetFormatString(const char *format, const char *prefix, const char *suffix);
std::string GenerateTimeDateFilename(const char *extension, bool noSpace = false);
std::string GetFormatExt(const char *container);
std::string GetOutputFilename(const char *path, const char *container, bool noSpace, bool overwrite,
			      const char *format);
QObject *CreateShortcutFilter();
