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

#include <chrono>

/************************************************************************/
/*
 * ClaimResult status codes:
 *   Ok -> we have claimed a sticker
 *   AlreadyClaimed -> we have claimed the sticker before
 *   NoSale -> we could not find a claimable sticker
 *   Error -> we have encountered an error
 *
 * "nextClaimTime" might be faked in some situations:
 *   NoSale -> a fabricated "10am Valve Time"
 *   Error -> some time after the attempt
 *
 * "errorCount" is the number of consecutive errors
 * we have gotten; it's use to calculate a good
 * nextClaimTime.
 *
 * "item" will be filled in on "Ok" status
 */

#pragma once

#include "Steam/CommunityItemClass.hpp"

/************************************************************************/

namespace SteamBot
{
    namespace SaleSticker
    {
        class Status
        {
        public:
            enum ClaimResult {
                Invalid,
                Ok,
                AlreadyClaimed,
                NoSale,
                Error
            };

        public:
            class Item
            {
            public:
                SteamBot::CommunityItemClass type=SteamBot::CommunityItemClass::Invalid;

                std::string name;
                std::string title;
                std::string description;

            public:
                Item();
                ~Item();

                boost::json::value toJson() const;
            };

        public:
            std::chrono::system_clock::time_point when;

            ClaimResult result=ClaimResult::Invalid;
            unsigned int errorCount=0;
            std::chrono::system_clock::time_point nextClaimTime;

            Item item;

        public:
            Status();
            ~Status();

        public:
            boost::json::value toJson() const;
        };

        // Points into the whiteboard, so it's valid until yield
        const Status& claim();
    }
}
