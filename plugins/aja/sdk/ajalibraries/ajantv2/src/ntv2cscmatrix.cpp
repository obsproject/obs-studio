/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2cscmatrix.cpp
    @brief		Implementation of the CNTV2CSCMatrix class for abstract color space matrix operations.
    @copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2cscmatrix.h"
#include <math.h>

static const double kPi(3.1415926536);


CNTV2CSCMatrix::CNTV2CSCMatrix (const NTV2ColorSpaceMatrixType inPreset)
{
    InitMatrix (inPreset);
}


void CNTV2CSCMatrix::InitMatrix (const NTV2ColorSpaceMatrixType inPreset)
{
    switch (inPreset)
    {
    case NTV2_Unity_Matrix:
    default:
        mA0 = 1.0;
        mA1 = 0.0;
        mA2 = 0.0;
        mB0 = 0.0;
        mB1 = 1.0;
        mB2 = 0.0;
        mC0 = 0.0;
        mC1 = 0.0;
        mC2 = 1.0;

        mPreOffset0 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset1 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset2 = NTV2_CSCMatrix_ZeroOffset;

        mPostOffsetA = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetB = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetC = NTV2_CSCMatrix_ZeroOffset;
        break;

    case NTV2_Unity_SMPTE_Matrix:
        mA0 = 1.0;
        mA1 = 0.0;
        mA2 = 0.0;
        mB0 = 0.0;
        mB1 = 1.0;
        mB2 = 0.0;
        mC0 = 0.0;
        mC1 = 0.0;
        mC2 = 1.0;

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTECOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTECOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTECOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTECOffset;
        break;

        // RGB full range -> YCbCr Rec 709 (HD)
    case NTV2_GBRFull_to_YCbCr_Rec709_Matrix:
        mA0 =  0.612427;		// G -> Y
        mA1 =  0.061829;		// B -> Y
        mA2 =  0.182068;		// R -> Y
        mB0 = -0.337585;		// G -> Cb
        mB1 =  0.437927;		// B -> Cb
        mB2 = -0.100342;		// R -> Cb
        mC0 = -0.397766;		// G -> Cr
        mC1 = -0.040161;		// B -> Cr
        mC2 =  0.437927;		// R -> Cr

        mPreOffset0 = NTV2_CSCMatrix_ZeroOffset;		// full-range RGB: no offset
        mPreOffset1 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset2 = NTV2_CSCMatrix_ZeroOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;	// YCbCr: "SMPTE" offsets
        mPostOffsetB = NTV2_CSCMatrix_SMPTECOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTECOffset;
        break;

        // RGB full range -> YCbCr Rec 2020
    case NTV2_GBRFull_to_YCbCr_Rec2020_Matrix:
        mA0 =  0.58057;		// G -> Y
        mA1 =  0.05078;		// B -> Y
        mA2 =  0.22495;		// R -> Y
        mB0 = -0.31563;		// G -> Cb
        mB1 =  0.43793;		// B -> Cb
        mB2 = -0.12230;		// R -> Cb
        mC0 = -0.40271;		// G -> Cr
        mC1 = -0.03522;		// B -> Cr
        mC2 =  0.43793;		// R -> Cr

        mPreOffset0 = NTV2_CSCMatrix_ZeroOffset;		// full-range RGB: no offset
        mPreOffset1 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset2 = NTV2_CSCMatrix_ZeroOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;	// YCbCr: "SMPTE" offsets
        mPostOffsetB = NTV2_CSCMatrix_SMPTECOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTECOffset;
        break;

        // RGB full range -> YCbCr Rec 601 (SD)
    case NTV2_GBRFull_to_YCbCr_Rec601_Matrix:
        mA0 =  0.502655;		// G -> Y
        mA1 =  0.097626;		// B -> Y
        mA2 =  0.256042;		// R -> Y
        mB0 = -0.290131;		// G -> Cb
        mB1 =  0.437927;		// B -> Cb
        mB2 = -0.147797;		// R -> Cb
        mC0 = -0.366699;		// G -> Cr
        mC1 = -0.071228;		// B -> Cr
        mC2 =  0.437927;		// R -> Cr

        mPreOffset0 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset1 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset2 = NTV2_CSCMatrix_ZeroOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTECOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTECOffset;
        break;

        // RGB SMPTE range -> YCbCr Rec 709 (HD)
    case NTV2_GBRSMPTE_to_YCbCr_Rec709_Matrix:
        mA0 =  0.715210;		// G -> Y
        mA1 =  0.072205;		// B -> Y
        mA2 =  0.212585;		// R -> Y
        mB0 = -0.394226;		// G -> Cb
        mB1 =  0.511414;		// B -> Cb
        mB2 = -0.117188;		// R -> Cb
        mC0 = -0.464508;		// G -> Cr
        mC1 = -0.046906;		// B -> Cr
        mC2 =  0.511414;		// R -> Cr

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTEYOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTECOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTECOffset;
        break;


        // RGB SMPTE range -> YCbCr Rec 2020
    case NTV2_GBRSMPTE_to_YCbCr_Rec2020_Matrix:
        mA0 =  0.67800;		// G -> Y
        mA1 =  0.05930;		// B -> Y
        mA2 =  0.26270;		// R -> Y
        mB0 = -0.368594;		// G -> Cb
        mB1 =  0.511414;		// B -> Cb
        mB2 = -0.14282;		// R -> Cb
        mC0 = -0.470284;		// G -> Cr
        mC1 = -0.04113;		// B -> Cr
        mC2 =  0.511414;		// R -> Cr

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTEYOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTECOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTECOffset;
        break;

        // RGB SMPTE range -> YCbCr Rec 601 (SD)
    case NTV2_GBRSMPTE_to_YCbCr_Rec601_Matrix:
        mA0 =  0.587006;		// G -> Y
        mA1 =  0.113983;		// B -> Y
        mA2 =  0.299011;		// R -> Y
        mB0 = -0.338837;		// G -> Cb
        mB1 =  0.511414;		// B -> Cb
        mB2 = -0.172577;		// R -> Cb
        mC0 = -0.428253;		// G -> Cr
        mC1 = -0.083160;		// B -> Cr
        mC2 =  0.511414;		// R -> Cr

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTEYOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTECOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTECOffset;
        break;



        // YCbCr -> RGB full range Rec 709 (HD)
    case NTV2_YCbCr_to_GBRFull_Rec709_Matrix:
        mA0 =  1.167786;		// Y  -> G
        mA1 = -0.213898;		// Cb -> G
        mA2 = -0.534515;		// Cr -> G
        mB0 =  1.167786;		// Y  -> B
        mB1 =  2.118591;		// Cb -> B
        mB2 =  0.000000;		// Cr -> B
        mC0 =  1.167786;		// Y  -> R
        mC1 =  0.000000;		// Cb -> R
        mC2 =  1.797974;		// Cr -> R

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;		// YCbCr: "SMPTE" offsets
        mPreOffset1 = NTV2_CSCMatrix_SMPTECOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTECOffset;

        mPostOffsetA = NTV2_CSCMatrix_ZeroOffset;		// full-range RGB: no offset
        mPostOffsetB = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetC = NTV2_CSCMatrix_ZeroOffset;
        break;

        // YCbCr -> RGB full range Rec 2020
    case NTV2_YCbCr_to_GBRFull_Rec2020_Matrix:
        mA0 =  1.167786;		// Y  -> G
        mA1 = -0.187877;		// Cb -> G
        mA2 = -0.652337;		// Cr -> G
        mB0 =  1.167786;		// Y  -> B
        mB1 =  2.148061;		// Cb -> B
        mB2 =  0.000000;		// Cr -> B
        mC0 =  1.167786;		// Y  -> R
        mC1 =  0.000000;		// Cb -> R
        mC2 =  1.683611;		// Cr -> R

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;		// YCbCr: "SMPTE" offsets
        mPreOffset1 = NTV2_CSCMatrix_SMPTECOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTECOffset;

        mPostOffsetA = NTV2_CSCMatrix_ZeroOffset;		// full-range RGB: no offset
        mPostOffsetB = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetC = NTV2_CSCMatrix_ZeroOffset;
        break;

        // YCbCr -> RGB full range Rec 601 (SD)
    case NTV2_YCbCr_to_GBRFull_Rec601_Matrix:
        mA0 =  1.167786;		// Y  -> G
        mA1 = -0.392944;		// Cb -> G
        mA2 = -0.815399;		// Cr -> G
        mB0 =  1.167786;		// Y  -> B
        mB1 =  2.023163;		// Cb -> B
        mB2 =  0.000000;		// Cr -> B
        mC0 =  1.167786;		// Y  -> R
        mC1 =  0.000000;		// Cb -> R
        mC2 =  1.600708;		// Cr -> R

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTECOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTECOffset;

        mPostOffsetA = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetB = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetC = NTV2_CSCMatrix_ZeroOffset;
        break;

        // YCbCr -> RGB SMPTE range Rec 709 (HD)
    case NTV2_YCbCr_to_GBRSMPTE_Rec709_Matrix:
        mA0 =  1.000000;		// Y  -> G
        mA1 = -0.183167;		// Cb -> G
        mA2 = -0.457642;		// Cr -> G
        mB0 =  1.000000;		// Y  -> B
        mB1 =  1.814148;		// Cb -> B
        mB2 =  0.000000;		// Cr -> B
        mC0 =  1.000000;		// Y  -> R
        mC1 =  0.000000;		// Cb -> R
        mC2 =  1.539673;		// Cr -> R

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTECOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTECOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTEYOffset;
        break;

        // YCbCr -> RGB SMPTE range Rec 2020
    case NTV2_YCbCr_to_GBRSMPTE_Rec2020_Matrix:
        mA0 =  1.000000;		// Y  -> G
        mA1 = -0.160880066;		// Cb -> G
        mA2 = -0.5585997088;		// Cr -> G
        mB0 =  1.000000;		// Y  -> B
        mB1 =  1.839404464;		// Cb -> B
        mB2 =  0.000000;		// Cr -> B
        mC0 =  1.000000;		// Y  -> R
        mC1 =  0.000000;		// Cb -> R
        mC2 =  1.441684821;		// Cr -> R

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTECOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTECOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTEYOffset;
        break;

        // YCbCr -> RGB SMPTE range Rec 601 (SD)
    case NTV2_YCbCr_to_GBRSMPTE_Rec601_Matrix:
        mA0 =  1.000000;		// Y  -> G
        mA1 = -0.336426;		// Cb -> G
        mA2 = -0.698181;		// Cr -> G
        mB0 =  1.000000;		// Y  -> B
        mB1 =  1.732483;		// Cb -> B
        mB2 =  0.000000;		// Cr -> B
        mC0 =  1.000000;		// Y  -> R
        mC1 =  0.000000;		// Cb -> R
        mC2 =  1.370728;		// Cr -> R

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTECOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTECOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTEYOffset;
        break;

        // YCbCr Rec 601 (SD) -> YCbCr Rec 709 (HD)
    case NTV2_YCbCrRec601_to_YCbCrRec709_Matrix:
        mA0 =  1.00000000;	// Y  -> Y
        mA1 = -0.11554975;	// Cb -> Y
        mA2 = -0.20793764;	// Cr -> Y
        mB0 =  0.00000000;	// Y  -> Cb
        mB1 =  1.01863972;	// Cb -> Cb
        mB2 =  0.11461795;	// Cr -> Cb
        mC0 =  0.00000000;	// Y  -> Cr
        mC1 =  0.07504945;	// Cb -> Cr
        mC2 =  1.02532707;	// Cr -> Cr

        mPreOffset0 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset1 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset2 = NTV2_CSCMatrix_ZeroOffset;

        mPostOffsetA = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetB = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetC = NTV2_CSCMatrix_ZeroOffset;
        break;

        // YCbCr Rec 709 (HD) -> YCbCr Rec 601 (SD)
    case NTV2_YCbCrRec709_to_YCbCrRec601_Matrix:
        mA0 =  1.00000000;	// Y  -> Y
        mA1 =  0.09931166;	// Cb -> Y
        mA2 =  0.19169955;	// Cr -> Y
        mB0 =  0.00000000;	// Y  -> Cb
        mB1 =  0.98985381;	// Cb -> Cb
        mB2 = -0.11065251;	// Cr -> Cb
        mC0 =  0.00000000;	// Y  -> Cr
        mC1 = -0.07245296;	// Cb -> Cr
        mC2 =  0.98339782;	// Cr -> Cr

        mPreOffset0 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset1 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset2 = NTV2_CSCMatrix_ZeroOffset;

        mPostOffsetA = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetB = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetC = NTV2_CSCMatrix_ZeroOffset;
        break;



        // RGB Full range -> RGB SMPTE
    case NTV2_GBRFull_to_GBRSMPTE_Matrix:
        mA0 =  0.855469;		// Gf -> Gs
        mA1 =  0.000000;		// Bf -> Gs
        mA2 =  0.000000;		// Rf -> Gs
        mB0 =  0.000000;		// Gf -> Bs
        mB1 =  0.855469;		// Bf -> Bs
        mB2 =  0.000000;		// Rf -> Bs
        mC0 =  0.000000;		// Gf -> Rs
        mC1 =  0.000000;		// Bf -> Rs
        mC2 =  0.855469;		// Rf -> Rs

        mPreOffset0 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset1 = NTV2_CSCMatrix_ZeroOffset;
        mPreOffset2 = NTV2_CSCMatrix_ZeroOffset;

        mPostOffsetA = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetB = NTV2_CSCMatrix_SMPTEYOffset;
        mPostOffsetC = NTV2_CSCMatrix_SMPTEYOffset;
        break;

        // RGB SMPTE range -> RGB Full
    case NTV2_GBRSMPTE_to_GBRFull_Matrix:
        mA0 =  1.168950;		// Gs -> Gf
        mA1 =  0.000000;		// Bs -> Gf
        mA2 =  0.000000;		// Rs -> Gf
        mB0 =  0.000000;		// Gs -> Bf
        mB1 =  1.168950;		// Bs -> Bf
        mB2 =  0.000000;		// Rs -> Bf
        mC0 =  0.000000;		// Gs -> Rf
        mC1 =  0.000000;		// Bs -> Rf
        mC2 =  1.168950;		// Rs -> Rf

        mPreOffset0 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset1 = NTV2_CSCMatrix_SMPTEYOffset;
        mPreOffset2 = NTV2_CSCMatrix_SMPTEYOffset;

        mPostOffsetA = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetB = NTV2_CSCMatrix_ZeroOffset;
        mPostOffsetC = NTV2_CSCMatrix_ZeroOffset;
        break;
    }

    mPreset = inPreset;
}


