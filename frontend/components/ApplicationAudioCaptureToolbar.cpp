#include "ApplicationAudioCaptureToolbar.hpp"
#include "ui_device-select-toolbar.h"
#include "moc_ApplicationAudioCaptureToolbar.cpp"

ApplicationAudioCaptureToolbar::ApplicationAudioCaptureToolbar(QWidget *parent, OBSSource source)
	: ComboSelectToolbar(parent, source)
{
}

void ApplicationAudioCaptureToolbar::Init()
{
	delete ui->activateButton;
	ui->activateButton = nullptr;

	obs_module_t *mod = obs_get_module("win-wasapi");
	const char *device_str = obs_module_get_locale_text(mod, "Window");
	ui->deviceLabel->setText(device_str);

	prop_name = "window";

	ComboSelectToolbar::Init();
}
