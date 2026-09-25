#include <string>
#include <iostream>
#include "tc/misc/misc.hpp"

void Tc::Log(std::string msg)
{
#ifdef DEBUG_OUTPUT
    std::cout << msg;
#endif
}
