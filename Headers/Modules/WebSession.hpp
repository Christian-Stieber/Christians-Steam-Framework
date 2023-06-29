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
