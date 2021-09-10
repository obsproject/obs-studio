/* SPDX-License-Identifier: MIT */
#pragma once

#include <memory>

namespace aja {

// shim for std::make unique from c++14
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace aja