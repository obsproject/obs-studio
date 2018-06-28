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
#include "caption.h"
#include "eia608.h"
#include "utf8.h"
#include "xds.h"
#include <stdio.h>
#include <string.h>
////////////////////////////////////////////////////////////////////////////////
void caption_frame_buffer_clear(caption_frame_buffer_t* buff)
{
    memset(buff, 0, sizeof(caption_frame_buffer_t));
}

void caption_frame_state_clear(caption_frame_t* frame)
{
    frame->write = 0;
    frame->timestamp = -1;
    frame->state = (caption_frame_state_t){ 0, 0, 0, SCREEN_ROWS - 1, 0, 0 }; // clear global state
}

void caption_frame_init(caption_frame_t* frame)
{
    xds_init(&frame->xds);
    caption_frame_state_clear(frame);
    caption_frame_buffer_clear(&frame->back);
    caption_frame_buffer_clear(&frame->front);
}
////////////////////////////////////////////////////////////////////////////////
// Helpers
static caption_frame_cell_t* frame_buffer_cell(caption_frame_buffer_t* buff, int row, int col)
{
    if (!buff || 0 > row || SCREEN_ROWS <= row || 0 > col || SCREEN_COLS <= col) {
        return 0;
    }

    return &buff->cell[row][col];
}

uint16_t _eia608_from_utf8(const char* s); // function is in eia608.c.re2c
int caption_frame_write_char(caption_frame_t* frame, int row, int col, eia608_style_t style, int underline, const char* c)
{
    if (!frame->write || !_eia608_from_utf8(c)) {
        return 0;
    }

    caption_frame_cell_t* cell = frame_buffer_cell(frame->write, row, col);

    if (cell && utf8_char_copy(&cell->data[0], c)) {
        cell->uln = underline;
        cell->sty = style;
        return 1;
    }

    return 0;
}

const utf8_char_t* caption_frame_read_char(caption_frame_t* frame, int row, int col, eia608_style_t* style, int* underline)
{
    // always read from front
    caption_frame_cell_t* cell = frame_buffer_cell(&frame->front, row, col);

    if (!cell) {
        if (style) {
            (*style) = eia608_style_white;
        }

        if (underline) {
            (*underline) = 0;
        }

        return EIA608_CHAR_NULL;
    }

    if (style) {
        (*style) = cell->sty;
    }

    if (underline) {
        (*underline) = cell->uln;
    }

    return &cell->data[0];
}

////////////////////////////////////////////////////////////////////////////////
// Parsing
libcaption_stauts_t caption_frame_carriage_return(caption_frame_t* frame)
{
    if (0 > frame->state.row || SCREEN_ROWS <= frame->state.row) {
        return LIBCAPTION_ERROR;
    }

    int r = frame->state.row - (frame->state.rup - 1);

    if (0 >= r || !caption_frame_rollup(frame)) {
        return LIBCAPTION_OK;
    }

    for (; r < SCREEN_ROWS; ++r) {
        uint8_t* dst = (uint8_t*)frame_buffer_cell(frame->write, r - 1, 0);
        uint8_t* src = (uint8_t*)frame_buffer_cell(frame->write, r - 0, 0);
        memcpy(dst, src, sizeof(caption_frame_cell_t) * SCREEN_COLS);
    }

    frame->state.col = 0;
    caption_frame_cell_t* cell = frame_buffer_cell(frame->write, SCREEN_ROWS - 1, 0);
    memset(cell, 0, sizeof(caption_frame_cell_t) * SCREEN_COLS);
    return LIBCAPTION_OK;
}
////////////////////////////////////////////////////////////////////////////////
libcaption_stauts_t eia608_write_char(caption_frame_t* frame, char* c)
{
    if (0 == c || 0 == c[0] || SCREEN_ROWS <= frame->state.row || 0 > frame->state.row || SCREEN_COLS <= frame->state.col || 0 > frame->state.col) {
        // NO-OP
    } else if (caption_frame_write_char(frame, frame->state.row, frame->state.col, frame->state.sty, frame->state.uln, c)) {
        frame->state.col += 1;
    }

    return LIBCAPTION_OK;
}

