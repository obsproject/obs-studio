/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2videodefines.h
	@brief		Declares common video macros and structs used in the SDK.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.  All rights reserved.
**/
#ifndef VIDEODEFINES_H
#define VIDEODEFINES_H
#include "ajatypes.h"

#define PI_FLOAT (3.141592654)

#define CCIR601_8BIT_BLACK 16
#define CCIR601_8BIT_WHITE  235
#define CCIR601_8BIT_CHROMAOFFSET 128

#define CCIR601_10BIT_BLACK 64
#define CCIR601_10BIT_WHITE  940
#define CCIR601_10BIT_CHROMAOFFSET 512

#define MIN_RGB_8BIT	0
#define MAX_RGB_8BIT	255
#define MIN_RGB_10BIT	0
#define MAX_RGB_10BIT	1023
#define MIN_RGB_16BIT	0			//	KAM
#define MAX_RGB_16BIT	65535		//	KAM

//	NOTE:	Changed the "(__x__) < MIN_RGB_nBIT" comparisons to "(__x__) <= MIN_RGB_nBIT"
//			in the following three macros to eliminate gcc "comparison always true" warnings
//			when __x__ is an unsigned value.
#define ClipRGB_8BIT(__x__)			((__x__) > MAX_RGB_8BIT   ?  (MAX_RGB_8BIT)									\
									                          :  ((__x__) <= MIN_RGB_8BIT   ?  (MIN_RGB_8BIT)	\
									                                                        :  (__x__)))
#define ClipRGB_10BIT(__x__)		((__x__) > MAX_RGB_10BIT  ?  (MAX_RGB_10BIT)								\
									                          :  ((__x__) <= MIN_RGB_10BIT  ?  (MIN_RGB_10BIT)	\
									                                                        :  (__x__)))
#define ClipRGB_16BIT(__x__)		((__x__) > MAX_RGB_16BIT  ?  (MAX_RGB_16BIT)								\
									                          :  ((__x__) <= MIN_RGB_16BIT  ?  (MIN_RGB_16BIT)	\
									                                                        :  (__x__)))

#define NUMACTIVELINES_525 486
#define NUMACTIVELINES_625 576

#define MAXSQUAREPIXELS_525 648
#define MAXSQUAREPIXELS_625 768

#define NUMCOMPONENTPIXELS 720
#define YCBCRLINEPITCH_SD             480  // in 32 bit words for packed 10 bit
#define RGBALPHALINEPITCH_625          720  // in 32 bit words no matter what framegeometry
#define RGBALPHALINEPITCH_525          720  // in 32 bit words no matter what framegeometry
#define RGB24LINEPITCH_525		  (540)
#define RGB24LINEPITCH_625		  (540)


// HD Defines
#define HD_NUMACTIVELINES_720         720   //duh?
#define HD_NUMACTIVELINES_1080        1080  // double duh?
#define HD_NUMACTIVELINES_2K          1556  
#define HD_NUMLINES_2K				  1588  // In actual FrameBuffer
#define HD_FIRSTACTIVELINE_2K		  (HD_NUMLINES_2K-HD_NUMACTIVELINES_2K)  
#define HD_NUMLINES_4K				2160
#define FD_NUMLINES_8K				4320

#define HD_NUMACTIVELINES_720_QREZ    (HD_NUMACTIVELINES_720/2)  
#define HD_NUMACTIVELINES_1080_QREZ   (HD_NUMACTIVELINES_1080/2) 

#define HD_NUMCOMPONENTPIXELS_720     1280  // in a line
#define HD_NUMCOMPONENTPIXELS_1080    1920  // in a line
#define HD_NUMCOMPONENTPIXELS_1080_2K 2048  // in a line
#define HD_NUMCOMPONENTPIXELS_2K      2048  // in a line
#define HD_NUMCOMPONENTPIXELS_QUADHD  3840  // in a line
#define HD_NUMCOMPONENTPIXELS_4K	  4096  // in a line
#define FD_NUMCOMPONENTPIXELS_UHD2		7680  // in a line
#define FD_NUMCOMPONENTPIXELS_8K		8192  // in a line

#define HD_NUMCOMPONENTPIXELS_720_DVCPRO     960  // in a line
#define HD_NUMCOMPONENTPIXELS_1080_DVCPRO    1280  // in a line

#define HD_NUMCOMPONENTPIXELS_720_QREZ     (HD_NUMCOMPONENTPIXELS_720/2)  // in a line
#define HD_NUMCOMPONENTPIXELS_1080_QREZ    (HD_NUMCOMPONENTPIXELS_1080/2)  // in a line

#define HD_NUMCOMPONENTPIXELS_720_HDV     960  // in a line
#define HD_NUMCOMPONENTPIXELS_1080_HDV   1440  // in a line

// Linepitch always in 32 bit words.
#define HD_YCBCRLINEPITCH_720         864   // dx*8/3/4 32 bit words for packed 10 bit
#define HD_YCBCRLINEPITCH_1080        1280  // ""
#define HD_YCBCRLINEPITCH_2K          1376  // ""
#define HD_YCBCRLINEPITCH_3840        2560  // ""
#define HD_YCBCRLINEPITCH_4K          2752  // ""
#define FD_YCBCRLINEPITCH_UHD2        5120  // ""
#define FD_YCBCRLINEPITCH_8K          5472  // ""

