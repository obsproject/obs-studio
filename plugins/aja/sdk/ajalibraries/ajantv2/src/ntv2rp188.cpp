/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2rp188.cpp
	@brief		Implements the CRP188 class. See SMPTE RP188 standard for details.
	@copyright	(C) 2007-2021 AJA Video Systems, Inc.
**/

#include "ntv2rp188.h"
#include "stdio.h" // for sprintf

using namespace std;


//--------------------------------------------------------------------------------------------------------------------
// CRP188 definition
//--------------------------------------------------------------------------------------------------------------------
#if defined (MSWindows)
#pragma warning(disable: 4800)
#endif

const int kDigColon		= 10;			// index of ':' character
const int kDigSemicolon	= 11;			// index of ';' character
const int kDigDash		= 12;			// index of '-' character
const int kDigSpace		= 13;			// index of ' ' character
const int kDigAsterisk	= 14;			// index of '*' character
const int kMaxTCChars	= 15;			// number of characters we know how to make

const int kNumBurnInChars = 11;			// number of characters in burn-in display (assume "xx:xx:xx:xx")

const int kDigitDotWidth  = 24;			// width of dot map for each character (NOTE: kDigitDotWidth must be evenly divisible by 6 if you want this to work for 10-bit YUV!)
const int kDigitDotHeight = 18;			// height of dot map for each character

//	Timecode "Font" Glyphs for burn-in:
static const char CharMap [kMaxTCChars] [kDigitDotHeight] [kDigitDotWidth]	=
{
// '0'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
	
// '1'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '2'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '3'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '4'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '5'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '6'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '7'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '8'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '9'
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0},
	{0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0},
	{0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// ':' (colon)
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// ';' (semicolon)
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '-' (dash)
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// ' ' (blank)
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},

// '*' (asterisk)
	{
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 2, 3, 2, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 3, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3, 2, 3, 2, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 2, 3, 2, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
};


static char bcd[] =
{
	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
    '0',
    '0',
    '0',
    '0',
    '0',
    '0'
};

static char hexChar[] =
{
	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
    'A',
    'B',
    'C',
    'D',
    'E',
    'F'
};

//--------------------------------------------------------------------------------------------------------------------
// Constructors
//--------------------------------------------------------------------------------------------------------------------

CRP188::CRP188()
{
	Init();
    _bInitialized = false;
}

CRP188::CRP188 (ULWord ulFrms, ULWord ulSecs, ULWord ulMins, ULWord ulHrs, const TimecodeFormat tcFormat)
{
	Init();
    SetRP188 (ulFrms, ulSecs, ulMins, ulHrs, tcFormat);
}

CRP188::CRP188 (const RP188_STRUCT & rp188, const TimecodeFormat tcFormat)
{
	Init();
	SetRP188 (rp188, tcFormat);
}

CRP188::CRP188 (const NTV2_RP188 & inRP188, const TimecodeFormat tcFormat)
{
	Init ();
	SetRP188 (inRP188, tcFormat);
}

CRP188::CRP188 (const string & sRP188, const TimecodeFormat tcFormat)
{
	Init();
    SetRP188(sRP188, tcFormat);
}

CRP188::CRP188 (ULWord frames, const TimecodeFormat tcFormat)
{
	Init();
	SetRP188 (frames, tcFormat);
}


CRP188::~CRP188()
{
	if (_pCharRenderMap != NULL)
		free(_pCharRenderMap);
}


void CRP188::Init()
{
	_pCharRenderMap = NULL;
	_bRendered      = false;
	_bInitialized   = false;
	_bFresh			= false;
	_tcFormat		= kTCFormatUnknown;
}


bool CRP188::operator==( const CRP188& s)
{
	bool bResult = true;

		// this can be fancier: we're just looking for identical hrs/mins/sec/frames
		// (but NOT checking user bits, field IDs, etc.)
	ULWord sVal[4];

	bResult &= s.GetRP188Frms(sVal[0]);
	bResult &= s.GetRP188Secs(sVal[1]);
	bResult &= s.GetRP188Mins(sVal[2]);
	bResult &= s.GetRP188Hrs (sVal[3]);

	if (bResult)
	{
		if ( (_ulVal[0] == sVal[0])	&& (_ulVal[1] == sVal[1]) && (_ulVal[2] == sVal[2])	&& (_ulVal[3] == sVal[3]) )
			bResult = true;
		else
			bResult = false;
	}

	return bResult;
}


//--------------------------------------------------------------------------------------------------------------------
// Setters
//--------------------------------------------------------------------------------------------------------------------
void CRP188::SetRP188 (const RP188_STRUCT & rp188, const TimecodeFormat tcFormat)
{
	char unitFrames;
	char tensFrames;
	char unitSeconds;
	char tensSeconds;
	char unitMinutes;
	char tensMinutes;
	char unitHours;
	char tensHours;

	if (tcFormat != kTCFormatUnknown)
		_tcFormat = tcFormat;

	if (rp188.DBB == 0xffffffff)
		return;

    ULWord TC0_31  = rp188.Low;
    ULWord TC32_63 = rp188.High;
	_bDropFrameFlag  = (TC0_31 >> 10) & 0x01;	// Drop Frame:  timecode bit 10

	if ( FormatIs60_50fps(_tcFormat) )
	{
		// for frame rates > 39 fps, the field ID correlates frame pairs
		bool bFieldID = (FormatIsPAL(_tcFormat) ? ((TC32_63 & BIT_27) != 0) : ((TC0_31 & BIT_27) != 0) );	// Note: FID is in different words for PAL & NTSC!
		int numFrames = (((((TC0_31>>8)&0x3) * 10) + (TC0_31&0xF)) * 2) + (int)bFieldID;					// double the regular frame count and add fieldID
		unitFrames = bcd[numFrames % 10];
		tensFrames = bcd[numFrames / 10];
	}
	else
	{
		unitFrames = bcd[(TC0_31   )&0xF];
		tensFrames = bcd[(TC0_31>>8)&0x3];
	}
    unitSeconds = bcd[(TC0_31>>16)&0xF];
    tensSeconds = bcd[(TC0_31>>24)&0x7];

    unitMinutes = bcd[(TC32_63    )&0xF];
    tensMinutes = bcd[(TC32_63>> 8)&0x7];
    unitHours   = bcd[(TC32_63>>16)&0xF];
    tensHours   = bcd[(TC32_63>>24)&0x3];

	_ulVal[0] = (unitFrames-0x30)+((tensFrames-0x30)*10);
	_ulVal[1] = (unitSeconds-0x30)+((tensSeconds-0x30)*10);
	_ulVal[2] = (unitMinutes-0x30)+((tensMinutes-0x30)*10);
	_ulVal[3] = (unitHours-0x30)+((tensHours-0x30)*10);

    char timecodeString[12];
    timecodeString[0] =  tensHours;
    timecodeString[1] =  unitHours;
    timecodeString[2] = ':';
    timecodeString[3] =  tensMinutes;
    timecodeString[4] =  unitMinutes;
    timecodeString[5] = ':';
    timecodeString[6] =  tensSeconds;
    timecodeString[7] =  unitSeconds;
    if (_bDropFrameFlag)
    {
        timecodeString[8] = ';';
    }
    else
    {
    	timecodeString[8] = ':';
    }
    timecodeString[9] =  tensFrames;
    timecodeString[10] = unitFrames;
    timecodeString[11] = '\0';

    _sHMSF = timecodeString;

    ConvertTcStrToVal();


    _rp188        = rp188;
    _bInitialized = true;
	_bFresh = ((rp188.DBB & NEW_SELECT_RP188_RCVD) || (rp188.DBB & BIT(18)) || (rp188.DBB & BIT(19)));

	// User bits

	_bVaricamActiveF0 = (TC0_31 >> 5) & 0x01;   // Varicam "Active Frame 1": timecode bit 5
	_bVaricamActiveF1 = (TC0_31 >> 4) & 0x01;   // Varicam "Active Frame 2": timecode bit 4

	_bDropFrameFlag  = (TC0_31 >> 10) & 0x01;	// Drop Frame:  timecode bit 10
	_bColorFrameFlag = (TC0_31 >> 11) & 0x01;	// Color Frame: timecode bit 11

	_varicamRate = (_ulUserBits[3] * 10) + _ulUserBits[2];

	if ( FormatIsPAL(_tcFormat) )
	{
		_fieldID = (TC32_63 >> 27) & 0x01;  // Field ID: timecode bit 59 (25 fps spec)
	}
	else
	{
		_fieldID = (TC0_31  >> 27) & 0x01;  // Field ID: timecode bit 27 (30 fps spec)
	}
}


