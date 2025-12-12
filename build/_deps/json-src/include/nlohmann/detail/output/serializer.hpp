//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2008-2009 Björn Hoehrmann <bjoern@hoehrmann.de>
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm> // reverse, remove, fill, find, none_of
#include <array> // array
#include <clocale> // localeconv, lconv
#include <cmath> // labs, isfinite, isnan, signbit
#include <cstddef> // size_t, ptrdiff_t
#include <cstdint> // uint8_t
#include <cstdio> // snprintf
#include <limits> // numeric_limits
#include <string> // string, char_traits
#include <iomanip> // setfill, setw
#include <type_traits> // is_same
#include <utility> // move

#include <nlohmann/detail/conversions/to_chars.hpp>
#include <nlohmann/detail/exceptions.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/detail/meta/cpp_future.hpp>
#include <nlohmann/detail/output/binary_writer.hpp>
#include <nlohmann/detail/output/output_adapters.hpp>
#include <nlohmann/detail/string_concat.hpp>
#include <nlohmann/detail/value_t.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{

///////////////////
// serialization //
///////////////////

/// how to treat decoding errors
enum class error_handler_t
{
    strict,  ///< throw a type_error exception in case of invalid UTF-8
    replace, ///< replace invalid UTF-8 sequences with U+FFFD
    ignore   ///< ignore invalid UTF-8 sequences
};

template<typename BasicJsonType>
class serializer
{
    using string_t = typename BasicJsonType::string_t;
    using number_float_t = typename BasicJsonType::number_float_t;
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using binary_char_t = typename BasicJsonType::binary_t::value_type;
    static constexpr std::uint8_t UTF8_ACCEPT = 0;
    static constexpr std::uint8_t UTF8_REJECT = 1;

  public:
    /*!
    @param[in] s  output stream to serialize to
    @param[in] ichar  indentation character to use
    @param[in] error_handler_  how to react on decoding errors
    */
    serializer(output_adapter_t<char> s, const char ichar,
               error_handler_t error_handler_ = error_handler_t::strict)
        : o(std::move(s))
        , loc(std::localeconv())
        , thousands_sep(loc->thousands_sep == nullptr ? '\0' : std::char_traits<char>::to_char_type(* (loc->thousands_sep)))
        , decimal_point(loc->decimal_point == nullptr ? '\0' : std::char_traits<char>::to_char_type(* (loc->decimal_point)))
        , indent_char(ichar)
        , indent_string(512, indent_char)
        , error_handler(error_handler_)
    {}

    // delete because of pointer members
    serializer(const serializer&) = delete;
    serializer& operator=(const serializer&) = delete;
    serializer(serializer&&) = delete;
    serializer& operator=(serializer&&) = delete;
    ~serializer() = default;

