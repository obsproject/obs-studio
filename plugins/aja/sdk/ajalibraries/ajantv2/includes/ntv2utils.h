/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2utils.h
	@brief		Declares numerous NTV2 utility functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2UTILS_H
#define NTV2UTILS_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2videodefines.h"
#include "ntv2publicinterface.h"
#include "ntv2formatdescriptor.h"
#include "ntv2m31publicinterface.h"
#include "ntv2signalrouter.h"
#include <string>
#include <iostream>
#include <vector>
#if defined (AJALinux)
	#include <stdint.h>
#endif

#define Enum2Str(e)  {e, #e},
//////////////////////////////////////////////////////
//	BEGIN SECTION MOVED FROM 'videoutilities.h'
//////////////////////////////////////////////////////
#define DEFAULT_PATT_GAIN				0.9		//	some patterns pay attention to this...
#define HD_NUMCOMPONENTPIXELS_2K		2048
#define HD_NUMCOMPONENTPIXELS_1080_2K	2048
#define HD_NUMCOMPONENTPIXELS_1080		1920

#define CCIR601_10BIT_BLACK				64
#define CCIR601_10BIT_WHITE				940
#define CCIR601_10BIT_CHROMAOFFSET		512

#define CCIR601_8BIT_BLACK				16
#define CCIR601_8BIT_WHITE				235
#define CCIR601_8BIT_CHROMAOFFSET		128

// line pitch is in bytes.
#define FRAME_0_BASE							(0x0)
#define FRAME_1080_10BIT_LINEPITCH				(1280*4)
#define FRAME_1080_8BIT_LINEPITCH				(1920*2)
#define FRAME_QUADHD_10BIT_SIZE					(FRAME_1080_10BIT_LINEPITCH*2160)
#define FRAME_QUADHD_8BIT_SIZE					(FRAME_1080_8BIT_LINEPITCH*2160)
#define FRAME_BASE(__frameNum__,__frameSize__)	((__frameNum__)*(__frameSize__))

AJAExport NTV2_SHOULD_BE_DEPRECATED(uint32_t CalcRowBytesForFormat (const NTV2FrameBufferFormat inPF, const uint32_t pixWidth));	///< @deprecated	This function doesn't support planar pixel formats. Use ::NTV2FormatDescriptor instead.
AJAExport void UnPack10BitYCbCrBuffer (uint32_t* packedBuffer, uint16_t* ycbcrBuffer, uint32_t numPixels);
AJAExport void PackTo10BitYCbCrBuffer (const uint16_t *ycbcrBuffer, uint32_t *packedBuffer, const uint32_t numPixels);
AJAExport void MakeUnPacked10BitYCbCrBuffer (uint16_t* buffer, uint16_t Y , uint16_t Cb , uint16_t Cr,uint32_t numPixels);
AJAExport void ConvertLineTo8BitYCbCr (const uint16_t * ycbcr10BitBuffer, uint8_t * ycbcr8BitBuffer,	const uint32_t numPixels);
AJAExport void ConvertUnpacked10BitYCbCrToPixelFormat (uint16_t *unPackedBuffer, uint32_t *packedBuffer, uint32_t numPixels, NTV2FrameBufferFormat pixelFormat,
														bool bUseSmpteRange=false, bool bAlphaFromLuma=false);
AJAExport void MaskUnPacked10BitYCbCrBuffer (uint16_t* ycbcrUnPackedBuffer, uint16_t signalMask , uint32_t numPixels);
AJAExport void StackQuadrants (uint8_t* pSrc, uint32_t srcWidth, uint32_t srcHeight, uint32_t srcRowBytes, uint8_t* pDst);
AJAExport void CopyFromQuadrant (uint8_t* srcBuffer, uint32_t srcHeight, uint32_t srcRowBytes, uint32_t srcQuadrant, uint8_t* dstBuffer,  uint32_t quad13Offset=0);
AJAExport void CopyToQuadrant (uint8_t* srcBuffer, uint32_t srcHeight, uint32_t srcRowBytes, uint32_t dstQuadrant, uint8_t* dstBuffer, uint32_t quad13Offset=0);
//////////////////////////////////////////////////////
//	END SECTION MOVED FROM 'videoutilities.h'
//////////////////////////////////////////////////////

/**
	@brief		Unpacks a line of NTV2_FBF_10BIT_YCBCR video into 16-bit-per-component YUV data.
	@param[in]	pIn10BitYUVLine		A valid, non-NULL pointer to the start of the line that contains the NTV2_FBF_10BIT_YCBCR data
									to be converted.
	@param[out]	out16BitYUVLine		Receives the unpacked 16-bit-per-component YUV data. The sequence is cleared before filling.
									The UWord sequence will be Cb0, Y0, Cr0, Y1, Cb1, Y2, Cr1, Y3, Cb2, Y4, Cr2, Y5, . . .
	@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
	@return		True if successful;  otherwise false.
**/
AJAExport bool		UnpackLine_10BitYUVtoUWordSequence (const void * pIn10BitYUVLine, UWordSequence & out16BitYUVLine, ULWord inNumPixels);

/**
	@brief		Packs a line of 16-bit-per-component YCbCr (NTV2_FBF_10BIT_YCBCR) video into 10-bit-per-component YCbCr data.
	@param[in]	in16BitYUVLine		The UWordSequence that contains the 16-bit-per-component YUV data to be converted into
									10-bit-per-component YUV.
	@param[out]	pOut10BitYUVLine	A valid, non-NULL pointer to the output buffer to receive the packed 10-bit-per-component YUV data.
	@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
	@return		True if successful;  otherwise false.
**/
AJAExport bool		PackLine_UWordSequenceTo10BitYUV (const UWordSequence & in16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels);

/**
	@brief		Packs up to one raster line of uint16_t YUV components into an NTV2_FBF_10BIT_YCBCR frame buffer.
	@param[in]	inYCbCrLine		The YUV components to be packed into the frame buffer. This must contain at least 12 values.
	@param		inFrameBuffer	The frame buffer in host memory that is to be modified.
	@param[in]	inDescriptor	The NTV2FormatDescriptor that describes the frame buffer.
	@param[in]	inLineOffset	The zero-based line offset into the frame buffer where the packed components will be written.
	@return		True if successful;  otherwise false.
	@note		Neighboring components in the packed output will be corrupted if input component values exceed 0x3FF.
	@note		This is a safer version of the ::PackLine_UWordSequenceTo10BitYUV function.
**/
AJAExport bool YUVComponentsTo10BitYUVPackedBuffer (const std::vector<uint16_t> & inYCbCrLine, NTV2_POINTER & inFrameBuffer,
													const NTV2FormatDescriptor & inDescriptor, const UWord inLineOffset);

/**
	@brief		Unpacks up to one raster line of an NTV2_FBF_10BIT_YCBCR frame buffer into an array of uint16_t values
				containing the 10-bit YUV data.
	@param[out]	outYCbCrLine	The YUV components unpacked from the frame buffer. This will be cleared upon entry, and
								if successful, will contain at least 12 values upon exit.
	@param		inFrameBuffer	The frame buffer in host memory that is to be read.
	@param[in]	inDescriptor	The NTV2FormatDescriptor that describes the frame buffer.
	@param[in]	inLineOffset	The zero-based line offset into the frame buffer.
	@return		True if successful;  otherwise false.
	@note		This is a safer version of the ::UnpackLine_10BitYUVtoUWordSequence function.
**/
AJAExport bool UnpackLine_10BitYUVtoU16s (std::vector<uint16_t> & outYCbCrLine, const NTV2_POINTER & inFrameBuffer,
											const NTV2FormatDescriptor & inDescriptor, const UWord inLineOffset);


#if !defined (NTV2_DEPRECATE)
	AJAExport NTV2_DEPRECATED_f(void UnPackLineData (const ULWord * pIn10BitYUVLine, UWord * pOut16BitYUVLine, const ULWord inNumPixels));	///< @deprecated	Replaced by UnpackLine_10BitYUVto16BitYUV.
	AJAExport NTV2_DEPRECATED_f(void PackLineData (const UWord * pIn16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels));		///< @deprecated	Replaced by PackLine_16BitYUVto10BitYUV.
#endif	//	NTV2_DEPRECATE

/**
	@brief	Unpacks a line of 10-bit-per-component YCbCr video into 16-bit-per-component YCbCr (NTV2_FBF_10BIT_YCBCR) data.
	@param[in]	pIn10BitYUVLine		A valid, non-NULL pointer to the input buffer that contains the packed 10-bit-per-component YUV data
									to be converted into 16-bit-per-component YUV.
	@param[out]	pOut16BitYUVLine	A valid, non-NULL pointer to the output buffer to receive the unpacked 16-bit-per-component YUV data.
	@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
**/
AJAExport void UnpackLine_10BitYUVto16BitYUV (const ULWord * pIn10BitYUVLine, UWord * pOut16BitYUVLine, const ULWord inNumPixels);

/**
	@brief	Packs a line of 16-bit-per-component YCbCr (NTV2_FBF_10BIT_YCBCR) video into 10-bit-per-component YCbCr data.
	@param[in]	pIn16BitYUVLine		A valid, non-NULL pointer to the input buffer that contains the 16-bit-per-component YUV data to be
									converted into 10-bit-per-component YUV.
	@param[out]	pOut10BitYUVLine	A valid, non-NULL pointer to the output buffer to receive the packed 10-bit-per-component YUV data.
	@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
**/
AJAExport void PackLine_16BitYUVto10BitYUV (const UWord * pIn16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels);

