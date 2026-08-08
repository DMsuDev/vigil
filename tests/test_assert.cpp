// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/assert.h"
#include "vigil/logging/log_system.h"

#include <gtest/gtest.h>

namespace {

bool IncrementAndReturnTrue(int& counter)
{
    ++counter;
    return true;
}

} // namespace

TEST(AssertTest, VerifyAlwaysEvaluatesExpression)
{
    int counter = 0;

    VIGIL_VERIFY(IncrementAndReturnTrue(counter));

    ASSERT_EQ(counter, 1);
}

TEST(AssertTest, AssertEvaluationDependsOnBuildMode)
{
    int counter = 0;

    VIGIL_ASSERT(IncrementAndReturnTrue(counter));

#if defined(VIGIL_ENABLE_ASSERTS)
    ASSERT_EQ(counter, 1);
#else
    ASSERT_EQ(counter, 0);
#endif
}

#if defined(VIGIL_ENABLE_ASSERTS)

TEST(AssertTest, AssertFalseTerminatesProcess)
{
    EXPECT_DEATH(
        {
            ::vigil::LogSystem::Shutdown();
            VIGIL_ASSERT(false);
        },
        "Assertion failed");
}

TEST(AssertTest, AssertNotNullTerminatesOnNullPointer)
{
    EXPECT_DEATH(
        {
            ::vigil::LogSystem::Shutdown();
            int* ptr = nullptr;
            VIGIL_ASSERT_NOT_NULL(ptr);
        },
        "Assertion failed");
}

TEST(AssertTest, AssertInRangeTerminatesWhenValueIsOutOfRange)
{
    EXPECT_DEATH(
        {
            ::vigil::LogSystem::Shutdown();
            VIGIL_ASSERT_IN_RANGE(150, 0, 100);
        },
        "Assertion failed");
}

TEST(AssertTest, UnreachableAssertTerminatesProcess)
{
    EXPECT_DEATH(
        {
            ::vigil::LogSystem::Shutdown();
            VIGIL_UNREACHABLE_ASSERT();
        },
        "Assertion failed");
}

#endif
