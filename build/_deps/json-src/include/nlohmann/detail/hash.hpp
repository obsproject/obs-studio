//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint> // uint8_t
#include <cstddef> // size_t
#include <functional> // hash

#include <nlohmann/detail/abi_macros.hpp>
#include <nlohmann/detail/value_t.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{

// boost::hash_combine
inline std::size_t combine(std::size_t seed, std::size_t h) noexcept
{
    seed ^= h + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
    return seed;
}

/*!
@brief hash a JSON value

The hash function tries to rely on std::hash where possible. Furthermore, the
type of the JSON value is taken into account to have different hash values for
null, 0, 0U, and false, etc.

@tparam BasicJsonType basic_json specialization
@param j JSON value to hash
@return hash value of j
*/
template<typename BasicJsonType>
std::size_t hash(const BasicJsonType& j)
{
    using string_t = typename BasicJsonType::string_t;
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;

    const auto type = static_cast<std::size_t>(j.type());
    switch (j.type())
    {
        case BasicJsonType::value_t::null:
        case BasicJsonType::value_t::discarded:
        {
            return combine(type, 0);
        }

        case BasicJsonType::value_t::object:
        {
            auto seed = combine(type, j.size());
            for (const auto& element : j.items())
            {
                const auto h = std::hash<string_t> {}(element.key());
                seed = combine(seed, h);
                seed = combine(seed, hash(element.value()));
            }
            return seed;
        }

        case BasicJsonType::value_t::array:
        {
            auto seed = combine(type, j.size());
            for (const auto& element : j)
            {
                seed = combine(seed, hash(element));
            }
            return seed;
        }

        case BasicJsonType::value_t::string:
        {
            const auto h = std::hash<string_t> {}(j.template get_ref<const string_t&>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::boolean:
        {
            const auto h = std::hash<bool> {}(j.template get<bool>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::number_integer:
        {
            const auto h = std::hash<number_integer_t> {}(j.template get<number_integer_t>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::number_unsigned:
        {
            const auto h = std::hash<number_unsigned_t> {}(j.template get<number_unsigned_t>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::number_float:
        {
            const auto h = std::hash<number_float_t> {}(j.template get<number_float_t>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::binary:
        {
            auto seed = combine(type, j.get_binary().size());
            const auto h = std::hash<bool> {}(j.get_binary().has_subtype());
            seed = combine(seed, h);
            seed = combine(seed, static_cast<std::size_t>(j.get_binary().subtype()));
            for (const auto byte : j.get_binary())
            {
                seed = combine(seed, std::hash<std::uint8_t> {}(byte));
            }
            return seed;
        }

        default:                   // LCOV_EXCL_LINE
            JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
            return 0;              // LCOV_EXCL_LINE
    }
}

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END
