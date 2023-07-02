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

#include "Modules/Executor.hpp"
#include "Modules/AddFreeLicense.hpp"

/************************************************************************/

typedef SteamBot::Modules::AddFreeLicense::Messageboard::AddLicense AddLicense;

/************************************************************************/

namespace
{
    class AddLicenseCommand : public CLI::CLICommandBase
    {
    public:
        AddLicenseCommand(CLI& cli_)
            : CLICommandBase(cli_, "add-license", "<appid>", "add a (free) license", true)
        {
        }

        virtual ~AddLicenseCommand() =default;

    public:
        virtual bool execute(SteamBot::ClientInfo*, std::vector<std::string>&) override;
    };

    AddLicenseCommand::InitClass<AddLicenseCommand> init;
}

/************************************************************************/

bool AddLicenseCommand::execute(SteamBot::ClientInfo* clientInfo, std::vector<std::string>& words)
{
    if (words.size()==2)
    {
        uint64_t appId;
        if (CLI::Helpers::parseNumber(words[1], appId))
        {
            if (auto client=clientInfo->getClient())
            {
                bool success=SteamBot::Modules::Executor::execute(std::move(client), [appId](SteamBot::Client&) mutable {
                    AddLicense::add(static_cast<SteamBot::AppID>(appId));
                });

                if (success)
                {
                    std::cout << "I've asked Steam to add the license " << appId << " to your account" << std::endl;
                }
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

/************************************************************************/

void SteamBot::UI::CLI::useAddLicenseCommand()
{
}
