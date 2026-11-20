// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include <utility>

/**
 * @file hook_scope.h
 * @brief Per-hook, per-thread reentrancy protection for LogSystem hooks.
 *
 * A hook callback may call back into LogSystem (for example, calling
 * FlushAll() from inside OnFlush). Without protection that would invoke the
 * same hook again, and so on without end. Each hook has its own tag, so
 * nested calls into a *different* hook still fire normally.
 */

namespace vigil::detail {

/// @name Hook tags
/// Each tag identifies one hook and owns an independent per-thread flag.
/// @{
struct LevelChangeHook {};
struct FlushHook {};
struct ShutdownHook {};
/// @}

/// @brief Per-thread flag: true while the current thread is inside the hook @p Tag.
template <typename Tag>
inline thread_local bool g_InHook = false;

/// @brief Prevents recursive invocation of a specific hook on the current thread.
/// @tparam Tag Identifies the hook protected by this scope.
template <typename Tag>
class HookScope
{
public:
    HookScope() noexcept
        : m_Owns(!g_InHook<Tag>)
    {
        if (m_Owns) g_InHook<Tag> = true;
    }

    ~HookScope() noexcept
    {
        if (m_Owns) g_InHook<Tag> = false;
    }

    /// @brief Checks whether this scope acquired the hook protection.
    /// @return True if the hook can be invoked; false if it is already active.
    explicit operator bool() const noexcept { return m_Owns; }

    HookScope(const HookScope&) = delete;
    HookScope& operator=(const HookScope&) = delete;

private:
    bool m_Owns;
};

/**
 * @brief Invokes a hook callback unless it is empty or already running on this thread.
 *
 * Must be called after every internal lock has been released: the callback is
 * user code and may call back into LogSystem.
 *
 * Exceptions thrown by @p callback propagate to the caller. The reentrancy
 * flag is restored during stack unwinding.
 *
 * @tparam Tag      Identifies the hook (see @ref HookScope).
 * @tparam Callback Callable type (typically a std::function).
 * @tparam Args     Argument types forwarded to the callback.
 * @param callback  Callback to invoke. Ignored if empty.
 * @param args      Arguments forwarded to @p callback.
 *
 * @note Not suitable for hooks where a nested call must also skip other work
 *       (such as Shutdown()). Use @ref HookScope directly there.
 */
template <typename Tag, typename Callback, typename... Args>
void FireHook(const Callback& callback, Args&&... args)
{
    if (!callback)
        return;

    HookScope<Tag> scope;
    if (scope)
        callback(std::forward<Args>(args)...);
}

} // namespace vigil::detail
