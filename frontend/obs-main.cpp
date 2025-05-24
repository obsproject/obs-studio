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

#include <OBSApp.hpp>
#ifdef __APPLE__
#include <dialogs/OBSPermissions.hpp>
#endif
#include <utility/BaseLexer.hpp>
#include <utility/OBSTranslator.hpp>
#include <utility/platform.hpp>
#include <widgets/VolumeAccessibleInterface.hpp>

#include <qt-wrappers.hpp>
#include <util/platform.h>
#include <util/profiler.h>
#include <util/util.hpp>
#ifdef _WIN32
#include <util/windows/win-version.h>
#endif

#include <QFontDatabase>
#include <QProcess>
#include <QPushButton>
#include <curl/curl.h>

#include <fstream>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <shellapi.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#endif

using namespace std;

static log_handler_t def_log_handler;

extern string currentLogFile;
extern string lastLogFile;

bool portable_mode = false;
bool steam = false;
bool safe_mode = false;
bool disable_3p_plugins = false;
static bool unclean_shutdown = false;
bool multi = false;
static bool log_verbose = false;
static bool unfiltered_log = false;
bool opt_start_streaming = false;
bool opt_start_recording = false;
bool opt_studio_mode = false;
bool opt_start_replaybuffer = false;
bool opt_start_virtualcam = false;
bool opt_minimize_tray = false;
bool opt_allow_opengl = false;
bool opt_always_on_top = false;
bool opt_disable_updater = false;
bool opt_disable_missing_files_check = false;
string opt_starting_collection;
string opt_starting_profile;
string opt_starting_scene;

bool restart = false;
bool restart_safe = false;
static QStringList arguments;

string CurrentTimeString()
{
	using namespace std::chrono;

	struct tm tstruct;
	char buf[80];

	auto tp = system_clock::now();
	auto now = system_clock::to_time_t(tp);
	tstruct = *localtime(&now);

	size_t written = strftime(buf, sizeof(buf), "%T", &tstruct);
	if (ratio_less<system_clock::period, seconds::period>::value && written && (sizeof(buf) - written) > 5) {
		auto tp_secs = time_point_cast<seconds>(tp);
		auto millis = duration_cast<milliseconds>(tp - tp_secs).count();

		snprintf(buf + written, sizeof(buf) - written, ".%03u", static_cast<unsigned>(millis));
	}

	return buf;
}

static void LogString(fstream &logFile, const char *timeString, char *str, int log_level)
{
	static mutex logfile_mutex;
	string msg;
	msg += timeString;
	msg += str;

	logfile_mutex.lock();
	logFile << msg << endl;
	logfile_mutex.unlock();

	QMetaObject::invokeMethod(App(), "addLogLine", Qt::QueuedConnection, Q_ARG(int, log_level),
				  Q_ARG(QString, QString(msg.c_str())));
}

static inline void LogStringChunk(fstream &logFile, char *str, int log_level)
{
	char *nextLine = str;
	string timeString = CurrentTimeString();
	timeString += ": ";

	while (*nextLine) {
		char *nextLine = strchr(str, '\n');
		if (!nextLine)
			break;

		if (nextLine != str && nextLine[-1] == '\r') {
			nextLine[-1] = 0;
		} else {
			nextLine[0] = 0;
		}

		LogString(logFile, timeString.c_str(), str, log_level);
		nextLine++;
		str = nextLine;
	}

	LogString(logFile, timeString.c_str(), str, log_level);
}

#define MAX_REPEATED_LINES 30
#define MAX_CHAR_VARIATION (255 * 3)

static inline int sum_chars(const char *str)
{
	int val = 0;
	for (; *str != 0; str++)
		val += *str;

	return val;
}

