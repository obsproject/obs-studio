/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2registerexpert.cpp
	@brief		Implements the CNTV2RegisterExpert class.
	@copyright	(C) 2016-2021 AJA Video Systems, Inc.
 **/
#include "ntv2registerexpert.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2debug.h"
#include "ntv2endian.h"
#include "ntv2vpid.h"
#include "ajabase/common/common.h"
#include "ajabase/system/lock.h"
#include "ajabase/common/ajarefptr.h"
#include <algorithm>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <map>
#include "math.h"


using namespace std;

#define	LOGGING_MAPPINGS	(AJADebug::IsActive(AJA_DebugUnit_Enumeration))
#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') << uint64_t(__x__)  << dec
#define INSTP(_p_)			HEX16(uint64_t(_p_))
#define	REiFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_Enumeration, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	REiWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Enumeration, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	REiNOTE(__x__)		AJA_sNOTICE	(AJA_DebugUnit_Enumeration, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	REiINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_Enumeration, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	REiDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_Enumeration, INSTP(this) << "::" << AJAFUNC << ": " << __x__)


#if defined (NTV2_DEPRECATE)
	#define	AJA_LOCAL_STATIC	static
#else	//	!defined (NTV2_DEPRECATE)
	#define	AJA_LOCAL_STATIC
#endif	//	!defined (NTV2_DEPRECATE)


static const string	gChlClasses[8]	=	{	kRegClass_Channel1,	kRegClass_Channel2,	kRegClass_Channel3,	kRegClass_Channel4,
											kRegClass_Channel5,	kRegClass_Channel6,	kRegClass_Channel7,	kRegClass_Channel8	};
static const string sSpace(" ");
static const string sNull;


class RegisterExpert;
typedef AJARefPtr<RegisterExpert>	RegisterExpertPtr;
static uint32_t						gInstanceTally(0);
static uint32_t						gLivingInstances(0);


