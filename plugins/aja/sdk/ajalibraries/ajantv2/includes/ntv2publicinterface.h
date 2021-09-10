/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2publicinterface.h
	@brief		Declares enums and structs used by all platform drivers and the SDK.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2PUBLICINTERFACE_H
#define NTV2PUBLICINTERFACE_H

#include "ajatypes.h"
#include "ntv2enums.h"
#if !defined(NTV2_BUILDING_DRIVER)
	#include <iostream>
	#include <set>
	#include <map>
	#include <vector>
	#include <iomanip>
	#include <bitset>
	#include <string>
	#include "ajaexport.h"
	#include "string.h"
	#if defined(MSWindows)
		#pragma warning(disable:4800)	//	int/bool conversion
		#pragma warning(disable:4127)	//	Stop MSVC from bitching about "do{...}while(false)" macros
	#endif
#endif	//	user-space clients only

#if defined (MSWindows)
	#include <basetsd.h>
#elif defined (AJAMac)
    #pragma GCC diagnostic ignored "-Wunused-private-field"
#endif


typedef enum
{
	kRegGlobalControl,				// 0
	kRegCh1Control,					// 1
	kRegCh1PCIAccessFrame,			// 2
	kRegCh1OutputFrame,				// 3
	kRegCh1InputFrame,				// 4
	kRegCh2Control,					// 5
	kRegCh2PCIAccessFrame,			// 6
	kRegCh2OutputFrame,				// 7
	kRegCh2InputFrame,				// 8
	kRegVidProc1Control,			// 9
	kRegVidProcXptControl,			// 10
	kRegMixer1Coefficient,			// 11
	kRegSplitControl,				// 12
	kRegFlatMatteValue,				// 13
	kRegOutputTimingControl,		// 14
	kRegReserved15,					// 15
	kRegReserved16,					// 16
	kRegFlashProgramReg,			// 17
	kRegLineCount,					// 18
	kRegOutputTimingFinePhase,		// 19  (was kRegReserved3)
	kRegAud1Delay = kRegOutputTimingFinePhase, //19
	kRegVidIntControl,				// 20
	kRegStatus,						// 21
	kRegInputStatus,				// 22
	kRegAud1Detect,					// 23
	kRegAud1Control,				// 24
	kRegAud1SourceSelect,			// 25
	kRegAud1OutputLastAddr,			// 26
	kRegAud1InputLastAddr,			// 27
	kRegAud1Counter,				// 28
	kRegRP188InOut1DBB,				// 29
	kRegRP188InOut1Bits0_31,		// 30
	kRegRP188InOut1Bits32_63,		// 31
	kRegDMA1HostAddr,				// 32
	kRegDMA1LocalAddr,				// 33
	kRegDMA1XferCount,				// 34
	kRegDMA1NextDesc,				// 35
	kRegDMA2HostAddr,				// 36
	kRegDMA2LocalAddr,				// 37
	kRegDMA2XferCount,				// 38
	kRegDMA2NextDesc,				// 39
	kRegDMA3HostAddr,				// 40
	kRegDMA3LocalAddr,				// 41
	kRegDMA3XferCount,				// 42
	kRegDMA3NextDesc,				// 43
	kRegDMA4HostAddr,				// 44
	kRegDMA4LocalAddr,				// 45
	kRegDMA4XferCount,				// 46
	kRegDMA4NextDesc,				// 47
	kRegDMAControl,					// 48
	kRegDMAIntControl,				// 49
	kRegBoardID,					// 50
	kRegReserved51,					// 51	
	kRegReserved52,					// 52	
	kRegReserved53,					// 53	
	kRegReserved54,					// 54	
	kRegReserved55,					// 55	
	kRegReserved56,					// 56	
	kRegReserved57,					// 57	
	kRegXenaxFlashControlStatus,    // 58	
	kRegXenaxFlashAddress,			// 59	
	kRegXenaxFlashDIN,				// 60	
	kRegXenaxFlashDOUT,				// 61	
	kRegReserved62,					// 62	
	kRegCPLDVersion,				// 63	
	kRegRP188InOut2DBB,				// 64
	kRegRP188InOut2Bits0_31,		// 65
	kRegRP188InOut2Bits32_63,		// 66
	kRegCanDoStatus,				// 67	SDK 15.6 and later
	kRegCh1ColorCorrectionControl,	// 68	CamelCase fix in SDK 16.0
	kRegCh2ColorCorrectionControl,	// 69	CamelCase fix SDK 16.0
	kRegRS422Transmit,				// 70
	kRegRS422Receive,				// 71
	kRegRS422Control,				// 72
	kRegReserved73,					// 73
	kRegReserved74,					// 74
	kRegReserved75,					// 75
	kRegReserved76,					// 76
	kRegReserved77,					// 77
	kRegReserved78,					// 78
	kRegReserved79,					// 79
	kRegReserved80,					// 80

	kRegAnalogInputStatus,			// 81
	kRegAnalogInputControl,			// 82
	kRegReserved83,					// 83
	kRegFS1ProcAmpC1Y_C1CB,			// 84
	kRegFS1ProcAmpC1CR_C2CB,		// 85
	kRegFS1ProcAmpC2CROffsetY,		// 86
	kRegFS1AudioDelay,				// 87
	kRegAud2Delay = kRegFS1AudioDelay, //87
	kRegBitfileDate,				// 88
	kRegBitfileTime,				// 89
	
	kRegFS1I2CControl,				// 90
	kRegFS1I2C1Address,				// 91
	kRegFS1I2C1Data,				// 92
	kRegFS1I2C2Address,				// 93
	kRegFS1I2C2Data,				// 94
	kRegFS1ReferenceSelect,			// 95
	kRegAverageAudioLevelChan1_2,	// 96
	kRegAverageAudioLevelChan3_4,	// 97
	kRegAverageAudioLevelChan5_6,	// 98
	kRegAverageAudioLevelChan7_8,	// 99

	kRegDMA1HostAddrHigh,			// 100
	kRegDMA1NextDescHigh,			// 101
	kRegDMA2HostAddrHigh,			// 102
	kRegDMA2NextDescHigh,			// 103
	kRegDMA3HostAddrHigh,			// 104
	kRegDMA3NextDescHigh,			// 105
	kRegDMA4HostAddrHigh,			// 106
	kRegDMA4NextDescHigh,			// 107

	kRegGlobalControl3,				// 108
	kRegReserved109,				// 109	//	Used for camera record flag		kRegLANCAndLensTap = kRegReserved109, // Borg	//Note:  Lens Tap is no longer supported by software. ref bug 3342. 4/5/2012
	kRegLTCEmbeddedBits0_31,        // 110
	kRegLTCEmbeddedBits32_63,       // 111
	kRegLTCAnalogBits0_31,			// 112
	kRegLTCAnalogBits32_63,			// 113

	kRegLTCOutBits0_31	= kRegLTCEmbeddedBits0_31,	// 110
	kRegLTCOutBits32_63	= kRegLTCEmbeddedBits32_63,	// 111
	kRegLTCInBits0_31	= kRegLTCAnalogBits0_31,	// 112
	kRegLTCInBits32_63	= kRegLTCAnalogBits32_63,	// 113

	kRegReserved114,				// 114
	kRegAudioControl2,				// 115
	kRegSysmonControl,				// 116
	kRegSysmonConfig1_0,			// 117
	kRegSysmonConfig2,				// 118
	kRegSysmonVccIntDieTemp,		// 119

	kRegInternalExternalVoltage,	// 120
	kRegFlashProgramReg2,			// 121
	kRegHDMIOut3DStatus1,			// 122
	kRegHDMIOut3DStatus2,			// 123
	kRegHDMIOut3DControl,			// 124
	kRegHDMIOutControl,				// 125
	kRegHDMIInputStatus,			// 126
	kRegHDMIInputControl,			// 127
	kRegAnalogOutControl,			// 128
	kRegSDIOut1Control,				// 129

	kRegSDIOut2Control,				// 130
	kRegConversionControl,			// 131
	kRegFrameSync1Control,			// 132
	kRegI2CWriteData,				// 133
	kRegFrameSync2Control,			// 134
	kRegI2CWriteControl,			// 135
	kRegXptSelectGroup1,			// 136
	kRegXptSelectGroup2,			// 137
	kRegXptSelectGroup3,			// 138
	kRegXptSelectGroup4,			// 139
	kRegXptSelectGroup5,			// 140
	kRegXptSelectGroup6,			// 141

	kRegCSCoefficients1_2,			// 142
	kRegCSCoefficients3_4,			// 143
	kRegCSCoefficients5_6,			// 144
	kRegCSCoefficients7_8,			// 145
	kRegCSCoefficients9_10,			// 146

	kRegCS2Coefficients1_2,			// 147
	kRegCS2Coefficients3_4,			// 148
	kRegCS2Coefficients5_6,			// 149
	kRegCS2Coefficients7_8,			// 150
	kRegCS2Coefficients9_10,		// 151

	kRegField1Line21CaptionDecode,	// 152		//	OBSOLETE
	kRegField2Line21CaptionDecode,	// 153		//	OBSOLETE
	kRegField1Line21CaptionEncode,	// 154		//	OBSOLETE
	kRegField2Line21CaptionEncode,	// 155		//	OBSOLETE
	kRegVANCGrabberSetup,			// 156		//	OBSOLETE
	kRegVANCGrabberStatus1,		    // 157		//	OBSOLETE
	kRegVANCGrabberStatus2,		    // 158		//	OBSOLETE
	kRegVANCGrabberDataBuffer,		// 159		//	OBSOLETE
	kRegVANCInserterSetup1,			// 160		//	OBSOLETE
	kRegVANCInserterSetup2,			// 161		//	OBSOLETE
	kRegVANCInserterDataBuffer,		// 162		//	OBSOLETE
	
	kRegXptSelectGroup7,			// 163
	kRegXptSelectGroup8,			// 164
	kRegCh1ControlExtended,			// 165
	kRegCh2ControlExtended,			// 166
	kRegAFDVANCGrabber,			    // 167
	kRegFS1DownConverter2Control,	// 168
	kRegSDIOut3Control,				// 169
	kRegSDIOut4Control,				// 170
	kRegAFDVANCInserterSDI1,		// 171
	kRegAFDVANCInserterSDI2,		// 172
    kRegAudioChannelMappingCh1,     // 173		//	OBSOLETE
    kRegXptSelectGroup36,		    // 174
    kRegXptSelectGroup37,		    // 175
    kRegAudioChannelMappingCh4,     // 176		//	OBSOLETE
    kRegAudioChannelMappingCh5,     // 177		//	OBSOLETE
    kRegAudioChannelMappingCh6,     // 178		//	OBSOLETE
    kRegAudioChannelMappingCh7,     // 179		//	OBSOLETE
    kRegAudioChannelMappingCh8,     // 180		//	OBSOLETE
	
	kRegReserved181,				// 181
	kRegReserved182,				// 182
	kRegReserved183,				// 183
	kRegReserved184,				// 184
	kRegReserved185,				// 185
	kRegReserved186,				// 186
	kRegReserved187,				// 187
	kRegVideoPayloadIDLinkA,		// 188
	kRegVideoPayloadIDLinkB,		// 189
	kRegSDIIn1VPIDA = kRegVideoPayloadIDLinkA,
	kRegSDIIn1VPIDB = kRegVideoPayloadIDLinkB,
	kRegAudioOutputSourceMap,		// 190
	kRegXptSelectGroup11,			// 191
	kRegStereoCompressor,			// 192
	kRegXptSelectGroup12,			// 193
	kRegFrameApertureOffset,		// 194
	kRegFirmwareUserID,				// 195
	kRegReserved196,				// 196
	kRegReserved197,				// 197
	kRegReserved198,				// 198
	kRegReserved199,				// 199
	kRegReserved200,				// 200
	kRegReserved201,				// 201
	kRegRP188InOut1Bits0_31_2,		// 202
	kRegRP188InOut1Bits32_63_2,		// 203
	kRegRP188InOut2Bits0_31_2,		// 204
	kRegRP188InOut2Bits32_63_2,		// 205
	kRegRP188InOut3Bits0_31_2,		// 206
	kRegRP188InOut3Bits32_63_2,		// 207
	kRegRP188InOut4Bits0_31_2,		// 208
	kRegRP188InOut4Bits32_63_2,		// 209
	kRegRP188InOut5Bits0_31_2,		// 210
	kRegRP188InOut5Bits32_63_2,		// 211
	kRegRP188InOut6Bits0_31_2,		// 212
	kRegRP188InOut6Bits32_63_2,		// 213
	kRegRP188InOut7Bits0_31_2,		// 214
	kRegRP188InOut7Bits32_63_2,		// 215
	kRegRP188InOut8Bits0_31_2,		// 216
	kRegRP188InOut8Bits32_63_2,		// 217
	kRegReserved218,				// 218
	kRegReserved219,				// 219
	kRegReserved220,				// 220
	kRegReserved221,				// 221
	kRegReserved222,				// 222
	kRegReserved223,				// 223
	kRegReserved224,				// 224
	kRegReserved225,				// 225
	kRegReserved226,				// 226
	kRegReserved227,				// 227
	kRegReserved228,				// 228
	kRegReserved229,				// 229
	kRegReserved230,				// 230
	kRegReserved231,				// 231
	kRegSDIInput3GStatus,			// 232
	kRegLTCStatusControl,			// 233
	kRegSDIOut1VPIDA,				// 234
	kRegSDIOut1VPIDB,				// 235
	kRegSDIOut2VPIDA,				// 236
	kRegSDIOut2VPIDB,				// 237
	kRegSDIIn2VPIDA,				// 238
	kRegSDIIn2VPIDB,				// 239
	kRegAud2Control,				// 240
	kRegAud2SourceSelect,			// 241
	kRegAud2OutputLastAddr,			// 242
	kRegAud2InputLastAddr,			// 243
	kRegRS4222Transmit,				// 244
	kRegRS4222Receive,				// 245
	kRegRS4222Control,				// 246
	kRegVidProc2Control,			// 247
	kRegMixer2Coefficient,			// 248
	kRegFlatMatte2Value,			// 249
	kRegXptSelectGroup9,			// 250
	kRegXptSelectGroup10,			// 251
	kRegLTC2EmbeddedBits0_31,       // 252
	kRegLTC2EmbeddedBits32_63,      // 253
	kRegLTC2AnalogBits0_31,			// 254
	kRegLTC2AnalogBits32_63,		// 255
	kRegSDITransmitControl,			// 256
	kRegCh3Control,					// 257
	kRegCh3OutputFrame,				// 258
	kRegCh3InputFrame,				// 259
	kRegCh4Control,					// 260
	kRegCh4OutputFrame,				// 261
	kRegCh4InputFrame,				// 262
	kRegXptSelectGroup13,			// 263
	kRegXptSelectGroup14,			// 264
	kRegStatus2,					// 265
	kRegVidIntControl2,				// 266
	kRegGlobalControl2,				// 267
	kRegRP188InOut3DBB,				// 268
	kRegRP188InOut3Bits0_31,		// 269
	kRegRP188InOut3Bits32_63,		// 270
	kRegSDIOut3VPIDA,				// 271
	kRegSDIOut3VPIDB,				// 272
	kRegRP188InOut4DBB,				// 273
	kRegRP188InOut4Bits0_31,		// 274
	kRegRP188InOut4Bits32_63,		// 275
	kRegSDIOut4VPIDA,				// 276
	kRegSDIOut4VPIDB,				// 277
	kRegAud3Control,				// 278
	kRegAud4Control,				// 279
	kRegAud3SourceSelect,			// 280
	kRegAud4SourceSelect,			// 281
	kRegAudDetect2,					// 282
	kRegAud3OutputLastAddr,			// 283
	kRegAud3InputLastAddr,			// 284
	kRegAud4OutputLastAddr,			// 285
	kRegAud4InputLastAddr,			// 286
	kRegSDIInput3GStatus2,			// 287
	kRegInputStatus2,				// 288
	kRegCh3PCIAccessFrame,			// 289
	kRegCh4PCIAccessFrame,			// 290

	kRegCS3Coefficients1_2,			// 291
	kRegCS3Coefficients3_4,			// 292
	kRegCS3Coefficients5_6,			// 293
	kRegCS3Coefficients7_8,			// 294
	kRegCS3Coefficients9_10,		// 295

	kRegCS4Coefficients1_2,			// 296
	kRegCS4Coefficients3_4,			// 297
	kRegCS4Coefficients5_6,			// 298
	kRegCS4Coefficients7_8,			// 299
	kRegCS4Coefficients9_10,		// 300

	kRegXptSelectGroup17,			// 301
	kRegXptSelectGroup15,			// 302
	kRegXptSelectGroup16,			// 303

	kRegAud3Delay,					// 304
	kRegAud4Delay,					// 305

	kRegSDIIn3VPIDA,				// 306
	kRegSDIIn3VPIDB,				// 307
	kRegSDIIn4VPIDA,				// 308
	kRegSDIIn4VPIDB,				// 309

	kRegSDIWatchdogControlStatus,	// 310
	kRegSDIWatchdogTimeout,			// 311
	kRegSDIWatchdogKick1,			// 312
	kRegSDIWatchdogKick2,			// 313
	kRegReserved314,				// 314
	kRegReserved315,				// 315

	kRegLTC3EmbeddedBits0_31,       // 316
	kRegLTC3EmbeddedBits32_63,      // 317

	kRegLTC4EmbeddedBits0_31,       // 318
	kRegLTC4EmbeddedBits32_63,      // 319

	kRegReserved320,				// 320
	kRegReserved321,				// 321
	kRegReserved322,				// 322
	kRegReserved323,				// 323
	kRegReserved324,				// 324
	kRegReserved325,				// 325
	kRegReserved326,				// 326
	kRegReserved327,				// 327
	kRegReserved328,				// 328

	kRegHDMITimeCode,				// 329

	//HDMI HDR Registers
	kRegHDMIHDRGreenPrimary,		// 330
	kRegHDMIHDRBluePrimary,			// 331
	kRegHDMIHDRRedPrimary,			// 332
	kRegHDMIHDRWhitePoint,			// 333
	kRegHDMIHDRMasteringLuminence,	// 334
	kRegHDMIHDRLightLevel,			// 335
	kRegHDMIHDRControl,				// 336

	kRegSDIOut5Control,				// 337
	kRegSDIOut5VPIDA,				// 338
	kRegSDIOut5VPIDB,				// 339

	kRegRP188InOut5Bits0_31,		// 340
	kRegRP188InOut5Bits32_63,		// 341
	kRegRP188InOut5DBB,				// 342

	kReg3DLUTLoadControl,			// 343

	kRegLTC5EmbeddedBits0_31,       // 344
	kRegLTC5EmbeddedBits32_63,      // 345

	kRegDL5Control,					// 346

	kRegCS5Coefficients1_2,			// 347
	kRegCS5Coefficients3_4,			// 348
	kRegCS5Coefficients5_6,			// 349
	kRegCS5Coefficients7_8,			// 350
	kRegCS5Coefficients9_10,		// 351

	kRegXptSelectGroup18,			// 352

	kRegReserved353,				// 353
	
	kRegDC1,						// 354
	kRegDC2,						// 355
	kRegXptSelectGroup19,			// 356

	kRegXptSelectGroup20,			// 357
	kRegRasterizerControl,			// 358

	//HDMI V2 In Registers
	kRegHDMIV2I2C1Control,			// 359
	kRegHDMIV2I2C1Data,				// 360
	kRegHDMIV2VideoSetup,			// 361
	kRegHDMIV2HSyncDurationAndBackPorch,	// 362
	kRegHDMIV2HActive,				// 363
	kRegHDMIV2VSyncDurationAndBackPorchField1,	// 364
	kRegHDMIV2VSyncDurationAndBackPorchField2,	// 365
	kRegHDMIV2VActiveField1,		// 366
	kRegHDMIV2VActiveField2,		// 367
	kRegHDMIV2VideoStatus,			// 368
	kRegHDMIV2HorizontalMeasurements,		// 369
	kRegHDMIV2HBlankingMeasurements,		// 370
	kRegHDMIV2HBlankingMeasurements1,		// 371
	kRegHDMIV2VerticalMeasurementsField0,	// 372
	kRegHDMIV2VerticalMeasurementsField1,	// 373
	kRegHDMIV2i2c2Control,			// 374
	kRegHDMIV2i2c2Data,				// 375

	kRegLUTV2Control,				// 376

	//Scott: New Dax/Multi-channel Registers
	kRegGlobalControlCh2,			// 377
	kRegGlobalControlCh3,			// 378
	kRegGlobalControlCh4,			// 379
	kRegGlobalControlCh5,			// 380
	kRegGlobalControlCh6,			// 381
	kRegGlobalControlCh7,			// 382
	kRegGlobalControlCh8,			// 383

	kRegCh5Control,					// 384
	kRegCh5OutputFrame,				// 385
	kRegCh5InputFrame,				// 386
	kRegCh5PCIAccessFrame,			// 387

	kRegCh6Control,					// 388
	kRegCh6OutputFrame,				// 389
	kRegCh6InputFrame,				// 390
	kRegCh6PCIAccessFrame,			// 391

	kRegCh7Control,					// 392
	kRegCh7OutputFrame,				// 393
	kRegCh7InputFrame,				// 394
	kRegCh7PCIAccessFrame,			// 395

	kRegCh8Control,					// 396
	kRegCh8OutputFrame,				// 397
	kRegCh8InputFrame,				// 398
	kRegCh8PCIAccessFrame,			// 399

	kRegXptSelectGroup21,			// 400
	kRegXptSelectGroup22,			// 401
	kRegXptSelectGroup30,			// 402
	kRegXptSelectGroup23,			// 403
	kRegXptSelectGroup24,			// 404
	kRegXptSelectGroup25,			// 405
	kRegXptSelectGroup26,			// 406
	kRegXptSelectGroup27,			// 407
	kRegXptSelectGroup28,			// 408
	kRegXptSelectGroup29,			// 409

	kRegSDIIn5VPIDA,				// 410
	kRegSDIIn5VPIDB,				// 411

	kRegSDIIn6VPIDA,				// 412
	kRegSDIIn6VPIDB,				// 413
	kRegSDIOut6VPIDA,				// 414
	kRegSDIOut6VPIDB,				// 415
	kRegRP188InOut6Bits0_31,		// 416
	kRegRP188InOut6Bits32_63,		// 417
	kRegRP188InOut6DBB,				// 418
	kRegLTC6EmbeddedBits0_31,		// 419
	kRegLTC6EmbeddedBits32_63,		// 420

	kRegSDIIn7VPIDA,				// 421
	kRegSDIIn7VPIDB,				// 422
	kRegSDIOut7VPIDA,				// 423
	kRegSDIOut7VPIDB,				// 424
	kRegRP188InOut7Bits0_31,		// 425
	kRegRP188InOut7Bits32_63,		// 426
	kRegRP188InOut7DBB,				// 427
	kRegLTC7EmbeddedBits0_31,		// 428
	kRegLTC7EmbeddedBits32_63,		// 429

	kRegSDIIn8VPIDA,				// 430
	kRegSDIIn8VPIDB,				// 431
	kRegSDIOut8VPIDA,				// 432
	kRegSDIOut8VPIDB,				// 433
	kRegRP188InOut8Bits0_31,		// 434
	kRegRP188InOut8Bits32_63,		// 435
	kRegRP188InOut8DBB,				// 436
	kRegLTC8EmbeddedBits0_31,		// 437
	kRegLTC8EmbeddedBits32_63,		// 438

	kRegXptSelectGroup31,			// 439

	kRegAud5Control,				// 440
	kRegAud5SourceSelect,			// 441
	kRegAud5OutputLastAddr,			// 442
	kRegAud5InputLastAddr,			// 443

	kRegAud6Control,				// 444
	kRegAud6SourceSelect,			// 445
	kRegAud6OutputLastAddr,			// 446
	kRegAud6InputLastAddr,			// 447

	kRegAud7Control,				// 448
	kRegAud7SourceSelect,			// 449
	kRegAud7OutputLastAddr,			// 450
	kRegAud7InputLastAddr,			// 451

	kRegAud8Control,				// 452
	kRegAud8SourceSelect,			// 453
	kRegAud8OutputLastAddr,			// 454
	kRegAud8InputLastAddr,			// 455

	kRegAudioDetect5678,			// 456

	kRegSDI5678Input3GStatus,		// 457
	
	kRegInput56Status,				// 458
	kRegInput78Status,				// 459

	kRegCS6Coefficients1_2,			// 460
	kRegCS6Coefficients3_4,			// 461
	kRegCS6Coefficients5_6,			// 462
	kRegCS6Coefficients7_8,			// 463
	kRegCS6Coefficients9_10,		// 464

	kRegCS7Coefficients1_2,			// 465
	kRegCS7Coefficients3_4,			// 466
	kRegCS7Coefficients5_6,			// 467
	kRegCS7Coefficients7_8,			// 468
	kRegCS7Coefficients9_10,		// 469

	kRegCS8Coefficients1_2,			// 470
	kRegCS8Coefficients3_4,			// 471
	kRegCS8Coefficients5_6,			// 472
	kRegCS8Coefficients7_8,			// 473
	kRegCS8Coefficients9_10,		// 474

	kRegSDIOut6Control,				// 475
	kRegSDIOut7Control,				// 476
	kRegSDIOut8Control,				// 477

	kRegOutputTimingControlch2,		// 478
	kRegOutputTimingControlch3,		// 479
	kRegOutputTimingControlch4,		// 480
	kRegOutputTimingControlch5,		// 481
	kRegOutputTimingControlch6,		// 482
	kRegOutputTimingControlch7,		// 483
	kRegOutputTimingControlch8,		// 484

	kRegVidProc3Control,			// 485
	kRegMixer3Coefficient,			// 486
	kRegFlatMatte3Value,			// 487

	kRegVidProc4Control,			// 488
	kRegMixer4Coefficient,			// 489
	kRegFlatMatte4Value,			// 490

	kRegTRSErrorStatus,				// 491

	kRegAud5Delay,					// 492
	kRegAud6Delay,					// 493
	kRegAud7Delay,					// 494
	kRegAud8Delay,					// 495

	kRegPCMControl4321,				// 496
	kRegPCMControl8765,				// 497

	kRegCh1Control2MFrame,			// 498
	kRegCh2Control2MFrame,			// 499
	kRegCh3Control2MFrame,			// 500
	kRegCh4Control2MFrame,			// 501
	kRegCh5Control2MFrame,			// 502
	kRegCh6Control2MFrame,			// 503
	kRegCh7Control2MFrame,			// 504
	kRegCh8Control2MFrame,			// 505
	kRegXptSelectGroup32,			// 506
	kRegXptSelectGroup33,			// 507
	kRegXptSelectGroup34,			// 508
	kRegXptSelectGroup35,			// 509
	kRegReserved510,				// 510
	kRegReserved511,				// 511

	kRegNumRegisters
} NTV2RegisterNumber;

typedef NTV2RegisterNumber	RegisterNum;

#define kRegCh1ColorCorrectioncontrol	kRegCh1ColorCorrectionControl	//	CamelCase fixed in SDK 16.0
#define	kRegCh2ColorCorrectioncontrol	kRegCh2ColorCorrectionControl	//	CamelCase fixed in SDK 16.0

//	Discontinuous block of registers used for monitoring the incoming SDI signals
typedef enum NTV2RXSDIStatusRegister
{
	kRegRXSDI1Status = 2048,		// 2048
	kRegRXSDI1CRCErrorCount,		// 2049
	kRegRXSDI1FrameCountLow,		// 2050
	kRegRXSDI1FrameCountHigh,		// 2051
	kRegRXSDI1FrameRefCountLow,		// 2052
	kRegRXSDI1FrameRefCountHigh,	// 2053
	kRegRXSDI1Unused2054,			// 2054
	kRegRXSDI1Unused2055,			// 2055

	kRegRXSDI2Status,				// 2056
	kRegRXSDI2CRCErrorCount,		// 2057
	kRegRXSDI2FrameCountLow,		// 2058
	kRegRXSDI2FrameCountHigh,		// 2059
	kRegRXSDI2FrameRefCountLow,		// 2060
	kRegRXSDI2FrameRefCountHigh,	// 2061
	kRegRXSDI2Unused2062,			// 2062
	kRegRXSDI2Unused2063,			// 2063

	kRegRXSDI3Status,				// 2064
	kRegRXSDI3CRCErrorCount,		// 2065
	kRegRXSDI3FrameCountLow,		// 2066
	kRegRXSDI3FrameCountHigh,		// 2067
	kRegRXSDI3FrameRefCountLow,		// 2068
	kRegRXSDI3FrameRefCountHigh,	// 2069
	kRegRXSDI3Unused2070,			// 2070
	kRegRXSDI3Unused2071,			// 2071

	kRegRXSDI4Status,				// 2072
	kRegRXSDI4CRCErrorCount,		// 2073
	kRegRXSDI4FrameCountLow,		// 2074
	kRegRXSDI4FrameCountHigh,		// 2075
	kRegRXSDI4FrameRefCountLow,		// 2076
	kRegRXSDI4FrameRefCountHigh,	// 2077
	kRegRXSDI4Unused2078,			// 2078
	kRegRXSDI4Unused2079,			// 2079

	kRegRXSDI5Status,				// 2080
	kRegRXSDI5CRCErrorCount,		// 2081
	kRegRXSDI5FrameCountLow,		// 2082
	kRegRXSDI5FrameCountHigh,		// 2083
	kRegRXSDI5FrameRefCountLow,		// 2084
	kRegRXSDI5FrameRefCountHigh,	// 2085
	kRegRXSDI5Unused2086,			// 2086
	kRegRXSDI5Unused2087,			// 2087

	kRegRXSDI6Status,				// 2088
	kRegRXSDI6CRCErrorCount,		// 2089
	kRegRXSDI6FrameCountLow,		// 2090
	kRegRXSDI6FrameCountHigh,		// 2091
	kRegRXSDI6FrameRefCountLow,		// 2092
	kRegRXSDI6FrameRefCountHigh,	// 2093
	kRegRXSDI6Unused2094,			// 2094
	kRegRXSDI6Unused2095,			// 2095

	kRegRXSDI7Status,				// 2096
	kRegRXSDI7CRCErrorCount,		// 2097
	kRegRXSDI7FrameCountLow,		// 2098
	kRegRXSDI7FrameCountHigh,		// 2099
	kRegRXSDI7FrameRefCountLow,		// 2100
	kRegRXSDI7FrameRefCountHigh,	// 2101
	kRegRXSDI7Unused2102,			// 2102
	kRegRXSDI7Unused2103,			// 2103

	kRegRXSDI8Status,				// 2104
	kRegRXSDI8CRCErrorCount,		// 2105
	kRegRXSDI8FrameCountLow,		// 2106
	kRegRXSDI8FrameCountHigh,		// 2107
	kRegRXSDI8FrameRefCountLow,		// 2108
	kRegRXSDI8FrameRefCountHigh,	// 2109
	kRegRXSDI8Unused2110,			// 2110
	kRegRXSDI8Unused2111,			// 2111

	kRegRXSDIFreeRunningClockLow,	// 2112
	kRegRXSDIFreeRunningClockHigh,	// 2113

	kRegNumRXSDIRegisters = 2113 - 2048 + 1
} NTV2RXSDIStatusRegisters;

typedef enum
{
	kReg1DLUTLoadControl1 = 2200,	//2200
	kReg1DLUTLoadControl2,			//2201
	kReg1DLUTLoadControl3,			//2202
	kReg1DLUTLoadControl4,			//2203
	kReg1DLUTLoadControl5,			//2204
	kReg1DLUTLoadControl6,			//2205
	kReg1DLUTLoadControl7,			//2206
	kReg1DLUTLoadControl8			//2207
}NTV2LUTControlRegisters;

typedef enum
{
	kRegAudioMixerInputSelects = 2304		//	2304
	,kRegAudioMixerMainGain					//	2305
	,kRegAudioMixerAux1GainCh1				//	2306
	,kRegAudioMixerAux2GainCh1				//	2307
	,kRegAudioMixerChannelSelect			//	2308
	,kRegAudioMixerMutes					//	2309
	,kRegAudioMixerAux1GainCh2				//	2310
	,kRegAudioMixerAux2GainCh2				//	2311
	,kRegAudioMixerOutGain					//  2312
	,kRegAudioMixerOutLGain	= kRegAudioMixerOutGain
	,kRegAudioMixerOutRGain					//	2313
	,kRegAudioMixerAux1InputLevels = 2318	//	2318
	,kRegAudioMixerAux2InputLevels			//	2319
	,kRegAudioMixerMainInputLevelsPair0		//	2320
	,kRegAudioMixerMainInputLevelsPair1		//	2321
	,kRegAudioMixerMainInputLevelsPair2		//	2322
	,kRegAudioMixerMainInputLevelsPair3		//	2323
	,kRegAudioMixerMainInputLevelsPair4		//	2324
	,kRegAudioMixerMainInputLevelsPair5		//	2325
	,kRegAudioMixerMainInputLevelsPair6		//	2326
	,kRegAudioMixerMainInputLevelsPair7		//	2327
	,kRegAudioMixerMixedChannelOutputLevels	//	2328
	,kRegAudioMixerMainOutputLevelsPair0 = kRegAudioMixerMixedChannelOutputLevels	//	2328
	,kRegAudioMixerMainOutputLevelsPair1	//	2329
	,kRegAudioMixerMainOutputLevelsPair2	//	2330
	,kRegAudioMixerMainOutputLevelsPair3	//	2331
	,kRegAudioMixerMainOutputLevelsPair4	//	2332
	,kRegAudioMixerMainOutputLevelsPair5	//	2333
	,kRegAudioMixerMainOutputLevelsPair6	//	2334
	,kRegAudioMixerMainOutputLevelsPair7	//	2335	
} NTV2AudioMixerRegisters;

//	Discontinuous block of registers used for detecting non-PCM embedded audio.
typedef enum _NTV2NonPCMAudioDetectRegisters
{
	kRegFirstNonPCMAudioDetectRegister	= 2130,
	kRegNonPCMAudioDetectEngine1		= kRegFirstNonPCMAudioDetectRegister,		//	2130
	kRegNonPCMAudioDetectEngine2		= kRegFirstNonPCMAudioDetectRegister + 2,	//	2132
	kRegNonPCMAudioDetectEngine3		= kRegFirstNonPCMAudioDetectRegister + 4,	//	2134
	kRegNonPCMAudioDetectEngine4		= kRegFirstNonPCMAudioDetectRegister + 6,	//	2136
	kRegNonPCMAudioDetectEngine5		= kRegFirstNonPCMAudioDetectRegister + 8,	//	2138
	kRegNonPCMAudioDetectEngine6		= kRegFirstNonPCMAudioDetectRegister + 10,	//	2140
	kRegNonPCMAudioDetectEngine7		= kRegFirstNonPCMAudioDetectRegister + 12,	//	2142
	kRegNonPCMAudioDetectEngine8		= kRegFirstNonPCMAudioDetectRegister + 14,	//	2144
	kRegLastNonPCMAudioDetectRegister	= kRegNonPCMAudioDetectEngine8
} NTV2NonPCMAudioDetectRegisters;

//	New in SDK 15.6:
//	Some boards have new firmware that implements a "valid route" bitmap ROM accessed by a contiguous block of registers:
typedef enum _NTV2XptValidROMRegisters
{
	kRegFirstValidXptROMRegister	= 3072,	//	Starts at reg 3072
	kRegNumValidXptROMRegisters		= 1024,	//	It's 4096 bytes long
	kRegLastValidXptROMRegister		= kRegFirstValidXptROMRegister + kRegNumValidXptROMRegisters - 1,
	kRegInvalidValidXptROMRegister	= kRegFirstValidXptROMRegister + kRegNumValidXptROMRegisters
} NTV2XptValidROMRegisters;

//	Discontinuous block of registers used to control the enhanced color space converters
typedef enum
{
	kRegEnhancedCSC1Mode = 5120,	// 5120
	kRegEnhancedCSC1InOffset0_1,	// 5121
	kRegEnhancedCSC1InOffset2,		// 5122
	kRegEnhancedCSC1CoeffA0,		// 5123
	kRegEnhancedCSC1CoeffA1,		// 5124
	kRegEnhancedCSC1CoeffA2,		// 5125
	kRegEnhancedCSC1CoeffB0,		// 5126
	kRegEnhancedCSC1CoeffB1,		// 5127
	kRegEnhancedCSC1CoeffB2,		// 5128
	kRegEnhancedCSC1CoeffC0,		// 5129
	kRegEnhancedCSC1CoeffC1,		// 5130
	kRegEnhancedCSC1CoeffC2,		// 5131
	kRegEnhancedCSC1OutOffsetA_B,	// 5132
	kRegEnhancedCSC1OutOffsetC,		// 5133
	kRegEnhancedCSC1KeyMode,		// 5134
	kRegEnhancedCSC1KeyClipOffset,	// 5135
	kRegEnhancedCSC1KeyGain,		// 5136

	kRegNumEnhancedCSCRegisters = 5136 - 5120 + 1,

	kRegEnhancedCSC2Mode = 5184,	// 5184
	kRegEnhancedCSC2InOffset0_1,	// 5185
	kRegEnhancedCSC2InOffset2,		// 5186
	kRegEnhancedCSC2CoeffA0,		// 5187
	kRegEnhancedCSC2CoeffA1,		// 5188
	kRegEnhancedCSC2CoeffA2,		// 5189
	kRegEnhancedCSC2CoeffB0,		// 5190
	kRegEnhancedCSC2CoeffB1,		// 5191
	kRegEnhancedCSC2CoeffB2,		// 5192
	kRegEnhancedCSC2CoeffC0,		// 5193
	kRegEnhancedCSC2CoeffC1,		// 5194
	kRegEnhancedCSC2CoeffC2,		// 5195
	kRegEnhancedCSC2OutOffsetA_B,	// 5196
	kRegEnhancedCSC2OutOffsetC,		// 5197
	kRegEnhancedCSC2KeyMode,		// 5198
	kRegEnhancedCSC2KeyClipOffset,	// 5199
	kRegEnhancedCSC2KeyGain,		// 5200

	kRegEnhancedCSC3Mode = 5248,	// 5248
	kRegEnhancedCSC3InOffset0_1,	// 5249
	kRegEnhancedCSC3InOffset2,		// 5250
	kRegEnhancedCSC3CoeffA0,		// 5251
	kRegEnhancedCSC3CoeffA1,		// 5252
	kRegEnhancedCSC3CoeffA2,		// 5253
	kRegEnhancedCSC3CoeffB0,		// 5254
	kRegEnhancedCSC3CoeffB1,		// 5255
	kRegEnhancedCSC3CoeffB2,		// 5256
	kRegEnhancedCSC3CoeffC0,		// 5257
	kRegEnhancedCSC3CoeffC1,		// 5258
	kRegEnhancedCSC3CoeffC2,		// 5259
	kRegEnhancedCSC3OutOffsetA_B,	// 5260
	kRegEnhancedCSC3OutOffsetC,		// 5261
	kRegEnhancedCSC3KeyMode,		// 5262
	kRegEnhancedCSC3KeyClipOffset,	// 5263
	kRegEnhancedCSC3KeyGain,		// 5264

	kRegEnhancedCSC4Mode = 5312,	// 5312
	kRegEnhancedCSC4InOffset0_1,	// 5313
	kRegEnhancedCSC4InOffset2,		// 5314
	kRegEnhancedCSC4CoeffA0,		// 5315
	kRegEnhancedCSC4CoeffA1,		// 5316
	kRegEnhancedCSC4CoeffA2,		// 5317
	kRegEnhancedCSC4CoeffB0,		// 5318
	kRegEnhancedCSC4CoeffB1,		// 5319
	kRegEnhancedCSC4CoeffB2,		// 5320
	kRegEnhancedCSC4CoeffC0,		// 5321
	kRegEnhancedCSC4CoeffC1,		// 5322
	kRegEnhancedCSC4CoeffC2,		// 5323
	kRegEnhancedCSC4OutOffsetA_B,	// 5324
	kRegEnhancedCSC4OutOffsetC,		// 5325
	kRegEnhancedCSC4KeyMode,		// 5326
	kRegEnhancedCSC4KeyClipOffset,	// 5327
	kRegEnhancedCSC4KeyGain,		// 5328

	kRegEnhancedCSC5Mode = 5376,	// 5376
	kRegEnhancedCSC5InOffset0_1,	// 5377
	kRegEnhancedCSC5InOffset2,		// 5378
	kRegEnhancedCSC5CoeffA0,		// 5379
	kRegEnhancedCSC5CoeffA1,		// 5380
	kRegEnhancedCSC5CoeffA2,		// 5381
	kRegEnhancedCSC5CoeffB0,		// 5382
	kRegEnhancedCSC5CoeffB1,		// 5383
	kRegEnhancedCSC5CoeffB2,		// 5384
	kRegEnhancedCSC5CoeffC0,		// 5385
	kRegEnhancedCSC5CoeffC1,		// 5386
	kRegEnhancedCSC5CoeffC2,		// 5387
	kRegEnhancedCSC5OutOffsetA_B,	// 5388
	kRegEnhancedCSC5OutOffsetC,		// 5389
	kRegEnhancedCSC5KeyMode,		// 5390
	kRegEnhancedCSC5KeyClipOffset,	// 5391
	kRegEnhancedCSC5KeyGain,		// 5392

	kRegEnhancedCSC6Mode = 5440,	// 5440 or fight
	kRegEnhancedCSC6InOffset0_1,	// 5441
	kRegEnhancedCSC6InOffset2,		// 5442
	kRegEnhancedCSC6CoeffA0,		// 5443
	kRegEnhancedCSC6CoeffA1,		// 5444
	kRegEnhancedCSC6CoeffA2,		// 5445
	kRegEnhancedCSC6CoeffB0,		// 5446
	kRegEnhancedCSC6CoeffB1,		// 5447
	kRegEnhancedCSC6CoeffB2,		// 5448
	kRegEnhancedCSC6CoeffC0,		// 5449
	kRegEnhancedCSC6CoeffC1,		// 5450
	kRegEnhancedCSC6CoeffC2,		// 5451
	kRegEnhancedCSC6OutOffsetA_B,	// 5452
	kRegEnhancedCSC6OutOffsetC,		// 5453
	kRegEnhancedCSC6KeyMode,		// 5454
	kRegEnhancedCSC6KeyClipOffset,	// 5455
	kRegEnhancedCSC6KeyGain,		// 5456

	kRegEnhancedCSC7Mode = 5504,	// 5504
	kRegEnhancedCSC7InOffset0_1,	// 5505
	kRegEnhancedCSC7InOffset2,		// 5506
	kRegEnhancedCSC7CoeffA0,		// 5507
	kRegEnhancedCSC7CoeffA1,		// 5508
	kRegEnhancedCSC7CoeffA2,		// 5509
	kRegEnhancedCSC7CoeffB0,		// 5510
	kRegEnhancedCSC7CoeffB1,		// 5511
	kRegEnhancedCSC7CoeffB2,		// 5512
	kRegEnhancedCSC7CoeffC0,		// 5513
	kRegEnhancedCSC7CoeffC1,		// 5514
	kRegEnhancedCSC7CoeffC2,		// 5515
	kRegEnhancedCSC7OutOffsetA_B,	// 5516
	kRegEnhancedCSC7OutOffsetC,		// 5517
	kRegEnhancedCSC7KeyMode,		// 5518
	kRegEnhancedCSC7KeyClipOffset,	// 5519
	kRegEnhancedCSC7KeyGain,		// 5520

	kRegEnhancedCSC8Mode = 5568,	// 5568
	kRegEnhancedCSC8InOffset0_1,	// 5569
	kRegEnhancedCSC8InOffset2,		// 5570
	kRegEnhancedCSC8CoeffA0,		// 5571
	kRegEnhancedCSC8CoeffA1,		// 5572
	kRegEnhancedCSC8CoeffA2,		// 5573
	kRegEnhancedCSC8CoeffB0,		// 5574
	kRegEnhancedCSC8CoeffB1,		// 5575
	kRegEnhancedCSC8CoeffB2,		// 5576
	kRegEnhancedCSC8CoeffC0,		// 5577
	kRegEnhancedCSC8CoeffC1,		// 5578
	kRegEnhancedCSC8CoeffC2,		// 5579
	kRegEnhancedCSC8OutOffsetA_B,	// 5580
	kRegEnhancedCSC8OutOffsetC,		// 5581
	kRegEnhancedCSC8KeyMode,		// 5582
	kRegEnhancedCSC8KeyClipOffset,	// 5583
	kRegEnhancedCSC8KeyGain			// 5584

} NTV2EnhancedCSCRegisters;

typedef enum _NTV2OERegisters
{
	kRegOEData1					= 0x1700,
	kRegOEData256				= 0x173F
} NTV2OERegisters;

typedef enum
{
	kRegHDMIOutputAudioConfig = 0x1d4B
}NTV2HDMIOutAudioRegisters;

typedef enum _NTV2HDMIRegisters
{
	kRegHDMIOutputAuxData		= 0x0a00,
	kRegHDMIOutputConfig1       = 0x1d14,
	kRegHDMIInputStatus1        = 0x1d15,
	kRegHDMIControl1            = 0x1d16,
	kRegHDMIOutputConfig2       = 0x2514,
	kRegHDMIInputStatus2        = 0x2515,
	kRegHDMIControl2            = 0x2516,
	kRegHDMIOutputConfig3       = 0x2c12,
	kRegHDMIInputStatus3        = 0x2c13,
	kRegHDMIControl3            = 0x2c14,
	kRegHDMIOutputConfig4       = 0x3012,
	kRegHDMIInputStatus4        = 0x3013,
	kRegHDMIControl4            = 0x3014
} NTV2HDMIRegisters;

typedef enum _NTV2MultiRasterRegisters
{
	kRegMRQ1Control = 0x3C00,
	kRegMRQ2Control,
	kRegMRQ3Control,
	kRegMRQ4Control,
	kRegMROutControl,
	kRegMRSupport = 0x3C07
} NTV2MultiRasterRegisters;

typedef enum
{
    kRegRotaryEncoder = 0x940
} NTV2RotaryEncoderRegister;

#define NTV2_HDMIAuxMaxFrames	8
#define NTV2_HDMIAuxDataSize	32

#if !defined (NTV2_DEPRECATE)
	#define	KRegDMA1HostAddr			kRegDMA1HostAddr			///< @deprecated		Use kRegDMA1HostAddr instead.
	#define	KRegDMA2HostAddr			kRegDMA2HostAddr			///< @deprecated		Use kRegDMA2HostAddr instead.
	#define	KRegDMA3HostAddr			kRegDMA3HostAddr			///< @deprecated		Use kRegDMA3HostAddr instead.
	#define	KRegDMA4HostAddr			kRegDMA4HostAddr			///< @deprecated		Use kRegDMA4HostAddr instead.
	#define	kRegPanControl				kRegReserved15				///< @deprecated		Do not use (not supported).
	#define	kRegReserved2				kRegReserved16				///< @deprecated		Do not use.
	#define	kRegVidProcControl			kRegVidProc1Control			///< @deprecated		Use kRegVidProc1Control instead.
	#define	kRegMixerCoefficient		kRegMixer1Coefficient		///< @deprecated		Use kRegMixer1Coefficient instead.
	#define	kRegAudDetect				kRegAud1Detect				///< @deprecated		Use kRegAud1Detect instead.
	#define	kRegAudControl				kRegAud1Control				///< @deprecated		Use kRegAud1Control instead.
	#define	kRegAudSourceSelect			kRegAud1SourceSelect		///< @deprecated		Use kRegAud1SourceSelect instead.
	#define	kRegAudOutputLastAddr		kRegAud1OutputLastAddr		///< @deprecated		Use kRegAud1OutputLastAddr instead.
	#define	kRegAudInputLastAddr		kRegAud1InputLastAddr		///< @deprecated		Use kRegAud1InputLastAddr instead.
	#define	kRegAudCounter				kRegAud1Counter				///< @deprecated		Use kRegAud1Counter instead.
	#define	kRegFlatMattleValue			kRegFlatMatteValue			///< @deprecated		Use kRegFlatMatteValue instead.
	#define	kRegFlatMattle2Value		kRegFlatMatte2Value			///< @deprecated		Use kRegFlatMatte2Value instead.

	#define	kK2RegAnalogOutControl		kRegAnalogOutControl		///< @deprecated		Use kRegAnalogOutControl instead.
	#define	kK2RegSDIOut1Control		kRegSDIOut1Control			///< @deprecated		Use kRegSDIOut1Control instead.
	#define	kK2RegSDIOut2Control		kRegSDIOut2Control			///< @deprecated		Use kRegSDIOut2Control instead.
	#define	kK2RegConversionControl		kRegConversionControl		///< @deprecated		Use kRegConversionControl instead.
	#define	kK2RegFrameSync1Control		kRegFrameSync1Control		///< @deprecated		Use kRegFrameSync1Control instead.
	#define	kK2RegFrameSync2Control		kRegFrameSync2Control		///< @deprecated		Use kRegFrameSync2Control instead.
	#define	kK2RegXptSelectGroup1		kRegXptSelectGroup1			///< @deprecated		Use kRegXptSelectGroup1 instead.
	#define	kK2RegXptSelectGroup2		kRegXptSelectGroup2			///< @deprecated		Use kRegXptSelectGroup2 instead.
	#define	kK2RegXptSelectGroup3		kRegXptSelectGroup3			///< @deprecated		Use kRegXptSelectGroup3 instead.
	#define	kK2RegXptSelectGroup4		kRegXptSelectGroup4			///< @deprecated		Use kRegXptSelectGroup4 instead.
	#define	kK2RegXptSelectGroup5		kRegXptSelectGroup5			///< @deprecated		Use kRegXptSelectGroup5 instead.
	#define	kK2RegXptSelectGroup6		kRegXptSelectGroup6			///< @deprecated		Use kRegXptSelectGroup6 instead.
	#define	kK2RegCSCoefficients1_2		kRegCSCoefficients1_2		///< @deprecated		Use kRegCSCoefficients1_2 instead.
	#define	kK2RegCSCoefficients3_4		kRegCSCoefficients3_4		///< @deprecated		Use kRegCSCoefficients3_4 instead.
	#define	kK2RegCSCoefficients5_6		kRegCSCoefficients5_6		///< @deprecated		Use kRegCSCoefficients5_6 instead.
	#define	kK2RegCSCoefficients7_8		kRegCSCoefficients7_8		///< @deprecated		Use kRegCSCoefficients7_8 instead.
	#define	kK2RegCSCoefficients9_10	kRegCSCoefficients9_10		///< @deprecated		Use kRegCSCoefficients9_10 instead.
	#define	kK2RegCS2Coefficients1_2	kRegCS2Coefficients1_2		///< @deprecated		Use kRegCS2Coefficients1_2 instead.
	#define	kK2RegCS2Coefficients3_4	kRegCS2Coefficients3_4		///< @deprecated		Use kRegCS2Coefficients3_4 instead.
	#define	kK2RegCS2Coefficients5_6	kRegCS2Coefficients5_6		///< @deprecated		Use kRegCS2Coefficients5_6 instead.
	#define	kK2RegCS2Coefficients7_8	kRegCS2Coefficients7_8		///< @deprecated		Use kRegCS2Coefficients7_8 instead.
	#define	kK2RegCS2Coefficients9_10	kRegCS2Coefficients9_10		///< @deprecated		Use kRegCS2Coefficients9_10 instead.
	#define	kK2RegXptSelectGroup7		kRegXptSelectGroup7			///< @deprecated		Use kRegXptSelectGroup7 instead.
	#define	kK2RegXptSelectGroup8		kRegXptSelectGroup8			///< @deprecated		Use kRegXptSelectGroup8 instead.
	#define	kK2RegSDIOut3Control		kRegSDIOut3Control			///< @deprecated		Use kRegSDIOut3Control instead.
	#define	kK2RegSDIOut4Control		kRegSDIOut4Control			///< @deprecated		Use kRegSDIOut4Control instead.
	#define	kK2RegXptSelectGroup11		kRegXptSelectGroup11		///< @deprecated		Use kRegXptSelectGroup11 instead.
	#define	kK2RegXptSelectGroup12		kRegXptSelectGroup12		///< @deprecated		Use kRegXptSelectGroup12 instead.
	#define	kK2RegXptSelectGroup9		kRegXptSelectGroup9			///< @deprecated		Use kRegXptSelectGroup9 instead.
	#define	kK2RegXptSelectGroup10		kRegXptSelectGroup10		///< @deprecated		Use kRegXptSelectGroup10 instead.
	#define	kK2RegXptSelectGroup13		kRegXptSelectGroup13		///< @deprecated		Use kRegXptSelectGroup13 instead.
	#define	kK2RegXptSelectGroup14		kRegXptSelectGroup14		///< @deprecated		Use kRegXptSelectGroup14 instead.
	#define	kK2RegCS3Coefficients1_2	kRegCS3Coefficients1_2		///< @deprecated		Use kRegCS3Coefficients1_2 instead.
	#define	kK2RegCS3Coefficients3_4	kRegCS3Coefficients3_4		///< @deprecated		Use kRegCS3Coefficients3_4 instead.
	#define	kK2RegCS3Coefficients5_6	kRegCS3Coefficients5_6		///< @deprecated		Use kRegCS3Coefficients5_6 instead.
	#define	kK2RegCS3Coefficients7_8	kRegCS3Coefficients7_8		///< @deprecated		Use kRegCS3Coefficients7_8 instead.
	#define	kK2RegCS3Coefficients9_10	kRegCS3Coefficients9_10		///< @deprecated		Use kRegCS3Coefficients9_10 instead.
	#define	kK2RegCS4Coefficients1_2	kRegCS4Coefficients1_2		///< @deprecated		Use kRegCS4Coefficients1_2 instead.
	#define	kK2RegCS4Coefficients3_4	kRegCS4Coefficients3_4		///< @deprecated		Use kRegCS4Coefficients3_4 instead.
	#define	kK2RegCS4Coefficients5_6	kRegCS4Coefficients5_6		///< @deprecated		Use kRegCS4Coefficients5_6 instead.
	#define	kK2RegCS4Coefficients7_8	kRegCS4Coefficients7_8		///< @deprecated		Use kRegCS4Coefficients7_8 instead.
	#define	kK2RegCS4Coefficients9_10	kRegCS4Coefficients9_10		///< @deprecated		Use kRegCS4Coefficients9_10 instead.
	#define	kK2RegXptSelectGroup17		kRegXptSelectGroup17		///< @deprecated		Use kRegXptSelectGroup17 instead.
	#define	kK2RegXptSelectGroup15		kRegXptSelectGroup15		///< @deprecated		Use kRegXptSelectGroup15 instead.
	#define	kK2RegXptSelectGroup16		kRegXptSelectGroup16		///< @deprecated		Use kRegXptSelectGroup16 instead.
	#define	kK2RegSDIOut5Control		kRegSDIOut5Control			///< @deprecated		Use kRegSDIOut5Control instead.
	#define	kK2RegCS5Coefficients1_2	kRegCS5Coefficients1_2		///< @deprecated		Use kRegCS5Coefficients1_2 instead.
	#define	kK2RegCS5Coefficients3_4	kRegCS5Coefficients3_4		///< @deprecated		Use kRegCS5Coefficients3_4 instead.
	#define	kK2RegCS5Coefficients5_6	kRegCS5Coefficients5_6		///< @deprecated		Use kRegCS5Coefficients5_6 instead.
	#define	kK2RegCS5Coefficients7_8	kRegCS5Coefficients7_8		///< @deprecated		Use kRegCS5Coefficients7_8 instead.
	#define	kK2RegCS5Coefficients9_10	kRegCS5Coefficients9_10		///< @deprecated		Use kRegCS5Coefficients9_10 instead.
	#define	kK2RegXptSelectGroup18		kRegXptSelectGroup18		///< @deprecated		Use kRegXptSelectGroup18 instead.
	#define	kK2RegXptSelectGroup19		kRegXptSelectGroup19		///< @deprecated		Use kRegXptSelectGroup19 instead.
	#define	kK2RegXptSelectGroup20		kRegXptSelectGroup20		///< @deprecated		Use kRegXptSelectGroup20 instead.
	#define	kK2RegCS6Coefficients1_2	kRegCS6Coefficients1_2		///< @deprecated		Use kRegCS6Coefficients1_2 instead.
	#define	kK2RegCS6Coefficients3_4	kRegCS6Coefficients3_4		///< @deprecated		Use kRegCS6Coefficients3_4 instead.
	#define	kK2RegCS6Coefficients5_6	kRegCS6Coefficients5_6		///< @deprecated		Use kRegCS6Coefficients5_6 instead.
	#define	kK2RegCS6Coefficients7_8	kRegCS6Coefficients7_8		///< @deprecated		Use kRegCS6Coefficients7_8 instead.
	#define	kK2RegCS6Coefficients9_10	kRegCS6Coefficients9_10		///< @deprecated		Use kRegCS6Coefficients9_10 instead.
	#define	kK2RegCS7Coefficients1_2	kRegCS7Coefficients1_2		///< @deprecated		Use kRegCS7Coefficients1_2 instead.
	#define	kK2RegCS7Coefficients3_4	kRegCS7Coefficients3_4		///< @deprecated		Use kRegCS7Coefficients3_4 instead.
	#define	kK2RegCS7Coefficients5_6	kRegCS7Coefficients5_6		///< @deprecated		Use kRegCS7Coefficients5_6 instead.
	#define	kK2RegCS7Coefficients7_8	kRegCS7Coefficients7_8		///< @deprecated		Use kRegCS7Coefficients7_8 instead.
	#define	kK2RegCS7Coefficients9_10	kRegCS7Coefficients9_10		///< @deprecated		Use kRegCS7Coefficients9_10 instead.
	#define	kK2RegCS8Coefficients1_2	kRegCS8Coefficients1_2		///< @deprecated		Use kRegCS8Coefficients1_2 instead.
	#define	kK2RegCS8Coefficients3_4	kRegCS8Coefficients3_4		///< @deprecated		Use kRegCS8Coefficients3_4 instead.
	#define	kK2RegCS8Coefficients5_6	kRegCS8Coefficients5_6		///< @deprecated		Use kRegCS8Coefficients5_6 instead.
	#define	kK2RegCS8Coefficients7_8	kRegCS8Coefficients7_8		///< @deprecated		Use kRegCS8Coefficients7_8 instead.
	#define	kK2RegCS8Coefficients9_10	kRegCS8Coefficients9_10		///< @deprecated		Use kRegCS8Coefficients9_10 instead.
	#define	kK2RegSDIOut6Control		kRegSDIOut6Control			///< @deprecated		Use kRegSDIOut6Control instead.
	#define	kK2RegSDIOut7Control		kRegSDIOut7Control			///< @deprecated		Use kRegSDIOut7Control instead.
	#define	kK2RegSDIOut8Control		kRegSDIOut8Control			///< @deprecated		Use kRegSDIOut8Control instead.


	// Special Registers for the XENAX
	#define XENAX_REG_START 256
	#define XENAX_NUM_REGS  (119+36+41)
	#define NUM_HW_REGS     (XENAX_REG_START + XENAX_NUM_REGS)

	// Registers unique to a particular device with their own address space
	// These are 16-bit registers
	#define BORG_FUSION_REG_START 8000

	typedef enum
	{
		kRegBorgFusionBootFPGAVerBoardID = BORG_FUSION_REG_START, // 0
		kRegBorgFusionFPGAConfigCtrl,			// 1
		kRegBorgFusionPanelPushButtonDebounced,	// 2
		kRegBorgFusionPanelPushButtonChanges,	// 3
		kRegBorgFusionLEDPWMThreshholds,		// 4
		kRegBorgFusionPowerCtrl,				// 5
		kRegBorgFusionIRQ3nIntCtrl,				// 6
		kRegBorgFusionIRQ3nIntSrc,				// 7
		kRegBorgFusionDisplayCtrlStatus,		// 8
		kRegBorgFusionDisplayWriteData,			// 9
		kRegBorgFusionAnalogFlags,				// 10
		kRegBorgFusionReserved11,				// 11 
		kRegBonesActelCFSlots = kRegBorgFusionReserved11,			// Compact Flash S1 & S2 status
		kRegBorgFusionReserved12,				// 12
		kRegBonesActelCFSlotsChanges = kRegBorgFusionReserved12,	// Compact Flash Slot Changes Present
		kRegBorgFusionReserved13,				// 13
		kRegBorgFusionReserved14,				// 14
		kRegBorgFusionReserved15,				// 15
		kRegBorgFusionTempSensor1,				// 16
		kRegBorgFusionTempSensor2,				// 17
		kRegBorgFusion5_0vPowerRail,			// 18: +5.0 v power rail, 8mV res
		kRegBorgFusion2_5vPowerRail,			// 19: +2.5v power rail, 4mV res
		kRegBorgFusion1_95vPowerRail,			// 20: +1.95v power rail, 4mV res
		kRegBorgFusion1_8vPowerRail,			// 21: +1.8v power rail, 2mV res
		kRegBorgFusion1_5vPowerRail,			// 22: +1.5v power rail, 2mV res
		kRegBorgFusion1_0vPowerRail,			// 23: +1.0v power rail, 2mV res
		kRegBorgFusion12vPowerRail,				// 24: +12v input power, 8mV res
		kRegBorgFusion3_3vPowerRail,			// 25: +3.3v power rail, 4mV res

		kRegBorgFusionProdIdLo = BORG_FUSION_REG_START + 50,// 50, lo 16b of kRegBoardID data
		kRegBorgFusionProdIdHi,								// 51, hi 16b of kRegBoardID data
		kRegBorgFusionNumRegistersDummy,
		kRegBorgFusionNumRegisters = kRegBorgFusionNumRegistersDummy - BORG_FUSION_REG_START + 1
	} BorgFusionRegisterNum;

	#define DNX_REG_START 8100

	typedef enum
	{
		// The paragraph numbers following the Block and Section register
		// descriptions refer to the "KiProMini: DNX Codec" document
		// originally written by Avid and adapted by Paul Greaves.

		// Block 1.1: Nestor Common
		kRegDNX_DeviceID = DNX_REG_START,       // ro (read only)
		kRegDNX_Revision,                       // ro
		kRegDNX_Diagnostics,
		kRegDNX_Reset,
		kRegDNX_NestorControl,
		kRegDNX_NestorAutoloadControl,
		kRegDNX_Status,
		kRegDNX_NestorInterrupt,
		kRegDNX_NestorInterruptEnable,
		kRegDNX_EncoderM1AStatus,               // ro (Was Flash Addr)
		kRegDNX_EncoderM2AStatus,               // ro (Was Flash Data)
		kRegDNX_DecoderM1BStatus,               // ro (Was Temp Sensor Addr Ctl)
		kRegDNX_DecoderM2BStatus,               // ro (Was Temp Sensor Wr Ctl)
		kRegDNX_TempSensorReadData,             // ro (Unused)
		kRegDNX_Timeout,

		// Block 1.2: SMPTE Format
		kRegDNX_Field1StartEnd,
		kRegDNX_Field1ActiveStartEnd,
		kRegDNX_Field2StartEnd,
		kRegDNX_Field2ActiveStartEnd,
		kRegDNX_HorizontalStartEnd,
		kRegDNX_FormatChromaClipping,
		kRegDNX_FormatLumaClipping,
		kRegDNX_Reserved1,

		// Section 1.2.9: Formatter
		kRegDNX_A0Parameter,
		kRegDNX_A1Parameter,
		kRegDNX_A2Parameter,
		kRegDNX_A3Parameter,
		kRegDNX_A4Parameter,
		kRegDNX_A5Parameter,
		kRegDNX_A6Parameter,
		kRegDNX_A7Parameter,
		kRegDNX_A8Parameter,
		kRegDNX_A9Parameter,
		kRegDNX_TOFHorizontalResetValue,
		kRegDNX_TOFVerticalResetValue,
		kRegDNX_HorizontalStartOfHANCCode,
		kRegDNX_HorizontalStartOfSAVCode,
		kRegDNX_HorizontalStartOfActiveVideo,
		kRegDNX_HorizontalEndOfLine,

		// Block 1.5: Encoder
		kRegDNX_EncoderInterrupt,
		kRegDNX_EncoderControl,
		kRegDNX_EncoderInterruptEnable,
		kRegDNX_RateControlAddress,
		kRegDNX_RateControlReadData,            // ro
		kRegDNX_MacroblockLineNumber,           // ro
		kRegDNX_RateControlIndex,               // ro
		kRegDNX_DCTPackerIndex,                 // ro
		kRegDNX_EncodeTableWrite,
		kRegDNX_EncoderDebug,                   // ro

		// Section 1.5.11: The VCID used for encoding
		kRegDNX_EncoderVCIDRegister,

		// Section 1.5.12.1/2: Encoder Parameter RAM
		// We are only supporting seven VCID
		// types at this time.
		kRegDNX_EncoderParameterRAMLocation0_0,
		kRegDNX_EncoderParameterRAMLocation1_0,

		kRegDNX_EncoderParameterRAMLocation0_1,
		kRegDNX_EncoderParameterRAMLocation1_1,

		kRegDNX_EncoderParameterRAMLocation0_2,
		kRegDNX_EncoderParameterRAMLocation1_2,

		kRegDNX_EncoderParameterRAMLocation0_3,
		kRegDNX_EncoderParameterRAMLocation1_3,

		kRegDNX_EncoderParameterRAMLocation0_4,
		kRegDNX_EncoderParameterRAMLocation1_4,

		kRegDNX_EncoderParameterRAMLocation0_5,
		kRegDNX_EncoderParameterRAMLocation1_5,

		kRegDNX_EncoderParameterRAMLocation0_6,
		kRegDNX_EncoderParameterRAMLocation1_6,

		// Block 1.6: Decoder
		kRegDNX_DecoderInterrupt,
		kRegDNX_DecoderControl,
		kRegDNX_DecoderInterruptEnable,
		kRegDNX_DecodeTime,                     // ro
		kRegDNX_FrameCount,                     // ro
		kRegDNX_DecodeTableWrite,
		kRegDNX_DecoderDebug,                   // ro
		kRegDNX_Pipe1StallStatus1,              // ro
		kRegDNX_Pipe1StallStatus2,              // ro
		kRegDNX_Pipe2StallStatus1,              // ro
		kRegDNX_Pipe2StallStatus2,              // ro

		// Section 1.6.12: The VCID to use regardless of the frame header
		kRegDNX_DecoderVCIDRegister,

		// Section 1.6.13: Decoder Parameter RAM
		kRegDNX_DecoderParameterRAMLocation0_0,
		kRegDNX_DecoderParameterRAMLocation1_0,

		kRegDNX_DecoderParameterRAMLocation0_1,
		kRegDNX_DecoderParameterRAMLocation1_1,

		kRegDNX_DecoderParameterRAMLocation0_2,
		kRegDNX_DecoderParameterRAMLocation1_2,

		kRegDNX_DecoderParameterRAMLocation0_3,
		kRegDNX_DecoderParameterRAMLocation1_3,

		kRegDNX_DecoderParameterRAMLocation0_4,
		kRegDNX_DecoderParameterRAMLocation1_4,

		kRegDNX_DecoderParameterRAMLocation0_5,
		kRegDNX_DecoderParameterRAMLocation1_5,

		kRegDNX_DecoderParameterRAMLocation0_6,
		kRegDNX_DecoderParameterRAMLocation1_6,

		kRegDNX_MaximumRegister   = kRegDNX_DecoderParameterRAMLocation1_6,
		kRegDNX_NumberOfRegisters = ((kRegDNX_MaximumRegister - DNX_REG_START) + 1)
	} DNXRegisterNum;
#endif	//	!defined (NTV2_DEPRECATE)
#if defined (NTV2_DEPRECATE_13_0)
	#define	kRegAuxInterruptDelay	kRegBitfileDate			///< @deprecated		Use kRegBitfileDate instead.
	#define	kRegReserved89			kRegBitfileTime			///< @deprecated		Use kRegBitfileTime instead.
#endif	//	NTV2_DEPRECATE_13_0

// Virtual registers
#include "ntv2virtualregisters.h"

typedef struct
{
	ULWord initialized;
	ULWord contrast;
	ULWord brightness;
	ULWord hue;
	ULWord CbOffset;		// Not user controllable
	ULWord CrOffset;		// Not user controllable
	ULWord saturationCb;
	ULWord saturationCr;
} VirtualProcAmpRegisters_base;

typedef struct
{
	VirtualProcAmpRegisters_base SD;
	VirtualProcAmpRegisters_base HD;
} VirtualProcAmpRegisters;

// These have a nice mapping to the virtual registers
typedef struct
{
	UByte contrast;
	UByte brightness;
	UByte hue;
	UByte CbOffset;		// Not user controllable
	UByte CrOffset;		// Not user controllable
	UByte saturationCb;
	UByte saturationCr;
} ADV7189BProcAmpRegisters;	// Works for SD portion of ADV7402A also

// These do not have a nice mapping to the virtual registers
// All are 10-bit registers spread out over two I2C addresses.
typedef struct
{
	UByte hex73;	// [7:6] set, [5:0] upper bits contrast
	UByte hex74;	// [7:4] lower bits contrast, [3:0] upper bits saturation Cb
	UByte hex75;	// [7:2] lower bits saturation Cb, [1:0] upper bits saturation Cr
	UByte hex76;	// [7:0] lower bits saturation Cr
	UByte hex77;	// [7:6] clear, [5:0] upper bits brightness.
	UByte hex78;	// [7:4] lower bits brightness, [3:0] high bits Cb offset
	UByte hex79;	// [7:2] lower bits Cb offset, [1:0] high bits Cr offset
	UByte hex7A;	// [7:0] lower bits Cr offset
} ADV7402AHDProcAmpRegisters;

// Kind of a hack
// This will have to be a union or something if/when we add another proc amp processor
typedef struct
{
	ADV7189BProcAmpRegisters	SD;	// Works for SD portion of ADV7402A also
	ADV7402AHDProcAmpRegisters	HD;	
} HardwareProcAmpRegisterImage;

// SD procamp regs
typedef enum
{
	kRegADV7189BContrast = 0x08,
	kRegADV7189BBrightness = 0x0A,
	kRegADV7189BHue = 0x0B,
	kRegADV7189BCbOffset = 0xE1,
	kRegADV7189BCrOffset = 0xE2,
	kRegADV7189BSaturationCb = 0xE3,
	kRegADV7189BSaturationCr = 0xE4
} ADV7189BRegisterNum;	// Works for SD portion of ADV7402A also

typedef enum
{
	// Global Control
	kRegMaskFrameRate			= BIT(0) + BIT(1) + BIT(2),
	kRegMaskFrameRateHiBit		= BIT(22),
	kRegMaskGeometry			= BIT(3) + BIT(4) + BIT(5) + BIT(6),
	kRegMaskStandard			= BIT(7) + BIT(8) + BIT(9),
	kRegMaskRefSource			= BIT(10) + BIT(11) + BIT(12),
	kRegMaskRefInputVoltage		= BIT(12),	// DEPRECATED! - Now part of kRegMaskRefSource - do not use on new boards
	kRegMaskSmpte372Enable		= BIT(15),
	kRegMaskLED					= BIT(16) + BIT(17) + BIT(18) + BIT(19),
	kRegMaskRegClocking			= BIT(21) + BIT(20),
	kRegMaskDualLinkInEnable	= BIT(23),
	kRegMaskQuadTsiEnable		= BIT(24),
	kRegMaskBankSelect			= BIT(25) + BIT(26),
	kRegMaskDualLinkOutEnable	= BIT(27),
	kRegMaskRP188ModeCh1		= BIT(28),
	kRegMaskRP188ModeCh2		= BIT(29),
	kRegMaskCCHostBankSelect	= BIT(30) + BIT(31),

	// Global Control 2
	kRegMaskRefSource2			= BIT(0),
	kRegMaskPCRReferenceEnable	= BIT(1),
	kRegMaskQuadMode			= BIT(3),
	kRegMaskAud1PlayCapMode		= BIT(4),
	kRegMaskAud2PlayCapMode		= BIT(5),
	kRegMaskAud3PlayCapMode		= BIT(6),
	kRegMaskAud4PlayCapMode		= BIT(7),
	kRegMaskAud5PlayCapMode		= BIT(8),
	kRegMaskAud6PlayCapMode		= BIT(9),
	kRegMaskAud7PlayCapMode		= BIT(10),
	kRegMaskAud8PlayCapMode		= BIT(11),
	kRegMaskQuadMode2			= BIT(12),
	kRegMaskSmpte372Enable4		= BIT(13),
	kRegMaskSmpte372Enable6		= BIT(14),
	kRegMaskSmpte372Enable8		= BIT(15),
	kRegMaskIndependentMode		= BIT(16),
	kRegMask2MFrameSupport		= BIT(17),
	kRegMaskAudioMixerPresent	= BIT(18),
	kRegMaskIsDNXIV				= BIT(19),
	kRegMask425FB12				= BIT(20),
	kRegMask425FB34				= BIT(21),
	kRegMask425FB56				= BIT(22),
	kRegMask425FB78				= BIT(23),
	kRegMaskRP188ModeCh3		= BIT(28),
	kRegMaskRP188ModeCh4		= BIT(29),
	kRegMaskRP188ModeCh5		= BIT(30),
	kRegMaskRP188ModeCh6		= BIT(31),
	kRegMaskRP188ModeCh7		= BIT(26),
	kRegMaskRP188ModeCh8		= BIT(27),

	// kRegGlobalControl3	(reg 108)
	kRegMaskAnalogIOControl			= BIT(1) + BIT(0),
	kRegMaskAnalogIOControl_14	= BIT(0),
	kRegMaskAnalogIOControl_58	= BIT(1),
	kRegMaskQuadQuadMode		= BIT(2),
	kRegMaskQuadQuadMode2		= BIT(3),
	kRegMaskQuadQuadSquaresMode	= BIT(4),
	kRegMaskVUMeterSelect		= BIT(5),
	kRegMaskFramePulseEnable	= BIT(6),
	kRegMaskFramePulseRefSelect	= BIT(8) + BIT(9) + BIT(10) + BIT(11),

	// Audio Control 2
	kRegMaskAud1RateHigh		= BIT(0),
	kRegMaskAud2RateHigh		= BIT(1),
	kRegMaskAud3RateHigh		= BIT(2),
	kRegMaskAud4RateHigh		= BIT(3),
	kRegMaskAud5RateHigh		= BIT(4),
	kRegMaskAud6RateHigh		= BIT(5),
	kRegMaskAud7RateHigh		= BIT(6),
	kRegMaskAud8RateHigh		= BIT(7),

	// Channel Control - kRegCh1Control, kRegCh2Control, kRegCh3Control, kRegCh4Control
	kRegMaskMode				= BIT(0),
	kRegMaskFrameFormat			= BIT(1) + BIT(2) + BIT(3) + BIT(4),
	kRegMaskAlphaFromInput2		= BIT(5),
	kRegMaskFrameFormatHiBit	= BIT(6), // KAM
	kRegMaskChannelDisable		= BIT(7),
	kRegMaskWriteBack			= BIT(8),
	kRegMaskFrameOrientation	= BIT(10),
	kRegMaskQuarterSizeMode		= BIT(11),
	kRegMaskFrameBufferMode		= BIT(12),
	kKHRegMaskDownconvertInput	= BIT(14),
	kLSRegMaskVideoInputSelect	= BIT(15),
	kRegMaskDitherOn8BitInput	= BIT(16),
	kRegMaskQuality				= BIT(17),
	kRegMaskEncodeAsPSF			= BIT(18),
	kK2RegMaskFrameSize			= BIT(21) + BIT(20),
	kRegMaskChannelCompressed	= BIT(22),
	kRegMaskRGB8b10bCvtMode		= BIT(23),
	kRegMaskVBlankRGBRange		= BIT(24),
	kRegMaskQuality2			= BIT(25) + BIT(26),
	kRegCh1BlackOutputMask		= BIT(27),		// KiPro black output bit
	kRegMaskSonySRExpressBit	= BIT(28),
	kRegMaskFrameSizeSetBySW	= BIT(29),
	kRegMaskVidProcVANCShift	= BIT(31),

	// Video Crosspoint Control
	kRegMaskVidXptFGVideo			= BIT(0) + BIT(1) + BIT(2),
	kRegMaskVidXptBGVideo			= BIT(4) + BIT(5) + BIT(6),
	kRegMaskVidXptFGKey				= BIT(8) + BIT(9) + BIT(10),
	kRegMaskVidXptBGKey				= BIT(12) + BIT(13) + BIT(14),
	kRegMaskVidXptSecVideo			= BIT(16) + BIT(17) + BIT(18),
	
	// Video Processing Control
	kRegMaskVidProcMux1				= BIT(0) + BIT(1),
	kRegMaskVidProcMux2				= BIT(2) + BIT(3),
	kRegMaskVidProcMux3				= BIT(4) + BIT(5),
	kRegMaskVidProcMux4				= BIT(6) + BIT(7),
	kRegMaskVidProcMux5				= BIT(8) + BIT(9) + BIT(10),
	kRegMaskVidProcLimiting			= BIT(11) + BIT(12),
	kRegMaskVidProcVancSource		= BIT(13),
	kRegMaskVidProcRGBRange			= BIT(14),
	kRegMaskVidProcFGMatteEnable	= BIT(18),
	kRegMaskVidProcBGMatteEnable	= BIT(19),
	kRegMaskVidProcFGControl		= BIT(20) + BIT(21),
	kRegMaskVidProcBGControl		= BIT(22) + BIT(23),
	kRegMaskVidProcMode				= BIT(24) + BIT(25),
	kRegMaskVidProcRGBModeSupported	= BIT(26),
	kRegMaskVidProcSyncFail			= BIT(27),
	kRegMaskVidProcSplitStd			= BIT(28) + BIT(29) + BIT(30),
	kRegMaskVidProcSubtitleEnable	= BIT(31),
	
	// kRegStatus
	kRegMaskHardwareVersion			= BIT(0) + BIT(1) + BIT(2) + BIT(3),
	kRegMaskFPGAVersion				= BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11),
	kRegMaskLTCInPresent			= BIT(17),

	// Video Interrupt Control
	kRegMaskIntEnableMask	= BIT(5) + BIT(4) + BIT(3) + BIT(2) + BIT(1) + BIT(0),

	// Audio Control
	kRegMaskCaptureEnable			= BIT(0),
	kRegMaskNumBits					= BIT(1),	// shouldn't this be BIT(2)?
	kRegMaskOutputTone				= BIT(1),
	kRegMask20BitMode				= BIT(2),
	kRegMaskLoopBack				= BIT(3),
	kRegMaskAudioTone				= BIT(7),
	kRegMaskResetAudioInput			= BIT(8),
	kRegMaskResetAudioOutput		= BIT(9),
	kRegMaskInputStartAtVBI			= BIT(10),	// New in 15.6
	kRegMaskPauseAudio				= BIT(11),
	kRegMaskEmbeddedOutputMuteCh1   = BIT(12), // added for FS1
	kRegMaskEmbeddedOutputSupressCh1 = BIT(13), // added for FS1 but available on other boards
	kRegMaskOutputStartAtVBI		= BIT(14),	// New in 15.6
	kRegMaskEmbeddedOutputSupressCh2 = BIT(15), // added for FS1 but available on other boards
	kRegMaskNumChannels				= BIT(16),
	kRegMaskEmbeddedOutputMuteCh2   = BIT(17), // added for FS1
	kRegMaskAudioRate				= BIT(18),
	kRegMaskEncodedAudioMode		= BIT(19), // addded for FS1 but available on other boards
	kRegMaskAudio16Channel			= BIT(20),
	kRegMaskAudio8Channel			= BIT(23),
	kK2RegMaskKBoxAnalogMonitor		= BIT(24) + BIT(25),
	kK2RegMaskKBoxAudioInputSelect	= BIT(26),
	kK2RegMaskKBoxDetect			= BIT(27),
	kK2RegMaskBOCableDetect			= BIT(28),
	kK2RegMaskAudioLevel			= BIT(29),
	kFS1RegMaskAudioLevel			= BIT(29)+BIT(30),
	kK2RegResetAudioDAC				= BIT(30),
	kK2RegMaskAudioBufferSize       = BIT(31),
	kK2RegMaskAverageAudioLevel		= 0xFFFFffff,	// read the entire register

	kFS1RegMaskAudioBufferSize		= BIT(31),
	kLHRegMaskResetAudioDAC			= BIT(31),
	// FS1 output control "Freeze (last good frame) On Input Loss"
	kRegMaskLossOfInput				= BIT(11),
	
	// Audio Source Select
	kRegMaskAudioSource				= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kRegMaskEmbeddedAudioInput		= BIT(16),
	kRegMaskAudioAutoErase			= BIT(19),
	kRegMaskAnalogHDMIvsAES     	= BIT(20),
	kRegMask3GbSelect				= BIT(21),
	kRegMaskEmbeddedAudioClock		= BIT(22),
	kRegMaskEmbeddedAudioInput2		= BIT(23),
	kRegMaskAnalogAudioInGain		= BIT(24),
	kRegMaskAnalogAudioInJack		= BIT(25),

	// Input Status
	kRegMaskInput1FrameRate			= BIT(0)+BIT(1)+BIT(2),
	kRegMaskInput1Geometry			= BIT(4)+BIT(5)+BIT(6),
	kRegMaskInput1Progressive		= BIT(7),
	kRegMaskInput2FrameRate			= BIT(8)+BIT(9)+BIT(10),
	kRegMaskInput2Geometry			= BIT(12)+BIT(13)+BIT(14),
	kRegMaskInput2Progressive		= BIT(15),
	kRegMaskReferenceFrameRate		= BIT(16)+BIT(17)+BIT(18)+BIT(19),
	kRegMaskReferenceFrameLines		= BIT(20)+BIT(21)+BIT(22),
	kRegMaskReferenceProgessive		= BIT(23),
	kRegMaskAESCh12Present			= BIT(24),
	kRegMaskAESCh34Present			= BIT(25),
	kRegMaskAESCh56Present			= BIT(26),
	kRegMaskAESCh78Present			= BIT(27),
	kRegMaskInput1FrameRateHigh		= BIT(28),
	kRegMaskInput2FrameRateHigh		= BIT(29),
	kRegMaskInput1GeometryHigh		= BIT(30),
	kRegMaskInput2GeometryHigh		= BIT(31),

	// RP-188 Source
	kRegMaskRP188SourceSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	kRegMaskRP188DBB = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7),
	
    // DMA Control
    kRegMaskForce64         = BIT(4),
    kRegMaskAutodetect64    = BIT(5),
	kRegMaskFirmWareRev		= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kRegMaskDMAPauseDisable = BIT(16),

	// Color Correction Control - kRegCh1ColorCorrectionControl (68), kRegCh2ColorCorrectionControl (69)
	kRegMaskSaturationValue			= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9),
	kRegMaskCCOutputBankSelect		= BIT(16),
	kRegMaskCCMode					= BIT(17)+BIT(18),
	kRegMaskCC5HostAccessBankSelect	= BIT(20),
	kRegMaskCC5OutputBankSelect		= BIT(21),
	kRegMaskLUT5Select				= BIT(28),
	kRegMaskLUTSelect				= BIT(29),
	kRegMaskCC3OutputBankSelect		= BIT(30),
	kRegMaskCC4OutputBankSelect		= BIT(31),

	//	kRegCanDoStatus
	kRegMaskCanDoValidXptROM			= BIT(0),
	kRegMaskCanDoAudioWaitForVBI		= BIT(1),

	//	kRegLUTV2Control
	kRegMaskLUT1Enable					= BIT(0),
	kRegMaskLUT2Enable					= BIT(1),
	kRegMaskLUT3Enable					= BIT(2),
	kRegMaskLUT4Enable					= BIT(3),
	kRegMaskLUT5Enable					= BIT(4),
	kRegMaskLUT6Enable					= BIT(5),
	kRegMaskLUT7Enable					= BIT(6),
	kRegMaskLUT8Enable					= BIT(7),
	kRegMaskLUT1HostAccessBankSelect	= BIT(8),
	kRegMaskLUT2HostAccessBankSelect	= BIT(9),
	kRegMaskLUT3HostAccessBankSelect	= BIT(10),
	kRegMaskLUT4HostAccessBankSelect	= BIT(11),
	kRegMaskLUT5HostAccessBankSelect	= BIT(12),
	kRegMaskLUT6HostAccessBankSelect	= BIT(13),
	kRegMaskLUT7HostAccessBankSelect	= BIT(14),
	kRegMaskLUT8HostAccessBankSelect	= BIT(15),
	kRegMaskLUT1OutputBankSelect		= BIT(16),
	kRegMaskLUT2OutputBankSelect		= BIT(17),
	kRegMaskLUT3OutputBankSelect		= BIT(18),
	kRegMaskLUT4OutputBankSelect		= BIT(19),
	kRegMaskLUT5OutputBankSelect		= BIT(20),
	kRegMaskLUT6OutputBankSelect		= BIT(21),
	kRegMaskLUT7OutputBankSelect		= BIT(22),
	kRegMaskLUT8OutputBankSelect		= BIT(23),
	kRegMask12BitLUTPlaneSelect			= BIT(24)+BIT(25),
	kRegMask12BitLUTSupport				= BIT(28),

	
	// RS422 Control 
	kRegMaskRS422TXEnable   	= BIT(0),
	kRegMaskRS422TXFIFOEmpty   	= BIT(1),
	kRegMaskRS422TXFIFOFull		= BIT(2),
	kRegMaskRS422RXEnable   	= BIT(3),
	kRegMaskRS422RXFIFONotEmpty	= BIT(4),
	kRegMaskRS422RXFIFOFull		= BIT(5),
	kRegMaskRS422RXParityError	= BIT(6),
	kRegMaskRS422RXFIFOOverrun	= BIT(7),
	kRegMaskRS422Flush			= BIT(6)+BIT(7),
	kRegMaskRS422Present		= BIT(8),
	kRegMaskRS422TXInhibit		= BIT(9),
	kRegMaskRS422ParitySense	= BIT(12),		// 0 = Odd, 1 = Even
	kRegMaskRS422ParityDisable	= BIT(13),		// 0 = Use bit 12 setting, 1 = No parity
	kRegMaskRS422BaudRate		= BIT(16)+BIT(17)+BIT(18),


	// FS1 ProcAmp Control
	kFS1RegMaskProcAmpC1Y     = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11),
	kFS1RegMaskProcAmpC1CB    = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27),
	kFS1RegMaskProcAmpC1CR    = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11),
	kFS1RegMaskProcAmpC2CB    = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27),
	kFS1RegMaskProcAmpC2CR    = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11),
	kFS1RegMaskProcAmpOffsetY = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27),


	kRegMaskAudioInDelay	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12),
	kRegMaskAudioOutDelay	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28),

	// FS1 Audio Delay
	kFS1RegMaskAudioDelay		= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12),

	// Borg Audio Delay			
	kBorgRegMaskPlaybackEEAudioDelay = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kBorgRegMaskCaputreAudioDelay = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	

	// kRegOutputTimingControl 
	kBorgRegMaskOutTimingCtrlHorzOfs = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12),
	kBorgRegMaskOutTimingCtrlVertOfs = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28),


	// FS1 I2C
	kFS1RegMaskI2C1ControlWrite	= BIT(0),
	kFS1RegMaskI2C1ControlRead	= BIT(1),
	kFS1RegMaskI2C1ControlBusy	= BIT(2),
	kFS1RegMaskI2C1ControlError	= BIT(3),
	kFS1RegMaskI2C2ControlWrite	= BIT(4),
	kFS1RegMaskI2C2ControlRead	= BIT(5),
	kFS1RegMaskI2C2ControlBusy	= BIT(6),
	kFS1RegMaskI2C2ControlError	= BIT(7),
	
	kFS1RegMaskI2CAddress		= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4) + BIT(5)+BIT(6)+BIT(7),
	kFS1RegMaskI2CSubAddress	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kFS1RegMaskI2CWriteData	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4) + BIT(5)+BIT(6)+BIT(7),
	kFS1RegMaskI2CReadData	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	
	// kRegFS1ReferenceSelect (in Reg 95)
	kRegMaskLTCLoopback = BIT(10),
	kFS1RefMaskLTCOnRefInSelect = BIT(4),
	kRegMaskLTCOnRefInSelect = BIT(5),
	kFS1RefMaskLTCEmbeddedOutEnable = BIT(8),
	kFS1RegMaskReferenceInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3),
	kFS1RegMaskColorFIDSubcarrierReset = BIT(14),
	kFS1RegMaskFreezeOutput = BIT(15),
	kFS1RegMaskProcAmpInputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kFS1RegMaskSecondAnalogOutInputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	// FS1 AFD Mode
	kFS1RegMaskAFDReceived_Code = BIT(3)+BIT(2)+BIT(1)+BIT(0),
	kFS1RegMaskAFDReceived_AR = BIT(4),
	kFS1RegMaskAFDReceived_VANCPresent = BIT(7),
	kFS1RegMaskUpconvertAutoAFDEnable = BIT(20),
	kFS1RegMaskUpconvert2AFDDefaultHoldLast = BIT(21),
	kFS1RegMaskDownconvertAutoAFDEnable = BIT(24),
	kFS1RegMaskDownconvertAFDDefaultHoldLast = BIT(25),
	kFS1RegMaskDownconvert2AutoAFDEnable = BIT(28),
	kFS1RegMaskDownconvert2AFDDefaultHoldLast = BIT(29),

	// FS1 AFD Inserter
	kFS1RegMaskAFDVANCInserter_Code = BIT(3)+BIT(2)+BIT(1)+BIT(0),
	kFS1RegMaskAFDVANCInserter_AR = BIT(4),
	kFS1RegMaskAFDVANCInserter_Mode = BIT(13)+BIT(12),
	kFS1RegMaskAFDVANCInserter_Line = BIT(26)+BIT(25)+BIT(24)+BIT(23)+BIT(22)+BIT(21)+BIT(20)+BIT(19)+BIT(18)+BIT(17)+BIT(16),

    // FS1 Audio Channel Mapping
    kFS1RegMaskAudioChannelMapping_Gain = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4) + BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9),
    kFS1RegMaskAudioChannelMapping_Phase = BIT(15),
    kFS1RegMaskAudioChannelMapping_Source = BIT(21)+BIT(20)+BIT(19)+BIT(18)+BIT(17)+BIT(16),
    kFS1RegMaskAudioChannelMapping_Mute = BIT(31),

    // FS1 Output Timing Fine Phase Adjust
    kRegMaskOutputTimingFinePhase = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4) + BIT(5)+BIT(6)+BIT(7)+BIT(8),
	
	//kRegAnalogInputStatus
	kRegMaskAnalogInputSD = BIT(14),
	kRegMaskAnalogInputIntegerRate = BIT(15),
	kRegMaskAnalogCompositeLocked  = BIT(16),
	kRegMaskAnalogCompositeFormat625  = BIT(18),
	
	//kRegAnalogInputControl  (Note - on some boards, ADC mode is set in Reg 128, kK2RegAnalogOutControl!)
	kRegMaskAnalogInputADCMode  = BIT(4)+BIT(3)+BIT(2)+BIT(1)+BIT(0),
	
	//kRegHDMIOut3DControl
	kRegMaskHDMIOut3DPresent		= BIT(3),
	kRegMaskHDMIOut3DMode			= BIT(4)+BIT(5)+BIT(6)+BIT(7),
	
	//kRegHDMIOutControl
	kRegMaskHDMIOutVideoStd			= BIT(2)+BIT(1)+BIT(0),
	kRegMaskHDMIOutV2VideoStd		= BIT(3)+BIT(2)+BIT(1)+BIT(0),
	kRegMaskHDMIOut8ChGroupSelect	= BIT(5),
	kRegMaskHDMIV2TxBypass			= BIT(7),
	kLHIRegMaskHDMIOutColorSpace	= BIT(8),
	kLHIRegMaskHDMIOutFPS			= BIT(12)+BIT(11)+BIT(10)+BIT(9),
	kRegMaskHDMIOutProgressive		= BIT(13),
	kLHIRegMaskHDMIOutBitDepth		= BIT(14),
	kRegMaskHDMIV2YCColor			= BIT(15),
	kRegMaskHDMIOutAudioFormat		= BIT(17)+BIT(16),
	kRegMaskHDMISampling			= BIT(19)+BIT(18),
	kRegMaskHDMIVOBD				= BIT(21)+BIT(20),
	kRegMaskSourceIsRGB				= BIT(23),
	kRegMaskHDMIOutPowerDown		= BIT(25),
	kRegMaskHDMITxEnable			= BIT(26),
	kRegMaskHDMIRxEnable			= BIT(27),
	kRegMaskHDMIOutRange			= BIT(28),
	kRegMaskHDMIOutAudioCh			= BIT(29),
	kLHIRegMaskHDMIOutDVI			= BIT(30),

	//kRegHDMIInputStatus
	kRegMaskInputStatusLock			= BIT(0),								// rename to kRegMaskAnalogInputStatusLock
	kRegMaskStable					= BIT(1),
	kLHIRegMaskHDMIInputColorSpace	= BIT(2),
	kLHIRegMaskHDMIInputBitDepth	= BIT(3),
	kRegMaskHDMIInV2VideoStd		= BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9),
	kLHIRegMaskHDMIOutputEDIDRGB	= BIT(10),
	kLHIRegMaskHDMIOutputEDID10Bit	= BIT(11),
	kLHIRegMaskHDMIInput2ChAudio	= BIT(12),
	kRegMaskHDMIInputProgressive	= BIT(13),
	kRegMaskVideoSD					= BIT(14),
	kLHIRegMaskHDMIOutputEDIDDVI	= BIT(15),
	kRegMaskHDMIInAudioRate			= BIT(19)+BIT(18)+BIT(17)+BIT(16),
	kRegMaskAudioWorkLength			= BIT(23)+BIT(22)+BIT(21)+BIT(20),
	kRegMaskInputStatusStd			= BIT(24)+BIT(25)+BIT(26),
	kLHIRegMaskHDMIInputProtocol	= BIT(27),
	kRegMaskInputStatusFPS			= BIT(28)+BIT(29)+BIT(30)+BIT(31),
	
	//kRegHDMIInputControl
	kRegMaskHDMIOutForceConfig		= BIT(1),
	kRegMaskHDMIAudioPairSelect		= BIT(3)+BIT(2),
	kRegMaskHDMISampleRateConverterEnable = BIT(4),
	kRegMaskHDMISwapInputAudCh34	= BIT(5),
	kRegMaskHDMISwapOutputAudCh34	= BIT(6),
	kRegMaskHDMIOutPrefer420		= BIT(7),
	kRegMaskHDMIInColorDepth		= BIT(13)+BIT(12),
	kRegMaskHDMIInColorSpace		= BIT(15)+BIT(14),
	kRegMaskHDMIOutAudioRate		= BIT(17)+BIT(16),
	kRegMaskHDMIOutSourceSelect		= BIT(23)+BIT(22)+BIT(21)+BIT(20),
	kRegMaskHDMIOutCropEnable		= BIT(24),
	kRegMaskHDMIOutForceHPD			= BIT(25),
	kRegMaskHDMIOut12Bit			= BIT(26),
	kRegMaskHDMIDebug				= BIT(27),
	kRegMaskHDMIInputRange			= BIT(28),
	kRegMaskHDMIOutAudio2ChannelSelect	= BIT(30)+BIT(29),
	kRegMaskHDMIInfoRange			= BIT(31),
	
	//kRegHDMIInputControl / kRegHDMIOutControl
	kRegMaskHDMIColorSpace			= BIT(4)+BIT(5),
	kRegMaskHDMIPolarity			= BIT(16)+BIT(17)+BIT(18)+BIT(19),

	//kK2RegAnalogOutControl - (controls Analog Inputs also, for some boards)
	kK2RegMaskVideoDACMode		= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4),
	kFS1RegMaskVideoDAC2Mode    = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12),
	kLHIRegMaskVideoDACStandard = BIT(13)+BIT(14)+BIT(15),
	kLSRegMaskVideoADCMode		= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20),
	kLHIRegMaskVideoDACMode		= BIT(21)+BIT(22)+BIT(23)+BIT(24),
	kLHIRegMaskVideoDACSetup	= BIT(21),							// bitwise interpretation of kLHIRegMaskVideoDACMode
	kLHIRegMaskVideoDACJapan	= BIT(22),							// bitwise interpretation of kLHIRegMaskVideoDACMode
	kLHIRegMaskVideoDACRGB		= BIT(23),							// bitwise interpretation of kLHIRegMaskVideoDACMode
	kLHIRegMaskVideoDACComponent = BIT(24),							// bitwise interpretation of kLHIRegMaskVideoDACMode
	kK2RegMaskOutHTiming		= BIT(31)+BIT(30)+BIT(29)+BIT(28)+BIT(27)+BIT(26)+BIT(25)+BIT(24),

	//kK2RegSDIOut1Control + kK2RegSDIOut2Control + kK2RegSDIOut3Control + kK2RegSDIOut4Control + kK2RegAnalogOutControl
	kK2RegMaskSDIOutStandard  = BIT(0)+BIT(1)+BIT(2),
	kK2RegMaskSDI1Out_2Kx1080Mode  = BIT(3),
	kLHRegMaskVideoOutputDigitalSelect = BIT(4) + BIT(5),
	kK2RegMaskSDIOutHBlankRGBRange  = BIT(7),
	kLHRegMaskVideoOutputAnalogSelect = BIT(8) + BIT(9),
	kRegMaskSDIOut6GbpsMode = BIT(16),
	kRegMaskSDIOut12GbpsMode = BIT(17),
	kRegMaskRGBLevelA = BIT(22),
	kRegMaskSDIOutLevelAtoLevelB = BIT(23),
	kLHIRegMaskSDIOut3GbpsMode = BIT(24),
	kLHIRegMaskSDIOutSMPTELevelBMode = BIT(25),
	kK2RegMaskVPIDInsertionEnable = BIT(26),
	kK2RegMaskVPIDInsertionOverwrite = BIT(27),	
	kK2RegMaskSDIOutDS1AudioSelect = BIT(28)+BIT(30),
	kK2RegMaskSDIOutDS2AudioSelect = BIT(29)+BIT(31),

	//kK2RegConversionControl and kK2Reg2ndConversionControl,
	kK2RegMaskConverterOutStandard = BIT(12)+BIT(13)+BIT(14),
	kK2RegMaskConverterOutRate = BIT(27)+BIT(28)+BIT(29)+BIT(30),
	kK2RegMaskUpConvertMode   = BIT(8)+BIT(9)+BIT(10),
	kK2RegMaskDownConvertMode   = BIT(4)+BIT(5),
	kK2RegMaskConverterInStandard = BIT(0)+BIT(1)+BIT(2),
	kK2RegMaskConverterInRate = BIT(23)+BIT(24)+BIT(25)+BIT(26),
	kK2RegMaskConverterPulldown = BIT(6),
	kK2RegMaskUCPassLine21 = BIT(16)+BIT(17),
	kK2RegMaskIsoConvertMode = BIT(20)+BIT(21)+BIT(22),
	kK2RegMaskDeinterlaceMode = BIT(15),
	kK2RegMaskEnableConverter = BIT(31),

	//kK2RegFrameSync1Control and kK2RegFrameSync2Control
	kK2RegMaskFrameSyncControlFrameDelay = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29),
	kK2RegMaskFrameSyncControlStandard = BIT(8)+BIT(9)+BIT(10),
	kK2RegMaskFrameSyncControlGeometry = BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskFrameSyncControlFrameFormat = BIT(0)+BIT(1)+BIT(2)+BIT(3),
	
	//kK2RegXptSelectGroup1
	kK2RegMaskCompressionModInputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	kK2RegMaskConversionModInputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskColorSpaceConverterInputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15), 
	kK2RegMaskXptLUTInputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7), 

	//kK2RegXptSelectGroup2
	kK2RegMaskDuallinkOutInputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	kK2RegMaskFrameSync2InputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskFrameSync1InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskFrameBuffer1InputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7), 

	//kK2RegXptSelectGroup3
	kK2RegMaskCSC1KeyInputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	kK2RegMaskSDIOut2InputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskSDIOut1InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskAnalogOutInputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),

	//kK2RegXptSelectGroup4
	kK2RegMaskMixerBGKeyInputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	kK2RegMaskMixerBGVidInputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskMixerFGKeyInputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskMixerFGVidInputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	
	//kK2RegXptSelectGroup5
	kK2RegMaskCSC2KeyInputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	kK2RegMaskCSC2VidInputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskXptLUT2InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskFrameBuffer2InputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	
	//kK2RegXptSelectGroup6
	kK2RegMaskWaterMarkerInputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskIICTInputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskHDMIOutInputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskSecondConverterInputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	
	//kK2RegXptSelectGroup7
	kK2RegMaskWaterMarker2InputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskIICT2InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskDuallinkOut2InputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),

	//kK2RegXptSelectGroup8
	kK2RegMaskSDIOut3InputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskSDIOut4InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskSDIOut5InputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	
	//kRegCh1ControlExtended
	//kRegCh2ControlExtended
	kK2RegMaskPulldownMode = BIT(2),

	//kK2RegXptSelectGroup9
	kK2RegMaskMixer2FGVidInputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskMixer2FGKeyInputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskMixer2BGVidInputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskMixer2BGKeyInputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kK2RegXptSelectGroup10
	kK2RegMaskSDIOut1DS2InputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskSDIOut2DS2InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	
	//kK2RegXptSelectGroup11
	kK2RegMaskDuallinkIn1InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskDuallinkIn1DSInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskDuallinkIn2InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskDuallinkIn2DSInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kK2RegXptSelectGroup12
	kK2RegMaskXptLUT3InputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskXptLUT4InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskXptLUT5InputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskXpt3DLUT1InputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kK2RegXptSelectGroup13
	kK2RegMaskFrameBuffer3InputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskFrameBuffer4InputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),

	//kK2RegXptSelectGroup14
	kK2RegMaskSDIOut3DS2InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskSDIOut5DS2InputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskSDIOut4DS2InputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup15
	kK2RegMaskDuallinkIn3InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskDuallinkIn3DSInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskDuallinkIn4InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskDuallinkIn4DSInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup16
	kK2RegMaskDuallinkOut3InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskDuallinkOut4InputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskDuallinkOut5InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),

	//kK2RegXptSelectGroup17
	kK2RegMaskCSC3VidInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskCSC3KeyInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskCSC4VidInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskCSC4KeyInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup18
	kK2RegMaskCSC5VidInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskCSC5KeyInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

	//kRegXptSelectGroup19
	kK2RegMask4KDCQ1InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMask4KDCQ2InputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMask4KDCQ3InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMask4KDCQ4InputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup20
	kK2RegMaskHDMIOutV2Q1InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskHDMIOutV2Q2InputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskHDMIOutV2Q3InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskHDMIOutV2Q4InputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup21
	kK2RegMaskFrameBuffer5InputSelect = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskFrameBuffer6InputSelect = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskFrameBuffer7InputSelect = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskFrameBuffer8InputSelect = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup22
	kK2RegMaskSDIOut6InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskSDIOut6DS2InputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskSDIOut7InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskSDIOut7DS2InputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup23
	kK2RegMaskCSC7VidInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskCSC7KeyInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskCSC8VidInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskCSC8KeyInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup24
	kK2RegMaskXptLUT6InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskXptLUT7InputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskXptLUT8InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),

	//kRegXptSelectGroup25
	kK2RegMaskDuallinkIn5InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskDuallinkIn5DSInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskDuallinkIn6InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskDuallinkIn6DSInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup26
	kK2RegMaskDuallinkIn7InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskDuallinkIn7DSInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskDuallinkIn8InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskDuallinkIn8DSInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup27
	kK2RegMaskDuallinkOut6InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskDuallinkOut7InputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskDuallinkOut8InputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),

	//kRegXptSelectGroup28
	kK2RegMaskMixer3FGVidInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskMixer3FGKeyInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskMixer3BGVidInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskMixer3BGKeyInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup29
	kK2RegMaskMixer4FGVidInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskMixer4FGKeyInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskMixer4BGVidInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskMixer4BGKeyInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup30
	kK2RegMaskSDIOut8InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskSDIOut8DS2InputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskCSC6VidInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskCSC6KeyInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup32
	kK2RegMask425Mux1AInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMask425Mux1BInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMask425Mux2AInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMask425Mux2BInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup33
	kK2RegMask425Mux3AInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMask425Mux3BInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMask425Mux4AInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMask425Mux4BInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup34
	kK2RegMaskFrameBuffer1BInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskFrameBuffer2BInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskFrameBuffer3BInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskFrameBuffer4BInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup35
	kK2RegMaskFrameBuffer5BInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskFrameBuffer6BInputSelect	= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kK2RegMaskFrameBuffer7BInputSelect	= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kK2RegMaskFrameBuffer8BInputSelect	= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

	//kRegXptSelectGroup36
	kK2RegMaskMultiLinkOutInputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kK2RegMaskMultiLinkOutDS2InputSelect	= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),

	//kK2RegCSCoefficients1_2
	//kK2RegCSC2oefficients1_2,
	//kK2RegCSC3oefficients1_2,
	//kK2RegCSC4oefficients1_2,
	//kK2RegCSC5Coefficients1_2,
	//kK2RegCSC6Coefficients1_2,
	//kK2RegCSC7Coefficients1_2,
	//kK2RegCSC8Coefficients1_2,
	kK2RegMaskVidKeySyncStatus       = BIT(28),
	kK2RegMaskMakeAlphaFromKeySelect = BIT(29),
	kK2RegMaskColorSpaceMatrixSelect = BIT(30),
	kK2RegMaskUseCustomCoefSelect    = BIT(31),

	//kRegCSCoefficients3_4,
	//kRegCS2Coefficients3_4,
	//kRegCS3Coefficients3_4,
	//kRegCS4Coefficients3_4,
	//kRegCS5Coefficients3_4,
	//kRegCS6Coefficients3_4,
	//kRegCS7Coefficients3_4,
	//kRegCS8Coefficients3_4,
	kK2RegMaskXena2RGBRange			= BIT(31),	

	//kK2RegCSCoefficients5_6,
	kK2RegMask2piCSC1				= BIT(28),

	//kK2RegCSCoefficients5_6,
	kK2RegMask2piCSC5				= BIT(28),

	//custom coefficients
	kK2RegMaskCustomCoefficientLow = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10),
	kK2RegMaskCustomCoefficientHigh = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26),

	// Enhanced Color Space Converter contol
	kK2RegMaskEnhancedCSCInputPixelFormat   = BIT(0)+BIT(1),
	kK2RegMaskEnhancedCSCOutputPixelFormat  = BIT(4)+BIT(5),
	kK2RegMaskEnhancedCSCChromaEdgeControl  = BIT(8)+BIT(9),
	kK2RegMaskEnhancedCSCChromaFilterSelect = BIT(12)+BIT(13),
	kK2RegMaskEnhancedCSCEnable             = BIT(29),
	kK2RegMaskEnhancedCSC4KMode             = BIT(28),

	kK2RegMaskEnhancedCSCKeySource          = BIT(0)+BIT(1),
	kK2RegMaskEnhancedCSCKeyOutputRange     = BIT(4),

	// Xena2K and Konax video processing
	kK2RegMaskXena2FgVidProcInputControl = BIT(20)+BIT(21),
	kK2RegMaskXena2BgVidProcInputControl = BIT(22)+BIT(23),
	kK2RegMaskXena2VidProcMode       	 = BIT(24)+BIT(25),
	kK2RegMaskXena2VidProcSplitStd		 = BIT(28) + BIT(29) + BIT(30),

	// 12(13 with sign) bit custom coefficients - backwards compatible with the 10(11 with sign) bit soft and hardware - jac
	kK2RegMaskCustomCoefficient12BitLow  = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12),
	kK2RegMaskCustomCoefficient12BitHigh = BIT(14)+BIT(15)+BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26),
	
	//kRegLTCStatusControl	(see register 233 -- kRegLTCStatusControl)
	kRegMaskLTC1InPresent	= BIT(0),
	kRegMaskLTC1InBypass = BIT(4),
	kRegMaskLTC2InPresent	= BIT(8),
	kRegMaskLTC2InBypass = BIT(12),

	// kRegAudioOutputSourceMap
	kRegMaskMonitorSource				= BIT(21)+BIT(20)+BIT(19)+BIT(18)+BIT(17)+BIT(16),
	kRegMaskHDMIOutAudioSource			= BIT(31)+BIT(30)+BIT(29)+BIT(28)+BIT(27)+BIT(26)+BIT(25)+BIT(24),

	// kRegSDIInput3GStatus
	kRegMaskSDIIn3GbpsMode = BIT(0),
	kRegMaskSDIIn3GbpsSMPTELevelBMode = BIT(1),
	kRegMaskSDIIn1LevelBtoLevelA = BIT(2),
	kRegMaskSDIIn16GbpsMode = BIT(6),
	kRegMaskSDIIn112GbpsMode = BIT(7),
	kRegMaskSDIInVPIDLinkAValid = BIT(4),
	kRegMaskSDIInVPIDLinkBValid = BIT(5),
	kRegMaskSDIIn23GbpsMode = BIT(8),
	kRegMaskSDIIn23GbpsSMPTELevelBMode = BIT(9),
	kRegMaskSDIIn2LevelBtoLevelA = BIT(10),
	kRegMaskSDIIn2VPIDLinkAValid = BIT(12),
	kRegMaskSDIIn2VPIDLinkBValid = BIT(13),
	kRegMaskSDIIn26GbpsMode = BIT(14),
	kRegMaskSDIIn212GbpsMode = BIT(15),
	kRegMaskSDIInTsiMuxSyncFail = BIT(16)+BIT(17)+BIT(18)+BIT(19),


	// kRegSDIInput3GStatus2
	kRegMaskSDIIn33GbpsMode = BIT(0),
	kRegMaskSDIIn33GbpsSMPTELevelBMode = BIT(1),
	kRegMaskSDIIn3LevelBtoLevelA = BIT(2),
	kRegMaskSDIIn3VPIDLinkAValid = BIT(4),
	kRegMaskSDIIn3VPIDLinkBValid = BIT(5),
	kRegMaskSDIIn36GbpsMode = BIT(6),
	kRegMaskSDIIn312GbpsMode = BIT(7),
	kRegMaskSDIIn43GbpsMode = BIT(8),
	kRegMaskSDIIn43GbpsSMPTELevelBMode = BIT(9),
	kRegMaskSDIIn4LevelBtoLevelA = BIT(10),
	kRegMaskSDIIn4VPIDLinkAValid = BIT(12),
	kRegMaskSDIIn4VPIDLinkBValid = BIT(13),
	kRegMaskSDIIn46GbpsMode = BIT(14),
	kRegMaskSDIIn412GbpsMode = BIT(15),

	// kRegSDI5678Input3GStatus
	kRegMaskSDIIn53GbpsMode = BIT(0),
	kRegMaskSDIIn53GbpsSMPTELevelBMode = BIT(1),
	kRegMaskSDIIn5LevelBtoLevelA = BIT(2),
	kRegMaskSDIIn5VPIDLinkAValid = BIT(4),
	kRegMaskSDIIn5VPIDLinkBValid = BIT(5),
	kRegMaskSDIIn56GbpsMode = BIT(6),
	kRegMaskSDIIn512GbpsMode = BIT(7),
	kRegMaskSDIIn63GbpsMode = BIT(8),
	kRegMaskSDIIn63GbpsSMPTELevelBMode = BIT(9),
	kRegMaskSDIIn6LevelBtoLevelA = BIT(10),
	kRegMaskSDIIn6VPIDLinkAValid = BIT(12),
	kRegMaskSDIIn6VPIDLinkBValid = BIT(13),
	kRegMaskSDIIn66GbpsMode = BIT(14),
	kRegMaskSDIIn612GbpsMode = BIT(15),
	kRegMaskSDIIn73GbpsMode = BIT(16),
	kRegMaskSDIIn73GbpsSMPTELevelBMode = BIT(17),
	kRegMaskSDIIn7LevelBtoLevelA = BIT(18),
	kRegMaskSDIIn7VPIDLinkAValid = BIT(20),
	kRegMaskSDIIn7VPIDLinkBValid = BIT(21),
	kRegMaskSDIIn76GbpsMode = BIT(22),
	kRegMaskSDIIn712GbpsMode = BIT(23),
	kRegMaskSDIIn83GbpsMode = BIT(24),
	kRegMaskSDIIn83GbpsSMPTELevelBMode = BIT(25),
	kRegMaskSDIIn8LevelBtoLevelA = BIT(26),
	kRegMaskSDIIn8VPIDLinkAValid = BIT(28),
	kRegMaskSDIIn8VPIDLinkBValid = BIT(29),
	kRegMaskSDIIn86GbpsMode = BIT(30),
	kRegMaskSDIIn812GbpsMode = BIT(31),

	// kRegVPID
	kRegMaskVPIDBitDepth				= BIT(1)+BIT(0),
	kRegmaskVPIDLuminance				= BIT(4),
	kRegMaskVPIDChannel					= BIT(7)+BIT(6),
	kRegMaskVPIDDualLinkChannel         = BIT(7)+BIT(6)+BIT(5),
	kRegMaskVPIDSampling				= BIT(11)+BIT(10)+BIT(9)+BIT(8),
	kRegMaskVPIDColorimetry				= BIT(12)+BIT(13),
	kRegMaskVPIDColorimetryAltLow		= BIT(12),
	kRegMaskVPIDImageAspect16x9Alt		= BIT(13),
	kRegMaskVPIDHorizontalSampling		= BIT(14),
	kRegMaskVPIDColorimetryAltHigh		= BIT(15),
	kRegMaskVPIDImageAspect16x9			= BIT(15),
	kRegMaskVPIDPictureRate				= BIT(19)+BIT(18)+BIT(17)+BIT(16),
	kRegMaskVPIDXferChars				= BIT(20)+BIT(21),
	kRegMaskVPIDProgressivePicture		= BIT(22),
	kRegMaskVPIDProgressiveTransport	= BIT(23),
	kRegMaskVPIDStandard				= BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
	kRegMaskVPIDVersionID				= BIT(31),
#if !defined (NTV2_DEPRECATE)
	kRegMaskVPIDDynamicRange			= BIT(4)+BIT(3),
#endif
	
	//Borg Test Pattern Generator
	kRegMaskTPGChromaSample             = BIT(9)+BIT(8)+BIT(7)+BIT(6)+BIT(5)+BIT(4)+BIT(3)+BIT(2)+BIT(1)+BIT(0),
	kRegMaskTPGLineBuffer               = BIT(11)+BIT(10),
	kRegMaskTPGFrameRate                = BIT(15)+BIT(14)+BIT(13)+BIT(12),
	kRegMaskTPGLuma                     = BIT(25)+BIT(24)+BIT(23)+BIT(22)+BIT(21)+BIT(20)+BIT(19)+BIT(18)+BIT(17)+BIT(16),
	kRegMaskTPGMulti                    = BIT(26),
    kRegMaskTPGReset                    = BIT(27),
	kRegMaskTPGStandard                 = BIT(30) + BIT(29) + BIT(28),
	kRegMaskTPGWriteEnable              = BIT(31),

	// Bones Actel Registers
	kRegMaskCFS1                        = BIT(0) + BIT(1), 
	kRegMaskCFS2                        = BIT(2) + BIT(3) ,

	// Audio Channel Control 2 or 8 channel playback mask
	kRegAudControlMask					= BIT(5),

	// Stereo Compressor Control
	kRegMaskStereoCompressorOutputMode		= BIT(3)+BIT(2)+BIT(1)+BIT(0),
	kRegMaskStereoCompressorFlipMode		= BIT(7)+BIT(6)+BIT(5)+BIT(4),
	kRegMaskStereoCompressorFlipLeftHorz	= BIT(4),
	kRegMaskStereoCompressorFlipLeftVert	= BIT(5),
	kRegMaskStereoCompressorFlipRightHorz	= BIT(6),
	kRegMaskStereoCompressorFlipRightVert	= BIT(7),
	kRegMaskStereoCompressorFormat			= BIT(10)+BIT(9)+BIT(8),
	kRegMaskStereoCompressorLeftSource		= BIT(23)+BIT(22)+BIT(21)+BIT(20)+BIT(19)+BIT(18)+BIT(17)+BIT(16),
	kRegMaskStereoCompressorRightSource		= BIT(31)+BIT(30)+BIT(29)+BIT(28)+BIT(27)+BIT(26)+BIT(25)+BIT(24),

	// SDI Direction Control
	kRegMaskSDI5Transmit	= BIT(24),
	kRegMaskSDI6Transmit	= BIT(25),
	kRegMaskSDI7Transmit	= BIT(26),
	kRegMaskSDI8Transmit	= BIT(27),
	kRegMaskSDI1Transmit	= BIT(28),
	kRegMaskSDI2Transmit	= BIT(29),
	kRegMaskSDI3Transmit	= BIT(30),
	kRegMaskSDI4Transmit	= BIT(31),
	
	// SDI watchdog control
	kRegMaskSDIRelayControl12	= BIT(0),
	kRegMaskSDIRelayControl34	= BIT(1),
	kRegMaskSDIWatchdogEnable12	= BIT(4),
	kRegMaskSDIWatchdogEnable34	= BIT(5),
	kRegMaskSDIRelayPosition12	= BIT(8),
	kRegMaskSDIRelayPosition34	= BIT(9),
	kRegMaskSDIWatchdogStatus	= BIT(12),

	// 4K Down Convert
	kRegMask4KDCRGBMode		= BIT(0),
	kRegMask4KDCYCC444Mode	= BIT(1),
	kRegMask4KDCPSFInMode	= BIT(2),
	kRegMask4KDCPSFOutMode	= BIT(3),

	// Quadrant Rasterizer Control
	kRegMaskRasterMode		= BIT(0)+BIT(1),
	kRegMaskTsiIO           = BIT(2),
	kRegMaskRasterLevelB	= BIT(4),
	kRegMaskRasterDecimate	= BIT(8),

	//	kRegRXSDI1Status, kRegRXSDI2Status, etc
	kRegMaskSDIInUnlockCount	= BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskSDIInLocked			= BIT(16),
	kRegMaskSDIInVpidValidA		= BIT(20),
	kRegMaskSDIInVpidValidB		= BIT(21),
	kRegMaskSDIInTRSError		= BIT(24),

	kRegMaskSDIInCRCErrorCountA = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskSDIInCRCErrorCountB = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMask2MFrameSize			= BIT(4)+BIT(3)+BIT(2)+BIT(1)+BIT(0),
	kRegMaskChannelBar			= BIT(24)+BIT(23)+BIT(22)+BIT(21)+BIT(20)+BIT(19)+BIT(18)+BIT(17)+BIT(16),

	kRegMaskPCMControlA1P1_2		= BIT(0),
	kRegMaskPCMControlA1P3_4		= BIT(1),
	kRegMaskPCMControlA1P5_6		= BIT(2),
	kRegMaskPCMControlA1P7_8		= BIT(3),
	kRegMaskPCMControlA1P9_10		= BIT(4),
	kRegMaskPCMControlA1P11_12		= BIT(5),
	kRegMaskPCMControlA1P13_14		= BIT(6),
	kRegMaskPCMControlA1P15_16		= BIT(7),

	kRegMaskPCMControlA2P1_2		= BIT(8),
	kRegMaskPCMControlA2P3_4		= BIT(9),
	kRegMaskPCMControlA2P5_6		= BIT(10),
	kRegMaskPCMControlA2P7_8		= BIT(11),
	kRegMaskPCMControlA2P9_10		= BIT(12),
	kRegMaskPCMControlA2P11_12		= BIT(13),
	kRegMaskPCMControlA2P13_14		= BIT(14),
	kRegMaskPCMControlA2P15_16		= BIT(15),

	kRegMaskPCMControlA3P1_2		= BIT(16),
	kRegMaskPCMControlA3P3_4		= BIT(17),
	kRegMaskPCMControlA3P5_6		= BIT(18),
	kRegMaskPCMControlA3P7_8		= BIT(19),
	kRegMaskPCMControlA3P9_10		= BIT(20),
	kRegMaskPCMControlA3P11_12		= BIT(21),
	kRegMaskPCMControlA3P13_14		= BIT(22),
	kRegMaskPCMControlA3P15_16		= BIT(23),

	kRegMaskPCMControlA4P1_2		= BIT(24),
	kRegMaskPCMControlA4P3_4		= BIT(25),
	kRegMaskPCMControlA4P5_6		= BIT(26),
	kRegMaskPCMControlA4P7_8		= BIT(27),
	kRegMaskPCMControlA4P9_10		= BIT(28),
	kRegMaskPCMControlA4P11_12		= BIT(29),
	kRegMaskPCMControlA4P13_14		= BIT(30),
	kRegMaskPCMControlA4P15_16		= BIT(31),

	kRegMaskPCMControlA5P1_2		= BIT(0),
	kRegMaskPCMControlA5P3_4		= BIT(1),
	kRegMaskPCMControlA5P5_6		= BIT(2),
	kRegMaskPCMControlA5P7_8		= BIT(3),
	kRegMaskPCMControlA5P9_10		= BIT(4),
	kRegMaskPCMControlA5P11_12		= BIT(5),
	kRegMaskPCMControlA5P13_14		= BIT(6),
	kRegMaskPCMControlA5P15_16		= BIT(7),

	kRegMaskPCMControlA6P1_2		= BIT(8),
	kRegMaskPCMControlA6P3_4		= BIT(9),
	kRegMaskPCMControlA6P5_6		= BIT(10),
	kRegMaskPCMControlA6P7_8		= BIT(11),
	kRegMaskPCMControlA6P9_10		= BIT(12),
	kRegMaskPCMControlA6P11_12		= BIT(13),
	kRegMaskPCMControlA6P13_14		= BIT(14),
	kRegMaskPCMControlA6P15_16		= BIT(15),

	kRegMaskPCMControlA7P1_2		= BIT(16),
	kRegMaskPCMControlA7P3_4		= BIT(17),
	kRegMaskPCMControlA7P5_6		= BIT(18),
	kRegMaskPCMControlA7P7_8		= BIT(19),
	kRegMaskPCMControlA7P9_10		= BIT(20),
	kRegMaskPCMControlA7P11_12		= BIT(21),
	kRegMaskPCMControlA7P13_14		= BIT(22),
	kRegMaskPCMControlA7P15_16		= BIT(23),

	kRegMaskPCMControlA8P1_2		= BIT(24),
	kRegMaskPCMControlA8P3_4		= BIT(25),
	kRegMaskPCMControlA8P5_6		= BIT(26),
	kRegMaskPCMControlA8P7_8		= BIT(27),
	kRegMaskPCMControlA8P9_10		= BIT(28),
	kRegMaskPCMControlA8P11_12		= BIT(29),
	kRegMaskPCMControlA8P13_14		= BIT(30),
	kRegMaskPCMControlA8P15_16		= BIT(31),

	kRegFanHiMask = BIT(16),
	kRegThermalMask = BIT(17) + BIT(18) + BIT(19),

	//kRegHDMIHDR.... 330-336
	kRegMaskHDMIHDRGreenPrimaryX = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskHDMIHDRGreenPrimaryY = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskHDMIHDRBluePrimaryX = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskHDMIHDRBluePrimaryY = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskHDMIHDRRedPrimaryX = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskHDMIHDRRedPrimaryY = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskHDMIHDRWhitePointX = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskHDMIHDRWhitePointY = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskHDMIHDRMaxMasteringLuminance = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskHDMIHDRMinMasteringLuminance = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskHDMIHDRMaxContentLightLevel = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskHDMIHDRMaxFrameAverageLightLevel = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),

	kRegMaskHDMIHDRNonContantLuminance = BIT(0),
    kRegMaskHDMIHDRDolbyVisionEnable = BIT(6),
	kRegMaskHDMIHDREnable = BIT(7),
	kRegMaskElectroOpticalTransferFunction = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23),
	kRegMaskHDRStaticMetadataDescriptorID = BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),

	kRegMaskAudioMixerMainInputSelect = BIT(2) + BIT(1) + BIT(0),
	kRegMaskAudioMixerAux1x2CHInput = BIT(6) + BIT(5) + BIT(4),
	kRegMaskAudioMixerAux2x2CHInput = BIT(10) + BIT(9) + BIT(8),
	kRegMaskAudioMixerChannelSelect = BIT(2) + BIT(1) + BIT(0),
	kRegMaskAudioMixerOutputChannelsMute =  BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerOutputChannel1Mute =  BIT(0),
	kRegMaskAudioMixerOutputChannel2Mute = BIT(1),
	kRegMaskAudioMixerOutputChannel3Mute = BIT(2),
	kRegMaskAudioMixerOutputChannel4Mute = BIT(3),
	kRegMaskAudioMixerOutputChannel5Mute = BIT(4),
	kRegMaskAudioMixerOutputChannel6Mute = BIT(5),
	kRegMaskAudioMixerOutputChannel7Mute = BIT(6),
	kRegMaskAudioMixerOutputChannel8Mute = BIT(7),
	kRegMaskAudioMixerOutputChannel9Mute = BIT(8),
	kRegMaskAudioMixerOutputChannel10Mute = BIT(9),
	kRegMaskAudioMixerOutputChannel11Mute = BIT(10),
	kRegMaskAudioMixerOutputChannel12Mute = BIT(11),
	kRegMaskAudioMixerOutputChannel13Mute = BIT(12),
	kRegMaskAudioMixerOutputChannel14Mute = BIT(13),
	kRegMaskAudioMixerOutputChannel15Mute = BIT(14),
	kRegMaskAudioMixerOutputChannel16Mute = BIT(15),
	kRegMaskAudioMixerMainInputEnable = BIT(16)+BIT(17),
	kRegMaskAudioMixerAux1InputEnable = BIT(18)+BIT(19),
	kRegMaskAudioMixerAux2InputEnable = BIT(20)+BIT(21),
	kRegMaskAudioMixerAux1Channel1Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerAux1Channel2Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerAux2Channel1Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerAux2Channel2Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainChannel1Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainChannel2Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainChannel3Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainChannel4Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainChannel5Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainChannel6Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainChannel7Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainChannel8Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainChannel9Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainChannel10Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainChannel11Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainChannel12Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainChannel13Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainChannel14Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainChannel15Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainChannel16Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerMainMixedOutputChannel1Level = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerMainMixedOutputChannel2Level = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerInputLeftLevel = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	kRegMaskAudioMixerInputRightLevel = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	kRegMaskAudioMixerLevelSampleCount	= BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),

    kRegMaskRotaryEncoderValue = BIT(7)+BIT(6)+BIT(5)+BIT(4)+BIT(3)+BIT(2)+BIT(1)+BIT(0),
    kRegMaskRotaryEncoderGain = BIT(13)+BIT(12)+BIT(11)+BIT(10)+BIT(9)+BIT(8),
	
	kRegMaskLUTAddress =	BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15) + \
							BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29),
	kRegMaskLUTLoad = BIT(31),

	kRegMaskMRStandard = BIT(0) + BIT(1) + BIT(2) + BIT(3),
	kRegMaskMRBypass = BIT(20),
	kRegMaskMREnable = BIT(24),
	kRegMaskMRSupport = BIT(0) + BIT(1) + BIT(2)

#if !defined (NTV2_DEPRECATE)
	,
	// kRegSDIInput3GStatus
	kLHIRegMaskSDIIn3GbpsMode = BIT(0),
	kLHIRegMaskSDIIn3GbpsSMPTELevelBMode = BIT(1),
	kLHIRegMaskSDIInVPIDLinkAValid = BIT(4),
	kLHIRegMaskSDIInVPIDLinkBValid = BIT(5),
	kLHIRegMaskSDIIn23GbpsMode = BIT(8),
	kLHIRegMaskSDIIn23GbpsSMPTELevelBMode = BIT(9),
	kLHIRegMaskSDIIn2VPIDLinkAValid = BIT(12),
	kLHIRegMaskSDIIn2VPIDLinkBValid = BIT(13),

	// kRegSDIInput3GStatus2
	kLHIRegMaskSDIIn33GbpsMode = BIT(0),
	kLHIRegMaskSDIIn33GbpsSMPTELevelBMode = BIT(1),
	kLHIRegMaskSDIIn3VPIDLinkAValid = BIT(4),
	kLHIRegMaskSDIIn3VPIDLinkBValid = BIT(5),
	kLHIRegMaskSDIIn43GbpsMode = BIT(8),
	kLHIRegMaskSDIIn43GbpsSMPTELevelBMode = BIT(9),
	kLHIRegMaskSDIIn4VPIDLinkAValid = BIT(12),
	kLHIRegMaskSDIIn4VPIDLinkBValid = BIT(13),
	//
	// Borg Fusion Registers
	//

	// Boot FPGA and BoardID
	kRegMaskBorgFusionBootFPGAVer = BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kRegMaskBorgFusionBoardID = BIT(0)+BIT(1)+BIT(2),

	// Codec and convert FPGA configuration control
	kRegMaskBorgFusionCodecFPGAProgram = BIT(0),
	kRegMaskBorgFusionCodecFPGAInit = BIT(1),
	kRegMaskBorgFusionCodecFPGADone = BIT(2),

	kRegMaskBorgFusionConvertFPGAProgram = BIT(4),
	kRegMaskBorgFusionConvertFPGAInit = BIT(5),
	kRegMaskBorgFusionConvertFPGADone = BIT(6),

	// Panel Push buttons debounced and SATA drive present state
	kRegMaskBorgFusionPushButtonSlotDebounced = BIT(0),
	kRegMaskBorgFusionPushButtonAdjustDownDebounced = BIT(1),
	kRegMaskBorgFusionPushButtonAdjustUpDebounced = BIT(2),
	kRegMaskBorgFusionPushButtonDeleteClipDebounced = BIT(3),
	kRegMaskBorgFusionPushButtonSelectDownDebounced = BIT(4),
	kRegMaskBorgFusionPushButtonSelectUpDebounced = BIT(5),
	kRegMaskBorgFusionPushButtonFastFwdDebounced = BIT(6),
	kRegMaskBorgFusionPushButtonRecordDebounced = BIT(7),
	kRegMaskBorgFusionPushButtonPlayDebounced = BIT(8),
	kRegMaskBorgFusionPushButtonStopDebounced = BIT(9),
	kRegMaskBorgFusionPushButtonRewindDebounced = BIT(10),
	kRegMaskBorgFusionPushButtonMediaDebounced = BIT(11),
	kRegMaskBorgFusionPushButtonConfigDebounced = BIT(12),
	kRegMaskBorgFusionPushButtonStatusDebounced = BIT(13),
	kRegMaskBorgFusionPushButtonSATADrivePresentDebounced = BIT(14),
	kRegMaskBorgFusionPushButtonPowerDebounced = BIT(15),

	// Panel Push buttons and SATA drive present changes
	kRegMaskBorgFusionPushButtonSlotChange = BIT(0),
	kRegMaskBorgFusionPushButtonAdjustDownChange = BIT(1),
	kRegMaskBorgFusionPushButtonAdjustUpChange = BIT(2),
	kRegMaskBorgFusionPushButtonDeleteClipChange = BIT(3),
	kRegMaskBorgFusionPushButtonSelectDownChange = BIT(4),
	kRegMaskBorgFusionPushButtonSelectUpChange = BIT(5),
	kRegMaskBorgFusionPushButtonFastFwdChange = BIT(6),
	kRegMaskBorgFusionPushButtonRecordChange = BIT(7),
	kRegMaskBorgFusionPushButtonPlayChange = BIT(8),
	kRegMaskBorgFusionPushButtonStopChange = BIT(9),
	kRegMaskBorgFusionPushButtonRewindChange = BIT(10),
	kRegMaskBorgFusionPushButtonMediaChange = BIT(11),
	kRegMaskBorgFusionPushButtonConfigChange = BIT(12),
	kRegMaskBorgFusionPushButtonStatusChange = BIT(13),
	kRegMaskBorgFusionPushButtonSATADrivePresentChange = BIT(14),
	kRegMaskBorgFusionPushButtonPowerButtonChange = BIT(15),

	// LED Pulse Width Modulation Threshholds
	kRegMaskBorgFusionPWMThreshExpressCard2 = BIT(0)+BIT(1)+BIT(2)+BIT(3),
	kRegMaskBorgFusionPWMThreshExpressCard1 = BIT(4)+BIT(5)+BIT(6)+BIT(7),
	kRegMaskBorgFusionPWMThreshPower = BIT(8)+BIT(9)+BIT(10)+BIT(11),
	kRegMaskBonesFusionPWMThreshLCDBacklightLED = BIT(12)+BIT(13)+BIT(14)+BIT(15),

	// Power control - System
	kRegMaskBorgFusionPowerCtrlWiFiReset = BIT(0),
	kRegMaskBorgFusionPowerCtrlFirewirePower = BIT(1),
	kRegMaskBorgFusionPowerCtrlGigEthReset = BIT(2),
	kRegMaskBorgFusionPowerCtrlPCIExpClockStop = BIT(3),

	// Power control - Storage devices - Borg Fusion
	kRegMaskBorgFusionPowerCtrlPCIExpCard1_3_3vPower = BIT(8),	// Express Card 1 3.3v power
	kRegMaskBorgFusionPowerCtrlPCIExpCard1_1_5vPower = BIT(9),	// Express Card 1 1.5v power
	kRegMaskBorgFusionPowerCtrlPCIExpCard2_3_3vPower = BIT(10),	// Express Card 2 3.3v power
	kRegMaskBorgFusionPowerCtrlPCIExpCard2_1_5vPower = BIT(11),	// Express Card 2 1.5v power
	kRegMaskBorgFusionPowerCtrlSata_12vPower = BIT(12),			// SATA Drive 12v power

	// Power control - Storage devices - Bones Actel
	kRegMaskBonesActelPowerCtrlCFSlot2_BridgeReset   = BIT(8),	// Bones Actel CF Slot 2 (CPU) Bridge Reset
	kRegMaskBonesActelPowerCtrlCFSlot2_Power         = BIT(9),	// Bones Actel CF Slot 2 (CPU) Power
	kRegMaskBonesActelPowerCtrlCFSlot1_Power         = BIT(10),	// Bones Actel CF Slot 1 (VIDeo) Power
	kRegMaskBonesActelPowerCtrlCFSlot1_BridgeReset   = BIT(11),	// Bones Actel CF Slot 1 (VIDeo) Bridge Reset

	// Power control - Storage devices - Barclay Actel Fusion
	kRegMaskBarclayFusionPowerCtrlPS1Active = BIT(6), 			// Barclay Fusion Power Supply 1 active bit
	kRegMaskBarclayFusionPowerCtrlPS2Active = BIT(5), 			// Barclay Fusion Power Supply 2 active bit

	kRegMaskBarclayFusionIdentifyLEDCtrl = BIT(1), 	//Barclay Identify LED On/Off bit, Rear LED //RS

	// Power control - Pushbutton LEDs
	kRegMaskBorgFusionPowerCtrlPCIExpCard2LED = BIT(13),
	kRegMaskBorgFusionPowerCtrlPCIExpCard1LED = BIT(14),
	kRegMaskBorgFusionPowerCtrlPowerButtonLED = BIT(15),

	// IRQ3n Interrupt control
	kRegMaskBorgFusionIRQ3nIntCtrlPushButtonChangeEnable = BIT(0),
	kRegMaskBorgFusionIRQ3nIntCtrlInputVoltageLow9vEnable = BIT(1),
	kRegMaskBorgFusionIRQ3nIntCtrlDisplayFIFOFullEnable = BIT(2),
	kRegMaskBorgFusionIRQ3nIntCtrlSATAPresentChangeEnable = BIT(3),
	kRegMaskBorgFusionIRQ3nIntCtrlTemp1HighEnable = BIT(4),
	kRegMaskBorgFusionIRQ3nIntCtrlTemp2HighEnable = BIT(5),
	kRegMaskBorgFusionIRQ3nIntCtrlPowerButtonChangeEnable = BIT(6),

	// IRQ3n Interrupt source
	kRegMaskBorgFusionIRQ3nIntSrcPushButtonChange= BIT(0),
	kRegMaskBorgFusionIRQ3nIntSrcInputVoltageLow9v= BIT(1),
	kRegMaskBorgFusionIRQ3nIntSrcDisplayFIFOFull= BIT(2),
	kRegMaskBorgFusionIRQ3nIntSrcSATAPresentChange= BIT(3),
	kRegMaskBorgFusionIRQ3nIntSrcTemp1High= BIT(4),
	kRegMaskBorgFusionIRQ3nIntSrcTemp2High= BIT(5),
	kRegMaskBorgFusionIRQ3nIntSrcPowerButtonChange= BIT(6),

	// Noritake Display Control/Status
	kRegMaskBorgFusionDisplayCtrlReset = BIT (0),
	kRegMaskBorgFusionDisplayStatusBusyRaw = BIT (1),		// Not needed by CPU, used internally by FPGA
	kRegMaskBorgFusionDisplayStatusInterfaceBusy = BIT (7),	// FIFO full

	// Analog ADC flags - battery
	kRegMaskBorgFusionAnalogFlagsPowerLTE9v = BIT(0),	// +12 v supply <= 9.0 v battery critical
	kRegMaskBorgFusionAnalogFlagsPowerLTE10v = BIT(1),	// +12 v supply <= 10.0 v battery depleting
	kRegMaskBorgFusionAnalogFlagsPowerLTE11v = BIT(2),	// +12 v supply <= 11.0 v battery depleting
	kRegMaskBorgFusionAnalogFlagsPowerGTE13v = BIT(3),	// +12 v supply >= 13.0 v battery charging

	// Analog ADC flags - temperature sensor
	kRegMaskBorgFusionAnalogFlagsPowerTemp1High = BIT(4),	// Temp sensor 1 > 65 C
	kRegMaskBorgFusionAnalogFlagsPowerTemp2High = BIT(5),	// Temp sensor 2 > 65 C

	// Bones Actel Compact Flash Slot Debounced Card Present
	kRegMaskBonesActelCFSlot1_Present   = BIT(1)+BIT(0),
	kRegMaskBonesActelCFSlot2_Present   = BIT(3)+BIT(2),

	// Bones Actel Compact Flash Slot Changes Present
	kRegMaskBonesActelCFSlot1_Changes   = BIT(1)+BIT(0),
	kRegMaskBonesActelCFSlot2_Changes   = BIT(3)+BIT(2),
	// Pan (2K crop) - Xena 2
	kRegMaskPanMode			= BIT(30) + BIT(31),
	kRegMaskPanOffsetH		= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11),
	kRegMaskPanOffsetV		= BIT(12)+BIT(13)+BIT(14)+BIT(15)+BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)
#endif	//	!defined (NTV2_DEPRECATE)

} RegisterMask;

typedef enum
{
	// Global Control
	kRegShiftFrameRate = 0,
	kRegShiftFrameRateHiBit = 22,
	kRegShiftGeometry = 3,
	kRegShiftStandard = 7,
	kRegShiftRefSource = 10,
	kRegShiftRefInputVoltage = 12,
	kRegShiftSmpte372 = 15,
	kRegShiftLED = 16,
	kRegShiftRegClocking = 20,
	kRegShiftDualLinkInput = 23,
	kRegShiftQuadTsiEnable = 24,
	kRegShiftBankSelect = 25,
	kRegShiftDualLinKOutput = 27,
	kRegShiftRP188ModeCh1 = 28,
	kRegShiftRP188ModeCh2 = 29,
	kRegShiftCCHostAccessBankSelect = 30,

	// Global Control 2
	kRegShiftRefSource2 = 0,
	kRegShiftPCRReferenceEnable = 1,
	kRegShiftQuadMode = 3,
	kRegShiftAud1PlayCapMode = 4,
	kRegShiftAud2PlayCapMode = 5,
	kRegShiftAud3PlayCapMode = 6,
	kRegShiftAud4PlayCapMode = 7,
	kRegShiftAud5PlayCapMode = 8,
	kRegShiftAud6PlayCapMode = 9,
	kRegShiftAud7PlayCapMode = 10,
	kRegShiftAud8PlayCapMode = 11,
	kRegShiftQuadMode2 = 12,
	kRegShiftSmpte372Enable4 = 13,
	kRegShiftSmpte372Enable6 = 14,
	kRegShiftSmpte372Enable8 = 15,
	kRegShiftIndependentMode = 16,
	kRegShift2MFrameSupport = 17,
	kRegShiftAudioMixerPresent = 18,
	kRegShiftIsDNXIV = 19,
	kRegShift425FB12 = 20,
	kRegShift425FB34 = 21,
	kRegShift425FB56 = 22,
	kRegShift425FB78 = 23,
	kRegShiftRP188ModeCh3 = 28,
	kRegShiftRP188ModeCh4 = 29,
	kRegShiftRP188ModeCh5 = 30,
	kRegShiftRP188ModeCh6 = 31,
	kRegShiftRP188ModeCh7 = 26,
	kRegShiftRP188ModeCh8 = 27,

	// kRegGlobalControl3	(reg 108)
	kRegShiftAnalogIOControl = 0,
	kRegShiftQuadQuadMode = 2,
	kRegShiftQuadQuadMode2 = 3,
	kRegShiftQuadQuadSquaresMode = 4,
	kRegShiftVUMeterSelect = 5,
	kRegShiftFramePulseEnable = 6,
	kRegShiftFramePulseRefSelect = 8,

	// Audio Control 2
	kRegShiftAud1RateHigh		= 0,
	kRegShiftAud2RateHigh		= 1,
	kRegShiftAud3RateHigh		= 2,
	kRegShiftAud4RateHigh		= 3,
	kRegShiftAud5RateHigh		= 4,
	kRegShiftAud6RateHigh		= 5,
	kRegShiftAud7RateHigh		= 6,
	kRegShiftAud8RateHigh		= 7,

	// Channel Control - kRegCh1Control, kRegCh2Control, kRegCh3Control, kRegCh4Control
	kRegShiftMode						= 0,
	kRegShiftFrameFormat				= 1,
	kRegShiftFrameFormatHiBit			= 6, // KAM
	kRegShiftAlphaFromInput2			= 5,
	kRegShiftChannelDisable				= 7,
	kRegShiftWriteBack					= 8,
	kRegShiftFrameOrientation			= 10,
	kRegShiftQuarterSizeMode			= 11,
	kRegShiftFrameBufferMode			= 12,
	kKHRegShiftDownconvertInput			= 14,
	kLSRegShiftVideoInputSelect			= 15,
	kRegShiftDitherOn8BitInput			= 16,
	kRegShiftQuality					= 17,
	kRegShiftEncodeAsPSF				= 18,
	kK2RegShiftFrameSize				= 20,
	kRegShiftChannelCompressed			= 22,
	kRegShiftRGB8b10bCvtMode			= 23,
	kRegShiftVBlankRGBRangeMode			= 24,		// Deprecated
	kRegShiftVBlankRGBRange				= 24,
	kRegShiftQuality2					= 25,
	kRegCh1BlackOutputShift				= 27,		// KiPro bit set results in black output video and muted audio in capture mode, black output video in playback
	kRegShiftSonySRExpressBit			= 28,
	kRegShiftFrameSizeSetBySW			= 29,
	kRegShiftVidProcVANCShift			= 31,

	// Video Crosspoint Control
	kRegShiftVidXptFGVideo				= 0,
	kRegShiftVidXptBGVideo				= 4,
	kRegShiftVidXptFGKey				= 8,
	kRegShiftVidXptBGKey				= 12,
	kRegShiftVidXptSecVideo				= 16,

	// Video Processing Control
	kRegShiftVidProcMux1				= 0,
	kRegShiftVidProcMux2				= 2,
	kRegShiftVidProcMux3				= 4,
	kRegShiftVidProcMux4				= 6,
	kRegShiftVidProcMux5				= 8,
	kRegShiftVidProcLimiting			= 11,
	kRegShiftVidProcVancSource			= 13,
	kRegShiftVidProcRGBRange			= 14,
	kRegShiftVidProcFGMatteEnable		= 18,
	kRegShiftVidProcBGMatteEnable		= 19,
	kRegShiftVidProcFGControl			= 20,
	kRegShiftVidProcBGControl			= 22,
	kRegShiftVidProcMode				= 24,
	kRegShiftVidProcRGBModeSupported	= 26,
	kRegShiftVidProcSyncFail			= 27,
	kRegShiftVidProcSplitStd			= 28,
	kRegShiftVidProcSubtitleEnable	= 31,


	// Note:  See more bitfields for this register below, in the 'Xena2K and Konax Video Processing.' section
	
	// kRegStatus
	kRegShiftHardwareVersion			= 0,
	kRegShiftFPGAVersion				= 4,
	kRegShiftLTCInPresent				= 17,

	// Video Interrupt Control
	kRegShiftIntEnableMask				= 0,

	// Audio Control
	kRegShiftCaptureEnable				= 0,
	kRegShiftNumBits					= 1,	// shouldn't this be 2?
	kRegShiftOutputTone					= 1,
	kRegShift20BitMode					= 2,
	kRegShiftLoopBack					= 3,
	kRegShiftAudioTone					= 7,
	kRegShiftResetAudioInput			= 8,
	kRegShiftResetAudioOutput			= 9,
	kRegShiftInputStartAtVBI			= 10,	//	New in 15.6
	kRegShiftPauseAudio					= 11,
	kRegShiftEmbeddedOutputMuteCh1      = 12, // added for FS1
	kRegShiftEmbeddedOutputSupressCh1   = 13, // added for FS1 but available on other boards
	kRegShiftOutputStartAtVBI			= 14,	//	New in 15.6
	kRegShiftEmbeddedOutputSupressCh2	= 15, // added for FS1 but available on other boards
	kRegShiftNumChannels				= 16,
	kRegShiftEmbeddedOutputMuteCh2      = 17, // added for FS1
	kRegShiftAudioRate					= 18,
	kRegShiftEncodedAudioMode			= 19,
	kRegShiftAudio16Channel				= 20,
	kRegShiftAudio8Channel				= 23,
	kK2RegShiftKBoxAnalogMonitor		= 24,
	kK2RegShiftKBoxAudioInputSelect		= 26,
	kK2RegShiftKBoxDetect				= 27,
	kK2RegShiftBOCableDetect			= 28,
	kK2RegShiftAudioLevel				= 29,
	kK2RegShiftAverageAudioLevel		= 0,
	kFS1RegShiftAudioLevel				= 29,
	kK2RegShiftAudioBufferSize			= 31,
	kLHRegShiftResetAudioDAC			= 31,

	// Audio Source Select
	kRegShiftAudioSource				= 0,
	kRegShiftEmbeddedAudioInput			= 16,
	kRegShiftAudioAutoErase				= 19,
	kRegShiftAnalogHDMIvsAES     		= 20,
	kRegShift3GbSelect					= 21,
	kRegShiftEmbeddedAudioClock			= 22,
	kRegShiftEmbeddedAudioInput2		= 23,
	kRegShiftAnalogAudioInGain			= 24,
	kRegShiftAnalogAudioInJack			= 25,

	kRegShiftLossOfInput				= 11,

	// Input Status
	kRegShiftInput1FrameRate			= 0,
	kRegShiftInput1Geometry				= 4,
	kRegShiftInput1Progressive			= 7,
	kRegShiftInput2FrameRate			= 8,
	kRegShiftInput2Geometry				= 12,
	kRegShiftInput2Progressive			= 15,
	kRegShiftReferenceFrameRate			= 16,
	kRegShiftReferenceFrameLines		= 20,
	kRegShiftReferenceProgessive		= 23,
	kRegShiftAESCh12Present				= 24,
	kRegShiftAESCh34Present				= 25,
	kRegShiftAESCh56Present				= 26,
	kRegShiftAESCh78Present				= 27,
	kRegShiftInput1FrameRateHigh		= 28,
	kRegShiftInput2FrameRateHigh		= 29,
	kRegShiftInput1GeometryHigh			= 30,
	kRegShiftInput2GeometryHigh			= 31,

	// RP-188 Source
	kRegShiftRP188Source				= 24,
	kRegShiftRP188DBB					= 0,

    // DMA Control
	kRegShiftForce64					= 4,
    kRegShiftAutodetect64				= 5,
	kRegShiftFirmWareRev				= 8,
	kRegShiftDMAPauseDisable			= 16,
	
	// Color Correction Control
	kRegShiftSaturationValue			= 0,
	kRegShiftCCOutputBankSelect			= 16,
	kRegShiftCCMode						= 17,
	kRegShiftCC5HostAccessBankSelect	= 20,
	kRegShiftCC5OutputBankSelect		= 21,
	kRegShiftLUT5Select					= 28,
	kRegShiftLUTSelect					= 29,
	kRegShiftCC3OutputBankSelect		= 30,
	kRegShiftCC4OutputBankSelect		= 31,

	//	kRegCanDoStatus
	kRegShiftCanDoValidXptROM			= 0,
	kRegShiftCanDoAudioWaitForVBI		= 1,

	//	kRegLUTV2Control
	kRegShiftLUT1Enable					= 0,
	kRegShiftLUT2Enable					= 1,
	kRegShiftLUT3Enable					= 2,
	kRegShiftLUT4Enable					= 3,
	kRegShiftLUT5Enable					= 4,
	kRegShiftLUT6Enable					= 5,
	kRegShiftLUT7Enable					= 6,
	kRegShiftLUT8Enable					= 7,
	kRegShiftLUT1HostAccessBankSelect	= 8,
	kRegShiftLUT2HostAccessBankSelect	= 9,
	kRegShiftLUT3HostAccessBankSelect	= 10,
	kRegShiftLUT4HostAccessBankSelect	= 11,
	kRegShiftLUT5HostAccessBankSelect	= 12,
	kRegShiftLUT6HostAccessBankSelect	= 13,
	kRegShiftLUT7HostAccessBankSelect	= 14,
	kRegShiftLUT8HostAccessBankSelect	= 15,
	kRegShiftLUT1OutputBankSelect		= 16,
	kRegShiftLUT2OutputBankSelect		= 17,
	kRegShiftLUT3OutputBankSelect		= 18,
	kRegShiftLUT4OutputBankSelect		= 19,
	kRegShiftLUT5OutputBankSelect		= 20,
	kRegShiftLUT6OutputBankSelect		= 21,
	kRegShiftLUT7OutputBankSelect		= 22,
	kRegShiftLUT8OutputBankSelect		= 23,
	kRegShift12BitLUTPlaneSelect		= 24,
	kRegShift12BitLUTSupport			= 28,

	// RS422 Control 
	kRegShiftRS422TXEnable				= 0,
	kRegShiftRS422TXFIFOEmpty 	 	 	= 1,
	kRegShiftRS422TXFIFOFull			= 2,
	kRegShiftRS422RXEnable				= 3,
	kRegShiftRS422RXFIFONotEmpty		= 4,
	kRegShiftRS422RXFIFOFull			= 5,
	kRegShiftRS422RXParityError			= 6,
	kRegShiftRS422Flush					= 6,
	kRegShiftRS422RXFIFOOverrun			= 7,
	kRegShiftRS422Present				= 8,
	kRegShiftRS422TXInhibit				= 9,
	kRegShiftRS422ParitySense			= 12,
	kRegShiftRS422ParityDisable			= 13,
	kRegShiftRS422BaudRate				= 16,


	// FS1 ProcAmp Control
	kFS1RegShiftProcAmpC1Y				= 0,
	kFS1RegShiftProcAmpC1CB				= 16,
	kFS1RegShiftProcAmpC1CR				= 0,
	kFS1RegShiftProcAmpC2CB				= 16,
	kFS1RegShiftProcAmpC2CR				= 0,
	kFS1RegShiftProcAmpOffsetY			= 16,

	kRegShiftAudioInDelay				= 0,
	kRegShiftAudioOutDelay				= 16,
	
	// FS1 Audio Delay
	kFS1RegShiftAudioDelay				= 0,

	// Borg Audio Delay			
	kBorgRegShiftPlaybackEEAudioDelay = 0,
	kBorgRegShiftCaputreAudioDelay = 16,

	// kRegOutputTimingControl 
	kBorgRegShiftOutTimingCtrlHorzOfs = 0,
	kBorgRegShiftOutTimingCtrlVertOfs = 16,

	// FS1 I2C
	kFS1RegShiftI2C1ControlWrite		= 0,
	kFS1RegShiftI2C1ControlRead			= 1,
	kFS1RegShiftI2C1ControlBusy			= 2,
	kFS1RegShiftI2C1ControlError		= 3,
	kFS1RegShiftI2C2ControlWrite		= 4,
	kFS1RegShiftI2C2ControlRead			= 5,
	kFS1RegShiftI2C2ControlBusy			= 6,
	kFS1RegShiftI2C2ControlError		= 7,
	
	kFS1RegShiftI2CAddress				= 0,
	kFS1RegShiftI2CSubAddress			= 8,
	kFS1RegShiftI2CWriteData			= 0,
	kFS1RegShiftI2CReadData				= 8,
	
	//kRegFS1ReferenceSelect
	kRegShiftLTCLoopback				= 10,
	kFS1RegShiftReferenceInputSelect	= 0,
	kFS1RefShiftLTCOnRefInSelect		= 4,
	kRegShiftLTCOnRefInSelect			= 5,
	kFS1RefShiftLTCEmbeddedOutEnable	= 8,
	kFS1RegShiftColorFIDSubcarrierReset = 14,
	kFS1RegShiftFreezeOutput			= 15,
	kFS1RegShiftProcAmpInputSelect		= 16,
	kFS1RegShiftSecondAnalogOutInputSelect = 24,
	
	// FS1 AFD Mode
	kFS1RegShiftAFDReceived_Code = 0,
	kFS1RegShiftAFDReceived_AR = 4,
	kFS1RegShiftAFDReceived_VANCPresent = 7,
	kFS1RegShiftUpconvertAutoAFDEnable = 20,
	kFS1RegShiftUpconvert2AFDDefaultHoldLast = 21,
	kFS1RegShiftDownconvertAutoAFDEnable = 24,
	kFS1RegShiftDownconvertAFDDefaultHoldLast = 25,
	kFS1RegShiftDownconvert2AutoAFDEnable = 28,
	kFS1RegShiftDownconvert2AFDDefaultHoldLast = 29,

	// FS1 AFD Inserter
	kFS1RegShiftAFDVANCInserter_Code = 0,
	kFS1RegShiftAFDVANCInserter_AR = 4,
	kFS1RegShiftAFDVANCInserter_Mode = 12,
	kFS1RegShiftAFDVANCInserter_Line = 16,

    // FS1 Audio Channel Mapping
    kFS1RegShiftAudioChannelMapping_Gain = 0,
    kFS1RegShiftAudioChannelMapping_Phase = 15,
    kFS1RegShiftAudioChannelMapping_Source = 16,
    kFS1RegShiftAudioChannelMapping_Mute = 31,

    // FS1 Output Timing Fine Phase Adjust
    kRegShiftOutputTimingFinePhase = 0,
	
    //kRegAnalogInputStatus
	kRegShiftAnalogInputSD				= 14,
	kRegShiftAnalogInputIntegerRate		= 15,
	kRegShiftAnalogCompositeLocked		= 16,
	kRegShiftAnalogCompositeFormat625	= 18,

	//kRegAnalogInputControl
	kRegShiftAnalogInputADCMode			= 0,
	
	//kRegHDMIOut3DControl
	kRegShiftHDMIOut3DPresent			= 3,
	kRegShiftHDMIOut3DMode				= 4,
	
	//kRegHDMIOutControl
	kRegShiftHDMIOutVideoStd			= 0,
	kRegShiftHDMIOut8ChGroupSelect		= 5,
	kLHIRegShiftHDMIDownStreamDeviceYCbCrMode = 6,
	kLHIRegShiftHDMIDownStreamDevice10BitMode = 7,
	kRegShiftHDMIV2TxBypass				= 7,
	kLHIRegShiftHDMIOutColorSpace		= 8,
	kLHIRegShiftHDMIOutFPS				= 9,
	kRegShiftHDMIOutProgressive			= 13,
	kLHIRegShiftHDMIOutBitDepth			= 14,
	kRegShiftHDMISampling				= 18,
	kRegShiftHDMIVOBD					= 20,
	kRegShiftSourceIsRGB				= 23,
	kRegShiftHDMIOutPowerDown			= 25,
	kRegShiftHDMIOutRange				= 28,
	kRegShiftHDMIOutAudioCh				= 29,
	kLHIRegShiftHDMIOutDVI 				= 30,

	//kRegHDMIInputStatus
	kRegShiftInputStatusLock			= 0,
	kRegShiftStable						= 1,
	kLHIRegShiftHDMIInputColorSpace		= 2,
	kLHIRegShiftHDMIInputBitDepth		= 3,
	kRegShiftHDMIInV2VideoStd			= 4,
	kLHIRegShiftHDMIOutputEDIDRGB		= 10,
	kLHIRegShiftHDMIOutputEDID10Bit		= 11,
	kLHIRegShiftHDMIInput2ChAudio		= 12,
	kRegShiftHDMIInputProgressive		= 13,
	kLHIRegShiftHDMIOutputEDIDDVI		= 15,
	kRegShiftHDMIInAudioRate			= 16,
	kRegShiftAudioWorkLength			= 20,
	kRegShiftInputStatusStd				= 24,
	kLHIRegShiftHDMIInputProtocol		= 27,
	kRegShiftInputStatusFPS				= 28,

	//kRegHDMIInputControl
	kRegShiftHDMIOutForceConfig			= 1,
	kRegShiftHDMIAudioPairSelect 		= 2,
	kRegShiftHDMISampleRateConverterEnable = 4,
	kRegShiftHDMISwapInputAudCh34		= 5,
	kRegShiftHDMISwapOutputAudCh34		= 6,
	kRegShiftHDMIOutPrefer420			= 7,
    kRegShiftHDMIOutAudioFormat			= 8,
	kRegShiftHDMIInColorDepth			= 12,
	kRegShiftHDMIInColorSpace			= 14,
	kRegShiftHDMIOutAudioRate			= 16,
	kRegShiftHDMIOutSourceSelect		= 20,
	kRegShiftHDMIOutCropEnable			= 24,
	kRegShiftHDMIOutForceHPD			= 25,
	kRegShiftHDMIOut12Bit				= 26,
	kRegShiftHDMIDebug					= 27,
	kRegShiftHDMIInputRange				= 28,
	kRegShiftHDMIOutAudio2ChannelSelect	= 29,
	kRegShiftHDMIInfoRange				= 31,
	
	//kRegHDMIInputControl / kRegHDMIOutControl ???
	kRegShiftHDMIColorSpace				= 4,
	kRegShiftHDMIPolarity				= 16,

	//kK2RegAnalogOutControl,
	kK2RegShiftVideoDACMode				= 0,
	kFS1RegShiftVideoDAC2Mode			= 8,
	kLHIRegShiftVideoDACStandard		= 13,
	kLSRegShiftVideoADCMode				= 16,
	kLHIRegShiftVideoDACMode			= 21,		// 4 bit enum equivalent of bit 21-24
	kLHIRegShiftVideoDACSetup			= 21,
	kLHIRegShiftVideoDACJapan			= 22,
	kLHIRegShiftVideoDACRGB				= 23,
	kLHIRegShiftVideoDACComponent		= 24,
	kK2RegShiftOutHTiming				= 24,

	//kK2RegSDIOut1Control + kRegK2SDIOut2Control + kK2RegAnalogOutControl
	kK2RegShiftSDIOutStandard			= 0,
	kK2RegShiftSDI1Out_2Kx1080Mode		= 3,
	kLHRegShiftVideoOutputDigitalSelect	= 4,
	kK2RegShiftSDIOutHBlankRGBRange		= 7,
	kLHRegShiftVideoOutputAnalogSelect  = 8,
	kRegShiftSDIOut6GbpsMode			= 16,
	kRegShiftSDIOut12GbpsMode			= 17,
	kRegShiftRGBLevelA					= 22,
	kRegShiftSDIOutLevelAtoLevelB		= 23,
	kLHIRegShiftSDIOut3GbpsMode			= 24,
	kLHIRegShiftSDIOutSMPTELevelBMode	= 25,
	kK2RegShiftVPIDInsertionEnable		= 26,
	kK2RegShiftVPIDInsertionOverwrite	= 27,
	kK2RegShiftSDIOutDS1AudioSelect		= 28,//30,
	kK2RegShiftSDIOutDS2AudioSelect		= 29,//31,

	//kK2RegConversionControl,
	kK2RegShiftConverterOutStandard		= 12,
	kK2RegShiftConverterOutRate			= 27,
	kK2RegShiftUpConvertMode			= 8,
	kK2RegShiftDownConvertMode			= 4,
	kK2RegShiftConverterInStandard		= 0,
	kK2RegShiftConverterInRate			= 23,
	kK2RegShiftConverterPulldown		= 6,
	kK2RegShiftUCPassLine21				= 16,
	kK2RegShiftUCAutoLine21				= 17,
	kK2RegShiftIsoConvertMode			= 20,
	kK2RegShiftDeinterlaceMode			= 15,
	kK2RegShiftEnableConverter			= 31,

	//kK2RegFrameSync1Control and kK2RegFrameSync2Control
	kK2RegShiftFrameSyncControlFrameDelay = 24,
	kK2RegShiftFrameSyncControlStandard = 8,
	kK2RegShiftFrameSyncControlGeometry = 4,
	kK2RegShiftFrameSyncControlFrameFormat = 0,

	//kK2RegXptSelectGroup1
	kK2RegShiftCompressionModInputSelect = 24,
	kK2RegShiftConversionModInputSelect = 16,
	kK2RegShiftColorSpaceConverterInputSelect = 8, 
	kK2RegShiftXptLUTInputSelect		= 0, 

	//kK2RegXptSelectGroup2
	kK2RegShiftDuallinkOutInputSelect	= 24,
	kK2RegShiftFrameSync2InputSelect	= 16,
	kK2RegShiftFrameSync1InputSelect	= 8,
	kK2RegShiftFrameBuffer1InputSelect	= 0, 

	//kK2RegXptSelectGroup3
	kK2RegShiftCSC1KeyInputSelect		= 24,
	kK2RegShiftSDIOut2InputSelect		= 16,
	kK2RegShiftSDIOut1InputSelect		= 8,
	kK2RegShiftAnalogOutInputSelect		= 0,

	//kK2RegXptSelectGroup4
	kK2RegShiftMixerBGKeyInputSelect	= 24,
	kK2RegShiftMixerBGVidInputSelect	= 16,
	kK2RegShiftMixerFGKeyInputSelect	= 8,
	kK2RegShiftMixerFGVidInputSelect	= 0,

	//kK2RegXptSelectGroup5
	kK2RegShiftCSC2KeyInputSelect		= 24,
	kK2RegShiftCSC2VidInputSelect		= 16,
	kK2RegShiftXptLUT2InputSelect		= 8,
	kK2RegShiftFrameBuffer2InputSelect	= 0,

	//kK2RegXptSelectGroup6
	kK2RegShiftSecondConverterInputSelect = 24,
	kK2RegShiftHDMIOutInputSelect		= 16,
	kK2RegShiftIICTInputSelect			= 8,
	kK2RegShiftWaterMarkerInputSelect	= 0,

	//kK2RegXptSelectGroup7
	kK2RegShiftDuallinkOut2InputSelect	= 16,
	kK2RegShiftIICT2InputSelect			= 8,
	kK2RegShiftWaterMarker2InputSelect	= 0,

	//kK2RegXptSelectGroup8
	kK2RegShiftSDIOut5InputSelect		= 16,
	kK2RegShiftSDIOut4InputSelect		= 8,
	kK2RegShiftSDIOut3InputSelect		= 0,
	
	//kRegCh1ControlExtended
	//kRegCh2ControlExtended
	kK2RegShiftPulldownMode	= 2,
	
	//kK2RegXptSelectGroup9
	kK2RegShiftMixer2BGKeyInputSelect	= 24,
	kK2RegShiftMixer2BGVidInputSelect	= 16,
	kK2RegShiftMixer2FGKeyInputSelect	= 8,
	kK2RegShiftMixer2FGVidInputSelect	= 0,

	//kK2RegXptSelectGroup10
	kK2RegShiftSDIOut2DS2InputSelect	= 8,
	kK2RegShiftSDIOut1DS2InputSelect	= 0,
	
	//kK2RegXptSelectGroup11
	kK2RegShiftDuallinkIn1InputSelect	= 0,
	kK2RegShiftDuallinkIn1DSInputSelect = 8,
	kK2RegShiftDuallinkIn2InputSelect	= 16,
	kK2RegShiftDuallinkIn2DSInputSelect = 24,

	//kK2RegXptSelectGroup12
	kK2RegShiftXptLUT3InputSelect		= 0,
	kK2RegShiftXptLUT4InputSelect		= 8,
	kK2RegShiftXptLUT5InputSelect		= 16,
	kK2RegShiftXpt3DLUT1InputSelect		= 24,

	//kK2RegXptSelectGroup13
	kK2RegShiftFrameBuffer3InputSelect	= 0,
	kK2RegShiftFrameBuffer4InputSelect	= 16,

	//kK2RegXptSelectGroup14
	kK2RegShiftSDIOut3DS2InputSelect	= 8,
	kK2RegShiftSDIOut5DS2InputSelect	= 16,
	kK2RegShiftSDIOut4DS2InputSelect	= 24,

	//kRegXptSelectGroup15
	kK2RegShiftDuallinkIn3InputSelect	= 0,
	kK2RegShiftDuallinkIn3DSInputSelect = 8,
	kK2RegShiftDuallinkIn4InputSelect	= 16,
	kK2RegShiftDuallinkIn4DSInputSelect = 24,

	//kRegXptSelectGroup16
	kK2RegShiftDuallinkOut3InputSelect	= 0,
	kK2RegShiftDuallinkOut4InputSelect	= 8,
	kK2RegShiftDuallinkOut5InputSelect	= 16,

	//kK2RegXptSelectGroup17
	kK2RegShiftCSC3VidInputSelect = 0,
	kK2RegShiftCSC3KeyInputSelect = 8,
	kK2RegShiftCSC4VidInputSelect = 16,
	kK2RegShiftCSC4KeyInputSelect = 24,

	//kRegXptSelectGroup18
	kK2RegShiftCSC5VidInputSelect = 0,
	kK2RegShiftCSC5KeyInputSelect = 8,

	//kRegXptSelectGroup19
	kK2RegShift4KDCQ1InputSelect = 0,
	kK2RegShift4KDCQ2InputSelect = 8,
	kK2RegShift4KDCQ3InputSelect = 16,
	kK2RegShift4KDCQ4InputSelect = 24,

	//kRegXptSelectGroup20
	kK2RegShiftHDMIOutV2Q1InputSelect = 0,
	kK2RegShiftHDMIOutV2Q2InputSelect = 8,
	kK2RegShiftHDMIOutV2Q3InputSelect = 16,
	kK2RegShiftHDMIOutV2Q4InputSelect = 24,

	//kK2RegXptSelectGroup21
	kK2RegShiftFrameBuffer5InputSelect	= 0,
	kK2RegShiftFrameBuffer6InputSelect	= 8,
	kK2RegShiftFrameBuffer7InputSelect	= 16,
	kK2RegShiftFrameBuffer8InputSelect	= 24,

	//kK2RegXptSelectGroup22
	kK2RegShiftSDIOut6InputSelect		= 0,
	kK2RegShiftSDIOut6DS2InputSelect	= 8,
	kK2RegShiftSDIOut7InputSelect		= 16,
	kK2RegShiftSDIOut7DS2InputSelect	= 24,

	//kK2RegXptSelectGroup30
	kK2RegShiftSDIOut8InputSelect		= 0,
	kK2RegShiftSDIOut8DS2InputSelect	= 8,
	kK2RegShiftCSC6VidInputSelect		= 16,
	kK2RegShiftCSC6KeyInputSelect		= 24,

	//kK2RegXptSelectGroup23
	kK2RegShiftCSC7VidInputSelect	= 0,
	kK2RegShiftCSC7KeyInputSelect	= 8,
	kK2RegShiftCSC8VidInputSelect	= 16,
	kK2RegShiftCSC8KeyInputSelect	= 24,

	//kK2RegXptSelectGroup24
	kK2RegShiftXptLUT6InputSelect	= 0,
	kK2RegShiftXptLUT7InputSelect	= 8,
	kK2RegShiftXptLUT8InputSelect	= 16,

	//kK2RegXptSelectGroup25
	kK2RegShiftDuallinkIn5InputSelect	= 0,
	kK2RegShiftDuallinkIn5DSInputSelect	= 8,
	kK2RegShiftDuallinkIn6InputSelect	= 16,
	kK2RegShiftDuallinkIn6DSInputSelect	= 24,

	//kK2RegXptSelectGroup26
	kK2RegShiftDuallinkIn7InputSelect	= 0,
	kK2RegShiftDuallinkIn7DSInputSelect	= 8,
	kK2RegShiftDuallinkIn8InputSelect	= 16,
	kK2RegShiftDuallinkIn8DSInputSelect	= 24,

	//kK2RegXptSelectGroup27
	kK2RegShiftDuallinkOut6InputSelect	= 0,
	kK2RegShiftDuallinkOut7InputSelect	= 8,
	kK2RegShiftDuallinkOut8InputSelect	= 16,

	//kK2RegXptSelectGroup28
	kK2RegShiftMixer3FGVidInputSelect	= 0,
	kK2RegShiftMixer3FGKeyInputSelect	= 8,
	kK2RegShiftMixer3BGVidInputSelect	= 16,
	kK2RegShiftMixer3BGKeyInputSelect	= 24,

	//kK2RegXptSelectGroup29
	kK2RegShiftMixer4FGVidInputSelect	= 0,
	kK2RegShiftMixer4FGKeyInputSelect	= 8,
	kK2RegShiftMixer4BGVidInputSelect	= 16,
	kK2RegShiftMixer4BGKeyInputSelect	= 24,

	//kRegXptSelectGroup31
	kK2RegShift425Mux1AInputSelect = 0,
	kK2RegShift425Mux1BInputSelect = 8,
	kK2RegShift425Mux2AInputSelect = 16,
	kK2RegShift425Mux2BInputSelect = 24,

	//kRegXptSelectGroup32
	kK2RegShift425Mux3AInputSelect = 0,
	kK2RegShift425Mux3BInputSelect = 8,
	kK2RegShift425Mux4AInputSelect = 16,
	kK2RegShift425Mux4BInputSelect = 24,

	//kRegXptSelectGroup33
	kK2RegShiftFrameBuffer1BInputSelect = 0,
	kK2RegShiftFrameBuffer2BInputSelect = 8,
	kK2RegShiftFrameBuffer3BInputSelect = 16,
	kK2RegShiftFrameBuffer4BInputSelect = 24,

	//kRegXptSelectGroup36
	kK2RegShiftMultiLinkOutInputSelect = 0,
	kK2RegShiftMultiLinkOutDS2InputSelect = 8,

	//kRegXptSelectGroup34
	kK2RegShiftFrameBuffer5BInputSelect = 0,
	kK2RegShiftFrameBuffer6BInputSelect = 8,
	kK2RegShiftFrameBuffer7BInputSelect = 16,
	kK2RegShiftFrameBuffer8BInputSelect = 24,

	//kK2RegCSCoefficients1_2
	kK2RegShiftVidKeySyncStatus			= 28,
	kK2RegShiftMakeAlphaFromKeySelect	= 29,
	kK2RegShiftColorSpaceMatrixSelect	= 30,
	kK2RegShiftUseCustomCoefSelect		= 31,
	//kK2RegCSCoefficients3_4
	kK2RegShiftXena2RGBRange			= 31,	
	
	//kK2RegCSCoefficients5_6,
	kK2RegShift2piCSC1					= 28,

	//kK2RegCSCoefficients5_6,
	kK2RegShift2piCSC5					= 28,

	kK2RegShiftCustomCoefficientLow		= 0,
	kK2RegShiftCustomCoefficientHigh	= 16,

	// Enhanced Color Space Converter contol
	kK2RegShiftEnhancedCSCInputPixelFormat   = 0,
	kK2RegShiftEnhancedCSCOutputPixelFormat  = 4,
	kK2RegShiftEnhancedCSCChromaEdgeControl  = 8,
	kK2RegShiftEnhancedCSCChromaFilterSelect = 12,
	kK2RegShiftEnhancedCSCEnable             = 29,
	kK2RegShiftEnhancedCSC4KMode             = 28,

	kK2RegShiftEnhancedCSCKeySource          = 0,
	kK2RegShiftEnhancedCSCKeyOutputRange     = 4,
	
	// Xena2K and Konax Video Processing.
	kK2RegShiftXena2FgVidProcInputControl	= 20,
	kK2RegShiftXena2BgVidProcInputControl	= 22,
	kK2RegShiftXena2VidProcMode				= 24,
	kK2RegShiftXena2VidProcSplitStd 		= 28,
	kK2RegShiftVidProcSubtitleEnable		= 31,

	// the newer 13(12) bit coefficients end on bit 14 unlike the
	// 11(10) bit ones on 16 - jac
	kK2RegShiftCustomCoefficient12BitLow  = 0,
	kK2RegShiftCustomCoefficient12BitHigh = 14,
	
	//kRegLTCStatusControl
	kRegShiftLTC1InPresent	= 0,
	kRegShiftLTC1Bypass = 4,
	kRegShiftLTC2InPresent	= 8,
	kRegShiftLTC2Bypass = 12,

	// kRegAudioOutputSourceMap
	kRegShiftMonitorSource					= 16,
	kRegShiftHDMIOutAudioSource				= 24,

	// kRegSDIInput3GStatus
	kRegShiftSDIIn3GbpsMode 			= 0,
	kRegShiftSDIIn3GbpsSMPTELevelBMode 	= 1,
	kRegShiftSDIIn1LevelBtoLevelA		= 2,
	kRegShiftSDIInVPIDLinkAValid 		= 4,
	kRegShiftSDIInVPIDLinkBValid 		= 5,
	kRegShiftSDIIn16GbpsMode			= 6,
	kRegShiftSDIIn112GbpsMode			= 7,
	kRegShiftSDIIn23GbpsMode 			= 8,
	kRegShiftSDIIn23GbpsSMPTELevelBMode = 9,
	kRegShiftSDIIn2LevelBtoLevelA		= 10,
	kRegShiftSDIIn2VPIDLinkAValid 		= 12,
	kRegShiftSDIIn2VPIDLinkBValid 		= 13,
	kRegShiftSDIIn26GbpsMode			= 14,
	kRegShiftSDIIn212GbpsMode			= 15,
	kRegShiftSDIInTsiMuxSyncFail		= 16,

	// kRegSDIInput3GStatus2
	kRegShiftSDIIn33GbpsMode 			= 0,
	kRegShiftSDIIn33GbpsSMPTELevelBMode	= 1,
	kRegShiftSDIIn3LevelBtoLevelA		= 2,
	kRegShiftSDIIn3VPIDLinkAValid 		= 4,
	kRegShiftSDIIn3VPIDLinkBValid 		= 5,
	kRegShiftSDIIn36GbpsMode			= 6,
	kRegShiftSDIIn312GbpsMode			= 7,
	kRegShiftSDIIn43GbpsMode 			= 8,
	kRegShiftSDIIn43GbpsSMPTELevelBMode = 9,
	kRegShiftSDIIn4LevelBtoLevelA		= 10,
	kRegShiftSDIIn4VPIDLinkAValid 		= 12,
	kRegShiftSDIIn4VPIDLinkBValid 		= 13,
	kRegShiftSDIIn46GbpsMode			= 14,
	kRegShiftSDIIn412GbpsMode			= 15,

	// kRegSDI5678Input3GStatus
	kRegShiftSDIIn53GbpsMode			= 0,
	kRegShiftSDIIn53GbpsSMPTELevelBMode	= 1,
	kRegShiftSDIIn5LevelBtoLevelA		= 2,
	kRegShiftSDIIn5VPIDLinkAValid		= 4,
	kRegShiftSDIIn5VPIDLinkBValid		= 5,
	kRegShiftSDIIn56GbpsMode			= 6,
	kRegShiftSDIIn512GbpsMode			= 7,
	kRegShiftSDIIn63GbpsMode			= 8,
	kRegShiftSDIIn63GbpsSMPTELevelBMode	= 9,
	kRegShiftSDIIn6LevelBtoLevelA		= 10,
	kRegShiftSDIIn6VPIDLinkAValid		= 12,
	kRegShiftSDIIn6VPIDLinkBValid		= 13,
	kRegShiftSDIIn66GbpsMode			= 14,
	kRegShiftSDIIn612GbpsMode			= 15,
	kRegShiftSDIIn73GbpsMode			= 16,
	kRegShiftSDIIn73GbpsSMPTELevelBMode	= 17,
	kRegShiftSDIIn7LevelBtoLevelA		= 18,
	kRegShiftSDIIn7VPIDLinkAValid		= 20,
	kRegShiftSDIIn7VPIDLinkBValid		= 21,
	kRegShiftSDIIn76GbpsMode			= 22,
	kRegShiftSDIIn712GbpsMode			= 23,
	kRegShiftSDIIn83GbpsMode			= 24,
	kRegShiftSDIIn83GbpsSMPTELevelBMode	= 25,
	kRegShiftSDIIn8LevelBtoLevelA		= 26,
	kRegShiftSDIIn8VPIDLinkAValid		= 28,
	kRegShiftSDIIn8VPIDLinkBValid		= 29,
	kRegShiftSDIIn86GbpsMode			= 30,
	kRegShiftSDIIn812GbpsMode			= 31,

	// kRegVPID
	kRegShiftVPIDBitDepth				= 0,
	kRegShiftVPIDLuminance				= 4,
	kRegShiftVPIDDualLinkChannel        = 5,
	kRegShiftVPIDChannel				= 6,
	kRegShiftVPIDSampling				= 8,
	kRegShiftVPIDColorimetry			= 12,
	kRegShiftVPIDColorimetryAltLow		= 12,
	kRegShiftVPIDImageAspect16x9Alt		= 13,
	kRegShiftVPIDHorizontalSampling		= 14,
	kRegShiftVPIDColorimetryAltHigh		= 15,
	kRegShiftVPIDImageAspect16x9		= 15,
	kRegShiftVPIDPictureRate			= 16,
	kRegShiftVPIDXferChars				= 20,
	kRegShiftVPIDProgressivePicture		= 22,
	kRegShiftVPIDProgressiveTransport	= 23,
	kRegShiftVPIDStandard				= 24,
	kRegShiftVPIDVersionID				= 31,
#if !defined (NTV2_DEPRECATE)
	kRegShiftVPIDDynamicRange			= 3,
#endif

	// Borg Test Pattern Generator
	kRegShiftTPGChromaSample            = 0,
	kRegShiftTPGLineBuffer              = 10,
	kRegShiftTPGFrameRate               = 12,
	kRegShiftTPGLuma                    = 16,
	kRegShiftTPGMulti                   = 26,
	kRegShiftTPGReset                   = 27,
	kRegShiftTPGStandard                = 28,
	kRegShiftTPGWriteEnable             = 31,

	// Audio Channel Control 2 or 8 channel playback shift
	kRegAudControlShift					= 5,

	// Stereo Compressor control shift
	kRegShiftStereoCompressorOutputMode		= 0,
	kRegShiftStereoCompressorFlipMode		= 4,
	kRegShiftStereoCompressorFlipLeftHorz	= 4,
	kRegShiftStereoCompressorFlipLeftVert	= 5,
	kRegShiftStereoCompressorFlipRightHorz	= 6,
	kRegShiftStereoCompressorFlipRightVert	= 7,
	kRegShiftStereoCompressorFormat			= 8,
	kRegShiftStereoCompressorLeftSource		= 16,
	kRegShiftStereoCompressorRightSource	= 24,

	// SDI Direction Control Shift
	kRegShiftSDI5Transmit	= 24,
	kRegShiftSDI6Transmit	= 25,
	kRegShiftSDI7Transmit	= 26,
	kRegShiftSDI8Transmit	= 27,
	kRegShiftSDI1Transmit	= 28,
	kRegShiftSDI2Transmit	= 29,
	kRegShiftSDI3Transmit	= 30,
	kRegShiftSDI4Transmit	= 31,
	
	// SDI watchdog control
	kRegShiftSDIRelayControl12		= 0,
	kRegShiftSDIRelayControl34		= 1,
	kRegShiftSDIWatchdogEnable12	= 4,
	kRegShiftSDIWatchdogEnable34	= 5,
	kRegShiftSDIRelayPosition12		= 8,
	kRegShiftSDIRelayPosition34		= 9,
	kRegShiftSDIWatchdogStatus		= 12,

	kShiftDisplayMode		= 4,

	// 4K Down Convert
	kRegShift4KDCRGBMode	= 0,
	kRegShift4KDCYCC444Mode	= 1,
	kRegShift4KDCPSFInMode	= 2,
	kRegShift4KDCPSFOutMode	= 3,

	// Quadrant Rasterizer Control
	kRegShiftRasterMode		= 0,
	kRegShiftTsiIO			= 2,
	kRegShiftRasterLevelB	= 4,
	kRegShiftRasterDecimate	= 8,

	//	kRegRXSDI1Status, kRegRXSDI2Status, etc
	kRegShiftSDIInUnlockCount		= 0,
	kRegShiftSDIInLocked			= 16,
	kRegShiftSDIInVpidValidA		= 20,
	kRegShiftSDIInVpidValidB		= 21,
	kRegShiftSDIInTRSError			= 24,
	kRegShiftSDIInCRCErrorCountA	= 0,
	kRegShiftSDIInCRCErrorCountB	= 16,

	kRegShift2MFrameSize		= 0,
	kRegShiftChannelBar			= 16,

	kRegShiftPCMControlA1P1_2		= 0,
	kRegShiftPCMControlA1P3_4		= 1,
	kRegShiftPCMControlA1P5_6		= 2,
	kRegShiftPCMControlA1P7_8		= 3,
	kRegShiftPCMControlA1P9_10		= 4,
	kRegShiftPCMControlA1P11_12		= 5,
	kRegShiftPCMControlA1P13_14		= 6,
	kRegShiftPCMControlA1P15_16		= 7,

	kRegShiftPCMControlA2P1_2		= 8,
	kRegShiftPCMControlA2P3_4		= 9,
	kRegShiftPCMControlA2P5_6		= 10,
	kRegShiftPCMControlA2P7_8		= 11,
	kRegShiftPCMControlA2P9_10		= 12,
	kRegShiftPCMControlA2P11_12		= 13,
	kRegShiftPCMControlA2P13_14		= 14,
	kRegShiftPCMControlA2P15_16		= 15,

	kRegShiftPCMControlA3P1_2		= 16,
	kRegShiftPCMControlA3P3_4		= 17,
	kRegShiftPCMControlA3P5_6		= 18,
	kRegShiftPCMControlA3P7_8		= 19,
	kRegShiftPCMControlA3P9_10		= 20,
	kRegShiftPCMControlA3P11_12		= 21,
	kRegShiftPCMControlA3P13_14		= 22,
	kRegShiftPCMControlA3P15_16		= 23,

	kRegShiftPCMControlA4P1_2		= 24,
	kRegShiftPCMControlA4P3_4		= 25,
	kRegShiftPCMControlA4P5_6		= 26,
	kRegShiftPCMControlA4P7_8		= 27,
	kRegShiftPCMControlA4P9_10		= 28,
	kRegShiftPCMControlA4P11_12		= 29,
	kRegShiftPCMControlA4P13_14		= 30,
	kRegShiftPCMControlA4P15_16		= 31,

	kRegShiftPCMControlA5P1_2		= 0,
	kRegShiftPCMControlA5P3_4		= 1,
	kRegShiftPCMControlA5P5_6		= 2,
	kRegShiftPCMControlA5P7_8		= 3,
	kRegShiftPCMControlA5P9_10		= 4,
	kRegShiftPCMControlA5P11_12		= 5,
	kRegShiftPCMControlA5P13_14		= 6,
	kRegShiftPCMControlA5P15_16		= 7,

	kRegShiftPCMControlA6P1_2		= 8,
	kRegShiftPCMControlA6P3_4		= 9,
	kRegShiftPCMControlA6P5_6		= 10,
	kRegShiftPCMControlA6P7_8		= 11,
	kRegShiftPCMControlA6P9_10		= 12,
	kRegShiftPCMControlA6P11_12		= 13,
	kRegShiftPCMControlA6P13_14		= 14,
	kRegShiftPCMControlA6P15_16		= 15,

	kRegShiftPCMControlA7P1_2		= 16,
	kRegShiftPCMControlA7P3_4		= 17,
	kRegShiftPCMControlA7P5_6		= 18,
	kRegShiftPCMControlA7P7_8		= 19,
	kRegShiftPCMControlA7P9_10		= 20,
	kRegShiftPCMControlA7P11_12		= 21,
	kRegShiftPCMControlA7P13_14		= 22,
	kRegShiftPCMControlA7P15_16		= 23,

	kRegShiftPCMControlA8P1_2		= 24,
	kRegShiftPCMControlA8P3_4		= 25,
	kRegShiftPCMControlA8P5_6		= 26,
	kRegShiftPCMControlA8P7_8		= 27,
	kRegShiftPCMControlA8P9_10		= 28,
	kRegShiftPCMControlA8P11_12		= 29,
	kRegShiftPCMControlA8P13_14		= 30,
	kRegShiftPCMControlA8P15_16		= 31,

	kRegFanHiShift = 16,
	kRegThermalShift = 17,

	//kRegHDMIHDR.... 330-336
	kRegShiftHDMIHDRGreenPrimaryX = 0,
	kRegShiftHDMIHDRGreenPrimaryY = 16,
	kRegShiftHDMIHDRBluePrimaryX = 0,
	kRegShiftHDMIHDRBluePrimaryY = 16,
	kRegShiftHDMIHDRRedPrimaryX = 0,
	kRegShiftHDMIHDRRedPrimaryY = 16,
	kRegShiftHDMIHDRWhitePointX = 0,
	kRegShiftHDMIHDRWhitePointY = 16,
	kRegShiftHDMIHDRMaxMasteringLuminance = 0,
	kRegShiftHDMIHDRMinMasteringLuminance = 16,
	kRegShiftHDMIHDRMaxContentLightLevel = 0,
	kRegShiftHDMIHDRMaxFrameAverageLightLevel = 16,

	kRegShiftHDMIHDRNonContantLuminance = 0,
    kRegShiftHDMIHDRDolbyVisionEnable = 6,
	kRegShiftHDMIHDREnable = 7,
	kRegShiftElectroOpticalTransferFunction = 16,
	kRegShiftHDRStaticMetadataDescriptorID = 24,

	kRegShiftAudioMixerMainInputSelect = 0,
	kRegShiftAudioMixerAux1x2CHInput = 4,
	kRegShiftAudioMixerAux2x2CHInput = 8,
	kRegShiftAudioMixerChannelSelect = 0,
	kRegShiftAudioMixerOutputChannelsMute =  0,
	kRegShiftAudioMixerOutputChannel1Mute =  0,
	kRegShiftAudioMixerOutputChannel2Mute = 1,
	kRegShiftAudioMixerOutputChannel3Mute = 2,
	kRegShiftAudioMixerOutputChannel4Mute = 3,
	kRegShiftAudioMixerOutputChannel5Mute = 4,
	kRegShiftAudioMixerOutputChannel6Mute = 5,
	kRegShiftAudioMixerOutputChannel7Mute = 6,
	kRegShiftAudioMixerOutputChannel8Mute = 7,
	kRegShiftAudioMixerOutputChannel9Mute = 8,
	kRegShiftAudioMixerOutputChannel10Mute = 9,
	kRegShiftAudioMixerOutputChannel11Mute = 10,
	kRegShiftAudioMixerOutputChannel12Mute = 11,
	kRegShiftAudioMixerOutputChannel13Mute = 12,
	kRegShiftAudioMixerOutputChannel14Mute = 13,
	kRegShiftAudioMixerOutputChannel15Mute = 14,
	kRegShiftAudioMixerOutputChannel16Mute = 15,
	kRegShiftAudioMixerMainInputEnable = 16,
	kRegShiftAudioMixerAux1InputEnable = 18,
	kRegShiftAudioMixerAux2InputEnable = 20,
	kRegShiftAudioMixerAux1Channel1Level = 0,
	kRegShiftAudioMixerAux1Channel2Level = 16,
	kRegShiftAudioMixerAux2Channel1Level = 0,
	kRegShiftAudioMixerAux2Channel2Level = 16,
	kRegShiftAudioMixerMainChannel1Level = 0,
	kRegShiftAudioMixerMainChannel2Level = 16,
	kRegShiftAudioMixerMainChannel3Level = 0,
	kRegShiftAudioMixerMainChannel4Level = 16,
	kRegShiftAudioMixerMainChannel5Level = 0,
	kRegShiftAudioMixerMainChannel6Level = 16,
	kRegShiftAudioMixerMainChannel7Level = 0,
	kRegShiftAudioMixerMainChannel8Level = 16,
	kRegShiftAudioMixerMainChannel9Level = 0,
	kRegShiftAudioMixerMainChannel10Level = 16,
	kRegShiftAudioMixerMainChannel11Level = 0,
	kRegShiftAudioMixerMainChannel12Level = 16,
	kRegShiftAudioMixerMainChannel13Level = 0,
	kRegShiftAudioMixerMainChannel14Level = 16,
	kRegShiftAudioMixerMainChannel15Level = 0,
	kRegShiftAudioMixerMainChannel16Level = 16,
	kRegShiftAudioMixerMainMixedOutputChannel1Level = 0,
	kRegShiftAudioMixerMainMixedOutputChannel2Level = 16,
	kRegShiftAudioMixerInputLeftLevel = 0,
	kRegShiftAudioMixerInputRightLevel = 16,
	kRegShiftAudioMixerLevelSampleCount = 8,

    kRegShiftRotaryEncoderValue = 0,
    kRegShiftRotaryEncoderGain = 8,
	
	kRegShiftLUTAddress = 0,
	kRegShiftLUTLoad = 31,

	kRegShiftMRStandard = 0,
	kRegShiftMRBypass = 20,
	kRegShiftMREnable = 24,
	kRegShiftMRSupport = 0

#if !defined (NTV2_DEPRECATE)
	,
	// kRegSDIInput3GStatus
	kLHIRegShiftSDIIn3GbpsMode				= 0,
	kLHIRegShiftSDIIn3GbpsSMPTELevelBMode 	= 1,
	kLHIRegShiftSDIInVPIDLinkAValid 		= 4,
	kLHIRegShiftSDIInVPIDLinkBValid 		= 5,
	kLHIRegShiftSDIIn23GbpsMode 			= 8,
	kLHIRegShiftSDIIn23GbpsSMPTELevelBMode 	= 9,
	kLHIRegShiftSDIIn2VPIDLinkAValid 		= 12,
	kLHIRegShiftSDIIn2VPIDLinkBValid 		= 13,

	// kRegSDIInput3GStatus2
	kLHIRegShiftSDIIn33GbpsMode 			= 0,
	kLHIRegShiftSDIIn33GbpsSMPTELevelBMode 	= 1,
	kLHIRegShiftSDIIn3VPIDLinkAValid 		= 4,
	kLHIRegShiftSDIIn3VPIDLinkBValid 		= 5,
	kLHIRegShiftSDIIn43GbpsMode 			= 8,
	kLHIRegShiftSDIIn43GbpsSMPTELevelBMode 	= 9,
	kLHIRegShiftSDIIn4VPIDLinkAValid 		= 12,
	kLHIRegShiftSDIIn4VPIDLinkBValid 		= 13,
	//
	// Borg Fusion Registers
	//

	// Boot FPGA and BoardID
	kRegShiftBorgFusionBootFPGAVer = 4,
	kRegShiftBorgFusionBoardID = 0,

	// Codec and convert FPGA configuration control
	kRegShiftBorgFusionCodecFPGAProgram = 0,
	kRegShiftBorgFusionCodecFPGAInit = 1,
	kRegShiftBorgFusionCodecFPGADone = 2,

	kRegShiftBorgFusionConvertFPGAProgram = 4,
	kRegShiftBorgFusionConvertFPGAInit = 5,
	kRegShiftBorgFusionConvertFPGADone = 6,

	// Panel Push buttons debounced and SATA drive present state
	kRegShiftBorgFusionPushButtonStatusDebounced = 0,
	kRegShiftBorgFusionPushButtonConfigDebounced = 1,
	kRegShiftBorgFusionPushButtonMediaDebounced = 2,
	kRegShiftBorgFusionPushButtonRewindDebounced = 3,
	kRegShiftBorgFusionPushButtonStopDebounced = 4,
	kRegShiftBorgFusionPushButtonPlayDebounced = 5,
	kRegShiftBorgFusionPushButtonRecordDebounced = 6,
	kRegShiftBorgFusionPushButtonFastFwdDebounced = 7,
	kRegShiftBorgFusionPushButtonSelectUpDebounced = 8,
	kRegShiftBorgFusionPushButtonSelectDownDebounced = 9,
	kRegShiftBorgFusionPushButtonDeleteClipDebounced = 10,
	kRegShiftBorgFusionPushButtonAdjustUpDebounced = 11,
	kRegShiftBorgFusionPushButtonAdjustDownDebounced = 12,
	kRegShiftBorgFusionPushButtonSlotDebounced = 13,
	kRegShiftBorgFusionPushButtonSATADrivePresentDebounced = 14,

	// Panel Push buttons and SATA drive present changes
	kRegShiftBorgFusionPushButtonStatusChange = 0,
	kRegShiftBorgFusionPushButtonConfigChange = 1,
	kRegShiftBorgFusionPushButtonMediaChange = 2,
	kRegShiftBorgFusionPushButtonRewindChange = 3,
	kRegShiftBorgFusionPushButtonStopChange = 4,
	kRegShiftBorgFusionPushButtonPlayChange = 5,
	kRegShiftBorgFusionPushButtonRecordChange = 6,
	kRegShiftBorgFusionPushButtonFastFwdChange = 7,
	kRegShiftBorgFusionPushButtonSelectUpChange = 8,
	kRegShiftBorgFusionPushButtonSelectDownChange = 9,
	kRegShiftBorgFusionPushButtonDeleteClipChange = 10,
	kRegShiftBorgFusionPushButtonAdjustUpChange = 11,
	kRegShiftBorgFusionPushButtonAdjustDownChange = 12,
	kRegShiftBorgFusionPushButtonSlotChange = 13,
	kRegShiftBorgFusionPushButtonSATADrivePresentChange = 14,

	// LED Pulse Width Modulation Threshholds
	kRegShiftBorgFusionPWMThreshExpressCard2 = 0,
	kRegShiftBorgFusionPWMThreshExpressCard1 = 4,
	kRegShiftBorgFusionPWMThreshPower = 8,
	kRegShiftBorgFusionPWMThreshLCDBacklightLED = 12,


	// Power control - System
	kRegShiftBorgFusionPowerCtrlWiFiReset = 0,
	kRegShiftBorgFusionPowerCtrlFirewirePower = 1,
	kRegShiftBorgFusionPowerCtrlGigEthReset = 2,
	kRegShiftBorgFusionPowerCtrlPCIExpClockStop = 3,

	// Power control - Storage devices
	kRegShiftBorgFusionPowerCtrlPCIExpCard1_3_3vPower = 8,	// Express Card 1 3.3v power
	kRegShiftBorgFusionPowerCtrlPCIExpCard1_1_5vPower = 9,	// Express Card 1 1.5v power
	kRegShiftBorgFusionPowerCtrlPCIExpCard2_3_3vPower = 10,	// Express Card 2 3.3v power
	kRegShiftBorgFusionPowerCtrlPCIExpCard2_1_5vPower = 11,	// Express Card 2 1.5v power
	kRegShiftBorgFusionPowerCtrlSata_12vPower = 12,			// SATA Drive 12v power

	kRegShiftBonesActelPowerCtrlCFSlot2_BridgeReset = 8,
	kRegShiftBonesActelPowerCtrlCFSlot2_Power = 9,   // Compact Flash S2 Power
	kRegShiftBonesActelPowerCtrlCFSlot1_Power = 10,  // Compact Flash S1 Power
	kRegShiftBonesActelPowerCtrlCFSlot1_BridgeReset = 11,

	// Power control - Pushbutton LEDs
	kRegShiftBorgFusionPowerCtrlPCIExpCard2LED = 13,
	kRegShiftBorgFusionPowerCtrlPCIExpCard1LED = 14,
	kRegShiftBorgFusionPowerCtrlPowerButtonLED = 15,

	// IRQ3n Interrupt control
	kRegShiftBorgFusionIRQ3nIntCtrlPushButtonChangeEnable = 0,
	kRegShiftBorgFusionIRQ3nIntCtrlInputVoltageLow9vEnable = 1,
	kRegShiftBorgFusionIRQ3nIntCtrlDisplayFIFOFullEnable = 2,
	kRegShiftBorgFusionIRQ3nIntCtrlSATAPresentChangeEnable = 3,
	kRegShiftBorgFusionIRQ3nIntCtrlTemp1HighEnable = 4,
	kRegShiftBorgFusionIRQ3nIntCtrlTemp2HighEnable = 5,
	kRegShiftBorgFusionIRQ3nIntCtrlPowerButtonChangeEnable = 6,

	// IRQ3n Interrupt source
	kRegShiftBorgFusionIRQ3nIntSrcPushButtonChange= 0,
	kRegShiftBorgFusionIRQ3nIntSrcInputVoltageLow9v= 1,
	kRegShiftBorgFusionIRQ3nIntSrcDisplayFIFOFull= 2,
	kRegShiftBorgFusionIRQ3nIntSrcSATAPresentChange= 3,
	kRegShiftBorgFusionIRQ3nIntSrcTemp1High= 4,
	kRegShiftBorgFusionIRQ3nIntSrcTemp2High= 5,
	kRegShiftBorgFusionIRQ3nIntSrcPowerButtonChange= 6,

	// Noritake Display Control/Status
	kRegShiftBorgFusionDisplayCtrlReset = 0,
	kRegShiftBorgFusionDisplayStatusBusyRaw = 1,		// Not needed by CPU, used internally by FPGA
	kRegShiftBorgFusionDisplayStatusInterfaceBusy = 7,	// FIFO full

	// Analog ADC flags - battery
	kRegShiftBorgFusionAnalogFlagsPowerLTE9v = 0,	// +12 v supply <= 9.0 v battery critical
	kRegShiftBorgFusionAnalogFlagsPowerLTE10v = 1,	// +12 v supply <= 10.0 v battery depleting
	kRegShiftBorgFusionAnalogFlagsPowerLTE11v = 2,	// +12 v supply <= 11.0 v battery depleting
	kRegShiftBorgFusionAnalogFlagsPowerGTE13v = 3,	// +12 v supply >= 13.0 v battery charging

	// Analog ADC flags - temperature sensor
	kRegShiftBorgFusionAnalogFlagsPowerTemp1High = 4,	// Temp sensor 1 > 65 C
	kRegShiftBorgFusionAnalogFlagsPowerTemp2High = 5,	// Temp sensor 2 > 65 C
	// Pan (2K crop) - Xena 2
	kRegShiftPanMode					= 30,
	kRegShiftPanOffsetV					= 0,
	kRegShiftPanOffsetH					= 12
#endif	//	!defined (NTV2_DEPRECATE)

} RegisterShift;


// NWL Registers

// For the Mac we define an offset.  This is done so that read/write register can easily identify an NWL register from a VP register.  
// This is necessary since the NWL registers lives in another PCI BAR and these offsets conflict with normal VP registers.  With this 
// offset the driver knows which mapped BAR to use.  Windows maps individual registers at start so this isn't necessary for Windows.
#ifdef AJAMac
	#define NWL_REG_START 18000
#else
	#define NWL_REG_START 0
#endif

typedef enum
{
	kRegNwlS2C1Capabilities					= 0x0000+(NWL_REG_START),
	kRegNwlS2C1ControlStatus				= 0x0001+(NWL_REG_START),
	kRegNwlS2C1ChainStartAddressLow			= 0x0002+(NWL_REG_START),
	kRegNwlS2C1ChainStartAddressHigh		= 0x0003+(NWL_REG_START),
	kRegNwlS2C1HardwareTime					= 0x0004+(NWL_REG_START),
	kRegNwlS2C1ChainCompleteByteCount		= 0x0005+(NWL_REG_START),

	kRegNwlS2C2Capabilities					= 0x0040+(NWL_REG_START),
	kRegNwlS2C2ControlStatus				= 0x0041+(NWL_REG_START),
	kRegNwlS2C2ChainStartAddressLow			= 0x0042+(NWL_REG_START),
	kRegNwlS2C2ChainStartAddressHigh		= 0x0043+(NWL_REG_START),
	kRegNwlS2C2HardwareTime					= 0x0044+(NWL_REG_START),
	kRegNwlS2C2ChainCompleteByteCount		= 0x0045+(NWL_REG_START),

	kRegNwlC2S1Capabilities					= 0x0800+(NWL_REG_START),
	kRegNwlC2S1ControlStatus				= 0x0801+(NWL_REG_START),
 	kRegNwlC2S1ChainStartAddressLow			= 0x0802+(NWL_REG_START),
	kRegNwlC2S1ChainStartAddressHigh		= 0x0803+(NWL_REG_START),
	kRegNwlC2S1HardwareTime					= 0x0804+(NWL_REG_START),
	kRegNwlC2S1ChainCompleteByteCount		= 0x0805+(NWL_REG_START),

	kRegNwlC2S2Capabilities					= 0x0840+(NWL_REG_START),
	kRegNwlC2S2ControlStatus				= 0x0841+(NWL_REG_START),
	kRegNwlC2S2ChainStartAddressLow			= 0x0842+(NWL_REG_START),
	kRegNwlC2S2ChainStartAddressHigh		= 0x0843+(NWL_REG_START),
	kRegNwlC2S2HardwareTime					= 0x0844+(NWL_REG_START),
	kRegNwlC2S2ChainCompleteByteCount		= 0x0845+(NWL_REG_START),

	kRegNwlCommonControlStatus				= 0x1000+(NWL_REG_START),
	kRegNwlCommonBackEndCoreVersion			= 0x1001+(NWL_REG_START),
	kRegNwlCommonPCIExpressCoreVersion		= 0x1002+(NWL_REG_START),
	kRegNwlCommonUserVersion		   		= 0x1003+(NWL_REG_START)

} NwlRegisterNum;

// This is an aid for utility routines that maintain tables indexed by a register number, like spinlocks
typedef enum
{
	kRegNwlS2C1CapabilitiesIndex,				// 0
	kRegNwlS2C1ControlStatusIndex,				// 1
	kRegNwlS2C1ChainStartAddressLowIndex,		// 2
	kRegNwlS2C1ChainStartAddressHighIndex,		// 3
	kRegNwlS2C1HardwareTimeIndex,				// 4
	kRegNwlS2C1ChainCompleteByteCountIndex,		// 5

	kRegNwlS2C2CapabilitiesIndex,				// 6
	kRegNwlS2C2ControlStatusIndex,				// 7
	kRegNwlS2C2ChainStartAddressLowIndex,		// 8
	kRegNwlS2C2ChainStartAddressHighIndex,		// 9
	kRegNwlS2C2HardwareTimeIndex,				// 10
	kRegNwlS2C2ChainCompleteByteCountIndex,		// 11

	kRegNwlC2S1CapabilitiesIndex,				// 12
	kRegNwlC2S1ControlStatusIndex,				// 13
	kRegNwlC2S1ChainStartAddressLowIndex,		// 14
	kRegNwlC2S1ChainStartAddressHighIndex,		// 15
	kRegNwlC2S1HardwareTimeIndex,				// 16
	kRegNwlC2S1ChainCompleteByteCountIndex,		// 17

	kRegNwlC2S2CapabilitiesIndex,				// 18
	kRegNwlC2S2ControlStatusIndex,				// 19
	kRegNwlC2S2ChainStartAddressLowIndex,		// 20
	kRegNwlC2S2ChainStartAddressHighIndex,		// 21
	kRegNwlC2S2HardwareTimeIndex,				// 22
	kRegNwlC2S2ChainCompleteByteCountIndex,		// 23

	kRegNwlCommonControlStatusIndex,			// 24
	kRegNwlCommonBackEndCoreVersionIndex,		// 25
	kRegNwlCommonPCIExpressCoreVersionIndex,	// 26
	kRegNwlCommonUserVersionIndex,		   		// 27

	NUM_NWL_REGS
} NwlRegisterIndex;

typedef enum
{
	kRegMaskNwlCapabilitiesPresent				= BIT(0),
	kRegMaskNwlCapabilitiesEngineDirection		= BIT(1),
	kRegMaskNwlCapabilitiesEngineType			= (BIT(2)+BIT(3)),
	kRegMaskNwlCapabilitiesEngineNumber			= (BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15)),
	kRegMaskNwlCapabilitiesAddressSize			= (BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)),

	kRegMaskNwlControlStatusInterruptEnable		= BIT(0),
	kRegMaskNwlControlStatusInterruptActive		= BIT(1),
	kRegMaskNwlControlStatusChainStart			= BIT(8),
	kRegMaskNwlControlStatusDmaResetRequest		= BIT(9),
	kRegMaskNwlControlStatusChainRunning		= BIT(10),
	kRegMaskNwlControlStatusChainComplete		= BIT(11),
	kRegMaskNwlControlStatusChainErrorShort		= BIT(12),
	kRegMaskNwlControlStatusChainSoftwareShort	= BIT(13),
	kRegMaskNwlControlStatusChainHardwareShort	= BIT(14),
	kRegMaskNwlControlStatusAlignmentError		= BIT(15),
	kRegMaskNwlControlStatusDmaReset			= BIT(23),

	kRegMaskNwlCommonDmaInterruptEnable			= BIT(0),
	kRegMaskNwlCommonDmaInterruptActive 		= BIT(1),
	kRegMaskNwlCommonDmaInterruptPending 		= BIT(2),
	kRegMaskNwlCommonInterruptMode 				= BIT(3),
	kRegMaskNwlCommonUserInterruptEnable 		= BIT(4),
	kRegMaskNwlCommonUserInterruptActive 		= BIT(5),
	kRegMaskNwlCommonMaxPayloadSize 			= (BIT(8)+BIT(9)+BIT(10)),
	kRegMaskNwlCommonMaxReadRequestSize 		= (BIT(12)+BIT(13) +BIT(14)),
	kRegMaskNwlCommonS2CInterruptStatus0 		= BIT(16),
	kRegMaskNwlCommonS2CInterruptStatus1 		= BIT(17),
	kRegMaskNwlCommonS2CInterruptStatus2 		= BIT(18),
	kRegMaskNwlCommonS2CInterruptStatus3 		= BIT(19),
	kRegMaskNwlCommonS2CInterruptStatus4 		= BIT(20),
	kRegMaskNwlCommonS2CInterruptStatus5 		= BIT(21),
	kRegMaskNwlCommonS2CInterruptStatus6 		= BIT(22),
	kRegMaskNwlCommonS2CInterruptStatus7 		= BIT(23),
	kRegMaskNwlCommonC2SInterruptStatus0 		= BIT(24),
	kRegMaskNwlCommonC2SInterruptStatus1 		= BIT(25),
	kRegMaskNwlCommonC2SInterruptStatus2 		= BIT(26),
	kRegMaskNwlCommonC2SInterruptStatus3 		= BIT(27),
	kRegMaskNwlCommonC2SInterruptStatus4 		= BIT(28),
	kRegMaskNwlCommonC2SInterruptStatus5 		= BIT(29),
	kRegMaskNwlCommonC2SInterruptStatus6 		= BIT(30),
	kRegMaskNwlCommonC2SInterruptStatus7 		= BIT(31)
   
} NwlRegisterMask;

typedef enum
{
	kRegShiftNwlCapabilitiesPresent				= 0,
	kRegShiftNwlCapabilitiesEngineDirection		= 1,
	kRegShiftNwlCapabilitiesEngineType			= 2,
	kRegShiftNwlCapabilitiesEngineNumber		= 8,
	kRegShiftNwlCapabilitiesAddressSize			= 16,

	kRegShiftNwlControlStatusInterruptEnable	= 0,
	kRegShiftNwlControlStatusInterruptActive	= 1,
	kRegShiftNwlControlStatusChainStart			= 8,
	kRegShiftNwlControlStatusDmaResetRequest	= 9,
	kRegShiftNwlControlStatusChainRunning		= 10,
	kRegShiftNwlControlStatusChainComplete		= 11,
	kRegShiftNwlControlStatusChainErrorShort	= 12,
	kRegShiftNwlControlStatusChainSoftwareShort	= 13,
	kRegShiftNwlControlStatusChainHardwareShort	= 14,
	kRegShiftNwlControlStatusAlignmentError		= 15,
	kRegShiftNwlControlStatusDmaReset			= 23,

	kRegShiftNwlCommonDmaInterruptEnable		= 0,
	kRegShiftNwlCommonDmaInterruptActive 		= 1,
	kRegShiftNwlCommonDmaInterruptPending 		= 2,
	kRegShiftNwlCommonInterruptMode 			= 3,
	kRegShiftNwlCommonUserInterruptEnable 		= 4,
	kRegShiftNwlCommonUserInterruptActive 		= 5,
	kRegShiftNwlCommonMaxPayloadSize 			= 8,
	kRegShiftNwlCommonMaxReadRequestSize 		= 12,
	kRegShiftNwlCommonS2CInterruptStatus0 		= 16,
	kRegShiftNwlCommonS2CInterruptStatus1 		= 17,
	kRegShiftNwlCommonS2CInterruptStatus2 		= 18,
	kRegShiftNwlCommonS2CInterruptStatus3 		= 19,
	kRegShiftNwlCommonS2CInterruptStatus4 		= 20,
	kRegShiftNwlCommonS2CInterruptStatus5 		= 21,
	kRegShiftNwlCommonS2CInterruptStatus6 		= 22,
	kRegShiftNwlCommonS2CInterruptStatus7 		= 23,
	kRegShiftNwlCommonC2SInterruptStatus0 		= 24,
	kRegShiftNwlCommonC2SInterruptStatus1 		= 25,
	kRegShiftNwlCommonC2SInterruptStatus2 		= 26,
	kRegShiftNwlCommonC2SInterruptStatus3 		= 27,
	kRegShiftNwlCommonC2SInterruptStatus4 		= 28,
	kRegShiftNwlCommonC2SInterruptStatus5 		= 29,
	kRegShiftNwlCommonC2SInterruptStatus6 		= 30,
	kRegShiftNwlCommonC2SInterruptStatus7 		= 31

} NwlRegisterShift;

// XLNX Registers
#define XLNX_MAX_CHANNELS						4
#define XLNX_REG_TARGET_SIZE					0x400
#define XLNX_REG_CHANNEL_SIZE					0x40
#define XLNX_SUBSYSTEM_ID						0x1fc

typedef enum
{
	kRegXlnxTargetChannelH2C					= 0x0,
	kRegXlnxTargetChannelC2H					= 0x1,
	kRegXlnxTargetIRQ							= 0x2,
	kRegXlnxTargetConfig						= 0x3,
	kRegXlnxTargetSgdmaH2C						= 0x4,
	kRegXlnxTargetSgdmaC2H						= 0x5,
	kRegXlnxTargetSgdmaCommon					= 0x6,
	kRegXlnxTargetMsiX							= 0x8
} XlnxRegisterTarget;

typedef enum
{
	kRegXlnxChannelIdentifier					= 0x00,
	kRegXlnxChannelControl						= 0x01,
	kRegXlnxChannelControlW1S					= 0x02,
	kRegXlnxChannelControlW1C					= 0x03,
	kRegXlnxChannelStatus						= 0x10,
	kRegXlnxChannelStatusRC						= 0x11,
	kRegXlnxChannelDescCompleteCount			= 0x12,
	kRegXlnxChannelAlignments					= 0x13,
	kRegXlnxChannelPollModeAddressLow			= 0x22,
	kRegXlnxChannelPollModeAddressHigh			= 0x23,
	kRegXlnxChannelInterruptEnable				= 0x24,
	kRegXlnxChannelInterruptEnableW1S			= 0x25,
	kRegXlnxChannelInterruptEnableW1C			= 0x26,
	kRegXlnxChannelPerfControl					= 0x30,
	kRegXlnxChannelPerfCycleCountLow			= 0x31,
	kRegXlnxChannelPerfCycleCountHigh			= 0x32,
	kRegXlnxChannelPerfDataCountLow				= 0x33,
	kRegXlnxChannelPerfDataCountHigh			= 0x34,
	kRegXlnxChannelUserMaxReadRequestSize		= 0x44,

	kRegXlnxIrqIdentifier						= 0x00,
	kRegXlnxIrqUserInterruptEnable				= 0x01,
	kRegXlnxIrqUserInterruptEnableW1S			= 0x02,
	kRegXlnxIrqUserInterruptEnableW1C			= 0x03,
	kRegXlnxIrqChannelInterruptEnable			= 0x04,
	kRegXlnxIrqChannelInterruptEnableW1S		= 0x05,
	kRegXlnxIrqChannelInterruptEnableW1C		= 0x06,
	kRegXlnxIrqUserInterruptRequest				= 0x10,
	kRegXlnxIrqChannelInterruptRequest			= 0x11,
	kRegXlnxIrqUserInterruptPending				= 0x12,
	kRegXlnxIrqChannelInterruptPending			= 0x13,

	kRegXlnxSgdmaIdentifier						= 0x00,
	kRegXlnxSgdmaDescAddressLow					= 0x20,
	kRegXlnxSgdmaDescAddressHigh				= 0x21,
	kRegXlnxSgdmaDescAdjacent					= 0x22,
	kRegXlnxSgdmaDescCredits					= 0x23

} XlnxRegisterNum;

typedef enum
{
	kRegMaskXlnxSubsystemId						= 0xfff00000,
	kRegMaskXlnxTarget							= 0x000f0000,
	kRegMaskXlnxStreamInterface					= 0x00008000,
	kRegMaskXlnxIdTarget						= 0x00000f00,
	kRegMaskXlnxVersion							= 0x000000ff,

	kRegMaskXlnxStreamWriteBackDisable			= 0x08000000,
	kRegMaskXlnxPollModeWriteBackEnable			= 0x04000000,
	kRegMaskXlnxNonIncAddressMode				= 0x02000000,
	kRegMaskXlnxRun								= 0x00000001,

	kRegMaskXlnxAddressAlignment				= 0x00ff0000,
	kRegMaskXlnxTransferAlignment				= 0x0000ff00,
	kRegMaskXlnxAddressBits						= 0x000000ff,

	kRegMaskXlnxIntDescError					= 0x00f80000,
	kRegMaskXlnxIntWriteError					= 0x0007c000,
	kRegMaskXlnxIntReadError					= 0x00003e00,
	kRegMaskXlnxIntIdleStop						= 0x00000040,
	kRegMaskXlnxIntInvalidLength				= 0x00000020,
	kRegMaskXlnxIntMagicStop					= 0x00000010,
	kRegMaskXlnxIntAlignMismatch				= 0x00000008,
	kRegMaskXlnxIntDescComplete					= 0x00000004,
	kRegMaskXlnxIntDescStop						= 0x00000002,

	kRegMaskXlnxPerfRun							= 0x00000004,
	kRegMaskXlnxPerfClear						= 0x00000002,
	kRegMaskXlnxPerfAuto						= 0x00000001,

	kRegMaskXlnxPerfCycleCountMax				= 0x00010000,
	kRegMaskXlnxPerfCycleCountHigh				= 0x000003ff,

	kRegMaskXlnxPerfDataCountMax				= 0x00010000,
	kRegMaskXlnxPerfDataCountHigh				= 0x000003ff,

	kRegMaskXlnxUserMaxReadRequestEff			= 0x00000070,
	kRegMaskXlnxUserMaxReadRequestPgm			= 0x00000007

} XlnxRegisterMask;

typedef enum
{
	kRegShiftXlnxSubsystemId					= 20,
	kRegShiftXlnxTarget							= 16,
	kRegShiftXlnxStreamInterface				= 15,
	kRegShiftXlnxIdTarget						= 8,
	kRegShiftXlnxVersion			   			= 0,

	kRegShiftXlnxStreamWriteBackDisable			= 27,
	kRegShiftXlnxPollModeWriteBackEnable		= 26,
	kRegShiftXlnxNonIncAddressMode				= 25,
	kRegShiftXlnxRun							= 0,

	kRegShiftXlnxAddressAlignment				= 16,
	kRegShiftXlnxTransferAlignment				= 8,
	kRegShiftXlnxAddressBits					= 0,

	kRegShiftXlnxIntDescError					= 19,
	kRegShiftXlnxIntWriteError					= 14,
	kRegShiftXlnxIntReadError					= 9,
	kRegShiftXlnxIntIdleStop					= 6,
	kRegShiftXlnxIntInvalidLength				= 5,
	kRegShiftXlnxIntMagicStop					= 4,
	kRegShiftXlnxIntAlignMismatch				= 3,
	kRegShiftXlnxIntDescComplete				= 2,
	kRegShiftXlnxIntDescStop					= 1,

	kRegShiftXlnxPerfRun						= 2,
	kRegShiftXlnxPerfClear						= 1,
	kRegShiftXlnxPerfAuto						= 0,

	kRegShiftXlnxPerfCycleCountMax				= 16,
	kRegShiftXlnxPerfCycleCountHigh				= 0,

	kRegShiftXlnxPerfDataCountMax				= 16,
	kRegShiftXlnxPerfDataCountHigh				= 0,

	kRegShiftXlnxUserMaxReadRequestEff			= 4,
	kRegShiftXlnxUserMaxReadRequestPgm			= 0

} XlnxRegisterShift;

// P2P Registers

typedef enum
{
	kRegMaskMessageInterruptStatusChannel1		= BIT(0),
	kRegMaskMessageInterruptStatusChannel2		= BIT(1),
	kRegMaskMessageInterruptStatusChannel3		= BIT(2),
	kRegMaskMessageInterruptStatusChannel4		= BIT(3),
	kRegMaskMessageInterruptControlEnable1		= BIT(0),
	kRegMaskMessageInterruptControlEnable2		= BIT(1),
	kRegMaskMessageInterruptControlEnable3		= BIT(2),
	kRegMaskMessageInterruptControlEnable4		= BIT(3),
	kRegMaskMessageInterruptControlEnable5		= BIT(4),
	kRegMaskMessageInterruptControlEnable6		= BIT(5),
	kRegMaskMessageInterruptControlEnable7		= BIT(6),
	kRegMaskMessageInterruptControlEnable8		= BIT(7),
	kRegMaskMessageInterruptControlClear1		= BIT(16),
	kRegMaskMessageInterruptControlClear2		= BIT(17),
	kRegMaskMessageInterruptControlClear3		= BIT(18),
	kRegMaskMessageInterruptControlClear4		= BIT(19),
	kRegMaskMessageInterruptControlClear5		= BIT(20),
	kRegMaskMessageInterruptControlClear6		= BIT(21),
	kRegMaskMessageInterruptControlClear7		= BIT(22),
	kRegMaskMessageInterruptControlClear8		= BIT(23)
} MessageRegisterMask;


typedef enum
{
	kRegShiftMessageInterruptStatusChannel1		= 0,
	kRegShiftMessageInterruptStatusChannel2		= 1,
	kRegShiftMessageInterruptStatusChannel3		= 2,
	kRegShiftMessageInterruptStatusChannel4		= 3,
	kRegShiftMessageInterruptControlEnable1		= 0,
	kRegShiftMessageInterruptControlEnable2		= 1,
	kRegShiftMessageInterruptControlEnable3		= 2,
	kRegShiftMessageInterruptControlEnable4		= 3,
	kRegShiftMessageInterruptControlClear1		= 16,
	kRegShiftMessageInterruptControlClear2		= 17,
	kRegShiftMessageInterruptControlClear3		= 18,
	kRegShiftMessageInterruptControlClear4		= 19
} MessageRegisterShift;


#define	kTransferFlagVideoDMA1			(BIT(0))	// use dma channel 1 for video transfer
#define	kTransferFlagVideoDMA2			(BIT(1))	// use dma channel 2 for video transfer
#define	kTransferFlagVideoDMA3			(BIT(2))	// use dma channel 3 for video tranfer
#define	kTransferFlagVideoDMA4			(BIT(3))	// use dma channel 4 for video transfer
#define	kTransferFlagVideoDMAAny		(BIT(0)+BIT(1)+BIT(2)+BIT(3))
#define	kTransferFlagAudioDMA1			(BIT(4))	// use dma channel 1 for audio transfer
#define	kTransferFlagAudioDMA2			(BIT(5))	// use dma channel 2 for audio transfer
#define	kTransferFlagAudioDMA3			(BIT(6))	// use dma channel 3 for audio transfer
#define	kTransferFlagAudioDMA4			(BIT(7))	// use dma channel 4 for audio transfer
#define	kTransferFlagAudioDMAAny		(BIT(4)+BIT(5)+BIT(6)+BIT(7))
#define kTransferFlagDMAAny				(BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7))
#define kTransferFlagQuadFrame			(BIT(8))	// transfer a quad hd or 4k frame
#define kTransferFlagP2PPrepare			(BIT(9))	// prepare p2p target for synchronous transfer (no message)
#define kTransferFlagP2PComplete		(BIT(10))	// complete synchronous p2p transfer
#define kTransferFlagP2PTarget			(BIT(11))	// prepare p2p target for asynchronous transfer (with message)
#define kTransferFlagP2PTransfer		(BIT(12))	// transfer to p2p sync or async target

#define MAX_FRAMEBUFFERS                512			// Max for Corvid88

#define KONAIP_REGS_START                0x40000

////////////////////////////////////////////////////////////////////////////////////

#if !defined (NTV2_DEPRECATE)
	//----------------------- HDNTV ---------------------------------

	// Board-type-specific defines  NOTE: Either KSD, KHD, or HDNTV needs to be defined.
	#define HDNTV_NTV2_DEVICENAME                 ("hdntv")

		// Offset in Base Address 1 Space to Channel 2 Frame Buffer
	#define HDNTV_CHANNEL2_OFFSET                 (0x2000000)              // 32 MBytes

		// Size of each frame buffer
	#define HDNTV_FRAMEBUFFER_SIZE                (0x800000)               //  8 MBytes

	#define HDNTV_NUM_FRAMEBUFFERS                (8)

	#define HDNTV_NTV2_VERTICALINTERRUPT_GLOBAL_EVENT_NAME "_HDVerticalInterruptSignalEvent"
		// the event name shared among all Windows NT
		// This name to be appended to the actual Win32Name
	#define HDNTV_NTV2_CHANGE_GLOBAL_EVENT_NAME "_HDChangeSignalEvent"


	//----------------------- Xena KHD ---------------------------------

		// Offset in Base Address 1 Space to Channel 2 Frame Buffer
	#define KHD_CHANNEL2_OFFSET                 (0x2000000)              // 32 MBytes

		// Size of each frame buffer
	#define KHD_FRAMEBUFFER_SIZE                (0x800000)               //  8 MBytes

	#define KHD_NUM_FRAMEBUFFERS                (32)

	#define KHD_NTV2_VERTICALINTERRUPT_GLOBAL_EVENT_NAME "_HDVerticalInterruptSignalEvent"
		// the event name shared among all Windows NT
		// This name to be appended to the actual Win32Name
	#define KHD_NTV2_CHANGE_GLOBAL_EVENT_NAME "_HDChangeSignalEvent"

	//
	// 2K Specific....only on HD22(for now) boards when in 2K standard
	// 
	#define XENA_FRAMEBUFFERSIZE_2K				(0x1000000) 
	#define XENA_NUMFRAMEBUFFERS_2K				(16) 

	//----------------------- Xena KSD ---------------------------------

		// Offset in Base Address 1 Space to Channel 2 Frame Buffer
	#define KSD_CHANNEL2_OFFSET                 (0x800000)               // 8 MBytes

		// Size of each frame buffer
	#define KSD_FRAMEBUFFER_SIZE                (0x200000)               //  2 MBytes

	#define KSD_NUM_FRAMEBUFFERS                (64)

	#define KSD_NTV2_VERTICALINTERRUPT_GLOBAL_EVENT_NAME "_SDVerticalInterruptSignalEvent"
		// the event name shared among all Windows NT
		// This name to be appended to the actual Win32Name
	#define KSD_NTV2_CHANGE_GLOBAL_EVENT_NAME "_SDChangeSignalEvent"


	//----------------------- AJA KONA ---------------------------------

		// Offset in Base Address 1 Space to Channel 2 Frame Buffer
	#define KONA_CHANNEL2_OFFSET                 (0x2000000)              // 32 MBytes

		// Size of each frame buffer
	#define KONA_FRAMEBUFFER_SIZE                (0x800000)               //  8 MBytes

	#define KONA_NUM_FRAMEBUFFERS                (32)

	#define KHD_NTV2_VERTICALINTERRUPT_GLOBAL_EVENT_NAME "_HDVerticalInterruptSignalEvent"
		// the event name shared among all Windows NT
		// This name to be appended to the actual Win32Name
	#define KHD_NTV2_CHANGE_GLOBAL_EVENT_NAME "_HDChangeSignalEvent"


	//----------------------- AJA XENALS ---------------------------------

		// Offset in Base Address 1 Space to Channel 2 Frame Buffer
	#define XENALS_CHANNEL2_OFFSET                 (0x2000000)              // 32 MBytes..not applicable

		// Size of each frame buffer
	#define XENALS_FRAMEBUFFER_SIZE                (0x800000)               //  8 MBytes

		// the event name shared among all Windows NT
		// This name to be appended to the actual Win32Name
	#define XENALS_NUM_FRAMEBUFFERS                (32)

	//----------------------- AJA FS1  ---------------------------------

		// Offset in Base Address 1 Space to Channel 2 Frame Buffer
	#define FS1_CHANNEL2_OFFSET                 (0x0)						// Framebuffers not accessible to processor

		// Size of each frame buffer
	#define FS1_FRAMEBUFFER_SIZE                (0x0)               		//  Framebuffers not accessible to processor

		// the event name shared among all Windows NT
		// This name to be appended to the actual Win32Name
	#define FS1_NUM_FRAMEBUFFERS                (0)
	#define FS1_NTV2_VERTICALINTERRUPT_GLOBAL_EVENT_NAME "_FS1VerticalInterruptSignalEvent"

		// the event name shared among all Windows NT
		// This name to be appended to the actual Win32Name
	#define FS1_NTV2_CHANGE_GLOBAL_EVENT_NAME "_FS1ChangeSignalEvent"

	//----------------------- AJA BORG ---------------------------------

		// Offset in Base Address 1 Space to Channel 2 Frame Buffer
	// #define BORG_CHANNEL2_OFFSET                 (0)              // not applicable

	//-------------------- Defines for 32 1MB frame buffers (28 1MB video frame buffers and 1 4MB audio buffer)
		// Size of each frame buffer
	//#define BORG_FRAMEBUFFER_SIZE                (0x100000)               //  1 MBytes
	//#define BORG_NUM_FRAMEBUFFERS                (32)

	//-------------------- Defines for 24 1.125MB frame buffers (24 1.125MB video frame buffers and 1 4MB audio buffer)
	//                     this totals 27 MB video frame, 1 MB unused gap, and 1 4MB audio buffer based 4 MB below top of memory

	#define BORG_FRAMEBUFFER_SIZE                (0x120000)                 // 1.125 MBytes
		// this define is left at 32 for practical purposes to locate audio buffer at 4 MB below top of memory (32 - 4 = 28)
		// in reality, there are only 24 video frame buffers, but if you look at usage of this define, it is best left at 32
	#define BORG_NUM_FRAMEBUFFERS                (32)						// 32 * 1.125 = 36
	#define BONES_NUM_FRAMEBUFFERS               (99)						// 99 * 1.125 = ~112 MB
#endif	//	NTV2_DEPRECATE

//----------------------- AJA XENA2 ---------------------------------

	// Offset in Base Address 1 Space to Channel 2 Frame Buffer
#define XENA2_CHANNEL2_OFFSET                 (0x2000000)              // 32 MBytes..not applicable

	// Size of each frame buffer
#define XENA2_FRAMEBUFFER_SIZE                (0x800000)               //  8 MBytes

	// the event name shared among all Windows NT
	// This name to be appended to the actual Win32Name
#define XENA2_NUM_FRAMEBUFFERS                (16)
#define XENA2_NTV2_VERTICALINTERRUPT_GLOBAL_EVENT_NAME "_Xena2VerticalInterruptSignalEvent"

	// the event name shared among all Windows NT
	// This name to be appended to the actual Win32Name
#define KHD_NTV2_CHANGE_GLOBAL_EVENT_NAME "_HDChangeSignalEvent"


//
// Special defines
//
#define NTV2_MIN_FRAMEBUFFERSIZE KSD_FRAMEBUFFER_SIZE
#define NTV2_MAX_FRAMEBUFFERSIZE KHD_FRAMEBUFFER_SIZE
#define NTV2_MIN_FRAMEBUFFERS HDNTV_NUM_FRAMEBUFFERS
#if defined(XENA2)
#define NTV2_MAX_FRAMEBUFFERS MAX_FRAMEBUFFERS
#else
#define NTV2_MAX_FRAMEBUFFERS BONES_NUM_FRAMEBUFFERS 
#endif

#define NTV2_UART_FIFO_SIZE (127)

#define NTV2_PROGRAM_READY_BIT   	( BIT_8 )
#define NTV2_PROGRAM_DONE_BIT    	( BIT_9 )
#define NTV2_PROGRAM_RESET_BIT   	( BIT_10 )

/* PORT C(7) is output (default) */
#define NTV2_FS1_FALLBACK_MODE_BIT  ( BIT_11 )	

/* PORT C(7) is input/3-state */
#define NTV2_FS1_CPLD_ENH_MODE_BIT  ( BIT_12 )	

//////////////////////////////////////////////////////////////////////////////////////
// Enums used to specify Property actions with interrupts
//////////////////////////////////////////////////////////////////////////////////////

typedef enum _INTERRUPT_ENUMS_
{
	eVerticalInterrupt,								//	0
	eOutput1				= eVerticalInterrupt,
	eInterruptMask,									//	1
	eInput1,										//	2
	eInput2,										//	3
	eAudio,											//	4
	eAudioInWrap,									//	5
	eAudioOutWrap,									//	6
	eDMA1,											//	7
	eDMA2,											//	8
	eDMA3,											//	9
	eDMA4,											//	10
	eChangeEvent,									//	11
	eGetIntCount,									//	12
	eWrapRate,										//	13
    eUartTx,										//	14
	eUart1Tx				= eUartTx,
    eUartRx,										//	15
	eUart1Rx				= eUartRx,
	eAuxVerticalInterrupt,							//	16
	ePushButtonChange,								//	17
	eLowPower,										//	18
	eDisplayFIFO,									//	19
	eSATAChange,									//	20
	eTemp1High,										//	21
	eTemp2High,										//	22
	ePowerButtonChange,								//	23
	eInput3,										//	24
	eInput4,										//	25
    eUartTx2,										//	26
	eUart2Tx				= eUartTx2,
    eUartRx2,										//	27
	eUart2Rx				= eUartRx2,
	eHDMIRxV2HotplugDetect,							//	28
	eInput5,										//	29
	eInput6,										//	30
	eInput7,										//	31
	eInput8,										//	32
	eInterruptMask2,								//	33
	eOutput2,										//	34
	eOutput3,										//	35
	eOutput4,										//	36
	eOutput5,										//	37
	eOutput6,										//	38
	eOutput7,										//	39
	eOutput8,										//	40
	eNumInterruptTypes	//	This must be last		//	41
} INTERRUPT_ENUMS;


#define	MAX_NUM_EVENT_CODES						(eNumInterruptTypes)
#define	NTV2_IS_VALID_INTERRUPT_ENUM(__e__)		((__e__) >= eOutput1 && (__e__) < eNumInterruptTypes)
#define NTV2_IS_INPUT_INTERRUPT(__e__)		(	(__e__) == eInput1		\
											||	(__e__) == eInput2		\
											||	(__e__) == eInput3		\
											||	(__e__) == eInput4		\
											||	(__e__) == eInput5		\
											||	(__e__) == eInput6		\
											||	(__e__) == eInput7		\
											||	(__e__) == eInput8	)

#define NTV2_IS_OUTPUT_INTERRUPT(__e__)		(	(__e__) == eOutput1		\
											||	(__e__) == eOutput2		\
											||	(__e__) == eOutput3		\
											||	(__e__) == eOutput4		\
											||	(__e__) == eOutput5		\
											||	(__e__) == eOutput6		\
											||	(__e__) == eOutput7		\
											||	(__e__) == eOutput8	)


// Some Mac only ENUMS that had to be moved over to get Win/Linux code to compile,
// so these are only used by the Mac.
typedef enum
{
	kFreeRun,
	kReferenceIn,
	kVideoIn
} ReferenceSelect;


#if !defined (NTV2_BUILDING_DRIVER)
	typedef std::vector <ULWord>					NTV2ULWordVector;					///< @brief	An ordered sequence of ULWords.

	/**
		@brief		Streams a human-readable dump of the given NTV2ULWordVector into the specified output stream.
		@param[in]	inObj			Specifies the NTV2ULWordVector to be printed on the output stream.
		@param		inOutStream		Specifies the output stream to receive the dump. Defaults to std::cout.
		@return		A non-constant reference to the given output stream.
	**/
	AJAExport std::ostream & NTV2PrintULWordVector (const NTV2ULWordVector & inObj, std::ostream & inOutStream = std::cout);	//	New in SDK 16.0

	typedef std::vector <NTV2Channel>				NTV2ChannelList;			///< @brief	An ordered sequence of NTV2Channel values.
	typedef NTV2ChannelList::const_iterator			NTV2ChannelListConstIter;	///< @brief	A handy const iterator into an NTV2ChannelList.

	/**
		@brief		Streams a human-readable dump of the given NTV2ChannelList into the specified output stream.
		@param[in]	inObj			Specifies the NTV2ChannelList to be streamed to the output stream.
		@param		inCompact		Use 'true' for a compact display (the default);  otherwise use 'false' for a long-format.
		@param		inOutStream		Specifies the output stream to receive the dump. Defaults to std::cout.
		@return		A non-constant reference to the given output stream.
	**/
	AJAExport std::ostream & NTV2PrintChannelList (const NTV2ChannelList & inObj, const bool inCompact = true, std::ostream & inOutStream = std::cout);	//	New in SDK 16.0

	/**
		@return		A human-readable string containing a dump of the channel list.
		@param[in]	inObj			Specifies the NTV2ChannelList to be converted.
		@param		inCompact		Use 'true' for a compact display (the default);  otherwise use 'false' for a long-format.
	**/
	AJAExport std::string NTV2ChannelListToStr (const NTV2ChannelList & inObj, const bool inCompact = true);	//	New in SDK 16.0

	typedef std::set <NTV2Channel>					NTV2ChannelSet;				///< @brief	A set of distinct NTV2Channel values.
	typedef NTV2ChannelSet::const_iterator			NTV2ChannelSetConstIter;	///< @brief	A handy const iterator into an NTV2ChannelSet.

	/**
		@brief		Streams a human-readable dump of the given NTV2ChannelSet into the specified output stream.
		@param[in]	inObj			Specifies the NTV2ChannelSet to be streamed to the output stream.
		@param		inCompact		Use 'true' for a compact display (the default);  otherwise use 'false' for a long-format.
		@param		inOutStream		Specifies the output stream to receive the dump. Defaults to std::cout.
		@return		A non-constant reference to the given output stream.
	**/
	AJAExport std::ostream & NTV2PrintChannelSet (const NTV2ChannelSet & inObj, const bool inCompact = true, std::ostream & inOutStream = std::cout);	//	New in SDK 16.0

	/**
		@return		A human-readable string containing a dump of the channel set.
		@param[in]	inObj			Specifies the NTV2ChannelSet to be converted.
		@param		inCompact		Use 'true' for a compact display (the default);  otherwise use 'false' for a long-format.
	**/
	AJAExport std::string NTV2ChannelSetToStr (const NTV2ChannelSet & inObj, const bool inCompact = true);	//	New in SDK 16.0

	/**
		@param[in]	inFirstChannel	Specifies the first NTV2Channel.
		@param		inNumChannels	Specifies the number of channels.
		@return		An NTV2ChannelSet having the specified contiguous range of channels.
	**/
	AJAExport NTV2ChannelSet NTV2MakeChannelSet (const NTV2Channel inFirstChannel, const UWord inNumChannels = 1);	//	New in SDK 16.0

	/**
		@param[in]	inChannels		Specifies the NTV2Channels that should go into the NTV2ChannelSet.
		@return		An NTV2ChannelSet having the same channels as contained in the specified list.
	**/
	AJAExport NTV2ChannelSet NTV2MakeChannelSet (const NTV2ChannelList inChannels);	//	New in SDK 16.0

	/**
		@param[in]	inFirstChannel	Specifies the first NTV2Channel.
		@param		inNumChannels	Specifies the number of channels.
		@return		An NTV2ChannelList having the specified contiguous range of channels.
	**/
	AJAExport NTV2ChannelList NTV2MakeChannelList (const NTV2Channel inFirstChannel, const UWord inNumChannels = 1);	//	New in SDK 16.0

	/**
		@param[in]	inChannels		Specifies the NTV2Channels that should go into the NTV2ChannelList.
		@return		An NTV2ChannelList having the same channels as contained in the specified set.
	**/
	AJAExport NTV2ChannelList NTV2MakeChannelList (const NTV2ChannelSet inChannels);	//	New in SDK 16.0
#endif	//	!defined (NTV2_BUILDING_DRIVER)


/**
	@brief	Everything needed to call CNTV2Card::ReadRegister or CNTV2Card::WriteRegister functions.
**/
typedef struct NTV2RegInfo
{
	ULWord	registerNumber;		///< @brief	My register number to use in a ReadRegister or WriteRegister call.
	ULWord	registerValue;		///< @brief	My register value to use in a ReadRegister or WriteRegister call.
	ULWord	registerMask;		///< @brief	My register mask value to use in a ReadRegister or WriteRegister call.
	ULWord	registerShift;		///< @brief	My register shift value to use in a ReadRegister or WriteRegister call.

	#if !defined(NTV2_BUILDING_DRIVER)
		/**
			@brief	Constructs me from the given parameters.
			@param[in]	inRegNum	Specifies the register number to use. If not specified, defaults to zero.
			@param[in]	inRegValue	Specifies the register value to use. If not specified, defaults to zero.
			@param[in]	inRegMask	Specifies the bit mask to use. If not specified, defaults to 0xFFFFFFFF.
			@param[in]	inRegShift	Specifies the shift to use. If not specified, defaults to zero.
		**/
		AJAExport NTV2RegInfo (const ULWord inRegNum = 0, const ULWord inRegValue = 0, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0)
			:	registerNumber	(inRegNum),
				registerValue	(inRegValue),
				registerMask	(inRegMask),
				registerShift	(inRegShift)
		{
		}

		/**
			@brief	Sets me from the given parameters.
			@param[in]	inRegNum	Specifies the register number to use.
			@param[in]	inRegValue	Specifies the register value to use.
			@param[in]	inRegMask	Specifies the bit mask to use. If not specified, defaults to 0xFFFFFFFF.
			@param[in]	inRegShift	Specifies the shift to use. If not specified, defaults to zero.
		**/
		AJAExport inline void	Set (const ULWord inRegNum, const ULWord inRegValue, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0)
													{registerNumber	= inRegNum; registerValue = inRegValue; registerMask = inRegMask; registerShift = inRegShift;}
		/**
			@brief	Invalidates me, setting my register number, value, mask and shift values to 0xFFFFFFFF.
		**/
		AJAExport inline void	MakeInvalid (void)				{registerNumber	= registerValue	= registerMask	= registerShift	= 0xFFFFFFFF;}

		/**
			@return	True if I'm considered "valid", or false if my register number, value, mask and shift values are all 0xFFFFFFFF.
		**/
		AJAExport inline bool	IsValid (void) const			{return registerNumber != 0xFFFFFFFF || registerValue != 0xFFFFFFFF || registerMask != 0xFFFFFFFF || registerShift != 0xFFFFFFFF;}

		/**
			@return		True if I'm identical to the right-hand-side NTV2RegInfo.
			@param[in]	inRHS	Specifies the right-hand-side NTV2RegInfo that will be compared to me.
			@note		To synthesize the other comparison operators (!=, <=, >, >=), in client code, add "#include <utility>", and "using namespace std::rel_ops;".
		**/
		AJAExport inline bool	operator == (const NTV2RegInfo & inRHS) const	{return registerNumber == inRHS.registerNumber && registerValue == inRHS.registerValue
																			&& registerMask == inRHS.registerMask && registerShift == inRHS.registerShift;}
		/**
			@return		True if I'm less than the right-hand-side NTV2RegInfo.
			@param[in]	inRHS		Specifies the right-hand-side NTV2RegInfo that will be compared to me.
			@note		To synthesize the other comparison operators (!=, <=, >, >=), in client code, add "#include <utility>", and "using namespace std::rel_ops;".
		**/
		AJAExport bool			operator < (const NTV2RegInfo & inRHS) const;

		/**
			@brief		Renders me to the given output stream in a human-readable format.
			@param		outputStream	Specifies the output stream in which to Print my contents.
			@param		inAsCode		Deprecated as of SDK 16.0 -- use PrintCode instead. Defaults to false.
										If true, renders me as source code.
			@return		The output stream.
		**/
		AJAExport std::ostream & Print (std::ostream & outputStream, const bool inAsCode = false) const;

		/**
			@brief		Renders me to the given output stream as source code using a "WriteRegister" function call.
			@param		outputStream	Specifies the output stream to write into.
			@param[in]	inRadix			Specifies the radix to use for the my register value. Defaults to hexadecimal (base 16).
			@return		The output stream.
		**/
		AJAExport std::ostream & PrintCode (std::ostream & outputStream, const int inRadix = 16) const;	//	New in SDK 16.0
	#endif	//	not NTV2_BUILDING_DRIVER
} NTV2RegInfo;


typedef NTV2RegInfo	NTV2ReadWriteRegisterSingle;	///< @brief	This is an alias for NTV2RegInfo -- everything needed to make a future ReadRegister or WriteRegister call.

#if !defined (NTV2_BUILDING_DRIVER)
	typedef std::vector <NTV2RegInfo>		NTV2RegisterWrites, NTV2RegWrites;						///< @brief	An ordered sequence of zero or more NTV2RegInfo structs intended for WriteRegister.
	typedef NTV2RegWrites::const_iterator	NTV2RegisterWritesConstIter, NTV2RegWritesConstIter;	///< @brief	A handy const (read-only) iterator for iterating over the contents of an NTV2RegisterWrites instance.
	typedef NTV2RegWrites::iterator			NTV2RegisterWritesIter, NTV2RegWritesIter;				///< @brief	A handy non-const iterator for iterating over the contents of an NTV2RegisterWrites instance.
	typedef NTV2RegWrites					NTV2RegisterReads, NTV2RegReads;						///< @brief	An ordered sequence of zero or more NTV2RegInfo structs intended for ReadRegister.
	typedef NTV2RegWritesConstIter			NTV2RegisterReadsConstIter, NTV2RegReadsConstIter;		///< @brief	A handy const (read-only) iterator for iterating over the contents of an NTV2RegisterReads instance.
	typedef NTV2RegWritesIter				NTV2RegisterReadsIter, NTV2RegReadsIter;				///< @brief	A handy non-const iterator for iterating over the contents of an NTV2RegisterReads instance.

	/**
		@brief		Returns a const iterator to the first entry in the NTV2RegInfo collection with a matching register number.
		@param[in]	inRegNum	Specifies the register number of interest.
		@param[in]	inRegInfos	Specifies the NTV2RegInfo collection to search.
		@return		A const_iterator that references the entry in the NTV2RegInfo collection, or "end()" if not found.
	**/
	AJAExport NTV2RegReadsConstIter	FindFirstMatchingRegisterNumber (const uint32_t inRegNum, const NTV2RegReads & inRegInfos);

	/**
		@brief		Writes the given NTV2RegInfo to the specified output stream.
		@param		inOutStream	Specifies the output stream to receive the human-readable representation of the NTV2RegInfo instance.
		@param[in]	inObj		Specifies the NTV2RegInfo instance to print to the output stream.
		@return	A non-constant reference to the specified output stream.
	**/
	AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2RegInfo & inObj);

	/**
		@brief		Writes the given NTV2RegisterWrites to the specified output stream.
		@param		inOutStream	Specifies the output stream to receive the human-readable representation of the NTV2RegisterWrites instance.
		@param[in]	inObj		Specifies the NTV2RegisterWrites instance to print to the output stream.
		@return	A non-constant reference to the specified output stream.
	**/
	AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2RegisterWrites & inObj);
#endif	//	!defined (NTV2_BUILDING_DRIVER)


typedef struct NTV2RoutingEntry
{
	ULWord	registerNum;
	ULWord	mask;
	ULWord	shift;
	ULWord	value;
	#if !defined (NTV2_BUILDING_DRIVER)
		NTV2RoutingEntry & operator = (const NTV2RegInfo & inRHS);	///< @brief	Assigns an NTV2RegInfo to me.
	#endif	//	!defined (NTV2_BUILDING_DRIVER)
} NTV2RoutingEntry;	///< @deprecated	Formerly used by the CNTV2SignalRouter

#define MAX_ROUTING_ENTRIES 32

typedef struct
{
	ULWord				numEntries;
	NTV2RoutingEntry	routingEntry [MAX_ROUTING_ENTRIES];
} NTV2RoutingTable;	///< @deprecated	Formerly used by the CNTV2SignalRouter

#if !defined (NTV2_DEPRECATE)
	typedef	NTV2RoutingEntry	Xena2RoutingEntry;		///< @deprecated	'Xena' is obsolete.
	typedef	NTV2RoutingTable	Xena2RoutingTable;		///< @deprecated	'Xena' is obsolete.
#endif	//	if !defined (NTV2_DEPRECATE)


// Color Space Convert Custom Coefficients
typedef struct
{
	ULWord Coefficient1;
	ULWord Coefficient2;
	ULWord Coefficient3;
	ULWord Coefficient4;
	ULWord Coefficient5;
	ULWord Coefficient6;
	ULWord Coefficient7;
	ULWord Coefficient8;
	ULWord Coefficient9;
	ULWord Coefficient10;
} ColorSpaceConverterCustomCoefficients, NTV2CSCCustomCoeffs;

/////////////////////////////////////////////////////////////////////////////////////
// RP188 data structure used in AutoCirculate
/////////////////////////////////////////////////////////////////////////////////////

typedef struct RP188_STRUCT {
	ULWord	DBB;
	ULWord	Low;		//  |  BG 4  | Secs10 |  BG 3  | Secs 1 |  BG 2  | Frms10 |  BG 1  | Frms 1 |
	ULWord	High;		//  |  BG 8  | Hrs 10 |  BG 7  | Hrs  1 |  BG 6  | Mins10 |  BG 5  | Mins 1 |
} RP188_STRUCT;


#define	RP188_STRUCT_SET(_struct_,_dbb_,_lo_,_hi_)					do								\
																	{								\
																		(_struct_).DBB = (_dbb_);	\
																		(_struct_).Low = (_lo_);	\
																		(_struct_).High = (_hi_);	\
																	} while (false)

#define	RP188_PSTRUCT_SET(_pStruct_,_dbb_,_lo_,_hi_)				do								\
																	{								\
																		(_pStruct_)->DBB = (_dbb_);	\
																		(_pStruct_)->Low = (_lo_);	\
																		(_pStruct_)->High = (_hi_);	\
																	} while (false)

	// convenience masks to extract fields in .Low and .High words 
#define RP188_FRAMEUNITS_MASK   0x0000000F		// Frames (units digit)  in bits  3- 0 of .Low word
#define RP188_FRAMETENS_MASK	0x00000300		// Frames (tens digit)   in bits  9- 8 of .Low word
#define RP188_SECONDUNITS_MASK	0x000F0000		// Seconds (units digit) in bits 19-16 of .Low word
#define RP188_SECONDTENS_MASK	0x07000000		// Seconds (tens digit)  in bits 26-24 of .Low word
#define RP188_LOW_TIME_MASK		(RP188_FRAMEUNITS_MASK | RP188_FRAMETENS_MASK | RP188_SECONDUNITS_MASK | RP188_SECONDTENS_MASK)

#define RP188_MINUTESUNITS_MASK 0x0000000F		// Minutes (units digit) in bits  3- 0 of .High word
#define RP188_MINUTESTENS_MASK	0x00000700		// Minutes (tens digit)  in bits 10- 8 of .High word
#define RP188_HOURUNITS_MASK	0x000F0000		// Hours (units digit)   in bits 19-16 of .High word
#define RP188_HOURTENS_MASK		0x03000000		// Hours (tens digit)    in bits 25-24 of .High word
#define RP188_HIGH_TIME_MASK	(RP188_MINUTESUNITS_MASK | RP188_MINUTESTENS_MASK | RP188_HOURUNITS_MASK | RP188_HOURTENS_MASK)

	// private bit flags added to the RP188 DBB word
#define NEW_RP188_RCVD			0x00010000		// new RP188 data was received on ANY of the channels (LTC, VITC, etc.) (capture only)
#define NEW_SELECT_RP188_RCVD   0x00020000		// new RP188 data was received on the selected channel (capture only)
#define RP188_720P_FRAMEID		0x00400000		// 720p FrameID (capture only - set by driver software)
#define RP188_CHANGED_FLAG		0x00800000		// RP188 data changed compared to last frame (capture only - set by driver software)


/////////////////////////////////////////////////////////////////////////////////////
// Color Correction data structure used in AutoCirculate
/////////////////////////////////////////////////////////////////////////////////////
// Color Corrector has 3 tables(usually R, G and B). Each table has 1024 entries
// with 2 entries per 32 bit word....therefore 512 32 bit words per table.
#define NTV2_COLORCORRECTOR_WORDSPERTABLE	(512)										// number of ULONG words in EACH color table
#define NTV2_COLORCORRECTOR_TOTALWORDS		(NTV2_COLORCORRECTOR_WORDSPERTABLE * 3)		// total number of ULONG words in all 3 tables
#define NTV2_COLORCORRECTOR_TABLESIZE		(NTV2_COLORCORRECTOR_TOTALWORDS * 4)		// total length in bytes of all 3 tables: numWords * numColors * bytes/word
	
// 12 bit Color Corrector has 3 tables(usually R, G and B). Each table has 4096 entries
// with 2 entries per 32 bit word....therefore 2048 32 bit words per table.
#define NTV2_12BIT_COLORCORRECTOR_WORDSPERTABLE	(2048)											// number of ULONG words in EACH color table
#define NTV2_12BIT_COLORCORRECTOR_TOTALWORDS	(NTV2_12BIT_COLORCORRECTOR_WORDSPERTABLE * 3)	// total number of ULONG words in all 3 tables
#define NTV2_12BIT_COLORCORRECTOR_TABLESIZE		(NTV2_12BIT_COLORCORRECTOR_TOTALWORDS * 4)		// total length in bytes of all 3 tables: numWords * numColors * bytes/word


typedef struct NTV2ColorCorrectionInfo_64 {
   NTV2ColorCorrectionMode	mode;
   UWord_					saturationValue;	///	only used in 3way color correction mode.
   Pointer64				ccLookupTables;		///	R,G, and B lookup tables already formated for our hardware.
												///	Buffer needs to be NTV2_COLORCORRECTOR_TABLESIZE.
} NTV2ColorCorrectionInfo_64;


typedef struct {
   NTV2ColorCorrectionMode mode;
   UWord_  saturationValue;    /// only used in 3way color correction mode.
   ULWord* ccLookupTables; /// R,G, and B lookup tables already formated for our hardware.
						   /// Buffer needs to be NTV2_COLORCORRECTOR_TABLESIZE.
} NTV2ColorCorrectionInfo;

typedef struct {
   NTV2ColorCorrectionMode mode;
   UWord_   saturationValue;    /// only used in 3way color correction mode.
   ULWord* POINTER_32 ccLookupTables; /// R,G, and B lookup tables already formated for our hardware.
						   /// Buffer needs to be NTV2_COLORCORRECTOR_TABLESIZE.
} NTV2ColorCorrectionInfo_32;

// within each 32-bit LUT word, bits <31:22> = LUT[2i+1], bits <15:6> = LUT[2i] 
#define kRegColorCorrectionLUTOddShift	22
#define kRegColorCorrectionLUTEvenShift	 6

// the base BYTE offsets (from PCI Config Base Address 0) of the three Color Correction LUTs
// Note: if using these with GetRegisterBaseAddress() be sure to divide by 4 to get WORD offset!
#define kColorCorrectionLUTOffset_Red	(0x0800)
#define kColorCorrectionLUTOffset_Green	(0x1000)
#define kColorCorrectionLUTOffset_Blue	(0x1800)
	
// within each 32-bit LUT word, bits <31:22> = LUT[2i+1], bits <15:6> = LUT[2i] 
#define kRegColorCorrection12BitLUTOddShift		16
#define kRegColorCorrection12BitLUTEvenShift	 0

#define kRegColorCorrection10To12BitLUTOddShift		18
#define kRegColorCorrection10To12BitLUTEvenShift	 2

// Note: there is code that assumes that the three LUTs are contiguous. So if this relationship
//       changes (i.e. there are "gaps" between tables) then code will need to change!
#define kColorCorrectionLUTOffset_Base	(0x0800)	// BYTE offset
#define kColorCorrection12BitLUTOffset_Base	(0xe000)	// BYTE offset


/////////////////////////////////////////////////////////////////////////////////////
// VidProc data structure used in AutoCirculate
/////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	AUTOCIRCVIDPROCMODE_MIX,
	AUTOCIRCVIDPROCMODE_HORZWIPE,
	AUTOCIRCVIDPROCMODE_VERTWIPE,
	AUTOCIRCVIDPROCMODE_KEY,
	AUTOCIRCVIDPROCMODE_INVALID
} AutoCircVidProcMode;

#define	NTV2_IS_VALID_AUTOCIRCVIDPROCMODE(__m__)		((__m__) >= AUTOCIRCVIDPROCMODE_MIX && (__m__) < AUTOCIRCVIDPROCMODE_INVALID)



typedef struct AutoCircVidProcInfo
{
	AutoCircVidProcMode mode;
	NTV2Crosspoint      foregroundVideoCrosspoint;
	NTV2Crosspoint      backgroundVideoCrosspoint;
	NTV2Crosspoint      foregroundKeyCrosspoint;
	NTV2Crosspoint      backgroundKeyCrosspoint;
	Fixed_              transitionCoefficient;       // 0-0x10000
	Fixed_              transitionSoftness;         // 0-0x10000

	#if !defined (NTV2_BUILDING_DRIVER)
		public:
			AJAExport explicit AutoCircVidProcInfo ();
	#endif	//	user-space clients only
} AutoCircVidProcInfo;


/////////////////////////////////////////////////////////////////////////////////////
// CustomAncData data structure used in AutoCirculate
/////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	 ULWord Group1;
	 ULWord Group2;
	 ULWord Group3;
	 ULWord Group4;
} CUSTOM_ANC_STRUCT;

#define OBSOLETE_ANC_STRUCT	CUSTOM_ANC_STRUCT


////////////////////////////////////////////////////////////////////////////////////
// Control
////////////////////////////////////////////////////////////////////////////////////

typedef enum _AutoCircCommand_
{
	eInitAutoCirc,
	eStartAutoCirc,
	eStopAutoCirc,
	ePauseAutoCirc,
	eGetAutoCirc,
	eGetFrameStamp,
	eFlushAutoCirculate,
	ePrerollAutoCirculate,
	eTransferAutoCirculate,
	eAbortAutoCirc,
	eStartAutoCircAtTime,
	eTransferAutoCirculateEx,
	eTransferAutoCirculateEx2,
	eGetFrameStampEx2,
	eSetCaptureTask,
	eSetActiveFrame,
	AUTO_CIRC_NUM_COMMANDS,
	AUTO_CIRC_COMMAND_INVALID	= AUTO_CIRC_NUM_COMMANDS
} NTV2AutoCirculateCommand,	NTV2AutoCircCmd, AUTO_CIRC_COMMAND;


/**
	@brief	Describes the state of an AutoCirculate channel. See ::AUTOCIRCULATE_STATUS.
**/
typedef enum
{
	NTV2_AUTOCIRCULATE_DISABLED	= 0,		///< @brief	The AutoCirculate channel is stopped.
	NTV2_AUTOCIRCULATE_INIT,				///< @brief	The AutoCirculate channel is initializing, waiting for CNTV2Card::AutoCirculateStart to be called.
	NTV2_AUTOCIRCULATE_STARTING,			///< @brief	The AutoCirculate channel is starting, waiting for the first VBI and/or call to CNTV2Card::AutoCirculateTransfer.
	NTV2_AUTOCIRCULATE_PAUSED,				///< @brief	The AutoCirculate channel is paused.
	NTV2_AUTOCIRCULATE_STOPPING,			///< @brief	The AutoCirculate channel is stopping.
	NTV2_AUTOCIRCULATE_RUNNING,				///< @brief	The AutoCirculate channel is running.
	NTV2_AUTOCIRCULATE_STARTING_AT_TIME,	///< @brief	The AutoCirculate channel is starting, waiting for the start time specified in the call to CNTV2Card::AutoCirculateStart to arrive.
	NTV2_AUTOCIRCULATE_INVALID
} NTV2AutoCirculateState;

#define	NTV2_IS_VALID_AUTO_CIRC_STATE(__m__)		((__m__) >= NTV2_AUTOCIRCULATE_DISABLED  &&  (__m__) < NTV2_AUTOCIRCULATE_INVALID)


/**
	@brief	Describes the task mode state. See also: \ref devicesharing
**/
typedef enum
{
	NTV2_DISABLE_TASKS,				///< @brief	0: Disabled: Device is completely configured by controlling application(s) -- no driver involvement.
	NTV2_STANDARD_TASKS,			///< @brief	1: Standard/Retail: Device is completely controlled by AJA ControlPanel, service/daemon, and driver.
	NTV2_OEM_TASKS,					///< @brief	2: OEM: Device is configured by controlling application(s), with minimal driver involvement.
	NTV2_TASK_MODE_INVALID	= 0xFF
} NTV2EveryFrameTaskMode, NTV2TaskMode;

#define	NTV2_IS_VALID_TASK_MODE(__m__)		((__m__) == NTV2_DISABLE_TASKS  ||  (__m__) == NTV2_STANDARD_TASKS  ||  (__m__) == NTV2_OEM_TASKS)
#define	NTV2_IS_STANDARD_TASKS(__m__)		((__m__) == NTV2_STANDARD_TASKS)
#define NTV2_IS_DRIVER_ACTIVE_TASKS(__m__)	((__m__) == NTV2_STANDARD_TASKS  ||  (__m__) == NTV2_OEM_TASKS)


typedef enum
{
	NTV2_FREEZE_BITFILE			= BIT(30),
	NTV2_DRIVER_TASKS			= BIT(28)
} NTV2DebugReg;


#ifdef AJAMac
	#pragma pack(4)	// removes 64 bit alignment on non-64 bit fields
#endif

// Structure used for GetAutoCirculate
typedef struct
{
	NTV2Crosspoint			channelSpec;			// Not used by Windows.
	NTV2AutoCirculateState  state;
	LWord                   startFrame;
	LWord                   endFrame;
	LWord                   activeFrame;            // Current Frame# actually being output (or input), -1, if not active
	ULWord64                rdtscStartTime;         // Performance Counter at start
	ULWord64				audioClockStartTime;    // Register 28 with Wrap Logic
	ULWord64				rdtscCurrentTime;		// Performance Counter at time of call
	ULWord64				audioClockCurrentTime;	// Register 28 with Wrap Logic
	ULWord					framesProcessed;
	ULWord					framesDropped;
	ULWord					bufferLevel;    		// how many buffers ready to record or playback in driver
	BOOL_					bWithAudio;
	BOOL_					bWithRP188;
    BOOL_					bFbfChange;
    BOOL_					bFboChange ;
	BOOL_					bWithColorCorrection;
	BOOL_					bWithVidProc;          
	BOOL_					bWithCustomAncData;          
} AUTOCIRCULATE_STATUS_STRUCT;


typedef struct
{
	AUTO_CIRC_COMMAND	eCommand;
	NTV2Crosspoint		channelSpec;

	LWord				lVal1;
	LWord				lVal2;
    LWord               lVal3;
    LWord               lVal4;
    LWord               lVal5;
    LWord               lVal6;

	BOOL_				bVal1;
	BOOL_				bVal2;
    BOOL_               bVal3;
    BOOL_               bVal4;
    BOOL_               bVal5;
    BOOL_               bVal6;
    BOOL_               bVal7;
    BOOL_               bVal8;

	Pointer64           pvVal1;
	Pointer64           pvVal2;
	Pointer64           pvVal3;
	Pointer64           pvVal4;

} AUTOCIRCULATE_DATA_64;


typedef struct AUTOCIRCULATE_DATA
{
	AUTO_CIRC_COMMAND	eCommand;
	NTV2Crosspoint		channelSpec;

	LWord				lVal1;
	LWord				lVal2;
    LWord               lVal3;
    LWord               lVal4;
    LWord               lVal5;
    LWord               lVal6;

	BOOL_				bVal1;
	BOOL_				bVal2;
    BOOL_               bVal3;
    BOOL_               bVal4;
    BOOL_               bVal5;
    BOOL_               bVal6;
    BOOL_               bVal7;
    BOOL_               bVal8;

	void*				pvVal1;
	void*				pvVal2;
	void*				pvVal3;
	void*				pvVal4;

	#if !defined (NTV2_BUILDING_DRIVER)
		public:
			AJAExport explicit AUTOCIRCULATE_DATA (const AUTO_CIRC_COMMAND inCommand = AUTO_CIRC_COMMAND_INVALID, const NTV2Crosspoint inCrosspoint = NTV2CROSSPOINT_INVALID);
	#endif	//	user-space clients only
} AUTOCIRCULATE_DATA;


typedef struct
{
	AUTO_CIRC_COMMAND	eCommand;
	NTV2Crosspoint		channelSpec;

	LWord				lVal1;
	LWord				lVal2;
    LWord               lVal3;
    LWord               lVal4;
    LWord               lVal5;
    LWord               lVal6;

	BOOL_				bVal1;
	BOOL_				bVal2;
    BOOL_               bVal3;
    BOOL_               bVal4;
    BOOL_               bVal5;
    BOOL_               bVal6;
    BOOL_               bVal7;
    BOOL_               bVal8;

	void* POINTER_32	pvVal1;
	void* POINTER_32	pvVal2;
	void* POINTER_32	pvVal3;
	void* POINTER_32	pvVal4;

} AUTOCIRCULATE_DATA_32;


/////////////////////////////////////////////////////////////////////////////////////////
// GetFrameStamp
/////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	NTV2Crosspoint		channelSpec;	// Ignored in Windows

	//
	// Information from the requested frame (#FRAMESTAMP_CONTROL_STRUCT:frameNum
	//

	// Clock (System PerformanceCounter under Windows) at time of play or record.
    // audioClockTimeStamp is preferred, but not available on all boards.  (See comments below at 'currentTime' member.)
	LWord64				frameTime;

	//! The frame requested or -1 if not available
	ULWord				frame;

	//! 48kHz clock (in reg 28, extended to 64 bits) at time of play or record.
	ULWord64			audioClockTimeStamp;    // Register 28 with Wrap Logic

	//! The address that was used to transfer
	ULWord				audioExpectedAddress;

	//! For record - first position in buffer of audio (includes base offset)
	ULWord				audioInStartAddress;    // AudioInAddress at the time this Frame was stamped.

	//! For record - end position (exclusive) in buffer of audio (includes base offset)
	ULWord				audioInStopAddress;        // AudioInAddress at the Frame AFTER this Frame was stamped.

	//! For play - first position in buffer of audio
	ULWord				audioOutStopAddress;    // AudioOutAddress at the time this Frame was stamped.

	//! For play - end position (exclusive) in buffer of audio
	ULWord				audioOutStartAddress;    // AudioOutAddress at the Frame AFTER this Frame was stamped.

	//! Total audio and video bytes transfered
	ULWord				bytesRead;

	/** The actaul start sample when this frame was started in VBI
	* This may be used to check sync against audioInStartAddress (Play) or
	* audioOutStartAddress (Record).  In record it will always be equal, but
	* in playback if the clocks drift or the user supplies non aligned
	* audio sizes, then this will give the current difference from expected
	* vs actual position.  To be useful, play audio must be clocked in at
	* the correct rate.
	*/
	ULWord				startSample;

	//
	// Information from the current (active) frame
	//

	//! Current processor time ... on Windows, this is derived from KeQueryPerformanceCounter.
    //  This is the finest-grained counter available from the OS.
    //  The granularity of this counter can vary depending on the PC's HAL.
	//  audioClockCurrentTime is the recommended time-stamp to use instead of this (but is not available on all boards)!
	LWord64			    currentTime;               //! Last vertical blank frame for this channel's auto-circulate. (at the time of the IOCTL_NTV2_GET_FRAMESTAMP)
	ULWord				currentFrame;

	//! Last vertical blank timecode (RP-188)
	RP188_STRUCT		currentRP188;                   // ignored if withRP188 is false

	//! Vertical blank start of current frame
	LWord64			    currentFrameTime;

	//! 48kHz clock in reg 28 extended to 64 bits
	ULWord64			audioClockCurrentTime;      // Register 28 with Wrap Logic
	                                           // audioClockCurrentTime (from 48 kHz on-board clock) is consistent and accurate!
                                               // but is not available on the XenaSD-22.

	//! As set by play
	ULWord				currentAudioExpectedAddress;

	//! As found by isr
	ULWord				currentAudioStartAddress;

	//! At Call Field0 or Field1 _currently_ being OUTPUT (at the time of the IOCTL_NTV2_GET_FRAMESTAMP)
	ULWord				currentFieldCount;         
	ULWord				currentLineCount;			//! At Call Line# _currently_ being OUTPUT (at the time of the IOCTL_NTV2_GET_FRAMESTAMP)
	ULWord				currentReps;				//! Contains validCount (Play - reps remaining, Record - drops on frame)
	ULWord				currenthUser;				//! User cookie at last vblank
} FRAME_STAMP_STRUCT;


typedef struct
{
	NTV2Crosspoint          channelSpec;    // specify Input or Output channel for desired Frame
	NTV2AutoCirculateState  state;          // current state
	LWord                   transferFrame;  // framebuffer number the frame transferred to, -1 on error
	ULWord                  bufferLevel;    // how many buffers ready to record or playback
	ULWord                  framesProcessed;
	ULWord                  framesDropped;
	FRAME_STAMP_STRUCT      frameStamp;     // record. framestramp for that frame,playback
	ULWord					audioBufferSize;
	ULWord					audioStartSample;
} AUTOCIRCULATE_TRANSFER_STATUS_STRUCT, *PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT;


typedef struct
{
	NTV2Crosspoint					channelSpec;			//	Specify Input or Output channel for desired Frame
	Pointer64						videoBuffer;			//	Keep 64 bit aligned for performance reasons
	ULWord							videoBufferSize;
	ULWord							videoDmaOffset;			//	Must be initialized, 64 bit aligned
	Pointer64						audioBuffer;			//	Keep 64 bit aligned for performance reasons
	ULWord							audioBufferSize;
	ULWord							audioStartSample;		//	To ensure correct alignment in audio buffer .. NOT USED in Windows... audio now always starts at sample zero.
	ULWord							audioNumChannels;		//	1-6 NOTE!!! only 6 supported at this time
	ULWord							frameRepeatCount;		//	NOTE!!! not supported yet.
	RP188_STRUCT					rp188;					//	Ignored if withRP188 is false
	LWord							desiredFrame;			//	-1 if you want driver to find next available
	ULWord							hUser;					//	A user cookie returned by frame stamp
	ULWord							transferFlags;			//	disableAudioDMA is no longer used
	BOOL_							bDisableExtraAudioInfo;	//	No 24 byte 0 at front or size info in buffer
    NTV2FrameBufferFormat			frameBufferFormat;		//	Should be initialized, but can be overridden
	NTV2VideoFrameBufferOrientation	frameBufferOrientation;
	NTV2ColorCorrectionInfo_64		colorCorrectionInfo;
	AutoCircVidProcInfo				vidProcInfo;
	CUSTOM_ANC_STRUCT				customAncInfo;			///< @brief	This field is obsolete. Do not use.
		// The following params are for cases when you need to DMA multiple discontiguous "segments" of a video frame. One example
		// would be when a frame in Host memory is not "packed", i.e. there are extra "padding" bytes at the end of each row.
		// In this case you would set videoBufferSize to the number of active bytes per row, videoNumSegments to the number of rows,
		// and videoSegmentHostPitch to the number of bytes from the beginning of one row to the next. In this example,
		// videoSegmentCardPitch would be equal to videoBufferSize (i.e. the frame is packed in board memory).
		//   Another example would be DMAing a sub-section of a frame. In this case set videoBufferSize to the number of bytes in
		// one row of the subsection, videoNumSegments to the number of rows in the subsection, videoSegmentHostPitch to the rowBytes
		// of the entire frame in Host Memory, and videoSegmentCardPitch to the rowBytes of the entire frame in board memory.
		// Note: setting videoNumSegments to 0 or 1 defaults to original behavior (i.e. DMA one complete packed frame)
	ULWord							videoNumSegments;		//	Number of segments of size videoBufferSize to DMA (i.e. numLines)
	ULWord							videoSegmentHostPitch;	//	Offset (in bytes) between the beginning of one host segment and the beginning of the next host segment (i.e. host rowBytes)
	ULWord							videoSegmentCardPitch;	//	Offset (in bytes) between the beginning of one board segment and the beginning of the next board segment (i.e. board memory rowBytes)
	NTV2QuarterSizeExpandMode		videoQuarterSizeExpand;	//	Turns on the "quarter-size expand" (2x H + 2x V) hardware
	//ULWord *						ancBuffer;				//	Host ANC data buffer. If NULL, none transferred.
	//ULWord						ancBufferSize;			//	Capture:  before xfer: specifies max size of host ancBuffer; after xfer: actual number of ANC data bytes xferred
															//	Playout:  specifies number of ANC data bytes to xfer from host ancBuffer to device
															//	If zero, none transferred.
} AUTOCIRCULATE_TRANSFER_STRUCT_64, *PAUTOCIRCULATE_TRANSFER_STRUCT_64;


typedef struct
{
	NTV2Crosspoint					channelSpec;			//	specify Input or Output channel for desired Frame
	ULWord  *						videoBuffer;			//	Keep 64 bit aligned for performance reasons
	ULWord							videoBufferSize;
	ULWord							videoDmaOffset;			//	must be initialized, 64 bit aligned
	ULWord  *						audioBuffer;			//	Keep 64 bit aligned for performance reasons
	ULWord							audioBufferSize;
	ULWord							audioStartSample;		//	To ensure correct alignment in audio buffer .. NOT USED in Windows... audio now always starts at sample zero.
	ULWord							audioNumChannels;		//	1-6 NOTE!!! only 6 supported at this time
	ULWord							frameRepeatCount;		//	NOTE!!! not supported yet.
	RP188_STRUCT					rp188;					//	Ignored if withRP188 is false
	LWord							desiredFrame;			//	-1 if you want driver to find next available
	ULWord							hUser;					//	A user cookie returned by frame stamp
	ULWord							transferFlags;			//	disableAudioDMA is no longer used
	BOOL_							bDisableExtraAudioInfo;	//	No 24 byte 0 at front or size info in buffer
	NTV2FrameBufferFormat			frameBufferFormat;		//	Should be initialized, but can be overridden
	NTV2VideoFrameBufferOrientation	frameBufferOrientation;
	NTV2ColorCorrectionInfo			colorCorrectionInfo;
	AutoCircVidProcInfo				vidProcInfo;
	CUSTOM_ANC_STRUCT				customAncInfo;			///< @brief	This field is obsolete. Do not use.
		// The following params are for cases when you need to DMA multiple discontiguous "segments" of a video frame. One example
		// would be when a frame in Host memory is not "packed", i.e. there are extra "padding" bytes at the end of each row.
		// In this case you would set videoBufferSize to the number of active bytes per row, videoNumSegments to the number of rows,
		// and videoSegmentHostPitch to the number of bytes from the beginning of one row to the next. In this example,
		// videoSegmentCardPitch would be equal to videoBufferSize (i.e. the frame is packed in board memory).
		//   Another example would be DMAing a sub-section of a frame. In this case set videoBufferSize to the number of bytes in
		// one row of the subsection, videoNumSegments to the number of rows in the subsection, videoSegmentHostPitch to the rowBytes
		// of the entire frame in Host Memory, and videoSegmentCardPitch to the rowBytes of the entire frame in board memory.
		// Note: setting videoNumSegments to 0 or 1 defaults to original behavior (i.e. DMA one complete packed frame)
	ULWord							videoNumSegments;		//	Number of segments of size videoBufferSize to DMA (i.e. numLines)
	ULWord							videoSegmentHostPitch;	//	Offset (in bytes) between the beginning of one host segment and the beginning of the next host segment (i.e. host rowBytes)
	ULWord							videoSegmentCardPitch;	//	Offset (in bytes) between the beginning of one board segment and the beginning of the next board segment (i.e. board memory rowBytes)
	NTV2QuarterSizeExpandMode		videoQuarterSizeExpand;	//	Turns on the "quarter-size expand" (2x H + 2x V) hardware
	//ULWord *						ancBuffer;				//	Host ANC data buffer. If NULL, none transferred.
	//ULWord						ancBufferSize;			//	Capture:  before xfer: specifies max size of host ancBuffer; after xfer: actual number of ANC data bytes xferred
															//	Playout:  specifies number of ANC data bytes to xfer from host ancBuffer to device
															//	If zero, none transferred.

} AUTOCIRCULATE_TRANSFER_STRUCT, *PAUTOCIRCULATE_TRANSFER_STRUCT;


typedef struct
{
	NTV2Crosspoint					channelSpec;			//	Specify Input or Output channel for desired Frame
	ULWord  * POINTER_32			videoBuffer;			//	Keep 64 bit aligned for performance reasons
	ULWord							videoBufferSize;
	ULWord							videoDmaOffset;			//	Must be initialized, 64 bit aligned
	ULWord  * POINTER_32			audioBuffer;			//	Keep 64 bit aligned for performance reasons
	ULWord							audioBufferSize;
	ULWord							audioStartSample;		//	To ensure correct alignment in audio buffer .. NOT USED in Windows... audio now always starts at sample zero.
	ULWord							audioNumChannels;		//	1-6 NOTE!!! only 6 supported at this time
	ULWord							frameRepeatCount;		//	NOTE!!! not supported yet.
	RP188_STRUCT					rp188;					//	Ignored if withRP188 is false
	LWord							desiredFrame;			//	-1 if you want driver to find next available
	ULWord							hUser;					//	A user cookie returned by frame stamp
	ULWord							transferFlags;			//	disableAudioDMA is no longer used
	BOOL_							bDisableExtraAudioInfo;	//	No 24 byte 0 at front or size info in buffer .. NOT USED in Windows, extra audio no longer supported
	NTV2FrameBufferFormat			frameBufferFormat;		//	Should be initialized, but can be overridden
	NTV2VideoFrameBufferOrientation	frameBufferOrientation;
	NTV2ColorCorrectionInfo_32		colorCorrectionInfo;
	AutoCircVidProcInfo				vidProcInfo;
	CUSTOM_ANC_STRUCT				customAncInfo;			///< @brief	This field is obsolete. Do not use.
		// The following params are for cases when you need to DMA multiple discontiguous "segments" of a video frame. One example
		// would be when a frame in Host memory is not "packed", i.e. there are extra "padding" bytes at the end of each row.
		// In this case you would set videoBufferSize to the number of active bytes per row, videoNumSegments to the number of rows,
		// and videoSegmentHostPitch to the number of bytes from the beginning of one row to the next. In this example,
		// videoSegmentCardPitch would be equal to videoBufferSize (i.e. the frame is packed in board memory).
		//   Another example would be DMAing a sub-section of a frame. In this case set videoBufferSize to the number of bytes in
		// one row of the subsection, videoNumSegments to the number of rows in the subsection, videoSegmentHostPitch to the rowBytes
		// of the entire frame in Host Memory, and videoSegmentCardPitch to the rowBytes of the entire frame in board memory.
		// Note: setting videoNumSegments to 0 or 1 defaults to original behavior (i.e. DMA one complete packed frame)
	ULWord							videoNumSegments;		//	Number of segments of size videoBufferSize to DMA (i.e. numLines)
	ULWord							videoSegmentHostPitch;	//	Offset (in bytes) between the beginning of one host segment and the beginning of the next host segment (i.e. host rowBytes)
	ULWord							videoSegmentCardPitch;	//	Offset (in bytes) between the beginning of one board segment and the beginning of the next board segment (i.e. board memory rowBytes)
	NTV2QuarterSizeExpandMode		videoQuarterSizeExpand;	//	Turns on the "quarter-size expand" (2x H + 2x V) hardware
	//ULWord * POINTER_32			ancBuffer;				//	Host ANC data buffer. If NULL, none transferred.
	//ULWord						ancBufferSize;			//	Capture:  before xfer: specifies max size of host ancBuffer; after xfer: actual number of ANC data bytes xferred
															//	Playout:  specifies number of ANC data bytes to xfer from host ancBuffer to device
															//	If zero, none transferred.
} AUTOCIRCULATE_TRANSFER_STRUCT_32, *PAUTOCIRCULATE_TRANSFER_STRUCT_32;


// Structure for autocirculate peer to peer transfers.  For p2p target specify kTransferFlagP2PPrepare
// for completion using kTransferFlagP2PComplete or kTransferFlagP2PTarget for completion with message transfer.  
// Autocirculate will write an AUTOCIRCULATE_P2P_STRUCT to the video buffer specified to the target.  Pass this 
// buffer as the video buffer to the autocirculate p2p source (kTransferFlagP2PTransfer) to do the p2p transfer.  
// For completion with kTransferFlagP2PComplete specify the transferFrame from the kTransferFlagP2PPrepare.
typedef struct
{
	ULWord		p2pSize;					// size of p2p structure
	ULWord		p2pflags;					// p2p transfer flags
	ULWord64	videoBusAddress;			// frame buffer bus address
	ULWord64	messageBusAddress;			// message register bus address (0 if not required)
	ULWord		videoBusSize;				// size of the video aperture (bytes)
	ULWord		messageData;				// message data (write to message bus address to complete video transfer)
} AUTOCIRCULATE_P2P_STRUCT, *PAUTOCIRCULATE_P2P_STRUCT, CHANNEL_P2P_STRUCT, *PCHANNEL_P2P_STRUCT;


#define AUTOCIRCULATE_TASK_VERSION		0x00000001
#define AUTOCIRCULATE_TASK_MAX_TASKS	128

/**
	@brief	These are the available AutoCirculate task types.
**/
typedef enum
{
	eAutoCircTaskNone,
	eAutoCircTaskRegisterWrite,		// AutoCircRegisterTask
	eAutoCircTaskRegisterRead,		// AutoCircRegisterTask
	eAutoCircTaskTimeCodeWrite,		// AutoCircTimeCodeTask
	eAutoCircTaskTimeCodeRead,		// AutoCircTimeCodeTask
	MAX_NUM_AutoCircTaskTypes
} AutoCircTaskType;


#define	NTV2_IS_VALID_TASK_TYPE(_x_)			((_x_) > eAutoCircTaskNone && (_x_) < MAX_NUM_AutoCircTaskTypes)

#define	NTV2_IS_REGISTER_READ_TASK(_x_)			((_x_) == eAutoCircTaskRegisterRead)
#define	NTV2_IS_REGISTER_WRITE_TASK(_x_)		((_x_) == eAutoCircTaskRegisterWrite)
#define	NTV2_IS_REGISTER_TASK(_x_)				(NTV2_IS_REGISTER_WRITE_TASK (_x_) || NTV2_IS_REGISTER_READ_TASK (_x_))

#define	NTV2_IS_TIMECODE_READ_TASK(_x_)			((_x_) == eAutoCircTaskTimeCodeRead)
#define	NTV2_IS_TIMECODE_WRITE_TASK(_x_)		((_x_) == eAutoCircTaskTimeCodeWrite)
#define	NTV2_IS_TIMECODE_TASK(_x_)				(NTV2_IS_TIMECODE_WRITE_TASK (_x_) || NTV2_IS_TIMECODE_READ_TASK (_x_))


typedef struct  
{
	ULWord	regNum;
	ULWord	mask;
	ULWord	shift;
	ULWord	value;
} AutoCircRegisterTask;

typedef struct  
{
	RP188_STRUCT	TCInOut1;
	RP188_STRUCT	TCInOut2;
	RP188_STRUCT	LTCEmbedded;
	RP188_STRUCT	LTCAnalog;
	RP188_STRUCT	LTCEmbedded2;
	RP188_STRUCT	LTCAnalog2;
	RP188_STRUCT	TCInOut3;
	RP188_STRUCT	TCInOut4;
	RP188_STRUCT	TCInOut5;
	RP188_STRUCT	TCInOut6;
	RP188_STRUCT	TCInOut7;
	RP188_STRUCT	TCInOut8;
	RP188_STRUCT	LTCEmbedded3;
	RP188_STRUCT	LTCEmbedded4;
	RP188_STRUCT	LTCEmbedded5;
	RP188_STRUCT	LTCEmbedded6;
	RP188_STRUCT	LTCEmbedded7;
	RP188_STRUCT	LTCEmbedded8;
} AutoCircTimeCodeTask;

typedef struct AutoCircGenericTask
{
	AutoCircTaskType taskType;
	union
	{
		AutoCircRegisterTask	registerTask;
		AutoCircTimeCodeTask	timeCodeTask;
	} u;

	#if !defined (NTV2_BUILDING_DRIVER)
		public:
			AJAExport explicit AutoCircGenericTask ()	{u.registerTask.regNum = u.registerTask.mask = u.registerTask.shift = u.registerTask.value = 0;}
	#endif	//	user-space clients only
} AutoCircGenericTask;

typedef struct
{
	ULWord taskVersion;
	ULWord taskSize;
	ULWord numTasks;
	ULWord maxTasks;
	Pointer64 taskArray;
	ULWord reserved0;
	ULWord reserved1;
	ULWord reserved2;
	ULWord reserved3;
} AUTOCIRCULATE_TASK_STRUCT_64, *PAUTOCIRCULATE_TASK_STRUCT_64;

typedef struct
{
	ULWord taskVersion;
	ULWord taskSize;
	ULWord numTasks;
	ULWord maxTasks;
	AutoCircGenericTask* taskArray;
	ULWord reserved0;
	ULWord reserved1;
	ULWord reserved2;
	ULWord reserved3;
} AUTOCIRCULATE_TASK_STRUCT, *PAUTOCIRCULATE_TASK_STRUCT;

typedef struct
{
	ULWord taskVersion;
	ULWord taskSize;
	ULWord numTasks;
	ULWord maxTasks;
	AutoCircGenericTask* POINTER_32 taskArray;
	ULWord reserved0;
	ULWord reserved1;
	ULWord reserved2;
	ULWord reserved3;
} AUTOCIRCULATE_TASK_STRUCT_32, *PAUTOCIRCULATE_TASK_STRUCT_32;


// Information about the currently programmed Xilinx .bit file
#define NTV2_BITFILE_DATETIME_STRINGLENGTH	(16)
#define NTV2_BITFILE_DESIGNNAME_STRINGLENGTH	(100)
#define NTV2_BITFILE_PARTNAME_STRINGLENGTH	(16)
// Increment this when you change the bitfile information structure
// And be sure to update the driver so it can handle the new version.
#define NTV2_BITFILE_STRUCT_VERSION			(4)

// There is room for up to 4kbytes after the audio in the last frame,
// but a 4KB data struct overflows the stack in the ioctl routine in
// the driver under Linux.  
//#define NTV2_BITFILE_RESERVED_ULWORDS		(244)
//#define NTV2_BITFILE_RESERVED_ULWORDS		(243)    // added bitFileType
//#define NTV2_BITFILE_RESERVED_ULWORDS		(239)	 // added designName
//#define NTV2_BITFILE_RESERVED_ULWORDS		(235)	 // added partName
#define NTV2_BITFILE_RESERVED_ULWORDS		(234)	 // added whichFPGA

typedef struct {
	ULWord			checksum;										// Platform-dependent.  Deprecated on Linux.
	ULWord			structVersion;									// Version of this structure	
	ULWord			structSize;										// Total size of this structure

    ULWord          numBytes;										// Xilinx bitfile bytecount
	char			dateStr[NTV2_BITFILE_DATETIME_STRINGLENGTH];	// Date Xilinx bitfile compiled
	char			timeStr[NTV2_BITFILE_DATETIME_STRINGLENGTH];	// Time Xilinx bitfile compiled
	char			designNameStr[NTV2_BITFILE_DESIGNNAME_STRINGLENGTH];
	
	ULWord			bitFileType;										// NTV2BitfileType
	
	char			partNameStr[NTV2_BITFILE_PARTNAME_STRINGLENGTH];	// Part name (v4)
	NTV2XilinxFPGA	whichFPGA;

	ULWord			reserved[NTV2_BITFILE_RESERVED_ULWORDS];	

} BITFILE_INFO_STRUCT,*PBITFILE_INFO_STRUCT;


typedef struct {
	NTV2DMAEngine	dmaEngine;
	ULWord			dmaFlags;						// flags passed into DMA currently bit 1 is set for to indicate weird 4096 10bit YUV 4K frame
	
	Pointer64		dmaHostBuffer;					// vitrual address of host buffer
	ULWord			dmaSize;						// number of bytes to DMA
	ULWord			dmaCardFrameNumber;				// card frame number 
	ULWord			dmaCardFrameOffset;				// offset (in bytes) into card frame to begin DMA
	ULWord			dmaNumberOfSegments;			// number of segments of size videoBufferSize to DMA
	ULWord			dmaSegmentSize;					// size of each segment (if videoNumSegments > 1)
	ULWord			dmaSegmentHostPitch;			// offset (in bytes) between the beginning of one host-memory segment and the beginning of the next host-memory segment
	ULWord			dmaSegmentCardPitch;			// offset (in bytes) between the beginning of one Kona-memory segment and the beginning of the next Kona-memory segment

	BOOL_			dmaToCard;						// direction of DMA transfer
	
} DMA_TRANSFER_STRUCT_64 ,*PDMA_TRANSFER_STRUCT_64;


// NOTE: Max bitfilestruct size was NTV2_AUDIO_READBUFFEROFFSET - NTV2_AUDIO_WRAPADDRESS
// but is now practically unlimited.

// The following structure is used to retrieve the timestamp values of the last video 
// interrupts. Use GetInterruptTimeStamps(&
typedef struct {
	LWord64				lastOutputVerticalTimeStamp;
	LWord64				lastInput1VerticalTimeStamp;
	LWord64				lastInput2VerticalTimeStamp;
} INTERRUPT_TIMESTAMP_STRUCT,*PINTERRUPT_TIMESTAMP_STRUCT;

// System status calls and structs associated with specific opcodes 

typedef enum
{
	SSC_GetFirmwareProgress,				// return firmware progress informaiton
	SSC_End									// end of list
} SystemStatusCode;

typedef enum
{
	kProgramStateEraseMainFlashBlock,
	kProgramStateEraseSecondFlashBlock,
	kProgramStateEraseFailSafeFlashBlock,
	kProgramStateProgramFlash,
	kProgramStateVerifyFlash,
	kProgramStateFinished,
	kProgramStateEraseBank3,
	kProgramStateProgramBank3,
	kProgramStateVerifyBank3,
	kProgramStateEraseBank4,
	kProgramStateProgramBank4,
	kProgramStateVerifyBank4,
	kProgramStateErasePackageInfo,
	kProgramStateProgramPackageInfo,
	kProgramStateVerifyPackageInfo,
	kProgramStateCalculating
} ProgramState;

typedef enum {
	kProgramCommandReadID=0x9F,
	kProgramCommandWriteEnable=0x06,
	kProgramCommandWriteDisable=0x04,
	kProgramCommandReadStatus=0x05,
	kProgramCommandWriteStatus=0x01,
	kProgramCommandReadFast=0x0B,
	kProgramCommandPageProgram=0x02,
	kProgramCommandSectorErase=0xD8,
	kProgramCommandBankWrite=0x17
} ProgramCommand;

typedef struct {
	ULWord			programTotalSize;		
	ULWord			programProgress;
	ProgramState	programState;
} SSC_GET_FIRMWARE_PROGRESS_STRUCT;

// System control calls and structs associated with specific opcodes 

typedef enum
{
	SCC_Test,								// just a test for now
	SCC_End									// end of list	
} SystemControlCode;

typedef struct {
	ULWord			param1;					// test parameter 1
	ULWord			param2;					// test parameter 2
} SCC_TEST_STRUCT;


// Build information 
#define NTV2_BUILD_STRINGLENGTH		(1024)
#define NTV2_BUILD_STRUCT_VERSION	(0)

#define NTV2_BUILD_RESERVED_BYTES	(1016)

typedef struct {
	ULWord			structVersion;						// Version of this structure	
	ULWord			structSize;							// Total size of this structure

	char			buildStr[NTV2_BUILD_STRINGLENGTH];	// User-defined build string
	unsigned char	reserved[NTV2_BUILD_RESERVED_BYTES];	

} BUILD_INFO_STRUCT,*PBUILD_INFO_STRUCT;


#ifdef AJAMac
#pragma options align=reset
#endif

	// used to filter the vout menu display 
typedef enum
{
	kUndefinedFilterFormats		= 0,		// Undefined
	kDropFrameFormats			= BIT(0),	// 23.98 / 29.97 / 59.94
	kNonDropFrameFormats		= BIT(1),	// 24 / 30 / 60
	kEuropeanFormats			= BIT(2),	// 25 / 50
	k1080ProgressiveFormats		= BIT(3),	// 1080p 23.98/24/29.97/30 (exclude 1080psf)
	kREDFormats					= BIT(4),	// RED's odd geometries
	k2KFormats					= BIT(5),	// 2K formats
	k4KFormats					= BIT(6)	// 4K formats
	
} ActiveVideoOutSelect;

// STUFF moved from ntv2macinterface.h that is now common
typedef enum DesktopFrameBuffStatus
{
	kDesktopFBIniting	= 0,		// waiting for Finder? Window Mgr? to discover us
	kDesktopFBOff		= 1,		// Running - not in use
	kDesktopFBOn		= 2			// Running - in-use as Mac Desktop
	
} DesktopFrameBuffStatus;


typedef enum SharedPrefsPermissions
{
	kSharedPrefsRead			= 0,
	kSharedPrefsReadWrite		= 1
	
} SharedPrefsPermissions;


#if !defined(R2_DEPRECATED)

typedef enum TimelapseUnits
{
	kTimelapseFrames			= 0,		// frames
	kTimelapseSeconds			= 1,		// seconds
	kTimelapseMinutes			= 2,		// minutes
	kTimelapseHours				= 3			// hours

} TimelapseUnits;

typedef enum
{
	kDefaultModeDesktop,					// deprecated
	kDefaultModeVideoIn,
	kDefaultModeBlack,						// deprecated
	kDefaultModeHold,
	kDefaultModeTestPattern,
	kDefaultModeUnknown
} DefaultVideoOutMode;

typedef enum
{
	kHDMIOutCSCAutoDetect,
	kHDMIOutCSCAutoSet,
	kHDMIOutCSCRGB8bit,
	kHDMIOutCSCRGB10bit,
	kHDMIOutCSCRGB12bit,
	kHDMIOutCSCYCbCr8bit,		// deprecated
	kHDMIOutCSCYCbCr10bit
} HDMIOutColorSpaceMode;

typedef enum
{
	kHDMIOutProtocolAutoDetect,
	kHDMIOutProtocolAutoSet,
	kHDMIOutProtocolHDMI,
	kHDMIOutProtocolDVI
} HDMIOutProtocolMode;

typedef enum
{
	kHDMIOutStereoOff,
	kHDMIOutStereoAuto,
	kHDMIOutStereoSideBySide,
	kHDMIOutStereoTopBottom,
	kHDMIOutStereoFramePacked
	
} HDMIOutStereoSelect;

enum TestPatternFormat
{
	kPatternFormatAuto,
	kPatternFormatYUV10b,
	kPatternFormatRGB10b,
	kPatternFormatYUV8b,
    kPatternFormatRGB8b,
    kPatternFormatRGB12b
};

// deprecated - NTV2TestPatternSelect
// note: this order mimics (somewhat) that of NTV2TestPatternSelect in "ntv2testpatterngen.h"
typedef enum
{
	kTestPatternColorBar100,		// 100% Bars
	kTestPatternColorBar75,			// 75% Bars
	kTestPatternRamp,				// Ramp
	kTestPatternMultiburst,			// Mulitburst
	kTestPatternLinesweep,			// Line Sweep
	kTestPatternPathological,		// Pathological
	kTestPatternFlatField,			// Flat Field (50%)
	kTestPatternMultiPattern,		// a swath of everything
	kTestPatternBlack,				// Black
	kTestPatternBorder,				// Border
	kTestPatternCustom				// Custom ("Load File...")

} TestPatternSelect;


#endif //R2_DEPRECATED



typedef enum
{
	kRP188SourceEmbeddedLTC		= 0x0,		// NOTE these values are same as RP188 DBB channel select
	kRP188SourceEmbeddedVITC1	= 0x1,
	kRP188SourceEmbeddedVITC2	= 0x2,
	kRP188SourceLTCPort			= 0xFE
} RP188SourceFilterSelect;

#if !defined(NTV2_DEPRECATE_15_2)
	typedef RP188SourceFilterSelect		RP188SourceSelect;
#endif	//	!defined(NTV2_DEPRECATE_15_2)


// Masks
enum
{
	// kRegUserState1
	kMaskInputFormatSelect		= BIT(0)  + BIT(1)  + BIT(2)  + BIT(3)  + BIT(4)  + BIT(5)  + BIT(6)  + BIT(7), 
	kMaskPrimaryFormatSelect	= BIT(8)  + BIT(9)  + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15), 
	kMaskSecondaryFormatSelect	= BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23),
	kMaskAnalogInBlackLevel		= BIT(24),
	kMaskAnalogInputType		= BIT(28) + BIT(29) + BIT(30) + BIT(31),
	
	//kRegIoHDGlobalStatus
	kMaskStandBusyStatus		= BIT(2),
	
	// kRegIoHDGlobalControl
	kMaskStandAloneMode			= BIT(0) + BIT(1) + BIT(2),
	kMaskDisplayMode			= BIT(4) + BIT(5) + BIT(6)  + BIT(7),
	kMaskDisplayModeTCType		= BIT(8) + BIT(9) + BIT(10) + BIT(11),	// TimecodeFormat - when set to zero, Timecode type follows primary format
	
	// kVRegStartupStatusFlags
	kMaskStartComplete			= BIT(0),
	kMaskDesktopDisplayReady    = BIT(1),
	kMaskDaemonInitialized		= BIT(2)
};

// isoch streams (channels)
enum DriverStartPhase
{
	kStartPhase1				= 1,				// These start out at 1 because they become a bit setting
	kStartPhase2				= 2
};

typedef enum
{
	kPrimarySecondaryDisplayMode,
	kPrimaryTimecodeDisplayMode
} IoHDDisplayMode;

#define KONA_DEBUGFILTER_STRINGLENGTH 128
typedef struct
{
	char includeString[KONA_DEBUGFILTER_STRINGLENGTH];
	char excludeString[KONA_DEBUGFILTER_STRINGLENGTH];
} KonaDebugFilterStringInfo;


#if !defined(NTV2_DEPRECATE_15_6)
	typedef struct
	{
		NTV2RelayState	manualControl12;
		NTV2RelayState	manualControl34;
		NTV2RelayState	relayPosition12;
		NTV2RelayState	relayPosition34;
		NTV2RelayState	watchdogStatus;
		bool			watchdogEnable12;
		bool			watchdogEnable34;
		ULWord			watchdogTimeout;
	} NTV2SDIWatchdogState;
#endif	//	!defined(NTV2_DEPRECATE_15_6)

typedef enum
{
	maskEnableHancY = BIT(0),
	shiftEnableHancY = 0,
	maskEnableVancY = BIT(4),
	shiftEnableVancY = 4,
	maskEnableHancC = BIT(8),
	shiftEnableHancC = 8,
	maskEnableVancC = BIT(12),
	shiftEnableVancC = 12,
	maskSetProgressive = BIT(16),
	shiftSetProgressive = 16,
	maskSyncro = BIT(24) + BIT(25),
	shiftSyncro = 24,
	maskDisableExtractor = BIT(28),
	shiftDisableExtractor = 28,
	maskEnableSDMux = BIT(30),
	shiftEnableSDMux = 30,
	maskGrabLSBs = BIT(31),
	shiftGrabLSBs = 31,
	maskField1CutoffLine = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftField1CutoffLine = 0,
	maskField2CutoffLine = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26),
	shiftField2CutoffLine = 16,
	maskTotalBytesIn = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) +
							BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15) +
							BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23),
	shiftTotalBytesIn = 0,
	maskTotalOverrun = BIT(28),
	shiftTotalOverrun = 28,
	maskField1BytesIn = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) +
							BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15) +
							BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23),
	shiftField1BytesIn = 0,
	maskField1Overrun = BIT(28),
	shiftField1Overrun = 28,
	maskField2BytesIn = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) +
							BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15) +
							BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23),
	shiftField2BytesIn = 0,
	maskField2Overrun = BIT(28),
	shiftField2Overrun = 28,
	maskField1StartLine = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftField1StartLine = 0,
	maskField2StartLine = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26),
	shiftField2StartLine = 16,
	maskTotalFrameLines = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftTotalFrameLines = 0,
	maskFIDHi = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftFIDHi = 0,
	maskFIDLow = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26),
	shiftFIDLow = 16,
	maskPktIgnore_1_5_9_13_17 = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7),
	shiftPktIgnore_1_5_9_13_17 = 0,
	maskPktIgnore_2_6_10_14_18 = BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	shiftPktIgnore_2_6_10_14_18 = 8,
	maskPktIgnore_3_7_11_15_19 = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23),
	shiftPktIgnore_3_7_11_15_19 = 16,
	maskPktIgnore_4_8_12_16_20 = BIT(24) + BIT(25) + BIT(26) + BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	shiftPktIgnore_4_8_12_16_20 = 24,
	maskField1AnalogStartLine = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftField1AnalogStartLine = 0,
	maskField2AnalogStartLine = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26),
	shiftField2AnalogStartLine = 16
} ANCExtMaskShift;

typedef enum
{
	regAncExt_FIRST,
	regAncExtControl	=	regAncExt_FIRST,	//	Reg 0 - Control register
	regAncExtField1StartAddress,				//	Reg 1 - f1_start_address[31:0]
	regAncExtField1EndAddress,					//	Reg 2 - f1_end_address[31:0]
	regAncExtField2StartAddress,				//	Reg 3 - f2_start_addr[31:0]
	regAncExtField2EndAddress,					//	Reg 4 - f2_end_address[31:0]
	regAncExtFieldCutoffLine,					//	Reg 5 - f2_cutoff_line[10:0], f1_cutoff_line[10:0]
	regAncExtTotalStatus,						//	Reg 6 - mem_sz_overrun, total_bytes[15:0]
	regAncExtField1Status,						//	Reg 7 - mem_sz_overrun_f1, total_bytes_f1[15:0]
	regAncExtField2Status,						//	Reg 8 - mem_sz_overrun_f2, total_bytes_f2[15:0]
	regAncExtFieldVBLStartLine,					//	Reg 9 - f2_vbl_start[10:0], f1_vbl_start[10:0]
	regAncExtTotalFrameLines,					//	Reg 10 - total_lines[10:0]
	regAncExtFID,								//	Reg 11 - fid_low[10:0], fid_hi[10:0]
	regAncExtIgnorePacketReg_1_2_3_4,			//	Reg 12 - Packet Ignore bytes
	regAncExtIgnorePktsReg_First	= regAncExtIgnorePacketReg_1_2_3_4,
	regAncExtIgnorePacketReg_5_6_7_8,			//	Reg 13 - Packet Ignore bytes
	regAncExtIgnorePacketReg_9_10_11_12,		//	Reg 14 - Packet Ignore bytes
	regAncExtIgnorePacketReg_13_14_15_16,		//	Reg 15 - Packet Ignore bytes
	regAncExtIgnorePacketReg_17_18_19_20,		//	Reg 16 - Packet Ignore bytes
	regAncExtIgnorePktsReg_Last		= regAncExtIgnorePacketReg_17_18_19_20,
	regAncExtAnalogStartLine,					//	Reg 17 - analog_start_line[10:0]
	regAncExtField1AnalogYFilter,				//	Reg 18 - analog_line_y_f1[31:0] - one bit per F1 line past analog_start_line, 1=captureAnalogY, 0=normal
	regAncExtField2AnalogYFilter,				//	Reg 19 - analog_line_y_f2[31:0] - one bit per F2 line past analog_start_line, 1=captureAnalogY, 0=normal
	regAncExtField1AnalogCFilter,				//	Reg 20 - analog_line_c_f1[31:0] - one bit per F1 line past analog_start_line, 1=captureAnalogC, 0=normal
	regAncExtField2AnalogCFilter,				//	Reg 21 - analog_line_c_f2[31:0] - one bit per F2 line past analog_start_line, 1=captureAnalogC, 0=normal
	regAncExtTwoFrameCadenceDetect,				//	Reg 22 - not used in NTV2
	regAncExtRP188Type,							//	Reg 23 - not used in NTV2
	regAncExtTimecodeStatus0_31,				//	Reg 24 - not used in NTV2
	regAncExtTimecodeStatus32_63,				//	Reg 25 - not used in NTV2
	regAncExtTimecodeStatusDBB,					//	Reg 26 - not used in NTV2
	regAncExtAnalogActiveLineLength,			//	Reg 27 - analog active line length
	regAncExt_LAST
} ANCExtRegisters;

typedef enum
{
	regAncIns_FIRST,
	regAncInsFieldBytes	=	regAncIns_FIRST,
	regAncInsControl,
	regAncInsField1StartAddr,
	regAncInsField2StartAddr,
	regAncInsPixelDelay,
	regAncInsActiveStart,
	regAncInsLinePixels,
	regAncInsFrameLines,
	regAncInsFieldIDLines,
	regAncInsPayloadIDControl,
	regAncInsPayloadID,
	regAncInsBlankCStartLine,
	regAncInsBlankField1CLines,
	regAncInsBlankField2CLines,
	regAncInsFieldBytesHigh,
	regAncInsReserved15,
	regAncInsRtpPayloadID,
	regAncInsRtpSSRC,
	regAncInsIpChannel,
	regAncIns_LAST
} ANCInsRegisters;

typedef enum
{
	maskInsField1Bytes = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10) + BIT(11) + BIT(12) + BIT(13) + BIT(14) + BIT(15),
	shiftInsField1Bytes = 0,
	maskInsField2Bytes = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + +BIT(27) + BIT(28) + BIT(29) + BIT(30) + BIT(31),
	shiftInsField2Bytes = 16,
	maskInsEnableHancY = BIT(0),
	shiftInsEnableHancY = 0,
	maskInsEnableVancY = BIT(4),
	shiftInsEnableVancY = 4,
	maskInsEnableHancC = BIT(8),
	shiftInsEnableHancC = 8,
	maskInsEnableVancC = BIT(12),
	shiftInsEnableVancC = 12,
	maskInsSetProgressive = BIT(24),
	shiftInsSetProgressive = 24,
	maskInsDisableInserter = BIT(28),
	shiftInsDisableInserter = 28,
	maskInsExtendedMode = BIT(29),
	shiftInsExtendedMode = 29,
	maskInsEnablePktSplitSD = BIT(31),
	shiftInsEnablePktSplitSD = 31,
	maskInsHancDelay = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9),
	shiftINsHancDelay = 0,
	maskInsVancDelay = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26),
	shiftInsVancDelay = 16,
	maskInsField1FirstActive = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftInsField1FirstActive = 0,
	maskInsField2FirstActive = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26),
	shiftInsField2FirstActive = 16,
	maskInsActivePixelsInLine = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftInsActivePixelsInLine = 0,
	maskInsTotalPixelsInLine = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26) + BIT(27),
	shiftInsTotalPixelsInLine = 16,
	maskInsTotalLinesPerFrame = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftInsTotalLinesPerFrame = 0,
	maskInsFieldIDHigh = BIT(0) + BIT(1) + BIT(2) + BIT(3) + BIT(4) + BIT(5) + BIT(6) + BIT(7) + BIT(8) + BIT(9) + BIT(10),
	shiftInsFieldIDHigh = 0,
	maskInsFieldIDLow = BIT(16) + BIT(17) + BIT(18) + BIT(19) + BIT(20) + BIT(21) + BIT(22) + BIT(23) + BIT(24) + BIT(25) + BIT(26),
	shiftInsFieldIDLow = 16

} ANCInsMaskShift;


//	Driver Version ULWord encode/decode macros
//	Introduced in SDK 15.0
//	Common ULWord format on all platforms:
//
//		 3         2         1          
//		10987654321098765432109876543210
//		TTXMMMMMMMmmmmmmPPPPPPbbbbbbbbbb
//
//	MMMMMMM:	[28:22]	major version number
//	mmmmmm:		[21:16]	minor version number
//	PPPPPP:		[15:10]	point release number
//	bbbbbbbbbb:	[9:0]	build number
//	TT:			[31:30]	build type (0=release, 1=beta, 2=alpha, 3=dev)
#if defined(__CPLUSPLUS__) || defined(__cplusplus)
	#define	NTV2DriverVersionEncode(__maj__, __min__, __pt__, __bld__)			((ULWord(__maj__) & 0x0000007F) << 22)		\
																			|	((ULWord(__min__) & 0x0000003F) << 16)		\
																			|	((ULWord(__pt__ ) & 0x0000003F) << 10)		\
																			|	((ULWord(__bld__) & 0x000003FF) <<  0)
	#define	NTV2DriverVersionDecode_Major(__vers__)		((ULWord(__vers__) >> 22) & 0x0000007F)
	#define	NTV2DriverVersionDecode_Minor(__vers__)		((ULWord(__vers__) >> 16) & 0x0000003F)
	#define	NTV2DriverVersionDecode_Point(__vers__)		((ULWord(__vers__) >> 10) & 0x0000003F)
	#define	NTV2DriverVersionDecode_Build(__vers__)		((ULWord(__vers__) >>  0) & 0x000003FF)
#else
	#define	NTV2DriverVersionEncode(__maj__, __min__, __pt__, __bld__)			(((ULWord)(__maj__) & 0x0000007F) << 22)		\
																			|	(((ULWord)(__min__) & 0x0000003F) << 16)		\
																			|	(((ULWord)(__pt__ ) & 0x0000003F) << 10)		\
																			|	(((ULWord)(__bld__) & 0x000003FF) <<  0)
	#define	NTV2DriverVersionDecode_Major(__vers__)		(((ULWord)(__vers__) >> 22) & 0x0000007F)
	#define	NTV2DriverVersionDecode_Minor(__vers__)		(((ULWord)(__vers__) >> 16) & 0x0000003F)
	#define	NTV2DriverVersionDecode_Point(__vers__)		(((ULWord)(__vers__) >> 10) & 0x0000003F)
	#define	NTV2DriverVersionDecode_Build(__vers__)		(((ULWord)(__vers__) >>  0) & 0x000003FF)
#endif

//	Pack/Unpack SDK version in & out of ULWord:
#define	NTV2SDKVersionEncode		NTV2DriverVersionEncode
#define	NTV2SDKVersionDecode_Major	NTV2DriverVersionDecode_Major
#define	NTV2SDKVersionDecode_Minor	NTV2DriverVersionDecode_Minor
#define	NTV2SDKVersionDecode_Point	NTV2DriverVersionDecode_Point
#define	NTV2SDKVersionDecode_Build	NTV2DriverVersionDecode_Build


//////////////////////////////////////////////////////////////////////////////////////////////	BEGIN NEW AUTOCIRCULATE API

		#if AJATargetBigEndian
			#if defined(__CPLUSPLUS__) || defined(__cplusplus)
				#define	NTV2_4CC(_str_)					(	(uint32_t((UByte*(_str_))[0]) <<  0)  |	\
															(uint32_t((UByte*(_str_))[1]) <<  8)  |	\
															(uint32_t((UByte*(_str_))[2]) << 16)  |	\
															(uint32_t((UByte*(_str_))[3]) << 24))

				#define NTV2_FOURCC(_a_,_b_,_c_,_d_)	(	((uint32_t(_a_)) <<  0)	|		\
															((uint32_t(_b_)) <<  8)	|		\
															((uint32_t(_c_)) << 16)	|		\
															((uint32_t(_d_)) << 24))
				#if !defined (NTV2_BUILDING_DRIVER)
					#define	NTV2_4CC_AS_STRING(_x_)			std::string (1, ((_x_) & 0x000000FF) >>  0)	+	\
															std::string (1, ((_x_) & 0x0000FF00) >>  8)	+	\
															std::string (1, ((_x_) & 0x00FF0000) >> 16)	+	\
															std::string (1, ((_x_) & 0xFF000000) >> 24)
				#endif	//	!defined (NTV2_BUILDING_DRIVER)
			#else
				#define	NTV2_4CC(_str_)					(	((uint32_t)(((UByte *)(_str_))[0]) <<  0)  |	\
															((uint32_t)(((UByte *)(_str_))[1]) <<  8)  |	\
															((uint32_t)(((UByte *)(_str_))[2]) << 16)  |	\
															((uint32_t)(((UByte *)(_str_))[3]) << 24))

				#define NTV2_FOURCC(_a_,_b_,_c_,_d_)	(	(((uint32_t)(_a_)) <<  0)	|		\
															(((uint32_t)(_b_)) <<  8)	|		\
															(((uint32_t)(_c_)) << 16)	|		\
															(((uint32_t)(_d_)) << 24))
			#endif	//	C
		#else
			#if defined(__CPLUSPLUS__) || defined(__cplusplus)
				#define	NTV2_4CC(_str_)					(	(uint32_t((UByte*(_str_))[3]) <<  0)  |	\
															(uint32_t((UByte*(_str_))[2]) <<  8)  |	\
															(uint32_t((UByte*(_str_))[1]) << 16)  |	\
															(uint32_t((UByte*(_str_))[0]) << 24))

				#define	NTV2_FOURCC(_a_,_b_,_c_,_d_)	(	((uint32_t(_a_)) << 24)	|		\
															((uint32_t(_b_)) << 16)	|		\
															((uint32_t(_c_)) <<  8)	|		\
															((uint32_t(_d_)) <<  0) )
				#if !defined (NTV2_BUILDING_DRIVER)
					#define	NTV2_4CC_AS_STRING(_x_)			std::string(1, char((uint32_t(_x_) & 0xFF000000) >> 24)) +	\
															std::string(1, char((uint32_t(_x_) & 0x00FF0000) >> 16)) +	\
															std::string(1, char((uint32_t(_x_) & 0x0000FF00) >>  8)) +	\
															std::string(1, char((uint32_t(_x_) & 0x000000FF) >>  0))
				#endif	//	!defined (NTV2_BUILDING_DRIVER)
			#else
				#define	NTV2_4CC(_str_)					(	((uint32_t)(((UByte *)(_str_))[3]) <<  0)  |	\
															((uint32_t)(((UByte *)(_str_))[2]) <<  8)  |	\
															((uint32_t)(((UByte *)(_str_))[1]) << 16)  |	\
															((uint32_t)(((UByte *)(_str_))[0]) << 24))
	
				#define	NTV2_FOURCC(_a_,_b_,_c_,_d_)	(	(((uint32_t)(_a_)) << 24)	|		\
															(((uint32_t)(_b_)) << 16)	|		\
															(((uint32_t)(_c_)) <<  8)	|		\
															(((uint32_t)(_d_)) <<  0) )
			#endif	//	C
		#endif	//	else Little-Endian


		/**
			@brief	32-bit host addresses go into the upper 4 bytes of the ULWord64, while the lower 4 bytes contain 0xBAADF00D.
					64-bit host addresses consume the entire ULWord64.
		**/
		#define	NTV2_POINTER_TO_ULWORD64(__p__)		((sizeof (int *) == 4)  ?  (ULWord64 (ULWord64 (__p__) << 32) | 0x00000000BAADF00D)  :  ULWord64 (__p__))

		#define	NTV2_CURRENT_HEADER_VERSION		0					///< @brief	Current version of NTV2_HEADER struct, originally 0
		#define	NTV2_CURRENT_TRAILER_VERSION	0					///< @brief	Current version of NTV2_TRAILER struct, originally 0

		#define	AUTOCIRCULATE_STRUCT_VERSION	0									///< @brief	Version number of AutoCirculate structures, originally 0

		#define	NTV2_HEADER_TAG					NTV2_FOURCC ('N', 'T', 'V', '2')	///< @brief	Identifies the struct header
		#define	NTV2_TRAILER_TAG				NTV2_FOURCC ('n', 't', 'v', '2')	///< @brief	Identifies the struct trailer

		#define	NTV2_IS_VALID_HEADER_TAG(_x_)	((_x_) == NTV2_HEADER_TAG)
		#define	NTV2_IS_VALID_TRAILER_TAG(_x_)	((_x_) == NTV2_TRAILER_TAG)

        #define	NTV2_TYPE_VIRTUAL_DATA_RW		NTV2_FOURCC ('v', 'd', 'a', 't')	///< @brief	Identifies NTV2VirtualData struct
        #define	NTV2_TYPE_BANKGETSET			NTV2_FOURCC ('b', 'n', 'k', 'S')	///< @brief	Identifies NTV2BankSelGetSetRegs struct
		#define	AUTOCIRCULATE_TYPE_STATUS		NTV2_FOURCC ('s', 't', 'a', 't')	///< @brief	Identifies AUTOCIRCULATE_STATUS struct
		#define	AUTOCIRCULATE_TYPE_XFER			NTV2_FOURCC ('x', 'f', 'e', 'r')	///< @brief	Identifies AUTOCIRCULATE_TRANSFER struct
		#define	AUTOCIRCULATE_TYPE_XFERSTATUS	NTV2_FOURCC ('x', 'f', 's', 't')	///< @brief	Identifies AUTOCIRCULATE_TRANSFER_STATUS struct
		#define	AUTOCIRCULATE_TYPE_TASK			NTV2_FOURCC ('t', 'a', 's', 'k')	///< @brief	Identifies AUTOCIRCULATE_TASK struct
		#define	AUTOCIRCULATE_TYPE_FRAMESTAMP	NTV2_FOURCC ('s', 't', 'm', 'p')	///< @brief	Identifies FRAME_STAMP struct
		#define	AUTOCIRCULATE_TYPE_GETREGS		NTV2_FOURCC ('r', 'e', 'g', 'R')	///< @brief	Identifies NTV2GetRegisters struct
		#define	AUTOCIRCULATE_TYPE_SETREGS		NTV2_FOURCC ('r', 'e', 'g', 'W')	///< @brief	Identifies NTV2SetRegisters struct
		#define	AUTOCIRCULATE_TYPE_SDISTATS		NTV2_FOURCC ('s', 'd', 'i', 'S')	///< @brief	Identifies NTV2SDIStatus struct
        #define	NTV2_TYPE_AJADEBUGLOGGING		NTV2_FOURCC ('d', 'b', 'l', 'g')	///< @brief	Identifies NTV2DebugLogging struct
		#define	NTV2_TYPE_AJABUFFERLOCK			NTV2_FOURCC ('b', 'f', 'l', 'k')	///< @brief	Identifies NTV2BufferLock struct
		#define	NTV2_TYPE_AJABITSTREAM			NTV2_FOURCC ('b', 't', 's', 't')	///< @brief	Identifies NTV2Bitstream struct

		#define	NTV2_IS_VALID_STRUCT_TYPE(_x_)	(	(_x_) == AUTOCIRCULATE_TYPE_STATUS		||	\
													(_x_) == AUTOCIRCULATE_TYPE_XFER		||	\
													(_x_) == AUTOCIRCULATE_TYPE_XFERSTATUS	||	\
													(_x_) == AUTOCIRCULATE_TYPE_TASK		||	\
													(_x_) == AUTOCIRCULATE_TYPE_FRAMESTAMP	||	\
													(_x_) == AUTOCIRCULATE_TYPE_GETREGS		||	\
													(_x_) == AUTOCIRCULATE_TYPE_SETREGS		||	\
													(_x_) == AUTOCIRCULATE_TYPE_SDISTATS	||	\
                                                    (_x_) == NTV2_TYPE_BANKGETSET			||	\
                                                    (_x_) == NTV2_TYPE_VIRTUAL_DATA_RW		||	\
													(_x_) == NTV2_TYPE_AJADEBUGLOGGING		||	\
													(_x_) == NTV2_TYPE_AJABUFFERLOCK		||	\
													(_x_) == NTV2_TYPE_AJABITSTREAM	)


		//	NTV2_POINTER FLAGS
		#define	NTV2_POINTER_ALLOCATED				BIT(0)		///< @brief	Allocated using Allocate function?
		#define	NTV2_POINTER_PAGE_ALIGNED			BIT(1)		///< @brief	Allocated page-aligned?


		//	AUTOCIRCULATE OPTION FLAGS
		#define	AUTOCIRCULATE_WITH_RP188			BIT(0)		///< @brief	Use this to AutoCirculate with RP188
		#define	AUTOCIRCULATE_WITH_LTC				BIT(1)		///< @brief	Use this to AutoCirculate with analog LTC
		#define	AUTOCIRCULATE_WITH_FBFCHANGE		BIT(2)		///< @brief	Use this to AutoCirculate with the possibility of frame buffer format changes
		#define	AUTOCIRCULATE_WITH_FBOCHANGE		BIT(3)		///< @brief	Use this to AutoCirculate with the possibility of frame buffer orientation changes
		#define	AUTOCIRCULATE_WITH_COLORCORRECT		BIT(4)		///< @brief	Use this to AutoCirculate with color correction
		#define	AUTOCIRCULATE_WITH_VIDPROC			BIT(5)		///< @brief	Use this to AutoCirculate with video processing
		#define	AUTOCIRCULATE_WITH_ANC				BIT(6)		///< @brief	Use this to AutoCirculate with ancillary data
		#define	AUTOCIRCULATE_WITH_AUDIO_CONTROL	BIT(7)		///< @brief	Use this to AutoCirculate with no audio but with audio control
		#define	AUTOCIRCULATE_WITH_FIELDS			BIT(8)		///< @brief	Use this to AutoCirculate with fields as frames for interlaced formats
		#define AUTOCIRCULATE_WITH_HDMIAUX			BIT(9)		///< @brief	Use this to AutoCirculate with HDMI auxiliary data

		#define AUTOCIRCULATE_FRAME_FULL			BIT(20)		///< @brief Frame contains a full image
		#define AUTOCIRCULATE_FRAME_FIELD0			BIT(21)		///< @brief Frame contains field 0 of an interlaced image (first field in time)
		#define AUTOCIRCULATE_FRAME_FIELD1			BIT(22)		///< @brief Frame contains field 1 of an interlaced image (second field in time)

		#define AUTOCIRCULATE_P2P_PREPARE			BIT(28)		///< @brief prepare p2p target for synchronous transfer (no message)
		#define AUTOCIRCULATE_P2P_COMPLETE			BIT(29)		///< @brief complete synchronous p2p transfer
		#define AUTOCIRCULATE_P2P_TARGET			BIT(30)		///< @brief prepare p2p target for asynchronous transfer (with message)
		#define AUTOCIRCULATE_P2P_TRANSFER			BIT(31)		///< @brief transfer to p2p sync or async target

		#define DMABUFFERLOCK_LOCK					BIT(0)		///< @brief Used in ::NTV2BufferLock to page lock a buffer.
		#define DMABUFFERLOCK_UNLOCK_ALL			BIT(1)		///< @brief Used in ::NTV2BufferLock to unlock all locked buffers.
        #define DMABUFFERLOCK_MAP                   BIT(2)		///< @brief Used in ::NTV2BufferLock to IO map a buffer.
		#define DMABUFFERLOCK_UNLOCK				BIT(3)		///< @brief Used in ::NTV2BufferLock to unlock a buffer.
		#define DMABUFFERLOCK_AUTO					BIT(4)		///< @brief Used in ::NTV2BufferLock to auto page lock buffers.
		#define DMABUFFERLOCK_MANUAL				BIT(5)		///< @brief Used in ::NTV2BufferLock to manual page lock buffers.
		#define DMABUFFERLOCK_MAX_SIZE				BIT(6)		///< @brief Used in ::NTV2BufferLock to set max locked size.
        #define DMABUFFERLOCK_RDMA					BIT(7)		///< @brief Used in ::NTV2BufferLock to page lock a rdma buffer

		// Bitstream flags
		#define BITSTREAM_WRITE						BIT(0)		///< @brief Used in ::NTV2Bitstream to write a bitstream
		#define BITSTREAM_FRAGMENT					BIT(1)		///< @brief Used in ::NTV2Bitstream to indicate bitstream is a fragment
		#define BITSTREAM_SWAP						BIT(2)		///< @brief Used in ::NTV2Bitstream to byte swap bitstream data
		#define BITSTREAM_RESET_CONFIG				BIT(3)		///< @brief Used in ::NTV2Bitstream to reset config
		#define BITSTREAM_RESET_MODULE				BIT(4)		///< @brief Used in ::NTV2Bitstream to reset module
		#define BITSTREAM_READ_REGISTERS			BIT(5)		///< @brief Used in ::NTV2Bitstream to get status registers

		// Bitstream registers
		#define BITSTREAM_EXT_CAP					0			///< @brief Extended capability register
		#define BITSTREAM_VENDOR_HEADER				1			///< @brief Vendor-specific register
		#define BITSTREAM_JTAG_ID					2			///< @brief JTAG ID register
		#define BITSTREAM_VERSION					3			///< @brief Bitstream version register
		#define BITSTREAM_MCAP_STATUS				4			///< @brief MCAP status register
		#define BITSTREAM_MCAP_CONTROL				5			///< @brief MCAP control register
		#define BITSTREAM_MCAP_DATA					6			///< @brief MCAP data register
		#define BITSTREAM_NUM_REGISTERS				7			///< @brief Number of MCAP registes
	
		#if !defined (NTV2_BUILDING_DRIVER)
			/**
				Convenience macros that delimit the new structs.
				For driver builds, the structs are simply structs.
				For client builds, the structs are classes with the appropriate __declspec(dllexport) or __declspec(dllimport) decorations.
			**/
			#define	NTV2_STRUCT_BEGIN(__struct_name__)		class AJAExport __struct_name__ {public:
			#define	NTV2_STRUCT_END(__struct_name__)		};
			#define	NTV2_BEGIN_PRIVATE						private:
			#define	NTV2_END_PRIVATE						public:

			#if defined (_DEBUG)
				#define	NTV2_IS_STRUCT_VALID_IMPL(__hr__,__tr__)	bool NTV2_IS_STRUCT_VALID (void) const		{return __hr__.IsValid() && __tr__.IsValid();}
				#define	NTV2_ASSERT_STRUCT_VALID					do	{NTV2_ASSERT (NTV2_IS_STRUCT_VALID ());} while (false)
			#else
				#define	NTV2_IS_STRUCT_VALID_IMPL(__hr__,__tr__)
				#define	NTV2_ASSERT_STRUCT_VALID
			#endif

			//	Convenience macros for compactly formatting ostream output...
			#define	Hex(__x__)				std::hex << (__x__) << std::dec
			#define	xHex(__x__)				"0x" << Hex(__x__)
			#define	HexN(__x__,__n__)		std::hex << std::setw(int(__n__)) << (__x__) << std::dec
			#define	xHexN(__x__,__n__)		"0x" << HexN((__x__),(__n__))
			#define	Hex0N(__x__,__n__)		std::hex << std::setw(int(__n__)) << std::setfill('0') << (__x__) << std::dec << std::setfill(' ')
			#define	xHex0N(__x__,__n__)		"0x" << Hex0N((__x__),(__n__))
			#define	HEX(__x__)				std::hex << std::uppercase << (__x__) << std::dec << std::nouppercase
			#define	xHEX(__x__)				"0x" << HEX(__x__)
			#define	HEXN(__x__,__n__)		std::hex << std::uppercase << std::setw(int(__n__)) << (__x__) << std::dec << std::nouppercase
			#define	xHEXN(__x__,__n__)		"0x" << HEXN((__x__),(__n__))
			#define	HEX0N(__x__,__n__)		std::hex << std::uppercase << std::setw(int(__n__)) << std::setfill('0') << (__x__) << std::dec << std::setfill(' ') << std::nouppercase
			#define	xHEX0N(__x__,__n__)		"0x" << HEX0N((__x__),(__n__))
			#define	DEC(__x__)				std::dec << std::right << (__x__)
			#define	DECN(__x__,__n__)		std::dec << std::setw(int(__n__)) << std::right << (__x__)
			#define	DEC0N(__x__,__n__)		std::dec << std::setw(int(__n__)) << std::setfill('0') << std::right << (__x__) << std::dec << std::setfill(' ')
			#define	OCT(__x__)				std::oct << (__x__) << std::dec
			#define	OCT0N(__x__,__n__)		std::oct << std::setw(int(__n__)) << std::setfill('0') << (__x__) << std::dec << std::setfill(' ')
			#define	oOCT(__x__)				"o" << std::oct << (__x__) << std::dec
			#define	oOCT0N(__x__,__n__)		"o" << std::oct << std::setw(int(__n__)) << std::setfill('0') << (__x__) << std::dec << std::setfill(' ')
			#define	BIN064(__x__)			std::bitset<8>((uint64_t(__x__)&0xFF00000000000000)>>56) << "."		\
												<< std::bitset<8>((uint64_t(__x__)&0x00FF000000000000)>>48) << "."		\
												<< std::bitset<8>((uint64_t(__x__)&0x0000FF0000000000)>>40) << "."		\
												<< std::bitset<8>((uint64_t(__x__)&0x000000FF00000000)>>32) << "."		\
												<< std::bitset<8>((uint64_t(__x__)&0x00000000FF000000)>>24) << "."		\
												<< std::bitset<8>((uint64_t(__x__)&0x0000000000FF0000)>>16) << "."		\
												<< std::bitset<8>((uint64_t(__x__)&0x000000000000FF00)>>8) << "."		\
												<< std::bitset<8>( uint64_t(__x__)&0x00000000000000FF)
			#define	BIN032(__x__)			std::bitset<8>((uint32_t(__x__)&0xFF000000)>>24) << "."						\
												<< std::bitset<8>((uint32_t(__x__)&0x00FF0000)>>16) << "."				\
												<< std::bitset<8>((uint32_t(__x__)&0x0000FF00)>>8) << "."				\
												<< std::bitset<8>( uint32_t(__x__)&0x000000FF)
			#define	BIN016(__x__)			std::bitset<8>((uint16_t(__x__)&0xFF00)>>8) << "."							\
												<< std::bitset<8>( uint16_t(__x__)&0x00FF)
			#define	BIN012(__x__)			std::bitset<12>((uint16_t(__x__)&0x0FFF))
			#define	BIN010(__x__)			std::bitset<10>((uint16_t(__x__)&0x03FF))
			#define	BIN08(__x__)			std::bitset<8>(uint8_t(__x__))
			#define	BIN04(__x__)			std::bitset<4>(uint8_t(__x__))
			#define	BIN0N(__x__,__n__)		std::bitset<__n__>(uint8_t(__x__))
			#define	bBIN064(__x__)			"b"	<< BIN064(__x__)
			#define	bBIN032(__x__)			"b"	<< BIN032(__x__)
			#define	bBIN016(__x__)			"b"	<< BIN016(__x__)
			#define	bBIN012(__x__)			"b"	<< BIN012(__x__)
			#define	bBIN010(__x__)			"b"	<< BIN010(__x__)
			#define	bBIN08(__x__)			"b"	<< BIN08(__x__)
			#define	bBIN04(__x__)			"b"	<< BIN04(__x__)
			#define	bBIN0N(__x__,__n__)		"b"	<< BIN0N(__x__,__n__)
			#define	fDEC(__x__,__w__,__p__)	std::dec << std::fixed << std::setw(__w__) << std::setprecision(__p__) << (__x__)
		#else
			#define	NTV2_STRUCT_BEGIN(__struct_name__)		typedef struct __struct_name__ {
			#define	NTV2_STRUCT_END(__struct_name__)		} __struct_name__;
			#define	NTV2_BEGIN_PRIVATE
			#define	NTV2_END_PRIVATE
			#define	NTV2_IS_STRUCT_VALID_IMPL(__hr__,__tr__)
			#define	NTV2_ASSERT_STRUCT_VALID
		#endif

		#if !defined (NTV2_BUILDING_DRIVER)
			typedef	std::vector<uint8_t>				UByteSequence;				///< @brief	An ordered sequence of UByte (uint8_t) values.
			typedef	UByteSequence::const_iterator		UByteSequenceConstIter;		///< @brief	A handy const iterator for iterating over a UByteSequence.
			typedef	UByteSequence::iterator				UByteSequenceIter;			///< @brief	A handy non-const iterator for iterating over a UByteSequence.

			typedef	std::vector<uint16_t>				UWordSequence;				///< @brief	An ordered sequence of UWord (uint16_t) values.
			typedef	UWordSequence::const_iterator		UWordSequenceConstIter;		///< @brief	A handy const iterator for iterating over a UWordSequence.
			typedef	UWordSequence::iterator				UWordSequenceIter;			///< @brief	A handy non-const iterator for iterating over a UWordSequence.

			typedef	std::vector<uint32_t>				ULWordSequence;				///< @brief	An ordered sequence of ULWord (uint32_t) values.
			typedef	ULWordSequence::const_iterator		ULWordSequenceConstIter;	///< @brief	A handy const iterator for iterating over a ULWordSequence.
			typedef	ULWordSequence::iterator			ULWordSequenceIter;			///< @brief	A handy non-const iterator for iterating over a ULWordSequence.

			typedef	std::vector<uint64_t>				ULWord64Sequence;			///< @brief	An ordered sequence of ULWord64 (uint64_t) values.
			typedef	ULWord64Sequence::const_iterator	ULWord64SequenceConstIter;	///< @brief	A handy const iterator for iterating over a ULWord64Sequence.
			typedef	ULWord64Sequence::iterator			ULWord64SequenceIter;		///< @brief	A handy non-const iterator for iterating over a ULWord64Sequence.
		#endif	//	NTV2_BUILDING_DRIVER


		#if defined (AJAMac)
			#pragma pack (push, 4)
		#endif	//	defined (AJAMac)


		#if !defined (NTV2_BUILDING_DRIVER)
		/**
			@brief		Describes a segmented data transfer (copy or move).
			@details	A segmented transfer has the following pieces of information:
						-	A starting source and destination offset, in elements;
						-	A source and destination pitch (span between segments, in elements);
						-	A segment length, in elements;
						-	A segment count.
						It also has some optional attributes:
						-	Element size, in bytes. Defaults to 1 byte per element. Must be power-of-2. Maximum is 8.
						-	Optional "source vertical flip" flag to indicate that the source offset is interpreted as an offset, in elements,
							from the bottom of the source buffer, and during the transfer, the source pitch is subtracted instead of added.
							Defaults to normal "from top" source offset reference.
						-	Optional "destination vertical flip" flag to indicate that the destination offset is interpreted as an offset, in
							elements, from the bottom of the destination buffer, and during the transfer, the destination pitch is subtracted
							instead of added. Defaults to normal "from top" destination offset reference.
		**/
		class AJAExport NTV2SegmentedXferInfo
		{
			public:
				/**
					@brief	Constructs me as empty/invalid.
				**/
								NTV2SegmentedXferInfo()
									:	mFlags				(0),
										mNumSegments		(0),
										mElementsPerSegment	(0),
										mInitialSrcOffset	(0),
										mInitialDstOffset	(0),
										mSrcElementsPerRow	(0),
										mDstElementsPerRow	(0)		{setElementLength(1);}

				/**
					@name	Inquiry -- Essentials
				**/
				///@{
				/**
					@return	True if valid (i.e. a non-zero segment count and segment length);  otherwise false.
				**/
				inline bool		isValid (void) const	{return getSegmentCount() && getSegmentLength() ? true : false;}

				/**
					@return	The number of segments.
				**/
				inline ULWord	getSegmentCount (void) const		{return mNumSegments;}

				/**
					@return	The segment length, in elements.
				**/
				inline ULWord	getSegmentLength (void) const		{return mElementsPerSegment;}

				/**
					@return	The offset, in elements, to the start of the first source segment.
				**/
				inline ULWord	getSourceOffset (void) const		{return mInitialSrcOffset;}

				/**
					@return	The offset, in elements, to the start of the first destination segment.
				**/
				inline ULWord	getDestOffset (void) const			{return mInitialDstOffset;}

				/**
					@return	The number of elements between each source row.
				**/
				inline ULWord	getSourcePitch (void) const			{return mSrcElementsPerRow;}

				/**
					@return	The number of elements between each destination row.
				**/
				inline ULWord	getDestPitch (void) const			{return mDstElementsPerRow;}
				///@}

				/**
					@name	Inquiry
				**/
				///@{
				/**
					@return	The size of each element, in bytes.
				**/
				inline ULWord	getElementLength (void) const		{return ULWord(1 << (mFlags & 3));}

				/**
					@return	True if source rows should be traversed bottom-to-top;  otherwise false.
				**/
				inline bool		isSourceBottomUp (void) const		{return mFlags & BIT(8) ? true : false;}

				/**
					@return	True if source rows should be traversed top-to-bottom;  otherwise false.
				**/
				inline bool		isSourceTopDown (void) const		{return mFlags & BIT(8) ? false : true;}

				/**
					@return	True if destination rows should be traversed bottom-to-top;  otherwise false.
				**/
				inline bool		isDestBottomUp (void) const			{return mFlags & BIT(9) ? true : false;}

				/**
					@return	True if destination rows should be traversed top-to-bottom;  otherwise false.
				**/
				inline bool		isDestTopDown (void) const			{return mFlags & BIT(9) ? false : true;}

				/**
					@return	The total number of elements (i.e. the product of the segment count and length).
				**/
				ULWord			getTotalElements (void) const		{return getSegmentCount() * getSegmentLength();}

				/**
					@return	The total number of bytes.
				**/
				ULWord			getTotalBytes (void) const			{return getTotalElements() * getElementLength();}

				/**
					@return	The offset to the first element immediately past the last source segment.
				**/
				inline ULWord	getSourceEndOffset (void) const	//	New in SDK 16.0
								{return getSourceOffset()  +  getSourcePitch() * getSegmentCount()  +  getSegmentLength();}

				/**
					@return	The offset to the first element immediately past the last destination segment.
				**/
				inline ULWord	getDestEndOffset (void) const	//	New in SDK 16.0
								{return getDestOffset()  +  getDestPitch() * getSegmentCount()  +  getSegmentLength();}

				/**
					@brief		Writes a human-readable description of me into a given output stream.
					@param		inStrm			A non-constant reference to the output stream that will receive the description.
					@param[in]	inDumpSegments	If true, also dumps a description of each segment. Defaults to false.
					@return		A reference to the output stream.
				**/
				std::ostream &	Print (std::ostream & inStrm, const bool inDumpSegments = false) const;

				/**
					@param[in]	inInclDecl	If true, the default, include a declaration statement in the source code.
					@return		A string containing C++ source code that, when compiled, will result in a NTV2SegmentedXferInfo
								instance that will perfectly match me.
				**/
				std::string		getSourceCode (const bool inInclDecl = true) const;	//	New in SDK 16.0

				/**
					@param[in]	inElementOffset		Specifies the element offset of interest.
					@return		True if the element is contained within me.
				**/
				bool			containsElementAtOffset (const ULWord inElementOffset) const;	//	New in SDK 16.0

				/**
					@param[in]	inRHS	Specifies the NTV2SegmentedXferInfo to compare with me.
					@return		True if not equal.
					@note		The element sizes must match (though this ought to be compensated for in the comparison).
				**/
				bool			operator != (const NTV2SegmentedXferInfo & inRHS) const;	//	New in SDK 16.0

				/**
					@param[in]	inRHS	Specifies the NTV2SegmentedXferInfo to compare with me.
					@return		True if equal.
					@note		The element sizes must match (though this ought to be compensated for in the comparison).
				**/
				inline bool		operator == (const NTV2SegmentedXferInfo & inRHS) const		{return !(*this != inRHS);}	//	New in SDK 16.0
				///@}

				/**
					@name	Changing
				**/
				///@{

				NTV2SegmentedXferInfo &	reset (void);	///< @brief	Resets me to an invalid (all zero) state.

				/**
					@brief		Sets both my segment count and length.
					@param[in]	inNumSegs	My new segment count.
					@param[in]	inSegLength	My new segment length, in elements.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setSegmentInfo (const ULWord inNumSegs, const ULWord inSegLength)
												{return setSegmentCount(inNumSegs).setSegmentLength(inSegLength);}

				/**
					@brief		Sets my segment count.
					@param[in]	inNumSegments	My new segment count.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setSegmentCount (const ULWord inNumSegments)
												{mNumSegments = inNumSegments;  return *this;}

				/**
					@brief		Sets my segment length.
					@param[in]	inNumElements	My new segment length, in elements.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setSegmentLength (const ULWord inNumElements)
												{mElementsPerSegment = inNumElements;  return *this;}

				/**
					@brief		A convenience function that sets both my source offset and pitch.
					@param[in]	inOffset	The new offset, in elements, to the start of the first source segment.
					@param[in]	inPitch		The new number of elements between each source row.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setSourceInfo (const ULWord inOffset, const ULWord inPitch)
												{return setSourceOffset(inOffset).setSourcePitch(inPitch);}

				/**
					@brief		Sets my source offset.
					@param[in]	inOffset	The new offset, in elements, to the start of the first source segment.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setSourceOffset (const ULWord inOffset)
												{mInitialSrcOffset = inOffset;  return *this;}

				/**
					@brief		Sets my source pitch.
					@param[in]	inPitch		The new number of elements between each source row.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setSourcePitch (const ULWord inPitch)
												{mSrcElementsPerRow = inPitch;  return *this;}

				/**
					@brief		Sets my source direction.
					@param[in]	inTopDown	Specify true to traverse the source segments top-to-bottom;  otherwise specify false for bottom-to-top.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setSourceDirection (const bool inTopDown)
												{	mFlags &= (0xFFFFFFFF - BIT(8));
													if (!inTopDown)
														mFlags |= BIT(8);
													return *this;
												}

				/**
					@brief		A convenience function that sets both my destination offset and pitch.
					@param[in]	inOffset	The new offset, in elements, to the start of the first destination segment.
					@param[in]	inPitch		The new number of elements between each destination row.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setDestInfo (const ULWord inOffset, const ULWord inPitch)
												{return setDestOffset(inOffset).setDestPitch(inPitch);}

				/**
					@brief		Sets my destination offset.
					@param[in]	inOffset	The new offset, in elements, to the start of the first destination segment.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setDestOffset (const ULWord inOffset)			{mInitialDstOffset = inOffset;  return *this;}

				/**
					@brief		Sets my destination pitch.
					@param[in]	inPitch		The new number of elements between each destination row.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setDestPitch (const ULWord inPitch)				{mDstElementsPerRow = inPitch;  return *this;}

				/**
					@brief		Sets my destination scan direction.
					@param[in]	inTopDown	Specify true to traverse the destination segments top-to-bottom;  otherwise specify false for bottom-to-top.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setDestDirection (const bool inTopDown)			{mFlags = (0xFFFFFFFF - BIT(9)); if (!inTopDown) mFlags |= BIT(9);  return *this;}

				/**
					@brief		Sets my element length.
					@param[in]	inBytesPerElement	The new element size, in bytes. Must be a power of two (1, 2, 4, or 8).
					@note		This function does not transform any of my offsets, pitches or lengths.
					@return		A reference to me.
				**/
				inline NTV2SegmentedXferInfo &	setElementLength (const ULWord inBytesPerElement)	
												{
													if (inBytesPerElement  &&  inBytesPerElement < 9)
														if (!(inBytesPerElement & (inBytesPerElement - 1)))  // Power of 2?
														{
															ULWord	num(inBytesPerElement),  lengthBits(0);
															while (num >>= 1)
																lengthBits++;
															mFlags = (mFlags & ~3UL) | (lengthBits & 3UL);
														}
													return *this;
												}
				/**
					@brief		Swaps my source and destination offsets and pitches.
					@return		A reference to me.
				**/
				NTV2SegmentedXferInfo &			swapSourceAndDestination (void);	//	New in SDK 16.0
				///@}

			private:
				ULWord	mFlags;					///< @brief	Lowest 2 bits determines element size, kRegMaskFrameOrientation is bit 10
				ULWord	mNumSegments;			///< @brief	Number of segments to transfer (i.e. row count).
				ULWord	mElementsPerSegment;	///< @brief	Size of each segment, in elements.
				ULWord	mInitialSrcOffset;		///< @brief	Initial source offset, in elements.
				ULWord	mInitialDstOffset;		///< @brief	Initial destination offset, in elements.
				ULWord	mSrcElementsPerRow;		///< @brief	Source pitch (i.e. the span, in elements, between the starting elements of adjacent segments on the source).
				ULWord	mDstElementsPerRow;		///< @brief	Destination pitch (i.e. the span, in elements, between the starting elements of adjacent segments on the destination).
		};
		#endif	//	!defined (NTV2_BUILDING_DRIVER)

		/**
			@brief	A generic user-space buffer object that has an address and a length.
					Used most often to share an arbitrary-sized chunk of host memory with the NTV2 kernel driver
					through a CNTV2DriverInterface::NTV2Message call.

					-	For a static or global buffer, simply construct from the variable:
						@code
							static ULWord pFoo [1000];
							{
								NTV2_POINTER foo (pFoo, sizeof (pFoo));
								. . .
							}	//	When foo goes out of scope, it won't try to free pFoo
						@endcode
					-	For stack-based buffers, simply construct from the variable:
						@code
							{
								ULWord pFoo [100];
								NTV2_POINTER foo (pFoo, sizeof (pFoo));
								. . .
							}	//	No need to do anything, as both foo and pFoo are automatically freed when they go out of scope
						@endcode
					-	For a buffer you allocate and free yourself:
						@code
							NTV2_POINTER	foo (new Bar [1], sizeof (Bar));
							. . .
							delete [] (Bar*) foo.GetHostPointer ();		//	You must free the memory yourself
						@endcode
					-	For a 2K-byte buffer that's allocated and freed automatically by the SDK:
						@code
							{
								NTV2_POINTER foo (2048);
								::memset (foo.GetHostPointer(), 0, foo.GetByteCount());
								. . .
							}	//  The memory is freed automatically when foo goes out of scope
						@endcode
			@note	This struct uses a constructor to properly initialize itself.
					Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2_POINTER)
			NTV2_BEGIN_PRIVATE
				ULWord64	fUserSpacePtr;			///< @brief	User space pointer. Do not set directly. Use constructor or Set method.
				ULWord		fByteCount;				///< @brief	The (maximum) size of the buffer pointed to by fUserSpacePtr, in bytes.
													///			Do not set directly. Instead, use the constructor or the Set method.
				ULWord		fFlags;					///< @brief	Reserved for future use
				#if defined (AJAMac)
					ULWord64	fKernelSpacePtr;	///< @brief	Reserved -- Mac driver use only
					ULWord64	fIOMemoryDesc;		///< @brief	Reserved -- Mac driver use only
					ULWord64	fIOMemoryMap;		///< @brief	Reserved -- Mac driver use only
				#else
					ULWord64	fKernelHandle;		///< @brief	Reserved -- driver use only
				#endif
			NTV2_END_PRIVATE

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@name	Construction & Destruction
				**/
				///@{
				/**
					@brief		Constructs me from a client-supplied address and size.
					@param[in]	pInUserPointer	Specifies the user-space virtual memory address. The client is entirely responsible for it.
												Ignored if inByteCount is zero.
					@param[in]	inByteCount		Specifies the byte count. Ignored if pInUserPointer is NULL.
				**/
				explicit		NTV2_POINTER (const void * pInUserPointer, const size_t inByteCount);

				/**
					@brief		Constructs me from a client-specified byte count.
								In this case, I assume full responsibility for automatically freeing the memory I allocate.
					@param[in]	inByteCount		Optionally specifies the size of the allocated buffer, in bytes. Defaults to zero.
												If non-zero, causes Allocate to be called, and if successful, automatically zeroes the buffer.
												If zero (the default), I don't allocate anything, and my host pointer will be NULL.
				**/
								NTV2_POINTER (const size_t inByteCount = 0);

				/**
					@brief		Constructs me from another NTV2_POINTER instance.
					@param[in]	inObj		NTV2_POINTER instance to "deep" copy into me.
				**/
				explicit		NTV2_POINTER (const NTV2_POINTER & inObj);

				/**
					@brief		My destructor. If I'm responsible for the memory, I free it here.
				**/
								~NTV2_POINTER ();
				///@}

				/**
					@name	Inquiry
				**/
				///@{
				/**
					@return		My user-space host virtual address, as seen by the host process.
				**/
				inline void *	GetHostPointer (void) const
				{
					if (sizeof(int*) == 4)
						return reinterpret_cast <void*>((fUserSpacePtr & 0xFFFFFFFF00000000) >> 32);
					else
						return reinterpret_cast <void*>(fUserSpacePtr);
				}

				/**
					@return		My "raw" user-space host virtual address.
					@note		On 32-bit platforms, the true address is found in the most significant 4 bytes of the ULWord64 value.
				**/
				inline ULWord64	GetRawHostPointer (void) const			{return fUserSpacePtr;}

				/**
					@return		My size, in bytes.
				**/
				inline ULWord	GetByteCount (void) const				{return fByteCount;}

				/**
					@return		True if my host storage was allocated by my Allocate function;  otherwise false if my host storage
								address and size was provided by the client application.
				**/
				inline bool		IsAllocatedBySDK (void) const			{return fFlags & NTV2_POINTER_ALLOCATED ? true : false;}

				/**
					@return		True if my host storage was provided by the client application;  otherwise false if it was allocated
								by my Allocate function.
				**/
				inline bool		IsProvidedByClient (void) const			{return fFlags & NTV2_POINTER_ALLOCATED ? false : true;}

				/**
					@return		True if my user-space pointer is NULL, or my size is zero.
				**/
				inline bool		IsNULL (void) const						{return GetHostPointer() == AJA_NULL || GetByteCount() == 0;}

				/**
					@return		True if my host pointer is non-NULL and my byte count is non-zero;  otherwise false.
				**/
				inline			operator bool() const					{return !IsNULL();}

				/**
					@return		My size, in bytes, as a size_t.
				**/
				inline			operator size_t() const					{return size_t(GetByteCount());}	//	New in SDK 16.0

				/**
					@param[in]	inByteOffset	Specifies the offset from the start (or end) of my memory buffer.
												Must be less than my size (see GetByteCount).
					@param[in]	inFromEnd		Specify 'true' to reference the end of my buffer.
												Specify 'false' (the default) to reference the start of my buffer.
					@return		The host address of the given byte. Returns NULL upon failure.
				**/
				void *			GetHostAddress (const ULWord inByteOffset, const bool inFromEnd = false) const;

				/**
					@brief		Searches me for the given scalar value of type T starting at a given index position.
					@param[in]	inValue			Specifies the scalar value to be searched for.
					@param		inOutIndex		On entry, specifies where searching begins.
												On exit, receives the index of the matching value.
												Negative indexes search backward from the end of the buffer, where -1
												is the last T in the buffer (with the highest physical memory address).
					@return		True if successfully found; otherwise false.
				**/
				template<typename T> bool	Find (const T & inValue, int & inOutIndex)	//	New in SDK 16.0
				{
					const bool isAscending(inOutIndex >= 0);
					if (isAscending  &&  inOutIndex >= int(GetByteCount()))
						return false;	//	Past end
					if (!isAscending  &&  (1 - inOutIndex) >= int(GetByteCount()))
						return false;	//	Before start
					const T * pValues(*this);
					const int maxNdx(int(GetByteCount()) / sizeof(T));
					if (isAscending)
					{
						for (int ndx(inOutIndex);  ndx < maxNdx;  ndx++)
							if (pValues[ndx] == inValue)
								{inOutIndex = ndx;  return true;}
					}
					else
					{
						const int minNdx(0 - maxNdx);
						for (int ndx(inOutIndex);  ndx >= minNdx;  ndx++)
							if (pValues[maxNdx + ndx] == inValue)
								{inOutIndex = ndx;  return true;}
					}
					inOutIndex = 0;
					return false;	//	Not found
				}

				/**
					@return		True if the given memory buffer's contents are identical to my own.
					@param[in]	inBuffer		Specifies the memory buffer whose contents are to be compared with mine.
					@param[in]	inByteOffset	Specifies the byte offset to start comparing. Defaults to the first byte.
					@param[in]	inByteCount		Specifies the maximum number of bytes to compare. Defaults to 0xFFFFFFFF (entire buffer).
				**/
				bool			IsContentEqual (const NTV2_POINTER & inBuffer, const ULWord inByteOffset = 0, const ULWord inByteCount = 0xFFFFFFFF) const;

				/**
					@brief		Assuming my contents and the contents of the given buffer comprise ring buffers that periodically get overwritten
								in contiguous variable-length chunks, answers with the contiguous byte range that differs between the two.
					@param[in]	inBuffer			Specifies the memory buffer whose contents are to be compared with mine. Contents are
													assumed to comprise a ring buffer, where data periodically gets overwritten in chunks.
					@param[out]	outByteOffsetFirst	Receives the offset, in bytes, from the start of the buffer, of the first byte of the contiguous
													range that's different.
													Zero indicates the first byte in the buffer.
													If equal to NTV2_POINTER::GetByteCount(), then both buffers are identical.
													If greater than 'outByteOffsetLast', then a wrap condition exists (see Note).
					@param[out]	outByteOffsetLast	Receives the offset, in bytes, from the start of the buffer, of the last byte of the contiguous
													range that's different.
													Zero indicates the first byte in the buffer.
													If equal to NTV2_POINTER::GetByteCount(), then both buffers are identical.
													If less than 'outByteOffsetFirst', a wrap condition exists (see Note).
					@note		If a wrap condition exists -- i.e., the contiguous byte range that differs starts near the end and wraps around
								to near the front -- then 'outByteOffsetFirst' will be greater than 'outByteOffsetLast'.
					@return		True if successful;  otherwise false.
				**/
				bool			GetRingChangedByteRange (const NTV2_POINTER & inBuffer, ULWord & outByteOffsetFirst, ULWord & outByteOffsetLast) const;
				///@}

				/**
					@name	Changing
				**/
				///@{
				/**
					@brief		Allocates (or re-allocates) my user-space storage using the given byte count.
								I assume full responsibility for any memory that I allocate.
					@param[in]	inByteCount		Specifies the number of bytes to allocate.
												Specifying zero is the same as calling Set(NULL, 0).
					@param[in]	inPageAligned	Optionally specifies page alignment. If true, allocates a
												page-aligned block. If false (default), uses operator new.
					@return		True if successful;  otherwise false.
					@note		Any memory that I was referencing prior to this call that I was responsible for will automatically be freed.
				**/
				bool			Allocate (const size_t inByteCount, const bool inPageAligned = false);

				/**
					@brief		Deallocates my user-space storage (if I own it -- i.e. from a prior call to Allocate).
					@return		True if successful;  otherwise false.
				**/
				bool			Deallocate (void);

				/**
					@brief		Fills me with the given scalar value.
					@param[in]	inValue		The scalar value to fill me with.
					@return		True if I'm currently allocated;  otherwise false.
				**/
				template<typename T>	bool Fill (const T & inValue)
				{
					T *	pT	(reinterpret_cast<T*>(GetHostPointer()));
					const size_t loopCount(GetByteCount() / sizeof(T));
					if (pT)
						for (size_t n(0);  n < loopCount;  n++)
							pT[n] = inValue;
					return pT ? true : false;
				}

				/**
					@brief		Fills a portion of me with the given scalar value.
					@param[in]	inValue			The scalar value.
					@param[in]	inXferInfo		Describes the portion of me to be filled.
					@return		True if successful; otherwise false.
					@note		Offsets and lengths are checked. The function will return false for any overflow or underflow.
				**/
				template<typename T> bool	Fill (const T & inValue, const NTV2SegmentedXferInfo & inXferInfo)
				{
					if (!inXferInfo.isValid())
						return false;
					//	Fill a temporary buffer to hold all the segment data...
					NTV2_POINTER	segData(inXferInfo.getElementLength() * inXferInfo.getSegmentCount() * inXferInfo.getSegmentLength());
					if (!segData.Fill(inValue))
						return false;	//	Fill failed

					//	Copy the segment data into me...
					ULWord			srcOffset	(0);
					ULWord			dstOffset	(inXferInfo.getDestOffset() * inXferInfo.getElementLength());
					const ULWord	dstPitch	(inXferInfo.getDestPitch() * inXferInfo.getElementLength());
					const ULWord	bytesPerSeg	(inXferInfo.getSegmentLength() * inXferInfo.getElementLength());
					for (ULWord segNdx(0);  segNdx < inXferInfo.getSegmentCount();  segNdx++)
					{
						const void *	pSrc (segData.GetHostAddress(srcOffset));
						void *			pDst (GetHostAddress(dstOffset));
						if (!pSrc)	return false;
						if (!pDst)	return false;
						if (dstOffset + bytesPerSeg > GetByteCount())
							return false;	//	memcpy will write past end
						::memcpy (pDst,  pSrc,  bytesPerSeg);
						srcOffset += bytesPerSeg;	//	Bump src offset
						dstOffset += dstPitch;		//	Bump dst offset
					}	//	for each segment
					return true;
				}

				/**
					@brief		Assigns me from another NTV2_POINTER instance.
					@param[in]	inRHS		Specifies the NTV2_POINTER instance to assign ("deep" copy) to me.
				**/
				NTV2_POINTER &	operator = (const NTV2_POINTER & inRHS);

				/**
					@brief		Sets (or resets) me from a client-supplied address and size.
					@param[in]	pInUserPointer	Specifies the user-space virtual memory address. The client is entirely responsible for it.
												Ignored if inByteCount is zero.
					@param[in]	inByteCount		Specifies the byte count. Ignored if pInUserPointer is NULL.
					@return		True if both pInUserPointer and inByteCount agree (i.e. if pInUserPointer
								and inByteCount are both zero, or if they're both non-zero);  otherwise false
								if they don't agree (i.e. one is non-zero and the other is zero, or vice-versa).
					@note		Any memory that I was referencing prior to this call that I was responsible for will automatically be freed.
				**/
				bool			Set (const void * pInUserPointer, const size_t inByteCount);

				/**
					@brief		Sets (or resets) me from a client-supplied address and size.
					@param[in]	pInUserPointer	Specifies the user-space virtual memory address. The client is entirely responsible for it.
												Ignored if inByteCount is zero.
					@param[in]	inByteCount		Specifies the byte count. Ignored if pInUserPointer is NULL.
					@param[in]	inValue			Specifies the value to fill the buffer with.
					@return		True if both pInUserPointer and inByteCount agree (i.e. if pInUserPointer and inByteCount
								are both zero, or if they're both non-zero);  otherwise false if they don't agree (i.e. one
								is non-zero and the other is zero, or vice-versa).
					@note		Any memory that I was referencing prior to this call that I was responsible for will automatically be freed.
				**/
				bool			SetAndFill (const void * pInUserPointer, const size_t inByteCount, const UByte inValue);

				/**
					@brief		Replaces my contents from the given memory buffer without resizing me.
					@param[in]	inBuffer	Specifies the memory buffer whose contents are to be copied into my own.
											If this buffer is larger than I am, I am not resized; instead, only
											those bytes that fit in me will be copied.
					@return		True if successful; otherwise false.
				**/
				bool			SetFrom (const NTV2_POINTER & inBuffer);

				/**
					@brief		Replaces my contents from the given memory buffer, resizing me to the new byte count.
					@param[in]	pInSrcBuffer	Specifies the memory buffer whose contents are to be copied into my own.
					@param[in]	inByteCount		Specifies the number of bytes to be copied.
					@return		True if successful; otherwise false.
				**/
				bool			CopyFrom (const void * pInSrcBuffer, const ULWord inByteCount);

				/**
					@brief		Copies bytes from the given memory buffer into me.
					@param[in]	inSrcBuffer			Specifies the source memory buffer to be copied into me.
					@param[in]	inSrcByteOffset		Specifies the offset, in bytes, at which reading will commence in the source buffer.
					@param[in]	inDstByteOffset		Specifies the offset, in bytes, at which writing will commence in me.
					@param[in]	inByteCount			Specifies the total number of bytes to copy.
					@return		True if successful; otherwise false.
					@note		The offsets and byte counts are checked against the existing sizes of the two buffers.
								The function will return false for any overflow.
				**/
				bool			CopyFrom (const NTV2_POINTER & inSrcBuffer, const ULWord inSrcByteOffset, const ULWord inDstByteOffset, const ULWord inByteCount);

				/**
					@brief		Copies data segments from a given buffer into me.
					@param[in]	inSrcBuffer			Specifies the source memory buffer to be copied into me.
					@param[in]	inXferInfo			The segmented transfer info.
					@return		True if successful; otherwise false.
					@note		Offsets and lengths are checked. The function will return false for any overflow or underflow.
				**/
				bool			CopyFrom (const NTV2_POINTER & inSrcBuffer, const NTV2SegmentedXferInfo & inXferInfo);

				/**
					@brief		Swaps my underlying buffer with another's.
					@param[in]	inBuffer	Specifies the NTV2_POINTER I'll swap buffers with.
					@return		True if successful; otherwise false.
					@note		The buffers must have identical sizes, and must have equal ownership attributes.
					@note		AJA recommends not using this function to swap NTV2_POINTERs that were allocated in different
								executable modules (e.g., on Windows, an NTV2_POINTER that was allocated in a DLL with another
								that was allocated in an EXE).
				**/
				bool			SwapWith (NTV2_POINTER & inBuffer);

				/**
					@brief		Byte-swaps my contents 64-bits at a time.
					@return		True if successful;  otherwise false.
					@note		If my size (in bytes) is not evenly divisible by 8, the very last byte(s) won't get swapped.
				**/
				bool			ByteSwap64 (void);	//	New in SDK 16.0

				/**
					@brief		Byte-swaps my contents 32-bits at a time.
					@return		True if successful;  otherwise false.
					@note		If my size (in bytes) is not evenly divisible by 4, the very last byte(s) won't get swapped.
				**/
				bool			ByteSwap32 (void);	//	New in SDK 16.0

				/**
					@brief		Byte-swaps my contents 16-bits at a time.
					@return		True if successful;  otherwise false.
					@note		If my size (in bytes) is not evenly divisible by 2, the very last byte won't get swapped.
				**/
				bool			ByteSwap16 (void);	//	New in SDK 16.0
				///@}

				/**
					@name	Debugging/Printing
				**/
				///@{
				/**
					@brief	Prints a human-readable representation of me into the given output stream.
					@param	inOutStream		The output stream to receive my human-readable representation.
					@return	A reference to the given output stream.
				**/
				std::ostream &	Print (std::ostream & inOutStream) const;

				/**
					@param	inDumpMaxBytes		If non-zero, includes a hex dump of my contents up to the number of specified bytes (64 maximum).
					@return	A string containing a human-readable representation of me.
				**/
				std::string		AsString (UWord inDumpMaxBytes = 0) const;

				/**
					@brief	Dumps me in hex/octal/decimal, with/without Ascii, to the given output stream.
					@param	inOutputStream		Output stream that will receive the dump. Defaults to std::cout.
					@param	inStartByteOffset	The starting offset, in bytes, where the dump will start.
					@param	inByteCount			The number of bytes to be dumped. If zero, all bytes will be dumped.
					@param	inRadix				Specifies the radix of the dumped memory values.
												16=hex, 10=decimal, 8=octal, 2=binary -- all others disallowed.
					@param	inBytesPerGroup		Number of bytes to dump per contiguous group of numbers. Defaults to 4.
					@param	inGroupsPerLine		Number of contiguous groups of numbers to dump per output line.
												If zero, no grouping is done, and address & ASCII display is suppressed.
												Defaults to 8.
					@param	inAddressRadix		Specifies the radix of the address column.
												0=omit, 2=binary, 8=octal, 10=decimal, 16=hex -- all others disallowed.
												Defaults to 0.
					@param	inShowAscii			If True, show ASCII characters; otherwise no ASCII characters.
												Overridden to false if inGroupsPerLine is zero. Defaults to false.
					@param	inAddrOffset		Specifies a value to be added to the addresses that appear in the dump.
												Ignored if inGroupsPerLine is zero.
					@return	A non-constant reference to the output stream that received the dump.
				**/
				std::ostream &	Dump (	std::ostream &	inOutputStream		= std::cout,
										const size_t	inStartByteOffset	= 0,
										const size_t	inByteCount			= 0,
										const size_t	inRadix				= 16,
										const size_t	inBytesPerGroup		= 4,
										const size_t	inGroupsPerLine		= 8,
										const size_t	inAddressRadix		= 0,
										const bool		inShowAscii			= false,
										const size_t	inAddrOffset		= 0) const;

				/**
					@brief	Dumps me in hex/octal/decimal, with/without Ascii, into the given string.
					@param	inOutputString		String that will receive the dump. Appends the dump to the end of
												whatever is already in the string.
					@param	inStartByteOffset	The starting offset, in bytes, where the dump will start.
					@param	inByteCount			The number of bytes to be dumped. If zero, all bytes will be dumped.
					@param	inRadix				Specifies the radix of the dumped memory values.
												16=hex, 10=decimal, 8=octal, 2=binary -- all others disallowed.
					@param	inBytesPerGroup		Number of bytes to dump per contiguous group of numbers. Defaults to 4.
					@param	inGroupsPerLine		Number of contiguous groups of numbers to dump per output line.
												If zero, no grouping is done, and address & ASCII display is suppressed.
												Defaults to 8.
					@param	inAddressRadix		Specifies the radix of the address column.
												0=omit, 2=binary, 8=octal, 10=decimal, 16=hex -- all others disallowed.
												Defaults to 0.
					@param	inShowAscii			If True, show ASCII characters; otherwise no ASCII characters.
												Overridden to false if inGroupsPerLine is zero. Defaults to false.
					@param	inAddrOffset		Specifies a value to be added to the addresses that appear in the dump.
												Ignored if inGroupsPerLine is zero.
					@return	A reference to the output string that received the dump.
				**/
				std::string &	Dump (	std::string &	inOutputString,
										const size_t	inStartByteOffset	= 0,
										const size_t	inByteCount			= 0,
										const size_t	inRadix				= 16,
										const size_t	inBytesPerGroup		= 4,
										const size_t	inGroupsPerLine		= 8,
										const size_t	inAddressRadix		= 0,
										const bool		inShowAscii			= false,
										const size_t	inAddrOffset		= 0) const;
				///@}

				/**
					@name	Data Access
				**/
				///@{
				/**
					@return		My host address casted to a const T pointer.
				**/
				template<typename T>	operator const T*() const		{return reinterpret_cast<const T*>(GetHostPointer());}	//	New in SDK 16.0

				/**
					@return		My host address casted to a non-const T pointer.
				**/
				template<typename T>	operator T*() const				{return reinterpret_cast<T*>(GetHostPointer());}	//	New in SDK 16.0

				/**
					@brief		Resets an NTV2_POINTER instance to reference a contiguous segment of my memory buffer.
					@param[out]	outPtr			The NTV2_POINTER to be reset to my sub-segment.
					@param[in]	inByteOffset	Specifies the offset, in bytes, where the segment starts.
					@param[in]	inByteCount		Specifies the segment length, in bytes.
					@return		The specified NTV2_POINTER.  It will be set to null/empty upon failure.
					@note		The offset and byte count are both checked against my buffer size.
				**/
				NTV2_POINTER &			Segment (NTV2_POINTER & outPtr, const ULWord inByteOffset, const ULWord inByteCount) const;

				/**
					@return		A copy of the value at the given zero-based index position.
					@param[in]	inIndex		Specifies the zero-based index position (e.g. 0 is first value at start of my memory).
											If negative, indexes from the end of my memory (e.g. -1 is last value).
					@warning	Bad index values will result in access violation exceptions.
				**/
				inline	uint8_t			U8 (const int inIndex) const	{const uint8_t* pVal(*this);	return pVal[inIndex < 0 ? int(GetByteCount()) + inIndex : inIndex];}	//	New in SDK 16.0

				/**
					@return		A writeable (non-const) reference to the value at the given zero-based index position.
					@param[in]	inIndex		Specifies the zero-based index position (e.g. 0 is first value at start of my memory).
											If negative, indexes from the end of my memory (e.g. -1 is last value).
					@warning	Bad index values will result in access violation exceptions.
				**/
				inline	uint8_t &		U8 (const int inIndex)			{uint8_t* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()) + inIndex : inIndex];}	//	New in SDK 16.0

				inline	int8_t			I8 (const int inIndex) const	{const int8_t* pVal(*this);		return pVal[inIndex < 0 ? int(GetByteCount()) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	int8_t &		I8 (const int inIndex)			{int8_t* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	uint16_t		U16 (const int inIndex) const	{const uint16_t* pVal(*this);	return pVal[inIndex < 0 ? int(GetByteCount()/2) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	uint16_t &		U16 (const int inIndex)			{uint16_t* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()/2) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	int16_t			I16 (const int inIndex) const	{const int16_t* pVal(*this);	return pVal[inIndex < 0 ? int(GetByteCount()/2) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	int16_t &		I16 (const int inIndex)			{int16_t* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()/2) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	uint32_t		U32 (const int inIndex) const	{const uint32_t* pVal(*this);	return pVal[inIndex < 0 ? int(GetByteCount()/4) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	uint32_t &		U32 (const int inIndex)			{uint32_t* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()/4) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	int32_t			I32 (const int inIndex) const	{const int32_t* pVal(*this);	return pVal[inIndex < 0 ? int(GetByteCount()/4) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	int32_t &		I32 (const int inIndex)			{int32_t* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()/4) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	uint64_t		U64 (const int inIndex) const	{const uint64_t* pVal(*this);	return pVal[inIndex < 0 ? int(GetByteCount()/8) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	uint64_t &		U64 (const int inIndex)			{uint64_t* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()/8) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	int64_t			I64 (const int inIndex) const	{const int64_t* pVal(*this);	return pVal[inIndex < 0 ? int(GetByteCount()/8) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	int64_t &		I64 (const int inIndex)			{int64_t* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()/8) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	float			FL (const int inIndex) const	{const float* pVal(*this);		return pVal[inIndex < 0 ? int(GetByteCount()/sizeof(float)) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	float &			FL (const int inIndex)			{float* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()/sizeof(float)) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	double			DBL (const int inIndex) const	{const double* pVal(*this);		return pVal[inIndex < 0 ? int(GetByteCount()/sizeof(double)) + inIndex : inIndex];}	//	New in SDK 16.0
				inline	double &		DBL (const int inIndex)			{double* pVal(*this);			return pVal[inIndex < 0 ? int(GetByteCount()/sizeof(double)) + inIndex : inIndex];}	//	New in SDK 16.0
				///@}

				/**
					@name	Vector Conversion
				**/
				///@{
				/**
					@brief		Answers with my contents as a vector of unsigned 16-bit values.
					@param[out]	outU64s			Receives my contents as a vector of unsigned 64-bit values.
					@param[in]	inU64Offset		The starting offset, in 64-bit words, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 64-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 64-bit values may be less than this, depending on my size.
												Defaults to 16.
					@param[in]	inByteSwap		Specifies if the resulting values will be byte-swapped or not.
												Specify 'true' to byte-swap;  specify 'false' to return the normal, unswapped values.
												Defaults to 'false'.
					@return						True if successful;  otherwise false.
					@note		If my length is not evenly divisible by 8, my last bytes won't appear in the resulting vector. 
				**/
				bool						GetU64s (ULWord64Sequence & outU64s, const size_t inU64Offset = 0, const size_t inMaxSize = 16, const bool inByteSwap = false) const;

				/**
					@return		My contents as a vector of unsigned 64-bit values.
					@param[in]	inU64Offset		The starting offset, in 64-bit words, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 64-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 64-bit values may be less than this, depending on my size.
												Defaults to 16.
					@param[in]	inByteSwap		Specifies if the resulting values will be byte-swapped or not.
												Specify 'true' to byte-swap;  specify 'false' to return the normal, unswapped values.
												Defaults to 'false'.
					@note		If my length is not evenly divisible by 8, my last bytes won't appear in the resulting vector. 
				**/
				inline ULWord64Sequence		GetU64s (const size_t inU64Offset = 0, const size_t inMaxSize = 16, const bool inByteSwap = false) const	{ULWord64Sequence result; GetU64s(result, inU64Offset, inMaxSize, inByteSwap); return result;}

				/**
					@brief		Answers with my contents as a vector of unsigned 32-bit values.
					@param[out]	outU32s			Receives my contents as a vector of unsigned 32-bit values.
					@param[in]	inU32Offset		The starting offset, in 32-bit words, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 32-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 32-bit values may be less than this, depending on my size.
												Defaults to 32.
					@param[in]	inByteSwap		Specifies if the resulting values will be byte-swapped or not.
												Specify 'true' to byte-swap;  specify 'false' to return the normal, unswapped values.
												Defaults to 'false'.
					@return						True if successful;  otherwise false.
					@note		If my length is not evenly divisible by 4, my last bytes won't appear in the resulting vector. 
				**/
				bool						GetU32s (ULWordSequence & outU32s, const size_t inU32Offset = 0, const size_t inMaxSize = 32, const bool inByteSwap = false) const;

				/**
					@return		My contents as a vector of unsigned 32-bit values.
					@param[in]	inU32Offset		The starting offset, in 32-bit words, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 32-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 32-bit values may be less than this, depending on my size.
												Defaults to 32.
					@param[in]	inByteSwap		Specifies if the resulting values will be byte-swapped or not.
												Specify 'true' to byte-swap;  specify 'false' to return the normal, unswapped values.
												Defaults to 'false'.
					@note		If my length is not evenly divisible by 4, my last bytes won't appear in the resulting vector. 
				**/
				inline ULWordSequence		GetU32s (const size_t inU32Offset = 0, const size_t inMaxSize = 32, const bool inByteSwap = false) const	{ULWordSequence result; GetU32s(result, inU32Offset, inMaxSize, inByteSwap); return result;}

				/**
					@brief		Answers with my contents as a vector of unsigned 16-bit values.
					@param[out]	outU16s			Receives my contents as a vector of unsigned 16-bit values.
					@param[in]	inU16Offset		The starting offset, in 16-bit words, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 16-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 16-bit values may be less than this, depending on my size.
												Defaults to 64.
					@param[in]	inByteSwap		Specifies if the resulting values will be byte-swapped or not.
												Specify 'true' to byte-swap;  specify 'false' to return the normal, unswapped values.
												Defaults to 'false'.
					@return						True if successful;  otherwise false.
					@note		If my length is not evenly divisible by 2, my last byte won't appear in the resulting vector. 
				**/
				bool						GetU16s (UWordSequence & outU16s, const size_t inU16Offset = 0, const size_t inMaxSize = 64, const bool inByteSwap = false) const;

				/**
					@return		My contents as a vector of unsigned 16-bit values.
					@param[in]	inU16Offset		The starting offset, in 16-bit words, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 16-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 16-bit values may be less than this, depending on my size.
												Defaults to 64.
					@param[in]	inByteSwap		Specifies if the resulting values will be byte-swapped or not.
												Specify 'true' to byte-swap;  specify 'false' to return the normal, unswapped values.
												Defaults to 'false'.
					@note		If my length is not evenly divisible by 2, my last byte won't appear in the resulting vector. 
				**/
				inline UWordSequence		GetU16s (const size_t inU16Offset = 0, const size_t inMaxSize = 64, const bool inByteSwap = false) const	{UWordSequence result; GetU16s(result, inU16Offset, inMaxSize, inByteSwap); return result;}

				/**
					@brief		Answers with my contents as a vector of unsigned 8-bit values.
					@param[out]	outU8s			Receives my contents as a vector of unsigned 8-bit values.
					@param[in]	inU8Offset		The starting offset, in bytes, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 8-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 8-bit values may be less than this, depending on my size.
												Defaults to 128.
					@return						True if successful;  otherwise false.
				**/
				bool						GetU8s (UByteSequence & outU8s, const size_t inU8Offset = 0, const size_t inMaxSize = 128) const;

				/**
					@return		My contents as a vector of unsigned 8-bit values.
					@param[in]	inU8Offset		The starting offset, in bytes, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 8-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 8-bit values may be less than this, depending on my size.
												Defaults to 128.
				**/
				inline UByteSequence		GetU8s (const size_t inU8Offset = 0, const size_t inMaxSize = 128) const	{UByteSequence result; GetU8s(result, inU8Offset, inMaxSize); return result;}

				/**
					@brief		Answers with my contents as a character string.
					@param[out]	outString		Receives the character string copied verbatim from my contents.
					@param[in]	inU8Offset		The starting offset, in bytes, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 8-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 8-bit values may be less than this, depending on my size.
												Defaults to 128.
					@return						True if successful;  otherwise false.
					@note		This function blindly copies my contents into the outgoing string, without checking for validity.
				**/
				bool						GetString (std::string & outString, const size_t inU8Offset = 0, const size_t inMaxSize = 128) const;

				/**
					@return		My contents as a character string.
					@param[in]	inU8Offset		The starting offset, in bytes, where copying will commence.
					@param[in]	inMaxSize		Specifies the maximum number of 8-bit values to be returned.
												Use zero for unlimited.
												The actual number of returned 8-bit values may be less than this, depending on my size.
												Defaults to 128.
					@note		This function blindly copies my contents into the outgoing string, without checking for validity.
				**/
				inline std::string			GetString (const size_t inU8Offset = 0, const size_t inMaxSize = 128) const	{std::string result; GetString(result, inU8Offset, inMaxSize); return result;}

				/**
					@brief		Copies a vector of unsigned 64-bit values into me.
					@param[in]	inU64s			The vector of unsigned 64-bit values to be copied into me.
					@param[in]	inU64Offset		Specifies my starting offset (of 64-bit values) at which the vector values will be written.
												Defaults to zero.
					@param[in]	inByteSwap		Specifies if the 64-bit values will be byte-swapped or not before being written into me.
												Specify 'true' to byte-swap;  otherwise specify 'false'. Defaults to 'false'.
					@return						True if successful;  otherwise false.
				**/
				bool						PutU64s (const ULWord64Sequence & inU64s, const size_t inU64Offset = 0, const bool inByteSwap = false);

				/**
					@brief		Copies a vector of unsigned 32-bit values into me.
					@param[in]	inU32s			The vector of unsigned 32-bit values to be copied into me.
					@param[in]	inU32Offset		Specifies my starting offset (of 32-bit values) at which the vector values will be written.
												Defaults to zero.
					@param[in]	inByteSwap		Specifies if the 32-bit values will be byte-swapped or not before being written into me.
												Specify 'true' to byte-swap;  otherwise specify 'false'. Defaults to 'false'.
					@return						True if successful;  otherwise false.
				**/
				bool						PutU32s (const ULWordSequence & inU32s, const size_t inU32Offset = 0, const bool inByteSwap = false);

				/**
					@brief		Copies a vector of unsigned 16-bit values into me.
					@param[in]	inU16s			The vector of unsigned 16-bit values to be copied into me.
					@param[in]	inU16Offset		Specifies my starting offset (of 16-bit values) at which the vector values will be written.
												Defaults to zero.
					@param[in]	inByteSwap		Specifies if the 16-bit values will be byte-swapped or not before being written into me.
												Specify 'true' to byte-swap;  otherwise specify 'false'. Defaults to 'false'.
					@return						True if successful;  otherwise false.
				**/
				bool						PutU16s (const UWordSequence & inU16s, const size_t inU16Offset = 0, const bool inByteSwap = false);

				/**
					@brief		Copies a vector of unsigned 8-bit values into me.
					@param[in]	inU8s			The vector of unsigned 8-bit values to be copied into me.
					@param[in]	inU8Offset		Specifies my starting byte offset at which the vector values will be written.
												Defaults to zero.
					@return						True if successful;  otherwise false.
				**/
				bool						PutU8s (const UByteSequence & inU8s, const size_t inU8Offset = 0);
				///@}

				/**
					@name	Default Page Size
				**/
				///@{
				/**
					@return		Default page size, in bytes.
				**/
				static size_t				DefaultPageSize (void);

				/**
					@brief		Changes the default page size for use in future page-aligned allocations.
					@param[in]	inNewSize		The new page size value, in bytes. Must be a power of 2.
					@return		True if successful;  otherwise false.
				**/
				static bool					SetDefaultPageSize (const size_t inNewSize);
				///@}
			#endif	//	user-space clients only
		NTV2_STRUCT_END (NTV2_POINTER)


		/**
			@brief	This struct replaces the old RP188_STRUCT.
		**/
		NTV2_STRUCT_BEGIN (NTV2_RP188)
			ULWord	fDBB;
			ULWord	fLo;	///< @brief	| BG 4  | Secs10 |  BG 3  | Secs 1 |  BG 2  | Frms10 |  BG 1  | Frms 1 |
			ULWord	fHi;	///< @brief	| BG 8  | Hrs 10 |  BG 7  | Hrs  1 |  BG 6  | Mins10 |  BG 5  | Mins 1 |

			#if defined (NTV2_BUILDING_DRIVER)
				#define NTV2_RP188_from_RP188_STRUCT(_n_,_r_)		{	(_n_).fDBB = (_r_).DBB;						\
																		(_n_).fLo = (_r_).Low;						\
																		(_n_).fHi = (_r_).High;		}

				#define NTV2_RP188P_from_RP188_STRUCT(_np_,_r_)		{	(_np_)->fDBB = (_r_).DBB;					\
																		(_np_)->fLo = (_r_).Low;					\
																		(_np_)->fHi = (_r_).High;	}

				#define RP188_STRUCT_from_NTV2_RP188(_r_,_n_)		{	(_r_).DBB = (_n_).fDBB;						\
																		(_r_).Low = (_n_).fLo;						\
																		(_r_).High = (_n_).fHi;		}

				#define RP188_STRUCT_from_NTV2_RP188P(_r_,_np_)		{	(_r_).DBB = (_np_)->fDBB;					\
																		(_r_).Low = (_np_)->fLo;					\
																		(_r_).High = (_np_)->fHi;	}

				#define	NTV2_RP188_IS_VALID(_n_)						((_n_).fDBB != 0xFFFFFFFF || (_n_).fLo != 0xFFFFFFFF || (_n_).fHi != 0xFFFFFFFF)
			#else
				/**
					@brief	Constructs an NTV2_RP188 from each of its DBB, low and high ULWord components.
					@param[in]	inDBB	Specifies the DBB field.
					@param[in]	inLow	Specifies the "low" field, which contains seconds and frames.
					@param[in]	inHigh	Specifies the "high" field, which contains hours and minutes.
					@note	If no parameters are specified, a default "invalid" structure is created.
				**/
				inline explicit		NTV2_RP188 (const ULWord inDBB = 0xFFFFFFFF, const ULWord inLow = 0xFFFFFFFF, const ULWord inHigh = 0xFFFFFFFF)	:	fDBB (inDBB), fLo (inLow), fHi (inHigh)	{}

				/**
					@brief	Constructs an NTV2_RP188 from the given RP188_STRUCT.
					@param[in]	inOldRP188	Specifies the RP188_STRUCT to copy.
				**/
				inline explicit		NTV2_RP188 (const RP188_STRUCT & inOldRP188)	:	fDBB (inOldRP188.DBB), fLo (inOldRP188.Low), fHi (inOldRP188.High)	{}

				/**
					@brief	Answers true if I'm valid, or false if I'm not valid.
				**/
				inline bool			IsValid (void) const							{return fDBB != 0xFFFFFFFF || fLo != 0xFFFFFFFF || fHi != 0xFFFFFFFF;}

				/**
					@brief		Assigns a given RP188_STRUCT to me.
					@param[in]	inRHS	The RP188_STRUCT to assign to me.
					@return		A non-constant reference to myself.
				**/
				inline NTV2_RP188 &	operator = (const RP188_STRUCT & inRHS)			{fDBB = inRHS.DBB;  fLo = inRHS.Low;  fHi = inRHS.High;  return *this;}

				/**
					@param[in]	inRHS	The RP188_STRUCT to compare with me.
					@return		True if the right-hand-size argument is not equal to me.
				**/
				inline bool			operator != (const RP188_STRUCT & inRHS) const	{return fDBB != inRHS.DBB  ||  fLo != inRHS.Low  ||  fHi != inRHS.High;}

				/**
					@brief	Sets my fields from the given DBB, low and high ULWord components.
					@param[in]	inDBB	Specifies the DBB field.
					@param[in]	inLow	Specifies the "low" field, which contains seconds and frames.
					@param[in]	inHigh	Specifies the "high" field, which contains hours and minutes.
				**/
				inline void			Set (const ULWord inDBB = 0xFFFFFFFF, const ULWord inLow = 0xFFFFFFFF, const ULWord inHigh = 0xFFFFFFFF)	{fDBB = inDBB;  fLo = inLow;  fHi = inHigh;}

				/**
					@brief		Converts me into an RP188_STRUCT.
					@return		An equivalent RP188_STRUCT.
				**/
				inline operator		RP188_STRUCT () const							{RP188_STRUCT result;  result.DBB = fDBB;  result.Low = fLo;  result.High = fHi;  return result;}

				/**
					@return		True if I'm valid.
				**/
				inline operator		bool () const									{return IsValid();}
			#endif	//	user-space clients only
		NTV2_STRUCT_END (NTV2_RP188)

		#if !defined (NTV2_BUILDING_DRIVER)
			typedef std::vector <NTV2_RP188>			NTV2TimeCodeList;			///< @brief	An ordered sequence of zero or more NTV2_RP188 structures.
																					///			An NTV2TCIndex enum value can be used as an index into the list
																					///			(the list size should equal NTV2_MAX_NUM_TIMECODE_INDEXES).
			typedef NTV2TimeCodeList::const_iterator	NTV2TimeCodeListConstIter;	///< @brief	A handy const interator for iterating over an NTV2TimeCodeList.
			//typedef NTV2TimeCodeList::iterator		NTV2TimeCodeListIter;
			typedef std::map <NTV2TCIndex, NTV2_RP188>	NTV2TimeCodes;				///< @brief	A mapping of NTV2TCIndex enum values to NTV2_RP188 structures.
			typedef NTV2TimeCodes::const_iterator		NTV2TimeCodesConstIter;		///< @brief	A handy const interator for iterating over NTV2TCIndex/NTV2TimeCodeList pairs.

			typedef std::set <NTV2TCIndex>				NTV2TCIndexes;				///< @brief	A set of distinct NTV2TCIndex values.
			typedef NTV2TCIndexes::const_iterator		NTV2TCIndexesConstIter;		///< @brief	A handy const interator for iterating over an NTV2TCIndexes set.

			/**
				@brief	Appends the given NTV2_RP188 struct to the specified NTV2TimeCodeList.
				@param		inOutList	Specifies the NTV2TimeCodeList to be appended to.
				@param[in]	inRP188		Specifies the NTV2_RP188 to append to the list.
				@return	A non-constant reference to the specified NTV2TimeCodeList.
			**/
			NTV2TimeCodeList & operator << (NTV2TimeCodeList & inOutList, const NTV2_RP188 & inRP188);
		#endif	//	!defined (NTV2_BUILDING_DRIVER)


		/**
			@brief	For devices that support it (see ::NTV2DeviceCanDoSDIErrorChecks ), this struct reports SDI input error status information.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2SDIInputStatus)
			UWord			mCRCTallyA;				///< @brief	The number of lines having a CRC error was detected in the "B" stream of the SDI link
													///			since this tally was last reset.
			UWord			mCRCTallyB;				///< @brief	The number of lines having a CRC error was detected in the "A" stream of the SDI link
													///			since this tally was last reset.
			ULWord			mUnlockTally;			///< @brief	The number of times "RX Locked" went inactive since this tally was last reset.
			//ULWord64		mFrameTally;			///< @brief	The number of frames that have been detected on the SDI input since this tally was last reset.
			ULWord64		mFrameRefClockCount;	///< @brief	This count is incremented on each 148.xx reference clock, and latched at EAV for each frame.
			ULWord64		mGlobalClockCount;		///< @brief	This count is incremented on each 148.xx reference clock.
			bool			mFrameTRSError;			///< @brief	If true, SAV/EAV was missing, or the SDI framer had to realign, or "RX Locked" went
													///			inactive. This is updated once per frame (unless vertical timing is absent on the input).
			bool			mLocked;				///< @brief	If true, a valid SDI transport was detected in the received data stream.
													///			If false, at least 15 consecutive lines in the received stream had TRS errors.
			bool			mVPIDValidA;			///< @brief	If true, at least one valid SMPTE 352 packet was received over the last four VBI periods.
			bool			mVPIDValidB;			///< @brief	If true, at least one valid SMPTE 352 packet was received over the last four VBI periods.
			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs a default NTV2SDIInputStatus.
				**/
				explicit			NTV2SDIInputStatus ();

				/**
					@brief	Constructs a default NTV2SDIInputStatus.
				**/
				void				Clear (void);

				/**
					@brief	Prints a human-readable representation of me into the given output stream.
					@param	inOutStream		The output stream to receive my human-readable representation.
					@return	A reference to the given output stream.
				**/
				std::ostream &		Print (std::ostream & inOutStream) const;
			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (NTV2SDIInputStatus)


		/**
			@brief	Reports HDMI output status information.
			@note	This struct uses a constructor to properly initialize itself.
					Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2HDMIOutputStatus)
			bool					mEnabled;				///< @brief The output is enabled
			bool					mPixel420;				///< @brief The output pixels are 420 format
			NTV2HDMIColorSpace		mColorSpace;			///< @brief The output color space (RGB or YCbCr)
			NTV2HDMIRange			mRGBRange;				///< @brief The RGB output range (SMPTE or full)
			NTV2HDMIProtocol		mProtocol;				///< @brief The output protocol (HDMI vs DVI)
			NTV2Standard			mVideoStandard;			///< @brief The output standard
			NTV2FrameRate			mVideoRate;				///< @brief The output frame rate
			NTV2HDMIBitDepth		mVideoBitDepth;			///< @brief The output bit depth
			NTV2AudioFormat			mAudioFormat;			///< @brief The output audio format
			NTV2AudioRate			mAudioRate;				///< @brief The output audio rate
			NTV2HDMIAudioChannels	mAudioChannels;			///< @brief Number of output audio channels
			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs a default NTV2HDMIOutputStatus.
				**/
				explicit			NTV2HDMIOutputStatus ()								{Clear();}

				/**
					@brief	Constructs a NTV2HDMIOutputStatus from a register data value.
					@param[in]	inData		The status register value.
				**/
				explicit			NTV2HDMIOutputStatus (const ULWord inData)			{Clear();  SetFromRegValue(inData);}

				void				Clear (void);	///< @brief Resets me to an invalid state.

				/**
					@brief	Sets my fields from the given status register value.
					@param[in]	inData		The status register value.
					@return	True if successful;  otherwise false.
				**/
				bool				SetFromRegValue (const ULWord inData);

				/**
					@brief	Prints a human-readable representation of me into the given output stream.
					@param	inOutStream		The output stream to receive my human-readable representation.
					@return	A reference to the given output stream.
				**/
				std::ostream &		Print (std::ostream & inOutStream) const;
			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (NTV2HMDIOutputStatus)


        /**
			@brief	All new NTV2 structs start with this common header.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2_HEADER)
			NTV2_BEGIN_PRIVATE
				ULWord		fHeaderTag;			///< @brief	A special FourCC to identify a structure header & detect endianness, set when created
				ULWord		fType;				///< @brief	A special FourCC to identify a structure type ('stat', 'xfer', 'task', etc.), set when created
				ULWord		fHeaderVersion;		///< @brief	The version of this header structure, set when created, originally zero
				ULWord		fVersion;			///< @brief	The version of the structure that follows this header, set when created, originally zero
				ULWord		fSizeInBytes;		///< @brief	The total size of the struct, in bytes, including header, body and trailer, set when created
				ULWord		fPointerSize;		///< @brief	The size, in bytes, of a pointer on the host, set when created
				ULWord		fOperation;			///< @brief	An operation to perform -- currently unused -- reserved for future use -- set when created
				ULWord		fResultStatus;		///< @brief	The result status of the operation (zero if success or non-zero failure code), cleared when created, set by driver
			NTV2_END_PRIVATE

				#if !defined (NTV2_BUILDING_DRIVER)
					/**
						@brief		Constructs a default NTV2_HEADER having the proper tag, version, and the given type and size.
						@param[in]	inStructureType		Specifies the structure type.
						@param[in]	inSizeInBytes		Specifies the total size of the structure.
					**/
					explicit		NTV2_HEADER (const ULWord inStructureType, const ULWord inSizeInBytes);

					/**
						@brief		Returns my total size, in bytes, including header, body, and trailer.
						@return		My size, in bytes.
					**/
					inline ULWord	GetSizeInBytes (void) const			{return fSizeInBytes;}

					/**
						@brief	Prints a human-readable representation of me into the given output stream.
						@param	inOutStream		The output stream to receive my human-readable representation.
						@return	A reference to the given output stream.
					**/
					std::ostream &	Print (std::ostream & inOutStream) const;

					/**
						@return		True if my tag and type fields are valid;  otherwise false.
					**/
					inline bool		IsValid (void) const				{return NTV2_IS_VALID_HEADER_TAG (fHeaderTag) && NTV2_IS_VALID_STRUCT_TYPE (fType);}
				#endif	//	user-space clients only
		NTV2_STRUCT_END (NTV2_HEADER)


		/**
			@brief	All new NTV2 structs end with this common trailer.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2_TRAILER)
				ULWord		fTrailerVersion;	///< @brief	Trailer version, set when created. Before SDK 15.1: always zero;  15.1 or later: set to calling SDK version (packed).
				ULWord		fTrailerTag;		///< @brief	A special FourCC to identify the tail end of an NTV2 structure

				#if !defined (NTV2_BUILDING_DRIVER)
					explicit	NTV2_TRAILER ();	///< @brief	Constructs a default NTV2_TRAILER having the proper version and tag.

					/**
						@return		True if my tag is valid;  otherwise false.
					**/
					inline bool		IsValid (void) const	{return NTV2_IS_VALID_TRAILER_TAG (fTrailerTag);}
				#endif	//	user-space clients only
		NTV2_STRUCT_END (NTV2_TRAILER)


		/**
			@brief	This struct is used to augment the default full-frame AutoCirculate DMA transfer to accommodate multiple discontiguous
					"segments" of a video frame. The \ref ntv2fieldburn demo app shows how this can be used. Other examples:
					-	An in-host-memory frame has extra "padding" bytes at the end of each row.
						In this case, set NTV2SegmentedDMAInfo::acNumActiveBytesPerRow to the number of active bytes per row,
						NTV2SegmentedDMAInfo::acNumSegments to the number of rows, NTV2SegmentedDMAInfo::acSegmentHostPitch
						to the number of bytes from the beginning of one row to the next. In this example,
						NTV2SegmentedDMAInfo::acSegmentDevicePitch would equal NTV2SegmentedDMAInfo::acNumActiveBytesPerRow
						(i.e. the frame is packed in device memory).
					-	To DMA a sub-section of a frame, set NTV2SegmentedDMAInfo::acNumActiveBytesPerRow to the number of bytes
						that comprise one row of the subsection, then NTV2SegmentedDMAInfo::acNumSegments to the number of rows
						in the subsection, NTV2SegmentedDMAInfo::acSegmentHostPitch to the bytes-per-row of the entire frame in host
						memory, and NTV2SegmentedDMAInfo::acSegmentDevicePitch to the bytes-per-row of the entire frame in device memory.
			@note	IMPORTANT:  For segmented DMAs, the ::AUTOCIRCULATE_TRANSFER::acVideoBuffer.fByteCount field holds the segment
					byte count (i.e., the number of bytes to transfer per segment.

			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
			@todo	Create a new field in the ::NTV2SegmentedDMAInfo structure to eliminate the necessity of setting
					::AUTOCIRCULATE_TRANSFER::acVideoBuffer.fByteCount to store the segmented transfer's bytes-per-segment value.

			@note	Setting NTV2SegmentedDMAInfo::acNumSegments to 0 or 1 defaults to normal non-segmented DMA behavior
					(i.e. DMA one complete, packed frame).
		**/
		NTV2_STRUCT_BEGIN (NTV2SegmentedDMAInfo)
				ULWord		acNumSegments;				///< @brief	Number of segments of size ::AUTOCIRCULATE_TRANSFER::acVideoBuffer.fByteCount
														///			to transfer. Zero or 1 means normal (unsegmented) transfer.
				ULWord		acNumActiveBytesPerRow;		///< @brief	Number of active bytes in a row of video.
				ULWord		acSegmentHostPitch;			///< @brief	Offset, in bytes, between the start of one host segment and the start of
														///			the next host segment (i.e. host memory bytes-per-row).
				ULWord		acSegmentDevicePitch;		///< @brief	Offset, in bytes, between the start of one device segment and the start of
														///			the next device segment (i.e. device memory bytes-per-row).

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs a disabled NTV2SegmentedDMAInfo struct.
				**/
				explicit		NTV2SegmentedDMAInfo ();

				/**
					@brief	Constructs an NTV2SegmentedDMAInfo struct from a segment count, a host pitch, and device pitch value.
					@param[in]	inNumSegments			Specifies the number of segments (lines) to DMA.
					@param[in]	inNumActiveBytesPerRow	Specifies the number of active bytes in a row of video.
					@param[in]	inHostBytesPerRow		Specifies the offset, in bytes, between two adjacent segments on the host.
					@param[in]	inDeviceBytesPerRow		Specifies the offset, in bytes, between two adjacent segments on the device.
				**/
				explicit		NTV2SegmentedDMAInfo (const ULWord inNumSegments, const ULWord inNumActiveBytesPerRow, const ULWord inHostBytesPerRow, const ULWord inDeviceBytesPerRow);

				/**
					@brief	Sets the NTV2SegmentedDMAInfo struct members.
					@param[in]	inNumSegments			Specifies the number of segments (lines) to DMA.
														If 1 or zero, performs a Reset, and the remaining parameters are ignored.
					@param[in]	inNumActiveBytesPerRow	Specifies the number of active bytes in a row of video.
					@param[in]	inHostBytesPerRow		Specifies the offset, in bytes, between two adjacent segments on the host.
					@param[in]	inDeviceBytesPerRow		Specifies the offset, in bytes, between two adjacent segments on the device.
				**/
				void			Set (const ULWord inNumSegments, const ULWord inNumActiveBytesPerRow, const ULWord inHostBytesPerRow, const ULWord inDeviceBytesPerRow);

				/**
					@brief	Resets the NTV2SegmentedDMAInfo struct members to their default values (normal, non-segmented AutoCirculate DMA transfers).
				**/
				void			Reset (void);

				/**
					@return	My segment count.
				**/
				inline ULWord	GetSegmentCount (void) const				{return acNumSegments;}

				/**
					@return	True if I'm currently active (i.e., I have more than one segment;  otherwise false.
				**/
				inline bool		IsSegmented (void) const					{return GetSegmentCount() > 1;}
			#endif	//	user-space clients only
		NTV2_STRUCT_END (NTV2SegmentedDMAInfo)


		/**
			@brief	Color correction data used with AUTOCIRCULATE_WITH_COLORCORRECT option.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2ColorCorrectionData)
			NTV2ColorCorrectionMode		ccMode;					///< @brief	My mode (off, RGB, YCbCr, or 3-way)
			ULWord						ccSaturationValue;		///< @brief	My saturation value, used only in 3-way color correction mode.

			/**
				@brief	RGB lookup tables pre-formatted for AJA hardware as a contiguous block of NTV2_COLORCORRECTOR_TABLESIZE bytes.
						This field is owned by the SDK, which is wholly responsible for allocating and/or freeing it. If empty, no color
						correction tables will be transferred. Use the Getter/Setter methods to get/set this field.
			**/
			NTV2_POINTER				ccLookupTables;

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs a default NTV2ColorCorrectionData struct.
				**/
				explicit	NTV2ColorCorrectionData ();

				/**
					@brief	My destructor, which frees my private color correction table buffer.
				**/
							~NTV2ColorCorrectionData ();

				/**
					@brief	Frees my private color correction table buffer and resets my mode to "invalid".
				**/
				void		Clear (void);

				/**
					@return	True if my mode is valid and not "Off", and my color correction tables are allocated.
				**/
				inline bool	IsActive (void) const					{return NTV2_IS_ACTIVE_COLOR_CORRECTION_MODE (ccMode) && ccLookupTables.GetHostPointer ();}

				/**
					@brief	Sets this struct from the given mode, saturation and table data, replacing any existing mode, saturation and table data.
					@param[in]	inMode			Specifies the color correction mode.
					@param[in]	inSaturation	Specifies the saturation value (valid only for 3-way color correction mode).
					@param[in]	pInTableData	Specifies a valid, non-NULL pointer to a buffer that's at least NTV2_COLORCORRECTOR_TABLESIZE
												bytes long, that contains the table data to copy into my private buffer.
					@return	True if successful;  otherwise false.
				**/
				bool		Set (const NTV2ColorCorrectionMode inMode, const ULWord inSaturation, const void * pInTableData);

				NTV2_BEGIN_PRIVATE
					inline explicit						NTV2ColorCorrectionData (const NTV2ColorCorrectionData & inObj)	: ccLookupTables (0) {(void) inObj;}	///< @brief	You can't construct an NTV2ColorCorrectionData from another.
					inline NTV2ColorCorrectionData &	operator = (const NTV2ColorCorrectionData & inRHS)				{(void) inRHS; return *this;}	///< @brief	You cannot assign NTV2ColorCorrectionData instances.
				NTV2_END_PRIVATE
			#endif	//	user-space clients only
		NTV2_STRUCT_END (NTV2ColorCorrectionData)


		/**
			@brief	This is returned from the CNTV2Card::AutoCirculateGetStatus function.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (AUTOCIRCULATE_STATUS)
				NTV2_HEADER				acHeader;					///< @brief	The common structure header -- ALWAYS FIRST!
					NTV2Crosspoint			acCrosspoint;				///< @brief	The crosspoint (channel number with direction)
					NTV2AutoCirculateState	acState;					///< @brief	Current AutoCirculate state.
					LWord					acStartFrame;				///< @brief	First frame to circulate.		FIXFIXFIX	Why is this signed?		CHANGE TO ULWord??
					LWord					acEndFrame;					///< @brief	Last frame to circulate.		FIXFIXFIX	Why is this signed?		CHANGE TO ULWord??
					LWord					acActiveFrame;				///< @brief	Current frame actually being captured/played when CNTV2Card::AutoCirculateGetStatus called.	FIXFIXFIX	CHANGE TO ULWord??
					ULWord64				acRDTSCStartTime;			///< @brief	Timestamp of the first VBI received after CNTV2Card::AutoCirculateStart called, using host OS system clock.
					ULWord64				acAudioClockStartTime;		///< @brief	Timestamp of the first VBI received after CNTV2Card::AutoCirculateStart called, using "64-bit clean" value of the device's 48kHz audio clock (::kRegAud1Counter register).
					ULWord64				acRDTSCCurrentTime;			///< @brief	Timestamp when CNTV2Card::AutoCirculateGetStatus called, using the host OS system clock.
					ULWord64				acAudioClockCurrentTime;	///< @brief	Timestamp when CNTV2Card::AutoCirculateGetStatus called, using "64-bit clean" value of the device's 48kHz audio clock (::kRegAud1Counter register).
					ULWord					acFramesProcessed;			///< @brief	Total number of frames successfully processed since CNTV2Card::AutoCirculateStart called.
					ULWord					acFramesDropped;			///< @brief	Total number of frames dropped since CNTV2Card::AutoCirculateStart called
					ULWord					acBufferLevel;				///< @brief	Number of buffered frames in driver ready to capture or play
					ULWord					acOptionFlags;				///< @brief	AutoCirculate options used when CNTV2Card::AutoCirculateInitForInput or CNTV2Card::AutoCirculateInitForOutput called (e.g., AUTOCIRCULATE_WITH_RP188, etc.).
					NTV2AudioSystem			acAudioSystem;				///< @brief	The audio system being used for this channel (NTV2_AUDIOSYSTEM_INVALID if none)
				NTV2_TRAILER			acTrailer;					///< @brief	The common structure trailer -- ALWAYS LAST!

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs a default AUTOCIRCULATE_STATUS struct for the given NTV2Crosspoint.
				**/
				explicit				AUTOCIRCULATE_STATUS (const NTV2Crosspoint inCrosspoint = NTV2CROSSPOINT_CHANNEL1);

				/**
					@brief		Copies my data into the given AUTOCIRCULATE_STATUS_STRUCT.
					@param[out]	outOldStruct	Specifies the old AUTOCIRCULATE_STATUS_STRUCT that is to receive the copied data.
					@return		True if successful;  otherwise false.
				**/
				bool					CopyTo	(AUTOCIRCULATE_STATUS_STRUCT & outOldStruct);

				/**
					@brief		Copies the given AUTOCIRCULATE_STATUS_STRUCT into me.
					@param[in]	inOldStruct		Specifies the old AUTOCIRCULATE_STATUS_STRUCT that is to supply the copied data.
					@return		True if successful;  otherwise false.
				**/
				bool					CopyFrom (const AUTOCIRCULATE_STATUS_STRUCT & inOldStruct);

				/**
					@brief		Clears my data.
				**/
				void					Clear (void);

				/**
					@return		The number of frames being auto-circulated.
				**/
				inline ULWord			GetFrameCount (void) const							{return IsStopped() ? 0 : ULWord (acEndFrame - acStartFrame + 1);}

				/**
					@return		The total number of frames dropped since AutoCirculateStart called.
				**/
				inline ULWord			GetDroppedFrameCount (void) const					{return acFramesDropped;}

				/**
					@return		The total number of frames successfully processed (not dropped) since AutoCirculateStart called.
				**/
				inline ULWord			GetProcessedFrameCount (void) const					{return acFramesProcessed;}

				/**
					@return		The number of buffered frames in the driver ready to capture or play.
				**/
				inline ULWord			GetBufferLevel (void) const							{return acBufferLevel;}

				/**
					@return		The number of "unoccupied" output (playout) frames the device's AutoCirculate channel can currently accommodate.
				**/
				inline ULWord			GetNumAvailableOutputFrames (void) const			{return GetFrameCount () > acBufferLevel ? GetFrameCount () - acBufferLevel : 0;}

				/**
					@return		True if the device's AutoCirculate channel is ready to accept at least one more output frame.
				**/
				inline bool				CanAcceptMoreOutputFrames (void) const				{return GetNumAvailableOutputFrames () > 1;}

				/**
					@return		True if there's at least one captured frame that's ready to transfer from the device to the host;  otherwise false.
				**/
				inline bool				HasAvailableInputFrame (void) const					{return acBufferLevel > 1;}

				/**
					@return		The current active frame number.
				**/
				inline LWord			GetActiveFrame (void) const							{return acActiveFrame;}

				/**
					@return		The first frame number.
				**/
				inline uint16_t			GetStartFrame (void) const							{return uint16_t(acStartFrame);}

				/**
					@return		The last frame number.
				**/
				inline uint16_t			GetEndFrame (void) const							{return uint16_t(acEndFrame);}

				/**
					@return		My current state.
				**/
				inline NTV2AutoCirculateState	GetState (void) const						{return acState;}

				/**
					@return		My audio system. If NTV2_AUDIOSYSTEM_INVALID, then no audio is being captured/played.
				**/
				inline NTV2AudioSystem	GetAudioSystem (void) const							{return acAudioSystem;}

				/**
					@return		True if my state is currently NTV2_AUTOCIRCULATE_RUNNING;  otherwise false.
				**/
				inline bool				IsRunning (void) const								{return GetState () == NTV2_AUTOCIRCULATE_RUNNING;}

				/**
					@return		True if my state is currently NTV2_AUTOCIRCULATE_STARTING or NTV2_AUTOCIRCULATE_STARTING_AT_TIME;  otherwise false.
				**/
				inline bool				IsStarting (void) const								{return GetState () == NTV2_AUTOCIRCULATE_STARTING  ||  GetState () == NTV2_AUTOCIRCULATE_STARTING_AT_TIME;}

				/**
					@return		True if my state is currently NTV2_AUTOCIRCULATE_DISABLED;  otherwise false.
				**/
				inline bool				IsStopped (void) const								{return GetState () == NTV2_AUTOCIRCULATE_DISABLED;}

				/**
					@return		True if my state is currently NTV2_AUTOCIRCULATE_STOPPING;  otherwise false.
				**/
				inline bool				IsStopping (void) const								{return GetState () == NTV2_AUTOCIRCULATE_STOPPING;}

				/**
					@return		True if my state is currently NTV2_AUTOCIRCULATE_PAUSED;  otherwise false.
				**/
				inline bool				IsPaused (void) const								{return GetState () == NTV2_AUTOCIRCULATE_PAUSED;}

				/**
					@return		True if circulating with audio;  otherwise false.
				**/
				inline bool				WithAudio (void) const								{return NTV2_IS_VALID_AUDIO_SYSTEM (GetAudioSystem ());}

				/**
					@return		The option flags.
				**/
				inline ULWord			OptionFlags (void) const							{return acOptionFlags;}	//	New in SDK 16.0

				/**
					@return		True if operating the custom Anc inserter/extractor;  otherwise false.
				**/
				inline bool				WithCustomAnc (void) const							{return OptionFlags() & AUTOCIRCULATE_WITH_ANC;}	//	New in SDK 16.0

				/**
					@return		True if capturing/playing RP188 timecode;  otherwise false.
				**/
				inline bool				WithRP188 (void) const								{return OptionFlags() & AUTOCIRCULATE_WITH_RP188;}	//	New in SDK 16.0

				/**
					@return		True if capturing/playing analog LTC timecode;  otherwise false.
				**/
				inline bool				WithLTC (void) const								{return OptionFlags() & AUTOCIRCULATE_WITH_LTC;}	//	New in SDK 16.0

				/**
					@return		True if AUTOCIRCULATE_WITH_FBFCHANGE enabled; otherwise false.
				**/
				inline bool				WithFBFChange (void) const							{return OptionFlags() & AUTOCIRCULATE_WITH_FBFCHANGE;}	//	New in SDK 16.0

				/**
					@return		True if AUTOCIRCULATE_WITH_FBOCHANGE enabled; otherwise false.
				**/
				inline bool				WithFBOChange (void) const							{return OptionFlags() & AUTOCIRCULATE_WITH_FBOCHANGE;}	//	New in SDK 16.0

				/**
					@return		True if AUTOCIRCULATE_WITH_COLORCORRECT enabled; otherwise false.
				**/
				inline bool				WithColorCorrect (void) const						{return OptionFlags() & AUTOCIRCULATE_WITH_COLORCORRECT;}	//	New in SDK 16.0

				/**
					@return		True if AUTOCIRCULATE_WITH_VIDPROC enabled; otherwise false.
				**/
				inline bool				WithVidProc (void) const							{return OptionFlags() & AUTOCIRCULATE_WITH_VIDPROC;}	//	New in SDK 16.0

				/**
					@return		True if running in field mode;  otherwise false.
				**/
				inline bool				IsFieldMode (void) const							{return OptionFlags() & AUTOCIRCULATE_WITH_FIELDS;}	//	New in SDK 16.0

				/**
					@return		True if capturing/playing with HDMI auxiliary data;  otherwise false.
				**/
				inline bool				WithHDMIAuxData (void) const						{return OptionFlags() & AUTOCIRCULATE_WITH_HDMIAUX;}	//	New in SDK 16.0

				/**
					@return		True if capturing;  otherwise false.
				**/
				inline bool				IsInput (void) const								{return NTV2_IS_INPUT_CROSSPOINT(acCrosspoint);}

				/**
					@return		True if playing out;  otherwise false.
				**/
				inline bool				IsOutput (void) const								{return NTV2_IS_OUTPUT_CROSSPOINT(acCrosspoint);}

				/**
					@return		The NTV2Mode, whether NTV2_MODE_OUTPUT, NTV2_MODE_INPUT or NTV2_MODE_INVALID (if stopped).
				**/
				inline NTV2Mode			GetMode (void) const								{return IsStopped() ? NTV2_MODE_INVALID : (IsInput() ? NTV2_MODE_INPUT : NTV2_MODE_OUTPUT);}	//	New in SDK 15.5

				/**
					@return		My channel.
				**/
				NTV2Channel				GetChannel (void) const;

				/**
					@return		A string containing the human-readable data for the given zero-based index value.
					@param[in]	inIndexNum	A zero-based index number that specifies which member data value to return.
				**/
				std::string				operator []	(const unsigned inIndexNum) const;

				NTV2_IS_STRUCT_VALID_IMPL(acHeader,acTrailer)
			#endif	//	user-space clients only
		NTV2_STRUCT_END (AUTOCIRCULATE_STATUS)


		#if !defined (NTV2_BUILDING_DRIVER)
			typedef std::set <ULWord>						NTV2RegisterNumberSet;	///< @brief	A set of distinct ULWord values.
			typedef NTV2RegisterNumberSet					NTV2RegNumSet;			///< @brief	A set of distinct NTV2RegisterNumbers.
			typedef NTV2RegNumSet::const_iterator			NTV2RegNumSetConstIter;	///< @brief	A const iterator that iterates over a set of distinct NTV2RegisterNumbers.
			typedef NTV2RegNumSet::iterator					NTV2RegNumSetIter;		///< @brief	A non-constant iterator that iterates over a set of distinct NTV2RegisterNumbers.

			/**
				@brief	Adds the given register number to the specified NTV2RegisterNumberSet.
				@param		inOutSet			Specifies the NTV2RegisterNumberSet to be added to.
				@param[in]	inRegisterNumber	Specifies the NTV2RegisterNumber to add to the set.
				@return	A non-constant reference to the specified NTV2RegisterNumberSet.
			**/
			NTV2RegisterNumberSet & operator << (NTV2RegisterNumberSet & inOutSet, const ULWord inRegisterNumber);

			typedef std::map <ULWord, ULWord>				NTV2RegisterValueMap;		///< @brief	A mapping of distinct NTV2RegisterNumbers to their corresponding ULWord values.
			typedef NTV2RegisterValueMap::const_iterator	NTV2RegValueMapConstIter;	///< @brief	A const iterator that iterates over NTV2RegisterValueMap entries.
			typedef NTV2RegisterValueMap::iterator			NTV2RegValueMapIter;		///< @brief	A non-constant iterator that iterates over NTV2RegisterValueMap entries.
		#endif	//	!defined (NTV2_BUILDING_DRIVER)


		/**
			@brief	This is used by the CNTV2Card::ReadRegisters function.
			@note	There is no need to access any of this structure's fields directly. Simply call the CNTV2Card instance's ReadRegisters function.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2GetRegisters)		//	AUTOCIRCULATE_TYPE_GETREGS
			NTV2_BEGIN_PRIVATE
				NTV2_HEADER		mHeader;			///< @brief	The common structure header -- ALWAYS FIRST!
					ULWord			mInNumRegisters;	///< @brief	The number of registers to read in one batch.
					NTV2_POINTER	mInRegisters;		///< @brief	Array of register numbers to be read in one batch. The SDK owns this memory.
					ULWord			mOutNumRegisters;	///< @brief	The number of registers successfully read.
					NTV2_POINTER	mOutGoodRegisters;	///< @brief	Array of register numbers that were read successfully. The SDK owns this memory.
					NTV2_POINTER	mOutValues;			///< @brief	Array of register values that were read successfully. The SDK owns this memory.
				NTV2_TRAILER	mTrailer;			///< @brief	The common structure trailer -- ALWAYS LAST!
			NTV2_END_PRIVATE

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs an NTV2GetRegisters struct from the given NTV2RegisterNumberSet.
					@param[in]	inRegisterNumbers	A set of distinct NTV2RegisterNumbers to copy into the mRegisters field.
													If omitted, defaults to an empty set.
				**/
				explicit	NTV2GetRegisters (const NTV2RegNumSet & inRegisterNumbers = NTV2RegNumSet ());

				explicit	NTV2GetRegisters (NTV2RegisterReads & inRegReads);

				/**
					@brief	Resets me, starting over, now using the given NTV2RegisterNumberSet.
					@param[in]	inRegisterNumbers	A set of distinct NTV2RegisterNumbers to copy into my mInRegisters field.
				**/
				bool		ResetUsing (const NTV2RegNumSet & inRegisterNumbers);

				/**
					@brief	Resets me, starting over, using the given NTV2RegisterReads vector.
					@param[in]	inRegReads			A vector of NTV2RegInfo values to use for my mInRegisters field.
					@note		The mask and shift fields of the NTV2RegInfo values are ignored.
				**/
				bool		ResetUsing (const NTV2RegisterReads & inRegReads);

				/**
					@brief		Returns an NTV2RegNumSet built from my mOutGoodRegisters field.
					@param[out]	outGoodRegNums	Receives the set of "good" registers.
					@return		True if successful;  otherwise false.
				**/
				bool		GetGoodRegisters (NTV2RegNumSet & outGoodRegNums) const;

				/**
					@brief	Returns an NTV2RegisterValueMap built from my mOutGoodRegisters and mOutValues fields.
					@param[out]	outValues	Receives the register/value map.
					@return	True if successful;  otherwise false.
				**/
				bool		GetRegisterValues (NTV2RegisterValueMap & outValues) const;

				/**
					@brief	Returns a vector of NTV2RegInfo values built from my mOutGoodRegisters and mOutValues fields.
					@param[out]	outValues	Receives the register values.
					@return	True if successful;  otherwise false.
				**/
				bool		GetRegisterValues (NTV2RegisterReads & outValues) const;

				/**
					@brief	Prints a human-readable representation of me to the given output stream.
					@param	inOutStream		Specifies the output stream to use.
					@return	A reference to the output stream.
				**/
				std::ostream &	Print (std::ostream & inOutStream) const;

				NTV2_IS_STRUCT_VALID_IMPL(mHeader,mTrailer)

				NTV2_BEGIN_PRIVATE
					inline explicit				NTV2GetRegisters (const NTV2GetRegisters & inObj)	:	mHeader(0xFEFEFEFE, 0), mInRegisters(0), mOutGoodRegisters(0), mOutValues(0)
																									{(void) inObj;}					///< @brief	You cannot construct an NTV2GetRegisters from another.
					inline NTV2GetRegisters &	operator = (const NTV2GetRegisters & inRHS)			{(void) inRHS; return *this;}	///< @brief	You cannot assign NTV2GetRegisters.
				NTV2_END_PRIVATE

			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (NTV2GetRegisters)


		/**
			@brief	This is used by the CNTV2Card::WriteRegisters function.
			@note	There is no need to access any of this structure's fields directly. Simply call the CNTV2Card instance's WriteRegisters function.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2SetRegisters)		//	AUTOCIRCULATE_TYPE_SETREGS
			NTV2_HEADER		mHeader;			///< @brief	The common structure header -- ALWAYS FIRST!
				ULWord			mInNumRegisters;	///< @brief	The number of NTV2RegInfo's to be set.
				NTV2_POINTER	mInRegInfos;		///< @brief	Read-only array of NTV2RegInfo structs to be set. The SDK owns this memory.
				ULWord			mOutNumFailures;	///< @brief	The number of registers unsuccessfully written.
				NTV2_POINTER	mOutBadRegIndexes;	///< @brief	Array of UWords containing index numbers of the register writes that failed. The SDK owns this memory.
			NTV2_TRAILER	mTrailer;			///< @brief	The common structure trailer -- ALWAYS LAST!

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs an NTV2SetRegisters struct from the given NTV2RegisterWrites collection.
					@param[in]	inRegWrites		An ordered collection of NTV2ReadWriteRegisterSingle structs to be copied into my mInRegInfos field.
												If omitted, defaults to an empty collection.
				**/
							NTV2SetRegisters (const NTV2RegisterWrites & inRegWrites = NTV2RegisterWrites ());

				/**
					@brief	Resets me, starting over, now using the given NTV2RegisterNumberSet.
					@param[in]	inRegWrites		An ordered collection of NTV2ReadWriteRegisterSingle structs to be copied into my mInRegInfos field.
												If omitted, defaults to an empty collection.
				**/
				bool		ResetUsing (const NTV2RegisterWrites & inRegWrites);

				/**
					@brief		Returns an NTV2RegisterWrites built from my mOutBadRegInfos field.
					@param[out]	outFailedRegWrites	Receives the list of failed writes.
					@return		True if successful;  otherwise false.
				**/
				bool		GetFailedRegisterWrites (NTV2RegisterWrites & outFailedRegWrites) const;

				/**
					@brief	Prints a human-readable representation of me to the given output stream.
					@param	inOutStream		Specifies the output stream to use.
					@return	A reference to the output stream.
				**/
				std::ostream &	Print (std::ostream & inOutStream) const;

				NTV2_IS_STRUCT_VALID_IMPL(mHeader,mTrailer)

				NTV2_BEGIN_PRIVATE
					inline explicit				NTV2SetRegisters (const NTV2SetRegisters & inObj)	:	mHeader(0xFEFEFEFE, 0), mInNumRegisters(0), mInRegInfos(0), mOutNumFailures(0), mOutBadRegIndexes(0)
																									{(void) inObj;}					///< @brief	You cannot construct an NTV2SetRegisters from another.
					inline NTV2SetRegisters &	operator = (const NTV2SetRegisters & inRHS)			{(void) inRHS; return *this;}	///< @brief	You cannot assign NTV2SetRegisters.
				NTV2_END_PRIVATE

			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (NTV2SetRegisters)


		/**
			@brief	This is used to atomically perform bank-selected register reads or writes.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2BankSelGetSetRegs)		//	NTV2_TYPE_BANKGETSET
			NTV2_HEADER		mHeader;			///< @brief	The common structure header -- ALWAYS FIRST!
				ULWord			mIsWriting;			///< @brief	If non-zero, register(s) will be written;  otherwise, register(s) will be read.
				NTV2_POINTER	mInBankInfos;		///< @brief	Bank select NTV2RegInfo. The SDK owns this memory.
				NTV2_POINTER	mInRegInfos;		///< @brief	NTV2RegInfo array of registers be read/written. The SDK owns this memory.
			NTV2_TRAILER	mTrailer;			///< @brief	The common structure trailer -- ALWAYS LAST!

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs an NTV2BankSelGetSetRegs struct for atomically reading or writing the given bank-selected register.
					@param[in]	inBankSelect	The bank select register info.
					@param[in]	inRegInfo		The register info of the register to be read or written.
					@param[in]	inDoWrite		Specify 'true' to write the register value;  otherwise use 'false'. Defaults to 'false'.
				**/
				explicit	NTV2BankSelGetSetRegs (const NTV2RegInfo & inBankSelect, const NTV2RegInfo & inRegInfo, const bool inDoWrite = false);

				/**
					@return	The NTV2RegInfo at the given zero-based index position.
					@param	inIndex0		Specifies the zero-based index of the NTV2RegInfo to retrieve from my array.
				**/
				NTV2RegInfo		GetRegInfo (const UWord inIndex0 = 0) const;

				/**
					@brief	Prints a human-readable representation of me to the given output stream.
					@param	inOutStream		Specifies the output stream to use.
					@return	A reference to the output stream.
				**/
				std::ostream &	Print (std::ostream & inOutStream) const;

				NTV2_IS_STRUCT_VALID_IMPL(mHeader,mTrailer)

				NTV2_BEGIN_PRIVATE
					inline explicit				NTV2BankSelGetSetRegs (const NTV2BankSelGetSetRegs & inObj)	:	mHeader(0xFEFEFEFE, 0), mIsWriting(false), mInBankInfos(0), mInRegInfos(0)
																										{(void) inObj;}					///< @brief	You cannot construct an NTV2BankSelGetSetRegs from another.
					inline NTV2BankSelGetSetRegs &	operator = (const NTV2BankSelGetSetRegs & inRHS)	{(void) inRHS; return *this;}	///< @brief	You cannot assign NTV2BankSelGetSetRegs.
				NTV2_END_PRIVATE
			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (NTV2BankSelGetSetRegs)


        /**
            @brief	This is used to perform virtual data reads or writes.
            @note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
        **/
        NTV2_STRUCT_BEGIN (NTV2VirtualData)	//	NTV2_TYPE_VIRTUAL_DATA_RW
            NTV2_HEADER		mHeader;			///< @brief	The common structure header -- ALWAYS FIRST!
                ULWord			mTag;               ///< @brief	Tag for virtual data.  This value is used to recal saved data by tag.
                ULWord			mIsWriting;			///< @brief	If non-zero, virtual data will be written;  otherwise, virtual data will be read.
                NTV2_POINTER	mVirtualData;		///< @brief	Pointer object to virtual data. The SDK owns this memory.
            NTV2_TRAILER	mTrailer;			///< @brief	The common structure trailer -- ALWAYS LAST!

            #if !defined (NTV2_BUILDING_DRIVER)
                /**
                    @brief	Constructs an NTV2VirtualData struct for reading or writing virtual data.
                    @param[in]	inTag               The tag to use.
                    @param[in]	inVirtualData       Pointer to vitrual data.
                    @param[in]	inVirtualDataSize   The size of the virtual data.
                    @param[in]	inDoWrite           True if writing, false if reading.
                **/
                explicit	NTV2VirtualData (const ULWord inTag, const void* inVirtualData, const size_t inVirtualDataSize, const bool inDoWrite = false);

                /**
                    @brief	Prints a human-readable representation of me to the given output stream.
                    @param	inOutStream		Specifies the output stream to use.
                    @return	A reference to the output stream.
                **/
                std::ostream &	Print (std::ostream & inOutStream) const;

                NTV2_IS_STRUCT_VALID_IMPL(mHeader,mTrailer)

            #endif	//	!defined (NTV2_BUILDING_DRIVER)
        NTV2_STRUCT_END (NTV2VirtualData)


		/**
			@brief	This is used by the CNTV2Card::ReadSDIStatistics function.
			@note	There is no need to access any of this structure's fields directly. Simply call the CNTV2Card instance's ReadSDIStatistics function.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2SDIInStatistics)		//	AUTOCIRCULATE_TYPE_SDISTATS
			NTV2_BEGIN_PRIVATE
				NTV2_HEADER		mHeader;			///< @brief	The common structure header -- ALWAYS FIRST!
					NTV2_POINTER	mInStatistics;		///< @brief	Array of NTV2SDIStatus s to be read in one batch. The SDK owns this memory.
				NTV2_TRAILER	mTrailer;			///< @brief	The common structure trailer -- ALWAYS LAST!
			NTV2_END_PRIVATE

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs an NTV2GetSDIStatistics struct and initializes it to its default state.
				**/
				NTV2SDIInStatistics ();

				/**
					@brief		Resets the struct to its initialized state.
				**/
				void		Clear (void);

				/**
					@brief		Answers with the NTV2SDIInputStatus for the given SDI input spigot.
					@param[out]	outStatus			Receives the NTV2SDIInputStatus for the given SDI input.
					@param[in]	inSDIInputIndex0	Specifies the zero-based index of the SDI input of interest.
					@return		True if successful;  otherwise false.
				**/
				bool			GetSDIInputStatus (NTV2SDIInputStatus & outStatus, const UWord inSDIInputIndex0 = 0);

				/**
					@return		A non-const reference to the NTV2SDIInputStatus record for the given SDI input.
					@param[in]	inSDIInputIndex0	Specifies the zero-based index of the SDI input of interest.
				**/
				NTV2SDIInputStatus &	operator [] (const size_t inSDIInputIndex0);	//	New in SDK 16.0

				/**
					@brief	Prints a human-readable representation of me to the given output stream.
					@param	inOutStream		Specifies the output stream to use.
					@return	A reference to the output stream.
				**/
				std::ostream &	Print (std::ostream & inOutStream) const;

				NTV2_IS_STRUCT_VALID_IMPL(mHeader,mTrailer)
			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (NTV2SDIInStatistics)


		/**
			@brief	This is returned by the CNTV2Card::AutoCirculateGetFrameStamp function, and is also embedded in the AUTOCIRCULATE_TRANSFER struct
					returned from CNTV2Card::AutoCirculateTransfer. If used as its own CNTV2DriverInterface::NTV2Message (the new API version of the old CNTV2Card::GetFrameStamp call),
					pass the NTV2Channel in the least significant byte of FRAME_STAMP::acFrameTime, and the requested frame in FRAME_STAMP::acRequestedFrame.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (FRAME_STAMP)	//	AUTOCIRCULATE_TYPE_FRAMESTAMP
				NTV2_HEADER			acHeader;						///< @brief	The common structure header -- ALWAYS FIRST!
					LWord64				acFrameTime;					///< @brief	On exit, contains host OS clock at time of capture/play.
																		///<		On entry, contains ::NTV2Channel of interest, but only for new API ::FRAME_STAMP message.
					ULWord				acRequestedFrame;				///< @brief	The frame requested (0xFFFFFFFF == "not available"), including for new API (::FRAME_STAMP message).
					ULWord64			acAudioClockTimeStamp;			///< @brief	Number of 10MHz ticks at moment of play or record, based on 48kHz clock (from register 28).
					ULWord				acAudioExpectedAddress;			///< @brief	The address that was used to transfer
					ULWord				acAudioInStartAddress;			///< @brief	For record - first position in buffer of audio (includes base offset) -- AudioInAddress at the time this Frame was stamped
					ULWord				acAudioInStopAddress;			///< @brief	For record - end position (exclusive) in buffer of audio (includes base offset) -- AudioInAddress at the Frame AFTER this Frame was stamped
					ULWord				acAudioOutStopAddress;			///< @brief	For play - first position in buffer of audio -- AudioOutAddress at the time this Frame was stamped
					ULWord				acAudioOutStartAddress;			///< @brief	For play - end position (exclusive) in buffer of audio -- AudioOutAddress at the Frame AFTER it was stamped
					ULWord				acTotalBytesTransferred;		///< @brief	Total audio and video bytes transferred
					ULWord				acStartSample;					///< @brief	The actual start sample when this frame was started in VBI, which may be used to check sync against
																		///<		acAudioInStartAddress (Play) or acAudioOutStartAddress (Record).  In record it will always be equal,
																		///<		but in playback if the clocks drift or the user supplies non aligned audio sizes, then this will
																		///<		give the current difference from expected versus actual position. To be useful, playback audio must
																		///<		be clocked in at the correct rate.
					/**
						@name	Current (Active) Frame Information
					**/
					///@{

					/**
						@brief	Intended for capture, this is a sequence of ::NTV2_RP188 values received from the device (in ::NTV2TCIndex order).
								If empty, no timecodes will be transferred. This field is ignored if ::AUTOCIRCULATE_WITH_RP188 option is not set.
						@note	This field is owned by the SDK, which is responsible for allocating and/or freeing it.
								Call FRAME_STAMP::GetInputTimeCodes or FRAME_STAMP::GetInputTimeCode to retrieve the timecodes stored in this field.
					**/
					NTV2_POINTER		acTimeCodes;
					LWord64				acCurrentTime;					///< @brief	Current processor time, derived from the finest-grained counter available on the host OS.
																		///<		Granularity can vary depending on the HAL. FRAME_STAMP::acAudioClockCurrentTime is the recommended time-stamp to use instead of this.
					ULWord				acCurrentFrame;					///< @brief	Last vertical blank frame for this autocirculate channel (when CNTV2Card::AutoCirculateGetFrameStamp was called)
					LWord64				acCurrentFrameTime;				///< @brief	Vertical blank start of current frame
					ULWord64			acAudioClockCurrentTime;		///< @brief	Current time expressed as a count of 10MHz ticks, based on 48kHz clock (from register 28).
					ULWord				acCurrentAudioExpectedAddress;	//	FIXFIXFIX	Document		What is this?!
					ULWord				acCurrentAudioStartAddress;		///< @brief	As set by play
					ULWord				acCurrentFieldCount;			///< @brief	As found by ISR at Call Field0 or Field1 _currently_ being OUTPUT (when CNTV2Card::AutoCirculateGetFrameStamp was called)
					ULWord				acCurrentLineCount;				///< @brief	At Call Line# _currently_ being OUTPUT (at the time of the IOCTL_NTV2_GET_FRAMESTAMP)
					ULWord				acCurrentReps;					///< @brief	Contains validCount (Playout:  on repeated frames, number of reps remaining; Record: drops on frame)
					ULWord64			acCurrentUserCookie;			///< @brief	The frame's AUTOCIRCULATE_TRANSFER::acInUserCookie value that was set when CNTV2Card::AutoCirculateTransfer was called.
																		///			This can tell clients which frame was on-air at the last VBI.
					ULWord				acFrame;						///< @brief	Record/capture -- current frame number
					NTV2_SHOULD_BE_DEPRECATED	(NTV2_RP188	acRP188);	///< @brief	Deprecated -- call FRAME_STAMP::GetInputTimeCode instead.

					///@}
				NTV2_TRAILER		acTrailer;						///< @brief	The common structure trailer -- ALWAYS LAST!

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs a default FRAME_STAMP structure.
				**/
				explicit	FRAME_STAMP ();

				/**
					@brief	Constructs me from another FRAME_STAMP.
					@param[in]	inObj	Specifies the FRAME_STAMP to be copied.
				**/
							FRAME_STAMP (const FRAME_STAMP & inObj);

				/**
					@brief	My destructor.
				**/
							~FRAME_STAMP ();

				/**
					@brief		Sets my fields from the given FRAME_STAMP_STRUCT.
					@param[in]	inOldStruct		Specifies the FRAME_STAMP_STRUCT that is to be copied into me.
				**/
				bool		SetFrom	(const FRAME_STAMP_STRUCT & inOldStruct);

				/**
					@brief		Copies my fields into the given FRAME_STAMP_STRUCT.
					@param[out]	outOldStruct	Specifies the FRAME_STAMP_STRUCT that is to be copied from me.
				**/
				bool		CopyTo	(FRAME_STAMP_STRUCT & outOldStruct) const;

				/**
					@brief		Returns all RP188 timecodes associated with the frame in NTV2TCIndex order.
					@param[out]	outValues	Receives the NTV2TimeCodeList that was transferred from the device.
											An NTV2TCIndex enum value can be used to index into the list to fetch the desired timecode of interest.
					@return		True if successful;  otherwise false.
				**/
				bool		GetInputTimeCodes (NTV2TimeCodeList & outValues) const;

				/**
					@brief		Returns only those timecodes from the frame that are associated with a given SDI input.
					@param[out]	outTimeCodes	Receives the timecode values.
					@param[in]	inSDIInput		Specifies the SDI input of interest, expressed as an ::NTV2Channel.
					@param[in]	inValidOnly		Optionally specifies if only valid timecodes should be returned. Defaults to true.
												Specify false to return all timecode values associated with the channel of interest,
												even if they're not valid.
					@return		True if successful;  otherwise false.
				**/
				bool		GetInputTimeCodes (NTV2TimeCodes & outTimeCodes, const NTV2Channel inSDIInput, const bool inValidOnly = true) const;

				/**
					@brief		Answers with a specific timecode captured in my acTimeCodes member.
					@param[out]	outTimeCode		Receives the requested timecode value.
					@param[in]	inTCIndex		Specifies which NTV2TCIndex to use. Defaults to NTV2_TCINDEX_SDI1.
					@note		Note that specifying an SDI input source that's not connected to the framestore associated with the
								channel being AutoCirculated will likely result in a timecode that's not frame-accurate.
					@return		True if successful;  otherwise false.
				**/
				bool		GetInputTimeCode (NTV2_RP188 & outTimeCode, const NTV2TCIndex inTCIndex = NTV2_TCINDEX_SDI1) const;

				/**
					@brief		Answers with the NTV2SDIInputStatus for the given SDI input spigot.
					@param[out]	outStatus			Receives the NTV2SDIInputStatus for the given SDI input.
					@param[in]	inSDIInputIndex0	Specifies the zero-based index of the SDI input of interest.
					@return		True if successful;  otherwise false.
				**/
				bool		GetSDIInputStatus (NTV2SDIInputStatus & outStatus, const UWord inSDIInputIndex0 = 0) const;

				/**
					@brief		Sets one of my input timecodes.
					@param[in]	inTCNdx		Specifies which timecode (slot) to set.
					@param[in]	inTimecode	Specifies the timecode data to write.
					@return		True if successful;  otherwise false.
					@note		This function is provided for internal SDK use only.
				**/
				bool		SetInputTimecode (const NTV2TCIndex inTCNdx, const NTV2_RP188 & inTimecode);

				/**
					@param[in]	inRHS		The FRAME_STAMP to be assigned (copied) into me.
					@return		A reference to me.
				**/
				FRAME_STAMP &	operator = (const FRAME_STAMP & inRHS);

				/**
					@return		A string containing the human-readable data for the given zero-based index value.
					@param[in]	inIndexNum	A zero-based index number that specifies which member data value to return.
				**/
				std::string				operator []	(const unsigned inIndexNum) const;

				NTV2_IS_STRUCT_VALID_IMPL(acHeader,acTrailer)

			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (FRAME_STAMP)


		/**
			@brief	This object is embedded in the AUTOCIRCULATE_TRANSFER struct that's returned from the CNTV2Card::AutoCirculateTransfer function,
					and contains status information about the transfer and the state of AutoCirculate.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (AUTOCIRCULATE_TRANSFER_STATUS)	//	AUTOCIRCULATE_TYPE_XFERSTATUS
				NTV2_HEADER				acHeader;				///< @brief	The common structure header -- ALWAYS FIRST!
					NTV2AutoCirculateState  acState;				///< @brief	Current AutoCirculate state after the transfer
					LWord					acTransferFrame;		///< @brief	Frame buffer number the frame was transferred to/from. (-1 if failed)
					ULWord					acBufferLevel;			///< @brief	The number of frames ready to record/play after the transfer.
					ULWord					acFramesProcessed;		///< @brief	Total number of frames successfully processed since AutoCirculateStart.
					ULWord					acFramesDropped;		///< @brief	Total number of frames dropped since AutoCirculateStart.
					FRAME_STAMP				acFrameStamp;			///< @brief	Frame stamp for the transferred frame.
					ULWord					acAudioTransferSize;	///< @brief Number of bytes captured into the audio buffer.
					ULWord					acAudioStartSample;		///< @brief	Starting audio sample (valid for capture only).
					ULWord					acAncTransferSize;		///< @brief Total ancillary data bytes for field 1 transferred (capture only).
					ULWord					acAncField2TransferSize;///< @brief Total ancillary data bytes for field 2 transferred (capture only).
				NTV2_TRAILER			acTrailer;				///< @brief	The common structure trailer -- ALWAYS LAST!

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@brief	Constructs a default AUTOCIRCULATE_TRANSFER_STATUS structure.
				**/
				explicit						AUTOCIRCULATE_TRANSFER_STATUS ();

				/**
					@return		My FRAME_STAMP member.
				**/
				inline const FRAME_STAMP &		GetFrameStamp (void) const								{return acFrameStamp;}

				/**
					@return		The frame buffer number the frame was transferred to or from (or -1 if the transfer failed).
				**/
				inline LWord					GetTransferFrame (void) const							{return acTransferFrame;}

				/**
					@return		My state after the transfer.
				**/
				inline NTV2AutoCirculateState	GetState (void) const									{return acState;}

				/**
					@return		My buffer level after the transfer.
				**/
				inline ULWord					GetBufferLevel (void) const								{return acBufferLevel;}

				/**
					@return		The total number of frames successfully processed (not dropped) since AutoCirculateStart called.
				**/
				inline ULWord					GetProcessedFrameCount (void) const						{return acFramesProcessed;}

				/**
					@return		The total number of frames dropped since AutoCirculateStart called.
				**/
				inline ULWord					GetDroppedFrameCount (void) const						{return acFramesDropped;}

				/**
					@return		The number of audio bytes deposited/transferred into the host audio buffer after
								the last successful CNTV2Card::AutoCirculateTransfer.
				**/
				inline ULWord					GetCapturedAudioByteCount (void) const					{return acAudioTransferSize;}

				/**
					@return		The number of ancillary data bytes deposited/transferred into the host anc buffer after
								the last successful CNTV2Card::AutoCirculateTransfer.
					@param[in]	inField2	Specify \c true for Field 2;  otherwise \c false (the default) for Field 1 (or progessive).
				**/
				inline ULWord					GetCapturedAncByteCount (const bool inField2 = false) const	{return inField2 ? acAncField2TransferSize : acAncTransferSize;}

				NTV2_IS_STRUCT_VALID_IMPL(acHeader,acTrailer)

				NTV2_BEGIN_PRIVATE
					inline explicit							AUTOCIRCULATE_TRANSFER_STATUS (const AUTOCIRCULATE_TRANSFER_STATUS & inObj)	: acHeader(0xFEFEFEFE, 0) {(void) inObj;}					///< @brief	You cannot construct an AUTOCIRCULATE_TRANSFER_STATUS from another.
					inline AUTOCIRCULATE_TRANSFER_STATUS &	operator = (const AUTOCIRCULATE_TRANSFER_STATUS & inRHS)					{(void) inRHS; return *this;}	///< @brief	You cannot assign AUTOCIRCULATE_TRANSFER_STATUSs.
				NTV2_END_PRIVATE
			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (AUTOCIRCULATE_TRANSFER_STATUS)


		/**
			@brief	This object specifies the information that will be transferred to or from the AJA device in the CNTV2Card::AutoCirculateTransfer
					function call. It will be used by the device driver only if the AUTOCIRCULATE_WITH_ANC option was used in the call to
					CNTV2Card::AutoCirculateInitForInput or CNTV2Card::AutoCirculateInitForOutput.
			@note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (AUTOCIRCULATE_TRANSFER)
				NTV2_HEADER						acHeader;					///< @brief	The common structure header -- ALWAYS FIRST!

					/**
						@brief	The host video buffer. This field is owned by the client application, and thus is responsible for allocating and/or freeing it.
								If the pointer is NULL or the size is zero, no video will be transferred. AJA recommends keeping this buffer 64-bit aligned
								and page-aligned for best performance. Use the AUTOCIRCULATE_TRANSFER::SetVideoBuffer method to set or reset this field.
					**/
					NTV2_POINTER					acVideoBuffer;

					/**
						@brief	The host audio buffer. This field is owned by the client application, and thus is responsible for allocating and/or freeing it.
								If the pointer is NULL or the size is zero, no audio will be transferred. AJA recommends keeping this buffer 64-bit aligned
								and page-aligned for best performance. Use the AUTOCIRCULATE_TRANSFER::SetAudioBuffer method to set or reset this field.
					**/
					NTV2_POINTER					acAudioBuffer;

					/**
						@brief	The host ancillary data buffer. This field is owned by the client application, and thus is responsible for allocating and/or
								freeing it. If the pointer is NULL or the size is zero, no ancillary data will be transferred.
								Use the AUTOCIRCULATE_TRANSFER::SetAncBuffers method to set or reset this field.
						@note	If non-empty (i.e., non-NULL), be sure that the pointer address is aligned to the nearest 8-byte boundary. AJA recommends
								using a full 2048-byte buffer. For playout, its content should be compatible with what's documented in \ref ancillarydata
								and all subsequent bytes in the buffer should be zero. For capture, AJA recommends clearing (zeroing) the buffer prior to
								each transfer.
					**/
					NTV2_POINTER					acANCBuffer;

					/**
						@brief	The host "Field 2" ancillary data buffer. This field is owned by the client application, and thus is responsible for allocating
								and/or freeing it. If the pointer is NULL or the size is zero, no "Field 2" ancillary data will be transferred.
								Use the AUTOCIRCULATE_TRANSFER::SetAncBuffers method to set or reset this field.
						@note	If non-empty (i.e., non-NULL), be sure that the pointer address is aligned to the nearest 8-byte boundary. AJA recommends
								using a full 2048-byte buffer. For playout, its content should be compatible with what's documented in \ref ancillarydata
								and all subsequent bytes in the buffer should be zero. For capture, AJA recommends clearing (zeroing) the buffer prior to
								each transfer.
					**/
					NTV2_POINTER					acANCField2Buffer;

					/**
						@brief	Intended for playout, this is an ordered sequence of NTV2_RP188 values to send to the device. If empty, no timecodes will
								be transferred. This field is ignored if AUTOCIRCULATE_WITH_RP188 option is not set.
						@note	This field is owned by the SDK, which is responsible for allocating and/or freeing it.
								Use my AUTOCIRCULATE_TRANSFER::SetOutputTimeCodes or AUTOCIRCULATE_TRANSFER::SetOutputTimeCode methods to change this field.
					**/
					NTV2_POINTER					acOutputTimeCodes;

					/**
						@brief	Contains status information that's valid after CNTV2Card::AutoCirculateTransfer returns, including the driver buffer level, number of
								frames processed or dropped, audio and anc transfer byte counts, and a complete FRAME_STAMP that has even more detailed
								clocking information.
					**/
					AUTOCIRCULATE_TRANSFER_STATUS	acTransferStatus;

					/**
						@brief	Intended for playout, an optional app-specific cookie value that tags this frame, such that if this same tag appears in
								acTransferStatus.acFrameStamp.acCurrentUserCookie after AutoCirculateTransfer returns, or if it appears in the FRAME_STAMP's
								acCurrentUserCookie field after CNTV2Card::AutoCirculateGetFrameStamp returns, identifies when the frame was on-the-air.
					**/
					ULWord64						acInUserCookie;

					/**
						@brief	Optional byte offset into the device frame buffer. Defaults to zero. If non-zero, should be a multiple of the line length,
								or else bad video will likely result. On capture, this specifies where the first byte in the device video buffer will be
								read from (and that byte will end up in the first byte position in the host video buffer. Conversely, on playout, this
								specifies where the first byte from the host video buffer will end up in the device video buffer.
					**/
					ULWord							acInVideoDMAOffset;

					NTV2SegmentedDMAInfo			acInSegmentedDMAInfo;		///< @brief	Optional segmented DMA info, for use with specialized data transfers.
					NTV2ColorCorrectionData			acColorCorrection;			///< @brief	Color correction data. This field is ignored if AUTOCIRCULATE_WITH_COLORCORRECT option is not set.
					NTV2FrameBufferFormat			acFrameBufferFormat;		///< @brief	Specifies the frame buffer format to change to. Ignored if AUTOCIRCULATE_WITH_FBFCHANGE option is not set.
					NTV2VideoFrameBufferOrientation	acFrameBufferOrientation;	///< @brief	Specifies the frame buffer orientation to change to. Ignored if AUTOCIRCULATE_WITH_FBOCHANGE option is not set.
					AutoCircVidProcInfo				acVidProcInfo;				///< @brief	Specifies the mixer/keyer transition to make.  Ignored if AUTOCIRCULATE_WITH_VIDPROC option is not set.
					NTV2QuarterSizeExpandMode		acVideoQuarterSizeExpand;	///< @brief	Turns on the "quarter-size expand" (2x H + 2x V) hardware. Defaults to off (1:1).

					NTV2_POINTER					acHDMIAuxData;

					/**
						@name	Lesser-used and Deprecated Members
					**/
					///@{
					ULWord							acPeerToPeerFlags;			//// @brief	Used to control P2P transfers.
					ULWord							acFrameRepeatCount;			///< @brief Intended for playout. The number of times to repeat the frame being transferred.
					LWord							acDesiredFrame;				///< @brief	Used to specify a different frame in the circulate ring to transfer to/from.
					NTV2_SHOULD_BE_DEPRECATED(NTV2_RP188		acRP188);		///< @brief	Will be deprecated -- use AUTOCIRCULATE_TRANSFER::SetOutputTimeCode instead.
					NTV2_SHOULD_BE_DEPRECATED(NTV2Crosspoint	acCrosspoint);	///< @brief	Will be deprecated -- used internally by the SDK. Will be removed when the driver changes to use NTV2Channel/NTV2Mode.
					///@}
				NTV2_TRAILER					acTrailer;					///< @brief	The common structure trailer -- ALWAYS LAST!

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@name	Construction & Destruction
				**/
				///@{
				explicit							AUTOCIRCULATE_TRANSFER ();		///< @brief		Constructs a default AUTOCIRCULATE_TRANSFER struct.
													~AUTOCIRCULATE_TRANSFER ();		///< @brief	My default destructor, which frees all allocatable fields that I own.

				/**
					@brief	Constructs an AUTOCIRCULATE_TRANSFER object to use in a CNTV2Card::AutoCirculateTransfer call.
					@param	pInVideoBuffer		Specifies a pointer to the host video buffer. On capture, this buffer will be written during the DMA operation.
												On playout, this buffer will be read during the DMA operation. If NULL, no video will be transferred.
					@param	inVideoByteCount	On capture, specifies the maximum capacity of the host video buffer, in bytes.
												On playout, specifies the number of video bytes to transfer from the host buffer.
												If zero, no video will be transferred.
					@param	pInAudioBuffer		Optionally specifies a pointer to the host audio buffer. On capture, audio data will be DMA'd to this buffer.
												On playout, audio data will be read from this buffer. If NULL, no audio will be transferred.
												Defaults to NULL.
					@param	inAudioByteCount	Optionally specifies the maximum capacity of the host audio buffer, in bytes. If zero, no audio will be transferred.
												Defaults to zero.
					@param	pInANCBuffer		Optionally specifies a pointer to the host ancillary data buffer. On capture, ancillary data will be DMA'd into this buffer.
												On playout, ancillary data will be read from this buffer. If NULL, no Field 1 ancillary data will be transferred.
												Defaults to NULL.
					@param	inANCByteCount		Optionally specifies the maximum capacity of the Field 1 host ancillary data buffer, in bytes.
												If zero, no ancillary data (progressive or interlaced F1) will be transferred.  Defaults to zero.
					@param	pInANCF2Buffer		Optionally specifies a pointer to the Field 2 host ancillary data buffer. On capture, ancillary data will be DMA'd into this buffer.
												On playout, ancillary data will be read from this buffer. If NULL, no ancillary data will be transferred.
												Defaults to NULL.
					@param	inANCF2ByteCount	Optionally specifies the maximum capacity of the Field 2 host ancillary data buffer, in bytes.
												If zero, no ancillary data (interlaced F2) will be transferred.  Defaults to zero.
				**/
				explicit							AUTOCIRCULATE_TRANSFER (ULWord * pInVideoBuffer, const ULWord inVideoByteCount, ULWord * pInAudioBuffer = AJA_NULL,
																			const ULWord inAudioByteCount = 0, ULWord * pInANCBuffer = AJA_NULL, const ULWord inANCByteCount = 0,
																			ULWord * pInANCF2Buffer = AJA_NULL, const ULWord inANCF2ByteCount = 0);
				/**
					@brief		Resets the struct to its initialized state, with timecode capture disabled, freeing all buffers that were allocated by the SDK.
								(Buffers set by the client application are zeroed but not freed.)
				**/
				void								Clear (void);

				NTV2_BEGIN_PRIVATE
					inline explicit					AUTOCIRCULATE_TRANSFER (const AUTOCIRCULATE_TRANSFER & inObj)
																									:	acHeader(0xFEFEFEFE, 0), acVideoBuffer(0), acAudioBuffer(0),
																										acANCBuffer(0), acANCField2Buffer(0), acOutputTimeCodes(0), acHDMIAuxData(0)
																										{(void) inObj;}		///< @brief	You cannot construct an AUTOCIRCULATE_TRANSFER from another.
					inline AUTOCIRCULATE_TRANSFER &	operator = (const AUTOCIRCULATE_TRANSFER & inRHS)	{(void) inRHS; return *this;}	///< @brief	You cannot assign AUTOCIRCULATE_TRANSFERs.
				NTV2_END_PRIVATE
				///@}

				/**
					@name	Buffer Management
				**/
				///@{
				/**
					@brief	Sets my buffers for use in a subsequent call to CNTV2Card::AutoCirculateTransfer.
					@param	pInVideoBuffer		Specifies a pointer to the host video buffer. On capture, this buffer will be written during the DMA operation.
												On playout, this buffer will be read during the DMA operation. If NULL, no video will be transferred.
					@param	inVideoByteCount	On capture, specifies the maximum capacity of the host video buffer, in bytes.
												On playout, specifies the number of video bytes to transfer from the host buffer.
												If zero, no video will be transferred.
					@param	pInAudioBuffer		Specifies a pointer to the host audio buffer. On capture, audio data will be DMA'd to this buffer.
												On playout, audio data will be read from this buffer. If NULL, no audio will be transferred.
					@param	inAudioByteCount	On capture, specifies the maximum capacity of the host audio buffer, in bytes.
												On playout, specifies the number of audio bytes (and therefore audio samples) to transfer from the host buffer.
												If zero, no audio will be transferred.
					@param	pInANCBuffer		Specifies a pointer to the host ancillary data buffer. On capture, ancillary data will be DMA'd into this buffer.
												On playout, ancillary data will be read from this buffer. If NULL, no ancillary data will be transferred.
					@param	inANCByteCount		On capture, specifies the maximum capacity of the host ancillary data buffer, in bytes. After the transfer,
												it will contain the actual number of bytes transferred.
												On playout, specifies the number of ancillary data bytes to transfer from the host buffer.
												If zero, no ancillary data will be transferred.
					@param	pInANCF2Buffer		Optionally specifies a pointer to the Field 2 host ancillary data buffer. On capture, ancillary data for Field 2
												(interlaced video formats only) will be DMA'd into this buffer.
												On playout, ancillary data for Field 2 (interlaced video formats only) will be read from this buffer.
												If NULL, no Field 2 ancillary data will be transferred. Defaults to NULL.
					@param	inANCF2ByteCount	Optionally specifies the maximum capacity of the Field 2 host ancillary data buffer, in bytes.
												If zero, no ancillary data (interlaced F2) will be transferred.  Defaults to zero.
				**/
				bool									SetBuffers (ULWord * pInVideoBuffer, const ULWord inVideoByteCount,
																	ULWord * pInAudioBuffer, const ULWord inAudioByteCount,
																	ULWord * pInANCBuffer, const ULWord inANCByteCount,
																	ULWord * pInANCF2Buffer = AJA_NULL, const ULWord inANCF2ByteCount = 0);

				/**
					@brief	Sets my video buffer for use in a subsequent call to CNTV2Card::AutoCirculateTransfer.
					@param	pInVideoBuffer		Specifies a pointer to the host video buffer. On capture, this buffer will be written during the DMA operation.
												On playout, this buffer will be read during the DMA operation. If NULL, no video data will be transferred.
					@param	inVideoByteCount	Specifies the maximum capacity of the host video buffer, in bytes, or the maximum number of video data bytes to transfer.
												If zero, no video data will be transferred.
					@return	True if successful;  otherwise false.
					@note	Having the \c pInAudioBuffer address start on at least an 8-byte boundary or even better, on a page boundary,
							and the \c inAudioByteCount be a multiple of 8-bytes (or optimally a multiple of a page) increases PCIe DMA
							efficiency on most modern operating systems.
				**/
				bool									SetVideoBuffer (ULWord * pInVideoBuffer, const ULWord inVideoByteCount);

				/**
					@brief	Sets my audio buffer for use in a subsequent call to CNTV2Card::AutoCirculateTransfer.
					@param	pInAudioBuffer		Specifies a pointer to the host audio buffer. On capture, audio data will be DMA'd into this buffer.
												On playout, audio data will be DMA'd from this buffer. If NULL, no audio data will be transferred.
					@param	inAudioByteCount	On capture, specifies the maximum capacity of the host audio buffer, in bytes;  on playout, specifies
												the number of audio bytes (and therefore audio samples) to transfer. If zero, no audio data will be transferred.
					@return	True if successful;  otherwise false.
					@note	Having the \c pInAudioBuffer address start on at least an 8-byte boundary or even better, on a page boundary, increases PCIe
							DMA efficiency on most modern operating systems.
				**/
				bool									SetAudioBuffer (ULWord * pInAudioBuffer, const ULWord inAudioByteCount);

				/**
					@brief	Sets my ancillary data buffers for use in a subsequent call to CNTV2Card::AutoCirculateTransfer.
					@param	pInANCBuffer		Specifies a pointer to the host ancillary data buffer. On capture, ancillary data will be DMA'd into this buffer.
												On playout, ancillary data will be read from this buffer. If NULL, no ancillary data will be transferred.
					@param	inANCByteCount		On capture, specifies the maximum capacity of the host ancillary data buffer, in bytes. After the transfer,
												it will contain the actual number of bytes transferred.
												On playout, specifies the number of ancillary data bytes to transfer from the host buffer.
												If zero, no ancillary data will be transferred.
					@param	pInANCF2Buffer		Optionally specifies a pointer to the Field 2 host ancillary data buffer. On capture, ancillary data for Field 2
												(interlaced video formats only) will be DMA'd into this buffer.
												On playout, ancillary data for Field 2 (interlaced video formats only) will be read from this buffer.
												If NULL, no Field 2 ancillary data will be transferred. Defaults to NULL.
					@param	inANCF2ByteCount	Optionally specifies the maximum capacity of the Field 2 host ancillary data buffer, in bytes, or the maximum
												number of Field 2 ancillary data bytes to transfer. If zero, no F2 ancillary data will be transferred.
												Defaults to zero.
					@note	If using a non-NULL pointer address for either \c pInANCBuffer or \c pInANCF2Buffer, be sure they're aligned to the nearest 8-byte boundary.
					@note	If using a non-zero byte count, AJA recommends using a 2048-byte buffer (per field). There's no need to fill the entire buffer,
							but the data it contains should be compatible with what's documented in \ref anccapture or \ref ancplayout (as appropriate).
					@return	True if successful;  otherwise false.
				**/
				bool									SetAncBuffers (ULWord * pInANCBuffer, const ULWord inANCByteCount,
																		ULWord * pInANCF2Buffer = AJA_NULL, const ULWord inANCF2ByteCount = 0);
				/**
					@return		My video buffer.
				**/
				inline const NTV2_POINTER &				GetVideoBuffer (void) const								{return acVideoBuffer;}

				/**
					@return		My audio buffer.
				**/
				inline const NTV2_POINTER &				GetAudioBuffer (void) const								{return acAudioBuffer;}

				/**
					@param[in]	inField2	Specify true for Field2. Defaults to false (Field1).
					@return		My ancillary data buffer.
				**/
				inline const NTV2_POINTER &				GetAncBuffer (const bool inField2 = false) const		{return inField2 ? acANCField2Buffer : acANCBuffer;}
				///@}

				/**
					@name	Timecode
				**/
				///@{

				/**
					@brief		Intended for playout, replaces the contents of my acOutputTimeCodes member.
					@param[in]	inValues	Specifies the NTV2TimeCodes map to transfer to the device.
					@return		True if successful;  otherwise false.
					@note		The NTV2TimeCodes map should be populated only with timecodes that make sense for the AutoCirculate
								channel that's in use, otherwise the timecode output of other channels may be affected.
					@note		To work right, CNTV2Card::AutoCirculateInitForOutput must have been initialized with the AUTOCIRCULATE_WITH_RP188 option.
				**/
				bool									SetOutputTimeCodes (const NTV2TimeCodes & inValues);

				/**
					@brief		Intended for playout, sets one element of my acOutputTimeCodes member.
					@param[in]	inTimecode	Specifies the timecode value.
					@param[in]	inTCIndex	Specifies which NTV2TCIndex to use. Defaults to NTV2_TCINDEX_SDI1.
					@note		Note that specifying an SDI output destination that's not connected to the framestore associated with the
								channel being AutoCirculated can result in the wrong timecode being transmitted from another AutoCirculate
								channel's SDI output(s).
					@note		To work right, CNTV2Card::AutoCirculateInitForOutput must have been initialized with the AUTOCIRCULATE_WITH_RP188 option.
					@return		True if successful;  otherwise false.
				**/
				bool									SetOutputTimeCode (const NTV2_RP188 & inTimecode, const NTV2TCIndex inTCIndex = NTV2_TCINDEX_SDI1);

				/**
					@brief		Intended for playout, replaces all elements of my acOutputTimeCodes member with the given timecode value.
					@param[in]	inTimecode		Specifies the timecode value to use for all possible embedded (VITC, ATC-LTC) and analog timecode outputs for the device.
					@param[in]	inIncludeF2		Specify true to include Field2 timecode values (VITC2), or false to exclude them. Defaults to true.
					@note		Note that specifying an SDI output destination that's not connected to the framestore associated with the
								AutoCirculate channel can result in the wrong timecode being transmitted from another channel's SDI output(s).
					@return		True if successful;  otherwise false.
				**/
				bool									SetAllOutputTimeCodes (const NTV2_RP188 & inTimecode, const bool inIncludeF2 = true);

				/**
					@brief		Intended for capture, answers with the timecodes captured in my acTransferStatus member's acFrameStamp member.
					@param[out]	outValues	Receives the NTV2TimeCodeList that was transferred from the device.
											(An NTV2TCIndex value can be used as in index in the list.)
					@return		True if successful;  otherwise false.
					@note		To work right, CNTV2Card::AutoCirculateInitForInput must have been initialized with the AUTOCIRCULATE_WITH_RP188 option.
				**/
				bool									GetInputTimeCodes (NTV2TimeCodeList & outValues) const;

				/**
					@brief		Intended for capture, answers with a specific timecode captured in my acTransferStatus member's
								acFrameStamp member's acTimeCodes member.
					@param[out]	outTimeCode		Receives the requested timecode value.
					@param[in]	inTCIndex		Specifies which NTV2TCIndex to use. Defaults to NTV2_TCINDEX_SDI1.
					@note		Note that specifying an SDI input source that's not connected to the framestore associated with the
								channel being AutoCirculated will likely result in a timecode that's not frame-accurate.
					@note		To work right, CNTV2Card::AutoCirculateInitForInput must have been initialized with the AUTOCIRCULATE_WITH_RP188 option.
					@return		True if successful;  otherwise false.
				**/
				bool									GetInputTimeCode (NTV2_RP188 & outTimeCode, const NTV2TCIndex inTCIndex = NTV2_TCINDEX_SDI1) const;

				/**
					@brief		Retrieves the timecodes from the captured frame that are associated with the given SDI input.
					@param[out]	outTimeCodes	Receives the timecode values as a mapping of ::NTV2TCIndex values to ::NTV2_RP188 objects.
					@param[in]	inSDIInput		Specifies the SDI input of interest as an ::NTV2Channel. Defaults to ::NTV2_CHANNEL1 (SDI In 1).
					@param[in]	inValidOnly		Optionally specifies if only valid timecodes should be returned. Defaults to true.
												Specify false to return all timecode values associated with the SDI input of interest,
												even if they're not valid.
					@note		Note that specifying an SDI input source that's not connected to the framestore associated with the
								channel being AutoCirculated will likely result in a timecode that's not frame-accurate.
					@note		To work right, CNTV2Card::AutoCirculateInitForInput must have been initialized with the AUTOCIRCULATE_WITH_RP188 option.
					@return		True if successful;  otherwise false.
				**/
				bool									GetInputTimeCodes (NTV2TimeCodes & outTimeCodes, const NTV2Channel inSDIInput = NTV2_CHANNEL1, const bool inValidOnly = true) const;
				///@}

				/**
					@name	Miscellaneous
				**/
				///@{

				/**
					@brief		Intended for playout, replaces my current acUserCookie value with the new value.
					@param[in]	inUserCookie	Specifies the new cookie value.
					@return		True if successful;  otherwise false.
					@note		This is useful in playout applications that need to know when a specific frame was actually transmitted on-air.
								By calling this method with a unique tag (or application-specific pointer address), and monitoring the FRAME_STAMP
								after calling CNTV2Card::AutoCirculateTransfer or CNTV2Card::AutoCirculateGetFrameStamp, you can tell if/when the
								frame went on-the-air.
				**/
				inline bool								SetFrameUserCookie (const ULWord64 & inUserCookie)		{acInUserCookie = inUserCookie;  return true;}

				/**
					@brief		Sets my acFrameBufferFormat value to the given new value (if valid and circulating with AUTOCIRCULATE_WITH_FBFCHANGE option).
					@param[in]	inNewFormat		Specifies the new frame buffer format value to change to. Must be a valid format for the device.
					@return		True if successful;  otherwise false.
					@note		This function call is ignored if CNTV2Card::AutoCirculateInitForInput or CNTV2Card::AutoCirculateInitForOutput were
								called without the AUTOCIRCULATE_WITH_FBFCHANGE option set.
				**/
				bool									SetFrameBufferFormat (const NTV2FrameBufferFormat inNewFormat);

				/**
					@brief		Enables quarter-size expansion mode.
				**/
				inline void								EnableQuarterSizeExpandMode (void)				{acVideoQuarterSizeExpand = NTV2_QuarterSizeExpandOn;}

				/**
					@brief		Enables quarter-size expansion mode.
				**/
				inline void								DisableQuarterSizeExpandMode (void)				{acVideoQuarterSizeExpand = NTV2_QuarterSizeExpandOff;}

				/**
					@return		True if quarter-size expansion mode is enabled.
				**/
				inline bool								IsQuarterSizeExpandModeEnabled (void) const		{return acVideoQuarterSizeExpand == NTV2_QuarterSizeExpandOn;}

				/**
					@brief	Returns a constant reference to my AUTOCIRCULATE_TRANSFER_STATUS.
					@return	A constant reference to my AUTOCIRCULATE_TRANSFER_STATUS.
				**/
				inline const AUTOCIRCULATE_TRANSFER_STATUS &	GetTransferStatus (void) const			{return acTransferStatus;}

				/**
					@brief	Returns a constant reference to my FRAME_STAMP.
					@return	A constant reference to my FRAME_STAMP.
				**/
				inline const FRAME_STAMP &				GetFrameInfo (void) const						{return acTransferStatus.acFrameStamp;}

				/**
					@return		The number of audio bytes deposited/transferred into the host audio buffer after
								the last successful CNTV2Card::AutoCirculateTransfer.
				**/
				inline ULWord							GetCapturedAudioByteCount (void) const			{return acTransferStatus.GetCapturedAudioByteCount();}

				inline NTV2_DEPRECATED_f(ULWord	GetAudioByteCount (void) const)					{return GetCapturedAudioByteCount ();}	///< @deprecated	Use GetCapturedAudioByteCount instead.

				/**
					@return		The number of ancillary data bytes deposited/transferred into the host anc buffer after
								the last successful CNTV2Card::AutoCirculateTransfer.
					@param[in]	inField2	Specify true to obtain the Field 2 ancillary byte count.
											Specify false (the default) to return the Field 1 (or progessive) ancillary byte count.
				**/
				inline ULWord							GetCapturedAncByteCount (const bool inField2 = false) const	{return acTransferStatus.GetCapturedAncByteCount(inField2);}
				inline NTV2_SHOULD_BE_DEPRECATED(ULWord	GetAncByteCount (const bool inField2 = false) const)	{return GetCapturedAncByteCount(inField2);}	///< @deprecated	Use GetCapturedAncByteCount instead.

				/**
					@return		My current frame buffer format.
				**/
				inline NTV2FrameBufferFormat			GetFrameBufferFormat (void) const						{return acFrameBufferFormat;}

				/**
					@return		The frame number that was transferred (or -1 if failed).
				**/
				inline LWord							GetTransferFrameNumber (void) const						{return acTransferStatus.GetTransferFrame();}
				///@}

				/**
					@name	Segmented DMAs
				**/
				///@{
				/**
					@brief		Enables segmented DMAs given a segment count, a host pitch, and device pitch value.
					@param[in]	inNumSegments				Specifies the number of segments (lines) to DMA. If zero or 1, this actually disables segmented DMAs,
															and the remaining parameters are ignored.
					@param[in]	inNumActiveBytesPerLine		Specifies the number of active bytes in a line of video.
					@param[in]	inHostBytesPerLine			Specifies the offset, in bytes, between two adjacent segments on the host.
					@param[in]	inDeviceBytesPerLine		Specifies the offset, in bytes, between two adjacent segments on the device.
					@return		True if successful;  otherwise false.
					@note		IMPORTANT:  For segmented DMAs, the AUTOCIRCULATE_TRANSFER::acVideoBuffer.fByteCount field holds the segment byte count (i.e.,
											the number of bytes to transfer per segment.
					@todo		Add a new \c inNumBytesPerSegment parameter to eliminate the necessity of setting AUTOCIRCULATE_TRANSFER::acVideoBuffer.fByteCount
								to store the bytes-per-segment value.
				**/
				bool									EnableSegmentedDMAs (const ULWord inNumSegments, const ULWord inNumActiveBytesPerLine,
																				const ULWord inHostBytesPerLine, const ULWord inDeviceBytesPerLine);
				/**
					@brief		Disables segmented DMAs, performing a Reset on my acInSegmentedDMAInfo.
					@return		True if successful;  otherwise false.
				**/
				bool									DisableSegmentedDMAs (void);

				/**
					@return		True if segmented DMAs are currently enabled;  otherwise false.
				**/
				bool									SegmentedDMAsEnabled (void) const;
				///@}

				NTV2_IS_STRUCT_VALID_IMPL(acHeader,acTrailer)
			#endif	//	user-space clients only
		NTV2_STRUCT_END (AUTOCIRCULATE_TRANSFER)


        /**
            @brief	This is used to enable or disable AJADebug logging in the driver.
            @note	This struct uses a constructor to properly initialize itself. Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
        **/
        NTV2_STRUCT_BEGIN (NTV2DebugLogging)
            NTV2_HEADER		mHeader;			///< @brief	The common structure header -- ALWAYS FIRST!
                NTV2_POINTER	mSharedMemory;		///< @brief	Virtual address of AJADebug shared memory in calling process' context,
													//			and its length. The AJADebug logging facility owns and manages this memory.
													//			If NULL or zero length, debug logging will be disabled in the driver.
													//			If non-NULL and zero length, debug logging will be enabled in the driver.
                ULWord			mReserved[32];		///< @brief	Reserved for future expansion.
            NTV2_TRAILER	mTrailer;			///< @brief	The common structure trailer -- ALWAYS LAST!

            #if !defined (NTV2_BUILDING_DRIVER)
                /**
                    @brief	Constructs an NTV2DebugLogging struct.
                    @param[in]	inEnable            False to disable (the default), or True to enable.
                **/
                explicit	NTV2DebugLogging (const bool inEnable = false);

                /**
                    @brief	Prints a human-readable representation of me to the given output stream.
                    @param	inOutStream		Specifies the output stream to use.
                    @return	A reference to the output stream.
                **/
                std::ostream &	Print (std::ostream & inOutStream) const;

				NTV2_IS_STRUCT_VALID_IMPL(mHeader, mTrailer)

            #endif	//	!defined (NTV2_BUILDING_DRIVER)
        NTV2_STRUCT_END (NTV2DebugLogging)


		/**
			@brief	This is used to prelock a video/audio/anc buffer used as the source or target of DMA transfers.
					The driver will page-lock the buffer immediately, so it won't have to be done for each DMA transfer.
					This will reduce transfer time and CPU overhead at the cost of locking physical memory.
			@note	This struct uses a constructor to properly initialize itself.
					Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2BufferLock)
			NTV2_HEADER		mHeader;			///< @brief	The common structure header -- ALWAYS FIRST!
				NTV2_POINTER	mBuffer;			///< @brief	Virtual address of a buffer to prelock, and its length.
													//			A NULL buffer (or zero length) releases all locked buffers.
				ULWord			mFlags;				///< @brief Action flags (lock, unlock, etc)
				ULWord64		mMaxLockSize;		///< @brief Max locked bytes.
				ULWord			mReserved[30];		///< @brief	Reserved for future expansion.
			NTV2_TRAILER	mTrailer;			///< @brief	The common structure trailer -- ALWAYS LAST!

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@name	Construction & Destruction
				**/
				///@{
				explicit	NTV2BufferLock ();		///< @brief	Constructs a default NTV2BufferLock struct.
				inline		~NTV2BufferLock ()	{}	///< @brief	My default destructor, which frees all allocatable fields that I own.

				/**
					@brief	Constructs an NTV2BufferLock object to use in a CNTV2Card::DMABufferLock call.
					@param	inBuffer		Specifies the memory to be locked for DMA operations.
					@param	inFlags			Specifies action flags (e.g. ::DMABUFFERLOCK_LOCK, etc.).
				**/
				explicit	NTV2BufferLock (const NTV2_POINTER & inBuffer, const ULWord inFlags);

				/**
					@brief	Constructs an NTV2BufferLock object to use in a CNTV2Card::DMABufferLock call.
					@param	pInBuffer		Specifies a pointer to the host buffer. This buffer will be locked for DMA operations.
					@param	inByteCount		Specifies a the length of the buffer to lock in bytes.
					@param	inFlags			Specifies action flags (lock, unlock, etc)
				**/
				explicit	NTV2BufferLock (const ULWord * pInBuffer, const ULWord inByteCount, const ULWord inFlags);

				/**
					@brief	Constructs an NTV2BufferLock object to use in a CNTV2Card::DMABufferLock call.
					@param	inMaxLockSize	Specifies the maximum lock size in bytes
					@param	inFlags			Specifies action flags (lock, unlock, etc)
				**/
				explicit	NTV2BufferLock (const ULWord64 inMaxLockSize, const ULWord inFlags);	//	New in SDK 16.0
///@}

				/**
					@name	Changing
				**/
				///@{
				/**
					@brief	Sets the buffer to lock for use in a subsequent call to CNTV2Card::DMABufferLock.
					@param	inBuffer		Specifies the memory to be locked for DMA operations.
					@return	True if successful;  otherwise false.
				**/
				bool		SetBuffer (const NTV2_POINTER & inBuffer);

				/**
					@brief	Sets the buffer to lock for use in a subsequent call to CNTV2Card::DMABufferLock.
					@param	pInBuffer			Specifies a pointer to the host buffer. This buffer will be locked for DMA operations.
					@param	inByteCount			Specifies a the length of the buffer to lock in bytes.
					@return	True if successful;  otherwise false.
				**/
				inline bool	SetBuffer (const ULWord * pInBuffer, const ULWord inByteCount)
				{
					return SetBuffer(NTV2_POINTER(pInBuffer, inByteCount));
				}

				/**
					@brief	Sets the action flags for use in a subsequent call to CNTV2Card::DMABufferLock.
					@param	inFlags			Specifies action flags (lock, unlock, etc)
				**/
				inline void	SetFlags (const ULWord inFlags)		{NTV2_ASSERT_STRUCT_VALID;  mFlags = inFlags;}

				/**
					@brief	Sets the maximum lock size for use in a subsequent call to CNTV2Card::DMABufferLock.
					@param	inNumBytes		Specifies maximum lock size, in bytes.
				**/
				inline void	SetMaxLockSize (const ULWord64 inNumBytes)		{NTV2_ASSERT_STRUCT_VALID;  mMaxLockSize = inNumBytes;}	//	New in SDK 16.0

				/**
					@brief	Resets the struct to its initialized state.
					@note	This does not release locked buffers.
				**/
				inline void	Clear (void)
				{
					SetBuffer(NTV2_POINTER());
					SetFlags(0);
					SetMaxLockSize(0);
				}
				///@}

				/**
					@brief	Prints a human-readable representation of me to the given output stream.
					@param	inOutStream		Specifies the output stream to use.
					@return	A reference to the output stream.
				**/
				std::ostream &	Print (std::ostream & inOutStream) const;

				NTV2_IS_STRUCT_VALID_IMPL(mHeader, mTrailer)

			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (NTV2BufferLock)


		/**
			@brief	This is used for bitstream maintainance.	(New in SDK 16.0)
			@note	This struct uses a constructor to properly initialize itself.
					Do not use <b>memset</b> or <b>bzero</b> to initialize or "clear" it.
		**/
		NTV2_STRUCT_BEGIN (NTV2Bitstream)
			NTV2_HEADER		mHeader;			///< @brief	The common structure header -- ALWAYS FIRST!
				NTV2_POINTER	mBuffer;			///< @brief	Virtual address of a bitstream buffer and its length.
				ULWord			mFlags;				///< @brief Action flags (lock, unlock, etc)
				ULWord			mStatus;			///< @brief Action status
				ULWord			mRegisters[16];		///< @brief Resister data
				ULWord			mReserved[32];		///< @brief	Reserved for future expansion.
			NTV2_TRAILER	mTrailer;			///< @brief	The common structure trailer -- ALWAYS LAST!

			#if !defined (NTV2_BUILDING_DRIVER)
				/**
					@name	Construction & Destruction
				**/
				///@{
				explicit	NTV2Bitstream ();		///< @brief	Constructs a default NTV2Bitstream struct.
				inline		~NTV2Bitstream ()	{}	///< @brief	My default destructor, which frees all allocatable fields that I own.

				/**
					@brief	Constructs an NTV2Bitstream object to use in a CNTV2Card::LoadBitstream call.
					@param	inBuffer		Specifies the memory containing the bitstream to load.
					@param	inFlags			Specifies action flags (fragment swap, etc.).
				**/
				explicit	NTV2Bitstream (const NTV2_POINTER & inBuffer, const ULWord inFlags);

				/**
					@brief	Constructs an NTV2Bitstream object to use in a CNTV2Card::LoadBitstream call.
					@param	pInBuffer		Specifies a pointer to the host buffer containing the bitstream to load.
					@param	inByteCount		Specifies a the length of the bitstream in bytes.
					@param	inFlags			Specifies action flags (fragment, swap, etc)
				**/
				explicit	NTV2Bitstream (const ULWord * pInBuffer, const ULWord inByteCount, const ULWord inFlags);
				///@}

				/**
					@name	Changing
				**/
				///@{
				/**
					@brief	Sets the buffer to lock for use in a subsequent call to CNTV2Card::LoadBitstream.
					@param	inBuffer		Specifies the memory containing the bitstream to load.
					@return	True if successful;  otherwise false.
				**/
				bool		SetBuffer (const NTV2_POINTER & inBuffer);

				/**
					@brief	Sets the buffer to lock for use in a subsequent call to CNTV2Card::LoadBitstream.
					@param	pInBuffer			Specifies a pointer to the host buffer contiaining the bitstread to load.
					@param	inByteCount			Specifies a the length of the buffer to load in bytes.
					@return	True if successful;  otherwise false.
				**/
				inline bool	SetBuffer (const ULWord * pInBuffer, const ULWord inByteCount)	{return SetBuffer(NTV2_POINTER(pInBuffer, inByteCount));}

				/**
					@brief	Sets the action flags for use in a subsequent call to CNTV2Card::LoadBitstream.
					@param	inFlags			Specifies action flags (fragment, swap, etc)
				**/
				inline void	SetFlags (const ULWord inFlags)		{NTV2_ASSERT_STRUCT_VALID;  mFlags = inFlags;}

				/**
					@brief	Resets the struct to its initialized state.
				**/
				inline void	Clear (void)		{SetBuffer(NTV2_POINTER());}
				///@}

				/**
					@brief	Prints a human-readable representation of me to the given output stream.
					@param	inOutStream		Specifies the output stream to use.
					@return	A reference to the output stream.
				**/
				std::ostream &	Print (std::ostream & inOutStream) const;

				NTV2_IS_STRUCT_VALID_IMPL(mHeader, mTrailer)

			#endif	//	!defined (NTV2_BUILDING_DRIVER)
		NTV2_STRUCT_END (NTV2Bitstream)


		#if !defined (NTV2_BUILDING_DRIVER)
			typedef std::set <NTV2VideoFormat>					NTV2VideoFormatSet;					///< @brief	A set of distinct NTV2VideoFormat values.
			typedef NTV2VideoFormatSet::const_iterator			NTV2VideoFormatSetConstIter;		///< @brief	A handy const iterator for iterating over an NTV2VideoFormatSet.

			typedef std::set <NTV2FrameBufferFormat>			NTV2FrameBufferFormatSet;			///< @brief	A set of distinct NTV2FrameBufferFormat values.
			typedef NTV2FrameBufferFormatSet::const_iterator	NTV2FrameBufferFormatSetConstIter;	///< @brief	A handy const iterator for iterating over an NTV2FrameBufferFormatSet.

			typedef std::set <NTV2FrameGeometry>				NTV2GeometrySet;					///< @brief	A set of distinct NTV2FrameGeometry values.
			typedef NTV2GeometrySet::const_iterator				NTV2GeometrySetConstIter;			///< @brief	A handy const iterator for iterating over an NTV2GeometrySet.

			typedef std::set <NTV2Standard>						NTV2StandardSet;					///< @brief	A set of distinct NTV2Standard values.
			typedef NTV2StandardSet::const_iterator				NTV2StandardSetConstIter;			///< @brief	A handy const iterator for iterating over an NTV2StandardSet.

			typedef std::set <NTV2InputSource>					NTV2InputSourceSet;					///< @brief	A set of distinct NTV2InputSource values.
			typedef NTV2InputSourceSet::const_iterator			NTV2InputSourceSetConstIter;		///< @brief	A handy const iterator for iterating over an NTV2InputSourceSet.

			typedef std::set <NTV2OutputDestination>			NTV2OutputDestinations;				///< @brief	A set of distinct NTV2OutputDestination values.
			typedef NTV2OutputDestinations::const_iterator		NTV2OutputDestinationsConstIter;	///< @brief	A handy const iterator for iterating over an NTV2OutputDestinations.

			/**
				@brief		Prints the given ::UWordSequence contents into the given output stream.
				@param		inOutStream		The stream into which the given ::UWordSequence will be printed.
				@param[in]	inData			Specifies the ::UWordSequence to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const UWordSequence & inData);

			/**
				@brief		Prints the given ::ULWordSequence contents into the given output stream.
				@param		inOutStream		The stream into which the given ::ULWordSequence will be printed.
				@param[in]	inData			Specifies the ::ULWordSequence to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const ULWordSequence & inData);

			/**
				@brief		Prints the given ::ULWord64Sequence contents into the given output stream.
				@param		inOutStream		The stream into which the given ::ULWord64Sequence will be printed.
				@param[in]	inData			Specifies the ::ULWord64Sequence to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const ULWord64Sequence & inData);

			/**
				@return		A string that contains the human-readable representation of the given ::NTV2AutoCirculateState value.
				@param[in]	inState		Specifies the ::NTV2AutoCirculateState of interest.
			**/
			AJAExport std::string NTV2AutoCirculateStateToString (const NTV2AutoCirculateState inState);

			/**
				@brief	Returns a set of distinct ::NTV2VideoFormat values supported on the given device.
				@param[in]	inDeviceID	Specifies the ::NTV2DeviceID of the device of interest.
				@param[out]	outFormats	Receives the set of distinct ::NTV2VideoFormat values supported by the device.
				@return		True if successful;  otherwise false.
				@todo	This needs to be moved to a C++ compatible "device features" module.
			**/
			AJAExport bool NTV2DeviceGetSupportedVideoFormats (const NTV2DeviceID inDeviceID, NTV2VideoFormatSet & outFormats);

			/**
				@brief		Prints the given ::NTV2VideoFormatSet contents into the given output stream.
				@param		inOStream	The stream into which the human-readable list will be written.
				@param[in]	inFormats	Specifies the set of video formats to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOStream, const NTV2VideoFormatSet & inFormats);

			/**
				@brief	Returns a set of distinct ::NTV2FrameBufferFormat values supported on the given device.
				@param[in]	inDeviceID	Specifies the ::NTV2DeviceID of the device of interest.
				@param[out]	outFormats	Receives the set of distinct ::NTV2FrameBufferFormat values supported by the device.
				@return		True if successful;  otherwise false.
				@todo	This needs to be moved to a C++ compatible "device features" module.
			**/
			AJAExport bool NTV2DeviceGetSupportedPixelFormats (const NTV2DeviceID inDeviceID, NTV2FrameBufferFormatSet & outFormats);

			/**
				@brief		Prints the given ::NTV2FrameBufferFormatSet contents into the given output stream.
				@param		inOStream	The stream into which the human-readable list will be written.
				@param[in]	inFormats	Specifies the set of pixel formats to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOStream, const NTV2FrameBufferFormatSet & inFormats);

			/**
				@brief		Appends the given ::NTV2FrameBufferFormatSet contents into the given set.
				@param		inOutSet	The set to which the other set will be appended.
				@param[in]	inSet		Specifies the set whose contents will be appended.
				@return		A reference to the modified set.
			**/
			AJAExport NTV2FrameBufferFormatSet & operator += (NTV2FrameBufferFormatSet & inOutSet, const NTV2FrameBufferFormatSet inSet);

			/**
				@brief	Returns a set of distinct ::NTV2Standard values supported on the given device.
				@param[in]	inDeviceID		Specifies the ::NTV2DeviceID of the device of interest.
				@param[out]	outStandards	Receives the set of distinct ::NTV2Standard values supported by the device.
				@return		True if successful;  otherwise false.
				@todo	This needs to be moved to a C++ compatible "device features" module.
			**/
			AJAExport bool NTV2DeviceGetSupportedStandards (const NTV2DeviceID inDeviceID, NTV2StandardSet & outStandards);

			/**
				@brief		Prints the given ::NTV2StandardSet contents into the given output stream.
				@param		inOStream		The stream into which the human-readable list will be written.
				@param[in]	inStandards		Specifies the set of ::NTV2Standard values to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOStream, const NTV2StandardSet & inStandards);

			/**
				@brief		Appends the given ::NTV2StandardSet contents into the given set.
				@param		inOutSet	The set to which the other set will be appended.
				@param[in]	inSet		Specifies the set whose contents will be appended.
				@return		A reference to the modified set.
			**/
			AJAExport NTV2StandardSet & operator += (NTV2StandardSet & inOutSet, const NTV2StandardSet inSet);

			/**
				@brief		Returns a set of distinct ::NTV2FrameGeometry values supported on the given device.
				@param[in]	inDeviceID		Specifies the ::NTV2DeviceID of the device of interest.
				@param[out]	outGeometries	Receives the set of distinct ::NTV2FrameGeometry values supported by the device.
				@return		True if successful;  otherwise false.
				@todo		Needs to be moved to a C++ compatible "device features" module.
			**/
			AJAExport bool NTV2DeviceGetSupportedGeometries (const NTV2DeviceID inDeviceID, NTV2GeometrySet & outGeometries);

			/**
				@brief		Prints the given ::NTV2GeometrySet contents into the given output stream.
				@param		inOStream		The stream into which the human-readable list will be written.
				@param[in]	inGeometries	Specifies the set of ::NTV2FrameGeometry values to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOStream, const NTV2GeometrySet & inGeometries);

			/**
				@brief		Appends the given ::NTV2GeometrySet contents into the given set.
				@param		inOutSet	The set to which the other set will be appended.
				@param[in]	inSet		Specifies the set whose contents will be appended.
				@return		A reference to the modified set.
			**/
			AJAExport NTV2GeometrySet & operator += (NTV2GeometrySet & inOutSet, const NTV2GeometrySet inSet);

			/**
				@brief		Prints the given ::NTV2InputSourceSet contents into the given output stream.
				@param		inOStream	The stream into which the human-readable list will be written.
				@param[in]	inSet		Specifies the set to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOStream, const NTV2InputSourceSet & inSet);

			/**
				@brief		Appends the given ::NTV2InputSourceSet's contents into the given set.
				@param		inOutSet	The set to which the other set will be appended.
				@param[in]	inSet		Specifies the set whose contents will be appended.
				@return		A reference to the modified set.
			**/
			AJAExport NTV2InputSourceSet & operator += (NTV2InputSourceSet & inOutSet, const NTV2InputSourceSet & inSet);

			/**
				@brief		Prints the given ::NTV2OutputDestinations' contents into the given output stream.
				@param		inOStream	The stream into which the human-readable list will be written.
				@param[in]	inSet		Specifies the set to be streamed.
				@return		The "inOStream" that was specified.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOStream, const NTV2OutputDestinations & inSet);	//	New in SDK 16.0

			/**
				@brief		Appends the given ::NTV2OutputDestinations' contents into the given set.
				@param		inOutSet	The set to which the other set will be appended.
				@param[in]	inSet		Specifies the set whose contents will be appended.
				@return		A reference to the modified set.
			**/
			AJAExport NTV2OutputDestinations & operator += (NTV2OutputDestinations & inOutSet, const NTV2OutputDestinations & inSet);	//	New in SDK 16.0


			/**
				@brief	Streams the given ::NTV2_HEADER to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2_HEADER to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2_HEADER & inObj);

			/**
				@brief	Streams the given ::NTV2_TRAILER to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2_TRAILER to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2_TRAILER & inObj);

			/**
				@brief	Streams the given ::NTV2_POINTER to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2_POINTER to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2_POINTER & inObj);

			/**
				@brief	Streams the given ::NTV2_RP188 struct to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2_RP188 struct to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2_RP188 & inObj);

			/**
				@brief	Streams the given ::NTV2TimeCodeList to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2TimeCodeList to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2TimeCodeList & inObj);

			/**
				@brief	Streams the given ::NTV2TimeCodes map to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2TimeCodes map to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2TimeCodes & inObj);

			/**
				@brief	Streams the given ::NTV2TCIndexes to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2TCIndexes set to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2TCIndexes & inObj);

			/**
				@brief		Appends the given ::NTV2TCIndexes contents into the given set.
				@param		inOutSet	The set to which the other set will be appended.
				@param[in]	inSet		Specifies the set whose contents will be appended.
				@return		A reference to the modified set.
			**/
			AJAExport NTV2TCIndexes & operator += (NTV2TCIndexes & inOutSet, const NTV2TCIndexes & inSet);

			/**
				@brief	Streams the given ::FRAME_STAMP to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::FRAME_STAMP to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const FRAME_STAMP & inObj);

			/**
				@brief	Streams the given ::AUTOCIRCULATE_STATUS to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::AUTOCIRCULATE_STATUS to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const AUTOCIRCULATE_STATUS & inObj);

			/**
				@brief	Streams the given ::NTV2SegmentedDMAInfo to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2SegmentedDMAInfo to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2SegmentedDMAInfo & inObj);

			/**
				@brief	Streams the given ::AUTOCIRCULATE_TRANSFER to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::AUTOCIRCULATE_TRANSFER to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const AUTOCIRCULATE_TRANSFER & inObj);

			/**
				@brief	Streams the given ::FRAME_STAMP to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::FRAME_STAMP to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const FRAME_STAMP & inObj);

			/**
				@brief	Streams the given ::AUTOCIRCULATE_TRANSFER_STATUS to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::AUTOCIRCULATE_TRANSFER_STATUS to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const AUTOCIRCULATE_TRANSFER_STATUS & inObj);

			/**
				@brief	Streams the given ::NTV2RegisterNumberSet to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2RegisterNumberSet to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2RegisterNumberSet & inObj);

			/**
				@brief	Streams the given ::NTV2RegisterValueMap to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2RegisterValueMap to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2RegisterValueMap & inObj);

			/**
				@brief	Streams the given ::AutoCircVidProcInfo to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::AutoCircVidProcInfo to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const AutoCircVidProcInfo & inObj);

			/**
				@brief	Streams the given ::NTV2ColorCorrectionData to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2ColorCorrectionData to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2ColorCorrectionData & inObj);

			/**
				@brief	Streams the given ::NTV2GetRegisters to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2GetRegisters to be streamed.
				@return	The ostream being used.
			**/
			AJAExport inline std::ostream & operator << (std::ostream & inOutStream, const NTV2GetRegisters & inObj)	{return inObj.Print (inOutStream);}

			/**
				@brief	Streams the given ::NTV2SetRegisters to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2SetRegisters to be streamed.
				@return	The ostream being used.
			**/
			AJAExport inline std::ostream & operator << (std::ostream & inOutStream, const NTV2SetRegisters & inObj)		{return inObj.Print (inOutStream);}

			/**
				@brief	Streams the given ::NTV2BankSelGetSetRegs to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2BankSelGetSetRegs to be streamed.
				@return	The ostream being used.
			**/
			AJAExport inline std::ostream & operator << (std::ostream & inOutStream, const NTV2BankSelGetSetRegs & inObj)	{return inObj.Print (inOutStream);}

			/**
				@brief	Streams the given ::NTV2SDIInStatistics to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2SDIInStatistics to be streamed.
				@return	The ostream being used.
			**/
			AJAExport inline std::ostream & operator << (std::ostream & inOutStream, const NTV2SDIInStatistics & inObj)	{return inObj.Print (inOutStream);}

			/**
				@brief	Streams the given ::NTV2SDIInputStatus to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the ::NTV2SDIInputStatus to be streamed.
				@return	The ostream being used.
			**/
			AJAExport inline std::ostream &	operator << (std::ostream & inOutStream, const NTV2SDIInputStatus & inObj)	{return inObj.Print (inOutStream);}

			/**
				@brief	Streams the given ::NTV2SegmentedXferInfo to the specified ostream in a human-readable format.
				@param		inOutStrm		Specifies the ostream to use.
				@param[in]	inXferInfo		Specifies the ::NTV2SegmentedXferInfo to be streamed.
				@return	The ostream being used.
			**/
			AJAExport std::ostream & operator << (std::ostream & inOutStrm, const NTV2SegmentedXferInfo & inXferInfo);

			/**
				@brief	Streams the given NTV2DebugLogging struct to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the NTV2DebugLogging to be streamed.
				@return	The ostream being used.
			**/
			AJAExport inline std::ostream &	operator << (std::ostream & inOutStream, const NTV2DebugLogging & inObj)	{return inObj.Print (inOutStream);}

			/**
				@brief	Streams the given NTV2BufferLock struct to the specified ostream in a human-readable format.
				@param		inOutStream		Specifies the ostream to use.
				@param[in]	inObj			Specifies the NTV2BufferLock to be streamed.
				@return	The ostream being used.
			**/
			AJAExport inline std::ostream &	operator << (std::ostream & inOutStream, const NTV2BufferLock & inObj)	{return inObj.Print (inOutStream);}
		#endif	//	!defined (NTV2_BUILDING_DRIVER)

		#if defined (AJAMac)
			#pragma pack (pop)
		#endif	//	defined (AJAMac)
//////////////////////////////////////////////////////////////////////////////////////////////	END NEW AUTOCIRCULATE API


// maximum number of hevc streams
#define HEVC_STREAM_MAX						4

// maximum number of gpio ports
#define HEVC_GPIO_MAX						64

// version string maximum size (bytes)
#define HEVC_VERSION_STRING_SIZE			64

// picture and encoded information additional data size (bytes)
#define HEVC_ADDITIONAL_DATA_SIZE			((4 + 4 + 256) * 16)

// codec state flags 
#define HEVC_STATE_FLAG_VIDEO_STARTED		0x00000001U			// codec video input capture started

// transfer flags 
#define HEVC_TRANSFER_FLAG_IS_LAST_FRAME	0x00000001U			// last stream frame

// driver io status codes
#define HEVC_STATUS_SUCCESS					0x00000001U

// fatal error registers
#define HEVC_FATAL_ERROR_INFO_REG			0x08000100U			// codec error register base
#define HEVC_FATAL_ERROR_INFO_COUNT			64					// number of codec error registers

// driver debug register output enable bits
#define HEVC_DEBUG_DRIVER_REGISTER			0x080000FCU			// register address of debug bits
#define HEVC_DEBUG_MASK_INFO				0x00000001			// general probe and cleanup
#define HEVC_DEBUG_MASK_WARNING				0x00000002			// general warinings
#define HEVC_DEBUG_MASK_ERROR				0x00000004			// general erros
#define HEVC_DEBUG_MASK_INT_PRIMARY			0x00000008			// primary interrupt info
#define HEVC_DEBUG_MASK_INT_COMMAND			0x00000010			// command tasklet info
#define HEVC_DEBUG_MASK_INT_VEI				0x00000020			// raw stream tasklet info
#define HEVC_DEBUG_MASK_INT_SEO				0x00000040			// encoded stream tasklet info
#define HEVC_DEBUG_MASK_INT_ERROR			0x00000080			// interrupt errors
#define HEVC_DEBUG_MASK_REGISTER_INFO		0x00000100			// register read/write info
#define HEVC_DEBUG_MASK_REGISTER_STATE		0x00000200			// detailed register access info
#define HEVC_DEBUG_MASK_REGISTER_ERROR		0x00000400			// register access errors
#define HEVC_DEBUG_MASK_COMMAND_INFO		0x00000800			// command queue info
#define HEVC_DEBUG_MASK_COMMAND_STATE		0x00001000			// detailed command processing info
#define HEVC_DEBUG_MASK_COMMAND_ERROR		0x00002000			// command queue errors
#define HEVC_DEBUG_MASK_STREAM_INFO			0x00004000			// stream (dma) queue info
#define HEVC_DEBUG_MASK_STREAM_STATE		0x00008000			// detailed stream processing info
#define HEVC_DEBUG_MASK_STREAM_COPY			0x00010000			// stream data copy info
#define HEVC_DEBUG_MASK_STREAM_SEGMENT		0x00020000			// stream data segment info
#define HEVC_DEBUG_MASK_STREAM_FRAME		0x00040000			// stream vif frame info
#define HEVC_DEBUG_MASK_STREAM_ERROR		0x00080000			// stream queue errors
#define HEVC_DEBUG_MASK_MEMORY_ALLOC		0x00100000			// buffer memory allocation info
#define HEVC_DEBUG_MASK_MEMORY_ERROR		0x00200000			// buffer memory allocation errors
#define HEVC_DEBUG_MASK_DMA_INFO			0x00400000			// dma send info
#define HEVC_DEBUG_MASK_DMA_DESCRIPTOR		0x00800000			// dma descriptor dump
#define HEVC_DEBUG_MASK_DMA_ERROR			0x01000000			// dma errors
#define HEVC_DEBUG_MASK_STATUS_INFO			0x02000000			// status info requests
#define HEVC_DEBUG_MASK_RESERVED_0			0x04000000
#define HEVC_DEBUG_MASK_RESERVED_1			0x08000000
#define HEVC_DEBUG_MASK_RESERVED_2			0x10000000
#define HEVC_DEBUG_MASK_RESERVED_3			0x20000000
#define HEVC_DEBUG_MASK_RESERVED_4			0x40000000
#define HEVC_DEBUG_MASK_RESERVED_5			0x80000000

// ntv2 gpio input registers
#define HEVC_NTV2_GPIO_REGISTER_LOW			510
#define HEVC_NTV2_GPIO_REGISTER_HIGH		511


// hevc version information
typedef struct HevcVersion
{
	ULWord					major;
	ULWord					minor;
	ULWord					point;
	ULWord					build;
} HevcVersion;

// pci id information
typedef struct HevcPciId
{
	ULWord					vendor;
	ULWord					device;
	ULWord					subVendor;
	ULWord					subDevice;
} HevcPciId;

// hevc device mode
typedef enum HevcDeviceMode
{
	Hevc_DeviceMode_Unknown,
	Hevc_DeviceMode_Codec,							// codec mode
	Hevc_DeviceMode_Maintenance,					// maintenance mode
	Hevc_DeviceMode_Size
} HevcDeviceMode;

// hevc device information message
typedef struct HevcDeviceInfo
{
	HevcVersion				driverVersion;			// driver version
	HevcVersion				mcpuVersion;			// firmware versions
	char					systemFirmware[HEVC_VERSION_STRING_SIZE];
	char					standardFirmwareSingle[HEVC_VERSION_STRING_SIZE];
	char					standardFirmwareMultiple[HEVC_VERSION_STRING_SIZE];
	char					userFirmwareSingle[HEVC_VERSION_STRING_SIZE];
	char					userFirmwareMultiple[HEVC_VERSION_STRING_SIZE];
	HevcPciId				pciId;					// pci ids
	HevcDeviceMode			deviceMode;				// hardware device mode
	bool					mcpuVersionCheck;		// mcpu version supported 
	bool					systemVersionCheck;		// system version supported
	bool					standardSingleCheck;	// standard firmware single version supported
	bool					standardMultipleCheck;	// standard fimwrare multiple version supported
	bool					pciIdCheck;				// pci id supported
} HevcDeviceInfo;

// hevc register
typedef struct HevcDeviceRegister
{
    ULWord					address;                // register address
    ULWord					writeValue;             // register write value
    ULWord					readValue;              // register read value
    ULWord					mask;                   // register value mask
    ULWord					shift;                  // register value shift
    bool					write;                  // write flag
    bool					read;                   // read flag
	bool					forceBar4;				// force bar4 access
} HevcDeviceRegister;

// hevc main state
typedef enum HevcMainState
{
	Hevc_MainState_Unknown,
	Hevc_MainState_Boot,							// codec has booted
	Hevc_MainState_Init,							// initialize codec
	Hevc_MainState_Encode,							// configure encoding (load firmware?)
	Hevc_MainState_Error,							// codec must be reset
	Hevc_MainState_Size
} HevcMainState;

// encoder mode
typedef enum HevcEncodeMode
{
	Hevc_EncodeMode_Unknown,
	Hevc_EncodeMode_Single,							// encode a sigle stream
	Hevc_EncodeMode_Multiple,						// encode multiple streams
	Hevc_EncodeMode_Size
} HevcEncodeMode;

// encoder firmware type
typedef enum HevcFirmwareType
{
	Hevc_FirmwareType_Unknown,
	Hevc_FirmwareType_Standard,						// encode firmware standard
	Hevc_FirmwareType_User,							// encode firmware user
	Hevc_FirmwareType_Size
} HevcFirmwareType;

// hevc video interface state
typedef enum HevcVifState
{
	Hevc_VifState_Unknown,
	Hevc_VifState_Stop,								// video interface stop
	Hevc_VifState_Start,							// video interface start
	Hevc_VifState_Size
} HevcVifState;

// hevc video input state
typedef enum HevcVinState
{
	Hevc_VinState_Unknown,
	Hevc_VinState_Stop,								// video input stop
	Hevc_VinState_Start,							// video input start
	Hevc_VinState_Size
} HevcVinState;

// hevc encoder state
typedef enum HevcEhState
{
	Hevc_EhState_Unknown,
	Hevc_EhState_Stop,								// encoder stop
	Hevc_EhState_Start,								// encoder start
	Hevc_EhState_ReadyToStop,						// encoder ready to stop
	Hevc_EhState_Size
} HevcEhState;

// hevc gpio control
typedef enum HevcGpioControl
{
	Hevc_GpioControl_Unknown,
	Hevc_GpioControl_Function,						// configure gpio port function
	Hevc_GpioControl_Direction,						// configure gpio port direction
	Hevc_GpioControl_Set,							// set gpio port value
	Hevc_GpioControl_Get,							// get pgio port value
	Hevc_GpioControl_Size
} HevcGpioControl;

// hevc gpio function
typedef enum HevcGpioFunction
{
	Hevc_GpioFunction_Unknown,
	Hevc_GpioFunction_Gpio,							// gpio function is gpio
	Hevc_GpioFunction_Peripheral,					// gpio function is peripheral
	Hevc_GpioFunction_Size
} HevcGpioFunction;

// hevc gpio direction
typedef enum HevcGpioDirection
{
	Hevc_GpioDirection_Unknown,
	Hevc_GpioDirection_Input,						// gpio direction is input
	Hevc_GpioDirection_Output,						// gpio direction is output
	Hevc_GpioDirection_Size
} HevcGpioDirection;

// hevc gpio value
typedef enum HevcGpioValue
{
	Hevc_GpioValue_Unknown,
	Hevc_GpioValue_Low,								// gpio direction is input
	Hevc_GpioValue_High,							// gpio direction is output
	Hevc_GpioValue_Size
} HevcGpioValue;

typedef enum HevcChangeSequence
{
    Hevc_ChangeSequence_Unknown,
    Hevc_ChangeSequence_Enabled,
    Hevc_ChangeSequence_Disabled,
    Hevc_ChangeSequence_Size
} HevcChangeSequence;

// hevc change param target
#define Hevc_ParamTarget_None			0x00000000
#define Hevc_ParamTarget_Vbr			0x00000001	// change variable bitrate
#define Hevc_ParamTarget_Cbr			0x00000002	// change constant bitrate
#define Hevc_ParamTarget_Resolution		0x00000004	// change size, crop, pan, etc.
#define Hevc_ParamTarget_Frame_Rate		0x00000008	// change frame rate
#define Hevc_ParamTarget_All			0x0000000f

// hevc commands
typedef enum HevcCommand
{
	Hevc_Command_Unknown,
	Hevc_Command_MainState,							// set main state
	Hevc_Command_VinState,							// set video input state
	Hevc_Command_EhState,							// set encoder state
	Hevc_Command_Gpio,								// control gpio
	Hevc_Command_Reset,								// reset codec
    Hevc_Command_ChangeParam,                       // change dynamic params during encode
    Hevc_Command_ChangePicture,                     // change picture type
    Hevc_Command_Size
} HevcCommand;

// hevc command information
typedef struct HevcDeviceCommand
{
	HevcCommand				command;				// command type
	// main state command info
	HevcMainState			mainState;				// set main state
	HevcEncodeMode			encodeMode;				// set encoder mode
	HevcFirmwareType		firmwareType;			// set encode firmware type
	// vin/eh state command info
	HevcVinState			vinState;				// set video input state
	HevcEhState				ehState;				// set encoder state
	ULWord					streamBits;				// command applies to each stream bit
	// gpio command info
	HevcGpioControl			gpioControl;			// gpio control type
	ULWord					gpioNumber;				// gpio port number (function, direction, set, get)
	HevcGpioFunction		gpioFunction;			// gpio port function (function)
	HevcGpioDirection		gpioDirection;			// gpio port direction (direction)
	HevcGpioValue			gpioValue;				// gpio port value (set, get)
    // change encode params
	ULWord					paramTarget;			// parameters to change
    ULWord					paramStreamId;			// stream id
    HevcChangeSequence      changeSequence;         // start new sequence (vbr)
    ULWord                  maxBitRate;             // maximum bitrate (vbr)
    ULWord                  aveBitRate;             // average bitrate (vbr and cbr)
    ULWord                  minBitRate;             // minimum bitrate (vbr)
	ULWord					seqEndPicNumber;		// last picture number of sequence (resolution and frame rate)
	ULWord					hSizeEh;				// resolution parameters
	ULWord					vSizeEh;
	ULWord					cropLeft;
	ULWord					cropRight;
	ULWord					cropTop;
	ULWord					cropBottom;
	ULWord					panScanRectLeft;
	ULWord					panScanRectRight;
	ULWord					panScanRectTop;
	ULWord					panScanRectBottom;
	ULWord					videoSignalType;
	ULWord					videoFormat;
	ULWord					videoFullRangeFlag;
	ULWord					colourDescriptionPresentFlag;
	ULWord					colourPrimaries;
	ULWord					transferCharacteristics;
	ULWord					matrixCoeffs;
	ULWord					aspectRatioIdc;
	ULWord					sarWidth;
	ULWord					sarHeight;
	ULWord					frameRateCode;			// frame rate parameter
	// change picture type
	ULWord					picType;				// picture type
    ULWord					picStreamId;			// stream id
	ULWord					gopEndPicNumber;		// last picture number of gop
	// general command flags
	ULWord					flags;					// command flags
} HevcDeviceCommand;

// hevc stream types
typedef enum HevcStream
{
	Hevc_Stream_Unknown,
	Hevc_Stream_VideoRaw,							// raw data stream
	Hevc_Stream_VideoEnc,							// encoded data stream
	Hevc_Stream_Size
} HevcStream;

// hevc picture data (raw streams)
typedef struct HevcPictureData
{
	ULWord					serialNumber;			// serial number (application data)
	ULWord					ptsValueLow;			// presentation time stamp (90kHz)
	ULWord					ptsValueHigh;			// pts high bit only (33 bit roll over)
	ULWord					pictureNumber;			// start with 1 and increment for each picture
	ULWord					numAdditionalData;		// number of additional data entries
} HevcPictureData;

// hevc picture information (raw streams)
typedef struct HevcPictureInfo
{
    HevcPictureData			pictureData;			// raw stream picture data
//
//	additional data format
//	u32 additional_data_type
//	u32 additional_data_size (256 bytes max)
//	u8... additional_data_payload
//	... more additional data
//
//	additional data types
//	1 = sei data
//	2 = passthrough data (to encoded additional data of encoded frame)
//	4 = cancel sei on every gop (set additional size to 0)
//
//	sei data format
//	u8 user_sei_location
//	u8 user_sei_type
//	u8 user_sei_length
//	u8... user_sei_payload
//
//	user sei location
//	2 = every gop head picture
//	3 = this picture only
//
//	passthrough data format
//	u8... passthrough_data_payload
//
	UByte					additionalData[HEVC_ADDITIONAL_DATA_SIZE];
} HevcPictureInfo;

// hevc encoded stream data (encoded streams)
typedef struct HevcEncodedData
{
    ULWord                  serialNumber;			// serial number (from picture information)
    ULWord                  esOffsetLow;			// encoded stream frame location (?)
    ULWord                  esOffsetHigh;			// es frame location high 32 bits
    ULWord                  esSize;					// encoded stream frame size (?)
    ULWord                  ptsValueLow;			// presentation time stamp (picture information)
    ULWord                  ptsValueHigh;			// pts high bit (33 bit roll over)
    ULWord                  dtsValueLow;			// decoding time stamp (90 kHz)
    ULWord                  dtsValueHigh;			// dts high bit (33 bit roll over)
    ULWord                  itcValueLow;			// internal time clock (90 kHz)
    ULWord                  itcValueHigh;			// itc high bit (33 bit roll over)
    ULWord                  itcExtension;			// internal time extension (27 MHz)
    ULWord                  temporalId;				// temporal ID
	ULWord					esIdrType;				// 0 = not IDR, 1 = IDR, 3 = IDR command
    ULWord                  pictureType;			// 0 = I-frame, 1 = P-frame, 2 = B-frame
    ULWord                  nalOffset;				// offset to the nal top of the idr/i picture
    ULWord                  cpbValue;				// codec picture buffer occupancy value
	ULWord					esHSize;				// horizontal resolution
	ULWord					esVSize;				// vertical resolution
	ULWord					esUnitsInTick;			// frame duration (2x eh param value for half rate)
	ULWord					esBitRate;				// bit rate (Kbps)
    ULWord                  esEndFlag;              // 0 = not end of sequence, 1 = end of sequence
    ULWord                  esLastFrame;			// 0xffffffff = last frame
    ULWord                  reserved0;
    ULWord                  reserved1;
    ULWord                  reserved2;
    ULWord                  reserved3;
    ULWord                  reserved4;
    ULWord                  reserved5;
    ULWord                  reserved6;
    ULWord                  reserved7;
    ULWord                  numAdditionalData;		// number of additional data entries
} HevcEncodedData;

// hevc encode stream information (encoded streams)
typedef struct HevcEncodedInfo
{
    HevcEncodedData			encodedData;			// encoded stream data
	UByte					additionalData[HEVC_ADDITIONAL_DATA_SIZE];
} HevcEncodedInfo;

// hevc stream transfer information
typedef struct HevcDeviceTransfer
{
	HevcStream				streamType;				// transfer stream type
	ULWord					streamId;				// transfer stream id

	UByte*					pVideoBuffer;			// video buffer
	ULWord					videoBufferSize;		// total video buffer size
	ULWord					videoDataSize;			// video data size in buffer

	ULWord					segVideoPitch;			// video segment pitch
	ULWord					segCodecPitch;			// codec segment pitch
	ULWord					segSize;				// segment size
	ULWord					segCount;				// number of segments

    UByte*					pInfoBuffer;			// information buffer (picture or encoded)
	ULWord					infoBufferSize;			// total information buffer size
	ULWord					infoDataSize;			// information size in buffer

	LWord64					encodeTime;				// frame encode time (100ns host system clock)
	ULWord					flags;					// transfer flags (see above for last frame flag)
} HevcDeviceTransfer;

// hevc gpio port status
typedef struct hevc_gpio_state
{
	HevcGpioFunction		function;				// gpio last set port function
	HevcGpioDirection		direction;				// gpio last set port direction
	HevcGpioValue			setValue;				// gpio last set value
	HevcGpioValue			getValue;				// gpio last get value
} HevcGpioState;	

// hevc stream statistics (nsec, bytes)
typedef struct hevc_stream_statistics
{
	LWord64				transferCount;				// number of transfers queued
	LWord64				minTransferTime;			// minimum time between transfers
	LWord64				avrTransferTime;			// average time between transfers
	LWord64				maxTransferTime;			// maximum time between transfers
	LWord64				minTransferSize;			// minimum transfer size
	LWord64				maxTransferSize;			// maximum transfer size
	LWord64				avrTransferSize;			// average transfer size
	LWord64				minCopyDuration;			// time for io thread to copy frames 
	LWord64				maxCopyDuration;			//   to/from bounce buffer
	LWord64				avrCopyDuration;
	LWord64				minEnqueueDuration;			// time from io thread enqueue
	LWord64				maxEnqueueDuration;			//   to send to codec
	LWord64				avrEnqueueDuration;
	LWord64				minSendDuration;			// time from send to codec
	LWord64				maxSendDuration;			//   to codec acknowledge
	LWord64				avrSendDuration;
	LWord64				minDmaDuration;				// time from codec acknowledge
	LWord64				maxDmaDuration;				//   to codec dma completion
	LWord64				avrDmaDuration;
	LWord64				minDequeueDuration;			// time from io thread enqueue
	LWord64				maxDequeueDuration;			//   to io thread dequeue
	LWord64				avrDequeueDuration;
} HevcStreamStatistics;

// hevc status information
typedef struct HevcDeviceStatus
{
	HevcMainState			mainState;				// codec main state
	HevcEncodeMode			encodeMode;				// codec encode mode
	HevcFirmwareType		firmwareType;			// codec firmware type

	HevcVifState			vifState[HEVC_STREAM_MAX];	// video interface state
	HevcVinState			vinState[HEVC_STREAM_MAX];	// video input state
	HevcEhState				ehState[HEVC_STREAM_MAX];	// encoder state
	HevcGpioState			gpioState[HEVC_GPIO_MAX];	// gpio state

	LWord64					commandCount;			// number of commands processed
	LWord64					rawTransferCount;		// number of raw transfers processed
	LWord64					encTransferCount;		// number of encoded transfers processed

	ULWord					commandQueueLevel;		// number of commands in command queue
	ULWord					rawTransferQueueLevel;	// number of transfers in raw transfer queue
	ULWord					encTransferQueueLevel;	// number of transfers in encoded transfer queue
} HevcDeviceStatus;

// hevc debug information
typedef struct HevcDeviceDebug
{
	HevcDeviceStatus		deviceStatus;			// device status structure

	HevcStreamStatistics	rawStats[HEVC_STREAM_MAX];		// raw stream statistics
	HevcStreamStatistics	encStats[HEVC_STREAM_MAX];		// encoded stream statistics
	ULWord					queueLevel[HEVC_STREAM_MAX];	// stream queue level
	ULWord					clearRawStatsBits;		// stream bits to clear raw stream statistics
	ULWord					clearEncStatsBits;		// stream bits to clear encodec stream statistics

	ULWord					cmdContCount;			// codec command continuity count
	ULWord					cmdAckContCount;		// codec command acknowledge count
	ULWord					cmdMsgContCount;		// codec command message count
	ULWord					rawContCount;			// raw dma continuity count	
	ULWord					rawAckContCount;		// raw dma acknowledge count
	ULWord					rawMsgContCount;		// raw dma message count
	ULWord					encContCount;			// encoded dma continuity count
	ULWord					encAckContCount;		// encoded dma acnowledge count
	ULWord					encMsgContCount;		// encoded dma message count
} HevcDeviceDebug;

// hevc driver ioctl message types
typedef enum HevcMessageId
{
	Hevc_MessageId_Unknown,
	Hevc_MessageId_Info,							// get device information
	Hevc_MessageId_Register,						// write/read codec register
	Hevc_MessageId_Command,							// send a codec command
	Hevc_MessageId_Transfer,						// transfer codec stream data
	Hevc_MessageId_Status,							// get codec status
	Hevc_MessageId_Debug,							// get debug information
	Hevc_MessageId_Size
} HevcMessageId;

// hevc common driver ioctl message header
typedef struct hevcMessageHeader
{
	HevcMessageId			type;
	ULWord					size;
	ULWord					status;
	ULWord					reverved0;
	ULWord					reverved1;
	ULWord					reverved2;
} HevcMessageHeader;

// hevc stream transfer data
typedef struct HevcTransferData
{
	HevcStream				streamType;
	ULWord					streamId;
	ULWord64				videoBuffer;
	ULWord					videoBufferSize;
	ULWord					videoDataSize;
	ULWord					segVideoPitch;
	ULWord					segCodecPitch;
	ULWord					segSize;
	ULWord					segCount;
	ULWord64				infoBuffer;
	ULWord					infoBufferSize;
	ULWord					infoDataSize;
	LWord64					encodeTime;
	ULWord					flags;
} HevcTransferData;

// hevc driver device information message
typedef struct HevcMessageInfo
{
	HevcMessageHeader		header;
	HevcDeviceInfo			data;
} HevcMessageInfo;

// hevc driver register message
typedef struct HevcMessageRegister
{
	HevcMessageHeader		header;
	HevcDeviceRegister		data;
} HevcMessageRegister;

// hevc driver command message
typedef struct HevcMessageCommand
{
	HevcMessageHeader		header;
	HevcDeviceCommand		data;
} HevcMessageCommand;

// hevc driver transfer message
typedef struct HevcMessageTransfer
{
	HevcMessageHeader		header;
	HevcTransferData		data;
} HevcMessageTransfer;

// hevc driver status message
typedef struct HevcMessageStatus
{
	HevcMessageHeader		header;
	HevcDeviceStatus		data;
} HevcMessageStatus;

// hevc driver debug message
typedef struct HevcMessageDebug
{
	HevcMessageHeader		header;
	HevcDeviceDebug			data;
} HevcMessageDebug;

typedef struct HDRRegValues{
	uint16_t	greenPrimaryX;
	uint16_t	greenPrimaryY;
	uint16_t	bluePrimaryX;
	uint16_t	bluePrimaryY;
	uint16_t	redPrimaryX;
	uint16_t	redPrimaryY;
	uint16_t	whitePointX;
	uint16_t	whitePointY;
	uint16_t	maxMasteringLuminance;
	uint16_t	minMasteringLuminance;
	uint16_t	maxContentLightLevel;
	uint16_t	maxFrameAverageLightLevel;
	uint8_t		electroOpticalTransferFunction;
	uint8_t		staticMetadataDescriptorID;
}HDRRegValues;

typedef struct HDRFloatValues{
	float		greenPrimaryX;
	float		greenPrimaryY;
	float		bluePrimaryX;
	float		bluePrimaryY;
	float		redPrimaryX;
	float		redPrimaryY;
	float		whitePointX;
	float		whitePointY;
    uint16_t	maxMasteringLuminance;
	float		minMasteringLuminance;
	uint16_t	maxContentLightLevel;
	uint16_t	maxFrameAverageLightLevel;
    uint8_t		electroOpticalTransferFunction;
    uint8_t		staticMetadataDescriptorID;
}HDRFloatValues;

typedef struct HDRDriverValues{
	uint16_t	greenPrimaryX;
	uint16_t	greenPrimaryY;
	uint16_t	bluePrimaryX;
	uint16_t	bluePrimaryY;
	uint16_t	redPrimaryX;
	uint16_t	redPrimaryY;
	uint16_t	whitePointX;
	uint16_t	whitePointY;
	uint16_t	maxMasteringLuminance;
	uint16_t	minMasteringLuminance;
	uint16_t	maxContentLightLevel;
	uint16_t	maxFrameAverageLightLevel;
	uint8_t		electroOpticalTransferFunction;
	uint8_t		staticMetadataDescriptorID;
	uint8_t		luminance;
}HDRDriverValues;

#define NTV2_IS_VALID_HDR_PRIMARY(__val__)				((__val__) <= 0x0000C350)
#define NTV2_IS_VALID_HDR_MASTERING_LUMINENCE(__val__)	(true)
#define NTV2_IS_VALID_HDR_LIGHT_LEVEL(__val__)			(true)

#endif	//	NTV2PUBLICINTERFACE_H