static inline bool too_many_repeated_entries(fstream &logFile, const char *msg, const char *output_str)
{
	static mutex log_mutex;
	static const char *last_msg_ptr = nullptr;
	static int last_char_sum = 0;
	static int rep_count = 0;

	int new_sum = sum_chars(output_str);

	lock_guard<mutex> guard(log_mutex);

	if (unfiltered_log) {
		return false;
	}

	if (last_msg_ptr == msg) {
		int diff = std::abs(new_sum - last_char_sum);
		if (diff < MAX_CHAR_VARIATION) {
			return (rep_count++ >= MAX_REPEATED_LINES);
		}
	}

	if (rep_count > MAX_REPEATED_LINES) {
		logFile << CurrentTimeString() << ": Last log entry repeated for "
			<< to_string(rep_count - MAX_REPEATED_LINES) << " more lines" << endl;
	}

	last_msg_ptr = msg;
	last_char_sum = new_sum;
	rep_count = 0;

	return false;
}

static void do_log(int log_level, const char *msg, va_list args, void *param)
{
	fstream &logFile = *static_cast<fstream *>(param);
	char str[8192];

#ifndef _WIN32
	va_list args2;
	va_copy(args2, args);
#endif

	vsnprintf(str, sizeof(str), msg, args);

#ifdef _WIN32
	if (IsDebuggerPresent()) {
		int wNum = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
		if (wNum > 1) {
			static wstring wide_buf;
			static mutex wide_mutex;

			lock_guard<mutex> lock(wide_mutex);
			wide_buf.reserve(wNum + 1);
			wide_buf.resize(wNum - 1);
			MultiByteToWideChar(CP_UTF8, 0, str, -1, &wide_buf[0], wNum);
			wide_buf.push_back('\n');

			OutputDebugStringW(wide_buf.c_str());
		}
	}
#endif

#if !defined(_WIN32) && defined(_DEBUG)
	def_log_handler(log_level, msg, args2, nullptr);
#endif

	if (log_level <= LOG_INFO || log_verbose) {
#if !defined(_WIN32) && !defined(_DEBUG)
		def_log_handler(log_level, msg, args2, nullptr);
#endif
		if (!too_many_repeated_entries(logFile, msg, str))
			LogStringChunk(logFile, str, log_level);
	}

#if defined(_WIN32) && defined(OBS_DEBUGBREAK_ON_ERROR)
	if (log_level <= LOG_ERROR && IsDebuggerPresent())
		__debugbreak();
#endif

#ifndef _WIN32
	va_end(args2);
#endif
}

static bool get_token(lexer *lex, string &str, base_token_type type)
{
	base_token token;
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != type)
		return false;

	str.assign(token.text.array, token.text.len);
	return true;
}

static bool expect_token(lexer *lex, const char *str, base_token_type type)
{
	base_token token;
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != type)
		return false;

	return strref_cmp(&token.text, str) == 0;
}

static uint64_t convert_log_name(bool has_prefix, const char *name)
{
	BaseLexer lex;
	string year, month, day, hour, minute, second;

	lexer_start(lex, name);

	if (has_prefix) {
		string temp;
		if (!get_token(lex, temp, BASETOKEN_ALPHA))
			return 0;
	}

	if (!get_token(lex, year, BASETOKEN_DIGIT))
		return 0;
	if (!expect_token(lex, "-", BASETOKEN_OTHER))
		return 0;
	if (!get_token(lex, month, BASETOKEN_DIGIT))
		return 0;
	if (!expect_token(lex, "-", BASETOKEN_OTHER))
		return 0;
	if (!get_token(lex, day, BASETOKEN_DIGIT))
		return 0;
	if (!get_token(lex, hour, BASETOKEN_DIGIT))
		return 0;
	if (!expect_token(lex, "-", BASETOKEN_OTHER))
		return 0;
	if (!get_token(lex, minute, BASETOKEN_DIGIT))
		return 0;
	if (!expect_token(lex, "-", BASETOKEN_OTHER))
		return 0;
	if (!get_token(lex, second, BASETOKEN_DIGIT))
		return 0;

	stringstream timestring;
	timestring << year << month << day << hour << minute << second;
	return std::stoull(timestring.str());
}