/**
	I'm the the root source of register information. I provide answers to the public-facing CNTV2RegisterExpert class.
	There's only one instance of me.
	TODO:	Need to handle multi-register sparse bits.
			Search for MULTIREG_SPARSE_BITS -- it's where we need to improve how we present related information that's
			stored in more than one register.
**/
class RegisterExpert
{
public:
	static RegisterExpertPtr	GetInstance(const bool inCreateIfNecessary = true);
	static bool					DisposeInstance(void);

private:
	RegisterExpert()
	{
		AJAAutoLock	lock(&mGuardMutex);
		AJAAtomic::Increment(&gInstanceTally);
		AJAAtomic::Increment(&gLivingInstances);
		//	Name "Classic" registers using NTV2RegisterNameString...
		for (ULWord regNum (0);  regNum < kRegNumRegisters;  regNum++)
			DefineRegName (regNum,	::NTV2RegisterNameString(regNum));
		//	Now the rest...
		SetupBasicRegs();		//	Basic registers
		SetupVPIDRegs();		//	VPIDs
		SetupAncInsExt();		//	Anc Ins/Ext
		SetupXptSelect();		//	Xpt Select
		SetupDMARegs();			//	DMA
		SetupTimecodeRegs();	//	Timecode
		SetupAudioRegs();		//	Audio
		SetupMixerKeyerRegs();	//	Mixer/Keyer
		SetupHDMIRegs();		//	HDMI
		SetupSDIErrorRegs();	//	SDIError
		SetupCSCRegs();			//	CSCs
		SetupLUTRegs();			//	LUTs
		SetupVRegs();			//	Virtuals
		REiNOTE(DEC(gLivingInstances) << " extant, " << DEC(gInstanceTally) << " total");
		if (LOGGING_MAPPINGS)
		{
			REiDBG("RegsToStrsMap=" << mRegNumToStringMap.size()
					<< " RegsToDecodersMap=" << mRegNumToDecoderMap.size()
					<< " ClassToRegsMMap=" << mRegClassToRegNumMMap.size()
					<< " StrToRegsMMap=" << mStringToRegNumMMap.size()
					<< " InpXptsToXptRegInfoMap=" << mInputXpt2XptRegNumMaskIndexMap.size()
					<< " XptRegInfoToInpXptsMap=" << mXptRegNumMaskIndex2InputXptMap.size()
					<< " RegClasses=" << mAllRegClasses.size());
		}
	}	//	constructor
public:
	~RegisterExpert()
	{
		AJAAtomic::Decrement(&gLivingInstances);
		REiNOTE(DEC(gLivingInstances) << " extant, " << DEC(gInstanceTally) << " total");
	}	//	destructor

private:
	//	This class implements a functor that returns a string that contains a human-readable decoding
	//	of a register value, given its number and the ID of the device it came from.
	struct Decoder
	{
		//	The default reg decoder functor returns an empty string.
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inRegValue;
			(void) inDeviceID;
			return string();
		}
		virtual	~Decoder()	{}
	} mDefaultRegDecoder;

    void DefineRegName(const uint32_t regNumber, const string & regName)
    {
        if (!regName.empty())
        {
			AJAAutoLock	lock(&mGuardMutex);
            if (mRegNumToStringMap.find(regNumber) == mRegNumToStringMap.end())
            {
                mRegNumToStringMap.insert (RegNumToStringPair(regNumber, regName));
                string lowerCaseRegName(regName);
                mStringToRegNumMMap.insert (StringToRegNumPair(aja::lower(lowerCaseRegName), regNumber));
            }
        }
    }
	inline void	DefineRegDecoder(const uint32_t inRegNum, const Decoder & dec)
	{
		AJAAutoLock	lock(&mGuardMutex);
		mRegNumToDecoderMap.insert (RegNumToDecoderPair(inRegNum, &dec));
	}
	inline void	DefineRegClass (const uint32_t inRegNum, const string & className)
	{
		if (!className.empty())
		{
			AJAAutoLock	lock(&mGuardMutex);
			mRegClassToRegNumMMap.insert(StringToRegNumPair(className, inRegNum));
		}
	}
	void DefineRegReadWrite(const uint32_t inRegNum, const int rdWrt)
	{
		AJAAutoLock	lock(&mGuardMutex);
		if (rdWrt == READONLY)
		{
			NTV2_ASSERT (!IsRegisterWriteOnly(inRegNum));
			DefineRegClass (inRegNum, kRegClass_ReadOnly);
		}
		if (rdWrt == WRITEONLY)
		{
			NTV2_ASSERT (!IsRegisterReadOnly(inRegNum));
			DefineRegClass (inRegNum, kRegClass_WriteOnly);
		}
	}
	void DefineRegister(const uint32_t inRegNum, const string & regName, const Decoder & dec, const int rdWrt, const string & className1, const string & className2, const string & className3)
	{
		DefineRegName (inRegNum, regName);
		DefineRegDecoder (inRegNum, dec);
		DefineRegReadWrite (inRegNum, rdWrt);
		DefineRegClass (inRegNum, className1);
		DefineRegClass (inRegNum, className2);
		DefineRegClass (inRegNum, className3);
	}
	void DefineXptReg(const uint32_t inRegNum, const NTV2InputXptID xpt0, const NTV2InputXptID xpt1, const NTV2InputXptID xpt2, const NTV2InputXptID xpt3)
	{
		DefineRegister (inRegNum, sNull,	mDecodeXptGroupReg,	READWRITE,	kRegClass_Routing,	kRegClass_NULL,	kRegClass_NULL);
		const NTV2InputCrosspointID	indexes	[4]	= {xpt0, xpt1, xpt2, xpt3};
		for (int ndx(0);  ndx < 4;  ndx++)
		{
			if (indexes[ndx] == NTV2_INPUT_CROSSPOINT_INVALID)
				continue;
			const XptRegNumAndMaskIndex regNumAndNdx(inRegNum, ndx);
			if (mXptRegNumMaskIndex2InputXptMap.find(regNumAndNdx) == mXptRegNumMaskIndex2InputXptMap.end())
				mXptRegNumMaskIndex2InputXptMap [regNumAndNdx] = indexes[ndx];
			if (mInputXpt2XptRegNumMaskIndexMap.find(indexes[ndx]) == mInputXpt2XptRegNumMaskIndexMap.end())
				mInputXpt2XptRegNumMaskIndexMap[indexes[ndx]] = regNumAndNdx;
		}
	}

	void SetupBasicRegs(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		DefineRegister (kRegGlobalControl,		"",	mDecodeGlobalControlReg,	READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegGlobalControl2,		"",	mDecodeGlobalControl2,		READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegGlobalControl3,		"",	mDecodeGlobalControl3,		READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegGlobalControlCh2,	"",	mDecodeGlobalControlChanRegs,READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegGlobalControlCh3,	"",	mDecodeGlobalControlChanRegs,READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegGlobalControlCh4,	"",	mDecodeGlobalControlChanRegs,READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegGlobalControlCh5,	"",	mDecodeGlobalControlChanRegs,READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegGlobalControlCh6,	"",	mDecodeGlobalControlChanRegs,READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegGlobalControlCh7,	"",	mDecodeGlobalControlChanRegs,READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegGlobalControlCh8,	"",	mDecodeGlobalControlChanRegs,READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegCh1Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegCh2Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegCh3Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegCh4Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegCh5Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegCh6Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegCh7Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegCh8Control,			"",	mDecodeChannelControl,		READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegCh1PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegCh2PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegCh3PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegCh4PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegCh5PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegCh6PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegCh7PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegCh8PCIAccessFrame,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegCh1InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegCh2InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegCh3InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegCh4InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegCh5InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegCh6InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegCh7InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegCh8InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegCh1OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegCh2OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegCh3OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegCh4OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegCh5OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegCh6OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegCh7OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegCh8OutputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegSDIOut1Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegSDIOut2Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegSDIOut3Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegSDIOut4Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegSDIOut5Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegSDIOut6Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegSDIOut7Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegSDIOut8Control,		"",	mDecodeSDIOutputControl,	READWRITE,	kRegClass_Output,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegCh1ControlExtended,	"",	mDecodeChannelControlExt,	READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegCh2ControlExtended,	"",	mDecodeChannelControlExt,	READWRITE,	kRegClass_NULL,		kRegClass_Channel2,	kRegClass_NULL);

		DefineRegister (kRegCanDoStatus,		"",	mDecodeCanDoStatus,			READONLY,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);
		DefineRegister (kRegBitfileDate,		"",	mDecodeBitfileDateTime,		READONLY,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);
		DefineRegister (kRegBitfileTime,		"",	mDecodeBitfileDateTime,		READONLY,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);
		DefineRegister (kRegCPLDVersion,		"",	mDecodeCPLDVersion,			READWRITE,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);

		DefineRegister (kRegVidIntControl,		"",	mDecodeVidIntControl,		READWRITE,	kRegClass_Interrupt,		kRegClass_Channel1,	kRegClass_Channel2);
			DefineRegClass (kRegVidIntControl, kRegClass_Channel3);
			DefineRegClass (kRegVidIntControl, kRegClass_Channel4);
		DefineRegister (kRegStatus,				"",	mDecodeStatusReg,			READWRITE,	kRegClass_Interrupt,		kRegClass_Channel1,	kRegClass_Channel2);
			DefineRegClass (kRegStatus, kRegClass_Timecode);
		DefineRegister (kRegVidIntControl2,		"",	mDecodeVidIntControl2,		READWRITE,	kRegClass_Interrupt,		kRegClass_Channel5,	kRegClass_Channel5);
			DefineRegClass (kRegVidIntControl2, kRegClass_Channel7);
			DefineRegClass (kRegVidIntControl2, kRegClass_Channel8);
		DefineRegister (kRegStatus2,			"",	mDecodeStatus2Reg,			READWRITE,	kRegClass_Interrupt,		kRegClass_Channel3,	kRegClass_Channel4);
			DefineRegClass (kRegStatus2, kRegClass_Channel5);
			DefineRegClass (kRegStatus2, kRegClass_Channel6);
			DefineRegClass (kRegStatus2, kRegClass_Channel7);
			DefineRegClass (kRegStatus2, kRegClass_Channel8);
		DefineRegister (kRegInputStatus,		"",	mDecodeInputStatusReg,		READONLY,	kRegClass_Input,	kRegClass_Channel1,	kRegClass_Channel2);
			DefineRegClass (kRegInputStatus, kRegClass_Audio);
		DefineRegister (kRegSDIInput3GStatus,	"",	mDecodeSDIInputStatusReg,	READWRITE,	kRegClass_Input,	kRegClass_Channel1,	kRegClass_Channel2);
		DefineRegister (kRegSDIInput3GStatus2,	"",	mDecodeSDIInputStatusReg,	READWRITE,	kRegClass_Input,	kRegClass_Channel3,	kRegClass_Channel4);
		DefineRegister (kRegSDI5678Input3GStatus,"",mDecodeSDIInputStatusReg,	READWRITE,	kRegClass_Input,	kRegClass_Channel5,	kRegClass_Channel6);
			DefineRegClass (kRegSDI5678Input3GStatus, kRegClass_Channel7);
			DefineRegClass (kRegSDI5678Input3GStatus, kRegClass_Channel8);
		DefineRegister (kRegInputStatus2,		"",	mDecodeSDIInputStatus2Reg,	READONLY,	kRegClass_Input,	kRegClass_Channel3,	kRegClass_Channel4);	//	288
		DefineRegister (kRegInput56Status,		"",	mDecodeSDIInputStatus2Reg,	READONLY,	kRegClass_Input,	kRegClass_Channel5,	kRegClass_Channel6);	//	458
		DefineRegister (kRegInput78Status,		"",	mDecodeSDIInputStatus2Reg,	READONLY,	kRegClass_Input,	kRegClass_Channel7,	kRegClass_Channel8);	//	459

		DefineRegister (kRegFS1ReferenceSelect,	"", mDecodeFS1RefSelectReg,		READWRITE,	kRegClass_Input,	kRegClass_Timecode, kRegClass_NULL);
		DefineRegister (kRegSysmonVccIntDieTemp,"",	mDecodeSysmonVccIntDieTemp,	READONLY,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);
		DefineRegister (kRegSDITransmitControl,	"",	mDecodeSDITransmitCtrl,		READWRITE,	kRegClass_Channel1,	kRegClass_Channel2,	kRegClass_Channel3);
			DefineRegClass (kRegSDITransmitControl, kRegClass_Channel4);
			DefineRegClass (kRegSDITransmitControl, kRegClass_Channel5);
			DefineRegClass (kRegSDITransmitControl, kRegClass_Channel6);
			DefineRegClass (kRegSDITransmitControl, kRegClass_Channel7);
			DefineRegClass (kRegSDITransmitControl, kRegClass_Channel8);
		DefineRegister (kRegCh1InputFrame,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_NULL);

		DefineRegister (kRegConversionControl,	"",	mConvControlRegDecoder,		READWRITE,	kRegClass_NULL,		kRegClass_Channel1,	kRegClass_Channel2);
		DefineRegister (kRegSDIWatchdogControlStatus,"", mDecodeRelayCtrlStat,	READWRITE,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);
		DefineRegister (kRegSDIWatchdogTimeout,	"",	mDecodeWatchdogTimeout,		READWRITE,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);
		DefineRegister (kRegSDIWatchdogKick1,	"",	mDecodeWatchdogKick,		READWRITE,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);
		DefineRegister (kRegSDIWatchdogKick2,	"",	mDecodeWatchdogKick,		READWRITE,	kRegClass_NULL,		kRegClass_NULL,		kRegClass_NULL);
	}
	void SetupVPIDRegs(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		DefineRegister (kRegSDIIn1VPIDA,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (kRegSDIIn1VPIDB,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (kRegSDIOut1VPIDA,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegSDIOut1VPIDB,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegSDIOut2VPIDA,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegSDIOut2VPIDB,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegSDIIn2VPIDA,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (kRegSDIIn2VPIDB,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (kRegSDIOut3VPIDA,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel3);
		DefineRegister (kRegSDIOut3VPIDB,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel3);
		DefineRegister (kRegSDIOut4VPIDA,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel4);
		DefineRegister (kRegSDIOut4VPIDB,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel4);
		DefineRegister (kRegSDIIn3VPIDA,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (kRegSDIIn3VPIDB,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (kRegSDIIn4VPIDA,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (kRegSDIIn4VPIDB,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (kRegSDIOut5VPIDA,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel5);
		DefineRegister (kRegSDIOut5VPIDB,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel5);
		DefineRegister (kRegSDIIn5VPIDA,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel5);
		DefineRegister (kRegSDIIn5VPIDB,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel5);
		DefineRegister (kRegSDIIn6VPIDA,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel6);
		DefineRegister (kRegSDIIn6VPIDB,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel6);
		DefineRegister (kRegSDIOut6VPIDA,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel6);
		DefineRegister (kRegSDIOut6VPIDB,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel6);
		DefineRegister (kRegSDIIn7VPIDA,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel7);
		DefineRegister (kRegSDIIn7VPIDB,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel7);
		DefineRegister (kRegSDIOut7VPIDA,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel7);
		DefineRegister (kRegSDIOut7VPIDB,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel7);
		DefineRegister (kRegSDIIn8VPIDA,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel8);
		DefineRegister (kRegSDIIn8VPIDB,		"",	mVPIDInpRegDecoder,			READONLY,	kRegClass_VPID,		kRegClass_Input,	kRegClass_Channel8);
		DefineRegister (kRegSDIOut8VPIDA,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel8);
		DefineRegister (kRegSDIOut8VPIDB,		"",	mVPIDOutRegDecoder,			READWRITE,	kRegClass_VPID,		kRegClass_Output,	kRegClass_Channel8);
	}
	void SetupTimecodeRegs(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		DefineRegister	(kRegRP188InOut1DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut1Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut1Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut2DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut2Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut2Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister	(kRegLTCOutBits0_31,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_Output);
		DefineRegister	(kRegLTCOutBits32_63,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_Output);
		DefineRegister	(kRegLTCInBits0_31,				"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_Input);
		DefineRegister	(kRegLTCInBits32_63,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_Input);
		DefineRegister	(kRegRP188InOut1Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut1Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut2Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut2Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut3Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut3Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut4Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut4Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut5Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut5Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut6Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut6Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut7Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut7Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut8Bits0_31_2,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut8Bits32_63_2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister	(kRegLTCStatusControl,			"",	mLTCStatusControlDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_NULL,		kRegClass_NULL);
		DefineRegister	(kRegLTC2EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister	(kRegLTC2EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister	(kRegLTC2AnalogBits0_31,		"",	mDefaultRegDecoder,			READONLY,	kRegClass_Timecode,	kRegClass_NULL,		kRegClass_NULL);
		DefineRegister	(kRegLTC2AnalogBits32_63,		"",	mDefaultRegDecoder,			READONLY,	kRegClass_Timecode,	kRegClass_NULL,		kRegClass_NULL);
		DefineRegister	(kRegRP188InOut3DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut3Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut3Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut4DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut4Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut4Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister	(kRegLTC3EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister	(kRegLTC3EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister	(kRegLTC4EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister	(kRegLTC4EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut5Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut5Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut5DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister	(kRegLTC5EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister	(kRegLTC5EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut6Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut6Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut6DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister	(kRegLTC6EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister	(kRegLTC6EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut7Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut7Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut7DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister	(kRegLTC7EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister	(kRegLTC7EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut8Bits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut8Bits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister	(kRegRP188InOut8DBB,			"",	mRP188InOutDBBRegDecoder,	READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister	(kRegLTC8EmbeddedBits0_31,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister	(kRegLTC8EmbeddedBits32_63,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Timecode,	kRegClass_Channel8,	kRegClass_NULL);
	}	//	SetupTimecodeRegs
	
	void SetupAudioRegs(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		DefineRegister (kRegAud1Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegAud2Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegAud3Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegAud4Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegAud5Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegAud6Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegAud7Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegAud8Control,		"",	mDecodeAudControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegAud1Detect,			"",	mDecodeAudDetectReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_Channel2);
		DefineRegister (kRegAudDetect2,			"",	mDecodeAudDetectReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_Channel4);
		DefineRegister (kRegAudioDetect5678,	"",	mDecodeAudDetectReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegAud1SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegAud2SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegAud3SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegAud4SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegAud5SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegAud6SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegAud7SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegAud8SourceSelect,	"",	mDecodeAudSourceSelectReg,	READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegAud1Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister (kRegAud2Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_NULL);
		DefineRegister (kRegAud3Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_NULL);
		DefineRegister (kRegAud4Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_NULL);
		DefineRegister (kRegAud5Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_NULL);
		DefineRegister (kRegAud6Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_NULL);
		DefineRegister (kRegAud7Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_NULL);
		DefineRegister (kRegAud8Delay,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_NULL);
		DefineRegister (kRegAud1OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_Output);
		DefineRegister (kRegAud2OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_Output);
		DefineRegister (kRegAud3OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_Output);
		DefineRegister (kRegAud4OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_Output);
		DefineRegister (kRegAud5OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_Output);
		DefineRegister (kRegAud6OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_Output);
		DefineRegister (kRegAud7OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_Output);
		DefineRegister (kRegAud8OutputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_Output);
		DefineRegister (kRegAud1InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_Input);
		DefineRegister (kRegAud2InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel2,	kRegClass_Input);
		DefineRegister (kRegAud3InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel3,	kRegClass_Input);
		DefineRegister (kRegAud4InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel4,	kRegClass_Input);
		DefineRegister (kRegAud5InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_Input);
		DefineRegister (kRegAud6InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel6,	kRegClass_Input);
		DefineRegister (kRegAud7InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel7,	kRegClass_Input);
		DefineRegister (kRegAud8InputLastAddr,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Audio,	kRegClass_Channel8,	kRegClass_Input);
		DefineRegister (kRegPCMControl4321,		"",	mDecodePCMControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel1,	kRegClass_Channel2);
			DefineRegClass (kRegPCMControl4321, kRegClass_Channel3);
			DefineRegClass (kRegPCMControl4321, kRegClass_Channel4);
		DefineRegister (kRegPCMControl8765,		"",	mDecodePCMControlReg,		READWRITE,	kRegClass_Audio,	kRegClass_Channel5,	kRegClass_Channel6);
			DefineRegClass (kRegPCMControl8765, kRegClass_Channel7);
			DefineRegClass (kRegPCMControl8765, kRegClass_Channel8);
		DefineRegister (kRegAud1Counter,		"",	mDefaultRegDecoder,			READONLY,	kRegClass_Audio,	kRegClass_NULL,		kRegClass_NULL);
		DefineRegister (kRegAudioOutputSourceMap,"",mDecodeAudOutputSrcMap,		READWRITE,	kRegClass_Audio,	kRegClass_Output,	kRegClass_AES);
			DefineRegClass (kRegAudioOutputSourceMap, kRegClass_HDMI);

		DefineRegister (kRegAudioMixerInputSelects,				"kRegAudioMixerInputSelects",				mAudMxrInputSelDecoder,	READWRITE,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainGain,					"kRegAudioMixerMainGain",					mAudMxrGainDecoder,		READWRITE,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerAux1GainCh1,				"kRegAudioMixerAux1GainCh1",				mAudMxrGainDecoder,		READWRITE,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerAux2GainCh1,				"kRegAudioMixerAux2GainCh1",				mAudMxrGainDecoder,		READWRITE,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerChannelSelect,			"kRegAudioMixerChannelSelect",				mAudMxrChanSelDecoder,	READWRITE,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMutes,					"kRegAudioMixerMutes",						mAudMxrMutesDecoder,	READWRITE,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerAux1GainCh2,				"kRegAudioMixerAux1GainCh2",				mAudMxrGainDecoder,		READWRITE,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerAux2GainCh2,				"kRegAudioMixerAux2GainCh2",				mAudMxrGainDecoder,		READWRITE,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerAux1InputLevels,			"kRegAudioMixerAux1InputLevels",			mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerAux2InputLevels,			"kRegAudioMixerAux2InputLevels",			mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainInputLevelsPair0,		"kRegAudioMixerMainInputLevelsPair0",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainInputLevelsPair1,		"kRegAudioMixerMainInputLevelsPair1",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainInputLevelsPair2,		"kRegAudioMixerMainInputLevelsPair2",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainInputLevelsPair3,		"kRegAudioMixerMainInputLevelsPair3",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainInputLevelsPair4,		"kRegAudioMixerMainInputLevelsPair4",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainInputLevelsPair5,		"kRegAudioMixerMainInputLevelsPair5",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainInputLevelsPair6,		"kRegAudioMixerMainInputLevelsPair6",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainInputLevelsPair7,		"kRegAudioMixerMainInputLevelsPair7",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainOutputLevelsPair0,	"kRegAudioMixerMainOutputLevelsPair0",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainOutputLevelsPair1,	"kRegAudioMixerMainOutputLevelsPair1",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainOutputLevelsPair2,	"kRegAudioMixerMainOutputLevelsPair2",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainOutputLevelsPair3,	"kRegAudioMixerMainOutputLevelsPair3",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainOutputLevelsPair4,	"kRegAudioMixerMainOutputLevelsPair4",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainOutputLevelsPair5,	"kRegAudioMixerMainOutputLevelsPair5",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainOutputLevelsPair6,	"kRegAudioMixerMainOutputLevelsPair6",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister (kRegAudioMixerMainOutputLevelsPair7,	"kRegAudioMixerMainOutputLevelsPair7",		mAudMxrLevelDecoder,	READONLY,	kRegClass_Audio,	kRegClass_NULL,	kRegClass_NULL);
	}
	
	void SetupDMARegs(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		DefineRegister	(kRegDMA1HostAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA1HostAddrHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA1LocalAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA1XferCount,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA1NextDesc,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA1NextDescHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA2HostAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA2HostAddrHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA2LocalAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA2XferCount,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA2NextDesc,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA2NextDescHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA3HostAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA3HostAddrHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA3LocalAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA3XferCount,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA3NextDesc,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA3NextDescHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA4HostAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA4HostAddrHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA4LocalAddr,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA4XferCount,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA4NextDesc,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMA4NextDescHigh,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMAControl,		"",	mDMAControlRegDecoder,		READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegDMAIntControl,		"",	mDMAIntControlRegDecoder,	READWRITE,	kRegClass_DMA,	kRegClass_NULL,	kRegClass_NULL);
	}
	
	void SetupXptSelect(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		//				RegNum					0-7								8-15							16-23							24-31
		DefineXptReg	(kRegXptSelectGroup1,	NTV2_XptLUT1Input,				NTV2_XptCSC1VidInput,			NTV2_XptConversionModInput,		NTV2_XptCompressionModInput);
		DefineXptReg	(kRegXptSelectGroup2,	NTV2_XptFrameBuffer1Input,		NTV2_XptFrameSync1Input,		NTV2_XptFrameSync2Input,		NTV2_XptDualLinkOut1Input);
		DefineXptReg	(kRegXptSelectGroup3,	NTV2_XptAnalogOutInput,			NTV2_XptSDIOut1Input,			NTV2_XptSDIOut2Input,			NTV2_XptCSC1KeyInput);
		DefineXptReg	(kRegXptSelectGroup4,	NTV2_XptMixer1FGVidInput,		NTV2_XptMixer1FGKeyInput,		NTV2_XptMixer1BGVidInput,		NTV2_XptMixer1BGKeyInput);
		DefineXptReg	(kRegXptSelectGroup5,	NTV2_XptFrameBuffer2Input,		NTV2_XptLUT2Input,				NTV2_XptCSC2VidInput,			NTV2_XptCSC2KeyInput);
		DefineXptReg	(kRegXptSelectGroup6,	NTV2_XptWaterMarker1Input,		NTV2_XptIICT1Input,				NTV2_XptHDMIOutInput,			NTV2_XptOEInput);
		{	//	An additional input Xpt for kRegXptSelectGroup6 in mask index 2...
			const XptRegNumAndMaskIndex regNumAndNdx (kRegXptSelectGroup6, 2);
			if (mXptRegNumMaskIndex2InputXptMap.find (regNumAndNdx) == mXptRegNumMaskIndex2InputXptMap.end())
				mXptRegNumMaskIndex2InputXptMap [regNumAndNdx] = NTV2_XptHDMIOutQ1Input;
			if (mInputXpt2XptRegNumMaskIndexMap.find (NTV2_XptHDMIOutQ1Input) == mInputXpt2XptRegNumMaskIndexMap.end())
				mInputXpt2XptRegNumMaskIndexMap[NTV2_XptHDMIOutQ1Input] = regNumAndNdx;
		}
		DefineXptReg	(kRegXptSelectGroup7,	NTV2_XptWaterMarker2Input,		NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_XptDualLinkOut2Input,		NTV2_INPUT_CROSSPOINT_INVALID);
		DefineXptReg	(kRegXptSelectGroup8,	NTV2_XptSDIOut3Input,			NTV2_XptSDIOut4Input,			NTV2_XptSDIOut5Input,			NTV2_INPUT_CROSSPOINT_INVALID);
		DefineXptReg	(kRegXptSelectGroup9,	NTV2_XptMixer2FGVidInput,		NTV2_XptMixer2FGKeyInput,		NTV2_XptMixer2BGVidInput,		NTV2_XptMixer2BGKeyInput);
		DefineXptReg	(kRegXptSelectGroup10,	NTV2_XptSDIOut1InputDS2,		NTV2_XptSDIOut2InputDS2,		NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_INPUT_CROSSPOINT_INVALID);
		DefineXptReg	(kRegXptSelectGroup11,	NTV2_XptDualLinkIn1Input,		NTV2_XptDualLinkIn1DSInput,		NTV2_XptDualLinkIn2Input,		NTV2_XptDualLinkIn2DSInput);
		DefineXptReg	(kRegXptSelectGroup12,	NTV2_XptLUT3Input,				NTV2_XptLUT4Input,				NTV2_XptLUT5Input,				NTV2_Xpt3DLUT1Input);
		DefineXptReg	(kRegXptSelectGroup13,	NTV2_XptFrameBuffer3Input,		NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_XptFrameBuffer4Input,		NTV2_INPUT_CROSSPOINT_INVALID);
		DefineXptReg	(kRegXptSelectGroup14,	NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_XptSDIOut3InputDS2,		NTV2_XptSDIOut5InputDS2,		NTV2_XptSDIOut4InputDS2);
		DefineXptReg	(kRegXptSelectGroup15,	NTV2_XptDualLinkIn3Input,		NTV2_XptDualLinkIn3DSInput,		NTV2_XptDualLinkIn4Input,		NTV2_XptDualLinkIn4DSInput);
		DefineXptReg	(kRegXptSelectGroup16,	NTV2_XptDualLinkOut3Input,		NTV2_XptDualLinkOut4Input,		NTV2_XptDualLinkOut5Input,		NTV2_INPUT_CROSSPOINT_INVALID);
		DefineXptReg	(kRegXptSelectGroup17,	NTV2_XptCSC3VidInput,			NTV2_XptCSC3KeyInput,			NTV2_XptCSC4VidInput,			NTV2_XptCSC4KeyInput);
		DefineXptReg	(kRegXptSelectGroup18,	NTV2_XptCSC5VidInput,			NTV2_XptCSC5KeyInput,			NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_INPUT_CROSSPOINT_INVALID);
		DefineXptReg	(kRegXptSelectGroup19,	NTV2_Xpt4KDCQ1Input,			NTV2_Xpt4KDCQ2Input,			NTV2_Xpt4KDCQ3Input,			NTV2_Xpt4KDCQ4Input);
		DefineXptReg	(kRegXptSelectGroup20,	NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_XptHDMIOutQ2Input,			NTV2_XptHDMIOutQ3Input,			NTV2_XptHDMIOutQ4Input);
		DefineXptReg	(kRegXptSelectGroup21,	NTV2_XptFrameBuffer5Input,		NTV2_XptFrameBuffer6Input,		NTV2_XptFrameBuffer7Input,		NTV2_XptFrameBuffer8Input);
		DefineXptReg	(kRegXptSelectGroup22,	NTV2_XptSDIOut6Input,			NTV2_XptSDIOut6InputDS2,		NTV2_XptSDIOut7Input,			NTV2_XptSDIOut7InputDS2);
		DefineXptReg	(kRegXptSelectGroup23,	NTV2_XptCSC7VidInput,			NTV2_XptCSC7KeyInput,			NTV2_XptCSC8VidInput,			NTV2_XptCSC8KeyInput);
		DefineXptReg	(kRegXptSelectGroup24,	NTV2_XptLUT6Input,				NTV2_XptLUT7Input,				NTV2_XptLUT8Input,				NTV2_INPUT_CROSSPOINT_INVALID);
		DefineXptReg	(kRegXptSelectGroup25,	NTV2_XptDualLinkIn5Input,		NTV2_XptDualLinkIn5DSInput,		NTV2_XptDualLinkIn6Input,		NTV2_XptDualLinkIn6DSInput);
		DefineXptReg	(kRegXptSelectGroup26,	NTV2_XptDualLinkIn7Input,		NTV2_XptDualLinkIn7DSInput,		NTV2_XptDualLinkIn8Input,		NTV2_XptDualLinkIn8DSInput);
		DefineXptReg	(kRegXptSelectGroup27,	NTV2_XptDualLinkOut6Input,		NTV2_XptDualLinkOut7Input,		NTV2_XptDualLinkOut8Input,		NTV2_INPUT_CROSSPOINT_INVALID);
		DefineXptReg	(kRegXptSelectGroup28,	NTV2_XptMixer3FGVidInput,		NTV2_XptMixer3FGKeyInput,		NTV2_XptMixer3BGVidInput,		NTV2_XptMixer3BGKeyInput);
		DefineXptReg	(kRegXptSelectGroup29,	NTV2_XptMixer4FGVidInput,		NTV2_XptMixer4FGKeyInput,		NTV2_XptMixer4BGVidInput,		NTV2_XptMixer4BGKeyInput);
		DefineXptReg	(kRegXptSelectGroup30,	NTV2_XptSDIOut8Input,			NTV2_XptSDIOut8InputDS2,		NTV2_XptCSC6VidInput,			NTV2_XptCSC6KeyInput);
		DefineXptReg	(kRegXptSelectGroup32,	NTV2_Xpt425Mux1AInput,			NTV2_Xpt425Mux1BInput,			NTV2_Xpt425Mux2AInput,			NTV2_Xpt425Mux2BInput);
		DefineXptReg	(kRegXptSelectGroup33,	NTV2_Xpt425Mux3AInput,			NTV2_Xpt425Mux3BInput,			NTV2_Xpt425Mux4AInput,			NTV2_Xpt425Mux4BInput);
		DefineXptReg	(kRegXptSelectGroup34,	NTV2_XptFrameBuffer1BInput,		NTV2_XptFrameBuffer2BInput,		NTV2_XptFrameBuffer3BInput,		NTV2_XptFrameBuffer4BInput);
		DefineXptReg	(kRegXptSelectGroup35,	NTV2_XptFrameBuffer5BInput,		NTV2_XptFrameBuffer6BInput,		NTV2_XptFrameBuffer7BInput,		NTV2_XptFrameBuffer8BInput);
		DefineXptReg	(kRegXptSelectGroup36,	NTV2_XptMultiLinkOut1Input,		NTV2_XptMultiLinkOut1InputDS2,	NTV2_INPUT_CROSSPOINT_INVALID,	NTV2_INPUT_CROSSPOINT_INVALID);
		

		//	Expose the CanConnect ROM registers:
		for (ULWord regNum(kRegFirstValidXptROMRegister);  regNum < ULWord(kRegInvalidValidXptROMRegister);  regNum++)
		{	ostringstream regName;	//	used to synthesize reg name
			const ULWord rawInputXpt	((regNum - ULWord(kRegFirstValidXptROMRegister)) / 4UL + ULWord(NTV2_FIRST_INPUT_CROSSPOINT));
			const ULWord ndx			((regNum - ULWord(kRegFirstValidXptROMRegister)) % 4UL);
			const NTV2InputXptID inputXpt	(static_cast<NTV2InputXptID>(rawInputXpt));
			if (NTV2_IS_VALID_InputCrosspointID(inputXpt))
			{
				string inputXptEnumName	(::NTV2InputCrosspointIDToString(inputXpt,false));	//	e.g. "NTV2_XptFrameBuffer1Input"
				if (inputXptEnumName.empty())
					regName << "kRegXptValid" << DEC0N(rawInputXpt,3) << "N" << DEC(ndx);
				else
					regName << "kRegXptValid" << aja::replace(inputXptEnumName, "NTV2_Xpt", "") << DEC(ndx);
			}
			else
				regName << "kRegXptValue" << HEX0N(regNum,4);
			DefineRegister (regNum, regName.str(),	mDecodeXptValidReg,	READONLY,	kRegClass_NULL,	kRegClass_NULL,	kRegClass_NULL);
		}
	}	//	SetupXptSelect
	
	void SetupAncInsExt(void)
	{
		static const string	AncExtRegNames []	=	{	"Control",				"F1 Start Address",		"F1 End Address",
														"F2 Start Address",		"F2 End Address",		"Field Cutoff Lines",
														"Memory Total",			"F1 Memory Usage",		"F2 Memory Usage",
														"V Blank Lines",		"Lines Per Frame",		"Field ID Lines",
														"Ignore DID 1-4",		"Ignore DID 5-8",		"Ignore DID 9-12",
														"Ignore DID 13-16",		"Ignore DID 17-20",		"Analog Start Line",
														"Analog F1 Y Filter",	"Analog F2 Y Filter",	"Analog F1 C Filter",
														"Analog F2 C Filter"	"",						"",
														"",						"",						"",
														"Analog Act Line Len",	""};
		static const string	AncInsRegNames []	=	{	"Field Bytes",			"Control",				"F1 Start Address",
														"F2 Start Address",		"Pixel Delay",			"Active Start",
														"Pixels Per Line",		"Lines Per Frame",		"Field ID Lines",
														"Payload ID Control",	"Payload ID",			"Chroma Blank Lines",
														"F1 C Blanking Mask",	"F2 C Blanking Mask",	"Field Bytes High",
														"Reserved 15",			"RTP Payload ID",		"RTP SSRC",
														"IP Channel"};
		static const uint32_t	AncExtPerChlRegBase []	=	{	0x1000,	0x1040,	0x1080,	0x10C0,	0x1100,	0x1140,	0x1180,	0x11C0	};
		static const uint32_t	AncInsPerChlRegBase []	=	{	0x1200,	0x1240,	0x1280,	0x12C0,	0x1300,	0x1340,	0x1380,	0x13C0	};
		
		NTV2_ASSERT(sizeof(AncExtRegNames[0]) == sizeof(AncExtRegNames[1]));
		NTV2_ASSERT(size_t(regAncExt_LAST) == sizeof(AncExtRegNames)/sizeof(AncExtRegNames[0]));
		NTV2_ASSERT(size_t(regAncIns_LAST) == sizeof(AncInsRegNames)/sizeof(string));

		AJAAutoLock	lock(&mGuardMutex);
		for (ULWord offsetNdx (0);  offsetNdx < 8;  offsetNdx++)
		{
			for (ULWord reg(regAncExtControl);  reg < regAncExt_LAST;  reg++)
			{
				if (AncExtRegNames[reg].empty())	continue;
				ostringstream	oss;	oss << "Extract " << (offsetNdx+1) << " " << AncExtRegNames[reg];
				DefineRegName (AncExtPerChlRegBase[offsetNdx] + reg,	oss.str());
			}
			for (ULWord reg(regAncInsFieldBytes);  reg < regAncIns_LAST;  reg++)
			{
				ostringstream	oss;	oss << "Insert " << (offsetNdx+1) << " " << AncInsRegNames[reg];
				DefineRegName (AncInsPerChlRegBase[offsetNdx] + reg,	oss.str());
			}
		}
		for (ULWord ndx (0);  ndx < 8;  ndx++)
		{
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtControl,						"",	mDecodeAncExtControlReg,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1StartAddress,			"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1EndAddress,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2StartAddress,			"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2EndAddress,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtFieldCutoffLine,				"",	mDecodeAncExtFieldLines,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtTotalStatus,					"",	mDecodeAncExtStatus,			READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1Status,					"",	mDecodeAncExtStatus,			READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2Status,					"",	mDecodeAncExtStatus,			READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtFieldVBLStartLine,				"",	mDecodeAncExtFieldLines,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtTotalFrameLines,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtFID,							"",	mDecodeAncExtFieldLines,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_1_2_3_4,		"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_5_6_7_8,		"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_9_10_11_12,	"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_13_14_15_16,	"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtIgnorePacketReg_17_18_19_20,	"",	mDecodeAncExtIgnoreDIDs,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtAnalogStartLine,				"",	mDecodeAncExtFieldLines,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1AnalogYFilter,			"",	mDecodeAncExtAnalogFilter,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2AnalogYFilter,			"",	mDecodeAncExtAnalogFilter,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField1AnalogCFilter,			"",	mDecodeAncExtAnalogFilter,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtField2AnalogCFilter,			"",	mDecodeAncExtAnalogFilter,		READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			DefineRegister (AncExtPerChlRegBase [ndx] + regAncExtAnalogActiveLineLength,		"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Input,	gChlClasses[ndx]);
			
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsFieldBytes,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsControl,						"",	mDecodeAncInsControlReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsField1StartAddr,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsField2StartAddr,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsPixelDelay,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsActiveStart,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsLinePixels,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsFrameLines,					"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsFieldIDLines,					"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsPayloadIDControl,				"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsPayloadID,						"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsBlankCStartLine,				"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsBlankField1CLines,				"",	mDecodeAncInsChromaBlankReg,	READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsBlankField2CLines,				"",	mDecodeAncInsChromaBlankReg,	READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsFieldBytesHigh,				"",	mDecodeAncInsValuePairReg,		READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsRtpPayloadID,					"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsRtpSSRC,						"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
			DefineRegister (AncInsPerChlRegBase [ndx] + regAncInsIpChannel,						"",	mDefaultRegDecoder,				READWRITE,	kRegClass_Anc,	kRegClass_Output,	gChlClasses[ndx]);
		}
	}	//	SetupAncInsExt

	void SetupHDMIRegs(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		DefineRegister (kRegHDMIOutControl,							"",	mDecodeHDMIOutputControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegHDMIInputStatus,						"",	mDecodeHDMIInputStatus,		READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (kRegHDMIInputControl,						"",	mDecodeHDMIInputControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (kRegHDMIHDRGreenPrimary,					"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
		DefineRegister (kRegHDMIHDRBluePrimary,						"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
		DefineRegister (kRegHDMIHDRRedPrimary,						"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
		DefineRegister (kRegHDMIHDRWhitePoint,						"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
		DefineRegister (kRegHDMIHDRMasteringLuminence,				"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
		DefineRegister (kRegHDMIHDRLightLevel,						"",	mDecodeHDMIOutHDRPrimary,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
		DefineRegister (kRegHDMIHDRControl,							"",	mDecodeHDMIOutHDRControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_HDR);
		DefineRegister (kRegMRQ1Control,							"",	mDecodeHDMIOutMRControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegMRQ2Control,							"",	mDecodeHDMIOutMRControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegMRQ3Control,							"",	mDecodeHDMIOutMRControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegMRQ4Control,							"",	mDecodeHDMIOutMRControl,	READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (kRegHDMIV2I2C1Control,						"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2I2C1Data,							"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2VideoSetup,						"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2HSyncDurationAndBackPorch,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2HActive,							"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2VSyncDurationAndBackPorchField1,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2VSyncDurationAndBackPorchField2,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2VActiveField1,					"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2VActiveField2,					"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2VideoStatus,						"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2HorizontalMeasurements,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2HBlankingMeasurements,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2HBlankingMeasurements1,			"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2VerticalMeasurementsField0,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2VerticalMeasurementsField1,		"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2i2c2Control,						"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);
		DefineRegister (kRegHDMIV2i2c2Data,							"",	mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_NULL);

		DefineRegister (0x1d00,	"reg_hdmiin4_videocontrol",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d01,	"reg_hdmiin4_videodetect0",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d02,	"reg_hdmiin4_videodetect1",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d03,	"reg_hdmiin4_videodetect2",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d04,	"reg_hdmiin4_videodetect3",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d05,	"reg_hdmiin4_videodetect4",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d06,	"reg_hdmiin4_videodetect5",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d07,	"reg_hdmiin4_videodetect6",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d08,	"reg_hdmiin4_videodetect7",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d09,	"reg_hdmiin4_auxcontrol",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d0a,	"reg_hdmiin4_receiverstatus",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d0b,	"reg_hdmiin4_auxpacketignore0",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d0c,	"reg_hdmiin4_auxpacketignore1",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d0d,	"reg_hdmiin4_auxpacketignore2",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d0e,	"reg_hdmiin4_auxpacketignore3",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d0f,	"reg_hdmiin4_redrivercontrol",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d10,	"reg_hdmiin4_refclockfrequency",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d11,	"reg_hdmiin4_tmdsclockfrequency",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d12,	"reg_hdmiin4_rxclockfrequency",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d13,	"reg_hdmiin4_rxoversampling",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d14,	"reg_hdmiin4_output_config",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d15,	"reg_hdmiin4_input_status",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d16,	"reg_hdmiin4_control",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d1e,	"reg_hdmiin4_croplocation",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);
		DefineRegister (0x1d1f,	"reg_hdmiin4_pixelcontrol",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel1);

		DefineRegister (0x2500,	"reg_hdmiin4_videocontrol",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2501,	"reg_hdmiin4_videodetect0",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2502,	"reg_hdmiin4_videodetect1",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2503,	"reg_hdmiin4_videodetect2",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2504,	"reg_hdmiin4_videodetect3",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2505,	"reg_hdmiin4_videodetect4",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2506,	"reg_hdmiin4_videodetect5",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2507,	"reg_hdmiin4_videodetect6",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2508,	"reg_hdmiin4_videodetect7",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2509,	"reg_hdmiin4_auxcontrol",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x250a,	"reg_hdmiin4_receiverstatus",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x250b,	"reg_hdmiin4_auxpacketignore0",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x250c,	"reg_hdmiin4_auxpacketignore1",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x250d,	"reg_hdmiin4_auxpacketignore2",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x250e,	"reg_hdmiin4_auxpacketignore3",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x250f,	"reg_hdmiin4_redrivercontrol",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2510,	"reg_hdmiin4_refclockfrequency",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2511,	"reg_hdmiin4_tmdsclockfrequency",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2512,	"reg_hdmiin4_rxclockfrequency",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2513,	"reg_hdmiin4_rxoversampling",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2514,	"reg_hdmiin4_output_config",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2515,	"reg_hdmiin4_input_status",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x2516,	"reg_hdmiin4_control",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x251e,	"reg_hdmiin4_croplocation",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);
		DefineRegister (0x251f,	"reg_hdmiin4_pixelcontrol",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel2);

		DefineRegister (0x2c00,	"reg_hdmiin_i2c_control",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c01,	"reg_hdmiin_i2c_data",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c02,	"reg_hdmiin_video_setup",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c03,	"reg_hdmiin_hsync_duration",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c04,	"reg_hdmiin_h_active",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c05,	"reg_hdmiin_vsync_duration_fld1",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c06,	"reg_hdmiin_vsync_duration_fld2",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c07,	"reg_hdmiin_v_active_fld1",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c08,	"reg_hdmiin_v_active_fld2",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c09,	"reg_hdmiin_video_status",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c0a,	"reg_hdmiin_horizontal_data",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c0b,	"reg_hdmiin_hblank_data0",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c0c,	"reg_hdmiin_hblank_data1",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c0d,	"reg_hdmiin_vertical_data_fld1",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c0e,	"reg_hdmiin_vertical_data_fld2",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c0f,	"reg_hdmiin_color_depth",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c12,	"reg_hdmiin_output_config",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c13,	"reg_hdmiin_input_status",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);
		DefineRegister (0x2c14,	"reg_hdmiin_control",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel3);

		DefineRegister (0x3000,	"reg_hdmiin_i2c_control",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3001,	"reg_hdmiin_i2c_data",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3002,	"reg_hdmiin_video_setup",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3003,	"reg_hdmiin_hsync_duration",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3004,	"reg_hdmiin_h_active",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3005,	"reg_hdmiin_vsync_duration_fld1",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3006,	"reg_hdmiin_vsync_duration_fld2",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3007,	"reg_hdmiin_v_active_fld1",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3008,	"reg_hdmiin_v_active_fld2",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3009,	"reg_hdmiin_video_status",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x300a,	"reg_hdmiin_horizontal_data",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x300b,	"reg_hdmiin_hblank_data0",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x300c,	"reg_hdmiin_hblank_data1",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x300d,	"reg_hdmiin_vertical_data_fld1",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x300e,	"reg_hdmiin_vertical_data_fld2",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x300f,	"reg_hdmiin_color_depth",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3012,	"reg_hdmiin_output_config",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3013,	"reg_hdmiin_input_status",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);
		DefineRegister (0x3014,	"reg_hdmiin_control",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Input,	kRegClass_Channel4);

		DefineRegister (0x1d40,	"reg_hdmiout4_videocontrol",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d41,	"reg_hdmiout4_videosetup0",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d42,	"reg_hdmiout4_videosetup1",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d43,	"reg_hdmiout4_videosetup2",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d44,	"reg_hdmiout4_videosetup3",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d45,	"reg_hdmiout4_videosetup4",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d46,	"reg_hdmiout4_videosetup5",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d47,	"reg_hdmiout4_videosetup6",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d48,	"reg_hdmiout4_videosetup7",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d49,	"reg_hdmiout4_auxcontrol",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d4b,	"reg_hdmiout4_audiocontrol",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d4f,	"reg_hdmiout4_redrivercontrol",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d50,	"reg_hdmiout4_refclockfrequency",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d51,	"reg_hdmiout4_tmdsclockfrequency",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d52,	"reg_hdmiout4_txclockfrequency",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d53,	"reg_hdmiout4_fpllclockfrequency",		mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d54,	"reg_hdmiout4_audio_cts1",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d55,	"reg_hdmiout4_audio_cts2",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d56,	"reg_hdmiout4_audio_cts3",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d57,	"reg_hdmiout4_audio_cts4",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d58,	"reg_hdmiout4_audio_n",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d5e,	"reg_hdmiout4_croplocation",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d5f,	"reg_hdmiout4_pixelcontrol",			mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d60,	"reg_hdmiout4_i2ccontrol",				mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
		DefineRegister (0x1d61,	"reg_hdmiout4_i2cedid",					mDefaultRegDecoder,			READWRITE,	kRegClass_HDMI,		kRegClass_Output,	kRegClass_Channel1);
	}

	void SetupSDIErrorRegs(void)
	{
		static const ULWord	baseNum[]	=	{kRegRXSDI1Status,	kRegRXSDI2Status,	kRegRXSDI3Status,	kRegRXSDI4Status,	kRegRXSDI5Status,	kRegRXSDI6Status,	kRegRXSDI7Status,	kRegRXSDI8Status};
		static const string	suffixes []	=	{"Status",	"CRCErrorCount",	"FrameCountLow",	"FrameCountHigh",	"FrameRefCountLow",	"FrameRefCountHigh"};
		static const int	perms []	=	{READWRITE,	READWRITE,			READWRITE,			READWRITE,			READONLY,			READONLY};

		AJAAutoLock	lock(&mGuardMutex);
		for (ULWord chan (0);  chan < 8;  chan++)
			for (UWord ndx(0);  ndx < 6;  ndx++)
			{
				ostringstream	ossName;	ossName << "kRegRXSDI" << DEC(chan+1) << suffixes[ndx];
				const string &	regName		(ossName.str());
				const uint32_t	regNum		(baseNum[chan] + ndx);
				const int		perm		(perms[ndx]);
				if (ndx == 0)
					DefineRegister (regNum,  regName,  mSDIErrorStatusRegDecoder,  perm,  kRegClass_SDIError,  gChlClasses[chan],  kRegClass_Input);
				else if (ndx == 1)
					DefineRegister (regNum,  regName,  mSDIErrorCountRegDecoder,   perm,  kRegClass_SDIError,  gChlClasses[chan],  kRegClass_Input);
				else
					DefineRegister (regNum,  regName,  mDefaultRegDecoder,         perm,  kRegClass_SDIError,  gChlClasses[chan],  kRegClass_Input);
			}
		DefineRegister (kRegRXSDIFreeRunningClockLow, "kRegRXSDIFreeRunningClockLow", mDefaultRegDecoder, READONLY, kRegClass_SDIError, kRegClass_NULL, kRegClass_NULL);
		DefineRegister (kRegRXSDIFreeRunningClockHigh, "kRegRXSDIFreeRunningClockHigh", mDefaultRegDecoder, READONLY, kRegClass_SDIError, kRegClass_NULL, kRegClass_NULL);
	}	//	SetupSDIErrorRegs

	void SetupLUTRegs (void)
	{
		AJAAutoLock	lock(&mGuardMutex);
	}

	void SetupCSCRegs(void)
	{
		static const string	sChan[8] = {kRegClass_Channel1, kRegClass_Channel2, kRegClass_Channel3, kRegClass_Channel4, kRegClass_Channel5, kRegClass_Channel6, kRegClass_Channel7, kRegClass_Channel8};

		AJAAutoLock	lock(&mGuardMutex);
		for (unsigned num(0);  num < 8;  num++)
		{
			ostringstream ossRegName;  ossRegName << "kRegEnhancedCSC" << (num+1);
			const string & chanClass (sChan[num]);					const string rootName    (ossRegName.str());
			const string modeName    (rootName + "Mode");			const string inOff01Name (rootName + "InOffset0_1");			const string inOff2Name  (rootName + "InOffset2");
			const string coeffA0Name (rootName + "CoeffA0");		const string coeffA1Name (rootName + "CoeffA1");				const string coeffA2Name (rootName + "CoeffA2");
			const string coeffB0Name (rootName + "CoeffB0");		const string coeffB1Name (rootName + "CoeffB1");				const string coeffB2Name (rootName + "CoeffB2");
			const string coeffC0Name (rootName + "CoeffC0");		const string coeffC1Name (rootName + "CoeffC1");				const string coeffC2Name (rootName + "CoeffC2");
			const string outOffABName(rootName + "OutOffsetA_B");	const string outOffCName (rootName + "OutOffsetC");
			const string keyModeName (rootName + "KeyMode");		const string keyClipOffName (rootName + "KeyClipOffset");		const string keyGainName (rootName + "KeyGain");
			DefineRegister (64*num + kRegEnhancedCSC1Mode,			modeName,			mEnhCSCModeDecoder,		READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1InOffset0_1,	inOff01Name,		mEnhCSCOffsetDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1InOffset2,		inOff2Name,			mEnhCSCOffsetDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffA0,		coeffA0Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffA1,		coeffA1Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffA2,		coeffA2Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffB0,		coeffB0Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffB1,		coeffB1Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffB2,		coeffB2Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffC0,		coeffC0Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffC1,		coeffC1Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1CoeffC2,		coeffC2Name,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1OutOffsetA_B,	outOffABName,		mEnhCSCOffsetDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1OutOffsetC,	outOffCName,		mEnhCSCOffsetDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1KeyMode,		keyModeName,		mEnhCSCKeyModeDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1KeyClipOffset,	keyClipOffName,		mEnhCSCOffsetDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (64*num + kRegEnhancedCSC1KeyGain,		keyGainName,		mEnhCSCCoeffDecoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
		}
		static const NTV2RegisterNumber	sECSCRegs[8][5]	=	{	{	kRegCSCoefficients1_2,	kRegCSCoefficients3_4,	kRegCSCoefficients5_6,	kRegCSCoefficients7_8,	kRegCSCoefficients9_10	},
																{	kRegCS2Coefficients1_2,	kRegCS2Coefficients3_4,	kRegCS2Coefficients5_6,	kRegCS2Coefficients7_8,	kRegCS2Coefficients9_10	},
																{	kRegCS3Coefficients1_2,	kRegCS3Coefficients3_4,	kRegCS3Coefficients5_6,	kRegCS3Coefficients7_8,	kRegCS3Coefficients9_10	},
																{	kRegCS4Coefficients1_2,	kRegCS4Coefficients3_4,	kRegCS4Coefficients5_6,	kRegCS4Coefficients7_8,	kRegCS4Coefficients9_10	},
																{	kRegCS5Coefficients1_2,	kRegCS5Coefficients3_4,	kRegCS5Coefficients5_6,	kRegCS5Coefficients7_8,	kRegCS5Coefficients9_10	},
																{	kRegCS6Coefficients1_2,	kRegCS6Coefficients3_4,	kRegCS6Coefficients5_6,	kRegCS6Coefficients7_8,	kRegCS6Coefficients9_10	},
																{	kRegCS7Coefficients1_2,	kRegCS7Coefficients3_4,	kRegCS7Coefficients5_6,	kRegCS7Coefficients7_8,	kRegCS7Coefficients9_10	},
																{	kRegCS8Coefficients1_2,	kRegCS8Coefficients3_4,	kRegCS8Coefficients5_6,	kRegCS8Coefficients7_8,	kRegCS8Coefficients9_10	}	};
		for (unsigned chan(0);  chan < 8;  chan++)
		{
			const string & chanClass (sChan[chan]);
			DefineRegister (sECSCRegs[chan][0],	"",	mCSCoeff1234Decoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (sECSCRegs[chan][1],	"",	mCSCoeff1234Decoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (sECSCRegs[chan][2],	"",	mCSCoeff567890Decoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (sECSCRegs[chan][3],	"",	mCSCoeff567890Decoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
			DefineRegister (sECSCRegs[chan][4],	"",	mCSCoeff567890Decoder,	READWRITE,	kRegClass_CSC,	chanClass,		kRegClass_NULL);
		}

		//	LUT/ColorCorrection Registers...
		DefineRegister	(kRegCh1ColorCorrectionControl,	"",	mLUTV1ControlRegDecoder,	READWRITE,	kRegClass_LUT,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegCh2ColorCorrectionControl,	"",	mLUTV1ControlRegDecoder,	READWRITE,	kRegClass_LUT,	kRegClass_NULL,	kRegClass_NULL);
		DefineRegister	(kRegLUTV2Control,				"",	mLUTV2ControlRegDecoder,	READWRITE,	kRegClass_LUT,	kRegClass_NULL,	kRegClass_NULL);
		//	LUT tables...
#if 1	//	V2 tables need the appropriate Enable & Bank bits set in kRegLUTV2Control, otherwise they'll always readback zero!
		//	So it's kinda pointless to read/decode them unless we do the "bank-select" dance immediately before reading them...
		const ULWord REDreg(kColorCorrectionLUTOffset_Red/4), GRNreg(kColorCorrectionLUTOffset_Green/4), BLUreg(kColorCorrectionLUTOffset_Blue/4);
		for (ULWord ndx(0);  ndx < 512;  ndx++)
		{
			ostringstream regNameR, regNameG, regNameB;
			regNameR << "kRegLUTRed" << DEC0N(ndx,3);  regNameG << "kRegLUTGreen" << DEC0N(ndx,3);  regNameB << "kRegLUTBlue" << DEC0N(ndx,3);
			DefineRegister (REDreg + ndx, regNameR.str(), mLUTDecoder,	READWRITE,	kRegClass_LUT,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister (GRNreg + ndx, regNameG.str(), mLUTDecoder,	READWRITE,	kRegClass_LUT,	kRegClass_NULL,	kRegClass_NULL);
			DefineRegister (BLUreg + ndx, regNameB.str(), mLUTDecoder,	READWRITE,	kRegClass_LUT,	kRegClass_NULL,	kRegClass_NULL);
		}
#endif
	}	//	SetupCSCRegs

	void SetupMixerKeyerRegs(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		//	VidProc/Mixer/Keyer
		DefineRegister	(kRegVidProc1Control,	"",	mVidProcControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel1,	kRegClass_Channel2);
		DefineRegister	(kRegVidProc2Control,	"",	mVidProcControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel3,	kRegClass_Channel4);
		DefineRegister	(kRegVidProc3Control,	"",	mVidProcControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel5,	kRegClass_Channel6);
		DefineRegister	(kRegVidProc4Control,	"",	mVidProcControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel7,	kRegClass_Channel8);
		DefineRegister	(kRegSplitControl,		"",	mSplitControlRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel1,	kRegClass_NULL);
		DefineRegister	(kRegFlatMatteValue,	"",	mFlatMatteValueRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel1,	kRegClass_Channel2);
		DefineRegister	(kRegFlatMatte2Value,	"",	mFlatMatteValueRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel3,	kRegClass_Channel4);
		DefineRegister	(kRegFlatMatte3Value,	"",	mFlatMatteValueRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel5,	kRegClass_Channel6);
		DefineRegister	(kRegFlatMatte4Value,	"",	mFlatMatteValueRegDecoder,	READWRITE,	kRegClass_Mixer,	kRegClass_Channel7,	kRegClass_Channel8);
		DefineRegister	(kRegMixer1Coefficient,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Mixer,	kRegClass_Channel1,	kRegClass_Channel2);
		DefineRegister	(kRegMixer2Coefficient,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Mixer,	kRegClass_Channel3,	kRegClass_Channel4);
		DefineRegister	(kRegMixer3Coefficient,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Mixer,	kRegClass_Channel5,	kRegClass_Channel6);
		DefineRegister	(kRegMixer4Coefficient,	"",	mDefaultRegDecoder,			READWRITE,	kRegClass_Mixer,	kRegClass_Channel7,	kRegClass_Channel8);
	}

	void SetupVRegs(void)
	{
		AJAAutoLock	lock(&mGuardMutex);
		DefineRegName	(kVRegDriverVersion,					"kVRegDriverVersion");
		DefineRegName	(kVRegRelativeVideoPlaybackDelay,		"kVRegRelativeVideoPlaybackDelay");
		DefineRegName	(kVRegAudioRecordPinDelay,				"kVRegAudioRecordPinDelay");
		DefineRegName	(kVRegGlobalAudioPlaybackMode,			"kVRegGlobalAudioPlaybackMode");
		DefineRegName	(kVRegFlashProgramKey,					"kVRegFlashProgramKey");
		DefineRegName	(kVRegStrictTiming,						"kVRegStrictTiming");
		DefineRegName	(kVRegInputSelect,						"kVRegInputSelect");
		DefineRegName	(kVRegSecondaryFormatSelect,            "kVRegSecondaryFormatSelect");
		DefineRegName	(kVRegDigitalOutput1Select,				"kVRegDigitalOutput1Select");
		DefineRegName	(kVRegDigitalOutput2Select,				"kVRegDigitalOutput2Select");
		DefineRegName	(kVRegAnalogOutputSelect,				"kVRegAnalogOutputSelect");
		DefineRegName	(kVRegAnalogOutputType,					"kVRegAnalogOutputType");
		DefineRegName	(kVRegAnalogOutBlackLevel,				"kVRegAnalogOutBlackLevel");
		DefineRegName	(kVRegInputSelectUser,					"kVRegInputSelectUser");
		DefineRegName	(kVRegVideoOutPauseMode,				"kVRegVideoOutPauseMode");
		DefineRegName	(kVRegPulldownPattern,					"kVRegPulldownPattern");
		DefineRegName	(kVRegColorSpaceMode,					"kVRegColorSpaceMode");
		DefineRegName	(kVRegGammaMode,						"kVRegGammaMode");
		DefineRegName	(kVRegLUTType,							"kVRegLUTType");
		DefineRegName	(kVRegRGB10Range,						"kVRegRGB10Range");
		DefineRegName	(kVRegRGB10Endian,						"kVRegRGB10Endian");
		DefineRegName	(kVRegFanControl,						"kVRegFanControl");
		DefineRegName	(kVRegBitFileDownload,					"kVRegBitFileDownload");
		DefineRegName	(kVRegSaveRegistersToRegistry,			"kVRegSaveRegistersToRegistry");
		DefineRegName	(kVRegRecallRegistersFromRegistry,		"kVRegRecallRegistersFromRegistry");
		DefineRegName	(kVRegClearAllSubscriptions,			"kVRegClearAllSubscriptions");
		DefineRegName	(kVRegRestoreHardwareProcampRegisters,	"kVRegRestoreHardwareProcampRegisters");
		DefineRegName	(kVRegAcquireReferenceCount,			"kVRegAcquireReferenceCount");
		DefineRegName	(kVRegReleaseReferenceCount,			"kVRegReleaseReferenceCount");
		DefineRegName	(kVRegDTAudioMux0,						"kVRegDTAudioMux0");
		DefineRegName	(kVRegDTAudioMux1,						"kVRegDTAudioMux1");
		DefineRegName	(kVRegDTAudioMux2,						"kVRegDTAudioMux2");
		DefineRegName	(kVRegDTFirmware,						"kVRegDTFirmware");
		DefineRegName	(kVRegDTVersionAja,						"kVRegDTVersionAja");
		DefineRegName	(kVRegDTVersionDurian,					"kVRegDTVersionDurian");
		DefineRegName	(kVRegDTAudioCapturePinConnected,		"kVRegDTAudioCapturePinConnected");
		DefineRegName	(kVRegTimeStampMode,					"kVRegTimeStampMode");
		DefineRegName	(kVRegTimeStampLastOutputVerticalLo,	"kVRegTimeStampLastOutputVerticalLo");
		DefineRegName	(kVRegTimeStampLastOutputVerticalHi,	"kVRegTimeStampLastOutputVerticalHi");
		DefineRegName	(kVRegTimeStampLastInput1VerticalLo,	"kVRegTimeStampLastInput1VerticalLo");
		DefineRegName	(kVRegTimeStampLastInput1VerticalHi,	"kVRegTimeStampLastInput1VerticalHi");
		DefineRegName	(kVRegTimeStampLastInput2VerticalLo,	"kVRegTimeStampLastInput2VerticalLo");
		DefineRegName	(kVRegTimeStampLastInput2VerticalHi,	"kVRegTimeStampLastInput2VerticalHi");
		DefineRegName	(kVRegNumberVideoMappingRegisters,		"kVRegNumberVideoMappingRegisters");
		DefineRegName	(kVRegNumberAudioMappingRegisters,		"kVRegNumberAudioMappingRegisters");
		DefineRegName	(kVRegAudioSyncTolerance,				"kVRegAudioSyncTolerance");
		DefineRegName	(kVRegDmaSerialize,						"kVRegDmaSerialize");
		DefineRegName	(kVRegSyncChannel,						"kVRegSyncChannel");
		DefineRegName	(kVRegSyncChannels,						"kVRegSyncChannels");
		DefineRegName	(kVRegSoftwareUartFifo,					"kVRegSoftwareUartFifo");
		DefineRegName	(kVRegTimeCodeCh1Delay,					"kVRegTimeCodeCh1Delay");
		DefineRegName	(kVRegTimeCodeCh2Delay,					"kVRegTimeCodeCh2Delay");
		DefineRegName	(kVRegTimeCodeIn1Delay,					"kVRegTimeCodeIn1Delay");
		DefineRegName	(kVRegTimeCodeIn2Delay,					"kVRegTimeCodeIn2Delay");
		DefineRegName	(kVRegTimeCodeCh3Delay,					"kVRegTimeCodeCh3Delay");
		DefineRegName	(kVRegTimeCodeCh4Delay,					"kVRegTimeCodeCh4Delay");
		DefineRegName	(kVRegTimeCodeIn3Delay,					"kVRegTimeCodeIn3Delay");
		DefineRegName	(kVRegTimeCodeIn4Delay,					"kVRegTimeCodeIn4Delay");
		DefineRegName	(kVRegTimeCodeCh5Delay,					"kVRegTimeCodeCh5Delay");
		DefineRegName	(kVRegTimeCodeIn5Delay,					"kVRegTimeCodeIn5Delay");
		DefineRegName	(kVRegTimeCodeCh6Delay,					"kVRegTimeCodeCh6Delay");
		DefineRegName	(kVRegTimeCodeIn6Delay,					"kVRegTimeCodeIn6Delay");
		DefineRegName	(kVRegTimeCodeCh7Delay,					"kVRegTimeCodeCh7Delay");
		DefineRegName	(kVRegTimeCodeIn7Delay,					"kVRegTimeCodeIn7Delay");
		DefineRegName	(kVRegTimeCodeCh8Delay,					"kVRegTimeCodeCh8Delay");
		DefineRegName	(kVRegTimeCodeIn8Delay,					"kVRegTimeCodeIn8Delay");
		DefineRegName	(kVRegDebug1,							"kVRegDebug1");
		DefineRegName	(kVRegDebugLastFormat,					"kVRegDebugLastFormat");
		DefineRegName	(kVRegDebugIPConfigTimeMS,				"kVRegDebugIPConfigTimeMS");
		DefineRegName	(kVRegDisplayReferenceSelect,			"kVRegDisplayReferenceSelect");
		DefineRegName	(kVRegVANCMode,							"kVRegVANCMode");
		DefineRegName	(kVRegDualStreamTransportType,			"kVRegDualStreamTransportType");
		DefineRegName	(kVRegDSKMode,							"kVRegDSKMode");
		DefineRegName	(kVRegIsoConvertEnable,					"kVRegIsoConvertEnable");
		DefineRegName	(kVRegDSKAudioMode,						"kVRegDSKAudioMode");
		DefineRegName	(kVRegDSKForegroundMode,				"kVRegDSKForegroundMode");
		DefineRegName	(kVRegDSKForegroundFade,				"kVRegDSKForegroundFade");
		DefineRegName	(kVRegCaptureReferenceSelect,			"kVRegCaptureReferenceSelect");
		DefineRegName	(kVReg2XTransferMode,					"kVReg2XTransferMode");
		DefineRegName	(kVRegSDIOutput1RGBRange,				"kVRegSDIOutput1RGBRange");
		DefineRegName	(kVRegSDIInput1FormatSelect,			"kVRegSDIInput1FormatSelect");
		DefineRegName	(kVRegSDIInput2FormatSelect,			"kVRegSDIInput2FormatSelect");
		DefineRegName	(kVRegSDIInput1RGBRange,				"kVRegSDIInput1RGBRange");
		DefineRegName	(kVRegSDIInput2RGBRange,				"kVRegSDIInput2RGBRange");
		DefineRegName	(kVRegSDIInput1Stereo3DMode,			"kVRegSDIInput1Stereo3DMode");
		DefineRegName	(kVRegSDIInput2Stereo3DMode,			"kVRegSDIInput2Stereo3DMode");
		DefineRegName	(kVRegFrameBuffer1RGBRange,				"kVRegFrameBuffer1RGBRange");
		DefineRegName	(kVRegFrameBuffer1Stereo3DMode,			"kVRegFrameBuffer1Stereo3DMode");
		DefineRegName	(kVRegHDMIInRgbRange,					"kVRegHDMIInRgbRange");
		DefineRegName	(kVRegHDMIOutRgbRange,					"kVRegHDMIOutRgbRange");
		DefineRegName	(kVRegAnalogInBlackLevel,				"kVRegAnalogInBlackLevel");
		DefineRegName	(kVRegAnalogInputType,					"kVRegAnalogInputType");
		DefineRegName	(kVRegHDMIOutColorSpaceModeCtrl,		"kVRegHDMIOutColorSpaceModeCtrl");
		DefineRegName	(kVRegHDMIOutProtocolMode,				"kVRegHDMIOutProtocolMode");
		DefineRegName	(kVRegHDMIOutStereoSelect,				"kVRegHDMIOutStereoSelect");
		DefineRegName	(kVRegHDMIOutStereoCodecSelect,			"kVRegHDMIOutStereoCodecSelect");
		DefineRegName	(kVRegHdmiOutSubSample,					"kVRegHdmiOutSubSample");
		DefineRegName	(kVRegSDIInput1ColorSpaceMode,			"kVRegSDIInput1ColorSpaceMode");
		DefineRegName	(kVRegSDIInput2ColorSpaceMode,			"kVRegSDIInput2ColorSpaceMode");
		DefineRegName	(kVRegSDIOutput2RGBRange,				"kVRegSDIOutput2RGBRange");
		DefineRegName	(kVRegSDIOutput1Stereo3DMode,			"kVRegSDIOutput1Stereo3DMode");
		DefineRegName	(kVRegSDIOutput2Stereo3DMode,			"kVRegSDIOutput2Stereo3DMode");
		DefineRegName	(kVRegFrameBuffer2RGBRange,				"kVRegFrameBuffer2RGBRange");
		DefineRegName	(kVRegFrameBuffer2Stereo3DMode,			"kVRegFrameBuffer2Stereo3DMode");
		DefineRegName	(kVRegAudioGainDisable,					"kVRegAudioGainDisable");
		DefineRegName	(kVRegLTCOnRefInSelect,					"kVRegLTCOnRefInSelect");
		DefineRegName	(kVRegActiveVideoOutFilter,				"kVRegActiveVideoOutFilter");
		DefineRegName	(kVRegAudioInputMapSelect,				"kVRegAudioInputMapSelect");
		DefineRegName	(kVRegAudioInputDelay,					"kVRegAudioInputDelay");
		DefineRegName	(kVRegDSKGraphicFileIndex,				"kVRegDSKGraphicFileIndex");
		DefineRegName	(kVRegTimecodeBurnInMode,				"kVRegTimecodeBurnInMode");
		DefineRegName	(kVRegUseQTTimecode,					"kVRegUseQTTimecode");
		DefineRegName	(kVRegAvailable164,						"kVRegAvailable164");
		DefineRegName	(kVRegRP188SourceSelect,				"kVRegRP188SourceSelect");
		DefineRegName	(kVRegQTCodecModeDebug,					"kVRegQTCodecModeDebug");
		DefineRegName	(kVRegHDMIOutColorSpaceModeStatus,		"kVRegHDMIOutColorSpaceModeStatus");
		DefineRegName	(kVRegDeviceOnline,						"kVRegDeviceOnline");
		DefineRegName	(kVRegIsDefaultDevice,					"kVRegIsDefaultDevice");
		DefineRegName	(kVRegDesktopFrameBufferStatus,			"kVRegDesktopFrameBufferStatus");
		DefineRegName	(kVRegSDIOutput1ColorSpaceMode,			"kVRegSDIOutput1ColorSpaceMode");
		DefineRegName	(kVRegSDIOutput2ColorSpaceMode,			"kVRegSDIOutput2ColorSpaceMode");
		DefineRegName	(kVRegAudioOutputDelay,					"kVRegAudioOutputDelay");
		DefineRegName	(kVRegTimelapseEnable,					"kVRegTimelapseEnable");
		DefineRegName	(kVRegTimelapseCaptureValue,			"kVRegTimelapseCaptureValue");
		DefineRegName	(kVRegTimelapseCaptureUnits,			"kVRegTimelapseCaptureUnits");
		DefineRegName	(kVRegTimelapseIntervalValue,			"kVRegTimelapseIntervalValue");
		DefineRegName	(kVRegTimelapseIntervalUnits,			"kVRegTimelapseIntervalUnits");
		DefineRegName	(kVRegFrameBufferInstalled,				"kVRegFrameBufferInstalled");
		DefineRegName	(kVRegAnalogInStandard,					"kVRegAnalogInStandard");
		DefineRegName	(kVRegOutputTimecodeOffset,				"kVRegOutputTimecodeOffset");
		DefineRegName	(kVRegOutputTimecodeType,				"kVRegOutputTimecodeType");
		DefineRegName	(kVRegQuicktimeUsingBoard,				"kVRegQuicktimeUsingBoard");
		DefineRegName	(kVRegApplicationPID,					"kVRegApplicationPID");
		DefineRegName	(kVRegApplicationCode,					"kVRegApplicationCode");
		DefineRegName	(kVRegReleaseApplication,				"kVRegReleaseApplication");
		DefineRegName	(kVRegForceApplicationPID,				"kVRegForceApplicationPID");
		DefineRegName	(kVRegForceApplicationCode,				"kVRegForceApplicationCode");
		DefineRegName	(kVRegIpConfigStreamRefresh,			"kVRegIpConfigStreamRefresh");
		DefineRegName	(kVRegSDIInModel,						"kVRegSDIInModel");
		DefineRegName	(kVRegInputChangedCount,				"kVRegInputChangedCount");
		DefineRegName	(kVReg8kOutputTransportSelection,		"kVReg8kOutputTransportSelection");
		DefineRegName	(kVRegAnalogIoSelect,					"kVRegAnalogIoSelect");
		DefineRegName	(kVRegProcAmpSDRegsInitialized,			"kVRegProcAmpSDRegsInitialized");
		DefineRegName	(kVRegProcAmpStandardDefBrightness,		"kVRegProcAmpStandardDefBrightness");
		DefineRegName	(kVRegProcAmpStandardDefContrast,		"kVRegProcAmpStandardDefContrast");
		DefineRegName	(kVRegProcAmpStandardDefSaturation,		"kVRegProcAmpStandardDefSaturation");
		DefineRegName	(kVRegProcAmpStandardDefHue,			"kVRegProcAmpStandardDefHue");
		DefineRegName	(kVRegProcAmpStandardDefCbOffset,		"kVRegProcAmpStandardDefCbOffset");
		DefineRegName	(kVRegProcAmpStandardDefCrOffset,		"kVRegProcAmpStandardDefCrOffset");
		DefineRegName	(kVRegProcAmpEndStandardDefRange,		"kVRegProcAmpEndStandardDefRange");
		DefineRegName	(kVRegProcAmpHDRegsInitialized,			"kVRegProcAmpHDRegsInitialized");
		DefineRegName	(kVRegProcAmpHighDefBrightness,			"kVRegProcAmpHighDefBrightness");
		DefineRegName	(kVRegProcAmpHighDefContrast,			"kVRegProcAmpHighDefContrast");
		DefineRegName	(kVRegProcAmpHighDefSaturationCb,		"kVRegProcAmpHighDefSaturationCb");
		DefineRegName	(kVRegProcAmpHighDefSaturationCr,		"kVRegProcAmpHighDefSaturationCr");
		DefineRegName	(kVRegProcAmpHighDefHue,				"kVRegProcAmpHighDefHue");
		DefineRegName	(kVRegProcAmpHighDefCbOffset,			"kVRegProcAmpHighDefCbOffset");
		DefineRegName	(kVRegProcAmpHighDefCrOffset,			"kVRegProcAmpHighDefCrOffset");
		DefineRegName	(kVRegProcAmpEndHighDefRange,			"kVRegProcAmpEndHighDefRange");
		DefineRegName	(kVRegChannel1UserBufferLevel,			"kVRegChannel1UserBufferLevel");
		DefineRegName	(kVRegChannel2UserBufferLevel,			"kVRegChannel2UserBufferLevel");
		DefineRegName	(kVRegInput1UserBufferLevel,			"kVRegInput1UserBufferLevel");
		DefineRegName	(kVRegInput2UserBufferLevel,			"kVRegInput2UserBufferLevel");
		DefineRegName	(kVRegProgressivePicture,				"kVRegProgressivePicture");
		DefineRegName	(kVRegLUT2Type,							"kVRegLUT2Type");
		DefineRegName	(kVRegLUT3Type,							"kVRegLUT3Type");
		DefineRegName	(kVRegLUT4Type,							"kVRegLUT4Type");
		DefineRegName	(kVRegDigitalOutput3Select,				"kVRegDigitalOutput3Select");
		DefineRegName	(kVRegDigitalOutput4Select,				"kVRegDigitalOutput4Select");
		DefineRegName	(kVRegHDMIOutputSelect,					"kVRegHDMIOutputSelect");
		DefineRegName	(kVRegRGBRangeConverterLUTType,			"kVRegRGBRangeConverterLUTType");
		DefineRegName	(kVRegTestPatternChoice,				"kVRegTestPatternChoice");
		DefineRegName	(kVRegTestPatternFormat,				"kVRegTestPatternFormat");
		DefineRegName	(kVRegEveryFrameTaskFilter,				"kVRegEveryFrameTaskFilter");
		DefineRegName	(kVRegDefaultInput,						"kVRegDefaultInput");
		DefineRegName	(kVRegDefaultVideoOutMode,				"kVRegDefaultVideoOutMode");
		DefineRegName	(kVRegDefaultVideoFormat,				"kVRegDefaultVideoFormat");
		DefineRegName	(kVRegDigitalOutput5Select,				"kVRegDigitalOutput5Select");
		DefineRegName	(kVRegLUT5Type,							"kVRegLUT5Type");
		DefineRegName	(kVRegMacUserModeDebugLevel,			"kVRegMacUserModeDebugLevel");
		DefineRegName	(kVRegMacKernelModeDebugLevel,			"kVRegMacKernelModeDebugLevel");
		DefineRegName	(kVRegMacUserModePingLevel,				"kVRegMacUserModePingLevel");
		DefineRegName	(kVRegMacKernelModePingLevel,			"kVRegMacKernelModePingLevel");
		DefineRegName	(kVRegLatencyTimerValue,				"kVRegLatencyTimerValue");
		DefineRegName	(kVRegAudioInputSelect,					"kVRegAudioInputSelect");
		DefineRegName	(kVRegSerialSuspended,					"kVRegSerialSuspended");
		DefineRegName	(kVRegXilinxProgramming,				"kVRegXilinxProgramming");
		DefineRegName	(kVRegETTDiagLastSerialTimestamp,		"kVRegETTDiagLastSerialTimestamp");
		DefineRegName	(kVRegETTDiagLastSerialTimecode,		"kVRegETTDiagLastSerialTimecode");
		DefineRegName	(kVRegStartupStatusFlags,				"kVRegStartupStatusFlags");
		DefineRegName	(kVRegRGBRangeMode,						"kVRegRGBRangeMode");
		DefineRegName	(kVRegEnableQueuedDMAs,					"kVRegEnableQueuedDMAs");
		DefineRegName	(kVRegBA0MemorySize,					"kVRegBA0MemorySize");
		DefineRegName	(kVRegBA1MemorySize,					"kVRegBA1MemorySize");
		DefineRegName	(kVRegBA4MemorySize,					"kVRegBA4MemorySize");
		DefineRegName	(kVRegNumDmaDriverBuffers,				"kVRegNumDmaDriverBuffers");
		DefineRegName	(kVRegDMADriverBufferPhysicalAddress,	"kVRegDMADriverBufferPhysicalAddress");
		DefineRegName	(kVRegBA2MemorySize,					"kVRegBA2MemorySize");
		DefineRegName	(kVRegAcquireLinuxReferenceCount,		"kVRegAcquireLinuxReferenceCount");
		DefineRegName	(kVRegReleaseLinuxReferenceCount,		"kVRegReleaseLinuxReferenceCount");
		DefineRegName	(kVRegAdvancedIndexing,					"kVRegAdvancedIndexing");
		DefineRegName	(kVRegTimeStampLastInput3VerticalLo,	"kVRegTimeStampLastInput3VerticalLo");
		DefineRegName	(kVRegTimeStampLastInput3VerticalHi,	"kVRegTimeStampLastInput3VerticalHi");
		DefineRegName	(kVRegTimeStampLastInput4VerticalLo,	"kVRegTimeStampLastInput4VerticalLo");
		DefineRegName	(kVRegTimeStampLastInput4VerticalHi,	"kVRegTimeStampLastInput4VerticalHi");
		DefineRegName	(kVRegTimeStampLastInput5VerticalLo,	"kVRegTimeStampLastInput5VerticalLo");
		DefineRegName	(kVRegTimeStampLastInput5VerticalHi,	"kVRegTimeStampLastInput5VerticalHi");
		DefineRegName	(kVRegTimeStampLastInput6VerticalLo,	"kVRegTimeStampLastInput6VerticalLo");
		DefineRegName	(kVRegTimeStampLastInput6VerticalHi,	"kVRegTimeStampLastInput6VerticalHi");
		DefineRegName	(kVRegTimeStampLastInput7VerticalLo,	"kVRegTimeStampLastInput7VerticalLo");
		DefineRegName	(kVRegTimeStampLastInput7VerticalHi,	"kVRegTimeStampLastInput7VerticalHi");
		DefineRegName	(kVRegTimeStampLastInput8VerticalLo,	"kVRegTimeStampLastInput8VerticalLo");
		DefineRegName	(kVRegTimeStampLastInput8VerticalHi,	"kVRegTimeStampLastInput8VerticalHi");
		DefineRegName	(kVRegTimeStampLastOutput2VerticalLo,	"kVRegTimeStampLastOutput2VerticalLo");
		DefineRegName	(kVRegTimeStampLastOutput2VerticalHi,	"kVRegTimeStampLastOutput2VerticalHi");
		DefineRegName	(kVRegTimeStampLastOutput3VerticalLo,	"kVRegTimeStampLastOutput3VerticalLo");
		DefineRegName	(kVRegTimeStampLastOutput3VerticalHi,	"kVRegTimeStampLastOutput3VerticalHi");
		DefineRegName	(kVRegTimeStampLastOutput4VerticalLo,	"kVRegTimeStampLastOutput4VerticalLo");
		DefineRegName	(kVRegTimeStampLastOutput4VerticalHi,	"kVRegTimeStampLastOutput4VerticalHi");
		DefineRegName	(kVRegTimeStampLastOutput5VerticalLo,	"kVRegTimeStampLastOutput5VerticalLo");
		DefineRegName	(kVRegTimeStampLastOutput5VerticalHi,	"kVRegTimeStampLastOutput5VerticalHi");
		DefineRegName	(kVRegTimeStampLastOutput6VerticalLo,	"kVRegTimeStampLastOutput6VerticalLo");
		DefineRegName	(kVRegTimeStampLastOutput6VerticalHi,	"kVRegTimeStampLastOutput6VerticalHi");
		DefineRegName	(kVRegTimeStampLastOutput7VerticalLo,	"kVRegTimeStampLastOutput7VerticalLo");
		DefineRegName	(kVRegTimeStampLastOutput7VerticalHi,	"kVRegTimeStampLastOutput7VerticalHi");
		DefineRegName	(kVRegTimeStampLastOutput8VerticalLo,	"kVRegTimeStampLastOutput8VerticalLo");
		DefineRegName	(kVRegResetCycleCount,					"kVRegResetCycleCount");
		DefineRegName	(kVRegUseProgressive,					"kVRegUseProgressive");
		DefineRegName	(kVRegFlashSize,						"kVRegFlashSize");
		DefineRegName	(kVRegFlashStatus,						"kVRegFlashStatus");
		DefineRegName	(kVRegFlashState,						"kVRegFlashState");
		DefineRegName	(kVRegPCIDeviceID,						"kVRegPCIDeviceID");
		DefineRegName	(kVRegUartRxFifoSize,					"kVRegUartRxFifoSize");
		DefineRegName	(kVRegEFTNeedsUpdating,					"kVRegEFTNeedsUpdating");
		DefineRegName	(kVRegSuspendSystemAudio,				"kVRegSuspendSystemAudio");
		DefineRegName	(kVRegAcquireReferenceCounter,			"kVRegAcquireReferenceCounter");
		DefineRegName	(kVRegTimeStampLastOutput8VerticalHi,	"kVRegTimeStampLastOutput8VerticalHi");
		DefineRegName	(kVRegFramesPerVertical,				"kVRegFramesPerVertical");
		DefineRegName	(kVRegServicesInitialized,				"kVRegServicesInitialized");
		DefineRegName	(kVRegFrameBufferGangCount,				"kVRegFrameBufferGangCount");
		DefineRegName	(kVRegChannelCrosspointFirst,			"kVRegChannelCrosspointFirst");
		DefineRegName	(kVRegChannelCrosspointLast,			"kVRegChannelCrosspointLast");
		DefineRegName	(kVRegMonAncField1Offset,				"kVRegMonAncField1Offset");
		DefineRegName	(kVRegMonAncField2Offset,				"kVRegMonAncField2Offset");
		DefineRegName	(kVRegFollowInputFormat,				"kVRegFollowInputFormat");
		DefineRegName	(kVRegAncField1Offset,					"kVRegAncField1Offset");
		DefineRegName	(kVRegAncField2Offset,					"kVRegAncField2Offset");
		DefineRegName	(kVRegAgentCheck,						"kVRegAgentCheck");
		DefineRegName	(kVRegUnused_2,							"kVRegUnused_2");
		DefineRegName	(kVReg4kOutputTransportSelection,		"kVReg4kOutputTransportSelection");
		DefineRegName	(kVRegCustomAncInputSelect,				"kVRegCustomAncInputSelect");
		DefineRegName	(kVRegUseThermostat,					"kVRegUseThermostat");
		DefineRegName	(kVRegThermalSamplingRate,				"kVRegThermalSamplingRate");
		DefineRegName	(kVRegFanSpeed,							"kVRegFanSpeed");
		DefineRegName	(kVRegVideoFormatCh1,					"kVRegVideoFormatCh1");
		DefineRegName	(kVRegVideoFormatCh2,					"kVRegVideoFormatCh2");
		DefineRegName	(kVRegVideoFormatCh3,					"kVRegVideoFormatCh3");
		DefineRegName	(kVRegVideoFormatCh4,					"kVRegVideoFormatCh4");
		DefineRegName	(kVRegVideoFormatCh5,					"kVRegVideoFormatCh5");
		DefineRegName	(kVRegVideoFormatCh6,					"kVRegVideoFormatCh6");
		DefineRegName	(kVRegVideoFormatCh7,					"kVRegVideoFormatCh7");
		DefineRegName	(kVRegVideoFormatCh8,					"kVRegVideoFormatCh8");
		DefineRegName	(kVRegIPAddrEth0,						"kVRegIPAddrEth0");
		DefineRegName	(kVRegSubnetEth0,						"kVRegSubnetEth0");
		DefineRegName	(kVRegGatewayEth0,						"kVRegGatewayEth0");
		DefineRegName	(kVRegIPAddrEth1,						"kVRegIPAddrEth1");
		DefineRegName	(kVRegSubnetEth1,						"kVRegSubnetEth1");
		DefineRegName	(kVRegGatewayEth1,						"kVRegGatewayEth1");
		DefineRegName	(kVRegRxcEnable1,						"kVRegRxcEnable1");
        DefineRegName	(kVRegRxcSfp1RxMatch1,                  "kVRegRxcSfp1RxMatch1");
        DefineRegName	(kVRegRxcSfp1SourceIp1,                 "kVRegRxcSfp1SourceIp1");
        DefineRegName	(kVRegRxcSfp1DestIp1,                   "kVRegRxcSfp1DestIp1");
        DefineRegName	(kVRegRxcSfp1SourcePort1,               "kVRegRxcSfp1SourcePort1");
        DefineRegName	(kVRegRxcSfp1DestPort1,                 "kVRegRxcSfp1DestPort1");
        DefineRegName	(kVRegRxcSfp1Vlan1,                     "kVRegRxcSfp1Vlan1");
        DefineRegName	(kVRegRxcSfp2RxMatch1,                  "kVRegRxcSfp2RxMatch1");
        DefineRegName	(kVRegRxcSfp2SourceIp1,                 "kVRegRxcSfp2SourceIp1");
        DefineRegName	(kVRegRxcSfp2DestIp1,                   "kVRegRxcSfp2DestIp1");
        DefineRegName	(kVRegRxcSfp2SourcePort1,               "kVRegRxcSfp2SourcePort1");
        DefineRegName	(kVRegRxcSfp2DestPort1,                 "kVRegRxcSfp2DestPort1");
        DefineRegName	(kVRegRxcSfp2Vlan1,                     "kVRegRxcSfp2Vlan1");
		DefineRegName	(kVRegRxcSsrc1,							"kVRegRxcSsrc1");
		DefineRegName	(kVRegRxcPlayoutDelay1,					"kVRegRxcPlayoutDelay1");
		DefineRegName	(kVRegRxcEnable2,						"kVRegRxcEnable2");
        DefineRegName	(kVRegRxcSfp1RxMatch2,                  "kVRegRxcSfp1RxMatch2");
        DefineRegName	(kVRegRxcSfp1SourceIp2,                 "kVRegRxcSfp1SourceIp2");
        DefineRegName	(kVRegRxcSfp1DestIp2,                   "kVRegRxcSfp1DestIp2");
        DefineRegName	(kVRegRxcSfp1SourcePort2,               "kVRegRxcSfp1SourcePort2");
        DefineRegName	(kVRegRxcSfp1DestPort2,                 "kVRegRxcSfp1DestPort2");
        DefineRegName	(kVRegRxcSfp1Vlan2,                     "kVRegRxcSfp1Vlan2");
        DefineRegName	(kVRegRxcSfp2RxMatch2,                  "kVRegRxcSfp2RxMatch2");
        DefineRegName	(kVRegRxcSfp2SourceIp2,                 "kVRegRxcSfp2SourceIp2");
        DefineRegName	(kVRegRxcSfp2DestIp2,                   "kVRegRxcSfp2DestIp2");
        DefineRegName	(kVRegRxcSfp2SourcePort2,               "kVRegRxcSfp2SourcePort2");
        DefineRegName	(kVRegRxcSfp2DestPort2,                 "kVRegRxcSfp2DestPort2");
        DefineRegName	(kVRegRxcSfp2Vlan2,                     "kVRegRxcSfp2Vlan2");
		DefineRegName	(kVRegRxcSsrc2,							"kVRegRxcSsrc2");
		DefineRegName	(kVRegRxcPlayoutDelay2,					"kVRegRxcPlayoutDelay2");
		DefineRegName	(kVRegTxcEnable3,						"kVRegTxcEnable3");
        DefineRegName	(kVRegTxcSfp1LocalPort3,				"kVRegTxcSfp1LocalPort3");
        DefineRegName	(kVRegTxcSfp1RemoteIp3,                 "kVRegTxcSfp1RemoteIp3");
        DefineRegName	(kVRegTxcSfp1RemotePort3,               "kVRegTxcSfp1RemotePort3");
        DefineRegName	(kVRegTxcSfp2LocalPort3,                "kVRegTxcSfp2LocalPort3");
        DefineRegName	(kVRegTxcSfp2RemoteIp3,                 "kVRegTxcSfp2RemoteIp3");
        DefineRegName	(kVRegTxcSfp2RemotePort3,               "kVRegTxcSfp2RemotePort3");
		DefineRegName	(kVRegTxcEnable4,						"kVRegTxcEnable4");
        DefineRegName	(kVRegTxcSfp1LocalPort4,				"kVRegTxcSfp1LocalPort4");
        DefineRegName	(kVRegTxcSfp1RemoteIp4,                 "kVRegTxcSfp1RemoteIp4");
        DefineRegName	(kVRegTxcSfp1RemotePort4,               "kVRegTxcSfp1RemotePort4");
        DefineRegName	(kVRegTxcSfp2LocalPort4,                "kVRegTxcSfp2LocalPort4");
        DefineRegName	(kVRegTxcSfp2RemoteIp4,                 "kVRegTxcSfp2RemoteIp4");
        DefineRegName	(kVRegTxcSfp2RemotePort4,               "kVRegTxcSfp2RemotePort4");
		DefineRegName	(kVRegMailBoxAcquire,					"kVRegMailBoxAcquire");
		DefineRegName	(kVRegMailBoxRelease,					"kVRegMailBoxRelease");
		DefineRegName	(kVRegMailBoxAbort,						"kVRegMailBoxAbort");
		DefineRegName	(kVRegMailBoxTimeoutNS,					"kVRegMailBoxTimeoutNS");
		DefineRegName	(kVRegRxc_2DecodeSelectionMode1,		"kVRegRxc_2DecodeSelectionMode1");
		DefineRegName	(kVRegRxc_2DecodeProgramNumber1,		"kVRegRxc_2DecodeProgramNumber1");
		DefineRegName	(kVRegRxc_2DecodeProgramPID1,			"kVRegRxc_2DecodeProgramPID1");
		DefineRegName	(kVRegRxc_2DecodeAudioNumber1,			"kVRegRxc_2DecodeAudioNumber1");
		DefineRegName	(kVRegRxc_2DecodeSelectionMode2,		"kVRegRxc_2DecodeSelectionMode2");
		DefineRegName	(kVRegRxc_2DecodeProgramNumber2,		"kVRegRxc_2DecodeProgramNumber2");
		DefineRegName	(kVRegRxc_2DecodeProgramPID2,			"kVRegRxc_2DecodeProgramPID2");
		DefineRegName	(kVRegRxc_2DecodeAudioNumber2,			"kVRegRxc_2DecodeAudioNumber2");
		DefineRegName	(kVRegTxc_2EncodeVideoFormat1,			"kVRegTxc_2EncodeVideoFormat1");
		DefineRegName	(kVRegTxc_2EncodeUllMode1,				"kVRegTxc_2EncodeUllMode1");
		DefineRegName	(kVRegTxc_2EncodeBitDepth1,				"kVRegTxc_2EncodeBitDepth1");
		DefineRegName	(kVRegTxc_2EncodeChromaSubSamp1,		"kVRegTxc_2EncodeChromaSubSamp1");
		DefineRegName	(kVRegTxc_2EncodeMbps1,					"kVRegTxc_2EncodeMbps1");
		DefineRegName	(kVRegTxc_2EncodeAudioChannels1,		"kVRegTxc_2EncodeAudioChannels1");
		DefineRegName	(kVRegTxc_2EncodeStreamType1,			"kVRegTxc_2EncodeStreamType1");
		DefineRegName	(kVRegTxc_2EncodeProgramPid1,			"kVRegTxc_2EncodeProgramPid1");
		DefineRegName	(kVRegTxc_2EncodeVideoPid1,				"kVRegTxc_2EncodeVideoPid1");
		DefineRegName	(kVRegTxc_2EncodePcrPid1,				"kVRegTxc_2EncodePcrPid1");
		DefineRegName	(kVRegTxc_2EncodeAudio1Pid1,			"kVRegTxc_2EncodeAudio1Pid1");
		DefineRegName	(kVRegTxc_2EncodeVideoFormat2,			"kVRegTxc_2EncodeVideoFormat2");
		DefineRegName	(kVRegTxc_2EncodeUllMode2,				"kVRegTxc_2EncodeUllMode2");
		DefineRegName	(kVRegTxc_2EncodeBitDepth2,				"kVRegTxc_2EncodeBitDepth2");
		DefineRegName	(kVRegTxc_2EncodeChromaSubSamp2,		"kVRegTxc_2EncodeChromaSubSamp2");
		DefineRegName	(kVRegTxc_2EncodeMbps2,					"kVRegTxc_2EncodeMbps2");
		DefineRegName	(kVRegTxc_2EncodeAudioChannels2,		"kVRegTxc_2EncodeAudioChannels2");
		DefineRegName	(kVRegTxc_2EncodeStreamType2,			"kVRegTxc_2EncodeStreamType2");
		DefineRegName	(kVRegTxc_2EncodeProgramPid2,			"kVRegTxc_2EncodeProgramPid2");
		DefineRegName	(kVRegTxc_2EncodeVideoPid2,				"kVRegTxc_2EncodeVideoPid2");
		DefineRegName	(kVRegTxc_2EncodePcrPid2,				"kVRegTxc_2EncodePcrPid2");
		DefineRegName	(kVRegTxc_2EncodeAudio1Pid2,			"kVRegTxc_2EncodeAudio1Pid2");
		DefineRegName	(kVReg2022_7Enable,						"kVReg2022_7Enable");
		DefineRegName	(kVReg2022_7NetworkPathDiff,			"kVReg2022_7NetworkPathDiff");
		DefineRegName	(kVRegKIPRxCfgError,					"kVRegKIPRxCfgError");
		DefineRegName	(kVRegKIPTxCfgError,					"kVRegKIPTxCfgError");
		DefineRegName	(kVRegKIPEncCfgError,					"kVRegKIPEncCfgError");
		DefineRegName	(kVRegKIPDecCfgError,					"kVRegKIPDecCfgError");
		DefineRegName	(kVRegKIPNetCfgError,					"kVRegKIPNetCfgError");
		DefineRegName	(kVRegUseHDMI420Mode,					"kVRegUseHDMI420Mode");
		DefineRegName	(kVRegUnused501,						"kVRegUnused501");
		DefineRegName	(kVRegUserDefinedDBB,					"kVRegUserDefinedDBB");
		DefineRegName	(kVRegHDMIOutAudioChannels,				"kVRegHDMIOutAudioChannels");
		DefineRegName	(kVRegZeroHostAncPostCapture,			"kVRegZeroHostAncPostCapture");
		DefineRegName	(kVRegZeroDeviceAncPostCapture,			"kVRegZeroDeviceAncPostCapture");
		DefineRegName	(kVRegAudioMonitorChannelSelect,		"kVRegAudioMonitorChannelSelect");
		DefineRegName	(kVRegAudioMixerOverrideState,			"kVRegAudioMixerOverrideState");
		DefineRegName	(kVRegAudioMixerSourceMainEnable,		"kVRegAudioMixerSourceMainEnable");
		DefineRegName	(kVRegAudioMixerSourceAux1Enable,		"kVRegAudioMixerSourceAux1Enable");
		DefineRegName	(kVRegAudioMixerSourceAux2Enable,		"kVRegAudioMixerSourceAux2Enable");
		DefineRegName	(kVRegAudioMixerSourceMainGain,			"kVRegAudioMixerSourceMainGain");
		DefineRegName	(kVRegAudioMixerSourceAux1Gain,			"kVRegAudioMixerSourceAux1Gain");
		DefineRegName	(kVRegAudioMixerSourceAux2Gain,			"kVRegAudioMixerSourceAux2Gain");
		DefineRegName	(kVRegAudioCapMixerSourceMainEnable,	"kVRegAudioCapMixerSourceMainEnable");
		DefineRegName	(kVRegAudioCapMixerSourceAux1Enable,	"kVRegAudioCapMixerSourceAux1Enable");
		DefineRegName	(kVRegAudioCapMixerSourceAux2Enable,	"kVRegAudioCapMixerSourceAux2Enable");
		DefineRegName	(kVRegAudioCapMixerSourceMainGain,		"kVRegAudioCapMixerSourceMainGain");
		DefineRegName	(kVRegAudioCapMixerSourceAux1Gain,		"kVRegAudioCapMixerSourceAux1Gain");
		DefineRegName	(kVRegAudioCapMixerSourceAux2Gain,		"kVRegAudioCapMixerSourceAux2Gain");
		DefineRegName	(kVRegSwizzle4kInput,					"kVRegSwizzle4kInput");
		DefineRegName	(kVRegSwizzle4kOutput,					"kVRegSwizzle4kOutput");
		DefineRegName	(kVRegAnalogAudioIOConfiguration,		"kVRegAnalogAudioIOConfiguration");
		DefineRegName	(kVRegHdmiHdrOutChanged,				"kVRegHdmiHdrOutChanged");
		DefineRegName	(kVRegDisableAutoVPID,					"kVRegDisableAutoVPID");
		DefineRegName	(kVRegEnableBT2020,						"kVRegEnableBT2020");
		DefineRegName	(kVRegHdmiHdrOutMode,					"kVRegHdmiHdrOutMode");
		DefineRegName	(kVRegServicesForceInit,				"kVRegServicesForceInit");
		DefineRegName	(kVRegServicesModeFinal,				"kVRegServicesModeFinal");
		DefineRegName	(kVRegNTV2VPIDTransferCharacteristics,	"kVRegNTV2VPIDTransferCharacteristics");
		DefineRegName	(kVRegNTV2VPIDColorimetry,				"kVRegNTV2VPIDColorimetry");
		DefineRegName	(kVRegNTV2VPIDLuminance,				"kVRegNTV2VPIDLuminance");
		DefineRegName	(kVRegNTV2VPIDTransferCharacteristics2,	"kVRegNTV2VPIDTransferCharacteristics2");
		DefineRegName	(kVRegNTV2VPIDColorimetry2,				"kVRegNTV2VPIDColorimetry2");
		DefineRegName	(kVRegNTV2VPIDLuminance2,				"kVRegNTV2VPIDLuminance2");
		DefineRegName	(kVRegNTV2VPIDTransferCharacteristics3,	"kVRegNTV2VPIDTransferCharacteristics3");
		DefineRegName	(kVRegNTV2VPIDColorimetry3,				"kVRegNTV2VPIDColorimetry3");
		DefineRegName	(kVRegNTV2VPIDLuminance3,				"kVRegNTV2VPIDLuminance3");
		DefineRegName	(kVRegNTV2VPIDTransferCharacteristics4,	"kVRegNTV2VPIDTransferCharacteristics4");
		DefineRegName	(kVRegNTV2VPIDColorimetry4,				"kVRegNTV2VPIDColorimetry4");
		DefineRegName	(kVRegNTV2VPIDLuminance4,				"kVRegNTV2VPIDLuminance4");
		DefineRegName	(kVRegNTV2VPIDTransferCharacteristics5,	"kVRegNTV2VPIDTransferCharacteristics5");
		DefineRegName	(kVRegNTV2VPIDColorimetry5,				"kVRegNTV2VPIDColorimetry5");
		DefineRegName	(kVRegNTV2VPIDLuminance5,				"kVRegNTV2VPIDLuminance5");
		DefineRegName	(kVRegNTV2VPIDTransferCharacteristics6,	"kVRegNTV2VPIDTransferCharacteristics6");
		DefineRegName	(kVRegNTV2VPIDColorimetry6,				"kVRegNTV2VPIDColorimetry6");
		DefineRegName	(kVRegNTV2VPIDLuminance6,				"kVRegNTV2VPIDLuminance6");
		DefineRegName	(kVRegNTV2VPIDTransferCharacteristics7,	"kVRegNTV2VPIDTransferCharacteristics7");
		DefineRegName	(kVRegNTV2VPIDColorimetry7,				"kVRegNTV2VPIDColorimetry7");
		DefineRegName	(kVRegNTV2VPIDLuminance7,				"kVRegNTV2VPIDLuminance7");
		DefineRegName	(kVRegNTV2VPIDTransferCharacteristics8,	"kVRegNTV2VPIDTransferCharacteristics8");
		DefineRegName	(kVRegNTV2VPIDColorimetry8,				"kVRegNTV2VPIDColorimetry8");
		DefineRegName	(kVRegNTV2VPIDLuminance8,				"kVRegNTV2VPIDLuminance8");
		DefineRegName	(kVRegUserColorimetry,					"kVRegUserColorimetry");					
		DefineRegName	(kVRegUserTransfer,						"kVRegUserTransfer");
		DefineRegName	(kVRegUserLuminance,					"kVRegUserLuminance");
		DefineRegName	(kVRegHdrColorimetryCh1,				"kVRegHdrColorimetryCh1");
		DefineRegName	(kVRegHdrTransferCh1,					"kVRegHdrTransferCh1");
		DefineRegName	(kVRegHdrLuminanceCh1,					"kVRegHdrLuminanceCh1");
		DefineRegName	(kVRegHdrGreenXCh1,						"kVRegHdrGreenXCh1");
		DefineRegName	(kVRegHdrGreenYCh1,						"kVRegHdrGreenYCh1");
		DefineRegName	(kVRegHdrBlueXCh1,						"kVRegHdrBlueXCh1");
		DefineRegName	(kVRegHdrBlueYCh1,						"kVRegHdrBlueYCh1");
		DefineRegName	(kVRegHdrRedXCh1,						"kVRegHdrRedXCh1");
		DefineRegName	(kVRegHdrRedYCh1,						"kVRegHdrRedYCh1");
		DefineRegName	(kVRegHdrWhiteXCh1,						"kVRegHdrWhiteXCh1");
		DefineRegName	(kVRegHdrWhiteYCh1,						"kVRegHdrWhiteYCh1");
		DefineRegName	(kVRegHdrMasterLumMaxCh1,				"kVRegHdrMasterLumMaxCh1");
		DefineRegName	(kVRegHdrMasterLumMinCh1,				"kVRegHdrMasterLumMinCh1");
		DefineRegName	(kVRegHdrMaxCLLCh1,						"kVRegHdrMaxCLLCh1");
		DefineRegName	(kVRegHdrMaxFALLCh1,					"kVRegHdrMaxFALLCh1");
		DefineRegName	(kVRegHDROverrideState,					"kVRegHDROverrideState");
        DefineRegName	(kVRegPCIMaxReadRequestSize,			"kVRegPCIMaxReadRequestSize");
		DefineRegName	(kVRegUserInColorimetry,				"kVRegUserInColorimetry");					
		DefineRegName	(kVRegUserInTransfer,					"kVRegUserInTransfer");
		DefineRegName	(kVRegUserInLuminance,					"kVRegUserInLuminance");
		DefineRegName	(kVRegHdrInColorimetryCh1,				"kVRegHdrInColorimetryCh1");
		DefineRegName	(kVRegHdrInTransferCh1,					"kVRegHdrInTransferCh1");
		DefineRegName	(kVRegHdrInLuminanceCh1,				"kVRegHdrInLuminanceCh1");
		DefineRegName	(kVRegHdrInGreenXCh1,					"kVRegHdrInGreenXCh1");
		DefineRegName	(kVRegHdrInGreenYCh1,					"kVRegHdrInGreenYCh1");
		DefineRegName	(kVRegHdrInBlueXCh1,					"kVRegHdrInBlueXCh1");
		DefineRegName	(kVRegHdrInBlueYCh1,					"kVRegHdrInBlueYCh1");
		DefineRegName	(kVRegHdrInRedXCh1,						"kVRegHdrInRedXCh1");
		DefineRegName	(kVRegHdrInRedYCh1,						"kVRegHdrInRedYCh1");
		DefineRegName	(kVRegHdrInWhiteXCh1,					"kVRegHdrInWhiteXCh1");
		DefineRegName	(kVRegHdrInWhiteYCh1,					"kVRegHdrInWhiteYCh1");
		DefineRegName	(kVRegHdrInMasterLumMaxCh1,				"kVRegHdrInMasterLumMaxCh1");
		DefineRegName	(kVRegHdrInMasterLumMinCh1,				"kVRegHdrInMasterLumMinCh1");
		DefineRegName	(kVRegHdrInMaxCLLCh1,					"kVRegHdrInMaxCLLCh1");
		DefineRegName	(kVRegHdrInMaxFALLCh1,					"kVRegHdrInMaxFALLCh1");
		DefineRegName	(kVRegHDRInOverrideState,				"kVRegHDRInOverrideState");
		DefineRegName	(kVRegNTV2VPIDRGBRange1,				"kVRegNTV2VPIDRGBRange1");
		DefineRegName	(kVRegNTV2VPIDRGBRange2,				"kVRegNTV2VPIDRGBRange2");
		DefineRegName	(kVRegNTV2VPIDRGBRange3,				"kVRegNTV2VPIDRGBRange3");
		DefineRegName	(kVRegNTV2VPIDRGBRange4,				"kVRegNTV2VPIDRGBRange4");
		DefineRegName	(kVRegNTV2VPIDRGBRange5,				"kVRegNTV2VPIDRGBRange5");
		DefineRegName	(kVRegNTV2VPIDRGBRange6,				"kVRegNTV2VPIDRGBRange6");
		DefineRegName	(kVRegNTV2VPIDRGBRange7,				"kVRegNTV2VPIDRGBRange7");
		DefineRegName	(kVRegNTV2VPIDRGBRange8,				"kVRegNTV2VPIDRGBRange8");
		DefineRegName	(kVRegDmaTransferRateC2H1,				"kVRegDmaTransferRateC2H1");
		DefineRegName	(kVRegDmaHardwareRateC2H1,				"kVRegDmaHardwareRateC2H1");
		DefineRegName	(kVRegDmaTransferRateH2C1,				"kVRegDmaTransferRateH2C1");
		DefineRegName	(kVRegDmaHardwareRateH2C1,				"kVRegDmaHardwareRateH2C1");
		DefineRegName	(kVRegDmaTransferRateC2H2,				"kVRegDmaTransferRateC2H2");
		DefineRegName	(kVRegDmaHardwareRateC2H2,				"kVRegDmaHardwareRateC2H2");
		DefineRegName	(kVRegDmaTransferRateH2C2,				"kVRegDmaTransferRateH2C2");
		DefineRegName	(kVRegDmaHardwareRateH2C2,				"kVRegDmaHardwareRateH2C2");
		DefineRegName	(kVRegDmaTransferRateC2H3,				"kVRegDmaTransferRateC2H3");
		DefineRegName	(kVRegDmaHardwareRateC2H3,				"kVRegDmaHardwareRateC2H3");
		DefineRegName	(kVRegDmaTransferRateH2C3,				"kVRegDmaTransferRateH2C3");
		DefineRegName	(kVRegDmaHardwareRateH2C3,				"kVRegDmaHardwareRateH2C3");
		DefineRegName	(kVRegDmaTransferRateC2H4,				"kVRegDmaTransferRateC2H4");
		DefineRegName	(kVRegDmaHardwareRateC2H4,				"kVRegDmaHardwareRateC2H4");
		DefineRegName	(kVRegDmaTransferRateH2C4,				"kVRegDmaTransferRateH2C4");
		DefineRegName	(kVRegDmaHardwareRateH2C4,				"kVRegDmaHardwareRateH2C4");

		DefineRegName	(kVRegHDMIInAviInfo1,					"kVRegHDMIInAviInfo1");
		DefineRegName	(kVRegHDMIInDrmInfo1,					"kVRegHDMIInDrmInfo1");
		DefineRegName	(kVRegHDMIInDrmGreenPrimary1,			"kVRegHDMIInDrmGreenPrimary1");
		DefineRegName	(kVRegHDMIInDrmBluePrimary1,			"kVRegHDMIInDrmBluePrimary1");
		DefineRegName	(kVRegHDMIInDrmRedPrimary1,				"kVRegHDMIInDrmRedPrimary1");
		DefineRegName	(kVRegHDMIInDrmWhitePoint1,				"kVRegHDMIInDrmWhitePoint1");
		DefineRegName	(kVRegHDMIInDrmMasteringLuminence1,		"kVRegHDMIInDrmMasteringLuminence1");
		DefineRegName	(kVRegHDMIInDrmLightLevel1,				"kVRegHDMIInDrmLightLevel1");

		DefineRegName	(kVRegHDMIInAviInfo2,					"kVRegHDMIInAviInfo2");
		DefineRegName	(kVRegHDMIInDrmInfo2,                   "kVRegHDMIInDrmInfo2");
		DefineRegName	(kVRegHDMIInDrmGreenPrimary2,			"kVRegHDMIInDrmGreenPrimary2");
		DefineRegName	(kVRegHDMIInDrmBluePrimary2,			"kVRegHDMIInDrmBluePrimary2");
		DefineRegName	(kVRegHDMIInDrmRedPrimary2,				"kVRegHDMIInDrmRedPrimary2");
		DefineRegName	(kVRegHDMIInDrmWhitePoint2,				"kVRegHDMIInDrmWhitePoint2");
		DefineRegName	(kVRegHDMIInDrmMasteringLuminence2,		"kVRegHDMIInDrmMasteringLuminence2");
		DefineRegName	(kVRegHDMIInDrmLightLevel2,				"kVRegHDMIInDrmLightLevel2");

		DefineRegister	(kVRegHDMIOutStatus1,					"kVRegHDMIOutStatus1",	mDecodeHDMIOutputStatus,	READWRITE,	kRegClass_HDMI,	kRegClass_Output, kRegClass_NULL);

		DefineRegName	(kVRegLastAJA,							"kVRegLastAJA");
		DefineRegName	(kVRegFirstOEM,							"kVRegFirstOEM");

		for (ULWord ndx (0);  ndx < 1024;  ndx++)
		{
			ostringstream	oss;
			string			foo;
			oss << "VIRTUALREG_START+" << ndx;
			const ULWord	regNum	(VIRTUALREG_START + ndx);
			const string regName (oss.str());
			if (!regName.empty())
				if (mRegNumToStringMap.find (regNum) == mRegNumToStringMap.end())
				{
					mRegNumToStringMap.insert (RegNumToStringPair (regNum, regName));
					mStringToRegNumMMap.insert (StringToRegNumPair (ToLower (regName), regNum));
				}
			DefineRegDecoder (regNum, mDefaultRegDecoder);
			DefineRegReadWrite (regNum, READWRITE);
			DefineRegClass (regNum, kRegClass_Virtual);
		}
		DefineRegClass (kVRegMonAncField1Offset, kRegClass_Anc);
		DefineRegClass (kVRegMonAncField2Offset, kRegClass_Anc);
		DefineRegClass (kVRegAncField1Offset, kRegClass_Anc);
		DefineRegClass (kVRegAncField2Offset, kRegClass_Anc);
	}	//	SetupVRegs

public:
	static ostream & PrintLabelValuePairs (ostream & oss, const AJALabelValuePairs & inLabelValuePairs)
	{
		for (AJALabelValuePairsConstIter it(inLabelValuePairs.begin());  it != inLabelValuePairs.end();  )
		{
			const string &	label	(it->first);
			const string &	value	(it->second);
			if (label.empty())
				;
			else if (label.at(label.length()-1) != ' '  &&  label.at(label.length()-1) != ':')	//	C++11 "label.back()" would be better
				oss << label << ": " << value;
			else if (label.at(label.length()-1) == ':')	//	C++11 "label.back()" would be better
				oss << label << " " << value;
			else
				oss << label << value;
			if (++it != inLabelValuePairs.end())
				oss << endl;
		}
		return oss;
	}

	string RegNameToString (const uint32_t inRegNum) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		RegNumToStringMap::const_iterator	iter	(mRegNumToStringMap.find (inRegNum));
		if (iter != mRegNumToStringMap.end())
			return iter->second;

		ostringstream	oss;	oss << "Reg ";
		if (inRegNum <= kRegNumRegisters)
			oss << DEC(inRegNum);
		else if (inRegNum <= 0x0000FFFF)
			oss << xHEX0N(inRegNum,4);
		else
			oss << xHEX0N(inRegNum,8);
		return oss.str();
	}
	
	string RegValueToString (const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		RegNumToDecoderMap::const_iterator	iter	(mRegNumToDecoderMap.find (inRegNum));
		ostringstream	oss;
		if (iter != mRegNumToDecoderMap.end()  &&  iter->second)
		{
			const Decoder * pDecoder (iter->second);
			oss << (*pDecoder)(inRegNum, inRegValue, inDeviceID);
		}
		return oss.str();
	}
	
	bool	IsRegInClass (const uint32_t inRegNum, const string & inClassName) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		for (RegClassToRegNumConstIter	it	(mRegClassToRegNumMMap.find (inClassName));  it != mRegClassToRegNumMMap.end() && it->first == inClassName;  ++it)
			if (it->second == inRegNum)
				return true;
		return false;
	}
	
	inline bool		IsRegisterWriteOnly (const uint32_t inRegNum) const				{return IsRegInClass (inRegNum, kRegClass_WriteOnly);}
	inline bool		IsRegisterReadOnly (const uint32_t inRegNum) const				{return IsRegInClass (inRegNum, kRegClass_ReadOnly);}
	
	NTV2StringSet	GetAllRegisterClasses (void) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		if (mAllRegClasses.empty())
			for (RegClassToRegNumConstIter	it	(mRegClassToRegNumMMap.begin ());  it != mRegClassToRegNumMMap.end();  ++it)
				if (mAllRegClasses.find (it->first) == mAllRegClasses.end())
					mAllRegClasses.insert (it->first);
		return mAllRegClasses;
	}
	
	NTV2StringSet	GetRegisterClasses (const uint32_t inRegNum) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		NTV2StringSet	result;
		NTV2StringSet	allClasses	(GetAllRegisterClasses());
		for (NTV2StringSetConstIter	it	(allClasses.begin ());  it != allClasses.end();  ++it)
			if (IsRegInClass (inRegNum, *it))
				result.insert (*it);
		return result;
	}
	
	NTV2RegNumSet	GetRegistersForClass (const string & inClassName) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		NTV2RegNumSet	result;
		for (RegClassToRegNumConstIter	it(mRegClassToRegNumMMap.find(inClassName));  it != mRegClassToRegNumMMap.end() && it->first == inClassName;  ++it)
			if (result.find(it->second) == result.end())
				result.insert(it->second);
		return result;
	}
	

	NTV2RegNumSet	GetRegistersForDevice (const NTV2DeviceID inDeviceID, const bool inIncludeVirtuals) const
	{
		static const string	chanClasses[] = {kRegClass_Channel1, kRegClass_Channel2, kRegClass_Channel3, kRegClass_Channel4, kRegClass_Channel5, kRegClass_Channel6, kRegClass_Channel7, kRegClass_Channel8};
		NTV2RegNumSet		result;
		const uint32_t		maxRegNum	(::NTV2DeviceGetMaxRegisterNumber(inDeviceID));

		for (uint32_t regNum (0);  regNum <= maxRegNum;  regNum++)
			result.insert(regNum);

		AJAAutoLock	lock(&mGuardMutex);

		if (::NTV2DeviceCanDoCustomAnc(inDeviceID))
		{
			const NTV2RegNumSet	ancRegs			(GetRegistersForClass(kRegClass_Anc));
			const UWord			numVideoOutputs	(::NTV2DeviceGetNumVideoOutputs(inDeviceID));
			NTV2RegNumSet		allChanRegs;	//	For just those channels it supports
			for (UWord num(0);  num < numVideoOutputs;  num++)
			{
				const NTV2RegNumSet chRegs (GetRegistersForClass(chanClasses[num]));
				allChanRegs.insert(chRegs.begin(), chRegs.end());
			}
			std::set_intersection (ancRegs.begin(), ancRegs.end(),  allChanRegs.begin(), allChanRegs.end(),  std::inserter(result, result.begin()));
		}

		if (::NTV2DeviceCanDoSDIErrorChecks(inDeviceID))
		{
			const NTV2RegNumSet	sdiErrRegs	(GetRegistersForClass(kRegClass_SDIError));
			result.insert(sdiErrRegs.begin(), sdiErrRegs.end());
		}

		if (::NTV2DeviceCanDoAudioMixer(inDeviceID))
		{
			for (ULWord regNum(kRegAudioMixerInputSelects);  regNum <= kRegAudioMixerAux2GainCh2;  regNum++)
				result.insert(regNum);
			for (ULWord regNum(kRegAudioMixerAux1InputLevels);  regNum <= kRegAudioMixerMainOutputLevelsPair7; regNum++)
				result.insert(regNum);
		}

		if (::NTV2DeviceHasXilinxDMA(inDeviceID))
		{
		}

		if (::NTV2DeviceCanDoEnhancedCSC(inDeviceID))
		{
			const NTV2RegNumSet	ecscRegs	(GetRegistersForClass(kRegClass_CSC));
			const UWord			numCSCs		(::NTV2DeviceGetNumCSCs(inDeviceID));
			NTV2RegNumSet		allChanRegs;	//	For just those CSCs it supports
			for (UWord num(0);  num < numCSCs;  num++)
			{
				const NTV2RegNumSet chRegs (GetRegistersForClass(chanClasses[num]));
				allChanRegs.insert(chRegs.begin(), chRegs.end());
			}
			std::set_intersection (ecscRegs.begin(), ecscRegs.end(),  allChanRegs.begin(), allChanRegs.end(),  std::inserter(result, result.begin()));
		}

		if (::NTV2DeviceGetNumLUTs(inDeviceID))
		{
			const NTV2RegNumSet LUTRegs (GetRegistersForClass(kRegClass_LUT));
			result.insert(LUTRegs.begin(), LUTRegs.end());
		}

		if (::NTV2DeviceGetNumHDMIVideoInputs(inDeviceID) > 1)	//	KonaHDMI
		{
			for (ULWord regNum = 0x1d00; regNum <= 0x1d1f; regNum++)
				result.insert(regNum);
			for (ULWord regNum = 0x2500; regNum <= 0x251f; regNum++)
				result.insert(regNum);
			for (ULWord regNum = 0x2c00; regNum <= 0x2c1f; regNum++)
				result.insert(regNum);
			for (ULWord regNum = 0x3000; regNum <= 0x301f; regNum++)
				result.insert(regNum);
		}
		else if (NTV2DeviceGetHDMIVersion(inDeviceID) > 3)		//	Io4KPlus, IoIP2022, IoIP2110, Kona5, KonaHDMI
		{	//	v4 HDMI: Io4K+, IoIP2022, IoIP2110, Kona5, KonaHDMI...
			for (ULWord regNum = 0x1d00; regNum <= 0x1d1f; regNum++)
				result.insert(regNum);
			for (ULWord regNum = 0x1d40; regNum <= 0x1d5f; regNum++)
				result.insert(regNum);
			for (ULWord regNum = 0x3C00; regNum <= 0x3C0A; regNum++)
				result.insert(regNum);
		}

		if (inIncludeVirtuals)
		{
			const NTV2RegNumSet	vRegs	(GetRegistersForClass(kRegClass_Virtual));
			result.insert(vRegs.begin(), vRegs.end());
		}
		return result;
	}

	
	NTV2RegNumSet	GetRegistersWithName (const string & inName, const int inMatchStyle = EXACTMATCH) const
	{
		NTV2RegNumSet	result;
		string			nameStr(inName);
		const size_t	nameStrLen(aja::lower(nameStr).length());
		StringToRegNumConstIter it;
		AJAAutoLock	lock(&mGuardMutex);
		if (inMatchStyle == EXACTMATCH)
		{
			it = mStringToRegNumMMap.find(nameStr);
			if (it != mStringToRegNumMMap.end())
				result.insert(it->second);
			return result;
		}
		//	Inexact match...
		for (it = mStringToRegNumMMap.begin();  it != mStringToRegNumMMap.end();  ++it)
		{
			const size_t pos(it->first.find(nameStr));
			if (pos == string::npos)
				continue;
			switch (inMatchStyle)
			{
				case CONTAINS:		result.insert(it->second);					break;
				case STARTSWITH:	if (pos == 0)
										{result.insert(it->second);}
									break;
				case ENDSWITH:		if (pos+nameStrLen == it->first.length())
										{result.insert(it->second);}
									break;
				default:			break;
			}
		}
		return result;
	}
	
	bool		GetXptRegNumAndMaskIndex (const NTV2InputCrosspointID inInputXpt, uint32_t & outXptRegNum, uint32_t & outMaskIndex) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		outXptRegNum = 0xFFFFFFFF;
		outMaskIndex = 0xFFFFFFFF;
		InputXpt2XptRegNumMaskIndexMapConstIter	iter	(mInputXpt2XptRegNumMaskIndexMap.find (inInputXpt));
		if (iter == mInputXpt2XptRegNumMaskIndexMap.end())
			return false;
		outXptRegNum = iter->second.first;
		outMaskIndex = iter->second.second;
		return true;
	}
	
	NTV2InputCrosspointID	GetInputCrosspointID (const uint32_t inXptRegNum, const uint32_t inMaskIndex) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		const XptRegNumAndMaskIndex				key		(inXptRegNum, inMaskIndex);
		XptRegNumMaskIndex2InputXptMapConstIter	iter	(mXptRegNumMaskIndex2InputXptMap.find (key));
		if (iter != mXptRegNumMaskIndex2InputXptMap.end())
			return iter->second;
		return NTV2_INPUT_CROSSPOINT_INVALID;
	}
	
	ostream &	Print (ostream & inOutStream) const
	{
		AJAAutoLock	lock(&mGuardMutex);
		static const string		sLineBreak	(96, '=');
		static const uint32_t	sMasks[4]	=	{0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000};
		
		inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of RegNumToStringMap:  " << mRegNumToStringMap.size() << " mappings:" << endl << sLineBreak << endl;
		for (RegNumToStringMap::const_iterator it (mRegNumToStringMap.begin());  it != mRegNumToStringMap.end();  ++it)
			inOutStream	<< "reg " << setw(5) << it->first << "(" << HEX0N(it->first,8) << dec << ")  =>  '" << it->second << "'" << endl;
		
		inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of RegNumToDecoderMap:  " << mRegNumToDecoderMap.size() << " mappings:" << endl << sLineBreak << endl;
		for (RegNumToDecoderMap::const_iterator it (mRegNumToDecoderMap.begin());  it != mRegNumToDecoderMap.end();  ++it)
			inOutStream	<< "reg " << setw(5) << it->first << "(" << HEX0N(it->first,8) << dec << ")  =>  " << (it->second == &mDefaultRegDecoder ? "(default decoder)" : "Custom Decoder") << endl;
		
		inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of RegClassToRegNumMMap:  " << mRegClassToRegNumMMap.size() << " mappings:" << endl << sLineBreak << endl;
		for (RegClassToRegNumMMap::const_iterator it (mRegClassToRegNumMMap.begin());  it != mRegClassToRegNumMMap.end();  ++it)
			inOutStream	<< setw(32) << it->first << "  =>  reg " << setw(5) << it->second << "(" << HEX0N(it->second,8) << dec << ") " << RegNameToString(it->second) << endl;
		
		inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of StringToRegNumMMap:  " << mStringToRegNumMMap.size() << " mappings:" << endl << sLineBreak << endl;
		for (StringToRegNumMMap::const_iterator it (mStringToRegNumMMap.begin());  it != mStringToRegNumMMap.end();  ++it)
			inOutStream	<< setw(32) << it->first << "  =>  reg " << setw(5) << it->second << "(" << HEX0N(it->second,8) << dec << ") " << RegNameToString(it->second) << endl;
		
		inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of InputXpt2XptRegNumMaskIndexMap:  " << mInputXpt2XptRegNumMaskIndexMap.size() << " mappings:" << endl << sLineBreak << endl;
		for (InputXpt2XptRegNumMaskIndexMap::const_iterator it (mInputXpt2XptRegNumMaskIndexMap.begin());  it != mInputXpt2XptRegNumMaskIndexMap.end();  ++it)
			inOutStream	<< setw(32) << ::NTV2InputCrosspointIDToString(it->first) << "(" << HEX0N(it->first,2)
			<< ")  =>  reg " << setw(3) << it->second.first << "(" << HEX0N(it->second.first,3) << dec << "|" << setw(20) << RegNameToString(it->second.first)
			<< ") mask " << it->second.second << "(" << HEX0N(sMasks[it->second.second],8) << ")" << endl;
		
		inOutStream	<< endl << sLineBreak << endl << "RegisterExpert:  Dump of XptRegNumMaskIndex2InputXptMap:  " << mXptRegNumMaskIndex2InputXptMap.size() << " mappings:" << endl << sLineBreak << endl;
		for (XptRegNumMaskIndex2InputXptMap::const_iterator it (mXptRegNumMaskIndex2InputXptMap.begin());  it != mXptRegNumMaskIndex2InputXptMap.end();  ++it)
			inOutStream	<< "reg " << setw(3) << it->first.first << "(" << HEX0N(it->first.first,4) << "|" << setw(20) << RegNameToString(it->first.first)
			<< ") mask " << it->first.second << "(" << HEX0N(sMasks[it->first.second],8) << ")  =>  "
			<< setw(27) << ::NTV2InputCrosspointIDToString(it->second) << "(" << HEX0N(it->second,2) << ")" << endl;
		return inOutStream;
	}
	
private:
	typedef std::map<uint32_t, string>	RegNumToStringMap;
	typedef std::pair<uint32_t, string>	RegNumToStringPair;
	
	static string ToLower (const string & inStr)
	{
		string	result (inStr);
		std::transform (result.begin (), result.end (), result.begin (), ::tolower);
		return result;
	}

	struct DecodeGlobalControlReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			const NTV2FrameGeometry		frameGeometry		(NTV2FrameGeometry		(((inRegValue & kRegMaskGeometry      ) >>  3)));
			const NTV2Standard			videoStandard		(NTV2Standard			((inRegValue & kRegMaskStandard       ) >>  7));
			const NTV2ReferenceSource	referenceSource		(NTV2ReferenceSource	((inRegValue & kRegMaskRefSource      ) >> 10));
			const NTV2RegisterWriteMode	registerWriteMode	(NTV2RegisterWriteMode	((inRegValue & kRegMaskRegClocking    ) >> 20));
			const NTV2FrameRate			frameRate			(NTV2FrameRate			(((inRegValue & kRegMaskFrameRate     ) >> kRegShiftFrameRate)
																					 | ((inRegValue & kRegMaskFrameRateHiBit) >> (kRegShiftFrameRateHiBit - 3))));
			ostringstream	oss;
			oss	<< "Frame Rate: "				<< ::NTV2FrameRateToString (frameRate, true)				<< endl
				<< "Frame Geometry: "			<< ::NTV2FrameGeometryToString (frameGeometry, true)		<< endl
				<< "Standard: "					<< ::NTV2StandardToString (videoStandard, true)				<< endl
				<< "Reference Source: "			<< ::NTV2ReferenceSourceToString (referenceSource, true)	<< endl
				<< "Ch 2 link B 1080p 50/60: "	<< ((inRegValue & kRegMaskSmpte372Enable) ? "On" : "Off")	<< endl
				<< "LEDs ";
			for (int led(0);  led < 4;  ++led)
				oss	<< (((inRegValue & kRegMaskLED) >> (16 + led))  ?  "*"  :  ".");
			oss	<< endl
				<< "Register Clocking: "		<< ::NTV2RegisterWriteModeToString (registerWriteMode, true).c_str() << endl
				<< "Ch 1 RP-188 output: "		<< EnabDisab(inRegValue & kRegMaskRP188ModeCh1) << endl
				<< "Ch 2 RP-188 output: "		<< EnabDisab(inRegValue & kRegMaskRP188ModeCh2) << endl
				<< "Color Correction: "			<< "Channel: " << ((inRegValue & BIT(31)) ? "2" : "1")
				<< " Bank " << ((inRegValue & BIT (30)) ? "1" : "0");
			return oss.str();
		}
		virtual	~DecodeGlobalControlReg()	{}
	}	mDecodeGlobalControlReg;

	//	reg 267 aka kRegGlobalControl2
	struct DecodeGlobalControl2 : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			static const ULWord	playCaptModes[]	= {	kRegMaskAud1PlayCapMode,kRegMaskAud2PlayCapMode,kRegMaskAud3PlayCapMode,kRegMaskAud4PlayCapMode,
													kRegMaskAud5PlayCapMode,kRegMaskAud6PlayCapMode,kRegMaskAud7PlayCapMode,kRegMaskAud8PlayCapMode};
			static const ULWord	rp188Modes[]	= {	0, 0, kRegMaskRP188ModeCh3,kRegMaskRP188ModeCh4,kRegMaskRP188ModeCh5,ULWord(kRegMaskRP188ModeCh6),kRegMaskRP188ModeCh7,kRegMaskRP188ModeCh8};
			static const ULWord	k425Masks[]		= {	kRegMask425FB12, kRegMask425FB34, kRegMask425FB56, kRegMask425FB78};
			static const ULWord	BLinkModes[]	= {	kRegMaskSmpte372Enable4, kRegMaskSmpte372Enable6, kRegMaskSmpte372Enable8};
			ostringstream	oss;
		    oss	<< "Reference source bit 4: "	<< SetNotset(inRegValue & kRegMaskRefSource2)			<< endl
				<< "Quad Mode Channel 1-4: "	<< SetNotset(inRegValue & kRegMaskQuadMode)				<< endl
				<< "Quad Mode Channel 5-8: "	<< SetNotset(inRegValue & kRegMaskQuadMode2)			<< endl
				<< "Independent Channel Mode: " << SetNotset(inRegValue & kRegMaskIndependentMode)		<< endl
				<< "2MB Frame Support: "		<< SuppNotsupp(inRegValue & kRegMask2MFrameSupport)		<< endl
				<< "Audio Mixer: "				<< PresNotPres(inRegValue & kRegMaskAudioMixerPresent)	<< endl
				<< "Is DNXIV Product: "			<< YesNo(inRegValue & kRegMaskIsDNXIV)					<< endl;
			for (unsigned ch(0);  ch < 8;  ch++)
				oss << "Audio " << DEC(ch+1) << " Play/Capture Mode: " << OnOff(inRegValue & playCaptModes[ch])	<< endl;
			for (unsigned ch(2);  ch < 8;  ch++)
				oss << "Ch " << DEC(ch+1) << " RP188 Output: " << EnabDisab(inRegValue & rp188Modes[ch])	<< endl;
			for (unsigned ch(0);  ch < 3;  ch++)
				oss << "Ch " << DEC(2*(ch+2)) << " 1080p50/p60 Link-B Mode: " << EnabDisab(inRegValue & BLinkModes[ch])	<< endl;
			for (unsigned ch(0);  ch < 4;  ch++)
				oss << "Ch " << DEC(ch+1) << "/" << DEC(ch+2) << " 2SI Mode: " << EnabDisab(inRegValue & k425Masks[ch])	<< endl;
			oss	<< "2SI Min Align Delay 1-4: "	<< EnabDisab(inRegValue & BIT(24))			<< endl
				<< "2SI Min Align Delay 5-8: "	<< EnabDisab(inRegValue & BIT(25));
			return oss.str();
		}
		virtual	~DecodeGlobalControl2()	{}
	}	mDecodeGlobalControl2;
	
	//	reg 108 aka kRegGlobalControl3
	struct DecodeGlobalControl3 : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			oss	<< "Bidirectional analog audio 1-4: "	<< (inRegValue & kRegMaskAnalogIOControl_14 ? "Receive" : "Transmit")	<< endl
				<< "Bidirectional analog audio 5-8: "	<< (inRegValue & kRegMaskAnalogIOControl_58 ? "Receive" : "Transmit")	<< endl
				<< "VU Meter Audio Select: "			<< (inRegValue & kRegMaskVUMeterSelect ? "AudMixer" : "AudSys1")		<< endl
				<< "Quad Quad Mode FrameStores 1-2: "	<< EnabDisab(inRegValue & kRegMaskQuadQuadMode)							<< endl
				<< "Quad Quad Mode FrameStores 3-4: "	<< EnabDisab(inRegValue & kRegMaskQuadQuadMode2)						<< endl
				<< "Quad Quad Squares Mode 1-4: "		<< EnabDisab(inRegValue & kRegMaskQuadQuadSquaresMode)					<< endl
				<< "Frame Pulse Enable: "				<< EnabDisab(inRegValue & kRegMaskFramePulseEnable);
			if (inRegValue & kRegMaskFramePulseEnable)
				oss	<< endl
					<< "Frame Pulse Ref Src: "	<< DEC((inRegValue & kRegMaskFramePulseRefSelect) >> kRegShiftFramePulseRefSelect);
			return oss.str();
		}
		virtual	~DecodeGlobalControl3()	{}
	}	mDecodeGlobalControl3;
	
	// Regs 377,378,379,380,381,382,383 aka kRegGlobalControlCh2,kRegGlobalControlCh3,kRegGlobalControlCh4,kRegGlobalControlCh5,kRegGlobalControlCh6,kRegGlobalControlCh7,kRegGlobalControlCh8
	struct DecodeGlobalControlChanReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			const NTV2FrameGeometry	frameGeometry	= NTV2FrameGeometry((inRegValue & kRegMaskGeometry) >> 3);
			const NTV2Standard		videoStandard	= NTV2Standard((inRegValue & kRegMaskStandard) >> 7);
			const NTV2FrameRate		frameRate		= NTV2FrameRate(((inRegValue & kRegMaskFrameRate) >> kRegShiftFrameRate) | ((inRegValue & kRegMaskFrameRateHiBit) >> (kRegShiftFrameRateHiBit - 3)));
			oss << "Frame Rate: "		<< ::NTV2FrameRateToString (frameRate)			<< endl
				<< "Frame Geometry: "	<< ::NTV2FrameGeometryToString (frameGeometry)	<< endl
				<< "Standard: "			<< ::NTV2StandardToString (videoStandard);
			return oss.str();
		}
		virtual	~DecodeGlobalControlChanReg()	{}
	}	mDecodeGlobalControlChanRegs;

	//	Regs 1/5/257/260/384/388/392/396  aka  kRegCh1Control,kRegCh2Control,kRegCh3Control,kRegCh4Control,kRegCh5Control,kRegCh6Control,kRegCh7Control,kRegCh8Control
	struct DecodeChannelControlReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			const ULWord	fbfUpper	((inRegValue & kRegMaskFrameFormatHiBit) >> 2);
			const ULWord	fbfLower	((inRegValue & kRegMaskFrameFormat) >> 1);
			oss	<< "Mode: "					<< (inRegValue & kRegMaskMode ? "Capture" : "Display")												<< endl
				<< "Format: "				<< ::NTV2FrameBufferFormatToString(NTV2PixelFormat(fbfUpper | fbfLower),false)						<< endl
				<< "Channel: "				<< DisabEnab(inRegValue & kRegMaskChannelDisable)													<< endl
				<< "Viper Squeeze: "		<< (inRegValue & BIT(9)						? "Squeeze"				: "Normal")						<< endl
				<< "Flip Vertical: "		<< (inRegValue & kRegMaskFrameOrientation	? "Upside Down"			: "Normal")						<< endl
				<< "DRT Display: "			<< OnOff(inRegValue & kRegMaskQuarterSizeMode)														<< endl
				<< "Frame Buffer Mode: "	<< (inRegValue & kRegMaskFrameBufferMode	? "Field"				: "Frame")						<< endl
				<< "Dither: "				<< (inRegValue & kRegMaskDitherOn8BitInput	? "Dither 8-bit inputs"	: "No dithering")				<< endl
				<< "Frame Size: "			<< (1 << (((inRegValue & kK2RegMaskFrameSize) >> 20) + 1)) << " MB"									<< endl;
			if (inRegNum == kRegCh1Control  &&  ::NTV2DeviceSoftwareCanChangeFrameBufferSize(inDeviceID))
				oss	<< "Frame Size Override: "	<< EnabDisab(inRegValue & kRegMaskFrameSizeSetBySW)												<< endl;
			oss	<< "RGB Range: "			<< (inRegValue & BIT(24)					? "Black = 0x40"		: "Black = 0")					<< endl
				<< "VANC Data Shift: "		<< (inRegValue & kRegMaskVidProcVANCShift	? "Enabled"				: "Normal 8 bit conversion");
			return oss.str();
		}
		virtual	~DecodeChannelControlReg()	{}
	}	mDecodeChannelControl;
	
	struct DecodeFBControlReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			const bool		isOn	((inRegValue & (1 << 29)) != 0);
			const uint16_t	format	((inRegValue >> 15) & 0x1F);
			ostringstream	oss;
			oss << OnOff(isOn)	<< endl
				<< "Format: "	<< xHEX0N(format,4) << " (" << DEC(format) << ")";
			return oss.str();
		}
		virtual	~DecodeFBControlReg()	{}
	}	mDecodeFBControlReg;

	struct DecodeChannelControlExtReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			oss	<< "Input Video 2:1 Decimate: "		<< EnabDisab(inRegValue & BIT(0))		<< endl
				<< "HDMI Rx Direct: "				<< EnabDisab(inRegValue & BIT(1))		<< endl
				<< "3:2 Pulldown Mode: "			<< EnabDisab(inRegValue & BIT(2));
			return oss.str();
		}
		virtual	~DecodeChannelControlExtReg()	{}
	}	mDecodeChannelControlExt;
	
	struct DecodeSysmonVccIntDieTemp : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			const UWord		rawDieTemp	((inRegValue & 0x0000FFFF) >> 6);
			const UWord		rawVoltage	((inRegValue >> 22) & 0x3FF);
			const double	dieTempC	((double(rawDieTemp)) * 503.975 / 1024.0 - 273.15 );
			const double	dieTempF	(dieTempC * 9.0 / 5.0  +  32.0);
			const double	voltage		(double(rawVoltage)/ 1024.0 * 3.0);
			ostringstream	oss;
			oss << "Die Temperature: " << fDEC(dieTempC,5,2) << " Celcius  (" << fDEC(dieTempF,5,2) << " Fahrenheit"	<< endl
				<< "Core Voltage: " << fDEC(voltage,5,2) << " Volts DC";
			return oss.str();
		}
		virtual	~DecodeSysmonVccIntDieTemp()	{}
	}	mDecodeSysmonVccIntDieTemp;

	struct DecodeSDITransmitCtrl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			ostringstream	oss;
			if (::NTV2DeviceHasBiDirectionalSDI(inDeviceID))
			{
				const uint32_t	txEnableBits	(((inRegValue & 0x0F000000) >> 20) | ((inRegValue & 0xF0000000) >> 28));
				const UWord		numInputs		(::NTV2DeviceGetNumVideoInputs(inDeviceID));
				const UWord		numOutputs		(::NTV2DeviceGetNumVideoOutputs(inDeviceID));
				const UWord		numSpigots		(numInputs > numOutputs  ?  numInputs  :  numOutputs);
				if (numSpigots)
					for (UWord spigot(0);  spigot < numSpigots;  )
					{
						const uint32_t	txEnabled	(txEnableBits & BIT(spigot));
						oss	<< "SDI " << DEC(++spigot) << ": " << (txEnabled ? "Output/Transmit" : "Input/Receive");
						if (spigot < numSpigots)
							oss << endl;
					}
				else
					oss << "(No SDI inputs or outputs)";
			}
			else
				oss	<< "(Bi-directional SDI not supported)";
			return oss.str();
		}
		virtual	~DecodeSDITransmitCtrl()	{}
	}	mDecodeSDITransmitCtrl;

	struct DecodeConversionCtrl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inRegNum;
			ostringstream oss;
			if (!::NTV2DeviceGetUFCVersion(inDeviceID))
			{
				const ULWord bitfileID((inRegValue & kK2RegMaskConverterInRate) >> kK2RegShiftConverterInRate);
				oss	<< "Bitfile ID: "				<< xHEX0N(bitfileID, 2)					<< endl
					<< "Memory Test: Start: "		<< YesNo(inRegValue & BIT(28))			<< endl
					<< "Memory Test: Done: "		<< YesNo(inRegValue & BIT(29))			<< endl
					<< "Memory Test: Passed: "		<< YesNo(inRegValue & BIT(30));
			}
			else
			{
				const NTV2Standard			inStd		(       NTV2Standard( inRegValue & kK2RegMaskConverterInStandard  ));
				const NTV2Standard			outStd		(       NTV2Standard((inRegValue & kK2RegMaskConverterOutStandard ) >> kK2RegShiftConverterOutStandard));
				const NTV2FrameRate			inRate		(      NTV2FrameRate((inRegValue & kK2RegMaskConverterInRate      ) >> kK2RegShiftConverterInRate));
				const NTV2FrameRate			outRate		(      NTV2FrameRate((inRegValue & kK2RegMaskConverterOutRate     ) >> kK2RegShiftConverterOutRate));
				const NTV2UpConvertMode		upCvtMode	(  NTV2UpConvertMode((inRegValue & kK2RegMaskUpConvertMode        ) >> kK2RegShiftUpConvertMode));
				const NTV2DownConvertMode	dnCvtMode	(NTV2DownConvertMode((inRegValue & kK2RegMaskDownConvertMode      ) >> kK2RegShiftDownConvertMode));
				const NTV2IsoConvertMode	isoCvtMode	( NTV2IsoConvertMode((inRegValue & kK2RegMaskIsoConvertMode       ) >> kK2RegShiftIsoConvertMode));
				oss	<< "Input Video Standard: "				<< ::NTV2StandardToString(inStd, true)					<< endl
					<< "Input Video Frame Rate: "			<< ::NTV2FrameRateToString(inRate, true)				<< endl
					<< "Output Video Standard: "			<< ::NTV2StandardToString(outStd, true)					<< endl
					<< "Output Video Frame Rate: "			<< ::NTV2FrameRateToString(outRate, true)				<< endl
					<< "Up Convert Mode: "					<< ::NTV2UpConvertModeToString(upCvtMode, true)			<< endl
					<< "Down Convert Mode: "				<< ::NTV2DownConvertModeToString(dnCvtMode, true)		<< endl
					<< "SD Anamorphic ISO Convert Mode: "	<< ::NTV2IsoConvertModeToString(isoCvtMode, true)		<< endl
					<< "DownCvt 2-3 Pulldown: "				<< EnabDisab(inRegValue & kK2RegMaskConverterPulldown)	<< endl
					<< "Vert Filter Preload: "				<< DisabEnab(inRegValue & BIT(7))						<< endl
					<< "Output Vid Std PsF (Deint Mode): "	<< EnabDisab(inRegValue & kK2RegMaskDeinterlaceMode)	<< endl
					<< "Up Conv Line21 Pass|Blank Mode: "	<< DEC(ULWord(inRegValue & kK2RegMaskUCPassLine21) >> kK2RegShiftUCAutoLine21)	<< endl
					<< "UFC Clock: "						<< EnabDisab(inRegValue & kK2RegMaskEnableConverter);
			}
			return oss.str();
		}
	}	mConvControlRegDecoder;

	struct DecodeRelayCtrlStat : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			ostringstream	oss;
			if (::NTV2DeviceHasSDIRelays(inDeviceID))
			{
				oss	<< "SDI1-SDI2 Relay Control: "	<< ThruDeviceOrBypassed(inRegValue & kRegMaskSDIRelayControl12)		<< endl
					<< "SDI3-SDI4 Relay Control: "	<< ThruDeviceOrBypassed(inRegValue & kRegMaskSDIRelayControl34)		<< endl
					<< "SDI1-SDI2 Relay Watchdog: "	<< EnabDisab(inRegValue & kRegMaskSDIWatchdogEnable12)				<< endl
					<< "SDI3-SDI4 Relay Watchdog: "	<< EnabDisab(inRegValue & kRegMaskSDIWatchdogEnable34)				<< endl
					<< "SDI1-SDI2 Relay Position: "	<< ThruDeviceOrBypassed(inRegValue & kRegMaskSDIRelayPosition12)	<< endl
					<< "SDI3-SDI4 Relay Position: "	<< ThruDeviceOrBypassed(inRegValue & kRegMaskSDIRelayPosition34)	<< endl
					<< "Watchdog Timer Status: "	<< ThruDeviceOrBypassed(inRegValue & kRegMaskSDIWatchdogStatus);
			}
			else
				oss	<< "(SDI bypass relays not supported)";
			return oss.str();
		}
		virtual	~DecodeRelayCtrlStat()	{}
	}	mDecodeRelayCtrlStat;

	struct DecodeWatchdogTimeout : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			ostringstream	oss;
			if (::NTV2DeviceHasSDIRelays(inDeviceID))
			{
				const uint32_t	ticks8nanos	(inRegValue);	//	number of 8-nanosecond ticks
				const double	microsecs	(double(ticks8nanos) * 8.0 / 1000.0);
				const double	millisecs	(microsecs / 1000.0);
				oss << "Watchdog Timeout [8-ns ticks]: " << xHEX0N(ticks8nanos,8) << " (" << DEC(ticks8nanos) << ")" << endl
					<< "Watchdog Timeout [usec]: " << microsecs << endl
					<< "Watchdog Timeout [msec]: " << millisecs;
			}
			else
				oss	<< "(SDI bypass relays not supported)";
			return oss.str();
		}
		virtual	~DecodeWatchdogTimeout()	{}
	}	mDecodeWatchdogTimeout;

	struct DecodeWatchdogKick : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			ostringstream	oss;
			if (::NTV2DeviceHasSDIRelays(inDeviceID))
			{
				const uint32_t	whichReg(inRegNum - kRegSDIWatchdogKick1);
				NTV2_ASSERT(whichReg < 2);
				const uint32_t expectedValue(whichReg ? 0x01234567 : 0xA5A55A5A);
				oss << xHEX0N(inRegValue,8);
				if (inRegValue == expectedValue)
					oss << " (Normal)";
				else
					oss << " (Not expected, should be " << xHEX0N(expectedValue,8) << ")";
			}
			else
				oss	<< "(SDI bypass relays not supported)";
			return oss.str();
		}
		virtual	~DecodeWatchdogKick()	{}
	}	mDecodeWatchdogKick;

	struct DecodeInputVPID: public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			const uint32_t		regValue (NTV2EndianSwap32(inRegValue));	//	Input VPID register values require endian-swap
			ostringstream		oss;
			AJALabelValuePairs	info;
			const CNTV2VPID		ntv2vpid(regValue);
			PrintLabelValuePairs(oss, ntv2vpid.GetInfo(info));
			return oss.str();
		}
		virtual	~DecodeInputVPID()	{}
	}	mVPIDInpRegDecoder;

	struct DecodeOutputVPID: public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream		oss;
			AJALabelValuePairs	info;
			const CNTV2VPID		ntv2vpid(inRegValue);
			PrintLabelValuePairs(oss, ntv2vpid.GetInfo(info));
			return oss.str();
		}
		virtual	~DecodeOutputVPID()	{}
	}	mVPIDOutRegDecoder;

	struct DecodeBitfileDateTime : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			ostringstream	oss;
			if (inRegNum == kRegBitfileDate)
			{
				const UWord		yyyy	((inRegValue & 0xFFFF0000) >> 16);
				const UWord		mm		((inRegValue & 0x0000FF00) >> 8);
				const UWord		dd		(inRegValue & 0x000000FF);
				if (yyyy > 0x2015  &&  mm > 0  &&  mm < 0x13  &&  dd > 0  &&  dd < 0x32)
					oss << "Bitfile Date: " << HEX0N(mm,2) << "/" << HEX0N(dd,2) << "/"	<< HEX0N(yyyy,4);
				else
					oss << "Bitfile Date: " << xHEX0N(inRegValue, 8);
			}
			else if (inRegNum == kRegBitfileTime)
			{
				const UWord		hh	((inRegValue & 0x00FF0000) >> 16);
				const UWord		mm	((inRegValue & 0x0000FF00) >> 8);
				const UWord		ss	(inRegValue & 0x000000FF);
				if (hh < 0x24  &&  mm < 0x60  &&  ss < 0x60)
					oss << "Bitfile Time: " << HEX0N(hh,2) << ":" << HEX0N(mm,2) << ":"	<< HEX0N(ss,2);
				else
					oss << "Bitfile Time: " << xHEX0N(inRegValue, 8);
			}
			else NTV2_ASSERT(false);	//	impossible
			return oss.str();
		}
		virtual	~DecodeBitfileDateTime()	{}
	}	mDecodeBitfileDateTime;

	struct DecodeCanDoStatus : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inRegNum;	(void) inDeviceID;
			ostringstream	oss;
			oss	<< "Has CanConnect Xpt Route ROM: "		<< YesNo(inRegValue & BIT(0));
			return oss.str();
		}
		virtual	~DecodeCanDoStatus()	{}
	}	mDecodeCanDoStatus;

	struct DecodeVidControlReg : public Decoder		//	Bit31=Is16x9 | Bit30=IsMono
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			const bool		is16x9	((inRegValue & BIT(31)) != 0);
			const bool		isMono	((inRegValue & BIT(30)) != 0);
			ostringstream	oss;
			oss << "Aspect Ratio: " << (is16x9 ? "16x9" : "4x3") << endl
				<< "Depth: " << (isMono ? "Monochrome" : "Color");
			return oss.str();
		}
		virtual	~DecodeVidControlReg()	{}
	}	mDecodeVidControlReg;

	struct DecodeVidIntControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			oss	<< "Output 1 Vertical Enable: "		<< YesNo(inRegValue & BIT(0))					<< endl
				<< "Input 1 Vertical Enable: "		<< YesNo(inRegValue & BIT(1))					<< endl
				<< "Input 2 Vertical Enable: "		<< YesNo(inRegValue & BIT(2))					<< endl
				<< "Audio Out Wrap Interrupt Enable: "	<< YesNo(inRegValue & BIT(4))				<< endl
				<< "Audio In Wrap Interrupt Enable: "	<< YesNo(inRegValue & BIT(5))				<< endl
				<< "Wrap Rate Interrupt Enable: "	<< YesNo(inRegValue & BIT(6))					<< endl
				<< "UART Tx Interrupt Enable"		<< YesNo(inRegValue & BIT(7))					<< endl
				<< "UART Rx Interrupt Enable"		<< YesNo(inRegValue & BIT(8))					<< endl
				<< "UART Rx Interrupt Clear"		<< ActInact(inRegValue & BIT(15))				<< endl
				<< "UART 2 Tx Interrupt Enable"		<< YesNo(inRegValue & BIT(17))					<< endl
				<< "Output 2 Vertical Enable: "		<< YesNo(inRegValue & BIT(18))					<< endl
				<< "Output 3 Vertical Enable: "		<< YesNo(inRegValue & BIT(19))					<< endl
				<< "Output 4 Vertical Enable: "		<< YesNo(inRegValue & BIT(20))					<< endl
				<< "Output 4 Vertical Clear: "		<< ActInact(inRegValue & BIT(21))				<< endl
				<< "Output 3 Vertical Clear: "		<< ActInact(inRegValue & BIT(22))				<< endl
				<< "Output 2 Vertical Clear: "		<< ActInact(inRegValue & BIT(23))				<< endl
				<< "UART Tx Interrupt Clear"		<< ActInact(inRegValue & BIT(24))				<< endl
				<< "Wrap Rate Interrupt Clear"		<< ActInact(inRegValue & BIT(25))				<< endl
				<< "UART 2 Tx Interrupt Clear"		<< ActInact(inRegValue & BIT(26))				<< endl
				<< "Audio Out Wrap Interrupt Clear"	<< ActInact(inRegValue & BIT(27))				<< endl
				<< "Input 2 Vertical Clear: "		<< ActInact(inRegValue & BIT(29))				<< endl
				<< "Input 1 Vertical Clear: "		<< ActInact(inRegValue & BIT(30))				<< endl
				<< "Output 1 Vertical Clear: "		<< ActInact(inRegValue & BIT(31));
			return oss.str();
		}
		virtual	~DecodeVidIntControl()	{}
	}	mDecodeVidIntControl;
	
	struct DecodeVidIntControl2 : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			oss	<< "Input 3 Vertical Enable: "		<< YesNo(inRegValue & BIT(1))					<< endl
				<< "Input 4 Vertical Enable: "		<< YesNo(inRegValue & BIT(2))					<< endl
				<< "Input 5 Vertical Enable: "		<< YesNo(inRegValue & BIT(8))					<< endl
				<< "Input 6 Vertical Enable: "		<< YesNo(inRegValue & BIT(9))					<< endl
				<< "Input 7 Vertical Enable: "		<< YesNo(inRegValue & BIT(10))					<< endl
				<< "Input 8 Vertical Enable: "		<< YesNo(inRegValue & BIT(11))					<< endl
				<< "Output 5 Vertical Enable: "		<< YesNo(inRegValue & BIT(12))					<< endl
				<< "Output 6 Vertical Enable: "		<< YesNo(inRegValue & BIT(13))					<< endl
				<< "Output 7 Vertical Enable: "		<< YesNo(inRegValue & BIT(14))					<< endl
				<< "Output 8 Vertical Enable: "		<< YesNo(inRegValue & BIT(15))					<< endl
				<< "Output 8 Vertical Clear: "		<< ActInact(inRegValue & BIT(16))				<< endl
				<< "Output 7 Vertical Clear: "		<< ActInact(inRegValue & BIT(17))				<< endl
				<< "Output 6 Vertical Clear: "		<< ActInact(inRegValue & BIT(18))				<< endl
				<< "Output 5 Vertical Clear: "		<< ActInact(inRegValue & BIT(19))				<< endl
				<< "Input 8 Vertical Clear: "		<< ActInact(inRegValue & BIT(25))				<< endl
				<< "Input 7 Vertical Clear: "		<< ActInact(inRegValue & BIT(26))				<< endl
				<< "Input 6 Vertical Clear: "		<< ActInact(inRegValue & BIT(27))				<< endl
				<< "Input 5 Vertical Clear: "		<< ActInact(inRegValue & BIT(28))				<< endl
				<< "Input 4 Vertical Clear: "		<< ActInact(inRegValue & BIT(29))				<< endl
				<< "Input 3 Vertical Clear: "		<< ActInact(inRegValue & BIT(30));
			return oss.str();
		}
		virtual	~DecodeVidIntControl2()	{}
	}	mDecodeVidIntControl2;

	struct DecodeStatusReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			oss	<< "Input 1 Vertical Blank: "		<< ActInact(inRegValue & BIT(20))					<< endl
				<< "Input 1 Field ID: "				<< (inRegValue & BIT(21) ? "1" : "0")				<< endl
				<< "Input 1 Vertical Interrupt: "	<< ActInact(inRegValue & BIT(30))					<< endl
				<< "Input 2 Vertical Blank: "		<< ActInact(inRegValue & BIT(18))					<< endl
				<< "Input 2 Field ID: "				<< (inRegValue & BIT(19) ? "1" : "0")				<< endl
				<< "Input 2 Vertical Interrupt: "	<< ActInact(inRegValue & BIT(29))					<< endl
				<< "Output 1 Vertical Blank: "		<< ActInact(inRegValue & BIT(22))					<< endl
				<< "Output 1 Field ID: "			<< (inRegValue & BIT(23) ? "1" : "0")				<< endl
				<< "Output 1 Vertical Interrupt: "	<< ActInact(inRegValue & BIT(31))					<< endl
				<< "Output 2 Vertical Blank: "		<< ActInact(inRegValue & BIT(4))					<< endl
				<< "Output 2 Field ID: "			<< (inRegValue & BIT(5) ? "1" : "0")				<< endl
				<< "Output 2 Vertical Interrupt: "	<< ActInact(inRegValue & BIT(8))					<< endl;
			if (::NTV2DeviceGetNumVideoOutputs(inDeviceID) > 2)
				oss	<< "Output 3 Vertical Blank: "		<< ActInact(inRegValue & BIT(2))				<< endl
					<< "Output 3 Field ID: "			<< (inRegValue & BIT(3) ? "1" : "0")			<< endl
					<< "Output 3 Vertical Interrupt: "	<< ActInact(inRegValue & BIT(7))				<< endl
					<< "Output 4 Vertical Blank: "		<< ActInact(inRegValue & BIT(0))				<< endl
					<< "Output 4 Field ID: "			<< (inRegValue & BIT(1) ? "1" : "0")			<< endl
					<< "Output 4 Vertical Interrupt: "	<< ActInact(inRegValue & BIT(6))				<< endl;
			oss	<< "Aux Vertical Interrupt: "		<< ActInact(inRegValue & BIT(12))					<< endl
				<< "I2C 1 Interrupt: "				<< ActInact(inRegValue & BIT(14))					<< endl
				<< "I2C 2 Interrupt: "				<< ActInact(inRegValue & BIT(13))					<< endl
				<< "Chunk Rate Interrupt: "			<< ActInact(inRegValue & BIT(16))					<< endl;
			if (::NTV2DeviceGetNumSerialPorts(inDeviceID))
				oss	<< "Generic UART Interrupt: "	<< ActInact(inRegValue & BIT(9))					<< endl
					<< "Uart 1 Rx Interrupt: "		<< ActInact(inRegValue & BIT(15))					<< endl
					<< "Uart 1 Tx Interrupt: "		<< ActInact(inRegValue & BIT(24))					<< endl;
			if (::NTV2DeviceGetNumSerialPorts(inDeviceID) > 1)
				oss	<< "Uart 2 Tx Interrupt: "		<< ActInact(inRegValue & BIT(26))					<< endl;
			if (::NTV2DeviceGetNumLTCInputs(inDeviceID))
				oss	<< "LTC In 1 Present: "			<< YesNo(inRegValue & BIT(17))						<< endl;
			oss	<< "Wrap Rate Interrupt: "			<< ActInact(inRegValue & BIT(25))					<< endl
				<< "Audio Out Wrap Interrupt: "		<< ActInact(inRegValue & BIT(27))					<< endl
				<< "Audio 50Hz Interrupt: "			<< ActInact(inRegValue & BIT(28));
			return oss.str();
		}
		virtual	~DecodeStatusReg()	{}
	}	mDecodeStatusReg;

	struct DecodeCPLDVersion : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			oss	<< "CPLD Version: "					<< DEC(inRegValue & (BIT(0)|BIT(1)))	<< endl
				<< "Failsafe Bitfile Loaded: "		<< (inRegValue & BIT(4) ? "Yes" : "No")	<< endl
				<< "Force Reload: "					<< YesNo(inRegValue & BIT(8));
			return oss.str();
		}
		virtual	~DecodeCPLDVersion()	{}
	}	mDecodeCPLDVersion;

	struct DecodeStatus2Reg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			static const uint8_t	bitNumsInputVBlank[]	=	{20, 18, 16, 14, 12, 10};	//	Input 3/4/5/6/7/8 Vertical Blank
			static const uint8_t	bitNumsInputFieldID[]	=	{21, 19, 17, 15, 13, 11};	//	Input 3/4/5/6/7/8 Field ID
			static const uint8_t	bitNumsInputVertInt[]	=	{30, 29, 28, 27, 26, 25};	//	Input 3/4/5/6/7/8 Vertical Interrupt
			static const uint8_t	bitNumsOutputVBlank[]	=	{ 8,  6,  4,  2};			//	Output 5/6/7/8 Vertical Blank
			static const uint8_t	bitNumsOutputFieldID[]	=	{ 9,  7,  5,  3};			//	Output 5/6/7/8 Field ID
			static const uint8_t	bitNumsOutputVertInt[]	=	{31, 24, 23, 22};			//	Output 5/6/7/8 Vertical Interrupt
			ostringstream	oss;
			for (unsigned ndx(0);  ndx < 6;  ndx++)
				oss	<< "Input " << (ndx+3) << " Vertical Blank: "		<< ActInact(inRegValue & BIT(bitNumsInputVBlank[ndx]))			<< endl
					<< "Input " << (ndx+3) << " Field ID: "				<< (inRegValue & BIT(bitNumsInputFieldID[ndx]) ? "1" : "0")		<< endl
					<< "Input " << (ndx+3) << " Vertical Interrupt: "	<< ActInact(inRegValue & BIT(bitNumsInputVertInt[ndx]))			<< endl;
			for (unsigned ndx(0);  ndx < 4;  ndx++)
				oss	<< "Output " << (ndx+5) << " Vertical Blank: "		<< ActInact(inRegValue & BIT(bitNumsOutputVBlank[ndx]))			<< endl
					<< "Output " << (ndx+5) << " Field ID: "			<< (inRegValue & BIT(bitNumsOutputFieldID[ndx]) ? "1" : "0")	<< endl
					<< "Output " << (ndx+5) << " Vertical Interrupt: "	<< ActInact(inRegValue & BIT(bitNumsOutputVertInt[ndx]))		<< endl;
			oss	<< "HDMI In Hot-Plug Detect Interrupt: "	<< ActInact(inRegValue & BIT(0))	<< endl
				<< "HDMI In Chip Interrupt: "				<< ActInact(inRegValue & BIT(1));
			return oss.str();
		}
		virtual	~DecodeStatus2Reg()	{}
	}	mDecodeStatus2Reg;

	struct DecodeInputStatusReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			NTV2FrameRate	fRate1	(NTV2FrameRate( (inRegValue & (BIT( 0)|BIT( 1)|BIT( 2)        ))        | ((inRegValue & BIT(28)) >> (28-3)) ));
			NTV2FrameRate	fRate2	(NTV2FrameRate(((inRegValue & (BIT( 8)|BIT( 9)|BIT(10)        )) >>  8) | ((inRegValue & BIT(29)) >> (29-3)) ));
			NTV2FrameRate	fRateRf	(NTV2FrameRate(((inRegValue & (BIT(16)|BIT(17)|BIT(18)|BIT(19))) >> 16)                                      ));
			ostringstream	oss;
			oss	<< "Input 1 Frame Rate: " << ::NTV2FrameRateToString(fRate1, true) << endl
				<< "Input 1 Geometry: ";
			if (BIT(30) & inRegValue)
				switch (((BIT(4)|BIT(5)|BIT(6)) & inRegValue) >> 4)
			{
				case 0:		oss << "2K x 1080";		break;
				case 1:		oss << "2K x 1556";		break;
				default:	oss << "Invalid HI";	break;
			}
			else
				switch (((BIT(4)|BIT(5)|BIT(6)) & inRegValue) >> 4)
			{
				case 0:				oss << "Unknown";		break;
				case 1:				oss << "525";			break;
				case 2:				oss << "625";			break;
				case 3:				oss << "750";			break;
				case 4:				oss << "1125";			break;
				case 5:				oss << "1250";			break;
				case 6:	case 7:		oss << "Reserved";		break;
				default:			oss << "Invalid LO";	break;
			}
			oss	<< endl
				<< "Input 1 Scan Mode: "	<< ((BIT(7) & inRegValue) ? "Progressive" : "Interlaced") << endl
				<< "Input 2 Frame Rate: " << ::NTV2FrameRateToString(fRate2, true) << endl
				<< "Input 2 Geometry: ";
			if (BIT(31) & inRegValue)
				switch (((BIT(12)|BIT(13)|BIT(14)) & inRegValue) >> 12)
			{
				case 0:		oss << "2K x 1080";		break;
				case 1:		oss << "2K x 1556";		break;
				default:	oss << "Invalid HI";	break;
			}
			else
				switch (((BIT(12)|BIT(13)|BIT(14)) & inRegValue) >> 12)
			{
				case 0:				oss << "Unknown";		break;
				case 1:				oss << "525";			break;
				case 2:				oss << "625";			break;
				case 3:				oss << "750";			break;
				case 4:				oss << "1125";			break;
				case 5:				oss << "1250";			break;
				case 6:	case 7:		oss << "Reserved";		break;
				default:			oss << "Invalid LO";	break;
			}
			oss	<< endl
				<< "Input 2 Scan Mode: "	<< ((BIT(15) & inRegValue) ? "Progressive" : "Interlaced") << endl
				<< "Reference Frame Rate: " << ::NTV2FrameRateToString(fRateRf, true) << endl
				<< "Reference Geometry: ";
			switch (((BIT(20)|BIT(21)|BIT(22)|BIT(23)) & inRegValue) >> 20)
			{
				case 0:					oss << "Unknown";		break;
				case 1:					oss << "525";			break;
				case 2:					oss << "625";			break;
				case 3:					oss << "750";			break;
				case 4:					oss << "1125";			break;
				case 5: case 6: case 7:	oss << "N/A";			break;
				default:				oss << "Invalid";		break;
			}
			oss	<< endl
				<< "Reference Scan Mode: " << ((BIT(23) & inRegValue) ? "Progressive" : "Interlaced") << endl
				<< "AES Channel 1-2: " << ((BIT(24) & inRegValue) ? "Invalid" : "Valid") << endl
				<< "AES Channel 3-4: " << ((BIT(25) & inRegValue) ? "Invalid" : "Valid") << endl
				<< "AES Channel 5-6: " << ((BIT(26) & inRegValue) ? "Invalid" : "Valid") << endl
				<< "AES Channel 7-8: " << ((BIT(27) & inRegValue) ? "Invalid" : "Valid");
			return oss.str();
		}
		virtual	~DecodeInputStatusReg()	{}
	}	mDecodeInputStatusReg;

	struct DecodeSDIInputStatusReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			uint16_t		numSpigots(0), startSpigot(0), doTsiMuxSync(0);
			ostringstream	oss;
			switch (inRegNum)
			{
				case kRegSDIInput3GStatus:		numSpigots = 2;		startSpigot = 1;	doTsiMuxSync = 1;	break;
				case kRegSDIInput3GStatus2:		numSpigots = 2;		startSpigot = 3;	break;
				case kRegSDI5678Input3GStatus:	numSpigots = 4;		startSpigot = 5;	break;
			}
			if ((startSpigot-1) >= ::NTV2DeviceGetNumVideoInputs(inDeviceID))
				return oss.str();	//	Skip if no such SDI inputs
				
			for (uint16_t spigotNdx(0);  spigotNdx < numSpigots;  )
			{
				const uint16_t	spigotNum	(spigotNdx + startSpigot);
				const uint8_t	statusBits	((inRegValue >> (spigotNdx*8)) & 0xFF);
				const uint8_t	speedBits	(statusBits & 0xC1);
				ostringstream	ossSpeed, ossSpigot;
				ossSpigot << "SDI In " << spigotNum << " ";
				const string	spigotLabel	(ossSpigot.str());
				if (speedBits & 0x01)	ossSpeed << " 3G";
				if (::NTV2DeviceCanDo12GSDI(inDeviceID))
				{
					if (speedBits & 0x40)	ossSpeed << " 6G";
					if (speedBits & 0x80)	ossSpeed << " 12G";
				}
				if (speedBits == 0)		ossSpeed << " 1.5G";
				oss	<< spigotLabel << "Link Speed:"				<< ossSpeed.str()				<< endl
					<< spigotLabel << "SMPTE Level B: "			<< YesNo(statusBits & 0x02)		<< endl
					<< spigotLabel << "Link A VPID Valid: "		<< YesNo(statusBits & 0x10)		<< endl
					<< spigotLabel << "Link B VPID Valid: "		<< YesNo(statusBits & 0x20)		<< endl;
				if (::NTV2DeviceCanDo3GLevelConversion(inDeviceID))
					oss	<< spigotLabel << "3Gb-to-3Ga Conversion: "	<< EnabDisab(statusBits & 0x04);
				else
					oss	<< spigotLabel << "3Gb-to-3Ga Conversion: n/a";
				if (++spigotNdx < numSpigots)
					oss << endl;
			}	//	for each spigot
			if (doTsiMuxSync  &&  ::NTV2DeviceCanDo425Mux(inDeviceID))
				for (UWord tsiMux(0);  tsiMux < 4;  ++tsiMux)
					oss	<< endl
						<< "TsiMux" << DEC(tsiMux+1) << " Sync Fail: " << ((inRegValue & (0x00010000UL << tsiMux)) ? "FAILED" : "OK");
			return oss.str();
		}
		virtual	~DecodeSDIInputStatusReg()	{}
	}	mDecodeSDIInputStatusReg;

	struct DecodeSDIInputStatus2Reg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			const string sOdd	(inRegNum == kRegInputStatus2 ? "Input 3" : (inRegNum == kRegInput56Status ? "Input 5" : "Input 7"));
			const string sEven	(inRegNum == kRegInputStatus2 ? "Input 4" : (inRegNum == kRegInput56Status ? "Input 6" : "Input 8"));
			const NTV2FrameRate	fRate1	(NTV2FrameRate( (inRegValue & (BIT( 0)|BIT( 1)|BIT( 2) ))        | ((inRegValue & BIT(28)) >> (28-3)) ));
			const NTV2FrameRate	fRate2	(NTV2FrameRate(((inRegValue & (BIT( 8)|BIT( 9)|BIT(10) )) >>  8) | ((inRegValue & BIT(29)) >> (29-3)) ));
			ostringstream	oss;
			oss	<< sOdd << " Scan Mode: "	<< ((BIT(7) & inRegValue) ? "Progressive" : "Interlaced") << endl
				<< sOdd << " Frame Rate: " << ::NTV2FrameRateToString(fRate1, true) << endl
				<< sOdd << " Geometry: ";
			if (BIT(30) & inRegValue) switch (((BIT(4)|BIT(5)|BIT(6)) & inRegValue) >> 4)
			{
				case 0:				oss << "2K x 1080";		break;
				case 1:				oss << "2K x 1556";		break;
				default:			oss << "Invalid HI";	break;
			}
			else switch (((BIT(4)|BIT(5)|BIT(6)) & inRegValue) >> 4)
			{
				case 0:				oss << "Unknown";		break;
				case 1:				oss << "525";			break;
				case 2:				oss << "625";			break;
				case 3:				oss << "750";			break;
				case 4:				oss << "1125";			break;
				case 5:				oss << "1250";			break;
				case 6:	case 7:		oss << "Reserved";		break;
				default:			oss << "Invalid LO";	break;
			}
			oss	<< endl
				<< sEven << " Scan Mode: "	<< ((BIT(15) & inRegValue) ? "Progressive" : "Interlaced") << endl
				<< sEven << " Frame Rate: " << ::NTV2FrameRateToString(fRate2, true) << endl
				<< sEven << " Geometry: ";
			if (BIT(31) & inRegValue) switch (((BIT(12)|BIT(13)|BIT(14)) & inRegValue) >> 12)
			{
				case 0:				oss << "2K x 1080";		break;
				case 1:				oss << "2K x 1556";		break;
				default:			oss << "Invalid HI";	break;
			}
			else switch (((BIT(12)|BIT(13)|BIT(14)) & inRegValue) >> 12)
			{
				case 0:				oss << "Unknown";		break;
				case 1:				oss << "525";			break;
				case 2:				oss << "625";			break;
				case 3:				oss << "750";			break;
				case 4:				oss << "1125";			break;
				case 5:				oss << "1250";			break;
				case 6:	case 7:		oss << "Reserved";		break;
				default:			oss << "Invalid LO";	break;
			}
			return oss.str();
		}
		virtual	~DecodeSDIInputStatus2Reg()	{}
	}	mDecodeSDIInputStatus2Reg;

	struct DecodeFS1RefSelectReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;	(void) inRegNum;	//	kRegFS1ReferenceSelect
			ostringstream	oss;
			oss		<< "BNC Select(LHi): "				<< (inRegValue & 0x00000010 ? "LTCIn1" : "Ref")		<< endl
					<< "Ref BNC (Corvid): "				<< EnabDisab(inRegValue & 0x00000020)				<< endl
					<< "LTC Present (also Reg 21): "	<< YesNo(inRegValue & 0x00000040)					<< endl
					<< "LTC Emb Out Enable: "			<< YesNo(inRegValue & 0x00000080)					<< endl
					<< "LTC Emb In Enable: "			<< YesNo(inRegValue & 0x00000100)					<< endl
					<< "LTC Emb In Received: "			<< YesNo(inRegValue & 0x00000200)					<< endl
					<< "LTC BNC Out Source: "			<< (inRegValue & 0x00000400 ? "E-E" : "Reg112/113");
			return oss.str();
		}
	}	mDecodeFS1RefSelectReg;

	struct DecodeLTCStatusControlReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;	(void) inRegNum;	//	kRegLTCStatusControl
			const uint16_t	LTC1InTimingSelect	((inRegValue >> 1) & 0x0000007);
			const uint16_t	LTC2InTimingSelect	((inRegValue >> 9) & 0x0000007);
			const uint16_t	LTC1OutTimingSelect	((inRegValue >> 16) & 0x0000007);
			const uint16_t	LTC2OutTimingSelect	((inRegValue >> 20) & 0x0000007);
			ostringstream	oss;
			oss		<< "LTC 1 Input Present: "				<< YesNo(inRegValue & 0x00000001)												<< endl
					<< "LTC 1 Input FB Timing Select): "	<< xHEX0N(LTC1InTimingSelect,2) << " (" << DEC(LTC1InTimingSelect) << ")"		<< endl
					<< "LTC 1 Bypass: "						<< EnabDisab(inRegValue & 0x00000010)											<< endl
					<< "LTC 1 Bypass Select: "				<< DEC(ULWord((inRegValue >> 5) & 0x00000001))									<< endl
					<< "LTC 2 Input Present: "				<< YesNo(inRegValue & 0x00000100)												<< endl
					<< "LTC 2 Input FB Timing Select): "	<< xHEX0N(LTC2InTimingSelect,2) << " (" << DEC(LTC2InTimingSelect) << ")"		<< endl
					<< "LTC 2 Bypass: "						<< EnabDisab(inRegValue & 0x00001000)											<< endl
					<< "LTC 2 Bypass Select: "				<< DEC(ULWord((inRegValue >> 13) & 0x00000001))									<< endl
					<< "LTC 1 Output FB Timing Select): "	<< xHEX0N(LTC1OutTimingSelect,2) << " (" << DEC(LTC1OutTimingSelect) << ")"		<< endl
					<< "LTC 2 Output FB Timing Select): "	<< xHEX0N(LTC2OutTimingSelect,2) << " (" << DEC(LTC2OutTimingSelect) << ")";
			return oss.str();
		}
	}	mLTCStatusControlDecoder;

	struct DecodeAudDetectReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			ostringstream	oss;
			switch (inRegNum)
			{
				case kRegAud1Detect:
				case kRegAudDetect2:
					for (uint16_t num(0);  num < 8;  )
					{
						const uint16_t	group		(num / 2);
						const bool		isChan34	(num & 1);
						oss		<< "Group " << group << " CH " << (isChan34 ? "3-4: " : "1-2: ") << (inRegValue & BIT(num) ? "Present" : "Absent");
						if (++num < 8)
							oss << endl;
					}
					break;
					
				case kRegAudioDetect5678:
					break;
			}
			return oss.str();
		}
		virtual	~DecodeAudDetectReg()	{}
	}	mDecodeAudDetectReg;
	
	struct DecodeAudControlReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			static const string	ChStrs []	=	{	"Ch 1/2",	"Ch 3/4",	"Ch 5/6",	"Ch 7/8"	};
			uint16_t			sdiOutput	(0);
			switch (inRegNum)
			{	case kRegAud1Control:	sdiOutput = 1;	break;
				case kRegAud3Control:	sdiOutput = 3;	break;
				case kRegAud5Control:	sdiOutput = 5;	break;
				case kRegAud7Control:	sdiOutput = 7;	break;
				default:								break;
			}
			
			ostringstream		oss;
			oss		<< "Audio Capture: "		<< EnabDisab(BIT(0) & inRegValue)	<< endl
					<< "Audio Loopback: "		<< EnabDisab(BIT(3) & inRegValue)	<< endl
					<< "Audio Input: "			<< DisabEnab(BIT(8) & inRegValue)	<< endl
					<< "Audio Output: "			<< DisabEnab(BIT(9) & inRegValue)	<< endl;
			if (sdiOutput)
				oss	<< "Audio Embedder SDIOut" << sdiOutput		<< ": " << DisabEnab(BIT(13) & inRegValue)	<< endl
					<< "Audio Embedder SDIOut" << (sdiOutput+1)	<< ": " << DisabEnab(BIT(15) & inRegValue)	<< endl;
			
			oss		<< "A/V Sync Mode: "		<< EnabDisab(BIT(15) & inRegValue)	<< endl
					<< "AES Rate Converter: "	<< DisabEnab(BIT(19) & inRegValue)	<< endl
					<< "Audio Buffer Format: "	<< (BIT(20) & inRegValue ? "16-Channel " : (BIT(16) & inRegValue ? "8-Channel " : "6-Channel "))	<< endl
					<< (BIT(18) & inRegValue ? "96kHz" : "48kHz")									<< endl
					<< (BIT(18) & inRegValue ? "96kHz Support" : "48kHz Support")					<< endl
					<< (BIT(22) & inRegValue ? "Embedded Support" : "No Embedded Support")			<< endl
					<< (BIT(23) & inRegValue ? "8-Channel Support" : "6-Channel Support")			<< endl
					<< "K-box, Monitor: "		<< ChStrs [(BIT(24) & BIT(25) & inRegValue) >> 24]	<< endl
					<< "K-Box Input: "			<< (BIT(26) & inRegValue ? "XLR" : "BNC")			<< endl
					<< "K-Box: "				<< (BIT(27) & inRegValue ? "Present" : "Absent")	<< endl
					<< "Cable: "				<< (BIT(28) & inRegValue ? "XLR" : "BNC")			<< endl
					<< "Audio Buffer Size: "	<< (BIT(31) & inRegValue ? "4 MB" : "1 MB");
			return oss.str();
		}
		virtual	~DecodeAudControlReg()	{}
	}	mDecodeAudControlReg;
	
	struct DecodeAudSourceSelectReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			static const string		SrcStrs []		=	{	"AES Input",	"Embedded Groups 1 and 2",	""	};
			static const unsigned	SrcStrMap []	=	{	0,	1,	2,	2,	2,	1,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2	};
			const uint16_t			vidInput		=	(inRegValue & BIT(23) ? 2 : 0)  +  (inRegValue & BIT(16) ? 1 : 0);
			//	WARNING!  BIT(23) had better be clear on 0 & 1-input boards!!
			ostringstream	oss;
			oss	<< "Audio Source: "	<< SrcStrs [SrcStrMap [(BIT(0) | BIT(1) | BIT(2) | BIT(3)) & inRegValue]]	<< endl
				<< "Embedded Source Select: Video Input " << (1 + vidInput)										<< endl
				<< "AES Sync Mode bit (fib): "	<< EnabDisab(inRegValue & BIT(18))								<< endl
				<< "PCM disabled: "				<< YesNo(inRegValue & BIT(17))									<< endl
				<< "Erase head enable: "		<< YesNo(inRegValue & BIT(19))									<< endl
				<< "Embedded Clock Select: "	<< (inRegValue & BIT(22) ? "Video Input" : "Board Reference")	<< endl
				<< "3G audio source: "			<< (inRegValue & BIT(21) ? "Data stream 2" : "Data stream 1");
			return oss.str();
		}
		virtual	~DecodeAudSourceSelectReg()	{}
	}	mDecodeAudSourceSelectReg;

	struct DecodeAudOutputSrcMap : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			static const string	AESOutputStrs[]	= {	"AES Outputs 1-4",	"AES Outputs 5-8",	"AES Outputs 9-12",	"AES Outputs 13-16", ""};
			static const string	SrcStrs[]		= {	"AudSys1, Audio Channels 1-4",	"AudSys1, Audio Channels 5-8",
													"AudSys1, Audio Channels 9-12",	"AudSys1, Audio Channels 13-16",
													"AudSys2, Audio Channels 1-4",	"AudSys2, Audio Channels 5-8",
													"AudSys2, Audio Channels 9-12",	"AudSys2, Audio Channels 13-16",
													"AudSys3, Audio Channels 1-4",	"AudSys3, Audio Channels 5-8",
													"AudSys3, Audio Channels 9-12",	"AudSys3, Audio Channels 13-16",
													"AudSys4, Audio Channels 1-4",	"AudSys4, Audio Channels 5-8",
													"AudSys4, Audio Channels 9-12",	"AudSys4, Audio Channels 13-16",	""};
			static const unsigned		AESChlMappingShifts [4]	=	{0, 4, 8, 12};

			ostringstream	oss;
			const uint32_t				AESOutMapping	(inRegValue & 0x0000FFFF);
			const uint32_t				AnlgMonInfo		((inRegValue & kRegMaskMonitorSource) >> kRegShiftMonitorSource);
			const NTV2AudioSystem		AnlgMonAudSys	(NTV2AudioSystem(AnlgMonInfo >> 4));
			const NTV2AudioChannelPair	AnlgMonChlPair	(NTV2AudioChannelPair(AnlgMonInfo & 0xF));
			for (unsigned AESOutputQuad(0);  AESOutputQuad < 4;  AESOutputQuad++)
				oss	<< AESOutputStrs[AESOutputQuad] << " Source: " << SrcStrs[(AESOutMapping >> AESChlMappingShifts[AESOutputQuad]) & 0x0000000F] << endl;
			oss	<< "Analog Audio Monitor Output Source: " << ::NTV2AudioSystemToString(AnlgMonAudSys,true) << ", Channels " << ::NTV2AudioChannelPairToString(AnlgMonChlPair,true) << endl;

			//	HDMI Audio Output Mapping -- interpretation depends on bit 29 of register 125 kRegHDMIOutControl		MULTIREG_SPARSE_BITS
			const uint32_t				HDMIMonInfo		((inRegValue & kRegMaskHDMIOutAudioSource) >> kRegShiftHDMIOutAudioSource);
			{
				//	HDMI Audio 2-channel Mode:
				const NTV2AudioSystem		HDMIMonAudSys	(NTV2AudioSystem(HDMIMonInfo >> 4));
				const NTV2AudioChannelPair	HDMIMonChlPair	(NTV2AudioChannelPair(HDMIMonInfo & 0xF));
				oss	<< "HDMI 2-Chl Audio Output Source: " << ::NTV2AudioSystemToString(HDMIMonAudSys,true) << ", Channels " << ::NTV2AudioChannelPairToString(HDMIMonChlPair,true) << endl;
			}
			{
				//	HDMI Audio 8-channel Mode:
				const uint32_t					HDMIMon1234Info		(HDMIMonInfo & 0x0F);
				const NTV2AudioSystem			HDMIMon1234AudSys	(NTV2AudioSystem(HDMIMon1234Info >> 2));
				const NTV2Audio4ChannelSelect	HDMIMon1234SrcPairs	(NTV2Audio4ChannelSelect(HDMIMon1234Info & 0x3));
				const uint32_t					HDMIMon5678Info		((HDMIMonInfo >> 4) & 0x0F);
				const NTV2AudioSystem			HDMIMon5678AudSys	(NTV2AudioSystem(HDMIMon5678Info >> 2));
				const NTV2Audio4ChannelSelect	HDMIMon5678SrcPairs	(NTV2Audio4ChannelSelect(HDMIMon5678Info & 0x3));
				oss	<< "or HDMI 8-Chl Audio Output 1-4 Source: " << ::NTV2AudioSystemToString(HDMIMon1234AudSys,true) << ", Channels " << ::NTV2AudioChannelQuadToString(HDMIMon1234SrcPairs,true) << endl
					<< "or HDMI 8-Chl Audio Output 5-8 Source: " << ::NTV2AudioSystemToString(HDMIMon5678AudSys,true) << ", Channels " << ::NTV2AudioChannelQuadToString(HDMIMon5678SrcPairs,true);
			}
			return oss.str();
		}
		virtual	~DecodeAudOutputSrcMap()	{}
	}	mDecodeAudOutputSrcMap;

	struct DecodePCMControlReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			ostringstream	oss;
			const UWord		startAudioSystem (inRegNum == kRegPCMControl4321  ?  1  :  5);
			for (uint8_t audChan (0);  audChan < 4;  audChan++)
			{
				oss << "Audio System " << (startAudioSystem + audChan) << ": ";
				const uint8_t	pcmBits	(uint32_t(inRegValue >> (audChan * 8)) & 0x000000FF);
				if (pcmBits == 0x00)
					oss << "normal";
				else
				{
					oss << "non-PCM channels";
					for (uint8_t chanPair (0);  chanPair < 8;  chanPair++)
						if (pcmBits & (0x01 << chanPair))
							oss << "  " << (chanPair*2+1) << "-" << (chanPair*2+2);
				}
				if (audChan < 3)
					oss << endl;
			}
			return oss.str();
		}
		virtual	~DecodePCMControlReg()	{}
	}	mDecodePCMControlReg;

	struct DecodeAudioMixerInputSelectReg : public Decoder
	{	//	kRegAudioMixerInputSelects
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inDeviceID;	(void) inRegNum;
			const UWord	mainInputSrc((inRegValue     ) & 0x0000000F);
			const UWord	aux1InputSrc((inRegValue >> 4) & 0x0000000F);
			const UWord	aux2InputSrc((inRegValue >> 8) & 0x0000000F);
			ostringstream	oss;
			oss	<< "Main Input Source: "	<< ::NTV2AudioSystemToString(NTV2AudioSystem(mainInputSrc))	<< " (bits 0-3)" << endl
				<< "Aux Input 1 Source: "	<< ::NTV2AudioSystemToString(NTV2AudioSystem(aux1InputSrc))	<< " (bits 4-7)" << endl
				<< "Aux Input 2 Source: "	<< ::NTV2AudioSystemToString(NTV2AudioSystem(aux2InputSrc))	<< " (bits 8-11)";
			return oss.str();
		}
		virtual	~DecodeAudioMixerInputSelectReg()	{}
	}	mAudMxrInputSelDecoder;

	struct DecodeAudioMixerGainRegs : public Decoder
	{	//	kRegAudioMixerMainGain,
		//	kRegAudioMixerAux1GainCh1, kRegAudioMixerAux1GainCh2,
		//	kRegAudioMixerAux2GainCh1, kRegAudioMixerAux2GainCh2
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void)inRegNum;	(void)inDeviceID;
			static const double	kUnityGain	(0x00010000);
			const bool			atUnity		(inRegValue == 0x00010000);
			ostringstream		oss;
			if (atUnity)
				oss	<< "Gain: 0 dB (Unity)";
			else
			{
				const double	dValue		(inRegValue);
				const bool		aboveUnity	(inRegValue >= 0x00010000);
				const string	plusMinus	(atUnity ? "" : (aboveUnity ? "+" : "-"));
				const string	aboveBelow	(atUnity ? "at" : (aboveUnity ? "above" : "below"));
				const uint32_t	unityDiff	(aboveUnity ? inRegValue - 0x00010000 : 0x00010000 - inRegValue);
				const double	dB			(double(20.0) * ::log10(dValue/kUnityGain));
				oss	<< "Gain: "	<< dB << " dB, " << plusMinus << xHEX0N(unityDiff,6)
								<< " (" << plusMinus << DEC(unityDiff) << ") " << aboveBelow << " unity gain";
			}
			return oss.str();
		}
		virtual	~DecodeAudioMixerGainRegs()	{}
	}	mAudMxrGainDecoder;

	struct DecodeAudioMixerChannelSelectReg : public Decoder
	{	//	kRegAudioMixerChannelSelect
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inRegNum;	(void) inDeviceID;
			ostringstream	oss;
			const uint32_t	mainChanPair((inRegValue & kRegMaskAudioMixerChannelSelect   ) >> kRegShiftAudioMixerChannelSelect   );
			const uint32_t	powerOfTwo	((inRegValue & kRegMaskAudioMixerLevelSampleCount) >> kRegShiftAudioMixerLevelSampleCount);
			oss	<< "Main Input Source Channel Pair: "	<< ::NTV2AudioChannelPairToString(NTV2AudioChannelPair(mainChanPair)) << " (bits 0-2)" << endl
				<< "Level Measurement Sample Count: "	<< DEC(ULWord(1 << powerOfTwo)) << " (bits 8-15)";
			return oss.str();
		}
		virtual	~DecodeAudioMixerChannelSelectReg()	{}
	}	mAudMxrChanSelDecoder;


	struct DecodeAudioMixerMutesReg : public Decoder
	{	//	kRegAudioMixerMutes
		protected:
			typedef std::bitset<16>	AudioChannelSet16;
			typedef std::bitset<2>	AudioChannelSet2;
			static void SplitAudioChannelSet16(const AudioChannelSet16 & inChSet, NTV2StringList & outSet, NTV2StringList & outClear)
			{
				outSet.clear();  outClear.clear();
				for (size_t ndx(0);  ndx < 16;  ndx++)
				{	ostringstream oss;  oss << DEC(ndx+1);
					if (inChSet.test(ndx))
						outSet.push_back(oss.str());
					else
						outClear.push_back(oss.str());
				}
				if (outSet.empty())		outSet.push_back("<none>");
				if (outClear.empty())	outClear.push_back("<none>");
			}
			static void SplitAudioChannelSet2(const AudioChannelSet2 & inChSet, NTV2StringList & outSet, NTV2StringList & outClear)
			{
				outSet.clear();  outClear.clear();  static const string LR[] = {"L", "R"};
				for (size_t ndx(0);  ndx < 2;  ndx++)
					if (inChSet.test(ndx))
						outSet.push_back(LR[ndx]);
					else
						outClear.push_back(LR[ndx]);
				if (outSet.empty())		outSet.push_back("<none>");
				if (outClear.empty())	outClear.push_back("<none>");
			}
		public:
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inRegNum;	(void) inDeviceID;
			uint32_t	mainOutputMuteBits	((inRegValue & kRegMaskAudioMixerOutputChannelsMute) >> kRegShiftAudioMixerOutputChannelsMute);	//	Bits 0-15
			uint32_t	mainInputMuteBits	((inRegValue & kRegMaskAudioMixerMainInputEnable   ) >> kRegShiftAudioMixerMainInputEnable   );	//	Bits 16-17
			uint32_t	aux1InputMuteBits	((inRegValue & kRegMaskAudioMixerAux1InputEnable   ) >> kRegShiftAudioMixerAux1InputEnable   );	//	Bits 18-19
			uint32_t	aux2InputMuteBits	((inRegValue & kRegMaskAudioMixerAux2InputEnable   ) >> kRegShiftAudioMixerAux2InputEnable   );	//	Bits 20-21
			ostringstream	oss;
			NTV2StringList mutedMainOut, unmutedMainOut, mutedMain, unmutedMain, mutedAux1, unmutedAux1, mutedAux2, unmutedAux2;
			SplitAudioChannelSet16(AudioChannelSet16(mainOutputMuteBits), mutedMainOut, unmutedMainOut);
			SplitAudioChannelSet2(AudioChannelSet2(mainInputMuteBits), mutedMain, unmutedMain);
			SplitAudioChannelSet2(AudioChannelSet2(aux1InputMuteBits), mutedAux1, unmutedAux1);
			SplitAudioChannelSet2(AudioChannelSet2(aux2InputMuteBits), mutedAux2, unmutedAux2);
			oss	<< "Main Output Muted/Disabled Channels: "	<< mutedMainOut << endl	//	bits[0:15]
				<< "Main Output Unmuted/Enabled Channels: "	<< unmutedMainOut << endl;
			oss	<< "Main Input Muted/Disabled Channels: "	<< mutedMain	<< endl	//	bits[16:17]
				<< "Main Input Unmuted/Enabled Channels: "	<< unmutedMain	<< endl;
			oss	<< "Aux Input 1 Muted/Disabled Channels: "	<< mutedAux1	<< endl	//	bits[18:19]
				<< "Aux Input 1 Unmuted/Enabled Channels: "	<< unmutedAux1	<< endl;
			oss	<< "Aux Input 2 Muted/Disabled Channels: "	<< mutedAux2	<< endl	//	bits[20-21]
				<< "Aux Input 2 Unmuted/Enabled Channels: "	<< unmutedAux2;
			return oss.str();
		}
		virtual	~DecodeAudioMixerMutesReg()	{}
	}	mAudMxrMutesDecoder;

	struct DecodeAudioMixerLevelsReg : public Decoder
	{	//	kRegAudioMixerAux1InputLevels, kRegAudioMixerAux2InputLevels,
		//	kRegAudioMixerMainInputLevelsPair0 thru kRegAudioMixerMainInputLevelsPair7,
		//	kRegAudioMixerMixedChannelOutputLevels
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inDeviceID;
			static const string sLabels[] = {	"Aux Input 1", "Aux Input 2", "Main Input Audio Channels 1|2", "Main Input Audio Channels 3|4",
												"Main Input Audio Channels 5|6", "Main Input Audio Channels 7|8", "Main Input Audio Channels 9|10",
												"Main Input Audio Channels 11|12", "Main Input Audio Channels 13|14", "Main Input Audio Channels 15|16",
												"Main Output Audio Channels 1|2", "Main Output Audio Channels 3|4", "Main Output Audio Channels 5|6",
												"Main Output Audio Channels 7|8", "Main Output Audio Channels 9|10", "Main Output Audio Channels 11|12",
												"Main Output Audio Channels 13|14", "Main Output Audio Channels 15|16"};
			NTV2_ASSERT(inRegNum >= kRegAudioMixerAux1InputLevels);
			const uint32_t	labelOffset(inRegNum - kRegAudioMixerAux1InputLevels);
			NTV2_ASSERT(labelOffset < 18);
			const string &	label(sLabels[labelOffset]);
			const uint16_t	leftLevel	((inRegValue & kRegMaskAudioMixerInputLeftLevel) >> kRegShiftAudioMixerInputLeftLevel);
			const uint16_t	rightLevel	((inRegValue & kRegMaskAudioMixerInputRightLevel) >> kRegShiftAudioMixerInputRightLevel);
			ostringstream	oss;
			oss	<< label << " Left Level:"	<< xHEX0N(leftLevel, 4) << " (" << DEC(leftLevel) << ")" << endl	//	bits[0:15]
				<< label << " Right Level:"	<< xHEX0N(rightLevel,4) << " (" << DEC(rightLevel) << ")";			//	bits[16:31]
			return oss.str();
		}
		virtual	~DecodeAudioMixerLevelsReg()	{}
	}	mAudMxrLevelDecoder;

	struct DecodeAncExtControlReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			static const string	SyncStrs []	=	{	"field",	"frame",	"immediate",	"unknown"	};
			oss	<< "HANC Y enable: "		<< YesNo(inRegValue & BIT( 0))	<< endl
				<< "VANC Y enable: "		<< YesNo(inRegValue & BIT( 4))	<< endl
				<< "HANC C enable: "		<< YesNo(inRegValue & BIT( 8))	<< endl
				<< "VANC C enable: "		<< YesNo(inRegValue & BIT(12))	<< endl
				<< "Progressive video: "	<< YesNo(inRegValue & BIT(16))	<< endl
				<< "Synchronize: "			<< SyncStrs [(inRegValue & (BIT(24) | BIT(25))) >> 24]	<< endl
				<< "Memory writes: "		<< EnabDisab(!(inRegValue & BIT(28)))					<< endl
				<< "SD Y+C Demux: "			<< EnabDisab(inRegValue & BIT(30))						<< endl
				<< "Metadata from: "		<< (inRegValue & BIT(31) ? "LSBs" : "MSBs");
			return oss.str();
		}
		virtual	~DecodeAncExtControlReg()	{}
	}	mDecodeAncExtControlReg;
	
	struct DecodeAncExtFieldLinesReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			ostringstream	oss;
			const uint32_t	which		(inRegNum & 0x1F);
			const uint32_t	valueLow	(inRegValue & 0x7FF);
			const uint32_t	valueHigh	((inRegValue >> 16) & 0x7FF);
			switch (which)
			{
				case 5:		oss << "F1 cutoff line: "			<< valueLow	<< endl		//	regAncExtFieldCutoffLine
								<< "F2 cutoff line: "			<< valueHigh;
					break;
				case 9:		oss	<< "F1 VBL start line: "		<< valueLow	<< endl		//	regAncExtFieldVBLStartLine
								<< "F2 VBL start line: "		<< valueHigh;
					break;
				case 11:	oss	<< "Field ID high on line: "	<< valueLow	<< endl		//	regAncExtFID
								<< "Field ID low on line: "		<< valueHigh;
					break;
				case 17:	oss	<< "F1 analog start line: "		<< valueLow	<< endl		//	regAncExtAnalogStartLine
								<< "F2 analog start line: "		<< valueHigh;
					break;
				default:
					oss	<< "Invalid register type";
					break;
			}
			return oss.str();
		}
		virtual	~DecodeAncExtFieldLinesReg()	{}
	}	mDecodeAncExtFieldLines;
	
	struct DecodeAncExtStatusReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			ostringstream	oss;
			const uint32_t	which		(inRegNum & 0x1F);
			const uint32_t	byteTotal	(inRegValue & 0xFFFFFF);
			const bool		overrun		((inRegValue & BIT(28)) ? true : false);
			switch (which)
			{
				case 6:		oss	<< "Total bytes: ";			break;
				case 7:		oss	<< "Total F1 bytes: ";		break;
				case 8:		oss	<< "Total F2 bytes: ";		break;
				default:	oss << "Invalid register type";	break;
			}
			oss	<< DEC(byteTotal)	<< endl
				<< "Overrun: "	<< YesNo(overrun);
			return oss.str();
		}
		virtual	~DecodeAncExtStatusReg()	{}
	}	mDecodeAncExtStatus;
	
	struct DecodeAncExtIgnoreDIDReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			oss	<< "Ignoring DIDs " << HEX0N((inRegValue >> 0) & 0xFF, 2)
				<< ", " << HEX0N((inRegValue >> 8) & 0xFF, 2)
				<< ", " << HEX0N((inRegValue >> 16) & 0xFF, 2)
				<< ", " << HEX0N((inRegValue >> 24) & 0xFF, 2);
			return oss.str();
		}
		virtual	~DecodeAncExtIgnoreDIDReg()	{}
	}	mDecodeAncExtIgnoreDIDs;
	
	struct DecodeAncExtAnalogFilterReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegValue;
			(void) inDeviceID;
			ostringstream	oss;
			uint32_t		which	(inRegNum & 0x1F);
			oss	<< "Each 1 bit specifies capturing ";
			switch (which)
			{
				case 18:	oss << "F1 Y";		break;
				case 19:	oss << "F2 Y";		break;
				case 20:	oss << "F1 C";		break;
				case 21:	oss << "F2 C";		break;
				default:	return "Invalid register type";
			}
			oss << " line as analog, else digital";
			return oss.str();
		}
		virtual	~DecodeAncExtAnalogFilterReg()	{}
	}	mDecodeAncExtAnalogFilter;
	
	struct DecodeAncInsValuePairReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			ostringstream	oss;
			const uint32_t	which		(inRegNum & 0x1F);
			const uint32_t	valueLow	(inRegValue & 0xFFFF);
			const uint32_t	valueHigh	((inRegValue >> 16) & 0xFFFF);
			
			switch (which)
			{
				case 0:		oss	<< "F1 byte count low: "			<< valueLow				<< endl
								<< "F2 byte count low: "			<< valueHigh;
					break;
				case 4:		oss	<< "HANC pixel delay: "				<< (valueLow & 0x3FF)	<< endl
								<< "VANC pixel delay: "				<< (valueHigh & 0x7FF);
					break;
				case 5:		oss	<< "F1 first active line: "			<< (valueLow & 0x7FF)	<< endl
								<< "F2 first active line: "			<< (valueHigh & 0x7FF);
					break;
				case 6:		oss	<< "Active line length: "			<< (valueLow & 0x7FF)	<< endl
								<< "Total line length: "			<< (valueHigh & 0xFFF);
					break;
				case 8:		oss	<< "Field ID high on line: "		<< (valueLow & 0x7FF)	<< endl
								<< "Field ID low on line: "			<< (valueHigh & 0x7FF);
					break;
				case 11:	oss	<< "F1 chroma blnk start line: "	<< (valueLow & 0x7FF)	<< endl
								<< "F2 chroma blnk start line: "	<< (valueHigh & 0x7FF);
					break;
				case 14:	oss	<< "F1 byte count high: "			<< valueLow				<< endl
								<< "F2 byte count high: "			<< valueHigh;
					break;
				default:	return "Invalid register type";
			}
			return oss.str();
		}
		virtual	~DecodeAncInsValuePairReg()	{}
	}	mDecodeAncInsValuePairReg;
	
	struct DecodeAncInsControlReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			oss	<< "HANC Y enable: "		<< YesNo(inRegValue & BIT( 0))			<< endl
				<< "VANC Y enable: "		<< YesNo(inRegValue & BIT( 4))			<< endl
				<< "HANC C enable: "		<< YesNo(inRegValue & BIT( 8))			<< endl
				<< "VANC C enable: "		<< YesNo(inRegValue & BIT(12))			<< endl
				<< "Payload Y insert: "		<< YesNo(inRegValue & BIT(16))			<< endl
				<< "Payload C insert: "		<< YesNo(inRegValue & BIT(17))			<< endl
				<< "Payload F1 insert: "	<< YesNo(inRegValue & BIT(20))			<< endl
				<< "Payload F2 insert: "	<< YesNo(inRegValue & BIT(21))			<< endl
				<< "Progressive video: "	<< YesNo(inRegValue & BIT(24))			<< endl
				<< "Memory reads: "			<< EnabDisab(!(inRegValue & BIT(28)))	<< endl
				<< "SD Packet Split: "		<< EnabDisab(inRegValue & BIT(31));
			return oss.str();
		}
		virtual	~DecodeAncInsControlReg()	{}
	}	mDecodeAncInsControlReg;
	
	struct DecodeAncInsChromaBlankReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegValue;
			(void) inDeviceID;
			ostringstream	oss;
			uint32_t		which	(inRegNum & 0x1F);
			
			oss	<< "Each 1 bit specifies if chroma in ";
			switch (which)
			{
				case 12:	oss	<< "F1";	break;
				case 13:	oss	<< "F2";	break;
				default:	return "Invalid register type";
			}
			oss	<< " should be blanked or passed thru";
			return oss.str();
		}
		virtual	~DecodeAncInsChromaBlankReg()	{}
	}	mDecodeAncInsChromaBlankReg;
	
	struct DecodeXptGroupReg : public Decoder		//	Every byte is a crosspoint number 0-255
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			static unsigned	sShifts[4]	= {0, 8, 16, 24};
			ostringstream	oss;
			for (unsigned ndx(0);  ndx < 4;  ndx++)
			{
				const NTV2InputCrosspointID		inputXpt	(CNTV2RegisterExpert::GetInputCrosspointID (inRegNum, ndx));
				const NTV2OutputCrosspointID	outputXpt	(static_cast <NTV2OutputCrosspointID> ((inRegValue >> sShifts[ndx]) & 0xFF));
				if (NTV2_IS_VALID_InputCrosspointID (inputXpt))
					oss << ::NTV2InputCrosspointIDToString (inputXpt, false) << " <== " << ::NTV2OutputCrosspointIDToString (outputXpt, false);
				if (ndx < 3)
					oss << endl;
			}
			return oss.str();
		}
		virtual	~DecodeXptGroupReg()	{}
	}	mDecodeXptGroupReg;

	struct DecodeXptValidReg : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inDeviceID;
			NTV2_ASSERT(inRegNum >= uint32_t(kRegFirstValidXptROMRegister));
			NTV2_ASSERT(inRegNum < uint32_t(kRegInvalidValidXptROMRegister));
			ostringstream	oss;
			NTV2InputXptID	inputXpt;
			NTV2OutputXptIDSet	outputXpts;
			if (CNTV2SignalRouter::GetRouteROMInfoFromReg (inRegNum, inRegValue, inputXpt, outputXpts)
				&& NTV2_IS_VALID_InputCrosspointID(inputXpt))
			{
				NTV2StringList	outputXptNames;
				for (NTV2OutputXptIDSetConstIter it(outputXpts.begin());  it != outputXpts.end();  ++it)
				{
					const NTV2OutputXptID outputXpt(*it);
					const string name(::NTV2OutputCrosspointIDToString(outputXpt,true));
					ostringstream ss;
					if (name.empty())
						ss << xHEX0N(outputXpt,2) << "(" << DEC(outputXpt) << ")";
					else
						ss << "'" << name << "'";
					outputXptNames.push_back(ss.str());
				}
				if (!outputXptNames.empty())
					oss << "Valid Xpts: " << outputXptNames;
				return oss.str();
			}
			else
				return Decoder::operator()(inRegNum, inRegValue, inDeviceID);
		}
		virtual	~DecodeXptValidReg()	{}
	}	mDecodeXptValidReg;

	struct DecodeHDMIOutputControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			ostringstream	oss;
            static const string	sHDMIStdV1[]	=	{	"1080i",	"720p",	"480i",	"576i",	"1080p",	"SXGA", "", "", "", "", "", "", "", "", "", "" };
			static const string	sHDMIStdV2V3[]	=	{	"1080i",	"720p",	"480i",	"576i",	"1080p",	"1556i",	"2Kx1080p",	"2Kx1080i",	"UHD",	"4K", "", "", "", "", "", "" };
			static const string	sVidRates[]		=	{	"",	"60.00",	"59.94",	"30.00",	"29.97",	"25.00",	"24.00",	"23.98",	"50.00",	"48.00",	"47.95", "", "", "", "", "" };
			static const string	sSrcSampling[]	=	{	"YC422",	"RGB",	"YC420",	"Unknown/invalid"	};
			static const string	sBitDepth[]		=	{	"8",		"10",	"12",		"Unknown/invalid"	};
            const ULWord	hdmiVers		(::NTV2DeviceGetHDMIVersion(inDeviceID));
            const ULWord	rawVideoStd		(inRegValue & kRegMaskHDMIOutV2VideoStd);
			const string	hdmiVidStdStr	(hdmiVers > 1 ? sHDMIStdV2V3[rawVideoStd] : (hdmiVers == 1 ? sHDMIStdV1[rawVideoStd] : ""));
			const string	vidStdStr		(::NTV2StandardToString (NTV2Standard(rawVideoStd), true));
			const uint32_t	srcSampling		((inRegValue & kRegMaskHDMISampling) >> kRegShiftHDMISampling);
			const uint32_t	srcBPC			((inRegValue & (BIT(16)|BIT(17))) >> 16);
			const uint32_t	txBitDepth		((inRegValue & (BIT(20)|BIT(21))) >> 20);
			oss << "Video Standard: " << hdmiVidStdStr;
			if (hdmiVidStdStr != vidStdStr)
				oss << " (" << vidStdStr << ")";
			oss	<< endl
				<< "Color Mode: "				<< ((inRegValue & BIT( 8))	? "RGB"			: "YCbCr")		<< endl
				<< "Video Rate: "				<< sVidRates[(inRegValue & kLHIRegMaskHDMIOutFPS) >> kLHIRegShiftHDMIOutFPS]  << endl
				<< "Scan Mode: "				<< ((inRegValue & BIT(13))	? "Progressive"	: "Interlaced")	<< endl
				<< "Bit Depth: "				<< ((inRegValue & BIT(14))	? "10-bit"		: "8-bit")		<< endl
				<< "Output Color Sampling: "	<< ((inRegValue & BIT(15))	? "4:4:4"		: "4:2:2")		<< endl
				<< "Output Bit Depth: "			<< sBitDepth[txBitDepth]									<< endl
				<< "Src Color Sampling: "		<< sSrcSampling[srcSampling]								<< endl
				<< "Src Bits Per Component: "	<< sBitDepth[srcBPC]										<< endl
				<< "Output Range: "				<< ((inRegValue & BIT(28))	? "Full"		: "SMPTE")		<< endl
				<< "Audio Channels: "			<< ((inRegValue & BIT(29))	? "8"			: "2")			<< endl
				<< "Output: "					<< ((inRegValue & BIT(30))	? "DVI"			: "HDMI");
			if (::NTV2DeviceGetNumHDMIVideoInputs(inDeviceID) && ::NTV2DeviceGetNumHDMIVideoOutputs(inDeviceID))
				oss	<< endl
					<< "Audio Loopback: "		<< OnOff(inRegValue & BIT(31));
			return oss.str();
		}
		virtual	~DecodeHDMIOutputControl()	{}
	}	mDecodeHDMIOutputControl;
	
	struct DecodeHDMIInputStatus : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			ostringstream	oss;
			const ULWord	hdmiVers(::NTV2DeviceGetHDMIVersion (inDeviceID));
			const uint32_t	vidStd	(hdmiVers >= 2 ? (inRegValue & kRegMaskHDMIInV2VideoStd) >> kRegShiftHDMIInV2VideoStd : (inRegValue & kRegMaskInputStatusStd) >> kRegShiftInputStatusStd);
			const uint32_t	rate	((inRegValue & kRegMaskInputStatusFPS) >> kRegShiftInputStatusFPS);
			static const string	sStds[32] = {"1080i", "720p", "480i", "576i", "1080p", "SXGA", "2K1080p", "2K1080i", "3840p", "4096p"};
			static const string	sRates[32] = {"invalid", "60.00", "59.94", "30.00", "29.97", "25.00", "24.00", "23.98", "50.00", "48.00", "47.95" };
			oss	<< "HDMI Input: " << (inRegValue & BIT(0) ? "Locked" : "Unlocked")			<< endl
				<< "HDMI Input: " << (inRegValue & BIT(1) ? "Stable" : "Unstable")			<< endl
				<< "Color Mode: " << (inRegValue & BIT(2) ? "RGB" : "YCbCr")				<< endl
				<< "Bitdepth: " << (inRegValue & BIT(3) ? "10-bit" : "8-bit")				<< endl
				<< "Audio Channels: " << (inRegValue & BIT(12) ? 2 : 8)						<< endl
				<< "Scan Mode: " << (inRegValue & BIT(13) ? "Progressive" : "Interlaced")	<< endl
				<< "Standard: " << (inRegValue & BIT(14) ? "SD" : "HD")						<< endl
				<< "Video Standard: " << sStds[vidStd]										<< endl
				<< "Protocol: " << (inRegValue & BIT(27) ? "DVI" : "HDMI")					<< endl
				<< "Video Rate : " << (rate < 11 ? sRates[rate] : string("invalid"));
			return oss.str();
		}
		virtual	~DecodeHDMIInputStatus()	{}
	}	mDecodeHDMIInputStatus;
	
	struct DecodeHDMIInputControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;	(void) inDeviceID;
			ostringstream	oss;
			const UWord	chanPair	((inRegValue & (BIT(2) | BIT(3))) >> 2);
			const UWord	txSrcSel	((inRegValue & (BIT(20)|BIT(21)|BIT(22)|BIT(23))) >> 20);
			const UWord	txCh12Sel	((inRegValue & (BIT(29)|BIT(30))) >> 29);
			static const NTV2AudioChannelPair pairs[] = {NTV2_AudioChannel1_2, NTV2_AudioChannel3_4, NTV2_AudioChannel5_6, NTV2_AudioChannel7_8};
			oss	<< "HDMI In EDID Write-Enable: " << EnabDisab(inRegValue & BIT(0))							<< endl
				<< "HDMI Force Output Params: " << SetNotset(inRegValue & BIT(1))							<< endl
				<< "HDMI In Audio Chan Select: " << ::NTV2AudioChannelPairToString(pairs[chanPair], true)	<< endl
				<< "hdmi_rx_8ch_src_off: " << YesNo(inRegValue & BIT(4))									<< endl
				<< "Swap HDMI In Audio Ch. 3/4: " << YesNo(inRegValue & BIT(5))								<< endl
				<< "Swap HDMI Out Audio Ch. 3/4: " << YesNo(inRegValue & BIT(6))							<< endl
				<< "HDMI Prefer 420: " << SetNotset(inRegValue & BIT(7))									<< endl
				<< "hdmi_rx_spdif_err: " << SetNotset(inRegValue & BIT(8))									<< endl
				<< "hdmi_rx_afifo_under: " << SetNotset(inRegValue & BIT(9))								<< endl
				<< "hdmi_rx_afifo_empty: " << SetNotset(inRegValue & BIT(10))								<< endl
				<< "H polarity: " << (inRegValue & BIT(16) ? "Inverted" : "Normal")							<< endl
				<< "V polarity: " << (inRegValue & BIT(17) ? "Inverted" : "Normal")							<< endl
				<< "F polarity: " << (inRegValue & BIT(18) ? "Inverted" : "Normal")							<< endl
				<< "DE polarity: " << (inRegValue & BIT(19) ? "Inverted" : "Normal")						<< endl
				<< "Tx Src Sel: " << DEC(txSrcSel) << " (" << xHEX0N(txSrcSel,4) << ")"						<< endl
				<< "Tx Center Cut: " << SetNotset(inRegValue & BIT(24))										<< endl
				<< "Tx 12 bit: " << SetNotset(inRegValue & BIT(26))											<< endl
				<< "RGB Input Gamut: " << (inRegValue & BIT(28) ? "Full Range" : "Narrow Range (SMPTE)")	<< endl
				<< "Tx_ch12_sel: " << DEC(txCh12Sel) << " (" << xHEX0N(txCh12Sel,4) << ")"					<< endl
				<< "Input AVI Gamut: " << (inRegValue & BIT(31) ? "Full Range" : "Narrow Range (SMPTE)")	<< endl
				<< "EDID: " << SetNotset(inRegValue & BIT(31));
			return oss.str();
		}
		virtual	~DecodeHDMIInputControl()	{}
	}	mDecodeHDMIInputControl;

	struct DecodeHDMIOutputStatus : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inRegNum;
			const NTV2HDMIOutputStatus stat (inRegValue);
			ostringstream	oss;
			stat.Print(oss);
			return oss.str();
		}
		virtual	~DecodeHDMIOutputStatus()	{}
	}	mDecodeHDMIOutputStatus;

	struct DecodeHDMIOutHDRPrimary : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			ostringstream	oss;
			if (::NTV2DeviceCanDoHDMIHDROut (inDeviceID))
				switch (inRegNum)
			{
				case kRegHDMIHDRGreenPrimary:
				case kRegHDMIHDRBluePrimary:
				case kRegHDMIHDRRedPrimary:
				case kRegHDMIHDRWhitePoint:
				{	//	Asserts to validate this one code block will handle all cases:
					NTV2_ASSERT (kRegMaskHDMIHDRGreenPrimaryX == kRegMaskHDMIHDRBluePrimaryX  &&  kRegMaskHDMIHDRBluePrimaryX == kRegMaskHDMIHDRRedPrimaryX);
					NTV2_ASSERT (kRegMaskHDMIHDRGreenPrimaryY == kRegMaskHDMIHDRBluePrimaryY  &&  kRegMaskHDMIHDRBluePrimaryY == kRegMaskHDMIHDRRedPrimaryY);
					NTV2_ASSERT (kRegMaskHDMIHDRRedPrimaryX == kRegMaskHDMIHDRWhitePointX  &&  kRegMaskHDMIHDRRedPrimaryY == kRegMaskHDMIHDRWhitePointY);
					NTV2_ASSERT (kRegShiftHDMIHDRGreenPrimaryX == kRegShiftHDMIHDRBluePrimaryX  &&  kRegShiftHDMIHDRBluePrimaryX == kRegShiftHDMIHDRRedPrimaryX);
					NTV2_ASSERT (kRegShiftHDMIHDRGreenPrimaryY == kRegShiftHDMIHDRBluePrimaryY  &&  kRegShiftHDMIHDRBluePrimaryY == kRegShiftHDMIHDRRedPrimaryY);
					NTV2_ASSERT (kRegShiftHDMIHDRRedPrimaryX == kRegShiftHDMIHDRWhitePointX  &&  kRegShiftHDMIHDRRedPrimaryY == kRegShiftHDMIHDRWhitePointY);
					const uint16_t	xPrimary	((inRegValue & kRegMaskHDMIHDRRedPrimaryX) >> kRegShiftHDMIHDRRedPrimaryX);
					const uint16_t	yPrimary	((inRegValue & kRegMaskHDMIHDRRedPrimaryY) >> kRegShiftHDMIHDRRedPrimaryY);
					const double	xFloat		(double(xPrimary) * 0.00002);
					const double	yFloat		(double(yPrimary) * 0.00002);
					if (NTV2_IS_VALID_HDR_PRIMARY (xPrimary))
						oss	<< "X: "	<< fDEC(xFloat,7,5) << endl;
					else
						oss	<< "X: "	<< HEX0N(xPrimary, 4)	<< "(invalid)" << endl;
					if (NTV2_IS_VALID_HDR_PRIMARY (yPrimary))
						oss	<< "Y: "	<< fDEC(yFloat,7,5);
					else
						oss	<< "Y: "	<< HEX0N(yPrimary, 4)	<< "(invalid)";
					break;
				}
				case kRegHDMIHDRMasteringLuminence:
				{
					const uint16_t	minValue	((inRegValue & kRegMaskHDMIHDRMinMasteringLuminance) >> kRegShiftHDMIHDRMinMasteringLuminance);
					const uint16_t	maxValue	((inRegValue & kRegMaskHDMIHDRMaxMasteringLuminance) >> kRegShiftHDMIHDRMaxMasteringLuminance);
					const double	minFloat	(double(minValue) * 0.00001);
					const double	maxFloat	(maxValue);
					oss	<< "Min: "	<< fDEC(minFloat,7,5) << endl
						<< "Max: "	<< fDEC(maxFloat,7,5);
					break;
				}
				case kRegHDMIHDRLightLevel:
				{
					const uint16_t	cntValue	((inRegValue & kRegMaskHDMIHDRMaxContentLightLevel) >> kRegShiftHDMIHDRMaxContentLightLevel);
					const uint16_t	frmValue	((inRegValue & kRegMaskHDMIHDRMaxFrameAverageLightLevel) >> kRegShiftHDMIHDRMaxFrameAverageLightLevel);
					const double	cntFloat	(cntValue);
					const double	frmFloat	(frmValue);
					oss	<< "Max Content Light Level: "	<< fDEC(cntFloat,7,5)					<< endl
						<< "Max Frame Light Level: "	<< fDEC(frmFloat,7,5);
					break;
				}
				default:	NTV2_ASSERT(false);
			}
			return oss.str();
		}
		virtual	~DecodeHDMIOutHDRPrimary()	{}
	}	mDecodeHDMIOutHDRPrimary;
	
	struct DecodeHDMIOutHDRControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			static const string	sEOTFs[]	=	{"Trad Gamma SDR", "Trad Gamma HDR", "SMPTE ST 2084", "HLG"};
			ostringstream	oss;
			if (::NTV2DeviceCanDoHDMIHDROut (inDeviceID))
			{
				const uint16_t	EOTFvalue				((inRegValue & kRegMaskElectroOpticalTransferFunction) >> kRegShiftElectroOpticalTransferFunction);
				const uint16_t	staticMetaDataDescID	((inRegValue & kRegMaskHDRStaticMetadataDescriptorID) >> kRegShiftHDRStaticMetadataDescriptorID);
				oss	<< "HDMI Out Dolby Vision Enabled: " << YesNo(inRegValue & kRegMaskHDMIHDRDolbyVisionEnable)     << endl
					<< "HDMI HDR Out Enabled: "		     << YesNo(inRegValue & kRegMaskHDMIHDREnable)				<< endl
					<< "Constant Luminance: "		     << YesNo(inRegValue & kRegMaskHDMIHDRNonContantLuminance)	<< endl
					<< "EOTF: "						     << sEOTFs[(EOTFvalue < 3) ? EOTFvalue : 3]					<< endl
					<< "Static MetaData Desc ID: "	     << HEX0N(staticMetaDataDescID, 2) << " (" << DEC(staticMetaDataDescID) << ")";
			}
			return oss.str();
		}
		virtual	~DecodeHDMIOutHDRControl()	{}
	}	mDecodeHDMIOutHDRControl;
	
	struct DecodeHDMIOutMRControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			ostringstream	oss;
            static const string	sMRStandard[]	=	{	"1080i",	"720p",	"480i",	"576i",	"1080p",	"1556i",	"2Kx1080p",	"2Kx1080i",	"UHD",	"4K", "", "", "", "", "", "" };
			const ULWord	rawVideoStd		(inRegValue & kRegMaskMRStandard);
			const string	hdmiVidStdStr	(sMRStandard[rawVideoStd]);
			const string	vidStdStr		(::NTV2StandardToString (NTV2Standard(rawVideoStd), true));
			oss << "Video Standard: " << hdmiVidStdStr;
			if (hdmiVidStdStr != vidStdStr)
				oss << " (" << vidStdStr << ")";
			oss	<< endl
				<< "Capture Mode: "				<< ((inRegValue & kRegMaskMREnable)	? "Enabled"			: "Disabled");
			return oss.str();
		}
		virtual	~DecodeHDMIOutMRControl()	{}
	}	mDecodeHDMIOutMRControl;

	struct DecodeSDIOutputControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream		oss;
			const uint32_t		vidStd	(inRegValue & (BIT(0)|BIT(1)|BIT(2)));
			static const string	sStds[32] = {"1080i", "720p", "480i", "576i", "1080p", "1556i", "6", "7"};
			oss	<< "Video Standard: "			<< sStds[vidStd]										<< endl
				<< "2Kx1080 mode: "				<< (inRegValue & BIT(3) ? "2048x1080" : "1920x1080")	<< endl
				<< "HBlank RGB Range: Black="	<< (inRegValue & BIT(7) ? "0x40" : "0x04")				<< endl
				<< "12G enable: "				<< YesNo(inRegValue & BIT(17))							<< endl
				<< "6G enable: "				<< YesNo(inRegValue & BIT(16))							<< endl
				<< "3G enable: "				<< YesNo(inRegValue & BIT(24))							<< endl
				<< "3G mode: "					<< (inRegValue & BIT(25) ? "b" : "a")					<< endl
				<< "VPID insert enable: "		<< YesNo(inRegValue & BIT(26))							<< endl
				<< "VPID overwrite enable: "	<< YesNo(inRegValue & BIT(27))							<< endl
				<< "DS 1 audio source: Subsystem ";
			switch ((inRegValue & (BIT(28)|BIT(30))) >> 28)
			{
				case 0:	oss << (inRegValue & BIT(18) ? 5 : 1);	break;
				case 1:	oss << (inRegValue & BIT(18) ? 7 : 3);	break;
				case 4:	oss << (inRegValue & BIT(18) ? 6 : 2);	break;
				case 5:	oss << (inRegValue & BIT(18) ? 8 : 4);	break;
			}
			oss	<< endl	<< "DS 2 audio source: Subsystem ";
			switch ((inRegValue & (BIT(29)|BIT(31))) >> 29)
			{
				case 0:	oss << (inRegValue & BIT(19) ? 5 : 1);	break;
				case 1:	oss << (inRegValue & BIT(19) ? 7 : 3);	break;
				case 4:	oss << (inRegValue & BIT(19) ? 6 : 2);	break;
				case 5:	oss << (inRegValue & BIT(19) ? 8 : 4);	break;
			}
			return oss.str();
		}
		virtual	~DecodeSDIOutputControl()	{}
	}	mDecodeSDIOutputControl;
	
	struct DecodeDMAControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			const uint16_t	gen		((inRegValue & (BIT(20)|BIT(21)|BIT(22)|BIT(23))) >> 20);
			const uint16_t	lanes	((inRegValue & (BIT(16)|BIT(17)|BIT(18)|BIT(19))) >> 16);
			const uint16_t	fwRev	((inRegValue & 0x0000FF00) >> 8);
			ostringstream	oss;
			for (uint16_t engine(0);  engine < 4;  engine++)
				oss	<< "DMA " << (engine+1) << " Int Active?: "	<< YesNo(inRegValue & BIT(27+engine))						<< endl;
			oss	<< "Bus Error Int Active?: "					<< YesNo(inRegValue & BIT(31))								<< endl;
			for (uint16_t engine(0);  engine < 4;  engine++)
				oss	<< "DMA " << (engine+1) << " Busy?: "		<< YesNo(inRegValue & BIT(27+engine))						<< endl;
			oss	<< "Strap: "									<< ((inRegValue & BIT(7)) ? "Installed" : "Not Installed")	<< endl
				<< "Firmware Rev: "								<< xHEX0N(fwRev, 2) << " (" << DEC(fwRev) << ")"			<< endl
				<< "Gen: "										<< gen << ((gen > 0 && gen < 4) ? "" : " <invalid>")		<< endl
				<< "Lanes: "									<< DEC(lanes) << ((lanes < 9) ? "" : " <invalid>");
			return oss.str();
		}
		virtual	~DecodeDMAControl()	{}
	}	mDMAControlRegDecoder;

	struct DecodeDMAIntControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			for (uint16_t eng(0);  eng < 4;  eng++)
				oss	<< "DMA " << (eng+1) << " Enabled?: "	<< YesNo(inRegValue & BIT(eng))		<< endl;
			oss	<< "Bus Error Enabled?: "					<< YesNo(inRegValue & BIT(4))		<< endl;
			for (uint16_t eng(0);  eng < 4;  eng++)
				oss	<< "DMA " << (eng+1) << " Active?: "	<< YesNo(inRegValue & BIT(27+eng))	<< endl;
			oss	<< "Bus Error: "							<< YesNo(inRegValue & BIT(31));
			return oss.str();
		}
		virtual	~DecodeDMAIntControl()	{}
	}	mDMAIntControlRegDecoder;

	struct DecodeRP188InOutDBB : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			const bool	isReceivingRP188 (inRegValue & BIT(16));
			const bool	isReceivingSelectedRP188 (inRegValue & BIT(17));
			const bool	isReceivingLTC (inRegValue & BIT(18));
			const bool	isReceivingVITC (inRegValue & BIT(19));
			ostringstream	oss;
			oss	<< "RP188: "	<< (isReceivingRP188 ? (isReceivingSelectedRP188 ? "Selected" : "Unselected") : "No") << " RP-188 received"
								<< (isReceivingLTC ? " +LTC" : "") << (isReceivingVITC ? " +VITC" : "")					<< endl
				<< "Bypass: "	<< (inRegValue & BIT(23) ? (inRegValue & BIT(22) ? "SDI In 2" : "SDI In 1") : "Disabled")	<< endl
				<< "Filter: "	<< HEX0N((inRegValue & 0xFF000000) >> 24, 2)												<< endl
				<< "DBB: "		<< HEX0N((inRegValue & 0x0000FF00) >> 8, 2) << " " << HEX0N(inRegValue & 0x000000FF, 2);
			return oss.str();
		}
		virtual	~DecodeRP188InOutDBB()	{}
	}	mRP188InOutDBBRegDecoder;

	struct DecodeVidProcControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			static const string	sSplitStds [8]	=	{"1080i", "720p", "480i", "576i", "1080p", "1556i", "?6?", "?7?"};
			oss	<< "Mode: "				<< (inRegValue & kRegMaskVidProcMode ? ((inRegValue & BIT(24)) ? "Shaped" : "Unshaped") : "Full Raster")		<< endl
				<< "FG Control: "		<< (inRegValue & kRegMaskVidProcFGControl ? ((inRegValue & BIT(20)) ? "Shaped" : "Unshaped") : "Full Raster")	<< endl
				<< "BG Control: "		<< (inRegValue & kRegMaskVidProcBGControl ? ((inRegValue & BIT(22)) ? "Shaped" : "Unshaped") : "Full Raster")	<< endl
				<< "VANC Pass-Thru: "	<< ((inRegValue & BIT(13)) ? "Background" : "Foreground")														<< endl
				<< "FG Matte: "			<< EnabDisab(inRegValue & kRegMaskVidProcFGMatteEnable)															<< endl
				<< "BG Matte: "			<< EnabDisab(inRegValue & kRegMaskVidProcBGMatteEnable)															<< endl
				<< "Input Sync: "		<< (inRegValue & kRegMaskVidProcSyncFail ? "not in sync" : "in sync")								<< endl
				<< "Limiting: "			<< ((inRegValue & BIT(11)) ? "Off" : ((inRegValue & BIT(12)) ? "Legal Broadcast" : "Legal SDI"))				<< endl
				<< "Split Video Std: "	<< sSplitStds[inRegValue & kRegMaskVidProcSplitStd];
			return oss.str();
		}
		virtual	~DecodeVidProcControl()	{}
	}	mVidProcControlRegDecoder;
	
	struct DecodeSplitControl : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			const uint32_t	startmask	(0x0000FFFF);	//	16 bits
			const uint32_t	slopemask	(0x3FFF0000);	//	14 bits / high order byte
			const uint32_t	fractionmask(0x00000007);	//	3 bits for fractions
			oss	<< "Split Start: "	<< HEX0N((inRegValue & startmask) & ~fractionmask, 4) << " "
				<< HEX0N((inRegValue & startmask) & fractionmask, 4)					<< endl
				<< "Split Slope: "	<< HEX0N(((inRegValue & slopemask) >> 16) & ~fractionmask, 4) << " "
				<< HEX0N(((inRegValue & slopemask) >> 16) & fractionmask, 4)			<< endl
				<< "Split Type: "	<< ((inRegValue & BIT(30)) ? "Vertical" : "Horizontal");
			return oss.str();
		}
		virtual	~DecodeSplitControl()	{}
	}	mSplitControlRegDecoder;

	struct DecodeFlatMatteValue : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			const uint32_t	mask	(0x000003FF);	//	10 bits
			oss	<< "Flat Matte Cb: "	<< HEX0N(inRegValue & mask, 3)					<< endl
				<< "Flat Matte Y: "		<< HEX0N(((inRegValue >> 10) & mask) - 0x40, 3)	<< endl
				<< "Flat Matte Cr: "	<< HEX0N((inRegValue >> 20) & mask, 3);
			return oss.str();
		}
		virtual	~DecodeFlatMatteValue()	{}
	}	mFlatMatteValueRegDecoder;

	struct DecodeEnhancedCSCMode : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			static const string	sFiltSel[]  = {"Full", "Simple", "None", "?"};
			static const string	sEdgeCtrl[] = {"black", "extended pixels"};
			static const string	sPixFmts[]  = {"RGB 4:4:4", "YCbCr 4:4:4", "YCbCr 4:2:2", "?"};
			const uint32_t	filterSelect	((inRegValue >> 12) & 0x3);
			const uint32_t	edgeControl		((inRegValue >> 8) & 0x1);
			const uint32_t	outPixFmt		((inRegValue >> 4) & 0x3);
			const uint32_t	inpPixFmt		(inRegValue & 0x3);
			ostringstream	oss;
			oss	<< "Filter select: "		<< sFiltSel[filterSelect]					<< endl
				<< "Filter edge control: "	<< "Filter to " << sEdgeCtrl[edgeControl]	<< endl
				<< "Output pixel format: "	<< sPixFmts[outPixFmt]						<< endl
				<< "Input pixel format: "	<< sPixFmts[inpPixFmt];
			return oss.str();
		}
		virtual	~DecodeEnhancedCSCMode()	{}
	}	mEnhCSCModeDecoder;

	struct DecodeEnhancedCSCOffset : public Decoder
	{
		static string U10Dot6ToFloat (const uint32_t inOffset)
		{
			double result (double((inOffset >> 6) & 0x3FF));
			result += double(inOffset & 0x3F) / 64.0;
			ostringstream	oss;  oss << fDEC(result,12,5);  string resultStr(oss.str());
			return aja::replace (resultStr, sSpace, sNull);
		}
		static string U12Dot4ToFloat (const uint32_t inOffset)
		{
			double result (double((inOffset >> 4) & 0xFFF));
			result += double(inOffset & 0xF) / 16.0;
			ostringstream	oss;  oss << fDEC(result,12,4);  string resultStr(oss.str());
			return aja::replace (resultStr, sSpace, sNull);
		}
		static string S13Dot2ToFloat (const uint32_t inOffset)
		{
			double result (double((inOffset >> 2) & 0x1FFF));
			result += double(inOffset & 0x3) / 4.0;
			if (inOffset & BIT(15))
				result = -result;
			ostringstream	oss;  oss << fDEC(result,12,2);  string resultStr(oss.str());
			return aja::replace (resultStr, sSpace, sNull);
		}
		static string S11Dot4ToFloat (const uint32_t inOffset)
		{
			double result (double((inOffset >> 4) & 0x7FF));
			result += double(inOffset & 0xF) / 16.0;
			if (inOffset & BIT(15))
				result = -result;
			ostringstream	oss;  oss << fDEC(result,12,4);  string resultStr(oss.str());
			return aja::replace (resultStr, sSpace, sNull);
		}
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			const uint32_t	regNum (inRegNum & 0x1F);
			const uint32_t	lo (inRegValue & 0x0000FFFF);
			const uint32_t	hi ((inRegValue >> 16) & 0xFFFF);
			ostringstream	oss;
			switch (regNum)
			{
				case 1:		oss << "Component 0 input offset: "		<< U12Dot4ToFloat(lo)	<< " (12-bit), "	<< U10Dot6ToFloat(lo)	<< " (10-bit)"	<< endl
								<< "Component 1 input offset: "		<< U12Dot4ToFloat(hi)	<< " (12-bit), "	<< U10Dot6ToFloat(hi)	<< " (10-bit)";
							break;
				case 2:		oss	<< "Component 2 input offset: "		<< U12Dot4ToFloat(lo)	<< " (12-bit), "	<< U10Dot6ToFloat(lo)	<< " (10-bit)";
							break;
				case 12:	oss	<< "Component A output offset: "	<< U12Dot4ToFloat(lo)	<< " (12-bit), "	<< U10Dot6ToFloat(lo)	<< " (10-bit)"	<< endl
								<< "Component B output offset: "	<< U12Dot4ToFloat(hi)	<< " (12-bit), "	<< U10Dot6ToFloat(hi)	<< " (10-bit)";
							break;
				case 13:	oss	<< "Component C output offset: "	<< U12Dot4ToFloat(lo)	<< " (12-bit), "	<< U10Dot6ToFloat(lo)	<< " (10-bit)";
							break;
				case 15:	oss	<< "Key input offset: "				<< S13Dot2ToFloat(lo)	<< " (12-bit), "	<< S11Dot4ToFloat(lo)	<< " (10-bit)"	<< endl
								<< "Key output offset: "			<< U12Dot4ToFloat(hi)	<< " (12-bit), "	<< U10Dot6ToFloat(hi)	<< " (10-bit)";
							break;
				default:	break;
			}
			return oss.str();
		}
		virtual	~DecodeEnhancedCSCOffset()	{}
	}	mEnhCSCOffsetDecoder;

	struct DecodeEnhancedCSCKeyMode : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			static const string	sSrcSel[] = {"Key Input", "Video Y Input"};
			static const string	sRange[]  = {"Full Range", "SMPTE Range"};
			const uint32_t	keySrcSelect	(inRegValue & 0x1);
			const uint32_t	keyOutRange		((inRegValue >> 4) & 0x1);
			ostringstream	oss;
			oss	<< "Key Source Select: "	<< sSrcSel[keySrcSelect]		<< endl
				<< "Key Output Range: "		<< sRange[keyOutRange];
			return oss.str();
		}
		virtual	~DecodeEnhancedCSCKeyMode()	{}
	}	mEnhCSCKeyModeDecoder;

	struct DecodeEnhancedCSCCoefficient : public Decoder
	{
		static string S2Dot15ToFloat (const uint32_t inCoefficient)
		{
			double result = (double((inCoefficient >> 15) & 0x3));
			result += double(inCoefficient & 0x7FFF) / 32768.0;
			if (inCoefficient & BIT(17))
				result = -result;
			ostringstream	oss;  oss << fDEC(result,12,10);  string resultStr(oss.str());
			return aja::replace(resultStr, sSpace, sNull);
		}
		static string S12Dot12ToFloat (const uint32_t inCoefficient)
		{
			double result(double((inCoefficient >> 12) & 0xFFF));
			result += double(inCoefficient & 0xFFF) / 4096.0;
			if (inCoefficient & BIT(24))
				result = -result;
			ostringstream	oss;  oss << fDEC(result,12,6);  string resultStr(oss.str());
			return aja::replace(resultStr, sSpace, sNull);
		}
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			uint32_t		regNum (inRegNum & 0x1F);
			ostringstream	oss;
			if (regNum > 2 && regNum < 12)
			{
				regNum -= 3;
				static const string	sCoeffNames[] = {"A0", "A1", "A2", "B0", "B1", "B2", "C0", "C1", "C2"};
				const uint32_t	coeff	((inRegValue >> 9) & 0x0003FFFF);
				oss	<< sCoeffNames[regNum] << " coefficient: "	<< S2Dot15ToFloat(coeff) << " (" << xHEX0N(coeff,8) << ")";
			}
			else if (regNum == 16)
			{
				const uint32_t	gain	((inRegValue >> 4) & 0x01FFFFFF);
				oss	<< "Key gain: "								<< S12Dot12ToFloat(gain) << " (" << HEX0N(gain,8) << ")";
			}
			return oss.str();
		}
		virtual	~DecodeEnhancedCSCCoefficient()	{}
	}	mEnhCSCCoeffDecoder;

	struct DecodeCSCoeff1234 : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			const uint32_t	coeff1 (((inRegValue >> 11) & 0x00000003) | uint32_t(inRegValue & 0x000007FF));
			const uint32_t	coeff2 ((inRegValue >> 14) & 0x00001FFF);
			uint16_t		nCoeff1(1), nCoeff2(2);
			switch(inRegNum)
			{
				case kRegCSCoefficients3_4:		case kRegCS2Coefficients3_4:	case kRegCS3Coefficients3_4:	case kRegCS4Coefficients3_4:
				case kRegCS5Coefficients3_4:	case kRegCS6Coefficients3_4:	case kRegCS7Coefficients3_4:	case kRegCS8Coefficients3_4:
					nCoeff1 = 3;	nCoeff2 = 4;	break;
			}
			//			kRegCS?Coefficients1_2					kRegCS?Coefficients3_4
			//	CSC		1	2	3	4	5	6	7	8			1	2	3	4	5	6	7	8
			//	RegNum	142	147	291	296	347	460	465	470			143	148	292	297	348	461	466	471
			//	kRegCS?Coefficients1_2:	kK2RegMaskVidKeySyncStatus			= BIT(28)	0=OK		1=SyncFail		GetColorSpaceVideoKeySyncFail
			//	kRegCS?Coefficients1_2:	kK2RegMaskMakeAlphaFromKeySelect	= BIT(29)	0=No		1=Yes			GetColorSpaceMakeAlphaFromKey
			//	kRegCS?Coefficients1_2:	kK2RegMaskColorSpaceMatrixSelect	= BIT(30)	0=Rec709	1=Rec601		GetColorSpaceMatrixSelect
			//	kRegCS?Coefficients1_2:	kK2RegMaskUseCustomCoefSelect		= BIT(31)	0=No		1=Yes			GetColorSpaceUseCustomCoefficient
			//	kRegCS?Coefficients3_4:	kK2RegMaskXena2RGBRange				= BIT(31)	0=Full		1=SMPTE			GetColorSpaceRGBBlackRange
			//	kK2RegMaskCustomCoefficientLow		= BITS(0-10)	CSCCustomCoeffs.Coefficient1	GetColorSpaceCustomCoefficients
			//	kK2RegMaskCustomCoefficientHigh		= BITS(16-26)	CSCCustomCoeffs.Coefficient2	GetColorSpaceCustomCoefficients
			//	kK2RegMaskCustomCoefficient12BitLow	= BITS(0-12)	CSCCustomCoeffs.Coefficient1	GetColorSpaceCustomCoefficients12Bit
			//	kK2RegMaskCustomCoefficient12BitHigh= BITS(14-26)	CSCCustomCoeffs.Coefficient2	GetColorSpaceCustomCoefficients12Bit
			ostringstream	oss;
			if (nCoeff1 == 1)
				oss	<< "Video Key Sync Status: "		<< (inRegValue & BIT(28) ? "SyncFail" : "OK")	<< endl
					<< "Make Alpha From Key Input: "	<< EnabDisab(inRegValue & BIT(29))				<< endl
					<< "Matrix Select: "				<< (inRegValue & BIT(30) ? "Rec601" : "Rec709")	<< endl
					<< "Use Custom Coeffs: "			<< YesNo(inRegValue & BIT(31))					<< endl;
			else
				oss	<< "RGB Range: "					<< (inRegValue & BIT(31) ? "SMPTE (0x040-0x3C0)" : "Full (0x000-0x3FF)")	<< endl;
			oss	<< "Coefficient" << DEC(nCoeff1) << ": "	<< xHEX0N(coeff1, 4)	<< endl
				<< "Coefficient" << DEC(nCoeff2) << ": "	<< xHEX0N(coeff2, 4);
			return oss.str();
		}
		virtual	~DecodeCSCoeff1234()	{}
	}	mCSCoeff1234Decoder;

	struct DecodeCSCoeff567890 : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			const uint32_t	coeff5 (((inRegValue >> 11) & 0x00000003) | uint32_t(inRegValue & 0x000007FF));
			const uint32_t	coeff6 ((inRegValue >> 14) & 0x00001FFF);
			uint16_t	nCoeff5(5), nCoeff6(6);
			switch(inRegNum)
			{
				case kRegCSCoefficients7_8:		case kRegCS2Coefficients7_8:	case kRegCS3Coefficients7_8:	case kRegCS4Coefficients7_8:
				case kRegCS5Coefficients7_8:	case kRegCS6Coefficients7_8:	case kRegCS7Coefficients7_8:	case kRegCS8Coefficients7_8:
					nCoeff5 = 7;	nCoeff6 = 8;	break;
				case kRegCSCoefficients9_10:	case kRegCS2Coefficients9_10:	case kRegCS3Coefficients9_10:	case kRegCS4Coefficients9_10:
				case kRegCS5Coefficients9_10:	case kRegCS6Coefficients9_10:	case kRegCS7Coefficients9_10:	case kRegCS8Coefficients9_10:
					nCoeff5 = 9;	nCoeff6 = 10;	break;
			}
			//			kRegCS?Coefficients5_6				kRegCS?Coefficients7_8				kRegCS?Coefficients9_10
			//	CSC		1	2	3	4	5	6	7	8		1	2	3	4	5	6	7	8		1	2	3	4	5	6	7	8
			//	RegNum	143	148	292	297	348	461	466	471		144	149	293	298	349	462	467	472		145	150	294	299	350	463	468	473
			//	kK2RegMaskCustomCoefficientLow	= BITS(0-10) CSCCustomCoeffs.Coefficient5	GetColorSpaceCustomCoefficients
			//	kK2RegMaskCustomCoefficientHigh	= BITS(16-26) CSCCustomCoeffs.Coefficient6	GetColorSpaceCustomCoefficients
			//	kK2RegMaskCustomCoefficient12BitLow	= BITS(0-12) CSCCustomCoeffs.Coefficient5	GetColorSpaceCustomCoefficients12Bit
			//	kK2RegMaskCustomCoefficient12BitHigh= BITS(14-26) CSCCustomCoeffs.Coefficient6	GetColorSpaceCustomCoefficients12Bit
			ostringstream	oss;
			oss	<< "Coefficient" << DEC(nCoeff5) << ": "	<< xHEX0N(coeff5, 4)	<< endl
				<< "Coefficient" << DEC(nCoeff6) << ": "	<< xHEX0N(coeff6, 4);
			return oss.str();
		}
		virtual	~DecodeCSCoeff567890()	{}
	}	mCSCoeff567890Decoder;

	struct DecodeLUTV1ControlReg : public Decoder	//	kRegCh1ColorCorrectionControl (68), kRegCh2ColorCorrectionControl (69)
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	static const string sModes[] = {"Off", "RGB", "YCbCr", "3-Way", "Invalid"};
			const ULWord lutVersion (::NTV2DeviceGetLUTVersion(inDeviceID));
			const UWord saturation (UWord(inRegValue & kRegMaskSaturationValue));
			const UWord	mode (UWord((inRegValue & kRegMaskCCMode) >> kRegShiftCCMode));
			const bool outBankSelect (((inRegValue & kRegMaskCCOutputBankSelect) >> kRegShiftCCOutputBankSelect) ? true : false);
			const bool cc5HostBank (((inRegValue & kRegMaskCC5HostAccessBankSelect) >> kRegShiftCC5HostAccessBankSelect) ? true : false);
			const bool cc5OutputBank (((inRegValue & kRegMaskCC5OutputBankSelect) >> kRegShiftCC5OutputBankSelect) ? true : false);
			const bool cc5Select (((inRegValue & kRegMaskLUT5Select) >> kRegShiftLUT5Select) ? true : false);
			const bool ccConfig2 (((inRegValue & kRegMaskLUTSelect) >> kRegShiftLUTSelect) ? true : false);
			const bool cc3BankSel (((inRegValue & kRegMaskCC3OutputBankSelect) >> kRegShiftCC3OutputBankSelect) ? true : false);
			const bool cc4BankSel (((inRegValue & kRegMaskCC4OutputBankSelect) >> kRegShiftCC4OutputBankSelect) ? true : false);
			NTV2_ASSERT(mode < 4);
			ostringstream	oss;
			if (lutVersion != 1)
				oss	<< "(Register data relevant for V1 LUT, this device has V" << DEC(lutVersion) << " LUT)";
			else
			{
				oss	<< "LUT Saturation Value: "		<< xHEX0N(saturation,4) << " (" << DEC(saturation) << ")"	<< endl
					<< "LUT Output Bank Select: "	<< SetNotset(outBankSelect)									<< endl
					<< "LUT Mode: "					<< sModes[mode] << " (" << DEC(mode) << ")";
				if (inRegNum == kRegCh1ColorCorrectionControl)
					oss << endl
						<< "LUT5 Host Bank Select: "	<< SetNotset(cc5HostBank)		<< endl
						<< "LUT5 Output Bank Select: "	<< SetNotset(cc5OutputBank)		<< endl
						<< "LUT5 Select: "				<< SetNotset(cc5Select)			<< endl
						<< "Config 2nd LUT Set: "		<< YesNo(ccConfig2);
			}
			oss	<< endl
				<< "LUT3 Bank Select: "		<< SetNotset(cc3BankSel)	<< endl
				<< "LUT4 Bank Select: "		<< SetNotset(cc4BankSel);
			return oss.str();
		}
		virtual	~DecodeLUTV1ControlReg()	{}
	}	mLUTV1ControlRegDecoder;

	struct DecodeLUTV2ControlReg : public Decoder	//	kRegLUTV2Control	376
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{	(void) inRegNum;
			const ULWord lutVersion (::NTV2DeviceGetLUTVersion(inDeviceID));
			ostringstream	oss;
			if (lutVersion != 2)
				oss	<< "(Register data relevant for V2 LUT, this device has V" << DEC(lutVersion) << "LUT)";
			else
			{
				for (UWord lutNum(0);  lutNum < 8;  lutNum++)
					oss	<< "LUT" << DEC(lutNum+1) << " Enabled: " << (YesNo(inRegValue & (1<<lutNum)))							<< endl
						<< "LUT" << DEC(lutNum+1) << " Host Access Bank Select: " << (inRegValue & (1<<(lutNum+8)) ? '1' : '0')	<< endl
						<< "LUT" << DEC(lutNum+1) << " Output Bank Select: " << (inRegValue & (1<<(lutNum+16)) ? '1' : '0')		<< endl;
				oss	<< "12-Bit LUT mode: " << ((inRegValue & BIT(28)) ? "12-bit" : "10-bit")								<< endl
					<< "12-Bit LUT page reg: " << DEC(UWord((inRegValue & (BIT(24)|BIT(25))) >> 24));
			}
			return oss.str();
		}
		virtual	~DecodeLUTV2ControlReg()	{}
	}	mLUTV2ControlRegDecoder;

	struct DecodeLUT : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inDeviceID;
			static const ULWord RedReg(kColorCorrectionLUTOffset_Red/4), GreenReg(kColorCorrectionLUTOffset_Green/4), BlueReg(kColorCorrectionLUTOffset_Blue/4);
			const bool isRed(inRegNum >= RedReg && inRegNum < GreenReg), isGreen(inRegNum >= GreenReg && inRegNum < BlueReg), isBlue(inRegNum>=BlueReg);
			NTV2_ASSERT(isRed||isGreen||isBlue);
			ostringstream	oss;
			//	Within each 32-bit LUT word are stored two 10-bit values:
			//		-	bits <31:22> ==> LUT[2i+1]
			//		-	bits <15:6>  ==> LUT[2i]
			const string label(isRed ? "Red[" : (isGreen ? "Green[" : "Blue["));
			const ULWord ndx((inRegNum - (isRed ? RedReg : (isGreen ? GreenReg : BlueReg))) * 2);
			const ULWord lo((inRegValue >> kRegColorCorrectionLUTEvenShift) & 0x000003FF);
			const ULWord hi((inRegValue >> kRegColorCorrectionLUTOddShift) & 0x000003FF);
			oss	<< label << DEC0N(ndx+0,3) << "]: " << DEC0N(lo,3) << endl
				<< label << DEC0N(ndx+1,3) << "]: " << DEC0N(hi,3);
			return oss.str();
		}
		virtual	~DecodeLUT()	{}
	}	mLUTDecoder;

	struct DecodeSDIErrorStatus : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			if (::NTV2DeviceCanDoSDIErrorChecks(inDeviceID))
				oss	<< "Unlock Tally: "			<< DEC(inRegValue & 0x7FFF)		<< endl
					<< "Locked: "				<< YesNo(inRegValue & BIT(16))	<< endl
					<< "Link A VID Valid: "		<< YesNo(inRegValue & BIT(20))	<< endl
					<< "Link B VID Valid: "		<< YesNo(inRegValue & BIT(21))	<< endl
					<< "TRS Error Detected: "	<< YesNo(inRegValue & BIT(24));
			return oss.str();
		}
		virtual	~DecodeSDIErrorStatus()	{}
	}	mSDIErrorStatusRegDecoder;
	
	struct DecodeSDIErrorCount : public Decoder
	{
		virtual string operator()(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID) const
		{
			(void) inRegNum;
			(void) inDeviceID;
			ostringstream	oss;
			if (::NTV2DeviceCanDoSDIErrorChecks(inDeviceID))
				oss	<< "Link A: "		<< DEC(inRegValue & 0x0000FFFF)			<< endl
					<< "Link B: "		<< DEC((inRegValue & 0xFFFF0000) >> 16);
			return oss.str();
		}
		virtual	~DecodeSDIErrorCount()	{}
	}	mSDIErrorCountRegDecoder;
	
	static const int	NOREADWRITE	=	0;
	static const int	READONLY	=	1;
	static const int	WRITEONLY	=	2;
	static const int	READWRITE	=	3;
	
	static const int	CONTAINS	=	0;
	static const int	STARTSWITH	=	1;
	static const int	ENDSWITH	=	2;
	static const int	EXACTMATCH	=	3;

	typedef map <uint32_t, const Decoder *>		RegNumToDecoderMap;
	typedef pair <uint32_t, const Decoder *>	RegNumToDecoderPair;
	typedef multimap <string, uint32_t>			RegClassToRegNumMMap, StringToRegNumMMap;
	typedef pair <string, uint32_t>				StringToRegNumPair;
	typedef RegClassToRegNumMMap::const_iterator	RegClassToRegNumConstIter;
	typedef StringToRegNumMMap::const_iterator	StringToRegNumConstIter;

	typedef pair <uint32_t, uint32_t>							XptRegNumAndMaskIndex;	//	First: register number;  second: mask index (0=0x000000FF, 1=0x0000FF00, 2=0x00FF0000, 3=0xFF000000)
	typedef map <NTV2InputCrosspointID, XptRegNumAndMaskIndex>	InputXpt2XptRegNumMaskIndexMap;
	typedef map <XptRegNumAndMaskIndex, NTV2InputCrosspointID>	XptRegNumMaskIndex2InputXptMap;
	typedef	InputXpt2XptRegNumMaskIndexMap::const_iterator		InputXpt2XptRegNumMaskIndexMapConstIter;
	typedef	XptRegNumMaskIndex2InputXptMap::const_iterator		XptRegNumMaskIndex2InputXptMapConstIter;