AJAExport void RePackLineDataForYCbCrDPX (ULWord *packedycbcrLine, ULWord numULWords);
AJAExport void UnPack10BitDPXtoRGBAlpha10BitPixel (RGBAlpha10BitPixel* rgba10BitBuffer, const ULWord * DPXLinebuffer, ULWord numPixels, bool bigEndian);
AJAExport void UnPack10BitDPXtoForRP215withEndianSwap(UWord* rawrp215Buffer,ULWord* DPXLinebuffer ,ULWord numPixels);
AJAExport void UnPack10BitDPXtoForRP215(UWord* rawrp215Buffer,ULWord* DPXLinebuffer ,ULWord numPixels);
AJAExport void MaskYCbCrLine(UWord* ycbcrLine, UWord signalMask , ULWord numPixels);

/**
	@brief		Writes a line of unpacked 10-bit Y/C legal SMPTE black values into the given UWord buffer.
	@param[in]	pOutLineData	A valid, non-NULL pointer to the destination UWord buffer.
	@param[in]	inNumPixels		Specifies the width of the line, in pixels. Defaults to 1920.
	@warning	This function performs no error checking. Memory corruption will occur if the destination buffer
				is smaller than 4 x inNumPixels bytes (i.e. smaller than 2 x inNumPixels UWords).
**/
AJAExport void Make10BitBlackLine (UWord * pOutLineData, const ULWord inNumPixels = 1920);

AJAExport void Make10BitWhiteLine(UWord* pOutLineData, const ULWord numPixels=1920);
#if !defined(NTV2_DEPRECATE_13_0)
	AJAExport NTV2_DEPRECATED_f(void Fill10BitYCbCrVideoFrame (PULWord _baseVideoAddress,
																const NTV2Standard inStandard,
																const NTV2FrameBufferFormat inPixelFormat,
																const YCbCr10BitPixel inPixelColor,
																const bool inVancEnabled = false,
																const bool in2Kx1080 = false,
																const bool inWideVANC = false));	///< @deprecated	Use the identical function that accepts an ::NTV2VANCMode parameter instead of two booleans.
#endif	//	!defined(NTV2_DEPRECATE_13_0)
/**
	@return		True if successful;  otherwise false.
**/
AJAExport bool Fill10BitYCbCrVideoFrame (void * pBaseVideoAddress,
										 const NTV2Standard inStandard,
										 const NTV2FrameBufferFormat inPixelFormat,
										 const YCbCr10BitPixel inPixelColor,
										 const NTV2VANCMode inVancMode = NTV2_VANCMODE_OFF);

AJAExport void Make8BitBlackLine(UByte* lineData,ULWord numPixels=1920,NTV2FrameBufferFormat=NTV2_FBF_8BIT_YCBCR);
AJAExport void Make8BitWhiteLine(UByte* lineData,ULWord numPixels=1920,NTV2FrameBufferFormat=NTV2_FBF_8BIT_YCBCR);
AJAExport void Make10BitLine(UWord* lineData, const UWord Y, const UWord Cb, const UWord Cr, const ULWord numPixels = 1920);
AJAExport void Make8BitLine(UByte* lineData, UByte Y , UByte Cb , UByte Cr,ULWord numPixels=1920,NTV2FrameBufferFormat=NTV2_FBF_8BIT_YCBCR);
#if !defined(NTV2_DEPRECATE_13_0)
	AJAExport NTV2_DEPRECATED_f(void Fill8BitYCbCrVideoFrame (PULWord _baseVideoAddress,
																const NTV2Standard inStandard,
																const NTV2FrameBufferFormat inFBF,
																const YCbCrPixel inPixelColor,
																const bool inVancEnabled = false,
																const bool in2Kx1080 = false,
																const bool inWideVanc = false));	///< @deprecated	Use the identical function that accepts an ::NTV2VANCMode parameter instead of two booleans.
#endif	//	!defined(NTV2_DEPRECATE_13_0)
AJAExport bool Fill8BitYCbCrVideoFrame (void * pBaseVideoAddress,  const NTV2Standard inStandard,  const NTV2FrameBufferFormat inFBF,
										const YCbCrPixel inPixelColor,  const NTV2VANCMode inVancMode = NTV2_VANCMODE_OFF);
AJAExport void Fill4k8BitYCbCrVideoFrame(PULWord _baseVideoAddress,
									   NTV2FrameBufferFormat frameBufferFormat,
									   YCbCrPixel color,
									   bool vancEnabled=false,
									   bool b4k=false,
									   bool wideVANC=false);
AJAExport void CopyRGBAImageToFrame(ULWord* pSrcBuffer, ULWord srcHeight, ULWord srcWidth, 
									ULWord* pDstBuffer, ULWord dstHeight, ULWord dstWidth);

/**
	@brief	Sets all or part of a destination raster image to legal black.
	@param[in]	inPixelFormat			Specifies the NTV2FrameBufferFormat of the destination buffer.
										(Note that many pixel formats are not currently supported.)
	@param		pDstBuffer				Specifies the address of the destination buffer to be modified. Must be non-NULL.
	@param[in]	inDstBytesPerLine		The number of bytes per raster line of the destination buffer. Note that this value
										is used to compute the maximum pixel width of the destination raster. Also note that
										some pixel formats set constraints on this value (e.g., NTV2_FBF_10BIT_YCBCR requires
										this be a multiple of 16, while NTV2_FBF_8BIT_YCBCR requires an even number).
										Must exceed zero.
	@param[in]	inDstTotalLines			The total number of raster lines to set to legal black. Must exceed zero.
	@bug		Need implementations for NTV2_FBF_8BIT_YCBCR_YUY2, NTV2_FBF_10BIT_DPX, NTV2_FBF_10BIT_YCBCR_DPX, NTV2_FBF_24BIT_RGB,
				NTV2_FBF_24BIT_BGR, NTV2_FBF_10BIT_YCBCRA, NTV2_FBF_10BIT_DPX_LE, NTV2_FBF_48BIT_RGB, NTV2_FBF_10BIT_RGB_PACKED,
				NTV2_FBF_10BIT_ARGB, NTV2_FBF_16BIT_ARGB, the 3-plane planar formats NTV2_FBF_8BIT_YCBCR_420PL3,
				NTV2_FBF_8BIT_YCBCR_422PL3, NTV2_FBF_10BIT_YCBCR_420PL3_LE, and NTV2_FBF_10BIT_YCBCR_422PL3_LE, plus the 2-plane
				planar formats NTV2_FBF_10BIT_YCBCR_420PL2, NTV2_FBF_10BIT_YCBCR_422PL2, NTV2_FBF_8BIT_YCBCR_420PL2, and
				NTV2_FBF_8BIT_YCBCR_422PL2.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	SetRasterLinesBlack (const NTV2FrameBufferFormat	inPixelFormat,
										UByte *						pDstBuffer,
										const ULWord				inDstBytesPerLine,
										const UWord					inDstTotalLines);

/**
	@brief	Copies all or part of a source raster image into another raster at a given position.
	@param[in]	inPixelFormat			Specifies the NTV2FrameBufferFormat of both the destination and source buffers.
										(Note that many pixel formats are not currently supported.)
	@param		pDstBuffer				Specifies the starting address of the destination buffer to be modified. Must be non-NULL.
	@param[in]	inDstBytesPerLine		The number of bytes per raster line of the destination buffer. Note that this value
										is used to compute the maximum pixel width of the destination raster. Also note that
										some pixel formats set constraints on this value (e.g., NTV2_FBF_10BIT_YCBCR requires
										this be a multiple of 16, while NTV2_FBF_8BIT_YCBCR requires an even number).
										Must exceed zero.
	@param[in]	inDstTotalLines			The maximum height of the destination buffer, in raster lines. Must exceed zero.
	@param[in]	inDstVertLineOffset		Specifies the vertical line offset into the destination raster where the top edge
										of the source image will appear. This value must be less than the inDstTotalLines
										value (i.e., at least one line of the source must appear in the destination).
	@param[in]	inDstHorzPixelOffset	Specifies the horizontal pixel offset into the destination raster where the left
										edge of the source image will appear. This value must be less than the maximum
										width of the destination raster (as stipulated by the inDstBytesPerLine parameter).
										Thus, at least one pixel of the source (the leftmost edge) must appear in the destination
										(at the right edge). Note that some pixel formats set constraints on this value (e.g.,
										NTV2_FBF_10BIT_YCBCR requires this be a multiple of 6, while NTV2_FBF_8BIT_YCBCR requires
										this to be even).
	@param[in]	pSrcBuffer				Specifies the starting address of the source buffer to be copied from. Must be non-NULL.
	@param[in]	inSrcBytesPerLine		The number of bytes per raster line of the source buffer. Note that this value is used
										to compute the maximum pixel width of the source raster. Also note that some pixel formats
										set constraints on this value (e.g., NTV2_FBF_10BIT_YCBCR requires this be a multiple
										of 16, while NTV2_FBF_8BIT_YCBCR requires this to be even). Must exceed zero.
	@param[in]	inSrcTotalLines			The maximum height of the source buffer, in raster lines. Must exceed zero.
	@param[in]	inSrcVertLineOffset		Specifies the top edge of the source image to copy. This value must be less than
										the inSrcTotalLines value.
	@param[in]	inSrcVertLinesToCopy	Specifies the height of the source image to copy, in lines. This value can be larger
										than what's possible. The function guarantees that lines past the bottom edge of the
										source raster will not be accessed. It is not an error to specify zero, although
										nothing will be copied.
	@param[in]	inSrcHorzPixelOffset	Specifies the left edge of the source image to copy. This value must be less than the
										maximum width of the source raster (as stipulated by the inSrcBytesPerLine parameter).
										Note that some pixel formats set constraints on this value (e.g., NTV2_FBF_10BIT_YCBCR
										requires this be a multiple of 6, while NTV2_FBF_8BIT_YCBCR requires this to be even).
	@param[in]	inSrcHorzPixelsToCopy	Specifies the width of the source image to copy, in pixels. This value can be larger
										than what's possible. This function will ensure that pixels past the right edge of the
										source raster will not be accessed. It is not an error to specify zero, although nothing
										will be copied. Note that some pixel formats set constraints on this value(e.g.,
										NTV2_FBF_10BIT_YCBCR requires this be a multiple of 6, while NTV2_FBF_8BIT_YCBCR requires
										this to be even).
	@return		True if successful;  otherwise false.
	@note		The source and destination buffers MUST have the same pixel format.
	@note		The source and destination buffers must NOT point to the same buffer.
	@note		The use of unsigned values precludes positioning the source raster above the top line of the destination raster,
				or to the left of the destination raster's left edge. This function will, however, clip the source raster if it
				overhangs the bottom and/or right edge of the destination raster.
	@note		This function probably can't be made to work with planar formats.
	@bug		Needs implementations for NTV2_FBF_10BIT_YCBCRA, NTV2_FBF_10BIT_RGB_PACKED, NTV2_FBF_10BIT_ARGB,
				NTV2_FBF_16BIT_ARGB.
**/
AJAExport bool	CopyRaster (const NTV2FrameBufferFormat	inPixelFormat,
							UByte *						pDstBuffer,
							const ULWord				inDstBytesPerLine,
							const UWord					inDstTotalLines,
							const UWord					inDstVertLineOffset,
							const UWord					inDstHorzPixelOffset,
							const UByte *				pSrcBuffer,
							const ULWord				inSrcBytesPerLine,
							const UWord					inSrcTotalLines,
							const UWord					inSrcVertLineOffset,
							const UWord					inSrcVertLinesToCopy,
							const UWord					inSrcHorzPixelOffset,
							const UWord					inSrcHorzPixelsToCopy);

