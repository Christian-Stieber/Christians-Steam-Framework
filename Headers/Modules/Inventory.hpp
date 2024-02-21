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

#include "AssetKey.hpp"

#include <chrono>
#include <vector>

/************************************************************************/

namespace SteamBot
{
    namespace Inventory
    {
        class ItemKey
        {
        public:
            SteamBot::AppID appId=SteamBot::AppID::None;
            SteamBot::ContextID contextId=SteamBot::ContextID::None;
            SteamBot::AssetID assetId=SteamBot::AssetID::None;

        public:
            ItemKey();
            ItemKey(const boost::json::value&);
            virtual ~ItemKey();

        public:
            virtual boost::json::value toJson() const;
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace Inventory
    {
        class Item : public ItemKey, public SteamBot::AssetKey
        {
        public:
            using ItemKey::appId;
            uint32_t amount=0;

        public:
            Item(const boost::json::value&);
            virtual ~Item();

            bool init(const boost::json::object&);

            virtual boost::json::value toJson() const override;
        };
    }
}

/************************************************************************/
/*
 * In most cases, you'll be using the get() call to get the inventory
 * data; this will fetch the data from Steam as needed.
 *
 * I'm still storing an Inventory::Ptr into the whiteboard, but I
 * don't see much use for that.
 */

namespace SteamBot
{
    namespace Inventory
    {
        class Inventory
        {
        public:
            typedef std::shared_ptr<const Inventory> Ptr;

        public:
            std::chrono::system_clock::time_point when;
            std::vector<std::shared_ptr<const Item>> items;

        public:
            Inventory();
            ~Inventory();
        };

        std::shared_ptr<const Inventory> get();
    }
}
