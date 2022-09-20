/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.
Additional Code Copyright (C) 2016-2017 by c3r1c3 <c3r1c3@nevermindonline.com>

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
*****************************************************************************/
#include "../headers/VSTPlugin.h"
#include "../headers/vst-plugin-callbacks.hpp"

#include <util/platform.h>
#include <windows.h>

AEffect *VSTPlugin::loadEffect()
{
	AEffect *plugin = nullptr;

	wchar_t *wpath;
	os_utf8_to_wcs_ptr(pluginPath.c_str(), 0, &wpath);
	dllHandle = LoadLibraryW(wpath);
	bfree(wpath);
	if (dllHandle == nullptr) {

		DWORD errorCode = GetLastError();

		// Display the error message and exit the process
		if (errorCode == ERROR_BAD_EXE_FORMAT) {
			blog(LOG_WARNING, "Could not open library, "
					  "wrong architecture.");
		} else {
			blog(LOG_WARNING,
			     "Failed trying to load VST from '%s'"
			     ", error %d\n",
			     pluginPath.c_str(), GetLastError());
		}
		return nullptr;
	}

	vstPluginMain mainEntryPoint =
		(vstPluginMain)GetProcAddress(dllHandle, "VSTPluginMain");

	if (mainEntryPoint == nullptr) {
		mainEntryPoint = (vstPluginMain)GetProcAddress(
			dllHandle, "VstPluginMain()");
	}

	if (mainEntryPoint == nullptr) {
		mainEntryPoint =
			(vstPluginMain)GetProcAddress(dllHandle, "main");
	}

	if (mainEntryPoint == nullptr) {
		blog(LOG_WARNING, "Couldn't get a pointer to plug-in's main()");
		return nullptr;
	}

	// Instantiate the plug-in
	try {
		plugin = mainEntryPoint(hostCallback_static);
	} catch (...) {
		blog(LOG_WARNING, "VST plugin initialization failed");
		return nullptr;
	}

	if (plugin == nullptr) {
		blog(LOG_WARNING, "Couldn't create instance for '%s'",
		     pluginPath.c_str());
		return nullptr;
	}

	plugin->user = this;
	return plugin;
}

void VSTPlugin::unloadLibrary()
{
	if (dllHandle) {
		FreeLibrary(dllHandle);
		dllHandle = nullptr;
	}
}

bool VSTPlugin::vstLoaded()
{
	return (dllHandle != nullptr);
}
