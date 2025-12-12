//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint> // uint8_t, uint64_t
#include <tuple> // tie
#include <utility> // move

#include <nlohmann/detail/abi_macros.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN

/// @brief an internal type for a backed binary type
/// @sa https://json.nlohmann.me/api/byte_container_with_subtype/
template<typename BinaryType>
class byte_container_with_subtype : public BinaryType
{
  public:
    using container_type = BinaryType;
    using subtype_type = std::uint64_t;

    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/byte_container_with_subtype/
    byte_container_with_subtype() noexcept(noexcept(container_type()))
        : container_type()
    {}

    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/byte_container_with_subtype/
    byte_container_with_subtype(const container_type& b) noexcept(noexcept(container_type(b)))
        : container_type(b)
    {}

    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/byte_container_with_subtype/
    byte_container_with_subtype(container_type&& b) noexcept(noexcept(container_type(std::move(b))))
        : container_type(std::move(b))
    {}

    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/byte_container_with_subtype/
    byte_container_with_subtype(const container_type& b, subtype_type subtype_) noexcept(noexcept(container_type(b)))
        : container_type(b)
        , m_subtype(subtype_)
        , m_has_subtype(true)
    {}

    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/byte_container_with_subtype/
    byte_container_with_subtype(container_type&& b, subtype_type subtype_) noexcept(noexcept(container_type(std::move(b))))
        : container_type(std::move(b))
        , m_subtype(subtype_)
        , m_has_subtype(true)
    {}

    bool operator==(const byte_container_with_subtype& rhs) const
    {
        return std::tie(static_cast<const BinaryType&>(*this), m_subtype, m_has_subtype) ==
               std::tie(static_cast<const BinaryType&>(rhs), rhs.m_subtype, rhs.m_has_subtype);
    }

    bool operator!=(const byte_container_with_subtype& rhs) const
    {
        return !(rhs == *this);
    }

    /// @brief sets the binary subtype
    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/set_subtype/
    void set_subtype(subtype_type subtype_) noexcept
    {
        m_subtype = subtype_;
        m_has_subtype = true;
    }

    /// @brief return the binary subtype
    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/subtype/
    constexpr subtype_type subtype() const noexcept
    {
        return m_has_subtype ? m_subtype : static_cast<subtype_type>(-1);
    }

    /// @brief return whether the value has a subtype
    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/has_subtype/
    constexpr bool has_subtype() const noexcept
    {
        return m_has_subtype;
    }

    /// @brief clears the binary subtype
    /// @sa https://json.nlohmann.me/api/byte_container_with_subtype/clear_subtype/
    void clear_subtype() noexcept
    {
        m_subtype = 0;
        m_has_subtype = false;
    }

  private:
    subtype_type m_subtype = 0;
    bool m_has_subtype = false;
};

NLOHMANN_JSON_NAMESPACE_END
