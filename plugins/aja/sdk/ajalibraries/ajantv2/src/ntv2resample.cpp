/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2resample.cpp
	@brief		Implementations for the pixel resampling functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2resample.h"

inline RGBAlphaPixel CubicInterPolate( RGBAlphaPixel *Input, LWord Index);
inline Word CubicInterPolateWord( Word *Input, Fixed_ Index);
inline Word CubicInterPolateAudioWord( Word *Input, Fixed_ Index);

// Cubic Coefficient Table for resampling
static unsigned int CubicCoef[129] = {
 0x00000000  , /* 0 */
 0xFFFFFFE1  , /* 1 */
 0xFFFFFF88  , /* 2 */
 0xFFFFFEFB  , /* 3 */
 0xFFFFFE40  , /* 4 */
 0xFFFFFD5D  , /* 5 */
 0xFFFFFC58  , /* 6 */
 0xFFFFFB37  , /* 7 */
 0xFFFFFA00  , /* 8 */
 0xFFFFF8B9  , /* 9 */
 0xFFFFF768  , /* 10 */
 0xFFFFF613  , /* 11 */
 0xFFFFF4C0  , /* 12 */
 0xFFFFF375  , /* 13 */
 0xFFFFF238  , /* 14 */
 0xFFFFF10F  , /* 15 */
 0xFFFFF000  , /* 16 */
 0xFFFFEF11  , /* 17 */
 0xFFFFEE48  , /* 18 */
 0xFFFFEDAB  , /* 19 */
 0xFFFFED40  , /* 20 */
 0xFFFFED0D  , /* 21 */
 0xFFFFED18  , /* 22 */
 0xFFFFED67  , /* 23 */
 0xFFFFEE00  , /* 24 */
 0xFFFFEEE9  , /* 25 */
 0xFFFFF028  , /* 26 */
 0xFFFFF1C3  , /* 27 */
 0xFFFFF3C0  , /* 28 */
 0xFFFFF625  , /* 29 */
 0xFFFFF8F8  , /* 30 */
 0xFFFFFC3F  , /* 31 */
 0x00000000  , /* 32 */
 0x0000047D  , /* 33 */
 0x000009E8  , /* 34 */
 0x0000102F  , /* 35 */
 0x00001740  , /* 36 */
 0x00001F09  , /* 37 */
 0x00002778  , /* 38 */
 0x0000307B  , /* 39 */
 0x00003A00  , /* 40 */
 0x000043F5  , /* 41 */
 0x00004E48  , /* 42 */
 0x000058E7  , /* 43 */
 0x000063C0  , /* 44 */
 0x00006EC1  , /* 45 */
 0x000079D8  , /* 46 */
 0x000084F3  , /* 47 */
 0x00009000  , /* 48 */
 0x00009AED  , /* 49 */
 0x0000A5A8  , /* 50 */
 0x0000B01F  , /* 51 */
 0x0000BA40  , /* 52 */
 0x0000C3F9  , /* 53 */
 0x0000CD38  , /* 54 */
 0x0000D5EB  , /* 55 */
 0x0000DE00  , /* 56 */
 0x0000E565  , /* 57 */
 0x0000EC08  , /* 58 */
 0x0000F1D7  , /* 59 */
 0x0000F6C0  , /* 60 */
 0x0000FAB1  , /* 61 */
 0x0000FD98  , /* 62 */
 0x0000FF63  , /* 63 */
 0x00010000  , /* 64 */
 0x0000FF63  , /* 65 */
 0x0000FD98  , /* 66 */
 0x0000FAB1  , /* 67 */
 0x0000F6C0  , /* 68 */
 0x0000F1D7  , /* 69 */
 0x0000EC08  , /* 70 */
 0x0000E565  , /* 71 */
 0x0000DE00  , /* 72 */
 0x0000D5EB  , /* 73 */
 0x0000CD38  , /* 74 */
 0x0000C3F9  , /* 75 */
 0x0000BA40  , /* 76 */
 0x0000B01F  , /* 77 */
 0x0000A5A8  , /* 78 */
 0x00009AED  , /* 79 */
 0x00009000  , /* 80 */
 0x000084F3  , /* 81 */
 0x000079D8  , /* 82 */
 0x00006EC1  , /* 83 */
 0x000063C0  , /* 84 */
 0x000058E7  , /* 85 */
 0x00004E48  , /* 86 */
 0x000043F5  , /* 87 */
 0x00003A00  , /* 88 */
 0x0000307B  , /* 89 */
 0x00002778  , /* 90 */
 0x00001F09  , /* 91 */
 0x00001740  , /* 92 */
 0x0000102F  , /* 93 */
 0x000009E8  , /* 94 */
 0x0000047D  , /* 95 */
 0x00000000  , /* 96 */
 0xFFFFFC3F  , /* 97 */
 0xFFFFF8F8  , /* 98 */
 0xFFFFF625  , /* 99 */
 0xFFFFF3C0  , /* 100 */
 0xFFFFF1C3  , /* 101 */
 0xFFFFF028  , /* 102 */
 0xFFFFEEE9  , /* 103 */
 0xFFFFEE00  , /* 104 */
 0xFFFFED67  , /* 105 */
 0xFFFFED18  , /* 106 */
 0xFFFFED0D  , /* 107 */
 0xFFFFED40  , /* 108 */
 0xFFFFEDAB  , /* 109 */
 0xFFFFEE48  , /* 110 */
 0xFFFFEF11  , /* 111 */
 0xFFFFF000  , /* 112 */
 0xFFFFF10F  , /* 113 */
 0xFFFFF238  , /* 114 */
 0xFFFFF375  , /* 115 */
 0xFFFFF4C0  , /* 116 */
 0xFFFFF613  , /* 117 */
 0xFFFFF768  , /* 118 */
 0xFFFFF8B9  , /* 119 */
 0xFFFFFA00  , /* 120 */
 0xFFFFFB37  , /* 121 */
 0xFFFFFC58  , /* 122 */
 0xFFFFFD5D  , /* 123 */
 0xFFFFFE40  , /* 124 */
 0xFFFFFEFB  , /* 125 */
 0xFFFFFF88  , /* 126 */
 0xFFFFFFE1  , /* 127 */
 0x00000000  , /* 128 */
};



