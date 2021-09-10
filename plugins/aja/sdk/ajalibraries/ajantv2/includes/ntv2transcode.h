/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2transcode.h
	@brief		Declares a number of pixel format transcoder functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2TRANSCODE_H
#define NTV2TRANSCODE_H
#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2fixed.h"
#include "ntv2videodefines.h"
#include <vector>

// TODO: DEBUG ONLY
#include <stdio.h>


/**
	@brief		Converts a single 8-bit YCbCr '2vuy' raster line to 10-bit YCbCr 'v210' (from NTV2_FBF_8BIT_YCBCR to NTV2_FBF_10BIT_YCBCR ).
	@param[in]	pInSrcLine_2vuy		Specifies a valid, non-NULL address of the first byte of the '2vuy'
									raster line to be converted.
	@param[out]	pOutDstLine_v210	Specifies a valid, non-NULL address of the first byte of the 'v210'
									raster line to receive the converted data.
	@param[in]	inNumPixels			The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_2vuy_to_v210 (const UByte * pInSrcLine_2vuy, ULWord * pOutDstLine_v210, const ULWord inNumPixels);

/**
	@brief		Converts a single 8-bit YCbCr '2vuy' raster line to 10-bit YCbCr 'v210' (from NTV2_FBF_8BIT_YCBCR to NTV2_FBF_8BIT_YCBCR_YUY2 ).
	@param[in]	pInSrcLine_2vuy		Specifies a valid, non-NULL address of the first byte of the '2vuy'
									raster line to be converted.
	@param[out]	pOutDstLine_yuy2	Specifies a valid, non-NULL address of the first byte of the 'YUY2'
									raster line to receive the converted data.
	@param[in]	inNumPixels			The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_2vuy_to_yuy2 (const UByte * pInSrcLine_2vuy, UWord * pOutDstLine_yuy2, const ULWord inNumPixels);

/**
	@brief		Converts a single 10-bit YCbCr 'v210' raster line to 8-bit YCbCr '2vuy'.
	@param[in]	pInSrcLine_v210		Specifies a valid, non-NULL address of the first byte of the 'v210'
									raster line to be converted.
	@param[out]	pOutDstLine_2vuy	Specifies a valid, non-NULL address of the first byte of the '2vuy'
									raster line to receive the converted data.
	@param[in]	inNumPixels			The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_v210_to_2vuy (const ULWord * pInSrcLine_v210, UByte * pOutDstLine_2vuy, const ULWord inNumPixels);

/**
	@brief		Converts a single 10-bit YCbCr 'v210' raster line to 8-bit YCbCr '2vuy' vector.
	@param[in]	pInSrcLine_v210		Specifies a valid, non-NULL address of the first byte of the 'v210' raster line to be converted.
	@param[out]	outDstLine2vuy		Receives the 8-bit '2vuy' vector. Will be cleared upon entry and filled upon exit.
	@param[in]	inNumPixels			The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_v210_to_2vuy (const void * pInSrcLine_v210, std::vector<uint8_t> & outDstLine2vuy, const ULWord inNumPixels);

/**
	@brief		Converts a single 8-bit ABGR raster line to 10-bit ABGR (from NTV2_FBF_ABGR to NTV2_FBF_10BIT_RGB ).
	@param[in]	pInSrcLine_8bitABGR		Specifies a valid, non-NULL address of the first byte of the 8-bit ABGR raster line to be converted.
	@param[out]	pOutDstLine_10BitABGR	Specifies a valid, non-NULL address of the first byte of the 10-bit ABGR raster line to receive the converted data.
	@param[in]	inNumPixels				The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_8bitABGR_to_10bitABGR (const UByte * pInSrcLine_8bitABGR,  ULWord * pOutDstLine_10BitABGR, const ULWord inNumPixels);

/**
	@brief		Converts a single 8-bit ABGR raster line to 10-bit RGB DPX (from NTV2_FBF_ABGR to NTV2_FBF_10BIT_DPX ).
	@param[in]	pInSrcLine_8bitABGR		Specifies a valid, non-NULL address of the first byte of the 8-bit ABGR raster line to be converted.
	@param[out]	pOutDstLine_10BitDPX	Specifies a valid, non-NULL address of the first byte of the 10-bit RGB DPX raster line to receive the converted data.
	@param[in]	inNumPixels				The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_8bitABGR_to_10bitRGBDPX (const UByte * pInSrcLine_8bitABGR,  ULWord * pOutDstLine_10BitDPX, const ULWord inNumPixels);

/**
	@brief		Converts a single 8-bit ABGR raster line to 10-bit RGB DPX (from NTV2_FBF_ABGR to NTV2_FBF_10BIT_DPX_LE ).
	@param[in]	pInSrcLine_8bitABGR		Specifies a valid, non-NULL address of the first byte of the 8-bit ABGR raster line to be converted.
	@param[out]	pOutDstLine_10BitDPXLE	Specifies a valid, non-NULL address of the first byte of the 10-bit RGB DPX LE raster line to receive the converted data.
	@param[in]	inNumPixels				The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_8bitABGR_to_10bitRGBDPXLE (const UByte * pInSrcLine_8bitABGR,  ULWord * pOutDstLine_10BitDPXLE, const ULWord inNumPixels);

/**
	@brief		Converts a single 8-bit ABGR raster line to 24-bit RGB (from NTV2_FBF_ABGR to NTV2_FBF_24BIT_RGB ).
	@param[in]	pInSrcLine_8bitABGR		Specifies a valid, non-NULL address of the first byte of the 8-bit ABGR raster line to be converted.
	@param[out]	pOutDstLine_24BitRGB	Specifies a valid, non-NULL address of the first byte of the 24-bit RGB raster line to receive the converted data.
	@param[in]	inNumPixels				The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_8bitABGR_to_24bitRGB (const UByte * pInSrcLine_8bitABGR,  UByte * pOutDstLine_24BitRGB, const ULWord inNumPixels);

/**
	@brief		Converts a single 8-bit ABGR raster line to 24-bit BGR (from NTV2_FBF_ABGR to NTV2_FBF_24BIT_BGR ).
	@param[in]	pInSrcLine_8bitABGR		Specifies a valid, non-NULL address of the first byte of the 8-bit ABGR raster line to be converted.
	@param[out]	pOutDstLine_24BitBGR	Specifies a valid, non-NULL address of the first byte of the 24-bit BGR raster line to receive the converted data.
	@param[in]	inNumPixels				The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_8bitABGR_to_24bitBGR (const UByte * pInSrcLine_8bitABGR,  UByte * pOutDstLine_24BitBGR, const ULWord inNumPixels);

/**
	@brief		Converts a single 8-bit ABGR raster line to 48-bit RGB (from NTV2_FBF_ABGR to NTV2_FBF_48BIT_RGB ).
	@param[in]	pInSrcLine_8bitABGR		Specifies a valid, non-NULL address of the first byte of the 8-bit ABGR raster line to be converted.
	@param[out]	pOutDstLine_48BitRGB	Specifies a valid, non-NULL address of the first byte of the 48-bit RGB raster line to receive the converted data.
	@param[in]	inNumPixels				The number of pixels to be converted.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	ConvertLine_8bitABGR_to_48bitRGB (const UByte * pInSrcLine_8bitABGR,  ULWord * pOutDstLine_48BitRGB, const ULWord inNumPixels);


// ConvertLineToYCbCr422
// 8 Bit
AJAExport void	ConvertLineToYCbCr422 (RGBAlphaPixel *	RGBLine,
									   UByte *			YCbCrLine,
									   LWord			numPixels ,
									   LWord			startPixel,
									   bool				fUseSDMatrix);

// ConvertLineToYCbCr422
// 10 Bit
AJAExport void	ConvertLineToYCbCr422 (RGBAlphaPixel *	RGBLine,
									   UWord *			YCbCrLine,
									   LWord			numPixels,
									   LWord			startPixel,
									   bool				fUseSDMatrix);

// ConvertLinetoRGB
// 8 Bit Version
AJAExport void	ConvertLinetoRGB (UByte *			ycbcrBuffer,
								  RGBAlphaPixel *	rgbaBuffer ,
								  ULWord			numPixels,
								  bool				fUseSDMatrix,
								  bool				fUseSMPTERange	= false);

// ConvertLinetoRGB
// 10 Bit Version
AJAExport void	ConvertLinetoRGB (UWord *			ycbcrBuffer,
								  RGBAlphaPixel *	rgbaBuffer,
								  ULWord			numPixels,
								  bool				fUseSDMatrix,
								  bool				fUseSMPTERange	= false,
								  bool				fAlphaFromLuma	= false);

// ConvertLineto10BitRGB
// 10 Bit YCbCr and 10 Bit RGB Version
AJAExport void	ConvertLineto10BitRGB (UWord *				ycbcrBuffer,
									  RGBAlpha10BitPixel *	rgbaBuffer,
									  ULWord				numPixels,
									  bool					fUseSDMatrix,
									  bool					fUseSMPTERange	= false);
// ConvertLineto10BitYCbCrA
// 10 Bit YCbCr to 10 Bit YCbCrA
AJAExport void	ConvertLineto10BitYCbCrA (const UWord *	pInYCbCrBuffer,
										  ULWord *		pOutYCbCrABuffer,
										  const ULWord	inNumPixels);

// ConvertRGBLineto10BitRGB
// 8 Bit RGB and 10 Bit RGB Version
AJAExport void	ConvertRGBLineto10BitRGB (const RGBAlphaPixel *	pInRGBA8Buffer,
										  RGBAlpha10BitPixel *	pOutRGBA10Buffer,
										  const ULWord			inNumPixels);

// ConvertRGBALineToRGB
// 8 bit RGBA to 8 bit RGB (RGB24)
// Conversion is done into the same buffer
AJAExport void	ConvertRGBALineToRGB (RGBAlphaPixel *	rgbaBuffer,
									  ULWord			numPixels);

// ConvertRGBALineToBGR
// 8 bit RGBA to 8 bit BGR (BGR24)
// Conversion is done into the same buffer
AJAExport void	ConvertRGBALineToBGR (RGBAlphaPixel *	rgbaBuffer,
									  ULWord			numPixels);

// ConvertLineto10BitRGB
// 8 Bit RGBA to  and 10 Bit RGB Packed Version
AJAExport void	ConvertLineto10BitRGB (const RGBAlphaPixel *	pInRGBA8Buffer,
										ULWord *		pOutRGB10BitBuffer,
										ULWord			inNumPixels);

// ConvertLineto8BitYCbCr
// 10 Bit YCbCr to 8 Bit YCbCr
AJAExport void	ConvertLineto8BitYCbCr (UWord *	ycbcr10BitBuffer,
										UByte *	ycbcr8BitBuffer,
										ULWord	numPixels);

// Converts UYVY(2yuv) -> YUY2(yuv2) in place
AJAExport void	Convert8BitYCbCrToYUY2 (UByte *	ycbcrBuffer,
									  	ULWord	numPixels);

// Converts 8 Bit ARGB 8 Bit RGBA in place
AJAExport void	ConvertARGBYCbCrToRGBA (UByte *	rgbaBuffer,
										ULWord	numPixels);


// Converts 8 Bit ARGB 8 Bit ABGR in place
AJAExport void	ConvertARGBYCbCrToABGR (UByte *	rgbaBuffer,
										ULWord	numPixels);

// Convert 8 Bit ARGB to 8 bit RGB
AJAExport void	ConvertARGBToRGB (UByte *	rgbaLineBuffer,
									UByte *	rgbLineBuffer,
									ULWord	numPixels);

// Convert 16 Bit ARGB to 16 bit RGB
AJAExport void	Convert16BitARGBTo16BitRGBEx (UWord *	rgbaLineBuffer,
												UWord *	rgbLineBuffer,
												ULWord	numPixels);

// Convert 16 Bit ARGB to 16 bit RGB
AJAExport void	Convert16BitARGBTo16BitRGB (RGBAlpha16BitPixel *	rgbaLineBuffer,
											UWord *					rgbLineBuffer,
											ULWord					numPixels);

// Convert 16 Bit ARGB to 12 bit RGB Packed	//	New in SDK 15.5
AJAExport void Convert16BitARGBTo12BitRGBPacked(RGBAlpha16BitPixel *rgbaLineBuffer ,
												UByte * rgbLineBuffer,
												ULWord numPixels);

// Convert 8 Bit ARGB to 8 bit BGR
AJAExport void	ConvertARGBToBGR (const UByte *		pInRGBALineBuffer,
									UByte *			pOutRGBLineBuffer,
									const ULWord	inNumPixels);

// ConvertLineto16BitRGB
// 10 Bit YCbCr and 16 Bit RGB Version
AJAExport void	ConvertLineto16BitRGB (UWord *	 				ycbcrBuffer,
										RGBAlpha16BitPixel *	rgbaBuffer,
										ULWord					numPixels,
										bool					fUseSDMatrix,
										bool					fUseSMPTERange	= false);

/*
// ConvertLinetoRGB
// 10 Bit YCbCr Version -> 10 Bit RGB
AJAExport
void ConvertLinetoRGB(UWord * ycbcrBuffer, 
					  RGBAlpha10BitPixel * rgbaBuffer,
					  ULWord numPixels);
*/

