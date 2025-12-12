//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <iterator> // iterator, random_access_iterator_tag, bidirectional_iterator_tag, advance, next
#include <type_traits> // conditional, is_const, remove_const

#include <nlohmann/detail/exceptions.hpp>
#include <nlohmann/detail/iterators/internal_iterator.hpp>
#include <nlohmann/detail/iterators/primitive_iterator.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/detail/meta/cpp_future.hpp>
#include <nlohmann/detail/meta/type_traits.hpp>
#include <nlohmann/detail/value_t.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{

// forward declare, to be able to friend it later on
template<typename IteratorType> class iteration_proxy;
template<typename IteratorType> class iteration_proxy_value;

/*!
@brief a template for a bidirectional iterator for the @ref basic_json class
This class implements a both iterators (iterator and const_iterator) for the
@ref basic_json class.
@note An iterator is called *initialized* when a pointer to a JSON value has
      been set (e.g., by a constructor or a copy assignment). If the iterator is
      default-constructed, it is *uninitialized* and most methods are undefined.
      **The library uses assertions to detect calls on uninitialized iterators.**
@requirement The class satisfies the following concept requirements:
-
[BidirectionalIterator](https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator):
  The iterator that can be moved can be moved in both directions (i.e.
  incremented and decremented).
@since version 1.0.0, simplified in version 2.0.9, change to bidirectional
       iterators in version 3.0.0 (see https://github.com/nlohmann/json/issues/593)
*/
template<typename BasicJsonType>
class iter_impl // NOLINT(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
{
    /// the iterator with BasicJsonType of different const-ness
    using other_iter_impl = iter_impl<typename std::conditional<std::is_const<BasicJsonType>::value, typename std::remove_const<BasicJsonType>::type, const BasicJsonType>::type>;
    /// allow basic_json to access private members
    friend other_iter_impl;
    friend BasicJsonType;
    friend iteration_proxy<iter_impl>;
    friend iteration_proxy_value<iter_impl>;

    using object_t = typename BasicJsonType::object_t;
    using array_t = typename BasicJsonType::array_t;
    // make sure BasicJsonType is basic_json or const basic_json
    static_assert(is_basic_json<typename std::remove_const<BasicJsonType>::type>::value,
                  "iter_impl only accepts (const) basic_json");
    // superficial check for the LegacyBidirectionalIterator named requirement
    static_assert(std::is_base_of<std::bidirectional_iterator_tag, std::bidirectional_iterator_tag>::value
                  &&  std::is_base_of<std::bidirectional_iterator_tag, typename std::iterator_traits<typename array_t::iterator>::iterator_category>::value,
                  "basic_json iterator assumes array and object type iterators satisfy the LegacyBidirectionalIterator named requirement.");

  public:
    /// The std::iterator class template (used as a base class to provide typedefs) is deprecated in C++17.
    /// The C++ Standard has never required user-defined iterators to derive from std::iterator.
    /// A user-defined iterator should provide publicly accessible typedefs named
    /// iterator_category, value_type, difference_type, pointer, and reference.
    /// Note that value_type is required to be non-const, even for constant iterators.
    using iterator_category = std::bidirectional_iterator_tag;

    /// the type of the values when the iterator is dereferenced
    using value_type = typename BasicJsonType::value_type;
    /// a type to represent differences between iterators
    using difference_type = typename BasicJsonType::difference_type;
    /// defines a pointer to the type iterated over (value_type)
    using pointer = typename std::conditional<std::is_const<BasicJsonType>::value,
          typename BasicJsonType::const_pointer,
          typename BasicJsonType::pointer>::type;
    /// defines a reference to the type iterated over (value_type)
    using reference =
        typename std::conditional<std::is_const<BasicJsonType>::value,
        typename BasicJsonType::const_reference,
        typename BasicJsonType::reference>::type;

    iter_impl() = default;
    ~iter_impl() = default;
    iter_impl(iter_impl&&) noexcept = default;
    iter_impl& operator=(iter_impl&&) noexcept = default;

    /*!
    @brief constructor for a given JSON instance
    @param[in] object  pointer to a JSON object for this iterator
    @pre object != nullptr
    @post The iterator is initialized; i.e. `m_object != nullptr`.
    */
    explicit iter_impl(pointer object) noexcept : m_object(object)
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
            {
                m_it.object_iterator = typename object_t::iterator();
                break;
            }

