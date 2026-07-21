# ==============================================================================
#  VIGIL - COMPILER WARNINGS
# ==============================================================================
#  Description: Defines and enforces project-wide compiler diagnostic policies
#               and warning levels across supported toolchains and targets.
#
#  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
#  See LICENSE file in the project root for full license text.
# ==============================================================================

include_guard()

# ------------------------------------------------------------------------------
#  vigil_enable_warnings(<target>)
# ------------------------------------------------------------------------------
#  Applies Vigil's warning policy to a single target with PRIVATE scope, so
#  the strictness never leaks into consumers linking against vigil::vigil.
# ------------------------------------------------------------------------------
function(vigil_enable_warnings target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "Vigil: Target '${target}' does not exist. Cannot apply warning policy.")
  endif()

  if(PROJECT_IS_TOP_LEVEL)
    if(VIGIL_BUILD_WARNINGS)
      message(STATUS "Vigil: Enabling strict compiler warnings for target '${target}'.")
    else()
      message(STATUS "Vigil: Compiler warnings are disabled for target '${target}'.")
      return()
    endif()
  endif()

  if(MSVC)
    target_compile_options(${target} PRIVATE
      /W4          # High warning level.
      /permissive- # Disable non-conforming MSVC extensions.
      /w14242      # 'conversion': possible loss of data.
      /w14254      # 'operator': conversion, possible loss of data.
      /w14263      # member function does not override any base class virtual member function.
      /w14265      # class has virtual functions, but destructor is not virtual.
      /w14287      # unsigned/negative constant mismatch.
      /w14296      # expression is always false/true.
      /w14311      # pointer truncation.
      /w14545      # expression before comma evaluates to a function missing an argument list.
      /w14546      # function call before comma missing argument list.
      /w14547      # operator before comma has no effect.
      /w14549      # operator before comma has no effect; did you intend operator&?
      /w14555      # expression has no effect.
      /w14619      # pragma warning: there is no warning number.
      /w14640      # thread un-safe static member initialization.
      /w14826      # conversion is sign-extended, may cause unexpected runtime behavior.
      /w14905      # wide string literal cast to 'LPSTR'.
      /w14906      # string literal cast to 'LPWSTR'.
      /w14928      # illegal copy-initialization.
  )

  else()
    target_compile_options(${target} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow              # Variable shadows another in an outer scope.
      -Wnon-virtual-dtor    # Class with virtuals but non-virtual destructor.
      -Wold-style-cast      # C-style casts flagged in favor of C++ casts.
      -Wcast-align          # Potential performance problem from misaligned casts.
      -Wunused              # Anything declared but not used.
      -Woverloaded-virtual  # Overload hides a virtual function from a base class.
      -Wconversion          # Implicit conversions that may alter a value.
      -Wsign-conversion     # Implicit sign conversions.
      -Wnull-dereference    # Detected null dereference paths.
      -Wdouble-promotion    # float implicitly promoted to double.
      -Wformat=2            # Extra format-string checks for printf-family calls.
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(${target} PRIVATE
        -Wduplicated-cond     # Duplicated if/else-if condition.
        -Wduplicated-branches # Identical if/else branches.
        -Wlogical-op          # Suspicious use of logical operators.
        -Wuseless-cast        # Cast to the same type as the expression.
      )
    endif()
  endif()

  if(VIGIL_WARNINGS_AS_ERRORS)
    if(MSVC)
      target_compile_options(${target} PRIVATE /WX)
    else()
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()

endfunction()
