/* SPDX-License-Identifier: MIT */
/**
	@file		videoutilities.cpp
	@brief		Declares the ajabase library's video utility functions.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "common.h"
#include "videoutilities.h"
#include <string.h>


inline int FixedTrunc(int inFix)
{
  return (inFix>>16);
}

#define _ENDIAN_SWAP32(_data_) (((_data_<<24)&0xff000000)|((_data_<<8)&0x00ff0000)|((_data_>>8)&0x0000ff00)|((_data_>>24)&0x000000ff))
inline AJA_RGBAlphaPixel CubicInterPolate( AJA_RGBAlphaPixel *Input, int32_t Index);
inline int16_t CubicInterPolateWord( int16_t *Input, int32_t Index);
inline int16_t CubicInterPolateAudioWord( int16_t *Input, int32_t Index);

// Cubic Coefficient Table for resampling
uint32_t CubicCoef[129] = {
	0x00000000  , /* 0 */
	0xFFFFFFE1  , /* 1 */
	0xFFFFFF88  , /* 2 */
	0xFFFFFEFB  , /* 3 */
	0xFFFFFE40  , /* 4 */
	0xFFFFFD5D  , /* 5 */
	0xFFFFFC58  , /* 6 */
	0xFFFFFB37  , /* 7 */
	0xFFFFFA00  , /* 8 */
	0xFFFFF8B9  , /* 9 */
	0xFFFFF768  , /* 10 */
	0xFFFFF613  , /* 11 */
	0xFFFFF4C0  , /* 12 */
	0xFFFFF375  , /* 13 */
	0xFFFFF238  , /* 14 */
	0xFFFFF10F  , /* 15 */
	0xFFFFF000  , /* 16 */
	0xFFFFEF11  , /* 17 */
	0xFFFFEE48  , /* 18 */
	0xFFFFEDAB  , /* 19 */
	0xFFFFED40  , /* 20 */
	0xFFFFED0D  , /* 21 */
	0xFFFFED18  , /* 22 */
	0xFFFFED67  , /* 23 */
	0xFFFFEE00  , /* 24 */
	0xFFFFEEE9  , /* 25 */
	0xFFFFF028  , /* 26 */
	0xFFFFF1C3  , /* 27 */
	0xFFFFF3C0  , /* 28 */
	0xFFFFF625  , /* 29 */
	0xFFFFF8F8  , /* 30 */
	0xFFFFFC3F  , /* 31 */
	0x00000000  , /* 32 */
	0x0000047D  , /* 33 */
	0x000009E8  , /* 34 */
	0x0000102F  , /* 35 */
	0x00001740  , /* 36 */
	0x00001F09  , /* 37 */
	0x00002778  , /* 38 */
	0x0000307B  , /* 39 */
	0x00003A00  , /* 40 */
	0x000043F5  , /* 41 */
	0x00004E48  , /* 42 */
	0x000058E7  , /* 43 */
	0x000063C0  , /* 44 */
	0x00006EC1  , /* 45 */
	0x000079D8  , /* 46 */
	0x000084F3  , /* 47 */
	0x00009000  , /* 48 */
	0x00009AED  , /* 49 */
	0x0000A5A8  , /* 50 */
	0x0000B01F  , /* 51 */
	0x0000BA40  , /* 52 */
	0x0000C3F9  , /* 53 */
	0x0000CD38  , /* 54 */
	0x0000D5EB  , /* 55 */
	0x0000DE00  , /* 56 */
	0x0000E565  , /* 57 */
	0x0000EC08  , /* 58 */
	0x0000F1D7  , /* 59 */
	0x0000F6C0  , /* 60 */
	0x0000FAB1  , /* 61 */
	0x0000FD98  , /* 62 */
	0x0000FF63  , /* 63 */
	0x00010000  , /* 64 */
	0x0000FF63  , /* 65 */
	0x0000FD98  , /* 66 */
	0x0000FAB1  , /* 67 */
	0x0000F6C0  , /* 68 */
	0x0000F1D7  , /* 69 */
	0x0000EC08  , /* 70 */
	0x0000E565  , /* 71 */
	0x0000DE00  , /* 72 */
	0x0000D5EB  , /* 73 */
	0x0000CD38  , /* 74 */
	0x0000C3F9  , /* 75 */
	0x0000BA40  , /* 76 */
	0x0000B01F  , /* 77 */
	0x0000A5A8  , /* 78 */
	0x00009AED  , /* 79 */
	0x00009000  , /* 80 */
	0x000084F3  , /* 81 */
	0x000079D8  , /* 82 */
	0x00006EC1  , /* 83 */
	0x000063C0  , /* 84 */
	0x000058E7  , /* 85 */
	0x00004E48  , /* 86 */
	0x000043F5  , /* 87 */
	0x00003A00  , /* 88 */
	0x0000307B  , /* 89 */
	0x00002778  , /* 90 */
	0x00001F09  , /* 91 */
	0x00001740  , /* 92 */
	0x0000102F  , /* 93 */
	0x000009E8  , /* 94 */
	0x0000047D  , /* 95 */
	0x00000000  , /* 96 */
	0xFFFFFC3F  , /* 97 */
	0xFFFFF8F8  , /* 98 */
	0xFFFFF625  , /* 99 */
	0xFFFFF3C0  , /* 100 */
	0xFFFFF1C3  , /* 101 */
	0xFFFFF028  , /* 102 */
	0xFFFFEEE9  , /* 103 */
	0xFFFFEE00  , /* 104 */
	0xFFFFED67  , /* 105 */
	0xFFFFED18  , /* 106 */
	0xFFFFED0D  , /* 107 */
	0xFFFFED40  , /* 108 */
	0xFFFFEDAB  , /* 109 */
	0xFFFFEE48  , /* 110 */
	0xFFFFEF11  , /* 111 */
	0xFFFFF000  , /* 112 */
	0xFFFFF10F  , /* 113 */
	0xFFFFF238  , /* 114 */
	0xFFFFF375  , /* 115 */
	0xFFFFF4C0  , /* 116 */
	0xFFFFF613  , /* 117 */
	0xFFFFF768  , /* 118 */
	0xFFFFF8B9  , /* 119 */
	0xFFFFFA00  , /* 120 */
	0xFFFFFB37  , /* 121 */
	0xFFFFFC58  , /* 122 */
	0xFFFFFD5D  , /* 123 */
	0xFFFFFE40  , /* 124 */
	0xFFFFFEFB  , /* 125 */
	0xFFFFFF88  , /* 126 */
	0xFFFFFFE1  , /* 127 */
	0x00000000  , /* 128 */
};



void createVideoFrame( uint32_t *buffer , uint64_t frameNumber, 
					   AJA_PixelFormat pixFmt, uint32_t lines, uint32_t pixels, uint32_t linepitch, 
					   uint16_t y, uint16_t cb, uint16_t cr )
{
    AJA_UNUSED(buffer);
    AJA_UNUSED(frameNumber);
    AJA_UNUSED(pixFmt);
    AJA_UNUSED(lines);
    AJA_UNUSED(pixels);
    AJA_UNUSED(linepitch);
    AJA_UNUSED(y);
    AJA_UNUSED(cb);
    AJA_UNUSED(cr);
}


uint32_t AJA_CalcRowBytesForFormat(AJA_PixelFormat format, uint32_t width)
{
	int rowBytes = 0;

	switch (format)
	{
	case AJA_PixelFormat_YCbCr8:
	case AJA_PixelFormat_YUY28:	
		rowBytes = width * 2;
		break;

	case AJA_PixelFormat_YCbCr10:
	case AJA_PixelFormat_YCbCr_DPX:
		rowBytes = (( width % 48 == 0 ) ? width : (((width / 48 ) + 1) * 48)) * 8 / 3;
		break;

	case AJA_PixelFormat_RGB8_PACK:
	case AJA_PixelFormat_BGR8_PACK:
		rowBytes = width * 3;
		break;

	case AJA_PixelFormat_ARGB8:	
	case AJA_PixelFormat_RGBA8:
	case AJA_PixelFormat_ABGR8:
	case AJA_PixelFormat_RGB10:
	case AJA_PixelFormat_RGB10_PACK:
	case AJA_PixelFormat_RGB_DPX:
	case AJA_PixelFormat_RGB_DPX_LE:
		rowBytes = width * 4;
		break;

	case AJA_PixelFormat_RGB16:
		// Note: not technically true but for now using this for bayer test patterns.
	case AJA_PixelFormat_BAYER10_DPX_LJ:	
	case AJA_PixelFormat_BAYER12_DPX_LJ:	
	case AJA_PixelFormat_BAYER10_HS:
	case AJA_PixelFormat_BAYER12_HS:
		rowBytes = width * 6;
		break;

	case AJA_PixelFormat_RGB12:
		// 36 bits/pixel packed.
		rowBytes = width * 36 / 8;
		break;
		
	case AJA_PixelFormat_RAW10:
	case AJA_PixelFormat_RAW10_HS:
		// 10 bits/pixel raw packed
		rowBytes = width * 10 / 8;
		break;

	case AJA_PixelFormat_YCBCR10_420PL:
	case AJA_PixelFormat_YCBCR10_422PL:
		// 10 bits/pixel packed planer
		rowBytes = width * 20 / 16;
		break;
	
	// planar format are average-effective
	case AJA_PixelFormat_YCBCR8_420PL:
	case AJA_PixelFormat_YCBCR8_420PL3:
		rowBytes = width * 3 / 2;	// average-effective
		break;
	case AJA_PixelFormat_YCBCR8_422PL:
	case AJA_PixelFormat_YCBCR8_422PL3:
		rowBytes = width * 2;		// average-effective
		break;
	case AJA_PixelFormat_YCBCR10_420PL3LE:
		rowBytes = width * 3;		// average-effective
		break;
	case AJA_PixelFormat_YCBCR10_422PL3LE:
		rowBytes = width * 4;		// average-effective
		break;
		
	default:
		// TO DO.....add more
		break;

	}
 
	return rowBytes;
}

