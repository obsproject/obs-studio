/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2boardfeatures.h
	@deprecated	Please include ntv2devicefeatures.h instead.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/
#ifndef NTV2BOARDFEATURES_H
	#define NTV2BOARDFEATURES_H

	#if defined (NTV2_DEPRECATE)
		#if defined (MSWindows)
			#pragma message("'ntv2boardfeatures.h' is deprecated -- include 'ntv2devicefeatures.h' instead")
		#else
			#warning "'ntv2boardfeatures.h' is deprecated -- include 'ntv2devicefeatures.h' instead""
		#endif
	#else
		#include "ntv2devicefeatures.h"
	#endif
#endif	//	NTV2BOARDFEATURES_H