void CRP188::SetRP188 (const NTV2_RP188 & inRP188, const TimecodeFormat inFormat)
{
	RP188_STRUCT  tmpRP188 (inRP188);
	SetRP188 (tmpRP188, inFormat);
}


void CRP188::SetRP188 (const string &sRP188, const TimecodeFormat tcFormat)
{
	if (tcFormat != kTCFormatUnknown)
		_tcFormat = tcFormat;


    _sHMSF = sRP188;
    ConvertTcStrToVal();

	SetRP188(_ulVal[0], _ulVal[1], _ulVal[2], _ulVal[3], _tcFormat);
}


void CRP188::SetRP188 (ULWord frames, const TimecodeFormat tcFormat)
{
	if (tcFormat != kTCFormatUnknown)
		_tcFormat = tcFormat;

	ULWord ulHrs, ulMins, ulSecs, ulFrms;

	ConvertFrameCount(frames, _tcFormat, ulHrs, ulMins, ulSecs, ulFrms);

	SetRP188(ulFrms, ulSecs, ulMins, ulHrs, _tcFormat);
}


void CRP188::SetRP188 (ULWord ulFrms, ULWord ulSecs, ULWord ulMins, ULWord ulHrs,
					   NTV2FrameRate frameRate, const bool bDropFrame, const bool bSMPTE372)
{
	TimecodeFormat tcFormat = kTCFormat30fpsDF;

	switch (frameRate)
	{
		case NTV2_FRAMERATE_6000:	tcFormat = kTCFormat60fps;	break;
		case NTV2_FRAMERATE_5994:	tcFormat = kTCFormat60fps;	break;
		case NTV2_FRAMERATE_5000:	tcFormat = kTCFormat50fps;	break;
		case NTV2_FRAMERATE_4800:	tcFormat = kTCFormat48fps;	break;
		case NTV2_FRAMERATE_3000:	tcFormat = kTCFormat30fps;	break;
		case NTV2_FRAMERATE_2997:	tcFormat = kTCFormat30fps;	break;
		case NTV2_FRAMERATE_2500:	tcFormat = kTCFormat25fps;	break;
		case NTV2_FRAMERATE_2400:	tcFormat = kTCFormat24fps;	break;
		case NTV2_FRAMERATE_2398:	tcFormat = kTCFormat24fps;	break;
		default:												break;
	}

	if(frameRate == NTV2_FRAMERATE_2400 && bSMPTE372)
		tcFormat = kTCFormat48fps;

	if(frameRate == NTV2_FRAMERATE_3000 && bSMPTE372)
		tcFormat = kTCFormat60fps;

	if(frameRate == NTV2_FRAMERATE_2997 && bSMPTE372)
		tcFormat = kTCFormat60fps;

	if(frameRate == NTV2_FRAMERATE_2500 && bSMPTE372)
		tcFormat = kTCFormat50fps;

	if (tcFormat == kTCFormat30fps && bDropFrame)
		tcFormat = kTCFormat30fpsDF;

	if (tcFormat == kTCFormat60fps && bDropFrame)
		tcFormat = kTCFormat60fpsDF;

	SetRP188(ulFrms, ulSecs, ulMins, ulHrs, tcFormat);
}


void CRP188::SetRP188 (ULWord ulFrms, ULWord ulSecs, ULWord ulMins, ULWord ulHrs, const TimecodeFormat tcFormat)
{
	//printf("SetRP188 ulFrms = %d, ulSecs = %d, ulMins = %d, ulHrs = %d, framesPerSecond = %d, bDropFrame = %d, b50Hz = %d\n",
	//	ulFrms, ulSecs, ulMins, ulHrs, framesPerSecond, bDropFrame, b50Hz);

	if (tcFormat != kTCFormatUnknown)
		_tcFormat = tcFormat;

	bool bDropFrame = FormatIsDropFrame(_tcFormat);

	ULWord framesPerSecond = FramesPerSecond(_tcFormat);

    // insure that TC elements are valid
    if (ulFrms >= framesPerSecond)
    {
        ulSecs += ulFrms / framesPerSecond;
        ulFrms %= framesPerSecond;
    }

    if (ulSecs > 59)
    {
        ulMins += ulSecs / 60;
        ulSecs %= 60;
    }

    if (ulMins > 59)
    {
        ulHrs += ulMins / 60;
        ulMins %= 60;
    }

    if (ulHrs >= 24)
		ulHrs %= 24;

		// error check: if this is a DropFrame mode, chop off any frames that should have been dropped
	if (bDropFrame)
	{
		if (_tcFormat == kTCFormat30fpsDF)
		{
				// there shouldn't be any frames that look like "XX:XX:00:00" or "XX:XX:00:01" unless
				// we're on an exact 10 minute multiple
			if ( (ulSecs == 0) && ((ulFrms == 0) || (ulFrms == 1)) )
			{
				if ((ulMins % 10) != 0)		// not on a "10s" minute? (00, 10, 20, ...)
				{
						// bad frame digit - round up to "02"
					ulFrms = 2;
				}
			}
		}
		else if (_tcFormat == kTCFormat60fpsDF)
		{
			// As far as I know there is no "standard" for 60 fps DropFrame timecode, so we don't really
			// know which frame(s) to drop. In the mean time, we're going to simply assume that 60 fps
			// timecode is just like 30 fps timecode with "field" (or half-frame) precision...?

				// there shouldn't be any frames that look like "XX:XX:00:00" or "XX:XX:00:01" or
				// "XX:XX:00:02" or "XX:XX:00:03" unless we're on an exact 10 minute multiple
			if ( (ulSecs == 0) && ((ulFrms == 0) || (ulFrms == 1) || (ulFrms == 2) || (ulFrms == 3)) )
			{
				if ((ulMins % 10) != 0)		// not on a "10s" minute? (00, 10, 20, ...)
				{
						// bad frame digit - round up to "04"
					ulFrms = 4;
				}
			}
		}
	}

    _ulVal[0] = ulFrms;
    _ulVal[1] = ulSecs;
    _ulVal[2] = ulMins;
    _ulVal[3] = ulHrs;

    // format TC
    char timeCodeString[80];
	if (bDropFrame)
		sprintf(timeCodeString,"%02d:%02d:%02d;%02d", ulHrs, ulMins, ulSecs, ulFrms);	// drop frame uses ';'
	else
		sprintf(timeCodeString,"%02d:%02d:%02d:%02d", ulHrs, ulMins, ulSecs, ulFrms);	// non-drop uses ':'

    _sHMSF = timeCodeString;

	// Initialize the rp188 struct with TC and set all other fields to zero
    ConvertTcStrToReg();

	_bInitialized = true;
	_bFresh		  = false;			// we need to see the DBB reg to know...
	SetVaricamFrameActive(false, 0);
	SetVaricamFrameActive(false, 1);

	SetDropFrame(bDropFrame);
	SetColorFrame(false);
	SetVaricamRate( DefaultFrameRateForTimecodeFormat(_tcFormat) );

	if ( !FormatIs60_50fps(_tcFormat) )		// if we're using 50 or 60 fps format, the field id signifies frame pairs
		SetFieldID(0);

	SetBFGBits(false, false, false);

	//printf("RP188 _rp188.Low = %x, _rp188.High = %x, _rp188.DBB = %x, tcS = %s\n", _rp188.Low, _rp188.High, _rp188.DBB, _sHMSF.c_str());
}




