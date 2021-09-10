/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2cscmatrix.h
	@brief		Declares the utility class for abstract color space matrix operations.
	@copyright	2004-2021 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef NTV2_CSCMATRIX_H
#define NTV2_CSCMATRIX_H


#include "ajatypes.h"
#include "ajaexport.h"
#include "ntv2enums.h"
#include "ntv2card.h"


const int16_t NTV2_CSCMatrix_ZeroOffset	  = 0x0000;
const int16_t NTV2_CSCMatrix_SMPTEYOffset = 0x0800;	// 0x10 with added ms sign bit and ls-padded to 16 bits
const int16_t NTV2_CSCMatrix_SMPTECOffset = 0x4000;	// 0x80 with added ms sign bit and ls-padded to 16 bits


typedef enum NTV2CSCCoeff
{
	NTV2CSCCoeffIndex_A0,		///< @brief	Proportion of input component 0 applied to A output
	NTV2CSCCoeffIndex_A1,		///< @brief	Proportion of input component 1 applied to A output
	NTV2CSCCoeffIndex_A2,		///< @brief	Proportion of input component 2 applied to A output

	NTV2CSCCoeffIndex_B0,		///< @brief	Proportion of input component 0 applied to B output
	NTV2CSCCoeffIndex_B1,		///< @brief	Proportion of input component 1 applied to B output
	NTV2CSCCoeffIndex_B2,		///< @brief	Proportion of input component 2 applied to B output

	NTV2CSCCoeffIndex_C0,		///< @brief	Proportion of input component 0 applied to C output
	NTV2CSCCoeffIndex_C1,		///< @brief	Proportion of input component 1 applied to C output
	NTV2CSCCoeffIndex_C2 	  	///< @brief	Proportion of input component 2 applied to C output
} NTV2CSCCoeffIndex;


typedef enum NTV2CSCOffset
{
	NTV2CSCOffsetIndex_Pre0,
	NTV2CSCOffsetIndex_Pre1,
	NTV2CSCOffsetIndex_Pre2,

	NTV2CSCOffsetIndex_PostA,
	NTV2CSCOffsetIndex_PostB,
	NTV2CSCOffsetIndex_PostC
} NTV2CSCOffsetIndex;


/**
	@brief	This is used to compare CSC Matrix coefficients -- the default amount by which
			they can differ and still be considered "equal".
**/
const double NTV2CSCMatrix_MaxCoeffDiff = (1.0 / 4096.0);


/**
	@brief		This class supports the color space conversion matrix described below.

	@details	The CSC matrix consists of a pre-subtractor, a 3x3 matrix multiplier,
				and a post-adder. It receives up to three components as input (labeled
				Input0, Input1, and Input2), cross-processes them, and outputs them as
				OutputA, OutputB, OutputC. The standard component assignment for GBR
				and YCbCr video is:
									   YCbCr   RGB
					Input0 / OutputA =	 Y		G
					Input1 / OutputB =	 Cb		B
					Input2 / OutputC =	 Cr		R

				The basic purpose of the pre-subtractor is to remove any DC bias(es) that
				may be on the input video prior to the matrix multiply. For example, YCbCr
				video typically has a "black" offset of 0x10 for luminance (Y) and 0x80
				for chrominance (Cb/Cr - values given for 8-bit video). In general, these
				offsets must be subtracted from the video before using the 3x3 matrix.

				Similarly, the post-adder is typically used to re-add "black" biases to
				the output video. Note that both pre- and post-offsets are typically "positive"
				values, and assume a left-justified representation within a 16-bit integer.
				In the above example, Y Black offset would be 0x0800 (i.e. 0x10 with an added
				sign bit and ls-padded to 16 bits). Similarly, C Black Offset is 0x4000
				(0x80 with a sign bit and ls-padded to 16 bits).

				The 3x3 matrix coefficients are labeled according to the input (0, 1, or 2)
				and output (A, B, or C) they affect. For example, coefficient "A0" determines
				the amount of Input0 that is applied to OutputA. The matrix form of the
				equation can be represented as:

				[OutA OutB OutC] = [In0 In1 In2] [ A0  B0  C0
												   A1  B1  C1
												   A2  B2  C2 ]

				This class has methods for presetting known matrix values (for "typical"
				RGB <=> YUV conversions), as well as some typical "gain" and "hue rotation"
				matrices. It also includes methods for combining matrices for more complex
				effects, including matrix multiplication.
**/
class AJAExport CNTV2CSCMatrix
{
public:
	/**
		@brief		Instantiates and initializes the instance with a known set of coefficients.
	 	@param[in]	inPreset		Specifies an initial setting for the matrix coefficients.
	**/
	explicit					CNTV2CSCMatrix (const NTV2ColorSpaceMatrixType inPreset = NTV2_Unity_Matrix);


