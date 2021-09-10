/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2vpidfromspec.cpp
	@brief		Generates a VPID based on a specification struct. See the SMPTE 352 standard for details.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.
	@note		This file is included in driver builds. It must not contain any c++.
**/

#include "ajatypes.h"
#include "ntv2vpidfromspec.h"

#if !defined(NTV2_BUILDING_DRIVER)
	#include "ntv2utils.h"
#else
	#ifdef __cplusplus
		extern "C"
		{
	#endif
			#include "ntv2kona.h"
	#ifdef __cplusplus
		}
	#endif
#endif

bool SetVPIDFromSpec (ULWord * const			pOutVPID,
					  const VPIDSpec * const	pInVPIDSpec)
{
	NTV2VideoFormat			outputFormat	= NTV2_FORMAT_UNKNOWN;
	NTV2FrameBufferFormat	pixelFormat		= NTV2_FBF_INVALID;
	NTV2FrameRate			frameRate		= NTV2_FRAMERATE_UNKNOWN;
	NTV2VPIDTransferCharacteristics transferCharacteristics = NTV2_VPID_TC_SDR_TV;
	NTV2VPIDColorimetry		colorimetry		= NTV2_VPID_Color_Rec709;
	NTV2VPIDLuminance		luminance		= NTV2_VPID_Luminance_YCbCr;
	NTV2VPIDRGBRange		rgbRange		= NTV2_VPID_Range_Narrow;

	bool	isProgressivePicture	= false;
	bool	isProgressiveTransport	= false;
	bool	isDualLink				= false;
	bool	isLevelA				= false;
	bool	isLevelB				= false;
	bool	is3G					= false;
	bool	isRGB					= false;
	bool	isTSI					= false;
	bool	isStereo				= false;
	bool	is6G					= false;
	bool	is12G					= false;
	bool	enableBT2020			= false;
	bool	isMultiLink				= false;
	VPIDChannel vpidChannel			= VPIDChannel_1;

	uint8_t	byte1 = 0;
	uint8_t	byte2 = 0;
	uint8_t	byte3 = 0;
	uint8_t	byte4 = 0;
	
	uint8_t highBit = 0;
	uint8_t lowBit = 0;

	(void)enableBT2020;

	if (! pOutVPID || ! pInVPIDSpec)
		return false;

	outputFormat			= pInVPIDSpec->videoFormat;
	pixelFormat				= pInVPIDSpec->pixelFormat;
	isLevelA				= pInVPIDSpec->isOutputLevelA;
	isLevelB				= pInVPIDSpec->isOutputLevelB;
	is3G					= isLevelA || isLevelB;
	isDualLink				= pInVPIDSpec->isDualLink;
	isRGB					= pInVPIDSpec->isRGBOnWire;
	isTSI					= pInVPIDSpec->isTwoSampleInterleave;
	isStereo				= pInVPIDSpec->isStereo;
	is6G					= pInVPIDSpec->isOutput6G;
	is12G					= pInVPIDSpec->isOutput12G;
	vpidChannel				= pInVPIDSpec->vpidChannel;
	enableBT2020			= pInVPIDSpec->enableBT2020;
	transferCharacteristics = pInVPIDSpec->transferCharacteristics;
	colorimetry				= pInVPIDSpec->colorimetry;
	luminance				= pInVPIDSpec->luminance;
	rgbRange				= pInVPIDSpec->rgbRange;
	isMultiLink				= pInVPIDSpec->isMultiLink;


	if (! NTV2_IS_WIRE_FORMAT (outputFormat))
	{
		*pOutVPID = 0;

		return true;
	}

	if (!NTV2_IS_QUAD_QUAD_FORMAT(outputFormat) && (is6G || is12G))
		vpidChannel = VPIDChannel_1;

	frameRate				= GetNTV2FrameRateFromVideoFormat			(outputFormat);
	isProgressivePicture	= NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE (outputFormat);
	isProgressiveTransport	= isProgressivePicture;							//	Must be a progressive format to start

	if (NTV2_IS_720P_VIDEO_FORMAT(outputFormat) && !is3G)
	{
		isProgressiveTransport = false;
	}
	
	if (NTV2_IS_PSF_VIDEO_FORMAT(outputFormat))
	{
		isProgressiveTransport = false;										//	PSF is never a progressive transport
	}
	
	if (!isRGB && isDualLink &&  !isTSI)
	{
		isProgressiveTransport = false;										//	Dual link YCbCr is not a progressive transport
	}
	
	if (isTSI && NTV2_IS_4K_HFR_VIDEO_FORMAT(outputFormat) && isLevelB)
	{
		isProgressiveTransport = false;										//	Only TSI Quad Link 3.0 HFR Level B is not progressive
	}

	//
	//	Byte 1
	//

	switch (outputFormat)
	{
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_525psf_2997:
	case NTV2_FORMAT_625psf_2500:
		byte1 = is3G ? (uint8_t) VPIDStandard_483_576_3Gb : (uint8_t) VPIDStandard_483_576;	//	0x8D : 0x81
		break;

	case NTV2_FORMAT_720p_2398:
	case NTV2_FORMAT_720p_2500:
	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_720p_6000:
		if (is3G)
		{
			if (isLevelB)
				byte1 = isStereo ? (uint8_t) VPIDStandard_720_Stereo_3Gb : (uint8_t) VPIDStandard_720_3Gb;	//	0x8E : 0x8B
			else
				byte1 = isStereo ? (uint8_t) VPIDStandard_720_Stereo_3Ga : (uint8_t) VPIDStandard_720_3Ga;	//	0x91 : 0x88
		}
		else
			byte1 = (isStereo && isDualLink) ? (uint8_t) VPIDStandard_720_1080_Stereo : (uint8_t) VPIDStandard_720;		//	0xB1 : 0x84
		break;

	case NTV2_FORMAT_1080i_5000:		//	Same as NTV2_FORMAT_1080psf_2500
	case NTV2_FORMAT_1080i_5994:		//	Same as NTV2_FORMAT_1080psf_2997
	case NTV2_FORMAT_1080i_6000:		//	Same as NTV2_FORMAT_1080psf_3000
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2500_2:
	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_1080psf_2K_2398:
	case NTV2_FORMAT_1080psf_2K_2400:
	case NTV2_FORMAT_1080psf_2K_2500:
	case NTV2_FORMAT_1080p_2K_2398:
	case NTV2_FORMAT_1080p_2K_2400:
	case NTV2_FORMAT_1080p_2K_2500:
	case NTV2_FORMAT_1080p_2K_2997:
	case NTV2_FORMAT_1080p_2K_3000:
		if (is3G)
		{
			if (isLevelB)
			{
				if (isDualLink)
					byte1 = isStereo ? (uint8_t) VPIDStandard_1080_Stereo_3Gb : (uint8_t) VPIDStandard_1080_DualLink_3Gb;	//	0x8F : 0x8A
				else
					byte1 = isStereo ? (uint8_t) VPIDStandard_1080_Stereo_3Gb : (uint8_t) VPIDStandard_1080_3Gb;	//	0x8F : 0x8C
			}
			else
			{
				byte1 = isStereo ? (uint8_t) VPIDStandard_1080_Stereo_3Ga : (uint8_t) VPIDStandard_1080_3Ga;	//	0x92 : 0x89
			}
		}
		else
		{
			if (isDualLink)
			{
				byte1 = isStereo ? (uint8_t)VPIDStandard_720_1080_Stereo : (uint8_t)VPIDStandard_1080_DualLink;		//	0xB1 : 0x87
			}
			else
				byte1 = isStereo ? (uint8_t) VPIDStandard_720_1080_Stereo : (uint8_t) VPIDStandard_1080;		//	0xB1 : 0x85
		}
		break;

	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_1080p_6000_A:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_1080p_2K_4795_A:
	case NTV2_FORMAT_1080p_2K_4800_A:
	case NTV2_FORMAT_1080p_2K_5000_A:
	case NTV2_FORMAT_1080p_2K_5000_B:
	case NTV2_FORMAT_1080p_2K_5994_A:
	case NTV2_FORMAT_1080p_2K_5994_B:
	case NTV2_FORMAT_1080p_2K_6000_A:
	case NTV2_FORMAT_1080p_2K_6000_B:
		if (isRGB)
			byte1 = isLevelB ? (uint8_t) VPIDStandard_1080_Dual_3Gb : (uint8_t) VPIDStandard_1080_Dual_3Ga;		//	0x95 : 0x94
		else if (isDualLink)
		{
			byte1 = isLevelB ? (uint8_t)VPIDStandard_1080_DualLink_3Gb : (uint8_t)VPIDStandard_1080_DualLink;		//	0x8A : 0x87
		}
		else
			byte1 = isLevelB ? (uint8_t) VPIDStandard_1080_DualLink_3Gb : (uint8_t) VPIDStandard_1080_3Ga;			//	0x8A : 0x89
		break;

	case NTV2_FORMAT_4x1920x1080psf_2398:
	case NTV2_FORMAT_4x1920x1080psf_2400:
	case NTV2_FORMAT_4x1920x1080psf_2500:
	case NTV2_FORMAT_4x2048x1080psf_2398:
	case NTV2_FORMAT_4x2048x1080psf_2400:
	case NTV2_FORMAT_4x2048x1080psf_2500:
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_4x1920x1080p_2400:
	case NTV2_FORMAT_4x1920x1080p_2500:
	case NTV2_FORMAT_4x1920x1080p_2997:
	case NTV2_FORMAT_4x1920x1080p_3000:
	case NTV2_FORMAT_4x2048x1080p_2398:
	case NTV2_FORMAT_4x2048x1080p_2400:
	case NTV2_FORMAT_4x2048x1080p_2500:
	case NTV2_FORMAT_4x2048x1080p_2997:
	case NTV2_FORMAT_4x2048x1080p_3000:
		if (isTSI)
		{
			if(is12G)
				byte1 = VPIDStandard_2160_Single_12Gb; //0xCE
			else if(is6G)
				byte1 = VPIDStandard_2160_Single_6Gb; //0xC0
			else if (is3G)
			{
				if (isLevelB)
					byte1 = isDualLink? (uint8_t) VPIDStandard_2160_QuadDualLink_3Gb : (uint8_t) VPIDStandard_2160_DualLink;  //  0x98 : 0x96
				else
					byte1 = (uint8_t) VPIDStandard_2160_QuadLink_3Ga;  //  0x97
			}
			else
				byte1 = (uint8_t) VPIDStandard_2160_DualLink;  //  0x96 (bogus if not 3G)
		}
		else
		{
			if (is3G)
			{
				if (isLevelB)
					byte1 = isDualLink? (uint8_t) VPIDStandard_1080_DualLink_3Gb : (uint8_t) VPIDStandard_1080_3Gb;  //  8A : 8C
				else
					byte1 = (uint8_t) VPIDStandard_1080_3Ga;   // 89
			}
			else
			{
				byte1 = isDualLink ? (uint8_t)VPIDStandard_1080_DualLink : (uint8_t)VPIDStandard_1080;  //  0x87 : 0x85
			}
		}
		break;
    case NTV2_FORMAT_3840x2160psf_2398:
    case NTV2_FORMAT_3840x2160psf_2400:
    case NTV2_FORMAT_3840x2160psf_2500:
    case NTV2_FORMAT_3840x2160p_2398:
    case NTV2_FORMAT_3840x2160p_2400:
    case NTV2_FORMAT_3840x2160p_2500:
    case NTV2_FORMAT_3840x2160p_2997:
    case NTV2_FORMAT_3840x2160p_3000:
    case NTV2_FORMAT_3840x2160psf_2997:
    case NTV2_FORMAT_3840x2160psf_3000:
    case NTV2_FORMAT_4096x2160psf_2398:
    case NTV2_FORMAT_4096x2160psf_2400:
    case NTV2_FORMAT_4096x2160psf_2500:
    case NTV2_FORMAT_4096x2160p_2398:
    case NTV2_FORMAT_4096x2160p_2400:
    case NTV2_FORMAT_4096x2160p_2500:
    case NTV2_FORMAT_4096x2160p_2997:
    case NTV2_FORMAT_4096x2160p_3000:
    case NTV2_FORMAT_4096x2160psf_2997:
    case NTV2_FORMAT_4096x2160psf_3000:
		if(isMultiLink)
		{
			if (isLevelB)
				byte1 = isDualLink? (uint8_t) VPIDStandard_2160_QuadDualLink_3Gb : (uint8_t) VPIDStandard_2160_DualLink;  //  0x98 : 0x96
			else
				byte1 = (uint8_t) VPIDStandard_2160_QuadLink_3Ga;  //  0x97
		}
		else
		{
			byte1 = isDualLink ? (uint8_t) VPIDStandard_2160_Single_12Gb : (uint8_t) VPIDStandard_2160_Single_6Gb; //0xCE : 0xC0
		}
		break;

	case NTV2_FORMAT_4x1920x1080p_5000:
	case NTV2_FORMAT_4x1920x1080p_5994:
	case NTV2_FORMAT_4x1920x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_4795:
	case NTV2_FORMAT_4x2048x1080p_4800:
	case NTV2_FORMAT_4x2048x1080p_5000:
	case NTV2_FORMAT_4x2048x1080p_5994:
	case NTV2_FORMAT_4x2048x1080p_6000:
		if (isTSI)
		{
			if(is12G)
				byte1 = VPIDStandard_2160_Single_12Gb; // 0xCE
			else if(is6G)
				byte1 = VPIDStandard_2160_Single_6Gb; // 0xC0
			else
				byte1 = isLevelB ? (uint8_t) VPIDStandard_2160_QuadDualLink_3Gb : (uint8_t) VPIDStandard_2160_QuadLink_3Ga;	//	0x98 : 0x97
		}
		else
		{
			byte1 = isLevelB ? (uint8_t) VPIDStandard_1080_DualLink_3Gb : (uint8_t) VPIDStandard_1080_3Ga;		//	0x8A : 0x89
		}
		break;
    case NTV2_FORMAT_3840x2160p_5000:
    case NTV2_FORMAT_3840x2160p_5994:
    case NTV2_FORMAT_3840x2160p_6000:
    case NTV2_FORMAT_4096x2160p_4795:
    case NTV2_FORMAT_4096x2160p_4800:
    case NTV2_FORMAT_4096x2160p_5000:
    case NTV2_FORMAT_4096x2160p_5994:
    case NTV2_FORMAT_4096x2160p_6000:
    case NTV2_FORMAT_4096x2160p_11988:
    case NTV2_FORMAT_4096x2160p_12000:
		if(isMultiLink)
		{
			byte1 = isLevelB ? (uint8_t) VPIDStandard_2160_QuadDualLink_3Gb : (uint8_t) VPIDStandard_2160_QuadLink_3Ga;	//	0x98 : 0x97
		}
		else
		{
			byte1 = isDualLink ? (uint8_t) VPIDStandard_2160_DualLink_12Gb : (uint8_t) VPIDStandard_2160_Single_12Gb; // 0xD1 : 0xCE
		}
		break;

	case NTV2_FORMAT_4x3840x2160p_2398:
	case NTV2_FORMAT_4x3840x2160p_2400:
	case NTV2_FORMAT_4x3840x2160p_2500:
	case NTV2_FORMAT_4x3840x2160p_2997:
	case NTV2_FORMAT_4x3840x2160p_3000:
	case NTV2_FORMAT_4x4096x2160p_2398:
	case NTV2_FORMAT_4x4096x2160p_2400:
	case NTV2_FORMAT_4x4096x2160p_2500:
	case NTV2_FORMAT_4x4096x2160p_2997:
	case NTV2_FORMAT_4x4096x2160p_3000:
		byte1 = isRGB ? (uint8_t)VPIDStandard_4320_QuadLink_12Gb : (uint8_t)VPIDStandard_4320_DualLink_12Gb; // 0xD2 : 0xD0
		break;
		
	case NTV2_FORMAT_4x3840x2160p_5000:
	case NTV2_FORMAT_4x3840x2160p_5994:
	case NTV2_FORMAT_4x3840x2160p_6000:
	case NTV2_FORMAT_4x3840x2160p_5000_B:
	case NTV2_FORMAT_4x3840x2160p_5994_B:
	case NTV2_FORMAT_4x3840x2160p_6000_B:
	case NTV2_FORMAT_4x4096x2160p_4795:
	case NTV2_FORMAT_4x4096x2160p_4800:
	case NTV2_FORMAT_4x4096x2160p_5000:
	case NTV2_FORMAT_4x4096x2160p_5994:
	case NTV2_FORMAT_4x4096x2160p_6000:
	case NTV2_FORMAT_4x4096x2160p_4795_B:
	case NTV2_FORMAT_4x4096x2160p_4800_B:
	case NTV2_FORMAT_4x4096x2160p_5000_B:
	case NTV2_FORMAT_4x4096x2160p_5994_B:
	case NTV2_FORMAT_4x4096x2160p_6000_B:
		byte1 = VPIDStandard_4320_QuadLink_12Gb; // 0xD2
        break;
            
	default:
		*pOutVPID = 0;
		return true;
	}

	//
	//	Byte 2
	//

	//	Picture rate
	switch (frameRate)
	{
	case NTV2_FRAMERATE_2398:
		byte2 = VPIDPictureRate_2398;
		break;
	case NTV2_FRAMERATE_2400:
		byte2 = VPIDPictureRate_2400;
		break;
	case NTV2_FRAMERATE_2500:
		byte2 = VPIDPictureRate_2500;
		break;
	case NTV2_FRAMERATE_2997:
		byte2 = VPIDPictureRate_2997;
		break;
	case NTV2_FRAMERATE_3000:
		byte2 = VPIDPictureRate_3000;
		break;
	case NTV2_FRAMERATE_4795:
		byte2 = VPIDPictureRate_4795;
		break;
	case NTV2_FRAMERATE_4800:
		byte2 = VPIDPictureRate_4800;
		break;
	case NTV2_FRAMERATE_5000:
		byte2 = VPIDPictureRate_5000;
		break;
	case NTV2_FRAMERATE_5994:
		byte2 = VPIDPictureRate_5994;
		break;
	case NTV2_FRAMERATE_6000:
		byte2 = VPIDPictureRate_6000;
		break;
	default:
		*pOutVPID = 0;
		return true;
	}

	byte2 |= (transferCharacteristics << 4);

	//	Progressive picture
	byte2 |= isProgressivePicture ? (1UL << 6) : 0;	//	0x40

	//	Progressive transport
	byte2 |= isProgressiveTransport ? (1UL << 7) : 0;	//	0x80

	//
	//	Byte 3
	//

	//	Horizontal pixel count
	if (isStereo)
	{
	}
	else
	{
		byte3 |= NTV2_IS_2K_1080_VIDEO_FORMAT (outputFormat) ? (1UL << 6) : 0;	//	0x40
		byte3 |= NTV2_IS_4K_4096_VIDEO_FORMAT (outputFormat) ? (1UL << 6) : 0;	//	0x40
        byte3 |= NTV2_IS_UHD2_FULL_VIDEO_FORMAT (outputFormat) ? (1UL << 6) : 0;    //    0x40
	}

	//	Aspect ratio
	if ( NTV2_IS_HD_VIDEO_FORMAT		(outputFormat) &&
		 ! NTV2_IS_720P_VIDEO_FORMAT	(outputFormat) &&
		 ! NTV2_IS_2K_1080_VIDEO_FORMAT	(outputFormat))
	{
		if (isLevelA)
			byte3 |= (1UL << 7);			//	0x80
		else
			byte3 |= (1UL << 5);			//	0x20
	}

	if ( NTV2_IS_4K_VIDEO_FORMAT (outputFormat) &&
		 ! NTV2_IS_4K_4096_VIDEO_FORMAT (outputFormat))
	{
		if(is6G || is12G || isLevelA)
		{
			byte3 |= (1UL << 7);
		}
		else// if((!is3G) || isLevelB)
		{
			byte3 |= (1UL << 5);			//	0x20
		}
	}
    
    if ( NTV2_IS_UHD2_VIDEO_FORMAT (outputFormat))
    {
        if (isLevelB && isDualLink)
            byte3 |= (1UL << 5);            //    0x20
        else
            byte3 |= (1UL << 7);            //    0x80
    }
	
	//Colorimetry
	highBit = (colorimetry&0x2)>>1;
	lowBit = colorimetry&0x1;
	if ( NTV2_IS_HD_VIDEO_FORMAT		(outputFormat) &&
		 ! NTV2_IS_720P_VIDEO_FORMAT	(outputFormat))
	{
		if (isLevelA)
			byte3 |= (highBit << 5);
		else
			byte3 |= (highBit << 7);
		
		byte3 |= (lowBit << 4);
	}
	
	if ( NTV2_IS_4K_VIDEO_FORMAT (outputFormat) || NTV2_IS_QUAD_QUAD_FORMAT(outputFormat))
	{
		if(is6G || is12G || isLevelA)
		{
			byte3 |= (highBit << 5);
			byte3 |= (lowBit << 4);
		}
		else// if ((!is3G) || isLevelB)
		{
			byte3 |= (highBit << 7);
			byte3 |= (lowBit << 4);
		}
	}

	//	Sampling structure
	if (pixelFormat == NTV2_FBF_INVALID)
	{
		//	Pixel format not specified, make a guess
		if (pInVPIDSpec->isRGBOnWire)
			pixelFormat = NTV2_FBF_10BIT_DPX;	//	Most people use this if not 48 bit
		else
			pixelFormat = NTV2_FBF_10BIT_YCBCR;
	}

	if (isRGB)
	{
		switch (pixelFormat)
		{
		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_ABGR:
			byte3 |= VPIDSampling_GBRA_4444;
			break;

		case NTV2_FBF_10BIT_DPX:
		case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_48BIT_RGB:
		case NTV2_FBF_12BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_RGB_PACKED:
			byte3 |= VPIDSampling_GBR_444;
			break;

		//	Although RGB is on the wire, the pixel format can be YCbCr if
		//	the signal is routed through a CSC before going to a Dual Link.
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:
		case NTV2_FBF_10BIT_YCBCR_DPX:
			byte3 |= VPIDSampling_GBR_444;
			break;

		default:
			*pOutVPID = 0;
			return true;
		}
	}
	else
	{
		switch (pixelFormat)
		{
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:
		case NTV2_FBF_10BIT_YCBCR_DPX:
			byte3 |= VPIDSampling_YUV_422;
			break;

		default:
			*pOutVPID = 0;
			return true;
		}
	}

	//
	//	Byte 4
	//

	//	VPID channel
	if (pInVPIDSpec->isTwoSampleInterleave)
	{
		if ((isLevelB && NTV2_IS_4K_HFR_VIDEO_FORMAT (outputFormat)) || isRGB)
			byte4 |= vpidChannel << 5;
		else
			byte4 |= vpidChannel << 6;
	}
	else
	{
		if (pInVPIDSpec->useChannel)
			byte4 |= vpidChannel << 6;	
	}

	//	Audio
	if (isStereo)
	{
		byte4 |= pInVPIDSpec->isRightEye	<< 6;		//	0x40
		byte4 |= pInVPIDSpec->audioCarriage	<< 2;		//	0x0C
	}

	//Luminance and color difference signal
	if (NTV2_IS_QUAD_QUAD_FORMAT(outputFormat) ||
		NTV2_IS_4K_VIDEO_FORMAT(outputFormat) ||
		NTV2_IS_HD_VIDEO_FORMAT(outputFormat))
	{
		byte4 |= (luminance << 4);
	}

	if (NTV2_IS_QUAD_FRAME_FORMAT(outputFormat) &&
		NTV2_IS_SQUARE_DIVISION_FORMAT(outputFormat) &&
		pInVPIDSpec->isTwoSampleInterleave)
	{
		byte4 |= VPIDAudio_Copied << 2;
	}

	//	Bit depth
	if(NTV2_IS_VALID_FBF(pixelFormat))
	{
		bool is12Bit = (pixelFormat == NTV2_FBF_48BIT_RGB || pixelFormat == NTV2_FBF_12BIT_RGB_PACKED) ? true : false;
		byte4 |= is12Bit ? (rgbRange == NTV2_VPID_Range_Narrow ? VPIDBitDepth_12 : VPIDBitDepth_12_Full) : (rgbRange == NTV2_VPID_Range_Narrow ? VPIDBitDepth_10 : VPIDBitDepth_10_Full);
	}
	else
	{
		*pOutVPID = 0;
		return true;
	}

	//	Return VPID value to caller
	*pOutVPID = ((ULWord)byte1 << 24) | ((ULWord)byte2 << 16) | ((ULWord)byte3 << 8) | byte4;

	return true;
}

