/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2m31publicinterface.h
	@brief		Declares structs used for the Corvid HEVC.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.    All rights reserved.
**/

#ifndef NTV2M31PUBLICINTERFACE_H
#define NTV2M31PUBLICINTERFACE_H

#include "ajatypes.h"
#include "ntv2m31enums.h"

#define        CPARAM_REG_START         0x20000000

// Common param register index (to get actual register location we multiply index by 4 then add CPARAM_REG_START)

typedef enum
{
    kRegCParamCC,                       // 0x20000000
    kRegCParamReserved0,                // 0x20000004
    kRegCParamStillColor,               // 0x20000008
    kRegCParamScBuf,                    // 0x2000000C
    
    kRegCParamResoType7_0,              // 0x20000010
    kRegCParamResoType15_8,             // 0x20000014
    kRegCParamResoType23_16,            // 0x20000018
    kRegCParamResoType31_24	            // 0x2000001C

} CParamRegisterIndex;


typedef enum
{
    kRegMaskCParamCC                    = 0xFFFF,

	// kRegCParamStillColor
	kRegMaskCParamStillColorY			= BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
	kRegMaskCParamStillColorCb			= BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
	kRegMaskCParamStillColorCr			= BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),

    // kRegCParamScBuf
    kRegMaskCParamScRobustMode          = BIT(8)+BIT(9)+BIT(10)+BIT(11),
    kRegMaskCParamScBuf                 = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),

    // kRegCParamResoType7_0
    kRegMaskCParamResoType7             = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskCParamResoType6             = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskCParamResoType5             = BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskCParamResoType4             = BIT(16)+BIT(17)+BIT(18)+BIT(19),
    kRegMaskCParamResoType3             = BIT(12)+BIT(13)+BIT(15)+BIT(15),
    kRegMaskCParamResoType2             = BIT(8)+BIT(9)+BIT(10)+BIT(11),
    kRegMaskCParamResoType1             = BIT(4)+BIT(5)+BIT(6)+BIT(7),
    kRegMaskCParamResoType0             = BIT(0)+BIT(1)+BIT(2)+BIT(3)
    
} CParamRegisterMask;


typedef enum
{
    kRegShiftCParamCC                   = 0,

	// kRegCParamStillColor
    kRegShiftCParamStillColorY			= 16,
    kRegShiftCParamStillColorCb			= 8,
    kRegShiftCParamStillColorCr			= 0,

    // kRegCParamScBuf
    kRegShiftCParamScRobustMode         = 8,
    kRegShiftCParamScBuf                = 0,

    // kRegCParamResoType7_0
    kRegShiftCParamResoType7            = 28,
    kRegShiftCParamResoType6            = 24,
    kRegShiftCParamResoType5            = 20,
    kRegShiftCParamResoType4            = 16,
    kRegShiftCParamResoType3            = 12,
    kRegShiftCParamResoType2            = 8,
    kRegShiftCParamResoType1            = 4,
    kRegShiftCParamResoType0            = 0
    
} CParamRegisterShift;


#define        VIPARAM_REG_START        0x20000020
#define        VIPARAM_CH_SIZE          0x30

// VI param register index (to get actual register location we multiply index by 4 then add VIPARAM_REG_START)

