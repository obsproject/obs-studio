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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flv.h"
#include "avc.h"
#include "srt.h"
#include "wonderland.h"

#define SECONDS_PER_LINE 3.0
srt_t* appennd_caption (const utf8_char_t* data, srt_t* prev, srt_t** head)
{

    int r, c, chan = 0;
    ssize_t size = (ssize_t) strlen (data);
    size_t char_count, char_length, line_length = 0, trimmed_length = 0;

    for (r = 0 ; 0 < size && SCREEN_ROWS > r ; ++r) {
        line_length = utf8_line_length (data);
        trimmed_length = utf8_trimmed_length (data,line_length);
        char_count = utf8_char_count (data,trimmed_length);

        // If char_count is greater than one line can display, split it.
        if (SCREEN_COLS < char_count) {
            char_count = utf8_wrap_length (data,SCREEN_COLS);
            line_length = utf8_string_length (data,char_count+1);
        }

        // fprintf (stderr,"%.*s\n", line_length, data);
        prev = srt_new (data, line_length, prev ? prev->timestamp + SECONDS_PER_LINE : 0, prev, head);

        data += line_length;
        size -= (ssize_t) line_length;
    }

    return prev;
}

int main (int argc, char** argv)
{
    int i = 0;
    flvtag_t tag;
    srt_t* head = 0, *tail = 0;
    int has_audio, has_video;
    FILE* flv = flv_open_read (argv[1]);
    FILE* out = flv_open_write (argv[2]);
    flvtag_init (&tag);

    for (i = 0 ; wonderland[i][0]; ++i) {
        tail = appennd_caption (wonderland[i], tail, &head);
    }


    if (!flv_read_header (flv,&has_audio,&has_video)) {
        fprintf (stderr,"%s is not an flv file\n", argv[1]);
        return EXIT_FAILURE;
    }

    flv_write_header (out,has_audio,has_video);

    while (flv_read_tag (flv,&tag)) {
        if (head && flvtag_avcpackettype_nalu == flvtag_avcpackettype (&tag) && head->timestamp <= flvtag_pts_seconds (&tag)) {
            fprintf (stderr,"%f %s\n", flvtag_pts_seconds (&tag), srt_data (head));
            flvtag_addcaption (&tag, srt_data (head));
            head = srt_free_head (head);
        }


        flv_write_tag (out,&tag);
    }

    return EXIT_SUCCESS;
}
