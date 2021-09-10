/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2vpid.cpp
	@brief		Implements the CNTV2VPID class. See the SMPTE 352 standard for details.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.
**/

#include "ntv2vpid.h"
#include "ntv2vpidfromspec.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include "ntv2registerexpert.h"	//	For YesNo macro
#include <string.h>

using namespace std;

#ifndef NULL
#define NULL (0)
#endif

static NTV2VideoFormat	stTable720p			[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable2048p		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable1920p		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable2048psf		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable1920psf		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable2048i		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable1920i		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable3840pSID		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable4096pSID		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable3840psfSID	[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable4096psfSID	[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable3840pTSI		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable4096pTSI		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable7680p		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable8192p		[VPIDPictureRate_ReservedF + 1];
static bool				stTablesInitialized	(false);

class VPIDTableInitializer
{
	public:
		VPIDTableInitializer ()
		{
			for (int i = 0;  i < VPIDPictureRate_ReservedF + 1;  i++)
			{
				stTable720p[i]  = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
			}

			stTable720p[VPIDPictureRate_2398] = NTV2_FORMAT_720p_2398;
	//		stTable720p[VPIDPictureRate_2400] = NTV2_FORMAT_720p_2400;
			stTable720p[VPIDPictureRate_2500] = NTV2_FORMAT_720p_2500;
	//		stTable720p[VPIDPictureRate_2997] = NTV2_FORMAT_720p_2997;
	//		stTable720p[VPIDPictureRate_3000] = NTV2_FORMAT_720p_3000;
			stTable720p[VPIDPictureRate_5000] = NTV2_FORMAT_720p_5000;
			stTable720p[VPIDPictureRate_5994] = NTV2_FORMAT_720p_5994;
			stTable720p[VPIDPictureRate_6000] = NTV2_FORMAT_720p_6000;

			stTable2048p[VPIDPictureRate_2398] = NTV2_FORMAT_1080p_2K_2398;
			stTable2048p[VPIDPictureRate_2400] = NTV2_FORMAT_1080p_2K_2400;
			stTable2048p[VPIDPictureRate_2500] = NTV2_FORMAT_1080p_2K_2500;
			stTable2048p[VPIDPictureRate_2997] = NTV2_FORMAT_1080p_2K_2997;
			stTable2048p[VPIDPictureRate_3000] = NTV2_FORMAT_1080p_2K_3000;
			stTable2048p[VPIDPictureRate_4795] = NTV2_FORMAT_1080p_2K_4795_A;		// 3G-A
			stTable2048p[VPIDPictureRate_4800] = NTV2_FORMAT_1080p_2K_4800_A;		// 3G-A
			stTable2048p[VPIDPictureRate_5000] = NTV2_FORMAT_1080p_2K_5000_A;		// 3G-A
			stTable2048p[VPIDPictureRate_5994] = NTV2_FORMAT_1080p_2K_5994_A;		// 3G-A
			stTable2048p[VPIDPictureRate_6000] = NTV2_FORMAT_1080p_2K_6000_A;		// 3G-A
		
			stTable1920p[VPIDPictureRate_2398] = NTV2_FORMAT_1080p_2398;
			stTable1920p[VPIDPictureRate_2400] = NTV2_FORMAT_1080p_2400;
			stTable1920p[VPIDPictureRate_2500] = NTV2_FORMAT_1080p_2500;
			stTable1920p[VPIDPictureRate_2997] = NTV2_FORMAT_1080p_2997;
			stTable1920p[VPIDPictureRate_3000] = NTV2_FORMAT_1080p_3000;
	//		stTable1920p[VPIDPictureRate_4795] = NTV2_FORMAT_1080p_4795;			// 3G-A
	//		stTable1920p[VPIDPictureRate_4800] = NTV2_FORMAT_1080p_4800;			// 3G-A
			stTable1920p[VPIDPictureRate_5000] = NTV2_FORMAT_1080p_5000_A;			// 3G-A
			stTable1920p[VPIDPictureRate_5994] = NTV2_FORMAT_1080p_5994_A;			// 3G-A
			stTable1920p[VPIDPictureRate_6000] = NTV2_FORMAT_1080p_6000_A;			// 3G-A
		
			stTable2048psf[VPIDPictureRate_2398] = NTV2_FORMAT_1080psf_2K_2398;
			stTable2048psf[VPIDPictureRate_2400] = NTV2_FORMAT_1080psf_2K_2400;
			stTable2048psf[VPIDPictureRate_2500] = NTV2_FORMAT_1080psf_2K_2500;
	//		stTable2048psf[VPIDPictureRate_2997] = NTV2_FORMAT_1080psf_2K_2997;
	//		stTable2048psf[VPIDPictureRate_3000] = NTV2_FORMAT_1080psf_2K_3000;
			stTable2048psf[VPIDPictureRate_4795] = NTV2_FORMAT_1080p_2K_4795_B;		// 3G-B or Duallink 1.5G
			stTable2048psf[VPIDPictureRate_4800] = NTV2_FORMAT_1080p_2K_4800_B;		// 3G-B or Duallink 1.5G
			stTable2048psf[VPIDPictureRate_5000] = NTV2_FORMAT_1080p_2K_5000_B;		// 3G-B or Duallink 1.5G
			stTable2048psf[VPIDPictureRate_5994] = NTV2_FORMAT_1080p_2K_5994_B;		// 3G-B or Duallink 1.5G
			stTable2048psf[VPIDPictureRate_6000] = NTV2_FORMAT_1080p_2K_6000_B;		// 3G-B or Duallink 1.5G
		
			stTable1920psf[VPIDPictureRate_2398] = NTV2_FORMAT_1080psf_2398;
			stTable1920psf[VPIDPictureRate_2400] = NTV2_FORMAT_1080psf_2400;
			stTable1920psf[VPIDPictureRate_2500] = NTV2_FORMAT_1080psf_2500_2;
			stTable1920psf[VPIDPictureRate_2997] = NTV2_FORMAT_1080psf_2997_2;
			stTable1920psf[VPIDPictureRate_3000] = NTV2_FORMAT_1080psf_3000_2;
	//		stTable1920psf[VPIDPictureRate_4795] = NTV2_FORMAT_1080p_4795;			// 3G-B or Duallink 1.5G
	//		stTable1920psf[VPIDPictureRate_4800] = NTV2_FORMAT_1080p_4800;			// 3G-B or Duallink 1.5G
			stTable1920psf[VPIDPictureRate_5000] = NTV2_FORMAT_1080p_5000_B;		// 3G-B or Duallink 1.5G
			stTable1920psf[VPIDPictureRate_5994] = NTV2_FORMAT_1080p_5994_B;		// 3G-B or Duallink 1.5G
			stTable1920psf[VPIDPictureRate_6000] = NTV2_FORMAT_1080p_6000_B;		// 3G-B or Duallink 1.5G
		
	//		stTable2048i[VPIDPictureRate_2500] = NTV2_FORMAT_2048x1080i_2500;
	//		stTable2048i[VPIDPictureRate_2997] = NTV2_FORMAT_2048x1080i_2997;
	//		stTable2048i[VPIDPictureRate_3000] = NTV2_FORMAT_2048x1080i_3000;

			stTable1920i[VPIDPictureRate_2500] = NTV2_FORMAT_1080i_5000;
			stTable1920i[VPIDPictureRate_2997] = NTV2_FORMAT_1080i_5994;
			stTable1920i[VPIDPictureRate_3000] = NTV2_FORMAT_1080i_6000;

			stTable3840pSID[VPIDPictureRate_2398] = NTV2_FORMAT_4x1920x1080p_2398;
			stTable3840pSID[VPIDPictureRate_2400] = NTV2_FORMAT_4x1920x1080p_2400;	
			stTable3840pSID[VPIDPictureRate_2500] = NTV2_FORMAT_4x1920x1080p_2500;
			stTable3840pSID[VPIDPictureRate_2997] = NTV2_FORMAT_4x1920x1080p_2997;			
			stTable3840pSID[VPIDPictureRate_3000] = NTV2_FORMAT_4x1920x1080p_3000;			
			stTable3840pSID[VPIDPictureRate_5000] = NTV2_FORMAT_4x1920x1080p_5000;
			stTable3840pSID[VPIDPictureRate_5994] = NTV2_FORMAT_4x1920x1080p_5994;
			stTable3840pSID[VPIDPictureRate_6000] = NTV2_FORMAT_4x1920x1080p_6000;
			
			stTable3840psfSID[VPIDPictureRate_2398] = NTV2_FORMAT_4x1920x1080psf_2398;
			stTable3840psfSID[VPIDPictureRate_2400] = NTV2_FORMAT_4x1920x1080psf_2400;	
			stTable3840psfSID[VPIDPictureRate_2500] = NTV2_FORMAT_4x1920x1080psf_2500;
			stTable3840psfSID[VPIDPictureRate_2997] = NTV2_FORMAT_4x1920x1080psf_2997;			
			stTable3840psfSID[VPIDPictureRate_3000] = NTV2_FORMAT_4x1920x1080psf_3000;			
			stTable3840psfSID[VPIDPictureRate_5000] = NTV2_FORMAT_4x1920x1080p_5000;
			stTable3840psfSID[VPIDPictureRate_5994] = NTV2_FORMAT_4x1920x1080p_5994;
			stTable3840psfSID[VPIDPictureRate_6000] = NTV2_FORMAT_4x1920x1080p_6000;

			stTable4096pSID[VPIDPictureRate_2398] = NTV2_FORMAT_4x2048x1080p_2398;
			stTable4096pSID[VPIDPictureRate_2400] = NTV2_FORMAT_4x2048x1080p_2400;
			stTable4096pSID[VPIDPictureRate_2500] = NTV2_FORMAT_4x2048x1080p_2500;
			stTable4096pSID[VPIDPictureRate_2997] = NTV2_FORMAT_4x2048x1080p_2997;
			stTable4096pSID[VPIDPictureRate_3000] = NTV2_FORMAT_4x2048x1080p_3000;
			stTable4096pSID[VPIDPictureRate_4795] = NTV2_FORMAT_4x2048x1080p_4795;
			stTable4096pSID[VPIDPictureRate_4800] = NTV2_FORMAT_4x2048x1080p_4800;
			stTable4096pSID[VPIDPictureRate_5000] = NTV2_FORMAT_4x2048x1080p_5000;
			stTable4096pSID[VPIDPictureRate_5994] = NTV2_FORMAT_4x2048x1080p_5994;
			stTable4096pSID[VPIDPictureRate_6000] = NTV2_FORMAT_4x2048x1080p_6000;
			
			stTable4096psfSID[VPIDPictureRate_2398] = NTV2_FORMAT_4x2048x1080psf_2398;
			stTable4096psfSID[VPIDPictureRate_2400] = NTV2_FORMAT_4x2048x1080psf_2400;
			stTable4096psfSID[VPIDPictureRate_2500] = NTV2_FORMAT_4x2048x1080psf_2500;
			stTable4096psfSID[VPIDPictureRate_2997] = NTV2_FORMAT_4x2048x1080psf_2997;
			stTable4096psfSID[VPIDPictureRate_3000] = NTV2_FORMAT_4x2048x1080psf_3000;
			stTable4096psfSID[VPIDPictureRate_4795] = NTV2_FORMAT_4x2048x1080p_4795;
			stTable4096psfSID[VPIDPictureRate_4800] = NTV2_FORMAT_4x2048x1080p_4800;
			stTable4096psfSID[VPIDPictureRate_5000] = NTV2_FORMAT_4x2048x1080p_5000;
			stTable4096psfSID[VPIDPictureRate_5994] = NTV2_FORMAT_4x2048x1080p_5994;
			stTable4096psfSID[VPIDPictureRate_6000] = NTV2_FORMAT_4x2048x1080p_6000;

            stTable3840pTSI[VPIDPictureRate_2398] = NTV2_FORMAT_3840x2160p_2398;
            stTable3840pTSI[VPIDPictureRate_2400] = NTV2_FORMAT_3840x2160p_2400;
            stTable3840pTSI[VPIDPictureRate_2500] = NTV2_FORMAT_3840x2160p_2500;
            stTable3840pTSI[VPIDPictureRate_2997] = NTV2_FORMAT_3840x2160p_2997;
            stTable3840pTSI[VPIDPictureRate_3000] = NTV2_FORMAT_3840x2160p_3000;
            stTable3840pTSI[VPIDPictureRate_5000] = NTV2_FORMAT_3840x2160p_5000;
            stTable3840pTSI[VPIDPictureRate_5994] = NTV2_FORMAT_3840x2160p_5994;
            stTable3840pTSI[VPIDPictureRate_6000] = NTV2_FORMAT_3840x2160p_6000;

            stTable4096pTSI[VPIDPictureRate_2398] = NTV2_FORMAT_4096x2160p_2398;
            stTable4096pTSI[VPIDPictureRate_2400] = NTV2_FORMAT_4096x2160p_2400;
            stTable4096pTSI[VPIDPictureRate_2500] = NTV2_FORMAT_4096x2160p_2500;
            stTable4096pTSI[VPIDPictureRate_2997] = NTV2_FORMAT_4096x2160p_2997;
            stTable4096pTSI[VPIDPictureRate_3000] = NTV2_FORMAT_4096x2160p_3000;
            stTable4096pTSI[VPIDPictureRate_4795] = NTV2_FORMAT_4096x2160p_4795;
            stTable4096pTSI[VPIDPictureRate_4800] = NTV2_FORMAT_4096x2160p_4800;
            stTable4096pTSI[VPIDPictureRate_5000] = NTV2_FORMAT_4096x2160p_5000;
            stTable4096pTSI[VPIDPictureRate_5994] = NTV2_FORMAT_4096x2160p_5994;
            stTable4096pTSI[VPIDPictureRate_6000] = NTV2_FORMAT_4096x2160p_6000;
			
			stTable7680p[VPIDPictureRate_2398] = NTV2_FORMAT_4x3840x2160p_2398;
            stTable7680p[VPIDPictureRate_2400] = NTV2_FORMAT_4x3840x2160p_2400;
            stTable7680p[VPIDPictureRate_2500] = NTV2_FORMAT_4x3840x2160p_2500;
            stTable7680p[VPIDPictureRate_2997] = NTV2_FORMAT_4x3840x2160p_2997;
            stTable7680p[VPIDPictureRate_3000] = NTV2_FORMAT_4x3840x2160p_3000;
            stTable7680p[VPIDPictureRate_5000] = NTV2_FORMAT_4x3840x2160p_5000;
            stTable7680p[VPIDPictureRate_5994] = NTV2_FORMAT_4x3840x2160p_5994;
            stTable7680p[VPIDPictureRate_6000] = NTV2_FORMAT_4x3840x2160p_6000;

            stTable8192p[VPIDPictureRate_2398] = NTV2_FORMAT_4x4096x2160p_2398;
            stTable8192p[VPIDPictureRate_2400] = NTV2_FORMAT_4x4096x2160p_2400;
            stTable8192p[VPIDPictureRate_2500] = NTV2_FORMAT_4x4096x2160p_2500;
            stTable8192p[VPIDPictureRate_2997] = NTV2_FORMAT_4x4096x2160p_2997;
            stTable8192p[VPIDPictureRate_3000] = NTV2_FORMAT_4x4096x2160p_3000;
            stTable8192p[VPIDPictureRate_4795] = NTV2_FORMAT_4x4096x2160p_4795;
            stTable8192p[VPIDPictureRate_4800] = NTV2_FORMAT_4x4096x2160p_4800;
            stTable8192p[VPIDPictureRate_5000] = NTV2_FORMAT_4x4096x2160p_5000;
            stTable8192p[VPIDPictureRate_5994] = NTV2_FORMAT_4x4096x2160p_5994;
            stTable8192p[VPIDPictureRate_6000] = NTV2_FORMAT_4x4096x2160p_6000;
			
			stTablesInitialized = true;
		}	//	constructor

};	//	VPIDTableInitializer


static VPIDTableInitializer	gVPIDTableInitializer;


CNTV2VPID::CNTV2VPID(const ULWord inData)
	:	m_uVPID (inData)
{
	NTV2_ASSERT(stTablesInitialized);
}


CNTV2VPID::CNTV2VPID (const CNTV2VPID & inVPID)
{
	NTV2_ASSERT(stTablesInitialized);
	m_uVPID = inVPID.m_uVPID;
}


CNTV2VPID & CNTV2VPID::operator = (const CNTV2VPID & inRHS)
{
	NTV2_ASSERT(stTablesInitialized);
	if (&inRHS != this)
		m_uVPID = inRHS.m_uVPID;
	return *this;
}


bool CNTV2VPID::SetVPID (NTV2VideoFormat videoFormat,
						NTV2FrameBufferFormat frameBufferFormat,
						bool progressive,
						bool aspect16x9,
						VPIDChannel channel)
{
	return SetVPIDData (m_uVPID, videoFormat, frameBufferFormat, progressive, aspect16x9, channel);
}


bool CNTV2VPID::SetVPID (NTV2VideoFormat videoFormat,
						bool dualLink,
						bool format48Bit,
						bool output3Gb,
						bool st425,
						VPIDChannel channel)
{
	return SetVPIDData (m_uVPID,  videoFormat, dualLink, format48Bit, output3Gb, st425, channel);
}


bool CNTV2VPID::IsStandard3Ga (void) const
{
	switch(GetStandard())
	{
	case VPIDStandard_720_3Ga:				//	Not to be used for aspect ratio. 720p is always zero.
	case VPIDStandard_1080_3Ga:
	case VPIDStandard_1080_Dual_3Ga:
	case VPIDStandard_2160_QuadLink_3Ga:
		return true;
	default:
		break;
	}

	return false;
}

bool CNTV2VPID::IsStandardMultiLink4320 (void) const
{
	switch(GetStandard())
	{
	case VPIDStandard_4320_DualLink_12Gb:
	case VPIDStandard_4320_QuadLink_12Gb:
		return true;
	default:
		break;
	}

	return false;
}


bool CNTV2VPID::IsStandardTwoSampleInterleave (void) const
{
	switch(GetStandard())
	{
	case VPIDStandard_2160_DualLink:
	case VPIDStandard_2160_QuadLink_3Ga:
	case VPIDStandard_2160_QuadDualLink_3Gb:
		return true;
	default:
		break;
	}

	return false;
}

CNTV2VPID & CNTV2VPID::SetVersion (const VPIDVersion inVPIDVersion)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDVersionID) |
		((ULWord(inVPIDVersion) << kRegShiftVPIDVersionID) & kRegMaskVPIDVersionID);
	return *this;
}


VPIDVersion CNTV2VPID::GetVersion (void) const
{
	return VPIDVersion((m_uVPID & kRegMaskVPIDVersionID) >> kRegShiftVPIDVersionID); 
}


CNTV2VPID & CNTV2VPID::SetStandard (const VPIDStandard inStandard)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDStandard) |
		((ULWord(inStandard) << kRegShiftVPIDStandard) & kRegMaskVPIDStandard);
	return *this;
}


