// -----------------------------------------------------------------------------
//   _    __ __ _____ ___ __
//  | |  / /  _/ ____/  _/ /
//  | | / // // / __ / // /     Logging and Diagnostics for C++
//  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
//  |___/___/\____/___/_____/
//
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/preprocessor_utils.h"
#include "vigil/detail/compiler_attributes.h"

namespace vigil::crash_triggers {

namespace detail {

struct Base
{
    Base() { InvokeCall(); }

    virtual void Call() = 0;
    virtual ~Base()     = default;

private:
    VIGIL_NOINLINE void InvokeCall() { Call(); }
};

} // namespace detail

[[noreturn]] inline void TriggerPureVirtual()
{
    struct Caller : detail::Base
    {
        void Call() override {}
    };

    Caller caller;
}

} // namespace vigil::crash_triggers
