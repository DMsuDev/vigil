# ==============================================================================
#  VIGIL - DEPENDENCIES CONFIGURATION
# ==============================================================================
#  Description: Resolves and configures third-party dependencies used by Vigil.
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

if(VIGIL_INSTALL AND NOT VIGIL_USE_SYSTEM_FMT)
  message(FATAL_ERROR
    "Vigil: VIGIL_INSTALL requires VIGIL_USE_SYSTEM_FMT=ON. "
    "Installing with a vendored fmt copy is not supported, "
    "as its targets are not exported.")
endif()

if(VIGIL_USE_SYSTEM_FMT)
  find_package(fmt CONFIG REQUIRED GLOBAL)
  list(APPEND PKG_CONFIG_REQUIRES fmt)
elseif(NOT TARGET fmt::fmt)
  set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)
  add_subdirectory("${FMT_DIR}")
endif()

# spdlog -----------------------------------------------------------------------

if(NOT TARGET spdlog::spdlog)
  # Use the project's fmt library instead of spdlog's bundled copy.
  set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)

  # Always build spdlog as a static library to keep it an internal dependency.
  set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build spdlog as a shared library" FORCE)

  set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "Disable spdlog tests" FORCE)
  set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "Disable spdlog examples" FORCE)
  set(SPDLOG_BUILD_BENCH   OFF CACHE BOOL "Disable spdlog benchmarks" FORCE)

  set(SPDLOG_BUILD_PIC ON CACHE BOOL "Enable position-independent code for spdlog" FORCE)

  add_subdirectory("${SPDLOG_DIR}")
endif()

# Join the list of dependencies into a single string for pkg-config generation.
list(JOIN PKG_CONFIG_REQUIRES " " PKG_CONFIG_REQUIRES)
