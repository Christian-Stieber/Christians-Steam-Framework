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
    namespace Modules
    {
        namespace Inventory
        {
            class InventoryItem : public SteamBot::AssetKey
            {
            public:
                int32_t contextId=-1;
                uint64_t assetId=0;
                uint32_t amount=0;

            public:
                InventoryItem();
                virtual ~InventoryItem();

                bool init(const boost::json::object&);

                virtual boost::json::value toJson() const override;
            };

            namespace Messageboard
            {
                class LoadInventory
                {
                public:
                    LoadInventory();
                    ~LoadInventory();
                };
            }

            namespace Whiteboard
            {
                // Note: we store a Inventory::Ptr in the whiteboard
                class Inventory
                {
                public:
                    std::chrono::system_clock::time_point when;
                    std::vector<std::unique_ptr<InventoryItem>> items;

                public:
                    Inventory();
                    ~Inventory();
                };
            }
        }
    }
}
