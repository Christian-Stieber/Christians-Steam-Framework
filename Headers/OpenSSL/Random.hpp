#pragma once

#include <span>

/************************************************************************/

namespace SteamBot
{
    namespace OpenSSL
    {
        void makeRandomBytes(std::span<std::byte>);
    }
}
