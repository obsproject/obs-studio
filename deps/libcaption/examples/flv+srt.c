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
#include "srt.h"
#include "flv.h"
#include "avc.h"
// #include "sei.h"

#define MAX_SRT_SIZE (10*1024*1024)
#define MAX_READ_SIZE 4096

srt_t* srt_from_file (const char* path)
{
    srt_t* head = 0;
    size_t read, totl = 0;
    FILE* file = fopen (path,"r");

    if (file) {
        char* srt_data = malloc (MAX_SRT_SIZE);
        size_t srt_size = 0;
        size_t size = MAX_SRT_SIZE;
        char* data = srt_data;

        while (0 < (read = fread (data,1,size,file))) {
            totl += read; data += read; size -= read; srt_size += read;
        }

        head = srt_parse (srt_data,srt_size);
        free (srt_data);
    }

    return head;
}

int main (int argc, char** argv)
{
    flvtag_t tag;
    FILE* flv = flv_open_read (argv[1]);
    FILE* out = flv_open_write (argv[3]);

    int has_audio, has_video;
    flvtag_init (&tag);

    if (!flv_read_header (flv,&has_audio,&has_video)) {
        fprintf (stderr,"%s is not an flv file\n", argv[1]);
        return EXIT_FAILURE;
    }

    srt_t* head = srt_from_file (argv[2]);
    srt_t* srt = head;

    if (! head) {
        fprintf (stderr,"%s is not an srt file\n", argv[2]);
        return EXIT_FAILURE;
    }

    flv_write_header (out,has_audio,has_video);

    while (flv_read_tag (flv,&tag)) {
        // TODO handle B freame!
        if (srt && flvtag_pts_seconds (&tag) >= srt->timestamp && flvtag_avcpackettype_nalu == flvtag_avcpackettype (&tag)) {
            fprintf (stderr,"%f: %s\n", srt->timestamp, srt_data (srt));
            flvtag_addcaption (&tag, srt_data (srt));
            srt = srt->next;
        }

        flv_write_tag (out,&tag);
        // Write tag out here
    }

    srt_free (head);
    return EXIT_SUCCESS;
}
