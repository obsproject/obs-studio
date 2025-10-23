//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/stringconvert.h
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

#pragma once

#include "pluginterfaces/vst/vsttypes.h"
#include <string>

namespace Steinberg {
namespace Vst {
namespace StringConvert {

//------------------------------------------------------------------------
/**
 *	Forward to Steinberg::StringConvert::convert (...)
 */
std::u16string convert (const std::string& utf8Str);

//------------------------------------------------------------------------
/**
 *	Forward to Steinberg::StringConvert::convert (...)
 */
std::string convert (const std::u16string& str);

//------------------------------------------------------------------------
/**
 *  Forward to Steinberg::StringConvert::convert (...)
 */
std::string convert (const char* str, uint32_t max);

//------------------------------------------------------------------------
/**
 *  convert an UTF-8 string to an UTF-16 string buffer with max 127 characters
 *
 *  @param utf8Str UTF-8 string
 *  @param str     UTF-16 string buffer
 *
 *  @return true on success
 */
bool convert (const std::string& utf8Str, Steinberg::Vst::String128 str);

//------------------------------------------------------------------------
/**
 *  convert an UTF-8 string to an UTF-16 string buffer
 *
 *  @param utf8Str       UTF-8 string
 *  @param str           UTF-16 string buffer
 *  @param maxCharacters max characters that fit into str
 *
 *  @return true on success
 */
bool convert (const std::string& utf8Str, Steinberg::Vst::TChar* str,
                     uint32_t maxCharacters);

//------------------------------------------------------------------------
/**
 *  convert an UTF-16 string buffer to an UTF-8 string
 *
 *  @param str UTF-16 string buffer
 *
 *  @return UTF-8 string
 */
std::string convert (const Steinberg::Vst::TChar* str);

//------------------------------------------------------------------------
/**
 *  convert an UTF-16 string buffer to an UTF-8 string
 *
 *  @param str UTF-16 string buffer
 *	@param max maximum characters in str
 *
 *  @return UTF-8 string
 */
std::string convert (const Steinberg::Vst::TChar* str, uint32_t max);

//------------------------------------------------------------------------
} // StringConvert

//------------------------------------------------------------------------
inline const Steinberg::Vst::TChar* toTChar (const std::u16string& str)
{
	return reinterpret_cast<const Steinberg::Vst::TChar*> (str.data ());
}

//------------------------------------------------------------------------
/**
 *	convert a number to an UTF-16 string
 *
 *	@param value number
 *
 *	@return UTF-16 string
 */
template <typename NumberT>
std::u16string toString (NumberT value)
{
	auto u8str = std::to_string (value);
	return StringConvert::convert (u8str);
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

//------------------------------------------------------------------------
// Deprecated VST3 namespace
//------------------------------------------------------------------------
namespace VST3 {
namespace StringConvert {

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::u16string convert (const std::string& utf8Str)
{
	return Steinberg::Vst::StringConvert::convert (utf8Str);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::string convert (const std::u16string& str)
{
	return Steinberg::Vst::StringConvert::convert (str);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::string convert (const char* str, uint32_t max)
{
	return Steinberg::Vst::StringConvert::convert (str, max);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline bool convert (const std::string& utf8Str, Steinberg::Vst::String128 str)
{
	return Steinberg::Vst::StringConvert::convert (utf8Str, str);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline bool convert (const std::string& utf8Str, Steinberg::Vst::TChar* str, uint32_t maxCharacters)
{
	return Steinberg::Vst::StringConvert::convert (utf8Str, str, maxCharacters);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::string convert (const Steinberg::Vst::TChar* str)
{
	return Steinberg::Vst::StringConvert::convert (str);
}

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::StringConvert::convert (...)")
inline std::string convert (const Steinberg::Vst::TChar* str, uint32_t max)
{
	return Steinberg::Vst::StringConvert::convert (str, max);
}

//------------------------------------------------------------------------
} // StringConvert

//------------------------------------------------------------------------
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::toTChar (...)")
inline const Steinberg::Vst::TChar* toTChar (const std::u16string& str)
{
	return Steinberg::Vst::toTChar (str);
}

//------------------------------------------------------------------------
template <typename NumberT>
SMTG_DEPRECATED_MSG ("Use Steinberg::Vst::toString (...)")
std::u16string toString (NumberT value)
{
	return Steinberg::Vst::toString (value);
}

//------------------------------------------------------------------------
} // VST3
