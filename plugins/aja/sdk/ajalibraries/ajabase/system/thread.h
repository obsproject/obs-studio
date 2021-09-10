/* SPDX-License-Identifier: MIT */
/**
	@file		thread.h
	@brief		Declares the AJAThread class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_THREAD_H
#define AJA_THREAD_H

#include "ajabase/common/public.h"

// forward declarations
class AJAThread;
class AJAThreadImpl;


/**
 *	Template for external thread entry point.
 *	@relates AJAThread
 */
typedef void AJAThreadFunction(AJAThread* pThread, void* pContext);

/**
 *	Enumerate State of theads used to implemente state in run function.
 *  Not actually used by the thread class.
 */
enum AJAThreadState
{
    AJA_ThreadState_Running,
    AJA_ThreadState_Idle,
    AJA_ThreadState_Shutdown
};


/**
 *	Enumerate system independent thread priorities.
 */
enum AJAThreadPriority
{
	AJA_ThreadPriority_Unknown,			/**< Priority unknown, used as default. */	
	AJA_ThreadPriority_Low,				/**< Below normal priority. */
	AJA_ThreadPriority_Normal,			/**< Normal system priority. */
	AJA_ThreadPriority_High,			/**< Above normal priority. */
	AJA_ThreadPriority_TimeCritical,	/**< Priority designate for time critical tasks */
	AJA_ThreadPriority_AboveNormal      /**< Priority between normal and high           */
};


enum AJAThreadRealTimePolicy
{
    AJA_ThreadRealTimePolicyFIFO,
    AJA_ThreadRealTimePolicyRoundRobin
};


/**
 *	System independent class for creating and controlling threads.
 *	@ingroup AJAGroupSystem
 *
 *	This class can be used 2 ways.  If an external thread function is attached, the Start() method creates 
 *	a thread and invokes the attached function.  It is up to the attached function to query the Terminate()
 *	method to determine when the a Stop() has been issued.  If no external thread function is attached, the 
 *	Start() method creates a thread and invokes the ThreadRun() method.  ThreadRun() calls ThreadInit() and then 
 *	calls ThreadLoop() continuously until a Stop is issued at which time if calls ThreadFlush().  A subclass
 *	can override any of the functions it requires but should at least override ThreadLoop() to do some work
 *	then wait an appropriate amount of time.
 */
class AJA_EXPORT AJAThread
{
public:

	AJAThread();
	virtual ~AJAThread();
	
	/**
	 *	Start the thread.
	 *
	 *	@return		AJA_STATUS_SUCCESS	Thread started
	 *				AJA_STATUS_FAIL		Thread start failed
	 *
	 */
	virtual AJAStatus Start();

	/**
	 *	Stop the thread.
	 *
	 *	@param[in]	timeout				Wait timeout in milliseconds (0xffffffff infinite).
	 *	@return		AJA_STATUS_SUCCESS	Thread stopped
	 *				AJA_STATUS_TIMEOUT	Thread stop timeout
	 *				AJA_STATUS_FAIL		Thread stop wait failure
	 */

	virtual AJAStatus Stop(uint32_t timeout = 0xffffffff);

	/**
	 *	Kill the thread.
	 *
	 *	@return		AJA_STATUS_SUCCESS	Thread killed
	 *				AJA_STATUS_FAIL		Thread could not be killed
	 */
	virtual AJAStatus Kill(uint32_t exitCode);

	/**
	 *	Is the thread active.
	 *
	 *	@return		true	Thread active
	 *				false	Thread inactive
	 */
	virtual bool Active();

	/**
	 *	Is this the current running thread.
	 *
	 *	@return		true	Thread current
	 *				false	Thread not current
	 */
	virtual bool IsCurrentThread();

	/**
	 *	Set the thread priority.
	 *
	 *	@param[in]	priority	New thread priority.
	 *	@return		AJA_STATUS_SUCCESS		Thread priority set
	 *				AJA_STATUS_RANGE		Unknown priority
	 *				AJA_STATUS_FAIL			Priority not set
	 */
	virtual AJAStatus SetPriority(AJAThreadPriority priority);

	/**
	 *	Get the thread priority.
	 *
	 *	@return		AJA_STATUS_SUCCESS		Priority returned
	 *				AJA_STATUS_NULL			PPriority is null
	 */
	virtual AJAStatus GetPriority(AJAThreadPriority* pPriority);

    /**
     *	Set the thread to be realtime.
     *
     *	@param[in]	AJAThreadRealTimePolicy	Thread policy
     *              int                     Realtime  priority
     *	@return		AJA_STATUS_SUCCESS		Thread priority set
     *				AJA_STATUS_RANGE		Unknown priority
     *				AJA_STATUS_FAIL			Priority not set
     */
    virtual AJAStatus SetRealTime(AJAThreadRealTimePolicy policy, int priority);

	/**
	 *	Controlling function for the new thread.
	 *
	 *	This function calls ThreadInit() once then continuously call ThreadLoop until Terminate() is
	 *	true or ThreadLoop() returns false.  ThreadFlush() is called last.  Override this function
	 *	for complete control of the thread.
	 *
	 *	@return		AJA_STATUS_SUCCESS	No thread errors
	 *				AJA_STATUS_FAIL		ThreadInit or ThreadFlush returned failure.
	 */
	virtual AJAStatus ThreadRun();

	/**
	 *	Initialize the thread resources.
	 *
	 *	Override this function to initialize thread resources before executing the inner loop.
	 *
	 *	@return		AJA_STATUS_SUCCESS	Initialize successful
	 *				AJA_STATUS_FAIL		Initialize failed
	 */
	virtual AJAStatus ThreadInit();

	/**
	 *	The thread inner loop.
	 *
	 *	Override this function to repeat a task while the thread is active.
	 *
	 *	@return		true	Loop continues
	 *				false	Loop stops
	 */
	virtual bool ThreadLoop();

	/**
	 *	Flush the thread resources.
	 *
	 *	Override this function to cleanup before the thread exits.
	 *
	 *	@return		AJA_STATUS_SUCCESS	Thread flush successful
	 *				AJA_STATUS_FAIL		Thread flush failed
	 */
	virtual AJAStatus ThreadFlush();

	/**
	 *	The thread must terminate.
	 *
	 *	Threads call this function to determine when to stop.
	 *
	 *	@return		true	Thread stop requested
	 *				false	Thread can continue
	 */
	virtual bool Terminate();

	/**
	 *	Attach an external thread function.
	 *
	 *	The external function is called when the thread starts and returns when Terminate() is true
	 *	or the task is complete.
	 *
	 *	@param[in]	pThreadFunction		External thread entry point
	 *	@param[in]	pUserContext		Context passed to external thread function.
	 *	return		AJA_STATUS_SUCCESS	External function attached
	 */
	virtual AJAStatus Attach(AJAThreadFunction* pThreadFunction, void* pUserContext);

	/**
	 *
	 * 	Sets the thread name
	 *
	 * 	NOTE: This call must be made from within the thread.
	 *
	 * 	@param[in]	name			Name to assign to the thread
	 */
	virtual AJAStatus SetThreadName(const char *name);

    /**
     *	Get the Thread Id of the current running thread
     *
     *	@return The thread Id of the current thread, expressed as a 64 bit unsigned integer
     */
    static uint64_t	GetThreadId();

private:

	AJAThreadImpl* mpImpl;
};

#endif	//	AJA_THREAD_H