double CNTV2CSCMatrix::GetCoefficient (const NTV2CSCCoeffIndex inCoeffIndex) const
{
    switch (inCoeffIndex)
    {
    case NTV2CSCCoeffIndex_A0:	return mA0;
    case NTV2CSCCoeffIndex_A1:	return mA1;
    case NTV2CSCCoeffIndex_A2:	return mA2;
    case NTV2CSCCoeffIndex_B0:	return mB0;
    case NTV2CSCCoeffIndex_B1:	return mB1;
    case NTV2CSCCoeffIndex_B2:	return mB2;
    case NTV2CSCCoeffIndex_C0:	return mC0;
    case NTV2CSCCoeffIndex_C1:	return mC1;
    case NTV2CSCCoeffIndex_C2:	return mC2;
    }
    //	Must never get here
    NTV2_ASSERT (false && "Unhandled coefficient index");
    return 0.0;
}


void CNTV2CSCMatrix::SetCoefficient (const NTV2CSCCoeffIndex inCoeffIndex, const double inCoefficient)
{
    switch (inCoeffIndex)
    {
    case NTV2CSCCoeffIndex_A0:	mA0 = inCoefficient;	break;
    case NTV2CSCCoeffIndex_A1:	mA1 = inCoefficient;	break;
    case NTV2CSCCoeffIndex_A2:	mA2 = inCoefficient;	break;
    case NTV2CSCCoeffIndex_B0:	mB0 = inCoefficient;	break;
    case NTV2CSCCoeffIndex_B1:	mB1 = inCoefficient;	break;
    case NTV2CSCCoeffIndex_B2:	mB2 = inCoefficient;	break;
    case NTV2CSCCoeffIndex_C0:	mC0 = inCoefficient;	break;
    case NTV2CSCCoeffIndex_C1:	mC1 = inCoefficient;	break;
    case NTV2CSCCoeffIndex_C2:	mC2 = inCoefficient;	break;
    }
    mPreset = NTV2_Custom_Matrix;
}


