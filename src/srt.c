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

#define SRTTIME2MS(HH,MM,SS,MS) ((HH*3600.0 + MM*60.0 + SS) * 1000.0 + MS)
int srt_parse_time (srt_t* srt, const char* line)
{
    int hh1, hh2, mm1, mm2, ss1, ss2, ms1, ms2;

    if (8 == sscanf (line, "%d:%2d:%2d%*1[,.]%3d --> %d:%2d:%2d%*1[,.]%3d", &hh1, &mm1, &ss1, &ms1, &hh2, &mm2, &ss2, &ms2)) {
        srt->str_pts = SRTTIME2MS (hh1, mm1, ss1, ms1);
        srt->end_pts = SRTTIME2MS (hh2, mm2, ss2, ms2);
        return 1;
    }

    return 0;
}

void srt_free (srt_t* srt)
{
    srt_t* next;

    while (srt) {
        next = srt->next;
        free (srt);
        srt = next;
    }
}


srt_t* srt_new (const utf8_char_t* data, size_t size)
{
    srt_t* srt = malloc (sizeof (srt_t)+size+1);
    srt->str_pts = 0;
    srt->end_pts = 0;
    srt->next = 0;
    srt->aloc = size;
    utf8_char_t* dest = (utf8_char_t*) srt_data (srt);

    if (data) {
        memcpy (dest, data, size);
    } else {
        memset (dest, 0, size);
    }

    dest[size] = '\0';
    return srt;
}

srt_t* srt_parse (const utf8_char_t* data, size_t size)
{
    int counter;
    double str_pts, end_pts;
    srt_t* head = 0, *prev = 0;
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
                str_pts = SRTTIME2MS (hh1, mm1, ss1, ms1);
                end_pts = SRTTIME2MS (hh2, mm2, ss2, ms2);
            } else {

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
        srt_t* srt = srt_new (text,text_size);
        srt->str_pts = str_pts;
        srt->end_pts = end_pts;

        if (! prev) {  head = srt; }
        else { prev->next = srt; }

        prev = srt;
    }

    return head;
}

int srt_to_caption_frame (srt_t* srt, caption_frame_t* frame)
{
    const char* data = srt_data (srt);
    return caption_frame_from_text (frame,data);
}

srt_t* srt_from_caption_frame (caption_frame_t* frame, srt_t* prev)
{
    int r, c, x, s, uln;
    eia608_style_t sty;
    // CRLF per row, plus an extra at the end
    srt_t* srt = srt_new (0, 2+CAPTION_FRAME_TEXT_BYTES);
    utf8_char_t* data = srt_data (srt);
    srt->str_pts = frame->str_pts;
    srt->end_pts = frame->end_pts;

    if (prev) {
        prev->next = srt;
    }

    caption_frame_to_text (frame,data);
    // srt requires an extra new line
    strcat ( (char*) data,"\r\n");

    return srt;
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
        ms1 = (int) (srt->str_pts) % 1000;
        ss1 = (int) (srt->str_pts / 1000) % 60;
        mm1 = (int) (srt->str_pts / (1000*60)) % 60;
        hh1 = (int) (srt->str_pts / (1000*60*60));
        ms2 = (int) (srt->end_pts) % 1000;
        ss2 = (int) (srt->end_pts / 1000) % 60;
        mm2 = (int) (srt->end_pts / (1000*60)) % 60;
        hh2 = (int) (srt->end_pts / (1000*60*60));

        if ('s' == type) {
            printf ("%02d\r\n%d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\r\n%s\r\n", i,
                    hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2, srt_data (srt));
        }

        else if ('v' == type) {
            printf ("%d:%02d:%02d --> %02d:%02d:%02d\r\n%s\r\n",
                    hh1, mm1, ss1, hh2, mm2, ss2, srt_data (srt));

        }

    }
}

void srt_dump (srt_t* head) { _dump (head,'s'); }
void vtt_dump (srt_t* head) { _dump (head,'v'); }
