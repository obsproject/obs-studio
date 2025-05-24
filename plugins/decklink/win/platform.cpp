#include "../platform.hpp"

#include <util/platform.h>
#include <comdef.h>

IDeckLinkDiscovery *CreateDeckLinkDiscoveryInstance(void)
{
	IDeckLinkDiscovery *instance;
	const HRESULT result = CoCreateInstance(CLSID_CDeckLinkDiscovery, nullptr, CLSCTX_ALL, IID_IDeckLinkDiscovery,
						(void **)&instance);
	return result == S_OK ? instance : nullptr;
}

IDeckLinkIterator *CreateDeckLinkIteratorInstance(void)
{
	IDeckLinkIterator *iterator;
	const HRESULT result = CoCreateInstance(CLSID_CDeckLinkIterator, nullptr, CLSCTX_ALL, IID_IDeckLinkIterator,
						(void **)&iterator);
	return result == S_OK ? iterator : nullptr;
}

IDeckLinkVideoConversion *CreateVideoConversionInstance(void)
{
	IDeckLinkVideoConversion *conversion;
	const HRESULT result = CoCreateInstance(CLSID_CDeckLinkVideoConversion, nullptr, CLSCTX_ALL,
						IID_IDeckLinkVideoConversion, (void **)&conversion);
	return result == S_OK ? conversion : nullptr;
}

bool DeckLinkStringToStdString(decklink_string_t input, std::string &output)
{
	if (input == nullptr)
		return false;

	char *out = _com_util::ConvertBSTRToString(input);
	output = std::string(out);

	delete[] out;

	return true;
}
