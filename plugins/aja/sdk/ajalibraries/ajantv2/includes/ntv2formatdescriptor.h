/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2formatdescriptor.h
	@brief		Declares the NTV2FormatDescriptor class.
	@copyright	(C) 2016-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2FORMATDESC_H
#define NTV2FORMATDESC_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"
#include <string>
#include <iostream>
#include <vector>
#if defined (AJALinux)
	#include <stdint.h>
#endif


typedef std::vector <ULWord>					NTV2RasterLineOffsets;				///< @brief	An ordered sequence of zero-based line offsets into a frame buffer.
typedef NTV2RasterLineOffsets::const_iterator	NTV2RasterLineOffsetsConstIter;		///< @brief	A handy const iterator into an NTV2RasterLineOffsets.
typedef NTV2RasterLineOffsets::iterator			NTV2RasterLineOffsetsIter;			///< @brief	A handy non-const iterator into an NTV2RasterLineOffsets.

/**
	@brief		Streams a human-readable dump of the given NTV2RasterLineOffsets sequence into the specified output stream.
	@param[in]	inObj			Specifies the NTV2RasterLineOffsets to be streamed to the output stream.
	@param		inOutStream		Specifies the output stream to receive the dump. Defaults to std::cout.
	@return		A non-constant reference to the given output stream.
**/
AJAExport std::ostream & NTV2PrintRasterLineOffsets (const NTV2RasterLineOffsets & inObj, std::ostream & inOutStream = std::cout);

/**
	@brief	This provides additional information about a video frame for a given video standard or format and pixel format,
			including the total number of lines, number of pixels per line, line pitch, and which line contains the start
			of active video.
	@note	It is possible to construct a format descriptor that is not supported by the AJA device.
**/
class AJAExport NTV2FormatDescriptor
{
public:
	/**
		@name	Constructors
	**/
	///@{
	/**
		@brief	My default constructor initializes me in an "invalid" state.
	**/
	explicit		NTV2FormatDescriptor ();	///< @brief	My default constructor

	/**
		@brief	Construct from line and pixel count, plus line pitch.
		@param[in]	inNumLines			Specifies the total number of lines.
		@param[in]	inNumPixels			Specifies the total number of pixels.
		@param[in]	inLinePitch			Specifies the line pitch as the number of 32-bit words per line.
		@param[in]	inFirstActiveLine	Optionally specifies the first active line of video, where zero is the first (top) line. Defaults to zero.
	**/
	explicit 		NTV2FormatDescriptor (	const ULWord inNumLines,
											const ULWord inNumPixels,
											const ULWord inLinePitch,
											const ULWord inFirstActiveLine = 0);
#if !defined (NTV2_DEPRECATE_13_0)
	explicit	NTV2_DEPRECATED_f(NTV2FormatDescriptor (const NTV2Standard inVideoStandard,
														const NTV2FrameBufferFormat inFrameBufferFormat,
														const bool inVANCenabled,
														const bool in2Kby1080 = false,
														const bool inWideVANC = false));	///< @deprecated	Use the constructor that accepts an ::NTV2VANCMode parameter instead.

	explicit	NTV2_DEPRECATED_f(NTV2FormatDescriptor (const NTV2VideoFormat inVideoFormat,
														const NTV2FrameBufferFormat inFrameBufferFormat,
														const bool inVANCenabled,
														const bool inWideVANC = false));	///< @deprecated	Use the constructor that accepts an ::NTV2VANCMode parameter instead.
#endif	//	!defined (NTV2_DEPRECATE_13_0)

	/**
		@brief		Constructs me from the given video standard, pixel format, and VANC settings.
		@param[in]	inStandard				Specifies the video standard being used.
		@param[in]	inFrameBufferFormat		Specifies the pixel format of the frame buffer.
		@param[in]	inVancMode				Specifies the VANC mode. Defaults to OFF.
	**/
	explicit		NTV2FormatDescriptor (	const NTV2Standard			inStandard,
											const NTV2FrameBufferFormat	inFrameBufferFormat,
											const NTV2VANCMode			inVancMode		= NTV2_VANCMODE_OFF);

	/**
		@brief		Constructs me from the given video format, pixel format and VANC settings.
		@param[in]	inVideoFormat			Specifies the video format being used.
		@param[in]	inFrameBufferFormat		Specifies the pixel format of the frame buffer.
		@param[in]	inVancMode				Specifies the VANC mode.
	**/
	explicit		NTV2FormatDescriptor (	const NTV2VideoFormat		inVideoFormat,
											const NTV2FrameBufferFormat	inFrameBufferFormat,
											const NTV2VANCMode			inVancMode	= NTV2_VANCMODE_OFF);
	///@}
	
