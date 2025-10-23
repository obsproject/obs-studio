//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstautomationstate.h
// Created by  : Steinberg, 02/2015
// Description : VST Automation State Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Extended plug-in interface IEditController: Vst::IAutomationState
\ingroup vstIPlug vst365
- [plug imp]
- [extends IEditController]
- [released: 3.6.5]
- [optional]

Hosts can inform the plug-in about its current automation state (Read/Write/Nothing).
*/
class IAutomationState : public FUnknown
{
public:
//------------------------------------------------------------------------
	enum AutomationStates : int32
	{
		kNoAutomation = 0,		///< Not Read and not Write
		kReadState = 1 << 0,	///< Read state
		kWriteState = 1 << 1,	///< Write state

		kReadWriteState = kReadState | kWriteState, ///< Read and Write enable
	};

	/** Sets the current Automation state.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API setAutomationState (int32 state /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IAutomationState, 0xB4E8287F, 0x1BB346AA, 0x83A46667, 0x68937BAB)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