/* If upgrading from an older (non-XDG) build of OBS, move config files to XDG directory. */
/* TODO: Remove after version 32.0. */
#if defined(__FreeBSD__)
static void move_to_xdg(void)
{
	char old_path[512];
	char new_path[512];
	char *home = getenv("HOME");
	if (!home)
		return;

	if (snprintf(old_path, sizeof(old_path), "%s/.obs-studio", home) <= 0)
		return;

	/* make base xdg path if it doesn't already exist */
	if (GetAppConfigPath(new_path, sizeof(new_path), "") <= 0)
		return;
	if (os_mkdirs(new_path) == MKDIR_ERROR)
		return;

	if (GetAppConfigPath(new_path, sizeof(new_path), "obs-studio") <= 0)
		return;

	if (os_file_exists(old_path) && !os_file_exists(new_path)) {
		rename(old_path, new_path);
	}
}
#endif

static void delete_oldest_file(bool has_prefix, const char *location)
{
	BPtr<char> logDir(GetAppConfigPathPtr(location));
	string oldestLog;
	uint64_t oldest_ts = (uint64_t)-1;
	struct os_dirent *entry;

	unsigned int maxLogs = (unsigned int)config_get_uint(App()->GetAppConfig(), "General", "MaxLogs");

	os_dir_t *dir = os_opendir(logDir);
	if (dir) {
		unsigned int count = 0;

		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.')
				continue;

			uint64_t ts = convert_log_name(has_prefix, entry->d_name);

			if (ts) {
				if (ts < oldest_ts) {
					oldestLog = entry->d_name;
					oldest_ts = ts;
				}

				count++;
			}
		}

		os_closedir(dir);

		if (count > maxLogs) {
			stringstream delPath;

			delPath << logDir << "/" << oldestLog;
			os_unlink(delPath.str().c_str());
		}
	}
}

static void get_last_log(bool has_prefix, const char *subdir_to_use, std::string &last)
{
	BPtr<char> logDir(GetAppConfigPathPtr(subdir_to_use));
	struct os_dirent *entry;
	os_dir_t *dir = os_opendir(logDir);
	uint64_t highest_ts = 0;

	if (dir) {
		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.')
				continue;

			uint64_t ts = convert_log_name(has_prefix, entry->d_name);

			if (ts > highest_ts) {
				last = entry->d_name;
				highest_ts = ts;
			}
		}

		os_closedir(dir);
	}
}

static void create_log_file(fstream &logFile)
{
	stringstream dst;

	get_last_log(false, "obs-studio/logs", lastLogFile);

	currentLogFile = GenerateTimeDateFilename("txt");
	dst << "obs-studio/logs/" << currentLogFile.c_str();

	BPtr<char> path(GetAppConfigPathPtr(dst.str().c_str()));

#ifdef _WIN32
	BPtr<wchar_t> wpath;
	os_utf8_to_wcs_ptr(path, 0, &wpath);
	logFile.open(wpath, ios_base::in | ios_base::out | ios_base::trunc);
#else
	logFile.open(path, ios_base::in | ios_base::out | ios_base::trunc);
#endif

	if (logFile.is_open()) {
		delete_oldest_file(false, "obs-studio/logs");
		base_set_log_handler(do_log, &logFile);
	} else {
		blog(LOG_ERROR, "Failed to open log file");
	}
}

static auto ProfilerNameStoreRelease = [](profiler_name_store_t *store) {
	profiler_name_store_free(store);
};

using ProfilerNameStore = std::unique_ptr<profiler_name_store_t, decltype(ProfilerNameStoreRelease)>;

ProfilerNameStore CreateNameStore()
{
	return ProfilerNameStore{profiler_name_store_create(), ProfilerNameStoreRelease};
}

static auto SnapshotRelease = [](profiler_snapshot_t *snap) {
	profile_snapshot_free(snap);
};

using ProfilerSnapshot = std::unique_ptr<profiler_snapshot_t, decltype(SnapshotRelease)>;

ProfilerSnapshot GetSnapshot()
{
	return ProfilerSnapshot{profile_snapshot_create(), SnapshotRelease};
}

