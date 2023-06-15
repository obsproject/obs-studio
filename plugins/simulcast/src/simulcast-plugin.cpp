/*
simulcast
Copyright (C) 2023-2023 John R. Bradley <jocbrad@twitch.tv>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "simulcast-plugin.h"
#include "common.h"
#include "global-service.h"
#include "simulcast-dock-widget.h"

#include <QAction>
#include <QMainWindow>
#include <obs-module.h>
#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("simulcast", "en-US")
OBS_MODULE_AUTHOR("John R. Bradley")
const char *obs_module_name(void)
{
	return "simulcast";
}
const char *obs_module_description(void)
{
	return obs_module_text("Simulcast.Plugin.Description");
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "Loading module simulcast (%s)", SIMULCAST_VERSION);

	if (obs_get_version() < MAKE_SEMANTIC_VERSION(29, 1, 0))
		return false;

	auto mainWindow = (QMainWindow *)obs_frontend_get_main_window();
	if (mainWindow == nullptr)
		return false;

	QMetaObject::invokeMethod(mainWindow, []() {
		GetGlobalService().setCurrentThreadAsDefault();
	});

	auto dock = new SimulcastDockWidget(mainWindow);

	obs_frontend_add_dock_by_id("simulcast", "Simulcast", dock);

	obs_frontend_add_event_callback(
		[](enum obs_frontend_event event, void *private_data) {
			auto simulcastWidget =
				static_cast<SimulcastDockWidget *>(
					private_data);

			if (event ==
			    obs_frontend_event::OBS_FRONTEND_EVENT_EXIT) {
				simulcastWidget->SaveConfig();
			} else if (event ==
				   obs_frontend_event::
					   OBS_FRONTEND_EVENT_PROFILE_CHANGED) {
				static_cast<SimulcastDockWidget *>(private_data)
					->LoadConfig();
			}
		},
		dock);

	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "Unloading module");
}
