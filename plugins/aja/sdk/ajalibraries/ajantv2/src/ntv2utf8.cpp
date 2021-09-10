/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2utf8.cpp
	@brief		Implements the bare-bones UTF8 support functions (for the nub).
	@copyright	(C) 2008-2021 AJA Video Systems, Inc.
**/

#include <string.h>
#include <stdio.h>
#include "ntv2utf8.h"

#if defined (NTV2_NUB_CLIENT_SUPPORT)

static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/* returns length of next utf-8 sequence */
static int u8_seqlen(const char *s)
{
    return trailingBytesForUTF8[(unsigned int)(unsigned char)s[0]] + 1;
}


/* copy src to dest and truncate only on utf-8 boundaries */
void strncpyasutf8(char *dest, const char *src, int dest_buf_size)
{

	int bufleft = dest_buf_size -1;
	if (!dest_buf_size)
		return;
	
	memset(dest, 0, dest_buf_size);

	while (bufleft && *src)
	{
		// printf("\nbufleft=%d\n", bufleft);
		int u8_len = u8_seqlen(src);
		// printf("u8_len =%d\n", u8_len);
		 
		if (bufleft < u8_len) 
			break;

		for(int j = 0; j < u8_len; j++)
		{
			// printf("%02x ", (unsigned int)(unsigned char)*src);
			*dest++ = *src++;
		}
		bufleft -= u8_len;
	}
	// printf("\n\nexited loop with bufleft=%d\n", bufleft);

	// *dest = '\0'; /* Taken care of by memset/0 at entry */
	// printf ("%02x\n", *dest);
}

