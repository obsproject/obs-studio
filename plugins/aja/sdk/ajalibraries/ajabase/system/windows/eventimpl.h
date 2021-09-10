/* SPDX-License-Identifier: MIT */
/**
	@file		windows/eventimpl.h
	@brief		Declares the AJAEventImpl class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_EVENT_IMPL_H
#define AJA_EVENT_IMPL_H

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/event.h"


class AJAEventImpl
{
public:

	AJAEventImpl(bool manualReset, const std::string& name);
	virtual ~AJAEventImpl();

	AJAStatus Signal();
	AJAStatus Clear();

	AJAStatus SetState(bool signaled = true);
	AJAStatus GetState(bool* pSignaled);
	
	AJAStatus SetManualReset(bool manualReset);
	AJAStatus GetManualReset(bool* pManualReset);

	AJAStatus WaitForSignal(uint32_t timeout = 0xffffffff);

	virtual AJAStatus GetEventObject(uint64_t* pEventObject);

	HANDLE mEvent;

private:
	bool   mManualReset;
};

#endif	//	AJA_EVENT_IMPL_H
