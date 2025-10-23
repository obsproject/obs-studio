//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/thread/include/flock.h
// Created by  : Steinberg, 1995
// Description : locks
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------------
/** @file base/thread/include/flock.h
    Locks. */
/** @defgroup baseLocks Locks */
//----------------------------------------------------------------------------------

#pragma once

#include "base/source/fobject.h"
#include "pluginterfaces/base/ftypes.h"

#if SMTG_PTHREADS
#include <pthread.h>

#elif SMTG_OS_WINDOWS
struct CRITSECT							// CRITICAL_SECTION
{
	void* DebugInfo;					// PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
	Steinberg::int32 LockCount;			// LONG LockCount;
	Steinberg::int32 RecursionCount;	// LONG RecursionCount;
	void* OwningThread;					// HANDLE OwningThread
	void* LockSemaphore;				// HANDLE LockSemaphore
	Steinberg::int32 SpinCount;			// ULONG_PTR SpinCount
};
#endif

namespace Steinberg {
namespace Base {
namespace Thread {

//------------------------------------------------------------------------
/** Lock interface declaration.
@ingroup baseLocks	*/
//------------------------------------------------------------------------
struct ILock
{
//------------------------------------------------------------------------
	virtual ~ILock () {}

	/** Enables lock. */
	virtual void lock () = 0;

	/** Disables lock. */
	virtual void unlock () = 0;

	/** Tries to disable lock. */
	virtual bool trylock () = 0;
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
/**	FLock declaration.
@ingroup baseLocks	*/
//------------------------------------------------------------------------
class FLock : public ILock
{
public:
//------------------------------------------------------------------------

	/** Lock constructor.
	 *  @param name lock name
	 */
	FLock (const char8* name = "FLock");

	/** Lock destructor. */
	~FLock () SMTG_OVERRIDE;

	//-- ILock -----------------------------------------------------------
	void lock () SMTG_OVERRIDE;
	void unlock () SMTG_OVERRIDE;
	bool trylock () SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
#if SMTG_PTHREADS
	pthread_mutex_t mutex; ///< Mutex object

#elif SMTG_OS_WINDOWS
	CRITSECT section; ///< Critical section object
#endif
};

//------------------------------------------------------------------------
/**	FLockObj declaration. Reference counted lock
@ingroup baseLocks	*/
//------------------------------------------------------------------------
class FLockObject : public FObject, public FLock
{
public:
	OBJ_METHODS (FLockObject, FObject)
};

//------------------------------------------------------------------------
/**	FGuard - automatic object for locks.
@ingroup baseLocks	*/
//------------------------------------------------------------------------
class FGuard
{
public:
//------------------------------------------------------------------------

	/** FGuard constructor.
	 *  @param _lock guard this lock
	 */
	FGuard (ILock& _lock) : lock (_lock) { lock.lock (); }

	/** FGuard destructor. */
	~FGuard () { lock.unlock (); }

//------------------------------------------------------------------------
private:
	ILock& lock; ///< guarded lock
};

//------------------------------------------------------------------------
/**	Conditional Guard - Locks only if valid lock is passed.
@ingroup baseLocks	*/
//------------------------------------------------------------------------
class FConditionalGuard
{
public:
//------------------------------------------------------------------------

	/** FConditionGuard constructor.
	 *  @param _lock guard this lock
	 */
	FConditionalGuard (FLock* _lock) : lock (_lock)
	{
		if (lock)
			lock->lock ();
	}

	/** FConditionGuard destructor. */
	~FConditionalGuard ()
	{
		if (lock)
			lock->unlock ();
	}

//------------------------------------------------------------------------
private:
	FLock* lock; ///< guarded lock
};

//------------------------------------------------------------------------
} // namespace Thread
} // namespace Base
} // namespace Steinberg
