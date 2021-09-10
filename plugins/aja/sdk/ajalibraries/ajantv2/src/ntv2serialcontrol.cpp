/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2serialcontrol.cpp
	@brief		Declares the CNTV2SerialControl class.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2serialcontrol.h"
#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include <assert.h>



// NOTE!!! first byte is command length
static UByte playCommand[] =		{ 0x2, 0x20, 0x01 };
static UByte stopCommand[] =		{ 0x2, 0x20, 0x00 };
static UByte fastForwardCommand[] = { 0x2, 0x20, 0x10 };
static UByte rewindCommand[] =		{ 0x2, 0x20, 0x20 };
static UByte reversePlayCommand[] = { 0x3, 0x21, 0x21, 0x40 };
// static UByte deviceTypeCommand =	{ 0x2, 0x00, 0x11 };  // Unused, suppress warning
static UByte ltcTimeDataCommand[] =	{ 0x3, 0x61, 0x0C, 0x01 };  


CNTV2SerialControl::CNTV2SerialControl (const UWord inDeviceIndexNumber, const UWord inSerialPortIndexNum)
	:	_controlRegisterNum	(kRegRS422Control),
		_receiveRegisterNum	(kRegRS422Receive),
		_transmitRegisterNum(kRegRS422Transmit)
{
	if (CNTV2DeviceScanner::GetDeviceAtIndex(inDeviceIndexNumber, _ntv2Card))
	{
		if (inSerialPortIndexNum < ::NTV2DeviceGetNumSerialPorts (_ntv2Card.GetDeviceID ()))
		{
			if (inSerialPortIndexNum > 0)
			{
				_controlRegisterNum = kRegRS4222Control;
				_receiveRegisterNum = kRegRS4222Receive;
				_transmitRegisterNum = kRegRS4222Transmit;
			}	//	if using 2nd serial port

			//	Verify that the device can do RS422...
			ULWord  value	(0);
			_ntv2Card.ReadRegister (_controlRegisterNum, value);
			assert (value & kRegMaskRS422Present && "UART must be present!");

			//	Flush the FIFO's and enable TX and RX...
			value |= (kRegMaskRS422TXEnable + kRegMaskRS422RXEnable + kRegMaskRS422Flush);
			_ntv2Card.WriteRegister (_controlRegisterNum, value);

			//	Used for receive response from machine...
			_ntv2Card.EnableInterrupt (eUart1Rx);
			_ntv2Card.SubscribeEvent (eUart1Rx);
			_ntv2Card.EnableInterrupt (eUart1Tx);
			_ntv2Card.SubscribeEvent (eUart1Tx);
		}
		else	//	Desired serial port not present
			_ntv2Card.Close ();
	}
	_controlType = CONTROLTYPE_NTV2;

}	//	constructor


CNTV2SerialControl::~CNTV2SerialControl ()
{
	ULWord value = 0;

	if (_ntv2Card.IsOpen ())
	{
		//	Flush the FIFO's and disable TX and RX...
		value |= kRegMaskRS422Flush;
		value &= !(kRegMaskRS422TXEnable + kRegMaskRS422RXEnable);
		_ntv2Card.WriteRegister (_controlRegisterNum, value);
	
		_ntv2Card.DisableInterrupt (eUart1Rx);
		_ntv2Card.DisableInterrupt (eUart1Tx);
		_ntv2Card.Close ();	//	Unsubscribe happens in destructor
	}
}	//	destructor


ULWord  CNTV2SerialControl::Play (void)
{
	WriteCommand (playCommand, true);
	return GotACK ();
}


ULWord  CNTV2SerialControl::ReversePlay (void)
{
	WriteCommand (reversePlayCommand, true);
	return GotACK ();
}


ULWord  CNTV2SerialControl::Stop (void)
{
	WriteCommand (stopCommand, true);
	return GotACK ();
}


ULWord  CNTV2SerialControl::FastForward (void)
{
	WriteCommand (fastForwardCommand, true);
	return GotACK ();
}


ULWord  CNTV2SerialControl::Rewind (void)
{
	WriteCommand (rewindCommand, true);
	return GotACK ();
}


