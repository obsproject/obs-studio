/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2anc.cpp
	@brief		Implementations of anc-centric CNTV2Card methods.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2formatdescriptor.h"
#include "ntv2utils.h"



/////////////////////////////////////////////////////////////////
/////////////	PER-SDI-SPIGOT REGISTER NUMBERS		/////////////
/////////////////////////////////////////////////////////////////

//								SDI Spigot:		   1	   2	   3	   4	   5	   6	   7	   8
static const ULWord	sAncInsBaseRegNum[]	=	{	4608,	4672,	4736,	4800,	4864,	4928,	4992,	5056	};
static const ULWord	sAncExtBaseRegNum[]	=	{	4096,	4160,	4224,	4288,	4352,	4416,	4480,	4544	};
static const ULWord	kNumDIDRegisters		(regAncExtIgnorePktsReg_Last - regAncExtIgnorePktsReg_First + 1);


static inline ULWord	AncInsRegNum(const UWord inSDIOutput, const ANCInsRegisters inReg)
{
	return sAncInsBaseRegNum[inSDIOutput] + inReg;
}


static inline ULWord	AncExtRegNum(const UWord inSDIInput, const ANCExtRegisters inReg)
{
	return sAncExtBaseRegNum[inSDIInput] + inReg;
}



/////////////////////////////////////////////////////////////////////
/////////////	PER-VIDEO-STANDARD INSERTER INIT SETUP	/////////////
/////////////////////////////////////////////////////////////////////

typedef struct ANCInserterInitParams
{
	uint32_t	field1ActiveLine;
	uint32_t	field2ActiveLine;
	uint32_t	hActivePixels;
	uint32_t	hTotalPixels;
	uint32_t	totalLines;
	uint32_t	fidLow;
	uint32_t	fidHigh;
	uint32_t	field1SwitchLine;
	uint32_t	field2SwitchLine;
	uint32_t	pixelDelay;
} ANCInserterInitParams;

static const ANCInserterInitParams  inserterInitParamsTable [NTV2_NUM_STANDARDS] = {
//						F1		F2		Horz									F1		F2
//						Active	Active	Active	Total	Total	FID		FID		Switch	Switch	Pixel
//	Standard			Line	Line	Pixels	Pixels	Lines	Lo		Hi		Line	Line	Delay
/* 1080       */	{	21,		564,	1920,	2200,	1125,	1125,	563,	7,		569,	8,		},
/* 720        */	{	26,		0,		1280,	1280,	750,	0,		0,		7,		0,		8,		},
/* 525        */	{	21,		283,	720,	720,	525,	3,		265,	10,		273,	8,		},
/* 625        */	{	23,		336,	720,	864,	625,	625,	312,	6,		319,	8,		},
/* 1080p      */	{	42,		0,		1920,	2640,	1125,	0,		0,		7,		0,		8,		},
/* 2K         */	{	42,		0,		2048,	2640,	1125,	0,		0,		7,		0,		8,		},
/* 2Kx1080p   */	{	42,		0,		2048,	2640,	1125,	0,		0,		7,		0,		8,		},
/* 2Kx1080i   */	{	21,		564,	2048,	2200,	1125,	1125,	563,	7,		569,	8,		},
/* 3840x2160p */	{	42,		0,		1920,	2640,	1125,	0,		0,		7,		0,		8,		},
/* 4096x2160p */	{	42,		0,		2048,	2640,	1125,	0,		0,		7,		0,		8,		},
/* 3840HFR    */	{	42,		0,		1920,	2640,	1125,	0,		0,		7,		0,		8,		},
/* 4096HFR    */	{	42,		0,		2048,	2640,	1125,	0,		0,		7,		0,		8,		}
};



/////////////////////////////////////////////////////////////////////
/////////////	PER-VIDEO-STANDARD EXTRACTOR INIT SETUP	/////////////
/////////////////////////////////////////////////////////////////////

typedef struct ANCExtractorInitParams
{
	uint32_t	field1StartLine;
	uint32_t	field1CutoffLine;
	uint32_t	field2StartLine;
	uint32_t	field2CutoffLine;
	uint32_t	totalLines;
	uint32_t	fidLow;
	uint32_t	fidHigh;
	uint32_t	field1SwitchLine;
	uint32_t	field2SwitchLine;
	uint32_t	field1AnalogStartLine;
	uint32_t	field2AnalogStartLine;
	uint32_t	field1AnalogYFilter;
	uint32_t	field2AnalogYFilter;
	uint32_t	field1AnalogCFilter;
	uint32_t	field2AnalogCFilter;
	uint32_t	analogActiveLineLength;
} ANCExtractorInitParams;

