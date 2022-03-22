/**********************************************************************************************/
/* The MIT License                                                                            */
/*                                                                                            */
/* Copyright 2016-2017 Twitch Interactive, Inc. or its affiliates. All Rights Reserved.       */
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
#include "vtt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

srt_t* srt_new()
{
    return vtt_new();
}

void srt_free(srt_t* srt)
{
    vtt_free(srt);
}

vtt_t* srt_parse(const utf8_char_t* data, size_t size)
{
    return _vtt_parse(data, size, 1);
}

void srt_dump(srt_t* srt)
{
    int i;
    vtt_block_t* block;

    for (block = srt->cue_head, i = 1; block; block = block->next, ++i) {
        int hh1, hh2, mm1, mm2, ss1, ss2, ms1, ms2;
        vtt_crack_time(block->timestamp, &hh1, &mm1, &ss1, &ms1);
        vtt_crack_time(block->timestamp + block->duration, &hh2, &mm2, &ss2, &ms2);

        printf("%02d\r\n%d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\r\n%s\r\n", i,
            hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2, vtt_block_data(block));
    }
}
