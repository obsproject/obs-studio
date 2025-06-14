//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/connectionproxy.h
// Created by  : Steinberg, 04/2020
// Description : VST 3 Plug-in connection class
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstmessage.h"
#include "public.sdk/source/common/threadchecker.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Helper for creating and initializing component.
\ingroup Helper */
//------------------------------------------------------------------------
class ConnectionProxy : public IConnectionPoint
{
public:
	ConnectionProxy (IConnectionPoint* srcConnection);
	virtual ~ConnectionProxy ();

	//--- from IConnectionPoint
	tresult PLUGIN_API connect (IConnectionPoint* other) override;
	tresult PLUGIN_API disconnect (IConnectionPoint* other) override;
	tresult PLUGIN_API notify (IMessage* message) override;

	bool disconnect ();

//------------------------------------------------------------------------
	DECLARE_FUNKNOWN_METHODS
protected:
	std::unique_ptr<ThreadChecker> threadChecker {ThreadChecker::create ()};

	IPtr<IConnectionPoint> srcConnection;
	IPtr<IConnectionPoint> dstConnection;
};
}
} // namespaces
