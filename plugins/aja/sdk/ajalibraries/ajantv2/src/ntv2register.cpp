/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2register.cpp
	@brief		Implements most of CNTV2Card's register-based functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2registerexpert.h"
#include "ntv2endian.h"
#include "ntv2bitfile.h"
#include "ntv2mcsfile.h"
#include "ntv2registersmb.h"
#include "ntv2konaflashprogram.h"
#include "ntv2konaflashprogram.h"
#include "ntv2vpid.h"
#include "ajabase/system/debug.h"
#include <math.h>
#include <assert.h>
#if defined (AJALinux)
	#include "ntv2linuxpublicinterface.h"
#elif defined (MSWindows)
	#pragma warning(disable: 4800)
#endif
#include <deque>

#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') <<               uint64_t(__x__)  << dec
#define INSTP(_p_)			HEX16(uint64_t(_p_))
#define	CVIDFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_VideoGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	CVIDWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_VideoGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	CVIDNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_VideoGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	CVIDINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_VideoGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	CVIDDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_VideoGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

#define	LOGGING_ROUTING_CHANGES			(AJADebug::IsActive(AJA_DebugUnit_RoutingGeneric))
#define	ROUTEFAIL(__x__)	AJA_sERROR  (AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	ROUTEWARN(__x__)	AJA_sWARNING(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	ROUTENOTE(__x__)	AJA_sNOTICE (AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	ROUTEINFO(__x__)	AJA_sINFO   (AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	ROUTEDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

using namespace std;

// Indexes into array
#define		NOMINAL_H			0
#define		MIN_H				1
#define		MAX_H				2
#define		NOMINAL_V			3
#define		MIN_V				4
#define		MAX_V				5


// Timing constants for K2 and KLS boards
#define K2_NOMINAL_H			0x1000
#define K2_MIN_H				(K2_NOMINAL_H-0x800)
#define K2_MAX_H				(K2_NOMINAL_H+0x800)
#define K2_NOMINAL_V			0x0800
#define K2_MIN_V				(K2_NOMINAL_V-0x400)
#define K2_MAX_V				(K2_NOMINAL_V+0x400)

#define KLS_NOMINAL_525_H		0x0640
#define KLS_MIN_525_H			0x0000
#define KLS_MAX_525_H			0x06B3
#define KLS_NOMINAL_525_V		0x010A
#define KLS_MIN_525_V			0x0001	
#define KLS_MAX_525_V			0x020D	

#define KLS_NOMINAL_625_H		0x0638
#define KLS_MIN_625_H			0x0000
#define KLS_MAX_625_H			0x06BF
#define KLS_NOMINAL_625_V		0x0139
#define KLS_MIN_625_V			0x0001
#define KLS_MAX_625_V			0x0271


//	These static tables eliminate a lot of switch statements.
//	CAUTION:	These are predicated on NTV2Channel being ordinal (NTV2_CHANNEL1==0, NTV2_CHANNEL2==1, etc.)
static const ULWord	gChannelToGlobalControlRegNum []	= {	kRegGlobalControl, kRegGlobalControlCh2, kRegGlobalControlCh3, kRegGlobalControlCh4,
															kRegGlobalControlCh5, kRegGlobalControlCh6, kRegGlobalControlCh7, kRegGlobalControlCh8, 0};

static const ULWord	gChannelToSDIOutControlRegNum []	= {	kRegSDIOut1Control, kRegSDIOut2Control, kRegSDIOut3Control, kRegSDIOut4Control,
															kRegSDIOut5Control, kRegSDIOut6Control, kRegSDIOut7Control, kRegSDIOut8Control, 0};

static const ULWord	gChannelToControlRegNum []			= {	kRegCh1Control, kRegCh2Control, kRegCh3Control, kRegCh4Control, kRegCh5Control, kRegCh6Control,
															kRegCh7Control, kRegCh8Control, 0};

static const ULWord	gChannelToOutputFrameRegNum []		= {	kRegCh1OutputFrame, kRegCh2OutputFrame, kRegCh3OutputFrame, kRegCh4OutputFrame,
															kRegCh5OutputFrame, kRegCh6OutputFrame, kRegCh7OutputFrame, kRegCh8OutputFrame, 0};

static const ULWord	gChannelToInputFrameRegNum []		= {	kRegCh1InputFrame, kRegCh2InputFrame, kRegCh3InputFrame, kRegCh4InputFrame,
															kRegCh5InputFrame, kRegCh6InputFrame, kRegCh7InputFrame, kRegCh8InputFrame, 0};

static const ULWord	gChannelToPCIAccessFrameRegNum []	= {	kRegCh1PCIAccessFrame, kRegCh2PCIAccessFrame, kRegCh3PCIAccessFrame, kRegCh4PCIAccessFrame,
															kRegCh5PCIAccessFrame, kRegCh6PCIAccessFrame, kRegCh7PCIAccessFrame, kRegCh8PCIAccessFrame, 0};

static const ULWord	gChannelToRS422ControlRegNum []		= {	kRegRS422Control, kRegRS4222Control, 0};

static const ULWord	gChannelToSDIOutVPIDARegNum []		= {	kRegSDIOut1VPIDA, kRegSDIOut2VPIDA, kRegSDIOut3VPIDA, kRegSDIOut4VPIDA,
															kRegSDIOut5VPIDA, kRegSDIOut6VPIDA, kRegSDIOut7VPIDA, kRegSDIOut8VPIDA, 0};

static const ULWord	gChannelToSDIOutVPIDBRegNum []		= {	kRegSDIOut1VPIDB, kRegSDIOut2VPIDB, kRegSDIOut3VPIDB, kRegSDIOut4VPIDB,
															kRegSDIOut5VPIDB, kRegSDIOut6VPIDB, kRegSDIOut7VPIDB, kRegSDIOut8VPIDB, 0};

static const ULWord	gChannelToOutputTimingCtrlRegNum []	= {	kRegOutputTimingControl, kRegOutputTimingControlch2, kRegOutputTimingControlch3, kRegOutputTimingControlch4,
															kRegOutputTimingControlch5, kRegOutputTimingControlch6, kRegOutputTimingControlch7, kRegOutputTimingControlch8, 0};

static const ULWord	gChannelToSDIInput3GStatusRegNum []	= {	kRegSDIInput3GStatus,		kRegSDIInput3GStatus,		kRegSDIInput3GStatus2,		kRegSDIInput3GStatus2,
															kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	0};

static const ULWord	gChannelToSDIInVPIDARegNum []		= {	kRegSDIIn1VPIDA,			kRegSDIIn2VPIDA,			kRegSDIIn3VPIDA,			kRegSDIIn4VPIDA,
															kRegSDIIn5VPIDA,			kRegSDIIn6VPIDA,			kRegSDIIn7VPIDA,			kRegSDIIn8VPIDA,			0};

static const ULWord	gChannelToSDIInVPIDBRegNum []		= {	kRegSDIIn1VPIDB,			kRegSDIIn2VPIDB,			kRegSDIIn3VPIDB,			kRegSDIIn4VPIDB,
															kRegSDIIn5VPIDB,			kRegSDIIn6VPIDB,			kRegSDIIn7VPIDB,			kRegSDIIn8VPIDB,			0};

static const ULWord	gChannelToSDIInVPIDLinkAValidMask[]	= {	kRegMaskSDIInVPIDLinkAValid,	kRegMaskSDIIn2VPIDLinkAValid,	kRegMaskSDIIn3VPIDLinkAValid,	kRegMaskSDIIn4VPIDLinkAValid,
															kRegMaskSDIIn5VPIDLinkAValid,	kRegMaskSDIIn6VPIDLinkAValid,	kRegMaskSDIIn7VPIDLinkAValid,	kRegMaskSDIIn8VPIDLinkAValid,	0};

static const ULWord	gChannelToSDIInVPIDLinkBValidMask[]	= {	kRegMaskSDIInVPIDLinkBValid,	kRegMaskSDIIn2VPIDLinkBValid,	kRegMaskSDIIn3VPIDLinkBValid,	kRegMaskSDIIn4VPIDLinkBValid,
															kRegMaskSDIIn5VPIDLinkBValid,	kRegMaskSDIIn6VPIDLinkBValid,	kRegMaskSDIIn7VPIDLinkBValid,	kRegMaskSDIIn8VPIDLinkBValid,	0};

static const ULWord	gChannelToSDIIn3GbModeMask []		= {	kRegMaskSDIIn3GbpsSMPTELevelBMode,	kRegMaskSDIIn23GbpsSMPTELevelBMode,	kRegMaskSDIIn33GbpsSMPTELevelBMode,	kRegMaskSDIIn43GbpsSMPTELevelBMode,
															kRegMaskSDIIn53GbpsSMPTELevelBMode,	kRegMaskSDIIn63GbpsSMPTELevelBMode,	kRegMaskSDIIn73GbpsSMPTELevelBMode,	kRegMaskSDIIn83GbpsSMPTELevelBMode,	0};

static const ULWord	gChannelToSDIIn3GbModeShift []		= {	kRegShiftSDIIn3GbpsSMPTELevelBMode,		kRegShiftSDIIn23GbpsSMPTELevelBMode,	kRegShiftSDIIn33GbpsSMPTELevelBMode,	kRegShiftSDIIn43GbpsSMPTELevelBMode,
															kRegShiftSDIIn53GbpsSMPTELevelBMode,	kRegShiftSDIIn63GbpsSMPTELevelBMode,	kRegShiftSDIIn73GbpsSMPTELevelBMode,	kRegShiftSDIIn83GbpsSMPTELevelBMode,	0};

static const ULWord	gIndexToVidProcControlRegNum []		= {	kRegVidProc1Control,	kRegVidProc2Control,	kRegVidProc3Control,	kRegVidProc4Control,	0};

static const ULWord	gIndexToVidProcMixCoeffRegNum []	= {	kRegMixer1Coefficient,	kRegMixer2Coefficient,	kRegMixer3Coefficient,	kRegMixer4Coefficient,	0};

#if defined (NTV2_ALLOW_2MB_FRAMES)
static const ULWord	gChannelTo2MFrame []				= {	kRegCh1Control2MFrame, kRegCh2Control2MFrame, kRegCh3Control2MFrame, kRegCh4Control2MFrame, kRegCh5Control2MFrame, kRegCh6Control2MFrame,
															kRegCh7Control2MFrame, kRegCh8Control2MFrame, 0};
#endif	//	defined (NTV2_ALLOW_2MB_FRAMES)

static const ULWord	gChannelToRP188ModeGCRegisterNum[]		= {	kRegGlobalControl,			kRegGlobalControl,			kRegGlobalControl2,			kRegGlobalControl2,
																kRegGlobalControl2,			kRegGlobalControl2,			kRegGlobalControl2,			kRegGlobalControl2,			0};
static const ULWord	gChannelToRP188ModeMasks[]				= {	kRegMaskRP188ModeCh1,		kRegMaskRP188ModeCh2,		kRegMaskRP188ModeCh3,		kRegMaskRP188ModeCh4,
																kRegMaskRP188ModeCh5,		ULWord(kRegMaskRP188ModeCh6),	kRegMaskRP188ModeCh7,		kRegMaskRP188ModeCh8,		0};
static const ULWord	gChannelToRP188ModeShifts[]			= {	kRegShiftRP188ModeCh1,		kRegShiftRP188ModeCh2,		kRegShiftRP188ModeCh3,		kRegShiftRP188ModeCh4,
																kRegShiftRP188ModeCh5,		kRegShiftRP188ModeCh6,		kRegShiftRP188ModeCh7,		kRegShiftRP188ModeCh8,		0};
static const ULWord	gChlToRP188DBBRegNum[]				= {	kRegRP188InOut1DBB,			kRegRP188InOut2DBB,			kRegRP188InOut3DBB,			kRegRP188InOut4DBB,
																kRegRP188InOut5DBB,			kRegRP188InOut6DBB,			kRegRP188InOut7DBB,			kRegRP188InOut8DBB,			0};
static const ULWord	gChlToRP188Bits031RegNum[]			= {	kRegRP188InOut1Bits0_31,	kRegRP188InOut2Bits0_31,	kRegRP188InOut3Bits0_31,	kRegRP188InOut4Bits0_31,
																kRegRP188InOut5Bits0_31,	kRegRP188InOut6Bits0_31,	kRegRP188InOut7Bits0_31,	kRegRP188InOut8Bits0_31,	0};
static const ULWord	gChlToRP188Bits3263RegNum[]			= {	kRegRP188InOut1Bits32_63,	kRegRP188InOut2Bits32_63,	kRegRP188InOut3Bits32_63,	kRegRP188InOut4Bits32_63,
																kRegRP188InOut5Bits32_63,	kRegRP188InOut6Bits32_63,	kRegRP188InOut7Bits32_63,	kRegRP188InOut8Bits32_63,	0};

static const ULWord	gChannelToRXSDIStatusRegs []			= {	kRegRXSDI1Status,				kRegRXSDI2Status,				kRegRXSDI3Status,				kRegRXSDI4Status,				kRegRXSDI5Status,				kRegRXSDI6Status,				kRegRXSDI7Status,				kRegRXSDI8Status,				0};

static const ULWord	gChannelToRXSDICRCErrorCountRegs[] = { kRegRXSDI1CRCErrorCount, kRegRXSDI2CRCErrorCount, kRegRXSDI3CRCErrorCount, kRegRXSDI4CRCErrorCount, kRegRXSDI5CRCErrorCount, kRegRXSDI6CRCErrorCount, kRegRXSDI7CRCErrorCount, kRegRXSDI8CRCErrorCount, 0 };

static const ULWord	gChannelToSmpte372RegisterNum []		= {	kRegGlobalControl,			kRegGlobalControl,			kRegGlobalControl2,			kRegGlobalControl2,
																kRegGlobalControl2,			kRegGlobalControl2,			kRegGlobalControl2,			kRegGlobalControl2,			0};
static const ULWord	gChannelToSmpte372Masks []				= {	kRegMaskSmpte372Enable,		kRegMaskSmpte372Enable,		kRegMaskSmpte372Enable4,	kRegMaskSmpte372Enable4,
																kRegMaskSmpte372Enable6,	kRegMaskSmpte372Enable6,	kRegMaskSmpte372Enable8,	kRegMaskSmpte372Enable8,	0};
static const ULWord	gChannelToSmpte372Shifts []				= {	kRegShiftSmpte372,			kRegShiftSmpte372,		kRegShiftSmpte372Enable4,	kRegShiftSmpte372Enable4,
																kRegShiftSmpte372Enable6,	kRegShiftSmpte372Enable6,	kRegShiftSmpte372Enable8,	kRegShiftSmpte372Enable8,	0};
static const ULWord	gChannelToSDIIn3GModeMask []	= {	kRegMaskSDIIn3GbpsMode,		kRegMaskSDIIn23GbpsMode,	kRegMaskSDIIn33GbpsMode,	kRegMaskSDIIn43GbpsMode,
														kRegMaskSDIIn53GbpsMode,	kRegMaskSDIIn63GbpsMode,	kRegMaskSDIIn73GbpsMode,	kRegMaskSDIIn83GbpsMode,	0};

static const ULWord	gChannelToSDIIn3GModeShift []	= {	kRegShiftSDIIn3GbpsMode,	kRegShiftSDIIn23GbpsMode,	kRegShiftSDIIn33GbpsMode,	kRegShiftSDIIn43GbpsMode,
														kRegShiftSDIIn53GbpsMode,	kRegShiftSDIIn63GbpsMode,	kRegShiftSDIIn73GbpsMode,	kRegShiftSDIIn83GbpsMode,	0};
static const ULWord	gChannelToSDIIn6GModeMask []	= {	kRegMaskSDIIn16GbpsMode,		kRegMaskSDIIn26GbpsMode,	kRegMaskSDIIn36GbpsMode,	kRegMaskSDIIn46GbpsMode,
														kRegMaskSDIIn56GbpsMode,	kRegMaskSDIIn66GbpsMode,	kRegMaskSDIIn76GbpsMode,	kRegMaskSDIIn86GbpsMode,	0};

static const ULWord	gChannelToSDIIn6GModeShift []	= {	kRegShiftSDIIn16GbpsMode,	kRegShiftSDIIn26GbpsMode,	kRegShiftSDIIn36GbpsMode,	kRegShiftSDIIn46GbpsMode,
														kRegShiftSDIIn56GbpsMode,	kRegShiftSDIIn66GbpsMode,	kRegShiftSDIIn76GbpsMode,	kRegShiftSDIIn86GbpsMode,	0};

static const ULWord	gChannelToSDIIn12GModeMask []	= {	kRegMaskSDIIn112GbpsMode,		kRegMaskSDIIn212GbpsMode,	kRegMaskSDIIn312GbpsMode,	kRegMaskSDIIn412GbpsMode,
														kRegMaskSDIIn512GbpsMode,	kRegMaskSDIIn612GbpsMode,	kRegMaskSDIIn712GbpsMode,	ULWord(kRegMaskSDIIn812GbpsMode),	0};

static const ULWord	gChannelToSDIIn12GModeShift []	= {	kRegShiftSDIIn112GbpsMode,	kRegShiftSDIIn212GbpsMode,	kRegShiftSDIIn312GbpsMode,	kRegShiftSDIIn412GbpsMode,
														kRegShiftSDIIn512GbpsMode,	kRegShiftSDIIn612GbpsMode,	kRegShiftSDIIn712GbpsMode,	kRegShiftSDIIn812GbpsMode,	0};

static const ULWord	gChannelToSDIInputStatusRegNum []		= {	kRegInputStatus,		kRegInputStatus,		kRegInputStatus2,		kRegInputStatus2,
																kRegInput56Status,		kRegInput56Status,		kRegInput78Status,		kRegInput78Status,	0};

static const ULWord	gChannelToSDIInputRateMask []			= {	kRegMaskInput1FrameRate,			kRegMaskInput2FrameRate,			kRegMaskInput1FrameRate,			kRegMaskInput2FrameRate,
																kRegMaskInput1FrameRate,			kRegMaskInput2FrameRate,			kRegMaskInput1FrameRate,			kRegMaskInput2FrameRate,			0};
static const ULWord	gChannelToSDIInputRateHighMask []		= {	kRegMaskInput1FrameRateHigh,		kRegMaskInput2FrameRateHigh,		kRegMaskInput1FrameRateHigh,		kRegMaskInput2FrameRateHigh,
																kRegMaskInput1FrameRateHigh,		kRegMaskInput2FrameRateHigh,		kRegMaskInput1FrameRateHigh,		kRegMaskInput2FrameRateHigh,		0};
static const ULWord	gChannelToSDIInputRateShift []			= {	kRegShiftInput1FrameRate,			kRegShiftInput2FrameRate,			kRegShiftInput1FrameRate,			kRegShiftInput2FrameRate,
																kRegShiftInput1FrameRate,			kRegShiftInput2FrameRate,			kRegShiftInput1FrameRate,			kRegShiftInput2FrameRate,			0};
static const ULWord	gChannelToSDIInputRateHighShift []		= {	kRegShiftInput1FrameRateHigh,		kRegShiftInput2FrameRateHigh,		kRegShiftInput1FrameRateHigh,		kRegShiftInput2FrameRateHigh,
																kRegShiftInput1FrameRateHigh,		kRegShiftInput2FrameRateHigh,		kRegShiftInput1FrameRateHigh,		kRegShiftInput2FrameRateHigh,		0};

static const ULWord	gChannelToSDIInputGeometryMask []		= {	kRegMaskInput1Geometry,				kRegMaskInput2Geometry,				kRegMaskInput1Geometry,				kRegMaskInput2Geometry,
																kRegMaskInput1Geometry,				kRegMaskInput2Geometry,				kRegMaskInput1Geometry,				kRegMaskInput2Geometry,				0};
static const ULWord	gChannelToSDIInputGeometryHighMask []	= {	kRegMaskInput1GeometryHigh,			ULWord(kRegMaskInput2GeometryHigh),	kRegMaskInput1GeometryHigh,			ULWord(kRegMaskInput2GeometryHigh),
                                                                kRegMaskInput1GeometryHigh,			ULWord(kRegMaskInput2GeometryHigh),	kRegMaskInput1GeometryHigh,			ULWord(kRegMaskInput2GeometryHigh),	0};
static const ULWord	gChannelToSDIInputGeometryShift []		= {	kRegShiftInput1Geometry,			kRegShiftInput2Geometry,			kRegShiftInput1Geometry,			kRegShiftInput2Geometry,
																kRegShiftInput1Geometry,			kRegShiftInput2Geometry,			kRegShiftInput1Geometry,			kRegShiftInput2Geometry,			0};
static const ULWord	gChannelToSDIInputGeometryHighShift []	= {	kRegShiftInput1GeometryHigh,		kRegShiftInput2GeometryHigh,		kRegShiftInput1GeometryHigh,		kRegShiftInput2GeometryHigh,
																kRegShiftInput1GeometryHigh,		kRegShiftInput2GeometryHigh,		kRegShiftInput1GeometryHigh,		kRegShiftInput2GeometryHigh,		0};

static const ULWord	gChannelToSDIInputProgressiveMask []	= {	kRegMaskInput1Progressive,			kRegMaskInput2Progressive,			kRegMaskInput1Progressive,			kRegMaskInput2Progressive,
																kRegMaskInput1Progressive,			kRegMaskInput2Progressive,			kRegMaskInput1Progressive,			kRegMaskInput2Progressive,			0};
static const ULWord	gChannelToSDIInputProgressiveShift []	= {	kRegShiftInput1Progressive,			kRegShiftInput2Progressive,			kRegShiftInput1Progressive,			kRegShiftInput2Progressive,
																kRegShiftInput1Progressive,			kRegShiftInput2Progressive,			kRegShiftInput1Progressive,			kRegShiftInput2Progressive,			0};

static const ULWord	gChannelToVPIDTransferCharacteristics []	= {	kVRegNTV2VPIDTransferCharacteristics1,		kVRegNTV2VPIDTransferCharacteristics2,		kVRegNTV2VPIDTransferCharacteristics3,		kVRegNTV2VPIDTransferCharacteristics4,
																	kVRegNTV2VPIDTransferCharacteristics5,		kVRegNTV2VPIDTransferCharacteristics6,		kVRegNTV2VPIDTransferCharacteristics7,		kVRegNTV2VPIDTransferCharacteristics8,	0};

static const ULWord	gChannelToVPIDColorimetry []	= {	kVRegNTV2VPIDColorimetry1,		kVRegNTV2VPIDColorimetry2,		kVRegNTV2VPIDColorimetry3,		kVRegNTV2VPIDColorimetry4,
														kVRegNTV2VPIDColorimetry5,		kVRegNTV2VPIDColorimetry6,		kVRegNTV2VPIDColorimetry7,		kVRegNTV2VPIDColorimetry8,	0};

static const ULWord	gChannelToVPIDLuminance []	= {	kVRegNTV2VPIDLuminance1,		kVRegNTV2VPIDLuminance2,		kVRegNTV2VPIDLuminance3,		kVRegNTV2VPIDLuminance4,
													kVRegNTV2VPIDLuminance5,		kVRegNTV2VPIDLuminance6,		kVRegNTV2VPIDLuminance7,		kVRegNTV2VPIDLuminance8,	0};

static const ULWord	gChannelToVPIDRGBRange []	= {	kVRegNTV2VPIDRGBRange1,		kVRegNTV2VPIDRGBRange2,		kVRegNTV2VPIDRGBRange3,		kVRegNTV2VPIDRGBRange4,
													kVRegNTV2VPIDRGBRange5,		kVRegNTV2VPIDRGBRange6,		kVRegNTV2VPIDRGBRange7,		kVRegNTV2VPIDRGBRange8,	0};


// Method: SetEveryFrameServices
// Input:  NTV2EveryFrameTaskMode
// Output: NONE
bool CNTV2Card::SetEveryFrameServices (NTV2EveryFrameTaskMode mode)
{
	return WriteRegister(kVRegEveryFrameTaskFilter, ULWord(mode));
}

bool CNTV2Card::GetEveryFrameServices (NTV2EveryFrameTaskMode & outMode)
{
	return CNTV2DriverInterface::ReadRegister(kVRegEveryFrameTaskFilter, outMode);
}

bool CNTV2Card::SetDefaultVideoOutMode(ULWord mode)
{
	return WriteRegister(kVRegDefaultVideoOutMode, mode);
}

bool CNTV2Card::GetDefaultVideoOutMode(ULWord & outMode)
{
	return ReadRegister (kVRegDefaultVideoOutMode, outMode);
}

// Method: SetVideoFormat
// Input:  NTV2VideoFormat
// Output: NONE
bool CNTV2Card::SetVideoFormat (const NTV2VideoFormat value, const bool inIsRetail, const bool keepVancSettings, const NTV2Channel inChannel)
{	AJA_UNUSED(keepVancSettings);
	bool ajaRetail(inIsRetail);
#ifdef  MSWindows
	NTV2EveryFrameTaskMode mode;
	GetEveryFrameServices(mode);
	if(mode == NTV2_STANDARD_TASKS)
		ajaRetail = true;
#endif
	const NTV2Channel channel(IsMultiFormatActive() ? inChannel : NTV2_CHANNEL1);
	int	hOffset(0),  vOffset(0);

	if (ajaRetail)
	{	// Get the current H and V timing offsets
		GetVideoHOffset(hOffset);
		GetVideoVOffset(vOffset);
	}

	if (NTV2_IS_TSI_FORMAT(value) && !NTV2DeviceCanDoVideoFormat(GetDeviceID(), value))
		return false;

    NTV2Standard inStandard = GetNTV2StandardFromVideoFormat(value);
	if(inStandard == NTV2_STANDARD_2Kx1080p && NTV2_IS_PSF_VIDEO_FORMAT(value))
	{
		inStandard = NTV2_STANDARD_2Kx1080i;
	}
	else if(inStandard == NTV2_STANDARD_3840x2160p && NTV2_IS_PSF_VIDEO_FORMAT(value))
	{
		inStandard = NTV2_STANDARD_3840i;
	}
	else if(inStandard == NTV2_STANDARD_4096x2160p && NTV2_IS_PSF_VIDEO_FORMAT(value))
	{
		inStandard = NTV2_STANDARD_4096i;
	}
    NTV2FrameRate inFrameRate = GetNTV2FrameRateFromVideoFormat(value);
    NTV2FrameGeometry inFrameGeometry = GetNTV2FrameGeometryFromVideoFormat(value);
	bool squares;
	
#if !defined (NTV2_DEPRECATE)
	// If switching from high def to standard def or vice versa
	// some boards need to have a different bitfile loaded.
	CheckBitfile(value);
#endif	//	!defined (NTV2_DEPRECATE)

	// Set the standard for this video format
	SetStandard(inStandard, channel);

	// Set the framegeometry for this video format
	SetFrameGeometry(inFrameGeometry, ajaRetail, channel);

	// Set the framerate for this video format
	SetFrameRate(inFrameRate, channel);

	// Set SMPTE 372 1080p60 Dual Link option
	SetSmpte372(NTV2_IS_3Gb_FORMAT(value), channel);

	// set virtual video format
	WriteRegister (kVRegVideoFormatCh1 + channel, value);

	//This will handle 4k formats
	if (NTV2_IS_QUAD_FRAME_FORMAT(value))
	{
		SetQuadQuadFrameEnable(false, channel);
		Get4kSquaresEnable(squares, channel);
		if (squares)
		{
			Set4kSquaresEnable(true, channel);
		}
		else
		{
			SetQuadFrameEnable(true, channel);
		}
	}
	else if (NTV2_IS_QUAD_QUAD_FORMAT(value))
	{
		GetQuadQuadSquaresEnable(squares, channel);
		if (squares)
		{
			SetQuadQuadSquaresEnable(true, channel);
		}
		else
		{
			SetQuadQuadFrameEnable(true, channel);
		}
	}
	else
	{
		//	Non-quad format
		SetQuadFrameEnable(false, channel);
		SetQuadQuadFrameEnable(false, channel);
		if (!IsMultiFormatActive())
		{
			CopyVideoFormat(channel, NTV2_CHANNEL1, NTV2_CHANNEL8);
		}
	}
	
	// Set Progressive Picture State
    SetProgressivePicture(IsProgressivePicture(value));

	if (ajaRetail)
	{
		// Set the H and V timing (note: this will also set the nominal value for this given format)
		SetVideoHOffset(hOffset);
		SetVideoVOffset(vOffset);
	}
	else
	{
		// All other cards
		WriteOutputTimingControl(K2_NOMINAL_H | (K2_NOMINAL_V<<16), channel);
	}

    if (::NTV2DeviceCanDoMultiFormat(GetDeviceID()) && !IsMultiFormatActive())
    {
        // Copy channel 1 register write mode to all channels in single format mode
        NTV2RegisterWriteMode writeMode;
        GetRegisterWriteMode(writeMode);
        SetRegisterWriteMode(writeMode);
    }
	
	return true; 
}

bool CNTV2Card::SetVideoFormat (const NTV2ChannelSet & inFrameStores, const NTV2VideoFormat inVideoFormat, bool inIsAJARetail)
{
	size_t errors(0);
	for (NTV2ChannelSetConstIter it(inFrameStores.begin());  it != inFrameStores.end();  ++it)
		if (!SetVideoFormat(inVideoFormat, inIsAJARetail, false, *it))
			errors++;
	return errors == 0;
}

// Method: GetVideoFormat	 
// Input:  NONE
// Output: NTV2VideoFormat
bool CNTV2Card::GetVideoFormat (NTV2VideoFormat & outValue, NTV2Channel inChannel)
{
	if (!IsMultiFormatActive ())
		inChannel = NTV2_CHANNEL1;

	NTV2Standard	standard;
	GetStandard (standard, inChannel);

	NTV2FrameGeometry frameGeometry;
	GetFrameGeometry (frameGeometry, inChannel);

	NTV2FrameRate frameRate;
	GetFrameRate (frameRate, inChannel);

	ULWord smpte372Enabled;
	GetSmpte372 (smpte372Enabled, inChannel);

	ULWord progressivePicture;
	GetProgressivePicture (progressivePicture);

    bool isSquares = false;
    if(NTV2_IS_QUAD_FRAME_GEOMETRY(frameGeometry))
    {
        if(!NTV2DeviceCanDo12gRouting(GetDeviceID()))
            isSquares = true;
        else
            Get4kSquaresEnable(isSquares, inChannel);
    }

    return ::NTV2DeviceGetVideoFormatFromState_Ex2 (&outValue, frameRate, frameGeometry, standard, smpte372Enabled, progressivePicture, isSquares);
}

bool CNTV2Card::GetSupportedVideoFormats (NTV2VideoFormatSet & outFormats)
{
	return ::NTV2DeviceGetSupportedVideoFormats (GetDeviceID (), outFormats);
}

//	--------------------------------------------	BEGIN BLOCK
//	GetNTV2VideoFormat functions
//		These static functions don't work correctly, and are subject to deprecation. They remain here only by our fiat.

NTV2VideoFormat CNTV2Card::GetNTV2VideoFormat(NTV2FrameRate frameRate, UByte inputGeometry, bool progressiveTransport, bool isThreeG, bool progressivePicture)
{
	NTV2Standard standard = GetNTV2StandardFromScanGeometry(inputGeometry, progressiveTransport);
	return GetNTV2VideoFormat(frameRate, standard, isThreeG, inputGeometry, progressivePicture);
}

NTV2VideoFormat CNTV2Card::GetNTV2VideoFormat (NTV2FrameRate frameRate, NTV2Standard standard, bool isThreeG, UByte inputGeometry, bool progressivePicture, bool isSquareDivision)
{
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;

	switch (standard)
	{
		case NTV2_STANDARD_525:
			switch (frameRate)
			{
				case NTV2_FRAMERATE_2997:	videoFormat = progressivePicture ? NTV2_FORMAT_525psf_2997 : NTV2_FORMAT_525_5994;		break;
				case NTV2_FRAMERATE_2400:	videoFormat = NTV2_FORMAT_525_2400;			break;
				case NTV2_FRAMERATE_2398:	videoFormat = NTV2_FORMAT_525_2398;			break;
				default:																break;
			}
			break;
	
		case NTV2_STANDARD_625:
			if (frameRate == NTV2_FRAMERATE_2500)
				videoFormat = progressivePicture ? NTV2_FORMAT_625psf_2500 : NTV2_FORMAT_625_5000;
			break;
	
		case NTV2_STANDARD_720:
			switch (frameRate)
			{
				case NTV2_FRAMERATE_6000:	videoFormat = NTV2_FORMAT_720p_6000;		break;
				case NTV2_FRAMERATE_5994:	videoFormat = NTV2_FORMAT_720p_5994;		break;
				case NTV2_FRAMERATE_5000:	videoFormat = NTV2_FORMAT_720p_5000;		break;
				case NTV2_FRAMERATE_2398:	videoFormat = NTV2_FORMAT_720p_2398;		break;
				default:					break;
			}
			break;
	
		case NTV2_STANDARD_1080:
		case NTV2_STANDARD_2Kx1080i:
			switch (frameRate)
			{
				case NTV2_FRAMERATE_3000:
					if (isThreeG && progressivePicture)
						videoFormat = NTV2_FORMAT_1080p_6000_B;
					else
						videoFormat = progressivePicture ? NTV2_FORMAT_1080psf_3000_2 : NTV2_FORMAT_1080i_6000;
					break;
				case NTV2_FRAMERATE_2997:
					if (isThreeG && progressivePicture)
						videoFormat = NTV2_FORMAT_1080p_5994_B;
					else
						videoFormat = progressivePicture ? NTV2_FORMAT_1080psf_2997_2 : NTV2_FORMAT_1080i_5994;
					break;
				case NTV2_FRAMERATE_2500:
					if (isThreeG && progressivePicture)
					{
						if (inputGeometry == 8)
							videoFormat = NTV2_FORMAT_1080psf_2K_2500;
						else
							videoFormat = NTV2_FORMAT_1080p_5000_B;
					}
					else if (inputGeometry == 8)
						videoFormat = NTV2_FORMAT_1080psf_2K_2500;
					else
						 videoFormat = progressivePicture ? NTV2_FORMAT_1080psf_2500_2 : NTV2_FORMAT_1080i_5000;
					break;
				case NTV2_FRAMERATE_2400:
					if (isThreeG)
					{
						videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_4800_B : NTV2_FORMAT_UNKNOWN;
					}
					else
					{
						videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080psf_2K_2400 : NTV2_FORMAT_1080psf_2400;
					}
					break;
				case NTV2_FRAMERATE_2398:
					if (isThreeG)
					{
						videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_4795_B : NTV2_FORMAT_UNKNOWN;
					}
					else
					{
						videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080psf_2K_2398 : NTV2_FORMAT_1080psf_2398;
					}
					break;
				default:	break;	// Unsupported
			}
			break;
	
		case NTV2_STANDARD_1080p:
		case NTV2_STANDARD_2Kx1080p:
			switch (frameRate)
			{
				case NTV2_FRAMERATE_12000:	videoFormat = NTV2_FORMAT_4x2048x1080p_12000;		break;		// There's no single quadrant raster defined for 120 Hz raw
				case NTV2_FRAMERATE_11988:	videoFormat = NTV2_FORMAT_4x2048x1080p_11988;		break;		// There's no single quadrant raster defined for 119.88 Hz raw
				case NTV2_FRAMERATE_6000:	videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_6000_A : NTV2_FORMAT_1080p_6000_A;			break;
				case NTV2_FRAMERATE_5994:	videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_5994_A : NTV2_FORMAT_1080p_5994_A;			break;
				case NTV2_FRAMERATE_4800:	videoFormat = NTV2_FORMAT_1080p_2K_4800_A;			break;		// There's no 1920 raster defined at 48 Hz
				case NTV2_FRAMERATE_4795:	videoFormat = NTV2_FORMAT_1080p_2K_4795_A;			break;		// There's no 1920 raster defined at 47.95 Hz
				case NTV2_FRAMERATE_5000:	videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_5000_A : NTV2_FORMAT_1080p_5000_A;			break;
				case NTV2_FRAMERATE_3000:	videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_3000 : NTV2_FORMAT_1080p_3000;				break;
				case NTV2_FRAMERATE_2997:	videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_2997 : NTV2_FORMAT_1080p_2997;				break;
				case NTV2_FRAMERATE_2500:	videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_2500 : NTV2_FORMAT_1080p_2500;				break;
				case NTV2_FRAMERATE_2400:	videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_2400 : NTV2_FORMAT_1080p_2400;				break;
				case NTV2_FRAMERATE_2398:	videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_2398 : NTV2_FORMAT_1080p_2398;				break;
				default:			break;	// Unsupported
			}
			break;
	
		case NTV2_STANDARD_2K:		// 2kx1556
			switch (frameRate)
			{
				case NTV2_FRAMERATE_1500:	videoFormat = NTV2_FORMAT_2K_1500;		break;
				case NTV2_FRAMERATE_1498:	videoFormat = NTV2_FORMAT_2K_1498;		break;
				case NTV2_FRAMERATE_2400:	videoFormat = NTV2_FORMAT_2K_2400;		break;
				case NTV2_FRAMERATE_2398:	videoFormat = NTV2_FORMAT_2K_2398;		break;
				case NTV2_FRAMERATE_2500:	videoFormat = NTV2_FORMAT_2K_2500;		break;
				default:					break;	// Unsupported
			}
			break;
	
		case NTV2_STANDARD_3840HFR:		videoFormat = NTV2_FORMAT_UNKNOWN;		break;
	
		case NTV2_STANDARD_3840x2160p:
			switch (frameRate)
			{
				case NTV2_FRAMERATE_2398:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2398 : NTV2_FORMAT_3840x2160p_2398;	break;
				case NTV2_FRAMERATE_2400:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2400 : NTV2_FORMAT_3840x2160p_2400;	break;
				case NTV2_FRAMERATE_2500:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2500 : NTV2_FORMAT_3840x2160p_2500;	break;
				case NTV2_FRAMERATE_2997:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2997 : NTV2_FORMAT_3840x2160p_2997;	break;
				case NTV2_FRAMERATE_3000:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_3000 : NTV2_FORMAT_3840x2160p_3000;	break;
				case NTV2_FRAMERATE_5000:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5000 : NTV2_FORMAT_3840x2160p_5000;	break;
				case NTV2_FRAMERATE_5994:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5994 : NTV2_FORMAT_3840x2160p_5994;	break;
				case NTV2_FRAMERATE_6000:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_6000 : NTV2_FORMAT_3840x2160p_6000;	break;
				default:					videoFormat = NTV2_FORMAT_UNKNOWN;				break;
			}
			break;
			
		case NTV2_STANDARD_3840i:
			switch (frameRate)
			{
				case NTV2_FRAMERATE_2398:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2398 : NTV2_FORMAT_3840x2160psf_2398;	break;
				case NTV2_FRAMERATE_2400:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2400 : NTV2_FORMAT_3840x2160psf_2400;	break;
				case NTV2_FRAMERATE_2500:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2500 : NTV2_FORMAT_3840x2160psf_2500;	break;
				case NTV2_FRAMERATE_2997:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2997 : NTV2_FORMAT_3840x2160psf_2997;	break;
				case NTV2_FRAMERATE_3000:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_3000 : NTV2_FORMAT_3840x2160psf_3000;	break;
				default:					videoFormat = NTV2_FORMAT_UNKNOWN;				break;
			}
			break;
	
		case NTV2_STANDARD_4096HFR:		videoFormat = NTV2_FORMAT_UNKNOWN;		break;
	
		case NTV2_STANDARD_4096x2160p:
			switch (frameRate)
			{
				case NTV2_FRAMERATE_2398:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2398 : NTV2_FORMAT_4096x2160p_2398;	break;
				case NTV2_FRAMERATE_2400:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2400 : NTV2_FORMAT_4096x2160p_2400;	break;
				case NTV2_FRAMERATE_2500:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2500 : NTV2_FORMAT_4096x2160p_2500;	break;
				case NTV2_FRAMERATE_2997:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2997 : NTV2_FORMAT_4096x2160p_2997;	break;
				case NTV2_FRAMERATE_3000:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_3000 : NTV2_FORMAT_4096x2160p_3000;	break;
				case NTV2_FRAMERATE_4795:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4795 : NTV2_FORMAT_4096x2160p_4795;	break;
				case NTV2_FRAMERATE_4800:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4800 : NTV2_FORMAT_4096x2160p_4800;	break;
				case NTV2_FRAMERATE_5000:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5000 : NTV2_FORMAT_4096x2160p_5000;	break;
				case NTV2_FRAMERATE_5994:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5994 : NTV2_FORMAT_4096x2160p_5994;	break;
				case NTV2_FRAMERATE_6000:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_6000 : NTV2_FORMAT_4096x2160p_6000;	break;
				default:					videoFormat = NTV2_FORMAT_UNKNOWN;				break;
			}
			break;
			
		case NTV2_STANDARD_4096i:
			switch (frameRate)
			{
				case NTV2_FRAMERATE_2398:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2398 : NTV2_FORMAT_4096x2160psf_2398;	break;
				case NTV2_FRAMERATE_2400:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2400 : NTV2_FORMAT_4096x2160psf_2400;	break;
				case NTV2_FRAMERATE_2500:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2500 : NTV2_FORMAT_4096x2160psf_2500;	break;
				case NTV2_FRAMERATE_2997:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2997 : NTV2_FORMAT_4096x2160psf_2997;	break;
				case NTV2_FRAMERATE_3000:	videoFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_3000 : NTV2_FORMAT_4096x2160psf_3000;	break;
				default:					videoFormat = NTV2_FORMAT_UNKNOWN;				break;
			}
			break;
	
		case NTV2_STANDARD_7680:
			switch(frameRate)
			{
				case NTV2_FRAMERATE_2398:	videoFormat = NTV2_FORMAT_4x3840x2160p_2398;	break;
				case NTV2_FRAMERATE_2400:	videoFormat = NTV2_FORMAT_4x3840x2160p_2400;	break;
				case NTV2_FRAMERATE_2500:	videoFormat = NTV2_FORMAT_4x3840x2160p_2500;	break;
				case NTV2_FRAMERATE_2997:	videoFormat = NTV2_FORMAT_4x3840x2160p_2997;	break;
				case NTV2_FRAMERATE_3000:	videoFormat = NTV2_FORMAT_4x3840x2160p_3000;	break;
				case NTV2_FRAMERATE_5000:	videoFormat = NTV2_FORMAT_4x3840x2160p_5000;	break;
				case NTV2_FRAMERATE_5994:	videoFormat = NTV2_FORMAT_4x3840x2160p_5994;	break;
				case NTV2_FRAMERATE_6000:	videoFormat = NTV2_FORMAT_4x3840x2160p_6000;	break;
				default:					videoFormat = NTV2_FORMAT_UNKNOWN;
			}
			break;
			
		case NTV2_STANDARD_8192:
			switch(frameRate)
			{
				case NTV2_FRAMERATE_2398:	videoFormat = NTV2_FORMAT_4x4096x2160p_2398;	break;
				case NTV2_FRAMERATE_2400:	videoFormat = NTV2_FORMAT_4x4096x2160p_2400;	break;
				case NTV2_FRAMERATE_2500:	videoFormat = NTV2_FORMAT_4x4096x2160p_2500;	break;
				case NTV2_FRAMERATE_2997:	videoFormat = NTV2_FORMAT_4x4096x2160p_2997;	break;
				case NTV2_FRAMERATE_3000:	videoFormat = NTV2_FORMAT_4x4096x2160p_3000;	break;
				case NTV2_FRAMERATE_4795:	videoFormat = NTV2_FORMAT_4x4096x2160p_4795;	break;
				case NTV2_FRAMERATE_4800:	videoFormat = NTV2_FORMAT_4x4096x2160p_4800;	break;
				case NTV2_FRAMERATE_5000:	videoFormat = NTV2_FORMAT_4x4096x2160p_5000;	break;
				case NTV2_FRAMERATE_5994:	videoFormat = NTV2_FORMAT_4x4096x2160p_5994;	break;
				case NTV2_FRAMERATE_6000:	videoFormat = NTV2_FORMAT_4x4096x2160p_6000;	break;
				default:					videoFormat = NTV2_FORMAT_UNKNOWN;
			}
			break;

		case NTV2_STANDARD_UNDEFINED:
			videoFormat = NTV2_FORMAT_UNKNOWN;
			break;
	}

	return videoFormat;

}	//	GetNTV2VideoFormat

//	--------------------------------------------	END BLOCK

#if !defined(NTV2_DEPRECATE_14_3)
	bool CNTV2Card::GetNominalMinMaxHV (int * pOutNominalH, int * pOutMinH, int * pOutMaxH, int * pOutNominalV, int * pOutMinV, int * pOutMaxV)
	{
		return pOutNominalH  &&  pOutMinH  &&  pOutMaxH  &&  pOutNominalV  &&  pOutMinV  &&  pOutMaxV
				&& GetNominalMinMaxHV(*pOutNominalH, *pOutMinH, *pOutMaxH, *pOutNominalV, *pOutMinV, *pOutMaxV);
	}
#endif	//	NTV2_DEPRECATE_14_3

bool CNTV2Card::GetNominalMinMaxHV (int & outNominalH, int & outMinH, int & outMaxH, int & outNominalV, int & outMinV, int & outMaxV)
{
	NTV2VideoFormat		videoFormat;
	// Get current video format to find nominal value
	if (GetVideoFormat(videoFormat))
	{
		// Kona2 has static nominal H value for all formats
		outNominalH = K2_NOMINAL_H;
		outMinH = K2_MIN_H;
		outMaxH = K2_MAX_H;

		outNominalV = K2_NOMINAL_V;
		outMinV = K2_MIN_V;
		outMaxV = K2_MAX_V;
		return true;
	}
	return false;
}

bool CNTV2Card::SetVideoHOffset (int hOffset, const UWord inOutputSpigot)
{
	int				nominalH(0), minH(0), maxH(0), nominalV(0), minV(0), maxV(0), count(0);
	ULWord			timingValue(0), lineCount(0), lineCount2(0);
	NTV2DeviceID	boardID(GetDeviceID());

	// Get the nominal values for H and V
	if (!GetNominalMinMaxHV(nominalH, minH, maxH, nominalV, minV, maxV))
		return false;

	// Apply offset to nominal value (K2 you increment the timing by adding the timing value)
	if (::NTV2DeviceNeedsRoutingSetup(boardID))
		nominalH = nominalH + hOffset;
	else
		nominalH = nominalH - hOffset;
		
	// Clamp this value so that it doesn't exceed max, min 
	if (nominalH > maxH)
		nominalH = maxH;
	else if (nominalH < minH)
		nominalH = minH;

	if (!ReadOutputTimingControl(timingValue, inOutputSpigot))
		return false;

	// Handle special cases where we increment or decrement the timing by one
	// Some hardware LS/LH cannot handle this and inorder to do 1 pixel timing moves
	// we need to move by 3 then subtract 2, or subtract 3 then add 2 depending on 
	// direction.  Also we need to wait at least one horizontal line between the values
	// we set.

	if (LWord((timingValue & 0x0000FFFF)) == nominalH)
		return true;	//	Nothing to change
	if (((LWord((timingValue & 0x0000FFFF)) + 1) == nominalH) )
	{
		// Add 3 to the timing value. Note that nominalH is already advanced by one so
		// we just need to add 2.
		timingValue &= 0xFFFF0000;
		timingValue |= ULWord(nominalH + 2);
		WriteOutputTimingControl(timingValue, inOutputSpigot);

		// Wait a scanline
		count = 0;
		ReadLineCount (lineCount);
		do
		{	
			ReadLineCount (lineCount2);
			if (count > 1000000) return false;
			count++;
		} while (lineCount == lineCount2);

		// Now move timing back by 2.
		timingValue -= 2;
	}
	else if ( ((LWord((timingValue & 0x0000FFFF)) -1) == nominalH ) )
	{
		// Subtract 3 from the timing value. Note that nominalH is already decremented by one so
		// we just need to subtract 2.
		timingValue &= 0xFFFF0000;
		timingValue |= ULWord(nominalH - 2);
		WriteOutputTimingControl(timingValue, inOutputSpigot);

		// Wait a scanline
		count = 0;
		ReadLineCount (lineCount);
		do
		{	
			ReadLineCount (lineCount2);
			if (count > 1000000) return false;
			count++;
		} while (lineCount == lineCount2);				
		
		// Now move timing forward by 2.
		timingValue += 2;
	}
	else
	{
		// Setting arbitrary value so we don't need to do the +3-2 or -3+2 trick and
		// we can just set the new value
		timingValue &= 0xFFFF0000;
		timingValue |= ULWord(nominalH);
	}
	return WriteOutputTimingControl(timingValue, inOutputSpigot);
}	//	SetVideoHOffset


bool CNTV2Card::GetVideoHOffset (int & outHOffset,  const UWord inOutputSpigot)
{
	int	nominalH(0), minH(0), maxH(0), nominalV(0), minV(0), maxV(0);

	//	Get the nominal values for H and V...
	if (!GetNominalMinMaxHV(nominalH, minH, maxH, nominalV, minV, maxV))
		return false;

	ULWord	timingValue(0);
	if (!ReadOutputTimingControl(timingValue, inOutputSpigot))
		return false;
	timingValue &= 0xFFFF;	//	lower 16 bits has H timing value -- 0x1000 is nominal
	
	// Get offset from nominal value (K2 you increment the timing by adding the timing value)
	if (::NTV2DeviceNeedsRoutingSetup(GetDeviceID()))
		outHOffset = int(timingValue) - nominalH;
	else
		outHOffset = nominalH - int(timingValue);
	return true; 
}

bool CNTV2Card::SetVideoVOffset (int vOffset, const UWord inOutputSpigot)
{
	int	nominalH(0), minH(0), maxH(0), nominalV(0), minV(0), maxV(0);

	// Get the nominal values for H and V
	if (!GetNominalMinMaxHV(nominalH, minH, maxH, nominalV, minV, maxV))
		return false;

	// Apply offset to nominal value (K2 you increment the timing by adding the timing value)
	if (::NTV2DeviceNeedsRoutingSetup(GetDeviceID()))
		nominalV = nominalV + vOffset;
	else
		nominalV = nominalV - vOffset;
		
	// Clamp this value so that it doesn't exceed max, min 
	if (nominalV > maxV)
		nominalV = maxV;
	else if (nominalV < minV)
		nominalV = minV;

	ULWord	timingValue(0);
	if (!ReadOutputTimingControl(timingValue, inOutputSpigot))
		return false;
	timingValue &= 0x0000FFFF;				//	Clear V value, keep H value
	return WriteOutputTimingControl(timingValue | ULWord(nominalV << 16), inOutputSpigot);	//	Change only the V value
}

bool CNTV2Card::GetVideoVOffset (int & outVOffset, const UWord inOutputSpigot)
{
	int	nominalH(0), minH(0), maxH(0), nominalV(0), minV(0), maxV(0);
	
	// Get the nominal values for H and V
	if (!GetNominalMinMaxHV(nominalH, minH, maxH, nominalV, minV, maxV))
		return false;

	ULWord	timingValue(0);
	if (!ReadOutputTimingControl(timingValue, inOutputSpigot))
		return false;
	timingValue = (timingValue >> 16);

	// Get offset from nominal value (K2 you increment the timing by adding the timing value)
	if (::NTV2DeviceNeedsRoutingSetup(GetDeviceID()))
		outVOffset = int(timingValue) - nominalV;
	else
		outVOffset = nominalV - int(timingValue);
	return true; 
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::SetVideoFinePhase (int fOffset)		{return WriteRegister (kRegOutputTimingFinePhase, fOffset, kRegMaskOutputTimingFinePhase, kRegShiftOutputTimingFinePhase);}
	bool CNTV2Card::GetVideoFinePhase (int* fOffset)	{return ReadRegister (kRegOutputTimingFinePhase, (ULWord*) fOffset, kRegMaskOutputTimingFinePhase, kRegShiftOutputTimingFinePhase);}
#endif	//	!defined (NTV2_DEPRECATE)


bool CNTV2Card::GetNumberActiveLines (ULWord & outNumActiveLines)
{	
	NTV2Standard	standard	(NTV2_STANDARD_INVALID);

 	outNumActiveLines = 0;
	if (!GetStandard(standard))
		return false;

	switch (standard)
	{
		case NTV2_STANDARD_525:			outNumActiveLines = NUMACTIVELINES_525;			break;

		case NTV2_STANDARD_625:			outNumActiveLines = NUMACTIVELINES_625;			break;

		case NTV2_STANDARD_720:			outNumActiveLines = HD_NUMACTIVELINES_720;		break;

		case NTV2_STANDARD_2K:			outNumActiveLines = HD_NUMACTIVELINES_2K;		break;

		case NTV2_STANDARD_1080:
		case NTV2_STANDARD_1080p:
		case NTV2_STANDARD_2Kx1080p:
		case NTV2_STANDARD_2Kx1080i:	outNumActiveLines = HD_NUMACTIVELINES_1080;		break;

		case NTV2_STANDARD_3840x2160p:
		case NTV2_STANDARD_4096x2160p:
		case NTV2_STANDARD_3840HFR:
		case NTV2_STANDARD_4096HFR:
		case NTV2_STANDARD_3840i:
		case NTV2_STANDARD_4096i:		outNumActiveLines = HD_NUMLINES_4K;				break;

		case NTV2_STANDARD_7680:
		case NTV2_STANDARD_8192:		outNumActiveLines = FD_NUMLINES_8K;				break;
	#if defined(_DEBUG)
		case NTV2_NUM_STANDARDS:		outNumActiveLines = 0;							break;
	#else
		default:						outNumActiveLines = 0;							break;
	#endif
	}
	return outNumActiveLines != 0;
}

bool CNTV2Card::GetActiveFrameDimensions (NTV2FrameDimensions & outFrameDimensions, const NTV2Channel inChannel)
{
	outFrameDimensions = GetActiveFrameDimensions (inChannel);
	return outFrameDimensions.IsValid ();
}


NTV2FrameDimensions CNTV2Card::GetActiveFrameDimensions (const NTV2Channel inChannel)
{
	NTV2Standard		standard	(NTV2_STANDARD_INVALID);
	NTV2FrameGeometry	geometry	(NTV2_FG_INVALID);
	NTV2FrameDimensions	result;

	if (IsXilinxProgrammed()	//	If Xilinx not programmed, prevent returned size from being 4096 x 4096
		&& GetStandard(standard, inChannel)
			&& GetFrameGeometry(geometry, inChannel))
				switch (standard)
				{
					case NTV2_STANDARD_1080:
					case NTV2_STANDARD_1080p:
						result.SetWidth(geometry == NTV2_FG_2048x1080 || geometry == NTV2_FG_4x2048x1080  ?  HD_NUMCOMPONENTPIXELS_1080_2K  :  HD_NUMCOMPONENTPIXELS_1080);
						result.SetHeight(HD_NUMACTIVELINES_1080);
						if (NTV2_IS_QUAD_FRAME_GEOMETRY(geometry))
							result.Set(result.Width()*2,  HD_NUMLINES_4K);
						else if (NTV2_IS_QUAD_QUAD_FRAME_GEOMETRY(geometry))
							result.Set(result.Width()*4,  FD_NUMLINES_8K);
						break;
					case NTV2_STANDARD_720:			result.Set(HD_NUMCOMPONENTPIXELS_720,		HD_NUMACTIVELINES_720);		break;
					case NTV2_STANDARD_525:			result.Set(NUMCOMPONENTPIXELS,				NUMACTIVELINES_525);		break;
					case NTV2_STANDARD_625:			result.Set(NUMCOMPONENTPIXELS,				NUMACTIVELINES_625);		break;
					case NTV2_STANDARD_2K:			result.Set(HD_NUMCOMPONENTPIXELS_2K,		HD_NUMLINES_2K);			break;
					case NTV2_STANDARD_2Kx1080p:	result.Set(HD_NUMCOMPONENTPIXELS_1080_2K,	HD_NUMACTIVELINES_1080);	break;
					case NTV2_STANDARD_2Kx1080i:	result.Set(HD_NUMCOMPONENTPIXELS_1080_2K,	HD_NUMACTIVELINES_1080);	break;
					case NTV2_STANDARD_3840x2160p:	result.Set(HD_NUMCOMPONENTPIXELS_1080*2,	HD_NUMLINES_4K);			break;
					case NTV2_STANDARD_4096x2160p:	result.Set(HD_NUMCOMPONENTPIXELS_1080_2K*2,	HD_NUMLINES_4K);			break;
					case NTV2_STANDARD_3840HFR:		result.Set(HD_NUMCOMPONENTPIXELS_1080*2,	HD_NUMLINES_4K);			break;
					case NTV2_STANDARD_4096HFR:		result.Set(HD_NUMCOMPONENTPIXELS_1080_2K*2,	HD_NUMLINES_4K);			break;
					case NTV2_STANDARD_7680:		result.Set(HD_NUMCOMPONENTPIXELS_1080*4,	FD_NUMLINES_8K);			break;
					case NTV2_STANDARD_8192:		result.Set(HD_NUMCOMPONENTPIXELS_1080_2K*4,	FD_NUMLINES_8K);			break;
					case NTV2_STANDARD_3840i:		result.Set(HD_NUMCOMPONENTPIXELS_1080*2,	HD_NUMLINES_4K);			break;
					case NTV2_STANDARD_4096i:		result.Set(HD_NUMCOMPONENTPIXELS_1080_2K*2,	HD_NUMLINES_4K);			break;
				#if defined(_DEBUG)
					case NTV2_NUM_STANDARDS:																				break;
				#else
					default:																								break;
				#endif
				}

	return result;
}

#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::GetActiveFramebufferSize (SIZE * pOutFrameDimensions, const NTV2Channel inChannel)
	{
		const NTV2FrameDimensions	fd	(GetActiveFrameDimensions (inChannel));
		if (pOutFrameDimensions && fd.IsValid ())
		{
			pOutFrameDimensions->cx = fd.Width ();
			pOutFrameDimensions->cy = fd.Height ();
			return true;
		}
		return false;
	}
#endif	//	!defined (NTV2_DEPRECATE)


// Method: SetStandard
// Input:  NTV2Standard
// Output: NONE
bool CNTV2Card::SetStandard (NTV2Standard value, NTV2Channel channel)
{
	if (!IsMultiFormatActive ())
		channel = NTV2_CHANNEL1;
	NTV2Standard newStandard = value;
	if (NTV2_IS_QUAD_QUAD_STANDARD(newStandard))
	{
		newStandard = GetQuarterSizedStandard(newStandard);
	}
	if (NTV2_IS_QUAD_STANDARD(newStandard))
	{
		newStandard = GetQuarterSizedStandard(newStandard);
	}
	if (NTV2_IS_2K1080_STANDARD(newStandard))
	{
		newStandard = NTV2_IS_PROGRESSIVE_STANDARD(newStandard) ? NTV2_STANDARD_1080p : NTV2_STANDARD_1080;
	}

	return WriteRegister (gChannelToGlobalControlRegNum [channel],
						  newStandard,
						  kRegMaskStandard,
						  kRegShiftStandard);
}

// Method: GetStandard	  
// Input:  NONE
// Output: NTV2Standard
bool CNTV2Card::GetStandard (NTV2Standard & outValue, NTV2Channel inChannel)
{
	if (!IsMultiFormatActive ())
		inChannel = NTV2_CHANNEL1;
	bool status = CNTV2DriverInterface::ReadRegister (gChannelToGlobalControlRegNum[inChannel], outValue, kRegMaskStandard, kRegShiftStandard);
	if (status && ::NTV2DeviceCanDo4KVideo(_boardID))// || NTV2DeviceCanDo425Mux(_boardID)))
	{
		bool	quadFrameEnabled(false);
		status = GetQuadFrameEnable(quadFrameEnabled, inChannel);
		if (status  &&  quadFrameEnabled)
			outValue = Get4xSizedStandard(outValue);
		if( status && NTV2DeviceCanDo8KVideo(_boardID))
		{
			bool quadQuadFrameEnabled(false);
			status = GetQuadQuadFrameEnable(quadQuadFrameEnabled);
			if(status && quadQuadFrameEnabled)
				outValue = Get4xSizedStandard(outValue);
		}
	}
	return status;
}

// Method: IsProgressiveStandard	
// Input:  NONE
// Output: bool
bool CNTV2Card::IsProgressiveStandard (bool & outIsProgressive, NTV2Channel inChannel)
{
	ULWord			smpte372Enabled	(0);
	NTV2Standard	standard		(NTV2_STANDARD_INVALID);
	outIsProgressive = false;

	if (!IsMultiFormatActive())
		inChannel = NTV2_CHANNEL1;

	if (GetStandard (standard, inChannel) && GetSmpte372 (smpte372Enabled, inChannel))
	{	
		if (standard == NTV2_STANDARD_720 || standard == NTV2_STANDARD_1080p || smpte372Enabled)
			outIsProgressive = true;
		return true;
	}
	return false;
}

// Method: IsSDStandard	
// Input:  NONE
// Output: bool
bool CNTV2Card::IsSDStandard (bool & outIsStandardDef, NTV2Channel inChannel)
{
	NTV2Standard	standard	(NTV2_STANDARD_INVALID);
	outIsStandardDef = false;

	if (!IsMultiFormatActive())
		inChannel = NTV2_CHANNEL1;

	if (GetStandard (standard, inChannel))
	{
		if (standard == NTV2_STANDARD_525 || standard == NTV2_STANDARD_625)
			outIsStandardDef = true;   // SD standard
		return true;
	}
	return false;
}

#if !defined (NTV2_DEPRECATE)
	// Method: IsSDVideoADCMode
	// Input:  NTV2LSVideoADCMode
	// Output: bool
	bool CNTV2Card::IsSDVideoADCMode (NTV2LSVideoADCMode mode)
	{
		switch(mode)
		{
			case NTV2K2_480iADCComponentBeta:
			case NTV2K2_480iADCComponentSMPTE:
			case NTV2K2_480iADCSVideoUS:
			case NTV2K2_480iADCCompositeUS:
			case NTV2K2_480iADCComponentBetaJapan:
			case NTV2K2_480iADCComponentSMPTEJapan:
			case NTV2K2_480iADCSVideoJapan:
			case NTV2K2_480iADCCompositeJapan:
			case NTV2K2_576iADCComponentBeta:
			case NTV2K2_576iADCComponentSMPTE:
			case NTV2K2_576iADCSVideo:
			case NTV2K2_576iADCComposite:
				return true;

			default: 
				return false;
		}
	}

	// Method: IsHDVideoADCMode	
	// Input:  NTV2LSVideoADCMode
	// Output: bool
	bool CNTV2Card::IsHDVideoADCMode (NTV2LSVideoADCMode mode)
	{
		return !IsSDVideoADCMode(mode);
	}
#endif	//	!defined (NTV2_DEPRECATE)


// Method: SetFrameGeometry
// Input:  NTV2FrameGeometry
// Output: NONE
bool CNTV2Card::SetFrameGeometry (NTV2FrameGeometry value, bool ajaRetail, NTV2Channel channel)
{
#ifdef  MSWindows
	NTV2EveryFrameTaskMode mode;
	GetEveryFrameServices(mode);
	if(mode == NTV2_STANDARD_TASKS)
		ajaRetail = true;
#else
	(void) ajaRetail;
#endif
	if (!IsMultiFormatActive ())
		channel = NTV2_CHANNEL1;

	const ULWord			regNum		(gChannelToGlobalControlRegNum [channel]);
	const NTV2FrameGeometry	newGeometry	(value);
	NTV2FrameBufferFormat	format		(NTV2_FBF_NUMFRAMEBUFFERFORMATS);

	// This is the geometry of a single FB widget. Note this is the same as newGeometry unless it is a 4K format
	NTV2FrameGeometry newFrameStoreGeometry = newGeometry;
	NTV2FrameGeometry oldGeometry;
	bool status = GetFrameGeometry(oldGeometry, channel);
	if ( !status )
		return status;
		
	status = GetLargestFrameBufferFormatInUse(format);
	if ( !status )
		return status;
	
	// If quad frame geometry, each of 4 frame buffers geometry is one quarter of full size
	if (::NTV2DeviceCanDo4KVideo(_boardID))
	{
		newFrameStoreGeometry = newGeometry;
		if (NTV2_IS_QUAD_QUAD_FRAME_GEOMETRY(newFrameStoreGeometry))
		{
			newFrameStoreGeometry = GetQuarterSizedGeometry(newFrameStoreGeometry);
		}
		if (NTV2_IS_QUAD_FRAME_GEOMETRY(newFrameStoreGeometry))
		{
			newFrameStoreGeometry = GetQuarterSizedGeometry(newFrameStoreGeometry);
		}
	}

	ULWord oldFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID, oldGeometry, format);
	ULWord newFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID, newGeometry, format);
	bool changeBufferSize =	::NTV2DeviceCanChangeFrameBufferSize(_boardID) && (oldFrameBufferSize != newFrameBufferSize);

	status = WriteRegister (regNum, newFrameStoreGeometry, kRegMaskGeometry, kRegShiftGeometry);

	// If software set the frame buffer size, read the values from hardware
	if ( GetFBSizeAndCountFromHW(&_ulFrameBufferSize, &_ulNumFrameBuffers) )
	{
		changeBufferSize = false;
	}

	// Unfortunately, on boards that support 2k, the
	// numframebuffers and framebuffersize might need to
	// change.
	if ( changeBufferSize  )
	{
		_ulFrameBufferSize = newFrameBufferSize;
		_ulNumFrameBuffers = ::NTV2DeviceGetNumberFrameBuffers(_boardID, newGeometry, format);
	}
		
	return status;
}

// Method: GetFrameGeometry
// Input:  NONE
// Output: NTV2FrameGeometry
bool CNTV2Card::GetFrameGeometry (NTV2FrameGeometry & outValue, NTV2Channel inChannel)
{
	outValue = NTV2_FG_INVALID;
	if (!IsMultiFormatActive())
		inChannel = NTV2_CHANNEL1;
	else if (IS_CHANNEL_INVALID(inChannel))
		return false;

	bool status = CNTV2DriverInterface::ReadRegister (gChannelToGlobalControlRegNum[inChannel], outValue, kRegMaskGeometry, kRegShiftGeometry);
	// special case for quad-frame (4 frame buffer) geometry
	if (status  &&  (::NTV2DeviceCanDo4KVideo(_boardID) || NTV2DeviceCanDo425Mux(_boardID)))
	{
		bool	quadFrameEnabled(false);
		status = GetQuadFrameEnable(quadFrameEnabled, inChannel);
		if (status  &&  quadFrameEnabled)
			outValue = Get4xSizedGeometry(outValue);
		if( status && NTV2DeviceCanDo8KVideo(_boardID))
		{
			bool quadQuadFrameEnabled(false);
			status = GetQuadQuadFrameEnable(quadQuadFrameEnabled);
			if(status && quadQuadFrameEnabled)
				outValue = Get4xSizedGeometry(outValue);
		}
	}
	return status;
}

// Method: SetFramerate
// Input:  NTV2FrameRate
// Output: NONE
bool CNTV2Card::SetFrameRate (NTV2FrameRate value, NTV2Channel channel)
{
	const ULWord	loValue	(value & 0x7);
	const ULWord	hiValue	((value & 0x8) >> 3);

	if (!IsMultiFormatActive ())
		channel = NTV2_CHANNEL1;

	return WriteRegister (gChannelToGlobalControlRegNum [channel], loValue, kRegMaskFrameRate, kRegShiftFrameRate) &&
			WriteRegister (gChannelToGlobalControlRegNum [channel], hiValue, kRegMaskFrameRateHiBit, kRegShiftFrameRateHiBit);
}

// Method: GetFrameRate
// Input:  NONE
// Output: NTV2FrameRate
bool CNTV2Card::GetFrameRate (NTV2FrameRate & outValue, NTV2Channel inChannel)
{
	ULWord	returnVal1 (0), returnVal2 (0);
	outValue = NTV2_FRAMERATE_UNKNOWN;
	if (!IsMultiFormatActive())
		inChannel = NTV2_CHANNEL1;
	else if (IS_CHANNEL_INVALID(inChannel))
		return false;

	if (ReadRegister (gChannelToGlobalControlRegNum[inChannel], returnVal1, kRegMaskFrameRate, kRegShiftFrameRate) &&
		ReadRegister (gChannelToGlobalControlRegNum[inChannel], returnVal2, kRegMaskFrameRateHiBit, kRegShiftFrameRateHiBit))
	{
			outValue = static_cast <NTV2FrameRate> ((returnVal1 & 0x7) | ((returnVal2 & 0x1) << 3));
			return true;
	}
	return false;
}

// Method: SetSmpte372
bool CNTV2Card::SetSmpte372 (ULWord inValue, NTV2Channel inChannel)
{
	// Set true (1) to put card in SMPTE 372 dual-link mode (used for 1080p60, 1080p5994, 1080p50)
	// Set false (0) to disable this mode

	if (!IsMultiFormatActive())
		inChannel = NTV2_CHANNEL1;

	return WriteRegister (gChannelToSmpte372RegisterNum [inChannel], inValue, gChannelToSmpte372Masks [inChannel], gChannelToSmpte372Shifts [inChannel]);
}

// Method: GetSmpte372
bool CNTV2Card::GetSmpte372 (ULWord & outValue, NTV2Channel inChannel)
{
	// Return true (1) if card in SMPTE 372 dual-link mode (used for 1080p60, 1080p5994, 1080p50)
	// Return false (0) if this mode is disabled
	if (!IsMultiFormatActive())
		inChannel = NTV2_CHANNEL1;

	return ReadRegister (gChannelToSmpte372RegisterNum[inChannel], outValue, gChannelToSmpte372Masks[inChannel], gChannelToSmpte372Shifts[inChannel]);
}

// Method: SetProgressivePicture
// Input:  bool	--	Set true (1) to put card video format into progressive mode (used for 1080psf25 vs 1080i50, etc);
//					Set false (0) to disable this mode
// Output: NONE
bool CNTV2Card::SetProgressivePicture (ULWord value)
{
	return WriteRegister (kVRegProgressivePicture, value);
}

// Method: GetProgressivePicture
// Input:  NONE
// Output: bool	--	Return true (1) if card video format is progressive (used for 1080psf25 vs 1080i50, etc);
//					Return false (0) if this mode is disabled
bool CNTV2Card::GetProgressivePicture (ULWord & outValue)
{
	ULWord	returnVal	(0);
	bool	result1		(ReadRegister (kVRegProgressivePicture, returnVal));
	outValue = result1 ? returnVal : 0;
	return result1;
}

// Method: SetQuadFrameEnable
// Input:  bool
// Output: NONE
bool CNTV2Card::SetQuadFrameEnable (const bool inEnable, const NTV2Channel inChannel)
{
	bool ok(NTV2_IS_VALID_CHANNEL(inChannel) && ::NTV2DeviceCanDo4KVideo(_boardID));

	// Set true (1) to enable a Quad Frame Geometry
	// Set false (0) to disable this mode
	if (inEnable)
	{
		if (::NTV2DeviceCanDo12gRouting(_boardID))
		{
			if(ok)	ok = SetTsiFrameEnable(true, inChannel);
		}
		else if(::NTV2DeviceCanDo425Mux(_boardID))
		{
			if(ok)	ok = SetTsiFrameEnable(true, inChannel);
		}
		else
		{
			if(ok)	ok = Set4kSquaresEnable(true, inChannel);
		}
	}
	else
	{
		SetTsiFrameEnable(false, inChannel);
		Set4kSquaresEnable(false, inChannel);
	}
	return ok;
}

bool CNTV2Card::SetQuadQuadFrameEnable (const bool inEnable, const NTV2Channel inChannel)
{
	bool ok(NTV2_IS_VALID_CHANNEL(inChannel) && ::NTV2DeviceCanDo8KVideo(_boardID));
	if (inEnable)
	{
		if (!IsMultiFormatActive())
		{
			if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL1);
			if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL2);
			if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL3);
			if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL4);
		}
		else if (inChannel < NTV2_CHANNEL3)
		{
			if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL1);
			if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL2);
		}
		else if (inChannel < NTV2_CHANNEL5)
		{
			if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL3);
			if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL4);
		}
	}
	else
	{
		if(ok)	ok = SetQuadQuadSquaresEnable(false, inChannel);
	}

	if(!IsMultiFormatActive())
	{
		WriteRegister(kRegGlobalControl3, ULWord(inEnable ? 1 : 0), kRegMaskQuadQuadMode, kRegShiftQuadQuadMode);
		WriteRegister(kRegGlobalControl3, ULWord(inEnable ? 1 : 0), kRegMaskQuadQuadMode2, kRegShiftQuadQuadMode2);		
	}
	else
	{
		if (ok)	ok = WriteRegister(kRegGlobalControl3, ULWord(inEnable ? 1 : 0), (inChannel < NTV2_CHANNEL3) ? kRegMaskQuadQuadMode : kRegMaskQuadQuadMode2, (inChannel < NTV2_CHANNEL3) ? kRegShiftQuadQuadMode : kRegShiftQuadQuadMode2);
	}
	if (inEnable)
	{
		if (inChannel < NTV2_CHANNEL3)
		{
			if (ok)	ok = CopyVideoFormat(inChannel, NTV2_CHANNEL1, NTV2_CHANNEL2);
		}
		else
		{
			if (ok)	ok = CopyVideoFormat(inChannel, NTV2_CHANNEL3, NTV2_CHANNEL4);
		}
	}
	return ok;
}

bool CNTV2Card::SetQuadQuadSquaresEnable (const bool inValue, const NTV2Channel inChannel)
{
	(void)inChannel;
	bool ok(::NTV2DeviceCanDo8KVideo(_boardID));
	if (inValue)
	{
		if (ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL1);
		if (ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL2);
		if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL3);
		if(ok)	ok = SetQuadFrameEnable(true, NTV2_CHANNEL4);
		if(ok)	ok = SetQuadQuadFrameEnable(true, NTV2_CHANNEL1);
		if(ok)	ok = SetQuadQuadFrameEnable(true, NTV2_CHANNEL3);
	}
	if(ok)	ok = WriteRegister(kRegGlobalControl3, ULWord(inValue ? 1 : 0), kRegMaskQuadQuadSquaresMode, kRegShiftQuadQuadSquaresMode);
	return ok;
}

bool CNTV2Card::GetQuadQuadSquaresEnable (bool & outValue, const NTV2Channel inChannel)
{
	(void)inChannel;
	if (!::NTV2DeviceCanDo8KVideo(_boardID))
		return false;

	return CNTV2DriverInterface::ReadRegister(kRegGlobalControl3, outValue, kRegMaskQuadQuadSquaresMode, kRegShiftQuadQuadSquaresMode);
}

// Method: GetQuadFrameEnable
// Input:  NONE
// Output: bool
bool CNTV2Card::GetQuadFrameEnable (bool & outValue, const NTV2Channel inChannel)
{
	// Return true (1) Quad Frame Geometry is enabled
	// Return false (0) if this mode is disabled
	bool	quadEnabled	(0);
	bool	status2		(true);
	bool	s425Enabled	(false);
	bool	status1 = Get4kSquaresEnable (quadEnabled, inChannel);
	if (::NTV2DeviceCanDo425Mux (_boardID) || ::NTV2DeviceCanDo12gRouting(_boardID))
		status2 = GetTsiFrameEnable (s425Enabled, inChannel);

	outValue = (status1 & status2) ? ((quadEnabled | s425Enabled) ? true : false) : false;
	return status1;
}

bool CNTV2Card::GetQuadQuadFrameEnable (bool & outValue, const NTV2Channel inChannel)
{
	(void)inChannel;
	outValue = 0;
	if (::NTV2DeviceCanDo8KVideo(_boardID))
	{
		if (inChannel < NTV2_CHANNEL3)
			return CNTV2DriverInterface::ReadRegister(kRegGlobalControl3, outValue, kRegMaskQuadQuadMode, kRegShiftQuadQuadMode);
		else
			return CNTV2DriverInterface::ReadRegister(kRegGlobalControl3, outValue, kRegMaskQuadQuadMode2, kRegShiftQuadQuadMode2);
	}
	return true;
}

bool CNTV2Card::Set4kSquaresEnable (const bool inEnable, NTV2Channel inChannel)
{
	bool status = true;

	if(!::NTV2DeviceCanDo4KVideo(_boardID))
		return false;
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;

	if (inEnable)
	{
		// enable quad frame, disable 425
		if (!IsMultiFormatActive())
		{
			status = WriteRegister(kRegGlobalControl2, 1, kRegMaskQuadMode, kRegShiftQuadMode) &&
				WriteRegister(kRegGlobalControl2, 1, kRegMaskQuadMode2, kRegShiftQuadMode2) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB12, kRegShift425FB12) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB34, kRegShift425FB34) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB56, kRegShift425FB56) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB78, kRegShift425FB78) &&
				WriteRegister(kRegGlobalControl, 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
				WriteRegister(kRegGlobalControlCh2, 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
				WriteRegister(kRegGlobalControlCh3, 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
				WriteRegister(kRegGlobalControlCh4, 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable);
				CopyVideoFormat(inChannel, NTV2_CHANNEL1, NTV2_CHANNEL8);
		}
		else if (inChannel < NTV2_CHANNEL5)
		{
			status = WriteRegister (kRegGlobalControl2, 1, kRegMaskQuadMode, kRegShiftQuadMode) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB12, kRegShift425FB12) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB34, kRegShift425FB34) &&
				CopyVideoFormat(inChannel, NTV2_CHANNEL1, NTV2_CHANNEL4);
		}
		else
		{
			status = WriteRegister(kRegGlobalControl2, 1, kRegMaskQuadMode2, kRegShiftQuadMode2) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB56, kRegShift425FB56) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB78, kRegShift425FB78) &&
				CopyVideoFormat(inChannel, NTV2_CHANNEL5, NTV2_CHANNEL8);
		}
	}
	else
	{
        if (!IsMultiFormatActive())
        {
            status = WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode, kRegShiftQuadMode) &&
                WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode2, kRegShiftQuadMode2);
        }
		else if (inChannel < NTV2_CHANNEL5)
		{
			status = WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode, kRegShiftQuadMode);
		}
		else
		{
			status = WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode2, kRegShiftQuadMode2);
		}
	}

	return status;
}

bool CNTV2Card::Get4kSquaresEnable (bool & outIsEnabled, const NTV2Channel inChannel)
{
	outIsEnabled = false;
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;
	ULWord	squaresEnabled	(0);
	bool status = true;

	if (inChannel < NTV2_CHANNEL5)
		status = ReadRegister (kRegGlobalControl2, squaresEnabled, kRegMaskQuadMode, kRegShiftQuadMode);
	else
		status = ReadRegister(kRegGlobalControl2, squaresEnabled, kRegMaskQuadMode2, kRegShiftQuadMode2);

	outIsEnabled = (squaresEnabled ? true : false);
	return status;
}

// Method: SetTsiFrameEnable
// Input:  bool
// Output: NONE
bool CNTV2Card::SetTsiFrameEnable (const bool enable, const NTV2Channel inChannel)
{
	bool status = true;

	if(!::NTV2DeviceCanDo425Mux(_boardID) && !::NTV2DeviceCanDo12gRouting(_boardID))
		return false;
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;

	if(enable)
	{
		if (::NTV2DeviceCanDo12gRouting(_boardID))
		{
			status = WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode, kRegShiftQuadMode) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode2, kRegShiftQuadMode2) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB12, kRegShift425FB12) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB34, kRegShift425FB34) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB56, kRegShift425FB56) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB78, kRegShift425FB78);
			if (!status)
				return false;

			if (!IsMultiFormatActive())
			{
					status = WriteRegister(kRegGlobalControl, 1, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
					WriteRegister(kRegGlobalControlCh2, 1, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
					WriteRegister(kRegGlobalControlCh3, 1, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
					WriteRegister(kRegGlobalControlCh4, 1, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
					CopyVideoFormat(inChannel, NTV2_CHANNEL1, NTV2_CHANNEL8);
			}
			else
			{
				status = WriteRegister(gChannelToGlobalControlRegNum[inChannel], 1, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable);
			}
		}
		// enable 425 mode, disable squares
		else if (!IsMultiFormatActive())
		{
			status = WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode, kRegShiftQuadMode) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode2, kRegShiftQuadMode2) &&
				WriteRegister(kRegGlobalControl2, 1, kRegMask425FB12, kRegShift425FB12) &&
				WriteRegister(kRegGlobalControl2, 1, kRegMask425FB34, kRegShift425FB34) &&
				WriteRegister(kRegGlobalControl2, 1, kRegMask425FB56, kRegShift425FB56) &&
				WriteRegister(kRegGlobalControl2, 1, kRegMask425FB78, kRegShift425FB78) &&
				CopyVideoFormat(inChannel, NTV2_CHANNEL1, NTV2_CHANNEL8);
		}
		else if (inChannel < NTV2_CHANNEL3)
		{
			status = WriteRegister(kRegGlobalControl2, 1, kRegMask425FB12, kRegShift425FB12) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode, kRegShiftQuadMode) &&
				CopyVideoFormat(inChannel, NTV2_CHANNEL1, NTV2_CHANNEL2);
		}
		else if (inChannel < NTV2_CHANNEL5)
		{
			status = WriteRegister(kRegGlobalControl2, 1, kRegMask425FB34, kRegShift425FB34) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode, kRegShiftQuadMode) &&
				CopyVideoFormat(inChannel, NTV2_CHANNEL3, NTV2_CHANNEL4);
		}
		else if (inChannel < NTV2_CHANNEL7)
		{
			status = WriteRegister(kRegGlobalControl2, 1, kRegMask425FB56, kRegShift425FB56) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMaskQuadMode2, kRegShiftQuadMode2) &&
				CopyVideoFormat(inChannel, NTV2_CHANNEL5, NTV2_CHANNEL6);
		}
		else
		{
			status = WriteRegister(kRegGlobalControl2, 1, kRegMask425FB78, kRegShift425FB78) &&
				WriteRegister (kRegGlobalControl2, 0, kRegMaskQuadMode2, kRegShiftQuadMode2) &&
				CopyVideoFormat(inChannel, NTV2_CHANNEL7, NTV2_CHANNEL8);
		}
	}
	else
	{
		if (::NTV2DeviceCanDo12gRouting(_boardID))
		{
			if (!IsMultiFormatActive())
			{
				status = WriteRegister(kRegGlobalControl, 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
					WriteRegister(kRegGlobalControlCh2, 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
					WriteRegister(kRegGlobalControlCh3, 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable) &&
					WriteRegister(kRegGlobalControlCh4, 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable);
			}
			else
			{
				status = WriteRegister(gChannelToGlobalControlRegNum[inChannel], 0, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable);
			}
		}
        // disable 425 mode, enable squares
		else if (!IsMultiFormatActive())
		{
			status = WriteRegister(kRegGlobalControl2, 0, kRegMask425FB12, kRegShift425FB12) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB34, kRegShift425FB34) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB56, kRegShift425FB56) &&
				WriteRegister(kRegGlobalControl2, 0, kRegMask425FB78, kRegShift425FB78);
		}
		else if (inChannel < NTV2_CHANNEL3)
		{
            status = WriteRegister(kRegGlobalControl2, 0, kRegMask425FB12, kRegShift425FB12);
		}
		else if (inChannel < NTV2_CHANNEL5)
		{
            status = WriteRegister(kRegGlobalControl2, 0, kRegMask425FB34, kRegShift425FB34);
		}
		else if (inChannel < NTV2_CHANNEL7)
		{
            status = WriteRegister(kRegGlobalControl2, 0, kRegMask425FB56, kRegShift425FB56);
		}
		else
		{
            status = WriteRegister(kRegGlobalControl2, 0, kRegMask425FB78, kRegShift425FB78);
		}
	}

	return status;
}

// Method: GetTsiFrameEnable
// Input:  NONE
// Output: bool
bool CNTV2Card::GetTsiFrameEnable (bool & outIsEnabled, const NTV2Channel inChannel)
{
	outIsEnabled = false;
	if (!::NTV2DeviceCanDo425Mux (_boardID) && !::NTV2DeviceCanDo12gRouting(_boardID))
		return false;
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;
	// Return true (1) Quad Frame Geometry is enabled
	// Return false (0) if this mode is disabled
	bool returnVal(false), readOkay(false);

	if (::NTV2DeviceCanDo12gRouting(_boardID))
	{
		readOkay = GetQuadQuadFrameEnable(returnVal, inChannel);
		if (!returnVal)
			readOkay = CNTV2DriverInterface::ReadRegister(gChannelToGlobalControlRegNum[inChannel], returnVal, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable);
	}
	else
	{
		if (inChannel < NTV2_CHANNEL3)
			readOkay = CNTV2DriverInterface::ReadRegister (kRegGlobalControl2, returnVal, kRegMask425FB12, kRegShift425FB12);
		else if (inChannel < NTV2_CHANNEL5)
			readOkay = CNTV2DriverInterface::ReadRegister (kRegGlobalControl2, returnVal, kRegMask425FB34, kRegShift425FB34);
		else if (inChannel < NTV2_CHANNEL7)
			readOkay = CNTV2DriverInterface::ReadRegister (kRegGlobalControl2, returnVal, kRegMask425FB56, kRegShift425FB56);
		else
			readOkay = CNTV2DriverInterface::ReadRegister(kRegGlobalControl2, returnVal, kRegMask425FB78, kRegShift425FB78);
	}

	outIsEnabled = readOkay ? returnVal : 0;
	return readOkay;
}

bool CNTV2Card::GetTsiMuxSyncFail (bool & outSyncFailed, const NTV2Channel inWhichTsiMux)
{
	ULWord value(0);
	outSyncFailed = false;
	if (!::NTV2DeviceCanDo425Mux(_boardID))
		return false;
	if (!NTV2_IS_VALID_CHANNEL(inWhichTsiMux))
		return false;
	if (!ReadRegister(kRegSDIInput3GStatus, value, kRegMaskSDIInTsiMuxSyncFail, kRegShiftSDIInTsiMuxSyncFail))
		return false;
	if (value & (1<<inWhichTsiMux))
		outSyncFailed = true;
	return true;
}

bool CNTV2Card::CopyVideoFormat(const NTV2Channel inSrc, const NTV2Channel inFirst, const NTV2Channel inLast)
{
	ULWord standard = 0;
	ULWord rate1 = 0;
	ULWord rate2 = 0;
	ULWord s372 = 0;
	ULWord geometry = 0;
	ULWord format = 0;
	bool status = false;

	status = ReadRegister (gChannelToGlobalControlRegNum[inSrc], standard,  kRegMaskStandard,  kRegShiftStandard);
	status &= ReadRegister (gChannelToGlobalControlRegNum[inSrc], rate1, kRegMaskFrameRate, kRegShiftFrameRate);
	status &= ReadRegister (gChannelToGlobalControlRegNum[inSrc], rate2, kRegMaskFrameRateHiBit, kRegShiftFrameRateHiBit);
	status &= ReadRegister (gChannelToSmpte372RegisterNum[inSrc], s372, gChannelToSmpte372Masks[inSrc], gChannelToSmpte372Shifts[inSrc]);
	status &= ReadRegister (gChannelToGlobalControlRegNum[inSrc], geometry, kRegMaskGeometry, kRegShiftGeometry);
	status &= ReadRegister (kVRegVideoFormatCh1 + inSrc, format);
	if (!status) return false;

	for (int channel = inFirst; channel <= inLast; channel++)
	{
		status = WriteRegister (gChannelToGlobalControlRegNum[channel], standard, kRegMaskStandard, kRegShiftStandard);
		status &= WriteRegister (gChannelToGlobalControlRegNum[channel], rate1, kRegMaskFrameRate, kRegShiftFrameRate);
		status &= WriteRegister (gChannelToGlobalControlRegNum[channel], rate2, kRegMaskFrameRateHiBit, kRegShiftFrameRateHiBit);
		status &= WriteRegister (gChannelToSmpte372RegisterNum[channel], s372, gChannelToSmpte372Masks[channel], gChannelToSmpte372Shifts[channel]);
		status &= WriteRegister (gChannelToGlobalControlRegNum[channel], geometry, kRegMaskGeometry, kRegShiftGeometry);
		status &= WriteRegister (ULWord(kVRegVideoFormatCh1 + channel), format);
		if (!status) return false;
	}

	return true;
}

// Method: SetReference
// Input:  NTV2Reference
// Output: NONE
bool CNTV2Card::SetReference (const NTV2ReferenceSource inRefSource, const bool inKeepFramePulseSelect)
{
	NTV2DeviceID id = GetDeviceID();

	if (::NTV2DeviceCanDoLTCInOnRefPort(id) && inRefSource == NTV2_REFERENCE_EXTERNAL)
		SetLTCOnReference(false);
	
	if (NTV2DeviceCanDoFramePulseSelect(id) && !inKeepFramePulseSelect)
	{
		//Reset this for backwards compatibility
		EnableFramePulseReference(false);
	}

	//this looks slightly unusual but really
	//it is a 4 bit counter in 2 different registers
	ULWord refControl1 = ULWord(inRefSource), refControl2 = 0, ptpControl = 0;
	switch(inRefSource)
	{
	case NTV2_REFERENCE_INPUT5:
		refControl1 = 0;
		refControl2 = 1;
		break;
	case NTV2_REFERENCE_INPUT6:
		refControl1 = 1;
		refControl2 = 1;
		break;
	case NTV2_REFERENCE_INPUT7:
		refControl1 = 2;
		refControl2 = 1;
		break;
	case NTV2_REFERENCE_INPUT8:
		refControl1 = 3;
		refControl2 = 1;
		break;
	case NTV2_REFERENCE_SFP1_PCR:
		refControl1 = 4;
		refControl2 = 1;
		break;
	case NTV2_REFERENCE_SFP1_PTP:
		refControl1 = 4;
		refControl2 = 1;
		ptpControl = 1;
		break;
	case NTV2_REFERENCE_SFP2_PCR:
		refControl1 = 5;
		refControl2 = 1;
		break;
	case NTV2_REFERENCE_SFP2_PTP:
		refControl1 = 5;
		refControl2 = 1;
		ptpControl = 1;
		break;
    case NTV2_REFERENCE_HDMI_INPUT2:
        refControl1 = 4;
        refControl2 = 0;
        break;
    case NTV2_REFERENCE_HDMI_INPUT3:
        refControl1 = 6;
        refControl2 = 0;
        break;
    case NTV2_REFERENCE_HDMI_INPUT4:
        refControl1 = 7;
        refControl2 = 0;
        break;
    default:
		break;
	}

	if(IsIPDevice())
	{
		WriteRegister(kRegGlobalControl2, ptpControl, kRegMaskPCRReferenceEnable, kRegShiftPCRReferenceEnable);
	}

	if (::NTV2DeviceGetNumVideoChannels(_boardID) > 4 || IsIPDevice())
	{
		WriteRegister (kRegGlobalControl2, refControl2,	kRegMaskRefSource2, kRegShiftRefSource2);
	}
		
	return WriteRegister (kRegGlobalControl, refControl1, kRegMaskRefSource, kRegShiftRefSource);
}

// Method: GetReference
// Input:  NTV2Reference
// Output: NONE
bool CNTV2Card::GetReference (NTV2ReferenceSource & outValue)
{
	ULWord	refControl1(0), refControl2(0), ptpControl(0);
	bool	result	(ReadRegister (kRegGlobalControl, refControl1, kRegMaskRefSource, kRegShiftRefSource));

	outValue = NTV2ReferenceSource(refControl1);

	if (::NTV2DeviceGetNumVideoChannels(_boardID) > 4 || IsIPDevice())
	{
		ReadRegister (kRegGlobalControl2,  refControl2,  kRegMaskRefSource2,  kRegShiftRefSource2);
		if (refControl2)
			switch (outValue)
			{
			case 0:
				outValue = NTV2_REFERENCE_INPUT5;
			break;
			case 1:
				outValue = NTV2_REFERENCE_INPUT6;
				break;
			case 2:
				outValue = NTV2_REFERENCE_INPUT7;
				break;
			case 3:
				outValue = NTV2_REFERENCE_INPUT8;
				break;
			case 4:
				if(IsIPDevice())
				{
					ReadRegister(kRegGlobalControl2, ptpControl, kRegMaskPCRReferenceEnable, kRegShiftPCRReferenceEnable);
				}
				outValue = ptpControl == 0 ? NTV2_REFERENCE_SFP1_PCR : NTV2_REFERENCE_SFP1_PTP;
				break;
			case 5:
				if(IsIPDevice())
				{
					ReadRegister(kRegGlobalControl2, ptpControl, kRegMaskPCRReferenceEnable, kRegShiftPCRReferenceEnable);
				}
				outValue = ptpControl == 0 ? NTV2_REFERENCE_SFP2_PCR : NTV2_REFERENCE_SFP2_PTP;
				break;
			default:
				break;
			}
	}

    if (_boardID == DEVICE_ID_KONAHDMI)
    {
        switch(refControl1)
        {
        case 5:
            outValue = NTV2_REFERENCE_HDMI_INPUT1;
            break;
        case 4:
            outValue = NTV2_REFERENCE_HDMI_INPUT2;
            break;
        case 6:
            outValue = NTV2_REFERENCE_HDMI_INPUT3;
            break;
        case 7:
            outValue = NTV2_REFERENCE_HDMI_INPUT4;
            break;
        default:
            break;
        }
    }

    return result;
}

bool CNTV2Card::EnableFramePulseReference (const bool enable)
{
	NTV2DeviceID id = GetDeviceID();
	if(!::NTV2DeviceCanDoFramePulseSelect(id))
		return false;
	
	return WriteRegister (kRegGlobalControl3, enable ? 1 : 0, kRegMaskFramePulseEnable, kRegShiftFramePulseEnable);
}


bool CNTV2Card::GetEnableFramePulseReference (bool & outValue)
{
	NTV2DeviceID id = GetDeviceID();
	if(!::NTV2DeviceCanDoFramePulseSelect(id))
		return false;
	
	ULWord returnValue(0);
	bool status = ReadRegister(kRegGlobalControl3, returnValue, kRegMaskFramePulseEnable, kRegShiftFramePulseEnable);
	outValue = returnValue == 0 ? false : true;
	return status;
}

bool CNTV2Card::SetFramePulseReference (const NTV2ReferenceSource value)
{
	if(!::NTV2DeviceCanDoFramePulseSelect(GetDeviceID()))
		return false;	
	return WriteRegister (kRegGlobalControl3, ULWord(value), kRegMaskFramePulseRefSelect, kRegShiftFramePulseRefSelect);
}

bool CNTV2Card::GetFramePulseReference (NTV2ReferenceSource & outValue)
{
	ULWord	refControl1(0);
	if(!::NTV2DeviceCanDoFramePulseSelect(GetDeviceID()))
		return false;
	
	bool	result	(ReadRegister (kRegGlobalControl3, refControl1, kRegMaskFramePulseRefSelect, kRegShiftFramePulseRefSelect));
	outValue = NTV2ReferenceSource(refControl1);
	return result;
}

#if !defined (NTV2_DEPRECATE)
	// Deprecated - Use SetReference instead
	bool CNTV2Card::SetReferenceSource (NTV2ReferenceSource value, bool ajaRetail)
	{
		// On Mac we do this using a virtual register
		if (ajaRetail == true)
		{
			// Mac uses virtual registers - driver sorts it card specific details
			ULWord refSelect;
			switch (value)
			{
				case NTV2_REFERENCE_EXTERNAL:	refSelect = kReferenceIn;	break;
				case NTV2_REFERENCE_FREERUN:	refSelect = kFreeRun;		break;
				default:						refSelect = kVideoIn;		break;
			}
			
			return WriteRegister(kVRegDisplayReferenceSelect, refSelect);
		}
		else
			return WriteRegister (kRegGlobalControl, value, kRegMaskRefSource, kRegShiftRefSource);
	}

	// Deprecated - Use GetReference instead
	bool CNTV2Card::GetReferenceSource (NTV2ReferenceSource* value, bool ajaRetail)
	{
		(void) ajaRetail;
		ULWord returnVal;
		bool result = ReadRegister (kRegGlobalControl, &returnVal, kRegMaskRefSource, kRegShiftRefSource);
		*value = static_cast <NTV2ReferenceSource> (returnVal);
		return result;
	}

	// DEPRECATED! - Reg bit now part of kRegMaskRefSource - do not use on new boards
	bool CNTV2Card::SetReferenceVoltage (NTV2RefVoltage value)
	{
		return WriteRegister (kRegGlobalControl, value, kRegMaskRefInputVoltage, kRegShiftRefInputVoltage);
	}

	// DEPRECATED! - Reg bit now now part of kRegMaskRefSource - do not use on new boards
	bool CNTV2Card::GetReferenceVoltage (NTV2RefVoltage* value)
	{
		return ReadRegister (kRegGlobalControl, (ULWord*)value,	kRegMaskRefInputVoltage, kRegShiftRefInputVoltage);
	}

	// OBSOLETE
	bool CNTV2Card::SetFrameBufferMode(NTV2Channel channel, NTV2FrameBufferMode value)
	{
		return WriteRegister (gChannelToControlRegNum [channel], value, kRegMaskFrameBufferMode, kRegShiftFrameBufferMode);
	}

	// OBSOLETE
	bool CNTV2Card::GetFrameBufferMode (NTV2Channel inChannel, NTV2FrameBufferMode & outValue)
	{
		return ReadRegister (gChannelToControlRegNum [inChannel], reinterpret_cast <ULWord*> (&outValue), kRegMaskFrameBufferMode, kRegShiftFrameBufferMode);
	}
#endif	//	!defined (NTV2_DEPRECATE)


// Method: SetMode
// Input:  NTV2Channel,  NTV2Mode
// Output: NONE
bool CNTV2Card::SetMode (const NTV2Channel inChannel, const NTV2Mode inValue,  bool inIsRetail)
{
	#if !defined (NTV2_DEPRECATE)
		#ifdef  MSWindows
			NTV2EveryFrameTaskMode mode;
			GetEveryFrameServices(&mode);
			if(mode == NTV2_STANDARD_TASKS)
				inIsRetail = true;
		#endif
	#else
		(void) inIsRetail;
	#endif	//	!defined (NTV2_DEPRECATE)

	return WriteRegister (gChannelToControlRegNum [inChannel], inValue, kRegMaskMode, kRegShiftMode);
}

bool CNTV2Card::GetMode (const NTV2Channel inChannel, NTV2Mode & outValue)
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChannelToControlRegNum[inChannel], outValue, kRegMaskMode, kRegShiftMode);
}

bool CNTV2Card::GetFrameInfo (const NTV2Channel inChannel, NTV2FrameGeometry & outGeometry, NTV2FrameBufferFormat & outFBF)
{
	bool status = GetFrameGeometry(outGeometry);	//	WHY ISN'T inChannel PASSED TO GetFrameGeometry?!?!?!
	if ( !status )
		return status; // TODO: fix me

	status = GetFrameBufferFormat(inChannel, outFBF);
	if ( !status )
		return status; // TODO: fix me

	return status;
}

bool CNTV2Card::IsBufferSizeChangeRequired(NTV2Channel channel, NTV2FrameGeometry currentGeometry, NTV2FrameGeometry newGeometry,
										   NTV2FrameBufferFormat format)
{
	(void) channel;
	ULWord currentFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID,currentGeometry,format);
	ULWord requestedFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID,newGeometry,format);
	bool changeBufferSize =	::NTV2DeviceCanChangeFrameBufferSize(_boardID) && (currentFrameBufferSize != requestedFrameBufferSize);

	// If software has decreed a frame buffer size, don't try to change it
	if (IsBufferSizeSetBySW())
		changeBufferSize = false;

	return changeBufferSize;
}

bool CNTV2Card::IsBufferSizeChangeRequired(NTV2Channel channel, NTV2FrameGeometry geometry,
										   NTV2FrameBufferFormat currentFormat, NTV2FrameBufferFormat newFormat)
{
    (void)channel;
    
	ULWord currentFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID,geometry,currentFormat);
	ULWord requestedFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID,geometry,newFormat);
	bool changeBufferSize =	::NTV2DeviceCanChangeFrameBufferSize(_boardID) && (currentFrameBufferSize != requestedFrameBufferSize);

	// If software has decreed a frame buffer size, don't try to change it
	if (IsBufferSizeSetBySW())
		changeBufferSize = false;

	return changeBufferSize;
}

bool CNTV2Card::IsBufferSizeSetBySW()
{
	ULWord swControl;

	if (!::NTV2DeviceSoftwareCanChangeFrameBufferSize(_boardID))
		return false;

	if ( !ReadRegister(kRegCh1Control, swControl, kRegMaskFrameSizeSetBySW, kRegShiftFrameSizeSetBySW) )
		return false;

	return swControl != 0;
}

bool CNTV2Card::GetFBSizeAndCountFromHW(ULWord* size, ULWord* count)
{
	ULWord ch1Control;
	ULWord multiplier;
	ULWord sizeMultiplier;

	if (!IsBufferSizeSetBySW())
		return false;

	if (!ReadRegister(kRegCh1Control, ch1Control))
		return false;

	ch1Control &= BIT_20 | BIT_21;
	switch(ch1Control)
	{
	default:
	case 0:
		multiplier = 4;	// 2MB frame buffers
		sizeMultiplier = 2;
		break;
	case BIT_20:
		multiplier = 2;	// 4MB
		sizeMultiplier = 4;
		break;
	case BIT_21:
		multiplier = 1;	// 8MB
		sizeMultiplier = 8;
		break;
	case BIT_20 | BIT_21:
		multiplier = 0;		// 16MB
		sizeMultiplier = 16;
		break;
	}

	if (size)
		*size = sizeMultiplier * 1024 * 1024;

	if (count)
	{
		if (multiplier == 0)
			*count = ::NTV2DeviceGetNumberFrameBuffers(_boardID) / 2;
		else
			*count = multiplier * ::NTV2DeviceGetNumberFrameBuffers(_boardID);
	}

	NTV2FrameGeometry geometry = NTV2_FG_NUMFRAMEGEOMETRIES;
	GetFrameGeometry(geometry);
	if(geometry == NTV2_FG_4x1920x1080 || geometry == NTV2_FG_4x2048x1080)
	{
		*size *= 4;
		*count /= 4;
	}

	return true;
}

bool CNTV2Card::SetFrameBufferSize (NTV2Framesize size)
{
	ULWord reg1Contents;

	if (!::NTV2DeviceSoftwareCanChangeFrameBufferSize(_boardID))
		return false;

	if (!ReadRegister(kRegCh1Control, reg1Contents))
		return false;

	reg1Contents |= kRegMaskFrameSizeSetBySW;
	reg1Contents &= ~kK2RegMaskFrameSize;
	reg1Contents |= ULWord(size) << kK2RegShiftFrameSize;

	if (!WriteRegister(kRegCh1Control, reg1Contents))
		return false;

	if (!GetFBSizeAndCountFromHW(&_ulFrameBufferSize, &_ulNumFrameBuffers))
		return false;

	return true;
}

bool CNTV2Card::GetLargestFrameBufferFormatInUse(NTV2FrameBufferFormat & outFBF)
{
	NTV2FrameBufferFormat ch1format;
	NTV2FrameBufferFormat ch2format = NTV2_FBF_8BIT_YCBCR;

	if ( !GetFrameBufferFormat(NTV2_CHANNEL1, ch1format) )
		return false;

	if ( !GetFrameBufferFormat(NTV2_CHANNEL2, ch2format) &&
		::NTV2DeviceGetNumVideoChannels(_boardID) > 1)
		return false;

	NTV2FrameGeometry geometry;
	if (!GetFrameGeometry(geometry))
		return false;

	ULWord ch1FrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID,geometry,ch1format);
	ULWord ch2FrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID,geometry,ch2format);
	if ( ch1FrameBufferSize >= ch2FrameBufferSize )
		outFBF = ch1format;
	else
		outFBF = ch2format;
	return true;
}

bool CNTV2Card::IsMultiFormatActive (void)
{
	if (!::NTV2DeviceCanDoMultiFormat (_boardID))
		return false;

	bool isEnabled = false;
	if (!GetMultiFormatMode (isEnabled))
		return false;

	return isEnabled;
}

bool CNTV2Card::HasCanConnectROM (void)
{
	ULWord hasCanConnectROM(0);
	return ReadRegister(kRegCanDoStatus, hasCanConnectROM, kRegMaskCanDoValidXptROM, kRegShiftCanDoValidXptROM)  &&  (hasCanConnectROM ? true : false);
}

bool CNTV2Card::GetPossibleConnections (NTV2PossibleConnections & outConnections)
{
	outConnections.clear();
	if (!HasCanConnectROM())
		return false;

	NTV2RegReads ROMregs;
	return CNTV2SignalRouter::MakeRouteROMRegisters(ROMregs)
			&&  ReadRegisters(ROMregs)
			&&  CNTV2SignalRouter::GetPossibleConnections(ROMregs, outConnections);
}

/////////////////////////////////

// Method: SetFrameBufferFormat
// Input:  NTV2Channel, NTV2FrameBufferFormat
// Output: NONE
bool CNTV2Card::SetFrameBufferFormat(NTV2Channel channel, NTV2FrameBufferFormat newFormat, bool inIsRetailMode,
									 NTV2HDRXferChars inXferChars, NTV2HDRColorimetry inColorimetry, NTV2HDRLuminance inLuminance)
{
	#if !defined (NTV2_DEPRECATE)
		#ifdef  MSWindows
			NTV2EveryFrameTaskMode mode;
			GetEveryFrameServices(&mode);
			if(mode == NTV2_STANDARD_TASKS)
				inIsRetailMode = true;
		#endif
	#else
		(void) inIsRetailMode;
	#endif	//	!defined (NTV2_DEPRECATE)

	if (IS_CHANNEL_INVALID (channel))
		return false;

	const ULWord	regNum	(gChannelToControlRegNum[channel]);
	const ULWord	loValue	(newFormat & 0x0f);
	const ULWord	hiValue	((newFormat & 0x10) >> 4);
	NTV2FrameGeometry		currentGeometry	(NTV2_FG_INVALID);
	NTV2FrameBufferFormat	currentFormat	(NTV2_FBF_INVALID); // save for call to IsBufferSizeChangeRequired below
	bool status = GetFrameInfo(channel, currentGeometry, currentFormat);
	if (!status)
		return status;

	//	Set channel control register FBF bits 1,2,3,4,6...
	status =  WriteRegister (regNum, loValue, kRegMaskFrameFormat,      kRegShiftFrameFormat)
	      &&  WriteRegister (regNum, hiValue, kRegMaskFrameFormatHiBit, kRegShiftFrameFormatHiBit);

	#if !defined (NTV2_DEPRECATE)
		//	Mac driver does it's own bit file switching so only do this for MSWindows
		if (!inIsRetailMode)
			CheckBitfile ();	//	Might be to/from QRez
	#endif	//	!defined (NTV2_DEPRECATE)

	// If software set the frame buffer size, read the values from hardware
	if ( !GetFBSizeAndCountFromHW(&_ulFrameBufferSize, &_ulNumFrameBuffers) &&
		  IsBufferSizeChangeRequired(channel,currentGeometry,currentFormat,newFormat) )
	{
		_ulFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID,currentGeometry,newFormat);
		_ulNumFrameBuffers = ::NTV2DeviceGetNumberFrameBuffers(_boardID,currentGeometry,newFormat);
	}

	if (status)
		{if (newFormat != currentFormat)
			CVIDINFO("'" << GetDisplayName() << "': Channel " << DEC(UWord(channel)+1) << " FBF changed from "
					<< ::NTV2FrameBufferFormatToString(currentFormat) << " to "
					<< ::NTV2FrameBufferFormatToString(newFormat) << " (FBSize=" << xHEX0N(_ulFrameBufferSize,8)
					<< " numFBs=" << DEC(_ulNumFrameBuffers) << ")");}
	else
		CVIDFAIL("'" << GetDisplayName() << "': Failed to change channel " << DEC(UWord(channel)+1) << " FBF from "
				<< ::NTV2FrameBufferFormatToString(currentFormat) << " to " << ::NTV2FrameBufferFormatToString(newFormat));

	SetVPIDTransferCharacteristics(inXferChars, channel);
	SetVPIDColorimetry(inColorimetry, channel);
	SetVPIDLuminance(inLuminance, channel);
	return status;
}

bool CNTV2Card::SetFrameBufferFormat (const NTV2ChannelSet & inFrameStores,
									  const NTV2FrameBufferFormat inNewFormat,
									  const bool inIsAJARetail,
									  const NTV2HDRXferChars inXferChars,
									  const NTV2HDRColorimetry inColorimetry,
									  const NTV2HDRLuminance inLuminance)
{
	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inFrameStores.begin());  it != inFrameStores.end();  ++it)
		if (!SetFrameBufferFormat (*it, inNewFormat, inIsAJARetail, inXferChars, inColorimetry, inLuminance))
			failures++;
	return failures == 0;
}

// Method: GetFrameBufferFormat
// Input:  NTV2Channel
// Output: NTV2FrameBufferFormat
bool CNTV2Card::GetFrameBufferFormat(NTV2Channel inChannel, NTV2FrameBufferFormat & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;

	ULWord	returnVal1, returnVal2;
	bool	result1 = ReadRegister (gChannelToControlRegNum[inChannel], returnVal1, kRegMaskFrameFormat,      kRegShiftFrameFormat);
	bool	result2 = ReadRegister (gChannelToControlRegNum[inChannel], returnVal2, kRegMaskFrameFormatHiBit, kRegShiftFrameFormatHiBit);

	outValue = static_cast <NTV2FrameBufferFormat> ((returnVal1 & 0x0f) | ((returnVal2 & 0x1) << 4));
	return (result1 && result2);
}

// Method: SetFrameBufferQuarterSizeMode
// Input:  NTV2Channel, NTV2K2QuarterSizeExpandMode
// Output: NONE
bool CNTV2Card::SetFrameBufferQuarterSizeMode(NTV2Channel channel, NTV2QuarterSizeExpandMode value)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;
	return WriteRegister (gChannelToControlRegNum [channel], value, kRegMaskQuarterSizeMode, kRegShiftQuarterSizeMode);
}

// Method: GetFrameBufferQuarterSizeMode
// Input:  NTV2Channel
// Output: NTV2K2QuarterSizeExpandMode
bool CNTV2Card::GetFrameBufferQuarterSizeMode (NTV2Channel inChannel, NTV2QuarterSizeExpandMode & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChannelToControlRegNum[inChannel], outValue, kRegMaskQuarterSizeMode, kRegShiftQuarterSizeMode);
}

// Method: SetFrameBufferQuality - currently used for ProRes compressed buffers
// Input:  NTV2Channel, NTV2K2FrameBufferQuality
// Output: NONE
bool CNTV2Card::SetFrameBufferQuality(NTV2Channel channel, NTV2FrameBufferQuality quality)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;
	// note buffer quality is split between bit 17 (lo), 25-26 (hi)
	ULWord loValue = quality & 0x1;
	ULWord hiValue = (quality >> 1) & 0x3;
	return WriteRegister (gChannelToControlRegNum [channel], loValue, kRegMaskQuality, kRegShiftQuality) &&
			WriteRegister (gChannelToControlRegNum [channel], hiValue, kRegMaskQuality2, kRegShiftQuality2);
}

// Method: GetFrameBufferQuality - currently used for ProRes compressed buffers
// Input:  NTV2Channel
// Output: NTV2K2FrameBufferQuality
bool CNTV2Card::GetFrameBufferQuality (NTV2Channel inChannel, NTV2FrameBufferQuality & outQuality)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	outQuality = NTV2_FBQualityInvalid;
	// note buffer quality is split between bit 17 (lo), 25-26 (hi)
	ULWord loValue(0), hiValue(0);
	if (ReadRegister (gChannelToControlRegNum [inChannel], loValue, kRegMaskQuality, kRegShiftQuality) &&
		ReadRegister (gChannelToControlRegNum [inChannel], hiValue, kRegMaskQuality2, kRegShiftQuality2))
	{
		outQuality = NTV2FrameBufferQuality(loValue + ((hiValue & 0x3) << 1));
		return true;
	}
	return false;
}


// Method: SetEncodeAsPSF - currently used for ProRes compressed buffers
// Input:  NTV2Channel, NTV2K2FrameBufferQuality
// Output: NONE
bool CNTV2Card::SetEncodeAsPSF(NTV2Channel channel, NTV2EncodeAsPSF value)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;
	return WriteRegister (gChannelToControlRegNum [channel], value, kRegMaskEncodeAsPSF, kRegShiftEncodeAsPSF);
}

// Method: GetEncodeAsPSF - currently used for ProRes compressed buffers
// Input:  NTV2Channel
// Output: NTV2K2FrameBufferQuality
bool CNTV2Card::GetEncodeAsPSF (NTV2Channel inChannel, NTV2EncodeAsPSF & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChannelToControlRegNum[inChannel], outValue, kRegMaskEncodeAsPSF, kRegShiftEncodeAsPSF);
}

// Method: SetFrameBufferOrientation
// Input:  NTV2Channel,  NTV2VideoFrameBufferOrientation
// Output: NONE
bool CNTV2Card::SetFrameBufferOrientation (const NTV2Channel inChannel, const NTV2FBOrientation value)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (gChannelToControlRegNum [inChannel], value, kRegMaskFrameOrientation, kRegShiftFrameOrientation);
}

// Method: GetFrameBufferOrientation
// Input:  NTV2Channel
// Output: NTV2VideoFrameBufferOrientation
bool CNTV2Card::GetFrameBufferOrientation (const NTV2Channel inChannel, NTV2FBOrientation & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChannelToControlRegNum[inChannel], outValue, kRegMaskFrameOrientation, kRegShiftFrameOrientation);
}


// Method: SetFrameBufferSize
// Input:  NTV2Channel,  NTV2K2Framesize
// Output: NONE
bool CNTV2Card::SetFrameBufferSize (const NTV2Channel inChannel, const NTV2Framesize inValue)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
#if defined (NTV2_ALLOW_2MB_FRAMES)
	ULWord	supports2m (0);
	ReadRegister(kRegGlobalControl2, &supports2m, kRegMask2MFrameSupport, kRegShift2MFrameSupport);
	if(supports2m == 1)
	{
		ULWord value2M (0);
		switch(inValue)
		{
		case NTV2_FRAMESIZE_2MB:	value2M = 1;	break;
		case NTV2_FRAMESIZE_4MB:	value2M = 2;	break;
		case NTV2_FRAMESIZE_6MB:	value2M = 3;	break;
		case NTV2_FRAMESIZE_8MB:	value2M = 4;	break;
		case NTV2_FRAMESIZE_10MB:	value2M = 5;	break;
		case NTV2_FRAMESIZE_12MB:	value2M = 6;	break;
		case NTV2_FRAMESIZE_14MB:	value2M = 7;	break;
		case NTV2_FRAMESIZE_16MB:	value2M = 8;	break;
		case NTV2_FRAMESIZE_18MB:	value2M = 9;	break;
		case NTV2_FRAMESIZE_20MB:	value2M = 10;	break;
		case NTV2_FRAMESIZE_22MB:	value2M = 11;	break;
		case NTV2_FRAMESIZE_24MB:	value2M = 12;	break;
		case NTV2_FRAMESIZE_26MB:	value2M = 13;	break;
		case NTV2_FRAMESIZE_28MB:	value2M = 14;	break;
		case NTV2_FRAMESIZE_30MB:	value2M = 15;	break;
		case NTV2_FRAMESIZE_32MB:	value2M = 16;	break;
		default:	return false;
		}
		return WriteRegister(gChannelTo2MFrame [NTV2_CHANNEL1], value2M, kRegMask2MFrameSize, kRegShift2MFrameSupport);
	}
	else
#endif	//	defined (NTV2_ALLOW_2MB_FRAMES)
	if (inValue == NTV2_FRAMESIZE_2MB || inValue == NTV2_FRAMESIZE_4MB || inValue == NTV2_FRAMESIZE_8MB || inValue == NTV2_FRAMESIZE_16MB)
		return WriteRegister (gChannelToControlRegNum [NTV2_CHANNEL1], inValue, kK2RegMaskFrameSize, kK2RegShiftFrameSize);
	return false;
}

// Method: GetK2FrameBufferSize
// Input:  NTV2Channel
// Output: NTV2K2Framesize
bool CNTV2Card::GetFrameBufferSize (const NTV2Channel inChannel, NTV2Framesize & outValue)
{
	outValue = NTV2_FRAMESIZE_INVALID;
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;
#if defined (NTV2_ALLOW_2MB_FRAMES)
	ULWord	supports2m (0);
	ReadRegister(kRegGlobalControl2, &supports2m, kRegMask2MFrameSupport, kRegShift2MFrameSupport);
	if(supports2m == 1)
	{
		ULWord frameSize (0);
		ReadRegister(gChannelTo2MFrame[inChannel], &frameSize, kRegMask2MFrameSize, kRegShift2MFrameSize);
		if (frameSize)
		{
			switch (frameSize)
			{
			case 1:		outValue = NTV2_FRAMESIZE_2MB;	break;
			case 2:		outValue = NTV2_FRAMESIZE_4MB;	break;
			case 3:		outValue = NTV2_FRAMESIZE_6MB;	break;
			case 4:		outValue = NTV2_FRAMESIZE_8MB;	break;
			case 5:		outValue = NTV2_FRAMESIZE_10MB;	break;
			case 6:		outValue = NTV2_FRAMESIZE_12MB;	break;
			case 7:		outValue = NTV2_FRAMESIZE_14MB;	break;
			case 8:		outValue = NTV2_FRAMESIZE_16MB;	break;
			case 9:		outValue = NTV2_FRAMESIZE_18MB;	break;
			case 10:	outValue = NTV2_FRAMESIZE_20MB;	break;
			case 11:	outValue = NTV2_FRAMESIZE_22MB;	break;
			case 12:	outValue = NTV2_FRAMESIZE_24MB;	break;
			case 13:	outValue = NTV2_FRAMESIZE_26MB;	break;
			case 14:	outValue = NTV2_FRAMESIZE_28MB;	break;
			case 15:	outValue = NTV2_FRAMESIZE_30MB;	break;
			case 16:	outValue = NTV2_FRAMESIZE_32MB;	break;
			default:	return false;
			}
			NTV2_ASSERT (NTV2_IS_8MB_OR_16MB_FRAMESIZE(outValue));
			return true;
		}
	}
#endif	//	defined (NTV2_ALLOW_2MB_FRAMES)
	return CNTV2DriverInterface::ReadRegister (gChannelToControlRegNum[NTV2_CHANNEL1], outValue, kK2RegMaskFrameSize, kK2RegShiftFrameSize);
}


bool CNTV2Card::DisableChannel (const NTV2Channel inChannel)
{
	return WriteRegister (gChannelToControlRegNum [inChannel], ULWord (true), kRegMaskChannelDisable, kRegShiftChannelDisable);

}	//	DisableChannel


bool CNTV2Card::DisableChannels (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!DisableChannel(*it))
			failures++;
	return !failures;

}	//	DisableChannels


bool CNTV2Card::EnableChannel (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)
			&&  WriteRegister (gChannelToControlRegNum[inChannel], ULWord(false), kRegMaskChannelDisable, kRegShiftChannelDisable);

}	//	EnableChannel


bool CNTV2Card::EnableChannels (const NTV2ChannelSet & inChannels, const bool inDisableOthers)
{	UWord failures(0);
	for (NTV2Channel chan(NTV2_CHANNEL1);  chan < NTV2Channel(::NTV2DeviceGetNumFrameStores(GetDeviceID()));  chan = NTV2Channel(chan+1))
		if (inChannels.find(chan) == inChannels.end()  &&  inDisableOthers)
			DisableChannel(chan);
		else if (inChannels.find(chan) != inChannels.end())
			if (!EnableChannel(chan))
				failures++;
	return !failures;

}	//	EnableChannels


bool CNTV2Card::IsChannelEnabled (const NTV2Channel inChannel, bool & outEnabled)
{
	ULWord	value (0);
	if (!ReadRegister (gChannelToControlRegNum[inChannel], value, kRegMaskChannelDisable, kRegShiftChannelDisable))
		return false;
	outEnabled = value ? false : true;
	return true;
}	//	IsChannelEnabled


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::SetChannel2Disable(bool value)		{return WriteRegister (kRegCh2Control, value, kRegMaskChannelDisable, kRegShiftChannelDisable);}
	bool CNTV2Card::GetChannel2Disable(bool* value)
	{
		ULWord ULWvalue = *value;
		bool retVal =  ReadRegister (kRegCh2Control, &ULWvalue, kRegMaskChannelDisable, kRegShiftChannelDisable);
		*value = ULWvalue;
		return retVal;
	}

	bool CNTV2Card::SetChannel3Disable(bool value)		{return WriteRegister (kRegCh3Control, value, kRegMaskChannelDisable, kRegShiftChannelDisable);}
	bool CNTV2Card::GetChannel3Disable(bool* value)
	{
		ULWord ULWvalue = *value;
		bool retVal =  ReadRegister (kRegCh3Control, &ULWvalue, kRegMaskChannelDisable, kRegShiftChannelDisable);
		*value = ULWvalue;
		return retVal;
	}

	bool CNTV2Card::SetChannel4Disable(bool value)		{return WriteRegister (kRegCh4Control, value, kRegMaskChannelDisable, kRegShiftChannelDisable);}
	bool CNTV2Card::GetChannel4Disable(bool* value)
	{
		ULWord ULWvalue = *value;
		bool retVal =  ReadRegister (kRegCh4Control, &ULWvalue, kRegMaskChannelDisable, kRegShiftChannelDisable);
		*value = ULWvalue;
		return retVal;
	}
#endif	//	!defined (NTV2_DEPRECATE)


// Method: SetPCIAccessFrame
// Input:  NTV2Channel,  ULWord or equivalent(i.e. ULWord).
// Output: NONE
bool CNTV2Card::SetPCIAccessFrame (NTV2Channel channel, ULWord value, bool waitForVertical)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;
	bool result = WriteRegister (gChannelToPCIAccessFrameRegNum [channel], value);
	if (waitForVertical)
		WaitForOutputVerticalInterrupt (channel);

	return result;
}


// Method: FlopFlopPage
// Input:  NTV2Channel
// Output: NONE
bool CNTV2Card::FlipFlopPage (const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;

	ULWord pciAccessFrame(0);
	ULWord outputFrame(0);

	if (GetPCIAccessFrame (inChannel, pciAccessFrame))
		if (GetOutputFrame (inChannel, outputFrame))
			if (SetOutputFrame (inChannel, pciAccessFrame))
				if (SetPCIAccessFrame (inChannel, outputFrame))
					return true;
	return false;
}


bool CNTV2Card::GetPCIAccessFrame (const NTV2Channel inChannel, ULWord & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return ReadRegister (gChannelToPCIAccessFrameRegNum [inChannel], outValue);
}

bool CNTV2Card::SetOutputFrame (const NTV2Channel inChannel, const ULWord value)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (gChannelToOutputFrameRegNum [inChannel], value);
}

bool CNTV2Card::GetOutputFrame (const NTV2Channel inChannel, ULWord & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return ReadRegister (gChannelToOutputFrameRegNum [inChannel], outValue);
}

bool CNTV2Card::SetInputFrame (const NTV2Channel inChannel, const ULWord value)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (gChannelToInputFrameRegNum [inChannel], value);
}

bool CNTV2Card::GetInputFrame (const NTV2Channel inChannel, ULWord & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return ReadRegister (gChannelToInputFrameRegNum [inChannel], outValue);
}

bool CNTV2Card::SetAlphaFromInput2Bit (ULWord value)							{return WriteRegister (kRegCh1Control, value, kRegMaskAlphaFromInput2, kRegShiftAlphaFromInput2);}
bool CNTV2Card::GetAlphaFromInput2Bit (ULWord & outValue)						{return ReadRegister (kRegCh1Control, outValue, kRegMaskAlphaFromInput2, kRegShiftAlphaFromInput2);}
bool CNTV2Card::WriteGlobalControl (ULWord value)								{return WriteRegister (kRegGlobalControl, value);}
bool CNTV2Card::ReadGlobalControl (ULWord * pValue)								{return pValue ? ReadRegister (kRegGlobalControl, *pValue) : false;}

#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::WriteCh1Control (ULWord value)								{return WriteRegister (kRegCh1Control, value);}
	bool CNTV2Card::ReadCh1Control (ULWord *value)								{return ReadRegister (kRegCh1Control, value);}
	bool CNTV2Card::WriteCh1PCIAccessFrame (ULWord value)						{return WriteRegister (kRegCh1PCIAccessFrame, value);}
	bool CNTV2Card::ReadCh1PCIAccessFrame (ULWord *value)						{return ReadRegister (kRegCh1PCIAccessFrame, value);}
	bool CNTV2Card::WriteCh1OutputFrame (ULWord value)							{return WriteRegister (kRegCh1OutputFrame, value);}
	bool CNTV2Card::ReadCh1OutputFrame (ULWord *value)							{return ReadRegister (kRegCh1OutputFrame, value);}
	bool CNTV2Card::WriteCh1InputFrame (ULWord value)							{return WriteRegister (kRegCh1InputFrame, value);}
	bool CNTV2Card::ReadCh1InputFrame (ULWord *value)							{return ReadRegister (kRegCh1InputFrame, value);}
	bool CNTV2Card::WriteCh2Control (ULWord value)								{return WriteRegister (kRegCh2Control, value);}
	bool CNTV2Card::ReadCh2Control(ULWord *value)								{return ReadRegister (kRegCh2Control, value);}
	bool CNTV2Card::WriteCh2PCIAccessFrame (ULWord value)						{return WriteRegister (kRegCh2PCIAccessFrame, value);}
	bool CNTV2Card::ReadCh2PCIAccessFrame (ULWord *value)						{return ReadRegister (kRegCh2PCIAccessFrame, value);}
	bool CNTV2Card::WriteCh2OutputFrame (ULWord value)							{return WriteRegister (kRegCh2OutputFrame, value);}
	bool CNTV2Card::ReadCh2OutputFrame (ULWord *value)							{return ReadRegister (kRegCh2OutputFrame, value);}
	bool CNTV2Card::WriteCh2InputFrame (ULWord value)							{return WriteRegister (kRegCh2InputFrame, value);}
	bool CNTV2Card::ReadCh2InputFrame (ULWord *value)							{return ReadRegister (kRegCh2InputFrame, value);}
	bool CNTV2Card::WriteCh3Control (ULWord value)								{return WriteRegister (kRegCh3Control, value);}
	bool CNTV2Card::ReadCh3Control (ULWord *value)								{return ReadRegister (kRegCh3Control, value);}
	bool CNTV2Card::WriteCh3PCIAccessFrame (ULWord value)						{return WriteRegister (kRegCh3PCIAccessFrame, value);}
	bool CNTV2Card::ReadCh3PCIAccessFrame (ULWord *value)						{return ReadRegister (kRegCh3PCIAccessFrame, value);}
	bool CNTV2Card::WriteCh3OutputFrame (ULWord value)							{return WriteRegister (kRegCh3OutputFrame, value);}
	bool CNTV2Card::ReadCh3OutputFrame (ULWord *value)							{return ReadRegister (kRegCh3OutputFrame, value);}
	bool CNTV2Card::WriteCh3InputFrame (ULWord value)							{return WriteRegister (kRegCh3InputFrame, value);}
	bool CNTV2Card::ReadCh3InputFrame (ULWord *value)							{return ReadRegister (kRegCh3InputFrame, value);}
	bool CNTV2Card::WriteCh4Control (ULWord value)								{return WriteRegister (kRegCh4Control, value);}
	bool CNTV2Card::ReadCh4Control (ULWord *value)								{return ReadRegister (kRegCh4Control, value);}
	bool CNTV2Card::WriteCh4PCIAccessFrame (ULWord value)						{return WriteRegister (kRegCh4PCIAccessFrame, value);}
	bool CNTV2Card::ReadCh4PCIAccessFrame (ULWord *value)						{return ReadRegister (kRegCh4PCIAccessFrame, value);}
	bool CNTV2Card::WriteCh4OutputFrame (ULWord value)							{return WriteRegister (kRegCh4OutputFrame, value);}
	bool CNTV2Card::ReadCh4OutputFrame (ULWord *value)							{return ReadRegister (kRegCh4OutputFrame, value);}
	bool CNTV2Card::WriteCh4InputFrame (ULWord value)							{return WriteRegister (kRegCh4InputFrame, value);}
	bool CNTV2Card::ReadCh4InputFrame (ULWord *value)							{return ReadRegister (kRegCh4InputFrame, value);}
#endif	//	!defined (NTV2_DEPRECATE)


bool CNTV2Card::ReadLineCount (ULWord & outValue)
{
	return ReadRegister (kRegLineCount, outValue);
}

bool CNTV2Card::ReadFlashProgramControl (ULWord & outValue)
{
	return ReadRegister (kRegFlashProgramReg, outValue);
}


// Determine if Xilinx programmed
// Input:  NONE
// Output: ULWord or equivalent(i.e. ULWord).
bool CNTV2Card::IsXilinxProgrammed()
{
    ULWord programFlashValue;
	if(ReadFlashProgramControl(programFlashValue))
	{
		if ((programFlashValue & BIT(9)) == BIT(9))
		{
			return true;
		}
	}
	return false;
}

bool CNTV2Card::GetProgramStatus(SSC_GET_FIRMWARE_PROGRESS_STRUCT *statusStruct)
{
	ULWord totalSize = 0;
	ULWord totalProgress = 0;
	ULWord state = kProgramStateFinished;
	ReadRegister(kVRegFlashSize, totalSize);
	ReadRegister(kVRegFlashStatus, totalProgress);
	ReadRegister(kVRegFlashState, state);
	statusStruct->programTotalSize = totalSize;
	statusStruct->programProgress = totalProgress;
	statusStruct->programState = ProgramState(state);
	return true;
}

bool CNTV2Card::ProgramMainFlash(const char *fileName, bool bForceUpdate, bool bQuiet)
{
    CNTV2KonaFlashProgram thisDevice;
    thisDevice.SetBoard(GetIndexNumber());
    try
    {
		if (bQuiet)
			thisDevice.SetQuietMode();
        thisDevice.SetBitFile(fileName, MAIN_FLASHBLOCK);
        if(bForceUpdate)
            thisDevice.SetMBReset();
        thisDevice.Program(false);
    }
    catch (const char* Message)
    {
        (void)Message;
        return false;
    }
    return true;
}

bool CNTV2Card::GetRunningFirmwarePackageRevision (ULWord & outRevision)
{
    outRevision = 0;
    if (!IsOpen())
        return false;

    if (!IsIPDevice())
        return false;

    return ReadRegister(kRegSarekPackageVersion + SAREK_REGS, outRevision);
}

bool CNTV2Card::GetRunningFirmwareRevision (UWord & outRevision)
{
	outRevision = 0;
	if (!IsOpen())
		return false;

	uint32_t	regValue	(0);
	if (!ReadRegister (kRegDMAControl, regValue))
		return false;

	outRevision = uint16_t((regValue & 0x0000FF00) >> 8);
	return true;
}

bool CNTV2Card::GetRunningFirmwareDate (UWord & outYear, UWord & outMonth, UWord & outDay)
{
	outYear = outMonth = outDay = 0;
	if (!::NTV2DeviceCanReportRunningFirmwareDate(GetDeviceID()))
		return false;

	uint32_t	regValue	(0);
	if (!ReadRegister(kRegBitfileDate, regValue))
		return false;

	const UWord	yearBCD		((regValue & 0xFFFF0000) >> 16);	//	Year number in BCD
	const UWord	monthBCD	((regValue & 0x0000FF00) >> 8);		//	Month number in BCD
	const UWord	dayBCD		 (regValue & 0x000000FF);			//	Day number in BCD

	outYear =		((yearBCD & 0xF000) >> 12) * 1000
				+   ((yearBCD & 0x0F00) >>  8) * 100
				+   ((yearBCD & 0x00F0) >>  4) * 10
				+    (yearBCD & 0x000F);

	outMonth = ((monthBCD & 0x00F0) >> 4) * 10   +   (monthBCD & 0x000F);

	outDay = ((dayBCD & 0x00F0) >> 4) * 10   +   (dayBCD & 0x000F);

	return	outYear > 2010
		&&	outMonth > 0	&&  outMonth < 13
		&&	outDay > 0		&&  outDay < 32;		//	If the date's valid, then it's supported;  otherwise, it ain't
}


bool CNTV2Card::GetRunningFirmwareTime (UWord & outHours, UWord & outMinutes, UWord & outSeconds)
{
	outHours = outMinutes = outSeconds = 0;
	if (!::NTV2DeviceCanReportRunningFirmwareDate(GetDeviceID()))
		return false;

	uint32_t	regValue	(0);
	if (!ReadRegister(kRegBitfileTime, regValue))
		return false;

	const UWord	hoursBCD	((regValue & 0x00FF0000) >> 16);	//	Hours number in BCD
	const UWord	minutesBCD	((regValue & 0x0000FF00) >> 8);		//	Minutes number in BCD
	const UWord	secondsBCD	 (regValue & 0x000000FF);			//	Seconds number in BCD

	outHours = ((hoursBCD & 0x00F0) >>  4) * 10   +    (hoursBCD & 0x000F);

	outMinutes = ((minutesBCD & 0x00F0) >>  4) * 10   +    (minutesBCD & 0x000F);

	outSeconds = ((secondsBCD & 0x00F0) >>  4) * 10   +    (secondsBCD & 0x000F);

	return outHours < 24  &&  outMinutes < 60  &&  outSeconds < 60;	//	If the time's valid, then it's supported;  otherwise, it ain't
}


bool CNTV2Card::GetRunningFirmwareDate (std::string & outDate, std::string & outTime)
{
	outDate = outTime = string();
	UWord	yr(0), mo(0), dy(0), hr(0), mn(0), sec(0);
	if (!GetRunningFirmwareDate (yr, mo, dy))
		return false;
	if (!GetRunningFirmwareTime (hr, mn, sec))
		return false;

	ostringstream	date, time;
	date	<< DEC0N(yr,4)	<< "/"	<< DEC0N(mo,2)	<< "/"	<< DEC0N(dy,2);
	time	<< DEC0N(hr,2)	<< ":"	<< DEC0N(mn,2)	<< ":"	<< DEC0N(sec,2);

	outDate = date.str();
	outTime = time.str();
	return true;
}


bool CNTV2Card::GetRunningFirmwareUserID (ULWord & outUserID)
{
    outUserID = 0;
    if (!IsOpen())
        return false;

    ULWord	regValue	(0);
    if (!ReadRegister (kRegFirmwareUserID, regValue))
        return false;

    outUserID = regValue;
    return true;
}


///////////////////////////////////////////////////////////////////

bool CNTV2Card::ReadStatusRegister (ULWord *pValue)						{return pValue ? ReadRegister(kRegStatus, *pValue) : false;}
bool CNTV2Card::ReadStatus2Register (ULWord *pValue)					{return pValue ? ReadRegister(kRegStatus2, *pValue) : false;}
bool CNTV2Card::ReadInputStatusRegister (ULWord *pValue)				{return pValue ? ReadRegister(kRegInputStatus, *pValue) : false;}
bool CNTV2Card::ReadInputStatus2Register (ULWord *pValue)				{return pValue ? ReadRegister(kRegInputStatus2, *pValue) : false;}
bool CNTV2Card::ReadInput56StatusRegister (ULWord *pValue)				{return pValue ? ReadRegister(kRegInput56Status, *pValue) : false;}
bool CNTV2Card::ReadInput78StatusRegister (ULWord *pValue)				{return pValue ? ReadRegister(kRegInput78Status, *pValue) : false;}
bool CNTV2Card::Read3GInputStatusRegister(ULWord *pValue)				{return pValue ? ReadRegister(kRegSDIInput3GStatus, *pValue) : false;}
bool CNTV2Card::Read3GInputStatus2Register(ULWord *pValue)				{return pValue ? ReadRegister(kRegSDIInput3GStatus2, *pValue) : false;}
bool CNTV2Card::Read3GInput5678StatusRegister(ULWord *pValue)			{return pValue ? ReadRegister(kRegSDI5678Input3GStatus, *pValue) : false;}


bool CNTV2Card::GetVPIDValidA(const NTV2Channel inChannel)
{
	ULWord value(0);
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return ReadRegister(gChannelToSDIInput3GStatusRegNum[inChannel], value, gChannelToSDIInVPIDLinkAValidMask[inChannel])
			&&  value;
}

bool CNTV2Card::GetVPIDValidB(const NTV2Channel inChannel)
{
	ULWord value(0);
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return ReadRegister(gChannelToSDIInput3GStatusRegNum[inChannel], value, gChannelToSDIInVPIDLinkBValidMask[inChannel])
			&&  value;
}

bool CNTV2Card::ReadSDIInVPID (const NTV2Channel inChannel, ULWord & outValue_A, ULWord & outValue_B)
{
	ULWord	status	(0);
	ULWord	valA	(0);
	ULWord	valB	(0);

	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	if (!ReadRegister (gChannelToSDIInput3GStatusRegNum	[inChannel], status))
		return false;
	if (status & gChannelToSDIInVPIDLinkAValidMask [inChannel])
	{
		if (!ReadRegister (gChannelToSDIInVPIDARegNum [inChannel], valA))
			return false;
	}
	else
	{
		outValue_A = 0;
		outValue_B = 0;
		return false;
	}

	if (!ReadRegister (gChannelToSDIInput3GStatusRegNum	[inChannel], status))
		return false;
	if (status & gChannelToSDIInVPIDLinkBValidMask [inChannel])
	{
		if (!ReadRegister (gChannelToSDIInVPIDBRegNum [inChannel], valB))
			return false;
	}

	// reverse byte order
	if (GetDeviceID() == DEVICE_ID_KONALHI)
	{
		outValue_A = valA;
		outValue_B = valB;
	}
	else
	{
		outValue_A = NTV2EndianSwap32(valA);
		outValue_B = NTV2EndianSwap32(valB);
	}
	
	return true;

}	//	ReadSDIInVPID


bool CNTV2Card::WriteSDIInVPID (const NTV2Channel inChannel, const ULWord inValA, const ULWord inValB)
{
	ULWord	valA(inValA), valB(inValB);

	if (IS_CHANNEL_INVALID(inChannel))
		return false;

	// reverse byte order
	if (GetDeviceID() != DEVICE_ID_KONALHI)
	{
		valA = NTV2EndianSwap32(valA);
		valB = NTV2EndianSwap32(valB);
	}

	return WriteRegister (gChannelToSDIInVPIDARegNum[inChannel], valA)
			&&  WriteRegister (gChannelToSDIInVPIDBRegNum[inChannel], valB);

}	//	WriteSDIInVPID



#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::ReadSDIInVPID (NTV2Channel channel, ULWord* valueA, ULWord* valueB)
	{
		ULWord	valA (0), valB (0);
		bool	result (ReadSDIInVPID (channel, valA, valB));
		if (result)
		{
			if (valueA)
				*valueA = valA;
			if (valueB)
				*valueB = valB;
		}

		return result;
	}

	bool CNTV2Card::ReadSDI1InVPID (ULWord* valueA, ULWord* valueB)		{return ReadSDIInVPID (NTV2_CHANNEL1, valueA, valueB);}
	bool CNTV2Card::ReadSDI2InVPID (ULWord* valueA, ULWord* valueB)		{return ReadSDIInVPID (NTV2_CHANNEL2, valueA, valueB);}
	bool CNTV2Card::ReadSDI3InVPID (ULWord* valueA, ULWord* valueB)		{return ReadSDIInVPID (NTV2_CHANNEL3, valueA, valueB);}
	bool CNTV2Card::ReadSDI4InVPID (ULWord* valueA, ULWord* valueB)		{return ReadSDIInVPID (NTV2_CHANNEL4, valueA, valueB);}
	bool CNTV2Card::ReadUartRxFifoSize (ULWord * pOutSize)				{return pOutSize ? ReadRegister (kVRegUartRxFifoSize, pOutSize) : false;}
#endif	//	!defined (NTV2_DEPRECATE)


bool CNTV2Card::GetPCIDeviceID (ULWord & outPCIDeviceID)				{return ReadRegister (kVRegPCIDeviceID, outPCIDeviceID);}

bool CNTV2Card::SupportsP2PTransfer (void)
{
	ULWord	pciID	(0);

	if (GetPCIDeviceID (pciID))
		switch (pciID)
		{
			case 0xDB07:  // Kona3G + P2P
			case 0xDB08:  // Kona3G Quad + P2P
			case 0xEB0B:  // Kona4quad
			case 0xEB0C:  // Kona4ufc
			case 0xEB0E:  // Corvid 44
			case 0xEB0D:  // Corvid 88
				return true;

			default:
				return false;
		}
	return false;
}


bool CNTV2Card::SupportsP2PTarget (void)
{
	ULWord	pciID	(0);

	if (GetPCIDeviceID (pciID))
		switch (pciID)
		{
			case 0xDB07:  // Kona3G + P2P
			case 0xDB08:  // Kona3G Quad + P2P
			case 0xEB0C:  // Kona4ufc
			case 0xEB0E:  // Corvid 44
			case 0xEB0D:  // Corvid 88
				return true;

			default:
				return false;
		}
	return false;
}


bool CNTV2Card::SetRegisterWriteMode (const NTV2RegisterWriteMode value, const NTV2Channel inFrameStore)
{
    bool ret;

	if (IS_CHANNEL_INVALID(inFrameStore))
		return false;
    if (IsMultiFormatActive())
        return WriteRegister (gChannelToGlobalControlRegNum[inFrameStore], value, kRegMaskRegClocking, kRegShiftRegClocking);
    if (::NTV2DeviceCanDoMultiFormat(GetDeviceID()))
    {
        for (NTV2Channel chan(NTV2_CHANNEL1);  chan < NTV2Channel(::NTV2DeviceGetNumFrameStores(GetDeviceID()));  chan = NTV2Channel(chan+1))
        {
            ret = WriteRegister (gChannelToGlobalControlRegNum[chan], value, kRegMaskRegClocking, kRegShiftRegClocking);
            if (!ret)
                return false;
        }
        return true;
    }
    return WriteRegister (gChannelToGlobalControlRegNum[NTV2_CHANNEL1], value, kRegMaskRegClocking, kRegShiftRegClocking);
}


bool CNTV2Card::GetRegisterWriteMode (NTV2RegisterWriteMode & outValue, const NTV2Channel inFrameStore)
{
	if (IS_CHANNEL_INVALID (inFrameStore))
		return false;
	ULWord	value(0);
	if (!ReadRegister(gChannelToGlobalControlRegNum[IsMultiFormatActive() ? inFrameStore : NTV2_CHANNEL1],
								value, kRegMaskRegClocking, kRegShiftRegClocking))
		return false;
	outValue = NTV2RegisterWriteMode(value);
	return true;
}


bool CNTV2Card::SetLEDState (ULWord value)								{return WriteRegister (kRegGlobalControl, value, kRegMaskLED, kRegShiftLED);}
bool CNTV2Card::GetLEDState (ULWord & outValue)							{return ReadRegister (kRegGlobalControl, outValue, kRegMaskLED, kRegShiftLED);}


////////////////////////////////////////////////////////////////////
// RP188 methods
////////////////////////////////////////////////////////////////////

bool CNTV2Card::SetRP188Mode (const NTV2Channel inChannel, const NTV2_RP188Mode inValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (gChannelToRP188ModeGCRegisterNum [inChannel],	inValue, gChannelToRP188ModeMasks [inChannel], gChannelToRP188ModeShifts [inChannel]);
}


bool CNTV2Card::GetRP188Mode (const NTV2Channel inChannel, NTV2_RP188Mode & outMode)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	const bool	result	(CNTV2DriverInterface::ReadRegister (gChannelToRP188ModeGCRegisterNum[inChannel], outMode, gChannelToRP188ModeMasks[inChannel], gChannelToRP188ModeShifts[inChannel]));
	if (!result)
		outMode = NTV2_RP188_INVALID;
	return result;
}


bool CNTV2Card::GetRP188Data (const NTV2Channel inChannel, NTV2_RP188 & outRP188Data)
{
	outRP188Data = NTV2_RP188();
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return		ReadRegister (gChlToRP188DBBRegNum[inChannel],		outRP188Data.fDBB, kRegMaskRP188DBB, kRegShiftRP188DBB)
			&&	ReadRegister (gChlToRP188Bits031RegNum[inChannel],	outRP188Data.fLo)
			&&	ReadRegister (gChlToRP188Bits3263RegNum[inChannel],	outRP188Data.fHi);
}

#if !defined(NTV2_DEPRECATE_15_2)
	bool CNTV2Card::GetRP188Data (const NTV2Channel inChannel, ULWord inFrame, RP188_STRUCT & outRP188Data)
	{
		(void)		inFrame;
		NTV2_RP188	rp188data;
		const bool	result (GetRP188Data(inChannel, rp188data));
		outRP188Data = rp188data;
		return result;
	}

	bool CNTV2Card::SetRP188Data (const NTV2Channel inChannel, const ULWord inFrame, const RP188_STRUCT & inRP188Data)
	{
		(void) inFrame;
		return SetRP188Data(inChannel, NTV2_RP188(inRP188Data));
	}
#endif	//	!defined(NTV2_DEPRECATE_15_2)


bool CNTV2Card::SetRP188Data (const NTV2Channel inChannel, const NTV2_RP188 & inRP188Data)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	if (!inRP188Data.IsValid())
		return false;
	return		WriteRegister (gChlToRP188DBBRegNum[inChannel],		inRP188Data.fDBB, kRegMaskRP188DBB, kRegShiftRP188DBB)
			&&	WriteRegister (gChlToRP188Bits031RegNum[inChannel],	inRP188Data.fLo)
			&&	WriteRegister (gChlToRP188Bits3263RegNum[inChannel],	inRP188Data.fHi);
}


bool CNTV2Card::SetRP188SourceFilter (const NTV2Channel inChannel, UWord inValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (gChlToRP188DBBRegNum[inChannel], ULWord(inValue), kRegMaskRP188SourceSelect, kRegShiftRP188Source);
}


bool CNTV2Card::GetRP188SourceFilter (const NTV2Channel inChannel, UWord & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChlToRP188DBBRegNum[inChannel], outValue, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
}


bool CNTV2Card::IsRP188BypassEnabled (const NTV2Channel inChannel, bool & outIsBypassEnabled)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	//	Bit 23 of the RP188 DBB register will be set if output timecode will be grabbed directly from an input (bypass source)...
	ULWord	regValue	(0);
	bool	result		(NTV2_IS_VALID_CHANNEL(inChannel) && ReadRegister(gChlToRP188DBBRegNum[inChannel], regValue));
	if (result)
		outIsBypassEnabled = regValue & BIT(23);
	return result;
}


bool CNTV2Card::DisableRP188Bypass (const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	//	Clear bit 23 of my output destination's RP188 DBB register...
	return NTV2_IS_VALID_CHANNEL (inChannel) && WriteRegister (gChlToRP188DBBRegNum[inChannel], 0, BIT(23), 23);
}


bool CNTV2Card::EnableRP188Bypass (const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	//	Set bit 23 of my output destination's RP188 DBB register...
	return NTV2_IS_VALID_CHANNEL (inChannel) && WriteRegister (gChlToRP188DBBRegNum [inChannel], 1, BIT(23), 23);
}

static const ULWord gSDIOutToRP188Input[] = { 0, 2, 1, 3, 0, 2, 1, 3, 0 };

bool CNTV2Card::SetRP188BypassSource (const NTV2Channel inSDIOutput, const UWord inSDIInput)
{
	if (IS_CHANNEL_INVALID(inSDIOutput))
		return false;
	if (IS_CHANNEL_INVALID(NTV2Channel(inSDIInput)))
		return false;
	return WriteRegister(gChlToRP188DBBRegNum[inSDIOutput], gSDIOutToRP188Input[inSDIInput], BIT(21)|BIT(22), 21);
}

bool CNTV2Card::GetRP188BypassSource (const NTV2Channel inSDIOutput, UWord & outSDIInput)
{
	if (IS_CHANNEL_INVALID(inSDIOutput))
		return false;
	ULWord	val(0);
	if (!ReadRegister(gChlToRP188DBBRegNum[inSDIOutput], val, BIT(21)|BIT(22), 21))
		return false;
	switch(val)
	{
		case 0:		outSDIInput =  inSDIOutput < NTV2_CHANNEL5  ?  0  :  4;		break;
		case 2:		outSDIInput =  inSDIOutput < NTV2_CHANNEL5  ?  1  :  5;		break;
		case 1:		outSDIInput =  inSDIOutput < NTV2_CHANNEL5  ?  2  :  6;		break;
		case 3:		outSDIInput =  inSDIOutput < NTV2_CHANNEL5  ?  3  :  7;		break;
		default:	return false;
	}
	return true;
}


bool CNTV2Card::SetVideoLimiting (const NTV2VideoLimiting inValue)
{
	if (!NTV2_IS_VALID_VIDEOLIMITING(inValue))
		return false;
	CVIDINFO("'" << GetDisplayName() << "' set to " << ::NTV2VideoLimitingToString(inValue));
	return WriteRegister (kRegVidProc1Control, inValue, kRegMaskVidProcLimiting, kRegShiftVidProcLimiting);
}

bool CNTV2Card::GetVideoLimiting (NTV2VideoLimiting & outValue)
{
	return CNTV2DriverInterface::ReadRegister (kRegVidProc1Control, outValue, kRegMaskVidProcLimiting, kRegShiftVidProcLimiting);
}


//SetEnableVANCData
// Call SetVideoFormat with the desired video format BEFORE you call this function!
bool CNTV2Card::SetEnableVANCData (const bool inVANCenable, const bool inTallerVANC, const NTV2Channel inChannel)
{
	return SetVANCMode (NTV2VANCModeFromBools(inVANCenable, inTallerVANC), IsMultiFormatActive() ? inChannel : NTV2_CHANNEL1);
}

bool CNTV2Card::SetVANCMode (const NTV2ChannelSet & inChannels, const NTV2VANCMode inVancMode)
{
	size_t errors(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!SetEnableVANCData (inVancMode, *it))
			errors++;
	return !errors;
}


bool CNTV2Card::SetVANCMode (const NTV2VANCMode inVancMode, const NTV2Channel inChannel)
{
	const NTV2Channel ch (IsMultiFormatActive() ? inChannel : NTV2_CHANNEL1);
	if (IS_CHANNEL_INVALID(ch))
		return false;
	if (!NTV2_IS_VALID_VANCMODE(inVancMode))
		return false;

	NTV2FrameGeometry	fg(NTV2_FG_INVALID);
	NTV2Standard		st(NTV2_STANDARD_INVALID);
	GetStandard(st, ch);
	GetFrameGeometry(fg, ch);
	switch (st)
	{
		case NTV2_STANDARD_1080:
		case NTV2_STANDARD_1080p:
			if (fg == NTV2_FG_1920x1112  ||  fg == NTV2_FG_1920x1114  ||  fg == NTV2_FG_1920x1080)
				fg = NTV2_IS_VANCMODE_TALLER(inVancMode) ? NTV2_FG_1920x1114 : (NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_1920x1112 : NTV2_FG_1920x1080);
			else if (NTV2_IS_QUAD_FRAME_GEOMETRY(fg))		// 4K
				;	// do nothing for now
			else if (NTV2_IS_2K_1080_FRAME_GEOMETRY(fg))	// 2Kx1080
				fg = NTV2_IS_VANCMODE_TALLER(inVancMode) ? NTV2_FG_2048x1114 : (NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_2048x1112 : NTV2_FG_2048x1080);
			break;

		case NTV2_STANDARD_720:
			fg = NTV2_IS_VANCMODE_ON(inVancMode) ? NTV2_FG_1280x740 : NTV2_FG_1280x720;
			if (NTV2_IS_VANCMODE_TALLER(inVancMode))
				CVIDWARN("'taller' mode requested for 720p -- using 'tall' geometry instead");
			break;

		case NTV2_STANDARD_525:
			fg = NTV2_IS_VANCMODE_TALLER(inVancMode) ? NTV2_FG_720x514 : (NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_720x508 : NTV2_FG_720x486);
			break;

		case NTV2_STANDARD_625:
			fg = NTV2_IS_VANCMODE_TALLER(inVancMode) ? NTV2_FG_720x612 : (NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_720x598 : NTV2_FG_720x576);
			break;

		case NTV2_STANDARD_2K:
			fg = NTV2_IS_VANCMODE_ON(inVancMode) ? NTV2_FG_2048x1588 : NTV2_FG_2048x1556;
			if (NTV2_IS_VANCMODE_TALLER(inVancMode))
				CVIDWARN("'taller' mode requested for 2K standard '" << ::NTV2StandardToString(st) << "' -- using 'tall' instead");
			break;

		case NTV2_STANDARD_2Kx1080p:
		case NTV2_STANDARD_2Kx1080i:
			fg = NTV2_IS_VANCMODE_TALLER(inVancMode) ? NTV2_FG_2048x1114 : (NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_2048x1112 : NTV2_FG_2048x1080);
			break;

		case NTV2_STANDARD_3840x2160p:
		case NTV2_STANDARD_4096x2160p:
		case NTV2_STANDARD_3840HFR:
		case NTV2_STANDARD_4096HFR:
		case NTV2_STANDARD_7680:
		case NTV2_STANDARD_8192:
		case NTV2_STANDARD_3840i:
		case NTV2_STANDARD_4096i:
			if (NTV2_IS_VANCMODE_ON(inVancMode))
				CVIDWARN("'tall' or 'taller' mode requested for '" << ::NTV2StandardToString(st) << "' -- using non-VANC geometry instead");
			break;
	#if defined(_DEBUG)
		case NTV2_STANDARD_INVALID:		return false;
	#else
		default:						return false;
	#endif	//	_DEBUG
	}
	SetFrameGeometry (fg, false/*ajaRetail*/, ch);
	CVIDINFO("'" << GetDisplayName() << "' Ch" << DEC(ch+1) << ": set to " << ::NTV2VANCModeToString(inVancMode) << " for " << ::NTV2StandardToString(st) << " and " << ::NTV2FrameGeometryToString(fg));

	//	Only muck with limiting if not the xena2k board. Xena2k only turns off limiting in VANC area. Active video uses vidproccontrol setting...
	if (!::NTV2DeviceNeedsRoutingSetup(GetDeviceID()))
		SetVideoLimiting (NTV2_IS_VANCMODE_ON(inVancMode) ? NTV2_VIDEOLIMITING_OFF : NTV2_VIDEOLIMITING_LEGALSDI);

	return true;
}


//SetEnableVANCData - extended params
bool CNTV2Card::SetEnableVANCData (const bool inVANCenabled, const bool inTallerVANC, const NTV2Standard inStandard, const NTV2FrameGeometry inFrameGeometry, const NTV2Channel inChannel)
{	(void) inStandard; (void) inFrameGeometry;
	if (inTallerVANC && !inVANCenabled)
		return false;	//	conflicting VANC params
	return SetVANCMode (NTV2VANCModeFromBools (inVANCenabled, inTallerVANC), inChannel);
}


bool CNTV2Card::GetVANCMode (NTV2VANCMode & outVancMode, const NTV2Channel inChannel)
{
	bool				isTall			(false);
	bool				isTaller		(false);
	const NTV2Channel	channel			(IsMultiFormatActive() ? inChannel : NTV2_CHANNEL1);
	NTV2Standard		standard		(NTV2_STANDARD_INVALID);
	NTV2FrameGeometry	frameGeometry	(NTV2_FG_INVALID);

	outVancMode = NTV2_VANCMODE_INVALID;
	if (IS_CHANNEL_INVALID (channel))
		return false;

	GetStandard (standard, channel);
	GetFrameGeometry (frameGeometry, channel);

	switch (standard)
	{
		case NTV2_STANDARD_1080:
		case NTV2_STANDARD_1080p:
		case NTV2_STANDARD_2Kx1080p:	//	** MrBill **	IS THIS CORRECT?
		case NTV2_STANDARD_2Kx1080i:	//	** MrBill **	IS THIS CORRECT?
			if ( frameGeometry == NTV2_FG_1920x1112 || frameGeometry == NTV2_FG_2048x1112 ||
				 frameGeometry == NTV2_FG_1920x1114 || frameGeometry == NTV2_FG_2048x1114)
				isTall = true;
			if ( frameGeometry == NTV2_FG_1920x1114 || frameGeometry == NTV2_FG_2048x1114)
				isTaller = true;
			break;
		case NTV2_STANDARD_720:
			if ( frameGeometry == NTV2_FG_1280x740)
				isTall = true;
			break;
		case NTV2_STANDARD_525:
			if ( frameGeometry == NTV2_FG_720x508 ||
				 frameGeometry == NTV2_FG_720x514)
				isTall = true;
			if ( frameGeometry == NTV2_FG_720x514 )
				isTaller = true;
			break;
		case NTV2_STANDARD_625:
			if ( frameGeometry == NTV2_FG_720x598 ||
				 frameGeometry == NTV2_FG_720x612)
				isTall = true;
			if ( frameGeometry == NTV2_FG_720x612 )
				isTaller = true;
			break;
		case NTV2_STANDARD_2K:
			if ( frameGeometry == NTV2_FG_2048x1588)
				isTall = true;
			break;
		case NTV2_STANDARD_3840x2160p:
		case NTV2_STANDARD_4096x2160p:
		case NTV2_STANDARD_3840HFR:
		case NTV2_STANDARD_4096HFR:
		case NTV2_STANDARD_7680:
		case NTV2_STANDARD_8192:
		case NTV2_STANDARD_3840i:
		case NTV2_STANDARD_4096i:
			break;
	#if defined (_DEBUG)
		case NTV2_NUM_STANDARDS:	return false;
	#else
		default:					return false;
	#endif
	}
	outVancMode = NTV2VANCModeFromBools (isTall, isTaller);
	return true;
}


#if !defined(NTV2_DEPRECATE_14_3)
	bool CNTV2Card::GetEnableVANCData (bool & outIsEnabled, bool & outIsWideVANC, const NTV2Channel inChannel)
	{
		NTV2VANCMode	vancMode	(NTV2_VANCMODE_INVALID);
		outIsEnabled = outIsWideVANC = false;
		if (!GetVANCMode (vancMode, inChannel))
			return false;
		if (!NTV2_IS_VALID_VANCMODE(vancMode))
			return false;
		outIsEnabled = NTV2_IS_VANCMODE_ON(vancMode);
		outIsWideVANC = NTV2_IS_VANCMODE_TALLER(vancMode);
		return true;
	}


	bool CNTV2Card::GetEnableVANCData (bool * pOutIsEnabled, bool * pOutIsWideVANCEnabled, NTV2Channel inChannel)
	{
		NTV2VANCMode	vancMode	(NTV2_VANCMODE_INVALID);
		if (!pOutIsEnabled)
			return false;
		if (!GetVANCMode (vancMode, inChannel))
			return false;
		if (!NTV2_IS_VALID_VANCMODE(vancMode))
			return false;
		*pOutIsEnabled = NTV2_IS_VANCMODE_ON(vancMode);
		if (pOutIsWideVANCEnabled)
			*pOutIsWideVANCEnabled = NTV2_IS_VANCMODE_TALLER(vancMode);
		return true;
	}
#endif	//	!defined(NTV2_DEPRECATE_14_3)


bool CNTV2Card::SetVANCShiftMode (NTV2Channel inChannel, NTV2VANCDataShiftMode inValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	CVIDINFO("'" << GetDisplayName() << "' Ch" << DEC(inChannel+1) << ": Vanc data shift " << (inValue ? "enabled" : "disabled"));
	return WriteRegister (gChannelToControlRegNum [inChannel], inValue, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
}


bool CNTV2Card::GetVANCShiftMode (NTV2Channel inChannel, NTV2VANCDataShiftMode & outValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChannelToControlRegNum[inChannel], outValue, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
}


bool CNTV2Card::SetPulldownMode (NTV2Channel inChannel, bool inValue)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (inChannel == NTV2_CHANNEL2 ? kRegCh2ControlExtended : kRegCh1ControlExtended, inValue, kK2RegMaskPulldownMode, kK2RegShiftPulldownMode);
}


bool CNTV2Card::GetPulldownMode (NTV2Channel inChannel, bool & outValue)
{
	ULWord	value(0);
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	if (ReadRegister (inChannel == NTV2_CHANNEL2 ? kRegCh2ControlExtended : kRegCh1ControlExtended,  value,  kK2RegMaskPulldownMode,  kK2RegShiftPulldownMode))
	{
		outValue = value ? true : false;
		return true;
	}
	return false;
}


bool CNTV2Card::SetMixerVancOutputFromForeground (const UWord inWhichMixer, const bool inFromForegroundSource)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	CVIDINFO("'" << GetDisplayName() << "' Mixer" << DEC(inWhichMixer+1) << ": Vanc from " << (inFromForegroundSource ? "FG" : "BG"));
	return WriteRegister (gIndexToVidProcControlRegNum[inWhichMixer], inFromForegroundSource ? 1 : 0, kRegMaskVidProcVancSource, kRegShiftVidProcVancSource);
}


bool CNTV2Card::GetMixerVancOutputFromForeground (const UWord inWhichMixer, bool & outIsFromForegroundSource)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;

	ULWord	value	(0);
	bool	result	(ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], value, kRegMaskVidProcVancSource, kRegShiftVidProcVancSource));
	if (result)
		outIsFromForegroundSource = value ? true : false;
	return result;
}

#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::WritePanControl (ULWord value)							{return WriteRegister (kRegPanControl, value);}
	bool CNTV2Card::ReadPanControl (ULWord *value)							{return ReadRegister (kRegPanControl, value);}
#endif	//	!defined (NTV2_DEPRECATE)

bool CNTV2Card::SetMixerFGInputControl (const UWord inWhichMixer, const NTV2MixerKeyerInputControl inInputControl)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	CVIDINFO("'" << GetDisplayName() << "' Mixer" << DEC(inWhichMixer+1) << ": FG input ctrl=" << ::NTV2MixerInputControlToString(inInputControl));
	return WriteRegister (gIndexToVidProcControlRegNum[inWhichMixer], inInputControl, kK2RegMaskXena2FgVidProcInputControl, kK2RegShiftXena2FgVidProcInputControl);
}


bool CNTV2Card::GetMixerFGInputControl (const UWord inWhichMixer, NTV2MixerKeyerInputControl & outInputControl)
{
	outInputControl = NTV2MIXERINPUTCONTROL_INVALID;
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;

	ULWord	value	(0);
	bool	result	(ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], value, kK2RegMaskXena2FgVidProcInputControl, kK2RegShiftXena2FgVidProcInputControl));
	if (result)
		outInputControl = static_cast <NTV2MixerKeyerInputControl> (value);
	return result;
}


bool CNTV2Card::SetMixerBGInputControl (const UWord inWhichMixer, const NTV2MixerKeyerInputControl inInputControl)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	CVIDINFO("'" << GetDisplayName() << "' Mixer" << DEC(inWhichMixer+1) << ": BG input ctrl=" << ::NTV2MixerInputControlToString(inInputControl));
	return WriteRegister (gIndexToVidProcControlRegNum [inWhichMixer], inInputControl, kK2RegMaskXena2BgVidProcInputControl, kK2RegShiftXena2BgVidProcInputControl);
}


bool CNTV2Card::GetMixerBGInputControl (const UWord inWhichMixer, NTV2MixerKeyerInputControl & outInputControl)
{
	outInputControl = NTV2MIXERINPUTCONTROL_INVALID;
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;

	ULWord	value	(0);
	bool	result	(ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], value, kK2RegMaskXena2BgVidProcInputControl, kK2RegShiftXena2BgVidProcInputControl));
	if (result)
		outInputControl = static_cast <NTV2MixerKeyerInputControl> (value);
	return result;
}


bool CNTV2Card::SetMixerMode (const UWord inWhichMixer, const NTV2MixerKeyerMode inMode)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	CVIDINFO("'" << GetDisplayName() << "' Mixer" << DEC(inWhichMixer+1) << ": mode=" << ::NTV2MixerKeyerModeToString(inMode));
	return WriteRegister (gIndexToVidProcControlRegNum[inWhichMixer], inMode, kK2RegMaskXena2VidProcMode, kK2RegShiftXena2VidProcMode);
}


bool CNTV2Card::GetMixerMode (const UWord inWhichMixer, NTV2MixerKeyerMode & outMode)
{
	outMode = NTV2MIXERMODE_INVALID;

	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;

	ULWord	value	(0);
	bool	result	(ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], value, kK2RegMaskXena2VidProcMode, kK2RegShiftXena2VidProcMode));
	if (result)
		outMode = static_cast <NTV2MixerKeyerMode> (value);
	return result;
}


bool CNTV2Card::SetMixerCoefficient (const UWord inWhichMixer, const ULWord inMixCoefficient)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	CVIDINFO("'" << GetDisplayName() << "' Mixer" << DEC(inWhichMixer+1) << ": mixCoeff=" << xHEX0N(inMixCoefficient,8));
	return WriteRegister (gIndexToVidProcMixCoeffRegNum[inWhichMixer], inMixCoefficient);
}


bool CNTV2Card::GetMixerCoefficient (const UWord inWhichMixer, ULWord & outMixCoefficient)
{
	outMixCoefficient = 0;
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	return ReadRegister (gIndexToVidProcMixCoeffRegNum[inWhichMixer], outMixCoefficient);
}


bool CNTV2Card::GetMixerSyncStatus (const UWord inWhichMixer, bool & outIsSyncOK)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;

	bool syncFail (false);
	if (!CNTV2DriverInterface::ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], syncFail, kRegMaskVidProcSyncFail, kRegShiftVidProcSyncFail))
		return false;
	outIsSyncOK = syncFail ? false : true;
	return true;
}

bool CNTV2Card::GetMixerFGMatteEnabled (const UWord inWhichMixer, bool & outIsEnabled)
{
	outIsEnabled = false;
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	return !CNTV2DriverInterface::ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], outIsEnabled, kRegMaskVidProcFGMatteEnable, kRegShiftVidProcFGMatteEnable);
}

bool CNTV2Card::SetMixerFGMatteEnabled (const UWord inWhichMixer, const bool inIsEnabled)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	return !WriteRegister (gIndexToVidProcControlRegNum[inWhichMixer], inIsEnabled?1:0, kRegMaskVidProcFGMatteEnable, kRegShiftVidProcFGMatteEnable);
}

bool CNTV2Card::GetMixerBGMatteEnabled (const UWord inWhichMixer, bool & outIsEnabled)
{
	outIsEnabled = false;
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	return !CNTV2DriverInterface::ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], outIsEnabled, kRegMaskVidProcBGMatteEnable, kRegShiftVidProcBGMatteEnable);
}

bool CNTV2Card::SetMixerBGMatteEnabled (const UWord inWhichMixer, const bool inIsEnabled)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	return !WriteRegister (gIndexToVidProcControlRegNum[inWhichMixer], inIsEnabled?1:0, kRegMaskVidProcBGMatteEnable, kRegShiftVidProcBGMatteEnable);
}

static const ULWord	gMatteColorRegs[]	= {	kRegFlatMatteValue /*13*/,	kRegFlatMatte2Value	/*249*/,	kRegFlatMatte3Value /*487*/,	kRegFlatMatte4Value /*490*/, 0, 0, 0, 0};

bool CNTV2Card::GetMixerMatteColor (const UWord inWhichMixer, YCbCr10BitPixel & outYCbCrValue)
{
	ULWord	packedValue	(0);
	outYCbCrValue.cb = outYCbCrValue.y = outYCbCrValue.cr = 0;
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	if (!ReadRegister(gMatteColorRegs[inWhichMixer], packedValue))
		return false;

	outYCbCrValue.cb	=   packedValue        & 0x03FF;
	outYCbCrValue.y		= ((packedValue >> 10) & 0x03FF) + 0x0040;
	outYCbCrValue.cr	=  (packedValue >> 20) & 0x03FF;
	return true;
}

bool CNTV2Card::SetMixerMatteColor (const UWord inWhichMixer, const YCbCr10BitPixel inYCbCrValue)
{
	YCbCr10BitPixel	ycbcrPixel	(inYCbCrValue);
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;

	if (ycbcrPixel.y < 0x40) 
		ycbcrPixel.y = 0x0;	// clip y
	else
		ycbcrPixel.y -= 0x40;
	ycbcrPixel.y &= 0x3FF;
	ycbcrPixel.cb &= 0x3FF;
	ycbcrPixel.cr &= 0x3FF;

	//	Pack three 10-bit values into ULWord...
	const ULWord packedValue (ULWord(ycbcrPixel.cb)  |  (ULWord(ycbcrPixel.y) << 10)  |  (ULWord(ycbcrPixel.cr) << 20));
	CVIDINFO("'" << GetDisplayName() << "' Mixer" << DEC(inWhichMixer+1) << ": set to YCbCr=" << DEC(ycbcrPixel.y)
			<< "|" << DEC(ycbcrPixel.cb) << "|" << DEC(ycbcrPixel.cr) << ":" << HEXN(ycbcrPixel.y,3) << "|"
			<< HEXN(ycbcrPixel.cb,3) << "|" << HEXN(ycbcrPixel.cr,3) << ", write " << xHEX0N(packedValue,8)
			<< " into reg " << DEC(gMatteColorRegs[inWhichMixer]));

	//	Write it...
	return WriteRegister(gMatteColorRegs[inWhichMixer], packedValue);
}

bool CNTV2Card::MixerHasRGBModeSupport (const UWord inWhichMixer, bool & outIsSupported)
{
	outIsSupported = false;
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	return !CNTV2DriverInterface::ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], outIsSupported, kRegMaskVidProcRGBModeSupported, kRegShiftVidProcRGBModeSupported);
}

bool CNTV2Card::SetMixerRGBRange (const UWord inWhichMixer, const NTV2MixerRGBRange inRGBRange)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	return !WriteRegister (gIndexToVidProcControlRegNum[inWhichMixer], inRGBRange, kRegMaskVidProcRGBRange, kRegShiftVidProcRGBRange);
}

bool CNTV2Card::GetMixerRGBRange (const UWord inWhichMixer, NTV2MixerRGBRange & outRGBRange)
{
	if (inWhichMixer >= ::NTV2DeviceGetNumMixers(GetDeviceID()))
		return false;
	return !CNTV2DriverInterface::ReadRegister (gIndexToVidProcControlRegNum[inWhichMixer], outRGBRange, kRegMaskVidProcRGBRange, kRegShiftVidProcRGBRange);
}


////////////////////////////////////////////////////////////////////
//  Mapping methods
////////////////////////////////////////////////////////////////////

#if !defined(NTV2_DEPRECATE_16_0)
	// Method:	GetBaseAddress
	// Input:	NTV2Channel channel
	// Output:	bool status and modifies ULWord **pBaseAddress
	bool CNTV2Card::GetBaseAddress (NTV2Channel channel, ULWord **pBaseAddress)
	{
		ULWord ulFrame;
		if (IS_CHANNEL_INVALID (channel))
			return false;
		GetPCIAccessFrame (channel, ulFrame);
		if (ulFrame > GetNumFrameBuffers())
			ulFrame = 0;
	
		if (::NTV2DeviceIsDirectAddressable(GetDeviceID()))
		{
			if (!_pFrameBaseAddress)
				if (!MapFrameBuffers())
					return false;
			*pBaseAddress = _pFrameBaseAddress + ((ulFrame * _ulFrameBufferSize) / sizeof(ULWord));
		}
		else	// must be an _MM board
		{
			if (!_pCh1FrameBaseAddress)
				if (!MapFrameBuffers())
					return false;
			*pBaseAddress = (channel == NTV2_CHANNEL1) ? _pCh1FrameBaseAddress : _pCh2FrameBaseAddress;	//	DEPRECATE!
		}
		return true;
	}

	// Method:	GetBaseAddress
	// Input:	None
	// Output:	bool status and modifies ULWord *pBaseAddress
	bool CNTV2Card::GetBaseAddress (ULWord **pBaseAddress)
	{
		if (!_pFrameBaseAddress)
			if (!MapFrameBuffers())
				return false;
		*pBaseAddress = _pFrameBaseAddress;
		return true;
	}

	// Method:	GetRegisterBaseAddress
	// Input:	ULWord regNumber
	// Output:	bool status and modifies ULWord **pBaseAddress
	bool CNTV2Card::GetRegisterBaseAddress (ULWord regNumber, ULWord **pBaseAddress)
	{
		if (!_pRegisterBaseAddress)
			if (!MapRegisters())
				return false;
		#ifdef MSWindows
		if ((regNumber*4) >= _pRegisterBaseAddressLength)
			return false;
		#endif	//	MSWindows
		*pBaseAddress = _pRegisterBaseAddress + regNumber;
		return true;
	}

	// Method:	GetXena2FlashBaseAddress
	// Output:	bool status and modifies ULWord **pXena2FlashAddress
	bool CNTV2Card::GetXena2FlashBaseAddress (ULWord **pXena2FlashAddress)
	{
		if (!_pXena2FlashBaseAddress)
			if (!MapXena2Flash())
				return false;
		*pXena2FlashAddress = _pXena2FlashBaseAddress;
		return true;
	}
#endif	//	!defined(NTV2_DEPRECATE_16_0)

bool CNTV2Card::SetDualLinkOutputEnable (const bool enable)
{
	return WriteRegister (kRegGlobalControl,  enable ? 1 : 0,  kRegMaskDualLinkOutEnable,  kRegShiftDualLinKOutput);
}

bool CNTV2Card::GetDualLinkOutputEnable (bool & outIsEnabled)
{
	outIsEnabled = false;
	return CNTV2DriverInterface::ReadRegister (kRegGlobalControl, outIsEnabled, kRegMaskDualLinkOutEnable,  kRegShiftDualLinKOutput);
}


bool CNTV2Card::SetDualLinkInputEnable (const bool enable)
{
	return WriteRegister (kRegGlobalControl,  enable ? 1 : 0,  kRegMaskDualLinkInEnable,  kRegShiftDualLinkInput);
}


bool CNTV2Card::GetDualLinkInputEnable (bool & outIsEnabled)
{
	outIsEnabled = false;
	return CNTV2DriverInterface::ReadRegister (kRegGlobalControl,  outIsEnabled,  kRegMaskDualLinkInEnable,  kRegShiftDualLinkInput);
}


///////////////////////////////////////////////////////////////////

bool CNTV2Card::SetDitherFor8BitInputs (const NTV2Channel inChannel, const ULWord inDither)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return WriteRegister (gChannelToControlRegNum[inChannel], inDither, kRegMaskDitherOn8BitInput, kRegShiftDitherOn8BitInput);
}

bool CNTV2Card::GetDitherFor8BitInputs (const NTV2Channel inChannel, ULWord & outDither)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return ReadRegister(gChannelToControlRegNum[inChannel], outDither, kRegMaskDitherOn8BitInput, kRegShiftDitherOn8BitInput);
}

///////////////////////////////////////////////////////////////////

bool CNTV2Card::SetForce64(ULWord force64)										{return WriteRegister (kRegDMAControl,	force64,	kRegMaskForce64,	kRegShiftForce64);}
bool CNTV2Card::GetForce64(ULWord* force64)										{return force64 ? ReadRegister (kRegDMAControl,	*force64, kRegMaskForce64, kRegShiftForce64) : false;}
bool CNTV2Card::Get64BitAutodetect(ULWord* autodetect64)						{return autodetect64 ? ReadRegister (kRegDMAControl, *autodetect64, kRegMaskAutodetect64, kRegShiftAutodetect64) : false;}
#if !defined(NTV2_DEPRECATE_14_2)
	bool CNTV2Card::GetFirmwareRev (ULWord * pOutRevisionNumber)
	{
		UWord	revNum(0);
		if (!pOutRevisionNumber  ||  !GetRunningFirmwareRevision(revNum))
			return false;
		*pOutRevisionNumber = ULWord(revNum);
		return true;
	}
#endif	//	NTV2_DEPRECATE_14_2


/////////////////////////////////////////////////////////
// Kona2/Xena2/ related methods

// kK2RegAnalogOutControl
bool CNTV2Card::SetVideoDACMode (NTV2VideoDACMode value)						{return WriteRegister (kRegAnalogOutControl,	value,	kK2RegMaskVideoDACMode,	kK2RegShiftVideoDACMode);}
bool CNTV2Card::GetVideoDACMode (NTV2VideoDACMode & outValue)					{return CNTV2DriverInterface::ReadRegister (kRegAnalogOutControl, outValue,	kK2RegMaskVideoDACMode,	kK2RegShiftVideoDACMode);}
bool CNTV2Card::SetAnalogOutHTiming (ULWord value)								{return WriteRegister (kRegAnalogOutControl,	value,	kK2RegMaskOutHTiming,	kK2RegShiftOutHTiming);}
bool CNTV2Card::GetAnalogOutHTiming (ULWord & outValue)							{return ReadRegister (kRegAnalogOutControl, outValue,	kK2RegMaskOutHTiming,	kK2RegShiftOutHTiming);}

bool CNTV2Card::SetSDIOutputStandard (const UWord inOutputSpigot, const NTV2Standard inValue)
{
	if (IS_OUTPUT_SPIGOT_INVALID(inOutputSpigot))
		return false;

	bool is2Kx1080(false);
	switch(inValue)
	{
		case NTV2_STANDARD_2Kx1080p:
		case NTV2_STANDARD_2Kx1080i:
		case NTV2_STANDARD_4096x2160p:
		case NTV2_STANDARD_4096HFR:
		case NTV2_STANDARD_4096i:
			is2Kx1080 = true;
			break;
		default:
			break;
	}

	return WriteRegister (gChannelToSDIOutControlRegNum[inOutputSpigot], inValue, kK2RegMaskSDIOutStandard, kK2RegShiftSDIOutStandard)
			&&  SetSDIOut2Kx1080Enable(NTV2Channel(inOutputSpigot), is2Kx1080);
}

bool CNTV2Card::SetSDIOutputStandard (const NTV2ChannelSet & inSDIOutputs, const NTV2Standard inValue)
{
	size_t errors(0);
	for (NTV2ChannelSetConstIter it(inSDIOutputs.begin());  it != inSDIOutputs.end();  ++it)
		if (!SetSDIOutputStandard(*it, inValue))
			errors++;
	return !errors;
}

bool CNTV2Card::GetSDIOutputStandard (const UWord inOutputSpigot, NTV2Standard & outValue)
{
	if (IS_OUTPUT_SPIGOT_INVALID(inOutputSpigot))
		return false;
	bool is2kx1080(false);
	NTV2Standard newStd(NTV2_STANDARD_INVALID);
	const bool result (CNTV2DriverInterface::ReadRegister (gChannelToSDIOutControlRegNum[inOutputSpigot], newStd, kK2RegMaskSDIOutStandard, kK2RegShiftSDIOutStandard)
						&& GetSDIOut2Kx1080Enable(NTV2Channel(inOutputSpigot), is2kx1080));
	outValue = newStd;
	switch (newStd)
	{
		case NTV2_STANDARD_1080:	if (is2kx1080)
										outValue = NTV2_STANDARD_2Kx1080i;
									break;
		case NTV2_STANDARD_1080p:	if (is2kx1080)
										outValue = NTV2_STANDARD_2Kx1080p;
									break;
		default:
			break;
	}
	return result;
}

#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::SetSDIOutStandard (NTV2Standard value, NTV2Channel channel)		{return SetSDIOutputStandard (channel, value);}
	bool CNTV2Card::GetSDIOutStandard (NTV2Standard* pValue, NTV2Channel channel)	{return pValue ? GetSDIOutputStandard (channel, *pValue) : false;}
	bool CNTV2Card::GetSDIOutStandard (NTV2Standard& outValue, NTV2Channel channel)	{return GetSDIOutputStandard (channel, outValue);}
	bool CNTV2Card::SetK2SDI1OutStandard (NTV2Standard value)						{return SetSDIOutStandard (value, NTV2_CHANNEL1);}
	bool CNTV2Card::GetK2SDI1OutStandard (NTV2Standard* value)						{return GetSDIOutStandard (value, NTV2_CHANNEL1);}
	bool CNTV2Card::SetK2SDI2OutStandard (NTV2Standard value)						{return SetSDIOutStandard (value, NTV2_CHANNEL2);}
	bool CNTV2Card::GetK2SDI2OutStandard (NTV2Standard* value)						{return GetSDIOutStandard (value, NTV2_CHANNEL2);}
	bool CNTV2Card::SetK2SDI3OutStandard (NTV2Standard value)						{return SetSDIOutStandard (value, NTV2_CHANNEL3);}
	bool CNTV2Card::GetK2SDI3OutStandard (NTV2Standard* value)						{return GetSDIOutStandard (value, NTV2_CHANNEL3);}
	bool CNTV2Card::SetK2SDI4OutStandard (NTV2Standard value)						{return SetSDIOutStandard (value, NTV2_CHANNEL4);}
	bool CNTV2Card::GetK2SDI4OutStandard (NTV2Standard* value)						{return GetSDIOutStandard (value, NTV2_CHANNEL4);}
	bool CNTV2Card::SetK2SDI5OutStandard (NTV2Standard value)						{return SetSDIOutStandard (value, NTV2_CHANNEL5);}
	bool CNTV2Card::GetK2SDI5OutStandard (NTV2Standard* value)						{return GetSDIOutStandard (value, NTV2_CHANNEL5);}
	bool CNTV2Card::SetK2SDI6OutStandard (NTV2Standard value)						{return SetSDIOutStandard (value, NTV2_CHANNEL6);}
	bool CNTV2Card::GetK2SDI6OutStandard (NTV2Standard* value)						{return GetSDIOutStandard (value, NTV2_CHANNEL6);}
	bool CNTV2Card::SetK2SDI7OutStandard (NTV2Standard value)						{return SetSDIOutStandard (value, NTV2_CHANNEL7);}
	bool CNTV2Card::GetK2SDI7OutStandard (NTV2Standard* value)						{return GetSDIOutStandard (value, NTV2_CHANNEL7);}
	bool CNTV2Card::SetK2SDI8OutStandard (NTV2Standard value)						{return SetSDIOutStandard (value, NTV2_CHANNEL8);}
	bool CNTV2Card::GetK2SDI8OutStandard (NTV2Standard* value)						{return GetSDIOutStandard (value, NTV2_CHANNEL8);}
#endif	//	!defined (NTV2_DEPRECATE)


bool CNTV2Card::WriteOutputTimingControl (const ULWord inValue, const UWord inOutputSpigot)
{
	if (IS_OUTPUT_SPIGOT_INVALID(inOutputSpigot))
		return false;
	if (IsMultiFormatActive())
		return WriteRegister (gChannelToOutputTimingCtrlRegNum[inOutputSpigot], inValue);
	else if (::NTV2DeviceCanDoMultiFormat(GetDeviceID()))
	{
		//	Write all of the timing registers for UniFormat mode...
		switch (::NTV2DeviceGetNumVideoChannels(GetDeviceID()))
		{
			case 8:
				WriteRegister (gChannelToOutputTimingCtrlRegNum[NTV2_CHANNEL8], inValue);
				WriteRegister (gChannelToOutputTimingCtrlRegNum[NTV2_CHANNEL7], inValue);
				WriteRegister (gChannelToOutputTimingCtrlRegNum[NTV2_CHANNEL6], inValue);
				WriteRegister (gChannelToOutputTimingCtrlRegNum[NTV2_CHANNEL5], inValue);
				AJA_FALL_THRU;
			case 4:
				WriteRegister (gChannelToOutputTimingCtrlRegNum[NTV2_CHANNEL4], inValue);
				WriteRegister (gChannelToOutputTimingCtrlRegNum[NTV2_CHANNEL3], inValue);
				AJA_FALL_THRU;
			case 2:
				WriteRegister (gChannelToOutputTimingCtrlRegNum[NTV2_CHANNEL2], inValue);
				AJA_FALL_THRU;
			default:
				return WriteRegister (gChannelToOutputTimingCtrlRegNum [NTV2_CHANNEL1], inValue);
		}
	}
	else
		return WriteRegister (gChannelToOutputTimingCtrlRegNum [NTV2_CHANNEL1], inValue);
}


bool CNTV2Card::ReadOutputTimingControl (ULWord & outValue, const UWord inOutputSpigot)
{
	if (IS_OUTPUT_SPIGOT_INVALID (inOutputSpigot))
		return false;
	return ReadRegister (gChannelToOutputTimingCtrlRegNum[IsMultiFormatActive() ? inOutputSpigot : UWord(NTV2_CHANNEL1)], outValue);
}

// SDI1 HTiming
bool CNTV2Card::SetSDI1OutHTiming (ULWord value)		{return WriteRegister (kRegSDIOut1Control,	value,			kK2RegMaskOutHTiming,	kK2RegShiftOutHTiming);}
bool CNTV2Card::GetSDI1OutHTiming(ULWord* pValue)		{return pValue ? ReadRegister(kRegSDIOut1Control, *pValue, kK2RegMaskOutHTiming, kK2RegShiftOutHTiming) : false;}

// SDI2 HTiming
bool CNTV2Card::SetSDI2OutHTiming (ULWord value)		{return WriteRegister (kRegSDIOut2Control,	value,	kK2RegMaskOutHTiming,	kK2RegShiftOutHTiming);}
bool CNTV2Card::GetSDI2OutHTiming(ULWord* pValue)		{return pValue ? ReadRegister(kRegSDIOut2Control, *pValue, kK2RegMaskOutHTiming, kK2RegShiftOutHTiming) : false;}


bool CNTV2Card::SetSDIOutVPID (const ULWord inValueA, const ULWord inValueB, const UWord inOutputSpigot)
{
	if (IS_OUTPUT_SPIGOT_INVALID (inOutputSpigot))
		return false;
	if (inValueA)
	{
		if (!WriteRegister (gChannelToSDIOutVPIDARegNum [inOutputSpigot], inValueA)) return false;
		if (!WriteRegister (gChannelToSDIOutVPIDBRegNum [inOutputSpigot], inValueB)) return false;
		if (!WriteRegister (gChannelToSDIOutControlRegNum [inOutputSpigot], 1, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite))
			return false;
		if (!WriteRegister (gChannelToSDIOutControlRegNum [inOutputSpigot], 1, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable))
			return false;
		return true;
	}

	if (!WriteRegister (gChannelToSDIOutControlRegNum [inOutputSpigot], 0, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite))
		return false;
	if (!WriteRegister (gChannelToSDIOutControlRegNum [inOutputSpigot], 0, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable))
		return false;
	if (!WriteRegister (gChannelToSDIOutVPIDARegNum [inOutputSpigot], 0))
		return false;
	if (!WriteRegister (gChannelToSDIOutVPIDBRegNum [inOutputSpigot], 0))
		return false;
	return true;

}	//	SetSDIOutVPID


bool CNTV2Card::GetSDIOutVPID (ULWord & outValueA, ULWord & outValueB, const UWord inOutputSpigot)
{
	if (IS_OUTPUT_SPIGOT_INVALID(inOutputSpigot))
		return false;

	return ReadRegister(gChannelToSDIOutVPIDARegNum[inOutputSpigot], outValueA)
			&&  ReadRegister(gChannelToSDIOutVPIDBRegNum[inOutputSpigot], outValueB);

}	//	GetSDIOutVPID


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::SetK2SDI1OutVPID (ULWord valueA, ULWord valueB)	{return SetSDIOutVPID (valueA, valueB, NTV2_CHANNEL1);}
	bool CNTV2Card::SetK2SDI2OutVPID (ULWord valueA, ULWord valueB)	{return SetSDIOutVPID (valueA, valueB, NTV2_CHANNEL2);}
	bool CNTV2Card::SetK2SDI3OutVPID (ULWord valueA, ULWord valueB)	{return SetSDIOutVPID (valueA, valueB, NTV2_CHANNEL3);}
	bool CNTV2Card::SetK2SDI4OutVPID (ULWord valueA, ULWord valueB)	{return SetSDIOutVPID (valueA, valueB, NTV2_CHANNEL4);}
	bool CNTV2Card::SetK2SDI5OutVPID (ULWord valueA, ULWord valueB)	{return SetSDIOutVPID (valueA, valueB, NTV2_CHANNEL5);}
	bool CNTV2Card::SetK2SDI6OutVPID (ULWord valueA, ULWord valueB)	{return SetSDIOutVPID (valueA, valueB, NTV2_CHANNEL6);}
	bool CNTV2Card::SetK2SDI7OutVPID (ULWord valueA, ULWord valueB)	{return SetSDIOutVPID (valueA, valueB, NTV2_CHANNEL7);}
	bool CNTV2Card::SetK2SDI8OutVPID (ULWord valueA, ULWord valueB)	{return SetSDIOutVPID (valueA, valueB, NTV2_CHANNEL8);}

	bool CNTV2Card::GetK2SDI1OutVPID (ULWord* valueA, ULWord* valueB)	{ULWord	a (0), b (0);	return GetSDIOutVPID (valueA ? *valueA : a, valueB ? *valueB : b, NTV2_CHANNEL1);}
	bool CNTV2Card::GetK2SDI2OutVPID (ULWord* valueA, ULWord* valueB)
	{
		if (valueA)
		{
			if (!ReadRegister (kRegSDIOut2VPIDA, (ULWord*) valueA))
				return false;
		}
		if(valueB)
		{
			if (!ReadRegister (kRegSDIOut2VPIDB, (ULWord*) valueB))
				return false;
		}
		return true;
	}
	bool CNTV2Card::GetK2SDI3OutVPID(ULWord* valueA, ULWord* valueB)	{ULWord	a (0), b (0);	return GetSDIOutVPID (valueA ? *valueA : a, valueB ? *valueB : b, NTV2_CHANNEL3);}
	bool CNTV2Card::GetK2SDI4OutVPID(ULWord* valueA, ULWord* valueB)	{ULWord	a (0), b (0);	return GetSDIOutVPID (valueA ? *valueA : a, valueB ? *valueB : b, NTV2_CHANNEL4);}
	bool CNTV2Card::GetK2SDI5OutVPID(ULWord* valueA, ULWord* valueB)	{ULWord	a (0), b (0);	return GetSDIOutVPID (valueA ? *valueA : a, valueB ? *valueB : b, NTV2_CHANNEL5);}
	bool CNTV2Card::GetK2SDI6OutVPID(ULWord* valueA, ULWord* valueB)	{ULWord	a (0), b (0);	return GetSDIOutVPID (valueA ? *valueA : a, valueB ? *valueB : b, NTV2_CHANNEL6);}
	bool CNTV2Card::GetK2SDI7OutVPID(ULWord* valueA, ULWord* valueB)	{ULWord	a (0), b (0);	return GetSDIOutVPID (valueA ? *valueA : a, valueB ? *valueB : b, NTV2_CHANNEL7);}
	bool CNTV2Card::GetK2SDI8OutVPID(ULWord* valueA, ULWord* valueB)	{ULWord	a (0), b (0);	return GetSDIOutVPID (valueA ? *valueA : a, valueB ? *valueB : b, NTV2_CHANNEL8);}
#endif	//	!defined (NTV2_DEPRECATE)


// kRegConversionControl
bool CNTV2Card::SetUpConvertMode (const NTV2UpConvertMode inValue)		{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskUpConvertMode,		kK2RegShiftUpConvertMode);}
bool CNTV2Card::GetUpConvertMode (NTV2UpConvertMode & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskUpConvertMode,		kK2RegShiftUpConvertMode);}
bool CNTV2Card::SetConverterOutStandard (const NTV2Standard inValue)	{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskConverterOutStandard,	kK2RegShiftConverterOutStandard);}
bool CNTV2Card::GetConverterOutStandard (NTV2Standard & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskConverterOutStandard,	kK2RegShiftConverterOutStandard);}
bool CNTV2Card::SetConverterOutRate (const NTV2FrameRate inValue)		{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskConverterOutRate,		kK2RegShiftConverterOutRate);}
bool CNTV2Card::GetConverterOutRate (NTV2FrameRate & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskConverterOutRate,		kK2RegShiftConverterOutRate);}
bool CNTV2Card::SetConverterInStandard (const NTV2Standard inValue)		{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskConverterInStandard,	kK2RegShiftConverterInStandard);}
bool CNTV2Card::GetConverterInStandard (NTV2Standard & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskConverterInStandard,	kK2RegShiftConverterInStandard);}
bool CNTV2Card::SetConverterInRate (const NTV2FrameRate inValue)		{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskConverterInRate,		kK2RegShiftConverterInRate);}
bool CNTV2Card::GetConverterInRate (NTV2FrameRate & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskConverterInRate,		kK2RegShiftConverterInRate);}
bool CNTV2Card::SetDownConvertMode (const NTV2DownConvertMode inValue)	{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskDownConvertMode,		kK2RegShiftDownConvertMode);}
bool CNTV2Card::GetDownConvertMode (NTV2DownConvertMode & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskDownConvertMode,		kK2RegShiftDownConvertMode);}
bool CNTV2Card::SetIsoConvertMode (const NTV2IsoConvertMode inValue)	{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskIsoConvertMode,		kK2RegShiftIsoConvertMode);}
bool CNTV2Card::SetEnableConverter (const bool inValue)					{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskEnableConverter,		kK2RegShiftEnableConverter);}

bool CNTV2Card::GetEnableConverter (bool & outValue)
{
	ULWord ULWvalue = ULWord(outValue);
	bool retVal = ReadRegister (kRegConversionControl, ULWvalue, kK2RegMaskEnableConverter, kK2RegShiftEnableConverter);
	outValue = ULWvalue ? true : false;
	return retVal;
}

bool CNTV2Card::GetIsoConvertMode (NTV2IsoConvertMode & outValue)				{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,	kK2RegMaskIsoConvertMode,		kK2RegShiftIsoConvertMode);}
bool CNTV2Card::SetDeinterlaceMode (const ULWord inValue)						{return WriteRegister (kRegConversionControl,	ULWord(inValue),	kK2RegMaskDeinterlaceMode,		kK2RegShiftDeinterlaceMode);}
bool CNTV2Card::GetDeinterlaceMode (ULWord & outValue)							{return ReadRegister (kRegConversionControl,	outValue,			kK2RegMaskDeinterlaceMode,		kK2RegShiftDeinterlaceMode);}
bool CNTV2Card::SetConverterPulldown (const ULWord inValue)						{return WriteRegister (kRegConversionControl,	ULWord(inValue),	kK2RegMaskConverterPulldown,	kK2RegShiftConverterPulldown);}
bool CNTV2Card::GetConverterPulldown (ULWord & outValue)						{return ReadRegister (kRegConversionControl,	outValue,			kK2RegMaskConverterPulldown,	kK2RegShiftConverterPulldown);}
bool CNTV2Card::SetUCPassLine21 (const ULWord inValue)							{return WriteRegister (kRegConversionControl,	ULWord(inValue),	kK2RegMaskUCPassLine21,			kK2RegShiftUCPassLine21);}
bool CNTV2Card::GetUCPassLine21 (ULWord & outValue)								{return ReadRegister (kRegConversionControl,	outValue,			kK2RegMaskUCPassLine21,			kK2RegShiftUCPassLine21);}
bool CNTV2Card::SetUCAutoLine21 (const ULWord inValue)							{return WriteRegister (kRegConversionControl,	ULWord(inValue),	kK2RegMaskUCPassLine21,			kK2RegShiftUCAutoLine21);}
bool CNTV2Card::GetUCAutoLine21 (ULWord & outValue)								{return ReadRegister (kRegConversionControl,	outValue,			kK2RegMaskUCPassLine21,			kK2RegShiftUCAutoLine21);}

#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::SetSecondConverterInStandard (const NTV2Standard inValue)		{return WriteRegister (kRegFS1DownConverter2Control,	ULWord(inValue),	kK2RegMaskConverterInStandard,	kK2RegShiftConverterInStandard);}
	bool CNTV2Card::GetSecondConverterInStandard (NTV2Standard & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegFS1DownConverter2Control,	outValue,		kK2RegMaskConverterInStandard,	kK2RegShiftConverterInStandard);}
	bool CNTV2Card::SetSecondDownConvertMode (const NTV2DownConvertMode inValue)	{return WriteRegister (kRegFS1DownConverter2Control,	ULWord(inValue),	kK2RegMaskDownConvertMode,		kK2RegShiftDownConvertMode);}
	bool CNTV2Card::GetSecondDownConvertMode (NTV2DownConvertMode & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegFS1DownConverter2Control,	outValue,		kK2RegMaskDownConvertMode,		kK2RegShiftDownConvertMode);}
	bool CNTV2Card::SetSecondConverterOutStandard (const NTV2Standard inValue)		{return WriteRegister (kRegFS1DownConverter2Control,	ULWord(inValue),	kK2RegMaskConverterOutStandard,	kK2RegShiftConverterOutStandard);}
	bool CNTV2Card::GetSecondConverterOutStandard (NTV2Standard & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegFS1DownConverter2Control,	outValue,		kK2RegMaskConverterOutStandard,	kK2RegShiftConverterOutStandard);}
	bool CNTV2Card::SetSecondIsoConvertMode (const NTV2IsoConvertMode inValue)		{return WriteRegister (kRegFS1DownConverter2Control,	ULWord(inValue),	kK2RegMaskIsoConvertMode,		kK2RegShiftIsoConvertMode);}
	bool CNTV2Card::GetSecondIsoConvertMode (NTV2IsoConvertMode & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegFS1DownConverter2Control,	outValue,		kK2RegMaskIsoConvertMode,		kK2RegShiftIsoConvertMode);}
	bool CNTV2Card::SetSecondConverterPulldown (const ULWord inValue)				{return WriteRegister (kRegFS1DownConverter2Control,	ULWord(inValue),	kK2RegMaskConverterPulldown,	kK2RegShiftConverterPulldown);}
	bool CNTV2Card::GetSecondConverterPulldown (ULWord & outValue)					{return ReadRegister (kRegFS1DownConverter2Control,	outValue,				kK2RegMaskConverterPulldown,	kK2RegShiftConverterPulldown);}

	// kK2RegFrameSync1Control & kK2RegFrameSync2Control
	bool CNTV2Card::SetK2FrameSyncControlFrameDelay (NTV2FrameSyncSelect select, ULWord value)
	{
		return WriteRegister (select == NTV2_FrameSync1Select ? kK2RegFrameSync1Control : kK2RegFrameSync2Control, value, kK2RegMaskFrameSyncControlFrameDelay, kK2RegShiftFrameSyncControlFrameDelay);
	}

	bool CNTV2Card::GetK2FrameSyncControlFrameDelay (NTV2FrameSyncSelect select, ULWord *value)
	{
		return ReadRegister (select == NTV2_FrameSync1Select ? kK2RegFrameSync1Control : kK2RegFrameSync2Control, (ULWord*)value, kK2RegMaskFrameSyncControlFrameDelay, kK2RegShiftFrameSyncControlFrameDelay);
	}
		
	bool CNTV2Card::SetK2FrameSyncControlStandard (NTV2FrameSyncSelect select, NTV2Standard value)
	{
		return WriteRegister (select == NTV2_FrameSync1Select ? kK2RegFrameSync1Control : kK2RegFrameSync2Control, value, kK2RegMaskFrameSyncControlStandard, kK2RegShiftFrameSyncControlStandard);
	}

	bool CNTV2Card::GetK2FrameSyncControlStandard (NTV2FrameSyncSelect select, NTV2Standard *value)
	{
		return ReadRegister (select == NTV2_FrameSync1Select ? kK2RegFrameSync1Control : kK2RegFrameSync2Control, (ULWord*)value, kK2RegMaskFrameSyncControlStandard, kK2RegShiftFrameSyncControlStandard);
	}

	bool CNTV2Card::SetK2FrameSyncControlGeometry (NTV2FrameSyncSelect select, NTV2FrameGeometry value)
	{
		return WriteRegister (select == NTV2_FrameSync1Select ? kK2RegFrameSync1Control : kK2RegFrameSync2Control, value, kK2RegMaskFrameSyncControlGeometry, kK2RegShiftFrameSyncControlGeometry);
	}

	bool CNTV2Card::GetK2FrameSyncControlGeometry (NTV2FrameSyncSelect select, NTV2FrameGeometry *value)
	{
		return ReadRegister (select == NTV2_FrameSync1Select ? kK2RegFrameSync1Control : kK2RegFrameSync2Control, (ULWord*)value, kK2RegMaskFrameSyncControlGeometry, kK2RegShiftFrameSyncControlGeometry);
	}

	bool CNTV2Card::SetK2FrameSyncControlFrameFormat (NTV2FrameSyncSelect select, NTV2FrameBufferFormat value)
	{
		return WriteRegister (select == NTV2_FrameSync1Select ? kK2RegFrameSync1Control : kK2RegFrameSync2Control, value, kK2RegMaskFrameSyncControlFrameFormat, kK2RegShiftFrameSyncControlFrameFormat);
	}

	bool CNTV2Card::GetK2FrameSyncControlFrameFormat (NTV2FrameSyncSelect select, NTV2FrameBufferFormat *value)
	{
		return ReadRegister (select == NTV2_FrameSync1Select ? kK2RegFrameSync1Control : kK2RegFrameSync2Control, (ULWord*)value, kK2RegMaskFrameSyncControlFrameFormat, kK2RegShiftFrameSyncControlFrameFormat);
	}


	// Note:  One more Crosspoint lurks (in Register 95) ... FS1 Second Analog Out xpt
	#define XENA2_SEARCHTIMEOUT 5
	//
	//SetXena2VideoOutputStandard
	// Searches crosspoint matrix to determine standard.
	//
	bool CNTV2Card::SetVideoOutputStandard (const NTV2Channel inChannel)
	{
		NTV2Standard			standard		(NTV2_NUM_STANDARDS);
		NTV2VideoFormat			videoFormat		(NTV2_FORMAT_UNKNOWN);
		UWord					searchTimeout	(0);
		NTV2OutputCrosspointID	xptSelect		(NTV2_XptBlack);

		if (IS_CHANNEL_INVALID (inChannel))
			return false;
		switch (inChannel)
		{
			case NTV2_CHANNEL1:		GetXptSDIOut1InputSelect (&xptSelect);		break;
			case NTV2_CHANNEL2:		GetXptSDIOut2InputSelect (&xptSelect);		break;
			case NTV2_CHANNEL3:		GetXptSDIOut3InputSelect (&xptSelect);		break;
			case NTV2_CHANNEL4:		GetXptSDIOut4InputSelect (&xptSelect);		break;
			default:				return false;
		}
		
		do {
			switch (xptSelect)
			{
			case NTV2_XptSDIIn1:
				videoFormat = GetInputVideoFormat (NTV2_INPUTSOURCE_SDI1);
				standard = GetNTV2StandardFromVideoFormat(videoFormat);
				if ( standard == NTV2_NUM_STANDARDS) 
					searchTimeout = XENA2_SEARCHTIMEOUT;
				break;
			case NTV2_XptSDIIn2:
				videoFormat = GetInputVideoFormat (NTV2_INPUTSOURCE_SDI2);
				standard = GetNTV2StandardFromVideoFormat(videoFormat);
				if ( standard == NTV2_NUM_STANDARDS) 
					searchTimeout = XENA2_SEARCHTIMEOUT;
				break;
			case NTV2_XptSDIIn3:
				videoFormat = GetInputVideoFormat (NTV2_INPUTSOURCE_SDI3);
				standard = GetNTV2StandardFromVideoFormat(videoFormat);
				if ( standard == NTV2_NUM_STANDARDS) 
					searchTimeout = XENA2_SEARCHTIMEOUT;
				break;
			case NTV2_XptSDIIn4:
				videoFormat = GetInputVideoFormat (NTV2_INPUTSOURCE_SDI4);
				standard = GetNTV2StandardFromVideoFormat(videoFormat);
				if ( standard == NTV2_NUM_STANDARDS) 
					searchTimeout = XENA2_SEARCHTIMEOUT;
				break;
			case NTV2_XptFrameBuffer1YUV:
			case NTV2_XptFrameBuffer1RGB:
			case NTV2_XptFrameBuffer2YUV:
			case NTV2_XptFrameBuffer2RGB:
			case NTV2_XptFrameBuffer3YUV:
			case NTV2_XptFrameBuffer3RGB:
			case NTV2_XptFrameBuffer4YUV:
			case NTV2_XptFrameBuffer4RGB:
			case NTV2_XptBlack:
				GetVideoFormat(&videoFormat);
				standard = GetNTV2StandardFromVideoFormat(videoFormat);
				break;
			case NTV2_XptDuallinkIn1:
				videoFormat = GetInputVideoFormat (NTV2_INPUTSOURCE_SDI1);
				standard = GetNTV2StandardFromVideoFormat(videoFormat);
				if ( standard == NTV2_NUM_STANDARDS) 
					searchTimeout = XENA2_SEARCHTIMEOUT;
				break;
			case NTV2_XptConversionModule:
				GetConverterOutStandard(&standard);
				if ( standard >= NTV2_NUM_STANDARDS)
				{
					standard = NTV2_NUM_STANDARDS;
					searchTimeout = XENA2_SEARCHTIMEOUT;
				}
				break;
	// keep looking
			case NTV2_XptCSC1VidRGB:
			case NTV2_XptCSC1VidYUV:
			case NTV2_XptCSC1KeyYUV:
				GetXptColorSpaceConverterInputSelect(&xptSelect);
				break;
			case NTV2_XptCompressionModule:
				GetXptCompressionModInputSelect(&xptSelect);
				break;
			case NTV2_XptDuallinkOut1:
				GetXptDuallinkOutInputSelect(&xptSelect);
				break;
			case NTV2_XptLUT1Out:
				GetXptLUTInputSelect(&xptSelect);
				break;
			case NTV2_XptLUT2Out:
				GetXptLUT2InputSelect(&xptSelect);
				break;
			case NTV2_XptLUT3Out:
				GetXptLUT3InputSelect(&xptSelect);
				break;
			case NTV2_XptLUT4Out:
				GetXptLUT4InputSelect(&xptSelect);
				break;
			case NTV2_XptCSC2VidYUV:
			case NTV2_XptCSC2VidRGB:
			case NTV2_XptCSC2KeyYUV:
				GetXptCSC2VidInputSelect(&xptSelect);
				break;
			case NTV2_XptMixer1VidYUV:
			case NTV2_XptMixer1KeyYUV:
				GetXptMixer1FGVidInputSelect(&xptSelect);
				break;
			case NTV2_XptAlphaOut:
			default:
				searchTimeout = XENA2_SEARCHTIMEOUT;
				break;

			}

		} while (!NTV2_IS_VALID_STANDARD (standard)  &&  searchTimeout++ < XENA2_SEARCHTIMEOUT);

		return NTV2_IS_VALID_STANDARD (standard) ? SetSDIOutputStandard (inChannel, standard) : false;

	}	//	SetVideoOutputStandard

#endif	//	!defined (NTV2_DEPRECATE)


bool CNTV2Card::SetSecondaryVideoFormat(NTV2VideoFormat format)
{
	bool bResult = WriteRegister(kVRegSecondaryFormatSelect, format);
	
	#if !defined (NTV2_DEPRECATE)
		// If switching from high def to standard def or vice versa some
		// boards may need to have a different bitfile loaded.
		CheckBitfile ();
	#endif	//	!defined (NTV2_DEPRECATE)

	return bResult;
}

bool CNTV2Card::GetSecondaryVideoFormat(NTV2VideoFormat & outFormat)
{
	return CNTV2DriverInterface::ReadRegister (kVRegSecondaryFormatSelect, outFormat);
}


#if !defined(R2_DEPRECATE)

bool CNTV2Card::SetInputVideoSelect (NTV2InputVideoSelect input)
{
	bool bResult = WriteRegister(kVRegInputSelect, input);
	
	#if !defined (NTV2_DEPRECATE)
		// switching inputs may trigger a change in up/down converter mode
		CheckBitfile ();
	#endif	//	!defined (NTV2_DEPRECATE)

	return bResult;
}

bool CNTV2Card::GetInputVideoSelect(NTV2InputVideoSelect & outInputSelect)
{
	return CNTV2DriverInterface::ReadRegister(kVRegInputSelect, outInputSelect);
}

#endif // R2_DEPRECATE


NTV2VideoFormat CNTV2Card::GetInputVideoFormat (NTV2InputSource inSource, const bool inIsProgressivePicture)
{
	switch (inSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		return GetSDIInputVideoFormat (NTV2_CHANNEL1, inIsProgressivePicture);
		case NTV2_INPUTSOURCE_SDI2:		return GetSDIInputVideoFormat (NTV2_CHANNEL2, inIsProgressivePicture);
		case NTV2_INPUTSOURCE_SDI3:		return GetSDIInputVideoFormat (NTV2_CHANNEL3, inIsProgressivePicture);
		case NTV2_INPUTSOURCE_SDI4:		return GetSDIInputVideoFormat (NTV2_CHANNEL4, inIsProgressivePicture);
		case NTV2_INPUTSOURCE_SDI5:		return GetSDIInputVideoFormat (NTV2_CHANNEL5, inIsProgressivePicture);
		case NTV2_INPUTSOURCE_SDI6:		return GetSDIInputVideoFormat (NTV2_CHANNEL6, inIsProgressivePicture);
		case NTV2_INPUTSOURCE_SDI7:		return GetSDIInputVideoFormat (NTV2_CHANNEL7, inIsProgressivePicture);
		case NTV2_INPUTSOURCE_SDI8:		return GetSDIInputVideoFormat (NTV2_CHANNEL8, inIsProgressivePicture);
		case NTV2_INPUTSOURCE_HDMI1:	return GetHDMIInputVideoFormat (NTV2_CHANNEL1);
		case NTV2_INPUTSOURCE_HDMI2:	return GetHDMIInputVideoFormat (NTV2_CHANNEL2);
		case NTV2_INPUTSOURCE_HDMI3:	return GetHDMIInputVideoFormat (NTV2_CHANNEL3);
		case NTV2_INPUTSOURCE_HDMI4:	return GetHDMIInputVideoFormat (NTV2_CHANNEL4);
		case NTV2_INPUTSOURCE_ANALOG1:	return GetAnalogInputVideoFormat ();
		default:						return NTV2_FORMAT_UNKNOWN;
	}
}

NTV2VideoFormat CNTV2Card::GetSDIInputVideoFormat (NTV2Channel inChannel, bool inIsProgressivePicture)
{
	ULWord vpidDS1(0), vpidDS2(0);
	CNTV2VPID inputVPID;
	if (IS_CHANNEL_INVALID(inChannel))
		return NTV2_FORMAT_UNKNOWN;

	bool isValidVPID (GetVPIDValidA(inChannel));
	if (isValidVPID)
	{
		ReadSDIInVPID(inChannel, vpidDS1, vpidDS2);
		inputVPID.SetVPID(vpidDS1);
		isValidVPID = inputVPID.IsValid();
	}

	NTV2FrameRate inputRate(GetSDIInputRate(inChannel));
	NTV2FrameGeometry inputGeometry(GetSDIInputGeometry(inChannel));
	bool isProgressiveTrans (isValidVPID ? inputVPID.GetProgressiveTransport() : GetSDIInputIsProgressive(inChannel));
	bool isProgressivePic (isValidVPID ? inputVPID.GetProgressivePicture() : inIsProgressivePicture);
	bool isInput3G (false);
	
	if(inputRate == NTV2_FRAMERATE_INVALID)
		return NTV2_FORMAT_UNKNOWN;
	
	if (::NTV2DeviceCanDo3GIn(_boardID, inChannel) || ::NTV2DeviceCanDo12GIn(_boardID, inChannel))
	{
		GetSDIInput3GPresent(isInput3G, inChannel);
		NTV2VideoFormat format = isValidVPID ? inputVPID.GetVideoFormat() : GetNTV2VideoFormat(inputRate, inputGeometry, isProgressiveTrans, isInput3G, isProgressivePic);
		if (isValidVPID && format == NTV2_FORMAT_UNKNOWN)
		{
			//	Something might be incorrect in VPID
			isProgressiveTrans = GetSDIInputIsProgressive(inChannel);
			isProgressivePic = inIsProgressivePicture;
			format = GetNTV2VideoFormat(inputRate, inputGeometry, isProgressiveTrans, isInput3G, isProgressivePic);
		}
		if (::NTV2DeviceCanDo12GIn(_boardID, inChannel) && format != NTV2_FORMAT_UNKNOWN && !isValidVPID)
		{
			bool is6G(false), is12G(false);
			GetSDIInput6GPresent(is6G, inChannel);
			GetSDIInput12GPresent(is12G, inChannel);
			if (is6G || is12G)
			{
				format = GetQuadSizedVideoFormat(format, !NTV2DeviceCanDo12gRouting(GetDeviceID()) ? true : false);
			}
			if (inputVPID.IsStandardMultiLink4320())
			{
				format = GetQuadSizedVideoFormat(format, true);
			}
		}
		return format;
	}

	if (::NTV2DeviceCanDo292In(_boardID, inChannel))
	{
		if (_boardID == DEVICE_ID_KONALHI || _boardID == DEVICE_ID_KONALHIDVI)
		{
			GetSDIInput3GPresent(isInput3G, NTV2_CHANNEL1);
		}
		return GetNTV2VideoFormat(inputRate, inputGeometry, isProgressiveTrans, isInput3G, isProgressivePic);
	}

	return NTV2_FORMAT_UNKNOWN;
}


NTV2VideoFormat CNTV2Card::GetHDMIInputVideoFormat(NTV2Channel inChannel)
{
	NTV2VideoFormat format = NTV2_FORMAT_UNKNOWN;
	ULWord status;
	if (GetHDMIInputStatus(status, inChannel))
	{
		if ( (status & kRegMaskInputStatusLock) != 0 )
		{
			ULWord hdmiVersion = ::NTV2DeviceGetHDMIVersion(GetDeviceID());
			if(hdmiVersion == 1)
			{
				ULWord standard = ((status & kRegMaskInputStatusStd) >> kRegShiftInputStatusStd);
				if(standard == 0x5)	//	NTV2_STANDARD_2K (2048x1556psf) in HDMI is really SXGA!!
				{
					// We return 1080p60 for SXGA format
					return NTV2_FORMAT_1080p_6000_A;
				}
				else
				{
					format = GetNTV2VideoFormat (NTV2FrameRate((status & kRegMaskInputStatusFPS) >> kRegShiftInputStatusFPS),
												NTV2Standard((status & kRegMaskInputStatusStd) >> kRegShiftInputStatusStd),
												false,												// 3G
												0,													// input geometry
												false);												// progressive picture
				}
			}
			else if(hdmiVersion > 1)
			{
				NTV2FrameRate hdmiRate = NTV2FrameRate((status &kRegMaskInputStatusFPS) >> kRegShiftInputStatusFPS);
				NTV2Standard hdmiStandard = NTV2Standard((status & kRegMaskHDMIInV2VideoStd) >> kRegShiftHDMIInV2VideoStd);
				UByte inputGeometry = 0;
				if (hdmiStandard == NTV2_STANDARD_2Kx1080i || hdmiStandard == NTV2_STANDARD_2Kx1080p)
					inputGeometry = 8;
				format = GetNTV2VideoFormat (hdmiRate,	hdmiStandard, false, inputGeometry,	false);
			}
		}
	} 
	return format;
}

NTV2VideoFormat CNTV2Card::GetAnalogInputVideoFormat()
{
	NTV2VideoFormat format = NTV2_FORMAT_UNKNOWN;
	ULWord status;
	if (ReadRegister(kRegAnalogInputStatus, status))
	{
		if ( (status & kRegMaskInputStatusLock) != 0 )
			format =  GetNTV2VideoFormat ( NTV2FrameRate((status & kRegMaskInputStatusFPS) >> kRegShiftInputStatusFPS),
										   NTV2Standard ((status & kRegMaskInputStatusStd) >> kRegShiftInputStatusStd),
										   false,												// 3G
										   0,													// input geometry
										   false);												// progressive picture
	} 
	return format;
}

#if !defined (NTV2_DEPRECATE)
NTV2VideoFormat CNTV2Card::GetInputVideoFormat (int inputNum, bool progressivePicture)
{
	NTV2VideoFormat result = NTV2_FORMAT_UNKNOWN;
	NTV2DeviceID boardID = GetDeviceID();

	switch (inputNum)
	{
	case 0:	
		result = GetInput1VideoFormat(progressivePicture);
		break;

	case 1:	
		if ( boardID == DEVICE_ID_KONALHI || boardID == DEVICE_ID_IOEXPRESS)
			result = GetHDMIInputVideoFormat();
		else if (boardID == DEVICE_ID_KONALHEPLUS)
			result = GetAnalogInputVideoFormat();
		else
			result = GetInput2VideoFormat(progressivePicture);
		break;

	case 2:
		if (boardID == DEVICE_ID_IOXT)
			result = GetHDMIInputVideoFormat();
		else if (boardID == DEVICE_ID_KONALHI || boardID == DEVICE_ID_IOEXPRESS)
			result = GetAnalogInputVideoFormat();
		else if (boardID == DEVICE_ID_KONA3GQUAD || boardID == DEVICE_ID_CORVID24 || boardID == DEVICE_ID_IO4K ||
            boardID == DEVICE_ID_IO4KUFC || boardID == DEVICE_ID_KONA4 || boardID == DEVICE_ID_KONA4UFC || boardID == DEVICE_ID_KONA5 || boardID == DEVICE_ID_KONA5_4X12G)
			result = GetInput3VideoFormat(progressivePicture);
		break;

	case 3:
		if (boardID == DEVICE_ID_KONA3GQUAD || boardID == DEVICE_ID_CORVID24 || boardID == DEVICE_ID_IO4K ||
            boardID == DEVICE_ID_IO4KUFC || boardID == DEVICE_ID_KONA4 || boardID == DEVICE_ID_KONA4UFC || boardID == DEVICE_ID_KONA5 || boardID == DEVICE_ID_KONA5_4X12G)
			result = GetInput4VideoFormat(progressivePicture);
		break;

	case 4:
		result = GetInput5VideoFormat(progressivePicture);
		break;

	case 5:
		result = GetInput6VideoFormat(progressivePicture);
		break;

	case 6:
		result = GetInput7VideoFormat(progressivePicture);
		break;

	case 7:
		result = GetInput8VideoFormat(progressivePicture);
		break;
	}

	return result;
}
#endif

#if !defined (NTV2_DEPRECATE)
NTV2VideoFormat CNTV2Card::GetInput1VideoFormat (bool progressivePicture)
{
	ULWord status (0), threeGStatus (0);
	if (ReadRegister (kRegInputStatus, &status))
	{
		///Now it is really ugly
		if (::NTV2DeviceCanDo3GOut (_boardID, 0) && ReadRegister (kRegSDIInput3GStatus, &threeGStatus))
		{
			return GetNTV2VideoFormat(NTV2FrameRate (((status >> 25) & BIT_3) | (status & 0x7)),	//framerate
				((status >> 27) & BIT_3) | ((status >> 4) & 0x7),				//input geometry
				((status & BIT_7) >> 7),										//progressive transport
				(threeGStatus & BIT_0),											//3G
				progressivePicture);											//progressive picture
		}
		else
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 25) & BIT_3) | (status & 0x7)),	//framerate
				((status >> 27) & BIT_3) | ((status >> 4) & 0x7),			//input geometry
				((status & BIT_7) >> 7),									//progressive transport
				false,														//3G
				progressivePicture);										//progressive picture
		}
	}
	else
		return NTV2_FORMAT_UNKNOWN;
}

NTV2VideoFormat CNTV2Card::GetInput2VideoFormat (bool progressivePicture)
{
	ULWord status (0), threeGStatus (0);
	if (ReadRegister (kRegInputStatus, &status))
	{
		///Now it is really ugly
		if (::NTV2DeviceCanDo3GOut (_boardID, 1) && ReadRegister (kRegSDIInput3GStatus, &threeGStatus))
		{
			//This is a hack, LHI does not have a second input
			if ((_boardID == DEVICE_ID_KONALHI) && ((threeGStatus & kRegMaskSDIIn3GbpsSMPTELevelBMode) >> 1) && (threeGStatus & kRegMaskSDIIn3GbpsMode))
			{
				return GetNTV2VideoFormat (NTV2FrameRate (((status >> 26) & BIT_3) | ((status >> 8) & 0x7)),	//framerate
					((status >> 28) & BIT_3) | ((status >> 12) & 0x7),					//input geometry
					(status & BIT_15) >> 15,											//progressive transport
					(threeGStatus & kRegMaskSDIIn3GbpsMode),							//3G
					progressivePicture);												//progressive picture
			}
			else if (_boardID != DEVICE_ID_KONALHI)
				return GetNTV2VideoFormat (NTV2FrameRate (((status >> 26) & BIT_3) | ((status >> 8) & 0x7)),	//framerate
				((status >> 28) & BIT_3) | ((status >> 12) & 0x7),					//input geometry
				(status & BIT_15) >> 15,											//progressive transport
				(threeGStatus & BIT_8) >> 8,										//3G
				progressivePicture);												//progressive picture
			else
				return NTV2_FORMAT_UNKNOWN;
		}
		else
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 26) & BIT_3) | ((status >> 8) & 0x7)),	//framerate
				((status >> 28) & BIT_3) | ((status >> 12) & 0x7),					//input geometry
				(status & BIT_15) >> 15,											//progressive transport
				false,																//3G
				progressivePicture);												//progressive picture
		}
	}
	else
		return NTV2_FORMAT_UNKNOWN;
}

NTV2VideoFormat CNTV2Card::GetInput3VideoFormat (bool progressivePicture)
{
	ULWord status (0), threeGStatus (0);
	if (ReadRegister (kRegInputStatus2, &status))
	{
		///Now it is really ugly
		if (::NTV2DeviceCanDo3GOut (_boardID, 2) && ReadRegister (kRegSDIInput3GStatus2, &threeGStatus))
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 25) & BIT_3) | (status & 0x7)),	//framerate
				((status >> 27) & BIT_3) | ((status >> 4) & 0x7),			//input geometry
				((status & BIT_7) >> 7),									//progressive transport
				(threeGStatus & BIT_0),										//3G
				progressivePicture);										//progressive picture
		}
		else
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 25) & BIT_3) | (status & 0x7)),	//framerate
				((status >> 27) & BIT_3) | ((status >> 4) & 0x7),			//input geometry
				((status & BIT_7) >> 7),									//progressive transport
				false,														//3G
				progressivePicture);										//progressive picture
		}
	}
	else
		return NTV2_FORMAT_UNKNOWN;
}

NTV2VideoFormat CNTV2Card::GetInput4VideoFormat (bool progressivePicture)
{
	ULWord status (0), threeGStatus (0);
	if (ReadRegister (kRegInputStatus2, &status))
	{
		///Now it is really ugly
		if (::NTV2DeviceCanDo3GOut (_boardID, 3) && ReadRegister (kRegSDIInput3GStatus2, &threeGStatus))
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 26) & BIT_3) | ((status >> 8) & 0x7)),	//framerate
				((status >> 28) & BIT_3) | ((status >> 12) & 0x7),					//input geometry
				(status & BIT_15) >> 15,											//progressive transport
				(threeGStatus & BIT_8) >> 8,										//3G
				progressivePicture);												//progressive picture
		}
		else
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 26) & BIT_3) | ((status >> 8) & 0x7)),	//framerate
				((status >> 28) & BIT_3) | ((status >> 12) & 0x7),					//input geometry
				(status & BIT_15) >> 15,											//progressive transport
				false,																//3G
				progressivePicture);												//progressive picture
		}
	}
	else
		return NTV2_FORMAT_UNKNOWN;
}

NTV2VideoFormat CNTV2Card::GetInput5VideoFormat (bool progressivePicture)
{
	ULWord status (0), threeGStatus (0);
	if (ReadRegister (kRegInput56Status, &status))
	{
		if (ReadRegister (kRegSDI5678Input3GStatus, &threeGStatus))
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 25) & BIT_3) | (status & 0x7)),	//framerate
				((status >> 27) & BIT_3) | ((status >> 4) & 0x7),			//input geometry
				((status & BIT_7) >> 7),									//progressive transport
				(threeGStatus & BIT_0),										//3G
				progressivePicture);										//progressive picture
		}
	}
	return NTV2_FORMAT_UNKNOWN;
}

NTV2VideoFormat CNTV2Card::GetInput6VideoFormat (bool progressivePicture)
{
	ULWord status (0), threeGStatus (0);
	if (ReadRegister (kRegInput56Status, &status))
	{
		if (ReadRegister (kRegSDI5678Input3GStatus, &threeGStatus))
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 26) & BIT_3) | ((status >> 8) & 0x7)),	//framerate
				((status >> 28) & BIT_3) | ((status >> 12) & 0x7),					//input geometry
				(status & BIT_15) >> 15,											//progressive transport
				(threeGStatus & BIT_8) >> 8,										//3G
				progressivePicture);												//progressive picture
		}
	}
	return NTV2_FORMAT_UNKNOWN;
}

NTV2VideoFormat CNTV2Card::GetInput7VideoFormat (bool progressivePicture)
{
	ULWord status (0), threeGStatus (0);
	if (ReadRegister (kRegInput78Status, &status))
	{
		if (ReadRegister (kRegSDI5678Input3GStatus, &threeGStatus))
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 25) & BIT_3) | (status & 0x7)),	//framerate
				((status >> 27) & BIT_3) | ((status >> 4) & 0x7),			//input geometry
				((status & BIT_7) >> 7),									//progressive transport
				((threeGStatus & BIT_16) >> 16),							//3G
				progressivePicture);										//progressive picture
		}
	}
	return NTV2_FORMAT_UNKNOWN;
}

NTV2VideoFormat CNTV2Card::GetInput8VideoFormat (bool progressivePicture)
{
	ULWord status (0), threeGStatus (0);
	if (ReadRegister (kRegInput78Status, &status))
	{
		if (ReadRegister (kRegSDI5678Input3GStatus, &threeGStatus))
		{
			return GetNTV2VideoFormat (NTV2FrameRate (((status >> 26) & BIT_3) | ((status >> 8) & 0x7)),	//framerate
				((status >> 28) & BIT_3) | ((status >> 12) & 0x7),					//input geometry
				(status & BIT_15) >> 15,											//progressive transport
				(threeGStatus & BIT_24) >> 24,										//3G
				progressivePicture);												//progressive picture
		}
	}
	return NTV2_FORMAT_UNKNOWN;
}
#endif


#if !defined (NTV2_DEPRECATE)
	NTV2VideoFormat CNTV2Card::GetFS1AnalogCompositeInputVideoFormat()
	{
		// FS1 sometimes reports wrongly, if Composite Input is selected,
		// and there is no signal on the Composite input, but there is a signal on S-Video.
		// However, in this case the Frames Per Second reported will be '0' (invalid).
		 
		NTV2VideoFormat format = NTV2_FORMAT_UNKNOWN;
		ULWord compositeDetect;
		ULWord Pal;
		ULWord locked = 1;	// true
		ULWord componentLocked;
		ULWord FPS;
		ULWord success;
		success = ReadRegister (kRegAnalogInputStatus,
						(ULWord*)&compositeDetect,
						kRegMaskAnalogCompositeLocked,
						kRegShiftAnalogCompositeLocked);
		if (!success)
			locked = 0;
		
		success = ReadRegister (kRegAnalogInputStatus,
						(ULWord*)&FPS,
						kRegMaskInputStatusFPS,
						kRegShiftInputStatusFPS);
		if (!success)
			locked = 0;
		
		success = ReadRegister (kRegAnalogInputStatus,
						(ULWord*)&componentLocked,
						kRegMaskInputStatusLock,
						kRegShiftInputStatusLock);
		if (!success)
			locked = 0;
		
		{
			ULWord componentFormat;
			success = ReadRegister (kRegAnalogInputStatus,
							(ULWord*)&componentFormat,
							kRegMaskInputStatusStd,
							kRegShiftInputStatusStd);
			if (componentLocked)
				printf("Component Format = %d\n", componentFormat);
		}
		
		if ( (compositeDetect == 0) || (FPS == 0) || (componentLocked == 0) )
			locked = 0;
		
		if (locked)
		{
			if ( ReadRegister (kRegAnalogInputStatus,
							(ULWord*)&Pal,
							kRegMaskAnalogCompositeFormat625,
							kRegShiftAnalogCompositeFormat625) )
			{
				if (Pal)
					format = NTV2_FORMAT_625_5000;
				else
					format = NTV2_FORMAT_525_5994;
			}
		}
		return format;
	}
#endif	//	!defined (NTV2_DEPRECATE)


NTV2VideoFormat CNTV2Card::GetAnalogCompositeInputVideoFormat()
{
	NTV2VideoFormat format = NTV2_FORMAT_UNKNOWN;
	ULWord analogDetect;
	ULWord locked;
	ULWord Pal;
	// Use a single (atomic) read... so we don't return a bogus value if the register is changing while we're in here!
	if (ReadRegister(kRegAnalogInputStatus, analogDetect))
	{
		locked = (analogDetect & kRegMaskAnalogCompositeLocked) >> kRegShiftAnalogCompositeLocked;
		if (locked)
		{
			Pal = (analogDetect & kRegMaskAnalogCompositeFormat625) >> kRegShiftAnalogCompositeFormat625;
			// Validate NTSC/PAL reading with Frame Rate Family
			int integerRate;
			integerRate = (analogDetect & kRegMaskAnalogInputIntegerRate) >> kRegShiftAnalogInputIntegerRate;
	
			if (Pal)
			{
				if (integerRate)
					format = NTV2_FORMAT_625_5000;
				else
					format = NTV2_FORMAT_UNKNOWN;	// illegal combination - 625/59.94
			}
			else
			{
				if (integerRate)
					format = NTV2_FORMAT_UNKNOWN;	// illegal combination - 525/60
				else
					format = NTV2_FORMAT_525_5994;
			}
		}
	}
	return format;
}


NTV2VideoFormat CNTV2Card::GetReferenceVideoFormat()
{
	ULWord status;
	if ( ReadInputStatusRegister(&status) )
	{
		//Now it is not really ugly
		return GetNTV2VideoFormat(NTV2FrameRate((status>>16)&0xF),	//framerate
			((status>>20)&0x7),										//input geometry
			(status&BIT_23)>>23,									//progressive transport
			false,													//3G
			false);													//progressive picture
	} else
		return NTV2_FORMAT_UNKNOWN;

}

NTV2FrameRate CNTV2Card::GetSDIInputRate (const NTV2Channel channel)
{
	if (IS_CHANNEL_INVALID (channel))
		return NTV2_FRAMERATE_INVALID;

	ULWord rateLow (0), rateHigh (0);
	NTV2FrameRate currentRate (NTV2_FRAMERATE_INVALID);
	bool result = ReadRegister(gChannelToSDIInputStatusRegNum[channel], rateLow, gChannelToSDIInputRateMask[channel], gChannelToSDIInputRateShift[channel]);
	result = ReadRegister(gChannelToSDIInputStatusRegNum[channel], rateHigh, gChannelToSDIInputRateHighMask[channel], gChannelToSDIInputRateHighShift[channel]);
	AJA_UNUSED(result);
	currentRate = NTV2FrameRate(((rateHigh << 3) & BIT_3) | rateLow);
	if(NTV2_IS_VALID_NTV2FrameRate(currentRate))
		return currentRate;
	return NTV2_FRAMERATE_INVALID;
}	//	GetSDIInputRate

NTV2FrameGeometry CNTV2Card::GetSDIInputGeometry (const NTV2Channel channel)
{
	if (IS_CHANNEL_INVALID (channel))
		return NTV2_FG_INVALID;

	ULWord geometryLow (0), geometryHigh (0);
	NTV2FrameGeometry currentGeometry (NTV2_FG_INVALID);
	bool result = ReadRegister(gChannelToSDIInputStatusRegNum[channel], geometryLow, gChannelToSDIInputGeometryMask[channel], gChannelToSDIInputGeometryShift[channel]);
	result = ReadRegister(gChannelToSDIInputStatusRegNum[channel], geometryHigh, gChannelToSDIInputGeometryHighMask[channel], gChannelToSDIInputGeometryHighShift[channel]);
	AJA_UNUSED(result);
	currentGeometry = NTV2FrameGeometry(((geometryHigh << 3) & BIT_3) | geometryLow);
	if(NTV2_IS_VALID_NTV2FrameGeometry(currentGeometry))
		return currentGeometry;
	return NTV2_FG_INVALID;
}	//	GetSDIInputGeometry

bool CNTV2Card::GetSDIInputIsProgressive (const NTV2Channel channel)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;

	ULWord isProgressive = 0;
	ReadRegister(gChannelToSDIInputStatusRegNum[channel], isProgressive, gChannelToSDIInputProgressiveMask[channel], gChannelToSDIInputProgressiveShift[channel]);
	return isProgressive ? true : false;
}	//	GetSDIInputIsProgressive

bool CNTV2Card::GetSDIInput3GPresent (bool & outValue, const NTV2Channel channel)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;

	ULWord	value	(0);
	bool	result	(ReadRegister(gChannelToSDIInput3GStatusRegNum[channel], value, gChannelToSDIIn3GModeMask[channel], gChannelToSDIIn3GModeShift[channel]));
	outValue = static_cast <bool> (value);
	return result;

}	//	GetSDIInput3GPresent

#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::GetSDIInput3GPresent (bool* value, NTV2Channel channel)	{return value ? GetSDIInput3GPresent (*value, channel) : false;}
	bool CNTV2Card::GetSDI1Input3GPresent (bool* value)						{return value ? GetSDIInput3GPresent (*value, NTV2_CHANNEL1) : false;}
	bool CNTV2Card::GetSDI2Input3GPresent (bool* value)						{return value ? GetSDIInput3GPresent (*value, NTV2_CHANNEL2) : false;}
	bool CNTV2Card::GetSDI3Input3GPresent (bool* value)						{return value ? GetSDIInput3GPresent (*value, NTV2_CHANNEL3) : false;}
	bool CNTV2Card::GetSDI4Input3GPresent (bool* value)						{return value ? GetSDIInput3GPresent (*value, NTV2_CHANNEL4) : false;}
#endif	//	!defined (NTV2_DEPRECATE)


bool CNTV2Card::GetSDIInput3GbPresent (bool & outValue, const NTV2Channel channel)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;

	ULWord	value	(0);
	bool	result	(ReadRegister(gChannelToSDIInput3GStatusRegNum[channel], value, gChannelToSDIIn3GbModeMask[channel], gChannelToSDIIn3GbModeShift[channel]));
	outValue = static_cast <bool> (value);
	return result;

}	//	GetSDIInput3GPresent

bool CNTV2Card::GetSDIInput6GPresent (bool & outValue, const NTV2Channel channel)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;

	ULWord	value	(0);
	bool	result	(ReadRegister (gChannelToSDIInput3GStatusRegNum[channel], value, gChannelToSDIIn6GModeMask[channel], gChannelToSDIIn6GModeShift[channel]));
	outValue = static_cast <bool> (value);
	return result;

}	//	GetSDIInput3GPresent

bool CNTV2Card::GetSDIInput12GPresent (bool & outValue, const NTV2Channel channel)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;

	ULWord	value	(0);
	bool	result	(ReadRegister(gChannelToSDIInput3GStatusRegNum[channel], value, gChannelToSDIIn12GModeMask[channel], gChannelToSDIIn12GModeShift[channel]));
	outValue = static_cast <bool> (value);
	return result;

}	//	GetSDIInput3GPresent

#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::GetSDIInput3GbPresent (bool* value, NTV2Channel channel)	{return value ? GetSDIInput3GbPresent (*value, channel) : false;}
	bool CNTV2Card::GetSDI1Input3GbPresent (bool* value)						{return value ? GetSDIInput3GbPresent (*value, NTV2_CHANNEL1) : false;}
	bool CNTV2Card::GetSDI2Input3GbPresent (bool* value)						{return value ? GetSDIInput3GbPresent (*value, NTV2_CHANNEL2) : false;}
	bool CNTV2Card::GetSDI3Input3GbPresent (bool* value)						{return value ? GetSDIInput3GbPresent (*value, NTV2_CHANNEL3) : false;}
	bool CNTV2Card::GetSDI4Input3GbPresent (bool* value)						{return value ? GetSDIInput3GbPresent (*value, NTV2_CHANNEL4) : false;}


	bool CNTV2Card::SetLSVideoADCMode(NTV2LSVideoADCMode value)
	{
		bool result = WriteRegister (kK2RegAnalogOutControl,
							value,
							kLSRegMaskVideoADCMode,
							kLSRegShiftVideoADCMode);
		if (result)
		{
			if (NTV2BoardCanDoProcAmp(GetDeviceID()))
			{
				// The ADC chip is on an I2C bus; writing the ADC Mode to the FPGA Register
				// results in the FPGA performing an I2C Write to the ADC chip.
				// The (analog) ProcAmp implementation uses software control of the I2C bus,
				// so here is some 'arbitration' of the I2C bus. 
				WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 4);	//	Wait for 4 output verticals

				result = RestoreHardwareProcampRegisters();
			}
		}

		return result;
	}

	bool CNTV2Card::GetLSVideoADCMode(NTV2LSVideoADCMode* value)		{return ReadRegister (kK2RegAnalogOutControl,	(ULWord*)value,	kLSRegMaskVideoADCMode,	kLSRegShiftVideoADCMode);}
	// Method: SetKLSInputSelect
	// Input:  NTV2InputSource
	// Output: NONE
	bool CNTV2Card::SetKLSInputSelect(NTV2InputSource value)		{return WriteRegister (kRegCh1Control,	value,	kLSRegMaskVideoInputSelect,	kLSRegShiftVideoInputSelect);}
	// Method: GetKLSInputSelect
	// Output: NTV2InputSource
	bool CNTV2Card::GetKLSInputSelect(NTV2InputSource* value)		{return ReadRegister (kRegCh1Control,	(ULWord*)value,	kLSRegMaskVideoInputSelect,	kLSRegShiftVideoInputSelect);}
	// Kona LH/Xena LH Specific
		// Used to pick downconverter on inputs(sd bitfile only)
	bool CNTV2Card::SetLHDownconvertInput(bool value)		{return WriteRegister (kRegCh1Control,	value,	kKHRegMaskDownconvertInput,	kKHRegShiftDownconvertInput);}
	bool CNTV2Card::GetLHDownconvertInput(bool* value)
	{
		ULWord ULWvalue = *value;
		bool retVal = ReadRegister (kRegCh1Control,	&ULWvalue,	kKHRegMaskDownconvertInput,	kKHRegShiftDownconvertInput);
		*value = ULWvalue;
		return retVal;
	}


	// Used to pick downconverter on outputs.
	bool CNTV2Card::SetLHSDIOutput1Select (NTV2LHOutputSelect value)		{return WriteRegister (kK2RegSDIOut1Control,	value,			kLHRegMaskVideoOutputDigitalSelect,	kLHRegShiftVideoOutputDigitalSelect);}
	bool CNTV2Card::GetLHSDIOutput1Select (NTV2LHOutputSelect *value)		{return ReadRegister (kK2RegSDIOut1Control,		(ULWord*)value,	kLHRegMaskVideoOutputDigitalSelect,	kLHRegShiftVideoOutputDigitalSelect);}
	bool CNTV2Card::SetLHSDIOutput2Select (NTV2LHOutputSelect value)		{return WriteRegister (kK2RegSDIOut2Control,	value,			kLHRegMaskVideoOutputDigitalSelect,	kLHRegShiftVideoOutputDigitalSelect);}
	bool CNTV2Card::GetLHSDIOutput2Select (NTV2LHOutputSelect *value)		{return ReadRegister (kK2RegSDIOut2Control,		(ULWord*)value,	kLHRegMaskVideoOutputDigitalSelect,	kLHRegShiftVideoOutputDigitalSelect);}
	bool CNTV2Card::SetLHAnalogOutputSelect (NTV2LHOutputSelect value)		{return WriteRegister (kK2RegAnalogOutControl,	value,			kLHRegMaskVideoOutputAnalogSelect,	kLHRegShiftVideoOutputAnalogSelect);}
	bool CNTV2Card::GetLHAnalogOutputSelect (NTV2LHOutputSelect *value)		{return ReadRegister (kK2RegAnalogOutControl,	(ULWord*)value,	kLHRegMaskVideoOutputAnalogSelect,	kLHRegShiftVideoOutputAnalogSelect);}
#endif	//	!defined (NTV2_DEPRECATE)


#if !defined (NTV2_DEPRECATE)
	// Functions for cards that support more than one bitfile
	bool CNTV2Card::CheckBitfile(NTV2VideoFormat newValue)
	{
		bool bResult = false;
		
		// If switching from high def to standard def or vice versa some 
		// boards used to require a different bitfile loaded.
		return bResult;
	}


	NTV2BitfileType CNTV2Card::BitfileSwitchNeeded (NTV2DeviceID boardID, NTV2VideoFormat newValue, bool ajaRetail)
	{
		#if !defined (NTV2_DEPRECATE)
		#else	//	!defined(NTV2_DEPRECATE)
			(void) boardID;
			(void) newValue;
			(void) ajaRetail;
		#endif	//	!defined (NTV2_DEPRECATE)
		return NTV2_BITFILE_NO_CHANGE;

	}	//	BitfileSwitchNeeded

	// returns a numeric value to indicate the "size" of a given format. This is used by FormatCompare()
	// to determine whether two formats need up or down converters (or none) to translate between them.
	// This currently returns the V Size of the format since that works for existing up/down convert cases.
	static int FormatSize (NTV2VideoFormat fmt)
	{
		int result = 0;
		
		switch (fmt)
		{
			case NTV2_FORMAT_525_5994:
			case NTV2_FORMAT_525_2398:
			case NTV2_FORMAT_525_2400:
			case NTV2_FORMAT_525psf_2997:		result = 525;	break;
			
			case NTV2_FORMAT_625_5000:			
			case NTV2_FORMAT_625psf_2500:		result = 625;	break;

			case NTV2_FORMAT_720p_2398:
			case NTV2_FORMAT_720p_5994:
			case NTV2_FORMAT_720p_6000:
			case NTV2_FORMAT_720p_5000:			result = 720;	break;
			
			case NTV2_FORMAT_1080i_5000:
			case NTV2_FORMAT_1080i_5994:
			case NTV2_FORMAT_1080i_6000:
			case NTV2_FORMAT_1080psf_2398:
			case NTV2_FORMAT_1080psf_2400:
			case NTV2_FORMAT_1080psf_2500_2:
			case NTV2_FORMAT_1080psf_2997_2:
			case NTV2_FORMAT_1080psf_3000_2:
			case NTV2_FORMAT_1080p_2997:
			case NTV2_FORMAT_1080p_3000:
			case NTV2_FORMAT_1080p_2500:
			case NTV2_FORMAT_1080p_2398:
			case NTV2_FORMAT_1080p_2400:
			case NTV2_FORMAT_1080p_5000_B:
			case NTV2_FORMAT_1080p_5994_B:
			case NTV2_FORMAT_1080p_6000_B:
			case NTV2_FORMAT_1080p_5000_A:
			case NTV2_FORMAT_1080p_5994_A:
			case NTV2_FORMAT_1080p_6000_A:
			case NTV2_FORMAT_1080p_2K_2398:
			case NTV2_FORMAT_1080p_2K_2400:
			case NTV2_FORMAT_1080p_2K_2500:
			case NTV2_FORMAT_1080p_2K_2997:
			case NTV2_FORMAT_1080p_2K_3000:
			case NTV2_FORMAT_1080p_2K_5000:
			case NTV2_FORMAT_1080p_2K_4795_B:
			case NTV2_FORMAT_1080p_2K_4800_B:
			case NTV2_FORMAT_1080p_2K_5000_B:
			case NTV2_FORMAT_1080p_2K_5994_B:
			case NTV2_FORMAT_1080p_2K_6000_B:
			case NTV2_FORMAT_1080p_2K_5994:
			case NTV2_FORMAT_1080p_2K_6000:
			case NTV2_FORMAT_1080psf_2K_2398:
			case NTV2_FORMAT_1080psf_2K_2400:
			case NTV2_FORMAT_1080psf_2K_2500:		result = 1080;	break;
			
			case NTV2_FORMAT_2K_1498:
			case NTV2_FORMAT_2K_1500:
			case NTV2_FORMAT_2K_2398:
			case NTV2_FORMAT_2K_2400:
			case NTV2_FORMAT_2K_2500:			result = 1556;  break;
			
			case NTV2_FORMAT_UNKNOWN:
			default:							result = 0;		break;
		}
		
		return result;
	}


	// returns int < 0 if fmt1 is "smaller" than fmt2
	//               0 if fmt1 is the same size as fmt2
	//             > 0 if fmt1 is "larger" than fmt2
	int CNTV2Card::FormatCompare (NTV2VideoFormat fmt1, NTV2VideoFormat fmt2)
	{
		int result = 0;
		
		int fmtSize1 = ::FormatSize (fmt1);
		int fmtSize2 = ::FormatSize (fmt2);
		
		if (fmtSize1 < fmtSize2)
			result = -1;
		else if (fmtSize1 > fmtSize2)
			result = 1;
		else
			result = 0;
			
		return result;
	}
#endif	//	!defined (NTV2_DEPRECATE)


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::OutputRoutingTable (const NTV2RoutingTable * pInTable)
	{
		if (pInTable->numEntries > MAX_ROUTING_ENTRIES)
			return false;

		for (unsigned int count=0; count < pInTable->numEntries; count++)
		{
			const NTV2RoutingEntry	entry	(pInTable->routingEntry [count]);
			WriteRegister (entry.registerNum, entry.value, entry.mask, entry.shift);
		}

		return true;
	}
#endif	//	!defined (NTV2_DEPRECATE)


static const ULWord	sMasks[]	=	{	0x000000FF,	0x0000FF00,	0x00FF0000,	0xFF000000	};
static const ULWord	sShifts[]	=	{	         0,	         8,	        16,	        24	};


bool CNTV2Card::GetConnectedOutput (const NTV2InputCrosspointID inInputXpt, NTV2OutputCrosspointID & outOutputXpt)
{
	const ULWord	maxRegNum	(::NTV2DeviceGetMaxRegisterNumber (_boardID));
	uint32_t		regNum		(0);
	uint32_t		ndx			(0);

	outOutputXpt = NTV2_OUTPUT_CROSSPOINT_INVALID;
	if (!CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (inInputXpt, regNum, ndx))
		return false;

	if (!regNum)
		return false;	//	Register number is zero
	if (ndx > 3)
		return false;	//	Bad index
	if (regNum > maxRegNum)
		return false;	//	This device doesn't have that routing register

	return CNTV2DriverInterface::ReadRegister (regNum, outOutputXpt, sMasks[ndx], sShifts[ndx]);

}	//	GetConnectedOutput


bool CNTV2Card::GetConnectedInput (const NTV2OutputCrosspointID inOutputXpt, NTV2InputCrosspointID & outInputXpt)
{
	for (outInputXpt = NTV2_FIRST_INPUT_CROSSPOINT;
		outInputXpt <= NTV2_LAST_INPUT_CROSSPOINT;
		outInputXpt = NTV2InputCrosspointID(outInputXpt+1))
	{
		NTV2OutputCrosspointID	tmpOutputXpt	(NTV2_OUTPUT_CROSSPOINT_INVALID);
		if (GetConnectedOutput (outInputXpt, tmpOutputXpt))
			if (tmpOutputXpt == inOutputXpt)
				return true;
	}
	outInputXpt = NTV2_INPUT_CROSSPOINT_INVALID;
	return true;
}


bool CNTV2Card::GetConnectedInputs (const NTV2OutputCrosspointID inOutputXpt, NTV2InputCrosspointIDSet & outInputXpts)
{
	outInputXpts.clear();
	if (!NTV2_IS_VALID_OutputCrosspointID(inOutputXpt))
		return false;
	if (inOutputXpt == NTV2_XptBlack)
		return false;
	for (NTV2InputCrosspointID inputXpt(NTV2_FIRST_INPUT_CROSSPOINT);
		inputXpt <= NTV2_LAST_INPUT_CROSSPOINT;
		inputXpt = NTV2InputCrosspointID(inputXpt+1))
	{
		NTV2OutputCrosspointID	tmpOutputXpt(NTV2_OUTPUT_CROSSPOINT_INVALID);
		if (GetConnectedOutput (inputXpt, tmpOutputXpt))
			if (tmpOutputXpt == inOutputXpt)
				outInputXpts.insert(inputXpt);
	}
	return !outInputXpts.empty();
}


bool CNTV2Card::Connect (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, const bool inValidate)
{
	if (inOutputXpt == NTV2_XptBlack)
		return Disconnect (inInputXpt);

	const ULWord	maxRegNum	(::NTV2DeviceGetMaxRegisterNumber(_boardID));
	uint32_t		regNum		(0);
	uint32_t		ndx			(0);
	bool			canConnect	(true);

	if (!CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo(inInputXpt, regNum, ndx))
		return false;
	if (!regNum)
		return false;	//	Register number is zero
	if (ndx > 3)
		return false;	//	Bad index
	if (regNum > maxRegNum)
		return false;	//	This device doesn't have that routing register

	if (inValidate)		//	If caller requested xpt validation
		if (CanConnect(inInputXpt, inOutputXpt, canConnect))	//	If answer can be trusted
			if (!canConnect)	//	If route not valid
			{
				ROUTEFAIL (GetDisplayName() << ": Unsupported route " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt)
							<< ": reg=" << DEC(regNum) << " val=" << DEC(inOutputXpt) << " mask=" << xHEX0N(sMasks[ndx],8) << " shift=" << DEC(sShifts[ndx]));
				return false;
			}

	ULWord	outputXpt(0);
	const bool isLogging (LOGGING_ROUTING_CHANGES);
	if (isLogging)
		ReadRegister(regNum, outputXpt, sMasks[ndx], sShifts[ndx]);
	const bool result (WriteRegister(regNum, inOutputXpt, sMasks[ndx], sShifts[ndx]));
	if (isLogging)
	{
		if (!result)
			ROUTEFAIL(GetDisplayName() << ": Failed to connect " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt)
						<< ": reg=" << DEC(regNum) << " val=" << DEC(inOutputXpt) << " mask=" << xHEX0N(sMasks[ndx],8) << " shift=" << DEC(sShifts[ndx]));
		else if (outputXpt  &&  inOutputXpt != outputXpt)
			ROUTENOTE(GetDisplayName() << ": Connected " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt)
						<< " -- was from " << ::NTV2OutputCrosspointIDToString(NTV2OutputXptID(outputXpt)));
		else if (!outputXpt  &&  inOutputXpt != outputXpt)
			ROUTENOTE(GetDisplayName() << ": Connected " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt) << " -- was disconnected");
		//else	ROUTEDBG(GetDisplayName() << ": Connection " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt) << " unchanged -- already connected");
	}
	return result;
}


bool CNTV2Card::Disconnect (const NTV2InputCrosspointID inInputXpt)
{
	const ULWord	maxRegNum	(::NTV2DeviceGetMaxRegisterNumber(_boardID));
	uint32_t		regNum		(0);
	uint32_t		ndx			(0);
	ULWord			outputXpt	(0);

	if (!CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo(inInputXpt, regNum, ndx))
		return false;
	if (!regNum)
		return false;	//	Register number is zero
	if (ndx > 3)
		return false;	//	Bad index
	if (regNum > maxRegNum)
		return false;	//	This device doesn't have that routing register

	const bool isLogging (LOGGING_ROUTING_CHANGES);
	bool changed (false);
	if (isLogging)
		changed = ReadRegister(regNum, outputXpt, sMasks[ndx], sShifts[ndx])  &&  outputXpt;
	const bool result (WriteRegister(regNum, NTV2_XptBlack, sMasks[ndx], sShifts[ndx]));
	if (isLogging)
	{
		if (result && changed)
			ROUTENOTE(GetDisplayName() << ": Disconnected " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(NTV2OutputXptID(outputXpt)));
		else if (!result)
			ROUTEFAIL(GetDisplayName() << ": Failed to disconnect " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(NTV2OutputXptID(outputXpt))
						<< ": reg=" << DEC(regNum) << " val=0 mask=" << xHEX0N(sMasks[ndx],8) << " shift=" << DEC(sShifts[ndx]));
		//else	ROUTEDBG(GetDisplayName() << ": " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(NTV2OutputXptID(outputXpt)) << " already disconnected");
	}
	return result;
}


bool CNTV2Card::IsConnectedTo (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, bool & outIsConnected)
{
	NTV2OutputCrosspointID	outputID	(NTV2_XptBlack);

	outIsConnected = false;
	if (!GetConnectedOutput (inInputXpt, outputID))
		return false;

	outIsConnected = outputID == inOutputXpt;
	return true;
}


bool CNTV2Card::IsConnected (const NTV2InputCrosspointID inInputXpt, bool & outIsConnected)
{
	bool	isConnectedToBlack	(false);
	if (!IsConnectedTo (inInputXpt, NTV2_XptBlack, isConnectedToBlack))
		return false;

	outIsConnected = !isConnectedToBlack;
	return true;
}


bool CNTV2Card::CanConnect (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, bool & outCanConnect)
{
	outCanConnect = false;
	if (!HasCanConnectROM())
		return false;	//	No CanConnect ROM -- can't say

	//	NOTE:	This is not a very efficient implementation, but CanConnect probably isn't being
	//			called every frame. The good news is that this function now gets its information
	//			"straight from the horse's mouth" via the validated GetRouteROMInfoFromReg function,
	//			so its answer will be trustworthy.

	//	Check for reasonable input xpt...
	if (ULWord(inInputXpt) < ULWord(NTV2_FIRST_INPUT_CROSSPOINT)  ||  ULWord(inInputXpt) > NTV2_LAST_INPUT_CROSSPOINT)
	{
		ROUTEFAIL(GetDisplayName() << ": " << xHEX0N(UWord(inInputXpt),4) << " > "
				<< xHEX0N(UWord(NTV2_LAST_INPUT_CROSSPOINT),4) << " (out of range)");
		return false;
	}

	//	Every input xpt can connect to XptBlack...
	if (inOutputXpt == NTV2_XptBlack)
		{outCanConnect = true;	return true;}

	//	Check for reasonable output xpt...
	if (ULWord(inOutputXpt) >= UWord(NTV2_LAST_OUTPUT_CROSSPOINT))
	{
		ROUTEFAIL(GetDisplayName() << ":  Bad output xpt " << xHEX0N(ULWord(inOutputXpt),4) << " >= "
					<< xHEX0N(UWord(NTV2_LAST_OUTPUT_CROSSPOINT),4));
		return false;
	}

	//	Determine all legal output xpts for this input xpt...
	NTV2OutputXptIDSet legalOutputXpts;
	const uint32_t regBase(uint32_t(kRegFirstValidXptROMRegister)  +  4UL * uint32_t(inInputXpt - NTV2_FIRST_INPUT_CROSSPOINT));
	for (uint32_t ndx(0);  ndx < 4;  ndx++)
	{
		ULWord regVal(0);
		NTV2InputXptID inputXpt;
		ReadRegister(regBase + ndx, regVal);
		if (!CNTV2SignalRouter::GetRouteROMInfoFromReg (regBase + ndx, regVal, inputXpt, legalOutputXpts, true/*append*/))
			ROUTEWARN(GetDisplayName() << ":  GetRouteROMInfoFromReg failed for register " << DEC(regBase+ndx)
					<< ", input xpt ' " << ::NTV2InputCrosspointIDToString(inInputXpt) << "' " << xHEX0N(UWord(inInputXpt),2));
	}

	//	Is the route implemented?
	outCanConnect = legalOutputXpts.find(inOutputXpt) != legalOutputXpts.end();
	return true;
}


bool CNTV2Card::ApplySignalRoute (const CNTV2SignalRouter & inRouter, const bool inReplace)
{
	if (inReplace)
		if (!ClearRouting ())
			return false;

	NTV2RegisterWrites	registerWrites;
	if (!inRouter.GetRegisterWrites (registerWrites))
		return false;

	return WriteRegisters (registerWrites);
}

bool CNTV2Card::ApplySignalRoute (const NTV2XptConnections & inConnections, const bool inReplace)
{
	if (inReplace)
		if (!ClearRouting ())
			return false;

	unsigned failures(0);
	for (NTV2XptConnectionsConstIter iter(inConnections.begin());  iter != inConnections.end();  ++iter)
		if (!Connect(iter->first, iter->second))
			failures++;
	return failures == 0;
}

bool CNTV2Card::RemoveConnections (const NTV2XptConnections & inConnections)
{
	unsigned failures(0);
	for (NTV2XptConnectionsConstIter iter(inConnections.begin());  iter != inConnections.end();  ++iter)
		if (!Disconnect(iter->first))
			failures++;
	return failures == 0;
}


bool CNTV2Card::ClearRouting (void)
{
	const NTV2RegNumSet	routingRegisters	(CNTV2RegisterExpert::GetRegistersForClass (kRegClass_Routing));
	const ULWord		maxRegisterNumber	(::NTV2DeviceGetMaxRegisterNumber (_boardID));
	unsigned			nFailures			(0);
	ULWord				tally				(0);

	for (NTV2RegNumSetConstIter it(routingRegisters.begin());  it != routingRegisters.end();  ++it)	//	for each routing register
		if (*it <= maxRegisterNumber)																	//		if it's valid for this board
		{	ULWord	num(0);
			if (ReadRegister (*it, num))
				tally += num;
			if (!WriteRegister (*it, 0))																//			then if WriteRegister fails
				nFailures++;																			//				then bump the failure tally
		}

	if (tally && !nFailures)
		ROUTEINFO(GetDisplayName() << ": Routing cleared");
	else if (!nFailures)
		ROUTEDBG(GetDisplayName() << ": Routing already clear, nothing changed");
	else
		ROUTEFAIL(GetDisplayName() << ": " << DEC(nFailures) << " register write(s) failed");
	return nFailures == 0;

}	//	ClearRouting


bool CNTV2Card::GetRouting (CNTV2SignalRouter & outRouting)
{
	outRouting.Reset ();

	//	First, compile a set of NTV2WidgetIDs that are legit for this device...
	NTV2WidgetIDSet	validWidgets;
	if (!CNTV2SignalRouter::GetWidgetIDs (GetDeviceID(), validWidgets))
		return false;

	ROUTEDBG(GetDisplayName() << ": '" << ::NTV2DeviceIDToString(GetDeviceID()) << "' has " << validWidgets.size() << " widgets: " << validWidgets);

	//	Inspect every input of every widget...
	for (NTV2WidgetIDSetConstIter pWidgetID (validWidgets.begin ());  pWidgetID != validWidgets.end ();  ++pWidgetID)
	{
		const NTV2WidgetID	curWidgetID	(*pWidgetID);
		NTV2InputXptIDSet	inputs;

		CNTV2SignalRouter::GetWidgetInputs (curWidgetID, inputs);
		ROUTEDBG(GetDisplayName() << ": " << ::NTV2WidgetIDToString(curWidgetID) << " (" << ::NTV2WidgetIDToString(curWidgetID, true) << ") has " << inputs.size() << " input(s):  " << inputs);

		for (NTV2InputCrosspointIDSetConstIter pInputID (inputs.begin ());  pInputID != inputs.end ();  ++pInputID)
		{
			NTV2OutputCrosspointID	outputID	(NTV2_XptBlack);
			if (!GetConnectedOutput (*pInputID, outputID))
				ROUTEDBG(GetDisplayName() << ": 'GetConnectedOutput' failed for input " << ::NTV2InputCrosspointIDToString(*pInputID) << " (" << ::NTV2InputCrosspointIDToString(*pInputID, true) << ")");
			else if (outputID == NTV2_XptBlack)
				ROUTEDBG(GetDisplayName() << ": 'GetConnectedOutput' returned XptBlack for input '" << ::NTV2InputCrosspointIDToString(*pInputID, true) << "' (" << ::NTV2InputCrosspointIDToString(*pInputID, false) << ")");
			else
			{
				outRouting.AddConnection (*pInputID, outputID);		//	Record this connection...
				ROUTEDBG(GetDisplayName() << ": Connection found -- from input '" << ::NTV2InputCrosspointIDToString(*pInputID, true) << "' (" << ::NTV2InputCrosspointIDToString(*pInputID, false)
						<< ") <== to output '" << ::NTV2OutputCrosspointIDToString(outputID, true) << "' (" << ::NTV2OutputCrosspointIDToString(outputID, false) << ")");
			}
		}	//	for each input
	}	//	for each valid widget
	ROUTEDBG(GetDisplayName() << ": Returning " << outRouting);
	return true;

}	//	GetRouting

bool CNTV2Card::GetConnections (NTV2XptConnections & outConnections)
{
	outConnections.clear();
	NTV2RegisterReads regInfos;
	NTV2InputCrosspointIDSet inputXpts;
	return CNTV2SignalRouter::GetAllWidgetInputs (_boardID, inputXpts)
			&&  CNTV2SignalRouter::GetAllRoutingRegInfos (inputXpts, regInfos)
			&&  ReadRegisters(regInfos)
			&&  CNTV2SignalRouter::GetConnectionsFromRegs (inputXpts, regInfos, outConnections);
}


typedef deque <NTV2InputCrosspointID>				NTV2InputCrosspointQueue;
typedef NTV2InputCrosspointQueue::const_iterator	NTV2InputCrosspointQueueConstIter;
typedef NTV2InputCrosspointQueue::iterator			NTV2InputCrosspointQueueIter;


bool CNTV2Card::GetRoutingForChannel (const NTV2Channel inChannel, CNTV2SignalRouter & outRouting)
{
	NTV2InputCrosspointQueue			inputXptQueue;	//	List of inputs to trace backward from
	static const NTV2InputCrosspointID	SDIOutInputs []	= {	NTV2_XptSDIOut1Input,	NTV2_XptSDIOut2Input,	NTV2_XptSDIOut3Input,	NTV2_XptSDIOut4Input,
															NTV2_XptSDIOut5Input,	NTV2_XptSDIOut6Input,	NTV2_XptSDIOut7Input,	NTV2_XptSDIOut8Input};
	outRouting.Reset ();

	if (IS_CHANNEL_INVALID (inChannel))
		return false;

	//	Seed the input crosspoint queue...
	inputXptQueue.push_back (SDIOutInputs [inChannel]);

	//	Process all queued inputs...
	while (!inputXptQueue.empty ())
	{
		NTV2InputCrosspointID		inputXpt	(inputXptQueue.front ());
		NTV2OutputCrosspointID		outputXpt	(NTV2_XptBlack);
		NTV2WidgetID				widgetID	(NTV2_WIDGET_INVALID);
		NTV2InputCrosspointIDSet	inputXpts;

		inputXptQueue.pop_front ();

		if (inputXpt == NTV2_INPUT_CROSSPOINT_INVALID)
			continue;

		//	Find out what this input is connected to...
		if (!GetConnectedOutput (inputXpt, outputXpt))
			continue;	//	Keep processing input crosspoints, even if this fails

		if (outputXpt != NTV2_XptBlack)
		{
			//	Make a note of this connection...
			outRouting.AddConnection (inputXpt, outputXpt);

			//	Find out what widget this output belongs to...
			CNTV2SignalRouter::GetWidgetForOutput (outputXpt, widgetID);
			assert (NTV2_IS_VALID_WIDGET (widgetID));	//	FIXFIXFIX	I want to know of any missing NTV2OutputCrosspointID ==> NTV2WidgetID links
			if (!NTV2_IS_VALID_WIDGET (widgetID))
				continue;	//	Keep processing input crosspoints, even if no such widget
			if (!::NTV2DeviceCanDoWidget (GetDeviceID (), widgetID))
				continue;	//	Keep processing input crosspoints, even if no such widget on this device

			//	Add every input of the output's widget to the queue...
			CNTV2SignalRouter::GetWidgetInputs (widgetID, inputXpts);
			for (NTV2InputCrosspointIDSetConstIter it (inputXpts.begin ());  it != inputXpts.end ();  ++it)
				inputXptQueue.push_back (*it);
		}	//	if connected to something other than "black" output crosspoint
	}	//	loop til inputXptQueue empty

	ROUTEDBG(GetDisplayName() << ": Channel " << DEC(inChannel+1) << " routing: " << outRouting);
	return true;

}	//	GetRoutingForChannel


bool CNTV2Card::SetConversionMode (NTV2ConversionMode mode)
{
	NTV2Standard inStandard;
	NTV2Standard outStandard;
	bool isPulldown=false;
	bool isDeinterlace=false;

	#if !defined (NTV2_DEPRECATE)
	NTV2BitfileType desiredK2BitFile;		//	DEPRECATION_CANDIDATE
	NTV2BitfileType desiredX2BitFile;		//	DEPRECATION_CANDIDATE
	#endif	//	!defined (NTV2_DEPRECATE)

	///OK...shouldda been a table....started off as only 8 modes....
	switch ( mode )
	{
	case NTV2_1080i_5994to525_5994:
		inStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_DNCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		outStandard = NTV2_STANDARD_525;
		break;
	case NTV2_1080i_2500to625_2500:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_625;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_DNCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_720p_5994to525_5994:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_DNCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_720p_5000to625_2500:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_625;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_DNCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_525_5994to1080i_5994:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_UPCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_UPCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_525_5994to720p_5994:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_720;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_UPCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_UPCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_625_2500to1080i_2500:
		inStandard = NTV2_STANDARD_625;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_UPCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_UPCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_625_2500to720p_5000:
		inStandard = NTV2_STANDARD_625;
		outStandard = NTV2_STANDARD_720;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_UPCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_UPCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_720p_5000to1080i_2500:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_XUCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_720p_5994to1080i_5994:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_XUCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_720p_6000to1080i_3000:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_XUCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_1080i2398to525_2398:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_1080i2398to525_2997:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		isPulldown = true;
		break;
	case NTV2_1080i2400to525_2400:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_1080p2398to525_2398:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_1080p2398to525_2997:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		isPulldown = true;
		break;
	case NTV2_1080p2400to525_2400:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_1080i_2500to720p_5000:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_720;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_XDCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_1080i_5994to720p_5994:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_720;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_XDCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_1080i_3000to720p_6000:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_720;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_XDCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;

	case NTV2_1080i_2398to720p_2398:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_720;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_XDCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_720p_2398to1080i_2398:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;			//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_XUCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
		
	case NTV2_525_2398to1080i_2398:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_UPCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_UPCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
		
	case NTV2_525_5994to525_5994:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_DNCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;
	case NTV2_625_2500to625_2500:
		inStandard = NTV2_STANDARD_625;
		outStandard = NTV2_STANDARD_625;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_DNCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;

	case NTV2_525_5994to525psf_2997:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_525;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_DNCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		isDeinterlace = true;
		break;

	case NTV2_625_5000to625psf_2500:
		inStandard = NTV2_STANDARD_625;
		outStandard = NTV2_STANDARD_625;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_KONA2_DNCVT;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_XENA2_DNCVT;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		isDeinterlace = true;
		break;
		
	case NTV2_1080i_5000to1080psf_2500:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_NO_CHANGE;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		isDeinterlace = true;
		break;

	case NTV2_1080i_5994to1080psf_2997:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_NO_CHANGE;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		isDeinterlace = true;
		break;
		
	case NTV2_1080i_6000to1080psf_3000:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_1080;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_NO_CHANGE;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		isDeinterlace = true;
		break;

	case NTV2_1080p_3000to720p_6000:
		inStandard = NTV2_STANDARD_1080p;
		outStandard = NTV2_STANDARD_720;
		#if !defined (NTV2_DEPRECATE)
		desiredK2BitFile = NTV2_BITFILE_NO_CHANGE;		//	DEPRECATION_CANDIDATE
		desiredX2BitFile = NTV2_BITFILE_NO_CHANGE;		//	DEPRECATION_CANDIDATE
		#endif	//	!defined (NTV2_DEPRECATE)
		break;

	default:
		return false;
	}

	SetConverterInStandard(inStandard);
	SetConverterOutStandard(outStandard);
	if(::NTV2DeviceGetUFCVersion(GetDeviceID()) == 2)
	{
		NTV2VideoFormat format = GetInputForConversionMode(mode);
		SetConverterInRate(GetNTV2FrameRateFromVideoFormat(format));
		format = GetOutputForConversionMode(mode);
		SetConverterOutRate(GetNTV2FrameRateFromVideoFormat(format));
	}
	SetConverterPulldown( isPulldown );
	SetDeinterlaceMode(isDeinterlace);

	return true;

}	//	SetK2ConversionMode


bool CNTV2Card::GetConversionMode(NTV2ConversionMode & outMode)
{
   NTV2Standard inStandard;
   NTV2Standard outStandard;

   GetConverterInStandard(inStandard);
   GetConverterOutStandard(outStandard);

   outMode = NTV2_CONVERSIONMODE_INVALID;

   switch (inStandard)
   {
   case NTV2_STANDARD_525:
       if ( outStandard == NTV2_STANDARD_1080)
           outMode = NTV2_525_5994to1080i_5994;
       else  if ( outStandard == NTV2_STANDARD_720)
           outMode = NTV2_525_5994to720p_5994;
       else  if ( outStandard == NTV2_STANDARD_525)
           outMode = NTV2_525_5994to525_5994;
       break;
   case NTV2_STANDARD_625:
       if ( outStandard == NTV2_STANDARD_1080)
           outMode = NTV2_625_2500to1080i_2500;
       else  if ( outStandard == NTV2_STANDARD_720)
           outMode = NTV2_625_2500to720p_5000;
       else  if ( outStandard == NTV2_STANDARD_625)
           outMode = NTV2_625_2500to625_2500;
       break;
   case NTV2_STANDARD_720:
       if ( outStandard == NTV2_STANDARD_525)
           outMode = NTV2_720p_5994to525_5994;
       else if (outStandard == NTV2_STANDARD_625)
           outMode = NTV2_720p_5000to625_2500;
       break;
   case NTV2_STANDARD_1080:
       if ( outStandard == NTV2_STANDARD_525)
           outMode = NTV2_1080i_5994to525_5994;
       else if (outStandard == NTV2_STANDARD_625)
           outMode = NTV2_1080i_2500to625_2500;
       break;

   case NTV2_STANDARD_1080p:
	   if ( outStandard == NTV2_STANDARD_720)
		   outMode = NTV2_1080p_3000to720p_6000;
	   break;

   default:
	   return false;
   }
     return true;
}


// ProcAmp controls.  Only work on boards with analog inputs.
bool CNTV2Card::WriteSDProcAmpControlsInitialized	(const ULWord inValue)		{return WriteRegister (kVRegProcAmpSDRegsInitialized,		inValue);}
bool CNTV2Card::WriteSDBrightnessAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefBrightness,	inValue);}
bool CNTV2Card::WriteSDContrastAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefContrast,		inValue);}
bool CNTV2Card::WriteSDSaturationAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefSaturation,	inValue);}
bool CNTV2Card::WriteSDHueAdjustment				(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefHue,			inValue);}
bool CNTV2Card::WriteSDCbOffsetAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefCbOffset,		inValue);}
bool CNTV2Card::WriteSDCrOffsetAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefCrOffset,		inValue);}
bool CNTV2Card::WriteHDProcAmpControlsInitialized	(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHDRegsInitialized,		inValue);}
bool CNTV2Card::WriteHDBrightnessAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefBrightness,		inValue);}
bool CNTV2Card::WriteHDContrastAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefContrast,			inValue);}
bool CNTV2Card::WriteHDSaturationAdjustmentCb		(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefSaturationCb,		inValue);}
bool CNTV2Card::WriteHDSaturationAdjustmentCr		(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefSaturationCr,		inValue);}
bool CNTV2Card::WriteHDCbOffsetAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefCbOffset,			inValue);}
bool CNTV2Card::WriteHDCrOffsetAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefCrOffset,			inValue);}
bool CNTV2Card::ReadSDProcAmpControlsInitialized	(ULWord & outValue)			{return ReadRegister (kVRegProcAmpSDRegsInitialized,		outValue);}
bool CNTV2Card::ReadSDBrightnessAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefBrightness,	outValue);}
bool CNTV2Card::ReadSDContrastAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefContrast,		outValue);}
bool CNTV2Card::ReadSDSaturationAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefSaturation,	outValue);}
bool CNTV2Card::ReadSDHueAdjustment					(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefHue,			outValue);}
bool CNTV2Card::ReadSDCbOffsetAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefCbOffset,		outValue);}
bool CNTV2Card::ReadSDCrOffsetAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefCrOffset,		outValue);}
bool CNTV2Card::ReadHDProcAmpControlsInitialized	(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHDRegsInitialized,		outValue);}
bool CNTV2Card::ReadHDBrightnessAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefBrightness,		outValue);}
bool CNTV2Card::ReadHDContrastAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefContrast,			outValue);}
bool CNTV2Card::ReadHDSaturationAdjustmentCb		(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefSaturationCb,		outValue);}
bool CNTV2Card::ReadHDSaturationAdjustmentCr		(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefSaturationCr,		outValue);}
bool CNTV2Card::ReadHDCbOffsetAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefCbOffset,			outValue);}
bool CNTV2Card::ReadHDCrOffsetAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefCrOffset,			outValue);}

// FS1 (and other?) ProcAmp functions
// ProcAmp controls.
bool CNTV2Card::WriteProcAmpC1YAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC1Y_C1CB,		inValue,	kFS1RegMaskProcAmpC1Y,		kFS1RegShiftProcAmpC1Y);}
bool CNTV2Card::WriteProcAmpC1CBAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC1Y_C1CB,		inValue,	kFS1RegMaskProcAmpC1CB,		kFS1RegShiftProcAmpC1CB);}
bool CNTV2Card::WriteProcAmpC1CRAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC1CR_C2CB,		inValue,	kFS1RegMaskProcAmpC1CR,		kFS1RegShiftProcAmpC1CR);}
bool CNTV2Card::WriteProcAmpC2CBAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC1CR_C2CB,		inValue,	kFS1RegMaskProcAmpC2CB,		kFS1RegShiftProcAmpC2CB);}
bool CNTV2Card::WriteProcAmpC2CRAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC2CROffsetY,	inValue,	kFS1RegMaskProcAmpC2CR,		kFS1RegShiftProcAmpC2CR);}
bool CNTV2Card::WriteProcAmpOffsetYAdjustment		(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC2CROffsetY,	inValue,	kFS1RegMaskProcAmpOffsetY,	kFS1RegShiftProcAmpOffsetY);}

bool CNTV2Card::ReadProcAmpC1YAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC1Y_C1CB,		outValue,	kFS1RegMaskProcAmpC1Y,		kFS1RegShiftProcAmpC1Y);}
bool CNTV2Card::ReadProcAmpC1CBAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC1Y_C1CB,		outValue,	kFS1RegMaskProcAmpC1CB,		kFS1RegShiftProcAmpC1CB);}
bool CNTV2Card::ReadProcAmpC1CRAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC1CR_C2CB,		outValue,	kFS1RegMaskProcAmpC1CR,		kFS1RegShiftProcAmpC1CR);}
bool CNTV2Card::ReadProcAmpC2CBAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC1CR_C2CB,		outValue,	kFS1RegMaskProcAmpC2CB,		kFS1RegShiftProcAmpC2CB);}
bool CNTV2Card::ReadProcAmpC2CRAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC2CROffsetY,	outValue,	kFS1RegMaskProcAmpC2CR,		kFS1RegShiftProcAmpC2CR);}
bool CNTV2Card::ReadProcAmpOffsetYAdjustment		(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC2CROffsetY,	outValue,	kFS1RegMaskProcAmpOffsetY,	kFS1RegShiftProcAmpOffsetY);}



/////////////////////////////////////////////////////////////////////
// Stereo Compressor
bool CNTV2Card::SetStereoCompressorOutputMode		(NTV2StereoCompressorOutputMode inValue)	{return WriteRegister (kRegStereoCompressor,	ULWord(inValue),		kRegMaskStereoCompressorOutputMode,		kRegShiftStereoCompressorOutputMode);}
bool CNTV2Card::GetStereoCompressorOutputMode		(NTV2StereoCompressorOutputMode & outValue)	{return CNTV2DriverInterface::ReadRegister  (kRegStereoCompressor,	outValue,	kRegMaskStereoCompressorOutputMode,		kRegShiftStereoCompressorOutputMode);}
bool CNTV2Card::SetStereoCompressorFlipMode			(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipMode,		kRegShiftStereoCompressorFlipMode);}
bool CNTV2Card::GetStereoCompressorFlipMode			(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipMode,		kRegShiftStereoCompressorFlipMode);}
bool CNTV2Card::SetStereoCompressorFlipLeftHorz		(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipLeftHorz,	kRegShiftStereoCompressorFlipLeftHorz);}
bool CNTV2Card::GetStereoCompressorFlipLeftHorz		(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipLeftHorz,	kRegShiftStereoCompressorFlipLeftHorz);}
bool CNTV2Card::SetStereoCompressorFlipLeftVert		(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipLeftVert,	kRegShiftStereoCompressorFlipLeftVert);}
bool CNTV2Card::GetStereoCompressorFlipLeftVert		(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipLeftVert,	kRegShiftStereoCompressorFlipLeftVert);}
bool CNTV2Card::SetStereoCompressorFlipRightHorz	(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipRightHorz,	kRegShiftStereoCompressorFlipRightHorz);}
bool CNTV2Card::GetStereoCompressorFlipRightHorz	(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipRightHorz,	kRegShiftStereoCompressorFlipRightHorz);}
bool CNTV2Card::SetStereoCompressorFlipRightVert	(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipRightVert,	kRegShiftStereoCompressorFlipRightVert);}
bool CNTV2Card::GetStereoCompressorFlipRightVert	(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipRightVert,	kRegShiftStereoCompressorFlipRightVert);}
bool CNTV2Card::SetStereoCompressorStandard			(const NTV2Standard inValue)				{return WriteRegister (kRegStereoCompressor,	ULWord(inValue),		kRegMaskStereoCompressorFormat,			kRegShiftStereoCompressorFormat);}
bool CNTV2Card::GetStereoCompressorStandard			(NTV2Standard & outValue)					{return CNTV2DriverInterface::ReadRegister  (kRegStereoCompressor,	outValue,	kRegMaskStereoCompressorFormat,			kRegShiftStereoCompressorFormat);}
bool CNTV2Card::SetStereoCompressorLeftSource		(const NTV2OutputCrosspointID inValue)		{return WriteRegister (kRegStereoCompressor,	ULWord(inValue),		kRegMaskStereoCompressorLeftSource,		kRegShiftStereoCompressorLeftSource);}
bool CNTV2Card::GetStereoCompressorLeftSource		(NTV2OutputCrosspointID & outValue)			{return CNTV2DriverInterface::ReadRegister  (kRegStereoCompressor,	outValue,	kRegMaskStereoCompressorLeftSource,		kRegShiftStereoCompressorLeftSource);}
bool CNTV2Card::SetStereoCompressorRightSource		(const NTV2OutputCrosspointID inValue)		{return WriteRegister (kRegStereoCompressor,	ULWord(inValue),		kRegMaskStereoCompressorRightSource,	kRegShiftStereoCompressorRightSource);}
bool CNTV2Card::GetStereoCompressorRightSource		(NTV2OutputCrosspointID & outValue)			{return CNTV2DriverInterface::ReadRegister  (kRegStereoCompressor,	outValue,	kRegMaskStereoCompressorRightSource,	kRegShiftStereoCompressorRightSource);}

/////////////////////////////////////////////////////////////////////
// Analog
#if !defined(R2_DEPRECATE)
bool CNTV2Card::SetAnalogInputADCMode				(const NTV2LSVideoADCMode inValue)			{return WriteRegister (kRegAnalogInputControl,	ULWord(inValue),		kRegMaskAnalogInputADCMode,				kRegShiftAnalogInputADCMode);}
bool CNTV2Card::GetAnalogInputADCMode				(NTV2LSVideoADCMode & outValue)				{return CNTV2DriverInterface::ReadRegister  (kRegAnalogInputControl,	outValue,	kRegMaskAnalogInputADCMode,				kRegShiftAnalogInputADCMode);}
#endif


#if !defined (NTV2_DEPRECATE)
	//////////////////////////////////////////////////////////////////
	// Note: FS1 uses SetLSVideoDACMode for DAC #1.
	// FS1 Video DAC #2 (Used for Component Output on FS1)
	bool CNTV2Card::SetFS1VideoDAC2Mode (NTV2K2VideoDACMode value)		{return WriteRegister (kK2RegAnalogOutControl,	(ULWord)value,	kFS1RegMaskVideoDAC2Mode,	kFS1RegShiftVideoDAC2Mode);}
	bool CNTV2Card::GetFS1VideoDAC2Mode(NTV2K2VideoDACMode* value)		{return ReadRegister (kK2RegAnalogOutControl,	(ULWord*)value,	kFS1RegMaskVideoDAC2Mode,	kFS1RegShiftVideoDAC2Mode);}

	// FS1/MOAB supports a different procamp api than the LS.
	// The FS1/MOAB ProcAmp is digital instead of analog, and so does not conflict with ADC Mode	// So, we don't need to wait around for I2C arbitration between hardware and software
	// in order to talk to the ADC. 

	/////////////////////////////////////////////////////////////////
	// FS1 I2C Device Access Registers   (Reg 90-94)
	//////////////////////////////////////////////////////////////
	// I2C Control (Bus #1)
	bool CNTV2Card::GetFS1I2C1ControlWrite				(ULWord *value)						{return ReadRegister  (kRegFS1I2CControl,			value,			kFS1RegMaskI2C1ControlWrite,			kFS1RegShiftI2C1ControlWrite);}
	bool CNTV2Card::SetFS1I2C1ControlWrite				(ULWord value)						{return WriteRegister (kRegFS1I2CControl,			value,			kFS1RegMaskI2C1ControlWrite,			kFS1RegShiftI2C1ControlWrite);}
	bool CNTV2Card::GetFS1I2C1ControlRead				(ULWord *value)						{return ReadRegister  (kRegFS1I2CControl,			value,			kFS1RegMaskI2C1ControlRead,				kFS1RegShiftI2C1ControlRead);}
	bool CNTV2Card::SetFS1I2C1ControlRead				(ULWord value)						{return WriteRegister (kRegFS1I2CControl,			value,			kFS1RegMaskI2C1ControlRead,				kFS1RegShiftI2C1ControlRead);}
	bool CNTV2Card::GetFS1I2C1ControlBusy				(ULWord *value)						{return ReadRegister  (kRegFS1I2CControl,			value,			kFS1RegMaskI2C1ControlBusy,				kFS1RegShiftI2C1ControlBusy);}
	bool CNTV2Card::GetFS1I2C1ControlError				(ULWord *value)						{return ReadRegister  (kRegFS1I2CControl,			value,			kFS1RegMaskI2C1ControlError,			kFS1RegShiftI2C1ControlError);}
	// I2C Control (Bus #2)
	bool CNTV2Card::GetFS1I2C2ControlWrite				(ULWord *value)						{return ReadRegister  (kRegFS1I2CControl,			value,			kFS1RegMaskI2C2ControlWrite,			kFS1RegShiftI2C2ControlWrite);}
	bool CNTV2Card::SetFS1I2C2ControlWrite				(ULWord value)						{return WriteRegister (kRegFS1I2CControl,			value,			kFS1RegMaskI2C2ControlWrite,			kFS1RegShiftI2C2ControlWrite);}
	bool CNTV2Card::GetFS1I2C2ControlRead				(ULWord *value)						{return ReadRegister  (kRegFS1I2CControl,			value,			kFS1RegMaskI2C2ControlRead,				kFS1RegShiftI2C2ControlRead);}
	bool CNTV2Card::SetFS1I2C2ControlRead				(ULWord value)						{return WriteRegister (kRegFS1I2CControl,			value,			kFS1RegMaskI2C2ControlRead,				kFS1RegShiftI2C2ControlRead);}
	bool CNTV2Card::GetFS1I2C2ControlBusy				(ULWord *value)						{return ReadRegister  (kRegFS1I2CControl,			value,			kFS1RegMaskI2C2ControlBusy,				kFS1RegShiftI2C2ControlBusy);}
	bool CNTV2Card::GetFS1I2C2ControlError				(ULWord *value)						{return ReadRegister  (kRegFS1I2CControl,			value,			kFS1RegMaskI2C2ControlError,			kFS1RegShiftI2C2ControlError);}
	// I2C Bus #1
	bool CNTV2Card::GetFS1I2C1Address					(ULWord *value)						{return ReadRegister  (kRegFS1I2C1Address,			value,			kFS1RegMaskI2CAddress,					kFS1RegShiftI2CAddress);}
	bool CNTV2Card::SetFS1I2C1Address					(ULWord value)						{return WriteRegister (kRegFS1I2C1Address,			value,			kFS1RegMaskI2CAddress,					kFS1RegShiftI2CAddress);}
	bool CNTV2Card::GetFS1I2C1SubAddress				(ULWord *value)						{return ReadRegister  (kRegFS1I2C1Address,			value,			kFS1RegMaskI2CSubAddress,				kFS1RegShiftI2CSubAddress);}
	bool CNTV2Card::SetFS1I2C1SubAddress				(ULWord value)						{return WriteRegister (kRegFS1I2C1Address,			value,			kFS1RegMaskI2CSubAddress,				kFS1RegShiftI2CSubAddress);}
	bool CNTV2Card::GetFS1I2C1Data						(ULWord *value)						{return ReadRegister  (kRegFS1I2C1Data,				value,			kFS1RegMaskI2CReadData,					kFS1RegShiftI2CReadData);}
	bool CNTV2Card::SetFS1I2C1Data						(ULWord value)						{return WriteRegister (kRegFS1I2C1Data,				value,			kFS1RegMaskI2CWriteData,				kFS1RegShiftI2CWriteData);}
	// I2C Bus #2
	bool CNTV2Card::GetFS1I2C2Address					(ULWord *value)						{return ReadRegister  (kRegFS1I2C2Address,			value,			kFS1RegMaskI2CAddress,					kFS1RegShiftI2CAddress);}
	bool CNTV2Card::SetFS1I2C2Address					(ULWord value)						{return WriteRegister (kRegFS1I2C2Address,			value,			kFS1RegMaskI2CAddress,					kFS1RegShiftI2CAddress);}
	bool CNTV2Card::GetFS1I2C2SubAddress				(ULWord *value)						{return ReadRegister  (kRegFS1I2C2Address,			value,			kFS1RegMaskI2CSubAddress,				kFS1RegShiftI2CSubAddress);}
	bool CNTV2Card::SetFS1I2C2SubAddress				(ULWord value)						{return WriteRegister (kRegFS1I2C2Address,			value,			kFS1RegMaskI2CSubAddress,				kFS1RegShiftI2CSubAddress);}
	bool CNTV2Card::GetFS1I2C2Data						(ULWord *value)						{return ReadRegister  (kRegFS1I2C2Data,				value,			kFS1RegMaskI2CReadData,					kFS1RegShiftI2CReadData);}
	bool CNTV2Card::SetFS1I2C2Data						(ULWord value)						{return WriteRegister (kRegFS1I2C2Data,				value,			kFS1RegMaskI2CWriteData,				kFS1RegShiftI2CWriteData);}
	bool CNTV2Card::SetFS1ReferenceSelect				(NTV2FS1ReferenceSelect value)		{return WriteRegister (kRegFS1ReferenceSelect,		value,			kFS1RegMaskReferenceInputSelect,		kFS1RegShiftReferenceInputSelect);}
	bool CNTV2Card::GetFS1ReferenceSelect				(NTV2FS1ReferenceSelect *value)		{return ReadRegister  (kRegFS1ReferenceSelect,		(ULWord*)value,	kFS1RegMaskReferenceInputSelect,		kFS1RegShiftReferenceInputSelect);}
	bool CNTV2Card::SetFS1ColorFIDSubcarrierReset		(bool value)						{return WriteRegister (kRegFS1ReferenceSelect,		value,			kFS1RegMaskColorFIDSubcarrierReset,		kFS1RegShiftColorFIDSubcarrierReset);}
	bool CNTV2Card::GetFS1ColorFIDSubcarrierReset		(bool *value)						{return ReadRegister  (kRegFS1ReferenceSelect,		(ULWord*)value,	kFS1RegMaskColorFIDSubcarrierReset,		kFS1RegShiftColorFIDSubcarrierReset);}
	bool CNTV2Card::SetFS1FreezeOutput					(NTV2FS1FreezeOutput value)			{return WriteRegister (kRegFS1ReferenceSelect,		value,			kFS1RegMaskFreezeOutput,				kFS1RegShiftFreezeOutput);}
	bool CNTV2Card::GetFS1FreezeOutput					(NTV2FS1FreezeOutput *value)		{return ReadRegister  (kRegFS1ReferenceSelect,		(ULWord*)value,	kFS1RegMaskFreezeOutput,				kFS1RegShiftFreezeOutput);}
	bool CNTV2Card::SetFS1XptSecondAnalogOutInputSelect	(NTV2OutputCrosspointID value)		{return WriteRegister (kRegFS1ReferenceSelect,		value,			kFS1RegMaskSecondAnalogOutInputSelect,	kFS1RegShiftSecondAnalogOutInputSelect);}
	bool CNTV2Card::GetFS1XptSecondAnalogOutInputSelect	(NTV2OutputCrosspointID *value)		{return ReadRegister  (kRegFS1ReferenceSelect,		(ULWord*)value,	kFS1RegMaskSecondAnalogOutInputSelect,	kFS1RegShiftSecondAnalogOutInputSelect);}
	bool CNTV2Card::SetFS1XptProcAmpInputSelect			(NTV2OutputCrosspointID value)		{return WriteRegister (kRegFS1ReferenceSelect,		value,			kFS1RegMaskProcAmpInputSelect,			kFS1RegShiftProcAmpInputSelect);}
	bool CNTV2Card::GetFS1XptProcAmpInputSelect			(NTV2OutputCrosspointID *value)		{return ReadRegister  (kRegFS1ReferenceSelect,		(ULWord*)value,	kFS1RegMaskProcAmpInputSelect,			kFS1RegShiftProcAmpInputSelect);}
	bool CNTV2Card::SetFS1AudioDelay					(int value)							{return WriteRegister (kRegFS1AudioDelay,			value,			kFS1RegMaskAudioDelay,					kFS1RegShiftAudioDelay);}
	bool CNTV2Card::GetFS1AudioDelay					(int *value)						{return ReadRegister  (kRegFS1AudioDelay,			(ULWord*)value,	kFS1RegMaskAudioDelay,					kFS1RegShiftAudioDelay);}
	bool CNTV2Card::SetLossOfInput						(ULWord value)						{return WriteRegister (kRegFS1ReferenceSelect,		value,			kRegMaskLossOfInput,					kRegShiftLossOfInput);}
	bool CNTV2Card::SetFS1AudioAnalogLevel				(NTV2FS1AudioLevel value)			{return WriteRegister (kRegAudControl,				value,			kFS1RegMaskAudioLevel,					kFS1RegShiftAudioLevel);}
	bool CNTV2Card::GetFS1AudioAnalogLevel				(NTV2FS1AudioLevel *value)			{return ReadRegister  (kRegAudControl,				(ULWord*)value,	kFS1RegMaskAudioLevel,					kFS1RegShiftAudioLevel);}
	bool CNTV2Card::SetFS1OutputTone					(NTV2FS1OutputTone value)			{return WriteRegister (kRegAudControl,				value,			kRegMaskOutputTone,						kRegShiftOutputTone);}
	bool CNTV2Card::GetFS1OutputTone					(NTV2FS1OutputTone *value)			{return ReadRegister  (kRegAudControl,				(ULWord*)value,	kRegMaskOutputTone,						kRegShiftOutputTone);}
	bool CNTV2Card::SetFS1AudioTone						(NTV2FS1AudioTone value)			{return WriteRegister (kRegAudControl,				value,			kRegMaskAudioTone,						kRegShiftAudioTone);}
	bool CNTV2Card::GetFS1AudioTone						(NTV2FS1AudioTone *value)			{return ReadRegister  (kRegAudControl,				(ULWord*)value,	kRegMaskAudioTone,						kRegShiftAudioTone);}
	//	OBSOLETE: This function group modified regs 172-180, but no supported NTV2 devices use those regs:
		bool CNTV2Card::SetFS1AudioGain_Ch1				(int value)							{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioGain_Ch2				(int value)							{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioGain_Ch3				(int value)							{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioGain_Ch4				(int value)							{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioGain_Ch5				(int value)							{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioGain_Ch6				(int value)							{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioGain_Ch7				(int value)							{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioGain_Ch8				(int value)							{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioPhase_Ch1			(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioPhase_Ch2			(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioPhase_Ch3			(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioPhase_Ch4			(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioPhase_Ch5			(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioPhase_Ch6			(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioPhase_Ch7			(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioPhase_Ch8			(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioSource_Ch1			(NTV2AudioChannelMapping value)		{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioSource_Ch2			(NTV2AudioChannelMapping value)		{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioSource_Ch3			(NTV2AudioChannelMapping value)		{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioSource_Ch4			(NTV2AudioChannelMapping value)		{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioSource_Ch5			(NTV2AudioChannelMapping value)		{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioSource_Ch6			(NTV2AudioChannelMapping value)		{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioSource_Ch7			(NTV2AudioChannelMapping value)		{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioSource_Ch8			(NTV2AudioChannelMapping value)		{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioMute_Ch1				(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioMute_Ch2				(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioMute_Ch3				(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioMute_Ch4				(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioMute_Ch5				(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioMute_Ch6				(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioMute_Ch7				(bool value)						{(void) value; return false;}
		bool CNTV2Card::SetFS1AudioMute_Ch8				(bool value)						{(void) value; return false;}


	///////////////////////////////////////////////////////////////
	// FS1 AFD-related stuff
	bool CNTV2Card::SetFS1DownConvertAFDAutoEnable (bool value)		{return WriteRegister (kRegAFDVANCGrabber, value, kFS1RegMaskDownconvertAutoAFDEnable, kFS1RegShiftDownconvertAutoAFDEnable);}

	bool CNTV2Card::GetFS1DownConvertAFDAutoEnable(bool* value)
	{
		ULWord tempVal;
		bool retVal = ReadRegister (kRegAFDVANCGrabber, &tempVal, kFS1RegMaskDownconvertAutoAFDEnable, kFS1RegShiftDownconvertAutoAFDEnable);
		*value = (bool)tempVal;
		return retVal;
	}

	bool CNTV2Card::SetFS1SecondDownConvertAFDAutoEnable (bool value)		{return WriteRegister (kRegAFDVANCGrabber, value, kFS1RegMaskDownconvert2AutoAFDEnable, kFS1RegShiftDownconvert2AutoAFDEnable);}

	bool CNTV2Card::GetFS1SecondDownConvertAFDAutoEnable(bool* value)
	{
		ULWord tempVal;
		bool retVal =  ReadRegister (kRegAFDVANCGrabber, &tempVal, kFS1RegMaskDownconvert2AutoAFDEnable, kFS1RegShiftDownconvert2AutoAFDEnable);
		*value = (bool)tempVal;
		return retVal;
	}

	bool CNTV2Card::SetFS1DownConvertAFDDefaultHoldLast (bool value)		{return WriteRegister (kRegAFDVANCGrabber, value, kFS1RegMaskDownconvertAFDDefaultHoldLast, kFS1RegShiftDownconvertAFDDefaultHoldLast);}

	bool CNTV2Card::GetFS1DownConvertAFDDefaultHoldLast(bool* value)
	{
		ULWord tempVal;
		bool retVal = ReadRegister (kRegAFDVANCGrabber, &tempVal, kFS1RegMaskDownconvertAFDDefaultHoldLast, kFS1RegShiftDownconvertAFDDefaultHoldLast);
		*value = (bool)tempVal;
		return retVal;
	}

	bool CNTV2Card::SetFS1SecondDownConvertAFDDefaultHoldLast (bool value)		{return WriteRegister (kRegAFDVANCGrabber, value, kFS1RegMaskDownconvert2AFDDefaultHoldLast, kFS1RegShiftDownconvert2AFDDefaultHoldLast);}

	bool CNTV2Card::GetFS1SecondDownConvertAFDDefaultHoldLast(bool* value)
	{
		ULWord tempVal;
		bool retVal = ReadRegister (kRegAFDVANCGrabber, &tempVal, kFS1RegMaskDownconvert2AFDDefaultHoldLast, kFS1RegShiftDownconvert2AFDDefaultHoldLast);
		*value = (bool)tempVal;
		return retVal;
	}

	// Received AFD Code
	bool CNTV2Card::GetAFDReceivedCode(ULWord* value)		{return ReadRegister (kRegAFDVANCGrabber, (ULWord*)value, kFS1RegMaskAFDReceived_Code, kFS1RegShiftAFDReceived_Code);}
	bool CNTV2Card::GetAFDReceivedAR(ULWord* value)		{return ReadRegister (kRegAFDVANCGrabber, (ULWord*)value, kFS1RegMaskAFDReceived_AR, kFS1RegShiftAFDReceived_AR);}

	bool CNTV2Card::GetAFDReceivedVANCPresent(bool* value)
	{
		ULWord tempVal;
		bool retVal = ReadRegister (kRegAFDVANCGrabber, &tempVal, kFS1RegMaskAFDReceived_VANCPresent, kFS1RegShiftAFDReceived_VANCPresent);
		*value = (bool)tempVal;
		return retVal;
	}


	/////////
	bool CNTV2Card::SetAFDInsertMode_SDI1		(NTV2AFDInsertMode value)			{return WriteRegister (kRegAFDVANCInserterSDI1,	value,			kFS1RegMaskAFDVANCInserter_Mode,	kFS1RegShiftAFDVANCInserter_Mode);}
	bool CNTV2Card::GetAFDInsertMode_SDI1		(NTV2AFDInsertMode* value)			{return ReadRegister  (kRegAFDVANCInserterSDI1,	(ULWord*)value,	kFS1RegMaskAFDVANCInserter_Mode,	kFS1RegShiftAFDVANCInserter_Mode);}
	bool CNTV2Card::SetAFDInsertMode_SDI2		(NTV2AFDInsertMode value)			{return WriteRegister (kRegAFDVANCInserterSDI2,	value,			kFS1RegMaskAFDVANCInserter_Mode,	kFS1RegShiftAFDVANCInserter_Mode);}
	bool CNTV2Card::GetAFDInsertMode_SDI2		(NTV2AFDInsertMode* value)			{return ReadRegister  (kRegAFDVANCInserterSDI2,	(ULWord*)value,	kFS1RegMaskAFDVANCInserter_Mode,	kFS1RegShiftAFDVANCInserter_Mode);}
	bool CNTV2Card::SetAFDInsertAR_SDI1			(NTV2AFDInsertAspectRatio value)	{return WriteRegister (kRegAFDVANCInserterSDI1,	value,			kFS1RegMaskAFDVANCInserter_AR,		kFS1RegShiftAFDVANCInserter_AR);}
	bool CNTV2Card::GetAFDInsertAR_SDI1			(NTV2AFDInsertAspectRatio* value)	{return ReadRegister  (kRegAFDVANCInserterSDI1,	(ULWord*)value,	kFS1RegMaskAFDVANCInserter_AR,		kFS1RegShiftAFDVANCInserter_AR);}
	bool CNTV2Card::SetAFDInsertAR_SDI2			(NTV2AFDInsertAspectRatio value)	{return WriteRegister (kRegAFDVANCInserterSDI2,	value,			kFS1RegMaskAFDVANCInserter_AR,		kFS1RegShiftAFDVANCInserter_AR);}
	bool CNTV2Card::GetAFDInsertAR_SDI2			(NTV2AFDInsertAspectRatio* value)	{return ReadRegister  (kRegAFDVANCInserterSDI2,	(ULWord*)value,	kFS1RegMaskAFDVANCInserter_AR,		kFS1RegShiftAFDVANCInserter_AR);}
	bool CNTV2Card::SetAFDInsert_SDI1			(NTV2AFDInsertCode value)			{return WriteRegister (kRegAFDVANCInserterSDI1,	value,			kFS1RegMaskAFDVANCInserter_Code,	kFS1RegShiftAFDVANCInserter_Code);}
	bool CNTV2Card::GetAFDInsert_SDI1			(NTV2AFDInsertCode* value)			{return ReadRegister  (kRegAFDVANCInserterSDI1,	(ULWord*)value,	kFS1RegMaskAFDVANCInserter_Code,	kFS1RegShiftAFDVANCInserter_Code);}
	bool CNTV2Card::SetAFDInsert_SDI2			(NTV2AFDInsertCode value)			{return WriteRegister (kRegAFDVANCInserterSDI2,	value,			kFS1RegMaskAFDVANCInserter_Code,	kFS1RegShiftAFDVANCInserter_Code);}
	bool CNTV2Card::GetAFDInsert_SDI2			(NTV2AFDInsertCode* value)			{return ReadRegister  (kRegAFDVANCInserterSDI2,	(ULWord*)value,	kFS1RegMaskAFDVANCInserter_Code,	kFS1RegShiftAFDVANCInserter_Code);}
	bool CNTV2Card::SetAFDInsertLineNumber_SDI1	(ULWord value)						{return WriteRegister (kRegAFDVANCInserterSDI1,	value,			kFS1RegMaskAFDVANCInserter_Line,	kFS1RegShiftAFDVANCInserter_Line);}
	bool CNTV2Card::GetAFDInsertLineNumber_SDI1	(ULWord* value)						{return ReadRegister  (kRegAFDVANCInserterSDI1,	(ULWord*)value,	kFS1RegMaskAFDVANCInserter_Line,	kFS1RegShiftAFDVANCInserter_Line);}
	bool CNTV2Card::SetAFDInsertLineNumber_SDI2	(ULWord value)						{return WriteRegister (kRegAFDVANCInserterSDI2,	value,			kFS1RegMaskAFDVANCInserter_Line,	kFS1RegShiftAFDVANCInserter_Line);}
	bool CNTV2Card::GetAFDInsertLineNumber_SDI2	(ULWord* value)						{return ReadRegister  (kRegAFDVANCInserterSDI2,	(ULWord*)value,	kFS1RegMaskAFDVANCInserter_Line,	kFS1RegShiftAFDVANCInserter_Line);}
	NTV2ButtonState CNTV2Card::GetButtonState	(int buttonBit)						{(void) buttonBit;	return eButtonNotSupported;}
#endif	//	!defined (NTV2_DEPRECATE)


/////////////////////////////////////////////////////////
// LHI/IoExpress related methods
bool CNTV2Card::SetLHIVideoDACStandard (const NTV2Standard inValue)		{return WriteRegister (kRegAnalogOutControl, ULWord(inValue), kLHIRegMaskVideoDACStandard, kLHIRegShiftVideoDACStandard);}
bool CNTV2Card::GetLHIVideoDACStandard (NTV2Standard & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegAnalogOutControl, outValue, kLHIRegMaskVideoDACStandard, kLHIRegShiftVideoDACStandard);}
bool CNTV2Card::SetLHIVideoDACMode (const NTV2LHIVideoDACMode inValue)	{return WriteRegister (kRegAnalogOutControl, ULWord(inValue), kLHIRegMaskVideoDACMode, kLHIRegShiftVideoDACMode);}
bool CNTV2Card::GetLHIVideoDACMode (NTV2LHIVideoDACMode & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegAnalogOutControl, outValue, kLHIRegMaskVideoDACMode, kLHIRegShiftVideoDACMode);}

// overloaded - alternately takes NTV2K2VideoDACMode instead of NTV2LHIVideoDACMode
bool CNTV2Card::SetLHIVideoDACMode(const NTV2VideoDACMode inValue)
{
	switch(inValue)
	{
		case NTV2_480iRGB:					return SetLHIVideoDACMode(NTV2LHI_480iRGB)					&&  SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iYPbPrSMPTE:			return SetLHIVideoDACMode(NTV2LHI_480iYPbPrSMPTE)			&&  SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iYPbPrBetacam525:		return SetLHIVideoDACMode(NTV2LHI_480iYPbPrBetacam525)		&&  SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iYPbPrBetacamJapan:	return SetLHIVideoDACMode(NTV2LHI_480iYPbPrBetacamJapan)	&&  SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iNTSC_US_Composite:	return SetLHIVideoDACMode(NTV2LHI_480iNTSC_US_Composite)	&&  SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iNTSC_Japan_Composite:	return SetLHIVideoDACMode(NTV2LHI_480iNTSC_Japan_Composite)	&&  SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_576iRGB:					return SetLHIVideoDACMode(NTV2LHI_576iRGB)					&&  SetLHIVideoDACStandard(NTV2_STANDARD_625);
		case NTV2_576iYPbPrSMPTE:			return SetLHIVideoDACMode(NTV2LHI_576iYPbPrSMPTE)			&&  SetLHIVideoDACStandard(NTV2_STANDARD_625);
		case NTV2_576iPAL_Composite:		return SetLHIVideoDACMode(NTV2LHI_576iPAL_Composite)		&&  SetLHIVideoDACStandard(NTV2_STANDARD_625);
	
		case NTV2_1080iRGB:
		case NTV2_1080psfRGB:				return SetLHIVideoDACMode(NTV2LHI_1080iRGB)					&&  SetLHIVideoDACStandard(NTV2_STANDARD_1080);
	
		case NTV2_1080iSMPTE:
		case NTV2_1080psfSMPTE:				return SetLHIVideoDACMode(NTV2LHI_1080iSMPTE)				&&  SetLHIVideoDACStandard(NTV2_STANDARD_1080);
	
		case NTV2_720pRGB:					return SetLHIVideoDACMode(NTV2LHI_720pRGB)					&&  SetLHIVideoDACStandard(NTV2_STANDARD_720);
		case NTV2_720pSMPTE:				return SetLHIVideoDACMode(NTV2LHI_720pSMPTE)				&&  SetLHIVideoDACStandard(NTV2_STANDARD_720);
	
		// not yet supported
		case NTV2_1080iXVGA:
		case NTV2_1080psfXVGA:
		case NTV2_720pXVGA:
		case NTV2_2Kx1080RGB:
		case NTV2_2Kx1080SMPTE:
		case NTV2_2Kx1080XVGA:
		default:							break;
	}
	return false;
}


// overloaded - alternately returns NTV2K2VideoDACMode instead of NTV2LHIVideoDACMode
bool CNTV2Card::GetLHIVideoDACMode(NTV2VideoDACMode & outValue)
{
	NTV2LHIVideoDACMode	lhiValue	(NTV2_MAX_NUM_LHIVideoDACModes);
	NTV2Standard		standard	(NTV2_STANDARD_UNDEFINED);
	bool				result		(GetLHIVideoDACMode(lhiValue)  &&  GetLHIVideoDACStandard(standard));
	
	if (result)
		switch(standard)
		{
			case NTV2_STANDARD_525:
				switch (lhiValue)
				{
					case NTV2LHI_480iRGB:					outValue = NTV2_480iRGB;					break;
					case NTV2LHI_480iYPbPrSMPTE:			outValue = NTV2_480iYPbPrSMPTE;				break;
					case NTV2LHI_480iYPbPrBetacam525:		outValue = NTV2_480iYPbPrBetacam525;		break;
					case NTV2LHI_480iYPbPrBetacamJapan:		outValue = NTV2_480iYPbPrBetacamJapan;		break;
					case NTV2LHI_480iNTSC_US_Composite:		outValue = NTV2_480iNTSC_US_Composite;		break;
					case NTV2LHI_480iNTSC_Japan_Composite:	outValue = NTV2_480iNTSC_Japan_Composite;	break;
					default:								result = false;								break;
				}
				break;
			
			case NTV2_STANDARD_625:
				switch (lhiValue)
				{
					case NTV2LHI_576iRGB:					outValue = NTV2_576iRGB;					break;
					case NTV2LHI_576iYPbPrSMPTE:			outValue = NTV2_576iYPbPrSMPTE;				break;
					case NTV2LHI_576iPAL_Composite:			outValue = NTV2_576iPAL_Composite;			break;
					default:								result = false;								break;
				}
				break;
			
			case NTV2_STANDARD_1080:
				switch (lhiValue)
				{
					case NTV2LHI_1080iRGB:					outValue = NTV2_1080iRGB;					break;	// also NTV2LHI_1080psfRGB
					case NTV2LHI_1080iSMPTE:				outValue = NTV2_1080iSMPTE;					break;	// also NTV2LHI_1080psfSMPTE
					default:								result = false;								break;
				}
				break;
				
			case NTV2_STANDARD_720:
				switch (lhiValue)
				{
					case NTV2LHI_720pRGB:					outValue = NTV2_720pRGB;					break;
					case NTV2LHI_720pSMPTE:					outValue = NTV2_720pSMPTE;					break;
					default:								result = false;								break;
				}
				break;
	
			case NTV2_STANDARD_1080p:
			case NTV2_STANDARD_2K:
			case NTV2_STANDARD_2Kx1080p:
			case NTV2_STANDARD_2Kx1080i:
			case NTV2_STANDARD_3840x2160p:
			case NTV2_STANDARD_4096x2160p:
			case NTV2_STANDARD_3840HFR:
			case NTV2_STANDARD_4096HFR:
			case NTV2_STANDARD_3840i:
			case NTV2_STANDARD_4096i:
			case NTV2_STANDARD_INVALID:
			default:
				result = false;
				break;
		}
	return result;
}


bool CNTV2Card::SetLTCInputEnable(bool value)
{
	if(value && ((GetDeviceID() == DEVICE_ID_IO4K)		||
				 (GetDeviceID() == DEVICE_ID_IO4KUFC)	||
				 (GetDeviceID() == DEVICE_ID_IO4KPLUS)	||
				 (GetDeviceID() == DEVICE_ID_IOIP_2022) ||
				 (GetDeviceID() == DEVICE_ID_IOIP_2110) ||
				 (GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12)))
	{
		NTV2ReferenceSource source;
		GetReference(source);
//		if(source == NTV2_REFERENCE_EXTERNAL)
//			SetReference(NTV2_REFERENCE_FREERUN);
	}
	if(GetDeviceID() == DEVICE_ID_CORVID24)
	{
		value = !value;
	}
	WriteRegister (kRegFS1ReferenceSelect, value, kFS1RefMaskLTCOnRefInSelect, kFS1RefShiftLTCOnRefInSelect);
	return WriteRegister (kRegFS1ReferenceSelect, value? 0 : 1, kRegMaskLTCOnRefInSelect, kRegShiftLTCOnRefInSelect);
}

bool CNTV2Card::SetLTCOnReference(bool value)
{

	if(value && ((GetDeviceID() == DEVICE_ID_IO4K)		||
				 (GetDeviceID() == DEVICE_ID_IO4KUFC)	||
				 (GetDeviceID() == DEVICE_ID_IO4KPLUS)	||
				 (GetDeviceID() == DEVICE_ID_IOIP_2022) ||
				 (GetDeviceID() == DEVICE_ID_IOIP_2110) ||
				 (GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12)))
	{
		NTV2ReferenceSource source;
		GetReference(source);
//		if(source == NTV2_REFERENCE_EXTERNAL)
//			SetReference(NTV2_REFERENCE_FREERUN);
	}
	if(GetDeviceID() == DEVICE_ID_CORVID24)
	{
		value = !value;
	}
	WriteRegister (kRegFS1ReferenceSelect, value, kFS1RefMaskLTCOnRefInSelect, kFS1RefShiftLTCOnRefInSelect);
	return WriteRegister (kRegFS1ReferenceSelect, value? 0 : 1, kRegMaskLTCOnRefInSelect, kRegShiftLTCOnRefInSelect);
}

bool CNTV2Card::GetLTCInputEnable (bool & outIsEnabled)
{
	ULWord	tempVal	(0);
	if (!ReadRegister (kRegFS1ReferenceSelect, tempVal, kFS1RefMaskLTCOnRefInSelect, kFS1RefShiftLTCOnRefInSelect))
		return false;
	outIsEnabled = tempVal ? true : false;
	return true;
}

bool CNTV2Card::GetLTCOnReference (bool & outLTCIsOnReference)
{
	ULWord	tempVal	(0);
	bool	retVal	(ReadRegister (kRegFS1ReferenceSelect, tempVal, kFS1RefMaskLTCOnRefInSelect, kFS1RefShiftLTCOnRefInSelect));
	if (retVal)
		outLTCIsOnReference = (tempVal == 1) ? false : true;
	return retVal;
}

bool CNTV2Card::GetLTCInputPresent (bool & outIsPresent, const UWord inLTCInputNdx)
{
	if (inLTCInputNdx >= ::NTV2DeviceGetNumLTCInputs(_boardID))
		return false;	//	No such LTC input
	if (inLTCInputNdx)	//	LTCIn2
		return CNTV2DriverInterface::ReadRegister (kRegLTCStatusControl, outIsPresent, kRegMaskLTC2InPresent, kRegShiftLTC2InPresent);
	else				//	LTCIn1
	{
		CNTV2DriverInterface::ReadRegister (kRegStatus, outIsPresent, kRegMaskLTCInPresent, kRegShiftLTCInPresent);
		if(outIsPresent)
			return true;
		return CNTV2DriverInterface::ReadRegister (kRegLTCStatusControl, outIsPresent, kRegMaskLTC1InPresent, kRegShiftLTC1InPresent);
	}		
}

bool CNTV2Card::SetLTCEmbeddedOutEnable(bool value)
{
	return WriteRegister (kRegFS1ReferenceSelect, value, kFS1RefMaskLTCEmbeddedOutEnable, kFS1RefShiftLTCEmbeddedOutEnable);
}

bool CNTV2Card::GetLTCEmbeddedOutEnable (bool & outValue)
{
	ULWord	tempVal	(0);
	if (!ReadRegister(kRegFS1ReferenceSelect, tempVal, kFS1RefMaskLTCEmbeddedOutEnable, kFS1RefShiftLTCEmbeddedOutEnable))
		return false;
	outValue = tempVal ? true : false;
	return true;
}


bool CNTV2Card::ReadAnalogLTCInput (const UWord inLTCInput, RP188_STRUCT & outRP188Data)
{
	NTV2_RP188	result;
	if (!ReadAnalogLTCInput(inLTCInput, result))
		return false;
	outRP188Data = result;
	return true;
}


bool CNTV2Card::ReadAnalogLTCInput (const UWord inLTCInput, NTV2_RP188 & outRP188Data)
{
	outRP188Data.Set();
	if (inLTCInput >= ::NTV2DeviceGetNumLTCInputs(_boardID))
		return false;

	ULWord	regLo	(inLTCInput == 0 ? kRegLTCAnalogBits0_31 : (inLTCInput == 1 ? kRegLTC2AnalogBits0_31 : 0));
	ULWord	regHi	(inLTCInput == 0 ? kRegLTCAnalogBits32_63 : (inLTCInput == 1 ? kRegLTC2AnalogBits32_63 : 0));
	outRP188Data.fDBB = 0;
	return regLo  &&  regHi  &&  ReadRegister(regLo, outRP188Data.fLo)  &&  ReadRegister(regHi, outRP188Data.fHi);
}


bool CNTV2Card::GetAnalogLTCInClockChannel (const UWord inLTCInput, NTV2Channel & outChannel)
{
	if (inLTCInput >= ::NTV2DeviceGetNumLTCInputs (_boardID))
		return false;

	ULWord		value			(0);
	ULWord		shift			(inLTCInput == 0 ? 1 : (inLTCInput == 1 ? 9 : 0));	//	Bits 1|2|3 for LTCIn1, bits 9|10|11 for LTCIn2
	const bool	retVal 			(ReadRegister (kRegLTCStatusControl, value, 0x7, shift));
	if (retVal)
		outChannel = static_cast <NTV2Channel> (value + 1);
	return retVal;
}


bool CNTV2Card::SetAnalogLTCInClockChannel (const UWord inLTCInput, const NTV2Channel inChannel)
{
	if (inLTCInput >= ::NTV2DeviceGetNumLTCInputs (_boardID))
		return false;

	ULWord	shift			(inLTCInput == 0 ? 1 : (inLTCInput == 1 ? 9 : 0));	//	Bits 1|2|3 for LTCIn1, bits 9|10|11 for LTCIn2
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (kRegLTCStatusControl, inChannel - 1, 0x7, shift);
}


bool CNTV2Card::WriteAnalogLTCOutput (const UWord inLTCOutput, const RP188_STRUCT & inRP188Data)
{
	const NTV2_RP188	rp188data (inRP188Data);
	return WriteAnalogLTCOutput (inLTCOutput, rp188data);
}


bool CNTV2Card::WriteAnalogLTCOutput (const UWord inLTCOutput, const NTV2_RP188 & inRP188Data)
{
	if (inLTCOutput >= ::NTV2DeviceGetNumLTCOutputs (_boardID))
		return false;

	return WriteRegister (inLTCOutput == 0 ? kRegLTCAnalogBits0_31  : kRegLTC2AnalogBits0_31,  inRP188Data.fLo)
			&& WriteRegister (inLTCOutput == 0 ? kRegLTCAnalogBits32_63 : kRegLTC2AnalogBits32_63, inRP188Data.fHi);
}


bool CNTV2Card::GetAnalogLTCOutClockChannel (const UWord inLTCOutput, NTV2Channel & outChannel)
{
	if (inLTCOutput >= ::NTV2DeviceGetNumLTCOutputs (_boardID))
		return false;

	ULWord		value			(0);
	ULWord		shift			(inLTCOutput == 0 ? 16 : (inLTCOutput == 1 ? 20 : 0));	//	Bits 16|17|18 for LTCOut1, bits 20|21|22 for LTCOut2
	bool		isMultiFormat	(false);
	const bool	retVal 			(shift && GetMultiFormatMode (isMultiFormat) && isMultiFormat && ReadRegister (kRegLTCStatusControl, value, 0x7, shift));
	if (retVal)
		outChannel = static_cast <NTV2Channel> (value + 1);
	return retVal;
}


bool CNTV2Card::SetAnalogLTCOutClockChannel (const UWord inLTCOutput, const NTV2Channel inChannel)
{
	if (inLTCOutput >= ::NTV2DeviceGetNumLTCOutputs (_boardID))
		return false;

	ULWord	shift			(inLTCOutput == 0 ? 16 : (inLTCOutput == 1 ? 20 : 0));	//	Bits 16|17|18 for LTCOut1, bits 20|21|22 for LTCOut2
	bool	isMultiFormat	(false);
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return shift && GetMultiFormatMode (isMultiFormat) && isMultiFormat && WriteRegister (kRegLTCStatusControl, inChannel - 1, 0x7, shift);
}


bool CNTV2Card::SetSDITransmitEnable(NTV2Channel inChannel, bool enable)
{
	ULWord mask, shift;
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	switch (inChannel)
	{
		default:
		case NTV2_CHANNEL1:		mask = kRegMaskSDI1Transmit;	shift = kRegShiftSDI1Transmit;	break;
		case NTV2_CHANNEL2:		mask = kRegMaskSDI2Transmit;	shift = kRegShiftSDI2Transmit;	break;
		case NTV2_CHANNEL3:		mask = kRegMaskSDI3Transmit;	shift = kRegShiftSDI3Transmit;	break;
		case NTV2_CHANNEL4:		mask = kRegMaskSDI4Transmit;	shift = kRegShiftSDI4Transmit;	break;
		case NTV2_CHANNEL5:		mask = kRegMaskSDI5Transmit;	shift = kRegShiftSDI5Transmit;	break;
		case NTV2_CHANNEL6:		mask = kRegMaskSDI6Transmit;	shift = kRegShiftSDI6Transmit;	break;
		case NTV2_CHANNEL7:		mask = kRegMaskSDI7Transmit;	shift = kRegShiftSDI7Transmit;	break;
		case NTV2_CHANNEL8:		mask = kRegMaskSDI8Transmit;	shift = kRegShiftSDI8Transmit;	break;
	}
	return WriteRegister(kRegSDITransmitControl, enable, mask, shift);
}

bool CNTV2Card::SetSDITransmitEnable (const NTV2ChannelSet & inSDIConnectors, const bool inEnable)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inSDIConnectors.begin());  it != inSDIConnectors.end();  ++it)
		if (!SetSDITransmitEnable(*it, inEnable))
			failures++;
	return !failures;
}

bool CNTV2Card::GetSDITransmitEnable(NTV2Channel inChannel, bool & outIsEnabled)
{
	ULWord mask (0), shift (0), tempVal (0);
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	switch (inChannel)
	{
		default:
		case NTV2_CHANNEL1:		mask = kRegMaskSDI1Transmit;	shift = kRegShiftSDI1Transmit;	break;
		case NTV2_CHANNEL2:		mask = kRegMaskSDI2Transmit;	shift = kRegShiftSDI2Transmit;	break;
		case NTV2_CHANNEL3:		mask = kRegMaskSDI3Transmit;	shift = kRegShiftSDI3Transmit;	break;
		case NTV2_CHANNEL4:		mask = kRegMaskSDI4Transmit;	shift = kRegShiftSDI4Transmit;	break;
		case NTV2_CHANNEL5:		mask = kRegMaskSDI5Transmit;	shift = kRegShiftSDI5Transmit;	break;
		case NTV2_CHANNEL6:		mask = kRegMaskSDI6Transmit;	shift = kRegShiftSDI6Transmit;	break;
		case NTV2_CHANNEL7:		mask = kRegMaskSDI7Transmit;	shift = kRegShiftSDI7Transmit;	break;
		case NTV2_CHANNEL8:		mask = kRegMaskSDI8Transmit;	shift = kRegShiftSDI8Transmit;	break;
	}

	const bool	retVal	(ReadRegister (kRegSDITransmitControl, tempVal, mask, shift));
	outIsEnabled = static_cast <bool> (tempVal);
	return retVal;
}


bool CNTV2Card::SetSDIOut2Kx1080Enable (NTV2Channel inChannel, const bool inIsEnabled)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (gChannelToSDIOutControlRegNum [inChannel], inIsEnabled, kK2RegMaskSDI1Out_2Kx1080Mode, kK2RegShiftSDI1Out_2Kx1080Mode);
}

bool CNTV2Card::GetSDIOut2Kx1080Enable(NTV2Channel inChannel, bool & outIsEnabled)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	ULWord		tempVal	(0);
	const bool	retVal	(ReadRegister (gChannelToSDIOutControlRegNum[inChannel], tempVal, kK2RegMaskSDI1Out_2Kx1080Mode, kK2RegShiftSDI1Out_2Kx1080Mode));
	outIsEnabled = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::SetSDIOut3GEnable (const NTV2Channel inChannel, const bool inEnable)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return WriteRegister (gChannelToSDIOutControlRegNum[inChannel], inEnable, kLHIRegMaskSDIOut3GbpsMode, kLHIRegShiftSDIOut3GbpsMode);
}

bool CNTV2Card::GetSDIOut3GEnable (const NTV2Channel inChannel, bool & outIsEnabled)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChannelToSDIOutControlRegNum[inChannel], outIsEnabled, kLHIRegMaskSDIOut3GbpsMode, kLHIRegShiftSDIOut3GbpsMode);
}


bool CNTV2Card::SetSDIOut3GbEnable (const NTV2Channel inChannel, const bool inEnable)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return WriteRegister (gChannelToSDIOutControlRegNum[inChannel], inEnable, kLHIRegMaskSDIOutSMPTELevelBMode, kLHIRegShiftSDIOutSMPTELevelBMode);
}

bool CNTV2Card::GetSDIOut3GbEnable (const NTV2Channel inChannel, bool & outIsEnabled)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChannelToSDIOutControlRegNum[inChannel], outIsEnabled, kLHIRegMaskSDIOutSMPTELevelBMode, kLHIRegShiftSDIOutSMPTELevelBMode);
}

bool CNTV2Card::SetSDIOut6GEnable (const NTV2Channel inChannel, const bool inEnable)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	NTV2Channel channel (inChannel);
	if (!NTV2DeviceCanDo12gRouting(GetDeviceID()))
		channel = NTV2_CHANNEL3;
	if (inEnable)
		WriteRegister(gChannelToSDIOutControlRegNum[channel], 0, kRegMaskSDIOut12GbpsMode, kRegShiftSDIOut12GbpsMode);
	return WriteRegister(gChannelToSDIOutControlRegNum[channel], inEnable, kRegMaskSDIOut6GbpsMode, kRegShiftSDIOut6GbpsMode);
}

bool CNTV2Card::GetSDIOut6GEnable (const NTV2Channel inChannel, bool & outIsEnabled)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	bool	is6G(false), is12G(false);
	NTV2Channel channel (inChannel);
	if (!NTV2DeviceCanDo12gRouting(GetDeviceID()))
		channel = NTV2_CHANNEL3;
	const bool result (CNTV2DriverInterface::ReadRegister(gChannelToSDIOutControlRegNum[channel], is6G, kRegMaskSDIOut6GbpsMode, kRegShiftSDIOut6GbpsMode)
						&& CNTV2DriverInterface::ReadRegister(gChannelToSDIOutControlRegNum[channel], is12G, kRegMaskSDIOut12GbpsMode, kRegShiftSDIOut12GbpsMode));
	if (is6G && !is12G)
		outIsEnabled = true;
	else
		outIsEnabled = false;
	return result;
}

bool CNTV2Card::SetSDIOut12GEnable (const NTV2Channel inChannel, const bool inEnable)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	NTV2Channel channel (inChannel);
	if (!NTV2DeviceCanDo12gRouting(GetDeviceID()))
		channel = NTV2_CHANNEL3;
	if (inEnable)
		WriteRegister(gChannelToSDIOutControlRegNum[channel], 0, kRegMaskSDIOut6GbpsMode, kRegShiftSDIOut6GbpsMode);
	return WriteRegister(gChannelToSDIOutControlRegNum[channel], inEnable, kRegMaskSDIOut12GbpsMode, kRegShiftSDIOut12GbpsMode);
}

bool CNTV2Card::GetSDIOut12GEnable(const NTV2Channel inChannel, bool & outIsEnabled)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	NTV2Channel channel (inChannel);
	if (!NTV2DeviceCanDo12gRouting(GetDeviceID()))
		channel = NTV2_CHANNEL3;
	return CNTV2DriverInterface::ReadRegister(gChannelToSDIOutControlRegNum[channel], outIsEnabled, kRegMaskSDIOut12GbpsMode, kRegShiftSDIOut12GbpsMode);
}


// SDI bypass relay control
static bool WriteWatchdogControlBit (CNTV2Card & card, const ULWord inValue, const ULWord inMask, const ULWord inShift)
{
	if (!card.KickSDIWatchdog())
		return false;
	return card.WriteRegister (kRegSDIWatchdogControlStatus, inValue, inMask, inShift);
}


bool CNTV2Card::KickSDIWatchdog()
{
	if (!::NTV2DeviceHasSDIRelays(GetDeviceID()))
		return false;
	//	Write 0x01234567 into Kick2 register to begin watchdog reset, then in < 30 msec,
	//	write 0xA5A55A5A into Kick1 register to complete the reset...
	const bool status (WriteRegister(kRegSDIWatchdogKick2, 0x01234567));
	return status && WriteRegister(kRegSDIWatchdogKick1, 0xA5A55A5A);
}

bool CNTV2Card::GetSDIWatchdogStatus (NTV2RelayState & outValue)
{
	outValue = NTV2_RELAY_STATE_INVALID;
	if (!::NTV2DeviceHasSDIRelays(GetDeviceID()))
		return false;
	ULWord statusBit(0);
	if (!ReadRegister (kRegSDIWatchdogControlStatus, statusBit, kRegMaskSDIWatchdogStatus, kRegShiftSDIWatchdogStatus))
		return false;
	outValue = statusBit ? NTV2_THROUGH_DEVICE : NTV2_DEVICE_BYPASSED;
	return true;
}

bool CNTV2Card::GetSDIRelayPosition (NTV2RelayState & outValue, const UWord inIndex0)
{
	ULWord statusBit(0);
	outValue = NTV2_RELAY_STATE_INVALID;
	if (!::NTV2DeviceHasSDIRelays(GetDeviceID()))
		return false;
	if (inIndex0 > 1)
		return false;
	if (!ReadRegister (kRegSDIWatchdogControlStatus, statusBit,
						inIndex0 ? kRegMaskSDIRelayPosition34 : kRegMaskSDIRelayPosition12,
						inIndex0 ? kRegShiftSDIRelayPosition34 : kRegShiftSDIRelayPosition12))
		return false;
	outValue = statusBit ? NTV2_THROUGH_DEVICE : NTV2_DEVICE_BYPASSED;
	return true;
}

bool CNTV2Card::GetSDIRelayManualControl (NTV2RelayState & outValue, const UWord inIndex0)
{
	ULWord statusBit(0);
	outValue = NTV2_RELAY_STATE_INVALID;
	if (!::NTV2DeviceHasSDIRelays(GetDeviceID()))
		return false;
	if (inIndex0 > 1)
		return false;
	if (!ReadRegister (kRegSDIWatchdogControlStatus, statusBit,
						inIndex0 ? kRegMaskSDIRelayControl34 : kRegMaskSDIRelayControl12,
						inIndex0 ? kRegShiftSDIRelayControl34 : kRegShiftSDIRelayControl12))
		return false;
	outValue = statusBit ? NTV2_THROUGH_DEVICE : NTV2_DEVICE_BYPASSED;
	return true;
}

bool CNTV2Card::SetSDIRelayManualControl (const NTV2RelayState inValue, const UWord inIndex0)
{
	const ULWord statusBit ((inValue == NTV2_THROUGH_DEVICE) ? 1 : 0);
	if (!::NTV2DeviceHasSDIRelays(GetDeviceID()))
		return false;
	if (inIndex0 > 1)
		return false;
	return WriteWatchdogControlBit (*this, statusBit,
									inIndex0 ? kRegMaskSDIRelayControl34 : kRegMaskSDIRelayControl12,
									inIndex0 ? kRegShiftSDIRelayControl34 : kRegShiftSDIRelayControl12);
}

bool CNTV2Card::GetSDIWatchdogEnable (bool & outValue, const UWord inIndex0)
{
	ULWord statusBit(0);
	outValue = false;
	if (!::NTV2DeviceHasSDIRelays(GetDeviceID()))
		return false;
	if (inIndex0 > 1)
		return false;
	if (!ReadRegister (kRegSDIWatchdogControlStatus, statusBit,
						inIndex0 ? kRegMaskSDIWatchdogEnable34 : kRegMaskSDIWatchdogEnable12,
						inIndex0 ? kRegShiftSDIWatchdogEnable34 : kRegShiftSDIWatchdogEnable12))
		return false;
	outValue = statusBit ? true : false;
	return true;
}

bool CNTV2Card::SetSDIWatchdogEnable (const bool inValue, const UWord inIndex0)
{
	const ULWord statusBit ((inValue == NTV2_THROUGH_DEVICE) ? 1 : 0);
	if (!::NTV2DeviceHasSDIRelays(GetDeviceID()))
		return false;
	if (inIndex0 > 1)
		return false;
	return WriteWatchdogControlBit (*this, statusBit,
									inIndex0 ? kRegMaskSDIWatchdogEnable34 : kRegMaskSDIWatchdogEnable12,
									inIndex0 ? kRegShiftSDIWatchdogEnable34 : kRegShiftSDIWatchdogEnable12);
}

bool CNTV2Card::GetSDIWatchdogTimeout (ULWord & outValue)
{
	outValue = 0;
	if (!::NTV2DeviceHasSDIRelays(GetDeviceID()))
		return false;
	return ReadRegister (kRegSDIWatchdogTimeout, outValue);
}

bool CNTV2Card::SetSDIWatchdogTimeout (const ULWord inValue)
{
	return KickSDIWatchdog()  &&  WriteRegister (kRegSDIWatchdogTimeout, inValue);
}

#if !defined(NTV2_DEPRECATE_15_6)
	bool CNTV2Card::GetSDIWatchdogState (NTV2SDIWatchdogState & outState)
	{
		NTV2SDIWatchdogState tempState;
		if (   GetSDIRelayManualControl (tempState.manualControl12, 0  )
			&& GetSDIRelayManualControl (tempState.manualControl34, 1  )
			&& GetSDIRelayPosition      (tempState.relayPosition12, 0  )
			&& GetSDIRelayPosition      (tempState.relayPosition34, 1  )
			&& GetSDIWatchdogStatus     (tempState.watchdogStatus      )
			&& GetSDIWatchdogEnable     (tempState.watchdogEnable12, 0 )
			&& GetSDIWatchdogEnable     (tempState.watchdogEnable34, 1 )
			&& GetSDIWatchdogTimeout    (tempState.watchdogTimeout     ))
		{
			outState = tempState;
			return true;
		}
		return false;
	}

	bool CNTV2Card::SetSDIWatchdogState (const NTV2SDIWatchdogState & inState)
	{
		return SetSDIRelayManualControl (inState.manualControl12,  0)
			&& SetSDIRelayManualControl (inState.manualControl34,  1)
			&& SetSDIWatchdogTimeout    (inState.watchdogTimeout    )
			&& SetSDIWatchdogEnable     (inState.watchdogEnable12, 0)
			&& SetSDIWatchdogEnable     (inState.watchdogEnable34, 1);
	}
#endif	//	!defined(NTV2_DEPRECATE_15_6)


bool CNTV2Card::Enable4KDCRGBMode(bool enable)
{
	return WriteRegister(kRegDC1, enable, kRegMask4KDCRGBMode, kRegShift4KDCRGBMode);
}

bool CNTV2Card::GetEnable4KDCRGBMode(bool & outIsEnabled)
{
	ULWord		tempVal	(0);
	const bool	retVal	(ReadRegister (kRegDC1, tempVal, kRegMask4KDCRGBMode, kRegShift4KDCRGBMode));
	outIsEnabled = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::Enable4KDCYCC444Mode(bool enable)
{
	return WriteRegister(kRegDC1, enable, kRegMask4KDCYCC444Mode, kRegShift4KDCYCC444Mode);
}

bool CNTV2Card::GetEnable4KDCYCC444Mode(bool & outIsEnabled)
{
	ULWord		tempVal	(0);
	const bool	retVal	(ReadRegister (kRegDC1, tempVal, kRegMask4KDCYCC444Mode, kRegShift4KDCYCC444Mode));
	outIsEnabled = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::Enable4KDCPSFInMode(bool enable)
{
	return WriteRegister(kRegDC1, enable, kRegMask4KDCPSFInMode, kRegShift4KDCPSFInMode);
}

bool CNTV2Card::GetEnable4KDCPSFInMode(bool & outIsEnabled)
{
	ULWord		tempVal	(0);
	const bool	retVal	(ReadRegister (kRegDC1, tempVal, kRegMask4KDCPSFInMode, kRegShift4KDCPSFInMode));
	outIsEnabled = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::Enable4KPSFOutMode(bool enable)
{
	return WriteRegister(kRegDC1, enable, kRegMask4KDCPSFOutMode, kRegShift4KDCPSFOutMode);
}

bool CNTV2Card::GetEnable4KPSFOutMode(bool & outIsEnabled)
{
	ULWord		tempVal	(0);
	const bool	retVal	(ReadRegister (kRegDC1, tempVal, kRegMask4KDCPSFOutMode, kRegShift4KDCPSFOutMode));
	outIsEnabled = static_cast <bool> (tempVal);
	return retVal;
}


bool CNTV2Card::GetSDITRSError (const NTV2Channel inChannel)
{
	if (!::NTV2DeviceCanDoSDIErrorChecks(_boardID))
		return 0;
	if (IS_CHANNEL_INVALID(inChannel))
		return 0;
	ULWord value(0);
	ReadRegister(gChannelToRXSDIStatusRegs[inChannel], value, kRegMaskSDIInTRSError, kRegShiftSDIInTRSError);
	return value ? true : false;
}

bool CNTV2Card::GetSDILock (const NTV2Channel inChannel)
{
	if (!::NTV2DeviceCanDoSDIErrorChecks(_boardID))
		return 0;
	if (IS_CHANNEL_INVALID(inChannel))
		return 0;
	ULWord value(0);
	ReadRegister(gChannelToRXSDIStatusRegs[inChannel], value, kRegMaskSDIInLocked, kRegShiftSDIInLocked);
	return value ? true : false;
}

ULWord CNTV2Card::GetSDIUnlockCount (const NTV2Channel inChannel)
{
	if (!::NTV2DeviceCanDoSDIErrorChecks(_boardID))
		return 0;
	if (IS_CHANNEL_INVALID(inChannel))
		return 0;
	ULWord value(0);
	ReadRegister(gChannelToRXSDIStatusRegs[inChannel], value, kRegMaskSDIInUnlockCount, kRegShiftSDIInUnlockCount);
	return value;
}

ULWord CNTV2Card::GetCRCErrorCountA (const NTV2Channel inChannel)
{
	if (!::NTV2DeviceCanDoSDIErrorChecks(_boardID))
		return 0;
	if (IS_CHANNEL_INVALID(inChannel))
		return 0;
	ULWord value(0);
	ReadRegister(gChannelToRXSDICRCErrorCountRegs[inChannel], value, kRegMaskSDIInCRCErrorCountA, kRegShiftSDIInCRCErrorCountA);
	return value;
}

ULWord CNTV2Card::GetCRCErrorCountB (const NTV2Channel inChannel)
{
	if (!::NTV2DeviceCanDoSDIErrorChecks(_boardID))
		return 0;
	if (IS_CHANNEL_INVALID(inChannel))
		return 0;
	ULWord value(0);
	ReadRegister(gChannelToRXSDICRCErrorCountRegs[inChannel], value, kRegMaskSDIInCRCErrorCountB, kRegShiftSDIInCRCErrorCountB);
	return value;
}

bool CNTV2Card::SetSDIInLevelBtoLevelAConversion (const UWord inInputSpigot, const bool inEnable)
{
	if (!::NTV2DeviceCanDo3GLevelConversion (_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID (inInputSpigot))
		return false;

	ULWord regNum, mask, shift;
	switch (inInputSpigot)
	{
		case NTV2_CHANNEL1:		regNum = kRegSDIInput3GStatus;			mask = kRegMaskSDIIn1LevelBtoLevelA;	shift = kRegShiftSDIIn1LevelBtoLevelA;	break;
		case NTV2_CHANNEL2:		regNum = kRegSDIInput3GStatus;			mask = kRegMaskSDIIn2LevelBtoLevelA;	shift = kRegShiftSDIIn2LevelBtoLevelA;	break;
		case NTV2_CHANNEL3:		regNum = kRegSDIInput3GStatus2;			mask = kRegMaskSDIIn3LevelBtoLevelA;	shift = kRegShiftSDIIn3LevelBtoLevelA;	break;
		case NTV2_CHANNEL4:		regNum = kRegSDIInput3GStatus2;			mask = kRegMaskSDIIn4LevelBtoLevelA;	shift = kRegShiftSDIIn4LevelBtoLevelA;	break;
		case NTV2_CHANNEL5:		regNum = kRegSDI5678Input3GStatus;		mask = kRegMaskSDIIn5LevelBtoLevelA;	shift = kRegShiftSDIIn5LevelBtoLevelA;	break;
		case NTV2_CHANNEL6:		regNum = kRegSDI5678Input3GStatus;		mask = kRegMaskSDIIn6LevelBtoLevelA;	shift = kRegShiftSDIIn6LevelBtoLevelA;	break;
		case NTV2_CHANNEL7:		regNum = kRegSDI5678Input3GStatus;		mask = kRegMaskSDIIn7LevelBtoLevelA;	shift = kRegShiftSDIIn7LevelBtoLevelA;	break;
		case NTV2_CHANNEL8:		regNum = kRegSDI5678Input3GStatus;		mask = kRegMaskSDIIn8LevelBtoLevelA;	shift = kRegShiftSDIIn8LevelBtoLevelA;	break;
		default:				return false;
	}
	return WriteRegister(regNum, inEnable, mask, shift);
}

bool CNTV2Card::GetSDIInLevelBtoLevelAConversion (const UWord inInputSpigot, bool & outEnable)
{
	if (!::NTV2DeviceCanDo3GLevelConversion (_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID (inInputSpigot))
		return false;

	ULWord regNum, mask, shift;
	switch (inInputSpigot)
	{
		case NTV2_CHANNEL1:		regNum = kRegSDIInput3GStatus;			mask = kRegMaskSDIIn1LevelBtoLevelA;	shift = kRegShiftSDIIn1LevelBtoLevelA;	break;
		case NTV2_CHANNEL2:		regNum = kRegSDIInput3GStatus;			mask = kRegMaskSDIIn2LevelBtoLevelA;	shift = kRegShiftSDIIn2LevelBtoLevelA;	break;
		case NTV2_CHANNEL3:		regNum = kRegSDIInput3GStatus2;			mask = kRegMaskSDIIn3LevelBtoLevelA;	shift = kRegShiftSDIIn3LevelBtoLevelA;	break;
		case NTV2_CHANNEL4:		regNum = kRegSDIInput3GStatus2;			mask = kRegMaskSDIIn4LevelBtoLevelA;	shift = kRegShiftSDIIn4LevelBtoLevelA;	break;
		case NTV2_CHANNEL5:		regNum = kRegSDI5678Input3GStatus;		mask = kRegMaskSDIIn5LevelBtoLevelA;	shift = kRegShiftSDIIn5LevelBtoLevelA;	break;
		case NTV2_CHANNEL6:		regNum = kRegSDI5678Input3GStatus;		mask = kRegMaskSDIIn6LevelBtoLevelA;	shift = kRegShiftSDIIn6LevelBtoLevelA;	break;
		case NTV2_CHANNEL7:		regNum = kRegSDI5678Input3GStatus;		mask = kRegMaskSDIIn7LevelBtoLevelA;	shift = kRegShiftSDIIn7LevelBtoLevelA;	break;
		case NTV2_CHANNEL8:		regNum = kRegSDI5678Input3GStatus;		mask = kRegMaskSDIIn8LevelBtoLevelA;	shift = kRegShiftSDIIn8LevelBtoLevelA;	break;
		default:				return false;
	}
	ULWord tempVal;
	bool retVal = ReadRegister (regNum, tempVal, mask, shift);
	outEnable = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::SetSDIOutLevelAtoLevelBConversion (const UWord inOutputSpigot, const bool inEnable)
{
	if (!::NTV2DeviceCanDo3GLevelConversion(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID(inOutputSpigot))
		return false;

	return WriteRegister(gChannelToSDIOutControlRegNum[inOutputSpigot], inEnable, kRegMaskSDIOutLevelAtoLevelB, kRegShiftSDIOutLevelAtoLevelB);
}

bool CNTV2Card::SetSDIOutLevelAtoLevelBConversion (const NTV2ChannelSet & inSDIOutputs, const bool inEnable)
{
	size_t errors(0);
	for (NTV2ChannelSetConstIter it(inSDIOutputs.begin());  it != inSDIOutputs.end();  ++it)
		if (!SetSDIOutLevelAtoLevelBConversion(*it, inEnable))
			errors++;
	return !errors;
}

bool CNTV2Card::GetSDIOutLevelAtoLevelBConversion (const UWord inOutputSpigot, bool & outEnable)
{
	if (!::NTV2DeviceCanDo3GLevelConversion (_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID (inOutputSpigot))
		return false;

	ULWord		tempVal	(0);
	const bool	retVal	(ReadRegister (gChannelToSDIOutControlRegNum[inOutputSpigot], tempVal, kRegMaskSDIOutLevelAtoLevelB, kRegShiftSDIOutLevelAtoLevelB));
	outEnable = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::SetSDIOutRGBLevelAConversion(const UWord inOutputSpigot, const bool inEnable)
{
	if (!::NTV2DeviceCanDoRGBLevelAConversion(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID (inOutputSpigot))
		return false;

	return WriteRegister(gChannelToSDIOutControlRegNum[inOutputSpigot], inEnable, kRegMaskRGBLevelA, kRegShiftRGBLevelA);
}

bool CNTV2Card::SetSDIOutRGBLevelAConversion (const NTV2ChannelSet & inSDIOutputs, const bool inEnable)
{
	size_t errors(0);
	for (NTV2ChannelSetConstIter it(inSDIOutputs.begin());  it != inSDIOutputs.end();  ++it)
		if (!SetSDIOutRGBLevelAConversion(*it, inEnable))
			errors++;
	return !errors;
}

bool CNTV2Card::GetSDIOutRGBLevelAConversion(const UWord inOutputSpigot, bool & outEnable)
{
	if (!::NTV2DeviceCanDoRGBLevelAConversion(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID (inOutputSpigot))
		return false;

	ULWord		tempVal(0);
	const bool	retVal(ReadRegister(gChannelToSDIOutControlRegNum[inOutputSpigot], tempVal, kRegMaskRGBLevelA, kRegShiftRGBLevelA));
	outEnable = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::SetMultiFormatMode (const bool inEnable)
{
	if (!::NTV2DeviceCanDoMultiFormat (_boardID))
		return false;

	return WriteRegister (kRegGlobalControl2, inEnable ? 1 : 0, kRegMaskIndependentMode, kRegShiftIndependentMode);
}

bool CNTV2Card::GetMultiFormatMode (bool & outEnabled)
{
	ULWord		tempVal	(0);
	const bool	retVal	(::NTV2DeviceCanDoMultiFormat (_boardID) ? ReadRegister (kRegGlobalControl2, tempVal, kRegMaskIndependentMode, kRegShiftIndependentMode) : false);
	outEnabled = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::SetRS422Parity (const NTV2Channel inChannel, const NTV2_RS422_PARITY inParity)
{
	if (!::NTV2DeviceCanDoProgrammableRS422(_boardID))
		return false;	//	Non-programmable RS422
	if (inChannel >= ::NTV2DeviceGetNumSerialPorts(_boardID))
		return false;
	if (inParity == NTV2_RS422_NO_PARITY)
	{
		return WriteRegister (gChannelToRS422ControlRegNum [inChannel], 1, kRegMaskRS422ParityDisable, kRegShiftRS422ParityDisable);
	}
	else
	{
		ULWord tempVal (0);
		if (!ReadRegister (gChannelToRS422ControlRegNum[inChannel], tempVal))
			return false;

		tempVal &= ~kRegMaskRS422ParityDisable;
		switch (inParity)
		{
			case NTV2_RS422_ODD_PARITY:		tempVal &= ~kRegMaskRS422ParitySense;	break;
			case NTV2_RS422_EVEN_PARITY:	tempVal |=  kRegMaskRS422ParitySense;	break;

			case NTV2_RS422_PARITY_INVALID:
			default:						return false;
		}

		return WriteRegister (gChannelToRS422ControlRegNum [inChannel], tempVal);
	}
}

bool CNTV2Card::GetRS422Parity (const NTV2Channel inChannel, NTV2_RS422_PARITY & outParity)
{
	outParity = NTV2_RS422_PARITY_INVALID;
	if (inChannel >= ::NTV2DeviceGetNumSerialPorts(_boardID))
		return false;

	ULWord	tempVal (0);
	if (::NTV2DeviceCanDoProgrammableRS422(_boardID))	//	Read register only if programmable RS422
		if (!ReadRegister (gChannelToRS422ControlRegNum[inChannel], tempVal))
			return false;

	if (tempVal & kRegMaskRS422ParityDisable)
		outParity = NTV2_RS422_NO_PARITY;
	else if (tempVal & kRegMaskRS422ParitySense)
		outParity = NTV2_RS422_EVEN_PARITY;
	else
		outParity = NTV2_RS422_ODD_PARITY;	//	Default

	return true;
}

bool CNTV2Card::SetRS422BaudRate (const NTV2Channel inChannel, const NTV2_RS422_BAUD_RATE inBaudRate)
{
	if (!::NTV2DeviceCanDoProgrammableRS422(_boardID))
		return false;	//	Non-programmable RS422
	if (inChannel >= ::NTV2DeviceGetNumSerialPorts(_boardID))
		return false;	//	No such serial port

	ULWord	tempVal (0);
	switch (inBaudRate)
	{
		case NTV2_RS422_BAUD_RATE_38400:	tempVal = 0;		break;
		case NTV2_RS422_BAUD_RATE_19200:	tempVal = 1;		break;
		case NTV2_RS422_BAUD_RATE_9600:		tempVal = 2;		break;
		case NTV2_RS422_BAUD_RATE_INVALID:
		#if !defined(_DEBUG)
		default:
		#endif
											return false;
	}
	return WriteRegister (gChannelToRS422ControlRegNum [inChannel], tempVal, kRegMaskRS422BaudRate, kRegShiftRS422BaudRate);
}

bool CNTV2Card::GetRS422BaudRate (const NTV2Channel inChannel, NTV2_RS422_BAUD_RATE & outBaudRate)
{
	outBaudRate = NTV2_RS422_BAUD_RATE_INVALID;
	if (inChannel >= ::NTV2DeviceGetNumSerialPorts(_boardID))
		return false;	//	No such serial port

	ULWord	tempVal (0);	//	Default to 38400
	if (::NTV2DeviceCanDoProgrammableRS422(_boardID))	//	Read register only if programmable RS422
		if (!ReadRegister (gChannelToRS422ControlRegNum[inChannel], tempVal, kRegMaskRS422BaudRate, kRegShiftRS422BaudRate))
			return false;	//	ReadRegister failed

	switch (tempVal)
	{
		case 0:		outBaudRate = NTV2_RS422_BAUD_RATE_38400;	break;
		case 1:		outBaudRate = NTV2_RS422_BAUD_RATE_19200;	break;
		case 2:		outBaudRate = NTV2_RS422_BAUD_RATE_9600;	break;
		default:	return false;
	}
	return true;
}

bool CNTV2Card::AcquireMailBoxLock()
{
	ULWord val = 0;
	ReadRegister(kVRegMailBoxAcquire, val);
	return val;
}

bool CNTV2Card::ReleaseMailBoxLock()
{
	ULWord val = 0;
	ReadRegister(kVRegMailBoxRelease, val);
	return val;
}

bool CNTV2Card::AbortMailBoxLock()
{
	ULWord val = 0;
	ReadRegister(kVRegMailBoxAbort, val);
	return val;
}

bool CNTV2Card::GetDieTemperature (double & outTemp, const NTV2DieTempScale inTempScale)
{
	outTemp = 0.0;

	//	Read the temperature...
	ULWord			rawRegValue	(0);
	if (!ReadRegister (kRegSysmonVccIntDieTemp, rawRegValue))
		return false;

	const UWord		dieTempRaw	((rawRegValue & 0x0000FFFF) >> 6);
	const double	celsius		(double (dieTempRaw) * 503.975 / 1024.0 - 273.15);
	switch (inTempScale)
	{
		case NTV2DieTempScale_Celsius:		outTemp = celsius;							break;
		case NTV2DieTempScale_Fahrenheit:	outTemp = celsius * 9.0 / 5.0 + 32.0;		break;
		case NTV2DieTempScale_Kelvin:		outTemp = celsius + 273.15;					break;
		case NTV2DieTempScale_Rankine:		outTemp = (celsius + 273.15) * 9.0 / 5.0;	break;
		default:							return false;
	}
	return true;
}

bool CNTV2Card::GetDieVoltage (double & outVoltage)
{
	outVoltage = 0.0;

	//	Read the Vcc voltage...
	ULWord			rawRegValue	(0);
	if (!ReadRegister (kRegSysmonVccIntDieTemp, rawRegValue))
		return false;

	const UWord		coreVoltageRaw	((rawRegValue>>22) & 0x00003FF);
	const double	coreVoltageFloat (double(coreVoltageRaw)/ 1024.0 * 3.0);
	outVoltage = coreVoltageFloat;
	return true;
}

bool CNTV2Card::SetWarmBootFirmwareReload(bool enable)
{
	bool canReboot = false;
	CanWarmBootFPGA(canReboot);
	if(!canReboot)
		return false;
	return WriteRegister(kRegCPLDVersion, enable ? 1:0, BIT(8), 8);
}

#if defined(READREGMULTICHANGE)
	bool CNTV2Card::ReadRegisters (const NTV2RegNumSet & inRegisters,  NTV2RegisterValueMap & outValues)
	{
		outValues.clear ();
		if (!IsOpen())
			return false;		//	Device not open!
		if (inRegisters.empty())
			return false;		//	Nothing to do!
	
		NTV2GetRegisters getRegsParams (inRegisters);
		if (NTV2Message(reinterpret_cast <NTV2_HEADER*>(&getRegsParams)))
		{
			if (!getRegsParams.GetRegisterValues(outValues))
				return false;
		}
		else	//	Non-atomic user-space workaround until GETREGS implemented in driver...
			for (NTV2RegNumSetConstIter iter(inRegisters.begin());  iter != inRegisters.end();  ++iter)
			{
				ULWord	tempVal	(0);
				if (*iter != kRegXenaxFlashDOUT)	//	Prevent firmware erase/program/verify failures
					if (ReadRegister (*iter, tempVal))
						outValues[*iter] = tempVal;
			}
		return outValues.size() == inRegisters.size();
	}
#endif	//	!defined(READREGMULTICHANGE)


bool CNTV2Card::WriteRegisters (const NTV2RegisterWrites & inRegWrites)
{
	if (!_boardOpened)
		return false;		//	Device not open!
	if (inRegWrites.empty())
		return true;		//	Nothing to do!

	bool				result(false);
	NTV2SetRegisters	setRegsParams(inRegWrites);
	//cerr << "## DEBUG:  CNTV2Card::WriteRegisters:  setRegsParams:  " << setRegsParams << endl;
	result = NTV2Message(reinterpret_cast <NTV2_HEADER *> (&setRegsParams));
	if (!result)
	{
		//	Non-atomic user-space workaround until SETREGS implemented in driver...
		const NTV2RegInfo *	pRegInfos = reinterpret_cast<const NTV2RegInfo*>(setRegsParams.mInRegInfos.GetHostPointer());
		UWord *				pBadNdxs = reinterpret_cast<UWord*>(setRegsParams.mOutBadRegIndexes.GetHostPointer());
		for (ULWord ndx(0); ndx < setRegsParams.mInNumRegisters; ndx++)
		{
			if (!WriteRegister(pRegInfos[ndx].registerNumber, pRegInfos[ndx].registerValue, pRegInfos[ndx].registerMask, pRegInfos[ndx].registerShift))
				pBadNdxs[setRegsParams.mOutNumFailures++] = UWord(ndx);
		}
		result = true;
	}
	if (result  &&  setRegsParams.mInNumRegisters  &&  setRegsParams.mOutNumFailures)
		result = false;	//	fail if any writes failed
	if (!result)	CVIDFAIL("Failed: setRegsParams: " << setRegsParams);
	return result;
}

bool CNTV2Card::BankSelectWriteRegister (const NTV2RegInfo & inBankSelect, const NTV2RegInfo & inRegInfo)
{
	bool					result	(false);
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
	{
		// NOTE: DO NOT REMOVE THIS
		// It's needed for the nub client to work
        // TODO: Make an 'atomic bank-select op.---
		// For now, we just do 2 ops and hope nobody else clobbers the
		// bank select register
		if (!WriteRegister(inBankSelect.registerNumber,	inBankSelect.registerValue,	inBankSelect.registerMask,	inBankSelect.registerShift))
			return false;
		return WriteRegister(inRegInfo.registerNumber, inRegInfo.registerValue,	inRegInfo.registerMask,	inRegInfo.registerShift);
	}
	else
#endif	//	NTV2_NUB_CLIENT_SUPPORT
	{
		NTV2BankSelGetSetRegs	bankSelGetSetMsg	(inBankSelect, inRegInfo, true);
		//cerr << "## DEBUG:  CNTV2Card::BankSelectWriteRegister:  " << bankSelGetSetMsg << endl;
		result = NTV2Message (reinterpret_cast <NTV2_HEADER *> (&bankSelGetSetMsg));
        return result;
	}
}

bool CNTV2Card::BankSelectReadRegister (const NTV2RegInfo & inBankSelect, NTV2RegInfo & inOutRegInfo)
{
	bool					result	(false);
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
	{
		// NOTE: DO NOT REMOVE THIS
		// It's needed for the nub client to work

		// TODO: Make an 'atomic bank-select op.
		// For now, we just do 2 ops and hope nobody else clobbers the
		// bank select register
		if (!WriteRegister(inBankSelect.registerNumber, inBankSelect.registerValue, inBankSelect.registerMask, inBankSelect.registerShift))
			return false;
		return ReadRegister(inOutRegInfo.registerNumber, inOutRegInfo.registerValue, inOutRegInfo.registerMask, inOutRegInfo.registerShift);
	}
	else
#endif	//	NTV2_NUB_CLIENT_SUPPORT
	{
		NTV2BankSelGetSetRegs	bankSelGetSetMsg	(inBankSelect, inOutRegInfo);
		//cerr << "## DEBUG:  CNTV2Card::BankSelectReadRegister:  " << bankSelGetSetMsg << endl;
		result = NTV2Message (reinterpret_cast <NTV2_HEADER *> (&bankSelGetSetMsg));
		if (result && !bankSelGetSetMsg.mInRegInfos.IsNULL ())
			inOutRegInfo = bankSelGetSetMsg.GetRegInfo ();
        return result;
	}
}

bool CNTV2Card::WriteVirtualData (const ULWord inTag, const void* inVirtualData, const ULWord inVirtualDataSize)
{
    bool	result	(false);
#if defined (NTV2_NUB_CLIENT_SUPPORT)
    if (IsRemote())
    {
        // NOTE: DO NOT REMOVE THIS
        // It's needed for the nub client to work
    }
    else
#endif	//	NTV2_NUB_CLIENT_SUPPORT
    {
        NTV2VirtualData	virtualDataMsg	(inTag, inVirtualData, inVirtualDataSize, true);
        //cerr << "## DEBUG:  CNTV2Card::WriteVirtualData:  " << virtualDataMsg << endl;
        result = NTV2Message (reinterpret_cast <NTV2_HEADER *> (&virtualDataMsg));
    }
    return result;
}

bool CNTV2Card::ReadVirtualData (const ULWord inTag, void* outVirtualData, const ULWord inVirtualDataSize)
{
    bool	result	(false);
#if defined (NTV2_NUB_CLIENT_SUPPORT)
    if (IsRemote())
    {
        // NOTE: DO NOT REMOVE THIS
        // It's needed for the nub client to work
    }
    else
#endif	//	NTV2_NUB_CLIENT_SUPPORT
    {
        NTV2VirtualData	virtualDataMsg	(inTag, outVirtualData, inVirtualDataSize, false);
        //cerr << "## DEBUG:  CNTV2Card::ReadVirtualData:  " << virtualDataMsg << endl;
        result = NTV2Message (reinterpret_cast <NTV2_HEADER *> (&virtualDataMsg));
    }
    return result;
}

bool CNTV2Card::ReadSDIStatistics (NTV2SDIInStatistics & outStats)
{
	outStats.Clear ();
	if (!_boardOpened)
		return false;		//	Device not open!
	if (!::NTV2DeviceCanDoSDIErrorChecks (_boardID))
		return false;	//	Device doesn't support it!
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return false;
#endif	//	NTV2_NUB_CLIENT_SUPPORT
#if defined(AJAMac)	//	Unimplemented in Mac driver at least thru SDK 16.0, so implement it here
	NTV2RegisterReads sdiStatRegInfos;
	for (size_t sdi(0);  sdi < 8;  sdi++)
		for (ULWord reg(0);  reg < 6;  reg++)
			sdiStatRegInfos.push_back(NTV2RegInfo(reg + gChannelToRXSDIStatusRegs[sdi]));
	sdiStatRegInfos.push_back(NTV2RegInfo(kRegRXSDIFreeRunningClockLow));
	sdiStatRegInfos.push_back(NTV2RegInfo(kRegRXSDIFreeRunningClockHigh));
	//	Read the registers all at once...
	if (!ReadRegisters(sdiStatRegInfos))
		return false;
	//	Stuff the results into outStats...
	for (size_t sdi(0);  sdi < 8;  sdi++)
	{
		NTV2SDIInputStatus & outStat(outStats[sdi]);
		size_t ndx(sdi*6 + 0);	//	Start at kRegRXSDINStatus register
		NTV2_ASSERT(ndx < sdiStatRegInfos.size());
		NTV2RegInfo & regInfo(sdiStatRegInfos.at(ndx));
		NTV2_ASSERT(regInfo.registerNumber == gChannelToRXSDIStatusRegs[sdi]);
		outStat.mUnlockTally	= regInfo.registerValue & kRegMaskSDIInUnlockCount;
		outStat.mLocked			= regInfo.registerValue & kRegMaskSDIInLocked ? true : false;
		outStat.mFrameTRSError	= regInfo.registerValue & kRegMaskSDIInTRSError ? true : false;
		outStat.mVPIDValidA		= regInfo.registerValue & kRegMaskSDIInVpidValidA ? true : false;
		outStat.mVPIDValidB		= regInfo.registerValue & kRegMaskSDIInVpidValidB ? true : false;
		regInfo = sdiStatRegInfos.at(ndx+1);	//	kRegRXSDINCRCErrorCount
		outStat.mCRCTallyA		= regInfo.registerValue & kRegMaskSDIInCRCErrorCountA;
		outStat.mCRCTallyB		= (regInfo.registerValue & kRegMaskSDIInCRCErrorCountB) >> kRegShiftSDIInCRCErrorCountB;
		//									kRegRXSDINFrameCountHigh									kRegRXSDINFrameCountLow
		//outStat.mFrameTally = (ULWord64(sdiStatRegInfos.at(ndx+2).registerValue) << 32) | ULWord64(sdiStatRegInfos.at(ndx+1).registerValue);
		//									kRegRXSDINFrameRefCountHigh									kRegRXSDINFrameRefCountLow
		outStat.mFrameRefClockCount = (ULWord64(sdiStatRegInfos.at(ndx+5).registerValue) << 32) | ULWord64(sdiStatRegInfos.at(ndx+4).registerValue);
		//									kRegRXSDIFreeRunningClockHigh								kRegRXSDIFreeRunningClockLow
		outStat.mGlobalClockCount = (ULWord64(sdiStatRegInfos.at(8*6+1).registerValue) << 32) | ULWord64(sdiStatRegInfos.at(8*6+0).registerValue);
	}	//	for each SDI
	return true;
#else	//	else not MacOS
	return NTV2Message(reinterpret_cast<NTV2_HEADER*>(&outStats));
#endif
}

bool CNTV2Card::SetVPIDTransferCharacteristics (const NTV2VPIDTransferCharacteristics inValue, const NTV2Channel inChannel)
{
	return WriteRegister(gChannelToVPIDTransferCharacteristics[inChannel], inValue);
}

bool CNTV2Card::GetVPIDTransferCharacteristics (NTV2VPIDTransferCharacteristics & outValue, const NTV2Channel inChannel)
{
	ULWord	tempVal (0);
	if (!ReadRegister(gChannelToVPIDTransferCharacteristics[inChannel], tempVal))
		return false;
	outValue = static_cast <NTV2VPIDTransferCharacteristics> (tempVal);
	return true;
}

bool CNTV2Card::SetVPIDColorimetry (const NTV2VPIDColorimetry inValue, const NTV2Channel inChannel)
{
	return WriteRegister(gChannelToVPIDColorimetry[inChannel], inValue);
}

bool CNTV2Card::GetVPIDColorimetry (NTV2VPIDColorimetry & outValue, const NTV2Channel inChannel)
{
	ULWord	tempVal (0);
	if (!ReadRegister(gChannelToVPIDColorimetry[inChannel], tempVal))
		return false;
	outValue = static_cast <NTV2VPIDColorimetry> (tempVal);
	return true;
}

bool CNTV2Card::SetVPIDLuminance (const NTV2VPIDLuminance inValue, const NTV2Channel inChannel)
{
	return WriteRegister(gChannelToVPIDLuminance[inChannel], inValue);
}

bool CNTV2Card::GetVPIDLuminance (NTV2VPIDLuminance & outValue, const NTV2Channel inChannel)
{
	ULWord	tempVal (0);
	if (!ReadRegister(gChannelToVPIDLuminance[inChannel], tempVal))
		return false;
	outValue = static_cast <NTV2VPIDLuminance> (tempVal);
	return true;
}

bool CNTV2Card::SetVPIDRGBRange (const NTV2VPIDRGBRange inValue, const NTV2Channel inChannel)
{
	return WriteRegister(gChannelToVPIDRGBRange[inChannel], inValue);
}

bool CNTV2Card::GetVPIDRGBRange (NTV2VPIDRGBRange & outValue, const NTV2Channel inChannel)
{
	ULWord	tempVal (0);
	if (!ReadRegister(gChannelToVPIDRGBRange[inChannel], tempVal))
		return false;
	outValue = static_cast <NTV2VPIDRGBRange> (tempVal);
	return true;
}

bool CNTV2Card::HasMultiRasterWidget()
{
	ULWord hasMultiRasterWidget(0);
	return ReadRegister(kRegMRSupport, hasMultiRasterWidget, kRegMaskMRSupport, kRegShiftMRSupport)  &&  (hasMultiRasterWidget ? true : false);
}

bool CNTV2Card::SetMultiRasterBypassEnable (const bool inEnable)
{
	if (!HasMultiRasterWidget())
		return false;
	return WriteRegister(kRegMROutControl, inEnable, kRegMaskMRBypass, kRegShiftMRBypass);
}

bool CNTV2Card::GetMultiRasterBypassEnable (bool & outEnabled)
{
	ULWord	tempVal (0);
	if (!ReadRegister(kRegMROutControl, tempVal, kRegMaskMRBypass, kRegShiftMRBypass))
		return false;
	outEnabled = static_cast <bool> (tempVal);
	return true;
}


#if !defined (NTV2_DEPRECATE)
// deprecated - does not support progressivePicture, 3G, 2K
NTV2VideoFormat CNTV2Card::GetNTV2VideoFormat(UByte status, UByte frameRateHiBit)
{
	UByte linesPerFrame = (status>>4)&0x7;
	UByte frameRate = (status&0x7) | (frameRateHiBit<<3);
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	bool progressiveTransport = (status>>7)&0x1;

	switch ( linesPerFrame )
	{
	case 1: 
		// 525
		if ( frameRate == 4 )
			videoFormat = NTV2_FORMAT_525_5994;
		else if ( frameRate == 6 )
			videoFormat = NTV2_FORMAT_525_2400;
		else if ( frameRate == 7)
			videoFormat = NTV2_FORMAT_525_2398;
		break;
	case 2: 
		// 625
		if ( frameRate == 5 )
			videoFormat = NTV2_FORMAT_625_5000;
		break;

	case 3: 
		{
			// 720p
			if ( frameRate == 1 )
				videoFormat = NTV2_FORMAT_720p_6000;
			else if ( frameRate == 2 )
				videoFormat = NTV2_FORMAT_720p_5994;
			else if ( frameRate == 8 )
				videoFormat = NTV2_FORMAT_720p_5000;
		} 
		break;
	case 4: 
		{
			// 1080
			if ( progressiveTransport )
			{
				switch ( frameRate )
				{
				case 3:
					videoFormat = NTV2_FORMAT_1080p_3000;
					break;
				case 4:
					videoFormat = NTV2_FORMAT_1080p_2997;
					break;
				case 5:
					videoFormat = NTV2_FORMAT_1080p_2500;
					break;
				case 6:
					videoFormat = NTV2_FORMAT_1080p_2400;
					break;
				case 7:
					videoFormat = NTV2_FORMAT_1080p_2398;
					break;
				}

			}
			else
			{

				switch ( frameRate )
				{
				case 3:
					videoFormat = NTV2_FORMAT_1080i_6000;
					break;
				case 4:
					videoFormat = NTV2_FORMAT_1080i_5994;
					break;
				case 5:
					videoFormat = NTV2_FORMAT_1080i_5000;
					break;
				case 6:
					videoFormat = NTV2_FORMAT_1080psf_2400;
					break;
				case 7:
					videoFormat = NTV2_FORMAT_1080psf_2398;
					break;
				}
			}
		}
		break;

	}

	return videoFormat;
}

// deprecated - does not support progressivePicture, 3G
NTV2VideoFormat CNTV2Card::GetNTV2VideoFormat(NTV2FrameRate frameRate, UByte inputGeometry, bool progressiveTransport)
{
	NTV2Standard standard = GetNTV2StandardFromScanGeometry(inputGeometry, progressiveTransport);
	return GetNTV2VideoFormat(frameRate, standard, false, inputGeometry, false);
}

// deprecated - does not support progressivePicture, 3G, 2K
NTV2VideoFormat CNTV2Card::GetNTV2VideoFormat(NTV2FrameRate frameRate, NTV2Standard standard)
{
	return GetNTV2VideoFormat(frameRate, standard, false, 0, false);
}
#endif	//	!defined (NTV2_DEPRECATE)



//////////////////////////////////////////////////////////////

#ifdef MSWindows
#pragma warning(default: 4800)
#endif