// Pack 10 Bit RGBA to 10 Bit RGB Format for our board
AJAExport void	PackRGB10BitFor10BitRGB (RGBAlpha10BitPixel *	pBuffer,
										const ULWord					inNumPixels);

// Pack 10 Bit RGBA to 10 Bit DPX Format for our board
AJAExport void	PackRGB10BitFor10BitDPX (RGBAlpha10BitPixel *	pBuffer,
										const ULWord			inNumPixels,
										const bool				bigEndian	= true);

// Pack 10 Bit RGBA to NTV2_FBF_10BIT_RGB_PACKED Format for our board
AJAExport void	PackRGB10BitFor10BitRGBPacked (RGBAlpha10BitPixel *	pBuffer,
												const ULWord		inNumPixels);


inline void SDConvertRGBAlphatoYCbCr(RGBAlphaPixel * pSource, YCbCrPixel * pTarget)
{
  LWord Y,Cb,Cr;

  Y = CCIR601_8BIT_BLACK + FixedRound((Fixed_)0x41BC*pSource->Red +
					                  (Fixed_)0x810F*pSource->Green +
					                  (Fixed_)0x1910*pSource->Blue );
  pTarget->y = (UByte)Y;

  Cb = CCIR601_8BIT_CHROMAOFFSET + FixedRound((Fixed_)-0x25F1*pSource->Red -
					                          (Fixed_)0x4A7E*pSource->Green +
					                          (Fixed_)0x7070*pSource->Blue );

  pTarget->cb = (UByte)Cb;

  Cr = CCIR601_8BIT_CHROMAOFFSET + FixedRound((Fixed_)0x7070*pSource->Red -
					                          (Fixed_)0x5E27*pSource->Green -
					                          (Fixed_)0x1249*pSource->Blue );

  pTarget->cr = (UByte)Cr;
}

