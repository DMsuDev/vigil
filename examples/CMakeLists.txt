# ==============================================================================
#  VIGIL - EXAMPLE PROGRAMS
# ==============================================================================
#  Description: Small, self-contained programs demonstrating Vigil's public
#               API. Each example is compiled as an independent executable
#               target for granular build and reading access.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

# ------------------------------------------------------------------------------
#  Example Registration Helper Macro/Function
# ------------------------------------------------------------------------------
function(vigil_add_example folder name)
  if(folder STREQUAL "" OR name STREQUAL "")
    message(FATAL_ERROR "Vigil: Example folder and name must be specified.")
  endif()

  set(example_source "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/${name}.cpp")

  if(NOT EXISTS ${example_source})
    message(FATAL_ERROR "Vigil: Example source file '${example_source}' does not exist.")
  endif()

  add_executable(${name} "${folder}/${name}.cpp")

  target_link_libraries(${name}
    PRIVATE
      vigil::vigil
  )

  vigil_enable_warnings(${name})
  vigil_set_compiler_options(${name})

  set_target_properties(${name} PROPERTIES
    FOLDER "Vigil/Examples/${folder}"
  )
endfunction()

# ------------------------------------------------------------------------------
#  Example Target Declarations
# ------------------------------------------------------------------------------
vigil_add_example(meta main)
