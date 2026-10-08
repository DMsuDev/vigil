# ==============================================================================
#  VIGIL - CMAKE BUILD HELPERS
# ==============================================================================
#  Description: CMake functions for build environment configuration, compiler
#               options, warnings, and per-target optimizations.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include_guard()

# ------------------------------------------------------------------------------
# Internal: resolve the correct scope keyword for a given target.
# INTERFACE targets only accept INTERFACE; all others use PRIVATE.
# ------------------------------------------------------------------------------

function(_vigil_target_scope target out_scope)
  get_target_property(_type "${target}" TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY")
    set(${out_scope} INTERFACE PARENT_SCOPE)
  else()
    set(${out_scope} PRIVATE PARENT_SCOPE)
  endif()
endfunction()

# ------------------------------------------------------------------------------
# vigil_configure_cmake()
#
# Applies global CMake settings: RPATH for shared builds, default build type.
# Call once from the root CMakeLists.txt before defining any target.
# ------------------------------------------------------------------------------

function(vigil_configure_cmake)
  # Include platform-specific install directory names (bin/, lib/, etc.)
  if(PROJECT_IS_TOP_LEVEL)
    include(GNUInstallDirs)
  endif()

  # Configure RPATH for shared builds
  if(BUILD_SHARED_LIBS)
    if(APPLE)
      set(CMAKE_INSTALL_RPATH "@loader_path" PARENT_SCOPE)
      set(CMAKE_BUILD_RPATH   "@loader_path" PARENT_SCOPE)
    elseif(UNIX)
      set(CMAKE_INSTALL_RPATH "$ORIGIN" PARENT_SCOPE)
      set(CMAKE_BUILD_RPATH   "$ORIGIN" PARENT_SCOPE)
    endif()

    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable position independent code for all targets" FORCE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON PARENT_SCOPE)
  endif()

  # Set default build type for single-config generators
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
  endif()
endfunction()

# ------------------------------------------------------------------------------
# vigil_configure_compiler(target)
#
# Applies compiler-specific flags needed for standards conformance and
# portability. Safe to call on both INTERFACE and non-INTERFACE targets.
# ------------------------------------------------------------------------------

function(vigil_configure_compiler target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "vigil_configure_compiler: target '${target}' does not exist.")
  endif()

  _vigil_target_scope("${target}" _scope)

  set(_options)
  set(_definitions)

  if(MSVC)
    list(APPEND _options
      /utf-8              # Source and execution charset: UTF-8
      /permissive-        # Strict standards conformance
      /Zc:__cplusplus     # Report the actual C++ standard in __cplusplus
      /Zc:preprocessor    # Use the conformant preprocessor
      /MP                 # Multi-processor compilation
      /EHsc               # Standard C++ exception handling only
    )

    list(APPEND _definitions
      _CRT_SECURE_NO_WARNINGS  # Suppress MSVC's "use _s variants" noise
      NOMINMAX                 # Prevent <windows.h> from defining min/max macros
      WIN32_LEAN_AND_MEAN      # Exclude rarely-used Windows headers
      VC_EXTRALEAN             # Exclude even more rarely-used Windows headers
    )
  endif()

  target_compile_options("${target}" ${_scope} ${_options})
  target_compile_definitions("${target}" ${_scope} ${_definitions})
endfunction()

# ------------------------------------------------------------------------------
# vigil_configure_warnings(target)
#
# Applies the project warning suite to a target. Respects VIGIL_BUILD_WARNINGS
# and VIGIL_WARNINGS_AS_ERRORS. Safe to call on INTERFACE targets.
# ------------------------------------------------------------------------------

function(vigil_configure_warnings target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "vigil_configure_warnings: target '${target}' does not exist.")
  endif()

  if(NOT VIGIL_BUILD_WARNINGS)
    return()
  endif()

  _vigil_target_scope("${target}" _scope)

  set(_warnings)

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    list(APPEND _warnings
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow              # Local variable shadows an outer one
      -Wnon-virtual-dtor    # Non-virtual destructor in a polymorphic class
      -Wold-style-cast      # C-style casts
      -Wcast-align          # Casts that increase alignment requirements
      -Woverloaded-virtual  # Derived class hides a base virtual method
      -Wconversion          # Implicit conversions that may lose data
      -Wsign-conversion     # Signed/unsigned implicit conversions
      -Wnull-dereference    # Potential null pointer dereferences
      -Wdouble-promotion    # float implicitly promoted to double
      -Wformat=2            # Extra format string checks
    )
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND _warnings
      -Wmisleading-indentation  # Indentation doesn't match control flow
      -Wduplicated-cond         # Duplicated conditions in if/else chains
      -Wduplicated-branches     # if/else branches with identical bodies
      -Wlogical-op              # Suspicious uses of logical operators
      -Wuseless-cast            # Casts to the same type
    )
  endif()

  if(MSVC)
    list(APPEND _warnings
      /W4
      /w14242  # Conversion: possible loss of data (int to char)
      /w14254  # Conversion: possible loss of data (larger to smaller bit field)
      /w14263  # Member function hides base class virtual function
      /w14265  # Class has virtual functions but destructor is not virtual
      /w14287  # Unsigned/negative constant mismatch
      /w14296  # Expression is always false
      /w14311  # Pointer truncation from larger to smaller type
      /w14546  # Function call before comma missing argument list
      /w14555  # Expression has no effect
      /w14826  # Conversion from one type to another is sign-extended
      /w14928  # Illegal copy-initialization
      # ---- Disable ----
      /wd4127  # Conditional expression is constant (noisy with if constexpr)
    )
  endif()

  target_compile_options("${target}" ${_scope} ${_warnings})

  set_target_properties("${target}" PROPERTIES COMPILE_WARNING_AS_ERROR ${VIGIL_WARNINGS_AS_ERRORS})
