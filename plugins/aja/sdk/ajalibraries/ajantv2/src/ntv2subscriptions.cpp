/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2subscriptions.cpp
	@brief		Implementation of CNTV2Card's event notification subscription functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2card.h"

using namespace std; 


static INTERRUPT_ENUMS	gChannelToOutputVerticalInterrupt[]	= {eOutput1, eOutput2, eOutput3, eOutput4, eOutput5, eOutput6, eOutput7, eOutput8, eNumInterruptTypes};
static INTERRUPT_ENUMS	gChannelToInputVerticalInterrupt[]	= {eInput1,  eInput2,  eInput3,  eInput4,  eInput5,  eInput6,  eInput7,  eInput8,  eNumInterruptTypes};


//	Subscribe to events

bool CNTV2Card::SubscribeEvent (const INTERRUPT_ENUMS inEventCode)
{
	return NTV2_IS_VALID_INTERRUPT_ENUM(inEventCode)  &&  ConfigureSubscription (true, inEventCode, mInterruptEventHandles[inEventCode]);
}


bool CNTV2Card::SubscribeOutputVerticalEvent (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  SubscribeEvent(gChannelToOutputVerticalInterrupt[inChannel]);
}

bool CNTV2Card::SubscribeOutputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!SubscribeOutputVerticalEvent(*it))
			failures++;
	return !failures;
}


bool CNTV2Card::SubscribeInputVerticalEvent (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  SubscribeEvent(gChannelToInputVerticalInterrupt[inChannel]);
}

bool CNTV2Card::SubscribeInputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!SubscribeInputVerticalEvent(*it))
			failures++;
	return !failures;
}


//	Unsubscribe from events

bool CNTV2Card::UnsubscribeEvent (const INTERRUPT_ENUMS inEventCode)
{
	return NTV2_IS_VALID_INTERRUPT_ENUM(inEventCode)  &&  ConfigureSubscription (false, inEventCode, mInterruptEventHandles[inEventCode]);
}


bool CNTV2Card::UnsubscribeOutputVerticalEvent (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  UnsubscribeEvent(gChannelToOutputVerticalInterrupt[inChannel]);
}

bool CNTV2Card::UnsubscribeOutputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!UnsubscribeOutputVerticalEvent(*it))
			failures++;
	return !failures;
}


bool CNTV2Card::UnsubscribeInputVerticalEvent (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  UnsubscribeEvent(gChannelToInputVerticalInterrupt[inChannel]);
}

bool CNTV2Card::UnsubscribeInputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!UnsubscribeInputVerticalEvent(*it))
			failures++;
	return !failures;
}


//	Get interrupt count

bool CNTV2Card::GetOutputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = 0;
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  GetInterruptCount (gChannelToOutputVerticalInterrupt[inChannel], outCount);
}


bool CNTV2Card::GetInputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = 0;
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  GetInterruptCount (gChannelToInputVerticalInterrupt[inChannel], outCount);
}


//	Get event count

bool CNTV2Card::GetOutputVerticalEventCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = NTV2_IS_VALID_CHANNEL(inChannel)  ?  mEventCounts.at(gChannelToOutputVerticalInterrupt[inChannel])  :  0;
	return NTV2_IS_VALID_CHANNEL(inChannel);
}


bool CNTV2Card::GetInputVerticalEventCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = NTV2_IS_VALID_CHANNEL(inChannel)  ?  mEventCounts.at(gChannelToInputVerticalInterrupt[inChannel])  :  0;
	return NTV2_IS_VALID_CHANNEL(inChannel);
}


//	Set event count

bool CNTV2Card::SetOutputVerticalEventCount (const ULWord inCount, const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  SetInterruptEventCount(gChannelToOutputVerticalInterrupt[inChannel], inCount);
}

bool CNTV2Card::SetInputVerticalEventCount (const ULWord inCount, const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  SetInterruptEventCount(gChannelToInputVerticalInterrupt[inChannel], inCount);
}


bool CNTV2Card::WaitForOutputVerticalInterrupt (const NTV2Channel inChannel, UWord inRepeatCount)
{
	bool	result	(true);
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;
	if (!inRepeatCount)
		return false;
	do
	{
		result = WaitForInterrupt (gChannelToOutputVerticalInterrupt [inChannel]);
	} while (--inRepeatCount && result);
	return result;
}


bool CNTV2Card::WaitForInputVerticalInterrupt (const NTV2Channel inChannel, UWord inRepeatCount)
{
	bool	result	(true);
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;
	if (!inRepeatCount)
		return false;
	do
	{
		result = WaitForInterrupt (gChannelToInputVerticalInterrupt [inChannel]);
	} while (--inRepeatCount && result);
	return result;
}


