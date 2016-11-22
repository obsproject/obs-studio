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
#include "srt.h"
#include "avc.h"

#define LENGTH_SIZE 4
int main (int argc, char** argv)
{
    const char* path = argv[1];

    sei_t sei;
    flvtag_t tag;
    srt_t* srt = 0, *head = 0;
    int i, has_audio, has_video;
    caption_frame_t frame;

    flvtag_init (&tag);
    caption_frame_init (&frame);

    FILE* flv = flv_open_read (path);

    if (!flv_read_header (flv,&has_audio,&has_video)) {
        fprintf (stderr,"'%s' Not an flv file\n", path);
    } else {
        fprintf (stderr,"Reading from '%s'\n", path);
    }

    while (flv_read_tag (flv,&tag)) {
        if (flvtag_avcpackettype_nalu == flvtag_avcpackettype (&tag)) {
            ssize_t  size = flvtag_payload_size (&tag);
            uint8_t* data = flvtag_payload_data (&tag);

            while (0<size) {
                ssize_t  nalu_size = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
                uint8_t* nalu_data = &data[4];
                uint8_t  nalu_type = nalu_data[0]&0x1F;
                data += nalu_size + LENGTH_SIZE;
                size -= nalu_size + LENGTH_SIZE;

                if (6 == nalu_type) {
                    sei_init (&sei);
                    sei_parse_nalu (&sei, nalu_data, nalu_size, flvtag_dts (&tag), flvtag_cts (&tag));

                    cea708_t cea708;
                    sei_message_t* msg;
                    cea708_init (&cea708);

                    // for (msg = sei_message_head (&sei) ; msg ; msg = sei_message_next (msg)) {
                    //     if (sei_type_user_data_registered_itu_t_t35 == sei_message_type (msg)) {
                    //         cea708_parse (sei_message_data (msg), sei_message_size (msg), &cea708);
                    //         cea708_dump (&cea708);
                    //     }
                    // }

                    // sei_dump(&sei);
                    sei_to_caption_frame (&sei,&frame);
                    sei_free (&sei);

                    // caption_frame_dump (&frame);
                    srt = srt_from_caption_frame (&frame,srt,&head);
                }
            }
        }
    }

    srt_dump (head);
    srt_free (head);

    return 1;
}
