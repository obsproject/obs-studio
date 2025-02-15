#include "aja-card-manager.hpp"
#include <obs-module.h>
#include <ajantv2/includes/ntv2devicescanner.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("aja", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "aja";
}

extern void register_aja_source_info();
extern void register_aja_output_info();

bool obs_module_load(void)
{
	CNTV2DeviceScanner scanner;
	auto numDevices = scanner.GetNumDevices();
	if (numDevices == 0) {
		blog(LOG_WARNING, "No AJA devices found, skipping loading AJA plugin");
		return false;
	}

	aja::CardManager::Instance().EnumerateCards();

	register_aja_source_info();

	register_aja_output_info();

	return true;
}

void obs_module_post_load(void)
{
	struct calldata params = {0};
	auto cardManager = &aja::CardManager::Instance();
	calldata_set_ptr(&params, "card_manager", (void *)cardManager);
	auto signal_handler = obs_get_signal_handler();
	signal_handler_signal(signal_handler, "aja_loaded", &params);
	calldata_free(&params);
}

void obs_module_unload(void)
{
	aja::CardManager::Instance().ClearCardEntries();
}