libcaption_stauts_t caption_frame_end(caption_frame_t* frame)
{
    memcpy(&frame->front, &frame->back, sizeof(caption_frame_buffer_t));
    caption_frame_buffer_clear(&frame->back); // This is required
    return LIBCAPTION_READY;
}

libcaption_stauts_t caption_frame_decode_preamble(caption_frame_t* frame, uint16_t cc_data)
{
    eia608_style_t sty;
    int row, col, chn, uln;

    if (eia608_parse_preamble(cc_data, &row, &col, &sty, &chn, &uln)) {
        frame->state.row = row;
        frame->state.col = col;
        frame->state.sty = sty;
        frame->state.uln = uln;
    }

    return LIBCAPTION_OK;
}

libcaption_stauts_t caption_frame_decode_midrowchange(caption_frame_t* frame, uint16_t cc_data)
{
    eia608_style_t sty;
    int chn, unl;

    if (eia608_parse_midrowchange(cc_data, &chn, &sty, &unl)) {
        frame->state.sty = sty;
        frame->state.uln = unl;
    }

    return LIBCAPTION_OK;
}

libcaption_stauts_t caption_frame_backspace(caption_frame_t* frame)
{
    // do not reverse wrap (tw 28:20)
    frame->state.col = (0 < frame->state.col) ? (frame->state.col - 1) : 0;
    caption_frame_write_char(frame, frame->state.row, frame->state.col, eia608_style_white, 0, EIA608_CHAR_NULL);
    return LIBCAPTION_READY;
}

libcaption_stauts_t caption_frame_delete_to_end_of_row(caption_frame_t* frame)
{
    int c;
    if (frame->write) {
        for (c = frame->state.col; c < SCREEN_COLS; ++c) {
            caption_frame_write_char(frame, frame->state.row, c, eia608_style_white, 0, EIA608_CHAR_NULL);
        }
    }

    // TODO test this and replace loop
    //  uint8_t* dst = (uint8_t*)frame_buffer_cell(frame->write, frame->state.row, frame->state.col);
    //  memset(dst,0,sizeof(caption_frame_cell_t) * (SCREEN_COLS - frame->state.col - 1))

    return LIBCAPTION_READY;
}

libcaption_stauts_t caption_frame_decode_control(caption_frame_t* frame, uint16_t cc_data)
{
    int cc;
    eia608_control_t cmd = eia608_parse_control(cc_data, &cc);

    switch (cmd) {
    // PAINT ON
    case eia608_control_resume_direct_captioning:
        frame->state.rup = 0;
        frame->write = &frame->front;
        return LIBCAPTION_OK;

    case eia608_control_erase_display_memory:
        caption_frame_buffer_clear(&frame->front);
        return LIBCAPTION_READY;

    // ROLL-UP
    case eia608_control_roll_up_2:
        frame->state.rup = 1;
        frame->write = &frame->front;
        return LIBCAPTION_OK;

    case eia608_control_roll_up_3:
        frame->state.rup = 2;
        frame->write = &frame->front;
        return LIBCAPTION_OK;

    case eia608_control_roll_up_4:
        frame->state.rup = 3;
        frame->write = &frame->front;
        return LIBCAPTION_OK;

    case eia608_control_carriage_return:
        return caption_frame_carriage_return(frame);

    // Corrections (Is this only valid as part of paint on?)
    case eia608_control_backspace:
        return caption_frame_backspace(frame);
    case eia608_control_delete_to_end_of_row:
        return caption_frame_delete_to_end_of_row(frame);

    // POP ON
    case eia608_control_resume_caption_loading:
        frame->state.rup = 0;
        frame->write = &frame->back;
        return LIBCAPTION_OK;

    case eia608_control_erase_non_displayed_memory:
        caption_frame_buffer_clear(&frame->back);
        return LIBCAPTION_OK;

    case eia608_control_end_of_caption:
        return caption_frame_end(frame);

    // cursor positioning
    case eia608_tab_offset_0:
    case eia608_tab_offset_1:
    case eia608_tab_offset_2:
    case eia608_tab_offset_3:
        frame->state.col += (cmd - eia608_tab_offset_0);
        return LIBCAPTION_OK;

    // Unhandled
    default:
    case eia608_control_alarm_off:
    case eia608_control_alarm_on:
    case eia608_control_text_restart:
    case eia608_control_text_resume_text_display:
        return LIBCAPTION_OK;
    }
}

