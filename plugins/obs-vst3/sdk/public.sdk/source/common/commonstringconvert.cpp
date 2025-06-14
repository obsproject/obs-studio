//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/common/commonstringconvert.cpp
// Created by  : Steinberg, 07/2024
// Description : c++11 unicode string convert functions
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2024, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "commonstringconvert.h"

#include <codecvt>
#include <istream>
#include <locale>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace StringConvert {

//------------------------------------------------------------------------
namespace {

#if defined(_MSC_VER) && _MSC_VER >= 1900
#define USE_WCHAR_AS_UTF16TYPE
using UTF16Type = wchar_t;
#else
using UTF16Type = char16_t;
#endif

using Converter = std::wstring_convert<std::codecvt_utf8_utf16<UTF16Type>, UTF16Type>;

//------------------------------------------------------------------------
Converter& converter ()
{
	static Converter conv;
	return conv;
}

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
std::u16string convert (const std::string& utf8Str)
{
#if defined(USE_WCHAR_AS_UTF16TYPE)
	auto wstr = converter ().from_bytes (utf8Str);
	return {wstr.data (), wstr.data () + wstr.size ()};
#else
	return converter ().from_bytes (utf8Str);
#endif
}

//------------------------------------------------------------------------
std::string convert (const std::u16string& str)
{
	return converter ().to_bytes (reinterpret_cast<const UTF16Type*> (str.data ()),
	                              reinterpret_cast<const UTF16Type*> (str.data () + str.size ()));
}

//------------------------------------------------------------------------
std::string convert (const char* str, uint32_t max)
{
	std::string result;
	if (str)
	{
		result.reserve (max);
		for (uint32_t i = 0; i < max; ++i, ++str)
		{
			if (*str == 0)
				break;
			result += *str;
		}
	}
	return result;
}

//------------------------------------------------------------------------
} // StringConvert
} // Steinberg

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
