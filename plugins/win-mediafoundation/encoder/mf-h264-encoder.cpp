/*

This is provided under a dual MIT/GPLv2 license.  When using or
redistributing this, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) 2026 Qualcomm Technologies, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Contact Information:

Vignesh E, vignelum@qti.qualcomm.com
Qualcomm Technologies, Inc., Bangalore, India

MIT License

Copyright (c) 2026 Qualcomm Technologies, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE

*/

#include "mf-h264-encoder.hpp"

#include <obs-module.h>
#include <atlbase.h>
#include <Codecapi.h>
#include <d3d11.h>
#include <mfapi.h>
#include <mferror.h>

namespace {

eAVEncH264VProfile MapProfile(MF::H264Profile profile)
{
	switch (profile) {
	case MF::H264ProfileBaseline:
		return eAVEncH264VProfile_Base;
	case MF::H264ProfileMain:
		return eAVEncH264VProfile_Main;
	case MF::H264ProfileHigh:
		return eAVEncH264VProfile_High;
	default:
		return eAVEncH264VProfile_Base;
	}
}

eAVEncCommonRateControlMode MapRateControl(MF::H264RateControl rc)
{
	switch (rc) {
	case MF::H264RateControlCBR:
		return eAVEncCommonRateControlMode_CBR;
	case MF::H264RateControlConstrainedVBR:
		return eAVEncCommonRateControlMode_PeakConstrainedVBR;
	case MF::H264RateControlVBR:
		return eAVEncCommonRateControlMode_UnconstrainedVBR;
	case MF::H264RateControlCQP:
		return eAVEncCommonRateControlMode_Quality;
	default:
		return eAVEncCommonRateControlMode_CBR;
	}
}

} // namespace

MF::H264Encoder::H264Encoder(const obs_encoder_t *encoder, std::shared_ptr<EncoderDescriptor> descriptor, UINT32 width,
			     UINT32 height, UINT32 framerateNum, UINT32 framerateDen, H264Profile profile,
			     UINT32 bitrate)
	: MFBaseEncoder(encoder, descriptor, width, height, framerateNum, framerateDen, (int)profile, bitrate)
{
}

HRESULT MF::H264Encoder::CreateMediaTypes(ComPtr<IMFMediaType> &i, ComPtr<IMFMediaType> &o)
{
	HRESULT hr;
	CHECK_HR_ERROR(MFCreateMediaType(&i));
	CHECK_HR_ERROR(MFCreateMediaType(&o));

	CHECK_HR_ERROR(i->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HR_ERROR(i->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
	CHECK_HR_ERROR(MFSetAttributeSize(i, MF_MT_FRAME_SIZE, width, height));
	CHECK_HR_ERROR(MFSetAttributeRatio(i, MF_MT_FRAME_RATE, framerateNum, framerateDen));
	CHECK_HR_ERROR(i->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlaceMode::MFVideoInterlace_Progressive));
	CHECK_HR_ERROR(MFSetAttributeRatio(i, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

	CHECK_HR_ERROR(o->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HR_ERROR(o->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));
	CHECK_HR_ERROR(MFSetAttributeSize(o, MF_MT_FRAME_SIZE, width, height));
	CHECK_HR_ERROR(MFSetAttributeRatio(o, MF_MT_FRAME_RATE, framerateNum, framerateDen));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_AVG_BITRATE, initialBitrate * 1000));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlaceMode::MFVideoInterlace_Progressive));
	CHECK_HR_ERROR(MFSetAttributeRatio(o, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_VIDEO_LEVEL, (UINT32)-1));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_VIDEO_PROFILE, MapProfile((H264Profile)profile)));

	return S_OK;
fail:
	return hr;
}

bool MF::H264Encoder::SetRateControl(H264RateControl rateControl)
{
	return SetRateControlMode(static_cast<UINT32>(MapRateControl(rateControl)));
}

bool MF::H264Encoder::SetEntropyEncoding(H264EntropyEncoding entropyEncoding)
{
	return SetCodecBool(CODECAPI_AVEncH264CABACEnable, entropyEncoding == H264EntropyEncodingCABAC);
}
