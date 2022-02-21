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
#include "eia608.h"
#include <stdio.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
int eia608_row_map[] = { 10, -1, 0, 1, 2, 3, 11, 12, 13, 14, 4, 5, 6, 7, 8, 9 };
int eia608_reverse_row_map[] = { 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 0, 6, 7, 8, 9, 1 };

const char* eia608_style_map[] = {
    "white",
    "green",
    "blue",
    "cyan",
    "red",
    "yellow",
    "magenta",
    "italics",
};

static inline uint16_t eia608_row_pramble(int row, int chan, int x, int underline)
{
    row = eia608_reverse_row_map[row & 0x0F];
    return eia608_parity(0x1040 | (chan ? 0x0800 : 0x0000) | ((row << 7) & 0x0700) | ((row << 5) & 0x0020)) | ((x << 1) & 0x001E) | (underline ? 0x0001 : 0x0000);
}

uint16_t eia608_row_column_pramble(int row, int col, int chan, int underline) { return eia608_row_pramble(row, chan, 0x10 | (col / 4), underline); }
uint16_t eia608_row_style_pramble(int row, int chan, eia608_style_t style, int underline) { return eia608_row_pramble(row, chan, style, underline); }
uint16_t eia608_midrow_change(int chan, eia608_style_t style, int underline) { return eia608_parity(0x1120 | ((chan << 11) & 0x0800) | ((style << 1) & 0x000E) | (underline & 0x0001)); }

int eia608_parse_preamble(uint16_t cc_data, int* row, int* col, eia608_style_t* style, int* chan, int* underline)
{
    (*row) = eia608_row_map[((0x0700 & cc_data) >> 7) | ((0x0020 & cc_data) >> 5)];
    (*chan) = !!(0x0800 & cc_data);
    (*underline) = 0x0001 & cc_data;

    if (0x0010 & cc_data) {
        (*style) = eia608_style_white;
        (*col) = 4 * ((0x000E & cc_data) >> 1);
    } else {
        (*style) = (0x000E & cc_data) >> 1;
        (*col) = 0;
    }

    return 1;
}

int eia608_parse_midrowchange(uint16_t cc_data, int* chan, eia608_style_t* style, int* underline)
{
    (*chan) = !!(0x0800 & cc_data);

    if (0x1120 == (0x7770 & cc_data)) {
        (*style) = (0x000E & cc_data) >> 1;
        (*underline) = 0x0001 & cc_data;
    }

    return 1;
}
////////////////////////////////////////////////////////////////////////////////
// control command
eia608_control_t eia608_parse_control(uint16_t cc_data, int* cc)
{
    if (0x0200 & cc_data) {
        (*cc) = (cc_data & 0x0800 ? 0x01 : 0x00);
        return (eia608_control_t)(0x177F & cc_data);
    } else {
        (*cc) = (cc_data & 0x0800 ? 0x01 : 0x00) | (cc_data & 0x0100 ? 0x02 : 0x00);
        return (eia608_control_t)(0x167F & cc_data);
    }
}

uint16_t eia608_control_command(eia608_control_t cmd, int cc)
{
    uint16_t c = (cc & 0x01) ? 0x0800 : 0x0000;
    uint16_t f = (cc & 0x02) ? 0x0100 : 0x0000;

    if (eia608_tab_offset_0 == (eia608_control_t)(cmd & 0xFFC0)) {
        return (eia608_control_t)eia608_parity(cmd | c);
    } else {
        return (eia608_control_t)eia608_parity(cmd | c | f);
    }
}
////////////////////////////////////////////////////////////////////////////////
// text
static const char* utf8_from_index(int idx) { return (0 <= idx && EIA608_CHAR_COUNT > idx) ? eia608_char_map[idx] : ""; }
static int eia608_to_index(uint16_t cc_data, int* chan, int* c1, int* c2)
{
    (*c1) = (*c2) = -1;
    (*chan) = 0;
    cc_data &= 0x7F7F; // strip off parity bits

    // Handle Basic NA BEFORE we strip the channel bit
    if (eia608_is_basicna(cc_data)) {
        // we got first char, yes. But what about second char?
        (*c1) = (cc_data >> 8) - 0x20;
        cc_data &= 0x00FF;

        if (0x0020 <= cc_data && 0x0080 > cc_data) {
            (*c2) = cc_data - 0x20;
            return 2;
        }

        return 1;
    }

    // Check then strip second channel toggle
    (*chan) = cc_data & 0x0800;
    cc_data = cc_data & 0xF7FF;

    if (eia608_is_specialna(cc_data)) {
        // Special North American character
        (*c1) = cc_data - 0x1130 + 0x60;
        return 1;
    }

    if (0x1220 <= cc_data && 0x1240 > cc_data) {
        // Extended Western European character set, Spanish/Miscellaneous/French
        (*c1) = cc_data - 0x1220 + 0x70;
        return 1;
    }

    if (0x1320 <= cc_data && 0x1340 > cc_data) {
        // Extended Western European character set, Portuguese/German/Danish
        (*c1) = cc_data - 0x1320 + 0x90;
        return 1;
    }

    return 0;
}

