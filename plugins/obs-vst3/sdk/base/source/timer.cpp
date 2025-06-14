//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
// 
// Category    : Helpers
// Filename    : base/source/timer.cpp
// Created by  : Steinberg, 05/2006
// Description : Timer class for receiving triggers at regular intervals
// 
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "base/source/timer.h"

namespace Steinberg {
static bool timersEnabled = true;

//------------------------------------------------------------------------
DisableDispatchingTimers::DisableDispatchingTimers ()
{
	oldState = timersEnabled;
	timersEnabled = false;
}

//------------------------------------------------------------------------
DisableDispatchingTimers::~DisableDispatchingTimers ()
{
	timersEnabled = oldState;
}

//------------------------------------------------------------------------
namespace SystemTime {

//------------------------------------------------------------------------
struct ZeroStartTicks
{
	static const uint64 startTicks;

	static int32 getTicks32 ()
	{
		return static_cast<int32> (SystemTime::getTicks64 () - startTicks);
	}
};
const uint64 ZeroStartTicks::startTicks = SystemTime::getTicks64 ();

//------------------------------------------------------------------------
int32 getTicks ()
{
	return ZeroStartTicks::getTicks32 ();
}

//------------------------------------------------------------------------
} // namespace SystemTime
} // namespace Steinberg


#if SMTG_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>

#ifdef verify
#undef verify
#endif

namespace Steinberg {
namespace SystemTime {

//------------------------------------------------------------------------
struct MachTimeBase
{
private:
	struct mach_timebase_info timebaseInfo;

	MachTimeBase () { mach_timebase_info (&timebaseInfo); }

	static const MachTimeBase& instance ()
	{
		static MachTimeBase gInstance;
		return gInstance;
	}

public:
	static double getTimeNanos ()
	{
		const MachTimeBase& timeBase = instance ();
		double absTime = static_cast<double> (mach_absolute_time ());
		// nano seconds
		double d = (absTime / timeBase.timebaseInfo.denom) * timeBase.timebaseInfo.numer;
		return d;
	}
};

/*
	@return the current system time in milliseconds
*/
uint64 getTicks64 ()
{
	return static_cast<uint64> (MachTimeBase::getTimeNanos () / 1000000.);
}
//------------------------------------------------------------------------
} // namespace SystemTime

//------------------------------------------------------------------------
class MacPlatformTimer : public Timer
{
public:
	MacPlatformTimer (ITimerCallback* callback, uint32 milliseconds);
	~MacPlatformTimer ();

	void stop () override;
	bool verify () const { return platformTimer != nullptr; }

	static void timerCallback (CFRunLoopTimerRef timer, void* info);

protected:
	CFRunLoopTimerRef platformTimer;
	ITimerCallback* callback;
};

//------------------------------------------------------------------------
MacPlatformTimer::MacPlatformTimer (ITimerCallback* callback, uint32 milliseconds)
: platformTimer (nullptr), callback (callback)
{
	if (callback)
	{
		CFRunLoopTimerContext timerContext = {};
		timerContext.info = this;
		platformTimer = CFRunLoopTimerCreate (
		    kCFAllocatorDefault, CFAbsoluteTimeGetCurrent () + milliseconds * 0.001,
		    milliseconds * 0.001f, 0, 0, timerCallback, &timerContext);
		if (platformTimer)
			CFRunLoopAddTimer (CFRunLoopGetMain (), platformTimer, kCFRunLoopCommonModes);
	}
}

//------------------------------------------------------------------------
MacPlatformTimer::~MacPlatformTimer ()
{
	stop ();
}

//------------------------------------------------------------------------
void MacPlatformTimer::stop ()
{
	if (platformTimer)
	{
		CFRunLoopRemoveTimer (CFRunLoopGetMain (), platformTimer, kCFRunLoopCommonModes);
		CFRelease (platformTimer);
		platformTimer = nullptr;
	}
}

//------------------------------------------------------------------------
void MacPlatformTimer::timerCallback (CFRunLoopTimerRef, void* info)
{
	if (timersEnabled)
	{
		if (auto timer = (MacPlatformTimer*)info)
			timer->callback->onTimer (timer);
	}
}

//------------------------------------------------------------------------
Timer* Timer::create (ITimerCallback* callback, uint32 milliseconds)
{
	auto timer = NEW MacPlatformTimer (callback, milliseconds);
	if (timer->verify ())
		return timer;
	timer->release ();
	return nullptr;
}
//------------------------------------------------------------------------
} // namespace Steinberg

