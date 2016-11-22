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
#include "srt.h"
#include "avc.h"
#include <stdio.h>

int main (int argc, char** argv)
{
    const char* path = argv[1];

    ts_t ts;
    sei_t sei;
    avcnalu_t nalu;
    srt_t* srt = 0, *head = 0;
    caption_frame_t frame;
    uint8_t pkt[TS_PACKET_SIZE];
    ts_init (&ts);
    avcnalu_init (&nalu);
    caption_frame_init (&frame);

    FILE* file = fopen (path,"rb+");

    while (TS_PACKET_SIZE == fread (&pkt[0],1,TS_PACKET_SIZE, file)) {
        switch (ts_parse_packet (&ts,&pkt[0])) {
        case LIBCAPTION_OK:
            // fprintf (stderr,"read ts packet\n");
            break;

        case LIBCAPTION_READY: {
            // fprintf (stderr,"read ts packet DATA\n");
            while (ts.size) {
                // fprintf (stderr,"ts.size %d (%02X%02X%02X%02X)\n",ts.size, ts.data[0], ts.data[1], ts.data[2], ts.data[3]);

                switch (avcnalu_parse_annexb (&nalu, &ts.data, &ts.size)) {
                case LIBCAPTION_OK:
                    break;

                case LIBCAPTION_ERROR:
                    // fprintf (stderr,"LIBCAPTION_ERROR == avcnalu_parse_annexb()\n");
                    avcnalu_init (&nalu);
                    break;

                case LIBCAPTION_READY: {

                    if (6 == avcnalu_type (&nalu)) {
                        // fprintf (stderr,"NALU %d (%d)\n", avcnalu_type (&nalu), avcnalu_size (&nalu));
                        sei_init (&sei);
                        sei_parse_avcnalu (&sei, &nalu, ts_dts_seconds (&ts), ts_cts_seconds (&ts));

                        // sei_dump (&sei);

                        if (LIBCAPTION_READY == sei_to_caption_frame (&sei,&frame)) {
                            // caption_frame_dump (&frame);
                            srt = srt_from_caption_frame (&frame,srt,&head);

                            // srt_dump (srt);
                        }

                        sei_free (&sei);
                    }

                    avcnalu_init (&nalu);
                } break;
                }
            }
        } break;

        case LIBCAPTION_ERROR:
            // fprintf (stderr,"read ts packet ERROR\n");
            break;
        }

    }

    srt_dump (head);
    srt_free (head);

    return 1;
}
