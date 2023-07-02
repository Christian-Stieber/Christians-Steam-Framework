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

/************************************************************************/

namespace
{
    class CreateCommand : public CLI::CLICommandBase
    {
    public:
        CreateCommand(CLI& cli_)
            : CLICommandBase(cli_, "create", "<account>", "create a new bot", false)
        {
        }

        virtual ~CreateCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    CreateCommand::InitClass<CreateCommand> init;
}

/************************************************************************/

bool CreateCommand::execute(SteamBot::ClientInfo*, std::vector<std::string>& words)
{
    if (words.size()!=2) return false;

    const auto clientInfo=SteamBot::ClientInfo::create(std::string(words[1]));
    if (clientInfo==nullptr)
    {
        std::cout << "account \"" << words[1] << "\" already exists" << std::endl;
    }
    else
    {
        SteamBot::Client::launch(*clientInfo);
        std::cout << "launched new client \"" << clientInfo->accountName << "\"" << std::endl;
        std::cout << "NOTE: leave command mode to be able to see password/SteamGuard prompts!" << std::endl;

        cli.currentAccount=clientInfo;
        std::cout << "your current account is now \"" << cli.currentAccount->accountName << "\"" << std::endl;
    }
    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useCreateCommand()
{
}