bool CNTV2Card::GetOutputFieldID (const NTV2Channel channel, NTV2FieldID & outFieldID)
{
	//           	         	 	   CHANNEL1    CHANNEL2     CHANNEL3     CHANNEL4     CHANNEL5     CHANNEL6     CHANNEL7     CHANNEL8
	static ULWord	regNum []	=	{kRegStatus, kRegStatus,  kRegStatus,  kRegStatus, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, 0};
	static ULWord	bitShift []	=	{        23,          5,           3,           1,           9,           7,           5,           3, 0};

	// Check status register to see if it is the one we want.
	ULWord	statusValue	(0);
	ReadRegister (regNum[channel], statusValue);
	outFieldID = static_cast <NTV2FieldID> ((statusValue >> bitShift [channel]) & 0x1);

	return true;

}	//	GetOutputFieldID


bool CNTV2Card::WaitForOutputFieldID (const NTV2FieldID inFieldID, const NTV2Channel channel)
{
	//	Wait for next field interrupt...
	bool	bInterruptHappened	(WaitForOutputVerticalInterrupt (channel));

	// Check status register to see if it is the one we want.
	NTV2FieldID	currentFieldID (NTV2_FIELD0);
	GetOutputFieldID(channel, currentFieldID);

	//	If not, wait for another field interrupt...
	if (currentFieldID != inFieldID)
		bInterruptHappened = WaitForOutputVerticalInterrupt (channel);

	return bInterruptHappened;

}	//	WaitForOutputFieldID


bool CNTV2Card::GetInputFieldID (const NTV2Channel channel, NTV2FieldID & outFieldID)
{
	//           	         	 	   CHANNEL1    CHANNEL2     CHANNEL3     CHANNEL4     CHANNEL5     CHANNEL6     CHANNEL7     CHANNEL8
	static ULWord	regNum []	=	{kRegStatus, kRegStatus, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, 0};
	static ULWord	bitShift []	=	{        21,         19,          21,          19,          17,          15,          13,           3, 0};

	//	See if the field ID of the last input vertical interrupt is the one of interest...
	ULWord	statusValue (0);
	ReadRegister (regNum[channel], statusValue);
	outFieldID = static_cast <NTV2FieldID> ((statusValue >> bitShift [channel]) & 0x1);

	return true;

}	//	GetInputFieldID


bool CNTV2Card::WaitForInputFieldID (const NTV2FieldID inFieldID, const NTV2Channel channel)
{
	//	Wait for next field interrupt...
	bool	bInterruptHappened	(WaitForInputVerticalInterrupt (channel));

	//	See if the field ID of the last input vertical interrupt is the one of interest...
	NTV2FieldID	currentFieldID (NTV2_FIELD0);
	GetInputFieldID(channel, currentFieldID);

	//	If not, wait for another field interrupt...
	if (currentFieldID != inFieldID)
		bInterruptHappened = WaitForInputVerticalInterrupt (channel);

	return bInterruptHappened;

}	//	WaitForInputFieldID


