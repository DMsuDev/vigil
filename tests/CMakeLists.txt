# ==============================================================================
#  TEST SUITE ORCHESTRATOR
# ==============================================================================
#  Description: Wires up Vigil's two test tiers:
#     unit/  ->  Fast GoogleTest unit tests (VIGIL_BUILD_TESTS).
#   _    __ __ _____ ___ __
#  | |  / /  _/ ____/  _/ /
#  | | / // // / __ / // /     Logging and Diagnostics for C++
#  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
#  |___/___/\____/___/_____/
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

add_subdirectory(unit)
