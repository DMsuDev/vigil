# ==============================================================================
#  Install Integration Test Harness
#
#  Simulates a consumer who vendors Vigil directly via add_subdirectory().
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

set(cmake_add_subdirectory_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/project")

# ------------------------------------------------------------------------------
if(VIGIL_INSTALL_TESTS_CLEANUP)
  add_test(NAME cmake_add_subdirectory_cleanup
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${cmake_add_subdirectory_binary_dir}"
  )
  set_tests_properties(cmake_add_subdirectory_cleanup PROPERTIES
    FIXTURES_CLEANUP "cmake_add_subdirectory_fixture"
    LABELS           "vigil;install;environment_dependent"
  )
endif()

# ------------------------------------------------------------------------------
add_test(NAME cmake_add_subdirectory
  COMMAND "${CMAKE_CTEST_COMMAND}"
    --build-and-test
      "${CMAKE_CURRENT_SOURCE_DIR}/project"
      "${cmake_add_subdirectory_binary_dir}"
    --build-generator "${CMAKE_GENERATOR}"
    --build-config    "${CMAKE_BUILD_TYPE}"
    --build-options
      "-DVIGIL_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
      "-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
      "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    --test-command "${cmake_add_subdirectory_binary_dir}/bin/vigil_smoke"
)
set_tests_properties(cmake_add_subdirectory PROPERTIES
  FIXTURES_REQUIRED "cmake_add_subdirectory_fixture"
  LABELS            "vigil;install;environment_dependent"
)
