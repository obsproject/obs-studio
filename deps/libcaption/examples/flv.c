/**********************************************************************************************/
/* The MIT License                                                                            */
/*                                                                                            */
/* Copyright 2016-2016 Twitch Interactive, Inc. or its affiliates. All Rights Reserved.       */
/*                                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a copy               */
/* of this software and associated documentation files (the "Software"), to deal              */
/* in the Software without restriction, including without limitation the rights               */
/* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                  */
/* copies of the Software, and to permit persons to whom the Software is                      */
/* furnished to do so, subject to the following conditions:                                   */
/*                                                                                            */
/* The above copyright notice and this permission notice shall be included in                 */
/* all copies or substantial portions of the Software.                                        */
/*                                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                 */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE                */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                     */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,              */
/* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN                  */
/* THE SOFTWARE.                                                                              */
/**********************************************************************************************/
#include "flv.h"
#include <stdlib.h>
#include <string.h>

void flvtag_init (flvtag_t* tag)
{
    memset (tag,0,sizeof (flvtag_t));
}

void flvtag_free (flvtag_t* tag)
{
    if (tag->data) {
        free (tag->data);
    }

    flvtag_init (tag);
}

int flvtag_reserve (flvtag_t* tag, uint32_t size)
{
    size += FLV_TAG_HEADER_SIZE + FLV_TAG_FOOTER_SIZE;

    if (size > tag->aloc) {
        tag->data = realloc (tag->data,size);
        tag->aloc = size;
    }

    return 1;
}

FILE* flv_open_read (const char* flv)
{
    if (0 == flv || 0 == strcmp ("-",flv)) {
        return stdin;
    }

    return fopen (flv,"rb");
}

FILE* flv_open_write (const char* flv)
{
    if (0 == flv || 0 == strcmp ("-",flv)) {
        return stdout;
    }

    return fopen (flv,"wb");
}

FILE* flv_close (FILE* flv)
{
    fclose (flv);
    return 0;
}

int flv_read_header (FILE* flv, int* has_audio, int* has_video)
{
    uint8_t h[FLV_HEADER_SIZE];

    if (FLV_HEADER_SIZE != fread (&h[0],1,FLV_HEADER_SIZE,flv)) {
        return 0;
    }

    if ('F' != h[0] || 'L' != h[1] || 'V' != h[2]) {
        return 0;
    }

    (*has_audio) = h[4]&0x04;
    (*has_video) = h[4]&0x01;
    return 1;
}

int flv_write_header (FILE* flv, int has_audio, int has_video)
{
    uint8_t h[FLV_HEADER_SIZE] = {'F', 'L', 'V', 1, (has_audio?0x04:0x00) | (has_video?0x01:0x00), 0, 0, 0, 9, 0, 0, 0, 0 };
    return FLV_HEADER_SIZE == fwrite (&h[0],1,FLV_HEADER_SIZE,flv);
}

int flv_read_tag (FILE* flv, flvtag_t* tag)
{
    uint32_t size;
    uint8_t h[FLV_TAG_HEADER_SIZE];

    if (FLV_TAG_HEADER_SIZE != fread (&h[0],1,FLV_TAG_HEADER_SIZE,flv)) {
        return 0;
    }

    size = ( (h[1]<<16) | (h[2]<<8) |h[3]);
    flvtag_reserve (tag, size);
    // copy header to buffer
    memcpy (tag->data,&h[0],FLV_TAG_HEADER_SIZE);

    if (size+FLV_TAG_FOOTER_SIZE != fread (&tag->data[FLV_TAG_HEADER_SIZE],1,size+FLV_TAG_FOOTER_SIZE,flv)) {
        return 0;
    }

    return 1;
}

int flv_write_tag (FILE* flv, flvtag_t* tag)
{
    size_t size = flvtag_raw_size (tag);
    return size == fwrite (flvtag_raw_data (tag),1,size,flv);
}
////////////////////////////////////////////////////////////////////////////////
size_t flvtag_header_size (flvtag_t* tag)
{
    switch (flvtag_type (tag)) {
    case flvtag_type_audio:
        return FLV_TAG_HEADER_SIZE + (flvtag_soundformat_aac != flvtag_soundformat (tag) ? 1 : 2);

    case flvtag_type_video:
        // CommandFrame does not have a compositionTime
        return FLV_TAG_HEADER_SIZE + (flvtag_codecid_avc != flvtag_codecid (tag) ? 1 : (flvtag_frametype_commandframe != flvtag_frametype (tag) ? 5 : 2));

    default:
        return FLV_TAG_HEADER_SIZE;
    }
}

size_t flvtag_payload_size (flvtag_t* tag)
{
    return FLV_TAG_HEADER_SIZE + flvtag_size (tag) - flvtag_header_size (tag);
}

