//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/stringconvert.cpp
// Created by  : Steinberg, 11/2014
// Description : c++11 unicode string convert functions
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "pluginterfaces/base/fplatform.h"

#if SMTG_OS_WINDOWS
#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif
#endif // SMTG_OS_WINDOWS

#include "public.sdk/source/vst/utility/stringconvert.h"
#include "public.sdk/source/common/commonstringconvert.h"

#include <codecvt>
#include <istream>
#include <locale>

namespace Steinberg {
namespace Vst {
namespace StringConvert {

//------------------------------------------------------------------------
namespace {

#if defined(_MSC_VER) && _MSC_VER >= 1900
#define USE_WCHAR_AS_UTF16TYPE
using UTF16Type = wchar_t;
#else
using UTF16Type = char16_t;
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

using Converter = std::wstring_convert<std::codecvt_utf8_utf16<UTF16Type>, UTF16Type>;

//------------------------------------------------------------------------
Converter& converter ()
{
	static Converter conv;
	return conv;
}

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
std::u16string convert (const std::string& utf8Str)
{
	return Steinberg::StringConvert::convert (utf8Str);
}

//------------------------------------------------------------------------
bool convert (const std::string& utf8Str, Steinberg::Vst::String128 str)
{
	return convert (utf8Str, str, 128);
}

//------------------------------------------------------------------------
bool convert (const std::string& utf8Str, Steinberg::Vst::TChar* str, uint32_t maxCharacters)
{
	auto ucs2 = convert (utf8Str);
	if (ucs2.length () < maxCharacters)
	{
		ucs2.copy (reinterpret_cast<char16_t*> (str), ucs2.length ());
		str[ucs2.length ()] = 0;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
std::string convert (const Steinberg::Vst::TChar* str)
{
	return converter ().to_bytes (reinterpret_cast<const UTF16Type*> (str));
}

//------------------------------------------------------------------------
std::string convert (const Steinberg::Vst::TChar* str, uint32_t max)
{
	std::string result;
	if (str)
	{
		Steinberg::Vst::TChar tmp[2] {};
		for (uint32_t i = 0; i < max; ++i, ++str)
		{
			tmp[0] = *str;
			if (tmp[0] == 0)
				break;
			result += convert (tmp);
		}
	}
	return result;
}

//------------------------------------------------------------------------
std::string convert (const std::u16string& str)
{
	return Steinberg::StringConvert::convert (str);
}

//------------------------------------------------------------------------
std::string convert (const char* str, uint32_t max)
{
	return Steinberg::StringConvert::convert (str, max);
}

//------------------------------------------------------------------------
} // StringConvert
} // Vst
} // Steinberg
