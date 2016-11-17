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

#ifndef LIBCAPTION_XDS_H
#define LIBCAPTION_XDS_H

#include <stddef.h>
#include <inttypes.h>

typedef struct {
    int state;
    uint8_t class;
    uint8_t type;
    uint32_t size;
    uint8_t content[32];
    uint8_t checksum;
} xds_t;

void xds_init (xds_t* xds);
int xds_decode (xds_t* xds, uint16_t cc);

#endif
