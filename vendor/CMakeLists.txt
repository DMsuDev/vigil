# ==============================================================================
#  VIGIL - DEPENDENCIES CONFIGURATION
# ==============================================================================
#  Description: Integrates external libraries (fmt, spdlog) into the build tree.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

# ------------------------------------------------------------------------------
#  Path Definitions for Dependencies
# ------------------------------------------------------------------------------
set(FMT_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/fmt")
set(SPDLOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spdlog")

# ------------------------------------------------------------------------------
#  Dependency Resolution
# ------------------------------------------------------------------------------
set(PKG_CONFIG_REQUIRES "")

if(VIGIL_USE_SYSTEM_FMT)
  find_package(fmt CONFIG REQUIRED)
  list(APPEND PKG_CONFIG_REQUIRES fmt)
else()
  if(NOT TARGET fmt::fmt)
    set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)
    add_subdirectory("${FMT_DIR}")
  endif()
endif()

if(NOT TARGET spdlog::spdlog)
  # Tell spdlog to use the project's fmt library.
  set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)

  # Always build spdlog as a static library.
  set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build spdlog as a shared library" FORCE)

  set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "Disable spdlog tests" FORCE)
  set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "Disable spdlog examples" FORCE)
  set(SPDLOG_BUILD_BENCH   OFF CACHE BOOL "Disable spdlog benchmarks" FORCE)

  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/spdlog")
endif()

# Join the list of dependencies into a single string for pkg-config generation.
list(JOIN PKG_CONFIG_REQUIRES " " PKG_CONFIG_REQUIRES)