const static ANCExtractorInitParams  extractorInitParamsTable [NTV2_NUM_STANDARDS] = {
//					F1		F1		F2		F2								F1		F2		F1Anlg	F2Anlg	F1			F2			F1		F2		Analog
//					Start	Cutoff	Start	Cutoff	Total	FID		FID		Switch	Switch	Start	Start	Anlg		Anlg		Anlg	Anlg	Active
// Standard			Line	Line	Line	Line	Lines	Low		High	Line	Line	Line	Line	Y Filt		Y Filt		C Filt	C Filt	LineLength
/* 1080       */{	561,	26,		1124,	588,	1125,	1125,	563,	7,		569,	0,		0,		0,			0,			0,		0,		0x07800000	},
/* 720        */{	746,	745,	0,		0,		750,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x05000000	},
/* 525        */{	264,	30,		1,		293,	525,	3,		265,	10,		273,	4,		266,	0x20000,	0x40000,	0,		0,		0x02D00000	},
/* 625        */{	311,	33,		1,		346,	625,	625,	312,	6,		319,	5,		318,	0x10000,	0x10000,	0,		0,		0x02D00000	},
/* 1080p      */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2K         */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2Kx1080p   */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2Kx1080i   */{	561,	26,		1124,	588,	1125,	1125,	563,	7,		569,	0,		0,		0,			0,			0,		0,		0x07800000	},
/* 3840x2160p */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 4096x2160p */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 3840HFR    */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 4096HFR    */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
};


/////////////////////////////////////////////
//////////////   ANC BUFFER    //////////////
/////////////////////////////////////////////

bool CNTV2Card::AncSetFrameBufferSize (const ULWord inF1Size, const ULWord inF2Size)
{
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;

	bool ok(true);
	if (ok) ok = WriteRegister(kVRegAncField1Offset, inF1Size + inF2Size);
	if (ok) ok = WriteRegister(kVRegAncField2Offset, inF2Size);
	return ok;
}

static bool GetAncOffsets (CNTV2Card & inDevice, ULWord & outF1Offset, ULWord & outF2Offset)
{
	outF1Offset = outF2Offset = 0;
	return inDevice.ReadRegister(kVRegAncField1Offset, outF1Offset)
		&&  inDevice.ReadRegister(kVRegAncField2Offset, outF2Offset);
}

static bool GetAncField1Size (CNTV2Card & inDevice, ULWord & outFieldBytes)
{
	outFieldBytes = 0;
	ULWord	ancF1Offset(0), ancF2Offset(0);
	if (!GetAncOffsets(inDevice, ancF1Offset, ancF2Offset))
		return false;
	outFieldBytes = ancF1Offset - ancF2Offset;
	return true;
}

static bool GetAncField2Size (CNTV2Card & inDevice, ULWord & outFieldBytes)
{
	outFieldBytes = 0;
	ULWord	ancF1Offset(0), ancF2Offset(0);
	if (!GetAncOffsets(inDevice, ancF1Offset, ancF2Offset))
		return false;
	outFieldBytes = ancF2Offset;
	return true;
}


/////////////////////////////////////////////
/////////////	ANC INSERTER	/////////////
/////////////////////////////////////////////

static bool SetAncInsField1Bytes (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfBytes)
{
	return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldBytes), numberOfBytes & 0xffff,
								  maskInsField1Bytes, shiftInsField1Bytes) &&
			inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldBytesHigh), numberOfBytes >> 16,
											  maskInsField1Bytes, shiftInsField1Bytes);
}

static bool SetAncInsField2Bytes (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfBytes)
{
	return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldBytes), numberOfBytes & 0xffff,
								  maskInsField2Bytes, shiftInsField2Bytes) &&
			inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldBytesHigh), numberOfBytes >> 16,
											  maskInsField2Bytes, shiftInsField2Bytes);
}

static bool EnableAncInsHancY (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), bEnable ? 1 : 0, maskInsEnableHancY, shiftInsEnableHancY);
}

static bool EnableAncInsHancC (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
{
	return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), bEnable ? 1 : 0, maskInsEnableHancC, shiftInsEnableHancC);
}

static bool EnableAncInsVancY (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), bEnable ? 1 : 0, maskInsEnableVancY, shiftInsEnableVancY);
}

static bool EnableAncInsVancC (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), bEnable ? 1 : 0, maskInsEnableVancC, shiftInsEnableVancC);
}

static bool SetAncInsProgressive (CNTV2Card & inDevice, const UWord inSDIOutput, bool isProgressive)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), isProgressive ? 1 : 0, maskInsSetProgressive, shiftInsSetProgressive);
}

static bool SetAncInsSDPacketSplit (CNTV2Card & inDevice, const UWord inSDIOutput, bool inEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl),  inEnable ? 1 : 0,  ULWord(maskInsEnablePktSplitSD), shiftInsEnablePktSplitSD);
}

