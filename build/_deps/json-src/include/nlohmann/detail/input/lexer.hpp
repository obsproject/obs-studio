//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <array> // array
#include <clocale> // localeconv
#include <cstddef> // size_t
#include <cstdio> // snprintf
#include <cstdlib> // strtof, strtod, strtold, strtoll, strtoull
#include <initializer_list> // initializer_list
#include <string> // char_traits, string
#include <utility> // move
#include <vector> // vector

#include <nlohmann/detail/input/input_adapters.hpp>
#include <nlohmann/detail/input/position_t.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/detail/meta/type_traits.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{

///////////
// lexer //
///////////

template<typename BasicJsonType>
class lexer_base
{
  public:
    /// token types for the parser
    enum class token_type
    {
        uninitialized,    ///< indicating the scanner is uninitialized
        literal_true,     ///< the `true` literal
        literal_false,    ///< the `false` literal
        literal_null,     ///< the `null` literal
        value_string,     ///< a string -- use get_string() for actual value
        value_unsigned,   ///< an unsigned integer -- use get_number_unsigned() for actual value
        value_integer,    ///< a signed integer -- use get_number_integer() for actual value
        value_float,      ///< an floating point number -- use get_number_float() for actual value
        begin_array,      ///< the character for array begin `[`
        begin_object,     ///< the character for object begin `{`
        end_array,        ///< the character for array end `]`
        end_object,       ///< the character for object end `}`
        name_separator,   ///< the name separator `:`
        value_separator,  ///< the value separator `,`
        parse_error,      ///< indicating a parse error
        end_of_input,     ///< indicating the end of the input buffer
        literal_or_value  ///< a literal or the begin of a value (only for diagnostics)
    };

    /// return name of values of type token_type (only used for errors)
    JSON_HEDLEY_RETURNS_NON_NULL
    JSON_HEDLEY_CONST
    static const char* token_type_name(const token_type t) noexcept
    {
        switch (t)
        {
            case token_type::uninitialized:
                return "<uninitialized>";
            case token_type::literal_true:
                return "true literal";
            case token_type::literal_false:
                return "false literal";
            case token_type::literal_null:
                return "null literal";
            case token_type::value_string:
                return "string literal";
            case token_type::value_unsigned:
            case token_type::value_integer:
            case token_type::value_float:
                return "number literal";
            case token_type::begin_array:
                return "'['";
            case token_type::begin_object:
                return "'{'";
            case token_type::end_array:
                return "']'";
            case token_type::end_object:
                return "'}'";
            case token_type::name_separator:
                return "':'";
            case token_type::value_separator:
                return "','";
            case token_type::parse_error:
                return "<parse error>";
            case token_type::end_of_input:
                return "end of input";
            case token_type::literal_or_value:
                return "'[', '{', or a literal";
            // LCOV_EXCL_START
            default: // catch non-enum values
                return "unknown token";
                // LCOV_EXCL_STOP
        }
    }
};
/*!
@brief lexical analysis

This class organizes the lexical analysis during JSON deserialization.
*/
template<typename BasicJsonType, typename InputAdapterType>
class lexer : public lexer_base<BasicJsonType>
{
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;
    using string_t = typename BasicJsonType::string_t;
    using char_type = typename InputAdapterType::char_type;
    using char_int_type = typename char_traits<char_type>::int_type;

  public:
    using token_type = typename lexer_base<BasicJsonType>::token_type;

    explicit lexer(InputAdapterType&& adapter, bool ignore_comments_ = false) noexcept
        : ia(std::move(adapter))
        , ignore_comments(ignore_comments_)
        , decimal_point_char(static_cast<char_int_type>(get_decimal_point()))
    {}

