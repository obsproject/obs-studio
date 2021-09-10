/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2enums.h
	@brief		Enumerations for controlling NTV2 devices.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2ENUMS_H
#define NTV2ENUMS_H

#include "ajatypes.h"	//	Defines NTV2_DEPRECATE (or not)

//	Automated builds remove this and the following several lines		//	__AUTO_BUILD_REMOVE__
#if !defined (AJA_PATCHED)												//	__AUTO_BUILD_REMOVE__
	#define	AJA_DECIMAL_PLACEHOLDER			0							//	__AUTO_BUILD_REMOVE__
	#define	AJA_DATETIME_PLACEHOLDER		"00/00/0000 +8:00:00:00"	//	__AUTO_BUILD_REMOVE__
	#define	AJA_STRING_PLACEHOLDER			"d"							//	__AUTO_BUILD_REMOVE__
#endif																	//	__AUTO_BUILD_REMOVE__


/**
	SDK VERSION
	The next several macro values are patched when the SDK is built by AJA.
**/
#if defined (BUILDING_CMAKE)
#include "version.h" // version.h.in defines are filled out by cmake
#else
#define AJA_NTV2_SDK_VERSION_MAJOR		16			///< @brief	The SDK major version number, an unsigned decimal integer.
#define AJA_NTV2_SDK_VERSION_MINOR		1			///< @brief	The SDK minor version number, an unsigned decimal integer.
#define AJA_NTV2_SDK_VERSION_POINT		0			///< @brief	The SDK "point" release version, an unsigned decimal integer.
#define AJA_NTV2_SDK_BUILD_NUMBER		304			///< @brief	The SDK build number, an unsigned decimal integer.
#define AJA_NTV2_SDK_BUILD_DATETIME		"Thu Jul 22 00:44:31 UTC 2021"		///< @brief	The date and time the SDK was built, in this format: "MM/DD/YYYY +8:hh:mm:ss"
#define AJA_NTV2_SDK_BUILD_TYPE			"b"			///< @brief	The SDK build type, where "a"=alpha, "b"=beta, "d"=development, ""=release.
#endif

#define	AJA_NTV2_SDK_VERSION	((AJA_NTV2_SDK_VERSION_MAJOR << 24) | (AJA_NTV2_SDK_VERSION_MINOR << 16) | (AJA_NTV2_SDK_VERSION_POINT << 8) | (AJA_NTV2_SDK_BUILD_NUMBER))
#define	AJA_NTV2_SDK_VERSION_AT_LEAST(__a__,__b__)		(AJA_NTV2_SDK_VERSION >= (((__a__) << 24) | ((__b__) << 16)))
#define	AJA_NTV2_SDK_VERSION_BEFORE(__a__,__b__)		(AJA_NTV2_SDK_VERSION < (((__a__) << 24) | ((__b__) << 16)))


#if !defined(NTV2_DEPRECATE_14_3)
typedef enum
{
	DEVICETYPE_UNKNOWN=0,
	DEVICETYPE_NTV2=256,
	DEVICETYPE_MAX=256
	#if !defined (NTV2_DEPRECATE)
		,BOARDTYPE_UNKNOWN	= DEVICETYPE_UNKNOWN,
		BOARDTYPE_NTV2		= DEVICETYPE_NTV2,
		BOARDTYPE_MAX		= DEVICETYPE_MAX
	#endif	//	!defined (NTV2_DEPRECATE)
} NTV2DeviceType;		///< @deprecated	Obsolete.
#endif	//	!defined(NTV2_DEPRECATE_14_3)


#if !defined (NTV2_DEPRECATE)
	typedef NTV2DeviceType	NTV2BoardType;
	#define BOARDTYPE_SCANNABLE					(BOARDTYPE_NTV2)
	#define BOARDTYPE_AS_COMPILED				(DEVICETYPE_NTV2)
#endif


/**
	@brief	Identifies a specific AJA NTV2 device model number.
			The NTV2DeviceID is actually the PROM part number for a given device.
			They are used to determine the feature set of the device, and which firmware to flash.
**/
typedef enum
{
	DEVICE_ID_CORVID1					= 0x10244800,	///< @brief	See \ref corvid1corvid3g
	DEVICE_ID_CORVID22					= 0x10293000,	///< @brief	See \ref corvid22
	DEVICE_ID_CORVID24					= 0x10402100,	///< @brief	See \ref corvid24
	DEVICE_ID_CORVID3G					= 0x10294900,	///< @brief	See \ref corvid1corvid3g
	DEVICE_ID_CORVID44					= 0x10565400,	///< @brief	See \ref corvid44
	DEVICE_ID_CORVID44_8KMK				= 0x10832400,	///< @brief	See \ref corvid4412g
	DEVICE_ID_CORVID44_8K				= 0X10832401,	///< @brief	See \ref corvid4412g
	DEVICE_ID_CORVID44_2X4K				= 0X10832402,	///< @brief	See \ref corvid4412g
	DEVICE_ID_CORVID44_PLNR				= 0X10832403,	///< @brief	See \ref corvid4412g
	DEVICE_ID_CORVID88					= 0x10538200,	///< @brief	See \ref corvid88
	DEVICE_ID_CORVIDHBR					= 0x10668200,	///< @brief	See \ref corvidhbr
	DEVICE_ID_CORVIDHEVC				= 0x10634500,	///< @brief	See \ref corvidhevc
	DEVICE_ID_IO4K						= 0x10478300,	///< @brief	See \ref io4kquad
	DEVICE_ID_IO4KPLUS					= 0x10710800,	///< @brief	See \ref io4kplus
	DEVICE_ID_IO4KUFC					= 0x10478350,	///< @brief	See \ref io4kufc
	DEVICE_ID_IOEXPRESS					= 0x10280300,	///< @brief	See \ref ioexpress
	DEVICE_ID_IOIP_2022					= 0x10710850,	///< @brief	See \ref ioip
	DEVICE_ID_IOIP_2110					= 0x10710851,	///< @brief	See \ref ioip
	DEVICE_ID_IOIP_2110_RGB12			= 0x10710852,	///< @brief	See \ref ioip
	DEVICE_ID_IOX3						= 0x10920600,	///< @brief	See \ref iox3
	DEVICE_ID_IOXT						= 0x10378800,	///< @brief	See \ref ioxt
	DEVICE_ID_KONA1						= 0x10756600,	///< @brief	See \ref kona1
	DEVICE_ID_KONA3G					= 0x10294700,	///< @brief	See \ref kona3gufc
	DEVICE_ID_KONA3GQUAD				= 0x10322950,	///< @brief	See \ref kona3gquad
	DEVICE_ID_KONA4						= 0x10518400,	///< @brief	See \ref kona4quad
	DEVICE_ID_KONA4UFC					= 0x10518450,	///< @brief	See \ref kona4ufc
	DEVICE_ID_KONA5						= 0x10798400,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_8KMK				= 0x10798401,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_8K					= 0x10798402,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_2X4K				= 0x10798403,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_3DLUT				= 0x10798404,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE1					= 0x10798405,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE2					= 0x10798406,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE3					= 0x10798407,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE4					= 0x10798408,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE5					= 0x10798409,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE6					= 0x1079840A,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE7					= 0x1079840B,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE8					= 0x1079840C,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE9					= 0x1079840D,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE10				= 0x1079840E,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE11				= 0x1079840F,	///< @brief	See \ref kona5
	DEVICE_ID_KONA5_OE12				= 0x10798410,	///< @brief	See \ref kona5
	DEVICE_ID_KONAHDMI					= 0x10767400,	///< @brief	See \ref konahdmi
	DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K	= 0x10646702,	///< @brief	See \ref konaip
	DEVICE_ID_KONAIP_1RX_1TX_2110		= 0x10646705,	///< @brief	See \ref konaip
	DEVICE_ID_KONAIP_2022				= 0x10646700,	///< @brief	See \ref konaip
	DEVICE_ID_KONAIP_2110				= 0x10646706,	///< @brief	See \ref konaip
	DEVICE_ID_KONAIP_2110_RGB12			= 0x10646707,	///< @brief	See \ref konaip
	DEVICE_ID_KONAIP_2TX_1SFP_J2K		= 0x10646703,	///< @brief	See \ref konaip
	DEVICE_ID_KONAIP_4CH_2SFP			= 0x10646701,	///< @brief	See \ref konaip
	DEVICE_ID_KONALHEPLUS				= 0x10352300,	///< @brief	See \ref konalheplus
	DEVICE_ID_KONALHI					= 0x10266400,	///< @brief	See \ref konalhi
	DEVICE_ID_KONALHIDVI				= 0x10266401,	///< @brief	See \ref konalhi
	DEVICE_ID_TTAP						= 0x10416000,	///< @brief	See \ref ttap
	DEVICE_ID_TTAP_PRO					= 0x10879000,	///< @brief See \ref ttappro
#if !defined (NTV2_DEPRECATE_12_6)
	DEVICE_ID_CORVIDHDBT			= DEVICE_ID_CORVIDHBR,		//	Will deprecate in 12.6
#endif	//	NTV2_DEPRECATE_12_6
#if !defined (NTV2_DEPRECATE_14_0)
	DEVICE_ID_LHE_PLUS				= DEVICE_ID_KONALHEPLUS,	//	Will deprecate eventually
	DEVICE_ID_LHI					= DEVICE_ID_KONALHI,		//	Will deprecate eventually
	DEVICE_ID_LHI_DVI				= DEVICE_ID_KONALHIDVI,		//	Will deprecate eventually
	DEVICE_ID_KONAIP22				= DEVICE_ID_KONAIP_2022,	//	Will deprecate eventually
	DEVICE_ID_KONAIP4I				= DEVICE_ID_KONAIP_4CH_2SFP,//	Will deprecate eventually
	DEVICE_ID_KONAIP_2IN_2OUT		= DEVICE_ID_KONAIP_2022,	//	Will deprecate eventually
	DEVICE_ID_KONAIP_4I				= DEVICE_ID_KONAIP_4CH_2SFP,//	Will deprecate eventually
#endif	//	NTV2_DEPRECATE_14_0
	DEVICE_ID_NOTFOUND				= 0xFFFFFFFF,		///< @brief Invalid or "not found"
	DEVICE_ID_INVALID				= DEVICE_ID_NOTFOUND

} NTV2DeviceID;

#define DEVICE_ID_CORVID44_12G	DEVICE_ID_CORVID44_8KMK
#define DEVICE_ID_KONA5_4X12G	DEVICE_ID_KONA5_8K

#define DEVICE_IS_KONA5_OE(__d__)								\
	(	(__d__) == DEVICE_ID_KONA5_OE1	||						\
		(__d__) == DEVICE_ID_KONA5_OE2	||						\
		(__d__) == DEVICE_ID_KONA5_OE3	||						\
		(__d__) == DEVICE_ID_KONA5_OE4	||						\
		(__d__) == DEVICE_ID_KONA5_OE5	||						\
		(__d__) == DEVICE_ID_KONA5_OE6	||						\
		(__d__) == DEVICE_ID_KONA5_OE7	||						\
		(__d__) == DEVICE_ID_KONA5_OE8	||						\
		(__d__) == DEVICE_ID_KONA5_OE9	||						\
		(__d__) == DEVICE_ID_KONA5_OE10	||						\
		(__d__) == DEVICE_ID_KONA5_OE11	||						\
		(__d__) == DEVICE_ID_KONA5_OE12)

#if !defined (NTV2_DEPRECATE)
	typedef NTV2DeviceID	NTV2BoardID;	///< @deprecated	Use NTV2DeviceID instead. Identifiers with "board" in them are being phased out.

	typedef enum
	{
		BOARDSUBTYPE_AES,		///< @deprecated	Obsolete.
		BOARDSUBTYPE_22,		///< @deprecated	Obsolete.
		BOARDSUBTYPE_NONE		///< @deprecated	Obsolete.
	} NTV2BoardSubType;			///< @deprecated	This is obsolete.
#endif	//	!defined (NTV2_DEPRECATE)

#define NTV2_DEVICE_SUPPORTS_SMPTE2110(__d__)	(		(__d__) == DEVICE_ID_KONAIP_2110			\
													||	(__d__) == DEVICE_ID_KONAIP_2110_RGB12		\
													||	(__d__) == DEVICE_ID_KONAIP_1RX_1TX_2110	\
													||	(__d__) == DEVICE_ID_IOIP_2110				\
													||	(__d__) == DEVICE_ID_IOIP_2110_RGB12	)

#define	NTV2_DEVICE_SUPPORTS_SMPTE2022(__d__)	(		(__d__) == DEVICE_ID_KONAIP_2022			\
													||	(__d__) == DEVICE_ID_IOIP_2022	)


/**
	@brief	Identifies a particular video standard.
**/
typedef enum
{
	NTV2_STANDARD_1080,			///< @brief	Identifies SMPTE HD 1080i or 1080psf
	NTV2_STANDARD_720,			///< @brief	Identifies SMPTE HD 720p
	NTV2_STANDARD_525,			///< @brief	Identifies SMPTE SD 525i
	NTV2_STANDARD_625,			///< @brief	Identifies SMPTE SD 625i
	NTV2_STANDARD_1080p,		///< @brief	Identifies SMPTE HD 1080p
	NTV2_STANDARD_2K,			///< @deprecated	Identifies SMPTE HD 2048x1556psf (1.35 full-aperture film, obsolete in SDK 15.0 and later)
	NTV2_STANDARD_2Kx1080p,		///< @brief	Identifies SMPTE HD 2K1080p
	NTV2_STANDARD_2Kx1080i,		///< @brief	Identifies SMPTE HD 2K1080psf
	NTV2_STANDARD_3840x2160p,	///< @brief	Identifies Ultra-High-Definition (UHD)
	NTV2_STANDARD_4096x2160p,	///< @brief	Identifies 4K
	NTV2_STANDARD_3840HFR,		///< @brief	Identifies high frame-rate UHD
	NTV2_STANDARD_4096HFR,		///< @brief	Identifies high frame-rate 4K
	NTV2_STANDARD_7680,			///< @brief	Identifies UHD2
	NTV2_STANDARD_8192,			///< @brief	Identifies 8K
	NTV2_STANDARD_3840i,		///< @brief	Identifies Ultra-High-Definition (UHD) psf
	NTV2_STANDARD_4096i,		///< @brief	Identifies 4K psf
	NTV2_NUM_STANDARDS,
	NTV2_STANDARD_UNDEFINED	= NTV2_NUM_STANDARDS,
	NTV2_STANDARD_INVALID	= NTV2_NUM_STANDARDS
#if !defined (NTV2_DEPRECATE)
	,
	NTV2_V2_STANDARD_1080		= NTV2_STANDARD_1080,		///< @deprecated	Use NTV2_STANDARD_1080 instead.
	NTV2_V2_STANDARD_720		= NTV2_STANDARD_720,		///< @deprecated	Use NTV2_STANDARD_720 instead.
	NTV2_V2_STANDARD_525		= NTV2_STANDARD_525,		///< @deprecated	Use NTV2_STANDARD_525 instead.
	NTV2_V2_STANDARD_625		= NTV2_STANDARD_625,		///< @deprecated	Use NTV2_STANDARD_625 instead.
	NTV2_V2_STANDARD_1080p		= NTV2_STANDARD_1080p,		///< @deprecated	Use NTV2_STANDARD_1080p instead.
	NTV2_V2_STANDARD_2K			= NTV2_STANDARD_2K,			///< @deprecated	Use NTV2_STANDARD_2K instead.
	NTV2_V2_STANDARD_2Kx1080p	= NTV2_STANDARD_2Kx1080p,	///< @deprecated	Use NTV2_STANDARD_2Kx1080p instead.
	NTV2_V2_STANDARD_2Kx1080i	= NTV2_STANDARD_2Kx1080i,	///< @deprecated	Use NTV2_STANDARD_2Kx1080i instead.
	NTV2_V2_STANDARD_3840x2160p	= NTV2_STANDARD_3840x2160p,	///< @deprecated	Use NTV2_STANDARD_3840x2160p instead.
	NTV2_V2_STANDARD_4096x2160p	= NTV2_STANDARD_4096x2160p,	///< @deprecated	Use NTV2_STANDARD_4096x2160p instead.
	NTV2_V2_STANDARD_3840HFR	= NTV2_STANDARD_3840HFR,	///< @deprecated	Use NTV2_STANDARD_3840HFR instead.
	NTV2_V2_STANDARD_4096HFR	= NTV2_STANDARD_4096HFR,	///< @deprecated	Use NTV2_STANDARD_4096HFR instead.
	NTV2_V2_NUM_STANDARDS		= NTV2_NUM_STANDARDS,		///< @deprecated	Use NTV2_NUM_STANDARDS instead.
	NTV2_V2_STANDARD_UNDEFINED	= NTV2_STANDARD_UNDEFINED,	///< @deprecated	Use NTV2_STANDARD_UNDEFINED instead.
	NTV2_V2_STANDARD_INVALID	= NTV2_STANDARD_INVALID		///< @deprecated	Use NTV2_STANDARD_INVALID instead.
#endif	//	!defined (NTV2_DEPRECATE)
} NTV2Standard;

#define	NTV2_IS_VALID_STANDARD(__s__)			((__s__) >= NTV2_STANDARD_1080 && (__s__) < NTV2_STANDARD_UNDEFINED)
#define NTV2_IS_PROGRESSIVE_STANDARD(__s__)		(		(__s__) == NTV2_STANDARD_1080p			\
													||	(__s__) == NTV2_STANDARD_720			\
													||	(__s__) == NTV2_STANDARD_2Kx1080p		\
													||	(__s__) == NTV2_STANDARD_3840x2160p		\
													||	(__s__) == NTV2_STANDARD_4096x2160p		\
													||	(__s__) == NTV2_STANDARD_3840HFR		\
													||	(__s__) == NTV2_STANDARD_4096HFR		\
													||	(__s__) == NTV2_STANDARD_7680			\
													||	(__s__) == NTV2_STANDARD_8192	)
#define NTV2_IS_SD_STANDARD(__s__)				((__s__) == NTV2_STANDARD_525 || (__s__) == NTV2_STANDARD_625)
#define NTV2_IS_UHD_STANDARD(__s__)				((__s__) == NTV2_STANDARD_3840x2160p	\
													|| (__s__) == NTV2_STANDARD_3840HFR	\
													|| (__s__) == NTV2_STANDARD_3840i)
#define NTV2_IS_4K_STANDARD(__s__)				((__s__) == NTV2_STANDARD_4096x2160p	\
													|| (__s__) == NTV2_STANDARD_4096HFR	\
													|| (__s__) == NTV2_STANDARD_4096i)
#define NTV2_IS_QUAD_STANDARD(__s__)			(NTV2_IS_UHD_STANDARD(__s__) || NTV2_IS_4K_STANDARD(__s__))
#define NTV2_IS_2K1080_STANDARD(__s__)			((__s__) == NTV2_STANDARD_2Kx1080p || (__s__) == NTV2_STANDARD_2Kx1080i)
#define NTV2_IS_UHD2_STANDARD(__s__)			((__s__) == NTV2_STANDARD_7680)
#define NTV2_IS_8K_STANDARD(__s__)				((__s__) == NTV2_STANDARD_8192)
#define NTV2_IS_QUAD_QUAD_STANDARD(__s__)		(NTV2_IS_UHD2_STANDARD(__s__) || NTV2_IS_8K_STANDARD(__s__))
#define NTV2_IS_HFR_STANDARD(__s__)				(NTV2_STANDARD_3840HFR == (__s__) || NTV2_STANDARD_4096HFR == (__s__))

#if !defined (NTV2_DEPRECATE)
	typedef NTV2Standard	NTV2V2Standard;		///< @deprecated	Use NTV2Standard instead.
	#define IS_PROGRESSIVE_NTV2Standard			NTV2_IS_PROGRESSIVE_STANDARD
	#define IS_PROGRESSIVE_STANDARD				NTV2_IS_PROGRESSIVE_STANDARD
	#define	NTV2_IS_VALID_NTV2Standard			NTV2_IS_VALID_STANDARD
	#define	NTV2_IS_VALID_NTV2V2Standard		NTV2_IS_VALID_STANDARD
#endif	//	!defined (NTV2_DEPRECATE)


/**
	@brief	Identifies a particular video frame buffer format. See \ref devicefbformats for details.
**/
typedef enum
{
	NTV2_FBF_FIRST					= 0
	,NTV2_FBF_10BIT_YCBCR			= NTV2_FBF_FIRST	///< @brief	See \ref fbformat10bitycbcr
	,NTV2_FBF_8BIT_YCBCR								///< @brief	See \ref fbformat8bitycbcr
	,NTV2_FBF_ARGB										///< @brief	See \ref fbformats8bitrgb
	,NTV2_FBF_RGBA										///< @brief	See \ref fbformats8bitrgb
	,NTV2_FBF_10BIT_RGB									///< @brief	See \ref fbformats10bitrgb
	,NTV2_FBF_8BIT_YCBCR_YUY2							///< @brief	See \ref fbformatsyuy2
	,NTV2_FBF_ABGR										///< @brief	See \ref fbformats8bitrgb
	,NTV2_FBF_LAST_SD_FBF = NTV2_FBF_ABGR
	,NTV2_FBF_10BIT_DPX									///< @brief	See \ref fbformats10bitrgbdpx
	,NTV2_FBF_10BIT_YCBCR_DPX							///< @brief	See \ref fbformats10bitycbcrdpx
	,NTV2_FBF_8BIT_DVCPRO								///< @brief	See \ref fbformats8bitdvcpro
	,NTV2_FBF_8BIT_YCBCR_420PL3							///< @brief	See \ref fbformats8bitycbcrplanar3
	,NTV2_FBF_8BIT_HDV									///< @brief	See \ref fbformats8bithdv
	,NTV2_FBF_24BIT_RGB									///< @brief	See \ref fbformats24bitrgb
	,NTV2_FBF_24BIT_BGR									///< @brief	See \ref fbformats24bitbgr
	,NTV2_FBF_10BIT_YCBCRA								///< @brief	10-Bit YCbCrA
	,NTV2_FBF_10BIT_DPX_LE								///< @brief	10-Bit DPX Little-Endian
	,NTV2_FBF_48BIT_RGB									///< @brief	See \ref fbformats48bitrgb
	,NTV2_FBF_12BIT_RGB_PACKED							///< @brief	See \ref fbformats12bitpackedrgb
	,NTV2_FBF_PRORES_DVCPRO								///< @brief	Apple ProRes DVC Pro
	,NTV2_FBF_PRORES_HDV								///< @brief	Apple ProRes HDV
	,NTV2_FBF_10BIT_RGB_PACKED							///< @brief	10-Bit Packed RGB
	,NTV2_FBF_10BIT_ARGB								///< @brief	10-Bit ARGB
	,NTV2_FBF_16BIT_ARGB								///< @brief	16-Bit ARGB
	,NTV2_FBF_8BIT_YCBCR_422PL3							///< @brief	See \ref fbformats8bitycbcr422pl3
	,NTV2_FBF_10BIT_RAW_RGB								///< @brief	10-Bit Raw RGB
	,NTV2_FBF_10BIT_RAW_YCBCR							///< @brief	See \ref fbformats10bitcion
	,NTV2_FBF_10BIT_YCBCR_420PL3_LE						///< @brief	See \ref fbformats10bitycbcr420pl3
	,NTV2_FBF_10BIT_YCBCR_422PL3_LE						///< @brief	See \ref fbformats10bitycbcr422pl3
	,NTV2_FBF_10BIT_YCBCR_420PL2						///< @brief	10-Bit 4:2:0 2-Plane YCbCr
	,NTV2_FBF_10BIT_YCBCR_422PL2						///< @brief	10-Bit 4:2:2 2-Plane YCbCr
	,NTV2_FBF_8BIT_YCBCR_420PL2							///< @brief	8-Bit 4:2:0 2-Plane YCbCr
	,NTV2_FBF_8BIT_YCBCR_422PL2							///< @brief	8-Bit 4:2:2 2-Plane YCbCr
	,NTV2_FBF_LAST
	,NTV2_FBF_NUMFRAMEBUFFERFORMATS	= NTV2_FBF_LAST
	,NTV2_FBF_INVALID				= NTV2_FBF_NUMFRAMEBUFFERFORMATS
} NTV2FrameBufferFormat;

#if !defined(NTV2_DEPRECATE_14_0)
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_8BIT_QREZ,					NTV2_FBF_8BIT_YCBCR_420PL3);		///< @deprecated	Use NTV2_FBF_8BIT_YCBCR_420PL3 instead.
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_10BIT_DPX_LITTLEENDIAN,	NTV2_FBF_10BIT_DPX_LE);			///< @deprecated	Use NTV2_FBF_10BIT_DPX_LE instead.
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_UNUSED_23,					NTV2_FBF_8BIT_YCBCR_422PL3);		///< @deprecated	Use NTV2_FBF_8BIT_YCBCR_422PL3 instead.
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_UNUSED_26,					NTV2_FBF_10BIT_YCBCR_420PL3_LE);	///< @deprecated	Use NTV2_FBF_10BIT_YCBCR_420PL3_LE instead.
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_UNUSED_27,					NTV2_FBF_10BIT_YCBCR_422PL3_LE);	///< @deprecated	Use NTV2_FBF_10BIT_YCBCR_422PL3_LE instead.
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_10BIT_YCBCR_420PL,			NTV2_FBF_10BIT_YCBCR_420PL2);		///< @deprecated	Use NTV2_FBF_10BIT_YCBCR_420PL2 instead.
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_10BIT_YCBCR_422PL,			NTV2_FBF_10BIT_YCBCR_422PL2);		///< @deprecated	Use NTV2_FBF_10BIT_YCBCR_422PL2 instead.
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_8BIT_YCBCR_420PL,			NTV2_FBF_8BIT_YCBCR_420PL2);		///< @deprecated	Use NTV2_FBF_8BIT_YCBCR_420PL2 instead.
	NTV2_DEPRECATED_vi(const NTV2FrameBufferFormat NTV2_FBF_8BIT_YCBCR_422PL,			NTV2_FBF_8BIT_YCBCR_422PL2);		///< @deprecated	Use NTV2_FBF_8BIT_YCBCR_422PL2 instead.
#endif	//	NTV2_DEPRECATE_14_0


typedef NTV2FrameBufferFormat	NTV2PixelFormat;	///< @brief	An alias for NTV2FrameBufferFormat.


#define	NTV2_IS_VALID_FRAME_BUFFER_FORMAT(__s__)	((__s__) >= NTV2_FBF_10BIT_YCBCR  &&  (__s__) < NTV2_FBF_NUMFRAMEBUFFERFORMATS)

#define	NTV2_IS_VALID_FBF(__s__)					((__s__) >= NTV2_FBF_10BIT_YCBCR  &&  (__s__) < NTV2_FBF_NUMFRAMEBUFFERFORMATS)

#define	NTV2_IS_FBF_PLANAR(__s__)					(		(__s__) == NTV2_FBF_8BIT_YCBCR_420PL3		\
														||	(__s__) == NTV2_FBF_8BIT_YCBCR_422PL3		\
														||	(__s__) == NTV2_FBF_10BIT_YCBCR_420PL3_LE	\
														||	(__s__) == NTV2_FBF_10BIT_YCBCR_422PL3_LE	\
														||	(__s__) == NTV2_FBF_10BIT_YCBCR_420PL2		\
														||	(__s__) == NTV2_FBF_10BIT_YCBCR_422PL2		\
														||	(__s__) == NTV2_FBF_8BIT_YCBCR_420PL2		\
														||	(__s__) == NTV2_FBF_8BIT_YCBCR_422PL2		\
													)

#define	NTV2_IS_VALID_PLANAR_FRAME_BUFFER_FORMAT(__s__)		(NTV2_IS_FBF_PLANAR(__s__))

#define NTV2_IS_FBF_PRORES(__fbf__) 	(		(__fbf__) == NTV2_FBF_PRORES_DVCPRO				\
											||	(__fbf__) == NTV2_FBF_PRORES_HDV				\
										)

#define NTV2_IS_FBF_RGB(__fbf__)		(		(__fbf__) == NTV2_FBF_ARGB						\
											||	(__fbf__) == NTV2_FBF_RGBA						\
											||	(__fbf__) == NTV2_FBF_10BIT_RGB					\
											||	(__fbf__) == NTV2_FBF_ABGR						\
											||	(__fbf__) == NTV2_FBF_10BIT_DPX					\
											||	(__fbf__) == NTV2_FBF_24BIT_RGB					\
											||	(__fbf__) == NTV2_FBF_24BIT_BGR					\
											||	(__fbf__) == NTV2_FBF_10BIT_DPX_LE				\
											||	(__fbf__) == NTV2_FBF_48BIT_RGB					\
											||	(__fbf__) == NTV2_FBF_12BIT_RGB_PACKED			\
											||	(__fbf__) == NTV2_FBF_10BIT_RGB_PACKED			\
											||	(__fbf__) == NTV2_FBF_10BIT_ARGB				\
											||	(__fbf__) == NTV2_FBF_16BIT_ARGB				\
											||	(__fbf__) == NTV2_FBF_10BIT_RAW_RGB				\
										)

#define NTV2_IS_FBF_8BIT(__fbf__)		(		(__fbf__) == NTV2_FBF_8BIT_YCBCR				\
											||	(__fbf__) == NTV2_FBF_ARGB						\
											||	(__fbf__) == NTV2_FBF_RGBA						\
											||	(__fbf__) == NTV2_FBF_8BIT_YCBCR_YUY2			\
											||	(__fbf__) == NTV2_FBF_ABGR						\
											||	(__fbf__) == NTV2_FBF_8BIT_DVCPRO				\
										)

#define NTV2_IS_FBF_10BIT(__fbf__)		(		(__fbf__) == NTV2_FBF_10BIT_YCBCR				\
											||	(__fbf__) == NTV2_FBF_10BIT_RGB					\
											||	(__fbf__) == NTV2_FBF_10BIT_DPX					\
											||	(__fbf__) == NTV2_FBF_10BIT_YCBCR_DPX			\
											||	(__fbf__) == NTV2_FBF_10BIT_YCBCRA				\
											||	(__fbf__) == NTV2_FBF_10BIT_DPX_LE				\
											||	(__fbf__) == NTV2_FBF_10BIT_RGB_PACKED			\
											||	(__fbf__) == NTV2_FBF_10BIT_ARGB				\
											||	(__fbf__) == NTV2_FBF_10BIT_RAW_RGB				\
											||	(__fbf__) == NTV2_FBF_10BIT_RAW_YCBCR			\
											||	(__fbf__) == NTV2_FBF_10BIT_YCBCR_420PL3_LE		\
											||	(__fbf__) == NTV2_FBF_10BIT_YCBCR_422PL3_LE		\
											||	(__fbf__) == NTV2_FBF_10BIT_YCBCR_420PL2		\
											||	(__fbf__) == NTV2_FBF_10BIT_YCBCR_422PL2		\
										)

#define	NTV2_FBF_HAS_ALPHA(__fbf__)		(		(__fbf__) == NTV2_FBF_ARGB						\
											||	(__fbf__) == NTV2_FBF_RGBA						\
											||	(__fbf__) == NTV2_FBF_ABGR						\
											||	(__fbf__) == NTV2_FBF_10BIT_ARGB				\
											||	(__fbf__) == NTV2_FBF_16BIT_ARGB				\
											||	(__fbf__) == NTV2_FBF_10BIT_YCBCRA				\
										)

#define	NTV2_FBF_IS_RAW(__fbf__)		(		(__fbf__) == NTV2_FBF_10BIT_RAW_RGB				\
											||	(__fbf__) == NTV2_FBF_10BIT_RAW_YCBCR			\
										)

#define NTV2_FBF_IS_YCBCR(__fbf__)		(		!NTV2_IS_FBF_RGB(__fbf__)						\
											&&  !NTV2_FBF_IS_RAW(__fbf__)						\
											&&  !NTV2_IS_FBF_PRORES(__fbf__)					\
										)

#define NTV2_IS_FBF_12BIT_RGB(__fbf__)	(		(__fbf__) == NTV2_FBF_48BIT_RGB					\
											||	(__fbf__) == NTV2_FBF_12BIT_RGB_PACKED			\
										)


/**
	@brief	Identifies a particular video frame geometry.
**/
typedef enum
{
	NTV2_FG_1920x1080	= 0,	///< @brief	1920x1080, for 1080i and 1080p, ::NTV2_VANCMODE_OFF
	NTV2_FG_FIRST		= NTV2_FG_1920x1080,	///< @brief	The ordinally first geometry (New in SDK 16.0)
	NTV2_FG_1280x720	= 1,	///< @brief	1280x720, for 720p, ::NTV2_VANCMODE_OFF
	NTV2_FG_720x486		= 2,	///< @brief	720x486, for NTSC 525i and 525p60, ::NTV2_VANCMODE_OFF
	NTV2_FG_720x576		= 3,	///< @brief	720x576, for PAL 625i, ::NTV2_VANCMODE_OFF
	NTV2_FG_1920x1114	= 4,	///< @brief	1920x1080, ::NTV2_VANCMODE_TALLER
	NTV2_FG_2048x1114	= 5,	///< @brief	2048x1080, ::NTV2_VANCMODE_TALLER
	NTV2_FG_720x508		= 6,	///< @brief	720x486, for NTSC 525i, ::NTV2_VANCMODE_TALL
	NTV2_FG_720x598		= 7,	///< @brief	720x576, for PAL 625i, ::NTV2_VANCMODE_TALL
	NTV2_FG_1920x1112	= 8,	///< @brief	1920x1080, for 1080i and 1080p, ::NTV2_VANCMODE_TALL
	NTV2_FG_1280x740	= 9,	///< @brief	1280x720, for 720p, ::NTV2_VANCMODE_TALL
	NTV2_FG_2048x1080	= 10,	///< @brief	2048x1080, for 2Kx1080p, ::NTV2_VANCMODE_OFF
	NTV2_FG_2048x1556	= 11,	///< @brief	2048x1556, for 2Kx1556psf film format, ::NTV2_VANCMODE_OFF
	NTV2_FG_2048x1588 	= 12,	///< @brief	2048x1556, for 2Kx1556psf film format, ::NTV2_VANCMODE_TALL
	NTV2_FG_2048x1112 	= 13,	///< @brief	2048x1080, for 2Kx1080p, ::NTV2_VANCMODE_TALL
	NTV2_FG_720x514 	= 14,	///< @brief	720x486, for NTSC 525i and 525p60, ::NTV2_VANCMODE_TALLER
	NTV2_FG_720x612 	= 15,	///< @brief	720x576, for PAL 625i, ::NTV2_VANCMODE_TALLER
	NTV2_FG_4x1920x1080 = 16,	///< @brief	3840x2160, for UHD, ::NTV2_VANCMODE_OFF
	NTV2_FG_4x2048x1080 = 17,	///< @brief	4096x2160, for 4K, ::NTV2_VANCMODE_OFF
	NTV2_FG_4x3840x2160	= 18,	///< @brief	7680x4320, for UHD2, ::NTV2_VANCMODE_OFF
	NTV2_FG_4x4096x2160	= 19,	///< @brief	8192x4320, for 8K, ::NTV2_VANCMODE_OFF
	NTV2_FG_LAST		= NTV2_FG_4x4096x2160,	///< @brief	The ordinally last geometry (New in SDK 16.0)
	NTV2_FG_NUMFRAMEGEOMETRIES,
	NTV2_FG_INVALID	= NTV2_FG_NUMFRAMEGEOMETRIES
} NTV2FrameGeometry;

#define	NTV2_IS_VALID_NTV2FrameGeometry(__s__)	((__s__) >= NTV2_FG_FIRST && (__s__) < NTV2_FG_NUMFRAMEGEOMETRIES)

#define NTV2_IS_QUAD_QUAD_FRAME_GEOMETRY(geom) \
	(geom == NTV2_FG_4x3840x2160 || geom == NTV2_FG_4x4096x2160)
	
#define NTV2_IS_QUAD_FRAME_GEOMETRY(geom) \
	( geom >= NTV2_FG_4x1920x1080 && geom <= NTV2_FG_4x2048x1080 )

#define NTV2_IS_2K_1080_FRAME_GEOMETRY(geom) \
	(	geom == NTV2_FG_2048x1114 || \
		geom == NTV2_FG_2048x1080 || \
		geom == NTV2_FG_2048x1112 )

#define NTV2_IS_TALL_VANC_GEOMETRY(__g__)		(		(__g__) == NTV2_FG_720x508				\
													||	(__g__) == NTV2_FG_720x598				\
													||	(__g__) == NTV2_FG_1920x1112			\
													||	(__g__) == NTV2_FG_1280x740				\
													||	(__g__) == NTV2_FG_2048x1588			\
													||	(__g__) == NTV2_FG_2048x1112	)

#define NTV2_IS_TALLER_VANC_GEOMETRY(__g__)		(		(__g__) == NTV2_FG_1920x1114			\
													||  (__g__) == NTV2_FG_2048x1114			\
													||  (__g__) == NTV2_FG_720x514				\
													||  (__g__) == NTV2_FG_720x612	)

#define NTV2_IS_VANC_GEOMETRY(__g__)			(NTV2_IS_TALL_VANC_GEOMETRY(__g__) || NTV2_IS_TALLER_VANC_GEOMETRY(__g__))


/**
	@brief	Identifies a particular video frame rate.
	@note	These match the hardware register values.
**/
typedef enum
{
	NTV2_FRAMERATE_UNKNOWN	= 0,	///< @brief	Represents an unknown or invalid frame rate
	NTV2_FRAMERATE_6000		= 1,	///< @brief	60 frames per second
	NTV2_FRAMERATE_FIRST	= NTV2_FRAMERATE_6000,	///< @brief	First ordinal value (new in SDK 16.0)
	NTV2_FRAMERATE_5994		= 2,	///< @brief	Fractional rate of 60,000 frames per 1,001 seconds
	NTV2_FRAMERATE_3000		= 3,	///< @brief	30 frames per second
	NTV2_FRAMERATE_2997		= 4,	///< @brief	Fractional rate of 30,000 frames per 1,001 seconds
	NTV2_FRAMERATE_2500		= 5,	///< @brief	25 frames per second
	NTV2_FRAMERATE_2400		= 6,	///< @brief	24 frames per second
	NTV2_FRAMERATE_2398		= 7,	///< @brief	Fractional rate of 24,000 frames per 1,001 seconds
	NTV2_FRAMERATE_5000		= 8,	///< @brief	50 frames per second
	NTV2_FRAMERATE_4800		= 9,	///< @brief	48 frames per second
	NTV2_FRAMERATE_4795		= 10,	///< @brief	Fractional rate of 48,000 frames per 1,001 seconds
	NTV2_FRAMERATE_12000	= 11,	///< @brief	120 frames per second
	NTV2_FRAMERATE_11988	= 12,	///< @brief	Fractional rate of 120,000 frames per 1,001 seconds
	NTV2_FRAMERATE_1500		= 13,	///< @brief	15 frames per second
	NTV2_FRAMERATE_1498		= 14,	///< @brief	Fractional rate of 15,000 frames per 1,001 seconds
#if !defined(NTV2_DEPRECATE_16_0)
	// These were never implemented, and are here so old code will still compile
	NTV2_FRAMERATE_1900		= 15,	///< @deprecated	19 fps -- obsolete, was ordinal value 9 in old SDKs
	NTV2_FRAMERATE_1898		= 16,	///< @deprecated	19,000 frames per 1,001 seconds -- obsolete, was ordinal value 10 in old SDKs
	NTV2_FRAMERATE_1800		= 17,	///< @deprecated	18 fps -- obsolete, was ordinal value 11 in old SDKs
	NTV2_FRAMERATE_1798		= 18,	///< @deprecated	18,000 frames per 1,001 seconds -- obsolete, was ordinal value 12 in old SDKs
	NTV2_FRAMERATE_LAST		= NTV2_FRAMERATE_1798,	///< @brief	Last ordinal value (new in SDK 16.0)
#else	//	!defined(NTV2_DEPRECATE_16_0)
	NTV2_FRAMERATE_LAST		= NTV2_FRAMERATE_1498,	///< @brief	Last ordinal value (new in SDK 16.0)
#endif	//	!defined(NTV2_DEPRECATE_16_0)
	NTV2_NUM_FRAMERATES,
	NTV2_FRAMERATE_INVALID = NTV2_FRAMERATE_UNKNOWN
} NTV2FrameRate;

#define	NTV2_IS_VALID_NTV2FrameRate(__r__)		((__r__) >= NTV2_FRAMERATE_6000 && (__r__) < NTV2_NUM_FRAMERATES)
#define	NTV2_IS_SUPPORTED_NTV2FrameRate(__r__)	((__r__) >= NTV2_FRAMERATE_6000 && (__r__) <= NTV2_FRAMERATE_1498)

#if !defined(NTV2_DEPRECATE_16_0)
	#define NTV2_IS_FRACTIONAL_NTV2FrameRate(__r__)						\
		(	(__r__) == NTV2_FRAMERATE_1498	||							\
			(__r__) == NTV2_FRAMERATE_1798	||							\
			(__r__) == NTV2_FRAMERATE_1898	||							\
			(__r__) == NTV2_FRAMERATE_2398	||							\
			(__r__) == NTV2_FRAMERATE_2997	||							\
			(__r__) == NTV2_FRAMERATE_4795	||							\
			(__r__) == NTV2_FRAMERATE_5994	||							\
			(__r__) == NTV2_FRAMERATE_11988	)
#else	//	!defined(NTV2_DEPRECATE_16_0)
	#define NTV2_IS_FRACTIONAL_NTV2FrameRate(__r__)						\
		(	(__r__) == NTV2_FRAMERATE_1498	||							\
			(__r__) == NTV2_FRAMERATE_2398	||							\
			(__r__) == NTV2_FRAMERATE_2997	||							\
			(__r__) == NTV2_FRAMERATE_4795	||							\
			(__r__) == NTV2_FRAMERATE_5994	||							\
			(__r__) == NTV2_FRAMERATE_11988	)
#endif	//	!defined(NTV2_DEPRECATE_16_0)

#define NTV2_IS_HIGH_NTV2FrameRate(__r__)							\
	(	(__r__) == NTV2_FRAMERATE_4795	||							\
		(__r__) == NTV2_FRAMERATE_4800	||							\
		(__r__) == NTV2_FRAMERATE_5000	||							\
		(__r__) == NTV2_FRAMERATE_5994	||							\
		(__r__) == NTV2_FRAMERATE_6000	||							\
		(__r__) == NTV2_FRAMERATE_11988	||							\
		(__r__) == NTV2_FRAMERATE_12000	)


typedef enum
{
	NTV2_SG_UNKNOWN	= 0,
	NTV2_SG_525		= 1,
	NTV2_SG_625		= 2,
	NTV2_SG_750		= 3,
	NTV2_SG_1125	= 4,
	NTV2_SG_1250	= 5,
	NTV2_SG_2Kx1080	= 8,
	NTV2_SG_2Kx1556	= 9
} NTV2ScanGeometry;


// IMPORTANT When adding to the NTV2VideoFormat enum, don't forget to:
//		Add a corresponding case to GetNTV2FrameGeometryFromVideoFormat in r2deviceservices.cpp
//		Add a corresponding case to GetNTV2QuarterSizedVideoFormat in ntv2utils.cpp
//		Add a corresponding case to GetNTV2StandardFromVideoFormat in ntv2utils.cpp
//		Add a corresponding case to GetActiveVideoSize in ntv2utils.cpp
//		Add a corresponding case to GetNTV2FrameRateFromVideoFormat in ntv2utils.cpp
//		Add a corresponding case to GetDisplayWidth in ntv2utils.cpp
//		Add a corresponding case to GetDisplayHeight in ntv2utils.cpp
//		Add a corresponding string to NTV2VideoFormatStrings in ntv2utils.cpp
//		Add a corresponding timing to NTV2KonaHDTiming in ntv2register.cpp
//		Add a corresponding timing to NTV2KonaLHTiming in ntv2register.cpp
//		Add a corresponding case to SetVPIDData in ntv2vpid.cpp
//		Add a corresponding case to NTV2VideoFromatString in ntv2debug.cpp
//		Add a corresponding case to NTV2DeviceGetVideoFormatFromState_Ex in sdkgen/*.csv for ntv2devicefeatures.cpp
//		Consider adding a new test case to commonapps/hi5_4k_diag/main.cpp
//		Add a corresponding case to AJAVideoFormatNTV2Table in commonclasses/ntv2videoformataja.cpp
//			(If the format is really new, videotypes.h in ajabase/common may need updating)
//		Update the #defines following this enum

/**
	@brief	Identifies a particular video format.
**/
typedef enum _NTV2VideoFormat
{
	 NTV2_FORMAT_UNKNOWN

	,NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT			= 1
	,NTV2_FORMAT_FIRST_STANDARD_DEF_FORMAT		= 32
	,NTV2_FORMAT_FIRST_2K_DEF_FORMAT			= 64
	,NTV2_FORMAT_FIRST_4K_DEF_FORMAT			= 80
	,NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT2			= 110
	,NTV2_FORMAT_FIRST_UHD_TSI_DEF_FORMAT		= 200
	,NTV2_FORMAT_FIRST_4K_TSI_DEF_FORMAT		= 250
	,NTV2_FORMAT_FIRST_4K_DEF_FORMAT2			= 300
	,NTV2_FORMAT_FIRST_UHD2_DEF_FORMAT			= 350
	,NTV2_FORMAT_FIRST_UHD2_FULL_DEF_FORMAT		= 400

	,NTV2_FORMAT_1080i_5000					= NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT
	,NTV2_FORMAT_1080i_5994
	,NTV2_FORMAT_1080i_6000
	,NTV2_FORMAT_720p_5994
	,NTV2_FORMAT_720p_6000
	,NTV2_FORMAT_1080psf_2398
	,NTV2_FORMAT_1080psf_2400
	,NTV2_FORMAT_1080p_2997
	,NTV2_FORMAT_1080p_3000
	,NTV2_FORMAT_1080p_2500
	,NTV2_FORMAT_1080p_2398
	,NTV2_FORMAT_1080p_2400
	,NTV2_FORMAT_1080p_2K_2398
	,NTV2_FORMAT_1080p_2K_2400
	,NTV2_FORMAT_1080psf_2K_2398
	,NTV2_FORMAT_1080psf_2K_2400
	,NTV2_FORMAT_720p_5000
	,NTV2_FORMAT_1080p_5000_B
	,NTV2_FORMAT_1080p_5994_B
	,NTV2_FORMAT_1080p_6000_B
	,NTV2_FORMAT_720p_2398
	,NTV2_FORMAT_720p_2500
	,NTV2_FORMAT_1080p_5000_A
	,NTV2_FORMAT_1080p_5994_A
	,NTV2_FORMAT_1080p_6000_A
	,NTV2_FORMAT_1080p_2K_2500
	,NTV2_FORMAT_1080psf_2K_2500
	,NTV2_FORMAT_1080psf_2500_2
	,NTV2_FORMAT_1080psf_2997_2
	,NTV2_FORMAT_1080psf_3000_2
	,NTV2_FORMAT_END_HIGH_DEF_FORMATS

	,NTV2_FORMAT_525_5994					= NTV2_FORMAT_FIRST_STANDARD_DEF_FORMAT
	,NTV2_FORMAT_625_5000
	,NTV2_FORMAT_525_2398
	,NTV2_FORMAT_525_2400
	,NTV2_FORMAT_525psf_2997
	,NTV2_FORMAT_625psf_2500
	,NTV2_FORMAT_END_STANDARD_DEF_FORMATS

	,NTV2_FORMAT_2K_1498					= NTV2_FORMAT_FIRST_2K_DEF_FORMAT
	,NTV2_FORMAT_2K_1500
	,NTV2_FORMAT_2K_2398
	,NTV2_FORMAT_2K_2400
	,NTV2_FORMAT_2K_2500
	,NTV2_FORMAT_END_2K_DEF_FORMATS

	,NTV2_FORMAT_4x1920x1080psf_2398		= NTV2_FORMAT_FIRST_4K_DEF_FORMAT
	,NTV2_FORMAT_4x1920x1080psf_2400
	,NTV2_FORMAT_4x1920x1080psf_2500
	,NTV2_FORMAT_4x1920x1080p_2398
	,NTV2_FORMAT_4x1920x1080p_2400
	,NTV2_FORMAT_4x1920x1080p_2500
	,NTV2_FORMAT_4x2048x1080psf_2398
	,NTV2_FORMAT_4x2048x1080psf_2400
	,NTV2_FORMAT_4x2048x1080psf_2500
	,NTV2_FORMAT_4x2048x1080p_2398
	,NTV2_FORMAT_4x2048x1080p_2400
	,NTV2_FORMAT_4x2048x1080p_2500
	,NTV2_FORMAT_4x1920x1080p_2997
	,NTV2_FORMAT_4x1920x1080p_3000
	,NTV2_FORMAT_4x1920x1080psf_2997
	,NTV2_FORMAT_4x1920x1080psf_3000
	,NTV2_FORMAT_4x2048x1080p_2997
	,NTV2_FORMAT_4x2048x1080p_3000
	,NTV2_FORMAT_4x2048x1080psf_2997
	,NTV2_FORMAT_4x2048x1080psf_3000
	,NTV2_FORMAT_4x1920x1080p_5000
	,NTV2_FORMAT_4x1920x1080p_5994
	,NTV2_FORMAT_4x1920x1080p_6000
	,NTV2_FORMAT_4x2048x1080p_5000
	,NTV2_FORMAT_4x2048x1080p_5994
	,NTV2_FORMAT_4x2048x1080p_6000
	,NTV2_FORMAT_4x2048x1080p_4795
	,NTV2_FORMAT_4x2048x1080p_4800
	,NTV2_FORMAT_4x2048x1080p_11988
	,NTV2_FORMAT_4x2048x1080p_12000
	,NTV2_FORMAT_END_4K_DEF_FORMATS

	,NTV2_FORMAT_1080p_2K_6000_A			= NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT2
	,NTV2_FORMAT_1080p_2K_5994_A
	,NTV2_FORMAT_1080p_2K_2997
	,NTV2_FORMAT_1080p_2K_3000
	,NTV2_FORMAT_1080p_2K_5000_A
	,NTV2_FORMAT_1080p_2K_4795_A
	,NTV2_FORMAT_1080p_2K_4800_A
	,NTV2_FORMAT_1080p_2K_4795_B
	,NTV2_FORMAT_1080p_2K_4800_B
	,NTV2_FORMAT_1080p_2K_5000_B
	,NTV2_FORMAT_1080p_2K_5994_B
	,NTV2_FORMAT_1080p_2K_6000_B
	,NTV2_FORMAT_END_HIGH_DEF_FORMATS2

	,NTV2_FORMAT_3840x2160psf_2398		= NTV2_FORMAT_FIRST_UHD_TSI_DEF_FORMAT
	,NTV2_FORMAT_3840x2160psf_2400
	,NTV2_FORMAT_3840x2160psf_2500
	,NTV2_FORMAT_3840x2160p_2398
	,NTV2_FORMAT_3840x2160p_2400
	,NTV2_FORMAT_3840x2160p_2500
	,NTV2_FORMAT_3840x2160p_2997
	,NTV2_FORMAT_3840x2160p_3000
	,NTV2_FORMAT_3840x2160psf_2997
	,NTV2_FORMAT_3840x2160psf_3000
	,NTV2_FORMAT_3840x2160p_5000
	,NTV2_FORMAT_3840x2160p_5994
	,NTV2_FORMAT_3840x2160p_6000
	,NTV2_FORMAT_3840x2160p_5000_B
	,NTV2_FORMAT_3840x2160p_5994_B
	,NTV2_FORMAT_3840x2160p_6000_B

	,NTV2_FORMAT_4096x2160psf_2398		= NTV2_FORMAT_FIRST_4K_TSI_DEF_FORMAT
	,NTV2_FORMAT_4096x2160psf_2400
	,NTV2_FORMAT_4096x2160psf_2500
	,NTV2_FORMAT_4096x2160p_2398
	,NTV2_FORMAT_4096x2160p_2400
	,NTV2_FORMAT_4096x2160p_2500
	,NTV2_FORMAT_4096x2160p_2997
	,NTV2_FORMAT_4096x2160p_3000
	,NTV2_FORMAT_4096x2160psf_2997
	,NTV2_FORMAT_4096x2160psf_3000
	,NTV2_FORMAT_4096x2160p_4795
	,NTV2_FORMAT_4096x2160p_4800
	,NTV2_FORMAT_4096x2160p_5000
	,NTV2_FORMAT_4096x2160p_5994
	,NTV2_FORMAT_4096x2160p_6000
	,NTV2_FORMAT_4096x2160p_11988
	,NTV2_FORMAT_4096x2160p_12000
	,NTV2_FORMAT_4096x2160p_4795_B
	,NTV2_FORMAT_4096x2160p_4800_B
	,NTV2_FORMAT_4096x2160p_5000_B
	,NTV2_FORMAT_4096x2160p_5994_B
	,NTV2_FORMAT_4096x2160p_6000_B
	,NTV2_FORMAT_END_4K_TSI_DEF_FORMATS
		
	,NTV2_FORMAT_4x1920x1080p_5000_B	= NTV2_FORMAT_FIRST_4K_DEF_FORMAT2
	,NTV2_FORMAT_4x1920x1080p_5994_B
	,NTV2_FORMAT_4x1920x1080p_6000_B
	,NTV2_FORMAT_4x2048x1080p_5000_B
	,NTV2_FORMAT_4x2048x1080p_5994_B
	,NTV2_FORMAT_4x2048x1080p_6000_B
	,NTV2_FORMAT_4x2048x1080p_4795_B
	,NTV2_FORMAT_4x2048x1080p_4800_B
	,NTV2_FORMAT_END_4K_DEF_FORMATS2
		
	,NTV2_FORMAT_4x3840x2160p_2398		= NTV2_FORMAT_FIRST_UHD2_DEF_FORMAT
	,NTV2_FORMAT_4x3840x2160p_2400
	,NTV2_FORMAT_4x3840x2160p_2500
	,NTV2_FORMAT_4x3840x2160p_2997
	,NTV2_FORMAT_4x3840x2160p_3000
	,NTV2_FORMAT_4x3840x2160p_5000
	,NTV2_FORMAT_4x3840x2160p_5994
	,NTV2_FORMAT_4x3840x2160p_6000
	,NTV2_FORMAT_4x3840x2160p_5000_B
	,NTV2_FORMAT_4x3840x2160p_5994_B
	,NTV2_FORMAT_4x3840x2160p_6000_B
	,NTV2_FORMAT_END_UHD2_DEF_FORMATS

	,NTV2_FORMAT_4x4096x2160p_2398		= NTV2_FORMAT_FIRST_UHD2_FULL_DEF_FORMAT
	,NTV2_FORMAT_4x4096x2160p_2400
	,NTV2_FORMAT_4x4096x2160p_2500
	,NTV2_FORMAT_4x4096x2160p_2997
	,NTV2_FORMAT_4x4096x2160p_3000
	,NTV2_FORMAT_4x4096x2160p_4795
	,NTV2_FORMAT_4x4096x2160p_4800
	,NTV2_FORMAT_4x4096x2160p_5000
	,NTV2_FORMAT_4x4096x2160p_5994
	,NTV2_FORMAT_4x4096x2160p_6000
	,NTV2_FORMAT_4x4096x2160p_4795_B
	,NTV2_FORMAT_4x4096x2160p_4800_B
	,NTV2_FORMAT_4x4096x2160p_5000_B
	,NTV2_FORMAT_4x4096x2160p_5994_B
	,NTV2_FORMAT_4x4096x2160p_6000_B
	,NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS

	,NTV2_MAX_NUM_VIDEO_FORMATS = NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS
} NTV2VideoFormat;


#if !defined(NTV2_DEPRECATE_14_2)
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080psf_2500,			NTV2_FORMAT_1080i_5000);	///< @deprecated	Use NTV2_FORMAT_1080i_5000 instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080psf_2997,			NTV2_FORMAT_1080i_5994);	///< @deprecated	Use NTV2_FORMAT_1080i_5994 instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080psf_3000,			NTV2_FORMAT_1080i_6000);	///< @deprecated	Use NTV2_FORMAT_1080i_6000 instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_DEPRECATED_525_5994,	NTV2_FORMAT_1080p_2K_2398);	///< @deprecated	Use NTV2_FORMAT_1080p_2K_2398 instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_DEPRECATED_625_5000,	NTV2_FORMAT_1080p_2K_2400);	///< @deprecated	Use NTV2_FORMAT_1080p_2K_2400 instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080p_5000,			NTV2_FORMAT_1080p_5000_B);	///< @deprecated	Use NTV2_FORMAT_1080p_5000_B instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080p_5994,			NTV2_FORMAT_1080p_5994_B);	///< @deprecated	Use NTV2_FORMAT_1080p_5994_B instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080p_6000,			NTV2_FORMAT_1080p_6000_B);	///< @deprecated	Use NTV2_FORMAT_1080p_6000_B instead.

	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080p_2K_6000,		NTV2_FORMAT_1080p_2K_6000_A);	///< @deprecated	Use NTV2_FORMAT_1080p_2K_6000_A instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080p_2K_5994,		NTV2_FORMAT_1080p_2K_5994_A);	///< @deprecated	Use NTV2_FORMAT_1080p_2K_5994_A instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080p_2K_5000,		NTV2_FORMAT_1080p_2K_5000_A);	///< @deprecated	Use NTV2_FORMAT_1080p_2K_5000_A instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080p_2K_4795,		NTV2_FORMAT_1080p_2K_4795_A);	///< @deprecated	Use NTV2_FORMAT_1080p_2K_4795_A instead.
	NTV2_DEPRECATED_vi(const NTV2VideoFormat NTV2_FORMAT_1080p_2K_4800,		NTV2_FORMAT_1080p_2K_4800_A);	///< @deprecated	Use NTV2_FORMAT_1080p_2K_4800_A instead.
#endif	//	NTV2_DEPRECATE_14_2

#define NTV2_IS_VALID_VIDEO_FORMAT(__f__)							\
	(	NTV2_IS_HD_VIDEO_FORMAT (__f__)		||						\
		NTV2_IS_SD_VIDEO_FORMAT(__f__)		||						\
		NTV2_IS_2K_VIDEO_FORMAT(__f__)		||						\
		NTV2_IS_2K_1080_VIDEO_FORMAT(__f__)	||						\
		NTV2_IS_4K_VIDEO_FORMAT(__f__)		||						\
		NTV2_IS_QUAD_QUAD_FORMAT(__f__)	)

#define NTV2_IS_PAL_VIDEO_FORMAT(__f__)								\
	(	(__f__) == NTV2_FORMAT_1080i_5000	||						\
		(__f__) == NTV2_FORMAT_625_5000)

#define NTV2_IS_HD_VIDEO_FORMAT(__f__)								\
	(	(__f__) != NTV2_FORMAT_UNKNOWN	&&							\
		(((__f__) >= NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT	&&			\
		(__f__) < NTV2_FORMAT_END_HIGH_DEF_FORMATS)	||				\
		((__f__) >= NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT2	&&			\
		(__f__) < NTV2_FORMAT_END_HIGH_DEF_FORMATS2 )) )

#define NTV2_IS_SD_VIDEO_FORMAT(__f__)								\
	(	(__f__) != NTV2_FORMAT_UNKNOWN	&&							\
		(__f__) >= NTV2_FORMAT_FIRST_STANDARD_DEF_FORMAT	&&		\
		(__f__) < NTV2_FORMAT_END_STANDARD_DEF_FORMATS )

#define NTV2_IS_720P_VIDEO_FORMAT(__f__)							\
	(	(__f__) == NTV2_FORMAT_720p_2398	||						\
		(__f__) == NTV2_FORMAT_720p_2500	||						\
		(__f__) == NTV2_FORMAT_720p_5000	||						\
		(__f__) == NTV2_FORMAT_720p_5994	||						\
		(__f__) == NTV2_FORMAT_720p_6000	)

#define NTV2_IS_2K_VIDEO_FORMAT(__f__)								\
	(	(__f__) == NTV2_FORMAT_2K_1498	||							\
		(__f__) == NTV2_FORMAT_2K_1500	||							\
		(__f__) == NTV2_FORMAT_2K_2398	||							\
		(__f__) == NTV2_FORMAT_2K_2400	||							\
		(__f__) == NTV2_FORMAT_2K_2500	)

#define NTV2_IS_2K_1080_VIDEO_FORMAT(__f__)							\
	(	(__f__) == NTV2_FORMAT_1080p_2K_2398	||					\
		(__f__) == NTV2_FORMAT_1080psf_2K_2398	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_2400	||					\
		(__f__) == NTV2_FORMAT_1080psf_2K_2400	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_2500	||					\
		(__f__) == NTV2_FORMAT_1080psf_2K_2500	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_2997	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_3000	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4795_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4800_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4800_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4795_B	)

#define NTV2_IS_4K_VIDEO_FORMAT(__f__)								\
	(	((__f__) >= NTV2_FORMAT_FIRST_4K_DEF_FORMAT &&				\
		(__f__) < NTV2_FORMAT_END_4K_DEF_FORMATS	)   ||			\
		((__f__) >= NTV2_FORMAT_FIRST_UHD_TSI_DEF_FORMAT &&			\
		(__f__) < NTV2_FORMAT_END_4K_DEF_FORMATS2)					\
	)

#define NTV2_IS_4K_HFR_VIDEO_FORMAT(__f__)							\
	(	((__f__) >= NTV2_FORMAT_4x1920x1080p_5000	&&				\
		(__f__) <= NTV2_FORMAT_4x2048x1080p_12000	)   ||			\
		((__f__) >= NTV2_FORMAT_3840x2160p_5000		&&				\
		(__f__) <= NTV2_FORMAT_3840x2160p_6000		)   ||			\
		((__f__) >= NTV2_FORMAT_4096x2160p_5000		&&				\
		(__f__) <= NTV2_FORMAT_4096x2160p_12000)					\
	)

#define NTV2_IS_QUAD_HFR_VIDEO_FORMAT(__f__)						\
	(	((__f__) >= NTV2_FORMAT_4x1920x1080p_5000	&&				\
		(__f__) <= NTV2_FORMAT_4x2048x1080p_12000	)   ||			\
		((__f__) >= NTV2_FORMAT_3840x2160p_5000		&&				\
		(__f__) <= NTV2_FORMAT_3840x2160p_6000		)   ||			\
		((__f__) >= NTV2_FORMAT_4096x2160p_5000		&&				\
		(__f__) <= NTV2_FORMAT_4096x2160p_12000)					\
	)

#define NTV2_IS_QUAD_FRAME_FORMAT(__f__)							\
	(	((__f__) >= NTV2_FORMAT_FIRST_4K_DEF_FORMAT &&				\
		(__f__) < NTV2_FORMAT_END_4K_DEF_FORMATS	)   ||			\
		((__f__) >= NTV2_FORMAT_FIRST_UHD_TSI_DEF_FORMAT	&&		\
		(__f__) < NTV2_FORMAT_END_4K_DEF_FORMATS2	)				\
	)
	
#define NTV2_IS_QUAD_QUAD_FORMAT(__f__)								\
	(	((__f__) >= NTV2_FORMAT_FIRST_UHD2_DEF_FORMAT &&			\
		(__f__) < NTV2_FORMAT_END_UHD2_DEF_FORMATS	)   ||			\
		((__f__) >= NTV2_FORMAT_FIRST_UHD2_FULL_DEF_FORMAT	&&	\
		(__f__) < NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS	)		\
	)

#define NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(__f__)					\
	(	((__f__) >= NTV2_FORMAT_4x3840x2160p_5000	&&				\
		(__f__) <= NTV2_FORMAT_4x3840x2160p_6000_B	)   ||			\
		((__f__) >= NTV2_FORMAT_4x4096x2160p_4795	&&				\
		(__f__) <= NTV2_FORMAT_4x4096x2160p_6000_B		)			\
	)

#define NTV2_IS_4K_4096_VIDEO_FORMAT(__f__)							\
	(	(__f__) == NTV2_FORMAT_4x2048x1080p_2398	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_2398	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_2400	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_2400	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_2500	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_2500	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_2997	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_2997	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_3000	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_3000	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4795	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4800	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5000	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5994	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_6000	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_11988	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_12000	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_6000_B	||				\
		((__f__) >= NTV2_FORMAT_FIRST_4K_TSI_DEF_FORMAT &&			\
		(__f__) < NTV2_FORMAT_END_4K_TSI_DEF_FORMATS	)			\
	)

#define NTV2_IS_4K_QUADHD_VIDEO_FORMAT(__f__)						\
	(	(__f__) == NTV2_FORMAT_4x1920x1080p_2398	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_2398	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_2400	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_2400	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_2500	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_2500	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_2997	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_2997	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_3000	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_3000	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5000	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5994	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_6000	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_6000_B	||				\
		((__f__) >= NTV2_FORMAT_FIRST_UHD_TSI_DEF_FORMAT	&&		\
		(__f__) < NTV2_FORMAT_FIRST_4K_TSI_DEF_FORMAT   )			\
	)
	
#define NTV2_IS_UHD2_VIDEO_FORMAT(__f__)							\
	(   ((__f__) >= NTV2_FORMAT_FIRST_UHD2_DEF_FORMAT &&			\
		(__f__) < NTV2_FORMAT_END_UHD2_DEF_FORMATS)					\
	)

#define NTV2_IS_UHD2_FULL_VIDEO_FORMAT(__f__)						\
	(   ((__f__) >= NTV2_FORMAT_FIRST_UHD2_FULL_DEF_FORMAT &&		\
		(__f__) < NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS)			\
	)

#define NTV2_IS_8K_VIDEO_FORMAT(__f__)								\
	(   ((__f__) >= NTV2_FORMAT_FIRST_UHD2_DEF_FORMAT &&			\
		(__f__) < NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS)			\
	)

#define NTV2_IS_372_DUALLINK_FORMAT(__f__)							\
	(	(__f__) == NTV2_FORMAT_1080p_5000_B	||						\
		(__f__) == NTV2_FORMAT_1080p_5994_B	||						\
		(__f__) == NTV2_FORMAT_1080p_6000_B	||						\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4800_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4795_B	)

#define NTV2_IS_525_FORMAT(__f__)									\
	(	(__f__) == NTV2_FORMAT_525_5994	||							\
		(__f__) == NTV2_FORMAT_525_2398	||							\
		(__f__) == NTV2_FORMAT_525_2400	||							\
		(__f__) == NTV2_FORMAT_525psf_2997	)

#define NTV2_IS_625_FORMAT(__f__)									\
	(	(__f__) == NTV2_FORMAT_625_5000	||							\
		(__f__) == NTV2_FORMAT_625psf_2500	)

#define NTV2_IS_INTERMEDIATE_FORMAT(__f__)							\
	(	(__f__) == NTV2_FORMAT_2K_2398 ||							\
		(__f__) == NTV2_FORMAT_2K_2400 ||							\
		(__f__) == NTV2_FORMAT_720p_2398 ||							\
		(__f__) == NTV2_FORMAT_525_2398	)

#define NTV2_IS_3G_FORMAT(__f__)									\
	(	(__f__) == NTV2_FORMAT_1080p_5000_A	||						\
		(__f__) == NTV2_FORMAT_1080p_5000_B	||						\
		(__f__) == NTV2_FORMAT_1080p_5994_A	||						\
		(__f__) == NTV2_FORMAT_1080p_5994_B	||						\
		(__f__) == NTV2_FORMAT_1080p_6000_A	||						\
		(__f__) == NTV2_FORMAT_1080p_6000_B	||						\
		(__f__) == NTV2_FORMAT_1080p_2K_4795_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4800_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_A	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4800_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4795_B	||					\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5000	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5994	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_6000	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4795	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4800	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5000	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5994	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_6000	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_11988	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_12000	||				\
		((__f__) >= NTV2_FORMAT_3840x2160p_5000 &&					\
		(__f__) <= NTV2_FORMAT_3840x2160p_6000)		||				\
		((__f__) >= NTV2_FORMAT_4096x2160p_4795 &&					\
		(__f__) <= NTV2_FORMAT_4096x2160p_12000)   ||				\
		((__f__) >= NTV2_FORMAT_4x3840x2160p_5000 &&				\
		(__f__) <= NTV2_FORMAT_4x3840x2160p_6000)  ||				\
		((__f__) >= NTV2_FORMAT_4x4096x2160p_4795 &&				\
		(__f__) <= NTV2_FORMAT_4x4096x2160p_6000)					\
	)

#define NTV2_IS_6G_FORMAT(__f__)									\
	(	((__f__) >= NTV2_FORMAT_3840x2160psf_2398 &&				\
		(__f__) <= NTV2_FORMAT_3840x2160psf_3000)	||				\
		((__f__) >= NTV2_FORMAT_4096x2160psf_2398 &&				\
		(__f__) <= NTV2_FORMAT_4096x2160psf_3000 )					\
	)

#define NTV2_IS_12G_FORMAT(__f__)									\
	(	((__f__) >= NTV2_FORMAT_3840x2160p_5000 &&					\
		(__f__) <= NTV2_FORMAT_3840x2160p_6000_B)	||				\
		((__f__) >= NTV2_FORMAT_4096x2160p_5000 &&					\
		(__f__) <= NTV2_FORMAT_4096x2160p_6000_B )					\
	)

#define NTV2_IS_3Gb_FORMAT(__f__)									\
	(	(__f__) == NTV2_FORMAT_1080p_5000_B ||						\
		(__f__) == NTV2_FORMAT_1080p_5994_B ||						\
		(__f__) == NTV2_FORMAT_1080p_6000_B	||						\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4800_B	||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4795_B	||					\
		(__f__) == NTV2_FORMAT_3840x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_3840x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_3840x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_6000_B					\
	)

#define NTV2_IS_WIRE_FORMAT(__f__)									\
	(	(__f__) != NTV2_FORMAT_525_2398 &&							\
		(__f__) != NTV2_FORMAT_525_2400 &&							\
		(__f__) != NTV2_FORMAT_720p_2398 &&							\
		(__f__) != NTV2_FORMAT_720p_2500	)

#define NTV2_IS_PSF_VIDEO_FORMAT(__f__)								\
	(	(__f__) == NTV2_FORMAT_1080psf_2398 ||						\
		(__f__) == NTV2_FORMAT_1080psf_2400 ||						\
		(__f__) == NTV2_FORMAT_1080psf_2K_2398 ||					\
		(__f__) == NTV2_FORMAT_1080psf_2K_2400 ||					\
		(__f__) == NTV2_FORMAT_1080psf_2K_2500 ||					\
		(__f__) == NTV2_FORMAT_1080psf_2500_2 ||					\
		(__f__) == NTV2_FORMAT_1080psf_2997_2 ||					\
		(__f__) == NTV2_FORMAT_1080psf_3000_2 ||					\
		(__f__) == NTV2_FORMAT_525psf_2997 ||						\
		(__f__) == NTV2_FORMAT_625psf_2500 ||						\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_2398 ||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_2400 ||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_2500 ||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_2997 ||				\
		(__f__) == NTV2_FORMAT_4x1920x1080psf_3000 ||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_2398 ||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_2400 ||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_2500 ||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_2997 ||				\
		(__f__) == NTV2_FORMAT_4x2048x1080psf_3000 ||				\
		(__f__) == NTV2_FORMAT_3840x2160psf_2398 ||					\
		(__f__) == NTV2_FORMAT_3840x2160psf_2400 ||					\
		(__f__) == NTV2_FORMAT_3840x2160psf_2500 ||					\
		(__f__) == NTV2_FORMAT_3840x2160psf_2997 ||					\
		(__f__) == NTV2_FORMAT_3840x2160psf_3000 ||					\
		(__f__) == NTV2_FORMAT_4096x2160psf_2398 ||					\
		(__f__) == NTV2_FORMAT_4096x2160psf_2400 ||					\
		(__f__) == NTV2_FORMAT_4096x2160psf_2500 ||					\
		(__f__) == NTV2_FORMAT_4096x2160psf_2997 ||					\
		(__f__) == NTV2_FORMAT_4096x2160psf_3000					\
	)

#define NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(__f__)			\
	(	(__f__) != NTV2_FORMAT_1080i_5000 &&						\
		(__f__) != NTV2_FORMAT_1080i_5994 &&						\
		(__f__) != NTV2_FORMAT_1080i_6000 &&						\
		(__f__) != NTV2_FORMAT_525_5994 &&							\
		(__f__) != NTV2_FORMAT_625_5000		)

#define NTV2_VIDEO_FORMAT_IS_A(__f__)								\
	(	(__f__) == NTV2_FORMAT_1080p_5000_A ||						\
		(__f__) == NTV2_FORMAT_1080p_5994_A ||						\
		(__f__) == NTV2_FORMAT_1080p_6000_A ||						\
		(__f__) == NTV2_FORMAT_1080p_2K_4795_A ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4800_A ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_A ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_A ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_A ||					\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5000 ||					\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5994 ||					\
		(__f__) == NTV2_FORMAT_4x1920x1080p_6000 ||					\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4795 ||					\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4800 ||					\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5000 ||					\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5994 ||					\
		(__f__) == NTV2_FORMAT_4x2048x1080p_6000 ||					\
		(__f__) == NTV2_FORMAT_3840x2160p_5000 ||					\
		(__f__) == NTV2_FORMAT_3840x2160p_5994 ||					\
		(__f__) == NTV2_FORMAT_3840x2160p_6000 ||					\
		(__f__) == NTV2_FORMAT_4096x2160p_4795 ||					\
		(__f__) == NTV2_FORMAT_4096x2160p_4800 ||					\
		(__f__) == NTV2_FORMAT_4096x2160p_5000 ||					\
		(__f__) == NTV2_FORMAT_4096x2160p_5994 ||					\
		(__f__) == NTV2_FORMAT_4096x2160p_6000 ||					\
		(__f__) == NTV2_FORMAT_4x3840x2160p_5000 ||					\
		(__f__) == NTV2_FORMAT_4x3840x2160p_5994 ||					\
		(__f__) == NTV2_FORMAT_4x3840x2160p_6000 ||					\
		(__f__) == NTV2_FORMAT_4x4096x2160p_4795 ||					\
		(__f__) == NTV2_FORMAT_4x4096x2160p_4800 ||					\
		(__f__) == NTV2_FORMAT_4x4096x2160p_5000 ||					\
		(__f__) == NTV2_FORMAT_4x4096x2160p_5994 ||					\
		(__f__) == NTV2_FORMAT_4x4096x2160p_6000					\
	)

#define NTV2_VIDEO_FORMAT_IS_B(__f__)								\
	(	(__f__) == NTV2_FORMAT_1080p_5000_B ||						\
		(__f__) == NTV2_FORMAT_1080p_5994_B ||						\
		(__f__) == NTV2_FORMAT_1080p_6000_B ||						\
		(__f__) == NTV2_FORMAT_1080p_2K_4795_B ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_4800_B ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_B ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_B ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_B ||					\
		(__f__) == NTV2_FORMAT_3840x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_3840x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_3840x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_6000_B					\
	)

#define NTV2_VIDEO_FORMAT_IS_J2K_SUPPORTED(__f__)					\
	(	(__f__) == NTV2_FORMAT_525_5994 ||							\
		(__f__) == NTV2_FORMAT_625_5000 ||							\
		(__f__) == NTV2_FORMAT_720p_2398 ||							\
		(__f__) == NTV2_FORMAT_720p_2500 ||							\
		(__f__) == NTV2_FORMAT_720p_5000 ||							\
		(__f__) == NTV2_FORMAT_720p_5994 ||							\
		(__f__) == NTV2_FORMAT_720p_6000 ||							\
		(__f__) == NTV2_FORMAT_1080i_5000 ||						\
		(__f__) == NTV2_FORMAT_1080i_5994 ||						\
		(__f__) == NTV2_FORMAT_1080i_6000 ||						\
		(__f__) == NTV2_FORMAT_1080p_2398 ||						\
		(__f__) == NTV2_FORMAT_1080p_2400 ||						\
		(__f__) == NTV2_FORMAT_1080p_2500 ||						\
		(__f__) == NTV2_FORMAT_1080p_2997 ||						\
		(__f__) == NTV2_FORMAT_1080p_3000 ||						\
		(__f__) == NTV2_FORMAT_1080p_5000_A ||						\
		(__f__) == NTV2_FORMAT_1080p_5994_A ||						\
		(__f__) == NTV2_FORMAT_1080p_6000_A ||						\
		(__f__) == NTV2_FORMAT_1080p_2K_2398 ||						\
		(__f__) == NTV2_FORMAT_1080p_2K_2400 ||						\
		(__f__) == NTV2_FORMAT_1080p_2K_2500 ||						\
		(__f__) == NTV2_FORMAT_1080p_2K_2997 ||						\
		(__f__) == NTV2_FORMAT_1080p_2K_3000 ||						\
		(__f__) == NTV2_FORMAT_1080p_2K_5000_A ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_5994_A ||					\
		(__f__) == NTV2_FORMAT_1080p_2K_6000_A	)

#define NTV2_IS_TSI_FORMAT(__f__)									\
	(	((__f__) >= NTV2_FORMAT_FIRST_UHD_TSI_DEF_FORMAT	&&		\
		(__f__) < NTV2_FORMAT_FIRST_4K_TSI_DEF_FORMAT   ) 	)


#define NTV2_IS_SQUARE_DIVISION_FORMAT(__f__)						\
	(	((__f__) >= NTV2_FORMAT_FIRST_4K_DEF_FORMAT		&&			\
		(__f__) < NTV2_FORMAT_END_4K_DEF_FORMATS   )	||			\
		((__f__) >= NTV2_FORMAT_FIRST_4K_DEF_FORMAT2	&&			\
		(__f__) < NTV2_FORMAT_END_4K_DEF_FORMATS2   )   ||			\
		((__f__) >= NTV2_FORMAT_FIRST_UHD2_DEF_FORMAT	&&			\
		(__f__) < NTV2_FORMAT_END_UHD2_DEF_FORMATS   )  ||			\
		((__f__) >= NTV2_FORMAT_FIRST_UHD2_FULL_DEF_FORMAT	&&		\
		(__f__) < NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS   )		)
	
#define NTV2_VIDEO_FORMAT_NEEDS_CONVERSION(__f__)					\
	(	(__f__) == NTV2_FORMAT_3840x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_3840x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_3840x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4096x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x1920x1080p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4x2048x1080p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x3840x2160p_6000_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_4795_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_4800_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_5000_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_5994_B	||				\
		(__f__) == NTV2_FORMAT_4x4096x2160p_6000_B					\
	)

/**
	@brief		Identifies the mode of a frame store, either Capture (Input) or Display (Output).
	@see		CNTV2Card::SetMode, CNTV2Card::GetMode, \ref vidop-fs
**/
typedef enum
{
	NTV2_MODE_DISPLAY,							///< @brief	Playout (output) mode
	NTV2_MODE_OUTPUT	= NTV2_MODE_DISPLAY,	///< @brief	Output (playout, display) mode
	NTV2_MODE_CAPTURE,							///< @brief	Capture (input) mode
	NTV2_MODE_INPUT		= NTV2_MODE_CAPTURE,	///< @brief	Input (capture) mode
	NTV2_MODE_INVALID							///< @brief	The invalid mode
} NTV2Mode;

#define	NTV2_IS_VALID_MODE(__mode__)			(((__mode__) >= NTV2_MODE_DISPLAY) && ((__mode__) < NTV2_MODE_INVALID))
#define	NTV2_IS_INPUT_MODE(__mode__)			((__mode__) == NTV2_MODE_INPUT)
#define	NTV2_IS_OUTPUT_MODE(__mode__)			((__mode__) == NTV2_MODE_OUTPUT)


#if !defined (NTV2_DEPRECATE)
	typedef enum
	{
		NTV2_FRAMBUFFERMODE_FRAME,
		NTV2_FRAMBUFFERMODE_FIELD
	} NTV2FrameBufferMode;		///< @deprecated	Obsolete.
#endif	//	!defined (NTV2_DEPRECATE)


/**
	@brief		Identifies a specific video input source.
	@details	Always call ::NTV2DeviceCanDoInputSource to determine if a device has one of these input sources.
				Call CNTV2Card::GetInputVideoFormat to determine what video signal is present on the input (if any).
				Call ::GetInputSourceOutputXpt to get an NTV2OutputCrosspointID for one of these inputs to pass to
				CNTV2Card::Connect. See \ref devicesignalinputsoutputs.
	@warning	Do not rely on the ordinal values of these constants between successive SDKs, since new devices
				can be introduced that require additional inputs.
**/
typedef enum
{
	 NTV2_INPUTSOURCE_ANALOG1	///< @brief	Identifies the first analog video input
	,NTV2_INPUTSOURCE_HDMI1		///< @brief	Identifies the 1st HDMI video input
	,NTV2_INPUTSOURCE_HDMI2		///< @brief	Identifies the 2nd HDMI video input
	,NTV2_INPUTSOURCE_HDMI3		///< @brief	Identifies the 3rd HDMI video input
	,NTV2_INPUTSOURCE_HDMI4		///< @brief	Identifies the 4th HDMI video input
	,NTV2_INPUTSOURCE_SDI1		///< @brief	Identifies the 1st SDI video input
	,NTV2_INPUTSOURCE_SDI2		///< @brief	Identifies the 2nd SDI video input
	,NTV2_INPUTSOURCE_SDI3		///< @brief	Identifies the 3rd SDI video input
	,NTV2_INPUTSOURCE_SDI4		///< @brief	Identifies the 4th SDI video input
	,NTV2_INPUTSOURCE_SDI5		///< @brief	Identifies the 5th SDI video input
	,NTV2_INPUTSOURCE_SDI6		///< @brief	Identifies the 6th SDI video input
	,NTV2_INPUTSOURCE_SDI7		///< @brief	Identifies the 7th SDI video input
	,NTV2_INPUTSOURCE_SDI8		///< @brief	Identifies the 8th SDI video input
	,NTV2_INPUTSOURCE_INVALID	///< @brief	The invalid video input
	,NTV2_NUM_INPUTSOURCES = NTV2_INPUTSOURCE_INVALID
} NTV2InputSource;

#if !defined (NTV2_DEPRECATE_14_2)
	NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_ANALOG,	NTV2_INPUTSOURCE_ANALOG1);	///< @deprecated	Use NTV2_INPUTSOURCE_ANALOG1 instead.
	NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_HDMI,		NTV2_INPUTSOURCE_HDMI1);	///< @deprecated	Use NTV2_INPUTSOURCE_HDMI1 instead.
#endif
#if !defined (NTV2_DEPRECATE)
	#if defined(NTV2_BUILDING_DRIVER)
		#define	NTV2_INPUTSOURCE_DUALLINK	NTV2_INPUTSOURCE_SDI1
		#define	NTV2_INPUTSOURCE_DUALLINK1	NTV2_INPUTSOURCE_SDI1
		#define	NTV2_INPUTSOURCE_DUALLINK2	NTV2_INPUTSOURCE_SDI2
		#define	NTV2_INPUTSOURCE_DUALLINK3	NTV2_INPUTSOURCE_SDI3
		#define	NTV2_INPUTSOURCE_DUALLINK4	NTV2_INPUTSOURCE_SDI4
		#define	NTV2_INPUTSOURCE_DUALLINK5	NTV2_INPUTSOURCE_SDI5
		#define	NTV2_INPUTSOURCE_DUALLINK6	NTV2_INPUTSOURCE_SDI6
		#define	NTV2_INPUTSOURCE_DUALLINK7	NTV2_INPUTSOURCE_SDI7
		#define	NTV2_INPUTSOURCE_DUALLINK8	NTV2_INPUTSOURCE_SDI8
		#define	NTV2_INPUTSOURCE_SDI1_DS2	NTV2_INPUTSOURCE_SDI1
		#define	NTV2_INPUTSOURCE_SDI2_DS2	NTV2_INPUTSOURCE_SDI2
		#define	NTV2_INPUTSOURCE_SDI3_DS2	NTV2_INPUTSOURCE_SDI3
		#define	NTV2_INPUTSOURCE_SDI4_DS2	NTV2_INPUTSOURCE_SDI4
		#define	NTV2_INPUTSOURCE_SDI5_DS2	NTV2_INPUTSOURCE_SDI5
		#define	NTV2_INPUTSOURCE_SDI6_DS2	NTV2_INPUTSOURCE_SDI6
		#define	NTV2_INPUTSOURCE_SDI7_DS2	NTV2_INPUTSOURCE_SDI7
		#define	NTV2_INPUTSOURCE_SDI8_DS2	NTV2_INPUTSOURCE_SDI8
	#else
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK,	NTV2_INPUTSOURCE_SDI1);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI1 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK1,	NTV2_INPUTSOURCE_SDI1);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI1 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK2,	NTV2_INPUTSOURCE_SDI2);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI2 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK3,	NTV2_INPUTSOURCE_SDI3);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI3 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK4,	NTV2_INPUTSOURCE_SDI4);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI4 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK5,	NTV2_INPUTSOURCE_SDI5);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI5 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK6,	NTV2_INPUTSOURCE_SDI6);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI6 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK7,	NTV2_INPUTSOURCE_SDI7);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI7 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_DUALLINK8,	NTV2_INPUTSOURCE_SDI8);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI8 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_SDI1_DS2,	NTV2_INPUTSOURCE_SDI1);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI1 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_SDI2_DS2,	NTV2_INPUTSOURCE_SDI2);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI2 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_SDI3_DS2,	NTV2_INPUTSOURCE_SDI3);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI3 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_SDI4_DS2,	NTV2_INPUTSOURCE_SDI4);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI4 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_SDI5_DS2,	NTV2_INPUTSOURCE_SDI5);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI5 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_SDI6_DS2,	NTV2_INPUTSOURCE_SDI6);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI6 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_SDI7_DS2,	NTV2_INPUTSOURCE_SDI7);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI7 instead.
		NTV2_DEPRECATED_vi(const NTV2InputSource NTV2_INPUTSOURCE_SDI8_DS2,	NTV2_INPUTSOURCE_SDI8);	///< @deprecated	Use NTV2_INPUTSOURCE_SDI8 instead.
	#endif
#endif

#define	NTV2_INPUT_SOURCE_IS_HDMI(_inpSrc_)				((_inpSrc_) >= NTV2_INPUTSOURCE_HDMI1 && (_inpSrc_) <= NTV2_INPUTSOURCE_HDMI4)
#define	NTV2_INPUT_SOURCE_IS_ANALOG(_inpSrc_)			((_inpSrc_) == NTV2_INPUTSOURCE_ANALOG1)
#define	NTV2_INPUT_SOURCE_IS_SDI(_inpSrc_)				((_inpSrc_) >= NTV2_INPUTSOURCE_SDI1 && (_inpSrc_) <= NTV2_INPUTSOURCE_SDI8)
#define	NTV2_IS_VALID_INPUT_SOURCE(_inpSrc_)			(((_inpSrc_) >= 0) && ((_inpSrc_) < NTV2_INPUTSOURCE_INVALID))

/**
	@brief		Used to specify one or more ::NTV2InputSource types.
**/
typedef enum _NTV2InputSourceKinds
{
	NTV2_INPUTSOURCES_ALL		= 0xFF,	///< @brief	Specifies any/all input source kinds.
	NTV2_INPUTSOURCES_SDI		= 1,	///< @brief	Specifies SDI input source kinds.
	NTV2_INPUTSOURCES_HDMI		= 2,	///< @brief	Specifies HDMI input source kinds.
	NTV2_INPUTSOURCES_ANALOG	= 4,	///< @brief	Specifies analog input source kinds.
	NTV2_INPUTSOURCES_NONE		= 0		///< @brief	Doesn't specify any kind of input source.
} NTV2InputSourceKinds;


typedef enum
{
	#if defined (NTV2_DEPRECATE)
		NTV2_OUTPUTDESTINATION_ANALOG,
		NTV2_OUTPUTDESTINATION_HDMI,
		NTV2_OUTPUTDESTINATION_SDI1,
		NTV2_OUTPUTDESTINATION_SDI2,
		NTV2_OUTPUTDESTINATION_SDI3,
		NTV2_OUTPUTDESTINATION_SDI4,
		NTV2_OUTPUTDESTINATION_SDI5,
		NTV2_OUTPUTDESTINATION_SDI6,
		NTV2_OUTPUTDESTINATION_SDI7,
		NTV2_OUTPUTDESTINATION_SDI8,
	#else
		NTV2_OUTPUTDESTINATION_SDI1,
		NTV2_OUTPUTDESTINATION_ANALOG,
		NTV2_OUTPUTDESTINATION_SDI2,
		NTV2_OUTPUTDESTINATION_HDMI,
		NTV2_OUTPUTDESTINATION_DUALLINK,
		NTV2_OUTPUTDESTINATION_DUALLINK1	= NTV2_OUTPUTDESTINATION_DUALLINK,
		NTV2_OUTPUTDESTINATION_HDMI_14,
		NTV2_OUTPUTDESTINATION_DUALLINK2,
		NTV2_OUTPUTDESTINATION_SDI3,
		NTV2_OUTPUTDESTINATION_SDI4,
		NTV2_OUTPUTDESTINATION_SDI5,
		NTV2_OUTPUTDESTINATION_SDI6,
		NTV2_OUTPUTDESTINATION_SDI7,
		NTV2_OUTPUTDESTINATION_SDI8,
		NTV2_OUTPUTDESTINATION_DUALLINK3,
		NTV2_OUTPUTDESTINATION_DUALLINK4,
		NTV2_OUTPUTDESTINATION_DUALLINK5,
		NTV2_OUTPUTDESTINATION_DUALLINK6,
		NTV2_OUTPUTDESTINATION_DUALLINK7,
		NTV2_OUTPUTDESTINATION_DUALLINK8,
	#endif	//	!defined (NTV2_DEPRECATE)
	NTV2_OUTPUTDESTINATION_INVALID,
	NTV2_NUM_OUTPUTDESTINATIONS = NTV2_OUTPUTDESTINATION_INVALID	//	Always last!
} NTV2OutputDestination;

#if !defined (NTV2_DEPRECATE)
	#define	NTV2_OUTPUT_DEST_IS_HDMI(_dest_)			((_dest_) == NTV2_OUTPUTDESTINATION_HDMI || (_dest_) == NTV2_OUTPUTDESTINATION_HDMI_14)
	#define	NTV2_OUTPUT_DEST_IS_ANALOG(_dest_)			((_dest_) == NTV2_OUTPUTDESTINATION_ANALOG)
	#define	NTV2_OUTPUT_DEST_IS_SDI(_dest_)				(!NTV2_OUTPUT_DEST_IS_HDMI(_dest_) && !NTV2_OUTPUT_DEST_IS_ANALOG(_dest_) && (_dest_) < NTV2_NUM_OUTPUTDESTINATIONS)
	#define	NTV2_OUTPUT_DEST_IS_DUAL_LINK(_dest_)		(	(_dest_) == NTV2_OUTPUTDESTINATION_DUALLINK1	||	\
															(_dest_) == NTV2_OUTPUTDESTINATION_DUALLINK2	||	\
															(_dest_) == NTV2_OUTPUTDESTINATION_DUALLINK3	||	\
															(_dest_) == NTV2_OUTPUTDESTINATION_DUALLINK4	||	\
															(_dest_) == NTV2_OUTPUTDESTINATION_DUALLINK5	||	\
															(_dest_) == NTV2_OUTPUTDESTINATION_DUALLINK6	||	\
															(_dest_) == NTV2_OUTPUTDESTINATION_DUALLINK7	||	\
															(_dest_) == NTV2_OUTPUTDESTINATION_DUALLINK8	)
#else
	#define	NTV2_OUTPUT_DEST_IS_HDMI(_dest_)			((_dest_) == NTV2_OUTPUTDESTINATION_HDMI)
	#define	NTV2_OUTPUT_DEST_IS_ANALOG(_dest_)			((_dest_) == NTV2_OUTPUTDESTINATION_ANALOG)
	#define	NTV2_OUTPUT_DEST_IS_SDI(_dest_)				((_dest_) >= NTV2_OUTPUTDESTINATION_SDI1 && (_dest_) <= NTV2_OUTPUTDESTINATION_SDI8)
#endif	//	!defined (NTV2_DEPRECATE)
#define	NTV2_IS_VALID_OUTPUT_DEST(_dest_)				(((_dest_) >= 0) && ((_dest_) < NTV2_NUM_OUTPUTDESTINATIONS))


/**
	@brief		These enum values are mostly used to identify a specific \ref widget_framestore widget.
				They're also typically used to identify a particular channel (or video input/output stream).
				They can also be used anywhere a zero-based index value is expected.
	@note		In NTV2 parlance, the terms <b>Channel</b> and <b>Frame Store</b> are used interchangeably.
	@see		::NTV2DeviceGetNumFrameStores, \ref vidop-fs
**/
typedef enum
{
	NTV2_CHANNEL1,		///< @brief	Specifies channel or Frame Store 1 (or the first item).
	NTV2_CHANNEL2,		///< @brief	Specifies channel or Frame Store 2 (or the 2nd item).
	NTV2_CHANNEL3,		///< @brief	Specifies channel or Frame Store 3 (or the 3rd item).
	NTV2_CHANNEL4,		///< @brief	Specifies channel or Frame Store 4 (or the 4th item).
	NTV2_CHANNEL5,		///< @brief	Specifies channel or Frame Store 5 (or the 5th item).
	NTV2_CHANNEL6,		///< @brief	Specifies channel or Frame Store 6 (or the 6th item).
	NTV2_CHANNEL7,		///< @brief	Specifies channel or Frame Store 7 (or the 7th item).
	NTV2_CHANNEL8,		///< @brief	Specifies channel or Frame Store 8 (or the 8th item).
	NTV2_MAX_NUM_CHANNELS,			//	Always last!
	NTV2_CHANNEL_INVALID = NTV2_MAX_NUM_CHANNELS
} NTV2Channel;

#define NTV2_IS_VALID_CHANNEL(__x__)					((__x__) >= NTV2_CHANNEL1 && (__x__) < NTV2_MAX_NUM_CHANNELS)


/**
	@brief		These enum values are used for device selection/filtering.
	@see		::NTV2GetSupportedDevices
**/
typedef enum _NTV2DeviceKinds
{
	NTV2_DEVICEKIND_ALL			= 0xFFFF,	///< @brief	Specifies any/all devices.
	NTV2_DEVICEKIND_INPUT		= 0x0001,	///< @brief	Specifies devices that input (capture).
	NTV2_DEVICEKIND_OUTPUT		= 0x0002,	///< @brief	Specifies devices that output (playout).
	NTV2_DEVICEKIND_SDI			= 0x0004,	///< @brief	Specifies devices with SDI connectors.
	NTV2_DEVICEKIND_HDMI		= 0x0008,	///< @brief	Specifies devices with HDMI connectors.
	NTV2_DEVICEKIND_ANALOG		= 0x0010,	///< @brief	Specifies devices with analog video connectors.
	NTV2_DEVICEKIND_SFP			= 0x0020,	///< @brief	Specifies devices with SFP connectors.
	NTV2_DEVICEKIND_IP			= NTV2_DEVICEKIND_SFP,
	NTV2_DEVICEKIND_EXTERNAL	= 0x0040,	///< @brief	Specifies external devices (e.g. Thunderbolt).
	NTV2_DEVICEKIND_4K			= 0x0080,	///< @brief	Specifies devices that can do 4K video.
	NTV2_DEVICEKIND_8K			= 0x0100,	///< @brief	Specifies devices that can do 8K video.
//	NTV2_DEVICEKIND_HFR			= 0x0200,	///< @brief	Specifies devices that can handle HFR video.
	NTV2_DEVICEKIND_6G			= 0x0400,	///< @brief	Specifies devices that have 6G SDI connectors.
	NTV2_DEVICEKIND_12G			= 0x0800,	///< @brief	Specifies devices that have 12G SDI connectors.
	NTV2_DEVICEKIND_CUSTOM_ANC	= 0x1000,	///< @brief	Specifies devices that have custom Anc inserter/extractor firmware.
	NTV2_DEVICEKIND_RELAYS		= 0x2000,	///< @brief	Specifies devices that have bypass relays.
	NTV2_DEVICEKIND_NONE		= 0			///< @brief	Doesn't specify any kind of device.
} NTV2DeviceKinds;


/**
	@brief		Identifies a specific IP-based data stream.
	@warning	The ordinal values of the enum names may change in successive SDKs.
**/
typedef enum
{
	NTV2_VIDEO1_STREAM		= 0,
	NTV2_VIDEO2_STREAM		= 1,
	NTV2_VIDEO3_STREAM		= 2,
	NTV2_VIDEO4_STREAM		= 3,
	NTV2_AUDIO1_STREAM		= 4,
	NTV2_AUDIO2_STREAM		= 5,
	NTV2_AUDIO3_STREAM		= 6,
	NTV2_AUDIO4_STREAM		= 7,
	NTV2_ANC1_STREAM		= 8,
	NTV2_ANC2_STREAM		= 9,
	NTV2_ANC3_STREAM		= 10,
	NTV2_ANC4_STREAM		= 11,
	NTV2_VIDEO4K_STREAM		= 12,
	NTV2_MAX_NUM_SINGLE_STREAMS = NTV2_VIDEO4K_STREAM,
	NTV2_MAX_NUM_STREAMS	= 13,
	NTV2_STREAM_INVALID = NTV2_MAX_NUM_STREAMS
} NTV2Stream;

#define NTV2_STREAM_MASK_ALL ((1 << NTV2_MAX_NUM_STREAMS) - 1)


/**
	@brief		Identifies the kind of data that can be carried by an IP-based data stream.
**/
typedef enum
{
	VIDEO_STREAM,		///< @brief	Video data
	AUDIO_STREAM,		///< @brief	Audio data
	ANC_STREAM,			///< @brief	Anc data
	VIDEO_4K_STREAM,	///< @brief	4K Video stream using 4 streams
	INVALID_STREAM
} NTV2StreamType;


#define NTV2_IS_VALID_RX_STREAM(__x__)					((__x__) >= NTV2_VIDEO1_STREAM && (__x__) < NTV2_MAX_NUM_STREAMS)
#define NTV2_IS_VALID_TX_STREAM(__x__)					((__x__) >= NTV2_VIDEO1_STREAM && (__x__) < NTV2_MAX_NUM_STREAMS)
#define NTV2_IS_VALID_RX_SINGLE_STREAM(__x__)			((__x__) >= NTV2_VIDEO1_STREAM && (__x__) < NTV2_MAX_NUM_SINGLE_STREAMS)
#define NTV2_IS_VALID_TX_SINGLE_STREAM(__x__)			((__x__) >= NTV2_VIDEO1_STREAM && (__x__) < NTV2_MAX_NUM_SINGLE_STREAMS)

/**
	@brief		These enum values identify a specific source for the device's (output) reference clock.
	@see		CNTV2Card::GetReference, CNTV2Card::SetReference, \ref deviceclockingandsync
	@warning	Do not rely on the ordinal values of these constants between successive SDKs, since new devices
				can be introduced that require additional inputs.
**/
typedef enum
{
	NTV2_REFERENCE_EXTERNAL			= 0,	///< @brief	Specifies the External Reference connector.
	NTV2_REFERENCE_INPUT1			= 1,	///< @brief	Specifies the SDI In 1 connector.
	NTV2_REFERENCE_INPUT2			= 2,	///< @brief	Specifies the SDI In 2 connector.
	NTV2_REFERENCE_FREERUN			= 3,	///< @brief	Specifies the device's internal clock.
	NTV2_REFERENCE_ANALOG_INPUT1	= 4,	///< @brief	Specifies the Analog In 1 connector.
	NTV2_REFERENCE_HDMI_INPUT1		= 5,	///< @brief	Specifies the HDMI In 1 connector.
	NTV2_REFERENCE_INPUT3			= 6,	///< @brief	Specifies the SDI In 3 connector.
	NTV2_REFERENCE_INPUT4			= 7,	///< @brief	Specifies the SDI In 4 connector.
	NTV2_REFERENCE_INPUT5			= 8,	///< @brief	Specifies the SDI In 5 connector.
	NTV2_REFERENCE_INPUT6			= 9,	///< @brief	Specifies the SDI In 6 connector.
	NTV2_REFERENCE_INPUT7			= 10,	///< @brief	Specifies the SDI In 7 connector.
	NTV2_REFERENCE_INPUT8			= 11,	///< @brief	Specifies the SDI In 8 connector.
	NTV2_REFERENCE_SFP1_PTP			= 12,	///< @brief	Specifies the PTP source on SFP 1.
	NTV2_REFERENCE_SFP1_PCR			= 13,	///< @brief	Specifies the PCR source on SFP 1.
	NTV2_REFERENCE_SFP2_PTP			= 14,	///< @brief	Specifies the PTP source on SFP 2.
	NTV2_REFERENCE_SFP2_PCR			= 15,	///< @brief	Specifies the PCR source on SFP 2.
	NTV2_REFERENCE_HDMI_INPUT2		= 16,	///< @brief	Specifies the HDMI In 2 connector.
	NTV2_REFERENCE_HDMI_INPUT3		= 17,	///< @brief	Specifies the HDMI In 3 connector.
	NTV2_REFERENCE_HDMI_INPUT4		= 18,	///< @brief	Specifies the HDMI In 4 connector.
	NTV2_NUM_REFERENCE_INPUTS,			//	Always last!
	NTV2_REFERENCE_HDMI_INPUT		= NTV2_REFERENCE_HDMI_INPUT1,	///< @deprecated	Use NTV2_REFERENCE_HDMI_INPUT1 instead.
	NTV2_REFERENCE_ANALOG_INPUT		= NTV2_REFERENCE_ANALOG_INPUT1,	///< @deprecated	Use NTV2_REFERENCE_ANALOG_INPUT1 instead.
	NTV2_REFERENCE_INVALID = NTV2_NUM_REFERENCE_INPUTS
} NTV2ReferenceSource;

#define NTV2_IS_VALID_NTV2ReferenceSource(__x__)		((__x__) >= NTV2_REFERENCE_EXTERNAL && (__x__) < NTV2_NUM_REFERENCE_INPUTS)

typedef enum
{
	NTV2_REFVOLTAGE_1 ,
	NTV2_REFVOLTAGE_4
} NTV2RefVoltage;


typedef enum				// used in FS1
{
	NTV2FS1_RefSelect_None = 0,
	NTV2FS1_RefSelect_RefBNC = 1,
	NTV2FS1_RefSelect_ComponentY = 2,
	NTV2FS1_RefSelect_SVideo = 4,
	NTV2FS1_RefSelect_Composite = 8
} NTV2FS1ReferenceSelect;

typedef enum				// used in FS1
{
	NTV2FS1_FreezeOutput_Disable = 0,
	NTV2FS1_FreezeOutput_Enable  = 1
} NTV2FS1FreezeOutput;

typedef enum				// used in FS1
{
	NTV2FS1_OUTPUTTONE_DISABLE = 0,
	NTV2FS1_OUTPUTTONE_ENABLE = 1
} NTV2FS1OutputTone;

typedef enum				// used in FS1
{
	NTV2FS1_AUDIOTONE_400Hz = 0,
	NTV2FS1_AUDIOTONE_1000Hz = 1
} NTV2FS1AudioTone;

typedef enum				// used in FS1
{
	NTV2FS1_AUDIOLEVEL_24dBu = 0,
	NTV2FS1_AUDIOLEVEL_18dBu = 1,
	NTV2FS1_AUDIOLEVEL_12dBu = 2,
	NTV2FS1_AUDIOLEVEL_15dBu = 3
} NTV2FS1AudioLevel;

typedef enum
{
	NTV2_DISABLEINTERRUPTS			= 0x0,
	NTV2_OUTPUTVERTICAL				= 0x1,
	NTV2_INPUT1VERTICAL				= 0x2,
	NTV2_INPUT2VERTICAL				= 0x4,
	NTV2_AUDIOINTERRUPT				= 0x8,
	NTV2_AUDIOOUTWRAPINTERRUPT		= 0x10,
	NTV2_AUDIOINWRAPINTERRUPT		= 0x20,
	NTV2_AUDIOWRAPRATEINTERRUPT		= 0x40,
	NTV2_UART_TX_INTERRUPT			= 0x80,
	NTV2_UART_RX_INTERRUPT			= 0x100,
	NTV2_FS1_I2C_INTERRUPT1			= 0x200,
	NTV2_FS1_I2C_INTERRUPT2			= 0x400,
	NTV2_AUX_VERTICAL_INTERRUPT		= 0x800,
	NTV2_AUX_VERTICAL_INTERRUPT_CLEAR= 0x1000,
	NTV2_FS1_I2C_INTERRUPT2_CLEAR	= 0x2000,
	NTV2_FS1_I2C_INTERRUPT1_CLEAR	= 0x4000,
	NTV2_UART_RX_INTERRUPT_CLEAR	= 0x8000,
	NTV2_AUDIOCHUNKRATE_CLEAR		= 0x10000,
	NTV2_UART_TX_INTERRUPT2			= 0x20000,
	NTV2_OUTPUT2VERTICAL			= 0x40000,
	NTV2_OUTPUT3VERTICAL			= 0x80000,
	NTV2_OUTPUT4VERTICAL			= 0x100000,
	NTV2_OUTPUT4VERTICAL_CLEAR		= 0x200000,
	NTV2_OUTPUT3VERTICAL_CLEAR		= 0x400000,
	NTV2_OUTPUT2VERTICAL_CLEAR		= 0x800000,
	NTV2_UART_TX_INTERRUPT_CLEAR	= 0x1000000,
	NTV2_WRAPRATEINTERRUPT_CLEAR	= 0x2000000,
	NTV2_UART_TX_INTERRUPT2_CLEAR	= 0x4000000,
	NTV2_AUDIOOUTWRAPINTERRUPT_CLEAR= 0x8000000,
	NTV2_AUDIOINTERRUPT_CLEAR		= 0x10000000,
	NTV2_INPUT2VERTICAL_CLEAR		= 0x20000000,
	NTV2_INPUT1VERTICAL_CLEAR		= 0x40000000,
	NTV2_OUTPUTVERTICAL_CLEAR		= 0x80000000
} NTV2InterruptMask;

typedef enum
{
	NTV2_OUTPUTVERTICAL_SHIFT			= 0x0,
	NTV2_INPUT1VERTICAL_SHIFT			= 0x1,
	NTV2_INPUT2VERTICAL_SHIFT			= 0x2,
	NTV2_AUDIOINTERRUPT_SHIFT			= 0x3,
	NTV2_AUDIOOUTWRAPINTERRUPT_SHIFT	= 0x4,
	NTV2_AUDIOINWRAPINTERRUPT_SHIFT		= 0x5,
	NTV2_AUDIOWRAPRATEINTERRUPT_SHIFT	= 0x6,
	NTV2_UART_TX_INTERRUPT_SHIFT		= 0x7,
	NTV2_UART_RX_INTERRUPT_SHIFT		= 0x8,
	NTV2_FS1_I2C_INTERRUPT1_SHIFT		= 0x9,
	NTV2_FS1_I2C_INTERRUPT2_SHIFT		= 0xA,
	NTV2_AUX_VERTICAL_INTERRUPT_SHIFT	= 0xB,
	NTV2_AUX_VERTICAL_INTERRUPT_CLEAR_SHIFT	= 0xC,
	NTV2_FS1_I2C_INTERRUPT2_CLEAR_SHIFT		= 0xD,
	NTV2_FS1_I2C_INTERRUPT1_CLEAR_SHIFT		= 0xE,
	NTV2_UART_RX_INTERRUPT_CLEAR_SHIFT		= 0xF,
	NTV2_AUDIOCHUNKRATE_CLEAR_SHIFT			= 0x10,
	NTV2_UART_TX_INTERRUPT2_SHIFT			= 0x11,
	NTV2_OUTPUT2VERTICAL_SHIFT				= 0x12,
	NTV2_OUTPUT3VERTICAL_SHIFT				= 0x13,
	NTV2_OUTPUT4VERTICAL_SHIFT				= 0x14,
	NTV2_OUTPUT4VERTICAL_CLEAR_SHIFT		= 0x15,
	NTV2_OUTPUT3VERTICAL_CLEAR_SHIFT		= 0x16,
	NTV2_OUTPUT2VERTICAL_CLEAR_SHIFT		= 0x17,
	NTV2_UART_TX_INTERRUPT_CLEAR_SHIFT		= 0x18,
	NTV2_WRAPRATEINTERRUPT_CLEAR_SHIFT		= 0x19,
	NTV2_UART_TX_INTERRUPT2_CLEAR_SHIFT		= 0x1A,
	NTV2_AUDIOOUTWRAPINTERRUPT_CLEAR_SHIFT	= 0x1B,
	NTV2_AUDIOINTERRUPT_CLEAR_SHIFT			= 0x1C,
	NTV2_INPUT2VERTICAL_CLEAR_SHIFT			= 0x1D,
	NTV2_INPUT1VERTICAL_CLEAR_SHIFT			= 0x1E,
	NTV2_OUTPUTVERTICAL_CLEAR_SHIFT			= 0x1F
} NTV2InterruptShift;

typedef enum
{
	NTV2_INPUT3VERTICAL				= 0x2,
	NTV2_INPUT4VERTICAL				= 0x4,
	NTV2_HDMIRXV2HOTPLUGDETECT		= 0x10,
	NTV2_HDMIRXV2HOTPLUGDETECT_CLEAR= 0x20,
	NTV2_HDMIRXV2AVICHANGE			= 0x40,
	NTV2_HDMIRXV2AVICHANGE_CLEAR	= 0x80,
	NTV2_INPUT5VERTICAL				= 0x100,
	NTV2_INPUT6VERTICAL				= 0x200,
	NTV2_INPUT7VERTICAL				= 0x400,
	NTV2_INPUT8VERTICAL				= 0x800,
	NTV2_OUTPUT5VERTICAL			= 0x1000,
	NTV2_OUTPUT6VERTICAL			= 0x2000,
	NTV2_OUTPUT7VERTICAL			= 0x4000,
	NTV2_OUTPUT8VERTICAL			= 0x8000,
	NTV2_OUTPUT8VERTICAL_CLEAR		= 0x10000,
	NTV2_OUTPUT7VERTICAL_CLEAR		= 0x20000,
	NTV2_OUTPUT6VERTICAL_CLEAR		= 0x40000,
	NTV2_OUTPUT5VERTICAL_CLEAR		= 0x80000,
	NTV2_INPUT8VERTICAL_CLEAR		= 0x2000000,
	NTV2_INPUT7VERTICAL_CLEAR		= 0x4000000,
	NTV2_INPUT6VERTICAL_CLEAR		= 0x8000000,
	NTV2_INPUT5VERTICAL_CLEAR		= 0x10000000,
	NTV2_INPUT4VERTICAL_CLEAR		= 0x20000000,
	NTV2_INPUT3VERTICAL_CLEAR		= 0x40000000
} NTV2Interrupt2Mask;

typedef enum
{
	NTV2_INPUT3VERTICAL_SHIFT			= 0x1,
	NTV2_INPUT4VERTICAL_SHIFT			= 0x2,
	NTV2_HDMIRXV2HOTPLUGDETECT_SHIFT	= 0x4,
	NTV2_HDMIRXV2HOTPLUGDETECT_CLEAR_SHIFT= 0x5,
	NTV2_HDMIRXV2AVICHANGE_SHIFT		= 0x6,
	NTV2_HDMIRXV2AVICHANGE_CLEAR_SHIFT	= 0x7,
	NTV2_INPUT5VERTICAL_SHIFT			= 0x8,
	NTV2_INPUT6VERTICAL_SHIFT			= 0x9,
	NTV2_INPUT7VERTICAL_SHIFT			= 0xA,
	NTV2_INPUT8VERTICAL_SHIFT			= 0xB,
	NTV2_OUTPUT5VERTICAL_SHIFT			= 0xC,
	NTV2_OUTPUT6VERTICAL_SHIFT			= 0xD,
	NTV2_OUTPUT7VERTICAL_SHIFT			= 0xE,
	NTV2_OUTPUT8VERTICAL_SHIFT			= 0xF,
	NTV2_OUTPUT8VERTICAL_CLEAR_SHIFT	= 0x10,
	NTV2_OUTPUT7VERTICAL_CLEAR_SHIFT	= 0x11,
	NTV2_OUTPUT6VERTICAL_CLEAR_SHIFT	= 0x12,
	NTV2_OUTPUT5VERTICAL_CLEAR_SHIFT	= 0x13,
	NTV2_INPUT8VERTICAL_CLEAR_SHIFT		= 0x19,
	NTV2_INPUT7VERTICAL_CLEAR_SHIFT		= 0x1A,
	NTV2_INPUT6VERTICAL_CLEAR_SHIFT		= 0x1B,
	NTV2_INPUT5VERTICAL_CLEAR_SHIFT		= 0x1C,
	NTV2_INPUT4VERTICAL_CLEAR_SHIFT		= 0x1D,
	NTV2_INPUT3VERTICAL_CLEAR_SHIFT		= 0x1E
} NTV2Interrupt2Shift;

typedef enum
{
	NTV2_DISABLE_DMAINTERRUPTS	= 0,
	NTV2_DMA1_ENABLE	= 1,
	NTV2_DMA2_ENABLE	= 2,
	NTV2_DMA3_ENABLE	= 4,
	NTV2_DMA4_ENABLE	= 8,
	NTV2_DMA_BUS_ERROR	= 16
} NTV2DMAInterruptMask;

typedef enum
{
	NTV2_DMA1_CLEAR  = 0x08000000,
	NTV2_DMA2_CLEAR  = 0x10000000,
	NTV2_DMA3_CLEAR  = 0x20000000,
	NTV2_DMA4_CLEAR  = 0x40000000,
	NTV2_BUSERROR_CLEAR= 0x80000000
} NTV2DMAStatusBits;

/**
	@brief	These values are used to determine when certain register writes actually take effect.
			See CNTV2Card::SetRegisterWriteMode or \ref fieldframeinterrupts
**/
typedef enum
{
	NTV2_REGWRITE_SYNCTOFIELD,	///< @brief	<b>Field Mode:</b> Register changes take effect at the next field VBI.
	NTV2_REGWRITE_SYNCTOFRAME,	///< @brief	<b>Frame Mode:</b> Register changes take effect at the next frame VBI (power-up default).
	NTV2_REGWRITE_IMMEDIATE,	///< @brief	Register changes take effect immediately, without waiting for a field or frame VBI.
	NTV2_REGWRITE_SYNCTOFIELD_AFTER10LINES	///< @brief	Register changes take effect after 10 lines after the next field VBI (not commonly used).
} NTV2RegisterWriteMode;

typedef enum
{
	NTV2_SIGNALMASK_NONE	= 0,		///< Output Black.
	NTV2_SIGNALMASK_Y		= 1,		///< Output Y if set, else Output Y=0x40
	NTV2_SIGNALMASK_Cb		= 2,		///< Output Cb if set, elso Output Cb to 0x200
	NTV2_SIGNALMASK_Cr		= 4,		///< Output Cr if set, elso Output Cr to 0x200
	NTV2_SIGNALMASK_ALL		= 1+2+4		///< Output Cr if set, elso Output Cr to 0x200
} NTV2SignalMask;

/**
	@brief	Logically, these are an ::NTV2Channel combined with an ::NTV2Mode.
	@note	Do not use these, as they will be removed at some point in the future.
**/
typedef enum
{
		NTV2CROSSPOINT_CHANNEL1,
		NTV2CROSSPOINT_CHANNEL2,
		NTV2CROSSPOINT_INPUT1,
		NTV2CROSSPOINT_INPUT2,
		NTV2CROSSPOINT_MATTE,		///< @deprecated	This is obsolete
		NTV2CROSSPOINT_FGKEY,		///< @deprecated	This is obsolete
		NTV2CROSSPOINT_CHANNEL3,
		NTV2CROSSPOINT_CHANNEL4,
		NTV2CROSSPOINT_INPUT3,
		NTV2CROSSPOINT_INPUT4,
		NTV2CROSSPOINT_CHANNEL5,
		NTV2CROSSPOINT_CHANNEL6,
		NTV2CROSSPOINT_CHANNEL7,
		NTV2CROSSPOINT_CHANNEL8,
		NTV2CROSSPOINT_INPUT5,
		NTV2CROSSPOINT_INPUT6,
		NTV2CROSSPOINT_INPUT7,
		NTV2CROSSPOINT_INPUT8,
		NTV2_NUM_CROSSPOINTS,
		NTV2CROSSPOINT_INVALID = NTV2_NUM_CROSSPOINTS
} NTV2Crosspoint;

#define	NTV2_IS_INPUT_CROSSPOINT(__x__)			(	(__x__) == NTV2CROSSPOINT_INPUT1 ||							\
													(__x__) == NTV2CROSSPOINT_INPUT2 ||							\
													(__x__) == NTV2CROSSPOINT_INPUT3 ||							\
													(__x__) == NTV2CROSSPOINT_INPUT4 ||							\
													(__x__) == NTV2CROSSPOINT_INPUT5 ||							\
													(__x__) == NTV2CROSSPOINT_INPUT6 ||							\
													(__x__) == NTV2CROSSPOINT_INPUT7 ||							\
													(__x__) == NTV2CROSSPOINT_INPUT8	)

#define	NTV2_IS_OUTPUT_CROSSPOINT(__x__)		(	(__x__) == NTV2CROSSPOINT_CHANNEL1 ||						\
													(__x__) == NTV2CROSSPOINT_CHANNEL2 ||						\
													(__x__) == NTV2CROSSPOINT_CHANNEL3 ||						\
													(__x__) == NTV2CROSSPOINT_CHANNEL4 ||						\
													(__x__) == NTV2CROSSPOINT_CHANNEL5 ||						\
													(__x__) == NTV2CROSSPOINT_CHANNEL6 ||						\
													(__x__) == NTV2CROSSPOINT_CHANNEL7 ||						\
													(__x__) == NTV2CROSSPOINT_CHANNEL8	)

#define	NTV2_IS_VALID_NTV2CROSSPOINT(__x__)		(NTV2_IS_INPUT_CROSSPOINT (__x__) || NTV2_IS_OUTPUT_CROSSPOINT (__x__))


typedef enum
{
	NTV2VIDPROCMODE_MIX,
	NTV2VIDPROCMODE_SPLIT,
	NTV2VIDPROCMODE_KEY,
	NTV2VIDPROCMODE_INVALID
} NTV2Ch1VidProcMode;

typedef enum
{
	NTV2Ch2OUTPUTMODE_BGV,
	NTV2Ch2OUTPUTMODE_FGV,
	NTV2Ch2OUTPUTMODE_MIXEDKEY,
	NTV2Ch2OUTPUTMODE_INVALID
} NTV2Ch2OutputMode;


typedef enum
{
	NTV2SPLITMODE_HORZSPLIT,
	NTV2SPLITMODE_VERTSPLIT,
	NTV2SPLITMODE_HORZSLIT,
	NTV2SPLITMODE_VERTSLIT,
	NTV2SPLITMODE_INVALID
} NTV2SplitMode;


/**
	@brief		These enum values identify the Mixer/Keyer foreground and background input control values.
	@see		CNTV2Card::GetMixerFGInputControl, CNTV2Card::SetMixerFGInputControl, CNTV2Card::GetMixerBGInputControl, CNTV2Card::SetMixerBGInputControl, \ref widget_mixkey
**/
typedef enum
{
	NTV2MIXERINPUTCONTROL_FULLRASTER,
	NTV2MIXERINPUTCONTROL_SHAPED,
	NTV2MIXERINPUTCONTROL_UNSHAPED,
	NTV2MIXERINPUTCONTROL_INVALID
} NTV2MixerKeyerInputControl;

#define	NTV2_IS_VALID_MIXERINPUTCONTROL(__x__)		((__x__) >= NTV2MIXERINPUTCONTROL_FULLRASTER  &&  (__x__) < NTV2MIXERINPUTCONTROL_INVALID)

#if !defined (NTV2_DEPRECATE)
	typedef NTV2MixerKeyerInputControl	Xena2VidProcInputControl;
	#define	XENA2VIDPROCINPUTCONTROL_FULLRASTER		NTV2MIXERINPUTCONTROL_FULLRASTER
	#define	XENA2VIDPROCINPUTCONTROL_SHAPED			NTV2MIXERINPUTCONTROL_SHAPED
	#define	XENA2VIDPROCINPUTCONTROL_UNSHAPED		NTV2MIXERINPUTCONTROL_UNSHAPED
#endif	//	!defined (NTV2_DEPRECATE)


/**
	@brief		These enum values identify the mixer mode.
	@see		CNTV2Card::GetMixerMode, CNTV2Card::SetMixerMode, \ref widget_mixkey
**/
typedef enum
{
	NTV2MIXERMODE_FOREGROUND_ON,	///< @brief		Passes only foreground video + key to the Mixer output.
	NTV2MIXERMODE_MIX,				///< @brief		Overlays foreground video on top of background video.
	NTV2MIXERMODE_SPLIT,			///< @deprecated	Obsolete -- split-view is no longer supported.
	NTV2MIXERMODE_FOREGROUND_OFF,	///< @brief		Passes only background video + key to the Mixer output.
	NTV2MIXERMODE_INVALID			///< @brief		Invalid/uninitialized.
} NTV2MixerKeyerMode;

#define	NTV2_IS_VALID_MIXERMODE(__x__)		((__x__) >= NTV2MIXERMODE_FOREGROUND_ON  &&  (__x__) < NTV2MIXERMODE_INVALID)

#if !defined (NTV2_DEPRECATE)
	typedef NTV2MixerKeyerMode	Xena2VidProcMode;
	#define		XENAVIDPROCMODE_FOREGROUND_ON		NTV2MIXERMODE_FOREGROUND_ON
	#define		XENAVIDPROCMODE_MIX					NTV2MIXERMODE_MIX
	#define		XENAVIDPROCMODE_SPLIT				NTV2MIXERMODE_SPLIT
	#define		XENAVIDPROCMODE_FOREGROUND_OFF		NTV2MIXERMODE_FOREGROUND_OFF
#endif	//	!defined (NTV2_DEPRECATE)

typedef enum
{
	NTV2OUTPUTFILTER_NONE,
	NTV2OUTPUTFILTER_VERTICAL,
	NTV2OUTPUTFILTER_FIELD1,
	NTV2OUTPUTFILTER_FIELD2
} NTV2OutputFilter;

typedef enum
{
	NTV2PROCAMPSTANDARDDEFBRIGHTNESS,			/* SD Luma Offset					*/
	NTV2PROCAMPSTANDARDDEFCONTRAST,				/* SD Luma Gain 					*/
	NTV2PROCAMPSTANDARDDEFSATURATION,			/* SD Cb and Cr Gain				*/
	NTV2PROCAMPSTANDARDDEFHUE,					/* SD Composite and S-Video only 	*/
	NTV2PROCAMPHIGHDEFBRIGHTNESS,				/* HD Luma Offset					*/
	NTV2PROCAMPHIGHDEFCONTRAST,					/* HD Luma Gain 					*/
	NTV2PROCAMPHIGHDEFSATURATION,				/* HD Cb and Cr Gain 				*/
	NTV2PROCAMPHIGHDEFHUE						/* HD Hue, not implemented			*/
} NTV2ProcAmpControl;


#define NTV2_MAXBOARDS 8


typedef enum
{
	PROP_SETTINGS,
	PROP_TESTPATTERN,
	PROP_EXPORT,
	PROP_CAPTURE,
	PROP_VIDEOPROC,
	PROP_NONE
} NTV2Prop ;

/**
	@brief	These values are used to identify fields for interlaced video.
			See \ref fieldframeinterrupts and CNTV2Card::WaitForInputFieldID.
**/
typedef enum
{
	NTV2_FIELD0,	///< @brief	Identifies the first field in time for an interlaced video frame, or the first and only field in a progressive video frame.
	NTV2_FIELD1,	///< @brief	Identifies the last (second) field in time for an interlaced video frame.
	NTV2_FIELD_INVALID
} NTV2FieldID;

#define	NTV2_IS_VALID_FIELD(__x__)		((__x__) >= NTV2_FIELD0  &&  (__x__) < NTV2_FIELD_INVALID)


typedef enum
{
	DMA_READ,
	DMA_WRITE
} DMADirection;

typedef enum
{
	NTV2_PIO,   // don't change these equates 0-4
	NTV2_DMA1,
	NTV2_DMA2,
	NTV2_DMA3,
	NTV2_DMA4,
	NTV2_AUTODMA2,
	NTV2_AUTODMA3,
	NTV2_DMA_FIRST_AVAILABLE
} NTV2DMAEngine;

#define NTV2_NUM_DMA_ENGINES (NTV2_DMA4 - NTV2_DMA1 + 1)

typedef enum
{
	QUICKEXPORT_DESKTOP,
	QUICKEXPORT_WINDOW,
	QUICKEXPORT_CLIPBOARD,
	QUICKEXPORT_FILE
} QuickExportMode;

typedef enum
{
	NTV2CAPTURESOURCE_INPUT1,
	NTV2CAPTURESOURCE_INPUT2,
	NTV2CAPTURESOURCE_INPUT1_PLUS_INPUT2,
	NTV2CAPTURESOURCE_FRAMEBUFFER
} NTV2CaptureSource;

typedef enum
{
	NTV2CAPTUREDESTINATION_CLIPBOARD,
	NTV2CAPTUREDESTINATION_BMPFILE,
	NTV2CAPTUREDESTINATION_JPEGFILE,
	NTV2CAPTUREDESTINATION_YUVFILE,
	NTV2CAPTUREDESTINATION_TIFFFILE,
	NTV2CAPTUREDESTINATION_WINDOW,
	NTV2CAPTUREDESTINATION_PNGFILE,
	NTV2CAPTUREDESTINATION_FRAMEBUFFERONLY,	//	Just leave in FrameBuffer
	NTV2_MAX_NUM_CaptureDestinations
} NTV2CaptureDestination;

typedef enum
{
	NTV2CAPTUREMODE_FIELD,
	NTV2CAPTUREMODE_FRAME,
	NTV2_MAX_NUM_CaptureModes
} NTV2CaptureMode;

/**
	@brief	Represents the size of the audio buffer used by a device audio system for storing captured
			samples or samples awaiting playout. For example, NTV2_AUDIO_BUFFER_SIZE_4MB means that a
			4MB chunk of device memory is used to store captured audio samples, while another 4MB block
			of device memory is used to store audio samples for playout.
	@note	All NTV2 devices have standardized on 4MB audio buffers. Using a different value may result
			in unexpected behavior.
**/
typedef enum
{
	NTV2_AUDIO_BUFFER_SIZE_1MB	= 0,	//	0b00
	NTV2_AUDIO_BUFFER_SIZE_4MB	= 1,	//	0b01
	NTV2_AUDIO_BUFFER_STANDARD	= NTV2_AUDIO_BUFFER_SIZE_1MB,
	NTV2_AUDIO_BUFFER_BIG		= NTV2_AUDIO_BUFFER_SIZE_4MB,
#if !defined (NTV2_DEPRECATE)
	NTV2_AUDIO_BUFFER_SIZE_2MB	= 2,	//	0b10
	NTV2_AUDIO_BUFFER_SIZE_8MB	= 3,	//	0b11
	NTV2_AUDIO_BUFFER_MEDIUM	= NTV2_AUDIO_BUFFER_SIZE_2MB,
	NTV2_AUDIO_BUFFER_BIGGER	= NTV2_AUDIO_BUFFER_SIZE_8MB,
#endif	//	!defined (NTV2_DEPRECATE)
	NTV2_AUDIO_BUFFER_INVALID,
	NTV2_MAX_NUM_AudioBufferSizes	= NTV2_AUDIO_BUFFER_INVALID

} NTV2AudioBufferSize;

#define	NTV2_IS_VALID_AUDIO_BUFFER_SIZE(_x_)			((_x_) >= NTV2_AUDIO_BUFFER_STANDARD  &&  (_x_) < NTV2_MAX_NUM_AudioBufferSizes)


typedef enum
{
	NTV2_AUDIO_48K,
	NTV2_AUDIO_96K,
	NTV2_AUDIO_192K,
	NTV2_MAX_NUM_AudioRates,
	NTV2_AUDIO_RATE_INVALID	= NTV2_MAX_NUM_AudioRates
} NTV2AudioRate;

#define	NTV2_IS_VALID_AUDIO_RATE(_x_)			((_x_) < NTV2_MAX_NUM_AudioRates)


typedef enum
{
	NTV2_ENCODED_AUDIO_NORMAL,	// Normal, Sample Rate Converter enabled
	NTV2_ENCODED_AUDIO_SRC_DISABLED // AES ch. 1/2 encoded audio mode (SRC disabled)
} NTV2EncodedAudioMode;


typedef enum
{
	NTV2_AUDIO_FORMAT_LPCM,
	NTV2_AUDIO_FORMAT_DOLBY,
	NTV2_MAX_NUM_AudioFormats,
	NTV2_AUDIO_FORMAT_INVALID	= NTV2_MAX_NUM_AudioFormats
} NTV2AudioFormat;

#define	NTV2_IS_VALID_AUDIO_FORMAT(_x_)			((_x_) < NTV2_MAX_NUM_AudioFormats)

/**
	@brief	This enum value determines/states which SDI video input will be used to supply
			audio samples to an audio system.
			It assumes that the audio systems' audio source is set to NTV2_AUDIO_EMBEDDED.
	@see	CNTV2Card::SetEmbeddedAudioInput, CNTV2Card::GetEmbeddedAudioInput
**/
typedef enum
{
	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1,
	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2,
	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3,
	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4,
	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_5,
	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_6,
	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_7,
	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_8,
	NTV2_MAX_NUM_EmbeddedAudioInputs,
	NTV2_EMBEDDED_AUDIO_INPUT_INVALID	= NTV2_MAX_NUM_EmbeddedAudioInputs
} NTV2EmbeddedAudioInput;

#define	NTV2_IS_VALID_EMBEDDED_AUDIO_INPUT(_x_)			((_x_) >= NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1  &&  (_x_) < NTV2_EMBEDDED_AUDIO_INPUT_INVALID)


/**
	@brief	This enum value determines/states the device audio clock reference source.
			It was important to set this to ::NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT on older devices.
			Newer devices always use ::NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT and cannot be changed.
	@see	CNTV2Card::GetEmbeddedAudioClock, CNTV2Card::SetEmbeddedAudioClock, \ref audiooperation
**/
typedef enum
{
	NTV2_EMBEDDED_AUDIO_CLOCK_REFERENCE,	///< @brief	Audio clock derived from the device reference
	NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT,	///< @brief	Audio clock derived from the video input
	NTV2_MAX_NUM_EmbeddedAudioClocks,
	NTV2_EMBEDDED_AUDIO_CLOCK_INVALID	=	NTV2_MAX_NUM_EmbeddedAudioClocks
} NTV2EmbeddedAudioClock;

#define	NTV2_IS_VALID_EMBEDDED_AUDIO_CLOCK(_x_)			((_x_) < NTV2_MAX_NUM_EmbeddedAudioClocks)


/**
	@brief	This enum value determines/states where an audio system will obtain its audio samples.
	@see	CNTV2Card::SetAudioSystemInputSource, CNTV2Card::GetAudioSystemInputSource, \ref audiocapture
**/
typedef enum
{
	NTV2_AUDIO_EMBEDDED,		///< @brief	Obtain audio samples from the audio that's embedded in the video HANC
	NTV2_AUDIO_AES,				///< @brief	Obtain audio samples from the device AES inputs, if available.
	NTV2_AUDIO_ANALOG,			///< @brief	Obtain audio samples from the device analog input(s), if available.
	NTV2_AUDIO_HDMI,			///< @brief	Obtain audio samples from the device HDMI input, if available
	NTV2_AUDIO_MIC,
	NTV2_MAX_NUM_AudioSources,
	NTV2_AUDIO_SOURCE_INVALID	= NTV2_MAX_NUM_AudioSources
} NTV2AudioSource;

#define	NTV2_AUDIO_SOURCE_IS_EMBEDDED(_x_)		((_x_) == NTV2_AUDIO_EMBEDDED)
#define	NTV2_AUDIO_SOURCE_IS_AES(_x_)			((_x_) == NTV2_AUDIO_AES)
#define	NTV2_AUDIO_SOURCE_IS_ANALOG(_x_)		((_x_) == NTV2_AUDIO_ANALOG)
#define	NTV2_AUDIO_SOURCE_IS_HDMI(_x_)			((_x_) == NTV2_AUDIO_HDMI)
#define	NTV2_IS_VALID_AUDIO_SOURCE(_x_)			((_x_) >= NTV2_AUDIO_EMBEDDED  &&  (_x_) < NTV2_AUDIO_SOURCE_INVALID)


/**
	@brief	This enum value determines/states if an audio output embedder will embed silence (zeroes)
			or de-embedded audio from an SDI input.
	@see	CNTV2Card::SetAudioLoopBack, CNTV2Card::GetAudioLoopBack, CNTV2Card::SetAudioSystemInputSource, CNTV2Card::GetAudioSystemInputSource, \ref audioplayout
**/
typedef enum
{
	NTV2_AUDIO_LOOPBACK_OFF,		///< @brief	Embeds silence (zeroes) into the data stream.
	NTV2_AUDIO_LOOPBACK_ON,			///< @brief	Embeds SDI input source audio into the data stream.
	NTV2_AUDIO_LOOPBACK_INVALID
} NTV2AudioLoopBack;

#define	NTV2_IS_VALID_AUDIO_LOOPBACK(_x_)		((_x_) >= NTV2_AUDIO_LOOPBACK_OFF  &&  (_x_) < NTV2_AUDIO_LOOPBACK_INVALID)

/**
	@brief	Determines the order that raster lines are written into, or read out of, frame buffer memory on the device.
**/
typedef enum
{
	NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN,		///< @brief	Raster lines are read/written top-to-bottom from/to frame buffer memory.
	NTV2_FRAMEBUFFER_ORIENTATION_NORMAL			= NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN,
	NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP,		///< @brief	Raster lines are read/written bottom-to-top from/to frame buffer memory.
	NTV2_FRAMEBUFFER_ORIENTATION_INVALID,
	NTV2_MAX_NUM_VideoFrameBufferOrientations	= NTV2_FRAMEBUFFER_ORIENTATION_INVALID
} NTV2VideoFrameBufferOrientation, NTV2FrameBufferOrientation, NTV2FBOrientation;

#define	NTV2_IS_VALID_FRAMEBUFFER_ORIENTATION(_x_)		((_x_) >= NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN  &&  (_x_) < NTV2_MAX_NUM_VideoFrameBufferOrientations)
#define	NTV2_IS_FRAMEBUFFER_ORIENTATION_FLIPPED(_x_)	((_x_) == NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP)


typedef enum
{
	NTV2_CCHOSTACCESS_CH1BANK0,
	NTV2_CCHOSTACCESS_CH1BANK1,
	NTV2_CCHOSTACCESS_CH2BANK0,
	NTV2_CCHOSTACCESS_CH2BANK1,
	NTV2_CCHOSTACCESS_CH3BANK0,
	NTV2_CCHOSTACCESS_CH3BANK1,
	NTV2_CCHOSTACCESS_CH4BANK0,
	NTV2_CCHOSTACCESS_CH4BANK1,
	NTV2_CCHOSTACCESS_CH5BANK0,
	NTV2_CCHOSTACCESS_CH5BANK1,
	NTV2_CCHOSTACCESS_CH6BANK0,
	NTV2_CCHOSTACCESS_CH6BANK1,
	NTV2_CCHOSTACCESS_CH7BANK0,
	NTV2_CCHOSTACCESS_CH7BANK1,
	NTV2_CCHOSTACCESS_CH8BANK0,
	NTV2_CCHOSTACCESS_CH8BANK1,
	NTV2_MAX_NUM_ColorCorrectionHostAccessBanks
}NTV2ColorCorrectionHostAccessBank;

typedef enum
{
	NTV2_CCMODE_OFF,
	NTV2_CCMODE_RGB,
	NTV2_CCMODE_YCbCr,
	NTV2_CCMODE_3WAY,
	NTV2_MAX_NUM_ColorCorrectionModes,
	NTV2_CCMODE_INVALID	= NTV2_MAX_NUM_ColorCorrectionModes
} NTV2ColorCorrectionMode;

#define	NTV2_IS_VALID_COLOR_CORRECTION_MODE(__x__)		((__x__) >= NTV2_CCMODE_OFF && (__x__) < NTV2_MAX_NUM_ColorCorrectionModes)
#define	NTV2_IS_ACTIVE_COLOR_CORRECTION_MODE(__x__)		(NTV2_IS_VALID_COLOR_CORRECTION_MODE (__x__) && (__x__) != NTV2_CCMODE_OFF)


/////////////////////////////////////////////////////////////////////////////////////
// RP188 (timecode) enum and structs - added for oem driver
typedef enum
{
	NTV2_RP188_INPUT,
	NTV2_RP188_OUTPUT,
	NTV2_MAX_NUM_RP188Modes,
	NTV2_RP188_INVALID	= NTV2_MAX_NUM_RP188Modes
} NTV2_RP188Mode;   // matches sense of hardware register bit

#define	NTV2_IS_VALID_RP188_MODE(__x__)		((__x__) >= NTV2_RP188_INPUT && (__x__) < NTV2_MAX_NUM_RP188Modes)


typedef enum
{
	NTV2_AUDIOPLAYBACK_NOW,
	NTV2_AUDIOPLAYBACK_NEXTFRAME,
	NTV2_AUDIOPLAYBACK_NORMALAUTOCIRCULATE,   // default
	NTV2_AUDIOPLAYBACK_1STAUTOCIRCULATEFRAME  // only works for channelspec = NTV2CROSSPOINT_CHANNEL1
} NTV2_GlobalAudioPlaybackMode;

//////////////////////////////////////////////////////////////////////////////////////
/// Kona2/Xena2 specific enums
typedef enum
{
	NTV2_FRAMESIZE_2MB,
	NTV2_FRAMESIZE_4MB,
	NTV2_FRAMESIZE_8MB,
	NTV2_FRAMESIZE_16MB,
	NTV2_FRAMESIZE_6MB,
	NTV2_FRAMESIZE_10MB,
	NTV2_FRAMESIZE_12MB,
	NTV2_FRAMESIZE_14MB,
	NTV2_FRAMESIZE_18MB,
	NTV2_FRAMESIZE_20MB,
	NTV2_FRAMESIZE_22MB,
	NTV2_FRAMESIZE_24MB,
	NTV2_FRAMESIZE_26MB,
	NTV2_FRAMESIZE_28MB,
	NTV2_FRAMESIZE_30MB,
	NTV2_FRAMESIZE_32MB,
	NTV2_MAX_NUM_Framesizes,
	NTV2_FRAMESIZE_INVALID = NTV2_MAX_NUM_Framesizes
} NTV2Framesize;

#define	NTV2_IS_VALID_FRAMESIZE(__x__)		((__x__) >= NTV2_FRAMESIZE_2MB  &&  (__x__) < NTV2_MAX_NUM_Framesizes)
#define	NTV2_IS_VALID_8MB_FRAMESIZE(__x__)	((__x__) == NTV2_FRAMESIZE_8MB  ||  (__x__) == NTV2_FRAMESIZE_16MB  ||  (__x__) == NTV2_FRAMESIZE_32MB)


typedef enum
{
	NTV2_480iRGB,
	NTV2_480iYPbPrSMPTE,
	NTV2_480iYPbPrBetacam525,
	NTV2_480iYPbPrBetacamJapan,
	NTV2_480iNTSC_US_Composite,
	NTV2_480iNTSC_Japan_Composite,
	NTV2_576iRGB,
	NTV2_576iYPbPrSMPTE,
	NTV2_576iPAL_Composite,
	NTV2_1080iRGB,
	NTV2_1080psfRGB,
	NTV2_720pRGB,
	NTV2_1080iSMPTE,
	NTV2_1080psfSMPTE,
	NTV2_720pSMPTE,
	NTV2_1080iXVGA,
	NTV2_1080psfXVGA,
	NTV2_720pXVGA,
	NTV2_2Kx1080RGB,
	NTV2_2Kx1080SMPTE,
	NTV2_2Kx1080XVGA,
	NTV2_END_DACMODES,
	NTV2_MAX_NUM_VideoDACModes,
	NTV2_VIDEO_DAC_MODE_INVALID = NTV2_END_DACMODES
} NTV2VideoDACMode;

#define	NTV2_IS_VALID_VIDEO_DAC_MODE(__x__)		((__x__) >= NTV2_480iRGB  &&  (__x__) < NTV2_END_DACMODES)


typedef enum
{
	NTV2LHI_480iRGB						= 0xC,
	NTV2LHI_480iYPbPrSMPTE				= 0x8,
	NTV2LHI_480iYPbPrBetacam525			= 0x9,
	NTV2LHI_480iYPbPrBetacamJapan		= 0xA,
	NTV2LHI_480iNTSC_US_Composite		= 0x1,
	NTV2LHI_480iNTSC_Japan_Composite	= 0x2,
	NTV2LHI_576iRGB						= 0xC,
	NTV2LHI_576iYPbPrSMPTE				= 0x8,
	NTV2LHI_576iPAL_Composite			= 0x0,
	NTV2LHI_1080iRGB					= 0xC,
	NTV2LHI_1080psfRGB					= 0xC,
	NTV2LHI_1080iSMPTE					= 0x8,
	NTV2LHI_1080psfSMPTE				= 0x8,
	NTV2LHI_720pRGB						= 0xC,
	NTV2LHI_720pSMPTE					= 0x8,
	NTV2_MAX_NUM_LHIVideoDACModes
} NTV2LHIVideoDACMode;


// GetAnalogInputADCMode
typedef enum
{
	NTV2_480iADCComponentBeta,			//	0
	NTV2_480iADCComponentSMPTE,			//	1
	NTV2_480iADCSVideoUS,				//	2
	NTV2_480iADCCompositeUS,			//	3
	NTV2_480iADCComponentBetaJapan,		//	4
	NTV2_480iADCComponentSMPTEJapan,	//	5
	NTV2_480iADCSVideoJapan,			//	6
	NTV2_480iADCCompositeJapan,			//	7
	NTV2_576iADCComponentBeta,			//	8
	NTV2_576iADCComponentSMPTE,			//	9
	NTV2_576iADCSVideo,					//	10
	NTV2_576iADCComposite,				//	11
	NTV2_720p_60,	//	60 + 59.94		//	12
	NTV2_1080i_30,	//	30 + 29.97		//	13
	NTV2_720p_50,						//	14
	NTV2_1080i_25,						//	15
	NTV2_1080pSF24,	// 24 + 23.98		//	16
	NTV2_MAX_NUM_LSVideoADCModes		//	17
} NTV2LSVideoADCMode;


// Up/Down/Cross Converter modes
typedef enum
{
	NTV2_UpConvertAnamorphic,
	NTV2_UpConvertPillarbox4x3,
	NTV2_UpConvertZoom14x9,
	NTV2_UpConvertZoomLetterbox,
	NTV2_UpConvertZoomWide,
	NTV2_MAX_NUM_UpConvertModes,
	NTV2_UpConvertMode_Invalid	= NTV2_MAX_NUM_UpConvertModes
} NTV2UpConvertMode;


#if !defined(NTV2_DEPRECATE_16_1)
	typedef enum
	{
		NTV2_AnalogAudioIO_8Out,
		NTV2_AnalogAudioIO_4In_4Out,
		NTV2_AnalogAudioIO_4Out_4In,
		NTV2_AnalogAudioIO_8In
	} NTV2AnalogAudioIO;
#endif	//	!defined(NTV2_DEPRECATE_16_1)


typedef enum
{
	NTV2_DownConvertLetterbox,
	NTV2_DownConvertCrop,
	NTV2_DownConvertAnamorphic,
	NTV2_DownConvert14x9,
	NTV2_MAX_NUM_DownConvertModes,
	NTV2_DownConvertMode_Invalid	= NTV2_MAX_NUM_DownConvertModes
} NTV2DownConvertMode;


typedef enum
{
	NTV2_IsoLetterBox,
	NTV2_IsoHCrop,
	NTV2_IsoPillarBox,
	NTV2_IsoVCrop,
	NTV2_Iso14x9,
	NTV2_IsoPassThrough,
	NTV2_MAX_NUM_IsoConvertModes,
	NTV2_IsoConvertMode_Invalid	= NTV2_MAX_NUM_IsoConvertModes
} NTV2IsoConvertMode;


// This specifies the range of levels for 10-bit RGB (aka DualLink)
typedef enum
{
	NTV2_RGB10RangeFull,		//	Levels are 0 - 1023 (Full)
	NTV2_RGB10RangeSMPTE,		//	Levels are 64 - 940 (SMPTE)
	NTV2_MAX_NUM_RGB10Ranges
} NTV2RGB10Range;

typedef enum
{
	NTV2_MixerRGBRangeFull,
	NTV2_MixerRGBRangeSMPTE
} NTV2MixerRGBRange;


typedef enum
{
	NTV2_AudioMap12_12,
	NTV2_AudioMap34_12,
	NTV2_AudioMap56_12,
	NTV2_AudioMap78_12,
	NTV2_AudioMap910_12,
	NTV2_AudioMap1112_12,
	NTV2_AudioMap1314_12,
	NTV2_AudioMap1516_12,
	NTV2_MAX_NUM_AudioMapSelectEnums
} NTV2AudioMapSelect;


typedef enum					// Virtual Register: kVRegInputSelect - set in services
{
	NTV2_Input1Select,
	NTV2_Input2Select,
	NTV2_Input3Select,
	NTV2_Input4Select,
	NTV2_Input5Select,
	NTV2_Input2xDLHDSelect,
	NTV2_Input2x4kSelect,
	NTV2_Input4x4kSelect,
	NTV2_Input2x8kSelect,
	NTV2_Input4x8kSelect,
	NTV2_InputAutoSelect,
	NTV2_MAX_NUM_InputVideoSelectEnums
} NTV2InputVideoSelect;


typedef enum
{
	NTV2_AUDIOLEVEL_24dBu,
	NTV2_AUDIOLEVEL_18dBu,
	NTV2_AUDIOLEVEL_12dBu,
	NTV2_AUDIOLEVEL_15dBu,
	NTV2_MAX_NUM_AudioLevels
} NTV2AudioLevel;


#if !defined(R2_DEPRECATE)

typedef enum					// Deprecated
{
	NTV2_GammaNone,				// don't change LUTs for gamma (aka "Custom")
	NTV2_GammaAuto,				// switch between Rec 601 for SD and Rec 709 for HD
	NTV2_GammaMac,				// 1.8 "Macintosh" Power-function gamma
	NTV2_GammaRec601,			// 2.2 Rec 601 Power-function gamma
	NTV2_GammaRec709,			// 2.22.. Rec 709 gamma
	NTV2_GammaPC,				// 2.5 "PC" Power-function gamma
	NTV2_MAX_NUM_GammaTypes
} NTV2GammaType;


typedef enum						// Deprecated
{
	NTV2_ColorSpaceModeAuto,		// Auto Select
	NTV2_ColorSpaceModeYCbCr,		// YCbCr (TBD, add 420, 444 options)
	NTV2_ColorSpaceModeRgb,			// RGB
	NTV2_MAX_NUM_ColorSpaceModes
} NTV2ColorSpaceMode;


typedef enum						// Deprecated
{
	NTV2_RGBRangeAuto,				// don't change LUTs for gamma (aka "Custom")
	NTV2_RGBRangeFull,				// Levels are 0 - 1023 (Full)
	NTV2_RGBRangeSMPTE,				// Levels are 64 - 940 (SMPTE)
	NTV2_MAX_NUM_RGBRangeModes
} NTV2RGBRangeMode;


typedef enum
{
	NTV2_AnlgComposite,			// Composite or Composite
	NTV2_AnlgComponentSMPTE,	// Component (SMPTE/N10 levels)
	NTV2_AnlgComponentBetacam,	// Component (Betacam levels)
	NTV2_AnlgComponentRGB,		// Component (RGB)
	NTV2_AnlgXVGA,				// xVGA
	NTV2_AnlgSVideo,			// S-Video
	NTV2_MAX_NUM_AnalogTypes
} NTV2AnalogType;


typedef enum
{
	NTV2_Black75IRE,			//	7.5 IRE (NTSC-US)
	NTV2_Black0IRE,				//	0   IRE (NTSC-J)
	NTV2_MAX_NUM_AnalogBlackLevels
} NTV2AnalogBlackLevel;


#if !defined(NTV2_DEPRECATE_15_1)
	typedef enum
	{
		NTV2_YUVSelect,
		NTV2_RGBSelect,
		NTV2_Stereo3DSelect,
		NTV2_NUM_SDIInputFormats
	} NTV2SDIInputFormatSelect;
#endif	//	!defined(NTV2_DEPRECATE_15_1)


typedef enum								// Deprecated
{
	NTV2_DeviceUnavailable		= -1,
	NTV2_DeviceNotInitialized	= 0,
	NTV2_DeviceInitialized		= 1
} NTV2DeviceInitialized;


typedef enum								// Deprecated
{
	NTV2_AES_EBU_XLRSelect,
	NTV2_AES_EBU_BNCSelect,
	NTV2_Input1Embedded1_8Select,
	NTV2_Input1Embedded9_16Select,
	NTV2_Input2Embedded1_8Select,
	NTV2_Input2Embedded9_16Select,
	NTV2_AnalogSelect,
	NTV2_HDMISelect,
	NTV2_AudioInputOther,
	NTV2_MicInSelect,
	NTV2_HDMI2Select,
	NTV2_HDMI3Select,
	NTV2_HDMI4Select,
	NTV2_AutoAudioSelect,
	NTV2_MAX_NUM_InputAudioSelectEnums
} NTV2InputAudioSelect;

typedef enum
{
	NTV2_PrimaryOutputSelect,
	NTV2_SecondaryOutputSelect,
	NTV2_RgbOutputSelect,					// Deprecated
	NTV2_VideoPlusKeySelect,
	NTV2_StereoOutputSelect,
	NTV2_Quadrant1Select,
	NTV2_Quadrant2Select,
	NTV2_Quadrant3Select,
	NTV2_Quadrant4Select,
	NTV2_Quarter4k,
	NTV2_4kHalfFrameRate,					// deprecated
	NTV2_QuadView,
	NTV2_AutoOutputSelect,
	NTV2_MAX_NUM_OutputVideoSelectEnums
} NTV2OutputVideoSelect;


typedef enum								// Deprecated
{
	NTV2_SDITransport_Off,					// transport disabled, disconnected
	NTV2_SDITransport_1_5,					// Single Link, 1 wire 1.5G
	NTV2_SDITransport_3Ga,					// Single Link, 1 wire 3Ga
	NTV2_SDITransport_DualLink_1_5,			// Dual Link, 2 wire 1.5G
	NTV2_SDITransport_DualLink_3Gb,			// Dual Link, 1 wire 3Gb
	NTV2_SDITransport_QuadLink_1_5,			// Quad Link, 4 wire 1.5G (4K YUV)
	NTV2_SDITransport_QuadLink_3Gb,			// Quad Link, 2 wire 3Gb (4K YUV or Stereo RGB)
	NTV2_SDITransport_QuadLink_3Ga,			// Quad Link, 4 wire 3Ga (4K HFR)
	NTV2_SDITransport_OctLink_3Gb,			// Oct Link, 4 wire 3Gb (4K RGB, HFR)
	NTV2_SDITransport_6G,					// 6G see Quad Link
	NTV2_SDITransport_12G,					// 12G see Oct Link
	NTV2_MAX_NUM_SDITransportTypes,			// last
	NTV2_SDITransport_Auto					// auto mode
		=NTV2_MAX_NUM_SDITransportTypes	
} NTV2SDITransportType;


typedef enum								// Deprecated
{
	NTV2_4kTransport_Auto,					// auto
	NTV2_4kTransport_Quadrants_2wire,		// quads 2x wire
	NTV2_4kTransport_Quadrants_4wire,		// quads 4x wire
	NTV2_4kTransport_PixelInterleave,		// SMPTE 425-5 & 425-3
	NTV2_4kTransport_Quarter_1wire,			// quarter size
	NTV2_4kTransport_12g_6g_1wire,			// 12G / 6G 1wire
	NTV2_MAX_NUM_4kTransportTypes
} NTV24kTransportType;


typedef enum								// Deprecated
{
	NTV2_PanModeOff,
	NTV2_PanModeReserved,
	NTV2_PanMode2Kx1080,
	NTV2_PanMode1920x1080,
	NTV2_MAX_NUM_PanModes
} NTV2PanMode;


#if !defined(NTV2_DEPRECATE_15_1)
	// note: Pause Mode is a "software" feature - not performed in hardware
	typedef enum
	{
		NTV2_PauseOnFrame,
		NTV2_PauseOnField
	} NTV2PauseModeType;

	// note: 24 fps <-> 30 fps Pulldown is a "software" feature - not performed in hardware
	typedef enum
	{
		NTV2_Pulldown2323,
		NTV2_Pulldown2332,
		NTV2_Pulldown2224
	} NTV2PulldownPatternType;
#endif	//	!defined(NTV2_DEPRECATE_15_1)


//	NOTE:	Timecode Burn-In Mode is a "software" feature - not performed in hardware
typedef enum							// Deprecated
{
	NTV2_TimecodeBurnInOff,				//	no burn-in
	NTV2_TimecodeBurnInTC,				//	display current timecode
	NTV2_TimecodeBurnInUB,				//	display current user bits
	NTV2_TimecodeBurnInFrameCount,		//	display current frame count
	NTV2_TimecodeBurnInQuickTime,		//	(like frame count, but shows Mac QuickTime frame time)
	NTV2_MAX_NUM_TimecodeBurnInModeTypes
} NTV2TimecodeBurnInModeType;


// not in use?
//	This specifies the endian 10-bit RGB (aka DualLink)
typedef enum
{
	NTV2_RGB10LittleEndian,		//	Little Endian
	NTV2_RGB10BigEndian,		//	Big Endian
	NTV2_MAX_NUM_RGB10EndianEnums
} NTV2RGB10Endian;

#endif // R2_DEPRECATE



#if !defined (NTV2_DEPRECATE)
	// Audio Channel Mapping and Channel Gain/Phase controls used in FS1
	typedef enum				// used in FS1
	{
		NTV2_AUDIOCHANNELMAPPING_EMB1CH1 = 0,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH2,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH3,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH4,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH5,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH6,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH7,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH8,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH1 = 8,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH2,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH3,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH4,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH5,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH6,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH7,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH8,
		NTV2_AUDIOCHANNELMAPPING_ANALOGCH1 = 16,
		NTV2_AUDIOCHANNELMAPPING_ANALOGCH2,
		NTV2_AUDIOCHANNELMAPPING_ANALOGCH3,
		NTV2_AUDIOCHANNELMAPPING_ANALOGCH4,
		NTV2_AUDIOCHANNELMAPPING_ANALOGCH5,
		NTV2_AUDIOCHANNELMAPPING_ANALOGCH6,
		NTV2_AUDIOCHANNELMAPPING_ANALOGCH7,
		NTV2_AUDIOCHANNELMAPPING_ANALOGCH8,
		NTV2_AUDIOCHANNELMAPPING_AESCH1 = 24,
		NTV2_AUDIOCHANNELMAPPING_AESCH2,
		NTV2_AUDIOCHANNELMAPPING_AESCH3,
		NTV2_AUDIOCHANNELMAPPING_AESCH4,
		NTV2_AUDIOCHANNELMAPPING_AESCH5,
		NTV2_AUDIOCHANNELMAPPING_AESCH6,
		NTV2_AUDIOCHANNELMAPPING_AESCH7,
		NTV2_AUDIOCHANNELMAPPING_AESCH8,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH9 = 32,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH10,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH11,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH12,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH13,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH14,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH15,
		NTV2_AUDIOCHANNELMAPPING_EMB1CH16,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH9 = 40,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH10,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH11,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH12,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH13,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH14,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH15,
		NTV2_AUDIOCHANNELMAPPING_EMB2CH16
	} NTV2AudioChannelMapping;
#endif	//	!defined (NTV2_DEPRECATE)


#if !defined (NTV2_DEPRECATE)
	typedef enum
	{
		NTV2FS1AFDInsertMode_Off,
		NTV2FS1AFDInsertMode_Delete,
		NTV2FS1AFDInsertMode_Pass,
		NTV2FS1AFDInsertMode_Replace
	} NTV2AFDInsertMode;

	typedef enum
	{
		NTV2FS1AFD_InsertAR_4x3,
		NTV2FS1AFD_InsertAR_16x9
	} NTV2AFDInsertAspectRatio;

	typedef enum
	{
		NTV2AFDInsert_LetterboxTo16x9	= 0x04,
		NTV2AFDInsert_FullFrame			= 0x08,
		NTV2AFDInsert_Pillarbox4x3		= 0x09,
		NTV2AFDInsert_FullFrameProtected = 0x0A,
		NTV2AFDInsert_14x9				= 0x0B,
		NTV2AFDInsert_4x3Alt14x9		= 0x0D,
		NTV2AFDInsert_16x9Alt14x9		= 0x0E,
		NTV2AFDInsert_16x9Alt4x3		= 0x0F
	} NTV2AFDInsertCode;
#endif	//	!defined (NTV2_DEPRECATE)


typedef enum
{
	NTV2_QuarterSizeExpandOff,		//	Normal 1:1 playback
	NTV2_QuarterSizeExpandOn,		//	Hardware will pixel-double and line-double to expand quarter-size frame
	NTV2_MAX_NUM_QuarterSizeExpandModes,
	NTV2_QuarterSizeExpandInvalid = NTV2_MAX_NUM_QuarterSizeExpandModes
} NTV2QuarterSizeExpandMode;

#define	NTV2_IS_VALID_QuarterSizeExpandMode(__q__)		((__q__) == NTV2_QuarterSizeExpandOff || (__q__) == NTV2_QuarterSizeExpandOn)


typedef enum
{
	NTV2_StandardQuality		= 0x0,
	NTV2_HighQuality			= 0x1,
	NTV2_ProRes					= NTV2_StandardQuality,
	NTV2_ProResHQ				= NTV2_HighQuality,
	NTV2_ProResLT				= 0x2,
	NTV2_ProResProxy			= 0x4,
	NTV2_MAX_NUM_FrameBufferQuality,
	NTV2_FBQualityInvalid = NTV2_MAX_NUM_FrameBufferQuality
} NTV2FrameBufferQuality;

#define	NTV2_IS_VALID_FrameBufferQuality(__q__)		((__q__) == NTV2_StandardQuality || (__q__) == NTV2_HighQuality || (__q__) == NTV2_ProResLT || (__q__) == NTV2_ProResProxy)


typedef enum
{
	NTV2_NoPSF,		//	Currently only used for ProRes encoder
	NTV2_IsPSF,
	NTV2_INVALID_EncodeAsPSF
} NTV2EncodeAsPSF;

#define	NTV2_IS_VALID_EncodeAsPSF(__x__)		((__x__) == NTV2_NoPSF || (__x__) == NTV2_IsPSF)


/**
	@brief	Identifies a widget output, a signal source, that potentially can drive
			another widget's input (identified by ::NTV2InputCrosspointID).
	@see	CNTV2Card::Connect and also \ref ntv2signalrouting
**/
typedef enum NTV2OutputCrosspointID
{
	NTV2_FIRST_OUTPUT_CROSSPOINT		= 0x00
	,NTV2_XptBlack						= 0x00
	,NTV2_XptSDIIn1						= 0x01
	,NTV2_XptSDIIn2						= 0x02
	,NTV2_XptLUT1YUV					= 0x04
	,NTV2_XptLUT1Out					= NTV2_XptLUT1YUV | 0x80
	,NTV2_XptCSC1VidYUV					= 0x05
	,NTV2_XptCSC1VidRGB					= NTV2_XptCSC1VidYUV | 0x80
	,NTV2_XptConversionModule			= 0x06
	,NTV2_XptCompressionModule			= 0x07
	,NTV2_XptFrameBuffer1YUV			= 0x08
	,NTV2_XptFrameBuffer1RGB			= NTV2_XptFrameBuffer1YUV | 0x80
	,NTV2_XptFrameSync1YUV				= 0x09
	,NTV2_XptFrameSync1RGB				= NTV2_XptFrameSync1YUV | 0x80
	,NTV2_XptFrameSync2YUV				= 0x0A
	,NTV2_XptFrameSync2RGB				= NTV2_XptFrameSync2YUV | 0x80
	,NTV2_XptDuallinkOut1				= 0x0B
	,NTV2_XptAlphaOut					= 0x0C
	,NTV2_XptCSC1KeyYUV					= 0x0E
	,NTV2_XptFrameBuffer2YUV			= 0x0F
	,NTV2_XptFrameBuffer2RGB			= NTV2_XptFrameBuffer2YUV | 0x80
	,NTV2_XptCSC2VidYUV					= 0x10
	,NTV2_XptCSC2VidRGB					= NTV2_XptCSC2VidYUV | 0x80
	,NTV2_XptCSC2KeyYUV					= 0x11
	,NTV2_XptMixer1VidYUV				= 0x12
	,NTV2_XptMixer1KeyYUV				= 0x13
	,NTV2_XptMixer1VidRGB				= NTV2_XptMixer1VidYUV  | 0x80
	,NTV2_XptMultiLinkOut1DS1			= 0x14								///< @brief	New in SDK 16.0
	,NTV2_XptMultiLinkOut1DS2			= 0x15								///< @brief	New in SDK 16.0
	,NTV2_XptAnalogIn					= 0x16
	,NTV2_XptHDMIIn1					= 0x17
	,NTV2_XptHDMIIn1RGB					= NTV2_XptHDMIIn1 | 0x80
	,NTV2_XptMultiLinkOut1DS3			= 0x18								///< @brief	New in SDK 16.0
	,NTV2_XptMultiLinkOut1DS4			= 0x19								///< @brief	New in SDK 16.0
	,NTV2_XptMultiLinkOut2DS1			= 0x1A								///< @brief	New in SDK 16.0
	,NTV2_XptMultiLinkOut2DS2			= 0x1B								///< @brief	New in SDK 16.0
	,NTV2_XptDuallinkOut2				= 0x1C
	,NTV2_XptTestPatternYUV				= 0x1D
	,NTV2_XptSDIIn1DS2					= 0x1E
	,NTV2_XptSDIIn2DS2					= 0x1F
	,NTV2_XptMixer2VidYUV				= 0x20
	,NTV2_XptMixer2KeyYUV				= 0x21
	,NTV2_XptMixer2VidRGB				= NTV2_XptMixer2VidYUV | 0x80
	,NTV2_XptOEOutYUV					= 0x22
	,NTV2_XptOEOutRGB					= NTV2_XptOEOutYUV | 0x80
	,NTV2_XptStereoCompressorOut		= 0x23
	,NTV2_XptFrameBuffer3YUV			= 0x24
	,NTV2_XptFrameBuffer3RGB			= NTV2_XptFrameBuffer3YUV | 0x80
	,NTV2_XptFrameBuffer4YUV			= 0x25
	,NTV2_XptFrameBuffer4RGB			= NTV2_XptFrameBuffer4YUV | 0x80
	,NTV2_XptDuallinkOut1DS2			= 0x26
	,NTV2_XptDuallinkOut2DS2			= 0x27
	,NTV2_XptCSC5VidYUV					= 0x2C
	,NTV2_XptCSC5VidRGB					= NTV2_XptCSC5VidYUV | 0x80
	,NTV2_XptCSC5KeyYUV					= 0x2D
	,NTV2_XptMultiLinkOut2DS3			= 0x2E								///< @brief	New in SDK 16.0
	,NTV2_XptMultiLinkOut2DS4			= 0x2F								///< @brief	New in SDK 16.0
	,NTV2_XptSDIIn3						= 0x30
	,NTV2_XptSDIIn4						= 0x31
	,NTV2_XptSDIIn3DS2					= 0x32
	,NTV2_XptSDIIn4DS2					= 0x33
	,NTV2_XptDuallinkOut3				= 0x36
	,NTV2_XptDuallinkOut3DS2			= 0x37
	,NTV2_XptDuallinkOut4				= 0x38
	,NTV2_XptDuallinkOut4DS2			= 0x39
	,NTV2_XptCSC3VidYUV					= 0x3A
	,NTV2_XptCSC3VidRGB					= NTV2_XptCSC3VidYUV | 0x80
	,NTV2_XptCSC3KeyYUV					= 0x3B
	,NTV2_XptCSC4VidYUV					= 0x3C
	,NTV2_XptCSC4VidRGB					= NTV2_XptCSC4VidYUV | 0x80
	,NTV2_XptCSC4KeyYUV					= 0x3D
	,NTV2_XptDuallinkOut5				= 0x3E
	,NTV2_XptDuallinkOut5DS2			= 0x3F
	,NTV2_Xpt3DLUT1YUV					= 0x40
	,NTV2_Xpt3DLUT1RGB					= NTV2_Xpt3DLUT1YUV | 0x80
	,NTV2_XptHDMIIn1Q2					= 0x41
	,NTV2_XptHDMIIn1Q2RGB				= NTV2_XptHDMIIn1Q2 | 0x80
	,NTV2_XptHDMIIn1Q3					= 0x42
	,NTV2_XptHDMIIn1Q3RGB				= NTV2_XptHDMIIn1Q3 | 0x80
	,NTV2_XptHDMIIn1Q4					= 0x43
	,NTV2_XptHDMIIn1Q4RGB				= NTV2_XptHDMIIn1Q4 | 0x80
	,NTV2_Xpt4KDownConverterOut			= 0x44
	,NTV2_Xpt4KDownConverterOutRGB		= NTV2_Xpt4KDownConverterOut | 0x80
	,NTV2_XptSDIIn5						= 0x45
	,NTV2_XptSDIIn6						= 0x46
	,NTV2_XptSDIIn5DS2					= 0x47
	,NTV2_XptSDIIn6DS2					= 0x48
	,NTV2_XptSDIIn7						= 0x49
	,NTV2_XptSDIIn8						= 0x4A
	,NTV2_XptSDIIn7DS2					= 0x4B
	,NTV2_XptSDIIn8DS2					= 0x4C
	,NTV2_XptFrameBuffer5YUV			= 0x51
	,NTV2_XptFrameBuffer5RGB			= NTV2_XptFrameBuffer5YUV | 0x80
	,NTV2_XptFrameBuffer6YUV			= 0x52
	,NTV2_XptFrameBuffer6RGB			= NTV2_XptFrameBuffer6YUV | 0x80
	,NTV2_XptFrameBuffer7YUV			= 0x53
	,NTV2_XptFrameBuffer7RGB			= NTV2_XptFrameBuffer7YUV | 0x80
	,NTV2_XptFrameBuffer8YUV			= 0x54
	,NTV2_XptFrameBuffer8RGB			= NTV2_XptFrameBuffer8YUV | 0x80
	,NTV2_XptMixer3VidYUV				= 0x55
	,NTV2_XptMixer3KeyYUV				= 0x56
	,NTV2_XptMixer3VidRGB				= NTV2_XptMixer3VidYUV | 0x80
	,NTV2_XptMixer4VidYUV				= 0x57
	,NTV2_XptMixer4KeyYUV				= 0x58
	,NTV2_XptMixer4VidRGB				= NTV2_XptMixer4VidYUV | 0x80
	,NTV2_XptCSC6VidYUV					= 0x59
	,NTV2_XptCSC6VidRGB					= NTV2_XptCSC6VidYUV | 0x80
	,NTV2_XptCSC6KeyYUV					= 0x5A
	,NTV2_XptCSC7VidYUV					= 0x5B
	,NTV2_XptCSC7VidRGB					= NTV2_XptCSC7VidYUV | 0x80
	,NTV2_XptCSC7KeyYUV					= 0x5C
	,NTV2_XptCSC8VidYUV					= 0x5D
	,NTV2_XptCSC8VidRGB					= NTV2_XptCSC8VidYUV | 0x80
	,NTV2_XptCSC8KeyYUV					= 0x5E
	,NTV2_XptDuallinkOut6				= 0x62
	,NTV2_XptDuallinkOut6DS2			= 0x63
	,NTV2_XptDuallinkOut7				= 0x64
	,NTV2_XptDuallinkOut7DS2			= 0x65
	,NTV2_XptDuallinkOut8				= 0x66
	,NTV2_XptDuallinkOut8DS2			= 0x67
	,NTV2_Xpt425Mux1AYUV				= 0x68
	,NTV2_Xpt425Mux1ARGB				= NTV2_Xpt425Mux1AYUV | 0x80
	,NTV2_Xpt425Mux1BYUV				= 0x69
	,NTV2_Xpt425Mux1BRGB				= NTV2_Xpt425Mux1BYUV | 0x80
	,NTV2_Xpt425Mux2AYUV				= 0x6A
	,NTV2_Xpt425Mux2ARGB				= NTV2_Xpt425Mux2AYUV | 0x80
	,NTV2_Xpt425Mux2BYUV				= 0x6B
	,NTV2_Xpt425Mux2BRGB				= NTV2_Xpt425Mux2BYUV | 0x80
	,NTV2_Xpt425Mux3AYUV				= 0x6C
	,NTV2_Xpt425Mux3ARGB				= NTV2_Xpt425Mux3AYUV | 0x80
	,NTV2_Xpt425Mux3BYUV				= 0x6D
	,NTV2_Xpt425Mux3BRGB				= NTV2_Xpt425Mux3BYUV | 0x80
	,NTV2_Xpt425Mux4AYUV				= 0x6E
	,NTV2_Xpt425Mux4ARGB				= NTV2_Xpt425Mux4AYUV | 0x80
	,NTV2_Xpt425Mux4BYUV				= 0x6F
	,NTV2_Xpt425Mux4BRGB				= NTV2_Xpt425Mux4BYUV | 0x80
	,NTV2_XptFrameBuffer1_DS2YUV		= 0x70
	,NTV2_XptFrameBuffer1_DS2RGB		= NTV2_XptFrameBuffer1_DS2YUV | 0x80
	,NTV2_XptFrameBuffer2_DS2YUV		= 0x71
	,NTV2_XptFrameBuffer2_DS2RGB		= NTV2_XptFrameBuffer2_DS2YUV | 0x80
	,NTV2_XptFrameBuffer3_DS2YUV		= 0x72
	,NTV2_XptFrameBuffer3_DS2RGB		= NTV2_XptFrameBuffer3_DS2YUV | 0x80
	,NTV2_XptFrameBuffer4_DS2YUV		= 0x73
	,NTV2_XptFrameBuffer4_DS2RGB		= NTV2_XptFrameBuffer4_DS2YUV | 0x80
	,NTV2_XptFrameBuffer5_DS2YUV		= 0x74
	,NTV2_XptFrameBuffer5_DS2RGB		= NTV2_XptFrameBuffer5_DS2YUV | 0x80
	,NTV2_XptFrameBuffer6_DS2YUV		= 0x75
	,NTV2_XptFrameBuffer6_DS2RGB		= NTV2_XptFrameBuffer6_DS2YUV | 0x80
	,NTV2_XptFrameBuffer7_DS2YUV		= 0x76
	,NTV2_XptFrameBuffer7_DS2RGB		= NTV2_XptFrameBuffer7_DS2YUV | 0x80
	,NTV2_XptFrameBuffer8_DS2YUV		= 0x77
	,NTV2_XptFrameBuffer8_DS2RGB		= NTV2_XptFrameBuffer8_DS2YUV | 0x80
	,NTV2_XptHDMIIn2					= 0x78
	,NTV2_XptHDMIIn2RGB					= NTV2_XptHDMIIn2 | 0x80
	,NTV2_XptHDMIIn2Q2					= 0x79
	,NTV2_XptHDMIIn2Q2RGB				= NTV2_XptHDMIIn2Q2 | 0x80
	,NTV2_XptHDMIIn2Q3					= 0x7A
	,NTV2_XptHDMIIn2Q3RGB				= NTV2_XptHDMIIn2Q3 | 0x80
	,NTV2_XptHDMIIn2Q4					= 0x7B
	,NTV2_XptHDMIIn2Q4RGB				= NTV2_XptHDMIIn2Q4 | 0x80
	,NTV2_XptHDMIIn3					= 0x7C
	,NTV2_XptHDMIIn3RGB					= NTV2_XptHDMIIn3 | 0x80
	,NTV2_XptHDMIIn4					= 0x7D
	,NTV2_XptHDMIIn4RGB					= NTV2_XptHDMIIn4 | 0x80
	,NTV2_XptDuallinkIn1				= 0x83
	,NTV2_XptLUT2Out					= 0x8D
	,NTV2_XptIICTRGB					= 0x95
	,NTV2_XptIICT2RGB					= 0x9B
	,NTV2_XptDuallinkIn2				= 0xA8
	,NTV2_XptLUT3Out					= 0xA9
	,NTV2_XptLUT4Out					= 0xAA
	,NTV2_XptLUT5Out					= 0xAB
	,NTV2_XptDuallinkIn3				= 0xB4
	,NTV2_XptDuallinkIn4				= 0xB5
	,NTV2_XptDuallinkIn5				= 0xCD
	,NTV2_XptDuallinkIn6				= 0xCE
	,NTV2_XptDuallinkIn7				= 0xCF
	,NTV2_XptDuallinkIn8				= 0xD0
	,NTV2_XptLUT6Out					= 0xDF
	,NTV2_XptLUT7Out					= 0xE0
	,NTV2_XptLUT8Out					= 0xE1
	,NTV2_XptRuntimeCalc				= 0xFF
	,NTV2_LAST_OUTPUT_CROSSPOINT		= 0xFF
	,NTV2_OUTPUT_CROSSPOINT_INVALID		= 0xFF
	#if !defined(NTV2_DEPRECATE_16_0)
		,NTV2_XptDCIMixerVidYUV			= NTV2_XptOEOutYUV
		,NTV2_XptDCIMixerVidRGB			= NTV2_XptOEOutRGB
		,NTV2_XptLUT1RGB				= NTV2_XptLUT1Out					///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptLUT1Out
		,NTV2_XptLUT2RGB				= NTV2_XptLUT2Out					///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptLUT2Out
		,NTV2_XptWaterMarkerYUV			= NTV2_XptMultiLinkOut1DS1			///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptMultiLinkOut1DS1
		,NTV2_XptWaterMarkerRGB			= NTV2_XptWaterMarkerYUV | 0x80		///< @deprecated	Removed in SDK 16.0
		,NTV2_XptWaterMarker2YUV		= NTV2_XptMultiLinkOut2DS1			///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptMultiLinkOut2DS1
		,NTV2_XptWaterMarker2RGB		= NTV2_XptWaterMarker2YUV | 0x80	///< @deprecated	Removed in SDK 16.0
	#endif
	#if !defined(NTV2_DEPRECATE_15_3)
		,NTV2_XptFrameBuffer1_425YUV	= NTV2_XptFrameBuffer1_DS2YUV		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer1_DS2YUV
		,NTV2_XptFrameBuffer1_425RGB	= NTV2_XptFrameBuffer1_DS2RGB		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer1_DS2RGB
		,NTV2_XptFrameBuffer2_425YUV	= NTV2_XptFrameBuffer2_DS2YUV		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer2_DS2YUV
		,NTV2_XptFrameBuffer2_425RGB	= NTV2_XptFrameBuffer2_DS2RGB		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer2_DS2RGB
		,NTV2_XptFrameBuffer3_425YUV	= NTV2_XptFrameBuffer3_DS2YUV		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer3_DS2YUV
		,NTV2_XptFrameBuffer3_425RGB	= NTV2_XptFrameBuffer3_DS2RGB		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer3_DS2RGB
		,NTV2_XptFrameBuffer4_425YUV	= NTV2_XptFrameBuffer4_DS2YUV		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer4_DS2YUV
		,NTV2_XptFrameBuffer4_425RGB	= NTV2_XptFrameBuffer4_DS2RGB		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer4_DS2RGB
		,NTV2_XptFrameBuffer5_425YUV	= NTV2_XptFrameBuffer5_DS2YUV		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer5_DS2YUV
		,NTV2_XptFrameBuffer5_425RGB	= NTV2_XptFrameBuffer5_DS2RGB		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer5_DS2RGB
		,NTV2_XptFrameBuffer6_425YUV	= NTV2_XptFrameBuffer6_DS2YUV		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer6_DS2YUV
		,NTV2_XptFrameBuffer6_425RGB	= NTV2_XptFrameBuffer6_DS2RGB		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer6_DS2RGB
		,NTV2_XptFrameBuffer7_425YUV	= NTV2_XptFrameBuffer7_DS2YUV		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer7_DS2YUV
		,NTV2_XptFrameBuffer7_425RGB	= NTV2_XptFrameBuffer7_DS2RGB		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer7_DS2RGB
		,NTV2_XptFrameBuffer8_425YUV	= NTV2_XptFrameBuffer8_DS2YUV		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer8_DS2YUV
		,NTV2_XptFrameBuffer8_425RGB	= NTV2_XptFrameBuffer8_DS2RGB		///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer8_DS2RGB
	#endif
	#if !defined(NTV2_DEPRECATE_14_3)
		,NTV2_XptHDMIIn					= NTV2_XptHDMIIn1					///< @deprecated	Renamed in SDK 14.3 as ::NTV2_XptHDMIIn1
		,NTV2_XptHDMIInQ2				= NTV2_XptHDMIIn1Q2					///< @deprecated	Renamed in SDK 14.3 as ::NTV2_XptHDMIIn1Q2
		,NTV2_XptHDMIInQ3				= NTV2_XptHDMIIn1Q3					///< @deprecated	Renamed in SDK 14.3 as ::NTV2_XptHDMIIn1Q3
		,NTV2_XptHDMIInQ4				= NTV2_XptHDMIIn1Q4					///< @deprecated	Renamed in SDK 14.3 as ::NTV2_XptHDMIIn1Q4
		,NTV2_XptHDMIInRGB				= NTV2_XptHDMIIn1RGB				///< @deprecated	Renamed in SDK 14.3 as ::NTV2_XptHDMIIn1RGB
		,NTV2_XptHDMIInQ2RGB			= NTV2_XptHDMIIn1Q2RGB				///< @deprecated	Renamed in SDK 14.3 as ::NTV2_XptHDMIIn1Q2RGB
		,NTV2_XptHDMIInQ3RGB			= NTV2_XptHDMIIn1Q3RGB				///< @deprecated	Renamed in SDK 14.3 as ::NTV2_XptHDMIIn1Q3RGB
		,NTV2_XptHDMIInQ4RGB			= NTV2_XptHDMIIn1Q4RGB				///< @deprecated	Renamed in SDK 14.3 as ::NTV2_XptHDMIIn1Q4RGB
	#endif
	#if !defined (NTV2_DEPRECATE)
		,NTV2_XptFS1SecondConverter		= 0x18						///< @deprecated	Obsolete, do not use.
		,NTV2_XptFS1ProcAmp				= 0x19						///< @deprecated	Obsolete, do not use.
		,NTV2_XptFS1TestSignalGenerator	= 0x1D						///< @deprecated	Obsolete, do not use.
		,NTV2_XptCSCYUV					= NTV2_XptCSC1VidYUV		///< @deprecated	Use NTV2_XptCSC1VidYUV instead.
		,NTV2_XptLUT					= NTV2_XptLUT1Out			///< @deprecated	Use NTV2_XptLUT1Out instead.
		,NTV2_XptCSCRGB					= NTV2_XptCSC1VidRGB		///< @deprecated	Use NTV2_XptCSC1VidRGB instead.
		,NTV2_XptDuallinkIn				= NTV2_XptDuallinkIn1		///< @deprecated	Use NTV2_XptDuallinkIn1 instead.
		,NTV2_XptDuallinkOut			= NTV2_XptDuallinkOut1		///< @deprecated	Use NTV2_XptDuallinkOut1 instead.
		,NTV2_XptDuallinkOutDS2			= NTV2_XptDuallinkOut1DS2	///< @deprecated	Use NTV2_XptDuallinkOut1DS2 instead.
	#endif	//	!defined (NTV2_DEPRECATE)
		,NTV2_XptDuallinkIn1DS2	= 0x100
		,NTV2_XptDuallinkIn2DS2	= 0x101
		,NTV2_XptDuallinkIn3DS2	= 0x102
		,NTV2_XptDuallinkIn4DS2	= 0x103
		,NTV2_XptDuallinkIn5DS2	= 0x104
		,NTV2_XptDuallinkIn6DS2	= 0x105
		,NTV2_XptDuallinkIn7DS2	= 0x106
		,NTV2_XptDuallinkIn8DS2	= 0x107
} NTV2OutputCrosspointID, NTV2OutputXptID;

#if !defined(NTV2_DEPRECATE_16_0)
	typedef NTV2OutputXptID		NTV2CrosspointID;		///< @deprecated	In SDK 16.0, use ::NTV2OutputXptID instead.
#endif	//	!defined(NTV2_DEPRECATE_16_0)

#define	NTV2_IS_VALID_OutputCrosspointID(__s__)			((__s__) >= NTV2_XptBlack && (__s__) < NTV2_OUTPUT_CROSSPOINT_INVALID)
#define	NTV2_IS_RGB_OutputCrosspointID(__s__)			(((unsigned char)(__s__)) & 0x80)

/**
	@brief	Identifies a widget input that potentially can accept a signal emitted
			from another widget's output (identified by ::NTV2OutputCrosspointID).
	@see	CNTV2Card::Connect and also \ref ntv2signalrouting
**/
typedef enum NTV2InputCrosspointID
{
	NTV2_FIRST_INPUT_CROSSPOINT		= 0x01,
	NTV2_XptFrameBuffer1Input		= 0x01,
	NTV2_XptFrameBuffer1DS2Input	= 0x02,
	NTV2_XptFrameBuffer2Input		= 0x03,
	NTV2_XptFrameBuffer2DS2Input	= 0x04,
	NTV2_XptFrameBuffer3Input		= 0x05,
	NTV2_XptFrameBuffer3DS2Input	= 0x06,
	NTV2_XptFrameBuffer4Input		= 0x07,
	NTV2_XptFrameBuffer4DS2Input	= 0x08,
	NTV2_XptFrameBuffer5Input		= 0x09,
	NTV2_XptFrameBuffer5DS2Input	= 0x0A,
	NTV2_XptFrameBuffer6Input		= 0x0B,
	NTV2_XptFrameBuffer6DS2Input	= 0x0C,
	NTV2_XptFrameBuffer7Input		= 0x0D,
	NTV2_XptFrameBuffer7DS2Input	= 0x0E,
	NTV2_XptFrameBuffer8Input		= 0x0F,
	NTV2_XptFrameBuffer8DS2Input	= 0x10,
	NTV2_XptCSC1VidInput			= 0x11,
	NTV2_XptCSC1KeyInput			= 0x12,
	NTV2_XptCSC2VidInput			= 0x13,
	NTV2_XptCSC2KeyInput			= 0x14,
	NTV2_XptCSC3VidInput			= 0x15,
	NTV2_XptCSC3KeyInput			= 0x16,
	NTV2_XptCSC4VidInput			= 0x17,
	NTV2_XptCSC4KeyInput			= 0x18,
	NTV2_XptCSC5VidInput			= 0x19,
	NTV2_XptCSC5KeyInput			= 0x1A,
	NTV2_XptCSC6VidInput			= 0x1B,
	NTV2_XptCSC6KeyInput			= 0x1C,
	NTV2_XptCSC7VidInput			= 0x1D,
	NTV2_XptCSC7KeyInput			= 0x1E,
	NTV2_XptCSC8VidInput			= 0x1F,
	NTV2_XptCSC8KeyInput			= 0x20,
	NTV2_XptLUT1Input				= 0x21,
	NTV2_XptLUT2Input				= 0x22,
	NTV2_XptLUT3Input				= 0x23,
	NTV2_XptLUT4Input				= 0x24,
	NTV2_XptLUT5Input				= 0x25,
	NTV2_XptLUT6Input				= 0x26,
	NTV2_XptLUT7Input				= 0x27,
	NTV2_XptLUT8Input				= 0x28,
	NTV2_XptMultiLinkOut1Input		= 0x29,	///< @brief	New in SDK 16.0
	NTV2_XptMultiLinkOut1InputDS2	= 0x2A,	///< @brief	New in SDK 16.0
	NTV2_XptMultiLinkOut2Input		= 0x2B,	///< @brief	New in SDK 16.0
	NTV2_XptMultiLinkOut2InputDS2	= 0x2C,	///< @brief	New in SDK 16.0
	NTV2_XptSDIOut1Input			= 0x2D,
	NTV2_XptSDIOut1InputDS2			= 0x2E,
	NTV2_XptSDIOut2Input			= 0x2F,
	NTV2_XptSDIOut2InputDS2			= 0x30,
	NTV2_XptSDIOut3Input			= 0x31,
	NTV2_XptSDIOut3InputDS2			= 0x32,
	NTV2_XptSDIOut4Input			= 0x33,
	NTV2_XptSDIOut4InputDS2			= 0x34,
	NTV2_XptSDIOut5Input			= 0x35,
	NTV2_XptSDIOut5InputDS2			= 0x36,
	NTV2_XptSDIOut6Input			= 0x37,
	NTV2_XptSDIOut6InputDS2			= 0x38,
	NTV2_XptSDIOut7Input			= 0x39,
	NTV2_XptSDIOut7InputDS2			= 0x3A,
	NTV2_XptSDIOut8Input			= 0x3B,
	NTV2_XptSDIOut8InputDS2			= 0x3C,
	NTV2_XptDualLinkIn1Input		= 0x3D,
	NTV2_XptDualLinkIn1DSInput		= 0x3E,
	NTV2_XptDualLinkIn2Input		= 0x3F,
	NTV2_XptDualLinkIn2DSInput		= 0x40,
	NTV2_XptDualLinkIn3Input		= 0x41,
	NTV2_XptDualLinkIn3DSInput		= 0x42,
	NTV2_XptDualLinkIn4Input		= 0x43,
	NTV2_XptDualLinkIn4DSInput		= 0x44,
	NTV2_XptDualLinkIn5Input		= 0x45,
	NTV2_XptDualLinkIn5DSInput		= 0x46,
	NTV2_XptDualLinkIn6Input		= 0x47,
	NTV2_XptDualLinkIn6DSInput		= 0x48,
	NTV2_XptDualLinkIn7Input		= 0x49,
	NTV2_XptDualLinkIn7DSInput		= 0x4A,
	NTV2_XptDualLinkIn8Input		= 0x4B,
	NTV2_XptDualLinkIn8DSInput		= 0x4C,
	NTV2_XptDualLinkOut1Input		= 0x4D,
	NTV2_XptDualLinkOut2Input		= 0x4E,
	NTV2_XptDualLinkOut3Input		= 0x4F,
	NTV2_XptDualLinkOut4Input		= 0x50,
	NTV2_XptDualLinkOut5Input		= 0x51,
	NTV2_XptDualLinkOut6Input		= 0x52,
	NTV2_XptDualLinkOut7Input		= 0x53,
	NTV2_XptDualLinkOut8Input		= 0x54,
	NTV2_XptMixer1BGKeyInput		= 0x55,
	NTV2_XptMixer1BGVidInput		= 0x56,
	NTV2_XptMixer1FGKeyInput		= 0x57,
	NTV2_XptMixer1FGVidInput		= 0x58,
	NTV2_XptMixer2BGKeyInput		= 0x59,
	NTV2_XptMixer2BGVidInput		= 0x5A,
	NTV2_XptMixer2FGKeyInput		= 0x5B,
	NTV2_XptMixer2FGVidInput		= 0x5C,
	NTV2_XptMixer3BGKeyInput		= 0x5D,
	NTV2_XptMixer3BGVidInput		= 0x5E,
	NTV2_XptMixer3FGKeyInput		= 0x5F,
	NTV2_XptMixer3FGVidInput		= 0x60,
	NTV2_XptMixer4BGKeyInput		= 0x61,
	NTV2_XptMixer4BGVidInput		= 0x62,
	NTV2_XptMixer4FGKeyInput		= 0x63,
	NTV2_XptMixer4FGVidInput		= 0x64,
	NTV2_XptHDMIOutInput			= 0x65,
	NTV2_XptHDMIOutQ1Input			= NTV2_XptHDMIOutInput,
	NTV2_XptHDMIOutQ2Input			= 0x66,
	NTV2_XptHDMIOutQ3Input			= 0x67,
	NTV2_XptHDMIOutQ4Input			= 0x68,
	NTV2_Xpt4KDCQ1Input				= 0x69,
	NTV2_Xpt4KDCQ2Input				= 0x6A,
	NTV2_Xpt4KDCQ3Input				= 0x6B,
	NTV2_Xpt4KDCQ4Input				= 0x6C,
	NTV2_Xpt425Mux1AInput			= 0x6D,
	NTV2_Xpt425Mux1BInput			= 0x6E,
	NTV2_Xpt425Mux2AInput			= 0x6F,
	NTV2_Xpt425Mux2BInput			= 0x70,
	NTV2_Xpt425Mux3AInput			= 0x71,
	NTV2_Xpt425Mux3BInput			= 0x72,
	NTV2_Xpt425Mux4AInput			= 0x73,
	NTV2_Xpt425Mux4BInput			= 0x74,
	NTV2_XptAnalogOutInput			= 0x75,
	NTV2_Xpt3DLUT1Input				= 0x76,	///< @brief	New in SDK 16.0
	NTV2_XptAnalogOutCompositeOut	= 0x77,	//	deprecate?
	NTV2_XptStereoLeftInput			= 0x78,	//	deprecate?
	NTV2_XptStereoRightInput		= 0x79,	//	deprecate?
	NTV2_XptProAmpInput				= 0x7A,	//	deprecate?
	NTV2_XptIICT1Input				= 0x7B,	//	deprecate?
	NTV2_XptWaterMarker1Input		= 0x7C,	//	deprecate?
	NTV2_XptWaterMarker2Input		= 0x7D,	//	deprecate?
	NTV2_XptUpdateRegister			= 0x7E,	//	deprecate?
	NTV2_XptOEInput					= 0x7F,
	NTV2_XptCompressionModInput		= 0x80,	//	deprecate?
	NTV2_XptConversionModInput		= 0x81,	//	deprecate?
	NTV2_XptCSC1KeyFromInput2		= 0x82,	//	deprecate?
	NTV2_XptFrameSync2Input			= 0x83,	//	deprecate?
	NTV2_XptFrameSync1Input			= 0x84,	//	deprecate?
	NTV2_LAST_INPUT_CROSSPOINT		= 0x84,
	NTV2_INPUT_CROSSPOINT_INVALID	= 0xFFFFFFFF
	#if !defined(NTV2_DEPRECATE_16_0)
		,NTV2_XptConversionMod2Input = NTV2_XptOEInput				///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptOEInput
		,NTV2_XptSDIOut1Standard	= NTV2_XptMultiLinkOut1Input	///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptMultiLinkOut1Input
		,NTV2_XptSDIOut2Standard	= NTV2_XptMultiLinkOut1InputDS2	///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptMultiLinkOut1InputDS2
		,NTV2_XptSDIOut3Standard	= NTV2_XptMultiLinkOut2Input	///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptMultiLinkOut2Input
		,NTV2_XptSDIOut4Standard	= NTV2_XptMultiLinkOut2InputDS2	///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_XptMultiLinkOut2InputDS2
		,NTV2_XptIICT2Input			= NTV2_Xpt3DLUT1Input			///< @deprecated	Removed in SDK 16.0, redeployed as ::NTV2_Xpt3DLUT1Input
	#endif
	#if !defined(NTV2_DEPRECATE_15_3)
		,NTV2_XptFrameBuffer1BInput = NTV2_XptFrameBuffer1DS2Input	///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer1DS2Input
		,NTV2_XptFrameBuffer2BInput = NTV2_XptFrameBuffer2DS2Input	///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer2DS2Input
		,NTV2_XptFrameBuffer3BInput = NTV2_XptFrameBuffer3DS2Input	///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer3DS2Input
		,NTV2_XptFrameBuffer4BInput = NTV2_XptFrameBuffer4DS2Input	///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer4DS2Input
		,NTV2_XptFrameBuffer5BInput = NTV2_XptFrameBuffer5DS2Input	///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer5DS2Input
		,NTV2_XptFrameBuffer6BInput = NTV2_XptFrameBuffer6DS2Input	///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer6DS2Input
		,NTV2_XptFrameBuffer7BInput = NTV2_XptFrameBuffer7DS2Input	///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer7DS2Input
		,NTV2_XptFrameBuffer8BInput = NTV2_XptFrameBuffer8DS2Input	///< @deprecated	Renamed in SDK 15.3 as ::NTV2_XptFrameBuffer8DS2Input
	#endif
} NTV2InputCrosspointID, NTV2InputXptID;

#define	NTV2_IS_VALID_InputCrosspointID(__s__)			((__s__) >= NTV2_FIRST_INPUT_CROSSPOINT && (__s__) <= NTV2_LAST_INPUT_CROSSPOINT)


/**
	@brief	Identifies firmware widgets that logically can have zero or more signal inputs
			(identified by ::NTV2InputCrosspointID) and/or zero or more signal outputs
			(identified by ::NTV2OutputCrosspointID).
**/
#define NTV2_WIDGET_FIRST		0
typedef enum
{
	 NTV2_WgtFrameBuffer1	= NTV2_WIDGET_FIRST
	,NTV2_WgtFrameBuffer2
	,NTV2_WgtFrameBuffer3
	,NTV2_WgtFrameBuffer4
	,NTV2_WgtCSC1
	,NTV2_WgtCSC2
	,NTV2_WgtLUT1
	,NTV2_WgtLUT2
	,NTV2_WgtFrameSync1
	,NTV2_WgtFrameSync2
	,NTV2_WgtSDIIn1
	,NTV2_WgtSDIIn2
	,NTV2_Wgt3GSDIIn1
	,NTV2_Wgt3GSDIIn2
	,NTV2_Wgt3GSDIIn3
	,NTV2_Wgt3GSDIIn4
	,NTV2_WgtSDIOut1
	,NTV2_WgtSDIOut2
	,NTV2_WgtSDIOut3				//	UNUSED
	,NTV2_WgtSDIOut4				//	UNUSED
	,NTV2_Wgt3GSDIOut1
	,NTV2_Wgt3GSDIOut2
	,NTV2_Wgt3GSDIOut3
	,NTV2_Wgt3GSDIOut4
	,NTV2_WgtDualLinkIn1			//	UNUSED
	,NTV2_WgtDualLinkV2In1
	,NTV2_WgtDualLinkV2In2
	,NTV2_WgtDualLinkOut1			//	UNUSED
	,NTV2_WgtDualLinkOut2			//	UNUSED
	,NTV2_WgtDualLinkV2Out1
	,NTV2_WgtDualLinkV2Out2
	,NTV2_WgtAnalogIn1
	,NTV2_WgtAnalogOut1
	,NTV2_WgtAnalogCompositeOut1	//	UNUSED
	,NTV2_WgtHDMIIn1
	,NTV2_WgtHDMIOut1
	,NTV2_WgtUpDownConverter1
	,NTV2_WgtUpDownConverter2		//	UNUSED
	,NTV2_WgtMixer1
	,NTV2_WgtCompression1
	,NTV2_WgtProcAmp1				//	UNUSED
	,NTV2_WgtWaterMarker1
	,NTV2_WgtWaterMarker2
	,NTV2_WgtIICT1					//	UNUSED
	,NTV2_WgtIICT2					//	UNUSED
	,NTV2_WgtTestPattern1			//	UNUSED
	,NTV2_WgtGenLock
	,NTV2_WgtDCIMixer1				//	UNUSED
	,NTV2_WgtMixer2
	,NTV2_WgtStereoCompressor		//	UNUSED
	,NTV2_WgtLUT3
	,NTV2_WgtLUT4
	,NTV2_WgtDualLinkV2In3
	,NTV2_WgtDualLinkV2In4
	,NTV2_WgtDualLinkV2Out3
	,NTV2_WgtDualLinkV2Out4
	,NTV2_WgtCSC3
	,NTV2_WgtCSC4
	,NTV2_WgtHDMIIn1v2
	,NTV2_WgtHDMIOut1v2
	,NTV2_WgtSDIMonOut1
	,NTV2_WgtCSC5
	,NTV2_WgtLUT5
	,NTV2_WgtDualLinkV2Out5
	,NTV2_Wgt4KDownConverter
	,NTV2_Wgt3GSDIIn5
	,NTV2_Wgt3GSDIIn6
	,NTV2_Wgt3GSDIIn7
	,NTV2_Wgt3GSDIIn8
	,NTV2_Wgt3GSDIOut5
	,NTV2_Wgt3GSDIOut6
	,NTV2_Wgt3GSDIOut7
	,NTV2_Wgt3GSDIOut8
	,NTV2_WgtDualLinkV2In5
	,NTV2_WgtDualLinkV2In6
	,NTV2_WgtDualLinkV2In7
	,NTV2_WgtDualLinkV2In8
	,NTV2_WgtDualLinkV2Out6
	,NTV2_WgtDualLinkV2Out7
	,NTV2_WgtDualLinkV2Out8
	,NTV2_WgtCSC6
	,NTV2_WgtCSC7
	,NTV2_WgtCSC8
	,NTV2_WgtLUT6
	,NTV2_WgtLUT7
	,NTV2_WgtLUT8
	,NTV2_WgtMixer3
	,NTV2_WgtMixer4
	,NTV2_WgtFrameBuffer5
	,NTV2_WgtFrameBuffer6
	,NTV2_WgtFrameBuffer7
	,NTV2_WgtFrameBuffer8
	,NTV2_WgtHDMIIn1v3
	,NTV2_WgtHDMIOut1v3
	,NTV2_Wgt425Mux1
	,NTV2_Wgt425Mux2
	,NTV2_Wgt425Mux3
	,NTV2_Wgt425Mux4
	,NTV2_Wgt12GSDIIn1
	,NTV2_Wgt12GSDIIn2
	,NTV2_Wgt12GSDIIn3
	,NTV2_Wgt12GSDIIn4
	,NTV2_Wgt12GSDIOut1
	,NTV2_Wgt12GSDIOut2
	,NTV2_Wgt12GSDIOut3
	,NTV2_Wgt12GSDIOut4
	,NTV2_WgtHDMIIn1v4
	,NTV2_WgtHDMIIn2v4
	,NTV2_WgtHDMIIn3v4
	,NTV2_WgtHDMIIn4v4
	,NTV2_WgtHDMIOut1v4
	,NTV2_WgtHDMIOut1v5
	,NTV2_WgtMultiLinkOut1
	,NTV2_Wgt3DLUT1
	,NTV2_WgtMultiLinkOut2
	,NTV2_WgtOE1
	,NTV2_WgtModuleTypeCount
	,NTV2_WgtUndefined = NTV2_WgtModuleTypeCount
	,NTV2_WIDGET_INVALID = NTV2_WgtModuleTypeCount
} NTV2WidgetID;

#define	NTV2_IS_VALID_WIDGET(__w__)		(((__w__) >= NTV2_WIDGET_FIRST)  &&  ((__w__) < NTV2_WIDGET_INVALID))

typedef enum {
	NTV2WidgetType_First = 0
	,NTV2WidgetType_FrameStore = NTV2WidgetType_First
	,NTV2WidgetType_CSC
	,NTV2WidgetType_LUT
	,NTV2WidgetType_FrameSync
	,NTV2WidgetType_SDIIn
	,NTV2WidgetType_SDIIn3G
	,NTV2WidgetType_SDIOut
	,NTV2WidgetType_SDIOut3G
	,NTV2WidgetType_SDIMonOut
	,NTV2WidgetType_DualLinkV1In
	,NTV2WidgetType_DualLinkV2In
	,NTV2WidgetType_DualLinkV1Out
	,NTV2WidgetType_DualLinkV2Out
	,NTV2WidgetType_AnalogIn
	,NTV2WidgetType_AnalogOut
	,NTV2WidgetType_AnalogCompositeOut
	,NTV2WidgetType_HDMIInV1
	,NTV2WidgetType_HDMIInV2
	,NTV2WidgetType_HDMIInV3
	,NTV2WidgetType_HDMIInV4
	,NTV2WidgetType_UpDownConverter
	,NTV2WidgetType_Mixer
	,NTV2WidgetType_DCIMixer
	,NTV2WidgetType_Compression
	,NTV2WidgetType_StereoCompressor
	,NTV2WidgetType_ProcAmp
	,NTV2WidgetType_GenLock
	,NTV2WidgetType_4KDownConverter
	,NTV2WidgetType_HDMIOutV1
	,NTV2WidgetType_HDMIOutV2
	,NTV2WidgetType_HDMIOutV3
	,NTV2WidgetType_HDMIOutV4
	,NTV2WidgetType_HDMIOutV5
	,NTV2WidgetType_SMPTE425Mux
	,NTV2WidgetType_SDIIn12G
	,NTV2WidgetType_SDIOut12G
	,NTV2WidgetType_MultiLinkOut
	,NTV2WidgetType_LUT3D
	,NTV2WidgetType_OE
	,NTV2WidgetType_WaterMarker
	,NTV2WidgetType_IICT
	,NTV2WidgetType_TestPattern
	,NTV2WidgetType_Max
	,NTV2WidgetType_Invalid = NTV2WidgetType_Max
} NTV2WidgetType;

#define	NTV2_IS_VALID_WIDGET_TYPE(__w__)		(((__w__) >= NTV2WidgetType_First)  &&  ((__w__) < NTV2WidgetType_Invalid))

#if !defined (NTV2_DEPRECATE)
    typedef enum
    {
		NTV2_FrameSync1Select,
		NTV2_FrameSync2Select,
		NTV2K2_FrameSync1Select		= NTV2_FrameSync1Select,
		NTV2K2_FrameSync2Select		= NTV2_FrameSync2Select,
		NTV2_MAX_NUM_FrameSyncSelect
    } NTV2FrameSyncSelect;		//	FS1 ONLY
#endif	//	!defined (NTV2_DEPRECATE)


/**
    @brief	Identifies the \ref ntv2hwaccessories that may be attached to an AJA NTV2 device.
**/
typedef enum
{
	NTV2_BreakoutNone,			///< @brief	No identifiable breakout hardware appears to be attached.
	NTV2_BreakoutCableXLR,		///< @brief	Identifies the AES/EBU audio breakout cable that has XLR connectors
	NTV2_BreakoutCableBNC,		///< @brief	Identifies the AES/EBU audio breakout cable that has BNC connectors
	NTV2_KBox,					// Kona2
	NTV2_KLBox,					// KonaLS
	NTV2_K3Box,					// Kona3
	NTV2_KLHiBox,				// Kona LHI
	NTV2_KLHePlusBox,			// Kona LHe+
	NTV2_K3GBox,				// Kona3G
	NTV2_MAX_NUM_BreakoutTypes,
	NTV2_BreakoutType_Invalid	= NTV2_BreakoutNone
} NTV2BreakoutType;

#define	NTV2_IS_VALID_BREAKOUT_TYPE(__p__)		((__p__) > NTV2_BreakoutNone && (__p__) < NTV2_MAX_NUM_BreakoutTypes)


#define	EXTENDED_AUDIO_SUPPORTED

/**
    @brief	Identifies a pair of audio channels.
    @note	The audio channels in the pair are adjacent, and never span an audio group.
    @see	CNTV2Card::GetAudioPCMControl(const NTV2AudioSystem, const NTV2AudioChannelPair),
			CNTV2Card::SetAudioPCMControl(const NTV2AudioSystem, const NTV2AudioChannelPair),
			::NTV2DeviceGetMaxAudioChannels, \ref audiooperation
**/
typedef enum
{
	NTV2_AudioChannel1_2,		///< @brief	This selects audio channels 1 and 2		(Group 1 channels 1 and 2)
	NTV2_AudioChannel3_4,		///< @brief	This selects audio channels 3 and 4		(Group 1 channels 3 and 4)
	NTV2_AudioChannel5_6,		///< @brief	This selects audio channels 5 and 6		(Group 2 channels 1 and 2)
	NTV2_AudioChannel7_8,		///< @brief	This selects audio channels 7 and 8		(Group 2 channels 3 and 4)
	NTV2_AudioChannel9_10,		///< @brief	This selects audio channels 9 and 10	(Group 3 channels 1 and 2)
	NTV2_AudioChannel11_12,		///< @brief	This selects audio channels 11 and 12	(Group 3 channels 3 and 4)
	NTV2_AudioChannel13_14,		///< @brief	This selects audio channels 13 and 14	(Group 4 channels 1 and 2)
	NTV2_AudioChannel15_16,		///< @brief	This selects audio channels 15 and 16	(Group 4 channels 3 and 4)
	NTV2_AudioChannel17_18,		///< @brief	This selects audio channels 17 and 18
	NTV2_AudioChannel19_20,		///< @brief	This selects audio channels 19 and 20
	NTV2_AudioChannel21_22,		///< @brief	This selects audio channels 21 and 22
	NTV2_AudioChannel23_24,		///< @brief	This selects audio channels 23 and 24
	NTV2_AudioChannel25_26,		///< @brief	This selects audio channels 25 and 26
	NTV2_AudioChannel27_28,		///< @brief	This selects audio channels 27 and 28
	NTV2_AudioChannel29_30,		///< @brief	This selects audio channels 29 and 30
	NTV2_AudioChannel31_32,		///< @brief	This selects audio channels 31 and 32
	NTV2_AudioChannel33_34,		///< @brief	This selects audio channels 33 and 34
	NTV2_AudioChannel35_36,		///< @brief	This selects audio channels 35 and 36
	NTV2_AudioChannel37_38,		///< @brief	This selects audio channels 37 and 38
	NTV2_AudioChannel39_40,		///< @brief	This selects audio channels 39 and 40
	NTV2_AudioChannel41_42,		///< @brief	This selects audio channels 41 and 42
	NTV2_AudioChannel43_44,		///< @brief	This selects audio channels 43 and 44
	NTV2_AudioChannel45_46,		///< @brief	This selects audio channels 45 and 46
	NTV2_AudioChannel47_48,		///< @brief	This selects audio channels 47 and 48
	NTV2_AudioChannel49_50,		///< @brief	This selects audio channels 49 and 50
	NTV2_AudioChannel51_52,		///< @brief	This selects audio channels 51 and 52
	NTV2_AudioChannel53_54,		///< @brief	This selects audio channels 53 and 54
	NTV2_AudioChannel55_56,		///< @brief	This selects audio channels 55 and 56
	NTV2_AudioChannel57_58,		///< @brief	This selects audio channels 57 and 58
	NTV2_AudioChannel59_60,		///< @brief	This selects audio channels 59 and 60
	NTV2_AudioChannel61_62,		///< @brief	This selects audio channels 61 and 62
	NTV2_AudioChannel63_64,		///< @brief	This selects audio channels 63 and 64
	NTV2_AudioChannel65_66,		///< @brief	This selects audio channels 65 and 66
	NTV2_AudioChannel67_68,		///< @brief	This selects audio channels 67 and 68
	NTV2_AudioChannel69_70,		///< @brief	This selects audio channels 69 and 70
	NTV2_AudioChannel71_72,		///< @brief	This selects audio channels 71 and 72
	NTV2_AudioChannel73_74,		///< @brief	This selects audio channels 73 and 74
	NTV2_AudioChannel75_76,		///< @brief	This selects audio channels 75 and 76
	NTV2_AudioChannel77_78,		///< @brief	This selects audio channels 77 and 78
	NTV2_AudioChannel79_80,		///< @brief	This selects audio channels 79 and 80
	NTV2_AudioChannel81_82,		///< @brief	This selects audio channels 81 and 82
	NTV2_AudioChannel83_84,		///< @brief	This selects audio channels 83 and 84
	NTV2_AudioChannel85_86,		///< @brief	This selects audio channels 85 and 86
	NTV2_AudioChannel87_88,		///< @brief	This selects audio channels 87 and 88
	NTV2_AudioChannel89_90,		///< @brief	This selects audio channels 89 and 90
	NTV2_AudioChannel91_92,		///< @brief	This selects audio channels 91 and 92
	NTV2_AudioChannel93_94,		///< @brief	This selects audio channels 93 and 94
	NTV2_AudioChannel95_96,		///< @brief	This selects audio channels 95 and 96
	NTV2_AudioChannel97_98,		///< @brief	This selects audio channels 97 and 98
	NTV2_AudioChannel99_100,	///< @brief	This selects audio channels 99 and 100
	NTV2_AudioChannel101_102,	///< @brief	This selects audio channels 101 and 102
	NTV2_AudioChannel103_104,	///< @brief	This selects audio channels 103 and 104
	NTV2_AudioChannel105_106,	///< @brief	This selects audio channels 105 and 106
	NTV2_AudioChannel107_108,	///< @brief	This selects audio channels 107 and 108
	NTV2_AudioChannel109_110,	///< @brief	This selects audio channels 109 and 110
	NTV2_AudioChannel111_112,	///< @brief	This selects audio channels 111 and 112
	NTV2_AudioChannel113_114,	///< @brief	This selects audio channels 113 and 114
	NTV2_AudioChannel115_116,	///< @brief	This selects audio channels 115 and 116
	NTV2_AudioChannel117_118,	///< @brief	This selects audio channels 117 and 118
	NTV2_AudioChannel119_120,	///< @brief	This selects audio channels 119 and 120
	NTV2_AudioChannel121_122,	///< @brief	This selects audio channels 121 and 122
	NTV2_AudioChannel123_124,	///< @brief	This selects audio channels 123 and 124
	NTV2_AudioChannel125_126,	///< @brief	This selects audio channels 125 and 126
	NTV2_AudioChannel127_128,	///< @brief	This selects audio channels 127 and 128
	NTV2_MAX_NUM_AudioChannelPair,
	NTV2_AUDIO_CHANNEL_PAIR_INVALID	= NTV2_MAX_NUM_AudioChannelPair
} NTV2AudioChannelPair;

typedef NTV2AudioChannelPair	NTV2Audio2ChannelSelect;

#define	NTV2_IS_VALID_AUDIO_CHANNEL_PAIR(__p__)				((__p__) >= NTV2_AudioChannel1_2	&& (__p__) < NTV2_MAX_NUM_AudioChannelPair)
#define	NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(__p__)		((__p__) >= NTV2_AudioChannel1_2	&& (__p__) <= NTV2_AudioChannel15_16)
#define	NTV2_IS_NORMAL_AUDIO_CHANNEL_PAIR(__p__)			((__p__) >= NTV2_AudioChannel1_2	&& (__p__) <= NTV2_AudioChannel15_16)
#define	NTV2_IS_EXTENDED_AUDIO_CHANNEL_PAIR(__p__)			((__p__) >= NTV2_AudioChannel17_18	&& (__p__) < NTV2_MAX_NUM_AudioChannelPair)

#if !defined(NTV2_DEPRECATE_16_1)
	#define	NTV2_AudioMonitor1_2				NTV2_AudioChannel1_2		// Analog Audio Monitor Channels 1-2
	#define	NTV2_AudioMonitor3_4				NTV2_AudioChannel3_4		// Analog Audio Monitor Channels 3-4
	#define	NTV2_AudioMonitor5_6				NTV2_AudioChannel5_6		// Analog Audio Monitor Channels 5-6
	#define	NTV2_AudioMonitor7_8				NTV2_AudioChannel7_8		// Analog Audio Monitor Channels 7-8
	#define	NTV2_AudioMonitor9_10				NTV2_AudioChannel9_10		// Analog Audio Monitor Channels 9-10
	#define	NTV2_AudioMonitor11_12				NTV2_AudioChannel11_12		// Analog Audio Monitor Channels 11-12
	#define	NTV2_AudioMonitor13_14				NTV2_AudioChannel13_14		// Analog Audio Monitor Channels 13-14
	#define	NTV2_AudioMonitor15_16				NTV2_AudioChannel15_16		// Analog Audio Monitor Channels 15-16
	#define	NTV2_MAX_NUM_AudioMonitorSelect		NTV2_AudioChannel17_18
	#define	NTV2_AUDIO_MONITOR_INVALID			NTV2_MAX_NUM_AudioMonitorSelect
	typedef NTV2AudioChannelPair	NTV2AudioMonitorSelect;

	#define	NTV2_IS_VALID_AUDIO_MONITOR(__p__)		((__p__) < NTV2_MAX_NUM_AudioMonitorSelect)
#endif	//	!defined(NTV2_DEPRECATE_16_1)


typedef enum
{
	NTV2_AudioMixerChannel1,
	NTV2_AudioMixerChannel2,
	NTV2_AudioMixerChannel3,
	NTV2_AudioMixerChannel4,
	NTV2_AudioMixerChannel5,
	NTV2_AudioMixerChannel6,
	NTV2_AudioMixerChannel7,
	NTV2_AudioMixerChannel8,
	NTV2_AudioMixerChannel9,
	NTV2_AudioMixerChannel10,
	NTV2_AudioMixerChannel11,
	NTV2_AudioMixerChannel12,
	NTV2_AudioMixerChannel13,
	NTV2_AudioMixerChannel14,
	NTV2_AudioMixerChannel15,
	NTV2_AudioMixerChannel16,
	NTV2_MAX_NUM_AudioMixerChannel,
	NTV2_AUDIO_MIXER_CHANNEL_INVALID	=	NTV2_MAX_NUM_AudioMixerChannel
} NTV2AudioMixerChannel;

#define	NTV2_IS_VALID_AUDIO_MIXER_CHANNEL(__p__)		((__p__) >= NTV2_AudioMixerChannel1	&& (__p__) < NTV2_MAX_NUM_AudioMixerChannel)
#define	NTV2_IS_AUDIO_MIXER_CHANNELS_1_OR_2(__p__)		((__p__) >= NTV2_AudioMixerChannel1	&& (__p__) <= NTV2_AudioMixerChannel2)

/**
    @brief	Identifies the Audio Mixer's audio inputs.
    @see	See \ref audiomixer
**/
typedef enum
{
	NTV2_AudioMixerInputMain,	///< @brief	This selects the Audio Mixer's Main (primary) input
	NTV2_AudioMixerInputAux1,	///< @brief	This selects the Audio Mixer's 1st Auxiliary input
	NTV2_AudioMixerInputAux2,	///< @brief	This selects the Audio Mixer's 2nd Auxiliary input
	NTV2_MAX_NUM_AudioMixerInput,
	NTV2_AUDIO_MIXER_INPUT_INVALID	=	NTV2_MAX_NUM_AudioMixerInput
} NTV2AudioMixerInput;

#define	NTV2_IS_VALID_AUDIO_MIXER_INPUT(__p__)		((__p__) >= NTV2_AudioMixerInputMain	&& (__p__) < NTV2_AUDIO_MIXER_INPUT_INVALID)
#define	NTV2_IS_AUDIO_MIXER_INPUT_MAIN(__p__)		((__p__) == NTV2_AudioMixerInputMain)


/**
    @brief	Identifies a contiguous, adjacent group of four audio channels.
    @see	CNTV2Card::GetAESOutputSource, CNTV2Card::SetAESOutputSource, \ref audiooperation
**/
typedef enum
{
	NTV2_AudioChannel1_4,			///< @brief	This selects audio channels 1 thru 4
	NTV2_AudioChannel5_8,			///< @brief	This selects audio channels 5 thru 8
	NTV2_AudioChannel9_12,			///< @brief	This selects audio channels 9 thru 12
	NTV2_AudioChannel13_16,			///< @brief	This selects audio channels 13 thru 16
	NTV2_AudioChannel17_20,			///< @brief	This selects audio channels 17 thru 20
	NTV2_AudioChannel21_24,			///< @brief	This selects audio channels 21 thru 24
	NTV2_AudioChannel25_28,			///< @brief	This selects audio channels 25 thru 28
	NTV2_AudioChannel29_32,			///< @brief	This selects audio channels 29 thru 32
	NTV2_AudioChannel33_36,			///< @brief	This selects audio channels 33 thru 36
	NTV2_AudioChannel37_40,			///< @brief	This selects audio channels 37 thru 40
	NTV2_AudioChannel41_44,			///< @brief	This selects audio channels 41 thru 44
	NTV2_AudioChannel45_48,			///< @brief	This selects audio channels 45 thru 48
	NTV2_AudioChannel49_52,			///< @brief	This selects audio channels 49 thru 52
	NTV2_AudioChannel53_56,			///< @brief	This selects audio channels 53 thru 56
	NTV2_AudioChannel57_60,			///< @brief	This selects audio channels 57 thru 60
	NTV2_AudioChannel61_64,			///< @brief	This selects audio channels 61 thru 64
	NTV2_AudioChannel65_68,			///< @brief	This selects audio channels 65 thru 68
	NTV2_AudioChannel69_72,			///< @brief	This selects audio channels 69 thru 72
	NTV2_AudioChannel73_76,			///< @brief	This selects audio channels 73 thru 76
	NTV2_AudioChannel77_80,			///< @brief	This selects audio channels 77 thru 80
	NTV2_AudioChannel81_84,			///< @brief	This selects audio channels 81 thru 84
	NTV2_AudioChannel85_88,			///< @brief	This selects audio channels 85 thru 88
	NTV2_AudioChannel89_92,			///< @brief	This selects audio channels 89 thru 92
	NTV2_AudioChannel93_96,			///< @brief	This selects audio channels 93 thru 96
	NTV2_AudioChannel97_100,		///< @brief	This selects audio channels 97 thru 100
	NTV2_AudioChannel101_104,		///< @brief	This selects audio channels 101 thru 104
	NTV2_AudioChannel105_108,		///< @brief	This selects audio channels 105 thru 108
	NTV2_AudioChannel109_112,		///< @brief	This selects audio channels 109 thru 112
	NTV2_AudioChannel113_116,		///< @brief	This selects audio channels 113 thru 116
	NTV2_AudioChannel117_120,		///< @brief	This selects audio channels 117 thru 120
	NTV2_AudioChannel121_124,		///< @brief	This selects audio channels 121 thru 124
	NTV2_AudioChannel125_128,		///< @brief	This selects audio channels 125 thru 128
	NTV2_MAX_NUM_Audio4ChannelSelect,
	NTV2_AUDIO_CHANNEL_QUAD_INVALID	= NTV2_MAX_NUM_Audio4ChannelSelect
} NTV2Audio4ChannelSelect;

typedef NTV2Audio4ChannelSelect	NTV2AudioChannelQuad;

#define	NTV2_IS_VALID_AUDIO_CHANNEL_QUAD(__p__)				((__p__) >= NTV2_AudioChannel1_4	&& (__p__) < NTV2_MAX_NUM_Audio4ChannelSelect)
#define	NTV2_IS_NORMAL_AUDIO_CHANNEL_QUAD(__p__)			((__p__) >= NTV2_AudioChannel1_4	&& (__p__) < NTV2_AudioChannel17_20)
#define	NTV2_IS_EXTENDED_AUDIO_CHANNEL_QUAD(__p__)			((__p__) >= NTV2_AudioChannel17_20	&& (__p__) < NTV2_MAX_NUM_Audio4ChannelSelect)


/**
    @brief	Identifies a contiguous, adjacent group of eight audio channels.
    @see	CNTV2Card::GetHDMIOutAudioSource8Channel, CNTV2Card::SetHDMIOutAudioSource8Channel, \ref audiooperation
**/
typedef enum
{
	NTV2_AudioChannel1_8,			///< @brief	This selects audio channels 1 thru 8
	NTV2_AudioChannel9_16,			///< @brief	This selects audio channels 9 thru 16
	NTV2_AudioChannel17_24,			///< @brief	This selects audio channels 17 thru 24
	NTV2_AudioChannel25_32,			///< @brief	This selects audio channels 25 thru 32
	NTV2_AudioChannel33_40,			///< @brief	This selects audio channels 33 thru 40
	NTV2_AudioChannel41_48,			///< @brief	This selects audio channels 41 thru 48
	NTV2_AudioChannel49_56,			///< @brief	This selects audio channels 49 thru 56
	NTV2_AudioChannel57_64,			///< @brief	This selects audio channels 57 thru 64
	NTV2_AudioChannel65_72,			///< @brief	This selects audio channels 65 thru 72
	NTV2_AudioChannel73_80,			///< @brief	This selects audio channels 73 thru 80
	NTV2_AudioChannel81_88,			///< @brief	This selects audio channels 81 thru 88
	NTV2_AudioChannel89_96,			///< @brief	This selects audio channels 89 thru 96
	NTV2_AudioChannel97_104,		///< @brief	This selects audio channels 97 thru 104
	NTV2_AudioChannel105_112,		///< @brief	This selects audio channels 105 thru 112
	NTV2_AudioChannel113_120,		///< @brief	This selects audio channels 113 thru 120
	NTV2_AudioChannel121_128,		///< @brief	This selects audio channels 121 thru 128
	NTV2_MAX_NUM_Audio8ChannelSelect,
	NTV2_AUDIO_CHANNEL_OCTET_INVALID	= NTV2_MAX_NUM_Audio8ChannelSelect
} NTV2Audio8ChannelSelect;

typedef NTV2Audio8ChannelSelect	NTV2AudioChannelOctet;

#define	NTV2_IS_VALID_AUDIO_CHANNEL_OCTET(__p__)			((__p__) >= NTV2_AudioChannel1_8	&& (__p__) < NTV2_MAX_NUM_Audio8ChannelSelect)
#define	NTV2_IS_NORMAL_AUDIO_CHANNEL_OCTET(__p__)			((__p__) >= NTV2_AudioChannel1_8	&& (__p__) < NTV2_AudioChannel17_24)
#define	NTV2_IS_EXTENDED_AUDIO_CHANNEL_OCTET(__p__)			((__p__) >= NTV2_AudioChannel17_24	&& (__p__) < NTV2_MAX_NUM_Audio8ChannelSelect)


typedef enum
{
	NTV2_VideoProcBitFile,			//	Video processor bit file (loaded in cpu/xilinx)
	NTV2_PCIBitFile,				//	PCI flash bit file (loaded in cpu/xilinx)
	NTV2_FWBitFile,					//	FireWire bit file (loaded in cpu/xilinx)
	NTV2_Firmware,					//	Device firmware (loaded in cpu/xilinx)

	NTV2_BitFile1	= 100,			//	bit file 1 - resident in device memory, loaded or not
	NTV2_BitFile2,					//	bit file 2 - resident in device memory, loaded or not
	NTV2_BitFile3,					//	bit file 3 - resident in device memory, loaded or not
	NTV2_BitFile4,					//	bit file 4 - resident in device memory, loaded or not
	NTV2_BitFile5,					//	bit file 5 - resident in device memory, loaded or not
	NTV2_BitFile6,					//	bit file 6 - resident in device memory, loaded or not
	NTV2_BitFile7,					//	bit file 7 - resident in device memory, loaded or not
	NTV2_BitFile8,					//	bit file 8 - resident in device memory, loaded or not
	NTV2_MAX_NUM_BitFileTypes
} NTV2BitFileType;


typedef enum
{
    NTV2_BITFILE_NO_CHANGE		= 0,		// no bitfile change needed
    NTV2_BITFILE_TYPE_INVALID	= 0,
#if !defined (NTV2_DEPRECATE)
	NTV2_BITFILE_XENAHS_SD		= 1,		// XENA_HS: load SD bitfile								//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENAHS_HD		= 2,		// XENA-HS: load HD bitfile								//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_KONA2_DNCVT	= 3,		// KONA2: load downconverter bitfile					//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_KONA2_UPCVT	= 4,		// KONA2: load upconverter bitfile						//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENALH_SD		= 5,		// XENA_LH: load SD bitfile								//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENALH_HD		= 6,		// XENA-LH: load HD bitfile								//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENALS_UART	= 7,		// XENA-LS: with UART Support							//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENALS_CH2		= 8,		// XENA-LS: with Video Processing and Channel 2 Support	//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENA2_DNCVT	= 9,		// XENA2: load downconverter bitfile					//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENA2_UPCVT	= 10,		// XENA2: load upconverter bitfile						//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENA2_XDCVT	= 11,		// XENA2: load cross downconverter (1080->720) bitfile	//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENA2_XUCVT	= 12,		// XENA2: load cross upconverter (720->1080) bitfile	//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENALH_HDQRZ	= 13,		// XENA-LH - HD Qrez bitfile							//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENALH_SDQRZ	= 14,		// XENA-LH - SD Qrez bitfile							//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENAHS2_SD		= 15,		// XENA_HS2: load SD bitfile							//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENAHS2_HD		= 16,		// XENA-HS2: load HD bitfile							//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_MOAB_DNCVT		= 17,		// MOAB: load downconverter bitfile						//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_MOAB_UPCVT		= 18,		// MOAB: load upconverter bitfile						//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_MOAB_XDCVT		= 19,		// MOAB: load cross downconverter (1080->720) bitfile	//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_MOAB_XUCVT		= 20,		// MOAB: load cross upconverter (720->1080) bitfile		//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_XENA2_CSCVT	= 21,		// XENA2: load color space converter bitfile			//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_BORG_CODEC		= 25,		// Borg CoDec bitfile									//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_BORG_UFC		= 26,		// Borg VidProc bitfile									//	DEPRECATION_CANDIDATE
	NTV2_BITFILE_LHI_DVI_MAIN	= 34,		// LHi DVI main bitfile
#endif	//	!defined (NTV2_DEPRECATE)
	NTV2_BITFILE_CORVID1_MAIN	= 22,		// CORVID1 main bitfile
	NTV2_BITFILE_CORVID22_MAIN	= 23,		// Corvid22 main bitfile
	NTV2_BITFILE_KONA3G_MAIN	= 24,		// Kona3G main bitfile
	NTV2_BITFILE_LHI_MAIN		= 27,		// LHi main bitfile
	NTV2_BITFILE_IOEXPRESS_MAIN	= 28,		// IoExpress main bitfile
	NTV2_BITFILE_CORVID3G_MAIN	= 29,		// Corvid3G main bitfile
	NTV2_BITFILE_KONA3G_QUAD	= 30,		// Kona3G Quad bitfile
	NTV2_BITFILE_KONALHE_PLUS	= 31,		// LHePlus bitfile
	NTV2_BITFILE_IOXT_MAIN		= 32,		// IoXT bitfile
	NTV2_BITFILE_CORVID24_MAIN	= 33,		// Corvid24 main bitfile
	NTV2_BITFILE_TTAP_MAIN		= 35,		// T-Tap main bitfile
	NTV2_BITFILE_LHI_T_MAIN		= 36,		// LHi T main bitfile
	NTV2_BITFILE_IO4K_MAIN		= 37,
	NTV2_BITFILE_IO4KUFC_MAIN	= 38,
	NTV2_BITFILE_KONA4_MAIN		= 39,
	NTV2_BITFILE_KONA4UFC_MAIN	= 40,
	NTV2_BITFILE_CORVID88		= 41,
	NTV2_BITFILE_CORVID44		= 42,
	NTV2_BITFILE_CORVIDHEVC		= 43,
	NTV2_BITFILE_KONAIP_2022		= 44,
	NTV2_BITFILE_KONAIP_4CH_2SFP    = 45,
	NTV2_BITFILE_KONAIP_1RX_1TX_1SFP_J2K= 46,
	NTV2_BITFILE_KONAIP_2TX_1SFP_J2K= 47,
#if !defined(NTV2_DEPRECATE_15_6)
//	NTV2_BITFILE_KONAIP_2RX_1SFP_J2K= 48,	//	Never built or shipped
#endif	//	NTV2_DEPRECATE_15_6
	NTV2_BITFILE_KONAIP_1RX_1TX_2110= 49,
	NTV2_BITFILE_IO4KPLUS_MAIN	= 50,
	NTV2_BITFILE_IOIP_2022			= 51,
	NTV2_BITFILE_IOIP_2110			= 52,
	NTV2_BITFILE_KONAIP_2110		= 53,
	NTV2_BITFILE_KONA1				= 54,
	NTV2_BITFILE_KONAHDMI			= 55,
	NTV2_BITFILE_KONA5_MAIN			= 56,
	NTV2_BITFILE_KONA5_8KMK_MAIN	= 57,
	NTV2_BITFILE_CORVID44_8KMK_MAIN	= 58,
	NTV2_BITFILE_KONA5_8K_MAIN		= 59,
	NTV2_BITFILE_CORVID44_8K_MAIN	= 60,
	NTV2_BITFILE_TTAP_PRO_MAIN		= 61,
	NTV2_BITFILE_KONA5_2X4K_MAIN	= 62,
	NTV2_BITFILE_CORVID44_2X4K_MAIN	= 63,
	NTV2_BITFILE_KONA5_3DLUT_MAIN	= 64,
	NTV2_BITFILE_CORVID44_PLNR_MAIN	= 65,
	NTV2_BITFILE_IOX3_MAIN			= 66,
	NTV2_BITFILE_KONA5_OE1_MAIN		= 67,
	NTV2_BITFILE_KONA5_OE2_MAIN		= 68,
	NTV2_BITFILE_KONA5_OE3_MAIN		= 69,
	NTV2_BITFILE_KONA5_OE4_MAIN		= 70,
	NTV2_BITFILE_KONA5_OE5_MAIN		= 71,
	NTV2_BITFILE_KONA5_OE6_MAIN		= 72,
	NTV2_BITFILE_KONA5_OE7_MAIN		= 73,
	NTV2_BITFILE_KONA5_OE8_MAIN		= 74,
	NTV2_BITFILE_KONA5_OE9_MAIN		= 75,
	NTV2_BITFILE_KONA5_OE10_MAIN	= 76,
	NTV2_BITFILE_KONA5_OE11_MAIN	= 77,
	NTV2_BITFILE_KONA5_OE12_MAIN	= 78,
	NTV2_BITFILE_KONAIP_2110_RGB12	= 79,
	NTV2_BITFILE_IOIP_2110_RGB12	= 80,
	NTV2_BITFILE_NUMBITFILETYPES
} NTV2BitfileType;


#if !defined (NTV2_DEPRECATE)
	typedef enum
	{
		NTV2_BITFILE_K2					// KONA2: PCI firmware
	} NTV2PCIBitFileType;

	typedef enum
	{
		NTV2_BITFILE_MOABFW				// MOAB: firewire xilinx
	} NTV2FWBitFileType;


	typedef enum
	{
		NTV2_BITFILE_MOABPPC			// MOAB: PPC firmware
	} NTV2FirmwareBitFileType;
#endif	//	!defined (NTV2_DEPRECATE)


typedef enum
{
	NTV2_CSC_Method_Unimplemented,
	NTV2_CSC_Method_Original,
	NTV2_CSC_Method_Enhanced,
	NTV2_CSC_Method_Enhanced_4K,
	NTV2_MAX_NUM_ColorSpaceMethods
} NTV2ColorSpaceMethod;


typedef enum
{
	//	For use with the original color space converter
	NTV2_Rec709Matrix,						// Rec 709 is generally used in HD
	NTV2_Rec601Matrix,						// Rec 601 is generally used in SD

	//	Supported only by the enhanced color space converter
	NTV2_Custom_Matrix,						// The matrix coefficients are not a standard preset

	NTV2_Unity_Matrix,						// The matrix outputs are the same as the inputs
	NTV2_Unity_SMPTE_Matrix,				// A unity matrix with SMPTE pre- and post- offsets

	//	The GBR naming is a reminder the ordering of the hardware input channels is also GRB, not RGB
	NTV2_GBRFull_to_YCbCr_Rec709_Matrix,	// RGB full  range -> YCbCr Rec 709 (HD)
	NTV2_GBRFull_to_YCbCr_Rec601_Matrix,	// RGB full  range -> YCbCr Rec 601 (SD)
	NTV2_GBRSMPTE_to_YCbCr_Rec709_Matrix,	// RGB SMPTE range -> YCbCr Rec 709 (HD)
	NTV2_GBRSMPTE_to_YCbCr_Rec601_Matrix,	// RGB SMPTE range -> YCbCr Rec 601 (SD)

	NTV2_YCbCr_to_GBRFull_Rec709_Matrix,	// YCbCr -> RGB full  range Rec 709 (HD)
	NTV2_YCbCr_to_GBRFull_Rec601_Matrix,	// YCbCr -> RGB full  range Rec 601 (SD)
	NTV2_YCbCr_to_GBRSMPTE_Rec709_Matrix,	// YCbCr -> RGB SMPTE range Rec 709 (HD)
	NTV2_YCbCr_to_GBRSMPTE_Rec601_Matrix,	// YCbCr -> RGB SMPTE range Rec 601 (SD)

	NTV2_YCbCrRec601_to_YCbCrRec709_Matrix,	// YCbCr Rec 601 (SD) -> YCbCr Rec 709 (HD)
	NTV2_YCbCrRec709_to_YCbCrRec601_Matrix,	// YCbCr Rec 709 (HD) -> YCbCr Rec 601 (SD)

	NTV2_GBRFull_to_GBRSMPTE_Matrix,		// RGB full  range -> RGB SMPTE range
	NTV2_GBRSMPTE_to_GBRFull_Matrix,		// RGB SMPTE range -> RGB full  range

    NTV2_GBRFull_to_YCbCr_Rec2020_Matrix,	// RGB full  range -> YCbCr Rec 2020
    NTV2_GBRSMPTE_to_YCbCr_Rec2020_Matrix,	// RGB SMPTE range -> YCbCr Rec 2020
    NTV2_YCbCr_to_GBRFull_Rec2020_Matrix,	// YCbCr -> RGB full  range Rec 2020
    NTV2_YCbCr_to_GBRSMPTE_Rec2020_Matrix,	// YCbCr -> RGB SMPTE range Rec 2020

	#if !defined (NTV2_DEPRECATE)
		NTV2K2_Rec709Matrix		= NTV2_Rec709Matrix,
		NTV2K2_Rec601Matrix		= NTV2_Rec601Matrix,
	#endif	//	!defined (NTV2_DEPRECATE)
	NTV2_MAX_NUM_ColorSpaceMatrixTypes,
	NTV2_CSC_MATRIX_TYPE_INVALID = NTV2_MAX_NUM_ColorSpaceMatrixTypes
} NTV2ColorSpaceMatrixType;

#define	NTV2_IS_VALID_CSC_MATRIX_TYPE(__p__)	((__p__) >= NTV2_Rec709Matrix	&& (__p__) < NTV2_MAX_NUM_ColorSpaceMatrixTypes)


typedef enum
{
	 NTV2_DSKModeOff
	,NTV2_DSKModeFBOverMatte
	,NTV2_DSKModeFBOverVideoIn
	,NTV2_DSKModeGraphicOverMatte
	,NTV2_DSKModeGraphicOverVideoIn
	,NTV2_DSKModeGraphicOverFB
	,NTV2_DSKModeMax
	,NTV2_DSKMODE_INVALID = NTV2_DSKModeMax
} NTV2DSKMode;


typedef enum
{
	NTV2_DSKForegroundUnshaped,		//	DSK Foreground is not pre-multiplied by alpha
	NTV2_DSKForegroundShaped		//	DSK Background has already been pre-multiplied by alpha
} NTV2DSKForegroundMode;


typedef enum
{
	NTV2_DSKAudioForeground,		//	Take audio from frame buffer ("Playback")
	NTV2_DSKAudioBackground			//	Take audio from input ("E-E")
} NTV2DSKAudioMode;


// This is a user-pref control (currently only used on the Mac) that allows the user
// to specify which color-space matrix to use when converting between RGB and YUV
typedef enum
{
	NTV2_ColorSpaceTypeAuto,		// switch between Rec 601 for SD, Rec 709 for HD
	NTV2_ColorSpaceTypeRec601,		// always use Rec 601 matrix
	NTV2_ColorSpaceTypeRec709,		// always use Rec 709 matrix
	NTV2_MAX_NUM_ColorSpaceTypes
} NTV2ColorSpaceType;


/**
	@brief	This is a user-pref control (currently only used on the Mac) that allows the user
			to specify which flavor of Stereo 3D is to be used
**/
typedef enum
{
	NTV2_Stereo3DOff,				// Stereo Mode disabled
	NTV2_Stereo3DSideBySide,		// Muxed Single Stream - Side-By-Side
	NTV2_Stereo3DTopBottom,			// Muxed Single Stream - Top over Bottom
	NTV2_Stereo3DDualStream,		// Two independant streams
	NTV2_MAX_NUM_Stereo3DModes
} NTV2Stereo3DMode;


// The Mac implementation of color-space conversion uses the two LUT banks for holding
// two separate gamma-conversion LUTs: one for RGB=>YUV and the other for YUV=>RGB. This
// defines which bank is used for which conversion LUT.
// NOTE: using the LUT Banks this way conflicts with using them for AutoCirculate "Color Correction".
#define kLUTBank_RGB2YUV	0		// uses Bank 0 when converting from RGB=>YUV
#define	kLUTBank_YUV2RGB	1		// uses Bank 1 when converting from YUV=>RGB
#define kLUTBank_SMPTE2FULL	0		// uses Bank 0 when converting from SMPTE=>Full range RGB
#define kLUTBank_FULL2SMPTE	1		// uses Bank 0 when converting from Full=>SMPTE range RGB


/**
	@brief	This specifies what function(s) are currently loaded into the LUTs
**/
typedef enum
{
	NTV2_LUTUnknown,				//	Don't know...
	NTV2_LUTCustom,					//	None of the below (on purpose)
	NTV2_LUTLinear,					//	Linear (both banks)
	NTV2_LUTGamma18_Rec601,			//	Translates between Mac 1.8 and Rec 601 (Power Function 2.2), Full Range
	NTV2_LUTGamma18_Rec709,			//	Translates between Mac 1.8 and Rec 709, Full Range
	NTV2_LUTGamma18_Rec601_SMPTE,	//	Same as NTV2_LUTGamma18_Rec601, SMPTE range
	NTV2_LUTGamma18_Rec709_SMPTE,	//	Same as NTV2_LUTGamma18_Rec709, SMPTE range
	NTV2_LUTRGBRangeFull_SMPTE,		//	Translates Full <-> SMPTE range
	NTV2_MAX_NUM_LutTypes,
	NTV2_INVALID_LUT_TYPE = NTV2_MAX_NUM_LutTypes
} NTV2LutType;

#define	NTV2_IS_VALID_LUT_TYPE(__x__)		((__x__) >= NTV2_LUTUnknown  &&  (__x__) < NTV2_MAX_NUM_LutTypes)

/**
	   @brief  This specifies the LUT bit depth
**/
typedef enum
{
   NTV2_LUT10Bit,
   NTV2_LUT12Bit
} NTV2LutBitDepth;


/**
	@brief	This specifies the HDMI Out Stereo 3D Mode
**/
typedef enum
{
	NTV2_HDMI3DFramePacking	= 0x0,	//	Frame Packing mode
	NTV2_HDMI3DSideBySide	= 0x8,	//	Side By Side Stereo 3D mode
	NTV2_HDMI3DTopBottom	= 0x6,	//	Top Over Bottom Stereo 3D mode
	NTV2_MAX_NUM_HDMIOut3DModes
} NTV2HDMIOut3DMode;


/**
	@brief	Indicates or specifies HDMI Color Space
**/
typedef enum
{
	NTV2_HDMIColorSpaceAuto,	///< @brief	Automatic (not for OEM use)
	NTV2_HDMIColorSpaceRGB,		///< @brief	RGB color space
	NTV2_HDMIColorSpaceYCbCr,	///< @brief	YCbCr color space
	NTV2_MAX_NUM_HDMIColorSpaces,
	NTV2_INVALID_HDMI_COLORSPACE	= NTV2_MAX_NUM_HDMIColorSpaces
} NTV2HDMIColorSpace;

#define	NTV2_IS_VALID_HDMI_COLORSPACE(__x__)		((__x__) > NTV2_HDMIColorSpaceAuto  &&  (__x__) < NTV2_MAX_NUM_HDMIColorSpaces)


/**
	@brief	Indicates or specifies the HDMI protocol
**/
typedef enum
{
	NTV2_HDMIProtocolHDMI,		///< @brief	HDMI protocol
	NTV2_HDMIProtocolDVI,		///< @brief	DVI protocol
	NTV2_MAX_NUM_HDMIProtocols,
	NTV2_INVALID_HDMI_PROTOCOL	= NTV2_MAX_NUM_HDMIProtocols
} NTV2HDMIProtocol;

#define	NTV2_IS_VALID_HDMI_PROTOCOL(__x__)	((__x__) >= NTV2_HDMIProtocolHDMI  &&  (__x__) < NTV2_MAX_NUM_HDMIProtocols)


/**
	@brief	Indicates or specifies the HDMI RGB range
**/
typedef enum
{
	NTV2_HDMIRangeSMPTE,		///< @brief	Levels are 16 - 235 (SMPTE)
	NTV2_HDMIRangeFull,			///< @brief	Levels are  0 - 255 (Full)
	NTV2_MAX_NUM_HDMIRanges,
	NTV2_INVALID_HDMI_RANGE	= NTV2_MAX_NUM_HDMIRanges
} NTV2HDMIRange;

#define	NTV2_IS_VALID_HDMI_RANGE(__x__)		((__x__) < NTV2_MAX_NUM_HDMIRanges)


/**
	@brief	Indicates or specifies the HDMI colorimetry
**/
typedef enum
{
	NTV2_HDMIColorimetryNoData,
	NTV2_HDMIColorimetry601,
	NTV2_HDMIColorimetry709,
	NTV2_HDMIColorimetry2020,
	NTV2_HDMIColorimetry2020CL,
	NTV2_HDMIColorimetryDCI,
	NTV2_MAX_NUM_HDMIColorimetry,
	NTV2_INVALID_HDMI_Colorimetry	= NTV2_MAX_NUM_HDMIColorimetry
} NTV2HDMIColorimetry;

#define	NTV2_IS_VALID_HDMI_COLORIMETRY(__x__)		((__x__) < NTV2_MAX_NUM_HDMIColorimetry)


/**
	@brief	Indicates or specifies the HDMI audio channel count
**/
typedef enum
{
	NTV2_HDMIAudio2Channels,	///< @brief	2 audio channels
	NTV2_HDMIAudio8Channels,	///< @brief	8 audio channels
	NTV2_MAX_NUM_HDMIAudioChannelEnums,
	NTV2_INVALID_HDMI_AUDIO_CHANNELS	=	NTV2_MAX_NUM_HDMIAudioChannelEnums
} NTV2HDMIAudioChannels;

#define	NTV2_IS_VALID_HDMI_AUDIO_CHANNELS(__x__)	((__x__) >= NTV2_HDMIAudio2Channels)  &&  ((__x__) < NTV2_INVALID_HDMI_AUDIO_CHANNELS)

// LHI version HDMI Color Space I/O
typedef enum
{
	NTV2_LHIHDMIColorSpaceYCbCr,	//	YCbCr
	NTV2_LHIHDMIColorSpaceRGB,		//	RGB
	NTV2_MAX_NUM_LHIHDMIColorSpaces,
	NTV2_INVALID_LHI_HDMI_COLORSPACE	= NTV2_MAX_NUM_LHIHDMIColorSpaces
} NTV2LHIHDMIColorSpace;

#define	NTV2_IS_VALID_LHI_HDMI_COLORSPACE(__x__)	((__x__) < NTV2_MAX_NUM_LHIHDMIColorSpaces)


/**
	@brief	Indicates or specifies the HDMI video bit depth
**/
typedef enum
{
	NTV2_HDMI8Bit,				///< @brief	8 bit
	NTV2_HDMI10Bit,				///< @brief	10 bit
	NTV2_HDMI12Bit,				///< @brief	12 bit
	NTV2_MAX_NUM_HDMIBitDepths,
	NTV2_INVALID_HDMIBitDepth = NTV2_MAX_NUM_HDMIBitDepths
} NTV2HDMIBitDepth;

#define	NTV2_IS_VALID_HDMI_BITDEPTH(__x__)	((__x__) < NTV2_MAX_NUM_HDMIBitDepths)


// This specifies the output selection for the LH board
typedef enum
{
	NTV2LHOutputSelect_VidProc1,
	NTV2LHOutputSelect_DownConvert,
	NTV2LHOutputSelect_VidProc2,
	NTV2_MAX_NUM_LHOutputSelectEnums
} NTV2LHOutputSelect;


#if !defined (NTV2_DEPRECATE)
	//	Deprecated .. used only by Xena DXT application, which never shipped
	typedef enum
	{	DT_XENA_DXT_FIRMWARE_DESIRED_UNINITIALIZED,
		DT_XENA_DXT_FIRMWARE_DESIRED_HD,
		DT_XENA_DXT_FIRMWARE_DESIRED_SD
	} DT_XENA_DXT_FIRMWARE_DESIRED;
#endif	//	!defined (NTV2_DEPRECATE)


typedef enum
{
	 NTV2_1080i_5994to525_5994		//	0
	,NTV2_1080i_2500to625_2500		//	1
	,NTV2_720p_5994to525_5994		//	2
	,NTV2_720p_5000to625_2500		//	3
	,NTV2_525_5994to1080i_5994		//	4
	,NTV2_525_5994to720p_5994		//	5
	,NTV2_625_2500to1080i_2500		//	6
	,NTV2_625_2500to720p_5000		//	7
	,NTV2_720p_5000to1080i_2500		//	8
	,NTV2_720p_5994to1080i_5994		//	9
	,NTV2_720p_6000to1080i_3000		//	10
	,NTV2_1080i2398to525_2398		//	11
	,NTV2_1080i2398to525_2997		//	12
	,NTV2_1080i2400to525_2400		//	13
	,NTV2_1080p2398to525_2398		//	14
	,NTV2_1080p2398to525_2997		//	15
	,NTV2_1080p2400to525_2400		//	16
	,NTV2_1080i_2500to720p_5000		//	17
	,NTV2_1080i_5994to720p_5994		//	18
	,NTV2_1080i_3000to720p_6000		//	19
	,NTV2_1080i_2398to720p_2398		//	20
	,NTV2_720p_2398to1080i_2398		//	21
	,NTV2_525_2398to1080i_2398		//	22
	,NTV2_525_5994to525_5994		//	23
	,NTV2_625_2500to625_2500		//	24
	,NTV2_525_5994to525psf_2997		//	25
	,NTV2_625_5000to625psf_2500		//	26
	,NTV2_1080i_5000to1080psf_2500	//	27
	,NTV2_1080i_5994to1080psf_2997	//	28
	,NTV2_1080i_6000to1080psf_3000	//	29
	,NTV2_1080p_3000to720p_6000		//	30
	,NTV2_1080psf_2398to1080i_5994	//	31
	,NTV2_1080psf_2400to1080i_3000	//	32
	,NTV2_1080psf_2500to1080i_2500	//	33
	,NTV2_1080p_2398to1080i_5994	//	34
	,NTV2_1080p_2400to1080i_3000	//	35
	,NTV2_1080p_2500to1080i_2500	//	36
	,NTV2_NUM_CONVERSIONMODES		//	37
	,NTV2_CONVERSIONMODE_UNKNOWN = NTV2_NUM_CONVERSIONMODES
	,NTV2_CONVERSIONMODE_INVALID = NTV2_NUM_CONVERSIONMODES
} NTV2ConversionMode;


typedef enum
{
	NTV2_CSC_RGB_RANGE_FULL,
	NTV2_RGBBLACKRANGE_0_0x3FF		= NTV2_CSC_RGB_RANGE_FULL,
	NTV2_CSC_RGB_RANGE_SMPTE,
	NTV2_RGBBLACKRANGE_0x40_0x3C0	= NTV2_CSC_RGB_RANGE_SMPTE,
	NTV2_CSC_RGB_RANGE_INVALID,
	NTV2_MAX_NUM_RGBBlackRangeEnums	= NTV2_CSC_RGB_RANGE_INVALID
} NTV2_CSC_RGB_Range;

typedef NTV2_CSC_RGB_Range	NTV2RGBBlackRange;

#define	NTV2_IS_VALID_CSCRGBRANGE(__v__)		((__v__) >= NTV2_CSC_RGB_RANGE_FULL && (__v__) < NTV2_CSC_RGB_RANGE_INVALID)


typedef enum
{
	NTV2_VIDEOLIMITING_LEGALSDI,
	NTV2_VIDEOLIMITING_OFF,
	NTV2_VIDEOLIMITING_LEGALBROADCAST,
	NTV2_MAX_NUM_VideoLimitingEnums,
	NTV2_VIDEOLIMITING_INVALID	=	NTV2_MAX_NUM_VideoLimitingEnums
} NTV2VideoLimiting;

#define	NTV2_IS_VALID_VIDEOLIMITING(__v__)			((__v__) >= NTV2_VIDEOLIMITING_LEGALSDI && (__v__) < NTV2_VIDEOLIMITING_INVALID)
#define	NTV2_IS_LIMITING_LEGALSDI(__v__)			((__v__) == NTV2_VIDEOLIMITING_LEGALSDI)
#define	NTV2_IS_LIMITING_OFF(__v__)					((__v__) == NTV2_VIDEOLIMITING_OFF)
#define	NTV2_IS_LIMITING_LEGALBROADCAST(__v__)		((__v__) == NTV2_VIDEOLIMITING_LEGALBROADCAST)


/**
	@brief	These enum values identify the available VANC modes.
	@see	CNTV2Card::GetVANCMode, CNTV2Card::SetVANCMode, \ref vidop-fs, \ref vancframegeometries
**/
typedef enum
{
	NTV2_VANCMODE_OFF,		///< @brief	This identifies the mode in which there are no VANC lines in the frame buffer.
	NTV2_VANCMODE_TALL,		///< @brief	This identifies the "tall" mode in which there are some VANC lines in the frame buffer.
	NTV2_VANCMODE_TALLER,	///< @brief	This identifies the mode in which there are some + extra VANC lines in the frame buffer.
	NTV2_VANCMODE_INVALID	///< @brief	This identifies the invalid (unspecified, uninitialized) VANC mode.
} NTV2VANCMode;

#define	NTV2_IS_VALID_VANCMODE(__v__)			((__v__) >= NTV2_VANCMODE_OFF && (__v__) < NTV2_VANCMODE_INVALID)
#define	NTV2_IS_VANCMODE_TALL(__v__)			((__v__) == NTV2_VANCMODE_TALL)
#define	NTV2_IS_VANCMODE_TALLER(__v__)			((__v__) == NTV2_VANCMODE_TALLER)
#define NTV2_IS_VANCMODE_ON(__v__)				((__v__) > NTV2_VANCMODE_OFF && (__v__) < NTV2_VANCMODE_INVALID)
#define NTV2_IS_VANCMODE_OFF(__v__)				((__v__) == NTV2_VANCMODE_OFF)
#define NTV2VANCModeFromBools(_tall_,_taller_)	NTV2VANCMode ((_tall_) ? ((_taller_) ? NTV2_VANCMODE_TALLER : NTV2_VANCMODE_TALL) : NTV2_VANCMODE_OFF)


typedef enum
{
	NTV2_VANCDATA_NORMAL,
	NTV2_VANCDATA_8BITSHIFT_DISABLE = NTV2_VANCDATA_NORMAL,
	NTV2_VANCDATA_8BITSHIFT_ENABLE,
	NTV2_VANCDATA_INVALID,
	NTV2_MAX_NUM_VANCDataShiftModes = NTV2_VANCDATA_INVALID
} NTV2VANCDataShiftMode;

#define	NTV2_IS_VALID_VANCDATASHIFT(__v__)		((__v__) >= NTV2_VANCDATA_NORMAL && (__v__) < NTV2_MAX_NUM_VANCDataShiftModes)
#define	NTV2_IS_VANCDATASHIFT_ENABLED(__v__)	((__v__) == NTV2_VANCDATA_8BITSHIFT_ENABLE)


/////////////////////////////////////////////////////////////////////////////////////
// Driver debug message categories

typedef enum
{
   NTV2_DRIVER_ALL_DEBUG_MESSAGES	= -1,			//	Enable/disable all others
   NTV2_DRIVER_VIDEO_DEBUG_MESSAGES	= 0,			//	Video-specific messages
   NTV2_DRIVER_AUDIO_DEBUG_MESSAGES,				//	Audio-specific messages
   NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES,		//	ISR Autocirculate msgs
   NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES,//	Autocirculate control msgs
   NTV2_DRIVER_DMA_AUDIO_DEBUG_MESSAGES,			//	Audio Direct Memory Access msgs
   NTV2_DRIVER_DMA_VIDEO_DEBUG_MESSAGES,			//	Video Direct Memory Access msgs
   NTV2_DRIVER_RP188_DEBUG_MESSAGES,				//	For debugging RP188 messages
   NTV2_DRIVER_CUSTOM_ANC_DATA_DEBUG_MESSAGES,		//	For debugging ancillary data
   NTV2_DRIVER_DEBUG_DEBUG_MESSAGES,				//	For debugging debug messages
   NTV2_DRIVER_I2C_DEBUG_MESSAGES,					//	For debugging procamp/I2C writes
   NTV2_DRIVER_NUM_DEBUG_MESSAGE_SETS				//	Must be last!
} NTV2_DriverDebugMessageSet;


typedef enum
{
	eFPGAVideoProc,
	eFPGACodec,
	eFPGA_NUM_FPGAs
} NTV2XilinxFPGA;


#if !defined (NTV2_DEPRECATE)
	typedef enum
	{
		eButtonReleased,
		eButtonPressed,
		eButtonNotSupported
	} NTV2ButtonState;
#endif	//	!defined (NTV2_DEPRECATE)


/////////////////////////////////////////////////////////////////////////////////////
// Stereo Compressor Stuff
typedef enum
{
	NTV2_STEREOCOMPRESSOR_PASS_LEFT,
	NTV2_STEREOCOMPRESSOR_SIDExSIDE,
	NTV2_STEREOCOMPRESSOR_TOP_BOTTOM,
	NTV2_STEREOCOMPRESSOR_PASS_RIGHT,
	NTV2_STEREOCOMPRESSOR_DISABLE = 0xF
} NTV2StereoCompressorOutputMode;


typedef enum
{
	NTV2_STEREOCOMPRESSOR_NO_FLIP		= 0x0,
	NTV2_STEREOCOMPRESSOR_LEFT_HORZ		= 0x1,
	NTV2_STEREOCOMPRESSOR_LEFT_VERT		= 0x2,
	NTV2_STEREOCOMPRESSOR_RIGHT_HORZ	= 0x4,
	NTV2_STEREOCOMPRESSOR_RIGHT_VERT	= 0x8
} NTV2StereoCompressorFlipMode;


typedef enum
{
	NTV2_LUTCONTROL_1_2,
	NTV2_LUTCONTROL_3_4,
	NTV2_LUTCONTROL_5
} NTV2LUTControlSelect;
		
typedef enum
{
	NTV2_REDPLANE = 0x3,
	NTV2_GREENPLANE = 0x2,
	NTV2_BLUEPLANE = 0x1
} NTV2LUTPlaneSelect;


/**
	@brief	These enum values identify the available NTV2 Audio Systems.
	@see	See \ref audiooperation
**/
typedef enum
{
	NTV2_AUDIOSYSTEM_1,	///< @brief	This identifies the first Audio System.
	NTV2_AUDIOSYSTEM_2,	///< @brief	This identifies the 2nd Audio System.
	NTV2_AUDIOSYSTEM_3,	///< @brief	This identifies the 3rd Audio System.
	NTV2_AUDIOSYSTEM_4,	///< @brief	This identifies the 4th Audio System.
	NTV2_AUDIOSYSTEM_5,	///< @brief	This identifies the 5th Audio System.
	NTV2_AUDIOSYSTEM_6,	///< @brief	This identifies the 6th Audio System.
	NTV2_AUDIOSYSTEM_7,	///< @brief	This identifies the 7th Audio System.
	NTV2_AUDIOSYSTEM_8,	///< @brief	This identifies the 8th Audio System.
	NTV2_MAX_NUM_AudioSystemEnums,
	NTV2_NUM_AUDIOSYSTEMS		= NTV2_MAX_NUM_AudioSystemEnums,
	NTV2_AUDIOSYSTEM_INVALID	= NTV2_NUM_AUDIOSYSTEMS
} NTV2AudioSystem;

#define	NTV2_IS_VALID_AUDIO_SYSTEM(__x__)			((__x__) >= NTV2_AUDIOSYSTEM_1 && (__x__) < NTV2_MAX_NUM_AudioSystemEnums)


typedef enum
{
	NTV2_SDI1AUDIO,
	NTV2_SDI2AUDIO,
	NTV2_SDI3AUDIO,
	NTV2_SDI4AUDIO,
	NTV2_SDI5AUDIO,
	NTV2_SDI6AUDIO,
	NTV2_SDI7AUDIO,
	NTV2_SDI8AUDIO,
	NTV2_MAX_NUM_SDIAudioSelectEnums
	#if !defined (NTV2_DEPRECATE)
		,NTV2_NUM_SDIAUDIO	= NTV2_MAX_NUM_SDIAudioSelectEnums
	#endif	//	!defined (NTV2_DEPRECATE)
} NTV2SDIAudioSelect;


/**
	@brief	This enumerated data type identifies the two possible states of the bypass relays.
			See CNTV2Card::GetSDIRelayPosition, CNTV2Card::GetSDIRelayManualControl, CNTV2Card::GetSDIWatchdogStatus, etc.
**/
typedef enum
{
	NTV2_DEVICE_BYPASSED,		///< @brief	Input & output directly connected
	NTV2_THROUGH_DEVICE,		///< @brief	Input & output routed through device
	#if !defined (NTV2_DEPRECATE)
		NTV2_BYPASS				= NTV2_DEVICE_BYPASSED,
		NTV2_STRAIGHT_THROUGH	= NTV2_THROUGH_DEVICE,
	#endif	//	NTV2_DEPRECATE
	NTV2_MAX_NUM_RelayStates,
	NTV2_RELAY_STATE_INVALID	= NTV2_MAX_NUM_RelayStates
} NTV2RelayState;

#define	NTV2_IS_VALID_RELAY_STATE(__x__)			((__x__) >= NTV2_DEVICE_BYPASSED  &&  (__x__) < NTV2_MAX_NUM_RelayStates)


/**
	@brief	These enum values are indexes into the capture/playout AutoCirculate timecode arrays
	@see	AUTOCIRCULATE_TRANSFER::GetInputTimeCode, AUTOCIRCULATE_TRANSFER::SetOutputTimeCode
**/
typedef enum
{
	NTV2_TCINDEX_DEFAULT,		///< @brief	The "default" timecode (mostly used by the AJA "Retail" service and Control Panel)
	NTV2_TCINDEX_SDI1,			///< @brief	SDI 1 embedded VITC
	NTV2_TCINDEX_SDI2,			///< @brief	SDI 2 embedded VITC
	NTV2_TCINDEX_SDI3,			///< @brief	SDI 3 embedded VITC
	NTV2_TCINDEX_SDI4,			///< @brief	SDI 4 embedded VITC
	NTV2_TCINDEX_SDI1_LTC,		///< @brief	SDI 1 embedded ATC LTC
	NTV2_TCINDEX_SDI2_LTC,		///< @brief	SDI 2 embedded ATC LTC
	NTV2_TCINDEX_LTC1,			///< @brief	Analog LTC 1
	NTV2_TCINDEX_LTC2,			///< @brief	Analog LTC 2
	NTV2_TCINDEX_SDI5,			///< @brief	SDI 5 embedded VITC
	NTV2_TCINDEX_SDI6,			///< @brief	SDI 6 embedded VITC
	NTV2_TCINDEX_SDI7,			///< @brief	SDI 7 embedded VITC
	NTV2_TCINDEX_SDI8,			///< @brief	SDI 8 embedded VITC
	NTV2_TCINDEX_SDI3_LTC,		///< @brief	SDI 3 embedded ATC LTC
	NTV2_TCINDEX_SDI4_LTC,		///< @brief	SDI 4 embedded ATC LTC
	NTV2_TCINDEX_SDI5_LTC,		///< @brief	SDI 5 embedded ATC LTC
	NTV2_TCINDEX_SDI6_LTC,		///< @brief	SDI 6 embedded ATC LTC
	NTV2_TCINDEX_SDI7_LTC,		///< @brief	SDI 7 embedded ATC LTC
	NTV2_TCINDEX_SDI8_LTC,		///< @brief	SDI 8 embedded ATC LTC
	NTV2_TCINDEX_SDI1_2,		///< @brief	SDI 1 embedded VITC 2
	NTV2_TCINDEX_SDI2_2,		///< @brief	SDI 2 embedded VITC 2
	NTV2_TCINDEX_SDI3_2,		///< @brief	SDI 3 embedded VITC 2
	NTV2_TCINDEX_SDI4_2,		///< @brief	SDI 4 embedded VITC 2
	NTV2_TCINDEX_SDI5_2,		///< @brief	SDI 5 embedded VITC 2
	NTV2_TCINDEX_SDI6_2,		///< @brief	SDI 6 embedded VITC 2
	NTV2_TCINDEX_SDI7_2,		///< @brief	SDI 7 embedded VITC 2
	NTV2_TCINDEX_SDI8_2,		///< @brief	SDI 8 embedded VITC 2
	NTV2_MAX_NUM_TIMECODE_INDEXES,
	NTV2_TCINDEX_INVALID	= NTV2_MAX_NUM_TIMECODE_INDEXES
	#if !defined (NTV2_DEPRECATE)
		,NTV2_NUM_TCSOURCES	= NTV2_MAX_NUM_TIMECODE_INDEXES
	#endif	//	!defined (NTV2_DEPRECATE)
} NTV2TCIndex, NTV2TimecodeIndex;

#define	NTV2_IS_VALID_TIMECODE_INDEX(__x__)				(int32_t(__x__) >= int32_t(NTV2_TCINDEX_DEFAULT)  &&  int32_t(__x__) < int32_t(NTV2_MAX_NUM_TIMECODE_INDEXES))

#define	NTV2_IS_ANALOG_TIMECODE_INDEX(__x__)			((__x__) == NTV2_TCINDEX_LTC1 || (__x__) == NTV2_TCINDEX_LTC2)

#define	NTV2_IS_ATC_VITC1_TIMECODE_INDEX(__x__)			(		((__x__) >= NTV2_TCINDEX_SDI1  &&  (__x__) <= NTV2_TCINDEX_SDI4)	\
															||  ((__x__) >= NTV2_TCINDEX_SDI5  &&  (__x__) <= NTV2_TCINDEX_SDI8)	)

#define	NTV2_IS_ATC_VITC2_TIMECODE_INDEX(__x__)			(	(__x__) >= NTV2_TCINDEX_SDI1_2  &&  (__x__) <= NTV2_TCINDEX_SDI8_2	)

#define	NTV2_IS_ATC_LTC_TIMECODE_INDEX(__x__)			(		((__x__) >= NTV2_TCINDEX_SDI3_LTC  &&  (__x__) <= NTV2_TCINDEX_SDI8_LTC)	\
															||  ((__x__) == NTV2_TCINDEX_SDI1_LTC)											\
															||	((__x__) == NTV2_TCINDEX_SDI2_LTC)	)

#define	NTV2_IS_SDI_TIMECODE_INDEX(__x__)				(NTV2_IS_VALID_TIMECODE_INDEX(__x__)  &&  !NTV2_IS_ANALOG_TIMECODE_INDEX(__x__))


#if !defined(NTV2_DEPRECATE)
	typedef	NTV2TCIndex						NTV2TCSource;					///< @deprecated	Use NTV2TCIndex instead.
	#define	NTV2_TCSOURCE_DEFAULT			NTV2_TCINDEX_DEFAULT			///< @deprecated	Use NTV2_TCINDEX_DEFAULT instead.
	#define	NTV2_TCSOURCE_SDI1				NTV2_TCINDEX_SDI1				///< @deprecated	Use NTV2_TCINDEX_SDI1 instead.
	#define	NTV2_TCSOURCE_SDI2				NTV2_TCINDEX_SDI2				///< @deprecated	Use NTV2_TCINDEX_SDI2 instead.
	#define	NTV2_TCSOURCE_SDI3				NTV2_TCINDEX_SDI3				///< @deprecated	Use NTV2_TCINDEX_SDI3 instead.
	#define	NTV2_TCSOURCE_SDI4				NTV2_TCINDEX_SDI4				///< @deprecated	Use NTV2_TCINDEX_SDI4 instead.
	#define	NTV2_TCSOURCE_SDI1_LTC			NTV2_TCINDEX_SDI1_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI1_LTC instead.
	#define	NTV2_TCSOURCE_SDI2_LTC			NTV2_TCINDEX_SDI2_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI2_LTC instead.
	#define	NTV2_TCSOURCE_LTC1				NTV2_TCINDEX_LTC1				///< @deprecated	Use NTV2_TCINDEX_LTC1 instead.
	#define	NTV2_TCSOURCE_LTC2				NTV2_TCINDEX_LTC2				///< @deprecated	Use NTV2_TCINDEX_LTC2 instead.
	#define	NTV2_TCSOURCE_SDI5				NTV2_TCINDEX_SDI5				///< @deprecated	Use NTV2_TCINDEX_SDI5 instead.
	#define	NTV2_TCSOURCE_SDI6				NTV2_TCINDEX_SDI6				///< @deprecated	Use NTV2_TCINDEX_SDI6 instead.
	#define	NTV2_TCSOURCE_SDI7				NTV2_TCINDEX_SDI7				///< @deprecated	Use NTV2_TCINDEX_SDI7 instead.
	#define	NTV2_TCSOURCE_SDI8				NTV2_TCINDEX_SDI8				///< @deprecated	Use NTV2_TCINDEX_SDI8 instead.
	#define	NTV2_TCSOURCE_SDI3_LTC			NTV2_TCINDEX_SDI3_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI3_LTC instead.
	#define	NTV2_TCSOURCE_SDI4_LTC			NTV2_TCINDEX_SDI4_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI4_LTC instead.
	#define	NTV2_TCSOURCE_SDI5_LTC			NTV2_TCINDEX_SDI5_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI5_LTC instead.
	#define	NTV2_TCSOURCE_SDI6_LTC			NTV2_TCINDEX_SDI6_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI6_LTC instead.
	#define	NTV2_TCSOURCE_SDI7_LTC			NTV2_TCINDEX_SDI7_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI7_LTC instead.
	#define	NTV2_TCSOURCE_SDI8_LTC			NTV2_TCINDEX_SDI8_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI8_LTC instead.
	#define	NTV2_MAX_NUM_TIMECODE_SOURCES	NTV2_MAX_NUM_TIMECODE_INDEXES	///< @deprecated	Use NTV2_MAX_NUM_TIMECODE_INDEXES instead.
	#define	NTV2_TCSOURCE_INVALID			NTV2_TCINDEX_INVALID			///< @deprecated	Use NTV2_TCINDEX_INVALID instead.
	#define	NTV2_NUM_TCSOURCES				NTV2_MAX_NUM_TIMECODE_INDEXES	///< @deprecated	Use NTV2_MAX_NUM_TIMECODE_INDEXES instead.

	typedef	NTV2TCIndex						NTV2TCDestination;				///< @deprecated	Use NTV2TCIndex instead.
	#define	NTV2_TCDEST_DEFAULT				NTV2_TCINDEX_DEFAULT			///< @deprecated	Use NTV2_TCINDEX_DEFAULT instead.
	#define	NTV2_TCDEST_SDI1				NTV2_TCINDEX_SDI1				///< @deprecated	Use NTV2_TCINDEX_SDI1 instead.
	#define	NTV2_TCDEST_SDI2				NTV2_TCINDEX_SDI2				///< @deprecated	Use NTV2_TCINDEX_SDI2 instead.
	#define	NTV2_TCDEST_SDI3				NTV2_TCINDEX_SDI3				///< @deprecated	Use NTV2_TCINDEX_SDI3 instead.
	#define	NTV2_TCDEST_SDI4				NTV2_TCINDEX_SDI4				///< @deprecated	Use NTV2_TCINDEX_SDI4 instead.
	#define	NTV2_TCDEST_SDI1_LTC			NTV2_TCINDEX_SDI1_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI1_LTC instead.
	#define	NTV2_TCDEST_SDI2_LTC			NTV2_TCINDEX_SDI2_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI2_LTC instead.
	#define	NTV2_TCDEST_LTC1				NTV2_TCINDEX_LTC1				///< @deprecated	Use NTV2_TCINDEX_LTC1 instead.
	#define	NTV2_TCDEST_LTC2				NTV2_TCINDEX_LTC2				///< @deprecated	Use NTV2_TCINDEX_LTC2 instead.
	#define	NTV2_TCDEST_SDI5				NTV2_TCINDEX_SDI5				///< @deprecated	Use NTV2_TCINDEX_SDI5 instead.
	#define	NTV2_TCDEST_SDI6				NTV2_TCINDEX_SDI6				///< @deprecated	Use NTV2_TCINDEX_SDI6 instead.
	#define	NTV2_TCDEST_SDI7				NTV2_TCINDEX_SDI7				///< @deprecated	Use NTV2_TCINDEX_SDI7 instead.
	#define	NTV2_TCDEST_SDI8				NTV2_TCINDEX_SDI8				///< @deprecated	Use NTV2_TCINDEX_SDI8 instead.
	#define	NTV2_TCDEST_SDI3_LTC			NTV2_TCINDEX_SDI3_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI3_LTC instead.
	#define	NTV2_TCDEST_SDI4_LTC			NTV2_TCINDEX_SDI4_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI4_LTC instead.
	#define	NTV2_TCDEST_SDI5_LTC			NTV2_TCINDEX_SDI5_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI5_LTC instead.
	#define	NTV2_TCDEST_SDI6_LTC			NTV2_TCINDEX_SDI6_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI6_LTC instead.
	#define	NTV2_TCDEST_SDI7_LTC			NTV2_TCINDEX_SDI7_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI7_LTC instead.
	#define	NTV2_TCDEST_SDI8_LTC			NTV2_TCINDEX_SDI8_LTC			///< @deprecated	Use NTV2_TCINDEX_SDI8_LTC instead.
	#define	NTV2_MAX_NUM_TIMECODE_DESTS		NTV2_MAX_NUM_TIMECODE_INDEXES	///< @deprecated	Use NTV2_MAX_NUM_TIMECODE_INDEXES instead.
	#define	NTV2_TCDEST_INVALID				NTV2_TCINDEX_INVALID			///< @deprecated	Use NTV2_TCINDEX_INVALID instead.
	#define	NTV2_NUM_TCDESTS				NTV2_MAX_NUM_TIMECODE_INDEXES	///< @deprecated	Use NTV2_MAX_NUM_TIMECODE_INDEXES instead.
#endif	//	!defined(NTV2_DEPRECATE)


typedef enum
{
	NTV2_HDMI_V2_HDSD_BIDIRECTIONAL,
	NTV2_HDMI_V2_4K_CAPTURE,
	NTV2_HDMI_V2_4K_PLAYBACK,
	NTV2_HDMI_V2_MODE_INVALID
} NTV2HDMIV2Mode;

typedef enum
{
	VPIDVersion_0					= 0x0,	// deprecated
	VPIDVersion_1					= 0x1	// use this one
} VPIDVersion;

typedef enum
{
	VPIDStandard_Unknown						= 0x00,	// default
	VPIDStandard_483_576						= 0x81,	// ntsc / pal
	VPIDStandard_483_576_DualLink				= 0x82,	// no matching ntv2 format
	VPIDStandard_483_576_360Mbs					= 0x82,	// no matching ntv2 format
	VPIDStandard_483_576_540Mbs					= 0x83,	// no matching ntv2 format
	VPIDStandard_720							= 0x84,	// 720 single link
	VPIDStandard_1080							= 0x85,	// 1080 single link
	VPIDStandard_483_576_1485Mbs				= 0x86,	// no matching ntv2 format
	VPIDStandard_1080_DualLink					= 0x87,	// 1080 dual link
	VPIDStandard_720_3Ga						= 0x88,	// no matching ntv2 format
	VPIDStandard_1080_3Ga						= 0x89,	// 1080p 50/5994/60
	VPIDStandard_1080_DualLink_3Gb				= 0x8A,	// 1080 dual link 3Gb
	VPIDStandard_720_3Gb						= 0x8B,	// 2 - 720 links
	VPIDStandard_1080_3Gb						= 0x8C,	// 2 - 1080 links
	VPIDStandard_483_576_3Gb					= 0x8D,	// no matching ntv2 format
	VPIDStandard_720_Stereo_3Gb					= 0x8E,	// 720 Stereo 3Gb
	VPIDStandard_1080_Stereo_3Gb				= 0x8F,	// 1080 Stereo 3Gb
	VPIDStandard_1080_QuadLink					= 0x90,	// 1080 quad link 10Gbs
	VPIDStandard_720_Stereo_3Ga					= 0x91,	// 720 Stereo 3Ga
	VPIDStandard_1080_Stereo_3Ga				= 0x92,	// 1080 Stereo 3Ga
	VPIDStandard_1080_Stereo_DualLink_3Gb		= 0x93,	// 1080 Stereo dual link 3Gb
	VPIDStandard_1080_Dual_3Ga					= 0x94,	// 1080 dual link 3Ga
	VPIDStandard_1080_Dual_3Gb					= 0x95,	// 1080 dual link 3Gb
	VPIDStandard_2160_DualLink					= 0x96,	// 2160 dual link
	VPIDStandard_2160_QuadLink_3Ga				= 0x97,	// 2160 quad link 3Ga
	VPIDStandard_2160_QuadDualLink_3Gb			= 0x98,	// 2160 quad link 3Gb
	VPIDStandard_1080_Stereo_Quad_3Ga			= 0x99,	// 1080 Stereo Quad 3Ga
	VPIDStandard_1080_Stereo_Quad_3Gb			= 0x9A,	// 1080 Stereo Quad 3Gb
	VPIDStandard_2160_Stereo_Quad_3Gb			= 0x9B,	// 2160 Stereo Quad 3Gb
	VPIDStandard_1080_OctLink					= 0xA0,	// 1080 oct link 10Gbs
	VPIDStandard_UHDTV1_Single_DualLink_10Gb	= 0xA1,	// UHDTV1 single or dual link 10Gbs
	VPIDStandard_UHDTV2_Quad_OctaLink_10Gb		= 0xA2,	// UHDTV2 quad or octa link 10Gbs
	VPIDStandard_UHDTV1_MultiLink_10Gb			= 0xA5,	// UHDTV1 multi link 10Gbs
	VPIDStandard_UHDTV2_MultiLink_10Gb			= 0xA6,	// UHDTV2 multi link 10Gbs
	VPIDStandard_VC2							= 0xB0,	// VC2 compressed 1.5Gbs
	VPIDStandard_720_1080_Stereo				= 0xB1,	// 720 and 1080 Stereo dusl 1.5Gb
	VPIDStandard_VC2_Level65_270Mbs				= 0xB2,	// VC2 level 65 commpressed 270Mbs
	VPIDStandard_4K_DCPIF_FSW709_10Gbs			= 0xB3,	// 4K Digital Cinematography Production Image Formats FS/709 10Gbs
	VPIDStandard_FT_2048x1556_Dual				= 0xB4,	// Film Transfer 2048x1556 dual 1.5Gbs
	VPIDStandard_FT_2048x1556_3Gb				= 0xB5,	// Film Tramsfer 1048x1556 3Gb
	VPIDStandard_2160_Single_6Gb				= 0xC0,	// 2160 single link 6Gb
	VPIDStandard_1080_Single_6Gb				= 0xC1,	// 1080 single link 6Gb
	VPIDStandard_1080_AFR_Single_6Gb			= 0xC2,	// 1080 additional frame rates single link 6Gb
	VPIDStandard_2160_Single_12Gb				= 0xCE,	// 2160 single link 12Gb
	VPIDStandard_1080_10_12_AFR_Single_12Gb		= 0xCF,	// 1080 10 bit or 12 bit additional frame rates single link 12Gb
	VPIDStandard_4320_DualLink_12Gb				= 0xD0, // 4320 dual link 12Gb ST2802-11 Mode 1
	VPIDStandard_2160_DualLink_12Gb				= 0xD1, // 2160 RGB dual link 12Gb ST2082-11 Mode 2
	VPIDStandard_4320_QuadLink_12Gb				= 0xD2 // 4320 quad link 12Gb ST2082-12 Mode 1
} VPIDStandard;

typedef enum
{
	VPIDPictureRate_None			= 0x0,
	VPIDPictureRate_Reserved1		= 0x1,
	VPIDPictureRate_2398			= 0x2,
	VPIDPictureRate_2400			= 0x3,
	VPIDPictureRate_4795			= 0x4,
	VPIDPictureRate_2500			= 0x5,
	VPIDPictureRate_2997			= 0x6,
	VPIDPictureRate_3000			= 0x7,
	VPIDPictureRate_4800			= 0x8,
	VPIDPictureRate_5000			= 0x9,
	VPIDPictureRate_5994			= 0xa,
	VPIDPictureRate_6000			= 0xb,
	VPIDPictureRate_ReservedC		= 0xc,
	VPIDPictureRate_ReservedD		= 0xd,
	VPIDPictureRate_ReservedE		= 0xe,
	VPIDPictureRate_ReservedF		= 0xf
} VPIDPictureRate;

typedef enum
{
	VPIDSampling_YUV_422			= 0x0,
	VPIDSampling_YUV_444			= 0x1,
	VPIDSampling_GBR_444			= 0x2,
	VPIDSampling_YUV_420			= 0x3,
	VPIDSampling_YUVA_4224			= 0x4,
	VPIDSampling_YUVA_4444			= 0x5,
	VPIDSampling_GBRA_4444			= 0x6,
	VPIDSampling_Reserved7			= 0x7,
	VPIDSampling_YUVD_4224			= 0x8,
	VPIDSampling_YUVD_4444			= 0x9,
	VPIDSampling_GBRD_4444			= 0xa,
	VPIDSampling_ReservedB			= 0xb,
	VPIDSampling_ReservedC			= 0xc,
	VPIDSampling_ReservedD			= 0xd,
	VPIDSampling_ReservedE			= 0xe,
	VPIDSampling_XYZ_444			= 0xf
} VPIDSampling;

typedef enum
{
	VPIDChannel_1				= 0x0,
	VPIDChannel_2				= 0x1,
	VPIDChannel_3				= 0x2,
	VPIDChannel_4				= 0x3,
	VPIDChannel_5				= 0x4,
	VPIDChannel_6				= 0x5,
	VPIDChannel_7				= 0x6,
	VPIDChannel_8				= 0x7
} VPIDChannel;

typedef enum
{
	VPIDDynamicRange_100		= 0x0,
	VPIDDynamicRange_200		= 0x1,
	VPIDDynamicRange_400		= 0x2,
	VPIDDynamicRange_Reserved3	= 0x3
} VPIDDynamicRange;

typedef enum
{
	VPIDBitDepth_8				= 0x0,
	VPIDBitDepth_10_Full		= 0x0,
	VPIDBitDepth_10				= 0x1,
	VPIDBitDepth_12				= 0x2,
	VPIDBitDepth_12_Full		= 0x3
} VPIDBitDepth;

typedef enum
{
	VPIDLink_1					= 0x0,
	VPIDLink_2					= 0x1,
	VPIDLink_3					= 0x2,
	VPIDLink_4					= 0x3,
	VPIDLink_5					= 0x4,
	VPIDLink_6					= 0x5,
	VPIDLink_7					= 0x6,
	VPIDLink_8					= 0x7
} VPIDLink;

typedef enum
{
	VPIDAudio_Unknown			= 0x0,
	VPIDAudio_Copied			= 0x1,
	VPIDAudio_Additional		= 0x2,
	VPIDAudio_Reserved			= 0x3
} VPIDAudio;

/**
	@brief	These enum values identify RS-422 serial port parity configuration.
	@see	CNTV2Card::GetRS422Parity, CNTV2Card::SetRS422Parity
**/
typedef enum
{
	NTV2_RS422_NO_PARITY		= 0x0,	///< @brief	No parity
	NTV2_RS422_ODD_PARITY		= 0x1,	///< @brief	Odd parity -- this is the power-up default
	NTV2_RS422_EVEN_PARITY		= 0x2,	///< @brief	Even parity
	NTV2_RS422_PARITY_INVALID	= 0xFF
} NTV2_RS422_PARITY;

#define	NTV2_IS_VALID_RS422_PARITY(_x_)		((_x_) == NTV2_RS422_NO_PARITY  ||  (_x_) == NTV2_RS422_ODD_PARITY  ||  (_x_) == NTV2_RS422_EVEN_PARITY)


/**
	@brief	These enum values identify RS-422 serial port baud rate configuration.
	@see	CNTV2Card::GetRS422BaudRate, CNTV2Card::SetRS422BaudRate
**/
typedef enum
{
	NTV2_RS422_BAUD_RATE_38400	= 38400,	///< @brief	38400 baud -- this is the power-up default
	NTV2_RS422_BAUD_RATE_19200	= 19200,	///< @brief	19200 baud
	NTV2_RS422_BAUD_RATE_9600	=  9600,	///< @brief	9600 baud
	NTV2_RS422_BAUD_RATE_INVALID = 0
} NTV2_RS422_BAUD_RATE;

#define	NTV2_IS_VALID_RS422_BAUD_RATE(_x_)	((_x_) == NTV2_RS422_BAUD_RATE_38400  ||  (_x_) == NTV2_RS422_BAUD_RATE_19200  ||  (_x_) == NTV2_RS422_BAUD_RATE_9600)

typedef enum
{
	NTV2_HDMI_YC422	= 0,				///< @brief			Specifies YCbCr 4:2:2 color space.
	NTV2_HDMI_RGB	= 1,				///< @brief			Specifies RGB color space.
	NTV2_HDMI_YC420	= 2,				///< @brief			Specifies YCbCr 4:2:0 color space.
	NTV2_HDMI_422	= NTV2_HDMI_YC422,	///< @deprecated	Use NTV2_HDMI_YC422 instead.
	NTV2_HDMI_420	= NTV2_HDMI_YC420,	///< @deprecated	Use NTV2_HDMI_YC420 instead.
	NTV2_NUM_HDMICOLORSPACEVALS	= 3,
	NTV2_HDMI_INVALID			= NTV2_NUM_HDMICOLORSPACEVALS
} NTV2HDMISampleStructure;

#define	NTV2_IS_VALID_HDMI_SAMPLE_STRUCT(_x_)	((_x_) >= NTV2_HDMI_YC422  &&  (_x_) < NTV2_NUM_HDMICOLORSPACEVALS)

typedef enum
{
	NTV2_FanSpeed_Invalid	= 0,
	NTV2_FanSpeed_Low		= 50,		NTV2_FanSpeed_Min	= NTV2_FanSpeed_Low,
	NTV2_FanSpeed_Medium	= 75,
	NTV2_FanSpeed_High		= 100,		NTV2_FanSpeed_Max	= NTV2_FanSpeed_High
} NTV2FanSpeed;

#define	NTV2_IS_VALID_FAN_SPEED(_x_)	((_x_) == NTV2_FanSpeed_Low || (_x_) == NTV2_FanSpeed_Medium || (_x_) == NTV2_FanSpeed_High)

typedef enum
{
	NTV2DieTempScale_Celsius,
	NTV2DieTempScale_Fahrenheit,
	NTV2DieTempScale_Kelvin,
	NTV2DieTempScale_Rankine,
	NTV2DieTempScale_INVALID
} NTV2DieTempScale;

#define	NTV2_IS_VALID_DIETEMP_SCALE(_x_)	((_x_) >= NTV2DieTempScale_Celsius  &&  (_x_) < NTV2DieTempScale_INVALID)

/**
	@brief	These enumerations identify the various ancillary data regions located at the bottom
			of each frame buffer on the NTV2 device.
	@note	These are only applicable to devices that support custom anc insertion/extraction
			(see ::NTV2DeviceCanDoCustomAnc). Also see \ref anccapture  and \ref ancplayout .
**/
typedef enum
{
	NTV2_AncRgn_Field1,		///< @brief	Identifies the "normal" Field 1 ancillary data region.
	NTV2_AncRgn_Field2,		///< @brief	Identifies the "normal" Field 2 ancillary data region.
	NTV2_AncRgn_MonField1,	///< @brief	Identifies the "monitor" or "auxiliary" Field 1 ancillary data region.
	NTV2_AncRgn_MonField2,	///< @brief	Identifies the "monitor" or "auxiliary" Field 2 ancillary data region.
	NTV2_MAX_NUM_AncRgns,
	NTV2_AncRgn_FIRST	= NTV2_AncRgn_Field1,
	NTV2_AncRgn_LAST	= NTV2_AncRgn_MonField2,
	NTV2_AncRgn_INVALID	= NTV2_MAX_NUM_AncRgns,
	NTV2_AncRgn_All = 0xFFFF	///< @brief	Identifies "all" ancillary data regions.
} NTV2AncillaryDataRegion, NTV2AncDataRgn;

#define	NTV2_IS_ALL_ANC_RGNS(_x_)		((_x_) == NTV2_AncRgn_All)
#define	NTV2_IS_VALID_ANC_RGN(_x_)		(NTV2_IS_ALL_ANC_RGNS(_x_)  ||  ((_x_) >= NTV2_AncRgn_Field1  &&  (_x_) < NTV2_MAX_NUM_AncRgns))
#define	NTV2_IS_NORMAL_ANC_RGN(_x_)		((_x_) == NTV2_AncRgn_Field1  ||  (_x_) == NTV2_AncRgn_Field2)
#define	NTV2_IS_MONITOR_ANC_RGN(_x_)	((_x_) == NTV2_AncRgn_MonField1  ||  (_x_) == NTV2_AncRgn_MonField2)


typedef enum
{
	NTV2_VPID_TC_SDR_TV,
	NTV2_VPID_TC_HLG,
	NTV2_VPID_TC_PQ,
	NTV2_VPID_TC_Unspecified
} NTV2VPIDTransferCharacteristics, NTV2VPIDXferChars, NTV2HDRXferChars;
		
typedef enum
{
	NTV2_VPID_Color_Rec709,
	NTV2_VPID_Color_Reserved,
	NTV2_VPID_Color_UHDTV,
	NTV2_VPID_Color_Unknown
} NTV2VPIDColorimetry, NTV2HDRColorimetry;
		
typedef enum
{
	NTV2_VPID_Luminance_YCbCr,
	NTV2_VPID_Luminance_ICtCp
} NTV2VPIDLuminance, NTV2HDRLuminance;
		
typedef enum
{
	NTV2_VPID_Range_Narrow,
	NTV2_VPID_Range_Full
} NTV2VPIDRGBRange;


#if !defined (NTV2_DEPRECATE)
	typedef		NTV2AnalogType						NTV2K2AnalogType;					///< @deprecated	Use NTV2AnalogType instead.
	typedef		NTV2Audio2ChannelSelect				NTV2K2Audio2ChannelSelect;			///< @deprecated	Use NTV2Audio2ChannelSelect instead.
	typedef		NTV2Audio4ChannelSelect				NTV2K2Audio4ChannelSelect;			///< @deprecated	Use NTV2Audio4ChannelSelect instead.
	typedef		NTV2Audio8ChannelSelect				NTV2K2Audio8ChannelSelect;			///< @deprecated	Use NTV2Audio8ChannelSelect instead.
	typedef		NTV2AudioMapSelect					NTV2K2AudioMapSelect;				///< @deprecated	Use NTV2AudioMapSelect instead.
	typedef		NTV2AudioMonitorSelect				NTV2K2AudioMonitorSelect;			///< @deprecated	Use NTV2AudioMonitorSelect instead.
	typedef		NTV2BitFileType						NTV2K2BitFileType;					///< @deprecated	Use NTV2BitFileType instead.
	typedef		NTV2BreakoutType					NTV2K2BreakoutType;					///< @deprecated	Use NTV2BreakoutType instead.
	typedef		NTV2ColorSpaceMatrixType			NTV2K2ColorSpaceMatrixType;			///< @deprecated	Use NTV2ColorSpaceMatrixType instead.
	typedef		NTV2ColorSpaceMode					NTV2K2ColorSpaceMode;				///< @deprecated	Use NTV2ColorSpaceMode instead.
	typedef		NTV2ConversionMode					NTV2K2ConversionMode;				///< @deprecated	Use NTV2ConversionMode instead.
	typedef		NTV2CrosspointID					NTV2CrosspointSelections;			///< @deprecated	Use NTV2CrosspointID instead.
	typedef		NTV2CrosspointSelections			NTV2K2CrosspointSelections;			///< @deprecated	Use NTV2CrosspointID instead.
	typedef		NTV2DownConvertMode					NTV2K2DownConvertMode;				///< @deprecated	Use NTV2DownConvertMode instead.
	typedef		NTV2EncodeAsPSF						NTV2K2EncodeAsPSF;					///< @deprecated	Use NTV2EncodeAsPSF instead.
	typedef		NTV2FrameBufferQuality				NTV2K2FrameBufferQuality;			///< @deprecated	Use NTV2FrameBufferQuality instead.
	typedef		NTV2Framesize						NTV2K2Framesize;					///< @deprecated	Use NTV2Framesize instead.
	typedef		NTV2FrameSyncSelect					NTV2K2FrameSyncSelect;				///< @deprecated	Use NTV2FrameSyncSelect instead.
	typedef		NTV2InputAudioSelect				NTV2K2InputAudioSelect;				///< @deprecated	Use NTV2InputAudioSelect instead.
	typedef		NTV2InputVideoSelect				NTV2K2InputVideoSelect;				///< @deprecated	Use NTV2InputVideoSelect instead.
	typedef		NTV2IsoConvertMode					NTV2K2IsoConvertMode;				///< @deprecated	Use NTV2IsoConvertMode instead.
	typedef		NTV2OutputVideoSelect				NTV2K2OutputVideoSelect;			///< @deprecated	Use NTV2OutputVideoSelect instead.
	typedef		NTV2QuarterSizeExpandMode			NTV2K2QuarterSizeExpandMode;		///< @deprecated	Use NTV2QuarterSizeExpandMode instead.
	typedef		NTV2RegisterWriteMode				NTV2RegWriteMode;					///< @deprecated	Use NTV2RegisterWriteMode instead.
	typedef		NTV2RGBBlackRange					NTV2K2RGBBlackRange;				///< @deprecated	Use NTV2RGBBlackRange instead.
	typedef		NTV2SDIInputFormatSelect			NTV2K2SDIInputFormatSelect;			///< @deprecated	Use NTV2SDIInputFormatSelect instead.
	typedef		NTV2Stereo3DMode					NTV2K2Stereo3DMode;					///< @deprecated	Use NTV2Stereo3DMode instead.
	typedef		NTV2UpConvertMode					NTV2K2UpConvertMode;				///< @deprecated	Use NTV2UpConvertMode instead.
	typedef		NTV2VideoDACMode					NTV2K2VideoDACMode;					///< @deprecated	Use NTV2VideoDACMode instead.

	//	NTV2CrosspointSelections
	#define		NTV2K2_XptBlack						NTV2_XptBlack						///< @deprecated	Use NTV2_XptBlack instead.
	#define		NTV2K2_XptSDIIn1					NTV2_XptSDIIn1						///< @deprecated	Use NTV2_XptSDIIn1 instead.
	#define		NTV2K2_XptSDIIn1DS2					NTV2_XptSDIIn1DS2					///< @deprecated	Use NTV2_XptSDIIn1DS2 instead.
	#define		NTV2K2_XptSDIIn2					NTV2_XptSDIIn2						///< @deprecated	Use NTV2_XptSDIIn2 instead.
	#define		NTV2K2_XptSDIIn2DS2					NTV2_XptSDIIn2DS2					///< @deprecated	Use NTV2_XptSDIIn2DS2 instead.
	#define		NTV2K2_XptLUT1YUV					NTV2_XptLUT1YUV						///< @deprecated	Use NTV2_XptLUT1YUV instead.
	#define		NTV2K2_XptCSCYUV					NTV2_XptCSC1VidYUV					///< @deprecated	Use NTV2_XptCSC1VidYUV instead.
	#define		NTV2K2_XptCSC1VidYUV				NTV2_XptCSC1VidYUV					///< @deprecated	Use NTV2_XptCSC1VidYUV instead.
	#define		NTV2K2_XptConversionModule			NTV2_XptConversionModule			///< @deprecated	Use NTV2_XptConversionModule instead.
	#define		NTV2K2_XptCompressionModule			NTV2_XptCompressionModule			///< @deprecated	Use NTV2_XptCompressionModule instead.
	#define		NTV2K2_XptFrameBuffer1YUV			NTV2_XptFrameBuffer1YUV				///< @deprecated	Use NTV2_XptFrameBuffer1YUV instead.
	#define		NTV2K2_XptFrameSync1YUV				NTV2_XptFrameSync1YUV				///< @deprecated	Use NTV2_XptFrameSync1YUV instead.
	#define		NTV2K2_XptFrameSync2YUV				NTV2_XptFrameSync2YUV				///< @deprecated	Use NTV2_XptFrameSync2YUV instead.
	#define		NTV2K2_XptDuallinkOut				NTV2_XptDuallinkOut1				///< @deprecated	Use NTV2_XptDuallinkOut1 instead.
	#define		NTV2K2_XptDuallinkOutDS2			NTV2_XptDuallinkOut1DS2				///< @deprecated	Use NTV2_XptDuallinkOut1DS2 instead.
	#define		NTV2K2_XptDuallinkOut2				NTV2_XptDuallinkOut2				///< @deprecated	Use NTV2_XptDuallinkOut2 instead.
	#define		NTV2K2_XptDuallinkOut2DS2			NTV2_XptDuallinkOut2DS2				///< @deprecated	Use NTV2_XptDuallinkOut2DS2 instead.
	#define		NTV2K2_XptDuallinkOut3				NTV2_XptDuallinkOut3				///< @deprecated	Use NTV2_XptDuallinkOut3 instead.
	#define		NTV2K2_XptDuallinkOut3DS2			NTV2_XptDuallinkOut3DS2				///< @deprecated	Use NTV2_XptDuallinkOut3DS2 instead.
	#define		NTV2K2_XptDuallinkOut4				NTV2_XptDuallinkOut4				///< @deprecated	Use NTV2_XptDuallinkOut4 instead.
	#define		NTV2K2_XptDuallinkOut4DS2			NTV2_XptDuallinkOut4DS2				///< @deprecated	Use NTV2_XptDuallinkOut4DS2 instead.
	#define		NTV2K2_XptAlphaOut					NTV2_XptAlphaOut					///< @deprecated	Use NTV2_XptAlphaOut instead.
	#define		NTV2K2_XptAnalogIn					NTV2_XptAnalogIn					///< @deprecated	Use NTV2_XptAnalogIn instead.
	#define		NTV2K2_XptHDMIIn					NTV2_XptHDMIIn1						///< @deprecated	Use NTV2_XptHDMIIn1 instead.
	#define		NTV2K2_XptHDMIInQ2					NTV2_XptHDMIIn1Q2					///< @deprecated	Use NTV2_XptHDMIIn1Q2 instead.
	#define		NTV2K2_XptHDMIInQ3					NTV2_XptHDMIIn1Q3					///< @deprecated	Use NTV2_XptHDMIIn1Q3 instead.
	#define		NTV2K2_XptHDMIInQ4					NTV2_XptHDMIIn1Q4					///< @deprecated	Use NTV2_XptHDMIIn1Q4 instead.
	#define		NTV2K2_XptHDMIInRGB					NTV2_XptHDMIIn1RGB					///< @deprecated	Use NTV2_XptHDMIIn1RGB instead.
	#define		NTV2K2_XptHDMIInQ2RGB				NTV2_XptHDMIIn1Q2RGB				///< @deprecated	Use NTV2_XptHDMIIn1Q2RGB instead.
	#define		NTV2K2_XptHDMIInQ3RGB				NTV2_XptHDMIIn1Q3RGB				///< @deprecated	Use NTV2_XptHDMIIn1Q3RGB instead.
	#define		NTV2K2_XptHDMIInQ4RGB				NTV2_XptHDMIIn1Q4RGB				///< @deprecated	Use NTV2_XptHDMIIn1Q4RGB instead.
	#define		NTV2K2_XptFS1SecondConverter		NTV2_XptFS1SecondConverter			///< @deprecated	This is obsolete.
	#define		NTV2KS_XptFS1ProcAmp				NTV2_XptFS1ProcAmp					///< @deprecated	This is obsolete.
	#define		NTV2KS_XptFS1TestSignalGenerator	NTV2_XptFS1TestSignalGenerator		///< @deprecated	This is obsolete.
	#define		NTV2K2_XptDuallinkIn				NTV2_XptDuallinkIn1					///< @deprecated	Use NTV2_XptDuallinkIn1 instead.
	#define		NTV2K2_XptDuallinkIn2				NTV2_XptDuallinkIn2					///< @deprecated	Use NTV2_XptDuallinkIn2 instead.
	#define		NTV2K2_XptDuallinkIn3				NTV2_XptDuallinkIn3					///< @deprecated	Use NTV2_XptDuallinkIn3 instead.
	#define		NTV2K2_XptDuallinkIn4				NTV2_XptDuallinkIn4					///< @deprecated	Use NTV2_XptDuallinkIn4 instead.
	#define		NTV2K2_XptLUT						NTV2_XptLUT1Out						///< @deprecated	Use NTV2_XptLUT1Out instead.
	#define		NTV2K2_XptLUT1RGB					NTV2_XptLUT1Out						///< @deprecated	Use NTV2_XptLUT1Out instead.
	#define		NTV2K2_XptCSCRGB					NTV2_XptCSC1VidRGB					///< @deprecated	Use NTV2_XptCSC1VidRGB instead.
	#define		NTV2K2_XptCSC1VidRGB				NTV2_XptCSC1VidRGB					///< @deprecated	Use NTV2_XptCSC1VidRGB instead.
	#define		NTV2K2_XptFrameBuffer1RGB			NTV2_XptFrameBuffer1RGB				///< @deprecated	Use NTV2_XptFrameBuffer1RGB instead.
	#define		NTV2K2_XptFrameSync1RGB				NTV2_XptFrameSync1RGB				///< @deprecated	Use NTV2_XptFrameSync1RGB instead.
	#define		NTV2K2_XptFrameSync2RGB				NTV2_XptFrameSync2RGB				///< @deprecated	Use NTV2_XptFrameSync2RGB instead.
	#define		NTV2K2_XptLUT2RGB					NTV2_XptLUT2Out						///< @deprecated	Use NTV2_XptLUT2Out instead.
	#define		NTV2K2_XptCSC1KeyYUV				NTV2_XptCSC1KeyYUV					///< @deprecated	Use NTV2_XptCSC1KeyYUV instead.
	#define		NTV2K2_XptFrameBuffer2YUV			NTV2_XptFrameBuffer2YUV				///< @deprecated	Use NTV2_XptFrameBuffer2YUV instead.
	#define		NTV2K2_XptFrameBuffer2RGB			NTV2_XptFrameBuffer2RGB				///< @deprecated	Use NTV2_XptFrameBuffer2RGB instead.
	#define		NTV2K2_XptCSC2VidYUV				NTV2_XptCSC2VidYUV					///< @deprecated	Use NTV2_XptCSC2VidYUV instead.
	#define		NTV2K2_XptCSC2VidRGB				NTV2_XptCSC2VidRGB					///< @deprecated	Use NTV2_XptCSC2VidRGB instead.
	#define		NTV2K2_XptCSC2KeyYUV				NTV2_XptCSC2KeyYUV					///< @deprecated	Use NTV2_XptCSC2KeyYUV instead.
	#define		NTV2K2_XptMixerVidYUV				NTV2_XptMixerVidYUV					///< @deprecated	Use NTV2_XptMixerVidYUV instead.
	#define		NTV2K2_XptMixerKeyYUV				NTV2_XptMixerKeyYUV					///< @deprecated	Use NTV2_XptMixerKeyYUV instead.
	#define		NTV2K2_XptWaterMarkerRGB			NTV2_XptWaterMarkerRGB				///< @deprecated	Use NTV2_XptWaterMarkerRGB instead.
	#define		NTV2K2_XptWaterMarkerYUV			NTV2_XptWaterMarkerYUV				///< @deprecated	Use NTV2_XptWaterMarkerYUV instead.
	#define		NTV2K2_XptWaterMarker2RGB			NTV2_XptWaterMarker2RGB				///< @deprecated	Use NTV2_XptWaterMarker2RGB instead.
	#define		NTV2K2_XptWaterMarker2YUV			NTV2_XptWaterMarker2YUV				///< @deprecated	Use NTV2_XptWaterMarker2YUV instead.
	#define		NTV2K2_XptIICTRGB					NTV2_XptIICTRGB						///< @deprecated	Use NTV2_XptIICTRGB instead.
	#define		NTV2K2_XptIICT2RGB					NTV2_XptIICT2RGB					///< @deprecated	Use NTV2_XptIICT2RGB instead.
	#define		NTV2K2_XptTestPatternYUV			NTV2_XptTestPatternYUV				///< @deprecated	Use NTV2_XptTestPatternYUV instead.
	#define		NTV2K2_XptDCIMixerVidYUV			NTV2_XptDCIMixerVidYUV				///< @deprecated	Use NTV2_XptDCIMixerVidYUV instead.
	#define		NTV2K2_XptDCIMixerVidRGB			NTV2_XptDCIMixerVidRGB				///< @deprecated	Use NTV2_XptDCIMixerVidRGB instead.
	#define		NTV2K2_XptMixer2VidYUV				NTV2_XptMixer2VidYUV				///< @deprecated	Use NTV2_XptMixer2VidYUV instead.
	#define		NTV2K2_XptMixer2KeyYUV				NTV2_XptMixer2KeyYUV				///< @deprecated	Use NTV2_XptMixer2KeyYUV instead.
	#define		NTV2K2_XptStereoCompressorOut		NTV2_XptStereoCompressorOut			///< @deprecated	Use NTV2_XptStereoCompressorOut instead.
	#define		NTV2K2_XptLUT3Out					NTV2_XptLUT3Out						///< @deprecated	Use NTV2_XptLUT3Out instead.
	#define		NTV2K2_XptLUT4Out					NTV2_XptLUT4Out						///< @deprecated	Use NTV2_XptLUT4Out instead.
	#define		NTV2K2_XptFrameBuffer3YUV			NTV2_XptFrameBuffer3YUV				///< @deprecated	Use NTV2_XptFrameBuffer3YUV instead.
	#define		NTV2K2_XptFrameBuffer3RGB			NTV2_XptFrameBuffer3RGB				///< @deprecated	Use NTV2_XptFrameBuffer3RGB instead.
	#define		NTV2K2_XptFrameBuffer4YUV			NTV2_XptFrameBuffer4YUV				///< @deprecated	Use NTV2_XptFrameBuffer4YUV instead.
	#define		NTV2K2_XptFrameBuffer4RGB			NTV2_XptFrameBuffer4RGB				///< @deprecated	Use NTV2_XptFrameBuffer4RGB instead.
	#define		NTV2K2_XptSDIIn3					NTV2_XptSDIIn3						///< @deprecated	Use NTV2_XptSDIIn3 instead.
	#define		NTV2K2_XptSDIIn3DS2					NTV2_XptSDIIn3DS2					///< @deprecated	Use NTV2_XptSDIIn3DS2 instead.
	#define		NTV2K2_XptSDIIn4					NTV2_XptSDIIn4						///< @deprecated	Use NTV2_XptSDIIn4 instead.
	#define		NTV2K2_XptSDIIn4DS2					NTV2_XptSDIIn4DS2					///< @deprecated	Use NTV2_XptSDIIn4DS2 instead.
	#define		NTV2K2_XptCSC3VidYUV				NTV2_XptCSC3VidYUV					///< @deprecated	Use NTV2_XptCSC3VidYUV instead.
	#define		NTV2K2_XptCSC3VidRGB				NTV2_XptCSC3VidRGB					///< @deprecated	Use NTV2_XptCSC3VidRGB instead.
	#define		NTV2K2_XptCSC3KeyYUV				NTV2_XptCSC3KeyYUV					///< @deprecated	Use NTV2_XptCSC3KeyYUV instead.
	#define		NTV2K2_XptCSC4VidYUV				NTV2_XptCSC4VidYUV					///< @deprecated	Use NTV2_XptCSC4VidYUV instead.
	#define		NTV2K2_XptCSC4VidRGB				NTV2_XptCSC4VidRGB					///< @deprecated	Use NTV2_XptCSC4VidRGB instead.
	#define		NTV2K2_XptCSC4KeyYUV				NTV2_XptCSC4KeyYUV					///< @deprecated	Use NTV2_XptCSC4KeyYUV instead.
	#define		NTV2K2_XptCSC5VidYUV				NTV2_XptCSC5VidYUV					///< @deprecated	Use NTV2_XptCSC5VidYUV instead.
	#define		NTV2K2_XptCSC5VidRGB				NTV2_XptCSC5VidRGB					///< @deprecated	Use NTV2_XptCSC5VidRGB instead.
	#define		NTV2K2_XptCSC5KeyYUV				NTV2_XptCSC5KeyYUV					///< @deprecated	Use NTV2_XptCSC5KeyYUV instead.
	#define		NTV2K2_XptLUT5Out					NTV2_XptLUT5Out						///< @deprecated	Use NTV2_XptLUT5Out instead.
	#define		NTV2K2_XptDuallinkOut5				NTV2_XptDuallinkOut5				///< @deprecated	Use NTV2_XptDuallinkOut5 instead.
	#define		NTV2K2_XptDuallinkOut5DS2			NTV2_XptDuallinkOut5DS2				///< @deprecated	Use NTV2_XptDuallinkOut5DS2 instead.
	#define		NTV2K2_Xpt4KDownConverterOut		NTV2_Xpt4KDownConverterOut			///< @deprecated	Use NTV2_Xpt4KDownConverterOut instead.
	#define		NTV2K2_Xpt4KDownConverterOutRGB		NTV2_Xpt4KDownConverterOutRGB		///< @deprecated	Use NTV2_Xpt4KDownConverterOutRGB instead.
	#define		NTV2K2_XptFrameBuffer5YUV			NTV2_XptFrameBuffer5YUV				///< @deprecated	Use NTV2_Xpt4KDownConverterOutRGB instead.
	#define		NTV2K2_XptFrameBuffer5RGB			NTV2_XptFrameBuffer5RGB				///< @deprecated	Use NTV2_XptFrameBuffer5YUV instead.
	#define		NTV2K2_XptFrameBuffer6YUV			NTV2_XptFrameBuffer6YUV				///< @deprecated	Use NTV2_XptFrameBuffer5RGB instead.
	#define		NTV2K2_XptFrameBuffer6RGB			NTV2_XptFrameBuffer6RGB				///< @deprecated	Use NTV2_XptFrameBuffer6YUV instead.
	#define		NTV2K2_XptFrameBuffer7YUV			NTV2_XptFrameBuffer7YUV				///< @deprecated	Use NTV2_XptFrameBuffer6RGB instead.
	#define		NTV2K2_XptFrameBuffer7RGB			NTV2_XptFrameBuffer7RGB				///< @deprecated	Use NTV2_XptFrameBuffer7YUV instead.
	#define		NTV2K2_XptFrameBuffer8YUV			NTV2_XptFrameBuffer8YUV				///< @deprecated	Use NTV2_XptFrameBuffer7RGB instead.
	#define		NTV2K2_XptFrameBuffer8RGB			NTV2_XptFrameBuffer8RGB				///< @deprecated	Use NTV2_XptFrameBuffer8YUV instead.
	#define		NTV2K2_XptSDIIn5					NTV2_XptSDIIn5						///< @deprecated	Use NTV2_XptFrameBuffer8RGB instead.
	#define		NTV2K2_XptSDIIn5DS2					NTV2_XptSDIIn5DS2					///< @deprecated	Use NTV2_XptSDIIn5 instead.
	#define		NTV2K2_XptSDIIn6					NTV2_XptSDIIn6						///< @deprecated	Use NTV2_XptSDIIn5DS2 instead.
	#define		NTV2K2_XptSDIIn6DS2					NTV2_XptSDIIn6DS2					///< @deprecated	Use NTV2_XptSDIIn6 instead.
	#define		NTV2K2_XptSDIIn7					NTV2_XptSDIIn7						///< @deprecated	Use NTV2_XptSDIIn6DS2 instead.
	#define		NTV2K2_XptSDIIn7DS2					NTV2_XptSDIIn7DS2					///< @deprecated	Use NTV2_XptSDIIn7DS2 instead.
	#define		NTV2K2_XptSDIIn8					NTV2_XptSDIIn8						///< @deprecated	Use NTV2_XptSDIIn8 instead.
	#define		NTV2K2_XptSDIIn8DS2					NTV2_XptSDIIn8DS2					///< @deprecated	Use NTV2_XptSDIIn8DS2 instead.
	#define		NTV2K2_XptCSC6VidYUV				NTV2_XptCSC6VidYUV					///< @deprecated	Use NTV2_XptCSC6VidYUV instead.
	#define		NTV2K2_XptCSC6VidRGB				NTV2_XptCSC6VidRGB					///< @deprecated	Use NTV2_XptCSC6VidRGB instead.
	#define		NTV2K2_XptCSC6KeyYUV				NTV2_XptCSC6KeyYUV					///< @deprecated	Use NTV2_XptCSC6KeyYUV instead.
	#define		NTV2K2_XptCSC7VidYUV				NTV2_XptCSC7VidYUV					///< @deprecated	Use NTV2_XptCSC7VidYUV instead.
	#define		NTV2K2_XptCSC7VidRGB				NTV2_XptCSC7VidRGB					///< @deprecated	Use NTV2_XptCSC7VidRGB instead.
	#define		NTV2K2_XptCSC7KeyYUV				NTV2_XptCSC7KeyYUV					///< @deprecated	Use NTV2_XptCSC7KeyYUV instead.
	#define		NTV2K2_XptCSC8VidYUV				NTV2_XptCSC8VidYUV					///< @deprecated	Use NTV2_XptCSC8VidYUV instead.
	#define		NTV2K2_XptCSC8VidRGB				NTV2_XptCSC8VidRGB					///< @deprecated	Use NTV2_XptCSC8VidRGB instead.
	#define		NTV2K2_XptCSC8KeyYUV				NTV2_XptCSC8KeyYUV					///< @deprecated	Use NTV2_XptCSC8KeyYUV instead.
	#define		NTV2K2_XptLUT6Out					NTV2_XptLUT6Out						///< @deprecated	Use NTV2_XptLUT6Out instead.
	#define		NTV2K2_XptLUT7Out					NTV2_XptLUT7Out						///< @deprecated	Use NTV2_XptLUT7Out instead.
	#define		NTV2K2_XptLUT8Out					NTV2_XptLUT8Out						///< @deprecated	Use NTV2_XptLUT8Out instead.
	#define		NTV2K2_XptDuallinkOut6				NTV2_XptDuallinkOut6				///< @deprecated	Use NTV2_XptDuallinkOut6 instead.
	#define		NTV2K2_XptDuallinkOut6DS2			NTV2_XptDuallinkOut6DS2				///< @deprecated	Use NTV2_XptDuallinkOut6DS2 instead.
	#define		NTV2K2_XptDuallinkOut7				NTV2_XptDuallinkOut7				///< @deprecated	Use NTV2_XptDuallinkOut7 instead.
	#define		NTV2K2_XptDuallinkOut7DS2			NTV2_XptDuallinkOut7DS2				///< @deprecated	Use NTV2_XptDuallinkOut7DS2 instead.
	#define		NTV2K2_XptDuallinkOut8				NTV2_XptDuallinkOut8				///< @deprecated	Use NTV2_XptDuallinkOut8 instead.
	#define		NTV2K2_XptDuallinkOut8DS2			NTV2_XptDuallinkOut8DS2				///< @deprecated	Use NTV2_XptDuallinkOut8DS2 instead.
	#define		NTV2K2_XptMixer3VidYUV				NTV2_XptMixer3VidYUV				///< @deprecated	Use NTV2_XptMixer3VidYUV instead.
	#define		NTV2K2_XptMixer3KeyYUV				NTV2_XptMixer3KeyYUV				///< @deprecated	Use NTV2_XptMixer3KeyYUV instead.
	#define		NTV2K2_XptMixer4VidYUV				NTV2_XptMixer4VidYUV				///< @deprecated	Use NTV2_XptMixer4VidYUV instead.
	#define		NTV2K2_XptMixer4KeyYUV				NTV2_XptMixer4KeyYUV				///< @deprecated	Use NTV2_XptMixer4KeyYUV instead.
	#define		NTV2K2_XptDuallinkIn5				NTV2_XptDuallinkIn5					///< @deprecated	Use NTV2_XptDuallinkIn5 instead.
	#define		NTV2K2_XptDuallinkIn6				NTV2_XptDuallinkIn6					///< @deprecated	Use NTV2_XptDuallinkIn6 instead.
	#define		NTV2K2_XptDuallinkIn7				NTV2_XptDuallinkIn7					///< @deprecated	Use NTV2_XptDuallinkIn7 instead.
	#define		NTV2K2_XptDuallinkIn8				NTV2_XptDuallinkIn8					///< @deprecated	Use NTV2_XptDuallinkIn8 instead.
	#define		NTV2K2_XptRuntimeCalc				NTV2_XptRuntimeCalc					///< @deprecated	Use NTV2_XptRuntimeCalc instead.
	#define		NTV2_XptMixerVidYUV					NTV2_XptMixer1VidYUV				///< @deprecated	Use NTV2_XptMixer1VidYUV instead.
	#define		NTV2_XptMixerKeyYUV					NTV2_XptMixer1KeyYUV				///< @deprecated	User NTV2_XptMixer1KeyYUV instead.

	//	NTV2BitFileType
	#define		NTV2K2_VideoProcBitFile				NTV2_VideoProcBitFile				///< @deprecated	Use NTV2_VideoProcBitFile instead.
	#define		NTV2K2_PCIBitFile					NTV2_PCIBitFile						///< @deprecated	Use NTV2_PCIBitFile instead.
	#define		NTV2K2_FWBitFile					NTV2_FWBitFile						///< @deprecated	Use NTV2_FWBitFile instead.
	#define		NTV2K2_Firmware						NTV2_Firmware						///< @deprecated	Use NTV2_Firmware instead.
	#define		NTV2K2_BitFile1						NTV2_BitFile1						///< @deprecated	Use NTV2_BitFile1 instead.
	#define		NTV2K2_BitFile2						NTV2_BitFile2						///< @deprecated	Use NTV2_BitFile2 instead.
	#define		NTV2K2_BitFile3						NTV2_BitFile3						///< @deprecated	Use NTV2_BitFile3 instead.
	#define		NTV2K2_BitFile4						NTV2_BitFile4						///< @deprecated	Use NTV2_BitFile4 instead.
	#define		NTV2K2_BitFile5						NTV2_BitFile5						///< @deprecated	Use NTV2_BitFile5 instead.
	#define		NTV2K2_BitFile6						NTV2_BitFile6						///< @deprecated	Use NTV2_BitFile6 instead.
	#define		NTV2K2_BitFile7						NTV2_BitFile7						///< @deprecated	Use NTV2_BitFile7 instead.
	#define		NTV2K2_BitFile8						NTV2_BitFile8						///< @deprecated	Use NTV2_BitFile8 instead.

	//	NTV2ConversionMode
	#define		NTV2K2_1080i_5994to525_5994			NTV2_1080i_5994to525_5994			///< @deprecated	Use NTV2_1080i_5994to525_5994 instead.
	#define		NTV2K2_1080i_2500to625_2500			NTV2_1080i_2500to625_2500			///< @deprecated	Use NTV2_1080i_2500to625_2500 instead.
	#define		NTV2K2_720p_5994to525_5994			NTV2_720p_5994to525_5994			///< @deprecated	Use NTV2_720p_5994to525_5994 instead.
	#define		NTV2K2_720p_5000to625_2500			NTV2_720p_5000to625_2500			///< @deprecated	Use NTV2_720p_5000to625_2500 instead.
	#define		NTV2K2_525_5994to1080i_5994			NTV2_525_5994to1080i_5994			///< @deprecated	Use NTV2_525_5994to1080i_5994 instead.
	#define		NTV2K2_525_5994to720p_5994			NTV2_525_5994to720p_5994			///< @deprecated	Use NTV2_525_5994to720p_5994 instead.
	#define		NTV2K2_625_2500to1080i_2500			NTV2_625_2500to1080i_2500			///< @deprecated	Use NTV2_625_2500to1080i_2500 instead.
	#define		NTV2K2_625_2500to720p_5000			NTV2_625_2500to720p_5000			///< @deprecated	Use NTV2_625_2500to720p_5000 instead.
	#define		NTV2K2_720p_5000to1080i_2500		NTV2_720p_5000to1080i_2500			///< @deprecated	Use NTV2_720p_5000to1080i_2500 instead.
	#define		NTV2K2_720p_5994to1080i_5994		NTV2_720p_5994to1080i_5994			///< @deprecated	Use NTV2_720p_5994to1080i_5994 instead.
	#define		NTV2K2_720p_6000to1080i_3000		NTV2_720p_6000to1080i_3000			///< @deprecated	Use NTV2_720p_6000to1080i_3000 instead.
	#define		NTV2K2_1080i2398to525_2398			NTV2_1080i2398to525_2398			///< @deprecated	Use NTV2_1080i2398to525_2398 instead.
	#define		NTV2K2_1080i2398to525_2997			NTV2_1080i2398to525_2997			///< @deprecated	Use NTV2_1080i2398to525_2997 instead.
	#define		NTV2K2_1080i2400to525_2400			NTV2_1080i2400to525_2400			///< @deprecated	Use NTV2_1080i2400to525_2400 instead.
	#define		NTV2K2_1080p2398to525_2398			NTV2_1080p2398to525_2398			///< @deprecated	Use NTV2_1080p2398to525_2398 instead.
	#define		NTV2K2_1080p2398to525_2997			NTV2_1080p2398to525_2997			///< @deprecated	Use NTV2_1080p2398to525_2997 instead.
	#define		NTV2K2_1080p2400to525_2400			NTV2_1080p2400to525_2400			///< @deprecated	Use NTV2_1080p2400to525_2400 instead.
	#define		NTV2K2_1080i_2500to720p_5000		NTV2_1080i_2500to720p_5000			///< @deprecated	Use NTV2_1080i_2500to720p_5000 instead.
	#define		NTV2K2_1080i_5994to720p_5994		NTV2_1080i_5994to720p_5994			///< @deprecated	Use NTV2_1080i_5994to720p_5994 instead.
	#define		NTV2K2_1080i_3000to720p_6000		NTV2_1080i_3000to720p_6000			///< @deprecated	Use NTV2_1080i_3000to720p_6000 instead.
	#define		NTV2K2_1080i_2398to720p_2398		NTV2_1080i_2398to720p_2398			///< @deprecated	Use NTV2_1080i_2398to720p_2398 instead.
	#define		NTV2K2_720p_2398to1080i_2398		NTV2_720p_2398to1080i_2398			///< @deprecated	Use NTV2_720p_2398to1080i_2398 instead.
	#define		NTV2K2_525_2398to1080i_2398			NTV2_525_2398to1080i_2398			///< @deprecated	Use NTV2_525_2398to1080i_2398 instead.
	#define		NTV2K2_525_5994to525_5994			NTV2_525_5994to525_5994				///< @deprecated	Use NTV2_525_5994to525_5994 instead.
	#define		NTV2K2_625_2500to625_2500			NTV2_625_2500to625_2500				///< @deprecated	Use NTV2_625_2500to625_2500 instead.
	#define		NTV2K2_525_5994to525psf_2997		NTV2_525_5994to525psf_2997			///< @deprecated	Use NTV2_525_5994to525psf_2997 instead.
	#define		NTV2K2_625_5000to625psf_2500		NTV2_625_5000to625psf_2500			///< @deprecated	Use NTV2_625_5000to625psf_2500 instead.
	#define		NTV2K2_1080i_5000to1080psf_2500		NTV2_1080i_5000to1080psf_2500		///< @deprecated	Use NTV2_1080i_5000to1080psf_2500 instead.
	#define		NTV2K2_1080i_5994to1080psf_2997		NTV2_1080i_5994to1080psf_2997		///< @deprecated	Use NTV2_1080i_5994to1080psf_2997 instead.
	#define		NTV2K2_1080i_6000to1080psf_3000		NTV2_1080i_6000to1080psf_3000		///< @deprecated	Use NTV2_1080i_6000to1080psf_3000 instead.
	#define		NTV2K2_1080p_3000to720p_6000		NTV2_1080p_3000to720p_6000			///< @deprecated	Use NTV2_1080p_3000to720p_6000 instead.
	#define		NTV2K2_1080psf_2398to1080i_5994		NTV2_1080psf_2398to1080i_5994		///< @deprecated	Use NTV2_1080psf_2398to1080i_5994 instead.
	#define		NTV2K2_1080psf_2400to1080i_3000		NTV2_1080psf_2400to1080i_3000		///< @deprecated	Use NTV2_1080psf_2400to1080i_3000 instead.
	#define		NTV2K2_1080psf_2500to1080i_2500		NTV2_1080psf_2500to1080i_2500		///< @deprecated	Use NTV2_1080psf_2500to1080i_2500 instead.
	#define		NTV2K2_1080p_2398to1080i_5994		NTV2_1080p_2398to1080i_5994			///< @deprecated	Use NTV2_1080p_2398to1080i_5994 instead.
	#define		NTV2K2_1080p_2400to1080i_3000		NTV2_1080p_2400to1080i_3000			///< @deprecated	Use NTV2_1080p_2400to1080i_3000 instead.
	#define		NTV2K2_1080p_2500to1080i_2500		NTV2_1080p_2500to1080i_2500			///< @deprecated	Use NTV2_1080p_2500to1080i_2500 instead.
	#define		NTV2K2_NUM_CONVERSIONMODES			NTV2_NUM_CONVERSIONMODES			///< @deprecated	Use NTV2_NUM_CONVERSIONMODES instead.
	#define		NTV2K2_CONVERSIONMODE_UNKNOWN		NTV2_CONVERSIONMODE_UNKNOWN			///< @deprecated	Use NTV2_CONVERSIONMODE_UNKNOWN instead.

	//	NTV2RGBBlackRange
	#define		NTV2K2_RGBBLACKRANGE_0_0x3FF		NTV2_RGBBLACKRANGE_0_0x3FF			///< @deprecated	Use NTV2_RGBBLACKRANGE_0_0x3FF instead.
	#define		NTV2K2_RGBBLACKRANGE_0x40_0x3C0		NTV2_RGBBLACKRANGE_0x40_0x3C0		///< @deprecated	Use NTV2_RGBBLACKRANGE_0x40_0x3C0 instead.

	//	NTV2Framesize
	#define		NTV2K2_FRAMESIZE_2MB				NTV2_FRAMESIZE_2MB					///< @deprecated	Use NTV2_FRAMESIZE_2MB instead.
	#define		NTV2K2_FRAMESIZE_4MB				NTV2_FRAMESIZE_4MB					///< @deprecated	Use NTV2_FRAMESIZE_4MB instead.
	#define		NTV2K2_FRAMESIZE_8MB				NTV2_FRAMESIZE_8MB					///< @deprecated	Use NTV2_FRAMESIZE_8MB instead.
	#define		NTV2K2_FRAMESIZE_16MB				NTV2_FRAMESIZE_16MB					///< @deprecated	Use NTV2_FRAMESIZE_16MB instead.

    //	NTV2VideoDACMode
    #define		NTV2K2_480iRGB						NTV2_480iRGB						///< @deprecated	Use NTV2_480iRGB instead.
    #define		NTV2K2_480iYPbPrSMPTE				NTV2_480iYPbPrSMPTE					///< @deprecated	Use NTV2_480iYPbPrSMPTE instead.
    #define		NTV2K2_480iYPbPrBetacam525			NTV2_480iYPbPrBetacam525			///< @deprecated	Use NTV2_480iYPbPrBetacam525 instead.
    #define		NTV2K2_480iYPbPrBetacamJapan		NTV2_480iYPbPrBetacamJapan			///< @deprecated	Use NTV2_480iYPbPrBetacamJapan instead.
    #define		NTV2K2_480iNTSC_US_Composite		NTV2_480iNTSC_US_Composite			///< @deprecated	Use NTV2_480iNTSC_US_Composite instead.
    #define		NTV2K2_480iNTSC_Japan_Composite		NTV2_480iNTSC_Japan_Composite		///< @deprecated	Use NTV2_480iNTSC_Japan_Composite instead.
    #define		NTV2K2_576iRGB						NTV2_576iRGB						///< @deprecated	Use NTV2_576iRGB instead.
    #define		NTV2K2_576iYPbPrSMPTE				NTV2_576iYPbPrSMPTE					///< @deprecated	Use NTV2_576iYPbPrSMPTE instead.
    #define		NTV2K2_576iPAL_Composite			NTV2_576iPAL_Composite				///< @deprecated	Use NTV2_576iPAL_Composite instead.
    #define		NTV2K2_1080iRGB						NTV2_1080iRGB						///< @deprecated	Use NTV2_1080iRGB instead.
    #define		NTV2K2_1080psfRGB					NTV2_1080psfRGB						///< @deprecated	Use NTV2_1080psfRGB instead.
    #define		NTV2K2_720pRGB						NTV2_720pRGB						///< @deprecated	Use NTV2_720pRGB instead.
    #define		NTV2K2_1080iSMPTE					NTV2_1080iSMPTE						///< @deprecated	Use NTV2_1080iSMPTE instead.
    #define		NTV2K2_1080psfSMPTE					NTV2_1080psfSMPTE					///< @deprecated	Use NTV2_1080psfSMPTE instead.
    #define		NTV2K2_720pSMPTE					NTV2_720pSMPTE						///< @deprecated	Use NTV2_720pSMPTE instead.
    #define		NTV2K2_1080iXVGA					NTV2_1080iXVGA						///< @deprecated	Use NTV2_1080iXVGA instead.
    #define		NTV2K2_1080psfXVGA					NTV2_1080psfXVGA					///< @deprecated	Use NTV2_1080psfXVGA instead.
    #define		NTV2K2_720pXVGA						NTV2_720pXVGA						///< @deprecated	Use NTV2_720pXVGA instead.
    #define		NTV2K2_2Kx1080RGB					NTV2_2Kx1080RGB						///< @deprecated	Use NTV2_2Kx1080RGB instead.
    #define		NTV2K2_2Kx1080SMPTE					NTV2_2Kx1080SMPTE					///< @deprecated	Use NTV2_2Kx1080SMPTE instead.
    #define		NTV2K2_2Kx1080XVGA					NTV2_2Kx1080XVGA					///< @deprecated	Use NTV2_2Kx1080XVGA instead.

    //	NTV2LSVideoADCMode
    #define		NTV2K2_480iADCComponentBeta			NTV2_480iADCComponentBeta			///< @deprecated	Use NTV2_480iADCComponentBeta instead.
    #define		NTV2K2_480iADCComponentSMPTE		NTV2_480iADCComponentSMPTE			///< @deprecated	Use NTV2_480iADCComponentSMPTE instead.
    #define		NTV2K2_480iADCSVideoUS				NTV2_480iADCSVideoUS				///< @deprecated	Use NTV2_480iADCSVideoUS instead.
    #define		NTV2K2_480iADCCompositeUS			NTV2_480iADCCompositeUS				///< @deprecated	Use NTV2_480iADCCompositeUS instead.
    #define		NTV2K2_480iADCComponentBetaJapan	NTV2_480iADCComponentBetaJapan		///< @deprecated	Use NTV2_480iADCComponentBetaJapan instead.
    #define		NTV2K2_480iADCComponentSMPTEJapan	NTV2_480iADCComponentSMPTEJapan		///< @deprecated	Use NTV2_480iADCComponentSMPTEJapan instead.
    #define		NTV2K2_480iADCSVideoJapan			NTV2_480iADCSVideoJapan				///< @deprecated	Use NTV2_480iADCSVideoJapan instead.
    #define		NTV2K2_480iADCCompositeJapan		NTV2_480iADCCompositeJapan			///< @deprecated	Use NTV2_480iADCCompositeJapan instead.
    #define		NTV2K2_576iADCComponentBeta			NTV2_576iADCComponentBeta			///< @deprecated	Use NTV2_576iADCComponentBeta instead.
    #define		NTV2K2_576iADCComponentSMPTE		NTV2_576iADCComponentSMPTE			///< @deprecated	Use NTV2_576iADCComponentSMPTE instead.
    #define		NTV2K2_576iADCSVideo				NTV2_576iADCSVideo					///< @deprecated	Use NTV2_576iADCSVideo instead.
    #define		NTV2K2_576iADCComposite				NTV2_576iADCComposite				///< @deprecated	Use NTV2_576iADCComposite instead.
    #define		NTV2K2_720p_60						NTV2_720p_60						///< @deprecated	Use NTV2_720p_60 instead.
    #define		NTV2K2_1080i_30						NTV2_1080i_30						///< @deprecated	Use NTV2_1080i_30 instead.
    #define		NTV2K2_720p_50						NTV2_720p_50						///< @deprecated	Use NTV2_720p_50 instead.
    #define		NTV2K2_1080i_25						NTV2_1080i_25						///< @deprecated	Use NTV2_1080i_25 instead.
    #define		NTV2K2_1080pSF24					NTV2_1080pSF24						///< @deprecated	Use NTV2_1080pSF24 instead.

    //	NTV2AnalogType
    #define		NTV2K2_AnlgComposite				NTV2_AnlgComposite					///< @deprecated	Use NTV2_AnlgComposite instead.
    #define		NTV2K2_AnlgComponentSMPTE			NTV2_AnlgComponentSMPTE				///< @deprecated	Use NTV2_AnlgComponentSMPTE instead.
    #define		NTV2K2_AnlgComponentBetacam			NTV2_AnlgComponentBetacam			///< @deprecated	Use NTV2_AnlgComponentBetacam instead.
    #define		NTV2K2_AnlgComponentRGB				NTV2_AnlgComponentRGB				///< @deprecated	Use NTV2_AnlgComponentRGB instead.
    #define		NTV2K2_AnlgXVGA						NTV2_AnlgXVGA						///< @deprecated	Use NTV2_AnlgXVGA instead.
    #define		NTV2K2_AnlgSVideo					NTV2_AnlgSVideo						///< @deprecated	Use NTV2_AnlgSVideo instead.

    //	R2BlackLevel
    #define		NTV2K2_Black75IRE					R2_Black_75IRE						///< @deprecated	Use R2_Black_75IRE instead.
    #define		NTV2K2_Black0IRE					R2_Black_0IRE						///< @deprecated	Use R2_Black_0IRE instead.

    //	NTV2InputVideoSelect
    #define		NTV2K2_Input1Select					NTV2_Input1Select					///< @deprecated	Use NTV2_Input1Select instead.
    #define		NTV2K2_Input2Select					NTV2_Input2Select					///< @deprecated	Use NTV2_Input2Select instead.
    #define		NTV2K2_AnalogInputSelect			NTV2_AnalogInputSelect				///< @deprecated	Use NTV2_AnalogInputSelect instead.
    #define		NTV2K2_Input3Select					NTV2_Input3Select					///< @deprecated	Use NTV2_Input3Select instead.
    #define		NTV2K2_Input4Select					NTV2_Input4Select					///< @deprecated	Use NTV2_Input4Select instead.
    #define		NTV2K2_Input5Select					NTV2_Input5Select					///< @deprecated	Use NTV2_Input5Select instead.
    #define		NTV2K2_DualLinkInputSelect			NTV2_Input2xDLHDSelect				///< @deprecated	Use NTV2_Input2xDLHDSelect instead.
    #define		NTV2K2_DualLink2xSdi4k				NTV2_Input2x4kSelect				///< @deprecated	Use NTV2_Input2x4kSelect instead.
    #define		NTV2K2_DualLink4xSdi4k				NTV2_Input4x4kSelect				///< @deprecated	Use NTV2_Input4x4kSelect instead.
    #define		NTV2K2_InputSelectMax				NTV2_MAX_NUM_InputVideoSelectEnums	///< @deprecated	Use NTV2_MAX_NUM_InputVideoSelectEnums instead.

    //	NTV2SDIInputFormatSelect
    #define		NTV2K2_YUVSelect					NTV2_RGBSelect						///< @deprecated	Use NTV2_RGBSelect instead.
    #define		NTV2K2_RGBSelect					NTV2_RGBSelect						///< @deprecated	Use NTV2_RGBSelect instead.
    #define		NTV2K2_Stereo3DSelect				NTV2_Stereo3DSelect					///< @deprecated	Use NTV2_Stereo3DSelect instead.

    //	NTV2InputAudioSelect
    #define		NTV2K2_AES_EBU_XLRSelect			NTV2_AES_EBU_XLRSelect				///< @deprecated	Use NTV2_AES_EBU_XLRSelect instead.
    #define		NTV2K2_AES_EBU_BNCSelect			NTV2_AES_EBU_BNCSelect				///< @deprecated	Use NTV2_AES_EBU_BNCSelect instead.
    #define		NTV2K2_Input1Embedded1_8Select		NTV2_Input1Embedded1_8Select		///< @deprecated	Use NTV2_Input1Embedded1_8Select instead.
    #define		NTV2K2_Input1Embedded9_16Select		NTV2_Input1Embedded9_16Select		///< @deprecated	Use NTV2_Input1Embedded9_16Select instead.
    #define		NTV2K2_Input2Embedded1_8Select		NTV2_Input2Embedded1_8Select		///< @deprecated	Use NTV2_Input2Embedded1_8Select instead.
    #define		NTV2K2_Input2Embedded9_16Select		NTV2_Input2Embedded9_16Select		///< @deprecated	Use NTV2_Input2Embedded9_16Select instead.
    #define		NTV2K2_AnalogSelect					NTV2_AnalogSelect					///< @deprecated	Use NTV2_AnalogSelect instead.
    #define		NTV2K2_HDMISelect					NTV2_HDMISelect						///< @deprecated	Use NTV2_HDMISelect instead.
    #define		NTV2K2_AudioInputOther				NTV2_AudioInputOther				///< @deprecated	Use NTV2_AudioInputOther instead.

    //	NTV2AudioMapSelect
    #define		NTV2K2_AudioMap12_12				NTV2_AudioMap12_12					///< @deprecated	Use NTV2_AudioMap12_12 instead.
    #define		NTV2K2_AudioMap34_12				NTV2_AudioMap34_12					///< @deprecated	Use NTV2_AudioMap34_12 instead.
    #define		NTV2K2_AudioMap56_12				NTV2_AudioMap56_12					///< @deprecated	Use NTV2_AudioMap56_12 instead.
    #define		NTV2K2_AudioMap78_12				NTV2_AudioMap78_12					///< @deprecated	Use NTV2_AudioMap78_12 instead.
    #define		NTV2K2_AudioMap910_12				NTV2_AudioMap910_12					///< @deprecated	Use NTV2_AudioMap910_12 instead.
    #define		NTV2K2_AudioMap1112_12				NTV2_AudioMap1112_12				///< @deprecated	Use NTV2_AudioMap1112_12 instead.
    #define		NTV2K2_AudioMap1314_12				NTV2_AudioMap1314_12				///< @deprecated	Use NTV2_AudioMap1314_12 instead.
    #define		NTV2K2_AudioMap1516_12				NTV2_AudioMap1516_12				///< @deprecated	Use NTV2_AudioMap1516_12 instead.

    //	NTV2OutputVideoSelect
    #define		NTV2K2_PrimaryOutputSelect			NTV2_PrimaryOutputSelect			///< @deprecated	Use NTV2_PrimaryOutputSelect instead.
    #define		NTV2K2_SecondaryOutputSelect		NTV2_SecondaryOutputSelect			///< @deprecated	Use NTV2_SecondaryOutputSelect instead.
    #define		NTV2K2_DualLinkOutputSelect			NTV2_RgbOutputSelect				///< @deprecated	Use NTV2_RgbOutputSelect instead.
    #define		NTV2K2_VideoPlusKeySelect			NTV2_VideoPlusKeySelect				///< @deprecated	Use NTV2_VideoPlusKeySelect instead.
    #define		NTV2K2_StereoOutputSelect			NTV2_StereoOutputSelect				///< @deprecated	Use NTV2_StereoOutputSelect instead.
    #define		NTV2K2_Quadrant1Select				NTV2_Quadrant1Select				///< @deprecated	Use NTV2_Quadrant1Select instead.
    #define		NTV2K2_Quadrant2Select				NTV2_Quadrant2Select				///< @deprecated	Use NTV2_Quadrant2Select instead.
    #define		NTV2K2_Quadrant3Select				NTV2_Quadrant3Select				///< @deprecated	Use NTV2_Quadrant3Select instead.
    #define		NTV2K2_Quadrant4Select				NTV2_Quadrant4Select				///< @deprecated	Use NTV2_Quadrant4Select instead.
    #define		NTV2k2_Quarter4k					NTV2_Quarter4k						///< @deprecated	Use NTV2_Quarter4k instead.

    //	NTV2UpConvertMode
    #define		NTV2K2_UpConvertAnamorphic			NTV2_UpConvertAnamorphic			///< @deprecated	Use NTV2_UpConvertAnamorphic instead.
    #define		NTV2K2_UpConvertPillarbox4x3		NTV2_UpConvertPillarbox4x3			///< @deprecated	Use NTV2_UpConvertPillarbox4x3 instead.
    #define		NTV2K2_UpConvertZoom14x9			NTV2_UpConvertZoom14x9				///< @deprecated	Use NTV2_UpConvertZoom14x9 instead.
    #define		NTV2K2_UpConvertZoomLetterbox		NTV2_UpConvertZoomLetterbox			///< @deprecated	Use NTV2_UpConvertZoomLetterbox instead.
    #define		NTV2K2_UpConvertZoomWide			NTV2_UpConvertZoomWide				///< @deprecated	Use NTV2_UpConvertZoomWide instead.

    //	NTV2DownConvertMode
    #define		NTV2K2_DownConvertLetterbox			NTV2_DownConvertLetterbox			///< @deprecated	Use NTV2_DownConvertLetterbox instead.
    #define		NTV2K2_DownConvertCrop				NTV2_DownConvertCrop				///< @deprecated	Use NTV2_DownConvertCrop instead.
    #define		NTV2K2_DownConvertAnamorphic		NTV2_DownConvertAnamorphic			///< @deprecated	Use NTV2_DownConvertAnamorphic instead.
    #define		NTV2K2_DownConvert14x9				NTV2_DownConvert14x9				///< @deprecated	Use NTV2_DownConvert14x9 instead.

    //	NTV2IsoConvertMode
    #define		NTV2K2_IsoLetterBox					NTV2_IsoLetterBox					///< @deprecated	Use NTV2_IsoLetterBox instead.
    #define		NTV2K2_IsoHCrop						NTV2_IsoHCrop						///< @deprecated	Use NTV2_IsoHCrop instead.
    #define		NTV2K2_IsoPillarBox					NTV2_IsoPillarBox					///< @deprecated	Use NTV2_IsoPillarBox instead.
    #define		NTV2K2_IsoVCrop						NTV2_IsoVCrop						///< @deprecated	Use NTV2_IsoVCrop instead.
    #define		NTV2K2_Iso14x9						NTV2_Iso14x9						///< @deprecated	Use NTV2_Iso14x9 instead.
    #define		NTV2K2_IsoPassThrough				NTV2_IsoPassThrough					///< @deprecated	Use NTV2_IsoPassThrough instead.

    //	NTV2QuarterSizeExpandMode
    #define		NTV2K2_QuarterSizeExpandOff			NTV2_QuarterSizeExpandOff			///< @deprecated	Use NTV2_QuarterSizeExpandOff instead.
    #define		NTV2K2_QuarterSizeExpandOn			NTV2_QuarterSizeExpandOn			///< @deprecated	Use NTV2_QuarterSizeExpandOn instead.

    //	NTV2FrameBufferQuality
    #define		NTV2K2_StandardQuality				NTV2_StandardQuality				///< @deprecated	Use NTV2_StandardQuality instead.
    #define		NTV2K2_HighQuality					NTV2_HighQuality					///< @deprecated	Use NTV2_HighQuality instead.
    #define		NTV2K2_ProRes						NTV2_ProRes							///< @deprecated	Use NTV2_ProRes instead.
    #define		NTV2K2_ProResHQ						NTV2_ProResHQ						///< @deprecated	Use NTV2_ProResHQ instead.
    #define		NTV2K2_ProResLT						NTV2_ProResLT						///< @deprecated	Use NTV2_ProResLT instead.
    #define		NTV2K2_ProResProxy					NTV2_ProResProxy					///< @deprecated	Use NTV2_ProResProxy instead.

    //	NTV2EncodeAsPSF
    #define		NTV2K2_NoPSF						NTV2_NoPSF							///< @deprecated	Use NTV2_NoPSF instead.
    #define		NTV2K2_IsPSF						NTV2_IsPSF							///< @deprecated	Use NTV2_IsPSF instead.

    //	NTV2BreakoutType
    #define		NTV2K2_BreakoutNone					NTV2_BreakoutNone					///< @deprecated	Use NTV2_BreakoutNone instead.
    #define		NTV2K2_BreakoutCableXLR				NTV2_BreakoutCableXLR				///< @deprecated	Use NTV2_BreakoutCableXLR instead.
    #define		NTV2K2_BreakoutCableBNC				NTV2_BreakoutCableBNC				///< @deprecated	Use NTV2_BreakoutCableBNC instead.
    #define		NTV2K2_KBox							NTV2_KBox							///< @deprecated	Use NTV2_KBox instead.
    #define		NTV2K2_KLBox						NTV2_KLBox							///< @deprecated	Use NTV2_KLBox instead.
    #define		NTV2K2_K3Box						NTV2_K3Box							///< @deprecated	Use NTV2_K3Box instead.
    #define		NTV2K2_KLHiBox						NTV2_KLHiBox						///< @deprecated	Use NTV2_KLHiBox instead.
    #define		NTV2K2_KLHePlusBox					NTV2_KLHePlusBox					///< @deprecated	Use NTV2_KLHePlusBox instead.
    #define		NTV2K2_K3GBox						NTV2_K3GBox							///< @deprecated	Use NTV2_K3GBox instead.

    //	NTV2AudioMonitorSelect
    #define		NTV2K2_AudioMonitor1_2				NTV2_AudioMonitor1_2				///< @deprecated	Use NTV2_AudioMonitor1_2 instead.
    #define		NTV2K2_AudioMonitor3_4				NTV2_AudioMonitor3_4				///< @deprecated	Use NTV2_AudioMonitor3_4 instead.
    #define		NTV2K2_AudioMonitor5_6				NTV2_AudioMonitor5_6				///< @deprecated	Use NTV2_AudioMonitor5_6 instead.
    #define		NTV2K2_AudioMonitor7_8				NTV2_AudioMonitor7_8				///< @deprecated	Use NTV2_AudioMonitor7_8 instead.
    #define		NTV2K2_AudioMonitor9_10				NTV2_AudioMonitor9_10				///< @deprecated	Use NTV2_AudioMonitor9_10 instead.
    #define		NTV2K2_AudioMonitor11_12			NTV2_AudioMonitor11_12				///< @deprecated	Use NTV2_AudioMonitor11_12 instead.
    #define		NTV2K2_AudioMonitor13_14			NTV2_AudioMonitor13_14				///< @deprecated	Use NTV2_AudioMonitor13_14 instead.
    #define		NTV2K2_AudioMonitor15_16			NTV2_AudioMonitor15_16				///< @deprecated	Use NTV2_AudioMonitor15_16 instead.

    //	NTV2Audio2ChannelSelect
    #define		NTV2K2_AudioChannel1_2				NTV2_AudioChannel1_2				///< @deprecated	Use NTV2_AudioChannel1_2 instead.
    #define		NTV2K2_AudioChannel3_4				NTV2_AudioChannel3_4				///< @deprecated	Use NTV2_AudioChannel3_4 instead.
    #define		NTV2K2_AudioChannel5_6				NTV2_AudioChannel5_6				///< @deprecated	Use NTV2_AudioChannel5_6 instead.
    #define		NTV2K2_AudioChannel7_8				NTV2_AudioChannel7_8				///< @deprecated	Use NTV2_AudioChannel7_8 instead.
    #define		NTV2K2_AudioChannel9_10				NTV2_AudioChannel9_10				///< @deprecated	Use NTV2_AudioChannel9_10 instead.
    #define		NTV2K2_AudioChannel11_12			NTV2_AudioChannel11_12				///< @deprecated	Use NTV2_AudioChannel11_12 instead.
    #define		NTV2K2_AudioChannel13_14			NTV2_AudioChannel13_14				///< @deprecated	Use NTV2_AudioChannel13_14 instead.
    #define		NTV2K2_AudioChannel15_16			NTV2_AudioChannel15_16				///< @deprecated	Use NTV2_AudioChannel15_16 instead.
    #define		NTV2_MAX_NUM_Audio2ChannelSelect	NTV2_MAX_NUM_AudioChannelPair		///< @deprecated	Use NTV2_MAX_NUM_AudioChannelPair instead.

    //	NTV2Audio4ChannelSelect
    #define		NTV2K2_AudioChannel1_4				NTV2_AudioChannel1_4				///< @deprecated	Use NTV2_AudioChannel1_4 instead.
    #define		NTV2K2_AudioChannel5_8				NTV2_AudioChannel5_8				///< @deprecated	Use NTV2_AudioChannel5_8 instead.
    #define		NTV2K2_AudioChannel9_12				NTV2_AudioChannel9_12				///< @deprecated	Use NTV2_AudioChannel9_12 instead.
    #define		NTV2K2_AudioChannel13_16			NTV2_AudioChannel13_16				///< @deprecated	Use NTV2_AudioChannel13_16 instead.

    //	NTV2Audio8ChannelSelect
    #define		NTV2K2_AudioChannel1_8				NTV2_AudioChannel1_8				///< @deprecated	Use NTV2_AudioChannel1_8 instead.
    #define		NTV2K2_AudioChannel9_16				NTV2_AudioChannel9_16				///< @deprecated	Use NTV2_AudioChannel9_16 instead.

    //	NTV2ColorSpaceType
    #define		NTV2_ColorSpaceAuto					NTV2_ColorSpaceTypeAuto				///< @deprecated	Use NTV2_ColorSpaceTypeAuto instead.
    #define		NTV2_ColorSpaceRec601				NTV2_ColorSpaceTypeRec601			///< @deprecated	Use NTV2_ColorSpaceTypeRec601 instead.
    #define		NTV2_ColorSpaceRec709				NTV2_ColorSpaceTypeRec709			///< @deprecated	Use NTV2_ColorSpaceTypeRec709 instead.

    //	NTV2Stereo3DMode
    #define		NTV2K2_Stereo3DOff					NTV2_Stereo3DOff					///< @deprecated	Use NTV2_Stereo3DOff instead.
    #define		NTV2K2_Stereo3DSideBySide			NTV2_Stereo3DSideBySide				///< @deprecated	Use NTV2_Stereo3DSideBySide instead.
    #define		NTV2K2_Stereo3DTopBottom			NTV2_Stereo3DTopBottom				///< @deprecated	Use NTV2_Stereo3DTopBottom instead.
    #define		NTV2K2_Stereo3DDualStream			NTV2_Stereo3DDualStream				///< @deprecated	Use NTV2_Stereo3DDualStream instead.

	//	NTV2ColorSpaceMode
	#define		NTV2K2_ColorSpaceAuto				NTV2_ColorSpaceModeAuto				///< @deprecated	Use NTV2_ColorSpaceModeAuto instead.
	#define		NTV2K2_ColorSpaceYCbCr				NTV2_ColorSpaceModeYCbCr			///< @deprecated	Use NTV2_ColorSpaceModeYCbCr instead.
	#define		NTV2K2_ColorSpaceRGB				NTV2_ColorSpaceModeRGB				///< @deprecated	Use NTV2_ColorSpaceModeRGB instead.

	#define	NTV2_INPUT_SOURCE_IS_VALID(_inpSrc_)		(((_inpSrc_) >= 0) && ((_inpSrc_) < NTV2_NUM_INPUTSOURCES))
	#define	IS_VALID_NTV2FrameGeometry(__s__)			((__s__) >= NTV2_FG_1920x1080 && (__s__) < NTV2_FG_NUMFRAMEGEOMETRIES)
	#define	IS_VALID_NTV2FrameBufferFormat(__s__)		((__s__) >= NTV2_FBF_10BIT_YCBCR && (__s__) < NTV2_FBF_NUMFRAMEBUFFERFORMATS)
	#define	IS_VALID_NTV2V2Standard(__s__)				((__s__) >= NTV2_V2_STANDARD_1080 && (__s__) < NTV2_V2_STANDARD_UNDEFINED)
	#define	IS_VALID_NTV2Standard(__s__)				((__s__) >= NTV2_STANDARD_1080 && (__s__) < NTV2_STANDARD_UNDEFINED)
	#define	NTV2_IS_VALID_NTV2AudioSystem(__x__)		((__x__) >= NTV2_AUDIOSYSTEM_1 && (__x__) < NTV2_MAX_NUM_AudioSystemEnums)
	#define	NTV2_AUDIO_CHANNEL_PAIR_IS_VALID(__p__)		((__p__) >= NTV2_AudioChannel1_2 && (__p__) < NTV2_MAX_NUM_AudioChannelPair)
	#define	NTV2_IS_VALID_NTV2Crosspoint(__x__)			((__x__) >= NTV2CROSSPOINT_CHANNEL1 && (__x__) < NTV2_NUM_CROSSPOINTS)
	#define	NTV2_OUTPUT_DEST_IS_VALID(_dest_)			(((_dest_) >= 0) && ((_dest_) < NTV2_NUM_OUTPUTDESTINATIONS))
	#define IS_VALID_NTV2Channel(__x__)					((__x__) >= NTV2_CHANNEL1 && (__x__) < NTV2_MAX_NUM_CHANNELS)
	#define IS_VALID_NTV2ReferenceSource(__x__)			((__x__) >= NTV2_REFERENCE_EXTERNAL && (__x__) < NTV2_NUM_REFERENCE_INPUTS)

#endif	//	!defined (NTV2_DEPRECATE)

typedef enum
{
	NTV2IpErrNone,
	NTV2IpErrInvalidChannel,
	NTV2IpErrInvalidFormat,
	NTV2IpErrInvalidBitdepth,
	NTV2IpErrInvalidUllHeight,
	NTV2IpErrInvalidUllLevels,
	NTV2IpErrUllNotSupported,
	NTV2IpErrNotReady,
	NTV2IpErrSoftwareMismatch,
	NTV2IpErrSFP1NotConfigured,
	NTV2IpErrSFP2NotConfigured,
	NTV2IpErrInvalidIGMPVersion,
	NTV2IpErrCannotGetMacAddress,
	NTV2IpErrNotSupported,
	NTV2IpErrWriteSOMToMB,
	NTV2IpErrWriteSeqToMB,
	NTV2IpErrWriteCountToMB,
	NTV2IpErrTimeoutNoSOM,
	NTV2IpErrTimeoutNoSeq,
	NTV2IpErrTimeoutNoBytecount,
	NTV2IpErrExceedsFifo,
	NTV2IpErrNoResponseFromMB,
	NTV2IpErrAcquireMBTimeout,
	NTV2IpErrInvalidMBResponse,
	NTV2IpErrInvalidMBResponseSize,
	NTV2IpErrInvalidMBResponseNoMac,
	NTV2IpErrMBStatusFail,
	NTV2IpErrGrandMasterInfo,
	NTV2IpErrSDPTooLong,
	NTV2IpErrSDPNotFound,
	NTV2IpErrSDPEmpty,
	NTV2IpErrSDPInvalid,
	NTV2IpErrSDPURLInvalid,
	NTV2IpErrSDPNoVideo,
	NTV2IpErrSDPNoAudio,
	NTV2IpErrSDPNoANC,
	NTV2IpErrSFPNotFound,
	NTV2IpErrInvalidConfig,
	NTV2IpErrLLDPNotFound,
	NTV2IpNumErrTypes
} NTV2IpError;


#define FGVCROSSPOINTMASK (BIT_0+BIT_1+BIT_2+BIT_3)
#define FGVCROSSPOINTSHIFT (0)
#define BGVCROSSPOINTMASK (BIT_4+BIT_5+BIT_6+BIT_7)
#define BGVCROSSPOINTSHIFT (4)
#define FGKCROSSPOINTMASK (BIT_8+BIT_9+BIT_10+BIT_11)
#define FGKCROSSPOINTSHIFT (8)
#define BGKCROSSPOINTMASK (BIT_12+BIT_13+BIT_14+BIT_15)
#define BGKCROSSPOINTSHIFT (12)

#define VIDPROCMUX1MASK (BIT_0+BIT_1)
#define VIDPROCMUX1SHIFT (0)
#define VIDPROCMUX2MASK (BIT_2+BIT_3)
#define VIDPROCMUX2SHIFT (2)
#define VIDPROCMUX3MASK (BIT_4+BIT_5)
#define VIDPROCMUX3SHIFT (4)
#define VIDPROCMUX4MASK (BIT_6+BIT_7)
#define VIDPROCMUX4SHIFT (6)
#define VIDPROCMUX5MASK (BIT_8+BIT_9)
#define VIDPROCMUX5SHIFT (8)

#define SPLITMODEMASK (BIT_30+BIT_31)
#define SPLITMODESHIFT (30)


#endif //NTV2ENUMS_H
