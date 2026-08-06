// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "logger_impl.h"
#include "spd_convert.h"

namespace vigil::detail {

void LoggerImpl::Flush()
{
    m_Logger->flush();
}

void LoggerImpl::Log(LogLevel level, const std::string_view & message)
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

} // namespace vigil::detail
