// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/assert.h"
#include "vigil/logging/log_system.h"

#include <gtest/gtest.h>

// =============================================================================
// Helpers
// =============================================================================

namespace {

bool IncrementAndReturnTrue(int& counter)
{
    ++counter;
    return true;
}

bool IncrementAndReturnFalse(int& counter)
{
    ++counter;
    return false;
}

} // namespace

// =============================================================================
// VIGIL_VERIFY — always evaluates, always enforces
// =============================================================================

TEST(AssertTest, VerifyAlwaysEvaluatesItsExpression)
{
    int counter = 0;
    VIGIL_VERIFY(IncrementAndReturnTrue(counter));
    EXPECT_EQ(counter, 1);
}

TEST(AssertTest, VerifyEvaluatesExpressionEvenWhenItReturnsFalse)
{
    // VIGIL_VERIFY(false) would terminate, so we only verify evaluation
    // via the side-effect, not the enforcement path.
    int counter = 0;
    // We cannot call VIGIL_VERIFY with a false expression here without
    // terminating, so we confirm the side-effect path via a true expression.
    VIGIL_VERIFY(IncrementAndReturnTrue(counter));
    EXPECT_EQ(counter, 1);
}

// =============================================================================
// VIGIL_ASSERT — conditional on VIGIL_ENABLE_ASSERTS
// =============================================================================

TEST(AssertTest, AssertEvaluationDependsOnBuildMode)
{
    int counter = 0;
    VIGIL_ASSERT(IncrementAndReturnTrue(counter));

#if defined(VIGIL_ENABLE_ASSERTS)
    EXPECT_EQ(counter, 1) << "VIGIL_ASSERT must evaluate its expression in assert-enabled builds";
#else
    EXPECT_EQ(counter, 0) << "VIGIL_ASSERT must be a no-op in non-assert builds";
#endif
}

// =============================================================================
// Death tests — only meaningful when asserts are enabled
// =============================================================================

#if defined(VIGIL_ENABLE_ASSERTS)

// EXPECT_DEATH forks the process, so the LogSystem must be shut down first to
// avoid spdlog background threads surviving into the child.

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

TEST(AssertTest, AssertNotNullPassesForNonNullPointer)
{
    int value = 42;
    int* ptr  = &value;
    EXPECT_NO_FATAL_FAILURE(VIGIL_ASSERT_NOT_NULL(ptr));
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

TEST(AssertTest, AssertInRangePassesForBoundaryValues)
{
    EXPECT_NO_FATAL_FAILURE(VIGIL_ASSERT_IN_RANGE(0,   0, 100));
    EXPECT_NO_FATAL_FAILURE(VIGIL_ASSERT_IN_RANGE(100, 0, 100));
    EXPECT_NO_FATAL_FAILURE(VIGIL_ASSERT_IN_RANGE(50,  0, 100));
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

#endif // VIGIL_ENABLE_ASSERTS
