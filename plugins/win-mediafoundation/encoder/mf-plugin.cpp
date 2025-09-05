#include "mf-common.hpp"

#include <obs-module.h>
#include <util/profiler.h>

extern void RegisterMFH264Encoders();
extern void RegisterMFHEVCEncoders();
extern void RegisterMFAV1Encoders();

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-mediafoundation-encoder", "en-US")

extern "C" bool obs_module_load(void)
{
	MFStartup(MF_VERSION, MFSTARTUP_FULL);
	RegisterMFH264Encoders();
#ifdef ENABLE_HEVC
	RegisterMFHEVCEncoders();
#endif
	RegisterMFAV1Encoders();
	return true;
}

extern "C" void obs_module_unload(void)
{
	MFShutdown();
}
