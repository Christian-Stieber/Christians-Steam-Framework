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

typedef SteamBot::Modules::PlayGames::Messageboard::PlayGames PlayGames;
typedef SteamBot::Modules::PlayGames::Whiteboard::PlayingGames PlayingGames;

/************************************************************************/

PlayGames::PlayGames() =default;
PlayGames::~PlayGames() =default;

/************************************************************************/

boost::json::value PlayGames::toJson() const
{
    boost::json::object json;
    {
        boost::json::array array;
        for (SteamBot::AppID appId : appIds)
        {
            array.emplace_back(toInteger(appId));
        }
        json["appIds"]=std::move(array);
    }
    json["action"]=start ? "start" : "stop";
    return json;
}

/************************************************************************/

namespace
{
    class PlayGamesModule : public SteamBot::Client::Module
    {
    private:
        SteamBot::Messageboard::WaiterType<PlayGames> playGamesWaiter;

        std::vector<SteamBot::AppID> games;

    private:
        std::chrono::steady_clock::time_point lastUpdate;	// used for the attempt to update the playtime

    private:
        typedef decltype(lastUpdate)::clock UpdateClock;

        UpdateClock::time_point getNextUpdate() const
        {
            return lastUpdate+std::chrono::minutes(5);
        }

    private:
        void performUpdate();
        void reportGames() const;
        void sendGames() const;

    public:
        void handle(std::shared_ptr<const PlayGames>);

    public:
        PlayGamesModule() =default;
        virtual ~PlayGamesModule() =default;

        virtual void init(SteamBot::Client&) override;
        virtual void run(SteamBot::Client&) override;
    };

    PlayGamesModule::Init<PlayGamesModule> init;
}

/************************************************************************/

void PlayGamesModule::reportGames() const
{
    SteamBot::UI::OutputText output;
    output << "PlayGames: (presumably) playing ";

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
            output << appId;
        }
    }
}

/************************************************************************/

PlayingGames::PlayingGames(const std::vector<SteamBot::AppID>& other)
    : std::vector<SteamBot::AppID>(other)
{
}

PlayingGames::~PlayingGames() =default;

/************************************************************************/

void PlayGamesModule::sendGames() const
{
    auto message=std::make_unique<Steam::CMsgClientGamesPlayedMessageType>();

    // ToDo: should we use a different OS type?
    message->content.set_client_os_type(static_cast<uint32_t>(SteamBot::toInteger(Steam::getOSType())));

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

    if (games.empty())
    {
        getClient().whiteboard.clear<PlayingGames>();
    }
    else
    {
        getClient().whiteboard.set<PlayingGames>(games);
    }

    reportGames();
}

/************************************************************************/

void PlayGamesModule::handle(std::shared_ptr<const PlayGames> message)
{
    BOOST_LOG_TRIVIAL(debug) << "received PlayGames request: " << message->toJson();

    std::vector<SteamBot::AppID> stopped;
    bool somethingChanged=false;

    if (message->start)
    {
        for (const SteamBot::AppID playAppId: message->appIds)
        {
            bool alreadyPlaying=false;
            {
                for (const SteamBot::AppID playingAppId: games)
                {
                    if (playAppId==playingAppId)
                    {
                        alreadyPlaying=true;
                        break;
                    }
                }
            }
            if (!alreadyPlaying)
            {
                games.push_back(playAppId);
                somethingChanged=true;
            }
        }
    }
    else
    {
        auto count=SteamBot::erase(games, [message,&stopped](const SteamBot::AppID playingAppId) {
            for (const SteamBot::AppID stopAppId: message->appIds)
            {
                if (playingAppId==stopAppId)
                {
                    stopped.push_back(stopAppId);
                    return true;
                }
            }
            return false;
        });

        assert(count==stopped.size());
        somethingChanged=(count>0);
    }

    if (somethingChanged)
    {
        sendGames();
    }

    if (!message->start)
    {
        auto updateMessage=std::make_shared<SteamBot::Modules::OwnedGames::Messageboard::UpdateGames>(std::move(stopped));
        getClient().messageboard.send(std::move(updateMessage));
    }
}

/************************************************************************/

void PlayGamesModule::performUpdate()
{
    if (!games.empty())
    {
        if (UpdateClock::now()>=getNextUpdate())
        {
            auto updateMessage=std::make_shared<SteamBot::Modules::OwnedGames::Messageboard::UpdateGames>(games);
            getClient().messageboard.send(std::move(updateMessage));
            lastUpdate=UpdateClock::now();
            return;
        }
    }
    assert(false);
}

/************************************************************************/

void PlayGamesModule::init(SteamBot::Client& client)
{
    playGamesWaiter=client.messageboard.createWaiter<PlayGames>(*waiter);
}

/************************************************************************/

void PlayGamesModule::run(SteamBot::Client&)
{
    waitForLogin();

    while (true)
    {
        playGamesWaiter->handle(this);
        if (games.empty())
        {
            waiter->wait();
        }
        else
        {
            if (!waiter->wait<UpdateClock>(getNextUpdate()))
            {
                performUpdate();
            }
        }
    }
}

/************************************************************************/

void PlayGames::play(std::vector<SteamBot::AppID> appIds, bool start)
{
    auto message=std::make_shared<PlayGames>();
    message->appIds=std::move(appIds);
    message->start=start;
    SteamBot::Client::getClient().messageboard.send(std::move(message));
}
