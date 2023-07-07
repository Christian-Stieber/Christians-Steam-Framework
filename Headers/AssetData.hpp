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
#include "Modules/TradeOffers.hpp"
#include "DerefStuff.hpp"

#include <string>
#include <memory>
#include <unordered_set>

#include <boost/json/value.hpp>

/************************************************************************/

namespace SteamBot
{
    namespace AssetData
    {
        class AssetInfo : public SteamBot::AssetKey
        {
        public:
            enum class ItemType { Unknown, TradingCard, Emoticon, Gems };

        public:
            ItemType itemType=ItemType::Unknown;

            std::string name;
            std::string type;
            SteamBot::AppID marketFeeApp=SteamBot::AppID::None;

        public:
            AssetInfo(const boost::json::value&);
            virtual ~AssetInfo();

            virtual boost::json::value toJson() const;
        };

        // Some types
        typedef std::shared_ptr<SteamBot::AssetKey> KeyPtr;

        // Store an existing asset-data chunk (like from the Inventory code) into the data
        void store(const boost::json::value&);

        // Fetche any missing items
        typedef std::unordered_set<KeyPtr, SteamBot::SmartDerefStuff<KeyPtr>::Hash, SteamBot::SmartDerefStuff<KeyPtr>::Equals> KeySet;
        void fetch(const KeySet&);

        // Query
        std::shared_ptr<AssetInfo> query(KeyPtr);
    }
}
