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

/************************************************************************/

namespace
{
    class LaunchCommand : public CLI::CLICommandBase
    {
    public:
        LaunchCommand(CLI& cli_)
            : CLICommandBase(cli_, "launch", "", "launch an existing client", true)
        {
        }

        virtual ~LaunchCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    LaunchCommand::InitClass<LaunchCommand> init;
}

/************************************************************************/

bool LaunchCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    if (words.size()==1)
    {
        SteamBot::Client::launch(*clientInfo);
        std::cout << "launched client \"" << clientInfo->accountName << "\"" << std::endl;
        std::cout << "NOTE: leave command mode to be able to see password/SteamGuard prompts!" << std::endl;

        cli.currentAccount=clientInfo;
        std::cout << "your current account is now \"" << cli.currentAccount->accountName << "\"" << std::endl;

        return true;
    }
    else
    {
        return false;
    }
}

/************************************************************************/

void SteamBot::UI::CLI::useLaunchCommand()
{
}