int16_t CNTV2CSCMatrix::GetOffset (const NTV2CSCOffsetIndex inOffsetIndex) const
{
    switch (inOffsetIndex)
    {
    case NTV2CSCOffsetIndex_Pre0:	return mPreOffset0;
    case NTV2CSCOffsetIndex_Pre1:	return mPreOffset1;
    case NTV2CSCOffsetIndex_Pre2:	return mPreOffset2;
    case NTV2CSCOffsetIndex_PostA:	return mPostOffsetA;
    case NTV2CSCOffsetIndex_PostB:	return mPostOffsetB;
    case NTV2CSCOffsetIndex_PostC:	return mPostOffsetC;
    }
    //	Must never get here
    NTV2_ASSERT(false && "Unhandled offset index");
    return 0;
}


void CNTV2CSCMatrix::SetOffset (const NTV2CSCOffsetIndex inOffsetIndex, const int16_t inOffset)
{
    switch (inOffsetIndex)
    {
    case NTV2CSCOffsetIndex_Pre0:	mPreOffset0 = inOffset;		break;
    case NTV2CSCOffsetIndex_Pre1:	mPreOffset1 = inOffset;		break;
    case NTV2CSCOffsetIndex_Pre2:	mPreOffset2 = inOffset;		break;
    case NTV2CSCOffsetIndex_PostA:	mPostOffsetA = inOffset;	break;
    case NTV2CSCOffsetIndex_PostB:	mPostOffsetB = inOffset;	break;
    case NTV2CSCOffsetIndex_PostC:	mPostOffsetC = inOffset;	break;
    }
    mPreset = NTV2_Custom_Matrix;
}


