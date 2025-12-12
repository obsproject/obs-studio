//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

/****************************************************************************\
 * Note on documentation: The source files contain links to the online      *
 * documentation of the public API at https://json.nlohmann.me. This URL    *
 * contains the most recent documentation and should also be applicable to  *
 * previous versions; documentation for deprecated functions is not         *
 * removed, but marked deprecated. See "Generate documentation" section in  *
 * file docs/README.md.                                                     *
\****************************************************************************/

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#define INCLUDE_NLOHMANN_JSON_HPP_

#include <algorithm> // all_of, find, for_each
#include <cstddef> // nullptr_t, ptrdiff_t, size_t
#include <functional> // hash, less
#include <initializer_list> // initializer_list
#ifndef JSON_NO_IO
    #include <iosfwd> // istream, ostream
#endif  // JSON_NO_IO
#include <iterator> // random_access_iterator_tag
#include <memory> // unique_ptr
#include <string> // string, stoi, to_string
#include <utility> // declval, forward, move, pair, swap
#include <vector> // vector

#include <nlohmann/adl_serializer.hpp>
#include <nlohmann/byte_container_with_subtype.hpp>
#include <nlohmann/detail/conversions/from_json.hpp>
#include <nlohmann/detail/conversions/to_json.hpp>
#include <nlohmann/detail/exceptions.hpp>
#include <nlohmann/detail/hash.hpp>
#include <nlohmann/detail/input/binary_reader.hpp>
#include <nlohmann/detail/input/input_adapters.hpp>
#include <nlohmann/detail/input/lexer.hpp>
#include <nlohmann/detail/input/parser.hpp>
#include <nlohmann/detail/iterators/internal_iterator.hpp>
#include <nlohmann/detail/iterators/iter_impl.hpp>
#include <nlohmann/detail/iterators/iteration_proxy.hpp>
#include <nlohmann/detail/iterators/json_reverse_iterator.hpp>
#include <nlohmann/detail/iterators/primitive_iterator.hpp>
#include <nlohmann/detail/json_custom_base_class.hpp>
#include <nlohmann/detail/json_pointer.hpp>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/detail/string_concat.hpp>
#include <nlohmann/detail/string_escape.hpp>
#include <nlohmann/detail/meta/cpp_future.hpp>
#include <nlohmann/detail/meta/type_traits.hpp>
#include <nlohmann/detail/output/binary_writer.hpp>
#include <nlohmann/detail/output/output_adapters.hpp>
#include <nlohmann/detail/output/serializer.hpp>
#include <nlohmann/detail/value_t.hpp>
#include <nlohmann/json_fwd.hpp>
#include <nlohmann/ordered_map.hpp>

#if defined(JSON_HAS_CPP_17)
    #if JSON_HAS_STATIC_RTTI
        #include <any>
    #endif
    #include <string_view>
#endif

/*!
@brief namespace for Niels Lohmann
@see https://github.com/nlohmann
@since version 1.0.0
*/
NLOHMANN_JSON_NAMESPACE_BEGIN

/*!
@brief a class to store JSON values

@internal
@invariant The member variables @a m_value and @a m_type have the following
relationship:
- If `m_type == value_t::object`, then `m_value.object != nullptr`.
- If `m_type == value_t::array`, then `m_value.array != nullptr`.
- If `m_type == value_t::string`, then `m_value.string != nullptr`.
The invariants are checked by member function assert_invariant().

@note ObjectType trick from https://stackoverflow.com/a/9860911
@endinternal

@since version 1.0.0

@nosubgrouping
*/
NLOHMANN_BASIC_JSON_TPL_DECLARATION
class basic_json // NOLINT(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    : public ::nlohmann::detail::json_base_class<CustomBaseClass>
{
  private:
    template<detail::value_t> friend struct detail::external_constructor;

    template<typename>
    friend class ::nlohmann::json_pointer;
    // can be restored when json_pointer backwards compatibility is removed
    // friend ::nlohmann::json_pointer<StringType>;

    template<typename BasicJsonType, typename InputType>
    friend class ::nlohmann::detail::parser;
    friend ::nlohmann::detail::serializer<basic_json>;
    template<typename BasicJsonType>
    friend class ::nlohmann::detail::iter_impl;
    template<typename BasicJsonType, typename CharType>
    friend class ::nlohmann::detail::binary_writer;
    template<typename BasicJsonType, typename InputType, typename SAX>
    friend class ::nlohmann::detail::binary_reader;
    template<typename BasicJsonType>
    friend class ::nlohmann::detail::json_sax_dom_parser;
    template<typename BasicJsonType>
    friend class ::nlohmann::detail::json_sax_dom_callback_parser;
    friend class ::nlohmann::detail::exception;

    /// workaround type for MSVC
    using basic_json_t = NLOHMANN_BASIC_JSON_TPL;
    using json_base_class_t = ::nlohmann::detail::json_base_class<CustomBaseClass>;

  JSON_PRIVATE_UNLESS_TESTED:
    // convenience aliases for types residing in namespace detail;
    using lexer = ::nlohmann::detail::lexer_base<basic_json>;

    template<typename InputAdapterType>
    static ::nlohmann::detail::parser<basic_json, InputAdapterType> parser(
        InputAdapterType adapter,
        detail::parser_callback_t<basic_json>cb = nullptr,
        const bool allow_exceptions = true,
        const bool ignore_comments = false
    )
    {
        return ::nlohmann::detail::parser<basic_json, InputAdapterType>(std::move(adapter),
                std::move(cb), allow_exceptions, ignore_comments);
    }

  private:
    using primitive_iterator_t = ::nlohmann::detail::primitive_iterator_t;
    template<typename BasicJsonType>
    using internal_iterator = ::nlohmann::detail::internal_iterator<BasicJsonType>;
    template<typename BasicJsonType>
    using iter_impl = ::nlohmann::detail::iter_impl<BasicJsonType>;
    template<typename Iterator>
    using iteration_proxy = ::nlohmann::detail::iteration_proxy<Iterator>;
    template<typename Base> using json_reverse_iterator = ::nlohmann::detail::json_reverse_iterator<Base>;

    template<typename CharType>
    using output_adapter_t = ::nlohmann::detail::output_adapter_t<CharType>;

    template<typename InputType>
    using binary_reader = ::nlohmann::detail::binary_reader<basic_json, InputType>;
    template<typename CharType> using binary_writer = ::nlohmann::detail::binary_writer<basic_json, CharType>;

  JSON_PRIVATE_UNLESS_TESTED:
    using serializer = ::nlohmann::detail::serializer<basic_json>;

  public:
    using value_t = detail::value_t;
    /// JSON Pointer, see @ref nlohmann::json_pointer
    using json_pointer = ::nlohmann::json_pointer<StringType>;
    template<typename T, typename SFINAE>
    using json_serializer = JSONSerializer<T, SFINAE>;
    /// how to treat decoding errors
    using error_handler_t = detail::error_handler_t;
    /// how to treat CBOR tags
    using cbor_tag_handler_t = detail::cbor_tag_handler_t;
    /// helper type for initializer lists of basic_json values
    using initializer_list_t = std::initializer_list<detail::json_ref<basic_json>>;

    using input_format_t = detail::input_format_t;
    /// SAX interface type, see @ref nlohmann::json_sax
    using json_sax_t = json_sax<basic_json>;

    ////////////////
    // exceptions //
    ////////////////

    /// @name exceptions
    /// Classes to implement user-defined exceptions.
    /// @{

    using exception = detail::exception;
    using parse_error = detail::parse_error;
    using invalid_iterator = detail::invalid_iterator;
    using type_error = detail::type_error;
    using out_of_range = detail::out_of_range;
    using other_error = detail::other_error;

    /// @}

    /////////////////////
    // container types //
    /////////////////////

    /// @name container types
    /// The canonic container types to use @ref basic_json like any other STL
    /// container.
    /// @{

    /// the type of elements in a basic_json container
    using value_type = basic_json;

    /// the type of an element reference
    using reference = value_type&;
    /// the type of an element const reference
    using const_reference = const value_type&;

    /// a type to represent differences between iterators
    using difference_type = std::ptrdiff_t;
    /// a type to represent container sizes
    using size_type = std::size_t;

    /// the allocator type
    using allocator_type = AllocatorType<basic_json>;

    /// the type of an element pointer
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    /// the type of an element const pointer
    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

    /// an iterator for a basic_json container
    using iterator = iter_impl<basic_json>;
    /// a const iterator for a basic_json container
    using const_iterator = iter_impl<const basic_json>;
    /// a reverse iterator for a basic_json container
    using reverse_iterator = json_reverse_iterator<typename basic_json::iterator>;
    /// a const reverse iterator for a basic_json container
    using const_reverse_iterator = json_reverse_iterator<typename basic_json::const_iterator>;

    /// @}

    /// @brief returns the allocator associated with the container
    /// @sa https://json.nlohmann.me/api/basic_json/get_allocator/
    static allocator_type get_allocator()
    {
        return allocator_type();
    }

    /// @brief returns version information on the library
    /// @sa https://json.nlohmann.me/api/basic_json/meta/
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json meta()
    {
        basic_json result;

        result["copyright"] = "(C) 2013-2023 Niels Lohmann";
        result["name"] = "JSON for Modern C++";
        result["url"] = "https://github.com/nlohmann/json";
        result["version"]["string"] =
            detail::concat(std::to_string(NLOHMANN_JSON_VERSION_MAJOR), '.',
                           std::to_string(NLOHMANN_JSON_VERSION_MINOR), '.',
                           std::to_string(NLOHMANN_JSON_VERSION_PATCH));
        result["version"]["major"] = NLOHMANN_JSON_VERSION_MAJOR;
        result["version"]["minor"] = NLOHMANN_JSON_VERSION_MINOR;
        result["version"]["patch"] = NLOHMANN_JSON_VERSION_PATCH;

#ifdef _WIN32
        result["platform"] = "win32";
#elif defined __linux__
        result["platform"] = "linux";
#elif defined __APPLE__
        result["platform"] = "apple";
#elif defined __unix__
        result["platform"] = "unix";
#else
        result["platform"] = "unknown";
#endif

#if defined(__ICC) || defined(__INTEL_COMPILER)
        result["compiler"] = {{"family", "icc"}, {"version", __INTEL_COMPILER}};
#elif defined(__clang__)
        result["compiler"] = {{"family", "clang"}, {"version", __clang_version__}};
#elif defined(__GNUC__) || defined(__GNUG__)
        result["compiler"] = {{"family", "gcc"}, {"version", detail::concat(
                    std::to_string(__GNUC__), '.',
                    std::to_string(__GNUC_MINOR__), '.',
                    std::to_string(__GNUC_PATCHLEVEL__))
            }
        };
#elif defined(__HP_cc) || defined(__HP_aCC)
        result["compiler"] = "hp"
#elif defined(__IBMCPP__)
        result["compiler"] = {{"family", "ilecpp"}, {"version", __IBMCPP__}};
#elif defined(_MSC_VER)
        result["compiler"] = {{"family", "msvc"}, {"version", _MSC_VER}};
#elif defined(__PGI)
        result["compiler"] = {{"family", "pgcpp"}, {"version", __PGI}};
#elif defined(__SUNPRO_CC)
        result["compiler"] = {{"family", "sunpro"}, {"version", __SUNPRO_CC}};
#else
        result["compiler"] = {{"family", "unknown"}, {"version", "unknown"}};
#endif

#if defined(_MSVC_LANG)
        result["compiler"]["c++"] = std::to_string(_MSVC_LANG);
#elif defined(__cplusplus)
        result["compiler"]["c++"] = std::to_string(__cplusplus);
#else
        result["compiler"]["c++"] = "unknown";
#endif
        return result;
    }

    ///////////////////////////
    // JSON value data types //
    ///////////////////////////

    /// @name JSON value data types
    /// The data types to store a JSON value. These types are derived from
    /// the template arguments passed to class @ref basic_json.
    /// @{

    /// @brief default object key comparator type
    /// The actual object key comparator type (@ref object_comparator_t) may be
    /// different.
    /// @sa https://json.nlohmann.me/api/basic_json/default_object_comparator_t/
#if defined(JSON_HAS_CPP_14)
    // use of transparent comparator avoids unnecessary repeated construction of temporaries
    // in functions involving lookup by key with types other than object_t::key_type (aka. StringType)
    using default_object_comparator_t = std::less<>;
#else
    using default_object_comparator_t = std::less<StringType>;
#endif

    /// @brief a type for an object
    /// @sa https://json.nlohmann.me/api/basic_json/object_t/
    using object_t = ObjectType<StringType,
          basic_json,
          default_object_comparator_t,
          AllocatorType<std::pair<const StringType,
          basic_json>>>;

    /// @brief a type for an array
    /// @sa https://json.nlohmann.me/api/basic_json/array_t/
    using array_t = ArrayType<basic_json, AllocatorType<basic_json>>;

    /// @brief a type for a string
    /// @sa https://json.nlohmann.me/api/basic_json/string_t/
    using string_t = StringType;

    /// @brief a type for a boolean
    /// @sa https://json.nlohmann.me/api/basic_json/boolean_t/
    using boolean_t = BooleanType;

    /// @brief a type for a number (integer)
    /// @sa https://json.nlohmann.me/api/basic_json/number_integer_t/
    using number_integer_t = NumberIntegerType;

    /// @brief a type for a number (unsigned)
    /// @sa https://json.nlohmann.me/api/basic_json/number_unsigned_t/
    using number_unsigned_t = NumberUnsignedType;

    /// @brief a type for a number (floating-point)
    /// @sa https://json.nlohmann.me/api/basic_json/number_float_t/
    using number_float_t = NumberFloatType;

    /// @brief a type for a packed binary type
    /// @sa https://json.nlohmann.me/api/basic_json/binary_t/
    using binary_t = nlohmann::byte_container_with_subtype<BinaryType>;

    /// @brief object key comparator type
    /// @sa https://json.nlohmann.me/api/basic_json/object_comparator_t/
    using object_comparator_t = detail::actual_object_comparator_t<basic_json>;

    /// @}

  private:

    /// helper for exception-safe object creation
    template<typename T, typename... Args>
    JSON_HEDLEY_RETURNS_NON_NULL
    static T* create(Args&& ... args)
    {
        AllocatorType<T> alloc;
        using AllocatorTraits = std::allocator_traits<AllocatorType<T>>;

        auto deleter = [&](T * obj)
        {
            AllocatorTraits::deallocate(alloc, obj, 1);
        };
        std::unique_ptr<T, decltype(deleter)> obj(AllocatorTraits::allocate(alloc, 1), deleter);
        AllocatorTraits::construct(alloc, obj.get(), std::forward<Args>(args)...);
        JSON_ASSERT(obj != nullptr);
        return obj.release();
    }

    ////////////////////////
    // JSON value storage //
    ////////////////////////

  JSON_PRIVATE_UNLESS_TESTED:
    /*!
    @brief a JSON value

    The actual storage for a JSON value of the @ref basic_json class. This
    union combines the different storage types for the JSON value types
    defined in @ref value_t.

    JSON type | value_t type    | used type
    --------- | --------------- | ------------------------
    object    | object          | pointer to @ref object_t
    array     | array           | pointer to @ref array_t
    string    | string          | pointer to @ref string_t
    boolean   | boolean         | @ref boolean_t
    number    | number_integer  | @ref number_integer_t
    number    | number_unsigned | @ref number_unsigned_t
    number    | number_float    | @ref number_float_t
    binary    | binary          | pointer to @ref binary_t
    null      | null            | *no value is stored*

    @note Variable-length types (objects, arrays, and strings) are stored as
    pointers. The size of the union should not exceed 64 bits if the default
    value types are used.

    @since version 1.0.0
    */
    union json_value
    {
        /// object (stored with pointer to save storage)
        object_t* object;
        /// array (stored with pointer to save storage)
        array_t* array;
        /// string (stored with pointer to save storage)
        string_t* string;
        /// binary (stored with pointer to save storage)
        binary_t* binary;
        /// boolean
        boolean_t boolean;
        /// number (integer)
        number_integer_t number_integer;
        /// number (unsigned integer)
        number_unsigned_t number_unsigned;
        /// number (floating-point)
        number_float_t number_float;

        /// default constructor (for null values)
        json_value() = default;
        /// constructor for booleans
        json_value(boolean_t v) noexcept : boolean(v) {}
        /// constructor for numbers (integer)
        json_value(number_integer_t v) noexcept : number_integer(v) {}
        /// constructor for numbers (unsigned)
        json_value(number_unsigned_t v) noexcept : number_unsigned(v) {}
        /// constructor for numbers (floating-point)
        json_value(number_float_t v) noexcept : number_float(v) {}
        /// constructor for empty values of a given type
        json_value(value_t t)
        {
            switch (t)
            {
                case value_t::object:
                {
                    object = create<object_t>();
                    break;
                }

                case value_t::array:
                {
                    array = create<array_t>();
                    break;
                }

                case value_t::string:
                {
                    string = create<string_t>("");
                    break;
                }

                case value_t::binary:
                {
                    binary = create<binary_t>();
                    break;
                }

                case value_t::boolean:
                {
                    boolean = static_cast<boolean_t>(false);
                    break;
                }

                case value_t::number_integer:
                {
                    number_integer = static_cast<number_integer_t>(0);
                    break;
                }

                case value_t::number_unsigned:
                {
                    number_unsigned = static_cast<number_unsigned_t>(0);
                    break;
                }

                case value_t::number_float:
                {
                    number_float = static_cast<number_float_t>(0.0);
                    break;
                }

                case value_t::null:
                {
                    object = nullptr;  // silence warning, see #821
                    break;
                }

                case value_t::discarded:
                default:
                {
                    object = nullptr;  // silence warning, see #821
                    if (JSON_HEDLEY_UNLIKELY(t == value_t::null))
                    {
                        JSON_THROW(other_error::create(500, "961c151d2e87f2686a955a9be24d316f1362bf21 3.11.3", nullptr)); // LCOV_EXCL_LINE
                    }
                    break;
                }
            }
        }

