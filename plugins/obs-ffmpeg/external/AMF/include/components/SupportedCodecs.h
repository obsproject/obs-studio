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
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
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

//-------------------------------------------------------------------------------------------------
// An interface available on some components to provide information on supported input and output codecs 
//-------------------------------------------------------------------------------------------------
#ifndef AMF_SupportedCodecs_h
#define AMF_SupportedCodecs_h

#pragma once

#include "public/include/core/Interface.h"

//properties on the returned AMFPropertyStorage
#define SUPPORTEDCODEC_ID L"CodecId" //amf_int64
#define SUPPORTEDCODEC_SAMPLERATE L"SampleRate" //amf_int32

namespace amf
{
    class AMFSupportedCodecs : public AMFInterface
    {
    public:
        AMF_DECLARE_IID(0xc1003a83, 0x7934, 0x408a, 0x95, 0x5b, 0xc4, 0xdd, 0x85, 0x9d, 0xf5, 0x61)

        //call with increasing values until it returns AMF_OUT_OF_RANGE
        virtual AMF_RESULT     AMF_STD_CALL GetInputCodecAt(amf_size index, AMFPropertyStorage** codec) const = 0;
        virtual AMF_RESULT     AMF_STD_CALL GetOutputCodecAt(amf_size index, AMFPropertyStorage** codec) const = 0;
    };
    typedef AMFInterfacePtr_T<AMFSupportedCodecs> AMFSupportedCodecsPtr;
}

#endif