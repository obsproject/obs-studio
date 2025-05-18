/******************************************************************************
    Copyright (C) 2022-2025 pkv <pkv@obsproject.com>

    This file is part of win-asio.

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
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include <QScreen>

#include <util/util.hpp>
#include <util/platform.h>
#include "ASIOSettingsDialog.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("asio-output-ui", "en-US")

struct asio_ui_output {
	bool enabled;
	obs_output_t *output;
	OBSData settings;
};

// We use a global context for asio output.
struct asio_ui_output context = {0};
bool output_running = false;
ASIOSettingsDialog *settingsDialog_ = nullptr;
std::string g_currentDeviceName;

OBSData load_settings()
{
	BPtr<char> path = obs_module_get_config_path(obs_current_module(), "asioOutputProps.json");
	BPtr<char> jsonData = os_quick_read_utf8_file(path);
	if (!!jsonData) {
		obs_data_t *data = obs_data_create_from_json(jsonData);
		OBSData dataRet(data);
		obs_data_release(data);
		return dataRet;
	}
	return nullptr;
}

#define MAX_DEVICE_CHANNELS 32

void save_default_settings(obs_data_t *settings)
{
	BPtr<char> modulePath = obs_module_get_config_path(obs_current_module(), "");
	os_mkdirs(modulePath);
	BPtr<char> path = obs_module_get_config_path(obs_current_module(), "asioOutputProps.json");
	obs_data_t *data = obs_data_create();
	obs_data_set_string(data, "device_name", obs_data_get_string(settings, "device_name"));
	for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
		char key[32];
		snprintf(key, sizeof(key), "device_ch%d", i);
		obs_data_set_int(data, key, -1);
	}
	obs_data_save_json_safe(data, path, "tmp", "bak");
}

void output_stop()
{
	if (context.output) {
		obs_output_stop(context.output);
	}
	output_running = false;
}

void output_start()
{
	if (context.output != nullptr) {
		output_running = obs_output_start(context.output);
		if (!output_running)
			output_stop();
	}
}

void callback()
{
	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
	QWidget *obsSettingsDialog = nullptr;
	const auto topLevels = QApplication::topLevelWidgets();
	for (QWidget *widget : topLevels) {
		if (widget->isVisible() && QString(widget->metaObject()->className()).contains("OBSBasicSettings")) {
			obsSettingsDialog = widget;
			break;
		}
	}
	if (!settingsDialog_) {
		if (!obsSettingsDialog)
			settingsDialog_ = new ASIOSettingsDialog(mainWindow, context.output, context.settings);
		else
			settingsDialog_ = new ASIOSettingsDialog(obsSettingsDialog, context.output, context.settings);
		settingsDialog_->setAttribute(Qt::WA_DeleteOnClose);
		QObject::connect(settingsDialog_, &QObject::destroyed, []() { settingsDialog_ = nullptr; });
	}

	settingsDialog_->ShowHideDialog(context.enabled);
	if (obsSettingsDialog) {
		QRect settingsRect = obsSettingsDialog->geometry();
		QRect asioRect = settingsDialog_->geometry();
		QPoint newPos(settingsRect.right() + 100, settingsRect.top());
		QScreen *screen = obsSettingsDialog->screen();
		QRect desktopRect = screen->availableGeometry();
		if (newPos.x() + asioRect.width() > desktopRect.right())
			newPos.setX(desktopRect.right() - asioRect.width());
		settingsDialog_->move(newPos);
	}
}

void addOutputUI(void)
{
	QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction(obs_module_text("AsioOutput.Menu"));
	action->setObjectName("asioOutputSetupAction");

	obs_frontend_push_ui_translation(obs_module_get_string);
	obs_frontend_pop_ui_translation();
	// the UI is added through the callback, which is triggered in OBS Audio Settings
	action->connect(action, &QAction::triggered, callback);
	action->setVisible(false);
}

static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		if (context.settings) {
			const char *device = obs_data_get_string(context.settings, "device_name");
			if (device && *device) {
				g_currentDeviceName = device;
				if (!output_running) {
					output_start();
				}
			}
		}
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		if (output_running)
			output_stop();
	}
}

bool obs_module_load(void)
{
	return true;
}

void obs_module_unload(void)
{
	if (output_running)
		output_stop();

	if (context.output) {
		obs_output_release(context.output);
		context.output = nullptr;
	}

	if (context.settings) {
		obs_data_release(context.settings);
		context.settings = nullptr;
	}
	obs_frontend_remove_event_callback(OBSEvent, nullptr);
}

void obs_module_post_load(void)
{
	if (!obs_get_module("win-asio"))
		return;

	context.settings = load_settings();

	obs_output_t *const output = obs_output_create("asio_output", "asio_output", context.settings, NULL);

	if (output != nullptr) {
		context.enabled = true;
		context.output = output;

		if (!context.settings) {
			context.settings = obs_output_get_settings(output);
			save_default_settings(context.settings);
		}
		addOutputUI();
		obs_frontend_add_event_callback(OBSEvent, nullptr);
	} else {
		blog(LOG_INFO, "Failed to create ASIO output");
		// we add the UI even if there is no output to display a text saying ASIO is disabled
		addOutputUI();
	}
}
