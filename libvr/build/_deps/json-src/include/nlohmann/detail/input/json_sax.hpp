//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <string> // string
#include <utility> // move
#include <vector> // vector

#include <nlohmann/detail/exceptions.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/detail/string_concat.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN

/*!
@brief SAX interface

This class describes the SAX interface used by @ref nlohmann::json::sax_parse.
Each function is called in different situations while the input is parsed. The
boolean return value informs the parser whether to continue processing the
input.
*/
template<typename BasicJsonType>
struct json_sax
{
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;
    using string_t = typename BasicJsonType::string_t;
    using binary_t = typename BasicJsonType::binary_t;

    /*!
    @brief a null value was read
    @return whether parsing should proceed
    */
    virtual bool null() = 0;

    /*!
    @brief a boolean value was read
    @param[in] val  boolean value
    @return whether parsing should proceed
    */
    virtual bool boolean(bool val) = 0;

    /*!
    @brief an integer number was read
    @param[in] val  integer value
    @return whether parsing should proceed
    */
    virtual bool number_integer(number_integer_t val) = 0;

    /*!
    @brief an unsigned integer number was read
    @param[in] val  unsigned integer value
    @return whether parsing should proceed
    */
    virtual bool number_unsigned(number_unsigned_t val) = 0;

    /*!
    @brief a floating-point number was read
    @param[in] val  floating-point value
    @param[in] s    raw token value
    @return whether parsing should proceed
    */
    virtual bool number_float(number_float_t val, const string_t& s) = 0;

    /*!
    @brief a string value was read
    @param[in] val  string value
    @return whether parsing should proceed
    @note It is safe to move the passed string value.
    */
    virtual bool string(string_t& val) = 0;

    /*!
    @brief a binary value was read
    @param[in] val  binary value
    @return whether parsing should proceed
    @note It is safe to move the passed binary value.
    */
    virtual bool binary(binary_t& val) = 0;

    /*!
    @brief the beginning of an object was read
    @param[in] elements  number of object elements or -1 if unknown
    @return whether parsing should proceed
    @note binary formats may report the number of elements
    */
    virtual bool start_object(std::size_t elements) = 0;

    /*!
    @brief an object key was read
    @param[in] val  object key
    @return whether parsing should proceed
    @note It is safe to move the passed string.
    */
    virtual bool key(string_t& val) = 0;

    /*!
    @brief the end of an object was read
    @return whether parsing should proceed
    */
    virtual bool end_object() = 0;

    /*!
    @brief the beginning of an array was read
    @param[in] elements  number of array elements or -1 if unknown
    @return whether parsing should proceed
    @note binary formats may report the number of elements
    */
    virtual bool start_array(std::size_t elements) = 0;

    /*!
    @brief the end of an array was read
    @return whether parsing should proceed
    */
    virtual bool end_array() = 0;

    /*!
    @brief a parse error occurred
    @param[in] position    the position in the input where the error occurs
    @param[in] last_token  the last read token
    @param[in] ex          an exception object describing the error
    @return whether parsing should proceed (must return false)
    */
    virtual bool parse_error(std::size_t position,
                             const std::string& last_token,
                             const detail::exception& ex) = 0;

    json_sax() = default;
    json_sax(const json_sax&) = default;
    json_sax(json_sax&&) noexcept = default;
    json_sax& operator=(const json_sax&) = default;
    json_sax& operator=(json_sax&&) noexcept = default;
    virtual ~json_sax() = default;
};

namespace detail
{
/*!
@brief SAX implementation to create a JSON value from SAX events

This class implements the @ref json_sax interface and processes the SAX events
to create a JSON value which makes it basically a DOM parser. The structure or
hierarchy of the JSON value is managed by the stack `ref_stack` which contains
a pointer to the respective array or object for each recursion depth.

After successful parsing, the value that is passed by reference to the
constructor contains the parsed value.

@tparam BasicJsonType  the JSON type
*/
template<typename BasicJsonType>
class json_sax_dom_parser
{
  public:
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;
    using string_t = typename BasicJsonType::string_t;
    using binary_t = typename BasicJsonType::binary_t;

    /*!
    @param[in,out] r  reference to a JSON value that is manipulated while
                       parsing
    @param[in] allow_exceptions_  whether parse errors yield exceptions
    */
    explicit json_sax_dom_parser(BasicJsonType& r, const bool allow_exceptions_ = true)
        : root(r), allow_exceptions(allow_exceptions_)
    {}