// UnPack10BitYCbCrBuffer
// UnPack 10 Bit YCbCr Data to 16 bit Word per component
void AJA_UnPack10BitYCbCrBuffer( uint32_t* packedBuffer, uint16_t* ycbcrBuffer, uint32_t numPixels )
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
void AJA_PackTo10BitYCbCrBuffer( uint16_t *ycbcrBuffer, uint32_t *packedBuffer,uint32_t numPixels )
{
	for ( uint32_t inputCount=0, outputCount=0; 
		inputCount < (numPixels*2);
		outputCount += 4,inputCount += 12 )
	{
		packedBuffer[outputCount]   = ycbcrBuffer[inputCount+0] + (ycbcrBuffer[inputCount+1]<<10) + (ycbcrBuffer[inputCount+2]<<20);
		packedBuffer[outputCount+1] = ycbcrBuffer[inputCount+3] + (ycbcrBuffer[inputCount+4]<<10) + (ycbcrBuffer[inputCount+5]<<20);
		packedBuffer[outputCount+2] = ycbcrBuffer[inputCount+6] + (ycbcrBuffer[inputCount+7]<<10) + (ycbcrBuffer[inputCount+8]<<20);
		packedBuffer[outputCount+3] = ycbcrBuffer[inputCount+9] + (ycbcrBuffer[inputCount+10]<<10) + (ycbcrBuffer[inputCount+11]<<20);
	}
}

// AJA_PackTo10BitYCbCrDPXBuffer
// Pack 16 bit Word per component to 10 Bit YCbCr Data 
void AJA_PackTo10BitYCbCrDPXBuffer( uint16_t *ycbcrBuffer, uint32_t *packedBuffer,uint32_t numPixels , bool bigEndian)
{
	for ( uint32_t inputCount=0, outputCount=0; 
		inputCount < (numPixels*2);
		outputCount += 4,inputCount += 12 )
	{
		if ( bigEndian )
		{
			packedBuffer[outputCount]   = AJA_ENDIAN_SWAP32(((ycbcrBuffer[inputCount+0]<<20) + (ycbcrBuffer[inputCount+1]<<10) + (ycbcrBuffer[inputCount+2]))<<2);
			packedBuffer[outputCount+1] = AJA_ENDIAN_SWAP32(((ycbcrBuffer[inputCount+3]<<20) + (ycbcrBuffer[inputCount+4]<<10) + (ycbcrBuffer[inputCount+5]))<<2);
			packedBuffer[outputCount+2] = AJA_ENDIAN_SWAP32(((ycbcrBuffer[inputCount+6]<<20) + (ycbcrBuffer[inputCount+7]<<10) + (ycbcrBuffer[inputCount+8]))<<2);
			packedBuffer[outputCount+3] = AJA_ENDIAN_SWAP32(((ycbcrBuffer[inputCount+9]<<20) + (ycbcrBuffer[inputCount+10]<<10) + (ycbcrBuffer[inputCount+11]))<<2);
		}
		else
		{
			packedBuffer[outputCount]   = (((ycbcrBuffer[inputCount+0]<<20) + (ycbcrBuffer[inputCount+1]<<10) + (ycbcrBuffer[inputCount+2]))<<2);
			packedBuffer[outputCount+1] = (((ycbcrBuffer[inputCount+3]<<20) + (ycbcrBuffer[inputCount+4]<<10) + (ycbcrBuffer[inputCount+5]))<<2);
			packedBuffer[outputCount+2] = (((ycbcrBuffer[inputCount+6]<<20) + (ycbcrBuffer[inputCount+7]<<10) + (ycbcrBuffer[inputCount+8]))<<2);
			packedBuffer[outputCount+3] = (((ycbcrBuffer[inputCount+9]<<20) + (ycbcrBuffer[inputCount+10]<<10) + (ycbcrBuffer[inputCount+11]))<<2);
		}
	}
}

// Pack 10 Bit RGBA to 10 Bit RGB Format for our board
void AJA_PackRGB10BitFor10BitRGB(AJA_RGBAlpha10BitPixel* rgba10BitBuffer,uint32_t numPixels)
{
	uint32_t* outputBuffer = (uint32_t*)rgba10BitBuffer;
	for ( uint32_t pixel=0;pixel<numPixels;pixel++)
	{
		uint16_t Red = rgba10BitBuffer[pixel].Red;
		uint16_t Green = rgba10BitBuffer[pixel].Green;
		uint16_t Blue = rgba10BitBuffer[pixel].Blue;
		outputBuffer[pixel] = (Blue<<20) + (Green<<10) + Red;
	}
}

// Pack 10 Bit RGBA to NTV2_FBF_10BIT_RGB_PACKED Format for our board
void AJA_PackRGB10BitFor10BitRGBPacked(AJA_RGBAlpha10BitPixel* rgba10BitBuffer,uint32_t numPixels)
{
	uint32_t* outputBuffer = (uint32_t*)rgba10BitBuffer;
	for ( uint32_t pixel=0;pixel<numPixels;pixel++)
	{
		uint16_t Red = rgba10BitBuffer[pixel].Red;
		uint16_t Green = rgba10BitBuffer[pixel].Green;
		uint16_t Blue = rgba10BitBuffer[pixel].Blue;
		uint32_t value = (((Red>>2)&0xFF)<<16) + (((Green>>2)&0xFF)<<8) + ((Blue>>2)&0xFF);
		value |= ((Red&0x3)<<28) + ((Green&0x3)<<26) + ((Blue&0x3)<<24);

		outputBuffer[pixel] = value;
	}
}

// Pack 10 Bit RGBA to 10 Bit DPX Format for our board
void AJA_PackRGB10BitFor10BitDPX(AJA_RGBAlpha10BitPixel* rgba10BitBuffer,uint32_t numPixels, bool bigEndian)
{
	uint32_t* outputBuffer = (uint32_t*)rgba10BitBuffer;
	for ( uint32_t pixel=0;pixel<numPixels;pixel++)
	{
		uint16_t Red   = rgba10BitBuffer[pixel].Red;
		uint16_t Green = rgba10BitBuffer[pixel].Green;
		uint16_t Blue  = rgba10BitBuffer[pixel].Blue;
		uint32_t value = (Red<<22) + (Green<<12) + (Blue<<2);
		if ( bigEndian)
			outputBuffer[pixel] = ((value&0xFF)<<24) + (((value>>8)&0xFF)<<16) + (((value>>16)&0xFF)<<8) + ((value>>24)&0xFF);
		else
			outputBuffer[pixel] =value;
	}
}

// UnPack 10 Bit DPX Format linebuffer to RGBAlpha10BitPixel linebuffer.
void AJA_UnPack10BitDPXtoRGBAlpha10BitPixel(AJA_RGBAlpha10BitPixel* rgba10BitBuffer,uint32_t* DPXLinebuffer ,uint32_t numPixels, bool bigEndian)
{
	for ( uint32_t pixel=0;pixel<numPixels;pixel++)
	{
		uint32_t value = DPXLinebuffer[pixel];
		if ( bigEndian)
		{
			value = _ENDIAN_SWAP32(value);
		}
		else
		{
			rgba10BitBuffer[pixel].Red = (value>>22)&0x3FF;
			rgba10BitBuffer[pixel].Green = (value>>12)&0x3FF;
			rgba10BitBuffer[pixel].Blue = (value>>2)&0x3FF;

		}
	}
}

