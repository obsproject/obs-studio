/* SPDX-License-Identifier: MIT */
/**
	@file		memory.h
	@brief		Declares the AJAMemory class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_MEMORY_H
#define AJA_MEMORY_H

#include "ajabase/common/public.h"

/**
 *	Collection of system independent memory allocation functions.
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJAMemory
{
public:

	AJAMemory();
	virtual ~AJAMemory();
	
	/**
	 *	Allocate memory with no specific alignment requirements.
	 *
	 *	@param[in]	size	Bytes of memory to allocate.
	 *	@return				Address of allocated memory.  NULL if allocation fails.
	 */
	static void* Allocate(size_t size);

	/**
	 *	Free memory allocated using Allocate().
	 *
	 *	@param[in]	pMemory		Address of memory to free.
	 */
	static void  Free(void* pMemory);

	/**
	 *	Allocate memory aligned to alignment bytes.
	 *
	 *	@param[in]	size		Bytes of memory to allocate.
	 *	@param[in]	alignment	Alignment of allocated memory in bytes.
	 *	@return					Address of allocated memory.  NULL if allocation fails.
	 */
	static void* AllocateAligned(size_t size, size_t alignment);

	/**
	 *	Free memory allocated using AllocateAligned().
	 *
	 *	@param[in]	pMemory		Address of memory to free.
	 */
	static void  FreeAligned(void* pMemory);

	/**
	 *	Allocate memory aligned to alignment bytes.
	 *
	 *	Allocates a named system wide memory region.  If the named memory already exists that memory is
	 *	returned and size is updated to reflect the preallocated size.
	 *
	 *	@param[in,out]	size		Bytes of memory to allocate.
	 *	@param[in]		pShareName	Name of system wide memory to allocate.
	 *	@return						Address of allocated memory.  NULL if allocation fails.
	 */
	static void* AllocateShared(size_t* size, const char* pShareName);

	/**
	 *	Free memory allocated using AllocateShared().
	 *
	 *	@param[in]	pMemory		Address of memory to free.
	 */
	static void  FreeShared(void* pMemory);
};

#endif	//	AJA_MEMORY_H
