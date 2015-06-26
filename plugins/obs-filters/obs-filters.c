#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("obs-filters", "en-US")

extern struct obs_source_info mask_filter;
extern struct obs_source_info crop_filter;
extern struct obs_source_info gain_filter;
extern struct obs_source_info color_filter;
extern struct obs_source_info scroll_filter;
extern struct obs_source_info color_key_filter;
extern struct obs_source_info sharpness_filter;
extern struct obs_source_info chroma_key_filter;
extern struct obs_source_info async_delay_filter;
extern struct obs_source_info noise_gate_filter;

bool obs_module_load(void)
{
	obs_register_source(&mask_filter);
	obs_register_source(&crop_filter);
	obs_register_source(&gain_filter);
	obs_register_source(&color_filter);
	obs_register_source(&scroll_filter);
	obs_register_source(&color_key_filter);
	obs_register_source(&sharpness_filter);
	obs_register_source(&chroma_key_filter);
	obs_register_source(&async_delay_filter);
	obs_register_source(&noise_gate_filter);
	return true;
}
