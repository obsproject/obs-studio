/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2interrupts.cpp
	@brief		Implementation of CNTV2Card's interrupt functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2card.h"


static const INTERRUPT_ENUMS	gChannelToInputInterrupt []	=	{eInput1,	eInput2,	eInput3,	eInput4,	eInput5,	eInput6,	eInput7,	eInput8,	eNumInterruptTypes};
static const INTERRUPT_ENUMS	gChannelToOutputInterrupt [] =	{eOutput1,	eOutput2,	eOutput3,	eOutput4,	eOutput5,	eOutput6,	eOutput7,	eOutput8,	eNumInterruptTypes};


bool CNTV2Card::GetCurrentInterruptMasks (NTV2InterruptMask & outIntMask1, NTV2Interrupt2Mask & outIntMask2)
{
	return CNTV2DriverInterface::ReadRegister(kRegVidIntControl, outIntMask1)  &&  CNTV2DriverInterface::ReadRegister(kRegVidIntControl2, outIntMask2);
}


bool CNTV2Card::EnableInterrupt			(const INTERRUPT_ENUMS inInterruptCode)	{return ConfigureInterrupt (true, inInterruptCode);}
bool CNTV2Card::EnableOutputInterrupt	(const NTV2Channel channel)				{return EnableInterrupt (gChannelToOutputInterrupt [channel]);}
bool CNTV2Card::EnableInputInterrupt	(const NTV2Channel channel)				{return EnableInterrupt (gChannelToInputInterrupt [channel]);}
bool CNTV2Card::EnableInputInterrupt	(const NTV2ChannelSet & inFrameStores)
{
	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inFrameStores.begin());  it != inFrameStores.end();  ++it)
		if (!EnableInputInterrupt (*it))
			failures++;
	return failures == 0;
}

