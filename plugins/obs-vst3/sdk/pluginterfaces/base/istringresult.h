//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/istringresult.h
// Created by  : Steinberg, 01/2005
// Description : Strings Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"

namespace Steinberg {

//------------------------------------------------------------------------
/** Interface to return an ascii string of variable size. 
    In order to manage memory allocation and deallocation properly, 
	this interface is used to transfer a string as result parameter of
	a method requires a string of unknown size. 
- [host imp] or [plug imp]
- [released: SX 4]
*/
class IStringResult : public FUnknown
{
public:
//------------------------------------------------------------------------
	virtual void PLUGIN_API setText (const char8* text) = 0;    

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IStringResult, 0x550798BC, 0x872049DB, 0x84920A15, 0x3B50B7A8)


//------------------------------------------------------------------------
/** Interface to a string of variable size and encoding. 
- [host imp] or [plug imp]
- [released: ] 
*/
class IString : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Assign ASCII string */
	virtual void PLUGIN_API setText8 (const char8* text) = 0;    
	/** Assign unicode string */
	virtual void PLUGIN_API setText16 (const char16* text) = 0;

	/** Return ASCII string. If the string is unicode so far, it will be converted.
	    So you need to be careful, because the conversion can result in data loss. 
		It is save though to call getText8 if isWideString() returns false */
	virtual const char8* PLUGIN_API getText8 () = 0;   
	/** Return unicode string. If the string is ASCII so far, it will be converted. */
	virtual const char16* PLUGIN_API getText16 () = 0;    

	/** !Do not use this method! Early implementations take the given pointer as 
	     internal string and this will cause problems because 'free' will be used to delete the passed memory.
		 Later implementations will redirect 'take' to setText8 and setText16 */
	virtual void PLUGIN_API take (void* s, bool isWide) = 0;

	/** Returns true if the string is in unicode format, returns false if the string is ASCII */
	virtual bool PLUGIN_API isWideString () const = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IString, 0xF99DB7A3, 0x0FC14821, 0x800B0CF9, 0x8E348EDF)

//------------------------------------------------------------------------
} // namespace Steinberg
