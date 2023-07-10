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

#include "Client/Client.hpp"
#include "Modules/Inventory.hpp"
#include "MiscIDs.hpp"

#include <vector>

/************************************************************************/

namespace SteamBot
{
    class SendTrade
    {
    public:
        class Item : public SteamBot::Inventory::ItemKey
        {
        public:
            Item();
            ~Item();

        public:
            Item(const SteamBot::Inventory::Item&);

        public:
            uint32_t amount=0;
        };

    public:
        SendTrade();
        ~SendTrade();

    public:
        ClientInfo* partner;
        std::vector<Item> myItems;
        std::vector<Item> theirItems;

    public:
        bool send() const;
    };
}