// ReSampleLine
// RGBAlphaPixel Version
// NOTE: Input buffer needs to have extra sample at start and 2 at end.
void ReSampleLine(RGBAlphaPixel *Input, 
				  RGBAlphaPixel *Output,
				  UWord startPixel,
				  UWord endPixel,
				  LWord numInputPixels,
				  LWord numOutputPixels)
					  
{
  LWord count,inputIndex,reSampleStartPixel,reSampleEndPixel;
  Fixed_ accum = 0;
  Fixed_ increment,coefIndex;
  
  Input[-1] = Input[0];
  Input[numInputPixels] = Input[numInputPixels+1] = Input[numInputPixels-1];
  
  increment = (numInputPixels<<16)/numOutputPixels;
  reSampleStartPixel = (startPixel*numOutputPixels)/numInputPixels;
  reSampleEndPixel = (endPixel*numOutputPixels)/numInputPixels;
  for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
  {
	accum = (increment*count);
	inputIndex = FixedTrunc(accum);
	coefIndex = (accum>>11) & 0x1F;
	Output[count] = CubicInterPolate(&Input[inputIndex],coefIndex);
  }
  
}


// ReSampleLine
// Word Version
// NOTE: Input buffer needs to have extra sample at start and 2 at end.
void ReSampleLine(Word *Input, 
			      Word *Output,
			      UWord startPixel,
			      UWord endPixel,
			      LWord numInputPixels,
			      LWord numOutputPixels)
					  