	/**
		@name	Inquiry
	**/
	///@{
	inline bool		IsValid (void) const		{return numLines && numPixels && mNumPlanes && mLinePitch[0];}	///< @return	True if valid;  otherwise false.
	inline bool		IsVANC (void) const			{return firstActiveLine > 0;}									///< @return	True if VANC geometry;  otherwise false.
	inline bool		IsPlanar (void) const		{return GetNumPlanes() > 1 || NTV2_IS_FBF_PLANAR (mPixelFormat);}	///< @return	True if planar format;  otherwise false.

	/**
		@return		The number of samples spanned per sample used, usually 1 for most formats.
					Returns zero if an invalid plane is specified.
		@param[in]	inPlaneIndex0		Specifies the plane of interest. Defaults to zero.
		@note		Used for asymmetric vertical sampling, such as 3-plane 4:2:0 formats, where
					the returned value is 1 for the Y plane, and 2 for the Cb/Cr planes.
	**/
	ULWord			GetVerticalSampleRatio (const UWord inPlaneIndex0 = 0) const;	//	New in SDK 16.0

	/**
		@return		The total number of bytes required to hold the raster plane, including any VANC lines.
					Returns zero upon error.
		@note		To determine the byte count of all planes of a planar format, call GetTotalBytes.
		@param[in]	inPlaneIndex0		Specifies the plane of interest. Defaults to zero.
	**/
	inline ULWord	GetTotalRasterBytes (const UWord inPlaneIndex0 = 0) const
					{	const ULWord vSamp(GetVerticalSampleRatio(inPlaneIndex0));
						return vSamp ? (GetFullRasterHeight() * GetBytesPerRow(inPlaneIndex0) / vSamp) : 0;
					}

	/**
		@return		The total number of bytes required to hold the raster, including any VANC, and all planes of planar formats.
	**/
	ULWord			GetTotalBytes (void) const;	//	New in SDK 16.0

	/**
		@return		The total number of bytes required to hold the visible raster (i.e. active lines after any VANC lines).
		@param[in]	inPlaneIndex0	Specifies the plane of interest. Defaults to zero, the first plane.
	**/
	inline ULWord	GetVisibleRasterBytes (const UWord inPlaneIndex0 = 0) const	{return GetVisibleRasterHeight() * GetBytesPerRow(inPlaneIndex0);}

	/**
		@return		The number of bytes per row/line of the raster.
		@param[in]	inPlaneIndex0	Specifies the plane of interest. Defaults to zero.
	**/
	inline ULWord	GetBytesPerRow (const UWord inPlaneIndex0 = 0) const	{return inPlaneIndex0 < mNumPlanes ? mLinePitch[inPlaneIndex0] : 0;}

	inline ULWord	GetRasterWidth (void) const			{return numPixels;}		///< @return	The width of the raster, in pixels.
	inline UWord	GetNumPlanes (void) const			{return mNumPlanes;}	///< @return	The number of planes in the raster.
	std::string		PlaneToString (const UWord inPlaneIndex0) const;			///< @return	A string containing a human-readable name for the specified plane.

	/**
		@return		The zero-based index number of the plane that contains the byte at the given offset,
					or 0xFFFF if the offset is not within any plane in the buffer.
		@param[in]	inByteOffset	The offset, in bytes, to the byte of interest in the frame.
	**/
	UWord			ByteOffsetToPlane (const ULWord inByteOffset) const;

	/**
		@return		The zero-based index number of the raster line (row) that contains the byte at the given offset,
					or 0xFFFF if the offset does not fall within any plane or line in the buffer.
		@param[in]	inByteOffset	The offset, in bytes, to the byte of interest in the raster buffer.
	**/
	UWord			ByteOffsetToRasterLine (const ULWord inByteOffset) const;

	/**
		@return		True if the given byte offset is at the start of a new raster line (row);  otherwise false.
		@param[in]	inByteOffset	The offset, in bytes, to the byte of interest in the frame.
	**/
	bool			IsAtLineStart (ULWord inByteOffset) const;

	/**
		@return	The height of the raster, in lines.
		@param[in]	inVisibleOnly	Specify true to return just the visible height;  otherwise false (the default) to return the full height.
	**/
	inline ULWord	GetRasterHeight (const bool inVisibleOnly = false) const		{return inVisibleOnly ? GetVisibleRasterHeight() : GetFullRasterHeight();}

	/**
		@return		The full height of the raster, in lines (including VANC, if any).
	**/
	inline ULWord	GetFullRasterHeight (void) const								{return numLines;}

