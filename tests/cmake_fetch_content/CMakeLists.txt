# ==============================================================================
#  Install Integration Test Harness
#
#  Simulates a consumer that pulls Vigil via FetchContent using a local
#  SOURCE_DIR so the test runs offline against the current source tree.
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

set(cmake_fetch_content_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/project")

# ------------------------------------------------------------------------------
if(VIGIL_INSTALL_TESTS_CLEANUP)
  add_test(NAME cmake_fetch_content_cleanup
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${cmake_fetch_content_binary_dir}"
  )
  set_tests_properties(cmake_fetch_content_cleanup PROPERTIES
    FIXTURES_CLEANUP "cmake_fetch_content_fixture"
    LABELS           "vigil;install;environment_dependent"
  )
endif()

# ------------------------------------------------------------------------------
add_test(NAME cmake_fetch_content
  COMMAND "${CMAKE_CTEST_COMMAND}"
    --build-and-test
      "${CMAKE_CURRENT_SOURCE_DIR}/project"
      "${cmake_fetch_content_binary_dir}"
    --build-generator "${CMAKE_GENERATOR}"
    --build-config    "${CMAKE_BUILD_TYPE}"
    --build-options
      "-DVIGIL_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
      "-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
      "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    --test-command "${cmake_fetch_content_binary_dir}/bin/vigil_smoke"
)
set_tests_properties(cmake_fetch_content PROPERTIES
  FIXTURES_REQUIRED "cmake_fetch_content_fixture"
  LABELS            "vigil;install;environment_dependent"
)
