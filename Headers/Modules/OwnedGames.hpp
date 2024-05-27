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

#pragma once

#include "MiscIDs.hpp"

#include <string>
#include <unordered_map>
#include <memory>

#include <boost/json/value.hpp>

/************************************************************************/
/*
 * This will monitor the license list, and retrieve information about
 * owned games from Steam.
 *
 * It will also monitor the played games (for now, only games that we
 * are playing ourselves), and update the playtime information.
 *
 * Note: updates to specific games, like updating playtime, will not
 * create a new whitelist item. You can look for GameChanged messages,
 * though.
 */

/************************************************************************/
/*
 * This is sent when data for a game changed. For now(?), this happens
 * only in response to update requests, or new license data coming in.
 *
 * Note that (last time I checked), Steam only updates playtime data
 * every 30 minutes for an active game, or when the game is stopped.
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace OwnedGames
        {
            namespace Messageboard
            {
                class GameChanged
                {
                public:
                    SteamBot::AppID appId;
                    bool newGame=false;

                public:
                    GameChanged(SteamBot::AppID appId_, bool newGame_)
                        : appId(appId_), newGame(newGame_)
                    {
                    }
                };
            }
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace OwnedGames
        {
            void use();

            namespace Whiteboard
            {
                // Note: the whiteboard actually holds an OwnedGames::Ptr!!!!

                class OwnedGames
                {
                public:
                    typedef std::shared_ptr<const OwnedGames> Ptr;

                public:
                    class GameInfo
                    {
                    public:
                        GameInfo();
                        ~GameInfo();

                    public:
                        AppID appId;
                        std::string name;
                        std::chrono::system_clock::time_point lastPlayed;
                        std::chrono::minutes playtimeForever{0};

                    public:
                        boost::json::value toJson() const;

                        std::strong_ordering operator<=>(const GameInfo&) const;
                        bool operator==(const GameInfo&) const;
                    };

                public:
                    typedef std::vector<std::shared_ptr<const SteamBot::Modules::OwnedGames::Messageboard::GameChanged>> ChangeList;
                    ChangeList getGames_(const std::vector<SteamBot::AppID>* appIds=nullptr);

                public:
                    std::unordered_map<AppID, std::shared_ptr<const GameInfo>> games;

                public:
                    std::shared_ptr<const GameInfo> getInfo(AppID) const;

                public:
                    OwnedGames();
                    ~OwnedGames();
                    boost::json::value toJson() const;
                };
            }

            std::shared_ptr<const Whiteboard::OwnedGames::GameInfo> getInfo(AppID);
        }
    }
}

/************************************************************************/
/*
 * Send this to request re-checking specific games
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace OwnedGames
        {
            namespace Messageboard
            {
                class UpdateGames
                {
                public:
                    std::vector<SteamBot::AppID> appIds;

                public:
                    UpdateGames(std::vector<SteamBot::AppID> _={});
                    ~UpdateGames();
                };
            }
        }
    }
}
