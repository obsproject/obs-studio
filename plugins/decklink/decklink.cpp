#include "decklink.hpp"
#include "decklink-device-discovery.hpp"
#include "decklink-device-instance.hpp"
#include "decklink-device-mode.hpp"

#include <util/threading.h>

DeckLink::DeckLink(obs_source_t *source, DeckLinkDeviceDiscovery *discovery_) :
	discovery(discovery_), source(source)
{
	discovery->AddCallback(DeckLink::DevicesChanged, this);
}

DeckLink::~DeckLink(void)
{
	discovery->RemoveCallback(DeckLink::DevicesChanged, this);
	Deactivate();
}

DeckLinkDevice *DeckLink::GetDevice() const
{
	return instance ? instance->GetDevice() : nullptr;
}

void DeckLink::DevicesChanged(void *param, DeckLinkDevice *device, bool added)
{
	DeckLink *decklink = reinterpret_cast<DeckLink*>(param);
	std::lock_guard<std::recursive_mutex> lock(decklink->deviceMutex);

	obs_source_update_properties(decklink->source);

	if (added && !decklink->instance) {
		const char *hash;
		long long mode;
		obs_data_t *settings;

		settings = obs_source_get_settings(decklink->source);
		hash = obs_data_get_string(settings, "device_hash");
		mode = obs_data_get_int(settings, "mode_id");
		obs_data_release(settings);

		if (device->GetHash().compare(hash) == 0) {
			if (!decklink->activateRefs)
				return;
			if (decklink->Activate(device, mode))
				os_atomic_dec_long(&decklink->activateRefs);
		}

	} else if (!added && decklink->instance) {
		if (decklink->instance->GetDevice() == device) {
			os_atomic_inc_long(&decklink->activateRefs);
			decklink->Deactivate();
		}
	}
}

bool DeckLink::Activate(DeckLinkDevice *device, long long modeId)
{
	std::lock_guard<std::recursive_mutex> lock(deviceMutex);
	DeckLinkDevice *curDevice = GetDevice();
	const bool same = device == curDevice;
	const bool isActive = instance != nullptr;

	if (same) {
		if (!isActive)
			return false;
		if (instance->GetActiveModeId() == modeId &&
		    instance->GetActivePixelFormat() == pixelFormat &&
		    instance->GetActiveChannelFormat() == channelFormat)
			return false;
	}

	if (isActive)
		instance->StopCapture();

	if (!same)
		instance.Set(new DeckLinkDeviceInstance(this, device));

	if (instance == nullptr)
		return false;

	DeckLinkDeviceMode *mode = GetDevice()->FindMode(modeId);
	if (mode == nullptr) {
		instance = nullptr;
		return false;
	}

	if (!instance->StartCapture(mode)) {
		instance = nullptr;
		return false;
	}

	os_atomic_inc_long(&activateRefs);
	SaveSettings();
	return true;
}

void DeckLink::Deactivate(void)
{
	std::lock_guard<std::recursive_mutex> lock(deviceMutex);
	if (instance)
		instance->StopCapture();
	instance = nullptr;

	os_atomic_dec_long(&activateRefs);
}

void DeckLink::SaveSettings()
{
	if (!instance)
		return;

	DeckLinkDevice *device = instance->GetDevice();
	DeckLinkDeviceMode *mode = instance->GetMode();

	obs_data_t *settings = obs_source_get_settings(source);

	obs_data_set_string(settings, "device_hash",
			device->GetHash().c_str());
	obs_data_set_string(settings, "device_name",
			device->GetDisplayName().c_str());
	obs_data_set_int(settings, "mode_id", instance->GetActiveModeId());
	obs_data_set_string(settings, "mode_name", mode->GetName().c_str());

	obs_data_release(settings);
}

obs_source_t *DeckLink::GetSource(void) const
{
	return source;
}
