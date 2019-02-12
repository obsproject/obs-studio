#include "DecklinkOutputUI.h"
#include <obs-module.h>
#include <util/platform.h>
#include <util/util.hpp>
#include "decklink-ui-main.h"

DecklinkOutputUI::DecklinkOutputUI(QWidget *parent)
		: QDialog(parent),
		  ui(new Ui_Output)
{
	ui->setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	propertiesView = nullptr;

	connect(ui->startOutput, SIGNAL(released()), this, SLOT(StartOutput()));
	connect(ui->stopOutput, SIGNAL(released()), this, SLOT(StopOutput()));
}

void DecklinkOutputUI::ShowHideDialog()
{
	SetupPropertiesView();

	setVisible(!isVisible());
}

void DecklinkOutputUI::SetupPropertiesView()
{
	if (propertiesView)
		delete propertiesView;

	obs_data_t *settings = obs_data_create();

	OBSData data = load_settings();
	if (data)
		obs_data_apply(settings, data);

	propertiesView = new OBSPropertiesView(settings,
			"decklink_output",
			(PropertiesReloadCallback) obs_get_output_properties,
			170);

	ui->propertiesLayout->addWidget(propertiesView);
	obs_data_release(settings);

	connect(propertiesView, SIGNAL(Changed()), this, SLOT(PropertiesChanged()));
}

void DecklinkOutputUI::SaveSettings()
{
	BPtr<char> modulePath = obs_module_get_config_path(obs_current_module(), "");

	os_mkdirs(modulePath);

	BPtr<char> path = obs_module_get_config_path(obs_current_module(),
			"decklinkOutputProps.json");

	obs_data_t *settings = propertiesView->GetSettings();
	if (settings)
		obs_data_save_json_safe(settings, path, "tmp", "bak");
}

void DecklinkOutputUI::StartOutput()
{
	SaveSettings();
	output_start();
}

void DecklinkOutputUI::StopOutput()
{
	output_stop();
}

void DecklinkOutputUI::PropertiesChanged()
{
	SaveSettings();
}
