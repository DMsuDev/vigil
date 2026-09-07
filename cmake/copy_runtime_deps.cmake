# ==============================================================================
#  VIGIL - RUNTIME DEPENDENCIES UTILITIES
# ==============================================================================
#  Description: Provides utility functions to resolve and deploy runtime
#               dependencies (e.g., Windows DLLs) alongside built targets.
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include_guard()

# ------------------------------------------------------------------------------
#  vigil_copy_runtime_dependencies(<target>)
# ------------------------------------------------------------------------------
#  Copies all dynamic libraries (DLLs) required by <target> into its output
#  directory. On Linux and macOS, RPATH handles shared library resolution
#  automatically so this function is a no-op on those platforms.
# ------------------------------------------------------------------------------
function(vigil_copy_runtime_dependencies target)
  # Skip copying runtime dependencies if not on Windows or if building a static library.
  if(NOT WIN32 OR NOT VIGIL_BUILD_SHARED)
    return()
  endif()

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${target}>
            $<TARGET_FILE_DIR:${target}>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Copying runtime DLL dependencies for ${target}"
  )
endfunction()
