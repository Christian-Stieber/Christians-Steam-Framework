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

#include "Modules/DiscoveryQueue.hpp"
#include "Modules/Executor.hpp"

/************************************************************************/

typedef SteamBot::Modules::DiscoveryQueue::Messageboard::ClearQueue ClearQueue;

/************************************************************************/

namespace
{
    class ClearQueueCommand : public CLI::CLICommandBase
    {
    public:
        ClearQueueCommand(CLI& cli_)
            : CLICommandBase(cli_, "clear-queue", "[<account>]", "clear a discovery queue")
        {
        }

        virtual ~ClearQueueCommand() =default;

    public:
        virtual bool execute(std::vector<std::string>&) override;
    };

    ClearQueueCommand::InitClass<ClearQueueCommand> init;
}

/************************************************************************/

bool ClearQueueCommand::execute(std::vector<std::string>& words)
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
                client.messageboard.send(std::make_shared<ClearQueue>());
            });
            if (success)
            {
                std::cout << "requested queue clearing for account " << client->getClientInfo().accountName << std::endl;
            }
        }
    }
    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useClearQueueCommand()
{
}
