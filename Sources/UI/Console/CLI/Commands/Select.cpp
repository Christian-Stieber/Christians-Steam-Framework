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
    class SelectCommand : public CLI::CLICommandBase
    {
    public:
        SelectCommand(CLI& cli_)
            : CLICommandBase(cli_, "select", "<account>", "select a bot as target for some commands")
        {
        }

        virtual ~SelectCommand() =default;

    public:
        virtual bool execute(std::vector<std::string>&) override;
    };

    SelectCommand::InitClass<SelectCommand> init;
}


/************************************************************************/

bool SelectCommand::execute(std::vector<std::string>& words)
{
    if (words.size()==1)
    {
        if (cli.currentAccount==nullptr)
        {
            return false;
        }
        cli.currentAccount=nullptr;
        std::cout << "deselected the current account" << std::endl;
    }
    else if (words.size()==2)
    {
        const auto clientInfo=cli.getAccount(words[1]);
        if (clientInfo!=nullptr)
        {
            cli.currentAccount=clientInfo;
            std::cout << "your current account is now \"" << cli.currentAccount->accountName << "\"" << std::endl;
        }
    }
    else
    {
        return false;
    }
    return true;
}

/************************************************************************/

void SteamBot::UI::CLI::useSelectCommand()
{
}
