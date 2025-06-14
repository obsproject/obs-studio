//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/thread/include/fcondition.h
// Created by  : Steinberg, 1995
// Description : the threads and locks and signals...
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------------
/** @file base/thread/include/fcondition.h
	signal. */
/** @defgroup baseThread Thread Handling */
//----------------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ftypes.h"

#if SMTG_PTHREADS 
#include <pthread.h>
#endif

//------------------------------------------------------------------------
namespace Steinberg {
namespace Base {
namespace Thread {

//------------------------------------------------------------------------
/**	FCondition - wraps the signal and wait calls in win32
@ingroup baseThread	*/
//------------------------------------------------------------------------
class FCondition
{
public:
//------------------------------------------------------------------------

	/** Condition constructor.
	 *  @param name name of condition
	 */
	FCondition (const char8* name = nullptr /* "FCondition" */ );

	/** Condition destructor.
	 */
	~FCondition ();
	
	/** Signals one thread.
	 */
	void signal ();

	/** Signals all threads.
	 */
	void signalAll ();

	/** Waits for condition.
	 */
	void wait ();

	/** Waits for condition with timeout
	 *  @param timeout time out in milliseconds
	 *  @return false if timed out
	 */
	bool waitTimeout (int32 timeout = -1);

	/** Resets condition.
	 */
	void reset ();

#if SMTG_OS_WINDOWS
	/** Gets condition handle.
	 *  @return handle
	 */
	void* getHandle () { return event; }
#endif

//------------------------------------------------------------------------
private:
#if SMTG_PTHREADS
	pthread_mutex_t mutex;		///< Mutex object
	pthread_cond_t cond;		///< Condition object
	int32 state;				///< Use to hold the state of the signal
	int32 waiters;				///< Number of waiters

	#if DEVELOPMENT
	int32     waits;			///< Waits count
	int32     signalCount;		///< Signals count
	#endif
#elif SMTG_OS_WINDOWS
	void* event;				///< Event handle
#endif
};

//------------------------------------------------------------------------
} // namespace Thread
} // namespace Base
} // namespace Steinberg