inline void HDConvertRGBAlphatoYCbCr(RGBAlphaPixel * pSource, YCbCrPixel * pTarget)
{
  LWord Y,Cb,Cr;

  Y = CCIR601_8BIT_BLACK + FixedRound((Fixed_)0x2E8A*pSource->Red +
					                  (Fixed_)0x9C9F*pSource->Green +
					                  (Fixed_)0x0FD2*pSource->Blue );
  pTarget->y = UByte(Y);

  Cb = CCIR601_8BIT_CHROMAOFFSET + FixedRound((Fixed_)-0x18F4*pSource->Red -
					                          (Fixed_)0x545B*pSource->Green +
					                          (Fixed_)0x6DA9*pSource->Blue );

  pTarget->cb = uint8_t(SByte(Cb));

  Cr = CCIR601_8BIT_CHROMAOFFSET + FixedRound((Fixed_)0x6D71*pSource->Red -
					                          (Fixed_)0x6305*pSource->Green -
					                          (Fixed_)0x0A06*pSource->Blue );

  pTarget->cr = uint8_t(SByte(Cr));
}

 
inline void SDConvertRGBAlphatoYCbCr(const RGBAlphaPixel * pSource, YCbCr10BitPixel * pTarget)
{
  LWord Y,Cb,Cr;

  Y = CCIR601_10BIT_BLACK + (((Fixed_)0x41BC*pSource->Red +
					          (Fixed_)0x810F*pSource->Green +
					          (Fixed_)0x1910*pSource->Blue )>>14);
  pTarget->y = UWord(Y);

  Cb = CCIR601_10BIT_CHROMAOFFSET + (((Fixed_)-0x25F1*pSource->Red -
					                  (Fixed_)0x4A7E*pSource->Green +
					                  (Fixed_)0x7070*pSource->Blue )>>14);

  pTarget->cb = UWord(Cb&0x3FF);

  Cr = CCIR601_10BIT_CHROMAOFFSET + (((Fixed_)0x7070*pSource->Red -
					                  (Fixed_)0x5E27*pSource->Green -
					                  (Fixed_)0x1249*pSource->Blue )>>14);

  pTarget->cr = UWord(Cr&0x3FF);
}

