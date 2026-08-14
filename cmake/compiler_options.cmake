# ==============================================================================
#  VIGIL - COMPILER OPTIONS
# ==============================================================================
#  Description: Configures platform and compiler-specific build flags, code
#               generation settings, and hardening options for CMake targets.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include_guard()

# ------------------------------------------------------------------------------
#  vigil_set_compiler_options(<target>)
# ------------------------------------------------------------------------------
#  Applies Vigil's codegen/hardening policy to a single target with PRIVATE
#  scope.
# ------------------------------------------------------------------------------
function(vigil_set_compiler_options target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Vigil: Target '${target}' does not exist. Cannot apply compiler options.")
  endif()

  if(MSVC)
    target_compile_options(${target} PRIVATE
      /utf-8    # Treat source and execution character sets as UTF-8.
      /EHsc     # Standard C++ exception handling; extern "C" never throws.
      /Zc:preprocessor # Conforming preprocessor (matches GCC/Clang expansion rules).
    )

    # Reasonable release hardening beyond the CMake defaults.
    target_compile_options(${target} PRIVATE
      $<$<CONFIG:Release>:/Gy> # Function-level linking, enables safer /OPT:REF.
    )
  else()
    target_compile_options(${target} PRIVATE
      $<$<CONFIG:Debug>:-fno-omit-frame-pointer> # Keep frame pointers for debugging/profiling.
      $<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2>   # Buffer overflow checks in libc calls.
    )
  endif()
endfunction()