	/**
		@brief		My destructor.
	**/
	virtual	inline				~CNTV2CSCMatrix ()	{};


	/**
		@brief		Replace the current matrix coefficients with a known set of coefficients.
	 	@param[in]	inPreset		Specifies a new setting for the matrix coefficients.
	**/
	virtual void				InitMatrix (const NTV2ColorSpaceMatrixType inPreset);


	/**
		@brief		Returns the value of a requested matrix coefficient.
		@param[in]	inCoeffIndex	Specifies which matrix coefficient to return.
		@return		The value of the requested coefficient.
	**/
	virtual double				GetCoefficient (const NTV2CSCCoeffIndex inCoeffIndex) const;


	/**
		@brief		Changes the specified matrix coefficient to the given value.
		@param[in]	inCoeffIndex	Specifies which matrix coefficient to change.
		@param[in]	inCoefficient	Specifies the new matrix coefficient value.
	**/
	virtual void				SetCoefficient (const NTV2CSCCoeffIndex inCoeffIndex, const double inCoefficient);


	/**
		@brief		Returns the value of a requested offset.
		@param[in]	inOffsetIndex	Specifies which offset to return.
		@return		The value of the requested offset.
	**/
	virtual int16_t				GetOffset (const NTV2CSCOffsetIndex inOffsetIndex) const;


	/**
		@brief		Changes the specified offset to the given value.
		@param[in]	inOffsetIndex	Specifies which offset to change.
		@param[in]	inOffset		Specifies the new offset value.
	**/
	virtual void				SetOffset (const NTV2CSCOffsetIndex inOffsetIndex, const int16_t inOffset);


	/**
		@brief		Pre-multiply the current 3x3 matrix by the matrix of the given instance.
		@param[in]	inPreInstance	Specifies a reference to a CNTV2CSCMatrix instance.
	**/
	virtual void				PreMultiply (const CNTV2CSCMatrix & inPreInstance);


	/**
		@brief		Post-multiply the current 3x3 matrix by the matrix of the given instance.
	 *	@param[in]	inPostInstance	Specifies a reference to a CNTV2CSCMatrix instance.
	 */
	virtual void				PostMultiply (const CNTV2CSCMatrix & inPostInstance);


	/**
		@brief		Set the matrix diagonals (gains).
		@param[in]	inGain0			Specifies the amount of gain to be applied to the 1st channel (unity = 1.0)
		@param[in]	inGain1			Specifies the amount of gain to be applied to the 2nd channel
		@param[in]	inGain2			Specifies the amount of gain to be applied to the 3rd channel
	**/
	virtual void				SetGain (const double inGain0, const double inGain1, const double inGain2);


	/**
		@brief		Set matrix for hue rotation.
	 	@param[in]	inDegrees		Amount of Cb/Cr "rotation" (in degrees)
	 	@note		Assumes chroma channels in In 1/Out B, In 2/Out C. This will not work for RGB.
	**/
	virtual void				SetHueRotate (const double inDegrees);


	/**
		@brief		Set pre-offsets.
	 	@param[in]	inOffset0		The zero, or "black" level of the 1st input channel
	 	@param[in]	inOffset1		The zero, or "black" level of the 2nd input channel
	 	@param[in]	inOffset2		The zero, or "black" level of the 3rd input channel
	**/
	virtual void				SetPreOffsets (const int16_t inOffset0, const int16_t inOffset1, const int16_t inOffset2);


	/**
	 	@brief		Add pre-offsets.
	 	@param[in]	inIncrement0	The amount to be added to the existing preOffset0
	 	@param[in]	inIncrement1	The amount to be added to the existing preOffset1
	 	@param[in]	inIncrement2	The amount to be added to the existing preOffset2
	**/
	virtual void				AddPreOffsets (const int16_t inIncrement0, const int16_t inIncrement1, const int16_t inIncrement2);


