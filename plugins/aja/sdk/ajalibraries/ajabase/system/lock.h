/* SPDX-License-Identifier: MIT */
/**
	@file		lock.h
	@brief		Declares the AJALock class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_LOCK_H
#define AJA_LOCK_H

#include "ajabase/common/public.h"

#if defined(AJA_USE_CPLUSPLUS11)
	#include <mutex>
	#include <string>
	using std::recursive_timed_mutex;
	using std::string;
#endif

#define LOCK_TIME_INFINITE 0xffffffff


// forward declarations
class AJALockImpl;

/**
 *	Class to coordinate access to non reentrant code.
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJALock
{
public:

	/**
	 *	Constructor obtains a lockable object from the system.
	 *
	 *	Specify a name if the lock is to be shared system wide.
	 *
	 *	@param	pName	Name of a shared lock object.
	 */
	AJALock(const char* pName = NULL);
	virtual ~AJALock();

	/**
	 *	Request the lock.
	 *
	 *	@param[in]						timeout	Request timeout in milliseconds (0xffffffff infinite).
	 *	@return		AJA_STATUS_SUCCESS	Object locked
	 *				AJA_STATUS_TIMEOUT	Lock wait timeout
	 *				AJA_STATUS_OPEN		Lock not initialized
	 *				AJA_STATUS_FAIL		Lock failed
	 */
	virtual AJAStatus Lock(uint32_t timeout = LOCK_TIME_INFINITE);

	/**
	 *	Release the lock.
	 *
	 *	@return		AJA_STATUS_SUCCESS	Lock released
	 *				AJA_STATUS_OPEN		Lock not initialized
	 */
	virtual AJAStatus Unlock();

	/**
	 *	@return		True if valid (has implementation).
	 *				False if not valid.
	 */
	virtual inline bool IsValid(void) const
		{
			#if defined(AJA_USE_CPLUSPLUS11)
				return mpMutex != nullptr;
			#else
				return mpImpl != NULL;
			#endif
		}

private:
#if defined(AJA_USE_CPLUSPLUS11)
	recursive_timed_mutex* mpMutex={nullptr};
	string name;
#else
	AJALockImpl* mpImpl;
#endif
};

/**
 *	Automatically obtain and release a lock.
 */
class AJA_EXPORT AJAAutoLock
{
public:

	/**
	 *	Constructor waits for lock.
	 *
	 *	@param[in]	pLock	The required lock.
	 */
	AJAAutoLock(AJALock* pLock = NULL);

	/**
	 *	Destructor releases lock.
	 */
	virtual ~AJAAutoLock();

private:

	AJALock* mpLock;
};


#endif	//	AJA_LOCK_H