typedef enum
{
    kRegVI0ParamCC,                     // 0x20000020
    kRegVI1ParamCC,                     // 0x20000024
    kRegVI2ParamCC,                     // 0x20000028
    kRegVI3ParamCC,                     // 0x2000002C
    
    // Channel 0
    kRegVI0Param,                       // 0x20000030
    kRegVI0ParamStartLine,              // 0x20000034
    kRegVI0ParamMaxCount,               // 0x20000038
    kRegVI0ParamValidPosLine,           // 0x2000003C
    kRegVI0ParamValidLineCount,         // 0x20000040
    kRegVI0ParamValidVCount,            // 0x20000044
    kRegVI0ParamReserved0,              // 0x20000048
    kRegVI0ParamReserved1,              // 0x2000004C
    kRegVI0ParamReserved2,              // 0x20000050
    kRegVI0ParamReserved3,              // 0x20000054
    kRegVI0ParamMisc,                   // 0x20000058
    kRegVI0ParamReserved4,              // 0x2000005C
   
    // Channel 1
    kRegVI1Param,                       // 0x20000060
    kRegVI1ParamStartLine,              // 0x20000064
    kRegVI1ParamMaxCount,               // 0x20000068
    kRegVI1ParamValidPosLine,           // 0x2000006C
    kRegVI1ParamValidLineCount,         // 0x20000070
    kRegVI1ParamValidVCount,            // 0x20000074
    kRegVI1ParamReserved0,              // 0x20000078
    kRegVI1ParamReserved1,              // 0x2000007C
    kRegVI1ParamReserved2,              // 0x20000080
    kRegVI1ParamReserved3,              // 0x20000084
    kRegVI1ParamMisc,                   // 0x20000088
    kRegVI1ParamReserved4,              // 0x2000008C
  
    // Channel 2
    kRegVI2Param,                       // 0x20000090
    kRegVI2ParamStartLine,              // 0x20000094
    kRegVI2ParamMaxCount,               // 0x20000098
    kRegVI2ParamValidPosLine,           // 0x2000009C
    kRegVI2ParamValidLineCount,         // 0x200000A0
    kRegVI2ParamValidVCount,            // 0x200000A4
    kRegVI2ParamReserved0,              // 0x200000A8
    kRegVI2ParamReserved1,              // 0x200000AC
    kRegVI2ParamReserved2,              // 0x200000B0
    kRegVI2ParamReserved3,              // 0x200000B4
    kRegVI2ParamMisc,                   // 0x200000B8
    kRegVI2ParamReserved4,              // 0x200000BC
    
    // Channel 3
    kRegVI3Param,                       // 0x200000C0
    kRegVI3ParamStartLine,              // 0x200000C4
    kRegVI3ParamMaxCount,               // 0x200000C8
    kRegVI3ParamValidPosLine,           // 0x200000CC
    kRegVI3ParamValidLineCount,         // 0x200000D0
    kRegVI3ParamValidVCount,            // 0x200000D4
    kRegVI3ParamReserved0,              // 0x200000D8
    kRegVI3ParamReserved1,              // 0x200000DC
    kRegVI3ParamReserved2,              // 0x200000E0
    kRegVI3ParamReserved3,              // 0x200000E4
    kRegVI3ParamMisc,                   // 0x200000E8
    kRegVI3ParamReserved4,              // 0x200000EC
    
    // Extras at the end don't belong to a channel
    kRegVIParamReserved0,               // 0x200000F0
    kRegVIParamReserved1,               // 0x200000F4
    kRegVIParamReserved2,               // 0x200000F8
    kRegVIParamReserved3              	// 0x200000FC

} VIParamRegisterIndex;


typedef enum
{
    kRegMaskVIParamCC                   = 0xFFFF,
    
    // kRegVI0Param
    kRegMaskVIParamYCMux                = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVIParamYCSwap               = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskVIParamPrivate01            = BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskVIParamPrivate02            = BIT(16)+BIT(17)+BIT(18)+BIT(19),
    kRegMaskVIParamPrivate03            = BIT(12)+BIT(13)+BIT(15)+BIT(15),
    kRegMaskVIParamPrivate04            = BIT(8)+BIT(9)+BIT(10)+BIT(11),
    kRegMaskVIParamFormat               = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
    
    // kRegVI0ParamStartLine
    kRegMaskVIParamPrivate05            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVIParamPrivate06            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    // kRegVI0ParamMaxCount
    kRegMaskVIParamPrivate07            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVIParamPrivate08            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    // kRegVI1ParamValidPosLine
    kRegMaskVIParamPrivate09            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVIParamPrivate10            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    // kRegVI0ParamValidLineCount
    kRegMaskVIParamPrivate11            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVIParamPrivate12            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    // kRegVI0ParamValidVCount
    kRegMaskVIParamPrivate13            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    
    // kRegVI0ParamMisc
    kRegMaskVIParamGPIO                 = BIT(0),
    kRegMaskVIParamSyncMembers          = BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),
    kRegMaskVIParamSyncMaster           = BIT(8)+BIT(9)+BIT(10)+BIT(11),
    kRegMaskVIParamInputPort            = BIT(12)+BIT(13)+BIT(15)+BIT(15)

} VIParamRegisterMask;


typedef enum
{
    kRegShiftVIParamCC                  = 0,
    
    // kRegVI0Param
    kRegShiftVIParamYCMux               = 28,
    kRegShiftVIParamYCSwap              = 24,
    kRegShiftVIParamPrivate01           = 20,
    kRegShiftVIParamPrivate02           = 16,
    kRegShiftVIParamPrivate03           = 12,
    kRegShiftVIParamPrivate04           = 8,
    kRegShiftVIParamFormat              = 0,
    
    // kRegVI0ParamStartLine
    kRegShiftVIParamPrivate05           = 16,
    kRegShiftVIParamPrivate06           = 0,
    
    // kRegVI0ParamMaxCount
    kRegShiftVIParamPrivate07           = 16,
    kRegShiftVIParamPrivate08           = 0,
    
    // kRegVI1ParamValidPosLine
    kRegShiftVIParamPrivate09           = 16,
    kRegShiftVIParamPrivate10           = 0,
    
    // kRegVI0ParamValidLineCount
    kRegShiftVIParamPrivate11           = 16,
    kRegShiftVIParamPrivate12           = 0,
    
    // kRegVI0ParamValidVCount
    kRegShiftVIParamPrivate13           = 16,
    
    // kRegVI0ParamMisc
    kRegShiftVIParamGPIO                = 0,
    kRegShiftVIParamSyncMembers         = 3,
    kRegShiftVIParamSyncMaster          = 8,
    kRegShiftVIParamInputPort           = 12
    
} VIParamRegisterShift;


