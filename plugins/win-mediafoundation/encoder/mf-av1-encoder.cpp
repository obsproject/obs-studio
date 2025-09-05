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

#include "mf-av1-encoder.hpp"

#include <obs-module.h>
#include <util/profiler.hpp>
#include <atlbase.h>
#include <Codecapi.h>
#include <d3d11.h>
#include <mfapi.h>
#include <mferror.h>

namespace {

eAVEncAV1VProfile MapProfile(MF::AV1Profile profile)
{
	switch (profile) {
	case MF::AV1ProfileMain:
		return eAVEncAV1VProfile_Main_420_8;
	case MF::AV1ProfileMain10:
		return eAVEncAV1VProfile_Main_420_10;
	default:
		return eAVEncAV1VProfile_Main_420_8;
	}
}

eAVEncCommonRateControlMode MapRateControl(MF::AV1RateControl rc)
{
	switch (rc) {
	case MF::AV1RateControlCBR:
		return eAVEncCommonRateControlMode_CBR;
	case MF::AV1RateControlVBR:
		return eAVEncCommonRateControlMode_UnconstrainedVBR;
	default:
		return eAVEncCommonRateControlMode_CBR;
	}
}

bool ProcessPlanes(std::function<void(UINT32 height, INT32 plane)> func, UINT32 height)
{
	INT32 plane = 0;
	func(height, plane++);
	func(height / 2, plane);
	return true;
}

} // namespace

MF::AV1Encoder::AV1Encoder(const obs_encoder_t *encoder, std::shared_ptr<EncoderDescriptor> descriptor, UINT32 width,
			   UINT32 height, UINT32 framerateNum, UINT32 framerateDen, AV1Profile profile, UINT32 bitrate)
	: MFBaseEncoder(encoder, descriptor, width, height, framerateNum, framerateDen, (int)profile, bitrate)
{
}

HRESULT MF::AV1Encoder::CreateMediaTypes(ComPtr<IMFMediaType> &i, ComPtr<IMFMediaType> &o)
{
	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	blog(LOG_INFO, "MFAV1_Create format:%d", voi->format);

	HRESULT hr;
	CHECK_HR_ERROR(MFCreateMediaType(&i));
	CHECK_HR_ERROR(MFCreateMediaType(&o));

	CHECK_HR_ERROR(i->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	if (voi->format == VIDEO_FORMAT_P010) {
		CHECK_HR_ERROR(i->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_P010));
		CHECK_HR_ERROR(o->SetUINT32(MF_MT_MPEG2_PROFILE, MapProfile(AV1ProfileMain10)));
	} else if (voi->format == VIDEO_FORMAT_NV12) {
		CHECK_HR_ERROR(i->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
		CHECK_HR_ERROR(o->SetUINT32(MF_MT_MPEG2_PROFILE, MapProfile(AV1ProfileMain)));
	} else {
		blog(LOG_INFO, "Format is not supported");
		return E_INVALIDARG;
	}

	CHECK_HR_ERROR(MFSetAttributeSize(i, MF_MT_FRAME_SIZE, width, height));
	CHECK_HR_ERROR(MFSetAttributeRatio(i, MF_MT_FRAME_RATE, framerateNum, framerateDen));
	CHECK_HR_ERROR(i->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlaceMode::MFVideoInterlace_Progressive));
	CHECK_HR_ERROR(MFSetAttributeRatio(i, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

	CHECK_HR_ERROR(o->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HR_ERROR(o->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_AV1));
	CHECK_HR_ERROR(MFSetAttributeSize(o, MF_MT_FRAME_SIZE, width, height));
	CHECK_HR_ERROR(MFSetAttributeRatio(o, MF_MT_FRAME_RATE, framerateNum, framerateDen));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_AVG_BITRATE, initialBitrate * 1000));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlaceMode::MFVideoInterlace_Progressive));
	CHECK_HR_ERROR(MFSetAttributeRatio(o, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_VIDEO_LEVEL, (UINT32)-1));

	return S_OK;
fail:
	return hr;
}

bool MF::AV1Encoder::SetRateControl(AV1RateControl rateControl)
{
	return SetRateControlMode(static_cast<UINT32>(MapRateControl(rateControl)));
}

bool MF::AV1Encoder::ProcessInput(UINT8 **data, UINT32 *linesize, UINT64 pts, Status *status)
{
	ProfileScope("AV1Encoder::ProcessInput");

	HRESULT hr;
	ComPtr<IMFSample> sample;
	ComPtr<IMFMediaBuffer> buffer;
	BYTE *bufferData;
	UINT32 imageSize;

	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	if (voi->format == VIDEO_FORMAT_P010) {
		CHECK_HR_ERROR(MFCalculateImageSize(MFVideoFormat_P010, width, height, &imageSize));
	} else {
		CHECK_HR_ERROR(MFCalculateImageSize(MFVideoFormat_NV12, width, height, &imageSize));
	}

	CHECK_HR_ERROR(CreateEmptySample(sample, buffer, imageSize));

	{
		ProfileScope("AV1EncoderCopyInputSample");
		CHECK_HR_ERROR(buffer->Lock(&bufferData, NULL, NULL));
		ProcessPlanes(
			[&, this](DWORD h, int plane) {
				MFCopyImage(bufferData, linesize[plane], data[plane], linesize[plane], linesize[plane],
					    h);
				bufferData += linesize[plane] * h;
			},
			height);
	}

	CHECK_HR_ERROR(buffer->Unlock());
	CHECK_HR_ERROR(buffer->SetCurrentLength(imageSize));

	return SubmitInputSample(sample, pts, status);
fail:
	*status = FAILURE;
	return false;
}
