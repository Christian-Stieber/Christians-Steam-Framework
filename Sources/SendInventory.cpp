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
#include "Settings.hpp"
#include "Modules/Inventory.hpp"
#include "UI/UI.hpp"
#include "Client/ClientInfo.hpp"

/************************************************************************/

typedef SteamBot::Inventory::Inventory Inventory;
typedef SteamBot::SendTrade SendTrade;

/************************************************************************/

static const uint32_t itemsPerTrade=100;

/************************************************************************/

namespace
{
    class SendInventoryPartner : public SteamBot::Settings::SettingBotName
    {
    public:
        using SettingBotName::SettingBotName;

    private:
        virtual const std::string_view& name() const override
        {
            static const std::string_view string{"send-inventory-recipient"};
            return string;
        }

        virtual void storeWhiteboard(Ptr<> setting) const override
        {
            SteamBot::Settings::Internal::storeWhiteboard<SendInventoryPartner>(std::move(setting));
        }
    };

    SteamBot::Settings::Init<SendInventoryPartner> init;
}

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

bool SteamBot::sendInventory(const SteamBot::ClientInfo* partner)
{
    bool result=false;

    if (partner==nullptr)
    {
        partner=SteamBot::Client::getClient().whiteboard.get<SendInventoryPartner::Ptr<SendInventoryPartner>>()->clientInfo;
    }

    if (partner!=nullptr)
    {
        SteamBot::UI::OutputText() << "sending inventory to " << partner->accountName;
        if (auto inventory=SteamBot::Inventory::get())
        {
            SendTrade sendTrade;
            sendTrade.partner=partner;
            sendTrade.myItems=collectItems(*inventory);
            result=sendTrade.send();
        }
    }

    return result;
}
