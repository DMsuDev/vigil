# ==============================================================================
#  VIGIL - RUNTIME DEPENDENCIES UTILITIES
# ==============================================================================
#  Description: Provides utility functions to resolve and deploy runtime
#               dependencies (e.g., Windows DLLs) alongside built targets.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include_guard()

# ------------------------------------------------------------------------------
#  vigil_copy_runtime_dependencies(<target>)
# ------------------------------------------------------------------------------
#  Copies all dynamic libraries (DLLs) required by <target> into the output
#  directory containing the executable. Operates strictly on Windows platforms.
# ------------------------------------------------------------------------------
function(vigil_copy_runtime_dependencies target)
  if(NOT WIN32)
    return()
  endif()

  set(copy_command
    ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:${target}>
    $<TARGET_FILE_DIR:${target}>
  )

  set(noop_command ${CMAKE_COMMAND} -E true)

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      "$<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>,${copy_command},${noop_command}>"
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Copying runtime DLL dependencies for ${target}"
  )
endfunction()
