// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/compiler_attributes.h" // For VIGIL_CURRENT_FUNCTION()

#if defined(VIGIL_CPP20)
    #include <source_location>
#endif

/**
 * @file source_location.h
 * @brief Portable source-location capture abstraction.
 *
 * @details Provides a unified interface for capturing source-code location
 * metadata (file path, line number, column, and function signature) across supported C++ standards.
 *
 * - **C++20 and later:** Captured from `std::source_location`.
 * - **C++17:** Captured from compiler-provided macros.
 *
 * @see VIGIL_CURRENT_LOC()
 */

namespace vigil {

    /// @struct SourceLocation
    /// @brief Represents a location in the source code.
    struct SourceLocation {
        /// @brief Constructs an empty source location
        constexpr SourceLocation(
            const char* file = "",
            unsigned line = 0,
            const char* function = "") noexcept
            : m_File(file), m_Line(line), m_Function(function) {}

        /// @brief Returns the source file name (full or relative path).
        [[nodiscard]] constexpr const char* file_name() const noexcept { return m_File; }

        /// @brief Returns the 1-based line number within the source file.
        [[nodiscard]] constexpr unsigned line() const noexcept { return m_Line; }

        /**
         * @brief Returns the 1-based column number within the source file.
         * @note  Always returns `0` in the C++17 fallback implementation,
         * as column information is not available via standard macros.
         */
        [[nodiscard]] constexpr unsigned column() const noexcept { return 0; }

        /// @brief Returns the function name or compiler-provided function signature.
        [[nodiscard]] constexpr const char* function_name() const noexcept { return m_Function; }

    private:
        const char* m_File;     ///< Source file name.
        unsigned    m_Line;     ///< Source line number.
        const char* m_Function; ///< Function signature literal.
    };

#if defined(VIGIL_CPP20)
    #define VIGIL_CURRENT_LOC()   \
        ::vigil::SourceLocation{  \
            std::source_location::current().file_name(), \
            std::source_location::current().line(),      \
            std::source_location::current().function_name()}
#else
    #define VIGIL_CURRENT_LOC() \
        ::vigil::SourceLocation{__FILE__, static_cast<unsigned>(__LINE__), VIGIL_CURRENT_FUNCTION}
#endif

} // namespace vigil