private:	//	INSTANCE DATA
	mutable AJALock			mGuardMutex;
	RegNumToStringMap		mRegNumToStringMap;
	RegNumToDecoderMap		mRegNumToDecoderMap;
	RegClassToRegNumMMap	mRegClassToRegNumMMap;
	StringToRegNumMMap		mStringToRegNumMMap;
	mutable NTV2StringSet	mAllRegClasses;	//	Mutable -- caches results from 'const' method GetAllRegisterClasses
	InputXpt2XptRegNumMaskIndexMap		mInputXpt2XptRegNumMaskIndexMap;
	XptRegNumMaskIndex2InputXptMap		mXptRegNumMaskIndex2InputXptMap;
	
};	//	RegisterExpert


static RegisterExpertPtr	gpRegExpert;		//	Points to Register Expert Singleton
static AJALock				gRegExpertGuardMutex;


RegisterExpertPtr RegisterExpert::GetInstance(const bool inCreateIfNecessary)
{
	AJAAutoLock		locker(&gRegExpertGuardMutex);
	if (!gpRegExpert  &&  inCreateIfNecessary)
		gpRegExpert = new RegisterExpert;
	return gpRegExpert;
}

bool RegisterExpert::DisposeInstance(void)
{
	AJAAutoLock		locker(&gRegExpertGuardMutex);
	if (!gpRegExpert)
		return false;
	gpRegExpert = AJA_NULL;
	return true;
}