#elif SMTG_OS_WINDOWS

#include <windows.h>
#include <algorithm>
#include <list>

namespace Steinberg {
namespace SystemTime {

//------------------------------------------------------------------------
/*
    @return the current system time in milliseconds
*/
uint64 getTicks64 ()
{
#if defined(__MINGW32__)
	return GetTickCount ();
#else
	return GetTickCount64 ();
#endif
} // namespace SystemTime
} // namespace Steinberg

class WinPlatformTimer;
using WinPlatformTimerList = std::list<WinPlatformTimer*>;

//------------------------------------------------------------------------
// WinPlatformTimer
//------------------------------------------------------------------------
class WinPlatformTimer : public Timer
{
public:
//------------------------------------------------------------------------
	WinPlatformTimer (ITimerCallback* callback, uint32 milliseconds);
	~WinPlatformTimer () override;

	void stop () override;
	bool verify () const { return id != 0; }

//------------------------------------------------------------------------
private:
	UINT_PTR id;
	ITimerCallback* callback;

	static void addTimer (WinPlatformTimer* t);
	static void removeTimer (WinPlatformTimer* t);

	static void CALLBACK TimerProc (HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static WinPlatformTimerList* timers;
};

//------------------------------------------------------------------------
WinPlatformTimerList* WinPlatformTimer::timers = nullptr;

//------------------------------------------------------------------------
WinPlatformTimer::WinPlatformTimer (ITimerCallback* callback, uint32 milliseconds)
: callback (callback)
{
	id = SetTimer (nullptr, 0, milliseconds, TimerProc);
	if (id)
		addTimer (this);
}

//------------------------------------------------------------------------
WinPlatformTimer::~WinPlatformTimer ()
{
	stop ();
}

//------------------------------------------------------------------------
void WinPlatformTimer::addTimer (WinPlatformTimer* t)
{
	if (timers == nullptr)
		timers = NEW WinPlatformTimerList;
	timers->push_back (t);
}

//------------------------------------------------------------------------
void WinPlatformTimer::removeTimer (WinPlatformTimer* t)
{
	if (!timers)
		return;

	WinPlatformTimerList::iterator it = std::find (timers->begin (), timers->end (), t);
	if (it != timers->end ())
		timers->erase (it);
	if (timers->empty ())
	{
		delete timers;
		timers = nullptr;
	}
}

//------------------------------------------------------------------------
void WinPlatformTimer::stop ()
{
	if (!id)
		return;

	KillTimer (nullptr, id);
	removeTimer (this);
	id = 0;
}

//------------------------------------------------------------------------
void CALLBACK WinPlatformTimer::TimerProc (HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR idEvent,
                                           DWORD /*dwTime*/)
{
	if (timersEnabled && timers)
	{
		WinPlatformTimerList::const_iterator it = timers->cbegin ();
		while (it != timers->cend ())
		{
			WinPlatformTimer* timer = *it;
			if (timer->id == idEvent)
			{
				if (timer->callback)
					timer->callback->onTimer (timer);
				return;
			}
			++it;
		}
	}
}

//------------------------------------------------------------------------
Timer* Timer::create (ITimerCallback* callback, uint32 milliseconds)
{
	auto* platformTimer = NEW WinPlatformTimer (callback, milliseconds);
	if (platformTimer->verify ())
		return platformTimer;
	platformTimer->release ();
	return nullptr;
}

//------------------------------------------------------------------------
} // namespace Steinberg

#elif SMTG_OS_LINUX

#include <cassert>
#include <time.h>

namespace Steinberg {
namespace SystemTime {

//------------------------------------------------------------------------
/*
    @return the current system time in milliseconds
*/
uint64 getTicks64 ()
{
	struct timespec ts;
	clock_gettime (CLOCK_MONOTONIC, &ts);
	return static_cast<uint64> (ts.tv_sec) * 1000 + static_cast<uint64> (ts.tv_nsec) / 1000000;
}
//------------------------------------------------------------------------
} // namespace SystemTime

static CreateTimerFunc createTimerFunc = nullptr;

//------------------------------------------------------------------------
void InjectCreateTimerFunction (CreateTimerFunc f)
{
	createTimerFunc = f;
}

//------------------------------------------------------------------------
Timer* Timer::create (ITimerCallback* callback, uint32 milliseconds)
{
	if (createTimerFunc)
		return createTimerFunc (callback, milliseconds);
	return nullptr;
}

//------------------------------------------------------------------------
} // namespace Steinberg

#endif