	/**
		@return		The zero-based index number of the first active (visible) line in the raster. This will be zero for non-VANC rasters.
	**/
	inline ULWord	GetFirstActiveLine (void) const									{return firstActiveLine;}

	/**
		@return		The visible height of the raster, in lines (excluding VANC, if any).
	**/
	inline ULWord	GetVisibleRasterHeight (void) const								{return GetFullRasterHeight() - GetFirstActiveLine();}

	/**
		@brief		Answers with an NTV2_POINTER that describes the given row (and plane) given the NTV2_POINTER
					that describes the frame buffer.
		@param[in]	inFrameBuffer		Specifies the frame buffer (that includes all planes, if planar).
		@param		inOutRowBuffer		Receives the NTV2_POINTER that references the row (and plane) in the frame buffer.
		@param[in]	inRowIndex0			Specifies the row of interest in the buffer, where zero is the topmost row.
		@param[in]	inPlaneIndex0		Optionally specifies the plane of interest. Defaults to zero.
		@return		True if successful;  otherwise false.
	**/
	bool			GetRowBuffer (const NTV2_POINTER & inFrameBuffer, NTV2_POINTER & inOutRowBuffer,  const ULWord inRowIndex0,  const UWord inPlaneIndex0 = 0) const;	//	New in SDK 16.0

	/**
		@return		A pointer to the start of the given row in the given buffer, or NULL if row index is bad
					(using my description of the buffer contents).
		@param[in]	pInStartAddress		A pointer to the raster buffer.
		@param[in]	inRowIndex0			Specifies the row of interest in the buffer, where zero is the topmost row.
		@param[in]	inPlaneIndex0		Specifies the plane of interest. Defaults to zero.
	**/
	const void *	GetRowAddress (const void * pInStartAddress,  const ULWord inRowIndex0,  const UWord inPlaneIndex0 = 0) const;

	/**
		@return		A non-const pointer to the start of the given row in the given buffer, or NULL if row index is bad
					(using my description of the buffer contents).
		@param[in]	pInStartAddress		A non-const pointer to the raster buffer.
		@param[in]	inRowIndex0			Specifies the row of interest in the buffer, where zero is the topmost row.
		@param[in]	inPlaneIndex0		Specifies the plane of interest. Defaults to zero.
	**/
	void *			GetWriteableRowAddress (void * pInStartAddress,  const ULWord inRowIndex0,  const UWord inPlaneIndex0 = 0) const;

	/**
		@return		The absolute byte offset from the start of the frame buffer to the start of the given raster line
					in the given plane (or 0xFFFFFFFF if the row and/or plane indexes are bad).
		@note		This function assumes that the planes contiguously abut each other in memory, in ascending address order.
		@param[in]	inRowIndex0			Specifies the row of interest in the buffer, where zero is the topmost row.
		@param[in]	inPlaneIndex0		Specifies the plane of interest. Defaults to zero.
	**/
	ULWord			RasterLineToByteOffset (const ULWord inRowIndex0,  const UWord inPlaneIndex0 = 0) const;

	/**
		@return		A pointer to the start of the first visible row in the given buffer, or NULL if invalid
					(using my description of the buffer contents).
		@param[in]	pInStartAddress		A pointer to the raster buffer.
	**/
	inline UByte *	GetTopVisibleRowAddress (UByte * pInStartAddress) const				{return (UByte *) GetRowAddress (pInStartAddress, firstActiveLine);}

	/**
		@brief		Compares two buffers line-by-line (using my description of the buffer contents).
		@param[in]	pInStartAddress1		A valid, non-NULL pointer to the start of the first raster buffer.
		@param[in]	pInStartAddress2		A valid, non-NULL pointer to the start of the second raster buffer.
		@param[out]	outFirstChangedRowNum	Receives the zero-based row number of the first row that's different,
											or 0xFFFFFFFF if identical.
		@return		True if successful;  otherwise false.
	**/
	bool			GetFirstChangedRow (const void * pInStartAddress1, const void * pInStartAddress2, ULWord & outFirstChangedRowNum) const;

	/**
		@brief		Compares two buffers line-by-line (using my description of the buffer contents).
		@param[out]	outDiffs	Receives the ordered sequence of line offsets of the lines that differed.
								This will be empty if the two buffers are identical (or if an error occurs).
		@param[in]	pInBuffer1	Specifies the non-NULL address of the first memory buffer whose contents are to be compared.
		@param[in]	pInBuffer2	Specifies the non-NULL address of the second memory buffer whose contents are to be compared.
		@param[in]	inMaxLines	Optionally specifies the maximum number of lines to compare. If zero, all lines are compared.
								Defaults to zero (all lines).
		@return		True if successful;  otherwise false.
		@note		The buffers must be large enough to accommodate my video standard/format or else a memory access violation will occur.
	**/
	bool			GetChangedLines (NTV2RasterLineOffsets & outDiffs, const void * pInBuffer1, const void * pInBuffer2, const ULWord inMaxLines = 0) const;

