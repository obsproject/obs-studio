/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2enhancedcsc.cpp
	@brief		Implementation of CNTV2EnhancedCSC class.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.
**/

#include "ntv2publicinterface.h"
#include "ntv2enhancedcsc.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"


static const ULWord	gChannelToEnhancedCSCRegNum []		= {	kRegEnhancedCSC1Mode,	kRegEnhancedCSC2Mode,	kRegEnhancedCSC3Mode,	kRegEnhancedCSC4Mode,
															kRegEnhancedCSC5Mode,	kRegEnhancedCSC6Mode,	kRegEnhancedCSC7Mode,	kRegEnhancedCSC8Mode,	0};

static const double kTwo24th (double(1 << 24));


static ULWord ConvertCoeffDoubleToULWord (const double inCoefficient)
{
	return ULWord(inCoefficient * kTwo24th);
}


static double ConvertCoeffULWordToDouble (const ULWord inCoefficient)
{
	return double(inCoefficient) / kTwo24th;
}


bool CNTV2EnhancedCSC::SetInputPixelFormat (const NTV2EnhancedCSCPixelFormat inPixelFormat)
{
	switch (inPixelFormat)
	{
		case NTV2_Enhanced_CSC_Pixel_Format_RGB444:
		case NTV2_Enhanced_CSC_Pixel_Format_YCbCr444:
		case NTV2_Enhanced_CSC_Pixel_Format_YCbCr422:
			mInputPixelFormat = inPixelFormat;
			break;
		default:
			return false;
	}
	return true;
}	//	SetInputPixelFormat


bool CNTV2EnhancedCSC::SetOutputPixelFormat (const NTV2EnhancedCSCPixelFormat inPixelFormat)
{
	switch (inPixelFormat)
	{
		case NTV2_Enhanced_CSC_Pixel_Format_RGB444:
		case NTV2_Enhanced_CSC_Pixel_Format_YCbCr444:
		case NTV2_Enhanced_CSC_Pixel_Format_YCbCr422:
			mOutputPixelFormat = inPixelFormat;
			break;
		default:
			return false;
	}
	return true;
}	//	SetOutputPixelFormat


bool CNTV2EnhancedCSC::SetChromaFilterSelect (const NTV2EnhancedCSCChromaFilterSelect inChromaFilterSelect)
{
	switch (inChromaFilterSelect)
	{
		case NTV2_Enhanced_CSC_Chroma_Filter_Select_Full:
		case NTV2_Enhanced_CSC_Chroma_Filter_Select_Simple:
		case NTV2_Enhanced_CSC_Chroma_Filter_Select_None:
			mChromaFilterSelect = inChromaFilterSelect;
			break;
		default:
			return false;
	}
	return true;
}	//	SetChromaFilterSelect


bool CNTV2EnhancedCSC::SetChromaEdgeControl (const NTV2EnhancedCSCChromaEdgeControl inChromaEdgeControl)
{
	switch (inChromaEdgeControl)
	{
		case NTV2_Enhanced_CSC_Chroma_Edge_Control_Black:
		case NTV2_Enhanced_CSC_Chroma_Edge_Control_Extended:
			mChromaEdgeControl = inChromaEdgeControl;
			break;
		default:
			return false;
	}
	return true;
}	//	SetChromaEdgeControl


bool CNTV2EnhancedCSC::SetKeySource (const NTV2EnhancedCSCKeySource inKeySource)
{
	switch (inKeySource)
	{
		case NTV2_Enhanced_CSC_Key_Source_Key_Input:
		case NTV2_Enhanced_CSC_Key_Spurce_A0_Input:
			mKeySource = inKeySource;
			break;
		default:
			return false;
	}
	return true;
}	//	SetKeySource


bool CNTV2EnhancedCSC::SetKeyOutputRange (const NTV2EnhancedCSCKeyOutputRange inKeyOutputRange)
{
	switch (inKeyOutputRange)
	{
		case NTV2_Enhanced_CSC_Key_Output_Range_Full:
		case NTV2_Enhanced_CSC_Key_Output_Range_SMPTE:
			mKeyOutputRange = inKeyOutputRange;
			break;
		default:
			return false;
	}
	return true;
}	//	SetKeyOutputRange


bool CNTV2EnhancedCSC::SetKeyInputOffset (const int16_t inKeyInputOffset)
{
	//	To do: add some parameter checking
	mKeyInputOffset = inKeyInputOffset;
	return true;
}	//	SetKeyInputOffset


bool CNTV2EnhancedCSC::SetKeyOutputOffset (const uint16_t inKeyOutputOffset)
{
	//	To do: add some parameter checking
	mKeyOutputOffset = inKeyOutputOffset;
	return true;
}	//	SetKeyOutputOffset


