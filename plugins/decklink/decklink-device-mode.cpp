#include "decklink-device-mode.hpp"

DeckLinkDeviceMode::DeckLinkDeviceMode(IDeckLinkDisplayMode *mode, long long id)
	: id(id), mode(mode)
{
	if (mode == nullptr)
		return;

	decklink_string_t decklinkStringName;
	if (mode->GetName(&decklinkStringName) == S_OK)
		DeckLinkStringToStdString(decklinkStringName, name);
}

DeckLinkDeviceMode::DeckLinkDeviceMode(const std::string &name, long long id)
	: id(id), mode(nullptr), name(name)
{
}

DeckLinkDeviceMode::~DeckLinkDeviceMode(void) {}

BMDDisplayMode DeckLinkDeviceMode::GetDisplayMode(void) const
{
	if (mode != nullptr)
		return mode->GetDisplayMode();

	return bmdModeUnknown;
}

int DeckLinkDeviceMode::GetWidth()
{
	if (mode != nullptr)
		return mode->GetWidth();

	return 0;
}

int DeckLinkDeviceMode::GetHeight()
{
	if (mode != nullptr)
		return mode->GetHeight();

	return 0;
}

bool DeckLinkDeviceMode::GetFrameRate(BMDTimeValue *frameDuration,
				      BMDTimeScale *timeScale)
{
	if (mode != nullptr)
		return SUCCEEDED(mode->GetFrameRate(frameDuration, timeScale));

	return false;
}

BMDDisplayModeFlags DeckLinkDeviceMode::GetDisplayModeFlags(void) const
{
	if (mode != nullptr)
		return mode->GetFlags();

	return (BMDDisplayModeFlags)0;
}

long long DeckLinkDeviceMode::GetId(void) const
{
	return id;
}

const std::string &DeckLinkDeviceMode::GetName(void) const
{
	return name;
}

bool DeckLinkDeviceMode::IsEqualFrameRate(int64_t num, int64_t den)
{
	bool equal = false;

	if (mode) {
		BMDTimeValue frameDuration;
		BMDTimeScale timeScale;
		if (SUCCEEDED(mode->GetFrameRate(&frameDuration, &timeScale)))
			equal = timeScale * den == frameDuration * num;
	}

	return equal;
}

void DeckLinkDeviceMode::SetMode(IDeckLinkDisplayMode *mode_)
{
	mode = mode_;
}
