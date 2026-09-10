# ==============================================================================
#  VIGIL - EXAMPLE PROGRAMS
# ==============================================================================
#  Description: Small, self-contained programs demonstrating Vigil's public
#               API. Each example is compiled as an independent executable
#               target for granular build and reading access.
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

# ------------------------------------------------------------------------------
#  Example Registration Helper Macro/Function
# ------------------------------------------------------------------------------
function(vigil_add_example target_name source_file category)
  set(abs_source_file "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")

  if(NOT EXISTS ${abs_source_file})
    message(FATAL_ERROR "Vigil: Example source file '${abs_source_file}' does not exist.")
  endif()

  add_executable(${target_name} "${abs_source_file}")

  target_link_libraries(${target_name} PRIVATE vigil::vigil)

  # Enable C++20 for examples (designated initializers) while keeping library C++17
  target_compile_features(${target_name} PRIVATE cxx_std_20)

  # ----------------------------------------------------------------------------
  # Windows Icon Integration (.rc)
  # ----------------------------------------------------------------------------
  # TODO: Attach Windows Resource (.rc) for icon if on Windows
  #
  # if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/common/resources.rc")
  #     target_sources(${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/common/resources.rc")
  # endif()

  vigil_enable_warnings(${target_name})
  vigil_set_compiler_options(${target_name})

  vigil_copy_runtime_dependencies(${target_name})

  set_target_properties(${target_name} PROPERTIES
    FOLDER "Examples/${category}"
  )
endfunction()

# ------------------------------------------------------------------------------
#  Example Target Declarations
# ------------------------------------------------------------------------------

# Logging Examples
vigil_add_example(logger_example   "logging/logger_example.cpp"      "Logging")
vigil_add_example(hooks_example    "logging/hooks_example.cpp"       "Logging")
vigil_add_example(scoped_example   "logging/scoped_example.cpp"      "Logging")

# Diagnostics Examples
vigil_add_example(assert_example   "diagnostics/assert_example.cpp"  "Diagnostics")