#define        VINPARAM_REG_START       0x20000100
#define        VINPARAM_CH_SIZE         0x20

// VIn param register index (to get actual register location we multiply index by 4 then add VINPARAM_REG_START)

// Note:    Each encoder channel has the following VIn registers.  This register map will be repeated for each of the 32 virtual
//            channels.  The current firmware supports four physical encoder channels.

typedef enum
{
    kRegVINParamCC,                     // 0x20000100
    kRegVINParamSource,                 // 0x20000104
    kRegVINParamOut,                    // 0x20000108
    kRegVINParamSize,                   // 0x2000010C
    
    kRegVINParamPTSModeInitialMSB,      // 0x20000110
    kRegVINParamInitialPTS,             // 0x20000114
    kRegVINParamInitialSerialNum,       // 0x20000118
    kRegVINParamReserved1               // 0x2000011C

} VINParamRegisterIndex;


typedef enum
{
    kRegMaskVINParamCC                  = 0xFFFF,
    
    // kRegVINParamSource
    kRegMaskVINParamSource              = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVINParamSourceId            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskVINParamChromaFormat        = BIT(4)+ BIT(5)+BIT(6)+BIT(7),
    kRegMaskVINParamBitDepth            = BIT(0)+BIT(1)+BIT(2)+BIT(3),

    // kRegVINParamOut
    kRegMaskVInParamFrameRate           = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVInParamPrivate01           = BIT(0)+BIT(1)+BIT(2)+BIT(3),

    // kRegVINParamSize
    kRegMaskVINParamHSize               = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVINParamVSize               = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegVINParamPTSModeInitialMSB
    kRegMaskVINParamPTSMode             = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVINParamInitialPTSMSB       = BIT(0),

    kRegMaskVINParamInitialPTS          = 0xFFFFFFFF,
    kRegMaskVINParamInitialSerialNum    = 0xFFFFFFFF

} VINParamRegisterMask;


typedef enum
{
    kRegShiftVINParamCC                 = 0,

    // kRegVINParamSource
    kRegShiftVINParamSource             = 28,
    kRegShiftVINParamSourceId           = 16,
    kRegShiftVINParamChromaFormat       = 4,
    kRegShiftVINParamBitDepth           = 0,
    
    // kRegVINParamOut
    kRegShiftVInParamFrameRate          = 24,
    kRegShiftVInParamPrivate01          = 0,
    
    // kRegVINParamSize
    kRegShiftVINParamHSize              = 16,
    kRegShiftVINParamVSize              = 0,
    
    // kRegVINParamPTSModeInitialMSB
    kRegShiftVINParamPTSMode            = 28,
    kRegShiftVINParamInitialPTSMSB      = 0,
    
    kRegShiftVINParamInitialPTS         = 0,
    kRegShiftVINParamInitialSerialNum   = 0

} VINParamRegisterShift;


#define        VAPARAM_REG_START        0x20000800
#define        VAPARAM_CH_SIZE          0x30

// VA param register index (to get actual register location we multiply index by 4 then add VAPARAM_REG_START)

// Note:    Each encoder channel has the following VA registers.  This register map will be repeated for each of the 32 virtual
//            channels.  The current firmware supports four physical encoder channels.

typedef enum
{
    kRegVAParamCC,                      // 0x20000800
    kRegVAParamSource,                  // 0x20000804
    kRegVAParamRateFormat,              // 0x20000808
    kRegVAParamSizeVA,                  // 0x2000080C
    kRegVAParamSizeEH,                  // 0x20000810
    
    kRegVAParamCoef1_0,                 // 0x20000814
    kRegVAParamCoef3_2,                 // 0x20000818
    kRegVAParamCoef5_4,                 // 0x2000081C
    kRegVAParamCoef7_6,                 // 0x20000820

    kRegVAParamStartOffset,             // 0x20000824
    kRegVAParamSceneChange,             // 0x20000828
    kRegVAParamReserved2                // 0x2000082C

} VAParamRegisterIndex;


