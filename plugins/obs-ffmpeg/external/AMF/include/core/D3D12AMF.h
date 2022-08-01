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
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __D3D12AMF_h__
#define __D3D12AMF_h__
#pragma once 
#include "Platform.h"
#if defined(_WIN32)||(defined(__linux) && defined(AMF_WSL))
// syncronization properties set via SetPrivateData()
AMF_WEAK GUID  AMFResourceStateGUID = { 0x452da9bf, 0x4ad7, 0x47a5, { 0xa6, 0x9b, 0x96, 0xd3, 0x23, 0x76, 0xf2, 0xf3 } };   // Current resource state value (D3D12_RESOURCE_STATES ), sizeof(UINT), set on ID3D12Resource 
AMF_WEAK GUID  AMFFenceGUID         = { 0x910a7928, 0x57bd, 0x4b04, { 0x91, 0xa3, 0xe7, 0xb8, 0x04, 0x12, 0xcd, 0xa5 } };   // IUnknown (ID3D12Fence), set on ID3D12Resource  syncronization fence for this resource
AMF_WEAK GUID  AMFFenceValueGUID    = { 0x62a693d3, 0xbb4a, 0x46c9, { 0xa5, 0x04, 0x9a, 0x8e, 0x97, 0xbf, 0xf0, 0x56 } };   // The last value to wait on the fence from AMFFenceGUID; sizeof(UINT64), set on ID3D12Fence 
#endif

#endif // __D3D12AMF_h__