static void SaveProfilerData(const ProfilerSnapshot &snap)
{
	if (currentLogFile.empty())
		return;

	auto pos = currentLogFile.rfind('.');
	if (pos == currentLogFile.npos)
		return;

#define LITERAL_SIZE(x) x, (sizeof(x) - 1)
	ostringstream dst;
	dst.write(LITERAL_SIZE("obs-studio/profiler_data/"));
	dst.write(currentLogFile.c_str(), pos);
	dst.write(LITERAL_SIZE(".csv.gz"));
#undef LITERAL_SIZE

	BPtr<char> path = GetAppConfigPathPtr(dst.str().c_str());
	if (!profiler_snapshot_dump_csv_gz(snap.get(), path))
		blog(LOG_WARNING, "Could not save profiler data to '%s'", static_cast<const char *>(path));
}

static auto ProfilerFree = [](void *) {
	profiler_stop();

	auto snap = GetSnapshot();

	profiler_print(snap.get());
	profiler_print_time_between_calls(snap.get());

	SaveProfilerData(snap);

	profiler_free();
};

QAccessibleInterface *accessibleFactory(const QString &classname, QObject *object)
{
	if (classname == QLatin1String("VolumeSlider") && object && object->isWidgetType())
		return new VolumeAccessibleInterface(static_cast<QWidget *>(object));

	return nullptr;
}

static const char *run_program_init = "run_program_init";
static int run_program(fstream &logFile, int argc, char *argv[])
{
	int ret = -1;

	auto profilerNameStore = CreateNameStore();

	std::unique_ptr<void, decltype(ProfilerFree)> prof_release(static_cast<void *>(&ProfilerFree), ProfilerFree);

	profiler_start();
	profile_register_root(run_program_init, 0);

	ScopeProfiler prof{run_program_init};

#ifdef _WIN32
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

	QCoreApplication::addLibraryPath(".");

#if __APPLE__
	InstallNSApplicationSubclass();
	InstallNSThreadLocks();

	if (!isInBundle()) {
		blog(LOG_ERROR,
		     "OBS cannot be run as a standalone binary on macOS. Run the Application bundle instead.");
		return ret;
	}
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
	/* NOTE: Users blindly set this, but this theme is incompatble with Qt6 and
	 * crashes loading saved geometry. Just turn off this theme and let users complain OBS
	 * looks ugly instead of crashing. */
	const char *platform_theme = getenv("QT_QPA_PLATFORMTHEME");
	if (platform_theme && strcmp(platform_theme, "qt5ct") == 0)
		unsetenv("QT_QPA_PLATFORMTHEME");
#endif

	/* NOTE: This disables an optimisation in Qt that attempts to determine if
	 * any "siblings" intersect with a widget when determining the approximate
	 * visible/unobscured area. However, by Qt's own admission this is slow
	 * and in the case of OBS it significantly slows down lists with many
	 * elements (e.g. Hotkeys) and it is actually faster to disable it. */
	qputenv("QT_NO_SUBTRACTOPAQUESIBLINGS", "1");

	OBSApp program(argc, argv, profilerNameStore.get());
	try {
		QAccessible::installFactory(accessibleFactory);
		QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");
		QFontDatabase::addApplicationFont(":/fonts/OpenSans-Bold.ttf");
		QFontDatabase::addApplicationFont(":/fonts/OpenSans-Italic.ttf");

		bool created_log = false;

		program.AppInit();
		delete_oldest_file(false, "obs-studio/profiler_data");

		OBSTranslator translator;
		program.installTranslator(&translator);

		/* --------------------------------------- */
		/* check and warn if already running       */

		bool cancel_launch = false;
		bool already_running = false;

#ifdef _WIN32
		RunOnceMutex rom =
#endif
			CheckIfAlreadyRunning(already_running);

		if (!already_running) {
			goto run;
		}

		if (!multi) {
			QMessageBox mb(QMessageBox::Question, QTStr("AlreadyRunning.Title"),
				       QTStr("AlreadyRunning.Text"));
			mb.addButton(QTStr("AlreadyRunning.LaunchAnyway"), QMessageBox::YesRole);
			QPushButton *cancelButton = mb.addButton(QTStr("Cancel"), QMessageBox::NoRole);
			mb.setDefaultButton(cancelButton);

			mb.exec();
			cancel_launch = mb.clickedButton() == cancelButton;
		}

		if (cancel_launch)
			return 0;

		if (!created_log) {
			create_log_file(logFile);
			created_log = true;
		}

		if (multi) {
			blog(LOG_INFO, "User enabled --multi flag and is now "
				       "running multiple instances of OBS.");
		} else {
			blog(LOG_WARNING, "================================");
			blog(LOG_WARNING, "Warning: OBS is already running!");
			blog(LOG_WARNING, "================================");
			blog(LOG_WARNING, "User is now running multiple "
					  "instances of OBS!");
			/* Clear unclean_shutdown flag as multiple instances
			 * running from the same config will lead to a
			 * false-positive detection.*/
			unclean_shutdown = false;
		}

		/* --------------------------------------- */
	run:

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__FreeBSD__)
		// Mounted by termina during chromeOS linux container startup
		// https://chromium.googlesource.com/chromiumos/overlays/board-overlays/+/master/project-termina/chromeos-base/termina-lxd-scripts/files/lxd_setup.sh
		os_dir_t *crosDir = os_opendir("/opt/google/cros-containers");
		if (crosDir) {
			QMessageBox::StandardButtons buttons(QMessageBox::Ok);
			QMessageBox mb(QMessageBox::Critical, QTStr("ChromeOS.Title"), QTStr("ChromeOS.Text"), buttons,
				       nullptr);

			mb.exec();
			return 0;
		}
#endif

		if (!created_log)
			create_log_file(logFile);

		program.checkForUncleanShutdown();

		qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &message) {
			switch (type) {
#ifdef _DEBUG
			case QtDebugMsg:
				blog(LOG_DEBUG, "%s", QT_TO_UTF8(message));
				break;
			case QtInfoMsg:
				blog(LOG_INFO, "%s", QT_TO_UTF8(message));
				break;
#else
			case QtDebugMsg:
			case QtInfoMsg:
				break;
#endif
			case QtWarningMsg:
				blog(LOG_WARNING, "%s", QT_TO_UTF8(message));
				break;
			case QtCriticalMsg:
			case QtFatalMsg:
				blog(LOG_ERROR, "%s", QT_TO_UTF8(message));
				break;
			}
		});