bool CNTV2RegisterExpert::Allocate(void)
{
	AJAAutoLock	lock(&gRegExpertGuardMutex);
	RegisterExpertPtr pInst(RegisterExpert::GetInstance(true));
	return pInst ? true : false;
}

bool CNTV2RegisterExpert::IsAllocated(void)
{
	AJAAutoLock	lock(&gRegExpertGuardMutex);
	RegisterExpertPtr pInst(RegisterExpert::GetInstance(false));
	return pInst ? true : false;
}

bool CNTV2RegisterExpert::Deallocate(void)
{
	AJAAutoLock	lock(&gRegExpertGuardMutex);
	RegisterExpertPtr pInst(RegisterExpert::GetInstance(false));
	return pInst ? pInst->DisposeInstance() : false;
}

string CNTV2RegisterExpert::GetDisplayName (const uint32_t inRegNum)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	if (pRegExpert)
		return pRegExpert->RegNameToString(inRegNum);

	ostringstream	oss;	oss << "Reg ";
	if (inRegNum <= kRegNumRegisters)
		oss << DEC(inRegNum);
	else if (inRegNum <= 0x0000FFFF)
		oss << xHEX0N(inRegNum,4);
	else
		oss << xHEX0N(inRegNum,8);
	return oss.str();
}

