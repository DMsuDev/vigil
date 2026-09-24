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

#include <cstdlib>

namespace vigil::crash_triggers {

namespace detail {

struct Base
{
    virtual void Call() = 0;
    virtual ~Base()     = default;
};

struct Derived : Base
{
    void Call() override { std::abort(); } // never reached
};

} // namespace detail

[[noreturn]] inline void TriggerPureVirtual()
{
    struct Caller : detail::Base
    {
        Caller() { static_cast<detail::Base*>(this)->Call(); }
        void Call() override {}
    };

    // Placement new on a stack buffer so we control the object lifetime.
    alignas(Caller) unsigned char buf[sizeof(Caller)];
    ::new (buf) Caller();

    VIGIL_UNREACHABLE();
}

} // namespace vigil::crash_triggers