void CRP188::SetDropFrame(bool bDropFrameFlag)
{
	// Set member var and rp188 struct bit
	_bDropFrameFlag = bDropFrameFlag;
	if (bDropFrameFlag == true)
		_rp188.Low |= BIT_10;
	else
		_rp188.Low &= ~BIT_10;
}

void CRP188::SetColorFrame(bool bColorFrameFlag)
{
	// Set member var and rp188 struct bit
	_bColorFrameFlag = bColorFrameFlag;
	if (bColorFrameFlag == true)
		_rp188.Low |= BIT_11;
	else
		_rp188.Low &= ~BIT_11;
}

void CRP188::SetVaricamFrameActive(bool bVaricamActive, ULWord frame)
{
	if (frame == 0)
	{
		_bVaricamActiveF0 = bVaricamActive;
		if (bVaricamActive)
			_rp188.Low |= BIT_5;
		else
			_rp188.Low &= ~BIT_5;
	}
	else
	{
		_bVaricamActiveF1 = bVaricamActive;
		if (bVaricamActive)
			_rp188.Low |= BIT_4;
		else
			_rp188.Low &= ~BIT_4;
	}
}

void CRP188::SetVaricamRate(NTV2FrameRate frameRate)
{
	ULWord rate;

	switch (frameRate)
	{
		case NTV2_FRAMERATE_6000:
		case NTV2_FRAMERATE_5994:
			rate = 0x60;
			break;

		case NTV2_FRAMERATE_5000:
			rate = 0x50;
			break;

		case NTV2_FRAMERATE_4800:
		case NTV2_FRAMERATE_4795:
			rate = 0x48;
			break;

		case NTV2_FRAMERATE_3000:
		case NTV2_FRAMERATE_2997:
			rate = 0x30;
			break;

		case NTV2_FRAMERATE_2500:
			rate = 0x25;
			break;

		case NTV2_FRAMERATE_2400:
		case NTV2_FRAMERATE_2398:
			rate = 0x24;
			break;
		case NTV2_FRAMERATE_1500:
		case NTV2_FRAMERATE_1498:
			rate = 0x15;
			break;
		default:
			rate = 0;
			break;
	}
	_rp188.Low &= 0x0F0FFFFF;
	_rp188.Low |= (rate & 0x0f) << 20;
	_rp188.Low |= (rate & 0xf0) << 24;			// we really shift over to last group but nibble is already 4 bits over
}


void CRP188::SetFieldID(ULWord fieldID)
{
	_fieldID = fieldID;

	if ( FormatIsPAL(_tcFormat) )
	{
		if (fieldID)
			_rp188.High |=  BIT_27;				// Note: PAL is bit 27 of HIGH word
		else
			_rp188.High &= ~BIT_27;
	}
	else
	{
		if (fieldID)
			_rp188.Low |=  BIT_27;				// NTSC is bit 27 of LOW word
		else
			_rp188.Low &= ~BIT_27;
	}
}

bool CRP188::GetFieldID(void)
{
	bool bResult = (_fieldID != 0);

	if ( FormatIsPAL(_tcFormat) )
		bResult = ((_rp188.High & BIT_27) != 0);			// Note: PAL is bit 27 of HIGH word
	else
		bResult = ((_rp188.Low & BIT_27) != 0);			// NTSC is bit 27 of LOW word

	return bResult;
}

void CRP188::SetBFGBits(bool bBFG0, bool bBFG1, bool bBFG2)
{
	// Same place for 60Hz and 50Hz
	if (bBFG1)
		_rp188.High |= BIT_26;
	else
		_rp188.High &= ~BIT_26;


	if ( FormatIsPAL(_tcFormat) )
	{
		if (bBFG0)
			_rp188.Low |=  BIT_27;
		else
			_rp188.Low &= ~BIT_27;

		if (bBFG2)
			_rp188.High |=  BIT_11;
		else
			_rp188.High &= ~BIT_11;
	}
	else
	{
		if (bBFG0)
			_rp188.High |=  BIT_11;
		else
			_rp188.High &= ~BIT_11;

		if (bBFG2)
			_rp188.High |=  BIT_27;
		else
			_rp188.High &= ~BIT_27;
	}
}


void CRP188::SetSource (UByte src)
{
	_rp188.DBB = (_rp188.DBB & ~0xFF000000) | (src << 24);
}


UByte CRP188::GetSource () const
{
	ULWord val = (_rp188.DBB & 0xFF000000) >> 24;
	return (UByte) val;
}

void CRP188::SetOutputFilter (UByte src)
{
	_rp188.DBB = (_rp188.DBB & ~0x000000FF) | (src);
}


UByte CRP188::GetOutputFilter () const
{
	ULWord val = (_rp188.DBB & 0x000000FF);
	return (UByte) val;
}



//--------------------------------------------------------------------------------------------------------------------
// Getters
//--------------------------------------------------------------------------------------------------------------------
bool CRP188::GetRP188Str (string & sRP188) const
{
    sRP188 = _sHMSF;
    return _bInitialized;
}

const char *CRP188::GetRP188CString () const
{
    return ( _sHMSF.c_str() );
}

bool CRP188::GetRP188Frms (ULWord & ulFrms) const
{
    ulFrms = _ulVal[0];
    return _bInitialized;
}

bool CRP188::GetRP188Secs (ULWord & ulSecs) const
{
    ulSecs = _ulVal[1];
    return _bInitialized;
}

bool CRP188::GetRP188Mins (ULWord & ulMins) const
{
    ulMins = _ulVal[2];
    return _bInitialized;
}

bool CRP188::GetRP188Hrs  (ULWord & ulHrs) const
{
    ulHrs = _ulVal[3];
    return _bInitialized;
}

bool CRP188::GetFrameCount (ULWord & frameCount)
{
	ConvertTimecode (frameCount, _tcFormat, _ulVal[3], _ulVal[2], _ulVal[1], _ulVal[0]);

	return _bInitialized;
}


void CRP188::ConvertTimecode (ULWord & frameCount, TimecodeFormat format, ULWord hours, ULWord minutes, ULWord seconds, ULWord frames)
{
	ULWord frms = 0;

	if (!FormatIsDropFrame(format) )
	{
		// non-drop
		int mins = (60 * hours) + minutes;					// 60 minutes / hour
		int secs = (60 * mins)  + seconds;					// 60 seconds / minute
		frms  = (FramesPerSecond(format) * secs) + frames;	//    frames  / second
	}

	else
	{
		ULWord framesPerSec		   = FramesPerSecond(format);
		ULWord framesPerMin		   = framesPerSec * 60;								// 60 seconds/minute

		ULWord droppedFrames	   = (format == kTCFormat60fpsDF ? 4 : 2);			// number of frames dropped in a "drop second"
		ULWord dropFramesPerSec	   = framesPerSec - droppedFrames;					// "drop-seconds" have all frames but two (or 4)...
		ULWord dropframesPerMin    = (59 * framesPerSec) + dropFramesPerSec;		// "drop-minutes" have 1 drop and 59 regular seconds
		ULWord dropframesPerTenMin = (9 * dropframesPerMin) + framesPerMin;			// every ten minutes we get 1 regular and 9 drop minutes
		ULWord dropframesPerHr     = dropframesPerTenMin * 6;						// 60 minutes/hr.

		frms = hours * dropframesPerHr;				// convert hours

		int tenMins = minutes / 10;
		frms += tenMins * dropframesPerTenMin;		// convert 10's of minutes

		int mins = minutes % 10;					// convert minutes
		if (mins > 0)
		{
			frms += framesPerMin;					// the first minute (out of ten) is always a non-drop
			mins--;

			frms += mins * dropframesPerMin;		// the rest are drop frame minutes
		}

		int secs = seconds;							// convert seconds
		if (secs > 0)
		{
			if (_ulVal[2] % 10 != 0)				// if this is a drop-minute
			{
				frms += dropFramesPerSec;			// the first second (out of 59) is always a drop
				secs--;
			}

			frms += secs * framesPerSec;			// the rest are regular seconds
		}

		if (seconds == 0 && minutes % 10 != 0)		// convert frames1
		{
			if (frames >= droppedFrames)
				frms += (frames - droppedFrames);	// if this is a "drop-second", the frame count has been offset by droppedFrames (e.g. 2 - 29 instead of 0 - 27)
			else
				frms += 0;							// this shouldn't happen in dropframe format...
		}
		else
			frms += frames;							// otherwise count all frames
	}

	frameCount = frms;
}


	// convert a frame count to hours/minutes/seconds/frames (where frameCount "0" = 00:00:00:00)