    /*!
    @brief internal implementation of the serialization function

    This function is called by the public member function dump and organizes
    the serialization internally. The indentation level is propagated as
    additional parameter. In case of arrays and objects, the function is
    called recursively.

    - strings and object keys are escaped using `escape_string()`
    - integer numbers are converted implicitly via `operator<<`
    - floating-point numbers are converted to a string using `"%g"` format
    - binary values are serialized as objects containing the subtype and the
      byte array

    @param[in] val               value to serialize
    @param[in] pretty_print      whether the output shall be pretty-printed
    @param[in] ensure_ascii If @a ensure_ascii is true, all non-ASCII characters
    in the output are escaped with `\uXXXX` sequences, and the result consists
    of ASCII characters only.
    @param[in] indent_step       the indent level
    @param[in] current_indent    the current indent level (only used internally)
    */
    void dump(const BasicJsonType& val,
              const bool pretty_print,
              const bool ensure_ascii,
              const unsigned int indent_step,
              const unsigned int current_indent = 0)
    {
        switch (val.m_data.m_type)
        {
            case value_t::object:
            {
                if (val.m_data.m_value.object->empty())
                {
                    o->write_characters("{}", 2);
                    return;
                }

                if (pretty_print)
                {
                    o->write_characters("{\n", 2);

                    // variable to hold indentation for recursive calls
                    const auto new_indent = current_indent + indent_step;
                    if (JSON_HEDLEY_UNLIKELY(indent_string.size() < new_indent))
                    {
                        indent_string.resize(indent_string.size() * 2, ' ');
                    }

                    // first n-1 elements
                    auto i = val.m_data.m_value.object->cbegin();
                    for (std::size_t cnt = 0; cnt < val.m_data.m_value.object->size() - 1; ++cnt, ++i)
                    {
                        o->write_characters(indent_string.c_str(), new_indent);
                        o->write_character('\"');
                        dump_escaped(i->first, ensure_ascii);
                        o->write_characters("\": ", 3);
                        dump(i->second, true, ensure_ascii, indent_step, new_indent);
                        o->write_characters(",\n", 2);
                    }

                    // last element
                    JSON_ASSERT(i != val.m_data.m_value.object->cend());
                    JSON_ASSERT(std::next(i) == val.m_data.m_value.object->cend());
                    o->write_characters(indent_string.c_str(), new_indent);
                    o->write_character('\"');
                    dump_escaped(i->first, ensure_ascii);
                    o->write_characters("\": ", 3);
                    dump(i->second, true, ensure_ascii, indent_step, new_indent);

                    o->write_character('\n');
                    o->write_characters(indent_string.c_str(), current_indent);
                    o->write_character('}');
                }
                else
                {
                    o->write_character('{');

                    // first n-1 elements
                    auto i = val.m_data.m_value.object->cbegin();
                    for (std::size_t cnt = 0; cnt < val.m_data.m_value.object->size() - 1; ++cnt, ++i)
                    {
                        o->write_character('\"');
                        dump_escaped(i->first, ensure_ascii);
                        o->write_characters("\":", 2);
                        dump(i->second, false, ensure_ascii, indent_step, current_indent);
                        o->write_character(',');
                    }

                    // last element
                    JSON_ASSERT(i != val.m_data.m_value.object->cend());
                    JSON_ASSERT(std::next(i) == val.m_data.m_value.object->cend());
                    o->write_character('\"');
                    dump_escaped(i->first, ensure_ascii);
                    o->write_characters("\":", 2);
                    dump(i->second, false, ensure_ascii, indent_step, current_indent);

                    o->write_character('}');
                }

                return;
            }

            case value_t::array:
            {
                if (val.m_data.m_value.array->empty())
                {
                    o->write_characters("[]", 2);
                    return;
                }

                if (pretty_print)
                {
                    o->write_characters("[\n", 2);

                    // variable to hold indentation for recursive calls
                    const auto new_indent = current_indent + indent_step;
                    if (JSON_HEDLEY_UNLIKELY(indent_string.size() < new_indent))
                    {
                        indent_string.resize(indent_string.size() * 2, ' ');
                    }

                    // first n-1 elements
                    for (auto i = val.m_data.m_value.array->cbegin();
                            i != val.m_data.m_value.array->cend() - 1; ++i)
                    {
                        o->write_characters(indent_string.c_str(), new_indent);
                        dump(*i, true, ensure_ascii, indent_step, new_indent);
                        o->write_characters(",\n", 2);
                    }

                    // last element
                    JSON_ASSERT(!val.m_data.m_value.array->empty());
                    o->write_characters(indent_string.c_str(), new_indent);
                    dump(val.m_data.m_value.array->back(), true, ensure_ascii, indent_step, new_indent);

                    o->write_character('\n');
                    o->write_characters(indent_string.c_str(), current_indent);
                    o->write_character(']');
                }
                else
                {
                    o->write_character('[');

                    // first n-1 elements
                    for (auto i = val.m_data.m_value.array->cbegin();
                            i != val.m_data.m_value.array->cend() - 1; ++i)
                    {
                        dump(*i, false, ensure_ascii, indent_step, current_indent);
                        o->write_character(',');
                    }

                    // last element
                    JSON_ASSERT(!val.m_data.m_value.array->empty());
                    dump(val.m_data.m_value.array->back(), false, ensure_ascii, indent_step, current_indent);

                    o->write_character(']');
                }

                return;
            }

            case value_t::string:
            {
                o->write_character('\"');
                dump_escaped(*val.m_data.m_value.string, ensure_ascii);
                o->write_character('\"');
                return;
            }

            case value_t::binary:
            {
                if (pretty_print)
                {
                    o->write_characters("{\n", 2);

                    // variable to hold indentation for recursive calls
                    const auto new_indent = current_indent + indent_step;
                    if (JSON_HEDLEY_UNLIKELY(indent_string.size() < new_indent))
                    {
                        indent_string.resize(indent_string.size() * 2, ' ');
                    }

                    o->write_characters(indent_string.c_str(), new_indent);

                    o->write_characters("\"bytes\": [", 10);

                    if (!val.m_data.m_value.binary->empty())
                    {
                        for (auto i = val.m_data.m_value.binary->cbegin();
                                i != val.m_data.m_value.binary->cend() - 1; ++i)
                        {
                            dump_integer(*i);
                            o->write_characters(", ", 2);
                        }
                        dump_integer(val.m_data.m_value.binary->back());
                    }

                    o->write_characters("],\n", 3);
                    o->write_characters(indent_string.c_str(), new_indent);

                    o->write_characters("\"subtype\": ", 11);
                    if (val.m_data.m_value.binary->has_subtype())
                    {
                        dump_integer(val.m_data.m_value.binary->subtype());
                    }
                    else
                    {
                        o->write_characters("null", 4);
                    }
                    o->write_character('\n');
                    o->write_characters(indent_string.c_str(), current_indent);
                    o->write_character('}');
                }
                else
                {
                    o->write_characters("{\"bytes\":[", 10);

                    if (!val.m_data.m_value.binary->empty())
                    {
                        for (auto i = val.m_data.m_value.binary->cbegin();
                                i != val.m_data.m_value.binary->cend() - 1; ++i)
                        {
                            dump_integer(*i);
                            o->write_character(',');
                        }
                        dump_integer(val.m_data.m_value.binary->back());
                    }

                    o->write_characters("],\"subtype\":", 12);
                    if (val.m_data.m_value.binary->has_subtype())
                    {
                        dump_integer(val.m_data.m_value.binary->subtype());
                        o->write_character('}');
                    }
                    else
                    {
                        o->write_characters("null}", 5);
                    }
                }
                return;
            }

            case value_t::boolean:
            {
                if (val.m_data.m_value.boolean)
                {
                    o->write_characters("true", 4);
                }
                else
                {
                    o->write_characters("false", 5);
                }
                return;
            }

            case value_t::number_integer:
            {
                dump_integer(val.m_data.m_value.number_integer);
                return;
            }

            case value_t::number_unsigned:
            {
                dump_integer(val.m_data.m_value.number_unsigned);
                return;
            }

            case value_t::number_float:
            {
                dump_float(val.m_data.m_value.number_float);
                return;
            }

            case value_t::discarded:
            {
                o->write_characters("<discarded>", 11);
                return;
            }

            case value_t::null:
            {
                o->write_characters("null", 4);
                return;
            }

            default:            // LCOV_EXCL_LINE
                JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
        }
    }

