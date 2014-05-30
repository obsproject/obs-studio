#include <obs-module.h>

OBS_DECLARE_MODULE()

void RegisterWASAPIInput();
void RegisterWASAPIOutput();

bool obs_module_load(uint32_t libobs_ver)
{
	UNUSED_PARAMETER(libobs_ver);

	RegisterWASAPIInput();
	RegisterWASAPIOutput();
	return true;
}
