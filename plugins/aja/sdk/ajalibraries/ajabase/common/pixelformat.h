/* SPDX-License-Identifier: MIT */
/**
	@file		pixelformat.h
	@brief		Contains the declaration of the AJAPixelFormat class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _PixelFormat_Defined_
#define _PixelFormat_Defined_

#include "ajabase/common/videotypes.h"

#define IS_CION_RAW(pixFmt)	(pixFmt >= AJA_PixelFormat_RAW10 && pixFmt <= AJA_PixelFormat_RAW10_HS)

enum AJAScaleType
{
	AJAScaleTypeNone       = 0,
	AJAScaleType1280To1920 = 1 << 0,
	AJAScaleType960To1280  = 1 << 1,
	AJAScaleType1440To1920 = 1 << 2,
	AJAScaleTypeQRez       = 1 << 3,
	AJAScaleTypeHDV        = AJAScaleType960To1280 | AJAScaleType1440To1920,
	AJAScaleTypeDVCPro     = AJAScaleType960To1280 | AJAScaleType1280To1920
};


/**
	@class	AJAPixelFormat pixelformat.h "streams/common/pixelformat.h"
	@brief	Storage and utility class for AJA_PixelFormat defines.
			This class provides a means of storing and querying facts about the various AJA_PixelFormat values.
**/
class AJA_EXPORT AJAPixelFormat
{
public:

	AJAPixelFormat();
	AJAPixelFormat(AJA_PixelFormat format);
	virtual ~AJAPixelFormat();

	/**
	 *	Set current format value.
	 *
	 *	@param[in]	format    new AJA_PixelFormat.
	 */
	void			Set(AJA_PixelFormat format);

    /**
	 *  Query current format value.
	 *
	 *	@return		current AJA_PixelFormat value.
	 */
	AJA_PixelFormat	Query(void);

	/**
	 *  Query current fourCC value.
	 *
	 *	@return		current FourCC value.
	 */
	uint32_t QueryFourCC(void);

	/**
	 *  Query display name.
	 *
	 *	@return		current display name.
	 */
	const char*		QueryDisplayName(void);

	/**
	 *  Query RGB.
	 *
	 *	@return		whether or not current format is RGB.
	 */
	bool			QueryRgb(void);
	
	/**
	 *  Query max bit depth per component for specified format
	 *
	 *  @return		bit depth per component for specified format
	 */
	uint32_t		QueryBitDepth();

	/**
	 *  Query Scale Type.
	 *
	 *	@return		what type of scale format uses if any.
	 */
	AJAScaleType	QueryScaleType(void);

	/**
	 *  Static method to get number of possible formats
	 *  Useful for filling out GUI's
	 *
	 *	@return		number of possible formats
	 */
	static int		QueryNumberPossibleFormats();

	/**
	 *  Static method to get a format out of table of all possible formats
	 *
	 *	@param[in]	index   Zero-based index value.
	 *	@param[out]	fmt     Receives format for index.
	 *						Unmodified if index out of bounds.
	 *	@return		true if index within bounds
	 */
	static bool		QueryFormatAtIndex(int index,AJAPixelFormat& fmt);

	/**
	 *  Static method to see if a source and target resolution are scalable
	 *
	 *	@param[in]  bitmapWidth  width of the bitmap in PC memory
	 *  @param[in]  wireWidth    width of the bitmap in Kona memory
	 *  @param[in]  xAspect      Horizontal component of aspect ratio
	 *  @param[in]  yAspect      Vertical component of aspect ratio
	 *  @param[in]  pMatchingFormat  Pixel format used for matching
	 *  @param[out] pScalingFormat   Receives scaling pixel format
	 *	@return		returns true if one of the pixel formats will suffice for scaling
	 */
	static bool		QueryIsScalable(uint32_t bitmapWidth,uint32_t wireWidth,uint32_t xAspect,uint32_t yAspect,
		                            AJA_PixelFormat *pMatchingFormat,AJA_PixelFormat *pScalingFormat);

	/**
	 *  Static method to convert scaled x resolution
	 *
	 *	@param[in]  scaleType   type of scaling to be done
	 *  @param[in]  xIn         either the wire or bitmap resolution
	 *	@param[out] xOut        the corrected resolution
	 *  @return		returns true if conversion occurred
	 */
	static bool		ConvertWidth(AJAScaleType scaleType,int xIn,int &xOut);

	/**
	 *  Static method to convert aspect ratio from wire to bitmap
	 *
	 *	@param[in]	scaleType	Type of scaling to be done
	 *  @param[in]	hIn			Horizontal value to convert
	 *	@param[in]	vIn			Vertical value to convert
	 *	@param[out]	hOut		Converted horizontal value
	 *	@param[out]	vOut		Converted vertical value
	 *  @return		True if conversion occurred
	 */
	static bool		ConvertAspect(AJAScaleType scaleType,int hIn, int vIn, int &hOut,int &vOut);

	/**
	 *  Static method to provide a suggested scaling type and primary pixel format
	 *
	 *	@param[in]  xWire          wire resolution
	 *  @param[in]  xBitmap        bitmap resolution
	 *	@param[out] primaryFormat  unscaled pixel format
	 *	@param[out] scaleType      type of scaling that needs to occur
	 *  @return		returns true if conversion occurred
	 */
	static bool		QueryScaleType(int xWire,int xBitmap,AJA_PixelFormat &primaryFormat,AJAScaleType &scaleType);
	
	
	/**
	 *  Static method to provide expanded width for width-scaled rasters (e.g. DVCPro, HDV)
	 *
	 *	@param[in]  scaledWidth		width of scaled raster prior to full-width video expansion
	 *  @param[in]  height			height of scaled raster
	 *  @return		returns expanded width if input raster size matches known scaled raster type, otherwise returns input (scaledWidth)
	 */
	static int		QueryFullScaleWidth(int scaledWidth, int height);
	
	
	/**
	 *  Static method to provide associated pixel format (e.g. DVCPro, HDV)
	 *
	 *	@param[in]  scaledWidth		width of scaled raster prior to full-width video expansion
	 *  @param[in]  height			height of scaled raster
	 *  @return		returns associated pixel format
	 */
	static AJA_PixelFormat	QueryScaledPixelFormat(int scaledWidth, int height);

protected:
	AJA_PixelFormat m_format;
};

#endif	//	_PixelFormat_Defined_
