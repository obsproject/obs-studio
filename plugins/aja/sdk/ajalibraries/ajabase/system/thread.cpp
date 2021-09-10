/* SPDX-License-Identifier: MIT */
/**
	@file		thread.cpp
	@brief		Implements the AJAThread class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

// include the system dependent implementation class
#if defined(AJA_WINDOWS)
	#include "ajabase/system/windows/threadimpl.h"
#endif
#if defined(AJA_LINUX)
	#include "ajabase/system/linux/threadimpl.h"
#endif
#if defined(AJA_MAC)
	#include "ajabase/system/mac/threadimpl.h"
#endif


#include "ajabase/system/systemtime.h"


AJAThread::AJAThread()
{
	// create the implementation class
	mpImpl = new AJAThreadImpl(this);
}

AJAThread::~AJAThread()
{
	if(mpImpl)
	{
		delete mpImpl;
		mpImpl = NULL;
	}
}

AJAStatus
AJAThread::ThreadRun()
{
	AJAStatus status = AJA_STATUS_SUCCESS;
	bool loop = true;

	// initialize the thread
	status = ThreadInit();

    if (AJA_SUCCESS(status))
	{
		// call the loop until done
		while (loop && !Terminate())
		{
			loop = ThreadLoop();
		}

		// flush the thread when done
		status = ThreadFlush();
	}

	return status;
}


AJAStatus
AJAThread::ThreadInit()
{
	// default do nothing
	return AJA_STATUS_SUCCESS;
}


bool
AJAThread::ThreadLoop()
{
	// default wait a bit
	AJA_REPORT(0, AJA_DebugSeverity_Warning, "AJAThread::ThreadLoop  looping doing nothing");
	AJATime::Sleep(1000);
	return AJA_STATUS_TRUE;
}


AJAStatus
AJAThread::ThreadFlush()
{
	// default do nothing
	return AJA_STATUS_SUCCESS;
}


// interface to the implementation class

AJAStatus
AJAThread::Start()
{
	if(mpImpl)
		return mpImpl->Start();
	return AJA_STATUS_FAIL;
}


AJAStatus 
AJAThread::Stop(uint32_t timeout)
{
	if(mpImpl)
		return mpImpl->Stop(timeout);
	return AJA_STATUS_SUCCESS;
}

AJAStatus 
AJAThread::Kill(uint32_t exitCode)
{
	if(mpImpl)
		return mpImpl->Kill(exitCode);
	return AJA_STATUS_SUCCESS;
}

bool
AJAThread::Active()
{
	if(mpImpl)
		return mpImpl->Active();
	return false;
}

bool
AJAThread::IsCurrentThread()
{
	if(mpImpl)
		return mpImpl->IsCurrentThread();
	return false;
}


AJAStatus
AJAThread::SetPriority(AJAThreadPriority threadPriority)
{
	if(mpImpl)
		return mpImpl->SetPriority(threadPriority);
	return AJA_STATUS_FAIL;
}


AJAStatus 
AJAThread::GetPriority(AJAThreadPriority* pThreadPriority)
{
	if(mpImpl)
		return mpImpl->GetPriority(pThreadPriority);
	return AJA_STATUS_FAIL;
}


AJAStatus
AJAThread::SetRealTime(AJAThreadRealTimePolicy policy, int priority)
{
    if(mpImpl)
        return mpImpl->SetRealTime(policy, priority);
    return AJA_STATUS_FAIL;
}


bool 
AJAThread::Terminate()
{
	if(mpImpl)
		return mpImpl->mTerminate;
	return true;
}


AJAStatus
AJAThread::Attach(AJAThreadFunction* pThreadFunction, void* pUserContext)
{
	if(mpImpl)
		return mpImpl->Attach(pThreadFunction, pUserContext);
	return AJA_STATUS_FAIL;
}

AJAStatus AJAThread::SetThreadName(const char *name) {
	return mpImpl ? mpImpl->SetThreadName(name) : AJA_STATUS_FAIL;
}

uint64_t AJAThread::GetThreadId()
{
    return AJAThreadImpl::GetThreadId();
}
