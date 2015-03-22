#pragma once

#include "platform.hpp"

#include <string>

class DeckLinkDeviceMode {
protected:
	long long            id;
	IDeckLinkDisplayMode *mode;
	std::string          name;

public:
	DeckLinkDeviceMode(IDeckLinkDisplayMode *mode, long long id);
	DeckLinkDeviceMode(const std::string& name, long long id);
	virtual ~DeckLinkDeviceMode(void);

	BMDDisplayMode GetDisplayMode(void) const;
	long long GetId(void) const;
	const std::string& GetName(void) const;
};
