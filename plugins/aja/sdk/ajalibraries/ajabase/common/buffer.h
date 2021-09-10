/* SPDX-License-Identifier: MIT */
/**
	@file		buffer.h
	@brief		Implementation of AJABuffer class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_BUFFER_H
#define AJA_BUFFER_H

#include "public.h"

/**
 *	Class that represents a memory buffer.
 */
class AJA_EXPORT AJABuffer
{
public:
	AJABuffer();
	virtual ~AJABuffer();

	/**
	 *	Attach a preallocated buffer.
	 *
	 *	@param[in]	pBuffer				Address of the memory to attach.
	 *	@param[in]	size				Size of the memory region.
	 *	@return		AJA_STATUS_SUCCESS	Buffer successfully attached
	 *				AJA_STATUS_FAIL	Attach failed
	 */
	AJAStatus AttachBuffer(uint8_t* pBuffer, size_t size);

	/**
	 *	Allocate memory from the system.
	 *
	 *	@param[in]	size				Size of the memory buffer.
	 *	@param[in]	alignment			Memory buffer alignment in bytes.
	 *	@param[in]	pName				Name of shared memory buffer to attach.
	 *	@return		AJA_STATUS_SUCCESS	Buffer successfully allocated
	 *				AJA_STATUS_FAIL	Allocation failed
	 */
	AJAStatus AllocateBuffer(size_t size, size_t alignment = 0, char* pName = NULL);

	/**
	 *	Free allocated memory back to the system
	 *
	 *	@return		AJA_STATUS_SUCCESS	Buffer successfully freed
	 *				AJA_STATUS_FAIL		Free failed
	 */
	AJAStatus FreeBuffer();

	/**
	 *	Get the buffer address.
	 *
	 *	@return		Address of the buffer.
	 */
	uint8_t* GetBuffer();

	uint32_t* GetUINT32Buffer();

	/**
	 *	Get the buffer size.
	 *
	 *	@return		Size of the buffer.
	 */
	size_t GetBufferSize();

	/**
	 *	Get the buffer alignment.
	 *
	 *	@return		Alignment of the buffer in bytes.
	 */
	size_t GetBufferAlignment();

	/**
	 *	Get the name of the shared buffer.
	 *
	 *	@return		Name of the shared buffer, NULL if not shared.
	 */
	const char* GetBufferName();

	/**
	 *	Set the offset of the data in the buffer.
	 *
	 *	@param[in]	offset					Offset of the data.
	 *	@return		AJA_STATUS_SUCCESS		Data offset set
	 *				AJA_STATUS_RANGE		Data offset + data size > buffer size
	 */
	AJAStatus SetDataOffset(size_t offset);

	/**
	 *	Get the offset of the data in the buffer.
	 *
	 *	@return		Offset of the data.
	 */
	size_t GetDataOffset();

	/**
	 *	Set the size of the data region in the buffer
	 *
	 *	@param[in]	size					Size of the data.
	 *	@return		AJA_STATUS_SUCCESS		Size set
	 *				AJA_STATUS_RANGE		Data offset + data size > buffer size
	 */
	AJAStatus SetDataSize(size_t size);

	/**
	 *	Get the size of the data region in the buffer.
	 *
	 *	@return		Size of the data.
	 */
	size_t GetDataSize();

	/**
	 *	Get the address of the data region in the buffer.
	 *
	 *	@return		Address of the data.
	 */
	uint8_t* GetData();

private:

	void ComputeAlignment();

	uint8_t* mpAllocate;
	size_t mAllocateSize;
	size_t mAllocateAlignment;
	char* mpAllocateName;

	uint8_t* mpBuffer;
	size_t mBufferSize;
	size_t mBufferAlignment;

	size_t mDataOffset;
	size_t mDataSize;
};

#endif

