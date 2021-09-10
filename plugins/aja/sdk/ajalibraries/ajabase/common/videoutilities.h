/* SPDX-License-Identifier: MIT */
/**
	@file		videoutilities.h
	@brief		Declares the ajabase library's video utility functions.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_VIDEOUTILS_H
#define AJA_VIDEOUTILS_H

#include "public.h"
#include "videotypes.h"

#define DEFAULT_PATT_GAIN  0.9		// some patterns pay attention to this...
#define HD_NUMCOMPONENTPIXELS_2K  2048
#define HD_NUMCOMPONENTPIXELS_1080_2K  2048
#define HD_NUMCOMPONENTPIXELS_1080  1920
#define CCIR601_10BIT_BLACK  64
#define CCIR601_10BIT_WHITE  940
#define CCIR601_10BIT_CHROMAOFFSET  512

#define CCIR601_8BIT_BLACK  16
#define CCIR601_8BIT_WHITE  235
#define CCIR601_8BIT_CHROMAOFFSET  128

#define MIN_RGB_8BIT 0
#define MAX_RGB_8BIT 255
#define MIN_RGB_10BIT 0
#define MAX_RGB_10BIT 1023
#define MIN_RGB_16BIT 0
#define MAX_RGB_16BIT 65535

// line pitch is in bytes.
#define FRAME_0_BASE (0x0)
#define FRAME_1080_10BIT_LINEPITCH (1280*4)
#define FRAME_1080_8BIT_LINEPITCH (1920*2)
#define FRAME_QUADHD_10BIT_SIZE (FRAME_1080_10BIT_LINEPITCH*2160)
#define FRAME_QUADHD_8BIT_SIZE (FRAME_1080_8BIT_LINEPITCH*2160)
#define FRAME_BASE(__frameNum__,__frameSize__)	((__frameNum__)*(__frameSize__))

//	NOTE:	Changed the "(__x__) < MIN_RGB_nBIT" comparisons to "(__x__) <= MIN_RGB_nBIT"
//			in the following three macros to eliminate gcc "comparison always true" warnings
//			when __x__ is an unsigned value.
#if !defined(ClipRGB_8BIT)
	#define ClipRGB_8BIT(__x__)			((__x__) > MAX_RGB_8BIT   ?  (MAX_RGB_8BIT)									\
										                          :  ((__x__) <= MIN_RGB_8BIT   ?  (MIN_RGB_8BIT)	\
										                                                        :  (__x__)))
#endif
#if !defined(ClipRGB_10BIT)
	#define ClipRGB_10BIT(__x__)		((__x__) > MAX_RGB_10BIT  ?  (MAX_RGB_10BIT)								\
										                          :  ((__x__) <= MIN_RGB_10BIT  ?  (MIN_RGB_10BIT)	\
										                                                        :  (__x__)))
#endif

#define MIN_YCBCR_10BIT 4
#define MAX_YCBCR_10BIT 1019
#define ClipYCbCr_10BIT(X) ((X) > MAX_YCBCR_10BIT ? (MAX_YCBCR_10BIT) : ((X) < MIN_YCBCR_10BIT ? (MIN_YCBCR_10BIT) : (X)))


typedef enum { 
	AJA_SIGNALMASK_NONE=0,     // Output Black.
	AJA_SIGNALMASK_Y=1 ,       // Output Y if set, else Output Y=0x40
	AJA_SIGNALMASK_Cb=2 ,      // Output Cb if set, elso Output Cb to 0x200
	AJA_SIGNALMASK_Cr=4 ,       // Output Cr if set, elso Output Cr to 0x200
	AJA_SIGNALMASK_ALL=1+2+4   // Output Cr if set, elso Output Cr to 0x200
} AJASignalMask;

typedef struct {
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;
} AJA_RGBAlphaPixel;

typedef struct {
	uint8_t Alpha;
	uint8_t Red;
	uint8_t Green;
	uint8_t Blue;
} AJA_AlphaRGBPixel;

typedef struct {
	uint16_t Blue;
	uint16_t Green;
	uint16_t Red;
	uint16_t Alpha;
} AJA_RGBAlpha10BitPixel;

typedef struct {
	uint16_t Blue;
	uint16_t Green;
	uint16_t Red;
	uint16_t Alpha;
} AJA_RGBAlpha16BitPixel;

typedef struct
{
	uint16_t Alpha;
	uint16_t cb;
	uint16_t y;
	uint16_t cr;
} AJA_YCbCr10BitAlphaPixel;

typedef struct
{
	uint16_t cb;
	uint16_t y;
	uint16_t cr;
} AJA_YCbCr10BitPixel;

void AJA_EXPORT createVideoFrame( uint32_t *buffer , uint64_t frameNumber, AJA_PixelFormat pixFmt, uint32_t lines, uint32_t pixels, uint32_t linepitch, uint16_t y, uint16_t cb, uint16_t cr );
uint32_t AJA_EXPORT AJA_CalcRowBytesForFormat(AJA_PixelFormat format, uint32_t width);
void AJA_EXPORT AJA_UnPack10BitYCbCrBuffer(uint32_t* packedBuffer, uint16_t* ycbcrBuffer, uint32_t numPixels);
void AJA_EXPORT AJA_PackTo10BitYCbCrBuffer(uint16_t *ycbcrBuffer, uint32_t *packedBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_PackTo10BitYCbCrDPXBuffer( uint16_t *ycbcrBuffer, uint32_t *packedBuffer,uint32_t numPixels ,bool bigEndian = true);
void AJA_EXPORT AJA_PackRGB10BitFor10BitRGB(AJA_RGBAlpha10BitPixel* rgba10BitBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_PackRGB10BitFor10BitRGBPacked(AJA_RGBAlpha10BitPixel* rgba10BitBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_PackRGB10BitFor10BitDPX(AJA_RGBAlpha10BitPixel* rgba10BitBuffer,uint32_t numPixels, bool bigEndian=true);
void AJA_EXPORT AJA_UnPack10BitDPXtoRGBAlpha10BitPixel(AJA_RGBAlpha10BitPixel* rgba10BitBuffer,uint32_t* DPXLinebuffer ,uint32_t numPixels, bool bigEndian=true);
void AJA_EXPORT AJA_UnPack10BitDPXtoRGBAlphaBitPixel(uint8_t* rgbaBuffer,uint32_t* DPXLinebuffer ,uint32_t numPixels, bool bigEndian=true);
void AJA_EXPORT AJA_RePackLineDataForYCbCrDPX(uint32_t *packedycbcrLine, uint32_t numULWords);
void AJA_EXPORT AJA_MakeUnPacked8BitYCbCrBuffer( uint8_t* buffer, uint8_t Y , uint8_t Cb , uint8_t Cr,uint32_t numPixels );
void AJA_EXPORT AJA_MakeUnPacked10BitYCbCrBuffer(uint16_t* buffer, uint16_t Y , uint16_t Cb , uint16_t Cr,uint32_t numPixels);
void AJA_EXPORT AJA_ConvertLineto8BitYCbCr(uint16_t * ycbcr10BitBuffer, uint8_t * ycbcr8BitBuffer,	uint32_t numPixels);
void AJA_EXPORT AJA_ConvertLineToYCbCr422(AJA_RGBAlphaPixel * RGBLine, uint16_t* YCbCrLine, int32_t numPixels , int32_t startPixel,  bool fUseSDMatrix);
void AJA_EXPORT AJA_ConvertLineto10BitRGB(uint16_t * ycbcrBuffer, AJA_RGBAlpha10BitPixel * rgbaBuffer,uint32_t numPixels,bool fUseSDMatrix);
void AJA_EXPORT AJA_ConvertLinetoRGB(uint8_t * ycbcrBuffer, AJA_RGBAlphaPixel * rgbaBuffer, uint32_t numPixels, bool fUseSDMatrix);
void AJA_EXPORT AJA_ConvertLinetoRGB(uint16_t * ycbcrBuffer, AJA_RGBAlphaPixel * rgbaBuffer, uint32_t numPixels, bool fUseSDMatrix);
void AJA_EXPORT AJA_ConvertLineto16BitRGB(uint16_t * ycbcrBuffer, AJA_RGBAlpha16BitPixel * rgbaBuffer, uint32_t numPixels, bool fUseSDMatrix);
void AJA_EXPORT AJA_Convert16BitRGBtoBayer10BitDPXLJ(AJA_RGBAlpha16BitPixel * rgbaBuffer, uint32_t * bayerBuffer, 
													 uint32_t numPixels, uint32_t line, AJA_BayerColorPhase phase = AJA_BayerColorPhase_RedGreen);
void AJA_EXPORT AJA_Convert16BitRGBtoBayer12BitDPXLJ(AJA_RGBAlpha16BitPixel * rgbaBuffer, uint32_t * bayerBuffer, 
													 uint32_t numPixels, uint32_t line, AJA_BayerColorPhase phase = AJA_BayerColorPhase_RedGreen);
void AJA_EXPORT AJA_Convert16BitRGBtoBayer10BitDPXPacked(AJA_RGBAlpha16BitPixel * rgbaBuffer, uint8_t * bayerBuffer, 
														 uint32_t numPixels, uint32_t line, AJA_BayerColorPhase phase = AJA_BayerColorPhase_RedGreen);
void AJA_EXPORT AJA_Convert16BitRGBtoBayer12BitDPXPacked(AJA_RGBAlpha16BitPixel * rgbaBuffer, uint8_t * bayerBuffer, 
														 uint32_t numPixels, uint32_t line, AJA_BayerColorPhase phase = AJA_BayerColorPhase_RedGreen);
void AJA_EXPORT AJA_ConvertARGBToRGBA(uint8_t* rgbaBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_ConvertARGBToABGR(uint8_t* rgbaBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_ConvertARGBToRGB(uint8_t* rgbaBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_ConvertARGBToBGR(uint8_t* rgbaBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_Convert16BitARGBTo16BitRGB(AJA_RGBAlpha16BitPixel *rgbaLineBuffer ,uint16_t * rgbLineBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_Convert16BitARGBTo12BitRGBPacked(AJA_RGBAlpha16BitPixel *rgbaLineBuffer ,uint8_t * rgbLineBuffer,uint32_t numPixels);
void AJA_EXPORT AJA_Convert8BitYCbCrToYUY2(uint8_t * ycbcrBuffer, uint32_t numPixels);
void AJA_EXPORT AJA_ConvertUnpacked10BitYCbCrToPixelFormat(uint16_t *unPackedBuffer, uint32_t *packedBuffer, uint32_t numPixels, AJA_PixelFormat pixelFormat);
void AJA_EXPORT AJA_ConvertPixelFormatToRGBA(uint32_t *buffer, AJA_RGBAlphaPixel* rgbBuffer, uint32_t numPixels, AJA_PixelFormat pixelFormat,bool  bIsSD = false);
void AJA_EXPORT AJA_MaskUnPacked10BitYCbCrBuffer(uint16_t* ycbcrUnPackedBuffer, uint16_t signalMask , uint32_t numPixels);

void AJA_EXPORT AJA_ReSampleLine(AJA_RGBAlphaPixel *Input, AJA_RGBAlphaPixel *Output, uint16_t startPixel, uint16_t endPixel, int32_t numInputPixels, int32_t numOutputPixels);
void AJA_EXPORT AJA_ReSampleLine(int16_t *Input, int16_t *Output, uint16_t startPixel, uint16_t endPixel, int32_t numInputPixels, int32_t numOutputPixels);
void AJA_EXPORT AJA_ReSampleYCbCrSampleLine(int16_t *Input, int16_t *Output, int32_t numInputPixels, int32_t numOutputPixels); 
void AJA_EXPORT AJA_ReSampleAudio(int16_t *Input, int16_t *Output, uint16_t startPixel, uint16_t endPixel, int32_t numInputPixels, int32_t numOutputPixels, int16_t channelInterleaveMulitplier=1);

void AJA_EXPORT WriteLineToBuffer(AJA_PixelFormat pixelFormat, uint32_t currentLine, uint32_t numPixels, uint32_t linePitch, 
								  uint8_t* pOutputBuffer,uint32_t* pPackedLineBuffer);
void AJA_EXPORT WriteLineToBuffer(AJA_PixelFormat pixelFormat, AJA_BayerColorPhase bayerPhase, uint32_t currentLine,
								  uint32_t numPixels, uint32_t linePitch, uint8_t* pOutputBuffer,uint32_t* pPackedLineBuffer);

void AJA_EXPORT AJA_ConvertRGBAlpha10LineToYCbCr422(AJA_RGBAlpha10BitPixel * RGBLine,
                                                                                 uint16_t* YCbCrLine,
                                                                                 int32_t numPixels ,
                                                                                 int32_t startPixel,
                                                                                 bool fUseRGBFullRange=false);

inline	int16_t	AJA_FixedRound(int32_t inFix)
{ 
  int16_t retValue;
  
  if ( inFix < 0 ) 
  {
	retValue = (int16_t)(-((-inFix+0x8000)>>16));
  }
  else
  {
	retValue = (int16_t)((inFix + 0x8000)>>16);
  }
  return retValue;
}

inline 	void AJA_SDConvert10BitYCbCrto10BitRGB(AJA_YCbCr10BitAlphaPixel *pSource,
										   AJA_RGBAlpha10BitPixel *pTarget)
{
  int32_t Red,Green,Blue;
  int32_t ConvertedY;

  ConvertedY = 0x12A15*((int32_t)pSource->y - CCIR601_10BIT_BLACK);

  Red = AJA_FixedRound(ConvertedY +
		   0x19895*((int32_t)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET)));

  pTarget->Red = (uint16_t)ClipRGB_10BIT(Red);

  Blue = AJA_FixedRound(ConvertedY +
		    0x20469*((int32_t)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Blue = (uint16_t)ClipRGB_10BIT(Blue);

  Green = AJA_FixedRound(ConvertedY - 
		     0x644A*((int32_t)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
		     0xD01F*((int32_t)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Green = (uint16_t)ClipRGB_10BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

inline 	void AJA_HDConvert10BitYCbCrto10BitRGB(AJA_YCbCr10BitAlphaPixel *pSource,
										   AJA_RGBAlpha10BitPixel *pTarget)
{
  int32_t Red,Green,Blue;
  int32_t ConvertedY;

  ConvertedY = 0x12ACF*((int32_t)pSource->y - CCIR601_10BIT_BLACK);

  Red = AJA_FixedRound(ConvertedY +
		   0x1DF71*((int32_t)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET)));

  pTarget->Red = (uint16_t)ClipRGB_10BIT(Red);

  Blue = AJA_FixedRound(ConvertedY +
		    0x22A86*((int32_t)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Blue = (uint16_t)ClipRGB_10BIT(Blue);

  Green = AJA_FixedRound(ConvertedY - 
		     0x3806*((int32_t)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
		     0x8C32*((int32_t)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Green = (uint16_t)ClipRGB_10BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

inline 	void AJA_SDConvert10BitYCbCrtoRGB(AJA_YCbCr10BitAlphaPixel *pSource,
								      AJA_RGBAlphaPixel *pTarget)
{
  int32_t Red,Green,Blue;
  int32_t ConvertedY;

  ConvertedY = 0x4A86*((int32_t)pSource->y - CCIR601_10BIT_BLACK);

  Red = AJA_FixedRound(ConvertedY +
		   0x6626*((int32_t)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET)));

  pTarget->Red = (uint8_t)ClipRGB_8BIT(Red);

  Blue = AJA_FixedRound(ConvertedY +
		    0x811B*((int32_t)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Blue = (uint8_t)ClipRGB_8BIT(Blue);

  Green = AJA_FixedRound(ConvertedY - 
		     0x1913*((int32_t)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
		     0x3408*((int32_t)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Green = (uint8_t)ClipRGB_8BIT(Green);

  pTarget->Alpha = (uint8_t)pSource->Alpha;
}

inline 	void AJA_HDConvert10BitYCbCrtoRGB(AJA_YCbCr10BitAlphaPixel *pSource,
									  AJA_RGBAlphaPixel *pTarget)
{
  int32_t Red,Green,Blue;
  int32_t ConvertedY;

  ConvertedY = (0x12ACF>>2)*((int32_t)pSource->y - CCIR601_10BIT_BLACK);

  Red = AJA_FixedRound(ConvertedY +
		   (0x1DF71>>2)*((int32_t)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET)));

  pTarget->Red = (uint8_t)ClipRGB_8BIT(Red);

  Blue = AJA_FixedRound(ConvertedY +
		    (0x22A86>>2)*((int32_t)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Blue = (uint8_t)ClipRGB_8BIT(Blue);

  Green = AJA_FixedRound(ConvertedY - 
		     (0x3806>>2)*((int32_t)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
		     (0x8C32>>2)*((int32_t)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Green = (uint8_t)ClipRGB_8BIT(Green);

  pTarget->Alpha = (uint8_t)pSource->Alpha;
}

typedef void (*AJA_ConvertRGBAlphatoYCbCr)(AJA_RGBAlphaPixel * pSource, AJA_YCbCr10BitPixel * pTarget);

inline void AJA_SDConvertRGBAlphatoYCbCr(AJA_RGBAlphaPixel * pSource, AJA_YCbCr10BitPixel * pTarget)
{
	int32_t Y,Cb,Cr;

	Y = CCIR601_10BIT_BLACK + (((int32_t)0x41BC*pSource->Red +
		(int32_t)0x810F*pSource->Green +
		(int32_t)0x1910*pSource->Blue )>>14);
	pTarget->y = (uint16_t)Y;

	Cb = CCIR601_10BIT_CHROMAOFFSET + (((int32_t)-0x25F1*pSource->Red -
		(int32_t)0x4A7E*pSource->Green +
		(int32_t)0x7070*pSource->Blue )>>14);

	pTarget->cb = uint16_t(Cb&0x3FF);

	Cr = CCIR601_10BIT_CHROMAOFFSET + (((int32_t)0x7070*pSource->Red -
		(int32_t)0x5E27*pSource->Green -
		(int32_t)0x1249*pSource->Blue )>>14);

	pTarget->cr = uint16_t(Cr&0x3FF);
}

inline void AJA_HDConvertRGBAlphatoYCbCr(AJA_RGBAlphaPixel * pSource, AJA_YCbCr10BitPixel * pTarget)
{
	int32_t Y,Cb,Cr;

	Y = CCIR601_10BIT_BLACK + (((int32_t)0x2E8A*pSource->Red +
		(int32_t)0x9C9F*pSource->Green +
		(int32_t)0x0FD2*pSource->Blue )>>14);
	pTarget->y = (uint16_t)Y;

	Cb = CCIR601_10BIT_CHROMAOFFSET + (((int32_t)-0x18F4*pSource->Red -
		(int32_t)0x545B*pSource->Green +
		(int32_t)0x6DA9*pSource->Blue )>>14);

	pTarget->cb = uint16_t(Cb&0x3FF);

	Cr = CCIR601_10BIT_CHROMAOFFSET + (((int32_t)0x6D71*pSource->Red -
		(int32_t)0x6305*pSource->Green -
		(int32_t)0x0A06*pSource->Blue )>>14);

	pTarget->cr = uint16_t(Cr&0x3FF);
}

inline void AJA_HDConvertRGBAlpha10toYCbCr(AJA_RGBAlpha10BitPixel * pSource, AJA_YCbCr10BitPixel * pTarget,bool rgbFullRange)
{
	double dY,dCb,dCr;
	int32_t Y,Cb,Cr;
	if ( rgbFullRange)
	{
		dY =	(double(pSource->Red  ) * 0.182068) +
				(double(pSource->Green) * 0.612427) +
				(double(pSource->Blue ) * 0.061829);
		Y = CCIR601_10BIT_BLACK + int32_t(dY); /// should do rounding
		pTarget->y = uint16_t(Y&0x3FF);

		dCb =	(double(pSource->Red  ) * (-0.100342)) +
				(double(pSource->Green) * (-0.337585)) +
				(double(pSource->Blue ) * (0.437927));
		Cb = CCIR601_10BIT_CHROMAOFFSET + int32_t(dCb);
		pTarget->cb = uint16_t(Cb&0x3FF);

		dCr =	(double(pSource->Red  ) * (0.437927)) +
				(double(pSource->Green) * (-0.397766)) +
				(double(pSource->Blue ) * (-0.040161));
		Cr = CCIR601_10BIT_CHROMAOFFSET + int32_t(dCr);
		pTarget->cr = uint16_t(Cr&0x3FF);

	}
	else
	{
		dY =	(double(pSource->Red  ) * 0.212585) +
				(double(pSource->Green) * 0.715210) +
				(double(pSource->Blue ) * 0.072205);
		Y = int32_t(dY);
		pTarget->y = uint16_t(Y&0x3FF);

		dCb =	(double(pSource->Red  ) * (-0.117188)) +
				(double(pSource->Green) * (-0.394226)) +
				(double(pSource->Blue ) * (0.511414));
		Cb = CCIR601_10BIT_CHROMAOFFSET + int32_t(dCb);
		pTarget->cb = uint16_t(Cb&0x3FF);

		dCr =	(double(pSource->Red  ) * (0.511414)) +
				(double(pSource->Green) * (-0.464508)) +
				(double(pSource->Blue ) * (-0.046906));
		Cr = CCIR601_10BIT_CHROMAOFFSET + int32_t(dCr);
		pTarget->cr = uint16_t(Cr&0x3FF);
	}
}

#endif	//	AJA_VIDEOUTILS_H
