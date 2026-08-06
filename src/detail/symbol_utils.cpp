// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/detail/symbol_utils.h"

#include <array>

namespace vigil::detail {

namespace {

constexpr std::array<std::string_view, 5> kCallingConventions{
    "__cdecl ",
    "__stdcall ",
    "__fastcall ",
    "__vectorcall ",
    "__thiscall "
};

} // Anonymous namespace

std::string_view FormatFilePath(std::string_view path, unsigned levels)
{
    if (levels == 0 || path.empty())
        return path;

    std::size_t cursor = path.size();
    unsigned separatorsSeen = 0;

    while (cursor > 0) {
        const auto pos = path.find_last_of("/\\", cursor - 1);
        if (pos == std::string_view::npos)
            return path; // Fewer components than requested; return the whole thing.

        if (++separatorsSeen == levels)
            return path.substr(pos + 1);

        cursor = pos;
    }

    return path;
}

std::string CleanFunctionSignature(std::string_view signature)
{
    std::string result;
    result.reserve(signature.size());

    // Remove compiler-specific calling conventions from function signatures.
    std::size_t i = 0;
    while (i < signature.size()) {
        bool matched = false;

        for (const std::string_view cc : kCallingConventions) {
            if (signature.compare(i, cc.size(), cc) == 0) {
                i += cc.size();
                matched = true;
                break;
            }
        }

        if (!matched)
            result += signature[i++];
    }

    return result;
}

} // namespace vigil::detail