// UnPack 10 Bit DPX Format linebuffer to AJA_RGBAlphaPixel linebuffer.
void AJA_UnPack10BitDPXtoRGBAlphaBitPixel(uint8_t* rgbaBuffer,uint32_t* DPXLinebuffer ,uint32_t numPixels, bool bigEndian)
{
	for ( uint32_t pixel=0;pixel<numPixels;pixel++)
	{
		uint32_t value = DPXLinebuffer[pixel];
		uint8_t R,G,B;
		if ( bigEndian)
		{
			B = ((value & 0xF0000000)>>28) + ((value&0x000F0000)>>12); 
			G = ((value & 0x3F00)>>6) + ((value & 0xC00000)>>22);
			R = (value&0xFF);
		}
		else
		{
			B =  (value>>22)&0x3FF;
			G =  (value>>12)&0x3FF;
			R =   (value>>2)&0x3FF;
		}
		*rgbaBuffer++ = B;
		*rgbaBuffer++ = G;
		*rgbaBuffer++ = R;
		*rgbaBuffer++ = 0xFF;

	}
}
void AJA_MakeUnPacked10BitYCbCrBuffer( uint16_t* buffer, uint16_t Y , uint16_t Cb , uint16_t Cr,uint32_t numPixels )
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

void AJA_MakeUnPacked8BitYCbCrBuffer( uint8_t* buffer, uint8_t Y , uint8_t Cb , uint8_t Cr,uint32_t numPixels )
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

// ConvertLineto8BitYCbCr
// 10 Bit YCbCr to 8 Bit YCbCr
void AJA_ConvertLineto8BitYCbCr(uint16_t * ycbcr10BitBuffer, uint8_t * ycbcr8BitBuffer,	uint32_t numPixels)
{
	for ( uint32_t pixel=0;pixel<numPixels*2;pixel++)
	{
		ycbcr8BitBuffer[pixel] = ycbcr10BitBuffer[pixel]>>2;
	}

}

// ConvertLineto8BitYCbCr
// 10 Bit YCbCr to 8 Bit YCbCr
void AJA_ConvertLineToYCbCr422(AJA_RGBAlphaPixel * RGBLine, 
						   uint16_t* YCbCrLine, 
						   int32_t numPixels ,
						   int32_t startPixel,
						   bool fUseSDMatrix)
{
	AJA_YCbCr10BitPixel YCbCr;
	uint16_t *pYCbCr = &YCbCrLine[(startPixel&~1)*2];   // startPixel needs to be even

	AJA_ConvertRGBAlphatoYCbCr convertRGBAlphatoYCbCr;
	if ( fUseSDMatrix )
		convertRGBAlphatoYCbCr = AJA_HDConvertRGBAlphatoYCbCr;
	else
		convertRGBAlphatoYCbCr = AJA_SDConvertRGBAlphatoYCbCr;



	for ( int32_t pixel = 0; pixel < numPixels; pixel++ )
	{
		convertRGBAlphatoYCbCr(&RGBLine[pixel],&YCbCr);

		if ( pixel & 0x1 )
		{
			// Just Y
			*pYCbCr++ = YCbCr.y;
		}
		else
		{
			*pYCbCr++ = YCbCr.cb;
			*pYCbCr++ = YCbCr.y;
			*pYCbCr++ = YCbCr.cr;

		}

	}

}

// ConvertLineto8BitYCbCr
// 10 Bit RGB to 10 Bit YCbCr
void AJA_ConvertRGBAlpha10LineToYCbCr422(AJA_RGBAlpha10BitPixel * RGBLine, 
							   uint16_t* YCbCrLine, 
							   int32_t numPixels ,
							   int32_t startPixel,
							   bool fUseRGBFullRange)
{
	AJA_YCbCr10BitPixel YCbCr;
	uint16_t *pYCbCr = &YCbCrLine[(startPixel&~1)*2];   // startPixel needs to be even

	//AJA_ConvertRGBAlphatoYCbCr convertRGBAlphatoYCbCr;
	//if ( fUseSDMatrix )
	//convertRGBAlphatoYCbCr = AJA_HDConvertRGBAlpha10toYCbCr;



	for ( int32_t pixel = 0; pixel < numPixels; pixel++ )
	{
		AJA_HDConvertRGBAlpha10toYCbCr(&RGBLine[pixel],&YCbCr,fUseRGBFullRange);

		if ( pixel & 0x1 )
		{
			// Just Y
			*pYCbCr++ = YCbCr.y;
		}
		else
		{
			*pYCbCr++ = YCbCr.cb;
			*pYCbCr++ = YCbCr.y;
			*pYCbCr++ = YCbCr.cr;

		}

	}

}
// ConvertLineto10BitRGB
// 10 Bit YCbCr and 10 Bit RGB Version
void AJA_ConvertLineto10BitRGB(uint16_t * ycbcrBuffer, AJA_RGBAlpha10BitPixel * rgbaBuffer,uint32_t numPixels,bool fUseSDMatrix)
{
	AJA_YCbCr10BitAlphaPixel ycbcrPixel;
	uint16_t Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to AJA_RGBAlphaPixels.
	// 2 AJA_RGBAlphaPixels at a time.
	Cb1 = *ycbcrBuffer++;
	Y1 = *ycbcrBuffer++;
	Cr1 = *ycbcrBuffer++;
	for ( uint32_t count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (uint16_t)Cb1;
		ycbcrPixel.y = (uint16_t)Y1; 
		ycbcrPixel.cr = (uint16_t)Cr1; 
		ycbcrPixel.Alpha = CCIR601_10BIT_WHITE;

		if(fUseSDMatrix) {
			AJA_SDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,&rgbaBuffer[count]);
		} else {
			AJA_HDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,&rgbaBuffer[count]);
		}
		// Read lone midde Y;
		ycbcrPixel.y = *ycbcrBuffer++;

		// Read Next full bandwidth sample
		// unless we are at the end of a line
		if ( (count + 2 ) >= numPixels )
		{
			Cb2 = (uint16_t)Cb1;
			Y2 = (uint16_t)Y1;
			Cr2 = (uint16_t)Cr1;
		}
		else
		{
			Cb2 = *ycbcrBuffer++;
			Y2 = *ycbcrBuffer++;
			Cr2 = *ycbcrBuffer++;
		}
		// Interpolate and write Inpterpolated AJA_RGBAlphaPixel
		ycbcrPixel.cb = (uint16_t)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (uint16_t)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			AJA_SDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
		} else {
			AJA_HDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
		}

		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;

	}
}

// ConvertLinetoRGB
// 8 Bit YCbCr 8 Bit RGB version
void AJA_ConvertLinetoRGB(uint8_t * ycbcrBuffer, 
						  AJA_RGBAlphaPixel * rgbaBuffer,
						  uint32_t numPixels,
						  bool fUseSDMatrix)
{
	AJA_YCbCr10BitAlphaPixel ycbcrPixel;
	uint16_t Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to AJA_RGBAlphaPixels.
	// 2 AJA_RGBAlphaPixels at a time.
	Cb1 = (*ycbcrBuffer++)<<2;
	Y1 =  (*ycbcrBuffer++)<<2;
	Cr1 = (*ycbcrBuffer++)<<2;
	for ( uint32_t count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (uint16_t)Cb1;
		ycbcrPixel.y = (uint16_t)Y1; 
		ycbcrPixel.cr = (uint16_t)Cr1; 
		ycbcrPixel.Alpha = CCIR601_10BIT_WHITE;

		if(fUseSDMatrix) {
			AJA_SDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
		} else {
			AJA_HDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
		}
		// Read lone midde Y;
		ycbcrPixel.y = (*ycbcrBuffer++)<<2;

		// Read Next full bandwidth sample
		// unless we are at the end of a line
		if ( (count + 2 ) >= numPixels )
		{
			Cb2 = (uint16_t)(Cb1);
			Y2 = (uint16_t)(Y1);
			Cr2 = (uint16_t)(Cr1);
		}
		else
		{
			Cb2 = (*ycbcrBuffer++)<<2;
			Y2 = (*ycbcrBuffer++)<<2;
			Cr2 = (*ycbcrBuffer++)<<2;
		}
		// Interpolate and write Inpterpolated AJA_RGBAlphaPixel
		ycbcrPixel.cb = (uint16_t)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (uint16_t)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			AJA_SDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
		} else {
			AJA_HDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
		}

		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;

	}
}

