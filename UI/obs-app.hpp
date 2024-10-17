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

#include <QApplication>
#include <QTranslator>
#include <QPointer>
#include <QFileSystemWatcher>

#ifndef _WIN32
#include <QSocketNotifier>
#else
#include <QSessionManager>
#endif
#include <obs.hpp>
#include <util/lexer.h>
#include <util/profiler.h>
#include <util/util.hpp>
#include <util/platform.h>
#include <obs-frontend-api.h>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <filesystem>

#include "window-main.hpp"
#include "obs-app-theming.hpp"

std::string CurrentTimeString();
std::string CurrentDateTimeString();
std::string GenerateTimeDateFilename(const char *extension, bool noSpace = false);
std::string GenerateSpecifiedFilename(const char *extension, bool noSpace, const char *format);
std::string GetFormatString(const char *format, const char *prefix, const char *suffix);
std::string GetFormatExt(const char *container);
std::string GetOutputFilename(const char *path, const char *container, bool noSpace, bool overwrite,
			      const char *format);
QObject *CreateShortcutFilter();

struct BaseLexer {
	lexer lex;

public:
	inline BaseLexer() { lexer_init(&lex); }
	inline ~BaseLexer() { lexer_free(&lex); }
	operator lexer *() { return &lex; }
};

class OBSTranslator : public QTranslator {
	Q_OBJECT

public:
	virtual bool isEmpty() const override { return false; }

	virtual QString translate(const char *context, const char *sourceText, const char *disambiguation,
				  int n) const override;
};

typedef std::function<void()> VoidFunc;

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
	void themeFileChanged(const QString &);

public:
	OBSApp(int &argc, char **argv, profiler_name_store_t *store);
	~OBSApp();

	void AppInit();
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

	inline const char *GetLocale() const { return locale.c_str(); }

	OBSTheme *GetTheme() const { return currentTheme; }
	QList<OBSTheme> GetThemes() const { return themes.values(); }
	OBSTheme *GetTheme(const QString &name);
	bool SetTheme(const QString &name);
	bool IsThemeDark() const { return currentTheme ? currentTheme->isDark : false; }

	void SetBranchData(const std::string &data);
	std::vector<UpdateBranch> GetBranches();

	inline lookup_t *GetTextLookup() const { return textLookup; }

	inline const char *GetString(const char *lookupVal) const { return textLookup.GetString(lookupVal); }

	bool TranslateString(const char *lookupVal, const char **out) const;

	profiler_name_store_t *GetProfilerNameStore() const { return profilerNameStore; }

	const char *GetLastLog() const;
	const char *GetCurrentLog() const;

	const char *GetLastCrashLog() const;

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

public slots:
	void Exec(VoidFunc func);
	void ProcessSigInt();

signals:
	void StyleChanged();
};

int GetAppConfigPath(char *path, size_t size, const char *name);
char *GetAppConfigPathPtr(const char *name);

int GetProgramDataPath(char *path, size_t size, const char *name);
char *GetProgramDataPathPtr(const char *name);

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

bool GetFileSafeName(const char *name, std::string &file);
bool GetClosestUnusedFileName(std::string &path, const char *extension);
bool GetUnusedSceneCollectionFile(std::string &name, std::string &file);

bool WindowPositionValid(QRect rect);

extern bool portable_mode;
extern bool steam;
extern bool safe_mode;
extern bool disable_3p_plugins;

extern bool opt_start_streaming;
extern bool opt_start_recording;
extern bool opt_start_replaybuffer;
extern bool opt_start_virtualcam;
extern bool opt_minimize_tray;
extern bool opt_studio_mode;
extern bool opt_allow_opengl;
extern bool opt_always_on_top;
extern std::string opt_starting_scene;
extern bool restart;
extern bool restart_safe;

#ifdef _WIN32
extern "C" void install_dll_blocklist_hook(void);
extern "C" void log_blocked_dlls(void);
#endif
