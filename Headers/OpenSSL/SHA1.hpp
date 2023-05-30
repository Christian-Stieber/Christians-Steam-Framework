#pragma once

#include <span>

/************************************************************************/

namespace SteamBot
{
    namespace OpenSSL
    {
        void calculateSHA1(std::span<const std::byte>, std::span<std::byte, 20>);
    }
}
