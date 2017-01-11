/**********************************************************************************************/
/* Copyright 2016-2016 Twitch Interactive, Inc. or its affiliates. All Rights Reserved.       */
/*                                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file  */
/* except in compliance with the License. A copy of the License is located at                 */
/*                                                                                            */
/*     http://aws.amazon.com/apache2.0/                                                       */
/*                                                                                            */
/* or in the "license" file accompanying this file. This file is distributed on an "AS IS"    */
/* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the    */
/* License for the specific language governing permissions and limitations under the License. */
/**********************************************************************************************/
// http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_XDS.HTML#PR
#include "xds.h"
#include "caption.h"
#include <string.h>

void xds_init (xds_t* xds)
{
    memset (xds,0,sizeof (xds_t));
}

int xds_decode (xds_t* xds, uint16_t cc)
{
    switch (xds->state) {
    default:
    case 0:
        xds_init (xds);
        xds->class = (cc&0x0F00) >>8;
        xds->type = (cc&0x000F);
        xds->state = 1;
        return LIBCAPTION_OK;

    case 1:
        if (0x8F00 == (cc&0xFF00)) {
            xds->checksum = (cc&0x007F);
            xds->state = 0;
            return LIBCAPTION_READY;
        }

        if (xds->size < 32) {
            xds->content[xds->size+0] = (cc&0x7F00) >>8;
            xds->content[xds->size+1] = (cc&0x007F);
            xds->size += 2;
            return LIBCAPTION_OK;
        }
    }

    xds->state = 0;
    return LIBCAPTION_ERROR;
}
