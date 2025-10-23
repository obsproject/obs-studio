//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/futils.h
// Created by  : Steinberg, 01/2004
// Description : Basic utilities
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

namespace Steinberg {
//----------------------------------------------------------------------------
// min/max/etc. template functions
template <class T>
inline const T& Min (const T& a, const T& b)
{
	return b < a ? b : a;
}

//----------------------------------------------------------------------------
template <class T>
inline const T& Max (const T& a, const T& b)
{
	return a < b ? b : a;
}

//----------------------------------------------------------------------------
template <class T>
inline T Abs (const T& value)
{
	return (value >= (T)0) ? value : -value;
}

//----------------------------------------------------------------------------
template <class T>
inline T Sign (const T& value)
{
	return (value == (T)0) ? 0 : ((value >= (T)0) ? 1 : -1);
}

//----------------------------------------------------------------------------
template <class T>
inline T Bound (T minval, T maxval, T x)
{
	if (x < minval)
		return minval;
	if (x > maxval)
		return maxval;
	return x;
}

//----------------------------------------------------------------------------
template <class T>
void Swap (T& t1, T& t2)
{
	T tmp = t1;
	t1 = t2;
	t2 = tmp;
}

//----------------------------------------------------------------------------
template <class T>
bool IsApproximateEqual (T t1, T t2, T epsilon)
{
	if (t1 == t2)
		return true;
	T diff = t1 - t2;
	if (diff < 0.0)
		diff = -diff;
	if (diff < epsilon)
		return true;
	return false;
}

//----------------------------------------------------------------------------
template <class T>
inline T ToNormalized (const T& value, const int32 numSteps)
{
	return value / T (numSteps);
}

//----------------------------------------------------------------------------
template <class T>
inline int32 FromNormalized (const T& norm, const int32 numSteps)
{
	return Min<int32> (numSteps, int32 (norm * (numSteps + 1)));
}

//----------------------------------------------------------------------------
// Four character constant
#ifndef CCONST
#define CCONST(a, b, c, d) \
	 ((((int32) (a)) << 24) | (((int32) (b)) << 16) | (((int32) (c)) << 8) | (((int32) (d)) << 0))
#endif

//------------------------------------------------------------------------
} // namespace Steinberg
