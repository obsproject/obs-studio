#include <obs-module.h>

OBS_DECLARE_MODULE()

extern struct obs_source_info xshm_input;

bool obs_module_load(uint32_t obs_version)
{
    obs_register_source(&xshm_input);
    return true;
}
