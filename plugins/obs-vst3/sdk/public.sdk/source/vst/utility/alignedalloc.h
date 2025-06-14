//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/alignedalloc.h
// Created by  : Steinberg, 05/2023
// Description : aligned memory allocations
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ftypes.h"

#include <cstdlib>

#if __APPLE__
#include <AvailabilityMacros.h>
#endif

#ifdef _MSC_VER
#include <malloc.h>
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** aligned allocation
 *
 *	note that you need to use aligned_free to free the block of memory
 *
 *	@param numBytes		number of bytes to allocate
 *	@param alignment	alignment of memory base address.
 *		must be a power of 2 and at least as large as sizeof (void*) or zero in which it uses malloc
 *		for allocation
 *
 *	@return 			allocated memory
 */
inline void* aligned_alloc (size_t numBytes, uint32_t alignment)
{
	if (alignment == 0)
		return malloc (numBytes);
	void* data {nullptr};
#if SMTG_OS_MACOS && defined(MAC_OS_X_VERSION_MIN_REQUIRED) && \
    MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_15
	posix_memalign (&data, alignment, numBytes);
#elif defined(_MSC_VER)
	data = _aligned_malloc (numBytes, alignment);
#else
	data = std::aligned_alloc (alignment, numBytes);
#endif
	return data;
}

//------------------------------------------------------------------------
inline void aligned_free (void* addr, uint32_t alignment)
{
	if (alignment == 0)
		std::free (addr);
	else
	{
#if defined(_MSC_VER)
		_aligned_free (addr);
#else
		std::free (addr);
#endif
	}
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
