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

#include "ASIOSettingsDialog.h"
#include <util/util.hpp>

extern void output_start();
extern void output_stop();
extern bool output_running;
extern std::string g_currentDeviceName;

ASIOSettingsDialog::ASIOSettingsDialog(QWidget *parent, obs_output_t *output, OBSData settings)
	: QDialog(parent),
	  ui(new Ui::Output),
	  output_(output),
	  settings_(settings),
	  currentDeviceName("")
{
	ui->setupUi(this);
	setSizeGripEnabled(true);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	propertiesView = nullptr;
}

void ASIOSettingsDialog::ShowHideDialog()
{
	SetupPropertiesView();
	setVisible(!isVisible());
}

void ASIOSettingsDialog::SetupPropertiesView()
{
	if (propertiesView)
		delete propertiesView;

	propertiesView = new OBSPropertiesView(settings_, "asio_output",
					       (PropertiesReloadCallback)obs_get_output_properties, 170);

	ui->propertiesLayout->addWidget(propertiesView);
	currentDeviceName = g_currentDeviceName;

	connect(propertiesView, &OBSPropertiesView::Changed, this, &ASIOSettingsDialog::PropertiesChanged);
}

void ASIOSettingsDialog::SaveSettings()
{
	BPtr<char> modulePath = obs_module_get_config_path(obs_current_module(), "");
	os_mkdirs(modulePath);
	BPtr<char> path = obs_module_get_config_path(obs_current_module(), "asioOutputProps.json");
	obs_data_t *settings = propertiesView->GetSettings();

	if (settings)
		obs_data_save_json_safe(settings, path, "tmp", "bak");
}

void ASIOSettingsDialog::PropertiesChanged()
{
	obs_output_update(output_, settings_);
	SaveSettings();
	const char *dev = obs_data_get_string(settings_, "device_name");
	const std::string newDevice = (dev && *dev) ? dev : std::string{};

	const bool wasEmpty = currentDeviceName.empty();
	const bool nowEmpty = newDevice.empty();

	if (wasEmpty && !nowEmpty) {
		// No device swapped to a valid device: start if not running
		if (!output_running)
			output_start();
	} else if (!wasEmpty && nowEmpty) {
		// valid device swapped to None: stop if running
		if (output_running)
			output_stop();
	} else if (!nowEmpty && newDevice != currentDeviceName) {
		// output was already started so do nothing ...
	}

	currentDeviceName = newDevice;
	g_currentDeviceName = newDevice;
}
