#include <obs-module.h>

/* Defines common functions (required) */
OBS_DECLARE_MODULE()

/* Implements common ini-based locale (optional) */
OBS_MODULE_USE_DEFAULT_LOCALE("watcher", "en-US")

extern struct obs_source_info  watcher_source_info;  /* Defined in my-source.c  */

bool obs_module_load(void)
{
        obs_register_source(&watcher_source_info);
        return true;
}