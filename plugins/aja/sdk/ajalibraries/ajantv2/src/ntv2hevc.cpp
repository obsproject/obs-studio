/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2hevc.cpp
	@brief		Implementations of HEVC-related CNTV2Card methods.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2devicefeatures.h"
#include "ntv2card.h"

bool CNTV2Card::HevcGetDeviceInfo(HevcDeviceInfo* pInfo)
{
	HevcMessageInfo message;
	bool result;

	if(pInfo == NULL) return false;

	memset(&message, 0, sizeof(HevcMessageInfo));
	message.header.type = Hevc_MessageId_Info;
	message.header.size = sizeof(HevcMessageInfo);
	message.data = *pInfo;

	result = HevcSendMessage((HevcMessageHeader*)&message);
	if(result)
	{
		*pInfo = message.data;
	}

	return result;
}

bool CNTV2Card::HevcWriteRegister(ULWord address, ULWord value, ULWord mask, ULWord shift)
{
	HevcMessageRegister message;
	bool result;

	memset(&message, 0, sizeof(HevcMessageRegister));
	message.header.type = Hevc_MessageId_Register;
	message.header.size = sizeof(HevcMessageRegister);
	message.data.address = address;
	message.data.writeValue = value;
	message.data.readValue = 0;
	message.data.mask = mask;
	message.data.shift = shift;
	message.data.write = true;
	message.data.read = false;
	message.data.forceBar4 = false;

	result = HevcSendMessage((HevcMessageHeader*)&message);

	return result;
}

bool CNTV2Card::HevcReadRegister(ULWord address, ULWord* pValue, ULWord mask, ULWord shift)
{
	HevcMessageRegister message;
	bool result;

	if(pValue == NULL) return false;

	memset(&message, 0, sizeof(HevcMessageRegister));
	message.header.type = Hevc_MessageId_Register;
	message.header.size = sizeof(HevcMessageRegister);
	message.data.address = address;
	message.data.writeValue = 0;
	message.data.readValue = 0;
	message.data.mask = mask;
	message.data.shift = shift;
	message.data.write = false;
	message.data.read = true;
	message.data.forceBar4 = false;

	result = HevcSendMessage((HevcMessageHeader*)&message);
	if(result)
	{
		*pValue = message.data.readValue;
	}

	return result;
}

bool CNTV2Card::HevcSendCommand(HevcDeviceCommand* pCommand)
{
	HevcMessageCommand message;
	bool result;

	if(pCommand == NULL) return false;

	memset(&message, 0, sizeof(HevcMessageCommand));
	message.header.type = Hevc_MessageId_Command;
	message.header.size = sizeof(HevcMessageCommand);
	message.data = *pCommand;

	result = HevcSendMessage((HevcMessageHeader*)&message);
	if(result)
	{
		*pCommand = message.data;
	}

	return result;
}

bool CNTV2Card::HevcVideoTransfer(HevcDeviceTransfer* pTransfer)
{
	HevcMessageTransfer message;
	bool result;

	if(pTransfer == NULL) return false;

	// breakout copy to thunk
	memset(&message, 0, sizeof(HevcMessageTransfer));
	message.header.type = Hevc_MessageId_Transfer;
	message.header.size = sizeof(HevcMessageTransfer);
	message.data.streamType = pTransfer->streamType;
	message.data.streamId = pTransfer->streamId;
	message.data.videoBuffer = (ULWord64)((uintptr_t)(pTransfer->pVideoBuffer));
	message.data.videoBufferSize = pTransfer->videoBufferSize;
	message.data.videoDataSize = pTransfer->videoDataSize;
	message.data.segVideoPitch = pTransfer->segVideoPitch;
	message.data.segCodecPitch = pTransfer->segCodecPitch;
	message.data.segSize = pTransfer->segSize;
	message.data.segCount = pTransfer->segCount;
	message.data.infoBuffer = (ULWord64)((uintptr_t)(pTransfer->pInfoBuffer));
	message.data.infoBufferSize = pTransfer->infoBufferSize;
	message.data.infoDataSize = pTransfer->infoDataSize;
	message.data.encodeTime = pTransfer->encodeTime;
	message.data.flags = pTransfer->flags;

	result = HevcSendMessage((HevcMessageHeader*)&message);
	if(result)
	{
		// breakout copy to thunk
		pTransfer->streamType = message.data.streamType;
		pTransfer->streamId = message.data.streamId;
		pTransfer->pVideoBuffer = (UByte*)((uintptr_t)(message.data.videoBuffer));
		pTransfer->videoBufferSize = message.data.videoBufferSize;
		pTransfer->videoDataSize = message.data.videoDataSize;
		pTransfer->segVideoPitch = message.data.segVideoPitch;
		pTransfer->segCodecPitch = message.data.segCodecPitch;
		pTransfer->segSize = message.data.segSize;
		pTransfer->segCount = message.data.segCount;
		pTransfer->pInfoBuffer = (UByte*)((uintptr_t)(message.data.infoBuffer));
		pTransfer->infoBufferSize = message.data.infoBufferSize;
		pTransfer->infoDataSize = message.data.infoDataSize;
		pTransfer->encodeTime = message.data.encodeTime;
		pTransfer->flags = message.data.flags;
	}

	return result;
}

bool CNTV2Card::HevcGetStatus(HevcDeviceStatus* pStatus)
{
	HevcMessageStatus message;
	bool result;

	if(pStatus == NULL) return false;

	memset(&message, 0, sizeof(HevcMessageStatus));
	message.header.type = Hevc_MessageId_Status;
	message.header.size = sizeof(HevcMessageStatus);
	message.data = *pStatus;

	result = HevcSendMessage((HevcMessageHeader*)&message);
	if(result)
	{
		*pStatus = message.data;
	}

	return result;
}

bool CNTV2Card::HevcDebugInfo(HevcDeviceDebug* pDebug)
{
	HevcMessageDebug message;
	bool result;

	if(pDebug == NULL) return false;

	memset(&message, 0, sizeof(HevcMessageDebug));
	message.header.type = Hevc_MessageId_Debug;
	message.header.size = sizeof(HevcMessageDebug);
	message.data = *pDebug;

	result = HevcSendMessage((HevcMessageHeader*)&message);
	if(result)
	{
		*pDebug = message.data;
	}

	return result;
}

