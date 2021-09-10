/* SPDX-License-Identifier: MIT */
/**
	@file		basemachinecontrol.h
	@deprecated	Declares the CBaseMachineControl class.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#ifndef _BASEMACHINECONTROL_H
#define _BASEMACHINECONTROL_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"

/**
	@brief	A base class for deck control.
	@note	Not intended for use in production or mission-critical environments!
**/
class AJAExport CBaseMachineControl
{
	public:
		typedef enum
		{
			CONTROLTYPE_DDR,		//	DDREthernetControl
			CONTROLTYPE_XVID,		//	Uses NTV2 File to control AJA device
			CONTROLTYPE_NTV2,		//	Uses AJA NTV2 device serial port
			CONTROLTYPE_SERIAL,		//	Uses host serial Port
			CONTROLTYPE_INVALID
		} ControlType;

		typedef enum
		{
			CONTROL_UNIMPLEMENTED	= 0xFFFFFFFF
		} ControUnimplemented;

	public:
		explicit inline				CBaseMachineControl ()		:	_controlType (CONTROLTYPE_INVALID)			{}
		virtual inline				~CBaseMachineControl ()														{}

	public:
		virtual bool				Open (void)		= 0;
		virtual void    			Close (void)	= 0;

		//	These return true, false or CONTROL_UNIMPLEMENTED
		virtual inline ULWord		Play (void)																	{return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		ReversePlay (void)															{return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		Stop (void)																	{return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		FastForward (void)															{return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		Rewind (void)																{return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		AdvanceFrame (void)															{return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		BackFrame (void)															{return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		GetTimecodeString (SByte * pOutTimecodeString)								{(void) pOutTimecodeString;  return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		GotoFrameByString (SByte * pInFrameString)									{(void) pInFrameString;  return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		GotoFrame (ULWord inFrameNumber)											{(void) inFrameNumber;  return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		GotoFrameByHMS (UByte inHrs, UByte inMins, UByte inSecs, UByte inFrames)	{(void) inHrs;  (void) inMins;  (void) inSecs;  (void) inFrames;  return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		RecordAtFrame (ULWord inFrameNumber)										{(void) inFrameNumber;  return CONTROL_UNIMPLEMENTED;}
		virtual inline ULWord		Loop (ULWord inStartFrameNumber, ULWord inEndFrameNumber)					{(void) inStartFrameNumber;  (void) inEndFrameNumber;  return CONTROL_UNIMPLEMENTED;}
		virtual inline ControlType	GetControlType (void) const													{return _controlType;}

	protected:
		ControlType		_controlType;

};	//	CBaseMachineControl

#endif	//	_BASEMACHINECONTROL_H
