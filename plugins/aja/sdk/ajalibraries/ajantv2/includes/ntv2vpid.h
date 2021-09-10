/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2vpid.h
	@brief		Declares the CNTV2VPID class. See SMPTE 352 standard for details.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2VPID_H
#define NTV2VPID_H

#include "ajaexport.h"
#include "ntv2publicinterface.h"
#include "ajabase/system/info.h"

#if defined(AJALinux)
#include <stdio.h>
#endif

/**
    @brief	A convenience class that simplifies encoding or decoding the 4-byte VPID payload
			that can be read or written from/to VPID registers.
**/
class AJAExport CNTV2VPID
{
public:
	/**
		@name	Construction, Destruction, Copying, Assigning
	**/
	///@{
								CNTV2VPID (const ULWord inData = 0);
								CNTV2VPID (const CNTV2VPID & other);
	virtual CNTV2VPID &			operator = (const CNTV2VPID & inRHS);
	virtual inline				~CNTV2VPID ()							{}
	///@}

	/**
		@name	Inquiry
	**/
	///@{
	virtual inline ULWord			GetVPID (void) const			{return m_uVPID;}	///< @return	My current 4-byte VPID value.
	virtual VPIDVersion				GetVersion (void) const;
	virtual NTV2VideoFormat			GetVideoFormat (void) const;
	virtual bool					IsStandard3Ga (void) const;
	virtual bool					IsStandardMultiLink4320 (void) const;
	virtual bool					IsStandardTwoSampleInterleave (void) const;
	virtual VPIDStandard			GetStandard (void) const;
	virtual bool					GetProgressiveTransport (void) const;
	virtual bool					GetProgressivePicture (void) const;
	virtual VPIDPictureRate			GetPictureRate (void) const;
	virtual bool					GetImageAspect16x9 (void) const;
	virtual VPIDSampling			GetSampling (void) const;
	virtual bool					IsRGBSampling (void) const;						//	New in SDK 16.0
	virtual VPIDChannel				GetChannel (void) const;
	virtual VPIDChannel				GetDualLinkChannel (void) const;
	virtual VPIDBitDepth			GetBitDepth (void) const;
	virtual inline bool				IsValid (void) const			{return GetVersion() == VPIDVersion_1;}	///< @return	True if valid;  otherwise false.
	virtual AJALabelValuePairs &	GetInfo (AJALabelValuePairs & outInfo) const;
	virtual NTV2VPIDXferChars		GetTransferCharacteristics (void) const;
	virtual NTV2VPIDColorimetry		GetColorimetry (void) const;
	virtual NTV2VPIDLuminance		GetLuminance (void) const;
	virtual NTV2VPIDRGBRange		GetRGBRange (void) const;						//	New in SDK 16.0
	virtual std::ostream &			Print (std::ostream & ostrm) const;
	virtual std::ostream &			PrintPretty (std::ostream & ostrm) const;		//	New in SDK 16.0
    virtual std::string				AsString (const bool inTabular = false) const;	//	New in SDK 16.0
	///@}

	/**
		@name	Changing
	**/
	///@{
	virtual inline CNTV2VPID &	SetVPID (const ULWord inData)		{m_uVPID = inData;  return *this;}

	virtual bool				SetVPID (const NTV2VideoFormat		inVideoFormat,
										const NTV2FrameBufferFormat	inFrameBufferFormat,
										const bool					inIsProgressive,
										const bool					inIs16x9Aspect,
										const VPIDChannel			inVPIDChannel);

	virtual bool				SetVPID (const NTV2VideoFormat	inOutputFormat,
										const bool				inIsDualLink,
										const bool				inIs48BitRGBFormat,
										const bool				inIsOutput3Gb,
										const bool				inIsSMPTE425,
										const VPIDChannel		inVPIDhannel);


	
	virtual CNTV2VPID &			SetVersion					(const VPIDVersion inVersion);
	virtual CNTV2VPID &			SetStandard					(const VPIDStandard inStandard);
	virtual CNTV2VPID &			SetProgressiveTransport		(const bool inIsProgressiveTransport);
	virtual CNTV2VPID &			SetProgressivePicture		(const bool inIsProgressivePicture);
	virtual CNTV2VPID &			SetPictureRate				(const VPIDPictureRate inPictureRate);
	virtual CNTV2VPID &			SetImageAspect16x9			(const bool inIs16x9Aspect);
	virtual CNTV2VPID &			SetSampling					(const VPIDSampling inSampling);
	virtual CNTV2VPID &			SetChannel					(const VPIDChannel inChannel);
	virtual CNTV2VPID &			SetDualLinkChannel			(const VPIDChannel inChannel);
	virtual CNTV2VPID &			SetBitDepth					(const VPIDBitDepth inBitDepth);
	virtual CNTV2VPID &			SetTransferCharacteristics	(const NTV2VPIDXferChars inXferChars);
	virtual CNTV2VPID &			SetColorimetry				(const NTV2VPIDColorimetry inColorimetry);
	virtual CNTV2VPID &			SetLuminance				(const NTV2VPIDLuminance inLuminance);
	virtual CNTV2VPID &			SetRGBRange					(const NTV2VPIDRGBRange inRGBRange);	//	New in SDK 16.0
	virtual inline CNTV2VPID &	MakeInvalid					(void)		{return SetVPID(0);}		//	New in SDK 16.0
								