bool CNTV2EnhancedCSC::SetKeyGain (const double inKeyGain)
{
	//	To do: add some parameter checking
	mKeyGain = inKeyGain;
	return true;
}	//	SetKeyGain


bool CNTV2EnhancedCSC::SendToHardware (CNTV2Card & inDevice, const NTV2Channel inChannel) const
{
	if (!::NTV2DeviceCanDoEnhancedCSC(inDevice.GetDeviceID()))
		return false;	//	No Enhanced CSCs
	if (!inDevice.IsOpen ())
		return false;	//	Device not open
	if (UWord(inChannel) >= ::NTV2DeviceGetNumCSCs(inDevice.GetDeviceID()))
		return false;	//	Bad CSC index

	const ULWord cscBaseRegNum(gChannelToEnhancedCSCRegNum[inChannel]);
	ULWord cscRegs [kRegNumEnhancedCSCRegisters];

	//	Read-modify-write only the relevent control bits
	if (!inDevice.ReadRegister (cscBaseRegNum, cscRegs[0]))
		return false;

	cscRegs [0] = (cscRegs[0] & ~kK2RegMaskEnhancedCSCInputPixelFormat)   | ULWord(mInputPixelFormat   << kK2RegShiftEnhancedCSCInputPixelFormat);
	cscRegs [0] = (cscRegs[0] & ~kK2RegMaskEnhancedCSCOutputPixelFormat)  | ULWord(mOutputPixelFormat  << kK2RegShiftEnhancedCSCOutputPixelFormat);
	cscRegs [0] = (cscRegs[0] & ~kK2RegMaskEnhancedCSCChromaFilterSelect) | ULWord(mChromaFilterSelect << kK2RegShiftEnhancedCSCChromaFilterSelect);
	cscRegs [0] = (cscRegs[0] & ~kK2RegMaskEnhancedCSCChromaEdgeControl)  | ULWord(mChromaEdgeControl  << kK2RegShiftEnhancedCSCChromaEdgeControl);

    cscRegs [1] = (ULWord(Matrix().GetOffset(NTV2CSCOffsetIndex_Pre1)) << 16) | (ULWord(Matrix().GetOffset(NTV2CSCOffsetIndex_Pre0)&0xFFFF));

	cscRegs [2] = ULWord(Matrix().GetOffset(NTV2CSCOffsetIndex_Pre2));

	cscRegs [3] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_A0));
	cscRegs [4] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_A1));
	cscRegs [5] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_A2));
	cscRegs [6] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_B0));
	cscRegs [7] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_B1));
	cscRegs [8] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_B2));
	cscRegs [9] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_C0));
	cscRegs [10] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_C1));
	cscRegs [11] = ConvertCoeffDoubleToULWord (Matrix().GetCoefficient (NTV2CSCCoeffIndex_C2));
    cscRegs [12] = (ULWord(Matrix().GetOffset (NTV2CSCOffsetIndex_PostB)) << 16) | (ULWord(Matrix().GetOffset (NTV2CSCOffsetIndex_PostA)&0xFFFF));
	cscRegs [13] = ULWord(Matrix().GetOffset (NTV2CSCOffsetIndex_PostC));
	cscRegs [14] = ULWord((mKeyOutputRange  & kK2RegMaskEnhancedCSCKeyOutputRange)  << kK2RegShiftEnhancedCSCKeyOutputRange) |
				   ULWord((mKeySource       & kK2RegMaskEnhancedCSCKeySource)       << kK2RegShiftEnhancedCSCKeySource);
	cscRegs [15] = (ULWord(mKeyOutputOffset) << 16) | ULWord(mKeyInputOffset);
	cscRegs [16] = (ULWord(mKeyGain * 4096.0));

	NTV2RegInfo regInfo;
	NTV2RegWrites regs;
	regs.reserve(size_t(kRegNumEnhancedCSCRegisters));
	for (ULWord num(0);  num < kRegNumEnhancedCSCRegisters;  num++)
		regs.push_back(NTV2RegInfo(cscBaseRegNum+num, cscRegs[num]));

	return inDevice.WriteRegisters(regs);

}	//	SendToHardware