typedef enum
{
    kRegMaskVAParamCC                   = 0xFFFF,

    // kRegVAParamSource
    kRegMaskVAParamSource               = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamSourceId             = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskVAInterlace                 = BIT(8)+ BIT(9)+BIT(10)+BIT(11),
    kRegMaskVAParamChromaFormat         = BIT(4)+ BIT(5)+BIT(6)+BIT(7),
    kRegMaskVAParamBitDepth             = BIT(0)+BIT(1)+BIT(2)+BIT(3),
    
    // kRegVAParamRateFormat
    kRegMaskVAParamFrameRate            = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamChromaFormatOut      = BIT(4)+ BIT(5)+BIT(6)+BIT(7),
    kRegMaskVAParamBitDepthOut          = BIT(0)+BIT(1)+BIT(2)+BIT(3),

    // kRegVAParamSizeVA
    kRegMaskVAParamHSizeVA              = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamVSizeVA              = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegVAParamSizeEH
    kRegMaskVAParamHSizeEH              = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamVSizeEH              = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegVAParamCoef1_0
    kRegMaskVAParamCoef1                = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamCoef0                = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegVAParamCoef3_2
    kRegMaskVAParamCoef3                = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamCoef2                = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegVAParamCoef5_4
    kRegMaskVAParamCoef5                = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamCoef4                = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegVAParamCoef7_6
    kRegMaskVAParamCoef7                = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamCoef6                = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegVAParamStartOffset
    kRegMaskVAParamPrivate01            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskVAParamPrivate02            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegVAParamSceneChange
    kRegMaskVAParamSceneChange          = BIT(16)+BIT(17)+BIT(18)+BIT(19)

} VAParamRegisterMask;


typedef enum
{
    kRegShiftVAParamCC                  = 0,

    // kRegVAParamSource
    kRegShiftVAParamSource              = 28,
    kRegShiftVAParamSourceId            = 16,
    kRegShiftVAInterlace                = 8,
    kRegShiftVAParamChromaFormat        = 4,
    kRegShiftVAParamBitDepth            = 0,
    
    // kRegVAParamRateFormat
    kRegShiftVAParamFrameRate           = 24,
    kRegShiftVAParamChromaFormatOut     = 4,
    kRegShiftVAParamBitDepthOut         = 0,

    // kRegVAParamSizeVA
    kRegShiftVAParamHSizeVA             = 16,
    kRegShiftVAParamVSizeVA             = 0,
    
    // kRegVAParamSizeEH
    kRegShiftVAParamHSizeEH             = 16,
    kRegShiftVAParamVSizeEH             = 0,

    // kRegVAParamCoef1_0
    kRegShiftVAParamCoef1               = 16,
    kRegShiftVAParamCoef0               = 0,
    
    // kRegVAParamCoef3_2
    kRegShiftVAParamCoef3               = 16,
    kRegShiftVAParamCoef2               = 0,
    
    // kRegVAParamCoef5_4
    kRegShiftVAParamCoef5               = 16,
    kRegShiftVAParamCoef4               = 0,
    
    // kRegVAParamCoef7_6
    kRegShiftVAParamCoef7               = 16,
    kRegShiftVAParamCoef6               = 0,

    // kRegVAParamStartOffset
    kRegShiftVAParamPrivate01           = 16,
    kRegShiftVAParamPrivate02           = 0,
    
    // kRegVAParamSceneChange
    kRegShiftVAParamSceneChange         = 16

} VAParamRegisterShift;


#define        EHPARAM_REG_START        0x20001000
#define        EHPARAM_CH_SIZE          0x100

// VA param register index (to get actual register location we multiply index by 4 then add VAPARAM_REG_START)

// Note:    Each encoder channel has the following VA registers.  This register map will be repeated for each of the 32 virtual
//            channels.  The current firmware supports four physical encoder channels.

