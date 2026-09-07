# ==============================================================================
#  TEST SUITE ORCHESTRATOR
# ==============================================================================
#  Orchestrates test execution across two tiers:
#    - Unit (unit/)         : Fast GoogleTest suite (VIGIL_BUILD_TESTS)
#    - Integration (cmake_*): CMake consumer tests (VIGIL_CONSUMER_TESTS)
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

if(VIGIL_BUILD_TESTS)
  add_subdirectory(unit)
endif()

# ------------------------------------------------------------------------------
#  Install / Consumer Integration Tests (opt-in, slow)
# ------------------------------------------------------------------------------

if(VIGIL_CONSUMER_TESTS)
  message(STATUS "Vigil: Registering install/consumer integration tests.")

  add_subdirectory(cmake_fetch_content)
  add_subdirectory(cmake_add_subdirectory)

  if(VIGIL_INSTALL)
    add_subdirectory(cmake_import)
  else()
    message(STATUS "Vigil: Skipping cmake_import test (requires VIGIL_INSTALL=ON).")
  endif()
endif()