#ifdef __APPLE__
		MacPermissionStatus audio_permission = CheckPermission(kAudioDeviceAccess);
		MacPermissionStatus video_permission = CheckPermission(kVideoDeviceAccess);
		MacPermissionStatus input_monitoring_permission = CheckPermission(kInputMonitoring);
		MacPermissionStatus screen_permission = CheckPermission(kScreenCapture);

		int permissionsDialogLastShown =
			config_get_int(App()->GetAppConfig(), "General", "MacOSPermissionsDialogLastShown");
		if (permissionsDialogLastShown < MACOS_PERMISSIONS_DIALOG_VERSION) {
			OBSPermissions check(nullptr, screen_permission, video_permission, audio_permission,
					     input_monitoring_permission);
			check.exec();
		}
#endif

#ifdef _WIN32
		if (IsRunningOnWine()) {
			QMessageBox mb(QMessageBox::Question, QTStr("Wine.Title"), QTStr("Wine.Text"));
			mb.setTextFormat(Qt::RichText);
			mb.addButton(QTStr("AlreadyRunning.LaunchAnyway"), QMessageBox::AcceptRole);
			QPushButton *closeButton = mb.addButton(QMessageBox::Close);
			mb.setDefaultButton(closeButton);

			mb.exec();
			if (mb.clickedButton() == closeButton)
				return 0;
		}
#endif

		if (argc > 1) {
			stringstream stor;
			stor << argv[1];
			for (int i = 2; i < argc; ++i) {
				stor << " " << argv[i];
			}
			blog(LOG_INFO, "Command Line Arguments: %s", stor.str().c_str());
		}

		if (!program.OBSInit())
			return 0;

		prof.Stop();

		ret = program.exec();

	} catch (const char *error) {
		blog(LOG_ERROR, "%s", error);
		OBSErrorBox(nullptr, "%s", error);
	}

	if (restart || restart_safe) {
		arguments = qApp->arguments();

		if (restart_safe) {
			arguments.append("--safe-mode");
		} else {
			arguments.removeAll("--safe-mode");
		}
	}

	return ret;
}

