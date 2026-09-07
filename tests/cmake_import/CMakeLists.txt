# ==============================================================================
#  Install Integration Test Harness
#
#  Simulates a consumer that installed Vigil and locates it via
#  find_package(Vigil CONFIG REQUIRED). Only registered when VIGIL_INSTALL
#  is ON
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

# Staging prefix that vigil_install populates and cmake_import consumes
# via CMAKE_PREFIX_PATH.
set(VIGIL_INSTALL_TEST_PREFIX "${CMAKE_BINARY_DIR}/install-test-prefix"
  CACHE INTERNAL "Staging prefix used by Vigil's install integration tests"
)

set(cmake_import_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/project")

# ------------------------------------------------------------------------------
add_test(NAME vigil_install
  COMMAND "${CMAKE_COMMAND}"
    --install "${CMAKE_BINARY_DIR}"
    --config  "${CMAKE_BUILD_TYPE}"
    --prefix  "${VIGIL_INSTALL_TEST_PREFIX}"
)
set_tests_properties(vigil_install PROPERTIES
  FIXTURES_SETUP "cmake_import_fixture"
  LABELS         "vigil;install;environment_dependent"
)

if(VIGIL_INSTALL_TESTS_CLEANUP)
  add_test(NAME cmake_import_cleanup
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
      "${cmake_import_binary_dir}"
      "${VIGIL_INSTALL_TEST_PREFIX}"
  )
  set_tests_properties(cmake_import_cleanup PROPERTIES
    FIXTURES_CLEANUP "cmake_import_fixture"
    LABELS           "vigil;install;environment_dependent"
  )
endif()

# ------------------------------------------------------------------------------
add_test(NAME cmake_import
  COMMAND "${CMAKE_CTEST_COMMAND}"
    --build-and-test
      "${CMAKE_CURRENT_SOURCE_DIR}/project"
      "${cmake_import_binary_dir}"
    --build-generator "${CMAKE_GENERATOR}"
    --build-config    "${CMAKE_BUILD_TYPE}"
    --build-options
      "-DCMAKE_PREFIX_PATH=${VIGIL_INSTALL_TEST_PREFIX}"
      "-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
      "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    --test-command "${cmake_import_binary_dir}/bin/vigil_smoke"
)
set_tests_properties(cmake_import PROPERTIES
  FIXTURES_REQUIRED "cmake_import_fixture"
  LABELS            "vigil;install;environment_dependent"
)
