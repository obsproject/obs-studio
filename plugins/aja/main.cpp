#include "aja-card-manager.hpp"
#include <obs-module.h>

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
	aja::CardManager::Instance().EnumerateCards();

	aja_source_info = create_aja_source_info();
	obs_register_source(&aja_source_info);

	aja_output_info = create_aja_output_info();
	obs_register_output(&aja_output_info);

	return true;
}

void obs_module_unload(void)
{
	aja::CardManager::Instance().ClearCardEntries();
}
