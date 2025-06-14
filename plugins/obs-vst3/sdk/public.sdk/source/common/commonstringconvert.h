//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : stringconvert
// Filename    : public.sdk/source/common/commonstringconvert.h
// Created by  : Steinberg, 07/2024
// Description : read file routine
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
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>

namespace Steinberg {
namespace StringConvert {

//------------------------------------------------------------------------
/**
 *  convert an UTF-8 string to an UTF-16 string
 *
 *  @param utf8Str UTF-8 string
 *
 *  @return UTF-16 string
 */
std::u16string convert (const std::string& utf8Str);

//------------------------------------------------------------------------
/**
 *  convert an UTF-16 string to an UTF-8 string
 *
 *  @param str UTF-16 string
 *
 *  @return UTF-8 string
 */
std::string convert (const std::u16string& str);

//------------------------------------------------------------------------
/**
 *  convert a ASCII string buffer to an UTF-8 string
 *
 *  @param str ASCII string buffer
 *	@param max maximum characters in str
 *
 *  @return UTF-8 string
 */
std::string convert (const char* str, uint32_t max);

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
} // namespace StringConvert
} // namespace Steinberg
