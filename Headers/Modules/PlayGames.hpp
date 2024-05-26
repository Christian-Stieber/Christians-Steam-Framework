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
 * The PlayGames module lets you start and stop "playing" games.
 *
 * Of course, it doesn't launch anything; it just tells Steam that
 * you're "playing" the game.
 *
 * In addition, since I don't currently know whether I can get Steam
 * to update me on playtimes, I'm occasionally requesting a data
 * update from OwnedGames for the games that are being "played"
 * through this module. This includes requesting a an update when a
 * game is stopped -- since that's when Steam seems to reliably update
 * its data.
 */

/************************************************************************/

#pragma once

#include "MiscIDs.hpp"

#include <vector>

#include <boost/json/value.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace PlayGames
        {
            namespace Messageboard
            {
                class PlayGame
                {
                public:
                    AppID appId=AppID::None;
                    bool start=true;

                public:
                    PlayGame();
                    ~PlayGame();
                    boost::json::value toJson() const;

                public:
                    static void play(AppID, bool);
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
        namespace PlayGames
        {
            namespace Whiteboard
            {
                class PlayingGames : public std::vector<SteamBot::AppID>
                {
                public:
                    PlayingGames(const std::vector<SteamBot::AppID>&);
                    ~PlayingGames();
                };
            }
        }
    }
}