void CRP188::ConvertFrameCount (ULWord frameCount, TimecodeFormat format, ULWord & hours, ULWord & minutes, ULWord & seconds, ULWord & frames)
{
		// non-dropframe
	ULWord framesPerSec = FramesPerSecond(format);
	ULWord framesPerMin = framesPerSec * 60;		// 60 seconds/minute
	ULWord framesPerHr  = framesPerMin * 60;		// 60 minutes/hr.
	ULWord framesPerDay = framesPerHr  * 24;		// 24 hours/day

	if ( !FormatIsDropFrame(format) )
	{
		// non-dropframe
			// make sure we don't have more than 24 hours worth of frames
		frameCount = frameCount % framesPerDay;

			// how many hours?
		hours = frameCount / framesPerHr;
		frameCount = frameCount % framesPerHr;

			// how many minutes?
		minutes = frameCount / framesPerMin;
		frameCount = frameCount % framesPerMin;

			// how many seconds?
		seconds = frameCount / framesPerSec;

			// what's left is the frame count
		frames = frameCount % framesPerSec;
	}

	else
	{
		// dropframe
		ULWord droppedFrames	   = (_tcFormat == kTCFormat60fpsDF ? 4 : 2);	// number of frames dropped in a "drop second"
		ULWord dropFramesPerSec	   = framesPerSec - droppedFrames;
		ULWord dropframesPerMin    = (59 * framesPerSec) + dropFramesPerSec;	// every minute we get 1 drop and 59 regular seconds
		ULWord dropframesPerTenMin = (9 * dropframesPerMin) + framesPerMin;		// every ten minutes we get 1 regular and 9 drop minutes
		ULWord dropframesPerHr     = dropframesPerTenMin * 6;					// 60 minutes/hr.
		ULWord dropframesPerDay    = dropframesPerHr * 24;						// 24 hours/day

			// make sure we don't have more than 24 hours worth of frames
		frameCount = frameCount % dropframesPerDay;

			// how many hours?
		hours  = frameCount / dropframesPerHr;
		frameCount = frameCount % dropframesPerHr;

			// how many tens of minutes?
		minutes = 10 * (frameCount / dropframesPerTenMin);
		frameCount = frameCount % dropframesPerTenMin;

			// how many units of minutes?
		if (frameCount >= framesPerMin)
		{
			minutes += 1;	// got at least one minute (the first one is a non-drop minute)
			frameCount  = frameCount - framesPerMin;

				// any remaining minutes are drop-minutes
			minutes += frameCount / dropframesPerMin;
			frameCount  = frameCount % dropframesPerMin;
		}

			// how many seconds? depends on whether this was a regular or a drop minute...
		seconds = 0;
		if (minutes % 10 == 0)
		{
				// regular minute: all seconds are full length
			seconds = frameCount / framesPerSec;
			frameCount = frameCount % framesPerSec;
		}

		else
		{
				// drop minute: the first second is a drop second
			if (frameCount >= dropFramesPerSec)
			{
				seconds += 1;	// got at least one (the first one is a drop second)
				frameCount = frameCount - dropFramesPerSec;

					// any remaining seconds are full-length
				seconds += frameCount / framesPerSec;
				frameCount = frameCount % framesPerSec;
			}
		}

			// what's left is the frame count
		frames = frameCount;

			// if we happened to land on a drop-second, add 2 frames (the 28 frames are numbered 2 - 29, not 0 - 27)
		if ( (seconds == 0) && (minutes % 10 != 0))
			frames += droppedFrames;
	}
}


	// Add <frameCount> frames to current timecode
	// Accounts for 24-hr wrap in whatever format we're currently in
ULWord CRP188::AddFrames(ULWord frameCount)
{
	ULWord currentFrameCount, newFrameCount;
	GetFrameCount(currentFrameCount);

		// add
	newFrameCount = currentFrameCount + frameCount;

		// check for 24-hr rollover
	newFrameCount = newFrameCount % MaxFramesPerDay();

	SetRP188 (newFrameCount, _tcFormat);

	return newFrameCount;
}


	// Subtract <frameCount> frames from current timecode
	// Accounts for 24-hr wrap in whatever format we're currently in
ULWord CRP188::SubtractFrames (ULWord frameCount)
{
	ULWord currentFrameCount, newFrameCount;
	GetFrameCount(currentFrameCount);

		// make sure the amount we want to subtract is < 24 hrs.
	ULWord maxFrames = MaxFramesPerDay();
	ULWord subAmt = frameCount % maxFrames;

	if (subAmt <= currentFrameCount)
		newFrameCount = currentFrameCount - subAmt;					// no wrap - just subtract
	else
		newFrameCount = maxFrames - (subAmt - currentFrameCount);	// deal with 24 hr "underflow"

	SetRP188 (newFrameCount, _tcFormat);

	return newFrameCount;
}


	// returns the total number of frames in a 24 hr day for the given format
ULWord CRP188::MaxFramesPerDay(TimecodeFormat format) const
{
	ULWord result = 0;

	if (format == kTCFormatUnknown)
		format = _tcFormat;

	ULWord framesPerSec = FramesPerSecond(format);
	ULWord framesPerMin = framesPerSec * 60;		// 60 seconds/minute

	if ( !FormatIsDropFrame(format) )
	{
			// non-drop frame
		ULWord framesPerHr  = framesPerMin * 60;		// 60 minutes/hr.
		result				= framesPerHr  * 24;		// 24 hours/day
	}
	else
	{
		ULWord droppedFrames	   = (_tcFormat == kTCFormat60fpsDF ? 4 : 2);	// number of frames dropped in a "drop second"
		ULWord dropframesPerMin    = framesPerMin - droppedFrames;				// "drop-minutes" have all of the frames but two...
		ULWord dropframesPerTenMin = (9 * dropframesPerMin) + framesPerMin;		// every ten minutes we get 1 regular and 9 drop minutes
		ULWord dropframesPerHr     = dropframesPerTenMin * 6;					// 60 minutes/hr.
		result					   = dropframesPerHr * 24;						// 24 hours/day
	}

	return result;
}


	// FormatIsDropFrame()
	//    returns 'true' if the designated time code format is one of the "drop frame" formats
bool CRP188::FormatIsDropFrame(TimecodeFormat format) const
{
	bool bResult = false;

	if (format == kTCFormatUnknown)
		format = _tcFormat;

    if (format == kTCFormat30fpsDF || format == kTCFormat60fpsDF)
		bResult = true;

    return bResult;
}

	// FormatIs60_50fps()
	//    returns 'true' if the designated time code format is greater than 39 fps
	//    (requires using FieldID bit to store ls bit of frame "10s" count)