	/**
		@return		The full-raster NTV2FrameDimensions (including VANC lines, if any).
	**/
	NTV2FrameDimensions				GetFullRasterDimensions (void) const;

	/**
		@return		The visible NTV2FrameDimensions (excluding VANC lines, if any).
	**/
	NTV2FrameDimensions				GetVisibleRasterDimensions (void) const;

	/**
		@brief		Answers with the equivalent SMPTE line number for the given line offset into the frame buffer I describe.
		@param[in]	inLineOffset	Specifies the zero-based line offset into the frame buffer that I describe.
		@param[out]	outSMPTELine	Receives the equivalent SMPTE line number.
		@param[out]	outIsField2		Receives true if the line number is associated with Field 2 (interlaced only); otherwise false.
		@return		True if successful;  otherwise false.
	**/
	bool							GetSMPTELineNumber (const ULWord inLineOffset, ULWord & outSMPTELine, bool & outIsField2) const;

	/**
		@brief		Answers with the equivalent line offset into the raster I describe for the given SMPTE line number.
		@param[in]	inSMPTELine		Specifies the SMPTE line number.
		@param[out]	outLineOffset	Receives the zero-based line offset into the raster I describe.
		@return		True if successful;  otherwise false.
	**/
	bool							GetLineOffsetFromSMPTELine (const ULWord inSMPTELine, ULWord & outLineOffset) const;

	/**
		@brief		Sets the given ::NTV2SegmentedXferInfo to match my raster, as a source or destination.
		@param		inSegmentInfo	Specifies the segmented transfer object to modify.
		@param[in]	inIsSource		Specify 'true' (the default) to set the "source" aspect of the transfer
									info object;  otherwise 'false' to set the "destination" aspect of it.
		@return		A non-const reference to the ::NTV2SegmentedXferInfo object.
	**/
	NTV2SegmentedXferInfo &			GetSegmentedXferInfo (NTV2SegmentedXferInfo & inSegmentInfo, const bool inIsSource = true) const;

	/**
		@return	True if I'm equal to the given NTV2FormatDescriptor.
		@param[in]	inRHS	The right-hand-side operand that I'll be compared with.
	**/
	bool							operator == (const NTV2FormatDescriptor & inRHS) const;

	/**
		@brief		Writes a human-readable description of me into the given output stream.
		@param		inOutStream		The output stream to be written into.
		@param[in]	inDetailed		If true (the default), writes a detailed description;  otherwise writes a brief one.
		@return		The output stream I was handed.
	**/
	std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = true) const;

	/**
		@brief		Writes the given frame buffer line offset as a formatted SMPTE line number into the given output stream.
		@param		inOutStream		The output stream to be written into.
		@param[in]	inLineOffset	Specifies the zero-based line offset in the frame buffer.
		@param[in]	inForTextMode	Defaults to false. If true, omits the space between the field indicator and the line number, and adds leading zeroes to line number.
		@return		The output stream I was handed.
	**/
	std::ostream &					PrintSMPTELineNumber (std::ostream & inOutStream, const ULWord inLineOffset, const bool inForTextMode = false) const;

	inline NTV2Standard				GetVideoStandard (void) const	{return mStandard;}							///< @return	The video standard I was created with.
	inline NTV2VideoFormat			GetVideoFormat (void) const		{return mVideoFormat;}						///< @return	The video format I was created with.
	inline NTV2FrameBufferFormat	GetPixelFormat (void) const		{return mPixelFormat;}						///< @return	The pixel format I was created with.
	inline NTV2VANCMode				GetVANCMode (void) const		{return mVancMode;}							///< @return	The VANC mode I was created with.
	inline bool						IsSDFormat (void) const			{return NTV2_IS_SD_VIDEO_FORMAT(GetVideoFormat()) || NTV2_IS_SD_STANDARD(GetVideoStandard());}	///< @return	True if I was created with an SD video format or standard.
	inline bool						IsQuadRaster (void) const		{return NTV2_IS_QUAD_STANDARD(mStandard) || NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat);}	///< @return	True if I was created with a 4K/UHD video format or standard.
	inline bool						IsTallVanc (void) const			{return mVancMode == NTV2_VANCMODE_TALL;}	///< @return	True if I was created with just "tall" VANC.
	inline bool						IsTallerVanc (void) const		{return mVancMode == NTV2_VANCMODE_TALLER;}	///< @return	True if I was created with "taller" VANC.
	inline NTV2FrameGeometry		GetFrameGeometry (void) const	{return mFrameGeometry;}					///< @return	The frame geometry I was created with.
	bool							Is2KFormat (void) const;		///< @return	True if I was created with a 2Kx1080 video format.
	///@}

	void							MakeInvalid (void);				///< @brief	Resets me into an invalid (NULL) state.

	private:
		friend class CNTV2CaptionRenderer;	//	The caption renderer needs to call SetPixelFormat
		inline void					SetPixelFormat (const NTV2PixelFormat inPixFmt)		{mPixelFormat = inPixFmt;}			///< @brief	Internal use only
		void						FinalizePlanar (void);			///< @brief	Completes initialization for planar formats

	//	Member Data
	public:
		ULWord					numLines;			///< @brief	Height -- total number of lines
		ULWord					numPixels;			///< @brief	Width -- total number of pixels per line
		ULWord					linePitch;			///< @brief	Number of 32-bit words per line. Shadows mLinePitch[0] * sizeof(ULWord).
		ULWord					firstActiveLine;	///< @brief	First active line of video (0 if NTV2_VANCMODE_OFF)
	private:
		NTV2Standard			mStandard;			///< @brief	My originating video standard
		NTV2VideoFormat			mVideoFormat;		///< @brief	My originating video format (if known)
		NTV2FrameBufferFormat	mPixelFormat;		///< @brief	My originating frame buffer format
		NTV2VANCMode			mVancMode;			///< @brief	My originating VANC mode
		ULWord					mLinePitch[4];		///< @brief	Number of bytes per row/line (per-plane)
		UWord					mNumPlanes;			///< @brief	Number of planes
		NTV2FrameGeometry		mFrameGeometry;		///< @brief My originating frame geometry

};	//	NTV2FormatDescriptor

