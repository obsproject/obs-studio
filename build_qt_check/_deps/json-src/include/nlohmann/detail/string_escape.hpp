//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <nlohmann/detail/abi_macros.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{

/*!
@brief replace all occurrences of a substring by another string

@param[in,out] s  the string to manipulate; changed so that all
               occurrences of @a f are replaced with @a t
@param[in]     f  the substring to replace with @a t
@param[in]     t  the string to replace @a f

@pre The search string @a f must not be empty. **This precondition is
enforced with an assertion.**

@since version 2.0.0
*/
template<typename StringType>
inline void replace_substring(StringType& s, const StringType& f,
                              const StringType& t)
{
    JSON_ASSERT(!f.empty());
    for (auto pos = s.find(f);                // find first occurrence of f
            pos != StringType::npos;          // make sure f was found
            s.replace(pos, f.size(), t),      // replace with t, and
            pos = s.find(f, pos + t.size()))  // find next occurrence of f
    {}
}

/*!
 * @brief string escaping as described in RFC 6901 (Sect. 4)
 * @param[in] s string to escape
 * @return    escaped string
 *
 * Note the order of escaping "~" to "~0" and "/" to "~1" is important.
 */
template<typename StringType>
inline StringType escape(StringType s)
{
    replace_substring(s, StringType{"~"}, StringType{"~0"});
    replace_substring(s, StringType{"/"}, StringType{"~1"});
    return s;
}

/*!
 * @brief string unescaping as described in RFC 6901 (Sect. 4)
 * @param[in] s string to unescape
 * @return    unescaped string
 *
 * Note the order of escaping "~1" to "/" and "~0" to "~" is important.
 */
template<typename StringType>
static void unescape(StringType& s)
{
    replace_substring(s, StringType{"~1"}, StringType{"/"});
    replace_substring(s, StringType{"~0"}, StringType{"~"});
}

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END
