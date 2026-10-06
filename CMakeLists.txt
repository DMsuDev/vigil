# ==============================================================================
#  VIGIL - ROOT CMAKE CONFIGURATION
# ==============================================================================
#  Description: Main project orchestrator. Manages configuration options,
#               global build settings, internal dependencies, and subdirectories.
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

cmake_minimum_required(VERSION 3.28)

# ------------------------------------------------------------------------------
#  Modules & Helpers
# ------------------------------------------------------------------------------
set(VIGIL_BASE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH
  "${VIGIL_BASE_FOLDER}/cmake"
  "${VIGIL_BASE_FOLDER}/cmake/vendor"
)

# ------------------------------------------------------------------------------
#  Version
# ------------------------------------------------------------------------------
include(vigil_version)
vigil_load_version(VIGIL_VERSION VIGIL_PLAIN_VERSION)

project(Vigil
  LANGUAGES   C CXX
  VERSION     ${VIGIL_PLAIN_VERSION}
  DESCRIPTION "Cross-platform diagnostics library for C++."
)
message(STATUS "${PROJECT_NAME} version: ${VIGIL_VERSION}")
include("${VIGIL_BASE_FOLDER}/cmake/build_helpers.cmake")

vigil_configure_cmake()

# ------------------------------------------------------------------------------
#  Configuration Options
# ------------------------------------------------------------------------------

option(VIGIL_BUILD_SHARED          "Build Vigil as a shared library."                                  ${BUILD_SHARED_LIBS})
option(VIGIL_ENABLE_COVERAGE       "Enable code coverage instrumentation."                             OFF)
option(VIGIL_INSTALL               "Generate the install target."                                      OFF)
option(VIGIL_CRASH_TESTS           "Enable Vigil crash integration tests."                             OFF)
option(VIGIL_CONSUMER_TESTS        "Enable CMake consumer integration tests."                          OFF)
option(VIGIL_INSTALL_TESTS_CLEANUP "Delete generated consumer project dirs after install tests run."   ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_BUILD_TESTS           "Build Vigil's unit-test suite."                                    ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_BUILD_EXAMPLES        "Build Vigil's example programs."                                   ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_BUILD_WARNINGS        "Enable strict compiler warning suites."                            ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_WARNINGS_AS_ERRORS    "Treat compiler warnings as hard errors."                           OFF)
option(VIGIL_ENABLE_STACK_TRACE    "Enable cross-platform call stack capture and symbolication"        ON )
option(VIGIL_USE_SYSTEM_FMT        "Use system-installed fmt library"                                  ${VIGIL_INSTALL})

include(debug_default_options)
vigil_option_debug_default(VIGIL_ENABLE_ASSERTS
  "Enable runtime assertions."
  OFF
  ON
)
vigil_option_debug_default(VIGIL_ENABLE_SCOPED_LOG
  "Instrument scopes with entry/exit trace logging."
  OFF
  ON
)

# ------------------------------------------------------------------------------
#  Option Validation & Derived Settings
# ------------------------------------------------------------------------------

message(STATUS "Vigil: Using system-installed fmt library: ${VIGIL_USE_SYSTEM_FMT}")

if (DEFINED CMAKE_TOOLCHAIN_FILE)
  message(STATUS "Using toolchain file: \"${CMAKE_TOOLCHAIN_FILE}\"")
endif()

if(VIGIL_INSTALL AND NOT VIGIL_USE_SYSTEM_FMT)
  message(WARNING
    "[Vigil] VIGIL_USE_SYSTEM_FMT=OFF is incompatible with VIGIL_INSTALL=ON "
    "(fmt appears in Vigil's public headers). Forcing VIGIL_USE_SYSTEM_FMT=ON.")
  set(VIGIL_USE_SYSTEM_FMT ON CACHE BOOL "Use system-installed fmt instead of the vendored copy." FORCE)
endif()

if(VIGIL_WARNINGS_AS_ERRORS AND NOT VIGIL_BUILD_WARNINGS)
  message(WARNING
    "[Vigil] VIGIL_WARNINGS_AS_ERRORS=ON requires VIGIL_BUILD_WARNINGS=ON. "
    "Either set both to ON or disable WARNINGS_AS_ERRORS.")
endif()

if(VIGIL_INSTALL_TESTS_CLEANUP AND NOT VIGIL_BUILD_TESTS)
  message(WARNING "[Vigil] VIGIL_INSTALL_TESTS_CLEANUP=ON requires VIGIL_BUILD_TESTS=ON.")
endif()

if(VIGIL_ENABLE_COVERAGE AND NOT VIGIL_BUILD_TESTS)
  message(FATAL_ERROR "[Vigil] VIGIL_ENABLE_COVERAGE requires VIGIL_BUILD_TESTS=ON.")
endif()

# ------------------------------------------------------------------------------
#  Global Settings
# ------------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------------------------
#  Output Layout  (top-level only)
# ------------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
  # Isolate artifact generation inside the local binary footprint
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif()

# ------------------------------------------------------------------------------
#  Generated Configuration / Versioning Headers
# ------------------------------------------------------------------------------
configure_file(
  "${VIGIL_BASE_FOLDER}/include/vigil/version.h.in"
  "${PROJECT_BINARY_DIR}/include/vigil/version.h"
  @ONLY
)

# ------------------------------------------------------------------------------
#  Third-Party / Vendor Dependencies
# ------------------------------------------------------------------------------
add_subdirectory(vendor)

# ------------------------------------------------------------------------------
#  Library Sources Processing
# ------------------------------------------------------------------------------
add_subdirectory(src)

if(VIGIL_BUILD_EXAMPLES)
  message(STATUS "Vigil: Example targets are enabled.")
  add_subdirectory(examples)
endif()

if(VIGIL_BUILD_TESTS OR VIGIL_CONSUMER_TESTS OR VIGIL_CRASH_TESTS)
  message(STATUS "Vigil: Test suite execution is enabled.")
  enable_testing()
  add_subdirectory(tests)
endif()