uint8_t* flvtag_payload_data (flvtag_t* tag)
{
    size_t payload_offset = flvtag_header_size (tag);
    return &tag->data[payload_offset];
}
////////////////////////////////////////////////////////////////////////////////
int flvtag_updatesize (flvtag_t* tag, uint32_t size)
{
    tag->data[1] = size>>16; // DataSize
    tag->data[2] = size>>8; // DataSize
    tag->data[3] = size>>0; // DataSize
    size += 11;
    tag->data[size+0] = size>>24; // PrevTagSize
    tag->data[size+1] = size>>16; // PrevTagSize
    tag->data[size+2] = size>>8; // PrevTagSize
    tag->data[size+3] = size>>0; // PrevTagSize
    return 1;
}

#define FLVTAG_PREALOC 2048
int flvtag_initavc (flvtag_t* tag, uint32_t dts, int32_t cts, flvtag_frametype_t type)
{
    flvtag_init (tag);
    flvtag_reserve (tag,5+FLVTAG_PREALOC);
    tag->data[0] = flvtag_type_video;
    tag->data[4] = dts>>16;
    tag->data[5] = dts>>8;
    tag->data[6] = dts>>0;
    tag->data[7] = dts>>24;
    tag->data[8] = 0; // StreamID
    tag->data[9] = 0; // StreamID
    tag->data[10] = 0; // StreamID
    // VideoTagHeader
    tag->data[11] = ( (type<<4) %0xF0) |0x07; // CodecId
    tag->data[12] = 0x01; // AVC NALU
    tag->data[13] = cts>>16;
    tag->data[14] = cts>>8;
    tag->data[15] = cts>>0;
    flvtag_updatesize (tag,5);
    return 1;
}

int flvtag_initamf (flvtag_t* tag, uint32_t dts)
{
    flvtag_init (tag);
    flvtag_reserve (tag,FLVTAG_PREALOC);
    tag->data[0] = flvtag_type_scriptdata;
    tag->data[4] = dts>>16;
    tag->data[5] = dts>>8;
    tag->data[6] = dts>>0;
    tag->data[7] = dts>>24;
    tag->data[8] = 0; // StreamID
    tag->data[9] = 0; // StreamID
    tag->data[10] = 0; // StreamID
    flvtag_updatesize (tag,0);
    return 1;
}

// shamelessly taken from libtomcrypt, an public domain project
static void base64_encode (const unsigned char* in,  unsigned long inlen, unsigned char* out, unsigned long* outlen)
{
    static const char* codes = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned long i, len2, leven;
    unsigned char* p;

    /* valid output size ? */
    len2 = 4 * ( (inlen + 2) / 3);

    if (*outlen < len2 + 1) {
        *outlen = len2 + 1;
        fprintf (stderr,"\n\nHERE\n\n");
        return;
    }

    p = out;
    leven = 3* (inlen / 3);

    for (i = 0; i < leven; i += 3) {
        *p++ = codes[ (in[0] >> 2) & 0x3F];
        *p++ = codes[ ( ( (in[0] & 3) << 4) + (in[1] >> 4)) & 0x3F];
        *p++ = codes[ ( ( (in[1] & 0xf) << 2) + (in[2] >> 6)) & 0x3F];
        *p++ = codes[in[2] & 0x3F];
        in += 3;
    }

    if (i < inlen) {
        unsigned a = in[0];
        unsigned b = (i+1 < inlen) ? in[1] : 0;

        *p++ = codes[ (a >> 2) & 0x3F];
        *p++ = codes[ ( ( (a & 3) << 4) + (b >> 4)) & 0x3F];
        *p++ = (i+1 < inlen) ? codes[ ( ( (b & 0xf) << 2)) & 0x3F] : '=';
        *p++ = '=';
    }

    /* return ok */
    *outlen = p - out;
}

const char onCaptionInfo708[] = { 0x02,0x00,0x0D, 'o','n','C','a','p','t','i','o','n','I','n','f','o',
                                  0x08, 0x00, 0x00, 0x00, 0x02,
                                  0x00, 0x04, 't','y','p','e',
                                  0x02, 0x00, 0x03, '7','0','8',
                                  0x00, 0x04, 'd','a','t','a',
                                  0x02, 0x00,0x00
                                };

