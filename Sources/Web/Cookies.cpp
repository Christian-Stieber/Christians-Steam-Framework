#include "Web/Cookies.hpp"

/************************************************************************/

void SteamBot::Web::setCookie(std::string& result, std::string_view name, std::string_view value)
{
    if (!result.empty()) result.push_back(';');
    result.append(name);
    result.push_back('=');
    result.append(value);
}