{
  LWord count,inputIndex,reSampleStartPixel,reSampleEndPixel;
  Fixed_ accum = 0;
  Fixed_ increment,coefIndex;
  
  Input[-1] = Input[0];
  Input[numInputPixels] = Input[numInputPixels+1] = Input[numInputPixels-1];
  
  increment = (numInputPixels<<16)/numOutputPixels;
  reSampleStartPixel = (startPixel*numOutputPixels)/numInputPixels;
  reSampleEndPixel = (endPixel*numOutputPixels)/numInputPixels;
  for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
  {
	accum = (increment*count);
	inputIndex = FixedTrunc(accum);
	coefIndex = (accum>>11) & 0x1F;
	Output[count] = (Word)CubicInterPolateWord((Word *)&Input[inputIndex],coefIndex);
  }
  
}
#define MIN_YCBCR_10BIT 4
#define MAX_YCBCR_10BIT 1019
#define ClipYCbCr_10BIT(X) ((X) > MAX_YCBCR_10BIT ? (MAX_YCBCR_10BIT) : ((X) < MIN_YCBCR_10BIT ? (MIN_YCBCR_10BIT) : (X)))

// ReSampleLine
// Word Version
void ReSampleYCbCrSampleLine(Word *Input, 
			                 Word *Output,
			                 LWord numInputPixels,
			                 LWord numOutputPixels)
					  
{
  LWord count,inputIndex,reSampleStartPixel,reSampleEndPixel;
  Fixed_ accum = 0;
  Fixed_ increment,coefIndex;
  
  Word* yBuffer = new Word[numInputPixels+4];
  Word* cbBuffer = new Word[numInputPixels/2+4];
  Word* crBuffer = new Word[numInputPixels/2+4];

  // First Extract Y
  for ( count = 0; count < numInputPixels; count++ )
  {
	  if ( count & 0x1)
	  {
		crBuffer[count/2+1] = Input[count*2];
	  }
	  else
	  {
		cbBuffer[count/2+1] = Input[count*2];

	  }
	  yBuffer[count+1] = Input[count*2+1];
  }

  yBuffer[0] = yBuffer[1];
  yBuffer[numInputPixels+1] = yBuffer[numInputPixels+2] = yBuffer[numInputPixels];
  
  increment = (numInputPixels<<16)/numOutputPixels;
  reSampleStartPixel = 0;
  reSampleEndPixel = numOutputPixels;
  for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
  {
	accum = (increment*count);
	inputIndex = FixedTrunc(accum);
	coefIndex = (accum>>11) & 0x1F;
	Output[count*2+1] = (Word)CubicInterPolateWord((Word *)&yBuffer[inputIndex+1],coefIndex);
  }

  reSampleStartPixel = 0;
  reSampleEndPixel = numOutputPixels/2;
  cbBuffer[0] = cbBuffer[1];
  cbBuffer[numInputPixels/2+1] = cbBuffer[numInputPixels/2+2] = cbBuffer[numInputPixels/2];
  crBuffer[0] = crBuffer[1];
  crBuffer[numInputPixels/2+1] = crBuffer[numInputPixels/2+2] = crBuffer[numInputPixels/2];

  for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
  {
	accum = (increment*count);
	inputIndex = FixedTrunc(accum);
	coefIndex = (accum>>11) & 0x1F;
	Output[count*4] = (Word)CubicInterPolateWord((Word *)&cbBuffer[inputIndex+1],coefIndex);
	Output[count*4+2] = (Word)CubicInterPolateWord((Word *)&crBuffer[inputIndex+1],coefIndex);
  }

  delete [] yBuffer;
  delete [] cbBuffer;
  delete [] crBuffer;
}