void CNTV2CSCMatrix::PreMultiply (const CNTV2CSCMatrix & pre)
{
    // clone the current settings
    CNTV2CSCMatrix current (*this);

    // do matrix multiply
    mA0 = (pre.mA0 * current.mA0) + (pre.mB0 * current.mA1) + (pre.mC0 * current.mA2);
    mA1 = (pre.mA1 * current.mA0) + (pre.mB1 * current.mA1) + (pre.mC1 * current.mA2);
    mA2 = (pre.mA2 * current.mA0) + (pre.mB2 * current.mA1) + (pre.mC2 * current.mA2);

    mB0 = (pre.mA0 * current.mB0) + (pre.mB0 * current.mB1) + (pre.mC0 * current.mB2);
    mB1 = (pre.mA1 * current.mB0) + (pre.mB1 * current.mB1) + (pre.mC1 * current.mB2);
    mB2 = (pre.mA2 * current.mB0) + (pre.mB2 * current.mB1) + (pre.mC2 * current.mB2);

    mC0 = (pre.mA0 * current.mC0) + (pre.mB0 * current.mC1) + (pre.mC0 * current.mC2);
    mC1 = (pre.mA1 * current.mC0) + (pre.mB1 * current.mC1) + (pre.mC1 * current.mC2);
    mC2 = (pre.mA2 * current.mC0) + (pre.mB2 * current.mC1) + (pre.mC2 * current.mC2);

    mPreset = NTV2_Custom_Matrix;
}