static bool SetAncInsField1StartAddr (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t startAddr)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsField1StartAddr), startAddr);
}

static bool SetAncInsField2StartAddr (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t startAddr)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsField2StartAddr), startAddr);
}

static bool SetAncInsHancPixelDelay (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfPixels)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsPixelDelay), numberOfPixels, maskInsHancDelay, shiftINsHancDelay);
}

static bool SetAncInsVancPixelDelay (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfPixels)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsPixelDelay), numberOfPixels, maskInsVancDelay, shiftInsVancDelay);
}

static bool SetAncInsField1ActiveLine (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t activeLineNumber)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsActiveStart), activeLineNumber, maskInsField1FirstActive, shiftInsField1FirstActive);
}

static bool SetAncInsField2ActiveLine (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t activeLineNumber)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsActiveStart), activeLineNumber, maskInsField2FirstActive, shiftInsField2FirstActive);
}

static bool SetAncInsHActivePixels (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfActiveLinePixels)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsLinePixels), numberOfActiveLinePixels, maskInsActivePixelsInLine, shiftInsActivePixelsInLine);
}

static bool SetAncInsHTotalPixels (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfLinePixels)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsLinePixels), numberOfLinePixels, maskInsTotalPixelsInLine, shiftInsTotalPixelsInLine);
}

static bool SetAncInsTotalLines (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfLines)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFrameLines), numberOfLines, maskInsTotalLinesPerFrame, shiftInsTotalLinesPerFrame);
}

static bool SetAncInsFidHi (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldIDLines), lineNumber, maskInsFieldIDHigh, shiftInsFieldIDHigh);
}

static bool SetAncInsFidLow (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldIDLines), lineNumber, maskInsFieldIDLow, shiftInsFieldIDLow);
}

static bool SetAncInsRtpPayloadID (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t payloadID)
{
	return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsRtpPayloadID), payloadID);
}

static bool SetAncInsRtpSSRC (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t ssrc)
{
	return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsRtpSSRC), ssrc);
}

static bool SetAncInsIPChannel (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t channel)
{
	return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsIpChannel), channel);
}

static bool GetAncInsExtendedMode (CNTV2Card & inDevice, const UWord inSDIOutput, bool& extendedMode)
{
	bool		ok(true);
	uint32_t	regValue(0);
	extendedMode = false;
	if (ok) ok = inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), 1, maskInsExtendedMode, shiftInsExtendedMode);
	if (ok) ok = inDevice.ReadRegister(AncInsRegNum(inSDIOutput, regAncInsControl), regValue, maskInsExtendedMode, shiftInsExtendedMode);
	if (ok) extendedMode = (regValue == 1);
	return ok;
}

bool CNTV2Card::AncInsertInit (const UWord inSDIOutput, const NTV2Channel inChannel, const NTV2Standard inStandard)
{
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	NTV2Standard	theStandard	(inStandard);
	if (!NTV2_IS_VALID_STANDARD(theStandard))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetStandard(theStandard, theChannel))
			return false;
		if (!NTV2_IS_VALID_STANDARD(theStandard))
			return false;
	}

	const ANCInserterInitParams & initParams(inserterInitParamsTable[theStandard]);
	bool ok(true);
	bool extendedMode(false);
	if (ok) ok = GetAncInsExtendedMode (*this, inSDIOutput, extendedMode);
	if (ok)	ok = SetAncInsField1ActiveLine (*this, inSDIOutput, initParams.field1ActiveLine);
	if (ok)	ok = SetAncInsField2ActiveLine (*this, inSDIOutput, initParams.field2ActiveLine);
	if (ok)	ok = SetAncInsHActivePixels (*this, inSDIOutput, initParams.hActivePixels);
	if (ok)	ok = SetAncInsHTotalPixels (*this, inSDIOutput, initParams.hTotalPixels);
	if (ok)	ok = SetAncInsTotalLines (*this, inSDIOutput, initParams.totalLines);
	if (ok)	ok = SetAncInsFidLow (*this, inSDIOutput, extendedMode? initParams.field1SwitchLine : initParams.fidLow);
	if (ok)	ok = SetAncInsFidHi (*this, inSDIOutput, extendedMode? initParams.field2SwitchLine : initParams.fidHigh);
	if (ok)	ok = SetAncInsProgressive (*this, inSDIOutput, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	if (ok)	ok = SetAncInsSDPacketSplit (*this, inSDIOutput, NTV2_IS_SD_STANDARD(theStandard));
	if (ok)	ok = EnableAncInsHancC (*this, inSDIOutput, false);
	if (ok)	ok = EnableAncInsHancY (*this, inSDIOutput, false);
	if (ok)	ok = EnableAncInsVancC (*this, inSDIOutput, true);
	if (ok)	ok = EnableAncInsVancY (*this, inSDIOutput, true);
	if (ok)	ok = SetAncInsHancPixelDelay (*this, inSDIOutput, 0);
	if (ok)	ok = SetAncInsVancPixelDelay (*this, inSDIOutput, 0);
	if (ok)	ok = WriteRegister (AncInsRegNum(inSDIOutput, regAncInsBlankCStartLine), 0);
	if (ok)	ok = WriteRegister (AncInsRegNum(inSDIOutput, regAncInsBlankField1CLines), 0);
	if (ok)	ok = WriteRegister (AncInsRegNum(inSDIOutput, regAncInsBlankField2CLines), 0);
	if (ok)	ok = WriteRegister (AncInsRegNum(inSDIOutput, regAncInsPixelDelay), extendedMode? initParams.pixelDelay : 0);

	ULWord	field1Bytes(0);
	ULWord	field2Bytes(0);
	if (ok)	ok = GetAncField1Size(*this, field1Bytes);
	if (ok)	ok = GetAncField2Size(*this, field2Bytes);
	if (ok)	ok = SetAncInsField1Bytes (*this, inSDIOutput, field1Bytes);
	if (ok)	ok = SetAncInsField2Bytes (*this, inSDIOutput, field2Bytes);
	return ok;
}

