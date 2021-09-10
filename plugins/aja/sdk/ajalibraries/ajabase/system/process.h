/* SPDX-License-Identifier: MIT */
/**
	@file		process.h
	@brief		Declares the AJAProcess class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_PROCESS_H
#define AJA_PROCESS_H

#include "ajabase/common/public.h"

class AJAProcessImpl;

/** 
 *	Collection of system independent process related functions.
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJAProcess
{
public:

	AJAProcess();
	virtual ~AJAProcess();

	/**
	 *	Get the Process ID of the current running process
	 *
	 *	@return					The process ID of the current process, expressed as a 64 bit unsigned integer
	 */
static		uint64_t			GetPid();

	/**
	 *	Determine whether the given pid represents a currently running, valid process.
	 *
	 *	@param[in]	pid			The process ID of the process in question.
	 *
	 *	@return					true if the given process is valid, otherwise false.
	 */
static		bool				IsValid(uint64_t pid);

	/**
	 *	Activate the main window associated with the given handle.
	 *
	 *	@param[in]	handle		An OS-dependent window handle (HWND on Windows)
	 *
	 *	@return					true if the given window was activated, false if it couln't be found.
	 */
static		bool				Activate(uint64_t handle);

	/**
	 *	Activate the main window associated with the given handle, alternate form.
	 *
	 *	@param[in]	pWindow		NULL terminated character string representing the window caption.
	 *							Pass * at the front to find having the tail of their titles matching, 
	 *							i.e. "* - AJA ControlPanel"
	 *
	 *	@return					true if the given window was activated, false if it couln't be found.
	 */
static		bool				Activate(const char* pWindow);

private:

	AJAProcessImpl* mpImpl;

};

#endif	//	AJA_PROCESS_H
