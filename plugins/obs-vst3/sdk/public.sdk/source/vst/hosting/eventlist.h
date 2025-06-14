//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/eventlist.h
// Created by  : Steinberg, 03/05/2008.
// Description : VST 3 event list implementation
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstevents.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Example implementation of IEventList.
\ingroup sdkBase
*/
class EventList : public IEventList
{
public:
	EventList (int32 maxSize = 50);
	virtual ~EventList ();

	int32 PLUGIN_API getEventCount () SMTG_OVERRIDE { return fillCount; }
	tresult PLUGIN_API getEvent (int32 index, Event& e) SMTG_OVERRIDE;
	tresult PLUGIN_API addEvent (Event& e) SMTG_OVERRIDE;

	void setMaxSize (int32 maxSize);
	void clear () { fillCount = 0; }

	Event* getEventByIndex (int32 index) const;

//------------------------------------------------------------------------
	DECLARE_FUNKNOWN_METHODS
protected:
	Event* events {nullptr};
	int32 maxSize {0};
	int32 fillCount {0};
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
