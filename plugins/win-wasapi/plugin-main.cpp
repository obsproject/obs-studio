#include <obs-module.h>

#include <util/windows/win-version.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-wasapi", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Windows WASAPI audio input/output sources";
}

void RegisterWASAPIInput();
void RegisterWASAPIDeviceOutput();
void RegisterWASAPIProcessOutput();

bool obs_module_load(void)
{
	/* MS says 20348, but process filtering seems to work earlier */
	struct win_version_info ver;
	get_win_ver(&ver);
	struct win_version_info minimum;
	minimum.major = 10;
	minimum.minor = 0;
	minimum.build = 19041;
	minimum.revis = 0;
	const bool process_filter_supported =
		win_version_compare(&ver, &minimum) >= 0;

	RegisterWASAPIInput();
	RegisterWASAPIDeviceOutput();
	if (process_filter_supported)
		RegisterWASAPIProcessOutput();
	return true;
}
