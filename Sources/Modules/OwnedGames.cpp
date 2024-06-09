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
#include "SafeCast.hpp"
#include "AppInfo.hpp"

#include "steamdatabase/protobufs/steam/steammessages_player.steamclient.pb.h"

/************************************************************************/

typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;
typedef SteamBot::Modules::OwnedGames::Messageboard::UpdateGames UpdateGames;
typedef SteamBot::Modules::OwnedGames::Messageboard::GameChanged GameChanged;

typedef SteamBot::Modules::LicenseList::Whiteboard::Licenses Licenses;

/************************************************************************/

namespace
{
    class OwnedGamesModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Whiteboard::WaiterType<Licenses::Ptr> licenseList;
        SteamBot::Messageboard::WaiterType<UpdateGames> updateGames;

    private:
        void reportChanged(OwnedGames::ChangeList&) const;
        void getOwnedGames();

    public:
        void handle(std::shared_ptr<const UpdateGames>);

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

std::strong_ordering OwnedGames::GameInfo::operator<=>(const OwnedGames::GameInfo&) const =default;
bool OwnedGames::GameInfo::operator==(const OwnedGames::GameInfo&) const =default;

/************************************************************************/

UpdateGames::UpdateGames(std::vector<SteamBot::AppID> appIds_)
    : appIds(std::move(appIds_))
{
}

UpdateGames::~UpdateGames() =default;

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

std::shared_ptr<const OwnedGames::GameInfo> OwnedGames::getInfo(AppID appId) const
{
    auto iterator=games.find(appId);
    if (iterator!=games.end())
    {
        return iterator->second;
    }
    return nullptr;
}

/************************************************************************/

std::string OwnedGames::getName(AppID appId) const
{
    switch(appId)
    {
    case AppID::SteamClient:
        return "Steam Client";

    case AppID::SteamBackpack:
        return "Steam Backpack";

    case AppID::SteamScreenshots:
        return "Steam Cloud - Screenshots";

    case AppID::SteamWorkshop:
        return "Steam Workshop";

    default:
        if (auto info=getInfo(appId))
        {
            return info->name;
        }
        break;
    }

    return std::string{};
}

/************************************************************************/
/*
 * Retrieve game info from Steam. If an "appIds" list is provided,
 * only retrieve these games. Pass nullptr for all games.
 *
 * Updates the "games" map, and returns a list of AppIDs that have
 * actually changed (including newly added ones).
 */

OwnedGames::ChangeList OwnedGames::getGames_(const std::vector<SteamBot::AppID>* appIds)
{
    typedef SteamBot::Modules::UnifiedMessageClient::ProtobufService::Info<decltype(&::Player::GetOwnedGames)> GetOwnedGamesInfo;
    std::shared_ptr<GetOwnedGamesInfo::ResultType> response;
    {
        GetOwnedGamesInfo::RequestType request;
        if (auto steamId=SteamBot::Client::getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SteamID>())
        {
            request.set_steamid(steamId->getValue());
        }
        request.set_include_appinfo(true);
        request.set_include_extended_appinfo(true);
        request.set_include_played_free_games(true);
        request.set_skip_unvetted_apps(false);

        if (appIds!=nullptr)
        {
            for (auto appId : *appIds)
            {
                request.add_appids_filter(SteamBot::toUnsignedInteger(appId));
            }
        }

        response=SteamBot::Modules::UnifiedMessageClient::execute<GetOwnedGamesInfo::ResultType>("Player.GetOwnedGames#1", std::move(request));
    }

    // we don't need to keep the shared_ptr, but just in case something happens later...
    auto existingGames=SteamBot::Client::getClient().whiteboard.get<OwnedGames::Ptr>(OwnedGames::Ptr());

    OwnedGames::ChangeList changed;

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
                game->lastPlayed=std::chrono::system_clock::from_time_t(SteamBot::safeCast<time_t>(gameData.rtime_last_played()));
            }
            if (gameData.has_playtime_forever())
            {
                game->playtimeForever=std::chrono::minutes(gameData.playtime_forever());
            }

            if (existingGames!=nullptr)
            {
                const OwnedGames::GameInfo* existingGame=existingGames->getInfo(game->appId).get();
                if (existingGame==nullptr || *existingGame!=*game)
                {
                    changed.push_back(std::make_shared<GameChanged>(game->appId, existingGame==nullptr));
                    if (appIds!=nullptr)
                    {
                        BOOST_LOG_TRIVIAL(info) << "game data changed: " << game->toJson();
                    }
                }
            }

            games.insert_or_assign(game->appId, std::move(game));
        }
    }

    return changed;
}

/************************************************************************/

void OwnedGamesModule::reportChanged(OwnedGames::ChangeList& messages) const
{
    auto& messageboard=getClient().messageboard;
    for (auto& message : messages)
    {
        BOOST_LOG_TRIVIAL(info) << "OwnedGanes: " << message->appId << " has changed data";
        messageboard.send(std::move(message));
    }
    messages.clear();
}

/************************************************************************/

void OwnedGamesModule::getOwnedGames()
{
    auto ownedGames=std::make_shared<OwnedGames>();

    auto changed=ownedGames->getGames_();

    SteamBot::AppInfo::update(*ownedGames);

    BOOST_LOG_TRIVIAL(info) << "owned games: " << ownedGames->toJson();
    SteamBot::UI::OutputText() << "account owns " << ownedGames->games.size() << " games";

    getClient().whiteboard.set<OwnedGames::Ptr>(std::move(ownedGames));

    reportChanged(changed);

    updateGames->discardMessages();
}

/************************************************************************/

void OwnedGamesModule::init(SteamBot::Client& client)
{
    licenseList=client.whiteboard.createWaiter<Licenses::Ptr>(*waiter);
    updateGames=client.messageboard.createWaiter<UpdateGames>(*waiter);
}

/************************************************************************/

void OwnedGamesModule::handle(std::shared_ptr<const UpdateGames> message)
{
    if (!message->appIds.empty())
    {
        if (auto ownedGames=getClient().whiteboard.has<OwnedGames::Ptr>())
        {
            auto changed=const_cast<OwnedGames*>(ownedGames->get())->getGames_(&message->appIds);
            reportChanged(changed);
        }
        else
        {
            // ToDo: What should we do? An update was requested while we don't have any games at all?
        }
    }
}

/************************************************************************/

void OwnedGamesModule::run(SteamBot::Client&)
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

        updateGames->handle(this);
    }
}

/************************************************************************/

std::shared_ptr<const OwnedGames::GameInfo> SteamBot::Modules::OwnedGames::getInfo(AppID appId)
{
    if (auto games=SteamBot::Client::getClient().whiteboard.has<::OwnedGames::Ptr>())
    {
        return (*games)->getInfo(appId);
    }
    return nullptr;
}

/************************************************************************/

std::string SteamBot::Modules::OwnedGames::getName(AppID appId)
{
    std::string name;
    if (auto games=SteamBot::Client::getClient().whiteboard.has<::OwnedGames::Ptr>())
    {
        name=(*games)->getName(appId);
    }
    return name;
}

/************************************************************************/

std::ostream& SteamBot::operator<<(std::ostream& stream, SteamBot::AppID appId)
{
    stream << SteamBot::toInteger(appId);
    std::string name=SteamBot::Modules::OwnedGames::getName(appId);
    if (!name.empty())
    {
        stream << " (" << name << ")";
    }
    return stream;
}

/************************************************************************/

void SteamBot::Modules::OwnedGames::use()
{
}