	///@}


	/**
		@name	Class Methods
	**/
	///@{
	static bool					SetVPIDData (ULWord &					outVPID,
											const NTV2VideoFormat		inOutputFormat,
											const NTV2FrameBufferFormat	inFrameBufferFormat,
											const bool					inIsProgressive,
											const bool					inIs16x9Aspect,
											const VPIDChannel			inVPIDChannel,
											const bool					inUseVPIDChannel = true);		// defaults to using VPID channel

	static bool					SetVPIDData (ULWord &				outVPID,
											const NTV2VideoFormat	inOutputFormat,
											const bool				inIsDualLinkRGB,
											const bool				inIsRGB48Bit,
											const bool				inIsOutput3Gb,
											const bool				inIsSMPTE425,
											const VPIDChannel		inVPIDChannel,
											const bool				inUseVPIDChannel = true,
											const bool				inOutputIs6G = false,
											const bool				inOutputIs12G = false,
											const NTV2VPIDXferChars	inXferChars = NTV2_VPID_TC_SDR_TV,
											const NTV2VPIDColorimetry	inColorimetry = NTV2_VPID_Color_Rec709,
											const NTV2VPIDLuminance	inLuminance = NTV2_VPID_Luminance_YCbCr,
											const NTV2VPIDRGBRange	inRGBRange = NTV2_VPID_Range_Narrow);	//	New in SDK 16.0

	static const std::string	VersionString				(const VPIDVersion version);	//	New in SDK 15.5
	static const std::string	StandardString				(const VPIDStandard std);		//	New in SDK 15.5
	static const std::string	PictureRateString			(const VPIDPictureRate rate);	//	New in SDK 15.5
	static const std::string	SamplingString				(const VPIDSampling sample);	//	New in SDK 15.5
	static const std::string	ChannelString				(const VPIDChannel chan);		//	New in SDK 15.5
	static const std::string	DynamicRangeString			(const VPIDDynamicRange range);	//	New in SDK 15.5
	static const std::string	BitDepthString				(const VPIDBitDepth depth);		//	New in SDK 15.5
	static const std::string	LinkString					(const VPIDLink link);			//	New in SDK 15.5
	static const std::string	AudioString					(const VPIDAudio audio);		//	New in SDK 15.5
	static const std::string	VPIDVersionToString			(const VPIDVersion inVers);		//  New in SDK 16.0.1
	static const std::string	VPIDStandardToString		(const VPIDStandard inStd);		//  New in SDK 16.0.1
	static bool					VPIDStandardIsSingleLink	(const VPIDStandard inStd);		//	New in SDK 16.0
	static bool					VPIDStandardIsDualLink		(const VPIDStandard inStd);		//	New in SDK 16.0
	static bool					VPIDStandardIsQuadLink		(const VPIDStandard inStd);		//	New in SDK 16.0
	static bool					VPIDStandardIsOctLink		(const VPIDStandard inStd);		//	New in SDK 16.0
	#if !defined (NTV2_DEPRECATE)
		virtual VPIDDynamicRange NTV2_DEPRECATED_f(GetDynamicRange (void) const);
		virtual void NTV2_DEPRECATED_f(SetDynamicRange (const VPIDDynamicRange inDynamicRange));	
		static inline NTV2_DEPRECATED_f(bool	SetVPIDData (ULWord *				pOutVPID,
														const NTV2VideoFormat		inOutputFormat,
														const NTV2FrameBufferFormat	inFrameBufferFormat,
														const bool					inIsProgressive,
														const bool					inIs16x9Aspect,
														const VPIDChannel			inVPIDChannel,
														const bool					inUseVPIDChannel = true))
											{return pOutVPID ? SetVPIDData (*pOutVPID, inOutputFormat, inFrameBufferFormat, inIsProgressive, inIs16x9Aspect, inVPIDChannel, inUseVPIDChannel) : false;}

		static inline NTV2_DEPRECATED_f(bool	SetVPIDData (ULWord *				pOutVPID,
														const NTV2VideoFormat	inOutputFormat,
														const bool				inDualLinkRGB,
														const bool				inIsRGB48Bit,
														const bool				inIsOutput3Gb,
														const bool				inIsSMPTE425,
														const VPIDChannel		inVPIDChannel,
														const bool				inUseChannel = true))
											{return pOutVPID ? SetVPIDData (*pOutVPID, inOutputFormat, inDualLinkRGB, inIsRGB48Bit, inIsOutput3Gb, inIsSMPTE425, inVPIDChannel, inUseChannel) : false;}

		virtual inline NTV2_DEPRECATED_f(void	Init (void))				{}		///< @deprecated	Obsolete. Do not use.
	#endif	//	!defined (NTV2_DEPRECATE)
	///@}

private:
	ULWord	m_uVPID;	///< @brief	My 32-bit VPID data value

};	//	CNTV2VPID

AJAExport std::ostream &	operator << (std::ostream & ostrm, const CNTV2VPID & inData);

#endif	//	NTV2VPID_H
