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
#include <util/platform.h>

#include "obs-app.hpp"
#include "window-main-basic.hpp"
#include "obs-wrappers.hpp"
#include "wx-wrappers.hpp"
#include "platform.hpp"

using namespace std;

IMPLEMENT_APP(OBSApp);

OBSAppBase::~OBSAppBase()
{
	blog(LOG_INFO, "Number of memory leaks: %llu", bnum_allocs());
}

static void do_log(enum log_type type, const char *msg, va_list args)
{
	char bla[4096];
	vsnprintf(bla, 4095, msg, args);

#ifdef _WIN32
	OutputDebugStringA(bla);
	OutputDebugStringA("\n");

	if (type >= LOG_WARNING)
		__debugbreak();
#endif
}

bool OBSApp::InitGlobalConfigDefaults()
{
	config_set_default_string(globalConfig, "General", "Language", "en");
	config_set_default_int(globalConfig, "Window", "PosX",  -1);
	config_set_default_int(globalConfig, "Window", "PosY",  -1);
	config_set_default_int(globalConfig, "Window", "SizeX", -1);
	config_set_default_int(globalConfig, "Window", "SizeY", -1);

	vector<MonitorInfo> monitors;
	GetMonitors(monitors);

	if (!monitors.size()) {
		OBSErrorBox(NULL, "There appears to be no monitors.  Er, this "
		                  "technically shouldn't be possible.");
		return false;
	}

	uint32_t cx = monitors[0].cx;
	uint32_t cy = monitors[0].cy;

#if _WIN32
	config_set_default_string(globalConfig, "Video", "Renderer",
			"Direct3D 11");
#else
	config_set_default_string(globalConfig, "Video", "Renderer",
			"OpenGL");
#endif

	config_set_default_uint(globalConfig, "Video", "BaseCX",   cx);
	config_set_default_uint(globalConfig, "Video", "BaseCY",   cy);

	cx = cx * 10 / 15;
	cy = cy * 10 / 15;
	config_set_default_uint(globalConfig, "Video", "OutputCX", cx);
	config_set_default_uint(globalConfig, "Video", "OutputCY", cy);

	config_set_default_string(globalConfig, "Video", "FPSType", "Common");
	config_set_default_string(globalConfig, "Video", "FPSCommon", "30");
	config_set_default_uint(globalConfig, "Video", "FPSInt", 30);
	config_set_default_uint(globalConfig, "Video", "FPSNum", 30);
	config_set_default_uint(globalConfig, "Video", "FPSDen", 1);
	config_set_default_uint(globalConfig, "Video", "FPSNS", 33333333);

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
	BPtr<char> homePath(os_get_home_path());
	stringstream str;

	str << homePath << "/obs-studio";
	if (!do_mkdir(str.str().c_str()))
		return false;

	return true;
}

bool OBSApp::InitGlobalConfig()
{
	BPtr<char> homePath(os_get_home_path());
	stringstream str;

	if (!homePath) {
		OBSErrorBox(NULL, "Failed to get home path");
		return false;
	}

	str << homePath << "/obs-studio/global.ini";
	string path = move(str.str());

	int errorcode = globalConfig.Open(path.c_str(), CONFIG_OPEN_ALWAYS);
	if (errorcode != CONFIG_SUCCESS) {
		OBSErrorBox(NULL, "Failed to open global.ini: %d", errorcode);
		return false;
	}

	if (!InitGlobalConfigDefaults())
		return false;

	return true;
}

bool OBSApp::InitLocale()
{
	const char *lang = config_get_string(globalConfig, "General",
			"Language");

	stringstream file;
	file << "locale/" << lang << ".txt";

	string path;
	if (!GetDataFilePath(file.str().c_str(), path)) {
		/* use en-US by default if language file is not found */
		if (!GetDataFilePath("locale/en-US.txt", path)) {
			OBSErrorBox(NULL, "Failed to open locale file");
			return false;
		}
	}

	textLookup = text_lookup_create(path.c_str());
	return true;
}

bool OBSApp::OnInit()
{
	base_set_log_handler(do_log);

	if (!wxApp::OnInit())
		return false;
	if (!MakeUserDirs())
		return false;
	if (!InitGlobalConfig())
		return false;
	if (!InitLocale())
		return false;
	if (!obs_startup())
		return false;

	wxInitAllImageHandlers();

	dummyWindow = new wxFrame(NULL, wxID_ANY, "Dummy Window");

	OBSBasic *mainWindow = new OBSBasic();

	/* this is a test */
	struct obs_video_info ovi;
	ovi.graphics_module = "libobs-opengl";
	ovi.fps_num         = 30000;
	ovi.fps_den         = 1001;
	ovi.window_width    = 2;
	ovi.window_height   = 2;
	ovi.base_width      = 1920;
	ovi.base_height     = 1080;
	ovi.output_width    = 1280;
	ovi.output_height   = 720;
	ovi.output_format   = VIDEO_FORMAT_RGBA;
	ovi.adapter         = 0;
	ovi.window          = WxToGSWindow(dummyWindow);

	if (!obs_reset_video(&ovi))
		return false;

	mainWindow->Show();
	return true;
}

int OBSApp::OnExit()
{
	obs_shutdown();

	delete dummyWindow;
	dummyWindow = NULL;

	return wxApp::OnExit();
}

void OBSApp::CleanUp()
{
	wxApp::CleanUp();
}
