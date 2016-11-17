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

#include "eia608.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
// all possible utf8 valies, including invalid ones
void encode_utf8ish (int32_t in, char out[7])
{
    if (0 > in) {
        out[0] = 0;
        return;
    }

    // 0xxxxxxx, 7 bits
    if (0x80 > in) {
        out[0] = in;
        out[1] = 0;
        return;
    }

    // 110xxxxx   10xxxxxx, 11 bits
    if (0x800 > in) {
        out[0] = 0xC0 | ( (in >> (6*1)) & 0x1F);
        out[1] = 0x80 | ( (in >> (6*0)) & 0x3F);
        out[2] = 0;
        return;
    }

    // 1110xxxx   10xxxxxx    10xxxxxx, 16 bits
    if (0x10000 > in) {
        out[0] = 0xE0 | ( (in >> (6*2)) & 0x0F);
        out[1] = 0x80 | ( (in >> (6*1)) & 0x3F);
        out[2] = 0x80 | ( (in >> (6*0)) & 0x3F);
        out[3] = 0;
        return;
    }

    // 11110xxx   10xxxxxx    10xxxxxx    10xxxxxx, 21 bits
    if (0x200000 > in) {
        out[0] = 0xF0 | ( (in >> (6*3)) & 0x07);
        out[1] = 0x80 | ( (in >> (6*2)) & 0x3F);
        out[2] = 0x80 | ( (in >> (6*1)) & 0x3F);
        out[3] = 0x80 | ( (in >> (6*0)) & 0x3F);
        out[4] = 0;
        return;
    }

    // 111110xx   10xxxxxx    10xxxxxx    10xxxxxx    10xxxxxx, 26 bits
    if (0x4000000 > in) {
        out[0] = 0xF8 | ( (in >> (6*4)) & 0x03);
        out[1] = 0x80 | ( (in >> (6*3)) & 0x3F);
        out[2] = 0x80 | ( (in >> (6*2)) & 0x3F);
        out[3] = 0x80 | ( (in >> (6*1)) & 0x3F);
        out[4] = 0x80 | ( (in >> (6*0)) & 0x3F);
        out[5] = 0;
        return;
    }

    // 1111110x   10xxxxxx    10xxxxxx    10xxxxxx    10xxxxxx    10xxxxxx, 31 bits
    if (0x80000000 > in) {
        out[0] = 0xFC | ( (in >> (6*5)) & 0x01);
        out[1] = 0x80 | ( (in >> (6*4)) & 0x3F);
        out[2] = 0x80 | ( (in >> (6*3)) & 0x3F);
        out[3] = 0x80 | ( (in >> (6*2)) & 0x3F);
        out[4] = 0x80 | ( (in >> (6*1)) & 0x3F);
        out[5] = 0x80 | ( (in >> (6*0)) & 0x3F);
        out[6] = 0;
        return;
    }
}

void test_all_utf8()
{
    char s[7]; size_t size, count = 0; uint16_t code1, code2;

    for (int i = 0 ; i < 0x80000000 ; ++i) {
        encode_utf8ish (i, &s[0]);
        code1 = eia608_from_utf8 ( (const char*) &s[0], 0, &size);

        // code2 = eia608_from_utf8 ( (const char*) &s[0], 1, &size);
        if (code1) {
            ++count;
            printf ("%d: string: '%s' code: %04X\n",count, &s[0],code1);
        }
    }

    // Count must be 177
    // 176 charcters, pile we have two mapping for left quote mark
}

#define BIN "%d%d%d%d%d%d%d%d %d%d%d%d%d%d%d%d"
#define BIND(D) ((D)>>15)&0x01, ((D)>>14)&0x01,((D)>>13)&0x01,((D)>>12)&0x01,((D)>>11)&0x01,((D)>>10)&0x01,((D)>>9)&0x01,((D)>>8)&0x01,((D)>>7)&0x01,((D)>>6)&0x01,((D)>>5)&0x01,((D)>>4)&0x01,((D)>>3)&0x01,((D)>>2)&0x01,((D)>>1)&0x01,((D)>>0)&0x01


void print_bin (int n)
{
    int mask = 0x80;

    for (int mask = 0x80 ; mask ; mask >>= 1) {
        printf ("%s", n & mask ? "1" : "0");
    }

    printf ("\n");
}

void void_test_all_possible_code_words()
{
    for (int i = 0 ; i <= 0x3FFF ; ++i) {
        int16_t code = eia608_parity ( ( (i<<1) &0x7F00) | (i&0x7F));

        int count =eia608_cc_data_is_extended_data_service (code)+
                   eia608_cc_data_is_basic_north_american_character (code) +
                   eia608_cc_data_is_special_north_american_character (code) +
                   eia608_cc_data_is_extended_western_european_character (code) +
                   eia608_cc_data_is_nonwestern_norpak_character (code) +
                   eia608_cc_data_is_row_preamble (code) +
                   eia608_cc_data_is_control_command (code);

        if (1 < count) {
            printf ("code 0x%04X matched >1\n",code&0x7F7F);
        }

        // if (0 == count) {
        //     printf ("code 0x%04X not matched %d\n",eia608_strip_parity_bits (code), i);
        // }
    }
}

