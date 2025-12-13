#include <obs-module.h>
#include <util/profiler.h>

#include "mf-common.hpp"

extern void RegisterMFH264Encoders();
extern void RegisterMFHEVCEncoders();
extern void RegisterMFAV1Encoders();

extern "C" bool obs_module_load(void)
{
	MFStartup(MF_VERSION, MFSTARTUP_FULL);
	RegisterMFH264Encoders();
	RegisterMFHEVCEncoders();
	RegisterMFAV1Encoders();
	return true;
}

extern "C" void obs_module_unload(void)
{
	MFShutdown();
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-mf", "en-US")
