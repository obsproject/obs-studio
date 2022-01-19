#include "aja-card-manager.hpp"
#include <obs-module.h>
#include <ajantv2/includes/ntv2devicescanner.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("aja", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "aja";
}

extern struct obs_source_info create_aja_source_info();
struct obs_source_info aja_source_info;

extern struct obs_output_info create_aja_output_info();
struct obs_output_info aja_output_info;

bool obs_module_load(void)
{
	CNTV2DeviceScanner scanner;
	auto numDevices = scanner.GetNumDevices();
	if (numDevices == 0) {
		blog(LOG_WARNING,
		     "No AJA devices found, skipping loading AJA plugin");
		return false;
	}

	aja::CardManager::Instance().EnumerateCards();

	aja_source_info = create_aja_source_info();
	obs_register_source(&aja_source_info);

	aja_output_info = create_aja_output_info();
	obs_register_output(&aja_output_info);

	return true;
}

void obs_module_post_load(void)
{
	struct calldata params = {0};
	auto cardManager = &aja::CardManager::Instance();
	auto num = cardManager->NumCardEntries();
	blog(LOG_WARNING, "aja main card manager: %lu", cardManager);
	blog(LOG_WARNING, "NUM CARDS: %lu", num);

	calldata_set_ptr(&params, "card_manager", (void *)cardManager);
	auto signal_handler = obs_get_signal_handler();
	signal_handler_signal(signal_handler, "aja_loaded", &params);
	calldata_free(&params);
}

void obs_module_unload(void)
{
	aja::CardManager::Instance().ClearCardEntries();
}
