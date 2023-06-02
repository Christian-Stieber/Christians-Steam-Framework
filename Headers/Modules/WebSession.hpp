#pragma once

#include "HTTPClient.hpp"

#include <boost/url/url.hpp>

/************************************************************************/
/*
 * The "WebSession" module allows us to make authenticated https
 * queries to Steam.
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace WebSession
        {
            namespace Messageboard
            {
                class GetURL
                {
                public:
                    boost::urls::url url;
                };

                class GotURL
                {
                public:
                    std::shared_ptr<const GetURL> initiator;
                    SteamBot::HTTPClient::ResponseType response;
                };
            }
        }
    }
}