  JSON_PRIVATE_UNLESS_TESTED:
    /*!
    @brief dump escaped string

    Escape a string by replacing certain special characters by a sequence of an
    escape character (backslash) and another character and other control
    characters by a sequence of "\u" followed by a four-digit hex
    representation. The escaped string is written to output stream @a o.

    @param[in] s  the string to escape
    @param[in] ensure_ascii  whether to escape non-ASCII characters with
                             \uXXXX sequences

    @complexity Linear in the length of string @a s.
    */
    void dump_escaped(const string_t& s, const bool ensure_ascii)
    {
        std::uint32_t codepoint{};
        std::uint8_t state = UTF8_ACCEPT;
        std::size_t bytes = 0;  // number of bytes written to string_buffer

        // number of bytes written at the point of the last valid byte
        std::size_t bytes_after_last_accept = 0;
        std::size_t undumped_chars = 0;

        for (std::size_t i = 0; i < s.size(); ++i)
        {
            const auto byte = static_cast<std::uint8_t>(s[i]);

            switch (decode(state, codepoint, byte))
            {
                case UTF8_ACCEPT:  // decode found a new code point
                {
                    switch (codepoint)
                    {
                        case 0x08: // backspace
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 'b';
                            break;
                        }

                        case 0x09: // horizontal tab
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 't';
                            break;
                        }

                        case 0x0A: // newline
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 'n';
                            break;
                        }

                        case 0x0C: // formfeed
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 'f';
                            break;
                        }

                        case 0x0D: // carriage return
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 'r';
                            break;
                        }