bool CRP188::FormatIs60_50fps(TimecodeFormat format) const
{
	bool bResult = false;

	if (format == kTCFormatUnknown)
		format = _tcFormat;

    if (format == kTCFormat60fps ||
		format == kTCFormat60fpsDF ||
		format == kTCFormat48fps ||
		format == kTCFormat50fps)
		bResult = true;

    return bResult;
}

	// FormatIsPAL()
	//    returns 'true' if the designated time code format is one of the "PAL" formats
	//    (uses different bit allocations for timecode status "flags")
bool CRP188::FormatIsPAL(TimecodeFormat format) const
{
	bool bResult = false;

	if (format == kTCFormatUnknown)
		format = _tcFormat;

    if (format == kTCFormat25fps || format == kTCFormat50fps)
		bResult = true;

    return bResult;
}

bool CRP188::GetRP188Reg  (RP188_STRUCT & rp188) const
{
    rp188 = _rp188;
    return _bInitialized;
}

bool CRP188::GetRP188Reg  (NTV2_RP188 & outRP188) const
{
	outRP188 = _rp188;
	return _bInitialized;
}

bool CRP188::GetRP188UserBitsStr (string & sRP188UB)
{
	// Derive the userbits and userbits string from the rp188 struct
	RP188ToUserBits();

    sRP188UB = _sUserBits;
    return _bInitialized;
}

const char *CRP188::GetRP188UserBitsCString ()
{
	// Derive the userbits and userbits string from the rp188 struct
	RP188ToUserBits();

    return ( _sUserBits.c_str() );
}

ULWord CRP188::BinaryGroup (ULWord smpteNum)
{
	ULWord result = 0;

	// Derive the userbits and userbits string from the rp188 struct
	RP188ToUserBits();

	// note: SMPTE labels the Binary Groups 1 - 8, but we store them in a zero-based array
	if ( (smpteNum >= 1) && (smpteNum <= 8) )
		result = _ulUserBits[smpteNum-1];

	return result;
}

bool CRP188::SetBinaryGroup (ULWord smpteNum, ULWord bits)
{
	// note: SMPTE labels the Binary Groups 1 - 8, but we store them in a zero-based array
	if ( (smpteNum >= 1) && (smpteNum <= 8) )
	{
		_ulUserBits[smpteNum-1] = bits;

		if ( smpteNum < 5 )
		{
			ULWord newBits = _rp188.Low;
			ULWord shift   = (smpteNum-1) * 8 + 4;

			newBits &= ~(0xF << shift);
			_rp188.Low = newBits | ((bits & 0xF) << shift);
		}
		else
		{
			ULWord newBits = _rp188.High;
			ULWord shift   = (smpteNum-5) * 8 + 4;

			newBits &= ~(0xF << shift);
			_rp188.High = newBits | ((bits & 0xF) << shift);
		}

		return true;
	}

	return false;
}

bool CRP188::SetUserBits (ULWord bits)
{
	_ulUserBits[7] = (bits >> 28) & 0xF;				// Binary Group 8
	_ulUserBits[6] = (bits >> 24) & 0xF;				// Binary Group 7
	_ulUserBits[5] = (bits >> 20) & 0xF;				// Binary Group 6
	_ulUserBits[4] = (bits >> 16) & 0xF;				// Binary Group 5
	_ulUserBits[3] = (bits >> 12) & 0xF;				// Binary Group 4
	_ulUserBits[2] = (bits >>  8) & 0xF;				// Binary Group 3
	_ulUserBits[1] = (bits >>  4) & 0xF;				// Binary Group 2
	_ulUserBits[0] = (bits >>  0) & 0xF;				// Binary Group 1

	ULWord newHigh = _rp188.High & 0x0F0F0F0F;
	ULWord newLow  = _rp188.Low  & 0x0F0F0F0F;

	newHigh |= ((bits & 0xF0000000) >>  0) |
			   ((bits & 0x0F000000) >>  4) |
			   ((bits & 0x00F00000) >>  8) |
			   ((bits & 0x000F0000) >> 12);
	newLow  |= ((bits & 0x0000F000) << 16) |
			   ((bits & 0x00000F00) << 12) |
			   ((bits & 0x000000F0) <<  8) |
			   ((bits & 0x0000000F) <<  4);

	_rp188.High = newHigh;
	_rp188.Low  = newLow;

	return true;
}

ULWord CRP188::UDW (ULWord smpteUDW)
{
	ULWord result = 0;

	// note: SMPTE labels the UDWs 1 - 16! (i.e. NOT zero-based!)
	if ( (smpteUDW >= 1) && (smpteUDW <= 16) )
	{
		int index = smpteUDW - 1;		// make it zero-based

		if (index < 8)
			result = (_rp188.Low  >> (4 * index)) & 0x0F;		// 0 - 7 come from bits 0 - 31
		else
			result = (_rp188.High >> (4 * (index-8))) & 0x0F;   // 8 - 15 come from bits 32 - 63
	}

	return result;
}

ULWord CRP188::VaricamFrameRate (void)
{
		// the VariCam "Shooting Frame Rate" is encodec as BCD digits in
		// Binary Groups 4 (tens) and 3 (ones).
	return ( (BinaryGroup(4) * 10) + BinaryGroup(3) );
}


	// note: this doesn't deal (accurately) with drop-frame
ULWord CRP188::FramesPerSecond (TimecodeFormat format) const
{
	ULWord fps = 30;

	if (format == kTCFormatUnknown)
		format = _tcFormat;

	switch (format)
	{
		case kTCFormat24fps:	fps = 24;   break;
		case kTCFormat25fps:	fps = 25;   break;
		case kTCFormat30fps:
		case kTCFormat30fpsDF:	fps = 30;   break;
		case kTCFormat48fps:	fps = 48;   break;
		case kTCFormat50fps:	fps = 50;   break;
		case kTCFormat60fps:
		case kTCFormat60fpsDF:	fps = 60;   break;

		default:				fps = 30;	break;
	}

	return fps;
}


	// note: this is just for taking a stab at a default NTV2FrameRate based on an incoming timecode format
	// since there are MANY operational conditions where non-default frame rates vs timecode formats come up
	// (e.g. 720p60 uses 30 fps timecode, or 30 fps timecode could be 29.97 or 30 fps, etc...) the user should
	// NOT rely on this default to "always work" for their specific case.
NTV2FrameRate CRP188::DefaultFrameRateForTimecodeFormat (TimecodeFormat format) const
{
	if (format == kTCFormatUnknown)
		format = _tcFormat;

	NTV2FrameRate result = NTV2_FRAMERATE_UNKNOWN;

	switch (format)
	{
		case kTCFormat24fps:	result = NTV2_FRAMERATE_2400;		break;
		case kTCFormat25fps:	result = NTV2_FRAMERATE_2500;		break;
		case kTCFormat30fps:	result = NTV2_FRAMERATE_3000;		break;
		case kTCFormat30fpsDF:	result = NTV2_FRAMERATE_3000;		break;
		case kTCFormat48fps:	result = NTV2_FRAMERATE_5000;		break;
		case kTCFormat50fps:	result = NTV2_FRAMERATE_5000;		break;
		case kTCFormat60fps:	result = NTV2_FRAMERATE_6000;		break;
		case kTCFormat60fpsDF:	result = NTV2_FRAMERATE_6000;		break;

		default:				result = NTV2_FRAMERATE_UNKNOWN;	break;
	}

	return result;
}


//--------------------------------------------------------------------------------------------------------------------
// Private
//--------------------------------------------------------------------------------------------------------------------
// Convert Timecode string to integer value
void CRP188::ConvertTcStrToVal (void)
{
    int iOff;
    for (int i = 0; i < 4; i++)
    {
        iOff = i * 3;               // 2 spaces for 'xx' + 1 space for ':'
        string s(_sHMSF, iOff, 2);  // s starts at iOff and goes 2 spaces

        istringstream ist(s);       // turn s into ist, a string stream

        ist >> _ulVal[3 - i];       // convert from string to number
    }
}