inline void HDConvertRGBAlphatoYCbCr(const RGBAlphaPixel * pSource, YCbCr10BitPixel * pTarget)
{
  LWord Y,Cb,Cr;

  Y = CCIR601_10BIT_BLACK + (((Fixed_)0x2E8A*pSource->Red +
					          (Fixed_)0x9C9F*pSource->Green +
					          (Fixed_)0x0FD2*pSource->Blue )>>14);
  pTarget->y = (UWord)Y;

  Cb = CCIR601_10BIT_CHROMAOFFSET + (((Fixed_)-0x18F4*pSource->Red -
					                  (Fixed_)0x545B*pSource->Green +
					                  (Fixed_)0x6DA9*pSource->Blue )>>14);

  pTarget->cb = UWord(Cb&0x3FF);

  Cr = CCIR601_10BIT_CHROMAOFFSET + (((Fixed_)0x6D71*pSource->Red -
					                  (Fixed_)0x6305*pSource->Green -
					                  (Fixed_)0x0A06*pSource->Blue )>>14);

  pTarget->cr = UWord(Cr&0x3FF);
}
 
inline 	void SDConvertYCbCrtoRGB(YCbCrAlphaPixel *pSource,
										   RGBAlphaPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0x12A15*((LWord)pSource->y - CCIR601_8BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x19895*((LWord)(pSource->cr-CCIR601_8BIT_CHROMAOFFSET)));

  pTarget->Red = (UByte)ClipRGB_8BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x20469*((LWord)(pSource->cb-CCIR601_8BIT_CHROMAOFFSET) ));

  pTarget->Blue = (UByte)ClipRGB_8BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x644A*((LWord)(pSource->cb-CCIR601_8BIT_CHROMAOFFSET) ) -
		     0xD01F*((LWord)(pSource->cr-CCIR601_8BIT_CHROMAOFFSET) ));

  pTarget->Green = (UByte)ClipRGB_8BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

