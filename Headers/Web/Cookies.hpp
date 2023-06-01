#pragma once

#include <string>

/************************************************************************/
/*
 * Helper function to create a "Cookie" header value
 */

namespace SteamBot
{
    namespace Web
    {
        void setCookie(std::string&, std::string_view, std::string_view);
        void setCookie(std::string&, std::string_view, std::string_view);
    }
}
