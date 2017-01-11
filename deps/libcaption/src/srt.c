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
#include "srt.h"
#include "utf8.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



srt_t* srt_new (const utf8_char_t* data, size_t size, double timestamp, srt_t* prev, srt_t** head)
{
    srt_t* srt = malloc (sizeof (srt_t)+size+1);
    srt->next = 0;
    srt->duration = 0;
    srt->aloc = size;
    srt->timestamp = timestamp;
    utf8_char_t* dest = (utf8_char_t*) srt_data (srt);

    if (prev) {
        prev->next = srt;
        prev->duration = timestamp - prev->timestamp;
    }

    if (head && 0 == (*head)) {
        (*head) = srt;
    }

    if (data) {
        memcpy (dest, data, size);
    } else {
        memset (dest, 0, size);
    }

    dest[size] = '\0';
    return srt;
}

srt_t* srt_free_head (srt_t* head)
{
    srt_t* next = head->next;
    free (head);
    return next;
}

void srt_free (srt_t* srt)
{
    while (srt) {
        srt = srt_free_head (srt);
    }
}

#define SRTTIME2SECONDS(HH,MM,SS,MS) ((HH*3600.0) + (MM*60.0) + SS + (MS/1000.0))
srt_t* srt_parse (const utf8_char_t* data, size_t size)
{
    int counter;
    srt_t* head = 0, *prev = 0;
    double str_pts = 0, end_pts = 0;
    size_t line_length = 0, trimmed_length = 0;
    int hh1, hh2, mm1, mm2, ss1, ss2, ms1, ms2;

    for (;;) {
        line_length = 0;

        do {
            data += line_length;
            size -= line_length;
            line_length = utf8_line_length (data);
            trimmed_length = utf8_trimmed_length (data,line_length);
            // Skip empty lines
        } while (0 < line_length && 0 == trimmed_length);

        // linelength cant be zero before EOF
        if (0 == line_length) {
            break;
        }

        counter = atoi (data);
        // printf ("counter (%d): '%.*s'\n", line_length, (int) line_length, data);
        data += line_length;
        size -= line_length;

        line_length = utf8_line_length (data);
        // printf ("time (%d): '%.*s'\n", line_length, (int) line_length, data);

        {
            if (8 == sscanf (data, "%d:%2d:%2d%*1[,.]%3d --> %d:%2d:%2d%*1[,.]%3d", &hh1, &mm1, &ss1, &ms1, &hh2, &mm2, &ss2, &ms2)) {
                str_pts = SRTTIME2SECONDS (hh1, mm1, ss1, ms1);
                end_pts = SRTTIME2SECONDS (hh2, mm2, ss2, ms2);
            }

            data += line_length;
            size -= line_length;
        }

        // Caption text starts here
        const utf8_char_t* text = data;
        size_t text_size = 0;
        // printf ("time: '(%f --> %f)\n",srt.srt_time, srt.end_time);

        do {
            line_length = utf8_line_length (data);
            trimmed_length = utf8_trimmed_length (data,line_length);
            // printf ("cap (%d): '%.*s'\n", line_length, (int) trimmed_length, data);
            data += line_length;
            size -= line_length;
            text_size += line_length;
        } while (trimmed_length);

        // should we trim here?
        srt_t* srt = srt_new (text,text_size,str_pts,prev,&head);
        srt->duration = end_pts - str_pts;
        prev = srt;
    }

    return head;
}

int srt_to_caption_frame (srt_t* srt, caption_frame_t* frame)
{
    const char* data = srt_data (srt);
    return caption_frame_from_text (frame,data);
}

srt_t* srt_from_caption_frame (caption_frame_t* frame, srt_t* prev, srt_t** head)
{
    // CRLF per row, plus an extra at the end
    srt_t* srt = srt_new (0, 2+CAPTION_FRAME_TEXT_BYTES, frame->timestamp, prev, head);
    utf8_char_t* data = srt_data (srt);

    caption_frame_to_text (frame,data);
    // srt requires an extra new line
    strcat ( (char*) data,"\r\n");

    return srt;
}

static inline void _crack_time (double tt, int* hh, int* mm, int* ss, int* ms)
{
    (*ms) = (int) ((int64_t) (tt * 1000.0) % 1000);
    (*ss) = (int) ((int64_t) (tt) % 60);
    (*mm) = (int) ((int64_t) (tt / (60.0)) % 60);
    (*hh) = (int) ((int64_t) (tt / (60.0*60.0)));
}

static void _dump (srt_t* head, char type)
{
    int i;
    srt_t* srt;

    if ('v' == type) {
        printf ("WEBVTT\r\n");
    }

    for (srt = head, i = 1; srt; srt=srt_next (srt), ++i) {
        int hh1, hh2, mm1, mm2, ss1, ss2, ms1, ms2;
        _crack_time (srt->timestamp, &hh1, &mm1, &ss1, &ms1);
        _crack_time (srt->timestamp + srt->duration, &hh2, &mm2, &ss2, &ms2);

        if ('s' == type) {
            printf ("%02d\r\n%d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\r\n%s\r\n", i,
                    hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2, srt_data (srt));
        }

        else if ('v' == type) {
            printf ("%d:%02d:%02d.%03d --> %02d:%02d:%02d.%03d\r\n%s\r\n",
                    hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2, srt_data (srt));
        }
    }
}

void srt_dump (srt_t* head) { _dump (head,'s'); }
void vtt_dump (srt_t* head) { _dump (head,'v'); }
