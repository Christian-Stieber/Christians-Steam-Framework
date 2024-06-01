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
/*
 * When playing games, we make a short interruption every
 * "updateInterval" and request a playtime update from Steam.
 */

static constinit std::chrono::minutes updateInterval(10);
static constinit std::chrono::seconds updateDuration{5};

/************************************************************************/

typedef SteamBot::Modules::PlayGames::Messageboard::PlayGames PlayGames;
typedef SteamBot::Modules::PlayGames::Whiteboard::PlayingGames PlayingGames;

/************************************************************************/

typedef std::chrono::steady_clock Clock;

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
        class GameInfo
        {
        public:
            SteamBot::AppID appId;
            Clock::time_point nextUpdate;
            bool paused=false;

        public:
            GameInfo(SteamBot::AppID appId_, Clock::time_point now)
                : appId(appId_), nextUpdate(now+updateInterval)
            {
            }
        };

    private:
        SteamBot::Messageboard::WaiterType<PlayGames> playGamesWaiter;

        std::vector<GameInfo> games;

    private:
        Clock::time_point getNextUpdate() const;

    private:
        static void requestUpdates(std::vector<SteamBot::AppID>&&);

        void handleUpdates();
        void startGames(const std::vector<SteamBot::AppID>&);
        void stopGames(const std::vector<SteamBot::AppID>&);
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

Clock::time_point PlayGamesModule::getNextUpdate() const
{
    Clock::time_point result{Clock::time_point::max()};
    for (const auto& game: games)
    {
        if (game.nextUpdate<result)
        {
            result=game.nextUpdate;
        }
    }
    return result;
}

/************************************************************************/

void PlayGamesModule::reportGames() const
{
    std::vector<SteamBot::AppID> appIds;
    appIds.reserve(games.size());

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
        for (const auto& game : games)
        {
            output << separator;
            separator=", ";
            output << game.appId;

            appIds.push_back(game.appId);
        }
    }

    if (appIds.empty())
    {
        getClient().whiteboard.clear<PlayingGames>();
    }
    else
    {
        getClient().whiteboard.set<PlayingGames>(std::move(appIds));
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

    for (const auto& game : games)
    {
        if (!game.paused)
        {
            auto gameMessage=message->content.add_games_played();
            {
                SteamBot::GameID gameId;
                gameId.setAppType(SteamBot::GameID::AppType::App);
                gameId.setAppId(game.appId);
                gameMessage->set_game_id(gameId.getValue());
            }
        }
    }

    SteamBot::Modules::Connection::Messageboard::SendSteamMessage::send(std::move(message));
}

/************************************************************************/

void PlayGamesModule::startGames(const std::vector<SteamBot::AppID>& appIds)
{
    auto now=Clock::now();
    
    bool didChange=false;
    for (auto appId: appIds)
    {
        bool alreadyPlaying=false;
        {
            for (const auto& game: games)
            {
                if (appId==game.appId)
                {
                    alreadyPlaying=true;
                    break;
                }
            }
        }
        if (!alreadyPlaying)
        {
            games.emplace_back(appId, now);
            didChange=true;
        }
    }

    if (didChange)
    {
        sendGames();
        reportGames();
    }
}

/************************************************************************/

void PlayGamesModule::requestUpdates(std::vector<SteamBot::AppID>&& appIds)
{
    if (!appIds.empty())
    {
        auto updateMessage=std::make_shared<SteamBot::Modules::OwnedGames::Messageboard::UpdateGames>(std::move(appIds));
        getClient().messageboard.send(std::move(updateMessage));
    }
}

/************************************************************************/

void PlayGamesModule::stopGames(const std::vector<SteamBot::AppID>& appIds)
{
    std::vector<SteamBot::AppID> updates;
    SteamBot::erase(games, [&appIds, &updates](const GameInfo& game) {
        for (auto appId: appIds)
        {
            if (appId==game.appId)
            {
                // if a game is paused, we've already requested an update
                if (!game.paused)
                {
                    updates.push_back(appId);
                }
                return true;
            }
        }
        return false;
    });

    if (!updates.empty())
    {
        sendGames();
        reportGames();
        requestUpdates(std::move(updates));
    }
}

/************************************************************************/

void PlayGamesModule::handle(std::shared_ptr<const PlayGames> message)
{
    BOOST_LOG_TRIVIAL(debug) << "received PlayGames request: " << message->toJson();

    if (message->start)
    {
        startGames(message->appIds);
    }
    else
    {
        stopGames(message->appIds);
    }
}

/************************************************************************/

void PlayGamesModule::handleUpdates()
{
    if (!games.empty())
    {
        auto now=Clock::now();

        std::vector<SteamBot::AppID> updates;

        bool didChange=false;
        for (auto& game: games)
        {
            if (game.nextUpdate<=now)
            {
                if (game.paused)
                {
                    // SteamBot::UI::OutputText() << "unpausing " << game.appId;
                    game.paused=false;
                    game.nextUpdate=now+updateInterval;
                }
                else
                {
                    // SteamBot::UI::OutputText() << "pausing " << game.appId;
                    updates.push_back(game.appId);
                    game.paused=true;
                    game.nextUpdate=now+updateDuration;
                }
                didChange=true;
            }
        }

        if (didChange)
        {
            sendGames();
        }

        requestUpdates(std::move(updates));
    }
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
        waiter->wait<Clock>(getNextUpdate());
        playGamesWaiter->handle(this);
        handleUpdates();
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