VPIDStandard CNTV2VPID::GetStandard (void) const
{
	return VPIDStandard((m_uVPID & kRegMaskVPIDStandard) >> kRegShiftVPIDStandard); 
}


CNTV2VPID & CNTV2VPID::SetProgressiveTransport (const bool inIsProgressiveTransport)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDProgressiveTransport) |
		(((inIsProgressiveTransport ? 1UL : 0UL) << kRegShiftVPIDProgressiveTransport) & kRegMaskVPIDProgressiveTransport);
	return *this;
}


bool CNTV2VPID::GetProgressiveTransport (void) const
{
	 return (m_uVPID & kRegMaskVPIDProgressiveTransport) != 0; 
}


CNTV2VPID & CNTV2VPID::SetProgressivePicture (const bool inIsProgressivePicture)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDProgressivePicture) |
		(((inIsProgressivePicture ? 1UL : 0UL) << kRegShiftVPIDProgressivePicture) & kRegMaskVPIDProgressivePicture);
	return *this;
}


bool CNTV2VPID::GetProgressivePicture (void) const
{
	return (m_uVPID & kRegMaskVPIDProgressivePicture) != 0; 
}


CNTV2VPID & CNTV2VPID::SetPictureRate (const VPIDPictureRate inPictureRate)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDPictureRate) |
		((ULWord(inPictureRate) << kRegShiftVPIDPictureRate) & kRegMaskVPIDPictureRate);
	return *this;
}