bool CNTV2EnhancedCSC::GetFromHardware (CNTV2Card & inDevice, const NTV2Channel inChannel)
{
	if (!::NTV2DeviceCanDoEnhancedCSC(inDevice.GetDeviceID()))
		return false;	//	No Enhanced CSCs
	if (!inDevice.IsOpen ())
		return false;	//	Device not open
	if (UWord(inChannel) >= ::NTV2DeviceGetNumCSCs(inDevice.GetDeviceID()))
		return false;	//	Bad CSC index

	const ULWord cscBaseRegNum(gChannelToEnhancedCSCRegNum[inChannel]);
	NTV2RegReads regs;
	regs.reserve(size_t(kRegNumEnhancedCSCRegisters));
	for (ULWord ndx(0);  ndx < kRegNumEnhancedCSCRegisters;  ndx++)
		regs.push_back(NTV2RegInfo(cscBaseRegNum + ndx));

	if (!inDevice.ReadRegisters(regs))
		return false;	//	Read failure

	//	Check that all registers were read successfully
	if (regs.size() != size_t(kRegNumEnhancedCSCRegisters))
		return false;	//	Read failure

	mInputPixelFormat	=        NTV2EnhancedCSCPixelFormat ((regs[0].registerValue & kK2RegMaskEnhancedCSCInputPixelFormat)   >> kK2RegShiftEnhancedCSCInputPixelFormat);
	mOutputPixelFormat	=        NTV2EnhancedCSCPixelFormat	((regs[0].registerValue & kK2RegMaskEnhancedCSCOutputPixelFormat)  >> kK2RegShiftEnhancedCSCOutputPixelFormat);
	mChromaFilterSelect	= NTV2EnhancedCSCChromaFilterSelect	((regs[0].registerValue & kK2RegMaskEnhancedCSCChromaFilterSelect) >> kK2RegShiftEnhancedCSCChromaFilterSelect);
	mChromaEdgeControl	=  NTV2EnhancedCSCChromaEdgeControl	((regs[0].registerValue & kK2RegMaskEnhancedCSCChromaEdgeControl)  >> kK2RegShiftEnhancedCSCChromaEdgeControl);

	Matrix().SetOffset (NTV2CSCOffsetIndex_Pre0, int16_t(regs[1].registerValue & 0xFFFF));
	Matrix().SetOffset (NTV2CSCOffsetIndex_Pre1, int16_t(regs[1].registerValue >> 16));
	Matrix().SetOffset (NTV2CSCOffsetIndex_Pre2, int16_t(regs[2].registerValue & 0xFFFF));

	Matrix().SetCoefficient (NTV2CSCCoeffIndex_A0, ConvertCoeffULWordToDouble(regs[3].registerValue));
	Matrix().SetCoefficient (NTV2CSCCoeffIndex_A1, ConvertCoeffULWordToDouble(regs[4].registerValue));
	Matrix().SetCoefficient (NTV2CSCCoeffIndex_A2, ConvertCoeffULWordToDouble(regs[5].registerValue));

	Matrix().SetCoefficient (NTV2CSCCoeffIndex_B0, ConvertCoeffULWordToDouble(regs[6].registerValue));
	Matrix().SetCoefficient (NTV2CSCCoeffIndex_B1, ConvertCoeffULWordToDouble(regs[7].registerValue));
	Matrix().SetCoefficient (NTV2CSCCoeffIndex_B2, ConvertCoeffULWordToDouble(regs[8].registerValue));

	Matrix().SetCoefficient (NTV2CSCCoeffIndex_C0, ConvertCoeffULWordToDouble(regs[9].registerValue));
	Matrix().SetCoefficient (NTV2CSCCoeffIndex_C1, ConvertCoeffULWordToDouble(regs[10].registerValue));
	Matrix().SetCoefficient (NTV2CSCCoeffIndex_C2, ConvertCoeffULWordToDouble(regs[11].registerValue));

	Matrix().SetOffset (NTV2CSCOffsetIndex_PostA, int16_t(regs[12].registerValue & 0xFFFF));
	Matrix().SetOffset (NTV2CSCOffsetIndex_PostB, int16_t(regs[12].registerValue >> 16));
	Matrix().SetOffset (NTV2CSCOffsetIndex_PostC, int16_t(regs[13].registerValue & 0xFFFF));

	mKeyOutputRange	= NTV2EnhancedCSCKeyOutputRange	((regs[14].registerValue & kK2RegMaskEnhancedCSCKeyOutputRange)	>> kK2RegShiftEnhancedCSCKeyOutputRange);
	mKeySource		=      NTV2EnhancedCSCKeySource ((regs[14].registerValue & kK2RegMaskEnhancedCSCKeySource)		>> kK2RegShiftEnhancedCSCKeySource);

	mKeyInputOffset	=  int16_t(regs[15].registerValue & 0xFFFF);
	mKeyOutputOffset= uint16_t(regs[15].registerValue >> 16);
	mKeyGain = (double(regs[16].registerValue) / 4096.0);
	return true;

}	//	GetFromaHardware
