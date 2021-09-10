/* SPDX-License-Identifier: MIT */
/**
	@file		guid.h
	@brief		Generates a new, unique UUID as an STL string.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_GUID_H
#define AJA_GUID_H

#include <string>
#include "public.h"

extern "C"
{
	#if defined(AJA_WINDOWS)
        #include <rpc.h>
	#elif defined(AJA_LINUX)
		#include <stdio.h>
	#else
		#include <uuid/uuid.h>
	#endif
}

/**
 *	Generates a uuid.
 *
 *	@return		An STL string that contains the new UUID.
 */
std::string AJA_EXPORT CreateGuid (void);


#endif	//	AJA_GUID_H