VPIDPictureRate CNTV2VPID::GetPictureRate (void) const
{
	return VPIDPictureRate((m_uVPID & kRegMaskVPIDPictureRate) >> kRegShiftVPIDPictureRate); 
}


CNTV2VPID & CNTV2VPID::SetImageAspect16x9 (const bool inIs16x9Aspect)
{
	VPIDStandard standard = GetStandard();
	if(standard == VPIDStandard_1080	||
		standard == VPIDStandard_1080_DualLink ||
		standard == VPIDStandard_1080_DualLink_3Gb ||
		standard == VPIDStandard_2160_QuadDualLink_3Gb ||
		standard == VPIDStandard_2160_DualLink)
	{
		m_uVPID = (m_uVPID & ~kRegMaskVPIDImageAspect16x9Alt) |
			(((inIs16x9Aspect ? 1UL : 0UL) << kRegShiftVPIDImageAspect16x9Alt) & kRegMaskVPIDImageAspect16x9Alt);
	}
	else
	{
		m_uVPID = (m_uVPID & ~kRegMaskVPIDImageAspect16x9) |
			(((inIs16x9Aspect ? 1UL : 0UL) << kRegShiftVPIDImageAspect16x9) & kRegMaskVPIDImageAspect16x9);
	}
	return *this;
}


bool CNTV2VPID::GetImageAspect16x9 (void) const
{
	VPIDStandard standard = GetStandard();
	if(standard == VPIDStandard_1080	||
		standard == VPIDStandard_1080_DualLink ||
		standard == VPIDStandard_1080_DualLink_3Gb ||
		standard == VPIDStandard_2160_QuadDualLink_3Gb ||
		standard == VPIDStandard_2160_DualLink)
	{
		return (m_uVPID & kRegMaskVPIDImageAspect16x9Alt) != 0;
	}
	return (m_uVPID & kRegMaskVPIDImageAspect16x9) != 0; 
}


CNTV2VPID & CNTV2VPID::SetSampling (const VPIDSampling inSampling)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDSampling) |
		((ULWord(inSampling) << kRegShiftVPIDSampling) & kRegMaskVPIDSampling);
	return *this;
}


VPIDSampling CNTV2VPID::GetSampling (void) const
{
	return VPIDSampling((m_uVPID & kRegMaskVPIDSampling) >> kRegShiftVPIDSampling); 
}

bool CNTV2VPID::IsRGBSampling (void) const
{
	switch (GetSampling())
	{
		case VPIDSampling_GBR_444:
		case VPIDSampling_GBRA_4444:
		case VPIDSampling_GBRD_4444:	return true;
		default:	break;
	}
	return false;
}

CNTV2VPID & CNTV2VPID::SetChannel (const VPIDChannel inChannel)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDChannel) |
		((ULWord(inChannel) << kRegShiftVPIDChannel) & kRegMaskVPIDChannel);
	return *this;
}


VPIDChannel CNTV2VPID::GetChannel (void) const
{
	return VPIDChannel((m_uVPID & kRegMaskVPIDChannel) >> kRegShiftVPIDChannel); 
}


CNTV2VPID & CNTV2VPID::SetDualLinkChannel (const VPIDChannel inChannel)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDDualLinkChannel) |
		((ULWord(inChannel) << kRegShiftVPIDDualLinkChannel) & kRegMaskVPIDDualLinkChannel);
	return *this;
}


VPIDChannel CNTV2VPID::GetDualLinkChannel (void) const
{
	return VPIDChannel((m_uVPID & kRegMaskVPIDDualLinkChannel) >> kRegShiftVPIDDualLinkChannel);
}


CNTV2VPID & CNTV2VPID::SetBitDepth (const VPIDBitDepth inBitDepth)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDBitDepth) |
		((ULWord(inBitDepth) << kRegShiftVPIDBitDepth) & kRegMaskVPIDBitDepth);
	return *this;
}


VPIDBitDepth CNTV2VPID::GetBitDepth (void) const
{
	return VPIDBitDepth((m_uVPID & kRegMaskVPIDBitDepth) >> kRegShiftVPIDBitDepth); 
}

CNTV2VPID & CNTV2VPID::SetTransferCharacteristics (const NTV2VPIDXferChars iXferChars)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDXferChars) |
		((ULWord(iXferChars) << kRegShiftVPIDXferChars) & kRegMaskVPIDXferChars);
	return *this;
}


NTV2VPIDXferChars CNTV2VPID::GetTransferCharacteristics (void) const
{
	return NTV2VPIDXferChars((m_uVPID & kRegMaskVPIDXferChars) >> kRegShiftVPIDXferChars); 
}