int flvtag_amfcaption_708 (flvtag_t* tag, uint32_t timestamp, sei_message_t* msg)
{
    flvtag_initamf (tag,timestamp);
    unsigned long size = 1 + (4 * ( (sei_message_size (msg) + 2) / 3));
    flvtag_reserve (tag, sizeof (onCaptionInfo708) + size + 3);
    memcpy (flvtag_payload_data (tag),onCaptionInfo708,sizeof (onCaptionInfo708));
    uint8_t* data = flvtag_payload_data (tag) + sizeof (onCaptionInfo708);
    base64_encode (sei_message_data (msg), sei_message_size (msg), data, &size);

    // Update the size of the base64 string
    data[-2] = size >> 8;
    data[-1] = size >> 0;
    // write the last array element
    data[size+0] = 0x00;
    data[size+1] = 0x00;
    data[size+2] = 0x09;
    flvtag_updatesize (tag, sizeof (onCaptionInfo708) + size + 3);

    return 1;
}

const char onCaptionInfoUTF8[] = { 0x02,0x00,0x0D, 'o','n','C','a','p','t','i','o','n','I','n','f','o',
                                   0x08, 0x00, 0x00, 0x00, 0x02,
                                   0x00, 0x04, 't','y','p','e',
                                   0x02, 0x00, 0x04, 'U','T','F','8',
                                   0x00, 0x04, 'd','a','t','a',
                                   0x02, 0x00,0x00
                                 };

#define MAX_AMF_STRING 65636
int flvtag_amfcaption_utf8 (flvtag_t* tag, uint32_t timestamp, const utf8_char_t* text)
{
    flvtag_initamf (tag,timestamp);
    unsigned long size = strlen (text);

    if (MAX_AMF_STRING < size) {
        size = MAX_AMF_STRING;
    }

    flvtag_reserve (tag, sizeof (onCaptionInfoUTF8) + size + 3);
    memcpy (flvtag_payload_data (tag),onCaptionInfoUTF8,sizeof (onCaptionInfoUTF8));
    uint8_t* data = flvtag_payload_data (tag) + sizeof (onCaptionInfo708);
    memcpy (data,text,size);
    // Update the size of the string
    data[-2] = size >> 8;
    data[-1] = size >> 0;
    // write the last array element
    data[size+0] = 0x00;
    data[size+1] = 0x00;
    data[size+2] = 0x09;
    flvtag_updatesize (tag, sizeof (onCaptionInfoUTF8) + size + 3);

    return 1;
}

#define LENGTH_SIZE 4

int flvtag_avcwritenal (flvtag_t* tag, uint8_t* data, size_t size)
{
    uint32_t flvsize = flvtag_size (tag);
    flvtag_reserve (tag,flvsize+LENGTH_SIZE+size);
    uint8_t* payload = tag->data + FLV_TAG_HEADER_SIZE + flvsize;
    payload[0] = size>>24; // nalu size
    payload[1] = size>>16;
    payload[2] = size>>8;
    payload[3] = size>>0;
    memcpy (&payload[LENGTH_SIZE],data,size);
    flvtag_updatesize (tag,flvsize+LENGTH_SIZE+size);

    return 1;
}

int flvtag_addcaption (flvtag_t* tag, const utf8_char_t* text)
{
    if (flvtag_avcpackettype_nalu != flvtag_avcpackettype (tag)) {
        return 0;
    }

    sei_t sei;
    caption_frame_t frame;

    sei_init (&sei);
    caption_frame_init (&frame);
    caption_frame_from_text (&frame, text);
    sei_from_caption_frame (&sei, &frame);

    uint8_t* sei_data = malloc (sei_render_size (&sei));
    size_t sei_size = sei_render (&sei, sei_data);

    // rewrite tag
    flvtag_t new_tag;
    flvtag_initavc (&new_tag, flvtag_dts (tag), flvtag_cts (tag), flvtag_frametype (tag));
    uint8_t* data = flvtag_payload_data (tag);
    ssize_t  size = flvtag_payload_size (tag);

    while (0<size) {
        uint8_t*  nalu_data = &data[LENGTH_SIZE];
        uint8_t   nalu_type = nalu_data[0]&0x1F;
        uint32_t  nalu_size = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
        data += LENGTH_SIZE + nalu_size;
        size -= LENGTH_SIZE + nalu_size;

        if (0 < sei_size && 7 != nalu_type && 8 != nalu_type && 9 != nalu_type ) {
            // fprintf (stderr,"Wrote SEI %d '%d'\n\n", sei_size, sei_data[3]);
            flvtag_avcwritenal (&new_tag,sei_data,sei_size);
            sei_size = 0;
        }

        flvtag_avcwritenal (&new_tag,nalu_data,nalu_size);
    }

    // On the off chance we have an empty frame,
    // We still wish to append the sei
    if (0<sei_size) {
        // fprintf (stderr,"Wrote SEI %d\n\n", sei_size);
        flvtag_avcwritenal (&new_tag,sei_data,sei_size);
        sei_size = 0;
    }

    if (sei_data) {
        free (sei_data);
    }

    free (tag->data);
    sei_free (&sei);
    tag->data = new_tag.data;
    tag->aloc = new_tag.aloc;
    return 1;
}
