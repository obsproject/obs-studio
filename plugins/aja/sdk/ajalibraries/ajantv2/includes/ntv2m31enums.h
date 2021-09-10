/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2m31enums.h
	@brief		Enumerations for controlling NTV2 devices with m31 HEVC encoders.
	@copyright	(C) 2015-2021 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef NTV2M31ENUMS_H
#define NTV2M31ENUMS_H

typedef enum
{
	// File presets
    M31_FILE_720X480_420_8_5994i,               // 0
    M31_FILE_720X480_420_8_5994p,               // 1
    M31_FILE_720X480_420_8_60i,                 // 2
    M31_FILE_720X480_420_8_60p,                 // 3
    M31_FILE_720X480_422_10_5994i,              // 4
    M31_FILE_720X480_422_10_5994p,              // 5
    M31_FILE_720X480_422_10_60i,                // 6
    M31_FILE_720X480_422_10_60p,                // 7

    M31_FILE_720X576_420_8_50i,                 // 8
    M31_FILE_720X576_420_8_50p,                 // 9
    M31_FILE_720X576_422_10_50i,                // 10
    M31_FILE_720X576_422_10_50p,                // 11
    
    M31_FILE_1280X720_420_8_2398p,              // 12
    M31_FILE_1280X720_420_8_24p,                // 13
    M31_FILE_1280X720_420_8_25p,                // 14
    M31_FILE_1280X720_420_8_2997p,              // 15
    M31_FILE_1280X720_420_8_30p,                // 16
    M31_FILE_1280X720_420_8_50p,                // 17
    M31_FILE_1280X720_420_8_5994p,              // 18
    M31_FILE_1280X720_420_8_60p,                // 19
    
    M31_FILE_1280X720_422_10_2398p,             // 20
    M31_FILE_1280X720_422_10_24p,               // 21
    M31_FILE_1280X720_422_10_25p,               // 22
    M31_FILE_1280X720_422_10_2997p,             // 23
    M31_FILE_1280X720_422_10_30p,               // 24
    M31_FILE_1280X720_422_10_50p,               // 25
    M31_FILE_1280X720_422_10_5994p,             // 26
    M31_FILE_1280X720_422_10_60p,               // 27

    M31_FILE_1920X1080_420_8_2398p,             // 28
    M31_FILE_1920X1080_420_8_24p,               // 29
    M31_FILE_1920X1080_420_8_25p,               // 30
    M31_FILE_1920X1080_420_8_2997p,             // 31
    M31_FILE_1920X1080_420_8_30p,               // 32
    M31_FILE_1920X1080_420_8_50i,               // 33
    M31_FILE_1920X1080_420_8_50p,               // 34
    M31_FILE_1920X1080_420_8_5994i,             // 35
    M31_FILE_1920X1080_420_8_5994p,             // 36
    M31_FILE_1920X1080_420_8_60i,               // 37
    M31_FILE_1920X1080_420_8_60p,               // 38
    
    M31_FILE_1920X1080_422_10_2398p,            // 39
    M31_FILE_1920X1080_422_10_24p,              // 40
    M31_FILE_1920X1080_422_10_25p,              // 41
    M31_FILE_1920X1080_422_10_2997p,            // 42
    M31_FILE_1920X1080_422_10_30p,              // 43
    M31_FILE_1920X1080_422_10_50i,              // 44
    M31_FILE_1920X1080_422_10_50p,              // 45
    M31_FILE_1920X1080_422_10_5994i,            // 46
    M31_FILE_1920X1080_422_10_5994p,            // 47
    M31_FILE_1920X1080_422_10_60i,              // 48
    M31_FILE_1920X1080_422_10_60p,              // 49

    M31_FILE_2048X1080_420_8_2398p,             // 50
    M31_FILE_2048X1080_420_8_24p,               // 51
    M31_FILE_2048X1080_420_8_25p,               // 52
    M31_FILE_2048X1080_420_8_2997p,             // 53
    M31_FILE_2048X1080_420_8_30p,               // 54
    M31_FILE_2048X1080_420_8_50p,               // 55
    M31_FILE_2048X1080_420_8_5994p,             // 56
    M31_FILE_2048X1080_420_8_60p,               // 57

    M31_FILE_2048X1080_422_10_2398p,            // 58
    M31_FILE_2048X1080_422_10_24p,              // 59
    M31_FILE_2048X1080_422_10_25p,              // 60
    M31_FILE_2048X1080_422_10_2997p,            // 61
    M31_FILE_2048X1080_422_10_30p,              // 62
    M31_FILE_2048X1080_422_10_50p,              // 63
    M31_FILE_2048X1080_422_10_5994p,            // 64
    M31_FILE_2048X1080_422_10_60p,              // 65

    M31_FILE_3840X2160_420_8_2398p,             // 66
    M31_FILE_3840X2160_420_8_24p,               // 67
    M31_FILE_3840X2160_420_8_25p,               // 68
    M31_FILE_3840X2160_420_8_2997p,             // 69
    M31_FILE_3840X2160_420_8_30p,               // 70
    M31_FILE_3840X2160_420_8_50p,               // 71
    M31_FILE_3840X2160_420_8_5994p,             // 72
    M31_FILE_3840X2160_420_8_60p,               // 73

    M31_FILE_3840X2160_420_10_50p,              // 74
    M31_FILE_3840X2160_420_10_5994p,            // 75
    M31_FILE_3840X2160_420_10_60p,              // 76
    
    M31_FILE_3840X2160_422_8_2398p,             // 77
    M31_FILE_3840X2160_422_8_24p,               // 78
    M31_FILE_3840X2160_422_8_25p,               // 79
    M31_FILE_3840X2160_422_8_2997p,             // 80
    M31_FILE_3840X2160_422_8_30p,               // 81
    M31_FILE_3840X2160_422_8_50p,               // 82
    M31_FILE_3840X2160_422_8_5994p,             // 83
    M31_FILE_3840X2160_422_8_60p,               // 84
    
    M31_FILE_3840X2160_422_10_2398p,            // 85
    M31_FILE_3840X2160_422_10_24p,              // 86
    M31_FILE_3840X2160_422_10_25p,              // 87
    M31_FILE_3840X2160_422_10_2997p,            // 88
    M31_FILE_3840X2160_422_10_30p,              // 89
    M31_FILE_3840X2160_422_10_50p,              // 90
    M31_FILE_3840X2160_422_10_5994p,            // 91
    M31_FILE_3840X2160_422_10_60p,              // 92

    M31_FILE_4096X2160_420_10_5994p,            // 93
    M31_FILE_4096X2160_420_10_60p,              // 94
    M31_FILE_4096X2160_422_10_50p,              // 95
    M31_FILE_4096X2160_422_10_5994p_IF,         // 96
    M31_FILE_4096X2160_422_10_60p_IF,           // 97

	// Vif presets
    M31_VIF_720X480_420_8_5994i,                // 98
    M31_VIF_720X480_420_8_5994p,                // 99
    M31_VIF_720X480_420_8_60i,                  // 100
    M31_VIF_720X480_420_8_60p,                  // 101
    M31_VIF_720X480_422_10_5994i,               // 102
    M31_VIF_720X480_422_10_5994p,               // 103
    M31_VIF_720X480_422_10_60i,                 // 104
    M31_VIF_720X480_422_10_60p,                 // 105

    M31_VIF_720X576_420_8_50i,                  // 106
    M31_VIF_720X576_420_8_50p,                  // 107
    M31_VIF_720X576_422_10_50i,                 // 108
    M31_VIF_720X576_422_10_50p,                 // 109

    M31_VIF_1280X720_420_8_50p,                 // 110
    M31_VIF_1280X720_420_8_5994p,               // 111
    M31_VIF_1280X720_420_8_60p,                 // 112
    M31_VIF_1280X720_422_10_50p,                // 113
    M31_VIF_1280X720_422_10_5994p,              // 114
    M31_VIF_1280X720_422_10_60p,                // 115

    M31_VIF_1920X1080_420_8_50i,                // 116
    M31_VIF_1920X1080_420_8_50p,                // 117
    M31_VIF_1920X1080_420_8_5994i,              // 118
    M31_VIF_1920X1080_420_8_5994p,              // 119
    M31_VIF_1920X1080_420_8_60i,                // 120
    M31_VIF_1920X1080_420_8_60p,                // 121
    M31_VIF_1920X1080_420_10_50i,               // 122
    M31_VIF_1920X1080_420_10_50p,               // 123
    M31_VIF_1920X1080_420_10_5994i,             // 124
    M31_VIF_1920X1080_420_10_5994p,             // 125
    M31_VIF_1920X1080_420_10_60i,               // 126
    M31_VIF_1920X1080_420_10_60p,               // 127
    M31_VIF_1920X1080_422_10_5994i,             // 128
    M31_VIF_1920X1080_422_10_5994p,             // 129
    M31_VIF_1920X1080_422_10_60i,               // 130
    M31_VIF_1920X1080_422_10_60p,               // 131

    M31_VIF_3840X2160_420_8_30p,                // 132
    M31_VIF_3840X2160_420_8_50p,                // 133
    M31_VIF_3840X2160_420_8_5994p,              // 134
    M31_VIF_3840X2160_420_8_60p,                // 135
    M31_VIF_3840X2160_420_10_50p,               // 136
    M31_VIF_3840X2160_420_10_5994p,             // 137
    M31_VIF_3840X2160_420_10_60p,               // 138

    M31_VIF_3840X2160_422_10_30p,               // 139
    M31_VIF_3840X2160_422_10_50p,               // 140
    M31_VIF_3840X2160_422_10_5994p,             // 141
    M31_VIF_3840X2160_422_10_60p,               // 142

	M31_NUMVIDEOPRESETS
} M31VideoPreset;


