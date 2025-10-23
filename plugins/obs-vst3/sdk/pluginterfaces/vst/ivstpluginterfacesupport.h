//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstpluginterfacesupport.h
// Created by  : Steinberg, 11/2018
// Description : VST Interfaces
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
namespace Steinberg {
namespace Vst {
//------------------------------------------------------------------------
/** Host callback interface for an edit controller: Vst::IPlugInterfaceSupport
\ingroup vstIHost vst3612
- [host imp]
- [released: 3.6.12]
- [optional]

Allows a plug-in to ask the host if a given plug-in interface is supported/used by the host.
It is implemented by the hostContext given when the component is initialized.

\section IPlugInterfaceSupportExample Example

\code{.cpp}
//------------------------------------------------------------------------
tresult PLUGIN_API MyPluginController::initialize (FUnknown* context)
{
	// ...
	FUnknownPtr<IPlugInterfaceSupport> plugInterfaceSupport (context);
	if (plugInterfaceSupport)
	{
		if (plugInterfaceSupport->isPlugInterfaceSupported (IMidiMapping::iid) == kResultTrue)
			// IMidiMapping is used by the host
	}
	// ...
}
\endcode
\see IPluginBase
*/
class IPlugInterfaceSupport : public FUnknown
{
public:
	/** Returns kResultTrue if the associated interface to the given _iid is supported/used by the host. */
	virtual tresult PLUGIN_API isPlugInterfaceSupported (const TUID _iid) = 0;
	
 //------------------------------------------------------------------------
 static const FUID iid;
};

DECLARE_CLASS_IID (IPlugInterfaceSupport, 0x4FB58B9E, 0x9EAA4E0F, 0xAB361C1C, 0xCCB56FEA)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