    // delete because of pointer members
    lexer(const lexer&) = delete;
    lexer(lexer&&) = default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor)
    lexer& operator=(lexer&) = delete;
    lexer& operator=(lexer&&) = default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor)
    ~lexer() = default;

  private:
    /////////////////////
    // locales
    /////////////////////

    /// return the locale-dependent decimal point
    JSON_HEDLEY_PURE
    static char get_decimal_point() noexcept
    {
        const auto* loc = localeconv();
        JSON_ASSERT(loc != nullptr);
        return (loc->decimal_point == nullptr) ? '.' : *(loc->decimal_point);
    }

    /////////////////////
    // scan functions
    /////////////////////

    /*!
    @brief get codepoint from 4 hex characters following `\u`

    For input "\u c1 c2 c3 c4" the codepoint is:
      (c1 * 0x1000) + (c2 * 0x0100) + (c3 * 0x0010) + c4
    = (c1 << 12) + (c2 << 8) + (c3 << 4) + (c4 << 0)

    Furthermore, the possible characters '0'..'9', 'A'..'F', and 'a'..'f'
    must be converted to the integers 0x0..0x9, 0xA..0xF, 0xA..0xF, resp. The
    conversion is done by subtracting the offset (0x30, 0x37, and 0x57)
    between the ASCII value of the character and the desired integer value.

    @return codepoint (0x0000..0xFFFF) or -1 in case of an error (e.g. EOF or
            non-hex character)
    */
    int get_codepoint()
    {
        // this function only makes sense after reading `\u`
        JSON_ASSERT(current == 'u');
        int codepoint = 0;

        const auto factors = { 12u, 8u, 4u, 0u };
        for (const auto factor : factors)
        {
            get();

            if (current >= '0' && current <= '9')
            {
                codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x30u) << factor);
            }
            else if (current >= 'A' && current <= 'F')
            {
                codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x37u) << factor);
            }
            else if (current >= 'a' && current <= 'f')
            {
                codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x57u) << factor);
            }
            else
            {
                return -1;
            }
        }

        JSON_ASSERT(0x0000 <= codepoint && codepoint <= 0xFFFF);
        return codepoint;
    }

    /*!
    @brief check if the next byte(s) are inside a given range

    Adds the current byte and, for each passed range, reads a new byte and
    checks if it is inside the range. If a violation was detected, set up an
    error message and return false. Otherwise, return true.

    @param[in] ranges  list of integers; interpreted as list of pairs of
                       inclusive lower and upper bound, respectively

    @pre The passed list @a ranges must have 2, 4, or 6 elements; that is,
         1, 2, or 3 pairs. This precondition is enforced by an assertion.

    @return true if and only if no range violation was detected
    */
    bool next_byte_in_range(std::initializer_list<char_int_type> ranges)
    {
        JSON_ASSERT(ranges.size() == 2 || ranges.size() == 4 || ranges.size() == 6);
        add(current);

        for (auto range = ranges.begin(); range != ranges.end(); ++range)
        {
            get();
            if (JSON_HEDLEY_LIKELY(*range <= current && current <= *(++range))) // NOLINT(bugprone-inc-dec-in-conditions)
            {
                add(current);
            }
            else
            {
                error_message = "invalid string: ill-formed UTF-8 byte";
                return false;
            }
        }

        return true;
    }

    /*!
    @brief scan a string literal

    This function scans a string according to Sect. 7 of RFC 8259. While
    scanning, bytes are escaped and copied into buffer token_buffer. Then the
    function returns successfully, token_buffer is *not* null-terminated (as it
    may contain \0 bytes), and token_buffer.size() is the number of bytes in the
    string.

    @return token_type::value_string if string could be successfully scanned,
            token_type::parse_error otherwise

    @note In case of errors, variable error_message contains a textual
          description.
    */
    token_type scan_string()
    {
        // reset token_buffer (ignore opening quote)
        reset();

        // we entered the function by reading an open quote
        JSON_ASSERT(current == '\"');

        while (true)
        {
            // get next character
            switch (get())
            {
                // end of file while parsing string
                case char_traits<char_type>::eof():
                {
                    error_message = "invalid string: missing closing quote";
                    return token_type::parse_error;
                }

                // closing quote
                case '\"':
                {
                    return token_type::value_string;
                }

                // escapes
                case '\\':
                {
                    switch (get())
                    {
                        // quotation mark
                        case '\"':
                            add('\"');
                            break;
                        // reverse solidus
                        case '\\':
                            add('\\');
                            break;
                        // solidus
                        case '/':
                            add('/');
                            break;
                        // backspace
                        case 'b':
                            add('\b');
                            break;
                        // form feed
                        case 'f':
                            add('\f');
                            break;
                        // line feed
                        case 'n':
                            add('\n');
                            break;
                        // carriage return
                        case 'r':
                            add('\r');
                            break;
                        // tab
                        case 't':
                            add('\t');
                            break;

                        // unicode escapes
                        case 'u':
                        {
                            const int codepoint1 = get_codepoint();
                            int codepoint = codepoint1; // start with codepoint1

                            if (JSON_HEDLEY_UNLIKELY(codepoint1 == -1))
                            {
                                error_message = "invalid string: '\\u' must be followed by 4 hex digits";
                                return token_type::parse_error;
                            }

                            // check if code point is a high surrogate
                            if (0xD800 <= codepoint1 && codepoint1 <= 0xDBFF)
                            {
                                // expect next \uxxxx entry
                                if (JSON_HEDLEY_LIKELY(get() == '\\' && get() == 'u'))
                                {
                                    const int codepoint2 = get_codepoint();

                                    if (JSON_HEDLEY_UNLIKELY(codepoint2 == -1))
                                    {
                                        error_message = "invalid string: '\\u' must be followed by 4 hex digits";
                                        return token_type::parse_error;
                                    }

                                    // check if codepoint2 is a low surrogate
                                    if (JSON_HEDLEY_LIKELY(0xDC00 <= codepoint2 && codepoint2 <= 0xDFFF))
                                    {
                                        // overwrite codepoint
                                        codepoint = static_cast<int>(
                                                        // high surrogate occupies the most significant 22 bits
                                                        (static_cast<unsigned int>(codepoint1) << 10u)
                                                        // low surrogate occupies the least significant 15 bits
                                                        + static_cast<unsigned int>(codepoint2)
                                                        // there is still the 0xD800, 0xDC00 and 0x10000 noise
                                                        // in the result, so we have to subtract with:
                                                        // (0xD800 << 10) + DC00 - 0x10000 = 0x35FDC00
                                                        - 0x35FDC00u);
                                    }
                                    else
                                    {
                                        error_message = "invalid string: surrogate U+D800..U+DBFF must be followed by U+DC00..U+DFFF";
                                        return token_type::parse_error;
                                    }
                                }
                                else
                                {
                                    error_message = "invalid string: surrogate U+D800..U+DBFF must be followed by U+DC00..U+DFFF";
                                    return token_type::parse_error;
                                }
                            }
                            else
                            {
                                if (JSON_HEDLEY_UNLIKELY(0xDC00 <= codepoint1 && codepoint1 <= 0xDFFF))
                                {
                                    error_message = "invalid string: surrogate U+DC00..U+DFFF must follow U+D800..U+DBFF";
                                    return token_type::parse_error;
                                }
                            }

                            // result of the above calculation yields a proper codepoint
                            JSON_ASSERT(0x00 <= codepoint && codepoint <= 0x10FFFF);

                            // translate codepoint into bytes
                            if (codepoint < 0x80)
                            {
                                // 1-byte characters: 0xxxxxxx (ASCII)
                                add(static_cast<char_int_type>(codepoint));
                            }
                            else if (codepoint <= 0x7FF)
                            {
                                // 2-byte characters: 110xxxxx 10xxxxxx
                                add(static_cast<char_int_type>(0xC0u | (static_cast<unsigned int>(codepoint) >> 6u)));
                                add(static_cast<char_int_type>(0x80u | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
                            }
                            else if (codepoint <= 0xFFFF)
                            {
                                // 3-byte characters: 1110xxxx 10xxxxxx 10xxxxxx
                                add(static_cast<char_int_type>(0xE0u | (static_cast<unsigned int>(codepoint) >> 12u)));
                                add(static_cast<char_int_type>(0x80u | ((static_cast<unsigned int>(codepoint) >> 6u) & 0x3Fu)));
                                add(static_cast<char_int_type>(0x80u | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
                            }
                            else
                            {
                                // 4-byte characters: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                                add(static_cast<char_int_type>(0xF0u | (static_cast<unsigned int>(codepoint) >> 18u)));
                                add(static_cast<char_int_type>(0x80u | ((static_cast<unsigned int>(codepoint) >> 12u) & 0x3Fu)));
                                add(static_cast<char_int_type>(0x80u | ((static_cast<unsigned int>(codepoint) >> 6u) & 0x3Fu)));
                                add(static_cast<char_int_type>(0x80u | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
                            }

                            break;
                        }

                        // other characters after escape
                        default:
                            error_message = "invalid string: forbidden character after backslash";
                            return token_type::parse_error;
                    }

                    break;
                }

                // invalid control characters
                case 0x00:
                {
                    error_message = "invalid string: control character U+0000 (NUL) must be escaped to \\u0000";
                    return token_type::parse_error;
                }

                case 0x01:
                {
                    error_message = "invalid string: control character U+0001 (SOH) must be escaped to \\u0001";
                    return token_type::parse_error;
                }

                case 0x02:
                {
                    error_message = "invalid string: control character U+0002 (STX) must be escaped to \\u0002";
                    return token_type::parse_error;
                }

                case 0x03:
                {
                    error_message = "invalid string: control character U+0003 (ETX) must be escaped to \\u0003";
                    return token_type::parse_error;
                }

                case 0x04:
                {
                    error_message = "invalid string: control character U+0004 (EOT) must be escaped to \\u0004";
                    return token_type::parse_error;
                }

                case 0x05:
                {
                    error_message = "invalid string: control character U+0005 (ENQ) must be escaped to \\u0005";
                    return token_type::parse_error;
                }

                case 0x06:
                {
                    error_message = "invalid string: control character U+0006 (ACK) must be escaped to \\u0006";
                    return token_type::parse_error;
                }

                case 0x07:
                {
                    error_message = "invalid string: control character U+0007 (BEL) must be escaped to \\u0007";
                    return token_type::parse_error;
                }

                case 0x08:
                {
                    error_message = "invalid string: control character U+0008 (BS) must be escaped to \\u0008 or \\b";
                    return token_type::parse_error;
                }

                case 0x09:
                {
                    error_message = "invalid string: control character U+0009 (HT) must be escaped to \\u0009 or \\t";
                    return token_type::parse_error;
                }

                case 0x0A:
                {
                    error_message = "invalid string: control character U+000A (LF) must be escaped to \\u000A or \\n";
                    return token_type::parse_error;
                }

                case 0x0B:
                {
                    error_message = "invalid string: control character U+000B (VT) must be escaped to \\u000B";
                    return token_type::parse_error;
                }

                case 0x0C:
                {
                    error_message = "invalid string: control character U+000C (FF) must be escaped to \\u000C or \\f";
                    return token_type::parse_error;
                }

                case 0x0D:
                {
                    error_message = "invalid string: control character U+000D (CR) must be escaped to \\u000D or \\r";
                    return token_type::parse_error;
                }

                case 0x0E:
                {
                    error_message = "invalid string: control character U+000E (SO) must be escaped to \\u000E";
                    return token_type::parse_error;
                }

                case 0x0F:
                {
                    error_message = "invalid string: control character U+000F (SI) must be escaped to \\u000F";
                    return token_type::parse_error;
                }

                case 0x10:
                {
                    error_message = "invalid string: control character U+0010 (DLE) must be escaped to \\u0010";
                    return token_type::parse_error;
                }

                case 0x11:
                {
                    error_message = "invalid string: control character U+0011 (DC1) must be escaped to \\u0011";
                    return token_type::parse_error;
                }

                case 0x12:
                {
                    error_message = "invalid string: control character U+0012 (DC2) must be escaped to \\u0012";
                    return token_type::parse_error;
                }

                case 0x13:
                {
                    error_message = "invalid string: control character U+0013 (DC3) must be escaped to \\u0013";
                    return token_type::parse_error;
                }

                case 0x14:
                {
                    error_message = "invalid string: control character U+0014 (DC4) must be escaped to \\u0014";
                    return token_type::parse_error;
                }

                case 0x15:
                {
                    error_message = "invalid string: control character U+0015 (NAK) must be escaped to \\u0015";
                    return token_type::parse_error;
                }

                case 0x16:
                {
                    error_message = "invalid string: control character U+0016 (SYN) must be escaped to \\u0016";
                    return token_type::parse_error;
                }

                case 0x17:
                {
                    error_message = "invalid string: control character U+0017 (ETB) must be escaped to \\u0017";
                    return token_type::parse_error;
                }

                case 0x18:
                {
                    error_message = "invalid string: control character U+0018 (CAN) must be escaped to \\u0018";
                    return token_type::parse_error;
                }

                case 0x19:
                {
                    error_message = "invalid string: control character U+0019 (EM) must be escaped to \\u0019";
                    return token_type::parse_error;
                }

                case 0x1A:
                {
                    error_message = "invalid string: control character U+001A (SUB) must be escaped to \\u001A";
                    return token_type::parse_error;
                }

                case 0x1B:
                {
                    error_message = "invalid string: control character U+001B (ESC) must be escaped to \\u001B";
                    return token_type::parse_error;
                }

                case 0x1C:
                {
                    error_message = "invalid string: control character U+001C (FS) must be escaped to \\u001C";
                    return token_type::parse_error;
                }

                case 0x1D:
                {
                    error_message = "invalid string: control character U+001D (GS) must be escaped to \\u001D";
                    return token_type::parse_error;
                }

                case 0x1E:
                {
                    error_message = "invalid string: control character U+001E (RS) must be escaped to \\u001E";
                    return token_type::parse_error;
                }

                case 0x1F:
                {
                    error_message = "invalid string: control character U+001F (US) must be escaped to \\u001F";
                    return token_type::parse_error;
                }

                // U+0020..U+007F (except U+0022 (quote) and U+005C (backspace))
                case 0x20:
                case 0x21:
                case 0x23:
                case 0x24:
                case 0x25:
                case 0x26:
                case 0x27:
                case 0x28:
                case 0x29:
                case 0x2A:
                case 0x2B:
                case 0x2C:
                case 0x2D:
                case 0x2E:
                case 0x2F:
                case 0x30:
                case 0x31:
                case 0x32:
                case 0x33:
                case 0x34:
                case 0x35:
                case 0x36:
                case 0x37:
                case 0x38:
                case 0x39:
                case 0x3A:
                case 0x3B:
                case 0x3C:
                case 0x3D:
                case 0x3E:
                case 0x3F:
                case 0x40:
                case 0x41:
                case 0x42:
                case 0x43:
                case 0x44:
                case 0x45:
                case 0x46:
                case 0x47:
                case 0x48:
                case 0x49:
                case 0x4A:
                case 0x4B:
                case 0x4C:
                case 0x4D:
                case 0x4E:
                case 0x4F:
                case 0x50:
                case 0x51:
                case 0x52:
                case 0x53:
                case 0x54:
                case 0x55:
                case 0x56:
                case 0x57:
                case 0x58:
                case 0x59:
                case 0x5A:
                case 0x5B:
                case 0x5D:
                case 0x5E:
                case 0x5F:
                case 0x60:
                case 0x61:
                case 0x62:
                case 0x63:
                case 0x64:
                case 0x65:
                case 0x66:
                case 0x67:
                case 0x68:
                case 0x69:
                case 0x6A:
                case 0x6B:
                case 0x6C:
                case 0x6D:
                case 0x6E:
                case 0x6F:
                case 0x70:
                case 0x71:
                case 0x72:
                case 0x73:
                case 0x74:
                case 0x75:
                case 0x76:
                case 0x77:
                case 0x78:
                case 0x79:
                case 0x7A:
                case 0x7B:
                case 0x7C:
                case 0x7D:
                case 0x7E:
                case 0x7F:
                {
                    add(current);
                    break;
                }

                // U+0080..U+07FF: bytes C2..DF 80..BF
                case 0xC2:
                case 0xC3:
                case 0xC4:
                case 0xC5:
                case 0xC6:
                case 0xC7:
                case 0xC8:
                case 0xC9:
                case 0xCA:
                case 0xCB:
                case 0xCC:
                case 0xCD:
                case 0xCE:
                case 0xCF:
                case 0xD0:
                case 0xD1:
                case 0xD2:
                case 0xD3:
                case 0xD4:
                case 0xD5:
                case 0xD6:
                case 0xD7:
                case 0xD8:
                case 0xD9:
                case 0xDA:
                case 0xDB:
                case 0xDC:
                case 0xDD:
                case 0xDE:
                case 0xDF:
                {
                    if (JSON_HEDLEY_UNLIKELY(!next_byte_in_range({0x80, 0xBF})))
                    {
                        return token_type::parse_error;
                    }
                    break;
                }

                // U+0800..U+0FFF: bytes E0 A0..BF 80..BF
                case 0xE0:
                {
                    if (JSON_HEDLEY_UNLIKELY(!(next_byte_in_range({0xA0, 0xBF, 0x80, 0xBF}))))
                    {
                        return token_type::parse_error;
                    }
                    break;
                }

                // U+1000..U+CFFF: bytes E1..EC 80..BF 80..BF
                // U+E000..U+FFFF: bytes EE..EF 80..BF 80..BF
                case 0xE1:
                case 0xE2:
                case 0xE3:
                case 0xE4:
                case 0xE5:
                case 0xE6:
                case 0xE7:
                case 0xE8:
                case 0xE9:
                case 0xEA:
                case 0xEB:
                case 0xEC:
                case 0xEE:
                case 0xEF:
                {
                    if (JSON_HEDLEY_UNLIKELY(!(next_byte_in_range({0x80, 0xBF, 0x80, 0xBF}))))
                    {
                        return token_type::parse_error;
                    }
                    break;
                }

                // U+D000..U+D7FF: bytes ED 80..9F 80..BF
                case 0xED:
                {
                    if (JSON_HEDLEY_UNLIKELY(!(next_byte_in_range({0x80, 0x9F, 0x80, 0xBF}))))
                    {
                        return token_type::parse_error;
                    }
                    break;
                }

                // U+10000..U+3FFFF F0 90..BF 80..BF 80..BF
                case 0xF0:
                {
                    if (JSON_HEDLEY_UNLIKELY(!(next_byte_in_range({0x90, 0xBF, 0x80, 0xBF, 0x80, 0xBF}))))
                    {
                        return token_type::parse_error;
                    }
                    break;
                }

                // U+40000..U+FFFFF F1..F3 80..BF 80..BF 80..BF
                case 0xF1:
                case 0xF2:
                case 0xF3:
                {
                    if (JSON_HEDLEY_UNLIKELY(!(next_byte_in_range({0x80, 0xBF, 0x80, 0xBF, 0x80, 0xBF}))))
                    {
                        return token_type::parse_error;
                    }
                    break;
                }

                // U+100000..U+10FFFF F4 80..8F 80..BF 80..BF
                case 0xF4:
                {
                    if (JSON_HEDLEY_UNLIKELY(!(next_byte_in_range({0x80, 0x8F, 0x80, 0xBF, 0x80, 0xBF}))))
                    {
                        return token_type::parse_error;
                    }
                    break;
                }

                // remaining bytes (80..C1 and F5..FF) are ill-formed
                default:
                {
                    error_message = "invalid string: ill-formed UTF-8 byte";
                    return token_type::parse_error;
                }
            }
        }
    }

    /*!
     * @brief scan a comment
     * @return whether comment could be scanned successfully
     */
    bool scan_comment()
    {
        switch (get())
        {
            // single-line comments skip input until a newline or EOF is read
            case '/':
            {
                while (true)
                {
                    switch (get())
                    {
                        case '\n':
                        case '\r':
                        case char_traits<char_type>::eof():
                        case '\0':
                            return true;

                        default:
                            break;
                    }
                }
            }

            // multi-line comments skip input until */ is read
            case '*':
            {
                while (true)
                {
                    switch (get())
                    {
                        case char_traits<char_type>::eof():
                        case '\0':
                        {
                            error_message = "invalid comment; missing closing '*/'";
                            return false;
                        }

                        case '*':
                        {
                            switch (get())
                            {
                                case '/':
                                    return true;

                                default:
                                {
                                    unget();
                                    continue;
                                }
                            }
                        }

                        default:
                            continue;
                    }
                }
            }

            // unexpected character after reading '/'
            default:
            {
                error_message = "invalid comment; expecting '/' or '*' after '/'";
                return false;
            }
        }
    }

    JSON_HEDLEY_NON_NULL(2)
    static void strtof(float& f, const char* str, char** endptr) noexcept
    {
        f = std::strtof(str, endptr);
    }

    JSON_HEDLEY_NON_NULL(2)
    static void strtof(double& f, const char* str, char** endptr) noexcept
    {
        f = std::strtod(str, endptr);
    }

    JSON_HEDLEY_NON_NULL(2)
    static void strtof(long double& f, const char* str, char** endptr) noexcept
    {
        f = std::strtold(str, endptr);
    }

    /*!
    @brief scan a number literal

    This function scans a string according to Sect. 6 of RFC 8259.

    The function is realized with a deterministic finite state machine derived
    from the grammar described in RFC 8259. Starting in state "init", the
    input is read and used to determined the next state. Only state "done"
    accepts the number. State "error" is a trap state to model errors. In the
    table below, "anything" means any character but the ones listed before.

    state    | 0        | 1-9      | e E      | +       | -       | .        | anything
    ---------|----------|----------|----------|---------|---------|----------|-----------
    init     | zero     | any1     | [error]  | [error] | minus   | [error]  | [error]
    minus    | zero     | any1     | [error]  | [error] | [error] | [error]  | [error]
    zero     | done     | done     | exponent | done    | done    | decimal1 | done
    any1     | any1     | any1     | exponent | done    | done    | decimal1 | done
    decimal1 | decimal2 | decimal2 | [error]  | [error] | [error] | [error]  | [error]
    decimal2 | decimal2 | decimal2 | exponent | done    | done    | done     | done
    exponent | any2     | any2     | [error]  | sign    | sign    | [error]  | [error]
    sign     | any2     | any2     | [error]  | [error] | [error] | [error]  | [error]
    any2     | any2     | any2     | done     | done    | done    | done     | done

    The state machine is realized with one label per state (prefixed with
    "scan_number_") and `goto` statements between them. The state machine
    contains cycles, but any cycle can be left when EOF is read. Therefore,
    the function is guaranteed to terminate.

    During scanning, the read bytes are stored in token_buffer. This string is
    then converted to a signed integer, an unsigned integer, or a
    floating-point number.

    @return token_type::value_unsigned, token_type::value_integer, or
            token_type::value_float if number could be successfully scanned,
            token_type::parse_error otherwise

    @note The scanner is independent of the current locale. Internally, the
          locale's decimal point is used instead of `.` to work with the
          locale-dependent converters.
    */
    token_type scan_number()  // lgtm [cpp/use-of-goto]
    {
        // reset token_buffer to store the number's bytes
        reset();

        // the type of the parsed number; initially set to unsigned; will be
        // changed if minus sign, decimal point or exponent is read
        token_type number_type = token_type::value_unsigned;

        // state (init): we just found out we need to scan a number
        switch (current)
        {
            case '-':
            {
                add(current);
                goto scan_number_minus;
            }

            case '0':
            {
                add(current);
                goto scan_number_zero;
            }

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                add(current);
                goto scan_number_any1;
            }

            // all other characters are rejected outside scan_number()
            default:            // LCOV_EXCL_LINE
                JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
        }

scan_number_minus:
        // state: we just parsed a leading minus sign
        number_type = token_type::value_integer;
        switch (get())
        {
            case '0':
            {
                add(current);
                goto scan_number_zero;
            }

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                add(current);
                goto scan_number_any1;
            }

            default:
            {
                error_message = "invalid number; expected digit after '-'";
                return token_type::parse_error;
            }
        }

scan_number_zero:
        // state: we just parse a zero (maybe with a leading minus sign)
        switch (get())
        {
            case '.':
            {
                add(decimal_point_char);
                goto scan_number_decimal1;
            }

            case 'e':
            case 'E':
            {
                add(current);
                goto scan_number_exponent;
            }

            default:
                goto scan_number_done;
        }

scan_number_any1:
        // state: we just parsed a number 0-9 (maybe with a leading minus sign)
        switch (get())
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                add(current);
                goto scan_number_any1;
            }

            case '.':
            {
                add(decimal_point_char);
                goto scan_number_decimal1;
            }

            case 'e':
            case 'E':
            {
                add(current);
                goto scan_number_exponent;
            }

            default:
                goto scan_number_done;
        }

scan_number_decimal1:
        // state: we just parsed a decimal point
        number_type = token_type::value_float;
        switch (get())
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                add(current);
                goto scan_number_decimal2;
            }

            default:
            {
                error_message = "invalid number; expected digit after '.'";
                return token_type::parse_error;
            }
        }

scan_number_decimal2:
        // we just parsed at least one number after a decimal point
        switch (get())
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                add(current);
                goto scan_number_decimal2;
            }

            case 'e':
            case 'E':
            {
                add(current);
                goto scan_number_exponent;
            }

            default:
                goto scan_number_done;
        }