AJAExport NTV2Standard GetNTV2StandardFromScanGeometry (const UByte inScanGeometry, const bool inIsProgressiveTransport);

/**
	@return		The ::NTV2VideoFormat that is supported by the device (in frame buffer).
	@param[in]	inVideoFormat	Specifies the input ::NTV2VideoFormat of interest.
**/
AJAExport NTV2VideoFormat GetSupportedNTV2VideoFormatFromInputVideoFormat (const NTV2VideoFormat inVideoFormat);

/**
	@return		The ::NTV2Standard that corresponds to the given ::NTV2VideoFormat.
	@param[in]	inVideoFormat	Specifies the ::NTV2VideoFormat of interest.
	@see		::GetNTV2FrameGeometryFromVideoFormat, ::GetGeometryFromStandard
**/
AJAExport NTV2Standard GetNTV2StandardFromVideoFormat (const NTV2VideoFormat inVideoFormat);

/**
	@return		The ::NTV2FrameGeometry that corresponds to the given ::NTV2VideoFormat.
	@param[in]	inVideoFormat	Specifies the ::NTV2VideoFormat of interest.
	@see		::GetNTV2StandardFromScanGeometry
**/
AJAExport NTV2FrameGeometry GetNTV2FrameGeometryFromVideoFormat (const NTV2VideoFormat inVideoFormat);

#if defined (NTV2_DEPRECATE)
	#define	GetHdmiV2StandardFromVideoFormat(__vf__)	::GetNTV2StandardFromVideoFormat (__vf__)
#else
	AJAExport NTV2V2Standard	GetHdmiV2StandardFromVideoFormat (NTV2VideoFormat videoFormat);
#endif

#if !defined(NTV2_DEPRECATE_13_0)
	AJAExport NTV2_DEPRECATED_f(ULWord GetVideoActiveSize (const NTV2VideoFormat inVideoFormat,
															const NTV2FrameBufferFormat inFBFormat,
															const bool inVANCenabled,
															const bool inWideVANC = false));	///< @deprecated	Use the same function that accepts an ::NTV2VANCMode instead of two booleans.
	AJAExport NTV2_DEPRECATED_f(ULWord GetVideoWriteSize (const NTV2VideoFormat inVideoFormat,
															const NTV2FrameBufferFormat inFBFormat,
															const bool inVANCenabled,
															const bool inWideVANC));	///< @deprecated	Use the same function that accepts an ::NTV2VANCMode instead of two booleans.
#endif	//	!defined(NTV2_DEPRECATE_13_0)

/**
	@return		The minimum number of bytes required to store a single frame of video in the given frame buffer format
				having the given video format, including VANC lines, if any.
	@param[in]	inVideoFormat	Specifies the video format.
	@param[in]	inFBFormat		Specifies the frame buffer format.
	@param[in]	inVancMode		Optionally specifies the VANC mode. Defaults to OFF (no VANC lines).
**/
AJAExport ULWord GetVideoActiveSize (const NTV2VideoFormat inVideoFormat,
									const NTV2FrameBufferFormat inFBFormat,
									const NTV2VANCMode inVancMode = NTV2_VANCMODE_OFF);


/**
	@brief		Identical to the ::GetVideoActiveSize function, except rounds the result up to the nearest 4K page
				size multiple.
	@return		The number of bytes required to store a single frame of video in the given frame buffer format
				having the given video format, including VANC lines, if any, rounded up to the nearest 4096-byte multiple.
	@param[in]	inVideoFormat	Specifies the video format.
	@param[in]	inFBFormat		Specifies the frame buffer format.
	@param[in]	inVancMode		Optionally specifies the VANC mode. Defaults to OFF (no VANC lines).
	@details	Historically, this "nearest 4K page size" was necessary to get high-speed I/O bursting to work
				between the AJA device and the Windows disk system. The file needed to be opened with FILE_FLAG_NO_BUFFERING
				to bypass the file system cache.
**/
AJAExport ULWord GetVideoWriteSize (const NTV2VideoFormat inVideoFormat,
									const NTV2FrameBufferFormat inFBFormat,
									const NTV2VANCMode inVancMode = NTV2_VANCMODE_OFF);

AJAExport NTV2VideoFormat GetQuarterSizedVideoFormat (const NTV2VideoFormat inVideoFormat);
AJAExport NTV2VideoFormat GetQuadSizedVideoFormat (const NTV2VideoFormat inVideoFormat, const bool isSquareDivision = true);
AJAExport NTV2FrameGeometry GetQuarterSizedGeometry (const NTV2FrameGeometry inGeometry);
AJAExport NTV2FrameGeometry Get4xSizedGeometry (const NTV2FrameGeometry inGeometry);
AJAExport NTV2Standard GetQuarterSizedStandard (const NTV2Standard inGeometry);
AJAExport NTV2Standard Get4xSizedStandard (const NTV2Standard inGeometry, const bool bIs4k = false);

AJAExport double GetFramesPerSecond (const NTV2FrameRate inFrameRate);
inline double GetFrameTime (const NTV2FrameRate inFrameRate)	{return double(1.0) / GetFramesPerSecond(inFrameRate);}

/**
	@return		The first NTV2VideoFormat that matches the given frame rate and raster dimensions (and whether it's interlaced or not).
	@param[in]	inFrameRate		Specifies the frame rate of interest.
	@param[in]	inHeightLines	Specifies the raster height, in lines of visible video.
	@param[in]	inWidthPixels	Specifies the raster width, in pixels.
	@param[in]	inIsInterlaced	Specify true for interlaced/psf video, or false for progressive.
	@param[in]	inIsLevelB    	Specify true for level B, or false for everything else.
	@param[in]	inIsPSF			Specify true for segmented format false for everything else.

**/
AJAExport NTV2VideoFormat	GetFirstMatchingVideoFormat (const NTV2FrameRate inFrameRate,
														 const UWord inHeightLines,
														 const UWord inWidthPixels,
														 const bool inIsInterlaced,
														 const bool inIsLevelB,
														 const bool inIsPSF);

/**
	@brief		Answers with the given frame rate, in frames per second, as two components:
				the numerator and denominator of the fractional rate.
	@param[in]	inFrameRate				Specifies the frame rate of interest.
	@param[out]	outFractionNumerator	Receives the numerator of the fractional frame rate.
										This will be zero if the function returns false.
	@param[out]	outFractionDenominator	Receives the denominator of the fractional frame rate.
										If the function is successful, this will be either 1000 or 1001.
										This will be zero if the function returns false.
	@return		True if successful;  otherwise false.
**/
AJAExport bool GetFramesPerSecond (const NTV2FrameRate inFrameRate, ULWord & outFractionNumerator, ULWord & outFractionDenominator);

