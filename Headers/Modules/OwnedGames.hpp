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
#include "Printable.hpp"

#include <string>
#include <unordered_map>
#include <memory>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace OwnedGames
        {
            namespace Whiteboard
            {
                // Note: the whiteboard actually holds an OwnedGames::Ptr!!!!

                class OwnedGames : public Printable
                {
                public:
                    typedef std::shared_ptr<const OwnedGames> Ptr;

                public:
                    class GameInfo : public Printable
                    {
                    public:
                        GameInfo();
                        virtual ~GameInfo();
                        virtual boost::json::value toJson() const override;

                    public:
                        AppID appId;
                        std::string name;
                        std::chrono::system_clock::time_point lastPlayed;
                        std::chrono::minutes playtimeForever{0};
                    };

                public:
                    std::unordered_map<AppID, std::shared_ptr<const GameInfo>> games;

                public:
                    OwnedGames();
                    virtual ~OwnedGames();
                    virtual boost::json::value toJson() const override;
                };
            }
        }
    }
}