bool CNTV2Card::AncInsertSetComponents (const UWord inSDIOutput,
										const bool inVancY, const bool inVancC,
										const bool inHancY, const bool inHancC)
{
	bool ok(true);
	bool extendedMode(false);
	if (ok)	ok = EnableAncInsVancY(*this, inSDIOutput, inVancY);
	if (ok)	ok = EnableAncInsVancC(*this, inSDIOutput, inVancC);
	if (ok) ok = GetAncInsExtendedMode (*this, inSDIOutput, extendedMode);
	if (extendedMode)
	{
		if (ok)	ok = EnableAncInsHancY(*this, inSDIOutput, inHancY);
		if (ok)	ok = EnableAncInsHancC(*this, inSDIOutput, inHancC);
	}
	return ok;
}

bool CNTV2Card::AncInsertSetEnable (const UWord inSDIOutput, const bool inIsEnabled)
{
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
		return false;
	bool ok(true);
    if (!inIsEnabled)
    {
        if (ok)	ok = EnableAncInsHancC(*this, inSDIOutput, false);
        if (ok)	ok = EnableAncInsHancY(*this, inSDIOutput, false);
        if (ok)	ok = EnableAncInsVancC(*this, inSDIOutput, false);
        if (ok)	ok = EnableAncInsVancY(*this, inSDIOutput, false);
    }
    if (ok)	ok = WriteRegister(AncInsRegNum(inSDIOutput, regAncInsBlankCStartLine), 0);
    if (ok)	ok = WriteRegister(AncInsRegNum(inSDIOutput, regAncInsBlankField1CLines), 0);
    if (ok)	ok = WriteRegister(AncInsRegNum(inSDIOutput, regAncInsBlankField2CLines), 0);
    if (ok)	ok = WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), inIsEnabled ? 0 : 1, maskInsDisableInserter, shiftInsDisableInserter);
    return ok;
}

bool CNTV2Card::AncInsertIsEnabled (const UWord inSDIOutput, bool & outIsRunning)
{
	outIsRunning = false;
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (inSDIOutput >= ::NTV2DeviceGetNumVideoOutputs(_boardID))
		return false;

	ULWord	value(0);
	if (!ReadRegister(AncInsRegNum(inSDIOutput, regAncInsControl), value))
		return false;
	outIsRunning = (value & BIT(28)) ? false : true;
	return true;
}

bool CNTV2Card::AncInsertSetReadParams (const UWord inSDIOutput, const ULWord inFrameNumber, const ULWord inF1Size,
										const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	NTV2Framesize	theFrameSize(inFrameSize);
	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetFrameBufferSize(theChannel, theFrameSize))
			return false;
		if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
			return false;
	}

	bool ok(true);
	//	Calculate where ANC Inserter will read the data
	const ULWord	frameNumber (inFrameNumber + 1);	//	Start at beginning of next frame (then subtract offset later)
	ULWord	frameLocation (::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		frameLocation *= 4;
	if (quadQuadEnabled)
		frameLocation *= 4;

	ULWord			F1Offset(0);
	if (ok)	ok = ReadRegister (kVRegAncField1Offset, F1Offset);
	const ULWord	ANCStartMemory (frameLocation - F1Offset);
	if (ok)	ok = SetAncInsField1StartAddr (*this, inSDIOutput, ANCStartMemory);
	if (ok)	ok = SetAncInsField1Bytes (*this, inSDIOutput, inF1Size);
	return ok;
}

