#include <obs-module.h>

#include "whip-output.h"
#include "whip-service.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-webrtc", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "OBS WebRTC module";
}

bool obs_module_load()
{
	rtc::InitLogger(rtc::LogLevel::Warning, [](rtc::LogLevel level, std::string message) {
		blog(level <= rtc::LogLevel::Error ? LOG_ERROR : LOG_WARNING, "[obs-webrtc] %s", message.c_str());
	});

	register_whip_output();
	register_whip_service();

	return true;
}
