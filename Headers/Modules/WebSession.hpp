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

#include "Asio/HTTPClient.hpp"

#include <boost/url/url.hpp>

/************************************************************************/
/*
 * The "WebSession" module allows us to make authenticated https
 * queries to Steam.
 */

/***********************************************************************
 *
 * Note: I don't know whether I really like the "queryMaker" approach
 * here, but it seems the discovery queue wants a cookie value in
 * the request body, so I'm must delaying the request generation.
 *
 * Also, HTTTPClient request are usually stored in a unique_ptr, and
 * passed around like that, but the whole thing is a messageboard
 * message and that can't be modified...
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace WebSession
        {
            namespace Messageboard
            {
                typedef SteamBot::HTTPClient::Query::QueryPtr QueryPtr;

                class Request
                {
                public:
                    std::function<QueryPtr()> queryMaker;

                public:
                    Request();
                    ~Request();
                };

                class Response
                {
                public:
                    std::shared_ptr<const Request> initiator;
                    SteamBot::HTTPClient::Query::QueryPtr query;

                public:
                    Response();
                    ~Response();
                };
            }

            std::shared_ptr<const Messageboard::Response> makeQuery(std::shared_ptr<Messageboard::Request>);
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace WebSession
        {
            namespace Whiteboard
            {
                class Cookies
                {
                public:
                    std::string steamLogin;
                    std::string steamLoginSecure;
                    std::string sessionid;

                public:
                    Cookies();
                    ~Cookies();
                };
            }
        }
    }
}

/************************************************************************/
/*
 * Adds a "sessionid" parameter into the body before sending it
 *
 * Note: ASF supports different spellings for the sessionid parameter
 * that goes into the body data -- but, so far, I've only seen
 * "sessionid".
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace WebSession
        {
            class PostWithSession
            {
            public:
                std::string body;
                std::string referer;	// may be empty
                boost::urls::url url;

            public:
                // Note: this will trash the PostWithSession
                std::shared_ptr<const Messageboard::Response> execute();
            };

        }
    }
}