#if !defined (NTV2_DEPRECATE)
	//	Subscribe to events
	bool CNTV2Card::SubscribeOutputVerticalEvent ()				{return ConfigureSubscription (true,	eOutput1,				mInterruptEventHandles [eOutput1]);}
	bool CNTV2Card::SubscribeOutput2VerticalEvent ()			{return ConfigureSubscription (true,	eOutput2,				mInterruptEventHandles [eOutput2]);}
	bool CNTV2Card::SubscribeOutput3VerticalEvent ()			{return ConfigureSubscription (true,	eOutput3,				mInterruptEventHandles [eOutput3]);}
	bool CNTV2Card::SubscribeOutput4VerticalEvent ()			{return ConfigureSubscription (true,	eOutput4,				mInterruptEventHandles [eOutput4]);}
	bool CNTV2Card::SubscribeOutput5VerticalEvent ()			{return ConfigureSubscription (true,	eOutput5,				mInterruptEventHandles [eOutput5]);}
	bool CNTV2Card::SubscribeOutput6VerticalEvent ()			{return ConfigureSubscription (true,	eOutput6,				mInterruptEventHandles [eOutput6]);}
	bool CNTV2Card::SubscribeOutput7VerticalEvent ()			{return ConfigureSubscription (true,	eOutput7,				mInterruptEventHandles [eOutput7]);}
	bool CNTV2Card::SubscribeOutput8VerticalEvent ()			{return ConfigureSubscription (true,	eOutput8,				mInterruptEventHandles [eOutput8]);}
	bool CNTV2Card::SubscribeInput1VerticalEvent ()				{return ConfigureSubscription (true,	eInput1,				mInterruptEventHandles [eInput1]);}
	bool CNTV2Card::SubscribeInput2VerticalEvent ()				{return ConfigureSubscription (true,	eInput2,				mInterruptEventHandles [eInput2]);}
	bool CNTV2Card::SubscribeInput3VerticalEvent ()				{return ConfigureSubscription (true,	eInput3,				mInterruptEventHandles [eInput3]);}
	bool CNTV2Card::SubscribeInput4VerticalEvent ()				{return ConfigureSubscription (true,	eInput4,				mInterruptEventHandles [eInput4]);}
	bool CNTV2Card::SubscribeInput5VerticalEvent ()				{return ConfigureSubscription (true,	eInput5,				mInterruptEventHandles [eInput5]);}
	bool CNTV2Card::SubscribeInput6VerticalEvent ()				{return ConfigureSubscription (true,	eInput6,				mInterruptEventHandles [eInput6]);}
	bool CNTV2Card::SubscribeInput7VerticalEvent ()				{return ConfigureSubscription (true,	eInput7,				mInterruptEventHandles [eInput7]);}
	bool CNTV2Card::SubscribeInput8VerticalEvent ()				{return ConfigureSubscription (true,	eInput8,				mInterruptEventHandles [eInput8]);}
	bool CNTV2Card::SubscribeChangeEvent ()						{return ConfigureSubscription (true,	eChangeEvent,			mInterruptEventHandles [eChangeEvent]);}
	bool CNTV2Card::SubscribeAudioInterruptEvent ()				{return ConfigureSubscription (true,	eAudio,					mInterruptEventHandles [eAudio]);}
	bool CNTV2Card::SubscribeAudioInWrapInterruptEvent ()		{return ConfigureSubscription (true,	eAudioInWrap,			mInterruptEventHandles [eAudioInWrap]);}
	bool CNTV2Card::SubscribeAudioOutWrapInterruptEvent ()		{return ConfigureSubscription (true,	eAudioOutWrap,			mInterruptEventHandles [eAudioOutWrap]);}
	bool CNTV2Card::SubscribeUartTxInterruptEvent ()			{return ConfigureSubscription (true,	eUart1Tx,				mInterruptEventHandles [eUart1Tx]);}
	bool CNTV2Card::SubscribeUartRxInterruptEvent ()			{return ConfigureSubscription (true,	eUart1Rx,				mInterruptEventHandles [eUart1Rx]);}
	bool CNTV2Card::SubscribeUart2TxInterruptEvent ()			{return ConfigureSubscription (true,	eUart2Tx,				mInterruptEventHandles [eUart2Tx]);}
	bool CNTV2Card::SubscribeUart2RxInterruptEvent ()			{return ConfigureSubscription (true,	eUart2Rx,				mInterruptEventHandles [eUart2Rx]);}
	bool CNTV2Card::SubscribeAuxVerticalInterruptEvent ()		{return ConfigureSubscription (true,	eAuxVerticalInterrupt,	mInterruptEventHandles [eAuxVerticalInterrupt]);}
	bool CNTV2Card::SubscribeHDMIHotplugInterruptEvent ()		{return ConfigureSubscription (true,	eHDMIRxV2HotplugDetect,	mInterruptEventHandles [eHDMIRxV2HotplugDetect]);}
	bool CNTV2Card::SubscribeDMA1InterruptEvent ()				{return ConfigureSubscription (true,	eDMA1,					mInterruptEventHandles [eDMA1]);}
	bool CNTV2Card::SubscribeDMA2InterruptEvent ()				{return ConfigureSubscription (true,	eDMA2,					mInterruptEventHandles [eDMA2]);}
	bool CNTV2Card::SubscribeDMA3InterruptEvent ()				{return ConfigureSubscription (true,	eDMA3,					mInterruptEventHandles [eDMA3]);}
	bool CNTV2Card::SubscribeDMA4InterruptEvent ()				{return ConfigureSubscription (true,	eDMA4,					mInterruptEventHandles [eDMA4]);}

	//	Unsubscribe from events
	bool CNTV2Card::UnsubscribeOutputVerticalEvent ()			{return ConfigureSubscription (false,	eOutput1,				mInterruptEventHandles [eOutput1]);}
	bool CNTV2Card::UnsubscribeOutput2VerticalEvent ()			{return ConfigureSubscription (false,	eOutput2,				mInterruptEventHandles [eOutput2]);}
	bool CNTV2Card::UnsubscribeOutput3VerticalEvent ()			{return ConfigureSubscription (false,	eOutput3,				mInterruptEventHandles [eOutput3]);}
	bool CNTV2Card::UnsubscribeOutput4VerticalEvent ()			{return ConfigureSubscription (false,	eOutput4,				mInterruptEventHandles [eOutput4]);}
	bool CNTV2Card::UnsubscribeOutput5VerticalEvent ()			{return ConfigureSubscription (false,	eOutput5,				mInterruptEventHandles [eOutput5]);}
	bool CNTV2Card::UnsubscribeOutput6VerticalEvent ()			{return ConfigureSubscription (false,	eOutput6,				mInterruptEventHandles [eOutput6]);}
	bool CNTV2Card::UnsubscribeOutput7VerticalEvent ()			{return ConfigureSubscription (false,	eOutput7,				mInterruptEventHandles [eOutput7]);}
	bool CNTV2Card::UnsubscribeOutput8VerticalEvent ()			{return ConfigureSubscription (false,	eOutput8,				mInterruptEventHandles [eOutput8]);}
	bool CNTV2Card::UnsubscribeInput1VerticalEvent ()			{return ConfigureSubscription (false,	eInput1,				mInterruptEventHandles [eInput1]);}
	bool CNTV2Card::UnsubscribeInput2VerticalEvent ()			{return ConfigureSubscription (false,	eInput2,				mInterruptEventHandles [eInput2]);}
	bool CNTV2Card::UnsubscribeInput3VerticalEvent ()			{return ConfigureSubscription (false,	eInput3,				mInterruptEventHandles [eInput3]);}
	bool CNTV2Card::UnsubscribeInput4VerticalEvent ()			{return ConfigureSubscription (false,	eInput4,				mInterruptEventHandles [eInput4]);}
	bool CNTV2Card::UnsubscribeInput5VerticalEvent ()			{return ConfigureSubscription (false,	eInput5,				mInterruptEventHandles [eInput5]);}
	bool CNTV2Card::UnsubscribeInput6VerticalEvent ()			{return ConfigureSubscription (false,	eInput6,				mInterruptEventHandles [eInput6]);}
	bool CNTV2Card::UnsubscribeInput7VerticalEvent ()			{return ConfigureSubscription (false,	eInput7,				mInterruptEventHandles [eInput7]);}
	bool CNTV2Card::UnsubscribeInput8VerticalEvent ()			{return ConfigureSubscription (false,	eInput8,				mInterruptEventHandles [eInput8]);}
	bool CNTV2Card::UnsubscribeChangeEvent ()					{return ConfigureSubscription (false,	eChangeEvent,			mInterruptEventHandles [eChangeEvent]);}
	bool CNTV2Card::UnsubscribeAudioInterruptEvent ()			{return ConfigureSubscription (false,	eAudio,					mInterruptEventHandles [eAudio]);}
	bool CNTV2Card::UnsubscribeAudioInWrapInterruptEvent ()		{return ConfigureSubscription (false,	eAudioInWrap,			mInterruptEventHandles [eAudioInWrap]);}
	bool CNTV2Card::UnsubscribeAudioOutWrapInterruptEvent ()	{return ConfigureSubscription (false,	eAudioOutWrap,			mInterruptEventHandles [eAudioOutWrap]);}
	bool CNTV2Card::UnsubscribeUartTxInterruptEvent ()			{return ConfigureSubscription (false,	eUart1Tx,				mInterruptEventHandles [eUart1Tx]);}
	bool CNTV2Card::UnsubscribeUartRxInterruptEvent ()			{return ConfigureSubscription (false,	eUart1Rx,				mInterruptEventHandles [eUart1Rx]);}
	bool CNTV2Card::UnsubscribeUart2TxInterruptEvent ()			{return ConfigureSubscription (false,	eUart2Tx,				mInterruptEventHandles [eUart2Tx]);}
	bool CNTV2Card::UnsubscribeUart2RxInterruptEvent ()			{return ConfigureSubscription (false,	eUart2Rx,				mInterruptEventHandles [eUart2Rx]);}
	bool CNTV2Card::UnsubscribeAuxVerticalInterruptEvent ()		{return ConfigureSubscription (false,	eAuxVerticalInterrupt,	mInterruptEventHandles [eAuxVerticalInterrupt]);}
	bool CNTV2Card::UnsubscribeHDMIHotplugInterruptEvent ()		{return ConfigureSubscription (false,	eHDMIRxV2HotplugDetect,	mInterruptEventHandles [eHDMIRxV2HotplugDetect]);}
	bool CNTV2Card::UnsubscribeDMA1InterruptEvent ()			{return ConfigureSubscription (false,	eDMA1,					mInterruptEventHandles [eDMA1]);}
	bool CNTV2Card::UnsubscribeDMA2InterruptEvent ()			{return ConfigureSubscription (false,	eDMA2,					mInterruptEventHandles [eDMA2]);}
	bool CNTV2Card::UnsubscribeDMA3InterruptEvent ()			{return ConfigureSubscription (false,	eDMA3,					mInterruptEventHandles [eDMA3]);}
	bool CNTV2Card::UnsubscribeDMA4InterruptEvent ()			{return ConfigureSubscription (false,	eDMA4,					mInterruptEventHandles [eDMA4]);}

	//	Get interrupt count
	bool CNTV2Card::GetOutputVerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetOutputVerticalInterruptCount (*pCount, NTV2_CHANNEL1) : false;}
	bool CNTV2Card::GetOutput2VerticalInterruptCount	(ULWord *pCount)	{return pCount ? GetOutputVerticalInterruptCount (*pCount, NTV2_CHANNEL2) : false;}
	bool CNTV2Card::GetOutput3VerticalInterruptCount	(ULWord *pCount)	{return pCount ? GetOutputVerticalInterruptCount (*pCount, NTV2_CHANNEL3) : false;}
	bool CNTV2Card::GetOutput4VerticalInterruptCount	(ULWord *pCount)	{return pCount ? GetOutputVerticalInterruptCount (*pCount, NTV2_CHANNEL4) : false;}
	bool CNTV2Card::GetOutput5VerticalInterruptCount	(ULWord *pCount)	{return pCount ? GetOutputVerticalInterruptCount (*pCount, NTV2_CHANNEL5) : false;}
	bool CNTV2Card::GetOutput6VerticalInterruptCount	(ULWord *pCount)	{return pCount ? GetOutputVerticalInterruptCount (*pCount, NTV2_CHANNEL6) : false;}
	bool CNTV2Card::GetOutput7VerticalInterruptCount	(ULWord *pCount)	{return pCount ? GetOutputVerticalInterruptCount (*pCount, NTV2_CHANNEL7) : false;}
	bool CNTV2Card::GetOutput8VerticalInterruptCount	(ULWord *pCount)	{return pCount ? GetOutputVerticalInterruptCount (*pCount, NTV2_CHANNEL8) : false;}
	bool CNTV2Card::GetInput1VerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInputVerticalInterruptCount (*pCount, NTV2_CHANNEL1) : false;}
	bool CNTV2Card::GetInput2VerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInputVerticalInterruptCount (*pCount, NTV2_CHANNEL2) : false;}
	bool CNTV2Card::GetInput3VerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInputVerticalInterruptCount (*pCount, NTV2_CHANNEL3) : false;}
	bool CNTV2Card::GetInput4VerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInputVerticalInterruptCount (*pCount, NTV2_CHANNEL4) : false;}
	bool CNTV2Card::GetInput5VerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInputVerticalInterruptCount (*pCount, NTV2_CHANNEL5) : false;}
	bool CNTV2Card::GetInput6VerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInputVerticalInterruptCount (*pCount, NTV2_CHANNEL6) : false;}
	bool CNTV2Card::GetInput7VerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInputVerticalInterruptCount (*pCount, NTV2_CHANNEL7) : false;}
	bool CNTV2Card::GetInput8VerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInputVerticalInterruptCount (*pCount, NTV2_CHANNEL8) : false;}
	bool CNTV2Card::GetAudioInterruptCount				(ULWord *pCount)	{return pCount ? GetInterruptCount (eAudio,					*pCount) : false;}
	bool CNTV2Card::GetAudioInWrapInterruptCount		(ULWord *pCount)	{return pCount ? GetInterruptCount (eAudioInWrap,			*pCount) : false;}
	bool CNTV2Card::GetAuxVerticalInterruptCount		(ULWord *pCount)	{return pCount ? GetInterruptCount (eAuxVerticalInterrupt,	*pCount) : false;}
	bool CNTV2Card::GetAudioOutWrapInterruptCount		(ULWord *pCount)	{return pCount ? GetInterruptCount (eAudioOutWrap,			*pCount) : false;}

	//	Get event count
	bool CNTV2Card::GetOutputVerticalEventCount			(ULWord *pCount)	{return pCount ? GetOutputVerticalEventCount (*pCount, NTV2_CHANNEL1) : false;}
	bool CNTV2Card::GetOutput2VerticalEventCount		(ULWord *pCount)	{return pCount ? GetOutputVerticalEventCount (*pCount, NTV2_CHANNEL2) : false;}
	bool CNTV2Card::GetOutput3VerticalEventCount		(ULWord *pCount)	{return pCount ? GetOutputVerticalEventCount (*pCount, NTV2_CHANNEL3) : false;}
	bool CNTV2Card::GetOutput4VerticalEventCount		(ULWord *pCount)	{return pCount ? GetOutputVerticalEventCount (*pCount, NTV2_CHANNEL4) : false;}
	bool CNTV2Card::GetOutput5VerticalEventCount		(ULWord *pCount)	{return pCount ? GetOutputVerticalEventCount (*pCount, NTV2_CHANNEL5) : false;}
	bool CNTV2Card::GetOutput6VerticalEventCount		(ULWord *pCount)	{return pCount ? GetOutputVerticalEventCount (*pCount, NTV2_CHANNEL6) : false;}
	bool CNTV2Card::GetOutput7VerticalEventCount		(ULWord *pCount)	{return pCount ? GetOutputVerticalEventCount (*pCount, NTV2_CHANNEL7) : false;}
	bool CNTV2Card::GetOutput8VerticalEventCount		(ULWord *pCount)	{return pCount ? GetOutputVerticalEventCount (*pCount, NTV2_CHANNEL8) : false;}

	bool CNTV2Card::GetInput1VerticalEventCount			(ULWord *pCount)	{return pCount ? GetInputVerticalEventCount (*pCount, NTV2_CHANNEL1) : false;}
	bool CNTV2Card::GetInput2VerticalEventCount			(ULWord *pCount)	{return pCount ? GetInputVerticalEventCount (*pCount, NTV2_CHANNEL2) : false;}
	bool CNTV2Card::GetInput3VerticalEventCount			(ULWord *pCount)	{return pCount ? GetInputVerticalEventCount (*pCount, NTV2_CHANNEL3) : false;}
	bool CNTV2Card::GetInput4VerticalEventCount			(ULWord *pCount)	{return pCount ? GetInputVerticalEventCount (*pCount, NTV2_CHANNEL4) : false;}
	bool CNTV2Card::GetInput5VerticalEventCount			(ULWord *pCount)	{return pCount ? GetInputVerticalEventCount (*pCount, NTV2_CHANNEL5) : false;}
	bool CNTV2Card::GetInput6VerticalEventCount			(ULWord *pCount)	{return pCount ? GetInputVerticalEventCount (*pCount, NTV2_CHANNEL6) : false;}
	bool CNTV2Card::GetInput7VerticalEventCount			(ULWord *pCount)	{return pCount ? GetInputVerticalEventCount (*pCount, NTV2_CHANNEL7) : false;}
	bool CNTV2Card::GetInput8VerticalEventCount			(ULWord *pCount)	{return pCount ? GetInputVerticalEventCount (*pCount, NTV2_CHANNEL8) : false;}

	bool CNTV2Card::GetAudioInterruptEventCount			(ULWord *pCount)	{return pCount ? GetInterruptEventCount (eAudio,				*pCount) : false;}
	bool CNTV2Card::GetAudioInWrapInterruptEventCount	(ULWord *pCount)	{return pCount ? GetInterruptEventCount (eAudioInWrap,			*pCount) : false;}
	bool CNTV2Card::GetAuxVerticalEventCount			(ULWord *pCount)	{return pCount ? GetInterruptEventCount (eAuxVerticalInterrupt,	*pCount) : false;}
	bool CNTV2Card::GetAudioOutWrapInterruptEventCount	(ULWord *pCount)	{return pCount ? GetInterruptEventCount (eAudioOutWrap,			*pCount) : false;}

	//	Set event count
	bool CNTV2Card::SetOutput2VerticalEventCount (ULWord Count)				{return SetOutputVerticalEventCount (Count, NTV2_CHANNEL2);}
	bool CNTV2Card::SetOutput3VerticalEventCount (ULWord Count)				{return SetOutputVerticalEventCount (Count, NTV2_CHANNEL3);}
	bool CNTV2Card::SetOutput4VerticalEventCount (ULWord Count)				{return SetOutputVerticalEventCount (Count, NTV2_CHANNEL4);}
	bool CNTV2Card::SetOutput5VerticalEventCount (ULWord Count)				{return SetOutputVerticalEventCount (Count, NTV2_CHANNEL5);}
	bool CNTV2Card::SetOutput6VerticalEventCount (ULWord Count)				{return SetOutputVerticalEventCount (Count, NTV2_CHANNEL6);}
	bool CNTV2Card::SetOutput7VerticalEventCount (ULWord Count)				{return SetOutputVerticalEventCount (Count, NTV2_CHANNEL7);}
	bool CNTV2Card::SetOutput8VerticalEventCount (ULWord Count)				{return SetOutputVerticalEventCount (Count, NTV2_CHANNEL8);}

	bool CNTV2Card::SetInput1VerticalEventCount (ULWord Count)				{return SetInputVerticalEventCount (Count, NTV2_CHANNEL1);}
	bool CNTV2Card::SetInput2VerticalEventCount (ULWord Count)				{return SetInputVerticalEventCount (Count, NTV2_CHANNEL2);}
	bool CNTV2Card::SetInput3VerticalEventCount (ULWord Count)				{return SetInputVerticalEventCount (Count, NTV2_CHANNEL3);}
	bool CNTV2Card::SetInput4VerticalEventCount (ULWord Count)				{return SetInputVerticalEventCount (Count, NTV2_CHANNEL4);}
	bool CNTV2Card::SetInput5VerticalEventCount (ULWord Count)				{return SetInputVerticalEventCount (Count, NTV2_CHANNEL5);}
	bool CNTV2Card::SetInput6VerticalEventCount (ULWord Count)				{return SetInputVerticalEventCount (Count, NTV2_CHANNEL6);}
	bool CNTV2Card::SetInput7VerticalEventCount (ULWord Count)				{return SetInputVerticalEventCount (Count, NTV2_CHANNEL7);}
	bool CNTV2Card::SetInput8VerticalEventCount	(ULWord Count)				{return SetInputVerticalEventCount (Count, NTV2_CHANNEL8);}

	bool CNTV2Card::SetAudioInterruptEventCount			(ULWord Count)		{return SetInterruptEventCount (eAudio,					Count);}
	bool CNTV2Card::SetAudioInWrapInterruptEventCount	(ULWord Count)		{return SetInterruptEventCount (eAudioInWrap,			Count);}
	bool CNTV2Card::SetAuxVerticalEventCount			(ULWord Count)		{return SetInterruptEventCount (eAuxVerticalInterrupt,	Count);}
	bool CNTV2Card::SetAudioOutWrapInterruptEventCount	(ULWord Count)		{return SetInterruptEventCount (eAudioOutWrap,			Count);}

	//	Wait for interrupt/event
	bool CNTV2Card::WaitForVerticalInterrupt ()								{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL1);}
	bool CNTV2Card::WaitForOutput1VerticalInterrupt ()						{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL1);}
	bool CNTV2Card::WaitForOutput2VerticalInterrupt ()						{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL2);}
	bool CNTV2Card::WaitForOutput3VerticalInterrupt ()						{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL3);}
	bool CNTV2Card::WaitForOutput4VerticalInterrupt ()						{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL4);}
	bool CNTV2Card::WaitForOutput5VerticalInterrupt ()						{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL5);}
	bool CNTV2Card::WaitForOutput6VerticalInterrupt ()						{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL6);}
	bool CNTV2Card::WaitForOutput7VerticalInterrupt ()						{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL7);}
	bool CNTV2Card::WaitForOutput8VerticalInterrupt ()						{return WaitForOutputVerticalInterrupt (NTV2_CHANNEL8);}
	bool CNTV2Card::WaitForInput1Vertical ()								{return WaitForInputVerticalInterrupt (NTV2_CHANNEL1);}
	bool CNTV2Card::WaitForInput2Vertical ()								{return WaitForInputVerticalInterrupt (NTV2_CHANNEL2);}
	bool CNTV2Card::WaitForInput3Vertical ()								{return WaitForInputVerticalInterrupt (NTV2_CHANNEL3);}
	bool CNTV2Card::WaitForInput4Vertical ()								{return WaitForInputVerticalInterrupt (NTV2_CHANNEL4);}
	bool CNTV2Card::WaitForInput5Vertical ()								{return WaitForInputVerticalInterrupt (NTV2_CHANNEL5);}
	bool CNTV2Card::WaitForInput6Vertical ()								{return WaitForInputVerticalInterrupt (NTV2_CHANNEL6);}
	bool CNTV2Card::WaitForInput7Vertical ()								{return WaitForInputVerticalInterrupt (NTV2_CHANNEL7);}
	bool CNTV2Card::WaitForInput8Vertical ()								{return WaitForInputVerticalInterrupt (NTV2_CHANNEL8);}
	bool CNTV2Card::WaitForAudioInterrupt ()								{return WaitForInterrupt (eAudio,					80       );}
	bool CNTV2Card::WaitForAudioInWrapInterrupt ()							{return WaitForInterrupt (eAudioInWrap,				3000     );}
	bool CNTV2Card::WaitForAudioOutWrapInterrupt ()							{return WaitForInterrupt (eAudioOutWrap,			3000     );}
	bool CNTV2Card::WaitForUartTxInterruptEvent (ULWord timeoutMS)			{return WaitForInterrupt (eUartTx,					timeoutMS);}
	bool CNTV2Card::WaitForUartRxInterruptEvent (ULWord timeoutMS)			{return WaitForInterrupt (eUartRx,					timeoutMS);}
	bool CNTV2Card::WaitForUart2TxInterruptEvent (ULWord timeoutMS)			{return WaitForInterrupt (eUart2Tx,					timeoutMS);}
	bool CNTV2Card::WaitForUart2RxInterruptEvent (ULWord timeoutMS)			{return WaitForInterrupt (eUart2Rx,					timeoutMS);}
	bool CNTV2Card::WaitForHDMIHotplugInterruptEvent (ULWord timeoutMS)		{return WaitForInterrupt (eHDMIRxV2HotplugDetect,	timeoutMS);}
	bool CNTV2Card::WaitForAuxVerticalInterrupt ()							{return WaitForInterrupt (eAuxVerticalInterrupt				 );}
	bool CNTV2Card::WaitForDMA1Interrupt ()									{return WaitForInterrupt (eDMA1,					100      );}
	bool CNTV2Card::WaitForDMA2Interrupt ()									{return WaitForInterrupt (eDMA2,					100      );}
	bool CNTV2Card::WaitForDMA3Interrupt ()									{return WaitForInterrupt (eDMA3,					100      );}
	bool CNTV2Card::WaitForDMA4Interrupt ()									{return WaitForInterrupt (eDMA4,					100      );}
	bool CNTV2Card::WaitForPushButtonChangeInterrupt (ULWord timeoutMS)		{return WaitForInterrupt (ePushButtonChange,		timeoutMS);}
	bool CNTV2Card::WaitForLowPowerInterrupt (ULWord timeoutMS)				{return WaitForInterrupt (eLowPower,				timeoutMS);}
	bool CNTV2Card::WaitForDisplayFIFOInterrupt (ULWord timeoutMS)			{return WaitForInterrupt (eDisplayFIFO,				timeoutMS);}
	bool CNTV2Card::WaitForSATAChangeInterrupt (ULWord timeoutMS)			{return WaitForInterrupt (eSATAChange,				timeoutMS);}
	bool CNTV2Card::WaitForTemp1HighInterrupt (ULWord timeoutMS)			{return WaitForInterrupt (eTemp1High,				timeoutMS);}
	bool CNTV2Card::WaitForTemp2HighInterrupt (ULWord timeoutMS)			{return WaitForInterrupt (eTemp2High,				timeoutMS);}
	bool CNTV2Card::WaitForPowerButtonChangeInterrupt (ULWord timeoutMS)	{return WaitForInterrupt (ePowerButtonChange,		timeoutMS);}
	bool CNTV2Card::WaitForChangeEvent ()									{return WaitForInterrupt (eChangeEvent,				100      );}
	bool CNTV2Card::WaitForFieldID (NTV2FieldID fieldID)					{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL1);}
	bool CNTV2Card::WaitForOutput1FieldID (NTV2FieldID fieldID)				{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL1);}
	bool CNTV2Card::WaitForOutput2FieldID (NTV2FieldID fieldID)				{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL2);}
	bool CNTV2Card::WaitForOutput3FieldID (NTV2FieldID fieldID)				{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL3);}
	bool CNTV2Card::WaitForOutput4FieldID (NTV2FieldID fieldID)				{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL4);}
	bool CNTV2Card::WaitForOutput5FieldID (NTV2FieldID fieldID)				{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL5);}
	bool CNTV2Card::WaitForOutput6FieldID (NTV2FieldID fieldID)				{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL6);}
	bool CNTV2Card::WaitForOutput7FieldID (NTV2FieldID fieldID)				{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL7);}
	bool CNTV2Card::WaitForOutput8FieldID (NTV2FieldID fieldID)				{return WaitForOutputFieldID (fieldID, NTV2_CHANNEL8);}
	bool CNTV2Card::WaitForInput1FieldID (NTV2FieldID fieldID)				{return WaitForInputFieldID (fieldID, NTV2_CHANNEL1);}
	bool CNTV2Card::WaitForInput2FieldID (NTV2FieldID fieldID)				{return WaitForInputFieldID (fieldID, NTV2_CHANNEL2);}
	bool CNTV2Card::WaitForInput3FieldID (NTV2FieldID fieldID)				{return WaitForInputFieldID (fieldID, NTV2_CHANNEL3);}
	bool CNTV2Card::WaitForInput4FieldID (NTV2FieldID fieldID)				{return WaitForInputFieldID (fieldID, NTV2_CHANNEL4);}
	bool CNTV2Card::WaitForInput5FieldID (NTV2FieldID fieldID)				{return WaitForInputFieldID (fieldID, NTV2_CHANNEL5);}
	bool CNTV2Card::WaitForInput6FieldID (NTV2FieldID fieldID)				{return WaitForInputFieldID (fieldID, NTV2_CHANNEL6);}
	bool CNTV2Card::WaitForInput7FieldID (NTV2FieldID fieldID)				{return WaitForInputFieldID (fieldID, NTV2_CHANNEL7);}
	bool CNTV2Card::WaitForInput8FieldID (NTV2FieldID fieldID)				{return WaitForInputFieldID (fieldID, NTV2_CHANNEL8);}
#endif	//	!NTV2_DEPRECATE
