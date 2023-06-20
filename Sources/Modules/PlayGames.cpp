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

#include "Modules/Connection.hpp"
#include "Client/Module.hpp"
#include "Modules/PlayGames.hpp"
#include "Modules/OwnedGames.hpp"
#include "Steam/OSType.hpp"
#include "GameID.hpp"
#include "Vector.hpp"
#include "UI/UI.hpp"

#include "Steam/ProtoBuf/steammessages_clientserver.hpp"

/************************************************************************/

typedef SteamBot::Modules::PlayGames::Messageboard::PlayGame PlayGame;

/************************************************************************/

PlayGame::PlayGame() =default;
PlayGame::~PlayGame() =default;

/************************************************************************/

boost::json::value PlayGame::toJson() const
{
    boost::json::object json;
    SteamBot::enumToJson(json, "appId", appId);
    json["action"]=start ? "start" : "stop";
    return json;
}

/************************************************************************/

namespace
{
    class PlayGamesModule : public SteamBot::Client::Module
    {
    private:
        std::vector<SteamBot::AppID> games;

    private:
        void reportGames() const;
        void sendGames() const;

    public:
        void handle(std::shared_ptr<const PlayGame>);

    public:
        PlayGamesModule() =default;
        virtual ~PlayGamesModule() =default;

        virtual void run(SteamBot::Client&) override;
    };

    PlayGamesModule::Init<PlayGamesModule> init;
}

/************************************************************************/

void PlayGamesModule::reportGames() const
{
    SteamBot::UI::OutputText output;
    output << "(presumably) playing ";

    if (games.size()==0)
    {
        output << "no games";
    }
    else
    {
        typedef SteamBot::Modules::OwnedGames::Whiteboard::OwnedGames OwnedGames;
        auto ownedGames=getClient().whiteboard.get<OwnedGames::Ptr>(nullptr);
        const char* separator="";
        for (SteamBot::AppID appId : games)
        {
            output << separator;
            separator=", ";

            output << static_cast<std::underlying_type_t<Steam::OSType>>(appId);
            if (ownedGames)
            {
                if (auto info=ownedGames->getInfo(appId))
                {
                    output << " (" << info->name << ")";
                }
            }
        }
    }
}

/************************************************************************/

void PlayGamesModule::sendGames() const
{
    auto message=std::make_unique<Steam::CMsgClientGamesPlayedMessageType>();

    // ToDo: should we use a different OS type?
    message->content.set_client_os_type(static_cast<std::underlying_type_t<Steam::OSType>>(Steam::getOSType()));

    for (const auto& appId : games)
    {
        auto gameMessage=message->content.add_games_played();

        {
            SteamBot::GameID gameId;
            gameId.setAppType(SteamBot::GameID::AppType::App);
            gameId.setAppId(appId);
            gameMessage->set_game_id(gameId.getValue());
        }
    }

    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));

    reportGames();
}

/************************************************************************/

void PlayGamesModule::handle(std::shared_ptr<const PlayGame> message)
{
    BOOST_LOG_TRIVIAL(debug) << "received PlayGame request: " << *message;
    if (message->start)
    {
        for (const auto appId : games)
        {
            if (appId==message->appId)
            {
                return;
            }
        }
        games.push_back(message->appId);
    }
    else
    {
        auto count=SteamBot::erase(games, [message](SteamBot::AppID other) {
            return other==message->appId;
        });

        assert(count==0 || count==1);
        if (count==0)
        {
            return;
        }
    }

    sendGames();
}

/************************************************************************/

void PlayGamesModule::run(SteamBot::Client& client)
{
    std::shared_ptr<SteamBot::Messageboard::Waiter<PlayGame>> playGame;
    playGame=waiter->createWaiter<decltype(playGame)::element_type>(client.messageboard);

    while (true)
    {
        waiter->wait();
        playGame->handle(this);
    }
}

/************************************************************************/

void PlayGame::play(SteamBot::AppID appId, bool start)
{
    auto message=std::make_shared<PlayGame>();
    message->appId=appId;
    message->start=start;
    SteamBot::Client::getClient().messageboard.send(std::move(message));
}