ULWord  CNTV2SerialControl::AdvanceFrame (void)
{
	SByte timecodeString [12];
	GetTimecodeString (timecodeString);

	const SerialMachineResponse &	lastResponse	(GetLastResponse ());
	UByte hoursBCD   = lastResponse.buffer[5];
	UByte minutesBCD = lastResponse.buffer[4];
	UByte secondsBCD = lastResponse.buffer[3];
	UByte framesBCD  = lastResponse.buffer[2]&0x3F;
	UByte hours =   (hoursBCD>>4)*10   + (hoursBCD&0xF);
	UByte minutes = (minutesBCD>>4)*10 + (minutesBCD&0xF);
	UByte seconds = (secondsBCD>>4)*10 + (secondsBCD&0xF);
	UByte frames =  (framesBCD>>4)*10  + (framesBCD&0xF);


	frames++;
	if ( frames > 29 )
	{
		frames = 0;
		seconds++;
		if ( seconds > 59 )
		{
			seconds = 0;
			minutes++;
			if ( minutes > 59 )
				hours++; // don't even try to check for a wrap here
		}
	}

	GotoFrameByHMS (hours, minutes, seconds, frames);
	
	return true;
}


ULWord  CNTV2SerialControl::BackFrame (void)
{
	SByte timecodeString[12];
	GetTimecodeString(timecodeString);
	const SerialMachineResponse &	lastResponse	(GetLastResponse ());
	UByte hoursBCD   = lastResponse.buffer[5];
	UByte minutesBCD = lastResponse.buffer[4];
	UByte secondsBCD = lastResponse.buffer[3];
	UByte framesBCD  = lastResponse.buffer[2]&0x3F;
	UByte hours =   (hoursBCD>>4)*10   + (hoursBCD&0xF);
	UByte minutes = (minutesBCD>>4)*10 + (minutesBCD&0xF);
	UByte seconds = (secondsBCD>>4)*10 + (secondsBCD&0xF);
	UByte frames =  (framesBCD>>4)*10  + (framesBCD&0xF);

	frames--;
	if ( frames == 255 )
	{
		frames = 29;
		seconds--;
		if ( seconds == 255 )
		{
			seconds = 59;
			minutes--;
			if ( minutes == 255 )
			{
				minutes = 59;
				if ( hours > 0 )
					hours--;
			}
		}
	}

	GotoFrameByHMS (hours, minutes, seconds, frames);
	
	return true;

}	//	BackFrame


ULWord  CNTV2SerialControl::GetTimecodeString (SByte * timecodeString)
{
	static char bcd [] =	{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '0', '0', '0', '0', '0'};

	WriteCommand (ltcTimeDataCommand, true);

	const SerialMachineResponse &	lastResponse	(GetLastResponse ());
	if (lastResponse.length)
	{
		UByte hours   = lastResponse.buffer [5];
		UByte minutes = lastResponse.buffer [4];
		UByte seconds = lastResponse.buffer [3];
		UByte frames  = lastResponse.buffer [2] & 0x3F;

		timecodeString [0] =  bcd[hours>>4];
		timecodeString [1] =  bcd[hours&0xF];
		timecodeString [2] = ':';
		timecodeString [3] =  bcd[minutes>>4];
		timecodeString [4] =  bcd[minutes&0xF];
		timecodeString [5] = ':';
		timecodeString [6] =  bcd[seconds>>4];
		timecodeString [7] =  bcd[seconds&0xF];
		timecodeString [8] = ':';
		timecodeString [9] =  bcd[frames>>4];
		timecodeString [10] = bcd[frames&0xF];
		timecodeString [11] = '\0';
		//printf("%s\n", timecodeString);
	}
	
	return true;

}	//	GetTimecodeString


//GotoFrameByHoursMinutesSeconds
// NOTE no error checking.
ULWord  CNTV2SerialControl::GotoFrameByHMS (UByte hours, UByte minutes, UByte seconds, UByte frames)
{
	UByte	cueUpWithDataCommand [] =	{0x6, 0x24, 0x31, 0x00, 0x00, 0x00, 0x00};

	cueUpWithDataCommand [6] = ((hours / 10) << 4) + (hours % 10);
	cueUpWithDataCommand [5] = ((minutes / 10) << 4) + (minutes % 10);
	cueUpWithDataCommand [4] = ((seconds / 10) << 4) + (seconds % 10);
	cueUpWithDataCommand [3] = ((frames / 10) << 4) + (frames % 10);

	WriteCommand (cueUpWithDataCommand, true);
	return GotACK ();

}	//	GotoFrameByHoursMinutesSeconds