    // make class move-only
    json_sax_dom_parser(const json_sax_dom_parser&) = delete;
    json_sax_dom_parser(json_sax_dom_parser&&) = default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor)
    json_sax_dom_parser& operator=(const json_sax_dom_parser&) = delete;
    json_sax_dom_parser& operator=(json_sax_dom_parser&&) = default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor)
    ~json_sax_dom_parser() = default;

    bool null()
    {
        handle_value(nullptr);
        return true;
    }

    bool boolean(bool val)
    {
        handle_value(val);
        return true;
    }

    bool number_integer(number_integer_t val)
    {
        handle_value(val);
        return true;
    }

    bool number_unsigned(number_unsigned_t val)
    {
        handle_value(val);
        return true;
    }

    bool number_float(number_float_t val, const string_t& /*unused*/)
    {
        handle_value(val);
        return true;
    }

    bool string(string_t& val)
    {
        handle_value(val);
        return true;
    }

    bool binary(binary_t& val)
    {
        handle_value(std::move(val));
        return true;
    }

    bool start_object(std::size_t len)
    {
        ref_stack.push_back(handle_value(BasicJsonType::value_t::object));

        if (JSON_HEDLEY_UNLIKELY(len != static_cast<std::size_t>(-1) && len > ref_stack.back()->max_size()))
        {
            JSON_THROW(out_of_range::create(408, concat("excessive object size: ", std::to_string(len)), ref_stack.back()));
        }

        return true;
    }

    bool key(string_t& val)
    {
        JSON_ASSERT(!ref_stack.empty());
        JSON_ASSERT(ref_stack.back()->is_object());

        // add null at given key and store the reference for later
        object_element = &(ref_stack.back()->m_data.m_value.object->operator[](val));
        return true;
    }

    bool end_object()
    {
        JSON_ASSERT(!ref_stack.empty());
        JSON_ASSERT(ref_stack.back()->is_object());

        ref_stack.back()->set_parents();
        ref_stack.pop_back();
        return true;
    }

    bool start_array(std::size_t len)
    {
        ref_stack.push_back(handle_value(BasicJsonType::value_t::array));

        if (JSON_HEDLEY_UNLIKELY(len != static_cast<std::size_t>(-1) && len > ref_stack.back()->max_size()))
        {
            JSON_THROW(out_of_range::create(408, concat("excessive array size: ", std::to_string(len)), ref_stack.back()));
        }

        return true;
    }

    bool end_array()
    {
        JSON_ASSERT(!ref_stack.empty());
        JSON_ASSERT(ref_stack.back()->is_array());

        ref_stack.back()->set_parents();
        ref_stack.pop_back();
        return true;
    }

    template<class Exception>
    bool parse_error(std::size_t /*unused*/, const std::string& /*unused*/,
                     const Exception& ex)
    {
        errored = true;
        static_cast<void>(ex);
        if (allow_exceptions)
        {
            JSON_THROW(ex);
        }
        return false;
    }

    constexpr bool is_errored() const
    {
        return errored;
    }

  private:
    /*!
    @invariant If the ref stack is empty, then the passed value will be the new
               root.
    @invariant If the ref stack contains a value, then it is an array or an
               object to which we can add elements
    */
    template<typename Value>
    JSON_HEDLEY_RETURNS_NON_NULL
    BasicJsonType* handle_value(Value&& v)
    {
        if (ref_stack.empty())
        {
            root = BasicJsonType(std::forward<Value>(v));
            return &root;
        }

        JSON_ASSERT(ref_stack.back()->is_array() || ref_stack.back()->is_object());

        if (ref_stack.back()->is_array())
        {
            ref_stack.back()->m_data.m_value.array->emplace_back(std::forward<Value>(v));
            return &(ref_stack.back()->m_data.m_value.array->back());
        }

        JSON_ASSERT(ref_stack.back()->is_object());
        JSON_ASSERT(object_element);
        *object_element = BasicJsonType(std::forward<Value>(v));
        return object_element;
    }

    /// the parsed JSON value
    BasicJsonType& root;
    /// stack to model hierarchy of values
    std::vector<BasicJsonType*> ref_stack {};
    /// helper to hold the reference for the next object element
    BasicJsonType* object_element = nullptr;
    /// whether a syntax error occurred
    bool errored = false;
    /// whether to throw exceptions in case of errors
    const bool allow_exceptions = true;
};

template<typename BasicJsonType>
class json_sax_dom_callback_parser
{
  public:
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;
    using string_t = typename BasicJsonType::string_t;
    using binary_t = typename BasicJsonType::binary_t;
    using parser_callback_t = typename BasicJsonType::parser_callback_t;
    using parse_event_t = typename BasicJsonType::parse_event_t;