        /// constructor for strings
        json_value(const string_t& value) : string(create<string_t>(value)) {}

        /// constructor for rvalue strings
        json_value(string_t&& value) : string(create<string_t>(std::move(value))) {}

        /// constructor for objects
        json_value(const object_t& value) : object(create<object_t>(value)) {}

        /// constructor for rvalue objects
        json_value(object_t&& value) : object(create<object_t>(std::move(value))) {}

        /// constructor for arrays
        json_value(const array_t& value) : array(create<array_t>(value)) {}

        /// constructor for rvalue arrays
        json_value(array_t&& value) : array(create<array_t>(std::move(value))) {}

        /// constructor for binary arrays
        json_value(const typename binary_t::container_type& value) : binary(create<binary_t>(value)) {}

        /// constructor for rvalue binary arrays
        json_value(typename binary_t::container_type&& value) : binary(create<binary_t>(std::move(value))) {}

        /// constructor for binary arrays (internal type)
        json_value(const binary_t& value) : binary(create<binary_t>(value)) {}

        /// constructor for rvalue binary arrays (internal type)
        json_value(binary_t&& value) : binary(create<binary_t>(std::move(value))) {}

        void destroy(value_t t)
        {
            if (
                (t == value_t::object && object == nullptr) ||
                (t == value_t::array && array == nullptr) ||
                (t == value_t::string && string == nullptr) ||
                (t == value_t::binary && binary == nullptr)
            )
            {
                //not initialized (e.g. due to exception in the ctor)
                return;
            }
            if (t == value_t::array || t == value_t::object)
            {
                // flatten the current json_value to a heap-allocated stack
                std::vector<basic_json> stack;

                // move the top-level items to stack
                if (t == value_t::array)
                {
                    stack.reserve(array->size());
                    std::move(array->begin(), array->end(), std::back_inserter(stack));
                }
                else
                {
                    stack.reserve(object->size());
                    for (auto&& it : *object)
                    {
                        stack.push_back(std::move(it.second));
                    }
                }

                while (!stack.empty())
                {
                    // move the last item to local variable to be processed
                    basic_json current_item(std::move(stack.back()));
                    stack.pop_back();

                    // if current_item is array/object, move
                    // its children to the stack to be processed later
                    if (current_item.is_array())
                    {
                        std::move(current_item.m_data.m_value.array->begin(), current_item.m_data.m_value.array->end(), std::back_inserter(stack));

                        current_item.m_data.m_value.array->clear();
                    }
                    else if (current_item.is_object())
                    {
                        for (auto&& it : *current_item.m_data.m_value.object)
                        {
                            stack.push_back(std::move(it.second));
                        }

                        current_item.m_data.m_value.object->clear();
                    }

                    // it's now safe that current_item get destructed
                    // since it doesn't have any children
                }
            }

            switch (t)
            {
                case value_t::object:
                {
                    AllocatorType<object_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, object);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, object, 1);
                    break;
                }

