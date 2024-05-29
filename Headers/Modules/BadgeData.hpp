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

#include <unordered_map>
#include <string>
#include <memory>
#include <chrono>

#include <boost/json/value.hpp>

/************************************************************************/
/*
 * Note: for now, BadgeData is enabled/disabled through the CardFarmer
 * enable/disable setting. This is because I'm too lazy to add a
 * system where multiple clients can indicate that they want BadgeData
 * -- so we would automatically supply the date while at least one
 * client still wants it.
 *
 * Rright now, CardFarmer is the only client anyway.
 */

/************************************************************************/

namespace HTMLParser
{
    namespace Tree
    {
        class Element;
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace BadgeData
        {
            void use();

            namespace Whiteboard
            {
                class BadgeData
                {
                public:
                    typedef std::shared_ptr<const BadgeData> Ptr;

                public:
                    class BadgeInfo
                    {
                    public:
                        std::chrono::system_clock::time_point when{std::chrono::system_clock::now()};

                        unsigned int cardsEarned;		// how many cards are we entitled to?
                        unsigned int cardsReceived;		// how many have we received already?

                    public:
                        BadgeInfo();
                        ~BadgeInfo();

                        SteamBot::AppID init(const HTMLParser::Tree::Element&);

                        boost::json::value toJson() const;
                    };

                public:
                    std::unordered_map<AppID, BadgeInfo> badges;

                public:
                    BadgeData();
                    ~BadgeData();

                    boost::json::value toJson() const;
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
        namespace BadgeData
        {
            namespace Messageboard
            {
                class UpdateBadge
                {
                public:
                    AppID appId;

                public:
                    UpdateBadge(AppID appId_)
                        : appId(appId_)
                    {
                    }

                public:
                    static void update(AppID);
                };
            }
        }
    }
}