libcaption_stauts_t caption_frame_decode_text(caption_frame_t* frame, uint16_t cc_data)
{
    int chan;
    char char1[5], char2[5];
    size_t chars = eia608_to_utf8(cc_data, &chan, &char1[0], &char2[0]);

    if (eia608_is_westeu(cc_data)) {
        // Extended charcters replace the previous charcter for back compatibility
        caption_frame_backspace(frame);
    }

    if (0 < chars) {
        eia608_write_char(frame, char1);
    }

    if (1 < chars) {
        eia608_write_char(frame, char2);
    }

    return LIBCAPTION_OK;
}

libcaption_stauts_t caption_frame_decode(caption_frame_t* frame, uint16_t cc_data, double timestamp)
{
    if (!eia608_parity_varify(cc_data)) {
        frame->status = LIBCAPTION_ERROR;
        return frame->status;
    }

    if (eia608_is_padding(cc_data)) {
        frame->status = LIBCAPTION_OK;
        return frame->status;
    }

    if (0 > frame->timestamp || LIBCAPTION_READY == frame->status) {
        frame->timestamp = timestamp;
    }

    // skip duplicate controll commands. We also skip duplicate specialna to match the behaviour of iOS/vlc
    if ((eia608_is_specialna(cc_data) || eia608_is_control(cc_data)) && cc_data == frame->state.cc_data) {
        frame->status = LIBCAPTION_OK;
        return frame->status;
    }

    frame->state.cc_data = cc_data;

    if (frame->xds.state) {
        frame->status = xds_decode(&frame->xds, cc_data);
    } else if (eia608_is_xds(cc_data)) {
        frame->status = xds_decode(&frame->xds, cc_data);
    } else if (eia608_is_control(cc_data)) {
        frame->status = caption_frame_decode_control(frame, cc_data);
    } else if (eia608_is_basicna(cc_data) || eia608_is_specialna(cc_data) || eia608_is_westeu(cc_data)) {

        // Don't decode text if we dont know what mode we are in.
        if (!frame->write) {
            frame->status = LIBCAPTION_OK;
            return frame->status;
        }

        frame->status = caption_frame_decode_text(frame, cc_data);

        // If we are in paint on mode, display immiditally
        if (LIBCAPTION_OK == frame->status && caption_frame_painton(frame)) {
            frame->status = LIBCAPTION_READY;
        }
    } else if (eia608_is_preamble(cc_data)) {
        frame->status = caption_frame_decode_preamble(frame, cc_data);
    } else if (eia608_is_midrowchange(cc_data)) {
        frame->status = caption_frame_decode_midrowchange(frame, cc_data);
    }

    return frame->status;
}

