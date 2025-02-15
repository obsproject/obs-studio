#pragma once

#include <obs.hpp>

#include <ajantv2/includes/ntv2enums.h>
namespace aja {
class CardManager;
}

static const char *kProgramPropsFilename = "ajaOutputProps.json";
static const char *kPreviewPropsFilename = "ajaPreviewOutputProps.json";
static const char *kMiscPropsFilename = "ajaMiscProps.json";

OBSData load_settings(const char *filename);
void output_toggle();
void preview_output_toggle();
void populate_misc_device_list(obs_property_t *list, aja::CardManager *cardManager, NTV2DeviceID &firstDeviceID);
void populate_multi_view_audio_sources(obs_property_t *list, NTV2DeviceID id);
bool on_misc_device_selected(void *data, obs_properties_t *props, obs_property_t *list, obs_data_t *settings);
bool on_multi_view_toggle(void *data, obs_properties_t *props, obs_property_t *list, obs_data_t *settings);
