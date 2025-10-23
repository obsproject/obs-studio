//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/icloneable.h
// Created by  : Steinberg, 11/2007
// Description : Interface for object copies
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "funknown.h"

namespace Steinberg {

//------------------------------------------------------------------------
/**  Interface allowing an object to be copied.
- [plug & host imp] 
- [released: N4.12]
*/
class ICloneable : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Create exact copy of the object */
	virtual FUnknown* PLUGIN_API clone () = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (ICloneable, 0xD45406B9, 0x3A2D4443, 0x9DAD9BA9, 0x85A1454B)

//------------------------------------------------------------------------
} // namespace Steinberg