void  CRP188::ConvertTcStrToReg (void)
{
    memset ((void *) &_rp188, 0, sizeof(_rp188));
    char pcBuf[2];

    // Tens of hours
    int iOff = 0, iSingle;
    pcBuf[0] = _sHMSF[iOff];
    pcBuf[1] = 0;
    sscanf (pcBuf, "%d", &iSingle);
    _rp188.High |= (iSingle & 0x3) << 24;

    // Units of hours
    iOff = 1;
    pcBuf[0] = _sHMSF[iOff];
    sscanf (pcBuf, "%d", &iSingle);
    _rp188.High |= (iSingle & 0xF) << 16;

    // Tens of minutes
    iOff = 3;
    pcBuf[0] = _sHMSF[iOff];
    sscanf (pcBuf, "%d", &iSingle);
    _rp188.High |= (iSingle & 0x7) << 8;

    // Units of minutes
    iOff = 4;
    pcBuf[0] = _sHMSF[iOff];
    sscanf (pcBuf, "%d", &iSingle);
    _rp188.High |= (iSingle & 0xF);

    // Tens of seconds
    iOff = 6;
    pcBuf[0] = _sHMSF[iOff];
    sscanf (pcBuf, "%d", &iSingle);
    _rp188.Low |= (iSingle & 0x7) << 24;

    // Units of seconds
    iOff = 7;
    pcBuf[0] = _sHMSF[iOff];
    sscanf (pcBuf, "%d", &iSingle);
    _rp188.Low |= (iSingle & 0xF) << 16;

	if ( !FormatIs60_50fps() )
	{
		// Tens of frames
		iOff = 9;
		pcBuf[0] = _sHMSF[iOff];
		sscanf (pcBuf, "%d", &iSingle);
		_rp188.Low |= (iSingle & 0x3) << 8;

		// Units of frames
		iOff = 10;
		pcBuf[0] = _sHMSF[iOff];
		sscanf (pcBuf, "%d", &iSingle);
		_rp188.Low |= (iSingle & 0xF);
	}
	else
	{
		// For frame rates >= 40 fps, we need an extra bit for the frame "10s" digit. By convention, we use the FieldID
		// bit as the LS bit of the 3-bit field (in essence, dividing by two and using the FID bit as a "half-frame")
		// Tens of frames
		int iTens;
		iOff = 9;
		pcBuf[0] = _sHMSF[iOff];
		sscanf (pcBuf, "%d", &iTens);

		// Units of frames
		int iUnits;
		iOff = 10;
		pcBuf[0] = _sHMSF[iOff];
		sscanf (pcBuf, "%d", &iUnits);

		ULWord frameCount = (10 * iTens) + iUnits;
		if (frameCount >= FramesPerSecond())		// limit-check
			frameCount = FramesPerSecond() - 1;

			// LS bit
		SetFieldID(frameCount % 2);
		frameCount /= 2;

		_rp188.Low |=  ((frameCount / 10) & 0x3) << 8;	// Tens of frames (ms 2 bits)
		_rp188.Low |=  ((frameCount % 10) & 0xF)     ;	// Units of frames
	}
}

void  CRP188::RP188ToUserBits (void)
{
    char userBitsString[12];

	_ulUserBits[7] = (_rp188.High >> 28) & 0xF;				// Binary Group 8
    userBitsString[0] = hexChar[_ulUserBits[7]];

	_ulUserBits[6] = (_rp188.High >> 20) & 0xF;				// Binary Group 7
    userBitsString[1] = hexChar[_ulUserBits[6]];
    userBitsString[2] = ' ';

	_ulUserBits[5] = (_rp188.High >> 12) & 0xF;				// Binary Group 6
    userBitsString[3] = hexChar[_ulUserBits[5]];

	_ulUserBits[4] = (_rp188.High >>  4) & 0xF;				// Binary Group 5
    userBitsString[4] = hexChar[_ulUserBits[4]];
    userBitsString[5] = ' ';

	_ulUserBits[3] = (_rp188.Low  >> 28) & 0xF;				// Binary Group 4
    userBitsString[6] = hexChar[_ulUserBits[3]];

	_ulUserBits[2] = (_rp188.Low  >> 20) & 0xF;				// Binary Group 3
    userBitsString[7] = hexChar[_ulUserBits[2]];
    userBitsString[8] = ' ';

	_ulUserBits[1] = (_rp188.Low  >> 12) & 0xF;				// Binary Group 2
    userBitsString[9] = hexChar[_ulUserBits[1]];

	_ulUserBits[0] = (_rp188.Low  >>  4) & 0xF;				// Binary Group 1
    userBitsString[10] = hexChar[_ulUserBits[0]];
    userBitsString[11] = '\0';

    _sUserBits = userBitsString;
}



