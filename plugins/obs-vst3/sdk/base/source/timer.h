//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
// 
// Category    : Helpers
// Filename    : base/source/timer.h
// Created by  : Steinberg, 05/2006
// Description : Timer class for receiving tiggers at regular intervals
// 
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "fobject.h"
#include <limits>

#if SMTG_CPP17
#include <functional>
#endif

namespace Steinberg {
class Timer;

//------------------------------------------------------------------------
namespace SystemTime {

uint64 getTicks64 ();

inline uint64 getTicksDuration (uint64 old, uint64 now)
{
	if (old > now)
		return (std::numeric_limits<uint64>::max) () - old + now;
	return now - old;
}

int32 getTicks (); ///< deprecated, use getTicks64 ()

//------------------------------------------------------------------------
} // namespace SystemTime

//------------------------------------------------------------------------
/** @class ITimerCallback

    Implement this callback interface to receive triggers from a timer.
    Note: This interface is intended as a mix-in class and therefore does not provide ref-counting.

    @see Timer */
class ITimerCallback
{
public:
	virtual ~ITimerCallback () {}
	/** This method is called at the end of each interval.
	    \param timer The timer which calls. */
	virtual void onTimer (Timer* timer) = 0;
};

template <typename Call>
ITimerCallback* newTimerCallback (const Call& call)
{
	struct Callback : public ITimerCallback
	{
		Callback (const Call& call) : call (call) {}
		void onTimer (Timer* timer) SMTG_OVERRIDE { call (timer); }
		Call call;
	};
	return NEW Callback (call);
}

#if SMTG_CPP17
//------------------------------------------------------------------------
/** @class TimerCallback
 *
 *  a timer callback object using a funtion for the timer call
 */
struct TimerCallback final : ITimerCallback
{
    using CallbackFunc = std::function<void (Timer*)>;

    TimerCallback (CallbackFunc&& f) : f (std::move (f)) {}
    TimerCallback (const CallbackFunc& f) : f (f) {}

	void onTimer (Timer* timer) override { f (timer); }

private:
	CallbackFunc f;
};

#endif // SMTG_CPP17

// -----------------------------------------------------------------
/** @class Timer

    Timer is a class that allows you to receive triggers at regular intervals.
    Note: The timer class is an abstract base class with (hidden) platform specific subclasses.

    Usage:
    @code
    class TimerReceiver : public FObject, public ITimerCallback
    {
        ...
        virtual void onTimer (Timer* timer)
        {
            // do stuff
        }
        ...
    };

    TimerReceiver* receiver =  new TimerReceiver ();
    Timer* myTimer = Timer::create (receiver, 100); // interval: every 100ms
  
    ...
    ...

    if (myTimer)
        myTimer->release ();
    if (receiver)
        receiver->release ();
    @endcode

    @see ITimerCallback
 */
class Timer : public FObject
{
public:
	/** Create a timer with a given interval
	    \param callback The receiver of the timer calls.
	    \param intervalMilliseconds The timer interval in milliseconds.
	    \return The created timer if any, callers owns the timer. The timer starts immediately.
	*/
	static Timer* create (ITimerCallback* callback, uint32 intervalMilliseconds);

	virtual void stop () = 0; ///< Stop the timer.
};

// -----------------------------------------------------------------
/** @class DisableDispatchingTimers

    Disables dispatching of timers for the live time of this object
*/
class DisableDispatchingTimers
{
public:
	DisableDispatchingTimers ();
	~DisableDispatchingTimers ();

private:
	bool oldState;
};

#if SMTG_OS_LINUX
using CreateTimerFunc = Timer* (*)(ITimerCallback* callback, uint32 intervalMilliseconds);
void InjectCreateTimerFunction (CreateTimerFunc f);
#endif

//------------------------------------------------------------------------
} // namespace Steinberg
