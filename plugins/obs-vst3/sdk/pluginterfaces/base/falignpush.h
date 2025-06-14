//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/falignpush.h
// Created by  : Steinberg, 01/2004
// Description : Set alignment settings
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
#if SMTG_OS_MACOS
	#pragma GCC diagnostic ignored "-Wunknown-warning-option"
	#pragma GCC diagnostic ignored "-Wpragma-pack"
	#if SMTG_PLATFORM_64
		#pragma pack(push, 16)
	#else
		#pragma pack(push, 1)
	#endif
#elif defined __BORLANDC__
	#pragma -a8
#elif SMTG_OS_WINDOWS
	//! @brief warning C4103: alignment changed after including header, may be due to missing #pragma pack(pop)
	#ifdef _MSC_VER
		#pragma warning(disable : 4103)
	#endif
	#pragma pack(push)
	#if SMTG_PLATFORM_64
		#pragma pack(16)
	#else
		#pragma pack(8)
	#endif
#elif SMTG_OS_LINUX
	#if SMTG_PLATFORM_64
		#pragma pack(push, 16)
	#else
		#pragma pack(push, 8)
	#endif
#endif
//----------------------------------------------------------------------------------------------
