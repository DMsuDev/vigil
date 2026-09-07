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

#include "vigil/logging/log_system.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <memory>

using namespace vigil;

// Drain guarantee: every message is logged *inside* the ScopedRegistry scope.
// The registry destructor calls Shutdown(), which tears down spdlog's shared
// thread pool. That teardown posts a "terminate" token that blocks until every
// previously-queued item has been processed, so by the time the EXPECT runs
// all messages are guaranteed to have reached the sink.
// (See spdlog::details::thread_pool::~thread_pool.)

// How many messages to send. Large enough to queue across multiple pool
// iterations, small enough to keep the test fast.
static constexpr size_t kAsyncMessages = 100;

TEST(AsyncTest, MainLoggerDeliversEveryMessageAfterShutdown)
{
    std::shared_ptr<vigil::test::TestSinkMt> sink;

    {
        LogSystemConfig config;
        config.Name  = "async_main_delivery";
        config.Async = true;
        vigil::test::ScopedRegistry registry(config);

        sink = vigil::test::AttachTestSink(LogSystem::Main());

        for (size_t i = 0; i < kAsyncMessages; ++i)
            LogSystem::Main().Info("message #{}", i);
    } // Shutdown() drains the queue here.

    EXPECT_EQ(sink->msg_counter(), kAsyncMessages)
        << "Not all async messages were delivered before shutdown";
}

TEST(AsyncTest, NamedLoggerCanBeAsyncWhileMainIsSynchronous)
{
    std::shared_ptr<vigil::test::TestSinkMt> sink;

    {
        LogSystemConfig config;
        config.Name  = "async_named_override";
        config.Async = false; // main logger is synchronous
        vigil::test::ScopedRegistry registry(config);

        LogConfig namedConfig;
        namedConfig.Name  = "async_worker";
        namedConfig.Async = true;
        auto& named = LogSystem::Create(namedConfig);

        sink = vigil::test::AttachTestSink(named);

        for (size_t i = 0; i < kAsyncMessages; ++i)
            named.Info("message #{}", i);
    } // Shutdown() drains the queue here.

    EXPECT_EQ(sink->msg_counter(), kAsyncMessages)
        << "Not all async messages from the named logger were delivered before shutdown";
}

TEST(AsyncTest, AsyncMainLoggerDoesNotDeliverMessagesToSynchronousNamedLogger)
{
    // Verifies that sinks are per-logger: messages to the async main logger
    // must not appear in a sink attached to a separate synchronous named logger.
    std::shared_ptr<vigil::test::TestSinkMt> namedSink;

    {
        LogSystemConfig config;
        config.Name  = "async_isolation";
        config.Async = true;
        vigil::test::ScopedRegistry registry(config);

        auto& named = LogSystem::Create("sync_observer");
        namedSink   = vigil::test::AttachTestSink(named);

        for (size_t i = 0; i < kAsyncMessages; ++i)
            LogSystem::Main().Info("main message #{}", i);
    }

    EXPECT_EQ(namedSink->msg_counter(), 0u)
        << "Messages from the main logger leaked into an unrelated named logger's sink";
}
