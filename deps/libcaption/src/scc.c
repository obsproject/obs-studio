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
#include "scc.h"
#include "utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static scc_t* scc_relloc(scc_t* scc, unsigned int cc_count)
{
    if (0 == scc || scc->cc_aloc < cc_count) {
        // alloc 1.5 time what is asked for.
        scc = (scc_t*)realloc(scc, sizeof(scc_t) + ((cc_count * 15 / 10) * sizeof(uint16_t)));
        scc->cc_aloc = cc_count;
    }

    return scc;
}

scc_t* scc_new(int cc_count)
{
    scc_t* scc = scc_relloc(0, cc_count);
    scc->timestamp = 0.0;
    scc->cc_size = 0;
    return scc;
}

scc_t* scc_free(scc_t* scc)
{
    free(scc);
    return NULL;
}

double scc_time_to_timestamp(int hh, int mm, int ss, int ff)
{
    return (hh * 3600.0) + (mm * 60.0) + ss + (ff / 29.97);
}

// 00:00:25:16  9420 9440 aeae ae79 ef75 2068 6176 e520 79ef 75f2 20f2 ef62 eff4 e9e3 732c 2061 6e64 2049 94fe 9723 ea75 73f4 20f7 616e f420 f4ef 2062 e520 61f7 e573 ef6d e520 e96e 2073 7061 e3e5 ae80 942c 8080 8080 942f
size_t scc_to_608(scc_t** scc, const utf8_char_t* data)
{
    size_t llen, size = 0;
    int v1 = 0, v2 = 0, hh = 0, mm = 0, ss = 0, ff = 0, cc_data = 0;

    if (0 == data) {
        return 0;
    }

    if ((*scc)) {
        (*scc)->cc_size = 0;
    }

    // skip 'Scenarist_SCC V1.0' header
    if (2 == sscanf(data, "Scenarist_SCC V%1d.%1d", &v1, &v2)) {
        data += 18, size += 18;

        if (1 != v1 || 0 != v2) {
            return 0;
        }
    }

    // Skip blank lines
    for (;;) {
        llen = utf8_line_length(data);

        if (0 == llen || 0 != utf8_trimmed_length(data, llen)) {
            break;
        }

        data += llen;
        size += llen;
    }

    if (4 == sscanf(data, "%2d:%2d:%2d%*1[:;]%2d", &hh, &mm, &ss, &ff)) {
        data += 12, size += 12;
        // Get length of the remaining charcters
        llen = utf8_line_length(data);
        llen = utf8_trimmed_length(data, llen);
        unsigned int max_cc_count = 1 + ((unsigned int)llen / 5);
        (*scc) = scc_relloc((*scc), max_cc_count * 15 / 10);
        (*scc)->timestamp = scc_time_to_timestamp(hh, mm, ss, ff);
        (*scc)->cc_size = 0;

        while ((*scc)->cc_size < max_cc_count && 1 == sscanf(data, "%04x", &cc_data)) {
            (*scc)->cc_data[(*scc)->cc_size] = (uint16_t)cc_data;
            (*scc)->cc_size += 1;
            data += 5, size += 5;
        }
    }

    return size;
}
