/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
#include "window-obs-basic.hpp"
#include "obs-wrappers.hpp"
#include "wx-wrappers.hpp"
using namespace std;

IMPLEMENT_APP(OBSApp);

OBSAppBase::~OBSAppBase()
{
	blog(LOG_INFO, "Number of memory leaks: %u", bnum_allocs());
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

void OBSApp::InitGlobalConfigDefaults()
{
	config_set_default_int(globalConfig, "Window", "PosX",  -1);
	config_set_default_int(globalConfig, "Window", "PosY",  -1);
	config_set_default_int(globalConfig, "Window", "SizeX", -1);
	config_set_default_int(globalConfig, "Window", "SizeY", -1);
}

static bool do_mkdir(const char *path)
{
	if (os_mkdir(path) == MKDIR_ERROR) {
		blog(LOG_ERROR, "Failed to create directory %s", path);
		return false;
	}

	return true;
}

static bool MakeUserDirs()
{
	BPtr<char*> homePath = os_get_home_path();
	stringstream str;

	str << homePath << "/obs-studio";
	if (!do_mkdir(str.str().c_str()))
		return false;

	return true;
}

bool OBSApp::InitGlobalConfig()
{
	BPtr<char*> homePath = os_get_home_path();
	stringstream str;

	if (!homePath) {
		blog(LOG_ERROR, "Failed to get home path");
		return false;
	}

	str << homePath << "/obs-studio/global.ini";
	string path = move(str.str());

	int errorcode = globalConfig.Open(path.c_str(), CONFIG_OPEN_ALWAYS);
	if (errorcode != CONFIG_SUCCESS) {
		blog(LOG_ERROR, "Failed to open global.ini: %d", errorcode);
		return false;
	}

	InitGlobalConfigDefaults();
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
	if (!obs_startup())
		return false;

	wxInitAllImageHandlers();

	dummyWindow = new wxFrame(NULL, wxID_ANY, "Dummy Window");

	OBSBasic *mainWindow = new OBSBasic();

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