#define MAX_CRASH_REPORT_SIZE (200 * 1024)

#ifdef _WIN32

#define CRASH_MESSAGE                                                      \
	"Woops, OBS has crashed!\n\nWould you like to copy the crash log " \
	"to the clipboard? The crash log will still be saved to:\n\n%s"

static void main_crash_handler(const char *format, va_list args, void * /* param */)
{
	char *text = new char[MAX_CRASH_REPORT_SIZE];

	vsnprintf(text, MAX_CRASH_REPORT_SIZE, format, args);
	text[MAX_CRASH_REPORT_SIZE - 1] = 0;

	string crashFilePath = "obs-studio/crashes";

	delete_oldest_file(true, crashFilePath.c_str());

	string name = crashFilePath + "/";
	name += "Crash " + GenerateTimeDateFilename("txt");

	BPtr<char> path(GetAppConfigPathPtr(name.c_str()));

	fstream file;

#ifdef _WIN32
	BPtr<wchar_t> wpath;
	os_utf8_to_wcs_ptr(path, 0, &wpath);
	file.open(wpath, ios_base::in | ios_base::out | ios_base::trunc | ios_base::binary);
#else
	file.open(path, ios_base::in | ios_base::out | ios_base::trunc | ios_base::binary);
#endif
	file << text;
	file.close();

	string pathString(path.Get());

#ifdef _WIN32
	std::replace(pathString.begin(), pathString.end(), '/', '\\');
#endif

	string absolutePath = canonical(filesystem::path(pathString)).u8string();

	size_t size = snprintf(nullptr, 0, CRASH_MESSAGE, absolutePath.c_str());

	unique_ptr<char[]> message_buffer(new char[size + 1]);

	snprintf(message_buffer.get(), size + 1, CRASH_MESSAGE, absolutePath.c_str());

	string finalMessage = string(message_buffer.get(), message_buffer.get() + size);

	int ret = MessageBoxA(NULL, finalMessage.c_str(), "OBS has crashed!", MB_YESNO | MB_ICONERROR | MB_TASKMODAL);

	if (ret == IDYES) {
		size_t len = strlen(text);

		HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, len);
		memcpy(GlobalLock(mem), text, len);
		GlobalUnlock(mem);

		OpenClipboard(0);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, mem);
		CloseClipboard();
	}

	exit(-1);
}

static void load_debug_privilege(void)
{
	const DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
	TOKEN_PRIVILEGES tp;
	HANDLE token;
	LUID val;

	if (!OpenProcessToken(GetCurrentProcess(), flags, &token)) {
		return;
	}

	if (!!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL, NULL);
	}

	if (!!LookupPrivilegeValue(NULL, SE_INC_BASE_PRIORITY_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL, NULL)) {
			blog(LOG_INFO, "Could not set privilege to "
				       "increase GPU priority");
		}
	}

	CloseHandle(token);
}
#endif

static inline bool arg_is(const char *arg, const char *long_form, const char *short_form)
{
	return (long_form && strcmp(arg, long_form) == 0) || (short_form && strcmp(arg, short_form) == 0);
}

#ifdef _WIN32
static constexpr char vcRunErrorTitle[] = "Outdated Visual C++ Runtime";
static constexpr char vcRunErrorMsg[] = "OBS Studio requires a newer version of the Microsoft Visual C++ "
					"Redistributables.\n\nYou will now be directed to the download page.";
static constexpr char vcRunInstallerUrl[] = "https://obsproject.com/visual-studio-2022-runtimes";

static bool vc_runtime_outdated()
{
	win_version_info ver;
	if (!get_dll_ver(L"msvcp140.dll", &ver))
		return true;
	/* Major is always 14 (hence 140.dll), so we only care about minor. */
	if (ver.minor >= 40)
		return false;

	int choice = MessageBoxA(NULL, vcRunErrorMsg, vcRunErrorTitle, MB_OKCANCEL | MB_ICONERROR | MB_TASKMODAL);
	if (choice == IDOK) {
		/* Open the URL in the default browser. */
		ShellExecuteA(NULL, "open", vcRunInstallerUrl, NULL, NULL, SW_SHOWNORMAL);
	}

	return true;
}
#endif

