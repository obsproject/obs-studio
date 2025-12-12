//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cmath> // isfinite
#include <cstdint> // uint8_t
#include <functional> // function
#include <string> // string
#include <utility> // move
#include <vector> // vector

#include <nlohmann/detail/exceptions.hpp>
#include <nlohmann/detail/input/input_adapters.hpp>
#include <nlohmann/detail/input/json_sax.hpp>
#include <nlohmann/detail/input/lexer.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/detail/meta/is_sax.hpp>
#include <nlohmann/detail/string_concat.hpp>
#include <nlohmann/detail/value_t.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{
////////////
// parser //
////////////

enum class parse_event_t : std::uint8_t
{
    /// the parser read `{` and started to process a JSON object
    object_start,
    /// the parser read `}` and finished processing a JSON object
    object_end,
    /// the parser read `[` and started to process a JSON array
    array_start,
    /// the parser read `]` and finished processing a JSON array
    array_end,
    /// the parser read a key of a value in an object
    key,
    /// the parser finished reading a JSON value
    value
};

template<typename BasicJsonType>
using parser_callback_t =
    std::function<bool(int /*depth*/, parse_event_t /*event*/, BasicJsonType& /*parsed*/)>;

/*!
@brief syntax analysis

This class implements a recursive descent parser.
*/
template<typename BasicJsonType, typename InputAdapterType>
class parser
{
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;
    using string_t = typename BasicJsonType::string_t;
    using lexer_t = lexer<BasicJsonType, InputAdapterType>;
    using token_type = typename lexer_t::token_type;

  public:
    /// a parser reading from an input adapter
    explicit parser(InputAdapterType&& adapter,
                    const parser_callback_t<BasicJsonType> cb = nullptr,
                    const bool allow_exceptions_ = true,
                    const bool skip_comments = false)
        : callback(cb)
        , m_lexer(std::move(adapter), skip_comments)
        , allow_exceptions(allow_exceptions_)
    {
        // read first token
        get_token();
    }

    /*!
    @brief public parser interface

    @param[in] strict      whether to expect the last token to be EOF
    @param[in,out] result  parsed JSON value

    @throw parse_error.101 in case of an unexpected token
    @throw parse_error.102 if to_unicode fails or surrogate error
    @throw parse_error.103 if to_unicode fails
    */
    void parse(const bool strict, BasicJsonType& result)
    {
        if (callback)
        {
            json_sax_dom_callback_parser<BasicJsonType> sdp(result, callback, allow_exceptions);
            sax_parse_internal(&sdp);

            // in strict mode, input must be completely read
            if (strict && (get_token() != token_type::end_of_input))
            {
                sdp.parse_error(m_lexer.get_position(),
                                m_lexer.get_token_string(),
                                parse_error::create(101, m_lexer.get_position(),
                                                    exception_message(token_type::end_of_input, "value"), nullptr));
            }

            // in case of an error, return discarded value
            if (sdp.is_errored())
            {
                result = value_t::discarded;
                return;
            }

            // set top-level value to null if it was discarded by the callback
            // function
            if (result.is_discarded())
            {
                result = nullptr;
            }
        }
        else
        {
            json_sax_dom_parser<BasicJsonType> sdp(result, allow_exceptions);
            sax_parse_internal(&sdp);

            // in strict mode, input must be completely read
            if (strict && (get_token() != token_type::end_of_input))
            {
                sdp.parse_error(m_lexer.get_position(),
                                m_lexer.get_token_string(),
                                parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_of_input, "value"), nullptr));
            }