/**
	@param[in]	inDevID		Specifies the ::NTV2DeviceID of the device of interest.
	@param[in]	inFR		Specifies the ::NTV2FrameRate of interest.
	@param[in]	inFG		Specifies the ::NTV2FrameGeometry of interest.
	@param[in]	inStd		Specifies the ::NTV2Standard of interest.
	@deprecated	This function is deprecated.
	@note		The implementation of this function is very inefficient. Do not call it every frame.
	@return		True if the device having the given NTV2DeviceID supports the given frame rate, geometry and standard.
**/
AJAExport NTV2_SHOULD_BE_DEPRECATED(bool NTV2DeviceCanDoFormat (const NTV2DeviceID inDevID, const NTV2FrameRate inFR, const NTV2FrameGeometry inFG, const NTV2Standard inStd));

/**
	@brief	Returns the number of audio samples for a given video frame rate, audio sample rate, and frame number.
			This is useful since AJA devices use fixed audio sample rates (typically 48KHz), and some video frame
			rates will necessarily result in some frames having more audio samples than others.
	@param[in]	inFrameRate			Specifies the video frame rate.
	@param[in]	inAudioRate			Specifies the audio sample rate. Must be one of NTV2_AUDIO_48K or NTV2_AUDIO_96K.
	@param[in]	inCadenceFrame		Optionally specifies a frame number for maintaining proper cadence in a frame sequence,
									for those video frame rates that don't accommodate an even number of audio samples.
									Defaults to zero.
	@param[in]	inIsSMPTE372Enabled	Specifies that 1080p60, 1080p5994 or 1080p50 is being used. In this mode, the device's
									framerate might be NTV2_FRAMERATE_3000, but since 2 links are coming out, the video rate
									is effectively NTV2_FRAMERATE_6000. Defaults to false.
	@return	The number of audio samples.
	@see	See \ref audiosamplecount
**/
AJAExport ULWord				GetAudioSamplesPerFrame (const NTV2FrameRate inFrameRate, const NTV2AudioRate inAudioRate, ULWord inCadenceFrame = 0, bool inIsSMPTE372Enabled = false);
AJAExport LWord64				GetTotalAudioSamplesFromFrameNbrZeroUpToFrameNbr (NTV2FrameRate frameRate, NTV2AudioRate audioRate, ULWord frameNbrNonInclusive);

AJAExport NTV2_SHOULD_BE_DEPRECATED(ULWord GetVaricamRepeatCount (const NTV2FrameRate inSequenceRate, const NTV2FrameRate inPlayRate, const ULWord inCadenceFrame = 0));
AJAExport ULWord				GetScaleFromFrameRate (const NTV2FrameRate inFrameRate);
AJAExport NTV2FrameRate			GetFrameRateFromScale (long scale, long duration, NTV2FrameRate playFrameRate);
AJAExport NTV2FrameRate			GetNTV2FrameRateFromNumeratorDenominator (const ULWord inNumerator, const ULWord inDenominator);	//	New in SDK 16.0

/**
	@return		The NTV2FrameRate of the given NTV2VideoFormat.
	@param[in]	inVideoFormat	Specifies the NTV2VideoFormat of interest.
	@note		This function is physical-transport-centric, so "B" formats will answer with the frame rate
				of a single link. For example, ::NTV2_FORMAT_1080p_6000_B will result in ::NTV2_FRAMERATE_3000.
				See \ref duallinkoverview for more information.
**/
AJAExport NTV2FrameRate			GetNTV2FrameRateFromVideoFormat (const NTV2VideoFormat inVideoFormat);

/**
	@return		The equivalent non-VANC ::NTV2FrameGeometry value for a given ::NTV2FrameGeometry.
	@param[in]	inFrameGeometry	Specifies the ::NTV2FrameGeometry to be normalized into its non-VANC equivalent.
	@see		::GetVANCFrameGeometry
**/
AJAExport NTV2FrameGeometry		GetNormalizedFrameGeometry (const NTV2FrameGeometry inFrameGeometry);

/**
	@return		The equivalent VANC ::NTV2FrameGeometry value for a given ::NTV2VANCMode.
	@param[in]	inFrameGeometry	Specifies the ::NTV2FrameGeometry to be converted into its VANC equivalent.
	@param[in]	inVancMode		Specifies the desired ::NTV2VANCMode.
	@see		::GetNormalizedFrameGeometry
**/
AJAExport NTV2FrameGeometry		GetVANCFrameGeometry (const NTV2FrameGeometry inFrameGeometry, const NTV2VANCMode inVancMode);

/**
	@return		The first matching ::NTV2FrameGeometry that matches the given ::NTV2FrameDimensions,
				or ::NTV2_FG_INVALID if none match.
	@param[in]	inFD	Specifies the ::NTV2FrameDimensions of interest.
**/
AJAExport NTV2FrameGeometry		GetGeometryFromFrameDimensions (const NTV2FrameDimensions & inFD);	//	New in SDK 16.0

/**
	@return		True if the given ::NTV2FrameGeometry has tall or taller geometries associated with it;
				otherwise false.
	@param[in]	inFrameGeometry	Specifies the ::NTV2FrameGeometry.
	@see		::GetVANCFrameGeometry
**/
AJAExport bool					HasVANCGeometries (const NTV2FrameGeometry inFrameGeometry);

/**
	@return		An ::NTV2GeometrySet containing normal, tall and (possibly) taller frame geometries
				that are associated with the given ::NTV2FrameGeometry;  or an empty set if passed
				an invalid geometry.
	@param[in]	inFrameGeometry		Specifies the ::NTV2FrameGeometry. (Need not be normalized.)
	@note		The resulting set will, at the least, contain the given geometry (if valid).
	@see		::GetVANCFrameGeometry
**/
AJAExport NTV2GeometrySet		GetRelatedGeometries (const NTV2FrameGeometry inFrameGeometry);

/**
	@return		The equivalent NTV2VANCMode for a given ::NTV2FrameGeometry.
	@param[in]	inFrameGeometry	Specifies the ::NTV2FrameGeometry.
	@see		::GetVANCFrameGeometry
**/
AJAExport NTV2VANCMode			GetVANCModeForGeometry (const NTV2FrameGeometry inFrameGeometry);

/**
	@return		The pixel width of the given ::NTV2FrameGeometry.
	@param[in]	inFrameGeometry	Specifies the ::NTV2FrameGeometry of interest.
	@see		::GetVANCFrameGeometry
**/
AJAExport ULWord				GetNTV2FrameGeometryWidth (const NTV2FrameGeometry inFrameGeometry);

/**
	@return		The height, in lines, of the given ::NTV2FrameGeometry.
	@param[in]	inFrameGeometry	Specifies the ::NTV2FrameGeometry of interest.
	@see		::GetVANCFrameGeometry
**/
AJAExport ULWord				GetNTV2FrameGeometryHeight (const NTV2FrameGeometry inFrameGeometry);

/**
	@return		The equivalent normalized (non-VANC) ::NTV2FrameGeometry value for a given ::NTV2Standard.
	@param[in]	inStandard		Specifies the ::NTV2Standard to be converted into a normalized ::NTV2FrameGeometry.
**/
AJAExport NTV2FrameGeometry		GetGeometryFromStandard (const NTV2Standard inStandard);

/**
	@return		The equivalent ::NTV2Standard for the given ::NTV2FrameGeometry.
	@param[in]	inGeometry		Specifies the ::NTV2FrameGeometry to be converted into a ::NTV2Standard.
	@param[in]	inIsProgressive	Specifies if the resulting Standard should have a progressive transport or not.
								Defaults to true.
**/
AJAExport NTV2Standard			GetStandardFromGeometry (const NTV2FrameGeometry inGeometry, const bool inIsProgressive = true);

AJAExport ULWord				GetDisplayWidth (const NTV2VideoFormat videoFormat);
AJAExport ULWord				GetDisplayHeight (const NTV2VideoFormat videoFormat);
AJAExport NTV2ConversionMode	GetConversionMode (const NTV2VideoFormat inFormat, const NTV2VideoFormat outFormat);
AJAExport NTV2VideoFormat		GetInputForConversionMode (const NTV2ConversionMode conversionMode);
AJAExport NTV2VideoFormat		GetOutputForConversionMode (const NTV2ConversionMode conversionMode);

AJAExport NTV2Channel			GetNTV2ChannelForIndex (const ULWord inIndex0);
AJAExport ULWord				GetIndexForNTV2Channel (const NTV2Channel inChannel);