void ReSampleAudio(Word *Input, 
				   Word *Output,
				   UWord startPixel,
				   UWord endPixel,
				   LWord numInputPixels,
				   LWord numOutputPixels,
				   Word channelInterleaveMulitplier)
{
	LWord count,inputIndex,reSampleStartPixel,reSampleEndPixel;
	Fixed_ accum = 0;
	Fixed_ increment,coefIndex;

	increment = (numInputPixels<<16)/numOutputPixels;
	reSampleStartPixel = (startPixel*numOutputPixels)/numInputPixels;
	reSampleEndPixel = (endPixel*numOutputPixels)/numInputPixels;
	for ( count = reSampleStartPixel; count < reSampleEndPixel; count++ )
	{
		accum = (increment*count);
		inputIndex = FixedTrunc(accum);
		coefIndex = (accum>>11) & 0x1F;
		Output[count*channelInterleaveMulitplier] = (Word)CubicInterPolateAudioWord((Word *)&Input[inputIndex],coefIndex);
	}


}
inline Word CubicInterPolateWord( Word *Input, Fixed_ Index)
{
    LWord InterPolatedValue;

    InterPolatedValue = FixedTrunc(Input[-1]*CubicCoef[32-Index] + 
                                   Input[0]*CubicCoef[64-Index] + 
		                           Input[1]*CubicCoef[96-Index] + 
		                           Input[2]*CubicCoef[128-Index]);
	 
	//    return (Word)(InterPolatedValue&0x3FF);
	    return (Word)ClipYCbCr_10BIT(InterPolatedValue);
}

inline Word CubicInterPolateAudioWord( Word *Input, Fixed_ Index)
{
	LWord InterPolatedValue;

	InterPolatedValue = FixedTrunc(Input[-1]*CubicCoef[32-Index] + 
		Input[0]*CubicCoef[64-Index] + 
		Input[1]*CubicCoef[96-Index] + 
		Input[2]*CubicCoef[128-Index]);

	if ( InterPolatedValue > 32767 )
		InterPolatedValue = 32767;
	if ( InterPolatedValue < -32767 )
		InterPolatedValue = -32767;

	return (Word)(InterPolatedValue&0xFFFF);
}

inline RGBAlphaPixel CubicInterPolate( RGBAlphaPixel *Input, LWord Index)
{

    RGBAlphaPixel InterPolatedValue;
    LWord temp;


    temp = FixedTrunc(Input[-1].Red*CubicCoef[32-Index] + 
                      Input[0].Red*CubicCoef[64-Index] + 
		              Input[1].Red*CubicCoef[96-Index] + 
		              Input[2].Red*CubicCoef[128-Index]);

	InterPolatedValue.Red = (UByte)((temp < 0 ) ? 0 : (temp > 255 ) ? 255 : temp);

    temp= FixedTrunc(Input[-1].Green*CubicCoef[32-Index] + 
                     Input[0].Green*CubicCoef[64-Index] + 
	                 Input[1].Green*CubicCoef[96-Index] + 
	 	             Input[2].Green*CubicCoef[128-Index]);
	InterPolatedValue.Green = (UByte)((temp < 0 ) ? 0 : (temp > 255 ) ? 255 : temp);


    temp = FixedTrunc(Input[-1].Blue*CubicCoef[32-Index] + 
                      Input[0].Blue*CubicCoef[64-Index] + 
		              Input[1].Blue*CubicCoef[96-Index] + 
		              Input[2].Blue*CubicCoef[128-Index]);

	InterPolatedValue.Blue = (UByte)((temp < 0 ) ? 0 : (temp > 255 ) ? 255 : temp);
	 
    temp = FixedTrunc(Input[-1].Alpha*CubicCoef[32-Index] + 
                      Input[0].Alpha*CubicCoef[64-Index] + 
		              Input[1].Alpha*CubicCoef[96-Index] + 
		              Input[2].Alpha*CubicCoef[128-Index]);

	InterPolatedValue.Alpha = (UByte)((temp < 0 ) ? 0 : (temp > 255 ) ? 255 : temp);

    return InterPolatedValue;

}



