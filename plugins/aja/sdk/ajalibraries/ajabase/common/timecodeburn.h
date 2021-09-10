/* SPDX-License-Identifier: MIT */
/**
	@file		timecodeburn.h
	@brief		Declares the AJATimeCodeBurn class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TIMECODEBURN_H
#define AJA_TIMECODEBURN_H

#include "public.h"
//#include "ajabase/common/types.h"
#include "ajabase/common/videotypes.h"

/**
 *	Class to support burning a simple timecode over raster.
 *	@ingroup AJATimeCodeBurn
 */
class AJATimeCodeBurn
{
public:
	AJA_EXPORT AJATimeCodeBurn(void);
	AJA_EXPORT virtual ~AJATimeCodeBurn(void);
	/**
	 *	Render a small set of characters for timecode ahead of time...This needs to be called before
	 *  BurnTimeCode or BurnTimeCode will fail.
	 *
	 *	@param[in]	pixelFormat		Specifies the pixel format of the rendering buffer.
	 *	@param[in]	numPixels		Specifies the raster bitmap width.
	 *	@param[in]	numLines		Specifies the raster bitmap height.
	 */
	AJA_EXPORT bool RenderTimeCodeFont (AJA_PixelFormat pixelFormat, uint32_t numPixels, uint32_t numLines);

	/**
	 *	Burns a timecode in with simple font 00:00:00;00.
	 *
	 *
	 *	@param[in]	pBaseVideoAddress	Base address of Raster
	 *	@param[in]	inTimeCodeStr		A string containing something like "00:00:00:00"
	 *	@param[in]	inYPercent		    Percent down the screen. If 0, will make it 80.
	 *	@returns    True if successful;  otherwise false.
	 */
	AJA_EXPORT bool BurnTimeCode (void * pBaseVideoAddress, const std::string & inTimeCodeStr, const uint32_t inYPercent);

	/**
	 *	DEPRECATED: Use std::string version of this function.
	 */
	AJA_EXPORT bool BurnTimeCode (char * pBaseVideoAddress, const char * pTimeCodeString, const uint32_t percentY);

protected:

	void CopyDigit (int digitOffset,char *pFrameBuff);
	void writeV210Pixel (char **pBytePtr, int x, int c, int y);
	void writeYCbCr10PackedPlanerPixel (char **pBytePtr, int x, int y);

private:
	bool				_bRendered;			// set 'true' when Burn-In character map has been rendered
	char *				_pCharRenderMap;	// ptr to rendered Burn-In character set
	AJA_PixelFormat		_charRenderPixelFormat;	// frame buffer format of rendered characters
	int					_charRenderHeight;	// frame height for which rendered characters were rendered
	int					_charRenderWidth;	// frame width for which rendered characters were rendered
	int					_charWidthBytes;	// rendered character width in bytes
	int					_charHeightLines;	// rendered character height in frame lines
	int					_charPositionX;		// offset (in bytes) from left side of screen to first burn-in character
	int					_charPositionY;		// offset (in lines) from top of screen to top of burn-in characters
	int					_rowBytes;

};
#endif
