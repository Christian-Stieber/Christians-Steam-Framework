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

#include <unordered_map>
#include <optional>
#include <string>
#include <memory>

/************************************************************************/

namespace SteamBot
{
    namespace Modules
    {
        namespace GetPageData
        {
            void use();

            namespace Whiteboard
            {
                // The whiteboard gets an BadgeData::Ptr

                class BadgeData
                {
                public:
                    typedef std::shared_ptr<const BadgeData> Ptr;

                public:
                    class BadgeInfo
                    {
                    public:
                        std::optional<unsigned int> level;
                        std::optional<unsigned int> cardsReceived;
                        std::optional<unsigned int> cardsRemaining;

                    public:
                        virtual ~BadgeInfo();

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
