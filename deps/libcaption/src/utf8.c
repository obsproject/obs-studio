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



#include "utf8.h"
#include <string.h>

const utf8_char_t* utf8_char_next (const char* s)
{
    if (0x80 == (s[0]&0xC0)) { ++s; }

    return s;
}

// returnes the length of the char in bytes
size_t utf8_char_length (const utf8_char_t* c)
{
    // count null term as zero size
    if (0x00 == c[0]) { return 0; }

    if (0x00 == (c[0]&0x80)) { return 1; }

    if (0xC0 == (c[0]&0xE0) && 0x80 == (c[1]&0xC0)) { return 2; }

    if (0xE0 == (c[0]&0xF0) && 0x80 == (c[1]&0xC0) && 0x80 == (c[2]&0xC0)) { return 3; }

    if (0xF0 == (c[0]&0xF8) && 0x80 == (c[1]&0xC0) && 0x80 == (c[2]&0xC0) && 0x80 == (c[3]&0xC0)) { return 4; }

    return 0;
}

// returns length of the string in bytes
// size is number of charcter to count (0 to count until NULL term)
size_t utf8_string_length (const utf8_char_t* data, utf8_size_t size)
{
    size_t char_length, byts = 0;

    if (0 == size) {
        size = utf8_char_count (data,0);
    }

    for (; 0 < size ; --size) {
        if (0 == (char_length = utf8_char_length (data))) {
            break;
        }

        data += char_length;
        byts += char_length;
    }

    return byts;
}

size_t utf8_char_copy (utf8_char_t* dst, const utf8_char_t* src)
{
    size_t bytes = utf8_char_length (src);

    if (bytes&&dst) {
        memcpy (dst,src,bytes);
        dst[bytes] = '\0';
    }

    return bytes;
}

// returnes the number of utf8 charcters in a string given the number of bytes
// to count until the a null terminator, pass 0 for size
utf8_size_t utf8_char_count (const char* data, size_t size)
{
    size_t i, bytes = 0;
    utf8_size_t count = 0;

    if (0 == size) {
        size = strlen (data);
    }

    for (i = 0 ; i < size ; ++count, i += bytes) {
        if (0 == (bytes = utf8_char_length (&data[i]))) {
            break;
        }
    }

    return count;
}

// returnes the length of the line in bytes triming not printable charcters at the end
size_t utf8_trimmed_length (const char* data, size_t size)
{
    for (; 0 < size && ' ' >= (uint8_t) data[size-1] ; --size) { }

    return size;
}

// returns the length in bytes of the line including the new line charcter(s)
// auto detects between windows(CRLF), unix(LF), mac(CR) and riscos (LFCR) line endings
size_t utf8_line_length (const char* data)
{
    size_t len = 0;

    for (len = 0; 0 != data[len]; ++len) {
        if ('\r' == data[len]) {
            if ('\n' == data[len+1]) {
                return len + 2; // windows
            } else {
                return len + 1; // unix
            }
        } else if ('\n' == data[len]) {
            if ('\r' == data[len+1]) {
                return len + 2; // riscos
            } else {
                return len + 1; // macos
            }
        }
    }

    return len;
}

// returns number of chars to include before split
utf8_size_t utf8_wrap_length (const utf8_char_t* data, utf8_size_t size)
{
    // Set split_at to size, so if a split point cna not be found, retuns the size passed in
    size_t char_length, char_count, split_at = size;

    for (char_count = 0 ; char_count <= size ; ++char_count) {
        if (' ' >= (*data)) {
            split_at = char_count;
        }

        char_length = utf8_char_length (data);
        data += char_length;
    }

    return split_at;
}

int utf8_line_count (const utf8_char_t* data)
{
    size_t len = 0;
    int count = 0;

    do {
        len = utf8_line_length (data);
        data += len; ++count;
    } while (0<len);

    return count-1;
}
