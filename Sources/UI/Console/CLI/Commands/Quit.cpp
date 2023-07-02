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
#include "Modules/Executor.hpp"

#include "../Helpers.hpp"

/************************************************************************/

namespace
{
    class QuitCommand : public CLI::CLICommandBase
    {
    public:
        QuitCommand(CLI& cli_)
            : CLICommandBase(cli_, "quit", "", "Quit a client", true)
        {
        }

        virtual ~QuitCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    QuitCommand::InitClass<QuitCommand> init;
}

/************************************************************************/

bool QuitCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    if (words.size()==1)
    {
        if (auto client=clientInfo->getClient())
        {
            bool success=SteamBot::Modules::Executor::execute(client, [](SteamBot::Client& client) {
                client.quit(false);
            });
            if (success)
            {
                std::cout << "requested client \"" << client->getClientInfo().accountName << "\" to terminate" << std::endl;
            }
        }
        if (cli.currentAccount==clientInfo)
        {
            cli.currentAccount=nullptr;
        }
        return true;
    }
    else
    {
        return false;
    }
}

/************************************************************************/

void SteamBot::UI::CLI::useQuitCommand()
{
}
