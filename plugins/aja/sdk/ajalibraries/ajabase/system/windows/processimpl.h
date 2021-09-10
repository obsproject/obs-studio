/* SPDX-License-Identifier: MIT */
/**
	@file		windows/processimpl.h
	@brief		Declares the AJAProcessImpl class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_PROCESS_IMPL_H
#define AJA_PROCESS_IMPL_H

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/process.h"


class AJAProcessImpl
{
public:

						AJAProcessImpl();
virtual					~AJAProcessImpl();

static		uint64_t	GetPid();
static		bool		IsValid(uint64_t pid);

static		bool		Activate(const char* pWindow);
static		bool		Activate(uint64_t handle);
};


#endif	//	AJA_PROCESS_IMPL_H
