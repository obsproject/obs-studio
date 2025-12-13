#include "AudioCaptureToolbar.hpp"
#include "ui_device-select-toolbar.h"
#include "moc_AudioCaptureToolbar.cpp"

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

AudioCaptureToolbar::AudioCaptureToolbar(QWidget *parent, OBSSource source) : ComboSelectToolbar(parent, source) {}

void AudioCaptureToolbar::Init()
{
	delete ui->activateButton;
	ui->activateButton = nullptr;

	obs_module_t *mod = get_os_module("win-wasapi", "mac-capture", "linux-pulseaudio");
	if (!mod)
		return;

	const char *device_str = get_os_text(mod, "Device", "CoreAudio.Device", "Device");
	ui->deviceLabel->setText(device_str);

	prop_name = "device_id";

	ComboSelectToolbar::Init();
}
