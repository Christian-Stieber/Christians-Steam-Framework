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

#include "Client/Client.hpp"
#include "Modules/Executor.hpp"
#include "SendInventory.hpp"

/************************************************************************/

namespace
{
    class SendInventoryCommand : public CLI::CLICommandBase
    {
    public:
        SendInventoryCommand(CLI& cli_)
            : CLICommandBase(cli_, "send-inventory", "<recipient>", "send tradable items", true)
        {
        }

        virtual ~SendInventoryCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    SendInventoryCommand::InitClass<SendInventoryCommand> init;
}

/************************************************************************/

bool SendInventoryCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    if (words.size()==2)
    {
        if (auto partner=SteamBot::ClientInfo::find(words[1]))
        {
            bool success=false;
            if (auto client=clientInfo->getClient())
            {
                SteamBot::Modules::Executor::execute(std::move(client), [partner, &success](SteamBot::Client& client) {
                    success=SteamBot::sendInventory(partner);
                });
            }
            if (!success)
            {
                std::cout << "failed to send inventory" << std::endl;
            }
        }
        else
        {
            std::cout << "unknown account \"" << words[1] << "\"" << std::endl;
        }
        return true;
    }
    else
    {
        return false;
    }
}

/************************************************************************/

void SteamBot::UI::CLI::useSendInventoryCommand()
{
}