            case value_t::array:
            {
                m_it.array_iterator = typename array_t::iterator();
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
                m_it.primitive_iterator = primitive_iterator_t();
                break;
            }
        }
    }

    /*!
    @note The conventional copy constructor and copy assignment are implicitly
          defined. Combined with the following converting constructor and
          assignment, they support: (1) copy from iterator to iterator, (2)
          copy from const iterator to const iterator, and (3) conversion from
          iterator to const iterator. However conversion from const iterator
          to iterator is not defined.
    */

    /*!
    @brief const copy constructor
    @param[in] other const iterator to copy from
    @note This copy constructor had to be defined explicitly to circumvent a bug
          occurring on msvc v19.0 compiler (VS 2015) debug build. For more
          information refer to: https://github.com/nlohmann/json/issues/1608
    */
    iter_impl(const iter_impl<const BasicJsonType>& other) noexcept
        : m_object(other.m_object), m_it(other.m_it)
    {}

    /*!
    @brief converting assignment
    @param[in] other const iterator to copy from
    @return const/non-const iterator
    @note It is not checked whether @a other is initialized.
    */
    iter_impl& operator=(const iter_impl<const BasicJsonType>& other) noexcept
    {
        if (&other != this)
        {
            m_object = other.m_object;
            m_it = other.m_it;
        }
        return *this;
    }

    /*!
    @brief converting constructor
    @param[in] other  non-const iterator to copy from
    @note It is not checked whether @a other is initialized.
    */
    iter_impl(const iter_impl<typename std::remove_const<BasicJsonType>::type>& other) noexcept
        : m_object(other.m_object), m_it(other.m_it)
    {}

    /*!
    @brief converting assignment
    @param[in] other  non-const iterator to copy from
    @return const/non-const iterator
    @note It is not checked whether @a other is initialized.
    */
    iter_impl& operator=(const iter_impl<typename std::remove_const<BasicJsonType>::type>& other) noexcept // NOLINT(cert-oop54-cpp)
    {
        m_object = other.m_object;
        m_it = other.m_it;
        return *this;
    }

  JSON_PRIVATE_UNLESS_TESTED:
    /*!
    @brief set the iterator to the first value
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    void set_begin() noexcept
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
            {
                m_it.object_iterator = m_object->m_data.m_value.object->begin();
                break;
            }

            case value_t::array:
            {
                m_it.array_iterator = m_object->m_data.m_value.array->begin();
                break;
            }

            case value_t::null:
            {
                // set to end so begin()==end() is true: null is empty
                m_it.primitive_iterator.set_end();
                break;
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
                m_it.primitive_iterator.set_begin();
                break;
            }
        }
    }

    /*!
    @brief set the iterator past the last value
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    void set_end() noexcept
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
            {
                m_it.object_iterator = m_object->m_data.m_value.object->end();
                break;
            }

            case value_t::array:
            {
                m_it.array_iterator = m_object->m_data.m_value.array->end();
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
                m_it.primitive_iterator.set_end();
                break;
            }
        }
    }

  public:
    /*!
    @brief return a reference to the value pointed to by the iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    reference operator*() const
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
            {
                JSON_ASSERT(m_it.object_iterator != m_object->m_data.m_value.object->end());
                return m_it.object_iterator->second;
            }

            case value_t::array:
            {
                JSON_ASSERT(m_it.array_iterator != m_object->m_data.m_value.array->end());
                return *m_it.array_iterator;
            }

            case value_t::null:
                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
            {
                if (JSON_HEDLEY_LIKELY(m_it.primitive_iterator.is_begin()))
                {
                    return *m_object;
                }

                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));
            }
        }
    }

    /*!
    @brief dereference the iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    pointer operator->() const
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
            {
                JSON_ASSERT(m_it.object_iterator != m_object->m_data.m_value.object->end());
                return &(m_it.object_iterator->second);
            }

            case value_t::array:
            {
                JSON_ASSERT(m_it.array_iterator != m_object->m_data.m_value.array->end());
                return &*m_it.array_iterator;
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
                if (JSON_HEDLEY_LIKELY(m_it.primitive_iterator.is_begin()))
                {
                    return m_object;
                }

                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));
            }
        }
    }

    /*!
    @brief post-increment (it++)
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    iter_impl operator++(int)& // NOLINT(cert-dcl21-cpp)
    {
        auto result = *this;
        ++(*this);
        return result;
    }

    /*!
    @brief pre-increment (++it)
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    iter_impl& operator++()
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
            {
                std::advance(m_it.object_iterator, 1);
                break;
            }

            case value_t::array:
            {
                std::advance(m_it.array_iterator, 1);
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
                ++m_it.primitive_iterator;
                break;
            }
        }

        return *this;
    }

    /*!
    @brief post-decrement (it--)
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    iter_impl operator--(int)& // NOLINT(cert-dcl21-cpp)
    {
        auto result = *this;
        --(*this);
        return result;
    }

    /*!
    @brief pre-decrement (--it)
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    iter_impl& operator--()
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
            {
                std::advance(m_it.object_iterator, -1);
                break;
            }

            case value_t::array:
            {
                std::advance(m_it.array_iterator, -1);
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
                --m_it.primitive_iterator;
                break;
            }
        }

        return *this;
    }

    /*!
    @brief comparison: equal
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    template < typename IterImpl, detail::enable_if_t < (std::is_same<IterImpl, iter_impl>::value || std::is_same<IterImpl, other_iter_impl>::value), std::nullptr_t > = nullptr >
    bool operator==(const IterImpl& other) const
    {
        // if objects are not the same, the comparison is undefined
        if (JSON_HEDLEY_UNLIKELY(m_object != other.m_object))
        {
            JSON_THROW(invalid_iterator::create(212, "cannot compare iterators of different containers", m_object));
        }

        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
                return (m_it.object_iterator == other.m_it.object_iterator);

            case value_t::array:
                return (m_it.array_iterator == other.m_it.array_iterator);

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                return (m_it.primitive_iterator == other.m_it.primitive_iterator);
        }
    }

    /*!
    @brief comparison: not equal
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    template < typename IterImpl, detail::enable_if_t < (std::is_same<IterImpl, iter_impl>::value || std::is_same<IterImpl, other_iter_impl>::value), std::nullptr_t > = nullptr >
    bool operator!=(const IterImpl& other) const
    {
        return !operator==(other);
    }

    /*!
    @brief comparison: smaller
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    bool operator<(const iter_impl& other) const
    {
        // if objects are not the same, the comparison is undefined
        if (JSON_HEDLEY_UNLIKELY(m_object != other.m_object))
        {
            JSON_THROW(invalid_iterator::create(212, "cannot compare iterators of different containers", m_object));
        }

        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
                JSON_THROW(invalid_iterator::create(213, "cannot compare order of object iterators", m_object));

            case value_t::array:
                return (m_it.array_iterator < other.m_it.array_iterator);

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                return (m_it.primitive_iterator < other.m_it.primitive_iterator);
        }
    }

    /*!
    @brief comparison: less than or equal
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    bool operator<=(const iter_impl& other) const
    {
        return !other.operator < (*this);
    }

    /*!
    @brief comparison: greater than
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    bool operator>(const iter_impl& other) const
    {
        return !operator<=(other);
    }

    /*!
    @brief comparison: greater than or equal
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    bool operator>=(const iter_impl& other) const
    {
        return !operator<(other);
    }

    /*!
    @brief add to iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    iter_impl& operator+=(difference_type i)
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
                JSON_THROW(invalid_iterator::create(209, "cannot use offsets with object iterators", m_object));

            case value_t::array:
            {
                std::advance(m_it.array_iterator, i);
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
                m_it.primitive_iterator += i;
                break;
            }
        }

        return *this;
    }

    /*!
    @brief subtract from iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    iter_impl& operator-=(difference_type i)
    {
        return operator+=(-i);
    }

    /*!
    @brief add to iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    iter_impl operator+(difference_type i) const
    {
        auto result = *this;
        result += i;
        return result;
    }

    /*!
    @brief addition of distance and iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    friend iter_impl operator+(difference_type i, const iter_impl& it)
    {
        auto result = it;
        result += i;
        return result;
    }

    /*!
    @brief subtract from iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    iter_impl operator-(difference_type i) const
    {
        auto result = *this;
        result -= i;
        return result;
    }

    /*!
    @brief return difference
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    difference_type operator-(const iter_impl& other) const
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
                JSON_THROW(invalid_iterator::create(209, "cannot use offsets with object iterators", m_object));

            case value_t::array:
                return m_it.array_iterator - other.m_it.array_iterator;

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                return m_it.primitive_iterator - other.m_it.primitive_iterator;
        }
    }

    /*!
    @brief access to successor
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    reference operator[](difference_type n) const
    {
        JSON_ASSERT(m_object != nullptr);

        switch (m_object->m_data.m_type)
        {
            case value_t::object:
                JSON_THROW(invalid_iterator::create(208, "cannot use operator[] for object iterators", m_object));

            case value_t::array:
                return *std::next(m_it.array_iterator, n);

            case value_t::null:
                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
            {
                if (JSON_HEDLEY_LIKELY(m_it.primitive_iterator.get_value() == -n))
                {
                    return *m_object;
                }

                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));
            }
        }
    }

    /*!
    @brief return the key of an object iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    const typename object_t::key_type& key() const
    {
        JSON_ASSERT(m_object != nullptr);

        if (JSON_HEDLEY_LIKELY(m_object->is_object()))
        {
            return m_it.object_iterator->first;
        }

        JSON_THROW(invalid_iterator::create(207, "cannot use key() for non-object iterators", m_object));
    }

    /*!
    @brief return the value of an iterator
    @pre The iterator is initialized; i.e. `m_object != nullptr`.
    */
    reference value() const
    {
        return operator*();
    }

  JSON_PRIVATE_UNLESS_TESTED:
    /// associated JSON instance
    pointer m_object = nullptr;
    /// the actual iterator of the associated instance
    internal_iterator<typename std::remove_const<BasicJsonType>::type> m_it {};
};

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END
