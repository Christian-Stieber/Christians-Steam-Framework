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

#include "Client/Module.hpp"
#include "Modules/TradeToken.hpp"
#include "Modules/UnifiedMessageClient.hpp"
#include "Helpers/JSON.hpp"

#include "steamdatabase/protobufs/steam/steammessages_econ.steamclient.pb.h"

/************************************************************************/

namespace
{
    class TradeTokenModule : public SteamBot::Client::Module
    {
    public:
        TradeTokenModule() =default;
        virtual ~TradeTokenModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    TradeTokenModule::Init<TradeTokenModule> init;
}

/************************************************************************/

void TradeTokenModule::run(SteamBot::Client& client)
{
    waitForLogin();

    typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Econ::GetTradeOfferAccessToken)> GetTradeOfferAccessTokenInfo;
    std::shared_ptr<GetTradeOfferAccessTokenInfo::ResultType> response;
    {
        GetTradeOfferAccessTokenInfo::RequestType request;
        request.set_generate_new_token(false);
        response=SteamBot::Modules::UnifiedMessageClient::execute<GetTradeOfferAccessTokenInfo::ResultType>("Econ.GetTradeOfferAccessToken#1", std::move(request));
    }

    if (response->trade_offer_access_token().size()>0)
    {
        client.dataFile.update([response=std::move(response)](boost::json::value& json) {
            auto& item=SteamBot::JSON::createItem(json, "Info", "TradeToken");
            if (!item.is_null())
            {
                if (item.as_string()==response->trade_offer_access_token())
                {
                    return false;
                }
            }
            item=response->trade_offer_access_token();
            return true;
        });
    }
}

/************************************************************************/

std::string SteamBot::Modules::TradeToken::get(const SteamBot::DataFile& dataFile)
{
    std::string token;

    dataFile.examine([&token](const boost::json::value& json) mutable {
        if (auto value=SteamBot::JSON::getItem(json, "Info", "TradeToken"))
        {
            token=value->as_string();
        }
    });
    return token;
}