#define	IS_VALID_M31VideoPreset(__s__)	((__s__) >= M31_FILE_720X480_420_8_5994i && (__s__) < M31_NUMVIDEOPRESETS)

typedef enum
{
	M31_VIRTUAL_CH0,
	M31_VIRTUAL_CH1,
	M31_VIRTUAL_CH2,
	M31_VIRTUAL_CH3,
	M31_VIRTUAL_CH4,
	M31_VIRTUAL_CH5,
	M31_VIRTUAL_CH6,
	M31_VIRTUAL_CH7,
	
	M31_VIRTUAL_CH8,
	M31_VIRTUAL_CH9,
	M31_VIRTUAL_CH10,
	M31_VIRTUAL_CH11,
	M31_VIRTUAL_CH12,
	M31_VIRTUAL_CH13,
	M31_VIRTUAL_CH14,
	M31_VIRTUAL_CH15,
	
	M31_VIRTUAL_CH16,
	M31_VIRTUAL_CH17,
	M31_VIRTUAL_CH18,
	M31_VIRTUAL_CH19,
	M31_VIRTUAL_CH20,
	M31_VIRTUAL_CH21,
	M31_VIRTUAL_CH22,
	M31_VIRTUAL_CH23,
    
	M31_VIRTUAL_CH24,
	M31_VIRTUAL_CH25,
	M31_VIRTUAL_CH26,
	M31_VIRTUAL_CH27,
	M31_VIRTUAL_CH28,
	M31_VIRTUAL_CH29,
	M31_VIRTUAL_CH30,
	M31_VIRTUAL_CH31
} M31VirtualChannel;

