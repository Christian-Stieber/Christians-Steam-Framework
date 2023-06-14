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
#include "Asio/HTTPClient.hpp"

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

void Query::set(std::string_view key, std::string_view value)
{
    assert(!url.params().contains(key));
    url.params().set(key, value);
}

/************************************************************************/

void Query::set(std::string_view key, int value)
{
    set(std::move(key), makeString(value));
}

/************************************************************************/

void Query::set(std::string_view key, const std::vector<std::string>& values)
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
 */

std::shared_ptr<Query::WaiterType> SteamBot::WebAPI::perform(std::shared_ptr<SteamBot::Waiter> waiter, Query::QueryPtr query)
{
    auto result=waiter->createWaiter<Query::WaiterType>();
    result->setResult()=std::move(query);

    class MyWaiter : public SteamBot::WaiterBase
    {
    private:
        std::shared_ptr<WaiterBase> self;
        std::shared_ptr<Query::WaiterType> result;
        std::shared_ptr<SteamBot::HTTPClient::Query::WaiterType> httpWaiterItem;

    public:
        virtual ~MyWaiter() =default;

        MyWaiter(decltype(result) result_)
            : result(std::move(result_))
        {
        }

        void perform()
        {
            auto httpQuery=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, result->setResult().get()->url);
            self=shared_from_this();
            httpWaiterItem=SteamBot::HTTPClient::perform(self, std::move(httpQuery));
        }

    private:
        virtual void wakeup(ItemBase*) override
        {
            if (cancelled)
            {
                // ToDo: ???
            }
            else if (auto httpQuery=httpWaiterItem->getResult())
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
            }
        }
    };

    std::make_shared<MyWaiter>(result)->perform();

    return result;
}
