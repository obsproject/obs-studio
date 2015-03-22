#include "../platform.hpp"

bool DeckLinkStringToStdString(decklink_string_t input, std::string& output)
{
	const CFStringRef string = static_cast<CFStringRef>(input);
	const CFIndex length = CFStringGetLength(string);
	const CFIndex maxLength = CFStringGetMaximumSizeForEncoding(length,
			kCFStringEncodingASCII) + 1;

	char * const buffer = new char[maxLength];

	const bool result = CFStringGetCString(string, buffer, maxLength,
			kCFStringEncodingASCII);

	if (result)
		output = std::string(buffer);

	delete[] buffer;
	CFRelease(string);

	return result;
}
