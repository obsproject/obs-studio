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

#include "obs-app.hpp"
#include "window-basic-main.hpp"
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
#ifdef _WIN32
	char bla[4096];
	vsnprintf(bla, 4095, msg, args);

	OutputDebugStringA(bla);
	OutputDebugStringA("\n");

	if (type >= LOG_WARNING)
		__debugbreak();
#else
	vprintf(msg, args);
	printf("\n");
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
	BPtr<char> configPath(os_get_config_path("obs-studio"));
	return do_mkdir(configPath);
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
			blog(LOG_WARNING, "Failed to add locale file '%s'",
					path.c_str());
	} else {
		blog(LOG_WARNING, "Could not find locale file '%s'",
				file.str().c_str());
	}

	return true;
}

bool OBSApp::InitOBSBasic()
{
	OBSBasic *obsBasic = new OBSBasic();
	obsBasic->Show();

	mainWindow = obsBasic;
	return obsBasic->Init();
}

bool OBSApp::OnInit()
{
	base_set_log_handler(do_log);

	if (!wxApp::OnInit())
		return false;
	wxInitAllImageHandlers();

	if (!MakeUserDirs())
		return false;
	if (!InitGlobalConfig())
		return false;
	if (!InitLocale())
		return false;
	if (!InitOBSBasic())
		return false;

	return true;
}

int OBSApp::OnExit()
{
	return wxApp::OnExit();
}

void OBSApp::GetFPSCommon(uint32_t &num, uint32_t &den) const
{
	const char *val = config_get_string(globalConfig, "Video", "FPSCommon");

	if (strcmp(val, "10") == 0) {
		num = 30;
		den = 1;
	} else if (strcmp(val, "20") == 0) {
		num = 20;
		den = 1;
	} else if (strcmp(val, "25") == 0) {
		num = 25;
		den = 1;
	} else if (strcmp(val, "29.97") == 0) {
		num = 30000;
		den = 1001;
	} else if (strcmp(val, "48") == 0) {
		num = 48;
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

void OBSApp::GetFPSInteger(uint32_t &num, uint32_t &den) const
{
	num = (uint32_t)config_get_uint(globalConfig, "Video", "FPSInt");
	den = 1;
}

void OBSApp::GetFPSFraction(uint32_t &num, uint32_t &den) const
{
	num = (uint32_t)config_get_uint(globalConfig, "Video", "FPSNum");
	den = (uint32_t)config_get_uint(globalConfig, "Video", "FPSDen");
}

void OBSApp::GetFPSNanoseconds(uint32_t &num, uint32_t &den) const
{
	num = 1000000000;
	den = (uint32_t)config_get_uint(globalConfig, "Video", "FPSNS");
}

void OBSApp::GetConfigFPS(uint32_t &num, uint32_t &den) const
{
	const char *type = config_get_string(globalConfig, "Video", "FPSType");

	if (astrcmpi(type, "Integer") == 0)
		GetFPSInteger(num, den);
	else if (astrcmpi(type, "Fraction") == 0)
		GetFPSFraction(num, den);
	else if (astrcmpi(type, "Nanoseconds") == 0)
		GetFPSNanoseconds(num, den);
	else
		GetFPSCommon(num, den);
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
