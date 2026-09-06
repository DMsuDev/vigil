// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "logging/detail/logger_impl.h"
#include "logging/detail/spd_convert.h"

namespace vigil::detail {

LoggerImpl::LoggerImpl(Shared<spdlog::logger> logger,
                       spdlog::sink_ptr       consoleSink,
                       spdlog::sink_ptr       fileSink)
    : m_Logger(std::move(logger))
    , m_ConsoleSink(std::move(consoleSink))
    , m_FileSink(std::move(fileSink))
{}

void LoggerImpl::Log(LogLevel level, std::string_view message)
{
    m_Logger->log(ToSpdLevel(level), message);
}

void LoggerImpl::SetLevel(LogLevel level)
{
    m_Logger->set_level(ToSpdLevel(level));
}

LogLevel LoggerImpl::GetLevel() const noexcept
{
    return FromSpdLevel(m_Logger->level());
}

void LoggerImpl::Flush()
{
    m_Logger->flush();
}

VIGIL_API void LoggerImpl::AttachSink(spdlog::sink_ptr sink)
{
    m_Logger->sinks().push_back(std::move(sink));
}

Shared<spdlog::logger> LoggerImpl::SpdLogger() const noexcept
{
    return m_Logger;
}

spdlog::sink_ptr LoggerImpl::FileSink() const noexcept
{
    return m_FileSink;
}

spdlog::sink_ptr LoggerImpl::ConsoleSink() const noexcept
{
    return m_ConsoleSink;
}

std::string_view LoggerImpl::Name() const noexcept
{
    return m_Logger->name();
}

} // namespace vigil::detail
