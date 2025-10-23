//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/vstspeaker.h
// Created by  : Steinberg, 01/2018
// Description : common defines
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
//------------------------------------------------------------------------
/** \defgroup speakerArrangements Speaker Arrangements
\image html "vst3_speaker_types.jpg"
\n
A SpeakerArrangement is a bitset combination of speakers. For example:
\code
const SpeakerArrangement kStereo = kSpeakerL | kSpeakerR; // => hex: 0x03 / binary: 0011.
\endcode
\see IAudioProcessor::getBusArrangement () and IAudioProcessor::setBusArrangements ()
*/
//------------------------------------------------------------------------

//------------------------------------------------------------------------
/** Speaker Definitions.
 * \ingroup speakerArrangements
 */
/**@{*/
const Speaker kSpeakerL    = 1 << 0;		///< Left (L)
const Speaker kSpeakerR    = 1 << 1;		///< Right (R)
const Speaker kSpeakerC    = 1 << 2;		///< Center (C)
const Speaker kSpeakerLfe  = 1 << 3;		///< Subbass (Lfe)
const Speaker kSpeakerLs   = 1 << 4;		///< Left Surround (Ls)
const Speaker kSpeakerRs   = 1 << 5;		///< Right Surround (Rs)
const Speaker kSpeakerLc   = 1 << 6;		///< Left of Center (Lc) - Front Left Center
const Speaker kSpeakerRc   = 1 << 7;		///< Right of Center (Rc) - Front Right Center
const Speaker kSpeakerS    = 1 << 8;		///< Surround (S)
const Speaker kSpeakerCs   = kSpeakerS;		///< Center of Surround (Cs) - Back Center - Surround (S)
const Speaker kSpeakerSl   = 1 << 9;		///< Side Left (Sl)
const Speaker kSpeakerSr   = 1 << 10;		///< Side Right (Sr)
const Speaker kSpeakerTc   = 1 << 11;		///< Top Center Over-head, Top Middle (Tc)
const Speaker kSpeakerTfl  = 1 << 12;		///< Top Front Left (Tfl)
const Speaker kSpeakerTfc  = 1 << 13;		///< Top Front Center (Tfc)
const Speaker kSpeakerTfr  = 1 << 14;		///< Top Front Right (Tfr)
const Speaker kSpeakerTrl  = 1 << 15;		///< Top Rear/Back Left (Trl)
const Speaker kSpeakerTrc  = 1 << 16;		///< Top Rear/Back Center (Trc)
const Speaker kSpeakerTrr  = 1 << 17;		///< Top Rear/Back Right (Trr)
const Speaker kSpeakerLfe2 = 1 << 18;		///< Subbass 2 (Lfe2)
const Speaker kSpeakerM    = 1 << 19;		///< Mono (M)

const Speaker kSpeakerACN0  = (Speaker)1 << 20;	///< Ambisonic ACN 0
const Speaker kSpeakerACN1  = (Speaker)1 << 21;	///< Ambisonic ACN 1
const Speaker kSpeakerACN2  = (Speaker)1 << 22;	///< Ambisonic ACN 2
const Speaker kSpeakerACN3  = (Speaker)1 << 23;	///< Ambisonic ACN 3
const Speaker kSpeakerACN4  = (Speaker)1 << 38;	///< Ambisonic ACN 4
const Speaker kSpeakerACN5  = (Speaker)1 << 39;	///< Ambisonic ACN 5
const Speaker kSpeakerACN6  = (Speaker)1 << 40;	///< Ambisonic ACN 6
const Speaker kSpeakerACN7  = (Speaker)1 << 41;	///< Ambisonic ACN 7
const Speaker kSpeakerACN8  = (Speaker)1 << 42;	///< Ambisonic ACN 8
const Speaker kSpeakerACN9  = (Speaker)1 << 43;	///< Ambisonic ACN 9
const Speaker kSpeakerACN10 = (Speaker)1 << 44;	///< Ambisonic ACN 10
const Speaker kSpeakerACN11 = (Speaker)1 << 45;	///< Ambisonic ACN 11
const Speaker kSpeakerACN12 = (Speaker)1 << 46;	///< Ambisonic ACN 12
const Speaker kSpeakerACN13 = (Speaker)1 << 47;	///< Ambisonic ACN 13
const Speaker kSpeakerACN14 = (Speaker)1 << 48;	///< Ambisonic ACN 14
const Speaker kSpeakerACN15 = (Speaker)1 << 49;	///< Ambisonic ACN 15
const Speaker kSpeakerACN16 = (Speaker)1 << 50;	///< Ambisonic ACN 16
const Speaker kSpeakerACN17 = (Speaker)1 << 51;	///< Ambisonic ACN 17
const Speaker kSpeakerACN18 = (Speaker)1 << 52;	///< Ambisonic ACN 18
const Speaker kSpeakerACN19 = (Speaker)1 << 53;	///< Ambisonic ACN 19
const Speaker kSpeakerACN20 = (Speaker)1 << 54;	///< Ambisonic ACN 20
const Speaker kSpeakerACN21 = (Speaker)1 << 55;	///< Ambisonic ACN 21
const Speaker kSpeakerACN22 = (Speaker)1 << 56;	///< Ambisonic ACN 22
const Speaker kSpeakerACN23 = (Speaker)1 << 57;	///< Ambisonic ACN 23
const Speaker kSpeakerACN24 = (Speaker)1 << 58;	///< Ambisonic ACN 24

const Speaker kSpeakerTsl = (Speaker)1 << 24;	///< Top Side Left (Tsl)
const Speaker kSpeakerTsr = (Speaker)1 << 25;	///< Top Side Right (Tsr)
const Speaker kSpeakerLcs = (Speaker)1 << 26;	///< Left of Center Surround (Lcs) - Back Left Center
const Speaker kSpeakerRcs = (Speaker)1 << 27;	///< Right of Center Surround (Rcs) - Back Right Center

const Speaker kSpeakerBfl = (Speaker)1 << 28;	///< Bottom Front Left (Bfl)
const Speaker kSpeakerBfc = (Speaker)1 << 29;	///< Bottom Front Center (Bfc)
const Speaker kSpeakerBfr = (Speaker)1 << 30;	///< Bottom Front Right (Bfr)

const Speaker kSpeakerPl  = (Speaker)1 << 31;	///< Proximity Left (Pl)
const Speaker kSpeakerPr  = (Speaker)1 << 32;	///< Proximity Right (Pr)

const Speaker kSpeakerBsl = (Speaker)1 << 33;	///< Bottom Side Left (Bsl)
const Speaker kSpeakerBsr = (Speaker)1 << 34;	///< Bottom Side Right (Bsr)
const Speaker kSpeakerBrl = (Speaker)1 << 35;	///< Bottom Rear Left (Brl)
const Speaker kSpeakerBrc = (Speaker)1 << 36;	///< Bottom Rear Center (Brc)
const Speaker kSpeakerBrr = (Speaker)1 << 37;	///< Bottom Rear Right (Brr)

const Speaker kSpeakerLw = (Speaker)1 << 59;	///< Left Wide (Lw)
const Speaker kSpeakerRw = (Speaker)1 << 60;	///< Right Wide (Rw)
//------------------------------------------------------------------------
/** @}*/

