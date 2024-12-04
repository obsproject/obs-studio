#include "WindowCaptureToolbar.hpp"
#include "ui_device-select-toolbar.h"
#include "moc_WindowCaptureToolbar.cpp"

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

WindowCaptureToolbar::WindowCaptureToolbar(QWidget *parent, OBSSource source) : ComboSelectToolbar(parent, source) {}

void WindowCaptureToolbar::Init()
{
	delete ui->activateButton;
	ui->activateButton = nullptr;

	obs_module_t *mod = get_os_module("win-capture", "mac-capture", "linux-capture");
	if (!mod)
		return;

	const char *device_str = get_os_text(mod, "WindowCapture.Window", "WindowUtils.Window", "Window");
	ui->deviceLabel->setText(device_str);

#if !defined(_WIN32) && !defined(__APPLE__) //linux
	prop_name = "capture_window";
#else
	prop_name = "window";
#endif

#ifdef __APPLE__
	is_int = true;
#endif

	ComboSelectToolbar::Init();
}
