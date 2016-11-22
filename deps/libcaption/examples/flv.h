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
#ifndef LIBCAPTION_FLV_H
#define LIBCAPTION_FLV_H

#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#define FLV_HEADER_SIZE     13
#define FLV_FOOTER_SIZE      4
#define FLV_TAG_HEADER_SIZE 11
#define FLV_TAG_FOOTER_SIZE  4
////////////////////////////////////////////////////////////////////////////////
#include "avc.h"
////////////////////////////////////////////////////////////////////////////////
typedef struct {
    uint8_t* data;
    size_t   aloc;
} flvtag_t;

void flvtag_init (flvtag_t* tag);
void flvtag_free (flvtag_t* tag);
void flvtag_swap (flvtag_t* tag1, flvtag_t* tag2);
////////////////////////////////////////////////////////////////////////////////
typedef enum {
    flvtag_type_audio      = 0x08,
    flvtag_type_video      = 0x09,
    flvtag_type_scriptdata = 0x12,
} flvtag_type_t;

static inline flvtag_type_t flvtag_type (flvtag_t* tag) { return (flvtag_type_t) tag->data[0]&0x1F; }
////////////////////////////////////////////////////////////////////////////////
typedef enum {
    flvtag_soundformat_unknown                            = -1,
    flvtag_soundformat_linearpcmplatformendian            =  0,
    flvtag_soundformat_adpcm                              =  1,
    flvtag_soundformat_mp3                                =  2,
    flvtag_soundformat_linearpcmlittleendian              =  3,
    flvtag_soundformat_nellymoser_16khzmono               =  4,
    flvtag_soundformat_nellymoser_8khzmono                =  5,
    flvtag_soundformat_nellymoser                         =  6,
    flvtag_soundformat_g711alawlogarithmicpcm             =  7,
    flvtag_soundformat_g711mulawlogarithmicpcm            =  8,
    flvtag_soundformat_reserved                           =  9,
    flvtag_soundformat_aac                                = 10,
    flvtag_soundformat_speex                              = 11,
    flvtag_soundformat_mp3_8khz                           = 14,
    flvtag_soundformat_devicespecificsound                = 15
} flvtag_soundformat_t;

static inline flvtag_soundformat_t flvtag_soundformat (flvtag_t* tag) { return (flvtag_type_audio!=flvtag_type (tag)) ?flvtag_soundformat_unknown: (flvtag_soundformat_t) (tag->data[0]>>4) &0x0F; }
////////////////////////////////////////////////////////////////////////////////
typedef enum {
    flvtag_codecid_unknown                = -1,
    flvtag_codecid_sorensonh263           =  2,
    flvtag_codecid_screenvideo            =  3,
    flvtag_codecid_on2vp6                 =  4,
    flvtag_codecid_on2vp6withalphachannel =  5,
    flvtag_codecid_screenvideoversion2    =  6,
    flvtag_codecid_avc                    =  7
} flvtag_codecid_t;

static inline flvtag_codecid_t flvtag_codecid (flvtag_t* tag) { return (flvtag_type_video!=flvtag_type (tag)) ? (flvtag_codecid_unknown) : (tag->data[11]&0x0F); }
////////////////////////////////////////////////////////////////////////////////
typedef enum {
    flvtag_frametype_unknown              = -1,
    flvtag_frametype_keyframe             =  1,
    flvtag_frametype_interframe           =  2,
    flvtag_frametype_disposableinterframe =  3,
    flvtag_frametype_generatedkeyframe    =  4,
    flvtag_frametype_commandframe         =  5
} flvtag_frametype_t;

static inline flvtag_frametype_t flvtag_frametype (flvtag_t* tag) { return (flvtag_type_video!=flvtag_type (tag)) ?flvtag_frametype_keyframe: ( (tag->data[11]>>4) &0x0F); }
////////////////////////////////////////////////////////////////////////////////
typedef enum {
    flvtag_avcpackettype_unknown        = -1,
    flvtag_avcpackettype_sequenceheader =  0,
    flvtag_avcpackettype_nalu           =  1,
    flvtag_avcpackettype_endofsequence  =  2
} flvtag_avcpackettype_t;

static inline flvtag_avcpackettype_t flvtag_avcpackettype (flvtag_t* tag) { return (flvtag_codecid_avc!=flvtag_codecid (tag)) ?flvtag_avcpackettype_unknown:tag->data[12]; }
////////////////////////////////////////////////////////////////////////////////
static inline size_t   flvtag_size (flvtag_t* tag) { return (tag->data[1]<<16) | (tag->data[2]<<8) | tag->data[3]; }
static inline uint32_t flvtag_timestamp (flvtag_t* tag) { return (tag->data[7]<<24) | (tag->data[4]<<16) | (tag->data[5]<<8) | tag->data[6]; }
static inline uint32_t flvtag_dts (flvtag_t* tag) { return flvtag_timestamp (tag); }
static inline uint32_t flvtag_cts (flvtag_t* tag) { return (flvtag_avcpackettype_nalu!=flvtag_avcpackettype (tag)) ?0: (tag->data[13]<<16) | (tag->data[14]<<8) |tag->data[15]; }
static inline uint32_t flvtag_pts (flvtag_t* tag) { return flvtag_dts (tag)+flvtag_cts (tag); }
static inline double flvtag_dts_seconds (flvtag_t* tag) { return flvtag_dts (tag) / 1000.0; }
static inline double flvtag_cts_seconds (flvtag_t* tag) { return flvtag_cts (tag) / 1000.0; }
static inline double flvtag_pts_seconds (flvtag_t* tag) { return flvtag_pts (tag) / 1000.0; }
////////////////////////////////////////////////////////////////////////////////
size_t flvtag_header_size (flvtag_t* tag);
size_t flvtag_payload_size (flvtag_t* tag);
uint8_t* flvtag_payload_data (flvtag_t* tag);
////////////////////////////////////////////////////////////////////////////////
FILE* flv_open_read (const char* flv);
FILE* flv_open_write (const char* flv);
FILE* flv_close (FILE* flv);
////////////////////////////////////////////////////////////////////////////////
static inline const uint8_t* flvtag_raw_data (flvtag_t* tag) { return tag->data; }
static inline const size_t flvtag_raw_size (flvtag_t* tag) { return flvtag_size (tag)+FLV_TAG_HEADER_SIZE+FLV_TAG_FOOTER_SIZE; }
////////////////////////////////////////////////////////////////////////////////
int flv_read_tag (FILE* flv, flvtag_t* tag);
int flv_write_tag (FILE* flv, flvtag_t* tag);
int flv_read_header (FILE* flv, int* has_audio, int* has_video);
int flv_write_header (FILE* flv, int has_audio, int has_video);
////////////////////////////////////////////////////////////////////////////////
// If the tage has more that on sei message, they will be combined into one
sei_t* flv_read_sei (FILE* flv, flvtag_t* tag);
////////////////////////////////////////////////////////////////////////////////
int flvtag_initavc (flvtag_t* tag, uint32_t dts, int32_t cts, flvtag_frametype_t type);
int flvtag_avcwritenal (flvtag_t* tag, uint8_t* data, size_t size);
int flvtag_addcaption (flvtag_t* tag, const utf8_char_t* text);
////////////////////////////////////////////////////////////////////////////////
int flvtag_amfcaption_708 (flvtag_t* tag, uint32_t timestamp, sei_message_t* msg);
////////////////////////////////////////////////////////////////////////////////
// This method is expermental, and not currently available on Twitch
int flvtag_amfcaption_utf8 (flvtag_t* tag, uint32_t timestamp, const utf8_char_t* text);
#endif