inline 	void SDConvert10BitYCbCrtoRGB(const YCbCr10BitAlphaPixel *pSource,
										        RGBAlphaPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0x4A86*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x6626*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET)));

  pTarget->Red = (UByte)ClipRGB_8BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x811B*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Blue = (UByte)ClipRGB_8BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x1913*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
		     0x3408*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Green = (UByte)ClipRGB_8BIT(Green);

  pTarget->Alpha = (UByte)pSource->Alpha;
}

inline 	void HDConvertYCbCrtoRGB(YCbCrAlphaPixel *pSource,
										   RGBAlphaPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0x12ACF*((LWord)pSource->y - CCIR601_8BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x1DF71*((LWord)(pSource->cr-CCIR601_8BIT_CHROMAOFFSET)));

  pTarget->Red = (UByte)ClipRGB_8BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x22A86*((LWord)(pSource->cb-CCIR601_8BIT_CHROMAOFFSET) ));

  pTarget->Blue = (UByte)ClipRGB_8BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x3806*((LWord)(pSource->cb-CCIR601_8BIT_CHROMAOFFSET) ) -
		     0x8C32*((LWord)(pSource->cr-CCIR601_8BIT_CHROMAOFFSET) ));

  pTarget->Green = (UByte)ClipRGB_8BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

