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

#ifndef AMFVQEnhancer_h
#define AMFVQEnhancer_h

#pragma once

#define VE_FCR_DEFAULT_ATTENUATION 0.1

#define AMFVQEnhancer L"AMFVQEnhancer"

#define AMF_VIDEO_ENHANCER_ENGINE_TYPE       L"AMF_VIDEI_ENHANCER_ENGINE_TYPE"        // AMF_MEMORY_TYPE (DX11, DX12, OPENCL, VULKAN default : DX11)"                    - determines how the object is initialized and what kernels to use
#define AMF_VIDEO_ENHANCER_OUTPUT_SIZE       L"AMF_VIDEO_ENHANCER_OUTPUT_SIZE"        // AMFSize
#define AMF_VE_FCR_ATTENUATION               L"AMF_VE_FCR_ATTENUATION"                // Float in the range of [0.02, 0.4], default : 0.1
#define AMF_VE_FCR_RADIUS                    L"AMF_VE_FCR_RADIUS"                     // int  in the range of [1, 4]

#endif //#ifndef AMFVQEnhancer_h
