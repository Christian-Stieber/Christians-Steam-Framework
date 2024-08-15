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

#include "Asio/RateLimit.hpp"
#include "Client/Client.hpp"

#include <boost/log/trivial.hpp>
#include <boost/json/stream_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>

/************************************************************************/

namespace HTTPClient=SteamBot::HTTPClient;

/************************************************************************/

HTTPClient::RateLimitQueue& HTTPClient::getDefaultQueue()
{
    static RateLimitQueue& queue=*new RateLimitQueue(std::chrono::seconds(5));
    return queue;
}

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

HTTPClient::Query::QueryPtr HTTPClient::perform(HTTPClient::Query::QueryPtr query, HTTPClient::RateLimitQueue& queue)
{
    auto waiter=SteamBot::Waiter::create();
    auto cancellation=SteamBot::Client::getClient().cancel.registerObject(*waiter);

    auto responseWaiter=queue.perform(waiter, std::move(query));

    while (true)
    {
        waiter->wait();
        if (auto response=responseWaiter->getResult())
        {
            return std::move(*response);
        }
    }
}
