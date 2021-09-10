/* SPDX-License-Identifier: MIT */
/**
	@file		mac/processimpl.cpp
	@brief		Implements the AJAProcessImpl class on the Mac platform.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/mac/processimpl.h"
#include "ajabase/common/timer.h"
#include <signal.h>


// class AJAProcessImpl

AJAProcessImpl::AJAProcessImpl()
{
}


AJAProcessImpl::~AJAProcessImpl()
{
}

uint64_t
AJAProcessImpl::GetPid()
{
	return getpid();
}

bool
AJAProcessImpl::IsValid(uint64_t pid)
{
    if(kill(pid_t(pid),0)==0)
        return true;
    else
        return false;
}

bool
AJAProcessImpl::Activate(const char* pWindow)
{
    AJA_UNUSED(pWindow);
    
	//Dummy place holder
	return false;
}

bool
AJAProcessImpl::Activate(uint64_t handle)
{
    AJA_UNUSED(handle);
    
	//Dummy place holder
	return false;
}