bool CNTV2Card::DisableInterrupt		(const INTERRUPT_ENUMS inInterruptCode)
{
	if(NTV2_IS_INPUT_INTERRUPT(inInterruptCode) || NTV2_IS_OUTPUT_INTERRUPT(inInterruptCode))
		return true;
	return ConfigureInterrupt (false, inInterruptCode);
}
bool CNTV2Card::DisableOutputInterrupt	(const NTV2Channel channel)				{return DisableInterrupt (gChannelToOutputInterrupt [channel]);}
bool CNTV2Card::DisableInputInterrupt	(const NTV2Channel channel)				{return DisableInterrupt (gChannelToInputInterrupt [channel]);}
bool CNTV2Card::DisableInputInterrupt	(const NTV2ChannelSet & inFrameStores)
{
	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inFrameStores.begin());  it != inFrameStores.end();  ++it)
		if (!DisableInputInterrupt (*it))
			failures++;
	return failures == 0;
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::GetCurrentInterruptMask (NTV2InterruptMask * outInterruptMask)
	{
		return ReadRegister (kRegVidIntControl, reinterpret_cast <ULWord *> (outInterruptMask));
	}

	//	Enable interrupts
	bool CNTV2Card::EnableVerticalInterrupt ()			{return EnableOutputInterrupt (NTV2_CHANNEL1);}
	bool CNTV2Card::EnableOutput2VerticalInterrupt ()	{return EnableOutputInterrupt (NTV2_CHANNEL2);}
	bool CNTV2Card::EnableOutput3VerticalInterrupt ()	{return EnableOutputInterrupt (NTV2_CHANNEL3);}
	bool CNTV2Card::EnableOutput4VerticalInterrupt ()	{return EnableOutputInterrupt (NTV2_CHANNEL4);}
	bool CNTV2Card::EnableOutput5VerticalInterrupt ()	{return EnableOutputInterrupt (NTV2_CHANNEL5);}
	bool CNTV2Card::EnableOutput6VerticalInterrupt ()	{return EnableOutputInterrupt (NTV2_CHANNEL6);}
	bool CNTV2Card::EnableOutput7VerticalInterrupt ()	{return EnableOutputInterrupt (NTV2_CHANNEL7);}
	bool CNTV2Card::EnableOutput8VerticalInterrupt ()	{return EnableOutputInterrupt (NTV2_CHANNEL8);}
	bool CNTV2Card::EnableInput1Interrupt ()			{return EnableInputInterrupt (NTV2_CHANNEL1);}
	bool CNTV2Card::EnableInput2Interrupt ()			{return EnableInputInterrupt (NTV2_CHANNEL2);}
	bool CNTV2Card::EnableInput3Interrupt ()			{return EnableInputInterrupt (NTV2_CHANNEL3);}
	bool CNTV2Card::EnableInput4Interrupt ()			{return EnableInputInterrupt (NTV2_CHANNEL4);}
	bool CNTV2Card::EnableInput5Interrupt ()			{return EnableInputInterrupt (NTV2_CHANNEL5);}
	bool CNTV2Card::EnableInput6Interrupt ()			{return EnableInputInterrupt (NTV2_CHANNEL6);}
	bool CNTV2Card::EnableInput7Interrupt ()			{return EnableInputInterrupt (NTV2_CHANNEL7);}
	bool CNTV2Card::EnableInput8Interrupt ()			{return EnableInputInterrupt (NTV2_CHANNEL8);}
	bool CNTV2Card::EnableAudioInterrupt ()				{return ConfigureInterrupt (true,	eAudio);}
	bool CNTV2Card::EnableAudioInWrapInterrupt ()		{return ConfigureInterrupt (true,	eAudioInWrap);}
	bool CNTV2Card::EnableAudioOutWrapInterrupt ()		{return ConfigureInterrupt (true,	eAudioOutWrap);}
	bool CNTV2Card::EnableUartTxInterrupt ()			{return ConfigureInterrupt (true,	eUartTx);}
	bool CNTV2Card::EnableUartRxInterrupt ()			{return ConfigureInterrupt (true,	eUartRx);}
	bool CNTV2Card::EnableUart2TxInterrupt ()			{return ConfigureInterrupt (true,	eUartTx);}
	bool CNTV2Card::EnableUart2RxInterrupt ()			{return ConfigureInterrupt (true,	eUartRx);}
	bool CNTV2Card::EnableAuxVerticalInterrupt ()		{return ConfigureInterrupt (true,	eAuxVerticalInterrupt);}
	bool CNTV2Card::EnableHDMIHotplugInterrupt ()		{return ConfigureInterrupt (true,	eHDMIRxV2HotplugDetect);}

	//	Disable interrupts
	bool CNTV2Card::DisableVerticalInterrupt ()			{return DisableOutputInterrupt (NTV2_CHANNEL1);}
	bool CNTV2Card::DisableOutput2VerticalInterrupt ()	{return DisableOutputInterrupt (NTV2_CHANNEL2);}
	bool CNTV2Card::DisableOutput3VerticalInterrupt ()	{return DisableOutputInterrupt (NTV2_CHANNEL3);}
	bool CNTV2Card::DisableOutput4VerticalInterrupt ()	{return DisableOutputInterrupt (NTV2_CHANNEL4);}
	bool CNTV2Card::DisableOutput5VerticalInterrupt ()	{return DisableOutputInterrupt (NTV2_CHANNEL5);}
	bool CNTV2Card::DisableOutput6VerticalInterrupt ()	{return DisableOutputInterrupt (NTV2_CHANNEL6);}
	bool CNTV2Card::DisableOutput7VerticalInterrupt ()	{return DisableOutputInterrupt (NTV2_CHANNEL7);}
	bool CNTV2Card::DisableOutput8VerticalInterrupt ()	{return DisableOutputInterrupt (NTV2_CHANNEL8);}
	bool CNTV2Card::DisableInput1Interrupt ()			{return DisableInputInterrupt (NTV2_CHANNEL1);}
	bool CNTV2Card::DisableInput2Interrupt ()			{return DisableInputInterrupt (NTV2_CHANNEL2);}
	bool CNTV2Card::DisableInput3Interrupt ()			{return DisableInputInterrupt (NTV2_CHANNEL3);}
	bool CNTV2Card::DisableInput4Interrupt ()			{return DisableInputInterrupt (NTV2_CHANNEL4);}
	bool CNTV2Card::DisableInput5Interrupt ()			{return DisableInputInterrupt (NTV2_CHANNEL5);}
	bool CNTV2Card::DisableInput6Interrupt ()			{return DisableInputInterrupt (NTV2_CHANNEL6);}
	bool CNTV2Card::DisableInput7Interrupt ()			{return DisableInputInterrupt (NTV2_CHANNEL7);}
	bool CNTV2Card::DisableInput8Interrupt ()			{return DisableInputInterrupt (NTV2_CHANNEL8);}
	bool CNTV2Card::DisableAudioInterrupt ()			{return ConfigureInterrupt (false,	eAudio);}
	bool CNTV2Card::DisableAudioInWrapInterrupt ()		{return ConfigureInterrupt (false,	eAudioInWrap);}
	bool CNTV2Card::DisableAudioOutWrapInterrupt ()		{return ConfigureInterrupt (false,	eAudioOutWrap);}
	bool CNTV2Card::DisableUartTxInterrupt ()			{return ConfigureInterrupt (false,	eUartTx);}
	bool CNTV2Card::DisableUartRxInterrupt ()			{return ConfigureInterrupt (false,	eUartRx);}
	bool CNTV2Card::DisableUart2TxInterrupt ()			{return ConfigureInterrupt (false,	eUartTx);}
	bool CNTV2Card::DisableUart2RxInterrupt ()			{return ConfigureInterrupt (false,	eUartRx);}
	bool CNTV2Card::DisableAuxVerticalInterrupt ()		{return ConfigureInterrupt (false,	eAuxVerticalInterrupt);}
	bool CNTV2Card::DisableHDMIHotplugInterrupt ()		{return ConfigureInterrupt (false,	eHDMIRxV2HotplugDetect);}
#endif	//	!NTV2_DEPRECATE