inline 	void HDConvert10BitYCbCrtoRGB(const YCbCr10BitAlphaPixel *pSource,
										        RGBAlphaPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = (0x12ACF>>2)*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   (0x1DF71>>2)*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET)));

  pTarget->Red = (UByte)ClipRGB_8BIT(Red);

  Blue = FixedRound(ConvertedY +
		    (0x22A86>>2)*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Blue = (UByte)ClipRGB_8BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     (0x3806>>2)*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
		     (0x8C32>>2)*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Green = (UByte)ClipRGB_8BIT(Green);

  pTarget->Alpha = (UByte)pSource->Alpha;
}


inline 	void SDConvert10BitYCbCrto10BitRGB(YCbCr10BitAlphaPixel *pSource,
										   RGBAlpha10BitPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0x12A15*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x19895*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET)));

  pTarget->Red = (UWord)ClipRGB_10BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x20469*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Blue = (UWord)ClipRGB_10BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x644A*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
		     0xD01F*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Green = (UWord)ClipRGB_10BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

inline 	void HDConvert10BitYCbCrto10BitRGB(YCbCr10BitAlphaPixel *pSource,
										   RGBAlpha10BitPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0x12ACF*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x1DF71*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET)));

  pTarget->Red = (UWord)ClipRGB_10BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x22A86*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Blue = (UWord)ClipRGB_10BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x3806*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
		     0x8C32*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) ));

  pTarget->Green = (UWord)ClipRGB_10BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

// KAM - start
inline 	void SDConvert10BitYCbCrto16BitRGB(YCbCr10BitAlphaPixel *pSource,
										   RGBAlpha16BitPixel *pTarget)
{
  ULWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0x12A15*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = ULWord(FixedRound(ConvertedY +
				0x19895*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))));

  //Red = Red<<4; // only 12 bits used, put them in the MSB
  pTarget->Red = (UWord)ClipRGB_16BIT(Red<<6); // TBD: fix coefficents instead

  Blue = ULWord(FixedRound(ConvertedY +
				0x20469*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) )));

  //Blue = Blue<<4; // only 12 bits used, put them in the MSB
  pTarget->Blue = (UWord)ClipRGB_16BIT(Blue<<6); // TBD: fix coefficents instead

  Green = ULWord(FixedRound(ConvertedY -
				0x644A*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
				0xD01F*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) )));

  //Green = Green<<4; // only 12 bits used, put them in the MSB
  pTarget->Green = (UWord)ClipRGB_16BIT(Green<<6); // TBD: fix coefficents instead

  // TBD: shift alpha???
  pTarget->Alpha = pSource->Alpha;
}

inline 	void HDConvert10BitYCbCrto16BitRGB(YCbCr10BitAlphaPixel *pSource,
										   RGBAlpha16BitPixel *pTarget)
{
  ULWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0x12ACF*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = ULWord(FixedRound(ConvertedY +
				0x1DF71*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))));

  //Red = Red<<4; // only 12 bits used, put them in the MSB
  pTarget->Red = (UWord)ClipRGB_16BIT(Red); // TBD: fix coefficents instead

  Blue = ULWord(FixedRound(ConvertedY +
				0x22A86*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) )));

  //Blue = Blue<<4; // only 12 bits used, put them in the MSB
  pTarget->Blue = (UWord)ClipRGB_16BIT(Blue); // TBD: fix coefficents instead

  Green = ULWord(FixedRound(ConvertedY -
				0x3806*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET) ) -
				0x8C32*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET) )));

  //Green = Green<<4; // only 12 bits used, put them in the MSB
  pTarget->Green = (UWord)ClipRGB_16BIT(Green); // TBD: fix coefficents instead

  // TBD: shift alpha???
  pTarget->Alpha = pSource->Alpha;
}

