#pragma once

#include <functional>
#include <random>

/************************************************************************/

namespace SteamBot
{
    namespace Random
    {
        typedef std::mt19937 Generator;

        void makeRandomNumber(std::function<void(Generator&)>);
        Generator::result_type generateRandomNumber();
    }
}
