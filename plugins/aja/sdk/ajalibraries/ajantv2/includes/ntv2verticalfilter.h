/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2verticalfilter.h
	@brief		Declares the VerticalFilterLine and FieldInterpolateLine functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#ifndef VERTICALFILTER_H
#define VERTICALFILTER_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2videodefines.h"
#include "ntv2fixed.h"


AJAExport void	VerticalFilterLine (	RGBAlphaPixel *	topLine,
										RGBAlphaPixel *	midLine,
										RGBAlphaPixel *	bottomLine,
										RGBAlphaPixel *	destLine,
										LWord			numPixels	);

AJAExport void	FieldInterpolateLine (	RGBAlphaPixel *	topLine,
										RGBAlphaPixel *	bottomLine,
										RGBAlphaPixel *	destLine,
										LWord			numPixels	);

#endif	//	VERTICALFILTER_H