/* Convert UTF8 char to CodePage 437 char */
bool map_utf8_to_codepage437(const char *src, int u8_len, unsigned char *cp437equiv)
{
	unsigned char first = *src++;
	unsigned char second = *src++;

	*cp437equiv = 0;

	if (u8_len ==2)
	{
		if (first == 0xC6 && second == 0x92)
		{
			*cp437equiv = 0x9F;	// LATIN SMALL LETTER F WITH HOOK
		}
		else
		switch(first)
		{
			case 0xc2:
				switch(second)
				{
					case 0xa2: *cp437equiv = 0x9B;	break; // CENT SIGN
					case 0xA3: *cp437equiv = 0x9C;	break; // POUND SIGN
					case 0xA5: *cp437equiv = 0x9D;	break; // YEN SIGN

					case 0xAA: *cp437equiv = 0xA6;	break; // FEMININE ORDINAL INDICATOR
					case 0xBA: *cp437equiv = 0xA7;	break; // MASCULINE ORDINAL INDICATOR
					case 0xBF: *cp437equiv = 0xA8;	break; // INVERTED QUESTION MARK

					case 0xAC: *cp437equiv = 0xAA;	break; // NOT SIGN
					case 0xBD: *cp437equiv = 0xAB;	break; // VULGAR FRACTION ONE HALF
					case 0xBC: *cp437equiv = 0xAC;	break; // VULGAR FRACTION ONE QUARTER
					case 0xA1: *cp437equiv = 0xAD;	break; // INVERTED EXCLAMATION MARK
					case 0xAB: *cp437equiv = 0xAE;	break; // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
					case 0xBB: *cp437equiv = 0xAF;	break; // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK

					case 0xB5: *cp437equiv = 0xE6;	break; // MICRO SIGN

					case 0xB1: *cp437equiv = 0xF1;	break; // PLUS-MINUS SIGN

					case 0xB0: *cp437equiv = 0xF8;	break; // DEGREE SIGN

					case 0xB7: *cp437equiv = 0xFA;	break; // MIDDLE DOT

					case 0xB2: *cp437equiv = 0xFD;	break; // SUPERSCRIPT TWO
				}
				break;

			case 0xc3:
				switch(second)
				{
					case 0x87: *cp437equiv = 0x80;	break; // LATIN CAPITAL LETTER C WITH CEDILLA
					case 0xBC: *cp437equiv = 0x81;	break; // LATIN SMALL LETTER U WITH DIAERESIS
					case 0xA9: *cp437equiv = 0x82;	break; // LATIN SMALL LETTER E WITH ACUTE
					case 0xA2: *cp437equiv = 0x83;	break; // LATIN SMALL LETTER A WITH CIRCUMFLEX
					case 0xA4: *cp437equiv = 0x84;	break; // LATIN SMALL LETTER A WITH DIAERESIS
					case 0xA0: *cp437equiv = 0x85;	break; // LATIN SMALL LETTER A WITH GRAVE
					case 0xA5: *cp437equiv = 0x86;	break; // LATIN SMALL LETTER A WITH RING ABOVE
					case 0xA7: *cp437equiv = 0x87;	break; // LATIN SMALL LETTER C WITH CEDILLA
					case 0xAA: *cp437equiv = 0x88;	break; // LATIN SMALL LETTER E WITH CIRCUMFLEX
					case 0xAB: *cp437equiv = 0x89;	break; // LATIN SMALL LETTER E WITH DIAERESIS
					case 0xA8: *cp437equiv = 0x8A;	break; // LATIN SMALL LETTER E WITH GRAVE
					case 0xAF: *cp437equiv = 0x8B;	break; // LATIN SMALL LETTER I WITH DIAERESIS
					case 0xAE: *cp437equiv = 0x8C;	break; // LATIN SMALL LETTER I WITH CIRCUMFLEX
					case 0xAC: *cp437equiv = 0x8D;	break; // LATIN SMALL LETTER I WITH GRAVE
					case 0x84: *cp437equiv = 0x8E;	break; // LATIN CAPITAL LETTER A WITH DIAERESIS
					case 0x85: *cp437equiv = 0x8F;	break; // LATIN CAPITAL LETTER A WITH RING ABOVE
					case 0x89: *cp437equiv = 0x90;	break; // LATIN CAPITAL LETTER E WITH ACUTE
					case 0xA6: *cp437equiv = 0x91;	break; // LATIN SMALL LETTER AE
					case 0x86: *cp437equiv = 0x92;	break; // LATIN CAPITAL LETTER AE
					case 0xB4: *cp437equiv = 0x93;	break; // LATIN SMALL LETTER O WITH CIRCUMFLEX
					case 0xB6: *cp437equiv = 0x94;	break; // LATIN SMALL LETTER O WITH DIAERESIS
					case 0xB2: *cp437equiv = 0x95;	break; // LATIN SMALL LETTER O WITH GRAVE
					case 0xBB: *cp437equiv = 0x96;	break; // LATIN SMALL LETTER U WITH CIRCUMFLEX
					case 0xB9: *cp437equiv = 0x97;	break; // LATIN SMALL LETTER U WITH GRAVE
					case 0xBF: *cp437equiv = 0x98;	break; // LATIN SMALL LETTER Y WITH DIAERESIS
					case 0x96: *cp437equiv = 0x99;	break; // LATIN CAPITAL LETTER O WITH DIAERESIS
					case 0x9C: *cp437equiv = 0x9A;	break; // LATIN CAPITAL LETTER U WITH DIAERESIS

					case 0xA1: *cp437equiv = 0xA0;	break; // LATIN SMALL LETTER A WITH ACUTE
					case 0xAD: *cp437equiv = 0xA1;	break; // LATIN SMALL LETTER I WITH ACUTE
					case 0xB3: *cp437equiv = 0xA2;	break; // LATIN SMALL LETTER O WITH ACUTE
					case 0xBA: *cp437equiv = 0xA3;	break; // LATIN SMALL LETTER U WITH ACUTE
					case 0xB1: *cp437equiv = 0xA4;	break; // LATIN SMALL LETTER N WITH TILDE
					case 0x91: *cp437equiv = 0xA5;	break; // LATIN CAPITAL LETTER N WITH TILDE

					case 0x9F: *cp437equiv = 0xE1;	break; // LATIN SMALL LETTER SHARP S

					case 0xB7: *cp437equiv = 0xF6;	break; // DIVISION SIGN
				}
				break;

			case 0xce:
				switch(second)
				{
					case 0xB1: *cp437equiv = 0xE0;	break; // GREEK SMALL LETTER ALPHA

					case 0x93: *cp437equiv = 0xE2;	break; // GREEK CAPITAL LETTER GAMMA

					case 0xA3: *cp437equiv = 0xE4;	break; // GREEK CAPITAL LETTER SIGMA 

					case 0xA6: *cp437equiv = 0xE8;	break; // GREEK CAPITAL LETTER PHI
					case 0x98: *cp437equiv = 0xE9;	break; // GREEK CAPITAL LETTER THETA
					case 0xA9: *cp437equiv = 0xEA;	break; // GREEK CAPITAL LETTER OMEGA
					case 0xB4: *cp437equiv = 0xEB;	break; // GREEK SMALL LETTER DELTA

					case 0xB5: *cp437equiv = 0xEE;	break; // GREEK SMALL LETTER EPSILON
				}
				break;

			case 0xCF:
				switch(second)
				{
					case 0x80: *cp437equiv = 0xE3;	break; // GREEK SMALL LETTER PI

					case 0x83: *cp437equiv = 0xE5;	break; // GREEK SMALL LETTER SIGMA

					case 0x84: *cp437equiv = 0xE7;	break; // GREEK SMALL LETTER TAU

					case 0x86: *cp437equiv = 0xED;	break; // GREEK SMALL LETTER PHI
				}
				break;


		}
	}
	else if (u8_len == 3 && first == 0xE2)
	{
		unsigned char third = *src;
		switch(second)
		{
			case 0x81: 
				if (third == 0xBF)
					*cp437equiv = 0xFC; // SUPERSCRIPT LATIN SMALL LETTER N
				break;

			case 0x82: 
				if (third == 0xa7)
					*cp437equiv = 0x9e; // PESETA SIGN
				break;

			case 0x8c: 
				switch(third)
				{
					case 0x90: *cp437equiv = 0xA9;	break; // REVERSED NOT SIGN
					case 0xA0: *cp437equiv = 0xF4;	break; // TOP HALF INTEGRAL
					case 0xA1: *cp437equiv = 0xF5;	break; // BOTTOM HALF INTEGRAL
				}
				break;

			case 0x88: 
				switch(third)
				{
					case 0x9E: *cp437equiv = 0xEC;	break; // INFINITY 

					case 0xA9: *cp437equiv = 0xEF;	break; // INTERSECTION

					case 0x99: *cp437equiv = 0xF9;	break; // BULLET OPERATOR

					case 0x9A: *cp437equiv = 0xFB;	break; // SQUARE ROOT
				}
				break;

			case 0x89: 
				switch(third)
				{
					case 0xA1: *cp437equiv = 0xF0;	break; // IDENTICAL TO

					case 0xA5: *cp437equiv = 0xF2;	break; // GREATER-THAN OR EQUAL TO
					case 0xA4: *cp437equiv = 0xF3;	break; // LESS-THAN OR EQUAL TO

					case 0x88: *cp437equiv = 0xF7;	break; // ALMOST EQUAL TO
				}
				break;



			case 0x94: 
				switch(third)
				{
					case 0x82: *cp437equiv = 0xB3;	break; // BOX DRAWINGS LIGHT VERTICAL
					case 0xA4: *cp437equiv = 0xB4;	break; // BOX DRAWINGS LIGHT VERTICAL AND LEFT

					case 0x90: *cp437equiv = 0xBF;	break; // BOX DRAWINGS LIGHT DOWN AND LEFT
					case 0x94: *cp437equiv = 0xC0;	break; // BOX DRAWINGS LIGHT UP AND RIGHT
					case 0xB4: *cp437equiv = 0xC1;	break; // BOX DRAWINGS LIGHT UP AND HORIZONTAL
					case 0xAC: *cp437equiv = 0xC2;	break; // BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
					case 0x9C: *cp437equiv = 0xC3;	break; // BOX DRAWINGS LIGHT VERTICAL AND RIGHT
					case 0x80: *cp437equiv = 0xC4;	break; // BOX DRAWINGS LIGHT HORIZONTAL
					case 0xBC: *cp437equiv = 0xC5;	break; // BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL

					case 0x98: *cp437equiv = 0xD9;	break; // BOX DRAWINGS LIGHT UP AND LEFT
					case 0x8C: *cp437equiv = 0xDA;	break; // BOX DRAWINGS LIGHT DOWN AND RIGHT
				}
				break;

			case 0x95: 
				switch(third)
				{
					case 0xA1: *cp437equiv = 0xB5;	break; // BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
					case 0xA2: *cp437equiv = 0xB6;	break; // BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
					case 0x96: *cp437equiv = 0xB7;	break; // BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
					case 0x95: *cp437equiv = 0xB8;	break; // BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
					case 0xA3: *cp437equiv = 0xB9;	break; // BOX DRAWINGS DOUBLE VERTICAL AND LEFT
					case 0x91: *cp437equiv = 0xBA;	break; // BOX DRAWINGS DOUBLE VERTICAL
					case 0x97: *cp437equiv = 0xBB;	break; // BOX DRAWINGS DOUBLE DOWN AND LEFT
					case 0x9D: *cp437equiv = 0xBC;	break; // BOX DRAWINGS DOUBLE UP AND LEFT
					case 0x9C: *cp437equiv = 0xBD;	break; // BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
					case 0x9B: *cp437equiv = 0xBE;	break; // BOX DRAWINGS UP SINGLE AND LEFT DOUBLE

					case 0x9E: *cp437equiv = 0xC6;	break; // BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
					case 0x9F: *cp437equiv = 0xC7;	break; // BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
					case 0x9A: *cp437equiv = 0xC8;	break; // BOX DRAWINGS DOUBLE UP AND RIGHT
					case 0x94: *cp437equiv = 0xC9;	break; // BOX DRAWINGS DOUBLE DOWN AND RIGHT
					case 0xA9: *cp437equiv = 0xCA;	break; // BOX DRAWINGS DOUBLE UP AND HORIZONTAL
					case 0xA6: *cp437equiv = 0xCB;	break; // BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
					case 0xA0: *cp437equiv = 0xCC;	break; // BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
					case 0x90: *cp437equiv = 0xCD;	break; // BOX DRAWINGS DOUBLE HORIZONTAL
					case 0xAC: *cp437equiv = 0xCE;	break; // BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
					case 0xA7: *cp437equiv = 0xCF;	break; // BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
					case 0xA8: *cp437equiv = 0xD0;	break; // BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
					case 0xA4: *cp437equiv = 0xD1;	break; // BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
					case 0xA5: *cp437equiv = 0xD2;	break; // BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
					case 0x99: *cp437equiv = 0xD3;	break; // BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
					case 0x98: *cp437equiv = 0xD4;	break; // BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
					case 0x92: *cp437equiv = 0xD5;	break; // BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
					case 0x93: *cp437equiv = 0xD6;	break; // BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
					case 0xAB: *cp437equiv = 0xD7;	break; // BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
					case 0xAA: *cp437equiv = 0xD8;	break; // BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
				}
				break;


			case 0x96: 
				switch(third)
				{
					case 0x91: *cp437equiv = 0xB0;	break; // LIGHT SHADE
					case 0x92: *cp437equiv = 0xB1;	break; // MEDIUM SHADE
					case 0x93: *cp437equiv = 0xB2;	break; // DARK SHADE

					case 0x88: *cp437equiv = 0xDB;	break; // FULL BLOCK
					case 0x84: *cp437equiv = 0xDC;	break; // LOWER HALF BLOCK
					case 0x8C: *cp437equiv = 0xDD;	break; // LEFT HALF BLOCK
					case 0x90: *cp437equiv = 0xDE;	break; // RIGHT HALF BLOCK
					case 0x80: *cp437equiv = 0xDF;	break; // UPPER HALF BLOCK

					case 0xA0: *cp437equiv = 0xFE;	break; // BLACK SQUARE
				}
				break;
		}
	}

	return (*cp437equiv != 0)? true : false;
}


/* Convert UTF8 string to CodePage 437 string. */
/* Used to display some common UTF8 characters on embedded displays with CP437 support. - STC */
void strncpyasutf8_map_cp437(char *dest, const char *src, int dest_buf_size)
{
	int bufleft = dest_buf_size -1;
	if (!dest_buf_size)
		return;
	
	while (bufleft)
	{
		int u8_len = u8_seqlen(src);
		 
		if (bufleft < u8_len) 
			break;
		
		unsigned char cp437equiv;
		if (map_utf8_to_codepage437(src, u8_len, &cp437equiv)) 
		{
			src += u8_len;
			*dest++ = cp437equiv;
			// printf("%02x ", (unsigned char)cp437equiv);
			bufleft--;
		}
		else 
		{
			for(int j = 0; j < u8_len; j++)
			{
				// printf("%02x ", (unsigned char)*src);
				*dest++ = *src++;
			}
			bufleft -= u8_len;
		}
	}
	*dest = '\0';
	// printf("%02x\n", *dest);
}

#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
