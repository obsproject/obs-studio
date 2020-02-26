#include <obs-module.h>
#include "caffeine.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("caffeine", "en-US")

MODULE_EXPORT char const *obs_module_description(void)
{
	return obs_module_text("CaffeineModule");
}

extern struct obs_output_info caffeine_output_info;

/* Converts libcaffeine log levels to OBS levels */
static int caffeine_to_obs_log_level(caff_LogLevel level)
{
	switch (level) {
	case caff_LogLevelAll:
	case caff_LogLevelDebug:
		return LOG_DEBUG;
	case caff_LogLevelWarning:
		return LOG_WARNING;
	case caff_LogLevelError:
		return LOG_ERROR;
	case caff_LogLevelNone:
	default:
		/* Do not log */
		return 0;
	}
}

static void caffeine_log(caff_LogLevel level, char const *message)
{
	blog(caffeine_to_obs_log_level(level), "[libcaffeine] %s", message);
}

bool obs_module_load(void)
{
	obs_register_output(&caffeine_output_info);
	caff_Result result = caff_initialize("obs", OBS_VERSION,
					     caff_LogLevelDebug, caffeine_log);
	return result == caff_ResultSuccess;
}

void obs_module_unload(void) {}