    json_sax_dom_callback_parser(BasicJsonType& r,
                                 const parser_callback_t cb,
                                 const bool allow_exceptions_ = true)
        : root(r), callback(cb), allow_exceptions(allow_exceptions_)
    {
        keep_stack.push_back(true);
    }

    // make class move-only
    json_sax_dom_callback_parser(const json_sax_dom_callback_parser&) = delete;
    json_sax_dom_callback_parser(json_sax_dom_callback_parser&&) = default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor)
    json_sax_dom_callback_parser& operator=(const json_sax_dom_callback_parser&) = delete;
    json_sax_dom_callback_parser& operator=(json_sax_dom_callback_parser&&) = default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor)
    ~json_sax_dom_callback_parser() = default;

    bool null()
    {
        handle_value(nullptr);
        return true;
    }

    bool boolean(bool val)
    {
        handle_value(val);
        return true;
    }

    bool number_integer(number_integer_t val)
    {
        handle_value(val);
        return true;
    }

    bool number_unsigned(number_unsigned_t val)
    {
        handle_value(val);
        return true;
    }

    bool number_float(number_float_t val, const string_t& /*unused*/)
    {
        handle_value(val);
        return true;
    }

    bool string(string_t& val)
    {
        handle_value(val);
        return true;
    }

    bool binary(binary_t& val)
    {
        handle_value(std::move(val));
        return true;
    }

    bool start_object(std::size_t len)
    {
        // check callback for object start
        const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::object_start, discarded);
        keep_stack.push_back(keep);

        auto val = handle_value(BasicJsonType::value_t::object, true);
        ref_stack.push_back(val.second);

        // check object limit
        if (ref_stack.back() && JSON_HEDLEY_UNLIKELY(len != static_cast<std::size_t>(-1) && len > ref_stack.back()->max_size()))
        {
            JSON_THROW(out_of_range::create(408, concat("excessive object size: ", std::to_string(len)), ref_stack.back()));
        }

        return true;
    }

    bool key(string_t& val)
    {
        BasicJsonType k = BasicJsonType(val);

        // check callback for key
        const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::key, k);
        key_keep_stack.push_back(keep);

        // add discarded value at given key and store the reference for later
        if (keep && ref_stack.back())
        {
            object_element = &(ref_stack.back()->m_data.m_value.object->operator[](val) = discarded);
        }

        return true;
    }

    bool end_object()
    {
        if (ref_stack.back())
        {
            if (!callback(static_cast<int>(ref_stack.size()) - 1, parse_event_t::object_end, *ref_stack.back()))
            {
                // discard object
                *ref_stack.back() = discarded;
            }
            else
            {
                ref_stack.back()->set_parents();
            }
        }

        JSON_ASSERT(!ref_stack.empty());
        JSON_ASSERT(!keep_stack.empty());
        ref_stack.pop_back();
        keep_stack.pop_back();

        if (!ref_stack.empty() && ref_stack.back() && ref_stack.back()->is_structured())
        {
            // remove discarded value
            for (auto it = ref_stack.back()->begin(); it != ref_stack.back()->end(); ++it)
            {
                if (it->is_discarded())
                {
                    ref_stack.back()->erase(it);
                    break;
                }
            }
        }

        return true;
    }

    bool start_array(std::size_t len)
    {
        const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::array_start, discarded);
        keep_stack.push_back(keep);

        auto val = handle_value(BasicJsonType::value_t::array, true);
        ref_stack.push_back(val.second);

        // check array limit
        if (ref_stack.back() && JSON_HEDLEY_UNLIKELY(len != static_cast<std::size_t>(-1) && len > ref_stack.back()->max_size()))
        {
            JSON_THROW(out_of_range::create(408, concat("excessive array size: ", std::to_string(len)), ref_stack.back()));
        }

        return true;
    }

    bool end_array()
    {
        bool keep = true;

        if (ref_stack.back())
        {
            keep = callback(static_cast<int>(ref_stack.size()) - 1, parse_event_t::array_end, *ref_stack.back());
            if (keep)
            {
                ref_stack.back()->set_parents();
            }
            else
            {
                // discard array
                *ref_stack.back() = discarded;
            }
        }

        JSON_ASSERT(!ref_stack.empty());
        JSON_ASSERT(!keep_stack.empty());
        ref_stack.pop_back();
        keep_stack.pop_back();

        // remove discarded value
        if (!keep && !ref_stack.empty() && ref_stack.back()->is_array())
        {
            ref_stack.back()->m_data.m_value.array->pop_back();
        }

        return true;
    }

    template<class Exception>
    bool parse_error(std::size_t /*unused*/, const std::string& /*unused*/,
                     const Exception& ex)
    {
        errored = true;
        static_cast<void>(ex);
        if (allow_exceptions)
        {
            JSON_THROW(ex);
        }
        return false;
    }

    constexpr bool is_errored() const
    {
        return errored;
    }

  private:
    /*!
    @param[in] v  value to add to the JSON value we build during parsing
    @param[in] skip_callback  whether we should skip calling the callback
               function; this is required after start_array() and
               start_object() SAX events, because otherwise we would call the
               callback function with an empty array or object, respectively.

    @invariant If the ref stack is empty, then the passed value will be the new
               root.
    @invariant If the ref stack contains a value, then it is an array or an
               object to which we can add elements

    @return pair of boolean (whether value should be kept) and pointer (to the
            passed value in the ref_stack hierarchy; nullptr if not kept)
    */
    template<typename Value>
    std::pair<bool, BasicJsonType*> handle_value(Value&& v, const bool skip_callback = false)
    {
        JSON_ASSERT(!keep_stack.empty());

        // do not handle this value if we know it would be added to a discarded
        // container
        if (!keep_stack.back())
        {
            return {false, nullptr};
        }

        // create value
        auto value = BasicJsonType(std::forward<Value>(v));

        // check callback
        const bool keep = skip_callback || callback(static_cast<int>(ref_stack.size()), parse_event_t::value, value);

        // do not handle this value if we just learnt it shall be discarded
        if (!keep)
        {
            return {false, nullptr};
        }

        if (ref_stack.empty())
        {
            root = std::move(value);
            return {true, & root};
        }

        // skip this value if we already decided to skip the parent
        // (https://github.com/nlohmann/json/issues/971#issuecomment-413678360)
        if (!ref_stack.back())
        {
            return {false, nullptr};
        }

        // we now only expect arrays and objects
        JSON_ASSERT(ref_stack.back()->is_array() || ref_stack.back()->is_object());

        // array
        if (ref_stack.back()->is_array())
        {
            ref_stack.back()->m_data.m_value.array->emplace_back(std::move(value));
            return {true, & (ref_stack.back()->m_data.m_value.array->back())};
        }

        // object
        JSON_ASSERT(ref_stack.back()->is_object());
        // check if we should store an element for the current key
        JSON_ASSERT(!key_keep_stack.empty());
        const bool store_element = key_keep_stack.back();
        key_keep_stack.pop_back();

        if (!store_element)
        {
            return {false, nullptr};
        }

        JSON_ASSERT(object_element);
        *object_element = std::move(value);
        return {true, object_element};
    }

    /// the parsed JSON value
    BasicJsonType& root;
    /// stack to model hierarchy of values
    std::vector<BasicJsonType*> ref_stack {};
    /// stack to manage which values to keep
    std::vector<bool> keep_stack {};
    /// stack to manage which object keys to keep
    std::vector<bool> key_keep_stack {};
    /// helper to hold the reference for the next object element
    BasicJsonType* object_element = nullptr;
    /// whether a syntax error occurred
    bool errored = false;
    /// callback function
    const parser_callback_t callback = nullptr;
    /// whether to throw exceptions in case of errors
    const bool allow_exceptions = true;
    /// a discarded value for the callback
    BasicJsonType discarded = BasicJsonType::value_t::discarded;
};