// ConvertLinetoRGB
// 10 Bit YCbCr 8 Bit RGB version
void AJA_ConvertLinetoRGB(uint16_t * ycbcrBuffer, 
					  AJA_RGBAlphaPixel * rgbaBuffer,
					  uint32_t numPixels,
					  bool fUseSDMatrix)
{
	AJA_YCbCr10BitAlphaPixel ycbcrPixel;
	uint16_t Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to AJA_RGBAlphaPixels.
	// 2 AJA_RGBAlphaPixels at a time.
	Cb1 = *ycbcrBuffer++;
	Y1 = *ycbcrBuffer++;
	Cr1 = *ycbcrBuffer++;
	for ( uint32_t count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (uint16_t)Cb1;
		ycbcrPixel.y = (uint16_t)Y1; 
		ycbcrPixel.cr = (uint16_t)Cr1; 
		ycbcrPixel.Alpha = CCIR601_10BIT_WHITE;

		if(fUseSDMatrix) {
			AJA_SDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
		} else {
			AJA_HDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
		}
		// Read lone midde Y;
		ycbcrPixel.y = *ycbcrBuffer++;

		// Read Next full bandwidth sample
		// unless we are at the end of a line
		if ( (count + 2 ) >= numPixels )
		{
			Cb2 = (uint16_t)Cb1;
			Y2 = (uint16_t)Y1;
			Cr2 = (uint16_t)Cr1;
		}
		else
		{
			Cb2 = *ycbcrBuffer++;
			Y2 = *ycbcrBuffer++;
			Cr2 = *ycbcrBuffer++;
		}
		// Interpolate and write Inpterpolated AJA_RGBAlphaPixel
		ycbcrPixel.cb = (uint16_t)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (uint16_t)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			AJA_SDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
		} else {
			AJA_HDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
		}

		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;

	}
}

// ConvertLinetoRGB
// 10 Bit YCbCr 16 Bit RGB version
void AJA_ConvertLineto16BitRGB(uint16_t * ycbcrBuffer, AJA_RGBAlpha16BitPixel * rgbaBuffer, uint32_t numPixels, bool fUseSDMatrix)
{
	AJA_YCbCr10BitAlphaPixel ycbcrPixel;
	uint16_t Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to AJA_RGBAlpha16Pixels.
	// 2 AJA_RGBAlpha16Pixels at a time.
	Cb1 = *ycbcrBuffer++;
	Y1 = *ycbcrBuffer++;
	Cr1 = *ycbcrBuffer++;
	for ( uint32_t count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (uint16_t)Cb1;
		ycbcrPixel.y = (uint16_t)Y1; 
		ycbcrPixel.cr = (uint16_t)Cr1; 
		ycbcrPixel.Alpha = CCIR601_10BIT_WHITE;

		if(fUseSDMatrix) {
			AJA_SDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,(AJA_RGBAlpha10BitPixel *)&rgbaBuffer[count]);
			rgbaBuffer[count].Red = (rgbaBuffer[count].Red) << 6;
			rgbaBuffer[count].Green = (rgbaBuffer[count].Green) << 6;
			rgbaBuffer[count].Blue = (rgbaBuffer[count].Blue) << 6;
		} else {
			AJA_HDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,(AJA_RGBAlpha10BitPixel *)&rgbaBuffer[count]);
			rgbaBuffer[count].Red = (rgbaBuffer[count].Red) << 6;
			rgbaBuffer[count].Green = (rgbaBuffer[count].Green) << 6;
			rgbaBuffer[count].Blue = (rgbaBuffer[count].Blue) << 6;
		}

		// Read lone midde Y;
		ycbcrPixel.y = *ycbcrBuffer++;

		// Read Next full bandwidth sample
		// unless we are at the end of a line
		if ( (count + 2 ) >= numPixels )
		{
			Cb2 = (uint16_t)Cb1;
			Y2 = (uint16_t)Y1;
			Cr2 = (uint16_t)Cr1;
		}
		else
		{
			Cb2 = *ycbcrBuffer++;
			Y2 = *ycbcrBuffer++;
			Cr2 = *ycbcrBuffer++;
		}
		// Interpolate and write Inpterpolated AJA_RGBAlpha16Pixel
		ycbcrPixel.cb = (uint16_t)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (uint16_t)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			AJA_SDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,(AJA_RGBAlpha10BitPixel *)&rgbaBuffer[count+1]);
			rgbaBuffer[count+1].Red = (rgbaBuffer[count+1].Red) << 6;
			rgbaBuffer[count+1].Green = (rgbaBuffer[count+1].Green) << 6;
			rgbaBuffer[count+1].Blue = (rgbaBuffer[count+1].Blue) << 6;
		} else {
			AJA_HDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,(AJA_RGBAlpha10BitPixel *)&rgbaBuffer[count+1]);
			rgbaBuffer[count+1].Red = (rgbaBuffer[count+1].Red) << 6;
			rgbaBuffer[count+1].Green = (rgbaBuffer[count+1].Green) << 6;
			rgbaBuffer[count+1].Blue = (rgbaBuffer[count+1].Blue) << 6;
		}

		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;

	}
}

// ConvertLinetoBayer10BitDPXLJ
// 10 Bit YCbCr 10 Bit Bayer Left Justified version
void AJA_Convert16BitRGBtoBayer10BitDPXLJ(AJA_RGBAlpha16BitPixel * rgbaBuffer, uint32_t * bayerBuffer, 
										  uint32_t numPixels, uint32_t line, AJA_BayerColorPhase phase)
{
	uint32_t pixel0 = 0;
	uint32_t pixel1 = 0;

	for ( uint32_t count = 0; count < numPixels; count++ )
	{
		switch (phase)
		{
		default:
		case AJA_BayerColorPhase_RedGreen:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Red;
				pixel1 = rgbaBuffer[count].Green;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Blue;
			}
			break;
		case AJA_BayerColorPhase_GreenRed:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Red;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Blue;
				pixel1 = rgbaBuffer[count].Green;
			}
			break;
		case AJA_BayerColorPhase_BlueGreen:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Blue;
				pixel1 = rgbaBuffer[count].Green;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Red;
			}
			break;
		case AJA_BayerColorPhase_GreenBlue:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Blue;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Red;
				pixel1 = rgbaBuffer[count].Green;
			}
			break;
		}

		uint32_t ind = count/3;
		uint32_t rem = count%6;

		switch (rem)
		{
		case 0:
			bayerBuffer[ind] = ((pixel0 >> 6) & 0x3ff) << 2;
			break;
		case 1:
			bayerBuffer[ind] |= ((pixel1 >> 6) & 0x3ff) << 12;
			break;
		case 2:
			bayerBuffer[ind] |= ((pixel0 >> 6) & 0x3ff) << 22;
			break;
		case 3:
			bayerBuffer[ind] = ((pixel1 >> 6) & 0x3ff) << 2;
			break;
		case 4:
			bayerBuffer[ind] |= ((pixel0 >> 6) & 0x3ff) << 12;
			break;
		case 5:
			bayerBuffer[ind] |= ((pixel1 >> 6) & 0x3ff) << 22;
			break;
		default:
			break;
		}
	}
}

// ConvertLinetoBayer12BitDPXLJ
// 10 Bit YCbCr 12 Bit Bayer Left Justified version
void AJA_Convert16BitRGBtoBayer12BitDPXLJ(AJA_RGBAlpha16BitPixel * rgbaBuffer, uint32_t * bayerBuffer, 
										  uint32_t numPixels, uint32_t line, AJA_BayerColorPhase phase)
{
	uint32_t pixel0 = 0;
	uint32_t pixel1 = 0;
	uint16_t* buffer = (uint16_t*)bayerBuffer;

	for ( uint32_t count = 0; count < numPixels; count++ )
	{
		switch (phase)
		{
		default:
		case AJA_BayerColorPhase_RedGreen:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Red;
				pixel1 = rgbaBuffer[count].Green;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Blue;
			}
			break;
		case AJA_BayerColorPhase_GreenRed:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Red;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Blue;
				pixel1 = rgbaBuffer[count].Green;
			}
			break;
		case AJA_BayerColorPhase_BlueGreen:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Blue;
				pixel1 = rgbaBuffer[count].Green;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Red;
			}
			break;
		case AJA_BayerColorPhase_GreenBlue:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Blue;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Red;
				pixel1 = rgbaBuffer[count].Green;
			}
			break;
		}

		if ((count & 0x1) == 0)
		{
			buffer[count] = pixel0;
		}
		else
		{
			buffer[count] = pixel1;
		}
	}
}

