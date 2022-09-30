// 
// Notice Regarding Standards.  AMD does not provide a license or sublicense to
// any Intellectual Property Rights relating to any standards, including but not
// limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
// AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
// (collectively, the "Media Technologies"). For clarity, you will pay any
// royalties due for such third party technologies, which may include the Media
// Technologies that are owed as a result of AMD providing the Software to you.
// 
// MIT license 
// 
// Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef AMFHQScaler_h
#define AMFHQScaler_h

#pragma once

#define AMFHQScaler L"AMFHQScaler"


// various types of algorithms supported by the high-quality scaler
enum  AMF_HQ_SCALER_ALGORITHM_ENUM
{
    AMF_HQ_SCALER_ALGORITHM_BILINEAR = 0,
    AMF_HQ_SCALER_ALGORITHM_BICUBIC = 1,
    AMF_HQ_SCALER_ALGORITHM_FSR = 2,

};


// PA object properties
#define AMF_HQ_SCALER_ALGORITHM         L"HQScalerAlgorithm"        // amf_int64(AMF_HQ_SCALER_ALGORITHM_ENUM) (Bi-linear, Bi-cubic, RCAS, Auto)"      - determines which scaling algorithm will be used
                                                                    //                                                                                   auto will chose best option between algorithms available
#define AMF_HQ_SCALER_ENGINE_TYPE       L"HQScalerEngineType"       // AMF_MEMORY_TYPE (DX11, DX12, OPENCL, VULKAN default : DX11)"                    - determines how the object is initialized and what kernels to use

#define AMF_HQ_SCALER_OUTPUT_SIZE       L"HQSOutputSize"            // AMFSize                                                                         - output scaling width/hieight

#define AMF_HQ_SCALER_KEEP_ASPECT_RATIO  L"KeepAspectRatio"         // bool (default=false) Keep aspect ratio if scaling. 
#define AMF_HQ_SCALER_FILL               L"Fill"                    // bool (default=false) fill area out of ROI. 
#define AMF_HQ_SCALER_FILL_COLOR         L"FillColor"               // AMFColor 
#define AMF_HQ_SCALER_FROM_SRGB          L"FromSRGB"                   //  bool (default=true) Convert to SRGB. 


#endif //#ifndef AMFHQScaler_h