                        case 0x22: // quotation mark
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = '\"';
                            break;
                        }

                        case 0x5C: // reverse solidus
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = '\\';
                            break;
                        }

                        default:
                        {
                            // escape control characters (0x00..0x1F) or, if
                            // ensure_ascii parameter is used, non-ASCII characters
                            if ((codepoint <= 0x1F) || (ensure_ascii && (codepoint >= 0x7F)))
                            {
                                if (codepoint <= 0xFFFF)
                                {
                                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
                                    static_cast<void>((std::snprintf)(string_buffer.data() + bytes, 7, "\\u%04x",
                                                                      static_cast<std::uint16_t>(codepoint)));
                                    bytes += 6;
                                }
                                else
                                {
                                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
                                    static_cast<void>((std::snprintf)(string_buffer.data() + bytes, 13, "\\u%04x\\u%04x",
                                                                      static_cast<std::uint16_t>(0xD7C0u + (codepoint >> 10u)),
                                                                      static_cast<std::uint16_t>(0xDC00u + (codepoint & 0x3FFu))));
                                    bytes += 12;
                                }
                            }
                            else
                            {
                                // copy byte to buffer (all previous bytes
                                // been copied have in default case above)
                                string_buffer[bytes++] = s[i];
                            }
                            break;
                        }
                    }

                    // write buffer and reset index; there must be 13 bytes
                    // left, as this is the maximal number of bytes to be
                    // written ("\uxxxx\uxxxx\0") for one code point
                    if (string_buffer.size() - bytes < 13)
                    {
                        o->write_characters(string_buffer.data(), bytes);
                        bytes = 0;
                    }

                    // remember the byte position of this accept
                    bytes_after_last_accept = bytes;
                    undumped_chars = 0;
                    break;
                }

                case UTF8_REJECT:  // decode found invalid UTF-8 byte
                {
                    switch (error_handler)
                    {
                        case error_handler_t::strict:
                        {
                            JSON_THROW(type_error::create(316, concat("invalid UTF-8 byte at index ", std::to_string(i), ": 0x", hex_bytes(byte | 0)), nullptr));
                        }

                        case error_handler_t::ignore:
                        case error_handler_t::replace:
                        {
                            // in case we saw this character the first time, we
                            // would like to read it again, because the byte
                            // may be OK for itself, but just not OK for the
                            // previous sequence
                            if (undumped_chars > 0)
                            {
                                --i;
                            }

                            // reset length buffer to the last accepted index;
                            // thus removing/ignoring the invalid characters
                            bytes = bytes_after_last_accept;

                            if (error_handler == error_handler_t::replace)
                            {
                                // add a replacement character
                                if (ensure_ascii)
                                {
                                    string_buffer[bytes++] = '\\';
                                    string_buffer[bytes++] = 'u';
                                    string_buffer[bytes++] = 'f';
                                    string_buffer[bytes++] = 'f';
                                    string_buffer[bytes++] = 'f';
                                    string_buffer[bytes++] = 'd';
                                }
                                else
                                {
                                    string_buffer[bytes++] = detail::binary_writer<BasicJsonType, char>::to_char_type('\xEF');
                                    string_buffer[bytes++] = detail::binary_writer<BasicJsonType, char>::to_char_type('\xBF');
                                    string_buffer[bytes++] = detail::binary_writer<BasicJsonType, char>::to_char_type('\xBD');
                                }

                                // write buffer and reset index; there must be 13 bytes
                                // left, as this is the maximal number of bytes to be
                                // written ("\uxxxx\uxxxx\0") for one code point
                                if (string_buffer.size() - bytes < 13)
                                {
                                    o->write_characters(string_buffer.data(), bytes);
                                    bytes = 0;
                                }

                                bytes_after_last_accept = bytes;
                            }

                            undumped_chars = 0;

                            // continue processing the string
                            state = UTF8_ACCEPT;
                            break;
                        }

                        default:            // LCOV_EXCL_LINE
                            JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
                    }
                    break;
                }

                default:  // decode found yet incomplete multi-byte code point
                {
                    if (!ensure_ascii)
                    {
                        // code point will not be escaped - copy byte to buffer
                        string_buffer[bytes++] = s[i];
                    }
                    ++undumped_chars;
                    break;
                }
            }
        }

        // we finished processing the string
        if (JSON_HEDLEY_LIKELY(state == UTF8_ACCEPT))
        {
            // write buffer
            if (bytes > 0)
            {
                o->write_characters(string_buffer.data(), bytes);
            }
        }
        else
        {
            // we finish reading, but do not accept: string was incomplete
            switch (error_handler)
            {
                case error_handler_t::strict:
                {
                    JSON_THROW(type_error::create(316, concat("incomplete UTF-8 string; last byte: 0x", hex_bytes(static_cast<std::uint8_t>(s.back() | 0))), nullptr));
                }

                case error_handler_t::ignore:
                {
                    // write all accepted bytes
                    o->write_characters(string_buffer.data(), bytes_after_last_accept);
                    break;
                }

                case error_handler_t::replace:
                {
                    // write all accepted bytes
                    o->write_characters(string_buffer.data(), bytes_after_last_accept);
                    // add a replacement character
                    if (ensure_ascii)
                    {
                        o->write_characters("\\ufffd", 6);
                    }
                    else
                    {
                        o->write_characters("\xEF\xBF\xBD", 3);
                    }
                    break;
                }

                default:            // LCOV_EXCL_LINE
                    JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
            }
        }
    }

  private:
    /*!
    @brief count digits

    Count the number of decimal (base 10) digits for an input unsigned integer.

    @param[in] x  unsigned integer number to count its digits
    @return    number of decimal digits
    */
    inline unsigned int count_digits(number_unsigned_t x) noexcept
    {
        unsigned int n_digits = 1;
        for (;;)
        {
            if (x < 10)
            {
                return n_digits;
            }
            if (x < 100)
            {
                return n_digits + 1;
            }
            if (x < 1000)
            {
                return n_digits + 2;
            }
            if (x < 10000)
            {
                return n_digits + 3;
            }
            x = x / 10000u;
            n_digits += 4;
        }
    }

    /*!
     * @brief convert a byte to a uppercase hex representation
     * @param[in] byte byte to represent
     * @return representation ("00".."FF")
     */
    static std::string hex_bytes(std::uint8_t byte)
    {
        std::string result = "FF";
        constexpr const char* nibble_to_hex = "0123456789ABCDEF";
        result[0] = nibble_to_hex[byte / 16];
        result[1] = nibble_to_hex[byte % 16];
        return result;
    }

    // templates to avoid warnings about useless casts
    template <typename NumberType, enable_if_t<std::is_signed<NumberType>::value, int> = 0>
    bool is_negative_number(NumberType x)
    {
        return x < 0;
    }

    template < typename NumberType, enable_if_t <std::is_unsigned<NumberType>::value, int > = 0 >
    bool is_negative_number(NumberType /*unused*/)
    {
        return false;
    }

    /*!
    @brief dump an integer

    Dump a given integer to output stream @a o. Works internally with
    @a number_buffer.

    @param[in] x  integer number (signed or unsigned) to dump
    @tparam NumberType either @a number_integer_t or @a number_unsigned_t
    */
    template < typename NumberType, detail::enable_if_t <
                   std::is_integral<NumberType>::value ||
                   std::is_same<NumberType, number_unsigned_t>::value ||
                   std::is_same<NumberType, number_integer_t>::value ||
                   std::is_same<NumberType, binary_char_t>::value,
                   int > = 0 >
    void dump_integer(NumberType x)
    {
        static constexpr std::array<std::array<char, 2>, 100> digits_to_99
        {
            {
                {{'0', '0'}}, {{'0', '1'}}, {{'0', '2'}}, {{'0', '3'}}, {{'0', '4'}}, {{'0', '5'}}, {{'0', '6'}}, {{'0', '7'}}, {{'0', '8'}}, {{'0', '9'}},
                {{'1', '0'}}, {{'1', '1'}}, {{'1', '2'}}, {{'1', '3'}}, {{'1', '4'}}, {{'1', '5'}}, {{'1', '6'}}, {{'1', '7'}}, {{'1', '8'}}, {{'1', '9'}},
                {{'2', '0'}}, {{'2', '1'}}, {{'2', '2'}}, {{'2', '3'}}, {{'2', '4'}}, {{'2', '5'}}, {{'2', '6'}}, {{'2', '7'}}, {{'2', '8'}}, {{'2', '9'}},
                {{'3', '0'}}, {{'3', '1'}}, {{'3', '2'}}, {{'3', '3'}}, {{'3', '4'}}, {{'3', '5'}}, {{'3', '6'}}, {{'3', '7'}}, {{'3', '8'}}, {{'3', '9'}},
                {{'4', '0'}}, {{'4', '1'}}, {{'4', '2'}}, {{'4', '3'}}, {{'4', '4'}}, {{'4', '5'}}, {{'4', '6'}}, {{'4', '7'}}, {{'4', '8'}}, {{'4', '9'}},
                {{'5', '0'}}, {{'5', '1'}}, {{'5', '2'}}, {{'5', '3'}}, {{'5', '4'}}, {{'5', '5'}}, {{'5', '6'}}, {{'5', '7'}}, {{'5', '8'}}, {{'5', '9'}},
                {{'6', '0'}}, {{'6', '1'}}, {{'6', '2'}}, {{'6', '3'}}, {{'6', '4'}}, {{'6', '5'}}, {{'6', '6'}}, {{'6', '7'}}, {{'6', '8'}}, {{'6', '9'}},
                {{'7', '0'}}, {{'7', '1'}}, {{'7', '2'}}, {{'7', '3'}}, {{'7', '4'}}, {{'7', '5'}}, {{'7', '6'}}, {{'7', '7'}}, {{'7', '8'}}, {{'7', '9'}},
                {{'8', '0'}}, {{'8', '1'}}, {{'8', '2'}}, {{'8', '3'}}, {{'8', '4'}}, {{'8', '5'}}, {{'8', '6'}}, {{'8', '7'}}, {{'8', '8'}}, {{'8', '9'}},
                {{'9', '0'}}, {{'9', '1'}}, {{'9', '2'}}, {{'9', '3'}}, {{'9', '4'}}, {{'9', '5'}}, {{'9', '6'}}, {{'9', '7'}}, {{'9', '8'}}, {{'9', '9'}},
            }
        };

        // special case for "0"
        if (x == 0)
        {
            o->write_character('0');
            return;
        }

        // use a pointer to fill the buffer
        auto buffer_ptr = number_buffer.begin(); // NOLINT(llvm-qualified-auto,readability-qualified-auto,cppcoreguidelines-pro-type-vararg,hicpp-vararg)

        number_unsigned_t abs_value;

        unsigned int n_chars{};

        if (is_negative_number(x))
        {
            *buffer_ptr = '-';
            abs_value = remove_sign(static_cast<number_integer_t>(x));

            // account one more byte for the minus sign
            n_chars = 1 + count_digits(abs_value);
        }
        else
        {
            abs_value = static_cast<number_unsigned_t>(x);
            n_chars = count_digits(abs_value);
        }

        // spare 1 byte for '\0'
        JSON_ASSERT(n_chars < number_buffer.size() - 1);

        // jump to the end to generate the string from backward,
        // so we later avoid reversing the result
        buffer_ptr += n_chars;

        // Fast int2ascii implementation inspired by "Fastware" talk by Andrei Alexandrescu
        // See: https://www.youtube.com/watch?v=o4-CwDo2zpg
        while (abs_value >= 100)
        {
            const auto digits_index = static_cast<unsigned>((abs_value % 100));
            abs_value /= 100;
            *(--buffer_ptr) = digits_to_99[digits_index][1];
            *(--buffer_ptr) = digits_to_99[digits_index][0];
        }

        if (abs_value >= 10)
        {
            const auto digits_index = static_cast<unsigned>(abs_value);
            *(--buffer_ptr) = digits_to_99[digits_index][1];
            *(--buffer_ptr) = digits_to_99[digits_index][0];
        }
        else
        {
            *(--buffer_ptr) = static_cast<char>('0' + abs_value);
        }

        o->write_characters(number_buffer.data(), n_chars);
    }

    /*!
    @brief dump a floating-point number

    Dump a given floating-point number to output stream @a o. Works internally
    with @a number_buffer.

    @param[in] x  floating-point number to dump
    */
    void dump_float(number_float_t x)
    {
        // NaN / inf
        if (!std::isfinite(x))
        {
            o->write_characters("null", 4);
            return;
        }

        // If number_float_t is an IEEE-754 single or double precision number,
        // use the Grisu2 algorithm to produce short numbers which are
        // guaranteed to round-trip, using strtof and strtod, resp.
        //
        // NB: The test below works if <long double> == <double>.
        static constexpr bool is_ieee_single_or_double
            = (std::numeric_limits<number_float_t>::is_iec559 && std::numeric_limits<number_float_t>::digits == 24 && std::numeric_limits<number_float_t>::max_exponent == 128) ||
              (std::numeric_limits<number_float_t>::is_iec559 && std::numeric_limits<number_float_t>::digits == 53 && std::numeric_limits<number_float_t>::max_exponent == 1024);

        dump_float(x, std::integral_constant<bool, is_ieee_single_or_double>());
    }

    void dump_float(number_float_t x, std::true_type /*is_ieee_single_or_double*/)
    {
        auto* begin = number_buffer.data();
        auto* end = ::nlohmann::detail::to_chars(begin, begin + number_buffer.size(), x);

        o->write_characters(begin, static_cast<size_t>(end - begin));
    }

    void dump_float(number_float_t x, std::false_type /*is_ieee_single_or_double*/)
    {
        // get number of digits for a float -> text -> float round-trip
        static constexpr auto d = std::numeric_limits<number_float_t>::max_digits10;

        // the actual conversion
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
        std::ptrdiff_t len = (std::snprintf)(number_buffer.data(), number_buffer.size(), "%.*g", d, x);

        // negative value indicates an error
        JSON_ASSERT(len > 0);
        // check if buffer was large enough
        JSON_ASSERT(static_cast<std::size_t>(len) < number_buffer.size());

        // erase thousands separator
        if (thousands_sep != '\0')
        {
            // NOLINTNEXTLINE(readability-qualified-auto,llvm-qualified-auto): std::remove returns an iterator, see https://github.com/nlohmann/json/issues/3081
            const auto end = std::remove(number_buffer.begin(), number_buffer.begin() + len, thousands_sep);
            std::fill(end, number_buffer.end(), '\0');
            JSON_ASSERT((end - number_buffer.begin()) <= len);
            len = (end - number_buffer.begin());
        }

        // convert decimal point to '.'
        if (decimal_point != '\0' && decimal_point != '.')
        {
            // NOLINTNEXTLINE(readability-qualified-auto,llvm-qualified-auto): std::find returns an iterator, see https://github.com/nlohmann/json/issues/3081
            const auto dec_pos = std::find(number_buffer.begin(), number_buffer.end(), decimal_point);
            if (dec_pos != number_buffer.end())
            {
                *dec_pos = '.';
            }
        }

        o->write_characters(number_buffer.data(), static_cast<std::size_t>(len));

        // determine if we need to append ".0"
        const bool value_is_int_like =
            std::none_of(number_buffer.begin(), number_buffer.begin() + len + 1,
                         [](char c)
        {
            return c == '.' || c == 'e';
        });

        if (value_is_int_like)
        {
            o->write_characters(".0", 2);
        }
    }

    /*!
    @brief check whether a string is UTF-8 encoded

    The function checks each byte of a string whether it is UTF-8 encoded. The
    result of the check is stored in the @a state parameter. The function must
    be called initially with state 0 (accept). State 1 means the string must
    be rejected, because the current byte is not allowed. If the string is
    completely processed, but the state is non-zero, the string ended
    prematurely; that is, the last byte indicated more bytes should have
    followed.

    @param[in,out] state  the state of the decoding
    @param[in,out] codep  codepoint (valid only if resulting state is UTF8_ACCEPT)
    @param[in] byte       next byte to decode
    @return               new state

    @note The function has been edited: a std::array is used.

    @copyright Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
    @sa http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
    */
    static std::uint8_t decode(std::uint8_t& state, std::uint32_t& codep, const std::uint8_t byte) noexcept
    {
        static const std::array<std::uint8_t, 400> utf8d =
        {
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 00..1F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20..3F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40..5F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 60..7F
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // 80..9F
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // A0..BF
                8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0..DF
                0xA, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, // E0..EF
                0xB, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, // F0..FF
                0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, // s1..s2
                1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, // s3..s4
                1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, // s5..s6
                1, 3, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 // s7..s8
            }
        };

        JSON_ASSERT(byte < utf8d.size());
        const std::uint8_t type = utf8d[byte];

        codep = (state != UTF8_ACCEPT)
                ? (byte & 0x3fu) | (codep << 6u)
                : (0xFFu >> type) & (byte);

        const std::size_t index = 256u + static_cast<size_t>(state) * 16u + static_cast<size_t>(type);
        JSON_ASSERT(index < utf8d.size());
        state = utf8d[index];
        return state;
    }

    /*
     * Overload to make the compiler happy while it is instantiating
     * dump_integer for number_unsigned_t.
     * Must never be called.
     */
    number_unsigned_t remove_sign(number_unsigned_t x)
    {
        JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
        return x; // LCOV_EXCL_LINE
    }

    /*
     * Helper function for dump_integer
     *
     * This function takes a negative signed integer and returns its absolute
     * value as unsigned integer. The plus/minus shuffling is necessary as we can
     * not directly remove the sign of an arbitrary signed integer as the
     * absolute values of INT_MIN and INT_MAX are usually not the same. See
     * #1708 for details.
     */
    inline number_unsigned_t remove_sign(number_integer_t x) noexcept
    {
        JSON_ASSERT(x < 0 && x < (std::numeric_limits<number_integer_t>::max)()); // NOLINT(misc-redundant-expression)
        return static_cast<number_unsigned_t>(-(x + 1)) + 1;
    }

  private:
    /// the output of the serializer
    output_adapter_t<char> o = nullptr;

    /// a (hopefully) large enough character buffer
    std::array<char, 64> number_buffer{{}};

    /// the locale
    const std::lconv* loc = nullptr;
    /// the locale's thousand separator character
    const char thousands_sep = '\0';
    /// the locale's decimal point character
    const char decimal_point = '\0';

    /// string buffer
    std::array<char, 512> string_buffer{{}};

    /// the indentation character
    const char indent_char;
    /// the indentation string
    string_t indent_string;

    /// error_handler how to react on decoding errors
    const error_handler_t error_handler;
};

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END
