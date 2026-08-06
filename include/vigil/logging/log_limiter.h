// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"
#include "vigil/logging/log_level.h"

#include <string>

/**
 * @file log_limiter.h
 * @brief Rate-limiting helpers to avoid log spam from hot code paths.
 */

namespace vigil {

/// @brief Ensures a message tied to a given key is only ever logged once,
/// for the lifetime of the process (thread-safe).
class VIGIL_API LogOncePolicy {
public:
    LogOncePolicy() = delete;

    /// @brief True the first time this key is seen, false on every call after.
    /// @param key Unique identifier for the log message (e.g. function name).
    /// @return True if this is the first time @p key has been seen, false otherwise.
    [[nodiscard]] static bool ShouldLog(const std::string& key);

    /// @brief Logs @p msg on the main logger, only the first time @p key is seen.
    static void LogOnce(const std::string& key, LogLevel level, const std::string& msg);
};

/// @brief Rate-limits log messages so a given key is logged at most once per
/// TTL (time-to-live) window (thread-safe).
class VIGIL_API LogTTLPolicy {
public:
    LogTTLPolicy() = delete;

    /// @brief True if @p key has never been seen, or its TTL has expired.
    /// @param key        Unique identifier for the log message.
    /// @param ttlSeconds Minimum seconds between two logs of the same key.
    [[nodiscard]] static bool ShouldLog(const std::string& key, double ttlSeconds);

    /// @brief Logs @p msg on the main logger if the TTL for @p key has expired.
    static void LogTTL(const std::string& key, double ttlSeconds, LogLevel level, const std::string& msg);
};

} // namespace vigil