typedef NTV2FormatDescriptor NTV2FormatDesc;	///< @brief Shorthand for ::NTV2FormatDescriptor


/**
	@brief		Writes the given NTV2FormatDescriptor to the specified output stream.
	@param		inOutStream		Specifies the output stream to receive the human-readable representation of the NTV2FormatDescriptor.
	@param[in]	inFormatDesc	Specifies the NTV2FormatDescriptor instance to print to the output stream.
	@return	A non-constant reference to the specified output stream.
**/
AJAExport inline std::ostream & operator << (std::ostream & inOutStream, const NTV2FormatDescriptor & inFormatDesc)	{return inFormatDesc.Print (inOutStream);}


#if !defined (NTV2_DEPRECATE_13_0)
	AJAExport NTV2FormatDescriptor GetFormatDescriptor (const NTV2Standard			inVideoStandard,
														const NTV2FrameBufferFormat	inFrameBufferFormat,
														const bool					inVANCenabled	= false,
														const bool					in2Kby1080		= false,
														const bool					inWideVANC		= false);

AJAExport NTV2FormatDescriptor GetFormatDescriptor (const NTV2VideoFormat			inVideoFormat,
													 const NTV2FrameBufferFormat	inFrameBufferFormat,
													 const bool						inVANCenabled	= false,
													 const bool						inWideVANC		= false);
#endif	//	!defined (NTV2_DEPRECATE_12_6)

/**
	@brief		Unpacks a line of NTV2_FBF_10BIT_YCBCR video into 16-bit-per-component YUV data.
	@param[in]	pIn10BitYUVLine		A valid, non-NULL pointer to the start of the line that contains the packed NTV2_FBF_10BIT_YCBCR data
									to be converted.
	@param[in]	inFormatDesc		Describes the raster.
	@param[out]	out16BitYUVLine		Receives the unpacked 16-bit-per-component YUV data. The sequence is cleared before filling.
									The UWord sequence will be Cb0, Y0, Cr0, Y1, Cb1, Y2, Cr1, Y3, Cb2, Y4, Cr2, Y5, . . .
	@return		True if successful;  otherwise false.
**/
AJAExport bool		UnpackLine_10BitYUVtoUWordSequence (const void * pIn10BitYUVLine, const NTV2FormatDescriptor & inFormatDesc, UWordSequence & out16BitYUVLine);


#if !defined (NTV2_DEPRECATE)
	extern AJAExport const NTV2FormatDescriptor formatDescriptorTable [NTV2_NUM_STANDARDS] [NTV2_FBF_NUMFRAMEBUFFERFORMATS];
#endif	//	!defined (NTV2_DEPRECATE)


#endif	//	NTV2FORMATDESC_H
