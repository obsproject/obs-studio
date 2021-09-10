/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2vpidfromspec.h
	@brief		Declares functions for the C implementations of VPID generation from a VPIDSpec.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
	@note		This file is included in driver builds. It must not contain any C++.
**/

#ifndef NTV2VPIDFROMSPEC_H
#define NTV2VPIDFROMSPEC_H

#include "ajaexport.h"
#include "ntv2publicinterface.h"

/**
	@brief	Contains all the information needed to generate a valid VPID
**/

////VPID Stuff
///////////////////////////////////////////////////////////////

typedef struct
{
	NTV2VideoFormat			videoFormat;			///< @brief	Specifies the format of the video stream.
	NTV2FrameBufferFormat	pixelFormat;			///< @brief Specifies the pixel format of the source of the video stream.
	bool					isRGBOnWire;			///< @brief	If true, the transport on the wire is RGB.
	bool					isOutputLevelA;			///< @brief	If true, the video stream will leave the device as a level A signal.
	bool					isOutputLevelB;			///< @brief	If true, the video stream will leave the device as a level B signal.
	bool					isDualLink;				///< @brief	If true, the video stream is part of a SMPTE 372 dual link signal.
	bool					isTwoSampleInterleave;	///< @brief	If true, the video stream is in SMPTE 425-3 two sample interleave format.
	bool					useChannel;				///< @brief	If true, the following vpidChannel value should be inserted into th VPID.
	VPIDChannel				vpidChannel;			///< @brief Specifies the channel number of the video stream.
	bool					isStereo;				///< @brief	If true, the video stream is part of a stereo pair.
	bool					isRightEye;				///< @brief	If true, the video stream is the right eye of a stereo pair.
	VPIDAudio				audioCarriage;			///< @brief	Specifies how audio is carried in additional channels.
	bool					isOutput6G;				///< @brief	If true, the transport on the wire is 6G.
	bool					isOutput12G;			///< @brief	If true, the transport on the wire is 12G.
	bool					enableBT2020;			///< @brief	If true, the VPID will insert BT.2020 data.
	NTV2VPIDTransferCharacteristics	transferCharacteristics;	///< @brief Describes the transfer characteristics
	NTV2VPIDColorimetry		colorimetry;			///< @brief Describes the Colorimetry
	NTV2VPIDLuminance		luminance;				///< @brief Describes the luminance and color difference
	NTV2VPIDRGBRange		rgbRange;				///< @brief Describes the RGB range as full or SMPTE
	bool					isMultiLink;			///< @brief If true, the video stream is 12G -> 3G multi-link
} VPIDSpec;

typedef enum
{
	dualStreamFlag = 1,
	output3GFlag = 2,
	output3GbFlag = 4,
	SMPTE425Flag = 8,
	DC4KInPath = 16,
	CSCInPath = 32
} VPIDFlags;

typedef struct
{
	VPIDSpec		vpidSpec;
	ULWord			deviceNumber;
	NTV2Channel		videoChannel;
	ULWord			frameStoreIndex;
	VPIDFlags		flags;
	bool			is3G;
	bool			isDS2;
	bool			isComplete;
	ULWord			value;
	bool			isDS1;
	bool			isML1;
	bool			isML2;
	bool			isML3;
	bool			isML4;
} VPIDControl;

/**
	@brief		Generates a VPID based on the supplied specification.
	@param[out]	pOutVPID	Specifies the location where the generated VPID will be stored.
	@param[in]	pInVPIDSpec	Specifies the location of the settings describing the VPID to be generated.
	@return		True if generation was successful, otherwise false.
**/
#if defined(__cplusplus) && defined(NTV2_BUILDING_DRIVER)
extern "C"
{
#endif
AJAExport	bool	SetVPIDFromSpec (ULWord * const			pOutVPID,
									 const VPIDSpec * const	pInVPIDSpec);

#if defined(__cplusplus) && defined(NTV2_BUILDING_DRIVER)
}
#endif

#endif	// NTV2VPIDFROMSPEC_H