#define HD_YCBCRLINEPITCH_720_DVCPRO  (960*2/4)   // dvcpro always 8 bit
#define HD_YCBCRLINEPITCH_1080_DVCPRO (1280*2/4)  // dvcpro always 8 bit

#define HD_YCBCRLINEPITCH_720_QREZ  (1280/4)   // in 32 bit words for packed 10 bit
#define HD_YCBCRLINEPITCH_1080_QREZ (1920/4)   // ""

#define HD_YCBCRLINEPITCH_720_HDV   (960*2/4)   // hdv always 8 bit
#define HD_YCBCRLINEPITCH_1080_HDV  (1440*2/4)  // hdv always 8 bit

#define RGB24LINEPITCH_720			(960)
#define RGB24LINEPITCH_1080			(1440)
#define RGB24LINEPITCH_2048			(1536)
#define RGB24LINEPITCH_3840			(2880)
#define RGB24LINEPITCH_4096			(3072)
#define RGB24LINEPITCH_7680			(5760)
#define RGB24LINEPITCH_8192			(6144)

#define RGB48LINEPITCH_525			(RGB24LINEPITCH_525*2)
#define RGB48LINEPITCH_625			(RGB24LINEPITCH_625*2)
#define RGB48LINEPITCH_720			(RGB24LINEPITCH_720*2)
#define RGB48LINEPITCH_1080			(RGB24LINEPITCH_1080*2)
#define RGB48LINEPITCH_2048			(RGB24LINEPITCH_2048*2)
#define RGB48LINEPITCH_3840			(RGB24LINEPITCH_3840*2)
#define RGB48LINEPITCH_4096			(RGB24LINEPITCH_4096*2)
#define RGB48LINEPITCH_7680			(RGB24LINEPITCH_7680*2)
#define RGB48LINEPITCH_8192			(RGB24LINEPITCH_8192*2)

#define RGB12PLINEPITCH_525			(MAXSQUAREPIXELS_525*36/8)/4
#define RGB12PLINEPITCH_625			(MAXSQUAREPIXELS_625*36/8)/4
#define RGB12PLINEPITCH_720			(HD_NUMCOMPONENTPIXELS_720*36/8)/4
#define RGB12PLINEPITCH_1080		(HD_NUMCOMPONENTPIXELS_1080*36/8)/4
#define RGB12PLINEPITCH_2048		(HD_NUMCOMPONENTPIXELS_1080_2K*36/8)/4
#define RGB12PLINEPITCH_3840		(HD_NUMCOMPONENTPIXELS_QUADHD*36/8)/4
#define RGB12PLINEPITCH_4096		(HD_NUMCOMPONENTPIXELS_4K*36/8)/4
#define RGB12PLINEPITCH_7680		(FD_NUMCOMPONENTPIXELS_UHD2*36/8)/4
#define RGB12PLINEPITCH_8192		(FD_NUMCOMPONENTPIXELS_8K*36/8)/4

#define PRORES_MAXBUFFERSIZE		(1105920)	// bytes - PAL 10bit

// Roll Defines
#define HD_ROLLNUMLINES               4096
#define HD_ROLLNUMPIXELS              4096

typedef struct {
	unsigned char Blue;
	unsigned char Green;
	unsigned char Red;
	unsigned char Alpha;
} RGBAlphaPixel;


typedef struct {
	UWord Blue;
	UWord Green;
	UWord Red;
	UWord Alpha;
} RGBAlpha10BitPixel;

typedef struct {
	UWord Blue;
	UWord Green;
	UWord Red;
	UWord Alpha;
} RGBAlpha16BitPixel;

typedef struct {
	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
	unsigned char Alpha;
} AERGBAlphaPixel;

typedef struct {
	Fixed_ Blue;
	Fixed_ Green;
	Fixed_ Red;
	Fixed_ Alpha;
} RGBAlphaFixedPixel;

typedef struct {
	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
} RGBPixel;

typedef struct {
	unsigned char Blue;
	unsigned char Green;
	unsigned char Red;
} BGRPixel;

typedef struct
{
	unsigned char Alpha;
	unsigned char cr;
	unsigned char y;
	unsigned char cb;
} YCbCrAlphaPixel;

typedef struct
{
	unsigned char cb;
	unsigned char y;
	unsigned char cr;
} YCbCrPixel;

typedef struct
{
	UWord cb;
	UWord y;
	UWord cr;
} YCbCr10BitPixel;

typedef struct
{
	UWord Alpha;
	UWord cb;
	UWord y;
	UWord cr;
} YCbCr10BitAlphaPixel;

typedef struct {
	char SigName[40];
	short Y[NUMCOMPONENTPIXELS];
	short ColorDiff[NUMCOMPONENTPIXELS];
} TestLineDataStr;

#endif	//	VIDEODEFINES_H
