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
#ifndef LIBCAPTION_UTF8_H
#define LIBCAPTION_UTF8_H
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>

// These types exist to make the code more self dcoumenting
// utf8_char_t point is a null teminate string of utf8 encodecd chars
//
// utf8_size_t is the length of a string in chars
// size_t is bytes
typedef char utf8_char_t;
typedef size_t utf8_size_t;
/*! \brief
    \param

    Skiped continuation bytes
*/

const utf8_char_t* utf8_char_next(const utf8_char_t* c);
/*! \brief
    \param

    returnes the length of the char in bytes
*/
size_t utf8_char_length(const utf8_char_t* c);

/*! \brief
    \param

    returns 1 if first charcter is white space
*/
int utf8_char_whitespace(const utf8_char_t* c);

/*! \brief
    \param

    returns length of the string in bytes
    size is number of charcter to count (0 to count until NULL term)
*/
size_t utf8_string_length(const utf8_char_t* data, utf8_size_t size);
/*! \brief
    \param
*/
size_t utf8_char_copy(utf8_char_t* dst, const utf8_char_t* src);

/*! \brief
    \param

    returnes the number of utf8 charcters in a string givne the numbe of bytes
    to coutn until the a null terminator, pass 0 for size
*/
utf8_size_t utf8_char_count(const char* data, size_t size);
/*! \brief
    \param

    returnes the length of the line in bytes triming not printable characters at the end
*/
utf8_size_t utf8_trimmed_length(const utf8_char_t* data, utf8_size_t charcters);
/*! \brief
    \param

    returns the length in bytes of the line including the new line charcter(s)
    auto detects between windows(CRLF), unix(LF), mac(CR) and riscos (LFCR) line endings
*/
size_t utf8_line_length(const utf8_char_t* data);
/*! \brief
    \param

    returns number of chars to include before split
*/
utf8_size_t utf8_wrap_length(const utf8_char_t* data, utf8_size_t size);

/*! \brief
    \param

    returns number of new lines in the string
*/
int utf8_line_count(const utf8_char_t* data);

/*! \brief
    \param
    size in/out. In the the max seize, out is the size read;
    returns number of new lins in teh string
*/
#define UFTF_DEFAULT_MAX_FILE_SIZE = (50 * 1024 * 1024);

utf8_char_t* utf8_load_text_file(const char* path, size_t* size);

/*! \brief
    \param

    Compares 2 strings up to max len
*/
#ifndef strnstr
char* strnstr(const char* string1, const char* string2, size_t len);
#endif

#ifdef __cplusplus
}
#endif
#endif
