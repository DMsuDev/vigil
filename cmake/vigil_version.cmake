# ==============================================================================
#  VIGIL - VERSION LOADER
# ==============================================================================
#  Description: Reads the VERSION file from the project root and exposes two
#               variables to the caller:
#
#               <out_version>        Raw version string as written in VERSION,
#                                    including any prerelease suffix (e.g.
#                                    "1.2.0-WIP"). Passed to display strings
#                                    and artifact names.
#
#               <out_plain_version>  Suffix-stripped version string (e.g.
#                                    "1.2.0"). Passed to project() and
#                                    write_basic_package_version_file() which
#                                    require a pure SemVer value.
#
#  Recognized suffixes: -WIP, .WIP, -dev, .dev
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include_guard()


# ------------------------------------------------------------------------------
#  vigil_load_version(<out_version> <out_plain_version>)
# ------------------------------------------------------------------------------
function(vigil_load_version version plain_version)
  set(_file "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "VERSION file not found: ${_file}")
  endif()

  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_file}")

  file(READ "${_file}" _raw)
  string(STRIP "${_raw}" _raw)

  string(REGEX REPLACE "[-.]?(dev|WIP)$" "" _plain "${_raw}")
  string(REGEX MATCH   "[-.]?(dev|WIP)$" _suffix "${_raw}")

  set(${version}       "${_raw}"   PARENT_SCOPE)
  set(${plain_version} "${_plain}" PARENT_SCOPE)

  if(_suffix)
    set(VIGIL_IS_PRERELEASE TRUE PARENT_SCOPE)
  endif()
endfunction()
