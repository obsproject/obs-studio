//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/eventlist.cpp
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

#include "eventlist.h"

namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
IMPLEMENT_FUNKNOWN_METHODS (EventList, IEventList, IEventList::iid)

//-----------------------------------------------------------------------------
EventList::EventList (int32 inMaxSize)
{
	FUNKNOWN_CTOR
	setMaxSize (inMaxSize);
}

//-----------------------------------------------------------------------------
EventList::~EventList ()
{
	setMaxSize (0);
	FUNKNOWN_DTOR
}

//-----------------------------------------------------------------------------
void EventList::setMaxSize (int32 newMaxSize)
{
	if (events)
	{
		delete[] events;
		events = nullptr;
		fillCount = 0;
	}
	if (newMaxSize > 0)
		events = new Event[newMaxSize];
	maxSize = newMaxSize;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API EventList::getEvent (int32 index, Event& e)
{
	if (auto event = getEventByIndex (index))
	{
		memcpy (&e, event, sizeof (Event));
		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API EventList::addEvent (Event& e)
{
	if (maxSize > fillCount)
	{
		memcpy (&events[fillCount], &e, sizeof (Event));
		fillCount++;
		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
Event* EventList::getEventByIndex (int32 index) const
{
	if (index < fillCount)
		return &events[index];
	return nullptr;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