inline 	void SDConvertYCbCrtoRGBSmpte(YCbCrAlphaPixel *pSource,
										   RGBAlphaPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0xFF40*((LWord)pSource->y - CCIR601_8BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x15DDF*((LWord)(pSource->cr-CCIR601_8BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Red = (UByte)ClipRGB_8BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x1BA34*((LWord)(pSource->cb-CCIR601_8BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Blue = (UByte)ClipRGB_8BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x55E1*((LWord)(pSource->cb-CCIR601_8BIT_CHROMAOFFSET)) -
		     0xB237*((LWord)(pSource->cr-CCIR601_8BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Green = (UByte)ClipRGB_8BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

inline 	void SDConvert10BitYCbCrtoRGBSmpte(YCbCr10BitAlphaPixel *pSource,
										        RGBAlphaPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0x3FD1*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x5778*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Red = (UByte)ClipRGB_8BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x6E8E*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Blue = (UByte)ClipRGB_8BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x1579*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET)) -
		     0x2C8E*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Green = (UByte)ClipRGB_8BIT(Green);

  pTarget->Alpha = (UByte)pSource->Alpha;
}

inline 	void HDConvertYCbCrtoRGBSmpte(YCbCrAlphaPixel *pSource,
										   RGBAlphaPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0xFFDF*((LWord)pSource->y - CCIR601_8BIT_BLACK);

  Red = FixedRound(ConvertedY + 
		   0x19A8C*((LWord)(pSource->cr-CCIR601_8BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Red = (UByte)ClipRGB_8BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x1DAD7*((LWord)(pSource->cb-CCIR601_8BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Blue = (UByte)ClipRGB_8BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x2FF9*((LWord)(pSource->cb-CCIR601_8BIT_CHROMAOFFSET)) -
		     0x780D*((LWord)(pSource->cr-CCIR601_8BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Green = (UByte)ClipRGB_8BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

inline 	void HDConvert10BitYCbCrtoRGBSmpte(YCbCr10BitAlphaPixel *pSource,
										        RGBAlphaPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = (0xFFDF>>2)*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   (0x19A8C>>2)*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Red = (UByte)ClipRGB_8BIT(Red);

  Blue = FixedRound(ConvertedY +
		    (0x1DAD7>>2)*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Blue = (UByte)ClipRGB_8BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     (0x2FF9>>2)*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET)) -
		     (0x780D>>2)*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_8BIT_BLACK;

  pTarget->Green = (UByte)ClipRGB_8BIT(Green);

  pTarget->Alpha = (UByte)pSource->Alpha;
}


inline 	void SDConvert10BitYCbCrto10BitRGBSmpte(YCbCr10BitAlphaPixel *pSource,
										   RGBAlpha10BitPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0xFF40*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x15DDF*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_10BIT_BLACK;

  pTarget->Red = (UWord)ClipRGB_10BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x1BA34*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_10BIT_BLACK;

  pTarget->Blue = (UWord)ClipRGB_10BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x55E1*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET)) -
		     0xB237*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_10BIT_BLACK;

  pTarget->Green = (UWord)ClipRGB_10BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

inline 	void HDConvert10BitYCbCrto10BitRGBSmpte(YCbCr10BitAlphaPixel *pSource,
										   RGBAlpha10BitPixel *pTarget)
{
  LWord Red,Green,Blue;
  Fixed_ ConvertedY;

  ConvertedY = 0xFFDF*((LWord)pSource->y - CCIR601_10BIT_BLACK);

  Red = FixedRound(ConvertedY +
		   0x19A8C*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_10BIT_BLACK;

  pTarget->Red = (UWord)ClipRGB_10BIT(Red);

  Blue = FixedRound(ConvertedY +
		    0x1DAD7*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_10BIT_BLACK;

  pTarget->Blue = (UWord)ClipRGB_10BIT(Blue);

  Green = FixedRound(ConvertedY - 
		     0x2FF9*((LWord)(pSource->cb-CCIR601_10BIT_CHROMAOFFSET)) -
		     0x780D*((LWord)(pSource->cr-CCIR601_10BIT_CHROMAOFFSET))) + CCIR601_10BIT_BLACK;

  pTarget->Green = (UWord)ClipRGB_10BIT(Green);

  pTarget->Alpha = pSource->Alpha;
}

#endif	//	NTV2TRANSCODE_H
