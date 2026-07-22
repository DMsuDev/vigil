// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include <memory>
#include <utility>

/**
 * @file smart_pointers.h
 * @brief Short, semantic aliases for Vigil's preferred ownership model.
 *
 * Vigil standardizes on two ownership models throughout its public and
 * internal API:
 *
 * - vigil::Scope<T>  -> exclusive ownership (std::unique_ptr)
 * - vigil::Shared<T> -> shared ownership    (std::shared_ptr)
 *
 * Prefer Scope<T> by default. It provides deterministic exclusive ownership
 * with no reference-counting overhead. Use Shared<T> only when shared
 * ownership is genuinely required.
 */

namespace vigil {

/// @brief Alias for std::unique_ptr with exclusive ownership semantics.
template <typename T>
using Scope = std::unique_ptr<T>;

/// @brief Alias for std::shared_ptr with shared ownership semantics.
template <typename T>
using Shared = std::shared_ptr<T>;

/**
 * @brief Constructs an object of type T with exclusive ownership.
 * @tparam T    Type to construct.
 * @tparam Args Constructor argument types (deduced).
 * @param args  Arguments forwarded to T's constructor.
 * @return A Scope<T> owning the newly created object.
 * @note Exception-safe alternative to `Scope<T>(new T(...))`.
 */
template <typename T, typename... Args>
[[nodiscard]] Scope<T> CreateScope(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/**
 * @brief Constructs a T with shared ownership using perfect forwarding.
 * @tparam T    Type to construct.
 * @tparam Args Constructor argument types (deduced).
 * @param args  Arguments forwarded to T's constructor.
 * @return A Shared<T> owning the newly created object.
 * @note Uses reference counting. Prefer CreateScope() unless shared ownership is required.
 */
template <typename T, typename... Args>
[[nodiscard]] Shared<T> CreateShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

} // namespace vigil
