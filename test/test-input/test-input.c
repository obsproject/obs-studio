#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_source_info test_random;
extern struct obs_source_info test_sinewave;
extern struct obs_source_info test_filter;

#ifdef __APPLE__
extern struct obs_source_info osx_desktop;
#endif

bool obs_module_load(uint32_t libobs_version)
{
	obs_register_source(&test_random);
	obs_register_source(&test_sinewave);
	obs_register_source(&test_filter);

#ifdef __APPLE__
	obs_register_source(&osx_desktop);
#endif

	UNUSED_PARAMETER(libobs_version);
	return true;
}
