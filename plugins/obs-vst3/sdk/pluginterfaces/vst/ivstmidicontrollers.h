//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstmidicontrollers.h
// Created by  : Steinberg, 02/2006
// Description : VST MIDI Controller Enumeration
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Controller Numbers (MIDI) */
enum ControllerNumbers
{
	kCtrlBankSelectMSB	=	0,	///< Bank Select MSB
	kCtrlModWheel		=	1,	///< Modulation Wheel
	kCtrlBreath			=	2,	///< Breath controller

	kCtrlFoot			=	4,	///< Foot Controller
	kCtrlPortaTime		=	5,	///< Portamento Time
	kCtrlDataEntryMSB	=	6,	///< Data Entry MSB
	kCtrlVolume			=	7,	///< Channel Volume (formerly Main Volume)
	kCtrlBalance		=	8,	///< Balance

	kCtrlPan			=	10,	///< Pan
	kCtrlExpression		=	11,	///< Expression
	kCtrlEffect1		=	12,	///< Effect Control 1
	kCtrlEffect2		=	13,	///< Effect Control 2

	//---General Purpose Controllers #1 to #4---
	kCtrlGPC1			=	16,	///< General Purpose Controller #1
	kCtrlGPC2			=	17,	///< General Purpose Controller #2
	kCtrlGPC3			=	18,	///< General Purpose Controller #3
	kCtrlGPC4			=	19,	///< General Purpose Controller #4

	kCtrlBankSelectLSB	=	32,	///< Bank Select LSB

	kCtrlDataEntryLSB	=	38,	///< Data Entry LSB

	kCtrlSustainOnOff	=	64,	///< Damper Pedal On/Off (Sustain)
	kCtrlPortaOnOff		=	65,	///< Portamento On/Off
	kCtrlSustenutoOnOff	=	66,	///< Sustenuto On/Off
	kCtrlSoftPedalOnOff	=	67,	///< Soft Pedal On/Off
	kCtrlLegatoFootSwOnOff=	68,	///< Legato Footswitch On/Off
	kCtrlHold2OnOff		=	69,	///< Hold 2 On/Off

	//---Sound Controllers #1 to #10---
	kCtrlSoundVariation	=	70, ///< Sound Variation
	kCtrlFilterCutoff	=	71,	///< Filter Cutoff (Timbre/Harmonic Intensity)
	kCtrlReleaseTime	=	72,	///< Release Time
	kCtrlAttackTime		=	73,	///< Attack Time
	kCtrlFilterResonance=	74,	///< Filter Resonance (Brightness)
	kCtrlDecayTime		=	75,	///< Decay Time
	kCtrlVibratoRate	=	76,	///< Vibrato Rate
	kCtrlVibratoDepth	=	77,	///< Vibrato Depth
	kCtrlVibratoDelay	=	78,	///< Vibrato Delay
	kCtrlSoundCtrler10	=	79, ///< undefined

	//---General Purpose Controllers #5 to #8---
	kCtrlGPC5			=	80,	///< General Purpose Controller #5
	kCtrlGPC6			=	81,	///< General Purpose Controller #6
	kCtrlGPC7			=	82,	///< General Purpose Controller #7
	kCtrlGPC8			=	83,	///< General Purpose Controller #8

	kCtrlPortaControl	=	84,	///< Portamento Control

	//---Effect Controllers---
	kCtrlEff1Depth		=	91,	///< Effect 1 Depth (Reverb Send Level)
	kCtrlEff2Depth		=	92,	///< Effect 2 Depth (Tremolo Level)
	kCtrlEff3Depth		=	93,	///< Effect 3 Depth (Chorus Send Level)
	kCtrlEff4Depth		=	94,	///< Effect 4 Depth (Delay/Variation/Detune Level)
	kCtrlEff5Depth		=	95,	///< Effect 5 Depth (Phaser Level)

	kCtrlDataIncrement	=	96,	///< Data Increment (+1)
	kCtrlDataDecrement	=	97,	///< Data Decrement (-1)
	kCtrlNRPNSelectLSB 	=	98, ///< NRPN Select LSB
	kCtrlNRPNSelectMSB	=	99, ///< NRPN Select MSB
	kCtrlRPNSelectLSB	=	100, ///< RPN Select LSB
	kCtrlRPNSelectMSB	=	101, ///< RPN Select MSB

	//---Other Channel Mode Messages---
	kCtrlAllSoundsOff	=	120, ///< All Sounds Off
	kCtrlResetAllCtrlers =	121, ///< Reset All Controllers
	kCtrlLocalCtrlOnOff	=	122, ///< Local Control On/Off
	kCtrlAllNotesOff	=	123, ///< All Notes Off
	kCtrlOmniModeOff	=	124, ///< Omni Mode Off + All Notes Off
	kCtrlOmniModeOn		=	125, ///< Omni Mode On  + All Notes Off
	kCtrlPolyModeOnOff	=	126, ///< Poly Mode On/Off + All Sounds Off
	kCtrlPolyModeOn		=	127, ///< Poly Mode On

	//---Extra--------------------------
	kAfterTouch = 128,			///< After Touch (associated to Channel Pressure)
	kPitchBend  = 129,			///< Pitch Bend Change

	kCountCtrlNumber,			///< Count of Controller Number

	//---Extra for kLegacyMIDICCOutEvent-
	kCtrlProgramChange       = 130,	///< Program Change (use LegacyMIDICCOutEvent.value only)
	kCtrlPolyPressure        = 131,	///< Polyphonic Key Pressure (use LegacyMIDICCOutEvent.value for pitch and
									/// LegacyMIDICCOutEvent.value2 for pressure)
	kCtrlQuarterFrame        = 132,	///< Quarter Frame ((use LegacyMIDICCOutEvent.value only)

	kSystemSongSelect        = 133,	///< Song Select (use LegacyMIDICCOutEvent.value only)
	kSystemSongPointer       = 134,	///< Song Pointer (use LegacyMIDICCOutEvent.value for LSB and
									/// LegacyMIDICCOutEvent.value2 for MSB)
	kSystemCableSelect       = 135,	///< Cable Select (use LegacyMIDICCOutEvent.value only)
	kSystemTuneRequest       = 136,	///< Tune Request (use LegacyMIDICCOutEvent.value only)
	kSystemMidiClockStart    = 137,	///< Midi Clock Start (use LegacyMIDICCOutEvent.value only)
	kSystemMidiClockContinue = 138,	///< Midi Clock Continue (use LegacyMIDICCOutEvent.value only)
	kSystemMidiClockStop     = 139,	///< Midi Clock Stop (use LegacyMIDICCOutEvent.value only)
	kSystemActiveSensing     = 140,	///< Active Sensing (use LegacyMIDICCOutEvent.value only)
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
