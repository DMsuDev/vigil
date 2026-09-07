# ==============================================================================
#  VIGIL - INSTALL INTEGRATION TEST: cmake_import (consumer project)
# ==============================================================================
#  Description: Standalone consumer project simulating an external user who
#               installed Vigil and locates it via find_package(Vigil CONFIG).
#               This project must build in complete isolation from Vigil's own
#               build system - no Vigil-specific options are used here.
#
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

project(vigil_smoke_import LANGUAGES CXX)

find_package(Vigil CONFIG REQUIRED)

add_executable(vigil_smoke main.cpp)

target_link_libraries(vigil_smoke PRIVATE vigil::vigil)

target_compile_features(vigil_smoke PRIVATE cxx_std_17)

# Pin the output path regardless of single- vs multi-config generator so the
# harness in ../CMakeLists.txt can locate the binary with a fixed --test-command.
set(vigil_smoke_output_dir "${CMAKE_CURRENT_BINARY_DIR}/bin")
set_target_properties(vigil_smoke PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY                 "${vigil_smoke_output_dir}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG           "${vigil_smoke_output_dir}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE         "${vigil_smoke_output_dir}"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL      "${vigil_smoke_output_dir}"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${vigil_smoke_output_dir}"
)
