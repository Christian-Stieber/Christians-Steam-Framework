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

#include "UI/CLI.hpp"
#include "../Helpers.hpp"

#include "AssetData.hpp"
#include "Helpers/StringCompare.hpp"
#include "Modules/Inventory.hpp"
#include "Modules/Executor.hpp"

#include <regex>

/************************************************************************/

typedef SteamBot::Modules::Inventory::Whiteboard::Inventory Inventory;
typedef SteamBot::Modules::Inventory::InventoryItem InventoryItem;

/************************************************************************/

namespace
{
    class ListInventoryCommand : public CLI::CLICommandBase
    {
    public:
        ListInventoryCommand(CLI& cli_)
            : CLICommandBase(cli_, "list-inventory", "[<regex>]", "list inventory", true)
        {
        }

        virtual ~ListInventoryCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    ListInventoryCommand::InitClass<ListInventoryCommand> init;
}

/************************************************************************/

static Inventory::Ptr getInventory(const SteamBot::ClientInfo& clientInfo)
{
    Inventory::Ptr result;
    if (auto client=clientInfo.getClient())
    {
        SteamBot::Modules::Executor::execute(std::move(client), [&result](SteamBot::Client& client) mutable {
            if (auto inventory=client.whiteboard.has<Inventory::Ptr>())
            {
                result=*inventory;
            }
        });
    }
    return result;
}

/************************************************************************/

static void outputInventory(SteamBot::ClientInfo& clientInfo, const Inventory& inventory, std::string_view pattern)
{
    class Item
    {
    public:
        std::shared_ptr<const InventoryItem> inventoryItem;
        std::shared_ptr<const SteamBot::AssetData::AssetInfo> assetInfo;

    public:
        Item(decltype(inventoryItem) inventoryItem_, decltype(assetInfo) assetInfo_)
            : inventoryItem(std::move(inventoryItem_)), assetInfo(std::move(assetInfo_))
        {
        }
    };

    std::vector<Item> items;
    {
        std::regex regex(pattern.begin(), pattern.end(), std::regex_constants::icase);
        for (const auto& item : inventory.items)
        {
            if (auto assetInfo=SteamBot::AssetData::query(item))
            {
                if (std::regex_search(assetInfo->name, regex) || std::regex_search(assetInfo->type, regex))
                {
                    items.emplace_back(item, assetInfo);
                }
            }
        }
    }

    std::sort(items.begin(), items.end(), [](const Item& left, const Item& right) -> bool {
        auto result=SteamBot::caseInsensitiveStringCompare(left.assetInfo->type, right.assetInfo->type);
        if (result==decltype(result)::equivalent)
        {
            result=SteamBot::caseInsensitiveStringCompare(left.assetInfo->name, right.assetInfo->name);
        }
        return result==std::weak_ordering::less;
    });

    for (const auto& item : items)
    {
        std::cout << item.inventoryItem->appId;
        std::cout << "/" << item.inventoryItem->contextId;
        std::cout << "/" << item.inventoryItem->assetId;
        std::cout << ": ";

        if (item.inventoryItem->amount>1)
        {
            std::cout << item.inventoryItem->amount << "× ";
        }

        std::cout << "\"" << item.assetInfo->type << "\" / \"" << item.assetInfo->name << "\"\n";
    }
    std::cout << std::flush;
}

/************************************************************************/

bool ListInventoryCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    std::string_view pattern;

    if (words.size()==2)
    {
        pattern=words[1];
    }
    else if (words.size()==1)
    {
    }
    else
    {
        return false;
    }

    if (auto inventory=getInventory(*clientInfo))
    {
        outputInventory(*clientInfo, *inventory, pattern);
    }
    else
    {
        std::cout << "inventory not available for \"" << clientInfo->accountName << "\"" << std::endl;
    }

    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useListInventoryCommand()
{
    useLoadInventoryCommand();
}