CNTV2VPID & CNTV2VPID::SetColorimetry (const NTV2VPIDColorimetry inColorimetry)
{
	VPIDStandard standard (GetStandard());
	if (standard == VPIDStandard_1080	||
		standard == VPIDStandard_1080_DualLink ||
		standard == VPIDStandard_1080_DualLink_3Gb ||
		standard == VPIDStandard_2160_QuadDualLink_3Gb ||
		standard == VPIDStandard_2160_DualLink)
	{
		ULWord lowBit(0), highBit(0);
		highBit = (inColorimetry&0x2)>>1;
		lowBit = inColorimetry&0x1;
		m_uVPID = (m_uVPID & ~kRegMaskVPIDColorimetryAltHigh) |
			((highBit << kRegShiftVPIDColorimetryAltHigh) & kRegMaskVPIDColorimetryAltHigh);
		m_uVPID = (m_uVPID & ~kRegMaskVPIDColorimetryAltLow) |
			((lowBit << kRegShiftVPIDColorimetryAltLow) & kRegMaskVPIDColorimetryAltLow);
	}
	else
	{
		m_uVPID = (m_uVPID & ~kRegMaskVPIDColorimetry) |
			((ULWord(inColorimetry) << kRegShiftVPIDColorimetry) & kRegMaskVPIDColorimetry);
	}
	return *this;
}


NTV2VPIDColorimetry CNTV2VPID::GetColorimetry (void) const
{
	VPIDStandard standard (GetStandard());
	if(standard == VPIDStandard_1080	||
		standard == VPIDStandard_1080_DualLink ||
		standard == VPIDStandard_1080_DualLink_3Gb ||
		standard == VPIDStandard_2160_QuadDualLink_3Gb ||
		standard == VPIDStandard_2160_DualLink)
	{
		//The bits are sparse so...
		uint8_t lowBit(0), highBit(0), value(0);
		lowBit = (m_uVPID & kRegMaskVPIDColorimetryAltLow) >> kRegShiftVPIDColorimetryAltLow;
		highBit = (m_uVPID & kRegMaskVPIDColorimetryAltHigh) >> kRegShiftVPIDColorimetryAltHigh;
		value = (highBit << 1)|lowBit;
		return NTV2VPIDColorimetry(value);
	}
	return NTV2VPIDColorimetry((m_uVPID & kRegMaskVPIDColorimetry) >> kRegShiftVPIDColorimetry); 
}

CNTV2VPID & CNTV2VPID::SetLuminance (const NTV2VPIDLuminance inLuminance)
{
	m_uVPID = (m_uVPID & ~kRegmaskVPIDLuminance) |
		((ULWord(inLuminance) << kRegShiftVPIDLuminance) & kRegmaskVPIDLuminance);
	return *this;
}


NTV2VPIDLuminance CNTV2VPID::GetLuminance (void) const
{
	return NTV2VPIDLuminance((m_uVPID & kRegmaskVPIDLuminance) >> kRegShiftVPIDLuminance); 
}

CNTV2VPID & CNTV2VPID::SetRGBRange (const NTV2VPIDRGBRange inRGBRange)
{	
	switch(GetBitDepth())
	{
	case VPIDBitDepth_10_Full:
	case VPIDBitDepth_10:
		if(inRGBRange == NTV2_VPID_Range_Narrow || !IsRGBSampling())
			SetBitDepth(VPIDBitDepth_10);
		else
			SetBitDepth(VPIDBitDepth_10_Full);
	break;
	case VPIDBitDepth_12_Full:
	case VPIDBitDepth_12:
		if(inRGBRange == NTV2_VPID_Range_Narrow || !IsRGBSampling())
			SetBitDepth(VPIDBitDepth_12);
		else
			SetBitDepth(VPIDBitDepth_12_Full);
	}

	return *this;
}


NTV2VPIDRGBRange CNTV2VPID::GetRGBRange (void) const
{
	if(!IsRGBSampling())
		return NTV2_VPID_Range_Narrow;
	
	switch(GetBitDepth())
	{
	case VPIDBitDepth_10_Full:
	case VPIDBitDepth_12_Full:
		return NTV2_VPID_Range_Full;
	default:
		return NTV2_VPID_Range_Narrow;
	}
}

#if !defined (NTV2_DEPRECATE)
void CNTV2VPID::SetDynamicRange (const VPIDDynamicRange inDynamicRange)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDDynamicRange) |
		(((ULWord)inDynamicRange << kRegShiftVPIDDynamicRange) & kRegMaskVPIDDynamicRange);
}


VPIDDynamicRange CNTV2VPID::GetDynamicRange (void) const
{
	return (VPIDDynamicRange)((m_uVPID & kRegMaskVPIDDynamicRange) >> kRegShiftVPIDDynamicRange); 
}
#endif


//	static - this one doesn't support 3Gb
bool CNTV2VPID::SetVPIDData (ULWord &					outVPID,
							const NTV2VideoFormat		inOutputFormat,
							const NTV2FrameBufferFormat	inFrameBufferFormat,
							const bool					inIsProgressive,
							const bool					inIs16x9Aspect,
							const VPIDChannel			inChannel,
							const bool					inUseChannel)
{
	(void)inIsProgressive;
	(void)inIs16x9Aspect;
	
	bool		bRGB48Bit		(false);
	bool		bDualLinkRGB	(false);
	const bool	kOutput3Gb		(false);
	
	switch(inFrameBufferFormat)
	{
	case NTV2_FBF_48BIT_RGB:
		bRGB48Bit = true;
		bDualLinkRGB = true;			// RGB 12 bit
		break;
	case NTV2_FBF_ARGB:
	case NTV2_FBF_RGBA:
	case NTV2_FBF_ABGR:
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_10BIT_DPX:
	case NTV2_FBF_24BIT_RGB:
	case NTV2_FBF_24BIT_BGR:
    case NTV2_FBF_10BIT_DPX_LE:
	case NTV2_FBF_10BIT_RGB_PACKED:
	case NTV2_FBF_10BIT_ARGB:
	case NTV2_FBF_16BIT_ARGB:
		bDualLinkRGB = true;			// RGB 8+10 bit;
		bRGB48Bit = false;
		break;
	default:
		bDualLinkRGB = false;			// All other
		bRGB48Bit = false;
		break;
	}
	
	// should use this call directly
	return SetVPIDData (outVPID,
						inOutputFormat,
						bDualLinkRGB,
						bRGB48Bit,
						kOutput3Gb,
						false,			// not ST525
						inChannel,
						inUseChannel);
}


bool CNTV2VPID::SetVPIDData (ULWord &				outVPID,
							const NTV2VideoFormat	inOutputFormat,
							const bool				inIsDualLinkRGB,
							const bool				inIsRGB48Bit,
							const bool				inOutputIs3Gb,
							const bool				inIsSMPTE425,
							const VPIDChannel		inChannel,
							const bool				inUseChannel,
							const bool				inOutputIs6G,
							const bool				inOutputIs12G,
							const NTV2VPIDXferChars	inXferChars,
							const NTV2VPIDColorimetry	inColorimetry,
							const NTV2VPIDLuminance	inLuminance,
							const NTV2VPIDRGBRange	inRGBRange)
{
	VPIDSpec vpidSpec;


	::memset (&vpidSpec, 0, sizeof(vpidSpec));

	vpidSpec.videoFormat			= inOutputFormat;
	vpidSpec.pixelFormat			= inIsRGB48Bit ? NTV2_FBF_48BIT_RGB : NTV2_FBF_INVALID;
	vpidSpec.isRGBOnWire			= inIsDualLinkRGB;
	vpidSpec.isOutputLevelA			= (NTV2_IS_3G_FORMAT (inOutputFormat) && ! inOutputIs3Gb) ? true : false;
	vpidSpec.isOutputLevelB			= inOutputIs3Gb;
	vpidSpec.isDualLink				= inIsDualLinkRGB || (NTV2_IS_372_DUALLINK_FORMAT(inOutputFormat) && !vpidSpec.isOutputLevelA);
	vpidSpec.isTwoSampleInterleave	= inIsSMPTE425;
	vpidSpec.useChannel				= inUseChannel;
	vpidSpec.vpidChannel			= inChannel;
	vpidSpec.isStereo				= false;
	vpidSpec.isRightEye				= false;
	vpidSpec.audioCarriage			= VPIDAudio_Unknown;
	vpidSpec.isOutput6G				= inOutputIs6G;
	vpidSpec.isOutput12G			= inOutputIs12G;
	vpidSpec.transferCharacteristics = inXferChars;
	vpidSpec.colorimetry			= inColorimetry;
	vpidSpec.luminance				= inLuminance;
	vpidSpec.rgbRange				= inRGBRange;

	return ::SetVPIDFromSpec (&outVPID, &vpidSpec);
}