// ConvertLinetoBayer10BitDPXPacked
// 10 Bit YCbCr 10 Bit Bayer Packed version
void AJA_Convert16BitRGBtoBayer10BitDPXPacked(AJA_RGBAlpha16BitPixel * rgbaBuffer, uint8_t * bayerBuffer, 
											  uint32_t numPixels, uint32_t line, AJA_BayerColorPhase phase)
{
	uint32_t pixel0 = 0;
	uint32_t pixel1 = 0;
	uint32_t pack = 0;

	for ( uint32_t count = 0; count < numPixels; count++ )
	{
		switch (phase)
		{
		default:
		case AJA_BayerColorPhase_RedGreen:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Red;
				pixel1 = rgbaBuffer[count].Green;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Blue;
			}
			break;
		case AJA_BayerColorPhase_GreenRed:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Red;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Blue;
				pixel1 = rgbaBuffer[count].Green;
			}
			break;
		case AJA_BayerColorPhase_BlueGreen:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Blue;
				pixel1 = rgbaBuffer[count].Green;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Red;
			}
			break;
		case AJA_BayerColorPhase_GreenBlue:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Blue;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Red;
				pixel1 = rgbaBuffer[count].Green;
			}
			break;
		}

		switch (pack)
		{
		case 0:
			if ((count & 0x1) == 0)
			{
				*bayerBuffer++ = (uint8_t)((pixel0 >> 6) & 0xff);
				*bayerBuffer = (uint8_t)((pixel0 >> 14) & 0x3);
			}
			else
			{
				*bayerBuffer++ = (uint8_t)((pixel1 >> 6) & 0xff);
				*bayerBuffer = (uint8_t)((pixel1 >> 14) & 0x3);
			}
			pack = 2;
			break;
		case 2:
			if ((count & 0x1) == 0)
			{
				*bayerBuffer++ |= (uint8_t)(((pixel0 >> 6) & 0x3f) << 2);
				*bayerBuffer = (uint8_t)((pixel0 >> 12) & 0xf);
			}
			else
			{
				*bayerBuffer++ |= (uint8_t)(((pixel1 >> 6) & 0x3f) << 2);
				*bayerBuffer = (uint8_t)((pixel1 >> 12) & 0xf);
			}
			pack = 4;
			break;
		case 4:
			if ((count & 0x1) == 0)
			{
				*bayerBuffer++ |= (uint8_t)(((pixel0 >> 6) & 0xf) << 4);
				*bayerBuffer = (uint8_t)((pixel0 >> 10) & 0x3f);
			}
			else
			{
				*bayerBuffer++ |= (uint8_t)(((pixel1 >> 6) & 0xf) << 4);
				*bayerBuffer = (uint8_t)((pixel1 >> 10) & 0x3f);
			}
			pack = 6;
			break;
		case 6:
			if ((count & 0x1) == 0)
			{
				*bayerBuffer++ |= (uint8_t)(((pixel0 >> 6) & 0x3) << 6);
				*bayerBuffer++ = (uint8_t)((pixel0 >> 8) & 0xff);
			}
			else
			{
				*bayerBuffer++ |= (uint8_t)(((pixel1 >> 6) & 0x3) << 6);
				*bayerBuffer++ = (uint8_t)((pixel1 >> 8) & 0xff);
			}
			pack = 0;
			break;
		default:
			break;
		}
	}
}

// ConvertLinetoBayer12BitDPXPacked
// 10 Bit YCbCr 12 Bit Bayer Packed version
void AJA_Convert16BitRGBtoBayer12BitDPXPacked(AJA_RGBAlpha16BitPixel * rgbaBuffer, uint8_t * bayerBuffer, 
											  uint32_t numPixels, uint32_t line, AJA_BayerColorPhase phase)
{
	uint32_t pixel0 = 0;
	uint32_t pixel1 = 0;

	for ( uint32_t count = 0; count < numPixels; count++ )
	{
		switch (phase)
		{
		default:
		case AJA_BayerColorPhase_RedGreen:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Red;
				pixel1 = rgbaBuffer[count].Green;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Blue;
			}
			break;
		case AJA_BayerColorPhase_GreenRed:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Red;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Blue;
				pixel1 = rgbaBuffer[count].Green;
			}
			break;
		case AJA_BayerColorPhase_BlueGreen:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Blue;
				pixel1 = rgbaBuffer[count].Green;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Red;
			}
			break;
		case AJA_BayerColorPhase_GreenBlue:
			if ((line & 0x1) == 0)
			{
				pixel0 = rgbaBuffer[count].Green;
				pixel1 = rgbaBuffer[count].Blue;
			}
			else
			{
				pixel0 = rgbaBuffer[count].Red;
				pixel1 = rgbaBuffer[count].Green;
			}
			break;
		}

		if ((count & 0x1) == 0)
		{
			*bayerBuffer++ = (uint8_t)(((pixel0 >> 4) & 0xff));
			*bayerBuffer = (uint8_t)(((pixel0 >> 12) & 0xf));
		}
		else
		{
			*bayerBuffer++ |= (uint8_t)(((pixel1 >> 4) & 0xf) << 4);
			*bayerBuffer++ = (uint8_t)(((pixel1 >> 8) & 0xff));
		}
/*
		if ((count & 0x1) == 0)
		{
			*bayerBuffer = (uint8_t)(((pixel0 >> 12) & 0xf));
			*bayerBuffer++ |= (uint8_t)(((pixel0 >> 8) & 0xf) << 4);
			*bayerBuffer = (uint8_t)(((pixel0 >> 4) & 0xf));
		}
		else
		{
			*bayerBuffer++ |= (uint8_t)(((pixel1 >> 12) & 0xf) << 4);
			*bayerBuffer = (uint8_t)(((pixel1 >> 8) & 0xf));
			*bayerBuffer++ |= (uint8_t)(((pixel1 >> 4) & 0xf) << 4);
		}
*/
	}
}

// Converts 8 Bit ARGB 8 Bit RGBA in place
void AJA_ConvertARGBToRGBA(uint8_t* rgbaBuffer,uint32_t numPixels)
{
	for ( uint32_t pixel=0;pixel<numPixels*4;pixel+=4)
	{
		uint8_t B = rgbaBuffer[pixel];
		uint8_t G = rgbaBuffer[pixel+1];
		uint8_t R = rgbaBuffer[pixel+2];
		uint8_t A = rgbaBuffer[pixel+3];
		rgbaBuffer[pixel]   = A;
		rgbaBuffer[pixel+1] = R;
		rgbaBuffer[pixel+2] = G;
		rgbaBuffer[pixel+3] = B;
	}
}

// Converts 8 Bit ARGB 8 Bit ABGR in place
void AJA_ConvertARGBToABGR(uint8_t* rgbaBuffer,uint32_t numPixels)
{
	for ( uint32_t pixel=0;pixel<numPixels*4;pixel+=4)
	{
		uint8_t B = rgbaBuffer[pixel];
		uint8_t G = rgbaBuffer[pixel+1];
		uint8_t R = rgbaBuffer[pixel+2];
		uint8_t A = rgbaBuffer[pixel+3];
		rgbaBuffer[pixel]   = R;
		rgbaBuffer[pixel+1] = G;
		rgbaBuffer[pixel+2] = B;
		rgbaBuffer[pixel+3] = A;
	}
}

// Converts 8 Bit ARGB 8 Bit ABGR in place
static void AJA_ConvertARGBToRGB(uint8_t* rgbaBuffer,uint8_t * rgbBuffer, uint32_t numPixels)
{
	for ( uint32_t pixel=0;pixel<numPixels*4;pixel+=4)
	{
		uint8_t B = rgbaBuffer[pixel];
		uint8_t G = rgbaBuffer[pixel+1];
		uint8_t R = rgbaBuffer[pixel+2];
		*rgbBuffer++ = R;
		*rgbBuffer++ = G;
		*rgbBuffer++ = B;
	}
}

// Converts 8 Bit ARGB 8 Bit ABGR in place
static void AJA_ConvertARGBToBGR(uint8_t* rgbaBuffer, uint8_t * rgbBuffer, uint32_t numPixels)
{
	for ( uint32_t pixel=0;pixel<numPixels*4;pixel+=4)
	{
		uint8_t B = rgbaBuffer[pixel];
		uint8_t G = rgbaBuffer[pixel+1];
		uint8_t R = rgbaBuffer[pixel+2];
		*rgbBuffer++ = B;
		*rgbBuffer++ = G;
		*rgbBuffer++ = R;
	}
}

void AJA_Convert16BitARGBTo16BitRGB(AJA_RGBAlpha16BitPixel *rgbaLineBuffer ,uint16_t * rgbLineBuffer,uint32_t numPixels)
{
	for ( uint32_t pixel=0;pixel<numPixels;pixel++)
	{
		AJA_RGBAlpha16BitPixel* pixelBuffer = &(rgbaLineBuffer[pixel]);
		uint16_t B = pixelBuffer->Blue;
		uint16_t G = pixelBuffer->Green;
		uint16_t R = pixelBuffer->Red;
		*rgbLineBuffer++ = R;
		*rgbLineBuffer++ = G;
		*rgbLineBuffer++ = B;
	}
}

