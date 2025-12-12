//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <utility>

#include <nlohmann/detail/abi_macros.hpp>
#include <nlohmann/detail/conversions/from_json.hpp>
#include <nlohmann/detail/conversions/to_json.hpp>
#include <nlohmann/detail/meta/identity_tag.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN

/// @sa https://json.nlohmann.me/api/adl_serializer/
template<typename ValueType, typename>
struct adl_serializer
{
    /// @brief convert a JSON value to any value type
    /// @sa https://json.nlohmann.me/api/adl_serializer/from_json/
    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto from_json(BasicJsonType && j, TargetType& val) noexcept(
        noexcept(::nlohmann::from_json(std::forward<BasicJsonType>(j), val)))
    -> decltype(::nlohmann::from_json(std::forward<BasicJsonType>(j), val), void())
    {
        ::nlohmann::from_json(std::forward<BasicJsonType>(j), val);
    }

    /// @brief convert a JSON value to any value type
    /// @sa https://json.nlohmann.me/api/adl_serializer/from_json/
    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto from_json(BasicJsonType && j) noexcept(
    noexcept(::nlohmann::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType> {})))
    -> decltype(::nlohmann::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType> {}))
    {
        return ::nlohmann::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType> {});
    }

    /// @brief convert any value type to a JSON value
    /// @sa https://json.nlohmann.me/api/adl_serializer/to_json/
    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto to_json(BasicJsonType& j, TargetType && val) noexcept(
        noexcept(::nlohmann::to_json(j, std::forward<TargetType>(val))))
    -> decltype(::nlohmann::to_json(j, std::forward<TargetType>(val)), void())
    {
        ::nlohmann::to_json(j, std::forward<TargetType>(val));
    }
};

NLOHMANN_JSON_NAMESPACE_END