            // in case of an error, return discarded value
            if (sdp.is_errored())
            {
                result = value_t::discarded;
                return;
            }
        }

        result.assert_invariant();
    }

    /*!
    @brief public accept interface

    @param[in] strict  whether to expect the last token to be EOF
    @return whether the input is a proper JSON text
    */
    bool accept(const bool strict = true)
    {
        json_sax_acceptor<BasicJsonType> sax_acceptor;
        return sax_parse(&sax_acceptor, strict);
    }

    template<typename SAX>
    JSON_HEDLEY_NON_NULL(2)
    bool sax_parse(SAX* sax, const bool strict = true)
    {
        (void)detail::is_sax_static_asserts<SAX, BasicJsonType> {};
        const bool result = sax_parse_internal(sax);

        // strict mode: next byte must be EOF
        if (result && strict && (get_token() != token_type::end_of_input))
        {
            return sax->parse_error(m_lexer.get_position(),
                                    m_lexer.get_token_string(),
                                    parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_of_input, "value"), nullptr));
        }

        return result;
    }

  private:
    template<typename SAX>
    JSON_HEDLEY_NON_NULL(2)
    bool sax_parse_internal(SAX* sax)
    {
        // stack to remember the hierarchy of structured values we are parsing
        // true = array; false = object
        std::vector<bool> states;
        // value to avoid a goto (see comment where set to true)
        bool skip_to_state_evaluation = false;

        while (true)
        {
            if (!skip_to_state_evaluation)
            {
                // invariant: get_token() was called before each iteration
                switch (last_token)
                {
                    case token_type::begin_object:
                    {
                        if (JSON_HEDLEY_UNLIKELY(!sax->start_object(static_cast<std::size_t>(-1))))
                        {
                            return false;
                        }

                        // closing } -> we are done
                        if (get_token() == token_type::end_object)
                        {
                            if (JSON_HEDLEY_UNLIKELY(!sax->end_object()))
                            {
                                return false;
                            }
                            break;
                        }

                        // parse key
                        if (JSON_HEDLEY_UNLIKELY(last_token != token_type::value_string))
                        {
                            return sax->parse_error(m_lexer.get_position(),
                                                    m_lexer.get_token_string(),
                                                    parse_error::create(101, m_lexer.get_position(), exception_message(token_type::value_string, "object key"), nullptr));
                        }
                        if (JSON_HEDLEY_UNLIKELY(!sax->key(m_lexer.get_string())))
                        {
                            return false;
                        }

                        // parse separator (:)
                        if (JSON_HEDLEY_UNLIKELY(get_token() != token_type::name_separator))
                        {
                            return sax->parse_error(m_lexer.get_position(),
                                                    m_lexer.get_token_string(),
                                                    parse_error::create(101, m_lexer.get_position(), exception_message(token_type::name_separator, "object separator"), nullptr));
                        }

                        // remember we are now inside an object
                        states.push_back(false);

                        // parse values
                        get_token();
                        continue;
                    }

                    case token_type::begin_array:
                    {
                        if (JSON_HEDLEY_UNLIKELY(!sax->start_array(static_cast<std::size_t>(-1))))
                        {
                            return false;
                        }

                        // closing ] -> we are done
                        if (get_token() == token_type::end_array)
                        {
                            if (JSON_HEDLEY_UNLIKELY(!sax->end_array()))
                            {
                                return false;
                            }
                            break;
                        }

                        // remember we are now inside an array
                        states.push_back(true);

                        // parse values (no need to call get_token)
                        continue;
                    }

                    case token_type::value_float:
                    {
                        const auto res = m_lexer.get_number_float();

                        if (JSON_HEDLEY_UNLIKELY(!std::isfinite(res)))
                        {
                            return sax->parse_error(m_lexer.get_position(),
                                                    m_lexer.get_token_string(),
                                                    out_of_range::create(406, concat("number overflow parsing '", m_lexer.get_token_string(), '\''), nullptr));
                        }

                        if (JSON_HEDLEY_UNLIKELY(!sax->number_float(res, m_lexer.get_string())))
                        {
                            return false;
                        }

                        break;
                    }

                    case token_type::literal_false:
                    {
                        if (JSON_HEDLEY_UNLIKELY(!sax->boolean(false)))
                        {
                            return false;
                        }
                        break;
                    }

                    case token_type::literal_null:
                    {
                        if (JSON_HEDLEY_UNLIKELY(!sax->null()))
                        {
                            return false;
                        }
                        break;
                    }

                    case token_type::literal_true:
                    {
                        if (JSON_HEDLEY_UNLIKELY(!sax->boolean(true)))
                        {
                            return false;
                        }
                        break;
                    }

                    case token_type::value_integer:
                    {
                        if (JSON_HEDLEY_UNLIKELY(!sax->number_integer(m_lexer.get_number_integer())))
                        {
                            return false;
                        }
                        break;
                    }

                    case token_type::value_string:
                    {
                        if (JSON_HEDLEY_UNLIKELY(!sax->string(m_lexer.get_string())))
                        {
                            return false;
                        }
                        break;
                    }

                    case token_type::value_unsigned:
                    {
                        if (JSON_HEDLEY_UNLIKELY(!sax->number_unsigned(m_lexer.get_number_unsigned())))
                        {
                            return false;
                        }
                        break;
                    }

                    case token_type::parse_error:
                    {
                        // using "uninitialized" to avoid "expected" message
                        return sax->parse_error(m_lexer.get_position(),
                                                m_lexer.get_token_string(),
                                                parse_error::create(101, m_lexer.get_position(), exception_message(token_type::uninitialized, "value"), nullptr));
                    }
                    case token_type::end_of_input:
                    {
                        if (JSON_HEDLEY_UNLIKELY(m_lexer.get_position().chars_read_total == 1))
                        {
                            return sax->parse_error(m_lexer.get_position(),
                                                    m_lexer.get_token_string(),
                                                    parse_error::create(101, m_lexer.get_position(),
                                                            "attempting to parse an empty input; check that your input string or stream contains the expected JSON", nullptr));
                        }

                        return sax->parse_error(m_lexer.get_position(),
                                                m_lexer.get_token_string(),
                                                parse_error::create(101, m_lexer.get_position(), exception_message(token_type::literal_or_value, "value"), nullptr));
                    }
                    case token_type::uninitialized:
                    case token_type::end_array:
                    case token_type::end_object:
                    case token_type::name_separator:
                    case token_type::value_separator:
                    case token_type::literal_or_value:
                    default: // the last token was unexpected
                    {
                        return sax->parse_error(m_lexer.get_position(),
                                                m_lexer.get_token_string(),
                                                parse_error::create(101, m_lexer.get_position(), exception_message(token_type::literal_or_value, "value"), nullptr));
                    }
                }
            }
            else
            {
                skip_to_state_evaluation = false;
            }

            // we reached this line after we successfully parsed a value
            if (states.empty())
            {
                // empty stack: we reached the end of the hierarchy: done
                return true;
            }

            if (states.back())  // array
            {
                // comma -> next value
                if (get_token() == token_type::value_separator)
                {
                    // parse a new value
                    get_token();
                    continue;
                }

                // closing ]
                if (JSON_HEDLEY_LIKELY(last_token == token_type::end_array))
                {
                    if (JSON_HEDLEY_UNLIKELY(!sax->end_array()))
                    {
                        return false;
                    }

                    // We are done with this array. Before we can parse a
                    // new value, we need to evaluate the new state first.
                    // By setting skip_to_state_evaluation to false, we
                    // are effectively jumping to the beginning of this if.
                    JSON_ASSERT(!states.empty());
                    states.pop_back();
                    skip_to_state_evaluation = true;
                    continue;
                }

                return sax->parse_error(m_lexer.get_position(),
                                        m_lexer.get_token_string(),
                                        parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_array, "array"), nullptr));
            }

            // states.back() is false -> object

            // comma -> next value
            if (get_token() == token_type::value_separator)
            {
                // parse key
                if (JSON_HEDLEY_UNLIKELY(get_token() != token_type::value_string))
                {
                    return sax->parse_error(m_lexer.get_position(),
                                            m_lexer.get_token_string(),
                                            parse_error::create(101, m_lexer.get_position(), exception_message(token_type::value_string, "object key"), nullptr));
                }

                if (JSON_HEDLEY_UNLIKELY(!sax->key(m_lexer.get_string())))
                {
                    return false;
                }

                // parse separator (:)
                if (JSON_HEDLEY_UNLIKELY(get_token() != token_type::name_separator))
                {
                    return sax->parse_error(m_lexer.get_position(),
                                            m_lexer.get_token_string(),
                                            parse_error::create(101, m_lexer.get_position(), exception_message(token_type::name_separator, "object separator"), nullptr));
                }

                // parse values
                get_token();
                continue;
            }

            // closing }
            if (JSON_HEDLEY_LIKELY(last_token == token_type::end_object))
            {
                if (JSON_HEDLEY_UNLIKELY(!sax->end_object()))
                {
                    return false;
                }

                // We are done with this object. Before we can parse a
                // new value, we need to evaluate the new state first.
                // By setting skip_to_state_evaluation to false, we
                // are effectively jumping to the beginning of this if.
                JSON_ASSERT(!states.empty());
                states.pop_back();
                skip_to_state_evaluation = true;
                continue;
            }

            return sax->parse_error(m_lexer.get_position(),
                                    m_lexer.get_token_string(),
                                    parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_object, "object"), nullptr));
        }
    }

    /// get next token from lexer
    token_type get_token()
    {
        return last_token = m_lexer.scan();
    }

    std::string exception_message(const token_type expected, const std::string& context)
    {
        std::string error_msg = "syntax error ";

        if (!context.empty())
        {
            error_msg += concat("while parsing ", context, ' ');
        }

        error_msg += "- ";

        if (last_token == token_type::parse_error)
        {
            error_msg += concat(m_lexer.get_error_message(), "; last read: '",
                                m_lexer.get_token_string(), '\'');
        }
        else
        {
            error_msg += concat("unexpected ", lexer_t::token_type_name(last_token));
        }

        if (expected != token_type::uninitialized)
        {
            error_msg += concat("; expected ", lexer_t::token_type_name(expected));
        }

        return error_msg;
    }

  private:
    /// callback function
    const parser_callback_t<BasicJsonType> callback = nullptr;
    /// the type of the last read token
    token_type last_token = token_type::uninitialized;
    /// the lexer
    lexer_t m_lexer;
    /// whether to throw exceptions in case of errors
    const bool allow_exceptions = true;
};

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END
