#include "DisplayCaptureToolbar.hpp"
#include "ui_device-select-toolbar.h"
#include "moc_DisplayCaptureToolbar.cpp"

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

DisplayCaptureToolbar::DisplayCaptureToolbar(QWidget *parent, OBSSource source) : ComboSelectToolbar(parent, source) {}

void DisplayCaptureToolbar::Init()
{
	delete ui->activateButton;
	ui->activateButton = nullptr;

	obs_module_t *mod = get_os_module("win-capture", "mac-capture", "linux-capture");
	if (!mod)
		return;

	const char *device_str = get_os_text(mod, "Monitor", "DisplayCapture.Display", "Screen");
	ui->deviceLabel->setText(device_str);

#ifdef _WIN32
	prop_name = "monitor_id";
#elif __APPLE__
	prop_name = "display_uuid";
#else
	is_int = true;
	prop_name = "screen";
#endif

	ComboSelectToolbar::Init();
}