typedef enum
{
    kRegEHParamCC,                      // 0x20001000
    kRegEHParamSource,                  // 0x20001004
    kRegEHParamSizeVA,                  // 0x20001008
    kRegEHParamSizeEH,                  // 0x2000100C
    
    kRegEHParamProfile,                 // 0x20001010
    kRegEHParamAspectRatio,             // 0x20001014
    kRegEHParamSAR,                     // 0x20001018

    kRegEHParamSlice,                   // 0x2000101C
    kRegEHParamGop1,                    // 0x20001020
    kRegEHParamGop2,                    // 0x20001024
    kRegEHParamRCMode,                  // 0x20001028

    kRegEHParamBitRate,                 // 0x2000102C
    kRegEHParamVBRMaxBitRate,           // 0x20001030
    kRegEHParamVBRAveBitRate,           // 0x20001034
    kRegEHParamVBRMinBitRate,           // 0x20001038
    kRegEHParamVBRFillerBitRate,        // 0x2000103C
    
    kRegEHParamNumUnitsInTickMax,       // 0x20001040
    kRegEHParamTimeScale,               // 0x20001044
    kRegEHParamNumUnitsInTick,          // 0x20001048
    kRegEHParamCPBDelay,                // 0x2000104C
    kRegEHParamReserved1,               // 0x20001050
    kRegEHParamReserved2,               // 0x20001054

    kRegEHParamCUTUSize,                // 0x20001058
    kRegEHParamTUDepth,                 // 0x2000105C
    kRegEHParamTSkip,                   // 0x20001060
    kRegEHParamAMP_WP,                  // 0x20001064
    kRegEHParamReserved3,               // 0x20001068

    kRegEHParamPCM,                     // 0x2000106C
    kRegEHParamSAO,                     // 0x20001070
    kRegEHParamDF,                      // 0x20001074
    kRegEHParamRDOQ,                    // 0x20001078
    kRegEHParamPPS,                     // 0x2000107C
    kRegEHParamBufCtrl,                 // 0x20001080
    
    kRegEHParamOverscan,                // 0x20001084
    kRegEHParamVideo,                   // 0x20001088
    kRegEHParamMatrixCoef,              // 0x2000108C

    kRegEHParamCropLR,                  // 0x20001090
    kRegEHParamCropTB,                  // 0x20001094
    kRegEHParamGDR,                     // 0x20001098
    kRegEHParamRecovery,                // 0x2000109C
    
    kRegEHParamPanScanLR,               // 0x200010A0
    kRegEHParamPanScanTB,               // 0x200010A4
    kRegEHParamHash,                    // 0x200010A8

    kRegEHParamReserved4,               // 0x200010AC
    kRegEHParamReserved5,               // 0x200010B0
    kRegEHParamReserved6,               // 0x200010B4
    kRegEHParamReserved7,               // 0x200010B8
    kRegEHParamReserved8,               // 0x200010BC
    kRegEHParamReserved9,               // 0x200010C0
    kRegEHParamReserved10,              // 0x200010C4
    kRegEHParamReserved11,              // 0x200010C8
    kRegEHParamReserved12,              // 0x200010CC
    kRegEHParamReserved13,              // 0x200010D0
    kRegEHParamReserved14,              // 0x200010D4
    kRegEHParamReserved15,              // 0x200010D8
    kRegEHParamReserved16,              // 0x200010DC
    kRegEHParamReserved17,              // 0x200010E0
    kRegEHParamReserved18,              // 0x200010E4
    kRegEHParamReserved19,              // 0x200010E8
    kRegEHParamReserved20,              // 0x200010EC
    kRegEHParamReserved21,              // 0x200010F0
    kRegEHParamReserved22,              // 0x200010F4
    kRegEHParamReserved23,              // 0x200010F8
    kRegEHParamReserved24               // 0x200010FC

} EHParamRegisterIndex;


