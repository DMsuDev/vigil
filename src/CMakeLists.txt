# ==============================================================================
#  VIGIL - COMPONENT TARGET FILE (src)
# ==============================================================================
#  Description: Declares the core 'vigil' library target, populates source maps,
#               defines target properties, hooks up internal dependencies,
#               and establishes installation exports.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

# ------------------------------------------------------------------------------
#  Source File Definitions & Path Mapping
# ------------------------------------------------------------------------------
set(VIGIL_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(VIGIL_GENERATED_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")

set(VIGIL_SOURCES
  core/console.cpp

  logging/detail/logger_impl.cpp
  logging/logger.cpp
  logging/logger_registry.cpp
  logging/log_limiter.cpp

  detail/symbol_utils.cpp
  trace/stack_trace.cpp

  assert.cpp
)

set(VIGIL_HEADERS
  "${VIGIL_INCLUDE_DIR}/vigil/vigil.h"

  "${VIGIL_INCLUDE_DIR}/vigil/core/console.h"

  "${VIGIL_INCLUDE_DIR}/vigil/logging/logger.h"
  "${VIGIL_INCLUDE_DIR}/vigil/logging/logger_registry.h"
  "${VIGIL_INCLUDE_DIR}/vigil/logging/log_limiter.h"
  "${VIGIL_INCLUDE_DIR}/vigil/logging/log_level.h"
  "${VIGIL_INCLUDE_DIR}/vigil/logging/file_mode.h"

  "${VIGIL_INCLUDE_DIR}/vigil/assert.h"
)

set(VIGIL_GENERATED_HEADERS
  "${VIGIL_GENERATED_INCLUDE_DIR}/vigil/version.h"
)

# ------------------------------------------------------------------------------
# IDE Support
# ------------------------------------------------------------------------------
source_group(
  TREE "${VIGIL_INCLUDE_DIR}"
  PREFIX "Header Files"
  FILES ${VIGIL_HEADERS}
)

# ------------------------------------------------------------------------------
#  Target Initialization & Source Assignments
# ------------------------------------------------------------------------------
if(VIGIL_BUILD_SHARED)
  add_library(vigil SHARED)
else()
  add_library(vigil STATIC)
endif()

# This automates header installation and removes manual file copying commands.
target_sources(vigil
  PRIVATE
    ${VIGIL_SOURCES}
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS
      "${VIGIL_INCLUDE_DIR}"
      "${VIGIL_GENERATED_INCLUDE_DIR}"
    FILES
      ${VIGIL_HEADERS}
      ${VIGIL_GENERATED_HEADERS}
)

add_library(vigil::vigil ALIAS vigil)

# ------------------------------------------------------------------------------
#  Header Include Directories Resolution
# ------------------------------------------------------------------------------
target_include_directories(vigil
  PUBLIC
    $<BUILD_INTERFACE:${VIGIL_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${VIGIL_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# ------------------------------------------------------------------------------
#  Target Language Capabilities & Properties
# ------------------------------------------------------------------------------
target_compile_features(vigil PUBLIC cxx_std_17)

set_target_properties(vigil PROPERTIES
  CXX_EXTENSIONS OFF
  FOLDER         "Vigil"
)

# ------------------------------------------------------------------------------
#  Preprocessor Compile Definitions
# ------------------------------------------------------------------------------
target_compile_definitions(vigil
  PRIVATE
    # Windows-specific export symbol toggle when generating a shared .dll
    $<$<BOOL:${VIGIL_BUILD_SHARED}>:VIGIL_EXPORT_SYMBOLS>

    $<$<CONFIG:Debug>:VIGIL_BUILD_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:VIGIL_BUILD_RELEASE=1>

  PUBLIC
    # Tells consumers whether they are building against or consuming shared binary code
    $<$<BOOL:${VIGIL_BUILD_SHARED}>:VIGIL_SHARED_LIBRARY>

    # Enable assertion checks in Debug and RelWithDebInfo builds
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:VIGIL_ENABLE_ASSERTS>
)

# ------------------------------------------------------------------------------
#  Enable Optional Stack Trace Capture & Symbolication
# ------------------------------------------------------------------------------
if(VIGIL_ENABLE_STACK_TRACE)
  target_compile_definitions(vigil PUBLIC VIGIL_ENABLE_STACK_TRACE)

  if(WIN32)
    target_link_libraries(vigil PRIVATE dbghelp)
  endif()
endif()

# ------------------------------------------------------------------------------
#  Dependency Linking Setup
# ------------------------------------------------------------------------------
target_link_libraries(vigil
  PUBLIC
    fmt::fmt
  PRIVATE
    spdlog::spdlog
)

# ------------------------------------------------------------------------------
#  Compiler Flags, Diagnostics, and Warnings
# ------------------------------------------------------------------------------
vigil_enable_warnings(vigil)
vigil_set_compiler_options(vigil)

# ------------------------------------------------------------------------------
#  Package System Installation Architecture
# ------------------------------------------------------------------------------
if(VIGIL_INSTALL)
  message(STATUS "Configuring installation for Vigil.")

  # ----------------------------------------------------------------------------
  # Installation Paths & Variables
  # ----------------------------------------------------------------------------

  # Include standard helpers for package configuration file generation
  include(CMakePackageConfigHelpers)

  set(export_dest_dir       "${CMAKE_INSTALL_LIBDIR}/cmake/Vigil")
  set(project_config_in     "${PROJECT_SOURCE_DIR}/cmake/VigilConfig.cmake.in")
  set(project_config_out    "${PROJECT_BINARY_DIR}/VigilConfig.cmake")
  set(version_config_file   "${PROJECT_BINARY_DIR}/VigilConfigVersion.cmake")
  set(export_name            "vigil-targets")
  set(pkgconfig_install_dir "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
  set(pkg_config_file       "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc")

  # ----------------------------------------------------------------------------
  # Target & Header Installation:
  # Register targets and associated file sets for system installation
  # ----------------------------------------------------------------------------
  install(TARGETS vigil
    EXPORT ${export_name}
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    ARCHIVE          DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY          DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME          DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )

  # ----------------------------------------------------------------------------
  # Documentation & License Installation
  # ----------------------------------------------------------------------------
  install(FILES
    "${PROJECT_SOURCE_DIR}/README.md"
    "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME}"
  )

  # ----------------------------------------------------------------------------
  # Pkg-Config File Generation & Installation
  # ----------------------------------------------------------------------------
  if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(pkgconfig_includedir  "${CMAKE_INSTALL_INCLUDEDIR}")
  else()
    set(pkgconfig_includedir  "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
  endif()

  if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(pkgconfig_libdir "${CMAKE_INSTALL_LIBDIR}")
  else()
    set(pkgconfig_libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
  endif()

  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}.pc.in"
    "${pkg_config_file}"
    @ONLY
  )
  install(FILES "${pkg_config_file}" DESTINATION "${pkgconfig_install_dir}")

  # ----------------------------------------------------------------------------
  # CMake Package Configuration Generation
  # ----------------------------------------------------------------------------
  write_basic_package_version_file(
    "${version_config_file}"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
    "${project_config_in}"
    "${project_config_out}"
    INSTALL_DESTINATION "${export_dest_dir}"
  )

  # ----------------------------------------------------------------------------
  # Export Targets & Configuration Files Installation
  # ----------------------------------------------------------------------------
  install(
    EXPORT      ${export_name}
    FILE        ${export_name}.cmake
    NAMESPACE   vigil::
    DESTINATION "${export_dest_dir}"
  )

  install(FILES
    "${project_config_out}"
    "${version_config_file}"
    DESTINATION "${export_dest_dir}"
  )
endif()
