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
#ifndef LIBCAPTION_TS_H
#define LIBCAPTION_TS_H
#include "caption.h"
typedef struct {
    int16_t pmtpid;
    int16_t avcpid;
    int64_t pts;
    int64_t dts;
    size_t  size;
    const uint8_t* data;
} ts_t;

/*! \brief
    \param

    Expects 188 byte TS packet
*/
#define TS_PACKET_SIZE 188
void ts_init (ts_t* ts);
int ts_parse_packet (ts_t* ts, const uint8_t* data);
// return timestamp in seconds
static inline double ts_dts_seconds (ts_t* ts) { return ts->dts / 90000.0; }
static inline double ts_pts_seconds (ts_t* ts) { return ts->pts / 90000.0; }
static inline double ts_cts_seconds (ts_t* ts) { return (ts->dts - ts->pts) / 90000.0; }

#endif
