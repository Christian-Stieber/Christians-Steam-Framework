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

/************************************************************************/

#include "Client/Module.hpp"
#include "Modules/WebSession.hpp"
#include "Helpers/URLs.hpp"
#include "Web/CookieJar.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

namespace
{
    class TradeOffersModule : public SteamBot::Client::Module
    {
    public:
        TradeOffersModule() =default;
        virtual ~TradeOffersModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    TradeOffersModule::Init<TradeOffersModule> init;
}

/************************************************************************/

void TradeOffersModule::run(SteamBot::Client& client)
{
    waitForLogin();

    auto request=std::make_shared<Request>();
    request->queryMaker=[this](){
        auto url=SteamBot::URLs::getClientCommunityURL();
        url.segments().push_back("tradeoffers");
        url.params().set("l", "english");
        auto request=std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
        request->cookies=SteamBot::Web::CookieJar::get();
        return request;
    };

    auto response=SteamBot::Modules::WebSession::makeQuery(std::move(request));

    auto data=SteamBot::HTTPClient::parseString(*(response->query));
    BOOST_LOG_TRIVIAL(debug) << data;
}

/************************************************************************/

void TradeOffersModuleRun()
{
}
