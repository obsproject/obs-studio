/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2endian.h
	@brief		Defines a number of handy byte-swapping macros.
	@copyright	(C) 2008-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2ENDIAN_H
#define NTV2ENDIAN_H

#include "ajatypes.h"

// unconditional endian byte swap 

#define NTV2EndianSwap16(__val__)				\
	(	((UWord(__val__)<<8) & 0xFF00)		|	\
		((UWord(__val__)>>8) & 0x00FF)	)

#define NTV2EndianSwap32(__val__)					\
	(	((ULWord(__val__)<<24) & 0xFF000000)	|	\
		((ULWord(__val__)<< 8) & 0x00FF0000)	|	\
		((ULWord(__val__)>> 8) & 0x0000FF00)	|	\
		((ULWord(__val__)>>24) & 0x000000FF)	)

#define NTV2EndianSwap64(__val__)								\
	(	((ULWord64(__val__)<<56) & 0xFF00000000000000ULL)	|	\
		((ULWord64(__val__)<<40) & 0x00FF000000000000ULL)	|	\
		((ULWord64(__val__)<<24) & 0x0000FF0000000000ULL)	|	\
		((ULWord64(__val__)<< 8) & 0x000000FF00000000ULL)	|	\
		((ULWord64(__val__)>> 8) & 0x00000000FF000000ULL)	|	\
		((ULWord64(__val__)>>24) & 0x0000000000FF0000ULL)	|	\
		((ULWord64(__val__)>>40) & 0x000000000000FF00ULL)	|	\
		((ULWord64(__val__)>>56) & 0x00000000000000FFULL)	)


#if AJATargetBigEndian	//	BigEndian (BE) target host
	
	// BigEndian-to-host (NetworkByteOrder-to-host)		(native)
	#define NTV2EndianSwap16BtoH(__val__)				(__val__)
	#define NTV2EndianSwap16HtoB(__val__)				(__val__)
	#define NTV2EndianSwap32BtoH(__val__)				(__val__)
	#define NTV2EndianSwap32HtoB(__val__)				(__val__)
	#define NTV2EndianSwap64BtoH(__val__)				(__val__)
	#define NTV2EndianSwap64HtoB(__val__)				(__val__)
	
	// LittleEndian-to-host								(translate)
	#define NTV2EndianSwap16LtoH(__val__)				NTV2EndianSwap16(__val__)
	#define NTV2EndianSwap16HtoL(__val__)				NTV2EndianSwap16(__val__)
	#define NTV2EndianSwap32LtoH(__val__)				NTV2EndianSwap32(__val__)
	#define NTV2EndianSwap32HtoL(__val__)				NTV2EndianSwap32(__val__)
	#define NTV2EndianSwap64LtoH(__val__)				NTV2EndianSwap64(__val__)
	#define NTV2EndianSwap64HtoL(__val__)				NTV2EndianSwap64(__val__)

#else	// LittleEndian (LE) target host
	
	// BigEndian-to-host (NetworkByteOrder-to-host)		(translate)
	#define NTV2EndianSwap16BtoH(__val__)				NTV2EndianSwap16(__val__)
	#define NTV2EndianSwap16HtoB(__val__)				NTV2EndianSwap16(__val__)
	#define NTV2EndianSwap32BtoH(__val__)				NTV2EndianSwap32(__val__)
	#define NTV2EndianSwap32HtoB(__val__)				NTV2EndianSwap32(__val__)
	#define NTV2EndianSwap64BtoH(__val__)				NTV2EndianSwap64(__val__)
	#define NTV2EndianSwap64HtoB(__val__)				NTV2EndianSwap64(__val__)
	
	// LittleEndian-to-host								(native)
	#define NTV2EndianSwap16LtoH(__val__)				(__val__)
	#define NTV2EndianSwap16HtoL(__val__)				(__val__)
	#define NTV2EndianSwap32LtoH(__val__)				(__val__)
	#define NTV2EndianSwap32HtoL(__val__)				(__val__)
	#define NTV2EndianSwap64LtoH(__val__)				(__val__)
	#define NTV2EndianSwap64HtoL(__val__)				(__val__)

#endif

#endif	// NTV2ENDIAN_H