void CNTV2CSCMatrix::PostMultiply (const CNTV2CSCMatrix & post)
{
    // clone the current settings
    CNTV2CSCMatrix current (*this);

    // do matrix multiply
    mA0 = (current.mA0 * post.mA0) + (current.mB0 * post.mA1) + (current.mC0 * post.mA2);
    mA1 = (current.mA1 * post.mA0) + (current.mB1 * post.mA1) + (current.mC1 * post.mA2);
    mA2 = (current.mA2 * post.mA0) + (current.mB2 * post.mA1) + (current.mC2 * post.mA2);

    mB0 = (current.mA0 * post.mB0) + (current.mB0 * post.mB1) + (current.mC0 * post.mB2);
    mB1 = (current.mA1 * post.mB0) + (current.mB1 * post.mB1) + (current.mC1 * post.mB2);
    mB2 = (current.mA2 * post.mB0) + (current.mB2 * post.mB1) + (current.mC2 * post.mB2);

    mC0 = (current.mA0 * post.mC0) + (current.mB0 * post.mC1) + (current.mC0 * post.mC2);
    mC1 = (current.mA1 * post.mC0) + (current.mB1 * post.mC1) + (current.mC1 * post.mC2);
    mC2 = (current.mA2 * post.mC0) + (current.mB2 * post.mC1) + (current.mC2 * post.mC2);

    mPreset = NTV2_Custom_Matrix;
}


void CNTV2CSCMatrix::SetGain (const double inGain0, const double inGain1, const double inGain2)
{
    mA0 = inGain0;
    mB1 = inGain1;
    mC2 = inGain2;
    mPreset = NTV2_Custom_Matrix;
}


void CNTV2CSCMatrix::SetHueRotate (const double inDegrees)
{
    double rad = kPi * inDegrees / 180.0;
    mB1 =  cos (rad);
    mB2 =  sin (rad);
    mC1 = -sin (rad);
    mC2 =  cos (rad);
    mPreset = NTV2_Custom_Matrix;
}