#define	IS_VALID_M31VirtualChannel(__s__)	((__s__) >= M31_VIRTUAL_CH0 && (__s__) <= M31_VIRTUAL_CH31)

typedef enum
{
	M31_CH0,
	M31_CH1,
	M31_CH2,
	M31_CH3
} M31Channel;

#define	IS_VALID_M31Channel(__s__)	((__s__) >= M31_CH0 && (__s__) <= M31_CH3)

typedef enum
{
	M31_1920X1080i              = 0,            // SMPTE274M
	M31_1280X720p               = 2,            // SMPTE296M-2001
	M31_720X480i                = 4,            // ITU-R BT.656.4
	M31_720X576i                = 5,            // ITU-R BT.656.4
	M31_720X480p                = 8,            // SMPTE293M
	M31_720X576p                = 9,            // ITU-REC1358
	M31_1920X1080p              = 30,           // SMPTE274M
	M31_3840X2160_SMPTE435M     = 50,           // Square Division
	M31_3840X2160_SMPTE274M     = 51,           // 2-Sample Interleave
    M31_ARBITRARY_RESOLUTION    = 255           // 2-Sample Interleave
    
} M31VideoFormat;

typedef enum
{
	M31_READCC,
	M31_SETCC,
	M31_WRITECCPLUS,
	M31_READCCPLUS,
	M31_PUSH
} M31CC;

typedef enum
{
	M31_NOTUSE,
	M31_4K2K,
	M31_FULLHD,
	M31_HD,
	M31_SD
} M31ResoType;

#define	IS_VALID_M31ResoType(__s__)	((__s__) >= M31_NOTUSE && (__s__) <= M31_SD)

typedef enum
{
	M31_YCSeparate,
	M31_YCMultiplex
} M31YCMode;

typedef enum
{
    M31_RobustModeStop,
    M31_RobustModeBlue,
	M31_RobustModeColor
} M31RobustMode;

typedef enum
{
	M31_Slave,
	M31_Master
} M31SyncMasterMode;

typedef enum
{
    M31_DefaultPort = 0,
	M31_PortA       = 1,
	M31_PortB       = 2,
	M31_PortC       = 4,
	M31_PortD       = 8
} M31InputPort;

typedef enum
{
    M31_SourceVI,
	M31_SourceVEI
} M31SourceVI;

typedef enum
{
    M31_SourceVA,
	M31_SourceVAReserved
} M31SourceVA;

typedef enum
{
    M31_ChromaMono,
    M31_Chroma420,
	M31_Chroma422
} M31ChromaFormat;

typedef enum
{
    M31_BitDepth8   = 8,
	M31_BitDepth9   = 9,
    M31_BitDepth10  = 10
} M31BitDepth;

typedef enum
{
    M31_PTSModeAuto,
	M31_PTSModeHost
} M31PTSMode;

typedef enum
{
    M31_Progressive,
	M31_Interlace
} M31ScanMode;

typedef enum
{
    M31_EncodeModeDefault,
	M31_EncodeModeMode1,
    M31_EncodeModeMode2
} M31EncodeMode;

typedef enum
{
    M31_ProfileMain     = 1,
	M31_ProfileMain10   = 2,
    M31_ProfileMainSP   = 3,
    M31_Profile422      = 4
} M31Profile;

typedef enum
{
    M31_MainTier,
	M31_HighTier
} M31Tier;

typedef enum
{
    M31_FrameRateUD		= 0,
    M31_FrameRate2398	= 1,
    M31_FrameRate24		= 2,
    M31_FrameRate25		= 3,
    M31_FrameRate2997	= 4,
    M31_FrameRate30		= 5,
    M31_FrameRate50		= 6,
    M31_FrameRate5994	= 7,
    M31_FrameRate60		= 8
} M31FrameRate;

#endif //NTV2M31ENUMS_H