bool CNTV2Card::AncInsertSetField2ReadParams (const UWord inSDIOutput, const ULWord inFrameNumber, const ULWord inF2Size,
												const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	NTV2Framesize	theFrameSize(inFrameSize);
	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetFrameBufferSize(theChannel, theFrameSize))
			return false;
		if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
			return false;
	}

	bool ok(true);
	//	Calculate where ANC Inserter will read the data
	const ULWord	frameNumber (inFrameNumber + 1);	//	Start at beginning of next frame (then subtract offset later)
	ULWord	frameLocation (::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		frameLocation *= 4;
	if (quadQuadEnabled)
		frameLocation *= 4;

	ULWord			F2Offset(0);
	if (ok)	ok = ReadRegister (kVRegAncField2Offset, F2Offset);
	const ULWord	ANCStartMemory (frameLocation - F2Offset);
	if (ok)	ok = SetAncInsField2StartAddr (*this, inSDIOutput, ANCStartMemory);
	if (ok)	ok = SetAncInsField2Bytes (*this, inSDIOutput, inF2Size);
	return ok;
}

bool CNTV2Card::AncInsertSetIPParams (const UWord inSDIOutput, const UWord ancChannel, const ULWord payloadID, const ULWord ssrc)
{
	bool ok(false);

	if (::NTV2DeviceCanDoIP(_boardID))
	{
		ok = SetAncInsIPChannel (*this, inSDIOutput, ancChannel);
		if (ok)	ok = SetAncInsRtpPayloadID (*this, inSDIOutput, payloadID);
		if (ok)	ok = SetAncInsRtpSSRC (*this, inSDIOutput, ssrc);
	}
	return ok;
}


/////////////////////////////////////////////
/////////////	ANC EXTRACTOR	/////////////
/////////////////////////////////////////////

static bool EnableAncExtHancY (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableHancY, shiftEnableHancY);
}

static bool EnableAncExtHancC (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableHancC, shiftEnableHancC);
}

static bool EnableAncExtVancY (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableVancY, shiftEnableVancY);
}

static bool EnableAncExtVancC (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableVancC, shiftEnableVancC);
}

static bool SetAncExtSDDemux (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableSDMux, shiftEnableSDMux);
}

static bool SetAncExtProgressive (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskSetProgressive, shiftSetProgressive);
}

static bool SetAncExtSynchro (CNTV2Card & inDevice, const UWord inSDIInput)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), 0x1, maskSyncro, shiftSyncro);
}

/* currently unused
static bool SetAncExtLSBEnable (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, (ULWord)maskGrabLSBs, shiftGrabLSBs);
}
*/

static bool SetAncExtField1StartAddr (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t addr)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField1StartAddress), addr);
}

static bool SetAncExtField1EndAddr (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t addr)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField1EndAddress), addr);
}

static bool SetAncExtField2StartAddr (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t addr)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField2StartAddress), addr);
}

static bool SetAncExtField2EndAddr (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t addr)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField2EndAddress), addr);
}

static bool SetAncExtField1CutoffLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFieldCutoffLine), lineNumber, maskField1CutoffLine, shiftField1CutoffLine);
}

static bool SetAncExtField2CutoffLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFieldCutoffLine), lineNumber, maskField2CutoffLine, shiftField2CutoffLine);
}

static bool GetAncExtField1Status (CNTV2DriverInterface & inDevice, const UWord inSDIInput, ULWord & outF1Status)
{
	return inDevice.ReadRegister(AncExtRegNum(inSDIInput, regAncExtField1Status), outF1Status);
}

static bool GetAncExtField2Status (CNTV2DriverInterface & inDevice, const UWord inSDIInput, ULWord & outF2Status)
{
	return inDevice.ReadRegister(AncExtRegNum(inSDIInput, regAncExtField2Status), outF2Status);
}

static bool IsAncExtOverrun (CNTV2DriverInterface & inDevice, const UWord inSDIInput, bool & outIsOverrun)
{
	return inDevice.ReadRegister(AncExtRegNum(inSDIInput, regAncExtTotalStatus), outIsOverrun, maskTotalOverrun, shiftTotalOverrun);
}

static bool SetAncExtField1StartLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFieldVBLStartLine), lineNumber, maskField1StartLine, shiftField1StartLine);
}

static bool SetAncExtField2StartLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFieldVBLStartLine), lineNumber, maskField2StartLine, shiftField2StartLine);
}

static bool SetAncExtTotalFrameLines (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t totalFrameLines)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtTotalFrameLines), totalFrameLines, maskTotalFrameLines, shiftTotalFrameLines);
}

static bool SetAncExtFidLow (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFID), lineNumber, maskFIDLow, shiftFIDLow);
}

static bool SetAncExtFidHi (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFID), lineNumber, maskFIDHi, shiftFIDHi);
}

