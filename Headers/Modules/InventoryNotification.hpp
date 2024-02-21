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

#include "Modules/ClientNotification.hpp"
#include "Modules/Inventory.hpp"
#include "AssetData.hpp"

/************************************************************************/
/*
 * This is sent when an ClientNotification of type InventoryItem was
 * received; it adds some data to make it actually useful.
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace InventoryNotification
        {
            void use();

            namespace Messageboard
            {
                class InventoryNotification
                {
                public:
                    std::shared_ptr<const SteamBot::Modules::ClientNotification::Messageboard::ClientNotification> clientNotification;
                    std::shared_ptr<const SteamBot::Inventory::ItemKey> itemKey;
                    std::shared_ptr<const SteamBot::AssetData::AssetInfo> assetInfo;

                public:
                    InventoryNotification(decltype(clientNotification), decltype(itemKey), decltype(assetInfo));
                    ~InventoryNotification();

                public:
                    boost::json::value toJson() const;
                };
            }
        }
    }
}
