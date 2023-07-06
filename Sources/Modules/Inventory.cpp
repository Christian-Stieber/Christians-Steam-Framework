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
/*
 * Note: I found two services that might be inventory-related,
 * but could not get them to work for me:
 *    Econ.GetInventoryItemsWithDescriptions
 *    Inventory.GetInventory
 *
 * Thus, for now(?), we'll be using a http query.
 */

/************************************************************************/

#include "Client/Module.hpp"
#include "Modules/WebSession.hpp"
#include "RateLimit.hpp"
#include "MiscIDs.hpp"
#include "SteamID.hpp"

/************************************************************************/

typedef SteamBot::Modules::WebSession::Messageboard::Request Request;
typedef SteamBot::Modules::WebSession::Messageboard::Response Response;

/************************************************************************/

namespace
{
    class InventorysModule : public SteamBot::Client::Module
    {
    private:
        static SteamBot::RateLimiter& getRateLimiter();

    private:
        static void load();

    public:
        InventorysModule() =default;
        virtual ~InventorysModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    InventorysModule::Init<InventorysModule> init;
}

/************************************************************************/

SteamBot::RateLimiter& InventorysModule::getRateLimiter()
{
    static auto& instance=*new SteamBot::RateLimiter(std::chrono::seconds(30));
    return instance;
}

/************************************************************************/

void InventorysModule::load()
{
    static const auto appId=SteamBot::AppID::Steam;
    static const uint32_t contextId=6;	// Community Items

    boost::urls::url url("https://steamcommunity.com/inventory");

    {
        auto steamId=getClient().whiteboard.has<SteamBot::SteamID>();
        assert(steamId!=nullptr);
        url.segments().push_back(std::to_string(steamId->getValue()));
    }
    url.segments().push_back(std::to_string(static_cast<std::underlying_type_t<SteamBot::AppID>>(appId)));
    url.segments().push_back(std::to_string(contextId));

    url.params().set("l", "english");
    url.params().set("count", "500");

    // ToDo: loop over this to get pages
    {
        std::shared_ptr<const Response> response;
        getRateLimiter().limit([&url, &response]() {
            auto request=std::make_shared<Request>();
            request->queryMaker=[&url](){
                return std::make_unique<SteamBot::HTTPClient::Query>(boost::beast::http::verb::get, url);
            };
            response=SteamBot::Modules::WebSession::makeQuery(std::move(request));
        });
        BOOST_LOG_TRIVIAL(debug) << "Inventory: " << SteamBot::HTTPClient::parseString(*(response->query));
    }
}

/************************************************************************/

void InventorysModule::run(SteamBot::Client& client)
{
    waitForLogin();

    load();
}

/************************************************************************/

void InventorysModuleUse()
{
}
