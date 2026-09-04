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

  # ----------------------------------------------------------------------------
  #  Compile options
  # ----------------------------------------------------------------------------
  target_compile_options(${target} PRIVATE
    # --- MSVC: baseline codegen --------------------------------------------------
    $<$<CXX_COMPILER_ID:MSVC>:
      /utf-8
      /EHsc
      /Zc:preprocessor
      /MP
    >

    # --- GCC/Clang: debug helpers ------------------------------------------------
    # Preserve frame pointers for debuggers and profilers.
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:
      -fno-omit-frame-pointer
    >

    # --- GCC/Clang: dead-code stripping (compile side) ---------------------------
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:
      -ffunction-sections
      -fdata-sections
    >

    # --- MSVC: dead-code stripping (compile side) --------------------------------
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:
      /Gy
    >
  )

  # ----------------------------------------------------------------------------
  #  Compile definitions
  # ----------------------------------------------------------------------------
  target_compile_definitions(${target} PRIVATE
    # _FORTIFY_SOURCE requires optimisations to be active; only enable in Release.
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:
      _FORTIFY_SOURCE=2
    >
  )

  # ----------------------------------------------------------------------------
  #  Link options - dead-code stripping (linker side)
  # ----------------------------------------------------------------------------
  target_link_options(${target} PRIVATE
    # ld (Linux/ELF): discard unreferenced sections produced by -ffunction/data-sections.
    $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Linux>,$<CXX_COMPILER_ID:GNU,Clang>>:
      LINKER:--gc-sections
    >

    # ld64 (macOS/Mach-O): equivalent dead-stripping pass.
    $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Darwin>,$<CXX_COMPILER_ID:Clang,AppleClang>>:
      LINKER:-dead_strip
    >

    # MSVC linker: discard unreferenced COMDATs enabled by /Gy.
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:
      /OPT:REF
    >
  )
endfunction()