#if defined(__APPLE__) || defined(__linux__)
#define BASE_PATH ".."
#else
#define BASE_PATH "../.."
#endif

#if defined(ENABLE_PORTABLE_CONFIG) || defined(_WIN32)
#define ALLOW_PORTABLE_MODE 1
#else
#define ALLOW_PORTABLE_MODE 0
#endif

int main(int argc, char *argv[])
{
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);

	struct sigaction sig_handler;

	sig_handler.sa_handler = OBSApp::SigIntSignalHandler;
	sigemptyset(&sig_handler.sa_mask);
	sig_handler.sa_flags = 0;

	sigaction(SIGINT, &sig_handler, NULL);

	/* Block SIGPIPE in all threads, this can happen if a thread calls write on
	a closed pipe. */
	sigset_t sigpipe_mask;
	sigemptyset(&sigpipe_mask);
	sigaddset(&sigpipe_mask, SIGPIPE);
	sigset_t saved_mask;
	if (pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &saved_mask) == -1) {
		perror("pthread_sigmask");
		exit(1);
	}
#endif

#ifdef _WIN32
	// Abort as early as possible if MSVC runtime is outdated
	if (vc_runtime_outdated())
		return 1;
	// Try to keep this as early as possible
	install_dll_blocklist_hook();

	obs_init_win32_crash_handler();
	SetErrorMode(SEM_FAILCRITICALERRORS);
	load_debug_privilege();
	base_set_crash_handler(main_crash_handler, nullptr);

	const HMODULE hRtwq = LoadLibrary(L"RTWorkQ.dll");
	if (hRtwq) {
		typedef HRESULT(STDAPICALLTYPE * PFN_RtwqStartup)();
		PFN_RtwqStartup func = (PFN_RtwqStartup)GetProcAddress(hRtwq, "RtwqStartup");
		func();
	}
#endif

	base_get_log_handler(&def_log_handler, nullptr);

#if defined(__FreeBSD__)
	move_to_xdg();
