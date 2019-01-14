#pragma once

#include "decklink-device.hpp"
#include "decklink-device-discovery.hpp"

extern DeckLinkDeviceDiscovery *deviceEnum;

void fill_out_devices(obs_property_t *list);
