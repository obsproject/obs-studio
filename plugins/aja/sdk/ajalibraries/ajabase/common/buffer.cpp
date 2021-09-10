/* SPDX-License-Identifier: MIT */
/**
	@file		buffer.cpp
	@brief		Implementation of AJABuffer class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "common.h"
#include "buffer.h"
#include "ajabase/system/memory.h"
#include <string.h>


AJABuffer::AJABuffer()
{
	mpAllocate = NULL;
	mAllocateSize = 0;
	mAllocateAlignment = 0;

	mpBuffer = NULL;
	mBufferSize = 0;
	mBufferAlignment = 0;
	mpAllocateName = NULL;

	mDataOffset = 0;
	mDataSize = 0;
}


AJABuffer::~AJABuffer()
{
	FreeBuffer();
}


AJAStatus 
AJABuffer::AttachBuffer(uint8_t* pBuffer, size_t size)
{
	FreeBuffer();

	// use specified buffer
	mpBuffer = pBuffer;
	mBufferSize = size;

	// determine buffer alignment
	ComputeAlignment();

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJABuffer::AllocateBuffer(size_t size, size_t alignment, char* pName)
{
	FreeBuffer();

	// if shared buffer name specified then allocate a shared buffer
	if (pName != NULL && *pName != '\0')
	{
		size_t sharedSize = size;

		mpAllocate = (uint8_t*)AJAMemory::AllocateShared(&sharedSize, pName);
		if ((mpAllocate == NULL) && (sharedSize != 0))
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJABuffer::AllocateBuffer  Shared buffer allocation failed");
			return AJA_STATUS_FAIL;
		}
		mAllocateSize = sharedSize;

		mpBuffer = mpAllocate;
		mBufferSize = mAllocateSize;

		ComputeAlignment();
		
		size_t nameLen = strlen(pName);
		mpAllocateName = new char[nameLen + 1];
		AJA_ASSERT(mpAllocateName != NULL);
		strncpy(mpAllocateName, pName, nameLen);
	}
	// allocate a non shared buffer
	else
	{
		if(size == 0)
		{
			return AJA_STATUS_FAIL;
		}

		// if alignment is 0 then allocate with any alignment
		if (alignment == 0)
		{
			mpAllocate = (uint8_t*)AJAMemory::Allocate(size);
			if (mpAllocate == NULL)
			{
				AJA_REPORT(0, AJA_DebugSeverity_Error, "AJABuffer::AllocateBuffer  Buffer allocation failed");
				return AJA_STATUS_FAIL;
			}
			mAllocateSize = size;

			mpBuffer = mpAllocate;
			mBufferSize = mAllocateSize;
		}
		// if alignment is specified allocate aligned
		else
		{
			mpAllocate = (uint8_t*)AJAMemory::AllocateAligned(size, alignment);
			if (mpAllocate == NULL)
			{
				AJA_REPORT(0, AJA_DebugSeverity_Error, "AJABuffer::AllocateBuffer  Aligned buffer allocation failed");
				return AJA_STATUS_FAIL;
			}
			mAllocateSize = size;
			mAllocateAlignment = alignment;

			mpBuffer = mpAllocate;
			mBufferSize = mAllocateSize;
		}

		// determine the buffer alignment
		ComputeAlignment();

		// if alignment was specified, be sure that allocated buffer met the requirement
		if ((mAllocateAlignment != 0) && (mBufferAlignment < mAllocateAlignment))
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJABuffer::AllocateBuffer  Aligned buffer allocation failed");
			FreeBuffer();
			return AJA_STATUS_FAIL;
		}
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJABuffer::FreeBuffer()
{
	if (mpAllocate != NULL)
	{
		if (mpAllocateName != NULL)
		{
			AJAMemory::FreeShared(mpAllocateName);
		}
		else
		{
			if (mAllocateAlignment != 0)
			{
				AJAMemory::FreeAligned(mpAllocate);
			}
			else
			{
				AJAMemory::Free(mpAllocate);
			}
		}
	}

	mpAllocate = NULL;
	mAllocateSize = 0;
	mAllocateAlignment = 0;

	mpBuffer = NULL;
	mBufferSize = 0;
	mBufferAlignment = 0;
	mpAllocateName = NULL;

	mDataOffset = 0;
	mDataSize = 0;

	return AJA_STATUS_SUCCESS;
}


uint8_t* 
AJABuffer::GetBuffer()
{
	return mpBuffer;
}

uint32_t*
AJABuffer::GetUINT32Buffer()
{
	return (uint32_t*)mpBuffer;
}


size_t 
AJABuffer::GetBufferSize()
{
	return mBufferSize;
}


size_t 
AJABuffer::GetBufferAlignment()
{
	return mBufferAlignment;
}


const char* 
AJABuffer::GetBufferName()
{
	return mpAllocateName;
}


AJAStatus 
AJABuffer::SetDataOffset(size_t offset)
{
	mDataOffset = offset;

	if ((mDataOffset + mDataSize) > mBufferSize)
	{
		return AJA_STATUS_RANGE;
	}
	return AJA_STATUS_SUCCESS;
}


size_t 
AJABuffer::GetDataOffset()
{
	return mDataOffset;
}


AJAStatus 
AJABuffer::SetDataSize(size_t size)
{
	mDataSize = size;

	if ((mDataOffset + mDataSize) > mBufferSize)
	{
		return AJA_STATUS_RANGE;
	}
	return AJA_STATUS_SUCCESS;
}


size_t 
AJABuffer::GetDataSize()
{
	return mDataSize;
}


uint8_t* 
AJABuffer::GetData()
{
	return mpBuffer + mDataOffset;
}


void 
AJABuffer::ComputeAlignment()
{
	mBufferAlignment = 0;
	if (mpBuffer != NULL)
	{
		mBufferAlignment = 1;
        for (int32_t i = 0; i < 12; i++)
		{
			if (((uintptr_t)mpBuffer & ((uintptr_t)0x1 << i)) != 0)
			{
				break;
			}
			mBufferAlignment *= 2;
		}
	}
}
