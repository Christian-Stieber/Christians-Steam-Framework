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
    class ExitCommand : public CLI::CLICommandBase
    {
    public:
        ExitCommand(CLI& cli_)
            : CLICommandBase(cli_, "EXIT", "", "Exit the bot", false)
        {
        }

        virtual ~ExitCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>& words) override
        {
            if (words.size()>1) return false;

            cli.quit=true;
            return true;
        }
    };

    ExitCommand::InitClass<ExitCommand> init;
}

/************************************************************************/

void SteamBot::UI::CLI::useExitCommand()
{
}
