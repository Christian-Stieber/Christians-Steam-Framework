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

#include "Asio/Asio.hpp"
#include "WebAPI/WebAPI.hpp"
#include "Asio/RateLimit.hpp"
#include "Client/CallbackWaiter.hpp"
#include "SafeCast.hpp"

#include <charconv>
#include <boost/log/trivial.hpp>
#include <boost/url/url_view.hpp>

/************************************************************************/

typedef SteamBot::WebAPI::Query Query;

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

Query::Query(std::string_view interface, std::string_view method, unsigned int version)
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

Query::~Query() =default;

/************************************************************************/

void Query::set(std::string_view key, std::string_view value_)
{
    assert(!url.params().contains(key));
    url.params().set(key, value_);
}

/************************************************************************/

void Query::set(std::string_view key, int value_)
{
    set(std::move(key), makeString(value_));
}

/************************************************************************/

void Query::set(std::string_view key, const std::vector<std::string>& values)
{
    set("count", static_cast<int>(values.size()));
    for (size_t i=0; i<values.size(); i++)
    {
        std::string itemKey{key};
        itemKey+='[';
        itemKey+=makeString(SteamBot::safeCast<int>(i));
        itemKey+=']';
        set(itemKey, values[i]);
    }
}

/************************************************************************/
/*
 * Sends the query.
 */

std::shared_ptr<Query::WaiterType> SteamBot::WebAPI::perform(std::shared_ptr<SteamBot::WaiterBase> waiter, Query::QueryPtr query)
{
    auto initiator=[query=std::move(query)](std::shared_ptr<SteamBot::WaiterBase> waiter_, std::shared_ptr<Query::WaiterType> result) mutable {
        result->setResult()=std::move(query);
        auto httpQuery=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, result->setResult().get()->url);
        return SteamBot::HTTPClient::getDefaultQueue().perform(std::move(waiter_), std::move(httpQuery));
    };

    auto completer=[](std::shared_ptr<SteamBot::HTTPClient::Query::WaiterType> http, std::shared_ptr<Query::WaiterType> result) {
        assert(SteamBot::Asio::isThread());
        if (auto httpQuery=http->getResult())
        {
            if ((*httpQuery)->error)
            {
                result->setResult()->error=(*httpQuery)->error;
            }
            else
            {
                try
                {
                    result->setResult()->value=SteamBot::HTTPClient::parseJson(**httpQuery);
                }
                catch(const boost::system::system_error& exception)
                {
                    result->setResult()->error=exception.code();
                }
            }
            result->completed();
            return true;
        }
        return false;
    };

    return SteamBot::CallbackWaiter::perform(std::move(waiter), std::move(initiator), std::move(completer));
}
