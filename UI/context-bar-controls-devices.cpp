#include "context-bar-controls.hpp"
#include "context-bar-controls-devices.hpp"
#include "window-basic-main.hpp"

#include "ui_device-select-toolbar.h"

#ifdef _WIN32
#define get_os_module(win, mac, linux) obs_get_module(win)
#define get_os_text(mod, win, mac, linux) obs_module_get_locale_text(mod, win)
#elif __APPLE__
#define get_os_module(win, mac, linux) obs_get_module(mac)
#define get_os_text(mod, win, mac, linux) obs_module_get_locale_text(mod, mac)
#else
#define get_os_module(win, mac, linux) obs_get_module(linux)
#define get_os_text(mod, win, mac, linux) obs_module_get_locale_text(mod, linux)
#endif

/* ========================================================================= */

DeviceToolbarPropertiesThread::~DeviceToolbarPropertiesThread()
{
	obs_properties_destroy(props);
}

void DeviceToolbarPropertiesThread::run()
{
	props = obs_source_properties(source);
	source = nullptr;
	QMetaObject::invokeMethod(this, "Ready");
}

void DeviceToolbarPropertiesThread::Ready()
{
	OBSBasic *main = OBSBasic::Get();
	QLayoutItem *la = main->ui->emptySpace->layout()->itemAt(0);
	if (la) {
		DeviceCaptureToolbar *dct =
			qobject_cast<DeviceCaptureToolbar *>(la->widget());
		if (dct) {
			dct->SetProperties(props);
			props = nullptr;
		}
	}
}

/* ========================================================================= */

DeviceCaptureToolbar::DeviceCaptureToolbar(QWidget *parent, OBSSource source)
	: QWidget(parent),
	  weakSource(OBSGetWeakRef(source)),
	  ui(new Ui_DeviceSelectToolbar)
{
	ui->setupUi(this);

#ifndef _WIN32
	delete ui->activateButton;
	ui->activateButton = nullptr;
#endif

	setEnabled(false);

	obs_module_t *mod =
		get_os_module("win-dshow", "mac-avcapture", "linux-v4l2");
	const char *device_str = obs_module_get_locale_text(mod, "Device");
	ui->deviceLabel->setText(device_str);

	OBSBasic *main = OBSBasic::Get();
	if (!main->devicePropertiesThread ||
	    !main->devicePropertiesThread->isRunning()) {
		main->devicePropertiesThread.reset(
			new DeviceToolbarPropertiesThread(source));
		main->devicePropertiesThread->start();
	}
}

DeviceCaptureToolbar::~DeviceCaptureToolbar()
{
	delete ui;
	obs_properties_destroy(props);
}

void DeviceCaptureToolbar::UpdateActivateButtonName()
{
	obs_property_t *p = obs_properties_get(props, "activate");
	ui->activateButton->setText(obs_property_description(p));
}

extern void UpdateSourceComboToolbarProperties(QComboBox *combo,
					       OBSSource source,
					       obs_properties_t *props,
					       const char *prop_name,
					       bool is_int);
extern void UpdateSourceComboToolbarValue(QComboBox *combo, OBSSource source,
					  int idx, const char *prop_name,
					  bool is_int);

void DeviceCaptureToolbar::SetProperties(obs_properties_t *props_)
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		obs_properties_destroy(props_);
		return;
	}

#ifdef _WIN32
	prop_name = "video_device_id";
#elif __APPLE__
	prop_name = "device";
#else
	prop_name = "device_id";
#endif

	props = props_;
	UpdateSourceComboToolbarProperties(ui->device, source, props, prop_name,
					   false);
#ifdef _WIN32
	UpdateActivateButtonName();
#endif
	setEnabled(true);
}

void DeviceCaptureToolbar::on_device_currentIndexChanged(int idx)
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (idx == -1 || !source) {
		return;
	}

	UpdateSourceComboToolbarValue(ui->device, source, idx, prop_name,
				      false);
}

void DeviceCaptureToolbar::on_activateButton_clicked()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	obs_property_t *p = obs_properties_get(props, "activate");
	if (!p) {
		return;
	}

	obs_property_button_clicked(p, source.Get());
#ifdef _WIN32
	UpdateActivateButtonName();
#endif
}
