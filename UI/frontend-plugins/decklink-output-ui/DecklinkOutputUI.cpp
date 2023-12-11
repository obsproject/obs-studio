#include "DecklinkOutputUI.h"
#include <obs-module.h>
#include <util/platform.h>
#include <util/util.hpp>
#include "decklink-ui-main.h"
#include "obs-frontend-api.h"

static void SaveSettings(const char *filename, obs_data_t *settings)
{
	BPtr<char> modulePath =
		obs_module_get_config_path(obs_current_module(), "");

	os_mkdirs(modulePath);

	BPtr<char> path =
		obs_module_get_config_path(obs_current_module(), filename);

	obs_data_save_json_safe(settings, path, "tmp", "bak");
}

DecklinkOutputUI::DecklinkOutputUI(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui_Output)
{
	ui->setupUi(this);

	setSizeGripEnabled(true);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	propertiesView = nullptr;
	previewPropertiesView = nullptr;
}

void DecklinkOutputUI::ShowHideDialog()
{
	SetupPropertiesView();
	SetupPreviewPropertiesView();

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

	auto save = [](void * /* data */, obs_data_t *settings) {
		SaveSettings("decklinkOutputProps.json", settings);
	};

	propertiesView = (QWidget *)obs_frontend_generate_properties_by_type(
		settings, "decklink_output",
		(reload_cb)obs_get_output_properties, nullptr,
		(visual_update_cb)save, true);

	ui->propertiesLayout->addWidget(propertiesView);
	obs_data_release(settings);
}

void DecklinkOutputUI::SetupPreviewPropertiesView()
{
	if (previewPropertiesView)
		delete previewPropertiesView;

	obs_data_t *settings = obs_data_create();

	OBSData data = load_preview_settings();
	if (data)
		obs_data_apply(settings, data);

	auto save = [](void * /* data */, obs_data_t *settings) {
		SaveSettings("decklinkPreviewOutputProps.json", settings);
	};

	previewPropertiesView =
		(QWidget *)obs_frontend_generate_properties_by_type(
			settings, "decklink_output",
			(reload_cb)obs_get_output_properties, nullptr,
			(visual_update_cb)save, true);

	ui->previewPropertiesLayout->addWidget(previewPropertiesView);
	obs_data_release(settings);
}

void DecklinkOutputUI::on_outputButton_clicked()
{
	output_toggle();
}

void DecklinkOutputUI::OutputStateChanged(bool active)
{
	QString text;
	if (active) {
		text = QString(obs_module_text("Stop"));
	} else {
		text = QString(obs_module_text("Start"));
	}

	ui->outputButton->setChecked(active);
	ui->outputButton->setText(text);
}

void DecklinkOutputUI::on_previewOutputButton_clicked()
{
	preview_output_toggle();
}

void DecklinkOutputUI::PreviewOutputStateChanged(bool active)
{
	QString text;
	if (active) {
		text = QString(obs_module_text("Stop"));
	} else {
		text = QString(obs_module_text("Start"));
	}

	ui->previewOutputButton->setChecked(active);
	ui->previewOutputButton->setText(text);
}
