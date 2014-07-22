/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include <time.h>
#include <stdio.h>
#include <sstream>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <obs-config.h>
#include <obs.hpp>

#include <QProxyStyle>

#include "qt-wrappers.hpp"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "window-license-agreement.hpp"
#include "platform.hpp"

#include <fstream>

#ifdef _WIN32
#include <windows.h>
#define snprintf _snprintf
#else
#include <signal.h>
#endif

using namespace std;

static log_handler_t def_log_handler;

static string currentLogFile;
static string lastLogFile;

string CurrentTimeString()
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%X", &tstruct);
	return buf;
}

string CurrentDateTimeString()
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d, %X", &tstruct);
	return buf;
}

static void do_log(int log_level, const char *msg, va_list args, void *param)
{
	fstream &logFile = *static_cast<fstream*>(param);
	char str[4096];

#ifndef _WIN32
	va_list args2;
	va_copy(args2, args);
#endif

	vsnprintf(str, 4095, msg, args);

#ifdef _WIN32
	OutputDebugStringA(str);
	OutputDebugStringA("\n");
#else
	def_log_handler(log_level, msg, args2, nullptr);
#endif

	if (log_level <= LOG_INFO)
		logFile << CurrentTimeString() << ": " << str << endl;

#ifdef _WIN32
	if (log_level <= LOG_ERROR && IsDebuggerPresent())
		__debugbreak();
#endif
}

#define DEFAULT_LANG "en-US"

bool OBSApp::InitGlobalConfigDefaults()
{
	config_set_default_string(globalConfig, "General", "Language",
			DEFAULT_LANG);
	config_set_default_uint(globalConfig, "General", "MaxLogs", 10);

#if _WIN32
	config_set_default_string(globalConfig, "Video", "Renderer",
			"Direct3D 11");
#else
	config_set_default_string(globalConfig, "Video", "Renderer", "OpenGL");
#endif

	return true;
}

static bool do_mkdir(const char *path)
{
	if (os_mkdir(path) == MKDIR_ERROR) {
		OBSErrorBox(NULL, "Failed to create directory %s", path);
		return false;
	}

	return true;
}

static bool MakeUserDirs()
{
	BPtr<char> path;

	path = os_get_config_path("obs-studio");
	if (!do_mkdir(path))
		return false;

	path = os_get_config_path("obs-studio/basic");
	if (!do_mkdir(path))
		return false;

	path = os_get_config_path("obs-studio/logs");
	if (!do_mkdir(path))
		return false;

	return true;
}

bool OBSApp::InitGlobalConfig()
{
	BPtr<char> path(os_get_config_path("obs-studio/global.ini"));

	int errorcode = globalConfig.Open(path, CONFIG_OPEN_ALWAYS);
	if (errorcode != CONFIG_SUCCESS) {
		OBSErrorBox(NULL, "Failed to open global.ini: %d", errorcode);
		return false;
	}

	return InitGlobalConfigDefaults();
}

bool OBSApp::InitLocale()
{
	const char *lang = config_get_string(globalConfig, "General",
			"Language");

	locale = lang;

	string englishPath;
	if (!GetDataFilePath("locale/" DEFAULT_LANG ".ini", englishPath)) {
		OBSErrorBox(NULL, "Failed to find locale/" DEFAULT_LANG ".ini");
		return false;
	}

	textLookup = text_lookup_create(englishPath.c_str());
	if (!textLookup) {
		OBSErrorBox(NULL, "Failed to create locale from file '%s'",
				englishPath.c_str());
		return false;
	}

	bool userLocale = config_has_user_value(globalConfig, "General",
			"Language");
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

			blog(LOG_INFO, "Using preferred locale '%s'",
					locale_.c_str());
			locale = locale_;
			return true;
		}

		return true;
	}

	stringstream file;
	file << "locale/" << lang << ".ini";

	string path;
	if (GetDataFilePath(file.str().c_str(), path)) {
		if (!text_lookup_add(textLookup, path.c_str()))
			blog(LOG_ERROR, "Failed to add locale file '%s'",
					path.c_str());
	} else {
		blog(LOG_ERROR, "Could not find locale file '%s'",
				file.str().c_str());
	}

	return true;
}

OBSApp::OBSApp(int &argc, char **argv)
	: QApplication(argc, argv)
{}

void OBSApp::AppInit()
{
	if (!InitApplicationBundle())
		throw "Failed to initialize application bundle";
	if (!MakeUserDirs())
		throw "Failed to created required user directories";
	if (!InitGlobalConfig())
		throw "Failed to initialize global config";
	if (!InitLocale())
		throw "Failed to load locale";
}

const char *OBSApp::GetRenderModule() const
{
	const char *renderer = config_get_string(globalConfig, "Video",
			"Renderer");

	return (astrcmpi(renderer, "Direct3D 11") == 0) ?
		"libobs-d3d11" : "libobs-opengl";
}

bool OBSApp::OBSInit()
{
	bool licenseAccepted = config_get_bool(globalConfig, "General",
			"LicenseAccepted");
	OBSLicenseAgreement agreement(nullptr);

	if (licenseAccepted || agreement.exec() == QDialog::Accepted) {
		if (!licenseAccepted) {
			config_set_bool(globalConfig, "General",
					"LicenseAccepted", true);
			config_save(globalConfig);
		}

		mainWindow = new OBSBasic();

		mainWindow->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(mainWindow, SIGNAL(destroyed()), this, SLOT(quit()));

		mainWindow->OBSInit();

		return true;
	} else {
		return false;
	}
}