void print_charmap()
{
    for (int i = 0 ; i < EIA608_CHAR_COUNT ; ++i) {
        printf ("%s", eia608_char_map[i]);
    }

    printf ("\n");
}

void dance()
{
    for (int i = 0 ; i < 100 ; ++i) {
        const char* l = 0 == rand() % 2 ? EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT : EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT;
        const char* r = 0 == rand() % 2 ? EIA608_CHAR_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT : EIA608_CHAR_BOX_DRAWINGS_LIGHT_UP_AND_LEFT;
        printf ("%s %s%s%s%s%s%s%s %s ", EIA608_CHAR_EIGHTH_NOTE, l, EIA608_CHAR_LEFT_PARENTHESIS, EIA608_CHAR_EM_DASH, EIA608_CHAR_LOW_LINE, EIA608_CHAR_EM_DASH,
                EIA608_CHAR_RIGHT_PARENTHESIS, r, EIA608_CHAR_EIGHTH_NOTE);
    }

}


int main (int argc, const char** arg)
{
    // print_charmap();
    // // return 0;
    // srand (time (0));
    // // test_all_utf8();
    // // void_test_all_possible_code_words();
    // // return 0;
    // // print_charmap();
    // dance();
    // return 0;
    for (int i = 0 ; i <= 0x3FFF ; ++i) {
        uint16_t code1 = eia608_parity ( ( (i<<1) &0x7F00) | (i&0x7F));

        switch (eia608_cc_data_type (code1)) {
        default:
        case EIA608_CC_DATA_UNKNOWN:
            // printf ("Unknown code %04X\n",code);
            break;

        case EIA608_CC_DATA_CONTROL_COMMAND: {
            int cc;
            eia608_control_t cmd = eia608_parse_control (code1, &cc);
            uint16_t code2 = eia608_control_command (cmd,cc);

            if (code1 != code2) {
                printf (BIN " != " BIN " (0x%04x != 0x%04x) cc: %d\n", BIND (code1), BIND (code2),code1,code2,cc);
            }
        } break;


        case EIA608_CC_DATA_BASIC_NORTH_AMERICAN_CHARACTER: {
            char char1[5], char2[5]; int chan; size_t size;

            if (eia608_to_utf8 (code1, &chan, &char1[0], &char2[0])) {
                uint16_t code2 = eia608_from_utf8_2 (&char1[0], &char2[0]);

                // if the second char is invalid, mask it off, we will accept the first
                if (0x80 < (code1 &0x007F) || 0x20 > (code1 &0x007F)) {
                    code1 = (code1&0xFF00) |0x0080;
                }

                if (code1 == code2) {
                    // printf ("%s " BIN " == " BIN " (0x%04x == 0x%04x)\n", &char1[0], BIND (code1), BIND (code2),code1,code2);
                } else {
                    printf ("%s %s " BIN " != " BIN " (0x%04x != 0x%04x)\n", &char1[0], &char2[0], BIND (code1), BIND (code2),code1,code2);
                }
            }

        } break;

        case EIA608_CC_DATA_SPECIAL_NORTH_AMERICAN_CHARACTER:
        case EIA608_CC_DATA_EXTENDED_WESTERN_EUROPEAN_CHARACTER: {
            char char1[5], char2[5]; int chan; size_t size;

            if (eia608_to_utf8 (code1, &chan, &char1[0], &char2[0])) {
                uint16_t code2 = eia608_from_utf8 (&char1[0], chan, &size);

                if (code1 == code2) {
                    // printf ("%s " BIN " == " BIN " (0x%04x == 0x%04x)\n", &char1[0], BIND (code1), BIND (code2),code1,code2);
                } else {
                    printf ("%s " BIN " != " BIN " (0x%04x != 0x%04x)\n", &char1[0], BIND (code1), BIND (code2),code1,code2);
                }
            }
        } break;

            // #define EIA608_CODE_ROW_PREAMBLE                        4
            // #define EIA608_CODE_EXTENDED_DATA_SERVICE               5
            // #define EIA608_CODE_CONTROL_COMMAND                     6
        }
    }

    return 0;
}




//     for (uint16_t i  = 0 ; i < 0x4000; ++i) {
//         int chan;
//         char str[7];
//         uint16_t code = ( (i<<1) &0x7F00) | (i & 0x007F);
//
//         if (eia608_to_utf8 (code,&chan,str)) {
//             printf ("code: 0x%04X  str: '%s'\n", code,str);
//         }
//     }
//
//     // for(int i = 0 ; i < cie608_char_count ; ++i)
//     // {
//     //     cie608_char_map[i]
//     //
//     // }
//
//
//     for (int i = 0 ; i < 128 ; ++i) {
//         // print_bin( B7( i ) );
//         // print_bin( eia608_parity_table[i] );
//         printf ("%d  %d %d\n", i, 0x7F & eia608_parity_table[i], eia608_parity_table[i]);
//         // if ( i != eia608_parity_table[i] )
//         // {
//         //   printf( "ERROR\n" );
//         //
//         // }
//     }
//
// }
