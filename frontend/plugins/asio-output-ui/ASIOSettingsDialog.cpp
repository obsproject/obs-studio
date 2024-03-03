/*  Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 *
 * This file is part of win-asio.
 * 
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later.
 */

#include "ASIOSettingsDialog.h"
#include <util/util.hpp>

extern void output_start();
extern void output_stop();
extern bool output_running;
extern std::string g_currentDeviceName;

ASIOSettingsDialog::ASIOSettingsDialog(QWidget *parent, obs_output_t *output, OBSData settings)
	: QDialog(parent),
	  ui(new Ui::Output),
	  _output(output),
	  _settings(settings),
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

	propertiesView = new OBSPropertiesView(_settings, "asio_output",
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
	obs_output_update(_output, _settings);
	SaveSettings();
	const char *dev = obs_data_get_string(_settings, "device_name");
	const std::string newDevice = (dev && *dev) ? dev : std::string{};

	const bool wasEmpty = currentDeviceName.empty();
	const bool nowEmpty = newDevice.empty();

	if (wasEmpty && !nowEmpty) {
		// None -> Valid device: start if not running
		if (!output_running)
			output_start();
	} else if (!wasEmpty && nowEmpty) {
		// Valid device -> None: stop if running
		if (output_running)
			output_stop();
	} else if (!nowEmpty && newDevice != currentDeviceName) {
		// output was already started so do nothing ...
	}

	currentDeviceName = newDevice;
	g_currentDeviceName = newDevice;
}
