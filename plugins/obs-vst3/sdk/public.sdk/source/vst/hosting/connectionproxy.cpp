//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/connectionproxy.cpp
// Created by  : Steinberg, 04/2019
// Description : VST 3 Plug-in connection class
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "connectionproxy.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
IMPLEMENT_FUNKNOWN_METHODS (ConnectionProxy, IConnectionPoint, IConnectionPoint::iid)

//------------------------------------------------------------------------
ConnectionProxy::ConnectionProxy (IConnectionPoint* srcConnection)
: srcConnection (srcConnection) // share it
{
	FUNKNOWN_CTOR
}

//------------------------------------------------------------------------
ConnectionProxy::~ConnectionProxy ()
{
	FUNKNOWN_DTOR
}

//------------------------------------------------------------------------
tresult PLUGIN_API ConnectionProxy::connect (IConnectionPoint* other)
{
	if (other == nullptr)
		return kInvalidArgument;
	if (dstConnection)
		return kResultFalse;

	dstConnection = other; // share it
	tresult res = srcConnection->connect (this);
	if (res != kResultTrue)
		dstConnection = nullptr;
	return res;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ConnectionProxy::disconnect (IConnectionPoint* other)
{
	if (!other)
		return kInvalidArgument;

	if (other == dstConnection)
	{
		if (srcConnection)
			srcConnection->disconnect (this);
		dstConnection = nullptr;
		return kResultTrue;
	}

	return kInvalidArgument;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ConnectionProxy::notify (IMessage* message)
{
	if (dstConnection)
	{
		// We discard the message if we are not in the UI main thread
		if (threadChecker && threadChecker->test ())
			return dstConnection->notify (message);
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
bool ConnectionProxy::disconnect ()
{
	return disconnect (dstConnection) == kResultTrue;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg

