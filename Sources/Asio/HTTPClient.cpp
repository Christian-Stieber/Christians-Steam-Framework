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

#include "./BasicQuery.hpp"
#include "Client/Client.hpp"

#include <boost/asio/steady_timer.hpp>

#include <boost/log/trivial.hpp>
#include <boost/json/stream_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

namespace HTTPClient=SteamBot::HTTPClient;

/************************************************************************/

HTTPClient::Query::Query(boost::beast::http::verb method, boost::urls::url url_)
    : url(std::move(url_)), request(method, "", 11)
{
}

/************************************************************************/

HTTPClient::Query::~Query() =default;

/************************************************************************/

boost::json::value SteamBot::HTTPClient::parseJson(const SteamBot::HTTPClient::Query& query)
{
    assert(query.response.result()==boost::beast::http::status::ok);

    boost::json::stream_parser parser;
    const auto buffers=query.response.body().cdata();
    for (auto iterator=boost::asio::buffer_sequence_begin(buffers); iterator!=boost::asio::buffer_sequence_end(buffers); ++iterator)
    {
        parser.write(static_cast<const char*>((*iterator).data()), (*iterator).size());
    }
    parser.finish();
    boost::json::value result=parser.release();
    BOOST_LOG_TRIVIAL(debug) << "response JSON body: " << result;
    return result;
}

/************************************************************************/

std::string SteamBot::HTTPClient::parseString(const SteamBot::HTTPClient::Query& query)
{
    assert(query.response.result()==boost::beast::http::status::ok);

    std::string result;
    const auto buffers=query.response.body().cdata();
    for (auto iterator=boost::asio::buffer_sequence_begin(buffers); iterator!=boost::asio::buffer_sequence_end(buffers); ++iterator)
    {
        result.append(std::string_view{static_cast<const char*>((*iterator).data()), (*iterator).size()});
    }
    BOOST_LOG_TRIVIAL(debug) << "response string body has " << result.size() << " bytes";
    return result;
}

/************************************************************************/

HTTPClient::Query::QueryPtr HTTPClient::perform(HTTPClient::Query::QueryPtr query)
{
    auto waiter=SteamBot::Waiter::create();
    auto cancellation=SteamBot::Client::getClient().cancel.registerObject(*waiter);

    auto responseWaiter=SteamBot::HTTPClient::perform(waiter, std::move(query));
    while (true)
    {
        waiter->wait();
        if (auto response=responseWaiter->getResult())
        {
            return std::move(*response);
        }
    }
}

/************************************************************************/
/*
 * For some reason, I want to serialize the http queries, and also
 * slow them down a bit.
 */

namespace
{
    class Queue
    {
    private:
        typedef std::shared_ptr<HTTPClient::Query::WaiterType> ItemType;

        std::chrono::steady_clock::time_point lastQuery;
        std::queue<ItemType> queue;

        boost::asio::steady_timer timer{SteamBot::Asio::getIoContext()};

    public:
        void performNext()
        {
            assert(SteamBot::Asio::isThread());
            if (!queue.empty())
            {
                auto now=decltype(lastQuery)::clock::now();
                auto next=lastQuery+std::chrono::seconds(5);
                if (now<next)
                {
                    auto cancelled=timer.expires_at(next);
                    assert(cancelled==0);
                    timer.async_wait([this](const boost::system::error_code& error) {
                        assert(!error);
                        performNext();
                    });
                }
                else
                {
                    typedef SteamBot::HTTPClient::Internal::BasicQuery BasicQuery;
                    auto& item=queue.front();
                    auto query=std::make_shared<BasicQuery>(*(item->setResult()),[this, item](BasicQuery&) {
                        lastQuery=decltype(lastQuery)::clock::now();
                        assert(!queue.empty() && queue.front()==item);
                        item->completed();
                        queue.pop();
                        performNext();
                    });
                    SteamBot::HTTPClient::Internal::performWithRedirect(std::move(query));
                }
            }
        }

    public:
        void enqueue(ItemType item)
        {
            SteamBot::Asio::post("HTTPClient/Queue::enqueue", [this, item]() {
                queue.push(std::move(item));
                if (queue.size()==1)
                {
                    performNext();
                }
            });
        }
    };
}

/************************************************************************/

std::shared_ptr<HTTPClient::Query::WaiterType> HTTPClient::perform(std::shared_ptr<SteamBot::WaiterBase> waiter, HTTPClient::Query::QueryPtr query)
{
    static Queue& queue=*new Queue;

    auto result=waiter->createWaiter<HTTPClient::Query::WaiterType>();
    result->setResult()=std::move(query);
    queue.enqueue(result);
    return result;
}
