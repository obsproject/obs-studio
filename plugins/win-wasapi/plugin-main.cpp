#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-wasapi", "en-US")

void RegisterWASAPIInput();
void RegisterWASAPIOutput();

bool obs_module_load(void)
{
	RegisterWASAPIInput();
	RegisterWASAPIOutput();
	return true;
}
