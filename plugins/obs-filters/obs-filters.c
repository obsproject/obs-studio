#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("obs-filters", "en-US")

extern struct obs_source_info mask_filter;
extern struct obs_source_info crop_filter;
extern struct obs_source_info color_filter;
extern struct obs_source_info color_key_filter;
extern struct obs_source_info chroma_key_filter;
extern struct obs_source_info async_delay_filter;

bool obs_module_load(void)
{
	obs_register_source(&mask_filter);
	obs_register_source(&crop_filter);
	obs_register_source(&color_filter);
	obs_register_source(&color_key_filter);
	obs_register_source(&chroma_key_filter);
	obs_register_source(&async_delay_filter);
	return true;
}
