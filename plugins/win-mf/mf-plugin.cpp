#include <obs-module.h>
#include <util/profiler.h>

#include "mf-common.hpp"

extern "C" extern void RegisterMFAACEncoder();
extern void RegisterMFH264Encoders();


extern "C" bool obs_module_load(void)
{
	MFStartup(MF_VERSION, MFSTARTUP_FULL);

	RegisterMFAACEncoder();
	RegisterMFH264Encoders();

	return true;
}

extern "C" void obs_module_unload(void)
{
	MFShutdown();
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-mf", "en-US")