#endif

	obs_set_cmdline_args(argc, argv);

	for (int i = 1; i < argc; i++) {
		if (arg_is(argv[i], "--multi", "-m")) {
			multi = true;

#if ALLOW_PORTABLE_MODE
		} else if (arg_is(argv[i], "--portable", "-p")) {
			portable_mode = true;

#endif
		} else if (arg_is(argv[i], "--verbose", nullptr)) {
			log_verbose = true;

		} else if (arg_is(argv[i], "--safe-mode", nullptr)) {
			safe_mode = true;

		} else if (arg_is(argv[i], "--only-bundled-plugins", nullptr)) {
			disable_3p_plugins = true;

		} else if (arg_is(argv[i], "--always-on-top", nullptr)) {
			opt_always_on_top = true;

		} else if (arg_is(argv[i], "--unfiltered_log", nullptr)) {
			unfiltered_log = true;

		} else if (arg_is(argv[i], "--startstreaming", nullptr)) {
			opt_start_streaming = true;

		} else if (arg_is(argv[i], "--startrecording", nullptr)) {
			opt_start_recording = true;

		} else if (arg_is(argv[i], "--startreplaybuffer", nullptr)) {
			opt_start_replaybuffer = true;

		} else if (arg_is(argv[i], "--startvirtualcam", nullptr)) {
			opt_start_virtualcam = true;

		} else if (arg_is(argv[i], "--collection", nullptr)) {
			if (++i < argc)
				opt_starting_collection = argv[i];

		} else if (arg_is(argv[i], "--profile", nullptr)) {
			if (++i < argc)
				opt_starting_profile = argv[i];

		} else if (arg_is(argv[i], "--scene", nullptr)) {
			if (++i < argc)
				opt_starting_scene = argv[i];

		} else if (arg_is(argv[i], "--minimize-to-tray", nullptr)) {
			opt_minimize_tray = true;

		} else if (arg_is(argv[i], "--studio-mode", nullptr)) {
			opt_studio_mode = true;

		} else if (arg_is(argv[i], "--allow-opengl", nullptr)) {
			opt_allow_opengl = true;

		} else if (arg_is(argv[i], "--disable-updater", nullptr)) {
			opt_disable_updater = true;

		} else if (arg_is(argv[i], "--disable-missing-files-check", nullptr)) {
			opt_disable_missing_files_check = true;

		} else if (arg_is(argv[i], "--steam", nullptr)) {
			steam = true;

		} else if (arg_is(argv[i], "--help", "-h")) {
			std::string help =
				"--help, -h: Get list of available commands.\n\n"
				"--startstreaming: Automatically start streaming.\n"
				"--startrecording: Automatically start recording.\n"
				"--startreplaybuffer: Start replay buffer.\n"
				"--startvirtualcam: Start virtual camera (if available).\n\n"
				"--collection <string>: Use specific scene collection."
				"\n"
				"--profile <string>: Use specific profile.\n"
				"--scene <string>: Start with specific scene.\n\n"
				"--studio-mode: Enable studio mode.\n"
				"--minimize-to-tray: Minimize to system tray.\n"
#if ALLOW_PORTABLE_MODE
				"--portable, -p: Use portable mode.\n"
#endif
				"--multi, -m: Don't warn when launching multiple instances.\n\n"
				"--safe-mode: Run in Safe Mode (disables third-party plugins, scripting, and WebSockets).\n"
				"--only-bundled-plugins: Only load included (first-party) plugins\n"
				"--disable-shutdown-check: Disable unclean shutdown detection.\n"
				"--verbose: Make log more verbose.\n"
				"--always-on-top: Start in 'always on top' mode.\n\n"
				"--unfiltered_log: Make log unfiltered.\n\n"
				"--disable-updater: Disable built-in updater (Windows/Mac only)\n\n"
				"--disable-missing-files-check: Disable the missing files dialog which can appear on startup.\n\n";

#ifdef _WIN32
			MessageBoxA(NULL, help.c_str(), "Help", MB_OK | MB_ICONASTERISK);
#else
			std::cout << help << "--version, -V: Get current version.\n";
#endif
			exit(0);

		} else if (arg_is(argv[i], "--version", "-V")) {
			std::cout << "OBS Studio - " << App()->GetVersionString(false) << "\n";
			exit(0);
		}
	}

#if ALLOW_PORTABLE_MODE
	if (!portable_mode) {
		portable_mode = os_file_exists(BASE_PATH "/portable_mode") ||
				os_file_exists(BASE_PATH "/obs_portable_mode") ||
				os_file_exists(BASE_PATH "/portable_mode.txt") ||
				os_file_exists(BASE_PATH "/obs_portable_mode.txt");
	}

	if (!opt_disable_updater) {
		opt_disable_updater = os_file_exists(BASE_PATH "/disable_updater") ||
				      os_file_exists(BASE_PATH "/disable_updater.txt");
	}

	if (!opt_disable_missing_files_check) {
		opt_disable_missing_files_check = os_file_exists(BASE_PATH "/disable_missing_files_check") ||
						  os_file_exists(BASE_PATH "/disable_missing_files_check.txt");
	}
#endif

	fstream logFile;

	curl_global_init(CURL_GLOBAL_ALL);
	int ret = run_program(logFile, argc, argv);

#ifdef _WIN32
	if (hRtwq) {
		typedef HRESULT(STDAPICALLTYPE * PFN_RtwqShutdown)();
		PFN_RtwqShutdown func = (PFN_RtwqShutdown)GetProcAddress(hRtwq, "RtwqShutdown");
		func();
		FreeLibrary(hRtwq);
	}

	log_blocked_dlls();
#endif

	blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
	base_set_log_handler(nullptr, nullptr);

	if (restart || restart_safe) {
		auto executable = arguments.takeFirst();
		QProcess::startDetached(executable, arguments);
	}

	return ret;
}