	/**
		@brief		Set post-offsets.
	 	@param[in]	inOffsetA		The zero, or "black" level of the 1st output channel
	 	@param[in]	inOffsetB		The zero, or "black" level of the 2nd output channel
	 	@param[in]	inOffsetC		The zero, or "black" level of the 3rd output channel
	**/
	virtual void				SetPostOffsets (const int16_t inOffsetA, const int16_t inOffsetB, const int16_t inOffsetC);


	/**
	 	@brief		Add post-offsets.
	 	@param[in]	inIncrementA	The amount to be added to the existing postOffsetA
	 	@param[in]	inIncrementB	The amount to be added to the existing postOffsetB
	 	@param[in]	inIncrementC	The amount to be added to the existing postOffsetC
	**/
	virtual void				AddPostOffsets (const int16_t inIncrementA, const int16_t inIncrementB, const int16_t inIncrementC);


	/**
		@brief		Returns 'true' if this CSC Matrix has unity matrix coefficients.
		@return		True if the matrix is a unity matrix; otherwise false
		@note		ONLY compares matrix coefficients, NOT offsets.
	**/
	virtual bool				IsUnityMatrix (void);


	/**
		@brief		Returns true if the matrix coefficients and offsets EXACTLY match those of the specified instance.
		@note		Compares matrix coefficients and offsets.
	**/
	virtual	bool				operator == (const CNTV2CSCMatrix & rhs) const;


	/**
		@brief		Returns true if the matrix coefficients and offsets differ from those of the specified instance.
		@note		Compares matrix coefficients and offsets.
	**/
	virtual inline bool			operator != (const CNTV2CSCMatrix & rhs) const							{return !(*this == rhs);}


	/**
		@brief		Tests if the matrix coefficients can all be considered equal to those of the specified instance,
		@param[in]	inCSCMatrix		Specifies a reference to a CNTV2CSCMatrix instance.
		@param[in]	inMaxDiff		Specifies the amount by which two coefficients can differ and still be considered "equal"
		@return		True if all the matrix coefficients differ by an amount less than the tolerance; otherwise false
		@note		ONLY compares matrix coefficients, NOT offsets.
	**/
	virtual bool				IsEqual (const CNTV2CSCMatrix & inCSCMatrix, const double inMaxDiff = NTV2CSCMatrix_MaxCoeffDiff);


	/**
		@brief		Tests if two coefficients differ by less than the fiven tolerance.
	 	@param[in]	inCoeff1	Specifies the first coefficient to be compared
	 	@param[in]	inCoeff2	Specifies the second coefficient to be compared
		@param[in]	inMaxDiff	Specifies the amount by which two coefficients can differ and still be considered "equal"
		@return		True if the the two oefficients differ by an amount less than the tolerance; otherwise false
	**/
	virtual bool				CoeffEqual (const double inCoeff1, const double inCoeff2, const double inMaxDiff);


	//	Instance Data
	private:
		double mA0;							///< @brief	Specifies the amount of Input 0 to be applied to Output A
		double mA1;							///< @brief	Specifies the amount of Input 1 to be applied to Output A
		double mA2;							///< @brief	Specifies the amount of Input 2 to be applied to Output A

		double mB0;							///< @brief	Specifies the amount of Input 0 to be applied to Output B
		double mB1;							///< @brief	Specifies the amount of Input 1 to be applied to Output B
		double mB2;							///< @brief	Specifies the amount of Input 2 to be applied to Output B

		double mC0;							///< @brief	Specifies the amount of Input 0 to be applied to Output C
		double mC1;							///< @brief	Specifies the amount of Input 1 to be applied to Output C
		double mC2;							///< @brief	Specifies the amount of Input 2 to be applied to Output C

		int16_t mPreOffset0;				///< @brief	Specifies Input 0's offset from zero
		int16_t mPreOffset1;				///< @brief	Specifies Input 1's offset from zero
		int16_t mPreOffset2;				///< @brief	Specifies Input 2's offset from zero

		int16_t mPostOffsetA;				///< @brief	Specifies Output A's offset from zero
		int16_t mPostOffsetB;				///< @brief	Specifies Output B's offset from zero
		int16_t	mPostOffsetC;				///< @brief	Specifies Output C's offset from zero

		NTV2ColorSpaceMatrixType mPreset;	///< @brief	Specifies if the matrix coefficients correspond to preset values

};	//	CNTV2CSCMatrix

#endif	//	NTV2_CSCMATRIX_H