typedef enum
{
    kRegMaskEHParamCC                   = 0xFFFF,
    
    // kRegEHParamSource
    kRegMaskEHParamSource               = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamSourceId             = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamHsEncodeMode         = BIT(12)+BIT(13)+BIT(14)+BIT(15),
    kRegMaskEHParamInterlace            = BIT(8)+ BIT(9)+BIT(10)+BIT(11),
    kRegMaskEHParamChromaFormat         = BIT(4)+ BIT(5)+BIT(6)+BIT(7),
    kRegMaskEHParamBitDepth             = BIT(0)+BIT(1)+BIT(2)+BIT(3),
    
    // kRegEHParamSizeVA
    kRegMaskEHParamHSizeVA              = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamVSizeVA              = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    // kRegEHParamSizeEH
    kRegMaskEHParamHSizeEH              = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamVSizeEH              = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    //kRegEHParamProfile
    kRegMaskEHParamProfile              = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamLevel                = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamTier                 = BIT(12)+BIT(13)+BIT(14)+BIT(15),

    //kRegEHParamAspectRatio
    kRegMaskEHParamAspectRatio          = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),

    //kRegEHParamSAR
    kRegMaskEHParamSARWidth             = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamSARHeight            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    //kRegEHParamSlice
    kRegMaskEHParamPrivate01            = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamPrivate02            = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskEHParamPrivate03            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamPrivate04            = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    kRegMaskEHParamPrivate05            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),

    //kRegEHParamGop1
    kRegMaskEHParamFrameNumInGOP        = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamGOPHierarchy         = BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamUseTemporalID        = BIT(16)+BIT(17)+BIT(18)+BIT(19),
    kRegMaskEHParamPrivate06            = BIT(4)+BIT(5)+BIT(6)+BIT(7),
    kRegMaskEHParamPrivate07            = BIT(0)+BIT(1)+BIT(2)+BIT(3),
    
    //kRegEHParamGop2
    kRegMaskEHParamPASL0B               = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamIpPeriod             = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskEHParamAdaptiveGOP          = BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamClosedGOP            = BIT(16)+BIT(17)+BIT(18)+BIT(19),
    kRegMaskEHParamIDRInterval          = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    //kRegEHParamRCMode
    kRegMaskEHParamRCMode               = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamMinQpCtrl            = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskEHParamPrivate08            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamAdaptiveQuant        = BIT(0)+BIT(1)+BIT(2)+BIT(3),

    kRegMaskEHParamBitRate              = 0xFFFFFFFF,
    kRegMaskEHParamVBRMaxBitRate        = 0xFFFFFFFF,
    kRegMaskEHParamVBRAveBitRate        = 0xFFFFFFFF,
    kRegMaskEHParamPrivate09            = 0xFFFFFFFF,
    kRegMaskEHParamPrivate10            = 0xFFFFFFFF,
    kRegMaskEHParamNumUnitsInTickMax    = 0xFFFFFFFF,
    kRegMaskEHParamTimeScale            = 0xFFFFFFFF,
    kRegMaskEHParamNumUnitsInTick       = 0xFFFFFFFF,
    kRegMaskEHParamCPBDelay             = 0xFFFFFFFF,

    //kRegEHParamCUTUSize
    kRegMaskEHParamPrivate11            = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamMinCUSize            = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamMaxTUSize            = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    kRegMaskEHParamMinTUSize            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),

    //kRegEHParamTUDepth
    kRegMaskEHParamTUDepthIntra         = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    kRegMaskEHParamTUDepthInter         = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),

    //kRegEHParamTSkip
    kRegMaskEHParamPrivate12            = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamPrivate13            = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskEHParamIntraSmoothing       = BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamPrivate14            = BIT(16)+BIT(17)+BIT(18)+BIT(19),
    kRegMaskEHParamMergeCand            = BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    //kRegEHParamAMP_WP
    kRegMaskEHParamPrivate15            = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamPrivate16            = BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    //kRegEHParamPCM
    kRegMaskEHParamPrivate17            = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamPrivate18            = BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskEHParamPrivate19            = BIT(12)+BIT(13)+BIT(14)+BIT(15)+BIT(16)+BIT(17)+BIT(18)+BIT(19),
    kRegMaskEHParamPrivate20            = BIT(8)+BIT(9)+BIT(10)+BIT(11),
    kRegMaskEHParamPrivate21            = BIT(4)+BIT(5)+BIT(6)+BIT(7),
    kRegMaskEHParamPrivate22            = BIT(0)+BIT(1)+BIT(2)+BIT(3),
    
    //kRegEHParamSAO
    kRegMaskEHParamPrivate23            = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamPrivate24            = BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    //kRegEHParamDF
    kRegMaskEHParamDF                   = BIT(28)+BIT(29)+BIT(30)+BIT(31),

    //kRegEHParamRDOQ
    kRegMaskEHParamPrivate25            = BIT(28)+BIT(29)+BIT(30)+BIT(31),

    //kRegEHParamPPS
    kRegMaskEHParamPPSInsertion         = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamEOS                  = BIT(12)+BIT(13)+BIT(14)+BIT(15),
    kRegMaskEHParamESGap                = BIT(4)+BIT(5)+BIT(6)+BIT(7),
    kRegMaskEHParamMP4                  = BIT(0)+BIT(1)+BIT(2)+BIT(3),
    
    //kRegEHParamBufCtrl
    kRegMaskEHParamBufCtrl              = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamVCLHRD               = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskEHParamPrivate26            = BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamVPSTiming            = BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    //kRegEHParamOverscan
    kRegMaskEHParamOverscanInfo         = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamBitRestriction       = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    
    //kRegEHParamVideo
    kRegMaskEHParamVideoSignalType      = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamVideoFormat          = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskEHParamVideoFullRange       = BIT(20)+BIT(21)+BIT(22)+BIT(23),
    kRegMaskEHParamColourDescription    = BIT(16)+BIT(17)+BIT(18)+BIT(19),
    kRegMaskEHParamColourPrimaries      = BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    kRegMaskEHParamTransferChar         = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7),

    //kRegEHParamMatrixCoef
    kRegMaskEHParamMatrixCoef           = BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamChromaLocInfo        = BIT(8)+BIT(9)+BIT(10)+BIT(11),
    kRegMaskEHParamSampleLocTop         = BIT(4)+BIT(5)+BIT(6)+BIT(7),
    kRegMaskEHParamSampleLocBot         = BIT(0)+BIT(1)+BIT(2)+BIT(3),

    //kRegEHParamCropLR
    kRegMaskEHParamCropLeft             = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamCropRight            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    //kRegEHParamCropTB
    kRegMaskEHParamCropTop              = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamCropBottom           = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    //kRegEHParamGDR
    kRegMaskEHParamPrivate27            = BIT(4)+BIT(5)+BIT(6)+BIT(7),
    kRegMaskEHParamPrivate28            = BIT(0)+BIT(1)+BIT(2)+BIT(3),

    //kRegEHParamRecovery
    kRegMaskEHParamPrivate29            = BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamUseTpIrap            = BIT(24)+BIT(25)+BIT(26)+BIT(27),
    kRegMaskEHParamPicTiming            = BIT(16)+BIT(17)+BIT(18)+BIT(19),
    kRegMaskEHParamScanType             = BIT(8)+BIT(9)+BIT(10)+BIT(11),
    kRegMaskEHParamFramePacking         = BIT(0)+BIT(1)+BIT(2)+BIT(3),
    
    //kRegEHParamPanScanLR
    kRegMaskEHParamScanLeft             = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamScanRight            = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),

    //kRegEHParamPanScanTB
    kRegMaskEHParamScanTop              = BIT(16)+BIT(17)+BIT(18)+BIT(19)+BIT(20)+BIT(21)+BIT(22)+BIT(23)+BIT(24)+BIT(25)+BIT(26)+BIT(27)+BIT(28)+BIT(29)+BIT(30)+BIT(31),
    kRegMaskEHParamScanBottom           = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    
    //kRegEHParamHash
    kRegMaskEHParamPrivate30            = BIT(30)+BIT(31)
        
} EHParamRegisterMask;


