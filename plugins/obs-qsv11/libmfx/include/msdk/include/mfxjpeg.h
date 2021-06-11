// Copyright (c) 2017-2019 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef __MFX_JPEG_H__
#define __MFX_JPEG_H__

#include "mfxdefs.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* CodecId */
enum {
    MFX_CODEC_JPEG    = MFX_MAKEFOURCC('J','P','E','G')
};

/* CodecProfile, CodecLevel */
enum
{
    MFX_PROFILE_JPEG_BASELINE      = 1
};

enum
{
    MFX_ROTATION_0      = 0,
    MFX_ROTATION_90     = 1,
    MFX_ROTATION_180    = 2,
    MFX_ROTATION_270    = 3
};

enum {
    MFX_EXTBUFF_JPEG_QT      = MFX_MAKEFOURCC('J','P','G','Q'),
    MFX_EXTBUFF_JPEG_HUFFMAN = MFX_MAKEFOURCC('J','P','G','H')
};

enum {
    MFX_JPEG_COLORFORMAT_UNKNOWN = 0,
    MFX_JPEG_COLORFORMAT_YCbCr   = 1,
    MFX_JPEG_COLORFORMAT_RGB     = 2
};

enum {
    MFX_SCANTYPE_UNKNOWN        = 0,
    MFX_SCANTYPE_INTERLEAVED    = 1,
    MFX_SCANTYPE_NONINTERLEAVED = 2
};

enum {
    MFX_CHROMAFORMAT_JPEG_SAMPLING = 6
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    mfxU16  reserved[7];
    mfxU16  NumTable;

    mfxU16    Qm[4][64];
} mfxExtJPEGQuantTables;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;

    mfxU16  reserved[2];
    mfxU16  NumDCTable;
    mfxU16  NumACTable;

    struct {
        mfxU8   Bits[16];
        mfxU8   Values[12];
    } DCTables[4];

    struct {
        mfxU8   Bits[16];
        mfxU8   Values[162];
    } ACTables[4];
} mfxExtJPEGHuffmanTables;
MFX_PACK_END()

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif // __MFX_JPEG_H__
