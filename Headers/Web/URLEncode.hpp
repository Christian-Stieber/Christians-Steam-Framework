#pragma once

#include <string_view>
#include <span>

/************************************************************************/

namespace SteamBot
{
    namespace Web
    {
        std::string urlencode(std::string_view);
        std::string urlencode(std::span<const std::byte>);
        std::string urlencode(uint64_t);
    }
}

/************************************************************************/
/*
 * Create a "application/x-www-form-urlencoded" string from
 * data pieces
 */

namespace SteamBot
{
    namespace Web
    {
        void formUrlencode_(std::string&, std::string_view, std::string);

        void formUrlencode(std::string&, std::string_view, std::string_view);
        void formUrlencode(std::string&, std::string_view, std::span<const std::byte>);
        void formUrlencode(std::string&, std::string_view, uint64_t);
    }
}
