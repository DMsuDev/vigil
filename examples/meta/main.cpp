// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"

#include <iostream>

int main()
{
    std::cout << "Vigil version: " << vigil::version::string << std::endl;
    std::cout << "--------------------" << std::endl;
    std::cout << "Major: " << vigil::version::major << std::endl;
    std::cout << "Minor: " << vigil::version::minor << std::endl;
    std::cout << "Patch: " << vigil::version::patch << std::endl;
    return 0;
}
