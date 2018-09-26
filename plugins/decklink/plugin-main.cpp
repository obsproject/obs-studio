#include <obs-module.h>
#include "decklink-devices.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("decklink", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Blackmagic DeckLink source";
}

extern struct obs_source_info create_decklink_source_info();
struct obs_source_info decklink_source_info;

extern struct obs_output_info create_decklink_output_info();
struct obs_output_info decklink_output_info;

bool obs_module_load(void)
{
	deviceEnum = new DeckLinkDeviceDiscovery();
	if (!deviceEnum->Init())
		return true;

	decklink_source_info = create_decklink_source_info();
	obs_register_source(&decklink_source_info);

	decklink_output_info = create_decklink_output_info();
	obs_register_output(&decklink_output_info);

	return true;
}

void obs_module_unload(void)
{
	delete deviceEnum;
}
