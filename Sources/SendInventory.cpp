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

#include "SendInventory.hpp"
#include "SendTrade.hpp"
#include "AssetData.hpp"
#include "Modules/Inventory.hpp"

/************************************************************************/

typedef SteamBot::Modules::Inventory::Whiteboard::Inventory Inventory;
typedef SteamBot::SendTrade SendTrade;

/************************************************************************/

static const uint32_t itemsPerTrade=100;

/************************************************************************/
/*
 * Returns the first "itemsPerTrade" tradable items
 */

static std::vector<SendTrade::Item> collectItems(const Inventory& inventory)
{
    std::vector<SendTrade::Item> items;
    for (const auto& item: inventory.items)
    {
        if (auto assetInfo=SteamBot::AssetData::query(item))
        {
            if (assetInfo->isTradable)
            {
                items.emplace_back(*item);
                if (items.size()>=itemsPerTrade)
                {
                    break;
                }
            }
        }
    }
    return items;
}

/************************************************************************/

bool SteamBot::sendInventory(SteamBot::ClientInfo* partner)
{
    if (auto inventory=SteamBot::Client::getClient().whiteboard.has<Inventory::Ptr>())
    {
        SendTrade sendTrade;
        sendTrade.partner=partner;
        sendTrade.myItems=collectItems(**inventory);
        sendTrade.send();
    }
    return false;
}
