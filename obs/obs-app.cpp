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

#ifdef _WIN32
#include <fstream>
#include <windows.h>
#else
#include <signal.h>
#endif

using namespace std;

#ifdef _WIN32
static void do_log(int log_level, const char *msg, va_list args, void *param)
{
	fstream &logFile = *static_cast<fstream*>(param);
	char bla[4096];
	vsnprintf(bla, 4095, msg, args);

	OutputDebugStringA(bla);
	OutputDebugStringA("\n");

	if (log_level <= LOG_INFO)
		logFile << bla << endl;

	if (log_level <= LOG_ERROR && IsDebuggerPresent())
		__debugbreak();
}
#endif

bool OBSApp::InitGlobalConfigDefaults()
{
	config_set_default_string(globalConfig, "General", "Language", "en");

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
	BPtr<char> configPath;

	configPath = os_get_config_path("obs-studio");
	if (!do_mkdir(configPath))
		return false;

	configPath = os_get_config_path("obs-studio/basic");
	if (!do_mkdir(configPath))
		return false;

	configPath = os_get_config_path("obs-studio/studio");
	if (!do_mkdir(configPath))
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

	mainWindow = move(unique_ptr<OBSBasic>(new OBSBasic()));
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
	mainWindow->OBSInit();
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

int main(int argc, char *argv[])
{
#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	int ret = -1;
	QCoreApplication::addLibraryPath(".");
#ifdef _WIN32
	char *logPath = os_get_config_path("obs-studio/log.txt");
	fstream logFile;

	logFile.open(logPath, ios_base::in | ios_base::out | ios_base::trunc,
			_SH_DENYNO);
	bfree(logPath);

	if (logFile.is_open())
		base_set_log_handler(do_log, &logFile);
#endif

	try {
		OBSApp program(argc, argv);
		program.setStyle(new NoFocusFrameStyle);
		program.OBSInit();
		ret = program.exec();

	} catch (const char *error) {
		blog(LOG_ERROR, "%s", error);
	}

	blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
	base_set_log_handler(nullptr, nullptr);
	return ret;
}