NTV2VideoFormat CNTV2VPID::GetVideoFormat (void) const
{
	NTV2VideoFormat  videoFormat = NTV2_FORMAT_UNKNOWN;
	VPIDStandard vpidStandard	 = GetStandard();
	bool vpidProgPicture         = GetProgressivePicture();
	bool vpidProgTransport       = GetProgressiveTransport();
	bool vpidHorizontal2048      = ((m_uVPID & kRegMaskVPIDHorizontalSampling) != 0);

	VPIDPictureRate vpidFrameRate = GetPictureRate();

	switch (vpidStandard)
	{
	case VPIDStandard_483_576:							
		switch (vpidFrameRate)
		{
		case VPIDPictureRate_2500:
			videoFormat = NTV2_FORMAT_625_5000;
			break;
		case VPIDPictureRate_2997:
			videoFormat = NTV2_FORMAT_525_5994;
			break;
		default:
			break;
		}
		break;
	case VPIDStandard_720:
		switch (vpidFrameRate)
		{
		case VPIDPictureRate_5000:
			videoFormat = NTV2_FORMAT_720p_5000;
			break;
		case VPIDPictureRate_5994:
			videoFormat = NTV2_FORMAT_720p_5994;
			break;
		case VPIDPictureRate_6000:
			videoFormat = NTV2_FORMAT_720p_6000;
			break;
		default:
			break;
		}
		break;
	case VPIDStandard_720_3Gb:
		videoFormat = stTable720p[vpidFrameRate];
		break;
	case VPIDStandard_1080:
	case VPIDStandard_1080_DualLink:
	case VPIDStandard_1080_3Ga:
	case VPIDStandard_1080_3Gb:
	case VPIDStandard_1080_DualLink_3Gb:
	case VPIDStandard_1080_Dual_3Ga:
		if (vpidProgPicture)
		{
			if (vpidProgTransport)
			{
				if (vpidHorizontal2048)
				{
					videoFormat = stTable2048p[vpidFrameRate];
				}
				else
				{
					videoFormat = stTable1920p[vpidFrameRate];
				}
			}
			else
			{
				if (vpidHorizontal2048)
				{
					videoFormat = stTable2048psf[vpidFrameRate];
				}
				else
				{
					videoFormat = stTable1920psf[vpidFrameRate];
				}
			}
		}
		else
		{
			if (vpidHorizontal2048)
			{
				videoFormat = stTable2048i[vpidFrameRate];
			}
			else
			{
				videoFormat = stTable1920i[vpidFrameRate];
			}
		}
		break;
	case VPIDStandard_2160_DualLink:
	case VPIDStandard_2160_QuadLink_3Ga:
	case VPIDStandard_2160_QuadDualLink_3Gb:
		if (vpidProgTransport)
		{
			if (vpidHorizontal2048)
			{
				videoFormat = stTable4096pSID[vpidFrameRate];
			}
			else
			{
				videoFormat = stTable3840pSID[vpidFrameRate];
			}
		}
		else
		{
			if (vpidHorizontal2048)
			{
				videoFormat = stTable4096psfSID[vpidFrameRate];
			}
			else
			{
				videoFormat = stTable3840psfSID[vpidFrameRate];
			}
		}
		break;
	case VPIDStandard_2160_Single_6Gb:
	case VPIDStandard_2160_Single_12Gb:
		if (vpidHorizontal2048)
		{
			videoFormat = stTable4096pTSI[vpidFrameRate];
		}
		else
		{
			videoFormat = stTable3840pTSI[vpidFrameRate];
		}
		break;
	case VPIDStandard_2160_DualLink_12Gb:
		if (vpidHorizontal2048)
		{
			videoFormat = stTable4096pTSI[vpidFrameRate];
		}
		else
		{
			videoFormat = stTable3840pTSI[vpidFrameRate];
		}
		break;
	case VPIDStandard_4320_DualLink_12Gb:
	case VPIDStandard_4320_QuadLink_12Gb:
		if (vpidHorizontal2048)
		{
			videoFormat = stTable8192p[vpidFrameRate];
		}
		else
		{
			videoFormat = stTable7680p[vpidFrameRate];
		}
		break;
	default:
		break;
	}

	return videoFormat;
}

// Macro to simplify returning of string for given enum
#define VPID_ENUM_CASE_RETURN_STR(enum_name) case(enum_name): return #enum_name

const string CNTV2VPID::VersionString (const VPIDVersion version)
{
	switch (version)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDVersion_0);
		VPID_ENUM_CASE_RETURN_STR(VPIDVersion_1);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::StandardString (const VPIDStandard std)
{
	switch (std)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_Unknown);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_483_576);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_483_576_DualLink);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_483_576_540Mbs);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_720);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_483_576_1485Mbs);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_DualLink);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_720_3Ga);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_3Ga);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_DualLink_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_720_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_483_576_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_720_Stereo_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_Stereo_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_QuadLink);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_720_Stereo_3Ga);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_Stereo_3Ga);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_Stereo_DualLink_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_Dual_3Ga);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_Dual_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_2160_DualLink);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_2160_QuadLink_3Ga);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_2160_QuadDualLink_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_Stereo_Quad_3Ga);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_Stereo_Quad_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_2160_Stereo_Quad_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_OctLink);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_UHDTV1_Single_DualLink_10Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_UHDTV2_Quad_OctaLink_10Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_UHDTV1_MultiLink_10Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_UHDTV2_MultiLink_10Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_VC2);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_720_1080_Stereo);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_VC2_Level65_270Mbs);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_4K_DCPIF_FSW709_10Gbs);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_FT_2048x1556_Dual);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_FT_2048x1556_3Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_2160_Single_6Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_Single_6Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_AFR_Single_6Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_2160_Single_12Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_1080_10_12_AFR_Single_12Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_4320_DualLink_12Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_2160_DualLink_12Gb);
		VPID_ENUM_CASE_RETURN_STR(VPIDStandard_4320_QuadLink_12Gb);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::PictureRateString (const VPIDPictureRate rate)
{
	switch (rate)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_None);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_Reserved1);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_2398);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_2400);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_4795);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_2500);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_2997);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_3000);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_4800);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_5000);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_5994);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_6000);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_ReservedC);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_ReservedD);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_ReservedE);
		VPID_ENUM_CASE_RETURN_STR(VPIDPictureRate_ReservedF);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::SamplingString (const VPIDSampling sample)
{
	switch (sample)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_YUV_422);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_YUV_444);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_GBR_444);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_YUV_420);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_YUVA_4224);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_YUVA_4444);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_GBRA_4444);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_Reserved7);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_YUVD_4224);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_YUVD_4444);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_GBRD_4444);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_ReservedB);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_ReservedC);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_ReservedD);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_ReservedE);
		VPID_ENUM_CASE_RETURN_STR(VPIDSampling_XYZ_444);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::ChannelString (const VPIDChannel chan)
{
	switch (chan)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDChannel_1);
		VPID_ENUM_CASE_RETURN_STR(VPIDChannel_2);
		VPID_ENUM_CASE_RETURN_STR(VPIDChannel_3);
		VPID_ENUM_CASE_RETURN_STR(VPIDChannel_4);
		VPID_ENUM_CASE_RETURN_STR(VPIDChannel_5);
		VPID_ENUM_CASE_RETURN_STR(VPIDChannel_6);
		VPID_ENUM_CASE_RETURN_STR(VPIDChannel_7);
		VPID_ENUM_CASE_RETURN_STR(VPIDChannel_8);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::DynamicRangeString (const VPIDDynamicRange range)
{
	switch (range)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDDynamicRange_100);
		VPID_ENUM_CASE_RETURN_STR(VPIDDynamicRange_200);
		VPID_ENUM_CASE_RETURN_STR(VPIDDynamicRange_400);
		VPID_ENUM_CASE_RETURN_STR(VPIDDynamicRange_Reserved3);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::BitDepthString (const VPIDBitDepth depth)
{
	switch (depth)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDBitDepth_10_Full);
		VPID_ENUM_CASE_RETURN_STR(VPIDBitDepth_10);
		VPID_ENUM_CASE_RETURN_STR(VPIDBitDepth_12);
		VPID_ENUM_CASE_RETURN_STR(VPIDBitDepth_12_Full);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::LinkString (const VPIDLink link)
{
	switch (link)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDLink_1);
		VPID_ENUM_CASE_RETURN_STR(VPIDLink_2);
		VPID_ENUM_CASE_RETURN_STR(VPIDLink_3);
		VPID_ENUM_CASE_RETURN_STR(VPIDLink_4);
		VPID_ENUM_CASE_RETURN_STR(VPIDLink_5);
		VPID_ENUM_CASE_RETURN_STR(VPIDLink_6);
		VPID_ENUM_CASE_RETURN_STR(VPIDLink_7);
		VPID_ENUM_CASE_RETURN_STR(VPIDLink_8);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::AudioString (const VPIDAudio audio)
{
	switch (audio)
	{
		VPID_ENUM_CASE_RETURN_STR(VPIDAudio_Unknown);
		VPID_ENUM_CASE_RETURN_STR(VPIDAudio_Copied);
		VPID_ENUM_CASE_RETURN_STR(VPIDAudio_Additional);
		VPID_ENUM_CASE_RETURN_STR(VPIDAudio_Reserved);
		// intentionally not setting a default: so compiler will warn about missing enums
	}
	return "";
}

