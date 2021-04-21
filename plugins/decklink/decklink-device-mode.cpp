#include "decklink-device-mode.hpp"

DeckLinkDeviceMode::DeckLinkDeviceMode(IDeckLinkDisplayMode *mode, long long id)
	: id(id), mode(mode)
{
	if (mode == nullptr)
		return;

	mode->AddRef();

	decklink_string_t decklinkStringName;
	if (mode->GetName(&decklinkStringName) == S_OK)
		DeckLinkStringToStdString(decklinkStringName, name);
}

DeckLinkDeviceMode::DeckLinkDeviceMode(const std::string &name, long long id)
	: id(id), mode(nullptr), name(name)
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
	if (!mode)
		return false;

	BMDTimeValue timeValue;
	BMDTimeScale timeScale;
	if (mode->GetFrameRate(&timeValue, &timeScale) != S_OK)
		return false;

	// Calculate greatest common divisor of both values to properly compare framerates
	int decklinkGcd = std::gcd(timeScale, timeValue);
	int inputGcd = std::gcd(num, den);

	return ((timeScale / decklinkGcd) == (num / inputGcd) &&
		(timeValue / decklinkGcd) == (den / inputGcd));
}

void DeckLinkDeviceMode::SetMode(IDeckLinkDisplayMode *mode_)
{
	IDeckLinkDisplayMode *old = mode;
	if (old != nullptr)
		old->Release();

	mode = mode_;
	if (mode != nullptr)
		mode->AddRef();
}
