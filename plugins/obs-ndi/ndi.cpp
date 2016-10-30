#include <obs-module.h>
#include "ndi.hpp"
#include "ndi-finder.hpp"
#include "ndi-receiver.hpp"

std::string ndibridge_path;

extern "C" {

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("ndi", "en-US")

bool obs_module_load(void)
{
	ndibridge_path = "/usr/local/bin/ndibridge";
	GetGlobalFinder();
	NDIOutputRegister();
	NDISourceRegister();
	return true;
}

void obs_module_unload(void)
{
	GlobalFinderDestroy();
	GlobalReceiverDestroy();
}

}
