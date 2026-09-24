# ==============================================================================
#  Vigil - Crash Integration Test Harness
#
#  Builds a standalone consumer project that vendors Vigil directly through
#  add_subdirectory() and exercises Vigil's crash handling through individual
#  triggers.
#
#  Each trigger is registered as a separate CTest test case.
#
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include(CTest)

# ------------------------------------------------------------------------------
# Crash triggers
#
# These triggers are expected to terminate the consumer process through Vigil's
# crash handling infrastructure.
# ------------------------------------------------------------------------------
set(VIGIL_CRASH_TRIGGERS
  sigsegv
  sigabrt
  sigfpe
  sigill
  terminate
  pure_virtual
  bad_alloc
)

if(WIN32)
  list(APPEND VIGIL_CRASH_TRIGGERS invalid_param)
else()
  # TODO: Fix stack overflow handling on Windows platforms.
  list(APPEND VIGIL_CRASH_TRIGGERS stack_overflow)
endif()

# ------------------------------------------------------------------------------
# Consumer build directory
# ------------------------------------------------------------------------------

set(vigil_crash_binary_dir
  "${CMAKE_CURRENT_BINARY_DIR}/project"
)

# ------------------------------------------------------------------------------
# Configure and build standalone consumer
#
# The consumer vendors Vigil directly via add_subdirectory().
# ------------------------------------------------------------------------------

add_test(NAME vigil_crash_consumer
  COMMAND "${CMAKE_CTEST_COMMAND}"
    --build-and-test
      "${CMAKE_CURRENT_SOURCE_DIR}/project"
      "${vigil_crash_binary_dir}"
    --build-generator "${CMAKE_GENERATOR}"
    --build-config    "${CMAKE_BUILD_TYPE}"
    --build-options
      "-DVIGIL_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
      "-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
      "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
)

set_tests_properties(vigil_crash_consumer PROPERTIES
  FIXTURES_SETUP "vigil_crash_tests_fixture"
  LABELS "vigil;crash"
)

# ------------------------------------------------------------------------------
# Run crash tests
# ------------------------------------------------------------------------------

foreach(trigger IN LISTS VIGIL_CRASH_TRIGGERS)

  add_test(NAME "vigil_crash::${trigger}"
    COMMAND
      "${vigil_crash_binary_dir}/bin/vigil_crash_test"
      "${trigger}"
  )

  set_tests_properties("vigil_crash::${trigger}" PROPERTIES
    FIXTURES_REQUIRED "vigil_crash_tests_fixture"
    WILL_FAIL TRUE
    TIMEOUT 10
    LABELS "vigil;crash"
  )

endforeach()

# ------------------------------------------------------------------------------
# Optional cleanup
# ------------------------------------------------------------------------------

if(VIGIL_INSTALL_TESTS_CLEANUP)

  add_test(NAME vigil_crash_consumer_cleanup
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
      "${vigil_crash_binary_dir}"
  )

  set_tests_properties(vigil_crash_consumer_cleanup PROPERTIES
    FIXTURES_CLEANUP "vigil_crash_tests_fixture"
    LABELS "vigil;crash"
  )

endif()
