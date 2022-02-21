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
// http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_XDS.HTML#PR
#include "xds.h"
#include "caption.h"
#include <string.h>

void xds_init(xds_t* xds)
{
    memset(xds, 0, sizeof(xds_t));
}

int xds_decode(xds_t* xds, uint16_t cc)
{
    switch (xds->state) {
    default:
    case 0:
        xds_init(xds);
        xds->class_code = (cc & 0x0F00) >> 8;
        xds->type = (cc & 0x000F);
        xds->state = 1;
        return LIBCAPTION_OK;

    case 1:
        if (0x8F00 == (cc & 0xFF00)) {
            xds->checksum = (cc & 0x007F);
            xds->state = 0;
            return LIBCAPTION_READY;
        }

        if (xds->size < 32) {
            xds->content[xds->size + 0] = (cc & 0x7F00) >> 8;
            xds->content[xds->size + 1] = (cc & 0x007F);
            xds->size += 2;
            return LIBCAPTION_OK;
        }
    }

    xds->state = 0;
    return LIBCAPTION_ERROR;
}
