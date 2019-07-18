#include "DecklinkOutputUI.h"
#include <obs-module.h>
#include <util/platform.h>
#include <util/util.hpp>
#include "decklink-ui-main.h"
#include "../../UI/obs-app.hpp"

static void DecklinkOutStarting(void *data, calldata_t *params)
{
	UNUSED_PARAMETER(params);
	DecklinkOutputUI *decklinkUI = static_cast<DecklinkOutputUI *>(data);
	QMetaObject::invokeMethod(decklinkUI, "OutputButtonSetStarted");
}

static void DecklinkOutStopping(void *data, calldata_t *params)
{
	UNUSED_PARAMETER(params);
	DecklinkOutputUI *decklinkUI = static_cast<DecklinkOutputUI *>(data);
	QMetaObject::invokeMethod(decklinkUI, "OutputButtonSetStopped");
}

static void DecklinkPreviewOutStarting(void *data, calldata_t *params)
{
	UNUSED_PARAMETER(params);
	DecklinkOutputUI *decklinkUI = static_cast<DecklinkOutputUI *>(data);
	QMetaObject::invokeMethod(decklinkUI, "PreviewOutputButtonSetStarted");
}

static void DecklinkPreviewOutStopping(void *data, calldata_t *params)
{
	UNUSED_PARAMETER(params);
	DecklinkOutputUI *decklinkUI = static_cast<DecklinkOutputUI *>(data);
	QMetaObject::invokeMethod(decklinkUI, "PreviewOutputButtonSetStopped");
}

DecklinkOutputUI::DecklinkOutputUI(QWidget *parent)
	: QDialog(parent), ui(new Ui_Output)
{
	ui->setupUi(this);

	setSizeGripEnabled(true);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	propertiesView = nullptr;
	previewPropertiesView = nullptr;

	connect(ui->startOutput, SIGNAL(released()), this,
		SLOT(ToggleOutput()));

	connect(ui->startPreviewOutput, SIGNAL(released()), this,
		SLOT(TogglePreviewOutput()));

	OBSData settings = load_settings();

	if (settings) {
		output_main = obs_output_create(
			"decklink_output", "decklink_output", settings, NULL);
		obs_data_release(settings);
	}

	OBSData previewSettings = load_preview_settings();

	if (previewSettings) {
		context.output = obs_output_create("decklink_preview_output",
						   "decklink_preview_output",
						   previewSettings, NULL);
		obs_data_release(previewSettings);
	}

	decklinkOutStart.Connect(obs_output_get_signal_handler(output_main),
				 "starting", DecklinkOutStarting, this);
	decklinkOutStop.Connect(obs_output_get_signal_handler(output_main),
				"stopping", DecklinkOutStopping, this);

	decklinkPreviewOutStart.Connect(
		obs_output_get_signal_handler(context.output), "starting",
		DecklinkPreviewOutStarting, this);
	decklinkPreviewOutStop.Connect(
		obs_output_get_signal_handler(context.output), "stopping",
		DecklinkPreviewOutStopping, this);
}

DecklinkOutputUI::~DecklinkOutputUI()
{
	decklinkOutStart.Disconnect();
	decklinkOutStop.Disconnect();
	decklinkPreviewOutStart.Disconnect();
	decklinkPreviewOutStop.Disconnect();
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

	propertiesView = new OBSPropertiesView(
		settings, "decklink_output",
		(PropertiesReloadCallback)obs_get_output_properties, 170);

	ui->propertiesLayout->addWidget(propertiesView);
	obs_data_release(settings);

	connect(propertiesView, SIGNAL(Changed()), this,
		SLOT(PropertiesChanged()));
}

void DecklinkOutputUI::SaveSettings()
{
	BPtr<char> modulePath =
		obs_module_get_config_path(obs_current_module(), "");

	os_mkdirs(modulePath);

	BPtr<char> path = obs_module_get_config_path(
		obs_current_module(), "decklinkOutputProps.json");

	obs_data_t *settings = propertiesView->GetSettings();
	if (settings)
		obs_data_save_json_safe(settings, path, "tmp", "bak");
}

void DecklinkOutputUI::SetupPreviewPropertiesView()
{
	if (previewPropertiesView)
		delete previewPropertiesView;

	obs_data_t *settings = obs_data_create();

	OBSData data = load_preview_settings();
	if (data)
		obs_data_apply(settings, data);

	previewPropertiesView = new OBSPropertiesView(
		settings, "decklink_output",
		(PropertiesReloadCallback)obs_get_output_properties, 170);

	ui->previewPropertiesLayout->addWidget(previewPropertiesView);
	obs_data_release(settings);

	connect(previewPropertiesView, SIGNAL(Changed()), this,
		SLOT(PreviewPropertiesChanged()));
}

void DecklinkOutputUI::SavePreviewSettings()
{
	char *modulePath = obs_module_get_config_path(obs_current_module(), "");

	os_mkdirs(modulePath);

	char *path = obs_module_get_config_path(
		obs_current_module(), "decklinkPreviewOutputProps.json");

	obs_data_t *settings = previewPropertiesView->GetSettings();
	if (settings)
		obs_data_save_json_safe(settings, path, "tmp", "bak");
}

void DecklinkOutputUI::ToggleOutput()
{
	if (!main_output_running)
		StartOutput();
	else
		StopOutput();
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

void DecklinkOutputUI::OutputButtonSetStarted()
{
	ui->startOutput->setText(QTStr("Stop"));
	ui->startOutput->setChecked(true);
}

void DecklinkOutputUI::OutputButtonSetStopped()
{
	ui->startOutput->setText(QTStr("Start"));
	ui->startOutput->setChecked(false);
}

void DecklinkOutputUI::PropertiesChanged()
{
	SaveSettings();
}

void DecklinkOutputUI::TogglePreviewOutput()
{
	if (!preview_output_running)
		StartPreviewOutput();
	else
		StopPreviewOutput();
}

void DecklinkOutputUI::StartPreviewOutput()
{
	SavePreviewSettings();
	preview_output_start();
}

void DecklinkOutputUI::StopPreviewOutput()
{
	preview_output_stop();
}

void DecklinkOutputUI::PreviewOutputButtonSetStarted()
{
	ui->startPreviewOutput->setText(QTStr("Stop"));
	ui->startPreviewOutput->setChecked(true);
}

void DecklinkOutputUI::PreviewOutputButtonSetStopped()
{
	ui->startPreviewOutput->setText(QTStr("Start"));
	ui->startPreviewOutput->setChecked(false);
}

void DecklinkOutputUI::PreviewPropertiesChanged()
{
	SavePreviewSettings();
}
