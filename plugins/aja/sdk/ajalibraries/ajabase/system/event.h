/* SPDX-License-Identifier: MIT */
/**
	@file		event.h
	@brief		Declares the AJAEvent class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_EVENT_H
#define AJA_EVENT_H

#include "ajabase/common/public.h"
#include <string>

// forward declarations.
class AJAEventImpl;

/** 
 *	System independent event class for signaling between threads.
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJAEvent
{
public:

	friend AJA_EXPORT AJAStatus AJAWaitForEvents(AJAEvent*, uint32_t, bool, uint32_t);

	/**
	 *	Constructor obtains an event object from the system.
	 *
	 *	The event is automatically reset when the first waiting thread is released.  Set manualReset
	 *	to control the reset with the api.  Specify a name if the event is to be shared system wide.
	 *
	 *	@param[in]	manualReset		true if event must be reset by caller.
	 *								false if event automatically reset.
	 *	@param[in]	name			Name of a shared lock object.
	 */
	AJAEvent(bool manualReset = true, const std::string& name = "");
	virtual ~AJAEvent();

	/**
	* Set the event to the signaled state.
	*
	*	@return		AJA_STATUS_SUCCESS		Event signaled
	*				AJA_STATUS_FAIL			Signal failed
	*				AJA_STATUS_OPEN			Event not initialized
	*/
	virtual AJAStatus Signal();

	/**
	 *	Set the event to a non signaled state.
	 *
	 *	@return		AJA_STATUS_SUCCESS		Event cleared
	 *				AJA_STATUS_FAIL			Clear failed
	 *				AJA_STATUS_OPEN			Event not initialized
	 */
	virtual AJAStatus Clear();

	/**
	 *	Set the event to the state specified by the parameter signaled.
	 *
	 *	@param[in]	signaled				True to signal, false to clear
	 *	@return		AJA_STATUS_SUCCESS		State set
	 *				AJA_STATUS_FAIL			State not set
	 *				AJA_STATUS_OPEN			Event not initialized
	 */
	virtual AJAStatus SetState(bool signaled = true);

	/**
	 *	Get the current state of the event.
	 *
	 *	@param[out]	pSignaled				True if signaled
	 *	@return		AJA_STATUS_SUCCESS		State available
	 *				AJA_STATUS_OPEN			Event not initialized
	 *				AJA_STATUS_FAIL			Event error
	 */
	virtual AJAStatus GetState(bool* pSignaled);
	
	/**
	 *	Set the manual reset state.
	 *
	 *	@param[in]	manualReset				True to enable manual signal reset, False to enabled automatically reset of signal
	 *	@return		AJA_STATUS_SUCCESS		Manual reset state set
	 *				AJA_STATUS_FAIL			Manual reset state not set
	 */
	virtual AJAStatus SetManualReset(bool manualReset);
	
	/**
	 *	Get the manual reset state.
	 *
	 *	@param[out]	pManualReset			True indicates manually reset of signal is required, False indicates it is automatic
	 *	@return		AJA_STATUS_SUCCESS		Manual reset state initialized
	 *				AJA_STATUS_FAIL			Manual reset state not initialized
	 */
	virtual AJAStatus GetManualReset(bool* pManualReset);

	/**
	 *	Wait for the event to be signaled.
	 *
	 *	@param[in]	timeout					Wait timeout in milliseconds.
	 *	@return		AJA_STATUS_SUCCESS		Event was signaled
	 *				AJA_STATUS_TIMEOUT		Event wait timeout
	 *				AJA_STATUS_OPEN			Event not initialized
	 *				AJA_STATUS_FAIL			Event error
	 */
	virtual AJAStatus WaitForSignal(uint32_t timeout = 0xffffffff);

	/**
	 *	Get the system event object.
	 *
	 *	@param[out]	pEventObject			The system event object
	 *	@return		AJA_STATUS_SUCCESS		Event object returned
	 *				AJA_STATUS_OPEN			Event not initialized
	 */
	virtual AJAStatus GetEventObject(uint64_t* pEventObject);

private:

	AJAEventImpl* mpImpl;
};

	/**
	 *	Wait for a list of events to be signaled.
	 *	@relates AJAEvent
	 *
	 *	The wait can terminate when one or all of the events in the list is signaled.
	 *
	 *	@param[in]	pList					An array of events (AJAEventPtr).
	 *	@param[in]	numEvents				Number of events in the event array.
	 *	@param[in]	all						true to wait for all events to be signaled.
	 *	@param[in]	timeout					Wait timeout in milliseconds (0xffffffff infinite).
	 *	@return		AJA_STATUS_SUCCESS		events signaled
	 *				AJA_STATUS_TIMEOUT		event wait timeout
	 *				AJA_STATUS_OPEN			event not initialized
	 *				AJA_STATUS_RANGE		numEvents out of range
	 *				AJA_STATUS_FAIL			event error
	 */
	AJA_EXPORT AJAStatus AJAWaitForEvents(	AJAEvent* pList, 
											uint32_t numEvents, 
											bool all = true, 
											uint32_t timeout = 0xffffffff);

#endif	//	AJA_EVENT_H
