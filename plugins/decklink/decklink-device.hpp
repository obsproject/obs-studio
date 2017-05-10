#pragma once

#include "decklink.hpp"
#include "decklink-device-mode.hpp"

#include <map>
#include <string>
#include <vector>

class DeckLinkDevice {
	ComPtr<IDeckLink>                         device;
	std::map<long long, DeckLinkDeviceMode *> modeIdMap;
	std::vector<DeckLinkDeviceMode *>         modes;
	std::string                               name;
	std::string                               displayName;
	std::string                               hash;
	int32_t                                   maxChannel;
	volatile long                             refCount = 1;

public:
	DeckLinkDevice(IDeckLink *device);
	~DeckLinkDevice(void);

	ULONG AddRef(void);
	ULONG Release(void);

	bool Init();

	DeckLinkDeviceMode *FindMode(long long id);
	const std::string& GetDisplayName(void);
	const std::string& GetHash(void) const;
	const std::vector<DeckLinkDeviceMode *>& GetModes(void) const;
	const std::string& GetName(void) const;
	int32_t GetMaxChannel(void) const;

	bool GetInput(IDeckLinkInput **input);

	inline bool IsDevice(IDeckLink *device_)
	{
		return device_ == device;
	}
};
