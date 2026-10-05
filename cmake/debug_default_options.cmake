# ==============================================================================
#  VIGIL - DEBUG-DEFAULT OPTIONS
# ==============================================================================
#  Description: Declares BOOL cache options whose default value differs for
#               Debug configurations, while always respecting a value the
#               user has already given explicitly.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include_guard()

# ------------------------------------------------------------------------------
#  vigil_option_debug_default(<name> <docstring> <default> <debug_default>)
# ------------------------------------------------------------------------------
#  Declares option <name> with <default> as its normal value. If the user has
#  not already given <name> an explicit value (via -D<name>=... or a
#  pre-existing cache entry) AND the active configuration is Debug, the value
#  is forced to <debug_default> instead.
#
#  Use in place of a plain option() call for options that should auto-enable
#  in Debug builds without overriding a user's explicit choice.
# ------------------------------------------------------------------------------
function(vigil_option_debug_default name docstring default debug_default)
  # Must be captured BEFORE option() — after that call the cache entry always
  # exists, so this is the only point where "never set by the user" is
  # distinguishable from "left at the default".
  if(DEFINED CACHE{${name}})
    set(_user_set TRUE)
  else()
    set(_user_set FALSE)
  endif()

  option(${name} "${docstring}" ${default})

  if(NOT _user_set)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR
       "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES OR "RelWithDebInfo" IN_LIST CMAKE_CONFIGURATION_TYPES)
      set(${name} ${debug_default} CACHE BOOL "${docstring}" FORCE)
      message(STATUS "Vigil: '${name}' defaulted to ${debug_default} for ${CMAKE_BUILD_TYPE} build.")
    endif()
  endif()
endfunction()
