/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2fixed.h
	@brief		Declares several fixed-point math routines. Assumes 16-bit fraction.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2FIXED_H
#define NTV2FIXED_H

#include "ajatypes.h"

#define FIXED_ONE (1<<16)

#ifdef  MSWindows
	//Visual Studio 2008 or lower
	#if (_MSC_VER <= 1500)
		#define inline __inline
	#endif
#endif

#if defined (AJAMac) | defined (AJAVirtual)
	//	MacOS still defines the functions below in CarbonCore/FixMath.h
	#ifdef FixedRound
		#undef FixedRound
	#endif
	#ifdef FloatToFixed
		#undef FloatToFixed
	#endif
	#ifdef FixedToFloat
		#undef FixedToFloat
	#endif
	#if !defined (FixedTrunc)
        //	Conflicts with FixedTrunc function in AJABase's videoutilities.h
		#define FixedTrunc(__x__) ((__x__)>>16)
	#endif
	#define FixedRound(__x__)					(((__x__) < 0) ? (-((-(__x__)+0x8000)>>16)) :  (((__x__) + 0x8000)>>16))
	#define FixedMix(__min__,__max__,__mixer__) (FixedRound(((__max__)-(__min__))*(__mixer__)+(__min__)))
	#define FloatToFixed(__x__)					((Fixed_)((__x__) * (float)FIXED_ONE))
	#define FixedToFloat(__x__)					(((float)(__x__) / (float) 65536.0))
	#define FixedFrac(__x__)					(((__x__) < 0) ? (-(__x__) & 0xFFFF)) :  ((__x__) & 0xFFFF)

#else	//	not AJAMac

	// Prevent unsupported-floating link errors in Linux device driver.
	// The following are not used in the Linux driver but cause the 2.6 kernel 
	// linker to fail anyway.
	#ifndef __KERNEL__
		inline	Fixed_	FloatToFixed(float inFlt)
		{ 
		  return (Fixed_)(inFlt * (float)FIXED_ONE); 
		}

		inline	float	FixedToFloat(Fixed_ inFix)
		{ 
		  return((float)inFix/(float)65536.0); 
		}
	#endif	//	if __KERNEL__ undefined

	inline	Word	FixedRound(Fixed_ inFix)
	{ 
	  Word retValue;
	  
	  if ( inFix < 0 ) 
	  {
		retValue = (Word)(-((-inFix+0x8000)>>16));
	  }
	  else
	  {
		retValue = (Word)((inFix + 0x8000)>>16);
	  }
	  return retValue;
	}

	inline	Fixed_	FixedFrac(Fixed_ inFix)
	{ 
	  Fixed_ retValue;
	  
	  if ( inFix < 0 ) 
	  {
		retValue = -inFix&0xFFFF;
	  }
	  else
	  {
		retValue = inFix&0xFFFF;
	  }
	  
	  return retValue;
	}

	inline Fixed_ FixedTrunc(Fixed_ inFix)
	{
	  return (inFix>>16);
	}

	inline Word FixedMix(Word min, Word max, Fixed_ mixer)
	{
		Fixed_ result = (max-min)*mixer+min;

		return FixedRound(result);
	}
#endif	//	else not AJAMac

#endif	//	NTV2FIXED_H
