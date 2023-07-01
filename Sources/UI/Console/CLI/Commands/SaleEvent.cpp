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

#include "Modules/SaleQueue.hpp"
#include "Modules/SaleSticker.hpp"
#include "Modules/Executor.hpp"

/************************************************************************/

typedef SteamBot::Modules::SaleQueue::Messageboard::ClearSaleQueues ClearSaleQueues;
typedef SteamBot::Modules::SaleSticker::Messageboard::ClaimSaleSticker ClaimSaleSticker;

/************************************************************************/

namespace
{
    class SaleEventCommand : public CLI::CLICommandBase
    {
    public:
        SaleEventCommand(CLI& cli_)
            : CLICommandBase(cli_, "sale-event", "[<account>]", "clear sale queues and stickers")
        {
        }

        virtual ~SaleEventCommand() =default;

    public:
        virtual bool execute(std::vector<std::string>&) override;
    };

    SaleEventCommand::InitClass<SaleEventCommand> init;
}

/************************************************************************/

bool SaleEventCommand::execute(std::vector<std::string>& words)
{
    SteamBot::ClientInfo* clientInfo=nullptr;

    if (words.size()==1)
    {
        clientInfo=cli.getAccount();
    }
    else if (words.size()==2)
    {
        clientInfo=cli.getAccount(words[1]);
    }
    else
    {
        return false;
    }

    if (clientInfo!=nullptr)
    {
        if (auto client=clientInfo->getClient())
        {
            bool success=SteamBot::Modules::Executor::execute(client, [](SteamBot::Client& client) {
                client.messageboard.send(std::make_shared<ClearSaleQueues>());
                client.messageboard.send(std::make_shared<ClaimSaleSticker>());
            });
            if (success)
            {
                std::cout << "requested sale queue clearing and sticker claiming for account " << client->getClientInfo().accountName << std::endl;
            }
        }
    }
    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useSaleEventCommand()
{
}
