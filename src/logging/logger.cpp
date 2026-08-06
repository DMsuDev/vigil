// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/logger.h"

#include "logging/detail/logger_impl.h"
#include "logging/detail/spd_convert.h"

namespace vigil {

Logger::Logger(Shared<detail::LoggerImpl> impl) noexcept
    : m_Impl(std::move(impl))
{}

std::string_view Logger::GetName() const noexcept
{
    return m_Impl->m_Logger->name();
}

void Logger::SetLevel(LogLevel level)
{
    m_Impl->SetLevel(level);
}

LogLevel Logger::GetLevel() const noexcept
{
    return m_Impl->GetLevel();
}

void Logger::Flush()
{
    m_Impl->Flush();
}

void Logger::Trace(std::string_view message)
{
    LogImpl(LogLevel::Trace, std::string{message});
}
void Logger::Debug(std::string_view message)
{
    LogImpl(LogLevel::Debug, std::string{message});
}
void Logger::Info(std::string_view message)
{
    LogImpl(LogLevel::Info, std::string{message});
}
void Logger::Warn(std::string_view message)
{
    LogImpl(LogLevel::Warn, std::string{message});
}
void Logger::Error(std::string_view message)
{
    LogImpl(LogLevel::Error, std::string{message});
}
void Logger::Critical(std::string_view message)
{
    LogImpl(LogLevel::Critical, std::string{message});
}
void Logger::Log(LogLevel level, std::string_view message)
{
    LogImpl(level, std::string{message});
}

void Logger::LogImpl(LogLevel level, const std::string& formatted)
{
    m_Impl->Log(level, formatted);
}

} // namespace vigil