void AJA_Convert16BitARGBTo12BitRGBPacked(AJA_RGBAlpha16BitPixel *rgbaLineBuffer ,uint8_t * rgbLineBuffer,uint32_t numPixels)
{
	uint32_t numRowBytes = AJA_CalcRowBytesForFormat(AJA_PixelFormat_RGB12,numPixels);

	for ( uint32_t byteNumber = 0; byteNumber < numRowBytes; byteNumber += 9 )
	{
		AJA_RGBAlpha16BitPixel pixelValue = *rgbaLineBuffer++;
		uint16_t Blue1 = pixelValue.Blue>>4;
		uint16_t Green1 = pixelValue.Green>>4;
		uint16_t Red1 = pixelValue.Red>>4;

		pixelValue = *rgbaLineBuffer++;
		uint16_t Blue2 = pixelValue.Blue>>4;
		uint16_t Green2 = pixelValue.Green>>4;
		uint16_t Red2 = pixelValue.Red>>4;
		*rgbLineBuffer++ = (uint8_t)(Red1&0xFF);
		*rgbLineBuffer++ = (uint8_t)(((Red1>>8)&0xF) + ((Green1&0xF)<<4));
		*rgbLineBuffer++ = (uint8_t)((Green1>>4));
		*rgbLineBuffer++ = (uint8_t)(Blue1&0xFF);
		*rgbLineBuffer++ = (uint8_t)(((Blue1>>8)&0xF) + ((Red2&0xF)<<4));
		*rgbLineBuffer++ = (uint8_t)((Red2>>4));
		*rgbLineBuffer++ = (uint8_t)(Green2&0xFF);
		*rgbLineBuffer++ = (uint8_t)(((Green2>>8)&0xF) + ((Blue2&0xF)<<4));
		*rgbLineBuffer++ = (uint8_t)((Blue2>>4));

	}
}

// Converts UYVY(2yuv) -> YUY2(yuv2) in place
void AJA_Convert8BitYCbCrToYUY2(uint8_t * ycbcrBuffer, uint32_t numPixels)
{
	for ( uint32_t pixel=0;pixel<numPixels*2;pixel+=4)
	{
		uint8_t Cb = ycbcrBuffer[pixel];
		uint8_t Y1 = ycbcrBuffer[pixel+1];
		uint8_t Cr = ycbcrBuffer[pixel+2];
		uint8_t Y2 = ycbcrBuffer[pixel+3];
		ycbcrBuffer[pixel]   = Y1;
		ycbcrBuffer[pixel+1] = Cb;
		ycbcrBuffer[pixel+2] = Y2;
		ycbcrBuffer[pixel+3] = Cr;
	}
}

// RePackLineDataForYCbCrDPX2
void AJA_RePackLineDataForYCbCrDPX(uint32_t *packedycbcrLine, uint32_t numULWords)
{

	for ( uint16_t count = 0; count < numULWords; count++)
	{
		uint32_t value = (packedycbcrLine[count])<<2;
		value = (value<<24) + ((value>>24)&0x000000FF) + ((value<<8)&0x00FF0000) + ((value>>8)&0x0000FF00);
		 
		packedycbcrLine[count] = value;
	}
}



//***********************************************************************************************************

