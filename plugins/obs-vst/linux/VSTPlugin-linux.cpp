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

#include <util/platform.h>

AEffect* VSTPlugin::loadEffect()
{
	AEffect* plugin = nullptr;

	soHandle = os_dlopen(pluginPath.c_str());
	if (soHandle == nullptr) {
		blog(LOG_WARNING,
		     "Failed trying to load VST from '%s',"
		     "error %d\n",
		     pluginPath.c_str(),
		     errno);
		return nullptr;
	}

	vstPluginMain mainEntryPoint;

	mainEntryPoint = (vstPluginMain)os_dlsym(soHandle, "VSTPluginMain");

	if (mainEntryPoint == nullptr) {
		mainEntryPoint = (vstPluginMain)os_dlsym(soHandle, "VstPluginMain()");
	}

	if (mainEntryPoint == nullptr) {
		mainEntryPoint = (vstPluginMain)os_dlsym(soHandle, "main");
	}

	if (mainEntryPoint == nullptr) {
		blog(LOG_WARNING, "Couldn't get a pointer to plug-in's main()");
		return nullptr;
	}

	// Instantiate the plug-in
	plugin       = mainEntryPoint(hostCallback_static);
	plugin->user = this;
	return plugin;
}

void VSTPlugin::unloadLibrary()
{
	if (soHandle) {
		os_dlclose(soHandle);
		soHandle = nullptr;
	}
}