endfunction()

# ------------------------------------------------------------------------------
# vigil_disable_third_party_warnings(target)
#
# Silences all warnings for a third-party target so it is compiled
# independently from Vigil's warning policy.
# ------------------------------------------------------------------------------

function(vigil_disable_third_party_warnings target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "vigil_disable_third_party_warnings: target '${target}' does not exist.")
  endif()

  # Imported targets are prebuilt; there is nothing to compile.
  get_target_property(_imported "${target}" IMPORTED)
  if(_imported)
    return()
  endif()

  set_target_properties("${target}" PROPERTIES COMPILE_WARNING_AS_ERROR OFF)

  target_compile_options("${target}" PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-w>
    $<$<CXX_COMPILER_ID:MSVC>:/W0>
  )
endfunction()

# ------------------------------------------------------------------------------
# vigil_strip_dead_code(target)
#
# Enables per-target dead-code stripping for the Release configuration.
# Each toolchain uses its own mechanism:
#   - GCC/Clang (Linux):  -ffunction-sections/-fdata-sections + --gc-sections
#   - AppleClang (macOS): -dead_strip
#   - MSVC (Windows):     /Gy + /OPT:REF
# ------------------------------------------------------------------------------

function(vigil_strip_dead_code target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "vigil_strip_dead_code: target '${target}' does not exist.")
  endif()

  if(VIGIL_ENABLE_COVERAGE)
    return()
  endif()

  _vigil_target_scope("${target}" _scope)

  target_compile_options("${target}" ${_scope}
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:
      -ffunction-sections
      -fdata-sections
    >
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:
      /Gy
    >
  )

  target_link_options("${target}" ${_scope}
    $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Linux>,$<CXX_COMPILER_ID:GNU,Clang>>:
      LINKER:--gc-sections
    >
    $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Darwin>,$<CXX_COMPILER_ID:Clang,AppleClang>>:
      LINKER:-dead_strip
    >
    $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:
      /OPT:REF
    >
  )
endfunction()

# ------------------------------------------------------------------------------
# vigil_enable_coverage(target)
#
# Enables code coverage instrumentation for a target when
# VIGIL_ENABLE_COVERAGE is enabled.
#
# Supported compilers:
#   - GCC
#   - Clang
#   - AppleClang
# ------------------------------------------------------------------------------

function(vigil_enable_coverage target)
  if(NOT VIGIL_ENABLE_COVERAGE)
    return()
  endif()

  if(NOT TARGET "${target}")
    message(FATAL_ERROR "vigil_enable_coverage: target '${target}' does not exist.")
  endif()

  _vigil_target_scope("${target}" _scope)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options("${target}" ${_scope}
      --coverage
      -O0
    )
    target_link_options("${target}" ${_scope} --coverage)

  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    target_compile_options("${target}" ${_scope}
      -fprofile-instr-generate
      -fcoverage-mapping
      -O0
    )
    target_link_options("${target}" ${_scope} -fprofile-instr-generate)

  else()
    message(FATAL_ERROR
      "vigil_enable_coverage: compiler '${CMAKE_CXX_COMPILER_ID}' "
      "does not support code coverage instrumentation."
    )
  endif()
endfunction()

# ------------------------------------------------------------------------------
#  vigil_copy_runtime_dependencies(<target>)
#
#  Copies all dynamic libraries (DLLs) required by <target> into its output
#  directory. On Linux and macOS, RPATH handles shared library resolution
#  automatically so this function is a no-op on those platforms.
# ------------------------------------------------------------------------------

function(vigil_copy_runtime_dependencies target)
  # Get the type of the target to ensure it is suitable for copying runtime dependencies.
  get_target_property(_type ${target} TYPE)
  if(NOT _type MATCHES "^(EXECUTABLE|SHARED_LIBRARY|MODULE_LIBRARY)$")
    message(FATAL_ERROR
      "vigil_copy_runtime_dependencies: '${target}' must be an executable "
      "or shared/module library (got ${_type}).")
  endif()

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
