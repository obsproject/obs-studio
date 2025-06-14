//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/typesizecheck.h
// Created by  : Steinberg, 08/2018
// Description : Compile time type size check macro
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/fplatform.h"

#if SMTG_CPP11
#define SMTG_TYPE_STATIC_CHECK(Operator, Type, Platform64Size, MacOS32Size, Win32Size,             \
                               Linux32Size)                                                        \
	namespace {                                                                                    \
	template <typename Type, size_t w, size_t x, size_t y, size_t z>                               \
	struct Operator##Check##Type                                                                   \
	{                                                                                              \
		constexpr Operator##Check##Type ()                                                         \
		{                                                                                          \
			static_assert (Operator (Type) ==                                                      \
			                   (SMTG_PLATFORM_64 ? w : SMTG_OS_MACOS ? x : SMTG_OS_LINUX ? z : y), \
			               "Struct " #Operator " error: " #Type);                                     \
		}                                                                                          \
	};                                                                                             \
	static constexpr Operator##Check##Type<Type, Platform64Size, MacOS32Size, Win32Size,           \
	                                       Linux32Size>                                            \
	    instance##Operator##Type;                                                                  \
	}

/** Check the size of a structure depending on compilation platform
 *	Used to check that structure sizes don't change between SDK releases.
 */
#define SMTG_TYPE_SIZE_CHECK(Type, Platform64Size, MacOS32Size, Win32Size, Linux32Size) \
	SMTG_TYPE_STATIC_CHECK (sizeof, Type, Platform64Size, MacOS32Size, Win32Size, Linux32Size)

/** Check the alignment of a structure depending on compilation platform
 *	Used to check that structure alignments don't change between SDK releases.
 */
#define SMTG_TYPE_ALIGN_CHECK(Type, Platform64Size, MacOS32Size, Win32Size, Linux32Size) \
	SMTG_TYPE_STATIC_CHECK (alignof, Type, Platform64Size, MacOS32Size, Win32Size, Linux32Size)

#else
// need static_assert
#define SMTG_TYPE_SIZE_CHECK(Type, Platform64Size, MacOS32Size, Win32Size, Linux32Size)
#endif
