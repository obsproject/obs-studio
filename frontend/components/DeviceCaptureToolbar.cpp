#include "DeviceCaptureToolbar.hpp"
#include "ui_device-select-toolbar.h"
#include "moc_DeviceCaptureToolbar.cpp"

DeviceCaptureToolbar::DeviceCaptureToolbar(QWidget *parent, OBSSource source)
	: QWidget(parent),
	  weakSource(OBSGetWeakRef(source)),
	  ui(new Ui_DeviceSelectToolbar)
{
	ui->setupUi(this);

	delete ui->deviceLabel;
	delete ui->device;
	ui->deviceLabel = nullptr;
	ui->device = nullptr;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	active = obs_data_get_bool(settings, "active");

	obs_module_t *mod = obs_get_module("win-dshow");
	if (!mod)
		return;

	activateText = obs_module_get_locale_text(mod, "Activate");
	deactivateText = obs_module_get_locale_text(mod, "Deactivate");

	ui->activateButton->setText(active ? deactivateText : activateText);
}

DeviceCaptureToolbar::~DeviceCaptureToolbar() {}

void DeviceCaptureToolbar::on_activateButton_clicked()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	bool now_active = obs_data_get_bool(settings, "active");

	bool desyncedSetting = now_active != active;

	active = !active;

	const char *text = active ? deactivateText : activateText;
	ui->activateButton->setText(text);

	if (desyncedSetting) {
		return;
	}

	calldata_t cd = {};
	calldata_set_bool(&cd, "active", active);
	proc_handler_t *ph = obs_source_get_proc_handler(source);
	proc_handler_call(ph, "activate", &cd);
	calldata_free(&cd);
}
