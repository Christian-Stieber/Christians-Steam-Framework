/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#include "WebAPI/WebAPI.hpp"
#include "HTTPClient.hpp"

#include <charconv>
#include <boost/log/trivial.hpp>

/************************************************************************/

static const boost::urls::url_view defaultBaseAddress("https://api.steampowered.com");

/************************************************************************/

static std::string makeString(int value)
{
    std::array<char, 64> buffer;
    const auto result=std::to_chars(buffer.data(), buffer.data()+buffer.size(), value);
    assert(result.ec==std::errc());
    return std::string(buffer.data(), result.ptr);
}

/************************************************************************/

SteamBot::WebAPI::Request::Request(std::string_view interface, std::string_view method, unsigned int version)
    : url(defaultBaseAddress)
{
    url.segments().push_back(interface);
    url.segments().push_back(method);
    {
        static const std::string versionPrefix("v");
        url.segments().push_back(versionPrefix+makeString(static_cast<int>(version)));
    }
}

/************************************************************************/

SteamBot::WebAPI::Request::~Request() =default;

/************************************************************************/

void SteamBot::WebAPI::Request::set(std::string_view key, std::string_view value)
{
    assert(!url.params().contains(key));
    url.params().set(key, value);
}

/************************************************************************/

void SteamBot::WebAPI::Request::set(std::string_view key, int value)
{
    set(std::move(key), makeString(value));
}

/************************************************************************/

void SteamBot::WebAPI::Request::set(std::string_view key, const std::vector<std::string>& values)
{
    set("count", static_cast<int>(values.size()));
    for (int i=0; i<values.size(); i++)
    {
        std::string itemKey{key};
        itemKey+='[';
        itemKey+=makeString(i);
        itemKey+=']';
        set(itemKey, values[i]);
    }
}

/************************************************************************/
/*
 * Sends the query.
 * Returns the json-decoded response, or throws.
 */

boost::json::value SteamBot::WebAPI::Request::send() const
{
    auto request=std::make_shared<SteamBot::HTTPClient::Request>(boost::beast::http::verb::get, url);
    auto response=SteamBot::HTTPClient::query(std::move(request)).get();
    return SteamBot::HTTPClient::parseJson(*response);
}