int eia608_to_utf8(uint16_t c, int* chan, char* str1, char* str2)
{
    int c1, c2;
    int size = (int)eia608_to_index(c, chan, &c1, &c2);
    utf8_char_copy(str1, utf8_from_index(c1));
    utf8_char_copy(str2, utf8_from_index(c2));
    return size;
}

uint16_t eia608_from_basicna(uint16_t bna1, uint16_t bna2)
{
    if (!eia608_is_basicna(bna1) || !eia608_is_basicna(bna2)) {
        return 0;
    }

    return eia608_parity((0xFF00 & bna1) | ((0xFF00 & bna2) >> 8));
}

// prototype for re2c generated function
uint16_t _eia608_from_utf8(const utf8_char_t* s);
uint16_t eia608_from_utf8_1(const utf8_char_t* c, int chan)
{
    uint16_t cc_data = _eia608_from_utf8(c);

    if (0 == cc_data) {
        return cc_data;
    }

    if (chan && !eia608_is_basicna(cc_data)) {
        cc_data |= 0x0800;
    }

    return eia608_parity(cc_data);
}

uint16_t eia608_from_utf8_2(const utf8_char_t* c1, const utf8_char_t* c2)
{
    uint16_t cc1 = _eia608_from_utf8(c1);
    uint16_t cc2 = _eia608_from_utf8(c2);
    return eia608_from_basicna(cc1, cc2);
}
////////////////////////////////////////////////////////////////////////////////
void eia608_dump(uint16_t cc_data)
{
    eia608_style_t style;
    const char* text = 0;
    char char1[5], char2[5];
    char1[0] = char2[0] = 0;
    int row, col, chan, underline;

    if (!eia608_parity_varify(cc_data)) {
        text = "parity failed";
    } else if (0 == eia608_parity_strip(cc_data)) {
        text = "pad";
    } else if (eia608_is_basicna(cc_data)) {
        text = "basicna";
        eia608_to_utf8(cc_data, &chan, &char1[0], &char2[0]);
    } else if (eia608_is_specialna(cc_data)) {
        text = "specialna";
        eia608_to_utf8(cc_data, &chan, &char1[0], &char2[0]);
    } else if (eia608_is_westeu(cc_data)) {
        text = "westeu";
        eia608_to_utf8(cc_data, &chan, &char1[0], &char2[0]);
    } else if (eia608_is_xds(cc_data)) {
        text = "xds";
    } else if (eia608_is_midrowchange(cc_data)) {
        text = "midrowchange";
    } else if (eia608_is_norpak(cc_data)) {
        text = "norpak";
    } else if (eia608_is_preamble(cc_data)) {
        text = "preamble";
        eia608_parse_preamble(cc_data, &row, &col, &style, &chan, &underline);
        fprintf(stderr, "preamble %d %d %d %d %d\n", row, col, style, chan, underline);

    } else if (eia608_is_control(cc_data)) {
        switch (eia608_parse_control(cc_data, &chan)) {

        default:
            text = "unknown_control";
            break;

        case eia608_tab_offset_0:
            text = "eia608_tab_offset_0";
            break;

        case eia608_tab_offset_1:
            text = "eia608_tab_offset_1";
            break;

        case eia608_tab_offset_2:
            text = "eia608_tab_offset_2";
            break;

        case eia608_tab_offset_3:
            text = "eia608_tab_offset_3";
            break;

        case eia608_control_resume_caption_loading:
            text = "eia608_control_resume_caption_loading";
            break;

        case eia608_control_backspace:
            text = "eia608_control_backspace";
            break;

        case eia608_control_alarm_off:
            text = "eia608_control_alarm_off";
            break;

        case eia608_control_alarm_on:
            text = "eia608_control_alarm_on";
            break;

        case eia608_control_delete_to_end_of_row:
            text = "eia608_control_delete_to_end_of_row";
            break;

        case eia608_control_roll_up_2:
            text = "eia608_control_roll_up_2";
            break;

        case eia608_control_roll_up_3:
            text = "eia608_control_roll_up_3";
            break;

        case eia608_control_roll_up_4:
            text = "eia608_control_roll_up_4";
            break;

        case eia608_control_resume_direct_captioning:
            text = "eia608_control_resume_direct_captioning";
            break;

        case eia608_control_text_restart:
            text = "eia608_control_text_restart";
            break;

        case eia608_control_text_resume_text_display:
            text = "eia608_control_text_resume_text_display";
            break;

        case eia608_control_erase_display_memory:
            text = "eia608_control_erase_display_memory";
            break;

        case eia608_control_carriage_return:
            text = "eia608_control_carriage_return";
            break;

        case eia608_control_erase_non_displayed_memory:
            text = "eia608_control_erase_non_displayed_memory";
            break;

        case eia608_control_end_of_caption:
            text = "eia608_control_end_of_caption";
            break;
        }
    } else {
        text = "unhandled";
    }

    fprintf(stderr, "cc %04X (%04X) '%s' '%s' (%s)\n", cc_data, eia608_parity_strip(cc_data), char1, char2, text);
}