////////////////////////////////////////////////////////////////////////////////
int caption_frame_from_text(caption_frame_t* frame, const utf8_char_t* data)
{
    ssize_t size = (ssize_t)strlen(data);
    caption_frame_init(frame);
    frame->write = &frame->back;

    for (size_t r = 0; (*data) && size && r < SCREEN_ROWS;) {
        // skip whitespace at start of line
        while (size && utf8_char_whitespace(data)) {
            size_t s = utf8_char_length(data);
            data += s, size -= (ssize_t)s;
        }

        // get charcter count for wrap (or orest of line)
        utf8_size_t char_count = utf8_wrap_length(data, SCREEN_COLS);
        // write to caption frame
        for (size_t c = 0; c < char_count; ++c) {
            size_t char_length = utf8_char_length(data);
            caption_frame_write_char(frame, (int)r, (int)c, eia608_style_white, 0, data);
            data += char_length, size -= (ssize_t)char_length;
        }

        r += char_count ? 1 : 0; // Update row num only if not blank
    }

    caption_frame_end(frame);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
size_t caption_frame_to_text(caption_frame_t* frame, utf8_char_t* data)
{
    int r, c, uln, crlf = 0, count = 0;
    size_t s, size = 0;
    eia608_style_t sty;
    (*data) = '\0';

    for (r = 0; r < SCREEN_ROWS; ++r) {
        crlf += count, count = 0;
        for (c = 0; c < SCREEN_COLS; ++c) {
            const utf8_char_t* chr = caption_frame_read_char(frame, r, c, &sty, &uln);
            // dont start a new line until we encounter at least one printable character
            if (0 < utf8_char_length(chr) && (0 < count || !utf8_char_whitespace(chr))) {
                if (0 < crlf) {
                    memcpy(data, "\r\n\0", 3);
                    data += 2, size += 2, crlf = 0;
                }

                s = utf8_char_copy(data, chr);
                data += s, size += s, ++count;
            }
        }
    }

    return size;
}
////////////////////////////////////////////////////////////////////////////////
size_t caption_frame_dump_buffer(caption_frame_t* frame, utf8_char_t* buf)
{
    int r, c;
    size_t bytes, total = 0;
    bytes = sprintf(buf, "   timestamp: %f\n   row: %02d    col: %02d    roll-up: %d\n",
        frame->timestamp, frame->state.row, frame->state.col, caption_frame_rollup(frame));
    total += bytes, buf += bytes;
    bytes = sprintf(buf, "   00000000001111111111222222222233\t   00000000001111111111222222222233\n"
                         "   01234567890123456789012345678901\t   01234567890123456789012345678901\n"
                         "  %s--------------------------------%s\t  %s--------------------------------%s\n",
        EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT, EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT,
        EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT, EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT);
    total += bytes;
    buf += bytes;

    for (r = 0; r < SCREEN_ROWS; ++r) {
        bytes = sprintf(buf, "%02d%s", r, EIA608_CHAR_VERTICAL_LINE);
        total += bytes, buf += bytes;

        // front buffer
        for (c = 0; c < SCREEN_COLS; ++c) {
            caption_frame_cell_t* cell = frame_buffer_cell(&frame->front, r, c);
            bytes = utf8_char_copy(buf, (!cell || 0 == cell->data[0]) ? EIA608_CHAR_SPACE : &cell->data[0]);
            total += bytes, buf += bytes;
        }

        bytes = sprintf(buf, "%s\t%02d%s", EIA608_CHAR_VERTICAL_LINE, r, EIA608_CHAR_VERTICAL_LINE);
        total += bytes, buf += bytes;

        // back buffer
        for (c = 0; c < SCREEN_COLS; ++c) {
            caption_frame_cell_t* cell = frame_buffer_cell(&frame->back, r, c);
            bytes = utf8_char_copy(buf, (!cell || 0 == cell->data[0]) ? EIA608_CHAR_SPACE : &cell->data[0]);
            total += bytes, buf += bytes;
        }

        bytes = sprintf(buf, "%s\n", EIA608_CHAR_VERTICAL_LINE);
        total += bytes, buf += bytes;
    }

    bytes = sprintf(buf, "  %s--------------------------------%s\t  %s--------------------------------%s\n",
        EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT, EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_LEFT,
        EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT, EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_LEFT);
    total += bytes, buf += bytes;
    return total;
}

void caption_frame_dump(caption_frame_t* frame)
{
    utf8_char_t buff[CAPTION_FRAME_DUMP_BUF_SIZE];
    caption_frame_dump_buffer(frame, buff);
    fprintf(stderr, "%s\n", buff);
}
