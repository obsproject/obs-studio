//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/baseidds.cpp
// Created by  : Steinberg, 01/2008
// Description : Basic Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/istringresult.h"
#include "pluginterfaces/base/ipersistent.h"


namespace Steinberg {

DEF_CLASS_IID (IString)
DEF_CLASS_IID (IStringResult)

DEF_CLASS_IID (IPersistent)
DEF_CLASS_IID (IAttributes)
DEF_CLASS_IID (IAttributes2)
//------------------------------------------------------------------------
} // namespace Steinberg
