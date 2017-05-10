#include <sstream>

#include "decklink-device.hpp"

#include <util/threading.h>

DeckLinkDevice::DeckLinkDevice(IDeckLink *device_) : device(device_)
{
}

DeckLinkDevice::~DeckLinkDevice(void)
{
	for (DeckLinkDeviceMode *mode : modes)
		delete mode;
}

ULONG DeckLinkDevice::AddRef()
{
	return os_atomic_inc_long(&refCount);
}

ULONG DeckLinkDevice::Release()
{
	long ret = os_atomic_dec_long(&refCount);
	if (ret == 0)
		delete this;
	return ret;
}

bool DeckLinkDevice::Init()
{
	ComPtr<IDeckLinkInput> input;
	if (device->QueryInterface(IID_IDeckLinkInput, (void**)&input) != S_OK)
		return false;

	IDeckLinkDisplayModeIterator *modeIterator;
	if (input->GetDisplayModeIterator(&modeIterator) == S_OK) {
		IDeckLinkDisplayMode *displayMode;
		long long modeId = 1;

		while (modeIterator->Next(&displayMode) == S_OK) {
			if (displayMode == nullptr)
				continue;

			DeckLinkDeviceMode *mode =
				new DeckLinkDeviceMode(displayMode, modeId);
			modes.push_back(mode);
			modeIdMap[modeId] = mode;
			displayMode->Release();
			++modeId;
		}

		modeIterator->Release();
	}

	decklink_string_t decklinkModelName;
	decklink_string_t decklinkDisplayName;

	if (device->GetModelName(&decklinkModelName) != S_OK)
		return false;
	DeckLinkStringToStdString(decklinkModelName, name);

	if (device->GetDisplayName(&decklinkDisplayName) != S_OK)
		return false;
	DeckLinkStringToStdString(decklinkDisplayName, displayName);

	hash = displayName;

	ComPtr<IDeckLinkAttributes> attributes;
	const HRESULT result = device->QueryInterface(IID_IDeckLinkAttributes,
			(void **)&attributes);
	if (result != S_OK)
		return true;

	int64_t channels;
	/* Intensity Shuttle for Thunderbolt return 2; however, it supports 8 channels */
	if (name == "Intensity Shuttle Thunderbolt")
		maxChannel = 8;
	else if (attributes->GetInt(BMDDeckLinkMaximumAudioChannels, &channels) == S_OK)
		maxChannel = (int32_t)channels;
	else
		maxChannel = 2;

	/* http://forum.blackmagicdesign.com/viewtopic.php?f=12&t=33967
	 * BMDDeckLinkTopologicalID for older devices
	 * BMDDeckLinkPersistentID for newer ones */

	int64_t value;
	if (attributes->GetInt(BMDDeckLinkPersistentID,  &value) != S_OK &&
	    attributes->GetInt(BMDDeckLinkTopologicalID, &value) != S_OK)
		return true;

	std::ostringstream os;
	os << value << "_" << name;
	hash = os.str();
	return true;
}

bool DeckLinkDevice::GetInput(IDeckLinkInput **input)
{
	if (device->QueryInterface(IID_IDeckLinkInput, (void**)input) != S_OK)
		return false;
	return true;
}

DeckLinkDeviceMode *DeckLinkDevice::FindMode(long long id)
{
	return modeIdMap[id];
}

const std::string& DeckLinkDevice::GetDisplayName(void)
{
	return displayName;
}

const std::string& DeckLinkDevice::GetHash(void) const
{
	return hash;
}

const std::vector<DeckLinkDeviceMode *>& DeckLinkDevice::GetModes(void) const
{
	return modes;
}

const std::string& DeckLinkDevice::GetName(void) const
{
	return name;
}

int32_t DeckLinkDevice::GetMaxChannel(void) const
{
	return maxChannel;
}
