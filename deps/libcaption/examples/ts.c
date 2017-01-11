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
#include "ts.h"
#include <string.h>

void ts_init (ts_t* ts)
{
    memset (ts,0,sizeof (ts_t));
}

static int64_t ts_parse_pts (const uint8_t* data)
{
    // 0000 1110  1111 1111  1111 1110  1111 1111  1111 1110
    uint64_t pts = 0;
    pts |= (uint64_t) (data[0] & 0x0E) << 29;
    pts |= (uint64_t) (data[1] & 0xFF) << 22;
    pts |= (uint64_t) (data[2] & 0xFE) << 14;
    pts |= (uint64_t) (data[3] & 0xFF) <<  7;
    pts |= (uint64_t) (data[4] & 0xFE) >>  1;
    return pts;
}

int ts_parse_packet (ts_t* ts, const uint8_t* data)
{
    size_t i = 0;
    int pusi = !! (data[i + 1] & 0x40);  // Payload Unit Start Indicator
    int16_t pid = ( (data[i + 1] & 0x1F) << 8) | data[i + 2];    // PID
    int adaption_present = !! (data[i + 3] & 0x20);  // Adaptation field exist
    int payload_present = !! (data[i + 3] & 0x10);  // Contains payload
    i += 4;

    ts->data = 0;
    ts->size = 0;

    if (adaption_present) {
        uint8_t adaption_length = data[i + 0]; // adaption field length
        i += 1 + adaption_length;
    }

    if (pid == 0) {
        if (payload_present) {
            // Skip the payload.
            i += data[i] + 1;
        }

        ts->pmtpid = ( (data[i + 10] & 0x1F) << 8) | data[i + 11];
    } else if (pid == ts->pmtpid) {
        // PMT
        if (payload_present) {
            // Skip the payload.
            i += data[i] + 1;
        }

        uint16_t section_length = ( (data[i + 1] & 0x0F) << 8) | data[i + 2];
        int current = data[i + 5] & 0x01;
        int16_t program_info_length = ( (data[i + 10] & 0x0F) << 8) | data[i + 11];
        int16_t descriptor_loop_length = section_length - (9 + program_info_length + 4);   // 4 for the crc

        i += 12 + program_info_length;

        if (current) {
            while (descriptor_loop_length >= 5) {
                uint8_t stream_type    = data[i];
                int16_t elementary_pid = ( (data[i + 1] & 0x1F) << 8) | data[i + 2];
                int16_t esinfo_length  = ( (data[i + 3] & 0x0F) << 8) | data[i + 4];

                if (0x1B == stream_type) {
                    ts->avcpid = elementary_pid;
                }

                i += 5 + esinfo_length;
                descriptor_loop_length -= 5 + esinfo_length;
            }
        }
    } else if (payload_present && pid == ts->avcpid) {
        if (pusi) {
            // int data_alignment = !! (data[i + 6] & 0x04);
            int has_pts = !! (data[i + 7] & 0x80);
            int has_dts = !! (data[i + 7] & 0x40);
            uint8_t header_length = data[i + 8];

            if (has_pts) {
                ts->pts = ts_parse_pts (&data[i + 9]);
                ts->dts = has_dts ? ts_parse_pts (&data[i + 14]) : ts->pts;
            }

            i += 9 + header_length;
        }

        ts->data = &data[i];
        ts->size = TS_PACKET_SIZE-i;
        return LIBCAPTION_READY;
    }

    return LIBCAPTION_OK;
}