void CNTV2CSCMatrix::SetPreOffsets (const int16_t inOffset0, const int16_t inOffset1, const int16_t inOffset2)
{
    mPreOffset0 = inOffset0;
    mPreOffset1 = inOffset1;
    mPreOffset2 = inOffset2;
    mPreset = NTV2_Custom_Matrix;
}


void CNTV2CSCMatrix::AddPreOffsets (const int16_t inOffset0, const int16_t inOffset1, const int16_t inOffset2)
{
    mPreOffset0 += inOffset0;
    mPreOffset1 += inOffset1;
    mPreOffset2 += inOffset2;
    mPreset = NTV2_Custom_Matrix;
}


void CNTV2CSCMatrix::SetPostOffsets (const int16_t inOffsetA, const int16_t inOffsetB, const int16_t inOffsetC)
{
    mPostOffsetA = inOffsetA;
    mPostOffsetB = inOffsetB;
    mPostOffsetC = inOffsetC;
    mPreset = NTV2_Custom_Matrix;
}

void CNTV2CSCMatrix::AddPostOffsets (const int16_t inOffsetA, const int16_t inOffsetB, const int16_t inOffsetC)
{
    mPostOffsetA += inOffsetA;
    mPostOffsetB += inOffsetB;
    mPostOffsetC += inOffsetC;
    mPreset = NTV2_Custom_Matrix;
}


bool CNTV2CSCMatrix::IsUnityMatrix (void)
{
    bool bResult = true;	// assume positive result: now look for a reason to negate it

    if (mA0 != 1.0 || mA1 != 0.0 || mA2 != 0.0)
        bResult = false;

    if (mB0 != 0.0 || mB1 != 1.0 || mB2 != 0.0)
        bResult = false;

    if (mC0 != 0.0 || mC1 != 0.0 || mC2 != 1.0)
        bResult = false;

    // note: we're NOT looking at offsets, so this test will return true for EITHER
    //       NTV2_Unity_Matrix or NTV2_Unity_SMPTE_Matrix
    return bResult;
}


bool CNTV2CSCMatrix::operator == (const CNTV2CSCMatrix & rhs) const
{
    if ( (mA0 != rhs.mA0) || (mA1 != rhs.mA1) || (mA2 != rhs.mA2) )
        return false;

    if ( (mB0 != rhs.mB0) || (mB1 != rhs.mB1) || (mB2 != rhs.mB2) )
        return false;

    if ( (mC0 != rhs.mC0) || (mC1 != rhs.mC1) || (mC2 != rhs.mC2) )
        return false;

    if ( (mPreOffset0 != rhs.mPreOffset0) || (mPreOffset1 != rhs.mPreOffset1) || (mPreOffset2 != rhs.mPreOffset2) )
        return false;

    if ( (mPostOffsetA != rhs.mPostOffsetA) || (mPostOffsetB != rhs.mPostOffsetB) || (mPostOffsetC != rhs.mPostOffsetC) )
        return false;

    return true;
}


bool CNTV2CSCMatrix::IsEqual (const CNTV2CSCMatrix & inCSCMatrix, const double inMaxDiff)
{
    bool bResult = true;	// assume positive result: now look for a reason to negate it

    if (!CoeffEqual(mA0, inCSCMatrix.mA0, inMaxDiff) || !CoeffEqual(mA1, inCSCMatrix.mA1, inMaxDiff) || !CoeffEqual(mA2, inCSCMatrix.mA2, inMaxDiff))
        bResult = false;

    if (!CoeffEqual(mB0, inCSCMatrix.mB0, inMaxDiff) || !CoeffEqual(mB1, inCSCMatrix.mB1, inMaxDiff) || !CoeffEqual(mB2, inCSCMatrix.mB2, inMaxDiff))
        bResult = false;

    if (!CoeffEqual(mC0, inCSCMatrix.mC0, inMaxDiff) || !CoeffEqual(mC1, inCSCMatrix.mC1, inMaxDiff) || !CoeffEqual(mC2, inCSCMatrix.mC2, inMaxDiff))
        bResult = false;

    return bResult;
}


bool CNTV2CSCMatrix::CoeffEqual (const double inCoeff1, const double inCoeff2, const double inMaxDiff)
{
    return ::fabs(inCoeff1 - inCoeff2) < inMaxDiff;
}