//------------------------------------------------------------------------
/** Speaker Arrangement Definitions (SpeakerArrangement) */
namespace SpeakerArr
{
//------------------------------------------------------------------------
/** Speaker Arrangement Definitions.
 * for example: 5.0.5.3 for 5x Middle + 0x LFE + 5x Top + 3x Bottom
 * \ingroup speakerArrangements
 */
/**@{*/
const SpeakerArrangement kEmpty			 = 0;          ///< empty arrangement
const SpeakerArrangement kMono			 = kSpeakerM;  ///< M
const SpeakerArrangement kStereo		 = kSpeakerL   | kSpeakerR;    ///< L R
const SpeakerArrangement kStereoWide	 = kSpeakerLw  | kSpeakerRw;   ///< Lw Rw
const SpeakerArrangement kStereoSurround = kSpeakerLs  | kSpeakerRs;   ///< Ls Rs
const SpeakerArrangement kStereoCenter	 = kSpeakerLc  | kSpeakerRc;   ///< Lc Rc
const SpeakerArrangement kStereoSide	 = kSpeakerSl  | kSpeakerSr;   ///< Sl Sr
const SpeakerArrangement kStereoCLfe	 = kSpeakerC   | kSpeakerLfe;  ///< C Lfe
const SpeakerArrangement kStereoTF		 = kSpeakerTfl | kSpeakerTfr;  ///< Tfl Tfr
const SpeakerArrangement kStereoTS		 = kSpeakerTsl | kSpeakerTsr;  ///< Tsl Tsr
const SpeakerArrangement kStereoTR		 = kSpeakerTrl | kSpeakerTrr;  ///< Trl Trr
const SpeakerArrangement kStereoBF		 = kSpeakerBfl | kSpeakerBfr;  ///< Bfl Bfr
/** L R C Lc Rc */
const SpeakerArrangement kCineFront		 = kSpeakerL   | kSpeakerR | kSpeakerC | kSpeakerLc | kSpeakerRc;

/** L R C */											// 3.0
const SpeakerArrangement k30Cine		 = kSpeakerL  | kSpeakerR | kSpeakerC;
/** L R C Lfe */										// 3.1
const SpeakerArrangement k31Cine		 = k30Cine | kSpeakerLfe;
/** L R S */
const SpeakerArrangement k30Music		 = kSpeakerL  | kSpeakerR | kSpeakerCs;
/** L R Lfe S */
const SpeakerArrangement k31Music		 = k30Music | kSpeakerLfe;
/** L R C   S */										// LCRS
const SpeakerArrangement k40Cine		 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerCs;
/** L R C   Lfe S */									// LCRS+Lfe
const SpeakerArrangement k41Cine		 = k40Cine | kSpeakerLfe;
/** L R Ls  Rs */										// 4.0 (Quadro)
const SpeakerArrangement k40Music		 = kSpeakerL  | kSpeakerR | kSpeakerLs  | kSpeakerRs;
/** L R Lfe Ls Rs */									// 4.1 (Quadro+Lfe)
const SpeakerArrangement k41Music		 = k40Music | kSpeakerLfe;
/** L R C   Ls Rs */									// 5.0 (ITU 0+5+0.0 Sound System B)
const SpeakerArrangement k50			 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLs  | kSpeakerRs;
/** L R C  Lfe Ls Rs */									// 5.1 (ITU 0+5+0.1 Sound System B)
const SpeakerArrangement k51			 = k50 | kSpeakerLfe;
/** L R C  Ls  Rs Cs */
const SpeakerArrangement k60Cine		 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLs  | kSpeakerRs | kSpeakerCs;
/** L R C  Lfe Ls Rs Cs */
const SpeakerArrangement k61Cine		 = k60Cine | kSpeakerLfe;
/** L R Ls Rs  Sl Sr */
const SpeakerArrangement k60Music		 = kSpeakerL  | kSpeakerR | kSpeakerLs  | kSpeakerRs  | kSpeakerSl | kSpeakerSr;
/** L R Lfe Ls  Rs Sl Sr */
const SpeakerArrangement k61Music		 = k60Music | kSpeakerLfe;
/** L R C   Ls  Rs Lc Rc */
const SpeakerArrangement k70Cine		 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLs  | kSpeakerRs | kSpeakerLc | kSpeakerRc;
/** L R C Lfe Ls Rs Lc Rc */
const SpeakerArrangement k71Cine		 = k70Cine | kSpeakerLfe;
const SpeakerArrangement k71CineFullFront = k71Cine;
/** L R C   Ls  Rs Sl Sr */								// (ITU 0+7+0.0 Sound System I)
const SpeakerArrangement k70Music		 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLs  | kSpeakerRs | kSpeakerSl | kSpeakerSr;
/** L R C Lfe Ls Rs Sl Sr */							// (ITU 0+7+0.1 Sound System I)
const SpeakerArrangement k71Music		 = k70Music | kSpeakerLfe;

/** L R C Lfe Ls Rs Lcs Rcs */
const SpeakerArrangement k71CineFullRear = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLfe | kSpeakerLs | kSpeakerRs | kSpeakerLcs | kSpeakerRcs;
const SpeakerArrangement k71CineSideFill = k71Music;
/** L R C Lfe Ls Rs Pl Pr */
const SpeakerArrangement k71Proximity	 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLfe | kSpeakerLs | kSpeakerRs | kSpeakerPl | kSpeakerPr;

/** L R C Ls  Rs Lc Rc Cs */
const SpeakerArrangement k80Cine		 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLs  | kSpeakerRs | kSpeakerLc | kSpeakerRc | kSpeakerCs;
/** L R C Lfe Ls Rs Lc Rc Cs */
const SpeakerArrangement k81Cine		 = k80Cine | kSpeakerLfe;
/** L R C Ls  Rs Cs Sl Sr */
const SpeakerArrangement k80Music		 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLs  | kSpeakerRs | kSpeakerCs | kSpeakerSl | kSpeakerSr;
/** L R C Lfe Ls Rs Cs Sl Sr */
const SpeakerArrangement k81Music		 = k80Music | kSpeakerLfe;
/** L R C Ls Rs Lc Rc Sl Sr */
const SpeakerArrangement k90Cine		 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLs  | kSpeakerRs | kSpeakerLc | kSpeakerRc |
                                           kSpeakerSl | kSpeakerSr;
/** L R C Lfe Ls Rs Lc Rc Sl Sr */
const SpeakerArrangement k91Cine		 = k90Cine | kSpeakerLfe;
/** L R C Ls Rs Lc Rc Cs Sl Sr */
const SpeakerArrangement k100Cine		 = kSpeakerL  | kSpeakerR | kSpeakerC   | kSpeakerLs  | kSpeakerRs | kSpeakerLc | kSpeakerRc | kSpeakerCs |
                                           kSpeakerSl | kSpeakerSr;
/** L R C Lfe Ls Rs Lc Rc Cs Sl Sr */
const SpeakerArrangement k101Cine		 = k100Cine | kSpeakerLfe;

/** First-Order with Ambisonic Channel Number (ACN) ordering and SN3D normalization (4 channels) */
const SpeakerArrangement kAmbi1stOrderACN = kSpeakerACN0 | kSpeakerACN1 | kSpeakerACN2 | kSpeakerACN3;
/** Second-Order with Ambisonic Channel Number (ACN) ordering and SN3D normalization (9 channels) */
const SpeakerArrangement kAmbi2cdOrderACN = kAmbi1stOrderACN | kSpeakerACN4 | kSpeakerACN5 | kSpeakerACN6 | kSpeakerACN7 | kSpeakerACN8;
/** Third-Order with Ambisonic Channel Number (ACN) ordering and SN3D normalization (16 channels) */
const SpeakerArrangement kAmbi3rdOrderACN = kAmbi2cdOrderACN | kSpeakerACN9 | kSpeakerACN10 | kSpeakerACN11 | kSpeakerACN12 | kSpeakerACN13 | kSpeakerACN14 | kSpeakerACN15;
/** Fourth-Order with Ambisonic Channel Number (ACN) ordering and SN3D normalization (25 channels) */
const SpeakerArrangement kAmbi4thOrderACN = kAmbi3rdOrderACN | kSpeakerACN16 | kSpeakerACN17 | kSpeakerACN18 | kSpeakerACN19 | kSpeakerACN20 | 
                                            kSpeakerACN21 | kSpeakerACN22 | kSpeakerACN23 | kSpeakerACN24;
/** Fifth-Order with Ambisonic Channel Number (ACN) ordering and SN3D normalization (36 channels) */
const SpeakerArrangement kAmbi5thOrderACN = 0x000FFFFFFFFF;
/** Sixth-Order with Ambisonic Channel Number (ACN) ordering and SN3D normalization (49 channels) */
const SpeakerArrangement kAmbi6thOrderACN = 0x0001FFFFFFFFFFFF;
/** Seventh-Order with Ambisonic Channel Number (ACN) ordering and SN3D normalization (64 channels) */
const SpeakerArrangement kAmbi7thOrderACN = 0xFFFFFFFFFFFFFFFF;

/*-----------*/
/* 3D formats */
/*-----------*/
/** L R Ls Rs Tfl Tfr Trl Trr */						// 4.0.4
const SpeakerArrangement k80Cube		   = kSpeakerL | kSpeakerR | kSpeakerLs | kSpeakerRs  | kSpeakerTfl| kSpeakerTfr| kSpeakerTrl | kSpeakerTrr;
const SpeakerArrangement k40_4			   = k80Cube;

/** L R C Lfe Ls Rs Cs Tc */							// 6.1.1
const SpeakerArrangement k71CineTopCenter  = kSpeakerL | kSpeakerR | kSpeakerC  | kSpeakerLfe | kSpeakerLs | kSpeakerRs | kSpeakerCs  | kSpeakerTc; 

/** L R C Lfe Ls Rs Cs Tfc */							// 6.1.1
const SpeakerArrangement k71CineCenterHigh = kSpeakerL | kSpeakerR | kSpeakerC  | kSpeakerLfe | kSpeakerLs | kSpeakerRs | kSpeakerCs  | kSpeakerTfc; 

/** L R C Ls Rs Tfl Tfr */								// 5.0.2 (ITU 2+5+0.0 Sound System C)
const SpeakerArrangement k70CineFrontHigh = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs | kSpeakerTfl | kSpeakerTfr;
const SpeakerArrangement k70MPEG3D		  = k70CineFrontHigh;
const SpeakerArrangement k50_2			  = k70CineFrontHigh;

/** L R C Lfe Ls Rs Tfl Tfr */							// 5.1.2 (ITU 2+5+0.1 Sound System C)
const SpeakerArrangement k71CineFrontHigh = k70CineFrontHigh | kSpeakerLfe;
const SpeakerArrangement k71MPEG3D		  = k71CineFrontHigh;
const SpeakerArrangement k51_2			  = k71CineFrontHigh;

/** L R C Ls Rs Tsl Tsr */								// 5.0.2 (Side)
const SpeakerArrangement k70CineSideHigh = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs | kSpeakerTsl | kSpeakerTsr;
const SpeakerArrangement k50_2_TS = k70CineSideHigh;

/** L R C Lfe Ls Rs Tsl Tsr */							// 5.1.2 (Side)
const SpeakerArrangement k71CineSideHigh = k70CineSideHigh | kSpeakerLfe;
const SpeakerArrangement k51_2_TS = k71CineSideHigh;

/** L R Lfe Ls Rs Tfl Tfc Tfr Bfc */					// 4.1.3.1
const SpeakerArrangement k81MPEG3D		 = kSpeakerL | kSpeakerR | kSpeakerLfe | kSpeakerLs | kSpeakerRs |
                                           kSpeakerTfl | kSpeakerTfc | kSpeakerTfr | kSpeakerBfc;
const SpeakerArrangement k41_4_1		 = k81MPEG3D;

/** L R C Ls Rs Tfl Tfr Trl Trr */						// 5.0.4 (ITU 4+5+0.0 Sound System D)
const SpeakerArrangement k90			 = kSpeakerL  | kSpeakerR | kSpeakerC | kSpeakerLs  | kSpeakerRs |
                                           kSpeakerTfl| kSpeakerTfr | kSpeakerTrl | kSpeakerTrr;
const SpeakerArrangement k50_4			 = k90;

/** L R C Lfe Ls Rs Tfl Tfr Trl Trr */					// 5.1.4 (ITU 4+5+0.1 Sound System D)
const SpeakerArrangement k91			 = k90 | kSpeakerLfe;
const SpeakerArrangement k51_4			 = k91;

/** L R C Ls Rs Tfl Tfr Trl Trr Bfc */					// 5.0.4.1 (ITU 4+5+1.0 Sound System E)
const SpeakerArrangement k50_4_1		 = k50_4 | kSpeakerBfc;

/** L R C Lfe Ls Rs Tfl Tfr Trl Trr Bfc */				// 5.1.4.1 (ITU 4+5+1.1 Sound System E)
const SpeakerArrangement k51_4_1		 = k50_4_1 | kSpeakerLfe;

/** L R C Ls Rs Sl Sr Tsl Tsr */						// 7.0.2
const SpeakerArrangement k70_2			 = kSpeakerL  | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs | 
                                           kSpeakerSl | kSpeakerSr | kSpeakerTsl | kSpeakerTsr;

/** L R C Lfe Ls Rs Sl Sr Tsl Tsr */					// 7.1.2
const SpeakerArrangement k71_2			 = k70_2 | kSpeakerLfe;
const SpeakerArrangement k91Atmos		 = k71_2;		// 9.1 Dolby Atmos (3D)

/** L R C Ls Rs Sl Sr Tfl Tfr */						// 7.0.2 (~ITU 2+7+0.0)
const SpeakerArrangement k70_2_TF		 = k70Music | kSpeakerTfl | kSpeakerTfr;

/** L R C Lfe Ls Rs Sl Sr Tfl Tfr */					// 7.1.2 (~ITU 2+7+0.1)
const SpeakerArrangement k71_2_TF		 = k70_2_TF | kSpeakerLfe;

/** L R C Ls Rs Sl Sr Tfl Tfr Trc */					// 7.0.3 (ITU 3+7+0.0 Sound System F)
const SpeakerArrangement k70_3			 = k70_2_TF | kSpeakerTrc;

/** L R C Lfe Ls Rs Sl Sr Tfl Tfr Trc Lfe2 */			// 7.2.3 (ITU 3+7+0.2 Sound System F)
const SpeakerArrangement k72_3			 = k70_3 | kSpeakerLfe | kSpeakerLfe2;

/** L R C Ls Rs Sl Sr Tfl Tfr Trl Trr */				// 7.0.4 (ITU 4+7+0.0 Sound System J)
const SpeakerArrangement k70_4			 = k70_2_TF | kSpeakerTrl | kSpeakerTrr;

/** L R C Lfe Ls Rs Sl Sr Tfl Tfr Trl Trr */			// 7.1.4 (ITU 4+7+0.1 Sound System J)
const SpeakerArrangement k71_4			 = k70_4 | kSpeakerLfe;
const SpeakerArrangement k111MPEG3D		 = k71_4;

/** L R C Ls Rs Sl Sr Tfl Tfr Trl Trr Tsl Tsr */		// 7.0.6
const SpeakerArrangement k70_6			 = kSpeakerL | kSpeakerR | kSpeakerC | 
                                           kSpeakerLs | kSpeakerRs | kSpeakerSl | kSpeakerSr | 
                                           kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr | kSpeakerTsl | kSpeakerTsr;

/** L R C Lfe Ls Rs Sl Sr Tfl Tfr Trl Trr Tsl Tsr */	// 7.1.6
const SpeakerArrangement k71_6			 = k70_6 | kSpeakerLfe;

/** L R C Ls Rs Lc Rc Sl Sr Tfl Tfr Trl Trr */			// 9.0.4 (ITU 4+9+0.0 Sound System G)
const SpeakerArrangement k90_4			 = kSpeakerL | kSpeakerR | kSpeakerC |
                                           kSpeakerLs | kSpeakerRs | kSpeakerLc | kSpeakerRc | kSpeakerSl | kSpeakerSr |
                                           kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr;

/** L R C Lfe Ls Rs Lc Rc Sl Sr Tfl Tfr Trl Trr */		// 9.1.4 (ITU 4+9+0.1 Sound System G)
const SpeakerArrangement k91_4			 = k90_4 | kSpeakerLfe;

/** L R C Lfe Ls Rs Lc Rc Sl Sr Tfl Tfr Trl Trr Tsl Tsr */ // 9.0.6
const SpeakerArrangement k90_6			 = kSpeakerL | kSpeakerR | kSpeakerC |
                                           kSpeakerLs | kSpeakerRs | kSpeakerLc | kSpeakerRc | kSpeakerSl | kSpeakerSr |
                                           kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr | kSpeakerTsl | kSpeakerTsr;

/** L R C Lfe Ls Rs Lc Rc Sl Sr Tfl Tfr Trl Trr Tsl Tsr */ // 9.1.6
const SpeakerArrangement k91_6			 = k90_6 | kSpeakerLfe;

/** L R C Ls Rs Sl Sr Tfl Tfr Trl Trr Lw Rw */			// 9.0.4 (Dolby)
const SpeakerArrangement k90_4_W		 = kSpeakerL | kSpeakerR | kSpeakerC |
                                           kSpeakerLs | kSpeakerRs | kSpeakerLw | kSpeakerRw | kSpeakerSl | kSpeakerSr |
                                           kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr;

/** L R C Lfe Ls Rs Sl Sr Tfl Tfr Trl Trr Lw Rw */		// 9.1.4 (Dolby)
const SpeakerArrangement k91_4_W		 = k90_4_W | kSpeakerLfe;

/** L R C Ls Rs Sl Sr Tfl Tfr Trl Trr Tsl Tsr Lw Rw */	// 9.0.6 (Dolby)
const SpeakerArrangement k90_6_W		 = kSpeakerL | kSpeakerR | kSpeakerC |
                                           kSpeakerLs | kSpeakerRs | kSpeakerLw | kSpeakerRw | kSpeakerSl | kSpeakerSr |
                                           kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr | kSpeakerTsl | kSpeakerTsr;

/** L R C Lfe Ls Rs Sl Sr Tfl Tfr Trl Trr Tsl Tsr Lw Rw */ // 9.1.6 (Dolby)
const SpeakerArrangement k91_6_W		 = k90_6_W | kSpeakerLfe;

/** L R C Ls Rs Tc Tfl Tfr Trl Trr */					// 5.0.5 (10.0 Auro-3D)
const SpeakerArrangement k100			 = kSpeakerL  | kSpeakerR | kSpeakerC | kSpeakerLs  | kSpeakerRs | 
                                           kSpeakerTc | kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr;
const SpeakerArrangement k50_5			 = k100;

/** L R C Lfe Ls Rs Tc Tfl Tfr Trl Trr */				// 5.1.5 (10.1 Auro-3D)
const SpeakerArrangement k101			 = k50_5 | kSpeakerLfe;
const SpeakerArrangement k101MPEG3D		 = k101;
const SpeakerArrangement k51_5			 = k101;

/** L R C Lfe Ls Rs Tfl Tfc Tfr Trl Trr Lfe2 */			// 5.2.5
const SpeakerArrangement k102			 = kSpeakerL  | kSpeakerR | kSpeakerC  | kSpeakerLfe | kSpeakerLs | kSpeakerRs  |
                                           kSpeakerTfl| kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr | kSpeakerLfe2;
const SpeakerArrangement k52_5			 = k102;

/** L R C Ls Rs Tc Tfl Tfc Tfr Trl Trr */				// 5.0.6 (11.0 Auro-3D)
const SpeakerArrangement k110			 = kSpeakerL  | kSpeakerR | kSpeakerC | kSpeakerLs  | kSpeakerRs |
                                           kSpeakerTc | kSpeakerTfl | kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr;
const SpeakerArrangement k50_6			 = k110;

/** L R C Lfe Ls Rs Tc Tfl Tfc Tfr Trl Trr */			// 5.1.6 (11.1 Auro-3D)
const SpeakerArrangement k111			 = k110 | kSpeakerLfe;
const SpeakerArrangement k51_6			 = k111;

/** L R C Lfe Ls Rs Lc Rc Tfl Tfc Tfr Trl Trr Lfe2 */	// 7.2.5
const SpeakerArrangement k122			 = kSpeakerL  | kSpeakerR | kSpeakerC  | kSpeakerLfe | kSpeakerLs | kSpeakerRs	| kSpeakerLc  | kSpeakerRc |
                                           kSpeakerTfl| kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr | kSpeakerLfe2;
const SpeakerArrangement k72_5			 = k122;

/** L R C Ls Rs Sl Sr Tc Tfl Tfc Tfr Trl Trr */			// 7.0.6 (13.0 Auro-3D)
const SpeakerArrangement k130			 = kSpeakerL  | kSpeakerR | kSpeakerC | kSpeakerLs  | kSpeakerRs | kSpeakerSl | kSpeakerSr |
                                           kSpeakerTc | kSpeakerTfl | kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr;

/** L R C Lfe Ls Rs Sl Sr Tc Tfl Tfc Tfr Trl Trr */		// 7.1.6 (13.1 Auro-3D)
const SpeakerArrangement k131			 = k130 | kSpeakerLfe;

/** L R Ls Rs Sl Sr Tfl Tfr Trl Trr Bfl Bfr Brl Brr */	// 6.0.4.4
const SpeakerArrangement k140			 = kSpeakerL  | kSpeakerR | kSpeakerLs | kSpeakerRs | kSpeakerSl | kSpeakerSr |
                                           kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr |
                                           kSpeakerBfl | kSpeakerBfr | kSpeakerBrl | kSpeakerBrr;
const SpeakerArrangement k60_4_4		 = k140;

/** L R C Ls Rs Lc Rc Cs Sl Sr Tc Tfl Tfc Tfr Trl Trc Trr Tsl Tsr Bfl Bfc Bfr */			// 10.0.9.3 (ITU 9+10+3.0 Sound System H)
const SpeakerArrangement k220			 = kSpeakerL  | kSpeakerR | kSpeakerC  | kSpeakerLs | kSpeakerRs | kSpeakerLc | kSpeakerRc | kSpeakerCs | kSpeakerSl | kSpeakerSr | 
                                           kSpeakerTc | kSpeakerTfl | kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrc | kSpeakerTrr | kSpeakerTsl | kSpeakerTsr | 
                                           kSpeakerBfl| kSpeakerBfc | kSpeakerBfr;
const SpeakerArrangement k100_9_3 = k220;

/** L R C Lfe Ls Rs Lc Rc Cs Sl Sr Tc Tfl Tfc Tfr Trl Trc Trr Lfe2 Tsl Tsr Bfl Bfc Bfr */	// 10.2.9.3 (ITU 9+10+3.2 Sound System H)
const SpeakerArrangement k222			 = kSpeakerL  | kSpeakerR | kSpeakerC  | kSpeakerLfe | kSpeakerLs | kSpeakerRs | kSpeakerLc | kSpeakerRc | kSpeakerCs | kSpeakerSl | kSpeakerSr | 
                                           kSpeakerTc | kSpeakerTfl | kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrc | kSpeakerTrr | kSpeakerLfe2 | kSpeakerTsl | kSpeakerTsr | 
                                           kSpeakerBfl| kSpeakerBfc | kSpeakerBfr;
const SpeakerArrangement k102_9_3 = k222;

/** L R C Ls Rs Tfl Tfc Tfr Trl Trr Bfl Bfc Bfr */		// 5.0.5.3
const SpeakerArrangement k50_5_3 = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs |
                                   kSpeakerTfl | kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr |
								   kSpeakerBfl | kSpeakerBfc | kSpeakerBfr;

/** L R C Lfe Ls Rs Tfl Tfc Tfr Trl Trr Bfl Bfc Bfr */	// 5.1.5.3
const SpeakerArrangement k51_5_3 = k50_5_3 | kSpeakerLfe;

/** L R C Ls Rs Tsl Tsr Bfl Bfr	*/						// 5.0.2.2
const SpeakerArrangement k50_2_2 = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs |
                                   kSpeakerTsl | kSpeakerTsr |
                                   kSpeakerBfl | kSpeakerBfr;

/** L R C Ls Rs Tfl Tfr Trl Trr Bfl Bfr */				// 5.0.4.2
const SpeakerArrangement k50_4_2 = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs | 
                                   kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr |
                                   kSpeakerBfl | kSpeakerBfr;

/** L R C Ls Rs Sl Sr Tfl Tfr Trl Trr Bfl Bfr */		// 7.0.4.2
const SpeakerArrangement k70_4_2 = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs | kSpeakerSl | kSpeakerSr |
                                   kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr |
                                   kSpeakerBfl | kSpeakerBfr;

/** L R C Ls Rs Tfl Tfc Tfr Trl Trr */					// 5.0.5.0 (Sony 360RA)
const SpeakerArrangement k50_5_Sony = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs |
                                      kSpeakerTfl | kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr;

/** C Sl Sr Cs Tsl Tsr Bsl Bsr */						// 4.0.2.2 (Sony 360RA)
const SpeakerArrangement k40_2_2 = kSpeakerC | kSpeakerSl | kSpeakerSr | kSpeakerCs |
                                   kSpeakerTsl | kSpeakerTsr |
	                               kSpeakerBsl | kSpeakerBsr;

/** L R Ls Rs Tfl Tfr Trl Trr Bfl Bfr */				// 4.0.4.2 (Sony 360RA)
const SpeakerArrangement k40_4_2 = kSpeakerL | kSpeakerR | kSpeakerLs | kSpeakerRs |
                                   kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr |
                                   kSpeakerBfl | kSpeakerBfr;

/** L R C Ls Rs Tfl Tfc Tfr Bfl Bfr */					// 5.0.3.2 (Sony 360RA)
const SpeakerArrangement k50_3_2 = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs |
                                   kSpeakerTfl | kSpeakerTfc | kSpeakerTfr |
                                   kSpeakerBfl | kSpeakerBfr;

/** L R C Tfl Tfc Tfr Trl Trr Bfl Bfr */				// 3.0.5.2 (Sony 360RA)
const SpeakerArrangement k30_5_2 = kSpeakerL | kSpeakerR | kSpeakerC |
                                   kSpeakerTfl | kSpeakerTfc | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr |
                                   kSpeakerBfl | kSpeakerBfr;

/** L R Ls Rs Tfl Tfr Trl Trr Bfl Bfr Brl Brr */		// 4.0.4.4 (Sony 360RA)
const SpeakerArrangement k40_4_4 = kSpeakerL | kSpeakerR | kSpeakerLs | kSpeakerRs |
                                   kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr |
                                   kSpeakerBfl | kSpeakerBfr | kSpeakerBrl | kSpeakerBrr;

/** L R C Ls Rs Tfl Tfr Trl Trr Bfl Bfr Brl Brr */ 		// 5.0.4.4 (Sony 360RA)
const SpeakerArrangement k50_4_4 = kSpeakerL | kSpeakerR | kSpeakerC | kSpeakerLs | kSpeakerRs |
                                   kSpeakerTfl | kSpeakerTfr | kSpeakerTrl | kSpeakerTrr |
                                   kSpeakerBfl | kSpeakerBfr | kSpeakerBrl | kSpeakerBrr;

//------------------------------------------------------------------------
/** Speaker Arrangement String Representation.
 * \ingroup speakerArrangements
 */
/**@{*/
const CString kStringEmpty		= "";
const CString kStringMono		= "Mono";
const CString kStringStereo		= "Stereo";
const CString kStringStereoWide	= "Stereo (Lw Rw)";
const CString kStringStereoR	= "Stereo (Ls Rs)";
const CString kStringStereoC	= "Stereo (Lc Rc)";
const CString kStringStereoSide	= "Stereo (Sl Sr)";
const CString kStringStereoCLfe	= "Stereo (C LFE)";
const CString kStringStereoTF	= "Stereo (Tfl Tfr)";
const CString kStringStereoTS	= "Stereo (Tsl Tsr)";
const CString kStringStereoTR	= "Stereo (Trl Trr)";
const CString kStringStereoBF	= "Stereo (Bfl Bfr)";
const CString kStringCineFront  = "Cine Front";

const CString kString30Cine		= "LRC";
const CString kString30Music	= "LRS";
const CString kString31Cine		= "LRC+LFE";
const CString kString31Music	= "LRS+LFE";
const CString kString40Cine		= "LRCS";
const CString kString40Music	= "Quadro";
const CString kString41Cine		= "LRCS+LFE";
const CString kString41Music	= "Quadro+LFE";
const CString kString50			= "5.0";
const CString kString51			= "5.1";
const CString kString60Cine		= "6.0 Cine";
const CString kString60Music	= "6.0 Music";
const CString kString61Cine		= "6.1 Cine";
const CString kString61Music	= "6.1 Music";
const CString kString70Cine		= "7.0 SDDS";
const CString kString70CineOld	= "7.0 Cine (SDDS)";
const CString kString70Music	= "7.0";
const CString kString70MusicOld = "7.0 Music (Dolby)";
const CString kString71Cine		= "7.1 SDDS";
const CString kString71CineOld	= "7.1 Cine (SDDS)";
const CString kString71Music	= "7.1";
const CString kString71MusicOld	= "7.1 Music (Dolby)";
const CString kString71CineTopCenter	= "7.1 Cine Top Center";
const CString kString71CineCenterHigh	= "7.1 Cine Center High";
const CString kString71CineFullRear		= "7.1 Cine Full Rear"; 
const CString kString51_2		= "5.1.2";
const CString kString50_2		= "5.0.2";
const CString kString50_2TopSide = "5.0.2 Top Side";
const CString kString51_2TopSide = "5.1.2 Top Side";
const CString kString71Proximity = "7.1 Proximity";
const CString kString80Cine		= "8.0 Cine";
const CString kString80Music	= "8.0 Music";
const CString kString40_4		= "8.0 Cube";
const CString kString81Cine		= "8.1 Cine";
const CString kString81Music	= "8.1 Music";
const CString kString90Cine		= "9.0 Cine";
const CString kString91Cine		= "9.1 Cine";
const CString kString100Cine	= "10.0 Cine";
const CString kString101Cine	= "10.1 Cine";
const CString kString52_5		= "5.2.5";
const CString kString72_5		= "12.2";
const CString kString50_4		= "5.0.4";
const CString kString51_4		= "5.1.4";
const CString kString50_4_1		= "5.0.4.1";
const CString kString51_4_1		= "5.1.4.1";
const CString kString70_2		= "7.0.2";
const CString kString71_2		= "7.1.2";
const CString kString70_2_TF	= "7.0.2 Top Front";
const CString kString71_2_TF	= "7.1.2 Top Front";
const CString kString70_3		= "7.0.3";
const CString kString72_3		= "7.2.3";
const CString kString70_4		= "7.0.4";
const CString kString71_4		= "7.1.4";
const CString kString70_6		= "7.0.6";
const CString kString71_6		= "7.1.6";
const CString kString90_4		= "9.0.4 ITU";
const CString kString91_4		= "9.1.4 ITU";
const CString kString90_6		= "9.0.6 ITU";
const CString kString91_6		= "9.1.6 ITU";
const CString kString90_4_W		= "9.0.4";
const CString kString91_4_W		= "9.1.4";
const CString kString90_6_W		= "9.0.6";
const CString kString91_6_W		= "9.1.6";
const CString kString50_5		= "10.0 Auro-3D";
const CString kString51_5		= "10.1 Auro-3D";
const CString kString50_6		= "11.0 Auro-3D";
const CString kString51_6		= "11.1 Auro-3D";
const CString kString130		= "13.0 Auro-3D";
const CString kString131		= "13.1 Auro-3D";
const CString kString41_4_1		= "8.1 MPEG";
const CString kString60_4_4		= "14.0";
const CString kString220		= "22.0"; 
const CString kString222		= "22.2";
const CString kString50_5_3		= "5.0.5.3";
const CString kString51_5_3		= "5.1.5.3";
const CString kString50_2_2		= "5.0.2.2";
const CString kString50_4_2		= "5.0.4.2";
const CString kString70_4_2		= "7.0.4.2";
const CString kString50_5_Sony  = "5.0.5 Sony";
const CString kString40_2_2		= "4.0.3.2";
const CString kString40_4_2		= "4.0.4.2";
const CString kString50_3_2		= "5.0.3.2";
const CString kString30_5_2		= "3.0.5.2";
const CString kString40_4_4		= "4.0.4.4";
const CString kString50_4_4		= "5.0.4.4";

const CString kStringAmbi1stOrder = "1OA";
const CString kStringAmbi2cdOrder = "2OA";
const CString kStringAmbi3rdOrder = "3OA";
const CString kStringAmbi4thOrder = "4OA";
const CString kStringAmbi5thOrder = "5OA";
const CString kStringAmbi6thOrder = "6OA";
const CString kStringAmbi7thOrder = "7OA";
/**@}*/

//------------------------------------------------------------------------
/** Speaker Arrangement String Representation with Speakers Name.
 * \ingroup speakerArrangements
 */
/**@{*/
const CString kStringMonoS		= "M";
const CString kStringStereoS	= "L R";
const CString kStringStereoWideS = "Lw Rw";
const CString kStringStereoRS	= "Ls Rs";
const CString kStringStereoCS	= "Lc Rc";
const CString kStringStereoSS	= "Sl Sr";
const CString kStringStereoCLfeS= "C LFE";
const CString kStringStereoTFS	= "Tfl Tfr";
const CString kStringStereoTSS	= "Tsl Tsr";
const CString kStringStereoTRS	= "Trl Trr";
const CString kStringStereoBFS	= "Bfl Bfr";
const CString kStringCineFrontS = "L R C Lc Rc";
const CString kString30CineS	= "L R C";
const CString kString30MusicS	= "L R S";
const CString kString31CineS	= "L R C LFE";
const CString kString31MusicS	= "L R LFE S";
const CString kString40CineS	= "L R C S";
const CString kString40MusicS	= "L R Ls Rs";
const CString kString41CineS	= "L R C LFE S";
const CString kString41MusicS	= "L R LFE Ls Rs";
const CString kString50S		= "L R C Ls Rs";
const CString kString51S		= "L R C LFE Ls Rs";
const CString kString60CineS	= "L R C Ls Rs Cs";
const CString kString60MusicS	= "L R Ls Rs Sl Sr";
const CString kString61CineS	= "L R C LFE Ls Rs Cs";
const CString kString61MusicS	= "L R LFE Ls Rs Sl Sr";
const CString kString70CineS	= "L R C Ls Rs Lc Rc";
const CString kString70MusicS	= "L R C Ls Rs Sl Sr";
const CString kString71CineS	= "L R C LFE Ls Rs Lc Rc";
const CString kString71MusicS	= "L R C LFE Ls Rs Sl Sr";
const CString kString80CineS	= "L R C Ls Rs Lc Rc Cs";
const CString kString80MusicS	= "L R C Ls Rs Cs Sl Sr";
const CString kString81CineS	= "L R C LFE Ls Rs Lc Rc Cs";
const CString kString81MusicS	= "L R C LFE Ls Rs Cs Sl Sr";
const CString kString40_4S		= "L R Ls Rs Tfl Tfr Trl Trr";
const CString kString71CineTopCenterS	= "L R C LFE Ls Rs Cs Tc";
const CString kString71CineCenterHighS	= "L R C LFE Ls Rs Cs Tfc";
const CString kString71CineFullRearS    = "L R C LFE Ls Rs Lcs Rcs";
const CString kString50_2S		= "L R C Ls Rs Tfl Tfr";
const CString kString51_2S		= "L R C LFE Ls Rs Tfl Tfr";
const CString kString50_2TopSideS	= "L R C Ls Rs Tsl Tsr";
const CString kString51_2TopSideS	= "L R C LFE Ls Rs Tsl Tsr";
const CString kString71ProximityS	= "L R C LFE Ls Rs Pl Pr";
const CString kString90CineS	= "L R C Ls Rs Lc Rc Sl Sr";
const CString kString91CineS	= "L R C LFE Ls Rs Lc Rc Sl Sr";
const CString kString100CineS	= "L R C Ls Rs Lc Rc Cs Sl Sr";
const CString kString101CineS	= "L R C LFE Ls Rs Lc Rc Cs Sl Sr";
const CString kString50_4S		= "L R C Ls Rs Tfl Tfr Trl Trr";
const CString kString51_4S		= "L R C LFE Ls Rs Tfl Tfr Trl Trr";
const CString kString50_4_1S	= "L R C Ls Rs Tfl Tfr Trl Trr Bfc";
const CString kString51_4_1S	= "L R C LFE Ls Rs Tfl Tfr Trl Trr Bfc";
const CString kString70_2S		= "L R C Ls Rs Sl Sr Tsl Tsr"; 
const CString kString71_2S		= "L R C LFE Ls Rs Sl Sr Tsl Tsr";
const CString kString70_2_TFS	= "L R C Ls Rs Sl Sr Tfl Tfr";
const CString kString71_2_TFS	= "L R C LFE Ls Rs Sl Sr Tfl Tfr";
const CString kString70_3S		= "L R C Ls Rs Sl Sr Tfl Tfr Trc"; 
const CString kString72_3S		= "L R C LFE Ls Rs Sl Sr Tfl Tfr Trc LFE2";
const CString kString70_4S		= "L R C Ls Rs Sl Sr Tfl Tfr Trl Trr";
const CString kString71_4S		= "L R C LFE Ls Rs Sl Sr Tfl Tfr Trl Trr";
const CString kString70_6S		= "L R C Ls Rs Sl Sr Tfl Tfr Trl Trr Tsl Tsr";
const CString kString71_6S		= "L R C LFE Ls Rs Sl Sr Tfl Tfr Trl Trr Tsl Tsr";
const CString kString90_4S		= "L R C Ls Rs Lc Rc Sl Sr Tfl Tfr Trl Trr"; 
const CString kString91_4S		= "L R C LFE Ls Rs Lc Rc Sl Sr Tfl Tfr Trl Trr";
const CString kString90_6S		= "L R C Ls Rs Lc Rc Sl Sr Tfl Tfr Trl Trr Tsl Tsr";
const CString kString91_6S		= "L R C LFE Ls Rs Lc Rc Sl Sr Tfl Tfr Trl Trr Tsl Tsr";
const CString kString90_4_WS	= "L R C Ls Rs Sl Sr Tfl Tfr Trl Trr Lw Rw";
const CString kString91_4_WS	= "L R C LFE Ls Rs Sl Sr Tfl Tfr Trl Trr Lw Rw";
const CString kString90_6_WS	= "L R C Ls Rs Sl Sr Tfl Tfr Trl Trr Tsl Tsr Lw Rw";
const CString kString91_6_WS	= "L R C LFE Ls Rs Sl Sr Tfl Tfr Trl Trr Tsl Tsr Lw Rw";
const CString kString50_5S		= "L R C Ls Rs Tc Tfl Tfr Trl Trr";
const CString kString51_5S		= "L R C LFE Ls Rs Tc Tfl Tfr Trl Trr";
const CString kString50_5_SonyS = "L R C Ls Rs Tfl Tfc Tfr Trl Trr";
const CString kString50_6S		= "L R C Ls Rs Tc Tfl Tfc Tfr Trl Trr";
const CString kString51_6S		= "L R C LFE Ls Rs Tc Tfl Tfc Tfr Trl Trr";
const CString kString130S		= "L R C Ls Rs Sl Sr Tc Tfl Tfc Tfr Trl Trr";
const CString kString131S		= "L R C LFE Ls Rs Sl Sr Tc Tfl Tfc Tfr Trl Trr";
const CString kString52_5S		= "L R C LFE Ls Rs Tfl Tfc Tfr Trl Trr LFE2";
const CString kString72_5S		= "L R C LFE Ls Rs Lc Rc Tfl Tfc Tfr Trl Trr LFE2";
const CString kString41_4_1S	= "L R LFE Ls Rs Tfl Tfc Tfr Bfc";
const CString kString30_5_2S	= "L R C Tfl Tfc Tfr Trl Trr Bfl Bfr";
const CString kString40_2_2S	= "C Sl Sr Cs Tfc Tsl Tsr Trc";
const CString kString40_4_2S	= "L R Ls Rs Tfl Tfr Trl Trr Bfl Bfr";
const CString kString40_4_4S	= "L R Ls Rs Tfl Tfr Trl Trr Bfl Bfr Brl Brr";
const CString kString50_4_4S	= "L R C Ls Rs Tfl Tfr Trl Trr Bfl Bfr Brl Brr";
const CString kString60_4_4S	= "L R Ls Rs Sl Sr Tfl Tfr Trl Trr Bfl Bfr Brl Brr";
const CString kString50_5_3S	= "L R C Ls Rs Tfl Tfc Tfr Trl Trr Bfl Bfc Bfr";
const CString kString51_5_3S	= "L R C LFE Ls Rs Tfl Tfc Tfr Trl Trr Bfl Bfc Bfr";
const CString kString50_2_2S	= "L R C Ls Rs Tsl Tsr Bfl Bfr";
const CString kString50_3_2S	= "L R C Ls Rs Tfl Tfc Tfr Bfl Bfr";
const CString kString50_4_2S	= "L R C Ls Rs Tfl Tfr Trl Trr Bfl Bfr";
const CString kString70_4_2S	= "L R C Ls Rs Sl Sr Tfl Tfr Trl Trr Bfl Bfr";
const CString kString222S = "L R C LFE Ls Rs Lc Rc Cs Sl Sr Tc Tfl Tfc Tfr Trl Trc Trr LFE2 Tsl Tsr Bfl Bfc Bfr";
const CString kString220S = "L R C Ls Rs Lc Rc Cs Sl Sr Tc Tfl Tfc Tfr Trl Trc Trr Tsl Tsr Bfl Bfc Bfr";

const CString kStringAmbi1stOrderS = "0 1 2 3";
const CString kStringAmbi2cdOrderS = "0 1 2 3 4 5 6 7 8";
const CString kStringAmbi3rdOrderS = "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15";
const CString kStringAmbi4thOrderS = "0..24";
const CString kStringAmbi5thOrderS = "0..35";
const CString kStringAmbi6thOrderS = "0..48";
const CString kStringAmbi7thOrderS = "0..63";
/**@}*/

//------------------------------------------------------------------------
/** Returns number of channels used in speaker arrangement.
 * \ingroup speakerArrangements
 */
/**@{*/
inline int32 getChannelCount (SpeakerArrangement arr)
{
	int32 count = 0;
	while (arr)
	{
		if (arr & (SpeakerArrangement)1)
			++count;
		arr >>= 1;
	}
	return count;
}

//------------------------------------------------------------------------
/** Returns the index of a given speaker in a speaker arrangement (-1 if speaker is not part of the
 * arrangement).
 * \ingroup speakerArrangements
 */
inline int32 getSpeakerIndex (Speaker speaker, SpeakerArrangement arrangement)
{
	// check if speaker is present in arrangement
	if ((arrangement & speaker) == 0)
		return -1;

	int32 result = 0;
	Speaker i = 1;
	while (i < speaker)
	{
		if (arrangement & i)
			result++;
		i <<= 1;
	}

	return result;
}

//------------------------------------------------------------------------
/** Returns the speaker for a given index in a speaker arrangement
 * Return 0 when out of range.
 * \ingroup speakerArrangements
 */
inline Speaker getSpeaker (const SpeakerArrangement& arr, int32 index)
{
	SpeakerArrangement arrTmp = arr;

	int32 index2 = -1;
	int32 pos = -1;
	while (arrTmp)
	{
		if (arrTmp & 0x1)
			index2++;
		pos++;
		if (index2 == index)
			return (Speaker)1 << pos;

		arrTmp = arrTmp >> 1;
	}
	return 0;
}

//------------------------------------------------------------------------
/** Returns true if arrSubSet is a subset speaker of arr (means each speaker of arrSubSet is
 * included in arr).
 * \ingroup speakerArrangements
 */
inline bool isSubsetOf (const SpeakerArrangement& arrSubSet, const SpeakerArrangement& arr)
{
	return (arrSubSet == (arrSubSet & arr));
}

//------------------------------------------------------------------------
/** Returns true if arrangement is a Auro configuration.
 * \ingroup speakerArrangements
 */
inline bool isAuro (const SpeakerArrangement& arr)
{
	if (arr == k90 || arr == k91 || arr == k100 || arr == k101 || arr == k110 || arr == k111 ||
	    arr == k130 || arr == k131)
	{
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
/** Returns true if arrangement contains top (upper layer) speakers
 * \ingroup speakerArrangements
 */
inline bool hasTopSpeakers (const SpeakerArrangement& arr)
{
	if (arr & kSpeakerTc || arr & kSpeakerTfl || arr & kSpeakerTfc || arr & kSpeakerTfr ||
	    arr & kSpeakerTrl || arr & kSpeakerTrc || arr & kSpeakerTrr || arr & kSpeakerTsl ||
	    arr & kSpeakerTsr)
		return true;
	return false;
}

//------------------------------------------------------------------------
/** Returns true if arrangement contains bottom (lower layer) speakers
 * \ingroup speakerArrangements
 */
inline bool hasBottomSpeakers (const SpeakerArrangement& arr)
{
	if (arr & kSpeakerBfl || arr & kSpeakerBfc || arr & kSpeakerBfr || arr & kSpeakerBsl ||
	    arr & kSpeakerBsr || arr & kSpeakerBrr || arr & kSpeakerBrl || arr & kSpeakerBrc)
		return true;
	return false;
}

//------------------------------------------------------------------------
/** Returns true if arrangement contains middle layer (at ears level) speakers
 * \ingroup speakerArrangements
 */
inline bool hasMiddleSpeakers (const SpeakerArrangement& arr)
{
	if (arr & kSpeakerL || arr & kSpeakerR || arr & kSpeakerC || arr & kSpeakerLs ||
	    arr & kSpeakerRs || arr & kSpeakerLc || arr & kSpeakerRc || arr & kSpeakerCs ||
	    arr & kSpeakerSl || arr & kSpeakerSr || arr & kSpeakerM || arr & kSpeakerPl ||
	    arr & kSpeakerPr || arr & kSpeakerLcs || arr & kSpeakerRcs || arr & kSpeakerLw ||
	    arr & kSpeakerRw)
		return true;
	return false;
}

//------------------------------------------------------------------------
/** Returns true if arrangement contains LFE speakers
 * \ingroup speakerArrangements
 */
inline bool hasLfe (const SpeakerArrangement& arr)
{
	if (arr & kSpeakerLfe || arr & kSpeakerLfe2)
		return true;
	return false;
}

//------------------------------------------------------------------------
/** Returns true if arrangement is a 3D configuration ((top or bottom) and middle)
 * \ingroup speakerArrangements
 */
inline bool is3D (const SpeakerArrangement& arr)
{
	bool top = hasTopSpeakers (arr);
	bool bottom = hasBottomSpeakers (arr);
	bool middle = hasMiddleSpeakers (arr);

	if (((top || bottom) && middle) || (top && bottom))
		return true;
	return false;
}

//------------------------------------------------------------------------
/** Returns true if arrangement is a Ambisonic configuration.
 * \ingroup speakerArrangements
 */
inline bool isAmbisonics (const SpeakerArrangement& arr)
{
	if (arr == kAmbi1stOrderACN || arr == kAmbi2cdOrderACN || arr == kAmbi3rdOrderACN ||
	    arr == kAmbi4thOrderACN || arr == kAmbi5thOrderACN || arr == kAmbi6thOrderACN ||
	    arr == kAmbi7thOrderACN)
	{
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
/** Converts a speaker of a Ambisonic order 1 to 4 to a Ambisonic order 7 (5 to 7)
 * Return 0 when out of range.
 * \ingroup speakerArrangements
 */
inline Speaker convertSpeaker_Ambi_1234Order_to_Ambi567Order (Speaker speaker_1234_order)
{
	int32 idx = getSpeakerIndex (speaker_1234_order, kAmbi4thOrderACN);
	if (idx < 0)
		return 0;
	return (Speaker)1 << idx;
}

//------------------------------------------------------------------------
/** Converts a speaker of a Ambisonic order 5 to 7 to a Ambisonic order 4 (1 to 4).
 * Return 0 when out of range.
 * \ingroup speakerArrangements
 */
inline Speaker convertSpeaker_Ambi_567Order_to_Ambi1234Order (Speaker speaker_567_order)
{
	int32 idx = getSpeakerIndex (speaker_567_order, kAmbi7thOrderACN);
	if (idx < 0)
		return 0;
	return getSpeaker (kAmbi4thOrderACN, idx);
}

//------------------------------------------------------------------------
/** Returns the speaker arrangement associated to a string representation.
 *  Returns kEmpty if no associated arrangement is known.
 * \ingroup speakerArrangements
 */
inline SpeakerArrangement getSpeakerArrangementFromString (CString arrStr)
{
	if (!strcmp8 (arrStr, kStringMono))
		return kMono;
	if (!strcmp8 (arrStr, kStringStereo))
		return kStereo;
	if (!strcmp8 (arrStr, kStringStereoR))
		return kStereoSurround;
	if (!strcmp8 (arrStr, kStringStereoWide))
		return kStereoWide;
	if (!strcmp8 (arrStr, kStringStereoC))
		return kStereoCenter;
	if (!strcmp8 (arrStr, kStringStereoSide))
		return kStereoSide;
	if (!strcmp8 (arrStr, kStringStereoCLfe))
		return kStereoCLfe;
	if (!strcmp8 (arrStr, kStringStereoTF))
		return kStereoTF;
	if (!strcmp8 (arrStr, kStringStereoTS))
		return kStereoTS;
	if (!strcmp8 (arrStr, kStringStereoTR))
		return kStereoTR;
	if (!strcmp8 (arrStr, kStringStereoBF))
		return kStereoBF;
	if (!strcmp8 (arrStr, kStringCineFront))
		return kCineFront;
	if (!strcmp8 (arrStr, kString30Cine))
		return k30Cine;
	if (!strcmp8 (arrStr, kString30Music))
		return k30Music;
	if (!strcmp8 (arrStr, kString31Cine))
		return k31Cine;
	if (!strcmp8 (arrStr, kString31Music))
		return k31Music;
	if (!strcmp8 (arrStr, kString40Cine))
		return k40Cine;
	if (!strcmp8 (arrStr, kString40Music))
		return k40Music;
	if (!strcmp8 (arrStr, kString41Cine))
		return k41Cine;
	if (!strcmp8 (arrStr, kString41Music))
		return k41Music;
	if (!strcmp8 (arrStr, kString50))
		return k50;
	if (!strcmp8 (arrStr, kString51))
		return k51;
	if (!strcmp8 (arrStr, kString60Cine))
		return k60Cine;
	if (!strcmp8 (arrStr, kString60Music))
		return k60Music;
	if (!strcmp8 (arrStr, kString61Cine))
		return k61Cine;
	if (!strcmp8 (arrStr, kString61Music))
		return k61Music;
	if (!strcmp8 (arrStr, kString70Cine) || !strcmp8 (arrStr, kString70CineOld))
		return k70Cine;
	if (!strcmp8 (arrStr, kString70Music) || !strcmp8 (arrStr, kString70MusicOld))
		return k70Music;
	if (!strcmp8 (arrStr, kString71Cine) || !strcmp8 (arrStr, kString71CineOld))
		return k71Cine;
	if (!strcmp8 (arrStr, kString71Music) || !strcmp8 (arrStr, kString71MusicOld))
		return k71Music;
	if (!strcmp8 (arrStr, kString71Proximity))
		return k71Proximity;
	if (!strcmp8 (arrStr, kString80Cine))
		return k80Cine;
	if (!strcmp8 (arrStr, kString80Music))
		return k80Music;
	if (!strcmp8 (arrStr, kString81Cine))
		return k81Cine;
	if (!strcmp8 (arrStr, kString81Music))
		return k81Music;
	if (!strcmp8 (arrStr, kString52_5))
		return k52_5;
	if (!strcmp8 (arrStr, kString72_5))
		return k72_5;
	if (!strcmp8 (arrStr, kString40_4))
		return k40_4;
	if (!strcmp8 (arrStr, kString71CineTopCenter))
		return k71CineTopCenter;
	if (!strcmp8 (arrStr, kString71CineCenterHigh))
		return k71CineCenterHigh;
	if (!strcmp8 (arrStr, kString50_2))
		return k50_2;
	if (!strcmp8 (arrStr, kString51_2))
		return k51_2;
	if (!strcmp8 (arrStr, kString50_2TopSide))
		return k50_2_TS;
	if (!strcmp8 (arrStr, kString51_2TopSide))
		return k51_2_TS;
	if (!strcmp8 (arrStr, kString71CineFullRear))
		return k71CineFullRear;
	if (!strcmp8 (arrStr, kString90Cine))
		return k90Cine;
	if (!strcmp8 (arrStr, kString91Cine))
		return k91Cine;
	if (!strcmp8 (arrStr, kString100Cine))
		return k100Cine;
	if (!strcmp8 (arrStr, kString101Cine))
		return k101Cine;
	if (!strcmp8 (arrStr, kString50_4))
		return k50_4;
	if (!strcmp8 (arrStr, kString51_4))
		return k51_4;
	if (!strcmp8 (arrStr, kString50_4_1))
		return k50_4_1;
	if (!strcmp8 (arrStr, kString51_4_1))
		return k51_4_1;
	if (!strcmp8 (arrStr, kString41_4_1))
		return k41_4_1;
	if (!strcmp8 (arrStr, kString70_2))
		return k70_2;
	if (!strcmp8 (arrStr, kString71_2))
		return k71_2;
	if (!strcmp8 (arrStr, kString70_2_TF))
		return k70_2_TF;
	if (!strcmp8 (arrStr, kString71_2_TF))
		return k71_2_TF;
	if (!strcmp8 (arrStr, kString70_3))
		return k70_3;
	if (!strcmp8 (arrStr, kString72_3))
		return k72_3;
	if (!strcmp8 (arrStr, kString70_4))
		return k70_4;
	if (!strcmp8 (arrStr, kString71_4))
		return k71_4;
	if (!strcmp8 (arrStr, kString70_6))
		return k70_6;
	if (!strcmp8 (arrStr, kString71_6))
		return k71_6;
	if (!strcmp8 (arrStr, kString90_4))
		return k90_4;
	if (!strcmp8 (arrStr, kString91_4))
		return k91_4;
	if (!strcmp8 (arrStr, kString90_6))
		return k90_6;
	if (!strcmp8 (arrStr, kString91_6))
		return k91_6;
	if (!strcmp8 (arrStr, kString90_4_W))
		return k90_4_W;
	if (!strcmp8 (arrStr, kString91_4_W))
		return k91_4_W;
	if (!strcmp8 (arrStr, kString90_6_W))
		return k90_6_W;
	if (!strcmp8 (arrStr, kString91_6_W))
		return k91_6_W;
	if (!strcmp8 (arrStr, kString50_5))
		return k50_5;
	if (!strcmp8 (arrStr, kString51_5))
		return k51_5;
	if (!strcmp8 (arrStr, kString50_6))
		return k50_6;
	if (!strcmp8 (arrStr, kString51_6))
		return k51_6;
	if (!strcmp8 (arrStr, kString130))
		return k130;
	if (!strcmp8 (arrStr, kString131))
		return k131;
	if (!strcmp8 (arrStr, kString60_4_4))
		return k60_4_4;
	if (!strcmp8 (arrStr, kString222))
		return k222;
	if (!strcmp8 (arrStr, kString220))
		return k220;
	if (!strcmp8 (arrStr, kString50_5_3))
		return k50_5_3;
	if (!strcmp8 (arrStr, kString51_5_3))
		return k51_5_3;
	if (!strcmp8 (arrStr, kString50_2_2))
		return k50_2_2;
	if (!strcmp8 (arrStr, kString50_4_2))
		return k50_4_2;
	if (!strcmp8 (arrStr, kString70_4_2))
		return k70_4_2;

	if (!strcmp8 (arrStr, kString50_5_Sony))
		return k50_5_Sony;
	if (!strcmp8 (arrStr, kString40_2_2))
		return k40_2_2;
	if (!strcmp8 (arrStr, kString40_4_2))
		return k40_4_2;
	if (!strcmp8 (arrStr, kString50_3_2))
		return k50_3_2;
	if (!strcmp8 (arrStr, kString30_5_2))
		return k30_5_2;
	if (!strcmp8 (arrStr, kString40_4_4))
		return k40_4_4;
	if (!strcmp8 (arrStr, kString50_4_4))
		return k50_4_4;

	if (!strcmp8 (arrStr, kStringAmbi1stOrder))
		return kAmbi1stOrderACN;
	if (!strcmp8 (arrStr, kStringAmbi2cdOrder))
		return kAmbi2cdOrderACN;
	if (!strcmp8 (arrStr, kStringAmbi3rdOrder))
		return kAmbi3rdOrderACN;
	if (!strcmp8 (arrStr, kStringAmbi4thOrder))
		return kAmbi4thOrderACN;
	if (!strcmp8 (arrStr, kStringAmbi5thOrder))
		return kAmbi5thOrderACN;
	if (!strcmp8 (arrStr, kStringAmbi6thOrder))
		return kAmbi6thOrderACN;
	if (!strcmp8 (arrStr, kStringAmbi7thOrder))
		return kAmbi7thOrderACN;
	return kEmpty;
}

//------------------------------------------------------------------------
/** Returns the string representation of a given speaker arrangement.
 *  Returns kStringEmpty if arr is unknown. 
 * \ingroup speakerArrangements
 */
inline CString getSpeakerArrangementString (SpeakerArrangement arr, bool withSpeakersName)
{
	switch (arr)
	{
		case kMono:				return withSpeakersName ? kStringMonoS		: kStringMono;
		//--- Stereo pairs---
		case kStereo:			return withSpeakersName ? kStringStereoS	: kStringStereo;
		case kStereoSurround:	return withSpeakersName ? kStringStereoRS	: kStringStereoR;
		case kStereoWide:		return withSpeakersName ? kStringStereoWideS : kStringStereoWide;
		case kStereoCenter:		return withSpeakersName ? kStringStereoCS	: kStringStereoC;
		case kStereoSide:		return withSpeakersName ? kStringStereoSS	: kStringStereoSide;
		case kStereoCLfe:		return withSpeakersName ? kStringStereoCLfeS: kStringStereoCLfe;
		case kStereoTF:			return withSpeakersName ? kStringStereoTFS	: kStringStereoTF;
		case kStereoTS:			return withSpeakersName ? kStringStereoTSS	: kStringStereoTS;
		case kStereoTR:			return withSpeakersName ? kStringStereoTRS	: kStringStereoTR;
		case kStereoBF:			return withSpeakersName ? kStringStereoBFS	: kStringStereoBF;
		
		//--- ---
		case kCineFront:		return withSpeakersName ? kStringCineFrontS : kStringCineFront;
		case k30Cine:			return withSpeakersName ? kString30CineS	: kString30Cine;
		case k31Cine:			return withSpeakersName ? kString31CineS	: kString31Cine;
		case k30Music:			return withSpeakersName ? kString30MusicS	: kString30Music;
		case k31Music:			return withSpeakersName ? kString31MusicS	: kString31Music;
		case k40Cine:			return withSpeakersName ? kString40CineS	: kString40Cine;
		case k41Cine:			return withSpeakersName ? kString41CineS	: kString41Cine;
		case k40Music:			return withSpeakersName ? kString40MusicS	: kString40Music;
		case k41Music:			return withSpeakersName ? kString41MusicS	: kString41Music;
		case k50:				return withSpeakersName ? kString50S		: kString50;
		case k51:				return withSpeakersName ? kString51S		: kString51;
		case k60Cine:			return withSpeakersName ? kString60CineS	: kString60Cine;
		case k61Cine:			return withSpeakersName ? kString61CineS	: kString61Cine;
		case k60Music:			return withSpeakersName ? kString60MusicS	: kString60Music;
		case k61Music:			return withSpeakersName ? kString61MusicS	: kString61Music;
		case k70Cine:			return withSpeakersName ? kString70CineS	: kString70Cine;
		case k71Cine:			return withSpeakersName ? kString71CineS	: kString71Cine;
		case k70Music:			return withSpeakersName ? kString70MusicS	: kString70Music;
		case k71Music:			return withSpeakersName ? kString71MusicS	: kString71Music;
		case k71Proximity:		return withSpeakersName ? kString71ProximityS : kString71Proximity;
		case k80Cine:			return withSpeakersName ? kString80CineS	: kString80Cine;
		case k81Cine:			return withSpeakersName ? kString81CineS	: kString81Cine;
		case k80Music:			return withSpeakersName ? kString80MusicS	: kString80Music;
		case k81Music:			return withSpeakersName ? kString81MusicS	: kString81Music;
		case k71CineFullRear:	return withSpeakersName ? kString71CineFullRearS : kString71CineFullRear;
		case k90Cine:			return withSpeakersName ? kString90CineS	: kString90Cine;
		case k91Cine:			return withSpeakersName ? kString91CineS	: kString91Cine;
		case k100Cine:			return withSpeakersName ? kString100CineS	: kString100Cine;
		case k101Cine:			return withSpeakersName ? kString101CineS	: kString101Cine;

		//---With Tops ---
		case k71CineTopCenter:	return withSpeakersName ? kString71CineTopCenterS	: kString71CineTopCenter;
		case k71CineCenterHigh:	return withSpeakersName ? kString71CineCenterHighS	: kString71CineCenterHigh;
		case k50_2_TS:			return withSpeakersName ? kString50_2TopSideS		: kString50_2TopSide;
		case k51_2_TS:			return withSpeakersName ? kString51_2TopSideS		: kString51_2TopSide;
				
		case k40_4:				return withSpeakersName ? kString40_4S		: kString40_4;
		case k50_2:				return withSpeakersName ? kString50_2S		: kString50_2;
		case k51_2:				return withSpeakersName ? kString51_2S		: kString51_2;
		case k50_4:				return withSpeakersName ? kString50_4S		: kString50_4;
		case k51_4:				return withSpeakersName ? kString51_4S		: kString51_4;
		case k50_5:				return withSpeakersName ? kString50_5S		: kString50_5;
		case k51_5:				return withSpeakersName ? kString51_5S		: kString51_5; 
		case k52_5:				return withSpeakersName ? kString52_5S		: kString52_5;
		case k50_6:				return withSpeakersName ? kString50_6S		: kString50_6;
		case k51_6:				return withSpeakersName ? kString51_6S		: kString51_6;
		case k70_2:				return withSpeakersName ? kString70_2S		: kString70_2;
		case k71_2:				return withSpeakersName ? kString71_2S		: kString71_2;
		case k70_2_TF:			return withSpeakersName ? kString70_2_TFS	: kString70_2_TF;
		case k71_2_TF:			return withSpeakersName ? kString71_2_TFS	: kString71_2_TF;
		case k70_3:				return withSpeakersName ? kString70_3S		: kString70_3;
		case k72_3:				return withSpeakersName ? kString72_3S		: kString72_3;
		case k70_4:				return withSpeakersName ? kString70_4S		: kString70_4;
		case k71_4:				return withSpeakersName ? kString71_4S		: kString71_4;
		case k72_5:				return withSpeakersName ? kString72_5S		: kString72_5;
		case k70_6:				return withSpeakersName ? kString70_6S		: kString70_6;
		case k71_6:				return withSpeakersName ? kString71_6S		: kString71_6;
		case k90_4:				return withSpeakersName ? kString90_4S		: kString90_4;
		case k91_4:				return withSpeakersName ? kString91_4S		: kString91_4;
		case k90_6:				return withSpeakersName ? kString90_6S		: kString90_6;
		case k91_6:				return withSpeakersName ? kString91_6S		: kString91_6;
		case k90_4_W:			return withSpeakersName ? kString90_4_WS	: kString90_4_W;
		case k91_4_W:			return withSpeakersName ? kString91_4_WS	: kString91_4_W;
		case k90_6_W:			return withSpeakersName ? kString90_6_WS	: kString90_6_W;
		case k91_6_W:			return withSpeakersName ? kString91_6_WS	: kString91_6_W;
		case k130:				return withSpeakersName ? kString130S		: kString130;
		case k131:				return withSpeakersName ? kString131S		: kString131;
		
		//--- With Tops and Bottoms ---
		case k41_4_1:			return withSpeakersName ? kString41_4_1S	: kString41_4_1;
		case k50_4_1:			return withSpeakersName ? kString50_4_1S	: kString50_4_1;
		case k51_4_1:			return withSpeakersName ? kString51_4_1S	: kString51_4_1;
		case k50_5_3:			return withSpeakersName ? kString50_5_3S	: kString50_5_3;
		case k51_5_3:			return withSpeakersName ? kString51_5_3S	: kString51_5_3;
		case k50_2_2:			return withSpeakersName ? kString50_2_2S	: kString50_2_2;
		case k50_4_2:			return withSpeakersName ? kString50_4_2S	: kString50_4_2;
		case k60_4_4:			return withSpeakersName ? kString60_4_4S	: kString60_4_4;
		case k70_4_2:			return withSpeakersName ? kString70_4_2S	: kString70_4_2;

		case k50_5_Sony:		return withSpeakersName ? kString50_5_SonyS : kString50_5_Sony;
		case k40_2_2:			return withSpeakersName ? kString40_2_2S	: kString40_2_2;
		case k40_4_2:			return withSpeakersName ? kString40_4_2S	: kString40_4_2;
		case k50_3_2:			return withSpeakersName ? kString50_3_2S	: kString50_3_2;
		case k30_5_2:			return withSpeakersName ? kString30_5_2S	: kString30_5_2;
		case k40_4_4:			return withSpeakersName ? kString40_4_4S	: kString40_4_4;
		case k50_4_4:			return withSpeakersName ? kString50_4_4S	: kString50_4_4;

		case k220:				return withSpeakersName ? kString220S		: kString220;
		case k222:				return withSpeakersName ? kString222S		: kString222;
	}
	//--- Ambisonics ---
	if (arr == kAmbi1stOrderACN)
		return withSpeakersName ? kStringAmbi1stOrderS : kStringAmbi1stOrder;
	if (arr == kAmbi2cdOrderACN)
		return withSpeakersName ? kStringAmbi2cdOrderS : kStringAmbi2cdOrder;
	if (arr == kAmbi3rdOrderACN)
		return withSpeakersName ? kStringAmbi3rdOrderS : kStringAmbi3rdOrder;
	if (arr == kAmbi4thOrderACN)
		return withSpeakersName ? kStringAmbi4thOrderS : kStringAmbi4thOrder;
	if (arr == kAmbi5thOrderACN)
		return withSpeakersName ? kStringAmbi5thOrderS : kStringAmbi5thOrder;
	if (arr == kAmbi6thOrderACN)
		return withSpeakersName ? kStringAmbi6thOrderS : kStringAmbi6thOrder;
	if (arr == kAmbi7thOrderACN)
		return withSpeakersName ? kStringAmbi7thOrderS : kStringAmbi7thOrder;
	return kStringEmpty;
}

//------------------------------------------------------------------------
/** Returns a CString representation of a given speaker in a given arrangement.
 * \ingroup speakerArrangements
 */
inline CString getSpeakerShortName (const SpeakerArrangement& arr, int32 index)
{
	SpeakerArrangement arrTmp = arr;

	bool found = false;
	int32 index2 = -1;
	int32 pos = -1;
	while (arrTmp)
	{
		if (arrTmp & 0x1)
			index2++;
		pos++;
		if (index2 == index)
		{
			found = true;
			break;
		}
		arrTmp = arrTmp >> 1;
	}

	if (!found)
		return "";

	Speaker speaker = (Speaker)1 << pos;
	if (speaker == kSpeakerL)
		return "L";
	if (speaker == kSpeakerR)
		return "R";
	if (speaker == kSpeakerC)
		return "C";
	if (speaker == kSpeakerLfe)
		return "LFE";
	if (speaker == kSpeakerLs)
		return "Ls";
	if (speaker == kSpeakerRs)
		return "Rs";
	if (speaker == kSpeakerLc)
		return "Lc";
	if (speaker == kSpeakerRc)
		return "Rc";
	if (speaker == kSpeakerCs)
		return "S";
	if (speaker == kSpeakerSl)
		return "Sl";
	if (speaker == kSpeakerSr)
		return "Sr";
	if (speaker == kSpeakerTc)
		return "Tc";
	if (speaker == kSpeakerTfl)
		return "Tfl";
	if (speaker == kSpeakerTfc)
		return "Tfc";
	if (speaker == kSpeakerTfr)
		return "Tfr";
	if (speaker == kSpeakerTrl)
		return "Trl";
	if (speaker == kSpeakerTrc)
		return "Trc";
	if (speaker == kSpeakerTrr)
		return "Trr";
	if (speaker == kSpeakerLfe2)
		return "LFE2";
	if (speaker == kSpeakerM)
		return "M";

	if (speaker == kSpeakerACN0)
		return "0";
	if (speaker == kSpeakerACN1)
		return "1";
	if (speaker == kSpeakerACN2)
		return "2";
	if (speaker == kSpeakerACN3)
		return "3";
	if (speaker == kSpeakerACN4)
		return "4";
	if (speaker == kSpeakerACN5)
		return "5";
	if (speaker == kSpeakerACN6)
		return "6";
	if (speaker == kSpeakerACN7)
		return "7";
	if (speaker == kSpeakerACN8)
		return "8";
	if (speaker == kSpeakerACN9)
		return "9";
	if (speaker == kSpeakerACN10)
		return "10";
	if (speaker == kSpeakerACN11)
		return "11";
	if (speaker == kSpeakerACN12)
		return "12";
	if (speaker == kSpeakerACN13)
		return "13";
	if (speaker == kSpeakerACN14)
		return "14";
	if (speaker == kSpeakerACN15)
		return "15";
	if (speaker == kSpeakerACN16)
		return "16";
	if (speaker == kSpeakerACN17)
		return "17";
	if (speaker == kSpeakerACN18)
		return "18";
	if (speaker == kSpeakerACN19)
		return "19";
	if (speaker == kSpeakerACN20)
		return "20";
	if (speaker == kSpeakerACN21)
		return "21";
	if (speaker == kSpeakerACN22)
		return "22";
	if (speaker == kSpeakerACN23)
		return "23";
	if (speaker == kSpeakerACN24)
		return "24";

	if (speaker == kSpeakerTsl)
		return "Tsl";
	if (speaker == kSpeakerTsr)
		return "Tsr";
	if (speaker == kSpeakerLcs)
		return "Lcs";
	if (speaker == kSpeakerRcs)
		return "Rcs";

	if (speaker == kSpeakerBfl)
		return "Bfl";
	if (speaker == kSpeakerBfc)
		return "Bfc";
	if (speaker == kSpeakerBfr)
		return "Bfr";
	if (speaker == kSpeakerPl)
		return "Pl";
	if (speaker == kSpeakerPr)
		return "Pr";
	if (speaker == kSpeakerBsl)
		return "Bsl";
	if (speaker == kSpeakerBsr)
		return "Bsr";
	if (speaker == kSpeakerBrl)
		return "Brl";
	if (speaker == kSpeakerBrc)
		return "Brc";
	if (speaker == kSpeakerBrr)
		return "Brr";

	if (speaker == kSpeakerLw)
		return "Lw";
	if (speaker == kSpeakerRw)
		return "Rw";
	return "";
}

/**@}*/

//------------------------------------------------------------------------
} // namespace SpeakerArr
} // namespace Vst
} // namespace Steinberg
