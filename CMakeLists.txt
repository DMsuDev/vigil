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
list(APPEND CMAKE_MODULE_PATH "${VIGIL_BASE_FOLDER}/cmake")

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

# ------------------------------------------------------------------------------
#  Configuration Options
# ------------------------------------------------------------------------------
# Defaults are tied to PROJECT_IS_TOP_LEVEL so that consumer builds are never
# affected by Vigil's own development settings.

option(VIGIL_BUILD_SHARED        "Build Vigil as a shared library."                              ${BUILD_SHARED_LIBS})
option(VIGIL_INSTALL             "Generate the install target."                                  OFF)
option(VIGIL_BUILD_TESTS         "Build Vigil's unit-test suite."                                ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_BUILD_EXAMPLES      "Build Vigil's example programs."                               ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_BUILD_WARNINGS      "Enable strict compiler warning suites."                        ${PROJECT_IS_TOP_LEVEL})
option(VIGIL_WARNINGS_AS_ERRORS  "Treat compiler warnings as hard errors."                       OFF)
option(VIGIL_ENABLE_STACK_TRACE  "Enable cross-platform call stack capture and symbolication"    ON )
option(VIGIL_ENABLE_SCOPED_LOG   "Instrument scopes with entry/exit trace logging ."             OFF)
option(VIGIL_USE_SYSTEM_FMT      "Use system-installed fmt library"                               ${VIGIL_INSTALL})

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

# Enable scoped logging automatically in Debug builds unless explicitly set.
if(NOT DEFINED VIGIL_ENABLE_SCOPED_LOG OR NOT VIGIL_ENABLE_SCOPED_LOG)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
    set(VIGIL_ENABLE_SCOPED_LOG ON CACHE BOOL "Instrument scopes with entry/exit trace logging." FORCE)
    message(STATUS "Vigil: Scoped logging enabled automatically for Debug build.")
  endif()
endif()

# ------------------------------------------------------------------------------
#  Global Settings
# ------------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(compiler_options)
include(compiler_warnings)
include(copy_runtime_deps)

# ------------------------------------------------------------------------------
#  Output Layout & Platform Defaults  (top-level only)
# ------------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)

  # Standard install directories (bin, lib, include, etc.) with platform-specific defaults.
  include(GNUInstallDirs)

  # Isolate artifact generation inside the local binary footprint
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

  # Single-config generators: default to Release when no build type is set.
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()

  if(VIGIL_BUILD_SHARED)
    if(APPLE)
      set(CMAKE_INSTALL_RPATH "@loader_path")
    elseif(UNIX)
      set(CMAKE_INSTALL_RPATH "$ORIGIN")
    endif()
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
  endif()
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

if(VIGIL_BUILD_TESTS)
  message(STATUS "Vigil: Test suite execution is enabled.")
  enable_testing()
  add_subdirectory(tests)
endif()