scan_number_exponent:
        // we just parsed an exponent
        number_type = token_type::value_float;
        switch (get())
        {
            case '+':
            case '-':
            {
                add(current);
                goto scan_number_sign;
            }

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                add(current);
                goto scan_number_any2;
            }

            default:
            {
                error_message =
                    "invalid number; expected '+', '-', or digit after exponent";
                return token_type::parse_error;
            }
        }

scan_number_sign:
        // we just parsed an exponent sign
        switch (get())
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                add(current);
                goto scan_number_any2;
            }

            default:
            {
                error_message = "invalid number; expected digit after exponent sign";
                return token_type::parse_error;
            }
        }

scan_number_any2:
        // we just parsed a number after the exponent or exponent sign
        switch (get())
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                add(current);
                goto scan_number_any2;
            }

            default:
                goto scan_number_done;
        }

scan_number_done:
        // unget the character after the number (we only read it to know that
        // we are done scanning a number)
        unget();

        char* endptr = nullptr; // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
        errno = 0;

        // try to parse integers first and fall back to floats
        if (number_type == token_type::value_unsigned)
        {
            const auto x = std::strtoull(token_buffer.data(), &endptr, 10);

            // we checked the number format before
            JSON_ASSERT(endptr == token_buffer.data() + token_buffer.size());

            if (errno == 0)
            {
                value_unsigned = static_cast<number_unsigned_t>(x);
                if (value_unsigned == x)
                {
                    return token_type::value_unsigned;
                }
            }
        }
        else if (number_type == token_type::value_integer)
        {
            const auto x = std::strtoll(token_buffer.data(), &endptr, 10);

            // we checked the number format before
            JSON_ASSERT(endptr == token_buffer.data() + token_buffer.size());

            if (errno == 0)
            {
                value_integer = static_cast<number_integer_t>(x);
                if (value_integer == x)
                {
                    return token_type::value_integer;
                }
            }
        }

        // this code is reached if we parse a floating-point number or if an
        // integer conversion above failed
        strtof(value_float, token_buffer.data(), &endptr);

        // we checked the number format before
        JSON_ASSERT(endptr == token_buffer.data() + token_buffer.size());

        return token_type::value_float;
    }

    /*!
    @param[in] literal_text  the literal text to expect
    @param[in] length        the length of the passed literal text
    @param[in] return_type   the token type to return on success
    */
    JSON_HEDLEY_NON_NULL(2)
    token_type scan_literal(const char_type* literal_text, const std::size_t length,
                            token_type return_type)
    {
        JSON_ASSERT(char_traits<char_type>::to_char_type(current) == literal_text[0]);
        for (std::size_t i = 1; i < length; ++i)
        {
            if (JSON_HEDLEY_UNLIKELY(char_traits<char_type>::to_char_type(get()) != literal_text[i]))
            {
                error_message = "invalid literal";
                return token_type::parse_error;
            }
        }
        return return_type;
    }

    /////////////////////
    // input management
    /////////////////////

    /// reset token_buffer; current character is beginning of token
    void reset() noexcept
    {
        token_buffer.clear();
        token_string.clear();
        token_string.push_back(char_traits<char_type>::to_char_type(current));
    }

    /*
    @brief get next character from the input

    This function provides the interface to the used input adapter. It does
    not throw in case the input reached EOF, but returns a
    `char_traits<char>::eof()` in that case.  Stores the scanned characters
    for use in error messages.

    @return character read from the input
    */
    char_int_type get()
    {
        ++position.chars_read_total;
        ++position.chars_read_current_line;

        if (next_unget)
        {
            // just reset the next_unget variable and work with current
            next_unget = false;
        }
        else
        {
            current = ia.get_character();
        }

        if (JSON_HEDLEY_LIKELY(current != char_traits<char_type>::eof()))
        {
            token_string.push_back(char_traits<char_type>::to_char_type(current));
        }

        if (current == '\n')
        {
            ++position.lines_read;
            position.chars_read_current_line = 0;
        }

        return current;
    }

    /*!
    @brief unget current character (read it again on next get)

    We implement unget by setting variable next_unget to true. The input is not
    changed - we just simulate ungetting by modifying chars_read_total,
    chars_read_current_line, and token_string. The next call to get() will
    behave as if the unget character is read again.
    */
    void unget()
    {
        next_unget = true;

        --position.chars_read_total;

        // in case we "unget" a newline, we have to also decrement the lines_read
        if (position.chars_read_current_line == 0)
        {
            if (position.lines_read > 0)
            {
                --position.lines_read;
            }
        }
        else
        {
            --position.chars_read_current_line;
        }

        if (JSON_HEDLEY_LIKELY(current != char_traits<char_type>::eof()))
        {
            JSON_ASSERT(!token_string.empty());
            token_string.pop_back();
        }
    }

    /// add a character to token_buffer
    void add(char_int_type c)
    {
        token_buffer.push_back(static_cast<typename string_t::value_type>(c));
    }

  public:
    /////////////////////
    // value getters
    /////////////////////

    /// return integer value
    constexpr number_integer_t get_number_integer() const noexcept
    {
        return value_integer;
    }

    /// return unsigned integer value
    constexpr number_unsigned_t get_number_unsigned() const noexcept
    {
        return value_unsigned;
    }

    /// return floating-point value
    constexpr number_float_t get_number_float() const noexcept
    {
        return value_float;
    }

    /// return current string value (implicitly resets the token; useful only once)
    string_t& get_string()
    {
        return token_buffer;
    }

    /////////////////////
    // diagnostics
    /////////////////////

    /// return position of last read token
    constexpr position_t get_position() const noexcept
    {
        return position;
    }

    /// return the last read token (for errors only).  Will never contain EOF
    /// (an arbitrary value that is not a valid char value, often -1), because
    /// 255 may legitimately occur.  May contain NUL, which should be escaped.
    std::string get_token_string() const
    {
        // escape control characters
        std::string result;
        for (const auto c : token_string)
        {
            if (static_cast<unsigned char>(c) <= '\x1F')
            {
                // escape control characters
                std::array<char, 9> cs{{}};
                static_cast<void>((std::snprintf)(cs.data(), cs.size(), "<U+%.4X>", static_cast<unsigned char>(c))); // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
                result += cs.data();
            }
            else
            {
                // add character as is
                result.push_back(static_cast<std::string::value_type>(c));
            }
        }

        return result;
    }

    /// return syntax error message
    JSON_HEDLEY_RETURNS_NON_NULL
    constexpr const char* get_error_message() const noexcept
    {
        return error_message;
    }

    /////////////////////
    // actual scanner
    /////////////////////

    /*!
    @brief skip the UTF-8 byte order mark
    @return true iff there is no BOM or the correct BOM has been skipped
    */
    bool skip_bom()
    {
        if (get() == 0xEF)
        {
            // check if we completely parse the BOM
            return get() == 0xBB && get() == 0xBF;
        }

        // the first character is not the beginning of the BOM; unget it to
        // process is later
        unget();
        return true;
    }

    void skip_whitespace()
    {
        do
        {
            get();
        }
        while (current == ' ' || current == '\t' || current == '\n' || current == '\r');
    }

    token_type scan()
    {
        // initially, skip the BOM
        if (position.chars_read_total == 0 && !skip_bom())
        {
            error_message = "invalid BOM; must be 0xEF 0xBB 0xBF if given";
            return token_type::parse_error;
        }

        // read next character and ignore whitespace
        skip_whitespace();

        // ignore comments
        while (ignore_comments && current == '/')
        {
            if (!scan_comment())
            {
                return token_type::parse_error;
            }

            // skip following whitespace
            skip_whitespace();
        }

        switch (current)
        {
            // structural characters
            case '[':
                return token_type::begin_array;
            case ']':
                return token_type::end_array;
            case '{':
                return token_type::begin_object;
            case '}':
                return token_type::end_object;
            case ':':
                return token_type::name_separator;
            case ',':
                return token_type::value_separator;

            // literals
            case 't':
            {
                std::array<char_type, 4> true_literal = {{static_cast<char_type>('t'), static_cast<char_type>('r'), static_cast<char_type>('u'), static_cast<char_type>('e')}};
                return scan_literal(true_literal.data(), true_literal.size(), token_type::literal_true);
            }
            case 'f':
            {
                std::array<char_type, 5> false_literal = {{static_cast<char_type>('f'), static_cast<char_type>('a'), static_cast<char_type>('l'), static_cast<char_type>('s'), static_cast<char_type>('e')}};
                return scan_literal(false_literal.data(), false_literal.size(), token_type::literal_false);
            }
            case 'n':
            {
                std::array<char_type, 4> null_literal = {{static_cast<char_type>('n'), static_cast<char_type>('u'), static_cast<char_type>('l'), static_cast<char_type>('l')}};
                return scan_literal(null_literal.data(), null_literal.size(), token_type::literal_null);
            }

            // string
            case '\"':
                return scan_string();

            // number
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return scan_number();

            // end of input (the null byte is needed when parsing from
            // string literals)
            case '\0':
            case char_traits<char_type>::eof():
                return token_type::end_of_input;

            // error
            default:
                error_message = "invalid literal";
                return token_type::parse_error;
        }
    }

  private:
    /// input adapter
    InputAdapterType ia;

    /// whether comments should be ignored (true) or signaled as errors (false)
    const bool ignore_comments = false;

    /// the current character
    char_int_type current = char_traits<char_type>::eof();

    /// whether the next get() call should just return current
    bool next_unget = false;

    /// the start position of the current token
    position_t position {};

    /// raw input token string (for error messages)
    std::vector<char_type> token_string {};

    /// buffer for variable-length tokens (numbers, strings)
    string_t token_buffer {};

    /// a description of occurred lexer errors
    const char* error_message = "";

    // number values
    number_integer_t value_integer = 0;
    number_unsigned_t value_unsigned = 0;
    number_float_t value_float = 0;

    /// the decimal point
    const char_int_type decimal_point_char = '.';
};

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END