string OBSApp::GetVersionString() const
{
	stringstream ver;

#ifdef HAVE_OBSCONFIG_H
	ver << OBS_VERSION;
#else
	ver <<  LIBOBS_API_MAJOR_VER << "." <<
		LIBOBS_API_MINOR_VER << "." <<
		LIBOBS_API_PATCH_VER;

#endif
	ver << " (";

#ifdef _WIN32
	if (sizeof(void*) == 8)
		ver << "64bit, ";

	ver << "windows)";
#elif __APPLE__
	ver << "mac)";
#else /* assume linux for the time being */
	ver << "linux)";
#endif

	return ver.str();
}

#ifdef __APPLE__
#define INPUT_AUDIO_SOURCE  "coreaudio_input_capture"
#define OUTPUT_AUDIO_SOURCE "coreaudio_output_capture"
#elif _WIN32
#define INPUT_AUDIO_SOURCE  "wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE "wasapi_output_capture"
#else
#define INPUT_AUDIO_SOURCE  "pulse_input_capture"
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

QString OBSTranslator::translate(const char *context, const char *sourceText,
		const char *disambiguation, int n) const
{
	const char *out = nullptr;
	if (!text_lookup_getstr(App()->GetTextLookup(), sourceText, &out))
		return QString();

	UNUSED_PARAMETER(context);
	UNUSED_PARAMETER(disambiguation);
	UNUSED_PARAMETER(n);
	return QT_UTF8(out);
}

struct NoFocusFrameStyle : QProxyStyle
{
	void drawControl(ControlElement element, const QStyleOption *option,
			QPainter *painter, const QWidget *widget=nullptr)
		const override
	{
		if (element == CE_FocusFrame)
			return;

		QProxyStyle::drawControl(element, option, painter, widget);
	}
};

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

static uint64_t convert_log_name(const char *name)
{
	BaseLexer  lex;
	string     year, month, day, hour, minute, second;

	lexer_start(lex, name);

	if (!get_token(lex, year,   BASETOKEN_DIGIT)) return 0;
	if (!expect_token(lex, "-", BASETOKEN_OTHER)) return 0;
	if (!get_token(lex, month,  BASETOKEN_DIGIT)) return 0;
	if (!expect_token(lex, "-", BASETOKEN_OTHER)) return 0;
	if (!get_token(lex, day,    BASETOKEN_DIGIT)) return 0;
	if (!get_token(lex, hour,   BASETOKEN_DIGIT)) return 0;
	if (!expect_token(lex, "-", BASETOKEN_OTHER)) return 0;
	if (!get_token(lex, minute, BASETOKEN_DIGIT)) return 0;
	if (!expect_token(lex, "-", BASETOKEN_OTHER)) return 0;
	if (!get_token(lex, second, BASETOKEN_DIGIT)) return 0;

	stringstream timestring;
	timestring << year << month << day << hour << minute << second;
	return std::stoull(timestring.str());
}

static void delete_oldest_log(void)
{
	BPtr<char>       logDir(os_get_config_path("obs-studio/logs"));
	string           oldestLog;
	uint64_t         oldest_ts = (uint64_t)-1;
	struct os_dirent *entry;

	unsigned int maxLogs = (unsigned int)config_get_uint(
			App()->GlobalConfig(), "General", "MaxLogs");

	os_dir_t dir = os_opendir(logDir);
	if (dir) {
		unsigned int count = 0;

		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.')
				continue;

			uint64_t ts = convert_log_name(entry->d_name);

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

static void get_last_log(void)
{
	BPtr<char>       logDir(os_get_config_path("obs-studio/logs"));
	struct os_dirent *entry;
	os_dir_t         dir        = os_opendir(logDir);
	uint64_t         highest_ts = 0;

	if (dir) {
		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.')
				continue;

			uint64_t ts = convert_log_name(entry->d_name);

			if (ts > highest_ts) {
				lastLogFile = entry->d_name;
				highest_ts  = ts;
			}
		}

		os_closedir(dir);
	}
}

string GenerateTimeDateFilename(const char *extension)
{
	time_t    now = time(0);
	char      file[256] = {};
	struct tm *cur_time;

	cur_time = localtime(&now);
	snprintf(file, sizeof(file), "%d-%02d-%02d %02d-%02d-%02d.%s",
			cur_time->tm_year+1900,
			cur_time->tm_mon+1,
			cur_time->tm_mday,
			cur_time->tm_hour,
			cur_time->tm_min,
			cur_time->tm_sec,
			extension);

	return string(file);
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

static void create_log_file(fstream &logFile)
{
	stringstream dst;

	get_last_log();

	currentLogFile = GenerateTimeDateFilename("txt");
	dst << "obs-studio/logs/" << currentLogFile.c_str();

	BPtr<char> path(os_get_config_path(dst.str().c_str()));
	logFile.open(path,
			ios_base::in | ios_base::out | ios_base::trunc);

	if (logFile.is_open()) {
		delete_oldest_log();
		base_set_log_handler(do_log, &logFile);
	} else {
		blog(LOG_ERROR, "Failed to open log file");
	}
}

static int run_program(fstream &logFile, int argc, char *argv[])
{
	int ret = -1;
	QCoreApplication::addLibraryPath(".");

	OBSApp program(argc, argv);
	try {
		program.AppInit();

		OBSTranslator translator;

		create_log_file(logFile);

		program.installTranslator(&translator);
		program.setStyle(new NoFocusFrameStyle);

		ret = program.OBSInit() ? program.exec() : 0;

	} catch (const char *error) {
		blog(LOG_ERROR, "%s", error);
		OBSErrorBox(nullptr, "%s", error);
	}

	return ret;
}

int main(int argc, char *argv[])
{
#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	base_get_log_handler(&def_log_handler, nullptr);

	fstream logFile;

	int ret = run_program(logFile, argc, argv);

	blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
	base_set_log_handler(nullptr, nullptr);
	return ret;
}