static bool SetAncExtField1AnalogStartLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtAnalogStartLine), lineNumber, maskField1AnalogStartLine, shiftField1AnalogStartLine);
}

static bool SetAncExtField2AnalogStartLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtAnalogStartLine), lineNumber, maskField2AnalogStartLine, shiftField2AnalogStartLine);
}

static bool SetAncExtField1AnalogYFilter (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineFilter)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField1AnalogYFilter), lineFilter);
}

static bool SetAncExtField2AnalogYFilter (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineFilter)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField2AnalogYFilter), lineFilter);
}

static bool SetAncExtField1AnalogCFilter (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineFilter)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField1AnalogCFilter), lineFilter);
}

static bool SetAncExtField2AnalogCFilter (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineFilter)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField2AnalogCFilter), lineFilter);
}

static bool GetAncExtExtendedMode (CNTV2Card & inDevice, const UWord inSDIInput, bool& extendedMode)
{
	bool		ok(true);
	uint32_t	regValue(0);
	extendedMode = false;
	if (ok) ok = inDevice.WriteRegister(AncInsRegNum(inSDIInput, regAncInsControl), 1, maskInsExtendedMode, shiftInsExtendedMode);
	if (ok) ok = inDevice.ReadRegister(AncInsRegNum(inSDIInput, regAncInsControl), regValue, maskInsExtendedMode, shiftInsExtendedMode);
	if (ok) extendedMode = (regValue == 1);
	return ok;
}

bool CNTV2Card::AncExtractInit (const UWord inSDIInput, const NTV2Channel inChannel, const NTV2Standard inStandard)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIInput));
	NTV2Standard	theStandard	(inStandard);
	if (!NTV2_IS_VALID_STANDARD(theStandard))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetStandard(theStandard, theChannel))
			return false;
		if (!NTV2_IS_VALID_STANDARD(theStandard))
			return false;
	}

	const ANCExtractorInitParams &  extractorParams (extractorInitParamsTable[theStandard]);
	bool ok(true);
	bool extendedMode(false);
	if (ok) ok = GetAncExtExtendedMode (*this, inSDIInput, extendedMode);
	if (ok)	ok = SetAncExtProgressive (*this, inSDIInput, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	if (ok)	ok = SetAncExtField1StartLine (*this, inSDIInput, extractorParams.field1StartLine);
	if (ok)	ok = SetAncExtField1CutoffLine (*this, inSDIInput, extendedMode? extractorParams.field1SwitchLine : extractorParams.field1CutoffLine);
	if (ok)	ok = SetAncExtField2StartLine (*this, inSDIInput, extractorParams.field2StartLine);
	if (ok)	ok = SetAncExtField2CutoffLine (*this, inSDIInput, extendedMode? extractorParams.field2SwitchLine : extractorParams.field2CutoffLine);
	if (ok)	ok = SetAncExtTotalFrameLines (*this, inSDIInput, extractorParams.totalLines);
	if (ok)	ok = SetAncExtFidLow (*this, inSDIInput, extractorParams.fidLow);
	if (ok)	ok = SetAncExtFidHi (*this, inSDIInput, extractorParams.fidHigh);
	if (ok)	ok = SetAncExtField1AnalogStartLine (*this, inSDIInput, extractorParams.field1AnalogStartLine);
	if (ok)	ok = SetAncExtField2AnalogStartLine (*this, inSDIInput, extractorParams.field2AnalogStartLine);
	if (ok)	ok = SetAncExtField1AnalogYFilter (*this, inSDIInput, extractorParams.field1AnalogYFilter);
	if (ok)	ok = SetAncExtField2AnalogYFilter (*this, inSDIInput, extractorParams.field2AnalogYFilter);
	if (ok)	ok = SetAncExtField1AnalogCFilter (*this, inSDIInput, extractorParams.field1AnalogCFilter);
	if (ok)	ok = SetAncExtField2AnalogCFilter (*this, inSDIInput, extractorParams.field2AnalogCFilter);
	if (ok)	ok = AncExtractSetFilterDIDs (inSDIInput, AncExtractGetDefaultDIDs());
	if (ok)	ok = WriteRegister (AncExtRegNum(inSDIInput, regAncExtAnalogActiveLineLength), extractorParams.analogActiveLineLength);
	if (ok)	ok = SetAncExtSDDemux (*this, inSDIInput, NTV2_IS_SD_STANDARD(theStandard));
	if (ok)	ok = EnableAncExtHancC (*this, inSDIInput, true);
	if (ok)	ok = EnableAncExtHancY (*this, inSDIInput, true);
	if (ok)	ok = EnableAncExtVancC (*this, inSDIInput, true);
	if (ok)	ok = EnableAncExtVancY (*this, inSDIInput, true);
	if (ok)	ok = SetAncExtSynchro (*this, inSDIInput);
	if (ok)	ok = SetAncExtField1StartAddr (*this, inSDIInput, 0);
	if (ok)	ok = SetAncExtField1EndAddr (*this, inSDIInput, 0);
	if (ok)	ok = SetAncExtField2StartAddr (*this, inSDIInput, 0);
	if (ok)	ok = SetAncExtField2EndAddr (*this, inSDIInput, 0);
	return ok;
}

