# ==============================================================================
#  VIGIL - CRASH INTEGRATION TEST (consumer project)
# ==============================================================================
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

cmake_minimum_required(VERSION 3.28)
project(vigil_crash_consumer LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Add Vigil as a subdirectory from the source tree.
# ---------------------------------------------------------------------------
add_subdirectory("${VIGIL_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/vigil")

# ---------------------------------------------------------------------------
# vigil_crash_test -> single executable, trigger selected via argv[1]
# ---------------------------------------------------------------------------
add_executable(vigil_crash_test main.cpp)

target_include_directories(vigil_crash_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vigil_crash_test PRIVATE vigil::vigil)
target_compile_features(vigil_crash_test PRIVATE cxx_std_17)

# ------------------------------------------------------------------------------
# Keep the executable in a predictable location.
# ------------------------------------------------------------------------------

set(vigil_crash_output_dir "${CMAKE_CURRENT_BINARY_DIR}/bin")
set_target_properties(vigil_crash_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY                 "${vigil_crash_output_dir}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG           "${vigil_crash_output_dir}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE         "${vigil_crash_output_dir}"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL      "${vigil_crash_output_dir}"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${vigil_crash_output_dir}"
)