bool CRP188::InitBurnIn (NTV2FrameBufferFormat frameBufferFormat, NTV2FrameDimensions frameDimensions, LWord percentY)
{
	bool bResult = true;

		// see if we've already rendered this format/size
	if (_bRendered && _pCharRenderMap != NULL && frameBufferFormat == _charRenderFBF && frameDimensions.Height() == _charRenderHeight && frameDimensions.Width() == _charRenderWidth)
	{
		return bResult;			// already rendered...
	}

	else
	{
		// these are the pixel formats we know how to do...
		bool	bFormatOK		(true);
		int		bytesPerPixel	(0);
		switch (frameBufferFormat)
		{
			case NTV2_FBF_8BIT_YCBCR:
						bytesPerPixel = 2;
						break;

			case NTV2_FBF_ABGR:
						bytesPerPixel = 4;
						break;

			case NTV2_FBF_ARGB:
						bytesPerPixel = 4;
						break;

			case NTV2_FBF_10BIT_YCBCR:
						bytesPerPixel = 3;		// not really - we'll have to override this wherever "bytesPerPixel" is used below...
						break;

			case NTV2_FBF_10BIT_DPX:
						bytesPerPixel = 4;
						break;

			default:	bFormatOK = false;
						break;
		}

		if (bFormatOK)
		{
				// scale the characters based on the frame size they'll be used in
			int dotScale = 1;					// SD scale
			if (frameDimensions.Height() > 900)
				dotScale = 3;					// HD 1080
			else if (frameDimensions.Height() > 650)
				dotScale = 2;					// HD 720

			int dotWidth  = 1 * dotScale;		// pixels per "dot"
			int dotHeight = 2 * dotScale;		// frame lines per "dot"

				// exceptions: if this is DVCProHD or HDV, we're working with horizontally-scaled pixels. Tweak the dotWidth to compensate.
			if (frameDimensions.Height() > 900 && frameDimensions.Width() <= 1440)
				dotWidth = 2;			// 1280x1080 or 1440x1080

//			else if (frameDimensions.Height() > 650 && frameDimensions.Width() < 1100)
//				dotWidth = 1;			// 960x720

			int charWidthBytes  = kDigitDotWidth  * dotWidth * bytesPerPixel;
			if (frameBufferFormat == NTV2_FBF_10BIT_YCBCR)
				charWidthBytes  = (kDigitDotWidth * dotWidth * 16) / 6;		// note: assumes kDigitDotWidth is evenly divisible by 6!

			int charHeightLines = kDigitDotHeight * dotHeight;

				// if we had a previous render map, free it now
			if (_pCharRenderMap != NULL)
			{
				free(_pCharRenderMap);
				_pCharRenderMap = NULL;
			}

				// malloc space for a new render map
			_pCharRenderMap = (char*)malloc(kMaxTCChars * charWidthBytes * charHeightLines);
			if (_pCharRenderMap != NULL)
			{
				char *pRenderMap = _pCharRenderMap;

					// for each character...
				for (int c = 0; c < kMaxTCChars; c++)
				{
						// for each scan line...
					for (int y = 0; y < kDigitDotHeight; y++)
					{
							// each rendered line is duplicated N times
						for (int ydup = 0; ydup < dotHeight; ydup++)
						{
								// each dot...
							for (int x = 0; x < kDigitDotWidth; x++)
							{
								char dot = CharMap[c][y][x];

								if (frameBufferFormat == NTV2_FBF_8BIT_YCBCR)
								{
									char val = 0;
									switch (dot)
									{
										case 0:		val = char(0x040 >> 2);	break;	//	16
										case 1:		val = char(0x164 >> 2);	break;	//	89
										case 2:		val = char(0x288 >> 2);	break;	//	162
										case 3:		val = char(0x3AC >> 2);	break;	//	235
									}

										// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = char(0x80);	// C
										*pRenderMap++ = val;		// Y
									}
								}

								else if (frameBufferFormat == NTV2_FBF_10BIT_YCBCR)
								{
									int val = 0;
									switch (dot)
									{
										case 0:		val = 0x040;	break;	//	64
										case 1:		val = 0x164;	break;	//	356
										case 2:		val = 0x288;	break;	//	648
										case 3:		val = 0x3AC;	break;	//	940
									}

										// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										writeV210Pixel (&pRenderMap, ((x * dotWidth) + xdup), 0x200, val);
									}
								}

								else if (frameBufferFormat == NTV2_FBF_ABGR || frameBufferFormat == NTV2_FBF_ARGB )
								{
									char val = 0;
									switch (dot)
									{
										case 0:		val = (char)0x00;		break;
										case 1:		val = (char)0x55;		break;
										case 2:		val = (char)0xaa;		break;
										case 3:		val = (char)0xff;		break;
									}

										// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = 0;			// A
										*pRenderMap++ = val;		// R
										*pRenderMap++ = val;		// G
										*pRenderMap++ = val;		// B
									}
								}

								else if ( frameBufferFormat == NTV2_FBF_ARGB )
								{
									char val = 0;
									switch (dot)
									{
										case 0:		val = (char)0x00;		break;
										case 1:		val = (char)0x55;		break;
										case 2:		val = (char)0xaa;		break;
										case 3:		val = (char)0xff;		break;
									}

										// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = 0;			// A
										*pRenderMap++ = val;		// R
										*pRenderMap++ = val;		// G
										*pRenderMap++ = val;		// B
									}
								}

								else if (frameBufferFormat == NTV2_FBF_10BIT_DPX)
								{
									int val = 0;
									switch (dot)
									{
										case 0:		val =  64;		break;
										case 1:		val = 356;		break;
										case 2:		val = 648;		break;
										case 3:		val = 940;		break;
									}

										// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ =                        ((val & 0x3fc) >> 2);
										*pRenderMap++ = ((val & 0x003) << 6) | ((val & 0x3f0) >> 4);
										*pRenderMap++ = ((val & 0x00f) << 4) | ((val & 0x3c0) >> 6);
										*pRenderMap++ = ((val & 0x03f) << 2);
									}
								}
							}
						}
					}
				}

				_bRendered = true;
				_charRenderFBF    = frameBufferFormat;
				_charRenderHeight = frameDimensions.Height();
				_charRenderWidth  = frameDimensions.Width();

					// character sizes
				_charWidthBytes   = charWidthBytes;
				_charHeightLines  = charHeightLines;

					// burn-in offset
				int byteWidth = (frameDimensions.Width() * bytesPerPixel);
				if (frameBufferFormat == NTV2_FBF_10BIT_YCBCR)
					byteWidth  = (frameDimensions.Width() * 16) / 6;		// in 10-bit YUV, 6 pixels = 16 bytes

				_charPositionX = (byteWidth - (kNumBurnInChars * charWidthBytes)) / 2;		// assume centered
				if(percentY == 0)
				{
					_charPositionY = (frameDimensions.Height() * 8) / 10;								// assume 80% of the way down the screen
				}
				else
				{
					_charPositionY = (frameDimensions.Height() * percentY) / 100;
				}

					// 10-bit YUV has to start on a 16-byte boundary in order to match the "cadence"
				if (frameBufferFormat == NTV2_FBF_10BIT_YCBCR)
					_charPositionX &= ~0x0f;
			}
		}

		else	// we don't know how to do this pixel format...
			bResult = false;
	}

	return bResult;
}


	// write a single chroma/luma pair of components
	// note that it's up to the caller to make sure that the chroma is the correct Cb or Cr,
	// and is properly timed with adjacent pixels!
void CRP188::writeV210Pixel (char **pBytePtr, int x, int c, int y)
{
	char *p = *pBytePtr;

		// the components in each v210 6-pixel block are laid out like this (note that the UInt32s are swixelled!):
		//
		// Addr: |  3    2    1    0  |  7    6    5    4  | 11   10    9    8  | 15   14   13   12 |
		//       { 00 Cr0   Y0   Cb0    00 Y2   Cb2    Y1    00 Cb4   Y3   Cr2    00 Y5   Cr4   Y4  }
		//
	int cadence = x % 3;

	switch (cadence)
	{
		case 0:		// Cb0/Y0 or Cr2/Y3: we assume that p points to byte 0/8. When we are finished, it will still point to byte 0/8
				p[0] =      c & 0x0FF;									//  c<7:0>
				p[1] = ((   y & 0x03F) << 2) + ((   c & 0x300) >> 8);	//  y<5:0> +  c<9:8>
				p[2] =  (p[2] & 0x0F0)       + ((   y & 0x3C0) >> 6);	// (merge) +  y<9:6>
				break;

		case 1:		// Cr0/Y1 or Cb4/Y4: we assume that p points to byte 0/8. When we are finished, it will point to byte 4/12
				p[2] = ((   c & 0x00F) << 4) +  (p[2] & 0x00F);			//  c<3:0> + (merge)
				p[3] =                         ((   c & 0x3F0) >> 4);	//   '00'  +  c<9:4>
				p[4] =      y & 0x0FF;									//  y<7:0>
				p[5] =  (p[5] & 0x0FC)       + ((   y & 0x300) >> 8);	// (merge) +  y<9:8>
				*pBytePtr += 4;
				break;

		case 2:		// Cb2/Y2 or Cr4/Y5: we assume that p points to byte 4/12. When we are finished, it will point to byte 8/16
				p[1] = ((   c & 0x03F) << 2) +  (p[1] & 0x003);			//  c<5:0> + (merge)
				p[2] = ((   y & 0x00F) << 4) + ((   c & 0x3C0) >> 6);	//  y<3:0> +  c<9:6>
				p[3] =                         ((   y & 0x3F0) >> 4);	//   '00'  +  y<9:4>
				*pBytePtr += 4;
				break;
	}
}


