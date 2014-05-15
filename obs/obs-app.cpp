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
#include <obs.hpp>

#include <QProxyStyle>

#include "qt-wrappers.hpp"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
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

static void do_log(int log_level, const char *msg, va_list args, void *param)
{
	fstream &logFile = *static_cast<fstream*>(param);
	char str[4096];
	va_list args2;

	va_copy(args2, args);

	vsnprintf(str, 4095, msg, args);

#ifdef _WIN32
	OutputDebugStringA(str);
	OutputDebugStringA("\n");
#else
	def_log_handler(log_level, msg, args2, nullptr);
#endif

	if (log_level <= LOG_INFO)
		logFile << str << endl;

#ifdef _WIN32
	if (log_level <= LOG_ERROR && IsDebuggerPresent())
		__debugbreak();
#endif
}

bool OBSApp::InitGlobalConfigDefaults()
{
	config_set_default_string(globalConfig, "General", "Language", "en");
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

	path = os_get_config_path("obs-studio/studio");
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

#define DEFAULT_LANG "en"

bool OBSApp::InitLocale()
{
	const char *lang = config_get_string(globalConfig, "General",
			"Language");

	locale = lang;

	stringstream file;
	file << "locale/" << lang << ".txt";

	string englishPath;
	if (!GetDataFilePath("locale/" DEFAULT_LANG ".txt", englishPath)) {
		OBSErrorBox(NULL, "Failed to find locale/" DEFAULT_LANG ".txt");
		return false;
	}

	textLookup = text_lookup_create(englishPath.c_str());
	if (!textLookup) {
		OBSErrorBox(NULL, "Failed to create locale from file '%s'",
				englishPath.c_str());
		return false;
	}

	if (astrcmpi(lang, DEFAULT_LANG) == 0)
		return true;

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

	if (astrcmpi(renderer, "Direct3D 11") == 0)
		return "libobs-d3d11";
	else
		return "libobs-opengl";
}

void OBSApp::OBSInit()
{
	mainWindow = move(unique_ptr<OBSBasic>(new OBSBasic()));
	mainWindow->OBSInit();
}

string OBSApp::GetVersionString() const
{
	stringstream ver;
	ver << "v" <<
		LIBOBS_API_MAJOR_VER << "." <<
		LIBOBS_API_MINOR_VER << "." <<
		LIBOBS_API_PATCH_VER;

	if (sizeof(void*) == 8)
		ver << " (64bit)";
	else
		ver << " (32bit)";

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

static void delete_oldest_log(void)
{
	BPtr<char>       logDir(os_get_config_path("obs-studio/logs"));
	char             firstLog[256] = {};
	struct os_dirent *entry;

	unsigned int maxLogs = (unsigned int)config_get_uint(
			App()->GlobalConfig(), "General", "MaxLogs");

	os_dir_t dir = os_opendir(logDir);
	if (dir) {
		unsigned int count = 0;

		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory)
				continue;

			/* no hidden files */
			if (*entry->d_name == '.')
				continue;

			if (!*firstLog)
				strncpy(firstLog, entry->d_name, 255);

			count++;
		}

		os_closedir(dir);

		if (count > maxLogs && *firstLog) {
			stringstream delPath;

			delPath << logDir << "/" << firstLog;
			os_unlink(delPath.str().c_str());
		}
	}
}

static void create_log_file(fstream &logFile)
{
	stringstream dst;
	time_t       now = time(0);
	struct tm    *cur_time;

	cur_time = localtime(&now);
	if (cur_time) {
		char file[256] = {};

		snprintf(file, sizeof(file), "%d-%02d-%02d %02d-%02d-%02d.txt",
				cur_time->tm_year+1900,
				cur_time->tm_mon+1,
				cur_time->tm_mday,
				cur_time->tm_hour,
				cur_time->tm_min,
				cur_time->tm_sec);

		dst << "obs-studio/logs/" << file;

		BPtr<char> path(os_get_config_path(dst.str().c_str()));
		logFile.open(path,
				ios_base::in | ios_base::out | ios_base::trunc);
	}

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

	try {
		OBSApp program(argc, argv);
		OBSTranslator translator;

		create_log_file(logFile);

		program.installTranslator(&translator);
		program.setStyle(new NoFocusFrameStyle);
		program.OBSInit();

		ret = program.exec();

	} catch (const char *error) {
		blog(LOG_ERROR, "%s", error);
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