const string CNTV2VPID::VPIDVersionToString (const VPIDVersion inVers)
{
	switch (inVers)
	{
		case VPIDVersion_0:	return "0";
		case VPIDVersion_1:	return "1";
	#if !defined(_DEBUG)
		default:	break;
	#endif
	}
	return "";
}

const string CNTV2VPID::VPIDStandardToString (const VPIDStandard inStd)
{
	switch (inStd)
	{
		case VPIDStandard_Unknown:						return "Unknown";
		case VPIDStandard_483_576:						return "Standard Definition";
		case VPIDStandard_483_576_DualLink:				return "SD Dual Link?";
		case VPIDStandard_483_576_540Mbs:				return "SD 540Mbs?";
		case VPIDStandard_720:							return "720 Single Link";
		case VPIDStandard_1080:							return "1080 Single Link";
		case VPIDStandard_483_576_1485Mbs:				return "SD 1485Mbs?";
		case VPIDStandard_1080_DualLink:				return "1080 Dual Link";
		case VPIDStandard_720_3Ga:						return "720 3G Level A";
		case VPIDStandard_1080_3Ga:						return "1080 3G Level A";
		case VPIDStandard_1080_DualLink_3Gb:			return "1080 Dual Link 3G Level B";
		case VPIDStandard_720_3Gb:						return "2x720 3G Level B";
		case VPIDStandard_1080_3Gb:						return "2x1080 3G Level B";
		case VPIDStandard_483_576_3Gb:					return "SD 3G Level B?";
		case VPIDStandard_720_Stereo_3Gb:				return "720_Stereo_3Gb";
		case VPIDStandard_1080_Stereo_3Gb:				return "1080_Stereo_3Gb";
		case VPIDStandard_1080_QuadLink:				return "1080 Quad Link";
		case VPIDStandard_720_Stereo_3Ga:				return "720_Stereo_3Ga";
		case VPIDStandard_1080_Stereo_3Ga:				return "1080_Stereo_3Ga";
		case VPIDStandard_1080_Stereo_DualLink_3Gb:		return "1080_Stereo_DualLink_3Gb";
		case VPIDStandard_1080_Dual_3Ga:				return "1080 Dual Link 3Ga";
		case VPIDStandard_1080_Dual_3Gb:				return "1080 Dual Link 3Gb";
		case VPIDStandard_2160_DualLink:				return "2160 Dual Link";
		case VPIDStandard_2160_QuadLink_3Ga:			return "2160 Quad Link 3Ga";
		case VPIDStandard_2160_QuadDualLink_3Gb:		return "2160 Quad Dual Link 3Gb";
		case VPIDStandard_1080_Stereo_Quad_3Ga:			return "1080_Stereo_Quad_3Ga";
		case VPIDStandard_1080_Stereo_Quad_3Gb:			return "1080_Stereo_Quad_3Gb";
		case VPIDStandard_2160_Stereo_Quad_3Gb:			return "2160_Stereo_Quad_3Gb";
		case VPIDStandard_1080_OctLink:					return "1080 Octa Link";
		case VPIDStandard_UHDTV1_Single_DualLink_10Gb:	return "UHDTV1_Single_DualLink_10Gb";
		case VPIDStandard_UHDTV2_Quad_OctaLink_10Gb:	return "UHDTV2_Quad_OctaLink_10Gb";
		case VPIDStandard_UHDTV1_MultiLink_10Gb:		return "UHDTV1_MultiLink_10Gb";
		case VPIDStandard_UHDTV2_MultiLink_10Gb:		return "UHDTV2_MultiLink_10Gb";
		case VPIDStandard_VC2:							return "VC2";
		case VPIDStandard_720_1080_Stereo:				return "720_1080_Stereo";
		case VPIDStandard_VC2_Level65_270Mbs:			return "VC2_Level65_270Mbs";
		case VPIDStandard_4K_DCPIF_FSW709_10Gbs:		return "4K_DCPIF_FSW709_10Gbs";
		case VPIDStandard_FT_2048x1556_Dual:			return "FT_2048x1556_Dual";
		case VPIDStandard_FT_2048x1556_3Gb:				return "FT_2048x1556_3Gb";
		case VPIDStandard_2160_Single_6Gb:				return "2160_Single_6Gb";
		case VPIDStandard_1080_Single_6Gb:				return "1080_Single_6Gb";
		case VPIDStandard_1080_AFR_Single_6Gb:			return "1080_AFR_Single_6Gb";
		case VPIDStandard_2160_Single_12Gb:				return "2160_Single_12Gb";
		case VPIDStandard_1080_10_12_AFR_Single_12Gb:	return "1080_10_12_AFR_Single_12Gb";
		case VPIDStandard_4320_DualLink_12Gb:			return "4320_DualLink_12Gb";
		case VPIDStandard_2160_DualLink_12Gb:			return "2160_DualLink_12Gb";
		case VPIDStandard_4320_QuadLink_12Gb:			return "4320_QuadLink_12Gb";
	#if !defined(_DEBUG)
		default:	break;
	#endif
	}
	return "";
}

bool CNTV2VPID::VPIDStandardIsSingleLink (const VPIDStandard inStd)
{
	switch (inStd)
	{
		case VPIDStandard_483_576:
		case VPIDStandard_483_576_540Mbs:
		case VPIDStandard_720:
		case VPIDStandard_1080:
		case VPIDStandard_720_3Ga:
		case VPIDStandard_1080_3Ga:
		case VPIDStandard_720_3Gb:
		case VPIDStandard_1080_3Gb:
		case VPIDStandard_483_576_3Gb:
		case VPIDStandard_VC2:
		case VPIDStandard_VC2_Level65_270Mbs:
		case VPIDStandard_FT_2048x1556_3Gb:
		case VPIDStandard_2160_Single_6Gb:
		case VPIDStandard_1080_Single_6Gb:
		case VPIDStandard_1080_AFR_Single_6Gb:
		case VPIDStandard_2160_Single_12Gb:
		case VPIDStandard_1080_10_12_AFR_Single_12Gb:	return true;

		case VPIDStandard_720_Stereo_3Gb:
		case VPIDStandard_1080_Stereo_3Gb:
		case VPIDStandard_483_576_1485Mbs:
		case VPIDStandard_720_Stereo_3Ga:
		case VPIDStandard_1080_Stereo_3Ga:
//		case VPIDStandard_483_576_360Mbs:
		case VPIDStandard_483_576_DualLink:
		case VPIDStandard_1080_DualLink:
		case VPIDStandard_1080_DualLink_3Gb:
		case VPIDStandard_1080_QuadLink:
		case VPIDStandard_1080_Stereo_DualLink_3Gb:
		case VPIDStandard_1080_Dual_3Ga:
		case VPIDStandard_1080_Dual_3Gb:
		case VPIDStandard_2160_DualLink:
		case VPIDStandard_2160_QuadLink_3Ga:
		case VPIDStandard_2160_QuadDualLink_3Gb:
		case VPIDStandard_1080_Stereo_Quad_3Ga:
		case VPIDStandard_1080_Stereo_Quad_3Gb:
		case VPIDStandard_2160_Stereo_Quad_3Gb:
		case VPIDStandard_1080_OctLink:
		case VPIDStandard_UHDTV1_Single_DualLink_10Gb:
		case VPIDStandard_UHDTV2_Quad_OctaLink_10Gb:
		case VPIDStandard_UHDTV1_MultiLink_10Gb:
		case VPIDStandard_UHDTV2_MultiLink_10Gb:
		case VPIDStandard_720_1080_Stereo:
		case VPIDStandard_4K_DCPIF_FSW709_10Gbs:
		case VPIDStandard_FT_2048x1556_Dual:
		case VPIDStandard_4320_DualLink_12Gb:
		case VPIDStandard_2160_DualLink_12Gb:
		case VPIDStandard_4320_QuadLink_12Gb:
		default:	break;
	}
	return false;
}