typedef enum
{
    kRegShiftEHParamCC                  = 0,
    
    // kRegEHParamSource
    kRegShiftEHParamSource              = 28,
    kRegShiftEHParamSourceId            = 16,
    kRegShiftEHParamHsEncodeMode        = 12,
    kRegShiftEHParamInterlace           = 8,
    kRegShiftEHParamChromaFormat        = 4,
    kRegShiftEHParamBitDepth            = 0,
    
    // kRegEHParamSizeVA
    kRegShiftEHParamHSizeVA             = 16,
    kRegShiftEHParamVSizeVA             = 0,
    
    // kRegEHParamSizeEH
    kRegShiftEHParamHSizeEH             = 16,
    kRegShiftEHParamVSizeEH             = 0,
    
    //kRegEHParamProfile
    kRegShiftEHParamProfile             = 24,
    kRegShiftEHParamLevel               = 16,
    kRegShiftEHParamTier                = 12,
    
    //kRegEHParamAspectRatio
    kRegShiftEHParamAspectRatio         = 24,
    
    //kRegEHParamSAR
    kRegShiftEHParamSARWidth            = 16,
    kRegShiftEHParamSARHeight           = 0,
    
    //kRegEHParamSlice
    kRegShiftEHParamPrivate01           = 28,
    kRegShiftEHParamPrivate02           = 24,
    kRegShiftEHParamPrivate03           = 16,
    kRegShiftEHParamPrivate04           = 8,
    kRegShiftEHParamPrivate05           = 0,
    
    //kRegEHParamGop1
    kRegShiftEHParamFrameNumInGOP       = 24,
    kRegShiftEHParamGOPHierarchy        = 20,
    kRegShiftEHParamUseTemporalID       = 16,
    kRegShiftEHParamPrivate06           = 4,
    kRegShiftEHParamPrivate07           = 0,
    
    //kRegEHParamGop2
    kRegShiftEHParamPASL0B              = 28,
    kRegShiftEHParamIpPeriod            = 24,
    kRegShiftEHParamAdaptiveGOP         = 20,
    kRegShiftEHParamClosedGOP           = 16,
    kRegShiftEHParamIDRInterval         = 0,
    
    //kRegEHParamRCMode
    kRegShiftEHParamRCMode              = 28,
    kRegShiftEHParamMinQpCtrl           = 24,
    kRegShiftEHParamPrivate08           = 16,
    kRegShiftEHParamAdaptiveQuant       = 0,
    
    kRegShiftEHParamBitRate             = 0,
    kRegShiftEHParamVBRMaxBitRate       = 0,
    kRegShiftEHParamVBRAveBitRate       = 0,
    kRegShiftEHParamPrivate09           = 0,
    kRegShiftEHParamPrivate10           = 0,
    kRegShiftEHParamNumUnitsInTickMax   = 0,
    kRegShiftEHParamTimeScale           = 0,
    kRegShiftEHParamNumUnitsInTick      = 0,
    kRegShiftEHParamCPBDelay            = 0,
    
    //kRegEHParamCUTUSize
    kRegShiftEHParamPrivate11           = 24,
    kRegShiftEHParamMinCUSize           = 16,
    kRegShiftEHParamMaxTUSize           = 8,
    kRegShiftEHParamMinTUSize           = 0,
    
    //kRegEHParamTUDepth
    kRegShiftEHParamTUDepthIntra        = 8,
    kRegShiftEHParamTUDepthInter        = 0,
    
    //kRegEHParamTSkip
    kRegShiftEHParamPrivate12           = 28,
    kRegShiftEHParamPrivate13           = 24,
    kRegShiftEHParamIntraSmoothing      = 20,
    kRegShiftEHParamPrivate14           = 16,
    kRegShiftEHParamMergeCand           = 12,
    
    //kRegEHParamAMP_WP
    kRegShiftEHParamPrivate15           = 28,
    kRegShiftEHParamPrivate16           = 12,
    
    //kRegEHParamPCM
    kRegShiftEHParamPrivate17           = 28,
    kRegShiftEHParamPrivate18           = 20,
    kRegShiftEHParamPrivate19           = 12,
    kRegShiftEHParamPrivate20           = 8,
    kRegShiftEHParamPrivate21           = 4,
    kRegShiftEHParamPrivate22           = 0,
    
    //kRegEHParamSAO
    kRegShiftEHParamPrivate23           = 28,
    kRegShiftEHParamPrivate24           = 12,
    
    //kRegEHParamDF
    kRegShiftEHParamDF                  = 28,
    
    //kRegEHParamRDOQ
    kRegShiftEHParamPrivate25           = 28,
    
    //kRegEHParamPPS
    kRegShiftEHParamPPSInsertion        = 28,
    kRegShiftEHParamEOS                 = 12,
    kRegShiftEHParamESGap               = 4,
    kRegShiftEHParamMP4                 = 0,
    
    //kRegEHParamBufCtrl
    kRegShiftEHParamBufCtrl             = 28,
    kRegShiftEHParamVCLHRD              = 24,
    kRegShiftEHParamPrivate26           = 20,
    kRegShiftEHParamVPSTiming           = 12,
    
    //kRegEHParamOverscan
    kRegShiftEHParamOverscanInfo        = 28,
    kRegShiftEHParamBitRestriction      = 24,
    
    //kRegEHParamVideo
    kRegShiftEHParamVideoSignalType     = 28,
    kRegShiftEHParamVideoFormat         = 24,
    kRegShiftEHParamVideoFullRange      = 20,
    kRegShiftEHParamColourDescription   = 16,
    kRegShiftEHParamColourPrimaries     = 8,
    kRegShiftEHParamTransferChar        = 0,
    
    //kRegEHParamMatrixCoef
    kRegShiftEHParamMatrixCoef          = 24,
    kRegShiftEHParamChromaLocInfo       = 8,
    kRegShiftEHParamSampleLocTop        = 4,
    kRegShiftEHParamSampleLocBot        = 0,
   
    //kRegEHParamCropLR
    kRegShiftEHParamCropLeft            = 16,
    kRegShiftEHParamCropRight           = 0,
    
    //kRegEHParamCropTB
    kRegShiftEHParamCropTop             = 16,
    kRegShiftEHParamCropBottom          = 0,
    
    //kRegEHParamGDR
    kRegShiftEHParamPrivate27           = 4,
    kRegShiftEHParamPrivate28           = 0,
    
    //kRegEHParamRecovery
    kRegShiftEHParamPrivate29           = 28,
    kRegShiftEHParamUseTpIrap           = 24,
    kRegShiftEHParamPicTiming           = 16,
    kRegShiftEHParamScanType            = 8,
    kRegShiftEHParamFramePacking        = 0,
    
    //kRegEHParamPanScanLR
    kRegShiftEHParamScanLeft            = 16,
    kRegShiftEHParamScanRight           = 0,
    
    //kRegEHParamPanScanTB
    kRegShiftEHParamScanTop             = 16,
    kRegShiftEHParamScanBottom          = 0,
    
    //kRegEHParamHash
    kRegShiftEHParamPrivate30           = 30

    
} EHParamRegisterShift;


#endif    //    NTV2M31PUBLICINTERFACE_H