                case value_t::array:
                {
                    AllocatorType<array_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, array);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, array, 1);
                    break;
                }

                case value_t::string:
                {
                    AllocatorType<string_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, string);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, string, 1);
                    break;
                }

                case value_t::binary:
                {
                    AllocatorType<binary_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, binary);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, binary, 1);
                    break;
                }

                case value_t::null:
                case value_t::boolean:
                case value_t::number_integer:
                case value_t::number_unsigned:
                case value_t::number_float:
                case value_t::discarded:
                default:
                {
                    break;
                }
            }
        }
    };

  private:
    /*!
    @brief checks the class invariants

    This function asserts the class invariants. It needs to be called at the
    end of every constructor to make sure that created objects respect the
    invariant. Furthermore, it has to be called each time the type of a JSON
    value is changed, because the invariant expresses a relationship between
    @a m_type and @a m_value.

    Furthermore, the parent relation is checked for arrays and objects: If
    @a check_parents true and the value is an array or object, then the
    container's elements must have the current value as parent.

    @param[in] check_parents  whether the parent relation should be checked.
               The value is true by default and should only be set to false
               during destruction of objects when the invariant does not
               need to hold.
    */
    void assert_invariant(bool check_parents = true) const noexcept
    {
        JSON_ASSERT(m_data.m_type != value_t::object || m_data.m_value.object != nullptr);
        JSON_ASSERT(m_data.m_type != value_t::array || m_data.m_value.array != nullptr);
        JSON_ASSERT(m_data.m_type != value_t::string || m_data.m_value.string != nullptr);
        JSON_ASSERT(m_data.m_type != value_t::binary || m_data.m_value.binary != nullptr);

#if JSON_DIAGNOSTICS
        JSON_TRY
        {
            // cppcheck-suppress assertWithSideEffect
            JSON_ASSERT(!check_parents || !is_structured() || std::all_of(begin(), end(), [this](const basic_json & j)
            {
                return j.m_parent == this;
            }));
        }
        JSON_CATCH(...) {} // LCOV_EXCL_LINE
#endif
        static_cast<void>(check_parents);
    }

    void set_parents()
    {
#if JSON_DIAGNOSTICS
        switch (m_data.m_type)
        {
            case value_t::array:
            {
                for (auto& element : *m_data.m_value.array)
                {
                    element.m_parent = this;
                }
                break;
            }

            case value_t::object:
            {
                for (auto& element : *m_data.m_value.object)
                {
                    element.second.m_parent = this;
                }
                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                break;
        }
#endif
    }

    iterator set_parents(iterator it, typename iterator::difference_type count_set_parents)
    {
#if JSON_DIAGNOSTICS
        for (typename iterator::difference_type i = 0; i < count_set_parents; ++i)
        {
            (it + i)->m_parent = this;
        }
#else
        static_cast<void>(count_set_parents);
#endif
        return it;
    }

    reference set_parent(reference j, std::size_t old_capacity = static_cast<std::size_t>(-1))
    {
#if JSON_DIAGNOSTICS
        if (old_capacity != static_cast<std::size_t>(-1))
        {
            // see https://github.com/nlohmann/json/issues/2838
            JSON_ASSERT(type() == value_t::array);
            if (JSON_HEDLEY_UNLIKELY(m_data.m_value.array->capacity() != old_capacity))
            {
                // capacity has changed: update all parents
                set_parents();
                return j;
            }
        }

        // ordered_json uses a vector internally, so pointers could have
        // been invalidated; see https://github.com/nlohmann/json/issues/2962
#ifdef JSON_HEDLEY_MSVC_VERSION
#pragma warning(push )
#pragma warning(disable : 4127) // ignore warning to replace if with if constexpr
#endif
        if (detail::is_ordered_map<object_t>::value)
        {
            set_parents();
            return j;
        }
#ifdef JSON_HEDLEY_MSVC_VERSION
#pragma warning( pop )
#endif

        j.m_parent = this;
#else
        static_cast<void>(j);
        static_cast<void>(old_capacity);
#endif
        return j;
    }

  public:
    //////////////////////////
    // JSON parser callback //
    //////////////////////////

    /// @brief parser event types
    /// @sa https://json.nlohmann.me/api/basic_json/parse_event_t/
    using parse_event_t = detail::parse_event_t;

    /// @brief per-element parser callback type
    /// @sa https://json.nlohmann.me/api/basic_json/parser_callback_t/
    using parser_callback_t = detail::parser_callback_t<basic_json>;

    //////////////////
    // constructors //
    //////////////////

    /// @name constructors and destructors
    /// Constructors of class @ref basic_json, copy/move constructor, copy
    /// assignment, static functions creating objects, and the destructor.
    /// @{

    /// @brief create an empty value with a given type
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    basic_json(const value_t v)
        : m_data(v)
    {
        assert_invariant();
    }

    /// @brief create a null object
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    basic_json(std::nullptr_t = nullptr) noexcept // NOLINT(bugprone-exception-escape)
        : basic_json(value_t::null)
    {
        assert_invariant();
    }

    /// @brief create a JSON value from compatible types
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    template < typename CompatibleType,
               typename U = detail::uncvref_t<CompatibleType>,
               detail::enable_if_t <
                   !detail::is_basic_json<U>::value && detail::is_compatible_type<basic_json_t, U>::value, int > = 0 >
    basic_json(CompatibleType && val) noexcept(noexcept( // NOLINT(bugprone-forwarding-reference-overload,bugprone-exception-escape)
                JSONSerializer<U>::to_json(std::declval<basic_json_t&>(),
                                           std::forward<CompatibleType>(val))))
    {
        JSONSerializer<U>::to_json(*this, std::forward<CompatibleType>(val));
        set_parents();
        assert_invariant();
    }

    /// @brief create a JSON value from an existing one
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    template < typename BasicJsonType,
               detail::enable_if_t <
                   detail::is_basic_json<BasicJsonType>::value&& !std::is_same<basic_json, BasicJsonType>::value, int > = 0 >
    basic_json(const BasicJsonType& val)
    {
        using other_boolean_t = typename BasicJsonType::boolean_t;
        using other_number_float_t = typename BasicJsonType::number_float_t;
        using other_number_integer_t = typename BasicJsonType::number_integer_t;
        using other_number_unsigned_t = typename BasicJsonType::number_unsigned_t;
        using other_string_t = typename BasicJsonType::string_t;
        using other_object_t = typename BasicJsonType::object_t;
        using other_array_t = typename BasicJsonType::array_t;
        using other_binary_t = typename BasicJsonType::binary_t;

        switch (val.type())
        {
            case value_t::boolean:
                JSONSerializer<other_boolean_t>::to_json(*this, val.template get<other_boolean_t>());
                break;
            case value_t::number_float:
                JSONSerializer<other_number_float_t>::to_json(*this, val.template get<other_number_float_t>());
                break;
            case value_t::number_integer:
                JSONSerializer<other_number_integer_t>::to_json(*this, val.template get<other_number_integer_t>());
                break;
            case value_t::number_unsigned:
                JSONSerializer<other_number_unsigned_t>::to_json(*this, val.template get<other_number_unsigned_t>());
                break;
            case value_t::string:
                JSONSerializer<other_string_t>::to_json(*this, val.template get_ref<const other_string_t&>());
                break;
            case value_t::object:
                JSONSerializer<other_object_t>::to_json(*this, val.template get_ref<const other_object_t&>());
                break;
            case value_t::array:
                JSONSerializer<other_array_t>::to_json(*this, val.template get_ref<const other_array_t&>());
                break;
            case value_t::binary:
                JSONSerializer<other_binary_t>::to_json(*this, val.template get_ref<const other_binary_t&>());
                break;
            case value_t::null:
                *this = nullptr;
                break;
            case value_t::discarded:
                m_data.m_type = value_t::discarded;
                break;
            default:            // LCOV_EXCL_LINE
                JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
        }
        JSON_ASSERT(m_data.m_type == val.type());
        set_parents();
        assert_invariant();
    }

    /// @brief create a container (array or object) from an initializer list
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    basic_json(initializer_list_t init,
               bool type_deduction = true,
               value_t manual_type = value_t::array)
    {
        // check if each element is an array with two elements whose first
        // element is a string
        bool is_an_object = std::all_of(init.begin(), init.end(),
                                        [](const detail::json_ref<basic_json>& element_ref)
        {
            // The cast is to ensure op[size_type] is called, bearing in mind size_type may not be int;
            // (many string types can be constructed from 0 via its null-pointer guise, so we get a
            // broken call to op[key_type], the wrong semantics and a 4804 warning on Windows)
            return element_ref->is_array() && element_ref->size() == 2 && (*element_ref)[static_cast<size_type>(0)].is_string();
        });

        // adjust type if type deduction is not wanted
        if (!type_deduction)
        {
            // if array is wanted, do not create an object though possible
            if (manual_type == value_t::array)
            {
                is_an_object = false;
            }

            // if object is wanted but impossible, throw an exception
            if (JSON_HEDLEY_UNLIKELY(manual_type == value_t::object && !is_an_object))
            {
                JSON_THROW(type_error::create(301, "cannot create object from initializer list", nullptr));
            }
        }

        if (is_an_object)
        {
            // the initializer list is a list of pairs -> create object
            m_data.m_type = value_t::object;
            m_data.m_value = value_t::object;

            for (auto& element_ref : init)
            {
                auto element = element_ref.moved_or_copied();
                m_data.m_value.object->emplace(
                    std::move(*((*element.m_data.m_value.array)[0].m_data.m_value.string)),
                    std::move((*element.m_data.m_value.array)[1]));
            }
        }
        else
        {
            // the initializer list describes an array -> create array
            m_data.m_type = value_t::array;
            m_data.m_value.array = create<array_t>(init.begin(), init.end());
        }

        set_parents();
        assert_invariant();
    }

    /// @brief explicitly create a binary array (without subtype)
    /// @sa https://json.nlohmann.me/api/basic_json/binary/
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json binary(const typename binary_t::container_type& init)
    {
        auto res = basic_json();
        res.m_data.m_type = value_t::binary;
        res.m_data.m_value = init;
        return res;
    }

    /// @brief explicitly create a binary array (with subtype)
    /// @sa https://json.nlohmann.me/api/basic_json/binary/
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json binary(const typename binary_t::container_type& init, typename binary_t::subtype_type subtype)
    {
        auto res = basic_json();
        res.m_data.m_type = value_t::binary;
        res.m_data.m_value = binary_t(init, subtype);
        return res;
    }

    /// @brief explicitly create a binary array
    /// @sa https://json.nlohmann.me/api/basic_json/binary/
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json binary(typename binary_t::container_type&& init)
    {
        auto res = basic_json();
        res.m_data.m_type = value_t::binary;
        res.m_data.m_value = std::move(init);
        return res;
    }

    /// @brief explicitly create a binary array (with subtype)
    /// @sa https://json.nlohmann.me/api/basic_json/binary/
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json binary(typename binary_t::container_type&& init, typename binary_t::subtype_type subtype)
    {
        auto res = basic_json();
        res.m_data.m_type = value_t::binary;
        res.m_data.m_value = binary_t(std::move(init), subtype);
        return res;
    }

    /// @brief explicitly create an array from an initializer list
    /// @sa https://json.nlohmann.me/api/basic_json/array/
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json array(initializer_list_t init = {})
    {
        return basic_json(init, false, value_t::array);
    }

    /// @brief explicitly create an object from an initializer list
    /// @sa https://json.nlohmann.me/api/basic_json/object/
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json object(initializer_list_t init = {})
    {
        return basic_json(init, false, value_t::object);
    }

    /// @brief construct an array with count copies of given value
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    basic_json(size_type cnt, const basic_json& val):
        m_data{cnt, val}
    {
        set_parents();
        assert_invariant();
    }

    /// @brief construct a JSON container given an iterator range
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    template < class InputIT, typename std::enable_if <
                   std::is_same<InputIT, typename basic_json_t::iterator>::value ||
                   std::is_same<InputIT, typename basic_json_t::const_iterator>::value, int >::type = 0 >
    basic_json(InputIT first, InputIT last)
    {
        JSON_ASSERT(first.m_object != nullptr);
        JSON_ASSERT(last.m_object != nullptr);

        // make sure iterator fits the current value
        if (JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(201, "iterators are not compatible", nullptr));
        }

        // copy type from first iterator
        m_data.m_type = first.m_object->m_data.m_type;

        // check if iterator range is complete for primitive values
        switch (m_data.m_type)
        {
            case value_t::boolean:
            case value_t::number_float:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::string:
            {
                if (JSON_HEDLEY_UNLIKELY(!first.m_it.primitive_iterator.is_begin()
                                         || !last.m_it.primitive_iterator.is_end()))
                {
                    JSON_THROW(invalid_iterator::create(204, "iterators out of range", first.m_object));
                }
                break;
            }

            case value_t::null:
            case value_t::object:
            case value_t::array:
            case value_t::binary:
            case value_t::discarded:
            default:
                break;
        }

        switch (m_data.m_type)
        {
            case value_t::number_integer:
            {
                m_data.m_value.number_integer = first.m_object->m_data.m_value.number_integer;
                break;
            }

            case value_t::number_unsigned:
            {
                m_data.m_value.number_unsigned = first.m_object->m_data.m_value.number_unsigned;
                break;
            }

            case value_t::number_float:
            {
                m_data.m_value.number_float = first.m_object->m_data.m_value.number_float;
                break;
            }

            case value_t::boolean:
            {
                m_data.m_value.boolean = first.m_object->m_data.m_value.boolean;
                break;
            }

            case value_t::string:
            {
                m_data.m_value = *first.m_object->m_data.m_value.string;
                break;
            }

            case value_t::object:
            {
                m_data.m_value.object = create<object_t>(first.m_it.object_iterator,
                                        last.m_it.object_iterator);
                break;
            }

            case value_t::array:
            {
                m_data.m_value.array = create<array_t>(first.m_it.array_iterator,
                                                       last.m_it.array_iterator);
                break;
            }

            case value_t::binary:
            {
                m_data.m_value = *first.m_object->m_data.m_value.binary;
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                JSON_THROW(invalid_iterator::create(206, detail::concat("cannot construct with iterators from ", first.m_object->type_name()), first.m_object));
        }

        set_parents();
        assert_invariant();
    }

    ///////////////////////////////////////
    // other constructors and destructor //
    ///////////////////////////////////////

    template<typename JsonRef,
             detail::enable_if_t<detail::conjunction<detail::is_json_ref<JsonRef>,
                                 std::is_same<typename JsonRef::value_type, basic_json>>::value, int> = 0 >
    basic_json(const JsonRef& ref) : basic_json(ref.moved_or_copied()) {}

    /// @brief copy constructor
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    basic_json(const basic_json& other)
        : json_base_class_t(other)
    {
        m_data.m_type = other.m_data.m_type;
        // check of passed value is valid
        other.assert_invariant();

        switch (m_data.m_type)
        {
            case value_t::object:
            {
                m_data.m_value = *other.m_data.m_value.object;
                break;
            }

            case value_t::array:
            {
                m_data.m_value = *other.m_data.m_value.array;
                break;
            }

            case value_t::string:
            {
                m_data.m_value = *other.m_data.m_value.string;
                break;
            }

            case value_t::boolean:
            {
                m_data.m_value = other.m_data.m_value.boolean;
                break;
            }

            case value_t::number_integer:
            {
                m_data.m_value = other.m_data.m_value.number_integer;
                break;
            }

            case value_t::number_unsigned:
            {
                m_data.m_value = other.m_data.m_value.number_unsigned;
                break;
            }

            case value_t::number_float:
            {
                m_data.m_value = other.m_data.m_value.number_float;
                break;
            }

            case value_t::binary:
            {
                m_data.m_value = *other.m_data.m_value.binary;
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                break;
        }

        set_parents();
        assert_invariant();
    }

    /// @brief move constructor
    /// @sa https://json.nlohmann.me/api/basic_json/basic_json/
    basic_json(basic_json&& other) noexcept
        : json_base_class_t(std::forward<json_base_class_t>(other)),
          m_data(std::move(other.m_data))
    {
        // check that passed value is valid
        other.assert_invariant(false);

        // invalidate payload
        other.m_data.m_type = value_t::null;
        other.m_data.m_value = {};

        set_parents();
        assert_invariant();
    }

    /// @brief copy assignment
    /// @sa https://json.nlohmann.me/api/basic_json/operator=/
    basic_json& operator=(basic_json other) noexcept (
        std::is_nothrow_move_constructible<value_t>::value&&
        std::is_nothrow_move_assignable<value_t>::value&&
        std::is_nothrow_move_constructible<json_value>::value&&
        std::is_nothrow_move_assignable<json_value>::value&&
        std::is_nothrow_move_assignable<json_base_class_t>::value
    )
    {
        // check that passed value is valid
        other.assert_invariant();

        using std::swap;
        swap(m_data.m_type, other.m_data.m_type);
        swap(m_data.m_value, other.m_data.m_value);
        json_base_class_t::operator=(std::move(other));

        set_parents();
        assert_invariant();
        return *this;
    }

    /// @brief destructor
    /// @sa https://json.nlohmann.me/api/basic_json/~basic_json/
    ~basic_json() noexcept
    {
        assert_invariant(false);
    }

    /// @}

  public:
    ///////////////////////
    // object inspection //
    ///////////////////////

    /// @name object inspection
    /// Functions to inspect the type of a JSON value.
    /// @{

    /// @brief serialization
    /// @sa https://json.nlohmann.me/api/basic_json/dump/
    string_t dump(const int indent = -1,
                  const char indent_char = ' ',
                  const bool ensure_ascii = false,
                  const error_handler_t error_handler = error_handler_t::strict) const
    {
        string_t result;
        serializer s(detail::output_adapter<char, string_t>(result), indent_char, error_handler);

        if (indent >= 0)
        {
            s.dump(*this, true, ensure_ascii, static_cast<unsigned int>(indent));
        }
        else
        {
            s.dump(*this, false, ensure_ascii, 0);
        }

        return result;
    }

    /// @brief return the type of the JSON value (explicit)
    /// @sa https://json.nlohmann.me/api/basic_json/type/
    constexpr value_t type() const noexcept
    {
        return m_data.m_type;
    }

    /// @brief return whether type is primitive
    /// @sa https://json.nlohmann.me/api/basic_json/is_primitive/
    constexpr bool is_primitive() const noexcept
    {
        return is_null() || is_string() || is_boolean() || is_number() || is_binary();
    }

    /// @brief return whether type is structured
    /// @sa https://json.nlohmann.me/api/basic_json/is_structured/
    constexpr bool is_structured() const noexcept
    {
        return is_array() || is_object();
    }

    /// @brief return whether value is null
    /// @sa https://json.nlohmann.me/api/basic_json/is_null/
    constexpr bool is_null() const noexcept
    {
        return m_data.m_type == value_t::null;
    }

    /// @brief return whether value is a boolean
    /// @sa https://json.nlohmann.me/api/basic_json/is_boolean/
    constexpr bool is_boolean() const noexcept
    {
        return m_data.m_type == value_t::boolean;
    }

    /// @brief return whether value is a number
    /// @sa https://json.nlohmann.me/api/basic_json/is_number/
    constexpr bool is_number() const noexcept
    {
        return is_number_integer() || is_number_float();
    }

    /// @brief return whether value is an integer number
    /// @sa https://json.nlohmann.me/api/basic_json/is_number_integer/
    constexpr bool is_number_integer() const noexcept
    {
        return m_data.m_type == value_t::number_integer || m_data.m_type == value_t::number_unsigned;
    }

    /// @brief return whether value is an unsigned integer number
    /// @sa https://json.nlohmann.me/api/basic_json/is_number_unsigned/
    constexpr bool is_number_unsigned() const noexcept
    {
        return m_data.m_type == value_t::number_unsigned;
    }

    /// @brief return whether value is a floating-point number
    /// @sa https://json.nlohmann.me/api/basic_json/is_number_float/
    constexpr bool is_number_float() const noexcept
    {
        return m_data.m_type == value_t::number_float;
    }

    /// @brief return whether value is an object
    /// @sa https://json.nlohmann.me/api/basic_json/is_object/
    constexpr bool is_object() const noexcept
    {
        return m_data.m_type == value_t::object;
    }

    /// @brief return whether value is an array
    /// @sa https://json.nlohmann.me/api/basic_json/is_array/
    constexpr bool is_array() const noexcept
    {
        return m_data.m_type == value_t::array;
    }

    /// @brief return whether value is a string
    /// @sa https://json.nlohmann.me/api/basic_json/is_string/
    constexpr bool is_string() const noexcept
    {
        return m_data.m_type == value_t::string;
    }

    /// @brief return whether value is a binary array
    /// @sa https://json.nlohmann.me/api/basic_json/is_binary/
    constexpr bool is_binary() const noexcept
    {
        return m_data.m_type == value_t::binary;
    }

    /// @brief return whether value is discarded
    /// @sa https://json.nlohmann.me/api/basic_json/is_discarded/
    constexpr bool is_discarded() const noexcept
    {
        return m_data.m_type == value_t::discarded;
    }

    /// @brief return the type of the JSON value (implicit)
    /// @sa https://json.nlohmann.me/api/basic_json/operator_value_t/
    constexpr operator value_t() const noexcept
    {
        return m_data.m_type;
    }

    /// @}

  private:
    //////////////////
    // value access //
    //////////////////

    /// get a boolean (explicit)
    boolean_t get_impl(boolean_t* /*unused*/) const
    {
        if (JSON_HEDLEY_LIKELY(is_boolean()))
        {
            return m_data.m_value.boolean;
        }

        JSON_THROW(type_error::create(302, detail::concat("type must be boolean, but is ", type_name()), this));
    }

    /// get a pointer to the value (object)
    object_t* get_impl_ptr(object_t* /*unused*/) noexcept
    {
        return is_object() ? m_data.m_value.object : nullptr;
    }

    /// get a pointer to the value (object)
    constexpr const object_t* get_impl_ptr(const object_t* /*unused*/) const noexcept
    {
        return is_object() ? m_data.m_value.object : nullptr;
    }

    /// get a pointer to the value (array)
    array_t* get_impl_ptr(array_t* /*unused*/) noexcept
    {
        return is_array() ? m_data.m_value.array : nullptr;
    }

    /// get a pointer to the value (array)
    constexpr const array_t* get_impl_ptr(const array_t* /*unused*/) const noexcept
    {
        return is_array() ? m_data.m_value.array : nullptr;
    }

    /// get a pointer to the value (string)
    string_t* get_impl_ptr(string_t* /*unused*/) noexcept
    {
        return is_string() ? m_data.m_value.string : nullptr;
    }

    /// get a pointer to the value (string)
    constexpr const string_t* get_impl_ptr(const string_t* /*unused*/) const noexcept
    {
        return is_string() ? m_data.m_value.string : nullptr;
    }

    /// get a pointer to the value (boolean)
    boolean_t* get_impl_ptr(boolean_t* /*unused*/) noexcept
    {
        return is_boolean() ? &m_data.m_value.boolean : nullptr;
    }

    /// get a pointer to the value (boolean)
    constexpr const boolean_t* get_impl_ptr(const boolean_t* /*unused*/) const noexcept
    {
        return is_boolean() ? &m_data.m_value.boolean : nullptr;
    }

    /// get a pointer to the value (integer number)
    number_integer_t* get_impl_ptr(number_integer_t* /*unused*/) noexcept
    {
        return is_number_integer() ? &m_data.m_value.number_integer : nullptr;
    }

    /// get a pointer to the value (integer number)
    constexpr const number_integer_t* get_impl_ptr(const number_integer_t* /*unused*/) const noexcept
    {
        return is_number_integer() ? &m_data.m_value.number_integer : nullptr;
    }

    /// get a pointer to the value (unsigned number)
    number_unsigned_t* get_impl_ptr(number_unsigned_t* /*unused*/) noexcept
    {
        return is_number_unsigned() ? &m_data.m_value.number_unsigned : nullptr;
    }

    /// get a pointer to the value (unsigned number)
    constexpr const number_unsigned_t* get_impl_ptr(const number_unsigned_t* /*unused*/) const noexcept
    {
        return is_number_unsigned() ? &m_data.m_value.number_unsigned : nullptr;
    }

    /// get a pointer to the value (floating-point number)
    number_float_t* get_impl_ptr(number_float_t* /*unused*/) noexcept
    {
        return is_number_float() ? &m_data.m_value.number_float : nullptr;
    }

    /// get a pointer to the value (floating-point number)
    constexpr const number_float_t* get_impl_ptr(const number_float_t* /*unused*/) const noexcept
    {
        return is_number_float() ? &m_data.m_value.number_float : nullptr;
    }

    /// get a pointer to the value (binary)
    binary_t* get_impl_ptr(binary_t* /*unused*/) noexcept
    {
        return is_binary() ? m_data.m_value.binary : nullptr;
    }

    /// get a pointer to the value (binary)
    constexpr const binary_t* get_impl_ptr(const binary_t* /*unused*/) const noexcept
    {
        return is_binary() ? m_data.m_value.binary : nullptr;
    }

    /*!
    @brief helper function to implement get_ref()

    This function helps to implement get_ref() without code duplication for
    const and non-const overloads

    @tparam ThisType will be deduced as `basic_json` or `const basic_json`

    @throw type_error.303 if ReferenceType does not match underlying value
    type of the current JSON
    */
    template<typename ReferenceType, typename ThisType>
    static ReferenceType get_ref_impl(ThisType& obj)
    {
        // delegate the call to get_ptr<>()
        auto* ptr = obj.template get_ptr<typename std::add_pointer<ReferenceType>::type>();

        if (JSON_HEDLEY_LIKELY(ptr != nullptr))
        {
            return *ptr;
        }

        JSON_THROW(type_error::create(303, detail::concat("incompatible ReferenceType for get_ref, actual type is ", obj.type_name()), &obj));
    }

  public:
    /// @name value access
    /// Direct access to the stored value of a JSON value.
    /// @{

    /// @brief get a pointer value (implicit)
    /// @sa https://json.nlohmann.me/api/basic_json/get_ptr/
    template<typename PointerType, typename std::enable_if<
                 std::is_pointer<PointerType>::value, int>::type = 0>
    auto get_ptr() noexcept -> decltype(std::declval<basic_json_t&>().get_impl_ptr(std::declval<PointerType>()))
    {
        // delegate the call to get_impl_ptr<>()
        return get_impl_ptr(static_cast<PointerType>(nullptr));
    }

    /// @brief get a pointer value (implicit)
    /// @sa https://json.nlohmann.me/api/basic_json/get_ptr/
    template < typename PointerType, typename std::enable_if <
                   std::is_pointer<PointerType>::value&&
                   std::is_const<typename std::remove_pointer<PointerType>::type>::value, int >::type = 0 >
    constexpr auto get_ptr() const noexcept -> decltype(std::declval<const basic_json_t&>().get_impl_ptr(std::declval<PointerType>()))
    {
        // delegate the call to get_impl_ptr<>() const
        return get_impl_ptr(static_cast<PointerType>(nullptr));
    }

  private:
    /*!
    @brief get a value (explicit)

    Explicit type conversion between the JSON value and a compatible value
    which is [CopyConstructible](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)
    and [DefaultConstructible](https://en.cppreference.com/w/cpp/named_req/DefaultConstructible).
    The value is converted by calling the @ref json_serializer<ValueType>
    `from_json()` method.

    The function is equivalent to executing
    @code {.cpp}
    ValueType ret;
    JSONSerializer<ValueType>::from_json(*this, ret);
    return ret;
    @endcode

    This overloads is chosen if:
    - @a ValueType is not @ref basic_json,
    - @ref json_serializer<ValueType> has a `from_json()` method of the form
      `void from_json(const basic_json&, ValueType&)`, and
    - @ref json_serializer<ValueType> does not have a `from_json()` method of
      the form `ValueType from_json(const basic_json&)`

    @tparam ValueType the returned value type

    @return copy of the JSON value, converted to @a ValueType

    @throw what @ref json_serializer<ValueType> `from_json()` method throws

    @liveexample{The example below shows several conversions from JSON values
    to other types. There a few things to note: (1) Floating-point numbers can
    be converted to integers\, (2) A JSON array can be converted to a standard
    `std::vector<short>`\, (3) A JSON object can be converted to C++
    associative containers such as `std::unordered_map<std::string\,
    json>`.,get__ValueType_const}

    @since version 2.1.0
    */
    template < typename ValueType,
               detail::enable_if_t <
                   detail::is_default_constructible<ValueType>::value&&
                   detail::has_from_json<basic_json_t, ValueType>::value,
                   int > = 0 >
    ValueType get_impl(detail::priority_tag<0> /*unused*/) const noexcept(noexcept(
                JSONSerializer<ValueType>::from_json(std::declval<const basic_json_t&>(), std::declval<ValueType&>())))
    {
        auto ret = ValueType();
        JSONSerializer<ValueType>::from_json(*this, ret);
        return ret;
    }

    /*!
    @brief get a value (explicit); special case

    Explicit type conversion between the JSON value and a compatible value
    which is **not** [CopyConstructible](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)
    and **not** [DefaultConstructible](https://en.cppreference.com/w/cpp/named_req/DefaultConstructible).
    The value is converted by calling the @ref json_serializer<ValueType>
    `from_json()` method.

    The function is equivalent to executing
    @code {.cpp}
    return JSONSerializer<ValueType>::from_json(*this);
    @endcode

    This overloads is chosen if:
    - @a ValueType is not @ref basic_json and
    - @ref json_serializer<ValueType> has a `from_json()` method of the form
      `ValueType from_json(const basic_json&)`

    @note If @ref json_serializer<ValueType> has both overloads of
    `from_json()`, this one is chosen.

    @tparam ValueType the returned value type

    @return copy of the JSON value, converted to @a ValueType

    @throw what @ref json_serializer<ValueType> `from_json()` method throws

    @since version 2.1.0
    */
    template < typename ValueType,
               detail::enable_if_t <
                   detail::has_non_default_from_json<basic_json_t, ValueType>::value,
                   int > = 0 >
    ValueType get_impl(detail::priority_tag<1> /*unused*/) const noexcept(noexcept(
                JSONSerializer<ValueType>::from_json(std::declval<const basic_json_t&>())))
    {
        return JSONSerializer<ValueType>::from_json(*this);
    }

    /*!
    @brief get special-case overload

    This overloads converts the current @ref basic_json in a different
    @ref basic_json type

    @tparam BasicJsonType == @ref basic_json

    @return a copy of *this, converted into @a BasicJsonType

    @complexity Depending on the implementation of the called `from_json()`
                method.

    @since version 3.2.0
    */
    template < typename BasicJsonType,
               detail::enable_if_t <
                   detail::is_basic_json<BasicJsonType>::value,
                   int > = 0 >
    BasicJsonType get_impl(detail::priority_tag<2> /*unused*/) const
    {
        return *this;
    }

    /*!
    @brief get special-case overload

    This overloads avoids a lot of template boilerplate, it can be seen as the
    identity method

    @tparam BasicJsonType == @ref basic_json

    @return a copy of *this

    @complexity Constant.

    @since version 2.1.0
    */
    template<typename BasicJsonType,
             detail::enable_if_t<
                 std::is_same<BasicJsonType, basic_json_t>::value,
                 int> = 0>
    basic_json get_impl(detail::priority_tag<3> /*unused*/) const
    {
        return *this;
    }

    /*!
    @brief get a pointer value (explicit)
    @copydoc get()
    */
    template<typename PointerType,
             detail::enable_if_t<
                 std::is_pointer<PointerType>::value,
                 int> = 0>
    constexpr auto get_impl(detail::priority_tag<4> /*unused*/) const noexcept
    -> decltype(std::declval<const basic_json_t&>().template get_ptr<PointerType>())
    {
        // delegate the call to get_ptr
        return get_ptr<PointerType>();
    }

  public:
    /*!
    @brief get a (pointer) value (explicit)

    Performs explicit type conversion between the JSON value and a compatible value if required.

    - If the requested type is a pointer to the internally stored JSON value that pointer is returned.
    No copies are made.

    - If the requested type is the current @ref basic_json, or a different @ref basic_json convertible
    from the current @ref basic_json.

    - Otherwise the value is converted by calling the @ref json_serializer<ValueType> `from_json()`
    method.

    @tparam ValueTypeCV the provided value type
    @tparam ValueType the returned value type

    @return copy of the JSON value, converted to @tparam ValueType if necessary

    @throw what @ref json_serializer<ValueType> `from_json()` method throws if conversion is required

    @since version 2.1.0
    */
    template < typename ValueTypeCV, typename ValueType = detail::uncvref_t<ValueTypeCV>>
#if defined(JSON_HAS_CPP_14)
    constexpr
#endif
    auto get() const noexcept(
    noexcept(std::declval<const basic_json_t&>().template get_impl<ValueType>(detail::priority_tag<4> {})))
    -> decltype(std::declval<const basic_json_t&>().template get_impl<ValueType>(detail::priority_tag<4> {}))
    {
        // we cannot static_assert on ValueTypeCV being non-const, because
        // there is support for get<const basic_json_t>(), which is why we
        // still need the uncvref
        static_assert(!std::is_reference<ValueTypeCV>::value,
                      "get() cannot be used with reference types, you might want to use get_ref()");
        return get_impl<ValueType>(detail::priority_tag<4> {});
    }

    /*!
    @brief get a pointer value (explicit)

    Explicit pointer access to the internally stored JSON value. No copies are
    made.

    @warning The pointer becomes invalid if the underlying JSON object
    changes.

    @tparam PointerType pointer type; must be a pointer to @ref array_t, @ref
    object_t, @ref string_t, @ref boolean_t, @ref number_integer_t,
    @ref number_unsigned_t, or @ref number_float_t.

    @return pointer to the internally stored JSON value if the requested
    pointer type @a PointerType fits to the JSON value; `nullptr` otherwise

    @complexity Constant.

    @liveexample{The example below shows how pointers to internal values of a
    JSON value can be requested. Note that no type conversions are made and a
    `nullptr` is returned if the value and the requested pointer type does not
    match.,get__PointerType}

    @sa see @ref get_ptr() for explicit pointer-member access

    @since version 1.0.0
    */
    template<typename PointerType, typename std::enable_if<
                 std::is_pointer<PointerType>::value, int>::type = 0>
    auto get() noexcept -> decltype(std::declval<basic_json_t&>().template get_ptr<PointerType>())
    {
        // delegate the call to get_ptr
        return get_ptr<PointerType>();
    }

    /// @brief get a value (explicit)
    /// @sa https://json.nlohmann.me/api/basic_json/get_to/
    template < typename ValueType,
               detail::enable_if_t <
                   !detail::is_basic_json<ValueType>::value&&
                   detail::has_from_json<basic_json_t, ValueType>::value,
                   int > = 0 >
    ValueType & get_to(ValueType& v) const noexcept(noexcept(
                JSONSerializer<ValueType>::from_json(std::declval<const basic_json_t&>(), v)))
    {
        JSONSerializer<ValueType>::from_json(*this, v);
        return v;
    }

    // specialization to allow calling get_to with a basic_json value
    // see https://github.com/nlohmann/json/issues/2175
    template<typename ValueType,
             detail::enable_if_t <
                 detail::is_basic_json<ValueType>::value,
                 int> = 0>
    ValueType & get_to(ValueType& v) const
    {
        v = *this;
        return v;
    }

    template <
        typename T, std::size_t N,
        typename Array = T (&)[N], // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
        detail::enable_if_t <
            detail::has_from_json<basic_json_t, Array>::value, int > = 0 >
    Array get_to(T (&v)[N]) const // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    noexcept(noexcept(JSONSerializer<Array>::from_json(
                          std::declval<const basic_json_t&>(), v)))
    {
        JSONSerializer<Array>::from_json(*this, v);
        return v;
    }

    /// @brief get a reference value (implicit)
    /// @sa https://json.nlohmann.me/api/basic_json/get_ref/
    template<typename ReferenceType, typename std::enable_if<
                 std::is_reference<ReferenceType>::value, int>::type = 0>
    ReferenceType get_ref()
    {
        // delegate call to get_ref_impl
        return get_ref_impl<ReferenceType>(*this);
    }

    /// @brief get a reference value (implicit)
    /// @sa https://json.nlohmann.me/api/basic_json/get_ref/
    template < typename ReferenceType, typename std::enable_if <
                   std::is_reference<ReferenceType>::value&&
                   std::is_const<typename std::remove_reference<ReferenceType>::type>::value, int >::type = 0 >
    ReferenceType get_ref() const
    {
        // delegate call to get_ref_impl
        return get_ref_impl<ReferenceType>(*this);
    }

    /*!
    @brief get a value (implicit)

    Implicit type conversion between the JSON value and a compatible value.
    The call is realized by calling @ref get() const.

    @tparam ValueType non-pointer type compatible to the JSON value, for
    instance `int` for JSON integer numbers, `bool` for JSON booleans, or
    `std::vector` types for JSON arrays. The character type of @ref string_t
    as well as an initializer list of this type is excluded to avoid
    ambiguities as these types implicitly convert to `std::string`.

    @return copy of the JSON value, converted to type @a ValueType

    @throw type_error.302 in case passed type @a ValueType is incompatible
    to the JSON value type (e.g., the JSON value is of type boolean, but a
    string is requested); see example below

    @complexity Linear in the size of the JSON value.

    @liveexample{The example below shows several conversions from JSON values
    to other types. There a few things to note: (1) Floating-point numbers can
    be converted to integers\, (2) A JSON array can be converted to a standard
    `std::vector<short>`\, (3) A JSON object can be converted to C++
    associative containers such as `std::unordered_map<std::string\,
    json>`.,operator__ValueType}

    @since version 1.0.0
    */
    template < typename ValueType, typename std::enable_if <
                   detail::conjunction <
                       detail::negation<std::is_pointer<ValueType>>,
                       detail::negation<std::is_same<ValueType, std::nullptr_t>>,
                       detail::negation<std::is_same<ValueType, detail::json_ref<basic_json>>>,
                                        detail::negation<std::is_same<ValueType, typename string_t::value_type>>,
                                        detail::negation<detail::is_basic_json<ValueType>>,
                                        detail::negation<std::is_same<ValueType, std::initializer_list<typename string_t::value_type>>>,
