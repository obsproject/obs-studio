//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/pluginterfacesupport.h
// Created by  : Steinberg, 11/20018.
// Description : VST 3 hostclasses, example implementations for IPlugInterfaceSupport
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstpluginterfacesupport.h"

#include <vector>

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Example implementation of IPlugInterfaceSupport.
\ingroup hostingBase
*/
class PlugInterfaceSupport : public IPlugInterfaceSupport
{
public:
	PlugInterfaceSupport ();
	virtual ~PlugInterfaceSupport () = default;

	//--- IPlugInterfaceSupport ---------
	tresult PLUGIN_API isPlugInterfaceSupported (const TUID _iid) SMTG_OVERRIDE;

	void addPlugInterfaceSupported (const TUID _iid);
	bool removePlugInterfaceSupported (const TUID _iid);

	DECLARE_FUNKNOWN_METHODS

private:
	std::vector<FUID> mFUIDArray;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
