#include "bwm.hpp"
#include <iostream>

int main()
{
    std::unique_ptr<BasicWindowManager> bwm = BasicWindowManager::Create();

    if(!bwm)
    {
        std::cerr << "Error: Failed to create basic window manager instance." << std::endl;
        return EXIT_FAILURE;
    }

    bwm->Run();

    return EXIT_SUCCESS;
}