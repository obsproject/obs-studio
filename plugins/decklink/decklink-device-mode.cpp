#include "decklink-device-mode.hpp"

DeckLinkDeviceMode::DeckLinkDeviceMode(IDeckLinkDisplayMode *mode,
		long long id) : id(id), mode(mode)
{
	if (mode == nullptr)
		return;

	mode->AddRef();

	decklink_string_t decklinkStringName;
	if (mode->GetName(&decklinkStringName) == S_OK)
		DeckLinkStringToStdString(decklinkStringName, name);
}

DeckLinkDeviceMode::DeckLinkDeviceMode(const std::string& name, long long id) :
	id(id), mode(nullptr), name(name)
{
}

DeckLinkDeviceMode::~DeckLinkDeviceMode(void)
{
	if (mode != nullptr)
		mode->Release();
}

BMDDisplayMode DeckLinkDeviceMode::GetDisplayMode(void) const
{
	if (mode != nullptr)
		return mode->GetDisplayMode();

	return bmdModeUnknown;
}

long long DeckLinkDeviceMode::GetId(void) const
{
	return id;
}

const std::string& DeckLinkDeviceMode::GetName(void) const
{
	return name;
}
