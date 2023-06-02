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

#include <memory>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/fiber/future/future.hpp>
#include <boost/url/url_view_base.hpp>
#include <boost/json/value.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace HTTPClient
    {
        // thrown for any error
        class Exception { };

        struct Response
        {
            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::dynamic_body> response;
        };

        typedef std::unique_ptr<Response> ResponseType;

        class Request
        {
        public:
            boost::beast::http::verb method;
            const boost::urls::url_view_base& url;
            std::string body;
            std::string contentType;

        public:
            Request(boost::beast::http::verb, const boost::urls::url_view_base&);
            virtual ~Request();
        };

        typedef std::shared_ptr<Request> RequestType;

        boost::fibers::future<ResponseType> query(RequestType);
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
        boost::json::value parseJson(const Response&);
        std::string parseString(const Response&);
    }
}