bool CNTV2VPID::VPIDStandardIsDualLink (const VPIDStandard inStd)
{
	switch (inStd)
	{
		case VPIDStandard_720_Stereo_3Gb:
		case VPIDStandard_1080_Stereo_3Gb:
		case VPIDStandard_483_576_1485Mbs:
		case VPIDStandard_720_Stereo_3Ga:
		case VPIDStandard_1080_Stereo_3Ga:
//		case VPIDStandard_483_576_360Mbs:
		case VPIDStandard_483_576_DualLink:
		case VPIDStandard_1080_DualLink:
		case VPIDStandard_1080_DualLink_3Gb:
		case VPIDStandard_1080_Stereo_DualLink_3Gb:
		case VPIDStandard_1080_Dual_3Ga:
		case VPIDStandard_1080_Dual_3Gb:
		case VPIDStandard_2160_DualLink:
		case VPIDStandard_2160_QuadDualLink_3Gb:
		case VPIDStandard_UHDTV1_Single_DualLink_10Gb:
		case VPIDStandard_FT_2048x1556_Dual:
		case VPIDStandard_4320_DualLink_12Gb:
		case VPIDStandard_2160_DualLink_12Gb:
		case VPIDStandard_4320_QuadLink_12Gb:	return true;

		case VPIDStandard_483_576:
		case VPIDStandard_483_576_540Mbs:
		case VPIDStandard_720:
		case VPIDStandard_1080:
		case VPIDStandard_720_3Ga:
		case VPIDStandard_1080_3Ga:
		case VPIDStandard_720_3Gb:
		case VPIDStandard_1080_3Gb:
		case VPIDStandard_483_576_3Gb:
		case VPIDStandard_VC2:
		case VPIDStandard_VC2_Level65_270Mbs:
		case VPIDStandard_FT_2048x1556_3Gb:
		case VPIDStandard_2160_Single_6Gb:
		case VPIDStandard_1080_Single_6Gb:
		case VPIDStandard_1080_AFR_Single_6Gb:
		case VPIDStandard_2160_Single_12Gb:
		case VPIDStandard_1080_10_12_AFR_Single_12Gb:
		case VPIDStandard_1080_QuadLink:
		case VPIDStandard_2160_QuadLink_3Ga:
		case VPIDStandard_1080_Stereo_Quad_3Ga:
		case VPIDStandard_1080_Stereo_Quad_3Gb:
		case VPIDStandard_2160_Stereo_Quad_3Gb:
		case VPIDStandard_1080_OctLink:
		case VPIDStandard_UHDTV2_Quad_OctaLink_10Gb:
		case VPIDStandard_UHDTV1_MultiLink_10Gb:
		case VPIDStandard_UHDTV2_MultiLink_10Gb:
		case VPIDStandard_720_1080_Stereo:
		case VPIDStandard_4K_DCPIF_FSW709_10Gbs:
		default:	break;
	}
	return false;
}

bool CNTV2VPID::VPIDStandardIsQuadLink (const VPIDStandard inStd)
{
	switch (inStd)
	{
		case VPIDStandard_2160_QuadDualLink_3Gb:
		case VPIDStandard_1080_QuadLink:
		case VPIDStandard_2160_QuadLink_3Ga:
		case VPIDStandard_1080_Stereo_Quad_3Ga:
		case VPIDStandard_1080_Stereo_Quad_3Gb:
		case VPIDStandard_2160_Stereo_Quad_3Gb:
		case VPIDStandard_UHDTV2_Quad_OctaLink_10Gb:
		case VPIDStandard_4320_QuadLink_12Gb:		return true;

		case VPIDStandard_720_Stereo_3Gb:
		case VPIDStandard_1080_Stereo_3Gb:
		case VPIDStandard_483_576_1485Mbs:
		case VPIDStandard_720_Stereo_3Ga:
		case VPIDStandard_1080_Stereo_3Ga:
//		case VPIDStandard_483_576_360Mbs:
		case VPIDStandard_483_576_DualLink:
		case VPIDStandard_1080_DualLink:
		case VPIDStandard_1080_DualLink_3Gb:
		case VPIDStandard_1080_Stereo_DualLink_3Gb:
		case VPIDStandard_1080_Dual_3Ga:
		case VPIDStandard_1080_Dual_3Gb:
		case VPIDStandard_2160_DualLink:
		case VPIDStandard_UHDTV1_Single_DualLink_10Gb:
		case VPIDStandard_FT_2048x1556_Dual:
		case VPIDStandard_4320_DualLink_12Gb:
		case VPIDStandard_2160_DualLink_12Gb:
		case VPIDStandard_483_576:
		case VPIDStandard_483_576_540Mbs:
		case VPIDStandard_720:
		case VPIDStandard_1080:
		case VPIDStandard_720_3Ga:
		case VPIDStandard_1080_3Ga:
		case VPIDStandard_720_3Gb:
		case VPIDStandard_1080_3Gb:
		case VPIDStandard_483_576_3Gb:
		case VPIDStandard_VC2:
		case VPIDStandard_VC2_Level65_270Mbs:
		case VPIDStandard_FT_2048x1556_3Gb:
		case VPIDStandard_2160_Single_6Gb:
		case VPIDStandard_1080_Single_6Gb:
		case VPIDStandard_1080_AFR_Single_6Gb:
		case VPIDStandard_2160_Single_12Gb:
		case VPIDStandard_1080_10_12_AFR_Single_12Gb:
		case VPIDStandard_1080_OctLink:
		case VPIDStandard_UHDTV1_MultiLink_10Gb:
		case VPIDStandard_UHDTV2_MultiLink_10Gb:
		case VPIDStandard_720_1080_Stereo:
		case VPIDStandard_4K_DCPIF_FSW709_10Gbs:
		default:	break;
	}
	return false;
}

bool CNTV2VPID::VPIDStandardIsOctLink (const VPIDStandard inStd)
{
	switch (inStd)
	{
		case VPIDStandard_UHDTV2_Quad_OctaLink_10Gb:
		case VPIDStandard_1080_OctLink:		return true;

		case VPIDStandard_2160_QuadDualLink_3Gb:
		case VPIDStandard_1080_QuadLink:
		case VPIDStandard_2160_QuadLink_3Ga:
		case VPIDStandard_1080_Stereo_Quad_3Ga:
		case VPIDStandard_1080_Stereo_Quad_3Gb:
		case VPIDStandard_2160_Stereo_Quad_3Gb:
		case VPIDStandard_4320_QuadLink_12Gb:
		case VPIDStandard_720_Stereo_3Gb:
		case VPIDStandard_1080_Stereo_3Gb:
		case VPIDStandard_483_576_1485Mbs:
		case VPIDStandard_720_Stereo_3Ga:
		case VPIDStandard_1080_Stereo_3Ga:
//		case VPIDStandard_483_576_360Mbs:
		case VPIDStandard_483_576_DualLink:
		case VPIDStandard_1080_DualLink:
		case VPIDStandard_1080_DualLink_3Gb:
		case VPIDStandard_1080_Stereo_DualLink_3Gb:
		case VPIDStandard_1080_Dual_3Ga:
		case VPIDStandard_1080_Dual_3Gb:
		case VPIDStandard_2160_DualLink:
		case VPIDStandard_UHDTV1_Single_DualLink_10Gb:
		case VPIDStandard_FT_2048x1556_Dual:
		case VPIDStandard_4320_DualLink_12Gb:
		case VPIDStandard_2160_DualLink_12Gb:
		case VPIDStandard_483_576:
		case VPIDStandard_483_576_540Mbs:
		case VPIDStandard_720:
		case VPIDStandard_1080:
		case VPIDStandard_720_3Ga:
		case VPIDStandard_1080_3Ga:
		case VPIDStandard_720_3Gb:
		case VPIDStandard_1080_3Gb:
		case VPIDStandard_483_576_3Gb:
		case VPIDStandard_VC2:
		case VPIDStandard_VC2_Level65_270Mbs:
		case VPIDStandard_FT_2048x1556_3Gb:
		case VPIDStandard_2160_Single_6Gb:
		case VPIDStandard_1080_Single_6Gb:
		case VPIDStandard_1080_AFR_Single_6Gb:
		case VPIDStandard_2160_Single_12Gb:
		case VPIDStandard_1080_10_12_AFR_Single_12Gb:
		case VPIDStandard_UHDTV1_MultiLink_10Gb:
		case VPIDStandard_UHDTV2_MultiLink_10Gb:
		case VPIDStandard_720_1080_Stereo:
		case VPIDStandard_4K_DCPIF_FSW709_10Gbs:
		default:	break;
	}
	return false;
}