bool CNTV2Card::AncExtractSetComponents (const UWord inSDIInput,
											const bool inVancY, const bool inVancC,
											const bool inHancY, const bool inHancC)
{
	bool ok(true);
	if (ok)	ok = EnableAncExtVancY(*this, inSDIInput, inVancY);
	if (ok)	ok = EnableAncExtVancC(*this, inSDIInput, inVancC);
	if (ok)	ok = EnableAncExtHancY(*this, inSDIInput, inHancY);
	if (ok)	ok = EnableAncExtHancC(*this, inSDIInput, inHancC);
	return ok;
}

bool CNTV2Card::AncExtractSetEnable (const UWord inSDIInput, const bool inIsEnabled)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	bool ok(true);
	if (!inIsEnabled)
	{
		if (ok)	ok = EnableAncExtHancC (*this, inSDIInput, false);
		if (ok)	ok = EnableAncExtHancY (*this, inSDIInput, false);
		if (ok)	ok = EnableAncExtVancC (*this, inSDIInput, false);
		if (ok)	ok = EnableAncExtVancY (*this, inSDIInput, false);
	}
	if (ok)	ok = WriteRegister (AncExtRegNum(inSDIInput, regAncExtControl), inIsEnabled ? 0 : 1, maskDisableExtractor, shiftDisableExtractor);
	return ok;
}

bool CNTV2Card::AncExtractIsEnabled (const UWord inSDIInput, bool & outIsRunning)
{
	outIsRunning = false;
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	ULWord	value(0);
	if (!ReadRegister(AncExtRegNum(inSDIInput, regAncExtControl), value))
		return false;
	outIsRunning = (value & BIT(28)) ? false : true;
	return true;
}

bool CNTV2Card::AncExtractSetWriteParams (const UWord inSDIInput, const ULWord inFrameNumber,
											const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIInput));
	NTV2Framesize	theFrameSize(inFrameSize);
	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetFrameBufferSize(theChannel, theFrameSize))
			return false;
		if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
			return false;
	}
	if (IS_CHANNEL_INVALID(theChannel))
		return false;

	//	Calculate where ANC Extractor will put the data...
	bool			ok					(true);
	const ULWord	frameNumber			(inFrameNumber + 1);	//	This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord			frameLocation	(::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		frameLocation *= 4;
	if (quadQuadEnabled)
		frameLocation *= 4;

	ULWord F1Offset(0), F2Offset(0);
	if (ok)	ok = GetAncOffsets (*this, F1Offset, F2Offset);

	const ULWord	ANCStartMemory	(frameLocation - F1Offset);
	const ULWord	ANCStopMemory	(frameLocation - F2Offset - 1);
	if (ok)	ok = SetAncExtField1StartAddr (*this, inSDIInput, ANCStartMemory);
	if (ok)	ok = SetAncExtField1EndAddr (*this, inSDIInput, ANCStopMemory);
	return ok;
}

bool CNTV2Card::AncExtractSetField2WriteParams (const UWord inSDIInput, const ULWord inFrameNumber,
												const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIInput));
	NTV2Framesize	theFrameSize(inFrameSize);
	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetFrameBufferSize(theChannel, theFrameSize))
			return false;
		if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
			return false;
	}
	if (IS_CHANNEL_INVALID(theChannel))
		return false;

	//	Calculate where ANC Extractor will put the data...
	bool			ok					(true);
	const ULWord	frameNumber			(inFrameNumber + 1);	//	This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord			frameLocation	(::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		frameLocation *= 4;
	if (quadQuadEnabled)
		frameLocation *= 4;

	ULWord	F2Offset(0);
	if (ok)	ok = ReadRegister(kVRegAncField2Offset, F2Offset);

	const ULWord	ANCStartMemory	(frameLocation - F2Offset);
	const ULWord	ANCStopMemory	(frameLocation - 1);
	if (ok)	ok = SetAncExtField2StartAddr (*this, inSDIInput, ANCStartMemory);
	if (ok)	ok = SetAncExtField2EndAddr (*this, inSDIInput, ANCStopMemory);
    return true;
}

