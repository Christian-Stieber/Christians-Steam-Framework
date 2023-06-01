#include "Web/URLEncode.hpp"

#include <boost/url.hpp>

/************************************************************************/

std::string SteamBot::Web::urlencode(std::span<const std::byte> data)
{
    std::string_view bytes(static_cast<const char*>(static_cast<const void*>(data.data())), data.size());
    return boost::urls::encode(bytes, boost::urls::unreserved_chars);
}

/************************************************************************/

std::string SteamBot::Web::urlencode(std::string_view data)
{
    const std::span<const std::byte> bytes{static_cast<const std::byte*>(static_cast<const void*>(data.data())), data.size()};
    return urlencode(bytes);
}

/************************************************************************/

std::string SteamBot::Web::urlencode(uint64_t value)
{
    return urlencode(std::to_string(value));
}

/************************************************************************/
/*
 * "data" is already url-encoded
 */

void SteamBot::Web::formUrlencode_(std::string& result, std::string_view name, std::string data)
{
    if (!result.empty()) result.push_back('&');
    result.append(urlencode(name));
    result.push_back('=');
    result.append(std::move(data));
}

/************************************************************************/

void SteamBot::Web::formUrlencode(std::string& result, std::string_view name, std::string_view data)
{
    formUrlencode_(result, name, urlencode(data));
}

/************************************************************************/

void SteamBot::Web::formUrlencode(std::string& result, std::string_view name, std::span<const std::byte> data)
{
    formUrlencode_(result, name, urlencode(data));
}

/************************************************************************/

void SteamBot::Web::formUrlencode(std::string& result, std::string_view name, uint64_t value)
{
    formUrlencode_(result, name, urlencode(value));
}
