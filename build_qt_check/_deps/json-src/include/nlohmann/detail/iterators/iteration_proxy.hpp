//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef> // size_t
#include <iterator> // input_iterator_tag
#include <string> // string, to_string
#include <tuple> // tuple_size, get, tuple_element
#include <utility> // move

#if JSON_HAS_RANGES
    #include <ranges> // enable_borrowed_range
#endif

#include <nlohmann/detail/abi_macros.hpp>
#include <nlohmann/detail/meta/type_traits.hpp>
#include <nlohmann/detail/value_t.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{

template<typename string_type>
void int_to_string( string_type& target, std::size_t value )
{
    // For ADL
    using std::to_string;
    target = to_string(value);
}
template<typename IteratorType> class iteration_proxy_value
{
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = iteration_proxy_value;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::input_iterator_tag;
    using string_type = typename std::remove_cv< typename std::remove_reference<decltype( std::declval<IteratorType>().key() ) >::type >::type;

  private:
    /// the iterator
    IteratorType anchor{};
    /// an index for arrays (used to create key names)
    std::size_t array_index = 0;
    /// last stringified array index
    mutable std::size_t array_index_last = 0;
    /// a string representation of the array index
    mutable string_type array_index_str = "0";
    /// an empty string (to return a reference for primitive values)
    string_type empty_str{};

  public:
    explicit iteration_proxy_value() = default;
    explicit iteration_proxy_value(IteratorType it, std::size_t array_index_ = 0)
    noexcept(std::is_nothrow_move_constructible<IteratorType>::value
             && std::is_nothrow_default_constructible<string_type>::value)
        : anchor(std::move(it))
        , array_index(array_index_)
    {}

    iteration_proxy_value(iteration_proxy_value const&) = default;
    iteration_proxy_value& operator=(iteration_proxy_value const&) = default;
    // older GCCs are a bit fussy and require explicit noexcept specifiers on defaulted functions
    iteration_proxy_value(iteration_proxy_value&&)
    noexcept(std::is_nothrow_move_constructible<IteratorType>::value
             && std::is_nothrow_move_constructible<string_type>::value) = default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor,cppcoreguidelines-noexcept-move-operations)
    iteration_proxy_value& operator=(iteration_proxy_value&&)
    noexcept(std::is_nothrow_move_assignable<IteratorType>::value
             && std::is_nothrow_move_assignable<string_type>::value) = default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor,cppcoreguidelines-noexcept-move-operations)
    ~iteration_proxy_value() = default;

    /// dereference operator (needed for range-based for)
    const iteration_proxy_value& operator*() const
    {
        return *this;
    }

    /// increment operator (needed for range-based for)
    iteration_proxy_value& operator++()
    {
        ++anchor;
        ++array_index;

        return *this;
    }

    iteration_proxy_value operator++(int)& // NOLINT(cert-dcl21-cpp)
    {
        auto tmp = iteration_proxy_value(anchor, array_index);
        ++anchor;
        ++array_index;
        return tmp;
    }

    /// equality operator (needed for InputIterator)
    bool operator==(const iteration_proxy_value& o) const
    {
        return anchor == o.anchor;
    }

    /// inequality operator (needed for range-based for)
    bool operator!=(const iteration_proxy_value& o) const
    {
        return anchor != o.anchor;
    }

    /// return key of the iterator
    const string_type& key() const
    {
        JSON_ASSERT(anchor.m_object != nullptr);

        switch (anchor.m_object->type())
        {
            // use integer array index as key
            case value_t::array:
            {
                if (array_index != array_index_last)
                {
                    int_to_string( array_index_str, array_index );
                    array_index_last = array_index;
                }
                return array_index_str;
            }

            // use key from the object
            case value_t::object:
                return anchor.key();

            // use an empty key for all primitive types
            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                return empty_str;
        }
    }

    /// return value of the iterator
    typename IteratorType::reference value() const
    {
        return anchor.value();
    }
};

/// proxy class for the items() function
template<typename IteratorType> class iteration_proxy
{
  private:
    /// the container to iterate
    typename IteratorType::pointer container = nullptr;

  public:
    explicit iteration_proxy() = default;

    /// construct iteration proxy from a container
    explicit iteration_proxy(typename IteratorType::reference cont) noexcept
        : container(&cont) {}

    iteration_proxy(iteration_proxy const&) = default;
    iteration_proxy& operator=(iteration_proxy const&) = default;
    iteration_proxy(iteration_proxy&&) noexcept = default;
    iteration_proxy& operator=(iteration_proxy&&) noexcept = default;
    ~iteration_proxy() = default;

    /// return iterator begin (needed for range-based for)
    iteration_proxy_value<IteratorType> begin() const noexcept
    {
        return iteration_proxy_value<IteratorType>(container->begin());
    }

    /// return iterator end (needed for range-based for)
    iteration_proxy_value<IteratorType> end() const noexcept
    {
        return iteration_proxy_value<IteratorType>(container->end());
    }
};

// Structured Bindings Support
// For further reference see https://blog.tartanllama.xyz/structured-bindings/
// And see https://github.com/nlohmann/json/pull/1391
template<std::size_t N, typename IteratorType, enable_if_t<N == 0, int> = 0>
auto get(const nlohmann::detail::iteration_proxy_value<IteratorType>& i) -> decltype(i.key())
{
    return i.key();
}
// Structured Bindings Support
// For further reference see https://blog.tartanllama.xyz/structured-bindings/
// And see https://github.com/nlohmann/json/pull/1391
template<std::size_t N, typename IteratorType, enable_if_t<N == 1, int> = 0>
auto get(const nlohmann::detail::iteration_proxy_value<IteratorType>& i) -> decltype(i.value())
{
    return i.value();
}

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END

// The Addition to the STD Namespace is required to add
// Structured Bindings Support to the iteration_proxy_value class
// For further reference see https://blog.tartanllama.xyz/structured-bindings/
// And see https://github.com/nlohmann/json/pull/1391
namespace std
{

#if defined(__clang__)
    // Fix: https://github.com/nlohmann/json/issues/1401
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
template<typename IteratorType>
class tuple_size<::nlohmann::detail::iteration_proxy_value<IteratorType>> // NOLINT(cert-dcl58-cpp)
            : public std::integral_constant<std::size_t, 2> {};

template<std::size_t N, typename IteratorType>
class tuple_element<N, ::nlohmann::detail::iteration_proxy_value<IteratorType >> // NOLINT(cert-dcl58-cpp)
{
  public:
    using type = decltype(
                     get<N>(std::declval <
                            ::nlohmann::detail::iteration_proxy_value<IteratorType >> ()));
};
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

}  // namespace std

#if JSON_HAS_RANGES
    template <typename IteratorType>
    inline constexpr bool ::std::ranges::enable_borrowed_range<::nlohmann::detail::iteration_proxy<IteratorType>> = true;
#endif
