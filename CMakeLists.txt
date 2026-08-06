# ==============================================================================
#  VIGIL - ROOT CMAKE CONFIGURATION
# ==============================================================================
#  Description: Main project orchestrator. Manages configuration options,
#               global build settings, internal dependencies, and subdirectories.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

cmake_minimum_required(VERSION 3.28)

project(Vigil
  VERSION 0.1.1
  LANGUAGES CXX
  DESCRIPTION "Cross-platform diagnostics library for C++."
)
message(STATUS "Configuring ${PROJECT_NAME}: v${PROJECT_VERSION}")

# ------------------------------------------------------------------------------
#  Project-Wide Configuration Options
# ------------------------------------------------------------------------------
# Default behaviors are tied directly to PROJECT_IS_TOP_LEVEL. This prevents
# subproject builds from altering the consumer's output directory layout.

option(VIGIL_BUILD_SHARED "Build Vigil as a shared library." ${BUILD_SHARED_LIBS})
option(VIGIL_INSTALL      "Generate the install target." ${PROJECT_IS_TOP_LEVEL})

option(VIGIL_BUILD_EXAMPLES "Build Vigil's example programs." ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_BUILD_TESTS    "Build Vigil's unit-test suite."  ${PROJECT_IS_TOP_LEVEL})

option(VIGIL_BUILD_WARNINGS     "Enable strict compiler warning suites."  ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_WARNINGS_AS_ERRORS "Treat compiler warnings as hard errors." OFF)

option(VIGIL_ENABLE_STACK_TRACE "Enable cross-platform call stack capture and symbolication" ON)

# ------------------------------------------------------------------------------
#  Dependency Management Options
# ------------------------------------------------------------------------------
set(vigil_use_system_default  OFF)
if(NOT PROJECT_IS_TOP_LEVEL OR VIGIL_INSTALL)
  set(vigil_use_system_default  ON)
endif()

option(VIGIL_USE_SYSTEM_FMT "Use system-installed fmt library" ${vigil_use_system_default})
message(STATUS "Vigil: Using system-installed fmt library: ${VIGIL_USE_SYSTEM_FMT}")

# ------------------------------------------------------------------------------
#  Global Core Modules & System Paths
# ------------------------------------------------------------------------------

# Standard install directories (bin, lib, include, etc.) with platform-specific defaults.
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(compiler_options)
include(compiler_warnings)
include(copy_runtime_deps)

# ------------------------------------------------------------------------------
#  Isolated Target Architecture & Output Layout
# ------------------------------------------------------------------------------

if(PROJECT_IS_TOP_LEVEL)
  # Isolate artifact generation inside the local binary footprint
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

  # Fallback to standard Release profile if none is specified for single-config generators
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()
endif()

# ------------------------------------------------------------------------------
#  Generated Configuration / Versioning Headers
# ------------------------------------------------------------------------------
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/vigil/version.h.in"
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

if(VIGIL_BUILD_TESTS)
  message(STATUS "Vigil: Test suite execution is enabled.")
  enable_testing()
  add_subdirectory(tests)
endif()
