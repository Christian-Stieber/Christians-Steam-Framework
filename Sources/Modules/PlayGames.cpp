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
#include "Modules/PlayGames.hpp"
#include "Modules/Connection.hpp"
#include "Steam/OSType.hpp"
#include "GameID.hpp"

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
    return json;
}

/************************************************************************/

namespace
{
    class PlayGamesModule : public SteamBot::Client::Module
    {
    private:
        void playGame(std::shared_ptr<const PlayGame>);

    public:
        PlayGamesModule() =default;
        virtual ~PlayGamesModule() =default;

        virtual void run() override;
    };

    PlayGamesModule::Init<PlayGamesModule> init;
}

/************************************************************************/

void PlayGamesModule::playGame(std::shared_ptr<const PlayGame> message)
{
    auto games=std::make_unique<Steam::CMsgClientGamesPlayedMessageType>();
    games->content.set_client_os_type(static_cast<std::underlying_type_t<Steam::OSType>>(Steam::getOSType()));

    auto game=games->content.add_games_played();
    {
        SteamBot::GameID gameId;
        gameId.setAppType(SteamBot::GameID::AppType::App);
        gameId.setAppId(message->appId);
        game->set_game_id(gameId.getValue());
    }

    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(games));
}

/************************************************************************/

void PlayGamesModule::run()
{
    getClient().launchFiber("PlayGames::run", [this](){
        SteamBot::Waiter waiter;
        auto cancellation=getClient().cancel.registerObject(waiter);

        std::shared_ptr<SteamBot::Messageboard::Waiter<PlayGame>> playGameQueue;
        playGameQueue=waiter.createWaiter<decltype(playGameQueue)::element_type>(getClient().messageboard);

        while (true)
        {
            waiter.wait();
            {
                auto message=playGameQueue->fetch();
                if (message)
                {
                    playGame(std::move(message));
                }
            }
        }
    });
}

/************************************************************************/

void PlayGame::play(SteamBot::AppID appId)
{
    auto message=std::make_shared<PlayGame>();
    message->appId=appId;
    SteamBot::Client::getClient().messageboard.send(std::move(message));
}
