//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstring> // strlen
#include <string> // string
#include <utility> // forward

#include <nlohmann/detail/meta/cpp_future.hpp>
#include <nlohmann/detail/meta/detected.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
namespace detail
{

inline std::size_t concat_length()
{
    return 0;
}

template<typename... Args>
inline std::size_t concat_length(const char* cstr, const Args& ... rest);

template<typename StringType, typename... Args>
inline std::size_t concat_length(const StringType& str, const Args& ... rest);

template<typename... Args>
inline std::size_t concat_length(const char /*c*/, const Args& ... rest)
{
    return 1 + concat_length(rest...);
}

template<typename... Args>
inline std::size_t concat_length(const char* cstr, const Args& ... rest)
{
    // cppcheck-suppress ignoredReturnValue
    return ::strlen(cstr) + concat_length(rest...);
}

template<typename StringType, typename... Args>
inline std::size_t concat_length(const StringType& str, const Args& ... rest)
{
    return str.size() + concat_length(rest...);
}

template<typename OutStringType>
inline void concat_into(OutStringType& /*out*/)
{}

template<typename StringType, typename Arg>
using string_can_append = decltype(std::declval<StringType&>().append(std::declval < Arg && > ()));

template<typename StringType, typename Arg>
using detect_string_can_append = is_detected<string_can_append, StringType, Arg>;

template<typename StringType, typename Arg>
using string_can_append_op = decltype(std::declval<StringType&>() += std::declval < Arg && > ());

template<typename StringType, typename Arg>
using detect_string_can_append_op = is_detected<string_can_append_op, StringType, Arg>;

template<typename StringType, typename Arg>
using string_can_append_iter = decltype(std::declval<StringType&>().append(std::declval<const Arg&>().begin(), std::declval<const Arg&>().end()));

template<typename StringType, typename Arg>
using detect_string_can_append_iter = is_detected<string_can_append_iter, StringType, Arg>;

template<typename StringType, typename Arg>
using string_can_append_data = decltype(std::declval<StringType&>().append(std::declval<const Arg&>().data(), std::declval<const Arg&>().size()));

template<typename StringType, typename Arg>
using detect_string_can_append_data = is_detected<string_can_append_data, StringType, Arg>;

template < typename OutStringType, typename Arg, typename... Args,
           enable_if_t < !detect_string_can_append<OutStringType, Arg>::value
                         && detect_string_can_append_op<OutStringType, Arg>::value, int > = 0 >
inline void concat_into(OutStringType& out, Arg && arg, Args && ... rest);

template < typename OutStringType, typename Arg, typename... Args,
           enable_if_t < !detect_string_can_append<OutStringType, Arg>::value
                         && !detect_string_can_append_op<OutStringType, Arg>::value
                         && detect_string_can_append_iter<OutStringType, Arg>::value, int > = 0 >
inline void concat_into(OutStringType& out, const Arg& arg, Args && ... rest);

template < typename OutStringType, typename Arg, typename... Args,
           enable_if_t < !detect_string_can_append<OutStringType, Arg>::value
                         && !detect_string_can_append_op<OutStringType, Arg>::value
                         && !detect_string_can_append_iter<OutStringType, Arg>::value
                         && detect_string_can_append_data<OutStringType, Arg>::value, int > = 0 >
inline void concat_into(OutStringType& out, const Arg& arg, Args && ... rest);

template<typename OutStringType, typename Arg, typename... Args,
         enable_if_t<detect_string_can_append<OutStringType, Arg>::value, int> = 0>
inline void concat_into(OutStringType& out, Arg && arg, Args && ... rest)
{
    out.append(std::forward<Arg>(arg));
    concat_into(out, std::forward<Args>(rest)...);
}

template < typename OutStringType, typename Arg, typename... Args,
           enable_if_t < !detect_string_can_append<OutStringType, Arg>::value
                         && detect_string_can_append_op<OutStringType, Arg>::value, int > >
inline void concat_into(OutStringType& out, Arg&& arg, Args&& ... rest)
{
    out += std::forward<Arg>(arg);
    concat_into(out, std::forward<Args>(rest)...);
}

template < typename OutStringType, typename Arg, typename... Args,
           enable_if_t < !detect_string_can_append<OutStringType, Arg>::value
                         && !detect_string_can_append_op<OutStringType, Arg>::value
                         && detect_string_can_append_iter<OutStringType, Arg>::value, int > >
inline void concat_into(OutStringType& out, const Arg& arg, Args&& ... rest)
{
    out.append(arg.begin(), arg.end());
    concat_into(out, std::forward<Args>(rest)...);
}

template < typename OutStringType, typename Arg, typename... Args,
           enable_if_t < !detect_string_can_append<OutStringType, Arg>::value
                         && !detect_string_can_append_op<OutStringType, Arg>::value
                         && !detect_string_can_append_iter<OutStringType, Arg>::value
                         && detect_string_can_append_data<OutStringType, Arg>::value, int > >
inline void concat_into(OutStringType& out, const Arg& arg, Args&& ... rest)
{
    out.append(arg.data(), arg.size());
    concat_into(out, std::forward<Args>(rest)...);
}

template<typename OutStringType = std::string, typename... Args>
inline OutStringType concat(Args && ... args)
{
    OutStringType str;
    str.reserve(concat_length(args...));
    concat_into(str, std::forward<Args>(args)...);
    return str;
}

}  // namespace detail
NLOHMANN_JSON_NAMESPACE_END