#if defined(JSON_HAS_CPP_17) && (defined(__GNUC__) || (defined(_MSC_VER) && _MSC_VER >= 1910 && _MSC_VER <= 1914))
                                                detail::negation<std::is_same<ValueType, std::string_view>>,
#endif
#if defined(JSON_HAS_CPP_17) && JSON_HAS_STATIC_RTTI
                                                detail::negation<std::is_same<ValueType, std::any>>,
#endif
                                                detail::is_detected_lazy<detail::get_template_function, const basic_json_t&, ValueType>
                                                >::value, int >::type = 0 >
                                        JSON_EXPLICIT operator ValueType() const
    {
        // delegate the call to get<>() const
        return get<ValueType>();
    }

    /// @brief get a binary value
    /// @sa https://json.nlohmann.me/api/basic_json/get_binary/
    binary_t& get_binary()
    {
        if (!is_binary())
        {
            JSON_THROW(type_error::create(302, detail::concat("type must be binary, but is ", type_name()), this));
        }

        return *get_ptr<binary_t*>();
    }

    /// @brief get a binary value
    /// @sa https://json.nlohmann.me/api/basic_json/get_binary/
    const binary_t& get_binary() const
    {
        if (!is_binary())
        {
            JSON_THROW(type_error::create(302, detail::concat("type must be binary, but is ", type_name()), this));
        }

        return *get_ptr<const binary_t*>();
    }

    /// @}

    ////////////////////
    // element access //
    ////////////////////

    /// @name element access
    /// Access to the JSON value.
    /// @{

    /// @brief access specified array element with bounds checking
    /// @sa https://json.nlohmann.me/api/basic_json/at/
    reference at(size_type idx)
    {
        // at only works for arrays
        if (JSON_HEDLEY_LIKELY(is_array()))
        {
            JSON_TRY
            {
                return set_parent(m_data.m_value.array->at(idx));
            }
            JSON_CATCH (std::out_of_range&)
            {
                // create better exception explanation
                JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), this));
            }
        }
        else
        {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }
    }

    /// @brief access specified array element with bounds checking
    /// @sa https://json.nlohmann.me/api/basic_json/at/
    const_reference at(size_type idx) const
    {
        // at only works for arrays
        if (JSON_HEDLEY_LIKELY(is_array()))
        {
            JSON_TRY
            {
                return m_data.m_value.array->at(idx);
            }
            JSON_CATCH (std::out_of_range&)
            {
                // create better exception explanation
                JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), this));
            }
        }
        else
        {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }
    }

    /// @brief access specified object element with bounds checking
    /// @sa https://json.nlohmann.me/api/basic_json/at/
    reference at(const typename object_t::key_type& key)
    {
        // at only works for objects
        if (JSON_HEDLEY_UNLIKELY(!is_object()))
        {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }

        auto it = m_data.m_value.object->find(key);
        if (it == m_data.m_value.object->end())
        {
            JSON_THROW(out_of_range::create(403, detail::concat("key '", key, "' not found"), this));
        }
        return set_parent(it->second);
    }

    /// @brief access specified object element with bounds checking
    /// @sa https://json.nlohmann.me/api/basic_json/at/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    reference at(KeyType && key)
    {
        // at only works for objects
        if (JSON_HEDLEY_UNLIKELY(!is_object()))
        {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }

        auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
        if (it == m_data.m_value.object->end())
        {
            JSON_THROW(out_of_range::create(403, detail::concat("key '", string_t(std::forward<KeyType>(key)), "' not found"), this));
        }
        return set_parent(it->second);
    }

    /// @brief access specified object element with bounds checking
    /// @sa https://json.nlohmann.me/api/basic_json/at/
    const_reference at(const typename object_t::key_type& key) const
    {
        // at only works for objects
        if (JSON_HEDLEY_UNLIKELY(!is_object()))
        {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }

        auto it = m_data.m_value.object->find(key);
        if (it == m_data.m_value.object->end())
        {
            JSON_THROW(out_of_range::create(403, detail::concat("key '", key, "' not found"), this));
        }
        return it->second;
    }

    /// @brief access specified object element with bounds checking
    /// @sa https://json.nlohmann.me/api/basic_json/at/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    const_reference at(KeyType && key) const
    {
        // at only works for objects
        if (JSON_HEDLEY_UNLIKELY(!is_object()))
        {
            JSON_THROW(type_error::create(304, detail::concat("cannot use at() with ", type_name()), this));
        }

        auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
        if (it == m_data.m_value.object->end())
        {
            JSON_THROW(out_of_range::create(403, detail::concat("key '", string_t(std::forward<KeyType>(key)), "' not found"), this));
        }
        return it->second;
    }

    /// @brief access specified array element
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    reference operator[](size_type idx)
    {
        // implicitly convert null value to an empty array
        if (is_null())
        {
            m_data.m_type = value_t::array;
            m_data.m_value.array = create<array_t>();
            assert_invariant();
        }

        // operator[] only works for arrays
        if (JSON_HEDLEY_LIKELY(is_array()))
        {
            // fill up array with null values if given idx is outside range
            if (idx >= m_data.m_value.array->size())
            {
#if JSON_DIAGNOSTICS
                // remember array size & capacity before resizing
                const auto old_size = m_data.m_value.array->size();
                const auto old_capacity = m_data.m_value.array->capacity();
#endif
                m_data.m_value.array->resize(idx + 1);

#if JSON_DIAGNOSTICS
                if (JSON_HEDLEY_UNLIKELY(m_data.m_value.array->capacity() != old_capacity))
                {
                    // capacity has changed: update all parents
                    set_parents();
                }
                else
                {
                    // set parent for values added above
                    set_parents(begin() + static_cast<typename iterator::difference_type>(old_size), static_cast<typename iterator::difference_type>(idx + 1 - old_size));
                }
#endif
                assert_invariant();
            }

            return m_data.m_value.array->operator[](idx);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a numeric argument with ", type_name()), this));
    }

    /// @brief access specified array element
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    const_reference operator[](size_type idx) const
    {
        // const operator[] only works for arrays
        if (JSON_HEDLEY_LIKELY(is_array()))
        {
            return m_data.m_value.array->operator[](idx);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a numeric argument with ", type_name()), this));
    }

    /// @brief access specified object element
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    reference operator[](typename object_t::key_type key)
    {
        // implicitly convert null value to an empty object
        if (is_null())
        {
            m_data.m_type = value_t::object;
            m_data.m_value.object = create<object_t>();
            assert_invariant();
        }

        // operator[] only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            auto result = m_data.m_value.object->emplace(std::move(key), nullptr);
            return set_parent(result.first->second);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }

    /// @brief access specified object element
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    const_reference operator[](const typename object_t::key_type& key) const
    {
        // const operator[] only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            auto it = m_data.m_value.object->find(key);
            JSON_ASSERT(it != m_data.m_value.object->end());
            return it->second;
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }

    // these two functions resolve a (const) char * ambiguity affecting Clang and MSVC
    // (they seemingly cannot be constrained to resolve the ambiguity)
    template<typename T>
    reference operator[](T* key)
    {
        return operator[](typename object_t::key_type(key));
    }

    template<typename T>
    const_reference operator[](T* key) const
    {
        return operator[](typename object_t::key_type(key));
    }

    /// @brief access specified object element
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int > = 0 >
    reference operator[](KeyType && key)
    {
        // implicitly convert null value to an empty object
        if (is_null())
        {
            m_data.m_type = value_t::object;
            m_data.m_value.object = create<object_t>();
            assert_invariant();
        }

        // operator[] only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            auto result = m_data.m_value.object->emplace(std::forward<KeyType>(key), nullptr);
            return set_parent(result.first->second);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }

    /// @brief access specified object element
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int > = 0 >
    const_reference operator[](KeyType && key) const
    {
        // const operator[] only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
            JSON_ASSERT(it != m_data.m_value.object->end());
            return it->second;
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }

  private:
    template<typename KeyType>
    using is_comparable_with_object_key = detail::is_comparable <
        object_comparator_t, const typename object_t::key_type&, KeyType >;

    template<typename ValueType>
    using value_return_type = std::conditional <
        detail::is_c_string_uncvref<ValueType>::value,
        string_t, typename std::decay<ValueType>::type >;

  public:
    /// @brief access specified object element with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, detail::enable_if_t <
                   !detail::is_transparent<object_comparator_t>::value
                   && detail::is_getable<basic_json_t, ValueType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ValueType value(const typename object_t::key_type& key, const ValueType& default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            // if key is found, return value and given default value otherwise
            const auto it = find(key);
            if (it != end())
            {
                return it->template get<ValueType>();
            }

            return default_value;
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, class ReturnType = typename value_return_type<ValueType>::type,
               detail::enable_if_t <
                   !detail::is_transparent<object_comparator_t>::value
                   && detail::is_getable<basic_json_t, ReturnType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ReturnType value(const typename object_t::key_type& key, ValueType && default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            // if key is found, return value and given default value otherwise
            const auto it = find(key);
            if (it != end())
            {
                return it->template get<ReturnType>();
            }

            return std::forward<ValueType>(default_value);
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, class KeyType, detail::enable_if_t <
                   detail::is_transparent<object_comparator_t>::value
                   && !detail::is_json_pointer<KeyType>::value
                   && is_comparable_with_object_key<KeyType>::value
                   && detail::is_getable<basic_json_t, ValueType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ValueType value(KeyType && key, const ValueType& default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            // if key is found, return value and given default value otherwise
            const auto it = find(std::forward<KeyType>(key));
            if (it != end())
            {
                return it->template get<ValueType>();
            }

            return default_value;
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element via JSON Pointer with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, class KeyType, class ReturnType = typename value_return_type<ValueType>::type,
               detail::enable_if_t <
                   detail::is_transparent<object_comparator_t>::value
                   && !detail::is_json_pointer<KeyType>::value
                   && is_comparable_with_object_key<KeyType>::value
                   && detail::is_getable<basic_json_t, ReturnType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ReturnType value(KeyType && key, ValueType && default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            // if key is found, return value and given default value otherwise
            const auto it = find(std::forward<KeyType>(key));
            if (it != end())
            {
                return it->template get<ReturnType>();
            }

            return std::forward<ValueType>(default_value);
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element via JSON Pointer with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, detail::enable_if_t <
                   detail::is_getable<basic_json_t, ValueType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ValueType value(const json_pointer& ptr, const ValueType& default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            // if pointer resolves a value, return it or use default value
            JSON_TRY
            {
                return ptr.get_checked(this).template get<ValueType>();
            }
            JSON_INTERNAL_CATCH (out_of_range&)
            {
                return default_value;
            }
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element via JSON Pointer with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, class ReturnType = typename value_return_type<ValueType>::type,
               detail::enable_if_t <
                   detail::is_getable<basic_json_t, ReturnType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ReturnType value(const json_pointer& ptr, ValueType && default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            // if pointer resolves a value, return it or use default value
            JSON_TRY
            {
                return ptr.get_checked(this).template get<ReturnType>();
            }
            JSON_INTERNAL_CATCH (out_of_range&)
            {
                return std::forward<ValueType>(default_value);
            }
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    template < class ValueType, class BasicJsonType, detail::enable_if_t <
                   detail::is_basic_json<BasicJsonType>::value
                   && detail::is_getable<basic_json_t, ValueType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    ValueType value(const ::nlohmann::json_pointer<BasicJsonType>& ptr, const ValueType& default_value) const
    {
        return value(ptr.convert(), default_value);
    }

    template < class ValueType, class BasicJsonType, class ReturnType = typename value_return_type<ValueType>::type,
               detail::enable_if_t <
                   detail::is_basic_json<BasicJsonType>::value
                   && detail::is_getable<basic_json_t, ReturnType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    ReturnType value(const ::nlohmann::json_pointer<BasicJsonType>& ptr, ValueType && default_value) const
    {
        return value(ptr.convert(), std::forward<ValueType>(default_value));
    }

    /// @brief access the first element
    /// @sa https://json.nlohmann.me/api/basic_json/front/
    reference front()
    {
        return *begin();
    }

    /// @brief access the first element
    /// @sa https://json.nlohmann.me/api/basic_json/front/
    const_reference front() const
    {
        return *cbegin();
    }

    /// @brief access the last element
    /// @sa https://json.nlohmann.me/api/basic_json/back/
    reference back()
    {
        auto tmp = end();
        --tmp;
        return *tmp;
    }

    /// @brief access the last element
    /// @sa https://json.nlohmann.me/api/basic_json/back/
    const_reference back() const
    {
        auto tmp = cend();
        --tmp;
        return *tmp;
    }

    /// @brief remove element given an iterator
    /// @sa https://json.nlohmann.me/api/basic_json/erase/
    template < class IteratorType, detail::enable_if_t <
                   std::is_same<IteratorType, typename basic_json_t::iterator>::value ||
                   std::is_same<IteratorType, typename basic_json_t::const_iterator>::value, int > = 0 >
    IteratorType erase(IteratorType pos)
    {
        // make sure iterator fits the current value
        if (JSON_HEDLEY_UNLIKELY(this != pos.m_object))
        {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }

        IteratorType result = end();

        switch (m_data.m_type)
        {
            case value_t::boolean:
            case value_t::number_float:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::string:
            case value_t::binary:
            {
                if (JSON_HEDLEY_UNLIKELY(!pos.m_it.primitive_iterator.is_begin()))
                {
                    JSON_THROW(invalid_iterator::create(205, "iterator out of range", this));
                }

                if (is_string())
                {
                    AllocatorType<string_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.string);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.string, 1);
                    m_data.m_value.string = nullptr;
                }
                else if (is_binary())
                {
                    AllocatorType<binary_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.binary);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.binary, 1);
                    m_data.m_value.binary = nullptr;
                }

                m_data.m_type = value_t::null;
                assert_invariant();
                break;
            }

            case value_t::object:
            {
                result.m_it.object_iterator = m_data.m_value.object->erase(pos.m_it.object_iterator);
                break;
            }

            case value_t::array:
            {
                result.m_it.array_iterator = m_data.m_value.array->erase(pos.m_it.array_iterator);
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        return result;
    }

    /// @brief remove elements given an iterator range
    /// @sa https://json.nlohmann.me/api/basic_json/erase/
    template < class IteratorType, detail::enable_if_t <
                   std::is_same<IteratorType, typename basic_json_t::iterator>::value ||
                   std::is_same<IteratorType, typename basic_json_t::const_iterator>::value, int > = 0 >
    IteratorType erase(IteratorType first, IteratorType last)
    {
        // make sure iterator fits the current value
        if (JSON_HEDLEY_UNLIKELY(this != first.m_object || this != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(203, "iterators do not fit current value", this));
        }

        IteratorType result = end();

        switch (m_data.m_type)
        {
            case value_t::boolean:
            case value_t::number_float:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::string:
            case value_t::binary:
            {
                if (JSON_HEDLEY_LIKELY(!first.m_it.primitive_iterator.is_begin()
                                       || !last.m_it.primitive_iterator.is_end()))
                {
                    JSON_THROW(invalid_iterator::create(204, "iterators out of range", this));
                }

                if (is_string())
                {
                    AllocatorType<string_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.string);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.string, 1);
                    m_data.m_value.string = nullptr;
                }
                else if (is_binary())
                {
                    AllocatorType<binary_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, m_data.m_value.binary);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_data.m_value.binary, 1);
                    m_data.m_value.binary = nullptr;
                }

                m_data.m_type = value_t::null;
                assert_invariant();
                break;
            }

            case value_t::object:
            {
                result.m_it.object_iterator = m_data.m_value.object->erase(first.m_it.object_iterator,
                                              last.m_it.object_iterator);
                break;
            }

            case value_t::array:
            {
                result.m_it.array_iterator = m_data.m_value.array->erase(first.m_it.array_iterator,
                                             last.m_it.array_iterator);
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        return result;
    }

  private:
    template < typename KeyType, detail::enable_if_t <
                   detail::has_erase_with_key_type<basic_json_t, KeyType>::value, int > = 0 >
    size_type erase_internal(KeyType && key)
    {
        // this erase only works for objects
        if (JSON_HEDLEY_UNLIKELY(!is_object()))
        {
            JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        return m_data.m_value.object->erase(std::forward<KeyType>(key));
    }

    template < typename KeyType, detail::enable_if_t <
                   !detail::has_erase_with_key_type<basic_json_t, KeyType>::value, int > = 0 >
    size_type erase_internal(KeyType && key)
    {
        // this erase only works for objects
        if (JSON_HEDLEY_UNLIKELY(!is_object()))
        {
            JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        const auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
        if (it != m_data.m_value.object->end())
        {
            m_data.m_value.object->erase(it);
            return 1;
        }
        return 0;
    }

  public:

    /// @brief remove element from a JSON object given a key
    /// @sa https://json.nlohmann.me/api/basic_json/erase/
    size_type erase(const typename object_t::key_type& key)
    {
        // the indirection via erase_internal() is added to avoid making this
        // function a template and thus de-rank it during overload resolution
        return erase_internal(key);
    }

    /// @brief remove element from a JSON object given a key
    /// @sa https://json.nlohmann.me/api/basic_json/erase/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    size_type erase(KeyType && key)
    {
        return erase_internal(std::forward<KeyType>(key));
    }

    /// @brief remove element from a JSON array given an index
    /// @sa https://json.nlohmann.me/api/basic_json/erase/
    void erase(const size_type idx)
    {
        // this erase only works for arrays
        if (JSON_HEDLEY_LIKELY(is_array()))
        {
            if (JSON_HEDLEY_UNLIKELY(idx >= size()))
            {
                JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), this));
            }

            m_data.m_value.array->erase(m_data.m_value.array->begin() + static_cast<difference_type>(idx));
        }
        else
        {
            JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }
    }

    /// @}

    ////////////
    // lookup //
    ////////////

    /// @name lookup
    /// @{

    /// @brief find an element in a JSON object
    /// @sa https://json.nlohmann.me/api/basic_json/find/
    iterator find(const typename object_t::key_type& key)
    {
        auto result = end();

        if (is_object())
        {
            result.m_it.object_iterator = m_data.m_value.object->find(key);
        }

        return result;
    }

    /// @brief find an element in a JSON object
    /// @sa https://json.nlohmann.me/api/basic_json/find/
    const_iterator find(const typename object_t::key_type& key) const
    {
        auto result = cend();

        if (is_object())
        {
            result.m_it.object_iterator = m_data.m_value.object->find(key);
        }

        return result;
    }

    /// @brief find an element in a JSON object
    /// @sa https://json.nlohmann.me/api/basic_json/find/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    iterator find(KeyType && key)
    {
        auto result = end();

        if (is_object())
        {
            result.m_it.object_iterator = m_data.m_value.object->find(std::forward<KeyType>(key));
        }

        return result;
    }

    /// @brief find an element in a JSON object
    /// @sa https://json.nlohmann.me/api/basic_json/find/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    const_iterator find(KeyType && key) const
    {
        auto result = cend();

        if (is_object())
        {
            result.m_it.object_iterator = m_data.m_value.object->find(std::forward<KeyType>(key));
        }

        return result;
    }

    /// @brief returns the number of occurrences of a key in a JSON object
    /// @sa https://json.nlohmann.me/api/basic_json/count/
    size_type count(const typename object_t::key_type& key) const
    {
        // return 0 for all nonobject types
        return is_object() ? m_data.m_value.object->count(key) : 0;
    }

    /// @brief returns the number of occurrences of a key in a JSON object
    /// @sa https://json.nlohmann.me/api/basic_json/count/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    size_type count(KeyType && key) const
    {
        // return 0 for all nonobject types
        return is_object() ? m_data.m_value.object->count(std::forward<KeyType>(key)) : 0;
    }

    /// @brief check the existence of an element in a JSON object
    /// @sa https://json.nlohmann.me/api/basic_json/contains/
    bool contains(const typename object_t::key_type& key) const
    {
        return is_object() && m_data.m_value.object->find(key) != m_data.m_value.object->end();
    }

    /// @brief check the existence of an element in a JSON object
    /// @sa https://json.nlohmann.me/api/basic_json/contains/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    bool contains(KeyType && key) const
    {
        return is_object() && m_data.m_value.object->find(std::forward<KeyType>(key)) != m_data.m_value.object->end();
    }

    /// @brief check the existence of an element in a JSON object given a JSON pointer
    /// @sa https://json.nlohmann.me/api/basic_json/contains/
    bool contains(const json_pointer& ptr) const
    {
        return ptr.contains(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    bool contains(const typename ::nlohmann::json_pointer<BasicJsonType>& ptr) const
    {
        return ptr.contains(this);
    }

    /// @}

    ///////////////
    // iterators //
    ///////////////

    /// @name iterators
    /// @{

    /// @brief returns an iterator to the first element
    /// @sa https://json.nlohmann.me/api/basic_json/begin/
    iterator begin() noexcept
    {
        iterator result(this);
        result.set_begin();
        return result;
    }

    /// @brief returns an iterator to the first element
    /// @sa https://json.nlohmann.me/api/basic_json/begin/
    const_iterator begin() const noexcept
    {
        return cbegin();
    }

    /// @brief returns a const iterator to the first element
    /// @sa https://json.nlohmann.me/api/basic_json/cbegin/
    const_iterator cbegin() const noexcept
    {
        const_iterator result(this);
        result.set_begin();
        return result;
    }

    /// @brief returns an iterator to one past the last element
    /// @sa https://json.nlohmann.me/api/basic_json/end/
    iterator end() noexcept
    {
        iterator result(this);
        result.set_end();
        return result;
    }

    /// @brief returns an iterator to one past the last element
    /// @sa https://json.nlohmann.me/api/basic_json/end/
    const_iterator end() const noexcept
    {
        return cend();
    }

    /// @brief returns an iterator to one past the last element
    /// @sa https://json.nlohmann.me/api/basic_json/cend/
    const_iterator cend() const noexcept
    {
        const_iterator result(this);
        result.set_end();
        return result;
    }

    /// @brief returns an iterator to the reverse-beginning
    /// @sa https://json.nlohmann.me/api/basic_json/rbegin/
    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    /// @brief returns an iterator to the reverse-beginning
    /// @sa https://json.nlohmann.me/api/basic_json/rbegin/
    const_reverse_iterator rbegin() const noexcept
    {
        return crbegin();
    }

    /// @brief returns an iterator to the reverse-end
    /// @sa https://json.nlohmann.me/api/basic_json/rend/
    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    /// @brief returns an iterator to the reverse-end
    /// @sa https://json.nlohmann.me/api/basic_json/rend/
    const_reverse_iterator rend() const noexcept
    {
        return crend();
    }

    /// @brief returns a const reverse iterator to the last element
    /// @sa https://json.nlohmann.me/api/basic_json/crbegin/
    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    /// @brief returns a const reverse iterator to one before the first
    /// @sa https://json.nlohmann.me/api/basic_json/crend/
    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

  public:
    /// @brief wrapper to access iterator member functions in range-based for
    /// @sa https://json.nlohmann.me/api/basic_json/items/
    /// @deprecated This function is deprecated since 3.1.0 and will be removed in
    ///             version 4.0.0 of the library. Please use @ref items() instead;
    ///             that is, replace `json::iterator_wrapper(j)` with `j.items()`.
    JSON_HEDLEY_DEPRECATED_FOR(3.1.0, items())
    static iteration_proxy<iterator> iterator_wrapper(reference ref) noexcept
    {
        return ref.items();
    }

    /// @brief wrapper to access iterator member functions in range-based for
    /// @sa https://json.nlohmann.me/api/basic_json/items/
    /// @deprecated This function is deprecated since 3.1.0 and will be removed in
    ///         version 4.0.0 of the library. Please use @ref items() instead;
    ///         that is, replace `json::iterator_wrapper(j)` with `j.items()`.
    JSON_HEDLEY_DEPRECATED_FOR(3.1.0, items())
    static iteration_proxy<const_iterator> iterator_wrapper(const_reference ref) noexcept
    {
        return ref.items();
    }

    /// @brief helper to access iterator member functions in range-based for
    /// @sa https://json.nlohmann.me/api/basic_json/items/
    iteration_proxy<iterator> items() noexcept
    {
        return iteration_proxy<iterator>(*this);
    }

    /// @brief helper to access iterator member functions in range-based for
    /// @sa https://json.nlohmann.me/api/basic_json/items/
    iteration_proxy<const_iterator> items() const noexcept
    {
        return iteration_proxy<const_iterator>(*this);
    }

    /// @}

    //////////////
    // capacity //
    //////////////

    /// @name capacity
    /// @{

    /// @brief checks whether the container is empty.
    /// @sa https://json.nlohmann.me/api/basic_json/empty/
    bool empty() const noexcept
    {
        switch (m_data.m_type)
        {
            case value_t::null:
            {
                // null values are empty
                return true;
            }

            case value_t::array:
            {
                // delegate call to array_t::empty()
                return m_data.m_value.array->empty();
            }

            case value_t::object:
            {
                // delegate call to object_t::empty()
                return m_data.m_value.object->empty();
            }

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
            {
                // all other types are nonempty
                return false;
            }
        }
    }

    /// @brief returns the number of elements
    /// @sa https://json.nlohmann.me/api/basic_json/size/
    size_type size() const noexcept
    {
        switch (m_data.m_type)
        {
            case value_t::null:
            {
                // null values are empty
                return 0;
            }

            case value_t::array:
            {
                // delegate call to array_t::size()
                return m_data.m_value.array->size();
            }

            case value_t::object:
            {
                // delegate call to object_t::size()
                return m_data.m_value.object->size();
            }

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
            {
                // all other types have size 1
                return 1;
            }
        }
    }

    /// @brief returns the maximum possible number of elements
    /// @sa https://json.nlohmann.me/api/basic_json/max_size/
    size_type max_size() const noexcept
    {
        switch (m_data.m_type)
        {
            case value_t::array:
            {
                // delegate call to array_t::max_size()
                return m_data.m_value.array->max_size();
            }

            case value_t::object:
            {
                // delegate call to object_t::max_size()
                return m_data.m_value.object->max_size();
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
            {
                // all other types have max_size() == size()
                return size();
            }
        }
    }

    /// @}

    ///////////////
    // modifiers //
    ///////////////

    /// @name modifiers
    /// @{

    /// @brief clears the contents
    /// @sa https://json.nlohmann.me/api/basic_json/clear/
    void clear() noexcept
    {
        switch (m_data.m_type)
        {
            case value_t::number_integer:
            {
                m_data.m_value.number_integer = 0;
                break;
            }

            case value_t::number_unsigned:
            {
                m_data.m_value.number_unsigned = 0;
                break;
            }

            case value_t::number_float:
            {
                m_data.m_value.number_float = 0.0;
                break;
            }

            case value_t::boolean:
            {
                m_data.m_value.boolean = false;
                break;
            }

            case value_t::string:
            {
                m_data.m_value.string->clear();
                break;
            }

            case value_t::binary:
            {
                m_data.m_value.binary->clear();
                break;
            }

            case value_t::array:
            {
                m_data.m_value.array->clear();
                break;
            }

            case value_t::object:
            {
                m_data.m_value.object->clear();
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                break;
        }
    }

    /// @brief add an object to an array
    /// @sa https://json.nlohmann.me/api/basic_json/push_back/
    void push_back(basic_json&& val)
    {
        // push_back only works for null objects or arrays
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || is_array())))
        {
            JSON_THROW(type_error::create(308, detail::concat("cannot use push_back() with ", type_name()), this));
        }

        // transform null object into an array
        if (is_null())
        {
            m_data.m_type = value_t::array;
            m_data.m_value = value_t::array;
            assert_invariant();
        }

        // add element to array (move semantics)
        const auto old_capacity = m_data.m_value.array->capacity();
        m_data.m_value.array->push_back(std::move(val));
        set_parent(m_data.m_value.array->back(), old_capacity);
        // if val is moved from, basic_json move constructor marks it null, so we do not call the destructor
    }

    /// @brief add an object to an array
    /// @sa https://json.nlohmann.me/api/basic_json/operator+=/
    reference operator+=(basic_json&& val)
    {
        push_back(std::move(val));
        return *this;
    }

    /// @brief add an object to an array
    /// @sa https://json.nlohmann.me/api/basic_json/push_back/
    void push_back(const basic_json& val)
    {
        // push_back only works for null objects or arrays
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || is_array())))
        {
            JSON_THROW(type_error::create(308, detail::concat("cannot use push_back() with ", type_name()), this));
        }

        // transform null object into an array
        if (is_null())
        {
            m_data.m_type = value_t::array;
            m_data.m_value = value_t::array;
            assert_invariant();
        }

        // add element to array
        const auto old_capacity = m_data.m_value.array->capacity();
        m_data.m_value.array->push_back(val);
        set_parent(m_data.m_value.array->back(), old_capacity);
    }

    /// @brief add an object to an array
    /// @sa https://json.nlohmann.me/api/basic_json/operator+=/
    reference operator+=(const basic_json& val)
    {
        push_back(val);
        return *this;
    }

    /// @brief add an object to an object
    /// @sa https://json.nlohmann.me/api/basic_json/push_back/
    void push_back(const typename object_t::value_type& val)
    {
        // push_back only works for null objects or objects
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || is_object())))
        {
            JSON_THROW(type_error::create(308, detail::concat("cannot use push_back() with ", type_name()), this));
        }

        // transform null object into an object
        if (is_null())
        {
            m_data.m_type = value_t::object;
            m_data.m_value = value_t::object;
            assert_invariant();
        }

        // add element to object
        auto res = m_data.m_value.object->insert(val);
        set_parent(res.first->second);
    }

    /// @brief add an object to an object
    /// @sa https://json.nlohmann.me/api/basic_json/operator+=/
    reference operator+=(const typename object_t::value_type& val)
    {
        push_back(val);
        return *this;
    }

    /// @brief add an object to an object
    /// @sa https://json.nlohmann.me/api/basic_json/push_back/
    void push_back(initializer_list_t init)
    {
        if (is_object() && init.size() == 2 && (*init.begin())->is_string())
        {
            basic_json&& key = init.begin()->moved_or_copied();
            push_back(typename object_t::value_type(
                          std::move(key.get_ref<string_t&>()), (init.begin() + 1)->moved_or_copied()));
        }
        else
        {
            push_back(basic_json(init));
        }
    }

    /// @brief add an object to an object
    /// @sa https://json.nlohmann.me/api/basic_json/operator+=/
    reference operator+=(initializer_list_t init)
    {
        push_back(init);
        return *this;
    }

    /// @brief add an object to an array
    /// @sa https://json.nlohmann.me/api/basic_json/emplace_back/
    template<class... Args>
    reference emplace_back(Args&& ... args)
    {
        // emplace_back only works for null objects or arrays
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || is_array())))
        {
            JSON_THROW(type_error::create(311, detail::concat("cannot use emplace_back() with ", type_name()), this));
        }

        // transform null object into an array
        if (is_null())
        {
            m_data.m_type = value_t::array;
            m_data.m_value = value_t::array;
            assert_invariant();
        }

        // add element to array (perfect forwarding)
        const auto old_capacity = m_data.m_value.array->capacity();
        m_data.m_value.array->emplace_back(std::forward<Args>(args)...);
        return set_parent(m_data.m_value.array->back(), old_capacity);
    }

    /// @brief add an object to an object if key does not exist
    /// @sa https://json.nlohmann.me/api/basic_json/emplace/
    template<class... Args>
    std::pair<iterator, bool> emplace(Args&& ... args)
    {
        // emplace only works for null objects or arrays
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || is_object())))
        {
            JSON_THROW(type_error::create(311, detail::concat("cannot use emplace() with ", type_name()), this));
        }

        // transform null object into an object
        if (is_null())
        {
            m_data.m_type = value_t::object;
            m_data.m_value = value_t::object;
            assert_invariant();
        }

        // add element to array (perfect forwarding)
        auto res = m_data.m_value.object->emplace(std::forward<Args>(args)...);
        set_parent(res.first->second);

        // create result iterator and set iterator to the result of emplace
        auto it = begin();
        it.m_it.object_iterator = res.first;

        // return pair of iterator and boolean
        return {it, res.second};
    }

    /// Helper for insertion of an iterator
    /// @note: This uses std::distance to support GCC 4.8,
    ///        see https://github.com/nlohmann/json/pull/1257
    template<typename... Args>
    iterator insert_iterator(const_iterator pos, Args&& ... args)
    {
        iterator result(this);
        JSON_ASSERT(m_data.m_value.array != nullptr);

        auto insert_pos = std::distance(m_data.m_value.array->begin(), pos.m_it.array_iterator);
        m_data.m_value.array->insert(pos.m_it.array_iterator, std::forward<Args>(args)...);
        result.m_it.array_iterator = m_data.m_value.array->begin() + insert_pos;

        // This could have been written as:
        // result.m_it.array_iterator = m_data.m_value.array->insert(pos.m_it.array_iterator, cnt, val);
        // but the return value of insert is missing in GCC 4.8, so it is written this way instead.

        set_parents();
        return result;
    }

    /// @brief inserts element into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, const basic_json& val)
    {
        // insert only works for arrays
        if (JSON_HEDLEY_LIKELY(is_array()))
        {
            // check if iterator pos fits to this JSON value
            if (JSON_HEDLEY_UNLIKELY(pos.m_object != this))
            {
                JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
            }

            // insert to array and return iterator
            return insert_iterator(pos, val);
        }

        JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
    }

    /// @brief inserts element into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, basic_json&& val)
    {
        return insert(pos, val);
    }

    /// @brief inserts copies of element into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, size_type cnt, const basic_json& val)
    {
        // insert only works for arrays
        if (JSON_HEDLEY_LIKELY(is_array()))
        {
            // check if iterator pos fits to this JSON value
            if (JSON_HEDLEY_UNLIKELY(pos.m_object != this))
            {
                JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
            }

            // insert to array and return iterator
            return insert_iterator(pos, cnt, val);
        }

        JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
    }

    /// @brief inserts range of elements into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, const_iterator first, const_iterator last)
    {
        // insert only works for arrays
        if (JSON_HEDLEY_UNLIKELY(!is_array()))
        {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }

        // check if iterator pos fits to this JSON value
        if (JSON_HEDLEY_UNLIKELY(pos.m_object != this))
        {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }

        // check if range iterators belong to the same JSON object
        if (JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }

        if (JSON_HEDLEY_UNLIKELY(first.m_object == this))
        {
            JSON_THROW(invalid_iterator::create(211, "passed iterators may not belong to container", this));
        }

        // insert to array and return iterator
        return insert_iterator(pos, first.m_it.array_iterator, last.m_it.array_iterator);
    }

    /// @brief inserts elements from initializer list into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, initializer_list_t ilist)
    {
        // insert only works for arrays
        if (JSON_HEDLEY_UNLIKELY(!is_array()))
        {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }

        // check if iterator pos fits to this JSON value
        if (JSON_HEDLEY_UNLIKELY(pos.m_object != this))
        {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }

        // insert to array and return iterator
        return insert_iterator(pos, ilist.begin(), ilist.end());
    }

    /// @brief inserts range of elements into object
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    void insert(const_iterator first, const_iterator last)
    {
        // insert only works for objects
        if (JSON_HEDLEY_UNLIKELY(!is_object()))
        {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }

        // check if range iterators belong to the same JSON object
        if (JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }

        // passed iterators must belong to objects
        if (JSON_HEDLEY_UNLIKELY(!first.m_object->is_object()))
        {
            JSON_THROW(invalid_iterator::create(202, "iterators first and last must point to objects", this));
        }

        m_data.m_value.object->insert(first.m_it.object_iterator, last.m_it.object_iterator);
    }

    /// @brief updates a JSON object from another object, overwriting existing keys
    /// @sa https://json.nlohmann.me/api/basic_json/update/
    void update(const_reference j, bool merge_objects = false)
    {
        update(j.begin(), j.end(), merge_objects);
    }

    /// @brief updates a JSON object from another object, overwriting existing keys
    /// @sa https://json.nlohmann.me/api/basic_json/update/
    void update(const_iterator first, const_iterator last, bool merge_objects = false)
    {
        // implicitly convert null value to an empty object
        if (is_null())
        {
            m_data.m_type = value_t::object;
            m_data.m_value.object = create<object_t>();
            assert_invariant();
        }

        if (JSON_HEDLEY_UNLIKELY(!is_object()))
        {
            JSON_THROW(type_error::create(312, detail::concat("cannot use update() with ", type_name()), this));
        }

        // check if range iterators belong to the same JSON object
        if (JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }

        // passed iterators must belong to objects
        if (JSON_HEDLEY_UNLIKELY(!first.m_object->is_object()))
        {
            JSON_THROW(type_error::create(312, detail::concat("cannot use update() with ", first.m_object->type_name()), first.m_object));
        }

        for (auto it = first; it != last; ++it)
        {
            if (merge_objects && it.value().is_object())
            {
                auto it2 = m_data.m_value.object->find(it.key());
                if (it2 != m_data.m_value.object->end())
                {
                    it2->second.update(it.value(), true);
                    continue;
                }
            }
            m_data.m_value.object->operator[](it.key()) = it.value();
#if JSON_DIAGNOSTICS
            m_data.m_value.object->operator[](it.key()).m_parent = this;
#endif
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(reference other) noexcept (
        std::is_nothrow_move_constructible<value_t>::value&&
        std::is_nothrow_move_assignable<value_t>::value&&
        std::is_nothrow_move_constructible<json_value>::value&& // NOLINT(cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
        std::is_nothrow_move_assignable<json_value>::value
    )
    {
        std::swap(m_data.m_type, other.m_data.m_type);
        std::swap(m_data.m_value, other.m_data.m_value);

        set_parents();
        other.set_parents();
        assert_invariant();
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    friend void swap(reference left, reference right) noexcept (
        std::is_nothrow_move_constructible<value_t>::value&&
        std::is_nothrow_move_assignable<value_t>::value&&
        std::is_nothrow_move_constructible<json_value>::value&& // NOLINT(cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
        std::is_nothrow_move_assignable<json_value>::value
    )
    {
        left.swap(right);
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(array_t& other) // NOLINT(bugprone-exception-escape,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    {
        // swap only works for arrays
        if (JSON_HEDLEY_LIKELY(is_array()))
        {
            using std::swap;
            swap(*(m_data.m_value.array), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(array_t&) with ", type_name()), this));
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(object_t& other) // NOLINT(bugprone-exception-escape,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    {
        // swap only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            using std::swap;
            swap(*(m_data.m_value.object), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(object_t&) with ", type_name()), this));
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(string_t& other) // NOLINT(bugprone-exception-escape,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    {
        // swap only works for strings
        if (JSON_HEDLEY_LIKELY(is_string()))
        {
            using std::swap;
            swap(*(m_data.m_value.string), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(string_t&) with ", type_name()), this));
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(binary_t& other) // NOLINT(bugprone-exception-escape,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    {
        // swap only works for strings
        if (JSON_HEDLEY_LIKELY(is_binary()))
        {
            using std::swap;
            swap(*(m_data.m_value.binary), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(binary_t&) with ", type_name()), this));
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(typename binary_t::container_type& other) // NOLINT(bugprone-exception-escape)
    {
        // swap only works for strings
        if (JSON_HEDLEY_LIKELY(is_binary()))
        {
            using std::swap;
            swap(*(m_data.m_value.binary), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(binary_t::container_type&) with ", type_name()), this));
        }
    }

    /// @}

    //////////////////////////////////////////
    // lexicographical comparison operators //
    //////////////////////////////////////////

    /// @name lexicographical comparison operators
    /// @{

    // note parentheses around operands are necessary; see
    // https://github.com/nlohmann/json/issues/1530
#define JSON_IMPLEMENT_OPERATOR(op, null_result, unordered_result, default_result)                       \
    const auto lhs_type = lhs.type();                                                                    \
    const auto rhs_type = rhs.type();                                                                    \
    \
    if (lhs_type == rhs_type) /* NOLINT(readability/braces) */                                           \
    {                                                                                                    \
        switch (lhs_type)                                                                                \
        {                                                                                                \
            case value_t::array:                                                                         \
                return (*lhs.m_data.m_value.array) op (*rhs.m_data.m_value.array);                                     \
                \
            case value_t::object:                                                                        \
                return (*lhs.m_data.m_value.object) op (*rhs.m_data.m_value.object);                                   \
                \
            case value_t::null:                                                                          \
                return (null_result);                                                                    \
                \
            case value_t::string:                                                                        \
                return (*lhs.m_data.m_value.string) op (*rhs.m_data.m_value.string);                                   \
                \
            case value_t::boolean:                                                                       \
                return (lhs.m_data.m_value.boolean) op (rhs.m_data.m_value.boolean);                                   \
                \
            case value_t::number_integer:                                                                \
                return (lhs.m_data.m_value.number_integer) op (rhs.m_data.m_value.number_integer);                     \
                \
            case value_t::number_unsigned:                                                               \
                return (lhs.m_data.m_value.number_unsigned) op (rhs.m_data.m_value.number_unsigned);                   \
                \
            case value_t::number_float:                                                                  \
                return (lhs.m_data.m_value.number_float) op (rhs.m_data.m_value.number_float);                         \
                \
            case value_t::binary:                                                                        \
                return (*lhs.m_data.m_value.binary) op (*rhs.m_data.m_value.binary);                                   \
                \
            case value_t::discarded:                                                                     \
            default:                                                                                     \
                return (unordered_result);                                                               \
        }                                                                                                \
    }                                                                                                    \
    else if (lhs_type == value_t::number_integer && rhs_type == value_t::number_float)                   \
    {                                                                                                    \
        return static_cast<number_float_t>(lhs.m_data.m_value.number_integer) op rhs.m_data.m_value.number_float;      \
    }                                                                                                    \
    else if (lhs_type == value_t::number_float && rhs_type == value_t::number_integer)                   \
    {                                                                                                    \
        return lhs.m_data.m_value.number_float op static_cast<number_float_t>(rhs.m_data.m_value.number_integer);      \
    }                                                                                                    \
    else if (lhs_type == value_t::number_unsigned && rhs_type == value_t::number_float)                  \
    {                                                                                                    \
        return static_cast<number_float_t>(lhs.m_data.m_value.number_unsigned) op rhs.m_data.m_value.number_float;     \
    }                                                                                                    \
    else if (lhs_type == value_t::number_float && rhs_type == value_t::number_unsigned)                  \
    {                                                                                                    \
        return lhs.m_data.m_value.number_float op static_cast<number_float_t>(rhs.m_data.m_value.number_unsigned);     \
    }                                                                                                    \
    else if (lhs_type == value_t::number_unsigned && rhs_type == value_t::number_integer)                \
    {                                                                                                    \
        return static_cast<number_integer_t>(lhs.m_data.m_value.number_unsigned) op rhs.m_data.m_value.number_integer; \
    }                                                                                                    \
    else if (lhs_type == value_t::number_integer && rhs_type == value_t::number_unsigned)                \
    {                                                                                                    \
        return lhs.m_data.m_value.number_integer op static_cast<number_integer_t>(rhs.m_data.m_value.number_unsigned); \
    }                                                                                                    \
    else if(compares_unordered(lhs, rhs))\
    {\
        return (unordered_result);\
    }\
    \
    return (default_result);

  JSON_PRIVATE_UNLESS_TESTED:
    // returns true if:
    // - any operand is NaN and the other operand is of number type
    // - any operand is discarded
    // in legacy mode, discarded values are considered ordered if
    // an operation is computed as an odd number of inverses of others
    static bool compares_unordered(const_reference lhs, const_reference rhs, bool inverse = false) noexcept
    {
        if ((lhs.is_number_float() && std::isnan(lhs.m_data.m_value.number_float) && rhs.is_number())
                || (rhs.is_number_float() && std::isnan(rhs.m_data.m_value.number_float) && lhs.is_number()))
        {
            return true;
        }
#if JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
        return (lhs.is_discarded() || rhs.is_discarded()) && !inverse;
#else
        static_cast<void>(inverse);
        return lhs.is_discarded() || rhs.is_discarded();
#endif
    }

  private:
    bool compares_unordered(const_reference rhs, bool inverse = false) const noexcept
    {
        return compares_unordered(*this, rhs, inverse);
    }

  public:
#if JSON_HAS_THREE_WAY_COMPARISON
    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    bool operator==(const_reference rhs) const noexcept
    {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
        const_reference lhs = *this;
        JSON_IMPLEMENT_OPERATOR( ==, true, false, false)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }

    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    template<typename ScalarType>
    requires std::is_scalar_v<ScalarType>
    bool operator==(ScalarType rhs) const noexcept
    {
        return *this == basic_json(rhs);
    }

    /// @brief comparison: not equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ne/
    bool operator!=(const_reference rhs) const noexcept
    {
        if (compares_unordered(rhs, true))
        {
            return false;
        }
        return !operator==(rhs);
    }

    /// @brief comparison: 3-way
    /// @sa https://json.nlohmann.me/api/basic_json/operator_spaceship/
    std::partial_ordering operator<=>(const_reference rhs) const noexcept // *NOPAD*
    {
        const_reference lhs = *this;
        // default_result is used if we cannot compare values. In that case,
        // we compare types.
        JSON_IMPLEMENT_OPERATOR(<=>, // *NOPAD*
                                std::partial_ordering::equivalent,
                                std::partial_ordering::unordered,
                                lhs_type <=> rhs_type) // *NOPAD*
    }

    /// @brief comparison: 3-way
    /// @sa https://json.nlohmann.me/api/basic_json/operator_spaceship/
    template<typename ScalarType>
    requires std::is_scalar_v<ScalarType>
    std::partial_ordering operator<=>(ScalarType rhs) const noexcept // *NOPAD*
    {
        return *this <=> basic_json(rhs); // *NOPAD*
    }

#if JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
    // all operators that are computed as an odd number of inverses of others
    // need to be overloaded to emulate the legacy comparison behavior

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, undef JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON)
    bool operator<=(const_reference rhs) const noexcept
    {
        if (compares_unordered(rhs, true))
        {
            return false;
        }
        return !(rhs < *this);
    }

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    template<typename ScalarType>
    requires std::is_scalar_v<ScalarType>
    bool operator<=(ScalarType rhs) const noexcept
    {
        return *this <= basic_json(rhs);
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, undef JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON)
    bool operator>=(const_reference rhs) const noexcept
    {
        if (compares_unordered(rhs, true))
        {
            return false;
        }
        return !(*this < rhs);
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    template<typename ScalarType>
    requires std::is_scalar_v<ScalarType>
    bool operator>=(ScalarType rhs) const noexcept
    {
        return *this >= basic_json(rhs);
    }
#endif
#else
    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    friend bool operator==(const_reference lhs, const_reference rhs) noexcept
    {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
        JSON_IMPLEMENT_OPERATOR( ==, true, false, false)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }

    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator==(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs == basic_json(rhs);
    }

    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator==(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) == rhs;
    }

    /// @brief comparison: not equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ne/
    friend bool operator!=(const_reference lhs, const_reference rhs) noexcept
    {
        if (compares_unordered(lhs, rhs, true))
        {
            return false;
        }
        return !(lhs == rhs);
    }

    /// @brief comparison: not equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ne/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator!=(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs != basic_json(rhs);
    }

    /// @brief comparison: not equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ne/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator!=(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) != rhs;
    }

    /// @brief comparison: less than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_lt/
    friend bool operator<(const_reference lhs, const_reference rhs) noexcept
    {
        // default_result is used if we cannot compare values. In that case,
        // we compare types. Note we have to call the operator explicitly,
        // because MSVC has problems otherwise.
        JSON_IMPLEMENT_OPERATOR( <, false, false, operator<(lhs_type, rhs_type))
    }

    /// @brief comparison: less than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_lt/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs < basic_json(rhs);
    }

    /// @brief comparison: less than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_lt/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) < rhs;
    }

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    friend bool operator<=(const_reference lhs, const_reference rhs) noexcept
    {
        if (compares_unordered(lhs, rhs, true))
        {
            return false;
        }
        return !(rhs < lhs);
    }

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<=(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs <= basic_json(rhs);
    }

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<=(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) <= rhs;
    }

    /// @brief comparison: greater than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gt/
    friend bool operator>(const_reference lhs, const_reference rhs) noexcept
    {
        // double inverse
        if (compares_unordered(lhs, rhs))
        {
            return false;
        }
        return !(lhs <= rhs);
    }

    /// @brief comparison: greater than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gt/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs > basic_json(rhs);
    }

    /// @brief comparison: greater than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gt/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) > rhs;
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    friend bool operator>=(const_reference lhs, const_reference rhs) noexcept
    {
        if (compares_unordered(lhs, rhs, true))
        {
            return false;
        }
        return !(lhs < rhs);
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>=(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs >= basic_json(rhs);
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>=(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) >= rhs;
    }
#endif

#undef JSON_IMPLEMENT_OPERATOR

    /// @}

    ///////////////////
    // serialization //
    ///////////////////

    /// @name serialization
    /// @{
#ifndef JSON_NO_IO
    /// @brief serialize to stream
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ltlt/
    friend std::ostream& operator<<(std::ostream& o, const basic_json& j)
    {
        // read width member and use it as indentation parameter if nonzero
        const bool pretty_print = o.width() > 0;
        const auto indentation = pretty_print ? o.width() : 0;

        // reset width to 0 for subsequent calls to this stream
        o.width(0);

        // do the actual serialization
        serializer s(detail::output_adapter<char>(o), o.fill());
        s.dump(j, pretty_print, false, static_cast<unsigned int>(indentation));
        return o;
    }

    /// @brief serialize to stream
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ltlt/
    /// @deprecated This function is deprecated since 3.0.0 and will be removed in
    ///             version 4.0.0 of the library. Please use
    ///             operator<<(std::ostream&, const basic_json&) instead; that is,
    ///             replace calls like `j >> o;` with `o << j;`.
    JSON_HEDLEY_DEPRECATED_FOR(3.0.0, operator<<(std::ostream&, const basic_json&))
    friend std::ostream& operator>>(const basic_json& j, std::ostream& o)
    {
        return o << j;
    }
#endif  // JSON_NO_IO
    /// @}

    /////////////////////
    // deserialization //
    /////////////////////

    /// @name deserialization
    /// @{

    /// @brief deserialize from a compatible input
    /// @sa https://json.nlohmann.me/api/basic_json/parse/
    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json parse(InputType&& i,
                            const parser_callback_t cb = nullptr,
                            const bool allow_exceptions = true,
                            const bool ignore_comments = false)
    {
        basic_json result;
        parser(detail::input_adapter(std::forward<InputType>(i)), cb, allow_exceptions, ignore_comments).parse(true, result);
        return result;
    }

    /// @brief deserialize from a pair of character iterators
    /// @sa https://json.nlohmann.me/api/basic_json/parse/
    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json parse(IteratorType first,
                            IteratorType last,
                            const parser_callback_t cb = nullptr,
                            const bool allow_exceptions = true,
                            const bool ignore_comments = false)
    {
        basic_json result;
        parser(detail::input_adapter(std::move(first), std::move(last)), cb, allow_exceptions, ignore_comments).parse(true, result);
        return result;
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, parse(ptr, ptr + len))
    static basic_json parse(detail::span_input_adapter&& i,
                            const parser_callback_t cb = nullptr,
                            const bool allow_exceptions = true,
                            const bool ignore_comments = false)
    {
        basic_json result;
        parser(i.get(), cb, allow_exceptions, ignore_comments).parse(true, result);
        return result;
    }

    /// @brief check if the input is valid JSON
    /// @sa https://json.nlohmann.me/api/basic_json/accept/
    template<typename InputType>
    static bool accept(InputType&& i,
                       const bool ignore_comments = false)
    {
        return parser(detail::input_adapter(std::forward<InputType>(i)), nullptr, false, ignore_comments).accept(true);
    }

    /// @brief check if the input is valid JSON
    /// @sa https://json.nlohmann.me/api/basic_json/accept/
    template<typename IteratorType>
    static bool accept(IteratorType first, IteratorType last,
                       const bool ignore_comments = false)
    {
        return parser(detail::input_adapter(std::move(first), std::move(last)), nullptr, false, ignore_comments).accept(true);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, accept(ptr, ptr + len))
    static bool accept(detail::span_input_adapter&& i,
                       const bool ignore_comments = false)
    {
        return parser(i.get(), nullptr, false, ignore_comments).accept(true);
    }

    /// @brief generate SAX events
    /// @sa https://json.nlohmann.me/api/basic_json/sax_parse/
    template <typename InputType, typename SAX>
    JSON_HEDLEY_NON_NULL(2)
    static bool sax_parse(InputType&& i, SAX* sax,
                          input_format_t format = input_format_t::json,
                          const bool strict = true,
                          const bool ignore_comments = false)
    {
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        return format == input_format_t::json
               ? parser(std::move(ia), nullptr, true, ignore_comments).sax_parse(sax, strict)
               : detail::binary_reader<basic_json, decltype(ia), SAX>(std::move(ia), format).sax_parse(format, sax, strict);
    }

    /// @brief generate SAX events
    /// @sa https://json.nlohmann.me/api/basic_json/sax_parse/
    template<class IteratorType, class SAX>
    JSON_HEDLEY_NON_NULL(3)
    static bool sax_parse(IteratorType first, IteratorType last, SAX* sax,
                          input_format_t format = input_format_t::json,
                          const bool strict = true,
                          const bool ignore_comments = false)
    {
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        return format == input_format_t::json
               ? parser(std::move(ia), nullptr, true, ignore_comments).sax_parse(sax, strict)
               : detail::binary_reader<basic_json, decltype(ia), SAX>(std::move(ia), format).sax_parse(format, sax, strict);
    }

    /// @brief generate SAX events
    /// @sa https://json.nlohmann.me/api/basic_json/sax_parse/
    /// @deprecated This function is deprecated since 3.8.0 and will be removed in
    ///             version 4.0.0 of the library. Please use
    ///             sax_parse(ptr, ptr + len) instead.
    template <typename SAX>
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, sax_parse(ptr, ptr + len, ...))
    JSON_HEDLEY_NON_NULL(2)
    static bool sax_parse(detail::span_input_adapter&& i, SAX* sax,
                          input_format_t format = input_format_t::json,
                          const bool strict = true,
                          const bool ignore_comments = false)
    {
        auto ia = i.get();
        return format == input_format_t::json
               // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
               ? parser(std::move(ia), nullptr, true, ignore_comments).sax_parse(sax, strict)
               // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
               : detail::binary_reader<basic_json, decltype(ia), SAX>(std::move(ia), format).sax_parse(format, sax, strict);
    }
#ifndef JSON_NO_IO
    /// @brief deserialize from stream
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gtgt/
    /// @deprecated This stream operator is deprecated since 3.0.0 and will be removed in
    ///             version 4.0.0 of the library. Please use
    ///             operator>>(std::istream&, basic_json&) instead; that is,
    ///             replace calls like `j << i;` with `i >> j;`.
    JSON_HEDLEY_DEPRECATED_FOR(3.0.0, operator>>(std::istream&, basic_json&))
    friend std::istream& operator<<(basic_json& j, std::istream& i)
    {
        return operator>>(i, j);
    }

    /// @brief deserialize from stream
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gtgt/
    friend std::istream& operator>>(std::istream& i, basic_json& j)
    {
        parser(detail::input_adapter(i)).parse(false, j);
        return i;
    }
#endif  // JSON_NO_IO
    /// @}

    ///////////////////////////
    // convenience functions //
    ///////////////////////////

    /// @brief return the type as string
    /// @sa https://json.nlohmann.me/api/basic_json/type_name/
    JSON_HEDLEY_RETURNS_NON_NULL
    const char* type_name() const noexcept
    {
        switch (m_data.m_type)
        {
            case value_t::null:
                return "null";
            case value_t::object:
                return "object";
            case value_t::array:
                return "array";
            case value_t::string:
                return "string";
            case value_t::boolean:
                return "boolean";
            case value_t::binary:
                return "binary";
            case value_t::discarded:
                return "discarded";
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            default:
                return "number";
        }
    }

  JSON_PRIVATE_UNLESS_TESTED:
    //////////////////////
    // member variables //
    //////////////////////

    struct data
    {
        /// the type of the current element
        value_t m_type = value_t::null;

        /// the value of the current element
        json_value m_value = {};

        data(const value_t v)
            : m_type(v), m_value(v)
        {
        }

        data(size_type cnt, const basic_json& val)
            : m_type(value_t::array)
        {
            m_value.array = create<array_t>(cnt, val);
        }

        data() noexcept = default;
        data(data&&) noexcept = default;
        data(const data&) noexcept = delete;
        data& operator=(data&&) noexcept = delete;
        data& operator=(const data&) noexcept = delete;

        ~data() noexcept
        {
            m_value.destroy(m_type);
        }
    };

    data m_data = {};

#if JSON_DIAGNOSTICS
    /// a pointer to a parent value (for debugging purposes)
    basic_json* m_parent = nullptr;
#endif

    //////////////////////////////////////////
    // binary serialization/deserialization //
    //////////////////////////////////////////

    /// @name binary serialization/deserialization support
    /// @{

  public:
    /// @brief create a CBOR serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_cbor/
    static std::vector<std::uint8_t> to_cbor(const basic_json& j)
    {
        std::vector<std::uint8_t> result;
        to_cbor(j, result);
        return result;
    }

    /// @brief create a CBOR serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_cbor/
    static void to_cbor(const basic_json& j, detail::output_adapter<std::uint8_t> o)
    {
        binary_writer<std::uint8_t>(o).write_cbor(j);
    }

    /// @brief create a CBOR serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_cbor/
    static void to_cbor(const basic_json& j, detail::output_adapter<char> o)
    {
        binary_writer<char>(o).write_cbor(j);
    }

    /// @brief create a MessagePack serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_msgpack/
    static std::vector<std::uint8_t> to_msgpack(const basic_json& j)
    {
        std::vector<std::uint8_t> result;
        to_msgpack(j, result);
        return result;
    }

    /// @brief create a MessagePack serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_msgpack/
    static void to_msgpack(const basic_json& j, detail::output_adapter<std::uint8_t> o)
    {
        binary_writer<std::uint8_t>(o).write_msgpack(j);
    }

    /// @brief create a MessagePack serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_msgpack/
    static void to_msgpack(const basic_json& j, detail::output_adapter<char> o)
    {
        binary_writer<char>(o).write_msgpack(j);
    }

    /// @brief create a UBJSON serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_ubjson/
    static std::vector<std::uint8_t> to_ubjson(const basic_json& j,
            const bool use_size = false,
            const bool use_type = false)
    {
        std::vector<std::uint8_t> result;
        to_ubjson(j, result, use_size, use_type);
        return result;
    }

    /// @brief create a UBJSON serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_ubjson/
    static void to_ubjson(const basic_json& j, detail::output_adapter<std::uint8_t> o,
                          const bool use_size = false, const bool use_type = false)
    {
        binary_writer<std::uint8_t>(o).write_ubjson(j, use_size, use_type);
    }

    /// @brief create a UBJSON serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_ubjson/
    static void to_ubjson(const basic_json& j, detail::output_adapter<char> o,
                          const bool use_size = false, const bool use_type = false)
    {
        binary_writer<char>(o).write_ubjson(j, use_size, use_type);
    }

    /// @brief create a BJData serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_bjdata/
    static std::vector<std::uint8_t> to_bjdata(const basic_json& j,
            const bool use_size = false,
            const bool use_type = false)
    {
        std::vector<std::uint8_t> result;
        to_bjdata(j, result, use_size, use_type);
        return result;
    }

    /// @brief create a BJData serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_bjdata/
    static void to_bjdata(const basic_json& j, detail::output_adapter<std::uint8_t> o,
                          const bool use_size = false, const bool use_type = false)
    {
        binary_writer<std::uint8_t>(o).write_ubjson(j, use_size, use_type, true, true);
    }

    /// @brief create a BJData serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_bjdata/
    static void to_bjdata(const basic_json& j, detail::output_adapter<char> o,
                          const bool use_size = false, const bool use_type = false)
    {
        binary_writer<char>(o).write_ubjson(j, use_size, use_type, true, true);
    }

    /// @brief create a BSON serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_bson/
    static std::vector<std::uint8_t> to_bson(const basic_json& j)
    {
        std::vector<std::uint8_t> result;
        to_bson(j, result);
        return result;
    }

    /// @brief create a BSON serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_bson/
    static void to_bson(const basic_json& j, detail::output_adapter<std::uint8_t> o)
    {
        binary_writer<std::uint8_t>(o).write_bson(j);
    }

    /// @brief create a BSON serialization of a given JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/to_bson/
    static void to_bson(const basic_json& j, detail::output_adapter<char> o)
    {
        binary_writer<char>(o).write_bson(j);
    }

    /// @brief create a JSON value from an input in CBOR format
    /// @sa https://json.nlohmann.me/api/basic_json/from_cbor/
    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_cbor(InputType&& i,
                                const bool strict = true,
                                const bool allow_exceptions = true,
                                const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::cbor).sax_parse(input_format_t::cbor, &sdp, strict, tag_handler);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in CBOR format
    /// @sa https://json.nlohmann.me/api/basic_json/from_cbor/
    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_cbor(IteratorType first, IteratorType last,
                                const bool strict = true,
                                const bool allow_exceptions = true,
                                const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::cbor).sax_parse(input_format_t::cbor, &sdp, strict, tag_handler);
        return res ? result : basic_json(value_t::discarded);
    }

    template<typename T>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_cbor(ptr, ptr + len))
    static basic_json from_cbor(const T* ptr, std::size_t len,
                                const bool strict = true,
                                const bool allow_exceptions = true,
                                const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error)
    {
        return from_cbor(ptr, ptr + len, strict, allow_exceptions, tag_handler);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_cbor(ptr, ptr + len))
    static basic_json from_cbor(detail::span_input_adapter&& i,
                                const bool strict = true,
                                const bool allow_exceptions = true,
                                const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = i.get();
        // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::cbor).sax_parse(input_format_t::cbor, &sdp, strict, tag_handler);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in MessagePack format
    /// @sa https://json.nlohmann.me/api/basic_json/from_msgpack/
    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_msgpack(InputType&& i,
                                   const bool strict = true,
                                   const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::msgpack).sax_parse(input_format_t::msgpack, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in MessagePack format
    /// @sa https://json.nlohmann.me/api/basic_json/from_msgpack/
    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_msgpack(IteratorType first, IteratorType last,
                                   const bool strict = true,
                                   const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::msgpack).sax_parse(input_format_t::msgpack, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    template<typename T>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_msgpack(ptr, ptr + len))
    static basic_json from_msgpack(const T* ptr, std::size_t len,
                                   const bool strict = true,
                                   const bool allow_exceptions = true)
    {
        return from_msgpack(ptr, ptr + len, strict, allow_exceptions);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_msgpack(ptr, ptr + len))
    static basic_json from_msgpack(detail::span_input_adapter&& i,
                                   const bool strict = true,
                                   const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = i.get();
        // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::msgpack).sax_parse(input_format_t::msgpack, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in UBJSON format
    /// @sa https://json.nlohmann.me/api/basic_json/from_ubjson/
    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_ubjson(InputType&& i,
                                  const bool strict = true,
                                  const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::ubjson).sax_parse(input_format_t::ubjson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in UBJSON format
    /// @sa https://json.nlohmann.me/api/basic_json/from_ubjson/
    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_ubjson(IteratorType first, IteratorType last,
                                  const bool strict = true,
                                  const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::ubjson).sax_parse(input_format_t::ubjson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    template<typename T>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_ubjson(ptr, ptr + len))
    static basic_json from_ubjson(const T* ptr, std::size_t len,
                                  const bool strict = true,
                                  const bool allow_exceptions = true)
    {
        return from_ubjson(ptr, ptr + len, strict, allow_exceptions);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_ubjson(ptr, ptr + len))
    static basic_json from_ubjson(detail::span_input_adapter&& i,
                                  const bool strict = true,
                                  const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = i.get();
        // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::ubjson).sax_parse(input_format_t::ubjson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in BJData format
    /// @sa https://json.nlohmann.me/api/basic_json/from_bjdata/
    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_bjdata(InputType&& i,
                                  const bool strict = true,
                                  const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bjdata).sax_parse(input_format_t::bjdata, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in BJData format
    /// @sa https://json.nlohmann.me/api/basic_json/from_bjdata/
    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_bjdata(IteratorType first, IteratorType last,
                                  const bool strict = true,
                                  const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bjdata).sax_parse(input_format_t::bjdata, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in BSON format
    /// @sa https://json.nlohmann.me/api/basic_json/from_bson/
    template<typename InputType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_bson(InputType&& i,
                                const bool strict = true,
                                const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::forward<InputType>(i));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bson).sax_parse(input_format_t::bson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    /// @brief create a JSON value from an input in BSON format
    /// @sa https://json.nlohmann.me/api/basic_json/from_bson/
    template<typename IteratorType>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json from_bson(IteratorType first, IteratorType last,
                                const bool strict = true,
                                const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = detail::input_adapter(std::move(first), std::move(last));
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bson).sax_parse(input_format_t::bson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }

    template<typename T>
    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_bson(ptr, ptr + len))
    static basic_json from_bson(const T* ptr, std::size_t len,
                                const bool strict = true,
                                const bool allow_exceptions = true)
    {
        return from_bson(ptr, ptr + len, strict, allow_exceptions);
    }

    JSON_HEDLEY_WARN_UNUSED_RESULT
    JSON_HEDLEY_DEPRECATED_FOR(3.8.0, from_bson(ptr, ptr + len))
    static basic_json from_bson(detail::span_input_adapter&& i,
                                const bool strict = true,
                                const bool allow_exceptions = true)
    {
        basic_json result;
        detail::json_sax_dom_parser<basic_json> sdp(result, allow_exceptions);
        auto ia = i.get();
        // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
        const bool res = binary_reader<decltype(ia)>(std::move(ia), input_format_t::bson).sax_parse(input_format_t::bson, &sdp, strict);
        return res ? result : basic_json(value_t::discarded);
    }
    /// @}

    //////////////////////////
    // JSON Pointer support //
    //////////////////////////

    /// @name JSON Pointer functions
    /// @{

    /// @brief access specified element via JSON Pointer
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    reference operator[](const json_pointer& ptr)
    {
        return ptr.get_unchecked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    reference operator[](const ::nlohmann::json_pointer<BasicJsonType>& ptr)
    {
        return ptr.get_unchecked(this);
    }

    /// @brief access specified element via JSON Pointer
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    const_reference operator[](const json_pointer& ptr) const
    {
        return ptr.get_unchecked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    const_reference operator[](const ::nlohmann::json_pointer<BasicJsonType>& ptr) const
    {
        return ptr.get_unchecked(this);
    }

    /// @brief access specified element via JSON Pointer
    /// @sa https://json.nlohmann.me/api/basic_json/at/
    reference at(const json_pointer& ptr)
    {
        return ptr.get_checked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    reference at(const ::nlohmann::json_pointer<BasicJsonType>& ptr)
    {
        return ptr.get_checked(this);
    }

    /// @brief access specified element via JSON Pointer
    /// @sa https://json.nlohmann.me/api/basic_json/at/
    const_reference at(const json_pointer& ptr) const
    {
        return ptr.get_checked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    const_reference at(const ::nlohmann::json_pointer<BasicJsonType>& ptr) const
    {
        return ptr.get_checked(this);
    }

    /// @brief return flattened JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/flatten/
    basic_json flatten() const
    {
        basic_json result(value_t::object);
        json_pointer::flatten("", *this, result);
        return result;
    }

    /// @brief unflatten a previously flattened JSON value
    /// @sa https://json.nlohmann.me/api/basic_json/unflatten/
    basic_json unflatten() const
    {
        return json_pointer::unflatten(*this);
    }

    /// @}

    //////////////////////////
    // JSON Patch functions //
    //////////////////////////

    /// @name JSON Patch functions
    /// @{

    /// @brief applies a JSON patch in-place without copying the object
    /// @sa https://json.nlohmann.me/api/basic_json/patch/
    void patch_inplace(const basic_json& json_patch)
    {
        basic_json& result = *this;
        // the valid JSON Patch operations
        enum class patch_operations {add, remove, replace, move, copy, test, invalid};

        const auto get_op = [](const std::string & op)
        {
            if (op == "add")
            {
                return patch_operations::add;
            }
            if (op == "remove")
            {
                return patch_operations::remove;
            }
            if (op == "replace")
            {
                return patch_operations::replace;
            }
            if (op == "move")
            {
                return patch_operations::move;
            }
            if (op == "copy")
            {
                return patch_operations::copy;
            }
            if (op == "test")
            {
                return patch_operations::test;
            }

            return patch_operations::invalid;
        };

        // wrapper for "add" operation; add value at ptr
        const auto operation_add = [&result](json_pointer & ptr, basic_json val)
        {
            // adding to the root of the target document means replacing it
            if (ptr.empty())
            {
                result = val;
                return;
            }

            // make sure the top element of the pointer exists
            json_pointer const top_pointer = ptr.top();
            if (top_pointer != ptr)
            {
                result.at(top_pointer);
            }

            // get reference to parent of JSON pointer ptr
            const auto last_path = ptr.back();
            ptr.pop_back();
            // parent must exist when performing patch add per RFC6902 specs
            basic_json& parent = result.at(ptr);

            switch (parent.m_data.m_type)
            {
                case value_t::null:
                case value_t::object:
                {
                    // use operator[] to add value
                    parent[last_path] = val;
                    break;
                }

                case value_t::array:
                {
                    if (last_path == "-")
                    {
                        // special case: append to back
                        parent.push_back(val);
                    }
                    else
                    {
                        const auto idx = json_pointer::template array_index<basic_json_t>(last_path);
                        if (JSON_HEDLEY_UNLIKELY(idx > parent.size()))
                        {
                            // avoid undefined behavior
                            JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), &parent));
                        }

                        // default case: insert add offset
                        parent.insert(parent.begin() + static_cast<difference_type>(idx), val);
                    }
                    break;
                }

                // if there exists a parent it cannot be primitive
                case value_t::string: // LCOV_EXCL_LINE
                case value_t::boolean: // LCOV_EXCL_LINE
                case value_t::number_integer: // LCOV_EXCL_LINE
                case value_t::number_unsigned: // LCOV_EXCL_LINE
                case value_t::number_float: // LCOV_EXCL_LINE
                case value_t::binary: // LCOV_EXCL_LINE
                case value_t::discarded: // LCOV_EXCL_LINE
                default:            // LCOV_EXCL_LINE
                    JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
            }
        };

        // wrapper for "remove" operation; remove value at ptr
        const auto operation_remove = [this, & result](json_pointer & ptr)
        {
            // get reference to parent of JSON pointer ptr
            const auto last_path = ptr.back();
            ptr.pop_back();
            basic_json& parent = result.at(ptr);

            // remove child
            if (parent.is_object())
            {
                // perform range check
                auto it = parent.find(last_path);
                if (JSON_HEDLEY_LIKELY(it != parent.end()))
                {
                    parent.erase(it);
                }
                else
                {
                    JSON_THROW(out_of_range::create(403, detail::concat("key '", last_path, "' not found"), this));
                }
            }
            else if (parent.is_array())
            {
                // note erase performs range check
                parent.erase(json_pointer::template array_index<basic_json_t>(last_path));
            }
        };

        // type check: top level value must be an array
        if (JSON_HEDLEY_UNLIKELY(!json_patch.is_array()))
        {
            JSON_THROW(parse_error::create(104, 0, "JSON patch must be an array of objects", &json_patch));
        }

        // iterate and apply the operations
        for (const auto& val : json_patch)
        {
            // wrapper to get a value for an operation
            const auto get_value = [&val](const std::string & op,
                                          const std::string & member,
                                          bool string_type) -> basic_json &
            {
                // find value
                auto it = val.m_data.m_value.object->find(member);

                // context-sensitive error message
                const auto error_msg = (op == "op") ? "operation" : detail::concat("operation '", op, '\'');

                // check if desired value is present
                if (JSON_HEDLEY_UNLIKELY(it == val.m_data.m_value.object->end()))
                {
                    // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                    JSON_THROW(parse_error::create(105, 0, detail::concat(error_msg, " must have member '", member, "'"), &val));
                }

                // check if result is of type string
                if (JSON_HEDLEY_UNLIKELY(string_type && !it->second.is_string()))
                {
                    // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                    JSON_THROW(parse_error::create(105, 0, detail::concat(error_msg, " must have string member '", member, "'"), &val));
                }

                // no error: return value
                return it->second;
            };

            // type check: every element of the array must be an object
            if (JSON_HEDLEY_UNLIKELY(!val.is_object()))
            {
                JSON_THROW(parse_error::create(104, 0, "JSON patch must be an array of objects", &val));
            }

            // collect mandatory members
            const auto op = get_value("op", "op", true).template get<std::string>();
            const auto path = get_value(op, "path", true).template get<std::string>();
            json_pointer ptr(path);

            switch (get_op(op))
            {
                case patch_operations::add:
                {
                    operation_add(ptr, get_value("add", "value", false));
                    break;
                }

                case patch_operations::remove:
                {
                    operation_remove(ptr);
                    break;
                }

                case patch_operations::replace:
                {
                    // the "path" location must exist - use at()
                    result.at(ptr) = get_value("replace", "value", false);
                    break;
                }

                case patch_operations::move:
                {
                    const auto from_path = get_value("move", "from", true).template get<std::string>();
                    json_pointer from_ptr(from_path);

                    // the "from" location must exist - use at()
                    basic_json const v = result.at(from_ptr);

                    // The move operation is functionally identical to a
                    // "remove" operation on the "from" location, followed
                    // immediately by an "add" operation at the target
                    // location with the value that was just removed.
                    operation_remove(from_ptr);
                    operation_add(ptr, v);
                    break;
                }

                case patch_operations::copy:
                {
                    const auto from_path = get_value("copy", "from", true).template get<std::string>();
                    const json_pointer from_ptr(from_path);

                    // the "from" location must exist - use at()
                    basic_json const v = result.at(from_ptr);

                    // The copy is functionally identical to an "add"
                    // operation at the target location using the value
                    // specified in the "from" member.
                    operation_add(ptr, v);
                    break;
                }

                case patch_operations::test:
                {
                    bool success = false;
                    JSON_TRY
                    {
                        // check if "value" matches the one at "path"
                        // the "path" location must exist - use at()
                        success = (result.at(ptr) == get_value("test", "value", false));
                    }
                    JSON_INTERNAL_CATCH (out_of_range&)
                    {
                        // ignore out of range errors: success remains false
                    }

                    // throw an exception if test fails
                    if (JSON_HEDLEY_UNLIKELY(!success))
                    {
                        JSON_THROW(other_error::create(501, detail::concat("unsuccessful: ", val.dump()), &val));
                    }

                    break;
                }

                case patch_operations::invalid:
                default:
                {
                    // op must be "add", "remove", "replace", "move", "copy", or
                    // "test"
                    JSON_THROW(parse_error::create(105, 0, detail::concat("operation value '", op, "' is invalid"), &val));
                }
            }
        }
    }

    /// @brief applies a JSON patch to a copy of the current object
    /// @sa https://json.nlohmann.me/api/basic_json/patch/
    basic_json patch(const basic_json& json_patch) const
    {
        basic_json result = *this;
        result.patch_inplace(json_patch);
        return result;
    }

    /// @brief creates a diff as a JSON patch
    /// @sa https://json.nlohmann.me/api/basic_json/diff/
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json diff(const basic_json& source, const basic_json& target,
                           const std::string& path = "")
    {
        // the patch
        basic_json result(value_t::array);

        // if the values are the same, return empty patch
        if (source == target)
        {
            return result;
        }

        if (source.type() != target.type())
        {
            // different types: replace value
            result.push_back(
            {
                {"op", "replace"}, {"path", path}, {"value", target}
            });
            return result;
        }

        switch (source.type())
        {
            case value_t::array:
            {
                // first pass: traverse common elements
                std::size_t i = 0;
                while (i < source.size() && i < target.size())
                {
                    // recursive call to compare array values at index i
                    auto temp_diff = diff(source[i], target[i], detail::concat(path, '/', std::to_string(i)));
                    result.insert(result.end(), temp_diff.begin(), temp_diff.end());
                    ++i;
                }

                // We now reached the end of at least one array
                // in a second pass, traverse the remaining elements

                // remove my remaining elements
                const auto end_index = static_cast<difference_type>(result.size());
                while (i < source.size())
                {
                    // add operations in reverse order to avoid invalid
                    // indices
                    result.insert(result.begin() + end_index, object(
                    {
                        {"op", "remove"},
                        {"path", detail::concat(path, '/', std::to_string(i))}
                    }));
                    ++i;
                }

                // add other remaining elements
                while (i < target.size())
                {
                    result.push_back(
                    {
                        {"op", "add"},
                        {"path", detail::concat(path, "/-")},
                        {"value", target[i]}
                    });
                    ++i;
                }

                break;
            }

            case value_t::object:
            {
                // first pass: traverse this object's elements
                for (auto it = source.cbegin(); it != source.cend(); ++it)
                {
                    // escape the key name to be used in a JSON patch
                    const auto path_key = detail::concat(path, '/', detail::escape(it.key()));

                    if (target.find(it.key()) != target.end())
                    {
                        // recursive call to compare object values at key it
                        auto temp_diff = diff(it.value(), target[it.key()], path_key);
                        result.insert(result.end(), temp_diff.begin(), temp_diff.end());
                    }
                    else
                    {
                        // found a key that is not in o -> remove it
                        result.push_back(object(
                        {
                            {"op", "remove"}, {"path", path_key}
                        }));
                    }
                }

                // second pass: traverse other object's elements
                for (auto it = target.cbegin(); it != target.cend(); ++it)
                {
                    if (source.find(it.key()) == source.end())
                    {
                        // found a key that is not in this -> add it
                        const auto path_key = detail::concat(path, '/', detail::escape(it.key()));
                        result.push_back(
                        {
                            {"op", "add"}, {"path", path_key},
                            {"value", it.value()}
                        });
                    }
                }

                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
            {
                // both primitive type: replace value
                result.push_back(
                {
                    {"op", "replace"}, {"path", path}, {"value", target}
                });
                break;
            }
        }

        return result;
    }
    /// @}

    ////////////////////////////////
    // JSON Merge Patch functions //
    ////////////////////////////////

    /// @name JSON Merge Patch functions
    /// @{

    /// @brief applies a JSON Merge Patch
    /// @sa https://json.nlohmann.me/api/basic_json/merge_patch/
    void merge_patch(const basic_json& apply_patch)
    {
        if (apply_patch.is_object())
        {
            if (!is_object())
            {
                *this = object();
            }
            for (auto it = apply_patch.begin(); it != apply_patch.end(); ++it)
            {
                if (it.value().is_null())
                {
                    erase(it.key());
                }
                else
                {
                    operator[](it.key()).merge_patch(it.value());
                }
            }
        }
        else
        {
            *this = apply_patch;
        }
    }

    /// @}
};

/// @brief user-defined to_string function for JSON values
/// @sa https://json.nlohmann.me/api/basic_json/to_string/
NLOHMANN_BASIC_JSON_TPL_DECLARATION
std::string to_string(const NLOHMANN_BASIC_JSON_TPL& j)
{
    return j.dump();
}

inline namespace literals
{
inline namespace json_literals
{

/// @brief user-defined string literal for JSON values
/// @sa https://json.nlohmann.me/api/basic_json/operator_literal_json/
JSON_HEDLEY_NON_NULL(1)
#if !defined(JSON_HEDLEY_GCC_VERSION) || JSON_HEDLEY_GCC_VERSION_CHECK(4,9,0)
    inline nlohmann::json operator ""_json(const char* s, std::size_t n)
#else
    inline nlohmann::json operator "" _json(const char* s, std::size_t n)
#endif
{
    return nlohmann::json::parse(s, s + n);
}

/// @brief user-defined string literal for JSON pointer
/// @sa https://json.nlohmann.me/api/basic_json/operator_literal_json_pointer/
JSON_HEDLEY_NON_NULL(1)
#if !defined(JSON_HEDLEY_GCC_VERSION) || JSON_HEDLEY_GCC_VERSION_CHECK(4,9,0)
    inline nlohmann::json::json_pointer operator ""_json_pointer(const char* s, std::size_t n)
#else
    inline nlohmann::json::json_pointer operator "" _json_pointer(const char* s, std::size_t n)
#endif
{
    return nlohmann::json::json_pointer(std::string(s, n));
}

}  // namespace json_literals
}  // namespace literals
NLOHMANN_JSON_NAMESPACE_END

///////////////////////
// nonmember support //
///////////////////////

namespace std // NOLINT(cert-dcl58-cpp)
{

/// @brief hash value for JSON objects
/// @sa https://json.nlohmann.me/api/basic_json/std_hash/
NLOHMANN_BASIC_JSON_TPL_DECLARATION
struct hash<nlohmann::NLOHMANN_BASIC_JSON_TPL> // NOLINT(cert-dcl58-cpp)
{
    std::size_t operator()(const nlohmann::NLOHMANN_BASIC_JSON_TPL& j) const
    {
        return nlohmann::detail::hash(j);
    }
};

// specialization for std::less<value_t>
template<>
struct less< ::nlohmann::detail::value_t> // do not remove the space after '<', see https://github.com/nlohmann/json/pull/679
{
    /*!
    @brief compare two value_t enum values
    @since version 3.0.0
    */
    bool operator()(::nlohmann::detail::value_t lhs,
                    ::nlohmann::detail::value_t rhs) const noexcept
    {
#if JSON_HAS_THREE_WAY_COMPARISON
        return std::is_lt(lhs <=> rhs); // *NOPAD*
#else
        return ::nlohmann::detail::operator<(lhs, rhs);
#endif
    }
};

// C++20 prohibit function specialization in the std namespace.
#ifndef JSON_HAS_CPP_20

/// @brief exchanges the values of two JSON objects
/// @sa https://json.nlohmann.me/api/basic_json/std_swap/
NLOHMANN_BASIC_JSON_TPL_DECLARATION
inline void swap(nlohmann::NLOHMANN_BASIC_JSON_TPL& j1, nlohmann::NLOHMANN_BASIC_JSON_TPL& j2) noexcept(  // NOLINT(readability-inconsistent-declaration-parameter-name, cert-dcl58-cpp)
    is_nothrow_move_constructible<nlohmann::NLOHMANN_BASIC_JSON_TPL>::value&&                          // NOLINT(misc-redundant-expression,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    is_nothrow_move_assignable<nlohmann::NLOHMANN_BASIC_JSON_TPL>::value)
{
    j1.swap(j2);
}

#endif

}  // namespace std

#if JSON_USE_GLOBAL_UDLS
    #if !defined(JSON_HEDLEY_GCC_VERSION) || JSON_HEDLEY_GCC_VERSION_CHECK(4,9,0)
        using nlohmann::literals::json_literals::operator ""_json; // NOLINT(misc-unused-using-decls,google-global-names-in-headers)
        using nlohmann::literals::json_literals::operator ""_json_pointer; //NOLINT(misc-unused-using-decls,google-global-names-in-headers)
    #else
        using nlohmann::literals::json_literals::operator "" _json; // NOLINT(misc-unused-using-decls,google-global-names-in-headers)
        using nlohmann::literals::json_literals::operator "" _json_pointer; //NOLINT(misc-unused-using-decls,google-global-names-in-headers)
    #endif
#endif

#include <nlohmann/detail/macro_unscope.hpp>

#endif  // INCLUDE_NLOHMANN_JSON_HPP_