bool CRP188::BurnTC (char *pBaseVideoAddress, int rowBytes, TimecodeBurnMode burnMode, int64_t frameCount, bool bDisplay60_50fpsAs30_25)
{
	int val, char1, char2, trailingChar = kMaxTCChars;
	int charSizeBytes = _charWidthBytes * _charHeightLines;

	if (_bRendered)
	{
		char *pFrameBuff = pBaseVideoAddress + (_charPositionY * rowBytes) + _charPositionX;

		if (burnMode == kTCBurnTimecode || burnMode == kTCBurnUserBits)
		{
			for (int dig = 0; dig < 4; dig++)
			{
				if (burnMode == kTCBurnUserBits)
				{
					char1 = BinaryGroup(8 - (2*dig));	// user bits ("Binary Group") data
					char2 = BinaryGroup(7 - (2*dig));
				}
				else	// timecode
				{
					val = _ulVal[3-dig];	// timecode data

						// big ugly gross exception:
						// if the timecode format is >= 40 fps BUT we want to display the 2nd frame in the form: "XX:XX:XX:00*", where
						// the frame count runs from 0 - 29 (or 24) and '*' means "2nd frame", then we have to schmooze with the data...
					if (   dig == 3							// last digit (frames)
						&& FormatIs60_50fps(_tcFormat)		// timecode frame rate > 39 fps
						&& bDisplay60_50fpsAs30_25 )		// we wanna convert it
					{
						trailingChar = (val % 2 == 0 ? kDigSpace : kDigAsterisk);		// convert ls bit to trailing '*'
						val /= 2;														// convert 0 - 59 frames to 0 - 29 frames
					}

					char1 = val / 10;		// tens digit
					char2 = val % 10;		// units digit
				}

					// sanity check
				char1 = (char1 < 0 ? 0 : (char1 > kMaxTCChars ? kDigSpace : char1) );
				char2 = (char2 < 0 ? 0 : (char2 > kMaxTCChars ? kDigSpace : char2) );

					// "tens" digit
				CopyDigit ( (_pCharRenderMap + (char1 * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
				pFrameBuff += _charWidthBytes;

					// "ones" digit
				CopyDigit ( (_pCharRenderMap + (char2 * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
				pFrameBuff += _charWidthBytes;

					// add a colon after each pair (except last)
				if (dig < 3)
				{
					if (dig == 2 && FormatIsDropFrame() )
						CopyDigit ( (_pCharRenderMap + (kDigSemicolon * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
					else
						CopyDigit ( (_pCharRenderMap + (kDigColon * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
					pFrameBuff += _charWidthBytes;
				}
			}

				// if there is a "trailing character" (sometimes used for Field ID), do it now
			if (trailingChar >= 0 && trailingChar < kMaxTCChars)
			{
				CopyDigit ( (_pCharRenderMap + (trailingChar * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
				pFrameBuff += _charWidthBytes;
			}
		}

		else if (burnMode == kTCBurnBlank)
		{
			for (int dig = 0; dig < 4; dig++)
			{
					// 2 "dashes"
				CopyDigit ( (_pCharRenderMap + (kDigDash * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
				pFrameBuff += _charWidthBytes;

				CopyDigit ( (_pCharRenderMap + (kDigDash * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
				pFrameBuff += _charWidthBytes;

				if (dig < 3)
				{
					if (dig == 2 && FormatIsDropFrame() )
						CopyDigit ( (_pCharRenderMap + (kDigColon * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
					else
						CopyDigit ( (_pCharRenderMap + (kDigColon * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
					pFrameBuff += _charWidthBytes;
				}
			}
		}

		else	// display frame count
		{
			// we have two possible formats: if we the user DOESN'T pass in a frameCount, we calculate the frame count
			// from the current timecode. Since it can be (at most) 7 digits in size, we display 7 digits with 2 spaces
			// on either side. If the user DOES pass in a frame count it could be larger in size (we're going to allow
			// up to 9 digits plus a sign), so we're going to display 9 digits with one space on either side.

			int64_t count = frameCount;				// use passed-in frame count (allows for values outside the range of normal 24-hr timecode)
			int64_t scale = 1000000000;				// 9 digit max
			int maxDigits = 9;
			int numSpaces = 1;						// 1 + 9 + 1 = 11
			int i;
			if (count == kDefaultFrameCount)
			{
				ULWord	tmpCount (0);
				GetFrameCount(tmpCount);		// default = get current frame count
				count = tmpCount;
				scale = 10000000;					// 7 digit max
				maxDigits = 7;
				numSpaces = 2;						// 2 + 7 + 2 = 11
			}

			// 1 or 2 pre-spaces
			for (i = 0; i < numSpaces; i++)
			{
				// check for sign
				if (i == numSpaces-1)
				{
					if (count >= 0)
						CopyDigit ( (_pCharRenderMap + (kDigSpace * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
					else
					{
						CopyDigit ( (_pCharRenderMap + (kDigDash * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
						count = -count;
					}
					pFrameBuff += _charWidthBytes;
				}
				else
				{
					CopyDigit ( (_pCharRenderMap + (kDigSpace * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
					pFrameBuff += _charWidthBytes;
				}
			}

			if (count >= scale)
				count = count % scale;		// just wrap

			for (int dig = 0; dig < maxDigits; dig++)
			{
				scale = scale / 10;

				char1 = int(count / scale);
				CopyDigit ( (_pCharRenderMap + (char1 * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
				pFrameBuff += _charWidthBytes;

				count -= (scale * char1);		// get remainder
			}

			// 1 or 2 post-spaces
			for ( i = 0; i < numSpaces; i++)
			{
				CopyDigit ( (_pCharRenderMap + (kDigSpace * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
				pFrameBuff += _charWidthBytes;
			}
		}
	}

	return _bInitialized & _bRendered;
}

void CRP188::CopyDigit (char *pDigit, int digitWidth, int digitHeight, char *pFrameBuff, int fbRowBytes)
{
	for (int y = 0; y < digitHeight; y++)
	{
		char *pSrc = (pDigit + (y * digitWidth));
		char *pDst = (pFrameBuff + (y * fbRowBytes));

		memcpy(pDst, pSrc, digitWidth);
	}
}


string CRP188::GetTimeCodeString(bool bDisplay60_50fpsAs30_25)
{
	string result = "";

	int val, char1, char2, trailingChar = kMaxTCChars;

	for (int dig = 0; dig < 4; dig++)
	{
		// timecode
		{
			val = _ulVal[3-dig];	// timecode data

				// big ugly gross exception:
				// if the timecode format is >= 40 fps BUT we want to display the 2nd frame in the form: "XX:XX:XX:00*", where
				// the frame count runs from 0 - 29 (or 24) and '*' means "2nd frame", then we have to schmooze with the data...
			if (   dig == 3							// last digit (frames)
				&& FormatIs60_50fps(_tcFormat)		// timecode frame rate > 39 fps
				&& bDisplay60_50fpsAs30_25 )		// we wanna convert it
			{
				trailingChar = (val % 2 == 0 ? kDigSpace : kDigAsterisk);		// convert ls bit to trailing '*'
				val /= 2;														// convert 0 - 59 frames to 0 - 29 frames
			}

			char1 = val / 10;		// tens digit
			char2 = val % 10;		// units digit
		}

			// sanity check
		char1 = (char1 < 0 ? 0 : (char1 > kMaxTCChars ? kDigSpace : char1) );
		char2 = (char2 < 0 ? 0 : (char2 > kMaxTCChars ? kDigSpace : char2) );

			// "tens" digit
		result += '0' + char1;
		//CopyDigit ( (_pCharRenderMap + (char1 * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
		//pFrameBuff += _charWidthBytes;

			// "ones" digit
		result += '0' + char2;
		//CopyDigit ( (_pCharRenderMap + (char2 * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
		//pFrameBuff += _charWidthBytes;

			// add a colon after each pair (except last)
		if (dig < 3)
		{
			if (dig == 2 && FormatIsDropFrame() )
			{
				result += ";";
				//CopyDigit ( (_pCharRenderMap + (kDigSemicolon * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
			}
			else
			{
				result += ":";
				//CopyDigit ( (_pCharRenderMap + (kDigColon * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
			}
		}
	}
	
	// if there is a "trailing character" (sometimes used for Field ID), do it now
	if (trailingChar >= 0 && trailingChar < kMaxTCChars)
	{
		result += '0' + trailingChar;

		//CopyDigit ( (_pCharRenderMap + (trailingChar * charSizeBytes)), _charWidthBytes, _charHeightLines, pFrameBuff, rowBytes);
		//pFrameBuff += _charWidthBytes;
	}


	return result;
}

ostream & operator << (std::ostream & outputStream, const CRP188 & inObj)
{
	string	result;
	inObj.GetRP188Str (result);
	outputStream << result;
	return outputStream;
}


#ifdef MSWindows
#pragma warning(default: 4800)
#endif
