//------------------------------------------------------------------------
// Project     : SDK Base
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/pluginbasefwd.h
// Created by  : Steinberg, 10/2014
// Description : Forward declarations for pluginterfaces base module
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

namespace Steinberg {

class FUnknown;
class FUID;

template <typename T> class IPtr;

class ICloneable;
class IDependent;
class IUpdateHandler;

class IBStream;

struct KeyCode;

//------------------------------------------------------------------------
} // namespace Steinberg