// ConvertUnpacked10BitYCbCrToPixelFormat()
//		Converts a line of "unpacked" 10-bit Y/Cb/Cr pixels into a "packed" line in the pixel format
//	for the current frame buffer format.
void AJA_ConvertUnpacked10BitYCbCrToPixelFormat(uint16_t *unPackedBuffer, uint32_t *packedBuffer, uint32_t numPixels, AJA_PixelFormat pixelFormat)
{
	bool  bIsSD = false;
	if(numPixels < 1280)
		bIsSD = true;

	switch(pixelFormat) 
	{
		case AJA_PixelFormat_YCbCr10:
			AJA_PackTo10BitYCbCrBuffer(unPackedBuffer, packedBuffer, numPixels);
			break;

		case AJA_PixelFormat_YCbCr_DPX:
			AJA_PackTo10BitYCbCrDPXBuffer(unPackedBuffer, packedBuffer, numPixels);
			break;

		case AJA_PixelFormat_YCbCr8:
			AJA_ConvertLineto8BitYCbCr(unPackedBuffer,(uint8_t*)packedBuffer, numPixels);
			break;

		case AJA_PixelFormat_YUY28:
			AJA_ConvertLineto8BitYCbCr(unPackedBuffer,(uint8_t*)packedBuffer, numPixels);
			AJA_Convert8BitYCbCrToYUY2((uint8_t*)packedBuffer,numPixels);
			break;

		case AJA_PixelFormat_RGB10:
			AJA_ConvertLineto10BitRGB(unPackedBuffer,(AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD); 
			AJA_PackRGB10BitFor10BitRGB((AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels);
			break;

		case AJA_PixelFormat_RGB10_PACK:
			AJA_ConvertLineto10BitRGB(unPackedBuffer,(AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD); 
			AJA_PackRGB10BitFor10BitRGBPacked((AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels);
			break;

		case AJA_PixelFormat_RGB_DPX:
			AJA_ConvertLineto10BitRGB(unPackedBuffer,(AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD); 
			AJA_PackRGB10BitFor10BitDPX((AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels);
			break;

		case AJA_PixelFormat_RGB_DPX_LE:
			AJA_ConvertLineto10BitRGB(unPackedBuffer,(AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD); 
			AJA_PackRGB10BitFor10BitDPX((AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels,false);
			break;

		case AJA_PixelFormat_ARGB8:
			AJA_ConvertLinetoRGB(unPackedBuffer,(AJA_RGBAlphaPixel*)packedBuffer,numPixels, bIsSD);
			break;

		case AJA_PixelFormat_RGBA8:
			AJA_ConvertLinetoRGB(unPackedBuffer,(AJA_RGBAlphaPixel*)packedBuffer,numPixels, bIsSD);
			AJA_ConvertARGBToRGBA((uint8_t*)packedBuffer,numPixels);
			break;

		case AJA_PixelFormat_ABGR8:
			AJA_ConvertLinetoRGB(unPackedBuffer,(AJA_RGBAlphaPixel*)packedBuffer,numPixels, bIsSD);
			AJA_ConvertARGBToABGR((uint8_t*)packedBuffer,numPixels);
			break;

		case AJA_PixelFormat_RGB16:
			AJA_ConvertLineto16BitRGB(unPackedBuffer,(AJA_RGBAlpha16BitPixel*)packedBuffer,numPixels, bIsSD);
			AJA_Convert16BitARGBTo16BitRGB((AJA_RGBAlpha16BitPixel*)packedBuffer ,(uint16_t*) packedBuffer, numPixels);
			break;

		case AJA_PixelFormat_RGB12:
			AJA_ConvertLineto16BitRGB(unPackedBuffer,(AJA_RGBAlpha16BitPixel*)packedBuffer,numPixels, bIsSD);
			AJA_Convert16BitARGBTo12BitRGBPacked((AJA_RGBAlpha16BitPixel*)packedBuffer ,(uint8_t*) packedBuffer, numPixels);
			break;

		case AJA_PixelFormat_RGB8_PACK:
			AJA_ConvertLinetoRGB(unPackedBuffer, (AJA_RGBAlphaPixel*)packedBuffer,numPixels, bIsSD);
			AJA_ConvertARGBToRGB((uint8_t*)packedBuffer,(uint8_t*)packedBuffer, numPixels);
			break;

		case AJA_PixelFormat_BGR8_PACK:
			AJA_ConvertLinetoRGB(unPackedBuffer, (AJA_RGBAlphaPixel*)packedBuffer,numPixels, bIsSD);
			AJA_ConvertARGBToBGR((uint8_t*)packedBuffer,(uint8_t*)packedBuffer, numPixels);
			break;

		case AJA_PixelFormat_BAYER10_DPX_LJ:
			AJA_ConvertLineto16BitRGB(unPackedBuffer,(AJA_RGBAlpha16BitPixel*)packedBuffer,numPixels, bIsSD);
			break;
		case AJA_PixelFormat_BAYER12_DPX_LJ:
			AJA_ConvertLineto16BitRGB(unPackedBuffer,(AJA_RGBAlpha16BitPixel*)packedBuffer,numPixels, bIsSD);
			break;
		case AJA_PixelFormat_BAYER10_HS:
			AJA_ConvertLineto16BitRGB(unPackedBuffer,(AJA_RGBAlpha16BitPixel*)packedBuffer,numPixels, bIsSD);
			break;
		case AJA_PixelFormat_BAYER12_HS:
			AJA_ConvertLineto16BitRGB(unPackedBuffer,(AJA_RGBAlpha16BitPixel*)packedBuffer,numPixels, bIsSD);
			break;
			
		default:
			// TO DO: add all other formats.

			break;
	}
}

// ConvertUnpacked10BitYCbCrToPixelFormat()
//		Converts a line of "unpacked" 10-bit Y/Cb/Cr pixels into a "packed" line in the pixel format
//	for the current frame buffer format.
void AJA_ConvertPixelFormatToRGBA(uint32_t *buffer, AJA_RGBAlphaPixel* rgbBuffer, uint32_t numPixels, AJA_PixelFormat pixelFormat,bool  bIsSD)
{

	switch(pixelFormat) 
	{
	case AJA_PixelFormat_YCbCr10:
		{
			uint16_t* unPackedBuffer = new uint16_t[numPixels*2];
			AJA_UnPack10BitYCbCrBuffer(buffer,unPackedBuffer,numPixels);
			AJA_ConvertLinetoRGB(unPackedBuffer,(AJA_RGBAlphaPixel*)rgbBuffer,numPixels, bIsSD);
			delete [] unPackedBuffer;
		}
		break;
	case AJA_PixelFormat_YCbCr8:
		AJA_ConvertLinetoRGB((uint8_t *)buffer,rgbBuffer,numPixels,bIsSD);
		//AJA_ConvertLineto8BitYCbCr(unPackedBuffer,(uint8_t*)packedBuffer, numPixels);
		break;

#if 0

	case AJA_PixelFormat_YCbCr_DPX:
		AJA_RePackLineDataForYCbCrDPX(packedBuffer,AJA_CalcRowBytesForFormat(AJA_PixelFormat_YCbCr_DPX,numPixels));
		break;


	case AJA_PixelFormat_YUY28:
		AJA_ConvertLineto8BitYCbCr(unPackedBuffer,(uint8_t*)packedBuffer, numPixels);
		AJA_Convert8BitYCbCrToYUY2((uint8_t*)packedBuffer,numPixels);
		break;

	case AJA_PixelFormat_RGB10:
		AJA_ConvertLineto10BitRGB(unPackedBuffer,(AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD); 
		AJA_PackRGB10BitFor10BitRGB((AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels);
		break;

	case AJA_PixelFormat_RGB10_PACK:
		AJA_ConvertLineto10BitRGB(unPackedBuffer,(AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD); 
		AJA_PackRGB10BitFor10BitRGBPacked((AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels);
		break;

	case AJA_PixelFormat_RGB_DPX:
		AJA_ConvertLineto10BitRGB(unPackedBuffer,(AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD); 
		AJA_PackRGB10BitFor10BitDPX((AJA_RGBAlpha10BitPixel*)packedBuffer,numPixels);
		break;

	case AJA_PixelFormat_ARGB8:
		AJA_ConvertLinetoRGB(unPackedBuffer,(AJA_RGBAlphaPixel*)packedBuffer,numPixels, bIsSD);
		break;

	case AJA_PixelFormat_RGBA8:
		AJA_ConvertLinetoRGB(unPackedBuffer,(AJA_RGBAlphaPixel*)packedBuffer,numPixels, bIsSD);
		AJA_ConvertARGBToRGBA((uint8_t*)packedBuffer,numPixels);
		break;

	case AJA_PixelFormat_ABGR8:
		AJA_ConvertLinetoRGB(unPackedBuffer,(AJA_RGBAlphaPixel*)packedBuffer,numPixels, bIsSD);
		AJA_ConvertARGBToABGR((uint8_t*)packedBuffer,numPixels);
		break;

	case AJA_PixelFormat_RGB16:
		AJA_ConvertLineto16BitRGB(unPackedBuffer,(AJA_RGBAlpha16BitPixel*)packedBuffer,numPixels, bIsSD);
		AJA_Convert16BitARGBTo16BitRGB((AJA_RGBAlpha16BitPixel*)packedBuffer ,(uint16_t*) packedBuffer, numPixels);
		break;
#endif
	default:
		// TO DO: add all other formats.

		break;
	}

}

// MaskUnPacked10BitYCbCrBuffer
// Mask Data In place based on signalMask
void AJA_MaskUnPacked10BitYCbCrBuffer( uint16_t* ycbcrUnPackedBuffer, uint16_t signalMask , uint32_t numPixels )
{
	uint32_t pixelCount;

	// Not elegant but fairly fast.
	switch ( signalMask )
	{
	case AJA_SIGNALMASK_NONE:          // Output Black
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}
		break;
	case AJA_SIGNALMASK_Y:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break;
	case AJA_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}

		break;
	case AJA_SIGNALMASK_Y + AJA_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break; 

	case AJA_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break;
	case AJA_SIGNALMASK_Y + AJA_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
		}


		break; 
	case AJA_SIGNALMASK_Cb + AJA_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break; 
	case AJA_SIGNALMASK_Y + AJA_SIGNALMASK_Cb + AJA_SIGNALMASK_Cr:
		// Do nothing
		break; 
	}

}

void WriteLineToBuffer(AJA_PixelFormat pixelFormat, uint32_t currentLine, uint32_t numPixels, uint32_t linePitch, 
					   uint8_t* pOutputBuffer,uint32_t* pPackedLineBuffer )
{
	switch (pixelFormat)
	{
	case AJA_PixelFormat_BAYER10_DPX_LJ:
		{
			uint32_t bayerLinePitch = ((numPixels + 2)/3) * 4;
			uint8_t *pBuffer = pOutputBuffer + (currentLine * bayerLinePitch);
			AJA_Convert16BitRGBtoBayer10BitDPXLJ((AJA_RGBAlpha16BitPixel*)pPackedLineBuffer ,(uint32_t*)pBuffer, numPixels, currentLine);
		}
		break;
	case AJA_PixelFormat_BAYER12_DPX_LJ:
		{
			uint32_t bayerLinePitch = (linePitch/3);
			uint8_t *pBuffer = pOutputBuffer + (currentLine * bayerLinePitch);
			AJA_Convert16BitRGBtoBayer12BitDPXLJ((AJA_RGBAlpha16BitPixel*)pPackedLineBuffer ,(uint32_t*)pBuffer, numPixels, currentLine);

		}
		break;
	case AJA_PixelFormat_BAYER10_HS:
		{
			uint32_t bayerLinePitch = ((numPixels + 2)/3) * 4;
			uint8_t *pBuffer = pOutputBuffer + (currentLine * bayerLinePitch);
			AJA_Convert16BitRGBtoBayer10BitDPXPacked((AJA_RGBAlpha16BitPixel*)pPackedLineBuffer ,(uint8_t*)pBuffer, numPixels, currentLine);

		}
		break;
	case AJA_PixelFormat_BAYER12_HS:
		{
			uint32_t bayerLinePitch = linePitch/3;  
			uint8_t *pBuffer = pOutputBuffer + (currentLine * bayerLinePitch);
			AJA_Convert16BitRGBtoBayer12BitDPXPacked((AJA_RGBAlpha16BitPixel*)pPackedLineBuffer ,(uint8_t*)pBuffer, numPixels, currentLine);

		}
		break;
	default:
		{
			uint8_t *pBuffer = pOutputBuffer + (currentLine * linePitch);
			memcpy(pBuffer, pPackedLineBuffer, linePitch);

		}
		break;
	}
}


void WriteLineToBuffer(AJA_PixelFormat pixelFormat, AJA_BayerColorPhase bayerPhase, uint32_t currentLine, 
					   uint32_t numPixels, uint32_t linePitch, uint8_t* pOutputBuffer, uint32_t* pPackedLineBuffer )
{
	switch (pixelFormat)
	{
	case AJA_PixelFormat_BAYER10_DPX_LJ:
		{
			uint32_t bayerLinePitch = ((numPixels + 2)/3) * 4;
			uint8_t *pBuffer = pOutputBuffer + (currentLine * bayerLinePitch);
			AJA_Convert16BitRGBtoBayer10BitDPXLJ((AJA_RGBAlpha16BitPixel*)pPackedLineBuffer ,(uint32_t*)pBuffer, numPixels, currentLine, bayerPhase);
		}
		break;
	case AJA_PixelFormat_BAYER12_DPX_LJ:
		{
			uint32_t bayerLinePitch = (linePitch/3);
			uint8_t *pBuffer = pOutputBuffer + (currentLine * bayerLinePitch);
			AJA_Convert16BitRGBtoBayer12BitDPXLJ((AJA_RGBAlpha16BitPixel*)pPackedLineBuffer ,(uint32_t*)pBuffer, numPixels, currentLine, bayerPhase);

		}
		break;
	case AJA_PixelFormat_BAYER10_HS:
		{
			uint32_t bayerLinePitch = ((numPixels + 2)/3) * 4;
			uint8_t *pBuffer = pOutputBuffer + (currentLine * bayerLinePitch);
			AJA_Convert16BitRGBtoBayer10BitDPXPacked((AJA_RGBAlpha16BitPixel*)pPackedLineBuffer ,(uint8_t*)pBuffer, numPixels, currentLine, bayerPhase);

		}
		break;
	case AJA_PixelFormat_BAYER12_HS:
		{
			uint32_t bayerLinePitch = linePitch/4;  
			uint8_t *pBuffer = pOutputBuffer + (currentLine * bayerLinePitch);
			AJA_Convert16BitRGBtoBayer12BitDPXPacked((AJA_RGBAlpha16BitPixel*)pPackedLineBuffer ,(uint8_t*)pBuffer, numPixels, currentLine, bayerPhase);

		}
		break;
	default:
		{
			uint8_t *pBuffer = pOutputBuffer + (currentLine * linePitch);
			memcpy(pBuffer, pPackedLineBuffer, linePitch);

		}
		break;
	}
}

// ReSampleLine
// RGBAlphaPixel Version
// NOTE: Input buffer needs to have extra sample at start and 2 at end.
void AJA_ReSampleLine(AJA_RGBAlphaPixel *Input, 
				  AJA_RGBAlphaPixel *Output,
				  uint16_t startPixel,
				  uint16_t endPixel,
				  int32_t numInputPixels,
				  int32_t numOutputPixels)

{
	int32_t count,inputIndex,reSampleStartPixel,reSampleEndPixel;
	int32_t accum = 0;
	int32_t increment,coefIndex;

	Input[-1] = Input[0];
	Input[numInputPixels] = Input[numInputPixels+1] = Input[numInputPixels-1];

	increment = (numInputPixels<<16)/numOutputPixels;
	reSampleStartPixel = (startPixel*numOutputPixels)/numInputPixels;
	reSampleEndPixel = (endPixel*numOutputPixels)/numInputPixels;
	for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
	{
		accum = (increment*count);
		inputIndex = FixedTrunc(accum);
		coefIndex = (accum>>11) & 0x1F;
		Output[count] = CubicInterPolate(&Input[inputIndex],coefIndex);
	}

}


// ReSampleLine
// Word Version
// NOTE: Input buffer needs to have extra sample at start and 2 at end.
void AJA_ReSampleLine(int16_t *Input, 
				  int16_t *Output,
				  uint16_t startPixel,
				  uint16_t endPixel,
				  int32_t numInputPixels,
				  int32_t numOutputPixels)

{
	int32_t count,inputIndex,reSampleStartPixel,reSampleEndPixel;
	int32_t accum = 0;
	int32_t increment,coefIndex;

	Input[-1] = Input[0];
	Input[numInputPixels] = Input[numInputPixels+1] = Input[numInputPixels-1];

	increment = (numInputPixels<<16)/numOutputPixels;
	reSampleStartPixel = (startPixel*numOutputPixels)/numInputPixels;
	reSampleEndPixel = (endPixel*numOutputPixels)/numInputPixels;
	for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
	{
		accum = (increment*count);
		inputIndex = FixedTrunc(accum);
		coefIndex = (accum>>11) & 0x1F;
		Output[count] = (int16_t)CubicInterPolateWord((int16_t *)&Input[inputIndex],coefIndex);
	}
}

// ReSampleLine
// Word Version
void AJA_ReSampleYCbCrSampleLine(int16_t *Input, 
							 int16_t *Output,
							 int32_t numInputPixels,
							 int32_t numOutputPixels)

{
	int32_t count,inputIndex,reSampleStartPixel,reSampleEndPixel;
	int32_t accum = 0;
	int32_t increment,coefIndex;

	int16_t* yBuffer = new int16_t[numInputPixels+4];
	int16_t* cbBuffer = new int16_t[numInputPixels/2+4];
	int16_t* crBuffer = new int16_t[numInputPixels/2+4];

	// First Extract Y
	for ( count = 0; count < numInputPixels; count++ )
	{
		if ( count & 0x1)
		{
			crBuffer[count/2+1] = Input[count*2];
		}
		else
		{
			cbBuffer[count/2+1] = Input[count*2];

		}
		yBuffer[count+1] = Input[count*2+1];
	}

	yBuffer[0] = yBuffer[1];
	yBuffer[numInputPixels+1] = yBuffer[numInputPixels+2] = yBuffer[numInputPixels];

	increment = (numInputPixels<<16)/numOutputPixels;
	reSampleStartPixel = 0;
	reSampleEndPixel = numOutputPixels;
	for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
	{
		accum = (increment*count);
		inputIndex = FixedTrunc(accum);
		coefIndex = (accum>>11) & 0x1F;
		Output[count*2+1] = (int16_t)CubicInterPolateWord((int16_t *)&yBuffer[inputIndex+1],coefIndex);
	}

	reSampleStartPixel = 0;
	reSampleEndPixel = numOutputPixels/2;
	cbBuffer[0] = cbBuffer[1];
	cbBuffer[numInputPixels/2+1] = cbBuffer[numInputPixels/2+2] = cbBuffer[numInputPixels/2];
	crBuffer[0] = crBuffer[1];
	crBuffer[numInputPixels/2+1] = crBuffer[numInputPixels/2+2] = crBuffer[numInputPixels/2];

	for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
	{
		accum = (increment*count);
		inputIndex = FixedTrunc(accum);
		coefIndex = (accum>>11) & 0x1F;
		Output[count*4] = (int16_t)CubicInterPolateWord((int16_t *)&cbBuffer[inputIndex+1],coefIndex);
		Output[count*4+2] = (int16_t)CubicInterPolateWord((int16_t *)&crBuffer[inputIndex+1],coefIndex);
	}

	delete [] yBuffer;
	delete [] cbBuffer;
	delete [] crBuffer;
}

void AJA_ReSampleAudio(int16_t *Input, 
				   int16_t *Output,
				   uint16_t startPixel,
				   uint16_t endPixel,
				   int32_t numInputPixels,
				   int32_t numOutputPixels,
				   int16_t channelInterleaveMulitplier)
{
	int32_t count,inputIndex,reSampleStartPixel,reSampleEndPixel;
	int32_t accum = 0;
	int32_t increment,coefIndex;

	increment = (numInputPixels<<16)/numOutputPixels;
	reSampleStartPixel = (startPixel*numOutputPixels)/numInputPixels;
	reSampleEndPixel = (endPixel*numOutputPixels)/numInputPixels;
	for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
	{
		accum = (increment*count);
		inputIndex = FixedTrunc(accum);
		coefIndex = (accum>>11) & 0x1F;
		Output[count*channelInterleaveMulitplier] = (int16_t)CubicInterPolateAudioWord((int16_t *)&Input[inputIndex],coefIndex);
	}
}

inline AJA_RGBAlphaPixel CubicInterPolate( AJA_RGBAlphaPixel *Input, int32_t Index)
{

	AJA_RGBAlphaPixel InterPolatedValue;
	int32_t temp;


	temp = FixedTrunc(Input[-1].Red*CubicCoef[32-Index] + 
		Input[0].Red*CubicCoef[64-Index] + 
		Input[1].Red*CubicCoef[96-Index] + 
		Input[2].Red*CubicCoef[128-Index]);

	InterPolatedValue.Red = (uint8_t)((temp < 0 ) ? 0 : (temp > 255 ) ? 255 : temp);

	temp= FixedTrunc(Input[-1].Green*CubicCoef[32-Index] + 
		Input[0].Green*CubicCoef[64-Index] + 
		Input[1].Green*CubicCoef[96-Index] + 
		Input[2].Green*CubicCoef[128-Index]);
	InterPolatedValue.Green = (uint8_t)((temp < 0 ) ? 0 : (temp > 255 ) ? 255 : temp);


	temp = FixedTrunc(Input[-1].Blue*CubicCoef[32-Index] + 
		Input[0].Blue*CubicCoef[64-Index] + 
		Input[1].Blue*CubicCoef[96-Index] + 
		Input[2].Blue*CubicCoef[128-Index]);

	InterPolatedValue.Blue = (uint8_t)((temp < 0 ) ? 0 : (temp > 255 ) ? 255 : temp);

	temp = FixedTrunc(Input[-1].Alpha*CubicCoef[32-Index] + 
		Input[0].Alpha*CubicCoef[64-Index] + 
		Input[1].Alpha*CubicCoef[96-Index] + 
		Input[2].Alpha*CubicCoef[128-Index]);

	InterPolatedValue.Alpha = (uint8_t)((temp < 0 ) ? 0 : (temp > 255 ) ? 255 : temp);

	return InterPolatedValue;

}

inline int16_t CubicInterPolateWord( int16_t *Input, int32_t Index)
{
	int16_t InterPolatedValue;

	InterPolatedValue = FixedTrunc(Input[-1]*CubicCoef[32-Index] + 
		Input[0]*CubicCoef[64-Index] + 
		Input[1]*CubicCoef[96-Index] + 
		Input[2]*CubicCoef[128-Index]);

	//    return (Word)(InterPolatedValue&0x3FF);
	return (int16_t)ClipYCbCr_10BIT(InterPolatedValue);
}

inline int16_t CubicInterPolateAudioWord( int16_t *Input, int32_t Index)
{
	int32_t InterPolatedValue;

	InterPolatedValue = FixedTrunc(Input[-1]*CubicCoef[32-Index] + 
		Input[0]*CubicCoef[64-Index] + 
		Input[1]*CubicCoef[96-Index] + 
		Input[2]*CubicCoef[128-Index]);

	if ( InterPolatedValue > 32767 )
		InterPolatedValue = 32767;
	if ( InterPolatedValue < -32767 )
		InterPolatedValue = -32767;

	return (int16_t)(InterPolatedValue&0xFFFF);
}
