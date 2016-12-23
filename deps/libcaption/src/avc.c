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

#include "avc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
////////////////////////////////////////////////////////////////////////////////
// AVC RBSP Methods
//  TODO move the to a avcutils file
static size_t _find_emulation_prevention_byte (const uint8_t* data, size_t size)
{
    size_t offset = 2;

    while (offset < size) {
        if (0 == data[offset]) {
            // 0 0 X 3 //; we know X is zero
            offset += 1;
        } else if (3 != data[offset]) {
            // 0 0 X 0 0 3; we know X is not 0 and not 3
            offset += 3;
        } else if (0 != data[offset-1]) {
            // 0 X 0 0 3
            offset += 2;
        } else if (0 != data[offset-2]) {
            // X 0 0 3
            offset += 1;
        } else {
            // 0 0 3
            return offset;
        }
    }

    return size;
}

static size_t _copy_to_rbsp (uint8_t* destData, size_t destSize, const uint8_t* sorcData, size_t sorcSize)
{
    size_t toCopy, totlSize = 0;

    for (;;) {
        if (destSize >= sorcSize) {
            return 0;
        }

        // The following line IS correct! We want to look in sorcData up to destSize bytes
        // We know destSize is smaller than sorcSize because of the previous line
        toCopy = _find_emulation_prevention_byte (sorcData,destSize);
        memcpy (destData, sorcData, toCopy);
        totlSize += toCopy;
        destData += toCopy;
        destSize -= toCopy;

        if (0 == destSize) {
            return totlSize;
        }

        // skip the emulation prevention byte
        totlSize += 1;
        sorcData += toCopy + 1;
        sorcSize -= toCopy + 1;
    }

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
static inline size_t _find_emulated (uint8_t* data, size_t size)
{
    size_t offset = 2;

    while (offset < size) {
        if (3 < data[offset]) {
            // 0 0 X; we know X is not 0, 1, 2 or 3
            offset += 3;
        } else if (0 != data[offset-1]) {
            // 0 X 0 0 1
            offset += 2;
        } else if (0 != data[offset-2]) {
            // X 0 0 1
            offset += 1;
        } else {
            // 0 0 0, 0 0 1
            return offset;
        }
    }

    return size;
}

size_t _copy_from_rbsp (uint8_t* data, uint8_t* payloadData, size_t payloadSize)
{
    size_t total = 0;

    while (payloadSize) {
        size_t bytes = _find_emulated (payloadData,payloadSize);

        if (bytes > payloadSize) {
            return 0;
        }

        memcpy (data, payloadData, bytes);

        if (bytes == payloadSize) {
            return total + bytes;
        }

        data[bytes] = 3; // insert emulation prevention byte
        data += bytes + 1; total += bytes + 1;
        payloadData += bytes; payloadSize -= bytes;
    }

    return total;
}
////////////////////////////////////////////////////////////////////////////////
struct _sei_message_t {
    size_t size;
    sei_msgtype_t type;
    struct _sei_message_t* next;
};

sei_message_t* sei_message_next (sei_message_t* msg) { return ( (struct _sei_message_t*) msg)->next; }
sei_msgtype_t  sei_message_type (sei_message_t* msg) { return ( (struct _sei_message_t*) msg)->type; }
size_t         sei_message_size (sei_message_t* msg) { return ( (struct _sei_message_t*) msg)->size; }
uint8_t*       sei_message_data (sei_message_t* msg) { return ( (uint8_t*) msg) + sizeof (struct _sei_message_t); }
void           sei_message_free (sei_message_t* msg) { if (msg) { free (msg); } }

sei_message_t* sei_message_new (sei_msgtype_t type, uint8_t* data, size_t size)
{
    struct _sei_message_t* msg = (struct _sei_message_t*) malloc (sizeof (struct _sei_message_t) + size);
    msg->next = 0; msg->type = type; msg->size = size;

    if (data) {
        memcpy (sei_message_data (msg), data, size);
    } else {
        memset (sei_message_data (msg), 0, size);
    }

    return (sei_message_t*) msg;
}
////////////////////////////////////////////////////////////////////////////////
void sei_init (sei_t* sei)
{
    sei->dts = -1;
    sei->cts = -1;
    sei->head = 0;
    sei->tail = 0;
}

void sei_message_append (sei_t* sei, sei_message_t* msg)
{
    if (0 == sei->head) {
        sei->head = msg;
        sei->tail = msg;
    } else {
        sei->tail->next = msg;
        sei->tail = msg;
    }
}

void sei_free (sei_t* sei)
{
    sei_message_t* tail;

    while (sei->head) {
        tail = sei->head->next;
        free (sei->head);
        sei->head = tail;
    }

    sei_init (sei);
}

void sei_dump (sei_t* sei)
{
    fprintf (stderr,"SEI %p\n", sei);
    sei_dump_messages (sei->head);
}

void sei_dump_messages (sei_message_t* head)
{
    cea708_t cea708;
    sei_message_t* msg;
    cea708_init (&cea708);

    for (msg = head ; msg ; msg = sei_message_next (msg)) {
        uint8_t* data = sei_message_data (msg);
        size_t size =  sei_message_size (msg);
        fprintf (stderr,"-- Message %p\n-- Message Type: %d\n-- Message Size: %d\n", data, sei_message_type (msg), (int) size);

        while (size) {
            fprintf (stderr,"%02X ", *data);
            ++data; --size;
        }

        fprintf (stderr,"\n");

        if (sei_type_user_data_registered_itu_t_t35 == sei_message_type (msg)) {
            cea708_parse (sei_message_data (msg), sei_message_size (msg), &cea708);
            cea708_dump (&cea708);
        }


    }
}

////////////////////////////////////////////////////////////////////////////////
size_t sei_render_size (sei_t* sei)
{
    size_t size = 2; // nalu_type + stop bit
    sei_message_t* msg;

    for (msg = sei_message_head (sei) ; msg ; msg = sei_message_next (msg)) {
        size += 1 + (msg->type / 255);
        size += 1 + (msg->size / 255);
        size += 1 + (msg->size * 4/3);
    }

    return size;
}

// we can safely assume sei_render_size() bytes have been allocated for data
size_t sei_render (sei_t* sei, uint8_t* data)
{
    size_t escaped_size, size = 2; // nalu_type + stop bit
    sei_message_t* msg;
    (*data) = 6; ++data;

    for (msg = sei_message_head (sei) ; msg ; msg = sei_message_next (msg)) {
        int payloadType      = sei_message_type (msg);
        int payloadSize      = (int) sei_message_size (msg);
        uint8_t* payloadData = sei_message_data (msg);

        while (255 <= payloadType) {
            (*data) = 255;
            ++data; ++size;
            payloadType -= 255;
        }

        (*data) = payloadType;
        ++data; ++size;

        while (255 <= payloadSize) {
            (*data) = 255;
            ++data; ++size;
            payloadSize -= 255;
        }

        (*data) = payloadSize;
        ++data; ++size;

        if (0 >= (escaped_size = _copy_from_rbsp (data,payloadData,payloadSize))) {
            return 0;
        }

        data += escaped_size;
        size += escaped_size;
    }

    // write stop bit and return
    (*data) = 0x80;
    return size;
}

uint8_t* sei_render_alloc (sei_t* sei, size_t* size)
{
    size_t aloc = sei_render_size (sei);
    uint8_t* data = malloc (aloc);
    (*size) = sei_render (sei, data);
    return data;
}

////////////////////////////////////////////////////////////////////////////////
int sei_parse_nalu (sei_t* sei, const uint8_t* data, size_t size, double dts, double cts)
{
    assert (0<=cts); // cant present before decode
    sei->dts = dts;
    sei->cts = cts;
    int ret = 0;

    if (0 == data || 0 == size) {
        return 0;
    }

    uint8_t nal_unit_type = (*data) & 0x1F;
    ++data; --size;

    if (6 != nal_unit_type) {
        return 0;
    }

    // SEI may contain more than one payload
    while (1<size) {
        int payloadType = 0;
        int payloadSize = 0;

        while (0 < size && 255 == (*data)) {
            payloadType += 255;
            ++data; --size;
        }

        if (0 == size) {
            goto error;
        }

        payloadType += (*data);
        ++data; --size;

        while (0 < size && 255 == (*data)) {
            payloadSize += 255;
            ++data; --size;
        }

        if (0 == size) {
            goto error;
        }

        payloadSize += (*data);
        ++data; --size;

        if (payloadSize) {
            sei_message_t* msg = sei_message_new ( (sei_msgtype_t) payloadType, 0, payloadSize);
            uint8_t* payloadData = sei_message_data (msg);
            size_t bytes = _copy_to_rbsp (payloadData, payloadSize, data, size);
            sei_message_append (sei, msg);

            if ( (int) bytes < payloadSize) {
                goto error;
            }

            data += bytes; size -= bytes;
            ++ret;
        }
    }

    // There should be one trailing byte, 0x80. But really, we can just ignore that fact.
    return ret;
error:
    sei_init (sei);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
libcaption_stauts_t sei_to_caption_frame (sei_t* sei, caption_frame_t* frame)
{
    cea708_t cea708;
    sei_message_t* msg;
    libcaption_stauts_t status = LIBCAPTION_OK;

    cea708_init (&cea708);

    for (msg = sei_message_head (sei) ; msg ; msg = sei_message_next (msg)) {
        if (sei_type_user_data_registered_itu_t_t35 == sei_message_type (msg)) {
            cea708_parse (sei_message_data (msg), sei_message_size (msg), &cea708);
            status = libcaption_status_update (status, cea708_to_caption_frame (frame, &cea708, sei_pts (sei)));
        }
    }

    if (LIBCAPTION_READY == status) {
        frame->timestamp = sei->dts + sei->cts;
        frame->duration = 0;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
#define DEFAULT_CHANNEL 0

void sei_append_708 (sei_t* sei, cea708_t* cea708)
{
    sei_message_t* msg = sei_message_new (sei_type_user_data_registered_itu_t_t35, 0, CEA608_MAX_SIZE);
    msg->size = cea708_render (cea708, sei_message_data (msg), sei_message_size (msg));
    sei_message_append (sei,msg);
    // cea708_dump (cea708);
    cea708_init (cea708); // will confgure using HLS compatiable defaults
}

// This should be moved to 708.c
// This works for popon, but bad for paint on and roll up
// Please understand this function before you try to use it, setting null values have different effects than you may assume
void sei_encode_eia608 (sei_t* sei, cea708_t* cea708, uint16_t cc_data)
{
    // This one is full, flush and init a new one
    // shoudl this be 32? I cant remember
    if (31 == cea708->user_data.cc_count) {
        sei_append_708 (sei,cea708);
    }

    if (0 == cea708->user_data.cc_count) { // This is a new 708 header, but a continuation of a 608 stream
        cea708_add_cc_data (cea708, 1, cc_type_ntsc_cc_field_1, eia608_control_command (eia608_control_resume_caption_loading, DEFAULT_CHANNEL));
    }

    if (0 == cc_data) { // Finished
        sei_encode_eia608 (sei,cea708,eia608_control_command (eia608_control_end_of_caption, DEFAULT_CHANNEL));
        sei_append_708 (sei,cea708);
        return;
    }

    cea708_add_cc_data (cea708, 1, cc_type_ntsc_cc_field_1, cc_data);
}
////////////////////////////////////////////////////////////////////////////////
// TODO use alternate charcters instead of always using space before extended charcters
// TODO rewrite this function with better logic
int sei_from_caption_frame (sei_t* sei, caption_frame_t* frame)
{
    int r,c;
    cea708_t cea708;
    const char* data;
    uint16_t prev_cc_data;

    cea708_init (&cea708); // set up a new popon frame
    cea708_add_cc_data (&cea708, 1, cc_type_ntsc_cc_field_1, eia608_control_command (eia608_control_erase_non_displayed_memory, DEFAULT_CHANNEL));
    cea708_add_cc_data (&cea708, 1, cc_type_ntsc_cc_field_1, eia608_control_command (eia608_control_resume_caption_loading, DEFAULT_CHANNEL));

    for (r=0; r<SCREEN_ROWS; ++r) {
        // Calculate preamble
        for (c=0; c<SCREEN_COLS && 0 == *caption_frame_read_char (frame,r,c,0,0) ; ++c) {}

        // This row is blank
        if (SCREEN_COLS == c) {
            continue;
        }

        // Write preamble
        sei_encode_eia608 (sei, &cea708, eia608_row_column_pramble (r,c,DEFAULT_CHANNEL,0));
        int tab = c % 4;

        if (tab) {
            sei_encode_eia608 (sei, &cea708, eia608_tab (tab,DEFAULT_CHANNEL));
        }

        // Write the row
        for (prev_cc_data = 0, data = caption_frame_read_char (frame,r,c,0,0) ;
                (*data) && c < SCREEN_COLS ; ++c, data = caption_frame_read_char (frame,r,c,0,0)) {
            uint16_t cc_data = eia608_from_utf8_1 (data,DEFAULT_CHANNEL);

            if (!cc_data) {
                // We do't want to write bad data, so just ignore it.
            } else if (eia608_is_basicna (prev_cc_data)) {
                if (eia608_is_basicna (cc_data)) {
                    // previous and current chars are both basicna, combine them into current
                    sei_encode_eia608 (sei, &cea708, eia608_from_basicna (prev_cc_data,cc_data));
                } else if (eia608_is_westeu (cc_data)) {
                    // extended charcters overwrite the previous charcter, so insert a dummy char thren write the extended char
                    sei_encode_eia608 (sei, &cea708, eia608_from_basicna (prev_cc_data,eia608_from_utf8_1 (EIA608_CHAR_SPACE,DEFAULT_CHANNEL)));
                    sei_encode_eia608 (sei, &cea708, cc_data);
                } else {
                    // previous was basic na, but current isnt; write previous and current
                    sei_encode_eia608 (sei, &cea708, prev_cc_data);
                    sei_encode_eia608 (sei, &cea708, cc_data);
                }

                prev_cc_data = 0; // previous is handled, we can forget it now
            } else if (eia608_is_westeu (cc_data)) {
                // extended chars overwrite the previous chars, so insert a dummy char
                sei_encode_eia608 (sei, &cea708, eia608_from_utf8_1 (EIA608_CHAR_SPACE,DEFAULT_CHANNEL));
                sei_encode_eia608 (sei, &cea708, cc_data);
            } else if (eia608_is_basicna (cc_data)) {
                prev_cc_data = cc_data;
            } else {
                sei_encode_eia608 (sei, &cea708, cc_data);
            }

            if (eia608_is_specialna (cc_data)) {
                // specialna are treated as controll charcters. Duplicated controll charcters are discarded
                // So we for a resume after a specialna as a noop to break repetition detection
                // TODO only do this if the same charcter is repeated
                sei_encode_eia608 (sei, &cea708, eia608_control_command (eia608_control_resume_caption_loading, DEFAULT_CHANNEL));
            }
        }

        if (0 != prev_cc_data) {
            sei_encode_eia608 (sei, &cea708, prev_cc_data);
        }
    }

    sei_encode_eia608 (sei, &cea708, 0); // flush
    sei->dts = frame->timestamp; // assumes in order frames
    // sei_dump (sei);
    return 1;
}
////////////////////////////////////////////////////////////////////////////////
static int avc_is_start_code (const uint8_t* data, int size, int* len)
{
    if (3 > size) {
        return -1;
    }

    if (1 < data[2]) {
        return 3;
    }

    if (0 != data[1]) {
        return 2;
    }

    if (0 == data[0]) {
        if (1 == data[2]) {
            *len = 3;
            return 0;
        }

        if (4 <= size && 1 == data[3]) {
            *len = 4;
            return 0;
        }
    }

    return 1;
}


static int avc_find_start_code (const uint8_t* data, int size, int* len)
{
    int pos = 0;

    for (;;) {
        // is pos pointing to a start code?
        int isc = avc_is_start_code (data + pos, size - pos, len);

        if (0 < isc) {
            pos += isc;
        } else if (0 > isc) {
            // No start code found
            return isc;
        } else {
            // Start code found at pos
            return pos;
        }
    }
}


static int avc_find_start_code_increnental (const uint8_t* data, int size, int prev_size, int* len)
{
    int offset = (3 <= prev_size) ? (prev_size - 3) : 0;
    int pos = avc_find_start_code (data + offset, size - offset, len);

    if (0 <= pos) {
        return pos + offset;
    }

    return pos;
}

void avcnalu_init (avcnalu_t* nalu)
{
    memset (nalu,0,sizeof (avcnalu_t));
}

int avcnalu_parse_annexb (avcnalu_t* nalu, const uint8_t** data, size_t* size)
{
    int scpos, sclen;
    int new_size = (int) (nalu->size + (*size));

    if (new_size > MAX_NALU_SIZE) {
        (*size) = nalu->size = 0;
        return LIBCAPTION_ERROR;
    }

    memcpy (&nalu->data[nalu->size], (*data), (*size));
    scpos = avc_find_start_code_increnental (&nalu->data[0], new_size, (int) nalu->size, &sclen);

    if (0<=scpos) {
        (*data) += (scpos - nalu->size) + sclen;
        (*size) -= (scpos - nalu->size) + sclen;
        nalu->size = scpos;
        return 0 < nalu->size ? LIBCAPTION_READY : LIBCAPTION_OK;
    } else {
        (*size) = 0;
        nalu->size = new_size;
        return LIBCAPTION_OK;
    }
}
