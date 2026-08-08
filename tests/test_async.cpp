// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/log_system.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>

using namespace vigil;

// Inspired by spdlog's own vendor/spdlog/tests/test_async.cpp: rather than
// sleeping and hoping the background worker has caught up, every message is
// logged *inside* the ScopedRegistry's scope, and the assertion happens only
// after it goes out of scope. ScopedRegistry's dtor calls Shutdown(), which
// tears down spdlog's shared thread pool -- and that teardown posts a
// "terminate" message that blocks until every previously-queued message has
// drained (see spdlog::details::thread_pool::~thread_pool).

TEST(AsyncTest, MainLoggerDeliversEveryMessageOnceTheRegistryShutsDown)
{
    std::shared_ptr<vigil::test::TestSinkMt> sink;
    constexpr size_t messages = 200;

    {
        LogSystemConfig config;
        config.Name = "async_delivery_test";
        config.LogFile = "test_async_delivery.log";
        config.Async = true;
        vigil::test::ScopedRegistry registry(config);

        sink = vigil::test::AttachTestSink(LogSystem::Main());

        for (size_t i = 0; i < messages; ++i)
            LogSystem::Main().Info("Hello message #{}", i);
    }

    ASSERT_EQ(sink->msg_counter(), messages);
}
