#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_source_info test_random;
extern struct obs_source_info test_sinewave;
extern struct obs_source_info test_filter;
extern struct obs_source_info async_sync_test;
extern struct obs_source_info buffering_async_sync_test;
extern struct obs_source_info sync_video;
extern struct obs_source_info sync_audio;

bool obs_module_load(void)
{
	obs_register_source(&test_random);
	obs_register_source(&test_sinewave);
	obs_register_source(&test_filter);
	obs_register_source(&async_sync_test);
	obs_register_source(&buffering_async_sync_test);
	obs_register_source(&sync_video);
	obs_register_source(&sync_audio);
	return true;
}
