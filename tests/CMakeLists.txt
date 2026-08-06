# ==============================================================================
#  VIGIL - TEST SUITE
# ==============================================================================
#  Description: Builds Vigil's unit tests against GoogleTest.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

# ------------------------------------------------------------------------------
#  Dependency: GoogleTest
# ------------------------------------------------------------------------------
if(VIGIL_USE_SYSTEM_LIBS)
  find_package(GTest CONFIG REQUIRED)
else()
  if(NOT TARGET GTest::gtest)
    include(FetchContent)

    # Match MSVC runtime settings to avoid mismatches on Windows
    set(gtest_force_shared_crt ON  CACHE BOOL "Use shared CRT for gtest" FORCE)

    # Disable building the gmock framework to reduce build footprint.
    set(BUILD_GMOCK OFF CACHE BOOL "Disable gmock framework" FORCE)

    # Disable installation rules for GoogleTest to avoid polluting system paths.
    set(INSTALL_GTEST OFF CACHE BOOL "Disable gtest installation rules" FORCE)

    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        v1.17.0
      GIT_SHALLOW    TRUE
      SYSTEM
    )
    FetchContent_MakeAvailable(googletest)
  endif()
endif()

# Load the native GoogleTest discovery modules
include(GoogleTest)

# ------------------------------------------------------------------------------
#  Source File Maps
# ------------------------------------------------------------------------------
set(VIGIL_GTEST_SOURCES
  test_async.cpp
  test_log_limiter.cpp
  test_logger.cpp
  test_logger_registry.cpp
)

# ------------------------------------------------------------------------------
#  Test Binary Target Initialization
# ------------------------------------------------------------------------------
add_executable(vigil_tests ${VIGIL_GTEST_SOURCES})

target_link_libraries(vigil_tests
  PRIVATE
    vigil::vigil
    spdlog::spdlog
    GTest::gtest
    GTest::gtest_main
)

target_include_directories(vigil_tests
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
)

# ------------------------------------------------------------------------------
#  Target Policies & System Adjustments
# ------------------------------------------------------------------------------
vigil_enable_warnings(vigil_tests)
vigil_set_compiler_options(vigil_tests)

# Ensures that if Vigil or its vendor components are shared DLL assets on Windows,
# they are copied next to the test binary to allow running via CTest seamlessly.
vigil_copy_runtime_dependencies(vigil_tests)

set_target_properties(vigil_tests PROPERTIES
  FOLDER "Vigil/Tests"
)

# ------------------------------------------------------------------------------
#  Automated Test Discovery Registry
# ------------------------------------------------------------------------------
gtest_discover_tests(vigil_tests
  PROPERTIES
    LABELS "vigil"
)