static const string sVPIDPictureRate[]	= {	"None", "Reserved1", "23.98", "24.00", "47.95", "25.00", "29.97", "30.00", "48.00", "50.00", "59.94", "60.00",
											"ReservedC",    "ReservedD",    "ReservedE",    "ReservedF"	};
static const string sVPIDSampling[]		= {	"YCbCr 4:2:2",	"YCbCr 4:4:4",	"GBR 4:4:4",	"YCbCr 4:2:0",	"YCbCrA 4:2:2:4",	"YCbCrA 4:4:4:4",
											"GBRA 4:4:4:4",	"Reserved7",	"YCbCrD 4:2:2:4",	"YCbCrD 4:4:4:4",	"GBRD 4:4:4:4",	"ReservedB",
											"ReservedC",	"ReservedD",	"ReservedE",	"XYZ 4:4:4"	};
static const string sVPIDChannel[]		= {	"1", "2", "3", "4", "5", "6", "7", "8"	};
static const string sVPIDDynamicRange[]	= {	"100", "200", "400", "Reserved3"	};
static const string sVPIDBitDepth[]		= {	"10 Full", "10", "12", "12 Full"	};
static const string sVPIDLink[]			= {	"1", "2", "3", "4", "5", "6", "7", "8"	};
static const string	sVPIDAudio[]		= {	"Unknown", "Copied", "Additional", "Reserved" };
static const string sVPIDTransfer[]		= { "SDR", "HLG", "PQ", "Unspecified" };
static const string sVPIDColorimetry[]	= { "Rec709", "Reserved", "UHDTV", "Unknown" };
static const string sVPIDLuminance[]	= { "YCbCr", "ICtCp" };
static const string sVPIDRGBRange[]	= { "Narrow", "Full" };



ostream & CNTV2VPID::Print (ostream & ostrm) const
{
	ostrm	<< "VPID " << xHEX0N(m_uVPID,8)
			<< ": v" << CNTV2VPID::VPIDVersionToString(GetVersion());
	if (IsValid())
		ostrm	<< " " << CNTV2VPID::VPIDStandardToString(GetStandard())
				<< " " << ::NTV2VideoFormatToString(GetVideoFormat())
				<< " rate=" << sVPIDPictureRate[GetPictureRate()]
				<< " samp=" << sVPIDSampling[GetSampling()]
				<< " chan=" << sVPIDChannel[GetChannel()]
				<< " links=" << (VPIDStandardIsSingleLink(GetStandard()) ? "1" : "mult")
			//	<< " dynRange=" << sVPIDDynamicRange[GetDynamicRange()]
				<< " bitd=" << sVPIDBitDepth[GetBitDepth()]
				<< " 3Ga=" << YesNo(IsStandard3Ga())
				<< " tsi=" << YesNo(IsStandardTwoSampleInterleave())
				<< " 16x9=" << YesNo(GetImageAspect16x9())
				<< " xfer=" << sVPIDTransfer[GetTransferCharacteristics()]
				<< " colo=" << sVPIDColorimetry[GetColorimetry()]
				<< " lumi=" << sVPIDLuminance[GetLuminance()]
				<< " rng=" << sVPIDRGBRange[GetRGBRange()];
	return ostrm;
}

ostream & CNTV2VPID::PrintPretty (ostream & ostrm) const
{
	ostrm	<< "VPID " << xHEX0N(m_uVPID,8) << endl
			<< "Version = " << CNTV2VPID::VPIDVersionToString(GetVersion()) << endl;
	if (IsValid())
		ostrm	<< "Standard =  " << CNTV2VPID::VPIDStandardToString(GetStandard()) << endl
				<< "Format =  " << ::NTV2VideoFormatToString(GetVideoFormat()) << endl
				<< "Frame Rate = " << sVPIDPictureRate[GetPictureRate()] << endl
				<< "Sampling = " << sVPIDSampling[GetSampling()] << endl
				<< "Channel = " << sVPIDChannel[GetChannel()] << endl
				<< "Links = " << (VPIDStandardIsSingleLink(GetStandard()) ? "1" : "mult") << endl
            //	<< " dynRange=" << sVPIDDynamicRange[GetDynamicRange()] << endl
				<< "Bit Depth =" << sVPIDBitDepth[GetBitDepth()] << endl
				<< "3Ga= " << YesNo(IsStandard3Ga()) << endl
				<< "TSI = " << YesNo(IsStandardTwoSampleInterleave()) << endl
				<< "16x9 = " << YesNo(GetImageAspect16x9()) << endl
				<< "Xfer Char = " << sVPIDTransfer[GetTransferCharacteristics()] << endl
				<< "Colorimetry ="  << sVPIDColorimetry[GetColorimetry()] << endl
				<< "Luminance = " << sVPIDLuminance[GetLuminance()] << endl
				<< "RGB Range = " << sVPIDRGBRange[GetRGBRange()] << endl;
	return ostrm;
}

#define	YesOrNo(__x__)		((__x__) ? "Yes" : "No")

AJALabelValuePairs & CNTV2VPID::GetInfo (AJALabelValuePairs & outInfo) const
{
	ostringstream hexConv;	hexConv << xHEX0N(m_uVPID,8);
	AJASystemInfo::append(outInfo, "Raw Value",				hexConv.str());
	AJASystemInfo::append(outInfo, "Version",				CNTV2VPID::VPIDVersionToString(GetVersion()));
	if (!IsValid())
		return outInfo;
	AJASystemInfo::append(outInfo, "Standard",				CNTV2VPID::VPIDStandardToString(GetStandard()));
	AJASystemInfo::append(outInfo, "Video Format",			::NTV2VideoFormatToString(GetVideoFormat()));
	AJASystemInfo::append(outInfo, "Progressive Transport",	YesOrNo(GetProgressiveTransport()));
	AJASystemInfo::append(outInfo, "Progressive Picture",	YesOrNo(GetProgressivePicture()));
	AJASystemInfo::append(outInfo, "Frame Rate",			sVPIDPictureRate[GetPictureRate()]);
	AJASystemInfo::append(outInfo, "Sampling",				sVPIDSampling[GetSampling()]);
	AJASystemInfo::append(outInfo, "Channel",				sVPIDChannel[GetChannel()]);
	AJASystemInfo::append(outInfo, "Links",					VPIDStandardIsSingleLink(GetStandard()) ? "1" : "multiple");
//	AJASystemInfo::append(outInfo, "Dynamic Range",			sVPIDDynamicRange[GetDynamicRange()]);
	AJASystemInfo::append(outInfo, "Bit Depth",				sVPIDBitDepth[GetBitDepth()]);
	AJASystemInfo::append(outInfo, "3Ga",					YesOrNo(IsStandard3Ga()));
	AJASystemInfo::append(outInfo, "Two Sample Interleave",	YesOrNo(IsStandardTwoSampleInterleave()));
	AJASystemInfo::append(outInfo, "Aspect Ratio",			GetImageAspect16x9() ? "16x9" : "4x3");
	AJASystemInfo::append(outInfo, "Xfer Characteristics",	sVPIDTransfer[GetTransferCharacteristics()]);
	AJASystemInfo::append(outInfo, "Colorimetry",			sVPIDColorimetry[GetColorimetry()]);
	AJASystemInfo::append(outInfo, "Luminance",				sVPIDLuminance[GetLuminance()]);
	AJASystemInfo::append(outInfo, "RGB Range",				sVPIDRGBRange[GetRGBRange()]);
	return outInfo;
}

string CNTV2VPID::AsString (const bool inTabular) const
{
	if (inTabular)
	{	AJALabelValuePairs table;
		return AJASystemInfo::ToString(GetInfo(table));
	}
	else
	{	ostringstream oss;
		Print(oss);
		return oss.str();
	}
}

ostream & operator << (std::ostream & ostrm, const CNTV2VPID & inData)
{
	return inData.Print(ostrm);
}
