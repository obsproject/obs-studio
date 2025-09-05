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

#include "mf-hevc-encoder.hpp"

#include <obs-module.h>
#include <atlbase.h>
#include <Codecapi.h>
#include <d3d11.h>
#include <mfapi.h>
#include <mferror.h>

namespace {

eAVEncH265VProfile MapProfile(MF::HEVCProfile profile)
{
	switch (profile) {
	case MF::HEVCProfileMain:
	default:
		return eAVEncH265VProfile_Main_420_8;
	}
}

eAVEncCommonRateControlMode MapRateControl(MF::HEVCRateControl rc)
{
	switch (rc) {
	case MF::HEVCRateControlCBR:
		return eAVEncCommonRateControlMode_CBR;
	case MF::HEVCRateControlVBR:
		return eAVEncCommonRateControlMode_UnconstrainedVBR;
	default:
		return eAVEncCommonRateControlMode_CBR;
	}
}

} // namespace

MF::HEVCEncoder::HEVCEncoder(const obs_encoder_t *encoder, std::shared_ptr<EncoderDescriptor> descriptor, UINT32 width,
			     UINT32 height, UINT32 framerateNum, UINT32 framerateDen, HEVCProfile profile,
			     UINT32 bitrate)
	: MFBaseEncoder(encoder, descriptor, width, height, framerateNum, framerateDen, (int)profile, bitrate)
{
}

HRESULT MF::HEVCEncoder::CreateMediaTypes(ComPtr<IMFMediaType> &i, ComPtr<IMFMediaType> &o)
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
	CHECK_HR_ERROR(o->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_HEVC));
	CHECK_HR_ERROR(MFSetAttributeSize(o, MF_MT_FRAME_SIZE, width, height));
	CHECK_HR_ERROR(MFSetAttributeRatio(o, MF_MT_FRAME_RATE, framerateNum, framerateDen));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_AVG_BITRATE, initialBitrate * 1000));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlaceMode::MFVideoInterlace_Progressive));
	CHECK_HR_ERROR(MFSetAttributeRatio(o, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_VIDEO_LEVEL, (UINT32)-1));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_MPEG2_PROFILE, MapProfile((HEVCProfile)profile)));

	return S_OK;
fail:
	return hr;
}

bool MF::HEVCEncoder::SetRateControl(HEVCRateControl rateControl)
{
	return SetRateControlMode(static_cast<UINT32>(MapRateControl(rateControl)));
}

bool MF::HEVCEncoder::SetLowLatency(bool lowLatency)
{
	return SetCodecBool(CODECAPI_AVEncCommonLowLatency, lowLatency);
}
