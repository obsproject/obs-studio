//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstplugview.h
// Created by  : Steinberg, 01/2009
// Description : Plug-in User Interface Extension
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
#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
//------------------------------------------------------------------------
// IParameterFinder Interface
//------------------------------------------------------------------------
/** Extension for IPlugView to find view parameters (lookup value under mouse support): Vst::IParameterFinder
\ingroup pluginGUI vst302
- [plug imp]
- [extends IPlugView]
- [released: 3.0.2]
- [optional]

It is highly recommended to implement this interface.
A host can implement important functionality when a plug-in supports this interface.

For example, all Steinberg hosts require this interface in order to support the "AI Knob".
*/
class IParameterFinder: public FUnknown
{
public:
	//------------------------------------------------------------------------
	/** Find out which parameter in plug-in view is at given position (relative to plug-in view).
	 * \note [UI-thread & (Initialized | Connected) & plugView] */
	virtual tresult PLUGIN_API findParameter (int32 xPos, int32 yPos, ParamID& resultTag /*out*/) = 0;
	//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IParameterFinder, 0x0F618302, 0x215D4587, 0xA512073C, 0x77B9D383)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
