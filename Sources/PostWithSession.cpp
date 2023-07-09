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

#include "Modules/WebSession.hpp"
#include "Client/Client.hpp"
#include "Web/URLEncode.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::PostWithSession PostWithSession;

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

std::shared_ptr<const Response> PostWithSession::execute()
{
    std::shared_ptr<Request> request;

    request=std::make_shared<Request>();
    request->queryMaker=[this]() {
        {
            auto cookies=SteamBot::Client::getClient().whiteboard.has<SteamBot::Modules::WebSession::Whiteboard::Cookies>();
            assert(cookies!=nullptr);
            SteamBot::Web::formUrlencode(body, "sessionid", cookies->sessionid);
        }

        auto query=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::post, std::move(url));
        if (!referer.empty())
        {
            query->request.set(boost::beast::http::field::referer, referer);
        }
        query->request.body()=std::move(body);
        query->request.content_length(query->request.body().size());
        query->request.base().set("Content-Type", "application/x-www-form-urlencoded");
        return query;
    };

    return SteamBot::Modules::WebSession::makeQuery(std::move(request));
}
