// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/log_limiter.h"
#include "vigil/logging/log_system.h"

#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace vigil {

//==============================================================================
// LogOncePolicy
//==============================================================================

bool LogOncePolicy::ShouldLog(const std::string& key)
{
    static std::unordered_set<std::string> s_LoggedKeys;
    static std::mutex s_Mutex;

    std::scoped_lock lock(s_Mutex);
    return s_LoggedKeys.insert(key).second;
}

void LogOncePolicy::LogOnce(const std::string& key, LogLevel level, const std::string& msg)
{
    if (ShouldLog(key))
        LogSystem::Main().Log(level, "{}", msg);
}

//==============================================================================
// LogTTLPolicy
//==============================================================================

bool LogTTLPolicy::ShouldLog(const std::string& key, double ttlSeconds)
{
    using Clock = std::chrono::steady_clock;

    static std::unordered_map<std::string, Clock::time_point> s_Timestamps;
    static std::mutex s_Mutex;

    std::scoped_lock lock(s_Mutex);
    auto now = Clock::now();

    auto it = s_Timestamps.find(key);
    if (it == s_Timestamps.end()) {
        s_Timestamps.emplace(key, now);
        return true;
    }

    double elapsed = std::chrono::duration<double>(now - it->second).count();
    if (elapsed >= ttlSeconds) {
        it->second = now;
        return true;
    }

    return false;
}

void LogTTLPolicy::LogTTL(const std::string& key, double ttlSeconds, LogLevel level, const std::string& msg)
{
    if (ShouldLog(key, ttlSeconds))
        LogSystem::Main().Log(level, "{}", msg);
}

} // namespace vigil
