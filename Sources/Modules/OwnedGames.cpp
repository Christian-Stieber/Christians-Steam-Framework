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

#include "Modules/UnifiedMessageClient.hpp"
#include "Client/Module.hpp"
#include "Modules/OwnedGames.hpp"
#include "Modules/LicenseList.hpp"
#include "UI/UI.hpp"
#include "Helpers/Time.hpp"

#include "steamdatabase/protobufs/steam/steammessages_player.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;
typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses Licenses;

/************************************************************************/

namespace
{
    class OwnedGamesModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<Licenses::Ptr> licenseList;

    private:
        void getOwnedGames();

    public:
        OwnedGamesModule() =default;
        virtual ~OwnedGamesModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    OwnedGamesModule::Init<OwnedGamesModule> init;
}

/************************************************************************/

OwnedGames::OwnedGames() =default;
OwnedGames::~OwnedGames() =default;

OwnedGames::GameInfo::GameInfo() =default;
OwnedGames::GameInfo::~GameInfo() =default;

/************************************************************************/

boost::json::value OwnedGames::GameInfo::toJson() const
{
    boost::json::object json;
    SteamBot::enumToJson(json, "appId", appId);
    json["name"]=name;
    if (lastPlayed!=decltype(lastPlayed)())
    {
        json["last played"]=SteamBot::Time::toString(lastPlayed, true);
    }
    if (playtimeForever.count()!=0)
    {
        json["playtime"]=playtimeForever.count();
    }
    return json;
}

/************************************************************************/

boost::json::value OwnedGames::toJson() const
{
    boost::json::array json;
    for (const auto& item : games)
    {
        json.push_back(item.second->toJson());
    }
    return json;
}

/************************************************************************/

const OwnedGames::GameInfo* OwnedGames::getInfo(AppID appId) const
{
    auto iterator=games.find(appId);
    if (iterator!=games.end())
    {
        return iterator->second.get();
    }
    return nullptr;
}

/************************************************************************/

void OwnedGamesModule::getOwnedGames()
{
    typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Player::GetOwnedGames)> GetOwnedGamesInfo;
    std::shared_ptr<GetOwnedGamesInfo::ResultType> response;
    {
        GetOwnedGamesInfo::RequestType request;
        if (auto steamId=getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SteamID>())
        {
            request.set_steamid(steamId->getValue());
        }
        request.set_include_appinfo(true);
        request.set_include_played_free_games(true);
        response=SteamBot::Modules::UnifiedMessageClient::execute<GetOwnedGamesInfo::ResultType>("Player.GetOwnedGames#1", std::move(request));
    }

    auto ownedGames=std::make_shared<OwnedGames>();
    for (int index=0; index<response->games_size(); index++)
    {
        const auto& gameData=response->games(index);
        if (gameData.has_appid())
        {
            auto game=std::make_shared<OwnedGames::GameInfo>();
            game->appId=static_cast<SteamBot::AppID>(gameData.appid());
            if (gameData.has_name())
            {
                game->name=gameData.name();
            }
            if (gameData.has_rtime_last_played())
            {
                game->lastPlayed=std::chrono::system_clock::from_time_t(gameData.rtime_last_played());
            }
            if (gameData.has_playtime_forever())
            {
                game->playtimeForever=std::chrono::minutes(gameData.playtime_forever());
            }
            bool success=ownedGames->games.try_emplace(game->appId, std::move(game)).second;
            assert(success);
        }
    }

    BOOST_LOG_TRIVIAL(info) << "owned games: " << *ownedGames;
    SteamBot::UI::OutputText() << "account owns " << ownedGames->games.size() << " games";

    getClient().whiteboard.set<OwnedGames::Ptr>(std::move(ownedGames));
}

/************************************************************************/

void OwnedGamesModule::init(SteamBot::Client& client)
{
    licenseList=client.whiteboard.createWaiter<Licenses::Ptr>(*waiter);
}

/************************************************************************/

void OwnedGamesModule::run(SteamBot::Client& client)
{
    while (true)
    {
        waiter->wait();
        if (licenseList->isWoken())
        {
            auto list=licenseList->has();
            assert(list!=nullptr);

            getOwnedGames();
        }
    }
}

/************************************************************************/

void SteamBot::Modules::OwnedGames::use()
{
}
