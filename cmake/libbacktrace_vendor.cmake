# ==============================================================================
#  VIGIL - LIBBACKTRACE VENDOR MODULE
# ==============================================================================
#  Description: Configures the vendored libbacktrace dependency for POSIX
#               platforms and exposes it through the libbacktrace::libbacktrace
#               target.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include_guard()

# ------------------------------------------------------------------------------
#  Configure libbacktrace as an ExternalProject
# ------------------------------------------------------------------------------

include(ExternalProject)

set(LIBBACKTRACE_SOURCE_DIR  "${PROJECT_SOURCE_DIR}/vendor/libbacktrace")
set(LIBBACKTRACE_BINARY_DIR  "${PROJECT_BINARY_DIR}/_deps/libbacktrace")
set(LIBBACKTRACE_INSTALL_DIR "${LIBBACKTRACE_BINARY_DIR}/install")
set(LIBBACKTRACE_LIBRARY     "${LIBBACKTRACE_INSTALL_DIR}/lib/libbacktrace.a")

file(MAKE_DIRECTORY "${LIBBACKTRACE_INSTALL_DIR}/include")

ExternalProject_Add(libbacktrace
  SOURCE_DIR        "${LIBBACKTRACE_SOURCE_DIR}"
  BINARY_DIR        "${LIBBACKTRACE_BINARY_DIR}/build"
  INSTALL_DIR       "${LIBBACKTRACE_INSTALL_DIR}"
  CONFIGURE_COMMAND
    "${CMAKE_COMMAND}" -E env
    "CC=${CMAKE_C_COMPILER}"
    "CFLAGS=${CMAKE_C_FLAGS}"
    /bin/sh
    "${LIBBACKTRACE_SOURCE_DIR}/configure"
    "--prefix=<INSTALL_DIR>"
    "--enable-shared=no"
    "--with-pic"
  BUILD_COMMAND    make -j
  INSTALL_COMMAND  make install
  BUILD_BYPRODUCTS "${LIBBACKTRACE_LIBRARY}"
  UPDATE_COMMAND ""
)

# ------------------------------------------------------------------------------
#  Create the Internal Imported Target
# ------------------------------------------------------------------------------

add_library(vigil::libbacktrace STATIC IMPORTED GLOBAL)

set_target_properties(vigil::libbacktrace PROPERTIES
  IMPORTED_LOCATION
    "${LIBBACKTRACE_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES
    "${LIBBACKTRACE_INSTALL_DIR}/include"
)

# Ensure the static library exists before anything tries to link against it.
add_dependencies(vigil::libbacktrace libbacktrace)
