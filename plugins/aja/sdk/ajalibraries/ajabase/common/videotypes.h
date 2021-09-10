/* SPDX-License-Identifier: MIT */
/**
	@file		videotypes.h
	@brief		Declares the enumeration constants used in the ajabase library.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_VIDEODEFINES
#define AJA_VIDEODEFINES

enum AJA_VideoFormat
{
	AJA_VideoFormat_Unknown,				// 0
	AJA_VideoFormat_525i_2997,				// 1 525
	AJA_VideoFormat_525p_5994,
	AJA_VideoFormat_525i_2398,
    AJA_VideoFormat_525_2400,
    AJA_VideoFormat_525psf_2997,
	AJA_VideoFormat_625psf_2500,			// 6 625
	AJA_VideoFormat_625i_2500,
	AJA_VideoFormat_720p_2398,				// 8 HD - 720
	AJA_VideoFormat_720p_2400,
	AJA_VideoFormat_720p_2500,
	AJA_VideoFormat_720p_2997,
	AJA_VideoFormat_720p_3000,
	AJA_VideoFormat_720p_5000,
	AJA_VideoFormat_720p_5994,
	AJA_VideoFormat_720p_6000,
	AJA_VideoFormat_1080i_2500,				// 16 HD 1080i
	AJA_VideoFormat_1080i_2997,
	AJA_VideoFormat_1080i_3000,
	AJA_VideoFormat_1080psf_2398,			// 19 HD 1080psf
	AJA_VideoFormat_1080psf_2400,
	AJA_VideoFormat_1080psf_2500,
	AJA_VideoFormat_1080psf_2997,
	AJA_VideoFormat_1080psf_3000,
	AJA_VideoFormat_1080p_2398,				// 24 HD 1080p
	AJA_VideoFormat_1080p_2400,				
	AJA_VideoFormat_1080p_2500,				
	AJA_VideoFormat_1080p_2997,
	AJA_VideoFormat_1080p_3000,
    AJA_VideoFormat_1080p_5000,
    AJA_VideoFormat_1080p_5994,
    AJA_VideoFormat_1080p_6000,
	AJA_VideoFormat_2K_1080psf_2398,		// 32 2Kx1080psf
	AJA_VideoFormat_2K_1080psf_2400,
	AJA_VideoFormat_2K_1080psf_2500,
	AJA_VideoFormat_2K_1080psf_2997,
	AJA_VideoFormat_2K_1080psf_3000,
	AJA_VideoFormat_2K_1080p_2398,			// 37 2Kx1080p
	AJA_VideoFormat_2K_1080p_2400,
	AJA_VideoFormat_2K_1080p_2500,
	AJA_VideoFormat_2K_1080p_2997,
	AJA_VideoFormat_2K_1080p_3000,
	AJA_VideoFormat_2K_1080p_4795,
	AJA_VideoFormat_2K_1080p_4800,
	AJA_VideoFormat_2K_1080p_5000,
	AJA_VideoFormat_2K_1080p_5994,
	AJA_VideoFormat_2K_1080p_6000,
	AJA_VideoFormat_2K_1556psf_1498,		//2Kx1556
	AJA_VideoFormat_2K_1556psf_1500,
	AJA_VideoFormat_2K_1556psf_2398,
	AJA_VideoFormat_2K_1556psf_2400,
	AJA_VideoFormat_2K_1556psf_2500,
	AJA_VideoFormat_2K_1556psf_2997,
	AJA_VideoFormat_2K_1556psf_3000,
	AJA_VideoFormat_Quad_1080psf_2398,		// 54 UltaHD
	AJA_VideoFormat_Quad_1080psf_2400,
	AJA_VideoFormat_Quad_1080psf_2500,
	AJA_VideoFormat_Quad_1080psf_2997,
	AJA_VideoFormat_Quad_1080psf_3000,
	AJA_VideoFormat_Quad_1080p_2398,		// 59
	AJA_VideoFormat_Quad_1080p_2400,
	AJA_VideoFormat_Quad_1080p_2500,
	AJA_VideoFormat_Quad_1080p_2997,
	AJA_VideoFormat_Quad_1080p_3000,
	AJA_VideoFormat_Quad_1080p_5000,
	AJA_VideoFormat_Quad_1080p_5994,
	AJA_VideoFormat_Quad_1080p_6000,
	AJA_VideoFormat_4K_1080psf_2398,		// 67 4K
	AJA_VideoFormat_4K_1080psf_2400,
	AJA_VideoFormat_4K_1080psf_2500,
	AJA_VideoFormat_4K_1080psf_2997,
	AJA_VideoFormat_4K_1080psf_3000,
	AJA_VideoFormat_4K_1080p_2398,
	AJA_VideoFormat_4K_1080p_2400,
	AJA_VideoFormat_4K_1080p_2500,
	AJA_VideoFormat_4K_1080p_2997,	
	AJA_VideoFormat_4K_1080p_3000,
	AJA_VideoFormat_4K_1080p_4795,
	AJA_VideoFormat_4K_1080p_4800,
	AJA_VideoFormat_4K_1080p_5000,
	AJA_VideoFormat_4K_1080p_5994,
	AJA_VideoFormat_4K_1080p_6000,
	
	// UHD2 (8K)
	AJA_VideoFormat_4x3840x2160p_2398,
	AJA_VideoFormat_4x3840x2160p_2400,
	AJA_VideoFormat_4x3840x2160p_2500,
	AJA_VideoFormat_4x3840x2160p_2997,
	AJA_VideoFormat_4x3840x2160p_3000,
	AJA_VideoFormat_4x3840x2160p_5000,
	AJA_VideoFormat_4x3840x2160p_5994,
	AJA_VideoFormat_4x3840x2160p_6000,
	
	// UHD2 (8K)
	AJA_VideoFormat_4x4096x2160p_2398,
	AJA_VideoFormat_4x4096x2160p_2400,
	AJA_VideoFormat_4x4096x2160p_2500,
	AJA_VideoFormat_4x4096x2160p_2997,
	AJA_VideoFormat_4x4096x2160p_3000,
	AJA_VideoFormat_4x4096x2160p_4795,
	AJA_VideoFormat_4x4096x2160p_4800,
	AJA_VideoFormat_4x4096x2160p_5000,
	AJA_VideoFormat_4x4096x2160p_5994,
	AJA_VideoFormat_4x4096x2160p_6000,

	AJA_VideoFormat_Size
};

enum AJA_PixelFormat
{
	AJA_PixelFormat_Unknown,
	AJA_PixelFormat_YCbCr10,			/**< Pixel format YCbCr 10 bit */
	AJA_PixelFormat_YCbCr8,				/**< Pixel format YCbCr 8 bit */
	AJA_PixelFormat_ARGB8,				/**< Pixel format ARGB 8 bit */
	AJA_PixelFormat_RGBA8,				/**< Pixel format RGBA 8 bit */
	AJA_PixelFormat_RGB10,				/**< Pixel format RGB 10 bit */
	AJA_PixelFormat_YUY28,				/**< Pixel format YUY2 8 bit */
	AJA_PixelFormat_ABGR8,				/**< Pixel format ABGR 8 bit */
	AJA_PixelFormat_RGB_DPX,			/**< Pixel format RGB DPX */
	AJA_PixelFormat_YCbCr_DPX,			/**< Pixel format YCrCb DPX */
	AJA_PixelFormat_DVCPRO,				/**< Pixel format DVCPRO */
	AJA_PixelFormat_QREZ,				/**< Pixel format QREZ */
	AJA_PixelFormat_HDV,				/**< Pixel format HDV */
	AJA_PixelFormat_RGB8_PACK,			/**< Pixel format RGB 8 bit packed */
	AJA_PixelFormat_BGR8_PACK,			/**< Pixel format BGR 8 bit packed */
	AJA_PixelFormat_YCbCrA10,			/**< Pixel format YCbCrA 10 bit */
	AJA_PixelFormat_RGB_DPX_LE,			/**< Pixel format RGB DPX little endian */
	AJA_PixelFormat_RGB12,				/**< Pixel format RGB 12 bit */
	AJA_PixelFormat_RGB12P,				/**< Pixel format RGB 12 bit packed */
	AJA_PixelFormat_PRORES_DVPRO,		/**< Pixel format PRORES DVPRO */
	AJA_PixelFormat_PRORES_HDV,			/**< Pixel format PRORES HDV */
	AJA_PixelFormat_RGB10_PACK,			/**< Pixel format RGB 10 bit packed */
	AJA_PixelFormat_YCbCr12_444,		/**< Pixel format YCrCb 12 bit 444 */
	AJA_PixelFormat_YCbCr12_422,		/**< Pixel format YCbCr 12 bit 422 */
	AJA_PixelFormat_RGB16,				/**< Pixel format RGB 16 bit */
	AJA_PixelFormat_YCbCr10_4K_RQ,		/**< Pixel format YCbCr 10 bit 4K right quadrant */
	AJA_PixelFormat_BAYER10_DPX_LJ,		/**< Pixel format Bayer 10 bit DPX Left Justified */
	AJA_PixelFormat_BAYER12_DPX_LJ,		/**< Pixel format Bayer 12 bit DPX Left Justified */
	AJA_PixelFormat_BAYER10_HS,			/**< Pixel format Bayer 10 bit */
	AJA_PixelFormat_BAYER12_HS,			/**< Pixel format Bayer 12 bit */
	AJA_PixelFormat_BAYER12_PACK_HS,	/**< Pixel format Bayer 12 bit packed */
	AJA_PixelFormat_RAW10,				/**< Pixel format RAW 10 bit fully packed */
	AJA_PixelFormat_RAW10_HS,			/**< Pixel format RAW 10 bit fully packed, high speed */
	AJA_PixelFormat_YCBCR10_420PL,		/**< Pixel format YCbCr 10 bit 420 packed planer */
	AJA_PixelFormat_YCBCR10_422PL,		/**< Pixel format YCbCr 10 bit 422 packed planer */
	AJA_PixelFormat_YCBCR8_420PL,		/**< Pixel format YCbCr 8 bit 420 packed planer */
	AJA_PixelFormat_YCBCR8_422PL,		/**< Pixel format YCbCr 8 bit 422 packed planer */

	AJA_PixelFormat_YCBCR8_420PL3,		/**< Pixel format YCbCr 8 bit 420 planar */
	AJA_PixelFormat_YCBCR8_422PL3,		/**< Pixel format YCbCr 8 bit 422 planar */
	AJA_PixelFormat_YCBCR10_420PL3LE,	/**< Pixel format YCbCr 10 bit 420 little-endian planar */
	AJA_PixelFormat_YCBCR10_422PL3LE,	/**< Pixel format YCbCr 10 bit 422 little-endian planar */

    /// SMPTE 2022-6/211/OpenCL related....not actually pixel formats on any board...
            AJA_PixelFormat_S0226_720p50,
            AJA_PixelFormat_S0226_720p60,
            AJA_PixelFormat_S0226_1080i30,
            AJA_PixelFormat_S0226_1080i25,
            AJA_PixelFormat_S0226_1080p30,
            AJA_PixelFormat_S0226_1080p25,
            AJA_PixelFormat_S0226_1080p24,
            AJA_PixelFormat_S0226_525i30,
            AJA_PixelFormat_S0226_625i25,

            AJA_PixelFormat_RFC4175_720p,
            AJA_PixelFormat_RFC4175_1080i,
            AJA_PixelFormat_RFC4175_1080p,
            AJA_PixelFormat_RFC4175_525i30,
            AJA_PixelFormat_RFC4175_625i25,
            AJA_PixelFormat_RFC4175_2160p,

            AJA_PixelFormat_RGB10_3DLUT,
	AJA_PixelFormat_Size
};


