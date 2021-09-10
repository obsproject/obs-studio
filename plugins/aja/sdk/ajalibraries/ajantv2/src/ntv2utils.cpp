/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2utils.cpp
	@brief		Implementations for the NTV2 utility functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/
#include "ajatypes.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2registerexpert.h"
#include "ntv2videodefines.h"
#include "ntv2audiodefines.h"
#include "ntv2endian.h"
#include "ntv2debug.h"
#include "ntv2transcode.h"
#include "ntv2devicefeatures.h"
#include "ajabase/system/lock.h"
#if defined(AJALinux)
	#include <string.h>  // For memset
	#include <stdint.h>

#endif
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <map>


using namespace std;

#if defined (NTV2_DEPRECATE)
	#define	AJA_LOCAL_STATIC	static
#else	//	!defined (NTV2_DEPRECATE)
	#define	AJA_LOCAL_STATIC
#endif	//	!defined (NTV2_DEPRECATE)

// Macros to simplify returning of strings for given enum
#define NTV2UTILS_ENUM_CASE_RETURN_STR(enum_name) case(enum_name): return #enum_name

#define NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(condition, retail_name, enum_name)\
	case(enum_name): return condition ? retail_name : #enum_name

//////////////////////////////////////////////////////
//	BEGIN SECTION MOVED FROM 'videoutilities.cpp'
//////////////////////////////////////////////////////

uint32_t CalcRowBytesForFormat (const NTV2FrameBufferFormat inPixelFormat, const uint32_t inPixelWidth)
{
	uint32_t rowBytes = 0;

	switch (inPixelFormat)
	{
	case NTV2_FBF_8BIT_YCBCR:
	case NTV2_FBF_8BIT_YCBCR_YUY2:	
		rowBytes = inPixelWidth * 2;
		break;

	case NTV2_FBF_10BIT_YCBCR:	
	case NTV2_FBF_10BIT_YCBCR_DPX:
		rowBytes = (( inPixelWidth % 48 == 0 ) ? inPixelWidth : (((inPixelWidth / 48 ) + 1) * 48)) * 8 / 3;
		break;
	
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_10BIT_DPX:
    case NTV2_FBF_10BIT_DPX_LE:
	case NTV2_FBF_10BIT_RGB_PACKED:
	case NTV2_FBF_ARGB:	
	case NTV2_FBF_RGBA:
	case NTV2_FBF_ABGR:
		rowBytes = inPixelWidth * 4;
		break;

	case NTV2_FBF_24BIT_RGB:
	case NTV2_FBF_24BIT_BGR:
		rowBytes = inPixelWidth * 3;
		break;

 	case NTV2_FBF_8BIT_DVCPRO:
 		rowBytes = inPixelWidth * 2/4;
		break;

	case NTV2_FBF_48BIT_RGB:
		rowBytes = inPixelWidth * 6;
		break;
		
	case NTV2_FBF_12BIT_RGB_PACKED:
		rowBytes = inPixelWidth * 36 / 8;
		break;

	case NTV2_FBF_10BIT_YCBCR_420PL2:
	case NTV2_FBF_10BIT_YCBCR_422PL2:
		rowBytes = inPixelWidth * 20 / 16;
		break;

	case NTV2_FBF_8BIT_YCBCR_420PL2:
	case NTV2_FBF_8BIT_YCBCR_422PL2:
		rowBytes = inPixelWidth;
		break;
		
	case NTV2_FBF_8BIT_YCBCR_420PL3:
	case NTV2_FBF_8BIT_HDV:
	case NTV2_FBF_10BIT_YCBCRA:
	case NTV2_FBF_PRORES_DVCPRO:
	case NTV2_FBF_PRORES_HDV:
	case NTV2_FBF_10BIT_ARGB:
	case NTV2_FBF_16BIT_ARGB:
	case NTV2_FBF_8BIT_YCBCR_422PL3:
	case NTV2_FBF_10BIT_RAW_RGB:
	case NTV2_FBF_10BIT_RAW_YCBCR:
	case NTV2_FBF_NUMFRAMEBUFFERFORMATS:
	case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
	case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
		//	TO DO.....add more
		break;
	}

	return rowBytes;
}


bool UnpackLine_10BitYUVtoUWordSequence (const void * pIn10BitYUVLine, UWordSequence & out16BitYUVLine, ULWord inNumPixels)
{
	out16BitYUVLine.clear ();
	const ULWord *	pInputLine	(reinterpret_cast <const ULWord *> (pIn10BitYUVLine));

	if (!pInputLine)
		return false;	//	bad pointer
	if (inNumPixels < 6)
		return false;	//	bad width
	if (inNumPixels % 6)
		inNumPixels -= inNumPixels % 6;

	const ULWord	totalULWords	(inNumPixels * 4 / 6);	//	4 ULWords per 6 pixels

	for (ULWord inputCount (0);  inputCount < totalULWords;  inputCount++)
	{
		out16BitYUVLine.push_back ((pInputLine [inputCount]      ) & 0x3FF);
		out16BitYUVLine.push_back ((pInputLine [inputCount] >> 10) & 0x3FF);
		out16BitYUVLine.push_back ((pInputLine [inputCount] >> 20) & 0x3FF);
	}
	return true;
}


bool UnpackLine_10BitYUVtoUWordSequence (const void * pIn10BitYUVLine, const NTV2FormatDescriptor & inFormatDesc, UWordSequence & out16BitYUVLine)
{
	out16BitYUVLine.clear ();
	const ULWord *	pInputLine	(reinterpret_cast <const ULWord *> (pIn10BitYUVLine));

	if (!pInputLine)
		return false;	//	bad pointer
	if (!inFormatDesc.IsValid ())
		return false;	//	bad formatDesc
	if (inFormatDesc.GetRasterWidth () < 6)
		return false;	//	bad width
	if (inFormatDesc.GetPixelFormat() != NTV2_FBF_10BIT_YCBCR)
		return false;	//	wrong FBF

	for (ULWord inputCount (0);  inputCount < inFormatDesc.linePitch;  inputCount++)
	{
		out16BitYUVLine.push_back ((pInputLine [inputCount]      ) & 0x3FF);
		out16BitYUVLine.push_back ((pInputLine [inputCount] >> 10) & 0x3FF);
		out16BitYUVLine.push_back ((pInputLine [inputCount] >> 20) & 0x3FF);
	}
	return true;
}


// UnPack10BitYCbCrBuffer
// UnPack 10 Bit YCbCr Data to 16 bit Word per component
void UnPack10BitYCbCrBuffer( uint32_t* packedBuffer, uint16_t* ycbcrBuffer, uint32_t numPixels )
{
	for (  uint32_t sampleCount = 0, dataCount = 0; 
		sampleCount < (numPixels*2) ; 
		sampleCount+=3,dataCount++ )
	{
		ycbcrBuffer[sampleCount]   =  packedBuffer[dataCount]&0x3FF;  
		ycbcrBuffer[sampleCount+1] = (packedBuffer[dataCount]>>10)&0x3FF;  
		ycbcrBuffer[sampleCount+2] = (packedBuffer[dataCount]>>20)&0x3FF;  

	}
}

// PackTo10BitYCbCrBuffer
// Pack 16 bit Word per component to 10 Bit YCbCr Data 
void PackTo10BitYCbCrBuffer (const uint16_t * ycbcrBuffer, uint32_t * packedBuffer, const uint32_t numPixels)
{
	for ( uint32_t inputCount=0, outputCount=0; 
		inputCount < (numPixels*2);
		outputCount += 4, inputCount += 12 )
	{
		packedBuffer[outputCount]   = uint32_t (ycbcrBuffer[inputCount+0]) + uint32_t (ycbcrBuffer[inputCount+1]<<10) + uint32_t (ycbcrBuffer[inputCount+2]<<20);
		packedBuffer[outputCount+1] = uint32_t (ycbcrBuffer[inputCount+3]) + uint32_t (ycbcrBuffer[inputCount+4]<<10) + uint32_t (ycbcrBuffer[inputCount+5]<<20);
		packedBuffer[outputCount+2] = uint32_t (ycbcrBuffer[inputCount+6]) + uint32_t (ycbcrBuffer[inputCount+7]<<10) + uint32_t (ycbcrBuffer[inputCount+8]<<20);
		packedBuffer[outputCount+3] = uint32_t (ycbcrBuffer[inputCount+9]) + uint32_t (ycbcrBuffer[inputCount+10]<<10) + uint32_t (ycbcrBuffer[inputCount+11]<<20);
	}
}

void MakeUnPacked10BitYCbCrBuffer( uint16_t* buffer, uint16_t Y , uint16_t Cb , uint16_t Cr,uint32_t numPixels )
{
	// assumes lineData is large enough for numPixels
	for ( uint32_t count = 0; count < numPixels*2; count+=4 )
	{
		buffer[count] = Cb;
		buffer[count+1] = Y;
		buffer[count+2] = Cr;
		buffer[count+3] = Y;
	}	
}


// ConvertLineTo8BitYCbCr
// 10 Bit YCbCr to 8 Bit YCbCr
void ConvertLineTo8BitYCbCr (const uint16_t * ycbcr10BitBuffer, uint8_t * ycbcr8BitBuffer,	const uint32_t numPixels)
{
	for (uint32_t pixel(0);  pixel < numPixels * 2;  pixel++)
		ycbcr8BitBuffer[pixel] = uint8_t(ycbcr10BitBuffer[pixel] >> 2);
}

//***********************************************************************************************************

// ConvertUnpacked10BitYCbCrToPixelFormat()
//		Converts a line of "unpacked" 10-bit Y/Cb/Cr pixels into a "packed" line in the pixel format
//	for the current frame buffer format.
void ConvertUnpacked10BitYCbCrToPixelFormat(uint16_t *unPackedBuffer, uint32_t *packedBuffer, uint32_t numPixels, NTV2FrameBufferFormat pixelFormat,
											bool bUseSmpteRange, bool bAlphaFromLuma)
{
	bool  bIsSD = false;
	if(numPixels < 1280)
		bIsSD = true;

	switch(pixelFormat) 
	{
		case NTV2_FBF_10BIT_YCBCR:
			PackTo10BitYCbCrBuffer(unPackedBuffer, packedBuffer, numPixels);
			break;

		case NTV2_FBF_8BIT_YCBCR:
			ConvertLineTo8BitYCbCr(unPackedBuffer, reinterpret_cast<uint8_t*>(packedBuffer), numPixels);
			break;
		
		case NTV2_FBF_ARGB:
			ConvertLinetoRGB(unPackedBuffer, reinterpret_cast<RGBAlphaPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange, bAlphaFromLuma);
			break;
		
		case NTV2_FBF_RGBA:
			ConvertLinetoRGB(unPackedBuffer, reinterpret_cast<RGBAlphaPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange, bAlphaFromLuma);
			ConvertARGBYCbCrToRGBA(reinterpret_cast<UByte*>(packedBuffer), numPixels);
			break;
			
		case NTV2_FBF_10BIT_RGB:
			ConvertLineto10BitRGB(unPackedBuffer, reinterpret_cast<RGBAlpha10BitPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange);
			PackRGB10BitFor10BitRGB(reinterpret_cast<RGBAlpha10BitPixel*>(packedBuffer), numPixels);
			break;

		case NTV2_FBF_8BIT_YCBCR_YUY2:
			ConvertLineTo8BitYCbCr(unPackedBuffer, reinterpret_cast<uint8_t*>(packedBuffer), numPixels);
			Convert8BitYCbCrToYUY2(reinterpret_cast<uint8_t*>(packedBuffer), numPixels);
			break;
			
		case NTV2_FBF_ABGR:
			ConvertLinetoRGB(unPackedBuffer, reinterpret_cast<RGBAlphaPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange, bAlphaFromLuma);
			ConvertARGBYCbCrToABGR(reinterpret_cast<uint8_t*>(packedBuffer), numPixels);
			break;
			
		case NTV2_FBF_10BIT_DPX:
			ConvertLineto10BitRGB(unPackedBuffer, reinterpret_cast<RGBAlpha10BitPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange);
			PackRGB10BitFor10BitDPX(reinterpret_cast<RGBAlpha10BitPixel*>(packedBuffer), numPixels);
			break;

		case NTV2_FBF_10BIT_YCBCR_DPX:
			RePackLineDataForYCbCrDPX(packedBuffer, CalcRowBytesForFormat(NTV2_FBF_10BIT_YCBCR_DPX, numPixels));
			break;

		case NTV2_FBF_24BIT_RGB:
			ConvertLinetoRGB(unPackedBuffer,reinterpret_cast<RGBAlphaPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange);
			ConvertARGBToRGB(reinterpret_cast<UByte*>(packedBuffer), reinterpret_cast<UByte*>(packedBuffer), numPixels);
			break;

		case NTV2_FBF_24BIT_BGR:
			ConvertLinetoRGB(unPackedBuffer,reinterpret_cast<RGBAlphaPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange);
			ConvertARGBToBGR(reinterpret_cast<UByte*>(packedBuffer), reinterpret_cast<UByte*>(packedBuffer), numPixels);
			break;

        case NTV2_FBF_10BIT_DPX_LE:
			ConvertLineto10BitRGB(unPackedBuffer, reinterpret_cast<RGBAlpha10BitPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange);
			PackRGB10BitFor10BitDPX(reinterpret_cast<RGBAlpha10BitPixel*>(packedBuffer), numPixels, false);
			break;

		case NTV2_FBF_48BIT_RGB:
			ConvertLineto16BitRGB(unPackedBuffer, reinterpret_cast<RGBAlpha16BitPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange);
			Convert16BitARGBTo16BitRGB(reinterpret_cast<RGBAlpha16BitPixel*>(packedBuffer), reinterpret_cast<UWord*>(packedBuffer), numPixels);
			break;

		case NTV2_FBF_10BIT_RGB_PACKED:
			ConvertLineto10BitRGB(unPackedBuffer, reinterpret_cast<RGBAlpha10BitPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange);
			PackRGB10BitFor10BitRGBPacked(reinterpret_cast<RGBAlpha10BitPixel*>(packedBuffer), numPixels);
			break;
			
		case NTV2_FBF_12BIT_RGB_PACKED:
			ConvertLineto16BitRGB(unPackedBuffer, reinterpret_cast<RGBAlpha16BitPixel*>(packedBuffer), numPixels, bIsSD, bUseSmpteRange);
			Convert16BitARGBTo12BitRGBPacked(reinterpret_cast<RGBAlpha16BitPixel*>(packedBuffer), reinterpret_cast<UByte*>(packedBuffer), numPixels);
			break;
	#if defined(_DEBUG)
		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_YCBCR_420PL3:
		case NTV2_FBF_8BIT_HDV:
		case NTV2_FBF_10BIT_YCBCRA:
		case NTV2_FBF_PRORES_DVCPRO:
		case NTV2_FBF_PRORES_HDV:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
		case NTV2_FBF_8BIT_YCBCR_422PL3:
		case NTV2_FBF_10BIT_RAW_RGB:
		case NTV2_FBF_10BIT_RAW_YCBCR:
		case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_420PL2:
		case NTV2_FBF_10BIT_YCBCR_422PL2:
		case NTV2_FBF_8BIT_YCBCR_420PL2:
		case NTV2_FBF_8BIT_YCBCR_422PL2:
		case NTV2_FBF_LAST:
			break;
	#else
		default:	break;
	#endif
	}
}

// MaskUnPacked10BitYCbCrBuffer
// Mask Data In place based on signalMask
void MaskUnPacked10BitYCbCrBuffer( uint16_t* ycbcrUnPackedBuffer, uint16_t signalMask , uint32_t numPixels )
{
	uint32_t pixelCount;

	// Not elegant but fairly fast.
	switch ( signalMask )
	{
	case NTV2_SIGNALMASK_NONE:          // Output Black
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}
		break;
	case NTV2_SIGNALMASK_Y:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break;
	case NTV2_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}

		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break; 

	case NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
		}


		break; 
	case NTV2_SIGNALMASK_Cb + NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break; 
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cb + NTV2_SIGNALMASK_Cr:
		// Do nothing
		break; 
	}

}



//--------------------------------------------------------------------------------------------------------------------
//	StackQuadrants()
//
//	Take a 4K source, cut it into 4 quandrants and stack it into the destination. Also handle cases where
//	where source/destination rowBytes/widths are mismatched (eg 4096 -> 3840)
//--------------------------------------------------------------------------------------------------------------------
void StackQuadrants(uint8_t* pSrc, uint32_t srcWidth, uint32_t srcHeight, uint32_t srcRowBytes, 
					 uint8_t* pDst)
{
	(void) srcWidth;
	uint32_t dstSample;
	uint32_t srcSample;
	uint32_t copyRowBytes = srcRowBytes/2;
	uint32_t copyHeight = srcHeight/2;
	uint32_t dstRowBytes = copyRowBytes;
	uint32_t dstHeight = srcHeight/2;
	//uint32_t dstWidth = srcWidth/2;

	// rowbytes for left hand side quadrant
	uint32_t srcLHSQuadrantRowBytes = srcRowBytes/2;

	for (uint32_t quadrant=0; quadrant<4; quadrant++)
	{
		// starting point for source quadrant
		switch (quadrant)
		{
		default:
		case 0: srcSample = 0; break;													// quadrant 0, upper left
		case 1: srcSample = srcLHSQuadrantRowBytes; break;								// quadrant 1, upper right
		case 2: srcSample = (srcRowBytes*copyHeight); break;							// quadrant 2, lower left
		case 3: srcSample = (srcRowBytes*copyHeight) + srcLHSQuadrantRowBytes; break;	// quadrant 3, lower right
		}

		// starting point for destination stack
		dstSample = quadrant * dstRowBytes * dstHeight;

		for (uint32_t row=0; row<copyHeight; row++)
		{
			memcpy(&pDst[dstSample], &pSrc[srcSample], copyRowBytes);
			dstSample += dstRowBytes;
			srcSample += srcRowBytes;
		}
	}
}

// Copy a quater-sized quadrant from a source buffer to a destination buffer
// quad13Offset is almost always zero, but can be used for Quadrants 1, 3 for special offset frame buffers. (e.g. 4096x1080 10Bit YCbCr frame buffers)
void CopyFromQuadrant(uint8_t* srcBuffer, uint32_t srcHeight, uint32_t srcRowBytes, uint32_t srcQuadrant, uint8_t* dstBuffer, uint32_t quad13Offset)
{
	ULWord dstSample = 0;
	ULWord srcSample = 0;
	ULWord dstHeight = srcHeight / 2;
	ULWord dstRowBytes = srcRowBytes / 2;

	// calculate starting point for source of copy, based on source quadrant
	switch (srcQuadrant)
	{
	default:
	case 0: srcSample = 0; break;													// quadrant 0, upper left
	case 1: srcSample = dstRowBytes - quad13Offset; break;							// quadrant 1, upper right
	case 2: srcSample = srcRowBytes*dstHeight; break;								// quadrant 2, lower left
	case 3: srcSample = srcRowBytes*dstHeight + dstRowBytes - quad13Offset; break;	// quadrant 3, lower right
	}

	// for each row
	for (ULWord i=0; i<dstHeight; i++)
	{
		memcpy(&dstBuffer[dstSample], &srcBuffer[srcSample], dstRowBytes);
		dstSample += dstRowBytes;
		srcSample += srcRowBytes;
	}
}

// Copy a source buffer to a quadrant of a 4x-sized destination buffer
// quad13Offset is almost always zero, but can be used for Quadrants 1, 3 for special offset frame buffers. (e.g. 4096x1080 10Bit YCbCr frame buffers)
void CopyToQuadrant(uint8_t* srcBuffer, uint32_t srcHeight, uint32_t srcRowBytes, uint32_t dstQuadrant, uint8_t* dstBuffer, uint32_t quad13Offset)
{
	ULWord dstSample = 0;
	ULWord srcSample = 0;
	ULWord dstRowBytes = srcRowBytes * 2;

	// calculate starting point for destination of copy, based on destination quadrant
	switch (dstQuadrant)
	{
	default:
	case 0: dstSample = 0; break;													// quadrant 0, upper left
	case 1: dstSample = srcRowBytes - quad13Offset; break;							// quadrant 1, upper right
	case 2: dstSample = dstRowBytes*srcHeight; break;								// quadrant 2, lower left
	case 3: dstSample = dstRowBytes*srcHeight + srcRowBytes - quad13Offset; break;	// quadrant 3, lower right
	}

	// for each row
	for (ULWord i=0; i<srcHeight; i++)
	{
		memcpy(&dstBuffer[dstSample], &srcBuffer[srcSample], srcRowBytes);
		dstSample += dstRowBytes;
		srcSample += srcRowBytes;
	}
}
//////////////////////////////////////////////////////
//	END SECTION MOVED FROM 'videoutilities.cpp'
//////////////////////////////////////////////////////


void UnpackLine_10BitYUVto16BitYUV (const ULWord * pIn10BitYUVLine, UWord * pOut16BitYUVLine, const ULWord inNumPixels)
#if !defined (NTV2_DEPRECATE)
{
	::UnPackLineData (pIn10BitYUVLine, pOut16BitYUVLine, inNumPixels);
}

void UnPackLineData (const ULWord * pIn10BitYUVLine, UWord * pOut16BitYUVLine, const ULWord inNumPixels)
#endif	//	!defined (NTV2_DEPRECATE)
{
	NTV2_ASSERT (pIn10BitYUVLine && pOut16BitYUVLine && "UnpackLine_10BitYUVto16BitYUV -- NULL buffer pointer(s)");
	NTV2_ASSERT (inNumPixels && "UnpackLine_10BitYUVto16BitYUV -- Zero pixel count");

	for (ULWord outputCount = 0,  inputCount = 0;
		 outputCount < (inNumPixels * 2);
		 outputCount += 3,  inputCount++)
	{
		pOut16BitYUVLine [outputCount    ] =  pIn10BitYUVLine [inputCount]        & 0x3FF;
		pOut16BitYUVLine [outputCount + 1] = (pIn10BitYUVLine [inputCount] >> 10) & 0x3FF;
		pOut16BitYUVLine [outputCount + 2] = (pIn10BitYUVLine [inputCount] >> 20) & 0x3FF;
	}
}


void PackLine_16BitYUVto10BitYUV (const UWord * pIn16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels)
#if !defined (NTV2_DEPRECATE)
{
	::PackLineData (pIn16BitYUVLine, pOut10BitYUVLine, inNumPixels);
}

void PackLineData (const UWord * pIn16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels)
#endif	//	!defined (NTV2_DEPRECATE)
{
	NTV2_ASSERT (pIn16BitYUVLine && pOut10BitYUVLine && "PackLine_16BitYUVto10BitYUV -- NULL buffer pointer(s)");
	NTV2_ASSERT (inNumPixels && "PackLine_16BitYUVto10BitYUV -- Zero pixel count");

	for (ULWord inputCount = 0,  outputCount = 0;
		  inputCount < (inNumPixels * 2);
		  outputCount += 4,  inputCount += 12)
	{
		pOut10BitYUVLine [outputCount    ] = ULWord (pIn16BitYUVLine [inputCount + 0]) + (ULWord (pIn16BitYUVLine [inputCount + 1]) << 10) + (ULWord (pIn16BitYUVLine [inputCount + 2]) << 20);
		pOut10BitYUVLine [outputCount + 1] = ULWord (pIn16BitYUVLine [inputCount + 3]) + (ULWord (pIn16BitYUVLine [inputCount + 4]) << 10) + (ULWord (pIn16BitYUVLine [inputCount + 5]) << 20);
		pOut10BitYUVLine [outputCount + 2] = ULWord (pIn16BitYUVLine [inputCount + 6]) + (ULWord (pIn16BitYUVLine [inputCount + 7]) << 10) + (ULWord (pIn16BitYUVLine [inputCount + 8]) << 20);
		pOut10BitYUVLine [outputCount + 3] = ULWord (pIn16BitYUVLine [inputCount + 9]) + (ULWord (pIn16BitYUVLine [inputCount +10]) << 10) + (ULWord (pIn16BitYUVLine [inputCount +11]) << 20);
	}	//	for each component in the line
}


bool PackLine_UWordSequenceTo10BitYUV (const UWordSequence & in16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels)
{
	if (!pOut10BitYUVLine)
		return false;	//	NULL buffer pointer
	if (!inNumPixels)
		return false;	//	Zero pixel count
	if (ULWord(in16BitYUVLine.size()) < inNumPixels*2)
		return false;	//	UWordSequence too small

	for (ULWord inputCount = 0,  outputCount = 0;
		  inputCount < (inNumPixels * 2);
		  outputCount += 4,  inputCount += 12)
	{
		pOut10BitYUVLine[outputCount    ] = ULWord(in16BitYUVLine[inputCount + 0]) + (ULWord(in16BitYUVLine[inputCount + 1]) << 10) + (ULWord(in16BitYUVLine[inputCount + 2]) << 20);
		pOut10BitYUVLine[outputCount + 1] = ULWord(in16BitYUVLine[inputCount + 3]) + (ULWord(in16BitYUVLine[inputCount + 4]) << 10) + (ULWord(in16BitYUVLine[inputCount + 5]) << 20);
		pOut10BitYUVLine[outputCount + 2] = ULWord(in16BitYUVLine[inputCount + 6]) + (ULWord(in16BitYUVLine[inputCount + 7]) << 10) + (ULWord(in16BitYUVLine[inputCount + 8]) << 20);
		pOut10BitYUVLine[outputCount + 3] = ULWord(in16BitYUVLine[inputCount + 9]) + (ULWord(in16BitYUVLine[inputCount +10]) << 10) + (ULWord(in16BitYUVLine[inputCount +11]) << 20);
	}	//	for each component in the line
	return true;
}


bool YUVComponentsTo10BitYUVPackedBuffer (const vector<uint16_t> & inYCbCrLine,  NTV2_POINTER & inFrameBuffer,
											const NTV2FormatDescriptor & inDescriptor,  const UWord inLineOffset)
{
	if (inYCbCrLine.size() < 12)
		return false;	//	Input vector needs at least 12 components
	if (inFrameBuffer.IsNULL())
		return false;	//	NULL frame buffer
	if (!inDescriptor.IsValid())
		return false;	//	Bad format descriptor
	if (ULWord(inLineOffset) >= inDescriptor.GetFullRasterHeight())
		return false;	//	Illegal line offset
	if (inDescriptor.GetPixelFormat() != NTV2_FBF_10BIT_YCBCR)
		return false;	//	Not 'v210' pixel format

	const uint32_t	pixPerLineX2	(inDescriptor.GetRasterWidth() * 2);
	uint32_t *		pOutPackedLine	(AJA_NULL);
	if (inFrameBuffer.GetByteCount() < inDescriptor.GetBytesPerRow() * ULWord(inLineOffset+1))
		return false;	//	Buffer too small

	pOutPackedLine = reinterpret_cast<uint32_t*>(inDescriptor.GetWriteableRowAddress(inFrameBuffer.GetHostAddress(0), inLineOffset));
	if (!pOutPackedLine)
		return false;	//	Buffer too small

	for (uint32_t inputCount = 0, outputCount = 0;   inputCount < pixPerLineX2;   outputCount += 4, inputCount += 12)
	{
		if ((inputCount+11) >= uint32_t(inYCbCrLine.size()))
			break;	//	Early exit (not fatal)
	#if defined(_DEBUG)	//	'at' throws upon bad index values
		pOutPackedLine[outputCount]   = uint32_t(inYCbCrLine.at(inputCount+0)) | uint32_t(inYCbCrLine.at(inputCount+ 1)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 2)<<20);
		pOutPackedLine[outputCount+1] = uint32_t(inYCbCrLine.at(inputCount+3)) | uint32_t(inYCbCrLine.at(inputCount+ 4)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 5)<<20);
		pOutPackedLine[outputCount+2] = uint32_t(inYCbCrLine.at(inputCount+6)) | uint32_t(inYCbCrLine.at(inputCount+ 7)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 8)<<20);
		pOutPackedLine[outputCount+3] = uint32_t(inYCbCrLine.at(inputCount+9)) | uint32_t(inYCbCrLine.at(inputCount+10)<<10) | uint32_t(inYCbCrLine.at(inputCount+11)<<20);
	#else				//	'operator[]' doesn't throw
		pOutPackedLine[outputCount]   = uint32_t(inYCbCrLine[inputCount+0]) | uint32_t(inYCbCrLine[inputCount+ 1]<<10) | uint32_t(inYCbCrLine[inputCount+ 2]<<20);
		pOutPackedLine[outputCount+1] = uint32_t(inYCbCrLine[inputCount+3]) | uint32_t(inYCbCrLine[inputCount+ 4]<<10) | uint32_t(inYCbCrLine[inputCount+ 5]<<20);
		pOutPackedLine[outputCount+2] = uint32_t(inYCbCrLine[inputCount+6]) | uint32_t(inYCbCrLine[inputCount+ 7]<<10) | uint32_t(inYCbCrLine[inputCount+ 8]<<20);
		pOutPackedLine[outputCount+3] = uint32_t(inYCbCrLine[inputCount+9]) | uint32_t(inYCbCrLine[inputCount+10]<<10) | uint32_t(inYCbCrLine[inputCount+11]<<20);
	#endif
	}
	return true;
}


bool UnpackLine_10BitYUVtoU16s (vector<uint16_t> & outYCbCrLine, const NTV2_POINTER & inFrameBuffer,
								const NTV2FormatDescriptor & inDescriptor, const UWord inLineOffset)
{
	outYCbCrLine.clear();
	if (inFrameBuffer.IsNULL())
		return false;	//	NULL frame buffer
	if (!inDescriptor.IsValid())
		return false;	//	Bad format descriptor
	if (ULWord(inLineOffset) >= inDescriptor.GetFullRasterHeight())
		return false;	//	Illegal line offset
	if (inDescriptor.GetPixelFormat() != NTV2_FBF_10BIT_YCBCR)
		return false;	//	Not 'v210' pixel format
	if (inDescriptor.GetRasterWidth () < 6)
		return false;	//	bad width

	const ULWord *	pInputLine	(reinterpret_cast<const ULWord*>(inDescriptor.GetRowAddress(inFrameBuffer.GetHostPointer(), inLineOffset)));

	for (ULWord inputCount(0);  inputCount < inDescriptor.linePitch;  inputCount++)
	{
		outYCbCrLine.push_back((pInputLine[inputCount]      ) & 0x3FF);
		outYCbCrLine.push_back((pInputLine[inputCount] >> 10) & 0x3FF);
		outYCbCrLine.push_back((pInputLine[inputCount] >> 20) & 0x3FF);
	}
	return true;
}


// RePackLineDataForYCbCrDPX
void RePackLineDataForYCbCrDPX(ULWord *packedycbcrLine, ULWord numULWords )
{
	for ( UWord count = 0; count < numULWords; count++)
	{
		ULWord value = (packedycbcrLine[count])<<2;
		value = (value<<24) + ((value>>24)&0x000000FF) + ((value<<8)&0x00FF0000) + ((value>>8)&0x0000FF00);

		packedycbcrLine[count] = value;
	}
}
// UnPack 10 Bit DPX Format linebuffer to RGBAlpha10BitPixel linebuffer.
void UnPack10BitDPXtoRGBAlpha10BitPixel(RGBAlpha10BitPixel* rgba10BitBuffer, const ULWord* DPXLinebuffer ,ULWord numPixels, bool bigEndian)
{
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		ULWord value = DPXLinebuffer[pixel];
		if ( bigEndian)
		{
			rgba10BitBuffer[pixel].Red = UWord((value&0xC0)>>14) + UWord((value&0xFF)<<2);
			rgba10BitBuffer[pixel].Green = UWord((value&0x3F00)>>4) + UWord((value&0xF00000)>>20);
			rgba10BitBuffer[pixel].Blue = UWord((value&0xFC000000)>>26) + UWord((value&0xF0000)>>12);
		}
		else
		{
			rgba10BitBuffer[pixel].Red = (value>>22)&0x3FF;
			rgba10BitBuffer[pixel].Green = (value>>12)&0x3FF;
			rgba10BitBuffer[pixel].Blue = (value>>2)&0x3FF;

		}
	}
}

void UnPack10BitDPXtoForRP215withEndianSwap(UWord* rawrp215Buffer,ULWord* DPXLinebuffer ,ULWord numPixels)
{
	// gets the green component.
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		ULWord value = DPXLinebuffer[pixel];
		rawrp215Buffer[pixel] = ((value&0x3F00)>>4) + ((value&0xF00000)>>20);
	}
}

void UnPack10BitDPXtoForRP215(UWord* rawrp215Buffer,ULWord* DPXLinebuffer ,ULWord numPixels)
{
	// gets the green component.
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		ULWord value = DPXLinebuffer[pixel];
		rawrp215Buffer[pixel] = ((value&0x3F)>>4) + ((value&0xF00000)>>20);
	}
}

// MaskYCbCrLine
// Mask Data In place based on signalMask
void MaskYCbCrLine(UWord* ycbcrLine, UWord signalMask , ULWord numPixels)
{
	ULWord pixelCount;

	// Not elegant but fairly fast.
	switch ( signalMask )
	{
	case NTV2_SIGNALMASK_NONE:          // Output Black
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrLine[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrLine[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrLine[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}
		break;
	case NTV2_SIGNALMASK_Y:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrLine[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break;
	case NTV2_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrLine[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrLine[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}

		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break;

	case NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrLine[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrLine[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
		}


		break;
	case NTV2_SIGNALMASK_Cb + NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrLine[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cb + NTV2_SIGNALMASK_Cr:
		// Do nothing
		break;
	}

}

void Make10BitBlackLine (UWord * pOutLineData, const ULWord inNumPixels)
{
	// Assume 1080 format
	for (ULWord count(0);  count < inNumPixels*2;  count+=2)
	{
		pOutLineData[count]   = UWord(CCIR601_10BIT_CHROMAOFFSET);
		pOutLineData[count+1] = UWord(CCIR601_10BIT_BLACK);
	}
}

void Make10BitWhiteLine (UWord* pOutLineData, const ULWord inNumPixels)
{
	// assumes lineData is large enough for numPixels
	for (ULWord count(0);  count < inNumPixels*2;  count+=2)
	{
		pOutLineData[count] = UWord(CCIR601_10BIT_CHROMAOFFSET);
		pOutLineData[count+1] = UWord(CCIR601_10BIT_WHITE);
	}
}

void Make10BitLine (UWord* pOutLineData, const UWord Y, const UWord Cb, const UWord Cr, const ULWord inNumPixels)
{
	// assumes lineData is large enough for numPixels
	for (ULWord count(0);  count < inNumPixels*2;  count+=4)
	{
		pOutLineData[count] = Cb;
		pOutLineData[count+1] = Y;
		pOutLineData[count+2] = Cr;
		pOutLineData[count+3] = Y;
	}
}

#if !defined(NTV2_DEPRECATE_13_0)
	void Fill10BitYCbCrVideoFrame(PULWord _baseVideoAddress,
								 const NTV2Standard standard,
								 const NTV2FrameBufferFormat frameBufferFormat,
								 const YCbCr10BitPixel color,
								 const bool vancEnabled,
								 const bool twoKby1080,
								 const bool wideVANC)
	{
		NTV2FormatDescriptor fd (standard,frameBufferFormat,vancEnabled,twoKby1080,wideVANC);
		UWord lineBuffer[2048*2];
		Make10BitLine(lineBuffer,color.y,color.cb,color.cr,fd.numPixels);
		for ( UWord i= 0; i<fd.numLines; i++)
		{
			::PackLine_16BitYUVto10BitYUV(lineBuffer,_baseVideoAddress,fd.numPixels);
			_baseVideoAddress += fd.linePitch;
		}
	}
#endif	//	!defined(NTV2_DEPRECATE_13_0)

bool Fill10BitYCbCrVideoFrame (void * pBaseVideoAddress,
								const NTV2Standard inStandard,
								const NTV2FrameBufferFormat inFBF,
								const YCbCr10BitPixel inPixelColor,
								const NTV2VANCMode inVancMode)
{
	if (!pBaseVideoAddress)
		return false;

	const NTV2FormatDescriptor fd (inStandard, inFBF, inVancMode);
	UWord		lineBuffer[2048*2];
	ULWord *	pBaseAddress	(reinterpret_cast<ULWord*>(pBaseVideoAddress));
	Make10BitLine (lineBuffer, inPixelColor.y, inPixelColor.cb, inPixelColor.cr, UWord(fd.GetRasterWidth()));

	for (UWord lineNdx(0);  lineNdx < fd.numLines;  lineNdx++)
	{
		::PackLine_16BitYUVto10BitYUV (lineBuffer, pBaseAddress, fd.GetRasterWidth());
		pBaseAddress += fd.linePitch;
	}
	return true;
}


void Make8BitBlackLine(UByte* lineData,ULWord numPixels,NTV2FrameBufferFormat fbFormat)
{
	// assumes lineData is large enough for numPixels
	if ( fbFormat == NTV2_FBF_8BIT_YCBCR )
	{
		for ( uint32_t count = 0; count < numPixels*2; count+=2 )
		{
			lineData[count] = UWord(CCIR601_8BIT_CHROMAOFFSET);
			lineData[count+1] = UWord(CCIR601_8BIT_BLACK);
		}
	}
	else
	{
		// NTV2_FBF_8BIT_YCBCR_YUY2
		for ( uint32_t count = 0; count < numPixels*2; count+=2 )
		{
			lineData[count] = UWord(CCIR601_8BIT_BLACK);
			lineData[count+1] = UWord(CCIR601_8BIT_CHROMAOFFSET);
		}
	}
}

void Make8BitWhiteLine(UByte* lineData,ULWord numPixels,NTV2FrameBufferFormat fbFormat)
{
	// assumes lineData is large enough for numPixels
	// assumes lineData is large enough for numPixels
	if ( fbFormat == NTV2_FBF_8BIT_YCBCR )
	{
		for ( uint32_t count = 0; count < numPixels*2; count+=2 )
		{
			lineData[count] = UWord(CCIR601_8BIT_CHROMAOFFSET);
			lineData[count+1] = UWord(CCIR601_8BIT_WHITE);
		}
	}
	else
	{
		// NTV2_FBF_8BIT_YCBCR_YUY2
		for ( uint32_t count = 0; count < numPixels*2; count+=2 )
		{
			lineData[count] = UWord(CCIR601_8BIT_WHITE);
			lineData[count+1] = UWord(CCIR601_8BIT_CHROMAOFFSET);
		}
	}

}

void Make8BitLine(UByte* lineData, UByte Y , UByte Cb , UByte Cr,ULWord numPixels,NTV2FrameBufferFormat fbFormat)
{
	// assumes lineData is large enough for numPixels

	if ( fbFormat == NTV2_FBF_8BIT_YCBCR )
	{
		for ( ULWord count = 0; count < numPixels*2; count+=4 )
		{
			lineData[count] = Cb;
			lineData[count+1] = Y;
			lineData[count+2] = Cr;
			lineData[count+3] = Y;
		}
	}
	else
	{
		for ( ULWord count = 0; count < numPixels*2; count+=4 )
		{
			lineData[count] = Y;
			lineData[count+1] = Cb;
			lineData[count+2] = Y;
			lineData[count+3] = Cr;
		}

	}
}

#if !defined(NTV2_DEPRECATE_13_0)
	void Fill8BitYCbCrVideoFrame(PULWord _baseVideoAddress,
								 NTV2Standard standard,
								 NTV2FrameBufferFormat frameBufferFormat,
								 YCbCrPixel color,
								 bool vancEnabled,
								 bool twoKby1080,
								 bool wideVANC)
	{
		NTV2FormatDescriptor fd (standard,frameBufferFormat,vancEnabled,twoKby1080,wideVANC);
	
		for ( UWord i= 0; i<fd.numLines; i++)
		{
			Make8BitLine((UByte*)_baseVideoAddress,color.y,color.cb,color.cr,fd.numPixels,frameBufferFormat);
			_baseVideoAddress += fd.linePitch;
		}
	}
#endif	//	!defined(NTV2_DEPRECATE_13_0)

bool Fill8BitYCbCrVideoFrame (void * pBaseVideoAddress,  const NTV2Standard inStandard,  const NTV2FrameBufferFormat inFBF,
								const YCbCrPixel inPixelColor,  const NTV2VANCMode inVancMode)
{
	if (!pBaseVideoAddress)
		return false;

	const NTV2FormatDescriptor fd (inStandard, inFBF, inVancMode);
	UByte *		pBaseAddress	(reinterpret_cast<UByte*>(pBaseVideoAddress));

	for (UWord lineNdx(0);  lineNdx < fd.numLines;  lineNdx++)
	{
		Make8BitLine (pBaseAddress, inPixelColor.y, inPixelColor.cb, inPixelColor.cr, fd.numPixels, inFBF);
		pBaseAddress += fd.GetBytesPerRow();
	}
	return true;
}

void Fill4k8BitYCbCrVideoFrame(PULWord _baseVideoAddress,
							 NTV2FrameBufferFormat frameBufferFormat,
							 YCbCrPixel color,
							 bool vancEnabled,
							 bool b4k,
							 bool wideVANC)
{
	(void) vancEnabled;
	(void) wideVANC;
	NTV2FormatDescriptor fd;
	if(b4k)
	{
		fd.numLines = 2160;
		fd.numPixels = 4096;
		fd.firstActiveLine = 0;
		fd.linePitch = 4096*2/4;
	}
	else
	{
		fd.numLines = 2160;
		fd.numPixels = 3840;
		fd.firstActiveLine = 0;
		fd.linePitch = 3840*2/4;
	}

	Make8BitLine(reinterpret_cast<UByte*>(_baseVideoAddress), color.y, color.cb, color.cr, fd.numPixels*fd.numLines, frameBufferFormat);
}


// Copy arbrary-sized source image buffer to arbitrary-sized destination frame buffer.
// It will automatically clip and/or pad the source image to center it in the destination frame.
// This will work with any RGBA/RGB frame buffer formats with 4 Bytes/pixel size
void CopyRGBAImageToFrame(ULWord* pSrcBuffer, ULWord srcWidth, ULWord srcHeight,
						  ULWord* pDstBuffer, ULWord dstWidth, ULWord dstHeight)
{
	// all variables are in pixels
	ULWord topPad = 0, bottomPad = 0, leftPad = 0, rightPad = 0;
	ULWord contentHeight = 0;
	ULWord contentWidth = 0;
	ULWord* pSrc = pSrcBuffer;
	ULWord* pDst = pDstBuffer;

	if (dstHeight > srcHeight)
	{
		topPad = (dstHeight - srcHeight) / 2;
		bottomPad = dstHeight - topPad - srcHeight;
	}
	else
		pSrc += ((srcHeight - dstHeight) / 2) * srcWidth;

	if (dstWidth > srcWidth)
	{
		leftPad = (dstWidth - srcWidth) / 2;
		rightPad = dstWidth - srcWidth - leftPad;
	}
	else
		pSrc += (srcWidth - dstWidth) / 2;

	// content
	contentHeight = dstHeight - topPad - bottomPad;
	contentWidth = dstWidth - leftPad - rightPad;

	// top pad
	memset(pDst, 0, topPad * dstWidth * 4);
	pDst += topPad * dstWidth;

	// content
	while (contentHeight--)
	{
		// left
		memset(pDst, 0, leftPad * 4);
		pDst += leftPad;

		// content
		memcpy(pDst, pSrc, contentWidth * 4);
		pDst += contentWidth;
		pSrc += srcWidth;

		// right
		memset(pDst, 0, rightPad * 4);
		pDst += rightPad;
	}

	// bottom pad
	memset(pDst, 0, bottomPad * dstWidth * 4);
}


static bool SetRasterLinesBlack8BitYCbCr (UByte *			pDstBuffer,
											const ULWord		inDstBytesPerLine,
											const UWord		inDstTotalLines)
{
	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / 2);	//	2 bytes per pixel for '2vuy'
	UByte *			pLine				(pDstBuffer);
	NTV2_ASSERT(dstMaxPixelWidth < 64UL*1024UL);	//	Because Make8BitBlackLine takes uint16_t pixelWidth
	for (UWord lineNum(0);  lineNum < inDstTotalLines;  lineNum++)
	{
		::Make8BitBlackLine (pLine, UWord(dstMaxPixelWidth));
		pLine += inDstBytesPerLine;
	}
	return true;
}


static bool SetRasterLinesBlack10BitYCbCr (UByte *			pDstBuffer,
											const ULWord		inDstBytesPerLine,
											const UWord		inDstTotalLines)
{
	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / 16 * 6);
	UByte *		pLine				(pDstBuffer);
	for (UWord lineNum (0);  lineNum < inDstTotalLines;  lineNum++)
	{
		UWord *		pDstUWord	(reinterpret_cast <UWord *> (pLine));
		ULWord *	pDstULWord	(reinterpret_cast <ULWord *> (pLine));
		::Make10BitBlackLine (pDstUWord, dstMaxPixelWidth);
		::PackLine_16BitYUVto10BitYUV (pDstUWord, pDstULWord, dstMaxPixelWidth);
		pLine += inDstBytesPerLine;
	}
	return true;
}


static bool SetRasterLinesBlack8BitRGBs (const NTV2FrameBufferFormat	inPixelFormat,
											UByte *						pDstBuffer,
											const ULWord					inDstBytesPerLine,
											const UWord					inDstTotalLines,
											const bool					inIsSD	= false)
{
	UByte *			pLine				(pDstBuffer);
	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / 4);	//	4 bytes per pixel
	YCbCrAlphaPixel	YCbCr = {0 /*Alpha*/,	44/*cr*/,	10/*Y*/,	44/*cb*/};
	RGBAlphaPixel	rgb;
	if (inIsSD)
		::SDConvertYCbCrtoRGBSmpte (&YCbCr, &rgb);
	else
		::HDConvertYCbCrtoRGBSmpte (&YCbCr, &rgb);

	//	Set the first line...
	for (ULWord pixNum (0);  pixNum < dstMaxPixelWidth;  pixNum++)
	{
		switch (inPixelFormat)
		{
			case NTV2_FBF_ARGB:	pLine[0] = rgb.Alpha;	pLine[0] = rgb.Red;		pLine[0] = rgb.Green;	pLine[0] = rgb.Blue;	break;
			case NTV2_FBF_RGBA:	pLine[0] = rgb.Red;		pLine[0] = rgb.Green;	pLine[0] = rgb.Blue;	pLine[0] = rgb.Alpha;	break;
			case NTV2_FBF_ABGR:	pLine[0] = rgb.Alpha;	pLine[0] = rgb.Blue;	pLine[0] = rgb.Green;	pLine[0] = rgb.Red;		break;
			default:			return false;
		}
		pLine += 4;		//	4 bytes per pixel
	}

	//	Set the rest...
	pLine = pDstBuffer + inDstBytesPerLine;
	for (UWord lineNum (1);  lineNum < inDstTotalLines;  lineNum++)
	{
		::memcpy (pLine, pDstBuffer, inDstBytesPerLine);
		pLine += inDstBytesPerLine;
	}
	return true;
}


static bool SetRasterLinesBlack10BitRGB (UByte *			pDstBuffer,
											const ULWord		inDstBytesPerLine,
											const UWord		inDstTotalLines,
											const bool		inIsSD	= false)
{
	UByte *			pLine				(pDstBuffer + inDstBytesPerLine);
	ULWord *		pPixels				(reinterpret_cast <ULWord *> (pDstBuffer));
	const ULWord		dstMaxPixelWidth	(inDstBytesPerLine / 4);	//	4 bytes per pixel
	ULWord			blackOpaque			(0xC0400004);	(void) inIsSD;	//	For now

	//	Set the first line...
	for (ULWord pixNum(0);  pixNum < dstMaxPixelWidth;  pixNum++)
		pPixels[pixNum] = blackOpaque;

	//	Set the rest...
	for (UWord lineNum(1);  lineNum < inDstTotalLines;  lineNum++)
	{
		::memcpy (pLine, pDstBuffer, inDstBytesPerLine);
		pLine += inDstBytesPerLine;
	}
	return true;
}


bool SetRasterLinesBlack (const NTV2FrameBufferFormat	inPixelFormat,
							UByte *						pDstBuffer,
							const ULWord					inDstBytesPerLine,
							const UWord					inDstTotalLines)
{
	if (!pDstBuffer)					//	NULL buffer
		return false;
	if (inDstBytesPerLine == 0)			//	zero rowbytes
		return false;
	if (inDstTotalLines == 0)			//	zero height
		return false;

	switch (inPixelFormat)
	{
		case NTV2_FBF_10BIT_YCBCR:		return SetRasterLinesBlack10BitYCbCr (pDstBuffer, inDstBytesPerLine, inDstTotalLines);

		case NTV2_FBF_8BIT_YCBCR:		return SetRasterLinesBlack8BitYCbCr (pDstBuffer, inDstBytesPerLine, inDstTotalLines);

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:				return SetRasterLinesBlack8BitRGBs (inPixelFormat, pDstBuffer, inDstBytesPerLine, inDstTotalLines);

		case NTV2_FBF_10BIT_RGB:		return SetRasterLinesBlack10BitRGB (pDstBuffer, inDstBytesPerLine, inDstTotalLines);

		case NTV2_FBF_8BIT_YCBCR_YUY2:
		case NTV2_FBF_10BIT_DPX:
		case NTV2_FBF_10BIT_YCBCR_DPX:
		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_YCBCR_420PL3:
		case NTV2_FBF_8BIT_HDV:
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_10BIT_YCBCRA:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_48BIT_RGB:
		case NTV2_FBF_12BIT_RGB_PACKED:
		case NTV2_FBF_PRORES_DVCPRO:
		case NTV2_FBF_PRORES_HDV:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
		case NTV2_FBF_8BIT_YCBCR_422PL3:
		case NTV2_FBF_10BIT_RAW_RGB:
		case NTV2_FBF_10BIT_RAW_YCBCR:
		case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
        case NTV2_FBF_10BIT_YCBCR_420PL2:
        case NTV2_FBF_10BIT_YCBCR_422PL2:
        case NTV2_FBF_8BIT_YCBCR_420PL2:
        case NTV2_FBF_8BIT_YCBCR_422PL2:
		case NTV2_FBF_NUMFRAMEBUFFERFORMATS:
			return false;
	}
	return false;

}	//	SetRasterLinesBlack


static const UByte * GetReadAddress_2vuy (const UByte * pInFrameBuffer, const ULWord inBytesPerVertLine, const UWord inVertLineOffset, const UWord inHorzPixelOffset, const UWord inBytesPerHorzPixel)
{
	const UByte *	pResult	(pInFrameBuffer);
	NTV2_ASSERT (inBytesPerVertLine);
	NTV2_ASSERT ((inHorzPixelOffset & 1) == 0);	//	For '2vuy', horizontal pixel offset must be even!!
	pResult += inBytesPerVertLine * ULWord(inVertLineOffset);
	pResult += ULWord(inBytesPerHorzPixel) * ULWord(inHorzPixelOffset);
	return pResult;
}


static UByte * GetWriteAddress_2vuy (UByte * pInFrameBuffer, const ULWord inBytesPerVertLine, const UWord inVertLineOffset, const UWord inHorzPixelOffset, const UWord inBytesPerHorzPixel)
{
	UByte *	pResult	(pInFrameBuffer);
	NTV2_ASSERT (inBytesPerVertLine);
	NTV2_ASSERT ((inHorzPixelOffset & 1) == 0);	//	For '2vuy', horizontal pixel offset must be even!!
	pResult += inBytesPerVertLine * ULWord(inVertLineOffset);
	pResult += ULWord(inBytesPerHorzPixel) * ULWord(inHorzPixelOffset);
	return pResult;
}


//	This function should work on all 4-byte-per-2-pixel formats
static bool CopyRaster4BytesPer2Pixels (UByte *			pDstBuffer,				//	Dest buffer to be modified
										const ULWord	inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
										const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
										const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
										const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
										const UByte *	pSrcBuffer,				//	Src buffer
										const ULWord	inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
										const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
										const UWord		inSrcVertLineOffset,	//	Src image top edge
										const UWord		inSrcVertLinesToCopy,	//	Src image height
										const UWord		inSrcHorzPixelOffset,	//	Src image left edge
										const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	if (inDstHorzPixelOffset & 1)	//	dst odd pixel offset
		return false;
	if (inSrcHorzPixelOffset & 1)	//	src odd pixel offset
		return false;

	const ULWord	TWO_BYTES_PER_PIXEL	(2);			//	2 bytes per pixel for '2vuy'
	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / TWO_BYTES_PER_PIXEL);
	const ULWord	srcMaxPixelWidth	(inSrcBytesPerLine / TWO_BYTES_PER_PIXEL);
	UWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (ULWord(inSrcHorzPixelOffset + inSrcHorzPixelsToCopy) > srcMaxPixelWidth)
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	const UByte *	pSrc	(::GetReadAddress_2vuy (pSrcBuffer, inSrcBytesPerLine, inSrcVertLineOffset, inSrcHorzPixelOffset, TWO_BYTES_PER_PIXEL));
	UByte *			pDst	(::GetWriteAddress_2vuy (pDstBuffer, inDstBytesPerLine, inDstVertLineOffset, inDstHorzPixelOffset, TWO_BYTES_PER_PIXEL));

	for (UWord srcLinesToCopy (numVertLinesToCopy);  srcLinesToCopy > 0;  srcLinesToCopy--)	//	for each src raster line
	{
		UWord			dstPixelsCopied	(0);
		const UByte *	pSavedSrc		(pSrc);
		UByte *			pSavedDst		(pDst);
		for (UWord hPixelsToCopy (numHorzPixelsToCopy);  hPixelsToCopy > 0;  hPixelsToCopy--)	//	for each pixel/column
		{
			pDst[0] = pSrc[0];
			pDst[1] = pSrc[1];
			dstPixelsCopied++;
			if (dstPixelsCopied + inDstHorzPixelOffset >= UWord(dstMaxPixelWidth))
				break;	//	Clip to dst raster's right edge
			pDst += TWO_BYTES_PER_PIXEL;
			pSrc += TWO_BYTES_PER_PIXEL;
		}
		pSrc = pSavedSrc;
		pDst = pSavedDst;
		pSrc += inSrcBytesPerLine;
		pDst += inDstBytesPerLine;
	}	//	for each src line to copy
	return true;

}	//	CopyRaster4BytesPer2Pixels


//	This function should work on all 16-byte-per-6-pixel formats
static bool CopyRaster16BytesPer6Pixels (	UByte *			pDstBuffer,				//	Dest buffer to be modified
											const ULWord	inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width) -- must be evenly divisible by 16
											const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
											const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
											const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear -- must be evenly divisible by 6
											const UByte *	pSrcBuffer,				//	Src buffer
											const ULWord	inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width) -- must be evenly divisible by 16
											const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
											const UWord		inSrcVertLineOffset,	//	Src image top edge
											const UWord		inSrcVertLinesToCopy,	//	Src image height
											const UWord		inSrcHorzPixelOffset,	//	Src image left edge -- must be evenly divisible by 6
											const UWord		inSrcHorzPixelsToCopy)	//	Src image width -- must be evenly divisible by 6
{
	if (inDstHorzPixelOffset % 6)	//	dst pixel offset must be on 6-pixel boundary
		return false;
	if (inSrcHorzPixelOffset % 6)	//	src pixel offset must be on 6-pixel boundary
		return false;
	if (inDstBytesPerLine % 16)		//	dst raster width must be evenly divisible by 16 (width must be multiple of 6)
		return false;
	if (inSrcBytesPerLine % 16)		//	src raster width must be evenly divisible by 16 (width must be multiple of 6)
		return false;
	if (inSrcHorzPixelsToCopy % 6)	//	pixel width of src image portion to copy must be on 6-pixel boundary
		return false;

	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / 16 * 6);
	const ULWord	srcMaxPixelWidth	(inSrcBytesPerLine / 16 * 6);
	ULWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > UWord(srcMaxPixelWidth))
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	NTV2_ASSERT (numHorzPixelsToCopy % 6 == 0);
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * 16 / 6);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * 16 / 6);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * 16 / 6);	//	copy the line
	}

	return true;

}	//	CopyRaster16BytesPer6Pixels


//	This function should work on all 20-byte-per-16-pixel formats
static bool CopyRaster20BytesPer16Pixels (	UByte *			pDstBuffer,				//	Dest buffer to be modified
											const ULWord	inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width) -- must be evenly divisible by 20
											const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
											const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
											const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
											const UByte *	pSrcBuffer,				//	Src buffer
											const ULWord	inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width) -- must be evenly divisible by 20
											const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
											const UWord		inSrcVertLineOffset,	//	Src image top edge
											const UWord		inSrcVertLinesToCopy,	//	Src image height
											const UWord		inSrcHorzPixelOffset,	//	Src image left edge
											const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	if (inDstHorzPixelOffset % 16)	//	dst pixel offset must be on 16-pixel boundary
		return false;
	if (inSrcHorzPixelOffset % 16)	//	src pixel offset must be on 16-pixel boundary
		return false;
	if (inDstBytesPerLine % 20)		//	dst raster width must be evenly divisible by 20
		return false;
	if (inSrcBytesPerLine % 20)		//	src raster width must be evenly divisible by 20
		return false;
	if (inSrcHorzPixelsToCopy % 16)	//	pixel width of src image portion to copy must be on 16-pixel boundary
		return false;

	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / 20 * 16);
	const ULWord	srcMaxPixelWidth	(inSrcBytesPerLine / 20 * 16);
	ULWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > UWord(srcMaxPixelWidth))
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	NTV2_ASSERT (numHorzPixelsToCopy % 16 == 0);
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * 20 / 16);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * 20 / 16);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * 20 / 16);	//	copy the line
	}

	return true;

}	//	CopyRaster20BytesPer16Pixels

//	This function should work on all 36-byte-per-8-pixel formats
static bool CopyRaster36BytesPer8Pixels (	UByte *			pDstBuffer,				//	Dest buffer to be modified
											const ULWord	inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width) -- must be evenly divisible by 20
											const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
											const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
											const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
											const UByte *	pSrcBuffer,				//	Src buffer
											const ULWord	inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width) -- must be evenly divisible by 20
											const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
											const UWord		inSrcVertLineOffset,	//	Src image top edge
											const UWord		inSrcVertLinesToCopy,	//	Src image height
											const UWord		inSrcHorzPixelOffset,	//	Src image left edge
											const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	if (inDstHorzPixelOffset % 8)	//	dst pixel offset must be on 16-pixel boundary
		return false;
	if (inSrcHorzPixelOffset % 8)	//	src pixel offset must be on 16-pixel boundary
		return false;
	if (inDstBytesPerLine % 36)		//	dst raster width must be evenly divisible by 20
		return false;
	if (inSrcBytesPerLine % 36)		//	src raster width must be evenly divisible by 20
		return false;
	if (inSrcHorzPixelsToCopy % 8)	//	pixel width of src image portion to copy must be on 16-pixel boundary
		return false;

	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / 36 * 8);
	const ULWord	srcMaxPixelWidth	(inSrcBytesPerLine / 36 * 8);
	ULWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > UWord(srcMaxPixelWidth))
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	NTV2_ASSERT (numHorzPixelsToCopy % 8 == 0);
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * 36 / 8);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * 36 / 8);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * 36 / 8);	//	copy the line
	}

	return true;

}	//	CopyRaster20BytesPer16Pixels


//	This function should work on all 4-byte-per-pixel formats
static bool CopyRaster4BytesPerPixel (	UByte *			pDstBuffer,				//	Dest buffer to be modified
										const ULWord	inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
										const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
										const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
										const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear -- must be evenly divisible by 6
										const UByte *	pSrcBuffer,				//	Src buffer
										const ULWord	inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
										const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
										const UWord		inSrcVertLineOffset,	//	Src image top edge
										const UWord		inSrcVertLinesToCopy,	//	Src image height
										const UWord		inSrcHorzPixelOffset,	//	Src image left edge
										const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	const UWord	FOUR_BYTES_PER_PIXEL	(4);

	if (inDstBytesPerLine % FOUR_BYTES_PER_PIXEL)	//	dst raster width (in bytes) must be evenly divisible by 4
		return false;
	if (inSrcBytesPerLine % FOUR_BYTES_PER_PIXEL)	//	src raster width (in bytes) must be evenly divisible by 4
		return false;

	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / FOUR_BYTES_PER_PIXEL);
	const ULWord	srcMaxPixelWidth	(inSrcBytesPerLine / FOUR_BYTES_PER_PIXEL);
	ULWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > UWord(srcMaxPixelWidth))
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * FOUR_BYTES_PER_PIXEL);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * FOUR_BYTES_PER_PIXEL);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * FOUR_BYTES_PER_PIXEL);	//	copy the line
	}

	return true;

}	//	CopyRaster4BytesPerPixel


//	This function should work on all 3-byte-per-pixel formats
static bool CopyRaster3BytesPerPixel (	UByte *			pDstBuffer,				//	Dest buffer to be modified
										const ULWord	inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
										const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
										const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
										const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear -- must be evenly divisible by 6
										const UByte *	pSrcBuffer,				//	Src buffer
										const ULWord	inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
										const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
										const UWord		inSrcVertLineOffset,	//	Src image top edge
										const UWord		inSrcVertLinesToCopy,	//	Src image height
										const UWord		inSrcHorzPixelOffset,	//	Src image left edge
										const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	const UWord	THREE_BYTES_PER_PIXEL	(3);

	if (inDstBytesPerLine % THREE_BYTES_PER_PIXEL)	//	dst raster width (in bytes) must be evenly divisible by 3
		return false;
	if (inSrcBytesPerLine % THREE_BYTES_PER_PIXEL)	//	src raster width (in bytes) must be evenly divisible by 3
		return false;

	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / THREE_BYTES_PER_PIXEL);
	const ULWord	srcMaxPixelWidth	(inSrcBytesPerLine / THREE_BYTES_PER_PIXEL);
	ULWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > UWord(srcMaxPixelWidth))
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * THREE_BYTES_PER_PIXEL);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * THREE_BYTES_PER_PIXEL);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * THREE_BYTES_PER_PIXEL);	//	copy the line
	}

	return true;

}	//	CopyRaster3BytesPerPixel


//	This function should work on all 6-byte-per-pixel formats
static bool CopyRaster6BytesPerPixel (	UByte *			pDstBuffer,				//	Dest buffer to be modified
										const ULWord	inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
										const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
										const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
										const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear -- must be evenly divisible by 6
										const UByte *	pSrcBuffer,				//	Src buffer
										const ULWord	inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
										const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
										const UWord		inSrcVertLineOffset,	//	Src image top edge
										const UWord		inSrcVertLinesToCopy,	//	Src image height
										const UWord		inSrcHorzPixelOffset,	//	Src image left edge
										const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	const UWord	SIX_BYTES_PER_PIXEL	(6);

	if (inDstBytesPerLine % SIX_BYTES_PER_PIXEL)	//	dst raster width (in bytes) must be evenly divisible by 6
		return false;
	if (inSrcBytesPerLine % SIX_BYTES_PER_PIXEL)	//	src raster width (in bytes) must be evenly divisible by 6
		return false;

	const ULWord	dstMaxPixelWidth	(inDstBytesPerLine / SIX_BYTES_PER_PIXEL);
	const ULWord	srcMaxPixelWidth	(inSrcBytesPerLine / SIX_BYTES_PER_PIXEL);
	ULWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > UWord(srcMaxPixelWidth))
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * SIX_BYTES_PER_PIXEL);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * SIX_BYTES_PER_PIXEL);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * SIX_BYTES_PER_PIXEL);	//	copy the line
	}

	return true;

}	//	CopyRaster6BytesPerPixel


bool CopyRaster (const NTV2FrameBufferFormat	inPixelFormat,			//	Pixel format of both src and dst buffers
				UByte *							pDstBuffer,				//	Dest buffer to be modified
				const ULWord						inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
				const UWord						inDstTotalLines,		//	Dest buffer total lines in raster (max height)
				const UWord						inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
				const UWord						inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
				const UByte *					pSrcBuffer,				//	Src buffer
				const ULWord						inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
				const UWord						inSrcTotalLines,		//	Src buffer total lines in raster (max height)
				const UWord						inSrcVertLineOffset,	//	Src image top edge
				const UWord						inSrcVertLinesToCopy,	//	Src image height
				const UWord						inSrcHorzPixelOffset,	//	Src image left edge
				const UWord						inSrcHorzPixelsToCopy)	//	Src image width
{
	if (!pDstBuffer)					//	NULL buffer
		return false;
	if (!pSrcBuffer)					//	NULL buffer
		return false;
	if (pDstBuffer == pSrcBuffer)		//	src & dst buffers must be different
		return false;
	if (inDstBytesPerLine == 0)			//	zero rowbytes
		return false;
	if (inSrcBytesPerLine == 0)			//	zero rowbytes
		return false;
	if (inDstTotalLines == 0)			//	zero height
		return false;
	if (inSrcTotalLines == 0)			//	zero height
		return false;
	if (inDstVertLineOffset >= inDstTotalLines)		//	dst past bottom edge
		return false;
	if (inSrcVertLineOffset >= inSrcTotalLines)		//	src past bottom edge
		return false;
	switch (inPixelFormat)
	{
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_10BIT_YCBCR_DPX:			return CopyRaster16BytesPer6Pixels (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																					pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																					inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);
	
		case NTV2_FBF_8BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:			return CopyRaster4BytesPer2Pixels (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																					pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																					inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);
	
		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_DPX:
		case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_10BIT_RGB:				return CopyRaster4BytesPerPixel (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																				pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																				inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);
	
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:				return CopyRaster3BytesPerPixel	(pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																				pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																				inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);
	
		case NTV2_FBF_48BIT_RGB:				return CopyRaster6BytesPerPixel	(pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																				pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																				inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);
	
		case NTV2_FBF_12BIT_RGB_PACKED:			return CopyRaster36BytesPer8Pixels (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																					pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																					inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);
		case NTV2_FBF_10BIT_RAW_YCBCR:			return CopyRaster20BytesPer16Pixels (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																					pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																					inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);
	
		case NTV2_FBF_8BIT_DVCPRO:	//	Lossy
		case NTV2_FBF_8BIT_HDV:		//	Lossy
		case NTV2_FBF_8BIT_YCBCR_420PL3:
		case NTV2_FBF_10BIT_YCBCRA:
		case NTV2_FBF_PRORES_DVCPRO:
		case NTV2_FBF_PRORES_HDV:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
		case NTV2_FBF_10BIT_RAW_RGB:
		case NTV2_FBF_8BIT_YCBCR_422PL3:
		case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_420PL2:
		case NTV2_FBF_10BIT_YCBCR_422PL2:
		case NTV2_FBF_8BIT_YCBCR_420PL2:
		case NTV2_FBF_8BIT_YCBCR_422PL2:
		case NTV2_FBF_NUMFRAMEBUFFERFORMATS:
			return false;	//	Unsupported
	}
	return false;

}	//	CopyRaster


// frames per second
double GetFramesPerSecond (const NTV2FrameRate inFrameRate)
{
    switch (inFrameRate)
    {
		case NTV2_FRAMERATE_12000:	return 120.0;
		case NTV2_FRAMERATE_11988:	return 120000.0 / 1001.0;
		case NTV2_FRAMERATE_6000:	return 60.0;
		case NTV2_FRAMERATE_5994:	return 60000.0 / 1001.0;
		case NTV2_FRAMERATE_5000:	return 50.0;
		case NTV2_FRAMERATE_4800:	return 48.0;
		case NTV2_FRAMERATE_4795:	return 48000.0 / 1001.0;
		case NTV2_FRAMERATE_3000:	return 30.0;
		case NTV2_FRAMERATE_2997:	return 30000.0 / 1001.0;
		case NTV2_FRAMERATE_2500:	return 25.0;
		case NTV2_FRAMERATE_2400:	return 24.0;
		case NTV2_FRAMERATE_2398:	return 24000.0 / 1001.0;
		case NTV2_FRAMERATE_1500:	return 15.0;
		case NTV2_FRAMERATE_1498:	return 15000.0 / 1001.0;
#if !defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_1900:	return 19.0;
		case NTV2_FRAMERATE_1898:	return 19000.0 / 1001.0;
		case NTV2_FRAMERATE_1800:	return 18.0;
		case NTV2_FRAMERATE_1798:	return 18000.0 / 1001.0;
#endif	//!defined(NTV2_DEPRECATE_16_0)
#if defined(_DEBUG)
		case NTV2_NUM_FRAMERATES:
		case NTV2_FRAMERATE_UNKNOWN:	break;
#else
		default:						break;
#endif
	}
	return 30.0 / 1.001;
}


bool GetFramesPerSecond (const NTV2FrameRate inFrameRate, ULWord & outFractionNumerator, ULWord & outFractionDenominator)
{
	switch (inFrameRate)
    {
		case NTV2_FRAMERATE_12000:		outFractionNumerator = 120;     outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_11988:		outFractionNumerator = 120000;	outFractionDenominator = 1001;	break;
		case NTV2_FRAMERATE_6000:		outFractionNumerator = 60;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_5994:		outFractionNumerator = 60000;	outFractionDenominator = 1001;	break;
		case NTV2_FRAMERATE_5000:		outFractionNumerator = 50;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_4800:		outFractionNumerator = 48;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_4795:		outFractionNumerator = 48000;	outFractionDenominator = 1001;	break;
		case NTV2_FRAMERATE_3000:		outFractionNumerator = 30;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_2997:		outFractionNumerator = 30000;	outFractionDenominator = 1001;	break;
		case NTV2_FRAMERATE_2500:		outFractionNumerator = 25;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_2400:		outFractionNumerator = 24;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_2398:		outFractionNumerator = 24000;	outFractionDenominator = 1001;	break;
		case NTV2_FRAMERATE_1500:		outFractionNumerator = 15;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_1498:		outFractionNumerator = 15000;	outFractionDenominator = 1001;	break;
#if !defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_1900:		outFractionNumerator = 19;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_1898:		outFractionNumerator = 19000;	outFractionDenominator = 1001;	break;
		case NTV2_FRAMERATE_1800:		outFractionNumerator = 18;      outFractionDenominator = 1;		break;
		case NTV2_FRAMERATE_1798:		outFractionNumerator = 18000;	outFractionDenominator = 1001;	break;
#endif	//	!defined(NTV2_DEPRECATE_16_0)
#if defined(_DEBUG)
		case NTV2_NUM_FRAMERATES:
		case NTV2_FRAMERATE_UNKNOWN:	outFractionNumerator = 0;		outFractionDenominator = 0;		return false;
#else
		default:						outFractionNumerator = 0;		outFractionDenominator = 0;		return false;
#endif
	}
	return true;
}


NTV2Standard GetNTV2StandardFromScanGeometry (const UByte inScanGeometry, const bool inIsProgressiveTransport)
{
	switch (inScanGeometry)
	{
		case NTV2_SG_525:		return NTV2_STANDARD_525;
		case NTV2_SG_625:		return NTV2_STANDARD_625;
		case NTV2_SG_750:		return NTV2_STANDARD_720;
		case NTV2_SG_2Kx1556:	return NTV2_STANDARD_2K;

		case NTV2_SG_1125:
		case NTV2_SG_2Kx1080:	return inIsProgressiveTransport ? NTV2_STANDARD_1080p : NTV2_STANDARD_1080;

		default:	break;
	}
	return NTV2_STANDARD_INVALID;
}


NTV2VideoFormat GetFirstMatchingVideoFormat (const NTV2FrameRate inFrameRate,
											 const UWord inHeightLines,
											 const UWord inWidthPixels,
											 const bool inIsInterlaced,
											 const bool inIsLevelB,
											 const bool inIsPSF)
{
	for (NTV2VideoFormat fmt(NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT);  fmt < NTV2_MAX_NUM_VIDEO_FORMATS;  fmt = NTV2VideoFormat(fmt+1))
		if (inFrameRate == ::GetNTV2FrameRateFromVideoFormat(fmt))
			if (inHeightLines == ::GetDisplayHeight(fmt))
				if (inWidthPixels == ::GetDisplayWidth(fmt))
					if (inIsInterlaced == !::IsProgressiveTransport(fmt))
						if (inIsPSF == ::IsPSF(fmt))
							if (NTV2_VIDEO_FORMAT_IS_B(fmt) == inIsLevelB)
								return fmt;
	return NTV2_FORMAT_UNKNOWN;
}


NTV2VideoFormat GetQuarterSizedVideoFormat (const NTV2VideoFormat inVideoFormat)
{
	NTV2VideoFormat quarterSizedFormat(inVideoFormat);

	switch (inVideoFormat)
	{
		case NTV2_FORMAT_3840x2160psf_2398:
		case NTV2_FORMAT_4x1920x1080psf_2398:	quarterSizedFormat = NTV2_FORMAT_1080psf_2398;   break;
		case NTV2_FORMAT_3840x2160psf_2400:
		case NTV2_FORMAT_4x1920x1080psf_2400:	quarterSizedFormat = NTV2_FORMAT_1080psf_2400;   break;
		case NTV2_FORMAT_3840x2160psf_2500:
		case NTV2_FORMAT_4x1920x1080psf_2500:	quarterSizedFormat = NTV2_FORMAT_1080psf_2500_2; break;
		case NTV2_FORMAT_3840x2160psf_2997:
		case NTV2_FORMAT_4x1920x1080psf_2997:	quarterSizedFormat = NTV2_FORMAT_1080i_5994;   break;	//	NTV2_FORMAT_1080psf_2997
		case NTV2_FORMAT_3840x2160psf_3000:
		case NTV2_FORMAT_4x1920x1080psf_3000:	quarterSizedFormat = NTV2_FORMAT_1080i_6000;   break;	//	NTV2_FORMAT_1080psf_3000

		case NTV2_FORMAT_4096x2160psf_2398:
		case NTV2_FORMAT_4x2048x1080psf_2398:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_2398; break;
		case NTV2_FORMAT_4096x2160psf_2400:
		case NTV2_FORMAT_4x2048x1080psf_2400:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_2400; break;
		case NTV2_FORMAT_4096x2160psf_2500:
		case NTV2_FORMAT_4x2048x1080psf_2500:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_2500; break;
		//case NTV2_FORMAT_4x2048x1080psf_2997:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_2997; break;
		//case NTV2_FORMAT_4x2048x1080psf_3000:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_3000; break;

		case NTV2_FORMAT_3840x2160p_2398:
		case NTV2_FORMAT_4x1920x1080p_2398:		quarterSizedFormat = NTV2_FORMAT_1080p_2398; break;
		case NTV2_FORMAT_3840x2160p_2400:
		case NTV2_FORMAT_4x1920x1080p_2400:		quarterSizedFormat = NTV2_FORMAT_1080p_2400; break;
		case NTV2_FORMAT_3840x2160p_2500:
		case NTV2_FORMAT_4x1920x1080p_2500:		quarterSizedFormat = NTV2_FORMAT_1080p_2500; break;
		case NTV2_FORMAT_3840x2160p_2997:
		case NTV2_FORMAT_4x1920x1080p_2997:		quarterSizedFormat = NTV2_FORMAT_1080p_2997; break;
		case NTV2_FORMAT_3840x2160p_3000:
		case NTV2_FORMAT_4x1920x1080p_3000:		quarterSizedFormat = NTV2_FORMAT_1080p_3000; break;
		case NTV2_FORMAT_3840x2160p_5000:
		case NTV2_FORMAT_4x1920x1080p_5000:		quarterSizedFormat = NTV2_FORMAT_1080p_5000_A; break;
		case NTV2_FORMAT_3840x2160p_5994:
		case NTV2_FORMAT_4x1920x1080p_5994:		quarterSizedFormat = NTV2_FORMAT_1080p_5994_A; break;
		case NTV2_FORMAT_3840x2160p_6000:
		case NTV2_FORMAT_4x1920x1080p_6000:		quarterSizedFormat = NTV2_FORMAT_1080p_6000_A; break;
		case NTV2_FORMAT_3840x2160p_5000_B:
		case NTV2_FORMAT_4x1920x1080p_5000_B:	quarterSizedFormat = NTV2_FORMAT_1080p_5000_B; break;
		case NTV2_FORMAT_3840x2160p_5994_B:
		case NTV2_FORMAT_4x1920x1080p_5994_B:	quarterSizedFormat = NTV2_FORMAT_1080p_5994_B; break;
		case NTV2_FORMAT_3840x2160p_6000_B:
		case NTV2_FORMAT_4x1920x1080p_6000_B:	quarterSizedFormat = NTV2_FORMAT_1080p_6000_B; break;

		case NTV2_FORMAT_4096x2160p_2398:
		case NTV2_FORMAT_4x2048x1080p_2398:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_2398; break;
		case NTV2_FORMAT_4096x2160p_2400:
		case NTV2_FORMAT_4x2048x1080p_2400:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_2400; break;
		case NTV2_FORMAT_4096x2160p_2500:
		case NTV2_FORMAT_4x2048x1080p_2500:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_2500; break;
		case NTV2_FORMAT_4096x2160p_2997:
		case NTV2_FORMAT_4x2048x1080p_2997:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_2997; break;
		case NTV2_FORMAT_4096x2160p_3000:
		case NTV2_FORMAT_4x2048x1080p_3000:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_3000; break;
		case NTV2_FORMAT_4096x2160p_4795:
		case NTV2_FORMAT_4x2048x1080p_4795:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_4795_A; break;
		case NTV2_FORMAT_4096x2160p_4800:
		case NTV2_FORMAT_4x2048x1080p_4800:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_4800_A; break;
		case NTV2_FORMAT_4096x2160p_5000:
		case NTV2_FORMAT_4x2048x1080p_5000:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_5000_A; break;
		case NTV2_FORMAT_4096x2160p_5994:
		case NTV2_FORMAT_4x2048x1080p_5994:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_5994_A; break;
		case NTV2_FORMAT_4096x2160p_6000:
		case NTV2_FORMAT_4x2048x1080p_6000:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_6000_A; break;
		case NTV2_FORMAT_4096x2160p_4795_B:
		case NTV2_FORMAT_4x2048x1080p_4795_B:	quarterSizedFormat = NTV2_FORMAT_1080p_2K_4795_B; break;
		case NTV2_FORMAT_4096x2160p_4800_B:
		case NTV2_FORMAT_4x2048x1080p_4800_B:	quarterSizedFormat = NTV2_FORMAT_1080p_2K_4800_B; break;
		case NTV2_FORMAT_4096x2160p_5000_B:
		case NTV2_FORMAT_4x2048x1080p_5000_B:	quarterSizedFormat = NTV2_FORMAT_1080p_2K_5000_B; break;
		case NTV2_FORMAT_4096x2160p_5994_B:
		case NTV2_FORMAT_4x2048x1080p_5994_B:	quarterSizedFormat = NTV2_FORMAT_1080p_2K_5994_B; break;
		case NTV2_FORMAT_4096x2160p_6000_B:
		case NTV2_FORMAT_4x2048x1080p_6000_B:	quarterSizedFormat = NTV2_FORMAT_1080p_2K_6000_B; break;
		// No quarter sized formats for 119.88 or 120 Hz

		case NTV2_FORMAT_4x3840x2160p_2398:		quarterSizedFormat = NTV2_FORMAT_3840x2160p_2398; break;
		case NTV2_FORMAT_4x3840x2160p_2400:		quarterSizedFormat = NTV2_FORMAT_3840x2160p_2400; break;
		case NTV2_FORMAT_4x3840x2160p_2500:		quarterSizedFormat = NTV2_FORMAT_3840x2160p_2500; break;
		case NTV2_FORMAT_4x3840x2160p_2997:		quarterSizedFormat = NTV2_FORMAT_3840x2160p_2997; break;
		case NTV2_FORMAT_4x3840x2160p_3000:		quarterSizedFormat = NTV2_FORMAT_3840x2160p_3000; break;
		case NTV2_FORMAT_4x3840x2160p_5000:		quarterSizedFormat = NTV2_FORMAT_3840x2160p_5000; break;
		case NTV2_FORMAT_4x3840x2160p_5994:		quarterSizedFormat = NTV2_FORMAT_3840x2160p_5994; break;
		case NTV2_FORMAT_4x3840x2160p_6000:		quarterSizedFormat = NTV2_FORMAT_3840x2160p_6000; break;
		case NTV2_FORMAT_4x3840x2160p_5000_B:	quarterSizedFormat = NTV2_FORMAT_3840x2160p_5000_B; break;
		case NTV2_FORMAT_4x3840x2160p_5994_B:	quarterSizedFormat = NTV2_FORMAT_3840x2160p_5994_B; break;
		case NTV2_FORMAT_4x3840x2160p_6000_B:	quarterSizedFormat = NTV2_FORMAT_3840x2160p_6000_B; break;

		case NTV2_FORMAT_4x4096x2160p_2398:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_2398; break;
		case NTV2_FORMAT_4x4096x2160p_2400:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_2400; break;
		case NTV2_FORMAT_4x4096x2160p_2500:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_2500; break;
		case NTV2_FORMAT_4x4096x2160p_2997:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_2997; break;
		case NTV2_FORMAT_4x4096x2160p_3000:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_3000; break;
		case NTV2_FORMAT_4x4096x2160p_4795:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_4795; break;
		case NTV2_FORMAT_4x4096x2160p_4800:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_4800; break;
		case NTV2_FORMAT_4x4096x2160p_5000:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_5000; break;
		case NTV2_FORMAT_4x4096x2160p_5994:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_5994; break;
		case NTV2_FORMAT_4x4096x2160p_6000:		quarterSizedFormat = NTV2_FORMAT_4096x2160p_6000; break;
		case NTV2_FORMAT_4x4096x2160p_4795_B:	quarterSizedFormat = NTV2_FORMAT_4096x2160p_4795_B; break;
		case NTV2_FORMAT_4x4096x2160p_4800_B:	quarterSizedFormat = NTV2_FORMAT_4096x2160p_4800_B; break;
		case NTV2_FORMAT_4x4096x2160p_5000_B:	quarterSizedFormat = NTV2_FORMAT_4096x2160p_5000_B; break;
		case NTV2_FORMAT_4x4096x2160p_5994_B:	quarterSizedFormat = NTV2_FORMAT_4096x2160p_5994_B; break;
		case NTV2_FORMAT_4x4096x2160p_6000_B:	quarterSizedFormat = NTV2_FORMAT_4096x2160p_6000_B; break;
#if defined(_DEBUG)
		case NTV2_FORMAT_UNKNOWN:
		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_1080i_5994:
		case NTV2_FORMAT_1080i_6000:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
		case NTV2_FORMAT_1080psf_2398:
		case NTV2_FORMAT_1080psf_2400:
		case NTV2_FORMAT_1080p_2997:
		case NTV2_FORMAT_1080p_3000:
		case NTV2_FORMAT_1080p_2500:
		case NTV2_FORMAT_1080p_2398:
		case NTV2_FORMAT_1080p_2400:
		case NTV2_FORMAT_1080p_2K_2398:
		case NTV2_FORMAT_1080p_2K_2400:
		case NTV2_FORMAT_1080psf_2K_2398:
		case NTV2_FORMAT_1080psf_2K_2400:
		case NTV2_FORMAT_720p_5000:
		case NTV2_FORMAT_1080p_5000_B:
		case NTV2_FORMAT_1080p_5994_B:
		case NTV2_FORMAT_1080p_6000_B:
		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_720p_2500:
		case NTV2_FORMAT_1080p_5000_A:
		case NTV2_FORMAT_1080p_5994_A:
		case NTV2_FORMAT_1080p_6000_A:
		case NTV2_FORMAT_1080p_2K_2500:
		case NTV2_FORMAT_1080psf_2K_2500:
		case NTV2_FORMAT_1080psf_2500_2:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
		case NTV2_FORMAT_525_5994:
		case NTV2_FORMAT_625_5000:
		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_525psf_2997:
		case NTV2_FORMAT_625psf_2500:
		case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:
		case NTV2_FORMAT_END_2K_DEF_FORMATS:
		case NTV2_FORMAT_4x2048x1080psf_2997:
		case NTV2_FORMAT_4x2048x1080psf_3000:
		case NTV2_FORMAT_4x2048x1080p_11988:
		case NTV2_FORMAT_4x2048x1080p_12000:
		case NTV2_FORMAT_1080p_2K_6000_A:
		case NTV2_FORMAT_1080p_2K_5994_A:
		case NTV2_FORMAT_1080p_2K_2997:
		case NTV2_FORMAT_1080p_2K_3000:
		case NTV2_FORMAT_1080p_2K_5000_A:
		case NTV2_FORMAT_1080p_2K_4795_A:
		case NTV2_FORMAT_1080p_2K_4800_A:
		case NTV2_FORMAT_1080p_2K_4795_B:
		case NTV2_FORMAT_1080p_2K_4800_B:
		case NTV2_FORMAT_1080p_2K_5000_B:
		case NTV2_FORMAT_1080p_2K_5994_B:
		case NTV2_FORMAT_1080p_2K_6000_B:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
		case NTV2_FORMAT_END_4K_DEF_FORMATS2:
		case NTV2_FORMAT_4096x2160p_11988:
		case NTV2_FORMAT_4096x2160p_12000:
		case NTV2_FORMAT_4096x2160psf_2997:
		case NTV2_FORMAT_4096x2160psf_3000:
		case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
		case NTV2_FORMAT_END_UHD2_DEF_FORMATS:
		case NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS:
#else
		default:
#endif
			break;
	}
	return quarterSizedFormat;
}


NTV2VideoFormat GetQuadSizedVideoFormat (const NTV2VideoFormat inVideoFormat, const bool isSquareDivision)
{
	switch (inVideoFormat)
	{
		case  NTV2_FORMAT_1080psf_2398:		return isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2398 : NTV2_FORMAT_3840x2160psf_2398;
		case  NTV2_FORMAT_1080psf_2400:		return isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2400 : NTV2_FORMAT_3840x2160psf_2400;
		case  NTV2_FORMAT_1080psf_2500_2:	return isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2500 : NTV2_FORMAT_3840x2160psf_2500;
		case  NTV2_FORMAT_1080i_5994:       return isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2997 : NTV2_FORMAT_3840x2160psf_2997;
		case  NTV2_FORMAT_1080i_6000:       return isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_3000 : NTV2_FORMAT_3840x2160psf_3000;

		case  NTV2_FORMAT_1080psf_2K_2398:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2398 : NTV2_FORMAT_4096x2160psf_2398;
		case  NTV2_FORMAT_1080psf_2K_2400:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2400 : NTV2_FORMAT_4096x2160psf_2400;
		case  NTV2_FORMAT_1080psf_2K_2500:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2500 : NTV2_FORMAT_4096x2160psf_2500;
		//case NTV2_FORMAT_1080psf_2K_2997:	return NTV2_FORMAT_4x2048x1080psf_29;
		//case NT2_FORMAT_1080psf_2K_3000:	return NTV2V2_FORMAT_4x2048x1080psf_3000;

		case  NTV2_FORMAT_1080p_2398:		return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2398 : NTV2_FORMAT_3840x2160p_2398;
		case  NTV2_FORMAT_1080p_2400: 		return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2400 : NTV2_FORMAT_3840x2160p_2400;
		case  NTV2_FORMAT_1080p_2500: 		return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2500 : NTV2_FORMAT_3840x2160p_2500;
		case  NTV2_FORMAT_1080p_2997: 		return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2997 : NTV2_FORMAT_3840x2160p_2997;
		case  NTV2_FORMAT_1080p_3000: 		return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_3000 : NTV2_FORMAT_3840x2160p_3000;
		case  NTV2_FORMAT_1080p_5000_A: 	return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5000 : NTV2_FORMAT_3840x2160p_5000;
		case  NTV2_FORMAT_1080p_5994_A: 	return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5994 : NTV2_FORMAT_3840x2160p_5994;
		case  NTV2_FORMAT_1080p_6000_A: 	return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_6000 : NTV2_FORMAT_3840x2160p_6000;
		case  NTV2_FORMAT_1080p_5000_B:		return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5000_B : NTV2_FORMAT_3840x2160p_5000_B;
		case  NTV2_FORMAT_1080p_5994_B:		return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5994_B : NTV2_FORMAT_3840x2160p_5994_B;
		case  NTV2_FORMAT_1080p_6000_B:		return isSquareDivision ? NTV2_FORMAT_4x1920x1080p_6000_B : NTV2_FORMAT_3840x2160p_6000_B;

		case  NTV2_FORMAT_1080p_2K_2398: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2398 : NTV2_FORMAT_4096x2160p_2398;
		case  NTV2_FORMAT_1080p_2K_2400: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2400 : NTV2_FORMAT_4096x2160p_2400;
		case  NTV2_FORMAT_1080p_2K_2500: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2500 : NTV2_FORMAT_4096x2160p_2500;
		case  NTV2_FORMAT_1080p_2K_2997: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2997 : NTV2_FORMAT_4096x2160p_2997;
		case  NTV2_FORMAT_1080p_2K_3000: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_3000 : NTV2_FORMAT_4096x2160p_3000;
		case  NTV2_FORMAT_1080p_2K_4795_A: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4795 : NTV2_FORMAT_4096x2160p_4795;
		case  NTV2_FORMAT_1080p_2K_4800_A: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4800 : NTV2_FORMAT_4096x2160p_4800;
		case  NTV2_FORMAT_1080p_2K_5000_A:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5000 : NTV2_FORMAT_4096x2160p_5000;
		case  NTV2_FORMAT_1080p_2K_5994_A:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5994 : NTV2_FORMAT_4096x2160p_5994;
		case  NTV2_FORMAT_1080p_2K_6000_A:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_6000 : NTV2_FORMAT_4096x2160p_6000;
		case  NTV2_FORMAT_1080p_2K_4795_B: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4795_B : NTV2_FORMAT_4096x2160p_4795_B;
		case  NTV2_FORMAT_1080p_2K_4800_B: 	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4800_B : NTV2_FORMAT_4096x2160p_4800_B;
		case  NTV2_FORMAT_1080p_2K_5000_B:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5000_B : NTV2_FORMAT_4096x2160p_5000_B;
		case  NTV2_FORMAT_1080p_2K_5994_B:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5994_B : NTV2_FORMAT_4096x2160p_5994_B;
		case  NTV2_FORMAT_1080p_2K_6000_B:	return isSquareDivision ? NTV2_FORMAT_4x2048x1080p_6000_B : NTV2_FORMAT_4096x2160p_6000_B;

		case NTV2_FORMAT_3840x2160p_2398:	return NTV2_FORMAT_4x3840x2160p_2398;
		case NTV2_FORMAT_3840x2160p_2400:	return NTV2_FORMAT_4x3840x2160p_2400;
		case NTV2_FORMAT_3840x2160p_2500:	return NTV2_FORMAT_4x3840x2160p_2500;
		case NTV2_FORMAT_3840x2160p_2997:	return NTV2_FORMAT_4x3840x2160p_2997;
		case NTV2_FORMAT_3840x2160p_3000:	return NTV2_FORMAT_4x3840x2160p_3000;
		case NTV2_FORMAT_3840x2160p_5000:	return NTV2_FORMAT_4x3840x2160p_5000;
		case NTV2_FORMAT_3840x2160p_5994:	return NTV2_FORMAT_4x3840x2160p_5994;
		case NTV2_FORMAT_3840x2160p_6000:	return NTV2_FORMAT_4x3840x2160p_6000;
		case NTV2_FORMAT_3840x2160p_5000_B:	return NTV2_FORMAT_4x3840x2160p_5000_B;
		case NTV2_FORMAT_3840x2160p_5994_B:	return NTV2_FORMAT_4x3840x2160p_5994_B;
		case NTV2_FORMAT_3840x2160p_6000_B:	return NTV2_FORMAT_4x3840x2160p_6000_B;

		case NTV2_FORMAT_4096x2160p_2398:	return NTV2_FORMAT_4x4096x2160p_2398;
		case NTV2_FORMAT_4096x2160p_2400:	return NTV2_FORMAT_4x4096x2160p_2400;
		case NTV2_FORMAT_4096x2160p_2500:	return NTV2_FORMAT_4x4096x2160p_2500;
		case NTV2_FORMAT_4096x2160p_2997:	return NTV2_FORMAT_4x4096x2160p_2997;
		case NTV2_FORMAT_4096x2160p_3000:	return NTV2_FORMAT_4x4096x2160p_3000;
		case NTV2_FORMAT_4096x2160p_4795:	return NTV2_FORMAT_4x4096x2160p_4795;
		case NTV2_FORMAT_4096x2160p_4800:	return NTV2_FORMAT_4x4096x2160p_4800;
		case NTV2_FORMAT_4096x2160p_5000:	return NTV2_FORMAT_4x4096x2160p_5000;
		case NTV2_FORMAT_4096x2160p_5994:	return NTV2_FORMAT_4x4096x2160p_5994;
		case NTV2_FORMAT_4096x2160p_6000:	return NTV2_FORMAT_4x4096x2160p_6000;
		case NTV2_FORMAT_4096x2160p_4795_B:	return NTV2_FORMAT_4x4096x2160p_4795_B;
		case NTV2_FORMAT_4096x2160p_4800_B:	return NTV2_FORMAT_4x4096x2160p_4800_B;
		case NTV2_FORMAT_4096x2160p_5000_B:	return NTV2_FORMAT_4x4096x2160p_5000_B;
		case NTV2_FORMAT_4096x2160p_5994_B:	return NTV2_FORMAT_4x4096x2160p_5994_B;
		case NTV2_FORMAT_4096x2160p_6000_B:	return NTV2_FORMAT_4x4096x2160p_6000_B;

#if defined(_DEBUG)
		case NTV2_FORMAT_UNKNOWN:
		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
		case NTV2_FORMAT_720p_5000:
		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_720p_2500:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
		case NTV2_FORMAT_525_5994:
		case NTV2_FORMAT_625_5000:
		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_525psf_2997:
		case NTV2_FORMAT_625psf_2500:
		case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:
		case NTV2_FORMAT_END_2K_DEF_FORMATS:
		case NTV2_FORMAT_4x2048x1080psf_2997:
		case NTV2_FORMAT_4x2048x1080psf_3000:
		case NTV2_FORMAT_4x2048x1080p_11988:
		case NTV2_FORMAT_4x2048x1080p_12000:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
		case NTV2_FORMAT_END_4K_DEF_FORMATS2:
		case NTV2_FORMAT_4096x2160p_11988:
		case NTV2_FORMAT_4096x2160p_12000:
		case NTV2_FORMAT_4096x2160psf_2997:
		case NTV2_FORMAT_4096x2160psf_3000:
		case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
		case NTV2_FORMAT_FIRST_4K_DEF_FORMAT:
		case NTV2_FORMAT_4x1920x1080psf_2400:
		case NTV2_FORMAT_4x1920x1080psf_2500:
		case NTV2_FORMAT_4x1920x1080p_2398:
		case NTV2_FORMAT_4x1920x1080p_2400:
		case NTV2_FORMAT_4x1920x1080p_2500:
		case NTV2_FORMAT_4x2048x1080psf_2398:
		case NTV2_FORMAT_4x2048x1080psf_2400:
		case NTV2_FORMAT_4x2048x1080psf_2500:
		case NTV2_FORMAT_4x2048x1080p_2398:
		case NTV2_FORMAT_4x2048x1080p_2400:
		case NTV2_FORMAT_4x2048x1080p_2500:
		case NTV2_FORMAT_4x1920x1080p_2997:
		case NTV2_FORMAT_4x1920x1080p_3000:
		case NTV2_FORMAT_4x1920x1080psf_2997:
		case NTV2_FORMAT_4x1920x1080psf_3000:
		case NTV2_FORMAT_4x2048x1080p_2997:
		case NTV2_FORMAT_4x2048x1080p_3000:
		case NTV2_FORMAT_4x1920x1080p_5000:
		case NTV2_FORMAT_4x1920x1080p_5994:
		case NTV2_FORMAT_4x1920x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_5000:
		case NTV2_FORMAT_4x2048x1080p_5994:
		case NTV2_FORMAT_4x2048x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_4795:
		case NTV2_FORMAT_4x2048x1080p_4800:
		case NTV2_FORMAT_FIRST_UHD_TSI_DEF_FORMAT:
		case NTV2_FORMAT_3840x2160psf_2400:
		case NTV2_FORMAT_3840x2160psf_2500:
		case NTV2_FORMAT_3840x2160psf_2997:
		case NTV2_FORMAT_3840x2160psf_3000:
		case NTV2_FORMAT_FIRST_4K_TSI_DEF_FORMAT:
		case NTV2_FORMAT_4096x2160psf_2400:
		case NTV2_FORMAT_4096x2160psf_2500:
		case NTV2_FORMAT_FIRST_4K_DEF_FORMAT2:
		case NTV2_FORMAT_4x1920x1080p_5994_B:
		case NTV2_FORMAT_4x1920x1080p_6000_B:
		case NTV2_FORMAT_4x2048x1080p_5000_B:
		case NTV2_FORMAT_4x2048x1080p_5994_B:
		case NTV2_FORMAT_4x2048x1080p_6000_B:
		case NTV2_FORMAT_4x2048x1080p_4795_B:
		case NTV2_FORMAT_4x2048x1080p_4800_B:
		case NTV2_FORMAT_FIRST_UHD2_DEF_FORMAT:
		case NTV2_FORMAT_4x3840x2160p_2400:
		case NTV2_FORMAT_4x3840x2160p_2500:
		case NTV2_FORMAT_4x3840x2160p_2997:
		case NTV2_FORMAT_4x3840x2160p_3000:
		case NTV2_FORMAT_4x3840x2160p_5000:
		case NTV2_FORMAT_4x3840x2160p_5994:
		case NTV2_FORMAT_4x3840x2160p_6000:
		case NTV2_FORMAT_4x3840x2160p_5000_B:
		case NTV2_FORMAT_4x3840x2160p_5994_B:
		case NTV2_FORMAT_4x3840x2160p_6000_B:
		case NTV2_FORMAT_FIRST_UHD2_FULL_DEF_FORMAT:
		case NTV2_FORMAT_4x4096x2160p_2400:
		case NTV2_FORMAT_4x4096x2160p_2500:
		case NTV2_FORMAT_4x4096x2160p_2997:
		case NTV2_FORMAT_4x4096x2160p_3000:
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
		case NTV2_FORMAT_END_UHD2_DEF_FORMATS:
		case NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS:
#else
		default:
#endif
			break;
	}
	return inVideoFormat;
}

NTV2FrameGeometry GetQuarterSizedGeometry (const NTV2FrameGeometry inGeometry)
{
	switch (inGeometry)
	{
		case NTV2_FG_4x1920x1080:	return NTV2_FG_1920x1080;
		case NTV2_FG_4x2048x1080:	return NTV2_FG_2048x1080;
		case NTV2_FG_4x3840x2160:	return NTV2_FG_4x1920x1080;
		case NTV2_FG_4x4096x2160:	return NTV2_FG_4x2048x1080;
		default:					return inGeometry;
	}
}


NTV2FrameGeometry Get4xSizedGeometry (const NTV2FrameGeometry inGeometry)
{
	switch (inGeometry)
	{
		case NTV2_FG_1920x1080:		return NTV2_FG_4x1920x1080;
		case NTV2_FG_2048x1080:		return NTV2_FG_4x2048x1080;
		case NTV2_FG_4x1920x1080:	return NTV2_FG_4x3840x2160;
		case NTV2_FG_4x2048x1080:	return NTV2_FG_4x4096x2160;
		default:					return inGeometry;
	}
}

NTV2Standard GetQuarterSizedStandard (const NTV2Standard inStandard)
{
	switch (inStandard)
	{
		case NTV2_STANDARD_3840x2160p:
		case NTV2_STANDARD_3840HFR:		return NTV2_STANDARD_1080p;
		case NTV2_STANDARD_3840i:		return NTV2_STANDARD_1080;
		case NTV2_STANDARD_4096x2160p:
		case NTV2_STANDARD_4096HFR:		return NTV2_STANDARD_2Kx1080p;
		case NTV2_STANDARD_4096i:		return NTV2_STANDARD_2Kx1080i;
		case NTV2_STANDARD_7680:		return NTV2_STANDARD_3840x2160p;
		case NTV2_STANDARD_8192:		return NTV2_STANDARD_4096x2160p;
		default:						return inStandard;
	}
}


NTV2Standard Get4xSizedStandard (const NTV2Standard inStandard, const bool bIs4k)
{
	switch (inStandard)
	{
		case NTV2_STANDARD_1080:		return bIs4k ? NTV2_STANDARD_4096i : NTV2_STANDARD_3840i;
		case NTV2_STANDARD_1080p:		return bIs4k ? NTV2_STANDARD_4096x2160p : NTV2_STANDARD_3840x2160p;

		case NTV2_STANDARD_3840HFR:
		case NTV2_STANDARD_3840x2160p:	return NTV2_STANDARD_7680;

		case NTV2_STANDARD_4096HFR:
		case NTV2_STANDARD_4096x2160p:	return NTV2_STANDARD_8192;

		default:						return inStandard;
	}
}


NTV2Standard GetNTV2StandardFromVideoFormat (const NTV2VideoFormat inVideoFormat)
{
	NTV2Standard standard = NTV2_STANDARD_INVALID;
	
	switch (inVideoFormat)
    {
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080i_6000:
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2500_2:
	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_1080p_2K_4795_B:
	case NTV2_FORMAT_1080p_2K_4800_B:
	case NTV2_FORMAT_1080p_2K_5000_B:
	case NTV2_FORMAT_1080p_2K_5994_B:
	case NTV2_FORMAT_1080p_2K_6000_B:
		standard = NTV2_STANDARD_1080;
		break;
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_6000_A:
		standard = NTV2_STANDARD_1080p;
		break;
	case NTV2_FORMAT_1080p_2K_2398:
	case NTV2_FORMAT_1080p_2K_2400:
	case NTV2_FORMAT_1080p_2K_2500:
	case NTV2_FORMAT_1080p_2K_2997:
	case NTV2_FORMAT_1080p_2K_3000:
	case NTV2_FORMAT_1080p_2K_4795_A:
	case NTV2_FORMAT_1080p_2K_4800_A:
	case NTV2_FORMAT_1080p_2K_5000_A:
	case NTV2_FORMAT_1080p_2K_5994_A:
	case NTV2_FORMAT_1080p_2K_6000_A:
		standard = NTV2_STANDARD_2Kx1080p;
		break;
	case NTV2_FORMAT_1080psf_2K_2398:
	case NTV2_FORMAT_1080psf_2K_2400:
	case NTV2_FORMAT_1080psf_2K_2500:
		standard = NTV2_STANDARD_2Kx1080p;
		break;
	case NTV2_FORMAT_720p_2398:
	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_720p_6000:
	case NTV2_FORMAT_720p_2500:
		standard = NTV2_STANDARD_720;
		break;
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_525_2398:
	case NTV2_FORMAT_525_2400:
	case NTV2_FORMAT_525psf_2997:
		standard = NTV2_STANDARD_525;
		break;
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_625psf_2500:
		standard = NTV2_STANDARD_625 ;
		break;
	case NTV2_FORMAT_2K_1498:
	case NTV2_FORMAT_2K_1500:
	case NTV2_FORMAT_2K_2398:
	case NTV2_FORMAT_2K_2400:
	case NTV2_FORMAT_2K_2500:
		standard = NTV2_STANDARD_2K ;
		break;
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_4x1920x1080p_2400:
	case NTV2_FORMAT_4x1920x1080p_2500:
	case NTV2_FORMAT_4x1920x1080p_2997:
	case NTV2_FORMAT_4x1920x1080p_3000:
	case NTV2_FORMAT_3840x2160p_2398:
	case NTV2_FORMAT_3840x2160p_2400:
	case NTV2_FORMAT_3840x2160p_2500:
	case NTV2_FORMAT_3840x2160p_2997:
	case NTV2_FORMAT_3840x2160p_3000:
		standard = NTV2_STANDARD_3840x2160p;
		break;
	case NTV2_FORMAT_4x1920x1080psf_2398:
	case NTV2_FORMAT_4x1920x1080psf_2400:
	case NTV2_FORMAT_4x1920x1080psf_2500:
	case NTV2_FORMAT_4x1920x1080psf_2997:
	case NTV2_FORMAT_4x1920x1080psf_3000:
	case NTV2_FORMAT_3840x2160psf_2398:
	case NTV2_FORMAT_3840x2160psf_2400:
	case NTV2_FORMAT_3840x2160psf_2500:
	case NTV2_FORMAT_3840x2160psf_2997:
	case NTV2_FORMAT_3840x2160psf_3000:
		standard = NTV2_STANDARD_3840x2160p;
		break;		
	case NTV2_FORMAT_4x1920x1080p_5000:
	case NTV2_FORMAT_4x1920x1080p_5994:
	case NTV2_FORMAT_4x1920x1080p_6000:
	case NTV2_FORMAT_3840x2160p_5000:
	case NTV2_FORMAT_3840x2160p_5994:
	case NTV2_FORMAT_3840x2160p_6000:
		standard = NTV2_STANDARD_3840HFR;
		break;
	case NTV2_FORMAT_4x2048x1080p_2398:
	case NTV2_FORMAT_4x2048x1080p_2400:
	case NTV2_FORMAT_4x2048x1080p_2500:
	case NTV2_FORMAT_4x2048x1080p_2997:
	case NTV2_FORMAT_4x2048x1080p_3000:
	case NTV2_FORMAT_4x2048x1080p_4795:
	case NTV2_FORMAT_4x2048x1080p_4800:
	case NTV2_FORMAT_4096x2160p_2398:
	case NTV2_FORMAT_4096x2160p_2400:
	case NTV2_FORMAT_4096x2160p_2500:
	case NTV2_FORMAT_4096x2160p_2997:
	case NTV2_FORMAT_4096x2160p_3000:
	case NTV2_FORMAT_4096x2160p_4795:
	case NTV2_FORMAT_4096x2160p_4800:
		standard = NTV2_STANDARD_4096x2160p;
		break;
	case NTV2_FORMAT_4x2048x1080psf_2398:
	case NTV2_FORMAT_4x2048x1080psf_2400:
	case NTV2_FORMAT_4x2048x1080psf_2500:
	case NTV2_FORMAT_4x2048x1080psf_2997:
	case NTV2_FORMAT_4x2048x1080psf_3000:
	case NTV2_FORMAT_4096x2160psf_2398:
	case NTV2_FORMAT_4096x2160psf_2400:
	case NTV2_FORMAT_4096x2160psf_2500:
	case NTV2_FORMAT_4096x2160psf_2997:
	case NTV2_FORMAT_4096x2160psf_3000:
		standard = NTV2_STANDARD_4096x2160p;
		break;
	case NTV2_FORMAT_4x2048x1080p_5000:
	case NTV2_FORMAT_4x2048x1080p_5994:
	case NTV2_FORMAT_4x2048x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_11988:
	case NTV2_FORMAT_4x2048x1080p_12000:
	case NTV2_FORMAT_4096x2160p_5000:
	case NTV2_FORMAT_4096x2160p_5994:
	case NTV2_FORMAT_4096x2160p_6000:
	case NTV2_FORMAT_4096x2160p_11988:
	case NTV2_FORMAT_4096x2160p_12000:
		standard = NTV2_STANDARD_4096HFR;
		break;
		
	case NTV2_FORMAT_4x3840x2160p_2398:
	case NTV2_FORMAT_4x3840x2160p_2400:
	case NTV2_FORMAT_4x3840x2160p_2500:
	case NTV2_FORMAT_4x3840x2160p_2997:
	case NTV2_FORMAT_4x3840x2160p_3000:
	case NTV2_FORMAT_4x3840x2160p_5000:
	case NTV2_FORMAT_4x3840x2160p_5994:
	case NTV2_FORMAT_4x3840x2160p_6000:
		standard = NTV2_STANDARD_7680;
		break;
		
	case NTV2_FORMAT_4x4096x2160p_2398:
	case NTV2_FORMAT_4x4096x2160p_2400:
	case NTV2_FORMAT_4x4096x2160p_2500:
	case NTV2_FORMAT_4x4096x2160p_2997:
	case NTV2_FORMAT_4x4096x2160p_3000:
	case NTV2_FORMAT_4x4096x2160p_4795:
	case NTV2_FORMAT_4x4096x2160p_4800:
	case NTV2_FORMAT_4x4096x2160p_5000:
	case NTV2_FORMAT_4x4096x2160p_5994:
	case NTV2_FORMAT_4x4096x2160p_6000:
		standard = NTV2_STANDARD_8192;
		break;
				
				
#if defined (_DEBUG)
//	Debug builds warn about missing values
	case NTV2_FORMAT_UNKNOWN:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
	case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
	case NTV2_FORMAT_END_2K_DEF_FORMATS:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
	case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
	case NTV2_FORMAT_FIRST_4K_DEF_FORMAT2:
	case NTV2_FORMAT_END_4K_DEF_FORMATS2:
	case NTV2_FORMAT_END_UHD2_DEF_FORMATS:
	case NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS:
	case NTV2_FORMAT_3840x2160p_5000_B:
	case NTV2_FORMAT_3840x2160p_5994_B:
	case NTV2_FORMAT_3840x2160p_6000_B:
	case NTV2_FORMAT_4096x2160p_4795_B:
	case NTV2_FORMAT_4096x2160p_4800_B:
	case NTV2_FORMAT_4096x2160p_5000_B:
	case NTV2_FORMAT_4096x2160p_5994_B:
	case NTV2_FORMAT_4096x2160p_6000_B:
	case NTV2_FORMAT_4x1920x1080p_5994_B:
	case NTV2_FORMAT_4x1920x1080p_6000_B:
	case NTV2_FORMAT_4x2048x1080p_4795_B:
	case NTV2_FORMAT_4x2048x1080p_4800_B:
	case NTV2_FORMAT_4x2048x1080p_5000_B:
	case NTV2_FORMAT_4x2048x1080p_5994_B:
	case NTV2_FORMAT_4x2048x1080p_6000_B:
	case NTV2_FORMAT_4x3840x2160p_5000_B:
	case NTV2_FORMAT_4x3840x2160p_5994_B:
	case NTV2_FORMAT_4x3840x2160p_6000_B:
	case NTV2_FORMAT_4x4096x2160p_4795_B:
	case NTV2_FORMAT_4x4096x2160p_4800_B:
	case NTV2_FORMAT_4x4096x2160p_5000_B:
	case NTV2_FORMAT_4x4096x2160p_5994_B:
	case NTV2_FORMAT_4x4096x2160p_6000_B:
		break;	// Unsupported
#else
	default:
		break;
#endif
    }
	
	return standard;
}


//-------------------------------------------------------------------------------------------------------
//	GetSupportedNTV2VideoFormatFromInputVideoFormat
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat GetSupportedNTV2VideoFormatFromInputVideoFormat(const NTV2VideoFormat inVideoFormat)
{
	NTV2VideoFormat result;
	
	switch (inVideoFormat)
	{
		case NTV2_FORMAT_3840x2160p_5000_B:  result = NTV2_FORMAT_3840x2160p_5000; 		break;
		case NTV2_FORMAT_3840x2160p_5994_B:  result = NTV2_FORMAT_3840x2160p_5994; 		break;
		case NTV2_FORMAT_3840x2160p_6000_B:  result = NTV2_FORMAT_3840x2160p_6000; 		break;
		
		case NTV2_FORMAT_4096x2160p_4795_B:  result = NTV2_FORMAT_4096x2160p_4795; 		break;
		case NTV2_FORMAT_4096x2160p_4800_B:  result = NTV2_FORMAT_4096x2160p_4800; 		break;
		case NTV2_FORMAT_4096x2160p_5000_B:  result = NTV2_FORMAT_4096x2160p_5000; 		break;
		case NTV2_FORMAT_4096x2160p_5994_B:  result = NTV2_FORMAT_4096x2160p_5994; 		break;
		case NTV2_FORMAT_4096x2160p_6000_B:  result = NTV2_FORMAT_4096x2160p_6000; 		break;
		
		case NTV2_FORMAT_4x1920x1080p_5000_B:  result = NTV2_FORMAT_4x1920x1080p_5000; 	break;
		case NTV2_FORMAT_4x1920x1080p_5994_B:  result = NTV2_FORMAT_4x1920x1080p_5994; 	break;
		case NTV2_FORMAT_4x1920x1080p_6000_B:  result = NTV2_FORMAT_4x1920x1080p_6000; 	break;
		
		case NTV2_FORMAT_4x2048x1080p_4795_B:  result = NTV2_FORMAT_4x2048x1080p_4795; 	break;
		case NTV2_FORMAT_4x2048x1080p_4800_B:  result = NTV2_FORMAT_4x2048x1080p_4800; 	break;
		case NTV2_FORMAT_4x2048x1080p_5000_B:  result = NTV2_FORMAT_4x2048x1080p_5000; 	break;
		case NTV2_FORMAT_4x2048x1080p_5994_B:  result = NTV2_FORMAT_4x2048x1080p_5994; 	break;
		case NTV2_FORMAT_4x2048x1080p_6000_B:  result = NTV2_FORMAT_4x2048x1080p_6000; 	break;
		
		case NTV2_FORMAT_4x3840x2160p_5000_B:  result = NTV2_FORMAT_4x3840x2160p_5000; 	break;
		case NTV2_FORMAT_4x3840x2160p_5994_B:  result = NTV2_FORMAT_4x3840x2160p_5994; 	break;
		case NTV2_FORMAT_4x3840x2160p_6000_B:  result = NTV2_FORMAT_4x3840x2160p_6000; 	break;
		
		case NTV2_FORMAT_4x4096x2160p_4795_B:  result = NTV2_FORMAT_4x4096x2160p_4795; 	break;
		case NTV2_FORMAT_4x4096x2160p_4800_B:  result = NTV2_FORMAT_4x4096x2160p_4800; 	break;
		case NTV2_FORMAT_4x4096x2160p_5000_B:  result = NTV2_FORMAT_4x4096x2160p_5000; 	break;
		case NTV2_FORMAT_4x4096x2160p_5994_B:  result = NTV2_FORMAT_4x4096x2160p_5994; 	break;
		case NTV2_FORMAT_4x4096x2160p_6000_B:  result = NTV2_FORMAT_4x4096x2160p_6000; 	break;
		
		default: result = inVideoFormat; break;
	}
	
	return result;
}


//-------------------------------------------------------------------------------------------------------
//	GetNTV2FrameGeometryFromVideoFormat
//-------------------------------------------------------------------------------------------------------
NTV2FrameGeometry GetNTV2FrameGeometryFromVideoFormat(const NTV2VideoFormat inVideoFormat)
{
	NTV2FrameGeometry result = NTV2_FG_INVALID;

	switch (inVideoFormat)
	{	
	case NTV2_FORMAT_4x3840x2160p_2398:
	case NTV2_FORMAT_4x3840x2160p_2400:
	case NTV2_FORMAT_4x3840x2160p_2500:
	case NTV2_FORMAT_4x3840x2160p_2997:
	case NTV2_FORMAT_4x3840x2160p_3000:
	case NTV2_FORMAT_4x3840x2160p_5000:
	case NTV2_FORMAT_4x3840x2160p_5994:
	case NTV2_FORMAT_4x3840x2160p_6000:
	case NTV2_FORMAT_4x3840x2160p_5000_B:
	case NTV2_FORMAT_4x3840x2160p_5994_B:
	case NTV2_FORMAT_4x3840x2160p_6000_B:
		result = NTV2_FG_4x3840x2160;
		break;
		
	case NTV2_FORMAT_4x4096x2160p_2398:
	case NTV2_FORMAT_4x4096x2160p_2400:
	case NTV2_FORMAT_4x4096x2160p_2500:
	case NTV2_FORMAT_4x4096x2160p_2997:
	case NTV2_FORMAT_4x4096x2160p_3000:
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
		result = NTV2_FG_4x4096x2160;
		break;
			
	case NTV2_FORMAT_4x1920x1080psf_2398:
	case NTV2_FORMAT_4x1920x1080psf_2400:
	case NTV2_FORMAT_4x1920x1080psf_2500:
	case NTV2_FORMAT_4x1920x1080psf_2997:
	case NTV2_FORMAT_4x1920x1080psf_3000:
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_4x1920x1080p_2400:
	case NTV2_FORMAT_4x1920x1080p_2500:
	case NTV2_FORMAT_4x1920x1080p_2997:
	case NTV2_FORMAT_4x1920x1080p_3000:
	case NTV2_FORMAT_4x1920x1080p_5000:
	case NTV2_FORMAT_4x1920x1080p_5994:
	case NTV2_FORMAT_4x1920x1080p_6000:
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
	case NTV2_FORMAT_3840x2160p_5000:
	case NTV2_FORMAT_3840x2160p_5994:
	case NTV2_FORMAT_3840x2160p_6000:
	case NTV2_FORMAT_4x1920x1080p_5000_B:
	case NTV2_FORMAT_4x1920x1080p_5994_B:
	case NTV2_FORMAT_4x1920x1080p_6000_B:
	case NTV2_FORMAT_3840x2160p_5000_B:
	case NTV2_FORMAT_3840x2160p_5994_B:
	case NTV2_FORMAT_3840x2160p_6000_B:
		result = NTV2_FG_4x1920x1080;
		break;

	case NTV2_FORMAT_4x2048x1080psf_2398:
	case NTV2_FORMAT_4x2048x1080psf_2400:
	case NTV2_FORMAT_4x2048x1080psf_2500:
	case NTV2_FORMAT_4x2048x1080p_2398:
	case NTV2_FORMAT_4x2048x1080p_2400:
	case NTV2_FORMAT_4x2048x1080p_2500:
	case NTV2_FORMAT_4x2048x1080p_2997:
	case NTV2_FORMAT_4x2048x1080p_3000:
	case NTV2_FORMAT_4x2048x1080psf_2997:
	case NTV2_FORMAT_4x2048x1080psf_3000:
	case NTV2_FORMAT_4x2048x1080p_4795:
	case NTV2_FORMAT_4x2048x1080p_4800:
	case NTV2_FORMAT_4x2048x1080p_5000:
	case NTV2_FORMAT_4x2048x1080p_5994:
	case NTV2_FORMAT_4x2048x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_11988:
	case NTV2_FORMAT_4x2048x1080p_12000:
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
	case NTV2_FORMAT_4096x2160p_4795:
	case NTV2_FORMAT_4096x2160p_4800:
	case NTV2_FORMAT_4096x2160p_5000:
	case NTV2_FORMAT_4096x2160p_5994:
	case NTV2_FORMAT_4096x2160p_6000:
	case NTV2_FORMAT_4096x2160p_11988:
	case NTV2_FORMAT_4096x2160p_12000:
	case NTV2_FORMAT_4x2048x1080p_4795_B:
	case NTV2_FORMAT_4x2048x1080p_4800_B:
	case NTV2_FORMAT_4x2048x1080p_5000_B:
	case NTV2_FORMAT_4x2048x1080p_5994_B:
	case NTV2_FORMAT_4x2048x1080p_6000_B:
	case NTV2_FORMAT_4096x2160p_4795_B:
	case NTV2_FORMAT_4096x2160p_4800_B:
	case NTV2_FORMAT_4096x2160p_5000_B:
	case NTV2_FORMAT_4096x2160p_5994_B:
	case NTV2_FORMAT_4096x2160p_6000_B:
		result = NTV2_FG_4x2048x1080;
		break;

	case NTV2_FORMAT_2K_1498:
	case NTV2_FORMAT_2K_1500:
	case NTV2_FORMAT_2K_2398:
	case NTV2_FORMAT_2K_2400:
	case NTV2_FORMAT_2K_2500:
		result = NTV2_FG_2048x1556;
		break;

	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080i_6000:
	case NTV2_FORMAT_1080psf_2500_2:
	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_6000_A:
		result = NTV2_FG_1920x1080;
		break;

	case NTV2_FORMAT_1080p_2K_2398:
	case NTV2_FORMAT_1080p_2K_2400:
	case NTV2_FORMAT_1080p_2K_2500:
	case NTV2_FORMAT_1080psf_2K_2398:
	case NTV2_FORMAT_1080psf_2K_2400:
	case NTV2_FORMAT_1080psf_2K_2500:
	case NTV2_FORMAT_1080p_2K_2997:
	case NTV2_FORMAT_1080p_2K_3000:
	case NTV2_FORMAT_1080p_2K_4795_A:
	case NTV2_FORMAT_1080p_2K_4795_B:
	case NTV2_FORMAT_1080p_2K_4800_A:
	case NTV2_FORMAT_1080p_2K_4800_B:
	case NTV2_FORMAT_1080p_2K_5000_A:
	case NTV2_FORMAT_1080p_2K_5000_B:
	case NTV2_FORMAT_1080p_2K_5994_A:
	case NTV2_FORMAT_1080p_2K_5994_B:
	case NTV2_FORMAT_1080p_2K_6000_A:
	case NTV2_FORMAT_1080p_2K_6000_B:
		result = NTV2_FG_2048x1080;
		break;

	case NTV2_FORMAT_720p_2398:
	case NTV2_FORMAT_720p_2500:
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_720p_6000:
	case NTV2_FORMAT_720p_5000:
		result = NTV2_FG_1280x720;
		break;

	case NTV2_FORMAT_525_2398:
	case NTV2_FORMAT_525_2400:
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_525psf_2997:
		result = NTV2_FG_720x486;
		break;

	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_625psf_2500:
		result = NTV2_FG_720x576;
		break;

#if defined (_DEBUG)
//	Debug builds warn about missing values
	case NTV2_FORMAT_UNKNOWN:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
	case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
	case NTV2_FORMAT_END_2K_DEF_FORMATS:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
	case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
	case NTV2_FORMAT_END_4K_DEF_FORMATS2:
	case NTV2_FORMAT_END_UHD2_DEF_FORMATS:
	case NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS:
		break;	// Unsupported
#else
	default:
		break;
#endif
	}

	return result;
}


#if !defined(NTV2_DEPRECATE_13_0)
	// GetVideoActiveSize: returns the number of bytes of active video (including VANC lines, if any)
	ULWord GetVideoActiveSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat, const bool inTallVANC, const bool inTallerVANC)
	{
		return GetVideoActiveSize (inVideoFormat, inFBFormat, NTV2VANCModeFromBools (inTallVANC, inTallerVANC));
	}	//	GetVideoActiveSize

	ULWord GetVideoWriteSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat, const bool inTallVANC, const bool inTallerVANC)
	{
		return ::GetVideoWriteSize (inVideoFormat, inFBFormat, NTV2VANCModeFromBools (inTallVANC, inTallerVANC));
	}
#endif	//	!defined(NTV2_DEPRECATE_13_0)


ULWord GetVideoActiveSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat, const NTV2VANCMode inVancMode)
{
	const NTV2FormatDescriptor fd (inVideoFormat, inFBFormat, inVancMode);
	return fd.GetTotalBytes();
}	//	GetVideoActiveSize


// GetVideoWriteSize
// At least in Windows, to get bursting to work between our board and the disk
// system  without going through the file manager cache, you need to open the file
// with FILE_FLAG_NO_BUFFERING flag. With this you must do reads and writes
// on 4096 byte boundaries with most modern disk systems. You could actually
// do 512 on some systems though.
// So this function takes in the videoformat and the framebufferformat
// and gets the framesize you need to write to meet this requirement.
//

ULWord GetVideoWriteSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat, const NTV2VANCMode inVancMode)
{
	ULWord ulSize (::GetVideoActiveSize (inVideoFormat, inFBFormat, inVancMode));
	if (ulSize % 4096)
		ulSize = ((ulSize / 4096) + 1) * 4096;
	return ulSize;
}


// For a given framerate and audiorate, returns how many audio samples there
// will be in a frame's time. cadenceFrame is only used for 5994 or 2997 @ 48k.
// smpte372Enabled indicates that you are doing 1080p60,1080p5994 or 1080p50
// in this mode the boards framerate might be NTV2_FRAMERATE_3000, but since
// 2 links are coming out, the video rate is actually NTV2_FRAMERATE_6000
ULWord GetAudioSamplesPerFrame (const NTV2FrameRate inFrameRate, const NTV2AudioRate inAudioRate, ULWord inCadenceFrame, const bool inIsSMPTE372Enabled)
{
	NTV2FrameRate	frameRate(inFrameRate);
	ULWord			audioSamplesPerFrame(0);
	inCadenceFrame %= 5;

	if (inIsSMPTE372Enabled)
	{
		// the video is actually coming out twice as fast as the board rate
		// since there are 2 links.
		switch (inFrameRate)
		{
			case NTV2_FRAMERATE_3000:	frameRate = NTV2_FRAMERATE_6000;	break;
			case NTV2_FRAMERATE_2997:	frameRate = NTV2_FRAMERATE_5994;	break;
			case NTV2_FRAMERATE_2500:	frameRate = NTV2_FRAMERATE_5000;	break;
			case NTV2_FRAMERATE_2400:	frameRate = NTV2_FRAMERATE_4800;	break;
			case NTV2_FRAMERATE_2398:	frameRate = NTV2_FRAMERATE_4795;	break;
		default:
			break;
		}
	}

	if (inAudioRate == NTV2_AUDIO_48K)
	{
		switch (frameRate)
		{
			case NTV2_FRAMERATE_12000:
				audioSamplesPerFrame = 400;
				break;
			case NTV2_FRAMERATE_11988:
				switch (inCadenceFrame)
				{
					case 0:
					case 2:
					case 4:	audioSamplesPerFrame = 400;	break;
	
					case 1:
					case 3:	audioSamplesPerFrame = 401;	break;
				}
				break;
			case NTV2_FRAMERATE_6000:
				audioSamplesPerFrame = 800;
				break;
			case NTV2_FRAMERATE_5994:
				switch (inCadenceFrame)
				{
				case 0:	audioSamplesPerFrame = 800;	break;
	
				case 1:
				case 2:
				case 3:
				case 4:	audioSamplesPerFrame = 801;	break;
				}
				break;
			case NTV2_FRAMERATE_5000:	audioSamplesPerFrame = 1920/2;	break;
			case NTV2_FRAMERATE_4800:	audioSamplesPerFrame = 1000;	break;
			case NTV2_FRAMERATE_4795:	audioSamplesPerFrame = 1001;	break;
			case NTV2_FRAMERATE_3000:	audioSamplesPerFrame = 1600;	break;
			case NTV2_FRAMERATE_2997:
				// depends on cadenceFrame;
				switch (inCadenceFrame)
				{
					case 0:
					case 2:
					case 4:	audioSamplesPerFrame = 1602;	break;
	
					case 1:
					case 3:	audioSamplesPerFrame = 1601;	break;
				}
				break;
			case NTV2_FRAMERATE_2500:	audioSamplesPerFrame = 1920;	break;
			case NTV2_FRAMERATE_2400:	audioSamplesPerFrame = 2000;	break;
			case NTV2_FRAMERATE_2398:	audioSamplesPerFrame = 2002;	break;
			case NTV2_FRAMERATE_1500:	audioSamplesPerFrame = 3200;	break;
			case NTV2_FRAMERATE_1498:
				// depends on cadenceFrame;
				switch (inCadenceFrame)
				{
					case 0:	audioSamplesPerFrame = 3204;	break;
	
					case 1:
					case 2:
					case 3:
					case 4:	audioSamplesPerFrame = 3203;	break;
				}
				break;
	#if !defined(NTV2_DEPRECATE_16_0)
			case NTV2_FRAMERATE_1900:	// Not supported yet
			case NTV2_FRAMERATE_1898:	// Not supported yet
			case NTV2_FRAMERATE_1800: 	// Not supported yet
			case NTV2_FRAMERATE_1798:	// Not supported yet
	#endif	//!defined(NTV2_DEPRECATE_16_0)
			case NTV2_FRAMERATE_UNKNOWN:
			case NTV2_NUM_FRAMERATES:
				audioSamplesPerFrame = 0;
				break;
			}
	}
	else if (inAudioRate == NTV2_AUDIO_96K)
	{
		switch (frameRate)
		{
			case NTV2_FRAMERATE_12000:
				audioSamplesPerFrame = 800;
				break;
			case NTV2_FRAMERATE_11988:
				switch (inCadenceFrame)
				{
					case 0:
					case 1:
					case 2:
					case 3:	audioSamplesPerFrame = 901;	break;
	
					case 4:	audioSamplesPerFrame = 800;	break;
				}
				break;
			case NTV2_FRAMERATE_6000:	audioSamplesPerFrame = 800*2;	break;
			case NTV2_FRAMERATE_5994:
				switch (inCadenceFrame)
				{
					case 0:
					case 2:
					case 4:	audioSamplesPerFrame = 1602;	break;
	
					case 1:
					case 3:	audioSamplesPerFrame = 1601;	break;
				}
				break;
			case NTV2_FRAMERATE_5000:	audioSamplesPerFrame = 1920;	break;
			case NTV2_FRAMERATE_4800:	audioSamplesPerFrame = 2000;	break;
			case NTV2_FRAMERATE_4795:	audioSamplesPerFrame = 2002;	break;
			case NTV2_FRAMERATE_3000:	audioSamplesPerFrame = 1600*2;	break;
			case NTV2_FRAMERATE_2997:
				// depends on cadenceFrame;
				switch (inCadenceFrame)
				{
					case 0:	audioSamplesPerFrame = 3204;	break;
	
					case 1:
					case 2:
					case 3:
					case 4:	audioSamplesPerFrame = 3203;	break;
				}
				break;
			case NTV2_FRAMERATE_2500:	audioSamplesPerFrame = 1920*2;	break;
			case NTV2_FRAMERATE_2400:	audioSamplesPerFrame = 2000*2;	break;
			case NTV2_FRAMERATE_2398:	audioSamplesPerFrame = 2002*2;	break;
			case NTV2_FRAMERATE_1500:	audioSamplesPerFrame = 3200*2;	break;
			case NTV2_FRAMERATE_1498:
				// depends on cadenceFrame;
				switch (inCadenceFrame)
				{
					case 0:	audioSamplesPerFrame = 3204*2;	break;
	
					case 1:
					case 2:
					case 3:
					case 4:	audioSamplesPerFrame = 3203*2;	break;
				}
				break;
	#if !defined(NTV2_DEPRECATE_16_0)
			case NTV2_FRAMERATE_1900:	// Not supported yet
			case NTV2_FRAMERATE_1898:	// Not supported yet
			case NTV2_FRAMERATE_1800: 	// Not supported yet
			case NTV2_FRAMERATE_1798:	// Not supported yet
	#endif	//!defined(NTV2_DEPRECATE_16_0)
			case NTV2_FRAMERATE_UNKNOWN:
			case NTV2_NUM_FRAMERATES:
				audioSamplesPerFrame = 0*2; //haha
				break;
			}
	}
	else if (inAudioRate == NTV2_AUDIO_192K)
	{
		switch (frameRate)
		{
			case NTV2_FRAMERATE_12000:
				audioSamplesPerFrame = 1600;
				break;
			case NTV2_FRAMERATE_11988:
				switch (inCadenceFrame)
				{
					case 0:
					case 2:
					case 4:	audioSamplesPerFrame = 1602;	break;
	
					case 1:
					case 3:	audioSamplesPerFrame = 1601;	break;
				}
				break;
			case NTV2_FRAMERATE_6000:
				audioSamplesPerFrame = 3200;
				break;
			case NTV2_FRAMERATE_5994:
				switch (inCadenceFrame)
				{
					case 0:	audioSamplesPerFrame = 3204;	break;

					case 1:
					case 2:
					case 3:
					case 4:	audioSamplesPerFrame = 3203;	break;
				}
				break;
			case NTV2_FRAMERATE_5000:	audioSamplesPerFrame = 3840;	break;
			case NTV2_FRAMERATE_4800:	audioSamplesPerFrame = 4000;	break;
			case NTV2_FRAMERATE_4795:	audioSamplesPerFrame = 4004;	break;
			case NTV2_FRAMERATE_3000:	audioSamplesPerFrame = 6400;	break;
			case NTV2_FRAMERATE_2997:
				// depends on cadenceFrame;
				switch (inCadenceFrame)
				{
					case 0:
					case 1:	audioSamplesPerFrame = 6407;	break;
	
					case 2:
					case 3:
					case 4:	audioSamplesPerFrame = 6406;	break;
				}
				break;
			case NTV2_FRAMERATE_2500:	audioSamplesPerFrame = 7680;	break;
			case NTV2_FRAMERATE_2400:	audioSamplesPerFrame = 8000;	break;
			case NTV2_FRAMERATE_2398:	audioSamplesPerFrame = 8008;	break;
			case NTV2_FRAMERATE_1500:	audioSamplesPerFrame = 12800;	break;
			case NTV2_FRAMERATE_1498:
				// depends on cadenceFrame;
				switch (inCadenceFrame)
				{
					case 0:
					case 1:
					case 2:
					case 3:	audioSamplesPerFrame = 12813;	break;
	
					case 4:	audioSamplesPerFrame = 12812;	break;
				}
				break;
#if !defined(NTV2_DEPRECATE_16_0)
			case NTV2_FRAMERATE_1900:	// Not supported yet
			case NTV2_FRAMERATE_1898:	// Not supported yet
			case NTV2_FRAMERATE_1800: 	// Not supported yet
			case NTV2_FRAMERATE_1798:	// Not supported yet
#endif	//!defined(NTV2_DEPRECATE_16_0)
			case NTV2_FRAMERATE_UNKNOWN:
			case NTV2_NUM_FRAMERATES:
				audioSamplesPerFrame = 0*2; //haha
				break;
		}
	}

	return audioSamplesPerFrame;
}


// For a given framerate and audiorate and ending frame number (non-inclusive), returns the total number of audio samples over
// the range of video frames starting at frame number zero up to and not including the passed in frame number, inFrameNumNonInclusive.
// Utilizes cadence patterns in function immediately above,  GetAudioSamplesPerFrame().
// No smpte372Enabled support
LWord64 GetTotalAudioSamplesFromFrameNbrZeroUpToFrameNbr (const NTV2FrameRate inFrameRate, const NTV2AudioRate inAudioRate, const ULWord inFrameNumNonInclusive)
{
	LWord64 numTotalAudioSamples;
	LWord64 numAudioSamplesFromWholeGroups;

	ULWord numWholeGroupsOfFive;
	ULWord numAudioSamplesFromRemainder;
	ULWord remainder;

	numWholeGroupsOfFive = inFrameNumNonInclusive / 5;
	remainder = inFrameNumNonInclusive % 5;

	numTotalAudioSamples = 0;
	numAudioSamplesFromWholeGroups = 0;
	numAudioSamplesFromRemainder = 0;

	if (inAudioRate == NTV2_AUDIO_48K)
	{
		switch (inFrameRate)
		{
		case NTV2_FRAMERATE_12000:
			numTotalAudioSamples = 400 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_11988:
			numAudioSamplesFromWholeGroups = ((2*401) + (3*400)) * numWholeGroupsOfFive;
			numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((400 * remainder) + remainder/2);
			numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
			break;
		case NTV2_FRAMERATE_6000:
			numTotalAudioSamples = 800 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_5994:
			// depends on cadenceFrame;
			numAudioSamplesFromWholeGroups = ((1*800) + (4*801)) * numWholeGroupsOfFive;
			numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((801 * remainder) - 1);
			numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
			break;
		case NTV2_FRAMERATE_5000:
			numTotalAudioSamples = 1920/2 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_4800:
			numTotalAudioSamples = 1000 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_4795:
			numTotalAudioSamples = 1001 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_3000:
			numTotalAudioSamples = 1600 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_2997:
			// depends on cadenceFrame;
			numAudioSamplesFromWholeGroups = ((3*1602) + (2*1601)) * numWholeGroupsOfFive;
			numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((1602 * remainder) - remainder/2);
			numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
			break;
		case NTV2_FRAMERATE_2500:
			numTotalAudioSamples = 1920 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_2400:
			numTotalAudioSamples = 2000 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_2398:
			numTotalAudioSamples = 2002 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_1500:
			numTotalAudioSamples = 3200 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_1498:
			// depends on cadenceFrame;
			numAudioSamplesFromWholeGroups = ((1*3204) + (4*3203)) * numWholeGroupsOfFive;
			numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((3203 * remainder) + 1);
			numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
			break;
#if !defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_1900:	// Not supported yet
		case NTV2_FRAMERATE_1898:	// Not supported yet
		case NTV2_FRAMERATE_1800: 	// Not supported yet
		case NTV2_FRAMERATE_1798:	// Not supported yet
#endif	//!defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_UNKNOWN:
		case NTV2_NUM_FRAMERATES:
			numTotalAudioSamples = 0;
			break;
		}
	}
	else if (inAudioRate == NTV2_AUDIO_96K)
	{
		switch (inFrameRate)
		{
		case NTV2_FRAMERATE_12000:
			numTotalAudioSamples = 800 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_11988:
			numAudioSamplesFromWholeGroups = ((4*801) + (1*800)) * numWholeGroupsOfFive;
			numAudioSamplesFromRemainder = (remainder == 0) ? 0 : (801 * remainder);
			numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
			break;
		case NTV2_FRAMERATE_6000:
			numTotalAudioSamples = (800*2) * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_5994:
			numAudioSamplesFromWholeGroups = ((3*1602) + (2*1601)) * numWholeGroupsOfFive;
			numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((1602 * remainder) - remainder/2);
			numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
			break;
		case NTV2_FRAMERATE_5000:
			numTotalAudioSamples = 1920 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_4800:
			numTotalAudioSamples = 2000 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_4795:
			numTotalAudioSamples = 2002 * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_3000:
			numTotalAudioSamples = (1600*2) * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_2997:
			// depends on cadenceFrame;
			numAudioSamplesFromWholeGroups = ((1*3204) + (4*3203)) * numWholeGroupsOfFive;
			numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((3203 * remainder) + 1);
			numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
			break;
		case NTV2_FRAMERATE_2500:
			numTotalAudioSamples = (1920*2) * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_2400:
			numTotalAudioSamples = (2000*2) * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_2398:
			numTotalAudioSamples = (2002*2) * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_1500:
			numTotalAudioSamples = (3200*2) * inFrameNumNonInclusive;
			break;
		case NTV2_FRAMERATE_1498:
			// depends on cadenceFrame;
			numAudioSamplesFromWholeGroups = ((1*3204*2) + (4*3203*2)) * numWholeGroupsOfFive;
			numAudioSamplesFromRemainder = (remainder == 0) ? 0 : (((3203*2) * remainder) + 2);
			numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
			break;
#if !defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_1900:	// Not supported yet
		case NTV2_FRAMERATE_1898:	// Not supported yet
		case NTV2_FRAMERATE_1800: 	// Not supported yet
		case NTV2_FRAMERATE_1798:	// Not supported yet
#endif	//!defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_UNKNOWN:
		case NTV2_NUM_FRAMERATES:
			numTotalAudioSamples = 0*2; //haha
			break;
		}
	}

	return numTotalAudioSamples;
}


// For a given sequenceRate and playRate, given the cadenceFrame it returns how many times we
// repeate the frame to output varicam.  If the result is zero then this is an unsupported varicam
// rate.
ULWord GetVaricamRepeatCount (const NTV2FrameRate inSequenceRate, const NTV2FrameRate inPlayRate, const ULWord inCadenceFrame)
{
	ULWord result = 0;

	switch (inPlayRate)
	{
	case NTV2_FRAMERATE_6000:
		switch (inSequenceRate)
		{
		case NTV2_FRAMERATE_1500:
			result = 4;
			break;
		case NTV2_FRAMERATE_2400:			// 24 -> 60					2:3|2:3|2:3 ...
			//inCadenceFrame %= 2;
			switch (inCadenceFrame % 2)
			{
				case 0:
					result = 2;
					break;
				case 1:
					result = 3;
					break;
			}
			break;
		case NTV2_FRAMERATE_2500:			// 25 -> 60					2:3:2:3:2|2:3:2:3:2 ...
			//inCadenceFrame %= 5;
			switch (inCadenceFrame % 5)
			{
				case 0:
				case 2:
				case 4:
					result = 2;
					break;
				case 1:
				case 3:
					result = 3;
					break;
			}
			break;
		case NTV2_FRAMERATE_3000:			// 30 -> 60					2|2|2|2|2|2 ...
			result = 2;
			break;
		case NTV2_FRAMERATE_4800:			// 48 -> 60					2:1:1:1|2:1:1:1 ...
			//inCadenceFrame %= 4;
			switch (inCadenceFrame % 4)
			{
				case 0:
					result = 2;
					break;
				case 1:
				case 2:
				case 3:
					result = 1;
					break;
			}
			break;
		case NTV2_FRAMERATE_5000:			// 50 -> 60					2:1:1:1:1|2:1:1:1:1: ...
			//inCadenceFrame %= 5;
			switch (inCadenceFrame % 5)
			{
				case 0:
					result = 2;
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					result = 1;
					break;
			}
			break;
		case NTV2_FRAMERATE_6000:			// 60 -> 60					1|1|1|1|1|1 ...
			result = 1;
			break;
		default:
			break;
		}
		break;

	case NTV2_FRAMERATE_5994:
		switch (inSequenceRate)
		{
		case NTV2_FRAMERATE_1498:
			result = 4;
			break;
		case NTV2_FRAMERATE_2398:			// 23.98 -> 59.94			2:3|2:3|2:3 ...
			//inCadenceFrame %= 2;
			switch (inCadenceFrame % 2)
			{
				case 0:
					result = 2;
					break;
				case 1:
					result = 3;
					break;
			}
			break;
		case NTV2_FRAMERATE_2997:			// 29.97 -> 59.94			2|2|2|2|2|2 ...
			result = 2;
			break;
		case NTV2_FRAMERATE_4795:			// 47.95 -> 59.94			2:1:1:1|2:1:1:1 ...
			//inCadenceFrame %= 4;
			switch (inCadenceFrame % 4)
			{
				case 0:
					result = 2;
					break;
				case 1:
				case 2:
				case 3:
					result = 1;
					break;
			}
			break;
		case NTV2_FRAMERATE_5994:			// 59.94 -> 59.94			1|1|1|1|1|1 ...
			result = 1;
			break;
		default:
			break;
		}
		break;

	case NTV2_FRAMERATE_5000:
		switch (inSequenceRate)
		{
		case NTV2_FRAMERATE_2500:			// 25 -> 50					2|2|2|2|2| ...
			result = 2;
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
	return result;
}

ULWord GetScaleFromFrameRate (const NTV2FrameRate inFrameRate)
{
	switch (inFrameRate)
	{
		case NTV2_FRAMERATE_12000:		return 12000;
		case NTV2_FRAMERATE_11988:		return 11988;
		case NTV2_FRAMERATE_6000:		return 6000;
		case NTV2_FRAMERATE_5994:		return 5994;
		case NTV2_FRAMERATE_5000:		return 5000;
		case NTV2_FRAMERATE_4800:		return 4800;
		case NTV2_FRAMERATE_4795:		return 4795;
		case NTV2_FRAMERATE_3000:		return 3000;
		case NTV2_FRAMERATE_2997:		return 2997;
		case NTV2_FRAMERATE_2500:		return 2500;
		case NTV2_FRAMERATE_2400:		return 2400;
		case NTV2_FRAMERATE_2398:		return 2398;
		case NTV2_FRAMERATE_1500:		return 1500;
		case NTV2_FRAMERATE_1498:		return 1498;
#if !defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_1900:		return 1900;
		case NTV2_FRAMERATE_1898:		return 1898;
		case NTV2_FRAMERATE_1800:		return 1800;
		case NTV2_FRAMERATE_1798:		return 1798;
#endif	//!defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_UNKNOWN:	break;
#if defined(_DEBUG)
		case NTV2_NUM_FRAMERATES:		break;
#else
		default:						break;
#endif
	}
	return 0;
}

// GetFrameRateFromScale(long scale, long duration, NTV2FrameRate playFrameRate)
// For a given scale value it returns the associated frame rate.  This routine is
// used to calculate and decipher the sequence frame rate.
NTV2FrameRate GetFrameRateFromScale (long scale, long duration, NTV2FrameRate playFrameRate)
{
	NTV2FrameRate result = NTV2_FRAMERATE_6000;

	// Generally the duration is 100 and in that event the scale will tell us for sure what the
	// sequence rate is.
	if (duration == 100)
	{
		switch (scale)
		{
			case 12000:	result = NTV2_FRAMERATE_12000;	break;
			case 11988:	result = NTV2_FRAMERATE_11988;	break;
			case 6000:	result = NTV2_FRAMERATE_6000;	break;
			case 5994:	result = NTV2_FRAMERATE_5994;	break;
			case 5000:	result = NTV2_FRAMERATE_5000;	break;
			case 4800:	result = NTV2_FRAMERATE_4800;	break;
			case 4795:	result = NTV2_FRAMERATE_4795;	break;
			case 3000:	result = NTV2_FRAMERATE_3000;	break;
			case 2997:	result = NTV2_FRAMERATE_2997;	break;
			case 2500:	result = NTV2_FRAMERATE_2500;	break;
			case 2400:	result = NTV2_FRAMERATE_2400;	break;
			case 2398:	result = NTV2_FRAMERATE_2398;	break;
			case 1500:	result = NTV2_FRAMERATE_1500;	break;
			case 1498:	result = NTV2_FRAMERATE_1498;	break;
		}
	}
	else if (duration == 0)
	{
		result = playFrameRate;
	}
	else
	{
		float scaleFloat = scale / duration * float(100.0);
		long scaleInt = long(scaleFloat);

		// In this case we need to derive the sequence rate based on scale, duration and what
		// our playback rate is.  So first we break down what we might expect based on our
		// playback rate.  This gives us some room to look at values that are returned and
		// which are not exact based on rounding errors.  We can break this check up into two
		// camps because the assumption is we don't have to worry about playing back 23.98 fps
		// sequence on a 60 fps output and conversly playing back 30 fps sequences on a 59.94
		// fps output.
		switch (playFrameRate)
		{
		case NTV2_FRAMERATE_12000:
		case NTV2_FRAMERATE_6000:
		case NTV2_FRAMERATE_5000:
		case NTV2_FRAMERATE_4800:
		case NTV2_FRAMERATE_3000:
		case NTV2_FRAMERATE_2500:
		case NTV2_FRAMERATE_2400:
		case NTV2_FRAMERATE_1500:
			if (scaleInt <= 1500 + 100)
				result = NTV2_FRAMERATE_1500;
			else if (scaleInt <= 2400 + 50)
				result = NTV2_FRAMERATE_2400;
			else if (scaleInt <= 2500 + 100)
				result = NTV2_FRAMERATE_2500;
			else if (scaleInt <= 3000 + 100)
				result = NTV2_FRAMERATE_3000;
			else if (scaleInt <= 4800 + 100)
				result = NTV2_FRAMERATE_4800;
			else if (scaleInt <= 5000 + 100)
				result = NTV2_FRAMERATE_5000;
			else if (scaleInt <= 6000 + 100)
				result = NTV2_FRAMERATE_6000;
			else
				result = NTV2_FRAMERATE_12000;
			break;

		case NTV2_FRAMERATE_11988:
		case NTV2_FRAMERATE_5994:
		case NTV2_FRAMERATE_4795:
		case NTV2_FRAMERATE_2997:
		case NTV2_FRAMERATE_2398:
		case NTV2_FRAMERATE_1498:
			if (scaleInt <= 1498 + 100)				// add some fudge factor for rounding errors
				result = NTV2_FRAMERATE_1498;
			else if (scaleInt <= 2398 + 100)
				result = NTV2_FRAMERATE_2398;
			else if (scaleInt <= 2997 + 100)
				result = NTV2_FRAMERATE_2997;
			else if (scaleInt <= 4795 + 100)
				result = NTV2_FRAMERATE_4795;
			else if (scaleInt <= 5994 + 100)
				result = NTV2_FRAMERATE_5994;
			else
				result = NTV2_FRAMERATE_11988;
			break;
		default:
			break;
		}
	}
	return result;
}


NTV2FrameRate GetNTV2FrameRateFromNumeratorDenominator (const ULWord inNumerator, const ULWord inDenominator)
{
	if (inDenominator == 100)
		switch (inNumerator)
		{
			case 12000:	return NTV2_FRAMERATE_12000;
			case 11988:	return NTV2_FRAMERATE_11988;
			case 6000:	return NTV2_FRAMERATE_6000;
			case 5994:	return NTV2_FRAMERATE_5994;
			case 5000:	return NTV2_FRAMERATE_5000;
			case 4800:	return NTV2_FRAMERATE_4800;
			case 4795:	return NTV2_FRAMERATE_4795;
			case 3000:	return NTV2_FRAMERATE_3000;
			case 2997:	return NTV2_FRAMERATE_2997;
			case 2500:	return NTV2_FRAMERATE_2500;
			case 2400:	return NTV2_FRAMERATE_2400;
			case 2398:	return NTV2_FRAMERATE_2398;
			case 1500:	return NTV2_FRAMERATE_1500;
			case 1498:	return NTV2_FRAMERATE_1498;
			default:	break;
		}
	else
	{
		const ULWord denominator(inDenominator == 1 ? inDenominator * 1000ULL : inDenominator);
		const ULWord numerator(inDenominator == 1 ? inNumerator * 1000ULL : inNumerator);
		switch (numerator)
		{
			case 120000:	return (denominator == 1000) ? NTV2_FRAMERATE_12000 : NTV2_FRAMERATE_11988;
			case 60000:		return (denominator == 1000) ? NTV2_FRAMERATE_6000 : NTV2_FRAMERATE_5994;
			case 50000:		return (denominator == 1000) ? NTV2_FRAMERATE_5000 : NTV2_FRAMERATE_UNKNOWN;
			case 48000:		return (denominator == 1000) ? NTV2_FRAMERATE_4800 : NTV2_FRAMERATE_4795;
			case 30000:		return (denominator == 1000) ? NTV2_FRAMERATE_3000 : NTV2_FRAMERATE_2997;
			case 25000:		return (denominator == 1000) ? NTV2_FRAMERATE_2500 : NTV2_FRAMERATE_UNKNOWN;
			case 24000:		return (denominator == 1000) ? NTV2_FRAMERATE_2400 : NTV2_FRAMERATE_2398;
			case 15000:		return (denominator == 1000) ? NTV2_FRAMERATE_1500 : NTV2_FRAMERATE_1498;
			default:		break;
		}
	}
	return NTV2_FRAMERATE_UNKNOWN;
}


NTV2FrameRate GetNTV2FrameRateFromVideoFormat (const NTV2VideoFormat videoFormat)
{
    NTV2FrameRate frameRate = NTV2_FRAMERATE_UNKNOWN;
	switch ( videoFormat )
	{
	
	case NTV2_FORMAT_2K_1498:
		frameRate = NTV2_FRAMERATE_1498;
		break;
		
	case NTV2_FORMAT_2K_1500:
		frameRate = NTV2_FRAMERATE_1500;
		break;
	
	case NTV2_FORMAT_525_2398:
	case NTV2_FORMAT_720p_2398:
	case NTV2_FORMAT_1080psf_2K_2398:
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2K_2398:
	case NTV2_FORMAT_1080p_2K_4795_B:
	case NTV2_FORMAT_2K_2398:
	case NTV2_FORMAT_4x1920x1080psf_2398:
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_4x2048x1080psf_2398:
	case NTV2_FORMAT_4x2048x1080p_2398:
	case NTV2_FORMAT_4x2048x1080p_4795_B:
	case NTV2_FORMAT_3840x2160p_2398:
	case NTV2_FORMAT_3840x2160psf_2398:
	case NTV2_FORMAT_4096x2160p_2398:
	case NTV2_FORMAT_4096x2160psf_2398:
	case NTV2_FORMAT_4096x2160p_4795_B:
	case NTV2_FORMAT_4x3840x2160p_2398:
	case NTV2_FORMAT_4x4096x2160p_2398:
	case NTV2_FORMAT_4x4096x2160p_4795_B:
		frameRate = NTV2_FRAMERATE_2398;
		break;
		
	case NTV2_FORMAT_525_2400:
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2K_2400:
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2K_2400:
	case NTV2_FORMAT_1080p_2K_4800_B:
	case NTV2_FORMAT_2K_2400:
	case NTV2_FORMAT_4x1920x1080psf_2400:
	case NTV2_FORMAT_4x1920x1080p_2400:
	case NTV2_FORMAT_4x2048x1080psf_2400:
	case NTV2_FORMAT_4x2048x1080p_2400:
	case NTV2_FORMAT_4x2048x1080p_4800_B:
	case NTV2_FORMAT_3840x2160p_2400:
	case NTV2_FORMAT_3840x2160psf_2400:
	case NTV2_FORMAT_4096x2160p_2400:
	case NTV2_FORMAT_4096x2160psf_2400:
	case NTV2_FORMAT_4096x2160p_4800_B:
	case NTV2_FORMAT_4x3840x2160p_2400:
	case NTV2_FORMAT_4x4096x2160p_2400:
	case NTV2_FORMAT_4x4096x2160p_4800_B:
		frameRate = NTV2_FRAMERATE_2400;
		break;
		
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_625psf_2500:
	case NTV2_FORMAT_720p_2500:
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080psf_2500_2:
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_1080psf_2K_2500:
	case NTV2_FORMAT_1080p_2K_2500:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_1080p_2K_5000_B:
	case NTV2_FORMAT_2K_2500:
	case NTV2_FORMAT_4x1920x1080psf_2500:
	case NTV2_FORMAT_4x1920x1080p_2500:
	case NTV2_FORMAT_4x1920x1080p_5000_B:
	case NTV2_FORMAT_4x2048x1080psf_2500:
	case NTV2_FORMAT_4x2048x1080p_2500:
	case NTV2_FORMAT_4x2048x1080p_5000_B:
	case NTV2_FORMAT_3840x2160psf_2500:
	case NTV2_FORMAT_3840x2160p_2500:
	case NTV2_FORMAT_3840x2160p_5000_B:
	case NTV2_FORMAT_4096x2160psf_2500:
	case NTV2_FORMAT_4096x2160p_2500:
	case NTV2_FORMAT_4096x2160p_5000_B:
	case NTV2_FORMAT_4x3840x2160p_2500:
	case NTV2_FORMAT_4x3840x2160p_5000_B:
	case NTV2_FORMAT_4x4096x2160p_2500:
	case NTV2_FORMAT_4x4096x2160p_5000_B:
		frameRate = NTV2_FRAMERATE_2500;
		break;
	
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_525psf_2997:
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_1080p_2K_2997:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_1080p_2K_5994_B:
	case NTV2_FORMAT_4x1920x1080p_2997:
	case NTV2_FORMAT_4x1920x1080psf_2997:
	case NTV2_FORMAT_4x1920x1080p_5994_B:
	case NTV2_FORMAT_4x2048x1080p_2997:
	case NTV2_FORMAT_4x2048x1080psf_2997:
	case NTV2_FORMAT_4x2048x1080p_5994_B:
	case NTV2_FORMAT_3840x2160p_2997:
	case NTV2_FORMAT_3840x2160psf_2997:
	case NTV2_FORMAT_3840x2160p_5994_B:
	case NTV2_FORMAT_4096x2160p_2997:
	case NTV2_FORMAT_4096x2160psf_2997:
	case NTV2_FORMAT_4096x2160p_5994_B:
	case NTV2_FORMAT_4x3840x2160p_2997:
	case NTV2_FORMAT_4x3840x2160p_5994_B:
	case NTV2_FORMAT_4x4096x2160p_2997:
	case NTV2_FORMAT_4x4096x2160p_5994_B:
		frameRate = NTV2_FRAMERATE_2997;
		break;
		
	case NTV2_FORMAT_1080i_6000:
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080p_2K_3000:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_1080p_2K_6000_B:
	case NTV2_FORMAT_4x1920x1080p_3000:
	case NTV2_FORMAT_4x1920x1080psf_3000:
	case NTV2_FORMAT_4x1920x1080p_6000_B:
	case NTV2_FORMAT_4x2048x1080p_3000:
	case NTV2_FORMAT_4x2048x1080psf_3000:
	case NTV2_FORMAT_4x2048x1080p_6000_B:
	case NTV2_FORMAT_3840x2160p_3000:
	case NTV2_FORMAT_3840x2160psf_3000:
	case NTV2_FORMAT_3840x2160p_6000_B:
	case NTV2_FORMAT_4096x2160p_3000:
	case NTV2_FORMAT_4096x2160psf_3000:
	case NTV2_FORMAT_4096x2160p_6000_B:
	case NTV2_FORMAT_4x3840x2160p_3000:
	case NTV2_FORMAT_4x3840x2160p_6000_B:
	case NTV2_FORMAT_4x4096x2160p_3000:
	case NTV2_FORMAT_4x4096x2160p_6000_B:
		frameRate = NTV2_FRAMERATE_3000;
		break;
		
	case NTV2_FORMAT_1080p_2K_4795_A:
	case NTV2_FORMAT_4x2048x1080p_4795:
	case NTV2_FORMAT_4096x2160p_4795:
	case NTV2_FORMAT_4x4096x2160p_4795:
		frameRate = NTV2_FRAMERATE_4795;
		break;
		
	case NTV2_FORMAT_1080p_2K_4800_A:
	case NTV2_FORMAT_4x2048x1080p_4800:
	case NTV2_FORMAT_4096x2160p_4800:
	case NTV2_FORMAT_4x4096x2160p_4800:
		frameRate = NTV2_FRAMERATE_4800;
		break;

	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_2K_5000_A:
	case NTV2_FORMAT_4x1920x1080p_5000:
	case NTV2_FORMAT_4x2048x1080p_5000:
    case NTV2_FORMAT_3840x2160p_5000:
    case NTV2_FORMAT_4096x2160p_5000:
	case NTV2_FORMAT_4x3840x2160p_5000:
	case NTV2_FORMAT_4x4096x2160p_5000:
		frameRate = NTV2_FRAMERATE_5000;
		break;
		
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_2K_5994_A:
	case NTV2_FORMAT_4x1920x1080p_5994:
	case NTV2_FORMAT_4x2048x1080p_5994:
    case NTV2_FORMAT_3840x2160p_5994:
    case NTV2_FORMAT_4096x2160p_5994:
	case NTV2_FORMAT_4x3840x2160p_5994:
	case NTV2_FORMAT_4x4096x2160p_5994:
		frameRate = NTV2_FRAMERATE_5994;
		break;

	case NTV2_FORMAT_720p_6000:
	case NTV2_FORMAT_1080p_6000_A:
	case NTV2_FORMAT_1080p_2K_6000_A:
	case NTV2_FORMAT_4x1920x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_6000:
    case NTV2_FORMAT_3840x2160p_6000:
    case NTV2_FORMAT_4096x2160p_6000:
	case NTV2_FORMAT_4x3840x2160p_6000:
	case NTV2_FORMAT_4x4096x2160p_6000:
		frameRate = NTV2_FRAMERATE_6000;
		break;

	case NTV2_FORMAT_4x2048x1080p_11988:
    case NTV2_FORMAT_4096x2160p_11988:
		frameRate = NTV2_FRAMERATE_11988;
		break;
	case NTV2_FORMAT_4x2048x1080p_12000:
    case NTV2_FORMAT_4096x2160p_12000:
		frameRate = NTV2_FRAMERATE_12000;
		break;

#if defined (_DEBUG)
	//	Debug builds warn about missing values
	case NTV2_FORMAT_UNKNOWN:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
	case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
	case NTV2_FORMAT_END_2K_DEF_FORMATS:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
	case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
	case NTV2_FORMAT_END_4K_DEF_FORMATS2:
	case NTV2_FORMAT_END_UHD2_DEF_FORMATS:
	case NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS:
		break;
#else
	default:
		break;	// Unsupported -- fail
#endif
	}

	return frameRate;

}	//	GetNTV2FrameRateFromVideoFormat


NTV2FrameGeometry GetNormalizedFrameGeometry (const NTV2FrameGeometry inFrameGeometry)
{
	switch (inFrameGeometry)
	{
		case NTV2_FG_1920x1080:		//	1080i, 1080p
		case NTV2_FG_1280x720:		//	720p
		case NTV2_FG_720x486:		//	ntsc 525i, 525p60
		case NTV2_FG_720x576:		//	pal 625i
		case NTV2_FG_2048x1080:		//	2k1080p
		case NTV2_FG_2048x1556:		//	2k1556psf
		case NTV2_FG_4x1920x1080:	//	UHD
		case NTV2_FG_4x2048x1080:	//	4K
		case NTV2_FG_4x3840x2160:
		case NTV2_FG_4x4096x2160:
			return inFrameGeometry;	//	No change, already normalized

															//	525i
		case NTV2_FG_720x508:	return NTV2_FG_720x486;		//	720x486 + tall vanc
		case NTV2_FG_720x514:	return NTV2_FG_720x486;		//	720x486 + taller vanc (extra-wide ntsc)

															//	625i
		case NTV2_FG_720x598:	return NTV2_FG_720x576;		//	pal 625i + tall vanc
		case NTV2_FG_720x612:	return NTV2_FG_720x576;		//	720x576 + taller vanc (extra-wide pal)

															//	720p
		case NTV2_FG_1280x740:	return NTV2_FG_1280x720;	//	1280x720 + tall vanc

															//	1080
		case NTV2_FG_1920x1112:	return NTV2_FG_1920x1080;	//	1920x1080 + tall vanc
		case NTV2_FG_1920x1114:	return NTV2_FG_1920x1080;	//	1920x1080 + taller vanc

															//	2kx1080
		case NTV2_FG_2048x1112:	return NTV2_FG_2048x1080;	//	2048x1080 + tall vanc
		case NTV2_FG_2048x1114:	return NTV2_FG_2048x1080;	//	2048x1080 + taller vanc

															//	2kx1556 film
		case NTV2_FG_2048x1588:	return NTV2_FG_2048x1556;	//	2048x1556 + tall vanc

#if defined (_DEBUG)
		case NTV2_FG_INVALID:	break;
#else
		default:				break;
#endif
	}
	return NTV2_FG_INVALID;	//	fail
}


NTV2FrameGeometry GetVANCFrameGeometry (const NTV2FrameGeometry inFrameGeometry, const NTV2VANCMode inVancMode)
{
	if (!NTV2_IS_VALID_VANCMODE(inVancMode))
		return NTV2_FG_INVALID;	//	Invalid vanc mode
	if (!NTV2_IS_VALID_NTV2FrameGeometry(inFrameGeometry))
		return NTV2_FG_INVALID;	//	Invalid FG
	if (!NTV2_IS_VANCMODE_ON(inVancMode))
		return ::GetNormalizedFrameGeometry(inFrameGeometry);	//	Return normalized

	switch (inFrameGeometry)
	{
		case NTV2_FG_1920x1080:	//	1920x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_1920x1112:	//	1920x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_1920x1114:	//	1920x1080 ::NTV2_VANCMODE_TALLER
			return NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_1920x1112 : NTV2_FG_1920x1114;

		case NTV2_FG_1280x720:	//	1280x720, ::NTV2_VANCMODE_OFF
		case NTV2_FG_1280x740:	//	1280x720 ::NTV2_VANCMODE_TALL
			return NTV2_FG_1280x740;

		case NTV2_FG_720x486:	//	720x486 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x508:	//	720x486 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x514: 	//	720x486 ::NTV2_VANCMODE_TALLER
			return NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_720x508 : NTV2_FG_720x514;

		case NTV2_FG_720x576:	//	720x576 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x598:	//	720x576 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x612: 	//	720x576 ::NTV2_VANCMODE_TALLER
			return NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_720x598 : NTV2_FG_720x612;

		case NTV2_FG_2048x1080:	//	2048x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1112: //	2048x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_2048x1114:	//	2048x1080 ::NTV2_VANCMODE_TALLER
			return NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_2048x1112 : NTV2_FG_2048x1114;

		case NTV2_FG_2048x1556:	//	2048x1556 film ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1588: //	2048x1556 film ::NTV2_VANCMODE_TALL
			return NTV2_FG_2048x1588;

		case NTV2_FG_4x1920x1080:	//	3840x2160 ::NTV2_VANCMODE_OFF
		case NTV2_FG_4x2048x1080:	//	4096x2160 ::NTV2_VANCMODE_OFF
		case NTV2_FG_4x3840x2160:
		case NTV2_FG_4x4096x2160:
			return inFrameGeometry;	//	no tall or taller geometries!
#if defined (_DEBUG)
		case NTV2_FG_INVALID:	break;
#else
		default:				break;
#endif
	}
	return NTV2_FG_INVALID;	//	fail
}

NTV2FrameGeometry GetGeometryFromFrameDimensions (const NTV2FrameDimensions & inFD)
{
	for (NTV2FrameGeometry fg(NTV2_FG_FIRST);  fg < NTV2_FG_NUMFRAMEGEOMETRIES;  fg = NTV2FrameGeometry(fg+1))
		if (::GetNTV2FrameGeometryWidth(fg) == inFD.GetWidth())
			if (::GetNTV2FrameGeometryHeight(fg) == inFD.GetHeight())
				return fg;
	return NTV2_FG_INVALID;
}

bool HasVANCGeometries (const NTV2FrameGeometry inFG)
{
	switch (inFG)
	{
		case NTV2_FG_1920x1080:	//	1920x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_1920x1112:	//	1920x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_1920x1114:	//	1920x1080 ::NTV2_VANCMODE_TALLER
		case NTV2_FG_1280x720:	//	1280x720, ::NTV2_VANCMODE_OFF
		case NTV2_FG_1280x740:	//	1280x720 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x486:	//	720x486 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x508:	//	720x486 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x514: 	//	720x486 ::NTV2_VANCMODE_TALLER
		case NTV2_FG_720x576:	//	720x576 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x598:	//	720x576 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x612: 	//	720x576 ::NTV2_VANCMODE_TALLER
		case NTV2_FG_2048x1080:	//	2048x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1112: //	2048x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_2048x1114:	//	2048x1080 ::NTV2_VANCMODE_TALLER
		case NTV2_FG_2048x1556:	//	2048x1556 film ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1588: //	2048x1556 film ::NTV2_VANCMODE_TALL
			return true;

		case NTV2_FG_4x1920x1080:	//	3840x2160
		case NTV2_FG_4x2048x1080:	//	4096x2160
		case NTV2_FG_4x3840x2160:
		case NTV2_FG_4x4096x2160:
			break;					//	no tall or taller geometries!
#if defined (_DEBUG)
		case NTV2_FG_INVALID:	break;
#else
		default:				break;
#endif
	}
	return false;
}

NTV2GeometrySet GetRelatedGeometries (const NTV2FrameGeometry inFG)
{
	NTV2GeometrySet	result;
	switch (inFG)
	{
		case NTV2_FG_1920x1080:	//	1920x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_1920x1112:	//	1920x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_1920x1114:	//	1920x1080 ::NTV2_VANCMODE_TALLER
			result.insert(NTV2_FG_1920x1080);	result.insert(NTV2_FG_1920x1112);	result.insert(NTV2_FG_1920x1114);
			break;

		case NTV2_FG_1280x720:	//	1280x720, ::NTV2_VANCMODE_OFF
		case NTV2_FG_1280x740:	//	1280x720 ::NTV2_VANCMODE_TALL
			result.insert(NTV2_FG_1280x720);	result.insert(NTV2_FG_1280x740);
			break;

		case NTV2_FG_720x486:	//	720x486 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x508:	//	720x486 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x514: 	//	720x486 ::NTV2_VANCMODE_TALLER
			result.insert(NTV2_FG_720x486);	result.insert(NTV2_FG_720x508);	result.insert(NTV2_FG_720x514);
			break;

		case NTV2_FG_720x576:	//	720x576 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x598:	//	720x576 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x612: 	//	720x576 ::NTV2_VANCMODE_TALLER
			result.insert(NTV2_FG_720x576);	result.insert(NTV2_FG_720x598);	result.insert(NTV2_FG_720x612);
			break;

		case NTV2_FG_2048x1080:	//	2048x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1112: //	2048x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_2048x1114:	//	2048x1080 ::NTV2_VANCMODE_TALLER
			result.insert(NTV2_FG_2048x1080);	result.insert(NTV2_FG_2048x1112);	result.insert(NTV2_FG_2048x1114);
			break;

		case NTV2_FG_2048x1556:	//	2048x1556 film ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1588: //	2048x1556 film ::NTV2_VANCMODE_TALL
			result.insert(NTV2_FG_2048x1556);	result.insert(NTV2_FG_2048x1588);
			break;

		case NTV2_FG_4x1920x1080:	//	3840x2160
		case NTV2_FG_4x2048x1080:	//	4096x2160
		case NTV2_FG_4x3840x2160:
		case NTV2_FG_4x4096x2160:
			result.insert(inFG);
			break;					//	no tall or taller geometries!
#if defined (_DEBUG)
		case NTV2_FG_INVALID:	break;
#else
		default:				break;
#endif
	}
	return result;
}

NTV2VANCMode GetVANCModeForGeometry (const NTV2FrameGeometry inFG)
{
	if (NTV2_IS_TALL_VANC_GEOMETRY(inFG))
		return NTV2_VANCMODE_TALL;
	else if (NTV2_IS_TALLER_VANC_GEOMETRY(inFG))
		return NTV2_VANCMODE_TALLER;
	else if (NTV2_IS_VALID_NTV2FrameGeometry(inFG))
		return NTV2_VANCMODE_OFF;
	return NTV2_VANCMODE_INVALID;
}


NTV2FrameGeometry GetGeometryFromStandard (const NTV2Standard inStandard)
{
	switch (inStandard)
	{
	case NTV2_STANDARD_720:			return NTV2_FG_1280x720;	//	720p
	case NTV2_STANDARD_525:			return NTV2_FG_720x486;		//	525i
	case NTV2_STANDARD_625:			return NTV2_FG_720x576;		//	625i

	case NTV2_STANDARD_1080:
	case NTV2_STANDARD_1080p:		return NTV2_FG_1920x1080;	//	1080i, 1080psf, 1080p

	case NTV2_STANDARD_2K:			return NTV2_FG_2048x1556;	//	2048x1556 film

	case NTV2_STANDARD_2Kx1080p:
	case NTV2_STANDARD_2Kx1080i:	return NTV2_FG_2048x1080;	//	2K1080p/i/psf

	case NTV2_STANDARD_3840x2160p:								//	UHD
	case NTV2_STANDARD_3840HFR:									//	HFR UHD
	case NTV2_STANDARD_3840i:		return NTV2_FG_4x1920x1080;	//	HFR psf

	case NTV2_STANDARD_4096x2160p:								//	4K
	case NTV2_STANDARD_4096HFR:									//	HFR 4K
	case NTV2_STANDARD_4096i:		return NTV2_FG_4x2048x1080;	//	HFR 4K psf
		
	case NTV2_STANDARD_7680:		return NTV2_FG_4x3840x2160;
		
	case NTV2_STANDARD_8192:		return NTV2_FG_4x4096x2160;
#if defined (_DEBUG)
	case NTV2_STANDARD_INVALID:	break;
#else
	default:					break;
#endif
	}
	return NTV2_FG_INVALID;
}

NTV2Standard GetStandardFromGeometry (const NTV2FrameGeometry inGeometry, const bool inIsProgressive)
{
	switch (inGeometry)
	{
		case NTV2_FG_1920x1080:	//	1920x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_1920x1112:	//	1920x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_1920x1114:	//	1920x1080 ::NTV2_VANCMODE_TALLER
			return inIsProgressive ? NTV2_STANDARD_1080p : NTV2_STANDARD_1080;

		case NTV2_FG_1280x720:	//	1280x720, ::NTV2_VANCMODE_OFF
		case NTV2_FG_1280x740:	//	1280x720 ::NTV2_VANCMODE_TALL
			return NTV2_STANDARD_720;

		case NTV2_FG_720x486:	//	720x486 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x508:	//	720x486 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x514: 	//	720x486 ::NTV2_VANCMODE_TALLER
			return NTV2_STANDARD_525;

		case NTV2_FG_720x576:	//	720x576 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x598:	//	720x576 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x612: 	//	720x576 ::NTV2_VANCMODE_TALLER
			return NTV2_STANDARD_625;

		case NTV2_FG_2048x1080:	//	2048x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1112: //	2048x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_2048x1114:	//	2048x1080 ::NTV2_VANCMODE_TALLER
			return inIsProgressive ? NTV2_STANDARD_2Kx1080p : NTV2_STANDARD_2Kx1080i;

		case NTV2_FG_2048x1556:	//	2048x1556 film ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1588: //	2048x1556 film ::NTV2_VANCMODE_TALL
			return NTV2_STANDARD_2K;

		case NTV2_FG_4x1920x1080:	//	3840x2160
			return inIsProgressive ? NTV2_STANDARD_3840x2160p : NTV2_STANDARD_3840i; // NTV2_STANDARD_3840HFR

		case NTV2_FG_4x2048x1080:	//	4096x2160
			return inIsProgressive ? NTV2_STANDARD_4096x2160p : NTV2_STANDARD_4096i;	//	NTV2_STANDARD_4096HFR

		case NTV2_FG_4x3840x2160:	//	4320x7680	uhd 8K
			return NTV2_STANDARD_7680;

		case NTV2_FG_4x4096x2160:	//	4320x8192	8K
			return NTV2_STANDARD_8192;

#if defined (_DEBUG)
		case NTV2_FG_INVALID:	break;
#else
		default:				break;
#endif
	}
	return NTV2_STANDARD_INVALID;
}


bool NTV2DeviceCanDoFormat (const NTV2DeviceID		inDeviceID,
							const NTV2FrameRate		inFrameRate,
							const NTV2FrameGeometry	inFrameGeometry,
							const NTV2Standard		inStandard)
{	//	DEPRECATED FUNCTION
	//	This implementation is very inefficient, but...
	//	a)	this function is deprecated;
	//	b)	nobody should be calling it (they should be calling NTV2DeviceCanDoVideoFormat instead)
	//	c)	they shouldn't be calling it every frame.
	//	We could make it efficient by creating a static global rate/geometry/standard-to-videoFormat
	//	map, but that has race/deadlock issues.

	const NTV2FrameGeometry	fg	(::GetNormalizedFrameGeometry(inFrameGeometry));
	//	Look for a video format that matches the given frame rate, geometry and standard...
	for (NTV2VideoFormat vFmt(NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT);  vFmt < NTV2_MAX_NUM_VIDEO_FORMATS;  vFmt = NTV2VideoFormat(vFmt+1))
	{
		if (!NTV2_IS_VALID_VIDEO_FORMAT(vFmt))
			continue;
		const NTV2FrameRate		fr	(::GetNTV2FrameRateFromVideoFormat(vFmt));
		const NTV2Standard		std	(::GetNTV2StandardFromVideoFormat(vFmt));
		const NTV2FrameGeometry	geo	(::GetNTV2FrameGeometryFromVideoFormat(vFmt));
		if (fr == inFrameRate  &&  std == inStandard  &&  fg == geo)
			return ::NTV2DeviceCanDoVideoFormat(inDeviceID, vFmt);
	}
	return false;
}

ULWord GetNTV2FrameGeometryHeight (const NTV2FrameGeometry inGeometry)
{
	const NTV2FormatDescriptor fd (::GetStandardFromGeometry(inGeometry), NTV2_FBF_8BIT_YCBCR);
	return fd.GetRasterHeight(/*visOnly?*/false);	//	Include VANC
}

ULWord GetNTV2FrameGeometryWidth (const NTV2FrameGeometry inGeometry)
{
	const NTV2FormatDescriptor fd (::GetStandardFromGeometry(inGeometry), NTV2_FBF_8BIT_YCBCR);
	return fd.GetRasterWidth();
}


//	Displayable width of format, not counting HANC/VANC
ULWord GetDisplayWidth (const NTV2VideoFormat inVideoFormat)
{
	const NTV2FormatDescriptor fd (inVideoFormat, NTV2_FBF_8BIT_YCBCR);
	return fd.GetRasterWidth();
}	//	GetDisplayWidth


//	Displayable height of format, not counting HANC/VANC
ULWord GetDisplayHeight (const NTV2VideoFormat inVideoFormat)
{
	const NTV2FormatDescriptor fd (inVideoFormat, NTV2_FBF_8BIT_YCBCR);
	return fd.GetVisibleRasterHeight();
}	//	GetDisplayHeight



//	NTV2SmpteLineNumber::NTV2SmpteLineNumber (const NTV2Standard inStandard)
//	IMPLEMENTATION MOVED INTO 'ntv2formatdescriptor.cpp'
//	SO AS TO USE SAME LineNumbersF1/LineNumbersF2 TABLES

ULWord NTV2SmpteLineNumber::GetFirstActiveLine (const NTV2FieldID inFieldID) const
{
	if (!NTV2_IS_VALID_FIELD (inFieldID))
		return 0;

	if (inFieldID == NTV2_FIELD0)
		return firstFieldTop ? smpteFirstActiveLine : smpteSecondActiveLine;
	else
		return firstFieldTop ? smpteSecondActiveLine : smpteFirstActiveLine;
}


ostream & NTV2SmpteLineNumber::Print (ostream & inOutStream) const
{
	if (!IsValid ())
		inOutStream << "INVALID ";
	inOutStream	<< "SMPTELineNumber(";
	if (IsValid ())
		inOutStream	<< "1st=" << smpteFirstActiveLine << (firstFieldTop ? "(top)" : "")
				<< ", 2nd=" << smpteSecondActiveLine << (firstFieldTop ? "" : "(top)")
				<< ", std=" << ::NTV2StandardToString (mStandard) << ")";
	else
		inOutStream	<< "INVALID)";
	return inOutStream;
}


string NTV2SmpteLineNumber::PrintLineNumber (const ULWord inLineOffset, const NTV2FieldID inRasterFieldID) const
{
	ostringstream	oss;
	if (NTV2_IS_VALID_FIELD (inRasterFieldID) && !NTV2_IS_PROGRESSIVE_STANDARD (mStandard))
		oss << "F" << (inRasterFieldID == 0 ? "1" : "2") << " ";
	oss << "L" << dec << inLineOffset+GetFirstActiveLine(inRasterFieldID);
	return oss.str();
}

#if !defined (NTV2_DEPRECATE)
	AJA_LOCAL_STATIC const char * NTV2VideoFormatStrings [NTV2_MAX_NUM_VIDEO_FORMATS] =
	{
		"",							//	NTV2_FORMAT_UNKNOWN						//	0		//	not used
		"1080i 50.00",				//	NTV2_FORMAT_1080i_5000					//	1
		"1080i 59.94",				//	NTV2_FORMAT_1080i_5994					//	2
		"1080i 60.00",				//	NTV2_FORMAT_1080i_6000					//	3
		"720p 59.94",				//	NTV2_FORMAT_720p_5994					//	4
		"720p 60.00",				//	NTV2_FORMAT_720p_6000					//	5
		"1080psf 23.98",			//	NTV2_FORMAT_1080psf_2398				//	6
		"1080psf 24.00",			//	NTV2_FORMAT_1080psf_2400				//	7
		"1080p 29.97",				//	NTV2_FORMAT_1080p_2997					//	8
		"1080p 30.00",				//	NTV2_FORMAT_1080p_3000					//	9
		"1080p 25.00",				//	NTV2_FORMAT_1080p_2500					//	10
		"1080p 23.98",				//	NTV2_FORMAT_1080p_2398					//	11
		"1080p 24.00",				//	NTV2_FORMAT_1080p_2400					//	12
		"2048x1080p 23.98",			//	NTV2_FORMAT_1080p_2K_2398				//	13
		"2048x1080p 24.00",			//	NTV2_FORMAT_1080p_2K_2400				//	14
		"2048x1080psf 23.98",       //	NTV2_FORMAT_1080psf_2K_2398				//	15
		"2048x1080psf 24.00",		//	NTV2_FORMAT_1080psf_2K_2400				//	16
		"720p 50",					//	NTV2_FORMAT_720p_5000					//	17
		"1080p 50.00b",				//	NTV2_FORMAT_1080p_5000					//	18
		"1080p 59.94b",				//	NTV2_FORMAT_1080p_5994					//	19
		"1080p 60.00b",				//	NTV2_FORMAT_1080p_6000					//	20
		"720p 23.98",				//	NTV2_FORMAT_720p_2398					//	21
		"720p 25.00",				//	NTV2_FORMAT_720p_2500					//	22
		"1080p 50.00a",				//	NTV2_FORMAT_1080p_5000_A				//	23
		"1080p 59.94a",				//	NTV2_FORMAT_1080p_5994_A				//	24
		"1080p 60.00a",				//	NTV2_FORMAT_1080p_6000_A				//	25
		"2048x1080p 25.00",         //	NTV2_FORMAT_1080p_2K_2500				//	26
		"2048x1080psf 25.00",       //	NTV2_FORMAT_1080psf_2K_2500				//	27
		"1080psf 25",				//	NTV2_FORMAT_1080psf_2500_2				//	28
		"1080psf 29.97",			//	NTV2_FORMAT_1080psf_2997_2				//	29
		"1080psf 30",				//	NTV2_FORMAT_1080psf_3000_2				//	30
		"",							//	NTV2_FORMAT_END_HIGH_DEF_FORMATS		//	31		// not used
		"525i 59.94",				//	NTV2_FORMAT_525_5994					//	32
		"625i 50.00",				//	NTV2_FORMAT_625_5000					//	33
		"525 23.98",				//	NTV2_FORMAT_525_2398					//	34
		"525 24.00",				//	NTV2_FORMAT_525_2400					//	35		// not used
		"525psf 29.97",				//	NTV2_FORMAT_525psf_2997					//	36
		"625psf 25",				//	NTV2_FORMAT_625psf_2500					//	37
		"",							//	NTV2_FORMAT_END_STANDARD_DEF_FORMATS	//	38		// not used
		"",							//											//	39		// not used
		"",							//											//	40		// not used
		"",							//											//	41		// not used
		"",							//											//	42		// not used
		"",							//											//	43		// not used
		"",							//											//	44		// not used
		"",							//											//	45		// not used
		"",							//											//	46		// not used
		"",							//											//	47		// not used
		"",							//											//	48		// not used
		"",							//											//	49		// not used
		"",							//											//	50		// not used
		"",							//											//	51		// not used
		"",							//											//	52		// not used
		"",							//											//	53		// not used
		"",							//											//	54		// not used
		"",							//											//	55		// not used
		"",							//											//	56		// not used
		"",							//											//	57		// not used
		"",							//											//	58		// not used
		"",							//											//	59		// not used
		"",							//											//	60		// not used
		"",							//											//	61		// not used
		"",							//											//	62		// not used
		"",							//											//	63		// not used
		"2048x1556psf 14.98",		//	NTV2_FORMAT_2K_1498						//	64
		"2048x1556psf 15.00",		//	NTV2_FORMAT_2K_1500						//	65
		"2048x1556psf 23.98",		//	NTV2_FORMAT_2K_2398						//	66
		"2048x1556psf 24.00",		//	NTV2_FORMAT_2K_2400						//	67
		"2048x1556psf 25.00",		//	NTV2_FORMAT_2K_2500						//	68
		"",							//	NTV2_FORMAT_END_2K_DEF_FORMATS			//	69		// not used
		"",							//											//	70		// not used
		"",							//											//	71		// not used
		"",							//											//	72		// not used
		"",							//											//	73		// not used
		"",							//											//	74		// not used
		"",							//											//	75		// not used
		"",							//											//	76		// not used
		"",							//											//	77		// not used
		"",							//											//	78		// not used
		"",							//											//	79		// not used
		"4x1920x1080psf 23.98",		//	NTV2_FORMAT_4x1920x1080psf_2398			//	80
		"4x1920x1080psf 24.00",		//	NTV2_FORMAT_4x1920x1080psf_2400			//	81
		"4x1920x1080psf 25.00",		//	NTV2_FORMAT_4x1920x1080psf_2500			//	82
		"4x1920x1080p 23.98",		//	NTV2_FORMAT_4x1920x1080p_2398			//	83
		"4x1920x1080p 24.00",		//	NTV2_FORMAT_4x1920x1080p_2400			//	84
		"4x1920x1080p 25.00",		//	NTV2_FORMAT_4x1920x1080p_2500			//	85
		"4x2048x1080psf 23.98",		//	NTV2_FORMAT_4x2048x1080psf_2398			//	86
		"4x2048x1080psf 24.00",		//	NTV2_FORMAT_4x2048x1080psf_2400			//	87
		"4x2048x1080psf 25.00",		//	NTV2_FORMAT_4x2048x1080psf_2500			//	88
		"4x2048x1080p 23.98",		//	NTV2_FORMAT_4x2048x1080p_2398			//	89
		"4x2048x1080p 24.00",		//	NTV2_FORMAT_4x2048x1080p_2400			//	90
		"4x2048x1080p 25.00",		//	NTV2_FORMAT_4x2048x1080p_2500			//	91
		"4x1920x1080p 29.97",		//	NTV2_FORMAT_4x1920x1080p_2997			//	92
		"4x1920x1080p 30.00",		//	NTV2_FORMAT_4x1920x1080p_3000			//	93
		"4x1920x1080psf 29.97",		//	NTV2_FORMAT_4x1920x1080psf_2997			//	94		//	not supported
		"4x1920x1080psf 30.00",		//	NTV2_FORMAT_4x1920x1080psf_3000			//	95		//	not supported
		"4x2048x1080p 29.97",		//	NTV2_FORMAT_4x2048x1080p_2997			//	96
		"4x2048x1080p 30.00",		//	NTV2_FORMAT_4x2048x1080p_3000			//	97
		"4x2048x1080psf 29.97",		//	NTV2_FORMAT_4x2048x1080psf_2997			//	98		//	not supported
		"4x2048x1080psf 30.00",		//	NTV2_FORMAT_4x2048x1080psf_3000			//	99		//	not supported
		"4x1920x1080p 50.00",		//	NTV2_FORMAT_4x1920x1080p_5000			//	100
		"4x1920x1080p 59.94",		//	NTV2_FORMAT_4x1920x1080p_5994			//	101
		"4x1920x1080p 60.00",		//	NTV2_FORMAT_4x1920x1080p_6000			//	102
		"4x2048x1080p 50.00",		//	NTV2_FORMAT_4x2048x1080p_5000			//	103
		"4x2048x1080p 59.94",		//	NTV2_FORMAT_4x2048x1080p_5994			//	104
		"4x2048x1080p 60.00",		//	NTV2_FORMAT_4x2048x1080p_6000			//	105
		"4x2048x1080p 47.95",		//	NTV2_FORMAT_4x2048x1080p_4795			//	106
		"4x2048x1080p 48.00",		//	NTV2_FORMAT_4x2048x1080p_4800			//	107
		"4x2048x1080p 119.88",		//	NTV2_FORMAT_4x2048x1080p_11988			//	108
		"4x2048x1080p 120.00",		//	NTV2_FORMAT_4x2048x1080p_12000			//	109
		"2048x1080p 60.00a",		//	NTV2_FORMAT_1080p_2K_6000_A				//	110	//	NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT2
		"2048x1080p 59.94a",		//	NTV2_FORMAT_1080p_2K_5994_A				//	111
		"2048x1080p 29.97",			//	NTV2_FORMAT_1080p_2K_2997				//	112
		"2048x1080p 30.00",			//	NTV2_FORMAT_1080p_2K_3000				//	113
		"2048x1080p 50.00a",		//	NTV2_FORMAT_1080p_2K_5000_A				//	114
		"2048x1080p 47.95a",		//	NTV2_FORMAT_1080p_2K_4795_A				//	115
		"2048x1080p 48.00a",		//	NTV2_FORMAT_1080p_2K_4800_A				//	116
		"2048x1080p 47.95b",		// 	NTV2_FORMAT_1080p_2K_4795_B,			// 117
		"2048x1080p 48.00b",		// 	NTV2_FORMAT_1080p_2K_4800_B,			// 118
		"2048x1080p 50.00b",		// 	NTV2_FORMAT_1080p_2K_5000_B,			// 119
		"2048x1080p 59.94b",		// 	NTV2_FORMAT_1080p_2K_5994_B,			// 120
		"2048x1080p 60.00b",		// 	NTV2_FORMAT_1080p_2K_6000_B,			// 121
	};

	AJA_LOCAL_STATIC const char * NTV2VideoStandardStrings [NTV2_NUM_STANDARDS] =
	{
		"1080i",					//	NTV2_STANDARD_1080						//	0
		"720p",						//	NTV2_STANDARD_720						//	1
		"525",						//	NTV2_STANDARD_525						//	2
		"625",						//	NTV2_STANDARD_625						//	3
		"1080p",					//	NTV2_STANDARD_1080p						//	4
		"2k"						//	NTV2_STANDARD_2K						//	5
	};


	AJA_LOCAL_STATIC const char * NTV2PixelFormatStrings [NTV2_FBF_NUMFRAMEBUFFERFORMATS] =
	{
		"10BIT_YCBCR",						//	NTV2_FBF_10BIT_YCBCR			//	0
		"8BIT_YCBCR",						//	NTV2_FBF_8BIT_YCBCR				//	1
		"ARGB",								//	NTV2_FBF_ARGB					//	2
		"RGBA",								//	NTV2_FBF_RGBA					//	3
		"10BIT_RGB",						//	NTV2_FBF_10BIT_RGB				//	4
		"8BIT_YCBCR_YUY2",					//	NTV2_FBF_8BIT_YCBCR_YUY2		//	5
		"ABGR",								//	NTV2_FBF_ABGR					//	6
		"10BIT_DPX",						//	NTV2_FBF_10BIT_DPX				//	7
		"10BIT_YCBCR_DPX",					//	NTV2_FBF_10BIT_YCBCR_DPX		//	8
		"",									//	NTV2_FBF_8BIT_DVCPRO			//	9
		"I420",								//	NTV2_FBF_8BIT_YCBCR_420PL3		//	10
		"",									//	NTV2_FBF_8BIT_HDV				//	11
		"24BIT_RGB",						//	NTV2_FBF_24BIT_RGB				//	12
		"24BIT_BGR",						//	NTV2_FBF_24BIT_BGR				//	13
		"",									//	NTV2_FBF_10BIT_YCBCRA			//	14
        "DPX_LITTLEENDIAN",					//	NTV2_FBF_10BIT_DPX_LE           //	15
		"48BIT_RGB",						//	NTV2_FBF_48BIT_RGB				//	16
		"",									//	NTV2_FBF_PRORES					//	17
		"",									//	NTV2_FBF_PRORES_DVCPRO			//	18
		"",									//	NTV2_FBF_PRORES_HDV				//	19
		"",									//	NTV2_FBF_10BIT_RGB_PACKED		//	20
		"",									//	NTV2_FBF_10BIT_ARGB				//	21
		"",									//	NTV2_FBF_16BIT_ARGB				//	22
		"",									//	NTV2_FBF_8BIT_YCBCR_422PL3		//	23
		"10BIT_RAW_RGB",					//	NTV2_FBF_10BIT_RAW_RGB			//	24
		"10BIT_RAW_YCBCR"					//	NTV2_FBF_10BIT_RAW_YCBCR		//	25
	};
#endif	//	!defined (NTV2_DEPRECATE)



#if !defined (NTV2_DEPRECATE)
	//	More UI-friendly versions of above (used in Cables app)...
	AJA_LOCAL_STATIC const char * frameBufferFormats [NTV2_FBF_NUMFRAMEBUFFERFORMATS+1] =
	{
		"10 Bit YCbCr",						//	NTV2_FBF_10BIT_YCBCR			//	0
		"8 Bit YCbCr - UYVY",				//	NTV2_FBF_8BIT_YCBCR				//	1
		"8 Bit ARGB",						//	NTV2_FBF_ARGB					//	2
		"8 Bit RGBA",						//	NTV2_FBF_RGBA					//	3
		"10 Bit RGB",						//	NTV2_FBF_10BIT_RGB				//	4
		"8 Bit YCbCr - YUY2",				//	NTV2_FBF_8BIT_YCBCR_YUY2		//	5
		"8 Bit ABGR",						//	NTV2_FBF_ABGR					//	6
		"10 Bit RGB - DPX compatible",		//	NTV2_FBF_10BIT_DPX				//	7
		"10 Bit YCbCr - DPX compatible",	//	NTV2_FBF_10BIT_YCBCR_DPX		//	8
		"8 Bit DVCPro YCbCr - UYVY",		//	NTV2_FBF_8BIT_DVCPRO			//	9
		"8 Bit YCbCr 420 3-plane [I420]",	//	NTV2_FBF_8BIT_YCBCR_420PL3		//	10
		"8 Bit HDV YCbCr - UYVY",			//	NTV2_FBF_8BIT_HDV				//	11
		"24 Bit RGB",						//	NTV2_FBF_24BIT_RGB				//	12
		"24 Bit BGR",						//	NTV2_FBF_24BIT_BGR				//	13
		"10 Bit YCbCrA",					//	NTV2_FBF_10BIT_YCBCRA			//	14
		"10 Bit RGB - DPX LE",              //	NTV2_FBF_10BIT_DPX_LE           //	15
		"48 Bit RGB",						//	NTV2_FBF_48BIT_RGB				//	16
		"10 Bit YCbCr - Compressed",		//	NTV2_FBF_PRORES					//	17
		"10 Bit YCbCr DVCPro - Compressed",	//	NTV2_FBF_PRORES_DVCPRO			//	18
		"10 Bit YCbCr HDV - Compressed",	//	NTV2_FBF_PRORES_HDV				//	19
		"10 Bit RGB Packed",				//	NTV2_FBF_10BIT_RGB_PACKED		//	20
		"10 Bit ARGB",						//	NTV2_FBF_10BIT_ARGB				//	21
		"16 Bit ARGB",						//	NTV2_FBF_16BIT_ARGB				//	22
		"8 Bit YCbCr 422 3-plane [Y42B]",	//	NTV2_FBF_8BIT_YCBCR_422PL3		//	23
		"10 Bit Raw RGB",					//	NTV2_FBF_10BIT_RGB				//	24
		"10 Bit Raw YCbCr",					//	NTV2_FBF_10BIT_YCBCR			//	25
		"10 Bit YCbCr 420 3-plane LE",		//	NTV2_FBF_10BIT_YCBCR_420PL3_LE	//	26
		"10 Bit YCbCr 422 3-plane LE",		//	NTV2_FBF_10BIT_YCBCR_422PL3_LE	//	27
		"10 Bit YCbCr 420 2-Plane",			//	NTV2_FBF_10BIT_YCBCR_420PL2		//	28
		"10 Bit YCbCr 422 2-Plane",			//	NTV2_FBF_10BIT_YCBCR_422PL2		//	29
		"8 Bit YCbCr 420 2-Plane",			//	NTV2_FBF_8BIT_YCBCR_420PL2		//	30
		"8 Bit YCbCr 422 2-Plane",			//	NTV2_FBF_8BIT_YCBCR_422PL2		//	31
		""									//	NTV2_FBF_INVALID				//	32
	};
#endif	//	!defined (NTV2_DEPRECATE)


//	More UI-friendly versions of above (used in Cables app)...
AJA_LOCAL_STATIC const char * m31Presets [M31_NUMVIDEOPRESETS] =
{
    "FILE 720x480 420 Planar 8 Bit 59.94i",             // M31_FILE_720X480_420_8_5994i         // 0
    "FILE 720x480 420 Planar 8 Bit 59.94p",             // M31_FILE_720X480_420_8_5994p         // 1
    "FILE 720x480 420 Planar 8 Bit 60i",                // M31_FILE_720X480_420_8_60i           // 2
    "FILE 720x480 420 Planar 8 Bit 60p",                // M31_FILE_720X480_420_8_60p           // 3
    "FILE 720x480 422 Planar 10 Bit 59.94i",            // M31_FILE_720X480_422_10_5994i        // 4
    "FILE 720x480 422 Planar 10 Bit 59.94p",            // M31_FILE_720X480_422_10_5994p        // 5
    "FILE 720x480 422 Planar 10 Bit 60i",               // M31_FILE_720X480_422_10_60i          // 6
    "FILE 720x480 422 Planar 10 Bit 60p",               // M31_FILE_720X480_422_10_60p          // 7

    "FILE 720x576 420 Planar 8 Bit 50i",                // M31_FILE_720X576_420_8_50i           // 8
    "FILE 720x576 420 Planar 8 Bit 50p",                // M31_FILE_720X576_420_8_50p           // 9
    "FILE 720x576 422 Planar 10 Bit 50i",               // M31_FILE_720X576_422_10_50i          // 10
    "FILE 720x576 422 Planar 10 Bit 50p",               // M31_FILE_720X576_422_10_50p          // 11

    "FILE 1280x720 420 Planar 8 Bit 2398p",             // M31_FILE_1280X720_420_8_2398p        // 12
    "FILE 1280x720 420 Planar 8 Bit 24p",               // M31_FILE_1280X720_420_8_24p          // 13
    "FILE 1280x720 420 Planar 8 Bit 25p",               // M31_FILE_1280X720_420_8_25p          // 14
    "FILE 1280x720 420 Planar 8 Bit 29.97p",            // M31_FILE_1280X720_420_8_2997p        // 15
    "FILE 1280x720 420 Planar 8 Bit 30p",               // M31_FILE_1280X720_420_8_30p          // 16
    "FILE 1280x720 420 Planar 8 Bit 50p",               // M31_FILE_1280X720_420_8_50p          // 17
    "FILE 1280x720 420 Planar 8 Bit 59.94p",            // M31_FILE_1280X720_420_8_5994p        // 18
    "FILE 1280x720 420 Planar 8 Bit 60p",               // M31_FILE_1280X720_420_8_60p          // 19
    
    "FILE 1280x720 422 Planar 10 Bit 2398p",            // M31_FILE_1280X720_422_10_2398p       // 20
    "FILE 1280x720 422 Planar 10 Bit 25p",              // M31_FILE_1280X720_422_10_24p         // 21
    "FILE 1280x720 422 Planar 10 Bit 25p",              // M31_FILE_1280X720_422_10_25p         // 22
    "FILE 1280x720 422 Planar 10 Bit 29.97p",           // M31_FILE_1280X720_422_10_2997p       // 23
    "FILE 1280x720 422 Planar 10 Bit 30p",              // M31_FILE_1280X720_422_10_30p         // 24
    "FILE 1280x720 422 Planar 10 Bit 50p",              // M31_FILE_1280X720_422_10_50p         // 25
    "FILE 1280x720 422 Planar 10 Bit 59.94p",           // M31_FILE_1280X720_422_10_5994p       // 26
    "FILE 1280x720 422 Planar 10 Bit 60p",              // M31_FILE_1280X720_422_10_60p         // 27

    "FILE 1920x1080 420 Planar 8 Bit 2398p",            // M31_FILE_1920X1080_420_8_2398p       // 28
    "FILE 1920x1080 420 Planar 8 Bit 24p",              // M31_FILE_1920X1080_420_8_24p         // 29
    "FILE 1920x1080 420 Planar 8 Bit 25p",              // M31_FILE_1920X1080_420_8_25p         // 30
    "FILE 1920x1080 420 Planar 8 Bit 29.97p",           // M31_FILE_1920X1080_420_8_2997p       // 31
    "FILE 1920x1080 420 Planar 8 Bit 30p",              // M31_FILE_1920X1080_420_8_30p         // 32
    "FILE 1920x1080 420 Planar 8 Bit 50i",              // M31_FILE_1920X1080_420_8_50i         // 33
    "FILE 1920x1080 420 Planar 8 Bit 50p",              // M31_FILE_1920X1080_420_8_50p         // 34
    "FILE 1920x1080 420 Planar 8 Bit 59.94i",           // M31_FILE_1920X1080_420_8_5994i       // 35
    "FILE 1920x1080 420 Planar 8 Bit 59.94p",           // M31_FILE_1920X1080_420_8_5994p       // 36
    "FILE 1920x1080 420 Planar 8 Bit 60i",              // M31_FILE_1920X1080_420_8_60i         // 37
    "FILE 1920x1080 420 Planar 8 Bit 60p",              // M31_FILE_1920X1080_420_8_60p         // 38
    
    "FILE 1920x1080 422 Planar 10 Bit 2398p",           // M31_FILE_1920X1080_422_10_2398p      // 39
    "FILE 1920x1080 422 Planar 10 Bit 24p",             // M31_FILE_1920X1080_422_10_24p        // 40
    "FILE 1920x1080 422 Planar 10 Bit 25p",             // M31_FILE_1920X1080_422_10_25p        // 41
    "FILE 1920x1080 422 Planar 10 Bit 29.97p",          // M31_FILE_1920X1080_422_10_2997p      // 42
    "FILE 1920x1080 422 Planar 10 Bit 30p",             // M31_FILE_1920X1080_422_10_30p        // 43
    "FILE 1920x1080 422 Planar 10 Bit 50i",             // M31_FILE_1920X1080_422_10_50i        // 44
    "FILE 1920x1080 422 Planar 10 Bit 50p",             // M31_FILE_1920X1080_422_10_50p        // 45
    "FILE 1920x1080 422 Planar 10 Bit 59.94i",          // M31_FILE_1920X1080_422_10_5994i      // 46
    "FILE 1920x1080 422 Planar 10 Bit 59.94p",          // M31_FILE_1920X1080_422_10_5994p      // 47
    "FILE 1920x1080 422 Planar 10 Bit 60i",             // M31_FILE_1920X1080_422_10_60i        // 48
    "FILE 1920x1080 422 Planar 10 Bit 60p",             // M31_FILE_1920X1080_422_10_60p        // 49

    "FILE 2048x1080 420 Planar 8 Bit 2398p",            // M31_FILE_2048X1080_420_8_2398p       // 50
    "FILE 2048x1080 420 Planar 8 Bit 24p",              // M31_FILE_2048X1080_420_8_24p         // 51
    "FILE 2048x1080 420 Planar 8 Bit 25p",              // M31_FILE_2048X1080_420_8_25p         // 52
    "FILE 2048x1080 420 Planar 8 Bit 29.97p",           // M31_FILE_2048X1080_420_8_2997p       // 53
    "FILE 2048x1080 420 Planar 8 Bit 30p",              // M31_FILE_2048X1080_420_8_30p         // 54
    "FILE 2048x1080 420 Planar 8 Bit 50p",              // M31_FILE_2048X1080_420_8_50p         // 55
    "FILE 2048x1080 420 Planar 8 Bit 59.94p",           // M31_FILE_2048X1080_420_8_5994p       // 56
    "FILE 2048x1080 420 Planar 8 Bit 60p",              // M31_FILE_2048X1080_420_8_60p         // 57
    
    "FILE 2048x1080 422 Planar 10 Bit 2398p",           // M31_FILE_2048X1080_422_10_2398p      // 58
    "FILE 2048x1080 422 Planar 10 Bit 24p",             // M31_FILE_2048X1080_422_10_24p        // 59
    "FILE 2048x1080 422 Planar 10 Bit 25p",             // M31_FILE_2048X1080_422_10_25p        // 60
    "FILE 2048x1080 422 Planar 10 Bit 29.97p",          // M31_FILE_2048X1080_422_10_2997p      // 61
    "FILE 2048x1080 422 Planar 10 Bit 30p",             // M31_FILE_2048X1080_422_10_30p        // 62
    "FILE 2048x1080 422 Planar 10 Bit 50p",             // M31_FILE_2048X1080_422_10_50p        // 63
    "FILE 2048x1080 422 Planar 10 Bit 59.94p",          // M31_FILE_2048X1080_422_10_5994p      // 64
    "FILE 2048x1080 422 Planar 10 Bit 60p",             // M31_FILE_2048X1080_422_10_60p        // 65

    "FILE 3840x2160 420 Planar 8 Bit 2398p",            // M31_FILE_3840X2160_420_8_2398p       // 66
    "FILE 3840x2160 420 Planar 8 Bit 24p",              // M31_FILE_3840X2160_420_8_24p         // 67
    "FILE 3840x2160 420 Planar 8 Bit 25p",              // M31_FILE_3840X2160_420_8_25p         // 68
    "FILE 3840x2160 420 Planar 8 Bit 29.97p",           // M31_FILE_3840X2160_420_8_2997p       // 69
    "FILE 3840x2160 420 Planar 8 Bit 30p",              // M31_FILE_3840X2160_420_8_30p         // 70
    "FILE 3840x2160 420 Planar 8 Bit 50p",              // M31_FILE_3840X2160_420_8_50p         // 71
    "FILE 3840x2160 420 Planar 8 Bit 59.94p",           // M31_FILE_3840X2160_420_8_5994p       // 72
    "FILE 3840x2160 420 Planar 8 Bit 60p",              // M31_FILE_3840X2160_420_8_60p         // 73

    "FILE 3840x2160 420 Planar 10 Bit 50p",             // M31_FILE_3840X2160_420_10_50p        // 74
    "FILE 3840x2160 420 Planar 10 Bit 59.94p",          // M31_FILE_3840X2160_420_10_5994p      // 75
    "FILE 3840x2160 420 Planar 10 Bit 60p",             // M31_FILE_3840X2160_420_10_60p        // 76
  
    "FILE 3840x2160 422 Planar 8 Bit 2398p",            // M31_FILE_3840X2160_422_8_2398p       // 77
    "FILE 3840x2160 422 Planar 8 Bit 24p",              // M31_FILE_3840X2160_422_8_24p         // 78
    "FILE 3840x2160 422 Planar 8 Bit 25p",              // M31_FILE_3840X2160_422_8_25p         // 79
    "FILE 3840x2160 422 Planar 8 Bit 29.97p",           // M31_FILE_3840X2160_422_8_2997p       // 80
    "FILE 3840x2160 422 Planar 8 Bit 30p",              // M31_FILE_3840X2160_422_8_30p         // 81
    "FILE 3840x2160 422 Planar 8 Bit 50p",              // M31_FILE_3840X2160_422_8_60p         // 82
    "FILE 3840x2160 422 Planar 8 Bit 59.94p",           // M31_FILE_3840X2160_422_8_5994p       // 83
    "FILE 3840x2160 422 Planar 8 Bit 60p",              // M31_FILE_3840X2160_422_8_60p         // 84
    
    "FILE 3840x2160 422 Planar 10 Bit 2398p",           // M31_FILE_3840X2160_422_10_2398p      // 85
    "FILE 3840x2160 422 Planar 10 Bit 24p",             // M31_FILE_3840X2160_422_10_24p        // 86
    "FILE 3840x2160 422 Planar 10 Bit 25p",             // M31_FILE_3840X2160_422_10_25p        // 87
    "FILE 3840x2160 422 Planar 10 Bit 29.97p",          // M31_FILE_3840X2160_422_10_2997p      // 88
    "FILE 3840x2160 422 Planar 10 Bit 30p",             // M31_FILE_3840X2160_422_10_30p        // 89
    "FILE 3840x2160 422 Planar 10 Bit 50p",             // M31_FILE_3840X2160_422_10_50p        // 90
    "FILE 3840x2160 422 Planar 10 Bit 59.94p",          // M31_FILE_3840X2160_422_10_5994p      // 91
    "FILE 3840x2160 422 Planar 10 Bit 60p",             // M31_FILE_3840X2160_422_10_60p        // 92
    
    "FILE 4096x2160 420 Planar 10 Bit 5994p",           // M31_FILE_4096X2160_420_10_5994p,     // 93
    "FILE 4096x2160 420 Planar 10 Bit 60p",             // M31_FILE_4096X2160_420_10_60p,       // 94
    "FILE 4096x2160 422 Planar 10 Bit 50p",             // M31_FILE_4096X2160_422_10_50p,       // 95
	"FILE 4096x2160 422 Planar 10 Bit 5994p IOnly",     // M31_FILE_4096X2160_422_10_5994p_IF,  // 96
	"FILE 4096x2160 422 Planar 10 Bit 60p IOnly",       // M31_FILE_4096X2160_422_10_60p_IF,    // 97
    
    "VIF 720x480 420 Planar 8 Bit 59.94i",              // M31_VIF_720X480_420_8_5994i          // 98
    "VIF 720x480 420 Planar 8 Bit 59.94p",              // M31_VIF_720X480_420_8_5994p          // 99
    "VIF 720x480 420 Planar 8 Bit 60i",                 // M31_VIF_720X480_420_8_60i            // 100
    "VIF 720x480 420 Planar 8 Bit 60p",                 // M31_VIF_720X480_420_8_60p            // 101
    "VIF 720x480 422 Planar 10 Bit 59.94i",             // M31_VIF_720X480_422_10_5994i         // 102
    "VIF 720x480 422 Planar 10 Bit 59.94p",             // M31_VIF_720X480_422_10_5994p         // 103
    "VIF 720x480 422 Planar 10 Bit 60i",                // M31_VIF_720X480_422_10_60i           // 104
    "VIF 720x480 422 Planar 10 Bit 60p",                // M31_VIF_720X480_422_10_60p           // 105

    "VIF 720x576 420 Planar 8 Bit 50i",                 // M31_VIF_720X576_420_8_50i            // 106
    "VIF 720x576 420 Planar 8 Bit 50p",                 // M31_VIF_720X576_420_8_50p            // 107
    "VIF 720x576 422 Planar 10 Bit 50i",                // M31_VIF_720X576_422_10_50i           // 108
    "VIF 720x576 422 Planar 10 Bit 50p",                // M31_VIF_720X576_422_10_50p           // 109

    "VIF 1280x720 420 Planar 8 Bit 50p",                // M31_VIF_1280X720_420_8_50p           // 110
    "VIF 1280x720 420 Planar 8 Bit 59.94p",             // M31_VIF_1280X720_420_8_5994p         // 111
    "VIF 1280x720 420 Planar 8 Bit 60p",                // M31_VIF_1280X720_420_8_60p           // 112
    "VIF 1280x720 422 Planar 10 Bit 50p",               // M31_VIF_1280X720_422_10_50p          // 113
    "VIF 1280x720 422 Planar 10 Bit 59.94p",            // M31_VIF_1280X720_422_10_5994p        // 114
    "VIF 1280x720 422 Planar 10 Bit 60p",               // M31_VIF_1280X720_422_10_60p          // 115

    "VIF 1920x1080 420 Planar 8 Bit 50i",               // M31_VIF_1920X1080_420_8_50i          // 116
    "VIF 1920x1080 420 Planar 8 Bit 50p",               // M31_VIF_1920X1080_420_8_50p          // 117
    "VIF 1920x1080 420 Planar 8 Bit 59.94i",            // M31_VIF_1920X1080_420_8_5994i        // 118
    "VIF 1920x1080 420 Planar 8 Bit 59.94p",            // M31_VIF_1920X1080_420_8_5994p        // 119
    "VIF 1920x1080 420 Planar 8 Bit 60i",               // M31_VIF_1920X1080_420_8_60i          // 120
    "VIF 1920x1080 420 Planar 8 Bit 60p",               // M31_VIF_1920X1080_420_8_60p          // 121
    "VIF 1920x1080 420 Planar 10 Bit 50i",              // M31_VIF_1920X1080_420_10_50i         // 122
    "VIF 1920x1080 420 Planar 10 Bit 50p",              // M31_VIF_1920X1080_420_10_50p         // 123
    "VIF 1920x1080 420 Planar 10 Bit 59.94i",           // M31_VIF_1920X1080_420_10_5994i       // 124
    "VIF 1920x1080 420 Planar 10 Bit 59.94p",           // M31_VIF_1920X1080_420_10_5994p       // 125
    "VIF 1920x1080 420 Planar 10 Bit 60i",              // M31_VIF_1920X1080_420_10_60i         // 126
    "VIF 1920x1080 420 Planar 10 Bit 60p",              // M31_VIF_1920X1080_420_10_60p         // 127
    "VIF 1920x1080 422 Planar 10 Bit 59.94i",           // M31_VIF_1920X1080_422_10_5994i       // 128
    "VIF 1920x1080 422 Planar 10 Bit 59.94p",           // M31_VIF_1920X1080_422_10_5994p       // 129
    "VIF 1920x1080 422 Planar 10 Bit 60i",              // M31_VIF_1920X1080_422_10_60i         // 130
    "VIF 1920x1080 422 Planar 10 Bit 60p",              // M31_VIF_1920X1080_422_10_60p         // 131
  
    "VIF 3840x2160 420 Planar 8 Bit 30p",               // M31_VIF_3840X2160_420_8_30p          // 132
    "VIF 3840x2160 420 Planar 8 Bit 50p",               // M31_VIF_3840X2160_420_8_50p          // 133
    "VIF 3840x2160 420 Planar 8 Bit 59.94p",            // M31_VIF_3840X2160_420_8_5994p        // 134
    "VIF 3840x2160 420 Planar 8 Bit 60p",               // M31_VIF_3840X2160_420_8_5994p        // 135
    "VIF 3840x2160 420 Planar 10 Bit 50p",              // M31_VIF_3840X2160_420_8_60p          // 136
    "VIF 3840x2160 420 Planar 10 Bit 59.94p",           // M31_VIF_3840X2160_420_8_60p          // 137
    "VIF 3840x2160 420 Planar 10 Bit 60p",              // M31_VIF_3840X2160_420_10_5994p       // 138
    
    "VIF 3840x2160 422 Planar 10 Bit 30p",              // M31_VIF_3840X2160_422_10_30p         // 139
    "VIF 3840x2160 422 Planar 10 Bit 50p",              // M31_VIF_3840X2160_422_10_50p         // 140
    "VIF 3840x2160 422 Planar 10 Bit 59.94p",           // M31_VIF_3840X2160_422_10_5994p       // 141
    "VIF 3840x2160 422 Planar 10 Bit 60p",              // M31_VIF_3840X2160_422_10_60p         // 142
};

#if !defined (NTV2_DEPRECATE)
AJA_LOCAL_STATIC const char * NTV2FrameRateStrings [NTV2_NUM_FRAMERATES] =
{
	"Unknown",							//	NTV2_FRAMERATE_UNKNOWN			//	0
	"60.00",							//	NTV2_FRAMERATE_6000				//	1
	"59.94",							//	NTV2_FRAMERATE_5994				//	2
	"30.00",							//	NTV2_FRAMERATE_3000				//	3
	"29.97",							//	NTV2_FRAMERATE_2997				//	4
	"25.00",							//	NTV2_FRAMERATE_2500				//	5
	"24.00",							//	NTV2_FRAMERATE_2400				//	6
	"23.98",							//	NTV2_FRAMERATE_2398				//	7
	"50.00",							//	NTV2_FRAMERATE_5000				//	8
	"48.00",							//	NTV2_FRAMERATE_4800				//	9
	"47.95",							//	NTV2_FRAMERATE_4795				//	10
	"120.00",							//	NTV2_FRAMERATE_12000			//	11
	"119.88",							//	NTV2_FRAMERATE_11988			//	12
	"15.00",							//	NTV2_FRAMERATE_1500				//	13
	"14.98",							//	NTV2_FRAMERATE_1498				//	14
	"19.00",							//	NTV2_FRAMERATE_1900				//	15
	"18.98",							//	NTV2_FRAMERATE_1898				//	16
	"18.00",							//	NTV2_FRAMERATE_1800				//	17
	"17.98"								//	NTV2_FRAMERATE_1798				//	18
};
#endif

// Extracts a channel pair or all channels from the
// NTV2 channel buffer that is retrieved from the hardware.
int RecordCopyAudio(PULWord pAja, PULWord pSR, int iStartSample, int iNumBytes, int iChan0,
                    int iNumChans, bool bKeepAudio24Bits)
{
    const int SAMPLE_SIZE = NTV2_NUMAUDIO_CHANNELS * NTV2_AUDIOSAMPLESIZE;

    // Insurance to prevent bogus array sizes causing havoc
//    if (iNumBytes > 48048)      // 23.98 == 2002 * 24
//        iNumBytes = 48048;

    // Adjust the offset of the first valid channel
    if (iStartSample)
    {
        iChan0 += (NTV2_NUMAUDIO_CHANNELS - iStartSample);
    }

    // Driver records audio to offset 24 bytes
    PULWord pIn = &pAja[NTV2_NUMAUDIO_CHANNELS];
    UWord * puwOut = reinterpret_cast<UWord*>(pSR);

    // If our transfer size has a remainder and our chans are in it,
    // adjust number samples
    int iNumSamples = iNumBytes / SAMPLE_SIZE;
    int iMod = (iNumBytes % SAMPLE_SIZE) / 4;
    if (iMod > iChan0)
        iNumSamples++;
    // else if we have remainder with chans && chans total > number of chans
    // reduce start offset by the number of chans
    else if (iMod && iChan0 >= NTV2_NUMAUDIO_CHANNELS)
    {
        iNumSamples++;
        iChan0 -= NTV2_NUMAUDIO_CHANNELS;
    }
    // else if no remainder but start sample adjustment gives more chans
    // than number of chans, drop the start offset back by num chans
    else if (iChan0 >= NTV2_NUMAUDIO_CHANNELS)
    {
        iChan0 -= NTV2_NUMAUDIO_CHANNELS;
    }

    // Copy incoming audio to the outgoing array
    if (bKeepAudio24Bits)
    {
        for (int s = 0; s < iNumSamples; s++)
        {
            for (int c = iChan0; c < iChan0 + iNumChans; c++)
            {
                *pSR++ = pIn[c];
            }

            pIn += NTV2_NUMAUDIO_CHANNELS;
        }
    }
    else    // convert audio to 16 bits
    {
        for (int s = 0; s < iNumSamples; s++)
        {
            for (int c = iChan0; c < iChan0 + iNumChans; c++)
            {
                *puwOut++ = UWord(pIn[c] >> 16);
            }

            pIn += NTV2_NUMAUDIO_CHANNELS;
        }
    }

    return iNumSamples;
}

#include "math.h"
// M_PI is defined on RedHat Linux 9 in math.h
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif


ULWord	AddAudioTone (	ULWord *		pAudioBuffer,
						ULWord &		inOutCurrentSample,
						const ULWord	inNumSamples,
						const double	inSampleRate,
						const double	inAmplitude,
						const double	inFrequency,
						const ULWord	inNumBits,
						const bool		inByteSwap,
						const ULWord	inNumChannels)
{
	double			j			(inOutCurrentSample);
	const double	cycleLength	(inSampleRate / inFrequency);
	const double	scale		(double (ULWord (1 << (inNumBits - 1))) - 1.0);

	if (pAudioBuffer)
	{
		for (ULWord i = 0;  i < inNumSamples;  i++)
		{
			const double	nextFloat	= double(::sin (j / cycleLength * (M_PI * 2.0)) * inAmplitude);
			ULWord		value		= static_cast <ULWord> ((nextFloat * scale) + double(0.5));

			if (inByteSwap)
				value = NTV2EndianSwap32 (value);
			//odprintf("%f",(float)(LWord)value/(float)0x7FFFFFFF);

			for (ULWord channel = 0;  channel < inNumChannels;  channel++)
				*pAudioBuffer++ = value;

			j += 1.0;
			if (j > cycleLength)
				j -= cycleLength;
			inOutCurrentSample++;
		}	//	for each sample
	}	//	if pAudioBuffer

	return inNumSamples * 4 * inNumChannels;

}	//	AddAudioTone (ULWord)


ULWord	AddAudioTone (	UWord *			pAudioBuffer,
						ULWord &		inOutCurrentSample,
						const ULWord	inNumSamples,
						const double	inSampleRate,
						const double	inAmplitude,
						const double	inFrequency,
						const ULWord	inNumBits,
						const bool		inByteSwap,
						const ULWord	inNumChannels)
{
	double			j			(inOutCurrentSample);
	const double	cycleLength	(inSampleRate / inFrequency);
	const double	scale		(double (ULWord (1 << (inNumBits - 1))) - 1.0);

	if (pAudioBuffer)
	{
		for (ULWord i = 0;  i < inNumSamples;  i++)
		{
			const double	nextFloat	= double(::sin (j / cycleLength * (M_PI * 2.0)) * inAmplitude);
			UWord		value		= static_cast <UWord> ((nextFloat * scale) + double(0.5));

			if (inByteSwap)
				value = NTV2EndianSwap16(value);

			for (ULWord channel = 0;  channel < inNumChannels;  channel++)
				*pAudioBuffer++ = value;

			j += 1.0;
			if (j > cycleLength)
				j -= cycleLength;
			inOutCurrentSample++;
		}	//	for each sample
	}	//	if pAudioBuffer

	return inNumSamples * 4 * inNumChannels;

}	//	AddAudioTone (UWord)


ULWord	AddAudioTone (	ULWord *		pAudioBuffer,
						ULWord &		inOutCurrentSample,
						const ULWord	inNumSamples,
						const double	inSampleRate,
						const double *	pInAmplitudes,
						const double *	pInFrequencies,
						const ULWord	inNumBits,
						const bool		inByteSwap,
						const ULWord	inNumChannels)
{
	double			j [kNumAudioChannelsMax];
	double			cycleLength [kNumAudioChannelsMax];
	const double	scale		(double (ULWord (1 << (inNumBits - 1))) - 1.0);

	for (ULWord channel (0);  channel < inNumChannels;  channel++)
	{
		cycleLength[channel] = inSampleRate / pInFrequencies[channel];
		j [channel] = inOutCurrentSample;
	}

	if (pAudioBuffer && pInAmplitudes && pInFrequencies)
	{
		for (ULWord i (0);  i < inNumSamples;  i++)
		{
			for (ULWord channel (0);  channel < inNumChannels;  channel++)
			{
				const double	nextFloat	= double(::sin (j[channel] / cycleLength[channel] * (M_PI * 2.0)) * pInAmplitudes[channel]);
				ULWord		value		= static_cast <ULWord> ((nextFloat * scale) + double(0.5));

				if (inByteSwap)
					value = NTV2EndianSwap32(value);

				*pAudioBuffer++ = value;

				j[channel] += 1.0;
				if (j[channel] > cycleLength[channel])
					j[channel] -= cycleLength[channel];

			}
			inOutCurrentSample++;
		}	//	for each sample
	}	//	if pAudioBuffer && pInFrequencies

	return inNumSamples * 4 * inNumChannels;

}	//	AddAudioTone (per-chl freq & ampl)


ULWord AddAudioTestPattern (ULWord *		pAudioBuffer,
							ULWord &		inOutCurrentSample,
							const ULWord	inNumSamples,
							const ULWord	inModulus,
							const bool		inEndianConvert,
							const ULWord	inNumChannels)
{

	for (ULWord i(0);  i < inNumSamples;  i++)
	{
		ULWord value ((inOutCurrentSample % inModulus) << 16);
		if (inEndianConvert)
			value = NTV2EndianSwap32(value);
		for (ULWord channel(0);  channel < inNumChannels;  channel++)
			*pAudioBuffer++ = value;
		inOutCurrentSample++;
	}
	return inNumSamples * 4 * inNumChannels;
}


std::string NTV2DeviceIDToString (const NTV2DeviceID inValue,	const bool inForRetailDisplay)
{
	switch (inValue)
	{
	#if defined (AJAMac) || defined (MSWindows)
        case DEVICE_ID_KONALHI:                 return inForRetailDisplay ?	"KONA LHi"                  : "KonaLHi";
        case DEVICE_ID_KONALHIDVI:              return inForRetailDisplay ?	"KONA LHi DVI"              : "KonaLHiDVI";
	#endif
	#if defined (AJAMac)
        case DEVICE_ID_IOEXPRESS:               return inForRetailDisplay ?	"IoExpress"                 : "IoExpress";
	#elif defined (MSWindows)
        case DEVICE_ID_IOEXPRESS:               return inForRetailDisplay ?	"KONA IoExpress"            : "IoExpress";
	#else
        case DEVICE_ID_KONALHI:                 return inForRetailDisplay ?	"KONA LHi"                  : "OEM LHi";
        case DEVICE_ID_KONALHIDVI:              return inForRetailDisplay ?	"KONA LHi DVI"              : "OEM LHi DVI";
        case DEVICE_ID_IOEXPRESS:               return inForRetailDisplay ?	"IoExpress"                 : "OEM IoExpress";
	#endif
        case DEVICE_ID_NOTFOUND:                return inForRetailDisplay ?	"AJA Device"                : "(Not Found)";
        case DEVICE_ID_CORVID1:                 return inForRetailDisplay ?	"Corvid 1"                  : "Corvid";
        case DEVICE_ID_CORVID22:                return inForRetailDisplay ?	"Corvid 22"                 : "Corvid22";
        case DEVICE_ID_CORVID3G:                return inForRetailDisplay ?	"Corvid 3G"                 : "Corvid3G";
        case DEVICE_ID_KONA3G:                  return inForRetailDisplay ?	"KONA 3G"                   : "Kona3G";
        case DEVICE_ID_KONA3GQUAD:              return inForRetailDisplay ?	"KONA 3G QUAD"              : "Kona3GQuad";	//	Used to be "KONA 3G" for retail display
        case DEVICE_ID_KONALHEPLUS:             return inForRetailDisplay ?	"KONA LHe+"                 : "KonaLHe+";
        case DEVICE_ID_IOXT:                    return inForRetailDisplay ?	"IoXT"                      : "IoXT";
        case DEVICE_ID_CORVID24:                return inForRetailDisplay ?	"Corvid 24"                 : "Corvid24";
        case DEVICE_ID_TTAP:                    return inForRetailDisplay ?	"T-TAP"                     : "TTap";
		case DEVICE_ID_IO4K:					return inForRetailDisplay ?	"Io4K"						: "Io4K";
		case DEVICE_ID_IO4KUFC:					return inForRetailDisplay ?	"Io4K UFC"					: "Io4KUfc";
		case DEVICE_ID_KONA4:					return inForRetailDisplay ?	"KONA 4"					: "Kona4";
		case DEVICE_ID_KONA4UFC:				return inForRetailDisplay ?	"KONA 4 UFC"				: "Kona4Ufc";
		case DEVICE_ID_CORVID88:				return inForRetailDisplay ?	"Corvid 88"					: "Corvid88";
		case DEVICE_ID_CORVID44:				return inForRetailDisplay ?	"Corvid 44"					: "Corvid44";
		case DEVICE_ID_CORVIDHEVC:				return inForRetailDisplay ?	"Corvid HEVC"				: "CorvidHEVC";
        case DEVICE_ID_KONAIP_2022:             return "KonaIP s2022";
        case DEVICE_ID_KONAIP_4CH_2SFP:			return "KonaIP s2022 2+2";
        case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:	return "KonaIP J2K 1I 1O";
        case DEVICE_ID_KONAIP_2TX_1SFP_J2K:		return "KonaIP J2K 2O";
        case DEVICE_ID_KONAIP_1RX_1TX_2110:     return "KonaIP s2110 1I 1O";
		case DEVICE_ID_CORVIDHBR:               return inForRetailDisplay ? "Corvid HB-R"               : "CorvidHBR";
        case DEVICE_ID_IO4KPLUS:				return inForRetailDisplay ? "Avid DNxIV"                : "Io4KPlus";
        case DEVICE_ID_IOIP_2022:				return inForRetailDisplay ? "Avid DNxIP s2022"          : "IoIP-s2022";
		case DEVICE_ID_IOIP_2110:				return inForRetailDisplay ? "Avid DNxIP s2110"          : "IoIP-s2110";
		case DEVICE_ID_IOIP_2110_RGB12:			return inForRetailDisplay ? "Avid DNxIP s2110_RGB12"	: "IoIP-s2110_RGB12";
		case DEVICE_ID_KONAIP_2110:             return "KonaIP s2110";
		case DEVICE_ID_KONAIP_2110_RGB12:		return "KonaIP s2110 RGB12";
		case DEVICE_ID_KONA1:					return inForRetailDisplay ? "Kona 1"					: "Kona1";
        case DEVICE_ID_KONAHDMI:				return inForRetailDisplay ? "Kona HDMI"					: "KonaHDMI";
		case DEVICE_ID_KONA5:					return inForRetailDisplay ?	"KONA 5"					: "Kona5";
		case DEVICE_ID_KONA5_2X4K:				return inForRetailDisplay ?	"KONA 5 2x4K"				: "Kona5-2x4K";
		case DEVICE_ID_KONA5_3DLUT:				return inForRetailDisplay ?	"KONA 5 3DLUT"				: "Kona5-3DLUT";
        case DEVICE_ID_KONA5_8KMK:				return inForRetailDisplay ?	"KONA 5 8KMK"				: "Kona5-8KMK";
		case DEVICE_ID_KONA5_OE1:				return "Kona5-OE1";
		case DEVICE_ID_KONA5_OE2:				return "Kona5-OE2";
		case DEVICE_ID_KONA5_OE3:				return "Kona5-OE3";
		case DEVICE_ID_KONA5_OE4:				return "Kona5-OE4";
		case DEVICE_ID_KONA5_OE5:				return "Kona5-OE5";
		case DEVICE_ID_KONA5_OE6:				return "Kona5-OE6";
		case DEVICE_ID_KONA5_OE7:				return "Kona5-OE7";
		case DEVICE_ID_KONA5_OE8:				return "Kona5-OE8";
		case DEVICE_ID_KONA5_OE9:				return "Kona5-OE9";
		case DEVICE_ID_KONA5_OE10:				return "Kona5-OE10";
		case DEVICE_ID_KONA5_OE11:				return "Kona5-OE11";
		case DEVICE_ID_KONA5_OE12:				return "Kona5-OE12";
		case DEVICE_ID_CORVID44_8KMK:			return inForRetailDisplay ?	"Corvid 44 8KMK"			: "Corvid44-8KMK";
		case DEVICE_ID_CORVID44_PLNR:			return inForRetailDisplay ?	"Corvid 44 PLNR"			: "Corvid44-PLNR";
		case DEVICE_ID_KONA5_8K:				return inForRetailDisplay ?	"KONA 5 8K"					: "Kona5-8K";
		case DEVICE_ID_CORVID44_8K:				return inForRetailDisplay ?	"Corvid 44 8K"				: "Corvid44-8K";
		case DEVICE_ID_CORVID44_2X4K:			return inForRetailDisplay ?	"Corvid 44 2x4K"			: "Corvid44-2x4K";
		case DEVICE_ID_TTAP_PRO:				return inForRetailDisplay ? "T-TAP Pro"					: "TTapPro";
		case DEVICE_ID_IOX3:					return "IoX3";
#if defined(_DEBUG)
#else
	    default:					break;
#endif
	}
	return inForRetailDisplay ?	"Unknown" : "???";
}


#if !defined (NTV2_DEPRECATE)
	void GetNTV2RetailBoardString (NTV2BoardID inBoardID, std::string & outString)
	{
		outString = ::NTV2DeviceIDToString (inBoardID, true);
	}

	NTV2BoardType GetNTV2BoardTypeForBoardID (NTV2BoardID inBoardID)
	{
		return BOARDTYPE_NTV2;
	}

	void GetNTV2BoardString (NTV2BoardID inBoardID, string & outName)
	{
		outName = ::NTV2DeviceIDToString (inBoardID);
	}

	std::string NTV2BoardIDToString (const NTV2BoardID inValue,	const bool inForRetailDisplay)
	{
		return NTV2DeviceIDToString (inValue, inForRetailDisplay);
	}

	string frameBufferFormatString (const NTV2FrameBufferFormat inFrameBufferFormat)
	{
		string	result;
		if (inFrameBufferFormat >= 0 && inFrameBufferFormat < NTV2_FBF_NUMFRAMEBUFFERFORMATS)
			result = frameBufferFormats [inFrameBufferFormat];
		return result;
	}
#endif	//	!defined (NTV2_DEPRECATE)


NTV2Channel GetNTV2ChannelForIndex (const ULWord inIndex)
{
	return inIndex < NTV2_MAX_NUM_CHANNELS ? NTV2Channel(inIndex) : NTV2_CHANNEL1;
}

ULWord GetIndexForNTV2Channel (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel) ? ULWord(inChannel) : 0;
}


NTV2Channel NTV2CrosspointToNTV2Channel (const NTV2Crosspoint inCrosspointChannel)
{
	switch (inCrosspointChannel)
	{
		case NTV2CROSSPOINT_CHANNEL1:	return NTV2_CHANNEL1;
		case NTV2CROSSPOINT_CHANNEL2:	return NTV2_CHANNEL2;
		case NTV2CROSSPOINT_INPUT1:		return NTV2_CHANNEL1;
		case NTV2CROSSPOINT_INPUT2:		return NTV2_CHANNEL2;
		case NTV2CROSSPOINT_MATTE:		return NTV2_CHANNEL_INVALID;
		case NTV2CROSSPOINT_FGKEY:		return NTV2_CHANNEL_INVALID;
		case NTV2CROSSPOINT_CHANNEL3:	return NTV2_CHANNEL3;
		case NTV2CROSSPOINT_CHANNEL4:	return NTV2_CHANNEL4;
		case NTV2CROSSPOINT_INPUT3:		return NTV2_CHANNEL3;
		case NTV2CROSSPOINT_INPUT4:		return NTV2_CHANNEL4;
		case NTV2CROSSPOINT_CHANNEL5:	return NTV2_CHANNEL5;
		case NTV2CROSSPOINT_CHANNEL6:	return NTV2_CHANNEL6;
		case NTV2CROSSPOINT_CHANNEL7:	return NTV2_CHANNEL7;
		case NTV2CROSSPOINT_CHANNEL8:	return NTV2_CHANNEL8;
		case NTV2CROSSPOINT_INPUT5:		return NTV2_CHANNEL5;
		case NTV2CROSSPOINT_INPUT6:		return NTV2_CHANNEL6;
		case NTV2CROSSPOINT_INPUT7:		return NTV2_CHANNEL7;
		case NTV2CROSSPOINT_INPUT8:		return NTV2_CHANNEL8;
		case NTV2CROSSPOINT_INVALID:	return NTV2_CHANNEL_INVALID;
	}
	return NTV2_CHANNEL_INVALID;
}


NTV2Crosspoint GetNTV2CrosspointChannelForIndex (const ULWord index)
{
	switch(index)
	{
		default:
		case 0:	return NTV2CROSSPOINT_CHANNEL1;
		case 1:	return NTV2CROSSPOINT_CHANNEL2;
		case 2:	return NTV2CROSSPOINT_CHANNEL3;
		case 3:	return NTV2CROSSPOINT_CHANNEL4;
		case 4:	return NTV2CROSSPOINT_CHANNEL5;
		case 5:	return NTV2CROSSPOINT_CHANNEL6;
		case 6:	return NTV2CROSSPOINT_CHANNEL7;
		case 7:	return NTV2CROSSPOINT_CHANNEL8;
	}
}

ULWord GetIndexForNTV2CrosspointChannel (const NTV2Crosspoint channel)
{
	switch(channel)
	{
		default:
		case NTV2CROSSPOINT_CHANNEL1:	return 0;
		case NTV2CROSSPOINT_CHANNEL2:	return 1;
		case NTV2CROSSPOINT_CHANNEL3:	return 2;
		case NTV2CROSSPOINT_CHANNEL4:	return 3;
		case NTV2CROSSPOINT_CHANNEL5:	return 4;
		case NTV2CROSSPOINT_CHANNEL6:	return 5;
		case NTV2CROSSPOINT_CHANNEL7:	return 6;
		case NTV2CROSSPOINT_CHANNEL8:	return 7;
	}
}

NTV2Crosspoint GetNTV2CrosspointInputForIndex (const ULWord index)
{
	switch(index)
	{
		default:
		case 0:	return NTV2CROSSPOINT_INPUT1;
		case 1:	return NTV2CROSSPOINT_INPUT2;
		case 2:	return NTV2CROSSPOINT_INPUT3;
		case 3:	return NTV2CROSSPOINT_INPUT4;
		case 4:	return NTV2CROSSPOINT_INPUT5;
		case 5:	return NTV2CROSSPOINT_INPUT6;
		case 6:	return NTV2CROSSPOINT_INPUT7;
		case 7:	return NTV2CROSSPOINT_INPUT8;
	}
}

ULWord GetIndexForNTV2CrosspointInput (const NTV2Crosspoint channel)
{
	switch(channel)
	{
		default:
		case NTV2CROSSPOINT_INPUT1:	return 0;
		case NTV2CROSSPOINT_INPUT2:	return 1;
		case NTV2CROSSPOINT_INPUT3:	return 2;
		case NTV2CROSSPOINT_INPUT4:	return 3;
		case NTV2CROSSPOINT_INPUT5:	return 4;
		case NTV2CROSSPOINT_INPUT6:	return 5;
		case NTV2CROSSPOINT_INPUT7:	return 6;
		case NTV2CROSSPOINT_INPUT8:	return 7;
	}
}

NTV2Crosspoint GetNTV2CrosspointForIndex (const ULWord index)
{
	switch(index)
	{
		default:
		case 0:	return NTV2CROSSPOINT_CHANNEL1;
		case 1:	return NTV2CROSSPOINT_CHANNEL2;
		case 2:	return NTV2CROSSPOINT_CHANNEL3;
		case 3:	return NTV2CROSSPOINT_CHANNEL4;
		case 4:	return NTV2CROSSPOINT_INPUT1;
		case 5:	return NTV2CROSSPOINT_INPUT2;
		case 6:	return NTV2CROSSPOINT_INPUT3;
		case 7:	return NTV2CROSSPOINT_INPUT4;
		case 8:	return NTV2CROSSPOINT_CHANNEL5;
		case 9:	return NTV2CROSSPOINT_CHANNEL6;
		case 10:return NTV2CROSSPOINT_CHANNEL7;
		case 11:return NTV2CROSSPOINT_CHANNEL8;
		case 12:return NTV2CROSSPOINT_INPUT5;
		case 13:return NTV2CROSSPOINT_INPUT6;
		case 14:return NTV2CROSSPOINT_INPUT7;
		case 15:return NTV2CROSSPOINT_INPUT8;
	}
}

ULWord GetIndexForNTV2Crosspoint (const NTV2Crosspoint channel)
{
	switch(channel)
	{
		default:
		case NTV2CROSSPOINT_CHANNEL1:	return 0;
		case NTV2CROSSPOINT_CHANNEL2:	return 1;
		case NTV2CROSSPOINT_CHANNEL3:	return 2;
		case NTV2CROSSPOINT_CHANNEL4:	return 3;
		case NTV2CROSSPOINT_INPUT1:		return 4;
		case NTV2CROSSPOINT_INPUT2:		return 5;
		case NTV2CROSSPOINT_INPUT3:		return 6;
		case NTV2CROSSPOINT_INPUT4:		return 7;
		case NTV2CROSSPOINT_CHANNEL5:	return 8;
		case NTV2CROSSPOINT_CHANNEL6:	return 9;
		case NTV2CROSSPOINT_CHANNEL7:	return 10;
		case NTV2CROSSPOINT_CHANNEL8:	return 11;
		case NTV2CROSSPOINT_INPUT5:		return 12;
		case NTV2CROSSPOINT_INPUT6:		return 13;
		case NTV2CROSSPOINT_INPUT7:		return 14;
		case NTV2CROSSPOINT_INPUT8:		return 15;
	}
}


bool IsNTV2CrosspointInput (const NTV2Crosspoint inChannel)
{
	return NTV2_IS_INPUT_CROSSPOINT(inChannel);
}


bool IsNTV2CrosspointOutput (const NTV2Crosspoint inChannel)
{
	return NTV2_IS_OUTPUT_CROSSPOINT(inChannel);
}


NTV2EmbeddedAudioInput NTV2ChannelToEmbeddedAudioInput (const NTV2Channel inChannel)
{
	NTV2_ASSERT (NTV2Channel(NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1) == NTV2_CHANNEL1);
	NTV2_ASSERT (NTV2Channel(NTV2_MAX_NUM_EmbeddedAudioInputs) == NTV2_MAX_NUM_CHANNELS);
	return static_cast<NTV2EmbeddedAudioInput>(inChannel);
}


NTV2AudioSystem NTV2ChannelToAudioSystem (const NTV2Channel inChannel)
{
	NTV2_ASSERT (NTV2Channel(NTV2_AUDIOSYSTEM_1) == NTV2_CHANNEL1);
	NTV2_ASSERT (NTV2Channel(NTV2_MAX_NUM_AudioSystemEnums) == NTV2_MAX_NUM_CHANNELS);
	return static_cast<NTV2AudioSystem>(inChannel);
}


NTV2EmbeddedAudioInput NTV2InputSourceToEmbeddedAudioInput (const NTV2InputSource inInputSource)
{
	static const NTV2EmbeddedAudioInput	gInputSourceToEmbeddedAudioInputs []	= {	/* NTV2_INPUTSOURCE_ANALOG1 */	NTV2_MAX_NUM_EmbeddedAudioInputs,
																					/* NTV2_INPUTSOURCE_HDMI1 */	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1,
																					/* NTV2_INPUTSOURCE_HDMI2 */	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2,
																					/* NTV2_INPUTSOURCE_HDMI3 */	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3,
																					/* NTV2_INPUTSOURCE_HDMI4 */	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4,
																					/* NTV2_INPUTSOURCE_SDI1 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1,
																					/* NTV2_INPUTSOURCE_SDI2 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2,
																					/* NTV2_INPUTSOURCE_SDI3 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3,
																					/* NTV2_INPUTSOURCE_SDI4 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4,
																					/* NTV2_INPUTSOURCE_SDI5 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_5,
																					/* NTV2_INPUTSOURCE_SDI6 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_6,
																					/* NTV2_INPUTSOURCE_SDI7 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_7,
																					/* NTV2_INPUTSOURCE_SDI8 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_8,
																					/* NTV2_INPUTSOURCE_INVALID */	NTV2_MAX_NUM_EmbeddedAudioInputs};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  inInputSource < NTV2InputSource(sizeof(gInputSourceToEmbeddedAudioInputs) / sizeof(NTV2EmbeddedAudioInput)))
		return gInputSourceToEmbeddedAudioInputs [inInputSource];
	else
		return NTV2_MAX_NUM_EmbeddedAudioInputs;

}	//	InputSourceToEmbeddedAudioInput


NTV2AudioSource NTV2InputSourceToAudioSource (const NTV2InputSource inInputSource)
{
	if (!NTV2_IS_VALID_INPUT_SOURCE (inInputSource))
		return NTV2_AUDIO_SOURCE_INVALID;
	if (NTV2_INPUT_SOURCE_IS_SDI (inInputSource))
		return NTV2_AUDIO_EMBEDDED;
    else if (NTV2_INPUT_SOURCE_IS_HDMI (inInputSource))
		return NTV2_AUDIO_HDMI;
	else if (NTV2_INPUT_SOURCE_IS_ANALOG (inInputSource))
		return NTV2_AUDIO_ANALOG;
	return NTV2_AUDIO_SOURCE_INVALID;
}


NTV2Crosspoint NTV2ChannelToInputCrosspoint (const NTV2Channel inChannel)
{
	static const NTV2Crosspoint	gChannelToInputChannelSpec []	= {	NTV2CROSSPOINT_INPUT1,	NTV2CROSSPOINT_INPUT2,	NTV2CROSSPOINT_INPUT3,	NTV2CROSSPOINT_INPUT4,
																	NTV2CROSSPOINT_INPUT5,	NTV2CROSSPOINT_INPUT6,	NTV2CROSSPOINT_INPUT7,	NTV2CROSSPOINT_INPUT8, NTV2CROSSPOINT_INVALID};
	if (NTV2_IS_VALID_CHANNEL (inChannel))
		return gChannelToInputChannelSpec [inChannel];
	else
		return NTV2CROSSPOINT_INVALID;
}


NTV2Crosspoint NTV2ChannelToOutputCrosspoint (const NTV2Channel inChannel)
{
	static const NTV2Crosspoint	gChannelToOutputChannelSpec []	= {	NTV2CROSSPOINT_CHANNEL1,	NTV2CROSSPOINT_CHANNEL2,	NTV2CROSSPOINT_CHANNEL3,	NTV2CROSSPOINT_CHANNEL4,
																	NTV2CROSSPOINT_CHANNEL5,	NTV2CROSSPOINT_CHANNEL6,	NTV2CROSSPOINT_CHANNEL7,	NTV2CROSSPOINT_CHANNEL8, NTV2CROSSPOINT_INVALID};
	if (inChannel >= NTV2_CHANNEL1 && inChannel < NTV2_MAX_NUM_CHANNELS)
		return gChannelToOutputChannelSpec [inChannel];
	else
		return NTV2CROSSPOINT_INVALID;
}


INTERRUPT_ENUMS NTV2ChannelToInputInterrupt (const NTV2Channel inChannel)
{
	static const INTERRUPT_ENUMS	gChannelToInputInterrupt []	= {	eInput1, eInput2, eInput3, eInput4, eInput5, eInput6, eInput7, eInput8, eNumInterruptTypes};
	if (NTV2_IS_VALID_CHANNEL (inChannel))
		return gChannelToInputInterrupt [inChannel];
	else
		return eNumInterruptTypes;
}


INTERRUPT_ENUMS NTV2ChannelToOutputInterrupt (const NTV2Channel inChannel)
{
	static const INTERRUPT_ENUMS	gChannelToOutputInterrupt []	= {	eOutput1, eOutput2, eOutput3, eOutput4, eOutput5, eOutput6, eOutput7, eOutput8, eNumInterruptTypes};
	if (NTV2_IS_VALID_CHANNEL (inChannel))
		return gChannelToOutputInterrupt [inChannel];
	else
		return eNumInterruptTypes;
}


static const NTV2TCIndex gChanVITC1[]	= {	NTV2_TCINDEX_SDI1, NTV2_TCINDEX_SDI2, NTV2_TCINDEX_SDI3, NTV2_TCINDEX_SDI4, NTV2_TCINDEX_SDI5, NTV2_TCINDEX_SDI6, NTV2_TCINDEX_SDI7, NTV2_TCINDEX_SDI8};
static const NTV2TCIndex gChanVITC2[]	= {	NTV2_TCINDEX_SDI1_2, NTV2_TCINDEX_SDI2_2, NTV2_TCINDEX_SDI3_2, NTV2_TCINDEX_SDI4_2, NTV2_TCINDEX_SDI5_2, NTV2_TCINDEX_SDI6_2, NTV2_TCINDEX_SDI7_2, NTV2_TCINDEX_SDI8_2};
static const NTV2TCIndex gChanATCLTC[]	= {	NTV2_TCINDEX_SDI1_LTC, NTV2_TCINDEX_SDI2_LTC, NTV2_TCINDEX_SDI3_LTC, NTV2_TCINDEX_SDI4_LTC, NTV2_TCINDEX_SDI5_LTC, NTV2_TCINDEX_SDI6_LTC, NTV2_TCINDEX_SDI7_LTC, NTV2_TCINDEX_SDI8_LTC};


NTV2TCIndex NTV2ChannelToTimecodeIndex (const NTV2Channel inChannel, const bool inEmbeddedLTC, const bool inIsF2)
{
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inEmbeddedLTC ? gChanATCLTC[inChannel] : (inIsF2 ? gChanVITC2[inChannel] : gChanVITC1[inChannel]);
	return NTV2_TCINDEX_INVALID;
}


NTV2TCIndexes GetTCIndexesForSDIConnector (const NTV2Channel inSDI)
{
	NTV2TCIndexes	result;
	if (NTV2_IS_VALID_CHANNEL(inSDI))
		{result.insert(gChanVITC1[inSDI]);	result.insert(gChanVITC2[inSDI]);  result.insert(gChanATCLTC[inSDI]);}
	return result;
}


NTV2Channel NTV2TimecodeIndexToChannel (const NTV2TCIndex inTCIndex)
{
	static const NTV2Channel	gTCIndexToChannel []	= {	NTV2_CHANNEL_INVALID,	NTV2_CHANNEL1,	NTV2_CHANNEL2,	NTV2_CHANNEL3,	NTV2_CHANNEL4,	NTV2_CHANNEL1,	NTV2_CHANNEL2,
															NTV2_CHANNEL1,			NTV2_CHANNEL1,	NTV2_CHANNEL5,	NTV2_CHANNEL6,	NTV2_CHANNEL7,	NTV2_CHANNEL8,
															NTV2_CHANNEL3,			NTV2_CHANNEL4,	NTV2_CHANNEL5,	NTV2_CHANNEL6,	NTV2_CHANNEL7,	NTV2_CHANNEL8,	NTV2_CHANNEL_INVALID};
	return NTV2_IS_VALID_TIMECODE_INDEX (inTCIndex)  ?  gTCIndexToChannel [inTCIndex]  :  NTV2_CHANNEL_INVALID;
}


NTV2InputSource NTV2TimecodeIndexToInputSource (const NTV2TCIndex inTCIndex)
{
	static const NTV2InputSource	gTCIndexToInputSource []	= {	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_SDI1,	NTV2_INPUTSOURCE_SDI2,		NTV2_INPUTSOURCE_SDI3,		NTV2_INPUTSOURCE_SDI4,
																	NTV2_INPUTSOURCE_SDI1,		NTV2_INPUTSOURCE_SDI2,	NTV2_INPUTSOURCE_ANALOG1,	NTV2_INPUTSOURCE_ANALOG1,
																	NTV2_INPUTSOURCE_SDI5,		NTV2_INPUTSOURCE_SDI6,	NTV2_INPUTSOURCE_SDI7,		NTV2_INPUTSOURCE_SDI8,
																	NTV2_INPUTSOURCE_SDI3,		NTV2_INPUTSOURCE_SDI4,	NTV2_INPUTSOURCE_SDI5,		NTV2_INPUTSOURCE_SDI6,
																	NTV2_INPUTSOURCE_SDI7,		NTV2_INPUTSOURCE_SDI8,	NTV2_INPUTSOURCE_INVALID};
	return NTV2_IS_VALID_TIMECODE_INDEX (inTCIndex)  ?  gTCIndexToInputSource [inTCIndex]  :  NTV2_INPUTSOURCE_INVALID;
}


NTV2Crosspoint NTV2InputSourceToChannelSpec (const NTV2InputSource inInputSource)
{
	static const NTV2Crosspoint	gInputSourceToChannelSpec []	= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2CROSSPOINT_INPUT1,
																	/* NTV2_INPUTSOURCE_HDMI1 */		NTV2CROSSPOINT_INPUT1,
																	/* NTV2_INPUTSOURCE_HDMI2 */		NTV2CROSSPOINT_INPUT2,
																	/* NTV2_INPUTSOURCE_HDMI3 */		NTV2CROSSPOINT_INPUT3,
																	/* NTV2_INPUTSOURCE_HDMI4 */		NTV2CROSSPOINT_INPUT4,
																	/* NTV2_INPUTSOURCE_SDI1 */			NTV2CROSSPOINT_INPUT1,
																	/* NTV2_INPUTSOURCE_SDI2 */			NTV2CROSSPOINT_INPUT2,
																	/* NTV2_INPUTSOURCE_SDI3 */			NTV2CROSSPOINT_INPUT3,
																	/* NTV2_INPUTSOURCE_SDI4 */			NTV2CROSSPOINT_INPUT4,
																	/* NTV2_INPUTSOURCE_SDI5 */			NTV2CROSSPOINT_INPUT5,
																	/* NTV2_INPUTSOURCE_SDI6 */			NTV2CROSSPOINT_INPUT6,
																	/* NTV2_INPUTSOURCE_SDI7 */			NTV2CROSSPOINT_INPUT7,
																	/* NTV2_INPUTSOURCE_SDI8 */			NTV2CROSSPOINT_INPUT8,
																	/* NTV2_NUM_INPUTSOURCES */			NTV2_NUM_CROSSPOINTS};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  size_t (inInputSource) < sizeof (gInputSourceToChannelSpec) / sizeof (NTV2Channel))
		return gInputSourceToChannelSpec [inInputSource];
	else
		return NTV2_NUM_CROSSPOINTS;
	
}	//	NTV2InputSourceToChannelSpec


NTV2ReferenceSource NTV2InputSourceToReferenceSource (const NTV2InputSource inInputSource)
{
	static const NTV2ReferenceSource	gInputSourceToReferenceSource []	= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_REFERENCE_ANALOG_INPUT,
																				/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_REFERENCE_HDMI_INPUT1,
																				/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_REFERENCE_HDMI_INPUT2,
																				/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_REFERENCE_HDMI_INPUT3,
																				/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_REFERENCE_HDMI_INPUT4,
																				/* NTV2_INPUTSOURCE_SDI1 */			NTV2_REFERENCE_INPUT1,
																				/* NTV2_INPUTSOURCE_SDI2 */			NTV2_REFERENCE_INPUT2,
																				/* NTV2_INPUTSOURCE_SDI3 */			NTV2_REFERENCE_INPUT3,
																				/* NTV2_INPUTSOURCE_SDI4 */			NTV2_REFERENCE_INPUT4,
																				/* NTV2_INPUTSOURCE_SDI5 */			NTV2_REFERENCE_INPUT5,
																				/* NTV2_INPUTSOURCE_SDI6 */			NTV2_REFERENCE_INPUT6,
																				/* NTV2_INPUTSOURCE_SDI7 */			NTV2_REFERENCE_INPUT7,
																				/* NTV2_INPUTSOURCE_SDI8 */			NTV2_REFERENCE_INPUT8,
																				/* NTV2_NUM_INPUTSOURCES */			NTV2_NUM_REFERENCE_INPUTS};
	if (NTV2_IS_VALID_INPUT_SOURCE (inInputSource)  &&  size_t (inInputSource) < sizeof (gInputSourceToReferenceSource) / sizeof (NTV2ReferenceSource))
		return gInputSourceToReferenceSource [inInputSource];
	else
		return NTV2_NUM_REFERENCE_INPUTS;

}	//	NTV2InputSourceToReferenceSource


NTV2Channel NTV2InputSourceToChannel (const NTV2InputSource inInputSource)
{
	static const NTV2Channel	gInputSourceToChannel []	= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_CHANNEL1,
																/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_CHANNEL1,
																/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_CHANNEL2,
																/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_CHANNEL3,
																/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_CHANNEL4,
																/* NTV2_INPUTSOURCE_SDI1 */			NTV2_CHANNEL1,
																/* NTV2_INPUTSOURCE_SDI2 */			NTV2_CHANNEL2,
																/* NTV2_INPUTSOURCE_SDI3 */			NTV2_CHANNEL3,
																/* NTV2_INPUTSOURCE_SDI4 */			NTV2_CHANNEL4,
																/* NTV2_INPUTSOURCE_SDI5 */			NTV2_CHANNEL5,
																/* NTV2_INPUTSOURCE_SDI6 */			NTV2_CHANNEL6,
																/* NTV2_INPUTSOURCE_SDI7 */			NTV2_CHANNEL7,
																/* NTV2_INPUTSOURCE_SDI8 */			NTV2_CHANNEL8,
																/* NTV2_NUM_INPUTSOURCES */			NTV2_CHANNEL_INVALID};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  size_t (inInputSource) < sizeof (gInputSourceToChannel) / sizeof (NTV2Channel))
		return gInputSourceToChannel [inInputSource];
	else
		return NTV2_MAX_NUM_CHANNELS;

}	//	NTV2InputSourceToChannel


NTV2AudioSystem NTV2InputSourceToAudioSystem (const NTV2InputSource inInputSource)
{
	static const NTV2AudioSystem	gInputSourceToAudioSystem []	= {	/* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_AUDIOSYSTEM_1,
																		/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_AUDIOSYSTEM_1,
																		/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_AUDIOSYSTEM_2,
																		/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_AUDIOSYSTEM_3,
																		/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_AUDIOSYSTEM_4,
																		/* NTV2_INPUTSOURCE_SDI1 */			NTV2_AUDIOSYSTEM_1,
																		/* NTV2_INPUTSOURCE_SDI2 */			NTV2_AUDIOSYSTEM_2,
																		/* NTV2_INPUTSOURCE_SDI3 */			NTV2_AUDIOSYSTEM_3,
																		/* NTV2_INPUTSOURCE_SDI4 */			NTV2_AUDIOSYSTEM_4,
																		/* NTV2_INPUTSOURCE_SDI5 */			NTV2_AUDIOSYSTEM_5,
																		/* NTV2_INPUTSOURCE_SDI6 */			NTV2_AUDIOSYSTEM_6,
																		/* NTV2_INPUTSOURCE_SDI7 */			NTV2_AUDIOSYSTEM_7,
																		/* NTV2_INPUTSOURCE_SDI8 */			NTV2_AUDIOSYSTEM_8,
																		/* NTV2_NUM_INPUTSOURCES */			NTV2_NUM_AUDIOSYSTEMS};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  inInputSource < NTV2InputSource(sizeof(gInputSourceToAudioSystem) / sizeof(NTV2AudioSystem)))
		return gInputSourceToAudioSystem [inInputSource];
	else
		return NTV2_AUDIOSYSTEM_INVALID;

}	//	NTV2InputSourceToAudioSystem


NTV2TimecodeIndex NTV2InputSourceToTimecodeIndex (const NTV2InputSource inInputSource, const bool inEmbeddedLTC)
{
		static const NTV2TimecodeIndex	gInputSourceToTCIndex []= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_TCINDEX_LTC1,
																	/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_SDI1 */			NTV2_TCINDEX_SDI1,
																	/* NTV2_INPUTSOURCE_SDI2 */			NTV2_TCINDEX_SDI2,
																	/* NTV2_INPUTSOURCE_SDI3 */			NTV2_TCINDEX_SDI3,
																	/* NTV2_INPUTSOURCE_SDI4 */			NTV2_TCINDEX_SDI4,
																	/* NTV2_INPUTSOURCE_SDI5 */			NTV2_TCINDEX_SDI5,
																	/* NTV2_INPUTSOURCE_SDI6 */			NTV2_TCINDEX_SDI6,
																	/* NTV2_INPUTSOURCE_SDI7 */			NTV2_TCINDEX_SDI7,
																	/* NTV2_INPUTSOURCE_SDI8 */			NTV2_TCINDEX_SDI8,
																	/* NTV2_NUM_INPUTSOURCES */			NTV2_TCINDEX_INVALID};
		static const NTV2TimecodeIndex	gInputSourceToLTCIndex []= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_TCINDEX_LTC1,
																	/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_SDI1 */			NTV2_TCINDEX_SDI1_LTC,
																	/* NTV2_INPUTSOURCE_SDI2 */			NTV2_TCINDEX_SDI2_LTC,
																	/* NTV2_INPUTSOURCE_SDI3 */			NTV2_TCINDEX_SDI3_LTC,
																	/* NTV2_INPUTSOURCE_SDI4 */			NTV2_TCINDEX_SDI4_LTC,
																	/* NTV2_INPUTSOURCE_SDI5 */			NTV2_TCINDEX_SDI5_LTC,
																	/* NTV2_INPUTSOURCE_SDI6 */			NTV2_TCINDEX_SDI6_LTC,
																	/* NTV2_INPUTSOURCE_SDI7 */			NTV2_TCINDEX_SDI7_LTC,
																	/* NTV2_INPUTSOURCE_SDI8 */			NTV2_TCINDEX_SDI8_LTC,
																	/* NTV2_NUM_INPUTSOURCES */			NTV2_TCINDEX_INVALID};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  size_t (inInputSource) < sizeof (gInputSourceToTCIndex) / sizeof (NTV2TimecodeIndex))
		return inEmbeddedLTC ? gInputSourceToLTCIndex [inInputSource] : gInputSourceToTCIndex [inInputSource];
	else
		return NTV2_TCINDEX_INVALID;
}


NTV2InputSource NTV2ChannelToInputSource (const NTV2Channel inChannel, const NTV2InputSourceKinds inSourceType)
{
	static const NTV2InputSource	gChannelToSDIInputSource []	=	{	NTV2_INPUTSOURCE_SDI1,		NTV2_INPUTSOURCE_SDI2,		NTV2_INPUTSOURCE_SDI3,		NTV2_INPUTSOURCE_SDI4,
																		NTV2_INPUTSOURCE_SDI5,		NTV2_INPUTSOURCE_SDI6,		NTV2_INPUTSOURCE_SDI7,		NTV2_INPUTSOURCE_SDI8,
																		NTV2_INPUTSOURCE_INVALID	};
	static const NTV2InputSource	gChannelToHDMIInputSource[] =	{	NTV2_INPUTSOURCE_HDMI1,		NTV2_INPUTSOURCE_HDMI2,		NTV2_INPUTSOURCE_HDMI3,		NTV2_INPUTSOURCE_HDMI4,
																		NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,
																		NTV2_INPUTSOURCE_INVALID	};
	static const NTV2InputSource	gChannelToAnlgInputSource[] =	{	NTV2_INPUTSOURCE_ANALOG1,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,
																		NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,
																		NTV2_INPUTSOURCE_INVALID	};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		switch (inSourceType)
		{
			case NTV2_INPUTSOURCES_SDI:		return gChannelToSDIInputSource[inChannel];
			case NTV2_INPUTSOURCES_HDMI:	return gChannelToHDMIInputSource[inChannel];
			case NTV2_INPUTSOURCES_ANALOG:	return gChannelToAnlgInputSource[inChannel];
			default:						break;
		}
	return NTV2_INPUTSOURCE_INVALID;
}


NTV2Channel NTV2OutputDestinationToChannel (const NTV2OutputDestination inOutputDest)
{
	if (!NTV2_IS_VALID_OUTPUT_DEST (inOutputDest))
		return NTV2_CHANNEL_INVALID;

	#if defined (NTV2_DEPRECATE)
		static const NTV2Channel	gOutputDestToChannel []	=	{	NTV2_CHANNEL1,	NTV2_CHANNEL1,
																	NTV2_CHANNEL1,	NTV2_CHANNEL2,	NTV2_CHANNEL3,	NTV2_CHANNEL4,
																	NTV2_CHANNEL5,	NTV2_CHANNEL6,	NTV2_CHANNEL7,	NTV2_CHANNEL8,	NTV2_CHANNEL_INVALID	};
		return gOutputDestToChannel [inOutputDest];
	#else	//	else !defined (NTV2_DEPRECATE)
		switch (inOutputDest)
		{
			default:
			case NTV2_OUTPUTDESTINATION_SDI1:		return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_ANALOG:		return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_SDI2:		return NTV2_CHANNEL2;
			case NTV2_OUTPUTDESTINATION_HDMI:		return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_DUALLINK1:	return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_HDMI_14:	return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_DUALLINK2:	return NTV2_CHANNEL2;
			case NTV2_OUTPUTDESTINATION_SDI3:		return NTV2_CHANNEL3;
			case NTV2_OUTPUTDESTINATION_SDI4:		return NTV2_CHANNEL4;
			case NTV2_OUTPUTDESTINATION_SDI5:		return NTV2_CHANNEL5;
			case NTV2_OUTPUTDESTINATION_SDI6:		return NTV2_CHANNEL6;
			case NTV2_OUTPUTDESTINATION_SDI7:		return NTV2_CHANNEL7;
			case NTV2_OUTPUTDESTINATION_SDI8:		return NTV2_CHANNEL8;
			case NTV2_OUTPUTDESTINATION_DUALLINK3:	return NTV2_CHANNEL3;
			case NTV2_OUTPUTDESTINATION_DUALLINK4:	return NTV2_CHANNEL4;
			case NTV2_OUTPUTDESTINATION_DUALLINK5:	return NTV2_CHANNEL5;
			case NTV2_OUTPUTDESTINATION_DUALLINK6:	return NTV2_CHANNEL6;
			case NTV2_OUTPUTDESTINATION_DUALLINK7:	return NTV2_CHANNEL7;
			case NTV2_OUTPUTDESTINATION_DUALLINK8:	return NTV2_CHANNEL8;
		}
	#endif	//	else !defined (NTV2_DEPRECATE)
}


NTV2OutputDestination NTV2ChannelToOutputDestination (const NTV2Channel inChannel)
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return NTV2_OUTPUTDESTINATION_INVALID;

	#if defined (NTV2_DEPRECATE)
		static const NTV2OutputDestination	gChannelToOutputDest []	=	{	NTV2_OUTPUTDESTINATION_SDI1,	NTV2_OUTPUTDESTINATION_SDI2,	NTV2_OUTPUTDESTINATION_SDI3,	NTV2_OUTPUTDESTINATION_SDI4,
																			NTV2_OUTPUTDESTINATION_SDI5,	NTV2_OUTPUTDESTINATION_SDI6,	NTV2_OUTPUTDESTINATION_SDI7,	NTV2_OUTPUTDESTINATION_SDI8,
																			NTV2_NUM_OUTPUTDESTINATIONS	};
		return gChannelToOutputDest [inChannel];
	#else	//	else !defined (NTV2_DEPRECATE)
		switch (inChannel)
		{
			default:
			case NTV2_CHANNEL1:		return NTV2_OUTPUTDESTINATION_SDI1;
			case NTV2_CHANNEL2:		return NTV2_OUTPUTDESTINATION_SDI2;
			case NTV2_CHANNEL3:		return NTV2_OUTPUTDESTINATION_SDI3;
			case NTV2_CHANNEL4:		return NTV2_OUTPUTDESTINATION_SDI4;
			case NTV2_CHANNEL5:		return NTV2_OUTPUTDESTINATION_SDI5;
			case NTV2_CHANNEL6:		return NTV2_OUTPUTDESTINATION_SDI6;
			case NTV2_CHANNEL7:		return NTV2_OUTPUTDESTINATION_SDI7;
			case NTV2_CHANNEL8:		return NTV2_OUTPUTDESTINATION_SDI8;
		}
	#endif	//	else !defined (NTV2_DEPRECATE)
}


// if formats are transport equivalent (e.g. 1080i30 / 1080psf30) return the target version of the format
NTV2VideoFormat GetTransportCompatibleFormat (const NTV2VideoFormat inFormat, const NTV2VideoFormat inTargetFormat)
{
	// compatible return target version
	if (::IsTransportCompatibleFormat (inFormat, inTargetFormat))
		return inTargetFormat;

	// not compatible, return original format
	return inFormat;
}


// determine if 2 formats are transport compatible (e.g. 1080i30 / 1080psf30)
bool IsTransportCompatibleFormat (const NTV2VideoFormat inFormat1, const NTV2VideoFormat inFormat2)
{
	if (inFormat1 == inFormat2)
		return true;

	switch (inFormat1)
	{
		case NTV2_FORMAT_1080i_5000:		return inFormat2 == NTV2_FORMAT_1080psf_2500_2;
		case NTV2_FORMAT_1080i_5994:		return inFormat2 == NTV2_FORMAT_1080psf_2997_2;
		case NTV2_FORMAT_1080i_6000:		return inFormat2 == NTV2_FORMAT_1080psf_3000_2;
		case NTV2_FORMAT_1080psf_2500_2:	return inFormat2 == NTV2_FORMAT_1080i_5000;
		case NTV2_FORMAT_1080psf_2997_2:	return inFormat2 == NTV2_FORMAT_1080i_5994;
		case NTV2_FORMAT_1080psf_3000_2:	return inFormat2 == NTV2_FORMAT_1080i_6000;
		default:							return false;
	}
}


NTV2InputSource GetNTV2InputSourceForIndex (const ULWord inIndex0, const NTV2InputSourceKinds inKinds)
{
	static const NTV2InputSource	sSDIInputSources[]	= {	NTV2_INPUTSOURCE_SDI1,	NTV2_INPUTSOURCE_SDI2,	NTV2_INPUTSOURCE_SDI3,	NTV2_INPUTSOURCE_SDI4,
															NTV2_INPUTSOURCE_SDI5,	NTV2_INPUTSOURCE_SDI6,	NTV2_INPUTSOURCE_SDI7,	NTV2_INPUTSOURCE_SDI8};
	static const NTV2InputSource	sHDMIInputSources[]	= {	NTV2_INPUTSOURCE_HDMI1,	NTV2_INPUTSOURCE_HDMI2,	NTV2_INPUTSOURCE_HDMI3,	NTV2_INPUTSOURCE_HDMI4};
	static const NTV2InputSource	sANLGInputSources[]	= {	NTV2_INPUTSOURCE_ANALOG1 };
	switch (inKinds)
	{
		case NTV2_INPUTSOURCES_SDI:
			if (inIndex0 < sizeof(sSDIInputSources) / sizeof(NTV2InputSource))
				return sSDIInputSources[inIndex0];
			break;
		case NTV2_INPUTSOURCES_HDMI:
			if (inIndex0 < sizeof(sHDMIInputSources) / sizeof(NTV2InputSource))
				return sHDMIInputSources[inIndex0];
			break;
		case NTV2_INPUTSOURCES_ANALOG:
			if (inIndex0 < sizeof(sANLGInputSources) / sizeof(NTV2InputSource))
				return sANLGInputSources[inIndex0];
			break;
	#if defined(_DEBUG)
		case NTV2_INPUTSOURCES_NONE:
		case NTV2_INPUTSOURCES_ALL:
			break;
	#else
		default:	break;
	#endif
	}
	return NTV2_INPUTSOURCE_INVALID;
}


NTV2InputSource GetNTV2HDMIInputSourceForIndex (const ULWord inIndex0)	//	NTV2_SHOULD_BE_DEPRECATED
{
	return ::GetNTV2InputSourceForIndex(inIndex0, NTV2_INPUTSOURCES_HDMI);
}


ULWord GetIndexForNTV2InputSource (const NTV2InputSource inValue)
{
	static const ULWord	sInputSourcesIndexes []	= {	0,							//	NTV2_INPUTSOURCE_ANALOG1,
													0, 1, 2, 3,					//	NTV2_INPUTSOURCE_HDMI1 ... NTV2_INPUTSOURCE_HDMI4,
													0, 1, 2, 3, 4, 5, 6, 7 };	//	NTV2_INPUTSOURCE_SDI1 ... NTV2_INPUTSOURCE_SDI8
	if (static_cast <size_t> (inValue) < sizeof (sInputSourcesIndexes) / sizeof (ULWord))
		return sInputSourcesIndexes [inValue];
	else
		return 0xFFFFFFFF;

}	//	GetIndexForNTV2InputSource


ULWord NTV2FramesizeToByteCount (const NTV2Framesize inFrameSize)
{
	static ULWord	gFrameSizeToByteCount []	= {	2 /* NTV2_FRAMESIZE_2MB */,		4 /* NTV2_FRAMESIZE_4MB */,		8 /* NTV2_FRAMESIZE_8MB */,		16 /* NTV2_FRAMESIZE_16MB */,
													6 /* NTV2_FRAMESIZE_6MB */,		10 /* NTV2_FRAMESIZE_10MB */,	12 /* NTV2_FRAMESIZE_12MB */,	14 /* NTV2_FRAMESIZE_14MB */,
													18 /* NTV2_FRAMESIZE_18MB */,	20 /* NTV2_FRAMESIZE_20MB */,	22 /* NTV2_FRAMESIZE_22MB */,	24 /* NTV2_FRAMESIZE_24MB */,
													26 /* NTV2_FRAMESIZE_26MB */,	28 /* NTV2_FRAMESIZE_28MB */,	30 /* NTV2_FRAMESIZE_30MB */,	32 /* NTV2_FRAMESIZE_32MB */,
													0	};
	if (inFrameSize < NTV2_MAX_NUM_Framesizes  &&  inFrameSize < NTV2Framesize(sizeof(gFrameSizeToByteCount) / sizeof(ULWord)))
		return gFrameSizeToByteCount [inFrameSize] * 1024 * 1024;
	else
		return 0;

}	//	NTV2FramesizeToByteCount


ULWord NTV2AudioBufferSizeToByteCount (const NTV2AudioBufferSize inBufferSize)
{													//	NTV2_AUDIO_BUFFER_STANDARD	NTV2_AUDIO_BUFFER_BIG	NTV2_AUDIO_BUFFER_MEDIUM	NTV2_AUDIO_BUFFER_BIGGER	NTV2_AUDIO_BUFFER_INVALID
	static ULWord	gBufferSizeToByteCount []	=	{	1 * 1024 * 1024,			4 * 1024 * 1024,		2 * 1024 * 1024,			3 * 1024 * 1024,			0	};
	if (NTV2_IS_VALID_AUDIO_BUFFER_SIZE (inBufferSize))
		return gBufferSizeToByteCount [inBufferSize];
	return 0;
}

typedef	std::set<NTV2FrameRate>					NTV2FrameRates;
typedef NTV2FrameRates::const_iterator			NTV2FrameRatesConstIter;
typedef std::vector<NTV2FrameRates>				NTV2FrameRateFamilies;
typedef NTV2FrameRateFamilies::const_iterator	NTV2FrameRateFamiliesConstIter;

static NTV2FrameRateFamilies	sFRFamilies;
static AJALock					sFRFamMutex;


static bool CheckFrameRateFamiliesInitialized (void)
{
	if (!sFRFamMutex.IsValid())
		return false;

	AJAAutoLock autoLock (&sFRFamMutex);
	if (sFRFamilies.empty())
	{
		NTV2FrameRates	FR1498, FR1500, FR2398, FR2400, FR2500;
		FR1498.insert(NTV2_FRAMERATE_1498); FR1498.insert(NTV2_FRAMERATE_2997); FR1498.insert(NTV2_FRAMERATE_5994); FR1498.insert(NTV2_FRAMERATE_11988);
		sFRFamilies.push_back(FR1498);
		FR1500.insert(NTV2_FRAMERATE_1500); FR1500.insert(NTV2_FRAMERATE_3000); FR1500.insert(NTV2_FRAMERATE_6000); FR1500.insert(NTV2_FRAMERATE_12000);
		sFRFamilies.push_back(FR1500);
		FR2398.insert(NTV2_FRAMERATE_2398); FR2398.insert(NTV2_FRAMERATE_4795);
		sFRFamilies.push_back(FR2398);
		FR2400.insert(NTV2_FRAMERATE_2400); FR2400.insert(NTV2_FRAMERATE_4800);
		sFRFamilies.push_back(FR2400);
		FR2500.insert(NTV2_FRAMERATE_2500); FR2500.insert(NTV2_FRAMERATE_5000);
		sFRFamilies.push_back(FR2500);
	}
	return !sFRFamilies.empty();
}


NTV2FrameRate GetFrameRateFamily (const NTV2FrameRate inFrameRate)
{
	if (CheckFrameRateFamiliesInitialized())
		for (NTV2FrameRateFamiliesConstIter it(sFRFamilies.begin());  it != sFRFamilies.end();  ++it)
		{
			const NTV2FrameRates &	family (*it);
			NTV2FrameRatesConstIter	iter(family.find(inFrameRate));
			if (iter != family.end())
				return *(family.begin());
		}
	return NTV2_FRAMERATE_INVALID;
}


bool IsMultiFormatCompatible (const NTV2FrameRate inFrameRate1, const NTV2FrameRate inFrameRate2)
{
	if (inFrameRate1 == inFrameRate2)
		return true;

	if (!NTV2_IS_SUPPORTED_NTV2FrameRate(inFrameRate1) || !NTV2_IS_SUPPORTED_NTV2FrameRate(inFrameRate2))
		return false;

	const NTV2FrameRate	frFamily1 (GetFrameRateFamily(inFrameRate1));
	const NTV2FrameRate	frFamily2 (GetFrameRateFamily(inFrameRate2));

	if (!NTV2_IS_SUPPORTED_NTV2FrameRate(frFamily1)  ||  !NTV2_IS_SUPPORTED_NTV2FrameRate(frFamily2))
		return false;	//	Probably uninitialized

	return frFamily1 == frFamily2;

}	//	IsMultiFormatCompatible (NTV2FrameRate)


AJAExport bool IsMultiFormatCompatible (const NTV2VideoFormat inFormat1, const NTV2VideoFormat inFormat2)
{
	if (inFormat1 == NTV2_FORMAT_UNKNOWN || inFormat2 == NTV2_FORMAT_UNKNOWN)
		return false;
	return ::IsMultiFormatCompatible (::GetNTV2FrameRateFromVideoFormat (inFormat1), ::GetNTV2FrameRateFromVideoFormat (inFormat2));

}	//	IsMultiFormatCompatible (NTV2VideoFormat)


AJAExport bool IsPSF (const NTV2VideoFormat format)
{
	return NTV2_IS_PSF_VIDEO_FORMAT(format);
}


AJAExport bool IsProgressivePicture (const NTV2VideoFormat format)
{
	return NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(format);
}


AJAExport bool IsProgressiveTransport (const NTV2VideoFormat format)
{
	NTV2Standard standard (::GetNTV2StandardFromVideoFormat(format));
	return IsProgressiveTransport(standard);
}


AJAExport bool IsProgressiveTransport (const NTV2Standard standard)
{
	return NTV2_IS_PROGRESSIVE_STANDARD(standard);
}


AJAExport bool IsRGBFormat (const NTV2FrameBufferFormat format)
{
	return NTV2_IS_FBF_RGB(format);
}


AJAExport bool IsYCbCrFormat (const NTV2FrameBufferFormat format)
{
	return !NTV2_IS_FBF_RGB(format);	// works for now
}


AJAExport bool IsAlphaChannelFormat (const NTV2FrameBufferFormat format)
{
	return NTV2_FBF_HAS_ALPHA(format);
}


AJAExport bool Is2KFormat (const NTV2VideoFormat format)
{
	return NTV2_IS_2K_1080_VIDEO_FORMAT(format)  ||  NTV2_IS_2K_VIDEO_FORMAT(format);
}


AJAExport bool Is4KFormat (const NTV2VideoFormat format)
{
	return NTV2_IS_4K_4096_VIDEO_FORMAT(format)  ||  NTV2_IS_4K_QUADHD_VIDEO_FORMAT(format);
}


AJAExport bool Is8KFormat (const NTV2VideoFormat format)
{
	return NTV2_IS_QUAD_QUAD_FORMAT(format);
}


AJAExport bool IsRaw (const NTV2FrameBufferFormat frameBufferFormat)
{
	return NTV2_FBF_IS_RAW(frameBufferFormat);
}


AJAExport bool Is8BitFrameBufferFormat (const NTV2FrameBufferFormat format)
{
	return NTV2_IS_FBF_8BIT(format);
}


AJAExport bool IsVideoFormatA (const NTV2VideoFormat format)
{
	return NTV2_VIDEO_FORMAT_IS_A(format);
}


AJAExport bool IsVideoFormatB (const NTV2VideoFormat format)
{
	return NTV2_IS_3Gb_FORMAT(format);
}

AJAExport bool IsVideoFormatJ2KSupported (const NTV2VideoFormat format)
{
    return NTV2_VIDEO_FORMAT_IS_J2K_SUPPORTED(format);
}


#if !defined (NTV2_DEPRECATE)
	//
	// BuildRoutingTableForOutput
	// Relative to FrameStore
	// NOTE: convert ignored for now do to excessive laziness
	//
	// Its possible that if you are using this for channel 1 and
	// 2 you could use up resources and unexpected behavior will ensue.
	// for instance, if channel 1 routing uses up both color space converters
	// and then you try to setup channel 2 to use a color space converter.
	// it will overwrite channel 1's routing.
	bool BuildRoutingTableForOutput(CNTV2SignalRouter & outRouter,
									NTV2Channel channel,
									NTV2FrameBufferFormat fbf,
									bool convert, // ignored
									bool lut,
									bool dualLink,
									bool keyOut)	// only check for RGB Formats
									// NOTE: maybe add key out???????
	{
		(void) convert;
		bool status = false;
		bool canHaveKeyOut = false;
		switch (fbf)
		{
		case NTV2_FBF_8BIT_YCBCR_420PL3:
			// not supported
			break;

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
			canHaveKeyOut = true;
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_48BIT_RGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( lut && dualLink )
				{
					// first hook up framestore 1 to lut1,
					outRouter.addWithValue(GetXptLUTInputSelectEntry(),NTV2_XptFrameBuffer1RGB);

					// then hook up lut1 to dualLink
					outRouter.addWithValue(GetDuallinkOutInputSelectEntry(),NTV2_XptLUT1Out);

					// then hook up dualLink to SDI 1 and SDI 2
					outRouter.addWithValue(GetSDIOut1InputSelectEntry(),NTV2_XptDuallinkOut1);
					status = outRouter.addWithValue(GetSDIOut2InputSelectEntry(),NTV2_XptDuallinkOut1);

				}
				else
					if ( lut )
					{
						// first hook up framestore 1 to lut1,
						outRouter.addWithValue(GetXptLUTInputSelectEntry(),NTV2_XptFrameBuffer1RGB);

						// then hook up lut1 to csc1
						outRouter.addWithValue(GetCSC1VidInputSelectEntry(),NTV2_XptLUT1Out);

						// then hook up csc1 to sdi 1
						status = outRouter.addWithValue(GetSDIOut1InputSelectEntry(),NTV2_XptCSC1VidYUV);
						if ( keyOut && canHaveKeyOut)
						{
							status = outRouter.addWithValue(GetSDIOut1InputSelectEntry(),NTV2_XptCSC1KeyYUV);
						}
					}
					else
						if ( dualLink)
						{
							// hook up framestore 1 lut duallink
							outRouter.addWithValue(GetDuallinkOutInputSelectEntry(),NTV2_XptFrameBuffer1RGB);

							// then hook up dualLink to SDI 1 and SDI 2
							outRouter.addWithValue(GetSDIOut1InputSelectEntry(),NTV2_XptDuallinkOut1);
							status = outRouter.addWithValue(GetSDIOut2InputSelectEntry(),NTV2_XptDuallinkOut1);
						}
						else
						{
							outRouter.addWithValue(::GetCSC1VidInputSelectEntry(),NTV2_XptFrameBuffer1RGB);
							status = outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptCSC1VidYUV);
							if ( keyOut && canHaveKeyOut)
							{
								status = outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptCSC1KeyYUV);

							}

						}
			}
			else // if channel 2
			{

				// arbitrarily don't support duallink on channel 2
				if ( lut )
				{
					// first hook up framestore 2 to lut2,
					outRouter.addWithValue(::GetXptLUT2InputSelectEntry(),NTV2_XptFrameBuffer2RGB);

					// then hook up lut2 to csc2
					outRouter.addWithValue(::GetCSC2VidInputSelectEntry(),NTV2_XptLUT2Out);

					// then hook up csc2 to sdi 2 and any other output
					outRouter.addWithValue(::GetSDIOut1InputSelectEntry(), NTV2_XptCSC2VidYUV);
					outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
					outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
					status = outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptCSC2VidYUV);
				}
				else
				{
					outRouter.addWithValue(::GetCSC2VidInputSelectEntry(),NTV2_XptFrameBuffer2RGB);
					outRouter.addWithValue(::GetSDIOut1InputSelectEntry(), NTV2_XptCSC2VidRGB);
					outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptCSC2VidRGB);
					outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptCSC2VidRGB);
					status = outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptCSC2VidRGB);
				}
			}
			break;

		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_HDV:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( lut && dualLink )
				{
					// first uncompress it.
					outRouter.addWithValue(::GetCompressionModInputSelectEntry(),NTV2_XptFrameBuffer1YUV);

					// then send it to color space converter 1
					outRouter.addWithValue(::GetCSC1VidInputSelectEntry(),NTV2_XptCompressionModule);

					// color space convert 1 to lut 1
					outRouter.addWithValue(::GetXptLUTInputSelectEntry(),NTV2_XptCSC1VidRGB);

					// lut 1 to duallink in
					outRouter.addWithValue(::GetDuallinkOutInputSelectEntry(),NTV2_XptLUT1Out);

					// then hook up dualLink to SDI 1 and SDI 2
					outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptDuallinkOut1);
					status = outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptDuallinkOut1);

				}
				else  // if channel 2
					if ( lut )
					{
						// first uncompress it.
						outRouter.addWithValue(::GetCompressionModInputSelectEntry(),NTV2_XptFrameBuffer1YUV);

						// then send it to color space converter 1
						outRouter.addWithValue(::GetCSC1VidInputSelectEntry(),NTV2_XptCompressionModule);

						// color space convert 1 to lut 1
						outRouter.addWithValue(::GetXptLUTInputSelectEntry(),NTV2_XptCSC1VidRGB);

						// lut 1 to color space convert 2
						outRouter.addWithValue(::GetCSC2VidInputSelectEntry(),NTV2_XptLUT1Out);

						// color space converter 2 to outputs
						outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptCSC2VidYUV);
						outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
						outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
						status = outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptCSC2VidYUV);
					}
					else
					{
						// just send it straight out SDI 1 and other outputs
						outRouter.addWithValue(::GetCompressionModInputSelectEntry(),NTV2_XptFrameBuffer1YUV);
						outRouter.addWithValue(::GetSDIOut2InputSelectEntry(), NTV2_XptCompressionModule);
						outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptCompressionModule);
						outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptCompressionModule);
						status = outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptCompressionModule);
					}
			}
			else
			{
				// channel 2....an excersize for the user:)
				outRouter.addWithValue(::GetCompressionModInputSelectEntry(),NTV2_XptFrameBuffer1YUV);
				outRouter.addWithValue(::GetSDIOut2InputSelectEntry(), NTV2_XptFrameBuffer2YUV);
				outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptFrameBuffer2YUV);
				outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptFrameBuffer2YUV);
				status = outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptFrameBuffer2YUV);
			}
			break;

		default:
			// YCbCr Stuff
			if ( channel == NTV2_CHANNEL1)
			{
				if ( lut && dualLink )
				{
					// frame store 1 to color space converter 1
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptFrameBuffer1YUV);

					// color space convert 1 to lut 1
					outRouter.addWithValue (::GetXptLUTInputSelectEntry (), NTV2_XptCSC1VidRGB);

					// lut 1 to duallink in
					outRouter.addWithValue (::GetDuallinkOutInputSelectEntry (), NTV2_XptLUT1Out);

					// then hook up dualLink to SDI 1 and SDI 2
					outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptDuallinkOut1);
					status = outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptDuallinkOut1);

				}
				else
				if ( lut )
				{
					// frame store 1 to color space converter 1
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptFrameBuffer1YUV);

					// color space convert 1 to lut 1
					outRouter.addWithValue (::GetXptLUTInputSelectEntry (), NTV2_XptCSC1VidRGB);

					// lut 1 to color space convert 2
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptLUT1Out);

					// color space converter 2 to outputs
					outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptCSC2VidYUV);
					outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptCSC2VidYUV);
					outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptCSC2VidYUV);
					status = outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptCSC2VidYUV);
				}
				else
				{
					// just send it straight outputs
					outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptFrameBuffer1YUV);
					outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptFrameBuffer1YUV);
					status = outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptFrameBuffer1YUV);
				}
			}

			else if ( channel == NTV2_CHANNEL2)
			{
				// channel 2....an excersize for the user:)
				outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptFrameBuffer2YUV);
				outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptFrameBuffer2YUV);
				status = outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptFrameBuffer2YUV);
			}

			else if ( channel == NTV2_CHANNEL3)
			{
				// channel 3....an excersize for the user:)
				outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptFrameBuffer3YUV);
				outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptFrameBuffer3YUV);
				status = outRouter.addWithValue (::GetSDIOut3InputSelectEntry (), NTV2_XptFrameBuffer3YUV);
			}

			else if ( channel == NTV2_CHANNEL4)
			{
				// channel 4....an excersize for the user:)
				outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptFrameBuffer4YUV);
				outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptFrameBuffer4YUV);
				status = outRouter.addWithValue (::GetSDIOut4InputSelectEntry (), NTV2_XptFrameBuffer4YUV);
			}
			break;
		}


		return status;
	}

	//
	// BuildRoutingTableForInput
	// Relative to FrameStore
	//
	bool BuildRoutingTableForInput(CNTV2SignalRouter & outRouter,
								   NTV2Channel channel,
								   NTV2FrameBufferFormat fbf,
								   bool withKey,  // only supported for NTV2_CHANNEL1 for rgb formats with alpha
								   bool lut,	  // not supported
								   bool dualLink, // assume coming in RGB(only checked for NTV2_CHANNEL1
								   bool EtoE)
	{
		(void) lut;
		bool status = false;
		if (fbf == NTV2_FBF_8BIT_YCBCR_420PL3)
			return status;

		if ( EtoE)
		{
			if ( dualLink )
			{
				outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);
			}
			else if ( channel == NTV2_CHANNEL1)
			{
				outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
			}
			else if ( channel == NTV2_CHANNEL2)
			{
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);
			}
			else if ( channel == NTV2_CHANNEL3)
			{
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn3);
			}
			else if ( channel == NTV2_CHANNEL4)
			{
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn4);
			}
		}

		switch (fbf)
		{

			case NTV2_FBF_ARGB:
			case NTV2_FBF_RGBA:
			case NTV2_FBF_ABGR:
			case NTV2_FBF_10BIT_ARGB:
			case NTV2_FBF_16BIT_ARGB:
				if ( channel == NTV2_CHANNEL1)
				{
					if ( withKey)
					{
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						outRouter.addWithValue (::GetCSC1KeyInputSelectEntry (), NTV2_XptSDIIn2);
						outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 1);
						status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
					}
					else
					{
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 0);
						status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
					}
				}
				else
				{
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
				}
				break;
			case NTV2_FBF_10BIT_RGB:
			case NTV2_FBF_10BIT_RGB_PACKED:
			case NTV2_FBF_10BIT_DPX:
            case NTV2_FBF_10BIT_DPX_LE:
			case NTV2_FBF_24BIT_BGR:
			case NTV2_FBF_24BIT_RGB:
				if ( channel == NTV2_CHANNEL1)
				{
					if ( dualLink )
					{
						status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptDuallinkIn1);
					}
					else
					{
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
					}
				}
				else
				{
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
				}
				break;
			case NTV2_FBF_8BIT_DVCPRO:
			case NTV2_FBF_8BIT_HDV:
				if ( channel == NTV2_CHANNEL1)
				{
					status = outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn1);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCompressionModule);
				}
				else
				{
					status = outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn2);
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCompressionModule);
				}
				break;

			default:
				if( dualLink )
				{
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn1);
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn2);
				}
				else if ( channel == NTV2_CHANNEL1)
				{
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn1);
				}
				else if ( channel == NTV2_CHANNEL2)
				{
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn2);
				}
				else if ( channel == NTV2_CHANNEL3)
				{
					status = outRouter.addWithValue (::GetFrameBuffer3InputSelectEntry (), NTV2_XptSDIIn3);
				}
				else if ( channel == NTV2_CHANNEL4)
				{
					status = outRouter.addWithValue (::GetFrameBuffer4InputSelectEntry (), NTV2_XptSDIIn4);
				}
				break;
		}

		return status;

	}

	//
	// BuildRoutingTableForInput
	// Relative to FrameStore
	//
	bool BuildRoutingTableForInput(CNTV2SignalRouter & outRouter,
								   NTV2Channel channel,
								   NTV2FrameBufferFormat fbf,
								   bool convert,  // Turn on the conversion module
								   bool withKey,  // only supported for NTV2_CHANNEL1 for rgb formats with alpha
								   bool lut,	  // not supported
								   bool dualLink, // assume coming in RGB(only checked for NTV2_CHANNEL1
								   bool EtoE)
	{
		(void) convert;
		(void) lut;
		bool status = false;
		if (fbf == NTV2_FBF_8BIT_YCBCR_420PL3)
			return status;

		if ( EtoE)
		{
			if ( dualLink)
			{
				outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);

			}
			else
				if ( channel == NTV2_CHANNEL1)
				{
					outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
				}
				else
				{
					outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);
				}
		}

		switch (fbf)
		{

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( withKey)
				{
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					outRouter.addWithValue (::GetCSC1KeyInputSelectEntry (), NTV2_XptSDIIn2);
					outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 1);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);

				}
				else
				{
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 0);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
				}
			}
			else
			{
				outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
			}
			break;
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_24BIT_RGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( dualLink )
				{
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptDuallinkIn1);
				}
				else
				{
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
				}
			}
			else
			{
				outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
			}
			break;
		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_HDV:
			if ( channel == NTV2_CHANNEL1)
			{
				status = outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn1);
				status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCompressionModule);
			}
			else
			{
				status = outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn2);
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCompressionModule);
			}
			break;

		default:
			if ( channel == NTV2_CHANNEL1)
			{
				status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn1);
			}
			else
			{
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn2);
			}
			break;
		}

		return status;

	}

	//
	// BuildRoutingTableForInput
	// Includes input and channel now
	//
	bool BuildRoutingTableForInput(CNTV2SignalRouter & outRouter,
								   NTV2InputSource inputSource,
								   NTV2Channel channel,
								   NTV2FrameBufferFormat fbf,
								   bool convert,  // Turn on the conversion module
								   bool withKey,  // only supported for NTV2_CHANNEL1 for rgb formats with alpha
								   bool lut,	  // not supported
								   bool dualLink, // assume coming in RGB(only checked for NTV2_CHANNEL1
								   bool EtoE)
	{
		(void) convert;
		(void) lut;
		bool status = false;
		if (fbf == NTV2_FBF_8BIT_YCBCR_420PL3)
			return status;

		if ( EtoE)
		{
			if ( dualLink)
			{
				outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);

			}
			else
			{
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}
			}
		}

		switch (fbf)
		{

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( withKey)
				{
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					outRouter.addWithValue (::GetCSC1KeyInputSelectEntry (), NTV2_XptSDIIn2);
					outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 1);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);

				}
				else
				{
					//outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					switch ( inputSource )
					{
					case NTV2_INPUTSOURCE_SDI1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						break;
					case NTV2_INPUTSOURCE_SDI2:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn2);
						break;
					case NTV2_INPUTSOURCE_ANALOG1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptAnalogIn);
						break;
					case NTV2_INPUTSOURCE_HDMI1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptHDMIIn1);
						break;
					default:
						return false;
					}

					outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 0);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
				}
			}
			else
			{
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}

				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
			}
			break;
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_24BIT_RGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( dualLink )
				{
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (),NTV2_XptDuallinkIn1);
				}
				else
				{
					switch ( inputSource )
					{
					case NTV2_INPUTSOURCE_SDI1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						break;
					case NTV2_INPUTSOURCE_SDI2:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn2);
						break;
					case NTV2_INPUTSOURCE_ANALOG1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptAnalogIn);
						break;
					case NTV2_INPUTSOURCE_HDMI1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptHDMIIn1);
						break;
					default:
						return false;
					}
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
				}
			}
			else
			{
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
			}
			break;
		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_HDV:
			switch ( inputSource )
			{
			case NTV2_INPUTSOURCE_SDI1:
				outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn1);
				break;
			case NTV2_INPUTSOURCE_SDI2:
				outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn2);
				break;
			case NTV2_INPUTSOURCE_ANALOG1:
				outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptAnalogIn);
				break;
			case NTV2_INPUTSOURCE_HDMI1:
				outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptHDMIIn1);
				break;
			default:
				return false;
			}
			if ( channel == NTV2_CHANNEL1)
			{
				status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCompressionModule);
			}
			else
			{
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCompressionModule);
			}
			break;

		default:
			if ( channel == NTV2_CHANNEL1)
			{
				//status = outRouter.addWithValue(FrameBuffer1InputSelectEntry,NTV2_XptSDIIn1);
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}
			}
			else
			{
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}
			}
			break;
		}

		return status;
	 }


	// The 10-bit value converted from hex to decimal represents degrees Kelvin.
	// However, the system generates a value that is 5 deg C high. The decimal value
	// minus 273 deg minus 5 deg should be the degrees Centigrade. This is only accurate
	// to 5 deg C, supposedly.

	// These functions used to be a lot more complicated.  Hardware changes
	// reduced them to simple offset & scaling. - STC
	//
	ULWord ConvertFusionAnalogToTempCentigrade(ULWord adc10BitValue)
	{
		// Convert kelvin to centigrade and subtract 5 degrees hot part reports
		// and add empirical 8 degree fudge factor.
		return adc10BitValue -286;

	}

	ULWord ConvertFusionAnalogToMilliVolts(ULWord adc10BitValue, ULWord millivoltsResolution)
	{
		// Different rails have different mv/unit scales.
		return adc10BitValue * millivoltsResolution;
	}
#endif	//	!defined (NTV2_DEPRECATE)


NTV2ConversionMode GetConversionMode (const NTV2VideoFormat inFormat, const NTV2VideoFormat outFormat)
{
	NTV2ConversionMode cMode = NTV2_CONVERSIONMODE_UNKNOWN;

	switch( inFormat )
	{
	case NTV2_FORMAT_720p_5994:
		if ( outFormat == NTV2_FORMAT_525_5994 )
			cMode = NTV2_720p_5994to525_5994;
		else if ( outFormat == NTV2_FORMAT_1080i_5994)
			cMode = NTV2_720p_5994to1080i_5994;
		else if ( outFormat == NTV2_FORMAT_1080psf_2997_2)
			cMode = NTV2_720p_5994to1080i_5994;
		break;

	case NTV2_FORMAT_720p_5000:
		if ( outFormat == NTV2_FORMAT_625_5000 )
			cMode = NTV2_720p_5000to625_2500;
		else if ( outFormat == NTV2_FORMAT_1080i_5000)	//	NTV2_FORMAT_1080psf_2500
			cMode = NTV2_720p_5000to1080i_2500;
		else if ( outFormat == NTV2_FORMAT_1080psf_2500_2)
			cMode = NTV2_720p_5000to1080i_2500;
		break;

	case NTV2_FORMAT_525_2398:
		if ( outFormat == NTV2_FORMAT_1080psf_2398 )
			cMode = NTV2_525_2398to1080i_2398;
		break;

	case NTV2_FORMAT_525_5994:
		if ( outFormat == NTV2_FORMAT_1080i_5994 )
			cMode = NTV2_525_5994to1080i_5994;
		else if (outFormat == NTV2_FORMAT_1080psf_2997_2)
			cMode = NTV2_525_5994to1080i_5994;
		else if ( outFormat == NTV2_FORMAT_720p_5994 )
			cMode = NTV2_525_5994to720p_5994;
		else if ( outFormat == NTV2_FORMAT_525_5994 )
			cMode = NTV2_525_5994to525_5994;
		else if ( outFormat == NTV2_FORMAT_525psf_2997 )
			cMode = NTV2_525_5994to525psf_2997;
		break;

	case NTV2_FORMAT_625_5000:
		if ( outFormat == NTV2_FORMAT_1080i_5000)	//	NTV2_FORMAT_1080psf_2500
			cMode = NTV2_625_2500to1080i_2500;
		else if ( outFormat == NTV2_FORMAT_1080psf_2500_2)
			cMode = NTV2_625_2500to1080i_2500;
		else if ( outFormat == NTV2_FORMAT_720p_5000 )
			cMode = NTV2_625_2500to720p_5000;
		else if ( outFormat == NTV2_FORMAT_625_5000 )
			cMode = NTV2_625_2500to625_2500;
		else if ( outFormat == NTV2_FORMAT_625psf_2500 )
			cMode = NTV2_625_5000to625psf_2500;
		break;

	case NTV2_FORMAT_720p_6000:
		if ( outFormat == NTV2_FORMAT_1080i_6000)	//	NTV2_FORMAT_1080psf_3000
			cMode = NTV2_720p_6000to1080i_3000;
		else if (outFormat == NTV2_FORMAT_1080psf_3000_2 )
			cMode = NTV2_720p_6000to1080i_3000;
		break;

	case NTV2_FORMAT_1080psf_2398:
		if ( outFormat == NTV2_FORMAT_525_2398 )
			cMode = NTV2_1080i2398to525_2398;
		else if ( outFormat == NTV2_FORMAT_525_5994 )
			cMode = NTV2_1080i2398to525_2997;
		else if ( outFormat == NTV2_FORMAT_720p_2398 )
			cMode = NTV2_1080i_2398to720p_2398;
		else if ( outFormat == NTV2_FORMAT_1080i_5994 )
			cMode = NTV2_1080psf_2398to1080i_5994;
		break;

	case NTV2_FORMAT_1080psf_2400:
		if ( outFormat == NTV2_FORMAT_1080i_6000 )
			cMode = NTV2_1080psf_2400to1080i_3000;
		break;
		
	case NTV2_FORMAT_1080psf_2500_2:
		if ( outFormat == NTV2_FORMAT_625_5000 )
			cMode = NTV2_1080i_2500to625_2500;
		else if ( outFormat == NTV2_FORMAT_720p_5000 )
			cMode = NTV2_1080i_2500to720p_5000;
		else if ( outFormat == NTV2_FORMAT_1080psf_2500_2 )
			cMode = NTV2_1080i_5000to1080psf_2500;
		else if ( outFormat == NTV2_FORMAT_1080psf_2500_2 )
			cMode = NTV2_1080psf_2500to1080i_2500;
		break;
	
	case NTV2_FORMAT_1080p_2398:
		if ( outFormat == NTV2_FORMAT_1080i_5994 )
			cMode = NTV2_1080p_2398to1080i_5994;
		break;
	
	case NTV2_FORMAT_1080p_2400:
		if ( outFormat == NTV2_FORMAT_1080i_6000 )
			cMode = NTV2_1080p_2400to1080i_3000;
		break;
		
	case NTV2_FORMAT_1080p_2500:
		if ( outFormat == NTV2_FORMAT_1080i_5000 )
			cMode = NTV2_1080p_2500to1080i_2500;
		break;

	case NTV2_FORMAT_1080i_5000:
		if ( outFormat == NTV2_FORMAT_625_5000 )
			cMode = NTV2_1080i_2500to625_2500;
		else if ( outFormat == NTV2_FORMAT_720p_5000 )
			cMode = NTV2_1080i_2500to720p_5000;
		else if ( outFormat == NTV2_FORMAT_1080psf_2500_2 )
			cMode = NTV2_1080i_5000to1080psf_2500;
		break;

	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080i_5994:
		if ( outFormat == NTV2_FORMAT_525_5994 )
			cMode = NTV2_1080i_5994to525_5994;
		else if ( outFormat == NTV2_FORMAT_720p_5994 )
			cMode = NTV2_1080i_5994to720p_5994;
		else if ( outFormat == NTV2_FORMAT_1080psf_2997_2 )
			cMode = NTV2_1080i_5994to1080psf_2997;
		break;

	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080i_6000:
		if ( outFormat == NTV2_FORMAT_720p_6000 )
			cMode = NTV2_1080i_3000to720p_6000;
		else if ( outFormat == NTV2_FORMAT_1080psf_3000_2 )
			cMode = NTV2_1080i_6000to1080psf_3000;
		break;

	case NTV2_FORMAT_720p_2398:
		if ( outFormat == NTV2_FORMAT_1080psf_2398 )
			cMode = NTV2_720p_2398to1080i_2398;
		break;

	case NTV2_FORMAT_1080p_3000:
		if ( outFormat == NTV2_FORMAT_720p_6000 )
			cMode = NTV2_1080p_3000to720p_6000;
		break;

	default:
		break;
	}

	return cMode;
}

NTV2VideoFormat GetInputForConversionMode (const NTV2ConversionMode conversionMode)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;

	switch( conversionMode )
	{
	case NTV2_525_5994to525_5994: inputFormat = NTV2_FORMAT_525_5994; break;
	case NTV2_525_5994to720p_5994: inputFormat = NTV2_FORMAT_525_5994; break;
	case NTV2_525_5994to1080i_5994: inputFormat = NTV2_FORMAT_525_5994; break;
	case NTV2_525_2398to1080i_2398: inputFormat = NTV2_FORMAT_525_2398; break;
	case NTV2_525_5994to525psf_2997: inputFormat = NTV2_FORMAT_525_5994; break;

	case NTV2_625_2500to625_2500: inputFormat = NTV2_FORMAT_625_5000; break;
	case NTV2_625_2500to720p_5000: inputFormat = NTV2_FORMAT_625_5000; break;
	case NTV2_625_2500to1080i_2500: inputFormat = NTV2_FORMAT_625_5000; break;
	case NTV2_625_5000to625psf_2500: inputFormat = NTV2_FORMAT_625_5000; break;

	case NTV2_720p_5000to625_2500: inputFormat = NTV2_FORMAT_720p_5000; break;
	case NTV2_720p_5000to1080i_2500: inputFormat = NTV2_FORMAT_720p_5000; break;
	case NTV2_720p_5994to525_5994: inputFormat = NTV2_FORMAT_720p_5994; break;
	case NTV2_720p_5994to1080i_5994: inputFormat = NTV2_FORMAT_720p_5994; break;
	case NTV2_720p_6000to1080i_3000: inputFormat = NTV2_FORMAT_720p_6000; break;
	case NTV2_720p_2398to1080i_2398: inputFormat = NTV2_FORMAT_720p_2398; break;

	case NTV2_1080i2398to525_2398: inputFormat = NTV2_FORMAT_1080psf_2398; break;
	case NTV2_1080i2398to525_2997: inputFormat = NTV2_FORMAT_1080psf_2398; break;
	case NTV2_1080i_2398to720p_2398: inputFormat = NTV2_FORMAT_1080psf_2398; break;

	case NTV2_1080i_2500to625_2500: inputFormat = NTV2_FORMAT_1080i_5000; break;
	case NTV2_1080i_2500to720p_5000: inputFormat = NTV2_FORMAT_1080i_5000; break;
	case NTV2_1080i_5994to525_5994: inputFormat = NTV2_FORMAT_1080i_5994; break;
	case NTV2_1080i_5994to720p_5994: inputFormat = NTV2_FORMAT_1080i_5994; break;
	case NTV2_1080i_3000to720p_6000: inputFormat = NTV2_FORMAT_1080i_6000; break;
	case NTV2_1080i_5000to1080psf_2500: inputFormat = NTV2_FORMAT_1080i_5000; break;
	case NTV2_1080i_5994to1080psf_2997: inputFormat = NTV2_FORMAT_1080i_5994; break;
	case NTV2_1080i_6000to1080psf_3000: inputFormat = NTV2_FORMAT_1080i_6000; break;
	case NTV2_1080p_3000to720p_6000: inputFormat = NTV2_FORMAT_1080p_3000; break;

	default: inputFormat = NTV2_FORMAT_UNKNOWN; break;
	}
	return inputFormat;
}


NTV2VideoFormat GetOutputForConversionMode (const NTV2ConversionMode conversionMode)
{
	NTV2VideoFormat outputFormat = NTV2_FORMAT_UNKNOWN;

	switch( conversionMode )
	{
	case NTV2_525_5994to525_5994: outputFormat = NTV2_FORMAT_525_5994; break;
	case NTV2_525_5994to720p_5994: outputFormat = NTV2_FORMAT_720p_5994; break;
	case NTV2_525_5994to1080i_5994: outputFormat = NTV2_FORMAT_1080i_5994; break;
	case NTV2_525_2398to1080i_2398: outputFormat = NTV2_FORMAT_1080psf_2398; break;
	case NTV2_525_5994to525psf_2997: outputFormat = NTV2_FORMAT_525psf_2997; break;

	case NTV2_625_2500to625_2500: outputFormat = NTV2_FORMAT_625_5000; break;
	case NTV2_625_2500to720p_5000: outputFormat = NTV2_FORMAT_720p_5000; break;
	case NTV2_625_2500to1080i_2500: outputFormat = NTV2_FORMAT_1080i_5000; break;
	case NTV2_625_5000to625psf_2500: outputFormat = NTV2_FORMAT_625psf_2500; break;

	case NTV2_720p_5000to625_2500: outputFormat = NTV2_FORMAT_625_5000; break;
	case NTV2_720p_5000to1080i_2500: outputFormat = NTV2_FORMAT_1080i_5000; break;
	case NTV2_720p_5994to525_5994: outputFormat = NTV2_FORMAT_525_5994; break;
	case NTV2_720p_5994to1080i_5994: outputFormat = NTV2_FORMAT_1080i_5994; break;
	case NTV2_720p_6000to1080i_3000: outputFormat = NTV2_FORMAT_1080i_6000; break;
	case NTV2_720p_2398to1080i_2398: outputFormat = NTV2_FORMAT_1080psf_2398; break;

	case NTV2_1080i2398to525_2398: outputFormat = NTV2_FORMAT_525_2398; break;
	case NTV2_1080i2398to525_2997: outputFormat = NTV2_FORMAT_525_5994; break;
	case NTV2_1080i_2398to720p_2398: outputFormat = NTV2_FORMAT_720p_2398; break;
	//case NTV2_1080i2400to525_2400: outputFormat = NTV2_FORMAT_525_2400; break;

	//case NTV2_1080p2398to525_2398: outputFormat = NTV2_FORMAT_525_2398; break;
	//case NTV2_1080p2398to525_2997: outputFormat = NTV2_FORMAT_525_5994; break;
	//case NTV2_1080p2400to525_2400: outputFormat = NTV2_FORMAT_525_2400; break;

	case NTV2_1080i_2500to625_2500: outputFormat = NTV2_FORMAT_625_5000; break;
	case NTV2_1080i_2500to720p_5000: outputFormat = NTV2_FORMAT_720p_5000; break;
	case NTV2_1080i_5994to525_5994: outputFormat = NTV2_FORMAT_525_5994; break;
	case NTV2_1080i_5994to720p_5994: outputFormat = NTV2_FORMAT_720p_5994; break;
	case NTV2_1080i_3000to720p_6000: outputFormat = NTV2_FORMAT_720p_6000; break;
	case NTV2_1080i_5000to1080psf_2500: outputFormat = NTV2_FORMAT_1080psf_2500_2; break;
	case NTV2_1080i_5994to1080psf_2997: outputFormat = NTV2_FORMAT_1080psf_2997_2; break;
	case NTV2_1080i_6000to1080psf_3000: outputFormat = NTV2_FORMAT_1080psf_3000_2; break;
	case NTV2_1080p_3000to720p_6000: outputFormat = NTV2_FORMAT_720p_6000; break;
	default: outputFormat = NTV2_FORMAT_UNKNOWN; break;
	}
	return outputFormat;
}


ostream & operator << (ostream & inOutStream, const NTV2FrameDimensions inFrameDimensions)
{
	return inOutStream	<< inFrameDimensions.Width() << "Wx" << inFrameDimensions.Height() << "H";
}


ostream & operator << (ostream & inOutStream, const NTV2SmpteLineNumber & inSmpteLineNumber)
{
	return inSmpteLineNumber.Print (inOutStream);
}


string NTV2ChannelToString (const NTV2Channel inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Ch1", NTV2_CHANNEL1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Ch2", NTV2_CHANNEL2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Ch3", NTV2_CHANNEL3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Ch4", NTV2_CHANNEL4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Ch5", NTV2_CHANNEL5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Ch6", NTV2_CHANNEL6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Ch7", NTV2_CHANNEL7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Ch8", NTV2_CHANNEL8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "n/a", NTV2_CHANNEL_INVALID);
	}
	return "";
}


string NTV2AudioSystemToString (const NTV2AudioSystem inValue, const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_AUDIO_SYSTEM (inValue))
		oss << (inCompactDisplay ? "AudSys" : "NTV2_AUDIOSYSTEM_") << (inValue + 1);
	else
		oss << (inCompactDisplay ? "NoAudio" : "NTV2_AUDIOSYSTEM_INVALID");
	return oss.str ();
}


string NTV2AudioRateToString (const NTV2AudioRate inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "48 kHz",	NTV2_AUDIO_48K);
        NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "96 kHz",	NTV2_AUDIO_96K);
        NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "192 kHz",	NTV2_AUDIO_192K);
        NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "???",		NTV2_AUDIO_RATE_INVALID);
	}
	return "";
}


string NTV2AudioBufferSizeToString (const NTV2AudioBufferSize inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "1MB", NTV2_AUDIO_BUFFER_STANDARD);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4MB", NTV2_AUDIO_BUFFER_BIG);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "???", NTV2_MAX_NUM_AudioBufferSizes);
#if !defined (NTV2_DEPRECATE)
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2MB", NTV2_AUDIO_BUFFER_MEDIUM);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "8MB", NTV2_AUDIO_BUFFER_BIGGER);
#endif	//	!defined (NTV2_DEPRECATE)
	}
	return "";
}


string NTV2AudioLoopBackToString (const NTV2AudioLoopBack inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Off", NTV2_AUDIO_LOOPBACK_OFF);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "On", NTV2_AUDIO_LOOPBACK_ON);
		case NTV2_AUDIO_LOOPBACK_INVALID:	break; //special case
	}
	return "???";
}


string NTV2EmbeddedAudioClockToString (const NTV2EmbeddedAudioClock	inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "from device reference", NTV2_EMBEDDED_AUDIO_CLOCK_REFERENCE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "from video input", NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "???", NTV2_EMBEDDED_AUDIO_CLOCK_INVALID);
	}
	return "???";
}


string NTV2CrosspointToString (const NTV2Crosspoint inChannel)
{
	std::ostringstream	oss;
	oss	<< (::IsNTV2CrosspointInput(inChannel) ? "Capture " : "Playout ")
		<< (::IsNTV2CrosspointInput(inChannel) ? ::GetIndexForNTV2CrosspointInput(inChannel) : ::GetIndexForNTV2CrosspointChannel(inChannel)) + 1;
	return oss.str ();
}


string NTV2InputCrosspointIDToString (const NTV2InputCrosspointID inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 1", NTV2_XptFrameBuffer1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 1 B", NTV2_XptFrameBuffer1BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 2", NTV2_XptFrameBuffer2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 2 B", NTV2_XptFrameBuffer2BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 3", NTV2_XptFrameBuffer3Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 3 B", NTV2_XptFrameBuffer3BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 4", NTV2_XptFrameBuffer4Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 4 B", NTV2_XptFrameBuffer4BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 5", NTV2_XptFrameBuffer5Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 5 B", NTV2_XptFrameBuffer5BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 6", NTV2_XptFrameBuffer6Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 6 B", NTV2_XptFrameBuffer6BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 7", NTV2_XptFrameBuffer7Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 7 B", NTV2_XptFrameBuffer7BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 8", NTV2_XptFrameBuffer8Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 8 B", NTV2_XptFrameBuffer8BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 1 Vid", NTV2_XptCSC1VidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 1 Key", NTV2_XptCSC1KeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 2 Vid", NTV2_XptCSC2VidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 2 Key", NTV2_XptCSC2KeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 3 Vid", NTV2_XptCSC3VidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 3 Key", NTV2_XptCSC3KeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 4 Vid", NTV2_XptCSC4VidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 4 Key", NTV2_XptCSC4KeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 5 Vid", NTV2_XptCSC5VidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 5 Key", NTV2_XptCSC5KeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 6 Vid", NTV2_XptCSC6VidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 6 Key", NTV2_XptCSC6KeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 7 Vid", NTV2_XptCSC7VidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 7 Key", NTV2_XptCSC7KeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 8 Vid", NTV2_XptCSC8VidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 8 Key", NTV2_XptCSC8KeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 1", NTV2_XptLUT1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 2", NTV2_XptLUT2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 3", NTV2_XptLUT3Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 4", NTV2_XptLUT4Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 5", NTV2_XptLUT5Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 6", NTV2_XptLUT6Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 7", NTV2_XptLUT7Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 8", NTV2_XptLUT8Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ML Out 1", NTV2_XptMultiLinkOut1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ML Out 1 DS2", NTV2_XptMultiLinkOut1InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ML Out 2", NTV2_XptMultiLinkOut2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ML Out 2 DS2", NTV2_XptMultiLinkOut2InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 1", NTV2_XptSDIOut1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 1 DS2", NTV2_XptSDIOut1InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 2", NTV2_XptSDIOut2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 2 DS2", NTV2_XptSDIOut2InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 3", NTV2_XptSDIOut3Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 3 DS2", NTV2_XptSDIOut3InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 4", NTV2_XptSDIOut4Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 4 DS2", NTV2_XptSDIOut4InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 5", NTV2_XptSDIOut5Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 5 DS2", NTV2_XptSDIOut5InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 6", NTV2_XptSDIOut6Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 6 DS2", NTV2_XptSDIOut6InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 7", NTV2_XptSDIOut7Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 7 DS2", NTV2_XptSDIOut7InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 8", NTV2_XptSDIOut8Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI Out 8 DS2", NTV2_XptSDIOut8InputDS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 1", NTV2_XptDualLinkIn1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 1 DS", NTV2_XptDualLinkIn1DSInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 2", NTV2_XptDualLinkIn2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 2 DS", NTV2_XptDualLinkIn2DSInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 3", NTV2_XptDualLinkIn3Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 3 DS", NTV2_XptDualLinkIn3DSInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 4", NTV2_XptDualLinkIn4Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 4 DS", NTV2_XptDualLinkIn4DSInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 5", NTV2_XptDualLinkIn5Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 5 DS", NTV2_XptDualLinkIn5DSInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 6", NTV2_XptDualLinkIn6Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 6 DS", NTV2_XptDualLinkIn6DSInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 7", NTV2_XptDualLinkIn7Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 7 DS", NTV2_XptDualLinkIn7DSInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 8", NTV2_XptDualLinkIn8Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 8 DS", NTV2_XptDualLinkIn8DSInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 1", NTV2_XptDualLinkOut1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 2", NTV2_XptDualLinkOut2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 3", NTV2_XptDualLinkOut3Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 4", NTV2_XptDualLinkOut4Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 5", NTV2_XptDualLinkOut5Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 6", NTV2_XptDualLinkOut6Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 7", NTV2_XptDualLinkOut7Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 8", NTV2_XptDualLinkOut8Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 1 BG Key", NTV2_XptMixer1BGKeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 1 BG Vid", NTV2_XptMixer1BGVidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 1 FG Key", NTV2_XptMixer1FGKeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 1 FG Vid", NTV2_XptMixer1FGVidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 2 BG Key", NTV2_XptMixer2BGKeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 2 BG Vid", NTV2_XptMixer2BGVidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 2 FG Key", NTV2_XptMixer2FGKeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 2 FG Vid", NTV2_XptMixer2FGVidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 3 BG Key", NTV2_XptMixer3BGKeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 3 BG Vid", NTV2_XptMixer3BGVidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 3 FG Key", NTV2_XptMixer3FGKeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 3 FG Vid", NTV2_XptMixer3FGVidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 4 BG Key", NTV2_XptMixer4BGKeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 4 BG Vid", NTV2_XptMixer4BGVidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 4 FG Key", NTV2_XptMixer4FGKeyInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 4 FG Vid", NTV2_XptMixer4FGVidInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI Out", NTV2_XptHDMIOutInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI Out Q2", NTV2_XptHDMIOutQ2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI Out Q3", NTV2_XptHDMIOutQ3Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI Out Q4", NTV2_XptHDMIOutQ4Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4K DownConv Q1", NTV2_Xpt4KDCQ1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4K DownConv Q2", NTV2_Xpt4KDCQ2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4K DownConv Q3", NTV2_Xpt4KDCQ3Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4K DownConv Q4", NTV2_Xpt4KDCQ4Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 1A", NTV2_Xpt425Mux1AInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 1B", NTV2_Xpt425Mux1BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 2A", NTV2_Xpt425Mux2AInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 2B", NTV2_Xpt425Mux2BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 3A", NTV2_Xpt425Mux3AInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 3B", NTV2_Xpt425Mux3BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 4A", NTV2_Xpt425Mux4AInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 4B", NTV2_Xpt425Mux4BInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Analog Out", NTV2_XptAnalogOutInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Analog Composite Out", NTV2_XptAnalogOutCompositeOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Stereo Left", NTV2_XptStereoLeftInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Stereo Right", NTV2_XptStereoRightInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Pro Amp", NTV2_XptProAmpInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "IICT1", NTV2_XptIICT1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Water Marker 1", NTV2_XptWaterMarker1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Water Marker 2", NTV2_XptWaterMarker2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Update Register", NTV2_XptUpdateRegister);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Compression Module", NTV2_XptCompressionModInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Conversion Module", NTV2_XptConversionModInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 1 Key From In 2", NTV2_XptCSC1KeyFromInput2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FrameSync2", NTV2_XptFrameSync2Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FrameSync1", NTV2_XptFrameSync1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "3D LUT 1", NTV2_Xpt3DLUT1Input);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "OE", NTV2_XptOEInput);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "???", NTV2_INPUT_CROSSPOINT_INVALID);
	}
	return "";

}	//	NTV2InputCrosspointIDToString


string NTV2OutputCrosspointIDToString	(const NTV2OutputCrosspointID inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Black", NTV2_XptBlack);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 1", NTV2_XptSDIIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 1 DS2", NTV2_XptSDIIn1DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 2", NTV2_XptSDIIn2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 2 DS2", NTV2_XptSDIIn2DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 1 YUV", NTV2_XptLUT1YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 1 Vid YUV", NTV2_XptCSC1VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Conversion Module", NTV2_XptConversionModule);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Compression Module", NTV2_XptCompressionModule);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 1 YUV", NTV2_XptFrameBuffer1YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FrameSync 1 YUV", NTV2_XptFrameSync1YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FrameSync 2 YUV", NTV2_XptFrameSync2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 1", NTV2_XptDuallinkOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 1 DS2", NTV2_XptDuallinkOut1DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 2", NTV2_XptDuallinkOut2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 2 DS2", NTV2_XptDuallinkOut2DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 3", NTV2_XptDuallinkOut3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 3 DS2", NTV2_XptDuallinkOut3DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 4", NTV2_XptDuallinkOut4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 4 DS2", NTV2_XptDuallinkOut4DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Alpha Out", NTV2_XptAlphaOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Analog In", NTV2_XptAnalogIn);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1", NTV2_XptHDMIIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1 Q2", NTV2_XptHDMIIn1Q2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1 Q3", NTV2_XptHDMIIn1Q3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1 Q4", NTV2_XptHDMIIn1Q4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1 RGB", NTV2_XptHDMIIn1RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1 Q2 RGB", NTV2_XptHDMIIn1Q2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1 Q3 RGB", NTV2_XptHDMIIn1Q3RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1 Q4 RGB", NTV2_XptHDMIIn1Q4RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2", NTV2_XptHDMIIn2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2 Q2", NTV2_XptHDMIIn2Q2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2 Q3", NTV2_XptHDMIIn2Q3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2 Q4", NTV2_XptHDMIIn2Q4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2 RGB", NTV2_XptHDMIIn2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2 Q2 RGB", NTV2_XptHDMIIn2Q2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2 Q3 RGB", NTV2_XptHDMIIn2Q3RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2 Q4 RGB", NTV2_XptHDMIIn2Q4RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 3", NTV2_XptHDMIIn3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 3 RGB", NTV2_XptHDMIIn3RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 4", NTV2_XptHDMIIn4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 4 RGB", NTV2_XptHDMIIn4RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 1", NTV2_XptDuallinkIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 2", NTV2_XptDuallinkIn2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 3", NTV2_XptDuallinkIn3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 4", NTV2_XptDuallinkIn4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 1", NTV2_XptLUT1Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 1 Vid RGB", NTV2_XptCSC1VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 1 RGB", NTV2_XptFrameBuffer1RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FrameSync 1 RGB", NTV2_XptFrameSync1RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FrameSync 2 RGB", NTV2_XptFrameSync2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 2", NTV2_XptLUT2Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 1 Key YUV", NTV2_XptCSC1KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 2 YUV", NTV2_XptFrameBuffer2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 2 RGB", NTV2_XptFrameBuffer2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 2 Vid YUV", NTV2_XptCSC2VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 2 Vid RGB", NTV2_XptCSC2VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 2 Key YUV", NTV2_XptCSC2KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 1 Vid YUV", NTV2_XptMixer1VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 1 Key YUV", NTV2_XptMixer1KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 1 Vid RGB", NTV2_XptMixer1VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "IICT RGB", NTV2_XptIICTRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "IICT 2 RGB", NTV2_XptIICT2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Test Pattern YUV", NTV2_XptTestPatternYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 2 Vid YUV", NTV2_XptMixer2VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 2 Key YUV", NTV2_XptMixer2KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 2 Vid RGB", NTV2_XptMixer2VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Stereo Compressor Out", NTV2_XptStereoCompressorOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 3", NTV2_XptLUT3Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 4", NTV2_XptLUT4Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 3 YUV", NTV2_XptFrameBuffer3YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 3 RGB", NTV2_XptFrameBuffer3RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 4 YUV", NTV2_XptFrameBuffer4YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 4 RGB", NTV2_XptFrameBuffer4RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 3", NTV2_XptSDIIn3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 3 DS2", NTV2_XptSDIIn3DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 4", NTV2_XptSDIIn4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 4 DS2", NTV2_XptSDIIn4DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 3 Vid YUV", NTV2_XptCSC3VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 3 Vid RGB", NTV2_XptCSC3VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 3 Key YUV", NTV2_XptCSC3KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 4 Vid YUV", NTV2_XptCSC4VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 4 Vid RGB", NTV2_XptCSC4VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 4 Key YUV", NTV2_XptCSC4KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 5 Vid YUV", NTV2_XptCSC5VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 5 Vid RGB", NTV2_XptCSC5VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 5 Key YUV", NTV2_XptCSC5KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 5", NTV2_XptLUT5Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 5", NTV2_XptDuallinkOut5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 5 DS2", NTV2_XptDuallinkOut5DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4K DownConv Out", NTV2_Xpt4KDownConverterOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4K DownConv Out RGB", NTV2_Xpt4KDownConverterOutRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 5 YUV", NTV2_XptFrameBuffer5YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 5 RGB", NTV2_XptFrameBuffer5RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 6 YUV", NTV2_XptFrameBuffer6YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 6 RGB", NTV2_XptFrameBuffer6RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 7 YUV", NTV2_XptFrameBuffer7YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 7 RGB", NTV2_XptFrameBuffer7RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 8 YUV", NTV2_XptFrameBuffer8YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 8 RGB", NTV2_XptFrameBuffer8RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 5", NTV2_XptSDIIn5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 5 DS2", NTV2_XptSDIIn5DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 6", NTV2_XptSDIIn6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 6 DS2", NTV2_XptSDIIn6DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 7", NTV2_XptSDIIn7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 7 DS2", NTV2_XptSDIIn7DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 8", NTV2_XptSDIIn8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 8 DS2", NTV2_XptSDIIn8DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 6 Vid YUV", NTV2_XptCSC6VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 6 Vid RGB", NTV2_XptCSC6VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 6 Key YUV", NTV2_XptCSC6KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 7 Vid YUV", NTV2_XptCSC7VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 7 Vid RGB", NTV2_XptCSC7VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 7 Key YUV", NTV2_XptCSC7KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 8 Vid YUV", NTV2_XptCSC8VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 8 Vid RGB", NTV2_XptCSC8VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "CSC 8 Key YUV", NTV2_XptCSC8KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 6", NTV2_XptLUT6Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 7", NTV2_XptLUT7Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "LUT 8", NTV2_XptLUT8Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 6", NTV2_XptDuallinkOut6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 6 DS2", NTV2_XptDuallinkOut6DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 7", NTV2_XptDuallinkOut7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 7 DS2", NTV2_XptDuallinkOut7DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 8", NTV2_XptDuallinkOut8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL Out 8 DS2", NTV2_XptDuallinkOut8DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 3 Vid YUV", NTV2_XptMixer3VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 3 Key YUV", NTV2_XptMixer3KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 3 Vid RGB", NTV2_XptMixer3VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 4 Vid YUV", NTV2_XptMixer4VidYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 4 Key YUV", NTV2_XptMixer4KeyYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Mixer 4 Vid RGB", NTV2_XptMixer4VidRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 5", NTV2_XptDuallinkIn5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 6", NTV2_XptDuallinkIn6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 7", NTV2_XptDuallinkIn7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 8", NTV2_XptDuallinkIn8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 1 DS2", NTV2_XptDuallinkIn1DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 2 DS2", NTV2_XptDuallinkIn2DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 3 DS2", NTV2_XptDuallinkIn3DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 4 DS2", NTV2_XptDuallinkIn4DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 5 DS2", NTV2_XptDuallinkIn5DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 6 DS2", NTV2_XptDuallinkIn6DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 7 DS2", NTV2_XptDuallinkIn7DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DL In 8 DS2", NTV2_XptDuallinkIn8DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 1a YUV", NTV2_Xpt425Mux1AYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 1a RGB", NTV2_Xpt425Mux1ARGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 1b YUV", NTV2_Xpt425Mux1BYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 1b RGB", NTV2_Xpt425Mux1BRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 2a YUV", NTV2_Xpt425Mux2AYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 2a RGB", NTV2_Xpt425Mux2ARGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 2b YUV", NTV2_Xpt425Mux2BYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 2b RGB", NTV2_Xpt425Mux2BRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 3a YUV", NTV2_Xpt425Mux3AYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 3a RGB", NTV2_Xpt425Mux3ARGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 3b YUV", NTV2_Xpt425Mux3BYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 3b RGB", NTV2_Xpt425Mux3BRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 4a YUV", NTV2_Xpt425Mux4AYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 4a RGB", NTV2_Xpt425Mux4ARGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 4b YUV", NTV2_Xpt425Mux4BYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "425Mux 4b RGB", NTV2_Xpt425Mux4BRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 1 DS2 YUV", NTV2_XptFrameBuffer1_DS2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 1 DS2 RGB", NTV2_XptFrameBuffer1_DS2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 2 DS2 YUV", NTV2_XptFrameBuffer2_DS2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 2 DS2 RGB", NTV2_XptFrameBuffer2_DS2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 3 DS2 YUV", NTV2_XptFrameBuffer3_DS2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 3 DS2 RGB", NTV2_XptFrameBuffer3_DS2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 4 DS2 YUV", NTV2_XptFrameBuffer4_DS2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 4 DS2 RGB", NTV2_XptFrameBuffer4_DS2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 5 DS2 YUV", NTV2_XptFrameBuffer5_DS2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 5 DS2 RGB", NTV2_XptFrameBuffer5_DS2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 6 DS2 YUV", NTV2_XptFrameBuffer6_DS2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 6 DS2 RGB", NTV2_XptFrameBuffer6_DS2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 7 DS2 YUV", NTV2_XptFrameBuffer7_DS2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 7 DS2 RGB", NTV2_XptFrameBuffer7_DS2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 8 DS2 YUV", NTV2_XptFrameBuffer8_DS2YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "FB 8 DS2 RGB", NTV2_XptFrameBuffer8_DS2RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Runtime Calc", NTV2_XptRuntimeCalc);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Multi-Link Out 1 DS1", NTV2_XptMultiLinkOut1DS1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Multi-Link Out 1 DS2", NTV2_XptMultiLinkOut1DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Multi-Link Out 1 DS3", NTV2_XptMultiLinkOut1DS3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Multi-Link Out 1 DS4", NTV2_XptMultiLinkOut1DS4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Multi-Link Out 2 DS1", NTV2_XptMultiLinkOut2DS1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Multi-Link Out 2 DS2", NTV2_XptMultiLinkOut2DS2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Multi-Link Out 2 DS3", NTV2_XptMultiLinkOut2DS3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Multi-Link Out 2 DS4", NTV2_XptMultiLinkOut2DS4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "3D LUT 1 YUV", NTV2_Xpt3DLUT1YUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "3D LUT 1 RGB", NTV2_Xpt3DLUT1RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "OE Out YUV", NTV2_XptOEOutYUV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "OE Out RGB", NTV2_XptOEOutRGB);
	#if !defined(NTV2_DEPRECATE_16_0)
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "WaterMarker 1 RGB", NTV2_XptWaterMarkerRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "WaterMarker 2 RGB", NTV2_XptWaterMarker2RGB);
	#endif
	#if !defined(_DEBUG)
	default:								break;
	#endif
	}	//	switch on inValue
	return "";
}	//	NTV2OutputCrosspointIDToString


string NTV2WidgetIDToString (const NTV2WidgetID inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FB1", NTV2_WgtFrameBuffer1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FB2", NTV2_WgtFrameBuffer2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FB3", NTV2_WgtFrameBuffer3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FB4", NTV2_WgtFrameBuffer4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC1", NTV2_WgtCSC1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC2", NTV2_WgtCSC2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT1", NTV2_WgtLUT1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT2", NTV2_WgtLUT2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FS1", NTV2_WgtFrameSync1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FS2", NTV2_WgtFrameSync2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDIIn1", NTV2_WgtSDIIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDIIn2", NTV2_WgtSDIIn2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIIn1", NTV2_Wgt3GSDIIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIIn2", NTV2_Wgt3GSDIIn2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIIn3", NTV2_Wgt3GSDIIn3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIIn4", NTV2_Wgt3GSDIIn4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDIOut1", NTV2_WgtSDIOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDIOut2", NTV2_WgtSDIOut2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDIOut3", NTV2_WgtSDIOut3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDIOut4", NTV2_WgtSDIOut4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIOut1", NTV2_Wgt3GSDIOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIOut2", NTV2_Wgt3GSDIOut2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIOut3", NTV2_Wgt3GSDIOut3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIOut4", NTV2_Wgt3GSDIOut4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLIn1", NTV2_WgtDualLinkIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2In1", NTV2_WgtDualLinkV2In1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2In2", NTV2_WgtDualLinkV2In2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLOut1", NTV2_WgtDualLinkOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLOut2", NTV2_WgtDualLinkOut2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2Out1", NTV2_WgtDualLinkV2Out1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2Out2", NTV2_WgtDualLinkV2Out2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "AnlgIn1", NTV2_WgtAnalogIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "AnlgOut1", NTV2_WgtAnalogOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "AnlgCompOut1", NTV2_WgtAnalogCompositeOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIIn1", NTV2_WgtHDMIIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIOut1", NTV2_WgtHDMIOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "UDC1", NTV2_WgtUpDownConverter1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "UDC2", NTV2_WgtUpDownConverter2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Mixer1", NTV2_WgtMixer1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Compress1", NTV2_WgtCompression1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "ProcAmp1", NTV2_WgtProcAmp1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "WaterMrkr1", NTV2_WgtWaterMarker1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "WaterMrkr2", NTV2_WgtWaterMarker2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "IICT1", NTV2_WgtIICT1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "IICT2", NTV2_WgtIICT2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "TestPat1", NTV2_WgtTestPattern1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "GenLock", NTV2_WgtGenLock);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DCIMixer1", NTV2_WgtDCIMixer1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Mixer2", NTV2_WgtMixer2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "StereoComp", NTV2_WgtStereoCompressor);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT3", NTV2_WgtLUT3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT4", NTV2_WgtLUT4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2In3", NTV2_WgtDualLinkV2In3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2In4", NTV2_WgtDualLinkV2In4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2Out3", NTV2_WgtDualLinkV2Out3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2Out4", NTV2_WgtDualLinkV2Out4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC3", NTV2_WgtCSC3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC4", NTV2_WgtCSC4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv2In1", NTV2_WgtHDMIIn1v2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv2Out1", NTV2_WgtHDMIOut1v2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDIMonOut1", NTV2_WgtSDIMonOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC5", NTV2_WgtCSC5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT5", NTV2_WgtLUT5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2Out5", NTV2_WgtDualLinkV2Out5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "4KDC", NTV2_Wgt4KDownConverter);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIIn5", NTV2_Wgt3GSDIIn5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIIn6", NTV2_Wgt3GSDIIn6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIIn7", NTV2_Wgt3GSDIIn7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIIn8", NTV2_Wgt3GSDIIn8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIOut5", NTV2_Wgt3GSDIOut5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIOut6", NTV2_Wgt3GSDIOut6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIOut7", NTV2_Wgt3GSDIOut7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3GSDIOut8", NTV2_Wgt3GSDIOut8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2In5", NTV2_WgtDualLinkV2In5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2In6", NTV2_WgtDualLinkV2In6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2In7", NTV2_WgtDualLinkV2In7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2In8", NTV2_WgtDualLinkV2In8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2Out6", NTV2_WgtDualLinkV2Out6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2Out7", NTV2_WgtDualLinkV2Out7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DLv2Out8", NTV2_WgtDualLinkV2Out8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC6", NTV2_WgtCSC6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC7", NTV2_WgtCSC7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC8", NTV2_WgtCSC8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT6", NTV2_WgtLUT6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT7", NTV2_WgtLUT7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT8", NTV2_WgtLUT8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Mixer3", NTV2_WgtMixer3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Mixer4", NTV2_WgtMixer4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FB5", NTV2_WgtFrameBuffer5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FB6", NTV2_WgtFrameBuffer6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FB7", NTV2_WgtFrameBuffer7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FB8", NTV2_WgtFrameBuffer8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv3In1", NTV2_WgtHDMIIn1v3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv3Out1", NTV2_WgtHDMIOut1v3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "425Mux1", NTV2_Wgt425Mux1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "425Mux2", NTV2_Wgt425Mux2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "425Mux3", NTV2_Wgt425Mux3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "425Mux4", NTV2_Wgt425Mux4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12GSDIIn1", NTV2_Wgt12GSDIIn1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12GSDIIn2", NTV2_Wgt12GSDIIn2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12GSDIIn3", NTV2_Wgt12GSDIIn3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12GSDIIn4", NTV2_Wgt12GSDIIn4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12GSDIOut1", NTV2_Wgt12GSDIOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12GSDIOut2", NTV2_Wgt12GSDIOut2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12GSDIOut3", NTV2_Wgt12GSDIOut3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12GSDIOut4", NTV2_Wgt12GSDIOut4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv4In1", NTV2_WgtHDMIIn1v4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv4In2", NTV2_WgtHDMIIn2v4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv4In3", NTV2_WgtHDMIIn3v4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv4In4", NTV2_WgtHDMIIn4v4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv4Out1", NTV2_WgtHDMIOut1v4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMIv5Out1", NTV2_WgtHDMIOut1v5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "MultiLinkOut1", NTV2_WgtMultiLinkOut1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "MultiLinkOut2", NTV2_WgtMultiLinkOut2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3DLUT1", NTV2_Wgt3DLUT1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "OE1", NTV2_WgtOE1);
		case NTV2_WgtModuleTypeCount:				return "???";  //special case
	}
	return "";

}	//	NTV2WidgetIDToString

string NTV2WidgetTypeToString (const NTV2WidgetType inValue, const bool inCompactDisplay)
{
	switch (inValue) {
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FrameStore", NTV2WidgetType_FrameStore);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "CSC", NTV2WidgetType_CSC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LUT", NTV2WidgetType_LUT);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FrameSync", NTV2WidgetType_FrameSync);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI Input", NTV2WidgetType_SDIIn);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI Input 3G", NTV2WidgetType_SDIIn3G);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI Output", NTV2WidgetType_SDIOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI Output 3G", NTV2WidgetType_SDIOut3G);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI Monitor Output", NTV2WidgetType_SDIMonOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DualLink Input V1", NTV2WidgetType_DualLinkV1In);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DualLink Input V2", NTV2WidgetType_DualLinkV2In);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DualLink Output V1", NTV2WidgetType_DualLinkV1Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DualLink Output V2", NTV2WidgetType_DualLinkV2Out);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Analog Input", NTV2WidgetType_AnalogIn);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Analog Output", NTV2WidgetType_AnalogOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Analog Composite Output", NTV2WidgetType_AnalogCompositeOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Input V1", NTV2WidgetType_HDMIInV1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Input V2", NTV2WidgetType_HDMIInV2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Input V3", NTV2WidgetType_HDMIInV3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Input V4", NTV2WidgetType_HDMIInV4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Up-Down Converter", NTV2WidgetType_UpDownConverter);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Mixer", NTV2WidgetType_Mixer);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DCI Mixer", NTV2WidgetType_DCIMixer);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Compression", NTV2WidgetType_Compression);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Stereo Compressor", NTV2WidgetType_StereoCompressor);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Proc Amp", NTV2WidgetType_ProcAmp);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Genlock", NTV2WidgetType_GenLock);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "4K Down Converter", NTV2WidgetType_4KDownConverter);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Output V1", NTV2WidgetType_HDMIOutV1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Output V2", NTV2WidgetType_HDMIOutV2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Output V3", NTV2WidgetType_HDMIOutV3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Output V4", NTV2WidgetType_HDMIOutV4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI Output V5", NTV2WidgetType_HDMIOutV5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SMPTE 425 Mux", NTV2WidgetType_SMPTE425Mux);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI Input 12G", NTV2WidgetType_SDIIn12G);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI Output 12G", NTV2WidgetType_SDIOut12G);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Multi-Link Output", NTV2WidgetType_MultiLinkOut);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3D LUT", NTV2WidgetType_LUT3D);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "OE", NTV2WidgetType_OE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Watermarker", NTV2WidgetType_WaterMarker);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "IICT", NTV2WidgetType_IICT);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Test Pattern", NTV2WidgetType_TestPattern);
		case NTV2WidgetType_Max:				return "???";
	}
	return "";
}

string NTV2TaskModeToString (const NTV2EveryFrameTaskMode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Disabled", NTV2_DISABLE_TASKS);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Standard", NTV2_STANDARD_TASKS);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "OEM", NTV2_OEM_TASKS);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "??", NTV2_TASK_MODE_INVALID);
	}
	return "";
}


string NTV2RegNumSetToString (const NTV2RegisterNumberSet & inObj)
{
	ostringstream	oss;
	oss << inObj;
	return oss.str ();
}


ostream & operator << (ostream & inOutStr, const NTV2RegisterNumberSet & inObj)
{
	inOutStr << "[" << inObj.size () << " regs: ";
	for (NTV2RegNumSetConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStr << ::NTV2RegisterNumberToString (NTV2RegisterNumber (*iter));
		if (++iter != inObj.end ())
			inOutStr << ", ";
	}
	return inOutStr << "]";
}


NTV2RegisterNumberSet & operator << (NTV2RegisterNumberSet & inOutSet, const ULWord inRegisterNumber)
{
	inOutSet.insert (inRegisterNumber);
	return inOutSet;
}


string NTV2TCIndexToString (const NTV2TCIndex inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "DEFAULT", NTV2_TCINDEX_DEFAULT);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI1-VITC", NTV2_TCINDEX_SDI1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI2-VITC", NTV2_TCINDEX_SDI2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI3-VITC", NTV2_TCINDEX_SDI3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI4-VITC", NTV2_TCINDEX_SDI4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI1-LTC", NTV2_TCINDEX_SDI1_LTC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI2-LTC", NTV2_TCINDEX_SDI2_LTC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LTC1", NTV2_TCINDEX_LTC1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LTC2", NTV2_TCINDEX_LTC2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI5-VITC", NTV2_TCINDEX_SDI5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI6-VITC", NTV2_TCINDEX_SDI6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI7-VITC", NTV2_TCINDEX_SDI7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI8-VITC", NTV2_TCINDEX_SDI8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI3-LTC", NTV2_TCINDEX_SDI3_LTC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI4-LTC", NTV2_TCINDEX_SDI4_LTC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI5-LTC", NTV2_TCINDEX_SDI5_LTC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI6-LTC", NTV2_TCINDEX_SDI6_LTC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI7-LTC", NTV2_TCINDEX_SDI7_LTC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI8-LTC", NTV2_TCINDEX_SDI8_LTC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI1-VITC2", NTV2_TCINDEX_SDI1_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI2-VITC2", NTV2_TCINDEX_SDI2_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI3-VITC2", NTV2_TCINDEX_SDI3_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI4-VITC2", NTV2_TCINDEX_SDI4_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI5-VITC2", NTV2_TCINDEX_SDI5_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI6-VITC2", NTV2_TCINDEX_SDI6_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI7-VITC2", NTV2_TCINDEX_SDI7_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI8-VITC2", NTV2_TCINDEX_SDI8_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "", NTV2_TCINDEX_INVALID);
	}
	return "";
}


string NTV2AudioChannelPairToString (const NTV2AudioChannelPair inValue, const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_AUDIO_CHANNEL_PAIR(inValue))
		oss << (inCompactDisplay ? "" : "NTV2_AudioChannel")  <<  DEC(inValue * 2 + 1)  <<  (inCompactDisplay ? "-" : "_")  <<  DEC(inValue * 2 + 2);
	else if (!inCompactDisplay)
		oss << "NTV2_AUDIO_CHANNEL_PAIR_INVALID";
	return oss.str();
}


string NTV2AudioChannelQuadToString (const NTV2Audio4ChannelSelect inValue, const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_AUDIO_CHANNEL_QUAD (inValue))
		oss << (inCompactDisplay ? "" : "NTV2_AudioChannel")  <<  (inValue * 4 + 1)  <<  (inCompactDisplay ? "-" : "_")  <<  (inValue * 4 + 4);
	else if (!inCompactDisplay)
		oss << "NTV2_AUDIO_CHANNEL_QUAD_INVALID";
	return oss.str ();
}


string NTV2AudioChannelOctetToString (const NTV2Audio8ChannelSelect inValue, const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_AUDIO_CHANNEL_OCTET (inValue))
		oss << (inCompactDisplay ? "" : "NTV2_AudioChannel")  <<  (inValue * 8 + 1)  <<  (inCompactDisplay ? "-" : "_")  <<  (inValue * 8 + 8);
	else if (!inCompactDisplay)
		oss << "NTV2_AUDIO_CHANNEL_OCTET_INVALID";
	return oss.str ();
}


string NTV2FramesizeToString (const NTV2Framesize inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "2MB", NTV2_FRAMESIZE_2MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "4MB", NTV2_FRAMESIZE_4MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "8MB", NTV2_FRAMESIZE_8MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "16MB", NTV2_FRAMESIZE_16MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "6MB", NTV2_FRAMESIZE_6MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "10MB", NTV2_FRAMESIZE_10MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "12MB", NTV2_FRAMESIZE_12MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "14MB", NTV2_FRAMESIZE_14MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "18MB", NTV2_FRAMESIZE_18MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "20MB", NTV2_FRAMESIZE_20MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "22MB", NTV2_FRAMESIZE_22MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "24MB", NTV2_FRAMESIZE_24MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "26MB", NTV2_FRAMESIZE_26MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "28MB", NTV2_FRAMESIZE_28MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "30MB", NTV2_FRAMESIZE_30MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "32MB", NTV2_FRAMESIZE_32MB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "", NTV2_FRAMESIZE_INVALID);
	}
	return "";
}


string NTV2ModeToString (const NTV2Mode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Output", NTV2_MODE_DISPLAY);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Input", NTV2_MODE_CAPTURE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "", NTV2_MODE_INVALID);
	}
	return "";
}


string NTV2VANCModeToString (const NTV2VANCMode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "off", NTV2_VANCMODE_OFF);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "tall", NTV2_VANCMODE_TALL);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "taller", NTV2_VANCMODE_TALLER);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "", NTV2_VANCMODE_INVALID);
	}
	return "";
}


string NTV2MixerKeyerModeToString (const NTV2MixerKeyerMode inValue, const bool inCompactDisplay)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FGOn", NTV2MIXERMODE_FOREGROUND_ON);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Mix", NTV2MIXERMODE_MIX);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Split", NTV2MIXERMODE_SPLIT);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FGOff", NTV2MIXERMODE_FOREGROUND_OFF);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "", NTV2MIXERMODE_INVALID);
	}
	return "";
}


string NTV2MixerInputControlToString (const NTV2MixerKeyerInputControl inValue, const bool inCompactDisplay)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "FullRaster", NTV2MIXERINPUTCONTROL_FULLRASTER);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Shaped", NTV2MIXERINPUTCONTROL_SHAPED);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Unshaped", NTV2MIXERINPUTCONTROL_UNSHAPED);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "", NTV2MIXERINPUTCONTROL_INVALID);
	}
	return "";
}


string NTV2VideoLimitingToString (const NTV2VideoLimiting inValue, const bool inCompactDisplay)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LegalSDI", NTV2_VIDEOLIMITING_LEGALSDI);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Off", NTV2_VIDEOLIMITING_OFF);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "LegalBroadcast", NTV2_VIDEOLIMITING_LEGALBROADCAST);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "", NTV2_VIDEOLIMITING_INVALID);
	}
	return "";
}


string NTV2BreakoutTypeToString (const NTV2BreakoutType inValue, const bool inCompactDisplay)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "None", NTV2_BreakoutNone);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "XLR", NTV2_BreakoutCableXLR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "BNC", NTV2_BreakoutCableBNC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "KBox", NTV2_KBox);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "KLBox", NTV2_KLBox);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "K3Box", NTV2_K3Box);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "KLHiBox", NTV2_KLHiBox);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "KLHePlusBox", NTV2_KLHePlusBox);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "K3GBox", NTV2_K3GBox);
		case NTV2_MAX_NUM_BreakoutTypes:			break;  //special case
	}
	return "";
}

string NTV2AncDataRgnToStr (const NTV2AncDataRgn inValue, const bool inCompactDisplay)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "AncF1", NTV2_AncRgn_Field1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "AncF2", NTV2_AncRgn_Field2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "MonAncF1", NTV2_AncRgn_MonField1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "MonAncF2", NTV2_AncRgn_MonField2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "AncAll", NTV2_AncRgn_All);
		case NTV2_MAX_NUM_AncRgns:					break;  //special case
	}
	return "";
}

string NTV2UpConvertModeToString (const NTV2UpConvertMode inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Anamorphic",						NTV2_UpConvertAnamorphic);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "4" "\xC3\x97" "3 Pillar Box",	NTV2_UpConvertPillarbox4x3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Zoomed 14" "\xC3\x97" "9",		NTV2_UpConvertZoom14x9);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Zoomed Letterbox",				NTV2_UpConvertZoomLetterbox);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Zoomed Wide",					NTV2_UpConvertZoomWide);
		case NTV2_MAX_NUM_UpConvertModes:			break;  //special case
	}
	return "";
}

string NTV2DownConvertModeToString (const NTV2DownConvertMode inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Letterbox",						NTV2_DownConvertLetterbox);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Cropped",						NTV2_DownConvertCrop);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Anamorphic",						NTV2_DownConvertAnamorphic);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Zoomed 14" "\xC3\x97" "9",		NTV2_DownConvert14x9);
		case NTV2_MAX_NUM_DownConvertModes:			break;  //special case
	}
	return "";
}

string NTV2IsoConvertModeToString (const NTV2IsoConvertMode inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Letterbox",						NTV2_IsoLetterBox);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Horiz Cropped",					NTV2_IsoHCrop);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Vert Cropped",					NTV2_IsoVCrop);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Pillar Box",						NTV2_IsoPillarBox);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "14" "\xC3\x97" "9",				NTV2_Iso14x9);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Pass-Through",					NTV2_IsoPassThrough);
		case NTV2_MAX_NUM_IsoConvertModes:			break;  //special case
	}
	return "";
}

string NTV2HDMIBitDepthToString (const NTV2HDMIBitDepth inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "8-bit",		NTV2_HDMI8Bit);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "10-bit",		NTV2_HDMI10Bit);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "12-bit",		NTV2_HDMI12Bit);
		case NTV2_INVALID_HDMIBitDepth:		break;
	}
	return "";
}

string NTV2HDMIAudioChannelsToString (const NTV2HDMIAudioChannels inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "2-chl",		NTV2_HDMIAudio2Channels);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "8-chl",		NTV2_HDMIAudio8Channels);
		case NTV2_INVALID_HDMI_AUDIO_CHANNELS:		break;
	}
	return "";
}

string NTV2HDMIProtocolToString (const NTV2HDMIProtocol inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "HDMI",		NTV2_HDMIProtocolHDMI);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "DVI",		NTV2_HDMIProtocolDVI);
		case NTV2_INVALID_HDMI_PROTOCOL:		break;
	}
	return "";
}

string NTV2HDMIRangeToString (const NTV2HDMIRange inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "SMPTE",		NTV2_HDMIRangeSMPTE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Full",		NTV2_HDMIRangeFull);
		case NTV2_INVALID_HDMI_RANGE:		break;
	}
	return "";
}

string NTV2HDMIColorSpaceToString (const NTV2HDMIColorSpace inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Auto",		NTV2_HDMIColorSpaceAuto);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "RGB",		NTV2_HDMIColorSpaceRGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "YCbCr",		NTV2_HDMIColorSpaceYCbCr);
		case NTV2_INVALID_HDMI_COLORSPACE:		break;
	}
	return "";
}

string NTV2AudioFormatToString (const NTV2AudioFormat inValue, const bool inCompact)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "LPCM",		NTV2_AUDIO_FORMAT_LPCM);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompact, "Dolby",		NTV2_AUDIO_FORMAT_DOLBY);
		case NTV2_AUDIO_FORMAT_INVALID:		break;
	}
	return "";
}

string NTV2EmbeddedAudioInputToString (const NTV2EmbeddedAudioInput inValue,  const bool inCompactDisplay)
{
	switch(inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI1", NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI2", NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI3", NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI4", NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI5", NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI6", NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI7", NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI8", NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI?", NTV2_EMBEDDED_AUDIO_INPUT_INVALID);
	}
	return "";
}


string NTV2AudioSourceToString (const NTV2AudioSource inValue,  const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "SDI", NTV2_AUDIO_EMBEDDED);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "AES", NTV2_AUDIO_AES);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Analog", NTV2_AUDIO_ANALOG);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HDMI", NTV2_AUDIO_HDMI);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Mic", NTV2_AUDIO_MIC);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "", NTV2_AUDIO_SOURCE_INVALID);
	}
	return "";
}


string NTV2VideoFormatToString (const NTV2VideoFormat inFormat, const bool inUseFrameRate)
{
	switch (inFormat)
	{
	case NTV2_FORMAT_1080i_5000:	return inUseFrameRate ? "1080i25" 		: "1080i50";
	case NTV2_FORMAT_1080i_5994:	return inUseFrameRate ? "1080i29.97" 	: "1080i59.94";
	case NTV2_FORMAT_1080i_6000:	return inUseFrameRate ? "1080i30" 		: "1080i60";
	case NTV2_FORMAT_720p_5994:		return "720p59.94";
	case NTV2_FORMAT_720p_6000:		return "720p60";
	case NTV2_FORMAT_1080psf_2398:	return "1080sf23.98";
	case NTV2_FORMAT_1080psf_2400:	return "1080sf24";
	case NTV2_FORMAT_1080p_2997:	return "1080p29.97";
	case NTV2_FORMAT_1080p_3000:	return "1080p30";
	case NTV2_FORMAT_1080p_2500:	return "1080p25";
	case NTV2_FORMAT_1080p_2398:	return "1080p23.98";
	case NTV2_FORMAT_1080p_2400:	return "1080p24";
	case NTV2_FORMAT_1080p_2K_2398:	return "2Kp23.98";
	case NTV2_FORMAT_1080p_2K_2400:	return "2Kp24";
	case NTV2_FORMAT_1080psf_2K_2398:	return "2Ksf23.98";
	case NTV2_FORMAT_1080psf_2K_2400:	return "2Ksf24";
	case NTV2_FORMAT_720p_5000:		return "720p50";
	case NTV2_FORMAT_1080p_5000_B:	return "1080p50b";
	case NTV2_FORMAT_1080p_5994_B:	return "1080p59.94b";
	case NTV2_FORMAT_1080p_6000_B:	return "1080p60b";
	case NTV2_FORMAT_720p_2398:		return "720p23.98";
	case NTV2_FORMAT_720p_2500:		return "720p25";
	case NTV2_FORMAT_1080p_5000_A:	return "1080p50a";
	case NTV2_FORMAT_1080p_5994_A:	return "1080p59.94a";
	case NTV2_FORMAT_1080p_6000_A:	return "1080p60a";
	case NTV2_FORMAT_1080p_2K_2500:	return "2Kp25";
	case NTV2_FORMAT_1080psf_2K_2500:	return "2Ksf25";
	case NTV2_FORMAT_1080psf_2500_2:	return "1080sf25";
	case NTV2_FORMAT_1080psf_2997_2:	return "1080sf29.97";
	case NTV2_FORMAT_1080psf_3000_2:	return "1080sf30";
	case NTV2_FORMAT_525_5994:		return inUseFrameRate ? "525i29.97" : "525i59.94";
	case NTV2_FORMAT_625_5000:		return inUseFrameRate ? "625i25" 	: "625i50";
	case NTV2_FORMAT_525_2398:		return "525i23.98";
	case NTV2_FORMAT_525_2400:		return "525i24";
	case NTV2_FORMAT_525psf_2997:	return "525sf29.97";
	case NTV2_FORMAT_625psf_2500:	return "625sf25";
	case NTV2_FORMAT_2K_1498:		return "2Kx1556sf14.98";
	case NTV2_FORMAT_2K_1500:		return "2Kx1556sf15";
	case NTV2_FORMAT_2K_2398:		return "2Kx1556sf23.98";
	case NTV2_FORMAT_2K_2400:		return "2Kx1556sf24";
	case NTV2_FORMAT_2K_2500:		return "2Kx1556sf25";
	case NTV2_FORMAT_4x1920x1080psf_2398:	return "UHDsf23.98";
	case NTV2_FORMAT_4x1920x1080psf_2400:	return "UHDsf24";
	case NTV2_FORMAT_4x1920x1080psf_2500:	return "UHDsf25";
	case NTV2_FORMAT_4x1920x1080p_2398:		return "UHDp23.98";
	case NTV2_FORMAT_4x1920x1080p_2400:		return "UHDp24";
	case NTV2_FORMAT_4x1920x1080p_2500:		return "UHDp25";
	case NTV2_FORMAT_4x2048x1080psf_2398:	return "4Ksf23.98";
	case NTV2_FORMAT_4x2048x1080psf_2400:	return "4Ksf24";
	case NTV2_FORMAT_4x2048x1080psf_2500:	return "4Ksf25";
	case NTV2_FORMAT_4x2048x1080p_2398:		return "4Kp23.98";
	case NTV2_FORMAT_4x2048x1080p_2400:		return "4Kp24";
	case NTV2_FORMAT_4x2048x1080p_2500:		return "4Kp25";
	case NTV2_FORMAT_4x1920x1080p_2997:		return "UHDp29.97";
	case NTV2_FORMAT_4x1920x1080p_3000:		return "UHDp30";
	case NTV2_FORMAT_4x1920x1080psf_2997:	return "UHDsf29.97";
	case NTV2_FORMAT_4x1920x1080psf_3000:	return "UHDsf30";
	case NTV2_FORMAT_4x2048x1080p_2997:		return "4Kp29.97";
	case NTV2_FORMAT_4x2048x1080p_3000:		return "4Kp30";
	case NTV2_FORMAT_4x2048x1080psf_2997:	return "4Ksf29.97";
	case NTV2_FORMAT_4x2048x1080psf_3000:	return "4Ksf30";
	case NTV2_FORMAT_4x1920x1080p_5000:		return "UHDp50";
	case NTV2_FORMAT_4x1920x1080p_5994:		return "UHDp59.94";
	case NTV2_FORMAT_4x1920x1080p_6000:		return "UHDp60";
	case NTV2_FORMAT_4x2048x1080p_5000:		return "4Kp50";
	case NTV2_FORMAT_4x2048x1080p_5994:		return "4Kp59.94";
	case NTV2_FORMAT_4x2048x1080p_6000:		return "4Kp60";
	case NTV2_FORMAT_4x2048x1080p_4795:		return "4Kp47.95";
	case NTV2_FORMAT_4x2048x1080p_4800:		return "4Kp48";
	case NTV2_FORMAT_4x2048x1080p_11988:	return "4Kp119";
	case NTV2_FORMAT_4x2048x1080p_12000:	return "4Kp120";
	case NTV2_FORMAT_1080p_2K_6000_A:	return "2Kp60a";
	case NTV2_FORMAT_1080p_2K_5994_A:	return "2Kp59.94a";
	case NTV2_FORMAT_1080p_2K_2997:		return "2Kp29.97";
	case NTV2_FORMAT_1080p_2K_3000:		return "2Kp30";
	case NTV2_FORMAT_1080p_2K_5000_A:	return "2Kp50a";
	case NTV2_FORMAT_1080p_2K_4795_A:	return "2Kp47.95a";
	case NTV2_FORMAT_1080p_2K_4800_A:	return "2Kp48a";
	case NTV2_FORMAT_1080p_2K_4795_B:	return "2Kp47.95b";
	case NTV2_FORMAT_1080p_2K_4800_B:	return "2Kp48b";
	case NTV2_FORMAT_1080p_2K_5000_B:	return "2Kp50b";
	case NTV2_FORMAT_1080p_2K_5994_B:	return "2Kp59.94b";
	case NTV2_FORMAT_1080p_2K_6000_B:	return "2Kp60b";
	case NTV2_FORMAT_3840x2160psf_2398:	return "UHDsf23.98";
	case NTV2_FORMAT_3840x2160psf_2400:	return "UHDsf24";
	case NTV2_FORMAT_3840x2160psf_2500:	return "UHDsf25";
	case NTV2_FORMAT_3840x2160p_2398:	return "UHDp23.98";
	case NTV2_FORMAT_3840x2160p_2400:	return "UHDp24";
	case NTV2_FORMAT_3840x2160p_2500:	return "UHDp25";
	case NTV2_FORMAT_3840x2160p_2997:	return "UHDp29.97";
	case NTV2_FORMAT_3840x2160p_3000:	return "UHDp30";
	case NTV2_FORMAT_3840x2160psf_2997:	return "UHDsf29.97";
	case NTV2_FORMAT_3840x2160psf_3000:	return "UHDsf30";
	case NTV2_FORMAT_3840x2160p_5000:	return "UHDp50";
	case NTV2_FORMAT_3840x2160p_5994:	return "UHDp59.94";
	case NTV2_FORMAT_3840x2160p_6000:	return "UHDp60";
	case NTV2_FORMAT_4096x2160psf_2398:	return "4Ksf23.98";
	case NTV2_FORMAT_4096x2160psf_2400:	return "4Ksf24";
	case NTV2_FORMAT_4096x2160psf_2500:	return "4Ksf25";
	case NTV2_FORMAT_4096x2160p_2398:	return "4Kp23.98";
	case NTV2_FORMAT_4096x2160p_2400:	return "4Kp24";
	case NTV2_FORMAT_4096x2160p_2500:	return "4Kp25";
	case NTV2_FORMAT_4096x2160p_2997:	return "4Kp29.97";
	case NTV2_FORMAT_4096x2160p_3000:	return "4Kp30";
	case NTV2_FORMAT_4096x2160psf_2997:	return "4Ksf29.97";
	case NTV2_FORMAT_4096x2160psf_3000:	return "4Ksf30";
	case NTV2_FORMAT_4096x2160p_4795:	return "4Kp47.95";
	case NTV2_FORMAT_4096x2160p_4800:	return "4Kp48";
	case NTV2_FORMAT_4096x2160p_5000:	return "4Kp50";
	case NTV2_FORMAT_4096x2160p_5994:	return "4Kp59.94";
	case NTV2_FORMAT_4096x2160p_6000:	return "4Kp60";
	case NTV2_FORMAT_4096x2160p_11988:	return "4Kp119";
	case NTV2_FORMAT_4096x2160p_12000:	return "4Kp120";
	case NTV2_FORMAT_4x1920x1080p_5000_B:		return "UHDp50b";
	case NTV2_FORMAT_4x1920x1080p_5994_B:		return "UHDp59.94b";
	case NTV2_FORMAT_4x1920x1080p_6000_B:		return "UHDp60b";
	case NTV2_FORMAT_4x2048x1080p_5000_B:		return "4Kp50b";
	case NTV2_FORMAT_4x2048x1080p_5994_B:		return "4Kp59.94b";
	case NTV2_FORMAT_4x2048x1080p_6000_B:		return "4Kp60b";
	case NTV2_FORMAT_4x2048x1080p_4795_B:		return "4Kp47.95b";
	case NTV2_FORMAT_4x2048x1080p_4800_B:		return "4Kp48b";
	case NTV2_FORMAT_3840x2160p_5000_B:	return "UHDp50b";
	case NTV2_FORMAT_3840x2160p_5994_B:	return "UHDp59.94b";
	case NTV2_FORMAT_3840x2160p_6000_B:	return "UHDp60b";
	case NTV2_FORMAT_4096x2160p_4795_B:	return "4Kp47.95b";
	case NTV2_FORMAT_4096x2160p_4800_B:	return "4Kp48b";
	case NTV2_FORMAT_4096x2160p_5000_B:	return "4Kp50b";
	case NTV2_FORMAT_4096x2160p_5994_B:	return "4Kp59.94b";
	case NTV2_FORMAT_4096x2160p_6000_B:	return "4Kp60b";
	case NTV2_FORMAT_4x3840x2160p_2398: return "UHD2p23.98";
	case NTV2_FORMAT_4x3840x2160p_2400: return "UHD2p24";
	case NTV2_FORMAT_4x3840x2160p_2500: return "UHD2p25";
	case NTV2_FORMAT_4x3840x2160p_2997: return "UHD2p29.97";
	case NTV2_FORMAT_4x3840x2160p_3000: return "UHD2p30";
	case NTV2_FORMAT_4x3840x2160p_5000: return "UHD2p50";
	case NTV2_FORMAT_4x3840x2160p_5994: return "UHD2p59.94";
	case NTV2_FORMAT_4x3840x2160p_6000: return "UHD2p60";
	case NTV2_FORMAT_4x3840x2160p_5000_B: return "UHD2p50b";
	case NTV2_FORMAT_4x3840x2160p_5994_B: return "UHD2p59.94b";
	case NTV2_FORMAT_4x3840x2160p_6000_B: return "UHD2p60b";
	case NTV2_FORMAT_4x4096x2160p_2398: return "8Kp23.98";
	case NTV2_FORMAT_4x4096x2160p_2400: return "8Kp24";
	case NTV2_FORMAT_4x4096x2160p_2500: return "8Kp25";
	case NTV2_FORMAT_4x4096x2160p_2997: return "8Kp29.97";
	case NTV2_FORMAT_4x4096x2160p_3000: return "8Kp30";
	case NTV2_FORMAT_4x4096x2160p_4795: return "8Kp47.95";
	case NTV2_FORMAT_4x4096x2160p_4800: return "8Kp48";
	case NTV2_FORMAT_4x4096x2160p_5000: return "8Kp50";
	case NTV2_FORMAT_4x4096x2160p_5994: return "8Kp59.94";
	case NTV2_FORMAT_4x4096x2160p_6000: return "8Kp60";
	case NTV2_FORMAT_4x4096x2160p_4795_B: return "8Kp47.95b";
	case NTV2_FORMAT_4x4096x2160p_4800_B: return "8Kp48b";
	case NTV2_FORMAT_4x4096x2160p_5000_B: return "8Kp50b";
	case NTV2_FORMAT_4x4096x2160p_5994_B: return "8Kp59.94b";
	case NTV2_FORMAT_4x4096x2160p_6000_B: return "8Kp60b";
	default: return "Unknown";
	}
}	//	NTV2VideoFormatToString


string NTV2StandardToString (const NTV2Standard inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "1080i", NTV2_STANDARD_1080);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "720p", NTV2_STANDARD_720);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "525i", NTV2_STANDARD_525);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "625i", NTV2_STANDARD_625);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "1080p", NTV2_STANDARD_1080p);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2K", NTV2_STANDARD_2K);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2K1080p", NTV2_STANDARD_2Kx1080p);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2K1080i", NTV2_STANDARD_2Kx1080i);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "UHD", NTV2_STANDARD_3840x2160p);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4K", NTV2_STANDARD_4096x2160p);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "UHD HFR", NTV2_STANDARD_3840HFR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4K HFR", NTV2_STANDARD_4096HFR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "UHD2", NTV2_STANDARD_7680);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "8K", NTV2_STANDARD_8192);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "UHDsf", NTV2_STANDARD_3840i);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4Ksf", NTV2_STANDARD_4096i);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "", NTV2_STANDARD_INVALID);
	}
	return "";
}


string NTV2FrameBufferFormatToString (const NTV2FrameBufferFormat inValue,	const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-10", NTV2_FBF_10BIT_YCBCR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-8", NTV2_FBF_8BIT_YCBCR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RGBA-8", NTV2_FBF_ARGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ARGB-8", NTV2_FBF_RGBA);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RGB-10", NTV2_FBF_10BIT_RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUY2-8", NTV2_FBF_8BIT_YCBCR_YUY2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ABGR-8", NTV2_FBF_ABGR);
        NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RGB-10DPX", NTV2_FBF_10BIT_DPX);
        NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-10DPX", NTV2_FBF_10BIT_YCBCR_DPX);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "DVCProHD", NTV2_FBF_8BIT_DVCPRO);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-P420", NTV2_FBF_8BIT_YCBCR_420PL3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDV", NTV2_FBF_8BIT_HDV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RGB-8", NTV2_FBF_24BIT_RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "BGR-8", NTV2_FBF_24BIT_BGR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUVA-10", NTV2_FBF_10BIT_YCBCRA);
        NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RGB-10LDPX", NTV2_FBF_10BIT_DPX_LE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RGB-12", NTV2_FBF_48BIT_RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RGB-12P", NTV2_FBF_12BIT_RGB_PACKED);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ProRes-DVC", NTV2_FBF_PRORES_DVCPRO);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ProRes-HDV", NTV2_FBF_PRORES_HDV);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RGB-P10", NTV2_FBF_10BIT_RGB_PACKED);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ARGB-10", NTV2_FBF_10BIT_ARGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "ARGB-16", NTV2_FBF_16BIT_ARGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-P8", NTV2_FBF_8BIT_YCBCR_422PL3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RAW-RGB10", NTV2_FBF_10BIT_RAW_RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "RAW-YUV10", NTV2_FBF_10BIT_RAW_YCBCR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-P420-L10", NTV2_FBF_10BIT_YCBCR_420PL3_LE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-P-L10", NTV2_FBF_10BIT_YCBCR_422PL3_LE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-P420-10", NTV2_FBF_10BIT_YCBCR_420PL2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-P-10", NTV2_FBF_10BIT_YCBCR_422PL2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-P420-8", NTV2_FBF_8BIT_YCBCR_420PL2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "YUV-P-8", NTV2_FBF_8BIT_YCBCR_422PL2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Unknown", NTV2_FBF_INVALID);
	}
	return "";
}


string NTV2M31VideoPresetToString (const M31VideoPreset inValue, const bool inForRetailDisplay)
{
	if (inForRetailDisplay)
		return m31Presets [inValue];	//	frameBufferFormatString (inValue);
	
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X480_420_8_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X480_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X480_420_8_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X480_420_8_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X480_422_10_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X480_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X480_422_10_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X480_422_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X576_420_8_50i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X576_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X576_422_10_50i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_720X576_422_10_50p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_420_8_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_420_8_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_420_8_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_420_8_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_420_8_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_420_8_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_422_10_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_422_10_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_422_10_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_422_10_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_422_10_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_422_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1280X720_422_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_50i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_420_8_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_50i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_1920X1080_422_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_420_8_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_420_8_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_420_8_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_420_8_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_420_8_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_420_8_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_422_10_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_422_10_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_422_10_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_422_10_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_422_10_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_422_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_2048X1080_422_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_8_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_8_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_8_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_8_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_8_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_8_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_420_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_8_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_8_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_8_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_8_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_8_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_8_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_10_2398p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_10_24p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_10_25p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_10_2997p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_10_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_3840X2160_422_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_4096X2160_420_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_4096X2160_420_10_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_4096X2160_422_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_4096X2160_422_10_5994p_IF);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_FILE_4096X2160_422_10_60p_IF);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X480_420_8_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X480_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X480_420_8_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X480_420_8_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X480_422_10_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X480_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X480_422_10_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X480_422_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X576_420_8_50i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X576_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X576_422_10_50i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_720X576_422_10_50p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1280X720_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1280X720_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1280X720_420_8_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1280X720_422_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1280X720_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1280X720_422_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_8_50i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_8_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_8_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_8_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_10_50i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_10_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_10_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_420_10_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_422_10_5994i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_422_10_60i);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_1920X1080_422_10_60p);

		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_420_8_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_420_8_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_420_8_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_420_8_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_420_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_420_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_420_10_60p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_422_10_30p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_422_10_50p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_422_10_5994p);
		NTV2UTILS_ENUM_CASE_RETURN_STR(M31_VIF_3840X2160_422_10_60p);
		case M31_NUMVIDEOPRESETS:		return "";  //special case
	}
	return "";
}


string NTV2FrameGeometryToString (const NTV2FrameGeometry inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "1920x1080", NTV2_FG_1920x1080);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "1280x720", NTV2_FG_1280x720);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "720x486", NTV2_FG_720x486);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "720x576", NTV2_FG_720x576);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "1920x1114", NTV2_FG_1920x1114);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2048x1114", NTV2_FG_2048x1114);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "720x508", NTV2_FG_720x508);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "720x598", NTV2_FG_720x598);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "1920x1112", NTV2_FG_1920x1112);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "1280x740", NTV2_FG_1280x740);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2048x1080", NTV2_FG_2048x1080);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2048x1556", NTV2_FG_2048x1556);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2048x1588", NTV2_FG_2048x1588);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "2048x1112", NTV2_FG_2048x1112);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "720x514", NTV2_FG_720x514);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "720x612", NTV2_FG_720x612);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "3840x2160", NTV2_FG_4x1920x1080);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "4096x2160", NTV2_FG_4x2048x1080);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "7680x4320", NTV2_FG_4x3840x2160);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "8192x4320", NTV2_FG_4x4096x2160);
		case NTV2_FG_NUMFRAMEGEOMETRIES:			return "";  //special case
	}
	return "";
}


string NTV2FrameRateToString (const NTV2FrameRate inValue,	const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Unknown", NTV2_FRAMERATE_UNKNOWN);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "60.00", NTV2_FRAMERATE_6000);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "59.94", NTV2_FRAMERATE_5994);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "30.00", NTV2_FRAMERATE_3000);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "29.97", NTV2_FRAMERATE_2997);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "25.00",	NTV2_FRAMERATE_2500);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "24.00", NTV2_FRAMERATE_2400);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "23.98", NTV2_FRAMERATE_2398);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "50.00", NTV2_FRAMERATE_5000);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "48.00",	NTV2_FRAMERATE_4800);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "47.95",	NTV2_FRAMERATE_4795);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "120.00", NTV2_FRAMERATE_12000);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "119.88", NTV2_FRAMERATE_11988);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "15.00", NTV2_FRAMERATE_1500);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "14.98",	NTV2_FRAMERATE_1498);
#if !defined(NTV2_DEPRECATE_16_0)
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "19.00",	NTV2_FRAMERATE_1900);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "18.98",	NTV2_FRAMERATE_1898);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "18.00",	NTV2_FRAMERATE_1800);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "17.98", NTV2_FRAMERATE_1798);
#endif	//!defined(NTV2_DEPRECATE_16_0)
		case NTV2_NUM_FRAMERATES:					return "";  //special case
	}
	return "";
}


string NTV2InputSourceToString (const NTV2InputSource inValue,	const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Analog1", NTV2_INPUTSOURCE_ANALOG1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI1", NTV2_INPUTSOURCE_HDMI1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI2", NTV2_INPUTSOURCE_HDMI2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI3", NTV2_INPUTSOURCE_HDMI3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI4", NTV2_INPUTSOURCE_HDMI4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI1", NTV2_INPUTSOURCE_SDI1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI2", NTV2_INPUTSOURCE_SDI2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI3", NTV2_INPUTSOURCE_SDI3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI4", NTV2_INPUTSOURCE_SDI4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI5", NTV2_INPUTSOURCE_SDI5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI6", NTV2_INPUTSOURCE_SDI6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI7", NTV2_INPUTSOURCE_SDI7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI8", NTV2_INPUTSOURCE_SDI8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "", NTV2_INPUTSOURCE_INVALID);
	}
	return "";
}


string NTV2OutputDestinationToString (const NTV2OutputDestination inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Analog", NTV2_OUTPUTDESTINATION_ANALOG);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI", NTV2_OUTPUTDESTINATION_HDMI);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI1", NTV2_OUTPUTDESTINATION_SDI1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI2", NTV2_OUTPUTDESTINATION_SDI2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI3", NTV2_OUTPUTDESTINATION_SDI3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI4", NTV2_OUTPUTDESTINATION_SDI4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI5", NTV2_OUTPUTDESTINATION_SDI5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI6", NTV2_OUTPUTDESTINATION_SDI6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI7", NTV2_OUTPUTDESTINATION_SDI7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI8", NTV2_OUTPUTDESTINATION_SDI8);
#if !defined (NTV2_DEPRECATE)
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI 1.4", NTV2_OUTPUTDESTINATION_HDMI_14);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI1 DL", NTV2_OUTPUTDESTINATION_DUALLINK1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI2 DL", NTV2_OUTPUTDESTINATION_DUALLINK2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI3 DL", NTV2_OUTPUTDESTINATION_DUALLINK3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI4 DL", NTV2_OUTPUTDESTINATION_DUALLINK4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI5 DL", NTV2_OUTPUTDESTINATION_DUALLINK5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI6 DL", NTV2_OUTPUTDESTINATION_DUALLINK6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI7 DL", NTV2_OUTPUTDESTINATION_DUALLINK7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI8 DL", NTV2_OUTPUTDESTINATION_DUALLINK8);
#endif	//	!defined (NTV2_DEPRECATE)
		case NTV2_NUM_OUTPUTDESTINATIONS:			return "";  //special case
	}
	return "";
}


string NTV2ReferenceSourceToString (const NTV2ReferenceSource inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Reference In", NTV2_REFERENCE_EXTERNAL);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 1", NTV2_REFERENCE_INPUT1);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 2", NTV2_REFERENCE_INPUT2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Free Run", NTV2_REFERENCE_FREERUN);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Analog In", NTV2_REFERENCE_ANALOG_INPUT);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 1", NTV2_REFERENCE_HDMI_INPUT);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 3", NTV2_REFERENCE_INPUT3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 4", NTV2_REFERENCE_INPUT4);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 5", NTV2_REFERENCE_INPUT5);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 6", NTV2_REFERENCE_INPUT6);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 7", NTV2_REFERENCE_INPUT7);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SDI In 8", NTV2_REFERENCE_INPUT8);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SFP 1 PCR", NTV2_REFERENCE_SFP1_PCR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SFP 1 PTP", NTV2_REFERENCE_SFP1_PTP);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SFP 2 PCR", NTV2_REFERENCE_SFP2_PCR);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "SFP 2 PTP", NTV2_REFERENCE_SFP2_PTP);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 2", NTV2_REFERENCE_HDMI_INPUT2);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 3", NTV2_REFERENCE_HDMI_INPUT3);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "HDMI In 4", NTV2_REFERENCE_HDMI_INPUT4);
		case NTV2_NUM_REFERENCE_INPUTS:				return "";  //special case
	}
	return "";
}


string NTV2RegisterWriteModeToString (const NTV2RegisterWriteMode inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Sync To Field", NTV2_REGWRITE_SYNCTOFIELD);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Sync To Frame", NTV2_REGWRITE_SYNCTOFRAME);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inForRetailDisplay, "Immediate", NTV2_REGWRITE_IMMEDIATE);
		case NTV2_REGWRITE_SYNCTOFIELD_AFTER10LINES:	return "";  //special case
	}
	return "";
}


std::string NTV2InterruptEnumToString (const INTERRUPT_ENUMS inInterruptEnumValue)
{
	switch(inInterruptEnumValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_STR(eOutput1);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInterruptMask);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInput1);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInput2);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eAudio);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eAudioInWrap);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eAudioOutWrap);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eDMA1);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eDMA2);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eDMA3);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eDMA4);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eChangeEvent);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eGetIntCount);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eWrapRate);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eUart1Tx);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eUart1Rx);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eAuxVerticalInterrupt);
		NTV2UTILS_ENUM_CASE_RETURN_STR(ePushButtonChange);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eLowPower);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eDisplayFIFO);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eSATAChange);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eTemp1High);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eTemp2High);
		NTV2UTILS_ENUM_CASE_RETURN_STR(ePowerButtonChange);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInput3);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInput4);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eUart2Tx);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eUart2Rx);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eHDMIRxV2HotplugDetect);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInput5);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInput6);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInput7);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInput8);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eInterruptMask2);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eOutput2);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eOutput3);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eOutput4);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eOutput5);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eOutput6);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eOutput7);
		NTV2UTILS_ENUM_CASE_RETURN_STR(eOutput8);
		case eNumInterruptTypes:		return "";  //special case
	}
	return "";
}

std::string NTV2IpErrorEnumToString (const NTV2IpError inIpErrorEnumValue)
{
    switch (inIpErrorEnumValue)
    {
	case NTV2IpErrNone:                         return "";
	case NTV2IpErrInvalidChannel:               return "Invalid channel";
	case NTV2IpErrInvalidFormat:                return "Invalid format";
	case NTV2IpErrInvalidBitdepth:              return "Invalid bit depth";
	case NTV2IpErrInvalidUllHeight:             return "Invalid height in ull mode";
	case NTV2IpErrInvalidUllLevels:             return "Invalid number of levels in ull mode";
	case NTV2IpErrUllNotSupported:              return "Ull mode not supported";
	case NTV2IpErrNotReady:                     return "KonaIP card not ready";
	case NTV2IpErrSoftwareMismatch:             return "Host software does not match device firmware";
	case NTV2IpErrSFP1NotConfigured:            return "SFP 1 not configured";
	case NTV2IpErrSFP2NotConfigured:            return "SFP 2 not configured";
	case NTV2IpErrInvalidIGMPVersion:           return "Invalid IGMP version";
	case NTV2IpErrCannotGetMacAddress:          return "Failed to retrieve MAC address from ARP table";
	case NTV2IpErrNotSupported:					return "Not supported for by this firmware";
	case NTV2IpErrWriteSOMToMB:                 return "Could not write SOM to MB";
	case NTV2IpErrWriteSeqToMB:                 return "Could not write sequence number to MB";
	case NTV2IpErrWriteCountToMB:               return "Could not write count to MB";
	case NTV2IpErrTimeoutNoSOM:                 return "MB response timeout (no SOM)";
	case NTV2IpErrTimeoutNoSeq:                 return "MB response timeout (no sequence number)";
	case NTV2IpErrTimeoutNoBytecount:           return "MB response timeout (no bytecount)";
	case NTV2IpErrExceedsFifo:                  return "Response exceeds FIFO length";
	case NTV2IpErrNoResponseFromMB:             return "No response from MB";
	case NTV2IpErrAcquireMBTimeout:             return "AcquireMailBoxLock timeout";
	case NTV2IpErrInvalidMBResponse:            return "Invalid response from MB";
	case NTV2IpErrInvalidMBResponseSize:        return "Invalid response size from MB";
	case NTV2IpErrInvalidMBResponseNoMac:       return "MAC Address not found in response from MB";
	case NTV2IpErrMBStatusFail:                 return "MB Status Failure";
	case NTV2IpErrGrandMasterInfo:              return "PTP Grand Master Info not found";
	case NTV2IpErrSDPTooLong:                   return "SDP too long";
	case NTV2IpErrSDPNotFound:                  return "SDP not found";
	case NTV2IpErrSDPEmpty:                     return "SDP is empty";
	case NTV2IpErrSDPInvalid:                   return "SDP is not valid";
	case NTV2IpErrSDPURLInvalid:                return "Invalid SDP URL";
	case NTV2IpErrSDPNoVideo:                   return "SDP does not contain video";
	case NTV2IpErrSDPNoAudio:                   return "SDP does not contain audio";
	case NTV2IpErrSDPNoANC:                     return "SDP does not contain metadata";
	case NTV2IpErrSFPNotFound:                  return "SFP data not found";
	case NTV2IpErrInvalidConfig:                return "Invalid configuration";
	default:                                    return "Unknown IP error";
    }
}

ostream & operator << (ostream & inOutStream, const RP188_STRUCT & inObj)
{
	return inOutStream	<< "DBB=0x" << hex << setw (8) << setfill ('0') << inObj.DBB
						<< "|HI=0x" << hex << setw (8) << setfill ('0') << inObj.High
						<< "|LO=0x" << hex << setw (8) << setfill ('0') << inObj.Low
						<< dec;
}	//	RP188_STRUCT ostream operator


string NTV2GetBitfileName (const NTV2DeviceID inBoardID, const bool useOemNameOnWindows)
{
	bool useWindowsName = !useOemNameOnWindows;
#if defined (AJAMac) || defined (AJALinux)
	useWindowsName = false;
#endif
	switch (inBoardID)
	{
	case DEVICE_ID_NOTFOUND:					break;
	case DEVICE_ID_CORVID1:						return useWindowsName ? "corvid1_pcie.bit"          : "corvid1pcie.bit";
	case DEVICE_ID_CORVID22:					return useWindowsName ? "corvid22_pcie.bit"         : "Corvid22.bit";
	case DEVICE_ID_CORVID24:					return useWindowsName ? "corvid24_pcie.bit"         : "corvid24_quad.bit";
	case DEVICE_ID_CORVID3G:					return useWindowsName ? "corvid3G_pcie.bit"         : "corvid1_3gpcie.bit";
	case DEVICE_ID_CORVID44:					return useWindowsName ? "corvid44_pcie.bit"         : "corvid_44.bit";
	case DEVICE_ID_CORVID88:					return useWindowsName ? "corvid88_pcie.bit"         : "corvid_88.bit";
	case DEVICE_ID_CORVIDHEVC:					return useWindowsName ? "corvid_hevc.bit"           : "corvid_hevc.bit";
	case DEVICE_ID_IO4K:						return useWindowsName ? "io4k_pcie.bit"             : "IO_XT_4K.bit";
	case DEVICE_ID_IO4KUFC:						return useWindowsName ? "io4k_ufc_pcie.bit"         : "IO_XT_4K_UFC.bit";
	case DEVICE_ID_IOEXPRESS:					return useWindowsName ? "ioexpress_pcie.bit"        : "chekov_00_pcie.bit";
	case DEVICE_ID_IOXT:						return useWindowsName ? "ioxt_pcie.bit"             : "top_io_tx.bit";
	case DEVICE_ID_KONA3G:						return useWindowsName ? "kona3g_pcie.bit"           : "k3g_top.bit";
	case DEVICE_ID_KONA3GQUAD:					return useWindowsName ? "kona3g_quad_pcie.bit"      : "k3g_quad.bit";
	case DEVICE_ID_KONA4:						return useWindowsName ? "kona4_pcie.bit"            : "kona_4_quad.bit";
	case DEVICE_ID_KONA4UFC:					return useWindowsName ? "kona4_ufc_pcie.bit"        : "kona_4_ufc.bit";
	case DEVICE_ID_KONAIP_2022:					return useWindowsName ? "kip_s2022.mcs"             : "kip_s2022.mcs";
	case DEVICE_ID_KONAIP_4CH_2SFP:				return useWindowsName ? "s2022_56_2p2ch_rxtx.mcs"   : "s2022_56_2p2ch_rxtx.mcs";
	case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:		return useWindowsName ? "kip_j2k_1i1o.mcs"          : "kip_j2k_1i1o.mcs";
	case DEVICE_ID_KONAIP_2TX_1SFP_J2K:			return useWindowsName ? "kip_j2k_2o.mcs"            : "kip_j2k_2o.mcs";
	case DEVICE_ID_KONAIP_1RX_1TX_2110:			return useWindowsName ? "s2110_1rx_1tx.mcs"         : "s2110_1rx_1tx.mcs";
	case DEVICE_ID_KONALHEPLUS:					return useWindowsName ? "lheplus_pcie.bit"          : "lhe_12_pcie.bit";
	case DEVICE_ID_KONALHI:						return useWindowsName ? "lhi_pcie.bit"              : "top_pike.bit";
	case DEVICE_ID_TTAP:						return useWindowsName ? "ttap_pcie.bit"             : "t_tap_top.bit";
	case DEVICE_ID_IO4KPLUS:					return useWindowsName ? "io4kplus_pcie.bit"         : "io4kp.bit";
	case DEVICE_ID_IOIP_2022:					return useWindowsName ? "ioip_s2022.mcs"            : "ioip_s2022.mcs";
	case DEVICE_ID_IOIP_2110:					return useWindowsName ? "ioip_s2110.mcs"            : "ioip_s2110.mcs";
	case DEVICE_ID_IOIP_2110_RGB12:				return useWindowsName ? "ioip_s2110_rgb.mcs"		: "ioip_s2110_rgb.mcs";
	case DEVICE_ID_KONAIP_2110:					return useWindowsName ? "kip_s2110.mcs"             : "kip_s2110.mcs";
	case DEVICE_ID_KONAIP_2110_RGB12:			return useWindowsName ? "kip_s2110_rgb.mcs"			: "kip_s2110_rgb.mcs";
	case DEVICE_ID_KONAHDMI:					return useWindowsName ? "kona_hdmi_4rx.bit"         : "kona_hdmi_4rx.bit";
	case DEVICE_ID_KONA1:						return useWindowsName ? "kona1_pcie.bit"            : "kona1.bit";
	case DEVICE_ID_KONA5:						return "kona5_retail_tprom.bit";
	case DEVICE_ID_KONA5_2X4K:					return "kona5_2x4k_tprom.bit";
	case DEVICE_ID_KONA5_8KMK:					return "kona5_8k_mk_tprom.bit";
	case DEVICE_ID_KONA5_8K:					return "kona5_8k_tprom.bit";
	case DEVICE_ID_KONA5_3DLUT:					return "kona5_3d_lut_tprom.bit";
	case DEVICE_ID_KONA5_OE1:					return "kona5_oe_cfg1_tprom.bit";
	case DEVICE_ID_KONA5_OE2:					return "kona5_oe_cfg3_tprom.bit";
	case DEVICE_ID_KONA5_OE3:					return "kona5_oe_cfg3_tprom.bit";
	case DEVICE_ID_KONA5_OE4:					return "kona5_oe_cfg4_tprom.bit";
	case DEVICE_ID_KONA5_OE5:					return "kona5_oe_cfg5_tprom.bit";
	case DEVICE_ID_KONA5_OE6:					return "kona5_oe_cfg6_tprom.bit";
	case DEVICE_ID_KONA5_OE7:					return "kona5_oe_cfg7_tprom.bit";
	case DEVICE_ID_KONA5_OE8:					return "kona5_oe_cfg8_tprom.bit";
	case DEVICE_ID_KONA5_OE9:					return "kona5_oe_cfg9_tprom.bit";
	case DEVICE_ID_KONA5_OE10:					return "kona5_oe_cfg10_tprom.bit";
	case DEVICE_ID_KONA5_OE11:					return "kona5_oe_cfg11_tprom.bit";
	case DEVICE_ID_KONA5_OE12:					return "kona5_oe_cfg12_tprom.bit";
	case DEVICE_ID_CORVID44_8KMK:				return "c44_12g_8k_mk_tprom.bit";
	case DEVICE_ID_CORVID44_8K:					return "c44_12g_8k_tprom.bit";
	case DEVICE_ID_CORVID44_2X4K:				return "c44_12g_2x4k_tprom.bit";
	case DEVICE_ID_CORVID44_PLNR:				return "c44_12g_plnr_tprom.bit";
	case DEVICE_ID_TTAP_PRO:					return "t_tap_pro.bit";
	case DEVICE_ID_IOX3:						return "iox3.bit";
	default:									return "";
	}
	return "";
}	//	NTV2GetBitfileName


bool NTV2IsCompatibleBitfileName (const string & inBitfileName, const NTV2DeviceID inDeviceID)
{
	const string	deviceBitfileName	(::NTV2GetBitfileName (inDeviceID));
	if (inBitfileName == deviceBitfileName)
		return true;

	switch (inDeviceID)
	{
	case DEVICE_ID_KONA3GQUAD:	return ::NTV2GetBitfileName (DEVICE_ID_KONA3G) == inBitfileName;
	case DEVICE_ID_KONA3G:		return ::NTV2GetBitfileName (DEVICE_ID_KONA3GQUAD) == inBitfileName;

	case DEVICE_ID_KONA4:		return ::NTV2GetBitfileName (DEVICE_ID_KONA4UFC) == inBitfileName;
	case DEVICE_ID_KONA4UFC:	return ::NTV2GetBitfileName (DEVICE_ID_KONA4) == inBitfileName;

	case DEVICE_ID_IO4K:		return ::NTV2GetBitfileName (DEVICE_ID_IO4KUFC) == inBitfileName;
	case DEVICE_ID_IO4KUFC:		return ::NTV2GetBitfileName (DEVICE_ID_IO4K) == inBitfileName;

	default:					break;
	}
	return false;

}	//	IsCompatibleBitfileName


NTV2DeviceID NTV2GetDeviceIDFromBitfileName (const string & inBitfileName)
{
	typedef map <string, NTV2DeviceID>	BitfileName2DeviceID;
	static BitfileName2DeviceID			sBitfileName2DeviceID;
	if (sBitfileName2DeviceID.empty ())
	{
		static	NTV2DeviceID	sDeviceIDs [] =	{	DEVICE_ID_KONA3GQUAD,	DEVICE_ID_KONA3G,	DEVICE_ID_KONA4,		DEVICE_ID_KONA4UFC,	DEVICE_ID_KONALHI,
													DEVICE_ID_KONALHEPLUS,	DEVICE_ID_TTAP,		DEVICE_ID_CORVID1,		DEVICE_ID_CORVID22,	DEVICE_ID_CORVID24,
													DEVICE_ID_CORVID3G,		DEVICE_ID_IOXT,		DEVICE_ID_IOEXPRESS,	DEVICE_ID_IO4K,		DEVICE_ID_IO4KUFC,
                                                    DEVICE_ID_KONA1,		DEVICE_ID_KONAHDMI, DEVICE_ID_KONA5,        DEVICE_ID_KONA5_8KMK,DEVICE_ID_CORVID44_8KMK,
													DEVICE_ID_KONA5_8K,		DEVICE_ID_CORVID44_8K,	DEVICE_ID_TTAP_PRO,	DEVICE_ID_KONA5_2X4K,	DEVICE_ID_CORVID44_2X4K,
													DEVICE_ID_CORVID44_PLNR,DEVICE_ID_IOX3,
													DEVICE_ID_NOTFOUND };
		for (unsigned ndx (0);  ndx < sizeof (sDeviceIDs) / sizeof (NTV2DeviceID);  ndx++)
			sBitfileName2DeviceID [::NTV2GetBitfileName (sDeviceIDs [ndx])] = sDeviceIDs [ndx];
	}
	return sBitfileName2DeviceID [inBitfileName];
}


string NTV2GetFirmwareFolderPath (void)
{
	#if defined (AJAMac)
		return "/Library/Application Support/AJA/Firmware";
	#elif defined (MSWindows)
		HKEY	hKey		(AJA_NULL);
		DWORD	bufferSize	(1024);
		char *	lpData		(new char [bufferSize]);

		if (RegOpenKeyExA (HKEY_LOCAL_MACHINE, "Software\\AJA", NULL, KEY_READ, &hKey) == ERROR_SUCCESS
			&& RegQueryValueExA (hKey, "firmwarePath", NULL, NULL, (LPBYTE) lpData, &bufferSize) == ERROR_SUCCESS)
				return string (lpData);
		RegCloseKey (hKey);
		return "";
    #elif defined (AJALinux)
        return "/opt/aja/firmware";
	#else
		return "";
	#endif
}


NTV2DeviceIDSet NTV2GetSupportedDevices (const NTV2DeviceKinds inKinds)
{
	static const NTV2DeviceID	sValidDeviceIDs []	= {	DEVICE_ID_CORVID1,
														DEVICE_ID_CORVID22,
														DEVICE_ID_CORVID24,
														DEVICE_ID_CORVID3G,
														DEVICE_ID_CORVID44,
														DEVICE_ID_CORVID88,
 														DEVICE_ID_CORVIDHBR,
														DEVICE_ID_CORVIDHEVC,
														DEVICE_ID_IO4K,
														DEVICE_ID_IO4KUFC,
														DEVICE_ID_IO4KPLUS,
                                                        DEVICE_ID_IOIP_2022,
														DEVICE_ID_IOIP_2110,
														DEVICE_ID_IOIP_2110_RGB12,
														DEVICE_ID_IOEXPRESS,
														DEVICE_ID_IOXT,
														DEVICE_ID_KONA1,
														DEVICE_ID_KONA3G,
														DEVICE_ID_KONA3GQUAD,
														DEVICE_ID_KONA4,
														DEVICE_ID_KONA4UFC,
														DEVICE_ID_KONA5,
														DEVICE_ID_KONA5_2X4K,
														DEVICE_ID_KONA5_3DLUT,
														DEVICE_ID_KONA5_OE1,
														DEVICE_ID_KONA5_OE2,
														DEVICE_ID_KONA5_OE3,
														DEVICE_ID_KONA5_OE4,
														DEVICE_ID_KONA5_OE5,
														DEVICE_ID_KONA5_OE6,
														DEVICE_ID_KONA5_OE7,
														DEVICE_ID_KONA5_OE8,
														DEVICE_ID_KONA5_OE9,
														DEVICE_ID_KONA5_OE10,
														DEVICE_ID_KONA5_OE11,
														DEVICE_ID_KONA5_OE12,
                                                        DEVICE_ID_KONAHDMI,
														DEVICE_ID_KONAIP_2022,
														DEVICE_ID_KONAIP_4CH_2SFP,
														DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K,
														DEVICE_ID_KONAIP_2TX_1SFP_J2K,
														DEVICE_ID_KONAIP_1RX_1TX_2110,
														DEVICE_ID_KONAIP_2110,
														DEVICE_ID_KONAIP_2110_RGB12,
														DEVICE_ID_KONALHEPLUS,
														DEVICE_ID_KONALHI,
														DEVICE_ID_KONALHIDVI,
														DEVICE_ID_TTAP,
                                                        DEVICE_ID_KONA5_8KMK,
														DEVICE_ID_CORVID44_8KMK,
														DEVICE_ID_KONA5_8K,
														DEVICE_ID_CORVID44_8K,
														DEVICE_ID_CORVID44_2X4K,
														DEVICE_ID_CORVID44_PLNR,
														DEVICE_ID_TTAP_PRO,
														DEVICE_ID_IOX3,
                                                        DEVICE_ID_NOTFOUND	};
	NTV2DeviceIDSet	result;
	if (inKinds != NTV2_DEVICEKIND_NONE)
		for (unsigned ndx(0);  ndx < sizeof(sValidDeviceIDs) / sizeof(NTV2DeviceID);  ndx++)
		{
			const NTV2DeviceID	deviceID(sValidDeviceIDs[ndx]);
			if (deviceID != DEVICE_ID_NOTFOUND)
			{
				if (inKinds == NTV2_DEVICEKIND_ALL)
					{result.insert (deviceID);	continue;}
				//	else ...
				bool insertIt (false);
				if (insertIt)
					;
				else if (inKinds & NTV2_DEVICEKIND_INPUT  &&  ::NTV2DeviceCanDoCapture(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_OUTPUT  &&  ::NTV2DeviceCanDoPlayback(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_SDI  &&  (::NTV2DeviceGetNumVideoInputs(deviceID)+::NTV2DeviceGetNumVideoOutputs(deviceID)) > 0)
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_HDMI  &&  (::NTV2DeviceGetNumHDMIVideoInputs(deviceID)+::NTV2DeviceGetNumHDMIVideoOutputs(deviceID)) > 0)
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_ANALOG  &&  (::NTV2DeviceGetNumAnalogVideoInputs(deviceID)+::NTV2DeviceGetNumAnalogVideoOutputs(deviceID)) > 0)
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_SFP  &&  ::NTV2DeviceCanDoIP(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_EXTERNAL  &&  ::NTV2DeviceIsExternalToHost(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_4K  &&  ::NTV2DeviceCanDo4KVideo(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_12G  &&  ::NTV2DeviceCanDo12GSDI(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_6G  &&  ::NTV2DeviceCanDo12GSDI(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_CUSTOM_ANC  &&  ::NTV2DeviceCanDoCustomAnc(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_RELAYS  &&  ::NTV2DeviceHasSDIRelays(deviceID))
					insertIt = true;
				if (insertIt)
					result.insert (deviceID);
			}
		}	//	for each supported device
	return result;
}

ostream & operator << (std::ostream & inOutStr, const NTV2DeviceIDList & inList)
{
	for (NTV2DeviceIDListConstIter iter(inList.begin());  iter != inList.end ();  ++iter)
		inOutStr << (iter != inList.begin() ? ", " : "") << ::NTV2DeviceIDToString(*iter);
	return inOutStr;
}

ostream & operator << (ostream & inOutStr, const NTV2DeviceIDSet & inSet)
{
	for (NTV2DeviceIDSetConstIter iter(inSet.begin());  iter != inSet.end();  ++iter)
		inOutStr << (iter != inSet.begin() ? ", " : "") << ::NTV2DeviceIDToString(*iter);
	return inOutStr;
}


std::string NTV2GetVersionString (const bool inDetailed)
{
	ostringstream	oss;

	oss << AJA_NTV2_SDK_VERSION_MAJOR << "." << AJA_NTV2_SDK_VERSION_MINOR << "." << AJA_NTV2_SDK_VERSION_POINT;
	if (!string (AJA_NTV2_SDK_BUILD_TYPE).empty ())
		oss << " " << AJA_NTV2_SDK_BUILD_TYPE << AJA_NTV2_SDK_BUILD_NUMBER;
	#if defined (NTV2_DEPRECATE)
		if (inDetailed)
			oss << " (NTV2_DEPRECATE)";
	#endif
	if (inDetailed)
		oss	<< " built on " << AJA_NTV2_SDK_BUILD_DATETIME;
	return oss.str ();
}

UWord NTV2GetSDKVersionComponent (const int inVersionComponent)
{
	switch (inVersionComponent)
	{
		case 0:		return AJA_NTV2_SDK_VERSION_MAJOR;
		case 1:		return AJA_NTV2_SDK_VERSION_MINOR;
		case 2:		return AJA_NTV2_SDK_VERSION_POINT;
		case 3:		return AJA_NTV2_SDK_BUILD_NUMBER;
		default:	break;
	}
	return 0;
}


string NTV2RegisterNumberToString (const NTV2RegisterNumber inValue)
{
	return CNTV2RegisterExpert::GetDisplayName(inValue);
}


string AutoCircVidProcModeToString (const AutoCircVidProcMode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Mix", AUTOCIRCVIDPROCMODE_MIX);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "HWipe", AUTOCIRCVIDPROCMODE_HORZWIPE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "VWipe", AUTOCIRCVIDPROCMODE_VERTWIPE);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Key", AUTOCIRCVIDPROCMODE_KEY);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "n/a", AUTOCIRCVIDPROCMODE_INVALID);
	}
	return "??";
}


string NTV2ColorCorrectionModeToString (const NTV2ColorCorrectionMode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "Off", NTV2_CCMODE_OFF);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "RGB", NTV2_CCMODE_RGB);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "YCbCr", NTV2_CCMODE_YCbCr);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "3way", NTV2_CCMODE_3WAY);
		NTV2UTILS_ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inCompactDisplay, "n/a", NTV2_CCMODE_INVALID);
	}
	return "??";
}

bool convertHDRFloatToRegisterValues(const HDRFloatValues & inFloatValues, HDRRegValues & outRegisterValues)
{
	if ((inFloatValues.greenPrimaryX < 0 || inFloatValues.greenPrimaryX > float(1.0)) ||
		(inFloatValues.greenPrimaryY < 0 || inFloatValues.greenPrimaryY > float(1.0)) ||
		(inFloatValues.bluePrimaryX < 0 || inFloatValues.bluePrimaryX > float(1.0)) ||
		(inFloatValues.bluePrimaryY < 0 || inFloatValues.bluePrimaryY > float(1.0)) ||
		(inFloatValues.redPrimaryX < 0 || inFloatValues.redPrimaryX > float(1.0)) ||
		(inFloatValues.redPrimaryY < 0 || inFloatValues.redPrimaryY > float(1.0)) ||
		(inFloatValues.whitePointX < 0 || inFloatValues.whitePointX > float(1.0)) ||
        (inFloatValues.whitePointY < 0 || inFloatValues.whitePointY > float(1.0)) ||
        (inFloatValues.minMasteringLuminance < 0 || inFloatValues.minMasteringLuminance > float(6.5535)))
		return false;

	outRegisterValues.greenPrimaryX = static_cast<uint16_t>(inFloatValues.greenPrimaryX / float(0.00002));
	outRegisterValues.greenPrimaryY = static_cast<uint16_t>(inFloatValues.greenPrimaryY / float(0.00002));
	outRegisterValues.bluePrimaryX = static_cast<uint16_t>(inFloatValues.bluePrimaryX / float(0.00002));
	outRegisterValues.bluePrimaryY = static_cast<uint16_t>(inFloatValues.bluePrimaryY / float(0.00002));
	outRegisterValues.redPrimaryX = static_cast<uint16_t>(inFloatValues.redPrimaryX / float(0.00002));
	outRegisterValues.redPrimaryY = static_cast<uint16_t>(inFloatValues.redPrimaryY / float(0.00002));
	outRegisterValues.whitePointX = static_cast<uint16_t>(inFloatValues.whitePointX / float(0.00002));
	outRegisterValues.whitePointY = static_cast<uint16_t>(inFloatValues.whitePointY / float(0.00002));
    outRegisterValues.minMasteringLuminance = static_cast<uint16_t>(inFloatValues.minMasteringLuminance / float(0.0001));
    outRegisterValues.maxMasteringLuminance = inFloatValues.maxMasteringLuminance;
    outRegisterValues.maxContentLightLevel = inFloatValues.maxContentLightLevel;
    outRegisterValues.maxFrameAverageLightLevel = inFloatValues.maxFrameAverageLightLevel;
    outRegisterValues.electroOpticalTransferFunction = inFloatValues.electroOpticalTransferFunction;
    outRegisterValues.staticMetadataDescriptorID = inFloatValues.staticMetadataDescriptorID;
	return true;
}

bool convertHDRRegisterToFloatValues(const HDRRegValues & inRegisterValues, HDRFloatValues & outFloatValues)
{
	if ((inRegisterValues.greenPrimaryX > 0xC350) ||
		(inRegisterValues.greenPrimaryY > 0xC350) ||
		(inRegisterValues.bluePrimaryX > 0xC350) ||
		(inRegisterValues.bluePrimaryY > 0xC350) ||
		(inRegisterValues.redPrimaryX > 0xC350) ||
		(inRegisterValues.redPrimaryY > 0xC350) ||
		(inRegisterValues.whitePointX > 0xC350) ||
        (inRegisterValues.whitePointY > 0xC350))
		return false;
	outFloatValues.greenPrimaryX = static_cast<float>(inRegisterValues.greenPrimaryX * 0.00002);
	outFloatValues.greenPrimaryY = static_cast<float>(inRegisterValues.greenPrimaryY * 0.00002);
	outFloatValues.bluePrimaryX = static_cast<float>(inRegisterValues.bluePrimaryX * 0.00002);
	outFloatValues.bluePrimaryY = static_cast<float>(inRegisterValues.bluePrimaryY * 0.00002);
	outFloatValues.redPrimaryX = static_cast<float>(inRegisterValues.redPrimaryX * 0.00002);
	outFloatValues.redPrimaryY = static_cast<float>(inRegisterValues.redPrimaryY * 0.00002);
	outFloatValues.whitePointX = static_cast<float>(inRegisterValues.whitePointX * 0.00002);
	outFloatValues.whitePointY = static_cast<float>(inRegisterValues.whitePointY * 0.00002);
    outFloatValues.minMasteringLuminance = static_cast<float>(inRegisterValues.minMasteringLuminance * 0.0001);
    outFloatValues.maxMasteringLuminance = inRegisterValues.maxMasteringLuminance;
    outFloatValues.maxContentLightLevel = inRegisterValues.maxContentLightLevel;
    outFloatValues.maxFrameAverageLightLevel = inRegisterValues.maxFrameAverageLightLevel;
    outFloatValues.electroOpticalTransferFunction = inRegisterValues.electroOpticalTransferFunction;
    outFloatValues.staticMetadataDescriptorID = inRegisterValues.staticMetadataDescriptorID;
	return true;
}

void setHDRDefaultsForBT2020(HDRRegValues & outRegisterValues)
{
    outRegisterValues.greenPrimaryX = 0x2134;
    outRegisterValues.greenPrimaryY = 0x9BAA;
    outRegisterValues.bluePrimaryX = 0x1996;
    outRegisterValues.bluePrimaryY = 0x08FC;
    outRegisterValues.redPrimaryX = 0x8A48;
    outRegisterValues.redPrimaryY = 0x3908;
    outRegisterValues.whitePointX = 0x3D13;
    outRegisterValues.whitePointY = 0x4042;
    outRegisterValues.maxMasteringLuminance = 0x2710;
    outRegisterValues.minMasteringLuminance = 0x0032;
    outRegisterValues.maxContentLightLevel = 0;
    outRegisterValues.maxFrameAverageLightLevel = 0;
    outRegisterValues.electroOpticalTransferFunction = 0x02;
    outRegisterValues.staticMetadataDescriptorID = 0x00;
}

void setHDRDefaultsForDCIP3(HDRRegValues & outRegisterValues)
{
    outRegisterValues.greenPrimaryX = 0x33C2;
    outRegisterValues.greenPrimaryY = 0x86C4;
    outRegisterValues.bluePrimaryX = 0x1D4C;
    outRegisterValues.bluePrimaryY = 0x0BB8;
    outRegisterValues.redPrimaryX = 0x84D0;
    outRegisterValues.redPrimaryY = 0x3E80;
    outRegisterValues.whitePointX = 0x3D13;
    outRegisterValues.whitePointY = 0x4042;
    outRegisterValues.maxMasteringLuminance = 0x02E8;
    outRegisterValues.minMasteringLuminance = 0x0032;
    outRegisterValues.maxContentLightLevel = 0;
    outRegisterValues.maxFrameAverageLightLevel = 0;
    outRegisterValues.electroOpticalTransferFunction = 0x02;
    outRegisterValues.staticMetadataDescriptorID = 0x00;
}


ostream & operator << (ostream & inOutStr, const NTV2OutputCrosspointIDs & inList)
{
	inOutStr << "[";
	for (NTV2OutputCrosspointIDsConstIter it (inList.begin());  it != inList.end();  )
	{
		inOutStr << ::NTV2OutputCrosspointIDToString(*it);
		++it;
		if (it != inList.end())
			inOutStr << ",";
	}
	inOutStr << "]";
	return inOutStr;
}

/*
static ostream & operator << (ostream & inOutStr, const NTV2InputCrosspointIDs & inList)
{
	inOutStr << "[";
	for (NTV2InputCrosspointIDsConstIter it (inList.begin());  it != inList.end();  )
	{
		inOutStr << ::NTV2InputCrosspointIDToString(*it);
		++it;
		if (it != inList.end())
			inOutStr << ",";
	}
	inOutStr << "]";
	return inOutStr;
}
*/

ostream & operator << (ostream & inOutStream, const NTV2StringList & inData)
{
	for (NTV2StringListConstIter it(inData.begin());  it != inData.end();  )
	{
		inOutStream	<< *it;
		if (++it != inData.end())
			inOutStream << ", ";
	}
	return inOutStream;
}

ostream & operator << (ostream & inOutStream, const NTV2StringSet & inData)
{
	for (NTV2StringSetConstIter it(inData.begin());  it != inData.end();  )
	{
		inOutStream	<< *it;
		if (++it != inData.end())
			inOutStream << ", ";
	}
	return inOutStream;
}


NTV2RegisterReads FromRegNumSet (const NTV2RegNumSet & inRegNumSet)
{
	NTV2RegisterReads	result;
	for (NTV2RegNumSetConstIter it (inRegNumSet.begin());  it != inRegNumSet.end();  ++it)
		result.push_back (NTV2RegInfo (*it));
	return result;
}

NTV2RegNumSet ToRegNumSet (const NTV2RegisterReads & inRegReads)
{
	NTV2RegNumSet	result;
	for (NTV2RegisterReadsConstIter it (inRegReads.begin());  it != inRegReads.end();  ++it)
		result.insert (it->registerNumber);
	return result;
}

bool GetRegNumChanges (const NTV2RegNumSet & inBefore, const NTV2RegNumSet & inAfter, NTV2RegNumSet & outGone, NTV2RegNumSet & outSame, NTV2RegNumSet & outNew)
{
	outGone.clear();  outSame.clear();  outNew.clear();
	set_difference (inBefore.begin(), inBefore.end(), inAfter.begin(), inAfter.end(),  std::inserter(outGone, outGone.begin()));
	set_difference (inAfter.begin(), inAfter.end(), inBefore.begin(), inBefore.end(),  std::inserter(outNew, outNew.begin()));
	set_intersection (inBefore.begin(), inBefore.end(),  inAfter.begin(), inAfter.end(),  std::inserter(outSame, outSame.begin()));
	return true;
}

bool GetChangedRegisters (const NTV2RegisterReads & inBefore, const NTV2RegisterReads & inAfter, NTV2RegNumSet & outChanged)
{
	outChanged.clear();
	if (&inBefore == &inAfter)
		return false;	//	Same vector, identical!
	if (inBefore.size() != inAfter.size())
	{	//	Only check common reg nums...
		NTV2RegNumSet before(::ToRegNumSet(inBefore)), after(::ToRegNumSet(inAfter)), commonRegNums;
		set_intersection (before.begin(), before.end(),  after.begin(), after.end(),
							std::inserter(commonRegNums, commonRegNums.begin()));
		for (NTV2RegNumSetConstIter it(commonRegNums.begin());  it != commonRegNums.end();  ++it)
		{
			NTV2RegisterReadsConstIter	beforeIt(::FindFirstMatchingRegisterNumber(*it, inBefore));
			NTV2RegisterReadsConstIter	afterIt(::FindFirstMatchingRegisterNumber(*it, inAfter));
			if (beforeIt != inBefore.end()  &&  afterIt != inAfter.end()  &&  beforeIt->registerValue != afterIt->registerValue)
				outChanged.insert(*it);
		}
	}
	else if (inBefore.at(0).registerNumber == inAfter.at(0).registerNumber
		&& inBefore.at(inBefore.size()-1).registerNumber == inAfter.at(inAfter.size()-1).registerNumber)
	{	//	Assume identical reg num layout
		for (size_t ndx(0);  ndx < inBefore.size();  ndx++)
			if (inBefore[ndx].registerValue != inAfter[ndx].registerValue)
				outChanged.insert(inBefore[ndx].registerNumber);
	}
	else for (size_t ndx(0);  ndx < inBefore.size();  ndx++)
	{
		const NTV2RegInfo & beforeInfo(inBefore.at(ndx));
		const NTV2RegInfo & afterInfo(inAfter.at(ndx));
		if (beforeInfo.registerNumber == afterInfo.registerNumber)
		{
			if (beforeInfo.registerValue != afterInfo.registerValue)
				outChanged.insert(beforeInfo.registerNumber);
		}
		else
		{
			NTV2RegisterReadsConstIter	it(::FindFirstMatchingRegisterNumber(beforeInfo.registerNumber, inAfter));
			if (it != inAfter.end())
				if (beforeInfo.registerValue != it->registerValue)
					outChanged.insert(beforeInfo.registerNumber);
		}
	}
	return !outChanged.empty();
}


string PercentEncode (const string & inStr)
{	ostringstream oss;
	for (size_t ndx(0);  ndx < inStr.size();  ndx++)
	{
		const char chr(inStr.at(size_t(ndx)));
		if (::isalnum(chr)  ||  chr == '-'  ||  chr == '_'  ||  chr == '.'  ||  chr == '~')
			oss << chr;
		else
			oss << "%" << HEX0N(unsigned(chr),2);
	}
	return oss.str();
}

string PercentDecode (const string & inStr)
{	ostringstream oss;
	unsigned hexNum(0), state(0);	//	0=unreserved expected,   1=1st hex digit expected,  2=2nd hex digit expected
	for (size_t ndx(0);  ndx < inStr.size();  ndx++)
	{
		const char chr(inStr.at(size_t(ndx)));
		switch (state)
		{
			case 0:
				if (::isalnum(chr)  ||  chr == '-'  ||  chr == '_'  ||  chr == '.'  ||  chr == '~')
					oss << chr;
				if (chr == '%')
					{state++;  break;}
				break;
			case 1:
				if (chr >= 'A'  &&  chr <= 'F')
					hexNum = unsigned(chr + 10 - 'A') << 4;
				else if (chr >= 'a'  && chr <= 'f')
					hexNum = unsigned(chr + 10 - 'a') << 4;
				else if (chr >= '0'  &&  chr <= '9')
					hexNum = unsigned(chr - '0') << 4;
				else
					hexNum = 0;
				state++;
				break;
			case 2:
				if (chr >= 'A'  &&  chr <= 'F')
					hexNum += unsigned(chr + 10 - 'A');
				else if (chr >= 'a'  && chr <= 'f')
					hexNum += unsigned(chr + 10 - 'a');
				else if (chr >= '0'  &&  chr <= '9')
					hexNum += unsigned(chr - '0');
				else
					;
				oss << char(hexNum);
				hexNum = 0;
				state = 0;
				break;
			default:	NTV2_ASSERT(false);	break;
		}
	}
	return oss.str();
}
