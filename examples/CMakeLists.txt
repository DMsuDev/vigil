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
function(vigil_add_example name)
  set(example_source "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp")

  if(NOT EXISTS ${example_source})
    message(FATAL_ERROR "Vigil: Example source file '${example_source}' does not exist.")
  endif()

  add_executable(${name} "${name}.cpp")

  target_link_libraries(${name}
    PRIVATE
      vigil::vigil
  )

  vigil_enable_warnings(${name})
  vigil_set_compiler_options(${name})

  vigil_copy_runtime_dependencies(${name})

  set_target_properties(${name} PROPERTIES
    FOLDER "Vigil/Examples"
  )
endfunction()

# ------------------------------------------------------------------------------
#  Example Target Declarations
# ------------------------------------------------------------------------------
vigil_add_example(logger_example)
vigil_add_example(assert_example)
