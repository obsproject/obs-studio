#pragma once

#if defined(_WIN32)
#include <DeckLinkAPI.h>
typedef BOOL decklink_bool_t;
typedef BSTR decklink_string_t;
IDeckLinkDiscovery *CreateDeckLinkDiscoveryInstance(void);
#define IUnknownUUID IID_IUnknown
typedef REFIID CFUUIDBytes;
#define CFUUIDGetUUIDBytes(x) x
#elif defined(__APPLE__)
#include "mac/decklink-sdk/DeckLinkAPI.h"
#include <CoreFoundation/CoreFoundation.h>
typedef bool decklink_bool_t;
typedef CFStringRef decklink_string_t;
#elif defined(__linux__)
#include "linux/decklink-sdk/DeckLinkAPI.h"
typedef bool decklink_bool_t;
typedef const char *decklink_string_t;
#endif

#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>

#include <string>

bool DeckLinkStringToStdString(decklink_string_t input, std::string& output);