AJAExport NTV2_SHOULD_BE_DEPRECATED(NTV2Crosspoint	GetNTV2CrosspointChannelForIndex (const ULWord inIndex0));
AJAExport NTV2_SHOULD_BE_DEPRECATED(ULWord			GetIndexForNTV2CrosspointChannel (const NTV2Crosspoint inChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(NTV2Crosspoint	GetNTV2CrosspointInputForIndex (const ULWord inIndex0));
AJAExport NTV2_SHOULD_BE_DEPRECATED(ULWord			GetIndexForNTV2CrosspointInput (const NTV2Crosspoint inChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(NTV2Crosspoint	GetNTV2CrosspointForIndex (const ULWord inIndex0));
AJAExport NTV2_SHOULD_BE_DEPRECATED(ULWord			GetIndexForNTV2Crosspoint (const NTV2Crosspoint inChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(bool			IsNTV2CrosspointInput (const NTV2Crosspoint inChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(bool			IsNTV2CrosspointOutput (const NTV2Crosspoint inChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(std::string		NTV2CrosspointToString (const NTV2Crosspoint inChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(NTV2Channel		NTV2CrosspointToNTV2Channel (const NTV2Crosspoint inCrosspointChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(NTV2Crosspoint	NTV2ChannelToInputCrosspoint (const NTV2Channel inChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(NTV2Crosspoint	NTV2ChannelToOutputCrosspoint (const NTV2Channel inChannel));
AJAExport NTV2_SHOULD_BE_DEPRECATED(NTV2InputSource	GetNTV2HDMIInputSourceForIndex (const ULWord inIndex0));

AJAExport NTV2VideoFormat		GetTransportCompatibleFormat (const NTV2VideoFormat inFormat, const NTV2VideoFormat inTargetFormat);
AJAExport bool					IsTransportCompatibleFormat (const NTV2VideoFormat inFormat1, const NTV2VideoFormat inFormat2);

/**
	@return		The input source that corresponds to an index value, or ::NTV2_INPUTSOURCE_INVALID upon failure.
	@param[in]	inIndex0	Specifies the unsigned, zero-based integer value to be converted into an equivalent ::NTV2InputSource.
	@param[in]	inKinds		Optionally specifies the input source type (SDI, HDMI, Analog, etc.) of interest.
							Defaults to ::NTV2_INPUTSOURCES_SDI.
**/
AJAExport NTV2InputSource		GetNTV2InputSourceForIndex (const ULWord inIndex0, const NTV2InputSourceKinds inKinds = NTV2_INPUTSOURCES_SDI);
AJAExport ULWord				GetIndexForNTV2InputSource (const NTV2InputSource inValue);		//	0-based index

/**
	@brief		Converts the given ::NTV2Channel value into the equivalent input ::INTERRUPT_ENUMS value.
	@param[in]	inChannel		Specifies the ::NTV2Channel to be converted.
	@return		The equivalent input ::INTERRUPT_ENUMS value.
**/
AJAExport INTERRUPT_ENUMS		NTV2ChannelToInputInterrupt (const NTV2Channel inChannel);

/**
	@brief		Converts the given ::NTV2Channel value into the equivalent output ::INTERRUPT_ENUMS value.
	@param[in]	inChannel		Specifies the ::NTV2Channel to be converted.
	@return		The equivalent output ::INTERRUPT_ENUMS value.
**/
AJAExport INTERRUPT_ENUMS		NTV2ChannelToOutputInterrupt (const NTV2Channel inChannel);

/**
	@brief		Converts the given ::NTV2Channel value into the equivalent ::NTV2TCIndex value.
	@param[in]	inChannel		Specifies the ::NTV2Channel to be converted.
	@param[in]	inEmbeddedLTC	Specify true for embedded LTC. Defaults to false.
	@param[in]	inIsF2			Specify true for VITC2. Defaults to false.
	@return		The equivalent ::NTV2TCIndex value.
**/
AJAExport NTV2TCIndex			NTV2ChannelToTimecodeIndex (const NTV2Channel inChannel, const bool inEmbeddedLTC = false, const bool inIsF2 = false);

/**
	@return		The NTV2TCIndexes that are associated with the given SDI connector.
	@param[in]	inSDIConnector	The SDI connector of interest, specified as an ::NTV2Channel (a zero-based index number).
**/
AJAExport NTV2TCIndexes			GetTCIndexesForSDIConnector (const NTV2Channel inSDIConnector);

/**
	@brief		Converts the given ::NTV2TCIndex value into the appropriate ::NTV2Channel value.
	@param[in]	inTCIndex		Specifies the ::NTV2TCIndex to be converted.
	@return		The equivalent ::NTV2Channel value.
**/
AJAExport NTV2Channel			NTV2TimecodeIndexToChannel (const NTV2TCIndex inTCIndex);

/**
	@brief		Converts the given ::NTV2TCIndex value into the appropriate ::NTV2InputSource value.
	@param[in]	inTCIndex		Specifies the ::NTV2TCIndex to be converted.
	@return		The equivalent ::NTV2InputSource value.
**/
AJAExport NTV2InputSource		NTV2TimecodeIndexToInputSource (const NTV2TCIndex inTCIndex);


#define	GetTCIndexesForSDIInput			GetTCIndexesForSDIConnector
#define	NTV2ChannelToCaptureCrosspoint	NTV2ChannelToInputCrosspoint
#define	NTV2ChannelToIngestCrosspoint	NTV2ChannelToInputCrosspoint
#define	NTV2ChannelToInputChannelSpec	NTV2ChannelToInputCrosspoint
#define	NTV2ChannelToPlayoutCrosspoint	NTV2ChannelToOutputCrosspoint
#define	NTV2ChannelToOutputChannelSpec	NTV2ChannelToOutputCrosspoint


/**
	@brief		Converts the given ::NTV2Framesize value into an exact byte count.
	@param[in]	inFrameSize		Specifies the ::NTV2Framesize to be converted.
	@return		The equivalent number of bytes.
**/
AJAExport ULWord	NTV2FramesizeToByteCount (const NTV2Framesize inFrameSize);

/**
	@brief		Converts the given NTV2BufferSize value into its exact byte count.
	@param[in]	inBufferSize	Specifies the NTV2AudioBufferSize to be converted.
	@return		The equivalent number of bytes.
**/
AJAExport ULWord	NTV2AudioBufferSizeToByteCount (const NTV2AudioBufferSize inBufferSize);

/**
	@brief		Converts the given NTV2Channel value into its equivalent NTV2EmbeddedAudioInput.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent NTV2EmbeddedAudioInput value.
**/
AJAExport NTV2EmbeddedAudioInput NTV2ChannelToEmbeddedAudioInput (const NTV2Channel inChannel);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2EmbeddedAudioInput value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@return		The equivalent NTV2EmbeddedAudioInput value.
**/
AJAExport NTV2EmbeddedAudioInput NTV2InputSourceToEmbeddedAudioInput (const NTV2InputSource inInputSource);

/**
	@param[in]	inInputSource	Specifies the NTV2InputSource.
	@return		The NTV2AudioSource that corresponds to the given NTV2InputSource.
**/
AJAExport NTV2AudioSource NTV2InputSourceToAudioSource (const NTV2InputSource inInputSource);

/**
 @brief		Converts a given NTV2InputSource to its equivalent NTV2Crosspoint value.
 @param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
 @return		The equivalent NTV2Crosspoint value.
 **/
AJAExport NTV2Crosspoint NTV2InputSourceToChannelSpec (const NTV2InputSource inInputSource);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2Channel value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@return		The equivalent NTV2Channel value.
**/
AJAExport NTV2Channel NTV2InputSourceToChannel (const NTV2InputSource inInputSource);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2ReferenceSource value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@return		The equivalent NTV2ReferenceSource value.
**/
AJAExport NTV2ReferenceSource NTV2InputSourceToReferenceSource (const NTV2InputSource inInputSource);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2AudioSystem value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@return		The equivalent NTV2AudioSystem value.
**/
AJAExport NTV2AudioSystem NTV2InputSourceToAudioSystem (const NTV2InputSource inInputSource);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2TimecodeIndex value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@param[in]	inEmbeddedLTC	Specify true for embedded ATC LTC. Defaults to false.
	@return		The equivalent NTV2TimecodeIndex value.
**/
AJAExport NTV2TimecodeIndex NTV2InputSourceToTimecodeIndex (const NTV2InputSource inInputSource, const bool inEmbeddedLTC = false);

/**
	@brief		Converts the given NTV2Channel value into its equivalent NTV2AudioSystem.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent NTV2AudioSystem value.
**/
AJAExport NTV2AudioSystem NTV2ChannelToAudioSystem (const NTV2Channel inChannel);

/**
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@param[in]	inKinds			Specifies the type of input source of interest (SDI, HDMI, etc.).
								Defaults to SDI.
	@return		The NTV2InputSource value that corresponds to the given NTV2Channel value.
**/
AJAExport NTV2InputSource NTV2ChannelToInputSource (const NTV2Channel inChannel, const NTV2InputSourceKinds inKinds = NTV2_INPUTSOURCES_SDI);

/**
	@brief		Converts a given NTV2OutputDestination to its equivalent NTV2Channel value.
	@param[in]	inOutputDest	Specifies the NTV2OutputDestination to be converted.
	@return		The equivalent NTV2Channel value.
**/
AJAExport NTV2Channel NTV2OutputDestinationToChannel (const NTV2OutputDestination inOutputDest);

/**
	@brief		Converts the given NTV2Channel value into its ordinary equivalent NTV2OutputDestination.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent NTV2OutputDestination value.
**/
AJAExport NTV2OutputDestination NTV2ChannelToOutputDestination (const NTV2Channel inChannel);

/**
	@return		The frame rate family that the given ::NTV2FrameRate belongs to.
				(This is the ::NTV2FrameRate of the family having the lowest ordinal value.)
	@param[in]	inFrameRate		Specifies the frame rate of interest.
**/
AJAExport NTV2FrameRate GetFrameRateFamily (const NTV2FrameRate inFrameRate);

/**
	@brief	Compares two frame rates and returns true if they are "compatible" (with respect to a multiformat-capable device).
	@param[in]	inFrameRate1	Specifies one of the frame rates to be compared.
	@param[in]	inFrameRate2	Specifies another frame rate to be compared.
**/
AJAExport bool IsMultiFormatCompatible (const NTV2FrameRate inFrameRate1, const NTV2FrameRate inFrameRate2);

/**
	@brief	Compares two video formats and returns true if they are "compatible" (with respect to a multiformat-capable device).
	@param[in]	inFormat1	Specifies one of the video formats to be compared.
	@param[in]	inFormat2	Specifies another video format to be compared.
**/
AJAExport bool IsMultiFormatCompatible (const NTV2VideoFormat inFormat1, const NTV2VideoFormat inFormat2);

AJAExport bool IsPSF (const NTV2VideoFormat format);
AJAExport bool IsProgressivePicture (const NTV2VideoFormat format);
AJAExport bool IsProgressiveTransport (const NTV2VideoFormat format);
AJAExport bool IsProgressiveTransport (const NTV2Standard format);
AJAExport bool IsRGBFormat (const NTV2FrameBufferFormat format);
AJAExport bool IsYCbCrFormat (const NTV2FrameBufferFormat format);
AJAExport bool IsAlphaChannelFormat (const NTV2FrameBufferFormat format);
AJAExport bool Is2KFormat (const NTV2VideoFormat format);
AJAExport bool Is4KFormat (const NTV2VideoFormat format);
AJAExport bool Is8KFormat (const NTV2VideoFormat format);
AJAExport bool IsRaw (const NTV2FrameBufferFormat format);
AJAExport bool Is8BitFrameBufferFormat (const NTV2FrameBufferFormat fbFormat);
AJAExport bool IsVideoFormatA (const NTV2VideoFormat format);
AJAExport bool IsVideoFormatB (const NTV2VideoFormat format);
AJAExport bool IsVideoFormatJ2KSupported (const NTV2VideoFormat format);


AJAExport int  RecordCopyAudio (PULWord pAja, PULWord pSR, int iStartSample, int iNumBytes, int iChan0,
							   int iNumChans, bool bKeepAudio24Bits);

/**
	@brief	Fills the given buffer with 32-bit (ULWord) audio tone samples.
	@param		pAudioBuffer		If non-NULL, must be a valid pointer to the buffer to be filled with audio samples,
									and must be at least  4 x numSamples x numChannels  bytes in size.
									Callers may specify NULL to have the function return the required size of the buffer.
	@param		inOutCurrentSample	On entry, specifies the sample where waveform generation is to resume.
									If audioBuffer is non-NULL, on exit, receives the sample number where waveform generation left off.
									Callers should specify zero for the first invocation of this function.
	@param[in]	inNumSamples		Specifies the number of samples to generate.
	@param[in]	inSampleRate		Specifies the sample rate, in samples per second.
	@param[in]	inAmplitude			Specifies the amplitude of the generated tone.
	@param[in]	inFrequency			Specifies the frequency of the generated tone, in cycles per second (Hertz).
	@param[in]	inNumBits			Specifies the number of bits per sample. Should be between 8 and 32 (inclusive).
	@param[in]	inByteSwap			If true, byte-swaps each 32-bit sample before copying it into the destination buffer.
	@param[in]	inNumChannels		Specifies the number of audio channels to produce.
	@return		The total number of bytes written into the destination buffer (or if audioBuffer is NULL, the minimum
				required size of the destination buffer, in bytes).
**/
AJAExport ULWord	AddAudioTone (	ULWord *		pAudioBuffer,
									ULWord &		inOutCurrentSample,
									const ULWord	inNumSamples,
									const double	inSampleRate,
									const double	inAmplitude,
									const double	inFrequency,
									const ULWord	inNumBits,
									const bool		inByteSwap,
									const ULWord	inNumChannels);

/**
	@brief	Fills the given buffer with 32-bit (ULWord) audio tone samples with a different frequency in each audio channel.
	@param		pAudioBuffer		If non-NULL, must be a valid pointer to the buffer to be filled with audio samples,
									and must be at least  4 x numSamples x numChannels  bytes in size.
									Callers may specify NULL to have the function return the required size of the buffer.
	@param		inOutCurrentSample	On entry, specifies the sample where waveform generation is to resume.
									If audioBuffer is non-NULL, on exit, receives the sample number where waveform generation left off.
									Callers should specify zero for the first invocation of this function.
	@param[in]	inNumSamples		Specifies the number of samples to generate.
	@param[in]	inSampleRate		Specifies the sample rate, in samples per second.
	@param[in]	pInAmplitudes		A valid, non-NULL pointer to an array of per-channel amplitude values.
									This array must contain at least "inNumChannels" entries.
	@param[in]	pInFrequencies		A valid, non-NULL pointer to an array of per-channel frequency values, in cycles per second (Hertz).
									This array must contain at least "inNumChannels" entries.
	@param[in]	inNumBits			Specifies the number of bits per sample. Should be between 8 and 32 (inclusive).
	@param[in]	inByteSwap			If true, byte-swaps each 32-bit sample before copying it into the destination buffer.
	@param[in]	inNumChannels		Specifies the number of audio channels to produce.
	@return		The total number of bytes written into the destination buffer (or if audioBuffer is NULL, the minimum
				required size of the destination buffer, in bytes).
**/
AJAExport ULWord	AddAudioTone (	ULWord *		pAudioBuffer,
									ULWord &		inOutCurrentSample,
									const ULWord	inNumSamples,
									const double	inSampleRate,
									const double *	pInAmplitudes,
									const double *	pInFrequencies,
									const ULWord	inNumBits,
									const bool		inByteSwap,
									const ULWord	inNumChannels);

/**
	@brief	Fills the given buffer with 16-bit (UWord) audio tone samples.
	@param		pAudioBuffer		If non-NULL, must be a valid pointer to the buffer to be filled with audio samples,
									and must be at least numSamples*2*numChannels bytes in size.
									Callers may specify NULL to have the function return the required size of the buffer.
	@param		inOutCurrentSample	On entry, specifies the sample where waveform generation is to resume.
									If audioBuffer is non-NULL, on exit, receives the sample number where waveform generation left off.
									Callers should specify zero for the first invocation of this function.
	@param[in]	inNumSamples		Specifies the number of samples to generate.
	@param[in]	inSampleRate		Specifies the sample rate, in samples per second.
	@param[in]	inAmplitude			Specifies the amplitude of the generated tone.
	@param[in]	inFrequency			Specifies the frequency of the generated tone, in cycles per second (Hertz).
	@param[in]	inNumBits			Specifies the number of bits per sample. Should be between 8 and 16 (inclusive).
	@param[in]	inByteSwap			If true, byte-swaps each 16-bit sample before copying it into the destination buffer.
	@param[in]	inNumChannels		Specifies the number of audio channels to produce.
	@return		The total number of bytes written into the destination buffer (or if audioBuffer is NULL, the minimum
				required size of the destination buffer, in bytes).
**/
AJAExport ULWord	AddAudioTone (	UWord *			pAudioBuffer,
									ULWord &		inOutCurrentSample,
									const ULWord	inNumSamples,
									const double	inSampleRate,
									const double	inAmplitude,
									const double	inFrequency,
									const ULWord	inNumBits,
									const bool		inByteSwap,
									const ULWord	inNumChannels);

AJAExport ULWord	AddAudioTestPattern (ULWord *		pAudioBuffer,
										 ULWord &		inOutCurrentSample,
										 const ULWord	inNumSamples,
										 const ULWord	inModulus,
										 const bool		inEndianConvert,
										 const ULWord	inNumChannels);

#if !defined (NTV2_DEPRECATE)
	AJAExport bool BuildRoutingTableForOutput (CNTV2SignalRouter &		outRouter,
												NTV2Channel				channel,
												NTV2FrameBufferFormat	fbf,
												bool					convert		= false,	// ignored
												bool					lut			= false,
												bool					dualLink	= false,
												bool					keyOut		= false);

	AJAExport bool BuildRoutingTableForInput (CNTV2SignalRouter &		outRouter,
											   NTV2Channel				channel,
											   NTV2FrameBufferFormat	fbf,
											   bool						withKey		= false,
											   bool						lut			= false,
											   bool						dualLink	= false,
											   bool						EtoE		= true);

	AJAExport bool BuildRoutingTableForInput (CNTV2SignalRouter &		outRouter,
											   NTV2Channel				channel,
											   NTV2FrameBufferFormat	fbf,
											   bool						convert,  // Turn on the conversion module
											   bool						withKey,  // only supported for ::NTV2_CHANNEL1 for rgb formats with alpha
											   bool						lut,	  // not supported
											   bool						dualLink, // assume coming in RGB(only checked for ::NTV2_CHANNEL1
											   bool						EtoE);

	AJAExport bool BuildRoutingTableForInput (CNTV2SignalRouter &		outRouter,
											   NTV2InputSource			inputSource,
											   NTV2Channel				channel,
											   NTV2FrameBufferFormat	fbf,
											   bool						convert,  // Turn on the conversion module
											   bool						withKey,  // only supported for ::NTV2_CHANNEL1 for rgb formats with alpha
											   bool						lut,	  // not supported
											   bool						dualLink, // assume coming in RGB(only checked for ::NTV2_CHANNEL1
											   bool						EtoE);

	AJAExport ULWord ConvertFusionAnalogToTempCentigrade (ULWord adc10BitValue);

	AJAExport ULWord ConvertFusionAnalogToMilliVolts (ULWord adc10BitValue, ULWord millivoltsResolution);
#endif	//	!defined (NTV2_DEPRECATE)


/**
	@brief		Writes the given NTV2FrameDimensions to the specified output stream.
	@param		inOutStream			Specifies the output stream to receive the human-readable representation of the NTV2FrameDimensions.
	@param[in]	inFrameDimensions	Specifies the NTV2FrameDimensions to print to the output stream.
	@return	A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2FrameDimensions inFrameDimensions);


/**
	@brief		Used to describe Start of Active Video (SAV) location and field dominance for a given NTV2Standard.
				(See GetSmpteLineNumber function.)
**/
typedef struct AJAExport NTV2SmpteLineNumber
{
	ULWord			smpteFirstActiveLine;	///< @brief	SMPTE line number of first (top-most) active line of video
	ULWord			smpteSecondActiveLine;	///< @brief	SMPTE line number of second active line of video
	bool			firstFieldTop;			///< @brief	True if the first field on the wire is the top-most field in the raster (field dominance)
private:
	NTV2Standard	mStandard;				///< @brief	The NTV2Standard I was constructed from

public:
	/**
		@brief	Constructs me from a given NTV2Standard.
		@param[in]	inStandard		Specifies the NTV2Standard.
	**/
	explicit		NTV2SmpteLineNumber (const NTV2Standard inStandard = NTV2_STANDARD_INVALID);

	/**
		@return		True if valid;  otherwise false.
	**/
	inline bool		IsValid (void) const	{return NTV2_IS_VALID_STANDARD (mStandard) && smpteFirstActiveLine;}

	/**
		@returns	The SMPTE line number of the Start of Active Video (SAV).
		@param[in]	inRasterFieldID		Specifies a valid raster field ID (not the wire field ID).
										Defaults to NTV2_FIELD0 (i.e. first field) of the raster.
										Use NTV2_FIELD1 for the starting line of a PsF frame.
	**/
	ULWord			GetFirstActiveLine (const NTV2FieldID inRasterFieldID = NTV2_FIELD0) const;

	/**
		@returns	The SMPTE line number of the last raster line.
		@param[in]	inRasterFieldID		Specifies a valid raster field ID (not the wire field ID).
										Defaults to NTV2_FIELD0 (i.e. first field) of the raster.
										Use NTV2_FIELD1 for the starting line of a PsF frame.
	**/
	ULWord			GetLastLine (const NTV2FieldID inRasterFieldID = NTV2_FIELD0) const;

	/**
		@return	True if I'm equal to the given NTV2SmpteLineNumber.
		@param[in]	inRHS	The right-hand-side operand that I'll be compared with.
	**/
	bool			operator == (const NTV2SmpteLineNumber & inRHS) const;

	/**
		@brief		Writes a human-readable description of me into the given output stream.
		@param		inOutStream		The output stream to be written into.
		@return		The output stream I was handed.
	**/
	std::ostream &	Print (std::ostream & inOutStream) const;

	/**
		@param[in]	inLineOffset		Specifies a line offset to add to the printed SMPTE line number.
										Defaults to zero.
		@param[in]	inRasterFieldID		Specifies a valid raster field ID (not the wire field ID).
										Defaults to NTV2_FIELD0 (i.e. first field) of the raster.
		@return		A string containing the human-readable line number.
	**/
	std::string		PrintLineNumber (const ULWord inLineOffset = 0, const NTV2FieldID inRasterFieldID = NTV2_FIELD0) const;

private:
	explicit inline	NTV2SmpteLineNumber (const ULWord inFirstActiveLine,
										const ULWord inSecondActiveLine,
										const bool inFirstFieldTop,
										const NTV2Standard inStandard)
					:	smpteFirstActiveLine	(inFirstActiveLine),
						smpteSecondActiveLine	(inSecondActiveLine),
						firstFieldTop			(inFirstFieldTop),
						mStandard				(inStandard)	{	}

} NTV2SmpteLineNumber;


/**
	@brief		Writes the given NTV2SmpteLineNumber to the specified output stream.
	@param		inOutStream			Specifies the output stream to receive the human-readable representation of the NTV2SmpteLineNumber.
	@param[in]	inSmpteLineNumber	Specifies the NTV2SmpteLineNumber instance to print to the output stream.
	@return	A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2SmpteLineNumber & inSmpteLineNumber);


/**
	@brief		For the given video standard, returns the SMPTE-designated line numbers for Field 1 and Field 2 that correspond
				to the start-of-active-video (SAV).
	@param[in]	inStandard	Specifies the video standard of interest.
	@return		The NTV2SmpteLineNumber structure that corresponds to the given video standard.
**/
inline NTV2SmpteLineNumber GetSmpteLineNumber (const NTV2Standard inStandard)	{return NTV2SmpteLineNumber (inStandard);}


typedef std::vector <NTV2DeviceID>			NTV2DeviceIDList;			///< @brief	An ordered list of NTV2DeviceIDs.
typedef NTV2DeviceIDList::iterator			NTV2DeviceIDListIter;		///< @brief	A convenient non-const iterator for NTV2DeviceIDList.
typedef NTV2DeviceIDList::const_iterator	NTV2DeviceIDListConstIter;	///< @brief	A convenient const iterator for NTV2DeviceIDList.

AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2DeviceIDList & inSet);		///<	@brief	Handy ostream writer for NTV2DeviceIDList.


typedef std::set <NTV2DeviceID>			NTV2DeviceIDSet;			///< @brief	A set of NTV2DeviceIDs.
typedef NTV2DeviceIDSet::iterator		NTV2DeviceIDSetIter;		///< @brief	A convenient non-const iterator for NTV2DeviceIDSet.
typedef NTV2DeviceIDSet::const_iterator	NTV2DeviceIDSetConstIter;	///< @brief	A convenient const iterator for NTV2DeviceIDSet.

/**
	@brief		Returns an NTV2DeviceIDSet of devices supported by the SDK.
	@param[in]	inKinds		Optionally specifies an ::NTV2DeviceKinds filter. Defaults to ::NTV2_DEVICEKIND_ALL.
	@return		An ::NTV2DeviceIDSet of devices supported by the SDK.
**/
AJAExport NTV2DeviceIDSet NTV2GetSupportedDevices (const NTV2DeviceKinds inKinds = NTV2_DEVICEKIND_ALL);

AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2DeviceIDSet & inSet);		///<	@brief	Handy ostream writer for NTV2DeviceIDSet.

/**
	@brief		Returns an SDK version component value.
	@param[in]	inVersionComponent	Optionally specifies which version component to return.
									Legal values are 0 (major version), 1 (minor version), 2 (point or "dot" version),
									and 3 (build number).  Defaults to 0 (the major version).
	@return		The value of the requested version component, or zero if an invalid version component is specified.
**/
AJAExport UWord NTV2GetSDKVersionComponent (const int inVersionComponent = 0);	//	New in SDK 16.1


typedef std::vector <NTV2OutputCrosspointID>		NTV2OutputCrosspointIDs;			///< @brief	An ordered sequence of NTV2OutputCrosspointID values.
typedef NTV2OutputCrosspointIDs::iterator			NTV2OutputCrosspointIDsIter;		///< @brief	A convenient non-const iterator for NTV2OutputCrosspointIDs.
typedef NTV2OutputCrosspointIDs::const_iterator		NTV2OutputCrosspointIDsConstIter;	///< @brief	A convenient const iterator for NTV2OutputCrosspointIDs.

AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2OutputCrosspointIDs & inList);	///<	@brief	Handy ostream writer for NTV2OutputCrosspointIDs.


typedef std::vector <NTV2InputCrosspointID>			NTV2InputCrosspointIDs;				///< @brief	An ordered sequence of NTV2InputCrosspointID values.
typedef NTV2InputCrosspointIDs::iterator			NTV2InputCrosspointIDsIter;			///< @brief	A convenient non-const iterator for NTV2InputCrosspointIDs.
typedef NTV2InputCrosspointIDs::const_iterator		NTV2InputCrosspointIDsConstIter;	///< @brief	A convenient const iterator for NTV2InputCrosspointIDs.

AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2OutputCrosspointIDs & inList);	///<	@brief	Handy ostream writer for NTV2OutputCrosspointIDs.


AJAExport std::string NTV2DeviceIDToString				(const NTV2DeviceID				inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2VideoFormatToString			(const NTV2VideoFormat			inValue,	const bool inUseFrameRate = false);	//	New in SDK 12.0
AJAExport std::string NTV2StandardToString				(const NTV2Standard				inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2FrameBufferFormatToString		(const NTV2FrameBufferFormat	inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2M31VideoPresetToString		(const M31VideoPreset			inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2FrameGeometryToString			(const NTV2FrameGeometry		inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2FrameRateToString				(const NTV2FrameRate			inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2InputSourceToString			(const NTV2InputSource			inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2OutputDestinationToString		(const NTV2OutputDestination	inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2ReferenceSourceToString		(const NTV2ReferenceSource		inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2RegisterWriteModeToString		(const NTV2RegisterWriteMode	inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2InterruptEnumToString			(const INTERRUPT_ENUMS			inInterruptEnumValue);
AJAExport std::string NTV2IpErrorEnumToString           (const NTV2IpError              inIpErrorEnumValue);
AJAExport std::string NTV2ChannelToString				(const NTV2Channel				inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2AudioSystemToString			(const NTV2AudioSystem			inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2AudioRateToString				(const NTV2AudioRate			inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2AudioBufferSizeToString		(const NTV2AudioBufferSize		inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2AudioLoopBackToString			(const NTV2AudioLoopBack		inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2EmbeddedAudioClockToString	(const NTV2EmbeddedAudioClock	inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2GetBitfileName				(const NTV2DeviceID				inValue,	const bool useOemNameOnWindows = false);
AJAExport bool        NTV2IsCompatibleBitfileName		(const std::string & inBitfileName, const NTV2DeviceID inDeviceID);
AJAExport NTV2DeviceID NTV2GetDeviceIDFromBitfileName	(const std::string & inBitfileName);
AJAExport std::string NTV2GetFirmwareFolderPath			(void);
AJAExport std::ostream & operator << (std::ostream & inOutStream, const RP188_STRUCT & inObj);
AJAExport std::string NTV2GetVersionString				(const bool inDetailed = false);
AJAExport std::string NTV2RegisterNumberToString		(const NTV2RegisterNumber		inValue);	//	New in SDK 12.0
AJAExport std::string AutoCircVidProcModeToString		(const AutoCircVidProcMode		inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2ColorCorrectionModeToString	(const NTV2ColorCorrectionMode	inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2InputCrosspointIDToString		(const NTV2InputCrosspointID	inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2OutputCrosspointIDToString	(const NTV2OutputCrosspointID	inValue,	const bool inForRetailDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2WidgetIDToString				(const NTV2WidgetID				inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2WidgetTypeToString			(const NTV2WidgetType			inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2TaskModeToString				(const NTV2EveryFrameTaskMode	inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2RegNumSetToString				(const NTV2RegisterNumberSet &	inValue);	//	New in SDK 12.0
AJAExport std::string NTV2TCIndexToString				(const NTV2TCIndex				inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2AudioChannelPairToString		(const NTV2AudioChannelPair		inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2AudioChannelQuadToString		(const NTV2Audio4ChannelSelect	inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2AudioChannelOctetToString		(const NTV2Audio8ChannelSelect	inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2FramesizeToString				(const NTV2Framesize			inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2ModeToString					(const NTV2Mode					inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2VANCModeToString				(const NTV2VANCMode				inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2MixerKeyerModeToString		(const NTV2MixerKeyerMode		inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2MixerInputControlToString		(const NTV2MixerKeyerInputControl inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2VideoLimitingToString			(const NTV2VideoLimiting		inValue,	const bool inCompactDisplay = false);	//	New in SDK 12.0
AJAExport std::string NTV2BreakoutTypeToString			(const NTV2BreakoutType			inValue,	const bool inCompactDisplay = false);	//	New in SDK 15.5
AJAExport std::string NTV2AncDataRgnToStr				(const NTV2AncDataRgn			inValue,	const bool inCompactDisplay = false);	//	New in SDK 15.5
AJAExport std::string NTV2UpConvertModeToString			(const NTV2UpConvertMode		inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1
AJAExport std::string NTV2DownConvertModeToString		(const NTV2DownConvertMode		inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1
AJAExport std::string NTV2IsoConvertModeToString		(const NTV2IsoConvertMode		inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1

AJAExport std::string NTV2HDMIBitDepthToString			(const NTV2HDMIBitDepth			inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1
AJAExport std::string NTV2HDMIAudioChannelsToString		(const NTV2HDMIAudioChannels	inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1
AJAExport std::string NTV2HDMIProtocolToString			(const NTV2HDMIProtocol			inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1
AJAExport std::string NTV2HDMIRangeToString				(const NTV2HDMIRange			inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1
AJAExport std::string NTV2HDMIColorSpaceToString		(const NTV2HDMIColorSpace		inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1
AJAExport std::string NTV2AudioFormatToString			(const NTV2AudioFormat			inValue,	const bool inCompactDisplay = false);	//	New in SDK 16.1

AJAExport bool	convertHDRFloatToRegisterValues			(const HDRFloatValues & inFloatValues,		HDRRegValues & outRegisterValues);
AJAExport bool	convertHDRRegisterToFloatValues			(const HDRRegValues & inRegisterValues,		HDRFloatValues & outFloatValues);
AJAExport void  setHDRDefaultsForBT2020                 (HDRRegValues & outRegisterValues);
AJAExport void  setHDRDefaultsForDCIP3                  (HDRRegValues & outRegisterValues);
#if !defined(NTV2_DEPRECATE_16_1)
	inline std::string NTV2AudioMonitorSelectToString (const NTV2AudioMonitorSelect inValue, const bool inForRetailDisplay = false)	{return NTV2AudioChannelPairToString(inValue, inForRetailDisplay);}	///< @deprecated	Use ::NTV2AudioChannelPairToString instead.
#endif	//	!defined(NTV2_DEPRECATE_16_1)

typedef std::vector <std::string>		NTV2StringList;			//	New in SDK 12.5
typedef NTV2StringList::iterator		NTV2StringListIter;		//	New in SDK 16.0
typedef NTV2StringList::const_iterator	NTV2StringListConstIter;//	New in SDK 12.5
typedef std::set <std::string>			NTV2StringSet;			//	New in SDK 12.5
typedef NTV2StringSet::const_iterator	NTV2StringSetConstIter;	//	New in SDK 12.5

AJAExport std::string NTV2EmbeddedAudioInputToString	(const NTV2EmbeddedAudioInput	inValue,	const bool inCompactDisplay = false);	//	New in SDK 13.0
AJAExport std::string NTV2AudioSourceToString			(const NTV2AudioSource			inValue,	const bool inCompactDisplay = false);	//	New in SDK 13.0

AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2StringList & inData);	//	New in SDK 15.5
AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2StringSet & inData);


AJAExport NTV2RegisterReads	FromRegNumSet	(const NTV2RegNumSet &		inRegNumSet);
AJAExport NTV2RegNumSet		ToRegNumSet		(const NTV2RegisterReads &	inRegReads);
AJAExport bool				GetRegNumChanges (const NTV2RegNumSet & inBefore, const NTV2RegNumSet & inAfter, NTV2RegNumSet & outGone, NTV2RegNumSet & outSame, NTV2RegNumSet & outAdded);
AJAExport bool				GetChangedRegisters (const NTV2RegisterReads & inBefore, const NTV2RegisterReads & inAfter, NTV2RegNumSet & outChanged);	//	New in SDK 16.0

AJAExport std::string		PercentEncode (const std::string & inStr);	///< @return	The URL-encoded input string.
AJAExport std::string		PercentDecode (const std::string & inStr);	///< @return	The URL-decoded input string.


//	FUTURE	** THESE WILL BE DISAPPEARING **		Deprecate in favor of the new "NTV2xxxxxxToString" functions...
#define	NTV2CrosspointIDToString	NTV2OutputCrosspointIDToString	///< @deprecated	Use NTV2OutputCrosspointIDToString
#if !defined (NTV2_DEPRECATE)
	AJAExport NTV2_DEPRECATED_f(std::string NTV2V2StandardToString	(const NTV2V2Standard inValue,	const bool inForRetailDisplay = false));
	extern AJAExport NTV2_DEPRECATED_v(const char *	NTV2VideoFormatStrings[]);		///< @deprecated	Use NTV2VideoFormatToString instead.
	extern AJAExport NTV2_DEPRECATED_v(const char *	NTV2VideoStandardStrings[]);	///< @deprecated	Use NTV2StandardToString instead.
	extern AJAExport NTV2_DEPRECATED_v(const char *	NTV2PixelFormatStrings[]);		///< @deprecated	Use NTV2FrameBufferFormatToString instead.
	extern AJAExport NTV2_DEPRECATED_v(const char *	NTV2FrameRateStrings[]);		///< @deprecated	Use NTV2FrameRateToString instead.
	extern AJAExport NTV2_DEPRECATED_v(const char *	frameBufferFormats[]);			///< @deprecated	Use NTV2FrameBufferFormatToString instead.

	AJAExport NTV2_DEPRECATED_f(std::string		frameBufferFormatString		(NTV2FrameBufferFormat inFrameBufferFormat));		///< @deprecated	Use NTV2FrameBufferFormatToString and pass 'true' for 'inForRetailDisplay'
	AJAExport NTV2_DEPRECATED_f(void			GetNTV2BoardString			(NTV2BoardID inBoardID, std::string & outString));	///< @deprecated	Use NTV2DeviceIDToString and concatenate a space instead

	AJAExport NTV2_DEPRECATED_f(std::string		NTV2BoardIDToString			(const NTV2BoardID inValue, const bool inForRetailDisplay = false));	///< @deprecated	Use NTV2DeviceIDToString(NTV2DeviceID,bool) instead.
	AJAExport NTV2_DEPRECATED_f(void			GetNTV2RetailBoardString	(NTV2BoardID inBoardID, std::string & outString));	///< @deprecated	Use NTV2DeviceIDToString(NTV2DeviceID,bool) instead.
	AJAExport NTV2_DEPRECATED_f(NTV2BoardType	GetNTV2BoardTypeForBoardID	(NTV2BoardID inBoardID));							///< @deprecated	This function is obsolete because NTV2BoardType is obsolete.
#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2UTILS_H
