//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/vstpshpack4.h
// Created by  : Steinberg, 05/2010
// Description : This file turns 4 Bytes packing of structures on. The file 
//               pluginterfaces/base/falignpop.h is the complement to this file.
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------
#if defined __BORLANDC__ 
	#pragma -a4 
#else
	#if (_MSC_VER >= 800 && !defined(_M_I86)) || defined(_PUSHPOP_SUPPORTED)
		#pragma warning(disable:4103)
	#endif

	#pragma pack(push)
	#pragma pack(4)
#endif
