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

#pragma once
#include "mf-encoder.hpp"

namespace MF {

enum HEVCProfile {
	HEVCProfileMain,
};

enum HEVCRateControl {
	HEVCRateControlCBR,
	HEVCRateControlVBR,
};

// HEVCQp is a type alias for the shared EncQP struct.
using HEVCQP = EncQP;

class HEVCEncoder : public MFBaseEncoder {
public:
	HEVCEncoder(const obs_encoder_t *encoder, std::shared_ptr<EncoderDescriptor> descriptor, UINT32 width,
		    UINT32 height, UINT32 framerateNum, UINT32 framerateDen, HEVCProfile profile, UINT32 bitrate);

	bool SetRateControl(HEVCRateControl rateControl);
	// HEVC uses CODECAPI_AVEncCommonLowLatency instead of CODECAPI_AVLowLatencyMode.
	bool SetLowLatency(bool lowLatency) override;

protected:
	HRESULT CreateMediaTypes(ComPtr<IMFMediaType> &inputType, ComPtr<IMFMediaType> &outputType) override;
};

} // namespace MF