enum AJA_BayerColorPhase
{
	AJA_BayerColorPhase_Unknown,
	AJA_BayerColorPhase_RedGreen,
	AJA_BayerColorPhase_GreenRed,
	AJA_BayerColorPhase_BlueGreen,
	AJA_BayerColorPhase_GreenBlue,
	AJA_BayerColorPhase_Size
};


enum AJA_FrameRate
{
	AJA_FrameRate_Unknown,
	AJA_FrameRate_1498,
	AJA_FrameRate_1500,
	AJA_FrameRate_1798,
	AJA_FrameRate_1800,
	AJA_FrameRate_1898,
	AJA_FrameRate_1900,
	AJA_FrameRate_2398,
	AJA_FrameRate_2400,
	AJA_FrameRate_2500,
	AJA_FrameRate_2997,
	AJA_FrameRate_3000,
	AJA_FrameRate_4795,
	AJA_FrameRate_4800,
	AJA_FrameRate_5000,
	AJA_FrameRate_5994,
	AJA_FrameRate_6000,
	AJA_FrameRate_10000,
	AJA_FrameRate_11988,
	AJA_FrameRate_12000,

	AJA_FrameRate_Size
};

enum AJA_VideoConversion
{
	AJA_VideoConversion_Unknown,
	AJA_VideoConversion_1080i_5994to525_5994,
	AJA_VideoConversion_1080i_2500to625_2500,
	AJA_VideoConversion_720p_5994to525_5994,
	AJA_VideoConversion_720p_5000to625_2500,
	AJA_VideoConversion_525_5994to1080i_5994,
	AJA_VideoConversion_525_5994to720p_5994,
	AJA_VideoConversion_625_2500to1080i_2500,
	AJA_VideoConversion_625_2500to720p_5000,
	AJA_VideoConversion_720p_5000to1080i_2500,
	AJA_VideoConversion_720p_5994to1080i_5994,
	AJA_VideoConversion_720p_6000to1080i_3000,
	AJA_VideoConversion_1080i2398to525_2398,
	AJA_VideoConversion_1080i2398to525_2997,
	AJA_VideoConversion_1080i_2500to720p_5000,
	AJA_VideoConversion_1080i_5994to720p_5994,
	AJA_VideoConversion_1080i_3000to720p_6000,
	AJA_VideoConversion_1080i_2398to720p_2398,
	AJA_VideoConversion_720p_2398to1080i_2398,
	AJA_VideoConversion_525_2398to1080i_2398,
	AJA_VideoConversion_525_5994to525_5994,
	AJA_VideoConversion_625_2500to625_2500,
	AJA_VideoConversion_525_5994to525psf_2997,
	AJA_VideoConversion_625_5000to625psf_2500,
	AJA_VideoConversion_1080i_5000to1080psf_2500,
	AJA_VideoConversion_1080i_5994to1080psf_2997,
	AJA_VideoConversion_1080i_6000to1080psf_3000,
	AJA_VideoConversion_Size
};


enum AJA_HdrColorimetry		// compare R2HdrColorimetry
{
	AJA_HdrColor_Auto,
	AJA_HdrColor_SDR,
	AJA_HdrColor_P3,
	AJA_HdrColor_2020,
	AJA_HdrColor_Custom
};


enum AJA_HdrTransfer		// compare R2HdrTransfer
{
	AJA_HdrTrans_Auto,
	AJA_HdrTrans_SDR,
	AJA_HdrTrans_HLG,
	AJA_HdrTrans_PQ,
};


enum AJA_HdrLuminance		// compare R2HdrLuminance
{
	AJA_Luminance_Auto,
	AJA_Luminance_YCbCr,
	AJA_Luminance_ICtCp
};


#endif	//	AJA_VIDEODEFINES
