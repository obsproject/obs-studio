/* SPDX-License-Identifier: MIT */
/**
	@file		timer.h
	@brief		Declares the AJATimer class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TIMER_H
#define AJA_TIMER_H

#include "ajabase/common/public.h"

enum AJATimerPrecision
{
    AJATimerPrecisionMilliseconds,
    AJATimerPrecisionMicroseconds,
    AJATimerPrecisionNanoseconds
};

/**
 *	Class to support timing of events
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJATimer
{
public:

    AJATimer(AJATimerPrecision precision = AJATimerPrecisionMilliseconds);
	virtual ~AJATimer();

	/**
	 *	Start the timer.
	 */
	void Start();

	/**
	 *	Stop the timer.
	 */
	void Stop();

	/**
	 *	Reset the timer.
	 */
	void Reset();

	/**
	 *	Get the elapsed time.
	 *
	 *	If the timer is running, return the elapsed time since Start() was called.  If Stop() 
	 *	has been called, return the time between Start() and Stop().
	 *
     *	@return		The elapsed time in selected timer precision units
	 */
	uint32_t ElapsedTime();

	/**
	 *	Check for timeout.
	 *
	 *	Timeout checks the ElapsedTime() and returns true if it is greater than interval.
	 *
     *	@param	interval	Timeout interval in selected timer precision units.
	 *	@return				true if elapsed time greater than interval.
	 */
	bool Timeout(uint32_t interval);

	/**
	 *	Is the timer running.
	 *
	 *	@return				true if timer is running.
	 */
	bool IsRunning(void);

    /**
     *	Return the timer precision enum.
     *
     *	@return				precision enum that was used in the constructor.
     */
    AJATimerPrecision Precision(void);

    /**
     *	Return the display string for the given timer precision enum.
     *
     *  @param	precision	The precision enum to get the display string for.
     *  @param	longName	If true the string is set to a long description, otherwise an abbreviation.
     *	@return				string description
     */
    static std::string PrecisionName(AJATimerPrecision precision, bool longName = true);

private:

    uint64_t            mStartTime;
    uint64_t            mStopTime;
    bool                mRun;
    AJATimerPrecision   mPrecision;
};

#endif	//	AJA_TIMER_H
