/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2enhancedcsc.h
	@brief		Declares the CNTV2EnhancedCSC class.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2_ENHANCED_CSC_H
#define NTV2_ENHANCED_CSC_H

#include "ajaexport.h"
#include "ntv2card.h"
#include "ntv2cscmatrix.h"

typedef enum
{
	NTV2_Enhanced_CSC_Pixel_Format_RGB444,
	NTV2_Enhanced_CSC_Pixel_Format_YCbCr444,
	NTV2_Enhanced_CSC_Pixel_Format_YCbCr422,

	NTV2_Enhanced_CSC_Num_Pixel_Formats
} NTV2EnhancedCSCPixelFormat;


typedef enum
{
	NTV2_Enhanced_CSC_Chroma_Filter_Select_Full,
	NTV2_Enhanced_CSC_Chroma_Filter_Select_Simple,
	NTV2_Enhanced_CSC_Chroma_Filter_Select_None,

	NTV2_Enhanced_CSC_Num_Chroma_Filter_Selects
} NTV2EnhancedCSCChromaFilterSelect;


typedef enum
{
	NTV2_Enhanced_CSC_Chroma_Edge_Control_Black,
	NTV2_Enhanced_CSC_Chroma_Edge_Control_Extended,

	NTV2_Enhanced_CSC_Num_Chroma_Edge_Controls
} NTV2EnhancedCSCChromaEdgeControl;


typedef enum
{
	NTV2_Enhanced_CSC_Key_Source_Key_Input,
	NTV2_Enhanced_CSC_Key_Spurce_A0_Input,

	NTV2_Enhanced_CSC_Num_Key_Sources
} NTV2EnhancedCSCKeySource;


typedef enum
{
	NTV2_Enhanced_CSC_Key_Output_Range_Full,
	NTV2_Enhanced_CSC_Key_Output_Range_SMPTE,

	NTV2_Enhanced_CSC_Num_Key_Output_Ranges
} NTV2EnhancedCSCKeyOutputRange;


/**
	@brief	This class controls the enhanced color space converter.
**/
class AJAExport CNTV2EnhancedCSC
{
//	Class Methods
public:
	//	Construction, Copying, Assigning
	#if defined (NTV2_DEPRECATE)
		explicit	CNTV2EnhancedCSC ()	{	};
	#else
		explicit	CNTV2EnhancedCSC (CNTV2Card & inCard)
						:	mDevice (inCard)	{	};
	#endif

	virtual inline	~CNTV2EnhancedCSC ()		{	}


//	Instance Methods
public:
	inline			CNTV2CSCMatrix &			Matrix	(void)			{return mMatrix;}
	inline	const	CNTV2CSCMatrix &			Matrix	(void) const	{return mMatrix;}

			bool								SetInputPixelFormat		(const NTV2EnhancedCSCPixelFormat inPixelFormat);
	inline	NTV2EnhancedCSCPixelFormat			GetInputPixelFormat		(void) const		{return mInputPixelFormat;}

			bool								SetOutputPixelFormat	(const NTV2EnhancedCSCPixelFormat inPixelFormat);
	inline	NTV2EnhancedCSCPixelFormat			GetOutputPixelFormat	(void) const		{return mOutputPixelFormat;}

			bool								SetChromaFilterSelect	(const NTV2EnhancedCSCChromaFilterSelect inChromaFilterSelect);
	inline	NTV2EnhancedCSCChromaFilterSelect	GetChromaFilterSelect	(void) const		{return mChromaFilterSelect;}

			bool								SetChromaEdgeControl	(const NTV2EnhancedCSCChromaEdgeControl inChromaEdgeControl);
	inline	NTV2EnhancedCSCChromaEdgeControl	GetChromaEdgeControl	(void) const		{return mChromaEdgeControl;}

			bool								SetKeySource			(const NTV2EnhancedCSCKeySource inKeySource);
	inline	NTV2EnhancedCSCKeySource			GetKeySource			(void) const		{return mKeySource;}

			bool								SetKeyOutputRange		(const NTV2EnhancedCSCKeyOutputRange inKeyOutputRange);
	inline	NTV2EnhancedCSCKeyOutputRange		GetKeyOutputRange		(void) const		{return mKeyOutputRange;}

			bool								SetKeyInputOffset		(const int16_t		inKeyInputOffset);
	inline	int16_t								GetKeyInputOffset		(void) const		{return mKeyInputOffset;}

			bool								SetKeyOutputOffset		(const uint16_t		inKeyOutputOffset);
	inline	uint16_t							GetKeyOutputOffset		(void) const		{return mKeyOutputOffset;}

			bool								SetKeyGain				(const double		inKeyGain);
	inline	double								GetKeyGain				(void) const		{return mKeyGain;}

			bool								SendToHardware			(CNTV2Card & inDevice, const NTV2Channel inChannel) const;
			bool								GetFromHardware			(CNTV2Card & inDevice, const NTV2Channel inChannel);
	#if !defined (NTV2_DEPRECATE)
	inline	bool								SendToHardware			(const NTV2Channel	inChannel)		{return SendToHardware (mDevice, inChannel);}
	inline	bool								GetFromHardware			(const NTV2Channel	inChannel)		{return GetFromHardware (mDevice, inChannel);}
	#endif	//	!defined (NTV2_DEPRECATE)

//	Instance Data
private:
	NTV2EnhancedCSCPixelFormat					mInputPixelFormat;
	NTV2EnhancedCSCPixelFormat					mOutputPixelFormat;
	NTV2EnhancedCSCChromaFilterSelect			mChromaFilterSelect;
	NTV2EnhancedCSCChromaEdgeControl			mChromaEdgeControl;
	NTV2EnhancedCSCKeySource					mKeySource;
	NTV2EnhancedCSCKeyOutputRange				mKeyOutputRange;
	int16_t										mKeyInputOffset;
	uint16_t									mKeyOutputOffset;
	double										mKeyGain;
	CNTV2CSCMatrix								mMatrix;
	#if !defined (NTV2_DEPRECATE)
		CNTV2Card &								mDevice;
	#endif	//	!defined (NTV2_DEPRECATE)

};	//	CNTV2EnhancedCSC


#endif	//	NTV2_ENHANCED_CSC_H