string CNTV2RegisterExpert::GetDisplayValue (const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->RegValueToString(inRegNum, inRegValue, inDeviceID) : string();
}

bool CNTV2RegisterExpert::IsRegisterInClass (const uint32_t inRegNum, const string & inClassName)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->IsRegInClass(inRegNum, inClassName) : false;
}

NTV2StringSet CNTV2RegisterExpert::GetAllRegisterClasses (void)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->GetAllRegisterClasses() : NTV2StringSet();
}

NTV2StringSet CNTV2RegisterExpert::GetRegisterClasses (const uint32_t inRegNum)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->GetRegisterClasses(inRegNum) : NTV2StringSet();
}

NTV2RegNumSet CNTV2RegisterExpert::GetRegistersForClass	(const string & inClassName)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->GetRegistersForClass(inClassName) : NTV2RegNumSet();
}

NTV2RegNumSet CNTV2RegisterExpert::GetRegistersForChannel (const NTV2Channel inChannel)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return NTV2_IS_VALID_CHANNEL(inChannel)  ?  (pRegExpert ? pRegExpert->GetRegistersForClass(gChlClasses[inChannel]):NTV2RegNumSet())  :  NTV2RegNumSet();
}

NTV2RegNumSet CNTV2RegisterExpert::GetRegistersForDevice (const NTV2DeviceID inDeviceID, const bool inIncludeVirtuals)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->GetRegistersForDevice(inDeviceID, inIncludeVirtuals) : NTV2RegNumSet();
}

NTV2RegNumSet CNTV2RegisterExpert::GetRegistersWithName (const string & inName, const int inSearchStyle)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->GetRegistersWithName(inName, inSearchStyle) : NTV2RegNumSet();
}

NTV2InputCrosspointID CNTV2RegisterExpert::GetInputCrosspointID (const uint32_t inXptRegNum, const uint32_t inMaskIndex)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->GetInputCrosspointID(inXptRegNum, inMaskIndex) : NTV2_INPUT_CROSSPOINT_INVALID;
}

bool CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (const NTV2InputCrosspointID inInputXpt, uint32_t & outXptRegNum, uint32_t & outMaskIndex)
{
	AJAAutoLock	locker(&gRegExpertGuardMutex);
	RegisterExpertPtr pRegExpert(RegisterExpert::GetInstance());
	return pRegExpert ? pRegExpert->GetXptRegNumAndMaskIndex(inInputXpt, outXptRegNum, outMaskIndex) : false;
}
