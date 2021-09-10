/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2serialcontrol.cpp
	@brief		Implements the CNTV2SerialControl class.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#ifndef _NTV2SERIALCONTROL_H
#define _NTV2SERIALCONTROL_H

#include "ajaexport.h"
#include "basemachinecontrol.h"
#include "ntv2card.h"

#define	NTV2_SERIAL_RESPONSE_SIZE	64
#if !defined (NTV2_DEPRECATE)
	#define	XENA_SERIAL_RESPONSE_SIZE	NTV2_SERIAL_RESPONSE_SIZE
#endif


typedef struct
{
	UByte	length;
	UByte	buffer [NTV2_SERIAL_RESPONSE_SIZE];
} SerialMachineResponse;



/**
	@brief	This sample class demonstrates how to use the Corvid/Kona/Io UART RS422 port(s).
			It is NOT intended to be an extensive machine control implementation.
	@note	This class is not thread safe.
	@note	Currently assumes a 30fps frame rate.
**/
class AJAExport CNTV2SerialControl : public CBaseMachineControl
{
	//	Instance Methods
	public:
		//	Constructors & Destructors
		explicit				CNTV2SerialControl (const UWord inDeviceIndex = 0, const UWord inSerialPortIndexNum = 0);
		virtual					~CNTV2SerialControl ();

		//	Control Methods
		virtual inline bool		Open (void)											{return _ntv2Card.IsOpen ();}
		virtual inline void		Close (void)										{}
		virtual ULWord			Play (void);
		virtual ULWord			ReversePlay (void);
		virtual ULWord			Stop (void);
		virtual ULWord			FastForward (void);
		virtual ULWord			Rewind (void);
		virtual ULWord			AdvanceFrame (void);
		virtual ULWord			BackFrame (void);
		virtual ULWord			GotoFrameByHMS (UByte inHrs, UByte inMins, UByte inSecs, UByte inFrames);
		virtual ULWord			GetTimecodeString (SByte * timecodeString);

		//	Accessors
		virtual inline const SerialMachineResponse &	GetLastResponse (void) const	{return _serialMachineResponse;}	//	Warning: Not thread safe!

		virtual bool			WriteCommand (const UByte * txBuffer, bool response = true);

		virtual bool			WriteTxBuffer (const UByte * txBuffer, UWord length);
		virtual bool			ReadRxBuffer (UByte * rxBuffer, UWord & actualLength, UWord maxlength);

		virtual bool			GotACK (void);

		virtual bool			WaitForRxInterrupt (void);
		virtual bool			WaitForTxInterrupt (void);

	//	Instance Variables
	protected:
		CNTV2Card				_ntv2Card;
		SerialMachineResponse	_serialMachineResponse;
		RegisterNum				_controlRegisterNum;	///< @brief	Which UART control register to use:  kRegRS422Control (72) or kRegRS4222Control (246)
		RegisterNum				_receiveRegisterNum;	///< @brief	Which UART receive data register to use:  kRegRS422Receive (71) or kRegRS4222Receive (245)
		RegisterNum				_transmitRegisterNum;	///< @brief	Which UART transmit data register to use:  kRegRS422Transmit (70) or kRegRS4222Transmit (244)

};	//	CNTV2SerialControl


#if !defined (NTV2_DEPRECATE)
	typedef CNTV2SerialControl	CXenaSerialControl;		///< @deprecated	Use CNTV2SerialControl instead.
#endif

#endif	//	_NTV2SERIALCONTROL_H