//
// WriteCommand
// If response == true, then go read received bytes back from machine
// 
// It would work just as well to poll here to receive characters. The
// WaitForRxInterrupt is used only to demonstrate that it works.
bool CNTV2SerialControl::WriteCommand (const UByte * txBuffer, bool response)
{
	ULWord val;
	ULWord data;
	UByte commandLength = txBuffer [0];
	int i;

	_serialMachineResponse.length = 0;	//	Reset response length even if response isn't requested

	if (commandLength == 0 || commandLength > 14)
		return false;

	if ( _ntv2Card.IsOpen ())
	{	
		//	Transmit command and checksum...
		UByte checkSum = 0;
		for (UWord count = 1; count <= commandLength; count++)
		{	
			UByte value = txBuffer [count];
			_ntv2Card.WriteRegister (_transmitRegisterNum, value);
			checkSum += value;
		}
		_ntv2Card.WriteRegister (_transmitRegisterNum, checkSum);

		//	Wait for transmit FIFO empty (timeout = 1 byte / ms)
		for (i = 0; i < 3; i++)
		{
			_ntv2Card.WaitForInterrupt (eUartTx, commandLength + 1);

			//	Make sure the TX FIFO is empty...
			_ntv2Card.ReadRegister (_controlRegisterNum, val);
			if ((val & BIT_1) != 0)
				break;
		}

		if (!response)
			return true;

		//	Wait for the first byte from the deck...
		for (i = 0; i < 3; i++)
		{
			WaitForRxInterrupt ();
			//	Make sure that there is data in the RX FIFO...
			_ntv2Card.ReadRegister (_controlRegisterNum, val);
			if (val & BIT_4)
			{
				//	Wait for all the data...
				#if defined(AJAWindows) || defined(MSWindows)
					::Sleep (NTV2_SERIAL_RESPONSE_SIZE);
				#else
					usleep (NTV2_SERIAL_RESPONSE_SIZE * 1000);
				#endif
				break;
			}
		}

		//	Read the response (and everything else) from the FIFO...
		for (i = 0; i < 1000; i++)
		{
			_ntv2Card.ReadRegister (_controlRegisterNum, val);
			if ((val & BIT_4) == 0)
				break;

			_ntv2Card.ReadRegister (_receiveRegisterNum, data);
			if (_serialMachineResponse.length < NTV2_SERIAL_RESPONSE_SIZE)
				_serialMachineResponse.buffer [_serialMachineResponse.length++] = data & 0xFF;
		}

		if ((_serialMachineResponse.length == 0) || (_serialMachineResponse.length >= NTV2_SERIAL_RESPONSE_SIZE))
			return false;

		return true;
	}	//	if board open

	return false; // couldn't open board.

}	//	WriteCommand


// GotACK()
// Test if last response was the expected ACK
// For some commands this will NOT be the case.
bool CNTV2SerialControl::GotACK (void)
{
	bool status = false;
	if (_serialMachineResponse.length == 3)
	{
		if (_serialMachineResponse.buffer [0] == 0x10 &&
			_serialMachineResponse.buffer [1] == 0x01 &&
			_serialMachineResponse.buffer [2] == 0x11)
				status = true;
	}

	return status;

}	//	GotACK


bool CNTV2SerialControl::WriteTxBuffer (const UByte * txBuffer, UWord length)
{
	bool status = false;
	if (_ntv2Card.IsOpen ())
	{	
		for (UWord count = 0; count < length; count++)
			_ntv2Card.WriteRegister (_transmitRegisterNum, txBuffer [count]);

		if (WaitForTxInterrupt ())
			status = true;
	}
	return status;

}	//	WriteTxBuffer


bool CNTV2SerialControl::ReadRxBuffer (UByte * rxBuffer, UWord & actualLength, UWord maxLength)
{
	bool status = false;
	if (_ntv2Card.IsOpen ())
	{	
		ULWord val;
		ULWord numRead = 0;
		_ntv2Card.ReadRegister (_controlRegisterNum, val);

		while (val & BIT_4 && numRead < maxLength)
		{
			ULWord data;
			_ntv2Card.ReadRegister (_receiveRegisterNum, data);
			rxBuffer [numRead++] = data & 0xFF;
			_ntv2Card.ReadRegister (_controlRegisterNum, val);
		}
		actualLength = numRead;
	}

	return status;

}	//	ReadRxBuffer


bool CNTV2SerialControl::WaitForRxInterrupt (void)
{
	if (_ntv2Card.IsOpen ())
		return _ntv2Card.WaitForInterrupt (_controlRegisterNum == kRegRS422Control ? eUart1Rx : eUart2Rx);
	else
		return false;
}


bool CNTV2SerialControl::WaitForTxInterrupt (void)
{
	if (_ntv2Card.IsOpen ())
		return _ntv2Card.WaitForInterrupt (_controlRegisterNum == kRegRS422Control ? eUart1Tx : eUart2Tx);
	else
		return false;
}
