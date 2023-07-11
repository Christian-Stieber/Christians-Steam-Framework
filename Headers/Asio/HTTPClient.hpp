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

#pragma once

#include "Client/ResultWaiter.hpp"
#include "Web/CookieJar.hpp"

#include <memory>

#include <boost/beast/http/message.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include <boost/url/url.hpp>

#include <boost/json/value.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace HTTPClient
    {
        class Query
        {
        public:
            std::shared_ptr<SteamBot::Web::CookieJar> cookies;

        public:
            // fill in your request data
            boost::urls::url url;
            boost::beast::http::request<boost::beast::http::string_body> request;

        public:
            // this gets filled in during perform(). Check the error first.
            boost::system::error_code error;
            boost::beast::flat_buffer responseBuffer;
            boost::beast::http::response<boost::beast::http::dynamic_body> response;

        public:
            Query(boost::beast::http::verb, boost::urls::url);
            virtual ~Query();

        public:
            typedef std::unique_ptr<Query> QueryPtr;
            typedef SteamBot::ResultWaiter<QueryPtr> WaiterType;
        };

        std::shared_ptr<Query::WaiterType> perform(std::shared_ptr<SteamBot::WaiterBase>, Query::QueryPtr);
    }
}

/************************************************************************/
/*
 * This is a blocking call.
 * It will cancel.
 * Can also set and update cookies.
 */

namespace SteamBot
{
    namespace HTTPClient
    {
        Query::QueryPtr perform(Query::QueryPtr);
    }
}

/************************************************************************/
/*
 * Given a response from an HTTPCliebnt query, return the body as
 * "something useful".
 */

namespace SteamBot
{
    namespace HTTPClient
    {
        boost::json::value parseJson(const Query&);
        std::string parseString(const Query&);
    }
}