bool CNTV2Card::AncExtractGetFilterDIDs (const UWord inSDIInput, NTV2DIDSet & outDIDs)
{
	outDIDs.clear();
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	const ULWord	firstIgnoreRegNum	(AncExtRegNum(inSDIInput, regAncExtIgnorePktsReg_First));
	for (ULWord regNdx(0);  regNdx < kNumDIDRegisters;  regNdx++)
	{
		ULWord	regValue	(0);
		ReadRegister (firstIgnoreRegNum + regNdx,  regValue);
		for (unsigned regByte(0);  regByte < 4;  regByte++)
		{
			const NTV2DID	theDID	((regValue >> (regByte*8)) & 0x000000FF);
			if (theDID)
				outDIDs.insert(theDID);
		}
	}
	return true;
}

bool CNTV2Card::AncExtractSetFilterDIDs (const UWord inSDIInput, const NTV2DIDSet & inDIDs)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	const ULWord		firstIgnoreRegNum	(AncExtRegNum(inSDIInput, regAncExtIgnorePktsReg_First));
	NTV2DIDSetConstIter iter				(inDIDs.begin());

	for (ULWord regNdx(0);  regNdx < kNumDIDRegisters;  regNdx++)
	{
		ULWord	regValue	(0);
		for (unsigned regByte(0);  regByte < 4;  regByte++)
		{
			const NTV2DID	theDID	(iter != inDIDs.end()  ?  *iter++  :  0);
			regValue |= (ULWord(theDID) << (regByte*8));
		}
		WriteRegister (firstIgnoreRegNum + regNdx,  regValue);
	}
	return true;
}

bool CNTV2Card::AncExtractGetField1Size (const UWord inSDIInput, ULWord & outF1Size)
{
	outF1Size = 0;
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	bool	ok			(true);
	ULWord	regValue	(0);

	ok = GetAncExtField1Status(*this, inSDIInput, regValue);
	if (!ok || ((regValue & maskField1Overrun) != 0))
		return false;
	outF1Size = regValue & maskField1BytesIn;

	return ok;
}

bool CNTV2Card::AncExtractGetField2Size (const UWord inSDIInput, ULWord & outF2Size)
{
	outF2Size = 0;
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	bool	ok			(true);
	ULWord	regValue	(0);

	ok = GetAncExtField2Status(*this, inSDIInput, regValue);
	if (!ok || ((regValue & maskField2Overrun) != 0))
		return false;
	outF2Size = regValue & maskField2BytesIn;

	return ok;
}

bool CNTV2Card::AncExtractGetBufferOverrun (const UWord inSDIInput, bool & outIsOverrun, const UWord inField)
{
	outIsOverrun = false;
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;
	if (inField > 2)
		return false;
	ULWord status(0);
	switch (inField)
	{
		case 0:	return IsAncExtOverrun (*this, inSDIInput, outIsOverrun);
		case 1:	if (!GetAncExtField1Status(*this, inSDIInput, status))
					return false;
				outIsOverrun = (status & maskField1Overrun) ? true : false;
				return true;
		case 2:	if (!GetAncExtField2Status(*this, inSDIInput, status))
					return false;
				outIsOverrun = (status & maskField2Overrun) ? true : false;
				return true;
		default: break;
	}
	return false;
}



/////////////////////////////////////////////////
/////////////	STATIC FUNCTIONS	/////////////
/////////////////////////////////////////////////

UWord CNTV2Card::AncExtractGetMaxNumFilterDIDs (void)
{
	static const ULWord	kNumDIDsPerRegister	(4);
	return UWord(kNumDIDsPerRegister * kNumDIDRegisters);
}


NTV2DIDSet CNTV2Card::AncExtractGetDefaultDIDs (const bool inHDAudio)
{
	//													SMPTE299 HD Aud Grp 1-4		SMPTE299 HD Aud Ctrl Grp 1-4
	static const NTV2DID	sDefaultHDDIDs[]	=	{	0xE7,0xE6,0xE5,0xE4,		0xE3,0xE2,0xE1,0xE0,
	//													SMPTE299 HD Aud Grp 5-8		SMPTE299 HD Aud Ctrl Grp 5-8
														0xA7,0xA6,0xA5,0xA4,		0xA3,0xA2,0xA1,0xA0,	0x00};

	//													SMPTE272 SD Aud Grp 1-4		SMPTE272 SD Aud Ext Grp 1-4
	static const NTV2DID	sDefaultSDDIDs[]	=	{	0xFF,0xFD,0xFB,0xF9,		0xFE,0xFC,0xFA,0xF8,
	//													SMPTE272 SD Aud Ctrl Grp 1-4
														0xEF,0xEE,0xED,0xEC,		0x00};
	NTV2DIDSet	result;
	const NTV2DID *	pDIDArray (inHDAudio ? sDefaultHDDIDs : sDefaultSDDIDs);
	for (unsigned ndx(0);  pDIDArray[ndx];  ndx++)
		result.insert(pDIDArray[ndx]);

	return result;
}
