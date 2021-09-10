/* SPDX-License-Identifier: MIT */
/**
	@file		testpatterngen.h
	@brief		Declares the AJATestPatternGen class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TESTPATTERN_GEN
#define AJA_TESTPATTERN_GEN
#if !defined(NTV2_DEPRECATE_15_0)
#include "types.h"
#include "videotypes.h"
#include "videoutilities.h"

#include <vector>
typedef std::vector<uint8_t> AJATestPatternBuffer;

enum AJATestPatternSelect
{
	AJA_TestPatt_ColorBars100,
	AJA_TestPatt_ColorBars75,
	AJA_TestPatt_Ramp,
	AJA_TestPatt_MultiBurst,
	AJA_TestPatt_LineSweep,
	AJA_TestPatt_CheckField,
	AJA_TestPatt_FlatField,
	AJA_TestPatt_MultiPattern,
	AJA_TestPatt_Black,
	AJA_TestPatt_White,
	AJA_TestPatt_Border,
	AJA_TestPatt_LinearRamp,
	AJA_TestPatt_SlantRamp,
	AJA_TestPatt_ZonePlate,
	AJA_TestPatt_ColorQuadrant,
	AJA_TestPatt_ColorQuadrantBorder,
    AJA_TestPatt_ColorQuadrantTSI,
	AJA_TestPatt_All
};

//*********************************************************************************

// CTestPattern

class AJA_EXPORT AJATestPatternGen
{
	// Construction
public:
	AJATestPatternGen();

	// Implementation
public:
	virtual ~AJATestPatternGen();

	virtual bool DrawTestPattern(AJATestPatternSelect pattNum, uint32_t frameWidth, uint32_t frameHeight, AJA_PixelFormat pixelFormat, AJATestPatternBuffer &testPatternBuffer);
	virtual bool DrawTestPattern(AJATestPatternSelect pattNum, uint32_t frameWidth, uint32_t frameHeight, 
								AJA_PixelFormat pixelFormat, AJA_BayerColorPhase phase, AJATestPatternBuffer &testPatternBuffer);

	void setSignalMask(AJASignalMask signalMask) { _signalMask = signalMask;}
	AJASignalMask getSignalMask() { return _signalMask;}
	void setSliderValue(double sliderValue) { _sliderValue = sliderValue;}
	double getSliderValue() { return _sliderValue;}

protected:

	virtual bool DrawSegmentedTestPattern();
	virtual bool DrawYCbCrFrame(uint16_t Y, uint16_t Cb, uint16_t Cr);	
	virtual bool DrawBorderFrame();
	virtual bool DrawLinearRampFrame();
	virtual bool DrawSlantRampFrame();
	virtual bool DrawZonePlateFrame();
	virtual bool DrawQuandrantBorderFrame();
	virtual bool DrawColorQuandrantFrame();
    virtual bool DrawColorQuandrantTSIFrame();

protected:
	AJATestPatternSelect _patternNumber;
	uint32_t _frameWidth;     
	uint32_t _frameHeight;     
	uint32_t _linePitch;     
	uint32_t _dataLinePitch; 
	uint32_t _bufferSize; 
	uint8_t* _pTestPatternBuffer;
	uint32_t* _pPackedLineBuffer;
	uint16_t* _pUnPackedLineBuffer;

	double _sliderValue;
	AJASignalMask _signalMask;
	AJA_PixelFormat _pixelFormat;
	AJA_BayerColorPhase _bayerPhase;

};	//	AJATestPatternGen

#endif	//	!defined(NTV2_DEPRECATE_15_0)

#endif	//	AJA_TESTPATTERN_GEN