template<typename BasicJsonType>
class json_sax_acceptor
{
  public:
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;
    using string_t = typename BasicJsonType::string_t;
    using binary_t = typename BasicJsonType::binary_t;

    bool null()
    {
        return true;
    }

    bool boolean(bool /*unused*/)
    {
        return true;
    }

    bool number_integer(number_integer_t /*unused*/)
    {
        return true;
    }

    bool number_unsigned(number_unsigned_t /*unused*/)
    {
        return true;
    }

    bool number_float(number_float_t /*unused*/, const string_t& /*unused*/)
    {
        return true;
    }

    bool string(string_t& /*unused*/)
    {
        return true;
    }

    bool binary(binary_t& /*unused*/)
    {
        return true;
    }

    bool start_object(std::size_t /*unused*/ = static_cast<std::size_t>(-1))
    {
        return true;
    }

    bool key(string_t& /*unused*/)
    {
        return true;
    }

    bool end_object()
    {
        return true;
    }

    bool start_array(std::size_t /*unused*/ = static_cast<std::size_t>(-1))
    {
        return true;
    }

    bool end_array()
    {
        return true;
    }

    bool parse_error(std::size_t /*unused*/, const std::string& /*unused*/, const detail::exception& /*unused*/)
    {
        return false;
    }
};

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END
